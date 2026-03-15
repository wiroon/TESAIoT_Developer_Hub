/**
 * @file    main_example.c
 * @brief   Screen Background — Gradient with badge and animated uptime
 *
 * Sets a vertical gradient background with styled text overlays,
 * a pill-shaped version badge, and a timer-driven uptime counter.
 *
 * Functions:
 *   setup_gradient()         — Apply vertical gradient to container
 *   create_pill_badge()      — Build a pill-shaped badge with centered text
 *   uptime_timer_cb()        — Timer callback: update uptime display
 *   example_main()           — Entry point: compose splash screen with uptime
 */

#include "example_common.h"

static lv_obj_t *s_uptime_label;
static uint32_t  s_seconds;

/* ── Apply vertical gradient background ───────────────────────────── */
static void setup_gradient(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_make(10, 20, 60), 0);
    lv_obj_set_style_bg_grad_color(parent, lv_color_make(40, 100, 180), 0);
    lv_obj_set_style_bg_grad_dir(parent, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
}

/* ── Create a pill-shaped badge ───────────────────────────────────── */
static lv_obj_t *create_pill_badge(lv_obj_t *parent, const char *text,
                                    int w, int h, int y_offset)
{
    lv_obj_t *badge = lv_obj_create(parent);
    lv_obj_set_size(badge, w, h);
    lv_obj_set_style_bg_color(badge, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_20, 0);
    lv_obj_set_style_radius(badge, h / 2, 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_border_color(badge, lv_color_white(), 0);
    lv_obj_set_style_border_opa(badge, LV_OPA_40, 0);
    lv_obj_align(badge, LV_ALIGN_CENTER, 0, y_offset);

    lv_obj_t *lbl = lv_label_create(badge);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);

    return lbl;
}

/* ── Timer callback: update uptime ────────────────────────────────── */
static void uptime_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_seconds++;
    uint32_t m = s_seconds / 60;
    uint32_t s = s_seconds % 60;
    lv_label_set_text_fmt(s_uptime_label, "Uptime: %02d:%02d", m, s);
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_seconds = 0;
    setup_gradient(parent);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "BENTO : : Make Anything.");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);

    /* Subtitle */
    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "PSoC Edge E84 Developer Hub");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sub, lv_color_make(180, 200, 255), 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 10);

    /* Version badge */
    create_pill_badge(parent, "v2.0.0", 160, 40, 60);

    /* Uptime badge */
    s_uptime_label = create_pill_badge(parent, "Uptime: 00:00", 200, 36, 110);

    /* Update uptime every second */
    lv_timer_create(uptime_timer_cb, 1000, NULL);
}
