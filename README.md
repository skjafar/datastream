# Datastream 2.0

A cross-platform, lightweight communication protocol library for embedded systems, providing TCP and UDP interfaces for real-time parameter and register management.

## Quick Getting Started

Get up and running in under 5 minutes:

### 1. Add the Library to Your Project

Copy the datastream library into your project and add it to your build system:

```cmake
# CMakeLists.txt
add_subdirectory(datastream)
target_link_libraries(your_target PRIVATE datastream)
```

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
- **System Commands**: Device control (ON/OFF, RESET, TRIGGER, etc.)
- **Control Interface Management**: Exclusive write access control
- **Board Auto-Detection**: Automatic discovery of boards on the network
- **FreeRTOS Integration**: Task-based architecture with configurable priorities
- **Optional CRC8 Protection**: Data integrity verification for unreliable connections
- **Statistics Tracking**: Packet counting and error monitoring

## Protocol Specification

### Packet Structure

#### Input Packet (6-7 bytes)
```
+------+------+------+------+------+------+------+
| Type | Addr |      Value (32-bit)     | CRC* |
+------+------+------+------+------+------+------+
  1B     1B              4B               1B*
```

#### Output Packet (6-7 bytes)
```
+--------+------+------+------+------+------+------+
| Status | Addr |      Value (32-bit)     | CRC* |
+--------+------+------+------+------+------+------+
   1B      1B              4B               1B*
```

*CRC field is optional (configurable via `CRC8_ENABLE`)

### Command Types

| Type | Command | Description |
|------|---------|-------------|
| 0 | ds_type_SYS_COMMAND | Execute system commands (ON/OFF/RESET/etc.) |
| 1 | ds_type_READ_REGISTER | Read device register value |
| 2 | ds_type_WRITE_REGISTER | Write device register value |
| 3 | ds_type_READ_PARAMETER | Read configuration parameter |
| 4 | ds_type_WRITE_PARAMETER | Write configuration parameter |
| 5 | ds_type_CONTROL_INTERFACE | Request exclusive write access |
| 100+ | USER_DEFINED | User-defined input types |

### System Commands

Core commands (0-99 reserved):
- `ds_sys_command_READ_FLASH = 0` - Read parameters from flash memory
- `ds_sys_command_WRITE_FLASH = 1` - Write parameters to flash memory
- `ds_sys_command_RESET_FIRMWARE = 2` - Reset firmware

User-defined commands start from `ds_sys_command_USER_DEFINED_START = 100`

### Response Codes

#### Success Codes (Positive)
- `0` - System command OK
- `1` - Read register OK
- `2` - Write register OK
- `3` - Read parameter OK
- `4` - Write parameter OK
- `5` - Control interface OK

#### Error Codes (Negative)
- `-1` - System command error
- `-2` - Invalid input type
- `-3` - Packet size error
- `-4` - Address out of range
- `-5` - Permission denied (read-only)
- `-6` - Control interface error
- `-7` - Syntax error
- `-8` - Parameters error
- `-9` - CRC error

## Configuration

Datastream 2.0 introduces an external configuration system that allows you to customize the library without modifying any library files. This makes the library distributable while maintaining full customization capabilities.

### Quick Start Configuration

The simplest way to configure datastream is to create a `ds_app_config.h` file in your project. The library automatically detects and includes this file using the `__has_include` preprocessor feature - if the file doesn't exist, the library uses sensible defaults.

```c
// ds_app_config.h - Place in your project root or include path
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

// Optional: Suppress compiler warnings about using defaults
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

### Detailed Configuration Guide

For comprehensive configuration instructions, troubleshooting, and migration from older versions, see [USER_INTEGRATION_GUIDE.md](USER_INTEGRATION_GUIDE.md).

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
```

### Board Auto-Detection Configuration
```c
// Board type definitions - customize for your boards
#define DS_BOARD_TYPE BOARD_TYPE_PS_TRIG_FANOUT
#define DS_BOARD_NAME "datastream_test"
#define DS_FIRMWARE_VERSION 0x0200      // Version 2.0
#define DS_BOARD_SERIAL 0x12345678      // Unique per board
```

## API Reference

### Core Functions

#### Initialization
```c
void dsInitialize(void);
```
Initialize the datastream protocol system.

