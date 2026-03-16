# A13 — Data Recorder

Circular buffer data recorder for 3-axis accelerometer data with recording/pause/clear controls, live statistics (min/max/avg), real-time chart, and CSV export preview.

## What it demonstrates

- Circular buffer data structure for continuous recording
- Recording state machine (IDLE/RECORDING/PAUSED)
- Live statistics computation (running min, max, sum/avg)
- Multi-series real-time chart visualization
- CSV export format preview
- Recording controls with interactive buttons

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270 (accelerometer data)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Press REC to start recording accelerometer data
4. Press PAUSE to pause, CLEAR to reset
5. CSV preview shows the last 5 samples in export format
