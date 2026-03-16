/**
 * @file    main_example.c
 * @brief   Tilt-Based Joystick — BMI270 Accelerometer Visualizer
 *
 * @description
 *   Board tilt visualized as dual-stick crosshair display using BMI270
 *   accelerometer data from ipc_sensorhub_snapshot(). Left circle shows
 *   X/Y tilt, right circle shows filtered tilt with deadzone. Button
 *   indicators show tilt direction thresholds.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"

#if BSP_HAS_BMI270

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define STICK_R         80      /* Circle radius */
#define DOT_R           10      /* Moving dot radius */
#define POLL_MS         33      /* ~30 FPS */
#define TILT_MAX        9.8f    /* Max expected tilt (1g) */
#define DEADZONE        1.5f    /* Deadzone in m/s^2 */
#define NUM_DIRS        4       /* UP, DOWN, LEFT, RIGHT */

#define COLOR_BG        lv_color_hex(0x142240)
#define COLOR_STICK_BG  lv_color_hex(0x0A1628)
#define COLOR_DOT_RAW   lv_color_hex(0x2196F3)
#define COLOR_DOT_FILT  lv_color_hex(0x4CAF50)
#define COLOR_TEXT      lv_color_hex(0xE0E0E0)
#define COLOR_ACTIVE    lv_color_hex(0x4CAF50)
#define COLOR_INACTIVE  lv_color_hex(0x1A3050)
#define COLOR_ACCENT    lv_color_hex(0x00BCD4)
#define COLOR_WARNING   lv_color_hex(0xFF9800)

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    /* Raw tilt circle */
    lv_obj_t    *circle_raw;
    lv_obj_t    *dot_raw;
    lv_obj_t    *lbl_raw;
    /* Filtered tilt circle */
    lv_obj_t    *circle_filt;
    lv_obj_t    *dot_filt;
    lv_obj_t    *lbl_filt;
    /* Direction indicators */
    lv_obj_t    *dir_indicators[NUM_DIRS];
    /* Info */
    lv_obj_t    *accel_label;
    lv_obj_t    *magnitude_bar;
    lv_obj_t    *mag_label;
    lv_timer_t  *poll_timer;
} tilt_ctx_t;

static tilt_ctx_t s_ctx;

static const char *dir_names[] = { "UP", "DOWN", "LEFT", "RIGHT" };

/* ---------------------------------------------------------------------------
 * Helper: clamp float
 * --------------------------------------------------------------------------- */
