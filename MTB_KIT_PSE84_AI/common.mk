################################################################################
# \file common.mk
# \version 1.0
#
# \brief
# Settings shared across all projects.
# PSoC Edge MicroPython + AI Integration Project
#
################################################################################

MTB_TYPE=PROJECT

# Target board/hardware (BSP).
TARGET?=KIT_PSE84_AI

# Toolchain
TOOLCHAIN=GCC_ARM

# Build configuration (Debug/Release)
CONFIG=Release

# Config file for postbuild sign and merge operations.
COMBINE_SIGN_JSON?=configs/boot_with_extended_boot.json

################################################################################
# TESAIoT-BENTO Shared Libraries (self-contained in tesaiot-bento/ directory)
# Path relative to each sub-project (proj_cm33_s/, proj_cm33_ns/, proj_cm55/)
################################################################################
BENTO_LIBS_DIR = ../tesaiot-bento
BENTO_COMMON   = $(BENTO_LIBS_DIR)/common
BENTO_BOARD    = $(BENTO_LIBS_DIR)/kit-pse84-ai

include ../common_app.mk
