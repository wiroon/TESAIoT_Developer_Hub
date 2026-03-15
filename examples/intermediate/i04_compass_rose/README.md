# I04 — Compass Rose

Graphical compass with a rotating needle driven by BMM350 magnetometer heading. Renders cardinal labels (N/S/E/W), an outer ring, and a red needle on a canvas widget.

## What it demonstrates
- LVGL canvas widget for custom drawing
- Trigonometric needle rotation from heading data
- Cardinal direction text conversion
- Real-time magnetometer visualization at 12 Hz

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMM350

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
