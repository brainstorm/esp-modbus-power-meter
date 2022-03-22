#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <sdkconfig.h>

#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "mbcontroller.h"       // for mbcontroller defines and api
#include "modbus_params.h"      // for modbus parameters structures
#include "esp_log.h"            // for log_write
#include "sdkconfig.h"

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 

#include "app_priv.h"

static const char *TAG = "app_modbus";

#define CONFIG_MB_UART_RXD 8
#define CONFIG_MB_UART_TXD 6
#define CONFIG_MB_UART_RTS 27 // TODO: Assign to null or option to not use it since the shield automatically handles it?

// Defines below are used to define register start address for each type of Modbus registers
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) >> 1))
#define MB_REG_HOLDING_START_AREA0          (HOLD_OFFSET(holding_data0))
#define MB_REG_HOLDING_START_AREA1          (HOLD_OFFSET(holding_data4))

#define MB_PAR_INFO_GET_TIMEOUT             10 // Timeout for get parameter info
#define MB_CHAN_DATA_MAX_VAL                (6)
#define MB_CHAN_DATA_OFFSET                 (0.2f)
#define MB_READ_MASK                        (MB_EVENT_HOLDING_REG_RD)

#define MB_PORT_NUM     UART_NUM_1   // Number of UART port used for Modbus connection
//#define MB_SLAVE_ADDR   (CONFIG_FMB_CONTROLLER_SLAVE_ID)      // The address of device in Modbus network
// #define MB_DEV_SPEED    (CONFIG_FMB_UART_BAUD_RATE)  // The communication speed of the UART
static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;

static void app_modbus_update(TimerHandle_t handle)
{

    // Initialization of Discrete Inputs register area
    reg_area.type = MB_REG_HOLDING_START_AREA0;
    reg_area.start_offset = MB_REG_DISCRETE_INPUT_START;
    reg_area.address = (void*)&discrete_reg_params;
    reg_area.size = sizeof(discrete_reg_params);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    // TODO: Fetch current power

    // esp_rmaker_param_update_and_report(
    //         esp_rmaker_device_get_param_by_type(power_sensor_device, ESP_RMAKER_PARAM_TEMPERATURE),
    //         esp_rmaker_float(g_power));
}


esp_err_t freemodbus_init(void)
{
    // Here are the user defined instances for device parameters packed by 1 byte
    // These are keep the values that can be accessed from Modbus master
    holding_reg_params_t holding_reg_params = { 0 };

    mb_param_info_t reg_info; // keeps the Modbus registers access information
    mb_communication_info_t comm_info; // Modbus communication parameters
    mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure

    // Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    void* mbc_slave_handler = NULL;
    esp_err_t err = mbc_slave_init(MB_PORT_SERIAL_SLAVE, &mbc_slave_handler);
    if (err != ESP_OK) {
        return err;
    }

    comm_info.mode = MB_MODE_RTU,
    comm_info.slave_addr = 2;
    comm_info.port = MB_PORT_NUM;
    comm_info.baudrate = 9600;
    comm_info.parity = MB_PARITY_NONE;
    ESP_ERROR_CHECK(mbc_slave_setup((void*)&comm_info));

    // Initialize Modbus register area descriptors
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = MB_REG_HOLDING_START_AREA0; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&holding_reg_params.holding_data0; // Set pointer to storage instance
    
    // Set the size of register storage instance = 150 holding registers
    reg_area.size = (size_t)(HOLD_OFFSET(holding_data4) - HOLD_OFFSET(test_regs));
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = MB_REG_HOLDING_START_AREA1; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&holding_reg_params.holding_data4; // Set pointer to storage instance
    reg_area.size = sizeof(float) << 2; // Set the size of register storage instance
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    //setup_reg_data(); // Set values into known state

    // Starts of modbus controller and stack
    ESP_ERROR_CHECK(mbc_slave_start());

    // Set UART pin numbers
    ESP_ERROR_CHECK(uart_set_pin(MB_PORT_NUM, CONFIG_MB_UART_TXD,
                            CONFIG_MB_UART_RXD, CONFIG_MB_UART_RTS,
                            UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "Modbus slave stack initialized.");    

    return ESP_OK;
}

void app_modbus_init()
{
    freemodbus_init();
}