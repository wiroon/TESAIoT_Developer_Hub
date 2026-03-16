/**
 * @file    main_example.c
 * @brief   Camera Source Switch — DVP (OV7675) vs USB UVC
 *
 * @description
 *   Runtime switch between DVP and USB camera. Toggle button + status display
 *   showing current source, resolution, FPS. Canvas preview.
 *
 * @board    AI Kit (KIT_PSE84_AI) only
 * @author   TESAIoT
 */

#include "example_common.h"
#include "camera_manager.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define CAM_WIDTH           320
#define CAM_HEIGHT          240
#define CAM_BUF_SIZE        (CAM_WIDTH * CAM_HEIGHT * 2)
#define FRAME_POLL_MS       33

#define COLOR_BG            lv_color_hex(0x142240)
#define COLOR_CARD          lv_color_hex(0x1A2744)
#define COLOR_TEXT           lv_color_hex(0xE0E0E0)
#define COLOR_DVP            lv_color_hex(0x4CAF50)
#define COLOR_USB            lv_color_hex(0x2196F3)
#define COLOR_ERROR          lv_color_hex(0xF44336)

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t         *parent;
    lv_obj_t         *canvas;
    lv_obj_t         *source_label;
    lv_obj_t         *res_label;
    lv_obj_t         *fps_label;
    lv_obj_t         *status_label;
    lv_obj_t         *btn_switch;
    lv_obj_t         *btn_label;
    lv_obj_t         *indicator;
    uint8_t          *canvas_buf;
    camera_source_t   current_source;
    TaskHandle_t      preview_task;
    volatile bool     running;
    volatile bool     switching;
    uint32_t          frame_count;
    uint32_t          last_fps_tick;
    float             current_fps;
} cam_switch_ctx_t;

static cam_switch_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Source name helper
 * --------------------------------------------------------------------------- */
static const char *source_name(camera_source_t src)
{
    switch (src) {
        case CAMERA_SOURCE_DVP: return "DVP (OV7675)";
        case CAMERA_SOURCE_USB: return "USB UVC";
        default:                return "Unknown";
    }
}

/* ---------------------------------------------------------------------------
 * Update info panel
 * --------------------------------------------------------------------------- */
static void update_info(cam_switch_ctx_t *ctx)
{
    lv_color_t color = (ctx->current_source == CAMERA_SOURCE_DVP) ? COLOR_DVP : COLOR_USB;

    char src_str[48];
    snprintf(src_str, sizeof(src_str), "Source: %s", source_name(ctx->current_source));
    lv_label_set_text(ctx->source_label, src_str);
    lv_obj_set_style_text_color(ctx->source_label, color, 0);

    lv_label_set_text(ctx->res_label, "320x240 RGB565");

    lv_obj_set_style_bg_color(ctx->indicator, color, 0);
}

/* ---------------------------------------------------------------------------
 * Preview task
 * --------------------------------------------------------------------------- */
