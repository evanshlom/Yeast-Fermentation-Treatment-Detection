# Makefile — STM32F446RE bare-metal (Nucleo-64)
#
# Build:  make
# Clean:  make clean
# Flash:  copy gas_monitor.bin to Nucleo USB drive, or use ST-Link

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

TARGET  = gas_monitor
SRCS    = startup.c system.c detect.c main.c

ifeq ($(OS),Windows_NT)
    RM_CMD = cmd /C del /f /q
else
    RM_CMD = rm -f
endif

.PHONY: all clean

all: $(TARGET).bin
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(SRCS) stm32f446.h system.h detect.h link.ld
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	$(RM_CMD) $(TARGET).elf $(TARGET).bin