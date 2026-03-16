/*******************************************************************************
 * A15 — HSM Crypto Benchmark
 *
 * Production-derived OPTIGA Trust M crypto benchmark display for Developer Hub.
 * Adapted from page_hsm.c Tab 4 (Crypto Benchmark) in the TESAIoT firmware.
 *
 * Shows benchmark cards for each crypto operation:
 *   - SHA-256 Hash (256B input)
 *   - ECDSA Sign (ECC P-256, 32-byte digest)
 *   - ECDSA Verify (ECC P-256)
 *   - ECC P-256 Key Generation
 *   - Random (32 bytes from TRNG)
 *
 * Each operation has a "Run" button, result display, and timing.
 * Timer simulates benchmark execution (actual OPTIGA runs on CM33_NS).
 *
 * IPC commands documented (simulated in this example):
 *   IPC_CMD_HSM_BENCHMARK (0xD4) — run all 5 crypto benchmarks
 *   IPC_CMD_OPTIGA_SHA256 (0xE3) — SHA-256 hash
 *   IPC_CMD_OPTIGA_SIGN   (0xE4) — ECDSA signature
 *   IPC_CMD_OPTIGA_RANDOM (0xE2) — random bytes
 *
 * CY_SECTION_SHAREDMEM used for TX buffer in production.
 *
 * Entry point: void example_main(lv_obj_t *parent)
 *
 *******************************************************************************/

#include "example_common.h"

/*******************************************************************************
 * HSM Color Palette (from production page_hsm.c)
 *******************************************************************************/
#define HSM_COLOR_OK         0x50D890
#define HSM_COLOR_WARN       0xF2B84B
#define HSM_COLOR_CRIT       0xE85B5B
#define HSM_COLOR_BENCH      0x50D890
#define HSM_COLOR_TITLE      0xBB86FC

/*******************************************************************************
 * Benchmark Operation Definitions
 * Typical timings from Infineon SLS 32AIA datasheet.
 *******************************************************************************/
#define NUM_BENCH_OPS  5

typedef struct {
    const char *name;
    const char *ipc_cmd;
    uint16_t typical_ms;
    const char *result_sample;
} bench_op_def_t;

static const bench_op_def_t BENCH_OPS[NUM_BENCH_OPS] = {
    { "Random (32 B)",      "IPC_CMD_OPTIGA_RANDOM (0xE2)", 15,
      "7F2A91E4C83D05B6...3E8A" },
    { "SHA-256 (256 B)",    "IPC_CMD_OPTIGA_SHA256 (0xE3)", 30,
      "E3B0C44298FC1C14...B855" },
    { "ECC P-256 KeyGen",   "IPC_CMD_HSM_BENCHMARK (0xD4)", 80,
      "KeyPair OID 0xE0F1" },
    { "ECDSA Sign",         "IPC_CMD_OPTIGA_SIGN (0xE4)",   70,
      "3045022100A1B2C3...7D8E" },
    { "ECDSA Verify",       "IPC_CMD_HSM_BENCHMARK (0xD4)", 80,
      "Signature valid" },
};

/*******************************************************************************
 * Simulation State
 *******************************************************************************/
typedef struct {
    lv_obj_t *result_labels[NUM_BENCH_OPS];
    lv_obj_t *time_labels[NUM_BENCH_OPS];
    lv_obj_t *bars[NUM_BENCH_OPS];
    lv_obj_t *run_btns[NUM_BENCH_OPS];
    lv_obj_t *run_all_btn;
    lv_obj_t *status_label;
    lv_timer_t *timer;
    int current_op;
    int progress;
    bool running;
} bench_state_t;

static bench_state_t s_bench;

/*******************************************************************************
 * Simulation: single-op completion
 *******************************************************************************/
static void bench_complete_op(int idx)
{
    /* Simulate actual time: typical +/- 20% variance */
    uint16_t actual = BENCH_OPS[idx].typical_ms +
                      (uint16_t)(lv_rand(0, BENCH_OPS[idx].typical_ms / 5));

    lv_label_set_text(s_bench.result_labels[idx], BENCH_OPS[idx].result_sample);
    lv_obj_set_style_text_color(s_bench.result_labels[idx],
                                 lv_color_hex(HSM_COLOR_OK), 0);

    char buf[32];
    snprintf(buf, sizeof(buf), "%u ms", actual);
    lv_label_set_text(s_bench.time_labels[idx], buf);

    /* Color: green if within 2x typical, amber otherwise */
    uint32_t time_color = (actual <= BENCH_OPS[idx].typical_ms * 2)
                          ? HSM_COLOR_OK : HSM_COLOR_WARN;
    lv_obj_set_style_text_color(s_bench.time_labels[idx],
                                 lv_color_hex(time_color), 0);

    lv_bar_set_value(s_bench.bars[idx], 100, LV_ANIM_ON);
}

/*******************************************************************************
 * Timer: sequential benchmark execution
 *******************************************************************************/
