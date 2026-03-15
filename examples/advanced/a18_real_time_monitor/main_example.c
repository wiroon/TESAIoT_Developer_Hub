/**
 * A18 — Real-Time Process Monitor
 *
 * Industrial-style process monitor with alarm zones,
 * setpoint controls, PID-style indicators, and alarm history.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define REFRESH_MS      200
#define NUM_PROCS       3
#define ALARM_HIST_LEN  8
#define CHART_POINTS    80

typedef enum { ALARM_NONE, ALARM_LOW, ALARM_HIGH, ALARM_CRITICAL } alarm_t;

typedef struct {
    const char *name;
    const char *unit;
    lv_color_t  color;
    float       value;
    float       setpoint;
    float       low_limit;
    float       high_limit;
    float       critical_limit;
    alarm_t     alarm;
} process_t;

typedef struct {
    char    msg[64];
    alarm_t level;
} alarm_entry_t;

typedef struct {
    lv_obj_t       *parent;
    process_t       procs[NUM_PROCS];
    lv_obj_t       *value_labels[NUM_PROCS];
    lv_obj_t       *setpoint_labels[NUM_PROCS];
    lv_obj_t       *alarm_labels[NUM_PROCS];
    lv_obj_t       *gauge_bars[NUM_PROCS];
    lv_obj_t       *deviation_labels[NUM_PROCS];
    lv_obj_t       *chart;
    lv_chart_series_t *series[NUM_PROCS];
    lv_obj_t       *alarm_log;
    alarm_entry_t   alarms[ALARM_HIST_LEN];
    int             alarm_head;
    int             alarm_count;
    lv_obj_t       *time_label;
    lv_obj_t       *total_alarm_label;
    int             total_alarms;
} app_ctx_t;

static app_ctx_t g_ctx;

static void add_alarm(app_ctx_t *ctx, const char *proc_name, alarm_t level)
{
    alarm_entry_t *e = &ctx->alarms[ctx->alarm_head];
    const char *level_str = (level == ALARM_CRITICAL) ? "CRIT" :
                            (level == ALARM_HIGH) ? "HIGH" : "LOW";
    char tbuf[16];
    ipc_sensorhub_get_time_str(tbuf, sizeof(tbuf));
    snprintf(e->msg, sizeof(e->msg), "[%s] %s: %s alarm", tbuf, proc_name, level_str);
    e->level = level;
    ctx->alarm_head = (ctx->alarm_head + 1) % ALARM_HIST_LEN;
    if (ctx->alarm_count < ALARM_HIST_LEN) ctx->alarm_count++;
    ctx->total_alarms++;
}

static alarm_t check_alarm(process_t *p)
{
    float dev = fabsf(p->value - p->setpoint);
    if (p->value > p->critical_limit || p->value < p->low_limit * 0.8f) return ALARM_CRITICAL;
    if (p->value > p->high_limit) return ALARM_HIGH;
    if (p->value < p->low_limit) return ALARM_LOW;
    return ALARM_NONE;
}

static lv_color_t alarm_color(alarm_t a)
{
    switch (a) {
    case ALARM_CRITICAL: return UI_COLOR_ERROR;
    case ALARM_HIGH:     return UI_COLOR_WARNING;
    case ALARM_LOW:      return UI_COLOR_INFO;
    default:             return UI_COLOR_SUCCESS;
    }
}

static const char *alarm_str(alarm_t a)
{
    switch (a) {
    case ALARM_CRITICAL: return "CRITICAL";
    case ALARM_HIGH:     return "HIGH";
    case ALARM_LOW:      return "LOW";
    default:             return "NORMAL";
    }
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    /* Map sensors to process values */
#if BSP_HAS_DPS368
    ctx->procs[0].value = (float)snap.dps368.temperature_x100 / 100.0f;
    ctx->procs[1].value = (float)snap.dps368.pressure_x100 / 100.0f;
#else
    ctx->procs[0].value = 25.0f + (float)(snap.bmi270.ax & 0xFF) * 0.02f;
    ctx->procs[1].value = 1013.0f + (float)(snap.bmi270.ay & 0xFF) * 0.05f;
#endif

#if BSP_HAS_SHT40
    ctx->procs[2].value = (float)snap.sht40.humidity_x100 / 100.0f;
#else
    ctx->procs[2].value = 50.0f + (float)(snap.bmi270.az & 0xFF) * 0.1f;
