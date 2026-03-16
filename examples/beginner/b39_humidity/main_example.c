/**
 * @file    main_example.c
 * @brief   Humidity Gauge — SHT40 humidity arc display
 *
 * Reads SHT40 humidity every 1s and displays in an arc gauge.
 * AI Kit only (BSP_HAS_SHT40).
 */

#include "example_common.h"

#if BSP_HAS_SHT40
#include "sensor_sht40.h"

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_comfort;
    bool      sensor_ok;
} humidity_ctx_t;

static humidity_ctx_t ctx;

static const char *comfort_level(float rh)
{
    if (rh < 20.0f) return "Very Dry";
    if (rh < 30.0f) return "Dry";
    if (rh < 60.0f) return "Comfortable";
    if (rh < 80.0f) return "Humid";
    return "Very Humid";
}

static lv_color_t comfort_color(float rh)
{
    if (rh < 30.0f) return lv_palette_main(LV_PALETTE_BLUE);
    if (rh < 60.0f) return lv_palette_main(LV_PALETTE_GREEN);
    return lv_palette_main(LV_PALETTE_RED);
}

static void humidity_timer_cb(lv_timer_t *timer)
{
    humidity_ctx_t *c = (humidity_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->sensor_ok) return;

    float temperature, humidity;
    if (sht40_read(&temperature, &humidity) == 0) {
        lv_label_set_text_fmt(c->lbl_value, "%.1f%%", (double)humidity);
        lv_arc_set_value(c->arc, (int32_t)humidity);

        lv_color_t color = comfort_color(humidity);
        lv_obj_set_style_arc_color(c->arc, color, LV_PART_INDICATOR);
        lv_obj_set_style_text_color(c->lbl_value, color, 0);

        lv_label_set_text_fmt(c->lbl_comfort, "Comfort: %s", comfort_level(humidity));
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.sensor_ok = (sht40_init() == 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Humidity (SHT40)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    if (!ctx.sensor_ok) {
        lv_obj_t *err = lv_label_create(parent);
        lv_label_set_text(err, "SHT40 sensor not available");
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
    lv_obj_set_style_arc_color(ctx.arc, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_MAIN);
    lv_obj_align(ctx.arc, LV_ALIGN_CENTER, 0, -5);

    /* Center value */
    ctx.lbl_value = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_value, "--%");
    lv_obj_set_style_text_font(ctx.lbl_value, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(ctx.lbl_value, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(ctx.lbl_value, LV_ALIGN_CENTER, 0, -5);

    /* Comfort level */
    ctx.lbl_comfort = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_comfort, "Comfort: --");
    lv_obj_set_style_text_font(ctx.lbl_comfort, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx.lbl_comfort, LV_ALIGN_BOTTOM_MID, 0, -10);

    /* Timer: 1000ms */
    lv_timer_create(humidity_timer_cb, 1000, &ctx);
}

#else

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "SHT40 not available on this board.\nThis example requires AI Kit.");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_center(lbl);
}

#endif
