/**
 * A14 — Oscilloscope
 *
 * Oscilloscope-style waveform display of IMU data with
 * timebase control, trigger level, cursors, and measurements.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define SAMPLE_MS       20
#define TRACE_POINTS    200
#define NUM_TRACES      3
#define SCOPE_W         600
#define SCOPE_H         320
#define SCOPE_X         10
#define SCOPE_Y         50

typedef struct {
    lv_obj_t          *parent;
    lv_obj_t          *scope_area;
    lv_obj_t          *chart;
    lv_chart_series_t *traces[NUM_TRACES];
    lv_obj_t          *ch_labels[NUM_TRACES];
    lv_obj_t          *ch_vals[NUM_TRACES];
    lv_obj_t          *trigger_label;
    lv_obj_t          *trigger_line;
    lv_obj_t          *timebase_label;
    lv_obj_t          *freq_label;
    lv_obj_t          *vpp_labels[NUM_TRACES];
    lv_obj_t          *rms_labels[NUM_TRACES];
    lv_obj_t          *run_btn;
    bool               running;
    int                timebase_idx;
    float              trigger_level;
    float              history[NUM_TRACES][TRACE_POINTS];
    int                hist_idx;
    float              min_vals[NUM_TRACES];
    float              max_vals[NUM_TRACES];
    float              sum_sq[NUM_TRACES];
    uint32_t           sample_count;
    float              zero_cross_last[NUM_TRACES];
    uint32_t           zero_cross_count[NUM_TRACES];
} app_ctx_t;

static app_ctx_t g_ctx;

static const int timebase_ms[] = {10, 20, 50, 100, 200, 500};
static const char *timebase_str[] = {"10ms", "20ms", "50ms", "100ms", "200ms", "500ms"};

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    if (!ctx->running) return;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

#if BSP_HAS_BMI270
    float vals[3] = {
        (float)snap.bmi270.ax / 16384.0f,
        (float)snap.bmi270.ay / 16384.0f,
        (float)snap.bmi270.az / 16384.0f,
    };
#else
    float vals[3] = {0, 0, 1.0f};
#endif

    /* Store in history buffer */
    for (int i = 0; i < NUM_TRACES; i++) {
        ctx->history[i][ctx->hist_idx] = vals[i];

        /* Track min/max for Vpp */
        if (ctx->sample_count == 0) {
            ctx->min_vals[i] = ctx->max_vals[i] = vals[i];
        } else {
            if (vals[i] < ctx->min_vals[i]) ctx->min_vals[i] = vals[i];
            if (vals[i] > ctx->max_vals[i]) ctx->max_vals[i] = vals[i];
        }

        /* RMS accumulator */
        ctx->sum_sq[i] += vals[i] * vals[i];

        /* Zero crossing detection (channel 0 only for frequency) */
        if (i == 0 && ctx->sample_count > 0) {
            float prev = ctx->zero_cross_last[i];
            if ((prev < 0 && vals[i] >= 0) || (prev >= 0 && vals[i] < 0)) {
                ctx->zero_cross_count[i]++;
            }
        }
        ctx->zero_cross_last[i] = vals[i];

        /* Update chart */
        lv_chart_set_next_value(ctx->chart, ctx->traces[i],
            (lv_coord_t)(vals[i] * 1000.0f));
    }

    ctx->hist_idx = (ctx->hist_idx + 1) % TRACE_POINTS;
    ctx->sample_count++;

    /* Update value labels */
    static const char *ch_names[] = {"X", "Y", "Z"};
    for (int i = 0; i < NUM_TRACES; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s: %+.3f g", ch_names[i], (double)vals[i]);
        lv_label_set_text(ctx->ch_vals[i], buf);

        /* Vpp */
        float vpp = ctx->max_vals[i] - ctx->min_vals[i];
        snprintf(buf, sizeof(buf), "Vpp: %.3f", (double)vpp);
        lv_label_set_text(ctx->vpp_labels[i], buf);

        /* RMS */
        float rms = (ctx->sample_count > 0) ?
            sqrtf(ctx->sum_sq[i] / (float)ctx->sample_count) : 0;
        snprintf(buf, sizeof(buf), "RMS: %.3f", (double)rms);
        lv_label_set_text(ctx->rms_labels[i], buf);
    }

    /* Frequency estimate from zero crossings */
    if (ctx->sample_count > 100) {
        float elapsed_s = (float)ctx->sample_count * (float)SAMPLE_MS / 1000.0f;
        float freq = (float)ctx->zero_cross_count[0] / (2.0f * elapsed_s);
        char fbuf[32];
        snprintf(fbuf, sizeof(fbuf), "f: %.1f Hz", (double)freq);
        lv_label_set_text(ctx->freq_label, fbuf);
    }

    /* Reset stats periodically */
    if (ctx->sample_count % 500 == 0) {
        for (int i = 0; i < NUM_TRACES; i++) {
            ctx->min_vals[i] = ctx->max_vals[i] = vals[i];
            ctx->sum_sq[i] = 0;
            ctx->zero_cross_count[i] = 0;
        }
        ctx->sample_count = 1;
    }
}

