/**
 * A20 — Digital Twin
 *
 * Digital twin visualization of the PSoC Edge E84 board
 * with live sensor overlays, component status indicators,
 * and real-time data mapping to board positions.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define REFRESH_MS      300
#define BOARD_W         460
#define BOARD_H         280
#define BOARD_X         10
#define BOARD_Y         50
#define NUM_COMPONENTS  8

typedef struct {
    const char *name;
    lv_coord_t  x, y;
    lv_coord_t  w, h;
    lv_color_t  color;
    bool        active;
    const char *status;
} component_t;

typedef struct {
    lv_obj_t      *parent;
    lv_obj_t      *board_bg;
    lv_obj_t      *comp_rects[NUM_COMPONENTS];
    lv_obj_t      *comp_labels[NUM_COMPONENTS];
    lv_obj_t      *comp_status[NUM_COMPONENTS];
    component_t    comps[NUM_COMPONENTS];
    /* Live data overlays */
    lv_obj_t      *temp_overlay;
    lv_obj_t      *accel_overlay;
    lv_obj_t      *mag_overlay;
    lv_obj_t      *press_overlay;
    lv_obj_t      *wifi_overlay;
    /* Side panel */
    lv_obj_t      *detail_title;
    lv_obj_t      *detail_text;
    lv_obj_t      *cpu_bar;
    lv_obj_t      *cpu_label;
    lv_obj_t      *mem_bar;
    lv_obj_t      *mem_label;
    lv_obj_t      *temp_bar;
    lv_obj_t      *temp_label;
    /* Tilt visualization */
    lv_obj_t      *tilt_indicator;
    lv_obj_t      *tilt_card;
    int            selected_comp;
} app_ctx_t;

static app_ctx_t g_ctx;

static void init_components(app_ctx_t *ctx)
{
    ctx->comps[0] = (component_t){"PSoC E84\nCM33+CM55", 180, 100, 100, 60, UI_COLOR_PRIMARY, true, "Active"};
    ctx->comps[1] = (component_t){"BMI270\nIMU", 30, 40, 70, 40, UI_COLOR_BMI270, true, "Active"};
    ctx->comps[2] = (component_t){"DPS368\nBaro", 30, 100, 70, 40, UI_COLOR_DPS368, true, "Active"};
    ctx->comps[3] = (component_t){"SHT40\nClim", 30, 160, 70, 40, UI_COLOR_SHT40, true, "Active"};
    ctx->comps[4] = (component_t){"BMM350\nMag", 30, 220, 70, 40, UI_COLOR_BMM350, true, "Active"};
    ctx->comps[5] = (component_t){"CYW55513\nWiFi", 320, 40, 100, 50, UI_COLOR_INFO, true, "Active"};
    ctx->comps[6] = (component_t){"OPTIGA\nTrust M", 320, 110, 100, 50, UI_COLOR_ERROR, true, "Active"};
    ctx->comps[7] = (component_t){"QSPI\nFlash", 320, 180, 100, 50, lv_color_hex(0x607d8b), true, "Active"};

    /* Disable components not present on board */
#if !BSP_HAS_DPS368
    ctx->comps[2].active = false;
    ctx->comps[2].status = "N/A";
#endif
#if !BSP_HAS_SHT40
    ctx->comps[3].active = false;
    ctx->comps[3].status = "N/A";
#endif
#if !BSP_HAS_BMM350
    ctx->comps[4].active = false;
    ctx->comps[4].status = "N/A";
#endif
}

