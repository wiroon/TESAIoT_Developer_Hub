################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Makefile for configuring the Cortex-M55 core project
#
################################################################################
# \copyright
# (c) 2025, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG.  SPDX-License-Identifier: Apache-2.0
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

include ../common.mk

# Override toolchain for CM55 core to use LLVM Embedded Toolchain for Arm
# TOOLCHAIN=LLVM_ARM
# CY_COMPILER_LLVM_ARM_DIR=/Applications/LLVM-ET-Arm-19.1.5

################################################################################
# Basic Configuration
################################################################################

# Name of application (used to derive name of final linked file).
#
# If APPNAME is edited, ensure to update or regenerate launch
# configurations for your IDE.
APPNAME=proj_cm55

# If set to "true" or "1", display full command-lines when building.
VERBOSE=


################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS+=FREERTOS RTOS_AWARE
COMPONENTS+=GFXSS

# OPTIGA Trust M on CM55 — reads chip data for native HSM dashboard page
# I2C: SCB5 (0x30), shared with touch (0x38). Touch paused during access.
ENABLE_OPTIGA ?= 0

# AI Kit board package has OPTIGA PAL + tesaiot modules (shared across kits)
BENTO_BOARD_AI = $(BENTO_LIBS_DIR)/kit-pse84-ai

ifeq ($(ENABLE_OPTIGA), 1)
DEFINES+=ENABLE_OPTIGA
DEFINES+=MBEDTLS_CONFIG_FILE='"mbedtls/mbedtls_config.h"'
DEFINES+=MBEDTLS_USER_CONFIG_FILE='"mbedtls_user_config.h"'
DEFINES+=OPTIGA_LIB_EXTERNAL='"optiga_lib_config_mtb.h"'
DEFINES+=CY_CRYPTO_HAL_DISABLE
endif

ifeq ($(CONFIG_DISPLAY), WF101JTYAHMNB0_DISP)
DEFINES += MTB_DISPLAY_EK79007AD3 MTB_CTP_ILI2511
# Ignore unused 7-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-dsi-waveshare-7-0-lcd-c)
CY_IGNORE += $(SEARCH_touch-ctp-gt911)
# Ignore unused 4.3-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-dsi-waveshare-4-3-lcd)
CY_IGNORE += $(SEARCH_touch-ctp-ft5406)
else ifeq ($(CONFIG_DISPLAY), WS7P0DSI_RPI_DISP)
DEFINES += MTB_DISPLAY_WS7P0DSI_RPI MTB_CTP_GT911
# Ignore unused 10.1-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-tft-ek79007ad3)
CY_IGNORE += $(SEARCH_touch-ctp-ili2511)
# Ignore unused 4.3-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-dsi-waveshare-4-3-lcd)
CY_IGNORE += $(SEARCH_touch-ctp-ft5406)
else ifeq ($(CONFIG_DISPLAY), W4P3INCH_DISP)
DEFINES += MTB_DISPLAY_W4P3INCH_RPI MTB_CTP_FT5406
# Ignore unused 10.1-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-tft-ek79007ad3)
CY_IGNORE += $(SEARCH_touch-ctp-ili2511)
# Ignore unused 7-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-dsi-waveshare-7-0-lcd-c)
CY_IGNORE += $(SEARCH_touch-ctp-gt911)
endif

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

CORE=CM55
CORE_NAME=CM55_0

################################################################################
# OPTIGA Trust M Library (when ENABLE_OPTIGA=1)
################################################################################
OPTIGA_LIB_PATH=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/optiga-trust-m/release-v5.3.0

ifeq ($(ENABLE_OPTIGA), 1)
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

# PAL (Platform Abstraction Layer) — CYHAL implementation from AI Kit board package
OPTIGA_PAL_PATH=$(BENTO_BOARD_AI)/COMPONENT_OPTIGA_CYHAL
SOURCES+=$(OPTIGA_PAL_PATH)/pal.c \
         $(OPTIGA_PAL_PATH)/pal_gpio.c \
         $(OPTIGA_PAL_PATH)/pal_i2c.c \
         $(OPTIGA_PAL_PATH)/pal_ifx_i2c_config.c \
         $(OPTIGA_PAL_PATH)/pal_logger.c \
         $(OPTIGA_PAL_PATH)/pal_os_datastore.c \
         $(OPTIGA_PAL_PATH)/pal_os_event.c \
         $(OPTIGA_PAL_PATH)/pal_os_lock.c \
         $(OPTIGA_PAL_PATH)/pal_os_memory.c \
         $(OPTIGA_PAL_PATH)/pal_os_timer.c
