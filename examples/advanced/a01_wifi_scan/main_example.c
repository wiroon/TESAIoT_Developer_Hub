/**
 * @file    main_example.c
 * @brief   WiFi Scan + Table Display
 *
 * @description
 *   Scan WiFi access points using cy_wcm_start_scan and display
 *   results in a sorted LVGL table. Columns: SSID, RSSI, Channel, Security.
 *   Spinner shown during scan. Max 20 APs sorted by RSSI descending.
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
#define SECURITY_STR_LEN    16

/* RSSI thresholds for color coding */
#define RSSI_STRONG         (-50)
#define RSSI_MODERATE       (-70)

/* Colors */
#define COLOR_CARD_BG       lv_color_hex(0x142240)
#define COLOR_TEXT           lv_color_hex(0xE0E0E0)
#define COLOR_RSSI_GOOD     lv_color_hex(0x4CAF50)
#define COLOR_RSSI_MED      lv_color_hex(0xFFEB3B)
#define COLOR_RSSI_WEAK     lv_color_hex(0xF44336)
#define COLOR_HEADER_BG     lv_color_hex(0x1A237E)

/* ---------------------------------------------------------------------------
 * Data Types
 * --------------------------------------------------------------------------- */
typedef struct {
    char     ssid[SSID_MAX_LEN];
    int16_t  rssi;
    uint8_t  channel;
    uint32_t security;
} scan_entry_t;

typedef struct {
    lv_obj_t        *parent;
    lv_obj_t        *table;
    lv_obj_t        *spinner;
    lv_obj_t        *status_label;
    lv_obj_t        *btn_scan;
    scan_entry_t     results[MAX_SCAN_RESULTS];
    uint8_t          count;
    SemaphoreHandle_t scan_done_sem;
    volatile bool    scan_in_progress;
} wifi_scan_ctx_t;

static wifi_scan_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Helper: Security enum to readable string
 * --------------------------------------------------------------------------- */
static const char *security_to_str(uint32_t sec)
{
    if (sec == CY_WCM_SECURITY_OPEN)           return "Open";
    if (sec & CY_WCM_SECURITY_WPA3_WPA2_PSK)   return "WPA3/WPA2";
    if (sec & CY_WCM_SECURITY_WPA3_SAE)         return "WPA3";
    if (sec & CY_WCM_SECURITY_WPA2_AES_PSK)     return "WPA2";
    if (sec & CY_WCM_SECURITY_WPA_TKIP_PSK)     return "WPA";
    if (sec & CY_WCM_SECURITY_WEP_PSK)          return "WEP";
    return "Other";
}

/* ---------------------------------------------------------------------------
 * Insert result into sorted array (descending RSSI), skip duplicates
 * --------------------------------------------------------------------------- */
static void insert_sorted(scan_entry_t *arr, uint8_t *count, const scan_entry_t *entry)
{
    /* Check for duplicate SSID */
    for (uint8_t i = 0; i < *count; i++) {
        if (strncmp(arr[i].ssid, entry->ssid, SSID_MAX_LEN) == 0) {
            /* Keep the one with stronger signal */
            if (entry->rssi > arr[i].rssi) {
                arr[i] = *entry;
            }
            return;
        }
    }

    if (*count >= MAX_SCAN_RESULTS) {
        /* Replace weakest if new entry is stronger */
        if (entry->rssi > arr[*count - 1].rssi) {
            arr[*count - 1] = *entry;
        } else {
            return;
        }
    } else {
        arr[*count] = *entry;
        (*count)++;
    }

    /* Insertion sort: maintain descending RSSI order */
    for (int i = (int)(*count) - 1; i > 0; i--) {
        if (arr[i].rssi > arr[i - 1].rssi) {
            scan_entry_t tmp = arr[i];
            arr[i] = arr[i - 1];
            arr[i - 1] = tmp;
        } else {
            break;
        }
    }
}

/* ---------------------------------------------------------------------------
 * WCM Scan Callback (called from WCM context, NOT LVGL-safe)
 * --------------------------------------------------------------------------- */
