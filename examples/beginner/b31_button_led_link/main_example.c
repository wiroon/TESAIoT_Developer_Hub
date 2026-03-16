/**
 * @file    main_example.c
 * @brief   Button-LED Link — HW button drives HW LED + UI feedback
 *
 * SW1 button state is polled and written to LED GPIO.
 * Virtual LED and label track the real-time state.
 */

#include "example_common.h"
#include "cyhal_gpio.h"

#define BTN_PIN  P5_2
#define LED_PIN  P13_7

typedef struct {
    lv_obj_t *led_widget;
    lv_obj_t *lbl_state;
    bool      prev_state;
    bool      gpio_ok;
} link_ctx_t;

static link_ctx_t ctx;

static void link_timer_cb(lv_timer_t *timer)
{
    link_ctx_t *c = (link_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->gpio_ok) return;

    bool pressed = !cyhal_gpio_read(BTN_PIN);  /* Active low */

    if (pressed != c->prev_state) {
        /* Update hardware LED */
        cyhal_gpio_write(LED_PIN, pressed);

        /* Update virtual LED */
        if (pressed) {
            lv_led_on(c->led_widget);
            lv_label_set_text(c->lbl_state, LV_SYMBOL_OK " Button PRESSED — LED ON");
            lv_obj_set_style_text_color(c->lbl_state, lv_palette_main(LV_PALETTE_GREEN), 0);
        } else {
            lv_led_off(c->led_widget);
            lv_label_set_text(c->lbl_state, "Button Released — LED OFF");
            lv_obj_set_style_text_color(c->lbl_state, lv_palette_main(LV_PALETTE_GREY), 0);
        }
        c->prev_state = pressed;
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.prev_state = false;

    /* Initialize GPIOs */
    cy_rslt_t r1 = cyhal_gpio_init(BTN_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    cy_rslt_t r2 = cyhal_gpio_init(LED_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false);
    ctx.gpio_ok = (r1 == CY_RSLT_SUCCESS && r2 == CY_RSLT_SUCCESS);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Button-LED Link");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Instruction */
    lv_obj_t *info = lv_label_create(parent);
    lv_label_set_text(info, "Press SW1 button on the board");
    lv_obj_set_style_text_font(info, &lv_font_montserrat_14, 0);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 40);

    /* Virtual LED */
    ctx.led_widget = lv_led_create(parent);
    lv_obj_set_size(ctx.led_widget, 100, 100);
    lv_led_set_color(ctx.led_widget, lv_palette_main(LV_PALETTE_YELLOW));
    lv_led_off(ctx.led_widget);
    lv_obj_align(ctx.led_widget, LV_ALIGN_CENTER, 0, -20);

    /* State label */
    ctx.lbl_state = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_state, "Button Released — LED OFF");
    lv_obj_set_style_text_font(ctx.lbl_state, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ctx.lbl_state, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(ctx.lbl_state, LV_ALIGN_CENTER, 0, 60);

    /* Poll timer: 50ms */
    lv_timer_create(link_timer_cb, 50, &ctx);
}
