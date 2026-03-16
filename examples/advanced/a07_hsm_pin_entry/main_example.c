/**
 * @file    main_example.c
 * @brief   PIN Entry UI Demo
 *
 * @description
 *   Custom PIN pad UI with numeric keypad (3x4 btnmatrix), masked dot display,
 *   local hash verification, 3-attempt lockout with 30s countdown timer.
 *   Pure LVGL — no IPC or OPTIGA calls.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define PIN_MAX_LEN             8
#define PIN_MIN_LEN             4
#define MAX_ATTEMPTS            3
#define LOCKOUT_SECONDS         30

#define COLOR_BG                lv_color_hex(0x0D1B2A)
#define COLOR_CARD              lv_color_hex(0x142240)
#define COLOR_TEXT              lv_color_hex(0xE0E0E0)
#define COLOR_SUCCESS           lv_color_hex(0x4CAF50)
#define COLOR_ERROR             lv_color_hex(0xF44336)
#define COLOR_WARNING           lv_color_hex(0xFF9800)
#define COLOR_KEY_BG            lv_color_hex(0x1A3050)
#define COLOR_KEY_PRESS         lv_color_hex(0x2196F3)
#define COLOR_ACCENT            lv_color_hex(0x7C4DFF)

/* ---------------------------------------------------------------------------
 * Simple local hash for PIN verification (FNV-1a 32-bit)
 * Not cryptographically secure — demo purposes only
 * --------------------------------------------------------------------------- */
static uint32_t fnv1a_hash(const char *data, size_t len)
{
    uint32_t hash = 0x811C9DC5u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 0x01000193u;
    }
    return hash;
}

/* Pre-computed FNV-1a hash of "1234" */
#define STORED_PIN_HASH  0x6222E842u

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *pin_display;
    lv_obj_t    *status_label;
    lv_obj_t    *attempts_label;
    lv_obj_t    *btnmatrix;
    lv_obj_t    *lockout_label;
    lv_obj_t    *success_panel;
    lv_timer_t  *lockout_timer;
    char         pin[PIN_MAX_LEN + 1];
    uint8_t      pin_len;
    uint8_t      attempts;
    bool         locked;
    int          lockout_remaining;
} pin_ctx_t;

static pin_ctx_t s_ctx;

/* Button matrix map: 3x4 grid */
static const char *btnm_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};

/* ---------------------------------------------------------------------------
 * Update PIN display with dots
 * --------------------------------------------------------------------------- */
static void update_pin_display(pin_ctx_t *ctx)
{
    if (ctx->pin_len == 0) {
        lv_label_set_text(ctx->pin_display, "_ _ _ _");
        return;
    }

    char display[PIN_MAX_LEN * 2 + 1];
    size_t pos = 0;
    for (uint8_t i = 0; i < ctx->pin_len && pos + 2 < sizeof(display); i++) {
        if (i > 0) display[pos++] = ' ';
        display[pos++] = '*';
    }
    display[pos] = '\0';
    lv_label_set_text(ctx->pin_display, display);
}

/* ---------------------------------------------------------------------------
 * Lockout timer callback
 * --------------------------------------------------------------------------- */
static void lockout_timer_cb(lv_timer_t *timer)
{
    pin_ctx_t *ctx = (pin_ctx_t *)lv_timer_get_user_data(timer);

    ctx->lockout_remaining--;
    if (ctx->lockout_remaining <= 0) {
        ctx->locked = false;
        ctx->attempts = 0;
        lv_obj_add_flag(ctx->lockout_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ctx->btnmatrix, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ctx->status_label, "Enter PIN");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_TEXT, 0);
        lv_label_set_text(ctx->attempts_label, "Attempts: 0/3");
        lv_timer_delete(timer);
        ctx->lockout_timer = NULL;
    } else {
        char buf[48];
        snprintf(buf, sizeof(buf), "Locked — retry in %d seconds", ctx->lockout_remaining);
        lv_label_set_text(ctx->lockout_label, buf);
    }
}

/* ---------------------------------------------------------------------------
 * Verify PIN locally
 * --------------------------------------------------------------------------- */
