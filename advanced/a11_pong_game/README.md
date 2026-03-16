# A11 — Pong Game

Classic Pong against CPU opponent. Automatically selects the best input method: potentiometer on Eva Kit, CapSense slider, BMI270 tilt, or touch buttons.

## What it demonstrates

- Board-adaptive input with BSP feature guards
- Smooth paddle physics with velocity clamping
- Ball physics with angle-based paddle deflection
- CPU opponent with adjustable difficulty (speed scales with rally length)
- Dashed center line rendering
- First-to-11 scoring with win detection

## Hardware

- Board: KIT_PSE84_AI (tilt/buttons), KIT_PSE84_EVAL_EPC2 (potentiometer/CapSense)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Use potentiometer, CapSense slider, tilt, or touch UP/DOWN buttons
4. First player to 11 points wins
