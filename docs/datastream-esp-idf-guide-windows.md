# Datastream on ESP-IDF — Step-by-step Setup Guide (Windows)

This is the Windows / PowerShell counterpart to
[datastream-esp-idf-guide.md](datastream-esp-idf-guide.md). Section
numbers match the Linux/macOS guide, so a comment like "see step 9"
points to the same topic in either document.

By the end you will have an ESP32-S3 board that:

- connects to your Wi-Fi network,
- accepts datastream TCP requests on port `2009`,
- responds to UDP discovery on port `2011`.

> **Using WSL?** Follow the [main Linux/macOS guide](datastream-esp-idf-guide.md)
> from inside your WSL distro instead — every command works as written.
> The only WSL-specific extra is exposing the USB-serial device with
> [`usbipd-win`](https://learn.microsoft.com/windows/wsl/connect-usb)
> before `idf.py flash`. The rest of *this* document is for native
> Windows installations.

---

## 1. Prerequisites

| Tool | Version |
|---|---|
| ESP-IDF | 5.x — install via the [ESP-IDF Tools Installer](https://dl.espressif.com/dl/esp-idf/) or [EIM](https://github.com/espressif/idf-im-ui) |
| PowerShell | 5.1 (built into Windows 10/11) or PowerShell 7+ |
| Python | 3.10+ — bundled by the ESP-IDF installer |
| Git | bundled by the ESP-IDF installer, or [Git for Windows](https://git-scm.com/download/win) |
| VS Code + ESP-IDF extension | optional, for the IDE workflow |

After install, confirm the environment works. Either:

**Option A — use the Start-Menu shortcut.** Open **ESP-IDF X.Y PowerShell**
from the Start Menu. The window starts with `IDF_PATH` already exported.

**Option B — source `export.ps1` in your own PowerShell:**

```powershell
& "C:\Espressif\frameworks\esp-idf-vX.Y\export.ps1"
idf.py --version
```

(Replace `X.Y` with your installed version, e.g. `v5.5.2`.)

> **PowerShell refuses to run `export.ps1`?** Set the execution policy
> for your user:
> ```powershell
> Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
> ```

---

## 2. Bootstrap from a stock ESP-IDF example

Datastream needs the network stack to be up before it starts its tasks,
so the easiest starting point is an example that already pulls in
`protocol_examples_common`. `examples/protocols/http_request` is a small,
well-known one.

### PowerShell

```powershell
$proj = "$env:USERPROFILE\esp\projects\my_project"
New-Item -ItemType Directory -Force -Path $proj | Out-Null
Copy-Item "$env:IDF_PATH\examples\protocols\http_request\*" -Destination $proj -Recurse
Set-Location $proj

# Trim files we don't need from the example
Remove-Item pytest_*.py, sdkconfig.ci, README.md -ErrorAction SilentlyContinue
```

### VS Code (ESP-IDF extension)

1. Open the Command Palette → **ESP-IDF: Show Examples Projects**.
2. Pick the IDF version that came up.
3. Browse to **protocols → http_request** and click **Create project using example**.
4. Choose your destination folder.

Either way, you now have a working IDF project:

```
my_project\
├── CMakeLists.txt
└── main\
    ├── CMakeLists.txt
    ├── http_request_example_main.c
    └── idf_component.yml          # pulls protocol_examples_common
```

> **Keep the path short.** Windows builds occasionally hit `MAX_PATH`
> (260-char) limits deep inside `build\`. `C:\esp\proj` is safer than
> `C:\Users\YourName\OneDrive\Documents\Projects\esp\my_project`.

---

## 3. Pick your target chip

```powershell
idf.py set-target esp32s3
```

For other chips: `esp32`, `esp32c3`, `esp32c6`, `esp32s2`, `esp32p4`, etc.

In VS Code: bottom status bar → **ESP-IDF Target** → choose the chip.

---

## 4. Verify the baseline builds

Before you change anything, confirm the toolchain works on this project:

```powershell
idf.py build
```

You should get `Project build complete.` and a `build\http_request.bin`.
If this fails, fix the environment first — adding datastream on top of a
broken baseline only makes diagnosis harder.

---

## 5. Initialise git and add datastream as a submodule

```powershell
git init
git submodule add https://github.com/skjafar/datastream.git components/datastream
```

Why a submodule and not a copy?
- The version is pinned in `.gitmodules` — your build is reproducible.
- `git submodule update --remote` upgrades when you're ready.
- You never edit the library — your code stays in `main\`.

The library ships an ESP-IDF-aware `components\datastream\CMakeLists.txt`,
so **you do not need to write one**. ESP-IDF's component manager picks it
up automatically because it lives under `components\`.

> **`fatal: Filename too long` on submodule add?** Enable Git's long-path
> support (one-time, system-wide):
> ```powershell
> git config --global core.longpaths true
> ```

---

## 6. Create `main\ds_app_config.h`

The library auto-includes `ds_app_config.h` from anywhere on the include
path via `__has_include`. Putting it in `main\` (which already has
`INCLUDE_DIRS "."`) is the simplest way to make it visible.

`main\ds_app_config.h`:

```c
#ifndef DS_APP_CONFIG_H_
#define DS_APP_CONFIG_H_

#define DS_PLATFORM             ESP32
#define DS_BOARD_NAME           "DS-S3-Demo"        // shows up in discovery (max 16 chars)
#define DS_BOARD_TYPE           1
#define DS_BOARD_ID             0x00000001
#define DS_FIRMWARE_VERSION     0x0100

#define DS_TCP_PORT             2009
#define DS_UDP_PORT             2011

#define DS_AUTO_DETECTION_ENABLE  1
#define DS_STATS_ENABLE           1
#define CRC8_ENABLE               0

#define ESP32_LOGGING_ENABLE      1   // route DS_LOGI/DS_LOGE through esp_log

// Use the library's minimal default register/parameter layouts for the first
// build. Provide your own headers later via DS_USER_REGISTER_DEFINITIONS /
// DS_USER_PARAMETER_DEFINITIONS — see USER_INTEGRATION_GUIDE.md.
#define DS_SUPPRESS_DEFAULT_WARNINGS

#endif
```

> Templates for fuller configs and custom register/parameter headers are
> in `components\datastream\examples\`.

---

## 7. Replace `main.c`

Drop the HTTP example's networking-then-fetch logic and replace it with
the datastream init sequence. Datastream **must** be started after the
network stack has an IP — `example_connect()` blocks until that happens,
so we just call it first.

Delete the old file and write `main\main.c`:

```powershell
Remove-Item main\http_request_example_main.c
```

```c
// main/main.c
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "datastream.h"
#include "dsTCP.h"
#include "dsUDP.h"

static const char *TAG = "ds_demo";

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    ESP_LOGI(TAG, "network up, starting datastream");
    dsInitialize();
    dsTCPTaskCreate();
    dsUDPTaskCreate();
}
```

---

## 8. Update `main\CMakeLists.txt`

Tell ESP-IDF that `main` now uses datastream and the standard system
components it includes directly:

```cmake
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES datastream nvs_flash esp_netif esp_event)
```

Also rename the project in the top-level `CMakeLists.txt` so the
resulting binary has a sensible name:

```cmake
project(datastream_demo)
```

---

## 9. Configure Wi-Fi credentials

Open menuconfig:

```powershell
idf.py menuconfig
```

Navigate to **Example Connection Configuration** and set:

- **WiFi SSID** — your network name
- **WiFi Password** — your password

Save (`S`), exit (`Q`).

If your router uses **WPA3**, also drill into the same menu and set
**WiFi Scan Authentication Mode Threshold** to `WPA3 PSK`.

In VS Code: status bar → **SDK Configuration Editor (menuconfig)** → same
section.

---

## 10. Build, flash, monitor

First, find the board's COM port. Plug it in and check
**Device Manager → Ports (COM & LPT)** for an entry like:

- `Silicon Labs CP210x USB to UART Bridge (COM3)` — most ESP devkits
- `USB-SERIAL CH340 (COM4)` — common on clone boards
- `USB Serial Device (COM5)` — ESP32-S3 native USB-Serial/JTAG

If no COM port appears, install the bridge driver:
- [Silicon Labs CP210x driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- [WCH CH340 driver](https://www.wch-ic.com/downloads/CH341SER_EXE.html)

ESP32-S3 native USB doesn't need an extra driver on Windows 10/11.

Then build, flash, and open the serial monitor:

```powershell
idf.py build
idf.py -p COM3 flash monitor
```

Replace `COM3` with whatever Device Manager showed.

Successful boot looks roughly like:

```
I (5234) example_connect: Got IPv4 event: Interface "example_netif_sta" address: 192.168.x.y
I (5244) ds_demo: network up, starting datastream
```

Exit `idf.py monitor` with `Ctrl-]`.

In VS Code: use the **flame** (Build), **bolt** (Flash), and **monitor**
icons in the bottom status bar; pick the COM port from the bottom-left
status item.

---

## 11. Verify the device is reachable

From any machine on the same LAN, with the datastream repo checked out:

```powershell
Set-Location components\datastream
python tools\test_discovery.py
```

You should see your board respond with `DS_BOARD_NAME` and its IP. To
poke a parameter directly with no extra tooling:

```python
import socket, struct

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.x.y', 2009))            # board IP from monitor

