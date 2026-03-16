/**
 * @file    main_example.c
 * @brief   Compass Heading — BMM350 magnetometer heading via IPC snapshot
 *
 * Uses snap.bmm350.heading_x10 (pre-computed by CM33_NS, 0-3600 = 0.0-360.0 deg).
 * Displays on a full-circle arc gauge with cardinal direction text.
 */

#include "example_common.h"

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

/* ── Timer — read magnetometer heading via IPC at 10 Hz ──────────── */
static void compass_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    if (!snap.has_bmm350) return;

    /* heading_x10: 0-3600 representing 0.0-360.0 degrees */
    int32_t heading_deg = snap.bmm350.heading_x10 / 10;

    lv_arc_set_value(s_arc, heading_deg);
    lv_label_set_text_fmt(s_lbl_deg, "%d", (int)heading_deg);
    lv_label_set_text(s_lbl_dir, heading_to_cardinal(heading_deg));
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "I11 \xe2\x80\x94 Compass Heading",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Arc — full 360 degrees */
    s_arc = lv_arc_create(parent);
    lv_obj_set_size(s_arc, 240, 240);
    lv_obj_align(s_arc, LV_ALIGN_CENTER, 0, 10);
    lv_arc_set_range(s_arc, 0, 360);
    lv_arc_set_bg_angles(s_arc, 0, 360);
    lv_arc_set_rotation(s_arc, 270);     /* 0 deg = top (North) */
    lv_arc_set_value(s_arc, 0);
    lv_obj_set_style_arc_color(s_arc, UI_COLOR_BMM350, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x1a3050), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc, 14, LV_PART_MAIN);
    lv_obj_remove_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Degree label at center */
    s_lbl_deg = example_label_create(parent, "0",
        &lv_font_montserrat_28, lv_color_white());
    lv_obj_align(s_lbl_deg, LV_ALIGN_CENTER, 0, -4);

    /* Cardinal direction label */
    s_lbl_dir = example_label_create(parent, "N",
        &lv_font_montserrat_20, UI_COLOR_BMM350);
    lv_obj_align(s_lbl_dir, LV_ALIGN_CENTER, 0, 24);

    /* Cardinal markers around the arc */
    static const char *marks[] = { "N", "E", "S", "W" };
    static const int mx[] = { 0, 150, 0, -150 };
    static const int my[] = { -142, 10, 150, 10 };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *m = example_label_create(parent, marks[i],
            &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
        lv_obj_align(m, LV_ALIGN_CENTER, mx[i], my[i]);
    }

    /* Start timer */
    lv_timer_create(compass_timer_cb, 100, NULL);
}
