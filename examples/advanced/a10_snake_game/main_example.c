/**
 * A10 — Snake Game
 *
 * Classic snake game with directional controls.
 * Uses touch buttons for direction and BMI270 tilt as alternative input.
 * Features growing snake, food spawning, and wraparound movement.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>

#define GRID_W          26
#define GRID_H          17
#define CELL_SIZE       22
#define MAX_SNAKE       (GRID_W * GRID_H)
#define TICK_MS         150
#define BOARD_X         10
#define BOARD_Y         46

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } dir_t;
typedef enum { GS_READY, GS_PLAYING, GS_DEAD } gstate_t;

typedef struct { int x, y; } point_t;

typedef struct {
    lv_obj_t   *parent;
    lv_obj_t   *board;
    lv_obj_t   *cells[GRID_H][GRID_W];
    lv_obj_t   *score_label;
    lv_obj_t   *best_label;
    lv_obj_t   *status_label;
    lv_obj_t   *dir_btns[4];
    point_t     snake[MAX_SNAKE];
    int         snake_len;
    point_t     food;
    dir_t       dir;
    dir_t       next_dir;
    gstate_t    state;
    int         score;
    int         best;
    uint32_t    seed;
} app_ctx_t;

static app_ctx_t g_ctx;

static uint32_t rng(app_ctx_t *ctx)
{
    ctx->seed = ctx->seed * 1664525 + 1013904223;
    return (ctx->seed >> 16) & 0x7FFF;
}

static void spawn_food(app_ctx_t *ctx)
{
    bool occupied;
    do {
        ctx->food.x = (int)(rng(ctx) % GRID_W);
        ctx->food.y = (int)(rng(ctx) % GRID_H);
        occupied = false;
        for (int i = 0; i < ctx->snake_len; i++) {
            if (ctx->snake[i].x == ctx->food.x && ctx->snake[i].y == ctx->food.y) {
                occupied = true;
                break;
            }
        }
    } while (occupied);
}

static void clear_board(app_ctx_t *ctx)
{
    for (int r = 0; r < GRID_H; r++) {
        for (int c = 0; c < GRID_W; c++) {
            lv_obj_set_style_bg_color(ctx->cells[r][c], lv_color_hex(0x0d1520), 0);
        }
    }
}

static void draw_game(app_ctx_t *ctx)
{
    clear_board(ctx);

    /* Draw snake */
    for (int i = 0; i < ctx->snake_len; i++) {
        int x = ctx->snake[i].x;
        int y = ctx->snake[i].y;
        if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) {
            lv_color_t c = (i == 0) ? UI_COLOR_PRIMARY : UI_COLOR_BMI270;
            lv_obj_set_style_bg_color(ctx->cells[y][x], c, 0);
        }
    }

    /* Draw food */
    if (ctx->food.x >= 0 && ctx->food.x < GRID_W &&
        ctx->food.y >= 0 && ctx->food.y < GRID_H) {
        lv_obj_set_style_bg_color(ctx->cells[ctx->food.y][ctx->food.x],
                                  UI_COLOR_ERROR, 0);
    }
}

static void reset_game(app_ctx_t *ctx)
{
    ctx->snake_len = 4;
    ctx->dir = DIR_RIGHT;
    ctx->next_dir = DIR_RIGHT;
    ctx->score = 0;

    int sx = GRID_W / 2;
    int sy = GRID_H / 2;
    for (int i = 0; i < ctx->snake_len; i++) {
        ctx->snake[i].x = sx - i;
        ctx->snake[i].y = sy;
    }

    spawn_food(ctx);
    draw_game(ctx);

    ctx->state = GS_READY;
    lv_label_set_text(ctx->status_label, "Press any direction to start");
    lv_obj_clear_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);

    char sbuf[16];
    snprintf(sbuf, sizeof(sbuf), "Score: %d", ctx->score);
    lv_label_set_text(ctx->score_label, sbuf);
}

