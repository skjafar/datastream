# Datastream

A lightweight binary protocol for embedded systems. Read and write device state over TCP or UDP using a fixed 8-byte packet — no parsing, no framing, no allocator.

> **Platforms:** ESP32 · STM32 · RP2040 · RP2350 · Generic
> **Footprint:** ~4–9 KB flash, ~1 KB RAM
> **Status:** v0.2.4

---

## Why Datastream?

Most embedded projects end up reinventing the same wheel: a command dispatcher, a register table, a config blob in flash, and a way to reach it over the network. Datastream gives you all four in one small library, with a protocol simple enough to implement from a Python one-liner.

Three concepts, and that's the whole model:

| Concept | Lifetime | Use it for |
|---|---|---|
| **Register** | Runtime (RAM) | Live state — sensor values, outputs, status flags |
| **Parameter** | Persistent (flash) | Config that survives reboots — calibration, IP, mode |
| **System command** | One-shot | Actions — save to flash, reset, custom triggers |

You declare your registers and parameters as a plain C struct. The library handles the wire protocol, addressing, permissions, and flash I/O.

---

## Quick Start

### 1. Drop the library into your project

**CMake / Pico SDK**
```cmake
add_subdirectory(datastream)
target_link_libraries(your_target PRIVATE datastream)
```

**ESP-IDF** — copy into `components/datastream/` and add a `CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS  "src/datastream.c" "src/dsRegisters.c" "src/dsParameters.c"
          "src/dsTCP.c" "src/dsUDP.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES freertos esp_netif lwip
)
```

**STM32CubeIDE** — copy into `Middlewares/Third_Party/datastream/` and add `include/` to your compiler's include paths.

### 2. Create `ds_app_config.h`

The library auto-detects this file via `__has_include` — just put it somewhere on the include path.

```c
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

#define DS_PLATFORM    ESP32        // or STM32, RP2040, RP2350, GENERIC_PLATFORM
#define DS_BOARD_NAME  "My Board"
#define DS_TCP_PORT    2009
#define DS_UDP_PORT    2011

#endif
```

### 3. Initialize and go

```c
#include "datastream.h"
#include "dsTCP.h"
#include "dsUDP.h"

void app_main(void)
{
    // Bring up WiFi / Ethernet first, then:
    dsInitialize();
    dsTCPTaskCreate();
    dsUDPTaskCreate();
}
```

That's a working board. From a client, you can now read/write registers, manage parameters, trigger resets, and auto-discover it on the LAN.

### 4. Try it from Python

```python
import socket, struct

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.1.100', 2009))

# Read user register at address 0 — type(2) + addr(2) + value(4)
sock.send(struct.pack('<HHI', 1, 0, 0))     # type 1 = READ_REGISTER
status, addr, value = struct.unpack('<hHI', sock.recv(8))
print(f"Register 0 = {value}  (status {status})")
```

For full configuration and platform-specific setup, see **[USER_INTEGRATION_GUIDE.md](USER_INTEGRATION_GUIDE.md)**.

---

## Defining Your Registers and Parameters

This is the only real work you need to do. Create two headers and point the config at them. Check DSHub to make this much easier for yourself.

**`my_registers.h`** — live runtime state:
```c
#include "ds_default_config.h"

typedef struct PACKED {
    // Read-only (must come first)
    uint32_t STATUS_FLAGS;
    float    SENSOR_TEMP;
    uint32_t UPTIME;

    // Read/write
    uint32_t CONTROL_MODE;
    uint32_t OUTPUT_ENABLE;
} ds_register_names_t;

#define DS_REGISTERS_READ_ONLY_COUNT 3
```

**`my_parameters.h`** — persistent config:
```c
#include "ds_default_config.h"

typedef struct PACKED {
    uint32_t DEVICE_ID;
    uint32_t USES_DHCP;
    uint32_t IP_ADDR[4];
    float    CALIBRATION_OFFSET;
    uint32_t OPERATING_MODE;
} ds_parameter_names_t;
```

Then in `ds_app_config.h`:
```c
#define DS_USER_REGISTER_DEFINITIONS  "my_registers.h"
#define DS_USER_PARAMETER_DEFINITIONS "my_parameters.h"
```

Every field must be 32-bit aligned (`uint32_t`, `int32_t`, `float`, or arrays of these). The library auto-generates the address space — field position in the struct = register address.

