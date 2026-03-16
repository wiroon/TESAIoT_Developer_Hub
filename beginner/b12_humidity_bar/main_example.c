/**
 * @file main_example.c
 * @brief B12 — Humidity Bar: SHT40 humidity displayed as a colored bar.
 *
 * Reads humidity from the SHT40 climate sensor and displays it as a
 * horizontal bar with a percentage label. Color shifts from blue (dry)
 * to cyan (comfortable) to green (humid).
 * Requires BSP_HAS_SHT40 (AI Kit only).
 */

#include "example_common.h"

#if BSP_HAS_SHT40

static lv_obj_t *bar;
static lv_obj_t *hum_label;
static lv_obj_t *temp_label;

static void hum_timer_cb(lv_timer_t *t)
{
    (void)t;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_sht40) return;

    /* humidity_x100: e.g. 5523 = 55.23% RH */
    int32_t hum_pct = snap.sht40.humidity_x100 / 100;
    float   hum_f   = snap.sht40.humidity_x100 / 100.0f;
    float   temp_f  = snap.sht40.temperature_x100 / 100.0f;

    lv_bar_set_value(bar, (int32_t)hum_pct, LV_ANIM_ON);
    lv_label_set_text_fmt(hum_label, "%.1f%% RH", (double)hum_f);
    lv_label_set_text_fmt(temp_label, "Temp: %.1f C", (double)temp_f);

    /* Color shifts with humidity level */
    lv_color_t c;
    if (hum_pct < 30)       c = UI_COLOR_WARNING;    /* dry    */
    else if (hum_pct < 60)  c = UI_COLOR_SUCCESS;    /* comfy  */
    else                    c = UI_COLOR_PRIMARY;     /* humid  */
    lv_obj_set_style_bg_color(bar, c, LV_PART_INDICATOR);
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 20, 0);

    /* ---- title ---- */
    example_label_create(parent, "Humidity",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- humidity value ---- */
    hum_label = example_label_create(parent, "--.-%% RH",
                                     &lv_font_montserrat_28, UI_COLOR_PRIMARY);

    /* ---- bar ---- */
    bar = lv_bar_create(parent);
    lv_obj_set_size(bar, 500, 30);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 8, 0);
    lv_obj_set_style_radius(bar, 8, LV_PART_INDICATOR);

    /* ---- temperature side-reading ---- */
    temp_label = example_label_create(parent, "Temp: --.- C",
                                      &lv_font_montserrat_20,
                                      lv_color_hex(0x888888));

    /* ---- 500 ms update ---- */
    lv_timer_create(hum_timer_cb, 500, NULL);
}

#else /* !BSP_HAS_SHT40 */

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_t *lbl = example_label_create(parent,
        "SHT40 not available on this board",
        &lv_font_montserrat_20, UI_COLOR_WARNING);
    lv_obj_center(lbl);
}

#endif
