# A12 — WiFi Analyzer

WiFi channel spectrum analyzer with 13-channel bar chart, signal strength coloring, network list with security indicators, and best-channel recommendation engine.

## What it demonstrates

- Spectrum-style bar chart for 13 WiFi channels
- Color-coded congestion: green (1 network), orange (2), red (3+)
- Network list with SSID, channel, RSSI, and security status
- Channel recommendation algorithm (lowest congestion + weakest signals)
- Periodic scan with sensor-data-seeded variation
- dBm scale reference on Y-axis

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Watch the spectrum update every 5 seconds
4. Green bars indicate clear channels; red bars indicate congestion
