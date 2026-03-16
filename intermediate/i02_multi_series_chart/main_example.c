/**
 * @file    main_example.c
 * @brief   Multi-Series Chart — 3 color-coded series for BMI270 accel X/Y/Z
 *
 * Red = X, Green = Y, Blue = Z.  Timer reads IPC snapshot at 50 ms intervals.
 */

#include "example_common.h"

/* ── Static handles ──────────────────────────────────────────────── */
static lv_obj_t            *s_chart;
static lv_chart_series_t   *s_ser_x;
static lv_chart_series_t   *s_ser_y;
static lv_chart_series_t   *s_ser_z;
static lv_obj_t            *s_lbl_x;
static lv_obj_t            *s_lbl_y;
static lv_obj_t            *s_lbl_z;

/* ── Timer callback — read BMI270 accel via IPC, push to chart ──── */
static void timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    if (!snap.has_bmi270) return;

    /* Convert raw int16 to milli-g: raw / 16384.0 * 1000 */
    int32_t ax = (int32_t)(snap.bmi270.ax * 1000 / 16384);
    int32_t ay = (int32_t)(snap.bmi270.ay * 1000 / 16384);
    int32_t az = (int32_t)(snap.bmi270.az * 1000 / 16384);

    lv_chart_set_next_value(s_chart, s_ser_x, ax);
    lv_chart_set_next_value(s_chart, s_ser_y, ay);
    lv_chart_set_next_value(s_chart, s_ser_z, az);

    lv_label_set_text_fmt(s_lbl_x, "X: %d mg", (int)ax);
    lv_label_set_text_fmt(s_lbl_y, "Y: %d mg", (int)ay);
    lv_label_set_text_fmt(s_lbl_z, "Z: %d mg", (int)az);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "I02 \xe2\x80\x94 Multi-Series Accel Chart",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 700, 250);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, -10);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 80);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000);
    lv_chart_set_div_line_count(s_chart, 5, 8);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);

    /* Three series: Red=X, Green=Y, Blue=Z */
    s_ser_x = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_RED),   LV_CHART_AXIS_PRIMARY_Y);
    s_ser_y = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    s_ser_z = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_BLUE),  LV_CHART_AXIS_PRIMARY_Y);

    /* Value labels row */
    s_lbl_x = example_label_create(parent, "X: -- mg",
        &lv_font_montserrat_16, lv_palette_main(LV_PALETTE_RED));
    lv_obj_align(s_lbl_x, LV_ALIGN_BOTTOM_LEFT, 60, -12);

    s_lbl_y = example_label_create(parent, "Y: -- mg",
        &lv_font_montserrat_16, lv_palette_main(LV_PALETTE_GREEN));
    lv_obj_align(s_lbl_y, LV_ALIGN_BOTTOM_MID, 0, -12);

    s_lbl_z = example_label_create(parent, "Z: -- mg",
        &lv_font_montserrat_16, lv_palette_main(LV_PALETTE_BLUE));
    lv_obj_align(s_lbl_z, LV_ALIGN_BOTTOM_RIGHT, -60, -12);

    /* Start timer */
    lv_timer_create(timer_cb, 50, NULL);
}
