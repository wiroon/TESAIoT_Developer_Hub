/**
 * @file    main_example.c
 * @brief   CapSense Panel — Buttons (LEDs) + Slider (bar) — Eva Kit only
 *
 * Reads CapSense buttons and slider at 30 ms.  Two LED widgets for buttons,
 * one bar widget for the linear slider position (0-100%).
 */

#include "example_common.h"

#if BSP_HAS_CAPSENSE

#include "sensor_capsense.h"

static lv_obj_t *s_led0, *s_led1;
static lv_obj_t *s_bar;
static lv_obj_t *s_lbl_btn0, *s_lbl_btn1;
static lv_obj_t *s_lbl_slider;

/* ── Timer — poll CapSense at ~33 Hz ─────────────────────────────── */
static void capsense_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensor_capsense_data_t d;
    if (sensor_capsense_read(&d) != 0) return;

    /* Buttons */
    if (d.button0_active) {
        lv_led_on(s_led0);
        lv_obj_set_style_bg_color(s_led0, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_label_set_text(s_lbl_btn0, "BTN0: ON");
    } else {
        lv_led_off(s_led0);
        lv_obj_set_style_bg_color(s_led0, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_label_set_text(s_lbl_btn0, "BTN0: OFF");
    }

    if (d.button1_active) {
        lv_led_on(s_led1);
        lv_obj_set_style_bg_color(s_led1, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_label_set_text(s_lbl_btn1, "BTN1: ON");
    } else {
        lv_led_off(s_led1);
        lv_obj_set_style_bg_color(s_led1, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_label_set_text(s_lbl_btn1, "BTN1: OFF");
    }

    /* Slider → bar */
    int32_t pos = (int32_t)d.slider_position;
    lv_bar_set_value(s_bar, pos, LV_ANIM_ON);
    lv_label_set_text_fmt(s_lbl_slider, "Slider: %d %%", (int)pos);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I13 — CapSense Panel (Eva Kit)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Button LEDs */
    s_led0 = lv_led_create(parent);
    lv_obj_set_size(s_led0, 60, 60);
    lv_obj_align(s_led0, LV_ALIGN_CENTER, -120, -40);
    lv_led_off(s_led0);

    s_lbl_btn0 = lv_label_create(parent);
    lv_label_set_text(s_lbl_btn0, "BTN0: OFF");
    lv_obj_set_style_text_font(s_lbl_btn0, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_btn0, lv_color_white(), 0);
    lv_obj_align(s_lbl_btn0, LV_ALIGN_CENTER, -120, 10);

    s_led1 = lv_led_create(parent);
    lv_obj_set_size(s_led1, 60, 60);
    lv_obj_align(s_led1, LV_ALIGN_CENTER, 120, -40);
    lv_led_off(s_led1);

    s_lbl_btn1 = lv_label_create(parent);
    lv_label_set_text(s_lbl_btn1, "BTN1: OFF");
    lv_obj_set_style_text_font(s_lbl_btn1, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_btn1, lv_color_white(), 0);
    lv_obj_align(s_lbl_btn1, LV_ALIGN_CENTER, 120, 10);

    /* Slider bar */
    s_bar = lv_bar_create(parent);
    lv_obj_set_size(s_bar, 500, 30);
    lv_obj_align(s_bar, LV_ALIGN_CENTER, 0, 80);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar, lv_palette_main(LV_PALETTE_TEAL), LV_PART_INDICATOR);

    s_lbl_slider = lv_label_create(parent);
    lv_label_set_text(s_lbl_slider, "Slider: 0 %");
    lv_obj_set_style_text_font(s_lbl_slider, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_slider, lv_palette_main(LV_PALETTE_TEAL), 0);
    lv_obj_align(s_lbl_slider, LV_ALIGN_CENTER, 0, 120);

    /* Init + timer */
    sensor_capsense_init();
    lv_timer_create(capsense_timer_cb, 30, NULL);
}

#else /* BSP_HAS_CAPSENSE */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "I13 — CapSense Panel\n\n"
                           "Requires CapSense (Eva Kit only).\n"
                           "Not available on this board.");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_center(lbl);
}

#endif /* BSP_HAS_CAPSENSE */
