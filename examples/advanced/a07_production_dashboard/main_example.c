/**
 * A07 — Production Dashboard
 *
 * Factory-style multi-zone dashboard with KPIs, sparkline charts,
 * production counters, and alert indicators.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define REFRESH_MS      500
#define SPARK_POINTS    30
#define NUM_ZONES       3

typedef struct {
    const char *name;
    lv_color_t  color;
    int         produced;
    int         target;
    float       efficiency;
    float       temp;
    bool        running;
    int         defects;
} zone_t;

typedef struct {
    lv_obj_t   *parent;
    zone_t      zones[NUM_ZONES];
    lv_obj_t   *zone_cards[NUM_ZONES];
    lv_obj_t   *prod_labels[NUM_ZONES];
    lv_obj_t   *eff_bars[NUM_ZONES];
    lv_obj_t   *eff_labels[NUM_ZONES];
    lv_obj_t   *temp_labels[NUM_ZONES];
    lv_obj_t   *status_dots[NUM_ZONES];
    lv_obj_t   *defect_labels[NUM_ZONES];
    lv_obj_t   *total_prod_label;
    lv_obj_t   *total_eff_label;
    lv_obj_t   *uptime_label;
    lv_obj_t   *oee_label;
    lv_obj_t   *chart;
    lv_chart_series_t *spark_series[NUM_ZONES];
    lv_obj_t   *time_label;
    uint32_t    tick;
} app_ctx_t;

static app_ctx_t g_ctx;

static void init_zones(app_ctx_t *ctx)
{
    ctx->zones[0] = (zone_t){ "Zone A — Assembly", UI_COLOR_BMI270,  0, 500, 0, 0, true, 0 };
    ctx->zones[1] = (zone_t){ "Zone B — Solder",   UI_COLOR_DPS368,  0, 350, 0, 0, true, 0 };
    ctx->zones[2] = (zone_t){ "Zone C — QC Test",  UI_COLOR_SHT40,   0, 400, 0, 0, true, 0 };
}

static void simulate_production(app_ctx_t *ctx)
{
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    /* Use real sensor data to drive simulation variation */
    float jitter = (float)(snap.bmi270.ax & 0xFF) / 255.0f;

    for (int i = 0; i < NUM_ZONES; i++) {
        zone_t *z = &ctx->zones[i];
        if (!z->running) continue;

        int batch = 3 + (int)(jitter * 5.0f) + i;
        z->produced += batch;
        if (z->produced > z->target * 2) z->produced = z->target * 2;

        z->efficiency = (float)z->produced / (float)z->target * 100.0f;
        if (z->efficiency > 150.0f) z->efficiency = 150.0f;

#if BSP_HAS_SHT40
        z->temp = (float)snap.dps368.temperature_x100 / 100.0f + (float)i * 2.0f;
#elif BSP_HAS_DPS368
        z->temp = (float)snap.dps368.temperature_x100 / 100.0f + (float)i * 2.0f;
#else
        z->temp = 25.0f + jitter * 5.0f + (float)i * 2.0f;
#endif

        if (((int)(jitter * 100.0f) + i) % 7 == 0) {
            z->defects++;
        }

        /* Random pause/resume */
        if (ctx->tick % 40 == (uint32_t)(i * 13 + 7)) {
            z->running = !z->running;
        }
        if (ctx->tick % 40 == (uint32_t)(i * 13 + 12)) {
            z->running = true;
        }
    }
}

