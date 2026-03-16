# I11 — System Status Panel

Two-column system status panel. Left side shows WiFi connection details (SSID, IP, RSSI), NTP sync state, current time, and uptime. Right side shows sensor availability with green/red dots and board capability flags.

## What it demonstrates
- WiFi Manager status API for connection details
- NTP sync check and time string retrieval
- Compile-time sensor/feature detection with visual indicators
- Uptime counter using `lv_tick_get()`
- Two-column card layout

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: None required (displays availability of all)

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
