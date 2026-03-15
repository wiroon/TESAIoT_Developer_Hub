/**
 * @file    main_example.c
 * @brief   Textarea Input — Text entry with on-screen keyboard
 *
 * A textarea widget paired with an LVGL keyboard for touch-based
 * text entry. The keyboard is linked to the textarea automatically.
 */

#include "example_common.h"

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Enter Device Name:");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Textarea */
    lv_obj_t *ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, 380, 50);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_placeholder_text(ta, "Type here...");
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, 32);

    /* On-screen keyboard */
    lv_obj_t *kb = lv_keyboard_create(parent);
    lv_obj_set_size(kb, LV_PCT(100), 180);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);
}
