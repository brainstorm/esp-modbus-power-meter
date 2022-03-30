#include "app_rmaker.h"
#include "modbus_params.h"

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

    // Set already in sdkconfig (by default)
    //esp_rmaker_time_set_timezone(CONFIG_ESP_RMAKER_DEF_TIMEZONE);

    /* Create a device and add the relevant parameters to it */
    power_sensor_device = esp_rmaker_power_meter_sensor_device_create("Power", NULL, 0);
    esp_rmaker_node_add_device(node, power_sensor_device);

    // Add the rest of the power meter info: Amps, Volts, Frequency, Power Factor, etc...
    //create_rmaker_secondary_parameters(power_sensor_device, get_modbus_parameter_descriptor_table());
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

/* The primary parameter is Power (as in Watts), but there are ~14 other (secondary) parameters
   defined for this power meter, see CID table on app_modbus.c */
void create_rmaker_secondary_parameters(){
    esp_param_property_flags_t params_flags = PROP_FLAG_READ | PROP_FLAG_WRITE | PROP_FLAG_TIME_SERIES | PROP_FLAG_PERSIST;
    esp_rmaker_param_create("Amps_phase_1", "A", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("Amps_phase_2", "A", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("Amps_phase_3", "A", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("Volts_phase_1", "V", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("Volts_phase_2", "V", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("Volts_phase_3", "V", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("Power_Factor", NULL, esp_rmaker_float(0.0), params_flags); // Ratio, unit-less
    esp_rmaker_param_create("Frequency", "Hz", esp_rmaker_float(0.0), params_flags);
    // Less understood/verified values
    esp_rmaker_param_create("var", "W", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("VA", "W", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("uh", "Wh?", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("-uh", "Wh?", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("uAh", "Ah", esp_rmaker_float(0.0), params_flags);
    esp_rmaker_param_create("-uAh", "Ah", esp_rmaker_float(0.0), params_flags);
}

/* Sends parameters collected on the ModBus table towards rainmaker cloud */
void send_to_rmaker_cloud(uint16_t cid, float value, const esp_rmaker_device_t *device) {
    switch (cid)
    {
        case 0: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "Amps_phase_1"),
                esp_rmaker_int(value));
        }
        break;
        case 1: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "Amps_phase_2"),
                esp_rmaker_int(value));
        }
        break;
        case 2: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "Amps_phase_3"),
                esp_rmaker_int(value));
        }
        break;
        case 3: {
            // Watts (most important param) is sent to the root parameter so that it
            // shows nicely on the RainMaker mobile app(s) without opening the device properties.
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "Power Meter"),
                esp_rmaker_int(value));
        }
        break;
        case 4: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "var"),
                esp_rmaker_int(value));
        }
        break;
        case 5: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "VA"),
                esp_rmaker_int(value));
        }
        break;
        case 6: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "Volts_phase_1"),
                esp_rmaker_int(value));
        }
        break;
        case 7: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "Volts_phase_2"),
                esp_rmaker_int(value));
        }
        break;
        case 8: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "Volts_phase_3"),
                esp_rmaker_int(value));
        }
        break;
        case 9: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "Power_Factor"),
                esp_rmaker_int(value));
        }
        break;
        case 10: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "Frequency"),
                esp_rmaker_int(value));
        }
        break;
        case 11: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "uh"),
                esp_rmaker_int(value));
        }
        break;
        case 12: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "-uh"),
                esp_rmaker_int(value));
        }
        break;
        case 13: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "uAh"),
                esp_rmaker_int(value));
        }
        break;
        case 14: {
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(device, "-uAh"),
                esp_rmaker_int(value));
        }
        break;
    default:
        break;
    }
}