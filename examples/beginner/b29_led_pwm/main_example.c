/**
 * @file    main_example.c
 * @brief   LED PWM Brightness — Slider controls PWM duty cycle
 *
 * A slider adjusts PWM duty cycle on P13_7 from 0% to 100%.
 * Virtual LED and label show the current brightness.
 */

#include "example_common.h"
#include "cyhal_pwm.h"

#define LED_PWM_PIN   P13_7
#define PWM_FREQ_HZ   10000

typedef struct {
    lv_obj_t     *led_widget;
    lv_obj_t     *lbl_pct;
    cyhal_pwm_t   pwm;
    bool          pwm_ok;
} pwm_ctx_t;

static pwm_ctx_t ctx;

static void slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *slider = lv_event_get_target(e);
        int32_t val = lv_slider_get_value(slider);

        /* Update hardware PWM */
        if (ctx.pwm_ok) {
            cyhal_pwm_set_duty_cycle(&ctx.pwm, (float)val, PWM_FREQ_HZ);
        }

        /* Update virtual LED brightness (0-255 scale) */
        uint8_t brightness = (uint8_t)((val * 255) / 100);
        lv_led_set_brightness(ctx.led_widget, brightness);

        /* Update label */
        lv_label_set_text_fmt(ctx.lbl_pct, "Duty Cycle: %"PRId32"%%", val);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Initialize PWM */
    cy_rslt_t result = cyhal_pwm_init(&ctx.pwm, LED_PWM_PIN, NULL);
    ctx.pwm_ok = (result == CY_RSLT_SUCCESS);
    if (ctx.pwm_ok) {
        cyhal_pwm_set_duty_cycle(&ctx.pwm, 0.0f, PWM_FREQ_HZ);
        cyhal_pwm_start(&ctx.pwm);
    }

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "PWM LED Brightness");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
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
    ctx.lbl_pct = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_pct, "Duty Cycle: 0%");
    lv_obj_set_style_text_font(ctx.lbl_pct, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx.lbl_pct, LV_ALIGN_CENTER, 0, 70);
}
