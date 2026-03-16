/**
 * @file    main_example.c
 * @brief   WiFi Connect Flow
 *
 * @description
 *   Full WiFi STA flow: scan -> select AP from dropdown -> password textarea
 *   with keyboard -> connect with status spinner -> show IP address.
 *   State machine: IDLE -> SCANNING -> SELECTING -> CONNECTING -> CONNECTED -> ERROR
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define MAX_SCAN_RESULTS    20
#define SSID_MAX_LEN        33
#define PASSWORD_MAX_LEN    64
#define CONNECT_TIMEOUT_MS  15000

#define COLOR_BG            lv_color_hex(0x142240)
#define COLOR_TEXT           lv_color_hex(0xE0E0E0)
#define COLOR_SUCCESS       lv_color_hex(0x4CAF50)
#define COLOR_ERROR         lv_color_hex(0xF44336)
#define COLOR_PRIMARY       lv_color_hex(0x2196F3)

/* ---------------------------------------------------------------------------
 * State Machine
 * --------------------------------------------------------------------------- */
typedef enum {
    STATE_IDLE,
    STATE_SCANNING,
    STATE_SELECTING,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_ERROR,
} connect_state_t;

typedef struct {
    char     ssid[SSID_MAX_LEN];
    int16_t  rssi;
    uint32_t security;
} ap_entry_t;

typedef struct {
    lv_obj_t          *parent;
    lv_obj_t          *status_label;
    lv_obj_t          *ip_label;
    lv_obj_t          *spinner;
    lv_obj_t          *dropdown;
    lv_obj_t          *textarea;
    lv_obj_t          *keyboard;
    lv_obj_t          *btn_scan;
    lv_obj_t          *btn_connect;
    ap_entry_t         aps[MAX_SCAN_RESULTS];
    uint8_t            ap_count;
    connect_state_t    state;
    SemaphoreHandle_t  scan_done;
    TaskHandle_t       task_handle;
} wifi_connect_ctx_t;

static wifi_connect_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * State transition: update UI visibility
 * --------------------------------------------------------------------------- */
static void set_state(wifi_connect_ctx_t *ctx, connect_state_t new_state)
{
    ctx->state = new_state;

    bool show_spinner  = (new_state == STATE_SCANNING || new_state == STATE_CONNECTING);
    bool show_dropdown = (new_state == STATE_SELECTING || new_state == STATE_CONNECTING);
    bool show_password = (new_state == STATE_SELECTING);
    bool show_keyboard = (new_state == STATE_SELECTING);
    bool show_connect  = (new_state == STATE_SELECTING);
    bool show_scan     = (new_state == STATE_IDLE || new_state == STATE_CONNECTED ||
                          new_state == STATE_ERROR);
    bool show_ip       = (new_state == STATE_CONNECTED);

    if (show_spinner)  lv_obj_remove_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);

    if (show_dropdown) lv_obj_remove_flag(ctx->dropdown, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->dropdown, LV_OBJ_FLAG_HIDDEN);

    if (show_password) lv_obj_remove_flag(ctx->textarea, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->textarea, LV_OBJ_FLAG_HIDDEN);

    if (show_keyboard) lv_obj_remove_flag(ctx->keyboard, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->keyboard, LV_OBJ_FLAG_HIDDEN);

    if (show_connect)  lv_obj_remove_flag(ctx->btn_connect, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->btn_connect, LV_OBJ_FLAG_HIDDEN);

    if (show_scan)     lv_obj_remove_flag(ctx->btn_scan, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->btn_scan, LV_OBJ_FLAG_HIDDEN);

    if (show_ip)       lv_obj_remove_flag(ctx->ip_label, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->ip_label, LV_OBJ_FLAG_HIDDEN);
}

/* ---------------------------------------------------------------------------
 * Scan callback
 * --------------------------------------------------------------------------- */
static void scan_cb(cy_wcm_scan_result_t *result, void *arg, cy_wcm_scan_status_t status)
{
    wifi_connect_ctx_t *ctx = (wifi_connect_ctx_t *)arg;

    if (status == CY_WCM_SCAN_INCOMPLETE && result != NULL) {
        if (result->SSID[0] == '\0' || ctx->ap_count >= MAX_SCAN_RESULTS) return;

        /* Skip duplicates */
        for (uint8_t i = 0; i < ctx->ap_count; i++) {
            if (strncmp(ctx->aps[i].ssid, (const char *)result->SSID, SSID_MAX_LEN) == 0) {
                return;
            }
        }

        ap_entry_t *ap = &ctx->aps[ctx->ap_count];
        strncpy(ap->ssid, (const char *)result->SSID, SSID_MAX_LEN - 1);
        ap->rssi     = result->signal_strength;
        ap->security = result->security;
        ctx->ap_count++;
    } else if (status == CY_WCM_SCAN_COMPLETE) {
        xSemaphoreGive(ctx->scan_done);
    }
}

