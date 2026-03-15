/******************************************************************************
* File Name:   main.c
*
* Description: CM55 application with FreeRTOS + LVGL.
*              LVGL display on Waveshare 4.3" DSI LCD (800x480)
*              Uses TESAIoT display library for GFXSS/VGLite/LVGL init.
*              GFX task handles all LVGL + IPC initialization.
*
*              NOTE: CM55 must NOT use printf/retarget-io.
*              CM33_NS owns the UART for MicroPython REPL.
*
******************************************************************************/

#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tesaiot_display.h"
#include "bsp_feature_flags.h"
#if TESAIOT_ENABLE_FACE_RUNTIME
#include "face_mode_runtime.h"
#endif
#if BSP_HAS_RADAR
#include "radar_task.h"
#endif

/*******************************************************************************
* Macros
*******************************************************************************/
#define APP_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE * 8)
#define APP_TASK_PRIORITY       (configMAX_PRIORITIES - 2)
/* Temporary diagnosis toggles for hang isolation. */
#define TESAIOT_DIAG_DISABLE_RADAR_TASK   0
#define TESAIOT_DIAG_DISABLE_APP_TASK     0
/* Safety guard: keep default boot on SensorHub until native Face runtime
 * boot-path is fully stabilized in this merged firmware image. */
#define TESAIOT_ENABLE_FACE_RUNTIME_BOOT  0

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
#if !TESAIOT_DIAG_DISABLE_APP_TASK
static void app_task(void *arg);
#endif

/*******************************************************************************
* LVGL Tick Hook — called every 1ms from FreeRTOS tick ISR
* Required for LVGL timing (animations, input debounce, etc.)
*******************************************************************************/
void vApplicationTickHook(void)
{
#if TESAIOT_ENABLE_FACE_RUNTIME
    if (!face_mode_runtime_active()) {
        tesaiot_display_tick();
    }
#else
    tesaiot_display_tick();
#endif
}

#if !TESAIOT_DIAG_DISABLE_APP_TASK
/*******************************************************************************
* Function Name: app_task
* Summary: Waits for display + IPC init, then heartbeat LED.
*******************************************************************************/
static void app_task(void *arg)
{
    CY_UNUSED_PARAMETER(arg);

    /* Wait for display + IPC initialization to complete
     * tesaiot_display_ready: 0=pending, 1=display+IPC, 2=IPC-only (headless) */
    extern volatile uint8_t tesaiot_display_ready;
    uint32_t wait_count = 0;
    while (tesaiot_display_ready == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        wait_count++;
        if (wait_count > 150) {
            break;
        }
    }

    /* Heartbeat LED */
    for (;;)
    {
        Cy_GPIO_Inv(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif

/*******************************************************************************
* Function Name: main
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

#if TESAIOT_ENABLE_FACE_RUNTIME && TESAIOT_ENABLE_FACE_RUNTIME_BOOT
    /* Alternate boot mode: launch native Face-ID runtime. */
    if (face_mode_runtime_requested()) {
        face_mode_launch_runtime();
    }
#endif

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (CY_RSLT_SUCCESS != result)
    {
        for (;;) {
            Cy_GPIO_Inv(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
            Cy_SysLib_Delay(50);
        }
    }

    /* Enable global interrupts */
    __enable_irq();

    /* GFX task: GFXSS/LVGL init + IPC + sensorhub UI */
    BaseType_t xResult = tesaiot_display_init();
    if (pdPASS != xResult) {
        for (;;) {
            Cy_GPIO_Inv(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_PIN);
            Cy_SysLib_Delay(50);
        }
    }

    /* USB Host init moved to app_task (post-scheduler) — see app_task() */

#if BSP_HAS_RADAR && !TESAIOT_DIAG_DISABLE_RADAR_TASK
    xResult = xTaskCreate(tesaiot_radar_task, RADAR_TASK_NAME,
                          RADAR_TASK_STACK_SIZE, NULL,
                          RADAR_TASK_PRIORITY, NULL);
    if (pdPASS != xResult) {
        for (;;) {
            Cy_GPIO_Inv(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_PIN);
            Cy_SysLib_Delay(100);
        }
    }
#endif

#if !TESAIOT_DIAG_DISABLE_APP_TASK
    /* Optional app task (heartbeat only). Disabled in recovery mode to
     * reduce heap pressure during early boot. */
    xResult = xTaskCreate(app_task, "CM55_App",
                          APP_TASK_STACK_SIZE, NULL,
                          APP_TASK_PRIORITY, NULL);
    if (pdPASS != xResult) {
        for (;;) {
            Cy_GPIO_Inv(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
            Cy_GPIO_Inv(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_PIN);
            Cy_SysLib_Delay(100);
        }
    }
#endif

    /* Start the RTOS Scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;) {
        Cy_GPIO_Inv(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
        Cy_GPIO_Inv(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_PIN);
        Cy_SysLib_Delay(500);
    }
    return -1;
}

/*******************************************************************************
* FreeRTOS Hook Functions
*******************************************************************************/
/* Fault breadcrumb in fixed SRAM — survives until power cycle.
 * Read via J-Link: mem32 0x28000000 1 (or check LED pattern). */
#define FAULT_MARKER_ADDR  ((volatile uint32_t *)0x28000000)
#define FAULT_MARKER_STACK  0xDEAD0001
#define FAULT_MARKER_MALLOC 0xDEAD0002
#define FAULT_MARKER_HARD   0xDEAD0003

/* Blink LED N times, pause, repeat — each fault has a distinct count:
 *   1 blink  = Stack Overflow  (LED1)
 *   2 blinks = Malloc Failed   (LED2)
 *   3 blinks = HardFault       (LED1+LED2) */
static void fault_blink_pattern(uint8_t count, bool led1, bool led2)
{
    for (;;) {
        for (uint8_t i = 0; i < count; i++) {
            if (led1) Cy_GPIO_Clr(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
            if (led2) Cy_GPIO_Clr(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_PIN);
            for (volatile uint32_t d = 0; d < 300000; d++) {}
            if (led1) Cy_GPIO_Set(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
            if (led2) Cy_GPIO_Set(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_PIN);
            for (volatile uint32_t d = 0; d < 300000; d++) {}
        }
        for (volatile uint32_t d = 0; d < 1500000; d++) {}  /* Long pause */
    }
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    CY_UNUSED_PARAMETER(pxTask);
    CY_UNUSED_PARAMETER(pcTaskName);
    *FAULT_MARKER_ADDR = FAULT_MARKER_STACK;
    __disable_irq();
    fault_blink_pattern(1, true, false);  /* 1 blink LED1 */
}

void vApplicationMallocFailedHook(void)
{
    *FAULT_MARKER_ADDR = FAULT_MARKER_MALLOC;
    __disable_irq();
    fault_blink_pattern(2, false, true);  /* 2 blinks LED2 */
}

void HardFault_Handler(void)
{
    *FAULT_MARKER_ADDR = FAULT_MARKER_HARD;
    __disable_irq();
    fault_blink_pattern(3, true, true);   /* 3 blinks LED1+LED2 */
}
