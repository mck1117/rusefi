
ifeq ("$(wildcard $(RULESFILE))","")
$(info Invoking "git submodule update --init")
$(shell git submodule update --init)
$(info Invoked "git submodule update --init")
# make is not happy about newly checked out module for some reason but next invocation would work
$(error Please run 'make' again. Please make sure you have 'git' command in PATH)
endif

ifeq ($(PROJECT_BOARD),)
  PROJECT_BOARD = st_stm32f4
endif

ifeq ($(PROJECT_CPU),)
  PROJECT_CPU = f4/stm32f40x
endif

-include $(PROJECT_DIR)/config/boards/$(PROJECT_BOARD)/config.mk
include $(PROJECT_DIR)/config/mcu/$(PROJECT_CPU)/cpu.mk
include $(PROJECT_DIR)/config/mcu/$(PROJECT_CPU)/../base.mk

BOARDSRC = $(PROJECT_DIR)/config/mcu/$(PROJECT_CPU)/board.c
BOARDINC = $(PROJECT_DIR)/config/mcu/$(PROJECT_CPU)/

LDSCRIPT= $(PROJECT_DIR)/config/mcu/$(PROJECT_CPU)/link.ld




CPU_STARTUP_DIR = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/$(CPU_STARTUP)

ifeq ($(CPU_PLATFORM_DIR),)
CPU_PLATFORM_DIR = $(CHIBIOS)/os/hal/ports/STM32/$(CPU_PLATFORM)
endif

ifeq ($(GENERATED_ENUMS_DIR),)
GENERATED_ENUMS_DIR = $(PROJECT_DIR)/controllers/algo
endif
