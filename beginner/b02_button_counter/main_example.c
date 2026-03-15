/**
 * @file    main_example.c
 * @brief   Button Counter — Click counter with reset and styled display
 *
 * A button increments a counter on each click. A reset button clears
 * the count. The display shows both count and total clicks history.
 *
 * Functions:
 *   create_styled_button()   — Build a button with accent color and label
 *   reset_counter()          — Reset count and update display
 *   btn_event_cb()           — Event callback: increment or reset counter
 *   example_main()           — Entry point: compose counter UI
 */

#include "example_common.h"

static int32_t s_click_count = 0;
static lv_obj_t *s_lbl_count;

/* ── Create a styled button with accent color ────────────────────── */
static lv_obj_t *create_styled_button(lv_obj_t *parent, const char *text,
                                       lv_palette_t color, int w, int h,
                                       lv_align_t align, int x_ofs, int y_ofs)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_set_style_bg_color(btn, lv_palette_main(color), 0);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_shadow_width(btn, 6, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl);

    return btn;
}

/* ── Reset counter and update label ──────────────────────────────── */
static void reset_counter(void)
{
    s_click_count = 0;
    lv_label_set_text(s_lbl_count, "Count: 0");
}

/* ── Button event callback ───────────────────────────────────────── */
static void btn_event_cb(lv_event_t *e)
{
    intptr_t action = (intptr_t)lv_event_get_user_data(e);
    if (action == 0) {
        /* Increment */
        s_click_count++;
        lv_label_set_text_fmt(s_lbl_count, "Count: %"PRId32, s_click_count);
    } else {
        /* Reset */
        reset_counter();
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_click_count = 0;

    /* Counter display label */
    s_lbl_count = lv_label_create(parent);
    lv_label_set_text(s_lbl_count, "Count: 0");
    lv_obj_set_style_text_font(s_lbl_count, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_lbl_count, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(s_lbl_count, LV_ALIGN_CENTER, 0, -20);

    /* Click button */
    lv_obj_t *btn_inc = create_styled_button(parent, LV_SYMBOL_PLUS " Click Me!",
        LV_PALETTE_BLUE, 180, 60, LV_ALIGN_CENTER, -100, 50);
    lv_obj_add_event_cb(btn_inc, btn_event_cb, LV_EVENT_CLICKED, (void *)0);

    /* Reset button */
    lv_obj_t *btn_rst = create_styled_button(parent, LV_SYMBOL_REFRESH " Reset",
        LV_PALETTE_RED, 140, 60, LV_ALIGN_CENTER, 100, 50);
    lv_obj_add_event_cb(btn_rst, btn_event_cb, LV_EVENT_CLICKED, (void *)1);
}
