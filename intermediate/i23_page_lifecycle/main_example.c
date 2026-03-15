/**
 * @file    main_example.c
 * @brief   Page Lifecycle — create / render / destroy with pm_register
 *
 * Demonstrates the complete page_def_t lifecycle callbacks.
 * Logs each event to a visible label with tick timestamps.
 */

#include "example_common.h"
#include "sensorhub_ui.h"

/* ── Page state ──────────────────────────────────────────────────── */
static lv_obj_t *s_lbl_log;
static lv_obj_t *s_lbl_frames;
static uint32_t  s_frame_count;
static lv_timer_t *s_render_timer;

/* ── Create callback — build widget tree ─────────────────────────── */
static void my_page_create(lv_obj_t *parent)
{
    s_frame_count = 0;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I23 — Page Lifecycle Demo");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Log area */
    s_lbl_log = lv_label_create(parent);
    lv_label_set_text_fmt(s_lbl_log,
        "Lifecycle events:\n"
        "  [%u] CREATE called\n"
        "  [    ] render...\n"
        "  [    ] destroy pending",
        (unsigned)xTaskGetTickCount());
    lv_obj_set_style_text_font(s_lbl_log, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_log, lv_color_white(), 0);
    lv_obj_align(s_lbl_log, LV_ALIGN_CENTER, 0, -20);

    /* Frame counter */
    s_lbl_frames = lv_label_create(parent);
    lv_label_set_text(s_lbl_frames, "Render frames: 0");
    lv_obj_set_style_text_font(s_lbl_frames, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_frames,
                                lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(s_lbl_frames, LV_ALIGN_CENTER, 0, 50);

    /* Explanation */
    lv_obj_t *note = lv_label_create(parent);
    lv_label_set_text(note,
        "create()  -> build widgets (once)\n"
        "render()  -> update data (each tick)\n"
        "destroy() -> cleanup (on navigate away)");
    lv_obj_set_style_text_font(note, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(note, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/* ── Render callback — update dynamic data ───────────────────────── */
static void my_page_render(lv_obj_t *parent)
{
    (void)parent;
    s_frame_count++;
    lv_label_set_text_fmt(s_lbl_frames, "Render frames: %u",
                          (unsigned)s_frame_count);
}

/* ── Destroy callback — cleanup resources ────────────────────────── */
static void my_page_destroy(void)
{
    /* In a real page, stop timers and free memory here */
    s_lbl_log    = NULL;
    s_lbl_frames = NULL;
}

/* ── Page definition ─────────────────────────────────────────────── */
static const page_def_t s_my_page_def = {
    .create  = my_page_create,
    .render  = my_page_render,
    .destroy = my_page_destroy,
};

/* ── Entry point — register the page ─────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /*
     * In production code, pm_register is called at init time,
     * and the page manager controls create/render/destroy.
     *
     * For this demo, we directly call create + simulate render
     * via a timer so the example is self-contained.
     */
    my_page_create(parent);

    /* Simulate render loop via LVGL timer */
    s_render_timer = lv_timer_create(
        (lv_timer_cb_t)my_page_render, 200, parent);

    /*
     * Production registration (done once at boot):
     * pm_register(&s_pm, PAGE_ID_MY_PAGE, &s_my_page_def);
     */
    (void)s_my_page_def;  /* suppress unused warning in demo mode */
}
