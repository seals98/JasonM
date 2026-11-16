CROSS    ?= arm-none-eabi-
PLATFORM ?= template
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
SIZE     := $(CROSS)size

CFLAGS  := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard \
           -Os -g -Wall -Wextra -ffunction-sections -fdata-sections -ffreestanding -Isrc
LDFLAGS := -T linker/bootloader.ld -nostartfiles -nostdlib -Wl,--gc-sections -Wl,-Map=build/bootloader.map -lgcc

SRCS := src/startup.c src/main.c src/boot.c src/crc32.c src/board.c src/xmodem.c \
        src/nor.c src/slot.c src/dma.c platform/$(PLATFORM)/platform.c
OBJS := $(patsubst %.c,build/%.o,$(SRCS))

all: build/bootloader.bin
build/%.o: %.c $(wildcard src/*.h)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
build/bootloader.elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@
build/bootloader.bin: build/bootloader.elf
	$(OBJCOPY) -O binary $< $@

# Host-side tests: validation, XMODEM, NOR driver and slot layer against the simulated QSPI NOR.
HOSTCC := gcc -Wall -Wextra -Isrc -Iplatform/sim
test:
	@mkdir -p build
	$(HOSTCC) test/test_boot.c   src/boot.c src/crc32.c platform/sim/sim.c -o build/test_boot && build/test_boot
	$(HOSTCC) test/test_xmodem.c src/xmodem.c src/boot.c src/crc32.c src/nor.c src/slot.c platform/sim/sim.c -o build/test_xmodem && build/test_xmodem
	$(HOSTCC) test/test_nor.c    src/nor.c src/slot.c platform/sim/sim.c -o build/test_nor && build/test_nor
clean: ; rm -rf build
.PHONY: all test clean
