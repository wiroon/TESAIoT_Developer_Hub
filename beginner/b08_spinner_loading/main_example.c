/**
 * @file    main_example.c
 * @brief   Spinner Loading — Animated spinner with loading text
 *
 * A spinner widget provides visual loading feedback.
 * Styled with custom colors and accompanied by a status label.
 */

#include "example_common.h"

void example_main(lv_obj_t *parent)
{
    /* Spinner — 1000ms rotation, 60 degree arc */
    lv_obj_t *spinner = lv_spinner_create(parent);
    lv_obj_set_size(spinner, 120, 120);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -30);
    lv_spinner_set_anim_params(spinner, 1000, 200);

    /* Style the active arc */
    lv_obj_set_style_arc_color(spinner, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_INDICATOR);

    /* Style the background track */
    lv_obj_set_style_arc_color(spinner, lv_palette_lighten(LV_PALETTE_BLUE, 3), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_MAIN);

    /* Loading label */
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, "Loading...");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 60);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Spinner Demo");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
}
