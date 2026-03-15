/**
 * A11 — Pong Game
 *
 * Classic Pong with CapSense/Potentiometer paddle control on Eva Kit,
 * or touch button / tilt control on AI Kit.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define COURT_W         700
#define COURT_H         380
#define COURT_X         40
#define COURT_Y         50
#define PADDLE_W        12
#define PADDLE_H        70
#define BALL_SIZE       14
#define TICK_MS         20
#define BALL_SPEED      3.5f
#define AI_SPEED        2.5f
#define MAX_SCORE       11

typedef struct {
    lv_obj_t   *parent;
    lv_obj_t   *court;
    lv_obj_t   *paddle_l;
    lv_obj_t   *paddle_r;
    lv_obj_t   *ball;
    lv_obj_t   *center_line;
    lv_obj_t   *score_l_label;
    lv_obj_t   *score_r_label;
    lv_obj_t   *status_label;
    lv_obj_t   *input_label;
    lv_obj_t   *up_btn;
    lv_obj_t   *down_btn;
    float       paddle_l_y;
    float       paddle_r_y;
    float       ball_x, ball_y;
    float       ball_vx, ball_vy;
    int         score_l, score_r;
    bool        playing;
    bool        holding_up;
    bool        holding_down;
    uint32_t    rally;
} app_ctx_t;

static app_ctx_t g_ctx;

static void serve_ball(app_ctx_t *ctx, int side)
{
    ctx->ball_x = (float)(COURT_W / 2);
    ctx->ball_y = (float)(COURT_H / 2);
    ctx->ball_vx = (side > 0) ? BALL_SPEED : -BALL_SPEED;
    ctx->ball_vy = (side > 0) ? 1.5f : -1.5f;
    ctx->rally = 0;
}

static void reset_game(app_ctx_t *ctx)
{
    ctx->paddle_l_y = (float)(COURT_H / 2 - PADDLE_H / 2);
    ctx->paddle_r_y = (float)(COURT_H / 2 - PADDLE_H / 2);
    ctx->score_l = 0;
    ctx->score_r = 0;
    ctx->playing = false;
    serve_ball(ctx, 1);
    lv_label_set_text(ctx->score_l_label, "0");
    lv_label_set_text(ctx->score_r_label, "0");
    lv_label_set_text(ctx->status_label, "Tap to start");
    lv_obj_clear_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);
}

static void btn_up_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) ctx->holding_up = true;
    if (code == LV_EVENT_RELEASED) ctx->holding_up = false;

    if (!ctx->playing) {
        ctx->playing = true;
        lv_obj_add_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void btn_down_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) ctx->holding_down = true;
    if (code == LV_EVENT_RELEASED) ctx->holding_down = false;

    if (!ctx->playing) {
        ctx->playing = true;
        lv_obj_add_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void game_tick(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    /* Player paddle input */
    float player_target = ctx->paddle_l_y;

#if BSP_HAS_POTENTIOMETER
    /* Potentiometer: direct position mapping */
    float pot_pct = (float)snap.pot_percent_x10 / 10.0f;
    player_target = pot_pct / 100.0f * (float)(COURT_H - PADDLE_H);
#elif BSP_HAS_CAPSENSE
    /* CapSense slider: position mapping */
    if (snap.capsense.slider > 0) {
        player_target = (float)snap.capsense.slider / 100.0f * (float)(COURT_H - PADDLE_H);
    }
#elif BSP_HAS_BMI270
    /* Tilt: use accelerometer Y axis */
    float tilt = (float)snap.bmi270.ay / 16384.0f;
    player_target = ctx->paddle_l_y + tilt * 6.0f;
