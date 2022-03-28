#include <stdio.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

// ESP
#include "esp_log.h"            // for log_write
#include "esp_err.h"
#include "sdkconfig.h"

// FreeModbus
#include "mbcontroller.h"       // for mbcontroller defines and api
#include "modbus_params.h"      // for modbus parameters structures

// RainMaker
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 

#include "app_priv.h"
#include "app_rmaker.h"

static const char *TAG = "app_modbus";
holding_reg_params_t holding_reg_params = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// This macro is only useful for current, deprecated, 4.3.2 PlatformIO esp-idf version.
// Newer versions switch to MB_RETURN_ON_FALSE macro instead
#define MASTER_CHECK(a, ret_val, str, ...) \
    if (!(a)) { \
        ESP_LOGE(TAG, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        return (ret_val); \
    }

// TODO: Move this back to Kconfig parameters
#define CONFIG_MB_UART_RXD 8
#define CONFIG_MB_UART_TXD 6
#define CONFIG_MB_UART_RTS 7
// #define MB_PORT_NUM     (CONFIG_MB_UART_PORT_NUM)   // Number of UART port used for Modbus connection
// #define MB_DEV_SPEED    (CONFIG_MB_UART_BAUD_RATE)  // The communication speed of the UART
#define MB_PORT_NUM (1)
#define MB_DEV_SPEED (9600)

// Note: Some pins on target chip cannot be assigned for UART communication.
// See UART documentation for selected board and target to configure pins using Kconfig.

// Number of reading of parameters from slave
#define MASTER_MAX_RETRY 30

// Timeout to update cid over Modbus
#define UPDATE_CIDS_TIMEOUT_MS          (500)
#define UPDATE_CIDS_TIMEOUT_TICS        (UPDATE_CIDS_TIMEOUT_MS / portTICK_PERIOD_MS)

// Timeout between polls
#define POLL_TIMEOUT_MS                 (1)
#define POLL_TIMEOUT_TICS               (POLL_TIMEOUT_MS / portTICK_PERIOD_MS)

// The macro to get offset for parameter in the appropriate structure
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))

#define STR(fieldname) ((const char*)( fieldname ))
// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

// Enumeration of modbus device addresses accessed by master device
enum {
    MB_DEVICE_ADDR1 = 1, // Only one slave device used for the test (add other slave addresses here)
};

// Enumeration of all supported CIDs for device (used in parameter definition table)
enum {
    CID_HOLD_DATA_0 = 0,
    CID_HOLD_DATA_1,
    CID_HOLD_DATA_2,
    CID_HOLD_DATA_3,
    CID_HOLD_DATA_4,
    CID_HOLD_DATA_5,
    CID_HOLD_DATA_6,
    CID_HOLD_DATA_7,
    CID_HOLD_DATA_8,
    CID_HOLD_DATA_9,
    CID_HOLD_DATA_10,
    CID_HOLD_DATA_11,
    CID_HOLD_DATA_12,
    CID_HOLD_DATA_13,
    CID_HOLD_DATA_14,
    CID_COUNT
};

  /// Example values extracted from the power meter
  /// NOTE: Those holding reg offsets are valid for eModbus, for the present code,
  /// they are shifted slightly by 1-2 bytes :-S
  //
  //  0011:    0.193   <--- Amps panel 1
  //  0013:    0.258   <--- Amps panel 2
  //  0015:    0.210   <--- Amps panel 3
  //  0017:  219.600   <--- W total
  //  0019:   57.200   <--- var total

  //  0039:  105.200   <--- VA total
  //  0041:  236.550   <--- Volts panel 1
  //  0043:  236.580   <--- Volts panel 2
  //  0045:  236.480   <--- Volts panel 3

  //  001B:    0.828   <--- PF
  //  001D:   49.951   <--- Hz

  //  001F:    0.000   <---  uh
  //  0021:    0.070   <---  -uh
  //  0023:    0.000   <---  uAh
  //  0025:    0.030   <---  -uAh

