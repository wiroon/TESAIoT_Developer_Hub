/**
 * @file    main_example.c
 * @brief   Hello World Label — Display Hello BENTO with styled sections
 *
 * Demonstrates creating styled labels with helper functions, applying
 * a dark themed background, and organizing text in a vertical layout.
 *
 * Functions:
 *   setup_dark_background()  — Apply dark gradient to parent container
 *   create_styled_label()    — Reusable label factory with font + color + align
 *   create_divider()         — Horizontal divider line between sections
 *   example_main()           — Entry point: compose the hello screen
 */

#include "example_common.h"

/* ── Apply dark gradient background ───────────────────────────────── */
static void setup_dark_background(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_grad_color(parent, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_grad_dir(parent, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
}

/* ── Reusable styled label factory ────────────────────────────────── */
static lv_obj_t *create_styled_label(lv_obj_t *parent, const char *text,
                                      const lv_font_t *font, lv_color_t color,
                                      lv_align_t align, int y_offset)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_align(lbl, align, 0, y_offset);
    return lbl;
}

/* ── Horizontal divider line ──────────────────────────────────────── */
static lv_obj_t *create_divider(lv_obj_t *parent, int width, int y_offset,
                                 lv_color_t color)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_size(line, width, 2);
    lv_obj_set_style_bg_color(line, color, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_40, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 1, 0);
    lv_obj_align(line, LV_ALIGN_CENTER, 0, y_offset);
    return line;
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    setup_dark_background(parent);

    /* Title label at the top */
    create_styled_label(parent, "BENTO : : Make Anything.",
                        &lv_font_montserrat_16,
                        lv_palette_main(LV_PALETTE_BLUE),
                        LV_ALIGN_TOP_MID, 20);

    /* Divider below title */
    create_divider(parent, 300, -30, lv_palette_main(LV_PALETTE_BLUE));

    /* Main greeting at center */
    create_styled_label(parent, "Hello BENTO!",
                        &lv_font_montserrat_28,
                        lv_palette_main(LV_PALETTE_GREEN),
                        LV_ALIGN_CENTER, 0);

    /* Divider above footer */
    create_divider(parent, 400, 30, lv_palette_main(LV_PALETTE_GREY));

    /* Description label at the bottom */
    create_styled_label(parent, "PSoC Edge E84 Developer Hub — Beginner Example B01",
                        &lv_font_montserrat_14,
                        lv_palette_main(LV_PALETTE_GREY),
                        LV_ALIGN_BOTTOM_MID, -20);
}
