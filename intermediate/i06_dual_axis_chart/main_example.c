/**
 * @file    main_example.c
 * @brief   Dual-Axis Chart — Temperature + Pressure from DPS368 via IPC
 *
 * Primary Y-axis: temperature (C), Secondary Y-axis: pressure (hPa).
 * AI Kit only — guarded with BSP_HAS_DPS368.
 */

#include "example_common.h"

#if BSP_HAS_DPS368

static lv_obj_t          *s_chart;
static lv_chart_series_t *s_ser_temp;
static lv_chart_series_t *s_ser_pres;
static lv_obj_t          *s_lbl_temp;
static lv_obj_t          *s_lbl_pres;

/* ── Timer — read DPS368 via IPC every 500 ms ───────────────────── */
static void dps_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    if (!snap.has_dps368) return;

    /* temperature_x100: Celsius * 100 → store as 0.1 C for chart */
    int32_t temp_x10 = snap.dps368.temperature_x100 / 10;
    /* pressure_x100: hPa * 100 → store as 0.1 hPa for chart */
    int32_t pres_x10 = snap.dps368.pressure_x100 / 10;

    lv_chart_set_next_value(s_chart, s_ser_temp, temp_x10);
    lv_chart_set_next_value(s_chart, s_ser_pres, pres_x10);

    lv_label_set_text_fmt(s_lbl_temp, "Temp: %d.%d C",
        (int)(snap.dps368.temperature_x100 / 100),
        (int)(abs(snap.dps368.temperature_x100) % 100));
    lv_label_set_text_fmt(s_lbl_pres, "Pres: %d.%02d hPa",
        (int)(snap.dps368.pressure_x100 / 100),
        (int)(snap.dps368.pressure_x100 % 100));
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "I06 \xe2\x80\x94 Dual-Axis: Temp + Pressure",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 680, 240);
    lv_obj_align(s_chart, LV_ALIGN_CENTER, 0, -10);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 60);
    lv_obj_set_style_line_width(s_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_chart, 8, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);
    lv_chart_set_div_line_count(s_chart, 5, 6);

    /* Primary Y (temperature): 15.0 - 45.0 C stored as 150-450 */
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 150, 450);

    /* Secondary Y (pressure): 950.0 - 1050.0 hPa stored as 9500-10500 */
    lv_chart_set_range(s_chart, LV_CHART_AXIS_SECONDARY_Y, 9500, 10500);

    /* Temperature series: orange, primary Y */
    s_ser_temp = lv_chart_add_series(s_chart, UI_COLOR_DPS368,
                                     LV_CHART_AXIS_PRIMARY_Y);

    /* Pressure series: cyan, secondary Y */
    s_ser_pres = lv_chart_add_series(s_chart, UI_COLOR_PRIMARY,
                                     LV_CHART_AXIS_SECONDARY_Y);

    /* Value labels */
    s_lbl_temp = example_label_create(parent, "Temp: -- C",
        &lv_font_montserrat_16, UI_COLOR_DPS368);
    lv_obj_align(s_lbl_temp, LV_ALIGN_BOTTOM_LEFT, 80, -10);

    s_lbl_pres = example_label_create(parent, "Pres: -- hPa",
        &lv_font_montserrat_16, UI_COLOR_PRIMARY);
    lv_obj_align(s_lbl_pres, LV_ALIGN_BOTTOM_RIGHT, -80, -10);

    /* Start timer */
    lv_timer_create(dps_timer_cb, 500, NULL);
}

#else /* BSP_HAS_DPS368 == 0 */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "I06 \xe2\x80\x94 Dual-Axis Chart\n\n"
        "This example requires DPS368 (AI Kit only).\n"
        "Not available on this board.",
        &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
    lv_obj_center(lbl);
}

#endif /* BSP_HAS_DPS368 */
