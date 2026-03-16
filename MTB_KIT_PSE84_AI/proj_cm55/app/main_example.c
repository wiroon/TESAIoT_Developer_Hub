/**
 * @file    main_example.c
 * @brief   Default Example — Hello World with Board Info
 *
 * This is the default example included with the MTB Template.
 * Replace this file with any example from the TESAIoT Developer Hub.
 *
 * Download examples: https://dev.tesaiot.com
 *
 * Functions:
 *   create_info_card()      — Build a styled info card
 *   update_uptime_cb()      — Timer callback: update uptime counter
 *   example_main()          — Entry point: compose board info display
 */

#include "pse84_common.h"

static lv_obj_t *s_lbl_uptime;

/* ── Create an info card ──────────────────────────────────────────── */
static lv_obj_t *create_info_card(lv_obj_t *parent, const char *title,
                                   const char *value, lv_color_t accent)
{
    lv_obj_t *card = example_card_create(parent, 340, 100, UI_COLOR_CARD_BG);
    lv_obj_set_style_border_color(card, accent, 0);
    lv_obj_set_style_shadow_color(card, accent, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_title, accent, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 4, 0);

    lv_obj_t *lbl_val = lv_label_create(card);
    lv_label_set_text(lbl_val, value);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_val, lv_color_white(), 0);
    lv_obj_align(lbl_val, LV_ALIGN_BOTTOM_LEFT, 4, -4);

    return card;
}

/* ── Timer: update uptime ─────────────────────────────────────────── */
static void update_uptime_cb(lv_timer_t *timer)
{
    (void)timer;
    uint32_t sec = xTaskGetTickCount() / configTICK_RATE_HZ;
    uint32_t h = sec / 3600;
    uint32_t m = (sec % 3600) / 60;
    uint32_t s = sec % 60;
    lv_label_set_text_fmt(s_lbl_uptime, "%02u:%02u:%02u", (unsigned)h, (unsigned)m, (unsigned)s);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_HOME "  TESAIoT Developer Hub");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* Subtitle */
    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "MTB Template " LV_SYMBOL_PLAY " Replace this file with your example");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 55);

    /* Row 1: Board + Sensors */
    lv_obj_t *c1 = create_info_card(parent, "Board", BOARD_NAME, UI_COLOR_INFO);
    lv_obj_align(c1, LV_ALIGN_CENTER, -180, -60);

    char sensor_str[32];
    lv_snprintf(sensor_str, sizeof(sensor_str), "%d sensors detected", SENSOR_COUNT);
    lv_obj_t *c2 = create_info_card(parent, "Sensors", sensor_str, UI_COLOR_SUCCESS);
    lv_obj_align(c2, LV_ALIGN_CENTER, 180, -60);

    /* Row 2: Uptime + Status */
    lv_obj_t *c3 = create_info_card(parent, "Uptime", "00:00:00", UI_COLOR_WARNING);
    lv_obj_align(c3, LV_ALIGN_CENTER, -180, 60);
    /* Get the value label from card for live update */
    s_lbl_uptime = lv_obj_get_child(c3, 1);  /* second child = value label */

    lv_obj_t *c4 = create_info_card(parent, "Status", LV_SYMBOL_OK " Template Ready", UI_COLOR_PRIMARY);
    lv_obj_align(c4, LV_ALIGN_CENTER, 180, 60);

    /* Footer */
    lv_obj_t *footer = lv_label_create(parent);
    lv_label_set_text(footer, "Download examples at dev.tesaiot.com | proj_cm55/app/main_example.c");
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(footer, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -15);

    /* Start uptime timer */
    lv_timer_create(update_uptime_cb, 1000, NULL);
    update_uptime_cb(NULL);
}
