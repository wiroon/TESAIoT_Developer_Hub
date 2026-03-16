/**
 * A17 — Multi-Page Application
 *
 * Complete multi-page application with sidebar navigation,
 * 4 pages (Dashboard, Sensors, Settings, About), page transitions,
 * and persistent state across pages.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>

#define NUM_PAGES       4
#define SIDEBAR_W       120
#define CONTENT_W       (780 - SIDEBAR_W - 10)
#define CONTENT_H       410
#define REFRESH_MS      500

typedef enum { PAGE_DASH, PAGE_SENSORS, PAGE_SETTINGS, PAGE_ABOUT } page_id_t;

typedef struct {
    lv_obj_t   *parent;
    lv_obj_t   *sidebar;
    lv_obj_t   *content_area;
    lv_obj_t   *page_objs[NUM_PAGES];
    lv_obj_t   *nav_btns[NUM_PAGES];
    page_id_t   current_page;

    /* Dashboard page */
    lv_obj_t   *dash_temp_label;
    lv_obj_t   *dash_accel_label;
    lv_obj_t   *dash_wifi_label;
    lv_obj_t   *dash_uptime_label;

    /* Sensors page */
    lv_obj_t   *sens_labels[6];
    lv_obj_t   *sens_chart;
    lv_chart_series_t *sens_series;

    /* Settings */
    lv_obj_t   *bright_slider;
    lv_obj_t   *bright_label;
    lv_obj_t   *refresh_dd;
    lv_obj_t   *theme_sw;

    uint32_t    uptime_s;
    int         brightness;
} app_ctx_t;

static app_ctx_t g_ctx;

