/**
 * @file    main_example.c
 * @brief   Page Lifecycle — create / render / destroy pattern demo
 *
 * Demonstrates the complete page lifecycle using pure LVGL tab/page
 * switching.  Logs each event to a visible label with tick timestamps.
 */

#include "example_common.h"

/* ── Page state ──────────────────────────────────────────────────── */
static lv_obj_t *s_lbl_log;
static lv_obj_t *s_lbl_frames;
static uint32_t  s_frame_count;

/* ── Create callback — build widget tree ─────────────────────────── */
static void my_page_create(lv_obj_t *parent)
{
    s_frame_count = 0;

    lv_obj_t *title = example_label_create(parent,
        "I23 \xe2\x80\x94 Page Lifecycle Demo",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Log area */
    s_lbl_log = example_label_create(parent, "",
        &lv_font_montserrat_16, lv_color_white());
    lv_label_set_text_fmt(s_lbl_log,
        "Lifecycle events:\n"
        "  [%u] CREATE called\n"
        "  [    ] render...\n"
        "  [    ] destroy pending",
        (unsigned)xTaskGetTickCount());
    lv_obj_align(s_lbl_log, LV_ALIGN_CENTER, 0, -20);

    /* Frame counter */
    s_lbl_frames = example_label_create(parent, "Render frames: 0",
        &lv_font_montserrat_16, UI_COLOR_SUCCESS);
    lv_obj_align(s_lbl_frames, LV_ALIGN_CENTER, 0, 50);

    /* Explanation */
    lv_obj_t *note = example_label_create(parent,
        "create()  -> build widgets (once)\n"
        "render()  -> update data (each tick)\n"
        "destroy() -> cleanup (on navigate away)",
        &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/* ── Render callback — update dynamic data ───────────────────────── */
static void my_page_render(lv_timer_t *timer)
{
    (void)timer;
    s_frame_count++;
    if (s_lbl_frames != NULL) {
        lv_label_set_text_fmt(s_lbl_frames, "Render frames: %u",
                              (unsigned)s_frame_count);
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Build the page */
    my_page_create(parent);

    /* Simulate render loop via LVGL timer */
    lv_timer_create(my_page_render, 200, NULL);
}
