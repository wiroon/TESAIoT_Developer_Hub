/**
 * @file main_example.c
 * @brief B06 — Checkbox Options: Three checkboxes with a status label.
 *
 * Demonstrates LVGL checkbox widgets with event callbacks that update a
 * summary label showing which options are currently selected.
 */

#include "example_common.h"

static lv_obj_t *status_label;
static lv_obj_t *cb[3];

static const char *option_names[] = { "WiFi", "Bluetooth", "Sensors" };

static void update_status(void)
{
    static char buf[64];
    int pos = 0;
    bool any = false;

    pos += snprintf(buf + pos, sizeof(buf) - pos, "Active: ");
    for (int i = 0; i < 3; i++) {
        if (lv_obj_get_state(cb[i]) & LV_STATE_CHECKED) {
            if (any) pos += snprintf(buf + pos, sizeof(buf) - pos, ", ");
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%s", option_names[i]);
            any = true;
        }
    }
    if (!any) {
        snprintf(buf + pos, sizeof(buf) - pos, "None");
    }
    lv_label_set_text(status_label, buf);
}

static void cb_event(lv_event_t *e)
{
    (void)e;
    update_status();
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 20, 0);

    /* ---- title ---- */
    example_label_create(parent, "Checkbox Options",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- three checkboxes ---- */
    for (int i = 0; i < 3; i++) {
        cb[i] = lv_checkbox_create(parent);
        lv_checkbox_set_text(cb[i], option_names[i]);
        lv_obj_set_style_text_color(cb[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(cb[i], &lv_font_montserrat_20, 0);
        lv_obj_add_event_cb(cb[i], cb_event, LV_EVENT_VALUE_CHANGED, NULL);
    }

    /* ---- status label ---- */
    status_label = example_label_create(parent, "Active: None",
                                        &lv_font_montserrat_20,
                                        UI_COLOR_WARNING);
}
