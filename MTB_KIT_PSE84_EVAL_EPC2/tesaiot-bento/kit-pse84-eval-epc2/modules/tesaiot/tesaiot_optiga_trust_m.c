/**
 * SPDX-FileCopyrightText: 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
 *
 * \author TESAIoT Platform Developer Team
 * \author Infineon Technologies AG (OPTIGA Trust M integration support)
 *
 * \file tesaiot_optiga_trust_m.c
 *
 * \brief TESAIoT OPTIGA Trust M integration layer.
 *
 * This module provides the bridge between TESAIoT Platform (MQTT-based) and
 * OPTIGA Trust M hardware security. It handles:
 * - CSR workflow: generate keypair -> create CSR -> publish to platform -> receive cert
 * - Protected Update workflow: receive manifest + fragments -> apply to secure element
 * - Certificate lifecycle: auto-fallback, expiry detection, renewal triggering
 *
 * The design uses async callbacks with FreeRTOS primitives (queues, semaphores)
 * to avoid blocking the OPTIGA I2C bus during MQTT operations.
 *
 * Contact: Assoc. Prof. Wiroon Sriborrirux <sriborrirux@gmail.com>/<wiroon@tesa.or.th>
 *
 * \ingroup TESAIoT
 */

#include "tesaiot_optiga.h"
/* #include "tesaiot_csr.h" */          // CSR Workflow - REMOVED in v3.0.0
#include "tesaiot_optiga_core.h"
#include "tesaiot.h"
#include "tesaiot_config.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "task.h"

#include "publisher_task.h"
#include "mqtt_client_config.h"
#include "mqtt_task.h"
#include "cy_mqtt_api.h"
#include "optiga_crypt.h"
#include "tesaiot_config.h"

#include <mbedtls/base64.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern QueueHandle_t publisher_task_q;

/*******************************************************************************
 * Device Configuration (Extern Variables)
 *
 * These values are provided by mqtt_device_config_data.c which customer
 * compiles with their own DEVICE_ID and API_KEY from mqtt_client_config.h.
 * This allows library to be device-agnostic at compile time.
 ******************************************************************************/
extern const char* mqtt_device_id;    /* Device UUID for topic construction */
extern const char* mqtt_api_key;      /* API key for broker authentication */
extern const char* mqtt_factory_uid;  /* Factory UID from OPTIGA Trust M */

/*******************************************************************************
 * Module State
 *
 * These variables track the current workflow state. The state machine ensures
 * we don't start a new CSR/Protected Update while one is in progress.
 ******************************************************************************/
static volatile trustm_state_t trustm_state = TRUSTM_STATE_IDLE;
static TickType_t trustm_state_entered_ticks = 0;
static uint32_t trustm_payload_version = TRUSTM_INITIAL_PAYLOAD_VERSION;
static char trustm_correlation_id[37] = {0};  /* UUID v4 for request tracking */
static volatile optiga_lib_status_t trustm_optiga_status = OPTIGA_LIB_SUCCESS;
static QueueHandle_t trustm_scenario_queue = NULL;
static uint8_t *trustm_staged_pubkey = NULL;
static size_t trustm_staged_pubkey_len = 0U;
static bool g_using_fallback_cert = false;  /* true when factory cert is in use */
static bool g_force_factory_cert = true;    /* WORKAROUND: Force Factory Cert by default until key verification implemented */

/*******************************************************************************
 * External Semaphores for Smart Auto-Fallback
 *
 * These are created in subscriber_task.c and used here to synchronize
 * platform responses during certificate check/upload/sync operations.
 ******************************************************************************/
extern SemaphoreHandle_t check_certificate_response_semaphore;
extern SemaphoreHandle_t upload_certificate_response_semaphore;
extern SemaphoreHandle_t sync_certificate_response_semaphore;

/* CSR publish synchronization semaphore - subscriber waits on this before subscribe_to_topic()
 * Since we use direct publish (bypassing publisher_task), we must signal this here.
 */
extern SemaphoreHandle_t g_csr_publish_done_semaphore;
extern volatile bool platform_has_certificate;
extern volatile bool certificate_upload_success;
extern volatile bool check_certificate_response_received;
extern volatile bool upload_certificate_response_received;
extern volatile bool sync_certificate_response_received;
extern volatile bool certificate_sync_success;

/*******************************************************************************
 * Base64 Decoding
 ******************************************************************************/

/**
 * Decode base64-encoded payload received from TESAIoT platform.
 *
 * Platform sends binary data (manifests, fragments, certificates) as base64
 * strings in JSON. This function decodes them back to binary.
 *
 * Uses two-pass decode: first call with NULL buffer to get required size,
 * then allocate and decode. This avoids overallocation.
 *
 * @param input       Base64 string from platform JSON
 * @param input_len   Length of base64 string (excluding null terminator)
 * @param output      Pointer to receive allocated buffer (caller must free)
 * @param output_len  Pointer to receive decoded length
 * @return true on success, false if invalid base64 or allocation fails
 */
static bool trustm_decode_base64_payload(const uint8_t *input, size_t input_len,
                                         uint8_t **output, size_t *output_len)
{
    if (!input || input_len == 0U || !output || !output_len)
        return false;

    /* First call to get required buffer size */
    size_t required_len = 0U;
    int rc = mbedtls_base64_decode(NULL, 0, &required_len, input, input_len);
    if (!((rc == 0) || (rc == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)) || (required_len == 0U))
        return false;

    uint8_t *decoded = (uint8_t *)malloc(required_len);
    if (!decoded)
        return false;

    size_t actual_len = required_len;
    rc = mbedtls_base64_decode(decoded, required_len, &actual_len, input, input_len);
    if (rc != 0)
    {
        free(decoded);
        return false;
    }

    *output = decoded;
    *output_len = actual_len;
    return true;
}

/*******************************************************************************
 * OPTIGA Async Callback
 ******************************************************************************/

/**
 * Generic callback for OPTIGA async operations.
 *
 * All OPTIGA operations are non-blocking - they return immediately and call
 * this callback when complete. The callback stores the result in a global
 * variable that the calling code polls.
 *
 * Pattern used throughout this module:
 *   1. Set trustm_optiga_status = OPTIGA_LIB_BUSY
 *   2. Call optiga_xxx_operation()
 *   3. Poll until trustm_optiga_status != OPTIGA_LIB_BUSY (or timeout)
 *   4. Check final status
 *
 * This polling pattern is necessary because FreeRTOS tasks cannot block on
 * OPTIGA's I2C completion directly.
 */
static void trustm_optiga_callback(void *context, optiga_lib_status_t return_status)
{
    (void)context;
    trustm_optiga_status = return_status;
}

/*******************************************************************************
 * UUID Generation
 ******************************************************************************/

/**
 * Generate UUID v4 correlation ID for request-response matching.
 *
 * Every request to TESAIoT platform includes a correlation_id. When platform
 * responds (e.g., with a certificate or manifest), it echoes this ID so we
 * can match responses to requests - critical when multiple operations are
 * in flight.
 *
 * Uses OPTIGA's hardware TRNG for cryptographically secure random bytes.
 * If TRNG times out (e.g., I2C bus contention), falls back to XORing
 * FreeRTOS tick count into the bytes. The fallback is less secure but
 * correlation IDs don't need crypto-strength uniqueness.
 *
 * UUID v4 format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
 * where 4 = version 4 (random), y = variant bits (8, 9, A, or B)
 *
 * @param out  Buffer to receive UUID string (minimum 37 bytes)
 * @param len  Buffer size
 * @return true on success
 */
static bool trustm_generate_correlation_id(char *out, size_t len)
{
    if (!out || len < 37U)
        return false;

    uint8_t random_bytes[16] = {0};
    bool success = false;

    /* Lock OPTIGA mutex for thread-safe access */
    if (!optiga_manager_lock())
    {
        /* Fall through to software fallback */
    }
    else
    {
        /* Create temporary crypt instance with our callback */
        optiga_crypt_t *crypt = optiga_crypt_create(0, trustm_optiga_callback, NULL);
        if (crypt)
        {
            trustm_optiga_status = OPTIGA_LIB_BUSY;
            const TickType_t timeout_ticks = pdMS_TO_TICKS(TRUSTM_RNG_TIMEOUT_MS);
            TickType_t start_ticks = xTaskGetTickCount();

            optiga_lib_status_t status = optiga_crypt_random(crypt, OPTIGA_RNG_TYPE_TRNG,
                                                             random_bytes, sizeof(random_bytes));
            while (trustm_optiga_status == OPTIGA_LIB_BUSY)
            {
                if ((timeout_ticks > 0U) && ((xTaskGetTickCount() - start_ticks) >= timeout_ticks))
                    break;
                vTaskDelay(1);
            }

            if ((status == OPTIGA_LIB_SUCCESS) && (trustm_optiga_status == OPTIGA_LIB_SUCCESS))
                success = true;
#if TESAIOT_DEBUG_WARNING_ENABLED
            else if (trustm_optiga_status == OPTIGA_LIB_BUSY)
                printf("%s OPTIGA RNG timeout, using software fallback\n", LABEL_WARN);
#endif
            /* Destroy local crypt instance */
            optiga_crypt_destroy(crypt);
        }
        /* Unlock mutex */
        optiga_manager_unlock();
    }

    /* Software fallback: XOR tick count into random bytes */
    if (!success)
    {
        uint32_t ticks = (uint32_t)xTaskGetTickCount();
        for (size_t i = 0; i < sizeof(random_bytes); ++i)
            random_bytes[i] ^= (uint8_t)((ticks >> ((i % 4U) * 8U)) & 0xFFU);
    }

    /* Set UUID version 4 (random) and variant bits per RFC 4122 */
    random_bytes[6] = (random_bytes[6] & 0x0FU) | 0x40U;
    random_bytes[8] = (random_bytes[8] & 0x3FU) | 0x80U;

    (void)snprintf(out, len,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        random_bytes[0], random_bytes[1], random_bytes[2], random_bytes[3],
        random_bytes[4], random_bytes[5], random_bytes[6], random_bytes[7],
        random_bytes[8], random_bytes[9], random_bytes[10], random_bytes[11],
        random_bytes[12], random_bytes[13], random_bytes[14], random_bytes[15]);
    return true;
}