static void scan_cb(cy_wcm_scan_result_t *result, void *arg, cy_wcm_scan_status_t status)
{
    wifi_scan_ctx_t *ctx = (wifi_scan_ctx_t *)arg;

    if (status == CY_WCM_SCAN_INCOMPLETE && result != NULL) {
        scan_entry_t entry;
        memset(&entry, 0, sizeof(entry));
        strncpy(entry.ssid, (const char *)result->SSID, SSID_MAX_LEN - 1);
        entry.rssi     = result->signal_strength;
        entry.channel  = result->channel;
        entry.security = result->security;

        /* Skip hidden SSIDs */
        if (entry.ssid[0] != '\0') {
            insert_sorted(ctx->results, &ctx->count, &entry);
        }
    } else if (status == CY_WCM_SCAN_COMPLETE) {
        ctx->scan_in_progress = false;
        xSemaphoreGive(ctx->scan_done_sem);
    }
}

/* ---------------------------------------------------------------------------
 * Populate LVGL table with scan results (must be called from LVGL context)
 * --------------------------------------------------------------------------- */
static void populate_table(wifi_scan_ctx_t *ctx)
{
    lv_table_set_row_count(ctx->table, ctx->count + 1);
    lv_table_set_column_count(ctx->table, 4);

    lv_table_set_column_width(ctx->table, 0, 240);  /* SSID */
    lv_table_set_column_width(ctx->table, 1, 100);  /* RSSI */
    lv_table_set_column_width(ctx->table, 2, 80);   /* Channel */
    lv_table_set_column_width(ctx->table, 3, 120);  /* Security */

    /* Header row */
    lv_table_set_cell_value(ctx->table, 0, 0, "SSID");
    lv_table_set_cell_value(ctx->table, 0, 1, "RSSI (dBm)");
    lv_table_set_cell_value(ctx->table, 0, 2, "Channel");
    lv_table_set_cell_value(ctx->table, 0, 3, "Security");

    /* Data rows */
    char buf[16];
    for (uint8_t i = 0; i < ctx->count; i++) {
        uint16_t row = i + 1;

        lv_table_set_cell_value(ctx->table, row, 0, ctx->results[i].ssid);

        snprintf(buf, sizeof(buf), "%d", ctx->results[i].rssi);
        lv_table_set_cell_value(ctx->table, row, 1, buf);

        snprintf(buf, sizeof(buf), "%u", ctx->results[i].channel);
        lv_table_set_cell_value(ctx->table, row, 2, buf);

        lv_table_set_cell_value(ctx->table, row, 3,
                                security_to_str(ctx->results[i].security));
    }

    /* Update status */
    snprintf(buf, sizeof(buf), "Found: %u APs", ctx->count);
    lv_label_set_text(ctx->status_label, buf);
}

/* ---------------------------------------------------------------------------
 * Scan task: runs scan, waits for completion, updates UI
 * --------------------------------------------------------------------------- */
static void scan_task(void *pvParameters)
{
    wifi_scan_ctx_t *ctx = (wifi_scan_ctx_t *)pvParameters;

    for (;;) {
        /* Wait for scan trigger */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ctx->count = 0;
        ctx->scan_in_progress = true;
        memset(ctx->results, 0, sizeof(ctx->results));

        /* Show spinner */
        lv_lock();
        lv_obj_remove_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ctx->status_label, "Scanning...");
        lv_obj_add_flag(ctx->btn_scan, LV_OBJ_FLAG_HIDDEN);
        lv_unlock();

        /* Initiate scan */
        cy_wcm_scan_filter_t filter;
        memset(&filter, 0, sizeof(filter));

        cy_rslt_t res = cy_wcm_start_scan(scan_cb, ctx, &filter);
        if (res != CY_RSLT_SUCCESS) {
            lv_lock();
            lv_label_set_text(ctx->status_label, "Scan failed!");
            lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ctx->btn_scan, LV_OBJ_FLAG_HIDDEN);
            lv_unlock();
            continue;
        }

        /* Wait for scan complete (timeout 10s) */
        if (xSemaphoreTake(ctx->scan_done_sem, pdMS_TO_TICKS(10000)) != pdTRUE) {
            cy_wcm_stop_scan();
            ctx->scan_in_progress = false;
        }

        /* Update UI with results */
        lv_lock();
        lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ctx->btn_scan, LV_OBJ_FLAG_HIDDEN);
        populate_table(ctx);
        lv_unlock();
    }
}

