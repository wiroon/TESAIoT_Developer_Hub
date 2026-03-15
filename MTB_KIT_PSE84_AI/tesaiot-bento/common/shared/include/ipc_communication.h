/*******************************************************************************
 * File Name        : ipc_communication.h
 *
 * Description      : Headers and structures for IPC Pipes between CM33 and CM55.
 *                    Stripped-down version for lcd.print() IPC.
 *                    Based on TESA sensorhub ipc_communication.h
 *
 * Author           : Asst.Prof.Santi Nuratch, Ph.D
 *                    Thailand Embedded Systems Association (TESA)
 *
 *******************************************************************************/

#ifndef SOURCE_IPC_COMMUNICATION_H
#define SOURCE_IPC_COMMUNICATION_H

/*******************************************************************************
 * Header Files
 *******************************************************************************/
#include "cy_ipc_pipe.h"
#include "cy_pdl.h"
#include "cybsp.h"
#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * Macros
 *******************************************************************************/
#define CY_IPC_MAX_ENDPOINTS (5UL)
#define CY_IPC_CYPIPE_CLIENT_CNT (10UL)

#define CY_IPC_CHAN_CYPIPE_EP1 (4UL)
#define CY_IPC_INTR_CYPIPE_EP1 (4UL)
#define CY_IPC_CHAN_CYPIPE_EP2 (15UL)
#define CY_IPC_INTR_CYPIPE_EP2 (5UL)

/* IPC Pipe Endpoint-1 config (CM33) */
#define CY_IPC_CYPIPE_CHAN_MASK_EP1 CY_IPC_CH_MASK(CY_IPC_CHAN_CYPIPE_EP1)
#define CY_IPC_CYPIPE_INTR_MASK_EP1 CY_IPC_INTR_MASK(CY_IPC_INTR_CYPIPE_EP1)
#define CY_IPC_INTR_CYPIPE_PRIOR_EP1 (1UL)
#define CY_IPC_INTR_CYPIPE_MUX_EP1 (CY_IPC0_INTR_MUX(CY_IPC_INTR_CYPIPE_EP1))
#define CM33_IPC_PIPE_EP_ADDR (1UL)
#define CM33_IPC_PIPE_CLIENT_ID (3UL)

/* IPC Pipe Endpoint-2 config (CM55) */
#define CY_IPC_CYPIPE_CHAN_MASK_EP2 CY_IPC_CH_MASK(CY_IPC_CHAN_CYPIPE_EP2)
#define CY_IPC_CYPIPE_INTR_MASK_EP2 CY_IPC_INTR_MASK(CY_IPC_INTR_CYPIPE_EP2)
#define CY_IPC_INTR_CYPIPE_PRIOR_EP2 (1UL)
#define CY_IPC_INTR_CYPIPE_MUX_EP2 (CY_IPC0_INTR_MUX(CY_IPC_INTR_CYPIPE_EP2))
#define CM55_IPC_PIPE_EP_ADDR (2UL)
#define CM55_IPC_PIPE_CLIENT_ID (5UL)

/* Sensor data IPC client (separate from LCD client for independent callbacks) */
#define CM55_IPC_SENSOR_CLIENT_ID (6UL)
/* Sensor auto-control IPC client on CM33 (CM55 -> CM33) */
#define CM33_IPC_SENSOR_CTRL_CLIENT_ID (9UL)

/* Combined Interrupt Mask */
#define CY_IPC_CYPIPE_INTR_MASK (CY_IPC_CYPIPE_CHAN_MASK_EP1 | CY_IPC_CYPIPE_CHAN_MASK_EP2)

/* LCD terminal commands (CM33 → CM55) */
#define IPC_CMD_LCD_PRINT       (0xE0)
#define IPC_CMD_LCD_CLEAR       (0xE1)
#define IPC_CMD_LCD_THEME       (0xE2)

