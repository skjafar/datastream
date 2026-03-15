/*
 * ds_minimal_parameter_names.h
 *
 * Minimal default user parameter definitions for the datastream library.
 *
 * For production use, create your own parameter definitions file and configure
 * it via DS_USER_PARAMETER_DEFINITIONS. See USER_INTEGRATION_GUIDE.md for details.
 *
 * Created: 07/08/2025
 *  Author: Sofian.jafar
 */

#ifndef DS_PARAMETER_NAMES_H_
#define DS_PARAMETER_NAMES_H_

#include <ds_default_config.h>

/************************************* USER PARAMETERS ************************************
* Define your application parameters here.                                                *
* All fields must be 32-bit: uint32_t, int32_t, float, etc.                             *
* Parameters hold persistent configuration data stored in flash memory.                   *
* Create your own parameter definitions file for production use.                          *
******************************************************************************************/
typedef struct PACKED
{
    /*************** Add application specific parameters here ***************/

    // uint32_t    DEVICE_ID;
    // uint32_t    USES_DHCP;
    // uint32_t    IP_ADDR[4];
    // float       CALIBRATION_OFFSET;

    /*************** end application specific parameters ***************/

    // Placeholder — remove and replace with your own parameters.
    // At least one field is required to avoid a zero-sized struct.
    uint32_t    PLACEHOLDER;

}ds_parameter_names_t;

#endif /* DS_PARAMETER_NAMES_H_ */
