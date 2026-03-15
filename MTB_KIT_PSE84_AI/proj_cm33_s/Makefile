################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Makefile for configuring the Cortex-M33 core secure project
#
################################################################################

include ../common.mk

################################################################################
# Basic Configuration
################################################################################

APPNAME=proj_cm33_s

VERBOSE=

################################################################################
# Advanced Configuration
################################################################################

COMPONENTS+=SECURE_DEVICE

DISABLE_COMPONENTS+=

# Set this application to run at the CM33
CORE=CM33
CORE_NAME=CM33_0

# Set this application to be secure
VCORE_ATTRS=SECURE

APPTYPE=flash

SOURCES=
INCLUDES=
VFP_SELECT+=
CFLAGS+=
CXXFLAGS+=
ASFLAGS+=

LDFLAGS+=

ifeq ($(TOOLCHAIN),ARM)
 LDFLAGS+=--diag_suppress=L6848
endif

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
