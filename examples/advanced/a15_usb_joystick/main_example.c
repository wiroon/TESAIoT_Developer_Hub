/**
 * @file    main_example.c
 * @brief   Virtual Joystick Visualizer — Touch Input
 *
 * @description
 *   Virtual joystick using LVGL touch events. Two circular touch areas
 *   act as analog sticks. Button indicators respond to on-screen button
 *   presses. No USB HID or IPC calls — pure LVGL touch interaction.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define STICK_R         70      /* Stick area radius */
#define DOT_R           10      /* Thumb dot radius */
#define NUM_BUTTONS     6
#define UPDATE_MS       33      /* ~30 FPS */

#define COLOR_BG        lv_color_hex(0x142240)
#define COLOR_STICK_BG  lv_color_hex(0x0A1628)
#define COLOR_DOT       lv_color_hex(0x4CAF50)
#define COLOR_DOT_TOUCH lv_color_hex(0x66BB6A)
#define COLOR_TEXT      lv_color_hex(0xE0E0E0)
#define COLOR_ACTIVE    lv_color_hex(0x4CAF50)
#define COLOR_INACTIVE  lv_color_hex(0x1A3050)
#define COLOR_ACCENT    lv_color_hex(0x2196F3)
#define COLOR_BTN_A     lv_color_hex(0x2196F3)
#define COLOR_BTN_B     lv_color_hex(0xF44336)

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    /* Left stick */
    lv_obj_t    *stick_l;
    lv_obj_t    *dot_l;
    lv_obj_t    *lbl_l;
    int          lx, ly;        /* -128..+127 */
    /* Right stick */
    lv_obj_t    *stick_r;
    lv_obj_t    *dot_r;
    lv_obj_t    *lbl_r;
    int          rx, ry;
    /* Buttons */
    lv_obj_t    *btn_objs[NUM_BUTTONS];
    bool         btn_state[NUM_BUTTONS];
    /* Status */
    lv_obj_t    *status_label;
    lv_timer_t  *timer;
} vjoy_ctx_t;

static vjoy_ctx_t s_ctx;

static const char *btn_names[] = { "A", "B", "X", "Y", "LB", "RB" };
static const lv_color_t btn_colors_active[] = {
    {.blue = 0xF3, .green = 0x96, .red = 0x21},  /* A - blue */
    {.blue = 0x36, .green = 0x43, .red = 0xF4},  /* B - red */
    {.blue = 0x50, .green = 0xAF, .red = 0x4C},  /* X - green */
    {.blue = 0x00, .green = 0x98, .red = 0xFF},  /* Y - orange */
    {.blue = 0xFF, .green = 0x4D, .red = 0x7C},  /* LB - purple */
    {.blue = 0xD4, .green = 0xBC, .red = 0x00},  /* RB - cyan */
};

/* ---------------------------------------------------------------------------
 * Stick touch handler
 * --------------------------------------------------------------------------- */
static void stick_event_cb(lv_event_t *e)
{
    lv_obj_t *stick = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    bool is_left = (stick == s_ctx.stick_l);

    lv_obj_t *dot = is_left ? s_ctx.dot_l : s_ctx.dot_r;
    int *out_x = is_left ? &s_ctx.lx : &s_ctx.rx;
    int *out_y = is_left ? &s_ctx.ly : &s_ctx.ry;

    if (code == LV_EVENT_PRESSING) {
        lv_point_t point;
        lv_indev_get_point(lv_indev_active(), &point);

        /* Convert screen coords to stick-relative coords */
        int cx = lv_obj_get_x(stick) + STICK_R;
        int cy = lv_obj_get_y(stick) + STICK_R;

        /* Get parent's screen position for offset */
        int px = lv_obj_get_x(s_ctx.parent);
        int py = lv_obj_get_y(s_ctx.parent);
        cx += px;
        cy += py;

        int dx = (int)point.x - cx;
        int dy = (int)point.y - cy;

        /* Clamp to stick range */
        int range = STICK_R - DOT_R - 4;
        if (dx < -range) dx = -range;
        if (dx > range) dx = range;
        if (dy < -range) dy = -range;
        if (dy > range) dy = range;

        /* Map to -128..+127 */
        *out_x = dx * 128 / range;
        *out_y = dy * 128 / range;

        /* Move dot */
        lv_obj_align(dot, LV_ALIGN_CENTER, dx, dy);
    } else if (code == LV_EVENT_RELEASED) {
        /* Spring back to center */
        *out_x = 0;
        *out_y = 0;
        lv_obj_align(dot, LV_ALIGN_CENTER, 0, 0);
    }
}

/* ---------------------------------------------------------------------------
 * Button press/release handler
 * --------------------------------------------------------------------------- */
static void vbtn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (btn == s_ctx.btn_objs[i]) {
            if (code == LV_EVENT_PRESSED) {
                s_ctx.btn_state[i] = true;
                lv_obj_set_style_bg_color(btn, btn_colors_active[i], 0);
            } else if (code == LV_EVENT_RELEASED) {
                s_ctx.btn_state[i] = false;
                lv_obj_set_style_bg_color(btn, COLOR_INACTIVE, 0);
            }
            break;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Update timer: refresh axis labels
 * --------------------------------------------------------------------------- */
static void update_timer_cb(lv_timer_t *timer)
{
    vjoy_ctx_t *ctx = (vjoy_ctx_t *)lv_timer_get_user_data(timer);
    char buf[32];

    snprintf(buf, sizeof(buf), "X:%+4d Y:%+4d", ctx->lx, ctx->ly);
    lv_label_set_text(ctx->lbl_l, buf);

    snprintf(buf, sizeof(buf), "X:%+4d Y:%+4d", ctx->rx, ctx->ry);
    lv_label_set_text(ctx->lbl_r, buf);

    /* Count active buttons for status */
    int active = 0;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (ctx->btn_state[i]) active++;
    }
    snprintf(buf, sizeof(buf), "Buttons: %d/%d active", active, NUM_BUTTONS);
    lv_label_set_text(ctx->status_label, buf);
}

