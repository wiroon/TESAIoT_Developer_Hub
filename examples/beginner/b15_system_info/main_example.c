/**
 * @file main_example.c
 * @brief B15 — System Info: Display board name, uptime, and tick count.
 *
 * Shows embedded system information using FreeRTOS APIs and compile-time
 * macros. Uptime and tick count update every second.
 */

#include "example_common.h"

static lv_obj_t *uptime_label;
static lv_obj_t *tick_label;
static lv_obj_t *heap_label;

static void info_timer_cb(lv_timer_t *t)
{
    (void)t;

    /* Uptime from FreeRTOS tick count */
    uint32_t ticks   = xTaskGetTickCount();
    uint32_t seconds = ticks / configTICK_RATE_HZ;
    uint32_t hours   = seconds / 3600;
    uint32_t mins    = (seconds % 3600) / 60;
    uint32_t secs    = seconds % 60;

    lv_label_set_text_fmt(uptime_label, "Uptime: %02"PRIu32":%02"PRIu32":%02"PRIu32,
                          hours, mins, secs);
    lv_label_set_text_fmt(tick_label, "Ticks: %"PRIu32, ticks);

    /* Free heap (FreeRTOS) */
    size_t free_heap = xPortGetFreeHeapSize();
    lv_label_set_text_fmt(heap_label, "Free heap: %u bytes", (unsigned)free_heap);
}

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* ---- title ---- */
    example_label_create(parent, "System Info",
                         &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* ---- info card ---- */
    lv_obj_t *card = example_card_create(parent, 500, 250,
                                         lv_color_hex(0x1E3A5F));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_set_style_pad_gap(card, 12, 0);

    /* ---- static info ---- */
    example_label_create(card, "Board: BENTO PSoC Edge E84",
                         &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    example_label_create(card, "Core: CM55 (Cortex-M55)",
                         &lv_font_montserrat_20, UI_COLOR_TEXT);
    example_label_create(card, "RTOS: FreeRTOS",
                         &lv_font_montserrat_20, UI_COLOR_TEXT);

    /* ---- dynamic info ---- */
    uptime_label = example_label_create(card, "Uptime: 00:00:00",
                                        &lv_font_montserrat_20,
                                        UI_COLOR_SUCCESS);
    tick_label = example_label_create(card, "Ticks: 0",
                                      &lv_font_montserrat_20,
                                      lv_color_hex(0x888888));
    heap_label = example_label_create(card, "Free heap: -- bytes",
                                      &lv_font_montserrat_20,
                                      UI_COLOR_WARNING);

    /* ---- 1-second update ---- */
    lv_timer_create(info_timer_cb, 1000, NULL);
}
