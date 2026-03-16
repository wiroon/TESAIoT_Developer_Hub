# B08 — Progress Bar

An animated progress bar that fills from 0% to 100%, changing color from red to orange to green as it progresses.

## What it demonstrates

- Creating a bar widget with `lv_bar_create()`
- Timer-driven animation with `lv_timer_create()`
- Dynamic color changes based on value thresholds
- Bar styling for main track and indicator parts

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Watch the progress bar animate and change color