static void comp_click_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    app_ctx_t *ctx = &g_ctx;
    ctx->selected_comp = idx;

    /* Highlight selected */
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        lv_obj_set_style_border_width(ctx->comp_rects[i], (i == idx) ? 3 : 1, 0);
        lv_obj_set_style_border_color(ctx->comp_rects[i],
            (i == idx) ? UI_COLOR_PRIMARY : lv_color_hex(0x2a3a5c), 0);
    }

    component_t *c = &ctx->comps[idx];
    lv_label_set_text(ctx->detail_title, c->name);

    static const char *details[] = {
        "Dual-core MCU\nCM33: WiFi, MicroPython\nCM55: Display, AI, Sensors\n240MHz / 400MHz",
        "6-axis IMU\nAccel: +/-16g\nGyro: +/-2000dps\nI3C / SPI interface",
        "Barometric Pressure\nRange: 300-1200 hPa\nTemp: -40 to +85 C\nPrecision: +/-1 Pa",
        "Temp + Humidity\nTemp: +/-0.2 C\nRH: +/-1.8%\nI2C interface",
        "3-axis Magnetometer\nI3C interface\n+/-2000uT range\n0.3uT resolution",
        "WiFi 6 (802.11ax)\nSoftAP + STA\nBT 5.2 (LE only)\nSDHC interface",
        "Hardware Security\nECC P-256/P-384\nSHA-256, AES-128\nX.509 certificate store",
        "64MB QSPI Flash\nFirmware + Filesystem\nMicroPython scripts\nWear leveling",
    };
    lv_label_set_text(ctx->detail_text, details[idx]);
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    char buf[48];

    /* Temperature overlay */
#if BSP_HAS_DPS368 || BSP_HAS_SHT40
    snprintf(buf, sizeof(buf), "%.1f C", (double)((float)snap.dps368.temperature_x100 / 100.0f));
    lv_label_set_text(ctx->temp_overlay, buf);
    float temp_val = (float)snap.dps368.temperature_x100 / 100.0f;
#else
    lv_label_set_text(ctx->temp_overlay, "-- C");
    float temp_val = 25.0f;
#endif

    /* Accel overlay */
#if BSP_HAS_BMI270
    float ax = (float)snap.bmi270.ax / 16384.0f;
    float ay = (float)snap.bmi270.ay / 16384.0f;
    float az = (float)snap.bmi270.az / 16384.0f;
    snprintf(buf, sizeof(buf), "%.2f/%.2f/%.2f g", (double)ax, (double)ay, (double)az);
    lv_label_set_text(ctx->accel_overlay, buf);

    /* Tilt indicator */
    lv_coord_t tx = (lv_coord_t)(ax * 30.0f);
    lv_coord_t ty = (lv_coord_t)(ay * 30.0f);
    if (tx > 35) tx = 35; if (tx < -35) tx = -35;
    if (ty > 35) ty = 35; if (ty < -35) ty = -35;
    lv_obj_align(ctx->tilt_indicator, LV_ALIGN_CENTER, tx, ty);
#else
    lv_label_set_text(ctx->accel_overlay, "N/A");
#endif

    /* Mag overlay */
#if BSP_HAS_BMM350
    snprintf(buf, sizeof(buf), "%03.0f deg", (double)((float)snap.bmm350.heading_x10 / 10.0f));
    lv_label_set_text(ctx->mag_overlay, buf);
#else
    lv_label_set_text(ctx->mag_overlay, "N/A");
#endif

    /* Pressure overlay */
#if BSP_HAS_DPS368
    snprintf(buf, sizeof(buf), "%.1f hPa", (double)((float)snap.dps368.pressure_x100 / 100.0f));
    lv_label_set_text(ctx->press_overlay, buf);
#else
    lv_label_set_text(ctx->press_overlay, "N/A");
