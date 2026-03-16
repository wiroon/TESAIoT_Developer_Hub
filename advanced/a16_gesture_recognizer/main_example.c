/**
 * A16 — Gesture Recognizer
 *
 * Detects gestures (shake, tilt, flip, tap, rotate) from BMI270 IMU
 * data using threshold and pattern-based detection algorithms.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define SAMPLE_MS       30
#define HISTORY_LEN     30
#define NUM_GESTURES    6

typedef enum {
    GEST_NONE = 0,
    GEST_SHAKE,
    GEST_TILT_LEFT,
    GEST_TILT_RIGHT,
    GEST_FLIP,
    GEST_TAP,
    GEST_ROTATE,
} gesture_t;

typedef struct {
    const char *name;
    const char *icon;
    lv_color_t  color;
    int         count;
    uint32_t    last_tick;
} gesture_info_t;

typedef struct {
    lv_obj_t       *parent;
    gesture_info_t  gestures[NUM_GESTURES];
    lv_obj_t       *gesture_label;
    lv_obj_t       *gesture_icon;
    lv_obj_t       *count_labels[NUM_GESTURES];
    lv_obj_t       *indicator_bars[NUM_GESTURES];
    lv_obj_t       *raw_ax, *raw_ay, *raw_az;
    lv_obj_t       *raw_gx, *raw_gy, *raw_gz;
    lv_obj_t       *magnitude_bar;
    lv_obj_t       *mag_label;
    lv_obj_t       *chart;
    lv_chart_series_t *mag_series;
    lv_obj_t       *cooldown_bar;
    float           ax_hist[HISTORY_LEN];
    float           ay_hist[HISTORY_LEN];
    float           az_hist[HISTORY_LEN];
    float           gx_hist[HISTORY_LEN];
    int             hist_idx;
    int             hist_count;
    gesture_t       last_gesture;
    uint32_t        cooldown;
    uint32_t        tick;
} app_ctx_t;

static app_ctx_t g_ctx;

static void push_hist(app_ctx_t *ctx, float ax, float ay, float az, float gx)
{
    ctx->ax_hist[ctx->hist_idx] = ax;
    ctx->ay_hist[ctx->hist_idx] = ay;
    ctx->az_hist[ctx->hist_idx] = az;
    ctx->gx_hist[ctx->hist_idx] = gx;
    ctx->hist_idx = (ctx->hist_idx + 1) % HISTORY_LEN;
    if (ctx->hist_count < HISTORY_LEN) ctx->hist_count++;
}

static float variance(float *buf, int count, int head)
{
    if (count < 5) return 0;
    int n = (count > 15) ? 15 : count;
    float sum = 0, sum2 = 0;
    for (int i = 0; i < n; i++) {
        int idx = (head - n + i + HISTORY_LEN) % HISTORY_LEN;
        sum += buf[idx];
        sum2 += buf[idx] * buf[idx];
    }
    float mean = sum / (float)n;
    return sum2 / (float)n - mean * mean;
}

static gesture_t detect_gesture(app_ctx_t *ctx, float ax, float ay, float az, float gx, float gy, float gz)
{
    if (ctx->cooldown > 0) return GEST_NONE;

    float mag = sqrtf(ax * ax + ay * ay + az * az);
    float ax_var = variance(ctx->ax_hist, ctx->hist_count, ctx->hist_idx);
    float ay_var = variance(ctx->ay_hist, ctx->hist_count, ctx->hist_idx);
    float az_var = variance(ctx->az_hist, ctx->hist_count, ctx->hist_idx);
    float total_var = ax_var + ay_var + az_var;

    /* Shake: high variance in acceleration */
    if (total_var > 0.3f) return GEST_SHAKE;

    /* Flip: Z-axis negative (board upside down) */
    if (az < -0.7f) return GEST_FLIP;

    /* Tilt left/right */
    if (ax < -0.6f) return GEST_TILT_LEFT;
    if (ax > 0.6f) return GEST_TILT_RIGHT;

    /* Tap: sudden acceleration spike then return */
    if (mag > 2.0f) return GEST_TAP;

    /* Rotate: high gyro rate */
    float gyro_mag = sqrtf(gx * gx + gy * gy + gz * gz);
    if (gyro_mag > 150.0f) return GEST_ROTATE;

    return GEST_NONE;
}

