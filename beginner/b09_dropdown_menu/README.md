# B09 — Dropdown Menu

A dropdown with 5 sensor-related options. The currently selected item is displayed in a label below.

## What it demonstrates

- Creating a dropdown with `lv_dropdown_create()`
- Setting options as newline-separated strings
- Reading selected text with `lv_dropdown_get_selected_str()`
- Styling dropdown background, text, and border

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Tap the dropdown to open the list, select an option
