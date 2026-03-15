/**
 * @file main_example.c
 * @brief B20 — Card Layout: Golden-ratio card layout with title, subtitle, and value.
 *
 * Demonstrates a polished card-based dashboard layout using golden-ratio
 * proportions. Three cards display system metrics with structured typography:
 * title, subtitle, and a large value.
 */

#include "example_common.h"

static struct {
    const char *title;
    const char *subtitle;
    const char *value;
    lv_color_t  accent;
} cards[] = {
    { "CPU Load",   "Cortex-M55",     "42%",     {0} },
    { "Memory",     "SRAM Available",  "186 KB",  {0} },
    { "Uptime",     "Since boot",      "03:42",   {0} },
};

#define CARD_COUNT  (sizeof(cards) / sizeof(cards[0]))

/**
 * @brief Create a single information card with golden-ratio proportions.
 *
 * Card width:height ~ 1.618:1.  For w=230, h=142.
 */
static lv_obj_t *info_card_create(lv_obj_t *parent, int idx)
{
    lv_obj_t *card = example_card_create(parent, 230, 280,
                                         lv_color_hex(0x142240));
    lv_obj_set_style_border_color(card, cards[idx].accent, 0);
    lv_obj_set_style_border_width(card, 2, 0);

    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_set_style_pad_gap(card, 12, 0);

    /* ---- accent bar at top ---- */
    lv_obj_t *accent_bar = lv_obj_create(card);
    lv_obj_set_size(accent_bar, 60, 4);
    lv_obj_set_style_bg_color(accent_bar, cards[idx].accent, 0);
    lv_obj_set_style_bg_opa(accent_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(accent_bar, 2, 0);
    lv_obj_set_style_border_width(accent_bar, 0, 0);

    /* ---- title ---- */
    example_label_create(card, cards[idx].title,
                         &lv_font_montserrat_20, UI_COLOR_TEXT);

    /* ---- subtitle ---- */
    example_label_create(card, cards[idx].subtitle,
                         &lv_font_montserrat_16, lv_color_hex(0x888888));

    /* ---- large value ---- */
    example_label_create(card, cards[idx].value,
                         &lv_font_montserrat_28, cards[idx].accent);

    return card;
}

void example_main(lv_obj_t *parent)
{
    /* ---- initialize colors at runtime (LVGL 9.2 lv_color_t has no .full) ---- */
    cards[0].accent = lv_color_hex(0x4CAF50);
    cards[1].accent = lv_color_hex(0x2196F3);
    cards[2].accent = lv_color_hex(0xFF9800);

    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* ---- page title ---- */
    lv_obj_t *title = example_label_create(parent, "System Dashboard",
                                           &lv_font_montserrat_24,
                                           UI_COLOR_TEXT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    /* ---- subtitle ---- */
    lv_obj_t *sub = example_label_create(parent,
                        "BENTO PSoC Edge E84 — Card Layout Demo",
                        &lv_font_montserrat_16,
                        lv_color_hex(0x888888));
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 48);

    /* ---- card row ---- */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 760, 320);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (uint32_t i = 0; i < CARD_COUNT; i++) {
        info_card_create(row, (int)i);
    }
}
