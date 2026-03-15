/**
 * @file    main_example.c
 * @brief   Dropdown Select — Sensor list with selection display
 *
 * A dropdown lists sensor names. Selecting an item updates a label
 * to show the chosen sensor name.
 */

#include "example_common.h"

static lv_obj_t *lbl_selected;

static void dropdown_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *dd = lv_event_get_target(e);
        char buf[64];
        lv_dropdown_get_selected_str(dd, buf, sizeof(buf));
        lv_label_set_text_fmt(lbl_selected, "Selected: %s", buf);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Choose a Sensor:");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* Dropdown */
    lv_obj_t *dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd,
        "BMI270 (IMU)\n"
        "DPS368 (Barometer)\n"
        "SHT40 (Climate)\n"
        "BMM350 (Magnetometer)\n"
        "Radar (Presence)");
    lv_obj_set_width(dd, 300);
    lv_obj_align(dd, LV_ALIGN_CENTER, 0, -20);
    lv_obj_add_event_cb(dd, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Selected item label */
    lbl_selected = lv_label_create(parent);
    lv_label_set_text(lbl_selected, "Selected: BMI270 (IMU)");
    lv_obj_set_style_text_font(lbl_selected, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_selected, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(lbl_selected, LV_ALIGN_CENTER, 0, 50);
}
