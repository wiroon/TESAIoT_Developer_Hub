################################################################################
# \file Makefile
# \version 2.0
#
# \brief
# Makefile for configuring the Cortex-M33 non-secure project
# MicroPython scripting engine + WiFi stack + CM55 boot
#
# Architecture:
#   PREBUILD step builds libmicropython.a using MicroPython's own build system
#   MTB compiles BSP/PDL/HAL/retarget-io + main.c + wifi_init.c
#   Final link: MTB objects + libmicropython.a (--whole-archive)
#
# v2.0: Eva Kit (APP_KIT_PSE84_EVAL_EPC2) port from BiiL base
#        MicroPython runs as a FreeRTOS task
#        WiFi SDIO (SDHC0) accessed directly from CM33_NS (PPC-protected)
#
################################################################################

include ../common.mk

################################################################################
# Basic Configuration
################################################################################

APPNAME=proj_cm33_ns

VERBOSE=

################################################################################
# Advanced Configuration
################################################################################

# FreeRTOS + WiFi stack + BLE
# MicroPython runs as a FreeRTOS task. WiFi (WHD/WCM/lwIP) requires RTOS.
COMPONENTS+=FREERTOS RTOS_AWARE
COMPONENTS+=LWIP MBEDTLS SECURE_SOCKETS
COMPONENTS+=OPTIGA_CYHAL
# WICED_BLE disabled: BLE stack adds ~20KB BSS, m33_data only 256KB
# Re-enable when BLE is needed and RAM is optimized
# COMPONENTS+=WICED_BLE

# Custom mbedTLS config
MBEDTLSFLAGS = MBEDTLS_USER_CONFIG_FILE='"configs/mbedtls_user_config.h"'

# OPTIGA Trust M library config
OPTIGAFLAGS=OPTIGA_LIB_EXTERNAL='"optiga_lib_config_mtb.h"'
OPTIGA_LIB_PATH=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/optiga-trust-m/release-v5.3.0
DISABLE_COMPONENTS=

CORE=CM33
CORE_NAME=CM33_0

# Exclude MicroPython build directories and BiiL backup from MTB auto-discovery.
# MTB must NOT try to compile MicroPython sources - they are built separately.
# Exclude MicroPython build dirs, BiiL backup, and BMI270 library
# (libmicropython.a provides its own BMI270 driver via sensor_bmi270.c)
CY_IGNORE=mpy build-micropython _biil_backup
CY_IGNORE+=$(SEARCH_BMI270_SensorAPI) $(SEARCH_sensor-motion-bmi270)
CY_IGNORE+=$(SEARCH_btstack-integration) $(SEARCH_bt-fw-ifx-cyw55500a1)
# Exclude OPTIGA internal directories (MTB auto-discovers 168+ .c files otherwise)
CY_IGNORE+=$(OPTIGA_LIB_PATH)/external/mbedtls
CY_IGNORE+=$(OPTIGA_LIB_PATH)/extras
CY_IGNORE+=$(OPTIGA_LIB_PATH)/examples
CY_IGNORE+=$(OPTIGA_LIB_PATH)/tests
# Exclude Eva Kit dirs that contain .c compiled via SEARCH/COMPONENT or libmicropython.a
CY_IGNORE+=$(BENTO_LIBS_DIR)/kit-pse84-eval-epc2/modules
CY_IGNORE+=$(BENTO_LIBS_DIR)/kit-pse84-eval-epc2/mpy/eva_controls_auto.c

# MTB-compiled sources:
#   main.c       - FreeRTOS entry point (creates MicroPython task, boots CM55)
#   wifi_init.c  - SDIO + WCM initialization (lazy, on first wifi.scan/connect)
# driver/retarget_io_init.c is auto-discovered by MTB.
SOURCES+=main.c wifi_init.c ntp_sync.c ipc_hsm_handler.c
# IPC shared headers (ipc_communication.h, ipc_response_t, etc.)
INCLUDES+=$(BENTO_COMMON)/shared/include

