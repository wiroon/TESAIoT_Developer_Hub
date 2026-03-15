################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Makefile for configuring the Cortex-M55 core project
# Phase 1: Minimal FreeRTOS stub
# Phase 2: LVGL + Display (Waveshare 4.3" DSI)
# Phase 14: WiFi (CYW55513)
# Phase 14.5: TESAIoT + OPTIGA Trust M (secure credentials)
# Phase 15-16: WiFi/MQTT IPC proxy, Sensor Dashboard
#
################################################################################

include ../common.mk

################################################################################
# BSP Feature Flags (conditional compilation per TARGET)
################################################################################
-include ../bsps/TARGET_$(TARGET)/bsp_features.mk

################################################################################
# Basic Configuration
################################################################################

APPNAME=proj_cm55

# Force Release build — prebuilt libtesaiot_bento_cm55.a requires Release config
CONFIG=Release

VERBOSE=

################################################################################
# Advanced Configuration
################################################################################

# Phase 1: FreeRTOS
COMPONENTS+=FREERTOS RTOS_AWARE

# Phase 2: Display (GFXSS)
COMPONENTS+=GFXSS

# Phase 14: WiFi (CYW55513 via SDIO)
# BSP provides: WIFI_INTERFACE_SDIO SM 55500A1 HCI-UART MURATA-2FY
COMPONENTS+=LWIP MBEDTLS
COMPONENTS+=WCM SECURE_SOCKETS

# Phase 14.5: OPTIGA Trust M — set to 1 to enable (requires Trust M hardware)
# WARNING: OPTIGA PAL I2C may conflict with display I2C (FT5406 touch).
#          Disable when testing display/WiFi without Trust M.
ENABLE_OPTIGA ?= 0

ifeq ($(ENABLE_OPTIGA), 1)
COMPONENTS+=OPTIGA_CYHAL
DEFINES+=ENABLE_OPTIGA
# OPTIGA PAL from BENTO kit-pse84-ai (MTB auto-discovers COMPONENT_OPTIGA_CYHAL/)
SEARCH+=$(BENTO_BOARD)
endif

DISABLE_COMPONENTS=

CORE=CM55
CORE_NAME=CM55_0

################################################################################
# Phase 14.5: TESAIoT + OPTIGA Trust M Integration
# Controlled by ENABLE_OPTIGA flag (0=disabled, 1=enabled)
################################################################################

# OPTIGA Trust M library path (always defined for CY_IGNORE)
OPTIGA_LIB_PATH=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/optiga-trust-m/release-v5.3.0

ifeq ($(ENABLE_OPTIGA), 1)
# OPTIGA Trust M core source files (compiled directly for CM55)
SOURCES+=$(OPTIGA_LIB_PATH)/src/crypt/optiga_crypt.c \
         $(OPTIGA_LIB_PATH)/src/util/optiga_util.c \
         $(OPTIGA_LIB_PATH)/src/cmd/optiga_cmd.c \
         $(OPTIGA_LIB_PATH)/src/common/optiga_lib_common.c \
         $(OPTIGA_LIB_PATH)/src/common/optiga_lib_logger.c \
         $(OPTIGA_LIB_PATH)/src/comms/optiga_comms_ifx_i2c.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_config.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_data_link_layer.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_physical_layer.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_presentation_layer.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_transport_layer.c \
         $(OPTIGA_LIB_PATH)/extras/pal/pal_crypt_mbedtls.c

# OPTIGA Trust M include directories
INCLUDES+=$(OPTIGA_LIB_PATH)/include \
          $(OPTIGA_LIB_PATH)/include/common \
          $(OPTIGA_LIB_PATH)/include/pal \
          $(OPTIGA_LIB_PATH)/include/cmd \
          $(OPTIGA_LIB_PATH)/include/comms \
          $(OPTIGA_LIB_PATH)/include/ifx_i2c

# TESAIoT library headers — from BENTO kit-pse84-ai
INCLUDES+=$(BENTO_BOARD)/tesaiot/include

# TESAIoT IP-protected sources (compiled directly for CM55 — no pre-built .a)
SOURCES+=$(BENTO_BOARD)/tesaiot/src/tesaiot_license.c
SOURCES+=$(BENTO_BOARD)/tesaiot/src/tesaiot_crypto.c
SOURCES+=$(BENTO_BOARD)/tesaiot/src/tesaiot_license_data.c

# TESAIoT runtime modules (OPTIGA manager) — headers only
INCLUDES+=$(BENTO_BOARD)/modules/tesaiot

# ifx-mbedtls library include (entropy_poll.h etc.)
INCLUDES+=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/ifx-mbedtls/release-v3.6.400/library

endif  # ENABLE_OPTIGA

# Always ignore optiga internals (even when disabled, prevents MTB auto-discovery errors)
CY_IGNORE+=$(OPTIGA_LIB_PATH)/external/mbedtls
CY_IGNORE+=$(OPTIGA_LIB_PATH)/pal
# AWS SDK coreHTTP not needed (only MQTT); skip missing core_http_config.h
CY_IGNORE+=$(SEARCH_aws-iot-device-sdk-embedded-C)/libraries/standard/coreHTTP

