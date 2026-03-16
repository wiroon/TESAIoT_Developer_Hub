/**
 * @file    main_example.c
 * @brief   LED Toggle — Single button toggles virtual LED state
 *
 * Each click toggles the lv_led widget between on and off.
 * Demonstrates state tracking with a boolean flag.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *led_widget;
    lv_obj_t *lbl_state;
    lv_obj_t *btn_label;
    bool      is_on;
} toggle_ctx_t;

static toggle_ctx_t ctx;

static void toggle_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    ctx.is_on = !ctx.is_on;

    if (ctx.is_on) {
        lv_led_on(ctx.led_widget);
        lv_label_set_text(ctx.lbl_state, "State: ON");
        lv_label_set_text(ctx.btn_label, LV_SYMBOL_POWER " Turn OFF");
        lv_obj_set_style_text_color(ctx.lbl_state, UI_COLOR_SUCCESS, 0);
    } else {
        lv_led_off(ctx.led_widget);
        lv_label_set_text(ctx.lbl_state, "State: OFF");
        lv_label_set_text(ctx.btn_label, LV_SYMBOL_POWER " Turn ON");
        lv_obj_set_style_text_color(ctx.lbl_state, UI_COLOR_ERROR, 0);
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.is_on = false;

    /* Title */
    lv_obj_t *title = example_label_create(parent, "LED Toggle",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Virtual LED */
    ctx.led_widget = lv_led_create(parent);
    lv_obj_set_size(ctx.led_widget, 100, 100);
    lv_led_set_color(ctx.led_widget, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_off(ctx.led_widget);
    lv_obj_align(ctx.led_widget, LV_ALIGN_CENTER, 0, -40);

    /* Toggle button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_event_cb(btn, toggle_btn_cb, LV_EVENT_CLICKED, NULL);

    ctx.btn_label = lv_label_create(btn);
    lv_label_set_text(ctx.btn_label, LV_SYMBOL_POWER " Turn ON");
    lv_obj_center(ctx.btn_label);

    /* State label */
    ctx.lbl_state = example_label_create(parent, "State: OFF",
                                          &lv_font_montserrat_16, UI_COLOR_ERROR);
    lv_obj_align(ctx.lbl_state, LV_ALIGN_CENTER, 0, 100);
}
