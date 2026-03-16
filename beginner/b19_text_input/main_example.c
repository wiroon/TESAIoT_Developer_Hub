/**
 * @file main_example.c
 * @brief B19 — Text Input: Textarea with on-screen keyboard.
 *
 * Demonstrates the LVGL textarea widget paired with a keyboard. Tapping
 * the textarea opens the keyboard; typed text appears in real time.
 */

#include "example_common.h"

static lv_obj_t *ta;
static lv_obj_t *kb;
static lv_obj_t *echo_label;

static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_FOCUSED) {
        /* Show keyboard when textarea is focused */
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        /* Echo current text */
        const char *txt = lv_textarea_get_text(ta);
        lv_label_set_text_fmt(echo_label, "You typed: %s", txt);
    }
}

static void kb_ready_cb(lv_event_t *e)
{
    (void)e;
    /* Hide keyboard when OK/Enter is pressed */
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* ---- title ---- */
    lv_obj_t *title = example_label_create(parent, "Text Input",
                                           &lv_font_montserrat_24,
                                           UI_COLOR_TEXT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* ---- textarea ---- */
    ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, 500, 50);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 50);
    lv_textarea_set_placeholder_text(ta, "Type something...");
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, 0);
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x1E3A5F), 0);
    lv_obj_set_style_text_color(ta, UI_COLOR_TEXT, 0);
    lv_obj_set_style_border_color(ta, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(ta, 2, 0);

    lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, NULL);

    /* ---- echo label ---- */
    echo_label = example_label_create(parent, "You typed: ",
                                      &lv_font_montserrat_20,
                                      UI_COLOR_PRIMARY);
    lv_obj_align(echo_label, LV_ALIGN_TOP_MID, 0, 115);

    /* ---- keyboard (initially hidden) ---- */
    kb = lv_keyboard_create(parent);
    lv_obj_set_size(kb, 780, 250);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_ready_cb, LV_EVENT_READY, NULL);
}
