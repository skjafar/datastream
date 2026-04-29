# Datastream — User Integration Guide

> **Status:** v0.2.4

A full walkthrough for dropping the datastream library into an embedded project without modifying its source. For protocol details and API reference, see [README.md](README.md).

---

## Contents

1. [How It Fits Together](#how-it-fits-together)
2. [Quick Start (5 Minutes)](#quick-start-5-minutes)
3. [Platform Setup](#platform-setup)
   - [CMake / Pico SDK / Generic](#cmake--pico-sdk--generic)
   - [ESP-IDF](#esp-idf)
   - [STM32CubeIDE](#stm32cubeide)
4. [Configuring the Library](#configuring-the-library)
5. [Defining Registers](#defining-registers)
6. [Defining Parameters](#defining-parameters)
7. [System Registers](#system-registers)
8. [Flash Persistence](#flash-persistence)
9. [Custom System Commands](#custom-system-commands)
10. [Control Interface & Task Registration](#control-interface--task-registration)
11. [Custom Types and Callbacks](#custom-types-and-callbacks)
12. [Testing Your Setup](#testing-your-setup)
13. [Troubleshooting](#troubleshooting)
14. [Best Practices](#best-practices)

---

## How It Fits Together

Everything you add to a datastream-enabled project lives **outside** the library folder:

```
your_project/
├── <your source>
├── ds_app_config.h        ← master config (required if you want to override defaults)
├── my_registers.h         ← your runtime state   (optional — library has a minimal default)
├── my_parameters.h        ← your persistent data (optional — same)
└── datastream/            ← library source (never edit)
    ├── include/
    └── src/
```

The library discovers your configuration automatically through `__has_include`. You never include `datastream.h` with special flags; just put `ds_app_config.h` somewhere on the include path and the rest is handled.

**Three things you will likely always do:**
1. Create `ds_app_config.h` with your platform and board identity.
2. Declare your register and parameter structs.
3. Implement `dsReadParametersFromFlash()` / `dsWriteParametersToFlash()` for persistence.

Everything else is optional.

---

## Quick Start (5 Minutes)

### 1. Add the library

Pick the matching [platform setup](#platform-setup) section below. The rest of this Quick Start is platform-agnostic.

### 2. Create `ds_app_config.h`

```c
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

#define DS_PLATFORM     ESP32               // ESP32, STM32, RP2040, RP2350, GENERIC_PLATFORM
#define DS_BOARD_NAME   "MyDevice"
#define DS_BOARD_TYPE   1
#define DS_BOARD_ID     0x00000001

#define DS_TCP_PORT     2009
#define DS_UDP_PORT     2011

#define DS_AUTO_DETECTION_ENABLE  1
#define DS_STATS_ENABLE           1
#define CRC8_ENABLE               0

#endif
```

### 3. Initialize in your application

```c
#include "datastream.h"
#include "dsTCP.h"
#include "dsUDP.h"

void app_main(void)
{
    // Bring up the network stack first (WiFi / Ethernet / etc.), then:
    dsInitialize();
    dsTCPTaskCreate();
    dsUDPTaskCreate();
}
```

### 4. Flash and verify

The device will:
- Accept TCP connections on `DS_TCP_PORT`.
- Serve UDP requests on `DS_UDP_PORT`.
- Respond to broadcast discovery on the same UDP port.

Jump to [Testing Your Setup](#testing-your-setup) for a first end-to-end check.

---

## Platform Setup

### CMake / Pico SDK / Generic

A minimal CMake layout:

```
your_project/
├── CMakeLists.txt
├── src/main.c
├── include/ds_app_config.h
└── lib/datastream/          (library as a subdirectory)
```

```cmake
add_subdirectory(lib/datastream)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE datastream)
```

For RP2040 / RP2350, do this after `pico_sdk_init()` and link FreeRTOS+TCP as usual.

### ESP-IDF

Copy the library to `components/datastream/` and create a component build file:

```
your_project/
├── main/
│   ├── main.c
│   ├── ds_app_config.h
│   └── CMakeLists.txt
└── components/
    └── datastream/
        ├── CMakeLists.txt      (create this)
        ├── include/
        └── src/
```

`components/datastream/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS
        "src/datastream.c"
        "src/dsRegisters.c"
        "src/dsParameters.c"
        "src/dsTCP.c"
        "src/dsUDP.c"
        # "src/dsCLI.c"      # add if DS_CLI_ENABLE is set
    INCLUDE_DIRS "include"
    PRIV_REQUIRES freertos esp_netif lwip
)
```

`main/CMakeLists.txt` — make sure the directory holding `ds_app_config.h` is on the include path so the library can find it:
```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES datastream
)
```

Initialize **after** the network stack has an IP:
```c
wifi_init_and_connect();
wait_for_ip();

dsInitialize();
dsTCPTaskCreate();
dsUDPTaskCreate();
```

### STM32CubeIDE

1. Copy the library to `Middlewares/Third_Party/datastream/`.
2. In the CubeIDE Project Explorer, right-click the project → **Refresh**.
3. **Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include Paths.** Add (for both Debug and Release):
   ```
   ../Middlewares/Third_Party/datastream/include
   ```
4. If any `.c` under `Middlewares/Third_Party/datastream/src/` shows an "excluded from build" badge, right-click it → **Resource Configurations → Exclude from build** → uncheck both configurations.
5. Create `Core/Inc/ds_app_config.h` (`Core/Inc` is already on the default include path, so the library will find it automatically):
   ```c
   #define DS_PLATFORM   STM32
   #define DS_BOARD_NAME "My STM32 Board"
   ```
6. Make sure **FreeRTOS+TCP** is enabled and configured in the project — the STM32 platform relies on it for sockets (`FreeRTOS_IP.h`, `FreeRTOS_Sockets.h`).
7. Initialize from `main.c` after peripheral init:
   ```c
   HAL_Init();
   SystemClock_Config();
   MX_GPIO_Init();
   MX_ETH_Init();

   dsInitialize();
   dsTCPTaskCreate();
   dsUDPTaskCreate();

   vTaskStartScheduler();
   ```

> **IntelliSense warnings about `uint32_t`** in library headers are indexer artefacts — the build itself will still succeed.

---

## Configuring the Library

All configuration flows through `ds_app_config.h`. Every macro has a default; set only what you need.

### Core identity and protocol

| Macro | Default | Purpose |
|---|---|---|
| `DS_PLATFORM` | `STM32` | `ESP32`, `STM32`, `RP2040`, `RP2350`, `GENERIC_PLATFORM` |
| `DS_BOARD_NAME` | `"Datastream test"` | Reported in discovery (max 16 chars) |
| `DS_BOARD_TYPE` | `1` | Application-defined category |
| `DS_BOARD_ID` | `99` | Unique per device |
| `DS_FIRMWARE_VERSION` | `0x0200` | Reported in discovery |
| `CRC8_ENABLE` | `0` | Append 1-byte CRC to every packet |
| `DS_STATS_ENABLE` | `1` | Maintain `DS_PACKET_COUNT` and `DS_ERROR_COUNT` |
| `DS_AUTO_DETECTION_ENABLE` | `1` | UDP broadcast discovery support |
| `DS_CLI_ENABLE` | `0` | Optional text CLI over TCP |

### Network and tasks

| Macro | Default |
|---|---|
| `DS_TCP_PORT` / `DS_UDP_PORT` | `2009` / `2011` |
| `DS_TCP_TASK_PRIORITY` / `DS_UDP_TASK_PRIORITY` | `tskIDLE_PRIORITY + 6` |
| `DS_TCP_TASK_STACK_SIZE` / `DS_UDP_TASK_STACK_SIZE` | `2 * configMINIMAL_STACK_SIZE` |
| `DS_MAX_REGISTERED_TASKS` | `8` |

### Custom definition files

| Macro | Purpose |
|---|---|
| `DS_USER_REGISTER_DEFINITIONS` | Path to your register struct header |
| `DS_USER_PARAMETER_DEFINITIONS` | Path to your parameter struct header |
| `DS_USER_TYPES_DEFINITIONS` | Path to your shared enums/typedefs |

### Alternative configuration methods

If you prefer flags over a header:

```cmake
target_compile_definitions(your_target PRIVATE
    DS_PLATFORM=ESP32
    DS_USER_REGISTER_DEFINITIONS="my_registers.h"
    DS_BOARD_NAME="My Board"
)
```

```makefile
CFLAGS += -DDS_PLATFORM=ESP32
CFLAGS += -DDS_USER_REGISTER_DEFINITIONS=\"my_registers.h\"
```

### Toggling auto-detection of `ds_app_config.h`

| Macro | Effect |
|---|---|
| `DS_USE_APP_CONFIG` | Force-include (for compilers without `__has_include`) |
| `DS_NO_APP_CONFIG` | Skip include — rely only on build-system defines |
| `DS_SHOW_CONFIG_INFO` | Print a compile-time message saying whether the file was found |
| `DS_SUPPRESS_DEFAULT_WARNINGS` | Silence the "using defaults" informational pragmas |

---

## Defining Registers

**Registers** are the runtime face of your device — sensor readings, status flags, outputs. They live in RAM and are accessed via `READ_REGISTER` / `WRITE_REGISTER` packets.

> System registers (packet counts, control interface, 1 Hz counter) are separate; see [System Registers](#system-registers). Do not include them in your user register struct.

### Template — `my_registers.h`

```c
#ifndef MY_REGISTERS_H_
#define MY_REGISTERS_H_

#include "ds_default_config.h"

typedef struct PACKED
{
    /* ---- read-only (must come first) ---- */
    uint32_t STATUS_FLAGS;
    float    SENSOR_READING;
    uint32_t SYSTEM_UPTIME;

    /* ---- read/write ---- */
    uint32_t CONTROL_MODE;
    uint32_t OUTPUT_ENABLE;
    float    SETPOINT;

} ds_register_names_t;

DS_STATIC_ASSERT(sizeof(ds_register_names_t) % 4 == 0,
                 "Register struct must be 4-byte aligned");

#define DS_REGISTERS_BY_NAME_T_SIZE   sizeof(ds_register_names_t)
#define DS_REGISTER_COUNT             (DS_REGISTERS_BY_NAME_T_SIZE / sizeof(uint32_t))
#define DS_REGISTERS_READ_ONLY_COUNT  3    /* number of read-only fields above */

#endif
```

### Rules

- **Every field is 32-bit** (`uint32_t`, `int32_t`, `float`, or a 32-bit-sized enum/typedef).
- **Read-only fields come first**, and `DS_REGISTERS_READ_ONLY_COUNT` must match.
- **Arrays count as multiple registers** — `uint32_t x[4]` occupies four addresses.
- **At least one field is required** to avoid a zero-size struct.

### Accessing from your code

```c
REGS.byName.STATUS_FLAGS = 0x01;
REGS.byName.SENSOR_READING = read_temp();

uint32_t mode = REGS.byName.CONTROL_MODE;
```

Direct access skips permission checks — use the packet API (or `dsSetRegister()`) only when you want the same gating a network client would see.

---

## Defining Parameters

**Parameters** are configuration values that must survive reboots — calibration offsets, network settings, operating mode. Access from the wire uses `READ_PARAMETER` / `WRITE_PARAMETER`. Persistence is your responsibility (see [Flash Persistence](#flash-persistence)).

### Template — `my_parameters.h`

```c
#ifndef MY_PARAMETERS_H_
#define MY_PARAMETERS_H_

#include "ds_default_config.h"

typedef struct PACKED
{
    /* Identity */
    uint32_t DEVICE_ID;

    /* Network */
    uint32_t USES_DHCP;
    uint32_t IP_ADDR[4];
    uint32_t GATEWAY_ADDR[4];
    uint32_t NET_MASK[4];
    uint32_t MAC_ADDR[6];

    /* Application */
    float    CALIBRATION_OFFSET;
    float    CALIBRATION_SCALE;
    uint32_t OPERATING_MODE;
    uint32_t ALARM_THRESHOLD;

    /* Optional bookkeeping — useful in your flash driver */
    uint32_t PARAMETERS_SETS_IN_FLASH;
    uint32_t PARAMETERS_INITIALIZATION_MARKER;

} ds_parameter_names_t;

DS_STATIC_ASSERT(sizeof(ds_parameter_names_t) % 4 == 0,
                 "Parameter struct must be 4-byte aligned");

#define DS_PARAMETERS_T_SIZE   sizeof(ds_parameter_names_t)
#define DS_PARAMETER_COUNT     (DS_PARAMETERS_T_SIZE / sizeof(uint32_t))

#endif
```

### Rules

- All fields are 32-bit aligned (same rules as registers).
- Parameters are loaded once at boot via `dsReadParametersFromFlash()` and saved on `WRITE_FLASH`.
- Defaults can be supplied by overriding `dsSetParametersDefaults()`.

### Accessing from your code

```c
float cal = PARS.byName.CALIBRATION_OFFSET;
PARS.byName.OPERATING_MODE = MODE_ACTIVE;
```

---

## System Registers

System registers are owned by the library (struct `SYS_REGS`, accessed via packet types `READ_SYSTEM_REGISTER` / `WRITE_SYSTEM_REGISTER`). They are always present and their layout is fixed.

| Register | Access | Meaning |
|---|---|---|
| `DS_PACKET_COUNT` | read-only | Packets received (only if `DS_STATS_ENABLE`) |
| `DS_ERROR_COUNT` | read-only | Errors recorded (only if `DS_STATS_ENABLE`) |
| `CONTROL_INTERFACE` | read-only | `ds_control_interface_t` of the current writer |
| `COUNTER_1HZ` | read / write | Free 1 Hz counter — incremented by your app |

```c
SYS_REGS.byName.COUNTER_1HZ++;                   // tick from your timer task
uint32_t pkts = SYS_REGS.byName.DS_PACKET_COUNT;
```

Override the weak initialiser if you need custom starting values:

```c
void dsInitializeSystemRegisters(ds_system_registers_t *sysRegs)
{
    sysRegs->byName.COUNTER_1HZ = 0;
    /* DS_PACKET_COUNT / DS_ERROR_COUNT are already zeroed */
}
```

---

## Flash Persistence

The library provides **weak** hooks for parameter storage — if you don't override them, parameters are lost at power-off.

```c
/* Called on WRITE_FLASH and when defaults are applied */
void dsWriteParametersToFlash(ds_parameters_t *pars);

/* Called during dsInitialize() */
void dsReadParametersFromFlash(ds_parameters_t *pars);
```

Same pattern for the board name, if you want `dsSetBoardName()` to persist:

```c
void dsWriteBoardNameToFlash(const char *name);
void dsReadBoardNameFromFlash(char *name, size_t maxLen);
```

### ESP32 (NVS)

```c
#include "nvs_flash.h"
#include "nvs.h"
#include "dsParameters.h"

void dsWriteParametersToFlash(ds_parameters_t *pars)
{
    nvs_handle_t h;
    if (nvs_open("datastream", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, "params", pars, sizeof(*pars));
    nvs_commit(h);
    nvs_close(h);
}

void dsReadParametersFromFlash(ds_parameters_t *pars)
{
    nvs_handle_t h;
    size_t size = sizeof(*pars);
    if (nvs_open("datastream", NVS_READONLY, &h) != ESP_OK) return;
    nvs_get_blob(h, "params", pars, &size);
    nvs_close(h);
}
```

### STM32 (internal flash sector)

```c
#define FLASH_USER_START_ADDR  0x080E0000U   /* example — pick a reserved sector */

void dsWriteParametersToFlash(ds_parameters_t *pars)
{
    FLASH_EraseInitTypeDef erase = { /* configure for your sector */ };
    uint32_t err = 0;

    HAL_FLASH_Unlock();
    HAL_FLASHEx_Erase(&erase, &err);

    const uint64_t *src = (const uint64_t *)pars;
    uint32_t addr = FLASH_USER_START_ADDR;
    for (size_t i = 0; i < sizeof(*pars) / 8; ++i, addr += 8) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, src[i]);
    }
    HAL_FLASH_Lock();
}

void dsReadParametersFromFlash(ds_parameters_t *pars)
{
    memcpy(pars, (void *)FLASH_USER_START_ADDR, sizeof(*pars));
}
```

### RP2040 / RP2350

Use `pico/flash.h` (`flash_range_erase` + `flash_range_program`) from a core that isn't running XIP-sensitive code, or write through a filesystem such as LittleFS.

---

## Custom System Commands

The library reserves system command values **65000–65535**. Everything below is yours.

```c
typedef enum {
    cmd_START     = 0,
    cmd_STOP      = 1,
    cmd_TRIGGER   = 2,
    cmd_CALIBRATE = 3,
} my_sys_commands_t;

bool dsProcessUserSysCommand(const dsRxPacket *in, dsTxPacket *out)
{
    switch (in->contents.value) {
        case cmd_START:
            start_system();
            out->contents.status = SYS_COMMAND_OK_REPLY_VAL;
            return true;

        case cmd_CALIBRATE:
            if (!calibrate()) return false;      /* library returns error */
            out->contents.status = SYS_COMMAND_OK_REPLY_VAL;
            return true;

        default:
            return false;                         /* unknown → library returns error */
    }
}
```

Return `true` when you've handled the command (and set the reply status), `false` to let the library reply with a system-command error.

---

## Control Interface & Task Registration

Datastream enforces single-writer semantics: a task must first be registered as a control interface before its writes are accepted. TCP and UDP tasks register themselves at startup — you usually don't have to think about this.

### When to register manually

If an application task of yours needs write access (a CLI, USB handler, local state machine, etc.):

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
| `ds_control_TCP_CLI` | 101 |
| `ds_control_USB` | 102 |
| `ds_control_USER_DEFINED_START` | 100 |

Define your own:
```c
#define MY_INTERFACE (ds_control_USER_DEFINED_START + 10)
```

### Custom permission check

Override the weak predicate to gate writes however you like:

```c
bool dsCheckTaskWritePermission(void)
{
    TaskHandle_t t = xTaskGetCurrentTaskHandle();
    ds_control_interface_t type = dsGetTaskControlType(t);
    return (type == ds_control_TCP_DATASTREAM || type == ds_control_USB);
}
```

Increase the registration limit via `DS_MAX_REGISTERED_TASKS` (default `8`).

---

## Custom Types and Callbacks

### Shared types

If your register or parameter structs use enums, put them in a separate header and point `ds_app_config.h` at it:

```c
/* my_types.h */
typedef enum {
    MODE_STANDBY = 0,
    MODE_ACTIVE  = 1,
    MODE_ERROR   = 2,
} operating_mode_t;

DS_STATIC_ASSERT(sizeof(operating_mode_t) == sizeof(uint32_t),
                 "operating_mode_t must be 32-bit");
```

```c
/* ds_app_config.h */
#define DS_USER_TYPES_DEFINITIONS "my_types.h"
```

### React to reads and writes

```c
void dsRegisterSetCallback(uint16_t addr, uint32_t oldVal, uint32_t newVal);
void dsRegisterGetCallback(uint16_t addr, uint32_t val);
void dsParameterSetCallback(uint16_t addr, uint32_t oldVal, uint32_t newVal);
void dsParameterGetCallback(uint16_t addr, uint32_t val);
```

Useful for triggering side effects (e.g. reconfigure a PWM when `SETPOINT` changes).

### Packet-level hooks

```c
bool dsProcessUserDefinedType(dsRxPacket *in, dsTxPacket *out, reply_t *reply);
void dsErrorCallback(reply_t reply, dsRxPacket *in, dsTxPacket *out);
```

Use `dsProcessUserDefinedType` to implement packet types at or above `USER_DEFINED_START` (100+).

### Logging

**ESP32** — set `ESP32_LOGGING_ENABLE 1` in `ds_app_config.h`; `DS_LOGI` / `DS_LOGE` route through `esp_log`.

**Other platforms** — define the macros yourself:
```c
#define DS_LOGI(fmt, ...) printf("[INFO] "  fmt "\n", ##__VA_ARGS__)
#define DS_LOGE(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
```

---

## Testing Your Setup

Everything below works against either real hardware or the mock server in `tools/`.

### With no hardware — mock server

```bash
python3 tools/mock_esp32_discovery.py
```
```
Mock ESP32 Discovery Server
Listening on 0.0.0.0:2011
Board: MockESP32-Test, Type: 1, Serial: 0x12345678
Waiting for discovery requests…
```

### Passive UDP listener

```bash
python3 tools/test_udp_receive.py
```

### Discovery one-liner

```python
import socket, struct

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.settimeout(2.0)

sock.sendto(struct.pack('<I B 3s', 0xDEADBEEF, 0x01, b'\x00\x00\x00'),
            ('<broadcast>', 2011))

try:
    data, addr = sock.recvfrom(1024)
    print(f"Found device at {addr[0]}")
except socket.timeout:
    print("No devices found")
```

### TCP round-trip (read a parameter)

Remember the packet layout changed in v0.2.2: **type and address are now 16-bit.**

```python
import socket, struct

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.1.50', 2009))

# Request: type(u16) + addr(u16) + value(u32). type 3 = READ_PARAMETER
sock.send(struct.pack('<HHI', 3, 0, 0))

# Response: status(i16) + addr(u16) + value(u32)
status, addr, value = struct.unpack('<hHI', sock.recv(8))
print(f"Parameter 0 = {value}   (status: {status})")
```

### Symptom checklist

| Symptom | Likely cause | Fix |
|---|---|---|
| No discovery response | UDP blocked / wrong port | Check firewall, confirm `DS_UDP_PORT` |
| TCP connection refused | Task never started | Verify `dsTCPTaskCreate()` ran after `dsInitialize()` |
| Client receives garbage | Protocol mismatch | Use `<HHI` packing (16-bit type/addr) — not `<BB I` |
| Writes return `-5` (permission) | No control interface claimed | Send a `CONTROL_INTERFACE` packet first, or register your task |
| Parameters lost on reboot | Flash hooks not overridden | Implement `dsWriteParametersToFlash()` / `dsReadParametersFromFlash()` |

---

## Troubleshooting

### Compiler can't find custom headers
- Confirm the path in `DS_USER_REGISTER_DEFINITIONS` / `DS_USER_PARAMETER_DEFINITIONS` is resolvable from the build's include roots.
- Double-check that the file is actually added to the build (common issue in CubeIDE).

### Static assertion failures
- Every register/parameter field must be 32-bit — no `uint16_t`, no `bool`, no unpadded structs.
- Check `DS_REGISTERS_READ_ONLY_COUNT` matches the number of read-only fields.

### Linker errors
- Ensure all required `.c` files are listed — in ESP-IDF, check the component's `SRCS` list.
- If `DS_CLI_ENABLE` is set, add `src/dsCLI.c`.

### Verifying configuration at compile time
Add to `ds_app_config.h`:
```c
#define DS_SHOW_CONFIG_INFO
```
You will see one of:
```
DATASTREAM INFO: ds_app_config.h detected and included
DATASTREAM INFO: ds_app_config.h not found, using defaults
```

### Default-definition warnings
```
DATASTREAM: Using default register definitions. Define DS_USER_REGISTER_DEFINITIONS …
```
Either provide your own headers or add `#define DS_SUPPRESS_DEFAULT_WARNINGS`.

### Include-order issues
Always include `datastream.h` before `dsTCP.h` or `dsUDP.h`.

### STM32CubeIDE: phantom `uint32_t is not a type name`
IntelliSense/indexer artefact — the real build succeeds. Regenerate the index (Project → C/C++ Index → Rebuild) if it bothers you.

### Auto-detection not answering
- `DS_AUTO_DETECTION_ENABLE 1`
- `dsUDPTaskCreate()` called
- Board identity defined in `ds_app_config.h`
- Client reaches the board's broadcast domain (routers drop broadcasts)

---

## Best Practices

1. **`dsInitialize()` first, always** — before creating TCP/UDP tasks or registering your own.
2. **Never edit library files.** Every override lives in your project (`ds_app_config.h`, weak hooks, custom definition headers).
3. **Override the flash hooks.** Parameters are meaningless without them.
4. **Version-control your config.** `ds_app_config.h` and your register/parameter headers are *application* code — keep them in your repo, not vendored alongside the library.
5. **Start from `examples/`.** Copy `ds_app_config_example.h`, `ds_app_register_names_example.h`, `ds_app_parameter_names_example.h` and adapt — don't edit the examples in place.
6. **Use defaults when you can.** The library's sensible defaults keep `ds_app_config.h` short and your intent clear.

---

## Support

- **Protocol & API reference** — [README.md](README.md)
- **Generated API docs** — `doxygen Doxyfile`, then open `docs/html/index.html`
- **Helper scripts** — `tools/mock_esp32_discovery.py`, `tools/test_udp_receive.py`, `tools/test_discovery.py`
- **Commercial licensing** — [COMMERCIAL.md](COMMERCIAL.md)
