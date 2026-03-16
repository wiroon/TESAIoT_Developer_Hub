/**
 * @file    main_example.c
 * @brief   Retro Terminal Dashboard — Green-on-black CRT aesthetic
 *
 * @description
 *   Retro-styled sensor dashboard with green-on-black terminal aesthetic,
 *   CRT scanline effect, pixelated header, blinking cursor, and classic
 *   4-tone Game Boy color palette. Reads live sensor data via
 *   ipc_sensorhub_snapshot and displays in terminal-style formatted output.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"

/* ---------------------------------------------------------------------------
 * Game Boy / CRT Palette
 * --------------------------------------------------------------------------- */
#define GB_LIGHTEST     lv_color_hex(0x9BBC0F)
#define GB_LIGHT        lv_color_hex(0x8BAC0F)
#define GB_DARK         lv_color_hex(0x306230)
#define GB_DARKEST      lv_color_hex(0x0F380F)

#define TERM_GREEN      lv_color_hex(0x33FF33)
#define TERM_DIM        lv_color_hex(0x1A8C1A)
#define TERM_BG         lv_color_hex(0x0A0A0A)
#define TERM_BORDER     lv_color_hex(0x1A3A1A)

#define SCREEN_W        440
#define SCREEN_H        280
#define SCANLINE_SPACING 3
#define UPDATE_MS       1000

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *terminal;
    lv_obj_t    *cursor;
    lv_obj_t    *time_label;
    lv_obj_t    *uptime_label;
    lv_obj_t    *sensor_lines[6];
    lv_obj_t    *status_bar;
    lv_timer_t  *update_timer;
    lv_timer_t  *blink_timer;
    uint32_t     tick_count;
    bool         cursor_visible;
} retro_ctx_t;

static retro_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Blinking cursor timer
 * --------------------------------------------------------------------------- */
static void blink_timer_cb(lv_timer_t *timer)
{
    retro_ctx_t *ctx = (retro_ctx_t *)lv_timer_get_user_data(timer);
    ctx->cursor_visible = !ctx->cursor_visible;

    if (ctx->cursor_visible) {
        lv_obj_set_style_bg_opa(ctx->cursor, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_bg_opa(ctx->cursor, LV_OPA_TRANSP, 0);
    }
}

/* ---------------------------------------------------------------------------
 * Sensor update timer — reads real sensor data
 * --------------------------------------------------------------------------- */
static void update_timer_cb(lv_timer_t *timer)
{
    retro_ctx_t *ctx = (retro_ctx_t *)lv_timer_get_user_data(timer);
    ctx->tick_count++;

    /* Uptime */
    uint32_t secs = ctx->tick_count;
    uint32_t mins = secs / 60;
    uint32_t hrs  = mins / 60;
    char uptime_str[48];
    snprintf(uptime_str, sizeof(uptime_str),
             "> UPTIME: %02lu:%02lu:%02lu",
             (unsigned long)hrs, (unsigned long)(mins % 60),
             (unsigned long)(secs % 60));
    lv_label_set_text(ctx->uptime_label, uptime_str);

    /* Time from NTP if available */
    if (ipc_sensorhub_ntp_synced()) {
        char time_str[32]; ipc_sensorhub_get_time_str(time_str, sizeof(time_str));
        char time_buf[48];
        snprintf(time_buf, sizeof(time_buf), "> TIME: %s",
                 time_str ? time_str : "N/A");
        lv_label_set_text(ctx->time_label, time_buf);
    } else {
        lv_label_set_text(ctx->time_label, "> TIME: [NO NTP SYNC]");
    }

    /* Read sensor data */
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    {
        char buf[64];

        snprintf(buf, sizeof(buf), "> IMU.ACC  X:%+6.2f Y:%+6.2f Z:%+6.2f",
                 snap.bmi270.ax, snap.bmi270.ay, snap.bmi270.az);
        lv_label_set_text(ctx->sensor_lines[0], buf);

        snprintf(buf, sizeof(buf), "> IMU.GYR  X:%+7.1f Y:%+7.1f Z:%+7.1f",
                 snap.bmi270.gx, snap.bmi270.gy, snap.bmi270.gz);
        lv_label_set_text(ctx->sensor_lines[1], buf);

#if BSP_HAS_DPS368
        snprintf(buf, sizeof(buf), "> BARO     P:%7.1f hPa  T:%5.1f C",
                 snap.dps368.pressure_x100, snap.dps368.temperature_x100);
        lv_label_set_text(ctx->sensor_lines[2], buf);
#else
        lv_label_set_text(ctx->sensor_lines[2], "> BARO     [NOT AVAILABLE]");
#endif

#if BSP_HAS_SHT40
        snprintf(buf, sizeof(buf), "> CLIMATE  T:%5.1f C  H:%5.1f %%",
                 snap.sht40.temperature_x100, snap.sht40.humidity_x100);
        lv_label_set_text(ctx->sensor_lines[3], buf);
#else
        lv_label_set_text(ctx->sensor_lines[3], "> CLIMATE  [NOT AVAILABLE]");
#endif

#if BSP_HAS_BMM350
        snprintf(buf, sizeof(buf), "> MAG      X:%+7.1f Y:%+7.1f Z:%+7.1f",
                 snap.bmm350.mx_x100, snap.bmm350.my_x100, snap.bmm350.mz_x100);
        lv_label_set_text(ctx->sensor_lines[4], buf);
#else
        lv_label_set_text(ctx->sensor_lines[4], "> MAG      [NOT AVAILABLE]");
#endif

        /* WiFi status */
        snprintf(buf, sizeof(buf), "> WIFI     %s",
                 ipc_sensorhub_wifi_connected() ? "CONNECTED" : "DISCONNECTED");
        lv_label_set_text(ctx->sensor_lines[5], buf);
        lv_obj_set_style_text_color(ctx->sensor_lines[5],
            ipc_sensorhub_wifi_connected() ? TERM_GREEN : lv_color_hex(0xFF3333), 0);
    }

    /* Status bar blink */
    char status[64];
    snprintf(status, sizeof(status), " BENTO PSE84 | TICK %lu | MEM OK ",
             (unsigned long)ctx->tick_count);
    lv_label_set_text(ctx->status_bar, status);
}

/* ---------------------------------------------------------------------------
 * Helper: create a terminal-style label
 * --------------------------------------------------------------------------- */
static lv_obj_t *term_label(lv_obj_t *parent, const char *text, int y_offset)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, TERM_GREEN, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 12, y_offset);
    return lbl;
}

