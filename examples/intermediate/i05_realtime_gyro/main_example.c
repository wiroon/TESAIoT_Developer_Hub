/**
 * @file    main_example.c
 * @brief   Real-Time Gyroscope — BMI270 gyro XYZ live chart via IPC
 *
 * 50 ms timer reads IPC sensor snapshot (angular velocity in dps) and
 * plots X/Y/Z on a 100-point line chart with numeric labels.
 */

#include "example_common.h"

static lv_obj_t          *s_chart;
static lv_chart_series_t *s_ser_x, *s_ser_y, *s_ser_z;
static lv_obj_t          *s_lbl_x, *s_lbl_y, *s_lbl_z;

/* ── Timer — read gyro via IPC at 20 Hz ──────────────────────────── */
static void gyro_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    if (!snap.has_bmi270) return;

    /* raw / 16.4 = dps (integer approximation) */
    int32_t gx = (int32_t)(snap.bmi270.gx * 10 / 164);
    int32_t gy = (int32_t)(snap.bmi270.gy * 10 / 164);
    int32_t gz = (int32_t)(snap.bmi270.gz * 10 / 164);

    lv_chart_set_next_value(s_chart, s_ser_x, gx);
    lv_chart_set_next_value(s_chart, s_ser_y, gy);
    lv_chart_set_next_value(s_chart, s_ser_z, gz);

    lv_label_set_text_fmt(s_lbl_x, "gX: %d dps", (int)gx);
    lv_label_set_text_fmt(s_lbl_y, "gY: %d dps", (int)gy);
    lv_label_set_text_fmt(s_lbl_z, "gZ: %d dps", (int)gz);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "I05 \xe2\x80\x94 Real-Time Gyroscope",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 700, 240);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, -10);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 100);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, -500, 500);
    lv_chart_set_div_line_count(s_chart, 5, 8);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);

    /* Series: Red=X, Green=Y, Blue=Z */
    s_ser_x = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_RED),   LV_CHART_AXIS_PRIMARY_Y);
    s_ser_y = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    s_ser_z = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_BLUE),  LV_CHART_AXIS_PRIMARY_Y);

    /* Value labels */
    s_lbl_x = example_label_create(parent, "gX: -- dps",
        &lv_font_montserrat_16, lv_palette_main(LV_PALETTE_RED));
    lv_obj_align(s_lbl_x, LV_ALIGN_BOTTOM_LEFT, 50, -10);

    s_lbl_y = example_label_create(parent, "gY: -- dps",
        &lv_font_montserrat_16, lv_palette_main(LV_PALETTE_GREEN));
    lv_obj_align(s_lbl_y, LV_ALIGN_BOTTOM_MID, 0, -10);

    s_lbl_z = example_label_create(parent, "gZ: -- dps",
        &lv_font_montserrat_16, lv_palette_main(LV_PALETTE_BLUE));
    lv_obj_align(s_lbl_z, LV_ALIGN_BOTTOM_RIGHT, -50, -10);

    /* Start timer */
    lv_timer_create(gyro_timer_cb, 50, NULL);
}
