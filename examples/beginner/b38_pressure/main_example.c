/**
 * @file    main_example.c
 * @brief   Pressure Bar — DPS368 atmospheric pressure display
 *
 * Reads DPS368 pressure every 500ms. Displayed as a bar widget
 * with numeric value. AI Kit only (BSP_HAS_DPS368).
 */

#include "example_common.h"

#if BSP_HAS_DPS368
#include "sensor_dps368.h"

typedef struct {
    lv_obj_t *bar;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_status;
    bool      sensor_ok;
} pressure_ctx_t;

static pressure_ctx_t ctx;

static const char *pressure_status(float hpa)
{
    if (hpa < 980.0f)  return "Very Low";
    if (hpa < 1000.0f) return "Low";
    if (hpa < 1020.0f) return "Normal";
    if (hpa < 1040.0f) return "High";
    return "Very High";
}

static void pressure_timer_cb(lv_timer_t *timer)
{
    pressure_ctx_t *c = (pressure_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->sensor_ok) return;

    float temperature, pressure;
    if (dps368_read_both(&temperature, &pressure) == 0) {
        lv_label_set_text_fmt(c->lbl_value, "%.2f hPa", (double)pressure);

        /* Map 950-1050 to 0-100 */
        int32_t bar_val = (int32_t)((pressure - 950.0f));
        lv_bar_set_value(c->bar, LV_CLAMP(0, bar_val, 100), LV_ANIM_ON);

        lv_label_set_text_fmt(c->lbl_status, "Status: %s", pressure_status(pressure));
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.sensor_ok = (dps368_init() == 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Barometric Pressure (DPS368)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    if (!ctx.sensor_ok) {
        lv_obj_t *err = lv_label_create(parent);
        lv_label_set_text(err, "DPS368 sensor not available");
        lv_obj_set_style_text_color(err, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_align(err, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    /* Pressure value */
    ctx.lbl_value = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_value, "---- hPa");
    lv_obj_set_style_text_font(ctx.lbl_value, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(ctx.lbl_value, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(ctx.lbl_value, LV_ALIGN_CENTER, 0, -50);

    /* Pressure bar */
    ctx.bar = lv_bar_create(parent);
    lv_obj_set_size(ctx.bar, 500, 30);
    lv_bar_set_range(ctx.bar, 0, 100);
    lv_bar_set_value(ctx.bar, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx.bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_align(ctx.bar, LV_ALIGN_CENTER, 0, 0);

    /* Range labels */
    lv_obj_t *lbl_lo = lv_label_create(parent);
    lv_label_set_text(lbl_lo, "950");
    lv_obj_set_style_text_font(lbl_lo, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_lo, LV_ALIGN_CENTER, -260, 25);

    lv_obj_t *lbl_hi = lv_label_create(parent);
    lv_label_set_text(lbl_hi, "1050");
    lv_obj_set_style_text_font(lbl_hi, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_hi, LV_ALIGN_CENTER, 255, 25);

    /* Status label */
    ctx.lbl_status = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_status, "Status: --");
    lv_obj_set_style_text_font(ctx.lbl_status, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx.lbl_status, LV_ALIGN_CENTER, 0, 60);

    /* Timer: 500ms */
    lv_timer_create(pressure_timer_cb, 500, &ctx);
}

#else

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "DPS368 not available on this board.\nThis example requires AI Kit.");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_center(lbl);
}

#endif
