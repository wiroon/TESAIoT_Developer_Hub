/**
 * @file    main_example.c
 * @brief   Flex Row Layout — Four buttons in horizontal flex row
 *
 * Demonstrates LV_FLEX_FLOW_ROW with gap spacing and centered alignment.
 * Four colored buttons are arranged horizontally.
 */

#include "example_common.h"

static const char *btn_labels[] = { "Home", "Sensors", "WiFi", "Settings" };
static const lv_palette_t btn_colors[] = {
    LV_PALETTE_BLUE, LV_PALETTE_GREEN, LV_PALETTE_ORANGE, LV_PALETTE_GREY
};

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Flex Row Layout");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Flex row container */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 700, 80);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 15, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(row);
        lv_obj_set_size(btn, 140, 50);
        lv_obj_set_style_bg_color(btn, lv_palette_main(btn_colors[i]), 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_labels[i]);
        lv_obj_center(lbl);
    }
}