INCLUDES+=$(OPTIGA_PAL_PATH)

INCLUDES+=$(OPTIGA_LIB_PATH) \
          $(OPTIGA_LIB_PATH)/include \
          $(OPTIGA_LIB_PATH)/include/common \
          $(OPTIGA_LIB_PATH)/include/pal \
          $(OPTIGA_LIB_PATH)/include/cmd \
          $(OPTIGA_LIB_PATH)/include/comms \
          $(OPTIGA_LIB_PATH)/include/ifx_i2c

# tesaiot headers needed by ipc_service.c (guarded by ENABLE_OPTIGA)
INCLUDES+=$(BENTO_BOARD_AI)/tesaiot/include

MBEDTLS_PATH=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/ifx-mbedtls/release-v3.6.400
INCLUDES+=$(MBEDTLS_PATH)/include
INCLUDES+=$(MBEDTLS_PATH)/library

# mbedTLS sources needed by pal_crypt_mbedtls.c (HMAC-SHA256 + AES-CCM)
SOURCES+=$(MBEDTLS_PATH)/library/md.c \
         $(MBEDTLS_PATH)/library/ccm.c \
         $(MBEDTLS_PATH)/library/cipher.c \
         $(MBEDTLS_PATH)/library/cipher_wrap.c \
         $(MBEDTLS_PATH)/library/aes.c \
         $(MBEDTLS_PATH)/library/block_cipher.c \
         $(MBEDTLS_PATH)/library/platform.c \
         $(MBEDTLS_PATH)/library/platform_util.c \
         $(MBEDTLS_PATH)/library/constant_time.c
endif

CY_IGNORE+=$(OPTIGA_LIB_PATH)/external/mbedtls
CY_IGNORE+=$(OPTIGA_LIB_PATH)/pal
# Always ignore AI Kit dirs not explicitly needed (PAL + tesaiot added via SOURCES)
CY_IGNORE+=$(BENTO_BOARD_AI)/application_code
CY_IGNORE+=$(BENTO_BOARD_AI)/libraries
CY_IGNORE+=$(BENTO_BOARD_AI)/COMPONENT_OPTIGA_CYHAL
CY_IGNORE+=$(BENTO_BOARD_AI)/modules/tesaiot
CY_IGNORE+=$(BENTO_BOARD_AI)/tesaiot

ifneq ($(ENABLE_OPTIGA), 1)
CY_IGNORE+=$(OPTIGA_LIB_PATH)
endif

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
# IPC Pipe infrastructure source (CM55 side) — prebuilt in libtesaiot_bento_cm55.a
# SOURCES+=$(wildcard $(BENTO_COMMON)/shared/source/COMPONENT_CM55/*.c)

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
# Shared IPC infrastructure headers — from BENTO shared library
INCLUDES+=$(BENTO_COMMON)/shared/include
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/include
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/DCUser
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/DCUser/rtos
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/DCKernel
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/DCKernel/hardware/8000Nano
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/gcnano/VGLite
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/gcnano/VGLiteKernel
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/gcnano/VGLiteKernel/rtos
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/gcnano/inc

# Add additional defines to the build process (without a leading -D).
DEFINES+=CY_RETARGET_IO_CONVERT_LF_TO_CRLF _BAREMETAL=0

# Check which kit is being used
ifeq (APP_KIT_PSE84_AI, $(TARGET))
DEFINES+=USE_KIT_PSE84_AI
endif

# Ignore unused LVGL middleware files from compilation
CY_IGNORE += $(SEARCH_lvgl)/src/others/vg_lite_tvg $(SEARCH_lvgl)/src/libs/thorvg $(SEARCH_lvgl)/tests
CY_IGNORE += $(SEARCH_lvgl)/draw/nxp $(SEARCH_lvgl)/draw/renesas $(SEARCH_lvgl)/examples
CY_IGNORE += $(SEARCH_lvgl)/src/draw/vg_lite/lv_draw_vg_lite.c $(SEARCH_lvgl)/src/draw/vg_lite/lv_vg_lite_utils.c
CY_IGNORE += $(SEARCH_lvgl)/src/draw/vg_lite/lv_draw_vg_lite_img.c $(SEARCH_lvgl)/src/draw/sw/blend/helium
CY_IGNORE += $(SEARCH_lvgl)/src/draw/sw/blend/neon $(SEARCH_lvgl)/src/core/lv_refr.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_cover_1.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_cover_2.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_cover_3.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_btn_list_pause.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_btn_list_play.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_btn_loop.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_btn_next.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_btn_pause.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_btn_play.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_btn_prev.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_btn_rnd.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_corner_left.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_corner_right.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_icon_1.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_icon_2.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_icon_3.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_icon_4.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_list_border.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_slider_knob.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_wave_bottom.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_wave_top.c


CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_cover_1_large.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_cover_2_large.c
CY_IGNORE += $(SEARCH_lvgl)/demos/music/assets/img_lv_demo_music_cover_3_large.c
CY_IGNORE += $(SEARCH_lvgl)/demos/benchmark/assets/img_benchmark_avatar.c
CY_IGNORE += $(SEARCH_lvgl)/demos/benchmark/assets/img_benchmark_cogwheel_argb.c
CY_IGNORE += $(SEARCH_lvgl)/demos/benchmark/assets/img_benchmark_cogwheel_rgb.c

CY_IGNORE += $(SEARCH_lvgl)/demos/widgets/assets/img_clothes.c
CY_IGNORE += $(SEARCH_lvgl)/demos/widgets/assets/img_demo_widgets_avatar.c
CY_IGNORE += $(SEARCH_lvgl)/demos/widgets/assets/img_lvgl_logo.c

# BiiL legacy IPC — replaced by TESAIoT IPC (client_id=5 collision with ipc_lcd)
CY_IGNORE += ipc

# TESAIoT IPC modules — headers only (source compiled into libtesaiot_bento_cm55.a)
INCLUDES+=$(BENTO_COMMON)/modules/ipc_lcd
INCLUDES+=$(BENTO_COMMON)/modules/wifi_manager
INCLUDES+=$(BENTO_COMMON)/modules/ipc_sensorhub
INCLUDES+=$(BENTO_COMMON)/modules/ipc_service
INCLUDES+=$(BENTO_COMMON)/modules/ipc_ui

# LOCAL modules (Eva Kit specific — no AI-Core/AI-Game equivalent)
INCLUDES+=modules/cm55_sensor_poll

################################################################################
# Template Mode: User examples in app/ directory
# Drop your main_example.c from TESAIoT Developer Hub into app/
################################################################################
SOURCES+=$(wildcard app/*.c)
INCLUDES+=app

# sensorhub_ui — still needed for page helpers/theme, but demos are excluded
INCLUDES+=modules/sensorhub_ui

# Ignore stale lv_port_indev.c in modules/ — proj_cm55/lv_port_indev.c is canonical
CY_IGNORE+=modules/lvgl_display/core/lv_port_indev.c

# proj_cm55 root for lv_port_indev.h (needed by ipc_sensorhub touch IPC handlers)
INCLUDES+=.

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

# Ignore CM55 retarget-io driver (CM33_NS owns UART for MicroPython REPL)
CY_IGNORE+=driver
# Keep imported Face runtime sources out of the stable SensorHub image.
CY_IGNORE+=modules/face_id_real
# Temporarily keep ML middleware out until Face runtime is reintegrated.
CY_IGNORE+=$(SEARCH_ml-middleware)
CY_IGNORE+=$(SEARCH_ml-tflite-micro)

# USB Host HID Joystick (Logitech F310 via SEGGER emUSB-Host)
COMPONENTS+=USBH_BASE
DEFINES+=ENABLE_USB_HOST=1
DEFINES+=USBH_DISABLE_STANDARD_OUTPUT
INCLUDES+=imports/emusb-host/Config
INCLUDES+=imports/emusb-host/Config/COMPONENT_PSE84
INCLUDES+=$(BENTO_COMMON)/modules/usb_hid_joystick

# Eva Kit BSP feature flags (controls conditional tabs in sensorhub_ui)
# Eva Kit hardware: BMI270 + BMM350 + CapSense + Potentiometer
# AI Kit hardware: BMI270 + BMM350 + DPS368 + SHT40 + Radar
DEFINES+=BSP_HAS_BMI270=1
DEFINES+=BSP_HAS_BMM350=1
DEFINES+=BSP_HAS_CAPSENSE=1
DEFINES+=BSP_HAS_POTENTIOMETER=1

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT+=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CFLAGS+=

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS+=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS+=

# Additional / custom linker flags.
# Link prebuilt TESAIoT BENTO library (IPC, display, sensors, UI framework)
LDFLAGS+=-Wl,--whole-archive $(CURDIR)/../prebuilt/libtesaiot_bento_cm55.a -Wl,--no-whole-archive

# Additional / custom libraries to link in to the application.
LDLIBS+=

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=


################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION>
# field begins with $$ASSET_REPO$$, then the repo is deposited in the path
# specified by the CY_GETLIBS_SHARED_PATH variable. The default location is one
# directory level above the current app directory. This is used with
# CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../..

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

include $(CY_TOOLS_DIR)/make/start.mk
