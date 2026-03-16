/**
 * @file    main_example.c
 * @brief   Card Factory — Reusable tesaiot_card_create() with live values
 *
 * Builds styled containers matching the BENTO design system:
 * dark bg, rounded corners, accent border, shadow, title + divider.
 * Card values update every 2 seconds via timer.
 *
 * Functions:
 *   tesaiot_card_create()    — Build a dark styled card with title + divider
 *   create_card_value()      — Add a value label to a card with styling
 *   card_update_timer_cb()   — Timer callback: refresh card values
 *   example_main()           — Entry point: compose 4-card layout
 */

#include "example_common.h"

static lv_obj_t *s_val_labels[4];

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

/* ── Add a value label to a card ─────────────────────────────────── */
static lv_obj_t *create_card_value(lv_obj_t *card, const char *text,
                                    lv_color_t color, const lv_font_t *font)
{
    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 10);
    return lbl;
}

/* ── Timer callback: update card values ──────────────────────────── */
static void card_update_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    lv_label_set_text_fmt(s_val_labels[0], "AX: %d  AY: %d  AZ: %d mg",
        (int)lv_rand(-50, 50), (int)lv_rand(-50, 50), (int)(981 + lv_rand(-5, 5)));
    lv_label_set_text_fmt(s_val_labels[1], "%.1f C", 25.0f + (float)lv_rand(-20, 50) / 10.0f);
    lv_label_set_text_fmt(s_val_labels[2], "Heading: %d  (%s)",
        (int)lv_rand(0, 359), (lv_rand(0, 1) ? "NE" : "SW"));
    lv_label_set_text_fmt(s_val_labels[3], LV_SYMBOL_WIFI " %s\n192.168.4.1",
        (lv_rand(0, 5) > 0) ? "Connected" : "Reconnecting...");
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
    s_val_labels[0] = create_card_value(c1, "AX: 0  AY: 0  AZ: 981 mg",
        lv_color_white(), &lv_font_montserrat_14);

    /* Card 2: Temperature — top right */
    lv_obj_t *c2 = tesaiot_card_create(parent, "Temperature",
        lv_color_hex(0xFF9800), 340, 150);
    lv_obj_align(c2, LV_ALIGN_CENTER, 185, -55);
    s_val_labels[1] = create_card_value(c2, "27.3 C",
        lv_color_hex(0xFF9800), &lv_font_montserrat_28);

    /* Card 3: Compass — bottom left */
    lv_obj_t *c3 = tesaiot_card_create(parent, "Compass (BMM350)",
        lv_color_hex(0xE040FB), 340, 150);
    lv_obj_align(c3, LV_ALIGN_CENTER, -185, 115);
    s_val_labels[2] = create_card_value(c3, "Heading: 142  (SE)",
        lv_color_white(), &lv_font_montserrat_14);

    /* Card 4: WiFi — bottom right */
    lv_obj_t *c4 = tesaiot_card_create(parent, "WiFi Status",
        lv_color_hex(0x00BCD4), 340, 150);
    lv_obj_align(c4, LV_ALIGN_CENTER, 185, 115);
    s_val_labels[3] = create_card_value(c4, LV_SYMBOL_WIFI " Connected\n192.168.4.1",
        lv_color_hex(0x00BCD4), &lv_font_montserrat_14);

    /* Live update every 2 seconds */
    lv_timer_create(card_update_timer_cb, 2000, NULL);
}
