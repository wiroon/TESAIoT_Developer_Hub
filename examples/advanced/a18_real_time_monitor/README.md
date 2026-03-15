# A18 — Real-Time Process Monitor

Industrial-style process monitor for 3 process variables with setpoint tracking, alarm zones (LOW/HIGH/CRITICAL), deviation display, trend chart, and timestamped alarm history log.

## What it demonstrates

- Multi-level alarm system with color-coded zones
- Setpoint-based deviation tracking
- Process variable gauge bars with dynamic coloring
- Alarm event logging with timestamps
- Trend chart for process variable history
- Real sensor data mapped to industrial process variables

## Hardware

- Board: KIT_PSE84_AI (DPS368 + SHT40), KIT_PSE84_EVAL_EPC2 (DPS368)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Watch process values track real sensor data
4. Alarm log updates when values cross thresholds
