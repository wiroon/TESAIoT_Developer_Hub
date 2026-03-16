/**
 * @file    main_example.c
 * @brief   Pong Game — Pure LVGL with BMI270 tilt paddle control
 *
 * @description
 *   Classic Pong with float ball physics, AI opponent, bounce angles.
 *   Player paddle control via BMI270 accelerometer tilt (ipc_sensorhub_snapshot)
 *   and touch D-pad. Canvas-based rendering, first to 11 wins.
 *   No external game library dependencies.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define COURT_W         500
#define COURT_H         300
#define PADDLE_W        10
#define PADDLE_H        60
#define BALL_RADIUS     6
#define BALL_SPEED      4.0f
#define PADDLE_SPEED    5.0f
#define AI_SPEED        3.5f
#define AI_ERROR_RANGE  20
#define WIN_SCORE       11
#define TICK_MS         16      /* ~60 FPS */
#define TILT_SCALE      3.0f   /* m/s^2 to paddle speed */

#define COLOR_BG        lv_color_hex(0x0D1B2A)
#define COLOR_PADDLE    lv_color_hex(0xE0E0E0)
#define COLOR_BALL      lv_color_hex(0xFFFFFF)
#define COLOR_LINE      lv_color_hex(0x1A3050)
#define COLOR_SCORE     lv_color_hex(0xFFD740)
#define COLOR_P1        lv_color_hex(0x4CAF50)
#define COLOR_P2        lv_color_hex(0xF44336)

typedef struct {
    float x, y;
    float vx, vy;
} ball_t;

typedef struct {
    float y;
} paddle_t;

typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *canvas;
    lv_obj_t    *score_label;
    lv_obj_t    *result_panel;
    lv_obj_t    *result_label;
    uint8_t     *canvas_buf;
    lv_timer_t  *game_timer;
    ball_t       ball;
    paddle_t     player;
    paddle_t     ai;
    uint8_t      score_p1;
    uint8_t      score_p2;
    bool         game_over;
    /* Touch D-pad */
    bool         touch_up;
    bool         touch_down;
} pong_ctx_t;

static pong_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Reset ball to center
 * --------------------------------------------------------------------------- */
static void reset_ball(pong_ctx_t *ctx, int dir)
{
    ctx->ball.x  = COURT_W / 2.0f;
    ctx->ball.y  = COURT_H / 2.0f;
    ctx->ball.vx = BALL_SPEED * dir;
    ctx->ball.vy = ((float)((int)(lv_tick_get() % 200)) / 100.0f - 1.0f) * 2.0f;
}

/* ---------------------------------------------------------------------------
 * Render
 * --------------------------------------------------------------------------- */
static void render(pong_ctx_t *ctx)
{
    lv_canvas_fill_bg(ctx->canvas, COLOR_BG, LV_OPA_COVER);

    lv_layer_t layer;
    lv_canvas_init_layer(ctx->canvas, &layer);

    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);

    /* Center dashed line */
    rect.bg_color = COLOR_LINE;
    rect.bg_opa   = LV_OPA_COVER;
    rect.radius   = 0;
    for (int y = 0; y < COURT_H; y += 16) {
        lv_area_t dash = { COURT_W / 2 - 1, y, COURT_W / 2 + 1, y + 8 };
        lv_draw_rect(&layer, &rect, &dash);
    }

    /* Player paddle (left) */
    rect.bg_color = COLOR_P1;
    rect.radius   = 3;
    lv_area_t p1 = { 8, (int16_t)ctx->player.y,
                     8 + PADDLE_W, (int16_t)(ctx->player.y + PADDLE_H) };
    lv_draw_rect(&layer, &rect, &p1);

    /* AI paddle (right) */
    rect.bg_color = COLOR_P2;
    lv_area_t p2 = { COURT_W - 8 - PADDLE_W, (int16_t)ctx->ai.y,
                     COURT_W - 8, (int16_t)(ctx->ai.y + PADDLE_H) };
    lv_draw_rect(&layer, &rect, &p2);

    /* Ball */
    rect.bg_color = COLOR_BALL;
    rect.radius   = BALL_RADIUS;
    lv_area_t ball_area = {
        (int16_t)(ctx->ball.x - BALL_RADIUS), (int16_t)(ctx->ball.y - BALL_RADIUS),
        (int16_t)(ctx->ball.x + BALL_RADIUS), (int16_t)(ctx->ball.y + BALL_RADIUS)
    };
    lv_draw_rect(&layer, &rect, &ball_area);

    lv_canvas_finish_layer(ctx->canvas, &layer);
}

/* ---------------------------------------------------------------------------
 * Game tick
 * --------------------------------------------------------------------------- */