/* ---------------------------------------------------------------------------
 * Create a stick circle with touch support
 * --------------------------------------------------------------------------- */
static lv_obj_t *create_stick(lv_obj_t *parent, lv_obj_t **dot_out)
{
    lv_obj_t *circle = lv_obj_create(parent);
    lv_obj_set_size(circle, STICK_R * 2, STICK_R * 2);
    lv_obj_set_style_bg_color(circle, COLOR_STICK_BG, 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(circle, 2, 0);
    lv_obj_set_style_border_color(circle, COLOR_ACCENT, 0);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    /* Crosshairs */
    lv_obj_t *hl = lv_obj_create(circle);
    lv_obj_set_size(hl, STICK_R * 2 - 20, 1);
    lv_obj_set_style_bg_color(hl, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(hl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hl, 0, 0);
    lv_obj_center(hl);
    lv_obj_add_flag(hl, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *vl = lv_obj_create(circle);
    lv_obj_set_size(vl, 1, STICK_R * 2 - 20);
    lv_obj_set_style_bg_color(vl, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(vl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vl, 0, 0);
    lv_obj_center(vl);
    lv_obj_add_flag(vl, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* Thumb dot */
    *dot_out = lv_obj_create(circle);
    lv_obj_set_size(*dot_out, DOT_R * 2, DOT_R * 2);
    lv_obj_set_style_bg_color(*dot_out, COLOR_DOT, 0);
    lv_obj_set_style_bg_opa(*dot_out, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(*dot_out, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(*dot_out, 0, 0);
    lv_obj_set_style_shadow_width(*dot_out, 8, 0);
    lv_obj_set_style_shadow_color(*dot_out, COLOR_DOT, 0);
    lv_obj_center(*dot_out);
    lv_obj_add_flag(*dot_out, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* Touch events on circle */
    lv_obj_add_event_cb(circle, stick_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(circle, stick_event_cb, LV_EVENT_RELEASED, NULL);

    return circle;
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
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Virtual Joystick");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Status */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Touch sticks and buttons");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_MID, 0, 28);

    /* Left stick */
    s_ctx.stick_l = create_stick(parent, &s_ctx.dot_l);
    lv_obj_align(s_ctx.stick_l, LV_ALIGN_LEFT_MID, 20, -20);

    s_ctx.lbl_l = lv_label_create(parent);
    lv_label_set_text(s_ctx.lbl_l, "X:  +0 Y:  +0");
    lv_obj_set_style_text_color(s_ctx.lbl_l, COLOR_ACCENT, 0);
    lv_obj_align_to(s_ctx.lbl_l, s_ctx.stick_l, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    lv_obj_t *lbl_left = lv_label_create(parent);
    lv_label_set_text(lbl_left, "Left Stick");
    lv_obj_set_style_text_color(lbl_left, COLOR_TEXT, 0);
    lv_obj_align_to(lbl_left, s_ctx.stick_l, LV_ALIGN_OUT_TOP_MID, 0, -4);

    /* Right stick */
    s_ctx.stick_r = create_stick(parent, &s_ctx.dot_r);
    lv_obj_align(s_ctx.stick_r, LV_ALIGN_RIGHT_MID, -20, -20);

    s_ctx.lbl_r = lv_label_create(parent);
    lv_label_set_text(s_ctx.lbl_r, "X:  +0 Y:  +0");
    lv_obj_set_style_text_color(s_ctx.lbl_r, COLOR_ACCENT, 0);
    lv_obj_align_to(s_ctx.lbl_r, s_ctx.stick_r, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    lv_obj_t *lbl_right = lv_label_create(parent);
    lv_label_set_text(lbl_right, "Right Stick");
    lv_obj_set_style_text_color(lbl_right, COLOR_TEXT, 0);
    lv_obj_align_to(lbl_right, s_ctx.stick_r, LV_ALIGN_OUT_TOP_MID, 0, -4);

    /* Button grid (center) */
    lv_obj_t *btn_panel = lv_obj_create(parent);
    lv_obj_set_size(btn_panel, 200, 140);
    lv_obj_align(btn_panel, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(btn_panel, COLOR_BG, 0);
    lv_obj_set_style_border_color(btn_panel, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(btn_panel, 1, 0);
    lv_obj_set_style_radius(btn_panel, 8, 0);
    lv_obj_set_style_pad_all(btn_panel, 8, 0);
    lv_obj_set_flex_flow(btn_panel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(btn_panel, 6, 0);
    lv_obj_set_style_pad_column(btn_panel, 6, 0);
    lv_obj_set_flex_align(btn_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < NUM_BUTTONS; i++) {
        lv_obj_t *btn = lv_btn_create(btn_panel);
        lv_obj_set_size(btn, 54, 36);
        lv_obj_set_style_bg_color(btn, COLOR_INACTIVE, 0);
        lv_obj_set_style_radius(btn, 8, 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_names[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, vbtn_event_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(btn, vbtn_event_cb, LV_EVENT_RELEASED, NULL);

        s_ctx.btn_objs[i] = btn;
    }

    /* Update timer */
    s_ctx.timer = lv_timer_create(update_timer_cb, UPDATE_MS, &s_ctx);
}
