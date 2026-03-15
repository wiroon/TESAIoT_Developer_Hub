/**
 * @file    main_example.c
 * @brief   Face Detection with ML — SSD Nano on Camera Frames
 *
 * @description
 *   Camera frame -> SSD Nano face detection ML inference -> bounding box
 *   overlay on LVGL canvas. Shows confidence %, face count, inference time.
 *   Model runs on CM55 with NN acceleration.
 *
 * @board    AI Kit (KIT_PSE84_AI) only
 * @author   TESAIoT
 */

#include "example_common.h"
#include "face_id_engine.h"
#include "camera_manager.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define CAM_WIDTH           320
#define CAM_HEIGHT          240
#define CAM_BPP             2
#define CAM_BUF_SIZE        (CAM_WIDTH * CAM_HEIGHT * CAM_BPP)
#define MODEL_INPUT_SIZE    160
#define MAX_FACES           10
#define CONFIDENCE_THRESH   0.5f
#define FRAME_POLL_MS       50

#define COLOR_BG            lv_color_hex(0x142240)
#define COLOR_CARD          lv_color_hex(0x1A2744)
#define COLOR_TEXT           lv_color_hex(0xE0E0E0)
#define COLOR_BOX            lv_color_hex(0x4CAF50)
#define COLOR_BOX_LOW        lv_color_hex(0xFF9800)
#define COLOR_ERROR          lv_color_hex(0xF44336)
#define COLOR_ACCENT         lv_color_hex(0x2196F3)

/* ---------------------------------------------------------------------------
 * Data Structures
 * --------------------------------------------------------------------------- */
typedef struct {
    float x;        /* Normalized [0..1] top-left X */
    float y;        /* Normalized [0..1] top-left Y */
    float w;        /* Normalized width */
    float h;        /* Normalized height */
    float confidence;
} face_bbox_t;

typedef struct {
    face_bbox_t faces[MAX_FACES];
    uint8_t     count;
    uint32_t    inference_ms;
} detection_result_t;

typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *canvas;
    lv_obj_t    *face_count_lbl;
    lv_obj_t    *infer_time_lbl;
    lv_obj_t    *fps_label;
    lv_obj_t    *status_label;
    lv_obj_t    *btn_toggle;
    lv_obj_t    *btn_label;
    uint8_t     *canvas_buf;
    uint8_t     *model_input_buf;
    TaskHandle_t detect_task;
    volatile bool running;
    volatile bool stop_req;
    uint32_t     frame_count;
    uint32_t     last_fps_tick;
    float        avg_infer_ms;
} face_detect_ctx_t;

static face_detect_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Bilinear resize RGB565 for model input
 * --------------------------------------------------------------------------- */
static void resize_rgb565(const uint8_t *src, uint16_t sw, uint16_t sh,
                          uint8_t *dst, uint16_t dw, uint16_t dh)
{
    float x_ratio = (float)sw / (float)dw;
    float y_ratio = (float)sh / (float)dh;

    for (uint16_t dy = 0; dy < dh; dy++) {
        float src_y = dy * y_ratio;
        uint16_t sy = (uint16_t)src_y;
        if (sy >= sh - 1) sy = sh - 2;

        for (uint16_t dx = 0; dx < dw; dx++) {
            float src_x = dx * x_ratio;
            uint16_t sx = (uint16_t)src_x;
            if (sx >= sw - 1) sx = sw - 2;

            /* Simple nearest-neighbor for speed (bilinear adds 2x latency) */
            uint32_t src_off = (sy * sw + sx) * 2;
            uint32_t dst_off = (dy * dw + dx) * 2;
            dst[dst_off]     = src[src_off];
            dst[dst_off + 1] = src[src_off + 1];
        }
    }
}

/* ---------------------------------------------------------------------------
 * Draw bounding boxes on canvas
 * --------------------------------------------------------------------------- */
static void draw_bboxes(face_detect_ctx_t *ctx, const detection_result_t *result)
{
    lv_layer_t layer;
    lv_canvas_init_layer(ctx->canvas, &layer);

    for (uint8_t i = 0; i < result->count; i++) {
        const face_bbox_t *f = &result->faces[i];

        /* Scale normalized coords to canvas pixels */
        int16_t bx = (int16_t)(f->x * CAM_WIDTH);
        int16_t by = (int16_t)(f->y * CAM_HEIGHT);
        int16_t bw = (int16_t)(f->w * CAM_WIDTH);
        int16_t bh = (int16_t)(f->h * CAM_HEIGHT);

        /* Clamp to canvas bounds */
        if (bx < 0) bx = 0;
        if (by < 0) by = 0;
        if (bx + bw > CAM_WIDTH)  bw = CAM_WIDTH - bx;
        if (by + bh > CAM_HEIGHT) bh = CAM_HEIGHT - by;

        /* Choose color based on confidence */
        lv_color_t box_color = (f->confidence > 0.7f) ? COLOR_BOX : COLOR_BOX_LOW;

        /* Draw rectangle border */
        lv_draw_rect_dsc_t rect_dsc;
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_opa      = LV_OPA_TRANSP;
        rect_dsc.border_color = box_color;
        rect_dsc.border_width = 2;
        rect_dsc.radius       = 2;

        lv_area_t area = { bx, by, bx + bw, by + bh };
        lv_draw_rect(&layer, &rect_dsc, &area);

        /* Draw confidence label above box */
        char conf_str[16];
        snprintf(conf_str, sizeof(conf_str), "%.0f%%", f->confidence * 100.0f);

        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = box_color;
        label_dsc.font  = &lv_font_montserrat_14;
        label_dsc.text  = conf_str;

        lv_area_t label_area = { bx, by - 16, bx + 50, by };
        lv_draw_label(&layer, &label_dsc, &label_area);
    }

    lv_canvas_finish_layer(ctx->canvas, &layer);
}