static void game_tick(lv_timer_t *timer)
{
    pong_ctx_t *ctx = (pong_ctx_t *)lv_timer_get_user_data(timer);
    if (ctx->game_over) return;

    /* Read BMI270 tilt for paddle control */
    float player_move = 0.0f;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    {
        /* Y-axis tilt moves paddle vertically */
        player_move = (float)snap.bmi270.ay / 16384.0f * TILT_SCALE;
    }

    /* Merge touch D-pad */
    if (ctx->touch_up)   player_move -= PADDLE_SPEED;
    if (ctx->touch_down) player_move += PADDLE_SPEED;
    ctx->touch_up = ctx->touch_down = false;

    ctx->player.y += player_move;

    /* Clamp player paddle */
    if (ctx->player.y < 0) ctx->player.y = 0;
    if (ctx->player.y > COURT_H - PADDLE_H) ctx->player.y = COURT_H - PADDLE_H;

    /* AI logic: track ball with delay and error */
    float ball_center = ctx->ball.y;
    float ai_center = ctx->ai.y + PADDLE_H / 2.0f;
    float ai_error = (float)((int)(lv_tick_get() % AI_ERROR_RANGE) - AI_ERROR_RANGE / 2);
    float ai_target = ball_center + ai_error;

    if (ai_target > ai_center + 2.0f) ctx->ai.y += AI_SPEED;
    else if (ai_target < ai_center - 2.0f) ctx->ai.y -= AI_SPEED;

    if (ctx->ai.y < 0) ctx->ai.y = 0;
    if (ctx->ai.y > COURT_H - PADDLE_H) ctx->ai.y = COURT_H - PADDLE_H;

    /* Move ball */
    ctx->ball.x += ctx->ball.vx;
    ctx->ball.y += ctx->ball.vy;

    /* Top/bottom bounce */
    if (ctx->ball.y <= BALL_RADIUS) {
        ctx->ball.y = BALL_RADIUS;
        ctx->ball.vy = -ctx->ball.vy;
    }
    if (ctx->ball.y >= COURT_H - BALL_RADIUS) {
        ctx->ball.y = COURT_H - BALL_RADIUS;
        ctx->ball.vy = -ctx->ball.vy;
    }

    /* Player paddle collision */
    if (ctx->ball.x - BALL_RADIUS <= 8 + PADDLE_W &&
        ctx->ball.x > 8 &&
        ctx->ball.y >= ctx->player.y &&
        ctx->ball.y <= ctx->player.y + PADDLE_H) {
        float hit_pos = (ctx->ball.y - ctx->player.y) / PADDLE_H;
        float angle = (hit_pos - 0.5f) * 2.5f;
        ctx->ball.vx = BALL_SPEED;
        ctx->ball.vy = angle * BALL_SPEED;
        ctx->ball.x = 8 + PADDLE_W + BALL_RADIUS;
    }

    /* AI paddle collision */
    if (ctx->ball.x + BALL_RADIUS >= COURT_W - 8 - PADDLE_W &&
        ctx->ball.x < COURT_W - 8 &&
        ctx->ball.y >= ctx->ai.y &&
        ctx->ball.y <= ctx->ai.y + PADDLE_H) {
        float hit_pos = (ctx->ball.y - ctx->ai.y) / PADDLE_H;
        float angle = (hit_pos - 0.5f) * 2.5f;
        ctx->ball.vx = -BALL_SPEED;
        ctx->ball.vy = angle * BALL_SPEED;
        ctx->ball.x = COURT_W - 8 - PADDLE_W - BALL_RADIUS;
    }

    /* Score: ball past paddle */
    if (ctx->ball.x < 0) {
        ctx->score_p2++;
        reset_ball(ctx, -1);
    }
    if (ctx->ball.x > COURT_W) {
        ctx->score_p1++;
        reset_ball(ctx, 1);
    }

    /* Update score display */
    char score_str[16];
    snprintf(score_str, sizeof(score_str), "%u  -  %u", ctx->score_p1, ctx->score_p2);
    lv_label_set_text(ctx->score_label, score_str);

    /* Check win condition */
    if (ctx->score_p1 >= WIN_SCORE || ctx->score_p2 >= WIN_SCORE) {
        ctx->game_over = true;
        lv_obj_remove_flag(ctx->result_panel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ctx->result_label,
                          ctx->score_p1 >= WIN_SCORE ? "YOU WIN!" : "AI WINS!");
        lv_obj_set_style_text_color(ctx->result_label,
                                     ctx->score_p1 >= WIN_SCORE ? COLOR_P1 : COLOR_P2, 0);
    }

    render(ctx);
}

/* ---------------------------------------------------------------------------
 * D-pad and restart callbacks
 * --------------------------------------------------------------------------- */
static void dpad_up_cb(lv_event_t *e)   { s_ctx.touch_up = true;   (void)e; }
static void dpad_down_cb(lv_event_t *e) { s_ctx.touch_down = true; (void)e; }