static void bench_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (s_bench.current_op >= NUM_BENCH_OPS) {
        /* All done */
        lv_label_set_text(s_bench.status_label,
            "Benchmark complete. All operations measured.");
        lv_obj_set_style_text_color(s_bench.status_label,
                                     lv_color_hex(HSM_COLOR_OK), 0);
        s_bench.running = false;
        lv_obj_clear_flag(s_bench.run_all_btn, LV_OBJ_FLAG_HIDDEN);
        lv_timer_delete(s_bench.timer);
        s_bench.timer = NULL;
        return;
    }

    int idx = s_bench.current_op;

    /* Advance progress bar */
    s_bench.progress += 20;
    lv_bar_set_value(s_bench.bars[idx], s_bench.progress, LV_ANIM_ON);

    char buf[64];
    snprintf(buf, sizeof(buf), "Running: %s (%d%%)",
             BENCH_OPS[idx].name, s_bench.progress);
    lv_label_set_text(s_bench.status_label, buf);

    if (s_bench.progress >= 100) {
        bench_complete_op(idx);
        s_bench.current_op++;
        s_bench.progress = 0;
    }
}

/*******************************************************************************
 * Single "Run" button callback
 *******************************************************************************/
static void single_run_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (s_bench.running) return;

    /* Reset this op */
    lv_bar_set_value(s_bench.bars[idx], 0, LV_ANIM_OFF);
    lv_label_set_text(s_bench.result_labels[idx], "Running...");
    lv_obj_set_style_text_color(s_bench.result_labels[idx],
                                 lv_color_hex(0xFF9800), 0);
    lv_label_set_text(s_bench.time_labels[idx], "---");

    /* Simulate with a short delay then complete */
    bench_complete_op(idx);
}

/*******************************************************************************
 * "Run All" button callback
 *******************************************************************************/
