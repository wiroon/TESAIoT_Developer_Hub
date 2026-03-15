/**
 * @file    main_example.c
 * @brief   Hello World Label — Display Hello BENTO on screen with LVGL labels
 *
 * Three labels are placed at top, center, and bottom of the parent container.
 * Each uses a different palette color to demonstrate basic LVGL text styling.
 */

#include "example_common.h"

void example_main(lv_obj_t *parent)
{
    /* Title label at the top */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "BENTO : : Make Anything.");
    lv_obj_set_style_text_color(lbl_title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 20);

    /* Main greeting at center */
    lv_obj_t *lbl_hello = lv_label_create(parent);
    lv_label_set_text(lbl_hello, "Hello BENTO!");
    lv_obj_set_style_text_color(lbl_hello, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_text_font(lbl_hello, &lv_font_montserrat_28, 0);
    lv_obj_align(lbl_hello, LV_ALIGN_CENTER, 0, 0);

    /* Description label at the bottom */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "PSoC Edge E84 Developer Hub — Beginner Example B01");
    lv_obj_set_style_text_color(lbl_desc, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_BOTTOM_MID, 0, -20);
}
