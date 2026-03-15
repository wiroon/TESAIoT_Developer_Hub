/**
 * @file    main_example.c
 * @brief   OPTIGA Crypto Demo — SHA256 + ECDSA Sign + Verify
 *
 * @description
 *   1) User enters text 2) SHA256 hash (show hex) 3) Sign with OPTIGA key
 *   4) Verify signature. Key slot 0xE0F1 (NOT 0xE0F0 which is blocked).
 *   Uses touch pause/resume IPC for I2C bus sharing.
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
#define OPTIGA_KEY_SLOT         0xE0F1  /* NOT 0xE0F0 — blocked */
#define SHA256_DIGEST_LEN       32
#define MAX_SIGNATURE_LEN       72      /* DER-encoded ECDSA P-256 */
#define MAX_PUBKEY_LEN          68      /* Uncompressed P-256 public key */
#define IPC_CMD_TOUCH_PAUSE     0xD6
#define IPC_CMD_TOUCH_RESUME    0xD7

#define COLOR_BG                lv_color_hex(0x142240)
#define COLOR_TEXT               lv_color_hex(0xE0E0E0)
#define COLOR_SUCCESS            lv_color_hex(0x4CAF50)
#define COLOR_ERROR              lv_color_hex(0xF44336)
#define COLOR_PENDING            lv_color_hex(0x616161)
#define COLOR_ACTIVE             lv_color_hex(0xFF9800)
#define COLOR_ACCENT             lv_color_hex(0x7C4DFF)

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *textarea;
    lv_obj_t    *hash_label;
    lv_obj_t    *sig_label;
    lv_obj_t    *verify_label;
    lv_obj_t    *step_dots[4];
    lv_obj_t    *status_label;
    lv_obj_t    *btn_run;
    uint8_t      digest[SHA256_DIGEST_LEN];
    uint8_t      signature[MAX_SIGNATURE_LEN];
    uint16_t     sig_len;
    uint8_t      pubkey[MAX_PUBKEY_LEN];
    uint16_t     pubkey_len;
} crypto_ctx_t;

static crypto_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Hex formatter
 * --------------------------------------------------------------------------- */
static void to_hex(const uint8_t *data, size_t len, char *out, size_t out_len)
{
    size_t pos = 0;
    for (size_t i = 0; i < len && pos + 2 < out_len; i++) {
        pos += snprintf(out + pos, out_len - pos, "%02X", data[i]);
    }
}

/* ---------------------------------------------------------------------------
 * Update step dot
 * --------------------------------------------------------------------------- */
static void step_update(crypto_ctx_t *ctx, int idx, lv_color_t color)
{
    lv_obj_set_style_bg_color(ctx->step_dots[idx], color, 0);
}

/* ---------------------------------------------------------------------------
 * Crypto task
 * --------------------------------------------------------------------------- */
