/**
 * @file    main_example.c
 * @brief   Flex Row Layout — Navigation bar with active state tracking
 *
 * Four navigation buttons in a flex row. Tapping a button highlights it
 * as active and dims the others, demonstrating event callbacks with state.
 *
 * Functions:
 *   create_nav_button()      — Build a styled navigation button
 *   highlight_active()       — Set one button active, dim the rest
 *   nav_btn_cb()             — Event callback: switch active button
 *   example_main()           — Entry point: compose navigation bar
 */

#include "example_common.h"

typedef struct {
    const char   *label;
    lv_palette_t  color;
} nav_item_t;

static const nav_item_t s_items[] = {
    { "Home",     LV_PALETTE_BLUE },
    { "Sensors",  LV_PALETTE_GREEN },
    { "WiFi",     LV_PALETTE_ORANGE },
    { "Settings", LV_PALETTE_GREY },
};
#define NAV_COUNT 4

static lv_obj_t *s_buttons[NAV_COUNT];
static lv_obj_t *s_status_label;

/* ── Create a styled navigation button ────────────────────────────── */
static lv_obj_t *create_nav_button(lv_obj_t *parent, const nav_item_t *item)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 140, 50);
    lv_obj_set_style_bg_color(btn, lv_palette_main(item->color), 0);
    lv_obj_set_style_radius(btn, 8, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, item->label);
    lv_obj_center(lbl);

    return btn;
}

/* ── Highlight active button, dim others ──────────────────────────── */
static void highlight_active(int active_idx)
{
    for (int i = 0; i < NAV_COUNT; i++) {
        if (i == active_idx) {
            lv_obj_set_style_bg_opa(s_buttons[i], LV_OPA_COVER, 0);
            lv_obj_set_style_shadow_width(s_buttons[i], 12, 0);
            lv_obj_set_style_shadow_color(s_buttons[i],
                lv_palette_main(s_items[i].color), 0);
            lv_obj_set_style_shadow_opa(s_buttons[i], LV_OPA_40, 0);
        } else {
            lv_obj_set_style_bg_opa(s_buttons[i], LV_OPA_50, 0);
            lv_obj_set_style_shadow_width(s_buttons[i], 0, 0);
        }
    }
    lv_label_set_text_fmt(s_status_label, "Active: %s", s_items[active_idx].label);
    lv_obj_set_style_text_color(s_status_label,
        lv_palette_main(s_items[active_idx].color), 0);
}

/* ── Navigation button tap callback ───────────────────────────────── */
static void nav_btn_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    highlight_active(idx);
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Navigation Bar");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Flex row container */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 700, 80);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 15, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    for (int i = 0; i < NAV_COUNT; i++) {
        s_buttons[i] = create_nav_button(row, &s_items[i]);
        lv_obj_add_event_cb(s_buttons[i], nav_btn_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }

    /* Status label */
    s_status_label = lv_label_create(parent);
    lv_label_set_text(s_status_label, "Tap a button to navigate");
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_status_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_status_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    /* Set first button active by default */
    highlight_active(0);
}
