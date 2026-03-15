/**
 * @file    main_example.c
 * @brief   Face Database — Enrollment + recognition UI with gallery
 *
 * @description
 *   Face ID enrollment and recognition: gallery grid of enrolled faces,
 *   match result with confidence score, enroll/recognize/delete controls.
 *
 * @board    AI Kit (KIT_PSE84_AI) only — camera + AI inference
 * @author   TESAIoT
 */

#include "example_common.h"
#include "face_detection.h"
#include "ipc_frame_share.h"

/* ── Layout ─────────────────────────────────────────────────────── */
#define PREVIEW_W       240
#define PREVIEW_H       180
#define THUMB_SIZE       56
#define MAX_FACES        10
#define GALLERY_W       380
#define GALLERY_H        72
#define CONFIDENCE_THRESH 70

/* ── Face entry ─────────────────────────────────────────────────── */
typedef struct {
    char     name[16];
    bool     valid;
    uint8_t  thumbnail[THUMB_SIZE * THUMB_SIZE * 2];  /* RGB565 */
} face_entry_t;

/* ── State ──────────────────────────────────────────────────────── */
static face_entry_t s_faces[MAX_FACES];
static int          s_face_count = 0;
static lv_obj_t    *s_img_preview;
static lv_obj_t    *s_gallery_cont;
static lv_obj_t    *s_lbl_status;
static lv_obj_t    *s_lbl_result;
static lv_obj_t    *s_lbl_confidence;
static lv_obj_t    *s_result_dot;
static lv_image_dsc_t s_preview_dsc;

/* ── Gallery: refresh thumbnails ────────────────────────────────── */
static void refresh_gallery(void)
{
    /* Remove existing children */
    uint32_t cnt = lv_obj_get_child_count(s_gallery_cont);
    for (int i = (int)cnt - 1; i >= 0; i--) {
        lv_obj_delete(lv_obj_get_child(s_gallery_cont, i));
    }

    /* Add thumbnails */
    for (int i = 0; i < MAX_FACES; i++) {
        if (!s_faces[i].valid) continue;

        lv_obj_t *frame = lv_obj_create(s_gallery_cont);
        lv_obj_set_size(frame, THUMB_SIZE + 8, THUMB_SIZE + 20);
        lv_obj_set_style_bg_color(frame, UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(frame, 6, 0);
        lv_obj_set_style_border_width(frame, 1, 0);
        lv_obj_set_style_border_color(frame, lv_color_hex(0x2A3A5C), 0);
        lv_obj_set_style_pad_all(frame, 2, 0);
        lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

        /* Placeholder colored box for thumbnail */
        lv_obj_t *thumb = lv_obj_create(frame);
        lv_obj_set_size(thumb, THUMB_SIZE, THUMB_SIZE);
        lv_obj_set_style_bg_color(thumb, lv_color_hex(0x1a3050), 0);
        lv_obj_set_style_bg_opa(thumb, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(thumb, 4, 0);
        lv_obj_set_style_border_width(thumb, 0, 0);
        lv_obj_align(thumb, LV_ALIGN_TOP_MID, 0, 0);

        /* Face icon */
        lv_obj_t *icon = lv_label_create(thumb);
        lv_label_set_text(icon, LV_SYMBOL_EYE_OPEN);
        lv_obj_set_style_text_color(icon, UI_COLOR_PRIMARY, 0);
        lv_obj_center(icon);

        /* Name label */
        lv_obj_t *name_lbl = lv_label_create(frame);
        lv_label_set_text(name_lbl, s_faces[i].name);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name_lbl, UI_COLOR_TEXT, 0);
        lv_obj_align(name_lbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
}

/* ── Enroll button callback ─────────────────────────────────────── */
static void enroll_cb(lv_event_t *e)
{
    (void)e;

    if (s_face_count >= MAX_FACES) {
        lv_label_set_text(s_lbl_status, "Database full (max 10)");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_ERROR, 0);
        return;
    }

    lv_label_set_text(s_lbl_status, LV_SYMBOL_REFRESH " Enrolling...");
    lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_WARNING, 0);

    /* Capture frame */
    ipc_frame_t frame;
    if (ipc_frame_request(&frame) != CY_RSLT_SUCCESS) {
        lv_label_set_text(s_lbl_status, LV_SYMBOL_CLOSE " No frame");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_ERROR, 0);
        return;
    }

    /* Detect face */
    face_result_t det;
    if (face_detect_run(frame.data, frame.width, frame.height, &det) != 0 ||
        det.num_faces == 0) {
        ipc_frame_release(&frame);
        lv_label_set_text(s_lbl_status, LV_SYMBOL_CLOSE " No face detected");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_ERROR, 0);
        return;
    }

    /* Enroll the first detected face */
    int slot = -1;
    for (int i = 0; i < MAX_FACES; i++) {
        if (!s_faces[i].valid) { slot = i; break; }
    }

    if (slot >= 0) {
        face_enroll(det.embedding, slot);
        snprintf(s_faces[slot].name, sizeof(s_faces[slot].name),
                 "Face_%d", s_face_count + 1);
        s_faces[slot].valid = true;
        s_face_count++;

        lv_label_set_text_fmt(s_lbl_status, LV_SYMBOL_OK " Enrolled: %s",
                              s_faces[slot].name);
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_SUCCESS, 0);
        refresh_gallery();
    }

    ipc_frame_release(&frame);
}

