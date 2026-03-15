# Datastream v0.2.1

A cross-platform, lightweight communication protocol library for embedded systems, providing TCP and UDP interfaces for real-time parameter and register management.

## Quick Getting Started

Get up and running in under 5 minutes:

### 1. Add the Library to Your Project

#### CMake (RP2040 / RP2350 / Generic)

```cmake
# CMakeLists.txt
add_subdirectory(datastream)
target_link_libraries(your_target PRIVATE datastream)
```

#### ESP-IDF (ESP32)

Copy the library into your project's `components/` directory, then add a `CMakeLists.txt` inside the library folder:

```
your_esp_project/
└── components/
    └── datastream/
        ├── CMakeLists.txt   <-- create this file
        ├── include/
        └── src/
```

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

See [Adding the Library to an ESP-IDF Project](#adding-the-library-to-an-esp-idf-project) for full details.

#### STM32CubeIDE (STM32)

Copy the library into `Middlewares/Third_Party/datastream/` in your project, then add the include path and source files through the IDE settings.

See [Adding the Library to an STM32CubeIDE Project](#adding-the-library-to-an-stm32cubeside-project) for full details.

### 2. Create Your Configuration File

Create `ds_app_config.h` in your project's include path:

```c
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

#define DS_PLATFORM ESP32          // ESP32, STM32, RP2040, RP2350, or GENERIC_PLATFORM
#define DS_BOARD_NAME "My Board"
#define DS_TCP_PORT 2009
#define DS_UDP_PORT 2011

#endif
```

### 3. Initialize in Your Application

```c
#include "datastream.h"
#include "dsTCP.h"
#include "dsUDP.h"

void app_main(void) {
    dsInitialize();
    dsTCPTaskCreate();
    dsUDPTaskCreate();
    vTaskStartScheduler();
}
```

### 4. Test Your Setup

Use the provided helper scripts to verify communication:

```bash
# Terminal 1: Start the mock discovery server (for testing without hardware)
python3 tools/mock_esp32_discovery.py

# Terminal 2: Listen for UDP responses
python3 tools/test_udp_receive.py
```

See the [Helper Scripts](#helper-scripts) section for details on testing tools.

For complete configuration options and custom register/parameter definitions, see [USER_INTEGRATION_GUIDE.md](USER_INTEGRATION_GUIDE.md).

---

## Overview

Datastream is a protocol library designed for embedded systems that need to communicate with external clients over network interfaces. It provides a standardized way to read/write parameters and registers, execute system commands, and manage device control remotely.

### Key Features

- **Multi-Platform Support**: ESP32, STM32, RP2040, RP2350, and generic platforms
- **Network Interfaces**: TCP and UDP socket support
- **Parameter Management**: Persistent configuration storage with flash memory support
- **Register Access**: Real-time system state monitoring and control
- **System Registers**: Library-managed registers (stats, control interface, counters) accessed via dedicated commands
- **System Commands**: Device control (flash read/write, firmware reset, user-defined)
- **Control Interface Management**: Exclusive write access control
- **Board Auto-Detection**: Automatic discovery of boards on the network
- **FreeRTOS Integration**: Task-based architecture with configurable priorities
- **Optional CRC8 Protection**: Data integrity verification for unreliable connections
- **Statistics Tracking**: Packet counting and error monitoring

## Protocol Specification

### Packet Structure

#### Input Packet (6–7 bytes)
```
+------+------+------+------+------+------+------+
| Type | Addr |      Value (32-bit)     | CRC* |
+------+------+------+------+------+------+------+
  1B     1B              4B               1B*
```

#### Output Packet (6–7 bytes)
```
+--------+------+------+------+------+------+------+
| Status | Addr |      Value (32-bit)     | CRC* |
+--------+------+------+------+------+------+------+
   1B      1B              4B               1B*
```

*CRC field is optional (configurable via `CRC8_ENABLE`)

### Command Types

| Type | Constant | Description |
|------|----------|-------------|
| 0 | `ds_type_SYS_COMMAND` | Execute a system command |
| 1 | `ds_type_READ_REGISTER` | Read a user register value |
| 2 | `ds_type_WRITE_REGISTER` | Write a user register value |
| 3 | `ds_type_READ_PARAMETER` | Read a configuration parameter |
| 4 | `ds_type_WRITE_PARAMETER` | Write a configuration parameter |
| 5 | `ds_type_CONTROL_INTERFACE` | Request exclusive write access |
| 6 | `ds_type_READ_SYSTEM_REGISTER` | Read a library system register |
| 7 | `ds_type_WRITE_SYSTEM_REGISTER` | Write a library system register |
| 100+ | `ds_type_USER_DEFINED_START` | User-defined command types |

### System Commands

The library reserves command values 200 and above for built-in system operations:

| Value | Constant | Description |
|-------|----------|-------------|
| 200 | `ds_sys_command_READ_FLASH` | Read parameters from flash into RAM |
| 201 | `ds_sys_command_WRITE_FLASH` | Write parameters from RAM to flash |
| 202 | `ds_sys_command_RESET_FIRMWARE` | Perform a firmware reset |

User-defined system commands can use any value from 0 upwards. Since built-in commands start at 200, simply defining a normal C enum starting from zero is safe — values will never collide.

```c
// Your own system commands — no offset needed
typedef enum {
    cmd_START = 0,
    cmd_STOP  = 1,
    cmd_TRIGGER = 2,
} my_sys_commands_t;
```

Handle them by implementing the weak function:

```c
bool dsProcessUserSysCommand(const dsRxPacket *inputPacket, dsTxPacket *outputPacket)
{
    switch (inputPacket->contents.value)
    {
        case cmd_START:
            // ...
            outputPacket->contents.status = SYS_COMMAND_OK_REPLY_VAL;
            return true;

        default:
            return false;  // let library return error
    }
}
```

### Response Codes

#### Success Codes
| Value | Meaning |
|-------|---------|
| `0` | System command OK |
| `1` | Read register OK |
| `2` | Write register OK |
| `3` | Read parameter OK |
| `4` | Write parameter OK |
| `5` | Control interface OK |

#### Error Codes
| Value | Meaning |
|-------|---------|
| `-1` | System command error |
| `-2` | Invalid input type |
| `-3` | Packet size error |
| `-4` | Address out of range |
| `-5` | Permission denied (read-only or no control) |
| `-6` | Control interface error |
| `-7` | Syntax error |
| `-8` | Parameters error |
| `-9` | CRC error |

## Configuration

Datastream 2.0 introduces an external configuration system that allows you to customize the library without modifying any library files. This makes the library distributable while maintaining full customization capabilities.

### Quick Start Configuration

The simplest way to configure datastream is to create a `ds_app_config.h` file in your project. The library automatically detects and includes this file using the `__has_include` preprocessor feature — if the file doesn't exist, the library uses sensible defaults.

```c
// ds_app_config.h — place in your project root or include path
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

// Platform selection
#define DS_PLATFORM STM32

// Custom register and parameter definitions (optional)
#define DS_USER_REGISTER_DEFINITIONS "my_registers.h"
#define DS_USER_PARAMETER_DEFINITIONS "my_parameters.h"

// Board identification for auto-detection
#define DS_BOARD_TYPE BOARD_TYPE_CUSTOM_1
#define DS_BOARD_NAME "My Custom Board"
#define DS_BOARD_ID 42

// Optional: suppress compiler warnings about using defaults
#define DS_SUPPRESS_DEFAULT_WARNINGS

#endif
```

### Configuration Methods

#### Method 1: Automatic Detection (Recommended)
1. Create `ds_app_config.h` in your project
2. Include `datastream.h` in your application
3. The library automatically includes your configuration

#### Method 2: Build System Configuration
```cmake
# CMake
target_compile_definitions(your_target PRIVATE
    DS_PLATFORM=STM32
    DS_USER_REGISTER_DEFINITIONS="config/registers.h"
    DS_USER_PARAMETER_DEFINITIONS="config/parameters.h"
)
```

#### Method 3: Direct Header Configuration
```c
// In your main application file
#define DS_PLATFORM STM32
#define DS_USER_REGISTER_DEFINITIONS "my_registers.h"
#include "datastream.h"
```

### Platform Support

Available platforms (defined in `ds_platforms.h`):

| Platform | Value | Description |
|----------|-------|-------------|
| `STM32` | 4 | STM32 microcontrollers with FreeRTOS+TCP |
| `ESP32` | 1 | ESP32 with ESP-IDF FreeRTOS and native sockets |
| `RP2040` | 2 | Raspberry Pi Pico with FreeRTOS+TCP |
| `RP2350` | 3 | Raspberry Pi Pico 2 with FreeRTOS+TCP |
| `GENERIC_PLATFORM` | 99 | Generic platform support |

### Network Configuration

#### TCP Interface
```c
#define DS_TCP_PORT             2009
#define DS_TCP_TASK_PRIORITY    tskIDLE_PRIORITY + 6
#define DS_TCP_TASK_STACK_SIZE  2 * configMINIMAL_STACK_SIZE
```

#### UDP Interface
```c
#define DS_UDP_PORT             2011
#define DS_UDP_TASK_PRIORITY    tskIDLE_PRIORITY + 6
#define DS_UDP_TASK_STACK_SIZE  2 * configMINIMAL_STACK_SIZE
```

### Optional Features
```c
#define CRC8_ENABLE 0                   // Enable CRC8 checksum
#define DS_STATS_ENABLE 1               // Enable packet statistics
#define DS_AUTO_DETECTION_ENABLE 1      // Enable board auto-detection
#define DS_CLI_ENABLE 0                 // Enable CLI interface
```

### Board Auto-Detection Configuration
```c
#define DS_BOARD_TYPE BOARD_TYPE_CUSTOM_1
#define DS_BOARD_NAME "datastream_test"
#define DS_FIRMWARE_VERSION 0x0200      // Version 2.0
#define DS_BOARD_ID 0x12345678          // Unique identifier per board
```

### Detailed Configuration Guide

For comprehensive configuration instructions, troubleshooting, and migration from older versions, see [USER_INTEGRATION_GUIDE.md](USER_INTEGRATION_GUIDE.md).

---

## Adding the Library to an ESP-IDF Project

### Step 1: Copy the Library

Copy the datastream library into your project's `components/` directory:

```
your_project/
├── main/
│   ├── main.c
│   └── CMakeLists.txt
└── components/
    └── datastream/
        ├── CMakeLists.txt   <-- create this
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

If CLI support is enabled (`DS_CLI_ENABLE 1`), add `"src/dsCLI.c"` to SRCS.

### Step 3: Create Your Configuration File

Create `ds_app_config.h` in your `main/` directory or any directory in the include path:

```c
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

#define DS_PLATFORM             ESP32
#define DS_BOARD_NAME           "My ESP32 Board"
#define DS_BOARD_ID             0x00000001
#define DS_AUTO_DETECTION_ENABLE 1
#define DS_STATS_ENABLE         1
#define CRC8_ENABLE             0

#define DS_USER_REGISTER_DEFINITIONS  "my_registers.h"
#define DS_USER_PARAMETER_DEFINITIONS "my_parameters.h"

#endif
```

Make sure the directory containing `ds_app_config.h` is in the component's `INCLUDE_DIRS` or your main component's includes.

### Step 4: Initialize in Your Application

```c
#include "datastream.h"
#include "dsTCP.h"
#include "dsUDP.h"

void app_main(void)
{
    // Initialize network stack (WiFi / Ethernet) before calling dsInitialize()
    // ...

    dsInitialize();
    dsTCPTaskCreate();
    dsUDPTaskCreate();
}
```

### Step 5: Implement Flash Callbacks

You must override the two flash weak functions to enable parameter persistence:

```c
void dsWriteParametersToFlash(ds_parameters_t *parList)
{
    // Use NVS, SPIFFS, or raw flash write
    // Example with NVS:
    nvs_handle_t handle;
    nvs_open("datastream", NVS_READWRITE, &handle);
    nvs_set_blob(handle, "params", parList, sizeof(ds_parameters_t));
    nvs_commit(handle);
    nvs_close(handle);
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

## Adding the Library to an STM32CubeIDE Project

### Step 1: Copy the Library

Copy the `datastream/` folder into your project's `Middlewares/Third_Party/` directory:

```
your_project/
├── Core/
│   ├── Inc/
│   │   └── ds_app_config.h   <-- create this
│   └── Src/
└── Middlewares/
    └── Third_Party/
        └── datastream/
            ├── include/
            └── src/
```

In STM32CubeIDE, right-click the project → **Refresh** to make the IDE detect the new files.

### Step 2: Add the Include Path

Open **Project Properties → C/C++ Build → Settings → MCU GCC Compiler → Include Paths**.

Add:
```
../Middlewares/Third_Party/datastream/include
```

Do this for both **Debug** and **Release** configurations.

### Step 3: Verify Source Files Are Included in the Build

In the Project Explorer, expand `Middlewares/Third_Party/datastream/src/`. Each `.c` file should appear without a red exclusion badge.

If any file is excluded, right-click it → **Resource Configurations → Exclude from build** → uncheck the relevant configurations.

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

The library will auto-detect this file via `__has_include` as long as the `Core/Inc/` directory is in your project's include path (it is by default in CubeMX-generated projects).

### Step 5: Initialize in Your Application

```c
#include "datastream.h"
#include "dsTCP.h"
#include "dsUDP.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    // ... other peripheral initialization ...

    dsInitialize();
    dsTCPTaskCreate();
    dsUDPTaskCreate();

    vTaskStartScheduler();
    while (1) {}
}
```

### Step 6: Implement Flash Callbacks

You must override the two flash weak functions to enable parameter persistence:

```c
#include "dsParameters.h"

void dsWriteParametersToFlash(ds_parameters_t *parList)
{
    // Use your HAL flash driver to write parList to a reserved flash sector
}

void dsReadParametersFromFlash(ds_parameters_t *parList)
{
    // Read previously stored parameters from flash into parList
}
```

### FreeRTOS+TCP Requirement

The STM32 platform uses FreeRTOS+TCP for networking. Ensure FreeRTOS+TCP is enabled and configured in your project (via STM32CubeMX or manually). The library expects the standard FreeRTOS+TCP headers (`FreeRTOS_IP.h`, `FreeRTOS_Sockets.h`) to be in your include path.

---

## API Reference

### Core Functions

#### Initialization
```c
void dsInitialize(void);
```
Initialize the datastream protocol, system registers, user registers, parameters, and the control mutex. Call this before creating any network tasks.

#### Packet Processing
```c
void dsProcessPacket(dsRxPacket *inPacket, dsTxPacket *outPacket);
```
Process an incoming packet and populate the response. Called internally by the TCP/UDP tasks.

#### User-Defined Types
```c
bool dsProcessUserDefinedType(dsRxPacket *inPacket, dsTxPacket *outPacket, reply_t *reply);
```
Handle custom packet types (type 100+). Implement this weak function in your application.

#### User-Defined System Commands
```c
bool dsProcessUserSysCommand(const dsRxPacket *inputPacket, dsTxPacket *outputPacket);
```
Handle custom system commands (values 0–199). Implement this weak function in your application. Return `true` if the command was handled, `false` to let the library return an error.

### Parameter Management

```c
void dsSetParameter(uint32_t address, uint32_t value, reply_t *reply);
uint32_t dsGetParameter(uint32_t address, reply_t *reply);

// Weak — MUST be overridden for persistent storage
void dsWriteParametersToFlash(ds_parameters_t *parList);
void dsReadParametersFromFlash(ds_parameters_t *parList);
```

### User Register Management

```c
void dsSetRegister(uint32_t address, uint32_t value, reply_t *reply);
uint32_t dsGetRegister(uint32_t address, reply_t *reply);
```

### System Register Management

System registers are library-managed and accessed via `ds_type_READ_SYSTEM_REGISTER` / `ds_type_WRITE_SYSTEM_REGISTER` packet types. They are stored in the global `SYS_REGS` struct.

```c
uint32_t dsGetSystemRegister(uint32_t address, reply_t *reply);
void dsSetSystemRegister(uint32_t address, uint32_t value, reply_t *reply);
```

Available system registers (in address order):

| Register | Type | Access | Description |
|----------|------|--------|-------------|
| `DS_PACKET_COUNT` | `uint32_t` | Read-only | Total packets received (if `DS_STATS_ENABLE`) |
| `DS_ERROR_COUNT` | `uint32_t` | Read-only | Total errors encountered (if `DS_STATS_ENABLE`) |
| `CONTROL_INTERFACE` | `uint32_t` | Read-only | Current control interface type |
| `COUNTER_1HZ` | `uint32_t` | Read/write | 1 Hz counter, updated by application via `SYS_REGS` |

Access system registers directly in your application code:

```c
// Write from your application
SYS_REGS.byName.COUNTER_1HZ++;

// Read from your application
uint32_t packets = SYS_REGS.byName.DS_PACKET_COUNT;
```

### Network Tasks

```c
void dsTCPTaskCreate(void);  // Create TCP server task (port DS_TCP_PORT)
void dsUDPTaskCreate(void);  // Create UDP server task (port DS_UDP_PORT)
```

### Task Management

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

---

## Integration Guide

### 1. Include Headers
```c
#include "datastream.h"
#include "dsTCP.h"        // For TCP support
#include "dsUDP.h"        // For UDP support
```

### 2. Configure Library
**Option A: Create Application Config (Recommended)**
```c
// ds_app_config.h
#define DS_PLATFORM STM32
#define DS_USER_REGISTER_DEFINITIONS "my_registers.h"
#define DS_USER_PARAMETER_DEFINITIONS "my_parameters.h"
```

**Option B: Build System Defines**
```cmake
target_compile_definitions(your_target PRIVATE DS_PLATFORM=STM32)
```

### 3. Define Custom Registers and Parameters

**User register definitions (`my_registers.h`):**

User registers contain your application's real-time data. System registers (packet counts, control interface, counters) are managed by the library separately — do **not** add them here.

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

} ds_register_names_t;

DS_STATIC_ASSERT(sizeof(ds_register_names_t) % 4 == 0, "Register struct must be 4-byte aligned");

#define DS_REGISTERS_BY_NAME_T_SIZE     sizeof(ds_register_names_t)
#define DS_REGISTER_COUNT               (DS_REGISTERS_BY_NAME_T_SIZE / sizeof(uint32_t))
#define DS_REGISTERS_READ_ONLY_COUNT    3   // number of read-only registers at the top

#endif
```

**Parameter definitions (`my_parameters.h`):**

```c
#ifndef MY_PARAMETERS_H_
#define MY_PARAMETERS_H_

#include "ds_default_config.h"

typedef struct PACKED
{
    uint32_t DEVICE_ID;
    uint32_t USES_DHCP;
    uint32_t IP_ADDR[4];
    float    CALIBRATION_OFFSET;
    float    CALIBRATION_SCALE;
    uint32_t OPERATING_MODE;

    // Optional but recommended: bookkeeping fields for flash management
    uint32_t PARAMETERS_SETS_IN_FLASH;
    uint32_t PARAMETERS_INITIALIZATION_MARKER;

} ds_parameter_names_t;

DS_STATIC_ASSERT(sizeof(ds_parameter_names_t) % 4 == 0, "Parameter struct must be 4-byte aligned");

#define DS_PARAMETERS_T_SIZE   sizeof(ds_parameter_names_t)
#define DS_PARAMETER_COUNT     (DS_PARAMETERS_T_SIZE / sizeof(uint32_t))

#endif
```

### 4. Initialize System
```c
int main(void)
{
    // Initialize your system...

    dsInitialize();
    dsTCPTaskCreate();  // Optional: TCP interface
    dsUDPTaskCreate();  // Optional: UDP interface

    // Start FreeRTOS scheduler
    vTaskStartScheduler();
}
```

### 5. Implement Callbacks (Optional)
```c
// Called after a parameter is written
void dsParameterSetCallback(uint32_t address, uint32_t oldValue, uint32_t newValue)
{
    // React to parameter changes
}

// Called after a parameter is read
void dsParameterGetCallback(uint32_t address, uint32_t value)
{
}

// Called after a user register is written
void dsRegisterSetCallback(uint32_t address, uint32_t oldValue, uint32_t newValue)
{
    // React to register changes
}

// Called after a user register is read
void dsRegisterGetCallback(uint32_t address, uint32_t value)
{
}

// Handle user-defined system commands (values 0–199)
bool dsProcessUserSysCommand(const dsRxPacket *inputPacket, dsTxPacket *outputPacket)
{
    switch (inputPacket->contents.value)
    {
        case cmd_START:
            outputPacket->contents.status = SYS_COMMAND_OK_REPLY_VAL;
            return true;

        case cmd_STOP:
            outputPacket->contents.status = SYS_COMMAND_OK_REPLY_VAL;
            return true;

        default:
            return false;
    }
}
```

---

## Client Examples

### Python TCP Client
```python
import socket
import struct

def send_command(sock, cmd_type, address, value):
    # Pack command: type(1B) + addr(1B) + value(4B) — little-endian
    packet = struct.pack('<BB I', cmd_type, address, value)
    sock.send(packet)

    # Receive response: status(1B) + addr(1B) + value(4B)
    response = sock.recv(6)
    status, addr, val = struct.unpack('<bB I', response)
    return status, addr, val

# Connect to device
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.1.100', 2009))

# Read parameter at address 0
status, addr, value = send_command(sock, 3, 0, 0)  # 3 = READ_PARAMETER
print(f"Parameter 0: {value}")

# Write parameter at address 0
status, addr, value = send_command(sock, 4, 0, 42)  # 4 = WRITE_PARAMETER
print(f"Write status: {status}")

# Read system register (e.g. packet count at address 0)
status, addr, value = send_command(sock, 6, 0, 0)   # 6 = READ_SYSTEM_REGISTER
print(f"Packet count: {value}")

sock.close()
```

### C Client Example
```c
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct {
    uint8_t  type;
    uint8_t  address;
    uint32_t value;
} __attribute__((packed)) ds_packet_t;

uint32_t send_datastream_command(int sock, uint8_t type, uint8_t addr, uint32_t value)
{
    ds_packet_t tx = {type, addr, value};
    ds_packet_t rx;

    send(sock, &tx, sizeof(tx), 0);
    recv(sock, &rx, sizeof(rx), 0);

    return rx.value;
}
```

---

## Board Auto-Detection

Datastream 2.0 includes an automatic board discovery feature that allows clients to find and identify boards on the network without prior knowledge of their IP addresses.

### How It Works

1. **Discovery Request**: Client sends a UDP broadcast discovery packet to the network
2. **Board Response**: Each board responds with identification information
3. **Client Collection**: Client receives all responses and builds a list of available boards

### Discovery Protocol

#### Discovery Request Packet (8 bytes)
```
+--------+--------+--------+--------+
|    Magic Number (0xDEADBEEF)      |
+--------+--------+--------+--------+
|  Cmd   |     Reserved (3 bytes)   |
+--------+--------+--------+--------+
```

#### Discovery Response Packet (44 bytes)
```
+--------+--------+--------+--------+
|    Magic Number (0xDEADBEEF)      |   4 bytes
+--------+--------+--------+--------+
|  Cmd   | BrdTyp |  Firmware Ver.  |   1 + 1 + 2 = 4 bytes
+--------+--------+--------+--------+
|           Board ID                |   4 bytes
+--------+--------+--------+--------+
|         Board IP Address          |   4 bytes
+--------+--------+--------+--------+
| TCP Pt | UDP Pt |                 |   2 + 2 = 4 bytes
+--------+--------+                 |
|     MAC Address (6 bytes)         |   6 bytes
+                 +--------+--------+
|                | Reservd |         |   2 bytes
+--------+--------+--------+--------+
|     Board Name (16 bytes)         |   16 bytes
+--------+--------+--------+--------+
                              Total: 44 bytes
```

### Auto-Detection API

#### Initialization
```c
void dsAutoDetectionInit(void);
```
Initialize the auto-detection subsystem (called automatically by the UDP task).

#### Packet Processing
```c
bool dsProcessDiscoveryPacket(const uint8_t *rxBuffer, size_t rxSize,
                              uint8_t *txBuffer, size_t *txSize,
                              uint32_t clientIP);
```
Process incoming discovery requests and generate responses.

### Client Example — Board Discovery

#### Python Discovery Client
```python
import socket
import struct
import time

def discover_boards(timeout=2.0):
    """Discover all datastream boards on the network."""
    boards = []

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(timeout)

    # Discovery request: magic(4) + command(1) + padding(3)
    request = struct.pack('<I B 3s', 0xDEADBEEF, 0x01, b'\x00\x00\x00')

    try:
        sock.sendto(request, ('<broadcast>', 2011))

        start = time.time()
        while time.time() - start < timeout:
            try:
                data, addr = sock.recvfrom(1024)
                if len(data) >= 44:
                    # magic(4) cmd(1) type(1) fw(2) id(4) ip(4) tcp(2) udp(2) mac(6) pad(2) name(16)
                    magic, cmd, board_type, fw_ver, board_id, ip, tcp_port, udp_port, mac, _, name \
                        = struct.unpack('<I B B H I I H H 6s 2x 16s', data[:44])

                    if magic == 0xDEADBEEF and cmd == 0x02:
                        boards.append({
                            'ip':               addr[0],
                            'board_type':       board_type,
                            'firmware_version': f"{fw_ver >> 8}.{fw_ver & 0xFF}",
                            'board_id':         f"0x{board_id:08X}",
                            'tcp_port':         tcp_port,
                            'udp_port':         udp_port,
                            'mac':              ':'.join(f'{b:02X}' for b in mac),
                            'name':             name.decode('utf-8').rstrip('\x00'),
                        })
            except socket.timeout:
                break
    finally:
        sock.close()

    return boards

boards = discover_boards()
for board in boards:
    print(f"Found: {board['name']} at {board['ip']}:{board['tcp_port']}  MAC: {board['mac']}")
```

#### C Discovery Client
```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct {
    uint32_t magic;
    uint8_t  command;
    uint8_t  reserved[3];
} __attribute__((packed)) discovery_request_t;

typedef struct {
    uint32_t magic;
    uint8_t  command;
    uint8_t  board_type;
    uint16_t firmware_version;
    uint32_t board_id;
    uint32_t ip_address;
    uint16_t tcp_port;
    uint16_t udp_port;
    uint8_t  mac_address[6];
    uint8_t  reserved[2];
    char     board_name[16];
} __attribute__((packed)) discovery_response_t;  // 44 bytes

int discover_boards(void)
{
    int sock;
    struct sockaddr_in bcast_addr, from_addr;
    socklen_t from_len = sizeof(from_addr);
    discovery_request_t  req  = {0xDEADBEEF, 0x01, {0}};
    discovery_response_t resp;
    int broadcast = 1;
    struct timeval tv = {2, 0};

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,  &tv, sizeof(tv));

    memset(&bcast_addr, 0, sizeof(bcast_addr));
    bcast_addr.sin_family      = AF_INET;
    bcast_addr.sin_port        = htons(2011);
    bcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

    sendto(sock, &req, sizeof(req), 0,
           (struct sockaddr *)&bcast_addr, sizeof(bcast_addr));

    while (recvfrom(sock, &resp, sizeof(resp), 0,
                    (struct sockaddr *)&from_addr, &from_len) > 0)
    {
        if (resp.magic == 0xDEADBEEF && resp.command == 0x02)
        {
            printf("Found: %s  IP: %s  TCP: %d  MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   resp.board_name, inet_ntoa(from_addr.sin_addr),
                   resp.tcp_port,
                   resp.mac_address[0], resp.mac_address[1], resp.mac_address[2],
                   resp.mac_address[3], resp.mac_address[4], resp.mac_address[5]);
        }
    }

    close(sock);
    return 0;
}
```

### Integration Notes

- Auto-detection uses the same UDP port as regular datastream communication
- Discovery packets are automatically detected and handled by the UDP task
- No additional sockets or tasks are required
- Each board should have a unique `DS_BOARD_ID` for identification

---

## Helper Scripts

The `tools/` directory contains Python scripts for testing and debugging your datastream setup.

### Mock ESP32 Discovery Server

Simulates an ESP32 board responding to discovery requests. Useful for testing client applications without physical hardware.

```bash
python3 tools/mock_esp32_discovery.py
```

**Output:**
```
Mock ESP32 Discovery Server
Listening on 0.0.0.0:2011
Board: MockESP32-Test, Type: 1, Serial: 0x12345678
------------------------------------------------------------
Waiting for discovery requests... Press Ctrl+C to stop
```

### UDP Receive Test

A minimal UDP listener for diagnosing broadcast and response issues.

```bash
python3 tools/test_udp_receive.py
```

**Output:**
```
Listening on 192.168.1.100:54321
Waiting for UDP packets... Press Ctrl+C to stop
```

### Testing Workflow

1. **Without Hardware** — test your client application:
   ```bash
   python3 tools/mock_esp32_discovery.py
   # In another terminal, run your discovery client
   ```

2. **With Hardware** — verify board responses:
   ```bash
   python3 tools/test_udp_receive.py
   # Send a discovery broadcast and watch for responses
   ```

3. **Network Debugging** — check if packets are reaching your machine:
   ```bash
   python3 tools/test_udp_receive.py
   # Any UDP traffic to your IP will be displayed
   ```

---

## Error Handling

Always check the response status in your client:

```c
if (reply.val < 0)
{
    switch (reply.val)
    {
        case ADDRESS_OUT_OF_RANGE_ERROR_REPLY_VAL:
            // Handle address error
            break;
        case PERMISSION_ERROR_REPLY_VAL:
            // Handle permission error (read-only register or no control)
            break;
        case CRC_ERROR_REPLY_VAL:
            // Handle CRC mismatch
            break;
    }
}
```

---

## Control Interface Management

Datastream supports exclusive write access control through a task registration system. Only tasks with proper control interface permissions can write to registers and parameters.

### Task Registration System

Network tasks (`dsTCP`, `dsUDP`) register themselves automatically. If your application creates additional tasks that need write access, register them manually:

```c
void app_main(void)
{
    dsInitialize();

    // Register your application task
    TaskHandle_t myTask = xTaskGetCurrentTaskHandle();
    dsRegisterControlTask(myTask, ds_control_TCP_DATASTREAM, "AppTask");

    dsTCPTaskCreate();
    dsUDPTaskCreate();
}
```

### Control Interface Flow

1. **Task Registration**: Register task handles with `dsRegisterControlTask()`
2. **Request Control**: Client sends `CONTROL_INTERFACE` command via a registered task
3. **Permission Check**: Library verifies the calling task has write permission
4. **Write Operations**: Only the controlling task can write to registers/parameters
5. **Release Control**: Connection termination or explicit release frees control

### Control Interface Types

| Constant | Value | Description |
|----------|-------|-------------|
| `ds_control_UNDECIDED` | 0 | Not yet assigned |
| `ds_control_TCP_DATASTREAM` | 1 | TCP datastream connection |
| `ds_control_UDP_DATASTREAM` | 2 | UDP datastream connection |
| `ds_control_USER_DEFINED_START` | 100 | Base for user-defined types |
| `ds_control_TCP_CLI` | 101 | TCP CLI connection |
| `ds_control_USB` | 102 | USB connection |

---

## Memory Requirements

### Code Size (Flash Memory)

| Configuration | Debug (-Og) | Production (-Os) |
|---------------|-------------|-----------------|
| Core only | ~5.6 KB | ~3.9 KB |
| Core + TCP | ~7.0 KB | ~5.0 KB |
| Core + UDP | ~7.4 KB | ~5.2 KB |
| Full (TCP + UDP) | ~8.8 KB | ~6.4 KB |

**Component breakdown (debug build):**
- `datastream.c`: 4.4 KB — protocol processing, task registration
- `dsTCP.c`: 1.4 KB — TCP server task
- `dsUDP.c`: 1.8 KB — UDP server + auto-detection
- `dsRegisters.c`: 0.4 KB — register management
- `dsParameters.c`: 0.3 KB — parameter management

### RAM Usage

- **Static RAM (BSS)**: ~641 bytes
- **Stack per task**: 2–4 KB (configurable via `DS_TCP_TASK_STACK_SIZE`, `DS_UDP_TASK_STACK_SIZE`)
- **Network buffers**: managed by FreeRTOS+TCP / lwIP (outside datastream)

**Total typical RAM:** ~5–9 KB depending on task stack configuration.

### Size Optimization Tips

1. **Use `-Os` optimization** for production — saves ~2.6 KB (−29%)
2. **Disable unused interfaces** — removing UDP saves ~1.8 KB; removing TCP saves ~1.4 KB
3. **Disable optional features**:
   - `DS_STATS_ENABLE 0` — saves ~200 bytes
   - `CRC8_ENABLE 0` — saves ~150 bytes
   - `DS_AUTO_DETECTION_ENABLE 0` — saves ~600 bytes

---

## Platform-Specific Notes

### ESP32

- Uses the native ESP-IDF socket API (`<sys/socket.h>`, `<netinet/in.h>`)
- FreeRTOS headers require the `freertos/` prefix: `freertos/FreeRTOS.h`, `freertos/semphr.h`
- IP and MAC addresses are retrieved via `esp_netif_get_ip_info()` and `esp_netif_get_mac()`
- Logging uses ESP-IDF macros (`DS_LOGI`, `DS_LOGE`) when `ESP32_LOGGING_ENABLE 1` is defined
- Use `xSemaphore` mutex instead of `taskENTER_CRITICAL()` (the ESP32 critical section requires a spinlock parameter not present in standard FreeRTOS API)
- Parameters are typically persisted using NVS (Non-Volatile Storage)

### STM32

- Uses FreeRTOS+TCP (`FreeRTOS_IP.h`, `FreeRTOS_Sockets.h`)
- Compatible with STM32CubeMX-generated projects
- Tested with STM32F4, STM32F7, STM32H7 series
- Firmware reset uses `HAL_NVIC_SystemReset()` from `main.h`
- Parameters are typically persisted to an internal flash sector using HAL flash drivers

### RP2040 / RP2350

- Uses FreeRTOS+TCP
- Compatible with the Pico SDK
- Requires correct clock and network configuration before calling `dsInitialize()`

### GENERIC_PLATFORM

- Provides minimal platform abstraction
- Socket type, network calls, and MAC/IP retrieval must be supplied by the user via the relevant weak functions

---

## License

The datastream library is licensed under the [PolyForm Noncommercial License 1.0.0](LICENSE).

- **Free** for personal, hobby, educational, and non-profit use.
- **Commercial use** (products sold, revenue-generating services, internal business tools) requires a separate license — see [COMMERCIAL.md](COMMERCIAL.md).

```
SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
Copyright (c) 2026 Sofian Jafar
```

---

## Documentation

Generate the full Doxygen API reference:

```bash
cd datastream
doxygen Doxyfile
```

Then open `docs/html/index.html`. The documentation includes:
- Complete API reference with function documentation
- Module organization (core, registers, parameters, TCP, UDP)
- Usage examples and code snippets
- Platform-specific implementation notes

---

## Changelog

### Version 2.0 (Current)

#### Major Features
- **External Configuration System**: Configure library without modifying library files via `ds_app_config.h`
- **Automatic Platform Detection**: Platform-specific includes and optimisations for ESP32, STM32, RP2040/2350
- **Separate System Registers**: Library registers (stats, control interface, counter) moved to `SYS_REGS`, accessed via dedicated `READ_SYSTEM_REGISTER` / `WRITE_SYSTEM_REGISTER` packet types
- **System Commands at High Values**: Built-in system commands (READ_FLASH, WRITE_FLASH, RESET_FIRMWARE) start at 200, leaving 0–199 free for user-defined commands without any offset
- **User-Defined Registers/Parameters**: External header-based customisation with placeholder defaults
- **Board Auto-Detection**: UDP broadcast discovery protocol (0xDEADBEEF magic, 44-byte response with MAC address)
- **Task Registration System**: Runtime control interface management with `dsRegisterControlTask()`
- **Separate System Register Init**: `dsInitializeSystemRegisters()` weak function for system register setup
- **Comprehensive Doxygen Documentation**: Complete API documentation with examples

#### Architecture Improvements
- **Modular Design**: Separated TCP and UDP server implementations
- **Platform Abstraction**: Centralized platform definitions in `ds_platforms.h`
- **Configuration Hierarchy**: Default config → Platform config → Application config
- **Code Quality**: Consistent Allman-style bracing, 4-space indentation, removed dead code

#### Bug Fixes
- Fixed NULL pointer dereference in `datastream.c`
- Fixed undefined behavior with `strcat` on uninitialized memory in reply initialization
- Added NULL checks to `dsGetParameter()` and `dsGetRegister()`
- Fixed `WRITE_SYSTEM_REGISTER` response to echo written value on success (consistent with `WRITE_REGISTER`)
