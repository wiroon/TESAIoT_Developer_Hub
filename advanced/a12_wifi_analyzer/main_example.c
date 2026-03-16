/**
 * A12 — WiFi Channel Analyzer
 *
 * WiFi channel spectrum visualization with signal strength bars,
 * network list, and channel congestion analysis.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>

#define REFRESH_MS      5000
#define MAX_NETWORKS    16
#define NUM_CHANNELS    13
#define BAR_W           48
#define BAR_MAX_H       200
#define SPECTRUM_Y      60
#define SPECTRUM_X      30

typedef struct {
    char    ssid[33];
    int     channel;
    int     rssi;
    bool    secure;
} net_entry_t;

typedef struct {
    lv_obj_t     *parent;
    lv_obj_t     *ch_bars[NUM_CHANNELS];
    lv_obj_t     *ch_labels[NUM_CHANNELS];
    lv_obj_t     *ch_rssi_labels[NUM_CHANNELS];
    lv_obj_t     *net_list;
    lv_obj_t     *status_label;
    lv_obj_t     *count_label;
    lv_obj_t     *best_ch_label;
    net_entry_t   networks[MAX_NETWORKS];
    int           ch_power[NUM_CHANNELS];
    int           ch_count[NUM_CHANNELS];
    int           net_count;
    int           scan_num;
} app_ctx_t;

static app_ctx_t g_ctx;

static void generate_scan_data(app_ctx_t *ctx)
{
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    uint32_t seed = (uint32_t)(snap.bmi270.ax ^ snap.bmi270.ay ^ snap.bmi270.gx) + ctx->scan_num * 7;

    memset(ctx->ch_power, 0, sizeof(ctx->ch_power));
    memset(ctx->ch_count, 0, sizeof(ctx->ch_count));

    /* Simulate realistic WiFi scan results with variation from sensor data */
    static const char *ssid_pool[] = {
        "BENTO_Lab", "Office_5G", "IoT_Network", "Guest_WiFi",
        "SmartHome_2G", "TESAIoT_Dev", "Factory_AP", "Conference_Room",
        "Sensor_Hub", "Debug_Network", "Production", "Test_Bench",
        "R&D_Secure", "Visitor", "Camera_Net", "Mesh_Node_1"
    };

    ctx->net_count = 6 + (int)(seed % 8);
    if (ctx->net_count > MAX_NETWORKS) ctx->net_count = MAX_NETWORKS;

    for (int i = 0; i < ctx->net_count; i++) {
        strncpy(ctx->networks[i].ssid, ssid_pool[i % 16], 32);
        ctx->networks[i].channel = 1 + (int)((seed + i * 13) % NUM_CHANNELS);
        ctx->networks[i].rssi = -30 - (int)((seed + i * 7) % 50);
        ctx->networks[i].secure = ((seed + i) % 3 != 0);

        int ch = ctx->networks[i].channel - 1;
        if (ch >= 0 && ch < NUM_CHANNELS) {
            /* Track max RSSI per channel */
            if (ctx->ch_power[ch] == 0 || ctx->networks[i].rssi > ctx->ch_power[ch]) {
                ctx->ch_power[ch] = ctx->networks[i].rssi;
            }
            ctx->ch_count[ch]++;
        }
    }
}

