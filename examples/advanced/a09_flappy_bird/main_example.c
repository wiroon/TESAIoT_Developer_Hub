/**
 * A09 — Flappy Bird (Tilt Control)
 *
 * Flappy bird game controlled by tilting the board (BMI270).
 * Features scrolling pipes, score counter, collision detection,
 * and game-over screen with restart.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define GAME_W          780
#define GAME_H          420
#define BIRD_SIZE       20
#define PIPE_W          40
#define PIPE_GAP        120
#define NUM_PIPES       4
#define PIPE_SPEED      3
#define GRAVITY         0.4f
#define TILT_SENS       0.08f
#define TICK_MS         33

typedef enum { ST_READY, ST_PLAYING, ST_DEAD } game_state_t;

typedef struct {
    lv_coord_t x;
    lv_coord_t gap_y;   /* center of gap */
    bool       scored;
} pipe_t;

typedef struct {
    lv_obj_t     *parent;
    lv_obj_t     *game_area;
    lv_obj_t     *bird;
    lv_obj_t     *pipe_tops[NUM_PIPES];
    lv_obj_t     *pipe_bots[NUM_PIPES];
    lv_obj_t     *score_label;
    lv_obj_t     *status_label;
    lv_obj_t     *best_label;
    pipe_t        pipes[NUM_PIPES];
    float         bird_y;
    float         bird_vy;
    int           score;
    int           best_score;
    game_state_t  state;
    uint32_t      tick;
    uint32_t      seed;
} app_ctx_t;

static app_ctx_t g_ctx;

static uint32_t lcg_rand(app_ctx_t *ctx)
{
    ctx->seed = ctx->seed * 1103515245 + 12345;
    return (ctx->seed >> 16) & 0x7FFF;
}

static void init_pipes(app_ctx_t *ctx)
{
    for (int i = 0; i < NUM_PIPES; i++) {
        ctx->pipes[i].x = GAME_W + i * (GAME_W / NUM_PIPES + PIPE_W);
        ctx->pipes[i].gap_y = 80 + (int)(lcg_rand(ctx) % (GAME_H - 200));
        ctx->pipes[i].scored = false;
    }
}

static void reset_game(app_ctx_t *ctx)
{
    ctx->bird_y  = (float)(GAME_H / 2);
    ctx->bird_vy = 0;
    ctx->score   = 0;
    ctx->tick    = 0;
    init_pipes(ctx);
    ctx->state = ST_READY;
    lv_label_set_text(ctx->status_label, "Tilt forward to start!");
    lv_obj_clear_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);
}

static void update_pipe_visuals(app_ctx_t *ctx)
{
    for (int i = 0; i < NUM_PIPES; i++) {
        pipe_t *p = &ctx->pipes[i];
        lv_coord_t top_h = p->gap_y - PIPE_GAP / 2;
        lv_coord_t bot_y = p->gap_y + PIPE_GAP / 2;
        lv_coord_t bot_h = GAME_H - bot_y;

        if (top_h < 0) top_h = 0;
        if (bot_h < 0) bot_h = 0;

        lv_obj_set_pos(ctx->pipe_tops[i], p->x, 0);
        lv_obj_set_size(ctx->pipe_tops[i], PIPE_W, top_h);

        lv_obj_set_pos(ctx->pipe_bots[i], p->x, bot_y);
        lv_obj_set_size(ctx->pipe_bots[i], PIPE_W, bot_h);
    }
}

static bool check_collision(app_ctx_t *ctx)
{
    lv_coord_t bx = 120;
    lv_coord_t by = (lv_coord_t)ctx->bird_y;

    if (by < 0 || by + BIRD_SIZE > GAME_H) return true;

    for (int i = 0; i < NUM_PIPES; i++) {
        pipe_t *p = &ctx->pipes[i];
        if (bx + BIRD_SIZE > p->x && bx < p->x + PIPE_W) {
            lv_coord_t top_h = p->gap_y - PIPE_GAP / 2;
            lv_coord_t bot_y = p->gap_y + PIPE_GAP / 2;
            if (by < top_h || by + BIRD_SIZE > bot_y) return true;
        }
    }
    return false;
}