# READ_PARAMETER (type=3) at address 0
sock.send(struct.pack('<HHI', 3, 0, 0))
status, addr, value = struct.unpack('<hHI', sock.recv(8))
print(f"param[0] = {value}  status={status}")
```

> On Windows the Python launcher is `python`, not `python3`.

---

## 12. Common pitfalls (Windows-specific additions)

| Symptom | Cause | Fix |
|---|---|---|
| `& : ... cannot be loaded because running scripts is disabled` when running `export.ps1` | Default PowerShell execution policy | `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` |
| `fatal: Filename too long` cloning the submodule, or odd CMake "file not found" deep in `build\` | Windows MAX_PATH | `git config --global core.longpaths true`; set registry `HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnabled = 1`; keep the project at a short path like `C:\esp\proj` |
| Build is slow, or CMake re-runs every time | Project sits in OneDrive/Dropbox/iCloud | Move the project out of synced folders |
| `xtensa-esp32s3-elf-gcc.exe` missing or quarantined | Antivirus | Whitelist `C:\Espressif\tools\` |
| No COM port in Device Manager | Missing USB-serial driver | Install CP210x or CH340 driver from the vendor links above |
| `idf.py flash` fails with "Failed to connect" | Wrong COM port, board needs manual bootloader entry, or driver not loaded | Hold BOOT, tap RESET, retry; double-check the COM number |
| `nvs_flash.h file not found` during build | `main\CMakeLists.txt` REQUIRES list incomplete | Add `nvs_flash esp_netif esp_event` to REQUIRES |
| `dsInitialize` etc. linker errors | Datastream submodule not initialised | `git submodule update --init --recursive` after a fresh clone |
| Board boots but never reaches `dsInitialize` log | Wrong SSID/PSK or wrong auth threshold | Re-run menuconfig; for WPA3-only routers, set the auth threshold to `WPA3 PSK` |
| Build succeeds but device is silent on the network | `dsTCPTaskCreate()`/`dsUDPTaskCreate()` not called, or called before network up | Order must be: connect → `dsInitialize` → `dsTCPTaskCreate` → `dsUDPTaskCreate` |
| Discovery (`test_discovery.py`) finds nothing | Router blocking UDP broadcast, host on a different subnet, or Windows Firewall blocking | Test on the same subnet first; allow `python.exe` through Windows Firewall when prompted |

---

## 13. Updating the library

```powershell
Set-Location components\datastream
git fetch
git checkout <commit-or-tag>
Set-Location ..\..
git add components\datastream
git commit -m "Bump datastream to <version>"
```

To always track upstream `main`:

```powershell
git submodule update --remote components\datastream
```

---

## 14. Walkthrough — read 4 ADC channels and view them in DSHub

Up to this point the project uses the library's *minimal* default register
layout — fine for a smoke test, but every real device needs custom
registers. This section walks through wiring up four ADC1 channels and
exposing them as datastream registers that
[DSHub](https://github.com/skjafar/dshub) can read and chart in real time.

The code is portable across every ESP32 variant: ADC1 channels 0–3 exist
on all of them (only the GPIO pin numbers differ — see the table at the
end). We deliberately skip the DAC because the ESP32-S3 and the RISC-V
chips don't have one — keeping the example chip-agnostic matters more
than DAC coverage.

### 14.1 Define the register map in DSHub and export the header

In DSHub:

1. Open the **Map editor** and create a new map (e.g. `ds_s3_demo.map`).
2. Add the following fields, in this order — read-only first:
   - `ADC_RAW`             (uint32 array, length 4, read-only)
   - `SAMPLE_COUNT`        (uint32, read-only)
   - `SAMPLING_ENABLED`    (uint32, read/write)
   - `SAMPLE_INTERVAL_MS`  (uint32, read/write)

   The array entry occupies 4 consecutive register addresses
   (`ADC_RAW[0]`…`ADC_RAW[3]`) — datastream addresses are per-uint32, so
   one array of length 4 reads exactly the same on the wire as four
   scalars would.
3. Use DSHub's **Export → C Header** action to generate `my_registers.h`.
4. Save the `.map` file under DSHub's `client\public\maps\` so it can
   load it when you connect to the device.

> Defining the layout in the UI is optional — you can hand-write
> `my_registers.h` directly. The map file is what DSHub uses to label
> values; the C header is what the firmware compiles against.

### 14.2 Expected `main\my_registers.h`

For sanity-checking what DSHub exports, here is the exact header that
matches the code in [§ 14.4](#144-sample-the-adc-in-mainc):

```c
#ifndef MY_REGISTERS_H_
#define MY_REGISTERS_H_