/* ── Recognize button callback ──────────────────────────────────── */
static void recognize_cb(lv_event_t *e)
{
    (void)e;

    lv_label_set_text(s_lbl_status, LV_SYMBOL_REFRESH " Recognizing...");
    lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_WARNING, 0);

    /* Capture frame */
    ipc_frame_t frame;
    if (ipc_frame_request(&frame) != CY_RSLT_SUCCESS) {
        lv_label_set_text(s_lbl_status, LV_SYMBOL_CLOSE " No frame");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_ERROR, 0);
        return;
    }

    /* Detect + recognize */
    face_result_t det;
    if (face_detect_run(frame.data, frame.width, frame.height, &det) != 0 ||
        det.num_faces == 0) {
        ipc_frame_release(&frame);
        lv_label_set_text(s_lbl_status, LV_SYMBOL_CLOSE " No face detected");
        lv_label_set_text(s_lbl_result, "---");
        lv_label_set_text(s_lbl_confidence, "0%");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_ERROR, 0);
        return;
    }

    int match_id = -1;
    float confidence = 0.0f;
    face_recognize(det.embedding, &match_id, &confidence);

    ipc_frame_release(&frame);

    int conf_pct = (int)(confidence * 100.0f);
    lv_label_set_text_fmt(s_lbl_confidence, "%d%%", conf_pct);

    if (match_id >= 0 && match_id < MAX_FACES && s_faces[match_id].valid &&
        conf_pct >= CONFIDENCE_THRESH) {
        lv_label_set_text_fmt(s_lbl_result, LV_SYMBOL_OK " %s",
                              s_faces[match_id].name);
        lv_obj_set_style_text_color(s_lbl_result, UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_bg_color(s_result_dot, UI_COLOR_SUCCESS, 0);
        lv_label_set_text(s_lbl_status, "Match found!");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(s_lbl_result, LV_SYMBOL_CLOSE " Unknown");
        lv_obj_set_style_text_color(s_lbl_result, UI_COLOR_ERROR, 0);
        lv_obj_set_style_bg_color(s_result_dot, UI_COLOR_ERROR, 0);
        lv_label_set_text(s_lbl_status, "No match");
        lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_WARNING, 0);
    }
}

