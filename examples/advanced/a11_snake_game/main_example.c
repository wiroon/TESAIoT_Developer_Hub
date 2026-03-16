/**
 * @file    main_example.c
 * @brief   Snake Game — Grid-based with Joystick/Touch Control
 *
 * @description
 *   Classic Snake on 20x15 grid. F310 joystick + touch D-pad input.
 *   Food, growth, collision, scoring. Canvas-based rendering.
 *   Snake body as circular buffer of grid coordinates.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "game_common.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define GRID_W          20
#define GRID_H          15
#define CELL_SIZE       20      /* pixels per cell */
#define CANVAS_W        (GRID_W * CELL_SIZE)    /* 400 */
#define CANVAS_H        (GRID_H * CELL_SIZE)    /* 300 */
#define MAX_SNAKE_LEN   (GRID_W * GRID_H)
#define INITIAL_LEN     4
#define INITIAL_SPEED   200     /* ms per tick */
#define MIN_SPEED       80
#define SPEED_STEP      5

#define COLOR_BG_CELL   lv_color_hex(0x0D1B2A)
#define COLOR_GRID      lv_color_hex(0x142240)
#define COLOR_SNAKE_HEAD lv_color_hex(0x66BB6A)
#define COLOR_SNAKE_BODY lv_color_hex(0x388E3C)
#define COLOR_FOOD      lv_color_hex(0xF44336)
#define COLOR_TEXT       lv_color_hex(0xE0E0E0)

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } direction_t;

typedef struct {
    int8_t x;
    int8_t y;
} point_t;

typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *canvas;
    lv_obj_t    *score_label;
    lv_obj_t    *status_label;
    lv_obj_t    *gameover_panel;
    lv_obj_t    *final_score_label;
    lv_obj_t    *btn_restart;
    uint8_t     *canvas_buf;
    lv_timer_t  *game_timer;

    point_t      snake[MAX_SNAKE_LEN];
    uint16_t     snake_len;
    uint16_t     head_idx;   /* Circular buffer head */
    uint16_t     tail_idx;
    direction_t  dir;
    direction_t  next_dir;   /* Buffered input */
    point_t      food;
    uint32_t     score;
    uint32_t     speed_ms;
    bool         game_over;
    bool         started;
} snake_ctx_t;

static snake_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Simple PRNG
 * --------------------------------------------------------------------------- */
static uint32_t s_rng_state = 0;

static uint32_t rng_next(void)
{
    s_rng_state ^= s_rng_state << 13;
    s_rng_state ^= s_rng_state >> 17;
    s_rng_state ^= s_rng_state << 5;
    return s_rng_state;
}

/* ---------------------------------------------------------------------------
 * Check if point is on snake body
 * --------------------------------------------------------------------------- */
static bool is_snake(snake_ctx_t *ctx, int8_t x, int8_t y)
{
    uint16_t idx = ctx->tail_idx;
    for (uint16_t i = 0; i < ctx->snake_len; i++) {
        if (ctx->snake[idx].x == x && ctx->snake[idx].y == y) return true;
        idx = (idx + 1) % MAX_SNAKE_LEN;
    }
    return false;
}

/* ---------------------------------------------------------------------------
 * Spawn food at random empty position
 * --------------------------------------------------------------------------- */
static void spawn_food(snake_ctx_t *ctx)
{
    int attempts = 0;
    do {
        ctx->food.x = rng_next() % GRID_W;
        ctx->food.y = rng_next() % GRID_H;
        attempts++;
    } while (is_snake(ctx, ctx->food.x, ctx->food.y) && attempts < 1000);
}

/* ---------------------------------------------------------------------------
 * Draw the grid on canvas
 * --------------------------------------------------------------------------- */
