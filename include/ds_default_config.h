/*
 * ds_default_config.h
 *
 * Default configuration for datastream protocol library
 * 
 * This file provides default configuration values and automatically includes
 * application-specific configuration if available (ds_app_config.h).
 * 
 * For custom configuration, create ds_app_config.h in your project or use
 * build system defines. See USER_INTEGRATION_GUIDE.md for details.
 *
 * Created: 10/06/2025 03:19:06 AM
 *  Author: Sofian.jafar
 */

#ifndef DATASTREAM_CONFIG_H_
#define DATASTREAM_CONFIG_H_

#include <assert.h>

// Platform-specific includes - must be included before ds_app_config.h
#include "ds_platforms.h"

/********************* Auto-include application config if available ***************** */
// Automatically include application configuration if present
// This allows users to configure the library without modifying library files
// Uses __has_include to detect if ds_app_config.h exists before including it
#if defined(__has_include)
    #if __has_include("ds_app_config.h")
        #include "ds_app_config.h"
        #define DS_APP_CONFIG_INCLUDED 1
    #else
        #define DS_APP_CONFIG_INCLUDED 0
    #endif
#else
    // Fallback for compilers that don't support __has_include
    // Try to include anyway - will fail at compile time if not found
    #if defined(DS_USE_APP_CONFIG) || !defined(DS_NO_APP_CONFIG)
        #include "ds_app_config.h"
        #define DS_APP_CONFIG_INCLUDED 1
    #else
        #define DS_APP_CONFIG_INCLUDED 0
    #endif
#endif

/********************* Datastream Configuration starts here ***************** */
/* set DS_STATS_ENABLE to 1 to count the number of packets received and errors */
#ifndef DS_STATS_ENABLE
#define DS_STATS_ENABLE    1
#endif

// Platform selection - configure in ds_app_config.h or build system defines
// Available platforms: STM32, ESP32, RP2040, RP2350, GENERIC_PLATFORM
// Platform constants are defined in ds_platforms.h
#ifndef DS_PLATFORM
#define DS_PLATFORM STM32
#endif

// Enable logging for ESP32 platform, set to 0 to disable logging
// This is used to log messages to the console, useful for debugging
// Set to 1 to enable logging, 0 to disable
#if (DS_PLATFORM == ESP32)
    #define ESP32_LOGGING_ENABLE 1
#endif

// Enable CRC8 in the packet, probably not required when using TCP.
// highly recommended when using UDP or other interfaces
#ifndef CRC8_ENABLE
#define CRC8_ENABLE 0
#endif

/****** TCP Datastream control interface **********/
// Task Priority
#ifndef DS_TCP_TASK_PRIORITY
#define DS_TCP_TASK_PRIORITY    (tskIDLE_PRIORITY + 6)
#endif

// Task stack size
#ifndef DS_TCP_TASK_STACK_SIZE
#define DS_TCP_TASK_STACK_SIZE  (2 * configMINIMAL_STACK_SIZE)
#endif

// Task name
#ifndef DS_TCP_TASK_NAME
#define DS_TCP_TASK_NAME        "[DS] TCP"
#endif

// Port for TCP socket interface
#ifndef DS_TCP_PORT
#define DS_TCP_PORT             2009
#endif

/****** UDP Datastream control interface **********/
// Task Priority
#ifndef DS_UDP_TASK_PRIORITY
#define DS_UDP_TASK_PRIORITY    (tskIDLE_PRIORITY + 6)
#endif

// Task stack size
#ifndef DS_UDP_TASK_STACK_SIZE
#define DS_UDP_TASK_STACK_SIZE  (2 * configMINIMAL_STACK_SIZE)
#endif

// Task name
#ifndef DS_UDP_TASK_NAME
#define DS_UDP_TASK_NAME        "[DS] UDP"
#endif

// Port for UDP socket interface
#ifndef DS_UDP_PORT
#define DS_UDP_PORT             2011
#endif

/****** CLI over TCP interface **********/
// Enable CLI over TCP feature
// Set to 1 to enable, 0 to disable (default: disabled, compiles to nothing)
#ifndef DS_CLI_ENABLE
#define DS_CLI_ENABLE           0
#endif

// Task Priority
#ifndef DS_CLI_TASK_PRIORITY
#define DS_CLI_TASK_PRIORITY    (tskIDLE_PRIORITY + 5)
#endif

// Task stack size
#ifndef DS_CLI_TASK_STACK_SIZE
#define DS_CLI_TASK_STACK_SIZE  (2 * configMINIMAL_STACK_SIZE)
#endif

// Task name
#ifndef DS_CLI_TASK_NAME
#define DS_CLI_TASK_NAME        "[DS] CLI"
#endif

// Port for CLI TCP socket interface
#ifndef DS_CLI_PORT
#define DS_CLI_PORT             2013
#endif

