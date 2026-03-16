/**
 * @file    main_example.c
 * @brief   Multi-Page App — Home → Settings / Info with navigation stack
 *
 * Three pages: Home (cards), Settings (slider), Info (runtime stats).
 * Self-contained navigation within the example parent widget.
 */

#include "example_common.h"

/* ── Persistent state (survives navigation) ──────────────────────── */
static int32_t  s_brightness = 70;
static uint32_t s_nav_count;

/* ── Page management ─────────────────────────────────────────────── */
static lv_obj_t *s_root;
static lv_obj_t *s_page_container;
static lv_timer_t *s_info_timer;

enum { PAGE_HOME = 0, PAGE_SETTINGS, PAGE_INFO };

static void show_page(int page);

static void clear_page(void)
{
    if (s_info_timer) { lv_timer_delete(s_info_timer); s_info_timer = NULL; }
    if (s_page_container) { lv_obj_delete(s_page_container); s_page_container = NULL; }
}

/* ── Navigation helpers ──────────────────────────────────────────── */
static void nav_to_settings(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { clear_page(); show_page(PAGE_SETTINGS); }
}
static void nav_to_info(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { clear_page(); show_page(PAGE_INFO); }
}
static void nav_back_home(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { clear_page(); show_page(PAGE_HOME); }
}

/* ── Slider event ────────────────────────────────────────────────── */
static lv_obj_t *s_lbl_bright;
static void slider_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    s_brightness = lv_slider_get_value(lv_event_get_target(e));
    lv_label_set_text_fmt(s_lbl_bright, "Brightness: %d %%", (int)s_brightness);
}

/* ── Info timer ──────────────────────────────────────────────────── */
static lv_obj_t *s_lbl_uptime, *s_lbl_heap, *s_lbl_navs;
static void info_timer_cb(lv_timer_t *t) {
    (void)t;
    uint32_t sec = xTaskGetTickCount() / configTICK_RATE_HZ;
    lv_label_set_text_fmt(s_lbl_uptime, "Uptime: %u s", (unsigned)sec);
    lv_label_set_text_fmt(s_lbl_heap,   "Free heap: %u bytes",
                          (unsigned)xPortGetFreeHeapSize());
    lv_label_set_text_fmt(s_lbl_navs,   "Navigations: %u", (unsigned)s_nav_count);
}

/* ── Show page by ID ─────────────────────────────────────────────── */
static void show_page(int page)
{
    s_nav_count++;

    s_page_container = lv_obj_create(s_root);
    lv_obj_set_size(s_page_container, 700, 320);
    lv_obj_align(s_page_container, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_bg_color(s_page_container, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(s_page_container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_page_container, 12, 0);
    lv_obj_set_style_border_width(s_page_container, 1, 0);
    lv_obj_clear_flag(s_page_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *p = s_page_container;

    switch (page) {
    case PAGE_HOME: {
        lv_obj_t *t = lv_label_create(p);
        lv_label_set_text(t, LV_SYMBOL_HOME "  Home");
        lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(t, lv_color_white(), 0);
        lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 20);

        /* Settings card */
        lv_obj_t *c1 = lv_button_create(p);
        lv_obj_set_size(c1, 280, 100);
        lv_obj_align(c1, LV_ALIGN_CENTER, -160, 20);
        lv_obj_add_event_cb(c1, nav_to_settings, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_color(c1, lv_color_hex(0x1a3660), 0);
        lv_obj_t *l1 = lv_label_create(c1);
        lv_label_set_text(l1, LV_SYMBOL_SETTINGS "\nSettings");
        lv_obj_set_style_text_align(l1, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(l1);

        /* Info card */
        lv_obj_t *c2 = lv_button_create(p);
        lv_obj_set_size(c2, 280, 100);
        lv_obj_align(c2, LV_ALIGN_CENTER, 160, 20);
        lv_obj_add_event_cb(c2, nav_to_info, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_color(c2, lv_color_hex(0x1a3660), 0);
        lv_obj_t *l2 = lv_label_create(c2);
        lv_label_set_text(l2, LV_SYMBOL_FILE "\nInfo");
        lv_obj_set_style_text_align(l2, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(l2);
        break;
    }
    case PAGE_SETTINGS: {
        lv_obj_t *t = lv_label_create(p);
        lv_label_set_text(t, LV_SYMBOL_SETTINGS "  Settings");
        lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(t, lv_color_white(), 0);
        lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 20);

        s_lbl_bright = lv_label_create(p);
        lv_label_set_text_fmt(s_lbl_bright, "Brightness: %d %%", (int)s_brightness);
        lv_obj_set_style_text_font(s_lbl_bright, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(s_lbl_bright, lv_palette_main(LV_PALETTE_AMBER), 0);
        lv_obj_align(s_lbl_bright, LV_ALIGN_CENTER, 0, -30);

        lv_obj_t *sl = lv_slider_create(p);
        lv_obj_set_size(sl, 400, 20);
        lv_obj_align(sl, LV_ALIGN_CENTER, 0, 10);
        lv_slider_set_range(sl, 0, 100);
        lv_slider_set_value(sl, s_brightness, LV_ANIM_OFF);
        lv_obj_add_event_cb(sl, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

        lv_obj_t *btn = lv_button_create(p);
        lv_obj_set_size(btn, 160, 44);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_add_event_cb(btn, nav_back_home, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_obj_t *bl = lv_label_create(btn);
        lv_label_set_text(bl, LV_SYMBOL_LEFT " Back");
        lv_obj_center(bl);
        break;
    }
    case PAGE_INFO: {
        lv_obj_t *t = lv_label_create(p);
        lv_label_set_text(t, LV_SYMBOL_FILE "  Info");
        lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(t, lv_color_white(), 0);
        lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 20);

        s_lbl_uptime = lv_label_create(p);
        s_lbl_heap   = lv_label_create(p);
        s_lbl_navs   = lv_label_create(p);
        lv_obj_t *lbls[] = { s_lbl_uptime, s_lbl_heap, s_lbl_navs };
        for (int i = 0; i < 3; i++) {
            lv_label_set_text(lbls[i], "--");
            lv_obj_set_style_text_font(lbls[i], &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(lbls[i], lv_color_white(), 0);
            lv_obj_align(lbls[i], LV_ALIGN_TOP_LEFT, 40, 70 + i * 36);
        }
        s_info_timer = lv_timer_create(info_timer_cb, 500, NULL);
        info_timer_cb(NULL);

        lv_obj_t *btn = lv_button_create(p);
        lv_obj_set_size(btn, 160, 44);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_add_event_cb(btn, nav_back_home, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_obj_t *bl = lv_label_create(btn);
        lv_label_set_text(bl, LV_SYMBOL_LEFT " Back");
        lv_obj_center(bl);
        break;
    }
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_root = parent;
    s_page_container = NULL;
    s_info_timer = NULL;
    s_nav_count = 0;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I26 — Multi-Page App");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    show_page(PAGE_HOME);
}
