/**
 * @file    main_example.c
 * @brief   Compass Heading — BMM350 magnetometer → 360° arc + cardinal label
 *
 * Computes heading via atan2f(mag_y, mag_x), displays on a full-circle
 * arc gauge with cardinal direction text (N, NE, E, ...).
 */

#include "example_common.h"
#include "sensor_bmm350.h"
#include <math.h>

static lv_obj_t *s_arc;
static lv_obj_t *s_lbl_deg;
static lv_obj_t *s_lbl_dir;

/* ── Cardinal direction from heading ─────────────────────────────── */
static const char *heading_to_cardinal(int32_t deg)
{
    static const char *dirs[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    int idx = ((deg + 22) % 360) / 45;
    if (idx < 0) idx = 0;
    if (idx > 7) idx = 7;
    return dirs[idx];
}

/* ── Timer — read magnetometer at 10 Hz ──────────────────────────── */
static void compass_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensor_bmm350_data_t d;
    if (sensor_bmm350_read(&d) != 0) return;

    /* Compute heading from X/Y components */
    float heading_rad = atan2f(d.mag_y, d.mag_x);
    int32_t heading_deg = (int32_t)(heading_rad * 180.0f / 3.14159265f);
    if (heading_deg < 0) heading_deg += 360;

    lv_arc_set_value(s_arc, heading_deg);
    lv_label_set_text_fmt(s_lbl_deg, "%d", (int)heading_deg);
    lv_label_set_text(s_lbl_dir, heading_to_cardinal(heading_deg));
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I11 — Compass Heading");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Arc — full 360° */
    s_arc = lv_arc_create(parent);
    lv_obj_set_size(s_arc, 240, 240);
    lv_obj_align(s_arc, LV_ALIGN_CENTER, 0, 10);
    lv_arc_set_range(s_arc, 0, 360);
    lv_arc_set_bg_angles(s_arc, 0, 360);
    lv_arc_set_rotation(s_arc, 270);     /* 0° = top (North) */
    lv_arc_set_value(s_arc, 0);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0xE040FB), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x1a3050), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc, 14, LV_PART_MAIN);
    lv_obj_remove_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Degree label at center */
    s_lbl_deg = lv_label_create(parent);
    lv_label_set_text(s_lbl_deg, "0");
    lv_obj_set_style_text_font(s_lbl_deg, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_lbl_deg, lv_color_white(), 0);
    lv_obj_align(s_lbl_deg, LV_ALIGN_CENTER, 0, -4);

    /* Cardinal direction label */
    s_lbl_dir = lv_label_create(parent);
    lv_label_set_text(s_lbl_dir, "N");
    lv_obj_set_style_text_font(s_lbl_dir, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_lbl_dir, lv_color_hex(0xE040FB), 0);
    lv_obj_align(s_lbl_dir, LV_ALIGN_CENTER, 0, 24);

    /* Cardinal markers around the arc */
    static const char *marks[] = { "N", "E", "S", "W" };
    static const int mx[] = { 0, 150, 0, -150 };
    static const int my[] = { -142, 10, 150, 10 };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *m = lv_label_create(parent);
        lv_label_set_text(m, marks[i]);
        lv_obj_set_style_text_font(m, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(m, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_align(m, LV_ALIGN_CENTER, mx[i], my[i]);
    }

    /* Init sensor + timer */
    sensor_bmm350_init();
    lv_timer_create(compass_timer_cb, 100, NULL);
}
