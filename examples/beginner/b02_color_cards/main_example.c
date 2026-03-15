/**
 * @file main_example.c
 * @brief B02 — Color Cards: Four colored cards arranged in a 2x2 grid.
 *
 * Demonstrates creating styled card containers with different background
 * colors and arranging them using LVGL flex layout.
 */

#include "example_common.h"

/* Card definitions: color + label text */
static struct {
    lv_color_t color;
    const char *name;
} cards[] = {
    { {0}, "Green"  },   /* Material Green  */
    { {0}, "Blue"   },   /* Material Blue   */
    { {0}, "Orange" },   /* Material Orange  */
    { {0}, "Purple" },   /* Material Purple  */
};

void example_main(lv_obj_t *parent)
{
    /* ---- initialize colors at runtime (LVGL 9.2 lv_color_t has no .full) ---- */
    cards[0].color = lv_color_hex(0x4CAF50);
    cards[1].color = lv_color_hex(0x2196F3);
    cards[2].color = lv_color_hex(0xFF9800);
    cards[3].color = lv_color_hex(0xE040FB);

    /* ---- dark background ---- */
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* ---- flex container for 2x2 grid ---- */
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 20, 0);
    lv_obj_set_style_pad_gap(parent, 20, 0);

    /* ---- create four cards ---- */
    for (int i = 0; i < 4; i++) {
        lv_obj_t *card = example_card_create(parent, 360, 200,
                                             cards[i].color);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);

        lv_obj_t *lbl = example_label_create(card, cards[i].name,
                                             &lv_font_montserrat_24,
                                             lv_color_white());
        lv_obj_center(lbl);
    }
}
