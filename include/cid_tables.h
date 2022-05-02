#include "mbcontroller.h"       // for mbcontroller defines and api
#include "modbus_params.h"      // for modbus parameters structures
#include "sdkconfig.h"

// The macro to get offset for parameter in the appropriate structure
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))

#define STR(fieldname) ((const char*)( fieldname ))
// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

// Enumeration of modbus device addresses accessed by master device
enum {
    MB_DEVICE_ADDR1 = 1, // Only one slave device used for the test (add other slave addresses here)
};


#if CONFIG_POWER_METER_MODEL_YG899E_S9Y
    enum {
        CID_HOLD_DATA_0 = 0,
        CID_HOLD_DATA_1,
        CID_HOLD_DATA_2
    };

const mb_parameter_descriptor_t device_parameters[] = {
    //{ CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size, Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
    { CID_HOLD_DATA_0, STR("Amps_phase_1"), STR("A"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 35, 2,
            HOLD_OFFSET(holding_data0), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_1, STR("Amps_phase_2"), STR("A"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x21, 2,
            HOLD_OFFSET(holding_data1), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
    { CID_HOLD_DATA_2, STR("Amps_phase_3"), STR("A"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x26, 2,
            HOLD_OFFSET(holding_data2), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_3, STR("Watts"), STR("W"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x23, 2,
//             HOLD_OFFSET(holding_data3), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_4, STR("var"), STR("W"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x25, 2,
//             HOLD_OFFSET(holding_data4), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 5000, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_5, STR("VA"), STR("VA"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x57, 2,
//             HOLD_OFFSET(holding_data5), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 400, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_6, STR("Volts_phase_1"), STR("V"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x29, 2,
//             HOLD_OFFSET(holding_data6), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 400, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_7, STR("Volts_phase_2"), STR("V"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x31, 2,
//             HOLD_OFFSET(holding_data7), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 400, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_8, STR("Volts_phase_3"), STR("V"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x33, 2,
//             HOLD_OFFSET(holding_data8), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 400, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_9, STR("Power_Factor"), STR(""), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x27, 2,
//             HOLD_OFFSET(holding_data9), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 1, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_10, STR("Frequency"), STR("Hz"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x29, 2,
//             HOLD_OFFSET(holding_data10), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 60, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_11, STR("uh"), STR("Wh?"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x31, 2,
//             HOLD_OFFSET(holding_data11), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 10000, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_12, STR("-uh"), STR("Wh?"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x33, 2,
//             HOLD_OFFSET(holding_data12), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 10000, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_13, STR("uAh"), STR("Ah"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x35, 2,
//             HOLD_OFFSET(holding_data13), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 10000, .001 ), PAR_PERMS_READ },
//     { CID_HOLD_DATA_14, STR("-uAh"), STR("Ah"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0x37, 2,
//             HOLD_OFFSET(holding_data14), PARAM_TYPE_FLOAT, PARAM_SIZE_FLOAT, OPTS( 0, 1000, .001 ), PAR_PERMS_READ },
};
#elif CONFIG_POWER_METER_MODEL_YG912_S9Y

const mb_parameter_descriptor_t device_parameters[] = {
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

#elif CONFIG_POWER_METER_MODEL_YG914E_95Y
#elif CONFIG_POWER_METER_MODEL_DDS238_1_ZN
#elif CONFIG_POWER_METER_MODEL_PAC3100
#elif CONFIG_POWER_METER_MODEL_PZEM_016
#elif CONFIG_POWER_METER_MODEL_SDM120
#elif CONFIG_POWER_METER_MODEL_SDM220
#elif CONFIG_POWER_METER_MODEL_SDM630
#elif CONFIG_POWER_METER_MODEL_SMART_X96
#elif CONFIG_POWER_METER_MODEL_SMART_X96_HARMONIC
#endif
