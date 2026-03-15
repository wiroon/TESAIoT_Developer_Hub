/**
 * @file main_example.c
 * @brief B03 — Button Counter: A button that increments a counter label.
 *
 * Demonstrates LVGL button creation, event callbacks, and updating label
 * text dynamically in response to user interaction.
 */

#include "example_common.h"

static int32_t count = 0;
static lv_obj_t *count_label;

static void btn_click_cb(lv_event_t *e)
{
    (void)e;
    count++;
    lv_label_set_text_fmt(count_label, "%"PRId32, count);
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* ---- vertical flex layout ---- */
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 30, 0);

    /* ---- title ---- */
    example_label_create(parent, "Button Counter",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- large counter display ---- */
    count_label = example_label_create(parent, "0",
                                       &lv_font_montserrat_28,
                                       UI_COLOR_PRIMARY);

    /* ---- increment button ---- */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_set_style_bg_color(btn, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_add_event_cb(btn, btn_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "TAP +1");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(btn_lbl);
}