/**
 * Generate lightweight correlation ID for Smart Fallback operations.
 *
 * Unlike the full UUID generator, this uses simple tick count + rand()
 * for Smart Fallback operations (check_certificate, upload_certificate).
 * These operations are less security-critical and don't need TRNG.
 *
 * Format: "smart-fallback-{timestamp_hex}-{random_hex}"
 * Example: "smart-fallback-0012ab34-56cd78ef"
 */
static void generate_smart_fallback_correlation_id(char *buffer, size_t buffer_size)
{
    uint32_t timestamp = xTaskGetTickCount();
    uint32_t random = rand();
    snprintf(buffer, buffer_size, "smart-fallback-%08lx-%08lx",
             (unsigned long)timestamp, (unsigned long)random);
}

/*******************************************************************************
 * State Machine
 *
 * Tracks the current workflow state (IDLE, PUBLISHING_CSR, WAITING_FOR_MANIFEST,
 * APPLYING_UPDATE, ERROR, etc.). Only one workflow can run at a time.
 *
 * State transitions are logged with status codes for debugging. The entered_ticks
 * timestamp allows timeout detection for stuck states.
 ******************************************************************************/

/**
 * Reset state machine to IDLE.
 *
 * Called after completing a workflow (success or failure) to allow
 * starting a new operation. Also clears correlation ID and resets
 * payload version counter.
 */
void trustm_reset_state(void)
{
    trustm_state = TRUSTM_STATE_IDLE;
    trustm_state_entered_ticks = 0;
    trustm_payload_version = TRUSTM_INITIAL_PAYLOAD_VERSION;
    memset(trustm_correlation_id, 0, sizeof(trustm_correlation_id));
}

trustm_state_t trustm_get_state(void)
{
    return trustm_state;
}

/**
 * Transition to a new state with optional status logging.
 *
 * Records the transition timestamp for timeout detection. The status_code
 * and detail are logged when TESAIOT_DEBUG_VERBOSE_ENABLED is set.
 *
 * Common status_code values:
 *   "csr_sent"           - CSR published, waiting for platform response
 *   "manifest_received"  - Platform sent Protected Update manifest
 *   "update_complete"    - Successfully applied Protected Update
 *   "csr_publish_failed" - Failed to queue CSR for MQTT publish
 */
void trustm_update_state(trustm_state_t new_state, const char *status_code, const char *detail)
{
    trustm_state = new_state;
    trustm_state_entered_ticks = xTaskGetTickCount();

#if TESAIOT_MQTT_DEBUG_ENABLED && TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s State -> %d", LABEL_TRUSTM, (int)new_state);
    if (status_code) printf(" | status=%s", status_code);
    if (detail) printf(" | detail=%s", detail);
    printf("\n");
#endif
}

void trustm_report_error(const char *status_code, const char *detail)
{
    trustm_update_state(TRUSTM_STATE_ERROR, status_code, detail);
}

const char *trustm_current_correlation_id(void)
{
    return (trustm_correlation_id[0] != '\0') ? trustm_correlation_id : NULL;
}

uint32_t trustm_generate_payload_version(void)
{
    return trustm_payload_version++;
}

/*******************************************************************************
 * MQTT Publishing Infrastructure
 *
 * All MQTT publishes go through the publisher task queue. This design has
 * several benefits:
 *
 * 1. Decoupling: OPTIGA operations (which take 100s of ms on I2C) don't block
 *    network I/O. The publisher task handles MQTT independently.
 *
 * 2. Retry logic: If MQTT publish fails (network issue), publisher task can
 *    retry with exponential backoff without blocking the caller.
 *
 * 3. Memory management: The take_ownership flag lets caller decide whether
 *    publisher should free the payload buffer after sending.
 *
 * 4. Priority: Publisher task can prioritize messages (e.g., CSR before telemetry).
 ******************************************************************************/

/**
 * Queue a message for MQTT publishing.
 *
 * The message is added to publisher_task_q for async delivery. If take_ownership
 * is true, the publisher task will vPortFree() the payload after sending;
 * otherwise caller retains ownership and must not modify the buffer until
 * publish completes.
 *
 * @param topic           MQTT topic to publish to
 * @param payload         Message payload (JSON string typically)
 * @param payload_len     Length of payload in bytes
 * @param take_ownership  If true, publisher frees payload after send
 * @param max_attempts    Maximum retry attempts (1 = no retry)
 * @param retry_delay_ms  Delay between retries in milliseconds
 * @return 0 on success (queued), -1 on failure
 */
static int trustm_queue_publish(const char *topic, char *payload, size_t payload_len,
                                bool take_ownership, uint8_t max_attempts, uint32_t retry_delay_ms)
{
    if (!topic || !payload)
    {
        if (take_ownership && payload)
            vPortFree(payload);
        return -1;
    }

    /* Wait for publisher queue to be ready (created by publisher_task) */
    if (publisher_task_q == NULL)
    {
        printf("[TrustM] Waiting for publisher queue to initialize...\n");
        fflush(stdout);

        /* Wait up to 5 seconds for publisher task to create its queue */
        for (int wait = 0; wait < 50 && publisher_task_q == NULL; wait++)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (publisher_task_q == NULL)
        {
            printf("[TrustM] ERROR: Publisher queue never initialized (task not created?)\n");
            if (take_ownership)
                vPortFree(payload);
            return -1;
        }
        printf("[TrustM] Publisher queue ready!\n");
    }

    publisher_data_t queue_item = {0};
    queue_item.cmd = PUBLISH_MQTT_MSG;
    queue_item.data = payload;
    queue_item.payload_len = payload_len;
    strncpy(queue_item.topic, topic, sizeof(queue_item.topic) - 1U);
    queue_item.topic[sizeof(queue_item.topic) - 1U] = '\0';
    queue_item.topic_len = strlen(topic);
    queue_item.free_after_publish = take_ownership;
    queue_item.attempt = 0U;
    queue_item.max_attempts = (max_attempts == 0U) ? 1U : max_attempts;
    queue_item.retry_delay_ticks = (retry_delay_ms == 0U) ? 0 : pdMS_TO_TICKS(retry_delay_ms);

    printf("[TrustM] xQueueSendToBack: topic=%s len=%u q=%p\n",
           topic, (unsigned int)payload_len, (void*)publisher_task_q);
    fflush(stdout);

    if (xQueueSendToBack(publisher_task_q, &queue_item, portMAX_DELAY) != pdPASS)
    {
        printf("[TrustM] xQueueSendToBack FAILED!\n");
        fflush(stdout);
        if (take_ownership)
            vPortFree(payload);
        return -1;
    }

    printf("[TrustM] xQueueSendToBack OK - queued successfully\n");
    fflush(stdout);
    return 0;
}

/**
 * WORKAROUND: Direct publish bypassing publisher_task queue.
 *
 * The publisher_task context causes ERR_WOULDBLOCK (-7) in netconn_write_partly
 * even when TCP parameters look healthy (sndbuf=6000, qlen=0, tcp_state=ESTABLISHED).
 * However, MQTT CONNECT succeeds when done from mqtt_task context.
 *
 * This function calls cy_mqtt_publish() directly from the calling context
 * (CSR workflow / main task) instead of queuing to publisher_task.
 *
 * @param topic          Full topic string
 * @param payload        Message payload
 * @param payload_len    Length of payload in bytes
 * @return 0 on success, -1 on failure
 */
static int trustm_direct_publish(const char *topic, const char *payload, size_t payload_len)
{
    if (!topic || !payload || payload_len == 0)
    {
        printf("[DirectPub] ERROR: Invalid parameters\n");
        return -1;
    }

    if (mqtt_connection == NULL)
    {
        printf("[DirectPub] ERROR: mqtt_connection is NULL\n");
        return -1;
    }

    printf("[DirectPub] Publishing %u bytes to '%s'\n", (unsigned int)payload_len, topic);
    fflush(stdout);

    /* Create publish_info structure */
    cy_mqtt_publish_info_t publish_info = {
        .qos = CY_MQTT_QOS0,  /* QoS 0 for CSR (fire-and-forget, avoids PUBACK wait) */
        .topic = topic,
        .topic_len = strlen(topic),
        .payload = (const char *)payload,
        .payload_len = payload_len,
        .retain = false,
        .dup = false
    };

    /* Memory barriers for cache coherency on dual-core PSoC E84 */
    __DSB();
    __DMB();
    __ISB();

    printf("[DirectPub] Calling cy_mqtt_publish() from current context...\n");
    fflush(stdout);

    /* Small delay to allow TCP stack to be ready */
    vTaskDelay(pdMS_TO_TICKS(100));

    cy_rslt_t result = cy_mqtt_publish(mqtt_connection, &publish_info);

    printf("[DirectPub] cy_mqtt_publish returned: 0x%08X\n", (unsigned int)result);
    fflush(stdout);

    if (result == CY_RSLT_SUCCESS)
    {
        /* Give TCP stack time to actually send the packet */
        vTaskDelay(pdMS_TO_TICKS(500));
        printf("[DirectPub] SUCCESS!\n");
        fflush(stdout);
        return 0;
    }
    else
    {
        printf("[DirectPub] FAILED with error 0x%08X\n", (unsigned int)result);
        fflush(stdout);
        return -1;
    }
}

