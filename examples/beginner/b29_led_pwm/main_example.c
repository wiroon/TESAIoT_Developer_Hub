/**
 * @file    main_example.c
 * @brief   LED Brightness — Slider controls virtual LED brightness
 *
 * A slider adjusts the lv_led brightness from 0 to 255.
 * Simulates PWM duty cycle control using the LVGL LED widget.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *led_widget;
    lv_obj_t *lbl_pct;
} pwm_ctx_t;

static pwm_ctx_t ctx;

static void slider_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);

    /* Map 0-100% to 0-255 brightness */
    uint8_t brightness = (uint8_t)((val * 255) / 100);
    lv_led_set_brightness(ctx.led_widget, brightness);

    /* Update label */
    lv_label_set_text_fmt(ctx.lbl_pct, "Brightness: %"PRId32"%%", val);
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent, "LED Brightness Control",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Virtual LED */
    ctx.led_widget = lv_led_create(parent);
    lv_obj_set_size(ctx.led_widget, 80, 80);
    lv_led_set_color(ctx.led_widget, lv_palette_main(LV_PALETTE_BLUE));
    lv_led_set_brightness(ctx.led_widget, 0);
    lv_obj_align(ctx.led_widget, LV_ALIGN_CENTER, 0, -50);

    /* Slider 0-100 */
    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, 350);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 30);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Percentage label */
    ctx.lbl_pct = example_label_create(parent, "Brightness: 0%",
                                        &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(ctx.lbl_pct, LV_ALIGN_CENTER, 0, 70);
}
