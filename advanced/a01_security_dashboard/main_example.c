/**
 * A01 — Security Dashboard
 *
 * HSM security overview displaying chip identity, lifecycle state,
 * certificate slot status, and hardware security module health.
 * Uses LVGL 9.2 dark-themed cards with color-coded status indicators.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>

/* ---------- layout constants ---------- */
#define CARD_W          240
#define CARD_H          140
#define CARD_GAP        12
#define CARD_RADIUS     12
#define CARD_PAD        14
#define STATUS_DOT_SZ   12
#define TOP_BAR_H       44
#define REFRESH_MS       5000

/* ---------- simulated HSM data (populated via IPC in production) ---------- */
typedef struct {
    char     chip_uid[32];
    char     lifecycle[16];
    bool     health_ok;
    uint32_t boot_count;
    bool     cert_slots[4];     /* slot 0-3 populated */
    char     cert_names[4][20];
    uint32_t rng_entropy;       /* 0-100 quality score */
} hsm_state_t;

typedef struct {
    lv_obj_t *parent;
    lv_obj_t *uid_label;
    lv_obj_t *lifecycle_label;
    lv_obj_t *health_dot;
    lv_obj_t *health_text;
    lv_obj_t *boot_label;
    lv_obj_t *cert_dots[4];
    lv_obj_t *cert_labels[4];
    lv_obj_t *rng_bar;
    lv_obj_t *rng_label;
    lv_obj_t *last_update;
    hsm_state_t hsm;
    uint32_t refresh_count;
} app_ctx_t;

static app_ctx_t g_ctx;

/* ---------- helpers ---------- */
static lv_obj_t *create_card(lv_obj_t *parent, const char *title,
                             lv_coord_t x, lv_coord_t y,
                             lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, CARD_RADIUS, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(card, CARD_PAD, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x90a4ae), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    return card;
}

static lv_obj_t *make_dot(lv_obj_t *parent, lv_color_t color,
                          lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, STATUS_DOT_SZ, STATUS_DOT_SZ);
    lv_obj_set_pos(dot, x, y);
    lv_obj_set_style_bg_color(dot, color, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    return dot;
}

/* ---------- populate HSM state ---------- */
static void refresh_hsm_state(hsm_state_t *h)
{
    /* In production these come from IPC_CMD_HSM_REQUEST / IPC_CMD_HSM_HEALTH.
       Here we show realistic values that the real HSM would return. */
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    snprintf(h->chip_uid, sizeof(h->chip_uid), "E84-%04X-%04X-%04X",
             (unsigned)(snap.bmi270.ax & 0xFFFF),
             (unsigned)(snap.bmi270.ay & 0xFFFF),
             (unsigned)(snap.bmi270.az & 0xFFFF));

    strncpy(h->lifecycle, "OPERATIONAL", sizeof(h->lifecycle));
    h->health_ok   = true;
    h->boot_count  = g_ctx.refresh_count + 142;
    h->rng_entropy = 94 + (snap.bmi270.gx & 0x05);

    h->cert_slots[0] = true;
    h->cert_slots[1] = true;
    h->cert_slots[2] = false;
    h->cert_slots[3] = true;
    strncpy(h->cert_names[0], "Device Identity", 20);
    strncpy(h->cert_names[1], "TLS Client",      20);
    strncpy(h->cert_names[2], "(empty)",          20);
    strncpy(h->cert_names[3], "OTA Signing",      20);
}

/* ---------- update UI ---------- */
static void update_ui(app_ctx_t *ctx)
{
    hsm_state_t *h = &ctx->hsm;

    lv_label_set_text(ctx->uid_label, h->chip_uid);
    lv_label_set_text(ctx->lifecycle_label, h->lifecycle);

    lv_obj_set_style_bg_color(ctx->health_dot,
        h->health_ok ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);
    lv_label_set_text(ctx->health_text,
        h->health_ok ? "All tests passed" : "FAILURE DETECTED");

    char buf[32];
    snprintf(buf, sizeof(buf), "Boot count: %lu", (unsigned long)h->boot_count);
    lv_label_set_text(ctx->boot_label, buf);

    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_bg_color(ctx->cert_dots[i],
            h->cert_slots[i] ? UI_COLOR_SUCCESS : lv_color_hex(0x555555), 0);
        lv_label_set_text(ctx->cert_labels[i], h->cert_names[i]);
    }

    lv_bar_set_value(ctx->rng_bar, (int32_t)h->rng_entropy, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%lu%%", (unsigned long)h->rng_entropy);
    lv_label_set_text(ctx->rng_label, buf);

    char tbuf[32];
    ipc_sensorhub_get_time_str(tbuf, sizeof(tbuf));
    snprintf(buf, sizeof(buf), "Updated: %s", tbuf);
    lv_label_set_text(ctx->last_update, buf);
}

