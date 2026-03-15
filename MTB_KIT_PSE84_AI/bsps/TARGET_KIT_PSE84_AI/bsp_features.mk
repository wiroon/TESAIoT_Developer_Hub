################################################################################
# BSP Feature Flags: KIT_PSE84_AI (PSoC Edge AI Dev Kit)
#
# Included by Makefile.micropython and proj_cm55/Makefile to enable/disable
# BSP-specific sensor drivers and modules.
#
# Usage:
#   -include ../bsps/TARGET_$(TARGET)/bsp_features.mk
################################################################################

# On-board sensors
BSP_HAS_BMI270=1
BSP_HAS_BMM350=1

# AI Dev Kit specific sensors
BSP_HAS_DPS368=1
BSP_HAS_SHT40=1
BSP_HAS_RADAR=1

# Optional peripherals (requires base board)
BSP_HAS_CAPSENSE=0
BSP_HAS_POTENTIOMETER=0
BSP_HAS_AUDIO_CODEC=0

# USB Host (SEGGER emUSB-Host, USB-HS port)
BSP_HAS_USB_HOST=1

# Audio
BSP_HAS_PDM_MIC=1
