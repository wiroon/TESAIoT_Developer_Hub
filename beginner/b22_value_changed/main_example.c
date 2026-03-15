/**
 * @file    main_example.c
 * @brief   Value Changed Sync — Slider and Arc synchronized
 *
 * Moving the slider updates the arc, and vice versa.
 * A label shows the current shared value.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *slider;
    lv_obj_t *arc;
    lv_obj_t *label;
    bool      updating;  /* Guard against recursive updates */
} sync_ctx_t;

static sync_ctx_t ctx;

static void slider_event_cb(lv_event_t *e)
{
    if (ctx.updating) return;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        ctx.updating = true;
        int32_t val = lv_slider_get_value(ctx.slider);
        lv_arc_set_value(ctx.arc, val);
        lv_label_set_text_fmt(ctx.label, "Value: %"PRId32, val);
        ctx.updating = false;
    }
}

static void arc_event_cb(lv_event_t *e)
{
    if (ctx.updating) return;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        ctx.updating = true;
        int32_t val = lv_arc_get_value(ctx.arc);
        lv_slider_set_value(ctx.slider, val, LV_ANIM_OFF);
        lv_label_set_text_fmt(ctx.label, "Value: %"PRId32, val);
        ctx.updating = false;
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.updating = false;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Slider <-> Arc Sync");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Arc gauge */
    ctx.arc = lv_arc_create(parent);
    lv_obj_set_size(ctx.arc, 180, 180);
    lv_arc_set_rotation(ctx.arc, 135);
    lv_arc_set_bg_angles(ctx.arc, 0, 270);
    lv_arc_set_range(ctx.arc, 0, 100);
    lv_arc_set_value(ctx.arc, 50);
    lv_obj_align(ctx.arc, LV_ALIGN_CENTER, -120, 10);
    lv_obj_add_event_cb(ctx.arc, arc_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Value label inside arc */
    ctx.label = lv_label_create(parent);
    lv_label_set_text(ctx.label, "Value: 50");
    lv_obj_set_style_text_font(ctx.label, &lv_font_montserrat_24, 0);
    lv_obj_align(ctx.label, LV_ALIGN_CENTER, 120, -20);

    /* Slider */
    ctx.slider = lv_slider_create(parent);
    lv_obj_set_width(ctx.slider, 250);
    lv_slider_set_range(ctx.slider, 0, 100);
    lv_slider_set_value(ctx.slider, 50, LV_ANIM_OFF);
    lv_obj_align(ctx.slider, LV_ALIGN_CENTER, 120, 40);
    lv_obj_add_event_cb(ctx.slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
