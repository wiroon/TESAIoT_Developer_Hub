/**
 * i04_compass_rose — Production Compass with BMM350 Magnetometer
 *
 * Displays a large compass rose that rotates based on the BMM350
 * magnetometer heading. Cardinal labels (N/S/E/W) rotate with the
 * compass, heading degrees shown in the center, and cardinal text
 * direction displayed below.
 *
 * Board:  PSoC Edge E84 (AI Kit / Eva Kit)
 * Core:   CM55 (display + UI)
 */

#include "pse84_common.h"

/* ── Layout Constants ────────────────────────────────────────────── */
#define COMPASS_SIZE       250
#define COMPASS_RADIUS     (COMPASS_SIZE / 2)
#define CARDINAL_OFFSET    20    /* px from edge for N/S/E/W labels */
#define UPDATE_PERIOD_MS   100

/* ── Module State ────────────────────────────────────────────────── */
static lv_obj_t *s_compass_ring  = NULL;   /* Rotating outer ring       */
static lv_obj_t *s_heading_label = NULL;   /* Center: "235°"            */
static lv_obj_t *s_dir_label     = NULL;   /* Below compass: "SW"       */
static lv_obj_t *s_n_label       = NULL;   /* Cardinal N on ring        */
static lv_obj_t *s_s_label       = NULL;
static lv_obj_t *s_e_label       = NULL;
static lv_obj_t *s_w_label       = NULL;
static lv_timer_t *s_timer       = NULL;
static int32_t s_last_heading_x10 = -1;    /* Change gate               */

/* ── Cardinal Direction String ───────────────────────────────────── */
static const char *heading_to_cardinal(int32_t deg)
{
    if (deg >= 337 || deg < 23)   return "N";
    if (deg < 68)                 return "NE";
    if (deg < 113)                return "E";
    if (deg < 158)                return "SE";
    if (deg < 203)                return "S";
    if (deg < 248)                return "SW";
    if (deg < 293)                return "W";
    return "NW";
}

/* ── Compass Update Timer ────────────────────────────────────────── */
#if BSP_HAS_BMM350

static void compass_timer_cb(lv_timer_t *t)
{
    (void)t;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_bmm350 || !snap.bmm350_changed) return;

    int32_t heading_x10 = (int32_t)snap.bmm350.heading_x10;

    /* Change gate: skip if heading hasn't moved */
    if (heading_x10 == s_last_heading_x10) return;
    s_last_heading_x10 = heading_x10;

    int32_t deg = heading_x10 / 10;
    if (deg < 0)   deg += 360;
    if (deg >= 360) deg -= 360;

    /* Rotate the compass ring (negative = ring rotates opposite to heading)
     * LVGL uses 0.1-degree units for transform_angle */
    int32_t angle_lvgl = -(heading_x10);
    lv_obj_set_style_transform_angle(s_compass_ring, angle_lvgl, 0);

    /* Update center heading text */
    lv_label_set_text_fmt(s_heading_label, "%ld" LV_SYMBOL_DEGREE, (long)deg);

    /* Update cardinal direction */
    lv_label_set_text(s_dir_label, heading_to_cardinal(deg));
}

#endif /* BSP_HAS_BMM350 */

/* ── Create Cardinal Label on Ring ───────────────────────────────── */
static lv_obj_t *create_cardinal(lv_obj_t *ring, const char *text,
                                  lv_align_t align, int x_ofs, int y_ofs,
                                  lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(ring);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_align(lbl, align, x_ofs, y_ofs);
    return lbl;
}