static void update_ui(app_ctx_t *ctx)
{
    /* Update channel bars */
    int best_ch = 0;
    int best_score = -9999;

    for (int i = 0; i < NUM_CHANNELS; i++) {
        int bar_h;
        lv_color_t bar_color;

        if (ctx->ch_count[i] == 0) {
            bar_h = 4;
            bar_color = lv_color_hex(0x263238);
        } else {
            /* Convert RSSI to bar height: -30 = max, -90 = min */
            int rssi = ctx->ch_power[i];
            bar_h = (int)((float)(rssi + 90) / 60.0f * BAR_MAX_H);
            if (bar_h < 10) bar_h = 10;
            if (bar_h > BAR_MAX_H) bar_h = BAR_MAX_H;

            if (ctx->ch_count[i] >= 3) bar_color = UI_COLOR_ERROR;
            else if (ctx->ch_count[i] == 2) bar_color = UI_COLOR_WARNING;
            else bar_color = UI_COLOR_SUCCESS;
        }

        lv_obj_set_height(ctx->ch_bars[i], bar_h);
        lv_obj_set_style_bg_color(ctx->ch_bars[i], bar_color, 0);
        lv_obj_align(ctx->ch_bars[i], LV_ALIGN_BOTTOM_LEFT,
                     SPECTRUM_X + i * (BAR_W + 4), 0);

        char rbuf[16];
        if (ctx->ch_count[i] > 0) {
            snprintf(rbuf, sizeof(rbuf), "%d", ctx->ch_power[i]);
        } else {
            strncpy(rbuf, "--", sizeof(rbuf));
        }
        lv_label_set_text(ctx->ch_rssi_labels[i], rbuf);

        /* Score: fewer networks + weaker signals = better */
        int score = -ctx->ch_count[i] * 10;
        if (ctx->ch_power[i] != 0) score += ctx->ch_power[i];
        if (ctx->ch_count[i] == 0) score = 100;
        if (score > best_score) { best_score = score; best_ch = i; }
    }

    /* Update best channel recommendation */
    char bbuf[48];
    snprintf(bbuf, sizeof(bbuf), "Recommended: Channel %d", best_ch + 1);
    lv_label_set_text(ctx->best_ch_label, bbuf);

    /* Update network list */
    char list_text[1024] = "";
    for (int i = 0; i < ctx->net_count && i < 10; i++) {
        char line[96];
        snprintf(line, sizeof(line), "%s%s  Ch:%d  %ddBm\n",
                 ctx->networks[i].secure ? LV_SYMBOL_EYE_CLOSE " " : "",
                 ctx->networks[i].ssid,
                 ctx->networks[i].channel,
                 ctx->networks[i].rssi);
        strncat(list_text, line, sizeof(list_text) - strlen(list_text) - 1);
    }
    lv_label_set_text(ctx->net_list, list_text);

    char cbuf[32];
    snprintf(cbuf, sizeof(cbuf), "Found: %d networks", ctx->net_count);
    lv_label_set_text(ctx->count_label, cbuf);

    ctx->scan_num++;
    snprintf(cbuf, sizeof(cbuf), "Scan #%d", ctx->scan_num);
    lv_label_set_text(ctx->status_label, cbuf);
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    generate_scan_data(ctx);
    update_ui(ctx);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_WIFI " WiFi Channel Analyzer");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, 14, 6);

    ctx->status_label = lv_label_create(parent);
    lv_obj_set_style_text_color(ctx->status_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->status_label, 680, 12);

    /* Spectrum area */
    lv_obj_t *spec_card = lv_obj_create(parent);
    lv_obj_set_size(spec_card, 780, BAR_MAX_H + 50);
    lv_obj_set_pos(spec_card, 10, 34);
    lv_obj_set_style_bg_color(spec_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(spec_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(spec_card, 12, 0);
    lv_obj_set_style_border_width(spec_card, 1, 0);
    lv_obj_set_style_border_color(spec_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(spec_card, 8, 0);
    lv_obj_clear_flag(spec_card, LV_OBJ_FLAG_SCROLLABLE);

    /* dBm scale labels */
    static const char *db_labels[] = {"-30", "-50", "-70", "-90"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *dl = lv_label_create(spec_card);
        lv_label_set_text(dl, db_labels[i]);
        lv_obj_set_style_text_color(dl, lv_color_hex(0x455a64), 0);
        lv_obj_set_style_text_font(dl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(dl, 0, i * (BAR_MAX_H / 3));
    }

    /* Channel bars + labels */
    for (int i = 0; i < NUM_CHANNELS; i++) {
        lv_coord_t bx = SPECTRUM_X + i * (BAR_W + 4);

        ctx->ch_bars[i] = lv_obj_create(spec_card);
        lv_obj_set_size(ctx->ch_bars[i], BAR_W, 4);
        lv_obj_align(ctx->ch_bars[i], LV_ALIGN_BOTTOM_LEFT, bx, 0);
        lv_obj_set_style_bg_color(ctx->ch_bars[i], lv_color_hex(0x263238), 0);
        lv_obj_set_style_bg_opa(ctx->ch_bars[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->ch_bars[i], 4, 0);
        lv_obj_set_style_border_width(ctx->ch_bars[i], 0, 0);
        lv_obj_clear_flag(ctx->ch_bars[i], LV_OBJ_FLAG_SCROLLABLE);

        /* Channel number */
        char chbuf[4];
        snprintf(chbuf, sizeof(chbuf), "%d", i + 1);
        ctx->ch_labels[i] = lv_label_create(spec_card);
        lv_label_set_text(ctx->ch_labels[i], chbuf);
        lv_obj_set_style_text_color(ctx->ch_labels[i], lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(ctx->ch_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_align(ctx->ch_labels[i], LV_ALIGN_BOTTOM_LEFT, bx + BAR_W / 2 - 6, 16);

        /* RSSI value above bar */
        ctx->ch_rssi_labels[i] = lv_label_create(spec_card);
        lv_obj_set_style_text_color(ctx->ch_rssi_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->ch_rssi_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->ch_rssi_labels[i], bx + 6, 0);
        lv_label_set_text(ctx->ch_rssi_labels[i], "--");
    }

    /* Bottom row */
    lv_coord_t bot_y = BAR_MAX_H + 50 + 44;

    /* Network list (left) */
    lv_obj_t *net_card = lv_obj_create(parent);
    lv_obj_set_size(net_card, 480, 140);
    lv_obj_set_pos(net_card, 10, bot_y);
    lv_obj_set_style_bg_color(net_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(net_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(net_card, 12, 0);
    lv_obj_set_style_border_width(net_card, 1, 0);
    lv_obj_set_style_border_color(net_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(net_card, 10, 0);
    lv_obj_clear_flag(net_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *nh = lv_label_create(net_card);
    lv_label_set_text(nh, "DETECTED NETWORKS");
    lv_obj_set_style_text_color(nh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(nh, &lv_font_montserrat_14, 0);

    ctx->net_list = lv_label_create(net_card);
    lv_obj_set_style_text_color(ctx->net_list, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->net_list, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->net_list, 0, 20);
    lv_obj_set_width(ctx->net_list, 460);
    lv_label_set_long_mode(ctx->net_list, LV_LABEL_LONG_CLIP);
    lv_label_set_text(ctx->net_list, "Scanning...");

    /* Info panel (right) */
    lv_obj_t *info_card = lv_obj_create(parent);
    lv_obj_set_size(info_card, 288, 140);
    lv_obj_set_pos(info_card, 500, bot_y);
    lv_obj_set_style_bg_color(info_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(info_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(info_card, 12, 0);
    lv_obj_set_style_border_width(info_card, 1, 0);
    lv_obj_set_style_border_color(info_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(info_card, 10, 0);
    lv_obj_clear_flag(info_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ih = lv_label_create(info_card);
    lv_label_set_text(ih, "ANALYSIS");
    lv_obj_set_style_text_color(ih, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ih, &lv_font_montserrat_14, 0);

    ctx->count_label = lv_label_create(info_card);
    lv_obj_set_style_text_color(ctx->count_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->count_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->count_label, 0, 24);

    ctx->best_ch_label = lv_label_create(info_card);
    lv_obj_set_style_text_color(ctx->best_ch_label, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(ctx->best_ch_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->best_ch_label, 0, 48);

    /* Legend */
    static const struct { const char *text; lv_color_t c; } legend[] = {
        { LV_SYMBOL_OK " 1 network",     {.blue=0x50, .green=0xAF, .red=0x4C} },
        { LV_SYMBOL_OK " 2 networks",    {.blue=0x00, .green=0x98, .red=0xFF} },
        { LV_SYMBOL_OK " 3+ networks",   {.blue=0x00, .green=0x17, .red=0xFF} },
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *ll = lv_label_create(info_card);
        lv_label_set_text(ll, legend[i].text);
        lv_obj_set_style_text_color(ll, legend[i].c, 0);
        lv_obj_set_style_text_font(ll, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ll, 0, 76 + i * 20);
    }

    /* Initial scan */
    generate_scan_data(ctx);
    update_ui(ctx);

    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
