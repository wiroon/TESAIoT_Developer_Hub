/**
 * @file    main_example.c
 * @brief   Grid Layout — 2x2 grid with LED+Switch+Label per cell
 *
 * A 2x2 grid layout where each cell contains a virtual LED,
 * a switch to toggle it, and a channel name label.
 */

#include "example_common.h"

static const char *channel_names[] = { "CH 1", "CH 2", "CH 3", "CH 4" };
static const lv_palette_t led_colors[] = {
    LV_PALETTE_RED, LV_PALETTE_GREEN, LV_PALETTE_BLUE, LV_PALETTE_ORANGE
};

static void cell_switch_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *sw = lv_event_get_target(e);
        lv_obj_t *led = (lv_obj_t *)lv_event_get_user_data(e);
        if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
            lv_led_on(led);
        } else {
            lv_led_off(led);
        }
    }
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "4-Channel Control Panel");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Grid container */
    static int32_t col_dsc[] = { 340, 340, LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { 150, 150, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 720, 340);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid, 10, 0);
    lv_obj_set_style_pad_column(grid, 10, 0);
    lv_obj_set_style_pad_row(grid, 10, 0);

    for (int i = 0; i < 4; i++) {
        int col = i % 2;
        int row = i / 2;

        /* Cell container */
        lv_obj_t *cell = lv_obj_create(grid);
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, col, 1,
                             LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(cell, 8, 0);
        lv_obj_set_style_radius(cell, 12, 0);

        /* Channel label */
        lv_obj_t *lbl = lv_label_create(cell);
        lv_label_set_text(lbl, channel_names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);

        /* LED */
        lv_obj_t *led = lv_led_create(cell);
        lv_obj_set_size(led, 40, 40);
        lv_led_set_color(led, lv_palette_main(led_colors[i]));
        lv_led_off(led);

        /* Switch */
        lv_obj_t *sw = lv_switch_create(cell);
        lv_obj_set_size(sw, 60, 30);
        lv_obj_add_event_cb(sw, cell_switch_cb, LV_EVENT_VALUE_CHANGED, led);
    }
}
