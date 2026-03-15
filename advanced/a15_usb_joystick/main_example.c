/**
 * @file    main_example.c
 * @brief   USB Joystick HID Parser — F310 DirectInput
 *
 * @description
 *   emUSB-Host F310 (VID:046D PID:C216) DirectInput HID report parsing.
 *   Axes (X/Y/Z/Rz), 12 buttons, D-pad hat. IPC bridge CM55->CM33_NS.
 *   IPC_CMD_JOYSTICK_INIT (0xC1), IPC_CMD_JOYSTICK_STATE (0xC0).
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "usb_hid_joystick.h"
#include "USBH.h"
#include "ipc_communication.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define F310_VID                0x046D
#define F310_PID                0xC216
#define HID_REPORT_LEN          8
#define IPC_CMD_JOYSTICK_INIT   0xC1
#define IPC_CMD_JOYSTICK_STATE  0xC0
#define NUM_BUTTONS             12
#define POLL_MS                 16

#define COLOR_BG        lv_color_hex(0x142240)
#define COLOR_TEXT       lv_color_hex(0xE0E0E0)
#define COLOR_ACTIVE     lv_color_hex(0x4CAF50)
#define COLOR_INACTIVE   lv_color_hex(0x333333)
#define COLOR_AXIS       lv_color_hex(0x2196F3)
#define COLOR_ACCENT     lv_color_hex(0xFF9800)

/* HID Report byte offsets for F310 DirectInput */
#define OFS_LEFT_X      0
#define OFS_LEFT_Y      1
#define OFS_RIGHT_X     2
#define OFS_RIGHT_Y     3
#define OFS_DPAD        4
#define OFS_BUTTONS_LO  5
#define OFS_BUTTONS_HI  6

/* ---------------------------------------------------------------------------
 * Joystick parsed state
 * --------------------------------------------------------------------------- */
typedef struct {
    uint8_t  left_x;        /* 0-255, 128=center */
    uint8_t  left_y;
    uint8_t  right_x;
    uint8_t  right_y;
    uint8_t  dpad;          /* 0-7 direction, 0x0F=center */
    uint16_t buttons;       /* 12 button bits */
} joystick_state_t;

typedef struct {
    lv_obj_t        *parent;
    lv_obj_t        *axis_bars[4];
    lv_obj_t        *axis_labels[4];
    lv_obj_t        *btn_indicators[NUM_BUTTONS];
    lv_obj_t        *dpad_label;
    lv_obj_t        *status_label;
    lv_obj_t        *device_label;
    lv_timer_t      *poll_timer;
    joystick_state_t state;
    bool             connected;
} joystick_ctx_t;

static joystick_ctx_t s_ctx;

static const char *dpad_directions[] = {
    "N", "NE", "E", "SE", "S", "SW", "W", "NW",
    "", "", "", "", "", "", "", "--"
};

static const char *button_names[] = {
    "X", "A", "B", "Y", "LB", "RB", "LT", "RT",
    "Back", "Start", "L3", "R3"
};

/* ---------------------------------------------------------------------------
 * Parse HID report
 * --------------------------------------------------------------------------- */
static void parse_hid_report(const uint8_t *report, joystick_state_t *state)
{
    state->left_x   = report[OFS_LEFT_X];
    state->left_y   = report[OFS_LEFT_Y];
    state->right_x  = report[OFS_RIGHT_X];
    state->right_y  = report[OFS_RIGHT_Y];
    state->dpad     = report[OFS_DPAD] & 0x0F;
    state->buttons  = (uint16_t)report[OFS_BUTTONS_LO] |
                      ((uint16_t)(report[OFS_BUTTONS_HI] & 0x0F) << 8);
}

/* ---------------------------------------------------------------------------
 * Pack state for IPC transmission (8 bytes)
 * --------------------------------------------------------------------------- */
