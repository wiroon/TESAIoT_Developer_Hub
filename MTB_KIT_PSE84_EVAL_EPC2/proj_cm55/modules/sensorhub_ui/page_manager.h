/*******************************************************************************
 * File Name: page_manager.h
 *
 * Description: Page-based navigation manager for TESAIoT SensorHub UI.
 *              Replaces the TabView with Smart Home-style card menu + pages.
 *              Create-on-demand, destroy-on-leave for minimal LVGL heap usage.
 *
 *              Navigation: Home card grid -> tap card -> full page -> Back.
 *              Uses lv_screen_load_anim() with auto_del for clean transitions.
 *
 *******************************************************************************/

#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "lvgl.h"
#include "ipc_sensorhub.h"
#include "ipc_communication.h"
#include "bsp_feature_flags.h"

#define PM_MAX_PAGES       16
#define PM_NAV_STACK_DEPTH 12

/* Transition animation duration (ms). Must be >= 10 to avoid LVGL #4212 */
#define PM_ANIM_TIME_MS    300

/*******************************************************************************
 * Page IDs — BSP-conditional, matching sensor availability per board.
 *******************************************************************************/
typedef enum {
    PAGE_ID_HOME = 0,
    PAGE_ID_DASHBOARD,
    PAGE_ID_MOTION,
#if BSP_HAS_DPS368 || BSP_HAS_SHT40
    PAGE_ID_ENVIRON,
#endif
#if BSP_HAS_CAPSENSE || BSP_HAS_POTENTIOMETER
    PAGE_ID_CONTROLS,
#endif
    PAGE_ID_JOYSTICK,
    PAGE_ID_PLAYGROUND,
    PAGE_ID_FACE_IDENTIFICATION,
    PAGE_ID_SMART_WATCH,
    PAGE_ID_SPECTRUM_ANALYZER,
    PAGE_ID_WIFI_CONNECT,
    PAGE_ID_HSM,
    PAGE_ID_COUNT
} page_id_t;

/*******************************************************************************
 * Page Definition — function pointers for lifecycle management.
 *******************************************************************************/
typedef struct {
    const char *name;           /* Display name (e.g., "Dashboard") */
    const char *subtitle;       /* Brief description for home card */
    uint32_t    accent_color;   /* Card border + title color (hex) */
    lv_obj_t *(*create_cb)(void);                         /* Build screen */
    void      (*render_cb)(sensorhub_snapshot_t *snap);   /* Update data */
    void      (*destroy_cb)(void);                        /* Pre-destroy cleanup */
    bool        cacheable;      /* If true, screen survives nav-away (not destroyed) */
} page_def_t;

/*******************************************************************************
 * Page Manager State
 *******************************************************************************/
typedef struct {
    page_def_t  pages[PM_MAX_PAGES];
    uint8_t     page_count;
    page_id_t   current_page;
    page_id_t   nav_stack[PM_NAV_STACK_DEPTH];
    int8_t      nav_top;        /* -1 = empty stack */
    lv_obj_t   *status_lbl;     /* IDE Connected label (re-created per page) */
    lv_obj_t   *time_lbl;      /* Time display (re-created per page) */
    lv_obj_t   *wifi_lbl;      /* WiFi icon (re-created per page) */
    bool        animating;      /* Guard: true during screen transition */
    lv_obj_t   *cached_screens[PM_MAX_PAGES];  /* Screen cache for cacheable pages */
} page_manager_t;

/*******************************************************************************
 * Public API
 *******************************************************************************/

/** Initialize page manager (zeroes state, sets nav_top = -1). */
void pm_init(page_manager_t *pm);

/** Register a page definition. */
void pm_register(page_manager_t *pm, page_id_t id, const page_def_t *def);

/** Navigate forward to a page (push current to stack, slide left). */
void pm_navigate(page_manager_t *pm, page_id_t target);

/** Navigate back to previous page (pop stack, slide right). */
void pm_back(page_manager_t *pm);

/** Dispatch render to current page's render_cb. Skips if animating. */
void pm_render(page_manager_t *pm, sensorhub_snapshot_t *snap);

/** Get the currently active page ID. */
page_id_t pm_current(const page_manager_t *pm);

/** Check if a navigation animation is in progress. */
bool pm_is_animating(const page_manager_t *pm);

/*******************************************************************************
 * Helper: Create common page elements (re-used by all pages)
 *******************************************************************************/

/**
 * Create status bar on a screen (top 32px).
 * Re-created per page since auto_del destroys the old one.
 *
 * @param screen        Screen object (lv_obj_create(NULL) root).
 * @param out_status    Output: right-aligned status label (IDE Connected).
 * @return Status bar container.
 */
lv_obj_t *pm_create_status_bar(lv_obj_t *screen, lv_obj_t **out_status);

/**
 * Create a Back button (top-left, below status bar).
 *
 * @param screen  Screen object.
 * @param pm      Page manager (for back navigation callback).
 * @return Button object.
 */
lv_obj_t *pm_create_back_button(lv_obj_t *screen, page_manager_t *pm);

/**
 * Create full page header: status bar + back button + title.
 * Returns the content area object (positioned below header).
 *
 * @param screen       Screen object.
 * @param pm           Page manager.
 * @param title        Page title text.
 * @param title_color  Title text color (hex).
 * @return Content area container (below header, full remaining height).
 */
lv_obj_t *pm_create_page_with_header(lv_obj_t *screen, page_manager_t *pm,
                                      const char *title, uint32_t title_color);

/*******************************************************************************
 * Singleton Access (for external callers like IPC handlers)
 *******************************************************************************/

/** Get the global page manager instance (set by sensorhub_ui_init). */
page_manager_t *pm_get_instance(void);

/** Set the global page manager instance (called by sensorhub_ui_init). */
void pm_set_instance(page_manager_t *pm);

/* Shared IPC message buffer placed in .cy_sharedmem.
 * Reused by multiple UI pages to avoid exhausting the 4KB allocatable shared
 * window needed by IPC pipe payload pointers. */
extern ipc_msg_t g_pm_ipc_msg_shared;

#endif /* PAGE_MANAGER_H */
