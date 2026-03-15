/*******************************************************************************
 * File Name        : bsp_feature_flags.h
 *
 * Description      : BSP Feature Flags for PSoC Edge E84 projects.
 *                    Enables conditional compilation of sensor drivers,
 *                    MicroPython modules, and LVGL UI pages based on TARGET.
 *
 *                    Flags are set via -DBSP_HAS_XXX=1 in CFLAGS by the
 *                    build system (bsp_features.mk per TARGET).
 *
 * Supported targets:
 *   KIT_PSE84_AI            - PSoC Edge AI Dev Kit
 *
 * Usage in C:
 *   #include "bsp_feature_flags.h"
 *   #if BSP_HAS_DPS368
 *       dps368_init();
 *   #endif
 *
 *******************************************************************************/

#ifndef BSP_FEATURE_FLAGS_H
#define BSP_FEATURE_FLAGS_H

/*******************************************************************************
 * Common sensors (both boards)
 *******************************************************************************/
#ifndef BSP_HAS_BMI270
#define BSP_HAS_BMI270          0
#endif

#ifndef BSP_HAS_BMM350
#define BSP_HAS_BMM350          0
#endif

/*******************************************************************************
 * AI Dev Kit sensors (KIT_PSE84_AI)
 *******************************************************************************/
#ifndef BSP_HAS_DPS368
#define BSP_HAS_DPS368          0
#endif

#ifndef BSP_HAS_SHT40
#define BSP_HAS_SHT40           0
#endif

#ifndef BSP_HAS_RADAR
#define BSP_HAS_RADAR           0
#endif

/*******************************************************************************
 * Optional peripherals (requires base board or extension)
 *******************************************************************************/
#ifndef BSP_HAS_CAPSENSE
#define BSP_HAS_CAPSENSE        0
#endif

#ifndef BSP_HAS_POTENTIOMETER
#define BSP_HAS_POTENTIOMETER   0
#endif

#ifndef BSP_HAS_AUDIO_CODEC
#define BSP_HAS_AUDIO_CODEC     0
#endif

/*******************************************************************************
 * Audio (both boards have PDM mic, different configs)
 *******************************************************************************/
#ifndef BSP_HAS_PDM_MIC
#define BSP_HAS_PDM_MIC         0
#endif

/*******************************************************************************
 * Runtime feature toggles (safety gates)
 *******************************************************************************/
#ifndef TESAIOT_ENABLE_FACE_RUNTIME
#define TESAIOT_ENABLE_FACE_RUNTIME 0
#endif

#endif /* BSP_FEATURE_FLAGS_H */
