/**
 * @file    main_example.c
 * @brief   Card Container — Material Design cards with shadow and radius
 *
 * Three cards displayed in a row, each with shadow, rounded corners,
 * and distinct accent colors. Contains title, value, and subtitle.
 */

#include "example_common.h"

typedef struct {
    const char     *title;
    const char     *value;
    const char     *subtitle;
    lv_palette_t    accent;
} card_def_t;

static const card_def_t cards[] = {
    { "Temperature",  "25.3 \xC2\xB0""C",    "DPS368 Sensor",   LV_PALETTE_ORANGE },
    { "Humidity",     "45.7 %",               "SHT40 Sensor",    LV_PALETTE_BLUE },
    { "Pressure",     "1013 hPa",             "DPS368 Sensor",   LV_PALETTE_GREEN },
};

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Sensor Dashboard");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Card row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 740, 280);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);

    for (int i = 0; i < 3; i++) {
        /* Card */
        lv_obj_t *card = lv_obj_create(row);
        lv_obj_set_size(card, 210, 220);
        lv_obj_set_style_radius(card, 12, 0);
        lv_obj_set_style_shadow_width(card, 15, 0);
        lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
        lv_obj_set_style_shadow_offset_y(card, 5, 0);
        lv_obj_set_style_border_color(card, lv_palette_main(cards[i].accent), 0);
        lv_obj_set_style_border_width(card, 2, 0);
        lv_obj_set_style_border_side(card, LV_BORDER_SIDE_TOP, 0);
        lv_obj_set_style_pad_all(card, 15, 0);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(card, 10, 0);

        /* Card title */
        lv_obj_t *lbl_title = lv_label_create(card);
        lv_label_set_text(lbl_title, cards[i].title);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl_title, lv_palette_main(LV_PALETTE_GREY), 0);

        /* Card value */
        lv_obj_t *lbl_value = lv_label_create(card);
        lv_label_set_text(lbl_value, cards[i].value);
        lv_obj_set_style_text_font(lbl_value, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(lbl_value, lv_palette_main(cards[i].accent), 0);

        /* Card subtitle */
        lv_obj_t *lbl_sub = lv_label_create(card);
        lv_label_set_text(lbl_sub, cards[i].subtitle);
        lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_sub, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
    }
}
