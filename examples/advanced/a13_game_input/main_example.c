/**
 * @file    main_example.c
 * @brief   Accelerometer Game Input — BMI270 Tilt Control
 *
 * @description
 *   Uses BMI270 accelerometer data from ipc_sensorhub_snapshot() as game
 *   input. A character dot moves on screen based on board tilt. Includes
 *   touch D-pad overlay as fallback input. Tilt sensitivity adjustable.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"

#if BSP_HAS_BMI270

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define POLL_MS             33      /* ~30 FPS */
#define ARENA_W             400
#define ARENA_H             300
#define CHAR_SIZE           20
#define TILT_SCALE          4       /* pixels per (m/s^2) */
#define SPEED_MAX           8       /* max pixels per frame */
#define DPAD_BTN_SIZE       36

#define COLOR_BG            lv_color_hex(0x0D1B2A)
#define COLOR_ARENA         lv_color_hex(0x0A1628)
#define COLOR_CHAR          lv_color_hex(0x4CAF50)
#define COLOR_TRAIL         lv_color_hex(0x1B5E20)
#define COLOR_TEXT          lv_color_hex(0xE0E0E0)
#define COLOR_ACCENT        lv_color_hex(0x2196F3)
#define COLOR_DPAD          lv_color_hex(0x1A3050)
#define COLOR_ACTIVE        lv_color_hex(0x4CAF50)

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *arena;
    lv_obj_t    *character;
    lv_obj_t    *accel_label;
    lv_obj_t    *pos_label;
    lv_obj_t    *dpad_up, *dpad_down, *dpad_left, *dpad_right;
    lv_timer_t  *poll_timer;
    int          char_x;    /* character position in arena coords */
    int          char_y;
    bool         touch_up, touch_down, touch_left, touch_right;
} game_ctx_t;

static game_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Clamp helper
 * --------------------------------------------------------------------------- */
static int clamp_i(int val, int lo, int hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

/* ---------------------------------------------------------------------------
 * D-pad touch callbacks
 * --------------------------------------------------------------------------- */
static void dpad_press_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    bool pressed = (lv_event_get_code(e) == LV_EVENT_PRESSED);

    if (btn == s_ctx.dpad_up)    s_ctx.touch_up = pressed;
    if (btn == s_ctx.dpad_down)  s_ctx.touch_down = pressed;
    if (btn == s_ctx.dpad_left)  s_ctx.touch_left = pressed;
    if (btn == s_ctx.dpad_right) s_ctx.touch_right = pressed;
}

/* ---------------------------------------------------------------------------
 * Poll timer: read accelerometer + merge touch input
 * --------------------------------------------------------------------------- */
