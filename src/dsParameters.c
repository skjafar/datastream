/*
 * flash.c
 *
 * Created: 2/7/2021 3:29:12 PM
 *  Author: Sofian.jafar
 */ 

#include "dsParameters.h"

/* Standard includes. */
#include <stdint.h>
#include <string.h>

/* Other includes. */
#include "datastream.h"


// Parameters to be used by the system, must be 32 bits aligned to be written to Flash properly
ds_parameters_t PARS;

/*************************************
    Static functions prototypes
 ************************************/


/*************************************
        Function definitions
 ************************************/
/*
 * change value of parameter using it's address in the struct
 */
void dsSetParameter(uint32_t address, uint32_t value, reply_t * reply)
{
    // Check if the current task has permission to write to registers or parameters
    if (dsCheckTaskWritePermission())
    {
        if (address < DS_PARAMETER_COUNT)
        {
            uint32_t oldValue = PARS.byAddress[address]; // Store the old value before changing
            
            PARS.byAddress[address] = value; // update the parameter in the union
            
            dsParameterSetCallback(address, oldValue, value);
            
            *reply = WRITE_PARAMETER_OK;
        }
        else
        {
            *reply = ADDRESS_OUT_OF_RANGE_ERROR;
        }
    }
    else
    {
        *reply = CONTROL_INTERFACE_ERROR;
    }
}

/*
 * get value of parameter using it's address in the struct
 */
uint32_t dsGetParameter(uint32_t address, reply_t * reply)
{
    if (address < DS_PARAMETER_COUNT)
    {
        if (reply != NULL) {
            *reply = READ_PARAMETER_OK;
        }

        dsParameterGetCallback(address, PARS.byAddress[address]);

        return PARS.byAddress[address];
    }
    else
    {
        if (reply != NULL) {
            *reply = ADDRESS_OUT_OF_RANGE_ERROR;
        }
        return 0;
    }
}

/*
 * write parameters struct to flash
 */
WEAK void dsWriteParametersToFlash(ds_parameters_t * parList)
{
    // write the parameters to flash memory
    // this function should be provided by the user
    // it should write the parameters to flash memory in a way that is compatible with the flash
    // memory used by the system
    // for example, if the system uses SPI flash, it should use the SPI flash driver
    // to write the parameters to flash memory
    // if the system uses any other flash memory, it should use the appropriate driver
    // example: vWriteParametersToSTMFlash(parList, ERASE_FLASH);
}

/*
 * read parameters struct from flash
 */
WEAK void dsReadParametersFromFlash(ds_parameters_t * parList)
{
    // read the parameters from flash memory
    // this function should be provided by the user
    // it should read the parameters from flash memory in a way that is compatible with the flash
    // memory used by the system
    // for example, if the system uses SPI flash, it should use the SPI flash driver
    // to read the parameters from flash memory
    // if the system uses any other flash memory, it should use the appropriate driver
    // example: PARS = flashParameters;
}


WEAK void dsInitializeParameters(ds_parameters_t * parList)
{
    dsSetParametersDefaults(&PARS);
    // read stored parameters to be used
    dsReadParametersFromFlash(&PARS);
}

WEAK void dsSetParametersDefaults(ds_parameters_t * parList)
{
    // set all parameters to zero
    *parList = (const ds_parameters_t){ 0 };

    // Set sensible defaults for your application parameters here, for example:
    // parList->byName.DEVICE_ID    = 1;
    // parList->byName.USES_DHCP    = 1;
 
    dsWriteParametersToFlash(parList);
}

/* Callback for when a parameter is set */
WEAK void dsParameterSetCallback(uint32_t address, uint32_t oldValue, uint32_t newValue) 
{
      // Default: do nothing
}

/* Callback for when a parameter is read */
WEAK void dsParameterGetCallback(uint32_t address, uint32_t value)
{
      // Default: do nothing
}