Access them in your code by name:
```c
REGS.byName.SENSOR_TEMP = read_sensor();
if (PARS.byName.USES_DHCP) { ... }
```

---

## Protocol at a Glance

One fixed-size packet for everything. Little-endian, tightly packed.

### Request (8 bytes, 9 with CRC)
```
 0        2        4                      8
 +--------+--------+----------------------+------+
 |  type  |  addr  |      value (32-bit)  | CRC* |
 +--------+--------+----------------------+------+
    u16      u16              u32             u8*
```

### Response (8 bytes, 9 with CRC)
```
 0        2        4                      8
 +--------+--------+----------------------+------+
 | status |  addr  |      value (32-bit)  | CRC* |
 +--------+--------+----------------------+------+
    i16      u16              u32             u8*
```

*CRC is optional — toggle via `CRC8_ENABLE`. Recommended for UDP.*

### Request types

| Value | Type | Meaning |
|:---:|---|---|
| 0 | `SYS_COMMAND` | Run a system command (value field = command) |
| 1 | `READ_REGISTER` | Read user register by address |
| 2 | `WRITE_REGISTER` | Write user register by address |
| 3 | `READ_PARAMETER` | Read parameter by address |
| 4 | `WRITE_PARAMETER` | Write parameter by address |
| 5 | `CONTROL_INTERFACE` | Claim exclusive write access |
| 6 | `READ_SYSTEM_REGISTER` | Read a library-managed register |
| 7 | `WRITE_SYSTEM_REGISTER` | Write a library-managed register |
| 100+ | `USER_DEFINED_START` | Your own packet types |

### Response codes

Positive = success, negative = error.

| | Success | | Error |
|:---:|---|:---:|---|
| 0 | System command OK | -1 | System command error |
| 1 | Read register OK | -2 | Invalid input type |
| 2 | Write register OK | -3 | Packet size error |
| 3 | Read parameter OK | -4 | Address out of range |
| 4 | Write parameter OK | -5 | Permission denied |
| 5 | Control interface OK | -6 | Control interface error |
| | | -7 | Syntax error |
| | | -8 | Parameters error |
| | | -9 | CRC error |

### System commands

The library reserves values **65000–65535**. Everything below that is yours.

| Value | Built-in | Action |
|:---:|---|---|
| 65000 | `READ_FLASH` | Reload parameters from flash |
| 65001 | `WRITE_FLASH` | Save parameters to flash |
| 65002 | `RESET_FIRMWARE` | Hardware reset |

Add your own by handling the weak hook:
```c
typedef enum {
    cmd_CALIBRATE = 0,
    cmd_START,
    cmd_STOP,
} my_commands_t;

bool dsProcessUserSysCommand(const dsRxPacket *in, dsTxPacket *out)
{
    switch (in->contents.value) {
        case cmd_CALIBRATE:
            run_calibration();
            out->contents.status = SYS_COMMAND_OK_REPLY_VAL;
            return true;
        default:
            return false;   // library replies with error
    }
}
```

---

## Board Auto-Discovery

Clients find your board by broadcasting a UDP discovery packet. Boards answer with their identity. No hardcoded IPs, no mDNS dependency.

**Flow:**
```
Client  ──(UDP broadcast, 8 bytes)──▶  All boards on the LAN
Client  ◀──(UDP unicast, 44 bytes)──  Board A  (name, IP, ports, MAC)
Client  ◀──(UDP unicast, 44 bytes)──  Board B
...
```

Discovery uses the same UDP port as normal traffic — the UDP task auto-detects the magic number `0xDEADBEEF` and routes appropriately. Enabled by default; turn off with `DS_AUTO_DETECTION_ENABLE 0`.

**Python client:**
```python
import socket, struct

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.settimeout(2.0)

request = struct.pack('<I B 3s', 0xDEADBEEF, 0x01, b'\x00\x00\x00')
sock.sendto(request, ('<broadcast>', 2011))

while True:
    try:
        data, (ip, _) = sock.recvfrom(1024)
        magic, cmd, btype, fw, bid, _, tcp, udp, mac, _, name = \
            struct.unpack('<IBBHII HH 6s 2x 16s', data[:44])
        if magic == 0xDEADBEEF and cmd == 0x02:
            print(f"{name.rstrip(b'\\x00').decode()} @ {ip}  TCP:{tcp}  "
                  f"MAC:{':'.join(f'{b:02X}' for b in mac)}")
    except socket.timeout:
        break
```

