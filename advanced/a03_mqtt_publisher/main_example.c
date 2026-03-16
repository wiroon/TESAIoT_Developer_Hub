/**
 * @file    main_example.c
 * @brief   MQTT Dashboard — Visual publish/subscribe concept demo
 *
 * @description
 *   Visual MQTT dashboard demonstrating pub/sub concepts with LVGL widgets.
 *   Reads live sensor data via ipc_sensorhub_snapshot and displays it as
 *   MQTT-style topic/payload pairs. Simulates broker connection, message
 *   counting, QoS indicator, and a topic/message log — all rendered as
 *   LVGL UI elements.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"

/* ---------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------- */
#define MQTT_BROKER_HOST    "broker.hivemq.com"
#define MQTT_BROKER_PORT    1883
#define MQTT_CLIENT_ID      "tesaiot_pse84"
#define MQTT_TOPIC_ACCEL    "tesaiot/pse84/accel"
#define MQTT_TOPIC_GYRO     "tesaiot/pse84/gyro"
#define MQTT_TOPIC_STATUS   "tesaiot/pse84/status"
#define PUBLISH_INTERVAL_MS 2000
#define MAX_LOG_LINES       8

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *status_label;
    lv_obj_t    *indicator;
    lv_obj_t    *count_label;
    lv_obj_t    *payload_label;
    lv_obj_t    *qos_label;
    lv_obj_t    *log_label;
    lv_obj_t    *topic_ta;
    lv_obj_t    *message_ta;
    lv_timer_t  *publish_timer;
    uint32_t     msg_count;
    bool         connected;
    char         log_buf[512];
} mqtt_ctx_t;

static mqtt_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Append to rolling log
 * --------------------------------------------------------------------------- */
static void log_append(mqtt_ctx_t *ctx, const char *line)
{
    /* Keep only recent lines by shifting buffer */
    size_t line_len = strlen(line);
    size_t buf_len = strlen(ctx->log_buf);

    /* Count existing newlines */
    int nl_count = 0;
    for (size_t i = 0; i < buf_len; i++) {
        if (ctx->log_buf[i] == '\n') nl_count++;
    }

    /* If at max, remove first line */
    while (nl_count >= MAX_LOG_LINES) {
        char *nl = strchr(ctx->log_buf, '\n');
        if (!nl) break;
        size_t remove = (size_t)(nl - ctx->log_buf) + 1;
        memmove(ctx->log_buf, ctx->log_buf + remove, buf_len - remove + 1);
        buf_len -= remove;
        nl_count--;
    }

    /* Append new line */
    if (buf_len + line_len + 2 < sizeof(ctx->log_buf)) {
        if (buf_len > 0) {
            strcat(ctx->log_buf, "\n");
        }
        strcat(ctx->log_buf, line);
    }

    lv_label_set_text(ctx->log_label, ctx->log_buf);
}

/* ---------------------------------------------------------------------------
 * Simulated publish timer: reads sensors and displays as MQTT payload
 * --------------------------------------------------------------------------- */
static void publish_timer_cb(lv_timer_t *timer)
{
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)lv_timer_get_user_data(timer);

    if (!ctx->connected) {
        /* Simulate connection after first tick */
        ctx->connected = true;
        lv_label_set_text(ctx->status_label, "Connected to broker");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_bg_color(ctx->indicator, UI_COLOR_SUCCESS, 0);
        log_append(ctx, "CONNACK received, session established");
        return;
    }

    /* Read sensor snapshot */
    sensorhub_snapshot_t snap;
    if (ipc_sensorhub_snapshot(&snap) != 0) return;

    ctx->msg_count++;

    /* Build JSON payload from accelerometer data */
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f}",
             snap.bmi270.ax, snap.bmi270.ay, snap.bmi270.az);

    /* Update payload display */
    lv_label_set_text(ctx->payload_label, payload);

    /* Update count */
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "Messages: %lu",
             (unsigned long)ctx->msg_count);
    lv_label_set_text(ctx->count_label, count_str);

    /* Add to log */
    char log_line[160];
    snprintf(log_line, sizeof(log_line), "PUB %s %s",
             MQTT_TOPIC_ACCEL, payload);
    log_append(ctx, log_line);
}

/* ---------------------------------------------------------------------------
 * Manual publish button callback
 * --------------------------------------------------------------------------- */
