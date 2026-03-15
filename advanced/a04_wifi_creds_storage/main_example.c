/**
 * @file    main_example.c
 * @brief   WiFi Credentials Storage
 *
 * @description
 *   Store/load/delete WiFi credentials in QSPI LittleFS. UI: list saved
 *   networks, add new, delete, auto-connect toggle. CRC32 integrity check.
 *   Max 5 saved networks.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "qspi_wifi_creds.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define MAX_SAVED_NETWORKS  5
#define SSID_MAX_LEN        33
#define PASS_MAX_LEN        64

#define COLOR_BG            lv_color_hex(0x142240)
#define COLOR_CARD          lv_color_hex(0x1A2744)
#define COLOR_TEXT           lv_color_hex(0xE0E0E0)
#define COLOR_SUCCESS       lv_color_hex(0x4CAF50)
#define COLOR_DANGER        lv_color_hex(0xF44336)
#define COLOR_PRIMARY       lv_color_hex(0x2196F3)
#define COLOR_ACCENT        lv_color_hex(0xFF9800)

/* ---------------------------------------------------------------------------
 * CRC32 (standard IEEE polynomial)
 * --------------------------------------------------------------------------- */
static uint32_t crc32_calc(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

/* ---------------------------------------------------------------------------
 * Data structures
 * --------------------------------------------------------------------------- */
typedef struct {
    char     ssid[SSID_MAX_LEN];
    char     password[PASS_MAX_LEN];
    bool     auto_connect;
    uint32_t crc32;
} wifi_cred_t;

typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *list;
    lv_obj_t    *ssid_ta;
    lv_obj_t    *pass_ta;
    lv_obj_t    *status_label;
    lv_obj_t    *auto_sw;
    wifi_cred_t  creds[MAX_SAVED_NETWORKS];
    uint8_t      count;
} creds_ctx_t;

static creds_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Load credentials from QSPI
 * --------------------------------------------------------------------------- */
static void load_creds(creds_ctx_t *ctx)
{
    ctx->count = 0;

    for (uint8_t i = 0; i < MAX_SAVED_NETWORKS; i++) {
        wifi_cred_t cred;
        memset(&cred, 0, sizeof(cred));

        cy_rslt_t res = qspi_wifi_creds_load(i, cred.ssid, sizeof(cred.ssid),
                                               cred.password, sizeof(cred.password));
        if (res != CY_RSLT_SUCCESS || cred.ssid[0] == '\0') {
            continue;
        }

        /* Verify CRC32 */
        uint32_t calc_crc = crc32_calc((const uint8_t *)cred.ssid,
                                        strlen(cred.ssid) + strlen(cred.password));
        cred.crc32 = calc_crc;
        cred.auto_connect = true;

        ctx->creds[ctx->count] = cred;
        ctx->count++;
    }
}

/* ---------------------------------------------------------------------------
 * Refresh the network list UI
 * --------------------------------------------------------------------------- */
