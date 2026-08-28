# Makefile — STM32F446RE bare-metal (Nucleo-64)
#
# Build (training firmware):  make
# Build (test firmware):      make test
# Clean:                      make clean
# Flash:  copy the .bin to Nucleo USB drive, or use ST-Link

PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

CFLAGS  = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
          -Wall -Wextra -std=c11 -O2 \
          -fno-common -ffunction-sections -fdata-sections

LDFLAGS = -T link.ld \
          --specs=nano.specs --specs=nosys.specs \
          -Wl,--gc-sections \
          -lm -u _printf_float

TARGET       = gas_monitor
SRCS         = startup.c system.c detect.c main.c

TARGET_TEST  = gas_monitor_test
SRCS_TEST    = startup.c system.c detect.c ml_model.c main_test.c

# Use the right delete command depending on OS — avoids depending on
# rm.exe being present on PATH (Git/GnuWin32/WSL), which isn't
# guaranteed on a plain Windows install.
ifeq ($(OS),Windows_NT)
    RM_CMD = del /f /q
else
    RM_CMD = rm -f
endif

.PHONY: all test clean

all: $(TARGET).bin
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(SRCS) stm32f446.h system.h detect.h link.ld
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

test: $(TARGET_TEST).bin
	$(SIZE) $(TARGET_TEST).elf

$(TARGET_TEST).elf: $(SRCS_TEST) stm32f446.h system.h detect.h ml_model.h link.ld
	$(CC) $(CFLAGS) -o $@ $(SRCS_TEST) $(LDFLAGS)

$(TARGET_TEST).bin: $(TARGET_TEST).elf
	$(OBJCOPY) -O binary $< $@

clean:
	$(RM_CMD) $(TARGET).elf $(TARGET).bin $(TARGET_TEST).elf $(TARGET_TEST).bin