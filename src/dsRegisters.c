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

// Library-owned system registers
ds_system_registers_t SYS_REGS;

// User-defined registers
ds_registers_t REGS;

/* ======================== System Register Access ======================== */

uint32_t dsGetSystemRegister(uint16_t address, reply_t *reply)
{
    if (address < DS_SYSTEM_REGISTER_COUNT)
    {
        if (reply != NULL)
        {
            *reply = READ_REGISTER_OK;
        }
        return SYS_REGS.byAddress[address];
    }
    else
    {
        if (reply != NULL)
        {
            *reply = ADDRESS_OUT_OF_RANGE_ERROR;
        }
        return 0;
    }
}

void dsSetSystemRegister(uint16_t address, uint32_t value, reply_t *reply)
{
    (void)address;
    (void)value;
    // All system registers are read-only from the external protocol.
    *reply = PERMISSION_ERROR;
}

/* ======================== User Register Access ======================== */

void dsSetRegister(uint16_t address, uint32_t value, reply_t * reply)
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


uint32_t dsGetRegister(uint16_t address, reply_t * reply)
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
 * @brief Initializes the system register values.
 *
 * Sets the initial values for all library-owned system registers.
 * Called during dsInitialize(). Override this weak function if you need
 * to observe or react to system register initialization, but take care
 * not to break library-required defaults.
 *
 * @param[in,out] sysRegList Pointer to the system register structure to initialize
 */
WEAK void dsInitializeSystemRegisters(ds_system_registers_t *sysRegList)
{
    sysRegList->byName.CONTROL_INTERFACE = ds_control_UNDECIDED;
}

/**
 * @brief Initializes the user register values.
 *
 * Called during dsInitialize(). Override this weak function in your application
 * to set initial values for your own registers via the regList parameter.
 *
 * @param[in,out] regList Pointer to the user register structure to initialize
 */
WEAK void dsInitializeRegisters(ds_registers_t * regList)
{
    (void)regList;
    // Default: do nothing — user registers start at zero
}

// Weak functions to allow for custom register handling
// These can be overridden in the application if needed
WEAK void dsRegisterSetCallback(uint16_t address, uint32_t oldValue, uint32_t newValue)
{
    // Default: do nothing
}

WEAK void dsRegisterGetCallback(uint16_t address, uint32_t value)
{
    // Default: do nothing
}
