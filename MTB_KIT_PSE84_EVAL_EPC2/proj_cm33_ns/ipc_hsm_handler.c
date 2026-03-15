/*******************************************************************************
 * File Name: ipc_hsm_handler.c
 *
 * Description: HSM IPC handler - CM33_NS side.
 *              Receives HSM IPC commands from CM55 (page_hsm.c),
 *              performs OPTIGA Trust M operations, returns response
 *              via shared memory.
 *
 *              Supported commands:
 *                IPC_CMD_HSM_REQUEST   - Read chip data (UID, LCS, certs, counters)
 *                IPC_CMD_HSM_BENCHMARK - Run crypto benchmarks (RNG, SHA, ECC)
 *                IPC_CMD_HSM_READ_CERT - Read + parse X.509 cert DER
 *                IPC_CMD_HSM_PIN_CHECK - Check if PIN exists in DATA_3
 *                IPC_CMD_HSM_PIN_SET   - Store SHA-256(PIN) in OPTIGA DATA_3
 *                IPC_CMD_HSM_PIN_VERIFY- Verify PIN against stored hash
 *                IPC_CMD_HSM_HEALTH    - Run 8 self-tests
 *
 *              Architecture:
 *                ISR callback -> semaphore -> static task -> OPTIGA ops
 *                -> write ipc_response_t -> set ready=1
 *
 *******************************************************************************/

#include "ipc_hsm_handler.h"
#include "ipc_communication.h"
#include "optiga_util.h"
#include "optiga_crypt.h"
#include "optiga_lib_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * Static task + semaphore (all statically allocated)
 *******************************************************************************/
#define HSM_TASK_STACK_WORDS  1024  /* 4KB - enough for OPTIGA crypto ops */
#define HSM_TASK_PRIORITY     3

static StackType_t          s_hsm_stack[HSM_TASK_STACK_WORDS];
static StaticTask_t         s_hsm_tcb;
static SemaphoreHandle_t    s_hsm_sem;
static StaticSemaphore_t    s_hsm_sem_buf;
static volatile ipc_msg_t  *s_hsm_pending_msg;

/* Mutex serializing all OPTIGA access (HSM task + direct API callers) */
static SemaphoreHandle_t    s_optiga_mutex;
static StaticSemaphore_t    s_optiga_mutex_buf;

/*******************************************************************************
 * Touch pause/resume — SCB0 shared with CM55 touch (FT5406).
 * Must pause touch polling before any OPTIGA I2C access.
 ******************************************************************************/
CY_SECTION_SHAREDMEM static ipc_msg_t s_touch_ipc_msg;

