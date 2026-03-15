/**
 * @file    main_example.c
 * @brief   Card Container — Sensor cards with timer-driven value updates
 *
 * Three Material Design cards with shadow and accent colors. A timer
 * periodically refreshes the displayed sensor values.
 *
 * Functions:
 *   create_sensor_card()     — Build a styled card with title, value, subtitle
 *   format_sensor_value()    — Format a sensor reading with unit string
 *   card_update_timer_cb()   — Timer callback: refresh card values
 *   example_main()           — Entry point: compose card row with live data
 */

#include "example_common.h"

typedef struct {
    const char     *title;
    const char     *subtitle;
    const char     *unit;
    lv_palette_t    accent;
    int             min_x10;
    int             max_x10;
} card_def_t;

static const card_def_t s_cards[] = {
    { "Temperature", "DPS368 Sensor", "\xC2\xB0""C", LV_PALETTE_ORANGE, 200, 350 },
    { "Humidity",    "SHT40 Sensor",  "%",            LV_PALETTE_BLUE,   300, 800 },
    { "Pressure",    "DPS368 Sensor", "hPa",          LV_PALETTE_GREEN,  9900, 10400 },
};
#define CARD_COUNT 3

static lv_obj_t *s_value_labels[CARD_COUNT];

/* ── Build a styled sensor card ───────────────────────────────────── */
static lv_obj_t *create_sensor_card(lv_obj_t *parent, const card_def_t *def)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 210, 220);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_shadow_width(card, 15, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_shadow_offset_y(card, 5, 0);
    lv_obj_set_style_border_color(card, lv_palette_main(def->accent), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 10, 0);

    /* Title */
    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, def->title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, lv_palette_main(LV_PALETTE_GREY), 0);

    /* Value (updated by timer) */
    lv_obj_t *lbl_value = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_value, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl_value, lv_palette_main(def->accent), 0);

    /* Subtitle */
    lv_obj_t *lbl_sub = lv_label_create(card);
    lv_label_set_text(lbl_sub, def->subtitle);
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sub, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);

    return lbl_value;
}

/* ── Format sensor value string ───────────────────────────────────── */
static void format_sensor_value(char *buf, size_t len, const card_def_t *def)
{
    int val = lv_rand(def->min_x10, def->max_x10);
    snprintf(buf, len, "%d.%d %s", val / 10, val % 10, def->unit);
}

/* ── Timer callback: refresh card values ──────────────────────────── */
static void card_update_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    char buf[24];
    for (int i = 0; i < CARD_COUNT; i++) {
        format_sensor_value(buf, sizeof(buf), &s_cards[i]);
        lv_label_set_text(s_value_labels[i], buf);
    }
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Sensor Dashboard");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Card row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 740, 280);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);

    for (int i = 0; i < CARD_COUNT; i++) {
        s_value_labels[i] = create_sensor_card(row, &s_cards[i]);
    }

    /* Initial values + start timer */
    card_update_timer_cb(NULL);
    lv_timer_create(card_update_timer_cb, 2000, NULL);
}
