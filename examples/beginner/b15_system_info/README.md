# B15 — System Info

Display board identification, uptime (HH:MM:SS), FreeRTOS tick count, and free heap size.

## What it demonstrates

- Using FreeRTOS APIs: `xTaskGetTickCount()`, `xPortGetFreeHeapSize()`
- Computing uptime from tick count and `configTICK_RATE_HZ`
- Combining static and dynamic information in a card layout
- 1-second periodic updates with `lv_timer_create()`

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Observe the uptime, tick count, and heap info updating every second
