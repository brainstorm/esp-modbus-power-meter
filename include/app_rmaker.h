#pragma once

#include <esp_rmaker_core.h>

extern esp_rmaker_device_t *power_sensor_device;

void app_rmaker_init();
void send_to_rmaker_cloud(int cid, float value, const esp_rmaker_device_t *device);