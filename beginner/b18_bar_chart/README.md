# B18 — Bar Chart

Five colored vertical bars forming a static chart with value labels above and category labels below.

## What it demonstrates

- Building charts from basic LVGL objects (no chart widget needed)
- Absolute positioning with `lv_obj_set_pos()`
- Proportional height calculation from data values
- Using multiple colors for visual differentiation

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. The chart displays immediately with static data
