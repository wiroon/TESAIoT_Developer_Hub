/**
 * @file main_example.c
 * @brief B14 — LED Toggle: Three buttons to toggle virtual Red, Green, Blue LEDs.
 *
 * Demonstrates LVGL LED widget and button event handling.
 * Each button toggles a colored LED indicator on/off.
 */

#include "example_common.h"

static lv_obj_t *s_leds[3];
static bool s_led_state[3] = {false, false, false};

static const lv_color_t led_colors[] = {
    {0},  /* Red   — initialized in example_main */
    {0},  /* Green */
    {0},  /* Blue  */
};
static const char *led_names[] = {"Red", "Green", "Blue"};

static void led_btn_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    s_led_state[idx] = !s_led_state[idx];
    if (s_led_state[idx]) {
        lv_led_on(s_leds[idx]);
    } else {
        lv_led_off(s_leds[idx]);
    }
}

void example_main(lv_obj_t *parent)
{
    lv_color_t colors[3] = {
        lv_color_hex(0xFF0000),
        lv_color_hex(0x00FF00),
        lv_color_hex(0x0000FF),
    };

    /* Title */
    lv_obj_t *title = example_label_create(parent, "LED Toggle",
                                            &lv_font_montserrat_24, UI_COLOR_TEXT);
    /* สลับไฟ LED */
    example_label_create(parent,
        "สลับไฟ LED",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *sub = example_label_create(parent, "Tap a button to toggle the LED",
                                          &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 55);

    /* Create 3 LED + button pairs */
    int start_x = -200;
    for (int i = 0; i < 3; i++) {
        int cx = start_x + i * 200;

        /* LED indicator */
        s_leds[i] = lv_led_create(parent);
        lv_obj_set_size(s_leds[i], 60, 60);
        lv_obj_align(s_leds[i], LV_ALIGN_CENTER, cx, -40);
        lv_led_set_color(s_leds[i], colors[i]);
        lv_led_set_brightness(s_leds[i], 80);
        lv_led_off(s_leds[i]);

        /* Button */
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_size(btn, 140, 60);
        lv_obj_align(btn, LV_ALIGN_CENTER, cx, 50);
        lv_obj_set_style_bg_color(btn, colors[i], 0);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_shadow_width(btn, 8, 0);
        lv_obj_set_style_shadow_color(btn, colors[i], 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_50, 0);
        lv_obj_add_event_cb(btn, led_btn_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, led_names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_center(lbl);
    }
}
