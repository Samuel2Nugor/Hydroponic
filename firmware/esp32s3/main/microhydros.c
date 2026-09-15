#include <stddef.h>

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "nvs_flash.h"

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

    ESP_LOGI(TAG, "Wi-Fi connection is ready");
}
