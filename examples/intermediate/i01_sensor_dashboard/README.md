# I01 — Sensor Dashboard

Full 4-card sensor dashboard displaying live IMU acceleration/gyroscope, barometric pressure/temperature, climate temperature/humidity, and compass heading. All values update at 10 Hz via IPC snapshot.

## What it demonstrates
- Multi-card dashboard layout with flex-flow wrapping
- IPC sensor snapshot for real-time data acquisition
- Board-specific sensor guards (`#if BSP_HAS_xxx`)
- Periodic LVGL timer for continuous UI updates
- Color-coded cards matching native theme colors

## Hardware
- Board: KIT_PSE84_AI (all 4 cards), KIT_PSE84_EVAL_EPC2 (IMU + Compass only)
- Sensors: BMI270, DPS368, SHT40, BMM350

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
