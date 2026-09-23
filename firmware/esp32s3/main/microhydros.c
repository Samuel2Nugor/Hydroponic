#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "esp_chip_info.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_random.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "mqtt_publisher.h"
#include "nvs_flash.h"
#include "onewire_bus.h"
#include "onewire_device.h"
#include "sht31.h"
#include "wifi_manager.h"

static const char *TAG = "microhydros";

#define WATER_DS18B20_ROM UINT64_C(0xBE0000006DEFD428)
#define EXTERNAL_DS18B20_ROM UINT64_C(0xDA00000070118128)
#define ONEWIRE_GPIO GPIO_NUM_5

typedef struct {
    uint64_t rom;
    ds18b20_device_handle_t device;
    float temperature_c;
    const char *status;
    bool valid;
} ds18b20_sensor_t;

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

static void format_temperature_value(
    const ds18b20_sensor_t *sensor,
    char *buffer,
    size_t buffer_size
)
{
    if (sensor->valid) {
        snprintf(buffer, buffer_size, "%.2f", sensor->temperature_c);
    } else {
        snprintf(buffer, buffer_size, "null");
    }
}

static void prime_onewire_bus(void)
{
    gpio_reset_pin(ONEWIRE_GPIO);
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(ONEWIRE_GPIO, 1);
    esp_rom_delay_us(1000);

    gpio_set_level(ONEWIRE_GPIO, 0);
    esp_rom_delay_us(480);
    gpio_set_level(ONEWIRE_GPIO, 1);
    esp_rom_delay_us(480);

    gpio_reset_pin(ONEWIRE_GPIO);
}

static void read_ds18b20_sensors(
    ds18b20_sensor_t *water_sensor,
    ds18b20_sensor_t *external_sensor
)
{
    /*
     * The UART backend needs an initial reset transaction on this ESP32-S3
     * before its first ROM search. Without it, the first search reports no
     * participating devices even though both probes are present.
     */
    prime_onewire_bus();

    onewire_bus_config_t bus_config = {
        .bus_gpio_num = ONEWIRE_GPIO,
        .flags = {
            .en_pull_up = true,
        },
    };

    onewire_bus_uart_config_t uart_config = {
        .uart_port_num = 1,
    };

    onewire_bus_handle_t bus = NULL;
    esp_err_t result = onewire_new_bus_uart(
        &bus_config,
        &uart_config,
        &bus
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "1-Wire bus setup failed: %s", esp_err_to_name(result));
        return;
    }

    onewire_device_iter_handle_t iter = NULL;
    result = onewire_new_device_iter(bus, &iter);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "1-Wire iterator failed: %s", esp_err_to_name(result));
        onewire_bus_del(bus);
        return;
    }

    int device_count = 0;
    onewire_device_t discovered_device;

    while (
        (result = onewire_device_iter_get_next(
            iter,
            &discovered_device
        )) == ESP_OK
    ) {
        const uint64_t rom = (uint64_t)discovered_device.address;
        ds18b20_sensor_t *sensor = NULL;

        ESP_LOGI(TAG, "1-Wire ROM: %016" PRIX64, rom);

        if (rom == water_sensor->rom) {
            sensor = water_sensor;
        } else if (rom == external_sensor->rom) {
            sensor = external_sensor;
        } else {
            ESP_LOGW(TAG, "Unknown 1-Wire device ignored: %016" PRIX64, rom);
        }

        if (sensor != NULL) {
            ds18b20_config_t device_config = {};
            const esp_err_t device_result =
                ds18b20_new_device_from_enumeration(
                    &discovered_device,
                    &device_config,
                    &sensor->device
                );

            if (device_result == ESP_OK) {
                sensor->status = "read_error";
            } else {
                ESP_LOGE(
                    TAG,
                    "DS18B20 registration failed for ROM %016" PRIX64 ": %s",
                    rom,
                    esp_err_to_name(device_result)
                );
            }
        }

        device_count++;
    }

    if (result == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG, "1-Wire scan complete: %d device(s)", device_count);
    } else {
        ESP_LOGE(TAG, "1-Wire scan failed: %s", esp_err_to_name(result));
    }

    onewire_del_device_iter(iter);

    if (water_sensor->device == NULL && external_sensor->device == NULL) {
        ESP_LOGE(TAG, "No configured DS18B20 device available for reading");
        onewire_bus_del(bus);
        return;
    }

    result = ds18b20_trigger_temperature_conversion_for_all(bus);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "DS18B20 conversion failed: %s", esp_err_to_name(result));
        return;
    }

    ds18b20_sensor_t *sensors[] = {
        water_sensor,
        external_sensor,
    };

    for (size_t i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
        ds18b20_sensor_t *sensor = sensors[i];

        if (sensor->device == NULL) {
            continue;
        }

        result = ds18b20_get_temperature(
            sensor->device,
            &sensor->temperature_c
        );

        if (result != ESP_OK) {
            ESP_LOGE(
                TAG,
                "DS18B20 ROM %016" PRIX64 " read failed: %s",
                sensor->rom,
                esp_err_to_name(result)
            );
            continue;
        }

        sensor->valid = true;
        sensor->status = "ok";

        ESP_LOGI(
            TAG,
            "DS18B20 ROM %016" PRIX64 " temperature: %.2f C",
            sensor->rom,
            sensor->temperature_c
        );
    }
}