#include <stdint.h>

typedef struct PACKED
{
    /***** read only *****/
    uint32_t  ADC_RAW[4];
    uint32_t  SAMPLE_COUNT;

    /***** read/write *****/
    uint32_t  SAMPLING_ENABLED;
    uint32_t  SAMPLE_INTERVAL_MS;

} ds_register_names_t;

#define DS_REGISTERS_READ_ONLY_COUNT    5

#endif
```

Rules the file must follow (the library's static asserts will catch you
if it doesn't):
- Every field is 32-bit (`uint32_t`, `int32_t`, `float`).
- Read-only fields come first.
- `DS_REGISTERS_READ_ONLY_COUNT` matches the count of read-only fields.
- The struct is named `ds_register_names_t` — that name is what the
  library types `REGS.byName` against.
- `PACKED` does not need an `#include` — the library defines it before
  pulling this file in.

Drop the file at `main\my_registers.h`.

### 14.3 Point `ds_app_config.h` at it

Add one line to `main\ds_app_config.h`:

```c
#define DS_USER_REGISTER_DEFINITIONS  "my_registers.h"
```

Place it before `#define DS_SUPPRESS_DEFAULT_WARNINGS` (or remove the
suppress for registers — you now provide your own; the parameter default
warning is what the suppress is silencing).

