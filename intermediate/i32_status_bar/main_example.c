/**
 * @file    main_example.c
 * @brief   Status Bar — Bottom bar with WiFi, board name, uptime, sensor count
 *
 * Fixed-height flex row at the bottom. Updates every 1 second.
 *
 * Functions:
 *   create_status_bar()      — Build the bottom status bar with flex layout
 *   update_wifi_status()     — Update WiFi label text and color
 *   status_timer_cb()        — Timer callback: refresh uptime + WiFi state
 *   example_main()           — Entry point: compose content area + status bar
 */

#include "example_common.h"

/* ── Compile-time sensor count ───────────────────────────────────── */
#define SENSOR_COUNT  (1 /* BMI270 always present */                  \
    + (BSP_HAS_DPS368)                                                \
    + (BSP_HAS_SHT40)                                                 \
    + (BSP_HAS_BMM350)                                                \
    + (BSP_HAS_CAPSENSE)                                              \
    + (BSP_HAS_POTENTIOMETER))

#ifndef BOARD_NAME
  #define BOARD_NAME  "BENTO KIT_PSE84"
#endif

static lv_obj_t *s_lbl_wifi;
static lv_obj_t *s_lbl_uptime;
static bool      s_wifi_connected;

/* ── Create the bottom status bar ────────────────────────────────── */
static lv_obj_t *create_status_bar(lv_obj_t *parent, int w, int h)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, w, h);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 6, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_left(bar, 12, 0);
    lv_obj_set_style_pad_right(bar, 12, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    return bar;
}

/* ── Update WiFi status display ──────────────────────────────────── */
static void update_wifi_status(bool connected)
{
    s_wifi_connected = connected;
    if (connected) {
        lv_label_set_text(s_lbl_wifi, LV_SYMBOL_WIFI " Connected");
        lv_obj_set_style_text_color(s_lbl_wifi,
            lv_palette_main(LV_PALETTE_GREEN), 0);
    } else {
        lv_label_set_text(s_lbl_wifi, LV_SYMBOL_WIFI " Disconnected");
        lv_obj_set_style_text_color(s_lbl_wifi,
            lv_palette_main(LV_PALETTE_GREY), 0);
    }
}

/* ── Timer — update status bar every 1 s ─────────────────────────── */
static void status_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    /* Uptime */
    uint32_t total_sec = xTaskGetTickCount() / configTICK_RATE_HZ;
    uint32_t h = total_sec / 3600;
    uint32_t m = (total_sec % 3600) / 60;
    uint32_t s = total_sec % 60;
    lv_label_set_text_fmt(s_lbl_uptime, "%02u:%02u:%02u",
                          (unsigned)h, (unsigned)m, (unsigned)s);

    /* Simulate WiFi toggle every 10 seconds for demo */
    update_wifi_status(((total_sec / 10) % 2) == 0);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_wifi_connected = false;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I32 — Status Bar");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Main content area (placeholder) */
    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_set_size(content, 720, 280);
    lv_obj_align(content, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(content, 12, 0);
    lv_obj_set_style_border_width(content, 1, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *content_lbl = lv_label_create(content);
    lv_label_set_text(content_lbl, "Main content area\n\n"
                                    "The status bar below updates every second.\n"
                                    "WiFi toggles every 10 seconds for demo.");
    lv_obj_set_style_text_font(content_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(content_lbl, lv_color_white(), 0);
    lv_obj_center(content_lbl);

    /* ── Status bar ──────────────────────────────────────────────── */
    lv_obj_t *bar = create_status_bar(parent, 760, 36);

    /* WiFi status */
    s_lbl_wifi = lv_label_create(bar);
    lv_label_set_text(s_lbl_wifi, LV_SYMBOL_WIFI " --");
    lv_obj_set_style_text_font(s_lbl_wifi, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_wifi,
                                lv_palette_main(LV_PALETTE_GREY), 0);

    /* Board name */
    lv_obj_t *lbl_board = lv_label_create(bar);
    lv_label_set_text(lbl_board, BOARD_NAME);
    lv_obj_set_style_text_font(lbl_board, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_board, lv_color_white(), 0);

    /* Uptime */
    s_lbl_uptime = lv_label_create(bar);
    lv_label_set_text(s_lbl_uptime, "00:00:00");
    lv_obj_set_style_text_font(s_lbl_uptime, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_uptime,
                                lv_palette_main(LV_PALETTE_AMBER), 0);

    /* Sensor count */
    lv_obj_t *lbl_sensors = lv_label_create(bar);
    lv_label_set_text_fmt(lbl_sensors, "Sensors: %d", SENSOR_COUNT);
    lv_obj_set_style_text_font(lbl_sensors, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sensors,
                                lv_palette_main(LV_PALETTE_TEAL), 0);

    /* Start update timer */
    lv_timer_create(status_timer_cb, 1000, NULL);
    status_timer_cb(NULL);  /* initial update */
}