/* ── Delete all callback ────────────────────────────────────────── */
static void delete_cb(lv_event_t *e)
{
    (void)e;
    memset(s_faces, 0, sizeof(s_faces));
    s_face_count = 0;
    face_database_clear();
    refresh_gallery();
    lv_label_set_text(s_lbl_status, "Database cleared");
    lv_label_set_text(s_lbl_result, "---");
    lv_label_set_text(s_lbl_confidence, "0%");
    lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_color(s_result_dot, UI_COLOR_TEXT_DIM, 0);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    memset(s_faces, 0, sizeof(s_faces));
    s_face_count = 0;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "A21 \xe2\x80\x94 Face Database");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    /* Status */
    s_lbl_status = lv_label_create(parent);
    lv_label_set_text(s_lbl_status, LV_SYMBOL_EYE_OPEN " Ready");
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(s_lbl_status, LV_ALIGN_TOP_MID, 0, 26);

    /* === Preview area === */
    lv_obj_t *preview_card = example_card_create(parent, PREVIEW_W + 16,
                                                  PREVIEW_H + 16,
                                                  lv_color_hex(0x0A1628));
    lv_obj_align(preview_card, LV_ALIGN_TOP_MID, -80, 48);

    s_img_preview = lv_image_create(preview_card);
    lv_obj_set_size(s_img_preview, PREVIEW_W, PREVIEW_H);
    lv_obj_center(s_img_preview);

    lv_obj_t *ph = lv_label_create(preview_card);
    lv_label_set_text(ph, LV_SYMBOL_IMAGE " Camera Preview");
    lv_obj_set_style_text_color(ph, UI_COLOR_TEXT_DIM, 0);
    lv_obj_center(ph);

    /* === Result panel (right of preview) === */
    lv_obj_t *result_card = example_card_create(parent, 160, PREVIEW_H + 16,
                                                 UI_COLOR_CARD_BG);
    lv_obj_align_to(result_card, preview_card, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t *res_title = lv_label_create(result_card);
    lv_label_set_text(res_title, "Result");
    lv_obj_set_style_text_color(res_title, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(res_title, &lv_font_montserrat_14, 0);
    lv_obj_align(res_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_result_dot = lv_obj_create(result_card);
    lv_obj_set_size(s_result_dot, 24, 24);
    lv_obj_set_style_bg_color(s_result_dot, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(s_result_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_result_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_result_dot, 0, 0);
    lv_obj_clear_flag(s_result_dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(s_result_dot, LV_ALIGN_CENTER, 0, -20);

    s_lbl_confidence = lv_label_create(result_card);
    lv_label_set_text(s_lbl_confidence, "0%");
    lv_obj_set_style_text_font(s_lbl_confidence, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_lbl_confidence, UI_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_confidence, LV_ALIGN_CENTER, 0, 10);

    s_lbl_result = lv_label_create(result_card);
    lv_label_set_text(s_lbl_result, "---");
    lv_obj_set_style_text_font(s_lbl_result, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_result, UI_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_result, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* === Gallery row === */
    s_gallery_cont = lv_obj_create(parent);
    lv_obj_set_size(s_gallery_cont, GALLERY_W, GALLERY_H + 10);
    lv_obj_align(s_gallery_cont, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_flex_flow(s_gallery_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s_gallery_cont, 6, 0);
    lv_obj_set_style_bg_color(s_gallery_cont, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(s_gallery_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_gallery_cont, 8, 0);
    lv_obj_set_style_border_width(s_gallery_cont, 1, 0);
    lv_obj_set_style_border_color(s_gallery_cont, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_pad_all(s_gallery_cont, 6, 0);

    lv_obj_t *gal_ph = lv_label_create(s_gallery_cont);
    lv_label_set_text(gal_ph, "No enrolled faces");
    lv_obj_set_style_text_color(gal_ph, UI_COLOR_TEXT_DIM, 0);

    /* === Control buttons === */
    /* Enroll */
    lv_obj_t *btn_enroll = lv_btn_create(parent);
    lv_obj_set_size(btn_enroll, 120, 40);
    lv_obj_align(btn_enroll, LV_ALIGN_BOTTOM_MID, -130, -10);
    lv_obj_set_style_bg_color(btn_enroll, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(btn_enroll, 8, 0);
    lv_obj_add_event_cb(btn_enroll, enroll_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *el = lv_label_create(btn_enroll);
    lv_label_set_text(el, LV_SYMBOL_PLUS " Enroll");
    lv_obj_set_style_text_color(el, lv_color_white(), 0);
    lv_obj_center(el);

    /* Recognize */
    lv_obj_t *btn_recog = lv_btn_create(parent);
    lv_obj_set_size(btn_recog, 120, 40);
    lv_obj_align(btn_recog, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_recog, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(btn_recog, 8, 0);
    lv_obj_add_event_cb(btn_recog, recognize_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(btn_recog);
    lv_label_set_text(rl, LV_SYMBOL_EYE_OPEN " Match");
    lv_obj_set_style_text_color(rl, lv_color_white(), 0);
    lv_obj_center(rl);

    /* Delete all */
    lv_obj_t *btn_del = lv_btn_create(parent);
    lv_obj_set_size(btn_del, 120, 40);
    lv_obj_align(btn_del, LV_ALIGN_BOTTOM_MID, 130, -10);
    lv_obj_set_style_bg_color(btn_del, UI_COLOR_ERROR, 0);
    lv_obj_set_style_radius(btn_del, 8, 0);
    lv_obj_add_event_cb(btn_del, delete_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *dl = lv_label_create(btn_del);
    lv_label_set_text(dl, LV_SYMBOL_TRASH " Clear");
    lv_obj_set_style_text_color(dl, lv_color_white(), 0);
    lv_obj_center(dl);

    /* Initialize face detection */
    face_detect_init();
}
