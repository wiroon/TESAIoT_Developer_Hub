/**
 * @file    main_example.c
 * @brief   Image Icon Display — Built-in LVGL symbols in a row
 *
 * Displays LVGL symbol icons (WiFi, Settings, etc.) in a horizontal
 * flex row with name labels below each icon.
 */

#include "example_common.h"

typedef struct {
    const char *symbol;
    const char *name;
    lv_palette_t color;
} icon_def_t;

static const icon_def_t icons[] = {
    { LV_SYMBOL_WIFI,      "WiFi",      LV_PALETTE_BLUE },
    { LV_SYMBOL_SETTINGS,  "Settings",  LV_PALETTE_GREY },
    { LV_SYMBOL_BLUETOOTH, "Bluetooth", LV_PALETTE_INDIGO },
    { LV_SYMBOL_BATTERY_3, "Battery",   LV_PALETTE_GREEN },
    { LV_SYMBOL_HOME,      "Home",      LV_PALETTE_ORANGE },
    { LV_SYMBOL_WARNING,   "Warning",   LV_PALETTE_RED },
};

#define ICON_COUNT (sizeof(icons) / sizeof(icons[0]))

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Built-in LVGL Symbols");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Icon row container */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 700, 120);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(row, 10, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    for (size_t i = 0; i < ICON_COUNT; i++) {
        /* Column for icon + name */
        lv_obj_t *col = lv_obj_create(row);
        lv_obj_set_size(col, 90, 90);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(col, 4, 0);

        /* Symbol icon */
        lv_obj_t *icon = lv_label_create(col);
        lv_label_set_text(icon, icons[i].symbol);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(icon, lv_palette_main(icons[i].color), 0);

        /* Name label */
        lv_obj_t *name = lv_label_create(col);
        lv_label_set_text(name, icons[i].name);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name, lv_palette_main(LV_PALETTE_GREY), 0);
    }
}
