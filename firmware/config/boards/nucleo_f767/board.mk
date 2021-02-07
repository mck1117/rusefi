# List of all the board related files.
BOARDSRC = $(CHIBIOS)/os/hal/boards/ST_NUCLEO144_F767ZI/board.c
BOARDCPPSRC = $(PROJECT_DIR)/config/boards/nucleo_f767/board_configuration.cpp

# Required include directories
BOARDINC = $(PROJECT_DIR)/config/boards/nucleo_f767 $(PROJECT_DIR)/config/stm32f7ems
CONFDIR = config/stm32f7ems

# Override DEFAULT_ENGINE_TYPE
DDEFS += -DDEFAULT_ENGINE_TYPE=MINIMAL_PINS -DSTM32F767xx

# Shared variables
ALLCSRC   += $(BOARDSRC)
ALLCPPSRC += $(BOARDCPPSRC)
ALLINC    += $(BOARDINC)
