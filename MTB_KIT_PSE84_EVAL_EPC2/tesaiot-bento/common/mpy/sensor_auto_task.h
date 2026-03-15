/*******************************************************************************
 * File Name: sensor_auto_task.h
 *
 * Description: Auto-sensor background FreeRTOS task.
 *              Reads all sensors and pushes data via IPC to CM55 for LVGL
 *              dashboard display. Starts automatically on boot.
 *              MicroPython can pause/resume and control per-sensor enable.
 *
 *******************************************************************************/

#ifndef SENSOR_AUTO_TASK_H
#define SENSOR_AUTO_TASK_H

#include <stdbool.h>
#include <stdint.h>

/* Sensor enable bitmask flags */
#define SENSOR_AUTO_BMI270      (1 << 0)
#define SENSOR_AUTO_DPS368      (1 << 1)
#define SENSOR_AUTO_SHT40       (1 << 2)
#define SENSOR_AUTO_BMM350      (1 << 3)
#define SENSOR_AUTO_CAPSENSE    (1 << 4)
#define SENSOR_AUTO_POT         (1 << 5)
#define SENSOR_AUTO_ALL         (0x3F)

/* Create the auto-sensor FreeRTOS task (call from main before scheduler) */
void sensor_auto_task_create(void);

/* Start/resume auto-push */
void sensor_auto_start(void);

/* Pause auto-push (task suspended, zero CPU) */
void sensor_auto_stop(void);

/* Check if auto-push is running */
bool sensor_auto_is_running(void);

/* Set push interval in milliseconds (min 50, max 5000) */
void sensor_auto_set_rate(uint32_t interval_ms);
uint32_t sensor_auto_get_rate(void);

/* Per-sensor enable/disable */
void sensor_auto_set_mask(uint32_t mask);
uint32_t sensor_auto_get_mask(void);
void sensor_auto_enable(uint32_t flag);
void sensor_auto_disable(uint32_t flag);

/* Total push cycles since boot */
uint32_t sensor_auto_get_push_count(void);

/* Check and consume delete-main.py request (set by IPC ISR, polled from task) */
bool sensor_auto_is_delete_pending(void);

/*******************************************************************************
 * WiFi State + Time Push to CM55 (callable from any CM33_NS task/module)
 *
 * Sends non-blocking IPC push to CM55 topbar.
 * Call after wifi.connect()/disconnect() succeeds.
 ******************************************************************************/

/** Push WiFi connected/disconnected state to CM55 topbar */
void sensor_auto_push_wifi_state(bool connected);

/** Try NTP sync + push time to CM55. Call after WiFi connect succeeds. */
void sensor_auto_ntp_and_push_time(void);

#endif /* SENSOR_AUTO_TASK_H */
