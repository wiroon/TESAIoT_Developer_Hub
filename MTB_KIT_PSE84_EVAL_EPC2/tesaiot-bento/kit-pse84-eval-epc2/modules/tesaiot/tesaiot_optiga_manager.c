/**
 * SPDX-FileCopyrightText: 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
 *
 * \author TESAIoT Platform Developer Team
 * \author Infineon Technologies AG (OPTIGA Trust M integration support)
 *
 * \file tesaiot_optiga_manager.c
 *
 * \brief OPTIGA Trust M Persistent Instance Manager Implementation.
 * Singleton pattern with FreeRTOS mutex for thread-safe OPTIGA instance management.
 *
 * NOTE: Only util instance is kept persistent. Crypt operations should use
 * lock/unlock pattern with local create/destroy to match OPTIGA library expectations.
 *
 * This file is part of the TESAIoT AIoT Foundation Platform, developed in
 * collaboration with Infineon Technologies AG for PSoC Edge E84 + OPTIGA Trust M.
 *
 * Contact: Assoc. Prof. Wiroon Sriborrirux <sriborrirux@gmail.com>/<wiroon@tesa.or.th>
 *
 * \ingroup TESAIoT
 */

#include "tesaiot_optiga_core.h"
#include "tesaiot_config.h"
#include <stdio.h>

/******************************************************************************
* Global Variables
*******************************************************************************/

/* Single persistent OPTIGA util instance (created once, reused forever) */
static optiga_util_t *g_optiga_instance = NULL;

/* Mutex for thread-safe access to OPTIGA */
static SemaphoreHandle_t g_optiga_mutex = NULL;

/* Initialization flag */
static bool g_optiga_initialized = false;

/******************************************************************************
* Function: optiga_manager_init
*******************************************************************************/
bool optiga_manager_init(callback_handler_t callback, void *context)
{
 /* Check if already initialized */
 if (g_optiga_initialized) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Already initialized (instance=%p)\n", LABEL_OPTIGA_MANAGER, g_optiga_instance);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return true;
 }

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Initializing persistent instance...\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 fflush(stdout);

 /* Create mutex first */
 g_optiga_mutex = xSemaphoreCreateMutex();
 if (!g_optiga_mutex) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s ERROR: Failed to create mutex!\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return false;
 }

 /* Create OPTIGA util instance (once) */
 g_optiga_instance = optiga_util_create(0, callback, context);
 if (!g_optiga_instance) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s ERROR: optiga_util_create() failed!\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 vSemaphoreDelete(g_optiga_mutex);
 g_optiga_mutex = NULL;
 return false;
 }

 /* NOTE: Crypt instance is NOT created here - it should be created/destroyed
  * locally for each operation using optiga_manager_lock/unlock for serialization.
  * This matches the OPTIGA library's expected usage pattern. */

 g_optiga_initialized = true;
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Persistent util instance created: %p\n", LABEL_OPTIGA_MANAGER, g_optiga_instance);
 printf("%s Mutex created: %p\n", LABEL_OPTIGA_MANAGER, g_optiga_mutex);
 printf("%s NOTE: Crypt operations should use lock/unlock with local instance\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 fflush(stdout);

 return true;
}

/******************************************************************************
* Function: optiga_manager_acquire
*******************************************************************************/
optiga_util_t* optiga_manager_acquire(void)
{
 if (!g_optiga_initialized || !g_optiga_instance || !g_optiga_mutex) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s ERROR: Not initialized! Call optiga_manager_init() first.\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return NULL;
 }

 /* Acquire mutex (blocking) */
	#if TESAIOT_DEBUG_VERBOSE_ENABLED
	printf("%s Waiting for mutex (timeout=10s)...\n", LABEL_OPTIGA_MANAGER);
	fflush(stdout);
	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

	TickType_t timeout_ticks = pdMS_TO_TICKS(10000); // 10 seconds
 if (xSemaphoreTake(g_optiga_mutex, timeout_ticks) != pdTRUE) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s ERROR: Mutex timeout after 10 seconds!\n", LABEL_OPTIGA_MANAGER);
 printf("%s Another task is holding mutex - possible deadlock!\n", LABEL_OPTIGA_MANAGER);
 fflush(stdout);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return NULL;
 }


	#if TESAIOT_DEBUG_VERBOSE_ENABLED
	printf("%s Mutex acquired successfully\n", LABEL_OPTIGA_MANAGER);
	fflush(stdout);
	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 /* Return instance pointer (caller must call optiga_manager_release() after use) */
 return g_optiga_instance;
}

