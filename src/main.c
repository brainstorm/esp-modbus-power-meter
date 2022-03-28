#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>

#include <app_wifi.h>
#include <app_insights.h>
#include <esp_insights.h>

#include "app_priv.h"

static const char *TAG = "app_main";

esp_rmaker_device_t *power_sensor_device;

void app_main(void)
{
    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    //app_rgbled_init();
    xTaskCreate(app_modbus_init, "modbus_task", 16384, NULL, 5, NULL);
    //app_modbus_init();

    // TODO?
    // #define WIFI_RESET_BUTTON_TIMEOUT       3
    // #define FACTORY_RESET_BUTTON_TIMEOUT    10
    // // This is the button that is used for toggling the power
    // #define BUTTON_GPIO          0
    // #define BUTTON_ACTIVE_LEVEL  0
    // // This is the GPIO on which the power will be set
    // #define OUTPUT_GPIO    19


    // app_reset_button_register(app_reset_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL),
    //             WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init()
     */
    app_wifi_init();
    
    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Power meter");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Create a device and add the relevant parameters to it */
    power_sensor_device = esp_rmaker_power_meter_sensor_device_create("Power", NULL, 0);
    esp_rmaker_node_add_device(node, power_sensor_device);

    // TODO: The primary parameter is power, but there are ~14 other (secondary) parameters
    // defined for this power meter, see CID table on app_modbus.c
    esp_rmaker_device_add_param(power_sensor_device, esp_rmaker_power_meter_param_create("Watts", 0));
    // esp_rmaker_device_add_param(power_sensor_device, esp_rmaker_power_meter_param_create("Volts", 0));

    // Enable RMaker OTA updates: https://rainmaker.espressif.com/docs/ota.html
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    //esp_rmaker_ota_enable(&ota_config, OTA_USING_PARAMS);
    esp_rmaker_ota_enable(&ota_config, OTA_USING_TOPICS);

    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    app_insights_enable();
    
    // ESP insights is different than *app_insights*. The former is the espressif cloud thing, the latter
    // are the IC's firmware application logs.
    esp_insights_config_t config  = {
        .log_type = ESP_DIAG_LOG_TYPE_EVENT,
    };

    esp_insights_init(&config);

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}