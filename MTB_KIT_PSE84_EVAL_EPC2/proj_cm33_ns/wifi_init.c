/*******************************************************************************
 * File Name: wifi_init.c
 *
 * Description: WiFi SDIO + WCM initialization for CM33_NS.
 *              Adapted from PSoC-Edge-AI-TESAIoT-RTSP-Server/proj_cm33_ns/
 *              rtsp_server_task.c (app_sdio_init, lines 220-269).
 *
 *              SDHC0 is PPC-protected for CM33_NS only. This file provides
 *              lazy WiFi init — called from modwifi.c on first wifi use.
 *
 * Author: TESAIoT
 *
 ******************************************************************************/

#include "wifi_init.h"
#include "cybsp.h"
#include "cy_wcm.h"
#include "cy_sysint.h"
#include "mtb_hal_sdio.h"
#include "cy_sd_host.h"

#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Configuration
 ******************************************************************************/
#define WIFI_SDIO_INTERRUPT_PRIORITY      (7U)
#define WIFI_HOST_WAKE_INTERRUPT_PRIORITY  (2U)
#define WIFI_SDIO_FREQUENCY_HZ            (25000000U)

/*******************************************************************************
 * Private Variables
 ******************************************************************************/
static mtb_hal_sdio_t sdio_instance;
static cy_stc_sd_host_context_t sdhc_host_context;
static cy_wcm_config_t wcm_config;
static bool wifi_ready = false;

/*******************************************************************************
 * Interrupt Handlers
 ******************************************************************************/
static void sdio_interrupt_handler(void)
{
    mtb_hal_sdio_process_interrupt(&sdio_instance);
}

static void host_wake_interrupt_handler(void)
{
    mtb_hal_gpio_process_interrupt(&wcm_config.wifi_host_wake_pin);
}

/*******************************************************************************
 * Function Name: app_wifi_init
 ******************************************************************************/
cy_rslt_t app_wifi_init(void)
{
    if (wifi_ready) {
        return CY_RSLT_SUCCESS;
    }

    cy_rslt_t result;
    mtb_hal_sdio_cfg_t sdio_hal_cfg;

#ifdef BOOT_VERBOSE
    printf("[WiFi] Initializing SDIO...\r\n");
#endif

    /* SDIO interrupt */
    cy_stc_sysint_t sdio_intr_cfg = {
        .intrSrc = CYBSP_WIFI_SDIO_IRQ,
        .intrPriority = WIFI_SDIO_INTERRUPT_PRIORITY,
    };

    /* Host wake interrupt */
    cy_stc_sysint_t host_wake_intr_cfg = {
        .intrSrc = CYBSP_WIFI_HOST_WAKE_IRQ,
        .intrPriority = WIFI_HOST_WAKE_INTERRUPT_PRIORITY,
    };

    /* Initialize SDIO interrupt */
    if (CY_SYSINT_SUCCESS != Cy_SysInt_Init(&sdio_intr_cfg,
                                              sdio_interrupt_handler)) {
        printf("[WiFi] ERROR: SDIO interrupt init failed\r\n");
        return CY_RSLT_TYPE_ERROR;
    }
    NVIC_EnableIRQ(CYBSP_WIFI_SDIO_IRQ);

    /* Setup SDIO HAL */
    result = mtb_hal_sdio_setup(&sdio_instance,
                                 &CYBSP_WIFI_SDIO_sdio_hal_config,
                                 NULL, &sdhc_host_context);
    if (CY_RSLT_SUCCESS != result) {
        printf("[WiFi] ERROR: SDIO setup failed (0x%08lX)\r\n",
               (unsigned long)result);
        return result;
    }

    /* Enable and initialize SD Host */
    Cy_SD_Host_Enable(CYBSP_WIFI_SDIO_HW);
    Cy_SD_Host_Init(CYBSP_WIFI_SDIO_HW,
                    CYBSP_WIFI_SDIO_sdio_hal_config.host_config,
                    &sdhc_host_context);
    Cy_SD_Host_SetHostBusWidth(CYBSP_WIFI_SDIO_HW, CY_SD_HOST_BUS_WIDTH_4_BIT);

    /* Configure SDIO frequency and block size */
    sdio_hal_cfg.frequencyhal_hz = WIFI_SDIO_FREQUENCY_HZ;
    sdio_hal_cfg.block_size = 64U;  /* 64-byte SDIO block size */
    mtb_hal_sdio_configure(&sdio_instance, &sdio_hal_cfg);

    /* Setup WiFi GPIOs */
    mtb_hal_gpio_setup(&wcm_config.wifi_wl_pin,
                       CYBSP_WIFI_WL_REG_ON_PORT_NUM,
                       CYBSP_WIFI_WL_REG_ON_PIN);
    mtb_hal_gpio_setup(&wcm_config.wifi_host_wake_pin,
                       CYBSP_WIFI_HOST_WAKE_PORT_NUM,
                       CYBSP_WIFI_HOST_WAKE_PIN);

    /* Initialize host wake interrupt */
    if (CY_SYSINT_SUCCESS != Cy_SysInt_Init(&host_wake_intr_cfg,
                                              host_wake_interrupt_handler)) {
        printf("[WiFi] ERROR: Host wake interrupt init failed\r\n");
        return CY_RSLT_TYPE_ERROR;
    }
    NVIC_EnableIRQ(CYBSP_WIFI_HOST_WAKE_IRQ);

#ifdef BOOT_VERBOSE
    printf("[WiFi] SDIO ready. Initializing WCM...\r\n");
#endif

    /* Configure WCM with SDIO instance */
    memset(&wcm_config, 0, sizeof(wcm_config));
    /* Re-setup GPIO after memset cleared them */
    mtb_hal_gpio_setup(&wcm_config.wifi_wl_pin,
                       CYBSP_WIFI_WL_REG_ON_PORT_NUM,
                       CYBSP_WIFI_WL_REG_ON_PIN);
    mtb_hal_gpio_setup(&wcm_config.wifi_host_wake_pin,
                       CYBSP_WIFI_HOST_WAKE_PORT_NUM,
                       CYBSP_WIFI_HOST_WAKE_PIN);
    wcm_config.interface = CY_WCM_INTERFACE_TYPE_STA;
    wcm_config.wifi_interface_instance = &sdio_instance;

    /* Initialize WiFi Connection Manager (creates WHD thread internally) */
    result = cy_wcm_init(&wcm_config);
    if (CY_RSLT_SUCCESS != result) {
        printf("[WiFi] ERROR: WCM init failed (0x%08lX)\r\n",
               (unsigned long)result);
        return result;
    }

    wifi_ready = true;
#ifdef BOOT_VERBOSE
    printf("[WiFi] Ready!\r\n");
#endif
    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
 * Function Name: app_wifi_deinit
 ******************************************************************************/
void app_wifi_deinit(void)
{
    if (wifi_ready) {
        cy_wcm_deinit();
        wifi_ready = false;
        printf("[WiFi] Deinitialized\r\n");
    }
}

/*******************************************************************************
 * Function Name: app_wifi_is_ready
 ******************************************************************************/
bool app_wifi_is_ready(void)
{
    return wifi_ready;
}
