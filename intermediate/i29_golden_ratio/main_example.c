/**
 * @file    main_example.c
 * @brief   Golden Ratio Layout — Two-column split with live chart
 *
 * Major column (61.8%) holds a live chart updated by timer.
 * Minor column (38.2%) shows sensor info labels.
 *
 * Functions:
 *   create_panel()           — Build a dark bordered panel with title
 *   populate_chart()         — Initialize chart with series and random data
 *   chart_update_timer_cb()  — Timer callback: append new data point
 *   example_main()           — Entry point: compose golden ratio layout
 */

#include "example_common.h"

#define PHI_MAJOR    0.618f
#define PHI_MINOR    0.382f
#define TOTAL_W      740
#define TOTAL_H      330
#define GAP          10

static lv_obj_t         *s_chart;
static lv_chart_series_t *s_series;

/* ── Create a dark bordered panel with title ──────────────────────── */
static lv_obj_t *create_panel(lv_obj_t *parent, const char *title_text,
                               int w, int h, lv_palette_t accent,
                               lv_align_t align, int x_ofs, int y_ofs)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, w, h);
    lv_obj_align(panel, align, x_ofs, y_ofs);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(panel, 12, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_palette_main(accent), 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(panel);
    lv_label_set_text(lbl, title_text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(accent), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 8, 4);

    return panel;
}

/* ── Initialize chart with series and seed data ───────────────────── */
static void populate_chart(lv_obj_t *panel, int chart_w, int chart_h)
{
    s_chart = lv_chart_create(panel);
    lv_obj_set_size(s_chart, chart_w, chart_h);
    lv_obj_align(s_chart, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 60);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_chart, 0, 0);

    s_series = lv_chart_add_series(s_chart,
        lv_palette_main(LV_PALETTE_AMBER), LV_CHART_AXIS_PRIMARY_Y);
    for (int i = 0; i < 60; i++) {
        lv_chart_set_next_value(s_chart, s_series, lv_rand(20, 80));
    }
}

/* ── Timer callback: append new chart data ────────────────────────── */
static void chart_update_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    lv_chart_set_next_value(s_chart, s_series, lv_rand(20, 80));
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I29 — Golden Ratio Layout");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    int major_w = (int)(TOTAL_W * PHI_MAJOR);
    int minor_w = (int)(TOTAL_W * PHI_MINOR) - GAP;
    int x_start = -(TOTAL_W / 2);

    /* Major column (61.8%) — chart */
    char maj_text[32];
    snprintf(maj_text, sizeof(maj_text), "Major (%.1f%%)", PHI_MAJOR * 100);
    lv_obj_t *col_major = create_panel(parent, maj_text,
        major_w, TOTAL_H, LV_PALETTE_AMBER,
        LV_ALIGN_CENTER, x_start + major_w / 2, 20);
    populate_chart(col_major, major_w - 24, TOTAL_H - 40);

    /* Minor column (38.2%) — info panel */
    char min_text[32];
    snprintf(min_text, sizeof(min_text), "Minor (%.1f%%)", PHI_MINOR * 100);
    lv_obj_t *col_minor = create_panel(parent, min_text,
        minor_w, TOTAL_H, LV_PALETTE_TEAL,
        LV_ALIGN_CENTER, x_start + major_w + GAP + minor_w / 2, 20);

    static const char *labels[] = {
        "Sensor: BMI270", "Mode: Active", "Rate: 50 Hz",
        "Range: 2g", "Status: OK", "Samples: 1240"
    };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *lbl = lv_label_create(col_minor);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 12, 30 + i * 44);
    }

    /* Live chart update every 500ms */
    lv_timer_create(chart_update_timer_cb, 500, NULL);
}
