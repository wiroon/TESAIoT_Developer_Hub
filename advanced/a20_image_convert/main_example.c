/**
 * @file    main_example.c
 * @brief   Camera Frame Capture — JPEG encode + LVGL image preview
 *
 * @description
 *   Captures camera frame via IPC, encodes to JPEG, displays in LVGL image.
 *   Shows frame metadata: resolution, sizes, compression ratio, encode time.
 *
 * @board    AI Kit (KIT_PSE84_AI) only — camera sensor
 * @author   TESAIoT
 */

#include "example_common.h"
#include "ipc_frame_share.h"
#include "jpegenc_wrapper.h"

/* ── Layout ─────────────────────────────────────────────────────── */
#define PREVIEW_W       320
#define PREVIEW_H       240
#define JPEG_BUF_SIZE   (64 * 1024)   /* 64 KB JPEG buffer */

/* ── State ──────────────────────────────────────────────────────── */
static lv_obj_t *s_img;
static lv_obj_t *s_lbl_status;
static lv_obj_t *s_lbl_res;
static lv_obj_t *s_lbl_raw_size;
static lv_obj_t *s_lbl_jpeg_size;
static lv_obj_t *s_lbl_ratio;
static lv_obj_t *s_lbl_time;
static uint8_t   s_jpeg_buf[JPEG_BUF_SIZE];
static lv_image_dsc_t s_img_dsc;
static uint32_t  s_capture_count;

/* ── Capture button callback ────────────────────────────────────── */
static void capture_cb(lv_event_t *e)
{
    (void)e;

    lv_label_set_text(s_lbl_status, LV_SYMBOL_REFRESH " Capturing...");
    lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_WARNING, 0);

    /* Request frame from CM55 camera task */
    ipc_frame_t frame;
    cy_rslt_t res = ipc_frame_request(&frame);
    if (res != CY_RSLT_SUCCESS) {
        lv_label_set_text(s_lbl_status, LV_SYMBOL_CLOSE " Frame request failed");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_ERROR, 0);
        return;
    }

    uint32_t raw_size = frame.width * frame.height * 2;  /* RGB565 */
    lv_label_set_text_fmt(s_lbl_res, "Resolution: %ux%u", frame.width, frame.height);
    lv_label_set_text_fmt(s_lbl_raw_size, "Raw: %lu KB", (unsigned long)(raw_size / 1024));

    /* JPEG encode */
    uint32_t t_start = xTaskGetTickCount();
    uint32_t jpeg_size = 0;
    int enc_res = jpegenc_encode_rgb565(frame.data, frame.width, frame.height,
                                         s_jpeg_buf, JPEG_BUF_SIZE, &jpeg_size, 80);
    uint32_t t_elapsed = (xTaskGetTickCount() - t_start) * portTICK_PERIOD_MS;

    /* Release frame back to camera task */
    ipc_frame_release(&frame);

    if (enc_res != 0 || jpeg_size == 0) {
        lv_label_set_text(s_lbl_status, LV_SYMBOL_CLOSE " JPEG encode failed");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_ERROR, 0);
        return;
    }

    lv_label_set_text_fmt(s_lbl_jpeg_size, "JPEG: %lu KB", (unsigned long)(jpeg_size / 1024));
    lv_label_set_text_fmt(s_lbl_ratio, "Ratio: %.1fx",
                          (float)raw_size / (float)jpeg_size);
    lv_label_set_text_fmt(s_lbl_time, "Encode: %lu ms", (unsigned long)t_elapsed);

    /* Update LVGL image descriptor */
    s_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_img_dsc.header.cf    = LV_COLOR_FORMAT_RGB565;
    s_img_dsc.header.w     = frame.width;
    s_img_dsc.header.h     = frame.height;
    s_img_dsc.data_size    = raw_size;
    s_img_dsc.data         = frame.data;   /* Display raw for preview */

    lv_image_set_src(s_img, &s_img_dsc);

    s_capture_count++;
    lv_label_set_text_fmt(s_lbl_status, LV_SYMBOL_OK " Capture #%lu OK",
                          (unsigned long)s_capture_count);
    lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_SUCCESS, 0);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_capture_count = 0;
    memset(&s_img_dsc, 0, sizeof(s_img_dsc));

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "A20 \xe2\x80\x94 Camera Capture");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    /* Status */
    s_lbl_status = lv_label_create(parent);
    lv_label_set_text(s_lbl_status, LV_SYMBOL_IMAGE " Ready");
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(s_lbl_status, LV_ALIGN_TOP_MID, 0, 28);

    /* === Preview area === */
    lv_obj_t *preview_card = example_card_create(parent, PREVIEW_W + 16,
                                                  PREVIEW_H + 16,
                                                  lv_color_hex(0x0A1628));
    lv_obj_align(preview_card, LV_ALIGN_CENTER, 0, -30);

    s_img = lv_image_create(preview_card);
    lv_obj_set_size(s_img, PREVIEW_W, PREVIEW_H);
    lv_obj_center(s_img);

    /* Placeholder text over preview */
    lv_obj_t *ph = lv_label_create(preview_card);
    lv_label_set_text(ph, LV_SYMBOL_IMAGE " Tap Capture");
    lv_obj_set_style_text_color(ph, UI_COLOR_TEXT_DIM, 0);
    lv_obj_center(ph);

    /* === Capture button === */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 140, 44);
    lv_obj_align(btn, LV_ALIGN_CENTER, -120, 120);
    lv_obj_set_style_bg_color(btn, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_add_event_cb(btn, capture_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_IMAGE " Capture");
    lv_obj_set_style_text_color(btn_lbl, lv_color_white(), 0);
    lv_obj_center(btn_lbl);

    /* === Metadata panel === */
    lv_obj_t *info_card = example_card_create(parent, 200, 140, UI_COLOR_CARD_BG);
    lv_obj_align(info_card, LV_ALIGN_CENTER, 120, 100);

    s_lbl_res = example_label_create(info_card, "Resolution: --",
                                     &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(s_lbl_res, LV_ALIGN_TOP_LEFT, 0, 0);

    s_lbl_raw_size = example_label_create(info_card, "Raw: -- KB",
                                          &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(s_lbl_raw_size, LV_ALIGN_TOP_LEFT, 0, 20);

    s_lbl_jpeg_size = example_label_create(info_card, "JPEG: -- KB",
                                           &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(s_lbl_jpeg_size, LV_ALIGN_TOP_LEFT, 0, 40);

    s_lbl_ratio = example_label_create(info_card, "Ratio: --",
                                       &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(s_lbl_ratio, LV_ALIGN_TOP_LEFT, 0, 60);

    s_lbl_time = example_label_create(info_card, "Encode: -- ms",
                                      &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(s_lbl_time, LV_ALIGN_TOP_LEFT, 0, 80);
}
