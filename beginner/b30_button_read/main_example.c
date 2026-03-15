/**
 * @file    main_example.c
 * @brief   Hardware Button Read — Poll SW1/SW2 with timer
 *
 * Polls hardware buttons on P5_2 and P5_3 every 50ms.
 * Displays PRESSED/Released status with press counters.
 */

#include "example_common.h"
#include "cyhal_gpio.h"

#define BTN_SW1_PIN  P5_2
#define BTN_SW2_PIN  P5_3

typedef struct {
    lv_obj_t *lbl_sw1;
    lv_obj_t *lbl_sw2;
    lv_obj_t *lbl_count1;
    lv_obj_t *lbl_count2;
    bool      prev_sw1;
    bool      prev_sw2;
    int32_t   count_sw1;
    int32_t   count_sw2;
    bool      gpio_ok;
} btn_ctx_t;

static btn_ctx_t ctx;

static void poll_timer_cb(lv_timer_t *timer)
{
    btn_ctx_t *c = (btn_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->gpio_ok) return;

    bool sw1 = !cyhal_gpio_read(BTN_SW1_PIN);  /* Active low */
    bool sw2 = !cyhal_gpio_read(BTN_SW2_PIN);

    /* Detect press edges for counting */
    if (sw1 && !c->prev_sw1) {
        c->count_sw1++;
        lv_label_set_text_fmt(c->lbl_count1, "Presses: %"PRId32, c->count_sw1);
    }
    if (sw2 && !c->prev_sw2) {
        c->count_sw2++;
        lv_label_set_text_fmt(c->lbl_count2, "Presses: %"PRId32, c->count_sw2);
    }

    /* Update state labels */
    if (sw1 != c->prev_sw1) {
        lv_label_set_text(c->lbl_sw1, sw1 ? "PRESSED" : "Released");
        lv_obj_set_style_text_color(c->lbl_sw1,
            sw1 ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_GREY), 0);
    }
    if (sw2 != c->prev_sw2) {
        lv_label_set_text(c->lbl_sw2, sw2 ? "PRESSED" : "Released");
        lv_obj_set_style_text_color(c->lbl_sw2,
            sw2 ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_GREY), 0);
    }

    c->prev_sw1 = sw1;
    c->prev_sw2 = sw2;
}

void example_main(lv_obj_t *parent)
{
    ctx.prev_sw1 = false;
    ctx.prev_sw2 = false;
    ctx.count_sw1 = 0;
    ctx.count_sw2 = 0;

    /* Initialize GPIO inputs */
    cy_rslt_t r1 = cyhal_gpio_init(BTN_SW1_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    cy_rslt_t r2 = cyhal_gpio_init(BTN_SW2_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    ctx.gpio_ok = (r1 == CY_RSLT_SUCCESS && r2 == CY_RSLT_SUCCESS);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Hardware Button Input");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* SW1 panel */
    lv_obj_t *lbl_sw1_name = lv_label_create(parent);
    lv_label_set_text(lbl_sw1_name, "SW1 (P5_2):");
    lv_obj_set_style_text_font(lbl_sw1_name, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_sw1_name, LV_ALIGN_CENTER, -100, -40);

    ctx.lbl_sw1 = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_sw1, "Released");
    lv_obj_set_style_text_font(ctx.lbl_sw1, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ctx.lbl_sw1, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(ctx.lbl_sw1, LV_ALIGN_CENTER, -100, -10);

    ctx.lbl_count1 = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_count1, "Presses: 0");
    lv_obj_set_style_text_font(ctx.lbl_count1, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx.lbl_count1, LV_ALIGN_CENTER, -100, 20);

    /* SW2 panel */
    lv_obj_t *lbl_sw2_name = lv_label_create(parent);
    lv_label_set_text(lbl_sw2_name, "SW2 (P5_3):");
    lv_obj_set_style_text_font(lbl_sw2_name, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_sw2_name, LV_ALIGN_CENTER, 100, -40);

    ctx.lbl_sw2 = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_sw2, "Released");
    lv_obj_set_style_text_font(ctx.lbl_sw2, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ctx.lbl_sw2, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(ctx.lbl_sw2, LV_ALIGN_CENTER, 100, -10);

    ctx.lbl_count2 = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_count2, "Presses: 0");
    lv_obj_set_style_text_font(ctx.lbl_count2, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx.lbl_count2, LV_ALIGN_CENTER, 100, 20);

    /* Poll timer: 50ms */
    lv_timer_create(poll_timer_cb, 50, &ctx);
}
