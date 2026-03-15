/*******************************************************************************
 * File Name: ipc_ui_protocol.h
 *
 * Description: IPC protocol definitions for the MicroPython UI module.
 *              Shared between CM33_NS (modui.c) and CM55 (ipc_ui.c).
 *
 *              CM33_NS sends UI commands via IPC Pipe to CM55, which
 *              creates/modifies/deletes LVGL widgets in the UX/UI tab.
 *              Bidirectional for CREATE, POLL_EVENTS, and GET_VALUE.
 *
 *******************************************************************************/

#ifndef IPC_UI_PROTOCOL_H
#define IPC_UI_PROTOCOL_H

#include <stdint.h>

/*******************************************************************************
 * UI IPC Command Codes (0x50-0x61)
 *******************************************************************************/
#define IPC_CMD_UI_CREATE       (0x50)  /* Create widget (bidirectional) */
#define IPC_CMD_UI_DELETE       (0x51)  /* Delete widget by handle */
#define IPC_CMD_UI_SET_TEXT     (0x52)  /* Set text content */
#define IPC_CMD_UI_SET_VALUE    (0x53)  /* Set numeric value */
#define IPC_CMD_UI_SET_POSITION (0x54)  /* Set x,y position */
#define IPC_CMD_UI_SET_SIZE     (0x55)  /* Set width, height */
#define IPC_CMD_UI_SET_COLOR    (0x56)  /* Set primary color */
#define IPC_CMD_UI_SET_VISIBLE  (0x57)  /* Show/hide widget */
#define IPC_CMD_UI_POLL_EVENTS  (0x58)  /* Poll pending events (bidirectional) */
#define IPC_CMD_UI_CLEAR_ALL    (0x59)  /* Delete all widgets */
#define IPC_CMD_UI_SET_DOTMATRIX (0x5A) /* Set dot matrix bitmap data */
#define IPC_CMD_UI_GET_VALUE    (0x5B)  /* Read current value (bidirectional) */
#define IPC_CMD_UI_LIST         (0x5C)  /* List active widgets (bidirectional) */
#define IPC_CMD_UI_SET_IMAGE    (0x5D)  /* Set image pixel data (chunked RGB565) */
#define IPC_CMD_UI_SET_SCREEN   (0x5E)  /* Set screen dimensions + reset layout */
#define IPC_CMD_UI_IDE_STATUS   (0x5F)  /* Show/hide IDE connection status */
#define IPC_CMD_UI_CHART_ADD_SERIES (0x60) /* Add series to chart (bidirectional) */
#define IPC_CMD_UI_CHART_SET_NEXT   (0x61) /* Set next value for chart series */
#define IPC_CMD_UI_DEPLOY_SCREEN   (0x62) /* Show native deploy overlay (fire-and-forget) */

#define IPC_CMD_UI_FIRST        (0x50)
#define IPC_CMD_UI_LAST         (0x63)

/*******************************************************************************
 * Widget Type Enum
 *******************************************************************************/
typedef enum {
    UI_WIDGET_BUTTON    = 1,
    UI_WIDGET_LABEL     = 2,
    UI_WIDGET_SLIDER    = 3,
    UI_WIDGET_SWITCH    = 4,
    UI_WIDGET_CHECKBOX  = 5,
    UI_WIDGET_ARC       = 6,
    UI_WIDGET_BAR       = 7,   /* Progress bar */
    UI_WIDGET_SPINNER   = 8,
    UI_WIDGET_DROPDOWN  = 9,
    UI_WIDGET_TEXTAREA  = 10,
    UI_WIDGET_SEG7      = 11,  /* 7-segment display (styled label) */
    UI_WIDGET_DOTMATRIX = 12,  /* Dot matrix (lv_canvas) */
    UI_WIDGET_CHART     = 13,
    UI_WIDGET_IMAGE     = 14,  /* Image (lv_canvas: built-in icon or dynamic RGB565) */
    UI_WIDGET_PANEL     = 15,  /* Dark rect container (dashboard card) */
    UI_WIDGET_COMPASS   = 16,  /* Heading compass (circle + needle + NESW) */
} ui_widget_type_t;

/*******************************************************************************
 * Event Type Enum
 *******************************************************************************/
#define UI_EVENT_CLICKED        (1)
#define UI_EVENT_VALUE_CHANGED  (2)
#define UI_EVENT_TOGGLED        (3)

/*******************************************************************************
 * Limits
 *******************************************************************************/
#define UI_MAX_WIDGETS          (32)
#define UI_EVENT_RING_SIZE      (16)
#define UI_MAX_EVENTS_PER_POLL  (8)
#define UI_CREATE_TEXT_MAX      (96)
#define UI_CHART_MAX_SERIES     (4)

/*******************************************************************************
 * IPC Payload: UI_CREATE (0x50)
 * Packed into ipc_msg_t.data[] (max 128 bytes)
 * Total: 121 bytes
 *******************************************************************************/
typedef struct __attribute__((packed)) {
    uint8_t  widget_type;       /* ui_widget_type_t (1-16) */
    int16_t  x;                 /* X position (-1 = auto) */
    int16_t  y;                 /* Y position (-1 = auto) */
    int16_t  w;                 /* Width (-1 = auto) */
    int16_t  h;                 /* Height (-1 = auto) */
    uint32_t color;             /* 0xRRGGBB (0 = theme default) */
    int32_t  min_val;           /* Range minimum / cols (dotmatrix) */
    int32_t  max_val;           /* Range maximum / rows (dotmatrix) */
    int32_t  init_val;          /* Initial value */
    char     text[UI_CREATE_TEXT_MAX]; /* Label text (null-terminated) */
} ipc_ui_create_t;

/*******************************************************************************
 * IPC Response: UI_POLL_EVENTS (0x58)
 * Packed into ipc_response_t.data[] (max 240 bytes)
 * Up to UI_MAX_EVENTS_PER_POLL events (8 * 8 = 64 bytes)
 *******************************************************************************/
typedef struct __attribute__((packed)) {
    uint8_t  handle_id;         /* Widget that generated event */
    uint8_t  event_type;        /* UI_EVENT_* */
    int32_t  value;             /* Event value */
    uint16_t reserved;
} ipc_ui_event_t;

/*******************************************************************************
 * IPC Response: UI_LIST (0x5C)
 * Packed into ipc_response_t.data[] (max 240 bytes)
 * data[0] = count, then count × ui_widget_info_t entries (2 bytes each)
 * Max 32 widgets × 2 + 1 = 65 bytes
 *******************************************************************************/
typedef struct __attribute__((packed)) {
    uint8_t  handle_id;         /* Widget handle (0-31) */
    uint8_t  widget_type;       /* ui_widget_type_t */
} ipc_ui_widget_info_t;

/*******************************************************************************
 * IPC Response Status Codes
 *******************************************************************************/
#define UI_STATUS_OK            (0)
#define UI_STATUS_TABLE_FULL    (1)
#define UI_STATUS_INVALID_TYPE  (2)
#define UI_STATUS_INVALID_HANDLE (3)
#define UI_STATUS_ERROR         (4)
#define UI_STATUS_ALLOC_FAILED  (5)

#endif /* IPC_UI_PROTOCOL_H */
