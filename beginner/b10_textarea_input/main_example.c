/**
 * @file    main_example.c
 * @brief   Textarea Input — Text entry with validation and character count
 *
 * A textarea with on-screen keyboard, live character count display,
 * and text-changed callback that validates input length.
 *
 * Functions:
 *   create_input_field()     — Build textarea with placeholder and max length
 *   update_char_count()      — Refresh the character count label (N/32)
 *   textarea_event_cb()      — Callback: update count + validate on text change
 *   example_main()           — Entry point: compose input form with keyboard
 */

#include "example_common.h"

static lv_obj_t *s_count_label;
static lv_obj_t *s_status_label;

#define MAX_INPUT_LEN 32

/* ── Create textarea input field ──────────────────────────────────── */
static lv_obj_t *create_input_field(lv_obj_t *parent)
{
    lv_obj_t *ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, 380, 50);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_placeholder_text(ta, "Type here...");
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, MAX_INPUT_LEN);
    return ta;
}

/* ── Update character count display ───────────────────────────────── */
static void update_char_count(const char *text)
{
    int len = (int)strlen(text);
    lv_label_set_text_fmt(s_count_label, "%d / %d", len, MAX_INPUT_LEN);

    if (len >= MAX_INPUT_LEN) {
        lv_obj_set_style_text_color(s_count_label,
            lv_palette_main(LV_PALETTE_RED), 0);
    } else {
        lv_obj_set_style_text_color(s_count_label,
            lv_palette_main(LV_PALETTE_GREY), 0);
    }
}

/* ── Textarea event callback ──────────────────────────────────────── */
static void textarea_event_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    const char *text = lv_textarea_get_text(ta);

    update_char_count(text);

    int len = (int)strlen(text);
    if (len == 0) {
        lv_label_set_text(s_status_label, "Enter a device name");
        lv_obj_set_style_text_color(s_status_label,
            lv_palette_main(LV_PALETTE_GREY), 0);
    } else if (len < 3) {
        lv_label_set_text(s_status_label, "Too short (min 3 chars)");
        lv_obj_set_style_text_color(s_status_label,
            lv_palette_main(LV_PALETTE_ORANGE), 0);
    } else {
        lv_label_set_text_fmt(s_status_label, LV_SYMBOL_OK " \"%s\" is valid", text);
        lv_obj_set_style_text_color(s_status_label,
            lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Enter Device Name:");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Input field */
    lv_obj_t *ta = create_input_field(parent);
    lv_obj_add_event_cb(ta, textarea_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Character count */
    s_count_label = lv_label_create(parent);
    lv_label_set_text_fmt(s_count_label, "0 / %d", MAX_INPUT_LEN);
    lv_obj_set_style_text_font(s_count_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_count_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_count_label, LV_ALIGN_TOP_RIGHT, -60, 50);

    /* Validation status */
    s_status_label = lv_label_create(parent);
    lv_label_set_text(s_status_label, "Enter a device name");
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_status_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_status_label, LV_ALIGN_TOP_MID, 0, 95);

    /* On-screen keyboard */
    lv_obj_t *kb = lv_keyboard_create(parent);
    lv_obj_set_size(kb, LV_PCT(100), 180);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);
}
