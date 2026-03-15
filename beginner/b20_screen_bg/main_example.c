/**
 * @file    main_example.c
 * @brief   Screen Background — Vertical gradient background
 *
 * Sets a vertical gradient from dark blue to light blue on the
 * parent container, with a label overlay.
 */

#include "example_common.h"

void example_main(lv_obj_t *parent)
{
    /* Set gradient background on parent */
    lv_obj_set_style_bg_color(parent, lv_color_make(10, 20, 60), 0);
    lv_obj_set_style_bg_grad_color(parent, lv_color_make(40, 100, 180), 0);
    lv_obj_set_style_bg_grad_dir(parent, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Title with white text */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "BENTO : : Make Anything.");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -30);

    /* Subtitle */
    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "PSoC Edge E84 Developer Hub");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sub, lv_color_make(180, 200, 255), 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 20);

    /* Version badge */
    lv_obj_t *badge = lv_obj_create(parent);
    lv_obj_set_size(badge, 160, 40);
    lv_obj_set_style_bg_color(badge, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_20, 0);
    lv_obj_set_style_radius(badge, 20, 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_border_color(badge, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_border_opa(badge, LV_OPA_40, 0);
    lv_obj_align(badge, LV_ALIGN_CENTER, 0, 70);

    lv_obj_t *ver = lv_label_create(badge);
    lv_label_set_text(ver, "v2.0.0");
    lv_obj_set_style_text_color(ver, lv_color_white(), 0);
    lv_obj_set_style_text_font(ver, &lv_font_montserrat_14, 0);
    lv_obj_center(ver);
}
