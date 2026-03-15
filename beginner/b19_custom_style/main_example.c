/**
 * @file    main_example.c
 * @brief   Custom Style — Reusable lv_style_t applied to buttons
 *
 * Two style definitions (primary, secondary) are created and applied
 * to multiple buttons, demonstrating style reuse and pressed states.
 */

#include "example_common.h"

static lv_style_t style_primary;
static lv_style_t style_secondary;
static lv_style_t style_pressed;

static void init_styles(void)
{
    /* Primary style — blue, rounded, shadow */
    lv_style_init(&style_primary);
    lv_style_set_bg_color(&style_primary, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_opa(&style_primary, LV_OPA_COVER);
    lv_style_set_radius(&style_primary, 12);
    lv_style_set_shadow_width(&style_primary, 10);
    lv_style_set_shadow_opa(&style_primary, LV_OPA_30);
    lv_style_set_shadow_offset_y(&style_primary, 4);
    lv_style_set_text_color(&style_primary, lv_color_white());
    lv_style_set_pad_all(&style_primary, 15);

    /* Secondary style — outline, no fill */
    lv_style_init(&style_secondary);
    lv_style_set_bg_opa(&style_secondary, LV_OPA_TRANSP);
    lv_style_set_border_color(&style_secondary, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_border_width(&style_secondary, 2);
    lv_style_set_radius(&style_secondary, 12);
    lv_style_set_text_color(&style_secondary, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_pad_all(&style_secondary, 15);

    /* Pressed state */
    lv_style_init(&style_pressed);
    lv_style_set_bg_opa(&style_pressed, LV_OPA_70);
    lv_style_set_translate_y(&style_pressed, 2);
}

void example_main(lv_obj_t *parent)
{
    init_styles();

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Custom Styles");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Primary buttons */
    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_remove_style_all(btn);
        lv_obj_add_style(btn, &style_primary, 0);
        lv_obj_add_style(btn, &style_pressed, LV_STATE_PRESSED);
        lv_obj_set_size(btn, 200, 55);
        lv_obj_align(btn, LV_ALIGN_CENTER, -130, -30 + i * 70);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text_fmt(lbl, "Primary %d", i + 1);
        lv_obj_center(lbl);
    }

    /* Secondary buttons */
    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_remove_style_all(btn);
        lv_obj_add_style(btn, &style_secondary, 0);
        lv_obj_add_style(btn, &style_pressed, LV_STATE_PRESSED);
        lv_obj_set_size(btn, 200, 55);
        lv_obj_align(btn, LV_ALIGN_CENTER, 130, -30 + i * 70);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text_fmt(lbl, "Secondary %d", i + 1);
        lv_obj_center(lbl);
    }
}
