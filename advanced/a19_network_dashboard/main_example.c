/**
 * A19 — Network Dashboard
 *
 * Network status dashboard with WiFi signal strength,
 * connection history, IP info, and scan results table.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>

#define REFRESH_MS      2000
#define RSSI_HISTORY    40
#define MAX_SCAN        8

typedef struct {
    char    ssid[33];
    int     rssi;
    int     channel;
    bool    encrypted;
} scan_entry_t;

typedef struct {
    lv_obj_t   *parent;
    /* Connection panel */
    lv_obj_t   *status_dot;
    lv_obj_t   *status_label;
    lv_obj_t   *ssid_label;
    lv_obj_t   *ip_label;
    lv_obj_t   *mac_label;
    lv_obj_t   *rssi_label;
    lv_obj_t   *rssi_bar;
    lv_obj_t   *channel_label;
    /* Signal chart */
    lv_obj_t   *chart;
    lv_chart_series_t *rssi_series;
    /* Stats */
    lv_obj_t   *uptime_label;
    lv_obj_t   *tx_label;
    lv_obj_t   *rx_label;
    lv_obj_t   *reconnect_label;
    /* Scan results */
    lv_obj_t   *scan_labels[MAX_SCAN];
    lv_obj_t   *scan_rssi_bars[MAX_SCAN];
    lv_obj_t   *time_label;
    /* State */
    bool        connected;
    int         rssi_hist[RSSI_HISTORY];
    int         hist_idx;
    uint32_t    uptime_s;
    uint32_t    tx_bytes;
    uint32_t    rx_bytes;
    int         reconnects;
    scan_entry_t scans[MAX_SCAN];
    int         scan_count;
    uint32_t    tick;
} app_ctx_t;

static app_ctx_t g_ctx;

