/**
 * @file main_example.c
 * @brief B01 — Hello Label: Display a centered greeting on the 800x480 LCD.
 *
 * Demonstrates the simplest possible LVGL example: creating a single label
 * widget, setting its text, and centering it on the parent container.
 */

#include "example_common.h"

void example_main(lv_obj_t *parent)
{
    /* ---- background ---- */
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* ---- greeting label ---- */
    lv_obj_t *lbl = example_label_create(parent, "Hello, BENTO!",
                                         &lv_font_montserrat_28,
                                         UI_COLOR_PRIMARY);
    lv_obj_center(lbl);
}
