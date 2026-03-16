/**
 * @file    main_example.c
 * @brief   Chart with Legend — Color-coded legend items below a line chart
 *
 * Three series (Accel X/Y/Z) with a flex-row legend container showing
 * colored squares + labels that match each series color.
 */

#include "example_common.h"

static lv_obj_t          *s_chart;
static lv_chart_series_t *s_ser[3];

static const lv_palette_t s_colors[3] = {
    LV_PALETTE_RED, LV_PALETTE_GREEN, LV_PALETTE_BLUE
};
static const char *s_names[3] = { "Accel X", "Accel Y", "Accel Z" };

/* ── Helper: create one legend entry ─────────────────────────────── */
static void create_legend_item(lv_obj_t *row, lv_palette_t pal, const char *name)
{
    /* Item container */
    lv_obj_t *item = lv_obj_create(row);
    lv_obj_set_size(item, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(item, 6, 0);
    lv_obj_set_style_pad_all(item, 4, 0);
    lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(item, 0, 0);

    /* Color swatch */
    lv_obj_t *swatch = lv_obj_create(item);
    lv_obj_set_size(swatch, 16, 16);
    lv_obj_set_style_bg_color(swatch, lv_palette_main(pal), 0);
    lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(swatch, 3, 0);
    lv_obj_set_style_border_width(swatch, 0, 0);

    /* Label */
    lv_obj_t *lbl = lv_label_create(item);
    lv_label_set_text(lbl, name);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
}

/* ── Timer — push random values ──────────────────────────────────── */
static void timer_cb(lv_timer_t *timer)
{
    (void)timer;
    for (int i = 0; i < 3; i++) {
        lv_chart_set_next_value(s_chart, s_ser[i], lv_rand(-1000, 1000));
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I08 — Chart with Legend");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 700, 230);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, -20);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 80);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, -1200, 1200);
    lv_chart_set_div_line_count(s_chart, 5, 8);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);

    for (int i = 0; i < 3; i++) {
        s_ser[i] = lv_chart_add_series(s_chart,
                                       lv_palette_main(s_colors[i]),
                                       LV_CHART_AXIS_PRIMARY_Y);
    }

    /* Legend row */
    lv_obj_t *legend = lv_obj_create(parent);
    lv_obj_set_size(legend, 500, LV_SIZE_CONTENT);
    lv_obj_align(legend, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_flex_flow(legend, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(legend, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(legend, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(legend, 0, 0);
    lv_obj_set_style_pad_all(legend, 4, 0);

    for (int i = 0; i < 3; i++) {
        create_legend_item(legend, s_colors[i], s_names[i]);
    }

    /* Timer */
    lv_timer_create(timer_cb, 100, NULL);
}
