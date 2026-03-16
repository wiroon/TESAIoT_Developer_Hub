/**
 * @file    main_example.c
 * @brief   Tilt Display — Roll/Pitch from BMI270 accel via IPC as dual arc gauges
 *
 * Roll = atan2(ay, az), Pitch = atan2(-ax, sqrt(ay^2+az^2)).
 * Two arc gauges: green=Roll, orange=Pitch.  Range: -90 to +90 deg.
 */

#include "example_common.h"

#define ARC_SIZE    180

static lv_obj_t *s_arc_roll, *s_arc_pitch;
static lv_obj_t *s_lbl_roll, *s_lbl_pitch;

/* ── Helper: create one tilt gauge ───────────────────────────────── */
static void create_tilt_gauge(lv_obj_t *parent, const char *title,
                               lv_color_t color, int x_off,
                               lv_obj_t **arc_out, lv_obj_t **lbl_out)
{
    /* Card container */
    lv_obj_t *card = example_card_create(parent, 300, 260, UI_COLOR_CARD_BG);
    lv_obj_align(card, LV_ALIGN_CENTER, x_off, 15);

    /* Title */
    lv_obj_t *lbl_t = example_label_create(card, title,
        &lv_font_montserrat_16, lv_color_white());
    lv_obj_align(lbl_t, LV_ALIGN_TOP_MID, 0, 6);

    /* Arc */
    *arc_out = lv_arc_create(card);
    lv_obj_set_size(*arc_out, ARC_SIZE, ARC_SIZE);
    lv_obj_align(*arc_out, LV_ALIGN_CENTER, 0, 10);
    lv_arc_set_range(*arc_out, -90, 90);
    lv_arc_set_bg_angles(*arc_out, 135, 45);
    lv_arc_set_value(*arc_out, 0);
    lv_obj_set_style_arc_color(*arc_out, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(*arc_out, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(*arc_out, lv_color_hex(0x1a3050), LV_PART_MAIN);
    lv_obj_set_style_arc_width(*arc_out, 12, LV_PART_MAIN);
    lv_obj_remove_flag(*arc_out, LV_OBJ_FLAG_CLICKABLE);

    /* Center value */
    *lbl_out = example_label_create(card, "0",
        &lv_font_montserrat_24, color);
    lv_obj_align(*lbl_out, LV_ALIGN_CENTER, 0, 10);
}

/* ── Timer — compute roll/pitch via IPC at 20 Hz ─────────────────── */
static void tilt_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    if (!snap.has_bmi270) return;

    /* Convert raw to g */
    float ax = snap.bmi270.ax / 16384.0f;
    float ay = snap.bmi270.ay / 16384.0f;
    float az = snap.bmi270.az / 16384.0f;

    float roll  = atan2f(ay, az) * 180.0f / 3.14159265f;
    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / 3.14159265f;

    int32_t r = (int32_t)roll;
    int32_t p = (int32_t)pitch;

    /* Clamp to -90..90 */
    if (r < -90) r = -90; if (r > 90) r = 90;
    if (p < -90) p = -90; if (p > 90) p = 90;

    lv_arc_set_value(s_arc_roll, r);
    lv_arc_set_value(s_arc_pitch, p);
    lv_label_set_text_fmt(s_lbl_roll, "%d", (int)r);
    lv_label_set_text_fmt(s_lbl_pitch, "%d", (int)p);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = example_label_create(parent,
        "I12 \xe2\x80\x94 Tilt Display (Roll / Pitch)",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    create_tilt_gauge(parent, "Roll", UI_COLOR_BMI270,
                      -170, &s_arc_roll, &s_lbl_roll);
    create_tilt_gauge(parent, "Pitch", UI_COLOR_DPS368,
                       170, &s_arc_pitch, &s_lbl_pitch);

    lv_timer_create(tilt_timer_cb, 50, NULL);
}
