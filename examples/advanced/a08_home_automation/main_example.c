/**
 * A08 — Home Automation Dashboard
 *
 * Smart home UI with room cards, live sensor readings,
 * device control toggles, and scene buttons.
 */

#include "example_common.h"
#include <string.h>
#include <stdio.h>

#define REFRESH_MS      1000
#define NUM_ROOMS       4
#define NUM_DEVICES     6

typedef struct {
    const char *name;
    const char *icon;
    bool        on;
    lv_obj_t   *sw;
    lv_obj_t   *status_label;
} device_t;

typedef struct {
    const char *name;
    const char *icon;
    float       temp;
    float       humidity;
    int         light_pct;
    bool        occupied;
} room_t;

typedef struct {
    lv_obj_t   *parent;
    room_t      rooms[NUM_ROOMS];
    device_t    devices[NUM_DEVICES];
    lv_obj_t   *room_temp_labels[NUM_ROOMS];
    lv_obj_t   *room_hum_labels[NUM_ROOMS];
    lv_obj_t   *room_occ_dots[NUM_ROOMS];
    lv_obj_t   *room_light_bars[NUM_ROOMS];
    lv_obj_t   *outdoor_temp;
    lv_obj_t   *outdoor_hum;
    lv_obj_t   *outdoor_press;
    lv_obj_t   *energy_label;
    lv_obj_t   *time_label;
    float       energy_kwh;
} app_ctx_t;

static app_ctx_t g_ctx;

static void init_data(app_ctx_t *ctx)
{
    ctx->rooms[0] = (room_t){ "Living Room",  LV_SYMBOL_HOME,    24.0f, 45.0f, 80, true  };
    ctx->rooms[1] = (room_t){ "Bedroom",      LV_SYMBOL_IMAGE,   22.0f, 50.0f, 0,  false };
    ctx->rooms[2] = (room_t){ "Kitchen",      LV_SYMBOL_SETTINGS,26.0f, 55.0f, 100,true  };
    ctx->rooms[3] = (room_t){ "Office",       LV_SYMBOL_FILE,    23.0f, 42.0f, 60, true  };

    ctx->devices[0] = (device_t){ "AC Unit",       LV_SYMBOL_CHARGE,   true,  NULL, NULL };
    ctx->devices[1] = (device_t){ "Main Lights",   LV_SYMBOL_EYE_OPEN, true,  NULL, NULL };
    ctx->devices[2] = (device_t){ "Security Cam",  LV_SYMBOL_EYE_OPEN, true,  NULL, NULL };
    ctx->devices[3] = (device_t){ "Smart Lock",    LV_SYMBOL_EYE_CLOSE,false, NULL, NULL };
    ctx->devices[4] = (device_t){ "Thermostat",    LV_SYMBOL_SETTINGS, true,  NULL, NULL };
    ctx->devices[5] = (device_t){ "Water Heater",  LV_SYMBOL_WARNING,  false, NULL, NULL };
}

