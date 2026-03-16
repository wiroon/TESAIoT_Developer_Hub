/**
 * @file main_example.c
 * @brief B13 — Compass Heading: BMM350 heading as a number + cardinal direction.
 *
 * Reads the magnetometer heading from the BMM350 and displays it as
 * degrees plus a cardinal/ordinal direction label (N, NE, E, etc.).
 */

#include "example_common.h"

static lv_obj_t *heading_label;
static lv_obj_t *cardinal_label;

/**
 * @brief Convert heading in degrees to cardinal direction string.
 */
static const char *heading_to_cardinal(uint16_t deg)
{
    static const char *dirs[] = {
        "N", "NE", "E", "SE", "S", "SW", "W", "NW"
    };
    /* Each sector spans 45 degrees. Add 22.5 (half-sector) for rounding. */
    int idx = ((deg + 22) % 360) / 45;
    if (idx < 0 || idx > 7) idx = 0;
    return dirs[idx];
}

static void compass_timer_cb(lv_timer_t *t)
{
    (void)t;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_bmm350) return;

    /* heading_x10: e.g. 1804 = 180.4 degrees */
    uint16_t heading_deg = snap.bmm350.heading_x10 / 10;
    float    heading_f   = snap.bmm350.heading_x10 / 10.0f;

    lv_label_set_text_fmt(heading_label, "%.1f", (double)heading_f);
    lv_label_set_text(cardinal_label, heading_to_cardinal(heading_deg));
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* ---- title ---- */
    example_label_create(parent, "Compass Heading",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- compass card ---- */
    lv_obj_t *card = example_card_create(parent, 300, 200,
                                         lv_color_hex(0x1E3A5F));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(card, 8, 0);

    /* ---- large cardinal direction ---- */
    cardinal_label = example_label_create(card, "--",
                                          &lv_font_montserrat_28,
                                          UI_COLOR_PRIMARY);

    /* ---- heading in degrees ---- */
    heading_label = example_label_create(card, "--.-",
                                         &lv_font_montserrat_24,
                                         UI_COLOR_TEXT);

    /* ---- unit ---- */
    example_label_create(card, "degrees",
                         &lv_font_montserrat_16, lv_color_hex(0x888888));

    /* ---- 200 ms update ---- */
    lv_timer_create(compass_timer_cb, 200, NULL);
}
