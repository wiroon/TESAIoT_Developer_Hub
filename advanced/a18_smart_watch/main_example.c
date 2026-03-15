/**
 * @file    main_example.c
 * @brief   Smart Watch Face — Time + seconds arc + sensor cards
 *
 * @description
 *   Watch face with digital time, seconds arc sweep, and 4 sensor cards.
 *   Time derived from FreeRTOS tick count (uptime clock).
 *
 * @board    AI Kit, Eva Kit
 * @author   TESAIoT
 */

#include "example_common.h"

/* ── Layout ─────────────────────────────────────────────────────── */
#define CLOCK_ARC_SIZE   260
#define CARD_W           170
#define CARD_H            60

/* ── State ──────────────────────────────────────────────────────── */
static lv_obj_t *s_arc_sec;
static lv_obj_t *s_lbl_time;
static lv_obj_t *s_lbl_sec;
static lv_obj_t *s_lbl_date;
static lv_obj_t *s_sensor_vals[4];

/* ── Sensor card definition ─────────────────────────────────────── */
typedef struct {
    const char *icon;
    const char *name;
    lv_color_t  color;
} sensor_def_t;

static const sensor_def_t s_sensors[] = {
#if BSP_HAS_DPS368
    { LV_SYMBOL_SETTINGS, "Pressure",  {0} },
#endif
#if BSP_HAS_SHT40
    { LV_SYMBOL_CHARGE,   "Humidity",  {0} },
#endif
    { LV_SYMBOL_REFRESH,  "Accel",     {0} },
    { LV_SYMBOL_GPS,      "Compass",   {0} },
};

#define NUM_SENSORS (sizeof(s_sensors) / sizeof(s_sensors[0]))

/* ── Timer: update clock every second ───────────────────────────── */
static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;

    /* Derive time from FreeRTOS uptime */
    uint32_t tick_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t total_sec = tick_ms / 1000;
    uint32_t hrs  = (total_sec / 3600) % 24;
    uint32_t mins = (total_sec / 60) % 60;
    uint32_t secs = total_sec % 60;

    /* Time display */
    lv_label_set_text_fmt(s_lbl_time, "%02lu:%02lu",
                          (unsigned long)hrs, (unsigned long)mins);
    lv_label_set_text_fmt(s_lbl_sec, ":%02lu", (unsigned long)secs);

    /* Seconds arc: 0-360 degrees over 60 seconds */
    int32_t arc_end = (int32_t)(secs * 6);   /* 360 / 60 = 6 deg/sec */
    lv_arc_set_angles(s_arc_sec, 270, 270 + arc_end);

    /* Date line (boot date placeholder) */
    lv_label_set_text(s_lbl_date, "2026-03-15  SAT");

    /* Simulated sensor values (replace with real reads) */
    static uint32_t frame = 0;
    frame++;
    int idx = 0;
#if BSP_HAS_DPS368
    lv_label_set_text_fmt(s_sensor_vals[idx++], "%d hPa", 1013 + (int)(frame % 5));
#endif
#if BSP_HAS_SHT40
    lv_label_set_text_fmt(s_sensor_vals[idx++], "%d %%RH", 55 + (int)(frame % 10));
