# Project Name
TARGET = sampler_test

# Sources
# NOTE: could use wildcard here
CPP_SOURCES = \
	src/main.cpp \
	src/fatfs_utils.cpp \
	src/request_manager.cpp \
	src/sample_reader.cpp


# Library Locations
LIBDAISY_DIR = ./libs/libDaisy
DAISYSP_DIR = ./libs/DaisySP

# Use C++17 standard
CPP_STANDARD = -std=gnu++17

ifeq ($(LOGGER), 1)
# print something
$(info ** Logger enabled **)
# Enable logger
C_DEFS += -DLOGGER
# Enable floating point printf
# LDFLAGS += -u _printf_float
endif

CMSIS_DSP_SRC_DIR = $(LIBDAISY_DIR)/Drivers/CMSIS-DSP/Source
C_SOURCES = \
	$(CMSIS_DSP_SRC_DIR)/BasicMathFunctions/arm_scale_f32.c \
	$(CMSIS_DSP_SRC_DIR)/SupportFunctions/arm_fill_f32.c

# Includes FatFS source files within project.
USE_FATFS = 1

# To change custom bootloader
APP_TYPE = BOOT_SRAM
LDSCRIPT = alt_sram.lds

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

CFLAGS += -funroll-loops

.PHONY: monitor

monitor:
	screen -R sampler-test $(shell ls /dev/tty.usbmodem*) 115200
