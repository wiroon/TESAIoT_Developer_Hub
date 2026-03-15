/*******************************************************************************
 * File Name: usb_hid_joystick.h
 *
 * Description: USB Host HID Joystick driver for Logitech F310 (DirectInput).
 *              Runs on CM55 with FreeRTOS. Reads 8-byte HID reports via
 *              SEGGER emUSB-Host and stores latest state for IPC access.
 *
 * Supported: Logitech F310 (DirectInput mode, VID:046D PID:C216)
 *            Also accepts other Logitech gamepad PIDs.
 *
 *******************************************************************************/

#ifndef USB_HID_JOYSTICK_H
#define USB_HID_JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * F310 DirectInput Report (8 bytes)
 *
 * Byte 0: Left Stick X   (0x00-0xFF, 0x80=center)
 * Byte 1: Left Stick Y   (0x00-0xFF, 0x7F=center)
 * Byte 2: Right Stick X  (0x00-0xFF, 0x80=center)
 * Byte 3: Right Stick Y  (0x00-0xFF, 0x7F=center)
 * Byte 4: hat[0:3] + X[4] A[5] B[6] Y[7]
 * Byte 5: LB[0] RB[1] LT[2] RT[3] Back[4] Start[5] L3[6] R3[7]
 * Byte 6: Mode switch
 * Byte 7: Status
 *******************************************************************************/

/* F310 USB identification */
#define F310_VID                (0x046D)    /* Logitech */
#define F310_PID_DINPUT         (0xC216)    /* F310 DirectInput */
#define F310_PID_XINPUT         (0xC21D)    /* F310 XInput (not used) */

/* F310 report size */
#define F310_REPORT_SIZE        (8)

/* Button masks for byte 4 (face buttons + hat) */
#define F310_HAT_MASK           (0x0F)
#define F310_BTN_X              (1 << 4)
#define F310_BTN_A              (1 << 5)
#define F310_BTN_B              (1 << 6)
#define F310_BTN_Y              (1 << 7)

/* Button masks for byte 5 (shoulders + system) */
#define F310_BTN_LB             (1 << 0)
#define F310_BTN_RB             (1 << 1)
#define F310_BTN_LT             (1 << 2)
#define F310_BTN_RT             (1 << 3)
#define F310_BTN_BACK           (1 << 4)
#define F310_BTN_START          (1 << 5)
#define F310_BTN_L3             (1 << 6)
#define F310_BTN_R3             (1 << 7)

/* Hat switch values */
#define F310_HAT_UP             (0)
#define F310_HAT_UP_RIGHT       (1)
#define F310_HAT_RIGHT          (2)
#define F310_HAT_DOWN_RIGHT     (3)
#define F310_HAT_DOWN           (4)
#define F310_HAT_DOWN_LEFT      (5)
#define F310_HAT_LEFT           (6)
#define F310_HAT_UP_LEFT        (7)
#define F310_HAT_NEUTRAL        (8)

/* Raw 8-byte report structure */
typedef struct __attribute__((packed)) {
    uint8_t left_x;
    uint8_t left_y;
    uint8_t right_x;
    uint8_t right_y;
    uint8_t buttons1;   /* hat[0:3] + X[4] A[5] B[6] Y[7] */
    uint8_t buttons2;   /* LB[0] RB[1] LT[2] RT[3] Back[4] Start[5] L3[6] R3[7] */
    uint8_t mode;
    uint8_t status;
} f310_report_t;

/* USB init stages for debug tracking */
#define USB_STAGE_NONE          0
#define USB_STAGE_USBH_INIT     1   /* USBH_Init() called */
#define USB_STAGE_USBH_DONE     2   /* USBH_Init() returned */
#define USB_STAGE_MAIN_TASK     3   /* USBH_Task created */
#define USB_STAGE_ISR_TASK      4   /* USBH_ISRTask created */
#define USB_STAGE_HID_INIT      5   /* USBH_HID_Init() done */
#define USB_STAGE_CALLBACKS     6   /* Callbacks registered */
#define USB_STAGE_COMPLETE      7   /* Fully initialized */

/* Joystick state (read-only from IPC service) */
typedef struct {
    volatile uint8_t  connected;    /* 0=disconnected, 1=connected */
    volatile uint8_t  sequence;     /* Increments on each new report */
    volatile uint8_t  usb_init_done; /* 1 = USBH_Init completed OK */
    volatile uint8_t  init_stage;   /* USB_STAGE_* debug tracking */
    volatile uint16_t add_event_cnt; /* Device add events from USBH */
    volatile uint16_t remove_event_cnt; /* Device remove events */
    volatile uint16_t report_cnt;   /* HID reports received */
    volatile uint32_t isr_count;    /* USB interrupt count */
    volatile uint32_t port_power_cnt; /* Port power events */
    volatile uint8_t  usbh_running;  /* USBH_IsRunning() result */
    volatile uint8_t  num_devices;   /* USBH_GetNumDevicesConnected() */
    volatile uint8_t  root_conns;    /* USBH_GetNumRootPortConnections() */
    volatile uint8_t  usb_class;     /* USB interface class from enumeration */
    f310_report_t     report;       /* Latest 8-byte report */
    uint16_t          vid;
    uint16_t          pid;
    uint16_t          usb_vid;       /* VID from USB level (not HID) */
    uint16_t          usb_pid;       /* PID from USB level (not HID) */
} joystick_state_t;

/* Initialize USB Host HID joystick (creates USBH tasks) */
void usb_hid_joystick_init(void);

/* Request non-blocking USB Host initialization from a worker task. */
bool usb_hid_joystick_request_init(void);

/* Get pointer to current joystick state (thread-safe, volatile reads) */
const joystick_state_t* usb_hid_joystick_get_state(void);

/* Check if joystick is connected */
bool usb_hid_joystick_is_connected(void);

#endif /* USB_HID_JOYSTICK_H */
