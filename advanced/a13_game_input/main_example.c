/**
 * @file    main_example.c
 * @brief   Unified Game Input — F310 Joystick + Touch D-Pad
 *
 * @description
 *   Input abstraction: F310 HID joystick via IPC bridge + touch D-pad overlay.
 *   Both merged into game_input_state_t with booleans + analog axes.
 *   IPC_CMD_JOYSTICK_STATE (0xC0) for joystick, LVGL callbacks for touch.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "game_common.h"
#include "usb_hid_joystick.h"
#include "ipc_communication.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define IPC_CMD_JOYSTICK_STATE  0xC0
#define IPC_CMD_JOYSTICK_INIT   0xC1
#define ANALOG_CENTER           128
#define ANALOG_DEADZONE         30
#define DPAD_SIZE               120
#define DPAD_BTN_SIZE           36
#define POLL_MS                 33

#define COLOR_BG                lv_color_hex(0x0D1B2A)
#define COLOR_ACTIVE            lv_color_hex(0x4CAF50)
#define COLOR_INACTIVE          lv_color_hex(0x333333)
#define COLOR_TEXT              lv_color_hex(0xE0E0E0)
#define COLOR_DPAD              lv_color_hex(0x1A3050)

/* ---------------------------------------------------------------------------
 * Input State
 * --------------------------------------------------------------------------- */
typedef struct {
    /* Touch D-pad state */
    bool touch_up, touch_down, touch_left, touch_right;
    bool touch_a, touch_b;
} touch_state_t;

typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *dpad_up, *dpad_down, *dpad_left, *dpad_right;
    lv_obj_t    *btn_a, *btn_b;
    lv_obj_t    *state_labels[9];   /* up,down,left,right,a,b,start,ax,ay */
    lv_obj_t    *source_label;
    lv_obj_t    *analog_dot;
    lv_obj_t    *analog_area;
    touch_state_t touch;
    game_input_state_t merged;
    lv_timer_t  *poll_timer;
    bool         joy_connected;
} input_ctx_t;

static input_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Read joystick via IPC
 * --------------------------------------------------------------------------- */
static void read_joystick_ipc(game_input_state_t *state)
{
    uint8_t joy_data[8];
    cy_rslt_t res = ipc_send_cmd_with_response(IPC_CMD_JOYSTICK_STATE,
                                                 NULL, 0,
                                                 joy_data, sizeof(joy_data));
    if (res != CY_RSLT_SUCCESS) return;

    /* Parse HID report: byte 0-1 = axes, byte 2 = buttons, byte 3 = dpad */
    state->analog_x = joy_data[0];
    state->analog_y = joy_data[1];
    uint8_t buttons = joy_data[2];
    uint8_t dpad    = joy_data[3];

    /* Digital from analog (with deadzone) */
    if (state->analog_y < ANALOG_CENTER - ANALOG_DEADZONE) state->up = true;
    if (state->analog_y > ANALOG_CENTER + ANALOG_DEADZONE) state->down = true;
    if (state->analog_x < ANALOG_CENTER - ANALOG_DEADZONE) state->left = true;
    if (state->analog_x > ANALOG_CENTER + ANALOG_DEADZONE) state->right = true;

    /* D-pad hat */
    if (dpad == 0) state->up = true;
    if (dpad == 2) state->right = true;
    if (dpad == 4) state->down = true;
    if (dpad == 6) state->left = true;

    /* Buttons */
    state->a     = (buttons & 0x01) != 0;  /* X button */
    state->b     = (buttons & 0x02) != 0;  /* A button */
    state->start = (buttons & 0x08) != 0;  /* Start */
}

/* ---------------------------------------------------------------------------
 * Touch D-pad callbacks
 * --------------------------------------------------------------------------- */
static void dpad_press_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    bool pressed = (lv_event_get_code(e) == LV_EVENT_PRESSED);

    if (btn == s_ctx.dpad_up)    s_ctx.touch.touch_up = pressed;
    if (btn == s_ctx.dpad_down)  s_ctx.touch.touch_down = pressed;
    if (btn == s_ctx.dpad_left)  s_ctx.touch.touch_left = pressed;
    if (btn == s_ctx.dpad_right) s_ctx.touch.touch_right = pressed;
    if (btn == s_ctx.btn_a)      s_ctx.touch.touch_a = pressed;
    if (btn == s_ctx.btn_b)      s_ctx.touch.touch_b = pressed;
}