/**
 * Publish a JSON status message to TESAIoT platform.
 *
 * Constructs a JSON object with status code and optional detail message.
 * If a correlation_id is active (from a prior request), includes it so
 * platform can match this status to the originating request.
 *
 * Example output: {"status":"csr_sent","detail":"CSR published","correlation_id":"..."}
 *
 * Used for:
 *   - Acknowledging received commands
 *   - Reporting workflow progress/completion
 *   - Error reporting with diagnostic detail
 */
static int trustm_publish_status_json(const char *topic, const char *status_code,
                                      const char *detail, uint8_t max_attempts,
                                      uint32_t retry_delay_ms)
{
    if (!status_code)
        return -1;

    const char *detail_safe = (detail != NULL) ? detail : "";
    size_t payload_len = 0;

    /* Calculate required buffer size */
    if (trustm_correlation_id[0] != '\0')
    {
        payload_len = (size_t)snprintf(NULL, 0,
            "{\"status\":\"%s\",\"detail\":\"%s\",\"correlation_id\":\"%s\"}",
            status_code, detail_safe, trustm_correlation_id) + 1U;
    }
    else
    {
        payload_len = (size_t)snprintf(NULL, 0,
            "{\"status\":\"%s\",\"detail\":\"%s\"}",
            status_code, detail_safe) + 1U;
    }

    char *payload = (char *)malloc(payload_len);
    if (!payload)
        return -1;

    if (trustm_correlation_id[0] != '\0')
    {
        (void)snprintf(payload, payload_len,
            "{\"status\":\"%s\",\"detail\":\"%s\",\"correlation_id\":\"%s\"}",
            status_code, detail_safe, trustm_correlation_id);
    }
    else
    {
        (void)snprintf(payload, payload_len,
            "{\"status\":\"%s\",\"detail\":\"%s\"}",
            status_code, detail_safe);
    }

    return trustm_queue_publish(topic, payload, payload_len - 1U, true,
                                max_attempts, retry_delay_ms);
}

__attribute__((unused))
static void trustm_publish_status(const char *status_code, const char *detail)
{
    (void)trustm_publish_status_json(MQTT_PUB_TOPIC_COMMAND_STATUS, status_code, detail,
                                     TRUSTM_STATUS_MAX_ATTEMPTS, TRUSTM_STATUS_RETRY_DELAY_MS);
}

__attribute__((unused))
static void trustm_publish_telemetry_status(const char *status_code, const char *detail)
{
    char topic_buffer[96];
    (void)snprintf(topic_buffer, sizeof(topic_buffer), "%s%s",
                   MQTT_TELEMETRY_TOPIC_BASE, TRUSTM_STATUS_TELEMETRY_SUFFIX);
    (void)trustm_publish_status_json(topic_buffer, status_code, detail,
                                     TRUSTM_STATUS_TELEMETRY_ATTEMPTS, TRUSTM_STATUS_RETRY_DELAY_MS);
}

/*******************************************************************************
 * Scenario Queue
 *
 * Enables async triggering of workflows from different contexts (menu, MQTT
 * commands, auto-renewal timer). A FreeRTOS queue holds pending scenario IDs:
 *
 *   Scenario 1: Protected Update workflow
 *   Scenario 2: CSR workflow (generate keypair -> create CSR -> publish)
 *
 * The main task loop calls trustm_wait_for_scenario() to dequeue and execute.
 * Only one scenario can be pending - new requests overwrite previous ones.
 ******************************************************************************/

/**
 * Initialize the scenario queue.
 *
 * Must be called once at startup before any trustm_enqueue_scenario() calls.
 * Creates a FreeRTOS queue with capacity for 4 scenario IDs.
 *
 * @return true if queue created successfully
 */
bool trustm_runtime_init(void)
{
    if (trustm_scenario_queue == NULL)
    {
        trustm_scenario_queue = xQueueCreate(4, sizeof(uint8_t));
        if (trustm_scenario_queue == NULL)
            return false;
    }

#if TESAIOT_MQTT_DEBUG_ENABLED && TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Scenario queue initialised\n", LABEL_TRUSTM);
#endif
    return true;
}

/**
 * Queue a scenario for execution by the main task loop.
 *
 * Resets the queue before adding - this ensures only the most recent
 * request is processed if multiple triggers occur before execution.
 *
 * @param scenario  Scenario ID (1=Protected Update, 2=CSR workflow)
 * @return true if queued successfully
 */
bool trustm_enqueue_scenario(uint8_t scenario)
{
    /* Valid scenarios: 1 = Protected Update, 2 = CSR workflow */
    if ((scenario == 0U) || (scenario > 2U) || (trustm_scenario_queue == NULL))
        return false;

    xQueueReset(trustm_scenario_queue);  /* Only one pending scenario at a time */
    return (xQueueSendToBack(trustm_scenario_queue, &scenario, 0U) == pdTRUE);
}

/**
 * Wait for a scenario to be queued (blocking).
 *
 * Called by the main task loop to wait for work. Blocks until either:
 *   - A scenario is dequeued (returns true, scenario ID in *scenario)
 *   - Timeout expires (returns false)
 *
 * @param scenario       Pointer to receive dequeued scenario ID
 * @param ticks_to_wait  Maximum ticks to wait (portMAX_DELAY for infinite)
 * @return true if scenario dequeued, false on timeout
 */
bool trustm_wait_for_scenario(uint8_t *scenario, TickType_t ticks_to_wait)
{
    if ((scenario == NULL) || (trustm_scenario_queue == NULL))
        return false;

    bool dequeued = (xQueueReceive(trustm_scenario_queue, scenario, ticks_to_wait) == pdTRUE);
#if TESAIOT_MQTT_DEBUG_ENABLED && TESAIOT_DEBUG_VERBOSE_ENABLED
    if (dequeued)
        printf("%s Scenario dequeued (%u)\n", LABEL_TRUSTM, (unsigned int)(*scenario));
#endif
    return dequeued;
}

/*******************************************************************************
 * Public Key Handling
 *
 * The TESAIoT platform uses asymmetric cryptography to sign Protected Update
 * manifests. When platform rotates its signing key, it sends the new public
 * key here for storage in the Trust Anchor OID (0xE0E3).
 *
 * We "stage" the pubkey in RAM until the Protected Update workflow is ready
 * to install it. This prevents partial updates if the manifest/fragment
 * delivery fails.
 ******************************************************************************/

/**
 * Handle incoming public key from TESAIoT platform.
 *
 * The public key is used to verify Protected Update manifest signatures.
 * If payload is base64-encoded (common in JSON transport), decodes it first.
 * Replaces any previously staged pubkey.
 *
 * The staged pubkey is later written to Trust Anchor OID during
 * Protected Update workflow execution.
 *
 * @param pubkey_buf  Raw or base64-encoded public key (takes ownership)
 * @param pubkey_len  Length of pubkey_buf
 */
void trustm_handle_pubkey(uint8_t *pubkey_buf, size_t pubkey_len)
{
    if (!pubkey_buf || pubkey_len == 0U)
    {
        trustm_update_state(TRUSTM_STATE_IDLE, "pubkey_invalid", "Received empty TESAIoT pubkey payload");
        return;
    }

    /* Try base64 decode; if it fails, use raw payload */
    uint8_t *decoded = NULL;
    size_t decoded_len = 0U;
    bool decoded_ok = trustm_decode_base64_payload(pubkey_buf, pubkey_len, &decoded, &decoded_len);

    uint8_t *payload_ptr = pubkey_buf;
    size_t payload_len = pubkey_len;

    if (decoded_ok)
    {
        payload_ptr = decoded;
        payload_len = decoded_len;
        free(pubkey_buf);  /* Original buffer no longer needed */
    }

    /* Replace any previously staged pubkey */
    if (trustm_staged_pubkey)
        free(trustm_staged_pubkey);

    trustm_staged_pubkey = payload_ptr;
    trustm_staged_pubkey_len = payload_len;

    trustm_update_state(TRUSTM_STATE_IDLE, "pubkey_staged", "TESAIoT pubkey staged; rotation pending");

#if TESAIOT_MQTT_DEBUG_ENABLED && TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Stored %s pubkey payload (%u bytes)\n", LABEL_TRUSTM,
           decoded_ok ? "decoded" : "raw", (unsigned int)payload_len);
#endif
}

/*******************************************************************************
 * Command Publishing API
 *
 * These functions publish commands to the TESAIoT platform. The topic structure
 * is: device/{device_id}/commands/{suffix}
 ******************************************************************************/