static void crypto_task(void *pvParameters)
{
    crypto_ctx_t *ctx = (crypto_ctx_t *)pvParameters;
    char hex_buf[256];
    cy_rslt_t res;

    /* Get input text */
    lv_lock();
    const char *input = lv_textarea_get_text(ctx->textarea);
    size_t input_len = strlen(input);
    /* Copy to local buffer since LVGL text may change */
    char input_copy[128];
    strncpy(input_copy, input, sizeof(input_copy) - 1);
    input_copy[sizeof(input_copy) - 1] = '\0';
    input_len = strlen(input_copy);
    lv_unlock();

    if (input_len == 0) {
        lv_lock();
        lv_label_set_text(ctx->status_label, "Enter text first!");
        lv_obj_remove_flag(ctx->btn_run, LV_OBJ_FLAG_HIDDEN);
        lv_unlock();
        vTaskDelete(NULL);
        return;
    }

    /* Pause touch for I2C access */
    ipc_send_cmd(IPC_CMD_TOUCH_PAUSE, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Initialize OPTIGA */
    res = tesaiot_optiga_init();
    if (res != CY_RSLT_SUCCESS) {
        lv_lock();
        lv_label_set_text(ctx->status_label, "OPTIGA init failed!");
        step_update(ctx, 0, COLOR_ERROR);
        lv_unlock();
        goto done;
    }

    /* Step 1: SHA-256 Hash */
    lv_lock();
    step_update(ctx, 0, COLOR_ACTIVE);
    lv_label_set_text(ctx->status_label, "Computing SHA-256...");
    lv_unlock();

    res = tesaiot_optiga_sha256((const uint8_t *)input_copy, input_len, ctx->digest);
    lv_lock();
    if (res == CY_RSLT_SUCCESS) {
        step_update(ctx, 0, COLOR_SUCCESS);
        to_hex(ctx->digest, SHA256_DIGEST_LEN, hex_buf, sizeof(hex_buf));
        lv_label_set_text(ctx->hash_label, hex_buf);
    } else {
        step_update(ctx, 0, COLOR_ERROR);
        lv_label_set_text(ctx->status_label, "SHA-256 failed!");
        lv_unlock();
        goto done;
    }
    lv_unlock();

    /* Step 2: ECDSA Sign */
    lv_lock();
    step_update(ctx, 1, COLOR_ACTIVE);
    lv_label_set_text(ctx->status_label, "Signing with OPTIGA key 0xE0F1...");
    lv_unlock();

    ctx->sig_len = MAX_SIGNATURE_LEN;
    res = tesaiot_optiga_ecdsa_sign(ctx->digest, SHA256_DIGEST_LEN,
                                     OPTIGA_KEY_SLOT,
                                     ctx->signature, &ctx->sig_len);
    lv_lock();
    if (res == CY_RSLT_SUCCESS) {
        step_update(ctx, 1, COLOR_SUCCESS);
        to_hex(ctx->signature, ctx->sig_len > 32 ? 32 : ctx->sig_len,
               hex_buf, sizeof(hex_buf));
        strcat(hex_buf, "...");
        lv_label_set_text(ctx->sig_label, hex_buf);
    } else {
        step_update(ctx, 1, COLOR_ERROR);
        lv_label_set_text(ctx->status_label, "ECDSA sign failed!");
        lv_unlock();
        goto done;
    }
    lv_unlock();

    /* Step 3: Read public key for verification */
    lv_lock();
    step_update(ctx, 2, COLOR_ACTIVE);
    lv_label_set_text(ctx->status_label, "Reading public key...");
    lv_unlock();

    ctx->pubkey_len = MAX_PUBKEY_LEN;
    res = tesaiot_optiga_read_data(OPTIGA_KEY_SLOT + 0x100,
                                    ctx->pubkey, &ctx->pubkey_len);
    if (res != CY_RSLT_SUCCESS) {
        /* Generate keypair if no existing key */
        ctx->pubkey_len = MAX_PUBKEY_LEN;
        res = tesaiot_optiga_gen_keypair(OPTIGA_KEY_SLOT,
                                          ctx->pubkey, &ctx->pubkey_len);
    }

    lv_lock();
    if (res == CY_RSLT_SUCCESS) {
        step_update(ctx, 2, COLOR_SUCCESS);
    } else {
        step_update(ctx, 2, COLOR_ERROR);
        lv_label_set_text(ctx->status_label, "Public key read failed!");
        lv_unlock();
        goto done;
    }
    lv_unlock();

    /* Step 4: Verify signature */
    lv_lock();
    step_update(ctx, 3, COLOR_ACTIVE);
    lv_label_set_text(ctx->status_label, "Verifying signature...");
    lv_unlock();

    res = tesaiot_optiga_ecdsa_verify(ctx->digest, SHA256_DIGEST_LEN,
                                       ctx->signature, ctx->sig_len,
                                       ctx->pubkey, ctx->pubkey_len);
    lv_lock();
    if (res == CY_RSLT_SUCCESS) {
        step_update(ctx, 3, COLOR_SUCCESS);
        lv_label_set_text(ctx->verify_label, "SIGNATURE VALID");
        lv_obj_set_style_text_color(ctx->verify_label, COLOR_SUCCESS, 0);
        lv_label_set_text(ctx->status_label, "All steps completed successfully!");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_SUCCESS, 0);
    } else {
        step_update(ctx, 3, COLOR_ERROR);
        lv_label_set_text(ctx->verify_label, "SIGNATURE INVALID");
        lv_obj_set_style_text_color(ctx->verify_label, COLOR_ERROR, 0);
        lv_label_set_text(ctx->status_label, "Verification failed!");
    }
    lv_unlock();

done:
    /* Always resume touch */
    ipc_send_cmd(IPC_CMD_TOUCH_RESUME, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(50));

    lv_lock();
    lv_obj_remove_flag(ctx->btn_run, LV_OBJ_FLAG_HIDDEN);
    lv_unlock();

    vTaskDelete(NULL);
}

/* ---------------------------------------------------------------------------
 * Button callback
 * --------------------------------------------------------------------------- */