/* ---------------------------------------------------------------------------
 * Merge inputs and update display
 * --------------------------------------------------------------------------- */
static void poll_timer_cb(lv_timer_t *timer)
{
    input_ctx_t *ctx = (input_ctx_t *)lv_timer_get_user_data(timer);

    /* Clear merged state */
    memset(&ctx->merged, 0, sizeof(ctx->merged));
    ctx->merged.analog_x = ANALOG_CENTER;
    ctx->merged.analog_y = ANALOG_CENTER;

    /* Read joystick via IPC */
    read_joystick_ipc(&ctx->merged);

    /* Merge touch input */
    ctx->merged.up    |= ctx->touch.touch_up;
    ctx->merged.down  |= ctx->touch.touch_down;
    ctx->merged.left  |= ctx->touch.touch_left;
    ctx->merged.right |= ctx->touch.touch_right;
    ctx->merged.a     |= ctx->touch.touch_a;
    ctx->merged.b     |= ctx->touch.touch_b;

    /* Update state labels */
    static const char *names[] = {"UP","DOWN","LEFT","RIGHT","A","B","START","AX","AY"};
    bool states[] = {
        ctx->merged.up, ctx->merged.down, ctx->merged.left, ctx->merged.right,
        ctx->merged.a, ctx->merged.b, ctx->merged.start, false, false
    };

    for (int i = 0; i < 7; i++) {
        lv_obj_set_style_bg_color(ctx->state_labels[i],
                                   states[i] ? COLOR_ACTIVE : COLOR_INACTIVE, 0);
    }

    /* Analog values */
    char ax_str[16], ay_str[16];
    snprintf(ax_str, sizeof(ax_str), "X: %u", ctx->merged.analog_x);
    snprintf(ay_str, sizeof(ay_str), "Y: %u", ctx->merged.analog_y);
    lv_label_set_text(lv_obj_get_child(ctx->state_labels[7], 0), ax_str);
    lv_label_set_text(lv_obj_get_child(ctx->state_labels[8], 0), ay_str);

    /* Move analog dot */
    int dot_x = (int)(ctx->merged.analog_x - ANALOG_CENTER) * 40 / 128;
    int dot_y = (int)(ctx->merged.analog_y - ANALOG_CENTER) * 40 / 128;
    lv_obj_align_to(ctx->analog_dot, ctx->analog_area, LV_ALIGN_CENTER, dot_x, dot_y);
}

/* ---------------------------------------------------------------------------
 * Create a D-pad button
 * --------------------------------------------------------------------------- */
