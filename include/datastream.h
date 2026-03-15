/**
 * @file datastream.h
 * @brief Datastream Protocol Library - Main Header
 * @author Sofian Jafar
 * @date 15/06/2025
 *
 * @defgroup datastream Datastream Protocol
 * @brief Binary communication protocol for embedded systems with registers and parameters
 *
 * The datastream library provides a lightweight, efficient binary protocol for embedded systems
 * to communicate over TCP/UDP. It supports:
 * - Register access (read/write real-time data)
 * - Parameter management (persistent configuration storage)
 * - System commands (reset, flash operations, etc.)
 * - Control interface management (multi-task coordination)
 * - Auto-discovery protocol (network device detection)
 * - Platform abstraction (ESP32, STM32, RP2040, RP2350, FreeRTOS+TCP)
 *
 * @{
 */

// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 Sofian Jafar
// Free for personal/hobby/non-profit use. Commercial use requires a license — see COMMERCIAL.md.

#ifndef DATASTREAM_H_
#define DATASTREAM_H_

#include <ds_default_config.h>
#include <stdint.h>

#if (DS_PLATFORM == STM32)
    #define STM32_LOGGING_ENABLE 1
#endif

#if (DS_PLATFORM == ESP32)
    #if (ESP32_LOGGING_ENABLE == 1)
        #include "esp_log.h"
        #define DSTAG "datastream"
        #define DS_LOGI(fmt, ...) ESP_LOGI(DSTAG, fmt, ##__VA_ARGS__)
        #define DS_LOGE(fmt, ...) ESP_LOGE(DSTAG, fmt, ##__VA_ARGS__)
    #else
        #define DS_LOGI(fmt, ...) ((void)0)
        #define DS_LOGE(fmt, ...) ((void)0)
    #endif
#else
    #define DS_LOGI(fmt, ...) ((void)0)
    #define DS_LOGE(fmt, ...) ((void)0)
#endif

#if (DS_PLATFORM == RP2040)
    // RP2040 specific defines
#endif

#if (DS_PLATFORM == RP2350)
    // RP2350 specific defines
#endif

#if (DS_PLATFORM == STM32)
    // STM32 specific defines
#endif

#if (DS_PLATFORM == GENERIC_PLATFORM)
    // Other platforms defines
#endif

// Compiler-agnostic struct packing macro
#if defined(__GNUC__)
    #define PACKED __attribute__((packed))
#elif defined(__ICCARM__)
    #define PACKED __packed
#elif defined(__CC_ARM)
    #define PACKED __packed
#else
    #define PACKED
#endif

// Compiler-agnostic WEAK macro
#if defined(__GNUC__) || defined(__clang__)
    #define WEAK __attribute__((weak))
#elif defined(__ICCARM__)
    #define WEAK __weak
#elif defined(__CC_ARM)
    #define WEAK __weak
#else
    #define WEAK
#endif

// Compiler-agnostic static assert macro
#ifndef DS_STATIC_ASSERT
	#if defined(__cplusplus) && __cplusplus >= 201103L
		#define DS_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
	#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
		#define DS_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
	#else
		// Fallback for older compilers: creates invalid typedef if false
		#define DS_STATIC_ASSERT(expr, msg) typedef char static_assertion_##msg[(expr) ? 1 : -1]
	#endif
#endif

// Include platform-specific headers
#if (DS_PLATFORM == ESP32)
    // ESP32 FreeRTOS specific includes
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    // ESP32 network specific includes
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <errno.h>
    typedef int dsSocket_t; // Define Socket_t for ESP32 as int
    typedef struct sockaddr_in dsSockaddr_t; // Use sockaddr_in for ESP32
#else
    // FreeRTOS includes for other platforms
    #include "FreeRTOS.h"
    #include "task.h"
    #include <stdbool.h>
    // Native FreeRTOS+TCP includes
    #include "FreeRTOS_IP.h"
    #include "FreeRTOS_Sockets.h"
    #include "main.h"  // Usually contains HAL includes for HAL_NVIC_SystemReset
    typedef struct freertos_sockaddr dsSockaddr_t; // Use FreeRTOS freertos_sockaddr type for other platforms
    typedef Socket_t dsSocket_t; // Use FreeRTOS Socket_t type