// Data (Object) Dictionary for Modbus parameters of the power meter:
// The CID field in the table must be unique.
// Modbus Slave Addr field defines slave address of the device with correspond parameter.
// Modbus Reg Type - Type of Modbus register area (Holding register, Input Register and such).
// Reg Start field defines the start Modbus register number and Reg Size defines the number of registers for the characteristic accordingly.
// The Instance Offset defines offset in the appropriate parameter structure that will be used as instance to save parameter value.
// Data Type, Data Size specify type of the characteristic and its data size.
// Parameter Options field specifies the options that can be used to process parameter value (limits or masks).
// Access Mode - can be used to implement custom options for processing of characteristic (Read/Write restrictions, factory mode values and etc).
const mb_parameter_descriptor_t device_parameters[] = {
    //{ CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size, Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
    { CID_HOLD_DATA_0, STR("Amps_phase_1"), STR("A"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x12, 2,
            HOLD_OFFSET(holding_data0), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_1, STR("Amps_phase_2"), STR("A"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x14, 2,
            HOLD_OFFSET(holding_data1), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_2, STR("Amps_phase_3"), STR("A"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x10, 2,
            HOLD_OFFSET(holding_data2), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_3, STR("Watts"), STR("W"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x16, 2,
            HOLD_OFFSET(holding_data3), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_4, STR("var"), STR("W"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x26, 2,
            HOLD_OFFSET(holding_data4), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_5, STR("VA"), STR("VA"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x28, 2,
            HOLD_OFFSET(holding_data5), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 400, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_6, STR("Volts_phase_1"), STR("V"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x40, 2,
            HOLD_OFFSET(holding_data6), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 400, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_7, STR("Volts_phase_2"), STR("V"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x42, 2,
            HOLD_OFFSET(holding_data7), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 400, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_8, STR("Volts_phase_3"), STR("V"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x44, 2,
            HOLD_OFFSET(holding_data8), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 400, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_9, STR("Power_Factor"), STR(""), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x1A, 2,
            HOLD_OFFSET(holding_data9), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 1, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_10, STR("Frequency"), STR("Hz"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x1C, 2,
            HOLD_OFFSET(holding_data10), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 60, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_11, STR("uh"), STR("Wh?"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x1E, 2,
            HOLD_OFFSET(holding_data11), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 10000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_12, STR("-uh"), STR("Wh?"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x20, 2,
            HOLD_OFFSET(holding_data12), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 10000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_13, STR("uAh"), STR("Ah"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x22, 2,
            HOLD_OFFSET(holding_data13), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 10000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_14, STR("-uAh"), STR("Ah"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x24, 2,
            HOLD_OFFSET(holding_data14), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 1000, .001 ), PAR_PERMS_READ },
};

// Calculate number of parameters in the table
const uint16_t num_device_parameters = (sizeof(device_parameters)/sizeof(device_parameters[0]));

// // Get parameter description table
// mb_parameter_descriptor_t* get_modbus_parameter_descriptor_table() {
//         return device_parameters;
// }

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
static void read_power_meter(void *arg)
{
    esp_err_t err = ESP_OK;
    float current_value = 0;
    const mb_parameter_descriptor_t* param_descriptor = NULL;

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
            vTaskDelay(100/portTICK_PERIOD_MS);
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
                    send_to_rmaker_cloud(device_parameters, current_value, power_sensor_device);
                    
                    // Send instantaneous wattage to PVoutput.org
                    //if(cid == 3) send_to_pvoutput_org(cid, value);
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

    ESP_LOGI(TAG, "Destroy master...");
    ESP_ERROR_CHECK(mbc_master_destroy());
}

// Modbus master initialization
static esp_err_t mb_master_init()
{
    // Initialize and start Modbus controller
    mb_communication_info_t comm = {
            .port = MB_PORT_NUM,
            .mode = MB_MODE_RTU,
            .baudrate = MB_DEV_SPEED,
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
    err = uart_set_pin(MB_PORT_NUM, CONFIG_MB_UART_TXD, CONFIG_MB_UART_RXD,
                              UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    err = mbc_master_start();
    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
                            "mb controller start fail, returns(0x%x).",
                            (uint32_t)err);

    MASTER_CHECK((err == ESP_OK), ESP_ERR_INVALID_STATE,
            "mb serial set pin failure, uart_set_pin() returned (0x%x).", (uint32_t)err);
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

void app_modbus_init()
{
    while(1) {
        vTaskDelay(10000/portTICK_PERIOD_MS);
        // Initialization of device peripheral and objects
        ESP_ERROR_CHECK(mb_master_init());

        read_power_meter(NULL); // TODO: Call this func forever?
    }
}