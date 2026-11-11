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

## Not included
Clock setup, update transport (UART/USB/…) and flash programming are board-specific:
override the weak `board_*` functions in `src/board.c`.
