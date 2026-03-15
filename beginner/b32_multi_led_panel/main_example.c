/**
 * @file    main_example.c
 * @brief   Multi LED Control Panel — Grid control for all board LEDs
 *
 * BSP-guarded LED definitions for AI Kit (5 LEDs) and Eva Kit (2 LEDs).
 * Grid layout with switch controls and virtual LED indicators.
 */

#include "example_common.h"
#include "cyhal_gpio.h"

typedef struct {
    const char       *name;
    cyhal_gpio_t      pin;
    lv_palette_t      color;
    lv_obj_t         *led_widget;
    lv_obj_t         *sw_widget;
    bool              initialized;
} led_def_t;

#if BSP_HAS_DPS368  /* AI Kit identifier */
#define NUM_LEDS 5
static led_def_t leds[NUM_LEDS] = {
    { "Red",    P13_4, LV_PALETTE_RED,    NULL, NULL, false },
    { "Green",  P13_5, LV_PALETTE_GREEN,  NULL, NULL, false },
    { "Blue",   P13_6, LV_PALETTE_BLUE,   NULL, NULL, false },
    { "Orange", P13_7, LV_PALETTE_ORANGE, NULL, NULL, false },
    { "White",  P14_0, LV_PALETTE_GREY,   NULL, NULL, false },
};
#else  /* Eva Kit */
#define NUM_LEDS 2
static led_def_t leds[NUM_LEDS] = {
    { "LED1", P13_7, LV_PALETTE_RED,   NULL, NULL, false },
    { "LED2", P14_0, LV_PALETTE_GREEN, NULL, NULL, false },
};
#endif

static void switch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        led_def_t *led = (led_def_t *)lv_event_get_user_data(e);
        lv_obj_t *sw = lv_event_get_target(e);
        bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);

        if (led->initialized) {
            cyhal_gpio_write(led->pin, on);
        }
        if (on) {
            lv_led_on(led->led_widget);
        } else {
            lv_led_off(led->led_widget);
        }
    }
}

static void all_on_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (leds[i].initialized) cyhal_gpio_write(leds[i].pin, true);
        lv_led_on(leds[i].led_widget);
        lv_obj_add_state(leds[i].sw_widget, LV_STATE_CHECKED);
    }
}

static void all_off_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (leds[i].initialized) cyhal_gpio_write(leds[i].pin, false);
        lv_led_off(leds[i].led_widget);
        lv_obj_remove_state(leds[i].sw_widget, LV_STATE_CHECKED);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Initialize all LED GPIOs */
    for (int i = 0; i < NUM_LEDS; i++) {
        cy_rslt_t r = cyhal_gpio_init(leds[i].pin, CYHAL_GPIO_DIR_OUTPUT,
                                       CYHAL_GPIO_DRIVE_STRONG, false);
        leds[i].initialized = (r == CY_RSLT_SUCCESS);
    }

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text_fmt(title, "LED Control Panel (%d LEDs)", NUM_LEDS);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* LED control rows */
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, 500, 250);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_style_pad_all(list, 10, 0);

    for (int i = 0; i < NUM_LEDS; i++) {
        lv_obj_t *row = lv_obj_create(list);
        lv_obj_set_size(row, LV_PCT(100), 40);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
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
        lv_obj_add_event_cb(leds[i].sw_widget, switch_event_cb, LV_EVENT_VALUE_CHANGED, &leds[i]);
    }

    /* All ON / All OFF buttons */
    lv_obj_t *btn_on = lv_btn_create(parent);
    lv_obj_set_size(btn_on, 140, 45);
    lv_obj_align(btn_on, LV_ALIGN_BOTTOM_LEFT, 80, -10);
    lv_obj_set_style_bg_color(btn_on, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_on, all_on_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lon = lv_label_create(btn_on);
    lv_label_set_text(lon, "All ON");
    lv_obj_center(lon);

    lv_obj_t *btn_off = lv_btn_create(parent);
    lv_obj_set_size(btn_off, 140, 45);
    lv_obj_align(btn_off, LV_ALIGN_BOTTOM_RIGHT, -80, -10);
    lv_obj_set_style_bg_color(btn_off, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_off, all_off_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *loff = lv_label_create(btn_off);
    lv_label_set_text(loff, "All OFF");
    lv_obj_center(loff);
}
