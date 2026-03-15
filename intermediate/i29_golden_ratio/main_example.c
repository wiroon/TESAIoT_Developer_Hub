/**
 * @file    main_example.c
 * @brief   Golden Ratio Layout — 61.8% / 38.2% two-column split
 *
 * Major column holds a chart; minor column holds value labels.
 * Uses UI_PHI_MAJOR and UI_PHI_MINOR constants.
 */

#include "example_common.h"

#define UI_PHI_MAJOR    0.618f
#define UI_PHI_MINOR    0.382f
#define TOTAL_W         740
#define TOTAL_H         330
#define GAP             10

void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I29 — Golden Ratio Layout");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    int major_w = (int)(TOTAL_W * UI_PHI_MAJOR);
    int minor_w = (int)(TOTAL_W * UI_PHI_MINOR) - GAP;
    int x_start = -(TOTAL_W / 2);

    /* ── Major column (61.8%) — chart ────────────────────────────── */
    lv_obj_t *col_major = lv_obj_create(parent);
    lv_obj_set_size(col_major, major_w, TOTAL_H);
    lv_obj_align(col_major, LV_ALIGN_CENTER, x_start + major_w / 2, 20);
    lv_obj_set_style_bg_color(col_major, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(col_major, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(col_major, 12, 0);
    lv_obj_set_style_border_width(col_major, 1, 0);
    lv_obj_set_style_border_color(col_major, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_clear_flag(col_major, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *maj_title = lv_label_create(col_major);
    lv_label_set_text_fmt(maj_title, "Major (%.1f%%)", UI_PHI_MAJOR * 100);
    lv_obj_set_style_text_font(maj_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(maj_title, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_align(maj_title, LV_ALIGN_TOP_LEFT, 8, 4);

    lv_obj_t *chart = lv_chart_create(col_major);
    lv_obj_set_size(chart, major_w - 24, TOTAL_H - 40);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 60);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_line_width(chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart, 0, 0);

    lv_chart_series_t *ser = lv_chart_add_series(chart,
        lv_palette_main(LV_PALETTE_AMBER), LV_CHART_AXIS_PRIMARY_Y);
    for (int i = 0; i < 60; i++) {
        lv_chart_set_next_value(chart, ser, lv_rand(20, 80));
    }

    /* ── Minor column (38.2%) — info panel ───────────────────────── */
    lv_obj_t *col_minor = lv_obj_create(parent);
    lv_obj_set_size(col_minor, minor_w, TOTAL_H);
    lv_obj_align(col_minor, LV_ALIGN_CENTER,
                 x_start + major_w + GAP + minor_w / 2, 20);
    lv_obj_set_style_bg_color(col_minor, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(col_minor, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(col_minor, 12, 0);
    lv_obj_set_style_border_width(col_minor, 1, 0);
    lv_obj_set_style_border_color(col_minor, lv_palette_main(LV_PALETTE_TEAL), 0);
    lv_obj_clear_flag(col_minor, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *min_title = lv_label_create(col_minor);
    lv_label_set_text_fmt(min_title, "Minor (%.1f%%)", UI_PHI_MINOR * 100);
    lv_obj_set_style_text_font(min_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(min_title, lv_palette_main(LV_PALETTE_TEAL), 0);
    lv_obj_align(min_title, LV_ALIGN_TOP_LEFT, 8, 4);

    static const char *labels[] = {
        "Sensor: BMI270", "Mode: Active", "Rate: 50 Hz",
        "Range: 2g", "Status: OK", "Samples: 1240"
    };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *lbl = lv_label_create(col_minor);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 12, 30 + i * 44);
    }
}
