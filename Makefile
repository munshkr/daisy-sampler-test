# Project Name
TARGET = sampler_test

# Sources
CPP_SOURCES = src/main.cpp

# Library Locations
LIBDAISY_DIR = ./libs/libDaisy

# Use C++17 standard
CPP_STANDARD = -std=gnu++17

ifeq ($(LOGGER), 1)
# print something
$(info ** Logger enabled **)
# Enable logger
C_DEFS += -DLOGGER
# Enable floating point printf
LDFLAGS += -u _printf_float
endif

# To change custom bootloader
#APP_TYPE = BOOT_QSPI

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

.PHONY: monitor

monitor:
	screen -R sampler-test $(shell ls /dev/tty.usbmodem*) 115200