#endif

/* Dimensions the buffers into which input and output packets are placed. [bytes] */
#if(CRC8_ENABLE)
    #define DATASTREAM_INPUT_SIZE	7U
    #define DATASTREAM_OUTPUT_SIZE	7U
#else
    #define DATASTREAM_INPUT_SIZE	6U
    #define DATASTREAM_OUTPUT_SIZE	6U
#endif

// All possible reply values for the datastream protocol, add new values as needed
enum ds_reply_values
{
    SYS_COMMAND_OK_REPLY_VAL                = 0,
    READ_REGISTER_OK_REPLY_VAL              = 1,
    WRITE_REGISTER_OK_REPLY_VAL             = 2,
    READ_PARAMETER_OK_REPLY_VAL             = 3,
    WRITE_PARAMETER_OK_REPLY_VAL            = 4,
    CONTROL_INTERFACE_OK_REPLY_VAL          = 5,
    // error values are negative
    SYS_COMMAND_ERROR_REPLY_VAL             = -1,
    INPUT_TYPE_ERROR_REPLY_VAL              = -2,
    PACKET_SIZE_ERROR_REPLY_VAL             = -3,
    ADDRESS_OUT_OF_RANGE_ERROR_REPLY_VAL    = -4,
    PERMISSION_ERROR_REPLY_VAL              = -5,
    CONTROL_INTERFACE_ERROR_REPLY_VAL       = -6,
    SYNTAX_ERROR_REPLY_VAL                  = -7,
    PARAMETERS_ERROR_REPLY_VAL              = -8,
    CRC_ERROR_REPLY_VAL                     = -9
};

// define strings to be used in the replies if the reply is a string --> to be implemented later
#define SYS_COMMAND_OK_REPLY_STRING             "OK\r\n"
#define READ_REGISTER_OK_REPLY_STRING           "OK\r\n"
#define WRITE_REGISTER_OK_REPLY_STRING          "OK\r\n"
#define READ_PARAMETER_OK_REPLY_STRING          "OK\r\n"
#define WRITE_PARAMETER_OK_REPLY_STRING         "OK\r\n"
#define CONTROL_INTERFACE_OK_REPLY_STRING       "OK\r\n"
#define SYS_COMMAND_ERROR_REPLY_STRING          "Command error\r\n"
#define INPUT_TYPE_ERROR_REPLY_STRING           "Input type error\r\n"
#define PACKET_SIZE_ERROR_REPLY_STRING          "Packet size error\r\n"
#define ADDRESS_OUT_OF_RANGE_ERROR_REPLY_STRING "Address error\r\n"
#define PERMISSION_ERROR_REPLY_STRING           "Read-only error\r\n"
#define CONTROL_INTERFACE_ERROR_REPLY_STRING    "Not in Control\r\n"
#define SYNTAX_ERROR_REPLY_STRING               "Syntax Error\r\n"
#define PARAMETERS_ERROR_REPLY_STRING           "Parameters Error\r\n"
#define CRC_ERROR_REPLY_STRING                  "CRC Error\r\n"

// Core system commands processed by dsProcessSysCommand
// User-defined commands occupy 0–199; library commands are at 200–202.
// Users can define their own commands as a plain zero-based enum with no offset needed.
typedef enum
{
    // Library system commands (200-255 reserved for library use)
    ds_sys_command_READ_FLASH          = 200,  // Read parameters from flash
    ds_sys_command_WRITE_FLASH         = 201,  // Write parameters to flash
    ds_sys_command_RESET_FIRMWARE      = 202,  // Reset the firmware

    ds_sys_command_DUMMY               = 0xFFFFFFFF    // Ensure enum is 32-bit
}ds_sys_command_t;

