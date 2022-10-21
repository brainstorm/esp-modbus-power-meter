#include "app_rmaker.h"
#include "app_modbus.h"

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
        .enable_time_sync = true,
    };

    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Power meter");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    // /* Enable timezone service which will be require for setting appropriate timezone
    //  * from the phone apps for scheduling to work correctly.
    //  * For more information on the various ways of setting timezone, please check
    //  * https://rainmaker.espressif.com/docs/time-service.html.
    //  */
    esp_rmaker_timezone_service_enable();

    /* Create a device and add the relevant parameters to it */
    power_sensor_device = esp_rmaker_power_meter_sensor_device_create("Power", NULL, 0);
    esp_rmaker_node_add_device(node, power_sensor_device);

    // Add the rest of the power meter info: Amps, Volts, Frequency, Power Factor, etc...
    create_rmaker_secondary_parameters(power_sensor_device);

    // Enable RMaker OTA updates: https://rainmaker.espressif.com/docs/ota.html
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    //esp_rmaker_ota_enable(&ota_config, OTA_USING_PARAMS);
    esp_rmaker_ota_enable(&ota_config, OTA_USING_TOPICS);

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

void create_rmaker_secondary_parameters(esp_rmaker_device_t *device){
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("Amps_phase_1", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("Amps_phase_2", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("Amps_phase_3", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("Volts_phase_1", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("Volts_phase_2", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("Volts_phase_3", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("Power_Factor", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("Frequency", 0.0));
    // Less understood nor verified values
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("var", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("VA", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("uh", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("-uh", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("uAh", 0.0));
    esp_rmaker_device_add_param(device, esp_rmaker_power_meter_param_create("-uAh", 0.0));
}

/* Sends parameters collected on the ModBus table towards rainmaker cloud */
void send_to_rmaker_cloud(uint16_t cid, float value, const esp_rmaker_device_t *device) {
    //    for (uint8_t cid = 0; *mb_readings[cid].cid; cid++) {
        switch (cid)
        {
            case 0: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "Amps_phase_1"),
                    esp_rmaker_float(value));
            }
            break;
            case 1: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "Amps_phase_2"),
                    esp_rmaker_float(value));
            }
            break;
            case 2: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "Amps_phase_3"),
                    esp_rmaker_float(value));
            }
            break;
            case 3: {
                // Watts (most important param) is sent to the root parameter so that it
                // shows nicely on the RainMaker mobile app(s) without opening the device properties.
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "Power Meter"),
                    esp_rmaker_float(value));
            }
            break;
            case 4: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "var"),
                    esp_rmaker_float(value));
            }
            break;
            case 5: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "VA"),
                    esp_rmaker_float(value));
            }
            break;
            case 6: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "Volts_phase_1"),
                    esp_rmaker_float(value));
            }
            break;
            case 7: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "Volts_phase_2"),
                    esp_rmaker_float(value));
            }
            break;
            case 8: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "Volts_phase_3"),
                    esp_rmaker_float(value));
            }
            break;
            case 9: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "Power_Factor"),
                    esp_rmaker_float(value));
            }
            break;
            case 10: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "Frequency"),
                    esp_rmaker_float(value));
            }
            break;
            case 11: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "uh"),
                    esp_rmaker_float(value));
            }
            break;
            case 12: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "-uh"),
                    esp_rmaker_float(value));
            }
            break;
            case 13: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "uAh"),
                    esp_rmaker_float(value));
            }
            break;
            case 14: {
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param_by_name(device, "-uAh"),
                    esp_rmaker_float(value));
            }
            break;
        default:
            break;
        }
    //}

    // We are done reporting data to RMaker cloud, let's wait until more Modbus Data values are available...
    xTaskNotifyGive(modbus_task);
}