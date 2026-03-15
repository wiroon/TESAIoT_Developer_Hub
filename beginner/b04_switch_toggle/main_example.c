/**
 * @file    main_example.c
 * @brief   Switch Toggle — Switch controls a virtual LED on/off
 *
 * Toggling the switch turns an LVGL LED widget on or off.
 * A label shows the current state text.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *led;
    lv_obj_t *label;
} toggle_ctx_t;

static toggle_ctx_t ctx;

static void switch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *sw = lv_event_get_target(e);
        toggle_ctx_t *c = (toggle_ctx_t *)lv_event_get_user_data(e);

        bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
        if (is_on) {
            lv_led_on(c->led);
            lv_label_set_text(c->label, "State: ON");
            lv_obj_set_style_text_color(c->label, lv_palette_main(LV_PALETTE_GREEN), 0);
        } else {
            lv_led_off(c->led);
            lv_label_set_text(c->label, "State: OFF");
            lv_obj_set_style_text_color(c->label, lv_palette_main(LV_PALETTE_RED), 0);
        }
    }
}

void example_main(lv_obj_t *parent)
{
    /* Virtual LED */
    ctx.led = lv_led_create(parent);
    lv_obj_set_size(ctx.led, 80, 80);
    lv_led_set_color(ctx.led, lv_palette_main(LV_PALETTE_BLUE));
    lv_led_off(ctx.led);
    lv_obj_align(ctx.led, LV_ALIGN_CENTER, 0, -50);

    /* Switch */
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 80, 40);
    lv_obj_align(sw, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(sw, switch_event_cb, LV_EVENT_VALUE_CHANGED, &ctx);

    /* State label */
    ctx.label = lv_label_create(parent);
    lv_label_set_text(ctx.label, "State: OFF");
    lv_obj_set_style_text_color(ctx.label, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_font(ctx.label, &lv_font_montserrat_20, 0);
    lv_obj_align(ctx.label, LV_ALIGN_CENTER, 0, 70);
}
