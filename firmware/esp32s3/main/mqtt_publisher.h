#pragma once

#include "esp_err.h"

esp_err_t mqtt_publisher_start(void);

esp_err_t mqtt_publisher_publish_raw(
    const char *payload
);
