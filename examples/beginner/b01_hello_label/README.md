# B01 — Hello Label

Display a centered "Hello, BENTO!" greeting on the 800x480 LCD using a single LVGL label widget.

## What it demonstrates

- Minimal `example_main()` entry point structure
- Creating a label with `example_label_create()`
- Centering a widget with `lv_obj_center()`
- Using theme colors (`UI_COLOR_PRIMARY`, `UI_COLOR_CARD_BG`)

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
