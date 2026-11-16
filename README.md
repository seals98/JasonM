# JasonM — Cortex-M7 Bootloader

Bare-metal (no CMSIS/HAL) bootloader for Cortex-M7. Defaults target an STM32H7-style
memory map; adjust `src/config.h` and `linker/bootloader.ld` for your part.

## Flash layout
| Region | Address | Size |
|---|---|---|
| Bootloader | `0x08000000` | 128 KB |
| App slot (header + app) | `0x08020000` | 1920 KB |
| └ image header | `0x08020000` | 0x400 |
| └ app vector table | `0x08020400` | — |

## RAM
2 MB contiguous at `0x20000000` (`RAM_BASE`/`RAM_SIZE` in `src/config.h`, `RAM` in the linker script).
The bootloader's `.data/.bss/.noinit` sit at the bottom and its stack starts at the top (`0x20200000`);
an app's initial SP is accepted anywhere in `(0x20000000, 0x20200000]`.

## Boot flow
1. `board_init()`, enable I/D-cache.
2. If the app left `BOOT_REQUEST_MAGIC` in `boot_request` (`.noinit` RAM) → stay in update mode.
3. Validate slot: header magic/version/size, CRC-32 of the app, initial SP in RAM, reset vector
   has Thumb bit and lies inside the image.
4. Valid → `board_deinit()`, disable IRQs/SysTick, clean+disable caches, set `VTOR`, load `MSP`, branch.
   Invalid → `board_update_loop()` (weak hook; implement your transport + flash writer there).

## Building an application
Link the app at `0x08020400` (vector table first), then:
```
python3 tools/mkimage.py app.bin app_signed.bin --version 1
```
and flash `app_signed.bin` at `0x08020000`.

## Build / test
```
make         # needs arm-none-eabi-gcc
make test    # host-side unit tests for validation + CRC
```

## Firmware update (UART, XMODEM-CRC)
If the app is missing/invalid (or a boot request is set), the bootloader prints a banner on
USART3 (PD8/PD9, Nucleo-H743ZI ST-Link VCP, 115200 8N1) and sends `C`. Send `app_signed.bin`
with any XMODEM-CRC client (128 B or 1 KB blocks), e.g. `sz --xmodem`/`lrzsz`, TeraTerm, minicom.
Flash sectors are erased lazily right before they are written; after the transfer the image is
validated and, if good, started immediately.

| File | Role |
|---|---|
| `src/xmodem.c` | Hardware-independent XMODEM-CRC receiver (host-tested) |
| `src/flash_h7.c` | STM32H7 flash driver (sector erase, 256-bit word program) |
| `src/board_stm32h7.c` | Clock-free board init, SysTick time base, USART3, update loop |
| `src/board.c` | Weak defaults — port to another chip by replacing the two STM32H7 files |

Size: ~2.8 KB of the 128 KB bootloader region (`make` prints it; the linker fails if it overflows).

The STM32H7 register-level code (flash, USART, GPIO, RCC) compiles but has **not** been run on
hardware — verify on your board.
