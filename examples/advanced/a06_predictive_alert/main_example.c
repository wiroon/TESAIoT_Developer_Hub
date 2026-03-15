/**
 * A06 — Predictive Alert Engine
 *
 * Analyzes sensor trends over a sliding window, predicts threshold
 * crossings, and displays color-coded alerts with countdown timers.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define REFRESH_MS      200
#define HISTORY_LEN     60
#define NUM_CHANNELS    4
#define CARD_W          380
#define CARD_H          190

typedef enum {
    ALERT_NONE = 0,
    ALERT_INFO,
    ALERT_WARNING,
    ALERT_CRITICAL,
} alert_level_t;

typedef struct {
    const char  *name;
    const char  *unit;
    float        history[HISTORY_LEN];
    int          head;
    bool         full;
    float        threshold_hi;
    float        threshold_lo;
    float        current;
    float        trend_rate;        /* units per second */
    float        predicted_cross;   /* seconds until threshold cross, -1 = none */
    alert_level_t alert;
    lv_color_t   color;
} channel_t;

typedef struct {
    lv_obj_t   *parent;
    channel_t   ch[NUM_CHANNELS];
    lv_obj_t   *value_labels[NUM_CHANNELS];
    lv_obj_t   *trend_labels[NUM_CHANNELS];
    lv_obj_t   *predict_labels[NUM_CHANNELS];
    lv_obj_t   *alert_dots[NUM_CHANNELS];
    lv_obj_t   *chart;
    lv_chart_series_t *series[NUM_CHANNELS];
    lv_obj_t   *alert_log;
    int          alert_log_count;
    char         log_text[512];
} app_ctx_t;

static app_ctx_t g_ctx;

static void push_sample(channel_t *ch, float val)
{
    ch->history[ch->head] = val;
    ch->head = (ch->head + 1) % HISTORY_LEN;
    if (ch->head == 0) ch->full = true;
    ch->current = val;
}

static void compute_trend(channel_t *ch)
{
    int count = ch->full ? HISTORY_LEN : ch->head;
    if (count < 10) { ch->trend_rate = 0; ch->predicted_cross = -1; return; }

    /* Linear regression over last 20 samples */
    int n = (count > 20) ? 20 : count;
    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
    for (int i = 0; i < n; i++) {
        int idx = ((ch->head - n + i) + HISTORY_LEN) % HISTORY_LEN;
        float x = (float)i;
        float y = ch->history[idx];
        sum_x += x; sum_y += y; sum_xy += x * y; sum_xx += x * x;
    }
    float denom = (float)n * sum_xx - sum_x * sum_x;
    if (fabsf(denom) < 1e-6f) { ch->trend_rate = 0; ch->predicted_cross = -1; return; }

    float slope = ((float)n * sum_xy - sum_x * sum_y) / denom;
    ch->trend_rate = slope / ((float)REFRESH_MS / 1000.0f);

    /* Predict crossing */
    ch->predicted_cross = -1;
    if (ch->trend_rate > 0.001f && ch->current < ch->threshold_hi) {
        ch->predicted_cross = (ch->threshold_hi - ch->current) / ch->trend_rate;
    } else if (ch->trend_rate < -0.001f && ch->current > ch->threshold_lo) {
        ch->predicted_cross = (ch->threshold_lo - ch->current) / ch->trend_rate;
    }

    /* Determine alert level */
    if (ch->current > ch->threshold_hi || ch->current < ch->threshold_lo) {
        ch->alert = ALERT_CRITICAL;
    } else if (ch->predicted_cross > 0 && ch->predicted_cross < 10.0f) {
        ch->alert = ALERT_WARNING;
    } else if (ch->predicted_cross > 0 && ch->predicted_cross < 30.0f) {
        ch->alert = ALERT_INFO;
    } else {
        ch->alert = ALERT_NONE;
    }
}

static lv_color_t alert_color(alert_level_t a)
{
    switch (a) {
    case ALERT_CRITICAL: return UI_COLOR_ERROR;
    case ALERT_WARNING:  return UI_COLOR_WARNING;
    case ALERT_INFO:     return UI_COLOR_INFO;
    default:             return UI_COLOR_SUCCESS;
    }
}

