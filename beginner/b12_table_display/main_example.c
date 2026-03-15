/**
 * @file    main_example.c
 * @brief   Table Display — Sensor data table with timer-driven updates
 *
 * A table widget with header styling, data population helper, and
 * a timer that periodically refreshes sensor values with random data.
 *
 * Functions:
 *   setup_table_columns()    — Configure column count and widths
 *   populate_sensor_data()   — Fill table rows with sensor readings
 *   table_update_timer_cb()  — Timer callback: refresh values periodically
 *   example_main()           — Entry point: compose table with live updates
 */

#include "example_common.h"

typedef struct {
    const char *name;
    const char *unit;
    int         min_val_x10;
    int         max_val_x10;
} sensor_row_t;

static const sensor_row_t s_sensors[] = {
    { "Temperature", "\xC2\xB0""C", 200, 350 },
    { "Pressure",    "hPa",         9800, 10500 },
    { "Humidity",    "%",           300, 800 },
};
#define SENSOR_COUNT (sizeof(s_sensors) / sizeof(s_sensors[0]))

static lv_obj_t *s_table;

/* ── Configure table columns ──────────────────────────────────────── */
static void setup_table_columns(lv_obj_t *table)
{
    lv_table_set_column_count(table, 3);
    lv_table_set_row_count(table, SENSOR_COUNT + 1);
    lv_table_set_column_width(table, 0, 180);
    lv_table_set_column_width(table, 1, 120);
    lv_table_set_column_width(table, 2, 80);

    /* Header row */
    lv_table_set_cell_value(table, 0, 0, "Sensor");
    lv_table_set_cell_value(table, 0, 1, "Value");
    lv_table_set_cell_value(table, 0, 2, "Unit");
}

/* ── Populate table with sensor data ──────────────────────────────── */
static void populate_sensor_data(lv_obj_t *table)
{
    for (size_t i = 0; i < SENSOR_COUNT; i++) {
        int val = lv_rand(s_sensors[i].min_val_x10, s_sensors[i].max_val_x10);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d.%d", val / 10, val % 10);

        lv_table_set_cell_value(table, i + 1, 0, s_sensors[i].name);
        lv_table_set_cell_value(table, i + 1, 1, buf);
        lv_table_set_cell_value(table, i + 1, 2, s_sensors[i].unit);
    }
}

/* ── Timer callback: refresh sensor values ────────────────────────── */
static void table_update_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    populate_sensor_data(s_table);
}

/* ── Entry point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Sensor Readings (Live)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Table */
    s_table = lv_table_create(parent);
    lv_obj_set_style_text_font(s_table, &lv_font_montserrat_16, 0);
    lv_obj_align(s_table, LV_ALIGN_CENTER, 0, 10);

    setup_table_columns(s_table);
    populate_sensor_data(s_table);

    /* Style header row */
    lv_obj_set_style_bg_color(s_table, lv_palette_main(LV_PALETTE_BLUE),
                              LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_table, LV_OPA_20,
                            LV_PART_ITEMS | LV_STATE_DEFAULT);

    /* Update every 2 seconds */
    lv_timer_create(table_update_timer_cb, 2000, NULL);
}