int trustm_publish_command_payload(const char *topic_suffix, char *payload, size_t payload_len,
                                   bool take_ownership, uint8_t max_attempts, uint32_t retry_delay_ms)
{
    char topic_buffer[96];

    /* Build topic at runtime using customer's device ID (NOT compile-time macro) */
    if (topic_suffix && topic_suffix[0] != '\0')
        (void)snprintf(topic_buffer, sizeof(topic_buffer), "device/%s/commands/%s", mqtt_device_id, topic_suffix);
    else
        (void)snprintf(topic_buffer, sizeof(topic_buffer), "device/%s/commands", mqtt_device_id);

    int rc = trustm_queue_publish(topic_buffer, payload, payload_len, take_ownership,
                                  max_attempts, retry_delay_ms);

#if TESAIOT_MQTT_DEBUG_ENABLED && TESAIOT_DEBUG_VERBOSE_ENABLED
    if (rc == 0)
    {
        printf("%s Q-sent %u bytes\n", LABEL_TRUSTM, (unsigned int)payload_len);
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    else
    {
        printf("%s Q-fail rc=%d\n", LABEL_TRUSTM, rc);
        fflush(stdout);
    }
#endif

    return rc;
}

int trustm_publish_command_qos0(const char *topic_suffix, char *payload, size_t payload_len)
{
    char topic_buffer[96];

    /* Build topic at runtime using customer's device ID (NOT compile-time macro) */
    if (topic_suffix && topic_suffix[0] != '\0')
        (void)snprintf(topic_buffer, sizeof(topic_buffer), "device/%s/commands/%s", mqtt_device_id, topic_suffix);
    else
        (void)snprintf(topic_buffer, sizeof(topic_buffer), "device/%s/commands", mqtt_device_id);

    /* QoS 0 = fire and forget, single attempt, no retry */
    int rc = trustm_queue_publish(topic_buffer, payload, payload_len, true, 1, 0);

#if TESAIOT_MQTT_DEBUG_ENABLED && TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Q-%s QoS0 %u bytes\n", LABEL_TRUSTM, (rc == 0) ? "sent" : "fail", (unsigned int)payload_len);
    fflush(stdout);
#endif

    return rc;
}

int publish_mqtt_msg(const char *msg, const char *topic_suffix)
{
    if (!msg)
        return -1;

    size_t payload_len = strlen(msg);
    return trustm_publish_command_payload(topic_suffix, (char *)msg, payload_len, false, 1U, 0U);
}

/*******************************************************************************
 * CSR Workflow
 *
 * Certificate Signing Request flow:
 *
 * 1. Device generates ECC keypair in OPTIGA (TESAIOT_TARGET_KEY_OID)
 * 2. Device creates CSR containing public key + device identity
 * 3. This function publishes CSR to TESAIoT platform via MQTT
 * 4. Platform verifies device identity, signs certificate, sends back
 * 5. Device receives cert via subscriber_task and writes to OPTIGA
 *
 * The CSR is in PEM format with embedded newlines. For JSON transport,
 * newlines are escaped as literal "\n" strings.
 ******************************************************************************/

/**
 * Publish a CSR to TESAIoT platform for signing.
 *
 * Wraps the PEM-formatted CSR in a JSON envelope with:
 *   - device_id: Unique device identifier (Factory UID)
 *   - csr: PEM string with escaped newlines
 *   - correlation_id: UUID for request-response matching
 *
 * After successful publish, transitions state to WAITING_FOR_MANIFEST
 * and waits for platform to respond with signed certificate.
 *
 * @param csr               PEM-formatted CSR buffer
 * @param csr_length        Length of CSR in bytes
 * @param target_oid        OID where resulting certificate will be stored
 * @param trust_anchor_oid  Trust anchor OID (unused, for future use)
 * @param payload_version   Protocol version (unused, for future use)
 * @return 0 on success, -1 on failure
 */
int publish_csr(uint8_t *csr, size_t csr_length, uint16_t target_oid,
                uint16_t trust_anchor_oid, uint32_t payload_version)
{
    (void)trust_anchor_oid;
    (void)payload_version;
    int ret;

    /* Generate correlation ID for request tracking */
    if (!trustm_generate_correlation_id(trustm_correlation_id, sizeof(trustm_correlation_id)))
    {
        /* Fallback to device ID if TRNG fails */
        strncpy(trustm_correlation_id, MQTT_CLIENT_IDENTIFIER, sizeof(trustm_correlation_id) - 1U);
        trustm_correlation_id[sizeof(trustm_correlation_id) - 1U] = '\0';
    }

#if TESAIOT_MQTT_DEBUG_ENABLED
    printf("%s Publishing CSR (len=%u, target_oid=0x%04X)\n", LABEL_TRUSTM,
           (unsigned int)csr_length, target_oid);
#endif

    printf("CSR ready in PEM format\n");
    fflush(stdout);

    trustm_state = TRUSTM_STATE_PUBLISHING_CSR;
    trustm_state_entered_ticks = xTaskGetTickCount();

    /*
     * Build JSON directly WITHOUT intermediate csr_escaped buffer!
     * This saves ~469 bytes of malloc() that was causing heap exhaustion.
     *
     * Strategy: Calculate JSON size, malloc ONCE, then build JSON inline
     * while escaping CSR newlines on-the-fly.
     */

    /* Count newlines to calculate escaped CSR length */
    size_t newline_count = 0;
    size_t cr_count = 0;  /* Also count \r for removal */
    for (size_t i = 0; i < csr_length; i++)
    {
        if (csr[i] == '\n') newline_count++;
        if (csr[i] == '\r') cr_count++;
    }

    /* Calculate final JSON size:
     * - Base JSON structure: {"device_id":"...","csr":"...","correlation_id":"..."}
     * - CSR length after escaping: csr_length + newline_count - cr_count
     *   (each \n becomes \\n = +1 char, each \r removed = -1 char)
     */
    size_t escaped_csr_len = csr_length + newline_count - cr_count;
    size_t json_len = strlen("{\"device_id\":\"") + strlen(MQTT_CLIENT_IDENTIFIER) +
                     strlen("\",\"csr\":\"") + escaped_csr_len +
                     strlen("\",\"correlation_id\":\"") + strlen(trustm_correlation_id) +
                     strlen("\"}") + 1;  /* +1 for null terminator */

    printf("[CSR] Newlines: %u, Escaped CSR length: %u, JSON size: %u bytes\n",
           (unsigned int)newline_count, (unsigned int)escaped_csr_len, (unsigned int)json_len);
    fflush(stdout);

    /* Build JSON on temp stack, then copy to caller's CSR buffer */
    char temp_json[700];  /* Temp buffer on stack - freed after copy */
    if (json_len > sizeof(temp_json))
    {
        printf("[CSR] ERROR: JSON size (%u) exceeds temp buffer (%u)!\n",
               (unsigned int)json_len, (unsigned int)sizeof(temp_json));
        fflush(stdout);
        trustm_report_error("csr_json_too_large", "buffer_overflow");
        return -1;
    }
    printf("[CSR] Using temp stack buffer (%u bytes), will copy to CSR buffer...\n", (unsigned int)sizeof(temp_json));
    fflush(stdout);

    /* Build JSON in temp buffer */
    size_t json_idx = 0;

    /* Add: {"device_id":" */
    const char *prefix1 = "{\"device_id\":\"";
    strcpy(temp_json + json_idx, prefix1);
    json_idx += strlen(prefix1);

    /* Add device_id */
    strcpy(temp_json + json_idx, MQTT_CLIENT_IDENTIFIER);
    json_idx += strlen(MQTT_CLIENT_IDENTIFIER);

    /* Add: ","csr":" */
    const char *prefix2 = "\",\"csr\":\"";
    strcpy(temp_json + json_idx, prefix2);
    json_idx += strlen(prefix2);

    /* Add CSR with inline newline escaping */
    for (size_t i = 0; i < csr_length; i++)
    {
        if (csr[i] == '\n')
        {
            temp_json[json_idx++] = '\\';
            temp_json[json_idx++] = 'n';
        }
        else if (csr[i] != '\r')  /* Skip \r */
        {
            temp_json[json_idx++] = csr[i];
        }
    }

    /* Add: ","correlation_id":" */
    const char *prefix3 = "\",\"correlation_id\":\"";
    strcpy(temp_json + json_idx, prefix3);
    json_idx += strlen(prefix3);

    /* Add correlation_id */
    strcpy(temp_json + json_idx, trustm_correlation_id);
    json_idx += strlen(trustm_correlation_id);

    /* Add: "} */
    temp_json[json_idx++] = '\"';
    temp_json[json_idx++] = '}';
    temp_json[json_idx] = '\0';

    printf("[CSR] JSON built in temp (%u bytes)\n", (unsigned int)json_idx);
    fflush(stdout);

    /* CRITICAL: Copy JSON from temp stack buffer to CSR buffer (reuse CSR buffer!)
     * This allows temp_json (700 bytes on stack) to be freed while JSON persists
     * in caller's CSR buffer until publish completes.
     */
    memcpy((char *)csr, temp_json, json_idx + 1);  /* +1 for null terminator */
    printf("[CSR] JSON copied to CSR buffer, temp freed. Calling publish...\n");
    fflush(stdout);

    /* Memory barriers before publish to ensure all writes are visible */
    __DSB();
    __DMB();
    __ISB();

    /* WORKAROUND: Use direct publish instead of queuing to publisher_task
     * Publisher task context causes ERR_WOULDBLOCK (-7) in netconn_write_partly
     * even when TCP parameters look healthy. Direct publish from current context
     * (CSR workflow / main task) bypasses the problematic publisher_task.
     */
    char topic_buffer[96];
    (void)snprintf(topic_buffer, sizeof(topic_buffer), "device/%s/commands/csr", mqtt_device_id);

    printf("[CSR] Using DIRECT PUBLISH (bypassing publisher_task queue)\n");
    fflush(stdout);

    ret = trustm_direct_publish(topic_buffer, (const char *)csr, json_len - 1U);

    printf("[CSR] trustm_direct_publish returned %d\n", ret);
    fflush(stdout);

    if (ret == 0)
    {
        printf("[CSR] DEBUG: Calling trustm_update_state...\n");
        fflush(stdout);
        trustm_update_state(TRUSTM_STATE_WAITING_FOR_MANIFEST, "csr_sent", "CSR published to TESAIoT broker");
        printf("[CSR] DEBUG: trustm_update_state complete\n");
        fflush(stdout);

        /* Signal subscriber task that CSR publish is complete - it can now subscribe.
         * This is critical because we use direct publish (bypasses publisher_task),
         * so publisher_task's semaphore signal code is never reached.
         * Without this, subscriber waits 30s timeout before subscribing.
         */
        if (g_csr_publish_done_semaphore != NULL)
        {
            xSemaphoreGive(g_csr_publish_done_semaphore);
            printf("[CSR] Signaled g_csr_publish_done_semaphore - subscriber can now subscribe\n");
            fflush(stdout);
        }
    }
    else
    {
        printf("unable to publish csr (direct publish failed)\n");
        trustm_report_error("csr_publish_failed", "direct_publish");
    }

    printf("[CSR] DEBUG: About to return ret=%d\n", ret);
    fflush(stdout);
    return ret;
}

/*******************************************************************************
 * Protected Update Request
 *
 * When an OPTIGA OID has its access conditions set to "Integrity Protected",
 * it cannot be written directly - it requires a cryptographically signed
 * manifest and encrypted data fragments.
 *
 * This section requests the TESAIoT platform to generate such a bundle:
 *   1. Device sends request with target OID and trust anchor info
 *   2. Platform generates: manifest (signed) + fragments (encrypted)
 *   3. Device receives bundle via MQTT subscription
 *   4. Device applies using OPTIGA's Protected Update API
 *
 * This enables secure certificate provisioning even when OIDs are locked.
 ******************************************************************************/

/**
 * Request a Protected Update bundle from TESAIoT platform.
 *
 * Called when device needs to update a locked OID (e.g., device certificate).
 * Generates a new keypair first, then requests platform to create a
 * certificate and wrap it in a Protected Update bundle.
 *
 * Flow:
 *   1. Read device UID for identification
 *   2. Generate new ECC keypair in TESAIOT_TARGET_KEY_OID
 *   3. Publish request with target_oid, trust_anchor_oid, correlation_id
 *   4. Wait for platform to send back manifest + fragments
 *
 * Note: This function blocks indefinitely after publishing (test mode).
 */
void publish_request_protected_update(void)
{
#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Generating keypair before Protected Update request...\n", LABEL_PROTUPD);
#endif

    /* Read device UID for identification */
    char factory_uid[65] = {0};
    if (!tesaiot_read_factory_uid(factory_uid, sizeof(factory_uid)))
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Failed to read factory UID\n", LABEL_PROTUPD);
#endif
        return;
    }

    /* Generate new keypair - this is the key that will be certified */
    uint8_t public_key_der[128];
    uint16_t public_key_der_len = sizeof(public_key_der);
    if (!tesaiot_generate_device_keypair(TESAIOT_TARGET_KEY_OID, public_key_der, &public_key_der_len))
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Failed to generate keypair\n", LABEL_PROTUPD);
#endif
        return;
    }

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Keypair generated (%u bytes)\n", LABEL_PROTUPD, public_key_der_len);
#endif

    if (!trustm_generate_correlation_id(trustm_correlation_id, sizeof(trustm_correlation_id)))
    {
        strncpy(trustm_correlation_id, MQTT_CLIENT_IDENTIFIER, sizeof(trustm_correlation_id) - 1U);
        trustm_correlation_id[sizeof(trustm_correlation_id) - 1U] = '\0';
    }

    char device_id[128];
    if (!optiga_read_factory_uid(device_id, sizeof(device_id)))
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Failed to read Factory UID\n", LABEL_PROTUPD);
#endif
        return;
    }

    /* Convert OIDs to hex strings for JSON */
    char target_oid_hex[5];
    char trust_anchor_oid_hex[5];
    (void)snprintf(target_oid_hex, sizeof(target_oid_hex), "%04X", TESAIOT_DEVICE_CERT_OID);
    (void)snprintf(trust_anchor_oid_hex, sizeof(trust_anchor_oid_hex), "%04X", TESAIOT_TRUST_ANCHOR_OID);

    /* Calculate JSON payload size and allocate buffer */
    size_t json_len = (size_t)snprintf(NULL, 0,
        "{\"device_id\":\"%s\",\"target_oid\":\"%s\",\"trust_anchor_oid\":\"%s\","
        "\"payload_version\":2,\"correlation_id\":\"%s\"}",
        MQTT_USERNAME, target_oid_hex, trust_anchor_oid_hex, trustm_correlation_id) + 1U;

    char *temp_json = (char *)pvPortMalloc(json_len);
    if (!temp_json)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: pvPortMalloc failed\n", LABEL_PROTUPD);
