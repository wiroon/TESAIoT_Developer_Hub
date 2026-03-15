/**
 * @file    main_example.c
 * @brief   Progress Bar — Auto-incrementing bar with percentage label
 *
 * A horizontal bar fills from 0 to 100, then resets. Timer increments
 * value by 1 every 100ms. A label shows the current percentage.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *bar;
    lv_obj_t *label;
    int32_t   value;
} bar_ctx_t;

static bar_ctx_t ctx;

static void bar_timer_cb(lv_timer_t *timer)
{
    bar_ctx_t *c = (bar_ctx_t *)lv_timer_get_user_data(timer);

    c->value++;
    if (c->value > 100) {
        c->value = 0;
    }

    lv_bar_set_value(c->bar, c->value, LV_ANIM_ON);
    lv_label_set_text_fmt(c->label, "Progress: %"PRId32"%%", c->value);
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Loading...");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* Progress bar */
    ctx.bar = lv_bar_create(parent);
    lv_obj_set_size(ctx.bar, 400, 30);
    lv_bar_set_range(ctx.bar, 0, 100);
    lv_bar_set_value(ctx.bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx.bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_align(ctx.bar, LV_ALIGN_CENTER, 0, 0);

    /* Percentage label */
    ctx.label = lv_label_create(parent);
    lv_label_set_text(ctx.label, "Progress: 0%");
    lv_obj_set_style_text_font(ctx.label, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx.label, LV_ALIGN_CENTER, 0, 40);

    ctx.value = 0;

    /* Timer: increment every 100ms */
    lv_timer_create(bar_timer_cb, 100, &ctx);
}
