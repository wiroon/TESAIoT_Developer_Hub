/**
 * @file    main_example.c
 * @brief   Dropdown Select — Sensor list with detail panel
 *
 * A dropdown lists sensor names. Selecting an item updates a detail
 * panel showing the sensor name, type, and interface info.
 *
 * Functions:
 *   get_sensor_detail()      — Return detail string for selected sensor
 *   update_detail_panel()    — Refresh selection label and detail text
 *   dropdown_event_cb()      — Event callback: handle dropdown change
 *   example_main()           — Entry point: compose dropdown + detail UI
 */

#include "example_common.h"

static lv_obj_t *s_lbl_selected;
static lv_obj_t *s_lbl_detail;

static const char *s_sensor_details[] = {
    "6-axis IMU (Accel + Gyro)\nInterface: I3C / I2C\nRange: 2g / 2000 dps",
    "Barometric Pressure\nInterface: I2C / SPI\nRange: 300-1200 hPa",
    "Temperature & Humidity\nInterface: I2C\nAccuracy: 0.2C / 1.8% RH",
    "3-axis Magnetometer\nInterface: I3C / I2C\nRange: 2000 uT",
    "60 GHz Presence Detection\nInterface: SPI\nRange: 0-5 m",
};

/* ── Get detail string for sensor index ──────────────────────────── */
static const char *get_sensor_detail(uint16_t idx)
{
    if (idx < 5) return s_sensor_details[idx];
    return "Unknown sensor";
}

/* ── Update detail panel with selected sensor ────────────────────── */
static void update_detail_panel(lv_obj_t *dd)
{
    char buf[64];
    lv_dropdown_get_selected_str(dd, buf, sizeof(buf));
    lv_label_set_text_fmt(s_lbl_selected, "Selected: %s", buf);

    uint16_t idx = lv_dropdown_get_selected(dd);
    lv_label_set_text(s_lbl_detail, get_sensor_detail(idx));
}

/* ── Dropdown event callback ─────────────────────────────────────── */
static void dropdown_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *dd = lv_event_get_target(e);
        update_detail_panel(dd);
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
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
    lv_obj_align(dd, LV_ALIGN_CENTER, 0, -30);
    lv_obj_add_event_cb(dd, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Selected item label */
    s_lbl_selected = lv_label_create(parent);
    lv_label_set_text(s_lbl_selected, "Selected: BMI270 (IMU)");
    lv_obj_set_style_text_font(s_lbl_selected, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_selected, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(s_lbl_selected, LV_ALIGN_CENTER, 0, 30);

    /* Detail label */
    s_lbl_detail = lv_label_create(parent);
    lv_label_set_text(s_lbl_detail, get_sensor_detail(0));
    lv_obj_set_style_text_font(s_lbl_detail, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_detail, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_detail, LV_ALIGN_CENTER, 0, 80);
}