// A union to hold the datastream output packet, can be acessed as a buffer or as a structure
typedef union 
{
    uint8_t buffer[DATASTREAM_OUTPUT_SIZE];
    struct PACKED
    {
        int8_t  status;
        uint8_t  address;
        uint32_t value;
        #if(CRC8_ENABLE)
            uint8_t  crc;
        #endif
    }contents;
}dsTxPacket;

// A union to hold the datastream input packet, can be acessed as a buffer or as a structure
typedef union 
{
    uint8_t buffer[DATASTREAM_INPUT_SIZE];
    struct PACKED
    {
        uint8_t  type;
        uint8_t  address;
        uint32_t value;
        #if(CRC8_ENABLE)
            uint8_t  crc;
        #endif
    }contents;
}dsRxPacket;

/* reply types for a given datastream input packet */
typedef struct
{
    int8_t  val;
    char    string[20];
}reply_t;

// Core input types for the datastream protocol
// Users can extend this enum by defining additional types starting from ds_type_USER_DEFINED_START
typedef enum
{
    // Core datastream input types (0-99 reserved for library use)
    ds_type_SYS_COMMAND              = 0,    // Send system commands to the system
    ds_type_READ_REGISTER            = 1,    // Read a user register from the system
    ds_type_WRITE_REGISTER           = 2,    // Write a user register to the system
    ds_type_READ_PARAMETER           = 3,    // Read a parameter from the system
    ds_type_WRITE_PARAMETER          = 4,    // Write a parameter to the system
    ds_type_CONTROL_INTERFACE        = 5,    // Set the controlling task for the system
    ds_type_READ_SYSTEM_REGISTER     = 6,    // Read a system register from the library
    ds_type_WRITE_SYSTEM_REGISTER    = 7,    // Write a system register (currently all read-only)

    // User-defined input types should start from this value
    ds_type_USER_DEFINED_START  = 100,

    ds_type_DUMMY               = 0xFFFFFFFF    // Ensure enum is 32-bit
} ds_type_t;

/** @defgroup reply_objects Reply Objects
 *  @brief Pre-initialized reply structures for common responses
 *  @{
 */
extern const reply_t SYS_COMMAND_OK;              /**< System command executed successfully */
extern const reply_t READ_REGISTER_OK;            /**< Register read successful */
extern const reply_t WRITE_REGISTER_OK;           /**< Register write successful */
extern const reply_t READ_PARAMETER_OK;           /**< Parameter read successful */
extern const reply_t WRITE_PARAMETER_OK;          /**< Parameter write successful */
extern const reply_t CONTROL_INTERFACE_OK;        /**< Control interface command successful */
extern const reply_t SYS_COMMAND_ERROR;           /**< System command execution failed */
extern const reply_t INPUT_TYPE_ERROR;            /**< Unknown or invalid input type */
extern const reply_t PACKET_SIZE_ERROR;           /**< Received packet has incorrect size */
extern const reply_t ADDRESS_OUT_OF_RANGE_ERROR;  /**< Register/parameter address out of bounds */
extern const reply_t PERMISSION_ERROR;            /**< Attempted write to read-only register */
extern const reply_t CONTROL_INTERFACE_ERROR;     /**< Task lacks write permission */
extern const reply_t SYNTAX_ERROR;                /**< Command syntax error */
extern const reply_t PARAMETERS_ERROR;            /**< Invalid parameters */
/** @} */

/**
 * @brief Initialize the datastream subsystem
 *
 * This function must be called once before using any other datastream functions.
 * It initializes:
 * - Reply structures with predefined values
 * - Parameters (loads from flash if available)
 * - Registers (sets initial values)
 * - Control interface state
 *
 * @note This function uses a static flag to ensure it only initializes once
 * @note Safe to call multiple times (subsequent calls are ignored)
 */
void dsInitialize(void);

/**
 * @brief Register application-specific CLI commands (Reserved for future use)
 *
 * @note This function is currently a placeholder for future CLI functionality
 * @note Override this weak function in your application if needed
 */
void dsRegisterApplicationCLICommands(void);