// Maximum input line length
#ifndef DS_CLI_MAX_INPUT_LENGTH
#define DS_CLI_MAX_INPUT_LENGTH     128
#endif

// Maximum output line length
#ifndef DS_CLI_MAX_OUTPUT_LENGTH
#define DS_CLI_MAX_OUTPUT_LENGTH    256
#endif

/****** Board Auto-Detection Configuration **********/
// Enable board auto-detection feature
#ifndef DS_AUTO_DETECTION_ENABLE
#define DS_AUTO_DETECTION_ENABLE    1
#endif

// Board identification magic number
#ifndef DS_DISCOVERY_MAGIC
#define DS_DISCOVERY_MAGIC          0xDEADBEEF
#endif

// Board type definitions - customize for your boards
typedef enum
{
    BOARD_TYPE_UNKNOWN          = 0,
    BOARD_TYPE_CUSTOM_1         = 1,
    BOARD_TYPE_CUSTOM_2         = 2,
    BOARD_TYPE_CUSTOM_3         = 3
} ds_board_type_t;

// Define your board type here
#ifndef DS_BOARD_TYPE
    #define DS_BOARD_TYPE BOARD_TYPE_CUSTOM_1
#endif

// Board name - customize for your board
#ifndef DS_BOARD_NAME
    #define DS_BOARD_NAME "Datastream test"
#endif

// Firmware version - customize for your firmware
#ifndef DS_FIRMWARE_VERSION
    #define DS_FIRMWARE_VERSION 0x0200  // Version 2.0
#endif

// Board ID - should be unique per board
#ifndef DS_BOARD_ID
    #define DS_BOARD_ID 99
#endif

// ds_control_interface_t has been moved to datastream.h
// Users can now define their own control interface types starting from ds_control_USER_DEFINED_START
// Example:
// #define MY_CUSTOM_INTERFACE (ds_control_USER_DEFINED_START + 1)

/****** User Register and Parameter Definitions **********/
// External Configuration System:
// Define custom register and parameter headers in ds_app_config.h or build system:
// 
// Method 1: Application Configuration File (ds_app_config.h)
//   #define DS_USER_REGISTER_DEFINITIONS "my_registers.h"  
//   #define DS_USER_PARAMETER_DEFINITIONS "my_parameters.h"
//
// Method 2: Build System Defines
//   target_compile_definitions(target PRIVATE DS_USER_REGISTER_DEFINITIONS="config/registers.h")
//
// Method 3: Direct preprocessor defines (before including datastream.h)
//   #define DS_USER_REGISTER_DEFINITIONS "app_registers.h"
//
// Default Behavior:
// If custom definitions are not provided, the library uses minimal default definitions
// and will show informational compiler warnings to guide configuration.
//
// Warning Control:
// Define DS_SUPPRESS_DEFAULT_WARNINGS to suppress informational warnings about default usage

// Check if user has provided custom definitions
#ifndef DS_USER_REGISTER_DEFINITIONS
    // Use default register definitions from the library
    #define DS_USER_REGISTER_DEFINITIONS "ds_minimal_register_names.h"
    #define DS_USING_DEFAULT_REGISTERS
    #ifndef DS_SUPPRESS_DEFAULT_WARNINGS
        #pragma message("DATASTREAM: Using default register definitions. Define DS_USER_REGISTER_DEFINITIONS to use custom registers.")
    #endif
#endif

#ifndef DS_USER_PARAMETER_DEFINITIONS  
    // Use default parameter definitions from the library
    #define DS_USER_PARAMETER_DEFINITIONS "ds_minimal_parameter_names.h"
    #define DS_USING_DEFAULT_PARAMETERS
    #ifndef DS_SUPPRESS_DEFAULT_WARNINGS
        #pragma message("DATASTREAM: Using default parameter definitions. Define DS_USER_PARAMETER_DEFINITIONS to use custom parameters.")
    #endif
#endif

// Generate a summary warning if using all default definitions  
// We track this by checking if any of the "using default" flags are set
#ifndef DS_SUPPRESS_DEFAULT_WARNINGS
    #ifdef DS_USING_DEFAULT_REGISTERS
        #ifdef DS_USING_DEFAULT_PARAMETERS
                #pragma message("DATASTREAM WARNING: Using all default definitions. For shared library usage, define custom definitions. See USER_INTEGRATION_GUIDE.md")
        #endif
    #endif
#endif

/********************* Configuration Status Information ***************** */
// Optional: Provide feedback about configuration detection
#if defined(DS_SHOW_CONFIG_INFO) && !defined(DS_SUPPRESS_DEFAULT_WARNINGS)
    #if DS_APP_CONFIG_INCLUDED
        #pragma message("DATASTREAM INFO: ds_app_config.h detected and included")
    #else
        #pragma message("DATASTREAM INFO: ds_app_config.h not found, using defaults")
    #endif
#endif

#endif /* DATASTREAM_CONFIG_H_ */