static esp_err_t publish_telemetry(
    float internal_temperature_c,
    float internal_humidity_percent,
    const ds18b20_sensor_t *water_sensor,
    const ds18b20_sensor_t *external_sensor
)
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
    char water_temperature_value[24];
    char external_temperature_value[24];

    format_temperature_value(
        water_sensor,
        water_temperature_value,
        sizeof(water_temperature_value)
    );

    format_temperature_value(
        external_sensor,
        external_temperature_value,
        sizeof(external_temperature_value)
    );

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
                "\"internal_temperature_c\":%.2f,"
                "\"internal_humidity_percent\":%.2f,"
                "\"external_temperature_c\":%s,"
                "\"water_temperature_c\":%s"
            "},"
            "\"sensor_status\":{"
                "\"internal_sht31\":\"ok\","
                "\"external_ds18b20\":\"%s\","
                "\"water_ds18b20\":\"%s\""
            "}"
        "}",
        boot_id,
        sequence,
        uptime_ms,
        internal_temperature_c,
        internal_humidity_percent,
        external_temperature_value,
        water_temperature_value,
        external_sensor->status,
        water_sensor->status
    );

    if (
        payload_length < 0 ||
        (size_t)payload_length >= sizeof(payload)
    ) {
        ESP_LOGE(
            TAG,
            "Telemetry payload buffer is too small"
        );
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(
        TAG,
        "Publishing telemetry: boot_id=%s, sequence=%" PRIu32,
        boot_id,
        sequence
    );

    ESP_LOGI(
        TAG,
        "Internal SHT31 telemetry: temperature=%.2f C humidity=%.2f %%",
        internal_temperature_c,
        internal_humidity_percent
    );

    return mqtt_publisher_publish_raw(payload);
}

static esp_err_t synchronize_time(void)
{
    esp_sntp_config_t config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

    esp_err_t result = esp_netif_sntp_init(&config);

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "SNTP initialization failed: %s",
            esp_err_to_name(result)
        );
        return result;
    }

    ESP_LOGI(TAG, "Waiting for network time");

    result = esp_netif_sntp_sync_wait(
        pdMS_TO_TICKS(60000)
    );

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Time synchronization failed: %s",
            esp_err_to_name(result)
        );
        esp_netif_sntp_deinit();
        return result;
    }

    time_t now;
    struct tm utc_time;
    char time_text[32];

    time(&now);
    gmtime_r(&now, &utc_time);

    strftime(
        time_text,
        sizeof(time_text),
        "%Y-%m-%dT%H:%M:%SZ",
        &utc_time
    );

    ESP_LOGI(
        TAG,
        "Time synchronized: %s",
        time_text
    );

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

    ESP_LOGI(
        TAG,
        "MicroHydros firmware started"
    );

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

    const esp_err_t sht31_result = sht31_init();

    if (sht31_result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "SHT31 initialization failed: %s",
            esp_err_to_name(sht31_result)
        );
        return;
    }

    ESP_LOGI(
        TAG,
        "SHT31 initialization succeeded"
    );

    float internal_temperature_c;
    float internal_humidity_percent;

    const esp_err_t read_result = sht31_read(
        &internal_temperature_c,
        &internal_humidity_percent
    );

    if (read_result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "SHT31 read failed: %s",
            esp_err_to_name(read_result)
        );
        return;
    }

    ds18b20_sensor_t water_sensor = {
        .rom = WATER_DS18B20_ROM,
        .status = "not_detected",
    };

    ds18b20_sensor_t external_sensor = {
        .rom = EXTERNAL_DS18B20_ROM,
        .status = "not_detected",
    };

    read_ds18b20_sensors(
        &water_sensor,
        &external_sensor
    );

    const esp_err_t wifi_result =
        wifi_manager_connect();

    if (wifi_result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Wi-Fi initialization failed: %s",
            esp_err_to_name(wifi_result)
        );
        return;
    }

    ESP_LOGI(
        TAG,
        "Wi-Fi connection is ready"
    );

    const esp_err_t time_result =
        synchronize_time();

    if (time_result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "MQTT startup stopped: time is not synchronized"
        );
        return;
    }

    const esp_err_t mqtt_result =
        mqtt_publisher_start();

    if (mqtt_result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "MQTT initialization failed: %s",
            esp_err_to_name(mqtt_result)
        );
        return;
    }

    const esp_err_t publish_result =
        publish_telemetry(
            internal_temperature_c,
            internal_humidity_percent,
            &water_sensor,
            &external_sensor
        );

    if (publish_result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Telemetry publication failed: %s",
            esp_err_to_name(publish_result)
        );
        return;
    }

    ESP_LOGI(
        TAG,
        "Telemetry was queued successfully"
    );
}
