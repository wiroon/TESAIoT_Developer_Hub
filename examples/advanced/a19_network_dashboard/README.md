# A19 — Network Dashboard

WiFi network status dashboard showing connection info, real-time signal strength with history chart, TX/RX traffic counters, and periodic scan of nearby networks.

## What it demonstrates

- WiFi connection status via `ipc_sensorhub_wifi_connected()`
- Signal strength bar with color-coded quality levels
- RSSI history chart for signal monitoring over time
- TX/RX byte counters with human-readable formatting
- Nearby network scan with RSSI bars and security indicators
- Network statistics (uptime, reconnects)

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. WiFi connection status updates automatically
4. Signal chart shows RSSI history
5. Nearby networks refresh every 10 seconds
