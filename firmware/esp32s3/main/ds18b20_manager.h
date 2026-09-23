#ifndef DS18B20_MANAGER_H
#define DS18B20_MANAGER_H

#include <stdbool.h>

#include "esp_err.h"

typedef struct {
    float water_temperature_c;
    float external_temperature_c;
    const char *water_status;
    const char *external_status;
    bool water_valid;
    bool external_valid;
} ds18b20_readings_t;

esp_err_t ds18b20_manager_init(void);

esp_err_t ds18b20_manager_read(
    ds18b20_readings_t *readings
);

#endif
