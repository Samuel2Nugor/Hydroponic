#include <stddef.h>

#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "microhydros";

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    const size_t psram_total_bytes = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    
    const size_t psram_free_bytes = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        
    ESP_LOGI(TAG, "Microhydros firmware started");
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
}
