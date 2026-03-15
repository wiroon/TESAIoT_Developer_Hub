/**
 * sensorhub_ui_stubs.c — Stub implementations for SensorHub UI functions
 *
 * In template mode, the full SensorHub UI (page manager, dashboard, etc.)
 * is not included. However, IPC modules (ipc_lcd.c, ipc_ui.c) reference
 * some SensorHub UI functions. These stubs provide no-op implementations.
 */

#include "sensorhub_ui.h"

void sensorhub_ui_switch_to_uxui(void)
{
    /* No-op in template mode — no page manager to switch */
}

void sensorhub_ui_set_ide_connected(bool connected)
{
    (void)connected;
    /* No-op in template mode — no IDE connection indicator */
}
