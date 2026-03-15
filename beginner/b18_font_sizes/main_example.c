/**
 * @file    main_example.c
 * @brief   Font Sizes — Tappable font rows with preview panel
 *
 * Five Montserrat font sizes displayed as tappable rows. Selecting a row
 * updates a preview panel showing sample text at the chosen size.
 *
 * Functions:
 *   create_font_row()        — Build one clickable row with font sample
 *   update_preview()         — Refresh preview panel with selected font
 *   font_tap_cb()            — Event callback: handle font row selection
 *   example_main()           — Entry point: compose font list + preview
 */

#include "example_common.h"

typedef struct {
    const lv_font_t *font;
    const char      *label;
    int              size_px;
} font_entry_t;

static const font_entry_t s_fonts[] = {
    { &lv_font_montserrat_14, "Montserrat 14", 14 },
    { &lv_font_montserrat_16, "Montserrat 16", 16 },
    { &lv_font_montserrat_20, "Montserrat 20", 20 },
    { &lv_font_montserrat_24, "Montserrat 24", 24 },
    { &lv_font_montserrat_28, "Montserrat 28", 28 },
};
#define FONT_COUNT (sizeof(s_fonts) / sizeof(s_fonts[0]))

static lv_obj_t *s_preview_label;
static lv_obj_t *s_info_label;

/* ── Create a clickable font row ──────────────────────────────────── */
static lv_obj_t *create_font_row(lv_obj_t *parent, const font_entry_t *entry)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, 8, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text_fmt(lbl, "%s — BENTO", entry->label);
    lv_obj_set_style_text_font(lbl, entry->font, 0);
    lv_obj_center(lbl);

    return row;
}

/* ── Update preview with selected font ────────────────────────────── */
static void update_preview(const font_entry_t *entry)
{
    lv_obj_set_style_text_font(s_preview_label, entry->font, 0);
    lv_label_set_text(s_preview_label, "BENTO : : Make Anything.");
    lv_label_set_text_fmt(s_info_label, "Selected: %s (%dpx)",
                          entry->label, entry->size_px);
}

/* ── Font row tap callback ────────────────────────────────────────── */
static void font_tap_cb(lv_event_t *e)
{
    const font_entry_t *entry = (const font_entry_t *)lv_event_get_user_data(e);
    update_preview(entry);
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Font Size Reference");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Font list column */
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 500, 240);
    lv_obj_align(col, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(col, 10, 0);

    for (size_t i = 0; i < FONT_COUNT; i++) {
        lv_obj_t *row = create_font_row(col, &s_fonts[i]);
        lv_obj_add_event_cb(row, font_tap_cb, LV_EVENT_CLICKED,
                            (void *)&s_fonts[i]);
    }

    /* Preview panel */
    s_preview_label = lv_label_create(parent);
    lv_label_set_text(s_preview_label, "Tap a font to preview");
    lv_obj_set_style_text_font(s_preview_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_preview_label, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(s_preview_label, LV_ALIGN_BOTTOM_MID, 0, -40);

    s_info_label = lv_label_create(parent);
    lv_label_set_text(s_info_label, "");
    lv_obj_set_style_text_font(s_info_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_info_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_info_label, LV_ALIGN_BOTTOM_MID, 0, -15);
}