Test without hardware using the mock server in `tools/`:
```bash
python3 tools/mock_esp32_discovery.py      # simulates a board
python3 tools/test_udp_receive.py          # passive UDP listener
python3 tools/test_discovery.py            # end-to-end client
```

---

## Configuration Reference

All options go in `ds_app_config.h`. Everything has a sensible default; override only what you need.

### Core

| Macro | Default | Description |
|---|---|---|
| `DS_PLATFORM` | `STM32` | `ESP32`, `STM32`, `RP2040`, `RP2350`, or `GENERIC_PLATFORM` |
| `DS_BOARD_NAME` | `"Datastream test"` | 16-char max, shown in discovery |
| `DS_BOARD_ID` | `99` | Unique per device |
| `DS_FIRMWARE_VERSION` | `0x0200` | Reported in discovery |
| `CRC8_ENABLE` | `0` | Add 1-byte CRC to every packet |
| `DS_STATS_ENABLE` | `1` | Track packet/error counts |
| `DS_AUTO_DETECTION_ENABLE` | `1` | UDP broadcast discovery |
| `DS_CLI_ENABLE` | `0` | Optional text-mode CLI over TCP |

### Network

| Macro | Default |
|---|---|
| `DS_TCP_PORT` / `DS_UDP_PORT` | `2009` / `2011` |
| `DS_TCP_TASK_PRIORITY` / `DS_UDP_TASK_PRIORITY` | `tskIDLE_PRIORITY + 6` |
| `DS_TCP_TASK_STACK_SIZE` / `DS_UDP_TASK_STACK_SIZE` | `2 * configMINIMAL_STACK_SIZE` |

### Custom definitions

| Macro | Purpose |
|---|---|
| `DS_USER_REGISTER_DEFINITIONS` | Path to your register struct header |
| `DS_USER_PARAMETER_DEFINITIONS` | Path to your parameter struct header |
| `DS_USER_TYPES_DEFINITIONS` | Path to a header with shared enums/typedefs |
| `DS_SUPPRESS_DEFAULT_WARNINGS` | Silence the "using defaults" reminder |

---

## Persistent Storage

Parameters live in RAM until you explicitly save them. To enable persistence, override two weak functions:

```c
void dsWriteParametersToFlash(ds_parameters_t *pars);
void dsReadParametersFromFlash(ds_parameters_t *pars);
```

**ESP32 (NVS):**
```c
void dsWriteParametersToFlash(ds_parameters_t *pars)
{
    nvs_handle_t h;
    nvs_open("datastream", NVS_READWRITE, &h);
    nvs_set_blob(h, "params", pars, sizeof(*pars));
    nvs_commit(h);
    nvs_close(h);
}
```

**STM32** — use the HAL flash driver to write into a reserved sector.

Save is triggered by system command `WRITE_FLASH` (65001); reload by `READ_FLASH` (65000).

---

## API Summary

### Lifecycle
```c
void dsInitialize(void);            // Call once, before task creation
void dsTCPTaskCreate(void);         // Start TCP server
void dsUDPTaskCreate(void);         // Start UDP server + discovery
```

### Direct access (from your code)
```c
// User registers — runtime data
uint32_t v = dsGetRegister(addr, &reply);
dsSetRegister(addr, value, &reply);

// Parameters — persistent config
uint32_t v = dsGetParameter(addr, &reply);
dsSetParameter(addr, value, &reply);

// System registers — library stats & counters
uint32_t v = dsGetSystemRegister(addr, &reply);

// Or just touch the globals directly
REGS.byName.OUTPUT_ENABLE = 1;
PARS.byName.CALIBRATION_OFFSET = 3.2f;
SYS_REGS.byName.COUNTER_1HZ++;
```