static void switch_event_cb(lv_event_t *e)
{
    device_t *dev = (device_t *)lv_event_get_user_data(e);
    lv_obj_t *sw = lv_event_get_target(e);
    dev->on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    lv_label_set_text(dev->status_label, dev->on ? "ON" : "OFF");
    lv_obj_set_style_text_color(dev->status_label,
        dev->on ? UI_COLOR_SUCCESS : lv_color_hex(0x607d8b), 0);
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    /* Use real sensor data for outdoor environment */
#if BSP_HAS_SHT40
    float out_temp = (float)snap.dps368.temperature_x100 / 100.0f;
    float out_hum  = (float)snap.sht40.humidity_x100 / 100.0f;
#elif BSP_HAS_DPS368
    float out_temp = (float)snap.dps368.temperature_x100 / 100.0f;
    float out_hum  = 55.0f;
#else
    float out_temp = 28.0f;
    float out_hum  = 55.0f;
#endif

#if BSP_HAS_DPS368
    float out_press = (float)snap.dps368.pressure_x100 / 100.0f;
#else
    float out_press = 1013.25f;
#endif

    char buf[48];
    snprintf(buf, sizeof(buf), "%.1f C", (double)out_temp);
    lv_label_set_text(ctx->outdoor_temp, buf);
    snprintf(buf, sizeof(buf), "%.0f%%", (double)out_hum);
    lv_label_set_text(ctx->outdoor_hum, buf);
    snprintf(buf, sizeof(buf), "%.1f hPa", (double)out_press);
    lv_label_set_text(ctx->outdoor_press, buf);

    /* Update room data with slight variations from sensors */
    float var = (float)(snap.bmi270.ax & 0x0F) * 0.05f;
    for (int i = 0; i < NUM_ROOMS; i++) {
        ctx->rooms[i].temp += (var - 0.375f) * 0.1f;
        if (ctx->rooms[i].temp < 18.0f) ctx->rooms[i].temp = 18.0f;
        if (ctx->rooms[i].temp > 32.0f) ctx->rooms[i].temp = 32.0f;

        snprintf(buf, sizeof(buf), "%.1f C", (double)ctx->rooms[i].temp);
        lv_label_set_text(ctx->room_temp_labels[i], buf);

        snprintf(buf, sizeof(buf), "%.0f%%", (double)ctx->rooms[i].humidity);
        lv_label_set_text(ctx->room_hum_labels[i], buf);

        lv_bar_set_value(ctx->room_light_bars[i], ctx->rooms[i].light_pct, LV_ANIM_ON);

        lv_obj_set_style_bg_color(ctx->room_occ_dots[i],
            ctx->rooms[i].occupied ? UI_COLOR_SUCCESS : lv_color_hex(0x455a64), 0);
    }

    /* Energy counter */
    ctx->energy_kwh += 0.02f + var * 0.005f;
    snprintf(buf, sizeof(buf), "%.2f kWh", (double)ctx->energy_kwh);
    lv_label_set_text(ctx->energy_label, buf);

    char tbuf[32];
    ipc_sensorhub_get_time_str(tbuf, sizeof(tbuf));
    lv_label_set_text(ctx->time_label, tbuf);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;
    init_data(ctx);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Title bar */
    lv_obj_t *top = lv_obj_create(parent);
    lv_obj_set_size(top, 780, 40);
    lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(top, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top, 8, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(top);
    lv_label_set_text(title, LV_SYMBOL_HOME " Smart Home");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    ctx->time_label = lv_label_create(top);
    lv_obj_set_style_text_color(ctx->time_label, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(ctx->time_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->time_label, LV_ALIGN_RIGHT_MID, -10, 0);

    /* Room cards row */
    for (int i = 0; i < NUM_ROOMS; i++) {
        lv_coord_t rx = 10 + i * 194;
        lv_obj_t *rc = lv_obj_create(parent);
        lv_obj_set_size(rc, 186, 155);
        lv_obj_set_pos(rc, rx, 50);
        lv_obj_set_style_bg_color(rc, UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_bg_opa(rc, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(rc, 12, 0);
        lv_obj_set_style_border_width(rc, 1, 0);
        lv_obj_set_style_border_color(rc, lv_color_hex(0x2a3a5c), 0);
        lv_obj_set_style_pad_all(rc, 10, 0);
        lv_obj_clear_flag(rc, LV_OBJ_FLAG_SCROLLABLE);

        /* Room name + occupancy dot */
        ctx->room_occ_dots[i] = lv_obj_create(rc);
        lv_obj_set_size(ctx->room_occ_dots[i], 8, 8);
        lv_obj_set_pos(ctx->room_occ_dots[i], 0, 4);
        lv_obj_set_style_bg_opa(ctx->room_occ_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->room_occ_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(ctx->room_occ_dots[i], 0, 0);
        lv_obj_clear_flag(ctx->room_occ_dots[i], LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *rn = lv_label_create(rc);
        lv_label_set_text(rn, ctx->rooms[i].name);
        lv_obj_set_style_text_color(rn, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(rn, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(rn, 14, 0);

        /* Temp */
        lv_obj_t *th = lv_label_create(rc);
        lv_label_set_text(th, "Temp:");
        lv_obj_set_style_text_color(th, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(th, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(th, 0, 24);

        ctx->room_temp_labels[i] = lv_label_create(rc);
        lv_obj_set_style_text_color(ctx->room_temp_labels[i], UI_COLOR_PRIMARY, 0);
        lv_obj_set_style_text_font(ctx->room_temp_labels[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(ctx->room_temp_labels[i], 60, 22);

        /* Humidity */
        lv_obj_t *hh = lv_label_create(rc);
        lv_label_set_text(hh, "RH:");
        lv_obj_set_style_text_color(hh, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(hh, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(hh, 0, 48);

        ctx->room_hum_labels[i] = lv_label_create(rc);
        lv_obj_set_style_text_color(ctx->room_hum_labels[i], UI_COLOR_SHT40, 0);
        lv_obj_set_style_text_font(ctx->room_hum_labels[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(ctx->room_hum_labels[i], 60, 46);

        /* Light bar */
        lv_obj_t *lh = lv_label_create(rc);
        lv_label_set_text(lh, "Light:");
        lv_obj_set_style_text_color(lh, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(lh, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(lh, 0, 74);

        ctx->room_light_bars[i] = lv_bar_create(rc);
        lv_obj_set_size(ctx->room_light_bars[i], 100, 10);
        lv_obj_set_pos(ctx->room_light_bars[i], 52, 78);
        lv_bar_set_range(ctx->room_light_bars[i], 0, 100);
        lv_bar_set_value(ctx->room_light_bars[i], ctx->rooms[i].light_pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(ctx->room_light_bars[i], lv_color_hex(0x263238), LV_PART_MAIN);
        lv_obj_set_style_bg_color(ctx->room_light_bars[i], UI_COLOR_WARNING, LV_PART_INDICATOR);
    }

    /* Devices panel (bottom left) */
    lv_obj_t *dev_card = lv_obj_create(parent);
    lv_obj_set_size(dev_card, 500, 200);
    lv_obj_set_pos(dev_card, 10, 216);
    lv_obj_set_style_bg_color(dev_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(dev_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dev_card, 12, 0);
    lv_obj_set_style_border_width(dev_card, 1, 0);
    lv_obj_set_style_border_color(dev_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(dev_card, 12, 0);
    lv_obj_clear_flag(dev_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *dh = lv_label_create(dev_card);
    lv_label_set_text(dh, "DEVICE CONTROLS");
    lv_obj_set_style_text_color(dh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(dh, &lv_font_montserrat_14, 0);

    for (int i = 0; i < NUM_DEVICES; i++) {
        int col = i / 3;
        int row = i % 3;
        lv_coord_t dx = col * 240;
        lv_coord_t dy = 24 + row * 52;

        lv_obj_t *dl = lv_label_create(dev_card);
        lv_label_set_text(dl, ctx->devices[i].name);
        lv_obj_set_style_text_color(dl, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(dl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(dl, dx, dy);

        ctx->devices[i].sw = lv_switch_create(dev_card);
        lv_obj_set_size(ctx->devices[i].sw, 44, 22);
        lv_obj_set_pos(ctx->devices[i].sw, dx + 130, dy - 2);
        lv_obj_set_style_bg_color(ctx->devices[i].sw, lv_color_hex(0x37474f), LV_PART_MAIN);
        lv_obj_set_style_bg_color(ctx->devices[i].sw, UI_COLOR_PRIMARY, LV_PART_INDICATOR | LV_STATE_CHECKED);
        if (ctx->devices[i].on) {
            lv_obj_add_state(ctx->devices[i].sw, LV_STATE_CHECKED);
        }

        ctx->devices[i].status_label = lv_label_create(dev_card);
        lv_obj_set_style_text_font(ctx->devices[i].status_label, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->devices[i].status_label, dx + 182, dy);
        lv_label_set_text(ctx->devices[i].status_label, ctx->devices[i].on ? "ON" : "OFF");
        lv_obj_set_style_text_color(ctx->devices[i].status_label,
            ctx->devices[i].on ? UI_COLOR_SUCCESS : lv_color_hex(0x607d8b), 0);

        lv_obj_add_event_cb(ctx->devices[i].sw, switch_event_cb, LV_EVENT_VALUE_CHANGED,
                           &ctx->devices[i]);
    }

    /* Outdoor & Energy panel (bottom right) */
    lv_obj_t *out_card = lv_obj_create(parent);
    lv_obj_set_size(out_card, 268, 200);
    lv_obj_set_pos(out_card, 520, 216);
    lv_obj_set_style_bg_color(out_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(out_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(out_card, 12, 0);
    lv_obj_set_style_border_width(out_card, 1, 0);
    lv_obj_set_style_border_color(out_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(out_card, 12, 0);
    lv_obj_clear_flag(out_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *oh = lv_label_create(out_card);
    lv_label_set_text(oh, "OUTDOOR (LIVE SENSORS)");
    lv_obj_set_style_text_color(oh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(oh, &lv_font_montserrat_14, 0);

    lv_obj_t *ot = lv_label_create(out_card);
    lv_label_set_text(ot, "Temperature:");
    lv_obj_set_style_text_color(ot, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ot, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ot, 0, 26);

    ctx->outdoor_temp = lv_label_create(out_card);
    lv_obj_set_style_text_color(ctx->outdoor_temp, UI_COLOR_DPS368, 0);
    lv_obj_set_style_text_font(ctx->outdoor_temp, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(ctx->outdoor_temp, 130, 22);

    lv_obj_t *ohu = lv_label_create(out_card);
    lv_label_set_text(ohu, "Humidity:");
    lv_obj_set_style_text_color(ohu, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ohu, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ohu, 0, 52);

    ctx->outdoor_hum = lv_label_create(out_card);
    lv_obj_set_style_text_color(ctx->outdoor_hum, UI_COLOR_SHT40, 0);
    lv_obj_set_style_text_font(ctx->outdoor_hum, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(ctx->outdoor_hum, 130, 48);

    lv_obj_t *op = lv_label_create(out_card);
    lv_label_set_text(op, "Pressure:");
    lv_obj_set_style_text_color(op, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(op, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(op, 0, 78);

    ctx->outdoor_press = lv_label_create(out_card);
    lv_obj_set_style_text_color(ctx->outdoor_press, UI_COLOR_BMM350, 0);
    lv_obj_set_style_text_font(ctx->outdoor_press, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->outdoor_press, 130, 76);

    /* Energy */
    lv_obj_t *eh = lv_label_create(out_card);
    lv_label_set_text(eh, "ENERGY USAGE");
    lv_obj_set_style_text_color(eh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(eh, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(eh, 0, 114);

    ctx->energy_label = lv_label_create(out_card);
    lv_obj_set_style_text_color(ctx->energy_label, UI_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(ctx->energy_label, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(ctx->energy_label, 0, 136);
    lv_label_set_text(ctx->energy_label, "0.00 kWh");

    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
