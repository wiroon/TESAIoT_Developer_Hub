/**
 * @file    main_example.c
 * @brief   Checkbox Options — Four checkboxes with dynamic summary label
 *
 * Four sensor option checkboxes. A summary label updates to show
 * which options are currently selected.
 */

#include "example_common.h"

#define NUM_OPTIONS 4

static const char *option_names[NUM_OPTIONS] = {
    "Accelerometer", "Gyroscope", "Temperature", "Pressure"
};

typedef struct {
    lv_obj_t *checkboxes[NUM_OPTIONS];
    lv_obj_t *summary;
} cb_ctx_t;

static cb_ctx_t ctx;

static void update_summary(cb_ctx_t *c)
{
    static char buf[128];
    int offset = 0;
    int count = 0;

    offset += snprintf(buf + offset, sizeof(buf) - offset, "Selected: ");

    for (int i = 0; i < NUM_OPTIONS; i++) {
        if (lv_obj_has_state(c->checkboxes[i], LV_STATE_CHECKED)) {
            if (count > 0) {
                offset += snprintf(buf + offset, sizeof(buf) - offset, ", ");
            }
            offset += snprintf(buf + offset, sizeof(buf) - offset, "%s", option_names[i]);
            count++;
        }
    }

    if (count == 0) {
        snprintf(buf + offset, sizeof(buf) - offset, "None");
    }

    lv_label_set_text(c->summary, buf);
}

static void checkbox_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        cb_ctx_t *c = (cb_ctx_t *)lv_event_get_user_data(e);
        update_summary(c);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Select Sensors:");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Checkbox container with flex column */
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 300, 200);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(cont, 12, 0);

    for (int i = 0; i < NUM_OPTIONS; i++) {
        ctx.checkboxes[i] = lv_checkbox_create(cont);
        lv_checkbox_set_text(ctx.checkboxes[i], option_names[i]);
        lv_obj_add_event_cb(ctx.checkboxes[i], checkbox_event_cb, LV_EVENT_VALUE_CHANGED, &ctx);
    }

    /* Summary label */
    ctx.summary = lv_label_create(parent);
    lv_label_set_text(ctx.summary, "Selected: None");
    lv_obj_set_style_text_font(ctx.summary, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx.summary, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(ctx.summary, LV_ALIGN_BOTTOM_MID, 0, -15);
}
