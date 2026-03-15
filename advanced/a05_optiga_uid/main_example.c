/**
 * @file    main_example.c
 * @brief   OPTIGA Trust M UID Reader
 *
 * @description
 *   Init OPTIGA Trust M -> read UID -> display hex on LVGL.
 *   Touch pause/resume IPC for I2C bus sharing with CM55 touch.
 *   Pattern: IPC_CMD_TOUCH_PAUSE -> optiga_init -> optiga_uid_read -> IPC_CMD_TOUCH_RESUME
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"
#include "tesaiot_optiga_trust_m.h"
#include "ipc_communication.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define OPTIGA_UID_OID          0xE0C2
#define OPTIGA_UID_LEN          27
#define IPC_CMD_TOUCH_PAUSE     0xD6
#define IPC_CMD_TOUCH_RESUME    0xD7

#define COLOR_BG                lv_color_hex(0x142240)
#define COLOR_TEXT               lv_color_hex(0xE0E0E0)
#define COLOR_SUCCESS            lv_color_hex(0x4CAF50)
#define COLOR_ERROR              lv_color_hex(0xF44336)
#define COLOR_PENDING            lv_color_hex(0xFF9800)
#define COLOR_DONE               lv_color_hex(0x4CAF50)
#define COLOR_ACCENT             lv_color_hex(0x00BCD4)

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef enum {
    STEP_PAUSE_TOUCH,
    STEP_INIT_OPTIGA,
    STEP_READ_UID,
    STEP_RESUME_TOUCH,
    STEP_DONE,
    STEP_ERROR,
} optiga_step_t;

typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *step_labels[4];
    lv_obj_t    *step_dots[4];
    lv_obj_t    *uid_label;
    lv_obj_t    *status_label;
    lv_obj_t    *btn_read;
    uint8_t      uid_buf[OPTIGA_UID_LEN];
} optiga_uid_ctx_t;

static optiga_uid_ctx_t s_ctx;

static const char *step_names[] = {
    "Pause Touch (IPC 0xD6)",
    "Initialize OPTIGA",
    "Read UID (0xE0C2)",
    "Resume Touch (IPC 0xD7)",
};

/* ---------------------------------------------------------------------------
 * Update step indicator
 * --------------------------------------------------------------------------- */
static void update_step(optiga_uid_ctx_t *ctx, int step, bool success)
{
    lv_color_t color = success ? COLOR_DONE : COLOR_ERROR;
    lv_obj_set_style_bg_color(ctx->step_dots[step], color, 0);
}

/* ---------------------------------------------------------------------------
 * Format UID as hex string
 * --------------------------------------------------------------------------- */
static void format_uid_hex(const uint8_t *uid, size_t len, char *out, size_t out_len)
{
    size_t pos = 0;
    for (size_t i = 0; i < len && pos + 3 < out_len; i++) {
        pos += snprintf(out + pos, out_len - pos, "%02X", uid[i]);
        if (i < len - 1 && pos + 2 < out_len) {
            out[pos++] = ':';
        }
    }
    out[pos] = '\0';
}

/* ---------------------------------------------------------------------------
 * OPTIGA read task
 * --------------------------------------------------------------------------- */
static void optiga_read_task(void *pvParameters)
{
    optiga_uid_ctx_t *ctx = (optiga_uid_ctx_t *)pvParameters;
    bool success = true;

    /* Step 0: Pause touch */
    lv_lock();
    lv_label_set_text(ctx->status_label, "Pausing touch controller...");
    lv_unlock();

    cy_rslt_t res = ipc_send_cmd(IPC_CMD_TOUCH_PAUSE, NULL, 0);
    lv_lock();
    update_step(ctx, 0, res == CY_RSLT_SUCCESS);
    lv_unlock();
    if (res != CY_RSLT_SUCCESS) {
        success = false;
        goto cleanup;
    }

    /* Small delay for touch to fully release I2C */
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Step 1: Initialize OPTIGA */
    lv_lock();
    lv_label_set_text(ctx->status_label, "Initializing OPTIGA Trust M...");
    lv_unlock();

    res = tesaiot_optiga_init();
    lv_lock();
    update_step(ctx, 1, res == CY_RSLT_SUCCESS);
    lv_unlock();
    if (res != CY_RSLT_SUCCESS) {
        success = false;
        goto cleanup;
    }

    /* Step 2: Read UID */
    lv_lock();
    lv_label_set_text(ctx->status_label, "Reading UID from 0xE0C2...");
    lv_unlock();

    uint16_t uid_len = OPTIGA_UID_LEN;
    res = tesaiot_optiga_read_data(OPTIGA_UID_OID, ctx->uid_buf, &uid_len);
    lv_lock();
    update_step(ctx, 2, res == CY_RSLT_SUCCESS);
    lv_unlock();
    if (res != CY_RSLT_SUCCESS) {
        success = false;
        goto cleanup;
    }

cleanup:
    /* Step 3: Resume touch (always, even on error) */
    lv_lock();
    lv_label_set_text(ctx->status_label, "Resuming touch controller...");
    lv_unlock();

    cy_rslt_t resume_res = ipc_send_cmd(IPC_CMD_TOUCH_RESUME, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(50));

    lv_lock();
    update_step(ctx, 3, resume_res == CY_RSLT_SUCCESS);

    if (success) {
        /* Format and display UID */
        char hex_str[128];
        format_uid_hex(ctx->uid_buf, OPTIGA_UID_LEN, hex_str, sizeof(hex_str));
        lv_label_set_text(ctx->uid_label, hex_str);
        lv_obj_set_style_text_color(ctx->uid_label, COLOR_ACCENT, 0);
        lv_label_set_text(ctx->status_label, "UID read successfully!");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_SUCCESS, 0);
    } else {
        char err[64];
        snprintf(err, sizeof(err), "Failed at step (result: 0x%08lX)", (unsigned long)res);
        lv_label_set_text(ctx->status_label, err);
        lv_obj_set_style_text_color(ctx->status_label, COLOR_ERROR, 0);
    }

    lv_obj_remove_flag(ctx->btn_read, LV_OBJ_FLAG_HIDDEN);
    lv_unlock();

    vTaskDelete(NULL);
}