static void show_gesture(app_ctx_t *ctx, gesture_t g)
{
    if (g == GEST_NONE) return;

    int idx = (int)g - 1;
    ctx->gestures[idx].count++;
    ctx->gestures[idx].last_tick = ctx->tick;
    ctx->last_gesture = g;
    ctx->cooldown = 15;  /* ~500ms cooldown */

    char buf[48];
    snprintf(buf, sizeof(buf), "%s  %s",
             ctx->gestures[idx].icon, ctx->gestures[idx].name);
    lv_label_set_text(ctx->gesture_label, buf);
    lv_obj_set_style_text_color(ctx->gesture_label, ctx->gestures[idx].color, 0);

    snprintf(buf, sizeof(buf), "%d", ctx->gestures[idx].count);
    lv_label_set_text(ctx->count_labels[idx], buf);

    /* Flash indicator */
    lv_bar_set_value(ctx->indicator_bars[idx], 100, LV_ANIM_OFF);
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    ctx->tick++;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

#if BSP_HAS_BMI270
    float ax = (float)snap.bmi270.ax / 16384.0f;
    float ay = (float)snap.bmi270.ay / 16384.0f;
    float az = (float)snap.bmi270.az / 16384.0f;
    float gx = (float)snap.bmi270.gx / 16.4f;
    float gy = (float)snap.bmi270.gy / 16.4f;
    float gz = (float)snap.bmi270.gz / 16.4f;
#else
    float ax = 0, ay = 0, az = 1.0f, gx = 0, gy = 0, gz = 0;
#endif

    push_hist(ctx, ax, ay, az, gx);

    if (ctx->cooldown > 0) ctx->cooldown--;

    gesture_t g = detect_gesture(ctx, ax, ay, az, gx, gy, gz);
    show_gesture(ctx, g);

    /* Update raw values */
    char buf[32];
    snprintf(buf, sizeof(buf), "aX: %+.2f g", (double)ax);
    lv_label_set_text(ctx->raw_ax, buf);
    snprintf(buf, sizeof(buf), "aY: %+.2f g", (double)ay);
    lv_label_set_text(ctx->raw_ay, buf);
    snprintf(buf, sizeof(buf), "aZ: %+.2f g", (double)az);
    lv_label_set_text(ctx->raw_az, buf);
    snprintf(buf, sizeof(buf), "gX: %+.0f /s", (double)gx);
    lv_label_set_text(ctx->raw_gx, buf);
    snprintf(buf, sizeof(buf), "gY: %+.0f /s", (double)gy);
    lv_label_set_text(ctx->raw_gy, buf);
    snprintf(buf, sizeof(buf), "gZ: %+.0f /s", (double)gz);
    lv_label_set_text(ctx->raw_gz, buf);

    /* Magnitude */
    float mag = sqrtf(ax * ax + ay * ay + az * az);
    int mag_pct = (int)(mag / 3.0f * 100.0f);
    if (mag_pct > 100) mag_pct = 100;
    lv_bar_set_value(ctx->magnitude_bar, mag_pct, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%.2f g", (double)mag);
    lv_label_set_text(ctx->mag_label, buf);

    /* Chart */
    lv_chart_set_next_value(ctx->chart, ctx->mag_series, (lv_coord_t)(mag * 100.0f));

    /* Cooldown bar */
    lv_bar_set_value(ctx->cooldown_bar, (int)(ctx->cooldown * 100 / 15), LV_ANIM_ON);

    /* Fade indicator bars */
    for (int i = 0; i < NUM_GESTURES; i++) {
        int32_t v = lv_bar_get_value(ctx->indicator_bars[i]);
        if (v > 0) lv_bar_set_value(ctx->indicator_bars[i], v - 5, LV_ANIM_ON);
    }
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    ctx->gestures[0] = (gesture_info_t){ "SHAKE",       LV_SYMBOL_REFRESH,   UI_COLOR_ERROR,   0, 0 };
    ctx->gestures[1] = (gesture_info_t){ "TILT LEFT",   LV_SYMBOL_LEFT,      UI_COLOR_INFO,    0, 0 };
    ctx->gestures[2] = (gesture_info_t){ "TILT RIGHT",  LV_SYMBOL_RIGHT,     UI_COLOR_WARNING, 0, 0 };
    ctx->gestures[3] = (gesture_info_t){ "FLIP",        LV_SYMBOL_LOOP,      UI_COLOR_BMM350,  0, 0 };
    ctx->gestures[4] = (gesture_info_t){ "TAP",         LV_SYMBOL_OK,        UI_COLOR_SUCCESS, 0, 0 };
    ctx->gestures[5] = (gesture_info_t){ "ROTATE",      LV_SYMBOL_REFRESH,   UI_COLOR_PRIMARY, 0, 0 };

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_GPS " Gesture Recognizer");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, 14, 6);

    /* Main gesture display */
    lv_obj_t *main_card = lv_obj_create(parent);
    lv_obj_set_size(main_card, 380, 100);
    lv_obj_set_pos(main_card, 10, 36);
    lv_obj_set_style_bg_color(main_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(main_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(main_card, 12, 0);
    lv_obj_set_style_border_width(main_card, 1, 0);
    lv_obj_set_style_border_color(main_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(main_card, 14, 0);
    lv_obj_clear_flag(main_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *dh = lv_label_create(main_card);
    lv_label_set_text(dh, "DETECTED GESTURE");
    lv_obj_set_style_text_color(dh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(dh, &lv_font_montserrat_14, 0);

    ctx->gesture_label = lv_label_create(main_card);
    lv_label_set_text(ctx->gesture_label, "Waiting...");
    lv_obj_set_style_text_color(ctx->gesture_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->gesture_label, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(ctx->gesture_label, 0, 28);

    /* Cooldown bar */
    lv_obj_t *cdh = lv_label_create(main_card);
    lv_label_set_text(cdh, "Cooldown:");
    lv_obj_set_style_text_color(cdh, lv_color_hex(0x546e7a), 0);
    lv_obj_set_style_text_font(cdh, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(cdh, 0, 64);

    ctx->cooldown_bar = lv_bar_create(main_card);
    lv_obj_set_size(ctx->cooldown_bar, 250, 10);
    lv_obj_set_pos(ctx->cooldown_bar, 90, 68);
    lv_bar_set_range(ctx->cooldown_bar, 0, 100);
    lv_obj_set_style_bg_color(ctx->cooldown_bar, lv_color_hex(0x263238), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ctx->cooldown_bar, UI_COLOR_WARNING, LV_PART_INDICATOR);

    /* Gesture counters (right top) */
    lv_obj_t *cnt_card = lv_obj_create(parent);
    lv_obj_set_size(cnt_card, 380, 100);
    lv_obj_set_pos(cnt_card, 400, 36);
    lv_obj_set_style_bg_color(cnt_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(cnt_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cnt_card, 12, 0);
    lv_obj_set_style_border_width(cnt_card, 1, 0);
    lv_obj_set_style_border_color(cnt_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(cnt_card, 10, 0);
    lv_obj_clear_flag(cnt_card, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < NUM_GESTURES; i++) {
        int col = i / 3;
        int row = i % 3;
        lv_coord_t gx = col * 190;
        lv_coord_t gy = row * 28;

        lv_obj_t *gn = lv_label_create(cnt_card);
        lv_label_set_text(gn, ctx->gestures[i].name);
        lv_obj_set_style_text_color(gn, ctx->gestures[i].color, 0);
        lv_obj_set_style_text_font(gn, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(gn, gx, gy);

        ctx->count_labels[i] = lv_label_create(cnt_card);
        lv_obj_set_style_text_color(ctx->count_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->count_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->count_labels[i], gx + 110, gy);
        lv_label_set_text(ctx->count_labels[i], "0");

        ctx->indicator_bars[i] = lv_bar_create(cnt_card);
        lv_obj_set_size(ctx->indicator_bars[i], 40, 6);
        lv_obj_set_pos(ctx->indicator_bars[i], gx + 130, gy + 6);
        lv_bar_set_range(ctx->indicator_bars[i], 0, 100);
        lv_obj_set_style_bg_color(ctx->indicator_bars[i], lv_color_hex(0x1a2332), LV_PART_MAIN);
        lv_obj_set_style_bg_color(ctx->indicator_bars[i], ctx->gestures[i].color, LV_PART_INDICATOR);
    }

    /* Raw data + magnitude (bottom left) */
    lv_obj_t *raw_card = lv_obj_create(parent);
    lv_obj_set_size(raw_card, 280, 260);
    lv_obj_set_pos(raw_card, 10, 146);
    lv_obj_set_style_bg_color(raw_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(raw_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(raw_card, 12, 0);
    lv_obj_set_style_border_width(raw_card, 1, 0);
    lv_obj_set_style_border_color(raw_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(raw_card, 10, 0);
    lv_obj_clear_flag(raw_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *rh = lv_label_create(raw_card);
    lv_label_set_text(rh, "RAW SENSOR DATA");
    lv_obj_set_style_text_color(rh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(rh, &lv_font_montserrat_14, 0);

    lv_obj_t **raw_labels[] = {
        &ctx->raw_ax, &ctx->raw_ay, &ctx->raw_az,
        &ctx->raw_gx, &ctx->raw_gy, &ctx->raw_gz
    };
    for (int i = 0; i < 6; i++) {
        *raw_labels[i] = lv_label_create(raw_card);
        lv_obj_set_style_text_color(*raw_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(*raw_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(*raw_labels[i], (i / 3) * 140, 22 + (i % 3) * 20);
        lv_label_set_text(*raw_labels[i], "---");
    }

    /* Magnitude bar */
    lv_obj_t *mh = lv_label_create(raw_card);
    lv_label_set_text(mh, "Magnitude:");
    lv_obj_set_style_text_color(mh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(mh, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(mh, 0, 90);

    ctx->magnitude_bar = lv_bar_create(raw_card);
    lv_obj_set_size(ctx->magnitude_bar, 200, 16);
    lv_obj_set_pos(ctx->magnitude_bar, 0, 112);
    lv_bar_set_range(ctx->magnitude_bar, 0, 100);
    lv_obj_set_style_bg_color(ctx->magnitude_bar, lv_color_hex(0x263238), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ctx->magnitude_bar, UI_COLOR_PRIMARY, LV_PART_INDICATOR);

    ctx->mag_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->mag_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->mag_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->mag_label, 210, 110);

    /* Instructions */
    lv_obj_t *instr = lv_label_create(raw_card);
    lv_label_set_text(instr,
        "Gestures detected:\n"
        " Shake: rapid movement\n"
        " Tilt L/R: lean board\n"
        " Flip: turn upside down\n"
        " Tap: sharp impact\n"
        " Rotate: spin board");
    lv_obj_set_style_text_color(instr, lv_color_hex(0x546e7a), 0);
    lv_obj_set_style_text_font(instr, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(instr, 0, 140);

    /* Magnitude chart (bottom right) */
    lv_obj_t *chart_card = lv_obj_create(parent);
    lv_obj_set_size(chart_card, 490, 260);
    lv_obj_set_pos(chart_card, 300, 146);
    lv_obj_set_style_bg_color(chart_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(chart_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(chart_card, 12, 0);
    lv_obj_set_style_border_width(chart_card, 1, 0);
    lv_obj_set_style_border_color(chart_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(chart_card, 8, 0);
    lv_obj_clear_flag(chart_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cth = lv_label_create(chart_card);
    lv_label_set_text(cth, "ACCELERATION MAGNITUDE");
    lv_obj_set_style_text_color(cth, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(cth, &lv_font_montserrat_14, 0);

    ctx->chart = lv_chart_create(chart_card);
    lv_obj_set_size(ctx->chart, 470, 220);
    lv_obj_align(ctx->chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, 100);
    lv_chart_set_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, 0, 300);
    lv_obj_set_style_bg_color(ctx->chart, lv_color_hex(0x0d1117), 0);
    lv_obj_set_style_size(ctx->chart, 0, 0, LV_PART_INDICATOR);

    ctx->mag_series = lv_chart_add_series(ctx->chart, UI_COLOR_PRIMARY,
                                           LV_CHART_AXIS_PRIMARY_Y);

    lv_timer_create(timer_cb, SAMPLE_MS, ctx);
}
