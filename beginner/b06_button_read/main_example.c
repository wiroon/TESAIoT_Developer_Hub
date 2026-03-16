/**
 * @file    main_example.c
 * @brief   Button Read — Two on-screen buttons with press counters
 *
 * Two LVGL buttons simulate SW1 and SW2 input. Each tracks
 * a press counter and displays PRESSED/Released state.
 */

#include "example_common.h"

typedef struct {
    lv_obj_t *lbl_sw1;
    lv_obj_t *lbl_sw2;
    lv_obj_t *lbl_count1;
    lv_obj_t *lbl_count2;
    int32_t   count_sw1;
    int32_t   count_sw2;
} btn_ctx_t;

static btn_ctx_t ctx;

static void sw1_press_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_PRESSED) return;
    lv_label_set_text(ctx.lbl_sw1, "PRESSED");
    lv_obj_set_style_text_color(ctx.lbl_sw1, UI_COLOR_SUCCESS, 0);
    ctx.count_sw1++;
    lv_label_set_text_fmt(ctx.lbl_count1, "Presses: %"PRId32, ctx.count_sw1);
}

static void sw1_release_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    lv_label_set_text(ctx.lbl_sw1, "Released");
    lv_obj_set_style_text_color(ctx.lbl_sw1, UI_COLOR_TEXT_DIM, 0);
}

static void sw2_press_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_PRESSED) return;
    lv_label_set_text(ctx.lbl_sw2, "PRESSED");
    lv_obj_set_style_text_color(ctx.lbl_sw2, UI_COLOR_SUCCESS, 0);
    ctx.count_sw2++;
    lv_label_set_text_fmt(ctx.lbl_count2, "Presses: %"PRId32, ctx.count_sw2);
}

static void sw2_release_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    lv_label_set_text(ctx.lbl_sw2, "Released");
    lv_obj_set_style_text_color(ctx.lbl_sw2, UI_COLOR_TEXT_DIM, 0);
}

void example_main(lv_obj_t *parent)
{
    ctx.count_sw1 = 0;
    ctx.count_sw2 = 0;

    /* Title */
    lv_obj_t *title = example_label_create(parent, "Button Input",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    /* อ่านค่าปุ่มกด */
    example_label_create(parent,
        "อ่านค่าปุ่มกด",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* SW1 column */
    lv_obj_t *btn_sw1 = lv_btn_create(parent);
    lv_obj_set_size(btn_sw1, 160, 60);
    lv_obj_align(btn_sw1, LV_ALIGN_CENTER, -100, -60);
    lv_obj_add_event_cb(btn_sw1, sw1_press_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn_sw1, sw1_release_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *lbl_sw1_name = lv_label_create(btn_sw1);
    lv_label_set_text(lbl_sw1_name, "SW1");
    lv_obj_center(lbl_sw1_name);

    ctx.lbl_sw1 = example_label_create(parent, "Released",
                                        &lv_font_montserrat_24, UI_COLOR_TEXT_DIM);
    lv_obj_align(ctx.lbl_sw1, LV_ALIGN_CENTER, -100, -10);

    ctx.lbl_count1 = example_label_create(parent, "Presses: 0",
                                           &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(ctx.lbl_count1, LV_ALIGN_CENTER, -100, 20);

    /* SW2 column */
    lv_obj_t *btn_sw2 = lv_btn_create(parent);
    lv_obj_set_size(btn_sw2, 160, 60);
    lv_obj_align(btn_sw2, LV_ALIGN_CENTER, 100, -60);
    lv_obj_add_event_cb(btn_sw2, sw2_press_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn_sw2, sw2_release_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *lbl_sw2_name = lv_label_create(btn_sw2);
    lv_label_set_text(lbl_sw2_name, "SW2");
    lv_obj_center(lbl_sw2_name);

    ctx.lbl_sw2 = example_label_create(parent, "Released",
                                        &lv_font_montserrat_24, UI_COLOR_TEXT_DIM);
    lv_obj_align(ctx.lbl_sw2, LV_ALIGN_CENTER, 100, -10);

    ctx.lbl_count2 = example_label_create(parent, "Presses: 0",
                                           &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(ctx.lbl_count2, LV_ALIGN_CENTER, 100, 20);
}
