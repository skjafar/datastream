# Datastream Library - User Integration Guide

Version 2.0 | Last Updated: 2025-01-10

This guide explains how to integrate the datastream library into your project without modifying the library files, taking advantage of the new external configuration system and recent improvements.

---

## Table of Contents

1. [Quick Start (5 Minutes)](#quick-start-5-minutes)
2. [Testing Your Setup](#testing-your-setup)
3. [Overview](#overview)
4. [What's New in Version 2.0](#whats-new-in-version-20)
5. [Configuration Methods](#method-1-automatic-configuration-recommended---new-in-20)
6. [Defining Registers and Parameters](#defining-your-registers)
7. [Task Registration System](#task-registration-system-new-in-20)
8. [Migration from Older Versions](#migration-from-older-versions)
9. [Troubleshooting](#troubleshooting)

---

## Quick Start (5 Minutes)

Follow these steps to get datastream running on your embedded device.

### Step 1: Add Library to Your Project

Copy the datastream library to your project directory:

```
your_project/
├── src/
│   └── main.c
├── include/
│   └── ds_app_config.h      <-- Your configuration
└── lib/
    └── datastream/          <-- Library (do not modify)
        ├── include/
        └── src/
```

Add to your build system:

**CMake:**
```cmake
add_subdirectory(lib/datastream)
target_link_libraries(${PROJECT_NAME} PRIVATE datastream)
target_include_directories(${PROJECT_NAME} PRIVATE include)
```

**ESP-IDF (component):**
```cmake
# lib/datastream/CMakeLists.txt
idf_component_register(
    SRCS "src/datastream.c" "src/dsTCP.c" "src/dsUDP.c" "src/dsRegisters.c" "src/dsParameters.c"
    INCLUDE_DIRS "include" "../../include"
)
```

### Step 2: Create Configuration File

Create `include/ds_app_config.h`:

```c
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

// Required: Select your platform
#define DS_PLATFORM ESP32   // Options: ESP32, STM32, RP2040, RP2350, GENERIC_PLATFORM

// Required: Board identification
#define DS_BOARD_NAME "MyDevice"
#define DS_BOARD_TYPE 1
#define DS_BOARD_ID   0x00000001

// Network ports (defaults shown)
#define DS_TCP_PORT 2009
#define DS_UDP_PORT 2011

// Features
#define DS_AUTO_DETECTION_ENABLE 1
#define DS_STATS_ENABLE 1
#define CRC8_ENABLE 0

#endif // DS_APP_CONFIG_H_
```

### Step 3: Initialize in Your Application

```c
#include "datastream.h"
#include "dsTCP.h"
#include "dsUDP.h"

void app_main(void) {
    // 1. Initialize datastream
    dsInitialize();

    // 2. Register current task for control interface
    TaskHandle_t mainTask = xTaskGetCurrentTaskHandle();
    dsRegisterControlTask(mainTask, ds_control_TCP, "MainTask");

    // 3. Start network tasks
    dsTCPTaskCreate();   // TCP server on port 2009
    dsUDPTaskCreate();   // UDP server on port 2011 (with auto-detection)

    // 4. Start scheduler (if not already running)
    vTaskStartScheduler();
}
```

### Step 4: Build and Flash

Build your project and flash to the target device. The device will now:
- Listen for TCP connections on port 2009
- Listen for UDP packets on port 2011
- Respond to auto-detection broadcasts

---

## Testing Your Setup

Before deploying to hardware, verify everything works using the provided helper scripts.

### Prerequisites

- Python 3.6 or later
- Network access to your device (or use mock server for offline testing)

### Option A: Test Without Hardware (Mock Server)

Use the mock ESP32 server to test your client applications:

```bash
# Terminal 1: Start mock discovery server
cd tools
python3 mock_esp32_discovery.py
```

Expected output:
```
Mock ESP32 Discovery Server
Listening on 0.0.0.0:2011
Board: MockESP32-Test, Type: 1, Serial: 0x12345678
------------------------------------------------------------
Waiting for discovery requests... Press Ctrl+C to stop
```

Now send a discovery broadcast from your client or test script. The mock server responds with valid discovery packets.

### Option B: Test With Hardware

Verify your flashed device responds correctly:

```bash
# Terminal 1: Listen for UDP responses
cd tools
python3 test_udp_receive.py
```

Expected output:
```
Listening on 192.168.1.100:54321
Waiting for UDP packets... Press Ctrl+C to stop
```

Send a discovery broadcast to your network. You should see:
```
Received 44 bytes from 192.168.1.50:2011
Data: efbeadde0201000178563412...
```

### Option C: Quick Discovery Test

Use the Python discovery example from the README:

```python
import socket
import struct

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.settimeout(2.0)

# Discovery request: magic (0xDEADBEEF) + command (0x01) + padding
request = struct.pack('<I B 3s', 0xDEADBEEF, 0x01, b'\x00\x00\x00')
sock.sendto(request, ('<broadcast>', 2011))

try:
    data, addr = sock.recvfrom(1024)
    print(f"Found device at {addr[0]}")
except socket.timeout:
    print("No devices found")
finally:
    sock.close()
```

### Verifying TCP Communication

Test parameter read/write via TCP:

```python
import socket
import struct

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.1.50', 2009))  # Replace with your device IP

# Read parameter at address 0: type=3 (READ_PARAMETER), addr=0, value=0
packet = struct.pack('<BB I', 3, 0, 0)
sock.send(packet)

response = sock.recv(6)
status, addr, value = struct.unpack('<bB I', response)
print(f"Parameter 0 = {value} (status: {status})")

sock.close()
```

### Common Test Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| No response to discovery | UDP port blocked | Check firewall, ensure port 2011 is open |
| TCP connection refused | TCP task not running | Verify `dsTCPTaskCreate()` was called |
| Mock server not receiving | Broadcast not reaching | Check network configuration |
| Timeout on receive | Device not on network | Verify device IP, check with ping |

---

## Overview

The datastream library 2.0 introduces a modern external configuration system that allows you to:

- **Use the library without modifying any library files** - All customization via external headers
- **Define application-specific registers and parameters** - Custom data structures for your needs
- **Maintain clean separation** - Library code remains pristine and upgradeable
- **Automatic platform detection** - Smart defaults based on your target platform
- **Runtime task registration** - Dynamic control interface management
- **Zero-warning documentation** - Complete Doxygen API reference

## What's New in Version 2.0

### Major Changes
1. **External Configuration System** - New `ds_app_config.h` automatic detection
2. **Modular Network Interfaces** - Separated TCP (dsTCP.h/c) and UDP (dsUDP.h/c)
3. **Task Registration System** - Runtime control interface management
4. **Platform Abstraction** - Centralized platform definitions (ds_platforms.h)
5. **Bug Fixes** - NULL pointer safety, initialization fixes, coding style standardization
6. **Enhanced ESP32 Support** - Full IP/MAC address retrieval implementation
7. **Comprehensive Documentation** - Complete Doxygen API documentation with examples

### Breaking Changes from Version 1.x
- Removed `datastream_config.h` (replaced by `ds_app_config.h`)
- Removed `TCPdatastream.h/c` and `UDPdatastream.h/c` (replaced by `dsTCP.h/c` and `dsUDP.h/c`)
- Task handles now must be registered via `dsRegisterControlTask()` for control interface
- Configuration hierarchy changed: Default → Platform → Application

## Quick Start

### Method 1: Automatic Configuration (Recommended - New in 2.0)

The library automatically detects and includes `ds_app_config.h` from your project's include path. It uses the `__has_include` preprocessor feature to check if the file exists before including it, so the library works even without this file.

1. **Create `ds_app_config.h` in your project root:**
   ```c
   // ds_app_config.h - Place in your project's include path
   #ifndef DS_APP_CONFIG_H_
   #define DS_APP_CONFIG_H_

   // Platform selection (ESP32, STM32, RP2040, RP2350, GENERIC_PLATFORM)
   #define DS_PLATFORM ESP32

   // Optional: Custom register and parameter definitions
   #define DS_USER_REGISTER_DEFINITIONS "config/my_registers.h"
   #define DS_USER_PARAMETER_DEFINITIONS "config/my_parameters.h"

   // Optional: Board identification for auto-detection
   #define DS_BOARD_TYPE 1
   #define DS_BOARD_NAME "My Custom Board"
   #define DS_BOARD_ID 0x12345678

   // Optional: Network configuration
   #define DS_TCP_PORT 2009
   #define DS_UDP_PORT 2011

   // Optional: Feature flags
   #define DS_AUTO_DETECTION_ENABLE 1
   #define DS_STATS_ENABLE 1
   #define CRC8_ENABLE 0

   #endif // DS_APP_CONFIG_H_
   ```

2. **Include datastream headers in your application:**
   ```c
   #include "datastream.h"  // Automatically includes ds_app_config.h
   #include "dsTCP.h"       // For TCP support
   #include "dsUDP.h"       // For UDP support
   ```

3. **Initialize and register tasks:**
   ```c
   void app_main(void) {
       // Initialize datastream
       dsInitialize();

       // Register your task for control interface
       TaskHandle_t myTask = xTaskGetCurrentTaskHandle();
       dsRegisterControlTask(myTask, ds_control_TCP, "MainTask");

       // Start network tasks
       dsTCPTaskCreate();
       dsUDPTaskCreate();
   }
   ```

### Method 2: Using Preprocessor Defines (Build System)

Configure via CMake, Makefile, or IDE settings without creating `ds_app_config.h`:

**CMake:**
```cmake
target_compile_definitions(your_target PRIVATE
    DS_PLATFORM=ESP32
    DS_USER_REGISTER_DEFINITIONS="config/my_registers.h"
    DS_USER_PARAMETER_DEFINITIONS="config/my_parameters.h"
    DS_BOARD_NAME="My Board"
)
```

**Makefile:**
```makefile
CFLAGS += -DDS_PLATFORM=ESP32
CFLAGS += -DDS_USER_REGISTER_DEFINITIONS=\"config/my_registers.h\"
CFLAGS += -DDS_BOARD_NAME=\"My\ Board\"
```

### Method 3: Direct Include (Legacy)

Define configuration before including datastream headers:
```c
#define DS_PLATFORM STM32
#define DS_USER_REGISTER_DEFINITIONS "my_registers.h"
#include "datastream.h"
```

## File Structure

### Your Project Structure
```
your_project/
├── src/
│   └── main.c                     // Include datastream.h here
├── config/  
│   ├── my_registers.h             // Your register definitions
│   ├── my_parameters.h            // Your parameter definitions
│   ├── my_types.h                 // Your application types
│   └── project_config.h           // Optional: main config header
└── datastream_library/            // The datastream library (unchanged)
    ├── include/
    │   ├── datastream.h
    │   ├── datastream_default_config.h
    │   └── ...
    └── src/
        └── ...
```

## Defining Your Registers

### Register Definition Template (`my_registers.h`)

```c
#ifndef MY_REGISTERS_H_
#define MY_REGISTERS_H_

// Include the datastream config and your types
#include "datastream_default_config.h"
#include DS_USER_TYPES_DEFINITIONS

typedef struct PACKED
{
    // Read-only registers
    #if (DS_STATS_ENABLE == 1)
    uint32_t DS_PACKET_COUNT;      // Required for statistics
    uint32_t DS_ERROR_COUNT;       // Required for statistics  
    #endif
    
    ds_control_interface_t CONTROL_INTERFACE;  // Required for control
    
    // Your application registers
    uint32_t            STATUS_FLAGS;
    float               SENSOR_READING;
    uint32_t            SYSTEM_UPTIME;
    
    // Read-write registers
    uint32_t            CONTROL_MODE;
    uint32_t            OUTPUT_ENABLE;
    
} ds_register_names_t;

// Size validation
DS_STATIC_ASSERT(sizeof(ds_register_names_t) % 4 == 0, "Register struct must be 4-byte aligned");

#define DS_REGISTERS_BY_NAME_T_SIZE     sizeof(ds_register_names_t)
#define DS_REGISTER_COUNT               DS_REGISTERS_BY_NAME_T_SIZE / sizeof(uint32_t)
#define DS_REGISTERS_READ_ONLY_COUNT    6  // Count your read-only registers

#endif
```

### Important Register Requirements

1. **Required registers:**
   - `CONTROL_INTERFACE` (type: `ds_control_interface_t`) - Required for datastream operation
   - `DS_PACKET_COUNT` and `DS_ERROR_COUNT` (if `DS_STATS_ENABLE == 1`)

2. **All data must be 32-bit aligned** (use `uint32_t`, `int32_t`, `float`, or your custom 32-bit enums)

3. **Update `DS_REGISTERS_READ_ONLY_COUNT`** to match the number of read-only registers from the beginning of the struct

## Defining Your Parameters

### Parameter Definition Template (`my_parameters.h`)

```c
#ifndef MY_PARAMETERS_H_
#define MY_PARAMETERS_H_

#include "datastream_default_config.h"
#include DS_USER_TYPES_DEFINITIONS

typedef struct PACKED
{
    // Device identification
    uint32_t DEVICE_ID;
    
    // Network configuration (customize as needed)
    uint32_t USES_DHCP;
    uint32_t IP_ADDR[4];
    uint32_t GATEWAY_ADDR[4];
    uint32_t NET_MASK[4];
    uint32_t MAC_ADDR[6];
    
    // Your application parameters
    float    CALIBRATION_OFFSET;
    float    CALIBRATION_SCALE;
    uint32_t OPERATING_MODE;
    
    // Required system parameters (DO NOT REMOVE)
    uint32_t PARAMETERS_SETS_IN_FLASH;           // System managed
    uint32_t PARAMETERS_INITIALIZATION_MARKER;   // Version marker
    
} ds_parameter_names_t;

DS_STATIC_ASSERT(sizeof(ds_parameter_names_t) % 4 == 0, "Parameter struct must be 4-byte aligned");

#define DS_PARAMETERS_T_SIZE   sizeof(ds_parameter_names_t)
#define DS_PARAMETER_COUNT     DS_PARAMETERS_T_SIZE / sizeof(uint32_t)

#endif
```

### Important Parameter Requirements

1. **Required system parameters:**
   - `PARAMETERS_SETS_IN_FLASH` - Managed by the system
   - `PARAMETERS_INITIALIZATION_MARKER` - Used for parameter version control

2. **All data must be 32-bit aligned**

3. **Parameters are stored in flash** - design accordingly

## Defining Your Types

### Type Definition Template (`my_types.h`)

```c
#ifndef MY_TYPES_H_
#define MY_TYPES_H_

#include <stdint.h>

// Your application-specific enums (must be 32-bit)
typedef enum
{
    MODE_STANDBY = 0,
    MODE_ACTIVE = 1,
    MODE_ERROR = 2
} operating_mode_t;

typedef enum  
{
    STATUS_OK = 0x00000000,
    STATUS_WARNING = 0x00000001,
    STATUS_ERROR = 0x00000002
} status_flags_t;

// Verify all types are 32-bit
DS_STATIC_ASSERT(sizeof(operating_mode_t) == sizeof(uint32_t), "operating_mode_t must be 32-bit");
DS_STATIC_ASSERT(sizeof(status_flags_t) == sizeof(uint32_t), "status_flags_t must be 32-bit");

#endif
```

## Integration Steps

### Step 1: Library Setup
1. Copy the datastream library to your project (without modifications)
2. Add library include and source paths to your build system

### Step 2: Create Definition Files
1. Copy the example files from `examples/` directory
2. Rename and customize them for your application
3. Place them in your project (not in the library directory)

### Step 3: Configure Build System

**For CMake:**
```cmake
# Add preprocessor defines
target_compile_definitions(your_target PRIVATE
    STM32
    DS_USER_REGISTER_DEFINITIONS="config/my_registers.h"
    DS_USER_PARAMETER_DEFINITIONS="config/my_parameters.h"
    DS_USER_TYPES_DEFINITIONS="config/my_types.h"
)
```

**For Makefile:**
```makefile
CFLAGS += -DSTM32
CFLAGS += -DDS_USER_REGISTER_DEFINITIONS=\"config/my_registers.h\"
CFLAGS += -DDS_USER_PARAMETER_DEFINITIONS=\"config/my_parameters.h\"  
CFLAGS += -DDS_USER_TYPES_DEFINITIONS=\"config/my_types.h\"
```

**For IDEs (STM32CubeIDE, etc.):**
Add to preprocessor defines in project settings:
```
STM32
DS_USER_REGISTER_DEFINITIONS="config/my_registers.h"
DS_USER_PARAMETER_DEFINITIONS="config/my_parameters.h"
DS_USER_TYPES_DEFINITIONS="config/my_types.h"
```

### Step 4: Initialize and Use
```c
#include "datastream.h"  // Will use your custom definitions

int main(void)
{
    // Initialize datastream
    dsInitialize();
    
    // Your application code
    // Access registers: REGS.byName.STATUS_FLAGS = 0x01;
    // Access parameters: PARS.byName.DEVICE_ID = 123;
    
    return 0;
}
```

## Advanced Configuration

### Custom Control Interface Types

Define custom control interfaces in your types file:
```c
// In your my_types.h
typedef enum
{
    MY_CUSTOM_INTERFACE = ds_control_USER_DEFINED_START + 1,
    MY_OTHER_INTERFACE  = ds_control_USER_DEFINED_START + 2
} my_control_interfaces_t;
```

### Backward Compatibility

If no custom definitions are provided, the library will use the default definitions in:
- `ds_register_names.h`
- `ds_parameter_names.h`  
- `apptypes.h`

This ensures existing projects continue to work without changes.

## Task Registration System (New in 2.0)

Version 2.0 introduces a runtime task registration system for control interface management. This replaces the old hardcoded task handle approach.

### Why Task Registration?

- **Dynamic**: Register tasks at runtime instead of compile-time configuration
- **Flexible**: Multiple tasks can participate in the control interface
- **Debuggable**: Task names help identify which task has control
- **Safe**: Automatic validation and permission checking

### How to Use

#### 1. Register Your Tasks

```c
void app_main(void) {
    // Initialize datastream first
    dsInitialize();

    // Register the current task
    TaskHandle_t myTask = xTaskGetCurrentTaskHandle();
    bool success = dsRegisterControlTask(myTask, ds_control_TCP, "MainTask");

    if (!success) {
        ESP_LOGE(TAG, "Failed to register task");
    }

    // Create and register network tasks
    dsTCPTaskCreate();  // TCP task registers itself internally
    dsUDPTaskCreate();  // UDP task registers itself internally
}
```

#### 2. Implement Permission Checks (Optional)

The library automatically checks permissions, but you can override `dsCheckTaskWritePermission()`:

```c
// Custom permission check (weak function - override in your app)
bool dsCheckTaskWritePermission(void) {
    TaskHandle_t current = xTaskGetCurrentTaskHandle();
    ds_control_interface_t controlType = dsGetTaskControlType(current);

    // Allow specific task types
    return (controlType == ds_control_TCP || controlType == ds_control_USB);
}
```

#### 3. Unregister When Done (Optional)

```c
void cleanup(void) {
    TaskHandle_t myTask = xTaskGetCurrentTaskHandle();
    dsUnregisterControlTask(myTask);
}
```

### Task Management API

```c
// Register a task for control interface management
bool dsRegisterControlTask(TaskHandle_t taskHandle,
                          ds_control_interface_t controlType,
                          const char* taskName);

// Unregister a task
bool dsUnregisterControlTask(TaskHandle_t taskHandle);

// Get control type for a task
ds_control_interface_t dsGetTaskControlType(TaskHandle_t taskHandle);

// Check if current task has write permission
bool dsCheckTaskWritePermission(void);
```

### Maximum Registered Tasks

By default, up to 8 tasks can be registered. Configure via:
```c
#define DS_MAX_REGISTERED_TASKS 8  // In ds_app_config.h
```

## Migration from Older Versions

### From Version 1.x to 2.0

#### Step 1: Update Configuration Files

**Old (v1.x):**
```c
// datastream_config.h (modified library file)
#define PLATFORM STM32
#define TCP_PORT 2009
```

**New (v2.0):**
```c
// ds_app_config.h (your application file)
#define DS_PLATFORM STM32
#define DS_TCP_PORT 2009
```

#### Step 2: Update Header Includes

**Old:**
```c
#include "TCPdatastream.h"
#include "UDPdatastream.h"
```

**New:**
```c
#include "dsTCP.h"
#include "dsUDP.h"
```

#### Step 3: Add Task Registration

**Old:**
```c
// No registration needed - task handles were hardcoded
dsTCPTaskCreate();
```

**New:**
```c
dsInitialize();  // Initialize first
TaskHandle_t myTask = xTaskGetCurrentTaskHandle();
dsRegisterControlTask(myTask, ds_control_TCP, "MyTask");
dsTCPTaskCreate();
```

#### Step 4: Update Custom Definitions

If you previously modified library files directly:

1. **Extract your modifications** from `ds_register_names.h`, `ds_parameter_names.h`
2. **Create external definition files** in your project (e.g., `my_registers.h`, `my_parameters.h`)
3. **Configure the library** to use your files via `ds_app_config.h`:
   ```c
   #define DS_USER_REGISTER_DEFINITIONS "my_registers.h"
   #define DS_USER_PARAMETER_DEFINITIONS "my_parameters.h"
   ```
4. **Restore clean library** (or update from repository)
5. **Test thoroughly** to ensure everything works

### Configuration Mapping

| Version 1.x | Version 2.0 | Notes |
|-------------|-------------|-------|
| `datastream_config.h` | `ds_app_config.h` | Now optional, auto-detected |
| `PLATFORM` | `DS_PLATFORM` | Defined in `ds_platforms.h` |
| `TCP_PORT` | `DS_TCP_PORT` | Configurable in `ds_app_config.h` |
| `UDP_PORT` | `DS_UDP_PORT` | Configurable in `ds_app_config.h` |
| `TCPdatastream.h` | `dsTCP.h` | Renamed and refactored |
| `UDPdatastream.h` | `dsUDP.h` | Renamed and refactored |
| Hardcoded task handles | `dsRegisterControlTask()` | Runtime registration |

## Compiler Warnings

The library will generate compiler warnings to help you understand the configuration:

### Default Definition Warnings

When using default definitions, you'll see warnings like:
```
DATASTREAM: Using default register definitions. Define DS_USER_REGISTER_DEFINITIONS to use custom registers.
DATASTREAM: Using default parameter definitions. Define DS_USER_PARAMETER_DEFINITIONS to use custom parameters.
DATASTREAM: Using default type definitions. Define DS_USER_TYPES_DEFINITIONS to use custom types.
```

If using all defaults, you'll also see:
```
DATASTREAM WARNING: Using all default definitions. For shared library usage, define custom definitions. See USER_INTEGRATION_GUIDE.md
```

### Suppressing Warnings

If you intentionally want to use default definitions, you can suppress the warnings:
```c
#define DS_SUPPRESS_DEFAULT_WARNINGS
#include "datastream.h"
```

**Note:** These are informational warnings, not errors. Your code will still compile and work correctly.

## Code Quality and Bug Fixes (Version 2.0)

Version 2.0 includes significant code quality improvements and critical bug fixes:

### Critical Bug Fixes

1. **NULL Pointer Dereference** (datastream.c:213, 222)
   - **Issue**: Passing NULL reply pointer to `dsGetRegister()` caused crashes
   - **Fix**: Now passes valid reply pointer to prevent dereferencing NULL
   - **Impact**: High - prevented system crashes during register operations

2. **Undefined Behavior in Reply Initialization**
   - **Issue**: Using `strcat()` on uninitialized memory in reply string initialization
   - **Fix**: Changed to `strcpy()` for first string, ensuring proper initialization
   - **Impact**: High - prevented memory corruption and unpredictable behavior

3. **Missing NULL Checks in Getter Functions**
   - **Issue**: `dsGetParameter()` and `dsGetRegister()` didn't handle NULL reply pointers
   - **Fix**: Added defensive NULL checks before dereferencing
   - **Impact**: Medium - improved robustness and prevented potential crashes

### Code Style Standardization

The entire codebase has been standardized for consistency and readability:

- **Allman-Style Bracing**: All control structures now use consistent brace placement
  ```c
  // Standardized format
  if (condition)
  {
      // code
  }
  ```

- **4-Space Indentation**: Converted all tab indentation to 4 spaces
- **Dead Code Removal**: Removed commented-out code blocks and unused includes
- **Typo Fixes**: Fixed documentation typos (e.g., "Hanlde" → "Handle", "permnission" → "permission")

### Enhanced Platform Support

**ESP32 Improvements:**
- Implemented `dsGetIPAddress()` using `esp_netif_get_ip_info()`
- Implemented `dsGetMACAddress()` using `esp_netif_get_mac()` with fallback to `esp_read_mac()`
- Added `esp_mac.h` include for proper MAC address API support
- Enhanced logging with ESP-IDF macros (`DS_LOGI`, `DS_LOGE`, `DS_LOGW`)

### Documentation Quality

- **Zero Doxygen Warnings**: Complete API documentation generates without warnings
- **Comprehensive Function Documentation**: All public APIs documented with @param, @return, @note
- **Usage Examples**: Code examples in @code blocks throughout documentation
- **Module Organization**: Logical grouping with @defgroup and @ingroup tags

## Troubleshooting

### Common Issues

1. **Compiler can't find custom headers:**
   - Verify file paths in defines match actual file locations
   - Check build system include paths

2. **Structure size errors:**
   - Ensure all register/parameter members are 32-bit aligned
   - Use `DS_STATIC_ASSERT` to verify sizes

3. **Linking errors:**
   - Verify you're including all required datastream source files
   - Check that register/parameter counts are correct

4. **Too many compiler warnings:**
   - Define `DS_SUPPRESS_DEFAULT_WARNINGS` to suppress default usage warnings
   - Or better: define custom definitions to eliminate the warnings

### Debugging

Enable datastream logging by defining platform-specific logging in `ds_app_config.h`:

**ESP32:**
```c
// Logging is automatically configured via ESP-IDF
// DS_LOGI, DS_LOGE, DS_LOGW use ESP_LOG functions
```

**Other Platforms:**
```c
// Define custom logging macros in ds_app_config.h
#define DS_LOGI(tag, format, ...) printf("[INFO] " format "\n", ##__VA_ARGS__)
#define DS_LOGE(tag, format, ...) printf("[ERROR] " format "\n", ##__VA_ARGS__)
#define DS_LOGW(tag, format, ...) printf("[WARN] " format "\n", ##__VA_ARGS__)
```

### Configuration Detection

1. **Verify ds_app_config.h is Being Used:**
   - **Symptom**: Not sure if custom configuration is being applied
   - **Solution**: Enable configuration info messages
   - **How**:
     ```c
     // Add to your build system or at the top of ds_app_config.h
     #define DS_SHOW_CONFIG_INFO
     ```
   - **Output**: You'll see a message during compilation:
     ```
     DATASTREAM INFO: ds_app_config.h detected and included
     ```
     or
     ```
     DATASTREAM INFO: ds_app_config.h not found, using defaults
     ```

2. **Force Include or Exclude ds_app_config.h:**
   - **To force include** (for compilers without `__has_include` support):
     ```c
     #define DS_USE_APP_CONFIG
     ```
   - **To exclude** (use only defaults or build system defines):
     ```c
     #define DS_NO_APP_CONFIG
     ```

### New Issues in Version 2.0

1. **Task Registration Required:**
   - **Symptom**: Control interface requests fail, permission errors
   - **Solution**: Register tasks with `dsRegisterControlTask()` before use
   - **Example**:
     ```c
     dsInitialize();
     TaskHandle_t task = xTaskGetCurrentTaskHandle();
     dsRegisterControlTask(task, ds_control_TCP, "MyTask");
     ```

2. **Header Include Order:**
   - **Symptom**: Compiler errors about undefined types
   - **Solution**: Include `datastream.h` before `dsTCP.h` or `dsUDP.h`
   - **Correct Order**:
     ```c
     #include "datastream.h"  // Core definitions first
     #include "dsTCP.h"       // Then network interfaces
     #include "dsUDP.h"
     ```

3. **Auto-Detection Not Working:**
   - **Symptom**: Discovery packets not responding
   - **Solution**: Ensure `DS_AUTO_DETECTION_ENABLE` is set to 1
   - **Check**: Verify UDP task is created with `dsUDPTaskCreate()`
   - **Verify**: Board identification is properly configured in `ds_app_config.h`

## Support and Resources

### Helper Scripts

The `tools/` directory provides utilities for testing and debugging:

| Script | Purpose |
|--------|---------|
| `mock_esp32_discovery.py` | Simulates an ESP32 responding to discovery requests |
| `test_udp_receive.py` | Listens for and displays incoming UDP packets |

Run with Python 3:
```bash
python3 tools/mock_esp32_discovery.py   # Simulate a board
python3 tools/test_udp_receive.py       # Monitor UDP traffic
```

### Documentation

- **API Reference**: Generate with `doxygen Doxyfile`, then open `docs/html/index.html`
- **README.md**: Overview, quick start, and examples
- **USER_INTEGRATION_GUIDE.md**: This file - integration and migration guide

### Getting Help

For issues specific to the datastream library integration:

1. **Check Configuration**:
   - Verify `ds_app_config.h` is in your include path
   - Confirm `DS_PLATFORM` is correctly defined
   - Ensure custom definition files (if used) follow templates exactly

2. **Verify Requirements**:
   - All required registers/parameters are present
   - Proper 32-bit alignment of all data structures
   - Task registration is performed before network task creation

3. **Test Incrementally**:
   - Start with minimal configuration (no custom definitions)
   - Test with default registers/parameters first
   - Add customizations one at a time
   - Use provided examples as reference

4. **Check Logs**:
   - Enable logging macros for your platform
   - Look for initialization messages
   - Check for task registration confirmations
   - Monitor control interface state changes

### Best Practices

1. **Always initialize first**: Call `dsInitialize()` before any other datastream functions
2. **Register tasks early**: Register tasks immediately after initialization
3. **Keep library clean**: Never modify library files; use external configuration
4. **Version control**: Track your `ds_app_config.h` and custom definitions in your repository

---

This integration method ensures your project remains maintainable and the datastream library can be updated independently of your application-specific definitions. Version 2.0 provides significant improvements in code quality, safety, and developer experience.