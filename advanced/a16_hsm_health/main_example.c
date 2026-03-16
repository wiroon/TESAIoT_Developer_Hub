/*******************************************************************************
 * A14 — HSM Health Status Dashboard
 *
 * Production-derived HSM health status display for Developer Hub.
 * Adapted from page_hsm.c Tab 1 (Health/Status) in the TESAIoT firmware.
 *
 * Shows OPTIGA Trust M health status dashboard:
 *   - Chip UID display (27-byte hex string)
 *   - Lifecycle state indicator (Creation/Init/Operational/Terminated)
 *   - Key store slot status (4 slots with indicators)
 *   - Certificate presence (green/red dots per slot)
 *   - Random number test: simulated 16 random bytes display
 *   - IPC command reference documentation
 *
 * IPC commands documented (simulated in this example):
 *   IPC_CMD_HSM_REQUEST (0xD0) — read all OPTIGA data
 *   IPC_CMD_HSM_HEALTH  (0xD5) — run 8-point health check
 *
 * CY_SECTION_SHAREDMEM used for TX buffer in production.
 *
 * Entry point: void example_main(lv_obj_t *parent)
 *
 *******************************************************************************/

#include "example_common.h"

/*******************************************************************************
 * HSM Color Palette (Astro UX status system from production)
 *******************************************************************************/
#define HSM_COLOR_OK         0x50D890
#define HSM_COLOR_WARN       0xF2B84B
#define HSM_COLOR_CRIT       0xE85B5B
#define HSM_COLOR_CHIP       0x71C7EC
#define HSM_COLOR_CERT       0x448AFF
#define HSM_COLOR_KEY        0xFFD740
#define HSM_COLOR_TITLE      0xBB86FC

/*******************************************************************************
 * Simulated OPTIGA data (matches real production response layout)
 *******************************************************************************/
static const char *SIM_UID =
    "CD1633BF0201001C0005000A"
    "084F50544947412854442900070061";
static const char *SIM_FW_VER    = "v2.15.2801";
static const char *SIM_LCS      = "Operational (0x07)";

/* Health check test names (8 tests, from production HSM_HEALTH_TOTAL_LEN=8) */
static const char * const HEALTH_TESTS[] = {
    "HW Init", "UID Read", "Cert Read", "RNG",
    "SHA-256", "ECC Sign", "Data R/W", "Metadata"
};
#define NUM_HEALTH_TESTS  8

/* Certificate slot info */
static const struct { const char *oid; const char *status; bool present; } CERT_SLOTS[] = {
    { "0xE0E0", "Pre-provisioned", true  },
    { "0xE0E1", "Available",       true  },
    { "0xE0E2", "Available",       true  },
    { "0xE0E3", "Empty",           false },
};
#define NUM_CERT_SLOTS  4

/* Key store slots */
static const struct { const char *oid; const char *desc; uint32_t color; } KEY_SLOTS[] = {
    { "0xE0F0", "Device Private Key", HSM_COLOR_CRIT },
    { "0xE0F1", "ECC P-256 Session",  HSM_COLOR_OK   },
    { "0xE0F2", "ECC P-256 Spare",    HSM_COLOR_OK   },
    { "0xE0F3", "ECC P-384 Spare",    HSM_COLOR_WARN },
};
#define NUM_KEY_SLOTS  4

/*******************************************************************************
 * Helper: status dot indicator
 *******************************************************************************/
static lv_obj_t *create_dot(lv_obj_t *parent, uint32_t color)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_shadow_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    return dot;
}

/*******************************************************************************
 * Helper: card with title
 *******************************************************************************/
static lv_obj_t *create_card(lv_obj_t *parent, const char *title_text,
                              uint32_t accent_color, int w, int h,
                              int x, int y)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(accent_color), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, UI_CARD_RADIUS, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 4, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_color(title, lv_color_hex(accent_color), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    return card;
}

/*******************************************************************************
 * Helper: key-value row
 *******************************************************************************/
static void add_kv(lv_obj_t *parent, const char *key, const char *value,
                    uint32_t val_color)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *k = lv_label_create(row);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, lv_color_hex(0x00BCD4), 0);

    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, value);
    lv_obj_set_style_text_color(v, lv_color_hex(val_color), 0);
}

/*******************************************************************************
 * Example Entry Point
 *******************************************************************************/