static lv_obj_t *create_dpad_btn(lv_obj_t *parent, const char *symbol,
                                  lv_align_t align, int x_ofs, int y_ofs)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, DPAD_BTN_SIZE, DPAD_BTN_SIZE);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_set_style_bg_color(btn, COLOR_DPAD, 0);
    lv_obj_set_style_bg_color(btn, COLOR_ACTIVE, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_add_event_cb(btn, dpad_press_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn, dpad_press_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, symbol);
    lv_obj_center(lbl);

    return btn;
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
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Game Input Test");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Source label */
    s_ctx.source_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.source_label, "Sources: Touch D-Pad + F310 Joystick (IPC)");
    lv_obj_set_style_text_color(s_ctx.source_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.source_label, LV_ALIGN_TOP_MID, 0, 28);

    /* --- Touch D-pad (left side) --- */
    lv_obj_t *dpad_area = lv_obj_create(parent);
    lv_obj_set_size(dpad_area, DPAD_SIZE + 20, DPAD_SIZE + 20);
    lv_obj_align(dpad_area, LV_ALIGN_LEFT_MID, 16, 20);
    lv_obj_set_style_bg_color(dpad_area, COLOR_BG, 0);
    lv_obj_set_style_border_color(dpad_area, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(dpad_area, 1, 0);
    lv_obj_set_style_radius(dpad_area, 8, 0);

    s_ctx.dpad_up    = create_dpad_btn(dpad_area, LV_SYMBOL_UP,    LV_ALIGN_TOP_MID, 0, 4);
    s_ctx.dpad_down  = create_dpad_btn(dpad_area, LV_SYMBOL_DOWN,  LV_ALIGN_BOTTOM_MID, 0, -4);
    s_ctx.dpad_left  = create_dpad_btn(dpad_area, LV_SYMBOL_LEFT,  LV_ALIGN_LEFT_MID, 4, 0);
    s_ctx.dpad_right = create_dpad_btn(dpad_area, LV_SYMBOL_RIGHT, LV_ALIGN_RIGHT_MID, -4, 0);

    /* Action buttons */
    s_ctx.btn_a = create_dpad_btn(parent, "A", LV_ALIGN_RIGHT_MID, -80, 0);
    lv_obj_set_style_bg_color(s_ctx.btn_a, lv_color_hex(0x2196F3), 0);
    lv_obj_set_size(s_ctx.btn_a, 44, 44);
    lv_obj_set_style_radius(s_ctx.btn_a, LV_RADIUS_CIRCLE, 0);

    s_ctx.btn_b = create_dpad_btn(parent, "B", LV_ALIGN_RIGHT_MID, -24, 0);
    lv_obj_set_style_bg_color(s_ctx.btn_b, lv_color_hex(0xF44336), 0);
    lv_obj_set_size(s_ctx.btn_b, 44, 44);
    lv_obj_set_style_radius(s_ctx.btn_b, LV_RADIUS_CIRCLE, 0);

    /* --- State display (center) --- */
    static const char *btn_names[] = {"UP","DOWN","LEFT","RIGHT","A","B","START","AX","AY"};
    lv_obj_t *state_panel = lv_obj_create(parent);
    lv_obj_set_size(state_panel, 220, 240);
    lv_obj_align(state_panel, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_color(state_panel, lv_color_hex(0x142240), 0);
    lv_obj_set_style_border_width(state_panel, 1, 0);
    lv_obj_set_style_border_color(state_panel, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(state_panel, 8, 0);
    lv_obj_set_style_pad_all(state_panel, 8, 0);
    lv_obj_set_flex_flow(state_panel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(state_panel, 4, 0);
    lv_obj_set_style_pad_column(state_panel, 4, 0);

    for (int i = 0; i < 9; i++) {
        lv_obj_t *chip = lv_obj_create(state_panel);
        lv_obj_set_size(chip, (i < 7) ? 60 : 90, 28);
        lv_obj_set_style_bg_color(chip, COLOR_INACTIVE, 0);
        lv_obj_set_style_radius(chip, 6, 0);
        lv_obj_set_style_border_width(chip, 0, 0);

        lv_obj_t *lbl = lv_label_create(chip);
        lv_label_set_text(lbl, btn_names[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);

        s_ctx.state_labels[i] = chip;
    }

    /* Analog stick visualizer */
    s_ctx.analog_area = lv_obj_create(state_panel);
    lv_obj_set_size(s_ctx.analog_area, 90, 90);
    lv_obj_set_style_bg_color(s_ctx.analog_area, COLOR_BG, 0);
    lv_obj_set_style_radius(s_ctx.analog_area, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(s_ctx.analog_area, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(s_ctx.analog_area, 1, 0);

    s_ctx.analog_dot = lv_obj_create(s_ctx.analog_area);
    lv_obj_set_size(s_ctx.analog_dot, 12, 12);
    lv_obj_set_style_bg_color(s_ctx.analog_dot, COLOR_ACTIVE, 0);
    lv_obj_set_style_radius(s_ctx.analog_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_ctx.analog_dot, 0, 0);
    lv_obj_center(s_ctx.analog_dot);

    /* Init joystick via IPC */
    ipc_send_cmd(IPC_CMD_JOYSTICK_INIT, NULL, 0);
    game_input_init();

    /* Start poll timer */
    s_ctx.poll_timer = lv_timer_create(poll_timer_cb, POLL_MS, &s_ctx);
}
