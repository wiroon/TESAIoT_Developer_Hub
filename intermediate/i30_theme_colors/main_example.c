/**
 * @file    main_example.c
 * @brief   Theme Colors — Tappable color grid with detail panel
 *
 * Displays BENTO theme colors as swatches. Tapping a swatch shows
 * its hex value and semantic name in a detail panel.
 *
 * Functions:
 *   determine_text_color()   — Pick white/black text for readability on bg
 *   create_swatch()          — Build one grid swatch with name + hex labels
 *   swatch_tap_cb()          — Event callback: update detail panel on tap
 *   example_main()           — Entry point: compose grid + detail panel
 */

#include "example_common.h"

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

static lv_obj_t *s_detail_label;
static lv_obj_t *s_detail_swatch;

/* ── Determine readable text color for a given background ─────────── */
static lv_color_t determine_text_color(uint32_t bg_hex)
{
    /* Simple luminance check — dark bg gets white text */
    uint8_t r = (bg_hex >> 16) & 0xFF;
    uint8_t g = (bg_hex >>  8) & 0xFF;
    uint8_t b = (bg_hex >>  0) & 0xFF;
    int luminance = (r * 299 + g * 587 + b * 114) / 1000;
    return (luminance < 128) ? lv_color_white() : lv_color_black();
}

/* ── Create one swatch in the grid ────────────────────────────────── */
static lv_obj_t *create_swatch(lv_obj_t *grid, const theme_color_t *c,
                                int col, int row)
{
    lv_color_t bg = lv_color_hex(c->hex);
    lv_color_t text_col = determine_text_color(c->hex);

    lv_obj_t *swatch = lv_obj_create(grid);
    lv_obj_set_grid_cell(swatch, LV_GRID_ALIGN_STRETCH, col, 1,
                                 LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_bg_color(swatch, bg, 0);
    lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(swatch, 8, 0);
    lv_obj_set_style_border_width(swatch, 1, 0);
    lv_obj_set_style_border_color(swatch, lv_color_white(), 0);
    lv_obj_clear_flag(swatch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(swatch, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl_name = lv_label_create(swatch);
    lv_label_set_text(lbl_name, c->name);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_name, text_col, 0);
    lv_obj_align(lbl_name, LV_ALIGN_TOP_LEFT, 4, 4);

    char buf[12];
    snprintf(buf, sizeof(buf), "#%06X", (unsigned)c->hex);
    lv_obj_t *lbl_hex = lv_label_create(swatch);
    lv_label_set_text(lbl_hex, buf);
    lv_obj_set_style_text_font(lbl_hex, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_hex, text_col, 0);
    lv_obj_align(lbl_hex, LV_ALIGN_BOTTOM_RIGHT, -4, -4);

    return swatch;
}

/* ── Swatch tap callback ──────────────────────────────────────────── */
static void swatch_tap_cb(lv_event_t *e)
{
    const theme_color_t *c = (const theme_color_t *)lv_event_get_user_data(e);
    lv_label_set_text_fmt(s_detail_label, "%s  #%06X", c->name, (unsigned)c->hex);
    lv_obj_set_style_text_color(s_detail_label, lv_color_hex(c->hex), 0);
    lv_obj_set_style_bg_color(s_detail_swatch, lv_color_hex(c->hex), 0);
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I30 — BENTO Theme Colors");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Grid: 4 columns × 4 rows */
    static int32_t col_dsc[] = { 170, 170, 170, 170, LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { 68, 68, 68, 68, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 740, 310);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_row(grid, 6, 0);
    lv_obj_set_style_pad_column(grid, 6, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    for (uint32_t i = 0; i < COLOR_COUNT; i++) {
        lv_obj_t *swatch = create_swatch(grid, &s_colors[i], i % 4, i / 4);
        lv_obj_add_event_cb(swatch, swatch_tap_cb, LV_EVENT_CLICKED,
                            (void *)&s_colors[i]);
    }

    /* Detail panel */
    s_detail_swatch = lv_obj_create(parent);
    lv_obj_set_size(s_detail_swatch, 24, 24);
    lv_obj_set_style_radius(s_detail_swatch, 4, 0);
    lv_obj_set_style_bg_color(s_detail_swatch, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_border_width(s_detail_swatch, 0, 0);
    lv_obj_align(s_detail_swatch, LV_ALIGN_BOTTOM_MID, -90, -12);

    s_detail_label = lv_label_create(parent);
    lv_label_set_text(s_detail_label, "Tap a color swatch");
    lv_obj_set_style_text_font(s_detail_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_detail_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_detail_label, LV_ALIGN_BOTTOM_MID, 20, -14);
}
