/**
 * @file    main_example.c
 * @brief   Home Card Grid — 6 tappable cards in 3x2 grid
 *
 * Mirrors the s_card_defs[] pattern from page_home.c.
 * Each card tap updates a detail panel with page info.
 *
 * Functions:
 *   create_grid_card()       — Build one styled card in the grid
 *   update_tap_info()        — Update detail label with tapped card info
 *   card_tap_cb()            — Event callback: handle card tap
 *   example_main()           — Entry point: compose grid + detail panel
 */

#include "example_common.h"

/* ── Card definition ─────────────────────────────────────────────── */
typedef struct {
    uint8_t     page_id;
    const char *icon;
    const char *name;
    uint32_t    color_hex;
} card_def_t;

static const card_def_t s_card_defs[] = {
    { 1, LV_SYMBOL_HOME,     "Dashboard",  0x4CAF50 },
    { 2, LV_SYMBOL_SETTINGS, "Controls",   0xFF9800 },
    { 3, LV_SYMBOL_GPS,      "Compass",    0xE040FB },
    { 4, LV_SYMBOL_REFRESH,  "Smart Watch",0x2196F3 },
    { 5, LV_SYMBOL_WIFI,     "WiFi",       0x00BCD4 },
    { 6, LV_SYMBOL_PLAY,     "Playground", 0xFFD740 },
};
#define CARD_COUNT  (sizeof(s_card_defs) / sizeof(s_card_defs[0]))

static lv_obj_t *s_lbl_tapped;

/* ── Create one styled card in the grid ──────────────────────────── */
static lv_obj_t *create_grid_card(lv_obj_t *grid, const card_def_t *d,
                                   int col, int row)
{
    lv_color_t color = lv_color_hex(d->color_hex);

    lv_obj_t *card = lv_obj_create(grid);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, col, 1,
                               LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, color, 0);
    lv_obj_set_style_shadow_width(card, 8, 0);
    lv_obj_set_style_shadow_color(card, color, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Icon */
    lv_obj_t *icon = lv_label_create(card);
    lv_label_set_text(icon, d->icon);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(icon, color, 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -12);

    /* Name */
    lv_obj_t *name = lv_label_create(card);
    lv_label_set_text(name, d->name);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(name, lv_color_white(), 0);
    lv_obj_align(name, LV_ALIGN_BOTTOM_MID, 0, -8);

    return card;
}

/* ── Update tap info label ───────────────────────────────────────── */
static void update_tap_info(const card_def_t *def)
{
    lv_label_set_text_fmt(s_lbl_tapped,
        "Tapped: %s (page_id=%d)\n"
        "In production: pm_navigate(&s_pm, PAGE_ID_%d)",
        def->name, def->page_id, def->page_id);
}

/* ── Card tap handler ────────────────────────────────────────────── */
static void card_tap_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    intptr_t id = (intptr_t)lv_event_get_user_data(e);
    update_tap_info(&s_card_defs[id]);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I25 — Home Card Grid");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Grid container */
    static int32_t col_dsc[] = { 220, 220, 220, LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { 130, 130, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 720, 300);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, -15);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_row(grid, 10, 0);
    lv_obj_set_style_pad_column(grid, 10, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    /* Create cards from definition array */
    for (uint32_t i = 0; i < CARD_COUNT; i++) {
        lv_obj_t *card = create_grid_card(grid, &s_card_defs[i], i % 3, i / 3);
        lv_obj_add_event_cb(card, card_tap_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }

    /* Tap feedback label */
    s_lbl_tapped = lv_label_create(parent);
    lv_label_set_text(s_lbl_tapped, "Tap a card to navigate...");
    lv_obj_set_style_text_font(s_lbl_tapped, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_tapped,
                                lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_tapped, LV_ALIGN_BOTTOM_MID, 0, -6);
}
