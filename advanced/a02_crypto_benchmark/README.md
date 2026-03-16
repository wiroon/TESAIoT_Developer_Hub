# A02 — Crypto Benchmark

Visualizes cryptographic operation performance as animated horizontal bar charts. Compares hardware-accelerated vs software crypto timings for ECC, SHA-256, AES, HMAC, and TRNG operations.

## What it demonstrates

- Animated bar chart visualization with color-coded algorithms
- Periodic benchmark re-runs with jitter from live sensor data
- Professional column-based layout with headers and legend
- LVGL animation system for smooth bar transitions
- Real-time performance monitoring patterns

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Benchmarks re-run every 3 seconds with updated timing data
