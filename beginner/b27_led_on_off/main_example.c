/**
 * @file    main_example.c
 * @brief   Hardware LED On/Off — Buttons control GPIO LED
 *
 * ON and OFF buttons control hardware LED on P13_7.
 * A virtual LED mirrors the state on screen.
 */

#include "example_common.h"
#include "cyhal_gpio.h"

#define LED_PIN P13_7

typedef struct {
    lv_obj_t *led_widget;
    lv_obj_t *lbl_state;
    bool      initialized;
} led_ctx_t;

static led_ctx_t ctx;

static void on_btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (ctx.initialized) {
            cyhal_gpio_write(LED_PIN, true);
        }
        lv_led_on(ctx.led_widget);
        lv_label_set_text(ctx.lbl_state, "LED: ON");
        lv_obj_set_style_text_color(ctx.lbl_state, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

static void off_btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (ctx.initialized) {
            cyhal_gpio_write(LED_PIN, false);
        }
        lv_led_off(ctx.led_widget);
        lv_label_set_text(ctx.lbl_state, "LED: OFF");
        lv_obj_set_style_text_color(ctx.lbl_state, lv_palette_main(LV_PALETTE_RED), 0);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Initialize GPIO */
    cy_rslt_t result = cyhal_gpio_init(LED_PIN, CYHAL_GPIO_DIR_OUTPUT,
                                        CYHAL_GPIO_DRIVE_STRONG, false);
    ctx.initialized = (result == CY_RSLT_SUCCESS);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Hardware LED Control");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Virtual LED */
    ctx.led_widget = lv_led_create(parent);
    lv_obj_set_size(ctx.led_widget, 80, 80);
    lv_led_set_color(ctx.led_widget, lv_palette_main(LV_PALETTE_RED));
    lv_led_off(ctx.led_widget);
    lv_obj_align(ctx.led_widget, LV_ALIGN_CENTER, 0, -40);

    /* ON button */
    lv_obj_t *btn_on = lv_btn_create(parent);
    lv_obj_set_size(btn_on, 140, 55);
    lv_obj_align(btn_on, LV_ALIGN_CENTER, -90, 40);
    lv_obj_set_style_bg_color(btn_on, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_on, on_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_on = lv_label_create(btn_on);
    lv_label_set_text(lbl_on, "ON");
    lv_obj_center(lbl_on);

    /* OFF button */
    lv_obj_t *btn_off = lv_btn_create(parent);
    lv_obj_set_size(btn_off, 140, 55);
    lv_obj_align(btn_off, LV_ALIGN_CENTER, 90, 40);
    lv_obj_set_style_bg_color(btn_off, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_off, off_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_off = lv_label_create(btn_off);
    lv_label_set_text(lbl_off, "OFF");
    lv_obj_center(lbl_off);

    /* State label */
    ctx.lbl_state = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_state, "LED: OFF");
    lv_obj_set_style_text_font(ctx.lbl_state, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ctx.lbl_state, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align(ctx.lbl_state, LV_ALIGN_CENTER, 0, 100);
}
