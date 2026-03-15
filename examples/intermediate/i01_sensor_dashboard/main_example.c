/**
 * I01 — Sensor Dashboard
 *
 * Full 4-card sensor dashboard with live IMU, barometer, climate, and compass
 * readings updated every 100 ms via IPC snapshot.
 */
#include "example_common.h"

/* ── Per-card label handles ─────────────────────────────────────────────── */
typedef struct {
    /* IMU */
    lv_obj_t *lbl_ax, *lbl_ay, *lbl_az;
    lv_obj_t *lbl_gx, *lbl_gy, *lbl_gz;
    /* Barometer */
    lv_obj_t *lbl_press, *lbl_baro_temp;
    /* Climate */
    lv_obj_t *lbl_clim_temp, *lbl_humidity;
    /* Compass */
    lv_obj_t *lbl_heading;
} dash_ctx_t;

/* ── Timer callback ─────────────────────────────────────────────────────── */
static void timer_cb(lv_timer_t *t)
{
    dash_ctx_t *ctx = (dash_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

#if BSP_HAS_BMI270
    float ax = snap.bmi270.ax / 16384.0f;
    float ay = snap.bmi270.ay / 16384.0f;
    float az = snap.bmi270.az / 16384.0f;
    float gx = snap.bmi270.gx / 16.4f;
    float gy = snap.bmi270.gy / 16.4f;
    float gz = snap.bmi270.gz / 16.4f;
    lv_label_set_text_fmt(ctx->lbl_ax, "aX: %.2f g", (double)ax);
    lv_label_set_text_fmt(ctx->lbl_ay, "aY: %.2f g", (double)ay);
    lv_label_set_text_fmt(ctx->lbl_az, "aZ: %.2f g", (double)az);
    lv_label_set_text_fmt(ctx->lbl_gx, "gX: %.1f dps", (double)gx);
    lv_label_set_text_fmt(ctx->lbl_gy, "gY: %.1f dps", (double)gy);
    lv_label_set_text_fmt(ctx->lbl_gz, "gZ: %.1f dps", (double)gz);
#endif

#if BSP_HAS_DPS368
    float press = snap.dps368.pressure_x100 / 100.0f;
    float btemp = snap.dps368.temperature_x100 / 100.0f;
    lv_label_set_text_fmt(ctx->lbl_press, "P: %.1f hPa", (double)press);
    lv_label_set_text_fmt(ctx->lbl_baro_temp, "T: %.1f C", (double)btemp);
#endif

#if BSP_HAS_SHT40
    float ctemp = snap.sht40.temperature_x100 / 100.0f;
    float hum   = snap.sht40.humidity_x100 / 100.0f;
    lv_label_set_text_fmt(ctx->lbl_clim_temp, "T: %.1f C", (double)ctemp);
    lv_label_set_text_fmt(ctx->lbl_humidity, "H: %.1f %%", (double)hum);
#endif

#if BSP_HAS_BMM350
    float hdg = snap.bmm350.heading_x10 / 10.0f;
    lv_label_set_text_fmt(ctx->lbl_heading, "%.1f deg", (double)hdg);
#endif
}

/* ── Helper: create a titled card ───────────────────────────────────────── */
static lv_obj_t *make_card(lv_obj_t *parent, const char *title,
                           lv_coord_t w, lv_coord_t h, lv_color_t accent)
{
    lv_obj_t *card = example_card_create(parent, w, h,
                                         UI_COLOR_CARD_BG);
    lv_obj_set_style_border_color(card, accent, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    example_label_create(card, title, &lv_font_montserrat_16, accent);
    return card;
}

/* ── Entry point ────────────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    static dash_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(parent, 10, 0);
    lv_obj_set_style_pad_row(parent, 10, 0);
    lv_obj_set_style_pad_column(parent, 10, 0);

    /* Title */
    lv_obj_t *title = example_label_create(parent, "Sensor Dashboard",
                                           &lv_font_montserrat_24,
                                           UI_COLOR_PRIMARY);
    lv_obj_set_width(title, lv_pct(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    /* ── Card 1: IMU ─────────────────────────────────────────────────── */
#if BSP_HAS_BMI270
    {
        lv_obj_t *c = make_card(parent, "IMU (BMI270)", 370, 180,
                                UI_COLOR_BMI270);
        ctx.lbl_ax = example_label_create(c, "aX: --", &lv_font_montserrat_14,
                                          UI_COLOR_TEXT);
        ctx.lbl_ay = example_label_create(c, "aY: --", &lv_font_montserrat_14,
                                          UI_COLOR_TEXT);
        ctx.lbl_az = example_label_create(c, "aZ: --", &lv_font_montserrat_14,
                                          UI_COLOR_TEXT);
        ctx.lbl_gx = example_label_create(c, "gX: --", &lv_font_montserrat_14,
                                          UI_COLOR_TEXT_DIM);
        ctx.lbl_gy = example_label_create(c, "gY: --", &lv_font_montserrat_14,
                                          UI_COLOR_TEXT_DIM);
        ctx.lbl_gz = example_label_create(c, "gZ: --", &lv_font_montserrat_14,
                                          UI_COLOR_TEXT_DIM);
    }
#endif

    /* ── Card 2: Barometer ───────────────────────────────────────────── */
#if BSP_HAS_DPS368
    {
        lv_obj_t *c = make_card(parent, "Baro (DPS368)", 370, 180,
                                UI_COLOR_DPS368);
        ctx.lbl_press = example_label_create(c, "P: --", &lv_font_montserrat_20,
                                             UI_COLOR_TEXT);
        ctx.lbl_baro_temp = example_label_create(c, "T: --",
                                                 &lv_font_montserrat_20,
                                                 UI_COLOR_TEXT);
    }
#endif

    /* ── Card 3: Climate ─────────────────────────────────────────────── */
#if BSP_HAS_SHT40
    {
        lv_obj_t *c = make_card(parent, "Climate (SHT40)", 370, 180,
                                UI_COLOR_SHT40);
        ctx.lbl_clim_temp = example_label_create(c, "T: --",
                                                 &lv_font_montserrat_20,
                                                 UI_COLOR_TEXT);
        ctx.lbl_humidity = example_label_create(c, "H: --",
                                                &lv_font_montserrat_20,
                                                UI_COLOR_TEXT);
    }
#endif

    /* ── Card 4: Compass ─────────────────────────────────────────────── */
#if BSP_HAS_BMM350
    {
        lv_obj_t *c = make_card(parent, "Compass (BMM350)", 370, 180,
                                UI_COLOR_BMM350);
        ctx.lbl_heading = example_label_create(c, "--",
                                               &lv_font_montserrat_28,
                                               UI_COLOR_TEXT);
        lv_obj_set_style_text_align(ctx.lbl_heading, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(ctx.lbl_heading, lv_pct(100));
    }
#endif

    lv_timer_create(timer_cb, 100, &ctx);
}