static void update_ui(app_ctx_t *ctx)
{
    int total_prod = 0;
    float total_eff = 0;

    for (int i = 0; i < NUM_ZONES; i++) {
        zone_t *z = &ctx->zones[i];
        total_prod += z->produced;
        total_eff += z->efficiency;

        char buf[48];
        snprintf(buf, sizeof(buf), "%d / %d", z->produced, z->target);
        lv_label_set_text(ctx->prod_labels[i], buf);

        int eff_val = (int)z->efficiency;
        if (eff_val > 100) eff_val = 100;
        lv_bar_set_value(ctx->eff_bars[i], eff_val, LV_ANIM_ON);

        snprintf(buf, sizeof(buf), "%.0f%%", (double)z->efficiency);
        lv_label_set_text(ctx->eff_labels[i], buf);
        lv_obj_set_style_text_color(ctx->eff_labels[i],
            z->efficiency >= 90.0f ? UI_COLOR_SUCCESS :
            z->efficiency >= 70.0f ? UI_COLOR_WARNING : UI_COLOR_ERROR, 0);

        snprintf(buf, sizeof(buf), "%.1f C", (double)z->temp);
        lv_label_set_text(ctx->temp_labels[i], buf);

        lv_obj_set_style_bg_color(ctx->status_dots[i],
            z->running ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);

        snprintf(buf, sizeof(buf), "Defects: %d", z->defects);
        lv_label_set_text(ctx->defect_labels[i], buf);

        lv_chart_set_next_value(ctx->chart, ctx->spark_series[i],
            (lv_coord_t)z->efficiency);
    }

    char buf[48];
    snprintf(buf, sizeof(buf), "Total: %d units", total_prod);
    lv_label_set_text(ctx->total_prod_label, buf);

    float avg_eff = total_eff / (float)NUM_ZONES;
    snprintf(buf, sizeof(buf), "Avg Efficiency: %.1f%%", (double)avg_eff);
    lv_label_set_text(ctx->total_eff_label, buf);

    uint32_t mins = ctx->tick * REFRESH_MS / 60000;
    uint32_t secs = (ctx->tick * REFRESH_MS / 1000) % 60;
    snprintf(buf, sizeof(buf), "Uptime: %lu:%02lu", (unsigned long)mins, (unsigned long)secs);
    lv_label_set_text(ctx->uptime_label, buf);

    float oee = avg_eff * 0.95f * 0.98f / 100.0f;
    snprintf(buf, sizeof(buf), "OEE: %.1f%%", (double)(oee * 100.0f));
    lv_label_set_text(ctx->oee_label, buf);

    char tbuf[32];
    ipc_sensorhub_get_time_str(tbuf, sizeof(tbuf));
    lv_label_set_text(ctx->time_label, tbuf);
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    ctx->tick++;
    simulate_production(ctx);
    update_ui(ctx);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;
    init_zones(ctx);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Top bar */
    lv_obj_t *top = lv_obj_create(parent);
    lv_obj_set_size(top, 780, 44);
    lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(top, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top, 8, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(top);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Production Dashboard");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    ctx->time_label = lv_label_create(top);
    lv_obj_set_style_text_color(ctx->time_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->time_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->time_label, LV_ALIGN_RIGHT_MID, -10, 0);

    /* KPI row */
    lv_coord_t kpi_y = 54;
    lv_coord_t kpi_w = 186;

    const char *kpi_titles[] = {"TOTAL OUTPUT", "EFFICIENCY", "UPTIME", "OEE"};
    lv_obj_t **kpi_labels[] = {
        &ctx->total_prod_label, &ctx->total_eff_label,
        &ctx->uptime_label, &ctx->oee_label
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *kc = lv_obj_create(parent);
        lv_obj_set_size(kc, kpi_w, 60);
        lv_obj_set_pos(kc, 10 + i * (kpi_w + 8), kpi_y);
        lv_obj_set_style_bg_color(kc, UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_bg_opa(kc, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(kc, 10, 0);
        lv_obj_set_style_border_width(kc, 1, 0);
        lv_obj_set_style_border_color(kc, lv_color_hex(0x2a3a5c), 0);
        lv_obj_set_style_pad_all(kc, 8, 0);
        lv_obj_clear_flag(kc, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *kh = lv_label_create(kc);
        lv_label_set_text(kh, kpi_titles[i]);
        lv_obj_set_style_text_color(kh, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(kh, &lv_font_montserrat_14, 0);

        *kpi_labels[i] = lv_label_create(kc);
        lv_obj_set_style_text_color(*kpi_labels[i], UI_COLOR_PRIMARY, 0);
        lv_obj_set_style_text_font(*kpi_labels[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(*kpi_labels[i], 0, 22);
        lv_label_set_text(*kpi_labels[i], "---");
    }

    /* Zone cards */
    lv_coord_t zone_y = 124;
    lv_coord_t zone_w = 250;
    lv_coord_t zone_h = 170;

    for (int i = 0; i < NUM_ZONES; i++) {
        lv_obj_t *zc = lv_obj_create(parent);
        lv_obj_set_size(zc, zone_w, zone_h);
        lv_obj_set_pos(zc, 10 + i * (zone_w + 10), zone_y);
        lv_obj_set_style_bg_color(zc, UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_bg_opa(zc, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(zc, 12, 0);
        lv_obj_set_style_border_width(zc, 1, 0);
        lv_obj_set_style_border_color(zc, lv_color_hex(0x2a3a5c), 0);
        lv_obj_set_style_pad_all(zc, 10, 0);
        lv_obj_clear_flag(zc, LV_OBJ_FLAG_SCROLLABLE);
        ctx->zone_cards[i] = zc;

        /* Status dot */
        ctx->status_dots[i] = lv_obj_create(zc);
        lv_obj_set_size(ctx->status_dots[i], 10, 10);
        lv_obj_set_pos(ctx->status_dots[i], 0, 3);
        lv_obj_set_style_bg_color(ctx->status_dots[i], UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_bg_opa(ctx->status_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->status_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(ctx->status_dots[i], 0, 0);
        lv_obj_clear_flag(ctx->status_dots[i], LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *zn = lv_label_create(zc);
        lv_label_set_text(zn, ctx->zones[i].name);
        lv_obj_set_style_text_color(zn, ctx->zones[i].color, 0);
        lv_obj_set_style_text_font(zn, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(zn, 16, 0);

        /* Production count */
        lv_obj_t *ph = lv_label_create(zc);
        lv_label_set_text(ph, "Output:");
        lv_obj_set_style_text_color(ph, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(ph, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ph, 0, 24);

        ctx->prod_labels[i] = lv_label_create(zc);
        lv_obj_set_style_text_color(ctx->prod_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->prod_labels[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(ctx->prod_labels[i], 70, 22);

        /* Efficiency bar */
        ctx->eff_bars[i] = lv_bar_create(zc);
        lv_obj_set_size(ctx->eff_bars[i], 170, 14);
        lv_obj_set_pos(ctx->eff_bars[i], 0, 50);
        lv_bar_set_range(ctx->eff_bars[i], 0, 100);
        lv_obj_set_style_bg_color(ctx->eff_bars[i], lv_color_hex(0x1a2332), LV_PART_MAIN);
        lv_obj_set_style_bg_color(ctx->eff_bars[i], ctx->zones[i].color, LV_PART_INDICATOR);

        ctx->eff_labels[i] = lv_label_create(zc);
        lv_obj_set_style_text_font(ctx->eff_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->eff_labels[i], 180, 48);

        /* Temp */
        ctx->temp_labels[i] = lv_label_create(zc);
        lv_obj_set_style_text_color(ctx->temp_labels[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->temp_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->temp_labels[i], 0, 76);

        /* Defects */
        ctx->defect_labels[i] = lv_label_create(zc);
        lv_obj_set_style_text_color(ctx->defect_labels[i], UI_COLOR_WARNING, 0);
        lv_obj_set_style_text_font(ctx->defect_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->defect_labels[i], 0, 98);
    }

    /* Trend chart (bottom) */
    lv_obj_t *chart_card = lv_obj_create(parent);
    lv_obj_set_size(chart_card, 780, 120);
    lv_obj_set_pos(chart_card, 10, 304);
    lv_obj_set_style_bg_color(chart_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(chart_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(chart_card, 12, 0);
    lv_obj_set_style_border_width(chart_card, 1, 0);
    lv_obj_set_style_border_color(chart_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(chart_card, 8, 0);
    lv_obj_clear_flag(chart_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cth = lv_label_create(chart_card);
    lv_label_set_text(cth, "EFFICIENCY TREND");
    lv_obj_set_style_text_color(cth, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(cth, &lv_font_montserrat_14, 0);

    ctx->chart = lv_chart_create(chart_card);
    lv_obj_set_size(ctx->chart, 740, 80);
    lv_obj_align(ctx->chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, SPARK_POINTS);
    lv_chart_set_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, 0, 150);
    lv_obj_set_style_bg_color(ctx->chart, lv_color_hex(0x0d1117), 0);
    lv_obj_set_style_size(ctx->chart, 0, 0, LV_PART_INDICATOR);

    for (int i = 0; i < NUM_ZONES; i++) {
        ctx->spark_series[i] = lv_chart_add_series(ctx->chart,
            ctx->zones[i].color, LV_CHART_AXIS_PRIMARY_Y);
    }

    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