#### Packet Processing
```c
void dsProcessPacket(dsRxPacket *inPacket, dsTxPacket *outPacket);
```
Process incoming packet and generate response.

#### User-Defined Types
```c
bool dsProcessUserDefinedType(dsRxPacket *inPacket, dsTxPacket *outPacket, reply_t *reply);
```
Handle custom packet types (weak function - implement in your application).

### Parameter Management

```c
void dsSetParameter(uint32_t address, uint32_t value, reply_t *reply);
uint32_t dsGetParameter(uint32_t address, reply_t *reply);
void dsWriteParametersToFlash(ds_parameters_t *parList);
void dsReadParametersFromFlash(ds_parameters_t *parList);
```

### Register Management

```c
void dsSetRegister(uint32_t address, uint32_t value, reply_t *reply);
uint32_t dsGetRegister(uint32_t address, reply_t *reply);
```

### Network Tasks

```c
void dsTCPTaskCreate(void);  // Create TCP server task
void dsUDPTaskCreate(void);  // Create UDP server task
```

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

### 3. Define Custom Registers and Parameters (Optional)
Create custom definitions if needed:

**Register definitions (`my_registers.h`):**
```c
typedef struct PACKED {
    // Required system registers
    #if (DS_STATS_ENABLE == 1)
    uint32_t DS_PACKET_COUNT;
    uint32_t DS_ERROR_COUNT;
    #endif
    ds_control_interface_t CONTROL_INTERFACE;
    
    // Your application registers
    uint32_t STATUS_FLAGS;
    float SENSOR_READING;
    
} ds_register_names_t;

#define DS_REGISTERS_BY_NAME_T_SIZE sizeof(ds_register_names_t)
#define DS_REGISTER_COUNT (DS_REGISTERS_BY_NAME_T_SIZE / sizeof(uint32_t))
#define DS_REGISTERS_READ_ONLY_COUNT 3  // Count of read-only registers
```

**Parameter definitions (`my_parameters.h`):**
```c
typedef struct PACKED {
    // Your application parameters
    uint32_t DEVICE_ID;
    float CALIBRATION_FACTOR;
    
    // Required system parameters (must be at end)
    uint32_t PARAMETERS_SETS_IN_FLASH;
    uint32_t PARAMETERS_INITIALIZATION_MARKER;
    
} ds_parameter_names_t;

#define DS_PARAMETERS_T_SIZE sizeof(ds_parameter_names_t)
#define DS_PARAMETER_COUNT (DS_PARAMETERS_T_SIZE / sizeof(uint32_t))
```

### 4. Initialize System
```c
int main(void) {
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
// Parameter callbacks
void dsParameterSetCallback(uint32_t address, uint32_t oldValue, uint32_t newValue) {
    // Handle parameter changes
}

void dsParameterGetCallback(uint32_t address, uint32_t value) {
    // Handle parameter reads
}

// Register callbacks  
void dsRegisterSetCallback(uint32_t address, uint32_t oldValue, uint32_t newValue) {
    // Handle register changes
}

void dsRegisterGetCallback(uint32_t address, uint32_t value) {
    // Handle register reads
}

// System command processing
bool processSysCommand(ds_sys_command_t command, uint32_t value, reply_t *reply) {
    switch(command) {
        case command_ON:
            // Turn system on
            return true;
        case command_OFF:
            // Turn system off
            return true;
        // Handle other commands...
    }
    return false;
}
```

## Client Examples

### Python TCP Client
```python
import socket
import struct

def send_command(sock, cmd_type, address, value):
    # Pack command: type(1B) + addr(1B) + value(4B)
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
status, addr, value = send_command(sock, 3, 0, 0)  # READ_PARAMETER
print(f"Parameter 0: {value}")

# Write parameter at address 0
status, addr, value = send_command(sock, 4, 0, 42)  # WRITE_PARAMETER
print(f"Write status: {status}")

sock.close()
```

### C Client Example
```c
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct {
    uint8_t type;
    uint8_t address;
    uint32_t value;
} __attribute__((packed)) ds_packet_t;

int send_datastream_command(int sock, uint8_t type, uint8_t addr, uint32_t value) {
    ds_packet_t tx_packet = {type, addr, value};
    ds_packet_t rx_packet;
    
    send(sock, &tx_packet, sizeof(tx_packet), 0);
    recv(sock, &rx_packet, sizeof(rx_packet), 0);
    
    return rx_packet.value;
}
```

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

