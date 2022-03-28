#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "app_priv.h"
#include "app_rmaker.h"

//static const char *TAG = "app_main";

void app_main(void)
{
    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    //app_rgbled_init();
    xTaskCreate(app_modbus_init, "modbus_task", 16384, NULL, 5, NULL);
    //app_modbus_init();

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    
    /* Initialize all things ESP RainMaker Cloud and ESP Insights */
    app_rmaker_init();
}