#endif
    /* Accel magnitude */
    lv_label_set_text_fmt(s_sensor_vals[idx++], "%.1f g",
                          0.98f + (float)(frame % 5) * 0.01f);
    /* Compass heading */
    lv_label_set_text_fmt(s_sensor_vals[idx++], "%d\xc2\xb0",
                          (int)(frame * 3) % 360);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "A18 \xe2\x80\x94 Smart Watch");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    /* === Watch face background === */
    lv_obj_t *face_bg = example_card_create(parent, CLOCK_ARC_SIZE + 30,
                                             CLOCK_ARC_SIZE + 30,
                                             lv_color_hex(0x0A1628));
    lv_obj_align(face_bg, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_radius(face_bg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(face_bg, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(face_bg, 2, 0);

    /* Seconds arc */
    s_arc_sec = lv_arc_create(face_bg);
    lv_obj_set_size(s_arc_sec, CLOCK_ARC_SIZE, CLOCK_ARC_SIZE);
    lv_obj_center(s_arc_sec);
    lv_arc_set_bg_angles(s_arc_sec, 0, 360);
    lv_arc_set_angles(s_arc_sec, 270, 270);
    lv_obj_set_style_arc_color(s_arc_sec, lv_color_hex(0x1a3050), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc_sec, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_arc_sec, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc_sec, 6, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_arc_sec, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_remove_flag(s_arc_sec, LV_OBJ_FLAG_CLICKABLE);

    /* Hour tick marks (12 positions) */
    for (int i = 0; i < 12; i++) {
        float angle = (float)i * 30.0f * 3.14159f / 180.0f;
        int r = CLOCK_ARC_SIZE / 2 - 16;
        int cx = (int)(sinf(angle) * r);
        int cy = (int)(-cosf(angle) * r);

        lv_obj_t *tick = lv_obj_create(face_bg);
        lv_obj_set_size(tick, 4, (i % 3 == 0) ? 14 : 8);
        lv_obj_set_style_bg_color(tick, (i % 3 == 0) ? UI_COLOR_TEXT : UI_COLOR_TEXT_DIM, 0);
        lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(tick, 2, 0);
        lv_obj_set_style_border_width(tick, 0, 0);
        lv_obj_align(tick, LV_ALIGN_CENTER, cx, cy);
    }

    /* Time label */
    s_lbl_time = lv_label_create(face_bg);
    lv_label_set_text(s_lbl_time, "00:00");
    lv_obj_set_style_text_font(s_lbl_time, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_lbl_time, UI_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_time, LV_ALIGN_CENTER, 0, -10);

    /* Seconds label */
    s_lbl_sec = lv_label_create(face_bg);
    lv_label_set_text(s_lbl_sec, ":00");
    lv_obj_set_style_text_font(s_lbl_sec, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_lbl_sec, UI_COLOR_PRIMARY, 0);
    lv_obj_align(s_lbl_sec, LV_ALIGN_CENTER, 0, 16);

    /* Date line */
    s_lbl_date = lv_label_create(face_bg);
    lv_label_set_text(s_lbl_date, "----");
    lv_obj_set_style_text_font(s_lbl_date, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_date, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(s_lbl_date, LV_ALIGN_CENTER, 0, 40);

    /* === Sensor summary cards === */
    lv_color_t card_colors[] = {
        UI_COLOR_DPS368, UI_COLOR_SHT40, UI_COLOR_BMI270, UI_COLOR_BMM350
    };

    int num = (int)NUM_SENSORS;
    if (num > 4) num = 4;
    int total_w = num * CARD_W + (num - 1) * 8;
    int start_x = -total_w / 2 + CARD_W / 2;

    for (int i = 0; i < num; i++) {
        lv_obj_t *card = example_card_create(parent, CARD_W, CARD_H,
                                              UI_COLOR_CARD_BG);
        lv_obj_align(card, LV_ALIGN_BOTTOM_MID,
                     start_x + i * (CARD_W + 8), -10);

        /* Icon */
        lv_obj_t *icon = lv_label_create(card);
        lv_label_set_text(icon, s_sensors[i].icon);
        lv_obj_set_style_text_color(icon, card_colors[i < 4 ? i : 0], 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, 0);
        lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 0, 0);

        /* Name */
        lv_obj_t *name = lv_label_create(card);
        lv_label_set_text(name, s_sensors[i].name);
        lv_obj_set_style_text_color(name, UI_COLOR_TEXT_DIM, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 24, 0);

        /* Value */
        s_sensor_vals[i] = lv_label_create(card);
        lv_label_set_text(s_sensor_vals[i], "--");
        lv_obj_set_style_text_color(s_sensor_vals[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(s_sensor_vals[i], &lv_font_montserrat_20, 0);
        lv_obj_align(s_sensor_vals[i], LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }

    /* Start clock timer (1 second) */
    lv_timer_create(clock_timer_cb, 1000, NULL);
}