/* ---------- timer callback ---------- */
static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    ctx->refresh_count++;
    refresh_hsm_state(&ctx->hsm);
    update_ui(ctx);
}

/* ---------- entry point ---------- */
void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* ---- top bar ---- */
    lv_obj_t *top = lv_obj_create(parent);
    lv_obj_set_size(top, 780, TOP_BAR_H);
    lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(top, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top, 8, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_label_create(top);
    lv_label_set_text(icon, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_color(icon, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *title = lv_label_create(top);
    lv_label_set_text(title, "Security Dashboard");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 40, 0);

    ctx->last_update = lv_label_create(top);
    lv_obj_set_style_text_color(ctx->last_update, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->last_update, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->last_update, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_label_set_text(ctx->last_update, "Updated: --:--:--");

    /* ---- Card 1: Chip Identity ---- */
    lv_coord_t row1_y = TOP_BAR_H + 14;
    lv_obj_t *c1 = create_card(parent, "CHIP IDENTITY", 10, row1_y, CARD_W, CARD_H);

    lv_obj_t *uid_hdr = lv_label_create(c1);
    lv_label_set_text(uid_hdr, "UID");
    lv_obj_set_style_text_color(uid_hdr, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(uid_hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(uid_hdr, LV_ALIGN_TOP_LEFT, 0, 24);

    ctx->uid_label = lv_label_create(c1);
    lv_obj_set_style_text_color(ctx->uid_label, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(ctx->uid_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->uid_label, LV_ALIGN_TOP_LEFT, 0, 42);
    lv_label_set_text(ctx->uid_label, "---");

    lv_obj_t *lc_hdr = lv_label_create(c1);
    lv_label_set_text(lc_hdr, "Lifecycle");
    lv_obj_set_style_text_color(lc_hdr, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(lc_hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(lc_hdr, LV_ALIGN_TOP_LEFT, 0, 68);

    ctx->lifecycle_label = lv_label_create(c1);
    lv_obj_set_style_text_color(ctx->lifecycle_label, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(ctx->lifecycle_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->lifecycle_label, LV_ALIGN_TOP_LEFT, 0, 86);
    lv_label_set_text(ctx->lifecycle_label, "---");

    /* ---- Card 2: Health Status ---- */
    lv_obj_t *c2 = create_card(parent, "HEALTH STATUS",
                               10 + CARD_W + CARD_GAP, row1_y, CARD_W, CARD_H);

    ctx->health_dot = make_dot(c2, lv_color_hex(0x555555), 0, 30);
    ctx->health_text = lv_label_create(c2);
    lv_obj_set_style_text_color(ctx->health_text, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->health_text, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->health_text, STATUS_DOT_SZ + 8, 30);
    lv_label_set_text(ctx->health_text, "Checking...");

    ctx->boot_label = lv_label_create(c2);
    lv_obj_set_style_text_color(ctx->boot_label, lv_color_hex(0x90a4ae), 0);
    lv_obj_set_style_text_font(ctx->boot_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->boot_label, 0, 56);
    lv_label_set_text(ctx->boot_label, "Boot count: ---");

    /* RNG entropy bar */
    lv_obj_t *rng_hdr = lv_label_create(c2);
    lv_label_set_text(rng_hdr, "RNG Entropy");
    lv_obj_set_style_text_color(rng_hdr, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(rng_hdr, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(rng_hdr, 0, 80);

    ctx->rng_bar = lv_bar_create(c2);
    lv_obj_set_size(ctx->rng_bar, 150, 14);
    lv_obj_set_pos(ctx->rng_bar, 0, 100);
    lv_bar_set_range(ctx->rng_bar, 0, 100);
    lv_obj_set_style_bg_color(ctx->rng_bar, lv_color_hex(0x263238), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ctx->rng_bar, UI_COLOR_PRIMARY, LV_PART_INDICATOR);

    ctx->rng_label = lv_label_create(c2);
    lv_obj_set_style_text_color(ctx->rng_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->rng_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->rng_label, 160, 98);
    lv_label_set_text(ctx->rng_label, "--%");

    /* ---- Card 3: Certificate Slots ---- */
    lv_obj_t *c3 = create_card(parent, "CERTIFICATE SLOTS",
                               10 + 2 * (CARD_W + CARD_GAP), row1_y,
                               780 - 2 * (CARD_W + CARD_GAP) - 10, CARD_H);

    for (int i = 0; i < 4; i++) {
        lv_coord_t cy = 28 + i * 24;
        char slot_str[16];
        snprintf(slot_str, sizeof(slot_str), "Slot %d:", i);

        lv_obj_t *sl = lv_label_create(c3);
        lv_label_set_text(sl, slot_str);
        lv_obj_set_style_text_color(sl, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(sl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(sl, 0, cy);

        ctx->cert_dots[i] = make_dot(c3, lv_color_hex(0x555555), 56, cy + 2);

        ctx->cert_labels[i] = lv_label_create(c3);
        lv_obj_set_style_text_color(ctx->cert_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->cert_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->cert_labels[i], 74, cy);
        lv_label_set_text(ctx->cert_labels[i], "---");
    }

    /* ---- Card 4: Security Features (row 2) ---- */
    lv_coord_t row2_y = row1_y + CARD_H + CARD_GAP;
    lv_obj_t *c4 = create_card(parent, "SECURITY FEATURES", 10, row2_y, 380, 250);

    static const char *features[] = {
        LV_SYMBOL_OK " Secure Boot (eFuse verified)",
        LV_SYMBOL_OK " Hardware Root of Trust",
        LV_SYMBOL_OK " OPTIGA Trust M SLS32AIA",
        LV_SYMBOL_OK " TLS 1.3 mutual authentication",
        LV_SYMBOL_OK " Encrypted firmware updates",
        LV_SYMBOL_OK " Tamper detection (active)",
        LV_SYMBOL_OK " Debug port locked (DAP)",
        LV_SYMBOL_OK " Memory protection (PPC/SAU)",
    };

    for (int i = 0; i < 8; i++) {
        lv_obj_t *fl = lv_label_create(c4);
        lv_label_set_text(fl, features[i]);
        lv_obj_set_style_text_color(fl, UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_text_font(fl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(fl, 0, 24 + i * 26);
    }

    /* ---- Card 5: Crypto Algorithms ---- */
    lv_obj_t *c5 = create_card(parent, "SUPPORTED ALGORITHMS",
                               10 + 380 + CARD_GAP, row2_y, 388, 250);

    static const struct { const char *name; const char *status; lv_color_t clr; } algos[] = {
        { "ECC P-256 (NIST)",    "Hardware",  {.blue=0x50, .green=0xAF, .red=0x4C} },
        { "ECC P-384 (NIST)",    "Hardware",  {.blue=0x50, .green=0xAF, .red=0x4C} },
        { "SHA-256",             "Hardware",  {.blue=0x50, .green=0xAF, .red=0x4C} },
        { "AES-128-CCM",        "Hardware",  {.blue=0x50, .green=0xAF, .red=0x4C} },
        { "HMAC-SHA256",        "Hardware",  {.blue=0x50, .green=0xAF, .red=0x4C} },
        { "TRNG (NIST SP800)",  "Hardware",  {.blue=0x50, .green=0xAF, .red=0x4C} },
        { "RSA-2048",           "Software",  {.blue=0x00, .green=0x98, .red=0xFF} },
        { "X.509 v3 parsing",   "Software",  {.blue=0x00, .green=0x98, .red=0xFF} },
    };

    for (int i = 0; i < 8; i++) {
        lv_obj_t *nl = lv_label_create(c5);
        lv_label_set_text(nl, algos[i].name);
        lv_obj_set_style_text_color(nl, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(nl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(nl, 0, 24 + i * 26);

        lv_obj_t *sl = lv_label_create(c5);
        lv_label_set_text(sl, algos[i].status);
        lv_obj_set_style_text_color(sl, algos[i].clr, 0);
        lv_obj_set_style_text_font(sl, &lv_font_montserrat_14, 0);
        lv_obj_align(sl, LV_ALIGN_TOP_RIGHT, 0, 24 + i * 26);
    }

    /* initial data + start timer */
    refresh_hsm_state(&ctx->hsm);
    update_ui(ctx);
    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
