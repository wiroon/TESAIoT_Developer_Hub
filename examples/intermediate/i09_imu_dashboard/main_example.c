/**
 * @file    main_example.c
 * @brief   IMU Dashboard — 6-axis BMI270 with dual charts + 6 value labels
 *
 * Two card containers: Accelerometer (top) and Gyroscope (bottom).
 * Each card has a 3-series line chart and 3 value labels.
 */

#include "example_common.h"
#include "sensor_bmi270.h"

/* ── Card dimensions ─────────────────────────────────────────────── */
#define CARD_W      720
#define CARD_H      175
#define CHART_W     680
#define CHART_H     110

/* ── State ───────────────────────────────────────────────────────── */
static lv_obj_t          *s_chart_acc, *s_chart_gyr;
static lv_chart_series_t *s_acc[3], *s_gyr[3];
static lv_obj_t          *s_lbl_acc[3], *s_lbl_gyr[3];

static const lv_palette_t s_pal[3] = {
    LV_PALETTE_RED, LV_PALETTE_GREEN, LV_PALETTE_BLUE
};

/* ── Helper: create a sensor card ────────────────────────────────── */
static lv_obj_t *create_card(lv_obj_t *parent, const char *title, int y_off)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, CARD_W, CARD_H);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, y_off);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x2a4060), 0);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 4, 0);

    return card;
}

/* ── Helper: create chart inside card ────────────────────────────── */
static lv_obj_t *create_chart(lv_obj_t *card, int32_t ymin, int32_t ymax,
                               lv_chart_series_t *ser_out[3])
{
    lv_obj_t *chart = lv_chart_create(card);
    lv_obj_set_size(chart, CHART_W, CHART_H);
    lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 18);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 60);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, ymin, ymax);
    lv_chart_set_div_line_count(chart, 3, 6);
    lv_obj_set_style_line_width(chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart, 0, 0);

    for (int i = 0; i < 3; i++) {
        ser_out[i] = lv_chart_add_series(chart,
                                         lv_palette_main(s_pal[i]),
                                         LV_CHART_AXIS_PRIMARY_Y);
    }
    return chart;
}

/* ── Timer — 50 ms sensor read ───────────────────────────────────── */
static void imu_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensor_bmi270_data_t d;
    if (sensor_bmi270_read(&d) != 0) return;

    int32_t ax = (int32_t)(d.accel_x * 1000.0f);
    int32_t ay = (int32_t)(d.accel_y * 1000.0f);
    int32_t az = (int32_t)(d.accel_z * 1000.0f);
    int32_t gx = (int32_t)(d.gyro_x);
    int32_t gy = (int32_t)(d.gyro_y);
    int32_t gz = (int32_t)(d.gyro_z);

    int32_t acc_vals[3] = { ax, ay, az };
    int32_t gyr_vals[3] = { gx, gy, gz };

    for (int i = 0; i < 3; i++) {
        lv_chart_set_next_value(s_chart_acc, s_acc[i], acc_vals[i]);
        lv_chart_set_next_value(s_chart_gyr, s_gyr[i], gyr_vals[i]);
    }

    const char *axis[3] = { "X", "Y", "Z" };
    for (int i = 0; i < 3; i++) {
        lv_label_set_text_fmt(s_lbl_acc[i], "%s:%d", axis[i], (int)acc_vals[i]);
        lv_label_set_text_fmt(s_lbl_gyr[i], "%s:%d", axis[i], (int)gyr_vals[i]);
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I09 — IMU Dashboard");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    /* Accelerometer card */
    lv_obj_t *card_acc = create_card(parent, "Accelerometer (mg)", 28);
    s_chart_acc = create_chart(card_acc, -2000, 2000, s_acc);

    for (int i = 0; i < 3; i++) {
        s_lbl_acc[i] = lv_label_create(card_acc);
        lv_label_set_text(s_lbl_acc[i], "--");
        lv_obj_set_style_text_color(s_lbl_acc[i], lv_palette_main(s_pal[i]), 0);
        lv_obj_set_style_text_font(s_lbl_acc[i], &lv_font_montserrat_14, 0);
        lv_obj_align(s_lbl_acc[i], LV_ALIGN_BOTTOM_LEFT, 4 + i * 230, -2);
    }

    /* Gyroscope card */
    lv_obj_t *card_gyr = create_card(parent, "Gyroscope (dps)", 210);
    s_chart_gyr = create_chart(card_gyr, -500, 500, s_gyr);

    for (int i = 0; i < 3; i++) {
        s_lbl_gyr[i] = lv_label_create(card_gyr);
        lv_label_set_text(s_lbl_gyr[i], "--");
        lv_obj_set_style_text_color(s_lbl_gyr[i], lv_palette_main(s_pal[i]), 0);
        lv_obj_set_style_text_font(s_lbl_gyr[i], &lv_font_montserrat_14, 0);
        lv_obj_align(s_lbl_gyr[i], LV_ALIGN_BOTTOM_LEFT, 4 + i * 230, -2);
    }

    /* Init sensor + timer */
    sensor_bmi270_init();
    lv_timer_create(imu_timer_cb, 50, NULL);
}
