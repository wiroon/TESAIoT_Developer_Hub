/**
 * @file    main_example.c
 * @brief   Camera Settings UI — Mode switches, resolution, flip/mirror toggles
 *
 * @description
 *   Camera settings dashboard with mode selector (Photo/Video/Timelapse),
 *   resolution dropdown, flip/mirror toggle switches, white balance selector,
 *   exposure slider, and a live settings summary display. Pure LVGL UI demo
 *   demonstrating camera configuration patterns for embedded systems.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define NUM_MODES       3
#define NUM_WB_MODES    4

static const char *MODE_NAMES[] = { "Photo", "Video", "Timelapse" };
static const char *MODE_ICONS[] = { LV_SYMBOL_IMAGE, LV_SYMBOL_VIDEO, LV_SYMBOL_LOOP };
static const char *RES_OPTIONS = "320x240\n640x480\n800x600\n1280x720\n1920x1080";
static const char *WB_OPTIONS = "Auto\nDaylight\nCloudy\nTungsten";

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *mode_btns[NUM_MODES];
    lv_obj_t    *mode_labels[NUM_MODES];
    lv_obj_t    *res_dropdown;
    lv_obj_t    *wb_dropdown;
    lv_obj_t    *flip_sw;
    lv_obj_t    *mirror_sw;
    lv_obj_t    *exposure_slider;
    lv_obj_t    *exposure_label;
    lv_obj_t    *summary_label;
    lv_obj_t    *status_label;
    int          active_mode;
} camera_settings_ctx_t;

static camera_settings_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Update settings summary
 * --------------------------------------------------------------------------- */
static void update_summary(camera_settings_ctx_t *ctx)
{
    char res_buf[16];
    lv_dropdown_get_selected_str(ctx->res_dropdown, res_buf, sizeof(res_buf));

    char wb_buf[16];
    lv_dropdown_get_selected_str(ctx->wb_dropdown, wb_buf, sizeof(wb_buf));

    bool flip = lv_obj_has_state(ctx->flip_sw, LV_STATE_CHECKED);
    bool mirror = lv_obj_has_state(ctx->mirror_sw, LV_STATE_CHECKED);
    int32_t exposure = lv_slider_get_value(ctx->exposure_slider);

    char summary[256];
    snprintf(summary, sizeof(summary),
             "Mode: %s\n"
             "Resolution: %s\n"
             "White Balance: %s\n"
             "H-Flip: %s | V-Mirror: %s\n"
             "Exposure: %+ld EV",
             MODE_NAMES[ctx->active_mode],
             res_buf, wb_buf,
             flip ? "ON" : "OFF",
             mirror ? "ON" : "OFF",
             (long)exposure);

    lv_label_set_text(ctx->summary_label, summary);
}

/* ---------------------------------------------------------------------------
 * Mode button callback
 * --------------------------------------------------------------------------- */
static void mode_btn_cb(lv_event_t *e)
{
    camera_settings_ctx_t *ctx = (camera_settings_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);

    /* Find which button was clicked */
    for (int i = 0; i < NUM_MODES; i++) {
        if (ctx->mode_btns[i] == btn) {
            ctx->active_mode = i;

            /* Highlight active, dim others */
            lv_obj_set_style_bg_color(btn, UI_COLOR_PRIMARY, 0);
            lv_obj_set_style_text_color(ctx->mode_labels[i], lv_color_hex(0xFFFFFF), 0);
        } else {
            lv_obj_set_style_bg_color(ctx->mode_btns[i], UI_COLOR_CARD_BG, 0);
            lv_obj_set_style_text_color(ctx->mode_labels[i], UI_COLOR_TEXT_DIM, 0);
        }
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "Mode: %s", MODE_NAMES[ctx->active_mode]);
    lv_label_set_text(ctx->status_label, buf);
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);

    update_summary(ctx);
}

/* ---------------------------------------------------------------------------
 * Settings change callbacks
 * --------------------------------------------------------------------------- */