static void show_page(app_ctx_t *ctx, page_id_t page)
{
    ctx->current_page = page;
    for (int i = 0; i < NUM_PAGES; i++) {
        if (i == (int)page) {
            lv_obj_clear_flag(ctx->page_objs[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(ctx->nav_btns[i], UI_COLOR_PRIMARY, 0);
            lv_obj_set_style_bg_opa(ctx->nav_btns[i], LV_OPA_40, 0);
        } else {
            lv_obj_add_flag(ctx->page_objs[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(ctx->nav_btns[i], lv_color_hex(0x1a2332), 0);
            lv_obj_set_style_bg_opa(ctx->nav_btns[i], LV_OPA_COVER, 0);
        }
    }
}

static void nav_btn_cb(lv_event_t *e)
{
    int page = (int)(intptr_t)lv_event_get_user_data(e);
    show_page(&g_ctx, (page_id_t)page);
}

static void slider_cb(lv_event_t *e)
{
    app_ctx_t *ctx = &g_ctx;
    ctx->brightness = (int)lv_slider_get_value(ctx->bright_slider);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", ctx->brightness);
    lv_label_set_text(ctx->bright_label, buf);
}

static lv_obj_t *make_page(lv_obj_t *content_area)
{
    lv_obj_t *page = lv_obj_create(content_area);
    lv_obj_set_size(page, CONTENT_W - 4, CONTENT_H - 4);
    lv_obj_center(page);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 12, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}

static void build_dashboard_page(app_ctx_t *ctx, lv_obj_t *page)
{
    lv_obj_t *h = lv_label_create(page);
    lv_label_set_text(h, "Dashboard");
    lv_obj_set_style_text_color(h, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(h, &lv_font_montserrat_24, 0);

    static const struct { const char *title; lv_color_t c; int y; } cards[] = {
        {"TEMPERATURE", {.blue=0x00, .green=0x98, .red=0xFF}, 44},
        {"ACCELERATION", {.blue=0x50, .green=0xAF, .red=0x4C}, 124},
        {"WIFI STATUS", {.blue=0xF3, .green=0x96, .red=0x21}, 204},
        {"UPTIME", {.blue=0xFB, .green=0x40, .red=0xE0}, 284},
    };

    lv_obj_t **val_ptrs[] = {
        &ctx->dash_temp_label, &ctx->dash_accel_label,
        &ctx->dash_wifi_label, &ctx->dash_uptime_label
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *c = lv_obj_create(page);
        lv_obj_set_size(c, CONTENT_W - 30, 68);
        lv_obj_set_pos(c, 0, cards[i].y);
        lv_obj_set_style_bg_color(c, UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(c, 10, 0);
        lv_obj_set_style_border_width(c, 1, 0);
        lv_obj_set_style_border_color(c, lv_color_hex(0x2a3a5c), 0);
        lv_obj_set_style_pad_all(c, 10, 0);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *t = lv_label_create(c);
        lv_label_set_text(t, cards[i].title);
        lv_obj_set_style_text_color(t, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);

        *val_ptrs[i] = lv_label_create(c);
        lv_obj_set_style_text_color(*val_ptrs[i], cards[i].c, 0);
        lv_obj_set_style_text_font(*val_ptrs[i], &lv_font_montserrat_20, 0);
        lv_obj_set_pos(*val_ptrs[i], 0, 24);
        lv_label_set_text(*val_ptrs[i], "---");
    }
}

static void build_sensors_page(app_ctx_t *ctx, lv_obj_t *page)
{
    lv_obj_t *h = lv_label_create(page);
    lv_label_set_text(h, "Sensor Readings");
    lv_obj_set_style_text_color(h, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(h, &lv_font_montserrat_24, 0);

    static const char *names[] = {"Accel X", "Accel Y", "Accel Z", "Gyro X", "Gyro Y", "Gyro Z"};
    for (int i = 0; i < 6; i++) {
        lv_obj_t *n = lv_label_create(page);
        lv_label_set_text(n, names[i]);
        lv_obj_set_style_text_color(n, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(n, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(n, (i / 3) * 300, 40 + (i % 3) * 28);

        ctx->sens_labels[i] = lv_label_create(page);
        lv_obj_set_style_text_color(ctx->sens_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->sens_labels[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(ctx->sens_labels[i], (i / 3) * 300 + 80, 38 + (i % 3) * 28);
        lv_label_set_text(ctx->sens_labels[i], "0.00");
    }

    ctx->sens_chart = lv_chart_create(page);
    lv_obj_set_size(ctx->sens_chart, CONTENT_W - 40, 240);
    lv_obj_set_pos(ctx->sens_chart, 0, 140);
    lv_chart_set_type(ctx->sens_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->sens_chart, 100);
    lv_chart_set_range(ctx->sens_chart, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000);
    lv_obj_set_style_bg_color(ctx->sens_chart, lv_color_hex(0x0d1117), 0);
    lv_obj_set_style_size(ctx->sens_chart, 0, 0, LV_PART_INDICATOR);
    ctx->sens_series = lv_chart_add_series(ctx->sens_chart, UI_COLOR_BMI270,
                                            LV_CHART_AXIS_PRIMARY_Y);
}

static void build_settings_page(app_ctx_t *ctx, lv_obj_t *page)
{
    lv_obj_t *h = lv_label_create(page);
    lv_label_set_text(h, "Settings");
    lv_obj_set_style_text_color(h, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(h, &lv_font_montserrat_24, 0);

    /* Brightness */
    lv_obj_t *bl = lv_label_create(page);
    lv_label_set_text(bl, "Brightness:");
    lv_obj_set_style_text_color(bl, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(bl, 0, 50);

    ctx->bright_slider = lv_slider_create(page);
    lv_obj_set_size(ctx->bright_slider, 300, 16);
    lv_obj_set_pos(ctx->bright_slider, 0, 76);
    lv_slider_set_range(ctx->bright_slider, 10, 100);
    lv_slider_set_value(ctx->bright_slider, 80, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx->bright_slider, lv_color_hex(0x263238), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ctx->bright_slider, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ctx->bright_slider, UI_COLOR_TEXT, LV_PART_KNOB);
    lv_obj_add_event_cb(ctx->bright_slider, slider_cb, LV_EVENT_VALUE_CHANGED, ctx);

    ctx->bright_label = lv_label_create(page);
    lv_obj_set_style_text_color(ctx->bright_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->bright_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->bright_label, 320, 72);
    lv_label_set_text(ctx->bright_label, "80%");
    ctx->brightness = 80;

    /* Refresh rate */
    lv_obj_t *rl = lv_label_create(page);
    lv_label_set_text(rl, "Refresh Rate:");
    lv_obj_set_style_text_color(rl, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(rl, 0, 120);

    ctx->refresh_dd = lv_dropdown_create(page);
    lv_dropdown_set_options(ctx->refresh_dd, "100ms\n250ms\n500ms\n1000ms");
    lv_dropdown_set_selected(ctx->refresh_dd, 2);
    lv_obj_set_size(ctx->refresh_dd, 200, 36);
    lv_obj_set_pos(ctx->refresh_dd, 0, 144);
    lv_obj_set_style_bg_color(ctx->refresh_dd, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_text_color(ctx->refresh_dd, UI_COLOR_TEXT, 0);

    /* Dark mode toggle */
    lv_obj_t *tl = lv_label_create(page);
    lv_label_set_text(tl, "Dark Theme:");
    lv_obj_set_style_text_color(tl, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(tl, 0, 200);

    ctx->theme_sw = lv_switch_create(page);
    lv_obj_set_pos(ctx->theme_sw, 120, 198);
    lv_obj_set_size(ctx->theme_sw, 50, 24);
    lv_obj_add_state(ctx->theme_sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(ctx->theme_sw, lv_color_hex(0x37474f), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ctx->theme_sw, UI_COLOR_PRIMARY, LV_PART_INDICATOR | LV_STATE_CHECKED);

    /* Board info */
    lv_obj_t *bi = lv_label_create(page);
    lv_label_set_text(bi, "Board Configuration:");
    lv_obj_set_style_text_color(bi, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(bi, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(bi, 0, 260);

    char binfo[128];
    snprintf(binfo, sizeof(binfo),
        "BMI270: %s  |  DPS368: %s\n"
        "SHT40:  %s  |  BMM350: %s\n"
        "CapSense: %s  |  Pot: %s",
#if BSP_HAS_BMI270
        "YES",
#else
        "NO",
#endif
#if BSP_HAS_DPS368
        "YES",
#else
        "NO",
#endif
#if BSP_HAS_SHT40
        "YES",
#else
        "NO",
#endif
#if BSP_HAS_BMM350
        "YES",
#else
        "NO",
#endif
#if BSP_HAS_CAPSENSE
        "YES",
#else
        "NO",
#endif
#if BSP_HAS_POTENTIOMETER
        "YES"
#else
        "NO"
#endif
    );
    lv_obj_t *bv = lv_label_create(page);
    lv_label_set_text(bv, binfo);
    lv_obj_set_style_text_color(bv, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(bv, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(bv, 0, 280);
}

static void build_about_page(app_ctx_t *ctx, lv_obj_t *page)
{
    (void)ctx;
    lv_obj_t *h = lv_label_create(page);
    lv_label_set_text(h, "About");
    lv_obj_set_style_text_color(h, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(h, &lv_font_montserrat_24, 0);

    lv_obj_t *info = lv_label_create(page);
    lv_label_set_text(info,
        "BENTO : : Make Anything.\n\n"
        "Multi-Page Application Demo\n"
        "TESAIoT PSoC Edge E84\n\n"
        "Features:\n"
        "  - Sidebar navigation\n"
        "  - 4 interactive pages\n"
        "  - Live sensor integration\n"
        "  - Settings persistence\n"
        "  - BSP-aware configuration\n\n"
        "Platform: PSoC Edge E84\n"
        "Display: 800x480 LVGL 9.2\n"
        "Cores: CM33_S + CM33_NS + CM55\n\n"
        "https://ide.tesaiot.com/");
    lv_obj_set_style_text_color(info, lv_color_hex(0x90a4ae), 0);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(info, 0, 40);
    lv_obj_set_width(info, CONTENT_W - 50);
    lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    ctx->uptime_s++;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    char buf[48];

    /* Dashboard updates */
#if BSP_HAS_DPS368 || BSP_HAS_SHT40
    snprintf(buf, sizeof(buf), "%.1f C", (double)((float)snap.dps368.temperature_x100 / 100.0f));
#else
    snprintf(buf, sizeof(buf), "N/A");
#endif
    lv_label_set_text(ctx->dash_temp_label, buf);

#if BSP_HAS_BMI270
    float mag = sqrtf((float)(snap.bmi270.ax * snap.bmi270.ax + snap.bmi270.ay * snap.bmi270.ay + snap.bmi270.az * snap.bmi270.az)) / 16384.0f;
    snprintf(buf, sizeof(buf), "%.2f g", (double)mag);
#else
    snprintf(buf, sizeof(buf), "N/A");
#endif
    lv_label_set_text(ctx->dash_accel_label, buf);

    lv_label_set_text(ctx->dash_wifi_label,
        ipc_sensorhub_wifi_connected() ? "Connected" : "Disconnected");

    snprintf(buf, sizeof(buf), "%lum %lus",
             (unsigned long)(ctx->uptime_s / 60),
             (unsigned long)(ctx->uptime_s % 60));
    lv_label_set_text(ctx->dash_uptime_label, buf);

    /* Sensor page updates */
#if BSP_HAS_BMI270
    float vals[6] = {
        (float)snap.bmi270.ax / 16384.0f, (float)snap.bmi270.ay / 16384.0f, (float)snap.bmi270.az / 16384.0f,
        (float)snap.bmi270.gx / 16.4f, (float)snap.bmi270.gy / 16.4f, (float)snap.bmi270.gz / 16.4f
    };
    static const char *units[] = {"g", "g", "g", "dps", "dps", "dps"};
    for (int i = 0; i < 6; i++) {
        snprintf(buf, sizeof(buf), "%+.2f %s", (double)vals[i], units[i]);
        lv_label_set_text(ctx->sens_labels[i], buf);
    }
    lv_chart_set_next_value(ctx->sens_chart, ctx->sens_series,
        (lv_coord_t)(vals[0] * 1000.0f));
#endif
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Sidebar */
    ctx->sidebar = lv_obj_create(parent);
    lv_obj_set_size(ctx->sidebar, SIDEBAR_W, 440);
    lv_obj_set_pos(ctx->sidebar, 4, 4);
    lv_obj_set_style_bg_color(ctx->sidebar, lv_color_hex(0x0f1923), 0);
    lv_obj_set_style_bg_opa(ctx->sidebar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->sidebar, 10, 0);
    lv_obj_set_style_border_width(ctx->sidebar, 1, 0);
    lv_obj_set_style_border_color(ctx->sidebar, lv_color_hex(0x1a2a40), 0);
    lv_obj_set_style_pad_all(ctx->sidebar, 6, 0);
    lv_obj_clear_flag(ctx->sidebar, LV_OBJ_FLAG_SCROLLABLE);

    /* App title */
    lv_obj_t *at = lv_label_create(ctx->sidebar);
    lv_label_set_text(at, "BENTO");
    lv_obj_set_style_text_color(at, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(at, &lv_font_montserrat_16, 0);
    lv_obj_align(at, LV_ALIGN_TOP_MID, 0, 4);

    /* Nav buttons */
    static const char *page_names[] = {
        LV_SYMBOL_HOME " Home",
        LV_SYMBOL_SETTINGS " Sensors",
        LV_SYMBOL_LIST " Settings",
        LV_SYMBOL_FILE " About"
    };

    for (int i = 0; i < NUM_PAGES; i++) {
        ctx->nav_btns[i] = lv_btn_create(ctx->sidebar);
        lv_obj_set_size(ctx->nav_btns[i], SIDEBAR_W - 16, 40);
        lv_obj_set_pos(ctx->nav_btns[i], 2, 36 + i * 48);
        lv_obj_set_style_bg_color(ctx->nav_btns[i], lv_color_hex(0x1a2332), 0);
        lv_obj_set_style_radius(ctx->nav_btns[i], 8, 0);

        lv_obj_t *bl = lv_label_create(ctx->nav_btns[i]);
        lv_label_set_text(bl, page_names[i]);
        lv_obj_set_style_text_color(bl, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
        lv_obj_align(bl, LV_ALIGN_LEFT_MID, 4, 0);

        lv_obj_add_event_cb(ctx->nav_btns[i], nav_btn_cb, LV_EVENT_CLICKED,
                           (void *)(intptr_t)i);
    }

    /* Content area */
    ctx->content_area = lv_obj_create(parent);
    lv_obj_set_size(ctx->content_area, CONTENT_W, CONTENT_H);
    lv_obj_set_pos(ctx->content_area, SIDEBAR_W + 10, 4);
    lv_obj_set_style_bg_color(ctx->content_area, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx->content_area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->content_area, 10, 0);
    lv_obj_set_style_border_width(ctx->content_area, 1, 0);
    lv_obj_set_style_border_color(ctx->content_area, lv_color_hex(0x2a3a5c), 0);
    lv_obj_clear_flag(ctx->content_area, LV_OBJ_FLAG_SCROLLABLE);

    /* Build pages */
    for (int i = 0; i < NUM_PAGES; i++) {
        ctx->page_objs[i] = make_page(ctx->content_area);
    }

    build_dashboard_page(ctx, ctx->page_objs[PAGE_DASH]);
    build_sensors_page(ctx, ctx->page_objs[PAGE_SENSORS]);
    build_settings_page(ctx, ctx->page_objs[PAGE_SETTINGS]);
    build_about_page(ctx, ctx->page_objs[PAGE_ABOUT]);

    show_page(ctx, PAGE_DASH);
    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
