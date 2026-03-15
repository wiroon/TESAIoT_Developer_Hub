/**
 * @file    main_example.c
 * @brief   Custom Style — Reusable lv_style_t applied to buttons
 *
 * Two style definitions (primary, secondary) are created and applied
 * to multiple buttons, demonstrating style reuse and pressed states.
 *
 * Functions:
 *   init_styles()            — Initialize primary, secondary, and pressed styles
 *   create_styled_button()   — Build a button with a given style and label
 *   style_tap_cb()           — Event callback: show which style was tapped
 *   example_main()           — Entry point: compose styled button grid
 */

#include "example_common.h"

static lv_style_t style_primary;
static lv_style_t style_secondary;
static lv_style_t style_pressed;
static lv_obj_t  *s_lbl_info;

/* ── Initialize all style definitions ────────────────────────────── */
static void init_styles(void)
{
    /* Primary style — blue, rounded, shadow */
    lv_style_init(&style_primary);
    lv_style_set_bg_color(&style_primary, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_opa(&style_primary, LV_OPA_COVER);
    lv_style_set_radius(&style_primary, 12);
    lv_style_set_shadow_width(&style_primary, 10);
    lv_style_set_shadow_opa(&style_primary, LV_OPA_30);
    lv_style_set_shadow_offset_y(&style_primary, 4);
    lv_style_set_text_color(&style_primary, lv_color_white());
    lv_style_set_pad_all(&style_primary, 15);

    /* Secondary style — outline, no fill */
    lv_style_init(&style_secondary);
    lv_style_set_bg_opa(&style_secondary, LV_OPA_TRANSP);
    lv_style_set_border_color(&style_secondary, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_border_width(&style_secondary, 2);
    lv_style_set_radius(&style_secondary, 12);
    lv_style_set_text_color(&style_secondary, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_pad_all(&style_secondary, 15);

    /* Pressed state */
    lv_style_init(&style_pressed);
    lv_style_set_bg_opa(&style_pressed, LV_OPA_70);
    lv_style_set_translate_y(&style_pressed, 2);
}

/* ── Create a styled button with label ───────────────────────────── */
static lv_obj_t *create_styled_button(lv_obj_t *parent, const char *text,
                                       lv_style_t *style, int w, int h,
                                       lv_align_t align, int x_ofs, int y_ofs)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, style, 0);
    lv_obj_add_style(btn, &style_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(btn, w, h);
    lv_obj_align(btn, align, x_ofs, y_ofs);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);

    return btn;
}

/* ── Button tap callback — show style info ───────────────────────── */
static void style_tap_cb(lv_event_t *e)
{
    const char *name = (const char *)lv_event_get_user_data(e);
    lv_label_set_text_fmt(s_lbl_info, "Tapped: %s", name);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    init_styles();

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Custom Styles");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Primary buttons */
    for (int i = 0; i < 2; i++) {
        char text[16];
        snprintf(text, sizeof(text), "Primary %d", i + 1);
        lv_obj_t *btn = create_styled_button(parent, text,
            &style_primary, 200, 55, LV_ALIGN_CENTER, -130, -30 + i * 70);
        lv_obj_add_event_cb(btn, style_tap_cb, LV_EVENT_CLICKED,
                            (void *)(i == 0 ? "Primary 1" : "Primary 2"));
    }

    /* Secondary buttons */
    for (int i = 0; i < 2; i++) {
        char text[16];
        snprintf(text, sizeof(text), "Secondary %d", i + 1);
        lv_obj_t *btn = create_styled_button(parent, text,
            &style_secondary, 200, 55, LV_ALIGN_CENTER, 130, -30 + i * 70);
        lv_obj_add_event_cb(btn, style_tap_cb, LV_EVENT_CLICKED,
                            (void *)(i == 0 ? "Secondary 1" : "Secondary 2"));
    }

    /* Info label */
    s_lbl_info = lv_label_create(parent);
    lv_label_set_text(s_lbl_info, "Tap a button to see its style");
    lv_obj_set_style_text_font(s_lbl_info, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_info, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_info, LV_ALIGN_BOTTOM_MID, 0, -10);
}