static void run_all_cb(lv_event_t *e)
{
    (void)e;
    if (s_bench.running) return;

    /* Reset all */
    for (int i = 0; i < NUM_BENCH_OPS; i++) {
        lv_bar_set_value(s_bench.bars[i], 0, LV_ANIM_OFF);
        lv_label_set_text(s_bench.result_labels[i], "---");
        lv_obj_set_style_text_color(s_bench.result_labels[i],
                                     lv_color_hex(0xE0E0E0), 0);
        lv_label_set_text(s_bench.time_labels[i], "---");
        lv_obj_set_style_text_color(s_bench.time_labels[i],
                                     lv_color_hex(0x808080), 0);
    }

    s_bench.current_op = 0;
    s_bench.progress = 0;
    s_bench.running = true;
    lv_obj_add_flag(s_bench.run_all_btn, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(s_bench.status_label, "Starting benchmark...");
    lv_obj_set_style_text_color(s_bench.status_label,
                                 lv_color_hex(0xE0E0E0), 0);

    s_bench.timer = lv_timer_create(bench_timer_cb, 80, NULL);
}

/*******************************************************************************
 * Example Entry Point
 *******************************************************************************/
void example_main(lv_obj_t *parent)
{
    memset(&s_bench, 0, sizeof(s_bench));

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title,
        LV_SYMBOL_EYE_OPEN " OPTIGA Trust M - Crypto Benchmark");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(HSM_COLOR_BENCH), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* ทดสอบเข้ารหัส HSM */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "ทดสอบเข้ารหัส HSM");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* Status line */
    s_bench.status_label = lv_label_create(parent);
    lv_label_set_text(s_bench.status_label,
        "Tap Run All or individual Run buttons");
    lv_obj_set_style_text_color(s_bench.status_label,
                                 lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_pos(s_bench.status_label, 16, 30);

    /* Run All button */
    s_bench.run_all_btn = lv_btn_create(parent);
    lv_obj_set_size(s_bench.run_all_btn, 120, 32);
    lv_obj_align(s_bench.run_all_btn, LV_ALIGN_TOP_RIGHT, -16, 26);
    lv_obj_set_style_bg_color(s_bench.run_all_btn,
                               lv_color_hex(HSM_COLOR_BENCH), 0);
    lv_obj_set_style_radius(s_bench.run_all_btn, 8, 0);
    lv_obj_set_style_shadow_width(s_bench.run_all_btn, 0, 0);
    lv_obj_add_event_cb(s_bench.run_all_btn, run_all_cb,
                         LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(s_bench.run_all_btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_PLAY " Run All");
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(btn_lbl);

    /* Column header labels */
    int hdr_y = 60;
    lv_obj_t *h_op = lv_label_create(parent);
    lv_label_set_text(h_op, "Operation");
    lv_obj_set_style_text_color(h_op, lv_color_hex(0x808080), 0);
    lv_obj_set_pos(h_op, 16, hdr_y);

    lv_obj_t *h_typ = lv_label_create(parent);
    lv_label_set_text(h_typ, "Typical");
    lv_obj_set_style_text_color(h_typ, lv_color_hex(0x808080), 0);
    lv_obj_set_pos(h_typ, 180, hdr_y);

    lv_obj_t *h_act = lv_label_create(parent);
    lv_label_set_text(h_act, "Actual");
    lv_obj_set_style_text_color(h_act, lv_color_hex(0x808080), 0);
    lv_obj_set_pos(h_act, 240, hdr_y);

    lv_obj_t *h_res = lv_label_create(parent);
    lv_label_set_text(h_res, "Result");
    lv_obj_set_style_text_color(h_res, lv_color_hex(0x808080), 0);
    lv_obj_set_pos(h_res, 310, hdr_y);

    /* Benchmark operation cards */
    int card_y_start = 80;
    int card_h = 58;
    int card_gap = 4;

    for (int i = 0; i < NUM_BENCH_OPS; i++) {
        int y = card_y_start + i * (card_h + card_gap);
        char buf[32];

        /* Card background */
        lv_obj_t *card = lv_obj_create(parent);
        lv_obj_set_size(card, DISPLAY_WIDTH - 16, card_h);
        lv_obj_set_pos(card, 8, y);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x0D1B2A), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x2A3A5C), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_pad_all(card, 8, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        /* Operation name */
        lv_obj_t *op_lbl = lv_label_create(card);
        lv_label_set_text(op_lbl, BENCH_OPS[i].name);
        lv_obj_set_style_text_color(op_lbl, lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_text_font(op_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(op_lbl, 0, 0);

        /* IPC command reference (small, dimmed) */
        lv_obj_t *ipc_lbl = lv_label_create(card);
        lv_label_set_text(ipc_lbl, BENCH_OPS[i].ipc_cmd);
        lv_obj_set_style_text_color(ipc_lbl, lv_color_hex(0x606060), 0);
        lv_obj_set_pos(ipc_lbl, 0, 18);

        /* Typical time */
        snprintf(buf, sizeof(buf), "%u ms", BENCH_OPS[i].typical_ms);
        lv_obj_t *typ_lbl = lv_label_create(card);
        lv_label_set_text(typ_lbl, buf);
        lv_obj_set_style_text_color(typ_lbl, lv_color_hex(0x808080), 0);
        lv_obj_set_pos(typ_lbl, 172, 0);

        /* Actual time (filled after run) */
        s_bench.time_labels[i] = lv_label_create(card);
        lv_label_set_text(s_bench.time_labels[i], "---");
        lv_obj_set_style_text_color(s_bench.time_labels[i],
                                     lv_color_hex(0x808080), 0);
        lv_obj_set_pos(s_bench.time_labels[i], 232, 0);

        /* Result label */
        s_bench.result_labels[i] = lv_label_create(card);
        lv_label_set_text(s_bench.result_labels[i], "---");
        lv_obj_set_style_text_color(s_bench.result_labels[i],
                                     lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_pos(s_bench.result_labels[i], 302, 0);

        /* Progress bar */
        s_bench.bars[i] = lv_bar_create(card);
        lv_obj_set_size(s_bench.bars[i], 280, 6);
        lv_bar_set_range(s_bench.bars[i], 0, 100);
        lv_bar_set_value(s_bench.bars[i], 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_bench.bars[i],
                                   lv_color_hex(0x1A3050), 0);
        lv_obj_set_style_bg_color(s_bench.bars[i],
                                   lv_color_hex(HSM_COLOR_BENCH),
                                   LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_bench.bars[i], 3, 0);
        lv_obj_set_style_radius(s_bench.bars[i], 3, LV_PART_INDICATOR);
        lv_obj_set_pos(s_bench.bars[i], 0, 36);

        /* Individual Run button */
        s_bench.run_btns[i] = lv_btn_create(card);
        lv_obj_set_size(s_bench.run_btns[i], 60, 24);
        lv_obj_set_style_bg_color(s_bench.run_btns[i],
                                   lv_color_hex(0x2A3A5C), 0);
        lv_obj_set_style_radius(s_bench.run_btns[i], 6, 0);
        lv_obj_set_style_shadow_width(s_bench.run_btns[i], 0, 0);
        lv_obj_align(s_bench.run_btns[i], LV_ALIGN_TOP_RIGHT, 0, -4);
        lv_obj_add_event_cb(s_bench.run_btns[i], single_run_cb,
                             LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *rb = lv_label_create(s_bench.run_btns[i]);
        lv_label_set_text(rb, "Run");
        lv_obj_set_style_text_color(rb, lv_color_hex(0xE0E0E0), 0);
        lv_obj_center(rb);
    }

    /* Summary note at bottom */
    lv_obj_t *note = lv_label_create(parent);
    lv_label_set_text(note,
        "Typical = Infineon SLS 32AIA datasheet. "
        "Actual = simulated (real measurement via IPC_CMD_HSM_BENCHMARK on CM33_NS).");
    lv_obj_set_style_text_color(note, lv_color_hex(0x606060), 0);
    lv_obj_set_pos(note, 16, card_y_start + NUM_BENCH_OPS * (card_h + card_gap) + 4);
    lv_obj_set_width(note, DISPLAY_WIDTH - 32);
    lv_label_set_long_mode(note, LV_LABEL_LONG_WRAP);
}
