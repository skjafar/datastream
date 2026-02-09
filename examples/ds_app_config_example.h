/*
 * ds_app_config_example.h
 *
 * Example application configuration for datastream library
 *
 * This file demonstrates how to configure the datastream library for your project
 * using the external configuration system. The library automatically includes
 * this file if named 'ds_app_config.h' and placed in your project's include path.
 * 
 * Usage:
 * 1. Copy this file to your project as 'ds_app_config.h'
 * 2. Customize the settings for your application
 * 3. Create your custom register/parameter definition files if needed
 * 4. Include datastream.h in your application - configuration is automatic!
 * 
 * See USER_INTEGRATION_GUIDE.md for detailed configuration instructions.
 */

#ifndef DS_APP_CONFIG_EXAMPLE_H_
#define DS_APP_CONFIG_EXAMPLE_H_

/****** Datastream Library Configuration ******/

// Platform selection - choose one: STM32, ESP32, RP2040, RP2350, GENERIC_PLATFORM
// Platform constants are automatically defined in ds_platforms.h
#define DS_PLATFORM STM32

// Basic datastream settings
#define DS_STATS_ENABLE    1        // Enable packet statistics
#define CRC8_ENABLE        0        // Disable CRC for TCP, enable for UDP

// Task configuration
#define DS_TCP_TASK_PRIORITY    6
#define DS_UDP_TASK_PRIORITY    6
#define DS_TCP_PORT             2009
#define DS_UDP_PORT             2011

// Board identification for auto-detection
#define DS_BOARD_TYPE           1               // Your custom board type
#define DS_BOARD_NAME           "MyBoard"       // Your board name
#define DS_FIRMWARE_VERSION     0x0100          // Version 1.0
#define DS_BOARD_ID             123             // Unique board ID

/****** Custom Register/Parameter Definitions (Optional) ******/

// Define custom register and parameter files if you need application-specific definitions
// If not defined, the library uses minimal default definitions
#define DS_USER_REGISTER_DEFINITIONS    "my_registers.h"
#define DS_USER_PARAMETER_DEFINITIONS   "my_parameters.h"

// Optional: Suppress warnings about using default definitions
// #define DS_SUPPRESS_DEFAULT_WARNINGS

/****** Note: Configuration Complete ******/
// The library automatically includes ds_default_config.h which will process these settings
// No need to manually include anything else - just include "datastream.h" in your application

#endif /* DS_APP_CONFIG_EXAMPLE_H_ */