/*******************************************************************************
 * File Name: page_controls.h
 *
 * Description: Controls page — LEDs, CapSense, Potentiometer.
 *              Eva Kit only (BSP_HAS_CAPSENSE || BSP_HAS_POTENTIOMETER).
 *              Page-based replacement for tab_controls (create/render/destroy).
 *
 *******************************************************************************/

#ifndef PAGE_CONTROLS_H
#define PAGE_CONTROLS_H

#include "bsp_feature_flags.h"

#if BSP_HAS_CAPSENSE || BSP_HAS_POTENTIOMETER

#include "ipc_sensorhub.h"
#include "lvgl.h"

lv_obj_t *page_controls_create(void);
void      page_controls_render(sensorhub_snapshot_t *snap);
void      page_controls_destroy(void);

#endif /* BSP_HAS_CAPSENSE || BSP_HAS_POTENTIOMETER */
#endif /* PAGE_CONTROLS_H */
