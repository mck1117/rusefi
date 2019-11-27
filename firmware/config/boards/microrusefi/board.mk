# Combine the related files for a specific platform and MCU.

BOARDS_DIR = $(PROJECT_DIR)/config/boards

# Target ECU board design
BOARDSRC_CPP = $(BOARDS_DIR)/microrusefi/board_configuration.cpp

BOARDINC += $(BOARDS_DIR)/microrusefi

# Target processor details
ifeq ($(PROJECT_CPU),f4/stm32f40x)
  BOARDINC += $(PROJECT_DIR)/config/stm32f4ems	# For board.h
else
  BOARDINC += $(PROJECT_DIR)/config/stm32f7ems	# efifeatures/halconf/chconf.h
endif

# Set this if you want a default engine type other than normal MRE
ifeq ($(DEFAULT_ENGINE_TYPE),)
  DEFAULT_ENGINE_TYPE = -DDEFAULT_ENGINE_TYPE=MICRO_RUS_EFI
endif

ifeq ($(EFI_FATAL_ERROR_PIN),)
  EFI_FATAL_ERROR_PIN = -DEFI_FATAL_ERROR_PIN=GPIOE_3
endif


# Add them all together
DDEFS += $(MCU_DEFS) -DEFI_USE_OSC=TRUE -DFIRMWARE_ID=\"microRusEfi\" $(DEFAULT_ENGINE_TYPE)
