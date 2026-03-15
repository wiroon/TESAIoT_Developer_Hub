/**
 * @file    main_example.c
 * @brief   Font Sizes — Montserrat at 14, 16, 20, 24, 28
 *
 * Displays sample text at five different font sizes in a vertical
 * list for visual reference of available Montserrat variants.
 */

#include "example_common.h"

typedef struct {
    const lv_font_t *font;
    const char      *label;
} font_entry_t;

static const font_entry_t fonts[] = {
    { &lv_font_montserrat_14, "Montserrat 14" },
    { &lv_font_montserrat_16, "Montserrat 16" },
    { &lv_font_montserrat_20, "Montserrat 20" },
    { &lv_font_montserrat_24, "Montserrat 24" },
    { &lv_font_montserrat_28, "Montserrat 28" },
};

#define FONT_COUNT (sizeof(fonts) / sizeof(fonts[0]))

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Font Size Reference");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Column container */
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 600, 320);
    lv_obj_align(col, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(col, 15, 0);

    for (size_t i = 0; i < FONT_COUNT; i++) {
        lv_obj_t *lbl = lv_label_create(col);
        lv_label_set_text_fmt(lbl, "%s — BENTO : : Make Anything.", fonts[i].label);
        lv_obj_set_style_text_font(lbl, fonts[i].font, 0);
    }
}
