/**
 * I02 — Line Chart Acceleration
 *
 * Real-time scrolling line chart plotting BMI270 X/Y/Z acceleration.
 * 100-point history with color-coded series.
 */
#include "example_common.h"

#if !BSP_HAS_BMI270
#error "This example requires BMI270"
#endif

#define CHART_POINTS  100
#define UPDATE_MS     50

typedef struct {
    lv_obj_t       *chart;
    lv_chart_series_t *ser_x, *ser_y, *ser_z;
    lv_obj_t       *lbl_x, *lbl_y, *lbl_z;
} chart_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    chart_ctx_t *ctx = (chart_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    float ax = snap.bmi270.ax / 16384.0f;
    float ay = snap.bmi270.ay / 16384.0f;
    float az = snap.bmi270.az / 16384.0f;

    /* Chart expects int values; scale by 100 for 2-decimal precision */
    lv_chart_set_next_value(ctx->chart, ctx->ser_x, (int32_t)(ax * 100));
    lv_chart_set_next_value(ctx->chart, ctx->ser_y, (int32_t)(ay * 100));
    lv_chart_set_next_value(ctx->chart, ctx->ser_z, (int32_t)(az * 100));

    lv_label_set_text_fmt(ctx->lbl_x, "X: %.2f g", (double)ax);
    lv_label_set_text_fmt(ctx->lbl_y, "Y: %.2f g", (double)ay);
    lv_label_set_text_fmt(ctx->lbl_z, "Z: %.2f g", (double)az);
}

void example_main(lv_obj_t *parent)
{
    static chart_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_row(parent, 8, 0);

    /* Title */
    example_label_create(parent, "Accelerometer (BMI270)",
                         &lv_font_montserrat_24,
                         UI_COLOR_BMI270);

    /* Chart */
    ctx.chart = lv_chart_create(parent);
    lv_obj_set_size(ctx.chart, 760, 320);
    lv_chart_set_type(ctx.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx.chart, CHART_POINTS);
    lv_chart_set_range(ctx.chart, LV_CHART_AXIS_PRIMARY_Y, -200, 200);
    lv_chart_set_div_line_count(ctx.chart, 5, 8);
    lv_obj_set_style_bg_color(ctx.chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx.chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.chart, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ctx.chart, 1, 0);
    lv_obj_set_style_line_width(ctx.chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(ctx.chart, 0, 0, LV_PART_INDICATOR);

    ctx.ser_x = lv_chart_add_series(ctx.chart, lv_color_hex(0xF44336),
                                    LV_CHART_AXIS_PRIMARY_Y);
    ctx.ser_y = lv_chart_add_series(ctx.chart, UI_COLOR_SUCCESS,
                                    LV_CHART_AXIS_PRIMARY_Y);
    ctx.ser_z = lv_chart_add_series(ctx.chart, UI_COLOR_INFO,
                                    LV_CHART_AXIS_PRIMARY_Y);

    /* Initialize all points to zero */
    for (int i = 0; i < CHART_POINTS; i++) {
        lv_chart_set_next_value(ctx.chart, ctx.ser_x, 0);
        lv_chart_set_next_value(ctx.chart, ctx.ser_y, 0);
        lv_chart_set_next_value(ctx.chart, ctx.ser_z, 0);
    }

    /* Legend row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 760, 40);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    ctx.lbl_x = example_label_create(row, "X: --",
                                     &lv_font_montserrat_16,
                                     lv_color_hex(0xF44336));
    ctx.lbl_y = example_label_create(row, "Y: --",
                                     &lv_font_montserrat_16,
                                     UI_COLOR_SUCCESS);
    ctx.lbl_z = example_label_create(row, "Z: --",
                                     &lv_font_montserrat_16,
                                     UI_COLOR_INFO);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
