# B03 — Button Counter

A button that increments a counter label on each tap, demonstrating touch event handling.

## What it demonstrates

- Creating a styled button with `lv_btn_create()`
- Registering event callbacks with `lv_obj_add_event_cb()`
- Handling `LV_EVENT_CLICKED` events
- Updating label text dynamically with `lv_label_set_text_fmt()`

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Tap the button to increment the counter
