/**
 * @file    main_example.c
 * @brief   Spinner Loading — Animated spinner with simulated progress
 *
 * A spinner widget with a timer-driven progress simulation that cycles
 * through loading states and updates the status label.
 *
 * Functions:
 *   create_spinner_widget()  — Build and style the spinner with custom arc
 *   update_loading_status()  — Update status label text and color by state
 *   loading_timer_cb()       — Timer callback: advance progress 0→100→reset
 *   example_main()           — Entry point: compose spinner + progress UI
 */

#include "example_common.h"

static lv_obj_t *s_status_label;
static lv_obj_t *s_pct_label;
static int       s_progress;

/* ── Loading state definitions ────────────────────────────────────── */
typedef struct {
    int          threshold;
    const char  *text;
    lv_palette_t color;
} loading_state_t;

static const loading_state_t s_states[] = {
    {  25, "Initializing...",  LV_PALETTE_GREY },
    {  50, "Loading sensors...", LV_PALETTE_BLUE },
    {  75, "Calibrating...",   LV_PALETTE_ORANGE },
    { 100, "Ready!",           LV_PALETTE_GREEN },
};
#define STATE_COUNT (sizeof(s_states) / sizeof(s_states[0]))

/* ── Create and style the spinner widget ──────────────────────────── */
static lv_obj_t *create_spinner_widget(lv_obj_t *parent)
{
    lv_obj_t *spinner = lv_spinner_create(parent);
    lv_obj_set_size(spinner, 120, 120);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -40);
    lv_spinner_set_anim_params(spinner, 1000, 200);

    lv_obj_set_style_arc_color(spinner, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_palette_lighten(LV_PALETTE_BLUE, 3), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_MAIN);

    return spinner;
}

/* ── Update status label based on progress ────────────────────────── */
static void update_loading_status(int progress)
{
    for (int i = STATE_COUNT - 1; i >= 0; i--) {
        if (progress >= s_states[i].threshold || i == 0) {
            lv_label_set_text(s_status_label, s_states[i].text);
            lv_obj_set_style_text_color(s_status_label,
                lv_palette_main(s_states[i].color), 0);
            break;
        }
    }
    lv_label_set_text_fmt(s_pct_label, "%d%%", progress);
}

/* ── Timer callback: advance progress ─────────────────────────────── */
static void loading_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_progress += 2;
    if (s_progress > 100) s_progress = 0;
    update_loading_status(s_progress);
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_progress = 0;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Spinner Loading Demo");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Spinner */
    create_spinner_widget(parent);

    /* Percentage label */
    s_pct_label = lv_label_create(parent);
    lv_label_set_text(s_pct_label, "0%");
    lv_obj_set_style_text_font(s_pct_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_pct_label, lv_color_white(), 0);
    lv_obj_align(s_pct_label, LV_ALIGN_CENTER, 0, -40);

    /* Status label */
    s_status_label = lv_label_create(parent);
    lv_label_set_text(s_status_label, "Initializing...");
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_status_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_status_label, LV_ALIGN_CENTER, 0, 40);

    /* Timer — update every 200ms */
    lv_timer_create(loading_timer_cb, 200, NULL);
}
