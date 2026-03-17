/**
 * sensorhub_ui_bridge.c — Template-mode SensorHub UI bridge
 *
 * Replaces the full SensorHub UI (page manager, dashboard, all pages)
 * with a thin wrapper that delegates to example_main().
 *
 * tesaiot_display.c (in prebuilt .a) calls sensorhub_ui_init(scr),
 * which we redirect to the user's example_main(parent).
 */

#include "sensorhub_ui.h"
#include "pse84_common.h"

/* ── Called by tesaiot_display.c after LVGL + touch are ready ──────── */
bool sensorhub_ui_init(void *parent)
{
    example_main((lv_obj_t *)parent);
    return true;
}

/* ── Stubs for functions referenced by IPC modules ────────────────── */
void *sensorhub_ui_get_uxui_container(void)    { return NULL; }
void *sensorhub_ui_get_terminal_container(void) { return NULL; }
void  sensorhub_ui_switch_to_uxui(void)        { /* no-op */ }
void  sensorhub_ui_switch_to_terminal(void)    { /* no-op */ }
void  sensorhub_ui_set_ide_connected(bool c)   { (void)c; }
