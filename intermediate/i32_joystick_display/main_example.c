/**
 * @file    main_example.c
 * @brief   Joystick Visualization — dual-stick crosshair + button indicators
 *
 * @description
 *   Dual-analog-stick gamepad visualization with:
 *     - Left/right stick crosshair areas (200x200) with position dot
 *     - Dead zone ring (40px from center, drawn as circle)
 *     - A/B/X/Y face buttons as colored circles
 *     - LB/RB shoulder button indicators
 *     - D-pad (hat switch) directional arrows
 *     - Raw axis values (LX, LY, RX, RY: 0-255)
 *
 *   In production, joystick data arrives via IPC from CM33_NS USB Host:
 *     IPC_CMD_JOYSTICK_STATE (0xC0) — packed gamepad state
 *   This example uses touch input to simulate stick movement for
 *   demonstration without requiring a connected gamepad.
 *
 * @board   AI Kit, Eva Kit, Game Console (all boards)
 * @author  TESAIoT
 */

#include "example_common.h"

/* ── Layout ────────────────────────────────────────────────────────── */
#define STICK_SIZE       180
#define STICK_HALF       (STICK_SIZE / 2)
#define DOT_RADIUS       8
#define DEADZONE_RADIUS  30
#define AXIS_CENTER      128     /* 0-255 range, center = 128 */

/* ── Colors ────────────────────────────────────────────────────────── */
#define COLOR_BG         lv_color_hex(0x0D1B2A)
#define COLOR_CARD       lv_color_hex(0x142240)
#define COLOR_TEXT       lv_color_hex(0xE0E0E0)
#define COLOR_CROSS      lv_color_hex(0x2A3A5C)
#define COLOR_DOT        lv_color_hex(0x00BCD4)
#define COLOR_DEADZONE   lv_color_hex(0x1A3050)
#define COLOR_BTN_A      lv_color_hex(0x4CAF50)  /* green  */
#define COLOR_BTN_B      lv_color_hex(0xF44336)  /* red    */
#define COLOR_BTN_X      lv_color_hex(0x2196F3)  /* blue   */
#define COLOR_BTN_Y      lv_color_hex(0xFF9800)  /* orange */
#define COLOR_BTN_OFF    lv_color_hex(0x1A3050)
#define COLOR_SHOULDER   lv_color_hex(0x7C4DFF)

/* ── IPC reference (for documentation) ─────────────────────────────── */
#define IPC_CMD_JOYSTICK_STATE  (0xC0)

/* ── Gamepad state ─────────────────────────────────────────────────── */
typedef struct {
    uint8_t lx, ly;         /* left stick  0-255 */
    uint8_t rx, ry;         /* right stick 0-255 */
    bool    btn_a, btn_b, btn_x, btn_y;
    bool    btn_lb, btn_rb;
    bool    btn_start, btn_back;
    int8_t  hat;            /* -1=center, 0=N, 1=NE, 2=E, ... 7=NW */
} gamepad_state_t;

static gamepad_state_t s_state;

/* ── UI handles ────────────────────────────────────────────────────── */
static lv_obj_t *s_left_canvas;
static lv_obj_t *s_right_canvas;
static lv_obj_t *s_left_dot;
static lv_obj_t *s_right_dot;
static lv_obj_t *s_axis_label;
static lv_obj_t *s_btn_indicators[4];   /* A, B, X, Y */
static lv_obj_t *s_btn_labels[4];
static lv_obj_t *s_lb_indicator;
static lv_obj_t *s_rb_indicator;
static lv_obj_t *s_dpad_arrows[4];      /* N, E, S, W */

/* ── Map 0-255 axis to pixel offset from center ───────────────────── */
static int16_t axis_to_px(uint8_t val)
{
    return (int16_t)((int32_t)(val - AXIS_CENTER) * STICK_HALF / 128);
}

