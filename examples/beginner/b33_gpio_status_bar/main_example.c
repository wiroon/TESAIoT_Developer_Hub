/**
 * @file    main_example.c
 * @brief   Status Bar — LED indicators showing board state
 *
 * A horizontal status bar with three LED indicator dots.
 * Toggle buttons control each indicator on/off.
 */

#include "example_common.h"

#define NUM_INDICATORS 3

typedef struct {
    const char *name;
    lv_obj_t   *dot;
    lv_obj_t   *lbl;
    bool        active;
} indicator_t;

static indicator_t indicators[NUM_INDICATORS];

static void update_dot(indicator_t *ind)
{
    lv_obj_set_style_bg_color(ind->dot,
        ind->active ? lv_palette_main(LV_PALETTE_GREEN)
                    : lv_palette_main(LV_PALETTE_GREY), 0);
    lv_label_set_text_fmt(ind->lbl, "%s:%s",
                          ind->name, ind->active ? "ON" : "off");
}

static void toggle_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    indicator_t *ind = (indicator_t *)lv_event_get_user_data(e);
    ind->active = !ind->active;
    update_dot(ind);
}

static void create_status_item(lv_obj_t *bar, indicator_t *ind)
{
    lv_obj_t *item = lv_obj_create(bar);
    lv_obj_set_size(item, 110, 40);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(item, 4, 0);
    lv_obj_set_style_pad_column(item, 6, 0);
    lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(item, 0, 0);

    ind->dot = lv_obj_create(item);
    lv_obj_set_size(ind->dot, 16, 16);
    lv_obj_set_style_radius(ind->dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ind->dot, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_border_width(ind->dot, 0, 0);

    char buf[16];
    snprintf(buf, sizeof(buf), "%s:off", ind->name);
    ind->lbl = lv_label_create(item);
    lv_label_set_text(ind->lbl, buf);
    lv_obj_set_style_text_font(ind->lbl, &lv_font_montserrat_14, 0);
}

void example_main(lv_obj_t *parent)
{
    const char *names[] = { "PWR", "NET", "ACT" };
    for (int i = 0; i < NUM_INDICATORS; i++) {
        indicators[i].name = names[i];
        indicators[i].active = false;
    }

    /* Title */
    lv_obj_t *title = example_label_create(parent, "Status Bar Monitor",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Status bar */
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 600, 55);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_radius(bar, 8, 0);

    for (int i = 0; i < NUM_INDICATORS; i++) {
        create_status_item(bar, &indicators[i]);
    }

    /* Toggle buttons */
    for (int i = 0; i < NUM_INDICATORS; i++) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_size(btn, 140, 50);
        lv_obj_align(btn, LV_ALIGN_CENTER, (i - 1) * 170, 40);
        lv_obj_add_event_cb(btn, toggle_cb, LV_EVENT_CLICKED, &indicators[i]);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text_fmt(lbl, "Toggle %s", names[i]);
        lv_obj_center(lbl);
    }
}
