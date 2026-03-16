# A06 — Predictive Alert Engine

Analyzes sensor data trends using linear regression over a sliding window, predicts when thresholds will be crossed, and displays tiered alerts (NORMAL/INFO/WARNING/CRITICAL) with countdown timers.

## What it demonstrates

- Linear regression trend analysis on live sensor data
- Threshold crossing prediction with time estimates
- Multi-level alert system with color-coded indicators
- Real-time charting of 4 sensor channels simultaneously
- Alert log with aggregated status messages
- BSP-guarded sensor access with fallback values

## Hardware

- Board: KIT_PSE84_AI (all channels live), KIT_PSE84_EVAL_EPC2 (IMU live, env simulated)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tilt the board to trigger accelerometer threshold predictions
4. Watch alert status change from NORMAL to WARNING to CRITICAL
