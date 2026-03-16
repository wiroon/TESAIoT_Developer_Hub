/**
 * @file    main_example.c
 * @brief   Sensor Grid Cards — Multi-board sensor grid with BSP guards
 *
 * Grid of cards showing all available sensors.  BSP_HAS_* flags control
 * which cards are compiled in.  All data via IPC sensorhub snapshot.
 */

#include "example_common.h"

/* ── Sensor card descriptor ──────────────────────────────────────── */
typedef struct {
    const char  *icon;
    const char  *name;
    lv_color_t   color;
    lv_obj_t    *lbl_value;
} sensor_card_t;

#define MAX_CARDS   6
static sensor_card_t s_cards[MAX_CARDS];
static uint32_t      s_card_count;

/* ── Helper: create one sensor card ──────────────────────────────── */
static lv_obj_t *create_sensor_card(lv_obj_t *grid, const char *icon,
                                     const char *name, lv_color_t color,
                                     uint8_t col, uint8_t row)
{
    lv_obj_t *card = lv_obj_create(grid);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, col, 1,
                               LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_bg_color(card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, UI_CARD_RADIUS, 0);
    lv_obj_set_style_border_width(card, UI_CARD_BORDER, 0);
    lv_obj_set_style_border_color(card, color, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Icon */
    lv_obj_t *lbl_icon = example_label_create(card, icon,
        &lv_font_montserrat_24, color);
    lv_obj_align(lbl_icon, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Name */
    lv_obj_t *lbl_name = example_label_create(card, name,
        &lv_font_montserrat_14, lv_color_white());
    lv_obj_align(lbl_name, LV_ALIGN_TOP_RIGHT, 0, 4);

    /* Value */
    lv_obj_t *lbl_val = example_label_create(card, "--",
        &lv_font_montserrat_20, color);
    lv_obj_align(lbl_val, LV_ALIGN_BOTTOM_MID, 0, 0);

    return lbl_val;
}

/* ── Timer — update all sensor values via IPC ────────────────────── */
static void grid_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    uint32_t idx = 0;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    /* BMI270 (always present) */
    if (snap.has_bmi270 && idx < s_card_count) {
        int32_t ax_mg = snap.bmi270.ax * 1000 / 16384;
        lv_label_set_text_fmt(s_cards[idx].lbl_value, "%d mg", (int)ax_mg);
    }
    idx++;

#if BSP_HAS_BMM350
    if (snap.has_bmm350 && idx < s_card_count) {
        int32_t heading = snap.bmm350.heading_x10 / 10;
        lv_label_set_text_fmt(s_cards[idx].lbl_value, "%d deg", (int)heading);
    }
    idx++;
#endif

#if BSP_HAS_DPS368
    if (snap.has_dps368) {
        if (idx < s_card_count) {
            lv_label_set_text_fmt(s_cards[idx].lbl_value, "%d.%d C",
                (int)(snap.dps368.temperature_x100 / 100),
                (int)(abs(snap.dps368.temperature_x100) % 100 / 10));
        }
        idx++;
        if (idx < s_card_count) {
            lv_label_set_text_fmt(s_cards[idx].lbl_value, "%d hPa",
                (int)(snap.dps368.pressure_x100 / 100));
        }
        idx++;
    } else {
        idx += 2;
    }
#endif

#if BSP_HAS_SHT40
    if (snap.has_sht40 && idx < s_card_count) {
        lv_label_set_text_fmt(s_cards[idx].lbl_value, "%d.%d %%",
            (int)(snap.sht40.humidity_x100 / 100),
            (int)(snap.sht40.humidity_x100 % 100 / 10));
    }
    idx++;
#endif

    (void)idx;
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = example_label_create(parent,
        "I16 \xe2\x80\x94 Sensor Grid Cards",
        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Grid container */
    static int32_t col_dsc[] = { 220, 220, 220, LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { 120, 120, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 720, 280);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    /* Build card list based on BSP flags */
    uint32_t n = 0;

    s_cards[n].lbl_value = create_sensor_card(grid, LV_SYMBOL_REFRESH,
        "IMU", UI_COLOR_BMI270, n % 3, n / 3);
    s_cards[n].name = "IMU"; n++;

#if BSP_HAS_BMM350
    s_cards[n].lbl_value = create_sensor_card(grid, LV_SYMBOL_GPS,
        "Compass", UI_COLOR_BMM350, n % 3, n / 3);
    s_cards[n].name = "Compass"; n++;
#endif

#if BSP_HAS_DPS368
    s_cards[n].lbl_value = create_sensor_card(grid, LV_SYMBOL_CHARGE,
        "Temp", UI_COLOR_DPS368, n % 3, n / 3);
    s_cards[n].name = "Temp"; n++;

    s_cards[n].lbl_value = create_sensor_card(grid, LV_SYMBOL_DOWN,
        "Pressure", UI_COLOR_PRIMARY, n % 3, n / 3);
    s_cards[n].name = "Pressure"; n++;
#endif

#if BSP_HAS_SHT40
    s_cards[n].lbl_value = create_sensor_card(grid, LV_SYMBOL_EYE_OPEN,
        "Humidity", UI_COLOR_SHT40, n % 3, n / 3);
    s_cards[n].name = "Humidity"; n++;
#endif

    s_card_count = n;

    lv_timer_create(grid_timer_cb, 500, NULL);
}
