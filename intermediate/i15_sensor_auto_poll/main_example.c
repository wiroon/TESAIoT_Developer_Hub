/**
 * @file    main_example.c
 * @brief   Sensor Auto-Poll — FreeRTOS timer + LVGL timer decoupled pattern
 *
 * FreeRTOS xTimerCreate reads BMI270 at 50 ms (acquisition).
 * LVGL timer at 200 ms updates chart + labels (rendering).
 */

#include "example_common.h"
#include "sensor_bmi270.h"

/* ── Shared sensor buffer (FreeRTOS timer → LVGL timer) ──────────── */
static volatile struct {
    int32_t ax, ay, az;
    uint32_t read_count;
    uint32_t last_tick;
} s_sensor_buf;

static lv_obj_t          *s_chart;
static lv_chart_series_t *s_ser_x, *s_ser_y, *s_ser_z;
static lv_obj_t          *s_lbl_status;
static lv_obj_t          *s_lbl_val;

/* ── FreeRTOS timer — sensor acquisition at 20 Hz ────────────────── */
static void sensor_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;

    sensor_bmi270_data_t d;
    if (sensor_bmi270_read(&d) != 0) return;

    s_sensor_buf.ax = (int32_t)(d.accel_x * 1000.0f);
    s_sensor_buf.ay = (int32_t)(d.accel_y * 1000.0f);
    s_sensor_buf.az = (int32_t)(d.accel_z * 1000.0f);
    s_sensor_buf.read_count++;
    s_sensor_buf.last_tick = xTaskGetTickCount();
}

/* ── LVGL timer — UI update at 5 Hz ─────────────────────────────── */
static void ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    int32_t ax = s_sensor_buf.ax;
    int32_t ay = s_sensor_buf.ay;
    int32_t az = s_sensor_buf.az;

    lv_chart_set_next_value(s_chart, s_ser_x, ax);
    lv_chart_set_next_value(s_chart, s_ser_y, ay);
    lv_chart_set_next_value(s_chart, s_ser_z, az);

    lv_label_set_text_fmt(s_lbl_status, "Reads: %u  |  Tick: %u",
                          (unsigned)s_sensor_buf.read_count,
                          (unsigned)s_sensor_buf.last_tick);

    lv_label_set_text_fmt(s_lbl_val, "X:%d  Y:%d  Z:%d mg",
                          (int)ax, (int)ay, (int)az);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I15 — Sensor Auto-Poll (FreeRTOS Timer)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Status label */
    s_lbl_status = lv_label_create(parent);
    lv_label_set_text(s_lbl_status, "Reads: 0  |  Tick: 0");
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_status, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_status, LV_ALIGN_TOP_MID, 0, 30);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 700, 220);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, 10);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 60);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000);
    lv_chart_set_div_line_count(s_chart, 4, 6);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);

    s_ser_x = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_RED),   LV_CHART_AXIS_PRIMARY_Y);
    s_ser_y = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    s_ser_z = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_BLUE),  LV_CHART_AXIS_PRIMARY_Y);

    /* Value label */
    s_lbl_val = lv_label_create(parent);
    lv_label_set_text(s_lbl_val, "X:--  Y:--  Z:-- mg");
    lv_obj_set_style_text_font(s_lbl_val, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_val, lv_color_white(), 0);
    lv_obj_align(s_lbl_val, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* Init sensor */
    sensor_bmi270_init();
    memset((void *)&s_sensor_buf, 0, sizeof(s_sensor_buf));

    /* FreeRTOS software timer for acquisition */
    TimerHandle_t htimer = xTimerCreate("sensor_poll", pdMS_TO_TICKS(50),
                                        pdTRUE, NULL, sensor_timer_cb);
    if (htimer != NULL) {
        xTimerStart(htimer, 0);
    }

    /* LVGL timer for UI update */
    lv_timer_create(ui_timer_cb, 200, NULL);
}
