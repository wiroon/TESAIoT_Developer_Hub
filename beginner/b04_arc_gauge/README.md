# B04 — Arc Gauge

An arc widget configured as a read-only gauge that sweeps automatically between 0 and 100.

## What it demonstrates

- Creating and styling an arc with `lv_arc_create()`
- Configuring arc range, angles, and indicator appearance
- Making an arc read-only by removing clickable flag and knob
- Using `lv_timer_create()` for periodic animation updates

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Watch the arc sweep between 0% and 100% automatically
