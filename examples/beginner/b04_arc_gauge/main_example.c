/**
 * @file main_example.c
 * @brief B04 — Arc Gauge: An arc widget displaying a value from 0 to 100.
 *
 * Demonstrates the LVGL arc widget configured as a read-only gauge with
 * a timer that sweeps the value up and down automatically.
 */

#include "example_common.h"

static lv_obj_t *arc;
static lv_obj_t *val_label;
static int16_t value    = 0;
static int8_t  direction = 1;

static void sweep_timer_cb(lv_timer_t *t)
{
    (void)t;
    value += direction;
    if (value >= 100) direction = -1;
    if (value <= 0)   direction =  1;

    lv_arc_set_value(arc, value);
    lv_label_set_text_fmt(val_label, "%d%%", value);
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
    example_label_create(parent, "Arc Gauge",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- arc widget ---- */
    arc = lv_arc_create(parent);
    lv_obj_set_size(arc, 200, 200);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_arc_set_bg_angles(arc, 135, 45);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);     /* hide knob */
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);   /* read-only  */

    /* arc indicator color */
    lv_obj_set_style_arc_color(arc, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_MAIN);

    /* ---- value label centered inside arc ---- */
    val_label = example_label_create(arc, "0%",
                                     &lv_font_montserrat_28,
                                     UI_COLOR_PRIMARY);
    lv_obj_center(val_label);

    /* ---- 50 ms timer drives the sweep animation ---- */
    lv_timer_create(sweep_timer_cb, 50, NULL);
}
