/**
 * @file    main_example.c
 * @brief   Security Info Display — HSM Chip Overview
 *
 * @description
 *   Displays hardware security module (OPTIGA Trust M) chip information
 *   using LVGL cards: chip UID, lifecycle state, available certificate
 *   slots, and supported algorithms. Static display — no direct OPTIGA
 *   calls required (informational overview for the MTB template).
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "example_common.h"

/* ---------------------------------------------------------------------------
 * Colors
 * --------------------------------------------------------------------------- */
#define COLOR_BG                lv_color_hex(0x142240)
#define COLOR_CARD              lv_color_hex(0x0D1B2A)
#define COLOR_TEXT              lv_color_hex(0xE0E0E0)
#define COLOR_ACCENT            lv_color_hex(0x00BCD4)
#define COLOR_SUCCESS           lv_color_hex(0x4CAF50)
#define COLOR_WARNING           lv_color_hex(0xFF9800)
#define COLOR_SECTION           lv_color_hex(0x7C4DFF)

/* ---------------------------------------------------------------------------
 * Chip info (static reference data for OPTIGA Trust M SLS 32AIA)
 * --------------------------------------------------------------------------- */
static const char *CHIP_NAME       = "Infineon OPTIGA Trust M (SLS 32AIA)";
static const char *CHIP_UID_DEMO   = "CD:16:33:01:00:1C:00:05:00:00:0B:04:D5:03:70:00:12:00:17:00:03:00:00:00:9E:00:03";
static const char *LIFECYCLE       = "Operational (0x07)";

typedef struct {
    const char *slot;
    const char *type;
    const char *status;
} cert_slot_info_t;

static const cert_slot_info_t CERT_SLOTS[] = {
    { "0xE0E0", "X.509 Certificate", "Pre-provisioned" },
    { "0xE0E1", "X.509 Certificate", "Available"        },
    { "0xE0E2", "X.509 Certificate", "Available"        },
    { "0xE0E3", "X.509 Certificate", "Available"        },
};
#define NUM_CERT_SLOTS  4

typedef struct {
    const char *name;
    const char *detail;
} algo_info_t;

static const algo_info_t ALGORITHMS[] = {
    { "ECC P-256",   "ECDSA Sign/Verify, ECDH" },
    { "ECC P-384",   "ECDSA Sign/Verify, ECDH" },
    { "SHA-256",     "Hardware hash engine"     },
    { "AES-128/256", "CCM encrypt/decrypt"      },
    { "TRNG",        "True random number gen"   },
    { "RSA 1024/2048","Sign/Verify/Encrypt"     },
};
#define NUM_ALGORITHMS  6

/* ---------------------------------------------------------------------------
 * Helper: create a section card
 * --------------------------------------------------------------------------- */
static lv_obj_t *create_section(lv_obj_t *parent, const char *title_text,
                                 int w, int h, int x, int y)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_border_color(card, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 4, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_color(title, COLOR_SECTION, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    return card;
}

/* ---------------------------------------------------------------------------
 * Helper: create a key-value row inside a container
 * --------------------------------------------------------------------------- */
static void add_kv_row(lv_obj_t *parent, const char *key, const char *value,
                        lv_color_t val_color)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 8, 0);

    lv_obj_t *k = lv_label_create(row);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, COLOR_ACCENT, 0);

    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, value);
    lv_obj_set_style_text_color(v, val_color, 0);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " Security Info — OPTIGA Trust M");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* อ่าน UID ชิป OPTIGA */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "อ่าน UID ชิป OPTIGA");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* ---- Chip Identity Card (top-left) ---- */
    lv_obj_t *id_card = create_section(parent, LV_SYMBOL_EYE_OPEN " Chip Identity",
                                        380, 160, 8, 32);

    add_kv_row(id_card, "Chip:", CHIP_NAME, COLOR_TEXT);
    add_kv_row(id_card, "Lifecycle:", LIFECYCLE, COLOR_SUCCESS);
    add_kv_row(id_card, "I2C Addr:", "0x30 (SCB0, 100 kHz)", COLOR_TEXT);

    lv_obj_t *uid_title = lv_label_create(id_card);
    lv_label_set_text(uid_title, "UID (0xE0C2):");
    lv_obj_set_style_text_color(uid_title, COLOR_ACCENT, 0);

    lv_obj_t *uid_val = lv_label_create(id_card);
    lv_label_set_text(uid_val, CHIP_UID_DEMO);
    lv_obj_set_style_text_color(uid_val, COLOR_WARNING, 0);
    lv_label_set_long_mode(uid_val, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(uid_val, 350);

    /* ---- Certificate Slots Card (top-right) ---- */
    lv_obj_t *cert_card = create_section(parent, LV_SYMBOL_DOWNLOAD " Certificate Slots",
                                          370, 160, 396, 32);

    for (int i = 0; i < NUM_CERT_SLOTS; i++) {
        lv_obj_t *row = lv_obj_create(cert_card);
        lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(row, 8, 0);

        lv_obj_t *slot = lv_label_create(row);
        lv_label_set_text(slot, CERT_SLOTS[i].slot);
        lv_obj_set_style_text_color(slot, COLOR_ACCENT, 0);

        lv_obj_t *type = lv_label_create(row);
        lv_label_set_text(type, CERT_SLOTS[i].type);
        lv_obj_set_style_text_color(type, COLOR_TEXT, 0);

        lv_obj_t *status = lv_label_create(row);
        lv_label_set_text(status, CERT_SLOTS[i].status);
        bool pre = (i == 0);
        lv_obj_set_style_text_color(status, pre ? COLOR_SUCCESS : COLOR_WARNING, 0);
    }

    /* ---- Supported Algorithms Card (bottom) ---- */
    lv_obj_t *algo_card = create_section(parent, LV_SYMBOL_SETTINGS " Supported Algorithms",
                                          758, 170, 8, 200);
    lv_obj_set_flex_flow(algo_card, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(algo_card, 8, 0);
    lv_obj_set_style_pad_row(algo_card, 6, 0);

    /* Re-add title since flex-row messes with it — title already added by helper */

    for (int i = 0; i < NUM_ALGORITHMS; i++) {
        lv_obj_t *chip = lv_obj_create(algo_card);
        lv_obj_set_size(chip, 230, 50);
        lv_obj_set_style_bg_color(chip, COLOR_BG, 0);
        lv_obj_set_style_border_color(chip, COLOR_ACCENT, 0);
        lv_obj_set_style_border_width(chip, 1, 0);
        lv_obj_set_style_radius(chip, 8, 0);
        lv_obj_set_style_pad_all(chip, 6, 0);
        lv_obj_clear_flag(chip, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *name = lv_label_create(chip);
        lv_label_set_text(name, ALGORITHMS[i].name);
        lv_obj_set_style_text_color(name, COLOR_SUCCESS, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_obj_t *detail = lv_label_create(chip);
        lv_label_set_text(detail, ALGORITHMS[i].detail);
        lv_obj_set_style_text_color(detail, COLOR_TEXT, 0);
        lv_obj_align(detail, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }
}