/* ── Create stick area with crosshair ──────────────────────────────── */
static lv_obj_t *create_stick_area(lv_obj_t *parent, const char *label_text,
                                    lv_obj_t **dot_out)
{
    lv_obj_t *card = example_card_create(parent, STICK_SIZE + 16, STICK_SIZE + 40, COLOR_CARD);

    /* Label */
    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, -4);

    /* Stick field */
    lv_obj_t *field = lv_obj_create(card);
    lv_obj_set_size(field, STICK_SIZE, STICK_SIZE);
    lv_obj_align(field, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(field, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(field, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(field, STICK_HALF, 0);
    lv_obj_set_style_border_color(field, COLOR_CROSS, 0);
    lv_obj_set_style_border_width(field, 2, 0);
    lv_obj_clear_flag(field, LV_OBJ_FLAG_SCROLLABLE);

    /* Crosshair lines (horizontal) */
    lv_obj_t *h_line = lv_obj_create(field);
    lv_obj_set_size(h_line, STICK_SIZE - 20, 1);
    lv_obj_set_style_bg_color(h_line, COLOR_CROSS, 0);
    lv_obj_set_style_bg_opa(h_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(h_line, 0, 0);
    lv_obj_set_style_radius(h_line, 0, 0);
    lv_obj_center(h_line);

    /* Crosshair lines (vertical) */
    lv_obj_t *v_line = lv_obj_create(field);
    lv_obj_set_size(v_line, 1, STICK_SIZE - 20);
    lv_obj_set_style_bg_color(v_line, COLOR_CROSS, 0);
    lv_obj_set_style_bg_opa(v_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(v_line, 0, 0);
    lv_obj_set_style_radius(v_line, 0, 0);
    lv_obj_center(v_line);

    /* Dead zone ring */
    lv_obj_t *dz = lv_obj_create(field);
    lv_obj_set_size(dz, DEADZONE_RADIUS * 2, DEADZONE_RADIUS * 2);
    lv_obj_set_style_bg_opa(dz, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(dz, COLOR_DEADZONE, 0);
    lv_obj_set_style_border_width(dz, 1, 0);
    lv_obj_set_style_radius(dz, LV_RADIUS_CIRCLE, 0);
    lv_obj_center(dz);

    /* Position dot */
    lv_obj_t *dot = lv_obj_create(field);
    lv_obj_set_size(dot, DOT_RADIUS * 2, DOT_RADIUS * 2);
    lv_obj_set_style_bg_color(dot, COLOR_DOT, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_shadow_width(dot, 6, 0);
    lv_obj_set_style_shadow_color(dot, COLOR_DOT, 0);
    lv_obj_center(dot);

    *dot_out = dot;
    return field;
}

/* ── Create face button indicator ──────────────────────────────────── */
static lv_obj_t *create_btn_circle(lv_obj_t *parent, const char *text,
                                    lv_color_t color, int x, int y,
                                    lv_obj_t **label_out)
{
    lv_obj_t *circle = lv_obj_create(parent);
    lv_obj_set_size(circle, 36, 36);
    lv_obj_set_style_bg_color(circle, COLOR_BTN_OFF, 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(circle, color, 0);
    lv_obj_set_style_border_width(circle, 2, 0);
    lv_obj_align(circle, LV_ALIGN_CENTER, x, y);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(circle);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_center(lbl);

    *label_out = lbl;
    return circle;
}

/* ── Touch event on stick areas (simulation) ───────────────────────── */
static void stick_touch_cb(lv_event_t *e)
{
    lv_obj_t *field = lv_event_get_target(e);
    lv_point_t point;
    lv_indev_get_point(lv_indev_active(), &point);

    /* Convert screen coords to field-local coords */
    int32_t fx = lv_obj_get_x(field);
    int32_t fy = lv_obj_get_y(field);
    /* Account for parent offset */
    lv_obj_t *par = lv_obj_get_parent(field);
    if (par) {
        fx += lv_obj_get_x(par);
        fy += lv_obj_get_y(par);
        lv_obj_t *gpar = lv_obj_get_parent(par);
        if (gpar) {
            fx += lv_obj_get_x(gpar);
            fy += lv_obj_get_y(gpar);
        }
    }

    int32_t local_x = point.x - fx - STICK_HALF;
    int32_t local_y = point.y - fy - STICK_HALF;

    /* Clamp to stick range */
    if (local_x < -STICK_HALF) local_x = -STICK_HALF;
    if (local_x >  STICK_HALF) local_x =  STICK_HALF;
    if (local_y < -STICK_HALF) local_y = -STICK_HALF;
    if (local_y >  STICK_HALF) local_y =  STICK_HALF;

    /* Convert to 0-255 */
    uint8_t ax = (uint8_t)(AXIS_CENTER + local_x * 128 / STICK_HALF);
    uint8_t ay = (uint8_t)(AXIS_CENTER + local_y * 128 / STICK_HALF);

    /* Determine which stick (left field = s_left_canvas) */
    if (field == s_left_canvas) {
        s_state.lx = ax;
        s_state.ly = ay;
    } else {
        s_state.rx = ax;
        s_state.ry = ay;
    }
}

static void stick_release_cb(lv_event_t *e)
{
    lv_obj_t *field = lv_event_get_target(e);
    if (field == s_left_canvas) {
        s_state.lx = AXIS_CENTER;
        s_state.ly = AXIS_CENTER;
    } else {
        s_state.rx = AXIS_CENTER;
        s_state.ry = AXIS_CENTER;
    }
}

/* ── Update timer ──────────────────────────────────────────────────── */
static void update_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    /* Update stick dots */
    int16_t lx_px = axis_to_px(s_state.lx);
    int16_t ly_px = axis_to_px(s_state.ly);
    lv_obj_align(s_left_dot, LV_ALIGN_CENTER, lx_px, ly_px);

    int16_t rx_px = axis_to_px(s_state.rx);
    int16_t ry_px = axis_to_px(s_state.ry);
    lv_obj_align(s_right_dot, LV_ALIGN_CENTER, rx_px, ry_px);

    /* Update axis values */
    lv_label_set_text_fmt(s_axis_label,
        "LX:%3u  LY:%3u  |  RX:%3u  RY:%3u",
        s_state.lx, s_state.ly, s_state.rx, s_state.ry);

    /* Update face buttons (toggle with simulated timer) */
    const lv_color_t btn_colors[] = {
        COLOR_BTN_A, COLOR_BTN_B, COLOR_BTN_X, COLOR_BTN_Y
    };
    const bool *btn_states[] = {
        &s_state.btn_a, &s_state.btn_b, &s_state.btn_x, &s_state.btn_y
    };
    for (int i = 0; i < 4; i++) {
        if (*btn_states[i]) {
            lv_obj_set_style_bg_color(s_btn_indicators[i], btn_colors[i], 0);
        } else {
            lv_obj_set_style_bg_color(s_btn_indicators[i], COLOR_BTN_OFF, 0);
        }
    }

    /* Shoulder buttons */
    lv_obj_set_style_bg_color(s_lb_indicator,
        s_state.btn_lb ? COLOR_SHOULDER : COLOR_BTN_OFF, 0);
    lv_obj_set_style_bg_color(s_rb_indicator,
        s_state.btn_rb ? COLOR_SHOULDER : COLOR_BTN_OFF, 0);

    /* D-pad (hat) — highlight active direction */
    bool hat_n = (s_state.hat == 0 || s_state.hat == 1 || s_state.hat == 7);
    bool hat_e = (s_state.hat == 1 || s_state.hat == 2 || s_state.hat == 3);
    bool hat_s = (s_state.hat == 3 || s_state.hat == 4 || s_state.hat == 5);
    bool hat_w = (s_state.hat == 5 || s_state.hat == 6 || s_state.hat == 7);

    lv_obj_set_style_text_color(s_dpad_arrows[0],
        hat_n ? COLOR_DOT : COLOR_CROSS, 0);
    lv_obj_set_style_text_color(s_dpad_arrows[1],
        hat_e ? COLOR_DOT : COLOR_CROSS, 0);
    lv_obj_set_style_text_color(s_dpad_arrows[2],
        hat_s ? COLOR_DOT : COLOR_CROSS, 0);
    lv_obj_set_style_text_color(s_dpad_arrows[3],
        hat_w ? COLOR_DOT : COLOR_CROSS, 0);
}

/* ── Demo: cycle button states for visual feedback ─────────────────── */
static uint32_t s_demo_counter;

static void demo_btn_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_demo_counter++;

    /* Toggle buttons in pattern */
    s_state.btn_a = (s_demo_counter % 8 < 2);
    s_state.btn_b = ((s_demo_counter + 2) % 8 < 2);
    s_state.btn_x = ((s_demo_counter + 4) % 8 < 2);
    s_state.btn_y = ((s_demo_counter + 6) % 8 < 2);
    s_state.btn_lb = (s_demo_counter % 6 < 2);
    s_state.btn_rb = ((s_demo_counter + 3) % 6 < 2);

    /* Cycle hat switch */
    s_state.hat = (int8_t)((s_demo_counter / 3) % 9) - 1;
}

/* ── Entry point ───────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Reset state */
    memset(&s_state, 0, sizeof(s_state));
    s_state.lx = AXIS_CENTER;
    s_state.ly = AXIS_CENTER;
    s_state.rx = AXIS_CENTER;
    s_state.ry = AXIS_CENTER;
    s_state.hat = -1;
    s_demo_counter = 0;

    lv_obj_set_style_bg_color(parent, COLOR_BG, 0);

    /* ── Title ─────────────────────────────────────────────────────── */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Joystick Visualization");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* แสดงผลจอยสติ๊ก */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "แสดงผลจอยสติ๊ก");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* ── Left stick ────────────────────────────────────────────────── */
    s_left_canvas = create_stick_area(parent, "Left Stick", &s_left_dot);
    lv_obj_t *left_parent = lv_obj_get_parent(s_left_canvas);
    lv_obj_align(left_parent, LV_ALIGN_TOP_LEFT, 12, 32);
    lv_obj_add_flag(s_left_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_left_canvas, stick_touch_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_left_canvas, stick_release_cb, LV_EVENT_RELEASED, NULL);

    /* ── Right stick ───────────────────────────────────────────────── */
    s_right_canvas = create_stick_area(parent, "Right Stick", &s_right_dot);
    lv_obj_t *right_parent = lv_obj_get_parent(s_right_canvas);
    lv_obj_align(right_parent, LV_ALIGN_TOP_RIGHT, -12, 32);
    lv_obj_add_flag(s_right_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_right_canvas, stick_touch_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_right_canvas, stick_release_cb, LV_EVENT_RELEASED, NULL);

    /* ── Axis values ───────────────────────────────────────────────── */
    s_axis_label = lv_label_create(parent);
    lv_label_set_text_fmt(s_axis_label, "LX:%3u  LY:%3u  |  RX:%3u  RY:%3u",
                          AXIS_CENTER, AXIS_CENTER, AXIS_CENTER, AXIS_CENTER);
    lv_obj_set_style_text_font(s_axis_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_axis_label, COLOR_TEXT, 0);
    lv_obj_align(s_axis_label, LV_ALIGN_TOP_MID, 0, 276);

    /* ── Face buttons (A/B/X/Y) ────────────────────────────────────── */
    lv_obj_t *btn_card = example_card_create(parent, 200, 120, COLOR_CARD);
    lv_obj_align(btn_card, LV_ALIGN_TOP_RIGHT, -20, 300);

    /*         X
     *       Y   A
     *         B        */
    s_btn_indicators[0] = create_btn_circle(btn_card, "A", COLOR_BTN_A, 40, 0, &s_btn_labels[0]);
    s_btn_indicators[1] = create_btn_circle(btn_card, "B", COLOR_BTN_B, 0, 30, &s_btn_labels[1]);
    s_btn_indicators[2] = create_btn_circle(btn_card, "X", COLOR_BTN_X, 0, -30, &s_btn_labels[2]);
    s_btn_indicators[3] = create_btn_circle(btn_card, "Y", COLOR_BTN_Y, -40, 0, &s_btn_labels[3]);

    /* ── D-pad ─────────────────────────────────────────────────────── */
    lv_obj_t *dpad_card = example_card_create(parent, 130, 120, COLOR_CARD);
    lv_obj_align(dpad_card, LV_ALIGN_TOP_LEFT, 20, 300);

    lv_obj_t *dpad_title = lv_label_create(dpad_card);
    lv_label_set_text(dpad_title, "D-Pad");
    lv_obj_set_style_text_font(dpad_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(dpad_title, COLOR_TEXT, 0);
    lv_obj_align(dpad_title, LV_ALIGN_TOP_MID, 0, -6);

    /* N, E, S, W arrows */
    s_dpad_arrows[0] = lv_label_create(dpad_card);
    lv_label_set_text(s_dpad_arrows[0], LV_SYMBOL_UP);
    lv_obj_set_style_text_font(s_dpad_arrows[0], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_dpad_arrows[0], COLOR_CROSS, 0);
    lv_obj_align(s_dpad_arrows[0], LV_ALIGN_CENTER, 0, -22);

    s_dpad_arrows[1] = lv_label_create(dpad_card);
    lv_label_set_text(s_dpad_arrows[1], LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(s_dpad_arrows[1], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_dpad_arrows[1], COLOR_CROSS, 0);
    lv_obj_align(s_dpad_arrows[1], LV_ALIGN_CENTER, 22, 0);

    s_dpad_arrows[2] = lv_label_create(dpad_card);
    lv_label_set_text(s_dpad_arrows[2], LV_SYMBOL_DOWN);
    lv_obj_set_style_text_font(s_dpad_arrows[2], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_dpad_arrows[2], COLOR_CROSS, 0);
    lv_obj_align(s_dpad_arrows[2], LV_ALIGN_CENTER, 0, 22);

    s_dpad_arrows[3] = lv_label_create(dpad_card);
    lv_label_set_text(s_dpad_arrows[3], LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(s_dpad_arrows[3], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_dpad_arrows[3], COLOR_CROSS, 0);
    lv_obj_align(s_dpad_arrows[3], LV_ALIGN_CENTER, -22, 0);

    /* ── Shoulder buttons ──────────────────────────────────────────── */
    s_lb_indicator = lv_obj_create(parent);
    lv_obj_set_size(s_lb_indicator, 70, 28);
    lv_obj_align(s_lb_indicator, LV_ALIGN_TOP_LEFT, 60, 430);
    lv_obj_set_style_bg_color(s_lb_indicator, COLOR_BTN_OFF, 0);
    lv_obj_set_style_bg_opa(s_lb_indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_lb_indicator, 6, 0);
    lv_obj_set_style_border_color(s_lb_indicator, COLOR_SHOULDER, 0);
    lv_obj_set_style_border_width(s_lb_indicator, 1, 0);
    lv_obj_clear_flag(s_lb_indicator, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *lb_lbl = lv_label_create(s_lb_indicator);
    lv_label_set_text(lb_lbl, "LB");
    lv_obj_set_style_text_color(lb_lbl, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(lb_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lb_lbl);

    s_rb_indicator = lv_obj_create(parent);
    lv_obj_set_size(s_rb_indicator, 70, 28);
    lv_obj_align(s_rb_indicator, LV_ALIGN_TOP_RIGHT, -60, 430);
    lv_obj_set_style_bg_color(s_rb_indicator, COLOR_BTN_OFF, 0);
    lv_obj_set_style_bg_opa(s_rb_indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_rb_indicator, 6, 0);
    lv_obj_set_style_border_color(s_rb_indicator, COLOR_SHOULDER, 0);
    lv_obj_set_style_border_width(s_rb_indicator, 1, 0);
    lv_obj_clear_flag(s_rb_indicator, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *rb_lbl = lv_label_create(s_rb_indicator);
    lv_label_set_text(rb_lbl, "RB");
    lv_obj_set_style_text_color(rb_lbl, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(rb_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(rb_lbl);

    /* ── IPC reference note ────────────────────────────────────────── */
    lv_obj_t *note = lv_label_create(parent);
    lv_label_set_text(note,
        "Touch sticks to simulate | Buttons cycle in demo\n"
        "Production: IPC_CMD_JOYSTICK_STATE (0xC0) from CM33_NS USB Host");
    lv_obj_set_style_text_font(note, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(note, lv_color_hex(0x808080), 0);
    lv_obj_set_style_text_align(note, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(note, 440);
    lv_obj_align(note, LV_ALIGN_TOP_MID, 0, 468);

    /* ── Timers ────────────────────────────────────────────────────── */
    lv_timer_create(update_timer_cb, 20, NULL);      /* 50 fps UI update */
    lv_timer_create(demo_btn_timer_cb, 300, NULL);    /* button demo cycle */
}