/**
 * @brief Process a received datastream packet
 *
 * Main packet processing function that:
 * - Validates CRC (if enabled)
 * - Routes packet to appropriate handler based on type
 * - Generates response packet
 * - Updates statistics counters
 *
 * @param[in]  inPacket  Pointer to received packet structure
 * @param[out] outPacket Pointer to response packet structure (will be populated)
 *
 * @note Input packet must be DATASTREAM_INPUT_SIZE bytes
 * @note Output packet will be DATASTREAM_OUTPUT_SIZE bytes
 */
void dsProcessPacket(dsRxPacket *inPacket, dsTxPacket *outPacket);

/**
 * @brief Process user-defined packet types (weak function)
 *
 * Override this function to handle custom packet types (>= ds_type_USER_DEFINED_START).
 * Default implementation returns false (type not handled).
 *
 * @param[in]  inPacket  Pointer to received packet with user-defined type
 * @param[out] outPacket Pointer to response packet to populate
 * @param[out] reply     Pointer to reply structure to set status
 * @return true if packet was handled, false if type unknown
 *
 * @note This is a weak function - override in your application
 */
bool dsProcessUserDefinedType(dsRxPacket *inPacket, dsTxPacket *outPacket, reply_t *reply);

/**
 * @brief Generate error response for incorrect packet size
 *
 * @param[in]  inPacketSize Actual size of received packet
 * @param[out] outPacket    Response packet to populate with error
 */
void dsPacketSizeError(uint32_t inPacketSize, dsTxPacket *outPacket);

/**
 * @brief Check if current task has write permission
 *
 * Verifies that the calling task is registered and has control interface permission.
 *
 * @return true if task has write permission, false otherwise
 *
 * @note Tasks must be registered via dsRegisterControlTask()
 * @note Control must be acquired via CONTROL_INTERFACE command
 */
bool dsCheckTaskWritePermission(void);

/**
 * @brief Process system command packet
 *
 * Handles core system commands:
 * - READ_FLASH: Load parameters from flash
 * - WRITE_FLASH: Save parameters to flash
 * - RESET_FIRMWARE: Trigger system reset
 * - User-defined commands (0–199, routed to dsProcessUserSysCommand)
 *
 * @param[in]  inputPacket  Pointer to command packet
 * @param[out] outputPacket Pointer to response packet
 * @return true if command processed, false otherwise
 */
bool dsProcessSysCommand(const dsRxPacket *inputPacket, dsTxPacket *outputPacket);

/**
 * @brief Process user-defined system commands (weak function)
 *
 * Override to handle custom system commands (any command value not matching 200–202).
 *
 * @param[in]  inputPacket  Pointer to command packet
 * @param[out] outputPacket Pointer to response packet
 * @return true if command processed, false if unknown
 *
 * @note This is a weak function - override in your application
 */
bool dsProcessUserSysCommand(const dsRxPacket *inputPacket, dsTxPacket *outputPacket) __attribute__((weak));

/**
 * @brief Process control interface command
 *
 * Handles task registration for write permission control.
 *
 * @return true if control acquired successfully, false otherwise
 */
bool dsProcessControlInterfaceCommand(void);

/**
 * @brief Error callback hook (weak function)
 *
 * Called when an error occurs during packet processing.
 * Override for custom error handling/logging.
 *
 * @param[in] reply     Error reply structure
 * @param[in] inPacket  Packet that caused error
 * @param[in] outPacket Response packet
 *
 * @note This is a weak function - override in your application
 */
void dsErrorCallback(reply_t reply, dsRxPacket *inPacket, dsTxPacket *outPacket);

/**
 * @brief Get the current board name
 *
 * Returns the runtime board name. Defaults to DS_BOARD_NAME at startup,
 * overridden by flash-stored value if available.
 *
 * @return Pointer to the board name string (null-terminated, max 15 chars)
 */
const char* dsGetBoardName(void);

