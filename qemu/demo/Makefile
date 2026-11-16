# Build and run the jm-i2c demo.  QEMU must be the patched build (see ../README.md).
CROSS ?= arm-none-eabi-
QEMU  ?= qemu-system-arm

demo.elf: demo.c i2c_jm.c link.ld ../../src/hal.h
	$(CROSS)gcc -mcpu=cortex-m7 -mthumb -Os -Wall -Wextra -nostdlib -ffreestanding -T link.ld demo.c i2c_jm.c -o $@

run: demo.elf
	$(QEMU) -M mps2-an500 -nographic -semihosting -kernel demo.elf

clean: ; rm -f demo.elf
.PHONY: run clean