#endif

    /* Button input */
    if (ctx->holding_up) player_target -= 4.0f;
    if (ctx->holding_down) player_target += 4.0f;

    /* Smooth paddle movement */
    float diff = player_target - ctx->paddle_l_y;
    if (diff > 5.0f) diff = 5.0f;
    if (diff < -5.0f) diff = -5.0f;
    ctx->paddle_l_y += diff;

    /* Clamp */
    if (ctx->paddle_l_y < 0) ctx->paddle_l_y = 0;
    if (ctx->paddle_l_y > (float)(COURT_H - PADDLE_H))
        ctx->paddle_l_y = (float)(COURT_H - PADDLE_H);

    if (!ctx->playing) {
        lv_obj_set_y(ctx->paddle_l, (lv_coord_t)ctx->paddle_l_y);
        return;
    }

    /* AI paddle: track ball with some lag */
    float ai_target = ctx->ball_y - (float)(PADDLE_H / 2);
    float ai_diff = ai_target - ctx->paddle_r_y;
    float speed_limit = AI_SPEED + (float)ctx->rally * 0.05f;
    if (speed_limit > 5.0f) speed_limit = 5.0f;
    if (ai_diff > speed_limit) ai_diff = speed_limit;
    if (ai_diff < -speed_limit) ai_diff = -speed_limit;
    ctx->paddle_r_y += ai_diff;
    if (ctx->paddle_r_y < 0) ctx->paddle_r_y = 0;
    if (ctx->paddle_r_y > (float)(COURT_H - PADDLE_H))
        ctx->paddle_r_y = (float)(COURT_H - PADDLE_H);

    /* Ball movement */
    ctx->ball_x += ctx->ball_vx;
    ctx->ball_y += ctx->ball_vy;

    /* Top/bottom bounce */
    if (ctx->ball_y <= 0) {
        ctx->ball_y = 0;
        ctx->ball_vy = -ctx->ball_vy;
    }
    if (ctx->ball_y + BALL_SIZE >= COURT_H) {
        ctx->ball_y = (float)(COURT_H - BALL_SIZE);
        ctx->ball_vy = -ctx->ball_vy;
    }

    /* Left paddle collision */
    if (ctx->ball_x <= PADDLE_W + 10 && ctx->ball_vx < 0) {
        if (ctx->ball_y + BALL_SIZE >= ctx->paddle_l_y &&
            ctx->ball_y <= ctx->paddle_l_y + PADDLE_H) {
            ctx->ball_vx = -ctx->ball_vx * 1.02f;
            float hit_pos = (ctx->ball_y + BALL_SIZE / 2 - ctx->paddle_l_y) / PADDLE_H;
            ctx->ball_vy = (hit_pos - 0.5f) * 6.0f;
            ctx->ball_x = (float)(PADDLE_W + 10);
            ctx->rally++;
        }
    }

    /* Right paddle collision */
    if (ctx->ball_x + BALL_SIZE >= COURT_W - PADDLE_W - 10 && ctx->ball_vx > 0) {
        if (ctx->ball_y + BALL_SIZE >= ctx->paddle_r_y &&
            ctx->ball_y <= ctx->paddle_r_y + PADDLE_H) {
            ctx->ball_vx = -ctx->ball_vx * 1.02f;
            float hit_pos = (ctx->ball_y + BALL_SIZE / 2 - ctx->paddle_r_y) / PADDLE_H;
            ctx->ball_vy = (hit_pos - 0.5f) * 6.0f;
            ctx->ball_x = (float)(COURT_W - PADDLE_W - 10 - BALL_SIZE);
            ctx->rally++;
        }
    }

    /* Scoring */
    if (ctx->ball_x < 0) {
        ctx->score_r++;
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", ctx->score_r);
        lv_label_set_text(ctx->score_r_label, buf);
        serve_ball(ctx, -1);
    }
    if (ctx->ball_x > COURT_W) {
        ctx->score_l++;
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", ctx->score_l);
        lv_label_set_text(ctx->score_l_label, buf);
        serve_ball(ctx, 1);
    }

    /* Win condition */
    if (ctx->score_l >= MAX_SCORE || ctx->score_r >= MAX_SCORE) {
        const char *winner = (ctx->score_l >= MAX_SCORE) ? "You Win!" : "CPU Wins!";
        lv_label_set_text(ctx->status_label, winner);
        lv_obj_clear_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);
        reset_game(ctx);
        return;
    }

    /* Speed cap */
    float spd = sqrtf(ctx->ball_vx * ctx->ball_vx + ctx->ball_vy * ctx->ball_vy);
    if (spd > 8.0f) {
        ctx->ball_vx *= 8.0f / spd;
        ctx->ball_vy *= 8.0f / spd;
    }

    /* Update visuals */
    lv_obj_set_y(ctx->paddle_l, (lv_coord_t)ctx->paddle_l_y);
    lv_obj_set_y(ctx->paddle_r, (lv_coord_t)ctx->paddle_r_y);
    lv_obj_set_pos(ctx->ball, (lv_coord_t)ctx->ball_x, (lv_coord_t)ctx->ball_y);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Score display */
    ctx->score_l_label = lv_label_create(parent);
    lv_label_set_text(ctx->score_l_label, "0");
    lv_obj_set_style_text_color(ctx->score_l_label, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(ctx->score_l_label, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(ctx->score_l_label, COURT_X + COURT_W / 4, 10);

    ctx->score_r_label = lv_label_create(parent);
    lv_label_set_text(ctx->score_r_label, "0");
    lv_obj_set_style_text_color(ctx->score_r_label, UI_COLOR_ERROR, 0);
    lv_obj_set_style_text_font(ctx->score_r_label, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(ctx->score_r_label, COURT_X + 3 * COURT_W / 4, 10);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "PONG");
    lv_obj_set_style_text_color(title, lv_color_hex(0x37474f), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, COURT_X + COURT_W / 2 - 25, 14);

    /* Input mode label */
    ctx->input_label = lv_label_create(parent);
#if BSP_HAS_POTENTIOMETER
    lv_label_set_text(ctx->input_label, "Input: Potentiometer");
#elif BSP_HAS_CAPSENSE
    lv_label_set_text(ctx->input_label, "Input: CapSense Slider");
#else
    lv_label_set_text(ctx->input_label, "Input: Buttons / Tilt");
#endif
    lv_obj_set_style_text_color(ctx->input_label, lv_color_hex(0x546e7a), 0);
    lv_obj_set_style_text_font(ctx->input_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->input_label, COURT_X, 438);

    /* Court */
    ctx->court = lv_obj_create(parent);
    lv_obj_set_size(ctx->court, COURT_W, COURT_H);
    lv_obj_set_pos(ctx->court, COURT_X, COURT_Y);
    lv_obj_set_style_bg_color(ctx->court, lv_color_hex(0x0a1628), 0);
    lv_obj_set_style_bg_opa(ctx->court, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->court, 6, 0);
    lv_obj_set_style_border_width(ctx->court, 2, 0);
    lv_obj_set_style_border_color(ctx->court, lv_color_hex(0x1a3050), 0);
    lv_obj_clear_flag(ctx->court, LV_OBJ_FLAG_SCROLLABLE);

    /* Center line (dashed effect with objects) */
    for (int i = 0; i < COURT_H; i += 20) {
        lv_obj_t *dash = lv_obj_create(ctx->court);
        lv_obj_set_size(dash, 2, 10);
        lv_obj_set_pos(dash, COURT_W / 2 - 1, i);
        lv_obj_set_style_bg_color(dash, lv_color_hex(0x1a3050), 0);
        lv_obj_set_style_bg_opa(dash, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dash, 0, 0);
        lv_obj_clear_flag(dash, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Left paddle */
    ctx->paddle_l = lv_obj_create(ctx->court);
    lv_obj_set_size(ctx->paddle_l, PADDLE_W, PADDLE_H);
    lv_obj_set_pos(ctx->paddle_l, 8, COURT_H / 2 - PADDLE_H / 2);
    lv_obj_set_style_bg_color(ctx->paddle_l, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(ctx->paddle_l, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->paddle_l, 4, 0);
    lv_obj_set_style_border_width(ctx->paddle_l, 0, 0);
    lv_obj_clear_flag(ctx->paddle_l, LV_OBJ_FLAG_SCROLLABLE);

    /* Right paddle */
    ctx->paddle_r = lv_obj_create(ctx->court);
    lv_obj_set_size(ctx->paddle_r, PADDLE_W, PADDLE_H);
    lv_obj_set_pos(ctx->paddle_r, COURT_W - PADDLE_W - 8, COURT_H / 2 - PADDLE_H / 2);
    lv_obj_set_style_bg_color(ctx->paddle_r, UI_COLOR_ERROR, 0);
    lv_obj_set_style_bg_opa(ctx->paddle_r, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->paddle_r, 4, 0);
    lv_obj_set_style_border_width(ctx->paddle_r, 0, 0);
    lv_obj_clear_flag(ctx->paddle_r, LV_OBJ_FLAG_SCROLLABLE);

    /* Ball */
    ctx->ball = lv_obj_create(ctx->court);
    lv_obj_set_size(ctx->ball, BALL_SIZE, BALL_SIZE);
    lv_obj_set_style_bg_color(ctx->ball, UI_COLOR_TEXT, 0);
    lv_obj_set_style_bg_opa(ctx->ball, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->ball, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ctx->ball, 0, 0);
    lv_obj_clear_flag(ctx->ball, LV_OBJ_FLAG_SCROLLABLE);

    /* Control buttons (far right) */
    lv_coord_t bx = COURT_X + COURT_W + 6;

    ctx->up_btn = lv_btn_create(parent);
    lv_obj_set_size(ctx->up_btn, 28, COURT_H / 2 - 4);
    lv_obj_set_pos(ctx->up_btn, bx, COURT_Y);
    lv_obj_set_style_bg_color(ctx->up_btn, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_color(ctx->up_btn, UI_COLOR_PRIMARY, LV_STATE_PRESSED);
    lv_obj_set_style_radius(ctx->up_btn, 6, 0);
    lv_obj_t *ul = lv_label_create(ctx->up_btn);
    lv_label_set_text(ul, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(ul, UI_COLOR_TEXT, 0);
    lv_obj_center(ul);
    lv_obj_add_event_cb(ctx->up_btn, btn_up_cb, LV_EVENT_PRESSED, ctx);
    lv_obj_add_event_cb(ctx->up_btn, btn_up_cb, LV_EVENT_RELEASED, ctx);

    ctx->down_btn = lv_btn_create(parent);
    lv_obj_set_size(ctx->down_btn, 28, COURT_H / 2 - 4);
    lv_obj_set_pos(ctx->down_btn, bx, COURT_Y + COURT_H / 2 + 4);
    lv_obj_set_style_bg_color(ctx->down_btn, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_color(ctx->down_btn, UI_COLOR_PRIMARY, LV_STATE_PRESSED);
    lv_obj_set_style_radius(ctx->down_btn, 6, 0);
    lv_obj_t *dl = lv_label_create(ctx->down_btn);
    lv_label_set_text(dl, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(dl, UI_COLOR_TEXT, 0);
    lv_obj_center(dl);
    lv_obj_add_event_cb(ctx->down_btn, btn_down_cb, LV_EVENT_PRESSED, ctx);
    lv_obj_add_event_cb(ctx->down_btn, btn_down_cb, LV_EVENT_RELEASED, ctx);

    /* Status overlay */
    ctx->status_label = lv_label_create(ctx->court);
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->status_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(ctx->status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ctx->status_label, LV_ALIGN_CENTER, 0, 0);

    reset_game(ctx);
    lv_timer_create(game_tick, TICK_MS, ctx);
}
