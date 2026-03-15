# B12 — Humidity Bar

SHT40 humidity reading displayed as a colored bar that shifts from orange (dry) to green (comfortable) to cyan (humid).

## What it demonstrates

- Reading SHT40 climate sensor via IPC snapshot
- Converting `humidity_x100` and `temperature_x100` to real units
- Displaying sensor data as both a bar widget and text labels
- Guarding sensor code with `#if BSP_HAS_SHT40`

## Hardware

- Board: KIT_PSE84_AI only (SHT40 not available on Eva Kit)
- Sensors: SHT40 temperature/humidity

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Breathe on the sensor to see humidity change