#endif
        return;
    }

    (void)snprintf(temp_json, json_len,
        "{\"device_id\":\"%s\",\"target_oid\":\"%s\",\"trust_anchor_oid\":\"%s\","
        "\"payload_version\":2,\"correlation_id\":\"%s\"}",
        MQTT_USERNAME, target_oid_hex, trust_anchor_oid_hex, trustm_correlation_id);

    __DSB();
    __DMB();
    __ISB();

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Publishing Protected Update request (%u bytes)\n", LABEL_PROTUPD, (unsigned int)(json_len - 1));
#endif

    int result = trustm_publish_command_payload("request", temp_json, json_len - 1U, true,
                                                TRUSTM_CSR_MAX_ATTEMPTS, TRUSTM_CSR_RETRY_DELAY_MS);

    if (result == 0)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Request queued, waiting for publisher...\n", LABEL_PROTUPD);
#endif
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("\n========================================\n");
        printf("TEST DONE - HALTING PROGRAM\n");
#if TESAIOT_DEBUG_TRUSTM_ENABLED
        printf("========================================\n");
#endif
        fflush(stdout);

        while (1)
            vTaskDelay(pdMS_TO_TICKS(10000));
    }
    else
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Failed to queue request (code=%d)\n", LABEL_PROTUPD, result);
#endif
    }
}

/**
 * Publish Protected Update request to TESAIoT Platform.
 *
 * Uses DIRECT MQTT publish (not queue-based) to avoid error 0x0806000B.
 * This function is called from Protected Update workflow to request
 * certificate renewal from the TESAIoT Platform.
 *
 * Publishes JSON request to "commands/request" topic:
 *   {
 *     "device_id": "<mqtt_username>",
 *     "request_type": "protected_update",
 *     "target_oid": "E0E1",
 *     "trust_anchor_oid": "E0E8",
 *     "payload_version": 1,
 *     "client_type": "mcu",
 *     "correlation_id": "<uuid>"
 *   }
 *
 * @param target_oid        Hex string of target OID (e.g., "E0E1")
 * @param trust_anchor_oid  Hex string of trust anchor OID (e.g., "E0E8")
 * @param payload_version   Protocol version number
 * @return 0 on success, -1 on failure
 */
int tesaiot_publish_protected_update(const char *target_oid, const char *trust_anchor_oid,
                                     uint32_t payload_version)
{
    if (!target_oid || !trust_anchor_oid)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf(MENU_CSR_PROTUPD " ERROR: Invalid parameters\n");
#endif
        return -1;
    }

    if (!trustm_generate_correlation_id(trustm_correlation_id, sizeof(trustm_correlation_id)))
    {
        strncpy(trustm_correlation_id, MQTT_CLIENT_IDENTIFIER, sizeof(trustm_correlation_id) - 1U);
        trustm_correlation_id[sizeof(trustm_correlation_id) - 1U] = '\0';
    }

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf(MENU_CSR_PROTUPD " Protected Update Request\n");
    printf(MENU_CSR_PROTUPD " Target: %s, Anchor: %s, Version: %u\n",
           target_oid, trust_anchor_oid, (unsigned int)payload_version);
#endif

    /* Per CSR_PU_GUIDELINE: request_type and client_type are REQUIRED/Recommended */
    size_t json_len = (size_t)snprintf(NULL, 0,
        "{\"device_id\":\"%s\",\"request_type\":\"protected_update\","
        "\"target_oid\":\"%s\",\"trust_anchor_oid\":\"%s\","
        "\"payload_version\":%u,\"client_type\":\"mcu\",\"correlation_id\":\"%s\"}",
        MQTT_USERNAME, target_oid, trust_anchor_oid,
        (unsigned int)payload_version, trustm_correlation_id) + 1U;

    char *temp_json = (char *)pvPortMalloc(json_len);
    if (!temp_json)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf(MENU_CSR_PROTUPD " ERROR: Failed to allocate memory\n");
#endif
        return -1;
    }

    (void)snprintf(temp_json, json_len,
        "{\"device_id\":\"%s\",\"request_type\":\"protected_update\","
        "\"target_oid\":\"%s\",\"trust_anchor_oid\":\"%s\","
        "\"payload_version\":%u,\"client_type\":\"mcu\",\"correlation_id\":\"%s\"}",
        MQTT_USERNAME, target_oid, trust_anchor_oid,
        (unsigned int)payload_version, trustm_correlation_id);

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf(MENU_CSR_PROTUPD " JSON payload: %u bytes\n", (unsigned int)(json_len - 1));
#endif

    /* Display request details for debugging */
    printf("\n");
#if TESAIOT_DEBUG_TRUSTM_ENABLED
    printf("============================================\n");
#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Protected Update JSON Payload\n", LABEL_DEBUG);
#endif
    printf("============================================\n");
#endif
    printf("JSON length: %u bytes\n", (unsigned int)(json_len - 1));
    printf("device_id: %s\n", MQTT_USERNAME);
    printf("correlation_id: %s\n", trustm_correlation_id);
    printf("target_oid: %s\n", target_oid);
    printf("trust_anchor_oid: %s\n", trust_anchor_oid);
    printf("payload_version: %u\n", (unsigned int)payload_version);
#if TESAIOT_DEBUG_TRUSTM_ENABLED
    printf("============================================\n\n");
