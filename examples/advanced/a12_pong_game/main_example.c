/**
 * @file    main_example.c
 * @brief   Pong Game — Ball Physics vs AI Opponent
 *
 * @description
 *   Classic Pong with float ball physics, bounce angles, AI opponent.
 *   Paddle control via joystick/touch. First to 11 wins.
 *   Canvas-based rendering with center line.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "game_common.h"
#include <math.h>

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
    float target_y;
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
    bool         serving;
    int          serve_dir;
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
    ctx->ball.vy = ((float)(xTaskGetTickCount() % 200) / 100.0f - 1.0f) * 2.0f;
    ctx->serving = false;
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
    lv_area_t ball = {
        (int16_t)(ctx->ball.x - BALL_RADIUS), (int16_t)(ctx->ball.y - BALL_RADIUS),
        (int16_t)(ctx->ball.x + BALL_RADIUS), (int16_t)(ctx->ball.y + BALL_RADIUS)
    };
    lv_draw_rect(&layer, &rect, &ball);

    lv_canvas_finish_layer(ctx->canvas, &layer);
}

/* ---------------------------------------------------------------------------
 * Game tick
 * --------------------------------------------------------------------------- */
static void game_tick(lv_timer_t *timer)
{
    pong_ctx_t *ctx = (pong_ctx_t *)lv_timer_get_user_data(timer);
    if (ctx->game_over) return;

    /* Read player input */
    game_input_state_t input;
    game_input_read(&input);

    if (input.up)   ctx->player.y -= PADDLE_SPEED;
    if (input.down)  ctx->player.y += PADDLE_SPEED;

    /* Clamp player paddle */
    if (ctx->player.y < 0) ctx->player.y = 0;
    if (ctx->player.y > COURT_H - PADDLE_H) ctx->player.y = COURT_H - PADDLE_H;

    /* AI logic: track ball with delay and error */
    float ball_center = ctx->ball.y;
    float ai_center = ctx->ai.y + PADDLE_H / 2.0f;
    float ai_error = (float)((int)(xTaskGetTickCount() % AI_ERROR_RANGE) - AI_ERROR_RANGE / 2);
    float ai_target = ball_center + ai_error;

    if (ai_target > ai_center + 2.0f) {
        ctx->ai.y += AI_SPEED;
    } else if (ai_target < ai_center - 2.0f) {
        ctx->ai.y -= AI_SPEED;
    }

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
        /* Bounce with angle based on hit position */
        float hit_pos = (ctx->ball.y - ctx->player.y) / PADDLE_H; /* 0..1 */
        float angle = (hit_pos - 0.5f) * 2.5f; /* -1.25 to 1.25 */
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
 * Restart
 * --------------------------------------------------------------------------- */
static void btn_restart_cb(lv_event_t *e)
{
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

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "PONG");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 16, 4);

    /* Score */
    s_ctx.score_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.score_label, "0  -  0");
    lv_obj_set_style_text_font(s_ctx.score_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_ctx.score_label, COLOR_SCORE, 0);
    lv_obj_align(s_ctx.score_label, LV_ALIGN_TOP_MID, 0, 2);

    /* Canvas */
    s_ctx.canvas_buf = (uint8_t *)lv_malloc(COURT_W * COURT_H * 2);
    if (!s_ctx.canvas_buf) return;

    s_ctx.canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(s_ctx.canvas, s_ctx.canvas_buf,
                         COURT_W, COURT_H, LV_COLOR_FORMAT_RGB565);
    lv_obj_align(s_ctx.canvas, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_border_color(s_ctx.canvas, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(s_ctx.canvas, 1, 0);

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
    lv_obj_set_style_bg_color(restart, lv_color_hex(0x4CAF50), 0);
    lv_obj_add_event_cb(restart, btn_restart_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(restart);
    lv_label_set_text(rl, LV_SYMBOL_REFRESH " Restart");
    lv_obj_center(rl);

    /* Init game */
    game_input_init();
    s_ctx.player.y = COURT_H / 2.0f - PADDLE_H / 2.0f;
    s_ctx.ai.y = COURT_H / 2.0f - PADDLE_H / 2.0f;
    reset_ball(&s_ctx, 1);
    render(&s_ctx);

    s_ctx.game_timer = lv_timer_create(game_tick, TICK_MS, &s_ctx);
}
