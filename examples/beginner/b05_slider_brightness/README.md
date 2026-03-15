# B05 — Slider Brightness

A slider widget that adjusts the background panel brightness from dim to full.

## What it demonstrates

- Creating a slider with `lv_slider_create()`
- Handling `LV_EVENT_VALUE_CHANGED` events
- Mapping slider values to opacity
- Styling slider parts (main track, indicator, knob)

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Drag the slider to change the background brightness
