/**
 * @file    main_example.c
 * @brief   Multi-Series Chart — 3 color-coded series for BMI270 accel X/Y/Z
 *
 * Red = X, Green = Y, Blue = Z.  Timer reads sensor at 50 ms intervals.
 */

#include "example_common.h"
#include "sensor_bmi270.h"

/* ── Static handles ──────────────────────────────────────────────── */
static lv_obj_t            *s_chart;
static lv_chart_series_t   *s_ser_x;
static lv_chart_series_t   *s_ser_y;
static lv_chart_series_t   *s_ser_z;
static lv_obj_t            *s_lbl_x;
static lv_obj_t            *s_lbl_y;
static lv_obj_t            *s_lbl_z;

/* ── Timer callback — read BMI270 accel, push to chart ───────────── */
static void timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensor_bmi270_data_t data;
    if (sensor_bmi270_read(&data) != 0) {
        return;   /* sensor read failed — skip this tick */
    }

    /* Accelerometer values in milli-g */
    int32_t ax = (int32_t)(data.accel_x * 1000.0f);
    int32_t ay = (int32_t)(data.accel_y * 1000.0f);
    int32_t az = (int32_t)(data.accel_z * 1000.0f);

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
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I02 — Multi-Series Accel Chart");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
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
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);

    /* Three series: Red=X, Green=Y, Blue=Z */
    s_ser_x = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_RED),   LV_CHART_AXIS_PRIMARY_Y);
    s_ser_y = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    s_ser_z = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_BLUE),  LV_CHART_AXIS_PRIMARY_Y);

    /* Value labels row */
    s_lbl_x = lv_label_create(parent);
    lv_label_set_text(s_lbl_x, "X: -- mg");
    lv_obj_set_style_text_color(s_lbl_x, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_font(s_lbl_x, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_x, LV_ALIGN_BOTTOM_LEFT, 60, -12);

    s_lbl_y = lv_label_create(parent);
    lv_label_set_text(s_lbl_y, "Y: -- mg");
    lv_obj_set_style_text_color(s_lbl_y, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_text_font(s_lbl_y, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_y, LV_ALIGN_BOTTOM_MID, 0, -12);

    s_lbl_z = lv_label_create(parent);
    lv_label_set_text(s_lbl_z, "Z: -- mg");
    lv_obj_set_style_text_color(s_lbl_z, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_text_font(s_lbl_z, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_z, LV_ALIGN_BOTTOM_RIGHT, -60, -12);

    /* Sensor init + timer */
    sensor_bmi270_init();
    lv_timer_create(timer_cb, 50, NULL);
}
