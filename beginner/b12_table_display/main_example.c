/**
 * @file    main_example.c
 * @brief   Table Display — 4x3 sensor data table with header row
 *
 * A table widget displays sensor names, values, and units.
 * The header row has a distinct background color for visual separation.
 */

#include "example_common.h"

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Sensor Readings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Table */
    lv_obj_t *table = lv_table_create(parent);
    lv_obj_set_style_text_font(table, &lv_font_montserrat_16, 0);
    lv_obj_align(table, LV_ALIGN_CENTER, 0, 10);

    lv_table_set_column_count(table, 3);
    lv_table_set_row_count(table, 4);

    /* Set column widths */
    lv_table_set_column_width(table, 0, 180);
    lv_table_set_column_width(table, 1, 120);
    lv_table_set_column_width(table, 2, 80);

    /* Header row */
    lv_table_set_cell_value(table, 0, 0, "Sensor");
    lv_table_set_cell_value(table, 0, 1, "Value");
    lv_table_set_cell_value(table, 0, 2, "Unit");

    /* Data rows */
    lv_table_set_cell_value(table, 1, 0, "Temperature");
    lv_table_set_cell_value(table, 1, 1, "25.3");
    lv_table_set_cell_value(table, 1, 2, "\xC2\xB0""C");

    lv_table_set_cell_value(table, 2, 0, "Pressure");
    lv_table_set_cell_value(table, 2, 1, "1013.25");
    lv_table_set_cell_value(table, 2, 2, "hPa");

    lv_table_set_cell_value(table, 3, 0, "Humidity");
    lv_table_set_cell_value(table, 3, 1, "45.7");
    lv_table_set_cell_value(table, 3, 2, "%");

    /* Style header row with different background */
    lv_obj_set_style_bg_color(table, lv_palette_main(LV_PALETTE_BLUE), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(table, LV_OPA_20, LV_PART_ITEMS | LV_STATE_DEFAULT);
}
