/**
 * A13 — Data Recorder
 *
 * Circular buffer data recorder with configurable channels,
 * recording controls, statistics, and export-ready data display.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define BUFFER_SIZE     200
#define NUM_CHANNELS    3
#define SAMPLE_MS       100
#define CHART_POINTS    100

typedef enum { REC_IDLE, REC_RECORDING, REC_PAUSED } rec_state_t;

typedef struct {
    const char *name;
    const char *unit;
    lv_color_t  color;
    float       buffer[BUFFER_SIZE];
    int         head;
    int         count;
    float       min_val, max_val, sum;
} channel_t;

typedef struct {
    lv_obj_t        *parent;
    channel_t        ch[NUM_CHANNELS];
    lv_obj_t        *chart;
    lv_chart_series_t *series[NUM_CHANNELS];
    lv_obj_t        *stat_labels[NUM_CHANNELS][4]; /* cur, min, max, avg */
    lv_obj_t        *state_label;
    lv_obj_t        *count_label;
    lv_obj_t        *rate_label;
    lv_obj_t        *rec_btn;
    lv_obj_t        *pause_btn;
    lv_obj_t        *clear_btn;
    lv_obj_t        *export_area;
    rec_state_t      state;
    uint32_t         total_samples;
    uint32_t         elapsed_ms;
} app_ctx_t;

static app_ctx_t g_ctx;

static void push_sample(channel_t *ch, float val)
{
    ch->buffer[ch->head] = val;
    ch->head = (ch->head + 1) % BUFFER_SIZE;
    if (ch->count < BUFFER_SIZE) ch->count++;

    /* Update stats */
    if (ch->count == 1) {
        ch->min_val = ch->max_val = val;
        ch->sum = val;
    } else {
        if (val < ch->min_val) ch->min_val = val;
        if (val > ch->max_val) ch->max_val = val;
        ch->sum += val;
    }
}

static void clear_channel(channel_t *ch)
{
    ch->head = 0;
    ch->count = 0;
    ch->min_val = 0;
    ch->max_val = 0;
    ch->sum = 0;
}

static void update_stats(app_ctx_t *ctx)
{
    for (int i = 0; i < NUM_CHANNELS; i++) {
        channel_t *ch = &ctx->ch[i];
        char buf[32];

        /* Current */
        float cur = (ch->count > 0) ? ch->buffer[(ch->head - 1 + BUFFER_SIZE) % BUFFER_SIZE] : 0;
        snprintf(buf, sizeof(buf), "%.2f", (double)cur);
        lv_label_set_text(ctx->stat_labels[i][0], buf);

        /* Min */
        snprintf(buf, sizeof(buf), "%.2f", (double)ch->min_val);
        lv_label_set_text(ctx->stat_labels[i][1], buf);

        /* Max */
        snprintf(buf, sizeof(buf), "%.2f", (double)ch->max_val);
        lv_label_set_text(ctx->stat_labels[i][2], buf);

        /* Avg */
        float avg = (ch->count > 0) ? ch->sum / (float)ch->count : 0;
        snprintf(buf, sizeof(buf), "%.2f", (double)avg);
        lv_label_set_text(ctx->stat_labels[i][3], buf);
    }

    char cbuf[32];
    snprintf(cbuf, sizeof(cbuf), "Samples: %lu", (unsigned long)ctx->total_samples);
    lv_label_set_text(ctx->count_label, cbuf);

    float rate = (ctx->elapsed_ms > 0) ?
        (float)ctx->total_samples / ((float)ctx->elapsed_ms / 1000.0f) : 0;
    snprintf(cbuf, sizeof(cbuf), "Rate: %.1f Hz", (double)rate);
    lv_label_set_text(ctx->rate_label, cbuf);

    /* Export preview */
    char export_buf[256] = "timestamp,ax_g,ay_g,az_g\n";
    int start = (ctx->ch[0].head - 5 + BUFFER_SIZE) % BUFFER_SIZE;
    for (int j = 0; j < 5 && j < ctx->ch[0].count; j++) {
        int idx = (start + j) % BUFFER_SIZE;
        char line[64];
        snprintf(line, sizeof(line), "%lu,%.3f,%.3f,%.3f\n",
                 (unsigned long)(ctx->total_samples - 5 + j),
                 (double)ctx->ch[0].buffer[idx],
                 (double)ctx->ch[1].buffer[idx],
                 (double)ctx->ch[2].buffer[idx]);
        strncat(export_buf, line, sizeof(export_buf) - strlen(export_buf) - 1);
    }
    lv_label_set_text(ctx->export_area, export_buf);
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    if (ctx->state != REC_RECORDING) return;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

#if BSP_HAS_BMI270
    float ax = (float)snap.bmi270.ax / 16384.0f;
    float ay = (float)snap.bmi270.ay / 16384.0f;
    float az = (float)snap.bmi270.az / 16384.0f;
#else
    float ax = 0.0f, ay = 0.0f, az = 1.0f;
#endif

    push_sample(&ctx->ch[0], ax);
    push_sample(&ctx->ch[1], ay);
    push_sample(&ctx->ch[2], az);

    ctx->total_samples++;
    ctx->elapsed_ms += SAMPLE_MS;

    /* Update chart */
    for (int i = 0; i < NUM_CHANNELS; i++) {
        float cur = ctx->ch[i].buffer[(ctx->ch[i].head - 1 + BUFFER_SIZE) % BUFFER_SIZE];
        lv_chart_set_next_value(ctx->chart, ctx->series[i], (lv_coord_t)(cur * 1000.0f));
    }

    update_stats(ctx);
}