static const char *alert_text(alert_level_t a)
{
    switch (a) {
    case ALERT_CRITICAL: return "CRITICAL";
    case ALERT_WARNING:  return "WARNING";
    case ALERT_INFO:     return "INFO";
    default:             return "NORMAL";
    }
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    /* Sample channels */
#if BSP_HAS_BMI270
    push_sample(&ctx->ch[0], (float)snap.bmi270.ax / 16384.0f);
    push_sample(&ctx->ch[1], (float)snap.bmi270.ay / 16384.0f);
#else
    push_sample(&ctx->ch[0], 0.0f);
    push_sample(&ctx->ch[1], 0.0f);
#endif

#if BSP_HAS_DPS368
    push_sample(&ctx->ch[2], (float)snap.dps368.pressure_x100 / 100.0f);
#else
    push_sample(&ctx->ch[2], 1013.25f);
#endif

#if BSP_HAS_SHT40
    push_sample(&ctx->ch[3], (float)snap.dps368.temperature_x100 / 100.0f);
#else
    push_sample(&ctx->ch[3], 25.0f);
#endif

    /* Compute trends and predictions */
    for (int i = 0; i < NUM_CHANNELS; i++) {
        compute_trend(&ctx->ch[i]);

        /* Update chart */
        lv_chart_set_next_value(ctx->chart, ctx->series[i],
            (lv_coord_t)(ctx->ch[i].current * 100.0f));

        /* Update labels */
        char buf[48];
        snprintf(buf, sizeof(buf), "%s: %.2f %s",
                 ctx->ch[i].name, (double)ctx->ch[i].current, ctx->ch[i].unit);
        lv_label_set_text(ctx->value_labels[i], buf);

        /* Trend arrow */
        const char *arrow = (ctx->ch[i].trend_rate > 0.01f) ? LV_SYMBOL_UP :
                            (ctx->ch[i].trend_rate < -0.01f) ? LV_SYMBOL_DOWN : "=";
        snprintf(buf, sizeof(buf), "%s %+.3f/s", arrow, (double)ctx->ch[i].trend_rate);
        lv_label_set_text(ctx->trend_labels[i], buf);
        lv_obj_set_style_text_color(ctx->trend_labels[i],
            ctx->ch[i].trend_rate > 0.01f ? UI_COLOR_ERROR :
            ctx->ch[i].trend_rate < -0.01f ? UI_COLOR_INFO : UI_COLOR_TEXT, 0);

        /* Prediction */
        if (ctx->ch[i].predicted_cross > 0) {
            snprintf(buf, sizeof(buf), "Cross in %.1fs", (double)ctx->ch[i].predicted_cross);
        } else {
            snprintf(buf, sizeof(buf), "Stable");
        }
        lv_label_set_text(ctx->predict_labels[i], buf);
        lv_obj_set_style_text_color(ctx->predict_labels[i],
            alert_color(ctx->ch[i].alert), 0);

        /* Alert dot */
        lv_obj_set_style_bg_color(ctx->alert_dots[i],
            alert_color(ctx->ch[i].alert), 0);
    }

    /* Update alert log */
    ctx->log_text[0] = '\0';
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (ctx->ch[i].alert >= ALERT_WARNING) {
            char line[64];
            snprintf(line, sizeof(line), "[%s] %s: %s (%.1fs)\n",
                     alert_text(ctx->ch[i].alert), ctx->ch[i].name,
                     ctx->ch[i].alert == ALERT_CRITICAL ? "THRESHOLD EXCEEDED" : "APPROACHING",
                     (double)(ctx->ch[i].predicted_cross > 0 ? ctx->ch[i].predicted_cross : 0));
            strncat(ctx->log_text, line, sizeof(ctx->log_text) - strlen(ctx->log_text) - 1);
        }
    }
    if (ctx->log_text[0] == '\0') {
        strncpy(ctx->log_text, "All channels within normal range.", sizeof(ctx->log_text));
    }
    lv_label_set_text(ctx->alert_log, ctx->log_text);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    /* Initialize channels */
    ctx->ch[0] = (channel_t){ .name = "Accel X",  .unit = "g",   .threshold_hi = 1.5f,  .threshold_lo = -1.5f, .color = UI_COLOR_BMI270 };
    ctx->ch[1] = (channel_t){ .name = "Accel Y",  .unit = "g",   .threshold_hi = 1.5f,  .threshold_lo = -1.5f, .color = UI_COLOR_DPS368 };
    ctx->ch[2] = (channel_t){ .name = "Pressure", .unit = "hPa", .threshold_hi = 1025.0f, .threshold_lo = 1000.0f, .color = UI_COLOR_SHT40 };
    ctx->ch[3] = (channel_t){ .name = "Temp",     .unit = "C",   .threshold_hi = 40.0f,  .threshold_lo = 15.0f,  .color = UI_COLOR_BMM350 };

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_WARNING " Predictive Alert Engine");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, 14, 6);

    /* Chart card (left) */
    lv_obj_t *chart_card = lv_obj_create(parent);
    lv_obj_set_size(chart_card, CARD_W, CARD_H + 50);
    lv_obj_set_pos(chart_card, 10, 36);
    lv_obj_set_style_bg_color(chart_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(chart_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(chart_card, 12, 0);
    lv_obj_set_style_border_width(chart_card, 1, 0);
    lv_obj_set_style_border_color(chart_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(chart_card, 10, 0);
    lv_obj_clear_flag(chart_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->chart = lv_chart_create(chart_card);
    lv_obj_set_size(ctx->chart, 350, 190);
    lv_obj_align(ctx->chart, LV_ALIGN_TOP_MID, 0, 20);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, HISTORY_LEN);
    lv_chart_set_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, -200, 200);
    lv_obj_set_style_bg_color(ctx->chart, lv_color_hex(0x0d1117), 0);
    lv_obj_set_style_line_width(ctx->chart, 0, LV_PART_TICKS);
    lv_obj_set_style_size(ctx->chart, 0, 0, LV_PART_INDICATOR);

    for (int i = 0; i < NUM_CHANNELS; i++) {
        ctx->series[i] = lv_chart_add_series(ctx->chart, ctx->ch[i].color,
                                              LV_CHART_AXIS_PRIMARY_Y);
    }

    lv_obj_t *ch = lv_label_create(chart_card);
    lv_label_set_text(ch, "TREND CHART");
    lv_obj_set_style_text_color(ch, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ch, &lv_font_montserrat_14, 0);

    /* Channel status cards (right) */
    lv_obj_t *status_card = lv_obj_create(parent);
    lv_obj_set_size(status_card, CARD_W - 10, CARD_H + 50);
    lv_obj_set_pos(status_card, CARD_W + 20, 36);
    lv_obj_set_style_bg_color(status_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(status_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(status_card, 12, 0);
    lv_obj_set_style_border_width(status_card, 1, 0);
    lv_obj_set_style_border_color(status_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(status_card, 10, 0);
    lv_obj_clear_flag(status_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sh = lv_label_create(status_card);
    lv_label_set_text(sh, "CHANNEL STATUS");
    lv_obj_set_style_text_color(sh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(sh, &lv_font_montserrat_14, 0);

    for (int i = 0; i < NUM_CHANNELS; i++) {
        lv_coord_t y = 22 + i * 52;

        ctx->alert_dots[i] = lv_obj_create(status_card);
        lv_obj_set_size(ctx->alert_dots[i], 10, 10);
        lv_obj_set_pos(ctx->alert_dots[i], 0, y + 3);
        lv_obj_set_style_bg_color(ctx->alert_dots[i], UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_bg_opa(ctx->alert_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->alert_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(ctx->alert_dots[i], 0, 0);
        lv_obj_clear_flag(ctx->alert_dots[i], LV_OBJ_FLAG_SCROLLABLE);

        ctx->value_labels[i] = lv_label_create(status_card);
        lv_obj_set_style_text_color(ctx->value_labels[i], ctx->ch[i].color, 0);
        lv_obj_set_style_text_font(ctx->value_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->value_labels[i], 16, y);
        lv_label_set_text(ctx->value_labels[i], "---");

        ctx->trend_labels[i] = lv_label_create(status_card);
        lv_obj_set_style_text_font(ctx->trend_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->trend_labels[i], 16, y + 18);
        lv_label_set_text(ctx->trend_labels[i], "= 0.000/s");

        ctx->predict_labels[i] = lv_label_create(status_card);
        lv_obj_set_style_text_font(ctx->predict_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->predict_labels[i], 200, y + 9);
        lv_label_set_text(ctx->predict_labels[i], "Stable");
    }

    /* Alert log (bottom) */
    lv_obj_t *log_card = lv_obj_create(parent);
    lv_obj_set_size(log_card, 780, 130);
    lv_obj_set_pos(log_card, 10, 296);
    lv_obj_set_style_bg_color(log_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(log_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(log_card, 12, 0);
    lv_obj_set_style_border_width(log_card, 1, 0);
    lv_obj_set_style_border_color(log_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(log_card, 10, 0);
    lv_obj_clear_flag(log_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lh = lv_label_create(log_card);
    lv_label_set_text(lh, "ALERT LOG");
    lv_obj_set_style_text_color(lh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(lh, &lv_font_montserrat_14, 0);

    ctx->alert_log = lv_label_create(log_card);
    lv_obj_set_style_text_color(ctx->alert_log, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->alert_log, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->alert_log, 0, 22);
    lv_obj_set_width(ctx->alert_log, 760);
    lv_label_set_long_mode(ctx->alert_log, LV_LABEL_LONG_WRAP);
    lv_label_set_text(ctx->alert_log, "Initializing sensors...");

    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
