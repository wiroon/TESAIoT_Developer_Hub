/**
 * @file    main_example.c
 * @brief   Button-LED Link — Button press toggles LED indicator
 *
 * An on-screen button controls a virtual LED indicator.
 * Press the button to toggle the LED on/off with visual feedback.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *led_widget;
    lv_obj_t *lbl_state;
    bool      is_on;
} link_ctx_t;

static link_ctx_t ctx;

static void btn_press_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_PRESSED) return;

    ctx.is_on = true;
    lv_led_on(ctx.led_widget);
    lv_label_set_text(ctx.lbl_state, LV_SYMBOL_OK " Button PRESSED - LED ON");
    lv_obj_set_style_text_color(ctx.lbl_state, UI_COLOR_SUCCESS, 0);
}

static void btn_release_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    ctx.is_on = false;
    lv_led_off(ctx.led_widget);
    lv_label_set_text(ctx.lbl_state, "Button Released - LED OFF");
    lv_obj_set_style_text_color(ctx.lbl_state, UI_COLOR_TEXT_DIM, 0);
}

void example_main(lv_obj_t *parent)
{
    ctx.is_on = false;

    /* Title */
    lv_obj_t *title = example_label_create(parent, "Button-LED Link",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    /* ปุ่มกดเชื่อมต่อ LED */
    example_label_create(parent,
        "ปุ่มกดเชื่อมต่อ LED",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Instruction */
    lv_obj_t *info = example_label_create(parent, "Press and hold the button below",
                                           &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 40);

    /* Virtual LED */
    ctx.led_widget = lv_led_create(parent);
    lv_obj_set_size(ctx.led_widget, 100, 100);
    lv_led_set_color(ctx.led_widget, lv_palette_main(LV_PALETTE_YELLOW));
    lv_led_off(ctx.led_widget);
    lv_obj_align(ctx.led_widget, LV_ALIGN_CENTER, 0, -30);

    /* Button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_event_cb(btn, btn_press_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn, btn_release_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Hold Me");
    lv_obj_center(btn_lbl);

    /* State label */
    ctx.lbl_state = example_label_create(parent, "Button Released - LED OFF",
                                          &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
    lv_obj_align(ctx.lbl_state, LV_ALIGN_CENTER, 0, 100);
}
