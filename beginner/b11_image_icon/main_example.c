/**
 * @file    main_example.c
 * @brief   Image Icon Display — Tappable LVGL symbol buttons with status
 *
 * Displays LVGL symbol icons as tappable buttons. Tapping an icon
 * updates a status bar showing which icon was selected.
 *
 * Functions:
 *   create_icon_button()     — Build a styled icon column with symbol + name
 *   update_status()          — Update the status bar with selected icon info
 *   icon_tap_cb()            — Event callback: handle icon button tap
 *   example_main()           — Entry point: compose icon grid with status bar
 */

#include "example_common.h"

typedef struct {
    const char   *symbol;
    const char   *name;
    lv_palette_t  color;
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

static lv_obj_t *s_status_label;

/* ── Create a tappable icon button ────────────────────────────────── */
static lv_obj_t *create_icon_button(lv_obj_t *parent, const icon_def_t *def)
{
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 90, 90);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_width(col, 1, 0);
    lv_obj_set_style_border_color(col, lv_palette_lighten(def->color, 3), 0);
    lv_obj_set_style_border_opa(col, LV_OPA_50, 0);
    lv_obj_set_style_radius(col, 12, 0);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(col, 4, 0);
    lv_obj_add_flag(col, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *icon = lv_label_create(col);
    lv_label_set_text(icon, def->symbol);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(icon, lv_palette_main(def->color), 0);

    lv_obj_t *name = lv_label_create(col);
    lv_label_set_text(name, def->name);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(name, lv_palette_main(LV_PALETTE_GREY), 0);

    return col;
}

/* ── Update status bar with selected icon ─────────────────────────── */
static void update_status(const icon_def_t *def)
{
    lv_label_set_text_fmt(s_status_label, "%s  Selected: %s", def->symbol, def->name);
    lv_obj_set_style_text_color(s_status_label, lv_palette_main(def->color), 0);
}

/* ── Icon tap event callback ──────────────────────────────────────── */
static void icon_tap_cb(lv_event_t *e)
{
    const icon_def_t *def = (const icon_def_t *)lv_event_get_user_data(e);
    update_status(def);
}

/* ── Entry point ──────────────────────────────────────────────────── */
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
    lv_obj_align(row, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(row, 10, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    for (size_t i = 0; i < ICON_COUNT; i++) {
        lv_obj_t *btn = create_icon_button(row, &icons[i]);
        lv_obj_add_event_cb(btn, icon_tap_cb, LV_EVENT_CLICKED,
                            (void *)&icons[i]);
    }

    /* Status bar at bottom */
    s_status_label = lv_label_create(parent);
    lv_label_set_text(s_status_label, "Tap an icon to select");
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_status_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_status_label, LV_ALIGN_BOTTOM_MID, 0, -20);
}