/* Sensor data commands (CM33 → CM55) */
#define IPC_CMD_SENSOR_BMI270   (0x91)
#define IPC_CMD_SENSOR_DPS368   (0x92)
#define IPC_CMD_SENSOR_SHT40    (0x93)
#define IPC_CMD_SENSOR_BMM350   (0x94)
#define IPC_CMD_SENSOR_CAPSENSE (0x95)
#define IPC_CMD_SENSOR_POT      (0x96)
/* Sensor auto-control command (CM55 -> CM33), data[0]: 0=stop, 1=start */
#define IPC_CMD_SENSOR_AUTO_CTRL (0x97)
/* System recovery command (CM55 -> CM33): request one-shot safe boot */
#define IPC_CMD_SYSTEM_SAFE_REBOOT (0x98)
/* Delete /main.py command (CM55 -> CM33): remove autorun script + reboot */
#define IPC_CMD_DELETE_MAIN_PY     (0x99)

/* WiFi IPC commands (CM33 → CM55, future) */
#define IPC_CMD_WIFI_SCAN       (0xD0)
#define IPC_CMD_WIFI_CONNECT    (0xD1)
#define IPC_CMD_WIFI_DISCONNECT (0xD2)
#define IPC_CMD_WIFI_STATUS     (0xD3)
#define IPC_CMD_WIFI_IP         (0xD4)
#define IPC_CMD_WIFI_SOFTAP     (0xD5)

/* Touch I2C bus control (CM33 → CM55, for OPTIGA/CAPSENSE bus sharing) */
#define IPC_CMD_TOUCH_PAUSE     (0xD6)  /* Pause touch I2C polling */
#define IPC_CMD_TOUCH_RESUME    (0xD7)  /* Resume touch + reinit controller */
/* WiFi state push (CM33 → CM55, non-blocking notification) */
#define IPC_CMD_WIFI_STATE_PUSH (0xD8)  /* data[0]: 0=disconnected, 1=connected */
/* Formatted time push (CM33 → CM55, after NTP sync) */
#define IPC_CMD_TIME_PUSH       (0xD9)  /* data[]: null-terminated time string */

/* MQTT IPC commands (CM33 → CM55) */
#define IPC_CMD_MQTT_CONNECT    (0xF0)
#define IPC_CMD_MQTT_DISCONNECT (0xF1)
#define IPC_CMD_MQTT_PUBLISH    (0xF2)
#define IPC_CMD_MQTT_SUBSCRIBE  (0xF3)
#define IPC_CMD_MQTT_POLL       (0xF6)

/* UI IPC commands (CM33 → CM55, MicroPython ui module → LVGL widgets) */
#include "ipc_ui_protocol.h"

/* GPIO IPC command (CM33 → CM55, LED state bitmask for LVGL UI) */
#define IPC_CMD_GPIO_LED_STATE  (0x80)

/* LED toggle command (CM55 → CM33, data[0] = LED index 0-2) */
#define IPC_CMD_LED_TOGGLE      (0x81)

/* CM33 callback client ID for receiving LED toggle from CM55 */
#define CM33_IPC_LED_CLIENT_ID  (4UL)

/* Joystick IPC commands (CM33 → CM55 request, CM55 → CM33 response) */
#define IPC_CMD_JOYSTICK_STATE  (0xC0)  /* Read joystick state */
#define IPC_CMD_JOYSTICK_INIT   (0xC1)  /* Initialize USB Host HID */

/* Radar IPC command (CM33 → CM55 request, CM55 → CM33 response) */
#define IPC_CMD_RADAR_STATUS    (0xB0)

/* HSM page IPC commands (CM55 → CM33): OPTIGA Trust M operations */
#define IPC_CMD_HSM_REQUEST     (0xB5)  /* Read chip data (UID, LCS, certs, counters) */
#define IPC_CMD_HSM_BENCHMARK   (0xB6)  /* Run crypto benchmarks (ECC, SHA, RNG) */
#define IPC_CMD_HSM_READ_CERT   (0xB7)  /* Read + parse cert DER (slot in resp->cmd) */
#define IPC_CMD_HSM_PIN_CHECK   (0xB8)  /* Check if PIN exists in DATA_3 */
#define IPC_CMD_HSM_PIN_SET     (0xB9)  /* Write SHA-256(digits) to DATA_3 */
#define IPC_CMD_HSM_PIN_VERIFY  (0xBA)  /* Verify PIN against stored hash */
#define IPC_CMD_HSM_HEALTH      (0xBB)  /* Run 8 self-tests */
#define IPC_CMD_HSM_PIN_RESET   (0xBC)  /* Erase PIN from DATA_3 (requires old PIN verify) */
/* CM33 callback client ID for receiving HSM requests from CM55 */
#define CM33_IPC_HSM_CLIENT_ID  (2UL)