/* ---------------------------------------------------------------------------
 * Detection task
 * --------------------------------------------------------------------------- */
static void detect_task_fn(void *pvParameters)
{
    face_detect_ctx_t *ctx = (face_detect_ctx_t *)pvParameters;

    /* Init camera */
    camera_config_t cfg = {
        .width  = CAM_WIDTH,
        .height = CAM_HEIGHT,
        .format = CAMERA_FORMAT_RGB565,
        .source = CAMERA_SOURCE_USB,
    };

    cy_rslt_t res = camera_manager_init(&cfg);
    if (res != CY_RSLT_SUCCESS) {
        lv_lock();
        lv_label_set_text(ctx->status_label, "Camera init failed!");
        lv_unlock();
        ctx->running = false;
        vTaskDelete(NULL);
        return;
    }

    /* Init face detection model */
    res = face_detect_init();
    if (res != CY_RSLT_SUCCESS) {
        lv_lock();
        lv_label_set_text(ctx->status_label, "Model init failed!");
        lv_unlock();
        camera_manager_deinit();
        ctx->running = false;
        vTaskDelete(NULL);
        return;
    }

    camera_manager_start();

    lv_lock();
    lv_label_set_text(ctx->status_label, "Detecting faces...");
    lv_obj_set_style_text_color(ctx->status_label, COLOR_ACCENT, 0);
    lv_unlock();

    ctx->last_fps_tick = xTaskGetTickCount();
    uint32_t infer_sum = 0;
    uint32_t infer_count = 0;

    while (!ctx->stop_req) {
        camera_frame_t frame;
        res = camera_manager_get_frame(&frame, pdMS_TO_TICKS(100));
        if (res != CY_RSLT_SUCCESS || frame.data == NULL) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* Copy frame to canvas */
        lv_lock();
        memcpy(ctx->canvas_buf, frame.data, CAM_BUF_SIZE);
        lv_unlock();

        /* Resize for model input */
        resize_rgb565(frame.data, CAM_WIDTH, CAM_HEIGHT,
                      ctx->model_input_buf, MODEL_INPUT_SIZE, MODEL_INPUT_SIZE);

        camera_manager_release_frame(&frame);

        /* Run inference */
        uint32_t t_start = xTaskGetTickCount();
        face_detect_result_t raw_results;
        res = face_detect(ctx->model_input_buf, MODEL_INPUT_SIZE, MODEL_INPUT_SIZE,
                          &raw_results);
        uint32_t t_end = xTaskGetTickCount();
        uint32_t infer_ms = (t_end - t_start) * portTICK_PERIOD_MS;

        detection_result_t det;
        det.count = 0;
        det.inference_ms = infer_ms;

        if (res == CY_RSLT_SUCCESS) {
            /* Filter by confidence threshold */
            for (uint8_t i = 0; i < raw_results.count && det.count < MAX_FACES; i++) {
                if (raw_results.faces[i].confidence >= CONFIDENCE_THRESH) {
                    det.faces[det.count].x          = raw_results.faces[i].x;
                    det.faces[det.count].y          = raw_results.faces[i].y;
                    det.faces[det.count].w          = raw_results.faces[i].w;
                    det.faces[det.count].h          = raw_results.faces[i].h;
                    det.faces[det.count].confidence  = raw_results.faces[i].confidence;
                    det.count++;
                }
            }
        }

        /* Update inference stats */
        infer_sum += infer_ms;
        infer_count++;
        if (infer_count > 100) {
            infer_sum = infer_ms;
            infer_count = 1;
        }

        /* Update UI */
        lv_lock();
        lv_obj_invalidate(ctx->canvas);
        draw_bboxes(ctx, &det);

        char count_str[24];
        snprintf(count_str, sizeof(count_str), "Faces: %u", det.count);
        lv_label_set_text(ctx->face_count_lbl, count_str);

        char infer_str[32];
        snprintf(infer_str, sizeof(infer_str), "Infer: %lu ms (avg %.0f)",
                 (unsigned long)infer_ms, (float)infer_sum / (float)infer_count);
        lv_label_set_text(ctx->infer_time_lbl, infer_str);

        /* FPS */
        ctx->frame_count++;
        uint32_t now = xTaskGetTickCount();
        uint32_t elapsed = now - ctx->last_fps_tick;
        if (elapsed >= pdMS_TO_TICKS(1000)) {
            float fps = (float)ctx->frame_count * 1000.0f /
                        (float)(elapsed * portTICK_PERIOD_MS);
            char fps_str[16];
            snprintf(fps_str, sizeof(fps_str), "FPS: %.1f", fps);
            lv_label_set_text(ctx->fps_label, fps_str);
            ctx->frame_count = 0;
            ctx->last_fps_tick = now;
        }

        lv_unlock();
        vTaskDelay(pdMS_TO_TICKS(FRAME_POLL_MS));
    }

    camera_manager_stop();
    camera_manager_deinit();
    face_detect_deinit();

    lv_lock();
    lv_label_set_text(ctx->status_label, "Stopped");
    lv_label_set_text(ctx->btn_label, LV_SYMBOL_PLAY " Start");
    lv_unlock();

    ctx->running = false;
    ctx->stop_req = false;
    vTaskDelete(NULL);
}