#endif

    /* WiFi overlay */
    lv_label_set_text(ctx->wifi_overlay,
        ipc_sensorhub_wifi_connected() ? "Connected" : "Disconnected");
    lv_obj_set_style_text_color(ctx->wifi_overlay,
        ipc_sensorhub_wifi_connected() ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);

    /* System gauges */
    int cpu_pct = 30 + (int)(fabsf(ax) * 20.0f);
    if (cpu_pct > 100) cpu_pct = 100;
    lv_bar_set_value(ctx->cpu_bar, cpu_pct, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%d%%", cpu_pct);
    lv_label_set_text(ctx->cpu_label, buf);

    int mem_pct = 62;
    lv_bar_set_value(ctx->mem_bar, mem_pct, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%d%%", mem_pct);
    lv_label_set_text(ctx->mem_label, buf);

    int temp_pct = (int)((temp_val - 15.0f) / 35.0f * 100.0f);
    if (temp_pct < 0) temp_pct = 0;
    if (temp_pct > 100) temp_pct = 100;
    lv_bar_set_value(ctx->temp_bar, temp_pct, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%.0f C", (double)temp_val);
    lv_label_set_text(ctx->temp_label, buf);

    /* Update component status colors */
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        lv_obj_set_style_bg_opa(ctx->comp_rects[i],
            ctx->comps[i].active ? LV_OPA_40 : LV_OPA_10, 0);
    }
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;
    ctx->selected_comp = 0;

    init_components(ctx);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " Digital Twin — PSoC Edge E84");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, 14, 8);

    /* Board background */
    ctx->board_bg = lv_obj_create(parent);
    lv_obj_set_size(ctx->board_bg, BOARD_W, BOARD_H);
    lv_obj_set_pos(ctx->board_bg, BOARD_X, BOARD_Y);
    lv_obj_set_style_bg_color(ctx->board_bg, lv_color_hex(0x0a1628), 0);
    lv_obj_set_style_bg_opa(ctx->board_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->board_bg, 12, 0);
    lv_obj_set_style_border_width(ctx->board_bg, 2, 0);
    lv_obj_set_style_border_color(ctx->board_bg, lv_color_hex(0x1a3050), 0);
    lv_obj_clear_flag(ctx->board_bg, LV_OBJ_FLAG_SCROLLABLE);

    /* Board label */
    lv_obj_t *bl = lv_label_create(ctx->board_bg);
    lv_label_set_text(bl, "PSoC Edge E84");
    lv_obj_set_style_text_color(bl, lv_color_hex(0x1a3050), 0);
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(bl, 160, 260);

    /* Component rectangles */
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        component_t *c = &ctx->comps[i];

        ctx->comp_rects[i] = lv_obj_create(ctx->board_bg);
        lv_obj_set_size(ctx->comp_rects[i], c->w, c->h);
        lv_obj_set_pos(ctx->comp_rects[i], c->x, c->y);
        lv_obj_set_style_bg_color(ctx->comp_rects[i], c->color, 0);
        lv_obj_set_style_bg_opa(ctx->comp_rects[i], c->active ? LV_OPA_40 : LV_OPA_10, 0);
        lv_obj_set_style_radius(ctx->comp_rects[i], 6, 0);
        lv_obj_set_style_border_width(ctx->comp_rects[i], 1, 0);
        lv_obj_set_style_border_color(ctx->comp_rects[i], lv_color_hex(0x2a3a5c), 0);
        lv_obj_clear_flag(ctx->comp_rects[i], LV_OBJ_FLAG_SCROLLABLE);

        ctx->comp_labels[i] = lv_label_create(ctx->comp_rects[i]);
        lv_label_set_text(ctx->comp_labels[i], c->name);
        lv_obj_set_style_text_color(ctx->comp_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->comp_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_center(ctx->comp_labels[i]);

        lv_obj_add_event_cb(ctx->comp_rects[i], comp_click_cb, LV_EVENT_CLICKED,
                           (void *)(intptr_t)i);
    }

    /* Data overlays on board */
    ctx->accel_overlay = lv_label_create(ctx->board_bg);
    lv_obj_set_style_text_color(ctx->accel_overlay, UI_COLOR_BMI270, 0);
    lv_obj_set_style_text_font(ctx->accel_overlay, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->accel_overlay, 110, 50);

    ctx->press_overlay = lv_label_create(ctx->board_bg);
    lv_obj_set_style_text_color(ctx->press_overlay, UI_COLOR_DPS368, 0);
    lv_obj_set_style_text_font(ctx->press_overlay, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->press_overlay, 110, 112);

    ctx->temp_overlay = lv_label_create(ctx->board_bg);
    lv_obj_set_style_text_color(ctx->temp_overlay, UI_COLOR_SHT40, 0);
    lv_obj_set_style_text_font(ctx->temp_overlay, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->temp_overlay, 110, 172);

    ctx->mag_overlay = lv_label_create(ctx->board_bg);
    lv_obj_set_style_text_color(ctx->mag_overlay, UI_COLOR_BMM350, 0);
    lv_obj_set_style_text_font(ctx->mag_overlay, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->mag_overlay, 110, 232);

    ctx->wifi_overlay = lv_label_create(ctx->board_bg);
    lv_obj_set_style_text_font(ctx->wifi_overlay, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->wifi_overlay, 320, 95);

    /* Side panel: Detail view */
    lv_obj_t *detail_card = lv_obj_create(parent);
    lv_obj_set_size(detail_card, 300, 180);
    lv_obj_set_pos(detail_card, 480, 50);
    lv_obj_set_style_bg_color(detail_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(detail_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(detail_card, 12, 0);
    lv_obj_set_style_border_width(detail_card, 1, 0);
    lv_obj_set_style_border_color(detail_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(detail_card, 12, 0);
    lv_obj_clear_flag(detail_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *dh = lv_label_create(detail_card);
    lv_label_set_text(dh, "COMPONENT DETAIL");
    lv_obj_set_style_text_color(dh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(dh, &lv_font_montserrat_14, 0);

    ctx->detail_title = lv_label_create(detail_card);
    lv_obj_set_style_text_color(ctx->detail_title, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(ctx->detail_title, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->detail_title, 0, 22);
    lv_label_set_text(ctx->detail_title, "PSoC E84");

    ctx->detail_text = lv_label_create(detail_card);
    lv_obj_set_style_text_color(ctx->detail_text, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->detail_text, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->detail_text, 0, 52);
    lv_obj_set_width(ctx->detail_text, 270);
    lv_label_set_long_mode(ctx->detail_text, LV_LABEL_LONG_WRAP);
    lv_label_set_text(ctx->detail_text, "Tap a component on the board to see details.");

    /* System gauges */
    lv_obj_t *gauge_card = lv_obj_create(parent);
    lv_obj_set_size(gauge_card, 300, 120);
    lv_obj_set_pos(gauge_card, 480, 240);
    lv_obj_set_style_bg_color(gauge_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(gauge_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(gauge_card, 12, 0);
    lv_obj_set_style_border_width(gauge_card, 1, 0);
    lv_obj_set_style_border_color(gauge_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(gauge_card, 10, 0);
    lv_obj_clear_flag(gauge_card, LV_OBJ_FLAG_SCROLLABLE);

    static const char *gauge_names[] = {"CPU", "MEM", "TEMP"};
    lv_obj_t **bars[] = {&ctx->cpu_bar, &ctx->mem_bar, &ctx->temp_bar};
    lv_obj_t **labels[] = {&ctx->cpu_label, &ctx->mem_label, &ctx->temp_label};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *gl = lv_label_create(gauge_card);
        lv_label_set_text(gl, gauge_names[i]);
        lv_obj_set_style_text_color(gl, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(gl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(gl, 0, i * 32);

        *bars[i] = lv_bar_create(gauge_card);
        lv_obj_set_size(*bars[i], 180, 14);
        lv_obj_set_pos(*bars[i], 50, i * 32 + 2);
        lv_bar_set_range(*bars[i], 0, 100);
        lv_obj_set_style_bg_color(*bars[i], lv_color_hex(0x263238), LV_PART_MAIN);
        lv_obj_set_style_bg_color(*bars[i], UI_COLOR_PRIMARY, LV_PART_INDICATOR);

        *labels[i] = lv_label_create(gauge_card);
        lv_obj_set_style_text_color(*labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(*labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(*labels[i], 240, i * 32);
        lv_label_set_text(*labels[i], "--%");
    }

    /* Tilt indicator */
    ctx->tilt_card = lv_obj_create(parent);
    lv_obj_set_size(ctx->tilt_card, 90, 90);
    lv_obj_set_pos(ctx->tilt_card, 480, 370);
    lv_obj_set_style_bg_color(ctx->tilt_card, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(ctx->tilt_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->tilt_card, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ctx->tilt_card, 1, 0);
    lv_obj_set_style_border_color(ctx->tilt_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_clear_flag(ctx->tilt_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->tilt_indicator = lv_obj_create(ctx->tilt_card);
    lv_obj_set_size(ctx->tilt_indicator, 12, 12);
    lv_obj_align(ctx->tilt_indicator, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ctx->tilt_indicator, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(ctx->tilt_indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->tilt_indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ctx->tilt_indicator, 0, 0);
    lv_obj_clear_flag(ctx->tilt_indicator, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tl = lv_label_create(parent);
    lv_label_set_text(tl, "BOARD TILT");
    lv_obj_set_style_text_color(tl, lv_color_hex(0x546e7a), 0);
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(tl, 580, 400);

    /* Trigger initial detail */
    comp_click_cb(NULL);

    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
