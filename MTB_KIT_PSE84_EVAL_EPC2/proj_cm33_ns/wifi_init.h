/*******************************************************************************
 * File Name: wifi_init.h
 *
 * Description: WiFi SDIO + WCM initialization for CM33_NS.
 *              SDHC0 is PPC-protected for CM33_NS only.
 *              Lazy init: called on first wifi.scan() or wifi.connect().
 *
 * Author: TESAIoT
 *
 ******************************************************************************/

#ifndef WIFI_INIT_H
#define WIFI_INIT_H

#include "cy_result.h"
#include <stdbool.h>

/**
 * Initialize WiFi SDIO hardware and WiFi Connection Manager (WCM).
 * Includes: SDIO interrupt setup, SD Host init, GPIO setup (WL_REG_ON, HOST_WAKE),
 * and cy_wcm_init().
 *
 * Safe to call multiple times — returns immediately if already initialized.
 *
 * Must be called from a FreeRTOS task context (WCM creates internal threads).
 *
 * @return CY_RSLT_SUCCESS on success, error code on failure.
 */
cy_rslt_t app_wifi_init(void);

/**
 * Deinitialize WiFi (for soft reboot cleanup).
 * Calls cy_wcm_deinit().
 */
void app_wifi_deinit(void);

/**
 * Check if WiFi has been initialized.
 * @return true if app_wifi_init() has succeeded.
 */
bool app_wifi_is_ready(void);

#endif /* WIFI_INIT_H */