static void rec_btn_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    if (ctx->state == REC_IDLE || ctx->state == REC_PAUSED) {
        ctx->state = REC_RECORDING;
        lv_label_set_text(ctx->state_label, LV_SYMBOL_OK " RECORDING");
        lv_obj_set_style_text_color(ctx->state_label, UI_COLOR_ERROR, 0);
    } else {
        ctx->state = REC_IDLE;
        lv_label_set_text(ctx->state_label, "STOPPED");
        lv_obj_set_style_text_color(ctx->state_label, lv_color_hex(0x607d8b), 0);
    }
}

static void pause_btn_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    if (ctx->state == REC_RECORDING) {
        ctx->state = REC_PAUSED;
        lv_label_set_text(ctx->state_label, LV_SYMBOL_PAUSE " PAUSED");
        lv_obj_set_style_text_color(ctx->state_label, UI_COLOR_WARNING, 0);
    } else if (ctx->state == REC_PAUSED) {
        ctx->state = REC_RECORDING;
        lv_label_set_text(ctx->state_label, LV_SYMBOL_OK " RECORDING");
        lv_obj_set_style_text_color(ctx->state_label, UI_COLOR_ERROR, 0);
    }
}

static void clear_btn_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    ctx->state = REC_IDLE;
    ctx->total_samples = 0;
    ctx->elapsed_ms = 0;
    for (int i = 0; i < NUM_CHANNELS; i++) {
        clear_channel(&ctx->ch[i]);
    }
    lv_label_set_text(ctx->state_label, "CLEARED");
    lv_obj_set_style_text_color(ctx->state_label, lv_color_hex(0x607d8b), 0);
    update_stats(ctx);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    ctx->ch[0] = (channel_t){ .name = "Accel X", .unit = "g", .color = UI_COLOR_BMI270 };
    ctx->ch[1] = (channel_t){ .name = "Accel Y", .unit = "g", .color = UI_COLOR_DPS368 };
    ctx->ch[2] = (channel_t){ .name = "Accel Z", .unit = "g", .color = UI_COLOR_SHT40  };

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Header */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 780, 40);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hdr, 8, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, LV_SYMBOL_SAVE " Data Recorder");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    ctx->state_label = lv_label_create(hdr);
    lv_label_set_text(ctx->state_label, "IDLE");
    lv_obj_set_style_text_color(ctx->state_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->state_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->state_label, LV_ALIGN_CENTER, 0, 0);

    ctx->count_label = lv_label_create(hdr);
    lv_obj_set_style_text_color(ctx->count_label, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ctx->count_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->count_label, LV_ALIGN_RIGHT_MID, -120, 0);

    ctx->rate_label = lv_label_create(hdr);
    lv_obj_set_style_text_color(ctx->rate_label, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ctx->rate_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->rate_label, LV_ALIGN_RIGHT_MID, -10, 0);

    /* Control buttons */
    static const struct { const char *text; lv_color_t c; void (*cb)(lv_event_t *); } btns[] = {
        { LV_SYMBOL_PLAY " REC",   {.blue=0x00, .green=0x17, .red=0xFF}, rec_btn_cb   },
        { LV_SYMBOL_PAUSE " PAUSE",{.blue=0x00, .green=0x98, .red=0xFF}, pause_btn_cb },
        { LV_SYMBOL_TRASH " CLEAR",{.blue=0x8b, .green=0x7d, .red=0x60}, clear_btn_cb },
    };
    lv_obj_t *btn_ptrs[3];
    for (int i = 0; i < 3; i++) {
        btn_ptrs[i] = lv_btn_create(parent);
        lv_obj_set_size(btn_ptrs[i], 120, 34);
        lv_obj_set_pos(btn_ptrs[i], 10 + i * 128, 50);
        lv_obj_set_style_bg_color(btn_ptrs[i], btns[i].c, 0);
        lv_obj_set_style_radius(btn_ptrs[i], 8, 0);

        lv_obj_t *bl = lv_label_create(btn_ptrs[i]);
        lv_label_set_text(bl, btns[i].text);
        lv_obj_set_style_text_color(bl, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
        lv_obj_center(bl);

        lv_obj_add_event_cb(btn_ptrs[i], btns[i].cb, LV_EVENT_CLICKED, ctx);
    }

    /* Chart */
    lv_obj_t *chart_card = lv_obj_create(parent);
    lv_obj_set_size(chart_card, 500, 200);
    lv_obj_set_pos(chart_card, 10, 90);
    lv_obj_set_style_bg_color(chart_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(chart_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(chart_card, 12, 0);
    lv_obj_set_style_border_width(chart_card, 1, 0);
    lv_obj_set_style_border_color(chart_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(chart_card, 8, 0);
    lv_obj_clear_flag(chart_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->chart = lv_chart_create(chart_card);
    lv_obj_set_size(ctx->chart, 480, 180);
    lv_obj_align(ctx->chart, LV_ALIGN_CENTER, 0, 0);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, CHART_POINTS);
    lv_chart_set_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000);
    lv_obj_set_style_bg_color(ctx->chart, lv_color_hex(0x0d1117), 0);
    lv_obj_set_style_size(ctx->chart, 0, 0, LV_PART_INDICATOR);

    for (int i = 0; i < NUM_CHANNELS; i++) {
        ctx->series[i] = lv_chart_add_series(ctx->chart, ctx->ch[i].color,
                                              LV_CHART_AXIS_PRIMARY_Y);
    }

    /* Statistics table (right) */
    lv_obj_t *stat_card = lv_obj_create(parent);
    lv_obj_set_size(stat_card, 268, 200);
    lv_obj_set_pos(stat_card, 520, 90);
    lv_obj_set_style_bg_color(stat_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(stat_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(stat_card, 12, 0);
    lv_obj_set_style_border_width(stat_card, 1, 0);
    lv_obj_set_style_border_color(stat_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(stat_card, 10, 0);
    lv_obj_clear_flag(stat_card, LV_OBJ_FLAG_SCROLLABLE);

    /* Table headers */
    static const char *col_headers[] = {"Channel", "Cur", "Min", "Max", "Avg"};
    for (int c = 0; c < 5; c++) {
        lv_obj_t *h = lv_label_create(stat_card);
        lv_label_set_text(h, col_headers[c]);
        lv_obj_set_style_text_color(h, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(h, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(h, c * 50, 0);
    }

    for (int i = 0; i < NUM_CHANNELS; i++) {
        lv_coord_t ry = 22 + i * 56;

        lv_obj_t *cn = lv_label_create(stat_card);
        lv_label_set_text(cn, ctx->ch[i].name);
        lv_obj_set_style_text_color(cn, ctx->ch[i].color, 0);
        lv_obj_set_style_text_font(cn, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(cn, 0, ry);

        static const char *stat_names[] = {"Cur", "Min", "Max", "Avg"};
        for (int j = 0; j < 4; j++) {
            lv_obj_t *sn = lv_label_create(stat_card);
            lv_label_set_text(sn, stat_names[j]);
            lv_obj_set_style_text_color(sn, lv_color_hex(0x546e7a), 0);
            lv_obj_set_style_text_font(sn, &lv_font_montserrat_14, 0);
            lv_obj_set_pos(sn, j * 60, ry + 18);

            ctx->stat_labels[i][j] = lv_label_create(stat_card);
            lv_obj_set_style_text_color(ctx->stat_labels[i][j], UI_COLOR_TEXT, 0);
            lv_obj_set_style_text_font(ctx->stat_labels[i][j], &lv_font_montserrat_14, 0);
            lv_obj_set_pos(ctx->stat_labels[i][j], j * 60, ry + 34);
            lv_label_set_text(ctx->stat_labels[i][j], "0.00");
        }
    }

    /* Export preview (bottom) */
    lv_obj_t *exp_card = lv_obj_create(parent);
    lv_obj_set_size(exp_card, 780, 120);
    lv_obj_set_pos(exp_card, 10, 300);
    lv_obj_set_style_bg_color(exp_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(exp_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(exp_card, 12, 0);
    lv_obj_set_style_border_width(exp_card, 1, 0);
    lv_obj_set_style_border_color(exp_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(exp_card, 10, 0);
    lv_obj_clear_flag(exp_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *eh = lv_label_create(exp_card);
    lv_label_set_text(eh, "CSV EXPORT PREVIEW (last 5 samples)");
    lv_obj_set_style_text_color(eh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(eh, &lv_font_montserrat_14, 0);

    ctx->export_area = lv_label_create(exp_card);
    lv_obj_set_style_text_color(ctx->export_area, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(ctx->export_area, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->export_area, 0, 20);
    lv_obj_set_width(ctx->export_area, 760);
    lv_label_set_text(ctx->export_area, "Press REC to start recording...");

    lv_timer_create(timer_cb, SAMPLE_MS, ctx);
}
