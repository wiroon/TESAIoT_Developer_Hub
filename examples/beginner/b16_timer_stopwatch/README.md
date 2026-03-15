# B16 — Timer Stopwatch

A stopwatch with Start, Stop, and Reset buttons displaying elapsed time in MM:SS.mmm format.

## What it demonstrates

- Using `lv_timer_create()` for periodic time tracking (50 ms resolution)
- Button event callbacks for start/stop/reset control flow
- Formatting elapsed time into human-readable MM:SS.mmm
- Status indicator with dynamic color changes

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Tap Start to begin timing, Stop to pause, Reset to clear