static float clampf(float val, float lo, float hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

/* ---------------------------------------------------------------------------
 * Create a stick circle with crosshairs and dot
 * --------------------------------------------------------------------------- */
static lv_obj_t *create_circle(lv_obj_t *parent, lv_obj_t **dot_out,
                                 lv_color_t dot_color)
{
    lv_obj_t *circle = lv_obj_create(parent);
    lv_obj_set_size(circle, STICK_R * 2, STICK_R * 2);
    lv_obj_set_style_bg_color(circle, COLOR_STICK_BG, 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(circle, 2, 0);
    lv_obj_set_style_border_color(circle, dot_color, 0);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    /* Crosshairs */
    lv_obj_t *hl = lv_obj_create(circle);
    lv_obj_set_size(hl, STICK_R * 2 - 20, 1);
    lv_obj_set_style_bg_color(hl, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(hl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hl, 0, 0);
    lv_obj_center(hl);

    lv_obj_t *vl = lv_obj_create(circle);
    lv_obj_set_size(vl, 1, STICK_R * 2 - 20);
    lv_obj_set_style_bg_color(vl, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(vl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vl, 0, 0);
    lv_obj_center(vl);

    /* Moving dot */
    *dot_out = lv_obj_create(circle);
    lv_obj_set_size(*dot_out, DOT_R * 2, DOT_R * 2);
    lv_obj_set_style_bg_color(*dot_out, dot_color, 0);
    lv_obj_set_style_bg_opa(*dot_out, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(*dot_out, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(*dot_out, 0, 0);
    lv_obj_set_style_shadow_width(*dot_out, 8, 0);
    lv_obj_set_style_shadow_color(*dot_out, dot_color, 0);
    lv_obj_center(*dot_out);

    return circle;
}

/* ---------------------------------------------------------------------------
 * Poll timer callback
 * --------------------------------------------------------------------------- */
static void poll_timer_cb(lv_timer_t *timer)
{
    tilt_ctx_t *ctx = (tilt_ctx_t *)lv_timer_get_user_data(timer);

    sensorhub_snapshot_t snap;
    if (ipc_sensorhub_snapshot(&snap) != 0) return;

    float ax = snap.bmi270.ax;
    float ay = snap.bmi270.ay;

    /* Show raw values */
    char buf[64];
    snprintf(buf, sizeof(buf), "Accel: X=%+.2f Y=%+.2f Z=%+.2f",
             ax, ay, snap.bmi270.az);
    lv_label_set_text(ctx->accel_label, buf);

    /* Map raw accel to dot position */
    int range = STICK_R - DOT_R - 4;
    float scale = (float)range / TILT_MAX;

    int raw_dx = (int)(clampf(ax, -TILT_MAX, TILT_MAX) * scale);
    int raw_dy = (int)(clampf(ay, -TILT_MAX, TILT_MAX) * scale);
    lv_obj_align(ctx->dot_raw, LV_ALIGN_CENTER, raw_dx, raw_dy);

    snprintf(buf, sizeof(buf), "Raw: %+.1f, %+.1f", ax, ay);
    lv_label_set_text(ctx->lbl_raw, buf);

    /* Filtered: apply deadzone */
    float fx = (ax > DEADZONE || ax < -DEADZONE) ? ax : 0.0f;
    float fy = (ay > DEADZONE || ay < -DEADZONE) ? ay : 0.0f;

    int filt_dx = (int)(clampf(fx, -TILT_MAX, TILT_MAX) * scale);
    int filt_dy = (int)(clampf(fy, -TILT_MAX, TILT_MAX) * scale);
    lv_obj_align(ctx->dot_filt, LV_ALIGN_CENTER, filt_dx, filt_dy);

    snprintf(buf, sizeof(buf), "Filt: %+.1f, %+.1f", fx, fy);
    lv_label_set_text(ctx->lbl_filt, buf);

    /* Direction indicators: UP, DOWN, LEFT, RIGHT */
    float threshold = 3.0f;
    bool dirs[] = {
        fy < -threshold,    /* UP */
        fy > threshold,     /* DOWN */
        fx < -threshold,    /* LEFT */
        fx > threshold,     /* RIGHT */
    };

    for (int i = 0; i < NUM_DIRS; i++) {
        lv_obj_set_style_bg_color(ctx->dir_indicators[i],
                                   dirs[i] ? COLOR_ACTIVE : COLOR_INACTIVE, 0);
    }

    /* Magnitude bar */
    float mag = ax * ax + ay * ay;
    /* Approximate sqrt using fast integer method for display purposes */
    int mag_pct = (int)(mag * 100.0f / (TILT_MAX * TILT_MAX));
    if (mag_pct > 100) mag_pct = 100;
    lv_bar_set_value(ctx->magnitude_bar, mag_pct, LV_ANIM_ON);

    snprintf(buf, sizeof(buf), "Tilt: %d%%", mag_pct);
    lv_label_set_text(ctx->mag_label, buf);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_SHUFFLE " Tilt Joystick — BMI270");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    /* Accelerometer readout */
    s_ctx.accel_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.accel_label, "Accel: X=+0.00 Y=+0.00 Z=+0.00");
    lv_obj_set_style_text_color(s_ctx.accel_label, COLOR_ACCENT, 0);
    lv_obj_align(s_ctx.accel_label, LV_ALIGN_TOP_MID, 0, 24);

    /* Raw tilt circle (left) */
    s_ctx.circle_raw = create_circle(parent, &s_ctx.dot_raw, COLOR_DOT_RAW);
    lv_obj_align(s_ctx.circle_raw, LV_ALIGN_LEFT_MID, 20, -10);

    lv_obj_t *raw_title = lv_label_create(parent);
    lv_label_set_text(raw_title, "Raw Tilt");
    lv_obj_set_style_text_color(raw_title, COLOR_DOT_RAW, 0);
    lv_obj_align_to(raw_title, s_ctx.circle_raw, LV_ALIGN_OUT_TOP_MID, 0, -2);

    s_ctx.lbl_raw = lv_label_create(parent);
    lv_label_set_text(s_ctx.lbl_raw, "Raw: +0.0, +0.0");
    lv_obj_set_style_text_color(s_ctx.lbl_raw, COLOR_DOT_RAW, 0);
    lv_obj_align_to(s_ctx.lbl_raw, s_ctx.circle_raw, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* Filtered tilt circle (right) */
    s_ctx.circle_filt = create_circle(parent, &s_ctx.dot_filt, COLOR_DOT_FILT);
    lv_obj_align(s_ctx.circle_filt, LV_ALIGN_RIGHT_MID, -20, -10);

    lv_obj_t *filt_title = lv_label_create(parent);
    lv_label_set_text(filt_title, "Filtered (deadzone)");
    lv_obj_set_style_text_color(filt_title, COLOR_DOT_FILT, 0);
    lv_obj_align_to(filt_title, s_ctx.circle_filt, LV_ALIGN_OUT_TOP_MID, 0, -2);

    s_ctx.lbl_filt = lv_label_create(parent);
    lv_label_set_text(s_ctx.lbl_filt, "Filt: +0.0, +0.0");
    lv_obj_set_style_text_color(s_ctx.lbl_filt, COLOR_DOT_FILT, 0);
    lv_obj_align_to(s_ctx.lbl_filt, s_ctx.circle_filt, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* Direction indicators (center) */
    lv_obj_t *dir_panel = lv_obj_create(parent);
    lv_obj_set_size(dir_panel, 200, 100);
    lv_obj_align(dir_panel, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(dir_panel, COLOR_BG, 0);
    lv_obj_set_style_border_color(dir_panel, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(dir_panel, 1, 0);
    lv_obj_set_style_radius(dir_panel, 8, 0);
    lv_obj_set_style_pad_all(dir_panel, 8, 0);
    lv_obj_set_flex_flow(dir_panel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(dir_panel, 6, 0);
    lv_obj_set_style_pad_column(dir_panel, 6, 0);
    lv_obj_set_flex_align(dir_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < NUM_DIRS; i++) {
        lv_obj_t *ind = lv_obj_create(dir_panel);
        lv_obj_set_size(ind, 80, 30);
        lv_obj_set_style_bg_color(ind, COLOR_INACTIVE, 0);
        lv_obj_set_style_radius(ind, 6, 0);
        lv_obj_set_style_border_width(ind, 0, 0);
        lv_obj_clear_flag(ind, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(ind);
        lv_label_set_text(lbl, dir_names[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);

        s_ctx.dir_indicators[i] = ind;
    }

    /* Magnitude bar (bottom center) */
    s_ctx.mag_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.mag_label, "Tilt: 0%");
    lv_obj_set_style_text_color(s_ctx.mag_label, COLOR_WARNING, 0);
    lv_obj_align(s_ctx.mag_label, LV_ALIGN_BOTTOM_MID, -60, -8);

    s_ctx.magnitude_bar = lv_bar_create(parent);
    lv_obj_set_size(s_ctx.magnitude_bar, 200, 12);
    lv_bar_set_range(s_ctx.magnitude_bar, 0, 100);
    lv_bar_set_value(s_ctx.magnitude_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_ctx.magnitude_bar, COLOR_INACTIVE, 0);
    lv_obj_set_style_bg_color(s_ctx.magnitude_bar, COLOR_WARNING, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_ctx.magnitude_bar, 4, 0);
    lv_obj_set_style_radius(s_ctx.magnitude_bar, 4, LV_PART_INDICATOR);
    lv_obj_align(s_ctx.magnitude_bar, LV_ALIGN_BOTTOM_MID, 40, -10);

    /* Start poll timer */
    s_ctx.poll_timer = lv_timer_create(poll_timer_cb, POLL_MS, &s_ctx);
}

#else /* !BSP_HAS_BMI270 */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "BMI270 accelerometer not available on this board");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF44336), 0);
    lv_obj_center(lbl);
}

#endif /* BSP_HAS_BMI270 */
