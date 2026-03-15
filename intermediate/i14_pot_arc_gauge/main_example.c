/**
 * @file    main_example.c
 * @brief   Potentiometer Arc Gauge — 0-100% with color gradient (Eva Kit)
 *
 * Green < 50%, Yellow 50-80%, Red > 80%.  Arc updates every 50 ms.
 */

#include "example_common.h"

#if BSP_HAS_POTENTIOMETER

#include "sensor_potentiometer.h"

static lv_obj_t *s_arc;
static lv_obj_t *s_lbl_pct;
static lv_obj_t *s_lbl_raw;

/* ── Timer — read potentiometer at 20 Hz ─────────────────────────── */
static void pot_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensor_potentiometer_data_t d;
    if (sensor_potentiometer_read(&d) != 0) return;

    int32_t pct = (int32_t)(d.percentage);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    lv_arc_set_value(s_arc, pct);
    lv_label_set_text_fmt(s_lbl_pct, "%d %%", (int)pct);
    lv_label_set_text_fmt(s_lbl_raw, "ADC: %u", (unsigned)d.raw_value);

    /* Color gradient: green → yellow → red */
    lv_color_t color;
    if (pct < 50) {
        color = lv_palette_main(LV_PALETTE_GREEN);
    } else if (pct < 80) {
        color = lv_palette_main(LV_PALETTE_YELLOW);
    } else {
        color = lv_palette_main(LV_PALETTE_RED);
    }
    lv_obj_set_style_arc_color(s_arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(s_lbl_pct, color, 0);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I14 — Potentiometer Gauge (Eva Kit)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Arc gauge */
    s_arc = lv_arc_create(parent);
    lv_obj_set_size(s_arc, 260, 260);
    lv_obj_align(s_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_range(s_arc, 0, 100);
    lv_arc_set_bg_angles(s_arc, 135, 45);
    lv_arc_set_value(s_arc, 0);
    lv_obj_set_style_arc_color(s_arc, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x1a3050), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc, 16, LV_PART_MAIN);
    lv_obj_remove_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Percentage label */
    s_lbl_pct = lv_label_create(parent);
    lv_label_set_text(s_lbl_pct, "0 %");
    lv_obj_set_style_text_font(s_lbl_pct, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_lbl_pct, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(s_lbl_pct, LV_ALIGN_CENTER, 0, -8);

    /* Raw ADC label */
    s_lbl_raw = lv_label_create(parent);
    lv_label_set_text(s_lbl_raw, "ADC: 0");
    lv_obj_set_style_text_font(s_lbl_raw, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_raw, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_raw, LV_ALIGN_CENTER, 0, 20);

    /* Init + timer */
    sensor_potentiometer_init();
    lv_timer_create(pot_timer_cb, 50, NULL);
}

#else /* BSP_HAS_POTENTIOMETER */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "I14 — Potentiometer Arc Gauge\n\n"
                           "Requires Potentiometer (Eva Kit only).\n"
                           "Not available on this board.");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_center(lbl);
}

#endif /* BSP_HAS_POTENTIOMETER */