static void btn_publish_cb(lv_event_t *e)
{
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)lv_event_get_user_data(e);

    const char *topic = lv_textarea_get_text(ctx->topic_ta);
    const char *message = lv_textarea_get_text(ctx->message_ta);

    if (!topic || strlen(topic) == 0) {
        log_append(ctx, "ERR: empty topic");
        return;
    }

    ctx->msg_count++;

    char count_str[32];
    snprintf(count_str, sizeof(count_str), "Messages: %lu",
             (unsigned long)ctx->msg_count);
    lv_label_set_text(ctx->count_label, count_str);

    char log_line[160];
    snprintf(log_line, sizeof(log_line), "PUB %s %s", topic, message);
    log_append(ctx, log_line);

    lv_label_set_text(ctx->payload_label, message);
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
    lv_label_set_text(title, "MQTT Dashboard");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* --- Connection indicator --- */
    s_ctx.indicator = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.indicator, 14, 14);
    lv_obj_set_style_radius(s_ctx.indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_ctx.indicator, UI_COLOR_ERROR, 0);
    lv_obj_set_style_bg_opa(s_ctx.indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ctx.indicator, 0, 0);
    lv_obj_align(s_ctx.indicator, LV_ALIGN_TOP_LEFT, 16, 42);
    lv_obj_clear_flag(s_ctx.indicator, LV_OBJ_FLAG_SCROLLABLE);

    /* --- Status --- */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Connecting to broker...");
    lv_obj_set_style_text_color(s_ctx.status_label, UI_COLOR_WARNING, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 38, 38);

    /* --- Info card (left) --- */
    lv_obj_t *info_card = example_card_create(parent, 260, 160, UI_COLOR_CARD_BG);
    lv_obj_align(info_card, LV_ALIGN_TOP_LEFT, 8, 60);

    lv_obj_t *broker_lbl = example_label_create(info_card,
        "Broker: " MQTT_BROKER_HOST, &lv_font_montserrat_14, UI_COLOR_WARNING);
    lv_obj_align(broker_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *topic_lbl = example_label_create(info_card,
        "Topic: " MQTT_TOPIC_ACCEL, &lv_font_montserrat_14, UI_COLOR_PRIMARY);
    lv_obj_align(topic_lbl, LV_ALIGN_TOP_LEFT, 0, 20);

    s_ctx.qos_label = example_label_create(info_card,
        "QoS: 1 (At least once)", &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(s_ctx.qos_label, LV_ALIGN_TOP_LEFT, 0, 40);

    s_ctx.count_label = example_label_create(info_card,
        "Messages: 0", &lv_font_montserrat_20, lv_color_hex(0xFFFFFF));
    lv_obj_align(s_ctx.count_label, LV_ALIGN_TOP_LEFT, 0, 64);

    lv_obj_t *payload_title = example_label_create(info_card,
        "Last payload:", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(payload_title, LV_ALIGN_TOP_LEFT, 0, 92);

    s_ctx.payload_label = lv_label_create(info_card);
    lv_label_set_text(s_ctx.payload_label, "---");
    lv_obj_set_style_text_color(s_ctx.payload_label, UI_COLOR_SUCCESS, 0);
    lv_label_set_long_mode(s_ctx.payload_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ctx.payload_label, 230);
    lv_obj_align(s_ctx.payload_label, LV_ALIGN_TOP_LEFT, 0, 108);

    /* --- Manual publish panel (right) --- */
    lv_obj_t *pub_card = example_card_create(parent, 200, 160, UI_COLOR_CARD_BG);
    lv_obj_align(pub_card, LV_ALIGN_TOP_RIGHT, -8, 60);

    lv_obj_t *pub_title = example_label_create(pub_card,
        "Manual Publish", &lv_font_montserrat_14, UI_COLOR_PRIMARY);
    lv_obj_align(pub_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.topic_ta = lv_textarea_create(pub_card);
    lv_obj_set_size(s_ctx.topic_ta, 172, 32);
    lv_obj_align(s_ctx.topic_ta, LV_ALIGN_TOP_LEFT, 0, 22);
    lv_textarea_set_placeholder_text(s_ctx.topic_ta, "Topic");
    lv_textarea_set_one_line(s_ctx.topic_ta, true);
    lv_textarea_set_text(s_ctx.topic_ta, MQTT_TOPIC_STATUS);

    s_ctx.message_ta = lv_textarea_create(pub_card);
    lv_obj_set_size(s_ctx.message_ta, 172, 32);
    lv_obj_align(s_ctx.message_ta, LV_ALIGN_TOP_LEFT, 0, 60);
    lv_textarea_set_placeholder_text(s_ctx.message_ta, "Message");
    lv_textarea_set_one_line(s_ctx.message_ta, true);
    lv_textarea_set_text(s_ctx.message_ta, "{\"status\":\"ok\"}");

    lv_obj_t *pub_btn = lv_btn_create(pub_card);
    lv_obj_set_size(pub_btn, 172, 36);
    lv_obj_align(pub_btn, LV_ALIGN_TOP_LEFT, 0, 100);
    lv_obj_set_style_bg_color(pub_btn, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(pub_btn, 6, 0);
    lv_obj_add_event_cb(pub_btn, btn_publish_cb, LV_EVENT_CLICKED, &s_ctx);
    lv_obj_t *pub_lbl = lv_label_create(pub_btn);
    lv_label_set_text(pub_lbl, LV_SYMBOL_UPLOAD " Publish");
    lv_obj_center(pub_lbl);

    /* --- Message log (bottom) --- */
    lv_obj_t *log_card = example_card_create(parent, 560, 140, lv_color_hex(0x0A1628));
    lv_obj_align(log_card, LV_ALIGN_BOTTOM_MID, 0, -4);

    lv_obj_t *log_title = example_label_create(log_card,
        "Message Log", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(log_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.log_label = lv_label_create(log_card);
    lv_label_set_text(s_ctx.log_label, "Waiting for connection...");
    lv_obj_set_style_text_color(s_ctx.log_label, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(s_ctx.log_label, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(s_ctx.log_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ctx.log_label, 530);
    lv_obj_align(s_ctx.log_label, LV_ALIGN_TOP_LEFT, 0, 18);

    /* --- Start publish timer --- */
    s_ctx.publish_timer = lv_timer_create(publish_timer_cb, PUBLISH_INTERVAL_MS, &s_ctx);
}