/* ---------------------------------------------------------------------------
 * Read button callback
 * --------------------------------------------------------------------------- */
static void btn_read_cb(lv_event_t *e)
{
    optiga_uid_ctx_t *ctx = (optiga_uid_ctx_t *)lv_event_get_user_data(e);

    /* Reset step indicators */
    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_bg_color(ctx->step_dots[i], COLOR_PENDING, 0);
    }
    lv_label_set_text(ctx->uid_label, "---");
    lv_obj_set_style_text_color(ctx->uid_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_color(ctx->status_label, COLOR_TEXT, 0);
    lv_obj_add_flag(ctx->btn_read, LV_OBJ_FLAG_HIDDEN);

    xTaskCreate(optiga_read_task, "optiga_uid", 4096, ctx, 3, NULL);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* --- Title --- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " OPTIGA Trust M - UID");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* --- Status --- */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Read to start");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 38);

    /* --- Step progress panel --- */
    lv_obj_t *steps_panel = lv_obj_create(parent);
    lv_obj_set_size(steps_panel, 400, 180);
    lv_obj_align(steps_panel, LV_ALIGN_TOP_LEFT, 16, 60);
    lv_obj_set_style_bg_color(steps_panel, COLOR_BG, 0);
    lv_obj_set_style_border_color(steps_panel, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(steps_panel, 1, 0);
    lv_obj_set_style_radius(steps_panel, 8, 0);
    lv_obj_set_style_pad_all(steps_panel, 12, 0);
    lv_obj_set_flex_flow(steps_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(steps_panel, 8, 0);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = lv_obj_create(steps_panel);
        lv_obj_set_size(row, lv_pct(100), 32);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 10, 0);

        /* Step indicator dot */
        s_ctx.step_dots[i] = lv_obj_create(row);
        lv_obj_set_size(s_ctx.step_dots[i], 14, 14);
        lv_obj_set_style_radius(s_ctx.step_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_ctx.step_dots[i], COLOR_PENDING, 0);
        lv_obj_set_style_bg_opa(s_ctx.step_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_ctx.step_dots[i], 0, 0);

        /* Step label */
        s_ctx.step_labels[i] = lv_label_create(row);
        char step_text[64];
        snprintf(step_text, sizeof(step_text), "Step %d: %s", i + 1, step_names[i]);
        lv_label_set_text(s_ctx.step_labels[i], step_text);
        lv_obj_set_style_text_color(s_ctx.step_labels[i], COLOR_TEXT, 0);
    }

    /* --- UID display card --- */
    lv_obj_t *uid_card = lv_obj_create(parent);
    lv_obj_set_size(uid_card, 350, 80);
    lv_obj_align(uid_card, LV_ALIGN_TOP_RIGHT, -16, 80);
    lv_obj_set_style_bg_color(uid_card, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_border_color(uid_card, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(uid_card, 2, 0);
    lv_obj_set_style_radius(uid_card, 12, 0);
    lv_obj_set_style_pad_all(uid_card, 8, 0);

    lv_obj_t *uid_title = lv_label_create(uid_card);
    lv_label_set_text(uid_title, "Device UID:");
    lv_obj_set_style_text_color(uid_title, COLOR_ACCENT, 0);
    lv_obj_align(uid_title, LV_ALIGN_TOP_LEFT, 4, 0);

    s_ctx.uid_label = lv_label_create(uid_card);
    lv_label_set_text(s_ctx.uid_label, "---");
    lv_obj_set_style_text_color(s_ctx.uid_label, COLOR_TEXT, 0);
    lv_label_set_long_mode(s_ctx.uid_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ctx.uid_label, 330);
    lv_obj_align(s_ctx.uid_label, LV_ALIGN_BOTTOM_LEFT, 4, -4);

    /* --- Read button --- */
    s_ctx.btn_read = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_read, 140, 44);
    lv_obj_align(s_ctx.btn_read, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_read, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(s_ctx.btn_read, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_read, btn_read_cb, LV_EVENT_CLICKED, &s_ctx);

    lv_obj_t *btn_lbl = lv_label_create(s_ctx.btn_read);
    lv_label_set_text(btn_lbl, LV_SYMBOL_DOWNLOAD " Read");
    lv_obj_center(btn_lbl);
}
