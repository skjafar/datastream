/**
 * @file dsRegisters.h
 * @brief Datastream Register Management
 * @author Sofian Jafar
 * @date 3/16/2021
 *
 * @defgroup registers Register Management
 * @ingroup datastream
 * @brief Real-time data access via registers
 *
 * Registers hold real-time system state and sensor data. They can be:
 * - Read-only: Status, sensor readings, counters (first DS_REGISTERS_READ_ONLY_COUNT registers)
 * - Read-write: Control outputs, settings
 *
 * Registers are 32-bit values accessed by index. Use the byName member
 * for named access, or byAddress[] for index-based access.
 *
 * @{
 */

#ifndef DSREGISTERS_H_
#define DSREGISTERS_H_

#include "datastream.h"
#include DS_USER_REGISTER_DEFINITIONS

/** @brief Compile-time check for 32-bit alignment */
DS_STATIC_ASSERT(sizeof(ds_register_names_t) % 4 == 0, "ds_register_names_t size must be a multiple of 4 bytes");

/** @brief Size of register structure in bytes */
#define DS_REGISTERS_BY_NAME_T_SIZE     sizeof (ds_register_names_t)

/** @brief Total number of registers */
#define DS_REGISTER_COUNT               (DS_REGISTERS_BY_NAME_T_SIZE / sizeof(uint32_t))

/** Ensure register count fits in the protocol's uint8_t address field */
DS_STATIC_ASSERT(sizeof(ds_register_names_t) / 4 <= 256, "Register count exceeds uint8_t address space (max 256)");

/**
 * @brief Register storage union
 *
 * Provides dual access methods:
 * - byName: Structured access with named fields
 * - byAddress: Array access by register index
 */
typedef union
{
    ds_register_names_t     byName;                      /**< Named register access */
    uint32_t                byAddress[DS_REGISTER_COUNT]; /**< Indexed register access */
}ds_registers_t;

/** @brief Global register storage */
extern ds_registers_t REGS;

/**
 * @brief Set register value by address/index
 *
 * Writes a value to the specified register. Enforces:
 * - Write permission check (task must have control)
 * - Read-only protection (first DS_REGISTERS_READ_ONLY_COUNT registers)
 * - Address bounds checking
 *
 * @param[in]  address Register index (0 to DS_REGISTER_COUNT-1)
 * @param[in]  value   Value to write
 * @param[out] reply   Operation result (WRITE_REGISTER_OK, PERMISSION_ERROR, etc.)
 *
 * @note Triggers dsRegisterSetCallback() on successful write
 */
void dsSetRegister(uint32_t address, uint32_t value, reply_t * reply);

/**
 * @brief Get register value by address/index
 *
 * Reads the value of the specified register.
 *
 * @param[in]  address Register index (0 to DS_REGISTER_COUNT-1)
 * @param[out] reply   Operation result (READ_REGISTER_OK or ADDRESS_OUT_OF_RANGE_ERROR)
 * @return Register value, or 0 if address out of range
 *
 * @note Triggers dsRegisterGetCallback() on successful read
 * @note reply can be NULL if status not needed
 */
uint32_t dsGetRegister(uint32_t address, reply_t * reply);

/**
 * @brief Initialize registers to default values (weak function)
 *
 * Called during dsInitialize(). Override to set initial register values.
 * Default implementation sets CONTROL_INTERFACE to ds_control_UNDECIDED.
 *
 * @param[in,out] regList Pointer to register structure to initialize
 *
 * @note This is a weak function - override in your application
 */
void dsInitializeRegisters(ds_registers_t * regList);

/**
 * @brief Callback invoked after register write (weak function)
 *
 * Override to perform actions when a register is modified.
 * Examples: Update hardware outputs, trigger calculations, log changes.
 *
 * @param[in] address  Register index that was written
 * @param[in] oldValue Previous register value
 * @param[in] newValue New register value
 *
 * @note This is a weak function - override in your application
 * @note Called AFTER register value is updated
 */
void dsRegisterSetCallback(uint32_t address, uint32_t oldValue, uint32_t newValue);

/**
 * @brief Callback invoked after register read (weak function)
 *
 * Override to perform actions when a register is accessed.
 * Examples: Update cached sensor readings, log access.
 *
 * @param[in] address Register index that was read
 * @param[in] value   Current register value
 *
 * @note This is a weak function - override in your application
 * @note Called AFTER register value is retrieved
 */
void dsRegisterGetCallback(uint32_t address, uint32_t value);

/** @} */ // end of registers group

#endif /* DSREGISTERS_H_ */
