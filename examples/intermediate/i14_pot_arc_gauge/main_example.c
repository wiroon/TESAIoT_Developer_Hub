/**
 * @file    main_example.c
 * @brief   Potentiometer Arc Gauge — 0-100% with color gradient (Eva Kit)
 *
 * Green < 50%, Yellow 50-80%, Red > 80%.  Arc updates every 50 ms via IPC.
 */

#include "example_common.h"

#if BSP_HAS_POTENTIOMETER

static lv_obj_t *s_arc;
static lv_obj_t *s_lbl_pct;
static lv_obj_t *s_lbl_raw;

/* ── Timer — read potentiometer via IPC at 20 Hz ─────────────────── */
static void pot_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    if (!snap.has_pot) return;

    /* percent_x10: 0-1000 representing 0.0-100.0% */
    int32_t pct = snap.pot.percent_x10 / 10;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    lv_arc_set_value(s_arc, pct);
    lv_label_set_text_fmt(s_lbl_pct, "%d %%", (int)pct);
    lv_label_set_text_fmt(s_lbl_raw, "Raw: %u", (unsigned)snap.pot.raw);

    /* Color gradient: green -> yellow -> red */
    lv_color_t color;
    if (pct < 50) {
        color = UI_COLOR_SUCCESS;
    } else if (pct < 80) {
        color = UI_COLOR_WARNING;
    } else {
        color = UI_COLOR_ERROR;
    }
    lv_obj_set_style_arc_color(s_arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(s_lbl_pct, color, 0);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = example_label_create(parent,
        "I14 \xe2\x80\x94 Potentiometer Gauge (Eva Kit)",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Arc gauge */
    s_arc = lv_arc_create(parent);
    lv_obj_set_size(s_arc, 260, 260);
    lv_obj_align(s_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_range(s_arc, 0, 100);
    lv_arc_set_bg_angles(s_arc, 135, 45);
    lv_arc_set_value(s_arc, 0);
    lv_obj_set_style_arc_color(s_arc, UI_COLOR_SUCCESS, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x1a3050), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc, 16, LV_PART_MAIN);
    lv_obj_remove_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Percentage label */
    s_lbl_pct = example_label_create(parent, "0 %",
        &lv_font_montserrat_28, UI_COLOR_SUCCESS);
    lv_obj_align(s_lbl_pct, LV_ALIGN_CENTER, 0, -8);

    /* Raw ADC label */
    s_lbl_raw = example_label_create(parent, "Raw: 0",
        &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(s_lbl_raw, LV_ALIGN_CENTER, 0, 20);

    /* Start timer */
    lv_timer_create(pot_timer_cb, 50, NULL);
}

#else /* BSP_HAS_POTENTIOMETER */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "I14 \xe2\x80\x94 Potentiometer Arc Gauge\n\n"
        "Requires Potentiometer (Eva Kit only).\n"
        "Not available on this board.",
        &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
    lv_obj_center(lbl);
}

#endif /* BSP_HAS_POTENTIOMETER */