# OPTIGA Trust M library core sources (13 files)
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
INCLUDES+=$(BENTO_COMMON)/mpy
INCLUDES+=$(BENTO_LIBS_DIR)/kit-pse84-eval-epc2/mpy
# OPTIGA lib config header (optiga_lib_config_mtb.h) — dir is CY_IGNOREd for .c but needed for .h
INCLUDES+=$(BENTO_LIBS_DIR)/kit-pse84-eval-epc2/modules/tesaiot

# OPTIGA Trust M library includes (root needed for "include/optiga_*.h" relative paths)
INCLUDES+=$(OPTIGA_LIB_PATH)
INCLUDES+=$(OPTIGA_LIB_PATH)/include \
          $(OPTIGA_LIB_PATH)/include/common \
          $(OPTIGA_LIB_PATH)/include/pal \
          $(OPTIGA_LIB_PATH)/include/cmd \
          $(OPTIGA_LIB_PATH)/include/comms \
          $(OPTIGA_LIB_PATH)/include/ifx_i2c
# SEARCH path for COMPONENT_OPTIGA_CYHAL PAL files (MTB auto-discovery)
SEARCH+=$(BENTO_LIBS_DIR)/kit-pse84-eval-epc2

# Eval BSP generated sources include cy_graphics.h from the GFXSS package.
# Mirror the secure-core include set here so CM33_NS can compile the same
# generated BSP headers during the MTB phase.
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/include
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/DCUser
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/DCUser/rtos
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/DCKernel
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/dcnano8000/DCKernel/hardware/8000Nano
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/gcnano/VGLite
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/gcnano/VGLiteKernel
INCLUDES+=$(SEARCH_mtb-dsl-pse8xxgp)/pdl/drivers/third_party/COMPONENT_GFXSS/vsi/gcnano/inc

# Defines
DEFINES+=$(MBEDTLSFLAGS) $(OPTIGAFLAGS) CYBSP_WIFI_CAPABLE CY_RTOS_AWARE
# OPTIGA Trust M I2C pin defines (needed for OPTIGA PAL compilation)
DEFINES+=CYBSP_TRUSTM_I2C_SCL=P8_0 CYBSP_TRUSTM_I2C_SDA=P8_1
DEFINES+=MBEDTLS_CONFIG_FILE='"mbedtls/mbedtls_config.h"'
DEFINES+=MBEDTLS_PSA_CRYPTO_CONFIG_FILE='"configs/ifx_psa_crypto_config.h"'
DEFINES+=CY_RETARGET_IO_CONVERT_LF_TO_CRLF
# BT feature baud rate (3M for faster firmware download)
DEFINES+=CYBSP_BT_PLATFORM_CFG_BAUD_FEATURE=3000000

# Board-specific defines for conditional compilation
DEFINES+=USE_KIT_PSE84_EVAL_EPC2
# MQTT event thread: 3KB (default 8KB) — saves 5KB FreeRTOS heap
DEFINES+=CY_MQTT_EVENT_THREAD_STACK_SIZE=3072
# Suppress WHD boot banners (chip ID, WLAN MAC/FW/CLM, WHD VERSION)
DEFINES+=WHD_PRINT_DISABLE

VFP_SELECT+=
CFLAGS+=
CXXFLAGS+=
ASFLAGS+=

# Link libmicropython.a with --whole-archive to ensure all MicroPython
# modules are included (they use registration macros, not direct references).
# Template Mode: use prebuilt libmicropython.a from prebuilt/ directory
LDFLAGS+=-Wl,--whole-archive $(CURDIR)/../prebuilt/libmicropython.a -Wl,--no-whole-archive
LDLIBS+=

LINKER_SCRIPT=

# Template Mode: libmicropython.a is prebuilt (in ../prebuilt/).
# No PREBUILD step needed — MicroPython modules are already compiled.
# To rebuild libmicropython.a, use the full project at KIT_PSE84_EVAL_EPC2-MicroPython-AI-Core/
PREBUILD=

POSTBUILD=

################################################################################
# Paths
################################################################################

CY_APP_PATH=

CY_GETLIBS_SHARED_PATH=../..
CY_GETLIBS_SHARED_NAME=mtb_shared

include $(CY_TOOLS_DIR)/make/start.mk
