/**
 * @file    main_example.c
 * @brief   Environmental Cards — Temp, Pressure, Humidity arc gauges
 *
 * AI Kit only (DPS368 + SHT40).  Three cards with arc gauges updated
 * every 1 second from live sensor reads.
 */

#include "example_common.h"

#if BSP_HAS_DPS368 && BSP_HAS_SHT40

#include "sensor_dps368.h"
#include "sensor_sht40.h"

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

    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, CARD_W, CARD_H);
    lv_obj_align(card, LV_ALIGN_CENTER, x_off, 10);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x2a4060), 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
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
    g.lbl_value = lv_label_create(card);
    lv_label_set_text(g.lbl_value, "--");
    lv_obj_set_style_text_color(g.lbl_value, color, 0);
    lv_obj_set_style_text_font(g.lbl_value, &lv_font_montserrat_20, 0);
    lv_obj_align(g.lbl_value, LV_ALIGN_CENTER, 0, 8);

    return g;
}

/* ── Timer — read sensors at 1 Hz ────────────────────────────────── */
static void env_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    sensor_dps368_data_t dps;
    sensor_sht40_data_t  sht;

    if (sensor_dps368_read(&dps) == 0) {
        int32_t t = (int32_t)(dps.temperature);
        lv_arc_set_value(s_temp.arc, t);
        lv_label_set_text_fmt(s_temp.lbl_value, "%.1f C", dps.temperature);

        int32_t p = (int32_t)(dps.pressure);
        lv_arc_set_value(s_pres.arc, p);
        lv_label_set_text_fmt(s_pres.lbl_value, "%.0f hPa", dps.pressure);
    }

    if (sensor_sht40_read(&sht) == 0) {
        int32_t h = (int32_t)(sht.humidity);
        lv_arc_set_value(s_humi.arc, h);
        lv_label_set_text_fmt(s_humi.lbl_value, "%.1f %%", sht.humidity);
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I10 — Environmental Cards");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    s_temp = create_gauge_card(parent, "Temperature",
                               lv_color_hex(0xFF9800), 0, 50, -250);
    s_pres = create_gauge_card(parent, "Pressure",
                               lv_palette_main(LV_PALETTE_CYAN), 900, 1100, 0);
    s_humi = create_gauge_card(parent, "Humidity",
                               lv_color_hex(0x2196F3), 0, 100, 250);

    sensor_dps368_init();
    sensor_sht40_init();
    lv_timer_create(env_timer_cb, 1000, NULL);
}

#else /* BSP_HAS_DPS368 && BSP_HAS_SHT40 */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "I10 — Environmental Cards\n\n"
                           "Requires DPS368 + SHT40 (AI Kit only).\n"
                           "Not available on this board.");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_center(lbl);
}

#endif
