/**
 * @file    main_example.c
 * @brief   Card Factory — Reusable tesaiot_card_create() function
 *
 * Builds styled containers matching the BENTO design system:
 * dark bg, rounded corners, accent border, shadow, title + divider.
 */

#include "example_common.h"

/* ── Reusable card factory ───────────────────────────────────────── */
static lv_obj_t *tesaiot_card_create(lv_obj_t *parent, const char *title,
                                      lv_color_t color, int w, int h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, color, 0);
    lv_obj_set_style_shadow_width(card, 8, 0);
    lv_obj_set_style_shadow_color(card, color, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Title label */
    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 4, 0);

    /* Divider line */
    lv_obj_t *line = lv_obj_create(card);
    lv_obj_set_size(line, w - 28, 1);
    lv_obj_set_style_bg_color(line, color, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_40, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 22);

    return card;
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I28 — Card Factory");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Card 1: IMU — top left */
    lv_obj_t *c1 = tesaiot_card_create(parent, "IMU (BMI270)",
        lv_color_hex(0x4CAF50), 340, 150);
    lv_obj_align(c1, LV_ALIGN_CENTER, -185, -55);
    lv_obj_t *v1 = lv_label_create(c1);
    lv_label_set_text(v1, "AX: 0  AY: 0  AZ: 981 mg");
    lv_obj_set_style_text_color(v1, lv_color_white(), 0);
    lv_obj_align(v1, LV_ALIGN_CENTER, 0, 10);

    /* Card 2: Temperature — top right */
    lv_obj_t *c2 = tesaiot_card_create(parent, "Temperature",
        lv_color_hex(0xFF9800), 340, 150);
    lv_obj_align(c2, LV_ALIGN_CENTER, 185, -55);
    lv_obj_t *v2 = lv_label_create(c2);
    lv_label_set_text(v2, "27.3 C");
    lv_obj_set_style_text_font(v2, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(v2, lv_color_hex(0xFF9800), 0);
    lv_obj_align(v2, LV_ALIGN_CENTER, 0, 10);

    /* Card 3: Compass — bottom left */
    lv_obj_t *c3 = tesaiot_card_create(parent, "Compass (BMM350)",
        lv_color_hex(0xE040FB), 340, 150);
    lv_obj_align(c3, LV_ALIGN_CENTER, -185, 115);
    lv_obj_t *v3 = lv_label_create(c3);
    lv_label_set_text(v3, "Heading: 142  (SE)");
    lv_obj_set_style_text_color(v3, lv_color_white(), 0);
    lv_obj_align(v3, LV_ALIGN_CENTER, 0, 10);

    /* Card 4: WiFi — bottom right */
    lv_obj_t *c4 = tesaiot_card_create(parent, "WiFi Status",
        lv_color_hex(0x00BCD4), 340, 150);
    lv_obj_align(c4, LV_ALIGN_CENTER, 185, 115);
    lv_obj_t *v4 = lv_label_create(c4);
    lv_label_set_text(v4, LV_SYMBOL_WIFI " Connected\n192.168.4.1");
    lv_obj_set_style_text_color(v4, lv_color_hex(0x00BCD4), 0);
    lv_obj_align(v4, LV_ALIGN_CENTER, 0, 10);
}
