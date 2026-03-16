/**
 * @file main_example.c
 * @brief B17 — Reaction Game: Tap when the color changes, measure reaction time.
 *
 * A simple reaction-time game. A panel shows red, then after a random delay
 * turns green. The player taps as fast as possible and their reaction time
 * is displayed in milliseconds.
 */

#include "example_common.h"

typedef enum {
    STATE_IDLE,
    STATE_WAITING,   /* red — wait for green */
    STATE_GO,        /* green — tap now!      */
    STATE_RESULT,    /* show reaction time    */
} game_state_t;

static game_state_t state = STATE_IDLE;
static lv_obj_t    *panel;
static lv_obj_t    *msg_label;
static lv_obj_t    *time_label;
static uint32_t     go_tick;        /* tick when panel turned green */
static uint32_t     delay_remain;   /* ticks until GO */

static void game_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (state != STATE_WAITING) return;

    /* Count down the random delay (timer fires every 50 ms) */
    if (delay_remain <= 50) {
        /* GO! */
        state   = STATE_GO;
        go_tick = xTaskGetTickCount();
        lv_obj_set_style_bg_color(panel, UI_COLOR_SUCCESS, 0);
        lv_label_set_text(msg_label, "TAP NOW!");
    } else {
        delay_remain -= 50;
    }
}

static void panel_click_cb(lv_event_t *e)
{
    (void)e;

    switch (state) {
    case STATE_IDLE:
    case STATE_RESULT:
        /* Start a new round */
        state = STATE_WAITING;
        /* Random delay between 1.5 and 4 seconds */
        delay_remain = 1500 + (xTaskGetTickCount() % 2500);
        lv_obj_set_style_bg_color(panel, UI_COLOR_ERROR, 0);
        lv_label_set_text(msg_label, "Wait for green...");
        lv_label_set_text(time_label, "");
        break;

    case STATE_WAITING:
        /* Tapped too early */
        state = STATE_RESULT;
        lv_obj_set_style_bg_color(panel, UI_COLOR_WARNING, 0);
        lv_label_set_text(msg_label, "Too early! Tap to retry");
        lv_label_set_text(time_label, "");
        break;

    case STATE_GO: {
        /* Measure reaction time */
        uint32_t reaction_ms = (xTaskGetTickCount() - go_tick) * 1000 / configTICK_RATE_HZ;
        state = STATE_RESULT;
        lv_obj_set_style_bg_color(panel, UI_COLOR_PRIMARY, 0);
        lv_label_set_text(msg_label, "Tap to play again");
        lv_label_set_text_fmt(time_label, "%"PRIu32" ms", reaction_ms);
        break;
    }
    }
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* ---- title ---- */
    example_label_create(parent, "Reaction Game",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- tappable panel ---- */
    panel = lv_obj_create(parent);
    lv_obj_set_size(panel, 500, 250);
    lv_obj_set_style_bg_color(panel, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(panel, 20, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(panel, panel_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(panel, 10, 0);

    msg_label = example_label_create(panel, "Tap to start",
                                     &lv_font_montserrat_24,
                                     lv_color_white());
    time_label = example_label_create(panel, "",
                                      &lv_font_montserrat_28,
                                      lv_color_white());

    /* ---- 50 ms timer for delay countdown ---- */
    lv_timer_create(game_timer_cb, 50, NULL);
}
