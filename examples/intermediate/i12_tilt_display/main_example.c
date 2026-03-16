/**
 * @file    main_example.c
 * @brief   Tilt Display — Roll/Pitch from BMI270 accel as dual arc gauges
 *
 * Roll = atan2(ay, az), Pitch = atan2(-ax, sqrt(ay^2+az^2)).
 * Two arc gauges: green=Roll, orange=Pitch.  Range: -90° to +90°.
 */

#include "example_common.h"
#include "sensor_bmi270.h"
#include <math.h>

#define ARC_SIZE    180

static lv_obj_t *s_arc_roll, *s_arc_pitch;
static lv_obj_t *s_lbl_roll, *s_lbl_pitch;

/* ── Helper: create one tilt gauge ───────────────────────────────── */
static void create_tilt_gauge(lv_obj_t *parent, const char *title,
                               lv_color_t color, int x_off,
                               lv_obj_t **arc_out, lv_obj_t **lbl_out)
{
    /* Card container */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 300, 260);
    lv_obj_align(card, LV_ALIGN_CENTER, x_off, 15);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x2a4060), 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *lbl_t = lv_label_create(card);
    lv_label_set_text(lbl_t, title);
    lv_obj_set_style_text_color(lbl_t, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_t, &lv_font_montserrat_16, 0);
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
    *lbl_out = lv_label_create(card);
    lv_label_set_text(*lbl_out, "0");
    lv_obj_set_style_text_color(*lbl_out, color, 0);
    lv_obj_set_style_text_font(*lbl_out, &lv_font_montserrat_24, 0);
    lv_obj_align(*lbl_out, LV_ALIGN_CENTER, 0, 10);
}

/* ── Timer — compute roll/pitch at 20 Hz ─────────────────────────── */
static void tilt_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensor_bmi270_data_t d;
    if (sensor_bmi270_read(&d) != 0) return;

    float ax = d.accel_x;
    float ay = d.accel_y;
    float az = d.accel_z;

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
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I12 — Tilt Display (Roll / Pitch)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    create_tilt_gauge(parent, "Roll", lv_palette_main(LV_PALETTE_GREEN),
                      -170, &s_arc_roll, &s_lbl_roll);
    create_tilt_gauge(parent, "Pitch", lv_color_hex(0xFF9800),
                       170, &s_arc_pitch, &s_lbl_pitch);

    sensor_bmi270_init();
    lv_timer_create(tilt_timer_cb, 50, NULL);
}
