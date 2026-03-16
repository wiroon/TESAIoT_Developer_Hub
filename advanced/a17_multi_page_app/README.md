# A17 — Multi-Page Application

Complete multi-page application framework with sidebar navigation, 4 pages (Dashboard, Sensors, Settings, About), interactive controls, and live sensor data.

## What it demonstrates

- Sidebar navigation with page visibility toggling
- Dashboard with aggregate KPI cards
- Live sensor data display with chart
- Settings page with slider, dropdown, and switch controls
- BSP feature detection display
- Uptime counter and WiFi status monitoring
- Page architecture pattern for embedded LVGL apps

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tap sidebar buttons to navigate between pages
4. Dashboard shows live sensor summaries
5. Settings page controls persist across page switches
