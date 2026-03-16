# A14 — Oscilloscope

3-channel oscilloscope displaying BMI270 accelerometer waveforms with green phosphor aesthetic, grid overlay, Vpp and RMS measurements, zero-crossing frequency estimation, and adjustable timebase.

## What it demonstrates

- Oscilloscope-style green phosphor display with grid lines
- Real-time Vpp (peak-to-peak) and RMS computation
- Zero-crossing frequency estimation
- Timebase control (10ms to 500ms per division)
- Run/Stop toggle for waveform capture
- Per-channel measurements panel

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270 (3-axis accelerometer)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Shake/tilt the board to see waveforms
4. Use +/- buttons to adjust timebase
5. Tap STOP to freeze the display
