/**
 * @file dsParameters.h
 * @brief Datastream Parameter Management
 * @author Sofian Jafar
 * @date 2/7/2021
 *
 * @defgroup parameters Parameter Management
 * @ingroup datastream
 * @brief Persistent configuration storage and flash memory management
 *
 * Parameters hold persistent configuration data that survives power cycles.
 * They are stored in non-volatile memory (flash) and loaded at startup.
 *
 * Typical use cases:
 * - Network configuration (IP, gateway, MAC)
 * - Calibration values
 * - User preferences
 * - Device identification
 *
 * Parameters are 32-bit values accessed by index. Use the byName member
 * for named access, or byAddress[] for index-based access.
 *
 * @{
 */

#ifndef DSPARAMETERS_H_
#define DSPARAMETERS_H_

#include "datastream.h"
#include DS_USER_PARAMETER_DEFINITIONS

/** @brief Compile-time check for 32-bit alignment */
DS_STATIC_ASSERT(sizeof(ds_parameter_names_t) % 4 == 0, "ds_parameter_names_t size must be a multiple of 4 bytes");

/** @brief Size of parameter structure in bytes */
#define DS_PARAMETERS_T_SIZE   sizeof (ds_parameter_names_t)

/** @brief Total number of parameters */
#define DS_PARAMETER_COUNT     (DS_PARAMETERS_T_SIZE / sizeof(uint32_t))

/** Ensure parameter count fits in the protocol's uint16_t address field */
DS_STATIC_ASSERT(sizeof(ds_parameter_names_t) / 4 <= 65536, "Parameter count exceeds uint16_t address space (max 65536)");

/**
 * @brief Parameter storage union
 *
 * Provides dual access methods:
 * - byName: Structured access with named fields
 * - byAddress: Array access by parameter index
 */
typedef union
{
    ds_parameter_names_t    byName;                       /**< Named parameter access */
    uint32_t                byAddress[DS_PARAMETER_COUNT]; /**< Indexed parameter access */
}ds_parameters_t;

/** @brief Global parameter storage */
extern ds_parameters_t PARS;

/**
 * @brief Set parameter value by address/index
 *
 * Writes a value to the specified parameter. Enforces:
 * - Write permission check (task must have control)
 * - Address bounds checking
 *
 * @param[in]  address Parameter index (0 to DS_PARAMETER_COUNT-1)
 * @param[in]  value   Value to write
 * @param[out] reply   Operation result (WRITE_PARAMETER_OK, CONTROL_INTERFACE_ERROR, etc.)
 *
 * @note Changes are stored in RAM only until dsWriteParametersToFlash() is called
 * @note Triggers dsParameterSetCallback() on successful write
 */
void dsSetParameter(uint16_t address, uint32_t value, reply_t * reply);

/**
 * @brief Get parameter value by address/index
 *
 * Reads the value of the specified parameter.
 *
 * @param[in]  address Parameter index (0 to DS_PARAMETER_COUNT-1)
 * @param[out] reply   Operation result (READ_PARAMETER_OK or ADDRESS_OUT_OF_RANGE_ERROR)
 * @return Parameter value, or 0 if address out of range
 *
 * @note Triggers dsParameterGetCallback() on successful read
 * @note reply can be NULL if status not needed
 */
uint32_t dsGetParameter(uint16_t address, reply_t * reply);

/**
 * @brief Write parameters to flash memory (weak function)
 *
 * Saves the current parameter values to non-volatile storage.
 * Override this function with platform-specific flash write implementation.
 *
 * Platform implementations:
 * - STM32: Use HAL flash driver
 * - ESP32: Use NVS (Non-Volatile Storage) or partition API
 * - RP2040/RP2350: Use flash programming API
 *
 * @param[in] parList Pointer to parameters to save
 *
 * @note This is a weak function - MUST be overridden in your application
 * @note Default implementation does nothing
 *
 * Example for ESP32 NVS:
 * @code
 * void dsWriteParametersToFlash(ds_parameters_t * parList) {
 *     nvs_handle_t handle;
 *     nvs_open("storage", NVS_READWRITE, &handle);
 *     nvs_set_blob(handle, "params", parList, sizeof(ds_parameters_t));
 *     nvs_commit(handle);
 *     nvs_close(handle);
 * }
 * @endcode
 */
void dsWriteParametersToFlash(ds_parameters_t * parList);

/**
 * @brief Read parameters from flash memory (weak function)
 *
 * Loads parameter values from non-volatile storage.
 * Override this function with platform-specific flash read implementation.
 *
 * @param[out] parList Pointer to parameter structure to populate
 *
 * @note This is a weak function - MUST be overridden in your application
 * @note Default implementation does nothing
 * @note Called automatically by dsInitializeParameters()
 *
 * Example for ESP32 NVS:
 * @code
 * void dsReadParametersFromFlash(ds_parameters_t * parList) {
 *     nvs_handle_t handle;
 *     size_t size = sizeof(ds_parameters_t);
 *     nvs_open("storage", NVS_READONLY, &handle);
 *     nvs_get_blob(handle, "params", parList, &size);
 *     nvs_close(handle);
 * }
 * @endcode
 */
void dsReadParametersFromFlash(ds_parameters_t * parList);

/**
 * @brief Set parameters to default values (weak function)
 *
 * Resets all parameters to factory defaults.
 * Override to set application-specific default values.
 *
 * @param[out] parList Pointer to parameter structure to initialize
 *
 * @note This is a weak function - override in your application
 * @note Default implementation zeros all parameters
 */
void dsSetParametersDefaults(ds_parameters_t * parList);

/**
 * @brief Initialize parameter system (weak function)
 *
 * Called during dsInitialize(). Default behavior:
 * 1. Sets parameters to defaults
 * 2. Attempts to load from flash (if dsReadParametersFromFlash() implemented)
 *
 * Override for custom initialization logic.
 *
 * @param[in,out] parList Pointer to parameter structure to initialize
 *
 * @note This is a weak function - override in your application
 * @note Called automatically during startup
 */
void dsInitializeParameters(ds_parameters_t * parList);

/**
 * @brief Callback invoked after parameter write (weak function)
 *
 * Override to perform actions when a parameter is modified.
 * Examples: Validate values, apply changes, trigger reconfiguration.
 *
 * @param[in] address  Parameter index that was written
 * @param[in] oldValue Previous parameter value
 * @param[in] newValue New parameter value
 *
 * @note This is a weak function - override in your application
 * @note Called AFTER parameter value is updated in RAM
 * @note Changes are not automatically saved to flash
 */
void dsParameterSetCallback(uint16_t address, uint32_t oldValue, uint32_t newValue);

/**
 * @brief Callback invoked after parameter read (weak function)
 *
 * Override to perform actions when a parameter is accessed.
 * Examples: Log access, refresh cached values.
 *
 * @param[in] address Parameter index that was read
 * @param[in] value   Current parameter value
 *
 * @note This is a weak function - override in your application
 * @note Called AFTER parameter value is retrieved
 */
void dsParameterGetCallback(uint16_t address, uint32_t value);

/** @} */ // end of parameters group

#endif /* DSPARAMETERS_H_ */