#endif

    __DSB();
    __DMB();
    __ISB();

    /* Build full topic: device/{device_id}/commands/request */
    char topic_buffer[128];
    (void)snprintf(topic_buffer, sizeof(topic_buffer), "device/%s/commands/request", mqtt_device_id);

    /* Use DIRECT MQTT publish (not queue-based) to avoid error 0x0806000B */
    cy_mqtt_publish_info_t pub_info = {
        .qos = CY_MQTT_QOS0,  /* QoS 0 = fire-and-forget, avoids PUBACK timeout */
        .topic = topic_buffer,
        .topic_len = strlen(topic_buffer),
        .payload = temp_json,
        .payload_len = json_len - 1U,
        .retain = false,
        .dup = false
    };

    printf(MENU_CSR_PROTUPD " Publishing to: %s\n", topic_buffer);

    cy_rslt_t result = cy_mqtt_publish(mqtt_connection, &pub_info);

    int ret = (result == CY_RSLT_SUCCESS) ? 0 : -1;

    if (ret == 0)
    {
        /* Small delay to let TCP send the packet */
        vTaskDelay(pdMS_TO_TICKS(500));
        printf(MENU_CSR_PROTUPD " Request published successfully!\n");
    }
    else
    {
        printf(MENU_CSR_PROTUPD " ERROR: Publish failed (0x%08X)\n", (unsigned int)result);
    }

    vPortFree(temp_json);
    return ret;
}

/*******************************************************************************
 * Certificate Selection (Smart Auto-Fallback)
 *
 * OPTIGA Trust M contains two certificate slots:
 *   - OID 0xE0E0: Factory Certificate (Infineon-provisioned at manufacturing)
 *   - OID 0xE0E1: Device Certificate (TESAIoT platform-issued)
 *
 * For MQTT TLS mutual authentication, we prefer the device certificate
 * because it's issued by TESAIoT CA and contains device-specific identity.
 * However, if device cert is missing (first boot), expired, or invalid,
 * we fall back to factory certificate to maintain connectivity.
 *
 * This "Smart Auto-Fallback" ensures devices can always connect to platform
 * even during initial provisioning or certificate renewal scenarios.
 ******************************************************************************/

/**
 * Select which certificate OID to use for MQTT TLS authentication.
 *
 * Decision flow:
 *   1. Check device certificate (0xE0E1) - validate expiry, format
 *   2. If valid, use device cert (preferred - TESAIoT issued)
 *   3. If invalid/missing/expired, check factory cert (0xE0E0)
 *   4. If factory cert valid, use it as fallback
 *   5. If both invalid, use factory cert anyway (last resort)
 *
 * Sets g_using_fallback_cert flag so caller knows if renewal is needed.
 *
 * @return OID to use (0xE0E1 for device cert, 0xE0E0 for factory cert)
 */
/**
 * Set the runtime flag to force Factory Certificate usage.
 *
 * Call this when device certificate has key mismatch.
 * After setting this flag, the next MQTT connection will use:
 *   - Factory Certificate (0xE0E0)
 *   - Factory Key (0xE0F0)
 *
 * Then run CSR Workflow to get a new matching certificate
 * for OID 0xE0E1 with key 0xE0F1.
 *
 * The flag auto-resets after CSR workflow completes successfully.
 */
void tesaiot_set_force_factory_cert(bool force)
{
    g_force_factory_cert = force;
    printf("[CertSelect] Force Factory Certificate flag %s\n",
           force ? "ENABLED (recovery mode)" : "DISABLED (normal mode)");
}

bool tesaiot_get_force_factory_cert(void)
{
    return g_force_factory_cert;
}

uint16_t tesaiot_select_mqtt_certificate(void)
{
    printf("\n========== TESAIoT Certificate Selection ==========\n");

    /* WORKAROUND (2026-01-17): Force Factory Certificate by default
     *
     * After board reset, Device Certificate (0xE0E1) may not match Device Key (0xE0F1).
     * This causes TLS CertificateVerify to fail because the signature doesn't match
     * the public key in the certificate.
     *
     * Root cause: Code assumes cert/key always match, but they may not after reset
     * if the key was generated for a different certificate.
     *
     * TODO: Implement verify_cert_key_match() to check before selecting.
     * For now, force factory cert until user runs CSR workflow.
     */
    if (g_force_factory_cert)
    {
        printf("[CertSelect] SAFE MODE: Using Factory Certificate (default after reset)\n");
        printf("[CertSelect] Reason: Device Key (0xE0F1) may not match Device Cert (0xE0E1)\n");
        printf("[CertSelect] Using: Factory Certificate (0xE0E0) + Factory Key (0xE0F0)\n");
        printf("[CertSelect] Action: Run CSR Workflow to generate matching cert/key pair\n");
        printf("===================================================\n\n");
        g_using_fallback_cert = true;
        return 0xE0E0;
    }

    /* Check device certificate first (preferred) */
    cert_validation_result_t dev_cert_result;
    bool dev_cert_valid = optiga_check_certificate_validity(0xE0E1, &dev_cert_result);

    if (dev_cert_valid && dev_cert_result.cert_exists)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Device Certificate (0xE0E1) found:\n", LABEL_CERT_SELECT);
#endif
        printf("  Subject: %s\n", dev_cert_result.subject);
        printf("  Issuer: %s\n", dev_cert_result.issuer);
        printf("  Valid From: %s", ctime(&dev_cert_result.valid_from));
        printf("  Valid To: %s", ctime(&dev_cert_result.valid_to));
        printf("  Days Until Expiry: %u\n", (unsigned int)dev_cert_result.days_until_expiry);

        if (dev_cert_result.is_valid)
        {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
            printf("%s Device Certificate is VALID\n", LABEL_CERT_SELECT);
            printf("%s Selected: OID 0xE0E1\n", LABEL_CERT_SELECT);
#endif
#if TESAIOT_DEBUG_TRUSTM_ENABLED
            printf("===================================================\n\n");
#endif
            g_using_fallback_cert = false;
            return 0xE0E1;
        }
        else if (dev_cert_result.is_expired)
        {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
            printf("%s Device Certificate EXPIRED\n", LABEL_CERT_SELECT);
#endif
        }
        else
        {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
            printf("%s Device Certificate INVALID\n", LABEL_CERT_SELECT);
#endif
        }
    }
    else
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Device Certificate (0xE0E1) NOT FOUND\n", LABEL_CERT_SELECT);
#endif
    }

    /* Fallback to factory certificate */
#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Falling back to Factory Certificate (0xE0E0)...\n", LABEL_CERT_SELECT);
#endif

    cert_validation_result_t factory_cert_result;
    bool factory_cert_valid = optiga_check_certificate_validity(0xE0E0, &factory_cert_result);

    if (factory_cert_valid && factory_cert_result.cert_exists)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Factory Certificate (0xE0E0) found:\n", LABEL_CERT_SELECT);
#endif
        printf("  Subject: %s\n", factory_cert_result.subject);
        printf("  Issuer: %s\n", factory_cert_result.issuer);
        printf("  Valid From: %s", ctime(&factory_cert_result.valid_from));
        printf("  Valid To: %s", ctime(&factory_cert_result.valid_to));
        printf("  Days Until Expiry: %u\n", (unsigned int)factory_cert_result.days_until_expiry);

        if (factory_cert_result.is_valid)
        {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
            printf("%s Factory Certificate is VALID\n", LABEL_CERT_SELECT);
            printf("%s Selected: OID 0xE0E0 (fallback)\n", LABEL_CERT_SELECT);
#endif
#if TESAIOT_DEBUG_TRUSTM_ENABLED
            printf("===================================================\n\n");
#endif
            g_using_fallback_cert = true;
            return 0xE0E0;
        }
    }

    /* No valid certificate available - use factory cert anyway and hope for the best */
#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s CRITICAL ERROR: No valid certificate available!\n", LABEL_CERT_SELECT);
#endif
#if TESAIOT_DEBUG_TRUSTM_ENABLED
    printf("===================================================\n\n");
#endif

    g_using_fallback_cert = true;
    return 0xE0E0;
}

bool tesaiot_is_using_fallback_certificate(void)
{
    return g_using_fallback_cert;
}

/**
 * Trigger automatic CSR renewal when device cert is expired or missing.
 *
 * Called during MQTT connect if tesaiot_select_mqtt_certificate() fell back
 * to factory certificate. This creates a FreeRTOS task to run CSR workflow
 * asynchronously so MQTT connection can proceed immediately.
 *
 * The renewal runs in background - MQTT connection proceeds with factory
 * cert while CSR workflow runs separately. Once platform issues new cert,
 * next reconnect will use it.
 *
 * Uses flag-based approach: sets a pending flag that main loop checks.
 * CSR workflow runs in main loop context (large stack) rather than a
 * separate FreeRTOS task (limited stack causes overflow with mbedTLS).
 *
 * @return true if flag was set successfully
 */

/* Global flag for pending CSR renewal - checked by main loop */
static volatile bool g_auto_csr_pending = false;

bool tesaiot_auto_trigger_csr_renewal(void)
{
    /* License enforcement */
    TESAIOT_LICENSE_GATE_BOOL();

    printf("\n%s Setting auto CSR renewal flag...\n", LABEL_AUTO_RENEW);

    /* Set flag - main loop will execute CSR workflow in its context */
    g_auto_csr_pending = true;

    printf("%s Flag set. Main loop will execute CSR workflow.\n", LABEL_AUTO_RENEW);
    printf("%s (CSR runs in main context to avoid stack overflow)\n", LABEL_AUTO_RENEW);

    return true;
}

/**
 * @brief Check and execute pending CSR renewal (called from main loop)
 *
 * This function should be called periodically from the main menu loop.
 * If auto CSR renewal was triggered (e.g., after Factory Cert fallback),
 * this executes the workflow in main loop context where stack is large.
 *
 * @return true if CSR workflow was executed
 */