/* ---------------------------------------------------------------------------
 * WiFi task: handles scan and connect operations
 * --------------------------------------------------------------------------- */
static void wifi_task(void *pvParameters)
{
    wifi_connect_ctx_t *ctx = (wifi_connect_ctx_t *)pvParameters;

    for (;;) {
        uint32_t cmd = 0;
        xTaskNotifyWait(0, UINT32_MAX, &cmd, portMAX_DELAY);

        if (cmd == 1) {
            /* --- SCAN --- */
            ctx->ap_count = 0;

            lv_lock();
            set_state(ctx, STATE_SCANNING);
            lv_label_set_text(ctx->status_label, "Scanning networks...");
            lv_unlock();

            cy_wcm_scan_filter_t filter;
            memset(&filter, 0, sizeof(filter));
            cy_rslt_t res = cy_wcm_start_scan(scan_cb, ctx, &filter);

            if (res != CY_RSLT_SUCCESS) {
                lv_lock();
                lv_label_set_text(ctx->status_label, "Scan failed");
                set_state(ctx, STATE_ERROR);
                lv_unlock();
                continue;
            }

            xSemaphoreTake(ctx->scan_done, pdMS_TO_TICKS(10000));

            /* Populate dropdown */
            lv_lock();
            lv_dropdown_clear_options(ctx->dropdown);
            for (uint8_t i = 0; i < ctx->ap_count; i++) {
                char opt[48];
                snprintf(opt, sizeof(opt), "%s (%d dBm)", ctx->aps[i].ssid, ctx->aps[i].rssi);
                lv_dropdown_add_option(ctx->dropdown, opt, i);
            }
            if (ctx->ap_count > 0) {
                char status_buf[32];
                snprintf(status_buf, sizeof(status_buf), "Found %u networks", ctx->ap_count);
                lv_label_set_text(ctx->status_label, status_buf);
                set_state(ctx, STATE_SELECTING);
            } else {
                lv_label_set_text(ctx->status_label, "No networks found");
                set_state(ctx, STATE_ERROR);
            }
            lv_unlock();

        } else if (cmd == 2) {
            /* --- CONNECT --- */
            lv_lock();
            set_state(ctx, STATE_CONNECTING);
            lv_label_set_text(ctx->status_label, "Connecting...");

            uint32_t sel = lv_dropdown_get_selected(ctx->dropdown);
            const char *password = lv_textarea_get_text(ctx->textarea);
            lv_unlock();

            if (sel >= ctx->ap_count) continue;

            cy_wcm_connect_params_t params;
            memset(&params, 0, sizeof(params));
            memcpy(params.ap_credentials.SSID, ctx->aps[sel].ssid,
                   strlen(ctx->aps[sel].ssid));
            memcpy(params.ap_credentials.password, password,
                   strlen(password));
            params.ap_credentials.security = ctx->aps[sel].security;

            cy_wcm_ip_address_t ip;
            cy_rslt_t res = cy_wcm_connect_ap(&params, &ip);

            lv_lock();
            if (res == CY_RSLT_SUCCESS) {
                char ip_str[48];
                snprintf(ip_str, sizeof(ip_str), "IP: %u.%u.%u.%u",
                         (uint8_t)(ip.ip.v4),
                         (uint8_t)(ip.ip.v4 >> 8),
                         (uint8_t)(ip.ip.v4 >> 16),
                         (uint8_t)(ip.ip.v4 >> 24));
                lv_label_set_text(ctx->ip_label, ip_str);
                lv_obj_set_style_text_color(ctx->ip_label, COLOR_SUCCESS, 0);
                lv_label_set_text(ctx->status_label, "Connected!");
                lv_obj_set_style_text_color(ctx->status_label, COLOR_SUCCESS, 0);
                set_state(ctx, STATE_CONNECTED);
            } else {
                char err[48];
                snprintf(err, sizeof(err), "Connect failed: 0x%08lX", (unsigned long)res);
                lv_label_set_text(ctx->status_label, err);
                lv_obj_set_style_text_color(ctx->status_label, COLOR_ERROR, 0);
                set_state(ctx, STATE_ERROR);
            }
            lv_unlock();
        }
    }
}

/* ---------------------------------------------------------------------------
 * Button callbacks
 * --------------------------------------------------------------------------- */
