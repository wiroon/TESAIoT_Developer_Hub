# A07 — Production Dashboard

Factory-style production monitoring dashboard with 3 production zones, KPI cards, efficiency bars, temperature monitoring from real sensors, defect counting, and trend charting.

## What it demonstrates

- Multi-zone industrial dashboard layout
- KPI cards with live aggregate metrics (OEE, uptime, total output)
- Per-zone efficiency bars with color-coded status
- Live temperature from SHT40/DPS368 sensors
- Sparkline trend chart for efficiency over time
- Zone status dots (running/stopped) with simulated production events

## Hardware

- Board: KIT_PSE84_AI (live temperature), KIT_PSE84_EVAL_EPC2 (live temperature)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Watch production counters increment with sensor-driven variation
4. Zone temperatures track real sensor readings
