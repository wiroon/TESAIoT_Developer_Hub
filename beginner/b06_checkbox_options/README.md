# B06 — Checkbox Options

Three checkboxes representing system features, with a status label that updates to show which options are active.

## What it demonstrates

- Creating checkboxes with `lv_checkbox_create()`
- Reading checkbox state with `LV_STATE_CHECKED`
- Handling `LV_EVENT_VALUE_CHANGED` for multiple widgets
- Building dynamic status strings with `snprintf()`

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Tap checkboxes to toggle options; watch the status label update
