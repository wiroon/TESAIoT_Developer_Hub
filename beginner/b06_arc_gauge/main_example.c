/**
 * @file    main_example.c
 * @brief   Arc Gauge — Animated arc 0-100 with center value label
 *
 * An arc gauge auto-increments from 0 to 100 via a timer callback.
 * A center label shows the current value as a percentage.
 *
 * Functions:
 *   create_arc_gauge()       — Build an arc widget with range and styling
 *   update_arc_display()     — Set arc value and update center label
 *   arc_timer_cb()           — Timer callback: auto-increment arc value
 *   example_main()           — Entry point: compose arc gauge UI
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *label;
    int32_t   value;
} arc_ctx_t;

static arc_ctx_t ctx;

/* ── Create and style the arc gauge widget ───────────────────────── */
static lv_obj_t *create_arc_gauge(lv_obj_t *parent, int size,
                                   int rotation, int bg_angle)
{
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_arc_set_rotation(arc, rotation);
    lv_arc_set_bg_angles(arc, 0, bg_angle);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    return arc;
}

/* ── Update arc value and center label ───────────────────────────── */
static void update_arc_display(arc_ctx_t *c, int32_t val)
{
    c->value = val;
    lv_arc_set_value(c->arc, val);
    lv_label_set_text_fmt(c->label, "%"PRId32"%%", val);
}

/* ── Timer callback — auto-increment ─────────────────────────────── */
static void arc_timer_cb(lv_timer_t *timer)
{
    arc_ctx_t *c = (arc_ctx_t *)lv_timer_get_user_data(timer);
    int32_t next = c->value + 1;
    if (next > 100) next = 0;
    update_arc_display(c, next);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Arc gauge */
    ctx.arc = create_arc_gauge(parent, 200, 135, 270);
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