static void poll_timer_cb(lv_timer_t *timer)
{
    game_ctx_t *ctx = (game_ctx_t *)lv_timer_get_user_data(timer);
    int dx = 0, dy = 0;

    /* Read accelerometer from sensor snapshot */
    ipc_sensorhub_snapshot_t snap;
    if (ipc_sensorhub_snapshot(&snap) == 0) {
        /* snap.accel_x/y are in m/s^2, typical range -10..+10 */
        /* X-axis tilt moves character horizontally, Y-axis vertically */
        dx = clamp_i((int)(snap.accel_x * TILT_SCALE), -SPEED_MAX, SPEED_MAX);
        dy = clamp_i((int)(snap.accel_y * TILT_SCALE), -SPEED_MAX, SPEED_MAX);

        /* Show accelerometer values */
        char buf[64];
        snprintf(buf, sizeof(buf), "Accel: X=%.1f Y=%.1f Z=%.1f",
                 snap.accel_x, snap.accel_y, snap.accel_z);
        lv_label_set_text(ctx->accel_label, buf);
    }

    /* Merge touch D-pad input */
    if (ctx->touch_up)    dy -= 4;
    if (ctx->touch_down)  dy += 4;
    if (ctx->touch_left)  dx -= 4;
    if (ctx->touch_right) dx += 4;

    /* Update character position */
    ctx->char_x = clamp_i(ctx->char_x + dx, 0, ARENA_W - CHAR_SIZE);
    ctx->char_y = clamp_i(ctx->char_y + dy, 0, ARENA_H - CHAR_SIZE);

    lv_obj_set_pos(ctx->character, ctx->char_x, ctx->char_y);

    /* Update position label */
    char pos_buf[32];
    snprintf(pos_buf, sizeof(pos_buf), "Pos: (%d, %d)", ctx->char_x, ctx->char_y);
    lv_label_set_text(ctx->pos_label, pos_buf);
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
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Accelerometer Game Input");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Accelerometer readout */
    s_ctx.accel_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.accel_label, "Accel: X=0.0 Y=0.0 Z=0.0");
    lv_obj_set_style_text_color(s_ctx.accel_label, COLOR_ACCENT, 0);
    lv_obj_align(s_ctx.accel_label, LV_ALIGN_TOP_LEFT, 16, 28);

    /* Position label */
    s_ctx.pos_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.pos_label, "Pos: (200, 150)");
    lv_obj_set_style_text_color(s_ctx.pos_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.pos_label, LV_ALIGN_TOP_RIGHT, -16, 28);

    /* Game arena */
    s_ctx.arena = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.arena, ARENA_W, ARENA_H);
    lv_obj_align(s_ctx.arena, LV_ALIGN_LEFT_MID, 8, 16);
    lv_obj_set_style_bg_color(s_ctx.arena, COLOR_ARENA, 0);
    lv_obj_set_style_border_color(s_ctx.arena, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(s_ctx.arena, 2, 0);
    lv_obj_set_style_radius(s_ctx.arena, 8, 0);
    lv_obj_clear_flag(s_ctx.arena, LV_OBJ_FLAG_SCROLLABLE);

    /* Crosshair lines in arena */
    lv_obj_t *hl = lv_obj_create(s_ctx.arena);
    lv_obj_set_size(hl, ARENA_W - 20, 1);
    lv_obj_set_style_bg_color(hl, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(hl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hl, 0, 0);
    lv_obj_center(hl);

    lv_obj_t *vl = lv_obj_create(s_ctx.arena);
    lv_obj_set_size(vl, 1, ARENA_H - 20);
    lv_obj_set_style_bg_color(vl, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(vl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vl, 0, 0);
    lv_obj_center(vl);

    /* Character dot */
    s_ctx.character = lv_obj_create(s_ctx.arena);
    lv_obj_set_size(s_ctx.character, CHAR_SIZE, CHAR_SIZE);
    lv_obj_set_style_bg_color(s_ctx.character, COLOR_CHAR, 0);
    lv_obj_set_style_bg_opa(s_ctx.character, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_ctx.character, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_ctx.character, 0, 0);
    lv_obj_set_style_shadow_width(s_ctx.character, 10, 0);
    lv_obj_set_style_shadow_color(s_ctx.character, COLOR_CHAR, 0);

    /* Center character initially */
    s_ctx.char_x = (ARENA_W - CHAR_SIZE) / 2;
    s_ctx.char_y = (ARENA_H - CHAR_SIZE) / 2;
    lv_obj_set_pos(s_ctx.character, s_ctx.char_x, s_ctx.char_y);

    /* Touch D-pad (right side) */
    lv_obj_t *dpad_area = lv_obj_create(parent);
    lv_obj_set_size(dpad_area, 140, 140);
    lv_obj_align(dpad_area, LV_ALIGN_RIGHT_MID, -16, 16);
    lv_obj_set_style_bg_color(dpad_area, COLOR_BG, 0);
    lv_obj_set_style_border_color(dpad_area, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(dpad_area, 1, 0);
    lv_obj_set_style_radius(dpad_area, 8, 0);

    s_ctx.dpad_up    = create_dpad_btn(dpad_area, LV_SYMBOL_UP,    LV_ALIGN_TOP_MID, 0, 4);
    s_ctx.dpad_down  = create_dpad_btn(dpad_area, LV_SYMBOL_DOWN,  LV_ALIGN_BOTTOM_MID, 0, -4);
    s_ctx.dpad_left  = create_dpad_btn(dpad_area, LV_SYMBOL_LEFT,  LV_ALIGN_LEFT_MID, 4, 0);
    s_ctx.dpad_right = create_dpad_btn(dpad_area, LV_SYMBOL_RIGHT, LV_ALIGN_RIGHT_MID, -4, 0);

    /* Info */
    lv_obj_t *info = lv_label_create(parent);
    lv_label_set_text(info, "Tilt board or use D-pad");
    lv_obj_set_style_text_color(info, lv_color_hex(0x616161), 0);
    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -4);

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
