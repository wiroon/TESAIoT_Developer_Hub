/*******************************************************************************
 * File Name: page_examples.h
 *
 * Description: Example native pages for external showcase projects.
 *              These pages run as real LVGL screens linked from Home cards.
 *
 *******************************************************************************/

#ifndef PAGE_EXAMPLES_H
#define PAGE_EXAMPLES_H

#include "ipc_sensorhub.h"
#include "lvgl.h"

lv_obj_t *page_face_identification_create(void);
void      page_face_identification_render(sensorhub_snapshot_t *snap);
void      page_face_identification_destroy(void);

lv_obj_t *page_smart_watch_create(void);
void      page_smart_watch_render(sensorhub_snapshot_t *snap);
void      page_smart_watch_destroy(void);

lv_obj_t *page_spectrum_analyzer_create(void);
void      page_spectrum_analyzer_render(sensorhub_snapshot_t *snap);
void      page_spectrum_analyzer_destroy(void);

lv_obj_t *page_wifi_connect_create(void);
void      page_wifi_connect_render(sensorhub_snapshot_t *snap);
void      page_wifi_connect_destroy(void);

#endif /* PAGE_EXAMPLES_H */
