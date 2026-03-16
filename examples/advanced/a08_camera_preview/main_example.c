/**
 * @file    main_example.c
 * @brief   Camera Preview — USB UVC to LVGL Canvas
 *
 * @description
 *   USB UVC camera -> RGB565 frame -> LVGL canvas display.
 *   320x240 resolution, FPS counter, start/stop button.
 *   Canvas buffer in PSRAM (150 KB).
 *
 * @board    AI Kit (KIT_PSE84_AI) only
 * @author   TESAIoT
 */

#include "example_common.h"
#include "camera_manager.h"
#include "usb_uvc.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define CAM_WIDTH           320
#define CAM_HEIGHT          240
#define CAM_BPP             2       /* RGB565 = 2 bytes per pixel */
#define CAM_BUF_SIZE        (CAM_WIDTH * CAM_HEIGHT * CAM_BPP)
#define FPS_WINDOW          10
#define FRAME_POLL_MS       33      /* ~30 FPS target */

#define COLOR_BG            lv_color_hex(0x142240)
#define COLOR_TEXT           lv_color_hex(0xE0E0E0)
#define COLOR_SUCCESS       lv_color_hex(0x4CAF50)
#define COLOR_ERROR         lv_color_hex(0xF44336)

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *canvas;
    lv_obj_t    *fps_label;
    lv_obj_t    *status_label;
    lv_obj_t    *btn_toggle;
    lv_obj_t    *btn_label;
    uint8_t     *canvas_buf;
    TaskHandle_t preview_task;
    volatile bool running;
    volatile bool stop_requested;
    uint32_t     frame_times[FPS_WINDOW];
    uint8_t      frame_idx;
    uint32_t     total_frames;
} camera_ctx_t;

static camera_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Calculate FPS from sliding window
 * --------------------------------------------------------------------------- */
static float calc_fps(camera_ctx_t *ctx)
{
    if (ctx->total_frames < 2) return 0.0f;

    uint8_t count = (ctx->total_frames < FPS_WINDOW) ? ctx->total_frames : FPS_WINDOW;
    uint8_t oldest = (ctx->frame_idx + FPS_WINDOW - count) % FPS_WINDOW;
    uint32_t newest_time = ctx->frame_times[(ctx->frame_idx + FPS_WINDOW - 1) % FPS_WINDOW];
    uint32_t oldest_time = ctx->frame_times[oldest];

    uint32_t elapsed = newest_time - oldest_time;
    if (elapsed == 0) return 0.0f;

    return (float)(count - 1) * 1000.0f / (float)elapsed;
}

/* ---------------------------------------------------------------------------
 * Camera preview task
 * --------------------------------------------------------------------------- */
static void preview_task_fn(void *pvParameters)
{
    camera_ctx_t *ctx = (camera_ctx_t *)pvParameters;

    /* Initialize camera */
    camera_config_t cfg = {
        .width       = CAM_WIDTH,
        .height      = CAM_HEIGHT,
        .format      = CAMERA_FORMAT_RGB565,
        .source      = CAMERA_SOURCE_USB,
    };

    cy_rslt_t res = camera_manager_init(&cfg);
    if (res != CY_RSLT_SUCCESS) {
        lv_lock();
        lv_label_set_text(ctx->status_label, "Camera init failed!");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_ERROR, 0);
        lv_unlock();
        ctx->running = false;
        vTaskDelete(NULL);
        return;
    }

    res = camera_manager_start();
    if (res != CY_RSLT_SUCCESS) {
        lv_lock();
        lv_label_set_text(ctx->status_label, "Camera start failed!");
        lv_unlock();
        ctx->running = false;
        vTaskDelete(NULL);
        return;
    }

    lv_lock();
    lv_label_set_text(ctx->status_label, "Streaming...");
    lv_obj_set_style_text_color(ctx->status_label, COLOR_SUCCESS, 0);
    lv_unlock();

    while (!ctx->stop_requested) {
        camera_frame_t frame;
        res = camera_manager_get_frame(&frame, pdMS_TO_TICKS(100));
        if (res != CY_RSLT_SUCCESS) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* Validate frame */
        if (frame.data == NULL || frame.size != CAM_BUF_SIZE) {
            camera_manager_release_frame(&frame);
            continue;
        }

        /* Copy frame to canvas buffer and invalidate */
        lv_lock();
        memcpy(ctx->canvas_buf, frame.data, CAM_BUF_SIZE);
        lv_obj_invalidate(ctx->canvas);

        /* Track FPS */
        ctx->frame_times[ctx->frame_idx] = xTaskGetTickCount();
        ctx->frame_idx = (ctx->frame_idx + 1) % FPS_WINDOW;
        ctx->total_frames++;

        float fps = calc_fps(ctx);
        char fps_str[32];
        snprintf(fps_str, sizeof(fps_str), "FPS: %.1f  Frames: %lu",
                 fps, (unsigned long)ctx->total_frames);
        lv_label_set_text(ctx->fps_label, fps_str);

        lv_unlock();

        camera_manager_release_frame(&frame);
        vTaskDelay(pdMS_TO_TICKS(FRAME_POLL_MS));
    }

    /* Cleanup */
    camera_manager_stop();
    camera_manager_deinit();

    lv_lock();
    lv_label_set_text(ctx->status_label, "Stopped");
    lv_obj_set_style_text_color(ctx->status_label, COLOR_TEXT, 0);
    lv_label_set_text(ctx->btn_label, LV_SYMBOL_PLAY " Start");
    lv_unlock();

    ctx->running = false;
    ctx->stop_requested = false;
    vTaskDelete(NULL);
}

