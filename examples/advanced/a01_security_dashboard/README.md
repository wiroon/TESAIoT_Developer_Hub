# A01 — Security Dashboard

Comprehensive HSM security overview with chip identity, lifecycle state, certificate slot status, RNG entropy monitoring, and supported crypto algorithms. Professional dark-themed multi-card layout with live status indicators.

## What it demonstrates

- Multi-card dashboard layout with consistent dark theme
- Status dot indicators for health and certificate slots
- Animated progress bar for RNG entropy quality
- Periodic data refresh via `lv_timer_create`
- Real sensor data used for chip UID generation
- Professional security-oriented UI design patterns

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. The dashboard refreshes every 5 seconds with updated status
