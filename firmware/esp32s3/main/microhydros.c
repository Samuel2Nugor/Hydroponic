#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "esp_netif_sntp.h"

#include "mqtt_publisher.h"
#include "wifi_manager.h"

static const char *TAG = "microhydros";

static void initialize_nvs(void)
{
    esp_err_t result = nvs_flash_init();

    if (
        result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND
    ) {
        ESP_LOGW(TAG, "Erasing incompatible NVS data");
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }

    ESP_ERROR_CHECK(result);
}

static esp_err_t publish_synthetic_telemetry(void)
{
    char boot_id[9];

    snprintf(
        boot_id,
        sizeof(boot_id),
        "%08" PRIx32,
        esp_random()
    );

    const uint32_t sequence = 0;
    const int64_t uptime_ms = esp_timer_get_time() / 1000;

    char payload[512];

    const int payload_length = snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"schema_version\":1,"
        "\"device_id\":\"esp32s3-01\","
        "\"boot_id\":\"%s\","
        "\"sequence\":%" PRIu32 ","
        "\"uptime_ms\":%" PRIi64 ","
        "\"measurements\":{"
            "\"internal_temperature_c\":23.6,"
            "\"internal_humidity_percent\":61.4,"
            "\"external_temperature_c\":18.9,"
            "\"water_temperature_c\":20.7"
        "},"
        "\"sensor_status\":{"
            "\"internal_sht31\":\"ok\","
            "\"external_sht31\":\"ok\","
            "\"water_ds18b20\":\"ok\""
        "}"
        "}",
        boot_id,
        sequence,
        uptime_ms
    );

    if (
        payload_length < 0 ||
        (size_t)payload_length >= sizeof(payload)
    ) {
        ESP_LOGE(TAG, "Telemetry payload buffer is too small");
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(
        TAG,
        "Publishing synthetic telemetry: boot_id=%s, sequence=%" PRIu32,
        boot_id,
        sequence
    );

    return mqtt_publisher_publish_raw(payload);
}

static esp_err_t synchronize_time(void)
{
    esp_sntp_config_t config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

    esp_err_t result = esp_netif_sntp_init(&config);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "SNTP initialization failed: %s",
                 esp_err_to_name(result));
        return result;
    }

    ESP_LOGI(TAG, "Waiting for network time");

    result = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(60000));

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Time synchronization failed: %s",
                 esp_err_to_name(result));
        esp_netif_sntp_deinit();
        return result;
    }

    time_t now;
    struct tm utc_time;
    char time_text[32];

    time(&now);
    gmtime_r(&now, &utc_time);
    strftime(time_text, sizeof(time_text),
             "%Y-%m-%dT%H:%M:%SZ", &utc_time);

    ESP_LOGI(TAG, "Time synchronized: %s", time_text);
    return ESP_OK;
}

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    const size_t psram_total_bytes =
        heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    const size_t psram_free_bytes =
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    ESP_LOGI(TAG, "MicroHydros firmware started");
    ESP_LOGI(
        TAG,
        "CPU cores: %u, chip revision: %u",
        (unsigned int)chip_info.cores,
        (unsigned int)chip_info.revision
    );
    ESP_LOGI(
        TAG,
        "PSRAM heap: total=%zu bytes, free=%zu bytes",
        psram_total_bytes,
        psram_free_bytes
    );

    initialize_nvs();

    const esp_err_t wifi_result = wifi_manager_connect();

    if (wifi_result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Wi-Fi initialization failed: %s",
            esp_err_to_name(wifi_result)
        );
        return;
    }

    ESP_LOGI(TAG, "Wi-Fi connection is ready");

    const esp_err_t time_result = synchronize_time();

    if (time_result != ESP_OK) {
        ESP_LOGE(TAG, "MQTT startup stopped: time is not synchronized");
        return;
    }

    const esp_err_t mqtt_result = mqtt_publisher_start();

    if (mqtt_result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "MQTT initialization failed: %s",
            esp_err_to_name(mqtt_result)
        );
        return;
    }

    const esp_err_t publish_result =
        publish_synthetic_telemetry();

    if (publish_result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Telemetry publication failed: %s",
            esp_err_to_name(publish_result)
        );
        return;
    }

    ESP_LOGI(TAG, "Synthetic telemetry was queued successfully");
}
