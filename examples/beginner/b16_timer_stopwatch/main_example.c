/**
 * @file main_example.c
 * @brief B16 — Timer Stopwatch: Start/Stop/Reset stopwatch with elapsed time.
 *
 * Demonstrates a simple stopwatch using an LVGL timer. Three buttons
 * control start, stop, and reset. Elapsed time is displayed in MM:SS.mmm format.
 */

#include "example_common.h"

static lv_obj_t *time_label;
static lv_obj_t *status_label;
static lv_timer_t *sw_timer = NULL;
static uint32_t elapsed_ms = 0;
static bool     running    = false;

static void update_display(void)
{
    uint32_t mins = elapsed_ms / 60000;
    uint32_t secs = (elapsed_ms % 60000) / 1000;
    uint32_t ms   = elapsed_ms % 1000;

    lv_label_set_text_fmt(time_label, "%02"PRIu32":%02"PRIu32".%03"PRIu32,
                          mins, secs, ms);
}

static void stopwatch_tick(lv_timer_t *t)
{
    (void)t;
    if (!running) return;
    elapsed_ms += 50;   /* timer fires every 50 ms */
    update_display();
}

static void start_cb(lv_event_t *e)
{
    (void)e;
    running = true;
    lv_label_set_text(status_label, "RUNNING");
    lv_obj_set_style_text_color(status_label, UI_COLOR_SUCCESS, 0);
}

static void stop_cb(lv_event_t *e)
{
    (void)e;
    running = false;
    lv_label_set_text(status_label, "STOPPED");
    lv_obj_set_style_text_color(status_label, UI_COLOR_WARNING, 0);
}

static void reset_cb(lv_event_t *e)
{
    (void)e;
    running    = false;
    elapsed_ms = 0;
    update_display();
    lv_label_set_text(status_label, "RESET");
    lv_obj_set_style_text_color(status_label, UI_COLOR_TEXT, 0);
}

static lv_obj_t *make_btn(lv_obj_t *parent, const char *text,
                           lv_color_t color, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 150, 55);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl);
    return btn;
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 20, 0);

    /* ---- title ---- */
    example_label_create(parent, "Stopwatch",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- elapsed time ---- */
    time_label = example_label_create(parent, "00:00.000",
                                      &lv_font_montserrat_28, UI_COLOR_PRIMARY);

    /* ---- status ---- */
    status_label = example_label_create(parent, "READY",
                                        &lv_font_montserrat_20, UI_COLOR_TEXT);

    /* ---- button row ---- */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 20, 0);

    make_btn(row, "Start", UI_COLOR_SUCCESS, start_cb);
    make_btn(row, "Stop",  UI_COLOR_WARNING, stop_cb);
    make_btn(row, "Reset", UI_COLOR_ERROR,   reset_cb);

    /* ---- 50 ms timer for ~20 Hz update ---- */
    sw_timer = lv_timer_create(stopwatch_tick, 50, NULL);
}
