include ../../py/mkenv.mk

CROSS = 1

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# include py core make definitions
include $(TOP)/py/py.mk

ifeq ($(CROSS), 1)
CROSS_COMPILE ?= arm-none-eabi-
endif

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)

ifeq ($(CROSS), 1)
DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
CFLAGS_PSP = -DIN_PSP -mthumb -msoft-float -fsingle-precision-constant -Wdouble-promotion  -march=armv7ve
CFLAGS = $(INC) -Wall -std=c99 -nostdlib $(CFLAGS_PSP) $(COPT)
LDFLAGS = -nostdlib -T amd-psp.ld
SRC_BOARD_O = lib/utils/gchelper_a7.o
else
LD = gcc
CFLAGS = -m32 $(INC) -Wall -Werror -std=c99 $(COPT)
LDFLAGS = -m32 -Wl,-Map=$@.map,--cref -Wl,--gc-sections
endif

CSUPEROPT = -Os # save some code space

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -O2 -g
CFLAGS += -fdata-sections -ffunction-sections
endif

LIBS =

SRC_C = \
	main.c \
	lib/utils/printf.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c

OBJ_COMMON =
OBJ_COMMON += $(addprefix $(BUILD)/, $(SRC_BOARD_O))

OBJ = $(OBJ_COMMON) $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

ifeq ($(CROSS), 1)
all: $(BUILD)/firmware.bin
else
all: $(BUILD)/firmware.elf
endif

$(BUILD)/_cm-start.o: _cm-start.S $(BUILD)/genhdr/qstrdefs.generated.h
	$(CROSS_COMPILE)as -march=armv7ve -o $@ _cm-start.S

$(BUILD)/firmware.elf: $(BUILD)/_cm-start.o $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -g -Map=$(BUILD)/firmware.map -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) -O binary $^ $(BUILD)/firmware.bin

include $(TOP)/py/mkrules.mk
