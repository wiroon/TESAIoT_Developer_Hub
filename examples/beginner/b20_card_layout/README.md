# B20 — Card Layout

A polished dashboard with three information cards using structured typography: accent bar, title, subtitle, and a large value.

## What it demonstrates

- Creating reusable card components with consistent styling
- Flex layout with `LV_FLEX_ALIGN_SPACE_EVENLY` for even spacing
- Multi-level typography hierarchy (title, subtitle, value)
- Accent color bars and borders for visual identity
- Golden-ratio-inspired proportions for aesthetic cards

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. The dashboard displays immediately with static system metrics
