/**
 * @file    main_example.c
 * @brief   LED Brightness Control — Slider controls virtual LED brightness
 *
 * A slider (0-255) adjusts the brightness of an LVGL virtual LED widget.
 * A label displays the current brightness as a percentage.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *led;
    lv_obj_t *label;
} led_ctx_t;

static led_ctx_t ctx;

static void slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *slider = lv_event_get_target(e);
        led_ctx_t *c = (led_ctx_t *)lv_event_get_user_data(e);

        int32_t val = lv_slider_get_value(slider);
        lv_led_set_brightness(c->led, (uint8_t)val);

        int pct = (val * 100) / 255;
        lv_label_set_text_fmt(c->label, "Brightness: %d%%", pct);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Virtual LED */
    ctx.led = lv_led_create(parent);
    lv_obj_set_size(ctx.led, 80, 80);
    lv_led_set_color(ctx.led, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_set_brightness(ctx.led, 128);
    lv_obj_align(ctx.led, LV_ALIGN_CENTER, 0, -60);

    /* Brightness label */
    ctx.label = lv_label_create(parent);
    lv_label_set_text(ctx.label, "Brightness: 50%");
    lv_obj_set_style_text_font(ctx.label, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx.label, LV_ALIGN_CENTER, 0, 10);

    /* Slider 0-255 */
    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, 300);
    lv_slider_set_range(slider, 0, 255);
    lv_slider_set_value(slider, 128, LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 60);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, &ctx);
}