/* TESAIoT credential IPC commands (CM33 → CM55) */
#define IPC_CMD_TESAIOT_INIT       (0xA0)
#define IPC_CMD_TESAIOT_DEVICE_ID  (0xA1)
#define IPC_CMD_TESAIOT_LICENSE    (0xA2)
#define IPC_CMD_TESAIOT_CRED_READ  (0xA3)
#define IPC_CMD_TESAIOT_CRED_WRITE (0xA4)
#define IPC_CMD_TESAIOT_RANDOM     (0xA5)
#define IPC_CMD_TESAIOT_CRED_ERASE (0xA6)
#define IPC_CMD_TESAIOT_HASH       (0xA7)
#define IPC_CMD_TESAIOT_HMAC       (0xA8)
#define IPC_CMD_TESAIOT_AES_KEYGEN (0xA9)
#define IPC_CMD_TESAIOT_AES_ENC    (0xAA)
#define IPC_CMD_TESAIOT_AES_DEC    (0xAB)
#define IPC_CMD_TESAIOT_SIGN       (0xAC)
#define IPC_CMD_TESAIOT_COUNTER_RD (0xAD)
#define IPC_CMD_TESAIOT_COUNTER_INC (0xAE)
#define IPC_CMD_TESAIOT_HEALTH     (0xAF)
#define IPC_CMD_TESAIOT_LAST       (0xAF)

/* IPC Service client ID (WiFi + MQTT + TESAIoT bidirectional) */
#define CM55_IPC_SERVICE_CLIENT_ID (7UL)

/* IPC UI client ID (MicroPython ui module → LVGL widgets) */
#define CM55_IPC_UI_CLIENT_ID      (8UL)

/* Gravity constant for acceleration conversion */
#define GRAVITY_ACCEL           (9.80665f)

#define IPC_DATA_MAX_LEN (128UL)

/* Response data max length */
#define IPC_RESPONSE_DATA_MAX (240UL)

/*******************************************************************************
 * IPC Message Structure
 *******************************************************************************/
typedef struct
{
  uint16_t client_id;          /* Bits 0-7: Client ID */
  uint16_t intr_mask;          /* Bits 16-31: Release Mask (MANDATORY for Pipe Driver) */
  uint32_t cmd;                /* Command code */
  uint32_t value;              /* Command argument or flags */
  char data[IPC_DATA_MAX_LEN]; /* Payload buffer */
} ipc_msg_t;

/*******************************************************************************
 * Sensor Data Structures (packed into ipc_msg_t.data[])
 *******************************************************************************/

/* BMI270 IMU: accel + gyro raw data */
typedef struct __attribute__((packed)) {
    int16_t ax, ay, az;     /* Accel raw (divide by 16384 for g) */
    int16_t gx, gy, gz;     /* Gyro raw (divide by 16.4 for dps) */
    uint16_t sequence;
} ipc_sensor_bmi270_t;

/* DPS368: pressure + temperature */
typedef struct __attribute__((packed)) {
    int32_t pressure_x100;      /* hPa * 100 */
    int16_t temperature_x100;   /* Celsius * 100 */
    uint16_t sequence;
} ipc_sensor_dps368_t;

/* SHT40: humidity + temperature */
typedef struct __attribute__((packed)) {
    int16_t temperature_x100;   /* Celsius * 100 */
    uint16_t humidity_x100;     /* %RH * 100 */
    uint16_t sequence;
} ipc_sensor_sht40_t;