static void btn_run_cb(lv_event_t *e)
{
    crypto_ctx_t *ctx = (crypto_ctx_t *)lv_event_get_user_data(e);

    /* Reset indicators */
    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_bg_color(ctx->step_dots[i], COLOR_PENDING, 0);
    }
    lv_label_set_text(ctx->hash_label, "---");
    lv_label_set_text(ctx->sig_label, "---");
    lv_label_set_text(ctx->verify_label, "---");
    lv_obj_set_style_text_color(ctx->verify_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_color(ctx->status_label, COLOR_TEXT, 0);
    lv_obj_add_flag(ctx->btn_run, LV_OBJ_FLAG_HIDDEN);

    xTaskCreate(crypto_task, "optiga_crypto", 4096, ctx, 3, NULL);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    static const char *step_names[] = {
        "SHA-256 Hash", "ECDSA Sign", "Read PubKey", "Verify"
    };

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " OPTIGA Crypto");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Status */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Enter text and press Run");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 36);

    /* Input textarea */
    s_ctx.textarea = lv_textarea_create(parent);
    lv_obj_set_size(s_ctx.textarea, 340, 40);
    lv_obj_align(s_ctx.textarea, LV_ALIGN_TOP_LEFT, 16, 58);
    lv_textarea_set_placeholder_text(s_ctx.textarea, "Enter text to sign...");
    lv_textarea_set_one_line(s_ctx.textarea, true);
    lv_textarea_set_text(s_ctx.textarea, "Hello OPTIGA Trust M!");

    /* Run button */
    s_ctx.btn_run = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_run, 100, 40);
    lv_obj_align(s_ctx.btn_run, LV_ALIGN_TOP_LEFT, 368, 58);
    lv_obj_set_style_bg_color(s_ctx.btn_run, COLOR_ACCENT, 0);
    lv_obj_add_event_cb(s_ctx.btn_run, btn_run_cb, LV_EVENT_CLICKED, &s_ctx);
    lv_obj_t *rl = lv_label_create(s_ctx.btn_run);
    lv_label_set_text(rl, LV_SYMBOL_PLAY " Run");
    lv_obj_center(rl);

    /* Step indicators (horizontal) */
    lv_obj_t *steps = lv_obj_create(parent);
    lv_obj_set_size(steps, 560, 36);
    lv_obj_align(steps, LV_ALIGN_TOP_MID, 0, 104);
    lv_obj_set_style_bg_opa(steps, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(steps, 0, 0);
    lv_obj_set_style_pad_all(steps, 0, 0);
    lv_obj_set_flex_flow(steps, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(steps, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *chip = lv_obj_create(steps);
        lv_obj_set_size(chip, 120, 28);
        lv_obj_set_style_bg_color(chip, COLOR_BG, 0);
        lv_obj_set_style_border_width(chip, 1, 0);
        lv_obj_set_style_border_color(chip, COLOR_ACCENT, 0);
        lv_obj_set_style_radius(chip, 14, 0);
        lv_obj_set_style_pad_hor(chip, 6, 0);
        lv_obj_set_flex_flow(chip, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(chip, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(chip, 6, 0);

        s_ctx.step_dots[i] = lv_obj_create(chip);
        lv_obj_set_size(s_ctx.step_dots[i], 12, 12);
        lv_obj_set_style_radius(s_ctx.step_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_ctx.step_dots[i], COLOR_PENDING, 0);
        lv_obj_set_style_bg_opa(s_ctx.step_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_ctx.step_dots[i], 0, 0);

        lv_obj_t *lbl = lv_label_create(chip);
        lv_label_set_text(lbl, step_names[i]);
        lv_obj_set_style_text_color(lbl, COLOR_TEXT, 0);
    }

    /* Results panel */
    lv_obj_t *results = lv_obj_create(parent);
    lv_obj_set_size(results, 560, 200);
    lv_obj_align(results, LV_ALIGN_TOP_MID, 0, 146);
    lv_obj_set_style_bg_color(results, COLOR_BG, 0);
    lv_obj_set_style_border_color(results, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(results, 1, 0);
    lv_obj_set_style_radius(results, 8, 0);
    lv_obj_set_style_pad_all(results, 12, 0);
    lv_obj_set_flex_flow(results, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(results, 6, 0);

    /* Hash */
    lv_obj_t *h_title = lv_label_create(results);
    lv_label_set_text(h_title, "SHA-256 Hash:");
    lv_obj_set_style_text_color(h_title, COLOR_ACTIVE, 0);

    s_ctx.hash_label = lv_label_create(results);
    lv_label_set_text(s_ctx.hash_label, "---");
    lv_obj_set_style_text_color(s_ctx.hash_label, COLOR_TEXT, 0);
    lv_label_set_long_mode(s_ctx.hash_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ctx.hash_label, 530);

    /* Signature */
    lv_obj_t *s_title = lv_label_create(results);
    lv_label_set_text(s_title, "ECDSA Signature:");
    lv_obj_set_style_text_color(s_title, COLOR_ACTIVE, 0);

    s_ctx.sig_label = lv_label_create(results);
    lv_label_set_text(s_ctx.sig_label, "---");
    lv_obj_set_style_text_color(s_ctx.sig_label, COLOR_TEXT, 0);
    lv_label_set_long_mode(s_ctx.sig_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ctx.sig_label, 530);

    /* Verification result */
    s_ctx.verify_label = lv_label_create(results);
    lv_label_set_text(s_ctx.verify_label, "---");
    lv_obj_set_style_text_font(s_ctx.verify_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_ctx.verify_label, COLOR_TEXT, 0);
}
