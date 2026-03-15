# B11 — Temperature

Display the DPS368 barometric sensor's temperature reading as a large, color-coded value.

## What it demonstrates

- Reading DPS368 sensor data via IPC snapshot
- Converting `temperature_x100` to floating-point degrees Celsius
- Color-coding values by temperature range (cool/normal/hot)
- Guarding sensor-specific code with `#if BSP_HAS_DPS368`

## Hardware

- Board: KIT_PSE84_AI only (DPS368 not available on Eva Kit)
- Sensors: DPS368 barometric pressure/temperature

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Observe the temperature reading update every 500 ms
