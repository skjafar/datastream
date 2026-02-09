/*
 * ds_minimal_parameter_names.h
 *
 * Minimal default parameter definitions for datastream library
 *
 * This file provides minimal parameter definitions for backward compatibility.
 * For production use, create your own parameter definitions file and configure
 * it via DS_USER_PARAMETER_DEFINITIONS. See USER_INTEGRATION_GUIDE.md for details.
 *
 * Created: 07/08/2025
 *  Author: Sofian.jafar
 */

#ifndef DS_PARAMETER_NAMES_H_
#define DS_PARAMETER_NAMES_H_

/******* include files with user defines types (must be 32bit - 4 bytes long) ******/
#include <ds_default_config.h>
/************************************************************************************/

/************************************* PARAMETERS ****************************************
* Minimal parameter definitions - customize for your application                         *
* All data must be 32-bit aligned: uint32_t, int32_t, float, etc.                      *
* Parameters hold persistent configuration data stored in flash memory                   *
* Create your own parameter definitions file for production use                          *
* Parameters are designed to be stored in permanent storage (e.g. flash)                 *
*****************************************************************************************/ 
typedef struct PACKED
{   
    // Device info
    uint32_t    DEVICE_ID;
    
    /***** Ethernet settings *****/
    uint32_t    USES_DHCP;
    
    uint32_t    IP_ADDR[4];
    uint32_t    GATEWAY_ADDR[4];
    uint32_t    DNS_SERVER_ADDR[4];
    uint32_t    NET_MASK[4];
    uint32_t    MAC_ADDR[6];

    /*************** Add application specific parameters here ***************/
    
    // This is where you can add your application specific parameters

    /*************** end application specific parameters ***************/
    
    
    /*********************/
    /***** read only *****/
    /*********************/
    
    // This parameter is set automatically
    uint32_t    PARAMETERS_SETS_IN_FLASH; // number of parameter sets in flash

    // This parameter is set automatically, if this parameter is ever changed the settings will be reset to default
    uint32_t    PARAMETERS_INITIALIZATION_MARKER;
}ds_parameter_names_t;

#endif /* DS_PARAMETER_NAMES_H_ */