# When OPTIGA is disabled, ignore all OPTIGA-related sources to prevent MTB auto-discovery
ifneq ($(ENABLE_OPTIGA), 1)
CY_IGNORE+=$(OPTIGA_LIB_PATH)
CY_IGNORE+=$(BENTO_BOARD)/modules/tesaiot
CY_IGNORE+=$(BENTO_BOARD)/tesaiot
endif

SOURCES+=
INCLUDES+=

################################################################################
# USB Host HID Joystick (Logitech F310 via SEGGER emUSB-Host)
################################################################################

COMPONENTS+=USBH_BASE
DEFINES+=ENABLE_USB_HOST=1
DEFINES+=USBH_DISABLE_STANDARD_OUTPUT
INCLUDES+=imports/emusb-host/Config
INCLUDES+=imports/emusb-host/Config/COMPONENT_PSE84
INCLUDES+=$(BENTO_COMMON)/modules/usb_hid_joystick

################################################################################
# Radar Presence Detection (BGT60TR13C, SPI, 200 Hz) — AI Kit only
################################################################################

ifeq ($(BSP_HAS_RADAR),1)
INCLUDES+=$(BENTO_BOARD)/libraries/tesaiot-radar
endif

################################################################################
# Phase 2: LVGL Display Module
################################################################################

# LVGL display module includes
INCLUDES+=modules/lvgl_display/core
INCLUDES+=modules/lvgl_display/controller

# Waveshare 4.3" DSI (800x480) + FT5406 touch (direct, no IPC)
DEFINES+=MTB_DISPLAY_W4P3INCH_RPI MTB_CTP_FT5406
DEFINES+=CM55_TOUCH_INPUT_ENABLE=1

# Board-specific I2C display controller mapping
DEFINES+=USE_KIT_PSE84_AI

# Ignore stale lv_port_indev.c in modules/ — proj_cm55/lv_port_indev.c is canonical
CY_IGNORE+=modules/lvgl_display/core/lv_port_indev.c

# Ignore unused display/touch drivers that may be in mtb_shared
CY_IGNORE+=$(SEARCH_display-dsi-waveshare-7-0-lcd-c)
CY_IGNORE+=$(SEARCH_touch-ctp-gt911)
CY_IGNORE+=$(SEARCH_display-tft-ek79007ad3)
CY_IGNORE+=$(SEARCH_touch-ctp-ili2511)

# Ignore unused LVGL middleware files (reduce compile time)
CY_IGNORE+=$(SEARCH_lvgl)/src/others/vg_lite_tvg $(SEARCH_lvgl)/src/libs/thorvg $(SEARCH_lvgl)/tests
CY_IGNORE+=$(SEARCH_lvgl)/draw/nxp $(SEARCH_lvgl)/draw/renesas $(SEARCH_lvgl)/examples
CY_IGNORE+=$(SEARCH_lvgl)/src/draw/vg_lite/lv_draw_vg_lite.c $(SEARCH_lvgl)/src/draw/vg_lite/lv_vg_lite_utils.c
CY_IGNORE+=$(SEARCH_lvgl)/src/draw/vg_lite/lv_draw_vg_lite_img.c $(SEARCH_lvgl)/src/draw/sw/blend/helium
CY_IGNORE+=$(SEARCH_lvgl)/src/draw/sw/blend/neon $(SEARCH_lvgl)/src/core/lv_refr.c
CY_IGNORE+=$(SEARCH_lvgl)/demos
CY_IGNORE+=$(SEARCH_lvgl)/examples

# Ignore CM55 retarget-io driver (CM33_NS owns UART for MicroPython REPL)
CY_IGNORE+=driver
# Keep imported Face runtime sources out of the stable SensorHub image.
CY_IGNORE+=modules/face_id_real
# Temporarily keep ML middleware out until Face runtime is reintegrated.
CY_IGNORE+=$(SEARCH_ml-middleware)
CY_IGNORE+=$(SEARCH_ml-tflite-micro)

################################################################################
# IPC Communication (CM33 ↔ CM55 via Cypress IPC Pipe)
################################################################################

# IPC Pipe infrastructure — prebuilt in libtesaiot_bento_cm55.a
# SOURCES+=$(wildcard $(BENTO_COMMON)/shared/source/COMPONENT_CM55/*.c)
INCLUDES+=$(BENTO_COMMON)/shared/include
# TESAIoT IPC modules — headers only (source compiled into libtesaiot_bento_cm55.a)
INCLUDES+=$(BENTO_COMMON)/modules/ipc_lcd

# proj_cm55 root for lv_port_indev.h (needed by ipc_sensorhub touch IPC handlers)
INCLUDES+=.

INCLUDES+=$(BENTO_COMMON)/modules/ipc_sensorhub
INCLUDES+=$(BENTO_COMMON)/modules/ipc_ui

