/**
 * @file main_example.c
 * @brief B09 — Dropdown Menu: A dropdown with 5 options, selection shown below.
 *
 * Demonstrates the LVGL dropdown widget with an event handler that reads
 * the selected item and displays it in a separate label.
 */

#include "example_common.h"

static lv_obj_t *sel_label;

static void dd_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    char buf[48];
    lv_dropdown_get_selected_str(dd, buf, sizeof(buf));
    lv_label_set_text_fmt(sel_label, "Selected: %s", buf);
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
    example_label_create(parent, "Dropdown Menu",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- dropdown ---- */
    lv_obj_t *dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd,
        "Temperature\n"
        "Humidity\n"
        "Pressure\n"
        "Acceleration\n"
        "Compass");
    lv_obj_set_width(dd, 300);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_20, 0);
    lv_obj_set_style_bg_color(dd, lv_color_hex(0x1E3A5F), 0);
    lv_obj_set_style_text_color(dd, UI_COLOR_TEXT, 0);
    lv_obj_set_style_border_color(dd, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(dd, 2, 0);

    /* Style the dropdown list (opened state) */
    lv_obj_t *list = lv_dropdown_get_list(dd);
    if (list) {
        lv_obj_set_style_bg_color(list, lv_color_hex(0x1E3A5F), 0);
        lv_obj_set_style_text_color(list, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(list, &lv_font_montserrat_20, 0);
    }

    lv_obj_add_event_cb(dd, dd_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ---- selection display ---- */
    sel_label = example_label_create(parent, "Selected: Temperature",
                                     &lv_font_montserrat_20,
                                     UI_COLOR_PRIMARY);
}
