/**
 * @file    main_example.c
 * @brief   Color Palette — Tappable color grid with selection detail
 *
 * A 4x3 grid of LVGL palette colors. Tapping a swatch shows its name,
 * hex value, and light/dark variants in a detail panel below.
 *
 * Functions:
 *   create_color_swatch()    — Build one grid cell with color + label
 *   show_color_detail()      — Update detail panel with selected color info
 *   swatch_tap_cb()          — Event callback: handle swatch tap
 *   example_main()           — Entry point: compose grid + detail panel
 */

#include "example_common.h"

typedef struct {
    lv_palette_t palette;
    const char  *name;
} color_entry_t;

static const color_entry_t s_palette[] = {
    { LV_PALETTE_RED,         "Red" },
    { LV_PALETTE_PINK,        "Pink" },
    { LV_PALETTE_PURPLE,      "Purple" },
    { LV_PALETTE_DEEP_PURPLE, "DeepPurple" },
    { LV_PALETTE_INDIGO,      "Indigo" },
    { LV_PALETTE_BLUE,        "Blue" },
    { LV_PALETTE_CYAN,        "Cyan" },
    { LV_PALETTE_TEAL,        "Teal" },
    { LV_PALETTE_GREEN,       "Green" },
    { LV_PALETTE_LIME,        "Lime" },
    { LV_PALETTE_YELLOW,      "Yellow" },
    { LV_PALETTE_ORANGE,      "Orange" },
};
#define COLOR_COUNT 12

static lv_obj_t *s_detail_label;
static lv_obj_t *s_detail_swatch;

/* ── Create one color swatch in the grid ──────────────────────────── */
static lv_obj_t *create_color_swatch(lv_obj_t *grid, const color_entry_t *entry,
                                      int col, int row)
{
    lv_obj_t *cell = lv_obj_create(grid);
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, col, 1,
                         LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_bg_color(cell, lv_palette_main(entry->palette), 0);
    lv_obj_set_style_radius(cell, 8, 0);
    lv_obj_set_style_border_width(cell, 0, 0);
    lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl = lv_label_create(cell);
    lv_label_set_text(lbl, entry->name);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);

    return cell;
}

/* ── Update detail panel with selected color ──────────────────────── */
static void show_color_detail(const color_entry_t *entry)
{
    lv_label_set_text_fmt(s_detail_label, "Selected: %s", entry->name);
    lv_obj_set_style_text_color(s_detail_label,
        lv_palette_main(entry->palette), 0);

    lv_obj_set_style_bg_color(s_detail_swatch,
        lv_palette_main(entry->palette), 0);
}

/* ── Swatch tap event callback ────────────────────────────────────── */
static void swatch_tap_cb(lv_event_t *e)
{
    const color_entry_t *entry = (const color_entry_t *)lv_event_get_user_data(e);
    show_color_detail(entry);
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "LVGL Color Palette Reference");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Grid */
    static int32_t col_dsc[] = { 170, 170, 170, 170, LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { 80, 80, 80, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 740, 280);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid, 8, 0);
    lv_obj_set_style_pad_column(grid, 6, 0);
    lv_obj_set_style_pad_row(grid, 6, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    for (int i = 0; i < COLOR_COUNT; i++) {
        lv_obj_t *swatch = create_color_swatch(grid, &s_palette[i],
                                                i % 4, i / 4);
        lv_obj_add_event_cb(swatch, swatch_tap_cb, LV_EVENT_CLICKED,
                            (void *)&s_palette[i]);
    }

    /* Detail panel at bottom */
    s_detail_swatch = lv_obj_create(parent);
    lv_obj_set_size(s_detail_swatch, 30, 30);
    lv_obj_set_style_radius(s_detail_swatch, 6, 0);
    lv_obj_set_style_bg_color(s_detail_swatch, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_border_width(s_detail_swatch, 0, 0);
    lv_obj_align(s_detail_swatch, LV_ALIGN_BOTTOM_MID, -80, -15);

    s_detail_label = lv_label_create(parent);
    lv_label_set_text(s_detail_label, "Tap a color to select");
    lv_obj_set_style_text_font(s_detail_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_detail_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_detail_label, LV_ALIGN_BOTTOM_MID, 20, -18);
}
