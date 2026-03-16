/**
 * @file    main_example.c
 * @brief   Timer Uptime Counter — HH:MM:SS display updated every second
 *
 * An LVGL timer fires every 1000ms to update an uptime counter.
 * Displays hours, minutes, seconds, and total elapsed seconds.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *lbl_time;
    lv_obj_t *lbl_seconds;
    uint32_t  total_seconds;
} uptime_ctx_t;

static uptime_ctx_t ctx;

static void uptime_timer_cb(lv_timer_t *timer)
{
    uptime_ctx_t *c = (uptime_ctx_t *)lv_timer_get_user_data(timer);

    c->total_seconds++;
    uint32_t hrs = c->total_seconds / 3600;
    uint32_t mins = (c->total_seconds % 3600) / 60;
    uint32_t secs = c->total_seconds % 60;

    lv_label_set_text_fmt(c->lbl_time, "%02"PRIu32":%02"PRIu32":%02"PRIu32,
                          hrs, mins, secs);
    lv_label_set_text_fmt(c->lbl_seconds, "Total: %"PRIu32" seconds",
                          c->total_seconds);
}

void example_main(lv_obj_t *parent)
{
    ctx.total_seconds = 0;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "System Uptime");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    /* Time display — large font */
    ctx.lbl_time = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_time, "00:00:00");
    lv_obj_set_style_text_font(ctx.lbl_time, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(ctx.lbl_time, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(ctx.lbl_time, LV_ALIGN_CENTER, 0, -20);

    /* Total seconds label */
    ctx.lbl_seconds = lv_label_create(parent);
    lv_label_set_text(ctx.lbl_seconds, "Total: 0 seconds");
    lv_obj_set_style_text_font(ctx.lbl_seconds, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ctx.lbl_seconds, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(ctx.lbl_seconds, LV_ALIGN_CENTER, 0, 30);

    /* Timer icon */
    lv_obj_t *icon = lv_label_create(parent);
    lv_label_set_text(icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(icon, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -70);

    /* 1-second timer */
    lv_timer_create(uptime_timer_cb, 1000, &ctx);
}