static void pack_ipc_state(const joystick_state_t *state, uint8_t *out)
{
    out[0] = state->left_x;
    out[1] = state->left_y;
    out[2] = state->right_x;
    out[3] = state->right_y;
    out[4] = state->dpad;
    out[5] = (uint8_t)(state->buttons & 0xFF);
    out[6] = (uint8_t)((state->buttons >> 8) & 0x0F);
    out[7] = 0;    /* reserved */
}

/* ---------------------------------------------------------------------------
 * Poll timer: read joystick via IPC and update UI
 * --------------------------------------------------------------------------- */
static void poll_timer_cb(lv_timer_t *timer)
{
    joystick_ctx_t *ctx = (joystick_ctx_t *)lv_timer_get_user_data(timer);

    /* Request joystick state from CM55 via IPC */
    uint8_t report[HID_REPORT_LEN];
    cy_rslt_t res = ipc_send_cmd_with_response(IPC_CMD_JOYSTICK_STATE,
                                                 NULL, 0,
                                                 report, sizeof(report));
    if (res != CY_RSLT_SUCCESS) {
        if (ctx->connected) {
            lv_label_set_text(ctx->status_label, "Disconnected");
            lv_obj_set_style_text_color(ctx->status_label, lv_color_hex(0xF44336), 0);
            ctx->connected = false;
        }
        return;
    }

    if (!ctx->connected) {
        ctx->connected = true;
        lv_label_set_text(ctx->status_label, "Connected");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_ACTIVE, 0);
        char dev_str[64];
        snprintf(dev_str, sizeof(dev_str), "Logitech F310 (VID:%04X PID:%04X)",
                 F310_VID, F310_PID);
        lv_label_set_text(ctx->device_label, dev_str);
    }

    parse_hid_report(report, &ctx->state);

    /* Update axis bars */
    uint8_t axis_vals[] = {
        ctx->state.left_x, ctx->state.left_y,
        ctx->state.right_x, ctx->state.right_y
    };
    static const char *axis_names[] = {"LX", "LY", "RX", "RY"};

    for (int i = 0; i < 4; i++) {
        lv_bar_set_value(ctx->axis_bars[i], axis_vals[i], LV_ANIM_OFF);
        char val_str[16];
        snprintf(val_str, sizeof(val_str), "%s: %u", axis_names[i], axis_vals[i]);
        lv_label_set_text(ctx->axis_labels[i], val_str);
    }

    /* Update button indicators */
    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool pressed = (ctx->state.buttons >> i) & 1;
        lv_obj_set_style_bg_color(ctx->btn_indicators[i],
                                   pressed ? COLOR_ACTIVE : COLOR_INACTIVE, 0);
    }

    /* Update D-pad */
    lv_label_set_text(ctx->dpad_label, dpad_directions[ctx->state.dpad & 0x0F]);
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
    lv_label_set_text(title, LV_SYMBOL_USB " USB Joystick — F310");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Status + device info */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Waiting for device...");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_ACCENT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 32);

    s_ctx.device_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.device_label, "---");
    lv_obj_set_style_text_color(s_ctx.device_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.device_label, LV_ALIGN_TOP_LEFT, 16, 50);

    /* --- Axes panel (left) --- */
    lv_obj_t *axes_panel = lv_obj_create(parent);
    lv_obj_set_size(axes_panel, 240, 200);
    lv_obj_align(axes_panel, LV_ALIGN_LEFT_MID, 8, 20);
    lv_obj_set_style_bg_color(axes_panel, COLOR_BG, 0);
    lv_obj_set_style_border_color(axes_panel, COLOR_AXIS, 0);
    lv_obj_set_style_border_width(axes_panel, 1, 0);
    lv_obj_set_style_radius(axes_panel, 8, 0);
    lv_obj_set_style_pad_all(axes_panel, 10, 0);
    lv_obj_set_flex_flow(axes_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(axes_panel, 8, 0);

    lv_obj_t *axes_title = lv_label_create(axes_panel);
    lv_label_set_text(axes_title, "Analog Axes");
    lv_obj_set_style_text_color(axes_title, COLOR_AXIS, 0);

    static const char *axis_names_init[] = {"LX: 128", "LY: 128", "RX: 128", "RY: 128"};
    for (int i = 0; i < 4; i++) {
        s_ctx.axis_labels[i] = lv_label_create(axes_panel);
        lv_label_set_text(s_ctx.axis_labels[i], axis_names_init[i]);
        lv_obj_set_style_text_color(s_ctx.axis_labels[i], COLOR_TEXT, 0);

        s_ctx.axis_bars[i] = lv_bar_create(axes_panel);
        lv_obj_set_size(s_ctx.axis_bars[i], 200, 12);
        lv_bar_set_range(s_ctx.axis_bars[i], 0, 255);
        lv_bar_set_value(s_ctx.axis_bars[i], 128, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_ctx.axis_bars[i], COLOR_INACTIVE, 0);
        lv_obj_set_style_bg_color(s_ctx.axis_bars[i], COLOR_AXIS, LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_ctx.axis_bars[i], 4, 0);
        lv_obj_set_style_radius(s_ctx.axis_bars[i], 4, LV_PART_INDICATOR);
    }

    /* --- Buttons panel (right) --- */
    lv_obj_t *btns_panel = lv_obj_create(parent);
    lv_obj_set_size(btns_panel, 260, 200);
    lv_obj_align(btns_panel, LV_ALIGN_RIGHT_MID, -8, 20);
    lv_obj_set_style_bg_color(btns_panel, COLOR_BG, 0);
    lv_obj_set_style_border_color(btns_panel, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(btns_panel, 1, 0);
    lv_obj_set_style_radius(btns_panel, 8, 0);
    lv_obj_set_style_pad_all(btns_panel, 10, 0);
    lv_obj_set_flex_flow(btns_panel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(btns_panel, 6, 0);
    lv_obj_set_style_pad_column(btns_panel, 6, 0);

    lv_obj_t *btns_title = lv_label_create(btns_panel);
    lv_label_set_text(btns_title, "Buttons");
    lv_obj_set_style_text_color(btns_title, COLOR_ACCENT, 0);
    lv_obj_set_width(btns_title, lv_pct(100));

    for (int i = 0; i < NUM_BUTTONS; i++) {
        lv_obj_t *ind = lv_obj_create(btns_panel);
        lv_obj_set_size(ind, 52, 28);
        lv_obj_set_style_bg_color(ind, COLOR_INACTIVE, 0);
        lv_obj_set_style_radius(ind, 6, 0);
        lv_obj_set_style_border_width(ind, 0, 0);

        lv_obj_t *lbl = lv_label_create(ind);
        lv_label_set_text(lbl, button_names[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);

        s_ctx.btn_indicators[i] = ind;
    }

    /* D-pad display */
    lv_obj_t *dpad_container = lv_obj_create(btns_panel);
    lv_obj_set_size(dpad_container, 60, 60);
    lv_obj_set_style_bg_color(dpad_container, COLOR_BG, 0);
    lv_obj_set_style_border_color(dpad_container, COLOR_TEXT, 0);
    lv_obj_set_style_border_width(dpad_container, 1, 0);
    lv_obj_set_style_radius(dpad_container, LV_RADIUS_CIRCLE, 0);

    s_ctx.dpad_label = lv_label_create(dpad_container);
    lv_label_set_text(s_ctx.dpad_label, "--");
    lv_obj_set_style_text_font(s_ctx.dpad_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_ctx.dpad_label, COLOR_ACCENT, 0);
    lv_obj_center(s_ctx.dpad_label);

    /* Init IPC joystick */
    ipc_send_cmd(IPC_CMD_JOYSTICK_INIT, NULL, 0);

    /* Poll timer */
    s_ctx.poll_timer = lv_timer_create(poll_timer_cb, POLL_MS, &s_ctx);
}
