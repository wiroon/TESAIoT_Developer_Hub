# B10 — IMU XYZ

Display real-time BMI270 accelerometer X/Y/Z values in g-force units.

## What it demonstrates

- Reading sensor data via `ipc_sensorhub_snapshot()`
- Converting raw BMI270 accelerometer values to g-force (raw / 16384)
- Using `lv_timer_create()` for periodic sensor polling (100 ms)
- Checking `snap.has_bmi270` before accessing sensor data

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: BMI270 IMU (available on both boards)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Tilt the board to see X/Y/Z values change in real time