bool tesaiot_check_pending_csr(void)
{
    /* CSR Workflow - REMOVED in v3.0.0 */
    /* Always return false - no CSR workflow available */
    return false;
}

void tesaiot_reset_fallback_state(void)
{
#if TESAIOT_DEBUG_VERBOSE_ENABLED
    if (g_using_fallback_cert || g_force_factory_cert)
        printf("%s Resetting fallback state\n", LABEL_CERT_LIFECYCLE);
    else
        printf("%s Fallback state already clear\n", LABEL_CERT_LIFECYCLE);
#endif
    g_using_fallback_cert = false;

    /* CRITICAL: Clear force factory cert flag after CSR workflow completes
     * This allows future connections to use the new Device Certificate (0xE0E1)
     * which now matches the new keypair at 0xE0F1.
     */
    g_force_factory_cert = false;

    /* CRITICAL: Also clear pending CSR flag to prevent double CSR workflow
     * This is needed because:
     * 1. User runs CSR workflow directly
     * 2. connect_to_broker() sets g_auto_csr_pending=true when using Factory Cert
     * 3. After first CSR completes, main loop would see the flag and run again
     * Clearing here prevents the second unnecessary run.
     */
    g_auto_csr_pending = false;
}

/*******************************************************************************
 * Smart Auto-Fallback: Platform Synchronization
 *
 * The device and platform must stay in sync regarding certificate state.
 * These functions handle the synchronization:
 *
 * 1. check_device_certificate_exists()
 *    Check locally if device has a cert in OID 0xE0E1
 *
 * 2. check_platform_certificate()
 *    Ask platform via MQTT: "Do you have my certificate on file?"
 *    Platform responds with yes/no via subscription callback
 *
 * 3. upload_device_certificate_to_platform()
 *    If device has cert but platform doesn't, upload it for synchronization
 *
 * 4. sync_certificate_with_platform()
 *    Unified sync command that handles all cases automatically
 *
 * These operations use semaphores to wait for platform responses with timeout.
 ******************************************************************************/

/**
 * Check if device certificate exists locally in OPTIGA.
 *
 * Reads OID 0xE0E1 and checks if it contains valid data (>550 bytes
 * indicates a valid X.509 certificate). This is a local check only -
 * doesn't communicate with platform.
 *
 * @return true if device certificate exists in OPTIGA
 */
bool check_device_certificate_exists(void)
{
    uint16_t cert_size = 1800;
    uint8_t *cert_buffer = NULL;
    optiga_lib_status_t status;
    optiga_util_t *me_util = NULL;
    bool cert_found = false;

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Checking device certificate (OID 0xE0E1)...\n", LABEL_SMART_FALLBACK);
#endif

    cert_buffer = (uint8_t *)pvPortMalloc(cert_size);
    if (!cert_buffer)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Failed to allocate certificate buffer\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

    me_util = optiga_util_create(0, trustm_optiga_callback, NULL);
    if (!me_util)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: optiga_util_create failed\n", LABEL_SMART_FALLBACK);
#endif
        vPortFree(cert_buffer);
        return false;
    }

    trustm_optiga_status = OPTIGA_LIB_BUSY;
    status = optiga_util_read_data(me_util, TESAIOT_DEVICE_CERT_OID, 0, cert_buffer, &cert_size);
    (void)status;

    /* Wait for async operation with 5s timeout */
    uint32_t timeout_ms = 5000, elapsed_ms = 0;
    while (trustm_optiga_status == OPTIGA_LIB_BUSY && elapsed_ms < timeout_ms)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed_ms += 100;
    }

    /* Certificate exists if read succeeded and size is reasonable (>550 bytes for X.509) */
    if (trustm_optiga_status == OPTIGA_LIB_SUCCESS && cert_size > 550)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Device certificate FOUND (%u bytes)\n", LABEL_SMART_FALLBACK, cert_size);
#endif
        cert_found = true;
    }
    else
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Device certificate NOT found (status=0x%04X, size=%u)\n",
               LABEL_SMART_FALLBACK, trustm_optiga_status, cert_size);
#endif
        cert_found = false;
    }

    optiga_util_destroy(me_util);
    vPortFree(cert_buffer);

    return cert_found;
}

/**
 * Query TESAIoT platform to check if it has our certificate on file.
 *
 * Publishes a check_certificate request and waits for platform response.
 * Uses semaphore synchronization with 5-second timeout.
 *
 * This is useful when device has a cert but needs to verify platform
 * is aware of it (e.g., after re-flash or platform migration).
 *
 * @return true if platform confirms it has our certificate
 */
bool check_platform_certificate(void)
{
    char device_id[128];
    optiga_read_factory_uid(device_id, sizeof(device_id));

    char correlation_id[64];
    generate_smart_fallback_correlation_id(correlation_id, sizeof(correlation_id));

    size_t json_len = 256;
    char *temp_json = (char *)pvPortMalloc(json_len);
    if (!temp_json)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: malloc failed for check request\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

    snprintf(temp_json, json_len, "{\"correlation_id\":\"%s\"}", correlation_id);

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Asking Platform: Do you have my certificate?\n", LABEL_SMART_FALLBACK);
#endif

    __DSB();
    __DMB();
    __ISB();

    int ret = trustm_publish_command_payload("check_certificate", temp_json, strlen(temp_json),
                                             true, TRUSTM_CSR_MAX_ATTEMPTS, TRUSTM_CSR_RETRY_DELAY_MS);
    if (ret != 0)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Failed to publish check request\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

    /* Wait for platform response via semaphore */
    check_certificate_response_received = false;
    platform_has_certificate = false;

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Waiting for Platform response (5s timeout)...\n", LABEL_SMART_FALLBACK);
#endif
    BaseType_t sem_result = xSemaphoreTake(check_certificate_response_semaphore, pdMS_TO_TICKS(5000));

    if (sem_result == pdTRUE && check_certificate_response_received)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Platform %s certificate\n", LABEL_SMART_FALLBACK,
               platform_has_certificate ? "has" : "does NOT have");
#endif
        return platform_has_certificate;
    }
    else
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Timeout waiting for Platform response\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }
}

/**
 * Upload device certificate from OPTIGA to TESAIoT platform.
 *
 * Reads certificate from OID 0xE0E1, base64-encodes it, and publishes
 * to platform via MQTT. Used when device has a cert but platform is
 * missing it (sync scenario).
 *
 * Flow:
 *   1. Read DER certificate from OPTIGA
 *   2. Base64-encode for JSON transport
 *   3. Publish to "commands/upload_certificate" topic
 *   4. Wait for platform acknowledgment (5s timeout)
 *
 * @return true if platform confirms successful upload
 */
bool upload_device_certificate_to_platform(void)
{
    uint16_t cert_size = 2048;
    uint8_t cert_der[2048];
    optiga_lib_status_t status;
    optiga_util_t *me_util = NULL;

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Reading Device Certificate from OID 0xE0E1...\n", LABEL_SMART_FALLBACK);
#endif

    me_util = optiga_util_create(0, trustm_optiga_callback, NULL);
    if (!me_util)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: optiga_util_create failed\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

    trustm_optiga_status = OPTIGA_LIB_BUSY;
    status = optiga_util_read_data(me_util, TESAIOT_DEVICE_CERT_OID, 0, cert_der, &cert_size);
    (void)status;

    while (trustm_optiga_status == OPTIGA_LIB_BUSY)
        vTaskDelay(pdMS_TO_TICKS(10));

    optiga_util_destroy(me_util);

    if (trustm_optiga_status != OPTIGA_LIB_SUCCESS || cert_size == 0)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Cannot read device certificate\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Read device certificate: %u bytes DER\n", LABEL_SMART_FALLBACK, cert_size);
#endif

    /* Base64 encode the DER certificate for JSON transport */
    size_t encoded_buffer_len = ((cert_size + 2) / 3) * 4 + 1;
    char *cert_b64 = (char *)pvPortMalloc(encoded_buffer_len);
    if (!cert_b64)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: malloc failed for Base64 buffer\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

    size_t encoded_len = 0;
    int ret = mbedtls_base64_encode((unsigned char *)cert_b64, encoded_buffer_len,
                                    &encoded_len, cert_der, cert_size);
    if (ret != 0)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Base64 encoding failed: -0x%04X\n", LABEL_SMART_FALLBACK, -ret);
#endif
        vPortFree(cert_b64);
        return false;
    }
    cert_b64[encoded_len] = '\0';

    char device_id[128];
    optiga_read_factory_uid(device_id, sizeof(device_id));

    char correlation_id[64];
    generate_smart_fallback_correlation_id(correlation_id, sizeof(correlation_id));

    /* Build upload request JSON */
    size_t json_len = 4096 + encoded_len;
    char *temp_json = (char *)pvPortMalloc(json_len);
    if (!temp_json)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: malloc failed for JSON payload\n", LABEL_SMART_FALLBACK);
