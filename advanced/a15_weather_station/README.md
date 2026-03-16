# A15 — Weather Station

Complete weather station displaying temperature, humidity, and barometric pressure with derived metrics (dew point, heat index, altitude), trend arrows, comfort level, weather forecast, and historical charts.

## What it demonstrates

- Derived meteorological calculations (dew point, heat index, barometric altitude)
- Trend analysis with rate-of-change computation
- Weather forecast based on pressure trends
- Comfort level classification from temperature and humidity
- Multi-series historical chart
- BSP-guarded sensor access with graceful fallback

## Hardware

- Board: KIT_PSE84_AI (DPS368 + SHT40 = full weather), KIT_PSE84_EVAL_EPC2 (DPS368 only)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Watch live temperature, humidity, and pressure readings
4. Trend arrows show rate of change per minute
5. Forecast updates based on barometric pressure trends