#endif

    for (int i = 0; i < NUM_PROCS; i++) {
        process_t *p = &ctx->procs[i];
        alarm_t new_alarm = check_alarm(p);

        if (new_alarm != p->alarm && new_alarm != ALARM_NONE) {
            add_alarm(ctx, p->name, new_alarm);
        }
        p->alarm = new_alarm;

        /* Update UI */
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f %s", (double)p->value, p->unit);
        lv_label_set_text(ctx->value_labels[i], buf);

        snprintf(buf, sizeof(buf), "SP: %.1f", (double)p->setpoint);
        lv_label_set_text(ctx->setpoint_labels[i], buf);

        lv_label_set_text(ctx->alarm_labels[i], alarm_str(p->alarm));
        lv_obj_set_style_text_color(ctx->alarm_labels[i], alarm_color(p->alarm), 0);

        /* Deviation */
        float dev = p->value - p->setpoint;
        snprintf(buf, sizeof(buf), "%+.2f", (double)dev);
        lv_label_set_text(ctx->deviation_labels[i], buf);
        lv_obj_set_style_text_color(ctx->deviation_labels[i],
            fabsf(dev) < 1.0f ? UI_COLOR_SUCCESS :
            fabsf(dev) < 3.0f ? UI_COLOR_WARNING : UI_COLOR_ERROR, 0);

        /* Gauge bar: 0-100 based on range */
        float range = p->critical_limit - p->low_limit * 0.8f;
        int pct = (int)((p->value - p->low_limit * 0.8f) / range * 100.0f);
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        lv_bar_set_value(ctx->gauge_bars[i], pct, LV_ANIM_ON);
        lv_obj_set_style_bg_color(ctx->gauge_bars[i], alarm_color(p->alarm), LV_PART_INDICATOR);

        /* Chart */
        lv_chart_set_next_value(ctx->chart, ctx->series[i],
            (lv_coord_t)(p->value * 10.0f));
    }

    /* Update alarm log */
    char log_buf[512] = "";
    for (int i = 0; i < ctx->alarm_count; i++) {
        int idx = (ctx->alarm_head - ctx->alarm_count + i + ALARM_HIST_LEN) % ALARM_HIST_LEN;
        strncat(log_buf, ctx->alarms[idx].msg, sizeof(log_buf) - strlen(log_buf) - 2);
        strncat(log_buf, "\n", sizeof(log_buf) - strlen(log_buf) - 1);
    }
    if (log_buf[0] == '\0') strncpy(log_buf, "No alarms.", sizeof(log_buf));
    lv_label_set_text(ctx->alarm_log, log_buf);

    char tbuf[32];
    ipc_sensorhub_get_time_str(tbuf, sizeof(tbuf));
    lv_label_set_text(ctx->time_label, tbuf);

    char abuf[32];
    snprintf(abuf, sizeof(abuf), "Total Alarms: %d", ctx->total_alarms);
    lv_label_set_text(ctx->total_alarm_label, abuf);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    ctx->procs[0] = (process_t){"Reactor Temp", "C",   UI_COLOR_DPS368, 0, 25.0f, 18.0f, 32.0f, 40.0f, ALARM_NONE};
    ctx->procs[1] = (process_t){"Chamber Press","hPa", UI_COLOR_BMM350, 0, 1013.0f, 1000.0f, 1025.0f, 1040.0f, ALARM_NONE};
    ctx->procs[2] = (process_t){"Humidity",     "%RH", UI_COLOR_SHT40,  0, 50.0f, 30.0f, 70.0f, 85.0f, ALARM_NONE};

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Title bar */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 780, 40);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hdr, 8, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Real-Time Process Monitor");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    ctx->time_label = lv_label_create(hdr);
    lv_obj_set_style_text_color(ctx->time_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->time_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->time_label, LV_ALIGN_RIGHT_MID, -10, 0);

    /* Process cards */
    for (int i = 0; i < NUM_PROCS; i++) {
        lv_obj_t *pc = lv_obj_create(parent);
        lv_obj_set_size(pc, 250, 130);
        lv_obj_set_pos(pc, 10 + i * 260, 50);
        lv_obj_set_style_bg_color(pc, UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_bg_opa(pc, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(pc, 12, 0);
        lv_obj_set_style_border_width(pc, 1, 0);
        lv_obj_set_style_border_color(pc, lv_color_hex(0x2a3a5c), 0);
        lv_obj_set_style_pad_all(pc, 10, 0);
        lv_obj_clear_flag(pc, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *pn = lv_label_create(pc);
        lv_label_set_text(pn, ctx->procs[i].name);
        lv_obj_set_style_text_color(pn, ctx->procs[i].color, 0);
        lv_obj_set_style_text_font(pn, &lv_font_montserrat_14, 0);

        ctx->value_labels[i] = lv_label_create(pc);
        lv_obj_set_style_text_color(ctx->value_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->value_labels[i], &lv_font_montserrat_20, 0);
        lv_obj_set_pos(ctx->value_labels[i], 0, 20);

        ctx->setpoint_labels[i] = lv_label_create(pc);
        lv_obj_set_style_text_color(ctx->setpoint_labels[i], lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(ctx->setpoint_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->setpoint_labels[i], 140, 24);

        ctx->gauge_bars[i] = lv_bar_create(pc);
        lv_obj_set_size(ctx->gauge_bars[i], 220, 14);
        lv_obj_set_pos(ctx->gauge_bars[i], 0, 52);
        lv_bar_set_range(ctx->gauge_bars[i], 0, 100);
        lv_obj_set_style_bg_color(ctx->gauge_bars[i], lv_color_hex(0x263238), LV_PART_MAIN);
        lv_obj_set_style_radius(ctx->gauge_bars[i], 4, 0);
        lv_obj_set_style_radius(ctx->gauge_bars[i], 4, LV_PART_INDICATOR);

        ctx->alarm_labels[i] = lv_label_create(pc);
        lv_obj_set_style_text_font(ctx->alarm_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->alarm_labels[i], 0, 74);
        lv_label_set_text(ctx->alarm_labels[i], "NORMAL");

        lv_obj_t *dl = lv_label_create(pc);
        lv_label_set_text(dl, "Dev:");
        lv_obj_set_style_text_color(dl, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(dl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(dl, 120, 74);

        ctx->deviation_labels[i] = lv_label_create(pc);
        lv_obj_set_style_text_font(ctx->deviation_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->deviation_labels[i], 155, 74);
    }

    /* Chart */
    lv_obj_t *chart_card = lv_obj_create(parent);
    lv_obj_set_size(chart_card, 480, 200);
    lv_obj_set_pos(chart_card, 10, 190);
    lv_obj_set_style_bg_color(chart_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(chart_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(chart_card, 12, 0);
    lv_obj_set_style_border_width(chart_card, 1, 0);
    lv_obj_set_style_border_color(chart_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(chart_card, 8, 0);
    lv_obj_clear_flag(chart_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ch = lv_label_create(chart_card);
    lv_label_set_text(ch, "PROCESS TREND");
    lv_obj_set_style_text_color(ch, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ch, &lv_font_montserrat_14, 0);

    ctx->chart = lv_chart_create(chart_card);
    lv_obj_set_size(ctx->chart, 460, 160);
    lv_obj_align(ctx->chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, CHART_POINTS);
    lv_chart_set_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, 0, 1200);
    lv_obj_set_style_bg_color(ctx->chart, lv_color_hex(0x0d1117), 0);
    lv_obj_set_style_size(ctx->chart, 0, 0, LV_PART_INDICATOR);

    for (int i = 0; i < NUM_PROCS; i++) {
        ctx->series[i] = lv_chart_add_series(ctx->chart, ctx->procs[i].color,
                                              LV_CHART_AXIS_PRIMARY_Y);
    }

    /* Alarm log */
    lv_obj_t *log_card = lv_obj_create(parent);
    lv_obj_set_size(log_card, 288, 200);
    lv_obj_set_pos(log_card, 500, 190);
    lv_obj_set_style_bg_color(log_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(log_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(log_card, 12, 0);
    lv_obj_set_style_border_width(log_card, 1, 0);
    lv_obj_set_style_border_color(log_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(log_card, 10, 0);
    lv_obj_clear_flag(log_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lh = lv_label_create(log_card);
    lv_label_set_text(lh, "ALARM LOG");
    lv_obj_set_style_text_color(lh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(lh, &lv_font_montserrat_14, 0);

    ctx->total_alarm_label = lv_label_create(log_card);
    lv_obj_set_style_text_color(ctx->total_alarm_label, UI_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(ctx->total_alarm_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->total_alarm_label, 0, 20);

    ctx->alarm_log = lv_label_create(log_card);
    lv_obj_set_style_text_color(ctx->alarm_log, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->alarm_log, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->alarm_log, 0, 40);
    lv_obj_set_width(ctx->alarm_log, 268);
    lv_label_set_long_mode(ctx->alarm_log, LV_LABEL_LONG_CLIP);
    lv_label_set_text(ctx->alarm_log, "No alarms.");

    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
