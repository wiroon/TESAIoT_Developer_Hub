/**
 * @file    main_example.c
 * @brief   Flex Column Layout — Tappable menu with selection tracking
 *
 * Five menu items stacked vertically using LV_FLEX_FLOW_COLUMN.
 * Tapping an item highlights it and shows a detail panel.
 *
 * Functions:
 *   create_menu_item()       — Build a styled, clickable menu row
 *   update_selection()       — Highlight selected item, dim others
 *   menu_item_cb()           — Event callback: handle menu item tap
 *   example_main()           — Entry point: compose menu with detail panel
 */

#include "example_common.h"

typedef struct {
    const char   *label;
    const char   *detail;
    lv_palette_t  color;
} menu_entry_t;

static const menu_entry_t s_entries[] = {
    { "Dashboard",   "View sensor overview and charts",   LV_PALETTE_BLUE },
    { "Sensors",     "BMI270, DPS368, SHT40 readings",    LV_PALETTE_GREEN },
    { "WiFi Config", "Connect to access point",           LV_PALETTE_ORANGE },
    { "Playground",  "Run MicroPython scripts",           LV_PALETTE_PURPLE },
    { "Settings",    "Board configuration and system",    LV_PALETTE_GREY },
};
#define MENU_COUNT 5

static lv_obj_t *s_items[MENU_COUNT];
static lv_obj_t *s_detail_label;

/* ── Create a styled menu item row ────────────────────────────────── */
static lv_obj_t *create_menu_item(lv_obj_t *parent, const menu_entry_t *entry)
{
    lv_obj_t *item = lv_obj_create(parent);
    lv_obj_set_width(item, LV_PCT(100));
    lv_obj_set_flex_grow(item, 1);
    lv_obj_set_style_bg_color(item, lv_palette_lighten(entry->color, 4), 0);
    lv_obj_set_style_border_color(item, lv_palette_main(entry->color), 0);
    lv_obj_set_style_border_width(item, 2, 0);
    lv_obj_set_style_radius(item, 8, 0);
    lv_obj_set_style_pad_all(item, 8, 0);
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl = lv_label_create(item);
    lv_label_set_text_fmt(lbl, LV_SYMBOL_RIGHT "  %s", entry->label);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl);

    return item;
}

/* ── Highlight selected item, dim others ──────────────────────────── */
static void update_selection(int selected)
{
    for (int i = 0; i < MENU_COUNT; i++) {
        if (i == selected) {
            lv_obj_set_style_bg_opa(s_items[i], LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(s_items[i], 3, 0);
        } else {
            lv_obj_set_style_bg_opa(s_items[i], LV_OPA_50, 0);
            lv_obj_set_style_border_width(s_items[i], 1, 0);
        }
    }
    lv_label_set_text_fmt(s_detail_label, "%s  %s",
                          s_entries[selected].label, s_entries[selected].detail);
    lv_obj_set_style_text_color(s_detail_label,
        lv_palette_main(s_entries[selected].color), 0);
}

/* ── Menu item tap callback ───────────────────────────────────────── */
static void menu_item_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    update_selection(idx);
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Menu List");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Column container */
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 400, 280);
    lv_obj_align(col, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 8, 0);
    lv_obj_set_style_pad_all(col, 10, 0);

    for (int i = 0; i < MENU_COUNT; i++) {
        s_items[i] = create_menu_item(col, &s_entries[i]);
        lv_obj_add_event_cb(s_items[i], menu_item_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }

    /* Detail panel at bottom */
    s_detail_label = lv_label_create(parent);
    lv_label_set_text(s_detail_label, "Tap a menu item for details");
    lv_obj_set_style_text_font(s_detail_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_detail_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_detail_label, LV_ALIGN_BOTTOM_MID, 0, -15);
}
