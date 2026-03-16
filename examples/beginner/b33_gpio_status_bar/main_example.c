/**
 * @file    main_example.c
 * @brief   GPIO Status Bar — Monitor inputs and outputs in status bar
 *
 * A horizontal status bar shows LED and button states.
 * Timer polls inputs; toggle buttons control outputs.
 */

#include "example_common.h"
#include "cyhal_gpio.h"

#define BTN_SW1   P5_2
#define BTN_SW2   P5_3
#define LED1_PIN  P13_7

typedef struct {
    lv_obj_t *dot_sw1;
    lv_obj_t *dot_sw2;
    lv_obj_t *dot_led1;
    lv_obj_t *lbl_sw1;
    lv_obj_t *lbl_sw2;
    bool      led1_on;
    bool      gpio_ok;
} status_ctx_t;

static status_ctx_t ctx;

static void update_dot(lv_obj_t *dot, bool active)
{
    lv_obj_set_style_bg_color(dot,
        active ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_GREY), 0);
}

static void status_timer_cb(lv_timer_t *timer)
{
    status_ctx_t *c = (status_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->gpio_ok) return;

    bool sw1 = !cyhal_gpio_read(BTN_SW1);
    bool sw2 = !cyhal_gpio_read(BTN_SW2);

    update_dot(c->dot_sw1, sw1);
    update_dot(c->dot_sw2, sw2);
    lv_label_set_text(c->lbl_sw1, sw1 ? "SW1:ON" : "SW1:off");
    lv_label_set_text(c->lbl_sw2, sw2 ? "SW2:ON" : "SW2:off");
}

static void led_toggle_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ctx.led1_on = !ctx.led1_on;
    if (ctx.gpio_ok) {
        cyhal_gpio_write(LED1_PIN, ctx.led1_on);
    }
    update_dot(ctx.dot_led1, ctx.led1_on);
}

static lv_obj_t *create_status_item(lv_obj_t *bar, const char *name, lv_obj_t **dot_out, lv_obj_t **lbl_out)
{
    lv_obj_t *item = lv_obj_create(bar);
    lv_obj_set_size(item, 110, 40);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(item, 4, 0);
    lv_obj_set_style_pad_column(item, 6, 0);
    lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(item, 0, 0);

    *dot_out = lv_obj_create(item);
    lv_obj_set_size(*dot_out, 16, 16);
    lv_obj_set_style_radius(*dot_out, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(*dot_out, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_border_width(*dot_out, 0, 0);

    *lbl_out = lv_label_create(item);
    lv_label_set_text(*lbl_out, name);
    lv_obj_set_style_text_font(*lbl_out, &lv_font_montserrat_14, 0);

    return item;
}

void example_main(lv_obj_t *parent)
{
    ctx.led1_on = false;

    /* Init GPIOs */
    cy_rslt_t r1 = cyhal_gpio_init(BTN_SW1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    cy_rslt_t r2 = cyhal_gpio_init(BTN_SW2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    cy_rslt_t r3 = cyhal_gpio_init(LED1_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false);
    ctx.gpio_ok = (r1 == CY_RSLT_SUCCESS && r2 == CY_RSLT_SUCCESS && r3 == CY_RSLT_SUCCESS);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "GPIO Status Monitor");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Status bar */
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 600, 55);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_radius(bar, 8, 0);

    lv_obj_t *lbl_dummy;
    create_status_item(bar, "SW1:off", &ctx.dot_sw1, &ctx.lbl_sw1);
    create_status_item(bar, "SW2:off", &ctx.dot_sw2, &ctx.lbl_sw2);
    create_status_item(bar, "LED1:off", &ctx.dot_led1, &lbl_dummy);

    /* LED toggle button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 200, 55);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(btn, led_toggle_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Toggle LED1");
    lv_obj_center(btn_lbl);

    /* Poll timer: 100ms */
    lv_timer_create(status_timer_cb, 100, &ctx);
}
