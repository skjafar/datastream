/*
 * ds_app_register_names_example.h
 *
 * Example register definitions for custom datastream applications
 * 
 * This file shows how to define application-specific registers for the
 * datastream library. Copy and modify this file for your project.
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


/************************************* REGISTERS ****************************************
* ensure all data is 32bit, this is a structure for holding all registers used on-line  *
* data can be uint32_t, int32_t, float, etc.                                            *
* the registers are used to hold data that is read/write by the datastream protocol     *
* Add your application-specific registers below                                         * 
* The registers can be read-only or read/write, depending on your application           *
****************************************************************************************/ 
typedef struct PACKED
{
    /*********************/
    /***** read only *****/
    /*********************/

    #if (DS_STATS_ENABLE == 1)
    // The number of packets received, used for debugging and statistics
    uint32_t            DS_PACKET_COUNT;
    uint32_t            DS_ERROR_COUNT;
    #endif

    // Control interface status - required for datastream operation
    ds_control_interface_t CONTROL_INTERFACE;

    // Example application registers - customize these for your needs
    uint32_t            STATUS_FLAGS;
    float               SENSOR_VALUE_1;
    float               SENSOR_VALUE_2;
    uint32_t            SYSTEM_UPTIME;

    /**********************/
    /***** read/write *****/
    /**********************/

    // Example control registers - customize these for your needs
    uint32_t            CONTROL_MODE;
    uint32_t            OUTPUT_ENABLE;
    float               SETPOINT;

}ds_register_names_t;

// Define how many registers are read-only (count from the beginning)
// Update this number based on your read-only registers above
#if (DS_STATS_ENABLE == 1)
    #define DS_REGISTERS_READ_ONLY_COUNT    7  // DS_PACKET_COUNT, DS_ERROR_COUNT, CONTROL_INTERFACE, STATUS_FLAGS, SENSOR_VALUE_1, SENSOR_VALUE_2, SYSTEM_UPTIME
#else
    #define DS_REGISTERS_READ_ONLY_COUNT    5  // CONTROL_INTERFACE, STATUS_FLAGS, SENSOR_VALUE_1, SENSOR_VALUE_2, SYSTEM_UPTIME
#endif

#endif /* MINIMAL_DS_REGISTER_NAMES_H_ */