/* ---------------------------------------------------------------------------
 * Create scanline overlay effect
 * --------------------------------------------------------------------------- */
static void add_scanlines(lv_obj_t *terminal)
{
    for (int y = 0; y < SCREEN_H; y += SCANLINE_SPACING) {
        lv_obj_t *line = lv_obj_create(terminal);
        lv_obj_set_size(line, SCREEN_W - 4, 1);
        lv_obj_set_pos(line, 2, y);
        lv_obj_set_style_bg_color(line, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(line, 30, 0);  /* Very subtle */
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* Set parent background to black */
    lv_obj_set_style_bg_color(parent, TERM_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* === CRT Badge (top) === */
    lv_obj_t *badge = lv_obj_create(parent);
    lv_obj_set_size(badge, SCREEN_W, 28);
    lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(badge, GB_DARKEST, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge, GB_DARK, 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_radius(badge, 2, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *badge_text = lv_label_create(badge);
    lv_label_set_text(badge_text, "BENTO RETRO TERMINAL v1.0");
    lv_obj_set_style_text_color(badge_text, GB_LIGHTEST, 0);
    lv_obj_set_style_text_letter_space(badge_text, 2, 0);
    lv_obj_center(badge_text);

    /* === Terminal screen area === */
    lv_obj_t *bezel = lv_obj_create(parent);
    lv_obj_set_size(bezel, SCREEN_W + 12, SCREEN_H + 12);
    lv_obj_align(bezel, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(bezel, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(bezel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bezel, 6, 0);
    lv_obj_set_style_border_color(bezel, TERM_BORDER, 0);
    lv_obj_set_style_border_width(bezel, 2, 0);
    lv_obj_clear_flag(bezel, LV_OBJ_FLAG_SCROLLABLE);

    s_ctx.terminal = lv_obj_create(bezel);
    lv_obj_set_size(s_ctx.terminal, SCREEN_W, SCREEN_H);
    lv_obj_center(s_ctx.terminal);
    lv_obj_set_style_bg_color(s_ctx.terminal, TERM_BG, 0);
    lv_obj_set_style_bg_opa(s_ctx.terminal, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_ctx.terminal, 2, 0);
    lv_obj_set_style_border_width(s_ctx.terminal, 0, 0);
    lv_obj_clear_flag(s_ctx.terminal, LV_OBJ_FLAG_SCROLLABLE);

    /* Terminal content */
    int y = 8;
    lv_obj_t *header = term_label(s_ctx.terminal,
        "=== BENTO PSE84 SENSOR TERMINAL ===", y);
    lv_obj_set_style_text_color(header, TERM_DIM, 0);
    y += 20;

    lv_obj_t *sep1 = term_label(s_ctx.terminal,
        "----------------------------------------", y);
    lv_obj_set_style_text_color(sep1, TERM_BORDER, 0);
    y += 18;

    s_ctx.time_label = term_label(s_ctx.terminal, "> TIME: [SYNCING...]", y);
    y += 18;

    s_ctx.uptime_label = term_label(s_ctx.terminal, "> UPTIME: 00:00:00", y);
    y += 22;

    lv_obj_t *sep2 = term_label(s_ctx.terminal,
        "--- SENSOR READINGS ---", y);
    lv_obj_set_style_text_color(sep2, TERM_DIM, 0);
    y += 18;

    for (int i = 0; i < 6; i++) {
        s_ctx.sensor_lines[i] = term_label(s_ctx.terminal, "> ...", y);
        y += 18;
    }

    /* Blinking cursor */
    y += 4;
    lv_obj_t *prompt = term_label(s_ctx.terminal, ">", y);
    (void)prompt;

    s_ctx.cursor = lv_obj_create(s_ctx.terminal);
    lv_obj_set_size(s_ctx.cursor, 8, 14);
    lv_obj_set_pos(s_ctx.cursor, 24, y);
    lv_obj_set_style_bg_color(s_ctx.cursor, TERM_GREEN, 0);
    lv_obj_set_style_bg_opa(s_ctx.cursor, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ctx.cursor, 0, 0);
    lv_obj_set_style_radius(s_ctx.cursor, 1, 0);
    lv_obj_clear_flag(s_ctx.cursor, LV_OBJ_FLAG_SCROLLABLE);

    /* Scanline overlay */
    add_scanlines(s_ctx.terminal);

    /* === Status bar (bottom) === */
    lv_obj_t *status_bar_bg = lv_obj_create(parent);
    lv_obj_set_size(status_bar_bg, SCREEN_W + 12, 24);
    lv_obj_align(status_bar_bg, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(status_bar_bg, GB_DARK, 0);
    lv_obj_set_style_bg_opa(status_bar_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(status_bar_bg, 2, 0);
    lv_obj_set_style_border_width(status_bar_bg, 0, 0);
    lv_obj_clear_flag(status_bar_bg, LV_OBJ_FLAG_SCROLLABLE);

    s_ctx.status_bar = lv_label_create(status_bar_bg);
    lv_label_set_text(s_ctx.status_bar, " BENTO PSE84 | TICK 0 | MEM OK ");
    lv_obj_set_style_text_color(s_ctx.status_bar, GB_LIGHTEST, 0);
    lv_obj_set_style_text_font(s_ctx.status_bar, &lv_font_montserrat_14, 0);
    lv_obj_center(s_ctx.status_bar);

    /* === Game Boy palette swatches (bottom corners) === */
    lv_color_t gb_colors[] = { GB_LIGHTEST, GB_LIGHT, GB_DARK, GB_DARKEST };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *swatch = lv_obj_create(parent);
        lv_obj_set_size(swatch, 16, 16);
        lv_obj_align(swatch, LV_ALIGN_BOTTOM_LEFT, 8 + i * 20, -6);
        lv_obj_set_style_bg_color(swatch, gb_colors[i], 0);
        lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(swatch, 2, 0);
        lv_obj_set_style_border_width(swatch, 0, 0);
        lv_obj_clear_flag(swatch, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Start timers */
    s_ctx.update_timer = lv_timer_create(update_timer_cb, UPDATE_MS, &s_ctx);
    s_ctx.blink_timer = lv_timer_create(blink_timer_cb, 500, &s_ctx);
}