#### Discovery Response Packet (36 bytes)
```
+--------+--------+--------+--------+
|    Magic Number (0xDEADBEEF)      |
+--------+--------+--------+--------+
|  Cmd   | Type   | Firmware Ver.   |
+--------+--------+--------+--------+
|        Board Serial Number        |
+--------+--------+--------+--------+
|         Board IP Address          |
+--------+--------+--------+--------+
| TCP Pt | UDP Pt |                 |
+--------+--------+                 |
|     Board Name (16 bytes)         |
|                                   |
+--------+--------+--------+--------+
```

### Auto-Detection API

#### Initialization
```c
void dsAutoDetectionInit(void);
```
Initialize the auto-detection subsystem (called automatically by dsUDP task).

#### Packet Processing
```c
bool dsProcessDiscoveryPacket(const uint8_t *rxBuffer, size_t rxSize, 
                              uint8_t *txBuffer, size_t *txSize, 
                              uint32_t clientIP);
```
Process incoming discovery requests and generate responses.

### Client Example - Board Discovery

#### Python Discovery Client
```python
import socket
import struct
import time

def discover_boards(timeout=2.0):
    """Discover all datastream boards on the network"""
    boards = []
    
    # Create UDP socket for broadcast
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(timeout)
    
    # Prepare discovery request
    magic = 0xDEADBEEF
    command = 0x01  # DS_DISCOVERY_REQUEST
    request = struct.pack('<I B 3s', magic, command, b'\x00\x00\x00')
    
    try:
        # Send broadcast discovery request
        sock.sendto(request, ('<broadcast>', 2011))
        
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                data, addr = sock.recvfrom(1024)
                if len(data) >= 36:  # Discovery response size
                    # Parse response
                    resp = struct.unpack('<I B B H I I H H 16s', data[:36])
                    if resp[0] == magic and resp[1] == 0x02:  # Valid response
                        board = {
                            'ip': addr[0],
                            'board_type': resp[2],
                            'firmware_version': f"{resp[3]>>8}.{resp[3]&0xFF}",
                            'board_id': f"0x{resp[4]:08X}",
                            'tcp_port': resp[6],
                            'udp_port': resp[7],
                            'name': resp[8].decode('utf-8').rstrip('\x00')
                        }
                        boards.append(board)
            except socket.timeout:
                continue
                
    finally:
        sock.close()
    
    return boards

# Usage
boards = discover_boards()
for board in boards:
    print(f"Found: {board['name']} at {board['ip']}:{board['udp_port']}")
    print(f"  Type: {board['board_type']}, ID: {board['board_id']}")
```