/* ---------------------------------------------------------------------------
 * Button event: trigger rescan
 * --------------------------------------------------------------------------- */
static TaskHandle_t s_scan_task_handle = NULL;

static void btn_scan_cb(lv_event_t *e)
{
    if (s_scan_task_handle != NULL && !s_ctx.scan_in_progress) {
        xTaskNotifyGive(s_scan_task_handle);
    }
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;
    s_ctx.scan_done_sem = xSemaphoreCreateBinary();

    /* --- Title --- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "WiFi Scanner");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* --- Status label --- */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Ready to scan");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 44);

    /* --- Spinner (hidden initially) --- */
    s_ctx.spinner = lv_spinner_create(parent);
    lv_spinner_set_anim_params(s_ctx.spinner, 1000, 270);
    lv_obj_set_size(s_ctx.spinner, 40, 40);
    lv_obj_align(s_ctx.spinner, LV_ALIGN_TOP_RIGHT, -16, 36);
    lv_obj_add_flag(s_ctx.spinner, LV_OBJ_FLAG_HIDDEN);

    /* --- Table --- */
    s_ctx.table = lv_table_create(parent);
    lv_obj_set_size(s_ctx.table, 560, 280);
    lv_obj_align(s_ctx.table, LV_ALIGN_TOP_MID, 0, 68);
    lv_obj_set_style_bg_color(s_ctx.table, COLOR_CARD_BG, 0);
    lv_obj_set_style_text_color(s_ctx.table, COLOR_TEXT, 0);
    lv_obj_set_style_border_color(s_ctx.table, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(s_ctx.table, 1, 0);
    lv_obj_set_style_radius(s_ctx.table, 8, 0);

    /* Header style */
    lv_obj_set_style_bg_color(s_ctx.table, COLOR_HEADER_BG, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_ctx.table, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);

    /* Set initial column config */
    lv_table_set_column_count(s_ctx.table, 4);
    lv_table_set_row_count(s_ctx.table, 1);
    lv_table_set_column_width(s_ctx.table, 0, 240);
    lv_table_set_column_width(s_ctx.table, 1, 100);
    lv_table_set_column_width(s_ctx.table, 2, 80);
    lv_table_set_column_width(s_ctx.table, 3, 120);
    lv_table_set_cell_value(s_ctx.table, 0, 0, "SSID");
    lv_table_set_cell_value(s_ctx.table, 0, 1, "RSSI (dBm)");
    lv_table_set_cell_value(s_ctx.table, 0, 2, "Channel");
    lv_table_set_cell_value(s_ctx.table, 0, 3, "Security");

    /* --- Scan Button --- */
    s_ctx.btn_scan = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_scan, 140, 44);
    lv_obj_align(s_ctx.btn_scan, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_scan, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(s_ctx.btn_scan, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_scan, btn_scan_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(s_ctx.btn_scan);
    lv_label_set_text(btn_label, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(btn_label);

    /* --- Initialize WCM --- */
    cy_wcm_config_t wcm_cfg = { .interface = CY_WCM_INTERFACE_TYPE_STA };
    cy_rslt_t res = cy_wcm_init(&wcm_cfg);
    if (res != CY_RSLT_SUCCESS) {
        lv_label_set_text(s_ctx.status_label, "WCM init failed!");
        return;
    }

    /* --- Create scan task --- */
    xTaskCreate(scan_task, "wifi_scan", 4096, &s_ctx, 3, &s_scan_task_handle);

    /* Auto-start first scan */
    xTaskNotifyGive(s_scan_task_handle);
}
