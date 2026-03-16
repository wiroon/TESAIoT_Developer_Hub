# B17 — Reaction Game

A reaction-time game: wait for the panel to turn green, then tap as fast as you can. Your reaction time is displayed in milliseconds.

## What it demonstrates

- State machine pattern (IDLE -> WAITING -> GO -> RESULT)
- Measuring time intervals with `xTaskGetTickCount()`
- Pseudo-random delay using tick count modulus
- Dynamic background color changes for game feedback
- Handling "too early" taps gracefully

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Tap the panel to start, wait for green, tap as fast as you can
