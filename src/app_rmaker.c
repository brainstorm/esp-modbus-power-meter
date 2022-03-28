#include <stdio.h>
#include <stdint.h>

#include <esp_log.h>

#include <app_wifi.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>
#include <esp_insights.h>
#include <app_insights.h>

#include "app_rmaker.h"

static const char *TAG = "app_rmaker";
esp_rmaker_device_t *power_sensor_device;

void app_rmaker_init() {
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
    esp_err_t err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}

void send_to_rmaker_cloud(int cid, float value, const esp_rmaker_device_t *device) {
    switch (cid)
    {
        case 3: {
            assert(device != NULL);
            esp_rmaker_param_update_and_report(
                // esp_rmaker_device_get_param_by_type();
                esp_rmaker_device_get_param_by_name(device, "Watts"),
                esp_rmaker_int((int)value));

            // Copy the same Watts value (most important) to this other attribute so that it
            // shows nicely on the RainMaker mobile app(s)
            esp_rmaker_param_update_and_report(
                // esp_rmaker_device_get_param_by_type();
                esp_rmaker_device_get_param_by_name(device, "Power Meter"),
                esp_rmaker_int((int)value));

        }
        break;
    
    default:
        break;
    }
}