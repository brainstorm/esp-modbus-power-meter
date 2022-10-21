#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "modbus_params.h"

// To be able to send (task) notifications to modbus for syncronization
extern TaskHandle_t modbus_task;

// Holding only those three attributes, assuming floats across the board
// mb_parameter_descriptor_t has many info we do not use
struct mb_reporting_unit_t {
    const char* unit;
    const char* key;
    float value;
};

extern struct mb_reporting_unit_t mb_readings[15];

esp_err_t app_modbus_init();