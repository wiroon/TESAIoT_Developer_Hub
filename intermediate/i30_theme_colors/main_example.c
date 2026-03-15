/**
 * @file    main_example.c
 * @brief   Theme Colors — Visual reference grid for UI_COLOR_* constants
 *
 * Displays all BENTO theme colors from tesaiot_ui_theme.h as swatches
 * with hex values and semantic names.
 */

#include "example_common.h"

/* ── Color definitions (from tesaiot_ui_theme.h) ─────────────────── */
typedef struct {
    uint32_t    hex;
    const char *name;
} theme_color_t;

static const theme_color_t s_colors[] = {
    { 0x4CAF50, "BMI270"      },
    { 0xFF9800, "DPS368"      },
    { 0x2196F3, "SHT40"       },
    { 0xE040FB, "BMM350"      },
    { 0xFF1744, "Radar"       },
    { 0xFFD740, "Joystick"    },
    { 0x00BCD4, "WiFi"        },
    { 0x142240, "Card BG"     },
    { 0x0A1628, "Panel BG"    },
    { 0x2a4060, "Border"      },
    { 0x66BB6A, "Success"     },
    { 0xFFA726, "Warning"     },
    { 0xEF5350, "Error"       },
    { 0x42A5F5, "Info"        },
    { 0xFFFFFF, "Text Light"  },
    { 0x90A4AE, "Text Muted"  },
};
#define COLOR_COUNT  (sizeof(s_colors) / sizeof(s_colors[0]))

void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I30 — BENTO Theme Colors");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Grid: 4 columns × 4 rows */
    static int32_t col_dsc[] = { 170, 170, 170, 170, LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { 72, 72, 72, 72, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 740, 340);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_row(grid, 6, 0);
    lv_obj_set_style_pad_column(grid, 6, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    for (uint32_t i = 0; i < COLOR_COUNT; i++) {
        const theme_color_t *c = &s_colors[i];
        lv_color_t col = lv_color_hex(c->hex);

        lv_obj_t *swatch = lv_obj_create(grid);
        lv_obj_set_grid_cell(swatch, LV_GRID_ALIGN_STRETCH, i % 4, 1,
                                     LV_GRID_ALIGN_STRETCH, i / 4, 1);
        lv_obj_set_style_bg_color(swatch, col, 0);
        lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(swatch, 8, 0);
        lv_obj_set_style_border_width(swatch, 1, 0);
        lv_obj_set_style_border_color(swatch, lv_color_white(), 0);
        lv_obj_clear_flag(swatch, LV_OBJ_FLAG_SCROLLABLE);

        /* Determine text color for readability */
        bool dark_bg = (c->hex < 0x808080) && (c->hex != 0x4CAF50) &&
                       (c->hex != 0x66BB6A);
        lv_color_t text_col = dark_bg ? lv_color_white() : lv_color_black();

        /* Name */
        lv_obj_t *lbl_name = lv_label_create(swatch);
        lv_label_set_text(lbl_name, c->name);
        lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_name, text_col, 0);
        lv_obj_align(lbl_name, LV_ALIGN_TOP_LEFT, 4, 4);

        /* Hex value */
        lv_obj_t *lbl_hex = lv_label_create(swatch);
        char buf[12];
        snprintf(buf, sizeof(buf), "#%06X", (unsigned)c->hex);
        lv_label_set_text(lbl_hex, buf);
        lv_obj_set_style_text_font(lbl_hex, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_hex, text_col, 0);
        lv_obj_align(lbl_hex, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
    }
}