static void settings_changed_cb(lv_event_t *e)
{
    camera_settings_ctx_t *ctx = (camera_settings_ctx_t *)lv_event_get_user_data(e);
    update_summary(ctx);

    lv_label_set_text(ctx->status_label, "Settings updated");
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
}

static void exposure_changed_cb(lv_event_t *e)
{
    camera_settings_ctx_t *ctx = (camera_settings_ctx_t *)lv_event_get_user_data(e);
    int32_t val = lv_slider_get_value(ctx->exposure_slider);

    char buf[24];
    snprintf(buf, sizeof(buf), "EV: %+ld", (long)val);
    lv_label_set_text(ctx->exposure_label, buf);

    update_summary(ctx);
}

/* ---------------------------------------------------------------------------
 * Apply button callback
 * --------------------------------------------------------------------------- */
static void apply_cb(lv_event_t *e)
{
    camera_settings_ctx_t *ctx = (camera_settings_ctx_t *)lv_event_get_user_data(e);
    lv_label_set_text(ctx->status_label, "Settings applied!");
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
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
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Camera Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Status */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Configure camera settings");
    lv_obj_set_style_text_color(s_ctx.status_label, UI_COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 36);

    /* === Mode selector (top row of 3 buttons) === */
    for (int i = 0; i < NUM_MODES; i++) {
        s_ctx.mode_btns[i] = lv_btn_create(parent);
        lv_obj_set_size(s_ctx.mode_btns[i], 120, 44);
        lv_obj_align(s_ctx.mode_btns[i], LV_ALIGN_TOP_LEFT,
                     16 + i * 130, 60);
        lv_obj_set_style_bg_color(s_ctx.mode_btns[i],
                                   (i == 0) ? UI_COLOR_PRIMARY : UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_radius(s_ctx.mode_btns[i], 8, 0);
        lv_obj_add_event_cb(s_ctx.mode_btns[i], mode_btn_cb,
                            LV_EVENT_CLICKED, &s_ctx);

        char label_buf[32];
        snprintf(label_buf, sizeof(label_buf), "%s %s",
                 MODE_ICONS[i], MODE_NAMES[i]);
        s_ctx.mode_labels[i] = lv_label_create(s_ctx.mode_btns[i]);
        lv_label_set_text(s_ctx.mode_labels[i], label_buf);
        lv_obj_center(s_ctx.mode_labels[i]);
        lv_obj_set_style_text_color(s_ctx.mode_labels[i],
                                     (i == 0) ? lv_color_hex(0xFFFFFF) : UI_COLOR_TEXT_DIM, 0);
    }

    /* === Settings panel (left) === */
    lv_obj_t *settings_card = example_card_create(parent, 260, 210, UI_COLOR_CARD_BG);
    lv_obj_align(settings_card, LV_ALIGN_TOP_LEFT, 8, 114);

    /* Resolution */
    lv_obj_t *res_lbl = example_label_create(settings_card,
        "Resolution", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(res_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.res_dropdown = lv_dropdown_create(settings_card);
    lv_obj_set_width(s_ctx.res_dropdown, 230);
    lv_obj_align(s_ctx.res_dropdown, LV_ALIGN_TOP_LEFT, 0, 18);
    lv_dropdown_set_options(s_ctx.res_dropdown, RES_OPTIONS);
    lv_dropdown_set_selected(s_ctx.res_dropdown, 1);  /* Default 640x480 */
    lv_obj_add_event_cb(s_ctx.res_dropdown, settings_changed_cb,
                        LV_EVENT_VALUE_CHANGED, &s_ctx);

    /* White Balance */
    lv_obj_t *wb_lbl = example_label_create(settings_card,
        "White Balance", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(wb_lbl, LV_ALIGN_TOP_LEFT, 0, 60);

    s_ctx.wb_dropdown = lv_dropdown_create(settings_card);
    lv_obj_set_width(s_ctx.wb_dropdown, 230);
    lv_obj_align(s_ctx.wb_dropdown, LV_ALIGN_TOP_LEFT, 0, 78);
    lv_dropdown_set_options(s_ctx.wb_dropdown, WB_OPTIONS);
    lv_obj_add_event_cb(s_ctx.wb_dropdown, settings_changed_cb,
                        LV_EVENT_VALUE_CHANGED, &s_ctx);

    /* Flip / Mirror toggles */
    lv_obj_t *flip_lbl = example_label_create(settings_card,
        "H-Flip", &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(flip_lbl, LV_ALIGN_TOP_LEFT, 0, 122);

    s_ctx.flip_sw = lv_switch_create(settings_card);
    lv_obj_align(s_ctx.flip_sw, LV_ALIGN_TOP_LEFT, 60, 120);
    lv_obj_add_event_cb(s_ctx.flip_sw, settings_changed_cb,
                        LV_EVENT_VALUE_CHANGED, &s_ctx);

    lv_obj_t *mirror_lbl = example_label_create(settings_card,
        "V-Mirror", &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(mirror_lbl, LV_ALIGN_TOP_LEFT, 120, 122);

    s_ctx.mirror_sw = lv_switch_create(settings_card);
    lv_obj_align(s_ctx.mirror_sw, LV_ALIGN_TOP_LEFT, 190, 120);
    lv_obj_add_event_cb(s_ctx.mirror_sw, settings_changed_cb,
                        LV_EVENT_VALUE_CHANGED, &s_ctx);

    /* Exposure slider */
    lv_obj_t *exp_lbl = example_label_create(settings_card,
        "Exposure", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(exp_lbl, LV_ALIGN_TOP_LEFT, 0, 158);

    s_ctx.exposure_slider = lv_slider_create(settings_card);
    lv_obj_set_width(s_ctx.exposure_slider, 160);
    lv_obj_align(s_ctx.exposure_slider, LV_ALIGN_TOP_LEFT, 0, 178);
    lv_slider_set_range(s_ctx.exposure_slider, -3, 3);
    lv_slider_set_value(s_ctx.exposure_slider, 0, LV_ANIM_OFF);
    lv_obj_add_event_cb(s_ctx.exposure_slider, exposure_changed_cb,
                        LV_EVENT_VALUE_CHANGED, &s_ctx);

    s_ctx.exposure_label = example_label_create(settings_card,
        "EV: 0", &lv_font_montserrat_14, UI_COLOR_WARNING);
    lv_obj_align(s_ctx.exposure_label, LV_ALIGN_TOP_LEFT, 170, 175);

    /* === Summary panel (right) === */
    lv_obj_t *summary_card = example_card_create(parent, 220, 210, UI_COLOR_CARD_BG);
    lv_obj_align(summary_card, LV_ALIGN_TOP_RIGHT, -8, 114);

    lv_obj_t *sum_title = example_label_create(summary_card,
        "Current Settings", &lv_font_montserrat_16, UI_COLOR_PRIMARY);
    lv_obj_align(sum_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.summary_label = lv_label_create(summary_card);
    lv_label_set_text(s_ctx.summary_label, "---");
    lv_obj_set_style_text_color(s_ctx.summary_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_ctx.summary_label, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(s_ctx.summary_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ctx.summary_label, 192);
    lv_obj_align(s_ctx.summary_label, LV_ALIGN_TOP_LEFT, 0, 24);

    /* Apply button */
    lv_obj_t *apply_btn = lv_btn_create(parent);
    lv_obj_set_size(apply_btn, 160, 44);
    lv_obj_align(apply_btn, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(apply_btn, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(apply_btn, 8, 0);
    lv_obj_add_event_cb(apply_btn, apply_cb, LV_EVENT_CLICKED, &s_ctx);

    lv_obj_t *apply_lbl = lv_label_create(apply_btn);
    lv_label_set_text(apply_lbl, LV_SYMBOL_OK " Apply Settings");
    lv_obj_center(apply_lbl);

    /* Initialize summary */
    update_summary(&s_ctx);
}
