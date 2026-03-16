/**
 * @file    main_example.c
 * @brief   LED Toggle — Single button toggles hardware and virtual LED
 *
 * Each click toggles the LED state. Virtual LED and label stay in sync.
 */

#include "example_common.h"
#include "cyhal_gpio.h"

#define LED_PIN P13_7

typedef struct {
    lv_obj_t *led_widget;
    lv_obj_t *lbl_state;
    lv_obj_t *btn_label;
    bool      is_on;
    bool      gpio_ok;
} toggle_ctx_t;

static toggle_ctx_t ctx;

static void toggle_btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ctx.is_on = !ctx.is_on;

        if (ctx.gpio_ok) {
            cyhal_gpio_toggle(LED_PIN);
        }

        if (ctx.is_on) {
            lv_led_on(ctx.led_widget);
            lv_label_set_text(ctx.lbl_state, "State: ON");
            lv_label_set_text(ctx.btn_label, LV_SYMBOL_POWER " Turn OFF");
            lv_obj_set_style_text_color(ctx.lbl_state, lv_palette_main(LV_PALETTE_GREEN), 0);
        } else {
            lv_led_off(ctx.led_widget);
            lv_label_set_text(ctx.lbl_state, "State: OFF");
            lv_label_set_text(ctx.btn_label, LV_SYMBOL_POWER " Turn ON");
            lv_obj_set_style_text_color(ctx.lbl_state, lv_palette_main(LV_PALETTE_RED), 0);
        }
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.is_on = false;
    ctx.gpio_ok = (cyhal_gpio_init(LED_PIN, CYHAL_GPIO_DIR_OUTPUT,
                                    CYHAL_GPIO_DRIVE_STRONG, false) == CY_RSLT_SUCCESS);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "LED Toggle");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
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
    ctx.lbl_state = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_state, "State: OFF");
    lv_obj_set_style_text_font(ctx.lbl_state, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ctx.lbl_state, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align(ctx.lbl_state, LV_ALIGN_CENTER, 0, 100);
}