/**
 * @brief Set the board name at runtime
 *
 * Updates the in-memory board name. Use sys command WRITE_FLASH to persist.
 *
 * @param[in] name New board name (max 15 chars, will be null-terminated)
 */
void dsSetBoardName(const char* name);

/**
 * @brief Write board name to flash (weak function)
 *
 * Called automatically alongside dsWriteParametersToFlash() when
 * the WRITE_FLASH system command is issued.
 *
 * @param[in] name Board name to persist (null-terminated, 16 bytes max)
 *
 * @note This is a weak function - override in your application
 */
void dsWriteBoardNameToFlash(const char* name);

/**
 * @brief Read board name from flash (weak function)
 *
 * Called during dsInitialize() and when READ_FLASH system command is issued.
 * If flash contains a valid name, copy it into the buffer.
 *
 * @param[out] name   Buffer to write board name into
 * @param[in]  maxLen Size of the buffer (16)
 *
 * @note This is a weak function - override in your application
 */
void dsReadBoardNameFromFlash(char* name, size_t maxLen);

/**
 * @brief Control interface types for multi-task coordination
 *
 * Defines which task/interface has write permission to registers and parameters.
 * Users can extend this enum by defining values >= ds_control_USER_DEFINED_START.
 */
typedef enum
{
    ds_control_UNDECIDED       = 0,    /**< No task has control (initial state) */
    ds_control_TCP_DATASTREAM  = 1,    /**< TCP datastream task has control */
    ds_control_UDP_DATASTREAM  = 2,    /**< UDP datastream task has control */

    ds_control_USER_DEFINED_START = 100, /**< Start value for user-defined control types */

    // Example user-defined types (can be overridden in application)
    ds_control_TCP_CLI         = 101,  /**< TCP CLI interface control */
    ds_control_USB             = 102,  /**< USB CLI interface control */

    ds_control_DUMMY           = 0xFFFFFFFF    /**< Ensure enum is 32-bit */
} ds_control_interface_t;

/**
 * @brief Registered task information for control interface
 *
 * Stores task metadata for runtime control interface management.
 */
typedef struct {
    TaskHandle_t taskHandle;                /**< FreeRTOS task handle */
    ds_control_interface_t controlType;     /**< Control interface type */
    #ifdef configMAX_TASK_NAME_LEN
    char taskName[configMAX_TASK_NAME_LEN]; /**< Task name for debugging */
    #else
    char taskName[16];                       /**< Task name (default size) */
    #endif
} ds_registered_task_t;

/**
 * @brief Maximum number of tasks that can be registered
 *
 * Override this in your application config if you need more registered tasks.
 */
#ifndef DS_MAX_REGISTERED_TASKS
    #define DS_MAX_REGISTERED_TASKS 8
#endif

/** @defgroup task_registration Task Registration API
 *  @brief Runtime task registration for control interface management
 *  @{
 */

/**
 * @brief Register a task for control interface management
 *
 * Allows tasks to be registered at runtime for write permission control.
 * Registered tasks can acquire control via CONTROL_INTERFACE command.
 *
 * @param[in] taskHandle  FreeRTOS task handle to register
 * @param[in] controlType Control interface type for this task
 * @param[in] taskName    Optional task name for debugging (can be NULL)
 * @return true if registered successfully, false if registration failed
 *
 * @note If task is already registered, updates its control type
 * @note Returns false if maximum number of tasks (DS_MAX_REGISTERED_TASKS) reached
 */
bool dsRegisterControlTask(TaskHandle_t taskHandle, ds_control_interface_t controlType, const char* taskName);

/**
 * @brief Unregister a task from control interface management
 *
 * @param[in] taskHandle FreeRTOS task handle to unregister
 * @return true if unregistered successfully, false if task not found
 */
bool dsUnregisterControlTask(TaskHandle_t taskHandle);

/**
 * @brief Clear all registered tasks
 *
 * Removes all task registrations. Use with caution.
 */
void dsClearRegisteredTasks(void);

