/**
 * @file main_example.c
 * @brief B07 — Switch Toggle: On/Off switch with color indicator.
 *
 * Demonstrates the LVGL switch widget. Toggling the switch changes a
 * status indicator panel between green (ON) and red (OFF).
 */

#include "example_common.h"

static lv_obj_t *indicator;
static lv_obj_t *state_label;

static void switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool on = (lv_obj_get_state(sw) & LV_STATE_CHECKED) != 0;

    lv_obj_set_style_bg_color(indicator, on ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);
    lv_label_set_text(state_label, on ? "ON" : "OFF");
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 25, 0);

    /* ---- title ---- */
    example_label_create(parent, "Switch Toggle",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- color indicator circle ---- */
    indicator = lv_obj_create(parent);
    lv_obj_set_size(indicator, 120, 120);
    lv_obj_set_style_radius(indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(indicator, UI_COLOR_ERROR, 0);
    lv_obj_set_style_bg_opa(indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(indicator, 0, 0);

    /* label inside the indicator */
    state_label = example_label_create(indicator, "OFF",
                                       &lv_font_montserrat_28,
                                       lv_color_white());
    lv_obj_center(state_label);

    /* ---- switch ---- */
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 80, 40);
    lv_obj_set_style_bg_color(sw, UI_COLOR_SUCCESS, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
