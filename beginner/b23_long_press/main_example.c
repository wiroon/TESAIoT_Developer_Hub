/**
 * @file    main_example.c
 * @brief   Long Press Detection — Click vs long_press vs repeat
 *
 * A button detects click, long_press, and long_press_repeat events.
 * An output label shows which gesture was detected with counters.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *label;
    lv_obj_t *btn;
    int32_t   click_count;
    int32_t   repeat_count;
} press_ctx_t;

static press_ctx_t ctx;

static void press_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    press_ctx_t *c = (press_ctx_t *)lv_event_get_user_data(e);

    switch (code) {
    case LV_EVENT_CLICKED:
        c->click_count++;
        lv_obj_set_style_bg_color(c->btn, lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_label_set_text_fmt(c->label,
            "Event: CLICKED\nClick count: %"PRId32"\nRepeat count: %"PRId32,
            c->click_count, c->repeat_count);
        break;

    case LV_EVENT_LONG_PRESSED:
        lv_obj_set_style_bg_color(c->btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
        lv_label_set_text_fmt(c->label,
            "Event: LONG PRESSED\nClick count: %"PRId32"\nRepeat count: %"PRId32,
            c->click_count, c->repeat_count);
        break;

    case LV_EVENT_LONG_PRESSED_REPEAT:
        c->repeat_count++;
        lv_obj_set_style_bg_color(c->btn, lv_palette_main(LV_PALETTE_RED), 0);
        lv_label_set_text_fmt(c->label,
            "Event: LONG PRESS REPEAT\nClick count: %"PRId32"\nRepeat count: %"PRId32,
            c->click_count, c->repeat_count);
        break;

    default:
        break;
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.click_count = 0;
    ctx.repeat_count = 0;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Long Press Detection");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Button */
    ctx.btn = lv_btn_create(parent);
    lv_obj_set_size(ctx.btn, 200, 80);
    lv_obj_align(ctx.btn, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_bg_color(ctx.btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(ctx.btn, press_event_cb, LV_EVENT_ALL, &ctx);

    lv_obj_t *btn_lbl = lv_label_create(ctx.btn);
    lv_label_set_text(btn_lbl, "Press & Hold");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(btn_lbl);

    /* Output label */
    ctx.label = lv_label_create(parent);
    lv_label_set_text(ctx.label, "Tap or hold the button");
    lv_obj_set_style_text_font(ctx.label, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx.label, LV_ALIGN_CENTER, 0, 60);
}
