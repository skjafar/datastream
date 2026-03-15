# Datastream Library — User Integration Guide

Version 2.0 | Last Updated: 2026-03-15

This guide explains how to integrate the datastream library into your project without modifying the library files, taking advantage of the external configuration system.

---

## Table of Contents

1. [Quick Start (5 Minutes)](#quick-start-5-minutes)
2. [Adding to an ESP-IDF Project](#adding-to-an-esp-idf-project)
3. [Adding to an STM32CubeIDE Project](#adding-to-an-stm32cubeside-project)
4. [Testing Your Setup](#testing-your-setup)
5. [Overview](#overview)
6. [What's New in Version 2.0](#whats-new-in-version-20)
7. [Configuration Methods](#configuration-methods)
8. [Defining Registers and Parameters](#defining-your-registers)
9. [System Registers](#system-registers)
10. [Task Registration System](#task-registration-system)
11. [Implementing Flash Storage](#implementing-flash-storage)
12. [Implementing Custom System Commands](#implementing-custom-system-commands)
13. [Troubleshooting](#troubleshooting)

---

## Quick Start (5 Minutes)

### Step 1: Add Library to Your Project

See [Adding to an ESP-IDF Project](#adding-to-an-esp-idf-project) or [Adding to an STM32CubeIDE Project](#adding-to-an-stm32cubeside-project) for platform-specific instructions.

For CMake-based projects (RP2040, RP2350, generic):

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

```cmake
add_subdirectory(lib/datastream)
target_link_libraries(${PROJECT_NAME} PRIVATE datastream)
target_include_directories(${PROJECT_NAME} PRIVATE include)
```

### Step 2: Create Configuration File

Create `ds_app_config.h` in your include path:

```c
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

// Required: select your platform
#define DS_PLATFORM ESP32   // ESP32, STM32, RP2040, RP2350, GENERIC_PLATFORM

// Board identification
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

void app_main(void)
{
    dsInitialize();
    dsTCPTaskCreate();   // TCP server on port 2009
    dsUDPTaskCreate();   // UDP server on port 2011 (with auto-detection)
    vTaskStartScheduler();
}
```

### Step 4: Build and Flash

Once flashed, the device will:
- Listen for TCP connections on port 2009
- Listen for UDP packets on port 2011
- Respond to auto-detection broadcast requests

---

## Adding to an ESP-IDF Project

### Step 1: Copy Library as a Component

```
your_esp_project/
├── main/
│   ├── main.c
│   ├── ds_app_config.h       <-- your config
│   └── CMakeLists.txt
└── components/
    └── datastream/
        ├── CMakeLists.txt    <-- create this
        ├── include/
        └── src/
```

### Step 2: Create the Component CMakeLists.txt

```cmake
# components/datastream/CMakeLists.txt
idf_component_register(
    SRCS
        "src/datastream.c"
        "src/dsRegisters.c"
        "src/dsParameters.c"
        "src/dsTCP.c"
        "src/dsUDP.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        freertos
        esp_netif
        lwip
)
```

If you enable `DS_CLI_ENABLE 1`, also add `"src/dsCLI.c"` to SRCS.

### Step 3: Make ds_app_config.h Visible

The library uses `__has_include` to auto-detect `ds_app_config.h`. Place it in `main/` and ensure `main/` is in the component's `INCLUDE_DIRS`, or add it to your main component:

```cmake
# main/CMakeLists.txt
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."        # This makes ds_app_config.h findable
    REQUIRES datastream
)
```

### Step 4: Configure for ESP32

In `ds_app_config.h`:

```c
#define DS_PLATFORM ESP32

// Optional: enable ESP-IDF logging
#define ESP32_LOGGING_ENABLE 1

// Point to your register/parameter files
#define DS_USER_REGISTER_DEFINITIONS  "my_registers.h"
#define DS_USER_PARAMETER_DEFINITIONS "my_parameters.h"
```

### Step 5: Initialize After Network Stack

```c
#include "datastream.h"
#include "dsTCP.h"
#include "dsUDP.h"
#include "esp_wifi.h"
#include "esp_event.h"

void app_main(void)
{
    // Initialize WiFi/Ethernet and wait for IP before calling dsInitialize()
    wifi_init_and_connect();
    wait_for_ip();

    dsInitialize();
    dsTCPTaskCreate();
    dsUDPTaskCreate();
}
```

### Step 6: Implement Flash Storage (NVS Example)

```c
#include "nvs_flash.h"
#include "nvs.h"
#include "dsParameters.h"

void dsWriteParametersToFlash(ds_parameters_t *parList)
{
    nvs_handle_t handle;
    if (nvs_open("datastream", NVS_READWRITE, &handle) == ESP_OK)
    {
        nvs_set_blob(handle, "params", parList, sizeof(ds_parameters_t));
        nvs_commit(handle);
        nvs_close(handle);
    }
}

void dsReadParametersFromFlash(ds_parameters_t *parList)
{
    nvs_handle_t handle;
    size_t size = sizeof(ds_parameters_t);
    if (nvs_open("datastream", NVS_READONLY, &handle) == ESP_OK)
    {
        nvs_get_blob(handle, "params", parList, &size);
        nvs_close(handle);
    }
}
```

---

## Adding to an STM32CubeIDE Project

### Step 1: Copy Library Files

Copy the `datastream/` folder to `Middlewares/Third_Party/` in your project:

```
your_project/
├── Core/
│   ├── Inc/
│   │   └── ds_app_config.h    <-- create this
│   └── Src/
└── Middlewares/
    └── Third_Party/
        └── datastream/
            ├── include/
            └── src/
```

In STM32CubeIDE, right-click the project → **Refresh** to detect the new files.

### Step 2: Add Include Path

Open **Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include Paths**.

Add the following path and click **OK**:
```
../Middlewares/Third_Party/datastream/include
```

Do this for **both** Debug and Release configurations.

### Step 3: Verify Source Files Are in the Build

In the Project Explorer, expand `Middlewares/Third_Party/datastream/src/`. If any `.c` file has a small badge indicating it is excluded from the build, right-click it → **Resource Configurations → Exclude from build** → uncheck both Debug and Release.

### Step 4: Create Your Configuration File

Create `Core/Inc/ds_app_config.h`:

```c
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

#define DS_PLATFORM             STM32
#define DS_BOARD_NAME           "My STM32 Board"
#define DS_BOARD_ID             0x00000001
#define DS_AUTO_DETECTION_ENABLE 1
#define DS_STATS_ENABLE         1
#define CRC8_ENABLE             0

#define DS_USER_REGISTER_DEFINITIONS  "my_registers.h"
#define DS_USER_PARAMETER_DEFINITIONS "my_parameters.h"

#endif
```

The `Core/Inc/` directory is already in the default include path of CubeMX-generated projects, so `ds_app_config.h` will be auto-detected by the library.

### Step 5: Initialize in main.c

```c
#include "datastream.h"
#include "dsTCP.h"
#include "dsUDP.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ETH_Init();      // or whatever your network peripheral is
    // ... other CubeMX init calls ...

    dsInitialize();
    dsTCPTaskCreate();
    dsUDPTaskCreate();

    vTaskStartScheduler();
    while (1) {}
}
```

### Step 6: Implement Flash Storage

```c
#include "dsParameters.h"

// Example using a dedicated flash sector at FLASH_USER_START_ADDR
void dsWriteParametersToFlash(ds_parameters_t *parList)
{
    FLASH_EraseInitTypeDef eraseInit = { /* configure your sector */ };
    uint32_t pageError;

    HAL_FLASH_Unlock();
    HAL_FLASHEx_Erase(&eraseInit, &pageError);

    uint64_t *src = (uint64_t *)parList;
    uint32_t addr = FLASH_USER_START_ADDR;
    for (size_t i = 0; i < sizeof(ds_parameters_t) / 8; i++, addr += 8)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, src[i]);
    }

    HAL_FLASH_Lock();
}

void dsReadParametersFromFlash(ds_parameters_t *parList)
{
    memcpy(parList, (void *)FLASH_USER_START_ADDR, sizeof(ds_parameters_t));
}
```

### FreeRTOS+TCP Requirement

The STM32 platform uses FreeRTOS+TCP. Ensure it is enabled and configured in your project (via STM32CubeMX or manually). The library expects `FreeRTOS_IP.h` and `FreeRTOS_Sockets.h` to be in your include path.

---

## Testing Your Setup

### Prerequisites

- Python 3.6 or later
- Network access to your device (or use the mock server for offline testing)

### Option A: Test Without Hardware (Mock Server)

```bash
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

### Option B: Test With Hardware

```bash
cd tools
python3 test_udp_receive.py
```

Then send a discovery broadcast. You should see:
```
Received 44 bytes from 192.168.1.50:2011
Data: efbeadde020100...
```

### Option C: Quick Discovery Test

```python
import socket, struct

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.settimeout(2.0)

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

```python
import socket, struct

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.1.50', 2009))  # replace with your device IP

# READ_PARAMETER (type=3), address 0
packet = struct.pack('<BB I', 3, 0, 0)
sock.send(packet)

response = sock.recv(6)
status, addr, value = struct.unpack('<bB I', response)
print(f"Parameter 0 = {value}  (status: {status})")

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

The datastream library 2.0 introduces an external configuration system that allows you to:

- **Use the library without modifying any library files** — all customisation via external headers
- **Define application-specific registers and parameters** — custom data structures for your needs
- **Maintain clean separation** — library code remains pristine and upgradeable
- **Access system state via system registers** — stats, control interface, and counters managed by the library
- **Automatic platform detection** — smart defaults based on your target platform
- **Runtime task registration** — dynamic control interface management

---

## What's New in Version 2.0

### Major Changes
1. **External Configuration System** — new `ds_app_config.h` auto-detection
2. **Separate System Registers** — library-owned registers (`SYS_REGS`) accessed via `READ_SYSTEM_REGISTER` / `WRITE_SYSTEM_REGISTER`; user register structs no longer include them
3. **System Commands at High Values** — built-in commands at 200+, user commands start from 0 with no offset needed
4. **Modular Network Interfaces** — separated TCP (`dsTCP.h/c`) and UDP (`dsUDP.h/c`)
5. **Task Registration System** — runtime control interface management
6. **Platform Abstraction** — centralised platform definitions (`ds_platforms.h`)
7. **Bug Fixes** — NULL pointer safety, initialisation fixes, style standardisation
8. **Enhanced ESP32 Support** — full IP/MAC address retrieval
9. **Comprehensive Documentation** — complete Doxygen API documentation with examples

---

## Configuration Methods

### Method 1: Automatic Configuration (Recommended)

Create `ds_app_config.h` in your project's include path. The library detects it automatically via `__has_include`:

```c
// ds_app_config.h
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

#define DS_PLATFORM ESP32

// Optional: custom register and parameter definitions
#define DS_USER_REGISTER_DEFINITIONS  "config/my_registers.h"
#define DS_USER_PARAMETER_DEFINITIONS "config/my_parameters.h"

// Optional: board identification
#define DS_BOARD_TYPE 1
#define DS_BOARD_NAME "My Custom Board"
#define DS_BOARD_ID   0x12345678

// Optional: network configuration
#define DS_TCP_PORT 2009
#define DS_UDP_PORT 2011

// Optional: feature flags
#define DS_AUTO_DETECTION_ENABLE 1
#define DS_STATS_ENABLE 1
#define CRC8_ENABLE 0

#endif
```

### Method 2: Build System Configuration

```cmake
# CMake
target_compile_definitions(your_target PRIVATE
    DS_PLATFORM=ESP32
    DS_USER_REGISTER_DEFINITIONS="config/my_registers.h"
    DS_USER_PARAMETER_DEFINITIONS="config/my_parameters.h"
    DS_BOARD_NAME="My Board"
)
```

```makefile
# Makefile
CFLAGS += -DDS_PLATFORM=ESP32
CFLAGS += -DDS_USER_REGISTER_DEFINITIONS=\"config/my_registers.h\"
CFLAGS += -DDS_BOARD_NAME=\"My\ Board\"
```

### Method 3: Direct Include (Legacy)

```c
#define DS_PLATFORM STM32
#define DS_USER_REGISTER_DEFINITIONS "my_registers.h"
#include "datastream.h"
```

---

## Defining Your Registers

### What Are User Registers?

User registers hold your application's real-time data: sensor readings, status flags, control outputs. They are accessed by the client via `READ_REGISTER` / `WRITE_REGISTER` packet types (types 1 and 2).

System registers (packet counts, control interface type, 1 Hz counter) are managed by the library in a separate `SYS_REGS` struct and are **not part of the user register struct**. They are accessed via `READ_SYSTEM_REGISTER` / `WRITE_SYSTEM_REGISTER` (types 6 and 7).

### Register Definition Template (`my_registers.h`)

```c
#ifndef MY_REGISTERS_H_
#define MY_REGISTERS_H_

#include "ds_default_config.h"

typedef struct PACKED
{
    /***** read-only registers (must come first) *****/
    uint32_t    STATUS_FLAGS;
    float       SENSOR_READING;
    uint32_t    SYSTEM_UPTIME;

    /***** read/write registers *****/
    uint32_t    CONTROL_MODE;
    uint32_t    OUTPUT_ENABLE;
    float       SETPOINT;

} ds_register_names_t;

DS_STATIC_ASSERT(sizeof(ds_register_names_t) % 4 == 0, "Register struct must be 4-byte aligned");

#define DS_REGISTERS_BY_NAME_T_SIZE     sizeof(ds_register_names_t)
#define DS_REGISTER_COUNT               (DS_REGISTERS_BY_NAME_T_SIZE / sizeof(uint32_t))
#define DS_REGISTERS_READ_ONLY_COUNT    3   // adjust to match read-only fields above

#endif
```

### Register Requirements

1. **All fields must be 32-bit**: use `uint32_t`, `int32_t`, `float`, or a custom 32-bit enum
2. **Read-only registers must come first** in the struct; set `DS_REGISTERS_READ_ONLY_COUNT` accordingly
3. **Do not include system registers** — they are in `SYS_REGS`, not here
4. **At least one field is required** to avoid a zero-size struct (the minimal placeholder file satisfies this)

### Accessing Registers from Your Application

```c
// Write from your application (directly, no permission check needed)
REGS.byName.STATUS_FLAGS = 0x01;
REGS.byName.SENSOR_READING = 3.14f;

// Read from your application
uint32_t mode = REGS.byName.CONTROL_MODE;
```

---

## Defining Your Parameters

### What Are Parameters?

Parameters are persistent configuration values stored in flash memory. They survive power cycles. They are accessed by the client via `READ_PARAMETER` / `WRITE_PARAMETER` (types 3 and 4).

### Parameter Definition Template (`my_parameters.h`)

```c
#ifndef MY_PARAMETERS_H_
#define MY_PARAMETERS_H_

#include "ds_default_config.h"

typedef struct PACKED
{
    // Device identification
    uint32_t DEVICE_ID;

    // Network configuration
    uint32_t USES_DHCP;
    uint32_t IP_ADDR[4];
    uint32_t GATEWAY_ADDR[4];
    uint32_t NET_MASK[4];
    uint32_t MAC_ADDR[6];

    // Application parameters
    float    CALIBRATION_OFFSET;
    float    CALIBRATION_SCALE;
    uint32_t OPERATING_MODE;
    uint32_t ALARM_THRESHOLD;

    // Optional: bookkeeping fields useful in your flash read/write implementation
    uint32_t PARAMETERS_SETS_IN_FLASH;
    uint32_t PARAMETERS_INITIALIZATION_MARKER;

} ds_parameter_names_t;

DS_STATIC_ASSERT(sizeof(ds_parameter_names_t) % 4 == 0, "Parameter struct must be 4-byte aligned");

#define DS_PARAMETERS_T_SIZE   sizeof(ds_parameter_names_t)
#define DS_PARAMETER_COUNT     (DS_PARAMETERS_T_SIZE / sizeof(uint32_t))

#endif
```

### Parameter Requirements

1. **All fields must be 32-bit aligned**
2. **Parameters are stored in flash** — the library calls `dsWriteParametersToFlash()` / `dsReadParametersFromFlash()` which you must implement (see [Implementing Flash Storage](#implementing-flash-storage))
3. `PARAMETERS_SETS_IN_FLASH` and `PARAMETERS_INITIALIZATION_MARKER` are optional but recommended for managing flash version control in your own flash implementation

---

## System Registers

System registers are library-owned and stored in `SYS_REGS`. They are separate from user registers and are accessed via packet types 6 and 7.

| Register | Type | Access | Description |
|----------|------|--------|-------------|
| `DS_PACKET_COUNT` | `uint32_t` | Read-only | Total packets received (if `DS_STATS_ENABLE`) |
| `DS_ERROR_COUNT` | `uint32_t` | Read-only | Total errors (if `DS_STATS_ENABLE`) |
| `CONTROL_INTERFACE` | `uint32_t` | Read-only | Current control interface type |
| `COUNTER_1HZ` | `uint32_t` | Read/write | 1 Hz counter, updated by your application |

### Accessing System Registers from Application Code

```c
// Update the 1 Hz counter from your timer task
SYS_REGS.byName.COUNTER_1HZ++;

// Read stats
uint32_t total_packets = SYS_REGS.byName.DS_PACKET_COUNT;
```

### Customising System Register Initialisation

Override the weak function if you need custom initial values:

```c
void dsInitializeSystemRegisters(ds_system_registers_t *sysRegList)
{
    sysRegList->byName.COUNTER_1HZ = 0;
    // DS_PACKET_COUNT and DS_ERROR_COUNT are zeroed automatically
}
```

---

## Task Registration System

Version 2.0 introduces a runtime task registration system for control interface management. Only registered tasks can be granted exclusive write access.

### Why Task Registration?

- **Dynamic**: register tasks at runtime instead of compile-time configuration
- **Flexible**: multiple tasks can participate in the control interface
- **Debuggable**: task names help identify which task holds control
- **Safe**: automatic validation and permission checking

### How to Use

Network tasks (`dsTCP`, `dsUDP`) register themselves automatically. If your application task also needs write access, register it manually:

```c
void app_main(void)
{
    dsInitialize();

    TaskHandle_t myTask = xTaskGetCurrentTaskHandle();
    bool ok = dsRegisterControlTask(myTask, ds_control_TCP_DATASTREAM, "AppTask");

    dsTCPTaskCreate();
    dsUDPTaskCreate();
}
```

### Custom Permission Check (Optional)

```c
// Override this weak function to customise who can write
bool dsCheckTaskWritePermission(void)
{
    TaskHandle_t current = xTaskGetCurrentTaskHandle();
    ds_control_interface_t type = dsGetTaskControlType(current);
    return (type == ds_control_TCP_DATASTREAM || type == ds_control_USB);
}
```

### Maximum Registered Tasks

Configure via `ds_app_config.h`:
```c
#define DS_MAX_REGISTERED_TASKS 8
```

---

## Implementing Flash Storage

The library provides two weak functions you **must** override for parameter persistence. Without overriding them, parameters are lost on power cycle.

```c
// Called by WRITE_FLASH system command and dsInitialize() → dsSetParametersDefaults()
void dsWriteParametersToFlash(ds_parameters_t *parList);

// Called during dsInitialize() to load parameters from flash
void dsReadParametersFromFlash(ds_parameters_t *parList);
```

Similarly, override the board name flash functions if you use `dsSetBoardName()` with persistence:

```c
void dsWriteBoardNameToFlash(const char *name);
void dsReadBoardNameFromFlash(char *name, size_t maxLen);
```

---

## Implementing Custom System Commands

Built-in system commands occupy values 200–202. Values 0–199 are available for user-defined commands. Simply define a normal C enum (starting from 0 is safe — no offset needed):

```c
typedef enum {
    cmd_START    = 0,
    cmd_STOP     = 1,
    cmd_TRIGGER  = 2,
    cmd_CALIBRATE = 3,
} my_sys_commands_t;
```

Handle them by implementing the weak function:

```c
bool dsProcessUserSysCommand(const dsRxPacket *inputPacket, dsTxPacket *outputPacket)
{
    switch (inputPacket->contents.value)
    {
        case cmd_START:
            start_system();
            outputPacket->contents.status = SYS_COMMAND_OK_REPLY_VAL;
            return true;

        case cmd_STOP:
            stop_system();
            outputPacket->contents.status = SYS_COMMAND_OK_REPLY_VAL;
            return true;

        default:
            return false;  // unknown command → library returns error
    }
}
```

---

## Defining Your Types

If you need application-specific enums in your register or parameter structs, define them in a separate file:

```c
#ifndef MY_TYPES_H_
#define MY_TYPES_H_

#include <stdint.h>

typedef enum {
    MODE_STANDBY = 0,
    MODE_ACTIVE  = 1,
    MODE_ERROR   = 2
} operating_mode_t;

DS_STATIC_ASSERT(sizeof(operating_mode_t) == sizeof(uint32_t), "operating_mode_t must be 32-bit");

#endif
```

Point to it from `ds_app_config.h`:
```c
#define DS_USER_TYPES_DEFINITIONS "my_types.h"
```

---

## Compiler Warnings

### Default Definition Warnings

When no custom definitions are configured, you may see:
```
DATASTREAM: Using default register definitions. Define DS_USER_REGISTER_DEFINITIONS to use custom registers.
DATASTREAM: Using default parameter definitions. Define DS_USER_PARAMETER_DEFINITIONS to use custom parameters.
```

### Suppressing Warnings

If you intentionally use the minimal defaults, suppress with:
```c
#define DS_SUPPRESS_DEFAULT_WARNINGS
```

---

## Troubleshooting

### Common Issues

1. **Compiler can't find custom headers**
   - Verify file paths in `DS_USER_REGISTER_DEFINITIONS` / `DS_USER_PARAMETER_DEFINITIONS` match actual file locations relative to the build include path
   - Check that the directory is in your build system's include paths

2. **Structure size assertion failure**
   - Ensure all register/parameter members are 32-bit (`uint32_t`, `int32_t`, `float`)
   - Use `DS_STATIC_ASSERT` to catch misalignment at compile time

3. **Linking errors**
   - Verify all required `.c` files are included in the build
   - Check that `DS_REGISTER_COUNT` and `DS_PARAMETER_COUNT` are correctly defined

4. **Compiler warnings about default definitions**
   - Define `DS_SUPPRESS_DEFAULT_WARNINGS` to suppress, or provide custom definition files

5. **STM32CubeIDE: false `uint32_t is not a type name` errors in headers**
   - These are IntelliSense/indexer artefacts, not real compiler errors — the build will succeed

### Configuration Detection

**Verify ds_app_config.h is Being Used:**
```c
// Add to ds_app_config.h
#define DS_SHOW_CONFIG_INFO
```

You will see during compilation:
```
DATASTREAM INFO: ds_app_config.h detected and included
```
or:
```
DATASTREAM INFO: ds_app_config.h not found, using defaults
```

**Force or Exclude ds_app_config.h:**
```c
#define DS_USE_APP_CONFIG   // force include (for compilers without __has_include)
#define DS_NO_APP_CONFIG    // exclude (use only build-system defines)
```

### New Issues in Version 2.0

1. **Control interface requests fail with permission error**
   - Ensure the relevant task is registered via `dsRegisterControlTask()` before issuing a control command
   - Network tasks register themselves automatically — this mainly applies to custom application tasks

2. **Header include order errors**
   - Include `datastream.h` before `dsTCP.h` or `dsUDP.h`

3. **Auto-detection not responding**
   - Ensure `DS_AUTO_DETECTION_ENABLE 1` is set
   - Verify `dsUDPTaskCreate()` was called
   - Confirm board identification is configured in `ds_app_config.h`

---

## Best Practices

1. **Always initialize first**: call `dsInitialize()` before any other datastream function
2. **Keep library clean**: never modify library files — use external configuration
3. **Override flash functions**: persistence does not work without `dsWriteParametersToFlash()` / `dsReadParametersFromFlash()`
4. **Version-control your config files**: `ds_app_config.h`, register definitions, and parameter definitions should all live in your project repository, not in the library
5. **Use the examples as a starting point**: copy from `examples/` and customise — do not edit them in place

---

## Support and Resources

### Helper Scripts

| Script | Purpose |
|--------|---------|
| `tools/mock_esp32_discovery.py` | Simulates an ESP32 board responding to discovery requests |
| `tools/test_udp_receive.py` | Listens for and displays incoming UDP packets |
| `tools/test_discovery.py` | Discovery client for testing board discovery |

```bash
python3 tools/mock_esp32_discovery.py
python3 tools/test_udp_receive.py
```

### Documentation

- **API Reference**: generate with `doxygen Doxyfile`, then open `docs/html/index.html`
- **README.md**: protocol overview, quick start, client examples, and memory reference
- **USER_INTEGRATION_GUIDE.md**: this file — integration, platform setup, and migration

### Logging

**ESP32:**
```c
// Define in ds_app_config.h to enable
#define ESP32_LOGGING_ENABLE 1
// DS_LOGI / DS_LOGE are then routed to esp_log
```

**Other Platforms:**
```c
// Define custom logging macros in ds_app_config.h
#define DS_LOGI(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define DS_LOGE(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
```
