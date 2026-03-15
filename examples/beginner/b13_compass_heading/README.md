# B13 — Compass Heading

Display the BMM350 magnetometer heading as degrees and a cardinal direction (N, NE, E, SE, S, SW, W, NW).

## What it demonstrates

- Reading BMM350 magnetometer data via IPC snapshot
- Converting `heading_x10` to degrees
- Mapping heading degrees to cardinal directions
- Displaying multiple related values in a styled card

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: BMM350 magnetometer

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Rotate the board to see the heading and cardinal direction change
