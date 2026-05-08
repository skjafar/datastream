/*
 * ds_minimal_register_names.h
 *
 * Minimal default user register definitions for the datastream library.
 *
 * System registers (DS_PACKET_COUNT, DS_ERROR_COUNT, CONTROL_INTERFACE,
 * COUNTER_1HZ) are now managed entirely by the library in SYS_REGS and are
 * accessed via ds_type_READ_SYSTEM_REGISTER / ds_type_WRITE_SYSTEM_REGISTER.
 * Do NOT add them here.
 *
 * For production use, create your own register definitions file and configure
 * it via DS_USER_REGISTER_DEFINITIONS. See USER_INTEGRATION_GUIDE.md for details.
 *
 * Created: 07/08/2025
 *  Author: Sofian.jafar
 */

#ifndef DS_REGISTER_NAMES_H_
#define DS_REGISTER_NAMES_H_

#include <ds_default_config.h>

/************************************* USER REGISTERS *************************************
* Define your application registers here.                                                 *
* All fields must be 32-bit: uint32_t, int32_t, float, etc.                             *
* Read-only registers must come first; set DS_REGISTERS_READ_ONLY_COUNT accordingly.     *
* Create your own register definitions file for production use.                           *
******************************************************************************************/
typedef struct PACKED
{
    /*********************/
    /***** read only *****/
    /*********************/

    /*************** Add application specific read-only registers here ***************/

    // float               SENSOR_TEMPERATURE;   // example read-only register
    // uint32_t            STATUS_FLAGS;          // example read-only register


    /**********************/
    /***** read/write *****/
    /**********************/

    /*************** Add application specific read-write registers here ***************/

    // uint32_t            CONTROL_MODE;          // example read-write register
    // float               SETPOINT;              // example read-write register

    // Placeholder — remove and replace with your own registers.
    // At least one field is required to avoid a zero-sized struct.
    uint32_t            PLACEHOLDER;

}ds_register_names_t;

// Number of read-only user registers (counted from the start of the struct above).
// System registers are not counted here — they are managed by the library.
#define DS_REGISTERS_READ_ONLY_COUNT    1

#endif /* DS_REGISTER_NAMES_H_ */
