# A20 — Digital Twin

Digital twin visualization of the PSoC Edge E84 board showing component layout with interactive detail view, live sensor data overlays at physical component positions, system resource gauges, and board tilt indicator.

## What it demonstrates

- Board component layout with clickable detail panels
- Live sensor data overlays at physical IC positions
- BSP-aware component activation (greyed out if not present)
- System resource gauges (CPU, memory, temperature)
- Board tilt visualization from accelerometer
- Interactive component selection with detail information
- Digital twin concept for IoT device monitoring

## Hardware

- Board: KIT_PSE84_AI (all components active), KIT_PSE84_EVAL_EPC2 (some sensors N/A)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tap components on the board to see their details
4. Live sensor data appears next to their physical IC positions
5. Tilt the board to see the tilt indicator respond
