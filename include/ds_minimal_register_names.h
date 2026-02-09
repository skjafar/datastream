/*
 * ds_minimal_register_names.h
 *
 * Minimal default register definitions for datastream library
 * 
 * This file provides minimal register definitions for backward compatibility.
 * For production use, create your own register definitions file and configure
 * it via DS_USER_REGISTER_DEFINITIONS. See USER_INTEGRATION_GUIDE.md for details.
 *
 * Created: 07/08/2025
 *  Author: Sofian.jafar
 */

#ifndef DS_REGISTER_NAMES_H_
#define DS_REGISTER_NAMES_H_

/******* include files with user defines types (must be 32bit - 4 bytes long) ******/
#include <ds_default_config.h>
/************************************************************************************/

/************************************* REGISTERS ****************************************
* Minimal register definitions - customize for your application                         *
* All data must be 32-bit aligned: uint32_t, int32_t, float, etc.                     *
* Registers hold real-time data accessible via datastream protocol                     *
* Create your own register definitions file for production use                          * 
* The registers are read-only or read/write, depending on the application               *
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

    uint32_t            CONTROL_INTERFACE;

    uint32_t            COUNTER_1HZ;
    

    /*************** Add application specific registers here ***************/

    // This is where you can add your application specific registers

    /**** read only registers ****/
    // temperature readings - example, not used in the current application
    // float               tCC_TEMPERATURE;                    // unit:°C
    // float               AGPS_TEMPERATURE;                   // unit:°C


    
    /***** read/write registers *****/
    // uint32_t            DIG_OUTPUTS;

    
    /*************** end application specific registers ***************/

}ds_register_names_t;
#define DS_REGISTERS_READ_ONLY_COUNT    4

#endif /* DS_REGISTER_NAMES_H_ */