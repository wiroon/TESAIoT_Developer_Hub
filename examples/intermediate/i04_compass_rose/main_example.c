/**
 * I04 — Compass Rose
 *
 * Graphical compass with a rotating needle driven by BMM350 heading.
 * Uses LVGL canvas for drawing the needle and cardinal labels.
 */
#include "example_common.h"

#if !BSP_HAS_BMM350
#error "This example requires BMM350 magnetometer"
#endif

#include <math.h>

#define COMPASS_R     140
#define CENTER_X      160
#define CENTER_Y      160
#define CANVAS_W      320
#define CANVAS_H      320
#define UPDATE_MS     80

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    lv_obj_t *canvas;
    lv_obj_t *lbl_deg;
    lv_obj_t *lbl_dir;
    uint8_t  *cbuf;
} compass_ctx_t;

static const char *heading_to_dir(float deg)
{
    if (deg < 22.5f  || deg >= 337.5f) return "N";
    if (deg < 67.5f)  return "NE";
    if (deg < 112.5f) return "E";
    if (deg < 157.5f) return "SE";
    if (deg < 202.5f) return "S";
    if (deg < 247.5f) return "SW";
    if (deg < 292.5f) return "W";
    return "NW";
}

static void draw_compass(compass_ctx_t *ctx, float heading_deg)
{
    lv_layer_t layer;
    lv_canvas_init_layer(ctx->canvas, &layer);

    /* Clear background */
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = UI_COLOR_CARD_BG;
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 0;
    lv_area_t full = {0, 0, CANVAS_W - 1, CANVAS_H - 1};
    lv_draw_rect(&layer, &rect_dsc, &full);

    /* Outer circle */
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = UI_COLOR_TEXT_DIM;
    arc_dsc.width = 2;
    arc_dsc.center.x = CENTER_X;
    arc_dsc.center.y = CENTER_Y;
    arc_dsc.radius = COMPASS_R;
    arc_dsc.start_angle = 0;
    arc_dsc.end_angle = 360;
    lv_draw_arc(&layer, &arc_dsc);

    /* Inner circle */
    arc_dsc.radius = COMPASS_R - 20;
    arc_dsc.color = lv_color_hex(0x1A3050);
    lv_draw_arc(&layer, &arc_dsc);

    /* Needle: from center outward along heading */
    float rad = (heading_deg - 90.0f) * (float)M_PI / 180.0f;
    int nx = CENTER_X + (int)(cosf(rad) * (COMPASS_R - 30));
    int ny = CENTER_Y + (int)(sinf(rad) * (COMPASS_R - 30));

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = UI_COLOR_ERROR;
    line_dsc.width = 3;
    line_dsc.p1.x = CENTER_X;
    line_dsc.p1.y = CENTER_Y;
    line_dsc.p2.x = nx;
    line_dsc.p2.y = ny;
    lv_draw_line(&layer, &line_dsc);

    /* Tail (opposite direction) */
    int tx = CENTER_X - (int)(cosf(rad) * 40);
    int ty = CENTER_Y - (int)(sinf(rad) * 40);
    line_dsc.color = UI_COLOR_TEXT_DIM;
    line_dsc.width = 2;
    line_dsc.p1.x = CENTER_X;
    line_dsc.p1.y = CENTER_Y;
    line_dsc.p2.x = tx;
    line_dsc.p2.y = ty;
    lv_draw_line(&layer, &line_dsc);

    /* Cardinal labels */
    lv_draw_label_dsc_t lbl_dsc;
    lv_draw_label_dsc_init(&lbl_dsc);
    lbl_dsc.color = UI_COLOR_TEXT;
    lbl_dsc.font = &lv_font_montserrat_16;

    struct { const char *txt; int x, y; } cards[] = {
        {"N", CENTER_X - 5, CENTER_Y - COMPASS_R - 4},
        {"S", CENTER_X - 4, CENTER_Y + COMPASS_R - 16},
        {"E", CENTER_X + COMPASS_R - 10, CENTER_Y - 8},
        {"W", CENTER_X - COMPASS_R - 2, CENTER_Y - 8},
    };
    for (int i = 0; i < 4; i++) {
        lv_area_t a = {cards[i].x, cards[i].y,
                       cards[i].x + 20, cards[i].y + 20};
        lv_draw_label(&layer, &lbl_dsc, &a);
    }

    lv_canvas_finish_layer(ctx->canvas, &layer);
}

static void timer_cb(lv_timer_t *t)
{
    compass_ctx_t *ctx = (compass_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    float heading = snap.bmm350.heading_x10 / 10.0f;

    draw_compass(ctx, heading);
    lv_label_set_text_fmt(ctx->lbl_deg, "%.1f", (double)heading);
    lv_label_set_text(ctx->lbl_dir, heading_to_dir(heading));
}

void example_main(lv_obj_t *parent)
{
    static compass_ctx_t ctx;
    /* Canvas buffer — CF_RGB565 = 2 bytes/pixel */
    static uint8_t cbuf[CANVAS_W * CANVAS_H * 2];
    ctx.cbuf = cbuf;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 10, 0);
    lv_obj_set_style_pad_row(parent, 8, 0);

    example_label_create(parent, "Compass Rose",
                         &lv_font_montserrat_24,
                         UI_COLOR_BMM350);

    /* Canvas */
    ctx.canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(ctx.canvas, cbuf, CANVAS_W, CANVAS_H,
                         LV_COLOR_FORMAT_RGB565);
    lv_obj_set_size(ctx.canvas, CANVAS_W, CANVAS_H);

    /* Heading readout row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 300, 60);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_column(row, 16, 0);

    ctx.lbl_deg = example_label_create(row, "--",
                                       &lv_font_montserrat_28,
                                       UI_COLOR_TEXT);
    example_label_create(row, "deg", &lv_font_montserrat_16,
                         UI_COLOR_TEXT_DIM);
    ctx.lbl_dir = example_label_create(row, "--",
                                       &lv_font_montserrat_28,
                                       UI_COLOR_BMM350);

    draw_compass(&ctx, 0.0f);
    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
