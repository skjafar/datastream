/*
 * registers.c
 *
 * Created: 3/16/2021 1:51:55 PM
 *  Author: Sofian
 */ 

#include "dsRegisters.h"

/* FreeRTOS includes. */

/* Standard includes. */
#include <stdint.h>
#include <string.h>

/* Other includes. */
#include "dsParameters.h"
#include "datastream.h"

// Registers to be used by the system
ds_registers_t REGS;

void dsSetRegister(uint32_t address, uint32_t value, reply_t * reply)
{
    if (dsCheckTaskWritePermission())
    {
        if (address < DS_REGISTER_COUNT)
        {
            if (address >= DS_REGISTERS_READ_ONLY_COUNT)    
            {
                uint32_t oldValue = REGS.byAddress[address]; // Store the old value before changing
                
                REGS.byAddress[address] = value; // update the register in the union
                
                dsRegisterSetCallback(address, oldValue, value);
                
                *reply = WRITE_REGISTER_OK;
                
                // Log the register write operation
                DS_LOGI("Register at address %" PRIu32 " set to value %" PRIu32, address, value);
            }
            else
            {
                *reply = PERMISSION_ERROR;
            }
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


uint32_t dsGetRegister(uint32_t address, reply_t * reply)
{
    if (address < DS_REGISTER_COUNT)
    {
        if (reply != NULL) {
            *reply = READ_REGISTER_OK;
        }

        dsRegisterGetCallback(address, REGS.byAddress[address]);

        return REGS.byAddress[address];
    }
    else
    {
        if (reply != NULL) {
            *reply = ADDRESS_OUT_OF_RANGE_ERROR;
        }
        return 0;
    }
}

/**
 * @brief Initializes the register values for the system.
 *
 * This function sets the initial values for key registers, such as
 * FIRMWARE_VERSION and CONTROL_INTERFACE, to their default or configured states.
 * It should be called during system startup to ensure registers are properly initialized.
 */
WEAK void dsInitializeRegisters(ds_registers_t * regList)
{
    regList->byName.CONTROL_INTERFACE = ds_control_UNDECIDED;
}

// Weak functions to allow for custom register handling
// These can be overridden in the application if needed
WEAK void dsRegisterSetCallback(uint32_t address, uint32_t oldValue, uint32_t newValue)
{
    // Default: do nothing
}

WEAK void dsRegisterGetCallback(uint32_t address, uint32_t value)
{
    // Default: do nothing
}
