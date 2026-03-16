/**
 * @file    main_example.c
 * @brief   Sensor Grid Cards — Multi-board sensor grid with BSP guards
 *
 * Grid of cards showing all available sensors.  BSP_HAS_* flags control
 * which cards are compiled in.
 */

#include "example_common.h"
#include "sensor_bmi270.h"

#if BSP_HAS_DPS368
#include "sensor_dps368.h"
#endif
#if BSP_HAS_SHT40
#include "sensor_sht40.h"
#endif
#if BSP_HAS_BMM350
#include "sensor_bmm350.h"
#endif

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
    lv_obj_set_style_bg_color(card, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, color, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Icon */
    lv_obj_t *lbl_icon = lv_label_create(card);
    lv_label_set_text(lbl_icon, icon);
    lv_obj_set_style_text_font(lbl_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_icon, color, 0);
    lv_obj_align(lbl_icon, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Name */
    lv_obj_t *lbl_name = lv_label_create(card);
    lv_label_set_text(lbl_name, name);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_name, lv_color_white(), 0);
    lv_obj_align(lbl_name, LV_ALIGN_TOP_RIGHT, 0, 4);

    /* Value */
    lv_obj_t *lbl_val = lv_label_create(card);
    lv_label_set_text(lbl_val, "--");
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_val, color, 0);
    lv_obj_align(lbl_val, LV_ALIGN_BOTTOM_MID, 0, 0);

    return lbl_val;
}

/* ── Timer — update all sensor values ────────────────────────────── */
static void grid_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    uint32_t idx = 0;

    /* BMI270 (always present) */
    {
        sensor_bmi270_data_t d;
        if (sensor_bmi270_read(&d) == 0 && idx < s_card_count) {
            lv_label_set_text_fmt(s_cards[idx].lbl_value,
                                  "%.0f mg", d.accel_x * 1000.0f);
        }
        idx++;
    }

#if BSP_HAS_BMM350
    {
        sensor_bmm350_data_t d;
        if (sensor_bmm350_read(&d) == 0 && idx < s_card_count) {
            lv_label_set_text_fmt(s_cards[idx].lbl_value,
                                  "%.1f uT", d.mag_x);
        }
        idx++;
    }
#endif

#if BSP_HAS_DPS368
    {
        sensor_dps368_data_t d;
        if (sensor_dps368_read(&d) == 0) {
            if (idx < s_card_count) {
                lv_label_set_text_fmt(s_cards[idx].lbl_value,
                                      "%.1f C", d.temperature);
            }
            idx++;
            if (idx < s_card_count) {
                lv_label_set_text_fmt(s_cards[idx].lbl_value,
                                      "%.0f hPa", d.pressure);
            }
            idx++;
        } else {
            idx += 2;
        }
    }
#endif

#if BSP_HAS_SHT40
    {
        sensor_sht40_data_t d;
        if (sensor_sht40_read(&d) == 0 && idx < s_card_count) {
            lv_label_set_text_fmt(s_cards[idx].lbl_value,
                                  "%.1f %%", d.humidity);
        }
        idx++;
    }
#endif

    (void)idx;
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I16 — Sensor Grid Cards");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
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
        "IMU", lv_color_hex(0x4CAF50), n % 3, n / 3);
    s_cards[n].name = "IMU"; n++;

#if BSP_HAS_BMM350
    s_cards[n].lbl_value = create_sensor_card(grid, LV_SYMBOL_GPS,
        "Compass", lv_color_hex(0xE040FB), n % 3, n / 3);
    s_cards[n].name = "Compass"; n++;
#endif

#if BSP_HAS_DPS368
    s_cards[n].lbl_value = create_sensor_card(grid, LV_SYMBOL_CHARGE,
        "Temp", lv_color_hex(0xFF9800), n % 3, n / 3);
    s_cards[n].name = "Temp"; n++;

    s_cards[n].lbl_value = create_sensor_card(grid, LV_SYMBOL_DOWN,
        "Pressure", lv_palette_main(LV_PALETTE_CYAN), n % 3, n / 3);
    s_cards[n].name = "Pressure"; n++;
#endif

#if BSP_HAS_SHT40
    s_cards[n].lbl_value = create_sensor_card(grid, LV_SYMBOL_EYE_OPEN,
        "Humidity", lv_color_hex(0x2196F3), n % 3, n / 3);
    s_cards[n].name = "Humidity"; n++;
#endif

    s_card_count = n;

    /* Init sensors */
    sensor_bmi270_init();
#if BSP_HAS_BMM350
    sensor_bmm350_init();
#endif
#if BSP_HAS_DPS368
    sensor_dps368_init();
#endif
#if BSP_HAS_SHT40
    sensor_sht40_init();
#endif

    lv_timer_create(grid_timer_cb, 500, NULL);
}
