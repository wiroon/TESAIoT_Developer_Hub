/**
 * @file    main_example.c
 * @brief   CapSense Buttons — Touch buttons and slider (Eva Kit only)
 *
 * Reads CapSense touch buttons and slider position every 50ms.
 * Eva Kit only (BSP_HAS_CAPSENSE).
 */

#include "example_common.h"

#if BSP_HAS_CAPSENSE
#include "sensor_capsense.h"

#define NUM_CS_BTNS 3

typedef struct {
    lv_obj_t *btn_leds[NUM_CS_BTNS];
    lv_obj_t *btn_labels[NUM_CS_BTNS];
    lv_obj_t *slider_widget;
    lv_obj_t *lbl_slider_val;
    bool      sensor_ok;
} capsense_ctx_t;

static capsense_ctx_t ctx;

static void capsense_timer_cb(lv_timer_t *timer)
{
    capsense_ctx_t *c = (capsense_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->sensor_ok) return;

    /* Read buttons */
    uint8_t buttons = 0;
    capsense_read_buttons(&buttons);
    for (int i = 0; i < NUM_CS_BTNS; i++) {
        bool pressed = (buttons >> i) & 0x01;
        if (pressed) {
            lv_led_on(c->btn_leds[i]);
            lv_obj_set_style_text_color(c->btn_labels[i], lv_palette_main(LV_PALETTE_GREEN), 0);
        } else {
            lv_led_off(c->btn_leds[i]);
            lv_obj_set_style_text_color(c->btn_labels[i], lv_palette_main(LV_PALETTE_GREY), 0);
        }
    }

    /* Read slider */
    uint16_t slider_pos = 0;
    capsense_read_slider(&slider_pos);
    lv_slider_set_value(c->slider_widget, (int32_t)slider_pos, LV_ANIM_ON);
    lv_label_set_text_fmt(c->lbl_slider_val, "Slider: %u", slider_pos);
}

void example_main(lv_obj_t *parent)
{
    ctx.sensor_ok = (capsense_init() == 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "CapSense Touch (Eva Kit)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    if (!ctx.sensor_ok) {
        lv_obj_t *err = lv_label_create(parent);
        lv_label_set_text(err, "CapSense init failed");
        lv_obj_set_style_text_color(err, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_align(err, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    /* Button row */
    lv_obj_t *btn_row = lv_obj_create(parent);
    lv_obj_set_size(btn_row, 500, 120);
    lv_obj_align(btn_row, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);

    const char *btn_names[] = { "BTN 0", "BTN 1", "BTN 2" };
    for (int i = 0; i < NUM_CS_BTNS; i++) {
        lv_obj_t *col = lv_obj_create(btn_row);
        lv_obj_set_size(col, 130, 100);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);

        ctx.btn_leds[i] = lv_led_create(col);
        lv_obj_set_size(ctx.btn_leds[i], 50, 50);
        lv_led_set_color(ctx.btn_leds[i], lv_palette_main(LV_PALETTE_CYAN));
        lv_led_off(ctx.btn_leds[i]);

        ctx.btn_labels[i] = lv_label_create(col);
        lv_label_set_text(ctx.btn_labels[i], btn_names[i]);
        lv_obj_set_style_text_font(ctx.btn_labels[i], &lv_font_montserrat_14, 0);
    }

    /* Slider display */
    ctx.slider_widget = lv_slider_create(parent);
    lv_obj_set_width(ctx.slider_widget, 400);
    lv_slider_set_range(ctx.slider_widget, 0, 300);
    lv_slider_set_value(ctx.slider_widget, 0, LV_ANIM_OFF);
    lv_obj_remove_flag(ctx.slider_widget, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(ctx.slider_widget, LV_ALIGN_CENTER, 0, 50);

    ctx.lbl_slider_val = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_slider_val, "Slider: 0");
    lv_obj_set_style_text_font(ctx.lbl_slider_val, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx.lbl_slider_val, LV_ALIGN_CENTER, 0, 80);

    /* Timer: 50ms */
    lv_timer_create(capsense_timer_cb, 50, &ctx);
}

#else

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "CapSense not available on this board.\nThis example requires Eva Kit.");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_center(lbl);
}

#endif
