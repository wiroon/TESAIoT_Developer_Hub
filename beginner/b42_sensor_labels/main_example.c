/**
 * @file    main_example.c
 * @brief   All Sensors Dashboard — BSP-guarded multi-sensor display
 *
 * Displays all available sensor readings based on BSP feature flags.
 * Adapts automatically to AI Kit or Eva Kit hardware.
 */

#include "example_common.h"
#include "sensor_bmi270.h"

#if BSP_HAS_DPS368
#include "sensor_dps368.h"
#endif

#if BSP_HAS_SHT40
#include "sensor_sht40.h"
#endif

#if BSP_HAS_CAPSENSE
#include "sensor_capsense.h"
#endif

#if BSP_HAS_POTENTIOMETER
#include "sensor_potentiometer.h"
#endif

typedef struct {
    /* BMI270 — always available */
    lv_obj_t *lbl_accel;
    bool      bmi270_ok;

#if BSP_HAS_DPS368
    lv_obj_t *lbl_temp;
    lv_obj_t *lbl_press;
    bool      dps368_ok;
#endif

#if BSP_HAS_SHT40
    lv_obj_t *lbl_humidity;
    bool      sht40_ok;
#endif

#if BSP_HAS_CAPSENSE
    lv_obj_t *lbl_capsense;
    bool      capsense_ok;
#endif

#if BSP_HAS_POTENTIOMETER
    lv_obj_t *lbl_pot;
    bool      pot_ok;
#endif
} sensor_ctx_t;

static sensor_ctx_t ctx;

static lv_obj_t *create_sensor_card(lv_obj_t *parent, const char *title,
                                     lv_palette_t color, lv_obj_t **lbl_out)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 230, 110);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_border_color(card, lv_palette_main(color), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 4, 0);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_title, lv_palette_main(color), 0);

    *lbl_out = lv_label_create(card);
    lv_label_set_text(*lbl_out, "Initializing...");
    lv_obj_set_style_text_font(*lbl_out, &lv_font_montserrat_16, 0);

    return card;
}

static void sensor_timer_cb(lv_timer_t *timer)
{
    sensor_ctx_t *c = (sensor_ctx_t *)lv_timer_get_user_data(timer);

    /* BMI270 Accelerometer */
    if (c->bmi270_ok) {
        float ax, ay, az;
        if (bmi270_read_accel(&ax, &ay, &az) == 0) {
            lv_label_set_text_fmt(c->lbl_accel,
                "X:%+.2f\nY:%+.2f\nZ:%+.2f", (double)ax, (double)ay, (double)az);
        }
    }

#if BSP_HAS_DPS368
    if (c->dps368_ok) {
        float temp, press;
        if (dps368_read_both(&temp, &press) == 0) {
            lv_label_set_text_fmt(c->lbl_temp, "%.1f \xC2\xB0""C", (double)temp);
            lv_label_set_text_fmt(c->lbl_press, "%.1f hPa", (double)press);
        }
    }
#endif

#if BSP_HAS_SHT40
    if (c->sht40_ok) {
        float temp, hum;
        if (sht40_read(&temp, &hum) == 0) {
            lv_label_set_text_fmt(c->lbl_humidity, "%.1f%% RH\n%.1f \xC2\xB0""C",
                                  (double)hum, (double)temp);
        }
    }
#endif

#if BSP_HAS_CAPSENSE
    if (c->capsense_ok) {
        uint8_t btns = 0;
        capsense_read_buttons(&btns);
        lv_label_set_text_fmt(c->lbl_capsense, "Buttons: 0x%02X", btns);
    }
#endif

#if BSP_HAS_POTENTIOMETER
    if (c->pot_ok) {
        uint16_t val = 0;
        potentiometer_read(&val);
        lv_label_set_text_fmt(c->lbl_pot, "Raw: %u\n%d%%", val, (val * 100) / 4095);
    }
#endif
}

void example_main(lv_obj_t *parent)
{
    /* Initialize sensors */
    ctx.bmi270_ok = (bmi270_init() == 0);

#if BSP_HAS_DPS368
    ctx.dps368_ok = (dps368_init() == 0);
#endif
#if BSP_HAS_SHT40
    ctx.sht40_ok = (sht40_init() == 0);
#endif
#if BSP_HAS_CAPSENSE
    ctx.capsense_ok = (capsense_init() == 0);
#endif
#if BSP_HAS_POTENTIOMETER
    ctx.pot_ok = (potentiometer_init() == 0);
#endif

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "All Sensors Dashboard");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Card container with flex wrap */
    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 740, 300);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(grid, 8, 0);
    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    /* BMI270 card — always present */
    create_sensor_card(grid, LV_SYMBOL_REFRESH " IMU (BMI270)",
                       LV_PALETTE_GREEN, &ctx.lbl_accel);
    if (!ctx.bmi270_ok) lv_label_set_text(ctx.lbl_accel, "INIT FAIL");

#if BSP_HAS_DPS368
    create_sensor_card(grid, LV_SYMBOL_WARNING " Temp (DPS368)",
                       LV_PALETTE_ORANGE, &ctx.lbl_temp);
    create_sensor_card(grid, LV_SYMBOL_WARNING " Press (DPS368)",
                       LV_PALETTE_TEAL, &ctx.lbl_press);
    if (!ctx.dps368_ok) {
        lv_label_set_text(ctx.lbl_temp, "INIT FAIL");
        lv_label_set_text(ctx.lbl_press, "INIT FAIL");
    }
#endif

#if BSP_HAS_SHT40
    create_sensor_card(grid, LV_SYMBOL_WARNING " Climate (SHT40)",
                       LV_PALETTE_BLUE, &ctx.lbl_humidity);
    if (!ctx.sht40_ok) lv_label_set_text(ctx.lbl_humidity, "INIT FAIL");
#endif

#if BSP_HAS_CAPSENSE
    create_sensor_card(grid, "CapSense Touch",
                       LV_PALETTE_CYAN, &ctx.lbl_capsense);
    if (!ctx.capsense_ok) lv_label_set_text(ctx.lbl_capsense, "INIT FAIL");
#endif

#if BSP_HAS_POTENTIOMETER
    create_sensor_card(grid, "Potentiometer",
                       LV_PALETTE_PURPLE, &ctx.lbl_pot);
    if (!ctx.pot_ok) lv_label_set_text(ctx.lbl_pot, "INIT FAIL");
#endif

    /* Timer: 200ms */
    lv_timer_create(sensor_timer_cb, 200, &ctx);
}