################################################################################
# Template Mode: User examples in app/ directory
# Drop your main_example.c from TESAIoT Developer Hub into app/
################################################################################
SOURCES+=$(wildcard app/*.c)
INCLUDES+=app

# sensorhub_ui — still needed for page helpers/theme, but demos are excluded
INCLUDES+=modules/sensorhub_ui

INCLUDES+=$(BENTO_COMMON)/modules/wifi_manager
INCLUDES+=$(BENTO_COMMON)/modules/ipc_service

# Template: Ignore full SensorHub UI demo pages (we use example_main instead)
CY_IGNORE+=modules/sensorhub_ui/demo
CY_IGNORE+=modules/sensorhub_ui/page_home.c
CY_IGNORE+=modules/sensorhub_ui/page_dashboard.c
CY_IGNORE+=modules/sensorhub_ui/page_motion.c
CY_IGNORE+=modules/sensorhub_ui/page_environ.c
CY_IGNORE+=modules/sensorhub_ui/page_controls.c
CY_IGNORE+=modules/sensorhub_ui/page_joystick.c
CY_IGNORE+=modules/sensorhub_ui/page_playground.c
CY_IGNORE+=modules/sensorhub_ui/page_examples.c
CY_IGNORE+=modules/sensorhub_ui/page_hsm.c
CY_IGNORE+=modules/sensorhub_ui/sensorhub_ui.c

################################################################################
# Defines
################################################################################

# Basic defines
DEFINES+=CY_RETARGET_IO_CONVERT_LF_TO_CRLF
DEFINES+=CY_RTOS_AWARE

# Phase 14: WiFi + mbedTLS defines
MBEDTLSFLAGS = MBEDTLS_USER_CONFIG_FILE='"mbedtls_user_config.h"'
DEFINES+=$(MBEDTLSFLAGS) CYBSP_WIFI_CAPABLE
DEFINES+=MBEDTLS_CONFIG_FILE='"mbedtls/mbedtls_config.h"'
DEFINES+=CY_WIFI_COUNTRY=WHD_COUNTRY_UNITED_STATES

# Phase 14.5: OPTIGA Trust M + TESAIoT defines (only when enabled)
ifeq ($(ENABLE_OPTIGA), 1)
OPTIGAFLAGS=OPTIGA_LIB_EXTERNAL='"optiga_lib_config_mtb.h"'
DEFINES+=$(OPTIGAFLAGS)
DEFINES+=TESAIOT_DEBUG_LEVEL=3
DEFINES+=CY_CRYPTO_HAL_DISABLE
# PSA crypto config for mbedTLS + OPTIGA SE driver
DEFINES+=MBEDTLS_PSA_CRYPTO_CONFIG_FILE='"configs/ifx_psa_crypto_config.h"'
endif

# MQTT event thread timeout (prevent deadlock)
DEFINES+=CY_MQTT_RECEIVE_DATA_TIMEOUT_MS=100

################################################################################
# BSP Feature Flag Defines (passed to C code as BSP_HAS_XXX=1)
################################################################################
ifeq ($(BSP_HAS_BMI270),1)
DEFINES+=BSP_HAS_BMI270=1
endif
ifeq ($(BSP_HAS_BMM350),1)
DEFINES+=BSP_HAS_BMM350=1
endif
ifeq ($(BSP_HAS_DPS368),1)
DEFINES+=BSP_HAS_DPS368=1
endif
ifeq ($(BSP_HAS_SHT40),1)
DEFINES+=BSP_HAS_SHT40=1
endif
ifeq ($(BSP_HAS_RADAR),1)
DEFINES+=BSP_HAS_RADAR=1
endif
ifeq ($(BSP_HAS_CAPSENSE),1)
DEFINES+=BSP_HAS_CAPSENSE=1
endif
ifeq ($(BSP_HAS_POTENTIOMETER),1)
DEFINES+=BSP_HAS_POTENTIOMETER=1
endif
ifeq ($(BSP_HAS_AUDIO_CODEC),1)
DEFINES+=BSP_HAS_AUDIO_CODEC=1
endif
ifeq ($(BSP_HAS_PDM_MIC),1)
DEFINES+=BSP_HAS_PDM_MIC=1
endif

CFLAGS+=
CXXFLAGS+=
ASFLAGS+=
# Link prebuilt TESAIoT BENTO library (IPC, display, sensors, UI framework)
LDFLAGS+=-Wl,--whole-archive $(CURDIR)/../prebuilt/libtesaiot_bento_cm55.a -Wl,--no-whole-archive
LDLIBS+=
LINKER_SCRIPT=
PREBUILD=
POSTBUILD=

################################################################################
# Paths
################################################################################

CY_APP_PATH=

CY_GETLIBS_SHARED_PATH=../..
CY_GETLIBS_SHARED_NAME=mtb_shared

include $(CY_TOOLS_DIR)/make/start.mk
