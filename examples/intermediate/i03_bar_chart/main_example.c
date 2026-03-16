/**
 * @file    main_example.c
 * @brief   Bar Chart — 6 bars with a regenerate button
 *
 * Demonstrates LV_CHART_TYPE_BAR and lv_chart_set_value_by_id.
 * Button click regenerates all bar values with random data.
 */

#include "example_common.h"

#define BAR_COUNT   6

static lv_obj_t          *s_chart;
static lv_chart_series_t *s_series;

/* ── Populate chart with random bar values ───────────────────────── */
static void fill_bars(void)
{
    for (int i = 0; i < BAR_COUNT; i++) {
        lv_chart_set_value_by_id(s_chart, s_series, i, lv_rand(10, 95));
    }
    lv_chart_refresh(s_chart);
}

/* ── Button event callback ───────────────────────────────────────── */
static void btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        fill_bars();
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I03 — Bar Chart");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 600, 240);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, -20);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(s_chart, BAR_COUNT);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(s_chart, 5, 0);
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);
    lv_obj_set_style_pad_column(s_chart, 24, 0);

    /* Series */
    s_series = lv_chart_add_series(s_chart,
                                   lv_palette_main(LV_PALETTE_ORANGE),
                                   LV_CHART_AXIS_PRIMARY_Y);
    fill_bars();

    /* Regenerate button */
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 160, 44);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE), 0);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Regenerate");
    lv_obj_center(btn_lbl);
}
