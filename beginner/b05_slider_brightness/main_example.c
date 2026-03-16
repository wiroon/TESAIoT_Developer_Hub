/**
 * @file main_example.c
 * @brief B05 — Slider Brightness: A slider that changes background brightness.
 *
 * Demonstrates the LVGL slider widget with a value-changed event that
 * adjusts the parent container's background opacity in real time.
 */

#include "example_common.h"

static lv_obj_t *bg_panel;
static lv_obj_t *val_label;

static void slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);

    /* Map slider 0-100 to opacity 50-255 */
    lv_opa_t opa = (lv_opa_t)(LV_OPA_50 + (val * (LV_OPA_COVER - LV_OPA_50)) / 100);
    lv_obj_set_style_bg_opa(bg_panel, opa, 0);

    lv_label_set_text_fmt(val_label, "Brightness: %"PRId32"%%", val);
}

void example_main(lv_obj_t *parent)
{
    /* ---- background panel whose brightness we control ---- */
    bg_panel = parent;
    lv_obj_set_style_bg_color(parent, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_50, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 30, 0);

    /* ---- title ---- */
    example_label_create(parent, "Slider Brightness",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- value label ---- */
    val_label = example_label_create(parent, "Brightness: 0%",
                                     &lv_font_montserrat_20, UI_COLOR_TEXT);

    /* ---- slider ---- */
    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, 400);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);

    /* Style the slider track and indicator */
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);

    lv_obj_add_event_cb(slider, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
