/**
 * @file    main_example.c
 * @brief   Dual-Axis Chart — Temperature + Pressure from DPS368
 *
 * Primary Y-axis: temperature (C), Secondary Y-axis: pressure (hPa).
 * AI Kit only — guarded with BSP_HAS_DPS368.
 */

#include "example_common.h"

#if BSP_HAS_DPS368

#include "sensor_dps368.h"

static lv_obj_t          *s_chart;
static lv_chart_series_t *s_ser_temp;
static lv_chart_series_t *s_ser_pres;
static lv_obj_t          *s_lbl_temp;
static lv_obj_t          *s_lbl_pres;

/* ── Timer — read DPS368 every 500 ms ────────────────────────────── */
static void dps_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensor_dps368_data_t d;
    if (sensor_dps368_read(&d) != 0) return;

    /* Temperature in 0.1 C units for chart resolution */
    int32_t temp_x10 = (int32_t)(d.temperature * 10.0f);
    /* Pressure in 0.1 hPa units */
    int32_t pres_x10 = (int32_t)(d.pressure * 10.0f);

    lv_chart_set_next_value(s_chart, s_ser_temp, temp_x10);
    lv_chart_set_next_value(s_chart, s_ser_pres, pres_x10);

    lv_label_set_text_fmt(s_lbl_temp, "Temp: %.1f C", d.temperature);
    lv_label_set_text_fmt(s_lbl_pres, "Pres: %.1f hPa", d.pressure);
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I06 — Dual-Axis: Temp + Pressure");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 680, 240);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, -10);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 60);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);
    lv_chart_set_div_line_count(s_chart, 5, 6);

    /* Primary Y (temperature): 15.0 - 45.0 C  → stored as 150-450 */
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 150, 450);

    /* Secondary Y (pressure): 950.0 - 1050.0 hPa → stored as 9500-10500 */
    lv_chart_set_range(s_chart, LV_CHART_AXIS_SECONDARY_Y, 9500, 10500);

    /* Temperature series: orange, primary Y */
    s_ser_temp = lv_chart_add_series(s_chart,
                                     lv_color_hex(0xFF9800),
                                     LV_CHART_AXIS_PRIMARY_Y);

    /* Pressure series: cyan, secondary Y */
    s_ser_pres = lv_chart_add_series(s_chart,
                                     lv_palette_main(LV_PALETTE_CYAN),
                                     LV_CHART_AXIS_SECONDARY_Y);

    /* Value labels */
    s_lbl_temp = lv_label_create(parent);
    lv_label_set_text(s_lbl_temp, "Temp: -- C");
    lv_obj_set_style_text_color(s_lbl_temp, lv_color_hex(0xFF9800), 0);
    lv_obj_set_style_text_font(s_lbl_temp, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_temp, LV_ALIGN_BOTTOM_LEFT, 80, -10);

    s_lbl_pres = lv_label_create(parent);
    lv_label_set_text(s_lbl_pres, "Pres: -- hPa");
    lv_obj_set_style_text_color(s_lbl_pres, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_set_style_text_font(s_lbl_pres, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_pres, LV_ALIGN_BOTTOM_RIGHT, -80, -10);

    /* Init sensor + timer */
    sensor_dps368_init();
    lv_timer_create(dps_timer_cb, 500, NULL);
}

#else /* BSP_HAS_DPS368 == 0 */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "I06 — Dual-Axis Chart\n\n"
                           "This example requires DPS368 (AI Kit only).\n"
                           "Not available on this board.");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_center(lbl);
}

#endif /* BSP_HAS_DPS368 */
