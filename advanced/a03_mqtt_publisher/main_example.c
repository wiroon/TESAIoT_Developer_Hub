/**
 * @file    main_example.c
 * @brief   MQTT Sensor Publisher
 *
 * @description
 *   Connect WiFi -> MQTT broker -> publish JSON sensor data every 5s.
 *   UI shows connection status, message count, last published payload.
 *   Topic: tesaiot/{board_id}/sensors, QoS 1, clean session.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "cy_wcm.h"
#include "cy_mqtt_api.h"
#include "sensor_bmi270.h"

/* ---------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------- */
#define MQTT_BROKER_HOST    "broker.hivemq.com"
#define MQTT_BROKER_PORT    1883
#define MQTT_CLIENT_ID      "tesaiot_pse84"
#define MQTT_TOPIC          "tesaiot/pse84/sensors"
#define PUBLISH_INTERVAL_MS 5000
#define JSON_BUF_SIZE       256

#define WIFI_SSID           "YourSSID"
#define WIFI_PASSWORD       "YourPassword"

#define COLOR_BG            lv_color_hex(0x142240)
#define COLOR_TEXT           lv_color_hex(0xE0E0E0)
#define COLOR_SUCCESS       lv_color_hex(0x4CAF50)
#define COLOR_ERROR         lv_color_hex(0xF44336)
#define COLOR_ACCENT        lv_color_hex(0xFF9800)

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *status_label;
    lv_obj_t    *count_label;
    lv_obj_t    *payload_label;
    lv_obj_t    *spinner;
    lv_obj_t    *indicator;
    cy_mqtt_t    mqtt_handle;
    uint32_t     msg_count;
    volatile bool connected;
    volatile bool mqtt_connected;
} mqtt_ctx_t;

static mqtt_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * MQTT event callback
 * --------------------------------------------------------------------------- */
static void mqtt_event_cb(cy_mqtt_t handle, cy_mqtt_event_t event, void *arg)
{
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)arg;

    switch (event.type) {
        case CY_MQTT_EVENT_TYPE_DISCONNECT:
            ctx->mqtt_connected = false;
            break;
        default:
            break;
    }
}

/* ---------------------------------------------------------------------------
 * Connect to WiFi
 * --------------------------------------------------------------------------- */
static cy_rslt_t wifi_connect(void)
{
    cy_wcm_config_t wcm_cfg = { .interface = CY_WCM_INTERFACE_TYPE_STA };
    cy_rslt_t res = cy_wcm_init(&wcm_cfg);
    if (res != CY_RSLT_SUCCESS) return res;

    cy_wcm_connect_params_t params;
    memset(&params, 0, sizeof(params));
    memcpy(params.ap_credentials.SSID, WIFI_SSID, strlen(WIFI_SSID));
    memcpy(params.ap_credentials.password, WIFI_PASSWORD, strlen(WIFI_PASSWORD));
    params.ap_credentials.security = CY_WCM_SECURITY_WPA2_AES_PSK;

    cy_wcm_ip_address_t ip;
    return cy_wcm_connect_ap(&params, &ip);
}

/* ---------------------------------------------------------------------------
 * Connect to MQTT broker
 * --------------------------------------------------------------------------- */
static cy_rslt_t mqtt_connect(mqtt_ctx_t *ctx)
{
    cy_mqtt_connect_info_t info;
    memset(&info, 0, sizeof(info));
    info.client_id      = MQTT_CLIENT_ID;
    info.client_id_len  = strlen(MQTT_CLIENT_ID);
    info.clean_session  = true;
    info.keep_alive_sec = 60;

    cy_mqtt_broker_info_t broker;
    memset(&broker, 0, sizeof(broker));
    broker.hostname     = MQTT_BROKER_HOST;
    broker.hostname_len = strlen(MQTT_BROKER_HOST);
    broker.port         = MQTT_BROKER_PORT;

    cy_rslt_t res = cy_mqtt_create((uint8_t *)ctx, sizeof(*ctx),
                                    NULL, &broker, &info,
                                    &ctx->mqtt_handle);
    if (res != CY_RSLT_SUCCESS) return res;

    res = cy_mqtt_register_event_callback(ctx->mqtt_handle, mqtt_event_cb, ctx);
    if (res != CY_RSLT_SUCCESS) return res;

    res = cy_mqtt_connect(ctx->mqtt_handle, &info);
    return res;
}

/* ---------------------------------------------------------------------------
 * Build JSON payload from sensor data
 * --------------------------------------------------------------------------- */
static int build_json(char *buf, size_t len, uint32_t timestamp)
{
    float ax, ay, az;
    sensor_bmi270_read_accel(&ax, &ay, &az);

    return snprintf(buf, len,
        "{\"accel\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},\"ts\":%lu}",
        ax, ay, az, (unsigned long)timestamp);
}

/* ---------------------------------------------------------------------------
 * Publisher task
 * --------------------------------------------------------------------------- */
