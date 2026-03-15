/**
 * @file    main_example.c
 * @brief   Arc Gauge Factory — Reusable tesaiot_arc_gauge_create()
 *
 * Factory function builds arc + center label + title.  Returns handles
 * for easy value updates.  Four sample gauges demonstrate usage.
 */

#include "example_common.h"

/* ── Gauge handle struct ─────────────────────────────────────────── */
typedef struct {
    lv_obj_t *arc;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_title;
} arc_gauge_t;

/* ── Factory function ────────────────────────────────────────────── */
static arc_gauge_t tesaiot_arc_gauge_create(lv_obj_t *parent,
                                             const char *title,
                                             lv_color_t color,
                                             int32_t range_min,
                                             int32_t range_max,
                                             int size)
{
    arc_gauge_t g;

    /* Container for positioning */
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, size + 20, size + 50);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    g.lbl_title = lv_label_create(cont);
    lv_label_set_text(g.lbl_title, title);
    lv_obj_set_style_text_font(g.lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g.lbl_title, color, 0);
    lv_obj_align(g.lbl_title, LV_ALIGN_TOP_MID, 0, 0);

    /* Arc */
    g.arc = lv_arc_create(cont);
    lv_obj_set_size(g.arc, size, size);
    lv_obj_align(g.arc, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_arc_set_range(g.arc, range_min, range_max);
    lv_arc_set_bg_angles(g.arc, 135, 45);
    lv_arc_set_value(g.arc, range_min);
    lv_obj_set_style_arc_color(g.arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(g.arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(g.arc, lv_color_hex(0x1a3050), LV_PART_MAIN);
    lv_obj_set_style_arc_width(g.arc, 10, LV_PART_MAIN);
    lv_obj_remove_flag(g.arc, LV_OBJ_FLAG_CLICKABLE);

    /* Center value label */
    g.lbl_value = lv_label_create(cont);
    lv_label_set_text(g.lbl_value, "--");
    lv_obj_set_style_text_font(g.lbl_value, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(g.lbl_value, color, 0);
    lv_obj_align_to(g.lbl_value, g.arc, LV_ALIGN_CENTER, 0, 0);

    return g;
}

/* ── Helper: set gauge value and update label ────────────────────── */
static void gauge_set_value(arc_gauge_t *g, int32_t val, const char *fmt)
{
    lv_arc_set_value(g->arc, val);
    lv_label_set_text_fmt(g->lbl_value, fmt, (int)val);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I31 — Arc Gauge Factory");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Four gauges in a row */
    arc_gauge_t g_temp = tesaiot_arc_gauge_create(parent, "Temp",
        lv_color_hex(0xFF9800), 0, 50, 140);
    lv_obj_align(g_temp.arc->parent, LV_ALIGN_CENTER, -270, 20);
    gauge_set_value(&g_temp, 27, "%d C");

    arc_gauge_t g_pres = tesaiot_arc_gauge_create(parent, "Pressure",
        lv_palette_main(LV_PALETTE_CYAN), 900, 1100, 140);
    lv_obj_align(g_pres.arc->parent, LV_ALIGN_CENTER, -90, 20);
    gauge_set_value(&g_pres, 1013, "%d");

    arc_gauge_t g_humi = tesaiot_arc_gauge_create(parent, "Humidity",
        lv_color_hex(0x2196F3), 0, 100, 140);
    lv_obj_align(g_humi.arc->parent, LV_ALIGN_CENTER, 90, 20);
    gauge_set_value(&g_humi, 62, "%d %%");

    arc_gauge_t g_batt = tesaiot_arc_gauge_create(parent, "Battery",
        lv_palette_main(LV_PALETTE_GREEN), 0, 100, 140);
    lv_obj_align(g_batt.arc->parent, LV_ALIGN_CENTER, 270, 20);
    gauge_set_value(&g_batt, 85, "%d %%");
}