#### C Discovery Client
```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct {
    uint32_t magic;
    uint8_t command;
    uint8_t reserved[3];
} discovery_request_t;

typedef struct {
    uint32_t magic;
    uint8_t command;
    uint8_t board_type;
    uint16_t firmware_version;
    uint32_t board_id;        // Renamed from board_serial
    uint32_t ip_address;
    uint16_t tcp_port;
    uint16_t udp_port;
    char board_name[16];
} __attribute__((packed)) discovery_response_t;

int discover_boards(void) {
    int sock;
    struct sockaddr_in broadcast_addr, from_addr;
    socklen_t from_len = sizeof(from_addr);
    discovery_request_t request = {0xDEADBEEF, 0x01, {0}};
    discovery_response_t response;
    int broadcast = 1;
    struct timeval tv = {2, 0}; // 2 second timeout
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Send broadcast discovery request
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(2011);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
    
    sendto(sock, &request, sizeof(request), 0, 
           (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
    
    // Receive responses
    while (recvfrom(sock, &response, sizeof(response), 0, 
                   (struct sockaddr*)&from_addr, &from_len) > 0) {
        if (response.magic == 0xDEADBEEF && response.command == 0x02) {
            printf("Found board: %s at %s\n", 
                   response.board_name, inet_ntoa(from_addr.sin_addr));
            printf("  Type: %d, Serial: 0x%08X\n", 
                   response.board_type, response.board_id);
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
- Broadcast discovery works across network subnets when properly configured
- Each board should have a unique serial number for identification

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

When a discovery request is received, the server responds with a valid discovery response packet containing mock board information.

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

This script binds to your local IP and displays any incoming UDP packets with hex dumps.

### Testing Workflow

1. **Without Hardware** - Test your client application:
   ```bash
   # Start mock server
   python3 tools/mock_esp32_discovery.py

   # In another terminal, run your discovery client
   # The mock server will respond to discovery broadcasts
   ```

2. **With Hardware** - Verify board responses:
   ```bash
   # Listen for responses from real boards
   python3 tools/test_udp_receive.py

   # Send discovery broadcast from another terminal or your application
   ```

3. **Network Debugging** - Check if packets are reaching your machine:
   ```bash
   python3 tools/test_udp_receive.py
   # Any UDP traffic to your IP will be displayed
   ```

---

## Error Handling

The library provides comprehensive error handling through status codes. Always check the response status:

```c
if (reply.val < 0) {
    // Error occurred
    switch(reply.val) {
        case ADDRESS_OUT_OF_RANGE_ERROR_REPLY_VAL:
            // Handle address error
            break;
        case PERMISSION_ERROR_REPLY_VAL:
            // Handle permission error
            break;
        // Handle other errors...
    }
}
```

## Control Interface Management

Datastream supports exclusive write access control through a task registration system. Only tasks with proper control interface permissions can write to registers and parameters.

### Task Registration System

Applications must register their tasks at runtime to participate in the control interface system:

```c
// In your TCP task or main application
void app_main(void) {
    // Initialize datastream
    dsInitialize();

    // Register your task for control interface
    TaskHandle_t myTaskHandle = xTaskGetCurrentTaskHandle();
    dsRegisterControlTask(myTaskHandle, ds_control_TCP, "MyTCPTask");

    // Start network tasks
    dsTCPTaskCreate();
    dsUDPTaskCreate();
}
```

### Control Interface Flow

1. **Task Registration**: Register task handles with `dsRegisterControlTask()`
2. **Request Control**: Client sends `CONTROL_INTERFACE` command via registered task
3. **Permission Check**: Library verifies calling task has write permission via `dsCheckTaskWritePermission()`
4. **Write Operations**: Only the controlling task can write to registers/parameters
5. **Release Control**: Connection termination or explicit release automatically frees control

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

## Memory Requirements

### Code Size (Flash Memory)

The library has a minimal footprint that varies based on configuration and optimization level:

| Configuration | Debug (-Og) | Production (-Os) | Notes |
|---------------|-------------|------------------|-------|
| **Core Only** | ~5.6 KB | ~3.9 KB | Datastream core without network tasks |
| **Core + TCP** | ~7.0 KB | **~5.0 KB** | Most common configuration |
| **Core + UDP** | ~7.4 KB | ~5.2 KB | UDP with auto-detection |
| **Full (TCP + UDP)** | ~8.8 KB | **~6.4 KB** | Complete library (recommended) |

**Component Breakdown (Debug build):**
- Core (`datastream.c`): 4.4 KB - Protocol processing, task registration
- TCP (`dsTCP.c`): 1.4 KB - TCP server task
- UDP (`dsUDP.c`): 1.8 KB - UDP server + auto-detection
- Registers (`dsRegisters.c`): 0.4 KB - Register management
- Parameters (`dsParameters.c`): 0.3 KB - Parameter management

**Adding dsTCP:** +1,451 bytes (+25% over core)
**Adding dsUDP:** +1,851 bytes (+32% over core)

### RAM Usage

- **Static RAM (BSS)**: ~641 bytes
  - Core: 513 bytes (task registration, buffers)
  - Parameters: 104 bytes
  - Registers: 16 bytes
  - Task handles: 8 bytes (TCP + UDP)
- **Stack per Task**: 2-4 KB (configurable via `DS_TCP_TASK_STACK_SIZE`, `DS_UDP_TASK_STACK_SIZE`)
- **Network Buffers**: Managed by lwIP/FreeRTOS+TCP (outside datastream)

**Total Typical RAM:** ~5-9 KB (depends on task stack configuration)

### Size Optimization Tips

1. **Use `-Os` optimization** for production: Saves ~2.6 KB (-29%)
2. **TCP only**: Remove UDP to save ~1.8 KB
3. **UDP only**: Remove TCP to save ~1.4 KB
4. **Disable features**:
   - `DS_STATS_ENABLE 0`: Saves ~200 bytes
   - `CRC8_ENABLE 0`: Saves ~150 bytes
   - `DS_AUTO_DETECTION_ENABLE 0`: Saves ~600 bytes

See [CODE_SIZE_ANALYSIS.md](CODE_SIZE_ANALYSIS.md) for detailed measurements and optimization strategies.

## Platform-Specific Notes

### ESP32
- Uses native ESP-IDF socket API
- Optional ESP-IDF logging support
- Requires FreeRTOS

### STM32
- Uses FreeRTOS+TCP
- Compatible with STM32CubeMX projects
- Tested with STM32F4/F7/H7 series

### RP2040/RP2350
- Uses FreeRTOS+TCP
- Compatible with Pico SDK
- Requires proper clock configuration

## License

TBD.


## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new features
5. Submit a pull request

## Documentation

Comprehensive API documentation is available in HTML format. Generate it using Doxygen:

```bash
cd components/datastream
doxygen Doxyfile
```

Then open `docs/html/index.html` in your browser. The documentation includes:
- Complete API reference with function documentation
- Module organization (datastream core, registers, parameters, TCP, UDP)
- Usage examples and code snippets
- Platform-specific implementation notes
- Source code browser with cross-references

## Changelog

### Version 2.0 (Current)

#### Major Features
- **External Configuration System**: Configure library without modifying library files via `ds_app_config.h`
- **Automatic Platform Detection**: Platform-specific includes and optimizations for ESP32, STM32, RP2040/2350
- **User-Defined Registers/Parameters**: Custom register and parameter definitions via external headers
- **Board Auto-Detection**: UDP broadcast network discovery protocol (0xDEADBEEF magic number)
- **Task Registration System**: Runtime control interface management with `dsRegisterControlTask()`
- **Comprehensive Doxygen Documentation**: Complete API documentation with examples

#### Architecture Improvements
- **Modular Design**: Separated TCP ([dsTCP.h](include/dsTCP.h)/[dsTCP.c](src/dsTCP.c)) and UDP ([dsUDP.h](include/dsUDP.h)/[dsUDP.c](src/dsUDP.c)) interfaces
- **Platform Abstraction**: Centralized platform definitions in [ds_platforms.h](include/ds_platforms.h)
- **Configuration Hierarchy**: Default config → Platform config → Application config (ds_app_config.h)
- **Code Quality**: Consistent Allman-style bracing, 4-space indentation, removed dead code

#### Bug Fixes
- Fixed NULL pointer dereference in [datastream.c:213-222](src/datastream.c)
- Fixed undefined behavior with `strcat` on uninitialized memory in reply initialization
- Added NULL checks to `dsGetParameter()` and `dsGetRegister()` functions
- Fixed tab indentation inconsistencies in [dsUDP.c](src/dsUDP.c)

#### API Enhancements
- **ESP32 Platform Support**: Implemented IP address retrieval via `esp_netif_get_ip_info()`
- **ESP32 Platform Support**: Implemented MAC address retrieval via `esp_netif_get_mac()` with fallback
- **Discovery Protocol**: `dsProcessDiscoveryPacket()` for auto-detection
- **Auto-Detection Init**: `dsAutoDetectionInit()` for initialization
- **Task Management**: `dsRegisterControlTask()`, `dsUnregisterControlTask()`, `dsGetTaskControlType()`
- **Application CLI**: `dsRegisterApplicationCLICommands()` for future CLI integration

#### Developer Experience
- **Zero Warnings**: Clean Doxygen documentation generation with no warnings
- **Better Error Messages**: Enhanced logging with ESP32 `DS_LOGI`, `DS_LOGE`, `DS_LOGW` macros
- **Compiler Compatibility**: Support for GCC, Clang, IAR, ARM Compiler 6
- **Integration Guide**: Comprehensive [USER_INTEGRATION_GUIDE.md](USER_INTEGRATION_GUIDE.md) with examples

## Support

For questions, issues, or contributions, please visit the project repository or contact the maintainers.