#endif
        vPortFree(cert_b64);
        return false;
    }

    snprintf(temp_json, json_len,
        "{\"certificate_b64\":\"%s\",\"certificate_format\":\"DER\","
        "\"target_oid\":\"E0E1\",\"trust_anchor_oid\":\"E0E3\",\"correlation_id\":\"%s\"}",
        cert_b64, correlation_id);

    vPortFree(cert_b64);

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Uploading certificate to Platform...\n", LABEL_SMART_FALLBACK);
#endif

    __DSB();
    __DMB();
    __ISB();

    ret = trustm_publish_command_payload("upload_certificate", temp_json, strlen(temp_json),
                                         true, TRUSTM_CSR_MAX_ATTEMPTS, TRUSTM_CSR_RETRY_DELAY_MS);
    if (ret != 0)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Failed to publish upload request\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

    upload_certificate_response_received = false;
    certificate_upload_success = false;

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Waiting for Platform response (5s timeout)...\n", LABEL_SMART_FALLBACK);
#endif
    BaseType_t sem_result = xSemaphoreTake(upload_certificate_response_semaphore, pdMS_TO_TICKS(5000));

    if (sem_result == pdTRUE && upload_certificate_response_received)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Certificate upload %s\n", LABEL_SMART_FALLBACK,
               certificate_upload_success ? "SUCCESS" : "FAILED");
#endif
        return certificate_upload_success;
    }
    else
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Timeout waiting for Platform response\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }
}

/**
 * Unified certificate synchronization with TESAIoT platform.
 *
 * Single command that handles all sync scenarios:
 *   - Device has cert, platform missing: uploads to platform
 *   - Platform has cert, device missing: triggers download
 *   - Both have cert: verifies they match
 *
 * This simplifies application code - just call sync and platform
 * determines what action is needed.
 *
 * Uses 10-second timeout (longer than other operations because platform
 * may need to perform additional verification).
 *
 * @return true if synchronization successful
 */
bool sync_certificate_with_platform(void)
{
#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s UNIFIED CERTIFICATE SYNC\n", LABEL_SMART_FALLBACK);
#endif

    uint16_t cert_size = 2048;
    uint8_t cert_der[2048];
    optiga_lib_status_t status;
    optiga_util_t *me_util = NULL;

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Reading device certificate from OID 0xE0E1...\n", LABEL_SMART_FALLBACK);
#endif

    me_util = optiga_util_create(0, trustm_optiga_callback, NULL);
    if (!me_util)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: optiga_util_create failed\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

    trustm_optiga_status = OPTIGA_LIB_BUSY;
    status = optiga_util_read_data(me_util, TESAIOT_DEVICE_CERT_OID, 0, cert_der, &cert_size);
    (void)status;

    while (trustm_optiga_status == OPTIGA_LIB_BUSY)
        vTaskDelay(pdMS_TO_TICKS(10));

    optiga_util_destroy(me_util);

    if (trustm_optiga_status != OPTIGA_LIB_SUCCESS || cert_size == 0)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Cannot read device certificate\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Read device certificate: %u bytes DER\n", LABEL_SMART_FALLBACK, cert_size);
#endif

    /* Base64 encode for transport */
    size_t encoded_buffer_len = ((cert_size + 2) / 3) * 4 + 1;
    char *cert_b64 = (char *)pvPortMalloc(encoded_buffer_len);
    if (!cert_b64)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: malloc failed for Base64 buffer\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

    size_t encoded_len = 0;
    int ret = mbedtls_base64_encode((unsigned char *)cert_b64, encoded_buffer_len,
                                    &encoded_len, cert_der, cert_size);
    if (ret != 0)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Base64 encoding failed: -0x%04X\n", LABEL_SMART_FALLBACK, -ret);
#endif
        vPortFree(cert_b64);
        return false;
    }
    cert_b64[encoded_len] = '\0';

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Encoded certificate: %u bytes Base64\n", LABEL_SMART_FALLBACK, (unsigned int)encoded_len);
#endif

    char device_id[128];
    optiga_read_factory_uid(device_id, sizeof(device_id));

    size_t json_len = 4096 + encoded_len;
    char *temp_json = (char *)pvPortMalloc(json_len);
    if (!temp_json)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: malloc failed for JSON payload\n", LABEL_SMART_FALLBACK);
#endif
        vPortFree(cert_b64);
        return false;
    }

    snprintf(temp_json, json_len,
        "{\"device_id\":\"%s\",\"action\":\"sync_certificate\"}", device_id);

    vPortFree(cert_b64);

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s JSON payload ready: %u bytes\n", LABEL_SMART_FALLBACK, (unsigned int)strlen(temp_json));
#endif

    __DSB();
    __DMB();
    __ISB();

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Publishing unified sync request...\n", LABEL_SMART_FALLBACK);
#endif

    ret = trustm_publish_command_payload("sync_certificate", temp_json, strlen(temp_json),
                                         true, TRUSTM_CSR_MAX_ATTEMPTS, TRUSTM_CSR_RETRY_DELAY_MS);
    if (ret != 0)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Failed to publish sync request\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Sync request published successfully\n", LABEL_SMART_FALLBACK);
#endif

    sync_certificate_response_received = false;
    certificate_sync_success = false;

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Waiting for Platform sync response (10s timeout)...\n", LABEL_SMART_FALLBACK);
#endif
    BaseType_t sem_result = xSemaphoreTake(sync_certificate_response_semaphore, pdMS_TO_TICKS(10000));

    if (sem_result == pdTRUE && sync_certificate_response_received)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Certificate %s\n", LABEL_SMART_FALLBACK,
               certificate_sync_success ? "SYNCHRONIZED" : "sync FAILED");
#endif
        return certificate_sync_success;
    }
    else
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s Timeout waiting for Platform sync response\n", LABEL_SMART_FALLBACK);
#endif
        return false;
    }
}

/*******************************************************************************
 * OPTIGA Session Management
 *
 * OPTIGA Trust M caches frequently-accessed data (including Trust Anchor
 * public keys) in RAM for performance. After writing new data to NVM,
 * the cache may still contain old values.
 *
 * To force a reload from NVM, we must:
 *   1. Close the OPTIGA application session
 *   2. Wait for NVM write completion (~1 second)
 *   3. Reopen the session (triggers cache reload)
 *
 * This is critical after Trust Anchor rotation - if we don't reset,
 * manifest signature verification will use the old (cached) key and fail.
 ******************************************************************************/

/**
 * Reset OPTIGA session to force Trust Anchor reload from NVM.
 *
 * Must be called after writing a new Trust Anchor to OID 0xE0E3.
 * Without this reset, OPTIGA continues using cached (old) key values,
 * causing Protected Update manifest verification to fail.
 *
 * Sequence:
 *   1. close_application() - ends current session, flushes pending writes
 *   2. Wait 1 second - ensures NVM write completes
 *   3. open_application() - starts new session, reloads from NVM
 *
 * Uses async callback pattern with polling for completion.
 *
 * @param status_ptr  Pointer to status variable for async tracking
 * @param callback    Callback function for OPTIGA async operations
 * @return OPTIGA_LIB_SUCCESS on success, error code on failure
 */
optiga_lib_status_t tesaiot_reset_optiga_session(volatile optiga_lib_status_t *status_ptr,
                                                  callback_handler_t callback)
{
    optiga_lib_status_t status = 0xFFFF;

    if (!status_ptr || !callback)
    {
#if TESAIOT_DEBUG_ERROR_ENABLED
        printf("%s ERROR: Invalid parameters\n", LABEL_ERROR);
#endif
        return OPTIGA_UTIL_ERROR;
    }

#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s Resetting OPTIGA session to reload Trust Anchor from NVM...\n", LABEL_TRUSTM);
#endif

    /* Close current session to flush any cached data */
    optiga_util_t *me_close = optiga_util_create(0, callback, NULL);
    if (!me_close)
    {
#if TESAIOT_DEBUG_ERROR_ENABLED
        printf("%s ERROR: optiga_util_create() failed for session close\n", LABEL_ERROR);
#endif
        return OPTIGA_UTIL_ERROR;
    }

    *status_ptr = OPTIGA_LIB_BUSY;
    status = optiga_util_close_application(me_close, 0);

    if (status == OPTIGA_LIB_SUCCESS)
    {
        uint32_t timeout_ms = 2000, elapsed_ms = 0;
        while (*status_ptr == OPTIGA_LIB_BUSY && elapsed_ms < timeout_ms)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            elapsed_ms += 100;
        }

#if TESAIOT_DEBUG_WARNING_ENABLED
        if (*status_ptr != OPTIGA_LIB_SUCCESS)
            printf("%s WARN: Close application returned status 0x%04X\n", LABEL_WARN, *status_ptr);
#endif
    }

    optiga_util_destroy(me_close);

    /* Wait for NVM write to complete before reopening */
#if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("%s OPTIGA session closed, waiting 1s for NVM commit...\n", LABEL_TRUSTM);
#endif
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Reopen session - this forces reload of Trust Anchor from NVM */
    optiga_util_t *me_reopen = optiga_util_create(0, callback, NULL);
    if (!me_reopen)
    {
#if TESAIOT_DEBUG_ERROR_ENABLED
        printf("%s ERROR: optiga_util_create() failed for session reopen\n", LABEL_ERROR);
#endif
        return OPTIGA_UTIL_ERROR;
    }

    *status_ptr = OPTIGA_LIB_BUSY;
    status = optiga_util_open_application(me_reopen, 0);

    if (status == OPTIGA_LIB_SUCCESS)
    {
        uint32_t timeout_ms = 2000, elapsed_ms = 0;
        while (*status_ptr == OPTIGA_LIB_BUSY && elapsed_ms < timeout_ms)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            elapsed_ms += 100;
        }
    }

    optiga_util_destroy(me_reopen);

    if (*status_ptr == OPTIGA_LIB_SUCCESS)
    {
#if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s OPTIGA session reopened - Trust Anchor reloaded from NVM\n", LABEL_TRUSTM);
#endif
        return OPTIGA_LIB_SUCCESS;
    }
    else
    {
#if TESAIOT_DEBUG_WARNING_ENABLED
        printf("%s WARNING: OPTIGA reopen status: 0x%04X\n", LABEL_WARN, *status_ptr);
#endif
        return *status_ptr;
    }
}
