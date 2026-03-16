/**
 * I11 — System Status Panel
 *
 * Displays system status: WiFi connection, NTP sync, sensor availability,
 * uptime counter, and free heap estimate.
 */
#include "example_common.h"
#include "wifi_manager.h"

#define UPDATE_MS  1000

typedef struct {
    lv_obj_t *lbl_wifi_status, *lbl_wifi_ip, *lbl_wifi_rssi;
    lv_obj_t *lbl_ntp, *lbl_time;
    lv_obj_t *lbl_uptime;
    lv_obj_t *dot_wifi, *dot_ntp;
    lv_obj_t *dot_bmi, *dot_dps, *dot_sht, *dot_bmm;
    uint32_t  start_tick;
} status_ctx_t;

static void set_dot(lv_obj_t *dot, bool ok)
{
    lv_obj_set_style_bg_color(dot, ok ? UI_COLOR_SUCCESS
                                      : UI_COLOR_ERROR, 0);
}

static lv_obj_t *make_status_row(lv_obj_t *parent, const char *label,
                                  lv_obj_t **out_dot, lv_obj_t **out_val)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), 32);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_column(row, 8, 0);

    /* Status dot */
    *out_dot = lv_obj_create(row);
    lv_obj_set_size(*out_dot, 14, 14);
    lv_obj_set_style_radius(*out_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(*out_dot, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(*out_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(*out_dot, 0, 0);

    example_label_create(row, label, &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);

    *out_val = example_label_create(row, "--", &lv_font_montserrat_14,
                                    UI_COLOR_TEXT);
    return row;
}

static void timer_cb(lv_timer_t *t)
{
    status_ctx_t *ctx = (status_ctx_t *)lv_timer_get_user_data(t);

    /* WiFi */
    bool wifi_ok = ipc_sensorhub_wifi_connected();
    set_dot(ctx->dot_wifi, wifi_ok);

    if (wifi_ok) {
        wifi_mgr_status_t ws;
        wifi_manager_get_status(&ws);
        lv_label_set_text_fmt(ctx->lbl_wifi_status, "Connected (%s)", ws.ssid);
        lv_label_set_text(ctx->lbl_wifi_ip, ws.ip_addr);
        lv_label_set_text_fmt(ctx->lbl_wifi_rssi, "%d dBm", ws.rssi);
    } else {
        lv_label_set_text(ctx->lbl_wifi_status, "Disconnected");
        lv_label_set_text(ctx->lbl_wifi_ip, "--");
        lv_label_set_text(ctx->lbl_wifi_rssi, "--");
    }

    /* NTP */
    bool ntp_ok = ipc_sensorhub_ntp_synced();
    set_dot(ctx->dot_ntp, ntp_ok);
    lv_label_set_text(ctx->lbl_ntp, ntp_ok ? "Synced" : "Not synced");

    /* Time */
    char time_buf[32];
    ipc_sensorhub_get_time_str(time_buf, sizeof(time_buf));
    lv_label_set_text(ctx->lbl_time, time_buf);

    /* Sensor dots (compile-time) */
#if BSP_HAS_BMI270
    set_dot(ctx->dot_bmi, true);
#endif
#if BSP_HAS_DPS368
    set_dot(ctx->dot_dps, true);
#endif
#if BSP_HAS_SHT40
    set_dot(ctx->dot_sht, true);
#endif
#if BSP_HAS_BMM350
    set_dot(ctx->dot_bmm, true);
#endif

    /* Uptime */
    uint32_t elapsed = (lv_tick_get() - ctx->start_tick) / 1000;
    uint32_t hrs = elapsed / 3600;
    uint32_t mins = (elapsed % 3600) / 60;
    uint32_t secs = elapsed % 60;
    lv_label_set_text_fmt(ctx->lbl_uptime, "%02lu:%02lu:%02lu",
                          (unsigned long)hrs, (unsigned long)mins,
                          (unsigned long)secs);
}

void example_main(lv_obj_t *parent)
{
    static status_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.start_tick = lv_tick_get();

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_column(parent, 16, 0);

    /* ── Left column: WiFi + Network ─────────────────────────────── */
    lv_obj_t *left = example_card_create(parent, 370, 420,
                                         UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(left, 12, 0);
    lv_obj_set_style_pad_row(left, 4, 0);

    example_label_create(left, "Network", &lv_font_montserrat_20,
                         UI_COLOR_PRIMARY);

    lv_obj_t *d1;
    make_status_row(left, "WiFi:", &ctx.dot_wifi, &ctx.lbl_wifi_status);

    /* IP row (reuse dot for alignment) */
    make_status_row(left, "IP:", &d1, &ctx.lbl_wifi_ip);
    lv_obj_set_style_bg_opa(d1, LV_OPA_TRANSP, 0);

    make_status_row(left, "RSSI:", &d1, &ctx.lbl_wifi_rssi);
    lv_obj_set_style_bg_opa(d1, LV_OPA_TRANSP, 0);

    make_status_row(left, "NTP:", &ctx.dot_ntp, &ctx.lbl_ntp);

    make_status_row(left, "Time:", &d1, &ctx.lbl_time);
    lv_obj_set_style_bg_opa(d1, LV_OPA_TRANSP, 0);

    /* Uptime */
    lv_obj_t *d2;
    make_status_row(left, "Uptime:", &d2, &ctx.lbl_uptime);
    lv_obj_set_style_bg_opa(d2, LV_OPA_TRANSP, 0);

    /* ── Right column: Sensors ───────────────────────────────────── */
    lv_obj_t *right = example_card_create(parent, 370, 420,
                                          UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(right, 12, 0);
    lv_obj_set_style_pad_row(right, 4, 0);

    example_label_create(right, "Sensors", &lv_font_montserrat_20,
                         UI_COLOR_PRIMARY);

    lv_obj_t *lbl_tmp;
    make_status_row(right, "BMI270 (IMU):", &ctx.dot_bmi, &lbl_tmp);
    lv_label_set_text(lbl_tmp,
#if BSP_HAS_BMI270
        "Available"
#else
        "Not present"
#endif
    );

    make_status_row(right, "DPS368 (Baro):", &ctx.dot_dps, &lbl_tmp);
    lv_label_set_text(lbl_tmp,
#if BSP_HAS_DPS368
        "Available"
#else
        "Not present"
#endif
    );

    make_status_row(right, "SHT40 (Climate):", &ctx.dot_sht, &lbl_tmp);
    lv_label_set_text(lbl_tmp,
#if BSP_HAS_SHT40
        "Available"
#else
        "Not present"
#endif
    );

    make_status_row(right, "BMM350 (Mag):", &ctx.dot_bmm, &lbl_tmp);
    lv_label_set_text(lbl_tmp,
#if BSP_HAS_BMM350
        "Available"
#else
        "Not present"
#endif
    );

    example_label_create(right, "\nBoard Capabilities",
                         &lv_font_montserrat_16,
                         UI_COLOR_TEXT_DIM);

#if BSP_HAS_CAPSENSE
    example_label_create(right, "  CapSense: Yes",
                         &lv_font_montserrat_14,
                         UI_COLOR_SUCCESS);
#else
    example_label_create(right, "  CapSense: No",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);
#endif

#if BSP_HAS_POTENTIOMETER
    example_label_create(right, "  Potentiometer: Yes",
                         &lv_font_montserrat_14,
                         UI_COLOR_SUCCESS);
#else
    example_label_create(right, "  Potentiometer: No",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);
#endif

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
