#pragma once
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "esp_http_client.h"

// To be able to send (task) notifications to modbus for syncronization
extern TaskHandle_t pvoutput_task;

void pvoutput_update();
int app_pvoutput_init();