static void hsm_touch_send(uint32_t cmd)
{
    memset(&s_touch_ipc_msg, 0, sizeof(s_touch_ipc_msg));
    s_touch_ipc_msg.client_id = CM55_IPC_SENSOR_CLIENT_ID;
    s_touch_ipc_msg.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP1;
    s_touch_ipc_msg.cmd       = cmd;
    s_touch_ipc_msg.value     = 0;

    for (int i = 0; i < 50; i++) {
        if (CY_IPC_PIPE_SUCCESS == Cy_IPC_Pipe_SendMessage(
                CM55_IPC_PIPE_EP_ADDR, CM33_IPC_PIPE_EP_ADDR,
                (void *)&s_touch_ipc_msg, NULL)) {
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    printf("[HSM] WARN: touch IPC send failed (cmd=0x%02lX)\r\n",
           (unsigned long)cmd);
}

/*******************************************************************************
 * OPTIGA async wait pattern
 *******************************************************************************/
static volatile optiga_lib_status_t s_optiga_status;

static void optiga_cb(void *ctx, optiga_lib_status_t status)
{
    (void)ctx;
    s_optiga_status = status;
}

static optiga_lib_status_t wait_optiga(uint32_t timeout_ms)
{
    while (s_optiga_status == OPTIGA_LIB_BUSY && timeout_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (timeout_ms >= 10) timeout_ms -= 10;
        else timeout_ms = 0;
    }
    return s_optiga_status;
}

/*******************************************************************************
 * OPTIGA instance helpers - open/close for each command
 *******************************************************************************/
static optiga_util_t  *s_util;
static optiga_crypt_t *s_crypt;

static bool optiga_open(void)
{
    s_util = optiga_util_create(0, optiga_cb, NULL);
    if (!s_util) return false;

    s_crypt = optiga_crypt_create(0, optiga_cb, NULL);
    if (!s_crypt) {
        optiga_util_destroy(s_util);
        s_util = NULL;
        return false;
    }

    s_optiga_status = OPTIGA_LIB_BUSY;
    optiga_util_open_application(s_util, 0);
    if (wait_optiga(5000) != OPTIGA_LIB_SUCCESS) {
        optiga_crypt_destroy(s_crypt);
        optiga_util_destroy(s_util);
        s_crypt = NULL;
        s_util = NULL;
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(500));  /* Stabilization delay */
    return true;
}

static void optiga_close(void)
{
    if (s_util) {
        s_optiga_status = OPTIGA_LIB_BUSY;
        optiga_util_close_application(s_util, 0);
        wait_optiga(5000);
    }
    if (s_crypt) { optiga_crypt_destroy(s_crypt); s_crypt = NULL; }
    if (s_util)  { optiga_util_destroy(s_util);  s_util = NULL; }
}

/*******************************************************************************
 * Read OID helper
 *******************************************************************************/
static uint16_t read_oid(uint16_t oid, uint8_t *buf, uint16_t buf_len)
{
    uint16_t len = buf_len;
    s_optiga_status = OPTIGA_LIB_BUSY;
    optiga_lib_status_t rc = optiga_util_read_data(s_util, oid, 0, buf, &len);
    if (rc != OPTIGA_LIB_SUCCESS) return 0;
    if (wait_optiga(5000) != OPTIGA_LIB_SUCCESS) return 0;
    return len;
}

/*******************************************************************************
 * CMD: IPC_CMD_HSM_REQUEST - Read chip data (UID, LCS, certs, counters)
 *******************************************************************************/
static void handle_hsm_request(ipc_response_t *resp)
{
    uint8_t tmp[64];

    /* UID (27 bytes, OID 0xE0C2) */
    uint16_t len = read_oid(0xE0C2, tmp, 27);
    if (len > 0 && len <= 27)
        memcpy((void *)&resp->data[HSM_RESP_UID_OFF], tmp, len);

    /* LCS (1 byte, OID 0xE0C5) */
    len = read_oid(0xE0C5, tmp, 1);
    if (len > 0)
        resp->data[HSM_RESP_LCS_OFF] = tmp[0];

    /* Counter 0 (4 bytes, OID 0xE120) */
    len = read_oid(0xE120, tmp, 4);
    if (len >= 4)
        memcpy((void *)&resp->data[HSM_RESP_CTR0_OFF], tmp, 4);

    /* Counter 1 (4 bytes, OID 0xE121) */
    len = read_oid(0xE121, tmp, 4);
    if (len >= 4)
        memcpy((void *)&resp->data[HSM_RESP_CTR1_OFF], tmp, 4);

    /* Certificate presence (E0E0..E0E3) */
    static const uint16_t cert_oids[] = {0xE0E0, 0xE0E1, 0xE0E2, 0xE0E3};
    for (int i = 0; i < 4; i++) {
        len = read_oid(cert_oids[i], tmp, 8);
        resp->data[HSM_RESP_CERT_OFF + i] = (len > 0) ? 1 : 0;
    }

    /* Health: LCS 0x07 = operational */
    resp->data[HSM_RESP_HEALTH_OFF] =
        (resp->data[HSM_RESP_LCS_OFF] == 0x07) ? 1 : 0;

    resp->data_len = HSM_RESP_TOTAL_LEN;
    printf("[HSM] REQUEST OK (UID[0..2]=%02X%02X%02X)\r\n",
           resp->data[0], resp->data[1], resp->data[2]);
}

/*******************************************************************************
 * CMD: IPC_CMD_HSM_BENCHMARK - Run 5 real crypto benchmarks
 *
 * Response: 5 x uint16_t LE (ms) at offsets defined in ipc_hsm_handler.h
 *   [0] Random 32B, [2] SHA-256 256B, [4] ECC P-256 Keygen,
 *   [6] ECDSA Sign, [8] ECDSA Verify
 *******************************************************************************/
static void handle_hsm_benchmark(ipc_response_t *resp)
{
    uint16_t timings[HSM_BENCH_COUNT];
    memset(timings, 0, sizeof(timings));

    TickType_t t0, t1;
    uint8_t buf[256];

    /* 1. Random 32 bytes */
    t0 = xTaskGetTickCount();
    s_optiga_status = OPTIGA_LIB_BUSY;
    optiga_crypt_random(s_crypt, OPTIGA_RNG_TYPE_DRNG, buf, 32);
    wait_optiga(5000);
    t1 = xTaskGetTickCount();
    timings[0] = (uint16_t)(t1 - t0);

    /* 2. SHA-256 hash of 256 bytes */
    memset(buf, 0xAA, 256);
    hash_data_from_host_t hdata;
    hdata.buffer = buf;
    hdata.length = 256;
    uint8_t hash_out[32];

    t0 = xTaskGetTickCount();
    s_optiga_status = OPTIGA_LIB_BUSY;
    optiga_crypt_hash(s_crypt, OPTIGA_HASH_TYPE_SHA_256,
                      OPTIGA_CRYPT_HOST_DATA, &hdata, hash_out);
    wait_optiga(5000);
    t1 = xTaskGetTickCount();
    timings[1] = (uint16_t)(t1 - t0);

    /* 3. ECC P-256 Key Generation (session key) */
    uint8_t pubkey[68];
    uint16_t pubkey_len = sizeof(pubkey);

    t0 = xTaskGetTickCount();
    s_optiga_status = OPTIGA_LIB_BUSY;
    optiga_crypt_ecc_generate_keypair(s_crypt, OPTIGA_ECC_CURVE_NIST_P_256,
                                       (uint8_t)(OPTIGA_KEY_USAGE_SIGN),
                                       FALSE,
                                       &(optiga_key_id_t){OPTIGA_KEY_ID_SESSION_BASED},
                                       pubkey, &pubkey_len);
    wait_optiga(10000);
    t1 = xTaskGetTickCount();
    timings[2] = (uint16_t)(t1 - t0);

    /* 4. ECDSA Sign (sign hash_out with session key) */
    uint8_t sig[80];
    uint16_t sig_len = sizeof(sig);

    t0 = xTaskGetTickCount();
    s_optiga_status = OPTIGA_LIB_BUSY;
    optiga_crypt_ecdsa_sign(s_crypt, hash_out, 32,
                            OPTIGA_KEY_ID_SESSION_BASED, sig, &sig_len);
    wait_optiga(10000);
    t1 = xTaskGetTickCount();
    timings[3] = (uint16_t)(t1 - t0);

    /* 5. ECDSA Verify (verify sig with pubkey) */
    public_key_from_host_t pk;
    pk.public_key = pubkey;
    pk.length = pubkey_len;
    pk.key_type = OPTIGA_ECC_CURVE_NIST_P_256;

    t0 = xTaskGetTickCount();
    s_optiga_status = OPTIGA_LIB_BUSY;
    optiga_crypt_ecdsa_verify(s_crypt, hash_out, 32, sig, sig_len,
                              OPTIGA_CRYPT_HOST_DATA, &pk);
    wait_optiga(10000);
    t1 = xTaskGetTickCount();
    timings[4] = (uint16_t)(t1 - t0);

    /* Pack as uint16_t LE into response */
    for (int i = 0; i < HSM_BENCH_COUNT; i++) {
        resp->data[i * 2]     = (uint8_t)(timings[i] & 0xFF);
        resp->data[i * 2 + 1] = (uint8_t)(timings[i] >> 8);
    }
    resp->data_len = HSM_BENCH_TOTAL_LEN;

    printf("[HSM] BENCHMARK OK: RNG=%u SHA=%u KG=%u Sign=%u Vfy=%u ms\r\n",
           timings[0], timings[1], timings[2], timings[3], timings[4]);
}

/*******************************************************************************
 * CMD: IPC_CMD_HSM_READ_CERT - Read + parse X.509 cert DER
 *
 * Input:  resp->cmd = cert slot index (0-3)
 * Output: data[0..1] = DER size (uint16_t LE)
 *         data[2..]  = packed null-terminated strings:
 *           [subject_cn\0][issuer_cn\0][org\0][country\0][not_before\0][not_after\0]
 *******************************************************************************/

/* Cert DER buffer (max ~1728 bytes per OPTIGA cert slot) */
static uint8_t s_cert_der[1728];

/* Parse DER tag + length, return pointer past TL, set *out_len */
static const uint8_t *der_tl(const uint8_t *p, const uint8_t *end,
                              uint8_t *out_tag, uint16_t *out_len)
{
    if (p >= end) return NULL;
    *out_tag = *p++;
    if (p >= end) return NULL;

    if (*p < 0x80) {
        *out_len = *p++;
    } else if (*p == 0x81) {
        p++;
        if (p >= end) return NULL;
        *out_len = *p++;
    } else if (*p == 0x82) {
        p++;
        if (p + 1 >= end) return NULL;
        *out_len = ((uint16_t)p[0] << 8) | p[1];
        p += 2;
    } else {
        return NULL;  /* Unsupported length encoding */
    }
    return p;
}

/* Skip one TLV element, returning pointer past it */
static const uint8_t *der_skip(const uint8_t *p, const uint8_t *end)
{
    uint8_t tag;
    uint16_t len;
    p = der_tl(p, end, &tag, &len);
    if (!p) return NULL;
    p += len;
    return (p <= end) ? p : NULL;
}

/* Search RDN sequence for OID suffix (55 04 XX) and extract UTF8/PrintableString */
static bool find_rdn_attr(const uint8_t *seq, uint16_t seq_len,
                           uint8_t oid_last_byte,
                           char *out, uint16_t out_sz)
{
    const uint8_t *p = seq;
    const uint8_t *end = seq + seq_len;
    out[0] = '\0';

    while (p < end) {
        uint8_t tag;
        uint16_t setlen;
        const uint8_t *set_start = der_tl(p, end, &tag, &setlen);
        if (!set_start || tag != 0x31) { p = der_skip(p, end); if (!p) break; continue; }

        const uint8_t *set_end = set_start + setlen;
        const uint8_t *sp = set_start;

        while (sp < set_end) {
            uint8_t stag;
            uint16_t s_seq_len;
            const uint8_t *seq_p = der_tl(sp, set_end, &stag, &s_seq_len);
            if (!seq_p || stag != 0x30) break;
            const uint8_t *seq_end = seq_p + s_seq_len;

            /* OID */
            uint8_t oid_tag;
            uint16_t oid_len;
            const uint8_t *oid_val = der_tl(seq_p, seq_end, &oid_tag, &oid_len);
            if (!oid_val || oid_tag != 0x06) break;

            /* Check OID = 55 04 XX */
            if (oid_len >= 3 && oid_val[0] == 0x55 && oid_val[1] == 0x04 &&
                oid_val[2] == oid_last_byte) {
                const uint8_t *val_p = oid_val + oid_len;
                uint8_t vtag;
                uint16_t vlen;
                const uint8_t *vdata = der_tl(val_p, seq_end, &vtag, &vlen);
                if (vdata && vlen > 0) {
                    uint16_t copy = (vlen < out_sz - 1) ? vlen : (out_sz - 1);
                    memcpy(out, vdata, copy);
                    out[copy] = '\0';
                    return true;
                }
            }
            sp = seq_end;
        }
        p = set_start + setlen;
    }
    return false;
}

/* Format UTCTime (YYMMDDHHMMSSZ) or GeneralizedTime (YYYYMMDDHHMMSSZ) */
static void fmt_time(const uint8_t *val, uint16_t len, char *out, uint16_t out_sz)
{
    if (len >= 13 && len <= 15) {
        /* UTCTime: YYMMDDHHMMSSZ */
        int yr = (val[0] - '0') * 10 + (val[1] - '0');
        yr += (yr >= 50) ? 1900 : 2000;
        snprintf(out, out_sz, "%04d-%c%c-%c%c",
                 yr, val[2], val[3], val[4], val[5]);
    } else if (len >= 15) {
        /* GeneralizedTime: YYYYMMDDHHMMSSZ */
        snprintf(out, out_sz, "%c%c%c%c-%c%c-%c%c",
                 val[0], val[1], val[2], val[3],
                 val[4], val[5], val[6], val[7]);
    } else {
        snprintf(out, out_sz, "N/A");
    }
}

static void handle_hsm_read_cert(ipc_response_t *resp, const ipc_msg_t *msg)
{
    static const uint16_t cert_oids[] = {0xE0E0, 0xE0E1, 0xE0E2, 0xE0E3};
    int slot = (uint8_t)msg->data[0];  /* Slot index passed in msg data[0] */
    if (slot < 0 || slot > 3) {
        resp->status = 1;
        return;
    }

    uint16_t der_len = read_oid(cert_oids[slot], s_cert_der, sizeof(s_cert_der));
    if (der_len == 0) {
        resp->data[0] = 0;
        resp->data[1] = 0;
        resp->data_len = 2;
        printf("[HSM] CERT slot %d: empty (read returned 0)\r\n", slot);
        return;
    }

    /* Scan for DER SEQUENCE header (0x30 0x82) to skip OPTIGA wrapper.
     * OPTIGA cert format varies per slot:
     *   E0E0 factory: C0 [len2] [type_byte] [chain_len2] [type_byte] [cert_len2] 30 82 ...
     *   E0E1 project: C0 [len2] C1 [len2] C2 [len2] 30 82 ...
     * Scanning for 0x30 0x82 is the most robust approach. */
    const uint8_t *cert_p = s_cert_der;
    const uint8_t *cert_end = s_cert_der + der_len;
    while (cert_p + 4 <= cert_end) {
        if (cert_p[0] == 0x30 && cert_p[1] == 0x82)
            break;
        cert_p++;
    }
    if (cert_p + 4 > cert_end || cert_p[0] != 0x30) {
        /* No DER SEQUENCE found - slot is empty or corrupted */
        resp->data[0] = 0;
        resp->data[1] = 0;
        resp->data_len = 2;
        printf("[HSM] CERT slot %d: no DER found in %u bytes\r\n", slot, der_len);
        return;
    }

    /* DER cert starts at cert_p; actual cert length from DER header */
    uint16_t der_content_len = ((uint16_t)cert_p[2] << 8) | cert_p[3];
    uint16_t actual_der_len = der_content_len + 4;  /* tag(1) + 0x82(1) + len(2) + content */

    /* Store actual DER cert size as uint16_t LE */
    resp->data[HSM_CERT_SIZE_OFF]     = (uint8_t)(actual_der_len & 0xFF);
    resp->data[HSM_CERT_SIZE_OFF + 1] = (uint8_t)(actual_der_len >> 8);

    /* Update cert_end to actual cert boundary */
    if (cert_p + actual_der_len < cert_end)
        cert_end = cert_p + actual_der_len;

    /* Parse Certificate SEQUENCE -> TBSCertificate SEQUENCE */
    uint8_t tag;
    uint16_t tlen;
    const uint8_t *tbs_outer = der_tl(cert_p, cert_end, &tag, &tlen);
    if (!tbs_outer || tag != 0x30) goto pack_empty;

    const uint8_t *tbs_start = der_tl(tbs_outer, cert_end, &tag, &tlen);
    if (!tbs_start || tag != 0x30) goto pack_empty;
    const uint8_t *tbs_end = tbs_start + tlen;

    /* Walk TBSCertificate fields:
     * [0] Version (EXPLICIT TAG A0), [1] Serial, [2] SigAlg,
     * [3] Issuer, [4] Validity, [5] Subject */
    const uint8_t *field = tbs_start;

    /* Version (context [0] EXPLICIT) - optional */
    if (field < tbs_end && *field == 0xA0) {
        field = der_skip(field, tbs_end);
        if (!field) goto pack_empty;
    }
    /* Serial */
    field = der_skip(field, tbs_end);
    if (!field) goto pack_empty;
    /* SigAlg */
    field = der_skip(field, tbs_end);
    if (!field) goto pack_empty;

    /* Issuer (SEQUENCE of RDN SETs) */
    const uint8_t *issuer_seq;
    uint16_t issuer_len;
    issuer_seq = der_tl(field, tbs_end, &tag, &issuer_len);
    if (!issuer_seq || tag != 0x30) goto pack_empty;
    const uint8_t *issuer_data = issuer_seq;
    field = issuer_seq + issuer_len;

    /* Validity (SEQUENCE { notBefore, notAfter }) */
    const uint8_t *val_seq;
    uint16_t val_seq_len;
    val_seq = der_tl(field, tbs_end, &tag, &val_seq_len);
    if (!val_seq || tag != 0x30) goto pack_empty;
    const uint8_t *validity_p = val_seq;
    const uint8_t *validity_end = val_seq + val_seq_len;
    field = validity_end;

    /* Subject (SEQUENCE of RDN SETs) */
    const uint8_t *subj_seq;
    uint16_t subj_len;
    subj_seq = der_tl(field, tbs_end, &tag, &subj_len);
    if (!subj_seq || tag != 0x30) goto pack_empty;

    /* Extract fields into packed strings */
    char subject_cn[64] = "";
    char issuer_cn[64]  = "";
    char org[64]        = "";
    char country[8]     = "";
    char not_before[20] = "";
    char not_after[20]  = "";

    /* OID 55 04 03 = CN, 55 04 0A = Org, 55 04 06 = Country
     * Search Subject first; fall back to Issuer for Org/Country
     * (Infineon factory cert has Org/Country only in Issuer) */
    find_rdn_attr(subj_seq, subj_len, 0x03, subject_cn, sizeof(subject_cn));
    find_rdn_attr(issuer_data, issuer_len, 0x03, issuer_cn, sizeof(issuer_cn));
    find_rdn_attr(subj_seq, subj_len, 0x0A, org, sizeof(org));
    if (org[0] == '\0')
        find_rdn_attr(issuer_data, issuer_len, 0x0A, org, sizeof(org));
    find_rdn_attr(subj_seq, subj_len, 0x06, country, sizeof(country));
    if (country[0] == '\0')
        find_rdn_attr(issuer_data, issuer_len, 0x06, country, sizeof(country));

    /* Parse Validity dates */
    {
        uint8_t vtag;
        uint16_t vlen;
        const uint8_t *vp = der_tl(validity_p, validity_end, &vtag, &vlen);
        if (vp) {
            fmt_time(vp, vlen, not_before, sizeof(not_before));
            const uint8_t *vp2 = vp + vlen;
            vp2 = der_tl(vp2, validity_end, &vtag, &vlen);
            if (vp2) fmt_time(vp2, vlen, not_after, sizeof(not_after));
        }
    }

    /* Pack strings: [subject_cn\0][issuer_cn\0][org\0][country\0][not_before\0][not_after\0] */
    {
        uint8_t *dst = (uint8_t *)&resp->data[HSM_CERT_STRINGS_OFF];
        uint8_t *dst_end = (uint8_t *)&resp->data[IPC_RESPONSE_DATA_MAX];
        const char *strs[] = { subject_cn, issuer_cn, org, country, not_before, not_after };
        for (int i = 0; i < 6; i++) {
            uint16_t slen = (uint16_t)strlen(strs[i]) + 1;
            if (dst + slen > dst_end) break;
            memcpy(dst, strs[i], slen);
            dst += slen;
        }
        resp->data_len = (uint16_t)(dst - resp->data);
    }

    printf("[HSM] CERT slot %d: CN=%s, Issuer=%s, DER=%u\r\n",
           slot, subject_cn, issuer_cn, der_len);
    return;

pack_empty:
    /* Could not parse - return DER size only */
    resp->data_len = 2;
    printf("[HSM] CERT slot %d: parse failed (DER=%u)\r\n", slot, der_len);
}

/*******************************************************************************
 * CMD: IPC_CMD_HSM_PIN_CHECK / PIN_SET / PIN_VERIFY
 *
 * PIN stored as SHA-256(4 digits) in OPTIGA DATA_3 (OID 0xF1D2).
 * Matches hsm_dashboard.py pattern.
 *******************************************************************************/
static void handle_hsm_pin(ipc_response_t *resp, uint8_t cmd,
                            const uint8_t *data)
{
    switch (cmd) {
    case IPC_CMD_HSM_PIN_RESET: {
        /* Erase PIN data from DATA_3 (write 0 bytes = erase) */
        uint8_t zeros[32];
        memset(zeros, 0, sizeof(zeros));
        s_optiga_status = OPTIGA_LIB_BUSY;
        optiga_lib_status_t rc = optiga_util_write_data(s_util, HSM_PIN_DATA_OID,
            OPTIGA_UTIL_ERASE_AND_WRITE, 0, zeros, 1);
        resp->data[0] = (rc == OPTIGA_LIB_SUCCESS &&
                         wait_optiga(5000) == OPTIGA_LIB_SUCCESS) ? 1 : 0;
        resp->data_len = 1;
        printf("[HSM] PIN_RESET: %s\r\n", resp->data[0] ? "OK" : "failed");
        break;
    }

    case IPC_CMD_HSM_PIN_CHECK: {
        /* Check if 32 bytes exist in DATA_3 */
        uint8_t tmp[32];
        uint16_t len = read_oid(HSM_PIN_DATA_OID, tmp, 32);
        resp->data[0] = (len == 32) ? 1 : 0;
        resp->data_len = 1;
        printf("[HSM] PIN_CHECK: %s\r\n", (len == 32) ? "set" : "not set");
        break;
    }

    case IPC_CMD_HSM_PIN_SET: {
        /* data[0..3] = 4 digit values; hash them with OPTIGA SHA-256 */
        uint8_t pin_bytes[4];
        memcpy(pin_bytes, data, 4);

        hash_data_from_host_t hdata;
        hdata.buffer = pin_bytes;
        hdata.length = 4;
        uint8_t hash[32];

        s_optiga_status = OPTIGA_LIB_BUSY;
        optiga_lib_status_t rc = optiga_crypt_hash(s_crypt,
            OPTIGA_HASH_TYPE_SHA_256, OPTIGA_CRYPT_HOST_DATA, &hdata, hash);
        if (rc != OPTIGA_LIB_SUCCESS || wait_optiga(5000) != OPTIGA_LIB_SUCCESS) {
            resp->status = 3;
            resp->data[0] = 0;
            resp->data_len = 1;
            printf("[HSM] PIN_SET hash failed\r\n");
            break;
        }

        /* Write hash to DATA_3 */
        s_optiga_status = OPTIGA_LIB_BUSY;
        rc = optiga_util_write_data(s_util, HSM_PIN_DATA_OID,
                                    OPTIGA_UTIL_ERASE_AND_WRITE, 0, hash, 32);
        if (rc != OPTIGA_LIB_SUCCESS || wait_optiga(5000) != OPTIGA_LIB_SUCCESS) {
            resp->status = 4;
            resp->data[0] = 0;
            resp->data_len = 1;
            printf("[HSM] PIN_SET write failed\r\n");
            break;
        }

        resp->data[0] = 1;  /* Success */
        resp->data_len = 1;
        printf("[HSM] PIN_SET OK\r\n");
        break;
    }

    case IPC_CMD_HSM_PIN_VERIFY: {
        /* data[0..3] = 4 digit values; hash and compare with stored */
        uint8_t pin_bytes[4];
        memcpy(pin_bytes, data, 4);

        hash_data_from_host_t hdata;
        hdata.buffer = pin_bytes;
        hdata.length = 4;
        uint8_t hash[32];

        s_optiga_status = OPTIGA_LIB_BUSY;
        optiga_lib_status_t rc = optiga_crypt_hash(s_crypt,
            OPTIGA_HASH_TYPE_SHA_256, OPTIGA_CRYPT_HOST_DATA, &hdata, hash);
        if (rc != OPTIGA_LIB_SUCCESS || wait_optiga(5000) != OPTIGA_LIB_SUCCESS) {
            resp->status = 3;
            resp->data[0] = 0;
            resp->data_len = 1;
            break;
        }

        /* Read stored hash */
        uint8_t stored[32];
        uint16_t len = read_oid(HSM_PIN_DATA_OID, stored, 32);
        if (len != 32) {
            resp->data[0] = 0;
            resp->data_len = 1;
            printf("[HSM] PIN_VERIFY: no stored hash\r\n");
            break;
        }

        resp->data[0] = (memcmp(hash, stored, 32) == 0) ? 1 : 0;
        resp->data_len = 1;
        printf("[HSM] PIN_VERIFY: %s\r\n", resp->data[0] ? "match" : "mismatch");
        break;
    }
    }
}

/*******************************************************************************
 * CMD: IPC_CMD_HSM_HEALTH - Run 8 self-tests
 *
 * Response: 8 bytes, each 1=pass 0=fail
 * Order: HW, UID, Cert, RNG, SHA, ECC, RW, Meta
 *******************************************************************************/
static void handle_hsm_health(ipc_response_t *resp)
{
    uint8_t results[HSM_HEALTH_TOTAL_LEN];
    memset(results, 0, sizeof(results));
    uint8_t tmp[64];

    /* 1. HW test - read Security Status (OID 0xE0C1) */
    results[HSM_HEALTH_HW_OFF] = (read_oid(0xE0C1, tmp, 4) > 0) ? 1 : 0;

    /* 2. UID test - read 27-byte UID */
    results[HSM_HEALTH_UID_OFF] = (read_oid(0xE0C2, tmp, 27) == 27) ? 1 : 0;

    /* 3. Cert test - read first cert slot (>100 bytes = valid) */
    {
        uint16_t len = read_oid(0xE0E0, s_cert_der, sizeof(s_cert_der));
        results[HSM_HEALTH_CERT_OFF] = (len > 100) ? 1 : 0;
    }

    /* 4. RNG test - generate 32 random bytes */
    {
        s_optiga_status = OPTIGA_LIB_BUSY;
        optiga_lib_status_t rc = optiga_crypt_random(s_crypt,
            OPTIGA_RNG_TYPE_DRNG, tmp, 32);
        results[HSM_HEALTH_RNG_OFF] =
            (rc == OPTIGA_LIB_SUCCESS && wait_optiga(5000) == OPTIGA_LIB_SUCCESS) ? 1 : 0;
    }

    /* 5. SHA test - hash 16 bytes */
    {
        memset(tmp, 0x42, 16);
        hash_data_from_host_t hdata;
        hdata.buffer = tmp;
        hdata.length = 16;
        uint8_t hash[32];

        s_optiga_status = OPTIGA_LIB_BUSY;
        optiga_lib_status_t rc = optiga_crypt_hash(s_crypt,
            OPTIGA_HASH_TYPE_SHA_256, OPTIGA_CRYPT_HOST_DATA, &hdata, hash);
        results[HSM_HEALTH_SHA_OFF] =
            (rc == OPTIGA_LIB_SUCCESS && wait_optiga(5000) == OPTIGA_LIB_SUCCESS) ? 1 : 0;
    }

    /* 6. ECC test - generate session keypair + sign */
    {
        uint8_t pubkey[68];
        uint16_t pk_len = sizeof(pubkey);

        s_optiga_status = OPTIGA_LIB_BUSY;
        optiga_lib_status_t rc = optiga_crypt_ecc_generate_keypair(
            s_crypt, OPTIGA_ECC_CURVE_NIST_P_256,
            (uint8_t)(OPTIGA_KEY_USAGE_SIGN), FALSE,
            &(optiga_key_id_t){OPTIGA_KEY_ID_SESSION_BASED},
            pubkey, &pk_len);
        if (rc == OPTIGA_LIB_SUCCESS && wait_optiga(10000) == OPTIGA_LIB_SUCCESS) {
            /* Also sign to fully verify ECC */
            uint8_t digest[32] = {0};
            uint8_t sig[80];
            uint16_t sig_len = sizeof(sig);
            s_optiga_status = OPTIGA_LIB_BUSY;
            rc = optiga_crypt_ecdsa_sign(s_crypt, digest, 32,
                                         OPTIGA_KEY_ID_SESSION_BASED, sig, &sig_len);
            results[HSM_HEALTH_ECC_OFF] =
                (rc == OPTIGA_LIB_SUCCESS && wait_optiga(10000) == OPTIGA_LIB_SUCCESS) ? 1 : 0;
        }
    }

    /* 7. R/W test - write 4 bytes to USER0 (0xF1D8) then read back.
     *    Using 0xF1D8 (not 0xF1D0) to avoid overwriting device ID. */
    {
        uint8_t test_data[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
        s_optiga_status = OPTIGA_LIB_BUSY;
        optiga_lib_status_t rc = optiga_util_write_data(s_util, 0xF1D8,
            OPTIGA_UTIL_ERASE_AND_WRITE, 0, test_data, 4);
        if (rc == OPTIGA_LIB_SUCCESS && wait_optiga(5000) == OPTIGA_LIB_SUCCESS) {
            uint8_t readback[4] = {0};
            uint16_t rlen = read_oid(0xF1D8, readback, 4);
            results[HSM_HEALTH_RW_OFF] =
                (rlen == 4 && memcmp(readback, test_data, 4) == 0) ? 1 : 0;
        }
    }

    /* 8. Meta test - read metadata of OID 0xE0E0 */
    {
        uint8_t meta[64];
        uint16_t mlen = sizeof(meta);
        s_optiga_status = OPTIGA_LIB_BUSY;
        optiga_lib_status_t rc = optiga_util_read_metadata(s_util, 0xE0E0, meta, &mlen);
        results[HSM_HEALTH_META_OFF] =
            (rc == OPTIGA_LIB_SUCCESS && wait_optiga(5000) == OPTIGA_LIB_SUCCESS && mlen > 0) ? 1 : 0;
    }

    memcpy((void *)resp->data, results, HSM_HEALTH_TOTAL_LEN);
    resp->data_len = HSM_HEALTH_TOTAL_LEN;

    int pass = 0;
    for (int i = 0; i < HSM_HEALTH_TOTAL_LEN; i++) pass += results[i];
    printf("[HSM] HEALTH: %d/8 tests passed\r\n", pass);
}

/*******************************************************************************
 * CMD: IPC_CMD_TESAIOT_CRED_READ / CRED_WRITE / CRED_ERASE
 *
 * Credential storage in OPTIGA data slots — used by wifi_saved.c on CM55.
 * CRED_READ:  data[0]=slot → response data[0..N], data_len=N
 * CRED_WRITE: data[0]=slot, data[1]=len, data[2..2+len-1]=payload
 * CRED_ERASE: data[0]=slot → erase by writing 1 zero byte
 *******************************************************************************/
static void handle_cred_read(ipc_response_t *resp, const ipc_msg_t *msg)
{
    uint8_t slot = (uint8_t)msg->data[0];
    uint16_t oid = tesaiot_slot_to_oid(slot);
    if (oid == 0) {
        resp->status = 2;
        printf("[HSM] CRED_READ: invalid slot %u\r\n", slot);
        return;
    }

    uint16_t len = read_oid(oid, (uint8_t *)resp->data, IPC_RESPONSE_DATA_MAX);
    resp->data_len = len;
    printf("[HSM] CRED_READ slot %u (OID 0x%04X): %u bytes\r\n", slot, oid, len);
}

static void handle_cred_write(ipc_response_t *resp, const ipc_msg_t *msg)
{
    uint8_t slot = (uint8_t)msg->data[0];
    uint8_t data_len = (uint8_t)msg->data[1];
    uint16_t oid = tesaiot_slot_to_oid(slot);
    if (oid == 0 || data_len == 0 || data_len > (IPC_DATA_MAX_LEN - 2)) {
        resp->status = 2;
        printf("[HSM] CRED_WRITE: invalid slot %u or len %u\r\n", slot, data_len);
        return;
    }

    s_optiga_status = OPTIGA_LIB_BUSY;
    optiga_lib_status_t rc = optiga_util_write_data(
        s_util, oid, OPTIGA_UTIL_ERASE_AND_WRITE, 0,
        (const uint8_t *)&msg->data[2], data_len);
    if (rc != OPTIGA_LIB_SUCCESS || wait_optiga(5000) != OPTIGA_LIB_SUCCESS) {
        resp->status = 3;
        printf("[HSM] CRED_WRITE slot %u: FAILED\r\n", slot);
        return;
    }
    printf("[HSM] CRED_WRITE slot %u (OID 0x%04X): %u bytes OK\r\n",
           slot, oid, data_len);
}

static void handle_cred_erase(ipc_response_t *resp, const ipc_msg_t *msg)
{
    uint8_t slot = (uint8_t)msg->data[0];
    uint16_t oid = tesaiot_slot_to_oid(slot);
    if (oid == 0) {
        resp->status = 2;
        printf("[HSM] CRED_ERASE: invalid slot %u\r\n", slot);
        return;
    }

    uint8_t zero = 0;
    s_optiga_status = OPTIGA_LIB_BUSY;
    optiga_lib_status_t rc = optiga_util_write_data(
        s_util, oid, OPTIGA_UTIL_ERASE_AND_WRITE, 0, &zero, 1);
    if (rc != OPTIGA_LIB_SUCCESS || wait_optiga(5000) != OPTIGA_LIB_SUCCESS) {
        resp->status = 3;
        printf("[HSM] CRED_ERASE slot %u: FAILED\r\n", slot);
        return;
    }
    printf("[HSM] CRED_ERASE slot %u (OID 0x%04X): OK\r\n", slot, oid);
}

/*******************************************************************************
 * HSM task - command dispatcher
 *******************************************************************************/
static void hsm_task_func(void *arg)
{
    (void)arg;

    for (;;) {
        if (xSemaphoreTake(s_hsm_sem, portMAX_DELAY) != pdTRUE)
            continue;

        ipc_msg_t *msg = (ipc_msg_t *)s_hsm_pending_msg;
        if (!msg) continue;

        ipc_response_t *resp = (ipc_response_t *)(uintptr_t)msg->value;
        if (!resp) continue;

        /* Initialize response */
        memset((void *)resp->data, 0, IPC_RESPONSE_DATA_MAX);
        resp->data_len = 0;
        resp->status   = 0;
        resp->cmd      = (uint8_t)msg->cmd;

        /* Acquire OPTIGA mutex — serializes with ipc_hsm_cred_read_sync() */
        xSemaphoreTake(s_optiga_mutex, portMAX_DELAY);

        /* Pause CM55 touch to get exclusive SCB0 I2C access */
        hsm_touch_send(IPC_CMD_TOUCH_PAUSE);
        vTaskDelay(pdMS_TO_TICKS(50));

        /* Open OPTIGA */
        if (!optiga_open()) {
            printf("[HSM] optiga_open failed for cmd 0x%02lX\r\n",
                   (unsigned long)msg->cmd);
            resp->status = 1;
            resp->ready  = 1;
            hsm_touch_send(IPC_CMD_TOUCH_RESUME);
            xSemaphoreGive(s_optiga_mutex);
            continue;
        }

        /* Dispatch command */
        switch (msg->cmd) {
        case IPC_CMD_HSM_REQUEST:
            handle_hsm_request(resp);
            break;
        case IPC_CMD_HSM_BENCHMARK:
            handle_hsm_benchmark(resp);
            break;
        case IPC_CMD_HSM_READ_CERT:
            handle_hsm_read_cert(resp, msg);
            break;
        case IPC_CMD_HSM_PIN_CHECK:
        case IPC_CMD_HSM_PIN_SET:
        case IPC_CMD_HSM_PIN_VERIFY:
        case IPC_CMD_HSM_PIN_RESET:
            handle_hsm_pin(resp, (uint8_t)msg->cmd,
                           (const uint8_t *)msg->data);
            break;
        case IPC_CMD_HSM_HEALTH:
            handle_hsm_health(resp);
            break;
        case IPC_CMD_TESAIOT_CRED_READ:
            handle_cred_read(resp, msg);
            break;
        case IPC_CMD_TESAIOT_CRED_WRITE:
            handle_cred_write(resp, msg);
            break;
        case IPC_CMD_TESAIOT_CRED_ERASE:
            handle_cred_erase(resp, msg);
            break;
        default:
            printf("[HSM] Unknown cmd 0x%02lX\r\n", (unsigned long)msg->cmd);
            resp->status = 0xFF;
            break;
        }

        /* Close OPTIGA */
        optiga_close();

        /* Resume CM55 touch polling */
        hsm_touch_send(IPC_CMD_TOUCH_RESUME);

        xSemaphoreGive(s_optiga_mutex);

        /* Signal response ready */
        resp->ready = 1;
    }
}

/*******************************************************************************
 * ISR callback - accepts all HSM commands (0xB5-0xBB)
 *******************************************************************************/
static void hsm_ipc_callback(uint32_t *msg_data)
{
    if (!msg_data) return;
    ipc_msg_t *msg = (ipc_msg_t *)msg_data;

    /* Accept commands in HSM range (0xB5-0xBC) or CRED range (0xA3,0xA4,0xA6) */
    bool is_hsm  = (msg->cmd >= IPC_CMD_HSM_REQUEST && msg->cmd <= IPC_CMD_HSM_PIN_RESET);
    bool is_cred = (msg->cmd == IPC_CMD_TESAIOT_CRED_READ  ||
                    msg->cmd == IPC_CMD_TESAIOT_CRED_WRITE ||
                    msg->cmd == IPC_CMD_TESAIOT_CRED_ERASE);
    if (!is_hsm && !is_cred)
        return;

    s_hsm_pending_msg = msg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(s_hsm_sem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************
 * Public init - call from main.c after cm33_ipc_communication_setup()
 *******************************************************************************/
/*******************************************************************************
 * Public API: direct credential read (blocks calling task, thread-safe)
 ******************************************************************************/
bool ipc_hsm_cred_read_sync(uint8_t slot, uint8_t *buf, uint16_t buf_len,
                             uint16_t *out_len)
{
    if (!buf || buf_len == 0) return false;
    if (!s_optiga_mutex) return false;

    uint16_t oid = tesaiot_slot_to_oid(slot);
    if (oid == 0) return false;

    if (xSemaphoreTake(s_optiga_mutex, pdMS_TO_TICKS(10000)) != pdTRUE) {
        printf("[HSM] cred_read_sync: mutex timeout (slot %u)\r\n", slot);
        return false;
    }

    /* Pause CM55 touch to get exclusive SCB0 I2C access */
    hsm_touch_send(IPC_CMD_TOUCH_PAUSE);
    vTaskDelay(pdMS_TO_TICKS(50));

    bool ok = false;
    if (optiga_open()) {
        uint16_t len = read_oid(oid, buf, buf_len);
        printf("[HSM] cred_read_sync: slot %u OID 0x%04X → %u bytes\r\n",
               slot, oid, len);
        if (len > 0) {
            if (out_len) *out_len = len;
            ok = true;
        }
        optiga_close();
    } else {
        printf("[HSM] cred_read_sync: optiga_open FAILED (slot %u)\r\n", slot);
    }

    /* Resume CM55 touch polling */
    hsm_touch_send(IPC_CMD_TOUCH_RESUME);

    xSemaphoreGive(s_optiga_mutex);
    return ok;
}

/*******************************************************************************
 * Public API: batch credential read — single OPTIGA session for all slots.
 *
 * Opens OPTIGA ONCE, reads each slot, closes ONCE.
 * Much faster than calling ipc_hsm_cred_read_sync() per-slot (avoids 6×
 * open/close + 6× touch pause/resume which takes ~4 seconds total).
 *
 * IMPORTANT — SCB0 I2C bus sharing with CM55 touch (FT5406):
 *   OPTIGA Trust M and the capacitive touch controller share SCB0 I2C.
 *   We MUST pause CM55 touch polling before ANY OPTIGA I2C access, and
 *   resume it after. Without this, OPTIGA reads fail silently due to
 *   I2C bus contention. If a future board revision uses a separate I2C
 *   bus for OPTIGA, the touch pause/resume can be removed.
 ******************************************************************************/
int ipc_hsm_cred_read_batch(const uint8_t *slots, int num_slots,
                             uint8_t *bufs, uint16_t buf_each,
                             uint16_t *out_lens)
{
    if (!slots || !bufs || !out_lens || num_slots <= 0) return 0;
    if (!s_optiga_mutex) return 0;

    if (xSemaphoreTake(s_optiga_mutex, pdMS_TO_TICKS(10000)) != pdTRUE) {
        printf("[HSM] cred_read_batch: mutex timeout\r\n");
        return 0;
    }

    /* Pause CM55 touch — exclusive SCB0 I2C access for OPTIGA */
    hsm_touch_send(IPC_CMD_TOUCH_PAUSE);
    vTaskDelay(pdMS_TO_TICKS(50));

    int read_ok = 0;

    if (optiga_open()) {
        for (int i = 0; i < num_slots; i++) {
            uint16_t oid = tesaiot_slot_to_oid(slots[i]);
            out_lens[i] = 0;
            if (oid == 0) continue;

            uint8_t *dst = bufs + (i * buf_each);
            uint16_t len = read_oid(oid, dst, buf_each);
            out_lens[i] = len;
            if (len > 0) read_ok++;

            printf("[HSM] batch slot %u OID 0x%04X: %u bytes\r\n",
                   slots[i], oid, len);
        }
        optiga_close();
    } else {
        printf("[HSM] cred_read_batch: optiga_open FAILED\r\n");
    }

    /* Resume CM55 touch polling */
    hsm_touch_send(IPC_CMD_TOUCH_RESUME);
    xSemaphoreGive(s_optiga_mutex);

    printf("[HSM] cred_read_batch: %d/%d slots read OK\r\n", read_ok, num_slots);
    return read_ok;
}

/*******************************************************************************
 * Public init - call from main.c after cm33_ipc_communication_setup()
 ******************************************************************************/
void ipc_hsm_handler_init(void)
{
    s_optiga_mutex = xSemaphoreCreateMutexStatic(&s_optiga_mutex_buf);
    s_hsm_sem = xSemaphoreCreateBinaryStatic(&s_hsm_sem_buf);

    xTaskCreateStatic(
        hsm_task_func,
        "HSM_IPC",
        HSM_TASK_STACK_WORDS,
        NULL,
        HSM_TASK_PRIORITY,
        s_hsm_stack,
        &s_hsm_tcb);

    (void)Cy_IPC_Pipe_RegisterCallback(
        CM33_IPC_PIPE_EP_ADDR,
        hsm_ipc_callback,
        (uint32_t)CM33_IPC_HSM_CLIENT_ID);
}
