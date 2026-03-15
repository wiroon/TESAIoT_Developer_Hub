/**
 * @file    main_example.c
 * @brief   Joystick Visualizer — Dual sticks + shoulder/face buttons
 *
 * @description
 *   Full gamepad visualization: 2 circular analog stick areas with moving
 *   dots, 12 button indicators, D-pad compass rose. Polls via IPC bridge.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "usb_hid_joystick.h"
#include "ipc_communication.h"

/* ── Layout ─────────────────────────────────────────────────────── */
#define STICK_R          80      /* Stick circle radius             */
#define DOT_R            10      /* Moving dot radius               */
#define BTN_SIZE         32      /* Button indicator size            */
#define DPAD_SIZE        90      /* D-pad compass size              */
#define NUM_BUTTONS      12
#define IPC_CMD_JOYSTICK_STATE  0xC0
#define IPC_CMD_JOYSTICK_INIT   0xC1

/* ── Static state ───────────────────────────────────────────────── */
static lv_obj_t *s_dot_l, *s_dot_r;                  /* Stick dots        */
static lv_obj_t *s_stick_l, *s_stick_r;               /* Stick circles     */
static lv_obj_t *s_lbl_l, *s_lbl_r;                   /* Axis readouts     */
static lv_obj_t *s_btn[NUM_BUTTONS];                   /* Button indicators */
static lv_obj_t *s_dpad_dir[8];                        /* D-pad directions  */
static lv_obj_t *s_lbl_status;

static const char *s_btn_names[] = {
    "X", "A", "B", "Y", "LB", "RB", "LT", "RT",
    "BK", "ST", "L3", "R3"
};

static const char *s_dir_sym[] = {
    LV_SYMBOL_UP, "\xE2\x86\x97", LV_SYMBOL_RIGHT, "\xE2\x86\x98",
    LV_SYMBOL_DOWN, "\xE2\x86\x99", LV_SYMBOL_LEFT, "\xE2\x86\x96"
};

