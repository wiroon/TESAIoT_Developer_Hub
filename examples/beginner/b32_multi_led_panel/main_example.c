/**
 * @file    main_example.c
 * @brief   Multi LED Panel — Grid of 3 LED indicators with switches
 *
 * Three virtual LEDs (Red, Green, Blue) with switch controls.
 * All ON / All OFF buttons for batch control.
 */

#include "example_common.h"

#define NUM_LEDS 3

typedef struct {
    const char       *name;
    lv_palette_t      color;
    lv_obj_t         *led_widget;
    lv_obj_t         *sw_widget;
} led_def_t;

static led_def_t leds[NUM_LEDS] = {
    { "Red",   LV_PALETTE_RED,   NULL, NULL },
    { "Green", LV_PALETTE_GREEN, NULL, NULL },
    { "Blue",  LV_PALETTE_BLUE,  NULL, NULL },
};

static void switch_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    led_def_t *led = (led_def_t *)lv_event_get_user_data(e);
    lv_obj_t *sw = lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (on) {
        lv_led_on(led->led_widget);
    } else {
        lv_led_off(led->led_widget);
    }
}

static void all_on_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    for (int i = 0; i < NUM_LEDS; i++) {
        lv_led_on(leds[i].led_widget);
        lv_obj_add_state(leds[i].sw_widget, LV_STATE_CHECKED);
    }
}

static void all_off_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    for (int i = 0; i < NUM_LEDS; i++) {
        lv_led_off(leds[i].led_widget);
        lv_obj_remove_state(leds[i].sw_widget, LV_STATE_CHECKED);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent, "LED Control Panel",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* LED control rows */
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, 500, 220);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_style_pad_all(list, 10, 0);

    for (int i = 0; i < NUM_LEDS; i++) {
        lv_obj_t *row = lv_obj_create(list);
        lv_obj_set_size(row, LV_PCT(100), 50);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(row, 4, 0);
        lv_obj_set_style_border_width(row, 0, 0);

        /* Name */
        lv_obj_t *name = lv_label_create(row);
        lv_label_set_text(name, leds[i].name);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_set_width(name, 100);

        /* Virtual LED */
        leds[i].led_widget = lv_led_create(row);
        lv_obj_set_size(leds[i].led_widget, 30, 30);
        lv_led_set_color(leds[i].led_widget, lv_palette_main(leds[i].color));
        lv_led_off(leds[i].led_widget);

        /* Switch */
        leds[i].sw_widget = lv_switch_create(row);
        lv_obj_set_size(leds[i].sw_widget, 60, 30);
        lv_obj_add_event_cb(leds[i].sw_widget, switch_event_cb,
                            LV_EVENT_VALUE_CHANGED, &leds[i]);
    }

    /* All ON / All OFF buttons */
    lv_obj_t *btn_on = lv_btn_create(parent);
    lv_obj_set_size(btn_on, 140, 45);
    lv_obj_align(btn_on, LV_ALIGN_BOTTOM_LEFT, 80, -10);
    lv_obj_set_style_bg_color(btn_on, UI_COLOR_SUCCESS, 0);
    lv_obj_add_event_cb(btn_on, all_on_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lon = lv_label_create(btn_on);
    lv_label_set_text(lon, "All ON");
    lv_obj_center(lon);

    lv_obj_t *btn_off = lv_btn_create(parent);
    lv_obj_set_size(btn_off, 140, 45);
    lv_obj_align(btn_off, LV_ALIGN_BOTTOM_RIGHT, -80, -10);
    lv_obj_set_style_bg_color(btn_off, UI_COLOR_ERROR, 0);
    lv_obj_add_event_cb(btn_off, all_off_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *loff = lv_label_create(btn_off);
    lv_label_set_text(loff, "All OFF");
    lv_obj_center(loff);
}
