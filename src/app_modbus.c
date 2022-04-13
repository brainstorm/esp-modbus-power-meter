#include <stdio.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

// ESP
#include "esp_log.h"            // for log_write
#include "esp_err.h"
#include "sdkconfig.h"

// FreeModbus
#include "cid_tables.h"

// RainMaker
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 

#include "app_modbus.h"
#include "app_rmaker.h"
#include "app_pvoutput_org.h"

static const char *TAG = "app_modbus";

holding_reg_params_t holding_reg_params = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

float g_current_volts = -0.1;
float g_current_watts = -0.1;

#define MB_REPORTING_PERIOD    60*1000*5/portTICK_PERIOD_MS /* Report every 5 minutes, to avoid rate limiting, especially on pvoutput.org */

// Newer ESP-IDF versions (>4.4) switch to MB_RETURN_ON_FALSE macro instead
#define MASTER_CHECK(a, ret_val, str, ...) \
    if (!(a)) { \
        ESP_LOGE(TAG, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        return (ret_val); \
    }

#define MB_UART_RXD         (CONFIG_MB_UART_RXD)
#define MB_UART_TXD         (CONFIG_MB_UART_TXD)
#define MB_PORT_NUM         (CONFIG_MB_UART_PORT_NUM)
#define MB_UART_SPEED       (CONFIG_MB_UART_BAUD_RATE)

// Number of reading of parameters from slave
#define MASTER_MAX_RETRY 30

// Timeout to update cid over Modbus
#define UPDATE_CIDS_TIMEOUT_MS          (500)
#define UPDATE_CIDS_TIMEOUT_TICS        (UPDATE_CIDS_TIMEOUT_MS / portTICK_PERIOD_MS)

// Timeout between polls
#define POLL_TIMEOUT_MS                 (1)
#define POLL_TIMEOUT_TICS               (POLL_TIMEOUT_MS / portTICK_PERIOD_MS)



// Calculate number of parameters in the table
const uint16_t num_device_parameters = (sizeof(device_parameters)/sizeof(device_parameters[0]));

// Get pointer to parameter storage (instance) according to parameter description table
static void* master_get_param_data(const mb_parameter_descriptor_t* param_descriptor)
{
    assert(param_descriptor != NULL);
    void* instance_ptr = NULL;
    if (param_descriptor->param_offset != 0) {
       switch(param_descriptor->mb_param_type)
       {
           case MB_PARAM_HOLDING:
               instance_ptr = ((void*)&holding_reg_params + param_descriptor->param_offset - 1);
               break;
           default:
               instance_ptr = NULL;
               break;
       }
    } else {
        ESP_LOGE(TAG, "Wrong parameter offset for CID #%d", param_descriptor->cid);
        assert(instance_ptr != NULL);
    }
    return instance_ptr;
}

// Read power meter values over modbus, report to rainmaker
static void read_power_meter()
{
    esp_err_t err = ESP_OK;
    float current_value = 0;
    const mb_parameter_descriptor_t* param_descriptor = NULL;

    while(1) {
        ESP_LOGI(TAG, "Reading modbus holding registers from power meter...");

        // Read all found characteristics from slave(s)
        for (uint16_t cid = 0; (err != ESP_ERR_NOT_FOUND) && cid < MASTER_MAX_CIDS; cid++)
        {
            // Get data from parameters description table
            // and use this information to fill the characteristics description table
            // to have all required fields in just one table
            err = mbc_master_get_cid_info(cid, &param_descriptor);
            if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL)) {
                void* temp_data_ptr = master_get_param_data(param_descriptor);
                assert(temp_data_ptr);
                uint8_t type = 0;

                err = mbc_master_get_parameter(cid, (char*)param_descriptor->param_key,
                                                    (uint8_t*)&current_value, &type);
                if (err == ESP_OK) {
                    *(float*)temp_data_ptr = current_value;
                    if (param_descriptor->mb_param_type == MB_PARAM_HOLDING) {
                        ESP_LOGI(TAG, "Characteristic #%d %s (%s) value = %lf (0x%x) read successful.",
                                        param_descriptor->cid,
                                        (char*)param_descriptor->param_key,
                                        (char*)param_descriptor->param_units,
                                        current_value,
                                        *(uint32_t*)temp_data_ptr);

                        // Send parameters collected from ModBus to RMaker cloud as parameters
                        // few seconds to avoid rate limiting?
                        // TODO: Batch and store those queries on a queue instead
                        vTaskDelay(1000/portTICK_PERIOD_MS);
                        send_to_rmaker_cloud(cid, current_value, power_sensor_device);

                        // Horrible hack: Both rmaker cloud and pvoutput are time-coupled now,
                        // I need to use queues and do some proper refactoring, running out of time now though :-S
                        if (cid == 3) { // Power (Watts)
                            g_current_watts = current_value;
                            pvoutput_update(current_value);
                        } else if (cid == 6) { // Volts phase 1
                            g_current_volts = current_value;
                            pvoutput_update(current_value);
                        }                    
                    }
                } else {
                    ESP_LOGE(TAG, "Characteristic #%d (%s) read fail, err = 0x%x (%s).",
                                        param_descriptor->cid,
                                        (char*)param_descriptor->param_key,
                                        (int)err,
                                        (char*)esp_err_to_name(err));
                }
                vTaskDelay(POLL_TIMEOUT_TICS); // timeout between polls
            }
        }
        vTaskDelay(MB_REPORTING_PERIOD);
    }
    // ESP_LOGI(TAG, "Destroy master...");
    // ESP_ERROR_CHECK(mbc_master_destroy());
}

// Modbus master initialization
static esp_err_t mb_master_init()
{
    // Initialize and start Modbus controller
    mb_communication_info_t comm = {
            .port = MB_PORT_NUM,
            .mode = MB_MODE_RTU,
            .baudrate = MB_UART_SPEED,
            .parity = MB_PARITY_NONE
    };
    void* master_handler = NULL;

    esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler);
    MASTER_CHECK((master_handler != NULL), ESP_ERR_INVALID_STATE,
                                "mb controller initialization fail.");
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
                            "mb controller initialization fail, returns(0x%x).",
                            (uint32_t)err);
    err = mbc_master_setup((void*)&comm);
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
                            "mb controller setup fail, returns(0x%x).",
                            (uint32_t)err);

    // Set UART pin numbers
    err = uart_set_pin(MB_PORT_NUM, MB_UART_TXD, MB_UART_RXD,
                              UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
            "mb serial set pin failure, uart_set_pin() returned (0x%x).", (uint32_t)err);

    err = mbc_master_start();
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
                            "mb controller start fail, returns(0x%x).",
                            (uint32_t)err);
    
    // Set driver mode to Half Duplex
    err = uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX);
    //err = uart_set_mode(MB_PORT_NUM, UART_MODE_UART);
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
            "mb serial set mode failure, uart_set_mode() returned (0x%x).", (uint32_t)err);

    vTaskDelay(5);
    err = mbc_master_set_descriptor(&device_parameters[0], num_device_parameters);
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                "mb controller set descriptor fail, returns(0x%x).",
                                (uint32_t)err);
    
    ESP_LOGI(TAG, "Modbus master stack initialized...");
    return err;
}

esp_err_t app_modbus_init()
{
    mb_master_init();

    xTaskCreate(read_power_meter, "modbus_task", 16384, NULL, 5, NULL);

    // The task above should never end, if it does, that's a fail :-!
    return ESP_FAIL;
}