/* BMM350: magnetometer X/Y/Z in micro-Tesla * 100 */
typedef struct __attribute__((packed)) {
    int16_t mx_x100;           /* X uT * 100 */
    int16_t my_x100;           /* Y uT * 100 */
    int16_t mz_x100;           /* Z uT * 100 */
    uint16_t heading_x10;      /* Compass heading * 10 (0-3600) */
    uint16_t sequence;
} ipc_sensor_bmm350_t;

/* CapSense: touch buttons + slider */
typedef struct __attribute__((packed)) {
    uint8_t btn0_pressed;      /* 0 or 1 */
    uint8_t btn1_pressed;      /* 0 or 1 */
    uint8_t slider;            /* 0-100 (%) */
    uint8_t reserved;
    uint16_t sequence;
} ipc_sensor_capsense_t;

/* Potentiometer: ADC value */
typedef struct __attribute__((packed)) {
    uint16_t raw;              /* 0-65535 (16-bit scaled) */
    uint16_t percent_x10;     /* 0-1000 (0.0-100.0%) */
    uint16_t sequence;
} ipc_sensor_pot_t;

/* Joystick state (packed into ipc_response_t.data[]) */
typedef struct __attribute__((packed)) {
    uint8_t connected;         /* 0=disconnected, 1=connected */
    uint8_t sequence;          /* Increments on each new report */
    uint8_t left_x, left_y;   /* 0x00-0xFF, center ~0x80/0x7F */
    uint8_t right_x, right_y;
    uint8_t buttons1;          /* hat[0:3] + X[4] A[5] B[6] Y[7] */
    uint8_t buttons2;          /* LB[0] RB[1] LT[2] RT[3] Back[4] Start[5] L3[6] R3[7] */
    uint16_t vid, pid;         /* USB Vendor/Product ID */
    /* Debug fields */
    uint8_t usb_init_done;     /* 1 = USBH_Init completed OK */
    uint8_t init_stage;        /* USB_STAGE_* (0-7) */
    uint16_t add_event_cnt;    /* Device add events from USBH */
    uint16_t remove_event_cnt; /* Device remove events */
    uint16_t report_cnt;       /* HID reports received */
    uint32_t isr_count;        /* USB interrupt count */
    uint32_t port_power_cnt;   /* Port power events */
    uint8_t  usbh_running;     /* USBH_IsRunning() */
    uint8_t  num_devices;      /* USBH_GetNumDevicesConnected() */
    uint8_t  root_conns;       /* USBH_GetNumRootPortConnections() */
    uint8_t  usb_class;        /* USB interface class (0x03=HID, 0xFF=vendor) */
    uint16_t usb_vid;          /* VID from USB level */
    uint16_t usb_pid;          /* PID from USB level */
} ipc_joystick_state_t;

/* Radar status (packed into ipc_response_t.data[]) */
typedef struct __attribute__((packed)) {
    uint8_t initialized;        /* 1 if radar hw init succeeded */
    uint8_t presence;           /* 1 if person detected */
    uint8_t reserved[2];
    float   energy;             /* Smoothed signal energy delta */
} ipc_radar_status_t;

/*******************************************************************************
 * IPC Response Structure (bidirectional: CM55 → CM33_NS via shared memory)
 *
 * Pattern:
 *   CM33_NS: sets ready=0, sends IPC command with response ptr in msg.value
 *   CM55:    processes command, fills response, sets ready=1
 *   CM33_NS: busy-waits on ready, reads data
 *******************************************************************************/
typedef struct __attribute__((packed)) {
    volatile uint8_t ready;     /* 0=pending, 1=ready (written by CM55) */
    uint8_t cmd;                /* Echo of request command */
    uint8_t status;             /* 0=success, nonzero=error code */
    uint8_t reserved;
    uint16_t data_len;          /* Bytes of valid data in data[] */
    uint16_t reserved2;
    uint8_t data[IPC_RESPONSE_DATA_MAX]; /* Response payload */
} ipc_response_t;

/*******************************************************************************
 * WiFi Scan Result Entry (fits in ipc_response_t.data[])
 * Max entries = IPC_RESPONSE_DATA_MAX / sizeof(ipc_wifi_scan_entry_t) = 6
 *******************************************************************************/
#define IPC_WIFI_SCAN_MAX_ENTRIES (6)

