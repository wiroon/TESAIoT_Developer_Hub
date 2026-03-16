/**
 * A15 — Weather Station
 *
 * Complete weather station with temperature, humidity, pressure,
 * trend arrows, forecast indicator, and historical charts.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define REFRESH_MS      2000
#define HISTORY_LEN     60
#define CARD_W          250
#define CARD_H          140

typedef struct {
    float   history[HISTORY_LEN];
    int     head;
    int     count;
    float   current;
    float   prev;
    float   trend;  /* per minute */
} weather_ch_t;

typedef struct {
    lv_obj_t      *parent;
    weather_ch_t   temp;
    weather_ch_t   humidity;
    weather_ch_t   pressure;
    lv_obj_t      *temp_big;
    lv_obj_t      *temp_trend;
    lv_obj_t      *hum_big;
    lv_obj_t      *hum_trend;
    lv_obj_t      *press_big;
    lv_obj_t      *press_trend;
    lv_obj_t      *forecast_label;
    lv_obj_t      *forecast_icon;
    lv_obj_t      *dewpoint_label;
    lv_obj_t      *heatindex_label;
    lv_obj_t      *altitude_label;
    lv_obj_t      *chart;
    lv_chart_series_t *temp_series;
    lv_chart_series_t *hum_series;
    lv_chart_series_t *press_series;
    lv_obj_t      *time_label;
    lv_obj_t      *comfort_label;
} app_ctx_t;

static app_ctx_t g_ctx;

static void push_weather(weather_ch_t *ch, float val)
{
    ch->prev = ch->current;
    ch->current = val;
    ch->history[ch->head] = val;
    ch->head = (ch->head + 1) % HISTORY_LEN;
    if (ch->count < HISTORY_LEN) ch->count++;

    /* Trend: compare current vs 30 samples ago */
    if (ch->count > 30) {
        int old_idx = (ch->head - 30 + HISTORY_LEN) % HISTORY_LEN;
        ch->trend = (val - ch->history[old_idx]) / 30.0f * 60.0f;
    }
}

static const char *trend_arrow(float trend, float threshold)
{
    if (trend > threshold) return LV_SYMBOL_UP;
    if (trend < -threshold) return LV_SYMBOL_DOWN;
    return "=";
}

static float calc_dewpoint(float t, float rh)
{
    float a = 17.27f, b = 237.7f;
    float gamma = (a * t) / (b + t) + logf(rh / 100.0f);
    return (b * gamma) / (a - gamma);
}

static float calc_heat_index(float t, float rh)
{
    if (t < 27.0f) return t;
    float hi = -8.785f + 1.611f * t + 2.339f * rh - 0.1461f * t * rh
               - 0.01231f * t * t - 0.01642f * rh * rh
               + 0.002212f * t * t * rh + 0.000725f * t * rh * rh
               - 0.00000359f * t * t * rh * rh;
    return hi;
}

static float calc_altitude(float press_hpa)
{
    return 44330.0f * (1.0f - powf(press_hpa / 1013.25f, 0.1903f));
}

static const char *get_forecast(float press_trend, float pressure)
{
    if (press_trend > 0.5f) return "Improving";
    if (press_trend < -0.5f) return "Deteriorating";
    if (pressure > 1020.0f) return "Fair / Clear";
    if (pressure < 1005.0f) return "Storm Possible";
    return "Stable";
}

