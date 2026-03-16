/**
 * @file    main_example.c
 * @brief   LED On/Off — Buttons control virtual LED indicator
 *
 * ON and OFF buttons control an lv_led widget on screen.
 * Demonstrates lv_led_on(), lv_led_off(), and button event callbacks.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *led_widget;
    lv_obj_t *lbl_state;
} led_ctx_t;

static led_ctx_t ctx;

static void on_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_led_on(ctx.led_widget);
    lv_label_set_text(ctx.lbl_state, "LED: ON");
    lv_obj_set_style_text_color(ctx.lbl_state, UI_COLOR_SUCCESS, 0);
}

static void off_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_led_off(ctx.led_widget);
    lv_label_set_text(ctx.lbl_state, "LED: OFF");
    lv_obj_set_style_text_color(ctx.lbl_state, UI_COLOR_ERROR, 0);
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent, "LED On/Off Control",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    /* เปิด/ปิด LED */
    example_label_create(parent,
        "เปิด/ปิด LED",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Virtual LED indicator */
    ctx.led_widget = lv_led_create(parent);
    lv_obj_set_size(ctx.led_widget, 80, 80);
    lv_led_set_color(ctx.led_widget, lv_palette_main(LV_PALETTE_RED));
    lv_led_off(ctx.led_widget);
    lv_obj_align(ctx.led_widget, LV_ALIGN_CENTER, 0, -40);

    /* ON button */
    lv_obj_t *btn_on = lv_btn_create(parent);
    lv_obj_set_size(btn_on, 140, 55);
    lv_obj_align(btn_on, LV_ALIGN_CENTER, -90, 40);
    lv_obj_set_style_bg_color(btn_on, UI_COLOR_SUCCESS, 0);
    lv_obj_add_event_cb(btn_on, on_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_on = lv_label_create(btn_on);
    lv_label_set_text(lbl_on, "ON");
    lv_obj_center(lbl_on);

    /* OFF button */
    lv_obj_t *btn_off = lv_btn_create(parent);
    lv_obj_set_size(btn_off, 140, 55);
    lv_obj_align(btn_off, LV_ALIGN_CENTER, 90, 40);
    lv_obj_set_style_bg_color(btn_off, UI_COLOR_ERROR, 0);
    lv_obj_add_event_cb(btn_off, off_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_off = lv_label_create(btn_off);
    lv_label_set_text(lbl_off, "OFF");
    lv_obj_center(lbl_off);

    /* State label */
    ctx.lbl_state = example_label_create(parent, "LED: OFF",
                                          &lv_font_montserrat_16, UI_COLOR_ERROR);
    lv_obj_align(ctx.lbl_state, LV_ALIGN_CENTER, 0, 100);
}
