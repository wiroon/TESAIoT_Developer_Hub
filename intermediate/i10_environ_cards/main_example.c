/**
 * @file    main_example.c
 * @brief   Environmental Cards — Temp, Pressure, Humidity arc gauges via IPC
 *
 * AI Kit only (DPS368 + SHT40).  Three cards with arc gauges updated
 * every 1 second from IPC sensor snapshot.
 */

#include "example_common.h"

#if BSP_HAS_DPS368 && BSP_HAS_SHT40

#define CARD_W      220
#define CARD_H      200
#define ARC_SIZE    130

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *lbl_value;
} gauge_t;

static gauge_t s_temp, s_pres, s_humi;

/* ── Helper: create an arc-gauge card ────────────────────────────── */
static gauge_t create_gauge_card(lv_obj_t *parent, const char *title,
                                  lv_color_t color, int32_t range_min,
                                  int32_t range_max, int x_off)
{
    gauge_t g;

    lv_obj_t *card = example_card_create(parent, CARD_W, CARD_H, UI_COLOR_CARD_BG);
    lv_obj_align(card, LV_ALIGN_CENTER, x_off, 10);

    /* Title */
    lv_obj_t *lbl_title = example_label_create(card, title,
        &lv_font_montserrat_14, lv_color_white());
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 4);

    /* Arc */
    g.arc = lv_arc_create(card);
    lv_obj_set_size(g.arc, ARC_SIZE, ARC_SIZE);
    lv_obj_align(g.arc, LV_ALIGN_CENTER, 0, 8);
    lv_arc_set_range(g.arc, range_min, range_max);
    lv_arc_set_value(g.arc, range_min);
    lv_arc_set_bg_angles(g.arc, 135, 45);
    lv_obj_set_style_arc_color(g.arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(g.arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(g.arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_color(g.arc, lv_color_hex(0x1a3050), LV_PART_MAIN);
    lv_obj_remove_flag(g.arc, LV_OBJ_FLAG_CLICKABLE);

    /* Center value label */
    g.lbl_value = example_label_create(card, "--",
        &lv_font_montserrat_20, color);
    lv_obj_align(g.lbl_value, LV_ALIGN_CENTER, 0, 8);

    return g;
}

/* ── Timer — read sensors via IPC at 1 Hz ────────────────────────── */
static void env_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (snap.has_dps368) {
        /* temperature_x100: Celsius * 100 */
        int32_t t = snap.dps368.temperature_x100 / 100;
        lv_arc_set_value(s_temp.arc, t);
        lv_label_set_text_fmt(s_temp.lbl_value, "%d.%d C",
            (int)(snap.dps368.temperature_x100 / 100),
            (int)(abs(snap.dps368.temperature_x100) % 100 / 10));

        /* pressure_x100: hPa * 100 */
        int32_t p = snap.dps368.pressure_x100 / 100;
        lv_arc_set_value(s_pres.arc, p);
        lv_label_set_text_fmt(s_pres.lbl_value, "%d hPa", (int)p);
    }

    if (snap.has_sht40) {
        /* humidity_x100: %RH * 100 */
        int32_t h = snap.sht40.humidity_x100 / 100;
        lv_arc_set_value(s_humi.arc, h);
        lv_label_set_text_fmt(s_humi.lbl_value, "%d.%d %%",
            (int)(snap.sht40.humidity_x100 / 100),
            (int)(snap.sht40.humidity_x100 % 100 / 10));
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = example_label_create(parent,
        "I10 \xe2\x80\x94 Environmental Cards",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    s_temp = create_gauge_card(parent, "Temperature",
                               UI_COLOR_DPS368, 0, 50, -250);
    s_pres = create_gauge_card(parent, "Pressure",
                               UI_COLOR_PRIMARY, 900, 1100, 0);
    s_humi = create_gauge_card(parent, "Humidity",
                               UI_COLOR_SHT40, 0, 100, 250);

    lv_timer_create(env_timer_cb, 1000, NULL);
}

#else /* BSP_HAS_DPS368 && BSP_HAS_SHT40 */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "I10 \xe2\x80\x94 Environmental Cards\n\n"
        "Requires DPS368 + SHT40 (AI Kit only).\n"
        "Not available on this board.",
        &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
    lv_obj_center(lbl);
}

#endif
