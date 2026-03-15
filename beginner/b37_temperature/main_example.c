/**
 * @file    main_example.c
 * @brief   Temperature Gauge — DPS368 temperature in arc display
 *
 * Reads DPS368 temperature every 500ms and displays in an arc gauge.
 * AI Kit only (BSP_HAS_DPS368).
 */

#include "example_common.h"

#if BSP_HAS_DPS368
#include "sensor_dps368.h"

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_min_max;
    float     min_temp;
    float     max_temp;
    bool      first_reading;
    bool      sensor_ok;
} temp_ctx_t;

static temp_ctx_t ctx;

static void temp_timer_cb(lv_timer_t *timer)
{
    temp_ctx_t *c = (temp_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->sensor_ok) return;

    float temperature, pressure;
    if (dps368_read_both(&temperature, &pressure) == 0) {
        lv_label_set_text_fmt(c->lbl_value, "%.1f \xC2\xB0""C", (double)temperature);

        int32_t arc_val = (int32_t)(temperature * 2.0f);  /* 0-100 for 0-50C */
        lv_arc_set_value(c->arc, LV_CLAMP(0, arc_val, 100));

        /* Track min/max */
        if (c->first_reading) {
            c->min_temp = temperature;
            c->max_temp = temperature;
            c->first_reading = false;
        } else {
            if (temperature < c->min_temp) c->min_temp = temperature;
            if (temperature > c->max_temp) c->max_temp = temperature;
        }
        lv_label_set_text_fmt(c->lbl_min_max, "Min: %.1f  Max: %.1f",
                              (double)c->min_temp, (double)c->max_temp);
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.first_reading = true;
    ctx.sensor_ok = (dps368_init() == 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_WARNING " Temperature (DPS368)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    if (!ctx.sensor_ok) {
        lv_obj_t *err = lv_label_create(parent);
        lv_label_set_text(err, "DPS368 sensor not available");
        lv_obj_set_style_text_color(err, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_align(err, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    /* Arc gauge */
    ctx.arc = lv_arc_create(parent);
    lv_obj_set_size(ctx.arc, 220, 220);
    lv_arc_set_rotation(ctx.arc, 135);
    lv_arc_set_bg_angles(ctx.arc, 0, 270);
    lv_arc_set_range(ctx.arc, 0, 100);
    lv_arc_set_value(ctx.arc, 0);
    lv_obj_remove_style(ctx.arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ctx.arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ctx.arc, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_MAIN);
    lv_obj_align(ctx.arc, LV_ALIGN_CENTER, 0, -5);

    /* Center value */
    ctx.lbl_value = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_value, "-- \xC2\xB0""C");
    lv_obj_set_style_text_font(ctx.lbl_value, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(ctx.lbl_value, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_align(ctx.lbl_value, LV_ALIGN_CENTER, 0, -5);

    /* Min/Max */
    ctx.lbl_min_max = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_min_max, "Min: --  Max: --");
    lv_obj_set_style_text_font(ctx.lbl_min_max, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx.lbl_min_max, LV_ALIGN_BOTTOM_MID, 0, -10);

    /* Timer: 500ms */
    lv_timer_create(temp_timer_cb, 500, &ctx);
}

#else /* !BSP_HAS_DPS368 */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "DPS368 not available on this board.\nThis example requires AI Kit.");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_center(lbl);
}

#endif
