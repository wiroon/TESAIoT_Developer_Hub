/**
 * @file    main_example.c
 * @brief   Gyroscope XYZ — BMI270 gyro with arc gauge visualization
 *
 * Reads BMI270 gyroscope at 100ms intervals. Displays angular velocity
 * (degrees/sec) for X, Y, Z with arc gauges.
 */

#include "example_common.h"
#include "sensor_bmi270.h"

typedef struct {
    lv_obj_t *lbl_x;
    lv_obj_t *lbl_y;
    lv_obj_t *lbl_z;
    lv_obj_t *arc_x;
    lv_obj_t *arc_y;
    lv_obj_t *arc_z;
    bool      sensor_ok;
} gyro_ctx_t;

static gyro_ctx_t ctx;

static void gyro_timer_cb(lv_timer_t *timer)
{
    gyro_ctx_t *c = (gyro_ctx_t *)lv_timer_get_user_data(timer);
    if (!c->sensor_ok) return;

    float gx, gy, gz;
    if (bmi270_read_gyro(&gx, &gy, &gz) == 0) {
        lv_label_set_text_fmt(c->lbl_x, "X\n%+.1f", (double)gx);
        lv_label_set_text_fmt(c->lbl_y, "Y\n%+.1f", (double)gy);
        lv_label_set_text_fmt(c->lbl_z, "Z\n%+.1f", (double)gz);

        /* Map [-250, +250] dps to [0, 100] for arcs */
        int32_t ax = (int32_t)((gx + 250.0f) * 0.2f);
        int32_t ay = (int32_t)((gy + 250.0f) * 0.2f);
        int32_t az = (int32_t)((gz + 250.0f) * 0.2f);
        lv_arc_set_value(c->arc_x, LV_CLAMP(0, ax, 100));
        lv_arc_set_value(c->arc_y, LV_CLAMP(0, ay, 100));
        lv_arc_set_value(c->arc_z, LV_CLAMP(0, az, 100));
    }
}

static void create_gyro_gauge(lv_obj_t *parent, const char *axis, lv_palette_t color,
                               lv_obj_t **arc_out, lv_obj_t **lbl_out, int x_offset)
{
    *arc_out = lv_arc_create(parent);
    lv_obj_set_size(*arc_out, 150, 150);
    lv_arc_set_rotation(*arc_out, 135);
    lv_arc_set_bg_angles(*arc_out, 0, 270);
    lv_arc_set_range(*arc_out, 0, 100);
    lv_arc_set_value(*arc_out, 50);
    lv_obj_remove_style(*arc_out, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(*arc_out, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(*arc_out, lv_palette_main(color), LV_PART_INDICATOR);
    lv_obj_align(*arc_out, LV_ALIGN_CENTER, x_offset, 0);

    *lbl_out = lv_label_create(parent);
    lv_label_set_text_fmt(*lbl_out, "%s\n0.0", axis);
    lv_obj_set_style_text_font(*lbl_out, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(*lbl_out, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(*lbl_out, lv_palette_main(color), 0);
    lv_obj_align(*lbl_out, LV_ALIGN_CENTER, x_offset, 0);
}

void example_main(lv_obj_t *parent)
{
    ctx.sensor_ok = (bmi270_init() == 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Gyroscope (BMI270) — \xC2\xB0/s");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    if (!ctx.sensor_ok) {
        lv_obj_t *err = lv_label_create(parent);
        lv_label_set_text(err, "BMI270 init failed!");
        lv_obj_set_style_text_color(err, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_align(err, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    /* Three arc gauges side by side */
    create_gyro_gauge(parent, "X", LV_PALETTE_RED,   &ctx.arc_x, &ctx.lbl_x, -220);
    create_gyro_gauge(parent, "Y", LV_PALETTE_GREEN, &ctx.arc_y, &ctx.lbl_y,    0);
    create_gyro_gauge(parent, "Z", LV_PALETTE_BLUE,  &ctx.arc_z, &ctx.lbl_z,  220);

    /* Unit label */
    lv_obj_t *unit = lv_label_create(parent);
    lv_label_set_text(unit, "Unit: degrees/second | Range: \xC2\xB1""250 \xC2\xB0/s");
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_14, 0);
    lv_obj_align(unit, LV_ALIGN_BOTTOM_MID, 0, -10);

    /* Timer: 100ms */
    lv_timer_create(gyro_timer_cb, 100, &ctx);
}
