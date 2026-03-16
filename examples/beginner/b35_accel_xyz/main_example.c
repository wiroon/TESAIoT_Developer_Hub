/**
 * @file    main_example.c
 * @brief   Accelerometer XYZ — BMI270 live accel display
 *
 * Reads BMI270 accelerometer at 100ms intervals and displays
 * X, Y, Z values with color-coded labels and bar indicators.
 */

#include "example_common.h"
#include "sensor_bmi270.h"

typedef struct {
    lv_obj_t *lbl_x;
    lv_obj_t *lbl_y;
    lv_obj_t *lbl_z;
    lv_obj_t *bar_x;
    lv_obj_t *bar_y;
    lv_obj_t *bar_z;
    bool      sensor_ok;
} accel_ctx_t;

static accel_ctx_t ctx;

static void accel_timer_cb(lv_timer_t *timer)
{
    accel_ctx_t *c = (accel_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->sensor_ok) return;

    float ax, ay, az;
    if (bmi270_read_accel(&ax, &ay, &az) == 0) {
        lv_label_set_text_fmt(c->lbl_x, "X: %+.2f g", (double)ax);
        lv_label_set_text_fmt(c->lbl_y, "Y: %+.2f g", (double)ay);
        lv_label_set_text_fmt(c->lbl_z, "Z: %+.2f g", (double)az);

        /* Map [-2, +2]g to [0, 100] for bars */
        int32_t bx = (int32_t)((ax + 2.0f) * 25.0f);
        int32_t by = (int32_t)((ay + 2.0f) * 25.0f);
        int32_t bz = (int32_t)((az + 2.0f) * 25.0f);
        lv_bar_set_value(c->bar_x, LV_CLAMP(0, bx, 100), LV_ANIM_ON);
        lv_bar_set_value(c->bar_y, LV_CLAMP(0, by, 100), LV_ANIM_ON);
        lv_bar_set_value(c->bar_z, LV_CLAMP(0, bz, 100), LV_ANIM_ON);
    }
}

static void create_axis_row(lv_obj_t *parent, const char *name, lv_palette_t color,
                             lv_obj_t **lbl_out, lv_obj_t **bar_out, int y_offset)
{
    *lbl_out = lv_label_create(parent);
    lv_label_set_text_fmt(*lbl_out, "%s: +0.00 g", name);
    lv_obj_set_style_text_font(*lbl_out, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(*lbl_out, lv_palette_main(color), 0);
    lv_obj_align(*lbl_out, LV_ALIGN_LEFT_MID, 30, y_offset);

    *bar_out = lv_bar_create(parent);
    lv_obj_set_size(*bar_out, 300, 20);
    lv_bar_set_range(*bar_out, 0, 100);
    lv_bar_set_value(*bar_out, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(*bar_out, lv_palette_main(color), LV_PART_INDICATOR);
    lv_obj_align(*bar_out, LV_ALIGN_RIGHT_MID, -30, y_offset);
}

void example_main(lv_obj_t *parent)
{
    /* Initialize sensor */
    ctx.sensor_ok = (bmi270_init() == 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_REFRESH " Accelerometer (BMI270)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    if (!ctx.sensor_ok) {
        lv_obj_t *err = lv_label_create(parent);
        lv_label_set_text(err, "Sensor init failed!");
        lv_obj_set_style_text_color(err, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_align(err, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    /* Axis rows */
    create_axis_row(parent, "X", LV_PALETTE_RED,    &ctx.lbl_x, &ctx.bar_x, -40);
    create_axis_row(parent, "Y", LV_PALETTE_GREEN,  &ctx.lbl_y, &ctx.bar_y,  10);
    create_axis_row(parent, "Z", LV_PALETTE_BLUE,   &ctx.lbl_z, &ctx.bar_z,  60);

    /* Timer: 100ms */
    lv_timer_create(accel_timer_cb, 100, &ctx);
}
