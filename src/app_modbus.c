#include <stdio.h>
#include <stdint.h>

// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/semphr.h>

// ESP
#include "esp_log.h"            // for log_write
#include "esp_err.h"

// RainMaker
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 

// App specific
#include "app_modbus.h"
#include "app_rmaker.h"
#include "app_pvoutput_org.h"
#include "cid_tables.h"

static const char *TAG = "app_modbus";

// Holding reg init
holding_reg_params_t holding_reg_params = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// Calculate number of parameters in the table
const uint16_t num_device_parameters = (sizeof(device_parameters)/sizeof(device_parameters[0]));

// The number of parameters that intended to be used in the particular control process
#define MASTER_MAX_CIDS num_device_parameters

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

// Read power meter values over modbus, this is a FreeRTOS task
static void read_power_meter()
{
    esp_err_t err = ESP_OK;
    float current_value = 0;
    const mb_parameter_descriptor_t* param_descriptor = NULL;
    
    struct mb_reporting_unit_t mb_readings[MASTER_MAX_CIDS];
    
    while(1) {
        ESP_LOGI(TAG, "Reading modbus holding registers from power meter...");

        // Read all found characteristics from slave(s)
        for (uint16_t cid = 0; (err != ESP_ERR_NOT_FOUND) && cid < MASTER_MAX_CIDS; cid++)
        {
            // XXX: After a batch of modbus queries is read, the consumers must flag the
            // data as fetched to avoid race conditions.
            //
            // There shall be a mechanism for timeouts though, since ModBus values cannot be
            // stale just because there's a loss of connectivity or some cloud provider is down
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
                        ESP_LOGI(TAG, "Characteristic #%d %s (%s) value = %lf (0x%"PRIu32") read successful.",
                                        param_descriptor->cid,
                                        (char*)param_descriptor->param_key,
                                        (char*)param_descriptor->param_units,
                                        current_value,
                                        *(uint32_t*)temp_data_ptr);

                        // Add the informative triplets to the array, ready to report/send
                        mb_readings[cid].key = param_descriptor->param_key;
                        mb_readings[cid].value = current_value;
                        mb_readings[cid].unit = param_descriptor->param_units;

                        ESP_LOGI(TAG, "MB readings stored: for %s\n with value: %f\n", mb_readings[cid].key, mb_readings[cid].value);
                    }
                } else {
                    ESP_LOGE(TAG, "Characteristic #%d (%s) read fail, err = 0x%x (%s).",
                                        param_descriptor->cid,
                                        (char*)param_descriptor->param_key,
                                        (int)err,
                                        (char*)esp_err_to_name(err));
                }
                //vTaskDelay(MB_POLL_TIME_TICS); // time between polls
            }
        }
        // We might want to use *Indexed methods to avoid index 0, which clash with Streaming in FreeRTOS
        // 
        // "FreeRTOS Stream and Message Buffers use the task notification at array index 0. 
        // If you want to maintain the state of a task notification across a call to a Stream 
        // or Message Buffer API function then use a task notification at an array index greater than 0."

        //xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);
        vTaskDelay(MB_REPORTING_PERIOD);

        xTaskNotifyGive(pvoutput_task);
        //xTaskNotifyGive(rainmaker_task)
    }
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
                            "mb controller initialization fail, returns(0x%"PRIu32").",
                            (uint32_t)err);
    err = mbc_master_setup((void*)&comm);
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
                            "mb controller setup fail, returns(0x%"PRIu32").",
                            (uint32_t)err);

    // Set UART pin numbers
    err = uart_set_pin(MB_PORT_NUM, MB_UART_TXD, MB_UART_RXD,
                              UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
            "mb serial set pin failure, uart_set_pin() returned (0x%"PRIu32").", (uint32_t)err);

    err = mbc_master_start();
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
                            "mb controller start fail, returns(0x%"PRIu32").",
                            (uint32_t)err);
    
    // Set driver mode to Half Duplex
    err = uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX);
    //err = uart_set_mode(MB_PORT_NUM, UART_MODE_UART);
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
            "mb serial set mode failure, uart_set_mode() returned (0x%"PRIu32").", (uint32_t)err);

    vTaskDelay(5);
    err = mbc_master_set_descriptor(&device_parameters[0], num_device_parameters);
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                "mb controller set descriptor fail, returns(0x%"PRIu32").",
                                (uint32_t)err);
    
    ESP_LOGI(TAG, "Modbus master stack initialized...");
    return err;
}

esp_err_t app_modbus_init()
{
    mb_master_init();

    xTaskCreate(read_power_meter, "modbus_task", 16384, NULL, 5, &modbus_task);

    // The task above should never end, if it does, that's a fail :-!
    return ESP_FAIL;
}