static void generate_scan_results(app_ctx_t *ctx)
{
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    uint32_t seed = (uint32_t)(snap.bmi270.ax ^ snap.bmi270.gx) + ctx->tick;

    static const char *names[] = {
        "BENTO_Lab", "TESAIoT_Dev", "Office_5G", "Guest_WiFi",
        "IoT_Mesh", "Factory_AP", "Debug_Net", "Camera_Link"
    };

    ctx->scan_count = 4 + (int)(seed % 5);
    if (ctx->scan_count > MAX_SCAN) ctx->scan_count = MAX_SCAN;

    for (int i = 0; i < ctx->scan_count; i++) {
        strncpy(ctx->scans[i].ssid, names[i], 32);
        ctx->scans[i].rssi = -30 - (int)((seed + i * 11) % 50);
        ctx->scans[i].channel = 1 + (int)((seed + i * 7) % 13);
        ctx->scans[i].encrypted = ((seed + i) % 4 != 0);
    }
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    ctx->tick++;
    ctx->uptime_s += REFRESH_MS / 1000;

    ctx->connected = ipc_sensorhub_wifi_connected();

    char buf[64];

    /* Connection status */
    lv_obj_set_style_bg_color(ctx->status_dot,
        ctx->connected ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);
    lv_label_set_text(ctx->status_label,
        ctx->connected ? "Connected" : "Disconnected");
    lv_obj_set_style_text_color(ctx->status_label,
        ctx->connected ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);

    if (ctx->connected) {
        lv_label_set_text(ctx->ssid_label, "BENTO_Lab");
        lv_label_set_text(ctx->ip_label, "192.168.4.1");
        lv_label_set_text(ctx->mac_label, "00:A0:50:E8:4A:01");

        sensorhub_snapshot_t snap;
        ipc_sensorhub_snapshot(&snap);
        int rssi = -40 - (int)(snap.bmi270.ax & 0x1F);
        ctx->rssi_hist[ctx->hist_idx] = rssi;
        ctx->hist_idx = (ctx->hist_idx + 1) % RSSI_HISTORY;

        snprintf(buf, sizeof(buf), "%d dBm", rssi);
        lv_label_set_text(ctx->rssi_label, buf);

        int strength = (rssi + 90) * 100 / 60;
        if (strength < 0) strength = 0;
        if (strength > 100) strength = 100;
        lv_bar_set_value(ctx->rssi_bar, strength, LV_ANIM_ON);

        if (strength > 70) {
            lv_obj_set_style_bg_color(ctx->rssi_bar, UI_COLOR_SUCCESS, LV_PART_INDICATOR);
        } else if (strength > 40) {
            lv_obj_set_style_bg_color(ctx->rssi_bar, UI_COLOR_WARNING, LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(ctx->rssi_bar, UI_COLOR_ERROR, LV_PART_INDICATOR);
        }

        lv_label_set_text(ctx->channel_label, "Channel: 6");

        lv_chart_set_next_value(ctx->chart, ctx->rssi_series, (lv_coord_t)(rssi + 100));

        ctx->tx_bytes += 128 + (snap.bmi270.gx & 0xFF);
        ctx->rx_bytes += 256 + (snap.bmi270.gy & 0xFF);
    } else {
        lv_label_set_text(ctx->ssid_label, "--");
        lv_label_set_text(ctx->ip_label, "--");
        lv_label_set_text(ctx->rssi_label, "-- dBm");
        lv_bar_set_value(ctx->rssi_bar, 0, LV_ANIM_ON);
        lv_chart_set_next_value(ctx->chart, ctx->rssi_series, 0);
    }

    /* Stats */
    snprintf(buf, sizeof(buf), "Uptime: %lus", (unsigned long)ctx->uptime_s);
    lv_label_set_text(ctx->uptime_label, buf);

    if (ctx->tx_bytes > 1048576) {
        snprintf(buf, sizeof(buf), "TX: %.1f MB", (double)ctx->tx_bytes / 1048576.0);
    } else {
        snprintf(buf, sizeof(buf), "TX: %lu KB", (unsigned long)(ctx->tx_bytes / 1024));
    }
    lv_label_set_text(ctx->tx_label, buf);

    if (ctx->rx_bytes > 1048576) {
        snprintf(buf, sizeof(buf), "RX: %.1f MB", (double)ctx->rx_bytes / 1048576.0);
    } else {
        snprintf(buf, sizeof(buf), "RX: %lu KB", (unsigned long)(ctx->rx_bytes / 1024));
    }
    lv_label_set_text(ctx->rx_label, buf);

    snprintf(buf, sizeof(buf), "Reconnects: %d", ctx->reconnects);
    lv_label_set_text(ctx->reconnect_label, buf);

    /* Scan results (every 10 ticks) */
    if (ctx->tick % 5 == 0) {
        generate_scan_results(ctx);
        for (int i = 0; i < MAX_SCAN; i++) {
            if (i < ctx->scan_count) {
                snprintf(buf, sizeof(buf), "%s%s  Ch:%d  %ddBm",
                         ctx->scans[i].encrypted ? LV_SYMBOL_EYE_CLOSE " " : "  ",
                         ctx->scans[i].ssid,
                         ctx->scans[i].channel,
                         ctx->scans[i].rssi);
                lv_label_set_text(ctx->scan_labels[i], buf);

                int s = (ctx->scans[i].rssi + 90) * 100 / 60;
                if (s < 0) s = 0; if (s > 100) s = 100;
                lv_bar_set_value(ctx->scan_rssi_bars[i], s, LV_ANIM_ON);
                lv_obj_clear_flag(ctx->scan_labels[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(ctx->scan_rssi_bars[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(ctx->scan_labels[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ctx->scan_rssi_bars[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    char tbuf[32];
    ipc_sensorhub_get_time_str(tbuf, sizeof(tbuf));
    lv_label_set_text(ctx->time_label, tbuf);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Title */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 780, 40);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hdr, 8, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, LV_SYMBOL_WIFI " Network Dashboard");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    ctx->time_label = lv_label_create(hdr);
    lv_obj_set_style_text_color(ctx->time_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->time_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->time_label, LV_ALIGN_RIGHT_MID, -10, 0);

    /* Connection card */
    lv_obj_t *conn = lv_obj_create(parent);
    lv_obj_set_size(conn, 300, 190);
    lv_obj_set_pos(conn, 10, 50);
    lv_obj_set_style_bg_color(conn, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(conn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(conn, 12, 0);
    lv_obj_set_style_border_width(conn, 1, 0);
    lv_obj_set_style_border_color(conn, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(conn, 12, 0);
    lv_obj_clear_flag(conn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ch = lv_label_create(conn);
    lv_label_set_text(ch, "CONNECTION");
    lv_obj_set_style_text_color(ch, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ch, &lv_font_montserrat_14, 0);

    ctx->status_dot = lv_obj_create(conn);
    lv_obj_set_size(ctx->status_dot, 12, 12);
    lv_obj_set_pos(ctx->status_dot, 0, 24);
    lv_obj_set_style_bg_opa(ctx->status_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ctx->status_dot, 0, 0);
    lv_obj_clear_flag(ctx->status_dot, LV_OBJ_FLAG_SCROLLABLE);

    ctx->status_label = lv_label_create(conn);
    lv_obj_set_style_text_font(ctx->status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->status_label, 20, 22);

    ctx->ssid_label = lv_label_create(conn);
    lv_obj_set_style_text_color(ctx->ssid_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->ssid_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->ssid_label, 0, 46);

    ctx->ip_label = lv_label_create(conn);
    lv_obj_set_style_text_color(ctx->ip_label, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(ctx->ip_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->ip_label, 0, 66);

    ctx->mac_label = lv_label_create(conn);
    lv_obj_set_style_text_color(ctx->mac_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->mac_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->mac_label, 0, 86);

    ctx->channel_label = lv_label_create(conn);
    lv_obj_set_style_text_color(ctx->channel_label, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ctx->channel_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->channel_label, 0, 106);

    /* Signal strength */
    ctx->rssi_label = lv_label_create(conn);
    lv_obj_set_style_text_color(ctx->rssi_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->rssi_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->rssi_label, 0, 130);

    ctx->rssi_bar = lv_bar_create(conn);
    lv_obj_set_size(ctx->rssi_bar, 270, 14);
    lv_obj_set_pos(ctx->rssi_bar, 0, 150);
    lv_bar_set_range(ctx->rssi_bar, 0, 100);
    lv_obj_set_style_bg_color(ctx->rssi_bar, lv_color_hex(0x263238), LV_PART_MAIN);

    /* Signal chart + stats (top right) */
    lv_obj_t *sig_card = lv_obj_create(parent);
    lv_obj_set_size(sig_card, 468, 190);
    lv_obj_set_pos(sig_card, 320, 50);
    lv_obj_set_style_bg_color(sig_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(sig_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(sig_card, 12, 0);
    lv_obj_set_style_border_width(sig_card, 1, 0);
    lv_obj_set_style_border_color(sig_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(sig_card, 8, 0);
    lv_obj_clear_flag(sig_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sh = lv_label_create(sig_card);
    lv_label_set_text(sh, "SIGNAL HISTORY");
    lv_obj_set_style_text_color(sh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(sh, &lv_font_montserrat_14, 0);

    ctx->chart = lv_chart_create(sig_card);
    lv_obj_set_size(ctx->chart, 300, 150);
    lv_obj_set_pos(ctx->chart, 0, 22);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, RSSI_HISTORY);
    lv_chart_set_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_bg_color(ctx->chart, lv_color_hex(0x0d1117), 0);
    lv_obj_set_style_size(ctx->chart, 0, 0, LV_PART_INDICATOR);
    ctx->rssi_series = lv_chart_add_series(ctx->chart, UI_COLOR_SUCCESS,
                                            LV_CHART_AXIS_PRIMARY_Y);

    /* Stats */
    ctx->uptime_label = lv_label_create(sig_card);
    lv_obj_set_style_text_color(ctx->uptime_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->uptime_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->uptime_label, 320, 28);

    ctx->tx_label = lv_label_create(sig_card);
    lv_obj_set_style_text_color(ctx->tx_label, UI_COLOR_BMI270, 0);
    lv_obj_set_style_text_font(ctx->tx_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->tx_label, 320, 52);

    ctx->rx_label = lv_label_create(sig_card);
    lv_obj_set_style_text_color(ctx->rx_label, UI_COLOR_SHT40, 0);
    lv_obj_set_style_text_font(ctx->rx_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->rx_label, 320, 76);

    ctx->reconnect_label = lv_label_create(sig_card);
    lv_obj_set_style_text_color(ctx->reconnect_label, UI_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(ctx->reconnect_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->reconnect_label, 320, 100);

    /* Scan results (bottom) */
    lv_obj_t *scan_card = lv_obj_create(parent);
    lv_obj_set_size(scan_card, 780, 170);
    lv_obj_set_pos(scan_card, 10, 250);
    lv_obj_set_style_bg_color(scan_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(scan_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(scan_card, 12, 0);
    lv_obj_set_style_border_width(scan_card, 1, 0);
    lv_obj_set_style_border_color(scan_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(scan_card, 10, 0);
    lv_obj_clear_flag(scan_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *scth = lv_label_create(scan_card);
    lv_label_set_text(scth, "NEARBY NETWORKS");
    lv_obj_set_style_text_color(scth, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(scth, &lv_font_montserrat_14, 0);

    for (int i = 0; i < MAX_SCAN; i++) {
        int col = i / 4;
        int row = i % 4;
        lv_coord_t sx = col * 390;
        lv_coord_t sy = 22 + row * 34;

        ctx->scan_labels[i] = lv_label_create(scan_card);
        lv_obj_set_style_text_color(ctx->scan_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->scan_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->scan_labels[i], sx, sy);
        lv_label_set_text(ctx->scan_labels[i], "---");

        ctx->scan_rssi_bars[i] = lv_bar_create(scan_card);
        lv_obj_set_size(ctx->scan_rssi_bars[i], 60, 8);
        lv_obj_set_pos(ctx->scan_rssi_bars[i], sx + 310, sy + 5);
        lv_bar_set_range(ctx->scan_rssi_bars[i], 0, 100);
        lv_obj_set_style_bg_color(ctx->scan_rssi_bars[i], lv_color_hex(0x263238), LV_PART_MAIN);
        lv_obj_set_style_bg_color(ctx->scan_rssi_bars[i], UI_COLOR_SUCCESS, LV_PART_INDICATOR);
    }

    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
