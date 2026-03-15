/**
 * @file    main_example.c
 * @brief   Button Counter — Click counter with event callback
 *
 * A button increments a counter on each click. The count is displayed
 * in a label below the button using lv_label_set_text_fmt.
 */

#include "example_common.h"

static int32_t click_count = 0;

static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
        click_count++;
        lv_label_set_text_fmt(label, "Count: %"PRId32, click_count);
    }
}

void example_main(lv_obj_t *parent)
{
    /* Counter display label */
    lv_obj_t *lbl_count = lv_label_create(parent);
    lv_label_set_text(lbl_count, "Count: 0");
    lv_obj_set_style_text_font(lbl_count, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_count, LV_ALIGN_CENTER, 0, 40);

    /* Click button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 160, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -30);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, lbl_count);

    lv_obj_t *lbl_btn = lv_label_create(btn);
    lv_label_set_text(lbl_btn, "Click Me!");
    lv_obj_center(lbl_btn);

    click_count = 0;
}
