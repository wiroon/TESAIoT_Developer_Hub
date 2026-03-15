/**
 * @file    main_example.c
 * @brief   Flex Column Layout — Vertical list with flex_grow
 *
 * Five items stacked vertically using LV_FLEX_FLOW_COLUMN.
 * Items use flex_grow for proportional space distribution.
 */

#include "example_common.h"

static const char *items[] = {
    "Dashboard", "Sensors", "WiFi Config", "Playground", "Settings"
};
static const lv_palette_t colors[] = {
    LV_PALETTE_BLUE, LV_PALETTE_GREEN, LV_PALETTE_ORANGE, LV_PALETTE_PURPLE, LV_PALETTE_GREY
};

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Flex Column Layout");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Column container */
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 400, 330);
    lv_obj_align(col, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 8, 0);
    lv_obj_set_style_pad_all(col, 10, 0);

    for (int i = 0; i < 5; i++) {
        lv_obj_t *item = lv_obj_create(col);
        lv_obj_set_width(item, LV_PCT(100));
        lv_obj_set_flex_grow(item, 1);
        lv_obj_set_style_bg_color(item, lv_palette_lighten(colors[i], 4), 0);
        lv_obj_set_style_border_color(item, lv_palette_main(colors[i]), 0);
        lv_obj_set_style_border_width(item, 2, 0);
        lv_obj_set_style_radius(item, 8, 0);
        lv_obj_set_style_pad_all(item, 8, 0);

        lv_obj_t *lbl = lv_label_create(item);
        lv_label_set_text_fmt(lbl, LV_SYMBOL_RIGHT "  %s", items[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);
    }
}
