/**
 * @file    main_example.c
 * @brief   Page Render Loop — Timer-driven render updating sensor data per frame
 *
 * 100 ms render timer reads BMI270 accel magnitude via IPC snapshot,
 * updates chart + labels + frame counter with FPS estimate.
 */

#include "example_common.h"

static lv_obj_t          *s_chart;
static lv_chart_series_t *s_ser;
static lv_obj_t          *s_lbl_val;
static lv_obj_t          *s_lbl_frame;
static lv_obj_t          *s_lbl_fps;
static uint32_t           s_frame_count;
static uint32_t           s_last_sec_tick;
static uint32_t           s_frames_this_sec;
static uint32_t           s_fps;

/* ── Render callback — called each timer tick ────────────────────── */
static void render_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    if (!snap.has_bmi270) return;

    /* Convert raw to g, then compute magnitude in milli-g */
    float ax = snap.bmi270.ax / 16384.0f;
    float ay = snap.bmi270.ay / 16384.0f;
    float az = snap.bmi270.az / 16384.0f;

    int32_t mag = (int32_t)(sqrtf(ax * ax + ay * ay + az * az) * 1000.0f);

    lv_chart_set_next_value(s_chart, s_ser, mag);
    lv_label_set_text_fmt(s_lbl_val, "|A|: %d mg", (int)mag);

    s_frame_count++;
    s_frames_this_sec++;
    lv_label_set_text_fmt(s_lbl_frame, "Frame: %u", (unsigned)s_frame_count);

    /* Simple FPS estimate */
    uint32_t now = xTaskGetTickCount();
    if ((now - s_last_sec_tick) >= configTICK_RATE_HZ) {
        s_fps = s_frames_this_sec;
        s_frames_this_sec = 0;
        s_last_sec_tick = now;
    }
    lv_label_set_text_fmt(s_lbl_fps, "Render rate: ~%u Hz", (unsigned)s_fps);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_frame_count    = 0;
    s_frames_this_sec = 0;
    s_fps            = 0;
    s_last_sec_tick  = xTaskGetTickCount();

    lv_obj_t *title = example_label_create(parent,
        "I27 \xe2\x80\x94 Page Render Loop",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 16, 4);

    /* FPS label */
    s_lbl_fps = example_label_create(parent, "Render rate: ~0 Hz",
        &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(s_lbl_fps, LV_ALIGN_TOP_RIGHT, -16, 8);

    /* Chart: accel magnitude */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 700, 220);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, 0);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 80);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 3000);
    lv_chart_set_div_line_count(s_chart, 4, 8);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);

    s_ser = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_AMBER),
                                LV_CHART_AXIS_PRIMARY_Y);

    /* Value label */
    s_lbl_val = example_label_create(parent, "|A|: -- mg",
        &lv_font_montserrat_16, lv_palette_main(LV_PALETTE_AMBER));
    lv_obj_align(s_lbl_val, LV_ALIGN_BOTTOM_LEFT, 30, -10);

    /* Frame counter */
    s_lbl_frame = example_label_create(parent, "Frame: 0",
        &lv_font_montserrat_16, UI_COLOR_SUCCESS);
    lv_obj_align(s_lbl_frame, LV_ALIGN_BOTTOM_RIGHT, -30, -10);

    /* Start render timer */
    lv_timer_create(render_cb, 100, NULL);
}