### 14.4 Sample the ADC in `main.c`

Replace `main\main.c` with this version. It reuses the original
networking + datastream init, then spins up an ADC sampling task that
writes into `REGS.byName.*`:

```c
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_adc/adc_oneshot.h"

#include "datastream.h"
#include "dsRegisters.h"
#include "dsTCP.h"
#include "dsUDP.h"

static const char *TAG = "ds_demo";

static adc_oneshot_unit_handle_t s_adc1;

static const adc_channel_t s_channels[4] = {
    ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3,
};

static void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc1));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,        // ~0..3.3V full range
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    for (int i = 0; i < 4; i++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1, s_channels[i], &chan_cfg));
    }
}

static void adc_task(void *arg)
{
    REGS.byName.SAMPLING_ENABLED   = 1;
    REGS.byName.SAMPLE_INTERVAL_MS = 100;

    for (;;) {
        if (REGS.byName.SAMPLING_ENABLED) {
            for (int i = 0; i < 4; i++) {
                int v = 0;
                if (adc_oneshot_read(s_adc1, s_channels[i], &v) == ESP_OK) {
                    REGS.byName.ADC_RAW[i] = (uint32_t)v;
                }
            }
            REGS.byName.SAMPLE_COUNT++;
        }

        uint32_t interval = REGS.byName.SAMPLE_INTERVAL_MS;
        if (interval < 10) interval = 10;
        vTaskDelay(pdMS_TO_TICKS(interval));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    ESP_LOGI(TAG, "network up, starting datastream");
    dsInitialize();
    dsTCPTaskCreate();
    dsUDPTaskCreate();

    adc_init();
    xTaskCreate(adc_task, "adc_task", 3072, NULL, 5, NULL);
}
```

