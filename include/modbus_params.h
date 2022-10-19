/*
 * SPDX-FileCopyrightText: 2016-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*=====================================================================================
 * Description:
 *   The Modbus parameter structures used to define Modbus instances that
 *   can be addressed by Modbus protocol. Define these structures per your needs in
 *   your application. Below is just an example of possible parameters.
 *====================================================================================*/
#pragma once

//#include "cid_tables.h"

//#define MB_REPORTING_PERIOD    60*1000*5/portTICK_PERIOD_MS /* Report every 5 minutes, to avoid rate limiting, especially on pvoutput.org */
#define MB_REPORTING_PERIOD    60*1000*1/portTICK_PERIOD_MS /* Report every minute, to avoid rate limiting, especially on pvoutput.org */

// Every second for debugging purposes
//#define MB_REPORTING_PERIOD    1*1000/portTICK_PERIOD_MS

// Tweaked via idf.py menuconfig
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

// Newer ESP-IDF versions (>4.4) switch to MB_RETURN_ON_FALSE macro instead
#define MASTER_CHECK(a, ret_val, str, ...) \
    if (!(a)) { \
        ESP_LOGE(TAG, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        return (ret_val); \
    }

// This file defines structure of modbus parameters which reflect correspond modbus address space
// for each hodling registers register type
#pragma pack(push, 1)
typedef struct
{
    float holding_data0;
    float holding_data1;
    float holding_data2;
    float holding_data3;
    float holding_data4;
    float holding_data5;
    float holding_data6;
    float holding_data7;
    float holding_data8;
    float holding_data9;
    float holding_data10;
    float holding_data11;
    float holding_data12;
    float holding_data13;
    float holding_data14;

} holding_reg_params_t;
#pragma pack(pop)

extern holding_reg_params_t holding_reg_params;