/* ── Entry Point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent, LV_SYMBOL_GPS " Compass",
                                           &lv_font_montserrat_20, UI_COLOR_BMM350);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

#if BSP_HAS_BMM350

    /* Compass background circle */
    lv_obj_t *bg_circle = lv_obj_create(parent);
    lv_obj_set_size(bg_circle, COMPASS_SIZE + 20, COMPASS_SIZE + 20);
    lv_obj_align(bg_circle, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(bg_circle, (COMPASS_SIZE + 20) / 2, 0);
    lv_obj_set_style_bg_color(bg_circle, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(bg_circle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bg_circle, 2, 0);
    lv_obj_set_style_border_color(bg_circle, UI_COLOR_BMM350, 0);
    lv_obj_set_style_pad_all(bg_circle, 0, 0);
    lv_obj_clear_flag(bg_circle, LV_OBJ_FLAG_SCROLLABLE);

    /* Rotating ring container (holds cardinal letters) */
    s_compass_ring = lv_obj_create(bg_circle);
    lv_obj_remove_style_all(s_compass_ring);
    lv_obj_set_size(s_compass_ring, COMPASS_SIZE, COMPASS_SIZE);
    lv_obj_center(s_compass_ring);
    lv_obj_set_style_transform_pivot_x(s_compass_ring, COMPASS_RADIUS, 0);
    lv_obj_set_style_transform_pivot_y(s_compass_ring, COMPASS_RADIUS, 0);
    lv_obj_clear_flag(s_compass_ring, LV_OBJ_FLAG_SCROLLABLE);

    /* Cardinal direction labels on the ring */
    s_n_label = create_cardinal(s_compass_ring, "N", LV_ALIGN_TOP_MID, 0, CARDINAL_OFFSET,
                                UI_COLOR_ERROR);  /* North = red */
    s_s_label = create_cardinal(s_compass_ring, "S", LV_ALIGN_BOTTOM_MID, 0, -CARDINAL_OFFSET,
                                UI_COLOR_TEXT);
    s_e_label = create_cardinal(s_compass_ring, "E", LV_ALIGN_RIGHT_MID, -CARDINAL_OFFSET, 0,
                                UI_COLOR_TEXT);
    s_w_label = create_cardinal(s_compass_ring, "W", LV_ALIGN_LEFT_MID, CARDINAL_OFFSET, 0,
                                UI_COLOR_TEXT);

    /* Tick marks at 45-degree intervals (NE, SE, SW, NW) */
    create_cardinal(s_compass_ring, ".", LV_ALIGN_TOP_RIGHT, -35, 35, UI_COLOR_TEXT_DIM);
    create_cardinal(s_compass_ring, ".", LV_ALIGN_BOTTOM_RIGHT, -35, -35, UI_COLOR_TEXT_DIM);
    create_cardinal(s_compass_ring, ".", LV_ALIGN_BOTTOM_LEFT, 35, -35, UI_COLOR_TEXT_DIM);
    create_cardinal(s_compass_ring, ".", LV_ALIGN_TOP_LEFT, 35, 35, UI_COLOR_TEXT_DIM);

    /* Center heading display */
    s_heading_label = lv_label_create(bg_circle);
    lv_label_set_text(s_heading_label, "---" LV_SYMBOL_DEGREE);
    lv_obj_set_style_text_font(s_heading_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_heading_label, lv_color_white(), 0);
    lv_obj_center(s_heading_label);

    /* Fixed north pointer triangle (always points up = device heading) */
    lv_obj_t *pointer = lv_label_create(bg_circle);
    lv_label_set_text(pointer, LV_SYMBOL_UP);
    lv_obj_set_style_text_font(pointer, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(pointer, UI_COLOR_ERROR, 0);
    lv_obj_align(pointer, LV_ALIGN_TOP_MID, 0, 2);

    /* Cardinal direction text below compass */
    s_dir_label = lv_label_create(parent);
    lv_label_set_text(s_dir_label, "---");
    lv_obj_set_style_text_font(s_dir_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_dir_label, UI_COLOR_BMM350, 0);
    lv_obj_align(s_dir_label, LV_ALIGN_BOTTOM_MID, 0, -40);

    /* Magnetometer info */
    lv_obj_t *info = example_label_create(parent, "BMM350 Magnetometer",
                                          &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    /* เข็มทิศ */
    example_label_create(parent,
        "เข็มทิศ",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -16);

    /* Start update timer */
    s_timer = lv_timer_create(compass_timer_cb, UPDATE_PERIOD_MS, NULL);

#else
    /* BMM350 not available on this board */
    lv_obj_t *na_card = example_card_create(parent, 380, 120, UI_COLOR_CARD_BG);
    lv_obj_align(na_card, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *na_icon = example_label_create(na_card, LV_SYMBOL_WARNING,
                                             &lv_font_montserrat_28, UI_COLOR_WARNING);
    lv_obj_align(na_icon, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *na_text = example_label_create(na_card, "BMM350 not available\non this board",
                                             &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_set_style_text_align(na_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(na_text, LV_ALIGN_CENTER, 0, 10);
#endif
}