/**
 * @brief Get the control interface type for a registered task
 *
 * @param[in] taskHandle FreeRTOS task handle to query
 * @return Control interface type, or ds_control_UNDECIDED if task not registered
 */
ds_control_interface_t dsGetTaskControlType(TaskHandle_t taskHandle);

/** @} */ // end of task_registration

#if (DS_AUTO_DETECTION_ENABLE == 1)

/** @defgroup auto_detection Auto-Detection Protocol
 *  @brief UDP broadcast discovery protocol for finding datastream boards on network
 *  @{
 */

/**
 * @brief Auto-detection command types
 */
typedef enum
{
    DS_DISCOVERY_REQUEST    = 0x01,  /**< Discovery request from client */
    DS_DISCOVERY_RESPONSE   = 0x02   /**< Discovery response from board */
} ds_discovery_command_t;

/**
 * @brief Discovery request packet structure (5 bytes)
 *
 * Sent via UDP broadcast to discover datastream boards on the network.
 */
typedef struct PACKED
{
    uint32_t magic;    /**< DS_DISCOVERY_MAGIC (0xDEADBEEF) for packet identification */
    uint8_t  command;  /**< DS_DISCOVERY_REQUEST */
} ds_discovery_request_t;

/**
 * @brief Discovery response packet structure (44 bytes)
 *
 * Sent via UDP unicast in response to discovery request.
 * Contains board identification and network information.
 */
typedef struct PACKED
{
    uint32_t magic;              /**< DS_DISCOVERY_MAGIC (0xDEADBEEF) */
    uint8_t  command;            /**< DS_DISCOVERY_RESPONSE */
    uint8_t  board_type;         /**< Board type identifier (ds_board_type_t) */
    uint16_t firmware_version;   /**< Firmware version number */
    uint32_t board_id;           /**< Unique board identifier */
    uint32_t ip_address;         /**< Board's IP address (network byte order) */
    uint16_t tcp_port;           /**< TCP datastream port */
    uint16_t udp_port;           /**< UDP datastream port */
    uint8_t  mac_address[6];     /**< MAC address (6 bytes) */
    uint8_t  reserved[2];        /**< Reserved for alignment (must be 0) */
    char     board_name[16];     /**< Human-readable board name (null-terminated) */
} ds_discovery_response_t;

/**
 * @brief Initialize auto-detection subsystem
 *
 * Called automatically by dsInitialize() when DS_AUTO_DETECTION_ENABLE is set.
 * No need to call manually.
 */
void dsAutoDetectionInit(void);

/**
 * @brief Process discovery packet and generate response
 *
 * Validates incoming discovery request and generates response packet with
 * board information (IP, MAC, ports, etc.).
 *
 * @param[in]  rxBuffer  Received packet buffer
 * @param[in]  rxSize    Size of received packet
 * @param[out] txBuffer  Buffer to write response packet
 * @param[out] txSize    Size of response packet written
 * @param[in]  clientIP  Client IP address (for logging)
 * @return true if discovery packet was processed, false if invalid/not a discovery packet
 *
 * @note txBuffer must be at least sizeof(ds_discovery_response_t) bytes
 */
bool dsProcessDiscoveryPacket(const uint8_t *rxBuffer, size_t rxSize, uint8_t *txBuffer, size_t *txSize, uint32_t clientIP);

/**
 * @brief Get MAC address from network interface (weak function)
 *
 * Platform-specific MAC address retrieval. Override for custom implementations.
 * Default implementations:
 * - ESP32: Uses esp_netif_get_mac() with fallback to esp_read_mac()
 * - FreeRTOS+TCP: Uses FreeRTOS_FirstEndPoint()
 *
 * @param[out] mac_address Buffer to store 6-byte MAC address
 * @return true if MAC retrieved successfully, false on error
 *
 * @note This is a weak function - can be overridden in your application
 */
bool dsGetMACAddress(uint8_t mac_address[6]);

/** @} */ // end of auto_detection

#endif // DS_AUTO_DETECTION_ENABLE

/** @} */ // end of datastream group

#endif /* DATASTREAM_H_ */
