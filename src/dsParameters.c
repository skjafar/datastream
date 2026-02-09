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

    // set sensible defaults
    parList->byName.USES_DHCP                          = 1;

    parList->byName.IP_ADDR[0]                         = configIP_ADDR0;
    parList->byName.IP_ADDR[1]                         = configIP_ADDR1;
    parList->byName.IP_ADDR[2]                         = configIP_ADDR2;
    parList->byName.IP_ADDR[3]                         = configIP_ADDR3;

    parList->byName.NET_MASK[0]                        = configNET_MASK0;
    parList->byName.NET_MASK[1]                        = configNET_MASK1;
    parList->byName.NET_MASK[2]                        = configNET_MASK2;
    parList->byName.NET_MASK[3]                        = configNET_MASK3;

    // generate a random sequence for the MAC address
    parList->byName.MAC_ADDR[0]                        = 0x62;
    // do not remove the following lines, they make sure the MAC address is valid
    parList->byName.MAC_ADDR[0]                        = parList->byName.MAC_ADDR[0] & 0xFE; // unicast device bit0 = (0 unicast, 1 multicacst)
    parList->byName.MAC_ADDR[0]                        = parList->byName.MAC_ADDR[0] | 0x02; // Locally adminstred MAC (not globally unique) bit1 = (0 globally unique, 1 locally adminstered)
    parList->byName.MAC_ADDR[1]                        = uxRand() & 0xFF;
    parList->byName.MAC_ADDR[2]                        = uxRand() & 0xFF;
    parList->byName.MAC_ADDR[3]                        = uxRand() & 0xFF;
    parList->byName.MAC_ADDR[4]                        = uxRand() & 0xFF;
    parList->byName.MAC_ADDR[5]                        = uxRand() & 0xFF;
    // dsWriteParametersToFlash(parList);
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