static void btn_scan_clicked(lv_event_t *e)
{
    xTaskNotify(s_ctx.task_handle, 1, eSetValueWithOverwrite);
}

static void btn_connect_clicked(lv_event_t *e)
{
    xTaskNotify(s_ctx.task_handle, 2, eSetValueWithOverwrite);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;
    s_ctx.scan_done = xSemaphoreCreateBinary();

    /* --- Title --- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_WIFI " WiFi Connect");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* --- Status label --- */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Scan to find networks");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 38);

    /* --- Spinner --- */
    s_ctx.spinner = lv_spinner_create(parent);
    lv_spinner_set_anim_params(s_ctx.spinner, 1000, 270);
    lv_obj_set_size(s_ctx.spinner, 36, 36);
    lv_obj_align(s_ctx.spinner, LV_ALIGN_TOP_RIGHT, -16, 32);
    lv_obj_add_flag(s_ctx.spinner, LV_OBJ_FLAG_HIDDEN);

    /* --- Dropdown (network list) --- */
    s_ctx.dropdown = lv_dropdown_create(parent);
    lv_obj_set_width(s_ctx.dropdown, 400);
    lv_obj_align(s_ctx.dropdown, LV_ALIGN_TOP_MID, 0, 64);
    lv_dropdown_set_text(s_ctx.dropdown, "Select Network");
    lv_obj_add_flag(s_ctx.dropdown, LV_OBJ_FLAG_HIDDEN);

    /* --- Password textarea --- */
    s_ctx.textarea = lv_textarea_create(parent);
    lv_obj_set_size(s_ctx.textarea, 400, 44);
    lv_obj_align(s_ctx.textarea, LV_ALIGN_TOP_MID, 0, 110);
    lv_textarea_set_placeholder_text(s_ctx.textarea, "Enter password");
    lv_textarea_set_password_mode(s_ctx.textarea, true);
    lv_textarea_set_max_length(s_ctx.textarea, PASSWORD_MAX_LEN);
    lv_textarea_set_one_line(s_ctx.textarea, true);
    lv_obj_add_flag(s_ctx.textarea, LV_OBJ_FLAG_HIDDEN);

    /* --- Keyboard --- */
    s_ctx.keyboard = lv_keyboard_create(parent);
    lv_obj_set_size(s_ctx.keyboard, 600, 160);
    lv_obj_align(s_ctx.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(s_ctx.keyboard, s_ctx.textarea);
    lv_obj_add_flag(s_ctx.keyboard, LV_OBJ_FLAG_HIDDEN);

    /* --- IP address label --- */
    s_ctx.ip_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.ip_label, "");
    lv_obj_set_style_text_font(s_ctx.ip_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_ctx.ip_label, COLOR_SUCCESS, 0);
    lv_obj_align(s_ctx.ip_label, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_flag(s_ctx.ip_label, LV_OBJ_FLAG_HIDDEN);

    /* --- Scan button --- */
    s_ctx.btn_scan = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_scan, 140, 44);
    lv_obj_align(s_ctx.btn_scan, LV_ALIGN_BOTTOM_MID, -80, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_scan, COLOR_PRIMARY, 0);
    lv_obj_add_event_cb(s_ctx.btn_scan, btn_scan_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl1 = lv_label_create(s_ctx.btn_scan);
    lv_label_set_text(lbl1, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(lbl1);

    /* --- Connect button --- */
    s_ctx.btn_connect = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_connect, 140, 44);
    lv_obj_align(s_ctx.btn_connect, LV_ALIGN_BOTTOM_MID, 80, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_connect, COLOR_SUCCESS, 0);
    lv_obj_add_event_cb(s_ctx.btn_connect, btn_connect_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl2 = lv_label_create(s_ctx.btn_connect);
    lv_label_set_text(lbl2, LV_SYMBOL_OK " Connect");
    lv_obj_center(lbl2);
    lv_obj_add_flag(s_ctx.btn_connect, LV_OBJ_FLAG_HIDDEN);

    /* --- Initialize WCM --- */
    cy_wcm_config_t wcm_cfg = { .interface = CY_WCM_INTERFACE_TYPE_STA };
    cy_rslt_t res = cy_wcm_init(&wcm_cfg);
    if (res != CY_RSLT_SUCCESS) {
        lv_label_set_text(s_ctx.status_label, "WCM init failed!");
        return;
    }

    /* --- Create WiFi task --- */
    xTaskCreate(wifi_task, "wifi_connect", 4096, &s_ctx, 3, &s_ctx.task_handle);
}
