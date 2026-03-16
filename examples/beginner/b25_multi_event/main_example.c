/**
 * @file    main_example.c
 * @brief   Multi Event Logger — LV_EVENT_ALL with switch-case logging
 *
 * Captures all events on a button and logs them in a scrollable label.
 * Shows the full event lifecycle during touch interactions.
 */

#include "example_common.h"

#define LOG_BUF_SIZE 512

static char log_buf[LOG_BUF_SIZE];
static int  log_offset = 0;
static lv_obj_t *lbl_log;
static int event_seq = 0;

static const char *event_name(lv_event_code_t code)
{
    switch (code) {
    case LV_EVENT_PRESSED:              return "PRESSED";
    case LV_EVENT_PRESSING:             return "PRESSING";
    case LV_EVENT_RELEASED:             return "RELEASED";
    case LV_EVENT_CLICKED:              return "CLICKED";
    case LV_EVENT_LONG_PRESSED:         return "LONG_PRESSED";
    case LV_EVENT_LONG_PRESSED_REPEAT:  return "LONG_PRESS_RPT";
    case LV_EVENT_PRESS_LOST:           return "PRESS_LOST";
    case LV_EVENT_FOCUSED:              return "FOCUSED";
    case LV_EVENT_DEFOCUSED:            return "DEFOCUSED";
    default:                            return NULL;
    }
}

static void all_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const char *name = event_name(code);
    if (name == NULL) return;  /* Skip uninteresting events */

    event_seq++;
    int written = snprintf(log_buf + log_offset, LOG_BUF_SIZE - log_offset,
                           "%d: %s\n", event_seq, name);
    if (written > 0 && log_offset + written < LOG_BUF_SIZE) {
        log_offset += written;
    } else {
        /* Buffer full — reset */
        log_offset = 0;
        log_offset = snprintf(log_buf, LOG_BUF_SIZE, "--- Log reset ---\n%d: %s\n",
                              event_seq, name);
    }
    lv_label_set_text(lbl_log, log_buf);
}

static void clear_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        log_offset = 0;
        event_seq = 0;
        log_buf[0] = '\0';
        lv_label_set_text(lbl_log, "(empty)");
    }
}

void example_main(lv_obj_t *parent)
{
    log_offset = 0;
    event_seq = 0;
    log_buf[0] = '\0';

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Event Logger");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Test button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 180, 60);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 30, 40);
    lv_obj_add_event_cb(btn, all_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Touch Me");
    lv_obj_center(btn_lbl);

    /* Clear button */
    lv_obj_t *btn_clear = lv_btn_create(parent);
    lv_obj_set_size(btn_clear, 120, 40);
    lv_obj_align(btn_clear, LV_ALIGN_TOP_LEFT, 50, 110);
    lv_obj_set_style_bg_color(btn_clear, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_clear, clear_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clr_lbl = lv_label_create(btn_clear);
    lv_label_set_text(clr_lbl, "Clear");
    lv_obj_center(clr_lbl);

    /* Scrollable log panel */
    lv_obj_t *log_panel = lv_obj_create(parent);
    lv_obj_set_size(log_panel, 450, 300);
    lv_obj_align(log_panel, LV_ALIGN_RIGHT_MID, -20, 15);
    lv_obj_set_style_radius(log_panel, 8, 0);
    lv_obj_set_style_pad_all(log_panel, 10, 0);

    lbl_log = lv_label_create(log_panel);
    lv_label_set_text(lbl_log, "(empty)");
    lv_obj_set_style_text_font(lbl_log, &lv_font_montserrat_14, 0);
    lv_obj_set_width(lbl_log, 420);
    lv_label_set_long_mode(lbl_log, LV_LABEL_LONG_WRAP);
}
