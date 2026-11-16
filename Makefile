CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

CFLAGS  := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard \
           -Os -g -Wall -Wextra -ffunction-sections -fdata-sections -ffreestanding
LDFLAGS := -T linker/bootloader.ld -nostartfiles -nostdlib -Wl,--gc-sections -Wl,-Map=build/bootloader.map -lgcc

SRCS := src/startup.c src/main.c src/boot.c src/crc32.c src/board.c src/xmodem.c src/flash_h7.c src/board_stm32h7.c src/dma.c
OBJS := $(SRCS:src/%.c=build/%.o)

all: build/bootloader.bin
build/%.o: src/%.c src/*.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/bootloader.elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@
build/bootloader.bin: build/bootloader.elf
	$(OBJCOPY) -O binary $< $@
build: ; mkdir -p build

# Host-side unit test of the validation logic (no ARM toolchain needed).
test:
	mkdir -p build && gcc -Wall -Wextra -Isrc test/test_boot.c src/boot.c src/crc32.c -o build/test_boot && build/test_boot
	gcc -Wall -Wextra -Isrc test/test_xmodem.c src/xmodem.c src/boot.c src/crc32.c -o build/test_xmodem && build/test_xmodem
clean: ; rm -rf build
.PHONY: all test clean
