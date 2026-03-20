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
#include "ds_system_register_names.h"
#include DS_USER_REGISTER_DEFINITIONS

/* ---- System registers ---- */

/** @brief Compile-time check: system register struct must be 32-bit aligned */
DS_STATIC_ASSERT(sizeof(ds_system_register_names_t) % 4 == 0, "ds_system_register_names_t size must be a multiple of 4 bytes");

/** @brief Total number of system registers */
#define DS_SYSTEM_REGISTER_COUNT        (sizeof(ds_system_register_names_t) / sizeof(uint32_t))

/** @brief Ensure system register count fits in the protocol's uint16_t address field */
DS_STATIC_ASSERT(sizeof(ds_system_register_names_t) / 4 <= 65536, "System register count exceeds uint16_t address space (max 65536)");

/**
 * @brief System register storage union (library-owned)
 *
 * - byName: Named access to library system registers (e.g. SYS_REGS.byName.COUNTER_1HZ)
 * - byAddress: Indexed access used by the protocol handlers
 */
typedef union
{
    ds_system_register_names_t  byName;
    uint32_t                    byAddress[DS_SYSTEM_REGISTER_COUNT];
} ds_system_registers_t;

/** @brief Global system register storage (library-owned) */
extern ds_system_registers_t SYS_REGS;

/* ---- User registers ---- */

/** @brief Compile-time check for 32-bit alignment */
DS_STATIC_ASSERT(sizeof(ds_register_names_t) % 4 == 0, "ds_register_names_t size must be a multiple of 4 bytes");

/** @brief Size of register structure in bytes */
#define DS_REGISTERS_BY_NAME_T_SIZE     sizeof (ds_register_names_t)

/** @brief Total number of user registers */
#define DS_REGISTER_COUNT               (DS_REGISTERS_BY_NAME_T_SIZE / sizeof(uint32_t))

/** Ensure register count fits in the protocol's uint16_t address field */
DS_STATIC_ASSERT(sizeof(ds_register_names_t) / 4 <= 65536, "Register count exceeds uint16_t address space (max 65536)");

/**
 * @brief User register storage union
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

/** @brief Global user register storage */
extern ds_registers_t REGS;

/**
 * @brief Get system register value by address/index
 *
 * Reads from the library-owned SYS_REGS. No user callback is fired.
 *
 * @param[in]  address System register index (0 to DS_SYSTEM_REGISTER_COUNT-1)
 * @param[out] reply   Operation result (READ_REGISTER_OK or ADDRESS_OUT_OF_RANGE_ERROR)
 * @return Register value, or 0 if address out of range
 */
uint32_t dsGetSystemRegister(uint16_t address, reply_t *reply);

/**
 * @brief Set system register value by address/index
 *
 * All current system registers are read-only from the external protocol.
 * This function always returns PERMISSION_ERROR. It exists so that the
 * ds_type_WRITE_SYSTEM_REGISTER packet type can be handled cleanly and
 * writable system registers can be introduced in future without a protocol change.
 *
 * @param[in]  address System register index
 * @param[in]  value   Value (ignored — system registers are read-only)
 * @param[out] reply   Always set to PERMISSION_ERROR
 */
void dsSetSystemRegister(uint16_t address, uint32_t value, reply_t *reply);

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
void dsSetRegister(uint16_t address, uint32_t value, reply_t * reply);

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
uint32_t dsGetRegister(uint16_t address, reply_t * reply);

/**
 * @brief Initialize system registers to default values (weak function)
 *
 * Called during dsInitialize(). Override to observe or extend system register
 * initialization. Default implementation sets CONTROL_INTERFACE to ds_control_UNDECIDED.
 *
 * @param[in,out] sysRegList Pointer to system register structure to initialize
 *
 * @note This is a weak function - override in your application
 */
void dsInitializeSystemRegisters(ds_system_registers_t *sysRegList);

/**
 * @brief Initialize user registers to default values (weak function)
 *
 * Called during dsInitialize(). Override to set initial values for your
 * application registers.
 *
 * @param[in,out] regList Pointer to user register structure to initialize
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
void dsRegisterSetCallback(uint16_t address, uint32_t oldValue, uint32_t newValue);

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
void dsRegisterGetCallback(uint16_t address, uint32_t value);

/** @} */ // end of registers group

#endif /* DSREGISTERS_H_ */
