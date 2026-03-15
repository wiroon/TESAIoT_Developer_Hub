# TESAIoT Developer Hub — MTB Template for KIT_PSE84_EVAL_EPC2

ModusToolbox project template for the **PSoC Edge E84 Eva Kit** (`APP_KIT_PSE84_EVAL_EPC2`).

Drop a `main_example.c` from the [TESAIoT Developer Hub](https://dev.tesaiot.com) into
`proj_cm55/app/` and build — no extra setup needed.

---

## Quick Start

```bash
# 1. Fetch shared MTB libraries (one-time)
make getlibs

# 2. Build all 3 cores
make build -j

# 3. Flash to board
make program
```

Replace `proj_cm55/app/main_example.c` with any example from the Developer Hub.
That's it — build and flash.

> **WiFi Setup:** Before building WiFi examples, edit `proj_cm55/wifi_config.h` and set
> `STA_SSID` / `STA_PASSWORD` to your WiFi network credentials.

---

## IDE Setup

This template works with **three environments**. Choose the one that fits your workflow.

### Option A: Command Line (macOS / Linux)

The Quick Start commands above work directly in Terminal. Make sure `ModusToolbox/tools_3.6`
is installed and `make` is in your PATH.

### Option B: ModusToolbox Eclipse IDE (Windows / macOS)

1. **Install** ModusToolbox 3.6 from [Infineon Developer Center](https://www.infineon.com/modustoolbox)
2. **Import project:**
   - `File` > `Import...` > `ModusToolbox` > `Import Existing Application In-Place`
   - Browse to this `MTB_KIT_PSE84_EVAL_EPC2/` folder and click `Finish`
   - Eclipse will detect all 3 sub-projects (CM33_S, CM33_NS, CM55) automatically
3. **Fetch libraries:** Right-click the project > `ModusToolbox` > `Library Manager` > `Update`
   (equivalent to `make getlibs`)
4. **Build:** Click the **Build** button (hammer icon) or `Project` > `Build All`
5. **Flash:** Click the **Program** button (green arrow) or right-click > `ModusToolbox` > `Program`

> **Tip (Windows):** If you see path-length errors, clone the template to a short path
> like `C:\mtb\Eva_Kit\` instead of deep nested folders.

### Option C: MTB Assistant — VSCode Extension (Windows / macOS)

1. **Install** [Visual Studio Code](https://code.visualstudio.com/)
2. **Install** the **ModusToolbox Assistant** extension from the VSCode Marketplace
   (search "ModusToolbox" in Extensions, or install from `.vsix` if provided)
3. **Open folder:** `File` > `Open Folder...` > select this `MTB_KIT_PSE84_EVAL_EPC2/` folder
4. **Initialize:** MTB Assistant will detect the project and prompt to configure.
   Click `Yes` to set up IntelliSense and build tasks automatically
5. **Fetch libraries:** Open Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`)
   > `ModusToolbox: Library Manager` > `Update`
6. **Build:** Command Palette > `ModusToolbox: Build` or use the status bar build button
7. **Flash:** Command Palette > `ModusToolbox: Program`

> **Tip:** MTB Assistant provides IntelliSense (auto-complete, go-to-definition) for all
> BSP headers, LVGL APIs, and FreeRTOS functions — making it ideal for learning the APIs.

### Prerequisites (All Options)

| Requirement | Notes |
|-------------|-------|
| ModusToolbox 3.6+ | Includes GCC ARM compiler, make, OpenOCD |
| KitProg3 driver | Installed automatically with ModusToolbox (Windows) or via Homebrew (macOS) |
| USB cable | Micro-USB to KitProg3 debug port on the board |

---

## What You Get

### Hardware (KIT_PSE84_EVAL_EPC2)

| Feature | Detail |
|---------|--------|
| MCU | PSoC Edge E84 — Cortex-M55 (display/AI) + Cortex-M33 (sensors/WiFi) |
| Display | 800x480 DSI LCD (Waveshare 4.3"), capacitive touch (FT5406) |
| IMU | BMI270 — 6-axis accel + gyro |
| Compass | BMM350 — 3-axis magnetometer |
| CapSense | Touch buttons (btn0, btn1) + slider (0-100%) |
| Potentiometer | Analog dial (0-100%) |
| Audio Codec | On-board audio (WM8904 or equivalent) |
| PDM Microphone | Digital audio input |
| WiFi | CYW55513 — 2.4/5 GHz, STA + SoftAP |
| USB Host | SEGGER emUSB-Host HID (Logitech F310 joystick) |
| Security | OPTIGA Trust M (optional, `ENABLE_OPTIGA=1`) |
| LEDs | 3x RGB (P13_7 red, P13_4 green, P13_3 blue) |
| Buttons | SW1 (P5_2), SW2 (P5_3) |

**Note:** Eva Kit does NOT have DPS368 (barometer), SHT40 (humidity), or BGT60TR13C (radar).
Use `snap.has_dps368` / `snap.has_sht40` guards in your code for cross-board compatibility.

### Software Stack

| Layer | Technology |
|-------|-----------|
| RTOS | FreeRTOS 10.6 |
| GUI | LVGL 9.2 (800x480, hardware-accelerated) |
| Scripting | MicroPython (runs on CM33, pre-built) |
| WiFi | Cypress WCM + lwIP + mbedTLS |
| IPC | Cypress IPC Pipe (CM33 <-> CM55, zero-copy) |

---

## Available APIs

All APIs are accessible from `main_example.c` via `#include "example_common.h"`.

### Sensor Data (`ipc_sensorhub.h`)

Sensor values are pushed automatically from CM33 via IPC. Read them with:

```c
sensorhub_snapshot_t snap;
ipc_sensorhub_snapshot(&snap);

// IMU (BMI270)
if (snap.has_bmi270) {
    float ax = snap.bmi270.ax / 16384.0f;  // g
    float gx = snap.bmi270.gx / 16.4f;     // deg/s
}

// Compass (BMM350)
if (snap.has_bmm350) {
    float heading = snap.bmm350.heading_x10 / 10.0f;  // 0-360 degrees
}

// CapSense (Eva Kit)
if (snap.has_capsense) {
    bool btn0 = snap.capsense.btn0_pressed;
    bool btn1 = snap.capsense.btn1_pressed;
    uint8_t slider = snap.capsense.slider;  // 0-100%
}

// Potentiometer (Eva Kit)
if (snap.has_pot) {
    float percent = snap.pot.percent_x10 / 10.0f;  // 0-100%
}
```

| Function | Returns |
|----------|---------|
| `ipc_sensorhub_init()` | Start receiving sensor data |
| `ipc_sensorhub_snapshot(&snap)` | Get all sensor values at once |
| `ipc_sensorhub_wifi_connected()` | WiFi status (bool) |
| `ipc_sensorhub_ntp_synced()` | NTP time available? (bool) |
| `ipc_sensorhub_get_time_str(buf, size)` | Current time string |

### WiFi Manager (`wifi_manager.h`)

```c
#include "wifi_manager.h"

wifi_manager_init();
wifi_manager_connect("MySSID", "MyPassword");

if (wifi_manager_is_connected()) {
    const char *ip = wifi_manager_get_ip();
}
```

| Function | Description |
|----------|-------------|
| `wifi_manager_connect(ssid, pass)` | Connect to WiFi (blocking) |
| `wifi_manager_scan_start()` | Start async scan |
| `wifi_manager_scan_ready()` | Check if scan done |
| `wifi_manager_scan_result(out, max)` | Get scan results |
| `wifi_manager_start_softap()` | Start SoftAP mode |
| `wifi_manager_disconnect()` | Disconnect |
| `wifi_manager_get_ip()` | Get IP address string |

### LVGL 9.2 — UI Widgets

All LVGL widgets are available: labels, buttons, charts, arcs, bars, sliders,
spinners, dropdowns, tables, images, and more.

```c
// Create a label
lv_obj_t *lbl = lv_label_create(parent);
lv_label_set_text(lbl, "Hello, PSoC Edge!");

// Create a button with event
lv_obj_t *btn = lv_button_create(parent);
lv_obj_add_event_cb(btn, my_callback, LV_EVENT_CLICKED, NULL);

// Create a real-time chart
lv_obj_t *chart = lv_chart_create(parent);
lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
lv_chart_series_t *ser = lv_chart_add_series(chart, lv_color_hex(0x00BCD4), LV_CHART_AXIS_PRIMARY_Y);
```

### Helper Functions (from `example_common.h`)

```c
// Styled card container
lv_obj_t *card = example_card_create(parent, 340, 100, UI_COLOR_CARD_BG);

// Styled label
lv_obj_t *lbl = example_label_create(parent, "Title", &lv_font_montserrat_24, UI_COLOR_PRIMARY);
```

### LVGL Symbols (confirmed working on hardware)

```c
LV_SYMBOL_HOME      LV_SYMBOL_WIFI      LV_SYMBOL_USB
LV_SYMBOL_SETTINGS  LV_SYMBOL_PLAY      LV_SYMBOL_EDIT
LV_SYMBOL_OK        LV_SYMBOL_CLOSE     LV_SYMBOL_REFRESH
LV_SYMBOL_TRASH     LV_SYMBOL_LEFT      LV_SYMBOL_RIGHT
LV_SYMBOL_UP        LV_SYMBOL_DOWN      LV_SYMBOL_GPS
LV_SYMBOL_CHARGE    LV_SYMBOL_TINT      LV_SYMBOL_LOOP
LV_SYMBOL_SHUFFLE   LV_SYMBOL_AUDIO     LV_SYMBOL_LIST
LV_SYMBOL_EYE_OPEN  LV_SYMBOL_BACKSPACE
```

### Color Palette

```c
UI_COLOR_PRIMARY     // 0x00BCD4  Cyan accent
UI_COLOR_CARD_BG     // 0x142240  Dark card background
UI_COLOR_TEXT        // 0xE0E0E0  Light text
UI_COLOR_TEXT_DIM    // 0x808080  Dimmed text
UI_COLOR_SUCCESS     // 0x4CAF50  Green
UI_COLOR_WARNING     // 0xFF9800  Orange
UI_COLOR_ERROR       // 0xF44336  Red
UI_COLOR_INFO        // 0x2196F3  Blue
UI_COLOR_BMI270      // 0x4CAF50  IMU — green
UI_COLOR_BMM350      // 0xE040FB  Compass — purple
```

---

## Project Structure

```
MTB_KIT_PSE84_EVAL_EPC2/
  common.mk               # TARGET=APP_KIT_PSE84_EVAL_EPC2, toolchain, paths
  Makefile                 # Top-level application build
  proj_cm33_s/             # Secure boot (do not modify)
  proj_cm33_ns/            # MicroPython + WiFi (CM33 non-secure)
    Makefile               # Pre-built libmicropython.a, WiFi stack
  proj_cm55/               # Display + AI (your code goes here)
    Makefile               # LVGL, FreeRTOS, IPC, sensor APIs
    app/                   # <-- DROP YOUR EXAMPLES HERE
      main_example.c       # Your example (implements example_main)
      example_common.h     # Shared header — includes, macros, helpers
      sensorhub_ui_stubs.c # Required stubs (do not remove)
  prebuilt/                # Pre-built static libraries
    libmicropython.a       # MicroPython runtime (1.0 MB)
    libtesaiot.a           # TESAIoT license + crypto (88 KB)
    libtesaiot_license.a   # License data (16 KB)
    ifx_face_id.a          # Face ID ML engine (2.9 MB)
  tesaiot-bento/           # TESAIoT shared libraries (IPC, sensors, WiFi)
  bsps/                    # Board Support Package
  configs/                 # Signing / boot configuration
```

---

## Writing Your Own Example

Create `proj_cm55/app/main_example.c` with this signature:

```c
#include "example_common.h"

void example_main(lv_obj_t *parent)
{
    // parent is a full-screen 800x480 container with dark background
    // All LVGL, FreeRTOS, and sensor APIs are available here

    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "My First Example!");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl, UI_COLOR_PRIMARY, 0);
    lv_obj_center(lbl);
}
```

### Cross-Board Compatibility

Write examples that work on both AI Kit and Eva Kit using BSP feature guards:

```c
#if BSP_HAS_DPS368
    // Barometer — AI Kit only
    float pressure = snap.dps368.pressure_x100 / 100.0f;
#endif

#if BSP_HAS_CAPSENSE
    // CapSense — Eva Kit only
    if (snap.capsense.btn0_pressed) { /* ... */ }
#endif
```

### Rules

1. Your entry point is `void example_main(lv_obj_t *parent)` — always
2. All LVGL calls must happen in this function (runs in GFX task context)
3. Use `lv_timer_create()` for periodic updates (not `vTaskDelay` in GFX task)
4. Sensor data arrives automatically via IPC — just call `ipc_sensorhub_snapshot()`
5. Do not remove `sensorhub_ui_stubs.c` — it provides required linker symbols

---

## Sensor Comparison: AI Kit vs Eva Kit

| Sensor | AI Kit | Eva Kit |
|--------|--------|---------|
| BMI270 (IMU) | Yes | Yes |
| BMM350 (Compass) | Yes | Yes |
| DPS368 (Barometer) | Yes | **No** |
| SHT40 (Climate) | Yes | **No** |
| BGT60TR13C (Radar) | Yes | **No** |
| CapSense (Touch) | **No** | Yes |
| Potentiometer | **No** | Yes |
| Audio Codec | **No** | Yes |
| PDM Microphone | Yes | Yes |

---

## 95 Ready-Made Examples

Download from [dev.tesaiot.com](https://dev.tesaiot.com):

| Level | Count | Topics |
|-------|-------|--------|
| **Beginner** (b01-b42) | 42 | Hello World, buttons, sliders, arcs, spinners, dropdowns, tables, images, flex/grid layout, colors, fonts, styles, events, timers, LEDs, GPIO, sensor labels |
| **Intermediate** (i01-i32) | 32 | Line/bar/multi-series charts, real-time accel/gyro, compass heading, tilt display, CapSense, potentiometer, sensor dashboard, IPC communication, page navigation |
| **Advanced** (a01-a21) | 21 | WiFi scan/connect, MQTT, OPTIGA crypto, camera preview, face detection, Snake/Pong games, joystick, radar, smart watch, production dashboard |

---

## Board: PSoC Edge E84 Eva Kit

```
  +------------------------------------------+
  |  PSoC Edge E84 Eva Kit (EVAL_EPC2)       |
  |                                          |
  |  CM55 (Display/AI)    CM33 (Sensors/WiFi)|
  |  +-----------------+  +----------------+ |
  |  | LVGL 9.2        |  | MicroPython    | |
  |  | 800x480 LCD     |  | WiFi CYW55513  | |
  |  | FreeRTOS        |  | BMI270/BMM350  | |
  |  | Face ID ML      |  | CapSense/Pot   | |
  |  | USB Host HID    |  | OPTIGA Trust M | |
  |  +-----------------+  +----------------+ |
  |         ^    IPC Pipe    ^               |
  |         +----------------+               |
  +------------------------------------------+
```

---

## License

Copyright 2024-2026 TESAIoT & BDH. All rights reserved.
> Thai Embedded Systems Association (TESA) 