static void run_btn_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    ctx->running = !ctx->running;
    lv_obj_t *lbl = lv_obj_get_child(ctx->run_btn, 0);
    lv_label_set_text(lbl, ctx->running ? LV_SYMBOL_PAUSE " STOP" : LV_SYMBOL_PLAY " RUN");
}

static void tb_up_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    if (ctx->timebase_idx < 5) ctx->timebase_idx++;
    lv_label_set_text(ctx->timebase_label, timebase_str[ctx->timebase_idx]);
}

static void tb_down_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    if (ctx->timebase_idx > 0) ctx->timebase_idx--;
    lv_label_set_text(ctx->timebase_label, timebase_str[ctx->timebase_idx]);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;
    ctx->running = true;
    ctx->timebase_idx = 3;
    ctx->trigger_level = 0.5f;

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
    lv_label_set_text(title, LV_SYMBOL_CHARGE " Oscilloscope — 3ch IMU");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    /* Run/Stop button */
    ctx->run_btn = lv_btn_create(hdr);
    lv_obj_set_size(ctx->run_btn, 100, 30);
    lv_obj_align(ctx->run_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(ctx->run_btn, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(ctx->run_btn, 6, 0);
    lv_obj_t *rl = lv_label_create(ctx->run_btn);
    lv_label_set_text(rl, LV_SYMBOL_PAUSE " STOP");
    lv_obj_set_style_text_color(rl, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_14, 0);
    lv_obj_center(rl);
    lv_obj_add_event_cb(ctx->run_btn, run_btn_cb, LV_EVENT_CLICKED, ctx);

    /* Scope chart */
    ctx->scope_area = lv_obj_create(parent);
    lv_obj_set_size(ctx->scope_area, SCOPE_W, SCOPE_H);
    lv_obj_set_pos(ctx->scope_area, SCOPE_X, SCOPE_Y);
    lv_obj_set_style_bg_color(ctx->scope_area, lv_color_hex(0x001a00), 0);
    lv_obj_set_style_bg_opa(ctx->scope_area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->scope_area, 8, 0);
    lv_obj_set_style_border_width(ctx->scope_area, 2, 0);
    lv_obj_set_style_border_color(ctx->scope_area, lv_color_hex(0x004400), 0);
    lv_obj_set_style_pad_all(ctx->scope_area, 4, 0);
    lv_obj_clear_flag(ctx->scope_area, LV_OBJ_FLAG_SCROLLABLE);

    /* Grid lines (horizontal) */
    for (int i = 1; i < 8; i++) {
        lv_obj_t *gl = lv_obj_create(ctx->scope_area);
        lv_obj_set_size(gl, SCOPE_W - 12, 1);
        lv_obj_set_pos(gl, 0, i * (SCOPE_H - 12) / 8);
        lv_obj_set_style_bg_color(gl, lv_color_hex(0x003300), 0);
        lv_obj_set_style_bg_opa(gl, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(gl, 0, 0);
        lv_obj_clear_flag(gl, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Grid lines (vertical) */
    for (int i = 1; i < 10; i++) {
        lv_obj_t *gl = lv_obj_create(ctx->scope_area);
        lv_obj_set_size(gl, 1, SCOPE_H - 12);
        lv_obj_set_pos(gl, i * (SCOPE_W - 12) / 10, 0);
        lv_obj_set_style_bg_color(gl, lv_color_hex(0x003300), 0);
        lv_obj_set_style_bg_opa(gl, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(gl, 0, 0);
        lv_obj_clear_flag(gl, LV_OBJ_FLAG_SCROLLABLE);
    }

    ctx->chart = lv_chart_create(ctx->scope_area);
    lv_obj_set_size(ctx->chart, SCOPE_W - 16, SCOPE_H - 16);
    lv_obj_center(ctx->chart);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, TRACE_POINTS);
    lv_chart_set_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000);
    lv_obj_set_style_bg_opa(ctx->chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_size(ctx->chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(ctx->chart, 0, LV_PART_TICKS);

    static lv_color_t trace_colors[] = {
        {.blue=0x50, .green=0xAF, .red=0x4C},  /* green */
        {.blue=0xF3, .green=0x96, .red=0x21},  /* blue */
        {.blue=0x00, .green=0x98, .red=0xFF},  /* orange */
    };
    for (int i = 0; i < NUM_TRACES; i++) {
        ctx->traces[i] = lv_chart_add_series(ctx->chart, trace_colors[i],
                                              LV_CHART_AXIS_PRIMARY_Y);
    }

    /* Right panel */
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, 160, SCOPE_H + 80);
    lv_obj_set_pos(panel, 620, SCOPE_Y);
    lv_obj_set_style_bg_color(panel, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(panel, 12, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(panel, 10, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Channel readouts */
    static const char *ch_names[] = {"CH1 (X)", "CH2 (Y)", "CH3 (Z)"};
    for (int i = 0; i < NUM_TRACES; i++) {
        lv_coord_t cy = i * 96;

        ctx->ch_labels[i] = lv_label_create(panel);
        lv_label_set_text(ctx->ch_labels[i], ch_names[i]);
        lv_obj_set_style_text_color(ctx->ch_labels[i], trace_colors[i], 0);
        lv_obj_set_style_text_font(ctx->ch_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->ch_labels[i], 0, cy);

        ctx->ch_vals[i] = lv_label_create(panel);
        lv_obj_set_style_text_color(ctx->ch_vals[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->ch_vals[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->ch_vals[i], 0, cy + 18);

        ctx->vpp_labels[i] = lv_label_create(panel);
        lv_obj_set_style_text_color(ctx->vpp_labels[i], lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(ctx->vpp_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->vpp_labels[i], 0, cy + 36);
        lv_label_set_text(ctx->vpp_labels[i], "Vpp: 0.000");

        ctx->rms_labels[i] = lv_label_create(panel);
        lv_obj_set_style_text_color(ctx->rms_labels[i], lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(ctx->rms_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->rms_labels[i], 0, cy + 54);
        lv_label_set_text(ctx->rms_labels[i], "RMS: 0.000");
    }

    /* Timebase control */
    lv_coord_t ctrl_y = 290;
    lv_obj_t *tbh = lv_label_create(panel);
    lv_label_set_text(tbh, "Timebase:");
    lv_obj_set_style_text_color(tbh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(tbh, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(tbh, 0, ctrl_y);

    ctx->timebase_label = lv_label_create(panel);
    lv_label_set_text(ctx->timebase_label, timebase_str[ctx->timebase_idx]);
    lv_obj_set_style_text_color(ctx->timebase_label, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(ctx->timebase_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->timebase_label, 80, ctrl_y);

    lv_obj_t *tb_up = lv_btn_create(panel);
    lv_obj_set_size(tb_up, 30, 24);
    lv_obj_set_pos(tb_up, 60, ctrl_y + 20);
    lv_obj_set_style_bg_color(tb_up, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_radius(tb_up, 4, 0);
    lv_obj_t *tul = lv_label_create(tb_up);
    lv_label_set_text(tul, "+");
    lv_obj_set_style_text_color(tul, UI_COLOR_TEXT, 0);
    lv_obj_center(tul);
    lv_obj_add_event_cb(tb_up, tb_up_cb, LV_EVENT_CLICKED, ctx);

    lv_obj_t *tb_dn = lv_btn_create(panel);
    lv_obj_set_size(tb_dn, 30, 24);
    lv_obj_set_pos(tb_dn, 100, ctrl_y + 20);
    lv_obj_set_style_bg_color(tb_dn, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_radius(tb_dn, 4, 0);
    lv_obj_t *tdl = lv_label_create(tb_dn);
    lv_label_set_text(tdl, "-");
    lv_obj_set_style_text_color(tdl, UI_COLOR_TEXT, 0);
    lv_obj_center(tdl);
    lv_obj_add_event_cb(tb_dn, tb_down_cb, LV_EVENT_CLICKED, ctx);

    /* Frequency display */
    ctx->freq_label = lv_label_create(panel);
    lv_obj_set_style_text_color(ctx->freq_label, UI_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(ctx->freq_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->freq_label, 0, ctrl_y + 50);
    lv_label_set_text(ctx->freq_label, "f: -- Hz");

    /* Scale labels on scope */
    lv_obj_t *scale_top = lv_label_create(parent);
    lv_label_set_text(scale_top, "+2.0g");
    lv_obj_set_style_text_color(scale_top, lv_color_hex(0x004400), 0);
    lv_obj_set_style_text_font(scale_top, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(scale_top, SCOPE_X + 4, SCOPE_Y + 4);

    lv_obj_t *scale_mid = lv_label_create(parent);
    lv_label_set_text(scale_mid, " 0.0g");
    lv_obj_set_style_text_color(scale_mid, lv_color_hex(0x004400), 0);
    lv_obj_set_style_text_font(scale_mid, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(scale_mid, SCOPE_X + 4, SCOPE_Y + SCOPE_H / 2 - 8);

    lv_obj_t *scale_bot = lv_label_create(parent);
    lv_label_set_text(scale_bot, "-2.0g");
    lv_obj_set_style_text_color(scale_bot, lv_color_hex(0x004400), 0);
    lv_obj_set_style_text_font(scale_bot, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(scale_bot, SCOPE_X + 4, SCOPE_Y + SCOPE_H - 20);

    lv_timer_create(timer_cb, SAMPLE_MS, ctx);
}
