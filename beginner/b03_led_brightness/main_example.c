/**
 * @file    main_example.c
 * @brief   LED Brightness Control — Slider controls virtual LED brightness
 *
 * A slider (0-255) adjusts the brightness of an LVGL virtual LED widget.
 * A label displays the current brightness as a percentage.
 *
 * Functions:
 *   create_led_widget()      — Build the LED with color and initial brightness
 *   update_brightness()      — Apply brightness value and update label
 *   slider_event_cb()        — Event callback: handle slider value change
 *   example_main()           — Entry point: compose LED + slider UI
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *led;
    lv_obj_t *label;
} led_ctx_t;

static led_ctx_t ctx;

/* ── Create and configure the LED widget ─────────────────────────── */
static lv_obj_t *create_led_widget(lv_obj_t *parent, lv_palette_t color,
                                    int size, uint8_t initial_brightness)
{
    lv_obj_t *led = lv_led_create(parent);
    lv_obj_set_size(led, size, size);
    lv_led_set_color(led, lv_palette_main(color));
    lv_led_set_brightness(led, initial_brightness);
    return led;
}

/* ── Apply brightness and update display label ───────────────────── */
static void update_brightness(led_ctx_t *c, int32_t val)
{
    lv_led_set_brightness(c->led, (uint8_t)val);
    int pct = (val * 100) / 255;
    lv_label_set_text_fmt(c->label, "Brightness: %d%%", pct);
}

/* ── Slider event callback ───────────────────────────────────────── */
static void slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *slider = lv_event_get_target(e);
        led_ctx_t *c = (led_ctx_t *)lv_event_get_user_data(e);
        int32_t val = lv_slider_get_value(slider);
        update_brightness(c, val);
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Virtual LED */
    ctx.led = create_led_widget(parent, LV_PALETTE_GREEN, 80, 128);
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
