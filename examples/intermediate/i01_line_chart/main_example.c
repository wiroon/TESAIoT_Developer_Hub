/**
 * @file    main_example.c
 * @brief   Single Line Chart — Real-time line chart with timer-driven data
 *
 * Demonstrates lv_chart in line mode with 100 data points and a 200ms
 * LVGL timer that feeds random values into the series buffer.
 *
 * Functions:
 *   setup_chart()            — Create and configure the chart widget
 *   update_value_label()     — Format and display the latest data value
 *   timer_cb()               — Timer callback: add new data point every 200ms
 *   example_main()           — Entry point: compose chart + readout UI
 */

#include "example_common.h"

/* ── Static handles ──────────────────────────────────────────────── */
static lv_obj_t   *s_chart;
static lv_obj_t   *s_lbl_value;
static lv_chart_series_t *s_series;

/* ── Create and configure the chart widget ───────────────────────── */
static void setup_chart(lv_obj_t *parent, int w, int h, int point_count)
{
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, w, h);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, 20);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, point_count);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(s_chart, 5, 8);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);
    lv_obj_set_style_border_color(s_chart, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_radius(s_chart, 8, 0);

    s_series = lv_chart_add_series(s_chart,
                                   lv_palette_main(LV_PALETTE_CYAN),
                                   LV_CHART_AXIS_PRIMARY_Y);

    /* Pre-fill with zeros so the line starts flat */
    for (int i = 0; i < point_count; i++) {
        lv_chart_set_next_value(s_chart, s_series, 0);
    }
}

/* ── Update the live value readout ───────────────────────────────── */
static void update_value_label(int32_t val)
{
    lv_label_set_text_fmt(s_lbl_value, "Value: %d", (int)val);
}

/* ── Timer callback — add one random point every 200 ms ────────── */
static void timer_cb(lv_timer_t *timer)
{
    (void)timer;
    int32_t val = lv_rand(10, 90);
    lv_chart_set_next_value(s_chart, s_series, val);
    update_value_label(val);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I01 — Single Line Chart");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* Live value label */
    s_lbl_value = lv_label_create(parent);
    lv_label_set_text(s_lbl_value, "Value: --");
    lv_obj_set_style_text_font(s_lbl_value, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_value, LV_ALIGN_TOP_RIGHT, -16, 12);

    /* Chart */
    setup_chart(parent, 700, 280, 100);

    /* Start the 200 ms update timer */
    lv_timer_create(timer_cb, 200, NULL);
}