/******************************************************************************
* Function: optiga_manager_lock
*******************************************************************************/
bool optiga_manager_lock(void)
{
 if (!g_optiga_initialized || !g_optiga_mutex) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s ERROR: Not initialized! Call optiga_manager_init() first.\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return false;
 }

 /* Acquire mutex (blocking) */
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s [Lock] Waiting for mutex (timeout=10s)...\n", LABEL_OPTIGA_MANAGER);
 fflush(stdout);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

 TickType_t timeout_ticks = pdMS_TO_TICKS(10000); // 10 seconds
 if (xSemaphoreTake(g_optiga_mutex, timeout_ticks) != pdTRUE) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s ERROR: Lock mutex timeout after 10 seconds!\n", LABEL_OPTIGA_MANAGER);
 printf("%s Another task is holding mutex - possible deadlock!\n", LABEL_OPTIGA_MANAGER);
 fflush(stdout);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return false;
 }

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s [Lock] Mutex acquired successfully\n", LABEL_OPTIGA_MANAGER);
 fflush(stdout);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return true;
}

/******************************************************************************
* Function: optiga_manager_unlock
*******************************************************************************/
void optiga_manager_unlock(void)
{
 if (!g_optiga_mutex) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s ERROR: Mutex not initialized!\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return;
 }

 /* Release mutex */
 xSemaphoreGive(g_optiga_mutex);
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s [Unlock] Mutex released\n", LABEL_OPTIGA_MANAGER);
 fflush(stdout);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
}

/******************************************************************************
* Function: optiga_manager_release
*******************************************************************************/
void optiga_manager_release(void)
{
 if (!g_optiga_mutex) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s ERROR: Mutex not initialized!\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return;
 }

 /* Release mutex */
 xSemaphoreGive(g_optiga_mutex);
}

/******************************************************************************
* Function: optiga_manager_is_initialized
*******************************************************************************/
bool optiga_manager_is_initialized(void)
{
 return g_optiga_initialized && (g_optiga_instance != NULL);
}

/******************************************************************************
* Function: optiga_manager_create_crypt
*******************************************************************************/
optiga_crypt_t* optiga_manager_create_crypt(callback_handler_t callback, void *context)
{
 if (!g_optiga_initialized) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s ERROR: Not initialized! Cannot create crypt instance.\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 return NULL;
 }

 optiga_crypt_t *crypt = optiga_crypt_create(0, callback, context);
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 if (crypt) {
 printf("%s Created temporary crypt instance: %p\n", LABEL_OPTIGA_MANAGER, (void*)crypt);
 } else {
 printf("%s ERROR: optiga_crypt_create() failed!\n", LABEL_OPTIGA_MANAGER);
 }
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

 return crypt;
}

/******************************************************************************
* Function: optiga_manager_cleanup
*******************************************************************************/
void optiga_manager_cleanup(void)
{
 if (!g_optiga_initialized) {
 return;
 }

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Cleaning up persistent instance...\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

 /* Acquire mutex one last time */
 if (g_optiga_mutex) {
 xSemaphoreTake(g_optiga_mutex, portMAX_DELAY);
 }

 /* Destroy OPTIGA util instance */
 if (g_optiga_instance) {
 optiga_util_destroy(g_optiga_instance);
 g_optiga_instance = NULL;
 }

 /* Release and delete mutex */
 if (g_optiga_mutex) {
 xSemaphoreGive(g_optiga_mutex);
 vSemaphoreDelete(g_optiga_mutex);
 g_optiga_mutex = NULL;
 }

 g_optiga_initialized = false;
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Cleanup complete\n", LABEL_OPTIGA_MANAGER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
}
