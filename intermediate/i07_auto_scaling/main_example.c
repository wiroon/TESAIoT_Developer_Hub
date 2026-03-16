/**
 * @file    main_example.c
 * @brief   Auto-Scaling Chart — Dynamic Y-axis range based on data min/max
 *
 * Scans the chart buffer after each sample to recalculate Y-axis bounds
 * with a 10 % margin, so data always fills the visible area.
 */

#include "example_common.h"

#define POINT_CNT   80
#define MARGIN_PCT  10    /* % above/below data extremes */

static lv_obj_t          *s_chart;
static lv_chart_series_t *s_series;
static lv_obj_t          *s_lbl_range;
static lv_obj_t          *s_lbl_value;
static uint32_t           s_sample_count;

/* ── Recalculate Y-axis range from buffer contents ───────────────── */
static void auto_scale(void)
{
    int32_t *arr = lv_chart_get_y_array(s_chart, s_series);
    uint32_t cnt = LV_MIN(s_sample_count, POINT_CNT);
    if (cnt < 2) return;

    int32_t vmin = INT32_MAX;
    int32_t vmax = INT32_MIN;
    for (uint32_t i = 0; i < cnt; i++) {
        if (arr[i] < vmin) vmin = arr[i];
        if (arr[i] > vmax) vmax = arr[i];
    }

    /* Add margin */
    int32_t span = vmax - vmin;
    if (span < 20) span = 20;   /* minimum visible span */
    int32_t margin = span * MARGIN_PCT / 100;
    vmin -= margin;
    vmax += margin;

    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, vmin, vmax);
    lv_label_set_text_fmt(s_lbl_range, "Range: [%d .. %d]", (int)vmin, (int)vmax);
}

/* ── Timer callback ──────────────────────────────────────────────── */
static void timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    if (!snap.has_bmi270) return;

    /* raw / 16384 * 1000 = milli-g */
    int32_t val = snap.bmi270.ax * 1000 / 16384;
    lv_chart_set_next_value(s_chart, s_series, val);
    s_sample_count++;

    auto_scale();
    lv_label_set_text_fmt(s_lbl_value, "X: %d mg", (int)val);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "I07 \xe2\x80\x94 Auto-Scaling Chart",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 16, 6);

    /* Range label */
    s_lbl_range = example_label_create(parent, "Range: [--]",
        &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(s_lbl_range, LV_ALIGN_TOP_RIGHT, -16, 10);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 700, 250);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, 10);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, POINT_CNT);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, -1000, 1000);
    lv_chart_set_div_line_count(s_chart, 5, 8);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);

    s_series = lv_chart_add_series(s_chart,
                                   lv_palette_main(LV_PALETTE_AMBER),
                                   LV_CHART_AXIS_PRIMARY_Y);

    /* Value label */
    s_lbl_value = example_label_create(parent, "X: -- mg",
        &lv_font_montserrat_16, lv_palette_main(LV_PALETTE_AMBER));
    lv_obj_align(s_lbl_value, LV_ALIGN_BOTTOM_MID, 0, -10);

    /* Init */
    s_sample_count = 0;
    lv_timer_create(timer_cb, 50, NULL);
}
