/**
 * @file    main_example.c
 * @brief   Click Event Details — Event callback with code, target, user_data
 *
 * Three buttons share one callback. The callback identifies which button
 * was clicked using user_data and displays info in a label.
 */

#include "example_common.h"

static lv_obj_t *lbl_output;

static const char *btn_names[] = { "Sensor", "WiFi", "Settings" };
static const lv_palette_t btn_colors[] = {
    LV_PALETTE_GREEN, LV_PALETTE_BLUE, LV_PALETTE_ORANGE
};

static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const char *name = (const char *)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        lv_label_set_text_fmt(lbl_output,
            "Event: CLICKED\nTarget: %s button\nCode: %d",
            name, (int)code);
    } else if (code == LV_EVENT_PRESSED) {
        lv_label_set_text_fmt(lbl_output,
            "Event: PRESSED\nTarget: %s button\nCode: %d",
            name, (int)code);
    } else if (code == LV_EVENT_RELEASED) {
        lv_label_set_text_fmt(lbl_output,
            "Event: RELEASED\nTarget: %s button\nCode: %d",
            name, (int)code);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Click Event Details");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Button row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 500, 70);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(row);
        lv_obj_set_size(btn, 140, 50);
        lv_obj_set_style_bg_color(btn, lv_palette_main(btn_colors[i]), 0);
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, (void *)btn_names[i]);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_names[i]);
        lv_obj_center(lbl);
    }

    /* Output label */
    lbl_output = lv_label_create(parent);
    lv_label_set_text(lbl_output, "Tap a button to see event details");
    lv_obj_set_style_text_font(lbl_output, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_output, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(lbl_output, LV_ALIGN_CENTER, 0, 50);
}
