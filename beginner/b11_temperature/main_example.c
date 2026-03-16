/**
 * @file main_example.c
 * @brief B11 — Temperature: Show DPS368 temperature with a large number.
 *
 * Reads the barometric pressure sensor's temperature output and displays
 * it as a large, color-coded value. Requires BSP_HAS_DPS368 (AI Kit only).
 */

#include "example_common.h"

#if BSP_HAS_DPS368

static lv_obj_t *temp_label;
static lv_obj_t *unit_label;

static void temp_timer_cb(lv_timer_t *t)
{
    (void)t;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_dps368) return;

    /* temperature_x100: e.g. 2534 = 25.34 C */
    float temp_c = snap.dps368.temperature_x100 / 100.0f;

    lv_label_set_text_fmt(temp_label, "%.1f", (double)temp_c);

    /* Color based on temperature range */
    lv_color_t c;
    if (temp_c < 20.0f)       c = UI_COLOR_PRIMARY;   /* cool = cyan  */
    else if (temp_c < 30.0f)  c = UI_COLOR_SUCCESS;   /* normal = green */
    else                      c = UI_COLOR_ERROR;      /* hot = red     */
    lv_obj_set_style_text_color(temp_label, c, 0);
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 10, 0);

    /* ---- title ---- */
    example_label_create(parent, "Temperature",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- large temperature value ---- */
    temp_label = example_label_create(parent, "--.-",
                                      &lv_font_montserrat_28, UI_COLOR_PRIMARY);

    /* ---- unit label ---- */
    unit_label = example_label_create(parent, "degrees C",
                                      &lv_font_montserrat_20,
                                      lv_color_hex(0x888888));

    /* ---- sensor source note ---- */
    example_label_create(parent, "Source: DPS368 barometric sensor",
                         &lv_font_montserrat_16, lv_color_hex(0x555555));

    /* ---- 500 ms update ---- */
    lv_timer_create(temp_timer_cb, 500, NULL);
}

#else /* !BSP_HAS_DPS368 */

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_t *lbl = example_label_create(parent,
        "DPS368 not available on this board",
        &lv_font_montserrat_20, UI_COLOR_WARNING);
    lv_obj_center(lbl);
}

#endif
