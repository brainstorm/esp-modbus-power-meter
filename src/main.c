#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <nvs_flash.h>

#include "app_modbus.h"
#include "app_rmaker.h"
#include "app_rgbled.h"
#include "app_pvoutput_org.h"

static const char *TAG = "app_main";

// To pass around task notification/syncronization logic and read Modbus Data
TaskHandle_t modbus_task = NULL;
TaskHandle_t pvoutput_task = NULL;
TaskHandle_t rainmaker_task = NULL;

extern const mb_parameter_descriptor_t device_parameters[];
struct mb_reporting_unit_t mb_readings[15];

// XXX: Hack hack hack
float g_watts = -1.0;
float g_volts = -1.0;

void app_main(void)
{
    /* Initialize NVS, useful for WiFi calibration, time keeping among other systems */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    
    ESP_LOGI(TAG, "All systems go");
    #if CONFIG_RMAKER_SERVICE_ENABLE
    app_rmaker_init();      /* Initialize all things ESP RainMaker Cloud and ESP Insights */
    #endif
    app_pvoutput_init();    /* PVoutput.org: initialize after RMaker (system clock (SNTP) set) */
    app_modbus_init();      /* Initialize the power meter */
    //app_rgbled_init();
}