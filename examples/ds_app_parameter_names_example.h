/*
 * ds_app_parameter_names_example.h
 *
 * Example parameter definitions for custom datastream applications
 * 
 * This file shows how to define application-specific parameters for the
 * datastream library. Copy and modify this file for your project.
 * 
 * Usage:
 * 1. Copy this file to your project (e.g., as "my_parameters.h")
 * 2. Customize the parameter definitions for your application
 * 3. Configure in ds_app_config.h:
 *    #define DS_USER_PARAMETER_DEFINITIONS "my_parameters.h"
 * 4. Include "datastream.h" in your application
 * 
 * See USER_INTEGRATION_GUIDE.md for detailed instructions.
 */

#ifndef DS_APP_PARAMETER_NAMES_EXAMPLE_H_
#define DS_APP_PARAMETER_NAMES_EXAMPLE_H_


/************************************* PARAMETERS ****************************************
* ensure all data is 32bit, this is a structure for holding all parameters of the system *
* data can be uint32_t, int32_t, float, etc.                                             *
* Parameters are used to hold data that is read/write by the datastream protocol         *
* Parameters are designed to be stored in permanent storage (e.g. flash memory)          *
* Add your application-specific parameters below                                          *
*****************************************************************************************/ 
typedef struct PACKED
{   
    // Device identification
    uint32_t    DEVICE_ID;
    
    /***** Network Configuration *****/
    // Basic network settings - customize based on your networking needs
    uint32_t    USES_DHCP;
    uint32_t    IP_ADDR[4];          // IPv4 address as 4 bytes
    uint32_t    GATEWAY_ADDR[4];     // Gateway address as 4 bytes  
    uint32_t    NET_MASK[4];         // Subnet mask as 4 bytes
    uint32_t    MAC_ADDR[6];         // MAC address as 6 bytes (padded to 4-byte alignment)

    /***** Application Parameters *****/
    // Example application parameters - customize these for your needs
    float       CALIBRATION_OFFSET;
    float       CALIBRATION_SCALE;
    uint32_t    OPERATING_MODE;
    uint32_t    ALARM_THRESHOLD;

    /*********************/
    /***** System Parameters - DO NOT MODIFY THESE *****/
    /*********************/
    
    // This parameter tracks the number of parameter sets stored in flash
    uint32_t    PARAMETERS_SETS_IN_FLASH;
    
    // This parameter acts as a version marker - if changed, settings will reset to defaults
    // Change this value only when you want to force a parameter reset across all devices
    uint32_t    PARAMETERS_INITIALIZATION_MARKER;

}ds_parameter_names_t;

#endif /* MINIMAL_DS_PARAMETER_NAMES_H_ */