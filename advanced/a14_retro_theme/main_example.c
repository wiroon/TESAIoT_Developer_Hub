/**
 * @file    main_example.c
 * @brief   Retro Game Boy Theme — 4-tone palette + CRT scanlines
 *
 * @description
 *   Game Boy 4-tone green palette, CRT scanline effect on canvas,
 *   "DOT MATRIX WITH STEREO SOUND" badge. Reusable theme module.
 *   Colors: lightest=#9BBC0F, light=#8BAC0F, dark=#306230, darkest=#0F380F
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "game_common.h"

/* ---------------------------------------------------------------------------
 * Game Boy Palette
 * --------------------------------------------------------------------------- */
#define GB_LIGHTEST     lv_color_hex(0x9BBC0F)
#define GB_LIGHT        lv_color_hex(0x8BAC0F)
#define GB_DARK         lv_color_hex(0x306230)
#define GB_DARKEST      lv_color_hex(0x0F380F)

#define SCREEN_W        400
#define SCREEN_H        300
#define SCANLINE_OPA    60      /* 0..255 */

/* ---------------------------------------------------------------------------
 * Create CRT scanline overlay on canvas
 * --------------------------------------------------------------------------- */
static void create_scanline_overlay(lv_obj_t *parent, uint8_t *buf,
                                     uint16_t w, uint16_t h)
{
    lv_obj_t *canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, buf, w, h, LV_COLOR_FORMAT_RGB565);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_opa(canvas, SCANLINE_OPA, 0);

    /* Fill transparent, then draw dark lines on even rows */
    lv_canvas_fill_bg(canvas, lv_color_hex(0x000000), LV_OPA_TRANSP);

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_draw_rect_dsc_t line_dsc;
    lv_draw_rect_dsc_init(&line_dsc);
    line_dsc.bg_color = lv_color_hex(0x000000);
    line_dsc.bg_opa   = LV_OPA_50;

    for (int y = 0; y < h; y += 2) {
        lv_area_t line = { 0, y, w - 1, y };
        lv_draw_rect(&layer, &line_dsc, &line);
    }

    lv_canvas_finish_layer(canvas, &layer);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    /* Set parent to darkest green */
    lv_obj_set_style_bg_color(parent, GB_DARKEST, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* === DOT MATRIX Badge (top) === */
    lv_obj_t *badge = lv_obj_create(parent);
    lv_obj_set_size(badge, 380, 32);
    lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(badge, GB_DARKEST, 0);
    lv_obj_set_style_border_color(badge, GB_DARK, 0);
    lv_obj_set_style_border_width(badge, 2, 0);
    lv_obj_set_style_border_side(badge, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_radius(badge, 4, 0);

    lv_obj_t *badge_text = lv_label_create(badge);
    lv_label_set_text(badge_text, "DOT MATRIX WITH STEREO SOUND");
    lv_obj_set_style_text_color(badge_text, GB_LIGHTEST, 0);
    lv_obj_set_style_text_letter_space(badge_text, 2, 0);
    lv_obj_center(badge_text);

    /* === Game Screen Area (simulated GB screen) === */
    lv_obj_t *screen_bezel = lv_obj_create(parent);
    lv_obj_set_size(screen_bezel, SCREEN_W + 16, SCREEN_H + 16);
    lv_obj_align(screen_bezel, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(screen_bezel, GB_DARK, 0);
    lv_obj_set_style_radius(screen_bezel, 8, 0);
    lv_obj_set_style_border_width(screen_bezel, 0, 0);

    lv_obj_t *screen = lv_obj_create(screen_bezel);
    lv_obj_set_size(screen, SCREEN_W, SCREEN_H);
    lv_obj_center(screen);
    lv_obj_set_style_bg_color(screen, GB_LIGHTEST, 0);
    lv_obj_set_style_radius(screen, 2, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 16, 0);

    /* --- Demo content inside screen --- */
    /* Title */
    lv_obj_t *game_title = lv_label_create(screen);
    lv_label_set_text(game_title, "RETRO THEME");
    lv_obj_set_style_text_font(game_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(game_title, GB_DARKEST, 0);
    lv_obj_align(game_title, LV_ALIGN_TOP_MID, 0, 0);

    /* Palette swatches */
    static const struct { lv_color_t color; const char *name; } palette[] = {
        { {0}, "Lightest #9BBC0F" },
        { {0}, "Light    #8BAC0F" },
        { {0}, "Dark     #306230" },
        { {0}, "Darkest  #0F380F" },
    };

    lv_color_t colors[] = { GB_LIGHTEST, GB_LIGHT, GB_DARK, GB_DARKEST };

    for (int i = 0; i < 4; i++) {
        int y_off = 40 + i * 40;

        /* Color swatch */
        lv_obj_t *swatch = lv_obj_create(screen);
        lv_obj_set_size(swatch, 40, 30);
        lv_obj_align(swatch, LV_ALIGN_TOP_LEFT, 0, y_off);
        lv_obj_set_style_bg_color(swatch, colors[i], 0);
        lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(swatch, GB_DARKEST, 0);
        lv_obj_set_style_border_width(swatch, 1, 0);
        lv_obj_set_style_radius(swatch, 4, 0);

        /* Label */
        lv_obj_t *lbl = lv_label_create(screen);
        lv_label_set_text(lbl, palette[i].name);
        lv_obj_set_style_text_color(lbl, GB_DARKEST, 0);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 50, y_off + 6);
    }

    /* Demo button in retro style */
    lv_obj_t *btn = lv_btn_create(screen);
    lv_obj_set_size(btn, 120, 40);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, 0, 60);
    lv_obj_set_style_bg_color(btn, GB_DARK, 0);
    lv_obj_set_style_radius(btn, 4, 0);
    lv_obj_set_style_border_color(btn, GB_DARKEST, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_shadow_width(btn, 4, 0);
    lv_obj_set_style_shadow_color(btn, GB_DARKEST, 0);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "START");
    lv_obj_set_style_text_color(btn_lbl, GB_LIGHTEST, 0);
    lv_obj_center(btn_lbl);

    /* Demo bar in retro style */
    lv_obj_t *bar = lv_bar_create(screen);
    lv_obj_set_size(bar, 120, 16);
    lv_obj_align(bar, LV_ALIGN_TOP_RIGHT, 0, 120);
    lv_obj_set_style_bg_color(bar, GB_LIGHT, 0);
    lv_obj_set_style_bg_color(bar, GB_DARK, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 2, 0);
    lv_obj_set_style_radius(bar, 2, LV_PART_INDICATOR);
    lv_bar_set_value(bar, 65, LV_ANIM_OFF);

    lv_obj_t *bar_lbl = lv_label_create(screen);
    lv_label_set_text(bar_lbl, "HP: 65/100");
    lv_obj_set_style_text_color(bar_lbl, GB_DARKEST, 0);
    lv_obj_align(bar_lbl, LV_ALIGN_TOP_RIGHT, 0, 140);

    /* Demo text */
    lv_obj_t *flavor = lv_label_create(screen);
    lv_label_set_text(flavor, "Press START to begin\nyour adventure!");
    lv_obj_set_style_text_color(flavor, GB_DARK, 0);
    lv_obj_align(flavor, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* === CRT Scanline Overlay === */
    static uint8_t scanline_buf[SCREEN_W * SCREEN_H * 2];
    create_scanline_overlay(parent, scanline_buf, SCREEN_W, SCREEN_H);
}