typedef struct __attribute__((packed)) {
    char ssid[33];              /* SSID (null-terminated, max 32 chars) */
    int8_t rssi;                /* Signal strength dBm */
    uint8_t security;           /* 0=open, 1=WEP, 2=WPA, 3=WPA2, 4=WPA3 */
    uint8_t channel;            /* WiFi channel */
} ipc_wifi_scan_entry_t;        /* 36 bytes per entry */

/*******************************************************************************
 * TESAIoT Credential Slot Mapping
 *
 * Slot → OID via tesaiot_slot_to_oid() lookup (NOT simple addition).
 * Slot 4 (OID 0xF1D4) is RESERVED for Protected Update confidentiality key
 * (AES-CCM shared secret used by tesaiot CSR/Protected Update workflow).
 *
 * Type 3 Data Objects (0xF1D0-0xF1DB): max 140 bytes each
 * Type 2 Data Objects (0xF1E0-0xF1E1): max 1500 bytes each (seldom changed)
 *******************************************************************************/
#define TESAIOT_SLOT_DEVICE_ID   (0)   /* OID 0xF1D0 */
#define TESAIOT_SLOT_LICENSE_KEY (1)   /* OID 0xF1D1 */
#define TESAIOT_SLOT_MQTT_USER   (2)   /* OID 0xF1D2 */
#define TESAIOT_SLOT_MQTT_PASS   (3)   /* OID 0xF1D3 */
/* Slot 4 = RESERVED (OID 0xF1D4 — Protected Update confidentiality key) */
#define TESAIOT_SLOT_WIFI_SSID   (5)   /* OID 0xF1D5 */
#define TESAIOT_SLOT_WIFI_PASS   (6)   /* OID 0xF1D6 */
#define TESAIOT_SLOT_API_KEY     (7)   /* OID 0xF1D7 */
#define TESAIOT_SLOT_USER0       (8)   /* OID 0xF1D8 */
#define TESAIOT_SLOT_USER1       (9)   /* OID 0xF1D9 */
#define TESAIOT_SLOT_USER2       (10)  /* OID 0xF1DA */
#define TESAIOT_SLOT_USER3       (11)  /* OID 0xF1DB */
#define TESAIOT_SLOT_LARGE0      (12)  /* OID 0xF1E0 — 1500 bytes max */
#define TESAIOT_SLOT_LARGE1      (13)  /* OID 0xF1E1 — 1500 bytes max */
#define TESAIOT_SLOT_COUNT       (14)
#define TESAIOT_SLOT_RESERVED    (4)   /* Protected Update — do NOT use */
#define TESAIOT_SLOT_MAX_SIZE    (140)  /* Type 3 data objects (slots 0-11) */
#define TESAIOT_SLOT_LARGE_MAX   (1500) /* Type 2 data objects (slots 12-13) */

/* Slot → OID lookup (handles gap at slot 4 and large data slots 12-13) */
static inline uint16_t tesaiot_slot_to_oid(uint8_t slot) {
    if (slot <= 3)  return 0xF1D0 + slot;       /* 0xF1D0-0xF1D3 */
    if (slot >= 5 && slot <= 11) return 0xF1D0 + slot; /* 0xF1D5-0xF1DB */
    if (slot == 12) return 0xF1E0;               /* Large data 0 */
    if (slot == 13) return 0xF1E1;               /* Large data 1 */
    return 0;  /* Invalid (slot 4 reserved, or out of range) */
}

/* Max data size for a given slot */
static inline uint16_t tesaiot_slot_max_size(uint8_t slot) {
    if (slot == 12 || slot == 13) return TESAIOT_SLOT_LARGE_MAX;
    return TESAIOT_SLOT_MAX_SIZE;
}

/*******************************************************************************
 * Function prototypes
 *******************************************************************************/

/** CM33 */
void cm33_ipc_communication_setup(void);
void cm33_ipc_communication_recover(void);
void cm33_ipc_pipe_isr(void);

/** CM55 */
void cm55_ipc_communication_setup(void);
void cm55_ipc_pipe_isr(void);

#endif /* SOURCE_IPC_COMMUNICATION_H */
