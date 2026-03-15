################################################################################
# BSP Feature Flags: APP_KIT_PSE84_EVAL_EPC2 (PSoC Edge E84 Eva Kit)
#
# Included by Makefile.micropython and proj_cm55/Makefile to enable/disable
# BSP-specific sensor drivers and modules.
#
# Usage:
#   -include ../bsps/TARGET_$(TARGET)/bsp_features.mk
################################################################################

# Common sensors (both boards)
BSP_HAS_BMI270=1
BSP_HAS_BMM350=1

# AI Dev Kit sensors (not available on Eva Kit)
BSP_HAS_DPS368=0
BSP_HAS_SHT40=0
BSP_HAS_RADAR=0

# Eva Kit specific peripherals
BSP_HAS_CAPSENSE=1
BSP_HAS_POTENTIOMETER=1
BSP_HAS_AUDIO_CODEC=1

# USB Host (SEGGER emUSB-Host, USB-HS port)
BSP_HAS_USB_HOST=1

# Audio
BSP_HAS_PDM_MIC=1