static void btn_restart_cb(lv_event_t *e)
{
    (void)e;
    pong_ctx_t *ctx = &s_ctx;
    ctx->score_p1 = 0;
    ctx->score_p2 = 0;
    ctx->game_over = false;
    ctx->player.y = COURT_H / 2.0f - PADDLE_H / 2.0f;
    ctx->ai.y = COURT_H / 2.0f - PADDLE_H / 2.0f;
    reset_ball(ctx, 1);
    lv_obj_add_flag(ctx->result_panel, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ctx->score_label, "0  -  0");
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* Title + player labels */
    lv_obj_t *p1_lbl = lv_label_create(parent);
    lv_label_set_text(p1_lbl, "YOU");
    lv_obj_set_style_text_color(p1_lbl, COLOR_P1, 0);
    lv_obj_set_style_text_font(p1_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(p1_lbl, LV_ALIGN_TOP_LEFT, 16, 4);

    s_ctx.score_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.score_label, "0  -  0");
    lv_obj_set_style_text_font(s_ctx.score_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_ctx.score_label, COLOR_SCORE, 0);
    lv_obj_align(s_ctx.score_label, LV_ALIGN_TOP_MID, 0, 2);

    lv_obj_t *p2_lbl = lv_label_create(parent);
    lv_label_set_text(p2_lbl, "AI");
    lv_obj_set_style_text_color(p2_lbl, COLOR_P2, 0);
    lv_obj_set_style_text_font(p2_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(p2_lbl, LV_ALIGN_TOP_RIGHT, -16, 4);

    /* Canvas */
    s_ctx.canvas_buf = (uint8_t *)lv_malloc(COURT_W * COURT_H * 2);
    if (!s_ctx.canvas_buf) return;

    s_ctx.canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(s_ctx.canvas, s_ctx.canvas_buf,
                         COURT_W, COURT_H, LV_COLOR_FORMAT_RGB565);
    lv_obj_align(s_ctx.canvas, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_border_color(s_ctx.canvas, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(s_ctx.canvas, 1, 0);

    /* D-pad buttons (below canvas) */
    lv_obj_t *up_btn = lv_btn_create(parent);
    lv_obj_set_size(up_btn, 80, 36);
    lv_obj_align(up_btn, LV_ALIGN_BOTTOM_MID, -50, -8);
    lv_obj_set_style_bg_color(up_btn, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_border_color(up_btn, COLOR_P1, 0);
    lv_obj_set_style_border_width(up_btn, 1, 0);
    lv_obj_set_style_radius(up_btn, 8, 0);
    lv_obj_add_event_cb(up_btn, dpad_up_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ul = lv_label_create(up_btn);
    lv_label_set_text(ul, LV_SYMBOL_UP " Up");
    lv_obj_center(ul);

    lv_obj_t *down_btn = lv_btn_create(parent);
    lv_obj_set_size(down_btn, 80, 36);
    lv_obj_align(down_btn, LV_ALIGN_BOTTOM_MID, 50, -8);
    lv_obj_set_style_bg_color(down_btn, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_border_color(down_btn, COLOR_P1, 0);
    lv_obj_set_style_border_width(down_btn, 1, 0);
    lv_obj_set_style_radius(down_btn, 8, 0);
    lv_obj_add_event_cb(down_btn, dpad_down_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *dl = lv_label_create(down_btn);
    lv_label_set_text(dl, LV_SYMBOL_DOWN " Down");
    lv_obj_center(dl);

    /* Tilt hint */
    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "Tilt board to move paddle");
    lv_obj_set_style_text_color(hint, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -48);

    /* Result panel */
    s_ctx.result_panel = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.result_panel, 200, 100);
    lv_obj_align(s_ctx.result_panel, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_color(s_ctx.result_panel, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(s_ctx.result_panel, LV_OPA_90, 0);
    lv_obj_set_style_border_color(s_ctx.result_panel, COLOR_SCORE, 0);
    lv_obj_set_style_border_width(s_ctx.result_panel, 2, 0);
    lv_obj_set_style_radius(s_ctx.result_panel, 12, 0);
    lv_obj_add_flag(s_ctx.result_panel, LV_OBJ_FLAG_HIDDEN);

    s_ctx.result_label = lv_label_create(s_ctx.result_panel);
    lv_label_set_text(s_ctx.result_label, "");
    lv_obj_set_style_text_font(s_ctx.result_label, &lv_font_montserrat_24, 0);
    lv_obj_align(s_ctx.result_label, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *restart = lv_btn_create(s_ctx.result_panel);
    lv_obj_set_size(restart, 120, 36);
    lv_obj_align(restart, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(restart, UI_COLOR_SUCCESS, 0);
    lv_obj_add_event_cb(restart, btn_restart_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(restart);
    lv_label_set_text(rl, LV_SYMBOL_REFRESH " Restart");
    lv_obj_center(rl);

    /* Init game */
    s_ctx.player.y = COURT_H / 2.0f - PADDLE_H / 2.0f;
    s_ctx.ai.y = COURT_H / 2.0f - PADDLE_H / 2.0f;
    reset_ball(&s_ctx, 1);
    render(&s_ctx);

    s_ctx.game_timer = lv_timer_create(game_tick, TICK_MS, &s_ctx);
}
