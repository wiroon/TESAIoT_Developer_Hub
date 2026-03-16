/**
 * @file    main_example.c
 * @brief   User Data Callback — Struct pointers differentiate buttons
 *
 * Four buttons share one callback. Each button passes a struct with
 * a name and color, used to update the display panel on click.
 */

#include "example_common.h"

typedef struct {
    const char  *name;
    lv_color_t   color;
} btn_data_t;

static btn_data_t btn_data[] = {
    { "Accelerometer", { .blue = 0x50, .green = 0xAF, .red = 0x4C } },
    { "Barometer",     { .blue = 0x00, .green = 0x98, .red = 0xFF } },
    { "Climate",       { .blue = 0xF3, .green = 0x96, .red = 0x21 } },
    { "Magnetometer",  { .blue = 0xFB, .green = 0x40, .red = 0xE0 } },
};

static lv_obj_t *lbl_output;
static lv_obj_t *panel;

static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        btn_data_t *data = (btn_data_t *)lv_event_get_user_data(e);
        lv_label_set_text_fmt(lbl_output, "Selected: %s", data->name);
        lv_obj_set_style_bg_color(panel, data->color, 0);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "User Data Pattern");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Button row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 700, 60);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(row);
        lv_obj_set_size(btn, 150, 45);
        lv_obj_set_style_bg_color(btn, btn_data[i].color, 0);
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, &btn_data[i]);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_data[i].name);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(lbl);
    }

    /* Display panel */
    panel = lv_obj_create(parent);
    lv_obj_set_size(panel, 350, 160);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_radius(panel, 16, 0);
    lv_obj_set_style_bg_color(panel, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_30, 0);

    lbl_output = lv_label_create(panel);
    lv_label_set_text(lbl_output, "Tap a sensor button");
    lv_obj_set_style_text_font(lbl_output, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_output);
}