void example_main(lv_obj_t *parent)
{
    int col_w = 230;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " HSM Health Status");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* สุขภาพ HSM */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "สุขภาพ HSM");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* ---- Column 1: Chip Identity ---- */
    lv_obj_t *chip_card = create_card(parent,
        LV_SYMBOL_EYE_OPEN " Chip Identity",
        HSM_COLOR_CHIP, col_w, 180, 8, 32);

    add_kv(chip_card, "Type:", "OPTIGA Trust M V3", 0xE0E0E0);
    add_kv(chip_card, "FW:", SIM_FW_VER, 0xE0E0E0);
    add_kv(chip_card, "LCS:", SIM_LCS, HSM_COLOR_OK);
    add_kv(chip_card, "I2C:", "0x30, SCB0, 100kHz", 0xE0E0E0);

    lv_obj_t *uid_lbl = lv_label_create(chip_card);
    lv_label_set_text(uid_lbl, "UID (0xE0C2):");
    lv_obj_set_style_text_color(uid_lbl, lv_color_hex(0x00BCD4), 0);

    lv_obj_t *uid_val = lv_label_create(chip_card);
    lv_label_set_text(uid_val, SIM_UID);
    lv_obj_set_style_text_color(uid_val, lv_color_hex(0xFF9800), 0);
    lv_label_set_long_mode(uid_val, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(uid_val, col_w - 24);

    /* ---- Column 2: Health Check (8 tests) ---- */
    lv_obj_t *health_card = create_card(parent,
        LV_SYMBOL_OK " Health Check (8/8 PASS)",
        HSM_COLOR_OK, col_w, 180, col_w + 16, 32);

    /* Simulate all 8 health tests passing */
    for (int i = 0; i < NUM_HEALTH_TESTS; i++) {
        lv_obj_t *row = lv_obj_create(health_card);
        lv_obj_set_size(row, lv_pct(100), 16);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        /* Simulate: first 7 pass, last one fails (for demo variety) */
        bool pass = (i < 7);
        create_dot(row, pass ? HSM_COLOR_OK : HSM_COLOR_CRIT);

        lv_obj_t *nm = lv_label_create(row);
        lv_label_set_text(nm, HEALTH_TESTS[i]);
        lv_obj_set_style_text_color(nm,
            lv_color_hex(pass ? 0xE0E0E0 : HSM_COLOR_CRIT), 0);

        lv_obj_t *st = lv_label_create(row);
        lv_label_set_text(st, pass ? "PASS" : "FAIL");
        lv_obj_set_style_text_color(st,
            lv_color_hex(pass ? HSM_COLOR_OK : HSM_COLOR_CRIT), 0);
    }

    /* ---- Row 2, Col 1: Certificate Slots ---- */
    lv_obj_t *cert_card = create_card(parent,
        LV_SYMBOL_DOWNLOAD " Certificate Slots",
        HSM_COLOR_CERT, col_w, 130, 8, 220);

    for (int i = 0; i < NUM_CERT_SLOTS; i++) {
        lv_obj_t *row = lv_obj_create(cert_card);
        lv_obj_set_size(row, lv_pct(100), 16);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        create_dot(row, CERT_SLOTS[i].present ? HSM_COLOR_OK : HSM_COLOR_CRIT);

        lv_obj_t *oid = lv_label_create(row);
        lv_label_set_text(oid, CERT_SLOTS[i].oid);
        lv_obj_set_style_text_color(oid, lv_color_hex(0x00BCD4), 0);

        lv_obj_t *st = lv_label_create(row);
        lv_label_set_text(st, CERT_SLOTS[i].status);
        lv_obj_set_style_text_color(st, lv_color_hex(
            CERT_SLOTS[i].present ? HSM_COLOR_OK : HSM_COLOR_WARN), 0);
    }

    /* ---- Row 2, Col 2: Key Store Slots ---- */
    lv_obj_t *key_card = create_card(parent,
        LV_SYMBOL_EDIT " Key Store Slots",
        HSM_COLOR_KEY, col_w, 130, col_w + 16, 220);

    for (int i = 0; i < NUM_KEY_SLOTS; i++) {
        lv_obj_t *row = lv_obj_create(key_card);
        lv_obj_set_size(row, lv_pct(100), 16);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        create_dot(row, KEY_SLOTS[i].color);

        lv_obj_t *oid = lv_label_create(row);
        lv_label_set_text(oid, KEY_SLOTS[i].oid);
        lv_obj_set_style_text_color(oid, lv_color_hex(0x00BCD4), 0);

        lv_obj_t *desc = lv_label_create(row);
        lv_label_set_text(desc, KEY_SLOTS[i].desc);
        lv_obj_set_style_text_color(desc, lv_color_hex(0xE0E0E0), 0);
    }

    /* ---- IPC Reference ---- */
    lv_obj_t *ipc_card = create_card(parent,
        LV_SYMBOL_SETTINGS " IPC Command Reference",
        HSM_COLOR_TITLE, DISPLAY_WIDTH - 16, 50, 8, 358);

    lv_obj_t *ipc_info = lv_label_create(ipc_card);
    lv_label_set_text(ipc_info,
        "IPC_CMD_HSM_REQUEST (0xD0) : Read UID + LCS + Certs    |    "
        "IPC_CMD_HSM_HEALTH (0xD5) : 8-point health check    |    "
        "CY_SECTION_SHAREDMEM for TX/RX buffers");
    lv_obj_set_style_text_color(ipc_info, lv_color_hex(0xB0B0B0), 0);
    lv_label_set_long_mode(ipc_info, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ipc_info, DISPLAY_WIDTH - 40);
}
