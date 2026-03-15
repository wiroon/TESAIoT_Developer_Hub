/**
 * @file main_example.c
 * @brief B08 — Progress Bar: Animated progress bar from 0 to 100%.
 *
 * Demonstrates the LVGL bar widget with a timer that advances the
 * progress value and changes the bar color at completion.
 */

#include "example_common.h"

static lv_obj_t *bar;
static lv_obj_t *pct_label;
static int32_t progress = 0;

static void progress_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (progress < 100) {
        progress++;
    } else {
        progress = 0;   /* loop back to 0 */
    }

    lv_bar_set_value(bar, progress, LV_ANIM_ON);
    lv_label_set_text_fmt(pct_label, "%"PRId32"%%", progress);

    /* Change color based on progress */
    lv_color_t c;
    if (progress < 33)       c = UI_COLOR_ERROR;
    else if (progress < 66)  c = UI_COLOR_WARNING;
    else                     c = UI_COLOR_SUCCESS;
    lv_obj_set_style_bg_color(bar, c, LV_PART_INDICATOR);
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
    example_label_create(parent, "Progress Bar",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- percentage label ---- */
    pct_label = example_label_create(parent, "0%",
                                     &lv_font_montserrat_28, UI_COLOR_PRIMARY);

    /* ---- bar widget ---- */
    bar = lv_bar_create(parent);
    lv_obj_set_size(bar, 500, 30);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, UI_COLOR_ERROR, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 8, 0);
    lv_obj_set_style_radius(bar, 8, LV_PART_INDICATOR);

    /* ---- 80 ms timer (fills in ~8 seconds) ---- */
    lv_timer_create(progress_timer_cb, 80, NULL);
}
