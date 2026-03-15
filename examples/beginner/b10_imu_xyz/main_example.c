/**
 * @file main_example.c
 * @brief B10 — IMU XYZ: Show BMI270 accelerometer X/Y/Z in real time.
 *
 * Reads the shared sensor snapshot via IPC and displays the three
 * accelerometer axes as g-force values, updated every 100 ms.
 */

#include "example_common.h"

static lv_obj_t *lbl_x;
static lv_obj_t *lbl_y;
static lv_obj_t *lbl_z;

static void sensor_timer_cb(lv_timer_t *t)
{
    (void)t;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_bmi270) return;

    /* Convert raw int16 to g: raw / 16384.0 */
    float ax = snap.bmi270.ax / 16384.0f;
    float ay = snap.bmi270.ay / 16384.0f;
    float az = snap.bmi270.az / 16384.0f;

    lv_label_set_text_fmt(lbl_x, "X: %+.2f g", (double)ax);
    lv_label_set_text_fmt(lbl_y, "Y: %+.2f g", (double)ay);
    lv_label_set_text_fmt(lbl_z, "Z: %+.2f g", (double)az);
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 20, 0);

    /* ---- title ---- */
    example_label_create(parent, "IMU Accelerometer",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- sensor card ---- */
    lv_obj_t *card = example_card_create(parent, 400, 200,
                                         lv_color_hex(0x1E3A5F));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_set_style_pad_gap(card, 12, 0);

    /* ---- axis labels ---- */
    lbl_x = example_label_create(card, "X: --",
                                 &lv_font_montserrat_24, UI_COLOR_ERROR);
    lbl_y = example_label_create(card, "Y: --",
                                 &lv_font_montserrat_24, UI_COLOR_SUCCESS);
    lbl_z = example_label_create(card, "Z: --",
                                 &lv_font_montserrat_24, UI_COLOR_PRIMARY);

    /* ---- 100 ms update timer ---- */
    lv_timer_create(sensor_timer_cb, 100, NULL);
}