static const char *get_comfort(float t, float rh)
{
    if (t < 18.0f) return "Cold";
    if (t > 32.0f) return "Hot";
    if (rh > 70.0f) return "Humid";
    if (rh < 30.0f) return "Dry";
    if (t >= 20.0f && t <= 26.0f && rh >= 40.0f && rh <= 60.0f) return "Comfortable";
    return "Acceptable";
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    float temp_val, hum_val, press_val;

#if BSP_HAS_SHT40
    temp_val = (float)snap.dps368.temperature_x100 / 100.0f;
    hum_val  = (float)snap.sht40.humidity_x100 / 100.0f;
#elif BSP_HAS_DPS368
    temp_val = (float)snap.dps368.temperature_x100 / 100.0f;
    hum_val  = 50.0f;
#else
    temp_val = 25.0f;
    hum_val  = 50.0f;
#endif

#if BSP_HAS_DPS368
    press_val = (float)snap.dps368.pressure_x100 / 100.0f;
#else
    press_val = 1013.25f;
#endif

    push_weather(&ctx->temp, temp_val);
    push_weather(&ctx->humidity, hum_val);
    push_weather(&ctx->pressure, press_val);

    char buf[48];

    /* Temperature */
    snprintf(buf, sizeof(buf), "%.1f", (double)temp_val);
    lv_label_set_text(ctx->temp_big, buf);
    snprintf(buf, sizeof(buf), "%s %.2f C/min",
             trend_arrow(ctx->temp.trend, 0.05f), (double)ctx->temp.trend);
    lv_label_set_text(ctx->temp_trend, buf);

    /* Humidity */
    snprintf(buf, sizeof(buf), "%.0f", (double)hum_val);
    lv_label_set_text(ctx->hum_big, buf);
    snprintf(buf, sizeof(buf), "%s %.1f %%/min",
             trend_arrow(ctx->humidity.trend, 0.5f), (double)ctx->humidity.trend);
    lv_label_set_text(ctx->hum_trend, buf);

    /* Pressure */
    snprintf(buf, sizeof(buf), "%.1f", (double)press_val);
    lv_label_set_text(ctx->press_big, buf);
    snprintf(buf, sizeof(buf), "%s %.2f hPa/min",
             trend_arrow(ctx->pressure.trend, 0.1f), (double)ctx->pressure.trend);
    lv_label_set_text(ctx->press_trend, buf);

    /* Derived values */
    float dp = calc_dewpoint(temp_val, hum_val);
    snprintf(buf, sizeof(buf), "Dew Point: %.1f C", (double)dp);
    lv_label_set_text(ctx->dewpoint_label, buf);

    float hi = calc_heat_index(temp_val, hum_val);
    snprintf(buf, sizeof(buf), "Heat Index: %.1f C", (double)hi);
    lv_label_set_text(ctx->heatindex_label, buf);

    float alt = calc_altitude(press_val);
    snprintf(buf, sizeof(buf), "Altitude: %.0f m", (double)alt);
    lv_label_set_text(ctx->altitude_label, buf);

    /* Forecast */
    lv_label_set_text(ctx->forecast_label, get_forecast(ctx->pressure.trend, press_val));

    /* Comfort */
    lv_label_set_text(ctx->comfort_label, get_comfort(temp_val, hum_val));

    /* Charts */
    lv_chart_set_next_value(ctx->chart, ctx->temp_series, (lv_coord_t)(temp_val * 10.0f));
    lv_chart_set_next_value(ctx->chart, ctx->hum_series, (lv_coord_t)(hum_val));
    lv_chart_set_next_value(ctx->chart, ctx->press_series,
        (lv_coord_t)((press_val - 980.0f) * 2.0f));

    char tbuf[32];
    ipc_sensorhub_get_time_str(tbuf, sizeof(tbuf));
    lv_label_set_text(ctx->time_label, tbuf);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_GPS " Weather Station");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, 14, 6);

    ctx->time_label = lv_label_create(parent);
    lv_obj_set_style_text_color(ctx->time_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->time_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->time_label, 680, 12);

    /* Temperature card */
    lv_obj_t *tc = lv_obj_create(parent);
    lv_obj_set_size(tc, CARD_W, CARD_H);
    lv_obj_set_pos(tc, 10, 36);
    lv_obj_set_style_bg_color(tc, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(tc, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tc, 12, 0);
    lv_obj_set_style_border_width(tc, 1, 0);
    lv_obj_set_style_border_color(tc, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(tc, 12, 0);
    lv_obj_clear_flag(tc, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *th = lv_label_create(tc);
    lv_label_set_text(th, "TEMPERATURE");
    lv_obj_set_style_text_color(th, UI_COLOR_DPS368, 0);
    lv_obj_set_style_text_font(th, &lv_font_montserrat_14, 0);

    ctx->temp_big = lv_label_create(tc);
    lv_label_set_text(ctx->temp_big, "--.-");
    lv_obj_set_style_text_color(ctx->temp_big, UI_COLOR_DPS368, 0);
    lv_obj_set_style_text_font(ctx->temp_big, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(ctx->temp_big, 0, 26);

    lv_obj_t *tu = lv_label_create(tc);
    lv_label_set_text(tu, "C");
    lv_obj_set_style_text_color(tu, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(tu, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(tu, 120, 30);

    ctx->temp_trend = lv_label_create(tc);
    lv_obj_set_style_text_color(ctx->temp_trend, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ctx->temp_trend, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->temp_trend, 0, 66);

    ctx->dewpoint_label = lv_label_create(tc);
    lv_obj_set_style_text_color(ctx->dewpoint_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->dewpoint_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->dewpoint_label, 0, 90);

    /* Humidity card */
    lv_obj_t *hc = lv_obj_create(parent);
    lv_obj_set_size(hc, CARD_W, CARD_H);
    lv_obj_set_pos(hc, 10 + CARD_W + 10, 36);
    lv_obj_set_style_bg_color(hc, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(hc, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hc, 12, 0);
    lv_obj_set_style_border_width(hc, 1, 0);
    lv_obj_set_style_border_color(hc, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(hc, 12, 0);
    lv_obj_clear_flag(hc, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hh = lv_label_create(hc);
    lv_label_set_text(hh, "HUMIDITY");
    lv_obj_set_style_text_color(hh, UI_COLOR_SHT40, 0);
    lv_obj_set_style_text_font(hh, &lv_font_montserrat_14, 0);

    ctx->hum_big = lv_label_create(hc);
    lv_label_set_text(ctx->hum_big, "--");
    lv_obj_set_style_text_color(ctx->hum_big, UI_COLOR_SHT40, 0);
    lv_obj_set_style_text_font(ctx->hum_big, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(ctx->hum_big, 0, 26);

    lv_obj_t *hu = lv_label_create(hc);
    lv_label_set_text(hu, "%RH");
    lv_obj_set_style_text_color(hu, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(hu, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(hu, 80, 30);

    ctx->hum_trend = lv_label_create(hc);
    lv_obj_set_style_text_color(ctx->hum_trend, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ctx->hum_trend, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->hum_trend, 0, 66);

    ctx->heatindex_label = lv_label_create(hc);
    lv_obj_set_style_text_color(ctx->heatindex_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->heatindex_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->heatindex_label, 0, 90);

    /* Pressure card */
    lv_obj_t *pc = lv_obj_create(parent);
    lv_obj_set_size(pc, CARD_W, CARD_H);
    lv_obj_set_pos(pc, 10 + 2 * (CARD_W + 10), 36);
    lv_obj_set_style_bg_color(pc, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(pc, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pc, 12, 0);
    lv_obj_set_style_border_width(pc, 1, 0);
    lv_obj_set_style_border_color(pc, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(pc, 12, 0);
    lv_obj_clear_flag(pc, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ph = lv_label_create(pc);
    lv_label_set_text(ph, "PRESSURE");
    lv_obj_set_style_text_color(ph, UI_COLOR_BMM350, 0);
    lv_obj_set_style_text_font(ph, &lv_font_montserrat_14, 0);

    ctx->press_big = lv_label_create(pc);
    lv_label_set_text(ctx->press_big, "----.-");
    lv_obj_set_style_text_color(ctx->press_big, UI_COLOR_BMM350, 0);
    lv_obj_set_style_text_font(ctx->press_big, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(ctx->press_big, 0, 26);

    lv_obj_t *pu = lv_label_create(pc);
    lv_label_set_text(pu, "hPa");
    lv_obj_set_style_text_color(pu, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(pu, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(pu, 170, 34);

    ctx->press_trend = lv_label_create(pc);
    lv_obj_set_style_text_color(ctx->press_trend, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ctx->press_trend, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->press_trend, 0, 66);

    ctx->altitude_label = lv_label_create(pc);
    lv_obj_set_style_text_color(ctx->altitude_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->altitude_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->altitude_label, 0, 90);

    /* Forecast and Comfort (bottom left) */
    lv_obj_t *fc = lv_obj_create(parent);
    lv_obj_set_size(fc, 240, 100);
    lv_obj_set_pos(fc, 10, 186);
    lv_obj_set_style_bg_color(fc, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(fc, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(fc, 12, 0);
    lv_obj_set_style_border_width(fc, 1, 0);
    lv_obj_set_style_border_color(fc, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(fc, 12, 0);
    lv_obj_clear_flag(fc, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *fh = lv_label_create(fc);
    lv_label_set_text(fh, "FORECAST");
    lv_obj_set_style_text_color(fh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(fh, &lv_font_montserrat_14, 0);

    ctx->forecast_label = lv_label_create(fc);
    lv_obj_set_style_text_color(ctx->forecast_label, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(ctx->forecast_label, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(ctx->forecast_label, 0, 24);

    lv_obj_t *cmh = lv_label_create(fc);
    lv_label_set_text(cmh, "Comfort:");
    lv_obj_set_style_text_color(cmh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(cmh, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(cmh, 0, 56);

    ctx->comfort_label = lv_label_create(fc);
    lv_obj_set_style_text_color(ctx->comfort_label, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(ctx->comfort_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->comfort_label, 80, 54);

    /* History chart */
    lv_obj_t *chart_card = lv_obj_create(parent);
    lv_obj_set_size(chart_card, 520, 140);
    lv_obj_set_pos(chart_card, 260, 186);
    lv_obj_set_style_bg_color(chart_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(chart_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(chart_card, 12, 0);
    lv_obj_set_style_border_width(chart_card, 1, 0);
    lv_obj_set_style_border_color(chart_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(chart_card, 8, 0);
    lv_obj_clear_flag(chart_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ch_title = lv_label_create(chart_card);
    lv_label_set_text(ch_title, "HISTORY (60 samples)");
    lv_obj_set_style_text_color(ch_title, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ch_title, &lv_font_montserrat_14, 0);

    ctx->chart = lv_chart_create(chart_card);
    lv_obj_set_size(ctx->chart, 500, 100);
    lv_obj_align(ctx->chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, HISTORY_LEN);
    lv_chart_set_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, 0, 500);
    lv_obj_set_style_bg_color(ctx->chart, lv_color_hex(0x0d1117), 0);
    lv_obj_set_style_size(ctx->chart, 0, 0, LV_PART_INDICATOR);

    ctx->temp_series = lv_chart_add_series(ctx->chart, UI_COLOR_DPS368,
                                            LV_CHART_AXIS_PRIMARY_Y);
    ctx->hum_series = lv_chart_add_series(ctx->chart, UI_COLOR_SHT40,
                                           LV_CHART_AXIS_PRIMARY_Y);
    ctx->press_series = lv_chart_add_series(ctx->chart, UI_COLOR_BMM350,
                                             LV_CHART_AXIS_PRIMARY_Y);

    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
