/**
 * @file    main_example.c
 * @brief   LED Sequence — Knight Rider bounce with timer state machine
 *
 * Three LEDs animate in a bouncing sequence: R-G-B-G-R.
 * Speed buttons control the animation rate.
 */

#include "example_common.h"
#include "cyhal_gpio.h"

#define NUM_SEQ_LEDS 3

typedef struct {
    lv_obj_t     *leds[NUM_SEQ_LEDS];
    cyhal_gpio_t  pins[NUM_SEQ_LEDS];
    bool          gpio_ok;
    int           current;
    int           direction;  /* +1 or -1 */
    lv_timer_t   *timer;
    lv_obj_t     *lbl_speed;
    uint32_t      period_ms;
} seq_ctx_t;

static seq_ctx_t ctx;

static const lv_palette_t led_colors[] = {
    LV_PALETTE_RED, LV_PALETTE_GREEN, LV_PALETTE_BLUE
};

static void seq_timer_cb(lv_timer_t *timer)
{
    seq_ctx_t *c = (seq_ctx_t *)lv_timer_get_user_data(timer);

    /* Turn off current LED */
    lv_led_off(c->leds[c->current]);
    if (c->gpio_ok) cyhal_gpio_write(c->pins[c->current], false);

    /* Advance */
    c->current += c->direction;
    if (c->current >= NUM_SEQ_LEDS) {
        c->current = NUM_SEQ_LEDS - 2;
        c->direction = -1;
    } else if (c->current < 0) {
        c->current = 1;
        c->direction = 1;
    }

    /* Turn on new LED */
    lv_led_on(c->leds[c->current]);
    if (c->gpio_ok) cyhal_gpio_write(c->pins[c->current], true);
}

static void faster_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (ctx.period_ms > 50) {
        ctx.period_ms -= 50;
        lv_timer_set_period(ctx.timer, ctx.period_ms);
        lv_label_set_text_fmt(ctx.lbl_speed, "Speed: %"PRIu32"ms", ctx.period_ms);
    }
}

static void slower_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (ctx.period_ms < 500) {
        ctx.period_ms += 50;
        lv_timer_set_period(ctx.timer, ctx.period_ms);
        lv_label_set_text_fmt(ctx.lbl_speed, "Speed: %"PRIu32"ms", ctx.period_ms);
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.current = 0;
    ctx.direction = 1;
    ctx.period_ms = 200;
    ctx.pins[0] = P13_4;
    ctx.pins[1] = P13_5;
    ctx.pins[2] = P13_6;

    /* Init GPIOs */
    ctx.gpio_ok = true;
    for (int i = 0; i < NUM_SEQ_LEDS; i++) {
        if (cyhal_gpio_init(ctx.pins[i], CYHAL_GPIO_DIR_OUTPUT,
                            CYHAL_GPIO_DRIVE_STRONG, false) != CY_RSLT_SUCCESS) {
            ctx.gpio_ok = false;
        }
    }

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "LED Sequence");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* LED row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 500, 120);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);

    for (int i = 0; i < NUM_SEQ_LEDS; i++) {
        ctx.leds[i] = lv_led_create(row);
        lv_obj_set_size(ctx.leds[i], 80, 80);
        lv_led_set_color(ctx.leds[i], lv_palette_main(led_colors[i]));
        lv_led_off(ctx.leds[i]);
    }
    lv_led_on(ctx.leds[0]);  /* Start with first LED on */

    /* Speed label */
    ctx.lbl_speed = lv_label_create(parent);
    lv_label_set_text_fmt(ctx.lbl_speed, "Speed: %"PRIu32"ms", ctx.period_ms);
    lv_obj_set_style_text_font(ctx.lbl_speed, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx.lbl_speed, LV_ALIGN_CENTER, 0, 40);

    /* Speed controls */
    lv_obj_t *btn_fast = lv_btn_create(parent);
    lv_obj_set_size(btn_fast, 120, 45);
    lv_obj_align(btn_fast, LV_ALIGN_CENTER, -80, 80);
    lv_obj_add_event_cb(btn_fast, faster_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lf = lv_label_create(btn_fast);
    lv_label_set_text(lf, LV_SYMBOL_PLAY " Faster");
    lv_obj_center(lf);

    lv_obj_t *btn_slow = lv_btn_create(parent);
    lv_obj_set_size(btn_slow, 120, 45);
    lv_obj_align(btn_slow, LV_ALIGN_CENTER, 80, 80);
    lv_obj_add_event_cb(btn_slow, slower_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ls = lv_label_create(btn_slow);
    lv_label_set_text(ls, LV_SYMBOL_PAUSE " Slower");
    lv_obj_center(ls);

    /* Start timer */
    ctx.timer = lv_timer_create(seq_timer_cb, ctx.period_ms, &ctx);
}