/* ── Create stick circle ────────────────────────────────────────── */
static lv_obj_t *create_stick_circle(lv_obj_t *parent, lv_obj_t **dot_out)
{
    lv_obj_t *circle = lv_obj_create(parent);
    lv_obj_set_size(circle, STICK_R * 2, STICK_R * 2);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(circle, 2, 0);
    lv_obj_set_style_border_color(circle, lv_color_hex(0x2A3A5C), 0);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    /* Crosshairs */
    lv_obj_t *hl = lv_obj_create(circle);
    lv_obj_set_size(hl, STICK_R * 2 - 20, 1);
    lv_obj_set_style_bg_color(hl, lv_color_hex(0x1a3050), 0);
    lv_obj_set_style_bg_opa(hl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hl, 0, 0);
    lv_obj_center(hl);

    lv_obj_t *vl = lv_obj_create(circle);
    lv_obj_set_size(vl, 1, STICK_R * 2 - 20);
    lv_obj_set_style_bg_color(vl, lv_color_hex(0x1a3050), 0);
    lv_obj_set_style_bg_opa(vl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vl, 0, 0);
    lv_obj_center(vl);

    /* Moving dot */
    *dot_out = lv_obj_create(circle);
    lv_obj_set_size(*dot_out, DOT_R * 2, DOT_R * 2);
    lv_obj_set_style_bg_color(*dot_out, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(*dot_out, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(*dot_out, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(*dot_out, 0, 0);
    lv_obj_set_style_shadow_width(*dot_out, 8, 0);
    lv_obj_set_style_shadow_color(*dot_out, UI_COLOR_PRIMARY, 0);
    lv_obj_center(*dot_out);

    return circle;
}

/* ── Timer callback ─────────────────────────────────────────────── */
static void poll_cb(lv_timer_t *t)
{
    (void)t;
    uint8_t rpt[8];
    cy_rslt_t res = ipc_send_cmd_with_response(IPC_CMD_JOYSTICK_STATE,
                                                NULL, 0, rpt, 8);
    if (res != CY_RSLT_SUCCESS) {
        lv_label_set_text(s_lbl_status, LV_SYMBOL_CLOSE " No Gamepad");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_ERROR, 0);
        return;
    }

    lv_label_set_text(s_lbl_status, LV_SYMBOL_OK " Gamepad Connected");
    lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_SUCCESS, 0);

    uint8_t lx = rpt[0], ly = rpt[1], rx = rpt[2], ry = rpt[3];
    uint8_t hat = rpt[4] & 0x0F;
    uint16_t btns = (uint16_t)rpt[5] | ((uint16_t)(rpt[6] & 0x0F) << 8);

    /* Map 0-255 to pixel offset from center */
    int range = STICK_R - DOT_R - 4;
    lv_obj_align(s_dot_l, LV_ALIGN_CENTER,
                 ((int)lx - 128) * range / 128,
                 ((int)ly - 128) * range / 128);
    lv_obj_align(s_dot_r, LV_ALIGN_CENTER,
                 ((int)rx - 128) * range / 128,
                 ((int)ry - 128) * range / 128);

    lv_label_set_text_fmt(s_lbl_l, "X:%3d Y:%3d", lx, ly);
    lv_label_set_text_fmt(s_lbl_r, "X:%3d Y:%3d", rx, ry);

    /* Buttons */
    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool on = (btns >> i) & 1;
        lv_obj_set_style_bg_color(s_btn[i],
            on ? UI_COLOR_SUCCESS : lv_color_hex(0x1a3050), 0);
    }

    /* D-pad */
    for (int i = 0; i < 8; i++) {
        bool active = (hat == i);
        lv_obj_set_style_text_color(s_dpad_dir[i],
            active ? UI_COLOR_WARNING : lv_color_hex(0x333333), 0);
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    example_label_create(parent, "A16 \xe2\x80\x94 Joystick Visualizer",
                         &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(lv_obj_get_child(parent, 0), LV_ALIGN_TOP_MID, 0, 2);

    /* Status */
    s_lbl_status = example_label_create(parent, LV_SYMBOL_CLOSE " No Gamepad",
                                        &lv_font_montserrat_14, UI_COLOR_ERROR);
    lv_obj_align(s_lbl_status, LV_ALIGN_TOP_MID, 0, 26);

    /* Left stick */
    s_stick_l = create_stick_circle(parent, &s_dot_l);
    lv_obj_align(s_stick_l, LV_ALIGN_LEFT_MID, 20, -30);
    s_lbl_l = example_label_create(parent, "X:128 Y:128",
                                   &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align_to(s_lbl_l, s_stick_l, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* Right stick */
    s_stick_r = create_stick_circle(parent, &s_dot_r);
    lv_obj_align(s_stick_r, LV_ALIGN_RIGHT_MID, -20, -30);
    s_lbl_r = example_label_create(parent, "X:128 Y:128",
                                   &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align_to(s_lbl_r, s_stick_r, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* Button grid (center) */
    lv_obj_t *btn_grid = example_card_create(parent, 200, 130, UI_COLOR_CARD_BG);
    lv_obj_align(btn_grid, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_flex_flow(btn_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(btn_grid, 4, 0);
    lv_obj_set_style_pad_column(btn_grid, 4, 0);

    for (int i = 0; i < NUM_BUTTONS; i++) {
        s_btn[i] = lv_obj_create(btn_grid);
        lv_obj_set_size(s_btn[i], BTN_SIZE + 8, BTN_SIZE - 6);
        lv_obj_set_style_bg_color(s_btn[i], lv_color_hex(0x1a3050), 0);
        lv_obj_set_style_bg_opa(s_btn[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(s_btn[i], 6, 0);
        lv_obj_set_style_border_width(s_btn[i], 0, 0);
        lv_obj_clear_flag(s_btn[i], LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(s_btn[i]);
        lv_label_set_text(lbl, s_btn_names[i]);
        lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(lbl);
    }

    /* D-pad compass rose (bottom center) */
    lv_obj_t *dpad = example_card_create(parent, DPAD_SIZE, DPAD_SIZE,
                                          lv_color_hex(0x0A1628));
    lv_obj_align(dpad, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_radius(dpad, LV_RADIUS_CIRCLE, 0);

    /* 8 directional labels around center */
    static const int dx[] = { 0, 25, 30, 25,  0, -25, -30, -25};
    static const int dy[] = {-30,-22,  0, 22, 30,  22,   0, -22};

    for (int i = 0; i < 8; i++) {
        s_dpad_dir[i] = lv_label_create(dpad);
        lv_label_set_text(s_dpad_dir[i], s_dir_sym[i]);
        lv_obj_set_style_text_color(s_dpad_dir[i], lv_color_hex(0x333333), 0);
        lv_obj_set_style_text_font(s_dpad_dir[i], &lv_font_montserrat_16, 0);
        lv_obj_align(s_dpad_dir[i], LV_ALIGN_CENTER, dx[i], dy[i]);
    }

    /* Init joystick IPC */
    ipc_send_cmd(IPC_CMD_JOYSTICK_INIT, NULL, 0);

    /* 20ms poll */
    lv_timer_create(poll_cb, 20, NULL);
}
