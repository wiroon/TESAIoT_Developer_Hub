/**
 * @file main_example.c
 * @brief B18 — Bar Chart: Five vertical bars as a static chart with labels.
 *
 * Demonstrates building a simple bar chart from basic LVGL objects.
 * Each bar has a category label below and a value label above.
 */

#include "example_common.h"

static struct {
    const char *name;
    int32_t     value;
    lv_color_t  color;
} chart_data[] = {
    { "Mon",  72, {0} },
    { "Tue",  45, {0} },
    { "Wed",  89, {0} },
    { "Thu",  56, {0} },
    { "Fri",  93, {0} },
};

#define BAR_COUNT  (sizeof(chart_data) / sizeof(chart_data[0]))
#define BAR_WIDTH  80
#define BAR_GAP    20
#define MAX_HEIGHT 250

void example_main(lv_obj_t *parent)
{
    /* ---- initialize colors at runtime (LVGL 9.2 lv_color_t has no .full) ---- */
    chart_data[0].color = lv_color_hex(0x4CAF50);
    chart_data[1].color = lv_color_hex(0x2196F3);
    chart_data[2].color = lv_color_hex(0xFF9800);
    chart_data[3].color = lv_color_hex(0xE040FB);
    chart_data[4].color = lv_color_hex(0xFF1744);

    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* ---- title ---- */
    lv_obj_t *title = example_label_create(parent, "Weekly Activity",
                                           &lv_font_montserrat_24,
                                           UI_COLOR_TEXT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    /* ---- chart area ---- */
    int32_t total_w = (int32_t)(BAR_COUNT * BAR_WIDTH + (BAR_COUNT - 1) * BAR_GAP);
    int32_t start_x = (800 - total_w) / 2;
    int32_t base_y  = 420;  /* bottom of bars */

    for (uint32_t i = 0; i < BAR_COUNT; i++) {
        int32_t x = start_x + (int32_t)(i * (BAR_WIDTH + BAR_GAP));
        int32_t h = (chart_data[i].value * MAX_HEIGHT) / 100;

        /* ---- bar rectangle ---- */
        lv_obj_t *bar = lv_obj_create(parent);
        lv_obj_set_size(bar, BAR_WIDTH, h);
        lv_obj_set_pos(bar, x, base_y - h);
        lv_obj_set_style_bg_color(bar, chart_data[i].color, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bar, 8, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);

        /* ---- value label above bar ---- */
        lv_obj_t *val_lbl = lv_label_create(parent);
        lv_label_set_text_fmt(val_lbl, "%"PRId32, chart_data[i].value);
        lv_obj_set_style_text_font(val_lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(val_lbl, UI_COLOR_TEXT, 0);
        lv_obj_set_pos(val_lbl, x + BAR_WIDTH / 2 - 12, base_y - h - 22);

        /* ---- category label below bar ---- */
        lv_obj_t *cat_lbl = lv_label_create(parent);
        lv_label_set_text(cat_lbl, chart_data[i].name);
        lv_obj_set_style_text_font(cat_lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(cat_lbl, lv_color_hex(0x888888), 0);
        lv_obj_set_pos(cat_lbl, x + BAR_WIDTH / 2 - 14, base_y + 8);
    }
}
