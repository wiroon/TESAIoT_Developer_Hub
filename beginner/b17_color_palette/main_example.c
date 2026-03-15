/**
 * @file    main_example.c
 * @brief   Color Palette — 4x3 grid of LVGL palette colors
 *
 * Displays 12 LVGL palette colors in a grid with name labels.
 * Useful as a visual color reference for UI design.
 */

#include "example_common.h"

typedef struct {
    lv_palette_t palette;
    const char  *name;
} color_entry_t;

static const color_entry_t palette_entries[] = {
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

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "LVGL Color Palette Reference");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Grid */
    static int32_t col_dsc[] = { 170, 170, 170, 170, LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { 100, 100, 100, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 740, 340);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 12);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid, 8, 0);
    lv_obj_set_style_pad_column(grid, 6, 0);
    lv_obj_set_style_pad_row(grid, 6, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    for (int i = 0; i < COLOR_COUNT; i++) {
        int col = i % 4;
        int row = i / 4;

        lv_obj_t *cell = lv_obj_create(grid);
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, col, 1,
                             LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_style_bg_color(cell, lv_palette_main(palette_entries[i].palette), 0);
        lv_obj_set_style_radius(cell, 8, 0);
        lv_obj_set_style_border_width(cell, 0, 0);

        lv_obj_t *lbl = lv_label_create(cell);
        lv_label_set_text(lbl, palette_entries[i].name);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(lbl);
    }
}