static void verify_pin(pin_ctx_t *ctx)
{
    uint32_t hash = fnv1a_hash(ctx->pin, ctx->pin_len);

    /* Clear PIN from memory immediately */
    memset(ctx->pin, 0, sizeof(ctx->pin));
    ctx->pin_len = 0;
    update_pin_display(ctx);

    if (hash == STORED_PIN_HASH) {
        ctx->attempts = 0;
        lv_label_set_text(ctx->status_label, "ACCESS GRANTED");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_SUCCESS, 0);
        lv_obj_remove_flag(ctx->success_panel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ctx->attempts_label, "Attempts: 0/3");
    } else {
        ctx->attempts++;
        char buf[32];
        snprintf(buf, sizeof(buf), "Attempts: %u/%u", ctx->attempts, MAX_ATTEMPTS);
        lv_label_set_text(ctx->attempts_label, buf);

        if (ctx->attempts >= MAX_ATTEMPTS) {
            ctx->locked = true;
            ctx->lockout_remaining = LOCKOUT_SECONDS;
            lv_obj_add_flag(ctx->btnmatrix, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ctx->lockout_label, LV_OBJ_FLAG_HIDDEN);

            char lock_buf[48];
            snprintf(lock_buf, sizeof(lock_buf), "Locked — retry in %d seconds",
                     LOCKOUT_SECONDS);
            lv_label_set_text(ctx->lockout_label, lock_buf);
            lv_label_set_text(ctx->status_label, "TOO MANY ATTEMPTS");
            lv_obj_set_style_text_color(ctx->status_label, COLOR_ERROR, 0);

            ctx->lockout_timer = lv_timer_create(lockout_timer_cb, 1000, ctx);
        } else {
            lv_label_set_text(ctx->status_label, "Wrong PIN — try again");
            lv_obj_set_style_text_color(ctx->status_label, COLOR_WARNING, 0);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Button matrix callback
 * --------------------------------------------------------------------------- */
static void btnm_event_cb(lv_event_t *e)
{
    pin_ctx_t *ctx = &s_ctx;
    lv_obj_t *obj = lv_event_get_target(e);
    uint32_t id = lv_btnmatrix_get_selected_btn(obj);
    const char *txt = lv_btnmatrix_get_btn_text(obj, id);

    if (txt == NULL || ctx->locked) return;

    /* Hide success panel on any key press */
    lv_obj_add_flag(ctx->success_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(ctx->status_label, COLOR_TEXT, 0);

    if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        if (ctx->pin_len > 0) {
            ctx->pin_len--;
            ctx->pin[ctx->pin_len] = '\0';
            update_pin_display(ctx);
            lv_label_set_text(ctx->status_label, "Enter PIN");
        }
    } else if (strcmp(txt, LV_SYMBOL_OK) == 0) {
        if (ctx->pin_len < PIN_MIN_LEN) {
            lv_label_set_text(ctx->status_label, "PIN too short (min 4)");
            lv_obj_set_style_text_color(ctx->status_label, COLOR_WARNING, 0);
            return;
        }
        verify_pin(ctx);
    } else {
        if (ctx->pin_len < PIN_MAX_LEN) {
            ctx->pin[ctx->pin_len] = txt[0];
            ctx->pin_len++;
            ctx->pin[ctx->pin_len] = '\0';
            update_pin_display(ctx);
            lv_label_set_text(ctx->status_label, "Enter PIN");
        }
    }
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    lv_obj_set_style_bg_color(parent, COLOR_BG, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " Secure PIN Entry");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* PIN display card */
    lv_obj_t *pin_card = lv_obj_create(parent);
    lv_obj_set_size(pin_card, 240, 60);
    lv_obj_align(pin_card, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_bg_color(pin_card, COLOR_CARD, 0);
    lv_obj_set_style_border_color(pin_card, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(pin_card, 2, 0);
    lv_obj_set_style_radius(pin_card, 12, 0);

    s_ctx.pin_display = lv_label_create(pin_card);
    lv_label_set_text(s_ctx.pin_display, "_ _ _ _");
    lv_obj_set_style_text_font(s_ctx.pin_display, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_ctx.pin_display, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_letter_space(s_ctx.pin_display, 8, 0);
    lv_obj_center(s_ctx.pin_display);

    /* Status label */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Enter PIN (default: 1234)");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_MID, 0, 108);

    /* Attempts label */
    s_ctx.attempts_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.attempts_label, "Attempts: 0/3");
    lv_obj_set_style_text_color(s_ctx.attempts_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.attempts_label, LV_ALIGN_TOP_RIGHT, -16, 108);

    /* Button matrix (keypad) */
    s_ctx.btnmatrix = lv_btnmatrix_create(parent);
    lv_btnmatrix_set_map(s_ctx.btnmatrix, btnm_map);
    lv_obj_set_size(s_ctx.btnmatrix, 300, 220);
    lv_obj_align(s_ctx.btnmatrix, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(s_ctx.btnmatrix, COLOR_BG, 0);
    lv_obj_set_style_bg_color(s_ctx.btnmatrix, COLOR_KEY_BG, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_ctx.btnmatrix, COLOR_KEY_PRESS, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(s_ctx.btnmatrix, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_ctx.btnmatrix, &lv_font_montserrat_24, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_ctx.btnmatrix, 8, LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_ctx.btnmatrix, 0, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(s_ctx.btnmatrix, 6, 0);
    lv_obj_set_style_pad_gap(s_ctx.btnmatrix, 6, 0);
    lv_obj_add_event_cb(s_ctx.btnmatrix, btnm_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Lockout label (hidden) */
    s_ctx.lockout_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.lockout_label, "");
    lv_obj_set_style_text_font(s_ctx.lockout_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_ctx.lockout_label, COLOR_ERROR, 0);
    lv_obj_align(s_ctx.lockout_label, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_flag(s_ctx.lockout_label, LV_OBJ_FLAG_HIDDEN);

    /* Success panel (hidden) */
    s_ctx.success_panel = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.success_panel, 200, 50);
    lv_obj_align(s_ctx.success_panel, LV_ALIGN_TOP_MID, 0, 128);
    lv_obj_set_style_bg_color(s_ctx.success_panel, COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(s_ctx.success_panel, 8, 0);
    lv_obj_add_flag(s_ctx.success_panel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *ok_lbl = lv_label_create(s_ctx.success_panel);
    lv_label_set_text(ok_lbl, LV_SYMBOL_OK " Authenticated");
    lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ok_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(ok_lbl);
}
