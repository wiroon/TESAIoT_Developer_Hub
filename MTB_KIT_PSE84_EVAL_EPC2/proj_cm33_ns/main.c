/*******************************************************************************
 * File Name: main.c
 *
 * Description: CM33 Non-Secure FreeRTOS entry point.
 *              Creates MicroPython REPL task, boots CM55, starts scheduler.
 *
 *              Boot sequence:
 *                1. cybsp_init() + retarget-io (UART)
 *                2. Create MicroPython task
 *                3. Boot CM55 (display + camera + AI)
 *                4. Start FreeRTOS scheduler (never returns)
 *
 *              WiFi SDIO init is lazy — happens inside modwifi.c on first
 *              wifi.scan() or wifi.connect() call from Python.
 *
 * Author: TESAIoT (Asst.Prof.Santi Nuratch, Ph.D)
 * Version: 3.0 (FreeRTOS migration)
 *
 ******************************************************************************/

#include "cybsp.h"
#include "cy_sysclk.h"
#include "FreeRTOS.h"
#include "task.h"
#include "retarget_io_init.h"
#include "sensor_auto_task.h"
#include "ipc_hsm_handler.h"

/*******************************************************************************
 * MicroPython task entry point (defined in mpy_main.c, linked from
 * libmicropython.a)
 ******************************************************************************/
extern void mpy_task_entry(void *arg);

/*******************************************************************************
 * Task Configuration
 ******************************************************************************/
#define MPY_TASK_STACK_SIZE  (8 * 1024)   /* 8KB stack for deep REPL call chains */
#define MPY_TASK_PRIORITY    (3)          /* Mid priority: below WHD(5), above idle(0) */

/*******************************************************************************
 * CM55 Boot Configuration
 ******************************************************************************/
#define CM55_BOOT_WAIT_TIME_USEC    (10U)
#define CM55_APP_BOOT_ADDR          (CYMEM_CM33_0_m55_nvm_START + \
                                        CYBSP_MCUBOOT_HEADER_SIZE)

/*******************************************************************************
 * Function Name: main
 ******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (CY_RSLT_SUCCESS != result) {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io for UART serial (MicroPython REPL) */
    init_retarget_io();

#ifdef BOOT_VERBOSE
    printf("\r\n=====================================================\r\n");
    printf("PSoC Edge AI MicroPython + WiFi (CM33_NS) by TESAIoT\r\n");
    printf("FreeRTOS %s\r\n", tskKERNEL_VERSION_NUMBER);
    printf("=====================================================\r\n");
#endif

    /* Create MicroPython REPL task */
    BaseType_t xResult = xTaskCreate(
        mpy_task_entry,
        "MicroPython",
        MPY_TASK_STACK_SIZE / sizeof(StackType_t),
        NULL,
        MPY_TASK_PRIORITY,
        NULL
    );
    if (pdPASS != xResult) {
        printf("ERROR: Failed to create MicroPython task\r\n");
        CY_ASSERT(0);
    }

    /* Create sensor auto-push background task (reads sensors → IPC to CM55 LVGL) */
    sensor_auto_task_create();

    /* HSM Security page: IPC handler for OPTIGA Trust M data requests from CM55 */
    ipc_hsm_handler_init();
#ifdef BOOT_VERBOSE
    printf("[BOOT] ipc_hsm_handler OK\r\n");
#endif

    /* Enable GFXSS peripheral clocks from CM33_NS (required before CM55 can use them).
     * Our BSP's design.modus does not include GFXSS, so init_cycfg_peripherals()
     * does not enable these clocks. The official Infineon example BSP does include
     * GFXSS in its cycfg_peripherals.c — we replicate that here. */
    Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_GFXSS_GPU_PERI_NR,
                                 CY_MMIO_GFXSS_GPU_GROUP_NR,
                                 CY_MMIO_GFXSS_GPU_SLAVE_NR,
                                 CY_MMIO_GFXSS_GPU_CLK_HF_NR);
    Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_GFXSS_DC_PERI_NR,
                                 CY_MMIO_GFXSS_DC_GROUP_NR,
                                 CY_MMIO_GFXSS_DC_SLAVE_NR,
                                 CY_MMIO_GFXSS_DC_CLK_HF_NR);
    Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_GFXSS_MIPIDSI_PERI_NR,
                                 CY_MMIO_GFXSS_MIPIDSI_GROUP_NR,
                                 CY_MMIO_GFXSS_MIPIDSI_SLAVE_NR,
                                 CY_MMIO_GFXSS_MIPIDSI_CLK_HF_NR);

    /* Boot CM55 core (display + camera + AI + radar) */
#ifdef BOOT_VERBOSE
    printf("CM33_NS: Booting CM55...\r\n");
#endif
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);
#ifdef BOOT_VERBOSE
    printf("CM33_NS: CM55 boot initiated\r\n");
#endif

    /* Start FreeRTOS scheduler — never returns */
    vTaskStartScheduler();

    /* Should never reach here */
    CY_ASSERT(0);
    for (;;) {}
}

/*******************************************************************************
 * FreeRTOS Hook Functions
 ******************************************************************************/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pxTask;
    printf("FATAL: Stack overflow in task '%s'\r\n", pcTaskName);
    CY_ASSERT(0);
}

void vApplicationMallocFailedHook(void)
{
    printf("FATAL: Malloc failed (FreeRTOS heap exhausted)\r\n");
    CY_ASSERT(0);
}
