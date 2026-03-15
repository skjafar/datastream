/*
 * ds_app_register_names_example.h
 *
 * Example user register definitions for custom datastream applications.
 *
 * System registers (DS_PACKET_COUNT, DS_ERROR_COUNT, CONTROL_INTERFACE,
 * COUNTER_1HZ) are managed entirely by the library in SYS_REGS and accessed
 * via ds_type_READ_SYSTEM_REGISTER. Do NOT add them here.
 *
 * Usage:
 * 1. Copy this file to your project (e.g., as "my_registers.h")
 * 2. Customize the register definitions for your application
 * 3. Configure in ds_app_config.h:
 *    #define DS_USER_REGISTER_DEFINITIONS "my_registers.h"
 * 4. Include "datastream.h" in your application
 *
 * See USER_INTEGRATION_GUIDE.md for detailed instructions.
 */

#ifndef DS_APP_REGISTER_NAMES_EXAMPLE_H_
#define DS_APP_REGISTER_NAMES_EXAMPLE_H_


/************************************* USER REGISTERS *************************************
* All fields must be 32-bit: uint32_t, int32_t, float, etc.                             *
* Read-only registers must come first; set DS_REGISTERS_READ_ONLY_COUNT accordingly.     *
* System registers are NOT defined here — they live in SYS_REGS.                         *
******************************************************************************************/
typedef struct PACKED
{
    /*********************/
    /***** read only *****/
    /*********************/

    // Example application read-only registers — customize these for your needs
    uint32_t            STATUS_FLAGS;
    float               SENSOR_VALUE_1;
    float               SENSOR_VALUE_2;
    uint32_t            SYSTEM_UPTIME;

    /**********************/
    /***** read/write *****/
    /**********************/

    // Example control registers — customize these for your needs
    uint32_t            CONTROL_MODE;
    uint32_t            OUTPUT_ENABLE;
    float               SETPOINT;

}ds_register_names_t;

// Number of read-only user registers (counted from the start of the struct above).
// System registers are not counted here — they are managed by the library.
#define DS_REGISTERS_READ_ONLY_COUNT    4  // STATUS_FLAGS, SENSOR_VALUE_1, SENSOR_VALUE_2, SYSTEM_UPTIME

#endif /* DS_APP_REGISTER_NAMES_EXAMPLE_H_ */
