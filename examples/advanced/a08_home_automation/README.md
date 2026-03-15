# A08 — Home Automation

Smart home dashboard with 4 room cards showing temperature, humidity, light levels, and occupancy. Includes 6 device control switches, live outdoor sensor data, and energy usage tracking.

## What it demonstrates

- Room monitoring cards with temperature, humidity, and light bars
- Interactive device toggles using LVGL switches with event callbacks
- Live outdoor readings from DPS368/SHT40 sensors
- Energy usage counter driven by sensor variation
- Occupancy status dots per room
- Professional smart home UI layout

## Hardware

- Board: KIT_PSE84_AI (temp + humidity + pressure live), KIT_PSE84_EVAL_EPC2 (temp + pressure)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Toggle device switches to control simulated smart home devices
4. Outdoor readings update from real board sensors
