#pragma once

#include <stdio.h>
#include <stdint.h>

#include <esp_log.h>

#include <esp_rmaker_core.h>
#include <esp_modbus_master.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>
#include <esp_insights.h>
#include <app_wifi.h>
#include <app_insights.h>

extern esp_rmaker_device_t *power_sensor_device;

void app_rmaker_init();
void create_rmaker_secondary_parameters();
void send_to_rmaker_cloud(uint16_t cid, float value, const esp_rmaker_device_t *device);