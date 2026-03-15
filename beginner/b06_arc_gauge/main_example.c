/**
 * @file    main_example.c
 * @brief   Arc Gauge — Animated arc 0-100 with center value label
 *
 * An arc gauge auto-increments from 0 to 100 via a timer callback.
 * A center label shows the current value as a percentage.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *label;
    int32_t   value;
} arc_ctx_t;

static arc_ctx_t ctx;

static void arc_timer_cb(lv_timer_t *timer)
{
    arc_ctx_t *c = (arc_ctx_t *)lv_timer_get_user_data(timer);

    c->value++;
    if (c->value > 100) {
        c->value = 0;
    }

    lv_arc_set_value(c->arc, c->value);
    lv_label_set_text_fmt(c->label, "%"PRId32"%%", c->value);
}

void example_main(lv_obj_t *parent)
{
    /* Arc gauge */
    ctx.arc = lv_arc_create(parent);
    lv_obj_set_size(ctx.arc, 200, 200);
    lv_arc_set_rotation(ctx.arc, 135);
    lv_arc_set_bg_angles(ctx.arc, 0, 270);
    lv_arc_set_range(ctx.arc, 0, 100);
    lv_arc_set_value(ctx.arc, 0);
    lv_obj_remove_style(ctx.arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ctx.arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(ctx.arc, LV_ALIGN_CENTER, 0, -10);

    /* Center value label */
    ctx.label = lv_label_create(parent);
    lv_label_set_text(ctx.label, "0%");
    lv_obj_set_style_text_font(ctx.label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ctx.label, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(ctx.label, LV_ALIGN_CENTER, 0, -10);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Arc Gauge Demo");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -10);

    ctx.value = 0;

    /* Timer: increment every 50ms */
    lv_timer_create(arc_timer_cb, 50, &ctx);
}