static void dir_btn_cb(lv_event_t *e)
{
    app_ctx_t *ctx = &g_ctx;
    int dir_idx = (int)(intptr_t)lv_event_get_user_data(e);

    dir_t new_dir = (dir_t)dir_idx;

    /* Prevent 180-degree turns */
    if ((ctx->dir == DIR_UP && new_dir == DIR_DOWN) ||
        (ctx->dir == DIR_DOWN && new_dir == DIR_UP) ||
        (ctx->dir == DIR_LEFT && new_dir == DIR_RIGHT) ||
        (ctx->dir == DIR_RIGHT && new_dir == DIR_LEFT)) {
        return;
    }

    ctx->next_dir = new_dir;

    if (ctx->state == GS_READY) {
        ctx->state = GS_PLAYING;
        lv_obj_add_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (ctx->state == GS_DEAD) {
        reset_game(ctx);
    }
}

static void game_tick(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    if (ctx->state != GS_PLAYING) return;

    ctx->dir = ctx->next_dir;

    /* Also check tilt for direction input */
#if BSP_HAS_BMI270
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    float ax = (float)snap.bmi270.ax / 16384.0f;
    float ay = (float)snap.bmi270.ay / 16384.0f;

    if (ax > 0.4f && ctx->dir != DIR_LEFT) ctx->next_dir = DIR_RIGHT;
    else if (ax < -0.4f && ctx->dir != DIR_RIGHT) ctx->next_dir = DIR_LEFT;
    else if (ay > 0.4f && ctx->dir != DIR_UP) ctx->next_dir = DIR_DOWN;
    else if (ay < -0.4f && ctx->dir != DIR_DOWN) ctx->next_dir = DIR_UP;
#endif

    /* Calculate new head position */
    point_t new_head = ctx->snake[0];
    switch (ctx->dir) {
    case DIR_UP:    new_head.y--; break;
    case DIR_DOWN:  new_head.y++; break;
    case DIR_LEFT:  new_head.x--; break;
    case DIR_RIGHT: new_head.x++; break;
    }

    /* Wrap around */
    if (new_head.x < 0) new_head.x = GRID_W - 1;
    if (new_head.x >= GRID_W) new_head.x = 0;
    if (new_head.y < 0) new_head.y = GRID_H - 1;
    if (new_head.y >= GRID_H) new_head.y = 0;

    /* Self collision check */
    for (int i = 0; i < ctx->snake_len; i++) {
        if (ctx->snake[i].x == new_head.x && ctx->snake[i].y == new_head.y) {
            ctx->state = GS_DEAD;
            if (ctx->score > ctx->best) ctx->best = ctx->score;

            char msg[64];
            snprintf(msg, sizeof(msg), "Game Over! Score: %d  Best: %d",
                     ctx->score, ctx->best);
            lv_label_set_text(ctx->status_label, msg);
            lv_obj_clear_flag(ctx->status_label, LV_OBJ_FLAG_HIDDEN);

            char bbuf[32];
            snprintf(bbuf, sizeof(bbuf), "Best: %d", ctx->best);
            lv_label_set_text(ctx->best_label, bbuf);
            return;
        }
    }

    /* Check food */
    bool ate = (new_head.x == ctx->food.x && new_head.y == ctx->food.y);

    /* Move snake: shift body */
    if (!ate) {
        /* Remove tail */
        for (int i = ctx->snake_len - 1; i > 0; i--) {
            ctx->snake[i] = ctx->snake[i - 1];
        }
    } else {
        /* Grow: shift everything back, keep old tail */
        if (ctx->snake_len < MAX_SNAKE) {
            for (int i = ctx->snake_len; i > 0; i--) {
                ctx->snake[i] = ctx->snake[i - 1];
            }
            ctx->snake_len++;
        }
        ctx->score += 10;
        spawn_food(ctx);

        char sbuf[16];
        snprintf(sbuf, sizeof(sbuf), "Score: %d", ctx->score);
        lv_label_set_text(ctx->score_label, sbuf);
    }

    ctx->snake[0] = new_head;
    draw_game(ctx);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;
    ctx->seed = 12345;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Header */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 780, 36);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hdr, 8, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, LV_SYMBOL_PLAY " Snake");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    ctx->score_label = lv_label_create(hdr);
    lv_obj_set_style_text_color(ctx->score_label, UI_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(ctx->score_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->score_label, LV_ALIGN_CENTER, 0, 0);

    ctx->best_label = lv_label_create(hdr);
    lv_label_set_text(ctx->best_label, "Best: 0");
    lv_obj_set_style_text_color(ctx->best_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->best_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->best_label, LV_ALIGN_RIGHT_MID, -10, 0);

    /* Game board */
    ctx->board = lv_obj_create(parent);
    lv_coord_t bw = GRID_W * CELL_SIZE + 4;
    lv_coord_t bh = GRID_H * CELL_SIZE + 4;
    lv_obj_set_size(ctx->board, bw, bh);
    lv_obj_set_pos(ctx->board, BOARD_X, BOARD_Y);
    lv_obj_set_style_bg_color(ctx->board, lv_color_hex(0x0a1628), 0);
    lv_obj_set_style_bg_opa(ctx->board, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->board, 6, 0);
    lv_obj_set_style_border_width(ctx->board, 1, 0);
    lv_obj_set_style_border_color(ctx->board, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(ctx->board, 2, 0);
    lv_obj_clear_flag(ctx->board, LV_OBJ_FLAG_SCROLLABLE);

    /* Create grid cells */
    for (int r = 0; r < GRID_H; r++) {
        for (int c = 0; c < GRID_W; c++) {
            ctx->cells[r][c] = lv_obj_create(ctx->board);
            lv_obj_set_size(ctx->cells[r][c], CELL_SIZE - 2, CELL_SIZE - 2);
            lv_obj_set_pos(ctx->cells[r][c], c * CELL_SIZE, r * CELL_SIZE);
            lv_obj_set_style_bg_color(ctx->cells[r][c], lv_color_hex(0x0d1520), 0);
            lv_obj_set_style_bg_opa(ctx->cells[r][c], LV_OPA_COVER, 0);
            lv_obj_set_style_radius(ctx->cells[r][c], 2, 0);
            lv_obj_set_style_border_width(ctx->cells[r][c], 0, 0);
            lv_obj_clear_flag(ctx->cells[r][c], LV_OBJ_FLAG_SCROLLABLE);
        }
    }

    /* Direction buttons (right side) */
    lv_coord_t btn_x = bw + BOARD_X + 16;
    lv_coord_t btn_y = BOARD_Y + 60;
    lv_coord_t btn_sz = 55;
    lv_coord_t btn_gap = 4;

    static const char *btn_icons[] = {
        LV_SYMBOL_UP, LV_SYMBOL_DOWN, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT
    };
    static const lv_coord_t btn_offsets[][2] = {
        {btn_sz + btn_gap, 0},
        {btn_sz + btn_gap, (btn_sz + btn_gap) * 2},
        {0, btn_sz + btn_gap},
        {(btn_sz + btn_gap) * 2, btn_sz + btn_gap},
    };

    for (int i = 0; i < 4; i++) {
        ctx->dir_btns[i] = lv_btn_create(parent);
        lv_obj_set_size(ctx->dir_btns[i], btn_sz, btn_sz);
        lv_obj_set_pos(ctx->dir_btns[i],
                       btn_x + btn_offsets[i][0],
                       btn_y + btn_offsets[i][1]);
        lv_obj_set_style_bg_color(ctx->dir_btns[i], lv_color_hex(0x1a2332), 0);
        lv_obj_set_style_bg_color(ctx->dir_btns[i], UI_COLOR_PRIMARY, LV_STATE_PRESSED);
        lv_obj_set_style_radius(ctx->dir_btns[i], 10, 0);

        lv_obj_t *bl = lv_label_create(ctx->dir_btns[i]);
        lv_label_set_text(bl, btn_icons[i]);
        lv_obj_set_style_text_color(bl, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_20, 0);
        lv_obj_center(bl);

        lv_obj_add_event_cb(ctx->dir_btns[i], dir_btn_cb, LV_EVENT_CLICKED,
                           (void *)(intptr_t)i);
    }

    /* Controls hint */
    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "Touch buttons\nor tilt board");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x546e7a), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(hint, btn_x + 10, btn_y + (btn_sz + btn_gap) * 3 + 10);

    /* Status overlay */
    ctx->status_label = lv_label_create(ctx->board);
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(ctx->status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(ctx->status_label, bw - 20);
    lv_label_set_long_mode(ctx->status_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ctx->status_label, LV_ALIGN_CENTER, 0, 0);

    reset_game(ctx);
    lv_timer_create(game_tick, TICK_MS, ctx);
}