/* ---------------------------------------------------------------------------
 * Toggle callback
 * --------------------------------------------------------------------------- */
static void btn_toggle_cb(lv_event_t *e)
{
    face_detect_ctx_t *ctx = (face_detect_ctx_t *)lv_event_get_user_data(e);

    if (ctx->running) {
        ctx->stop_req = true;
        lv_label_set_text(ctx->btn_label, "Stopping...");
    } else {
        ctx->running = true;
        ctx->frame_count = 0;
        lv_label_set_text(ctx->btn_label, LV_SYMBOL_STOP " Stop");
        xTaskCreate(detect_task_fn, "face_det", 8192, ctx, 4, &ctx->detect_task);
    }
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_IMAGE " Face Detection");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Status */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Start");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 36);

    /* Canvas */
    s_ctx.canvas_buf = (uint8_t *)lv_malloc(CAM_BUF_SIZE);
    s_ctx.model_input_buf = (uint8_t *)lv_malloc(MODEL_INPUT_SIZE * MODEL_INPUT_SIZE * 2);

    if (!s_ctx.canvas_buf || !s_ctx.model_input_buf) {
        lv_label_set_text(s_ctx.status_label, "Memory alloc failed!");
        return;
    }
    memset(s_ctx.canvas_buf, 0, CAM_BUF_SIZE);

    s_ctx.canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(s_ctx.canvas, s_ctx.canvas_buf,
                         CAM_WIDTH, CAM_HEIGHT, LV_COLOR_FORMAT_RGB565);
    lv_obj_align(s_ctx.canvas, LV_ALIGN_LEFT_MID, 8, 16);
    lv_obj_set_style_border_color(s_ctx.canvas, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(s_ctx.canvas, 2, 0);

    /* Info panel */
    lv_obj_t *info = lv_obj_create(parent);
    lv_obj_set_size(info, 220, 160);
    lv_obj_align(info, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_bg_color(info, COLOR_CARD, 0);
    lv_obj_set_style_border_width(info, 1, 0);
    lv_obj_set_style_border_color(info, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(info, 12, 0);
    lv_obj_set_style_pad_all(info, 12, 0);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(info, 8, 0);

    lv_obj_t *info_title = lv_label_create(info);
    lv_label_set_text(info_title, "Detection Stats");
    lv_obj_set_style_text_color(info_title, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(info_title, &lv_font_montserrat_16, 0);

    s_ctx.face_count_lbl = lv_label_create(info);
    lv_label_set_text(s_ctx.face_count_lbl, "Faces: 0");
    lv_obj_set_style_text_color(s_ctx.face_count_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_ctx.face_count_lbl, &lv_font_montserrat_20, 0);

    s_ctx.infer_time_lbl = lv_label_create(info);
    lv_label_set_text(s_ctx.infer_time_lbl, "Infer: -- ms");
    lv_obj_set_style_text_color(s_ctx.infer_time_lbl, COLOR_TEXT, 0);

    s_ctx.fps_label = lv_label_create(info);
    lv_label_set_text(s_ctx.fps_label, "FPS: --");
    lv_obj_set_style_text_color(s_ctx.fps_label, lv_color_hex(0xFFD740), 0);

    lv_obj_t *model_lbl = lv_label_create(info);
    lv_label_set_text(model_lbl, "Model: SSD Nano 160x160");
    lv_obj_set_style_text_color(model_lbl, COLOR_TEXT, 0);

    /* Toggle button */
    s_ctx.btn_toggle = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_toggle, 140, 44);
    lv_obj_align(s_ctx.btn_toggle, LV_ALIGN_BOTTOM_MID, 100, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_toggle, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(s_ctx.btn_toggle, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_toggle, btn_toggle_cb, LV_EVENT_CLICKED, &s_ctx);

    s_ctx.btn_label = lv_label_create(s_ctx.btn_toggle);
    lv_label_set_text(s_ctx.btn_label, LV_SYMBOL_PLAY " Start");
    lv_obj_center(s_ctx.btn_label);
}