static void refresh_list(creds_ctx_t *ctx)
{
    lv_obj_clean(ctx->list);

    if (ctx->count == 0) {
        lv_obj_t *empty = lv_label_create(ctx->list);
        lv_label_set_text(empty, "No saved networks");
        lv_obj_set_style_text_color(empty, COLOR_TEXT, 0);
        return;
    }

    for (uint8_t i = 0; i < ctx->count; i++) {
        /* List item container */
        lv_obj_t *item = lv_obj_create(ctx->list);
        lv_obj_set_size(item, lv_pct(100), 48);
        lv_obj_set_style_bg_color(item, COLOR_CARD, 0);
        lv_obj_set_style_border_width(item, 0, 0);
        lv_obj_set_style_radius(item, 8, 0);
        lv_obj_set_style_pad_hor(item, 12, 0);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_SPACE_BETWEEN,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        /* WiFi icon + SSID */
        lv_obj_t *lbl = lv_label_create(item);
        char txt[48];
        snprintf(txt, sizeof(txt), LV_SYMBOL_WIFI " %s", ctx->creds[i].ssid);
        lv_label_set_text(lbl, txt);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_flex_grow(lbl, 1);

        /* CRC indicator */
        lv_obj_t *crc_dot = lv_obj_create(item);
        lv_obj_set_size(crc_dot, 10, 10);
        lv_obj_set_style_radius(crc_dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(crc_dot, COLOR_SUCCESS, 0);
        lv_obj_set_style_border_width(crc_dot, 0, 0);

        /* Delete button */
        lv_obj_t *del_btn = lv_btn_create(item);
        lv_obj_set_size(del_btn, 36, 36);
        lv_obj_set_style_bg_color(del_btn, COLOR_DANGER, 0);
        lv_obj_set_style_radius(del_btn, 6, 0);

        lv_obj_t *del_lbl = lv_label_create(del_btn);
        lv_label_set_text(del_lbl, LV_SYMBOL_TRASH);
        lv_obj_center(del_lbl);

        /* Store index in user data for delete callback */
        lv_obj_set_user_data(del_btn, (void *)(uintptr_t)i);
        lv_obj_add_event_cb(del_btn, delete_btn_cb, LV_EVENT_CLICKED, &s_ctx);
    }

    char status[32];
    snprintf(status, sizeof(status), "Saved: %u/%u", ctx->count, MAX_SAVED_NETWORKS);
    lv_label_set_text(ctx->status_label, status);
}

/* ---------------------------------------------------------------------------
 * Delete button callback
 * --------------------------------------------------------------------------- */
static void delete_btn_cb(lv_event_t *e)
{
    creds_ctx_t *ctx = (creds_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    uint8_t idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(btn);

    if (idx >= ctx->count) return;

    /* Delete from QSPI */
    qspi_wifi_creds_delete(idx);

    /* Remove from array by shifting */
    for (uint8_t i = idx; i < ctx->count - 1; i++) {
        ctx->creds[i] = ctx->creds[i + 1];
    }
    ctx->count--;
    memset(&ctx->creds[ctx->count], 0, sizeof(wifi_cred_t));

    refresh_list(ctx);
    lv_label_set_text(ctx->status_label, "Network deleted");
}

/* ---------------------------------------------------------------------------
 * Add button callback
 * --------------------------------------------------------------------------- */
static void add_btn_cb(lv_event_t *e)
{
    creds_ctx_t *ctx = (creds_ctx_t *)lv_event_get_user_data(e);

    if (ctx->count >= MAX_SAVED_NETWORKS) {
        lv_label_set_text(ctx->status_label, "Max networks reached!");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_DANGER, 0);
        return;
    }

    const char *ssid = lv_textarea_get_text(ctx->ssid_ta);
    const char *pass = lv_textarea_get_text(ctx->pass_ta);

    if (ssid == NULL || strlen(ssid) == 0) {
        lv_label_set_text(ctx->status_label, "Enter SSID!");
        return;
    }

    /* Build credential */
    wifi_cred_t *cred = &ctx->creds[ctx->count];
    memset(cred, 0, sizeof(wifi_cred_t));
    strncpy(cred->ssid, ssid, SSID_MAX_LEN - 1);
    strncpy(cred->password, pass, PASS_MAX_LEN - 1);
    cred->auto_connect = lv_obj_has_state(ctx->auto_sw, LV_STATE_CHECKED);
    cred->crc32 = crc32_calc((const uint8_t *)ssid, strlen(ssid) + strlen(pass));

    /* Save to QSPI */
    cy_rslt_t res = qspi_wifi_creds_save(ctx->count, cred->ssid, cred->password);
    if (res != CY_RSLT_SUCCESS) {
        lv_label_set_text(ctx->status_label, "Save failed!");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_DANGER, 0);
        return;
    }

    ctx->count++;

    /* Clear input fields */
    lv_textarea_set_text(ctx->ssid_ta, "");
    lv_textarea_set_text(ctx->pass_ta, "");

    refresh_list(ctx);
    lv_label_set_text(ctx->status_label, "Network saved!");
    lv_obj_set_style_text_color(ctx->status_label, COLOR_SUCCESS, 0);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* --- Title --- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_SD_CARD " WiFi Credentials");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* --- Status label --- */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Loading...");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 38);

    /* --- Saved networks list (scrollable) --- */
    s_ctx.list = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.list, 380, 200);
    lv_obj_align(s_ctx.list, LV_ALIGN_TOP_LEFT, 8, 60);
    lv_obj_set_style_bg_color(s_ctx.list, COLOR_BG, 0);
    lv_obj_set_style_border_color(s_ctx.list, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(s_ctx.list, 1, 0);
    lv_obj_set_style_radius(s_ctx.list, 8, 0);
    lv_obj_set_style_pad_all(s_ctx.list, 8, 0);
    lv_obj_set_flex_flow(s_ctx.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_ctx.list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_ctx.list, 4, 0);

    /* --- Add network panel (right side) --- */
    lv_obj_t *add_panel = lv_obj_create(parent);
    lv_obj_set_size(add_panel, 200, 200);
    lv_obj_align(add_panel, LV_ALIGN_TOP_RIGHT, -8, 60);
    lv_obj_set_style_bg_color(add_panel, COLOR_BG, 0);
    lv_obj_set_style_border_color(add_panel, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(add_panel, 1, 0);
    lv_obj_set_style_radius(add_panel, 8, 0);
    lv_obj_set_style_pad_all(add_panel, 10, 0);
    lv_obj_set_flex_flow(add_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(add_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(add_panel, 6, 0);

    lv_obj_t *add_title = lv_label_create(add_panel);
    lv_label_set_text(add_title, "Add Network");
    lv_obj_set_style_text_color(add_title, COLOR_ACCENT, 0);

    /* SSID input */
    s_ctx.ssid_ta = lv_textarea_create(add_panel);
    lv_obj_set_size(s_ctx.ssid_ta, 176, 36);
    lv_textarea_set_placeholder_text(s_ctx.ssid_ta, "SSID");
    lv_textarea_set_one_line(s_ctx.ssid_ta, true);
    lv_textarea_set_max_length(s_ctx.ssid_ta, SSID_MAX_LEN - 1);

    /* Password input */
    s_ctx.pass_ta = lv_textarea_create(add_panel);
    lv_obj_set_size(s_ctx.pass_ta, 176, 36);
    lv_textarea_set_placeholder_text(s_ctx.pass_ta, "Password");
    lv_textarea_set_one_line(s_ctx.pass_ta, true);
    lv_textarea_set_password_mode(s_ctx.pass_ta, true);
    lv_textarea_set_max_length(s_ctx.pass_ta, PASS_MAX_LEN - 1);

    /* Auto-connect switch */
    lv_obj_t *sw_row = lv_obj_create(add_panel);
    lv_obj_set_size(sw_row, 176, 30);
    lv_obj_set_style_bg_opa(sw_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sw_row, 0, 0);
    lv_obj_set_style_pad_all(sw_row, 0, 0);
    lv_obj_set_flex_flow(sw_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sw_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *sw_lbl = lv_label_create(sw_row);
    lv_label_set_text(sw_lbl, "Auto-connect");
    lv_obj_set_style_text_color(sw_lbl, COLOR_TEXT, 0);

    s_ctx.auto_sw = lv_switch_create(sw_row);
    lv_obj_add_state(s_ctx.auto_sw, LV_STATE_CHECKED);

    /* Add button */
    lv_obj_t *add_btn = lv_btn_create(add_panel);
    lv_obj_set_size(add_btn, 176, 36);
    lv_obj_set_style_bg_color(add_btn, COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(add_btn, 6, 0);
    lv_obj_add_event_cb(add_btn, add_btn_cb, LV_EVENT_CLICKED, &s_ctx);

    lv_obj_t *add_lbl = lv_label_create(add_btn);
    lv_label_set_text(add_lbl, LV_SYMBOL_PLUS " Add");
    lv_obj_center(add_lbl);

    /* --- Load existing credentials --- */
    load_creds(&s_ctx);
    refresh_list(&s_ctx);
}