static void render(snake_ctx_t *ctx)
{
    /* Clear canvas */
    lv_canvas_fill_bg(ctx->canvas, COLOR_BG_CELL, LV_OPA_COVER);

    /* Draw grid lines */
    lv_layer_t layer;
    lv_canvas_init_layer(ctx->canvas, &layer);

    /* Draw food */
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = COLOR_FOOD;
    rect.bg_opa   = LV_OPA_COVER;
    rect.radius   = 4;

    lv_area_t area = {
        ctx->food.x * CELL_SIZE + 2, ctx->food.y * CELL_SIZE + 2,
        (ctx->food.x + 1) * CELL_SIZE - 2, (ctx->food.y + 1) * CELL_SIZE - 2
    };
    lv_draw_rect(&layer, &rect, &area);

    /* Draw snake */
    uint16_t idx = ctx->tail_idx;
    for (uint16_t i = 0; i < ctx->snake_len; i++) {
        bool is_head = (i == ctx->snake_len - 1);
        rect.bg_color = is_head ? COLOR_SNAKE_HEAD : COLOR_SNAKE_BODY;
        rect.radius   = is_head ? 6 : 3;

        lv_area_t cell = {
            ctx->snake[idx].x * CELL_SIZE + 1,
            ctx->snake[idx].y * CELL_SIZE + 1,
            (ctx->snake[idx].x + 1) * CELL_SIZE - 1,
            (ctx->snake[idx].y + 1) * CELL_SIZE - 1
        };
        lv_draw_rect(&layer, &rect, &cell);
        idx = (idx + 1) % MAX_SNAKE_LEN;
    }

    lv_canvas_finish_layer(ctx->canvas, &layer);
}

/* ---------------------------------------------------------------------------
 * Game tick
 * --------------------------------------------------------------------------- */
static void game_tick(lv_timer_t *timer)
{
    snake_ctx_t *ctx = (snake_ctx_t *)lv_timer_get_user_data(timer);
    if (ctx->game_over) return;

    /* Read input */
    game_input_state_t input;
    game_input_read(&input);

    if (input.up && ctx->dir != DIR_DOWN)        ctx->next_dir = DIR_UP;
    else if (input.down && ctx->dir != DIR_UP)    ctx->next_dir = DIR_DOWN;
    else if (input.left && ctx->dir != DIR_RIGHT) ctx->next_dir = DIR_LEFT;
    else if (input.right && ctx->dir != DIR_LEFT) ctx->next_dir = DIR_RIGHT;

    ctx->dir = ctx->next_dir;

    /* Calculate new head position */
    uint16_t cur_head = (ctx->tail_idx + ctx->snake_len - 1) % MAX_SNAKE_LEN;
    point_t new_head = ctx->snake[cur_head];

    switch (ctx->dir) {
        case DIR_UP:    new_head.y--; break;
        case DIR_DOWN:  new_head.y++; break;
        case DIR_LEFT:  new_head.x--; break;
        case DIR_RIGHT: new_head.x++; break;
    }

    /* Wall collision */
    if (new_head.x < 0 || new_head.x >= GRID_W ||
        new_head.y < 0 || new_head.y >= GRID_H) {
        ctx->game_over = true;
        goto gameover;
    }

    /* Self collision */
    if (is_snake(ctx, new_head.x, new_head.y)) {
        ctx->game_over = true;
        goto gameover;
    }

    /* Add new head */
    uint16_t new_head_idx = (ctx->tail_idx + ctx->snake_len) % MAX_SNAKE_LEN;
    ctx->snake[new_head_idx] = new_head;

    /* Check food */
    if (new_head.x == ctx->food.x && new_head.y == ctx->food.y) {
        ctx->snake_len++;
        ctx->score += 10;

        /* Speed up */
        if (ctx->speed_ms > MIN_SPEED) {
            ctx->speed_ms -= SPEED_STEP;
            lv_timer_set_period(timer, ctx->speed_ms);
        }

        spawn_food(ctx);

        char score_str[24];
        snprintf(score_str, sizeof(score_str), "Score: %lu", (unsigned long)ctx->score);
        lv_label_set_text(ctx->score_label, score_str);
    } else {
        /* Remove tail (no growth) */
        ctx->tail_idx = (ctx->tail_idx + 1) % MAX_SNAKE_LEN;
    }

    render(ctx);
    return;

gameover:
    render(ctx);

    /* Show game over panel */
    lv_obj_remove_flag(ctx->gameover_panel, LV_OBJ_FLAG_HIDDEN);
    char final[32];
    snprintf(final, sizeof(final), "Score: %lu", (unsigned long)ctx->score);
    lv_label_set_text(ctx->final_score_label, final);
    lv_label_set_text(ctx->status_label, "GAME OVER");
}

/* ---------------------------------------------------------------------------
 * Initialize game state
 * --------------------------------------------------------------------------- */
