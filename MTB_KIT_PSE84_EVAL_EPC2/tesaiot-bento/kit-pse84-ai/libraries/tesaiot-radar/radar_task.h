/******************************************************************************
 * \file radar_task.h
 *
 * \brief
 *     TESAIoT Radar Task — BGT60TR13C 60 GHz radar presence detection.
 *
 *     FreeRTOS task that reads radar data via SPI, calculates signal energy,
 *     and sets a presence flag used by lcd_task for LCD power management.
 *
 *******************************************************************************
 * SPDX-FileCopyrightText: 2025-2026 TESAIoT Foundation Platform
 *
 * \author Assoc. Prof. Wiroon Sriborrirux and TESA Developer Team
 *
 * Link: https://tesaiot.github.io/developer-hub/
 ******************************************************************************/

#ifndef RADAR_TASK_H
#define RADAR_TASK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Configuration
 ******************************************************************************/

/** Radar task FreeRTOS parameters */
#define RADAR_TASK_NAME                 "Radar Task"
#define RADAR_TASK_STACK_SIZE           (8192U)
#define RADAR_TASK_PRIORITY             (configMAX_PRIORITIES - 4)  /* Keep well below GFX/app tasks */

/** Presence detection parameters */
#define RADAR_PRESENCE_THRESHOLD        20000.0f  /**< Smoothed delta threshold for motion-based presence detection */
#define RADAR_PRESENCE_ON_FRAMES        3         /**< Consecutive frames to confirm presence */
#define RADAR_ABSENCE_OFF_FRAMES        30        /**< Frames before absence (30 * ~5ms = ~150ms) */
#define RADAR_ABSENCE_TIMEOUT_MS        5000      /**< Total timeout before LCD off */

/** SPI and FIFO configuration */
#define RADAR_DEFAULT_FIFO_SETTING      128       /**< One frame per read to minimize SPI/load bursts */
#define RADAR_RING_BUFFER_SIZE          0x00004000 /**< 16K samples (32KB) */
#define RADAR_RING_BUFFER_MASK          0x00003FFF
#define RADAR_RING_BUFFER_MASK32        0x00001FFF
#define RADAR_MAX_FRAMES_PER_CYCLE      1

/*******************************************************************************
 * Types
 ******************************************************************************/

/** Radar presence state */
typedef enum {
    RADAR_PRESENCE_ABSENT   = 0,    /**< No person detected */
    RADAR_PRESENCE_DETECTED = 1,    /**< Person present */
} radar_presence_state_t;

/*******************************************************************************
 * Global Variables (extern)
 ******************************************************************************/

/**
 * Radar presence flag — set by tesaiot_radar_task, read by lcd_task.
 * volatile for cross-task access.
 */
extern volatile bool tesaiot_radar_presence_detected;

/**
 * Current radar signal energy — for debugging/display.
 */
extern volatile float tesaiot_radar_current_energy;

/**
 * Radar initialized flag.
 */
extern volatile bool tesaiot_radar_initialized;

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * Radar FreeRTOS task entry point.
 *
 * Initializes SPI, configures BGT60TR13C, and enters a polling loop
 * that reads radar data, calculates energy, and updates presence flag.
 *
 * \param[in] arg  Unused task argument.
 */
void tesaiot_radar_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* RADAR_TASK_H */