/* ---------------------------------------------------------------------------
 * Toggle button callback
 * --------------------------------------------------------------------------- */
static void btn_toggle_cb(lv_event_t *e)
{
    camera_ctx_t *ctx = (camera_ctx_t *)lv_event_get_user_data(e);

    if (ctx->running) {
        /* Stop */
        ctx->stop_requested = true;
        lv_label_set_text(ctx->btn_label, "Stopping...");
    } else {
        /* Start */
        ctx->running = true;
        ctx->total_frames = 0;
        ctx->frame_idx = 0;
        lv_label_set_text(ctx->btn_label, LV_SYMBOL_STOP " Stop");
        lv_label_set_text(ctx->status_label, "Starting camera...");

        xTaskCreate(preview_task_fn, "cam_preview", 4096, ctx, 4, &ctx->preview_task);
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
    lv_label_set_text(title, LV_SYMBOL_IMAGE " Camera Preview");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Status */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Start to begin");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 36);

    /* FPS counter */
    s_ctx.fps_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.fps_label, "FPS: --");
    lv_obj_set_style_text_color(s_ctx.fps_label, COLOR_SUCCESS, 0);
    lv_obj_align(s_ctx.fps_label, LV_ALIGN_TOP_RIGHT, -16, 36);

    /* Allocate canvas buffer from PSRAM */
    s_ctx.canvas_buf = (uint8_t *)lv_malloc(CAM_BUF_SIZE);
    if (s_ctx.canvas_buf == NULL) {
        lv_label_set_text(s_ctx.status_label, "PSRAM alloc failed!");
        return;
    }
    memset(s_ctx.canvas_buf, 0, CAM_BUF_SIZE);

    /* Canvas widget */
    s_ctx.canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(s_ctx.canvas, s_ctx.canvas_buf,
                         CAM_WIDTH, CAM_HEIGHT, LV_COLOR_FORMAT_RGB565);
    lv_obj_align(s_ctx.canvas, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_border_color(s_ctx.canvas, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(s_ctx.canvas, 2, 0);
    lv_obj_set_style_radius(s_ctx.canvas, 4, 0);

    /* Resolution label */
    lv_obj_t *res_lbl = lv_label_create(parent);
    char res_str[32];
    snprintf(res_str, sizeof(res_str), "%ux%u RGB565", CAM_WIDTH, CAM_HEIGHT);
    lv_label_set_text(res_lbl, res_str);
    lv_obj_set_style_text_color(res_lbl, COLOR_TEXT, 0);
    lv_obj_align_to(res_lbl, s_ctx.canvas, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

    /* Toggle button */
    s_ctx.btn_toggle = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_toggle, 140, 44);
    lv_obj_align(s_ctx.btn_toggle, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_toggle, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(s_ctx.btn_toggle, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_toggle, btn_toggle_cb, LV_EVENT_CLICKED, &s_ctx);

    s_ctx.btn_label = lv_label_create(s_ctx.btn_toggle);
    lv_label_set_text(s_ctx.btn_label, LV_SYMBOL_PLAY " Start");
    lv_obj_center(s_ctx.btn_label);
}
