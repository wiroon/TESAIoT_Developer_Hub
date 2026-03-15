# A10 — Snake Game

Classic snake game on a 26x17 grid with dual input: touch direction buttons and BMI270 tilt control. Features wraparound movement, growing snake, food spawning, and score tracking.

## What it demonstrates

- Grid-based game rendering with LVGL cell objects
- Dual input system: touch buttons + accelerometer tilt
- Snake growth and self-collision detection
- Wraparound grid boundaries
- State machine (READY/PLAYING/DEAD)
- Food spawning with snake-avoidance

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Optional: BMI270 for tilt control (touch buttons always available)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Press any direction button or tilt the board to start
4. Guide the snake to eat red food pellets
5. Avoid running into yourself
