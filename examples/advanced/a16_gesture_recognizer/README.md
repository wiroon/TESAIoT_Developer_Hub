# A16 — Gesture Recognizer

Detects 6 gestures from BMI270 IMU data: shake, tilt left, tilt right, flip, tap, and rotate. Uses variance analysis, threshold detection, and cooldown debouncing.

## What it demonstrates

- Signal variance computation for shake detection
- Threshold-based gesture classification
- Cooldown timer for debouncing rapid detections
- Gesture counter and indicator bar feedback
- Acceleration magnitude chart for motion visualization
- Raw 6-axis data display (accel + gyro)

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270 (6-axis IMU)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Shake, tilt, flip, tap, or rotate the board
4. Watch detected gestures and counters update
