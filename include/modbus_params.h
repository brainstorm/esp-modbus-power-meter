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
#ifndef _DEVICE_PARAMS
#define _DEVICE_PARAMS

// The number of parameters that intended to be used in the particular control process
#define MASTER_MAX_CIDS num_device_parameters

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

#endif // !defined(_DEVICE_PARAMS)
