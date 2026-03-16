/**
 * @file    main_example.c
 * @brief   Sensor Auto-Poll — IPC snapshot with decoupled UI update
 *
 * LVGL timer at 200 ms reads IPC sensor snapshot and updates chart + labels.
 * A separate 50 ms LVGL timer tracks read count for display.
 */

#include "example_common.h"

/* ── Shared state ────────────────────────────────────────────────── */
static uint32_t s_read_count;

static lv_obj_t          *s_chart;
static lv_chart_series_t *s_ser_x, *s_ser_y, *s_ser_z;
static lv_obj_t          *s_lbl_status;
static lv_obj_t          *s_lbl_val;

/* ── Fast poll timer — increment read counter at 20 Hz ───────────── */
static void poll_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_read_count++;
}

/* ── UI update timer — display at 5 Hz ───────────────────────────── */
static void ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    if (!snap.has_bmi270) return;

    /* raw / 16384 * 1000 = milli-g */
    int32_t ax = snap.bmi270.ax * 1000 / 16384;
    int32_t ay = snap.bmi270.ay * 1000 / 16384;
    int32_t az = snap.bmi270.az * 1000 / 16384;

    lv_chart_set_next_value(s_chart, s_ser_x, ax);
    lv_chart_set_next_value(s_chart, s_ser_y, ay);
    lv_chart_set_next_value(s_chart, s_ser_z, az);

    lv_label_set_text_fmt(s_lbl_status, "Reads: %u  |  Tick: %u",
                          (unsigned)s_read_count,
                          (unsigned)xTaskGetTickCount());

    lv_label_set_text_fmt(s_lbl_val, "X:%d  Y:%d  Z:%d mg",
                          (int)ax, (int)ay, (int)az);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = example_label_create(parent,
        "I15 \xe2\x80\x94 Sensor Auto-Poll",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Status label */
    s_lbl_status = example_label_create(parent, "Reads: 0  |  Tick: 0",
        &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
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
    lv_obj_set_style_bg_color(s_chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);

    s_ser_x = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_RED),   LV_CHART_AXIS_PRIMARY_Y);
    s_ser_y = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    s_ser_z = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_BLUE),  LV_CHART_AXIS_PRIMARY_Y);

    /* Value label */
    s_lbl_val = example_label_create(parent, "X:--  Y:--  Z:-- mg",
        &lv_font_montserrat_16, lv_color_white());
    lv_obj_align(s_lbl_val, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* Init */
    s_read_count = 0;

    /* Poll timer (fast) + UI timer (slow) */
    lv_timer_create(poll_timer_cb, 50, NULL);
    lv_timer_create(ui_timer_cb, 200, NULL);
}
