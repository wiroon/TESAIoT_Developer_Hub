/*******************************************************************************
 * File Name: ipc_ui.h
 *
 * Description: CM55 IPC UI handler — receives widget commands from CM33_NS
 *              MicroPython 'ui' module via IPC Pipe.
 *
 *              ISR callback -> FreeRTOS queue -> LVGL timer (50ms) processes
 *              commands in GFX task context.
 *
 *******************************************************************************/

#ifndef IPC_UI_H
#define IPC_UI_H

#include "lvgl.h"
#include <stdbool.h>

/**
 * Initialize the IPC UI handler.
 * Registers IPC callback, creates FreeRTOS queue, and LVGL timer.
 * Must be called after LVGL and IPC Pipe are initialized.
 *
 * @param parent  LVGL parent object for user-created widgets (UX/UI tab container).
 * @return true on success.
 */
bool ipc_ui_init(lv_obj_t *parent);

/**
 * Update the widget parent container (for page-based navigation).
 * Called when Playground page is created/destroyed.
 * Pass NULL to invalidate (page destroyed), non-NULL to activate.
 *
 * @param parent  LVGL container or NULL.
 */
void ipc_ui_set_container(lv_obj_t *parent);

#endif /* IPC_UI_H */
