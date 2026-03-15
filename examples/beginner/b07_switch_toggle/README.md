# B07 — Switch Toggle

An On/Off switch that changes a circular indicator between green (ON) and red (OFF).

## What it demonstrates

- Creating a switch with `lv_switch_create()`
- Reading checked state from switch events
- Dynamic color changes on indicator widgets
- Circular widget styling with `LV_RADIUS_CIRCLE`

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Tap the switch to toggle between ON and OFF states