A few things worth noticing:
- `REGS` is the global `ds_registers_t` instance the library exposes;
  `byName` lets you reference fields by the names you defined in
  `my_registers.h`.
- DSHub controls the loop directly: when a client writes
  `SAMPLING_ENABLED = 0`, the task stops sampling on the next tick.
  Same for `SAMPLE_INTERVAL_MS`.
- `SAMPLE_COUNT` is a free running counter — useful for confirming the
  task is actually running when readings look frozen.

### 14.5 Add `esp_adc` to the component requirements

`main\CMakeLists.txt`:

```cmake
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES datastream nvs_flash esp_netif esp_event esp_adc)
```

### 14.6 Build, flash, observe in DSHub

```powershell
idf.py build
idf.py -p COM3 flash monitor
```

In DSHub:
1. **Discovery panel** → scan; select the board (its `DS_BOARD_NAME`
   should match what you set in `ds_app_config.h`).
2. **Map profiles** → load the `.map` you saved in 14.1.
3. **Register panel** → live values for `ADC_RAW[0]…ADC_RAW[3]` and
   `SAMPLE_COUNT`.
4. Try writing `SAMPLE_INTERVAL_MS = 20` and `= 1000` — you'll see the
   plot rate change. Set `SAMPLING_ENABLED = 0` to pause.

Tie a 10 kΩ pot between 3.3V and GND with the wiper on one of the ADC
inputs to see the value sweep 0..4095.

### 14.7 ADC1 channel-to-GPIO map by chip

| Chip | CH0 | CH1 | CH2 | CH3 |
|---|---|---|---|---|
| ESP32 (original) | GPIO36 | GPIO37 | GPIO38 | GPIO39 |
| ESP32-S2 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
| ESP32-S3 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
| ESP32-C3 | GPIO0 | GPIO1 | GPIO2 | GPIO3 |
| ESP32-C6 / H2 | GPIO0 | GPIO1 | GPIO2 | GPIO3 |

Some channels share pins with strapping or USB lines on certain
dev-kits (e.g. GPIO0 on C3/C6 is the boot strap) — if a channel reads
garbage, double-check the dev-kit schematic before suspecting code.

### 14.8 Going further

- **Add parameters.** Same shape as `my_registers.h` but for values that
  must persist across reboots. See `components\datastream\USER_INTEGRATION_GUIDE.md`
  § *Defining Parameters* and § *Flash Persistence* (NVS implementation
  is provided ready to copy).
- **Calibrate the ADC.** Convert raw counts to millivolts with
  `adc_cali_create_scheme_curve_fitting` (or `_line_fitting` on chips
  that support it). Add a `ADCx_MV` register if you want DSHub to plot
  voltages directly.
- **Use callbacks.** Override `dsRegisterSetCallback()` to react when
  DSHub writes `SAMPLING_ENABLED` — useful for triggering side effects
  beyond just gating the loop.

---

## 15. Final project layout

```
my_project\
├── CMakeLists.txt                  # project(datastream_demo)
├── .gitmodules
├── main\
│   ├── CMakeLists.txt              # REQUIRES datastream nvs_flash esp_netif esp_event esp_adc
│   ├── ds_app_config.h
│   ├── idf_component.yml           # pulls protocol_examples_common
│   ├── main.c
│   └── my_registers.h              # exported from DSHub (or hand-written)
└── components\
    └── datastream\                 # git submodule
        ├── CMakeLists.txt          # ships upstream — do not edit
        ├── include\
        ├── src\
        ├── examples\
        ├── tools\
        └── USER_INTEGRATION_GUIDE.md
```

---

> **Note on validation.** This Windows guide was authored alongside a
> Linux/macOS reference build; the commands have not been re-run on a
> Windows machine. The PowerShell syntax, `idf.py` invocations, and file
> contents are semantically identical to the Linux guide, but if you hit
> a Windows-only oddity, please open an issue against your fork so the
> doc can be updated.
