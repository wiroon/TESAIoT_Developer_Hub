# CM33_NS Application Directory

This directory is reserved for user application code that runs on the
Cortex-M33 Non-Secure core (MicroPython + WiFi + Sensors).

## Current State

The CM33_NS core runs:
- MicroPython REPL (via libmicropython.a)
- Sensor auto-push task (pushes IMU/Baro/Climate data to CM55 via IPC)
- WiFi stack (CYW55513 SoftAP/Station)
- OPTIGA Trust M security

For most examples, you only need to modify `proj_cm55/app/main_example.c`.
The CM33_NS side provides sensor data automatically via IPC.

## Future Use

Advanced examples (WiFi, MQTT, OPTIGA) may require CM33_NS-side code here.
