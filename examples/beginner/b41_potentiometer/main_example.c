/**
 * @file    main_example.c
 * @brief   Potentiometer Gauge — Analog value in arc display
 *
 * Reads potentiometer every 100ms and displays in an arc gauge.
 * Eva Kit only (BSP_HAS_POTENTIOMETER).
 */

#include "example_common.h"

#if BSP_HAS_POTENTIOMETER
#include "sensor_potentiometer.h"

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_pct;
    lv_obj_t *bar;
    bool      sensor_ok;
} pot_ctx_t;

static pot_ctx_t ctx;

static void pot_timer_cb(lv_timer_t *timer)
{
    pot_ctx_t *c = (pot_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->sensor_ok) return;

    uint16_t raw_value = 0;
    if (potentiometer_read(&raw_value) == 0) {
        int32_t pct = (raw_value * 100) / 4095;

        lv_arc_set_value(c->arc, pct);
        lv_bar_set_value(c->bar, pct, LV_ANIM_ON);
        lv_label_set_text_fmt(c->lbl_value, "%u", raw_value);
        lv_label_set_text_fmt(c->lbl_pct, "%"PRId32"%%", pct);
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.sensor_ok = (potentiometer_init() == 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Potentiometer (Eva Kit)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    if (!ctx.sensor_ok) {
        lv_obj_t *err = lv_label_create(parent);
        lv_label_set_text(err, "Potentiometer init failed");
        lv_obj_set_style_text_color(err, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_align(err, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    /* Arc gauge */
    ctx.arc = lv_arc_create(parent);
    lv_obj_set_size(ctx.arc, 200, 200);
    lv_arc_set_rotation(ctx.arc, 135);
    lv_arc_set_bg_angles(ctx.arc, 0, 270);
    lv_arc_set_range(ctx.arc, 0, 100);
    lv_arc_set_value(ctx.arc, 0);
    lv_obj_remove_style(ctx.arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ctx.arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ctx.arc, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_MAIN);
    lv_obj_align(ctx.arc, LV_ALIGN_CENTER, -130, 0);

    /* Percentage inside arc */
    ctx.lbl_pct = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_pct, "0%");
    lv_obj_set_style_text_font(ctx.lbl_pct, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(ctx.lbl_pct, lv_palette_main(LV_PALETTE_PURPLE), 0);
    lv_obj_align(ctx.lbl_pct, LV_ALIGN_CENTER, -130, 0);

    /* Raw value */
    lv_obj_t *lbl_raw_title = lv_label_create(parent);
    lv_label_set_text(lbl_raw_title, "Raw ADC Value:");
    lv_obj_set_style_text_font(lbl_raw_title, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_raw_title, LV_ALIGN_CENTER, 130, -40);

    ctx.lbl_value = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_value, "0");
    lv_obj_set_style_text_font(ctx.lbl_value, &lv_font_montserrat_24, 0);
    lv_obj_align(ctx.lbl_value, LV_ALIGN_CENTER, 130, -10);

    /* Bar indicator */
    ctx.bar = lv_bar_create(parent);
    lv_obj_set_size(ctx.bar, 250, 20);
    lv_bar_set_range(ctx.bar, 0, 100);
    lv_bar_set_value(ctx.bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx.bar, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_INDICATOR);
    lv_obj_align(ctx.bar, LV_ALIGN_CENTER, 130, 30);

    /* Timer: 100ms */
    lv_timer_create(pot_timer_cb, 100, &ctx);
}

#else

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "Potentiometer not available.\nThis example requires Eva Kit.");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_center(lbl);
}

#endif
