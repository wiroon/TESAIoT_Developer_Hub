# A09 — Flappy Bird

Classic Flappy Bird game controlled by tilting the board. Uses BMI270 accelerometer for input. Features scrolling pipes, collision detection, scoring, and high score tracking.

## What it demonstrates

- Game loop with fixed timestep using `lv_timer_create`
- Physics simulation (gravity, velocity, position)
- Tilt-based input from BMI270 accelerometer
- Pipe recycling for infinite scrolling
- AABB collision detection
- Game state machine (READY/PLAYING/DEAD)
- LCG pseudo-random number generator for pipe placement

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270 (tilt control)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tilt the board forward to start
4. Tilt up/down to control the bird's altitude
5. Avoid pipes and the ground
6. Tilt back after game over to restart
