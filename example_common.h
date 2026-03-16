/**
 * example_common.h — Shared header for all TESAIoT Developer Hub C examples
 *
 * This header provides the common includes, macros, and forward declarations
 * needed by every example. Each example's main_example.c includes this file
 * and implements: void example_main(lv_obj_t *parent)
 *
 * Board:   PSoC Edge E84 (KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2)
 * MCU:     Cortex-M55 (display/UI) + Cortex-M33 (sensors/WiFi)
 * Display: 480×800 AMOLED, LVGL 9.2
 * RTOS:    FreeRTOS 10.6
 */

#ifndef EXAMPLE_COMMON_H
#define EXAMPLE_COMMON_H

/* ── LVGL 9.2 ─────────────────────────────────────────────────────── */
#include "lvgl.h"

/* ── Infineon HAL & BSP ───────────────────────────────────────────── */
#include "cybsp.h"
#include "cyhal.h"

/* ── FreeRTOS ─────────────────────────────────────────────────────── */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

/* ── Standard C ───────────────────────────────────────────────────── */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

/* ── BSP Feature Flags ────────────────────────────────────────────── */
/* These are defined per-project in bsp_feature_flags.h:
 *   BSP_HAS_DPS368         — Barometric pressure sensor (AI Kit only)
 *   BSP_HAS_SHT40          — Humidity/temp sensor (AI Kit only)
 *   BSP_HAS_CAPSENSE       — CapSense touch buttons (Eva Kit only)
 *   BSP_HAS_POTENTIOMETER  — SAR ADC potentiometer (Eva Kit only)
 *   BSP_HAS_BMI270         — IMU accel/gyro (all boards)
 *   BSP_HAS_BMM350         — Magnetometer (AI Kit + Eva Kit)
 */
#include "bsp_feature_flags.h"

/* ── Sensor I2C Common ────────────────────────────────────────────── */
#include "sensor_i2c.h"

/* ── Sensor Hub IPC (CM55 reads sensor data via IPC from CM33) ──── */
#include "ipc_sensorhub.h"

/* ── Display Constants ────────────────────────────────────────────── */
#define DISPLAY_WIDTH   480
#define DISPLAY_HEIGHT  800

/* ── Common Color Palette (from tesaiot_ui_theme.h) ───────────────── */
#define UI_COLOR_PRIMARY     lv_color_hex(0x00BCD4)    /* Cyan accent       */
#define UI_COLOR_CARD_BG     lv_color_hex(0x142240)    /* Dark card bg      */
#define UI_COLOR_TEXT        lv_color_hex(0xE0E0E0)    /* Light text        */
#define UI_COLOR_TEXT_DIM    lv_color_hex(0x808080)    /* Dimmed text       */
#define UI_COLOR_SUCCESS     lv_color_hex(0x4CAF50)    /* Green             */
#define UI_COLOR_WARNING     lv_color_hex(0xFF9800)    /* Orange            */
#define UI_COLOR_ERROR       lv_color_hex(0xF44336)    /* Red               */
#define UI_COLOR_INFO        lv_color_hex(0x2196F3)    /* Blue              */
#define UI_COLOR_BMI270      lv_color_hex(0x4CAF50)    /* IMU — green       */
#define UI_COLOR_DPS368      lv_color_hex(0xFF9800)    /* Baro — orange     */
#define UI_COLOR_SHT40       lv_color_hex(0x2196F3)    /* Climate — blue    */
#define UI_COLOR_BMM350      lv_color_hex(0xE040FB)    /* Compass — purple  */
#define UI_COLOR_RADAR       lv_color_hex(0xFF1744)    /* Radar — red       */

/* ── Golden Ratio Layout ──────────────────────────────────────────── */
#define UI_PHI_MAJOR         0.618f
#define UI_PHI_MINOR         0.382f
#define UI_CARD_RADIUS       12
#define UI_CARD_BORDER       1
#define UI_CARD_SHADOW       8
#define UI_CARD_PAD          12

/* ── Common GPIO Pins (KIT_PSE84_AI) ─────────────────────────────── */
/* Adjust per board BSP — these are the most common pins */
#define PIN_LED_RED       P13_7
#define PIN_LED_GREEN     P13_4
#define PIN_LED_BLUE      P13_3
#define PIN_BUTTON_SW1    P5_2
#define PIN_BUTTON_SW2    P5_3

/* ── Example Entry Point ──────────────────────────────────────────── */
/**
 * Each example implements this function.
 * @param parent  LVGL parent object (typically the active screen or a container)
 *
 * The example framework calls this after LVGL and the display are initialized.
 * The parent is a full-screen container (480×800) with dark background.
 */
void example_main(lv_obj_t *parent);

/* ── Helper: Create a standard card container ─────────────────────── */
static inline lv_obj_t *example_card_create(lv_obj_t *parent, int w, int h, lv_color_t bg_color)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, bg_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, UI_CARD_RADIUS, 0);
    lv_obj_set_style_border_width(card, UI_CARD_BORDER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_shadow_width(card, UI_CARD_SHADOW, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(card, UI_CARD_PAD, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/* ── Helper: Create a label with style ────────────────────────────── */
static inline lv_obj_t *example_label_create(lv_obj_t *parent, const char *text,
                                              const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

#endif /* EXAMPLE_COMMON_H */