static void publisher_task(void *pvParameters)
{
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)pvParameters;
    char json_buf[JSON_BUF_SIZE];

    /* Step 1: Connect WiFi */
    lv_lock();
    lv_label_set_text(ctx->status_label, "Connecting WiFi...");
    lv_unlock();

    cy_rslt_t res = wifi_connect();
    if (res != CY_RSLT_SUCCESS) {
        lv_lock();
        lv_label_set_text(ctx->status_label, "WiFi failed!");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_ERROR, 0);
        lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
        lv_unlock();
        vTaskDelete(NULL);
        return;
    }
    ctx->connected = true;

    /* Step 2: Connect MQTT */
    lv_lock();
    lv_label_set_text(ctx->status_label, "Connecting MQTT...");
    lv_unlock();

    res = mqtt_connect(ctx);
    if (res != CY_RSLT_SUCCESS) {
        lv_lock();
        char err[48];
        snprintf(err, sizeof(err), "MQTT failed: 0x%08lX", (unsigned long)res);
        lv_label_set_text(ctx->status_label, err);
        lv_obj_set_style_text_color(ctx->status_label, COLOR_ERROR, 0);
        lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
        lv_unlock();
        vTaskDelete(NULL);
        return;
    }
    ctx->mqtt_connected = true;

    lv_lock();
    lv_label_set_text(ctx->status_label, "Publishing...");
    lv_obj_set_style_text_color(ctx->status_label, COLOR_SUCCESS, 0);
    lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(ctx->indicator, COLOR_SUCCESS, 0);
    lv_unlock();

    /* Initialize BMI270 */
    sensor_bmi270_init();

    /* Step 3: Publish loop */
    for (;;) {
        if (!ctx->mqtt_connected) {
            /* Attempt reconnect */
            lv_lock();
            lv_label_set_text(ctx->status_label, "Reconnecting...");
            lv_obj_set_style_bg_color(ctx->indicator, COLOR_ERROR, 0);
            lv_unlock();

            res = mqtt_connect(ctx);
            if (res == CY_RSLT_SUCCESS) {
                ctx->mqtt_connected = true;
                lv_lock();
                lv_label_set_text(ctx->status_label, "Publishing...");
                lv_obj_set_style_bg_color(ctx->indicator, COLOR_SUCCESS, 0);
                lv_unlock();
            } else {
                vTaskDelay(pdMS_TO_TICKS(PUBLISH_INTERVAL_MS));
                continue;
            }
        }

        uint32_t ts = xTaskGetTickCount() / configTICK_RATE_HZ;
        int json_len = build_json(json_buf, sizeof(json_buf), ts);

        cy_mqtt_publish_info_t pub;
        memset(&pub, 0, sizeof(pub));
        pub.qos       = CY_MQTT_QOS1;
        pub.topic      = MQTT_TOPIC;
        pub.topic_len  = strlen(MQTT_TOPIC);
        pub.payload    = json_buf;
        pub.payload_len = (size_t)json_len;

        res = cy_mqtt_publish(ctx->mqtt_handle, &pub);
        if (res == CY_RSLT_SUCCESS) {
            ctx->msg_count++;

            lv_lock();
            char count_str[32];
            snprintf(count_str, sizeof(count_str), "Messages: %lu",
                     (unsigned long)ctx->msg_count);
            lv_label_set_text(ctx->count_label, count_str);
            lv_label_set_text(ctx->payload_label, json_buf);
            lv_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(PUBLISH_INTERVAL_MS));
    }
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
    lv_label_set_text(title, "MQTT Sensor Publisher");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* --- Connection indicator (circle) --- */
    s_ctx.indicator = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.indicator, 16, 16);
    lv_obj_set_style_radius(s_ctx.indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_ctx.indicator, COLOR_ERROR, 0);
    lv_obj_set_style_bg_opa(s_ctx.indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ctx.indicator, 0, 0);
    lv_obj_align(s_ctx.indicator, LV_ALIGN_TOP_LEFT, 16, 46);

    /* --- Status label --- */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Initializing...");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 40, 44);

    /* --- Spinner --- */
    s_ctx.spinner = lv_spinner_create(parent);
    lv_spinner_set_anim_params(s_ctx.spinner, 1000, 270);
    lv_obj_set_size(s_ctx.spinner, 36, 36);
    lv_obj_align(s_ctx.spinner, LV_ALIGN_TOP_RIGHT, -16, 36);

    /* --- Info card --- */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 560, 200);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(card, COLOR_BG, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* Topic */
    lv_obj_t *topic_lbl = lv_label_create(card);
    lv_label_set_text(topic_lbl, "Topic: " MQTT_TOPIC);
    lv_obj_set_style_text_color(topic_lbl, COLOR_ACCENT, 0);

    /* Broker */
    lv_obj_t *broker_lbl = lv_label_create(card);
    lv_label_set_text(broker_lbl, "Broker: " MQTT_BROKER_HOST ":" LV_STRINGIFY(MQTT_BROKER_PORT));
    lv_obj_set_style_text_color(broker_lbl, COLOR_TEXT, 0);

    /* Message count */
    s_ctx.count_label = lv_label_create(card);
    lv_label_set_text(s_ctx.count_label, "Messages: 0");
    lv_obj_set_style_text_color(s_ctx.count_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_ctx.count_label, &lv_font_montserrat_20, 0);

    /* Last payload */
    lv_obj_t *payload_title = lv_label_create(card);
    lv_label_set_text(payload_title, "Last payload:");
    lv_obj_set_style_text_color(payload_title, COLOR_TEXT, 0);

    s_ctx.payload_label = lv_label_create(card);
    lv_label_set_text(s_ctx.payload_label, "---");
    lv_obj_set_style_text_color(s_ctx.payload_label, COLOR_SUCCESS, 0);
    lv_label_set_long_mode(s_ctx.payload_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ctx.payload_label, 520);

    /* --- Create publisher task --- */
    xTaskCreate(publisher_task, "mqtt_pub", 8192, &s_ctx, 3, NULL);
}