### Hooks (weak — override as needed)
```c
// Packet handling
bool dsProcessUserDefinedType(dsRxPacket *in, dsTxPacket *out, reply_t *reply);
bool dsProcessUserSysCommand(const dsRxPacket *in, dsTxPacket *out);
void dsErrorCallback(reply_t reply, dsRxPacket *in, dsTxPacket *out);

// React to reads/writes
void dsRegisterSetCallback(uint16_t addr, uint32_t oldVal, uint32_t newVal);
void dsRegisterGetCallback(uint16_t addr, uint32_t val);
void dsParameterSetCallback(uint16_t addr, uint32_t oldVal, uint32_t newVal);
void dsParameterGetCallback(uint16_t addr, uint32_t val);

// Persistence
void dsWriteParametersToFlash(ds_parameters_t *pars);
void dsReadParametersFromFlash(ds_parameters_t *pars);
void dsWriteBoardNameToFlash(const char *name);
void dsReadBoardNameFromFlash(char *name, size_t maxLen);

// Initialization
void dsInitializeRegisters(ds_registers_t *regs);
void dsSetParametersDefaults(ds_parameters_t *pars);
```

### System registers

Library-managed, accessed via the `*_SYSTEM_REGISTER` packet types:

| Register | Access | Meaning |
|---|---|---|
| `DS_PACKET_COUNT` | read-only | Total packets received |
| `DS_ERROR_COUNT` | read-only | Total errors |
| `CONTROL_INTERFACE` | read-only | Which task currently has write control |
| `COUNTER_1HZ` | read/write | Free 1 Hz counter — your app ticks it |

---

## Control Interface (Write Access)

Datastream enforces single-writer-at-a-time. A client must first claim control via a `CONTROL_INTERFACE` packet — only then can it write registers or parameters. TCP/UDP tasks self-register at startup.

If you spawn your own task that needs write access:
```c
TaskHandle_t t = xTaskGetCurrentTaskHandle();
dsRegisterControlTask(t, ds_control_TCP_DATASTREAM, "AppTask");
```

Built-in interface types:

| Type | Value |
|---|:---:|
| `ds_control_UNDECIDED` | 0 |
| `ds_control_TCP_DATASTREAM` | 1 |
| `ds_control_UDP_DATASTREAM` | 2 |
| `ds_control_USER_DEFINED_START` | 100 |
| `ds_control_TCP_CLI` | 101 |
| `ds_control_USB` | 102 |

Define your own with `#define MY_INTERFACE (ds_control_USER_DEFINED_START + N)`.

---

## Footprint

Measured on ARM Cortex-M4 with GCC:

| Configuration | Debug (-Og) | Release (-Os) |
|---|---:|---:|
| Core only | 5.6 KB | 3.9 KB |
| Core + TCP | 7.0 KB | 5.0 KB |
| Core + UDP (with discovery) | 7.4 KB | 5.2 KB |
| Full (TCP + UDP) | 8.8 KB | 6.4 KB |

**RAM:** ~641 B static + 2–4 KB per network task.

To shrink further:
- `-Os` saves ~29% over `-Og`
- `DS_AUTO_DETECTION_ENABLE 0` → saves ~600 B
- `DS_STATS_ENABLE 0` → saves ~200 B
- `CRC8_ENABLE 0` (default) → saves ~150 B

---

## Platform Notes

**ESP32** — uses ESP-IDF sockets and `freertos/`-prefixed headers. Parameters typically persist via NVS. MAC and IP are fetched from `esp_netif`.

**STM32** — uses FreeRTOS+TCP. Works with CubeMX-generated projects; tested on F4, F7, H7. Firmware reset calls `HAL_NVIC_SystemReset()`.

**RP2040 / RP2350** — uses FreeRTOS+TCP with the Pico SDK. Configure clocks and networking before `dsInitialize()`.

**Generic** — minimal abstraction; supply your own weak functions for MAC/IP retrieval and any platform specifics.

See [USER_INTEGRATION_GUIDE.md](USER_INTEGRATION_GUIDE.md) for detailed per-platform setup.

---

## License

Licensed under [PolyForm Noncommercial License 1.0.0](LICENSE).

- **Free** for personal, hobby, educational, and non-profit use.
- **Commercial use** requires a separate license — see [COMMERCIAL.md](COMMERCIAL.md).

```
SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
Copyright (c) 2026 Sofian Jafar
```

---

## Documentation

- [USER_INTEGRATION_GUIDE.md](USER_INTEGRATION_GUIDE.md) — full setup walkthroughs per platform
- [COMMERCIAL.md](COMMERCIAL.md) — commercial licensing terms
- API reference — generate with `doxygen Doxyfile`, open `docs/html/index.html`
