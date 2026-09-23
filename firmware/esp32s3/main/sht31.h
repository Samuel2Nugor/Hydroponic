#ifndef SHT31_H
#define SHT31_H

#include "esp_err.h"

esp_err_t sht31_init(void);

esp_err_t sht31_read(
    float *temperature_c,
    float *humidity_percent
);

#endif