static void init_game(snake_ctx_t *ctx)
{
    ctx->snake_len = INITIAL_LEN;
    ctx->tail_idx  = 0;
    ctx->dir       = DIR_RIGHT;
    ctx->next_dir  = DIR_RIGHT;
    ctx->score     = 0;
    ctx->speed_ms  = INITIAL_SPEED;
    ctx->game_over = false;

    /* Place snake in center */
    int8_t start_x = GRID_W / 2 - INITIAL_LEN / 2;
    int8_t start_y = GRID_H / 2;

    for (uint16_t i = 0; i < INITIAL_LEN; i++) {
        ctx->snake[i].x = start_x + i;
        ctx->snake[i].y = start_y;
    }

    s_rng_state = xTaskGetTickCount();
    spawn_food(ctx);

    lv_label_set_text(ctx->score_label, "Score: 0");
    lv_label_set_text(ctx->status_label, "Playing");
    lv_obj_add_flag(ctx->gameover_panel, LV_OBJ_FLAG_HIDDEN);

    if (ctx->game_timer) {
        lv_timer_set_period(ctx->game_timer, INITIAL_SPEED);
    }

    render(ctx);
}

/* ---------------------------------------------------------------------------
 * Restart callback
 * --------------------------------------------------------------------------- */
static void btn_restart_cb(lv_event_t *e)
{
    init_game(&s_ctx);
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
    lv_label_set_text(title, LV_SYMBOL_PLAY " Snake");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 16, 4);

    /* Score */
    s_ctx.score_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.score_label, "Score: 0");
    lv_obj_set_style_text_font(s_ctx.score_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_ctx.score_label, lv_color_hex(0xFFD740), 0);
    lv_obj_align(s_ctx.score_label, LV_ALIGN_TOP_RIGHT, -16, 8);

    /* Status */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Playing");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_MID, 0, 8);

    /* Canvas */
    s_ctx.canvas_buf = (uint8_t *)lv_malloc(CANVAS_W * CANVAS_H * 2);
    if (!s_ctx.canvas_buf) {
        lv_label_set_text(s_ctx.status_label, "Alloc failed!");
        return;
    }

    s_ctx.canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(s_ctx.canvas, s_ctx.canvas_buf,
                         CANVAS_W, CANVAS_H, LV_COLOR_FORMAT_RGB565);
    lv_obj_align(s_ctx.canvas, LV_ALIGN_CENTER, 0, 16);
    lv_obj_set_style_border_color(s_ctx.canvas, lv_color_hex(0x388E3C), 0);
    lv_obj_set_style_border_width(s_ctx.canvas, 2, 0);

    /* Game over panel (centered overlay) */
    s_ctx.gameover_panel = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.gameover_panel, 220, 120);
    lv_obj_align(s_ctx.gameover_panel, LV_ALIGN_CENTER, 0, 16);
    lv_obj_set_style_bg_color(s_ctx.gameover_panel, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(s_ctx.gameover_panel, LV_OPA_90, 0);
    lv_obj_set_style_border_color(s_ctx.gameover_panel, COLOR_FOOD, 0);
    lv_obj_set_style_border_width(s_ctx.gameover_panel, 2, 0);
    lv_obj_set_style_radius(s_ctx.gameover_panel, 12, 0);
    lv_obj_add_flag(s_ctx.gameover_panel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *go_title = lv_label_create(s_ctx.gameover_panel);
    lv_label_set_text(go_title, "GAME OVER");
    lv_obj_set_style_text_font(go_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(go_title, COLOR_FOOD, 0);
    lv_obj_align(go_title, LV_ALIGN_TOP_MID, 0, 8);

    s_ctx.final_score_label = lv_label_create(s_ctx.gameover_panel);
    lv_label_set_text(s_ctx.final_score_label, "Score: 0");
    lv_obj_set_style_text_font(s_ctx.final_score_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_ctx.final_score_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_ctx.final_score_label, LV_ALIGN_CENTER, 0, -4);

    s_ctx.btn_restart = lv_btn_create(s_ctx.gameover_panel);
    lv_obj_set_size(s_ctx.btn_restart, 120, 36);
    lv_obj_align(s_ctx.btn_restart, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_bg_color(s_ctx.btn_restart, lv_color_hex(0x4CAF50), 0);
    lv_obj_add_event_cb(s_ctx.btn_restart, btn_restart_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(s_ctx.btn_restart);
    lv_label_set_text(rl, LV_SYMBOL_REFRESH " Restart");
    lv_obj_center(rl);

    /* Initialize and start game */
    game_input_init();
    init_game(&s_ctx);
    s_ctx.game_timer = lv_timer_create(game_tick, INITIAL_SPEED, &s_ctx);
}