static void preview_task_fn(void *pvParameters)
{
    cam_switch_ctx_t *ctx = (cam_switch_ctx_t *)pvParameters;

    /* Initialize with DVP as default */
    camera_config_t cfg = {
        .width  = CAM_WIDTH,
        .height = CAM_HEIGHT,
        .format = CAMERA_FORMAT_RGB565,
        .source = ctx->current_source,
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

    camera_manager_start();

    lv_lock();
    lv_label_set_text(ctx->status_label, "Streaming");
    lv_obj_set_style_text_color(ctx->status_label, COLOR_DVP, 0);
    update_info(ctx);
    lv_unlock();

    ctx->last_fps_tick = xTaskGetTickCount();

    while (ctx->running) {
        /* Handle source switch request */
        if (ctx->switching) {
            camera_manager_stop();

            camera_source_t new_src = (ctx->current_source == CAMERA_SOURCE_DVP)
                                      ? CAMERA_SOURCE_USB : CAMERA_SOURCE_DVP;

            lv_lock();
            lv_label_set_text(ctx->status_label, "Switching...");
            lv_obj_set_style_text_color(ctx->status_label, lv_color_hex(0xFF9800), 0);
            lv_unlock();

            res = camera_manager_switch_source(new_src);
            if (res == CY_RSLT_SUCCESS) {
                ctx->current_source = new_src;
                camera_manager_start();

                lv_lock();
                update_info(ctx);
                const char *target = (new_src == CAMERA_SOURCE_DVP) ? "DVP" : "USB";
                lv_label_set_text(ctx->btn_label, (new_src == CAMERA_SOURCE_DVP)
                    ? "Switch to USB" : "Switch to DVP");

                char status[32];
                snprintf(status, sizeof(status), "Streaming (%s)", target);
                lv_label_set_text(ctx->status_label, status);
                lv_color_t c = (new_src == CAMERA_SOURCE_DVP) ? COLOR_DVP : COLOR_USB;
                lv_obj_set_style_text_color(ctx->status_label, c, 0);
                lv_unlock();
            } else {
                /* Revert to previous source */
                camera_manager_start();
                lv_lock();
                lv_label_set_text(ctx->status_label, "Switch failed — reverting");
                lv_obj_set_style_text_color(ctx->status_label, COLOR_ERROR, 0);
                lv_unlock();
            }

            ctx->switching = false;
            ctx->frame_count = 0;
            ctx->last_fps_tick = xTaskGetTickCount();
        }

        /* Get frame */
        camera_frame_t frame;
        res = camera_manager_get_frame(&frame, pdMS_TO_TICKS(100));
        if (res == CY_RSLT_SUCCESS && frame.data != NULL && frame.size == CAM_BUF_SIZE) {
            lv_lock();
            memcpy(ctx->canvas_buf, frame.data, CAM_BUF_SIZE);
            lv_obj_invalidate(ctx->canvas);

            /* FPS calculation (every second) */
            ctx->frame_count++;
            uint32_t now = xTaskGetTickCount();
            uint32_t elapsed = now - ctx->last_fps_tick;
            if (elapsed >= pdMS_TO_TICKS(1000)) {
                ctx->current_fps = (float)ctx->frame_count * 1000.0f /
                                   (float)(elapsed * portTICK_PERIOD_MS);
                ctx->frame_count = 0;
                ctx->last_fps_tick = now;

                char fps_str[24];
                snprintf(fps_str, sizeof(fps_str), "FPS: %.1f", ctx->current_fps);
                lv_label_set_text(ctx->fps_label, fps_str);
            }
            lv_unlock();

            camera_manager_release_frame(&frame);
        }

        vTaskDelay(pdMS_TO_TICKS(FRAME_POLL_MS));
    }

    camera_manager_stop();
    camera_manager_deinit();
    vTaskDelete(NULL);
}

/* ---------------------------------------------------------------------------
 * Switch button callback
 * --------------------------------------------------------------------------- */
static void btn_switch_cb(lv_event_t *e)
{
    cam_switch_ctx_t *ctx = (cam_switch_ctx_t *)lv_event_get_user_data(e);
    if (!ctx->switching) {
        ctx->switching = true;
    }
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;
    s_ctx.current_source = CAMERA_SOURCE_DVP;
    s_ctx.running = true;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_IMAGE " Camera Switch");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Status + indicator */
    s_ctx.indicator = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.indicator, 14, 14);
    lv_obj_set_style_radius(s_ctx.indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_ctx.indicator, COLOR_DVP, 0);
    lv_obj_set_style_bg_opa(s_ctx.indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ctx.indicator, 0, 0);
    lv_obj_align(s_ctx.indicator, LV_ALIGN_TOP_LEFT, 16, 40);

    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Initializing...");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 38, 38);

    /* Info panel (right side) */
    lv_obj_t *info = lv_obj_create(parent);
    lv_obj_set_size(info, 200, 90);
    lv_obj_align(info, LV_ALIGN_TOP_RIGHT, -8, 56);
    lv_obj_set_style_bg_color(info, COLOR_CARD, 0);
    lv_obj_set_style_border_width(info, 1, 0);
    lv_obj_set_style_border_color(info, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(info, 8, 0);
    lv_obj_set_style_pad_all(info, 8, 0);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(info, 4, 0);

    s_ctx.source_label = lv_label_create(info);
    lv_label_set_text(s_ctx.source_label, "Source: DVP (OV7675)");
    lv_obj_set_style_text_color(s_ctx.source_label, COLOR_DVP, 0);

    s_ctx.res_label = lv_label_create(info);
    lv_label_set_text(s_ctx.res_label, "320x240 RGB565");
    lv_obj_set_style_text_color(s_ctx.res_label, COLOR_TEXT, 0);

    s_ctx.fps_label = lv_label_create(info);
    lv_label_set_text(s_ctx.fps_label, "FPS: --");
    lv_obj_set_style_text_color(s_ctx.fps_label, lv_color_hex(0xFFD740), 0);

    /* Canvas */
    s_ctx.canvas_buf = (uint8_t *)lv_malloc(CAM_BUF_SIZE);
    if (s_ctx.canvas_buf == NULL) {
        lv_label_set_text(s_ctx.status_label, "Buffer alloc failed!");
        return;
    }
    memset(s_ctx.canvas_buf, 0, CAM_BUF_SIZE);

    s_ctx.canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(s_ctx.canvas, s_ctx.canvas_buf,
                         CAM_WIDTH, CAM_HEIGHT, LV_COLOR_FORMAT_RGB565);
    lv_obj_align(s_ctx.canvas, LV_ALIGN_LEFT_MID, 16, 20);
    lv_obj_set_style_border_color(s_ctx.canvas, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(s_ctx.canvas, 2, 0);

    /* Switch button */
    s_ctx.btn_switch = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_switch, 180, 44);
    lv_obj_align(s_ctx.btn_switch, LV_ALIGN_BOTTOM_MID, 100, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_switch, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(s_ctx.btn_switch, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_switch, btn_switch_cb, LV_EVENT_CLICKED, &s_ctx);

    s_ctx.btn_label = lv_label_create(s_ctx.btn_switch);
    lv_label_set_text(s_ctx.btn_label, "Switch to USB");
    lv_obj_center(s_ctx.btn_label);

    /* Start preview task */
    xTaskCreate(preview_task_fn, "cam_switch", 4096, &s_ctx, 4, &s_ctx.preview_task);
}