static void game_tick(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

#if BSP_HAS_BMI270
    float tilt = (float)snap.bmi270.ay / 16384.0f;
#else
    float tilt = 0.0f;
#endif

    if (ctx->state == ST_READY) {
        /* Wait for forward tilt to start */
        if (tilt < -0.3f) {
            ctx->state = ST_PLAYING;
            lv_obj_add_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_y(ctx->bird, (lv_coord_t)ctx->bird_y);
        return;
    }

    if (ctx->state == ST_DEAD) {
        /* Wait for tilt to restart */
        if (tilt > 0.5f) {
            reset_game(ctx);
        }
        return;
    }

    /* Playing */
    ctx->tick++;

    /* Bird physics: tilt controls vertical velocity */
    ctx->bird_vy += GRAVITY;
    ctx->bird_vy -= tilt * TILT_SENS * 30.0f;

    /* Clamp velocity */
    if (ctx->bird_vy > 8.0f) ctx->bird_vy = 8.0f;
    if (ctx->bird_vy < -8.0f) ctx->bird_vy = -8.0f;

    ctx->bird_y += ctx->bird_vy;

    /* Move pipes */
    for (int i = 0; i < NUM_PIPES; i++) {
        ctx->pipes[i].x -= PIPE_SPEED;

        /* Score check */
        if (!ctx->pipes[i].scored && ctx->pipes[i].x + PIPE_W < 120) {
            ctx->pipes[i].scored = true;
            ctx->score++;
            char sbuf[16];
            snprintf(sbuf, sizeof(sbuf), "%d", ctx->score);
            lv_label_set_text(ctx->score_label, sbuf);
        }

        /* Recycle pipe */
        if (ctx->pipes[i].x + PIPE_W < 0) {
            ctx->pipes[i].x = GAME_W;
            ctx->pipes[i].gap_y = 80 + (int)(lcg_rand(ctx) % (GAME_H - 200));
            ctx->pipes[i].scored = false;
        }
    }

    /* Update visuals */
    lv_obj_set_y(ctx->bird, (lv_coord_t)ctx->bird_y);
    update_pipe_visuals(ctx);

    /* Collision */
    if (check_collision(ctx)) {
        ctx->state = ST_DEAD;
        if (ctx->score > ctx->best_score) ctx->best_score = ctx->score;

        char msg[64];
        snprintf(msg, sizeof(msg), "Game Over!  Score: %d  Best: %d\nTilt back to restart",
                 ctx->score, ctx->best_score);
        lv_label_set_text(ctx->status_label, msg);
        lv_obj_clear_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);

        char bbuf[32];
        snprintf(bbuf, sizeof(bbuf), "Best: %d", ctx->best_score);
        lv_label_set_text(ctx->best_label, bbuf);
    }
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;
    ctx->seed = 42;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Score header */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 780, 36);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hdr, 8, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, LV_SYMBOL_PLAY " Flappy Bird");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    ctx->score_label = lv_label_create(hdr);
    lv_label_set_text(ctx->score_label, "0");
    lv_obj_set_style_text_color(ctx->score_label, UI_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(ctx->score_label, &lv_font_montserrat_24, 0);
    lv_obj_align(ctx->score_label, LV_ALIGN_CENTER, 0, 0);

    ctx->best_label = lv_label_create(hdr);
    lv_label_set_text(ctx->best_label, "Best: 0");
    lv_obj_set_style_text_color(ctx->best_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->best_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->best_label, LV_ALIGN_RIGHT_MID, -10, 0);

    /* Game area */
    ctx->game_area = lv_obj_create(parent);
    lv_obj_set_size(ctx->game_area, GAME_W, GAME_H);
    lv_obj_set_pos(ctx->game_area, 10, 42);
    lv_obj_set_style_bg_color(ctx->game_area, lv_color_hex(0x0a1628), 0);
    lv_obj_set_style_bg_opa(ctx->game_area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->game_area, 8, 0);
    lv_obj_set_style_border_width(ctx->game_area, 1, 0);
    lv_obj_set_style_border_color(ctx->game_area, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_clip_corner(ctx->game_area, true, 0);
    lv_obj_clear_flag(ctx->game_area, LV_OBJ_FLAG_SCROLLABLE);

    /* Ground line */
    lv_obj_t *ground = lv_obj_create(ctx->game_area);
    lv_obj_set_size(ground, GAME_W, 4);
    lv_obj_set_pos(ground, 0, GAME_H - 4);
    lv_obj_set_style_bg_color(ground, UI_COLOR_BMI270, 0);
    lv_obj_set_style_bg_opa(ground, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ground, 0, 0);
    lv_obj_clear_flag(ground, LV_OBJ_FLAG_SCROLLABLE);

    /* Bird */
    ctx->bird = lv_obj_create(ctx->game_area);
    lv_obj_set_size(ctx->bird, BIRD_SIZE, BIRD_SIZE);
    lv_obj_set_pos(ctx->bird, 120, GAME_H / 2);
    lv_obj_set_style_bg_color(ctx->bird, UI_COLOR_WARNING, 0);
    lv_obj_set_style_bg_opa(ctx->bird, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->bird, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ctx->bird, 2, 0);
    lv_obj_set_style_border_color(ctx->bird, UI_COLOR_DPS368, 0);
    lv_obj_clear_flag(ctx->bird, LV_OBJ_FLAG_SCROLLABLE);

    /* Pipes */
    for (int i = 0; i < NUM_PIPES; i++) {
        ctx->pipe_tops[i] = lv_obj_create(ctx->game_area);
        lv_obj_set_style_bg_color(ctx->pipe_tops[i], UI_COLOR_BMI270, 0);
        lv_obj_set_style_bg_opa(ctx->pipe_tops[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->pipe_tops[i], 4, 0);
        lv_obj_set_style_border_width(ctx->pipe_tops[i], 0, 0);
        lv_obj_clear_flag(ctx->pipe_tops[i], LV_OBJ_FLAG_SCROLLABLE);

        ctx->pipe_bots[i] = lv_obj_create(ctx->game_area);
        lv_obj_set_style_bg_color(ctx->pipe_bots[i], UI_COLOR_BMI270, 0);
        lv_obj_set_style_bg_opa(ctx->pipe_bots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->pipe_bots[i], 4, 0);
        lv_obj_set_style_border_width(ctx->pipe_bots[i], 0, 0);
        lv_obj_clear_flag(ctx->pipe_bots[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Status overlay */
    ctx->status_label = lv_label_create(ctx->game_area);
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->status_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(ctx->status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(ctx->status_label, 400);
    lv_label_set_long_mode(ctx->status_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ctx->status_label, LV_ALIGN_CENTER, 0, 0);

    reset_game(ctx);
    update_pipe_visuals(ctx);

    lv_timer_create(game_tick, TICK_MS, ctx);
}
