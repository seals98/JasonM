# Cortex-M7 Bootloader

Bare-metal (no CMSIS/HAL) bootloader for a generic **Cortex-M7 SoC**:

| | |
|---|---|
| Core | Cortex-M7, 750 MHz, I/D-cache |
| RAM | 2 MB at `0x20000000` |
| Boot | on-chip **Boot ROM** → this bootloader, executed in place from **QSPI NOR** |
| Peripherals | UART (update console), QSPI, WDT, GDMA, I2C, I3C |

The SoC is not fixed: everything chip specific sits behind `src/hal.h` and is implemented in
`platform/<soc>/`. `platform/template` is the porting skeleton, `platform/sim` is a host model.

## Memory map (`src/config.h`, `linker/bootloader.ld`)
| Region | Address | Size |
|---|---|---|
| Boot ROM image header (FCB/IVT, SoC specific) | `0x60000000` | 4 KB |
| Bootloader (vectors at `0x60001000`) | `0x60000000` | 128 KB total |
| App slot: image header | `0x60020000` | 0x400 |
| App slot: app vector table | `0x60020400` | up to 16 MB − 128 KB |
| RAM | `0x20000000` | 2 MB (bootloader stack at the top) |

## Boot flow
1. Boot ROM starts the bootloader from QSPI (XIP). `board_init()`: 750 MHz clock, watchdog
   (`WDT_TIMEOUT_MS`), UART, QSPI/NOR detection; then I/D-cache on.
2. If the app left `BOOT_REQUEST_MAGIC` in `boot_request` (`.noinit` RAM) → stay in update mode.
3. Validate the slot: header magic/version/size, CRC-32 (watchdog kicked every 4 KB), initial SP in RAM,
   reset vector has Thumb bit and lies inside the image.
4. Valid → quiesce UART, disable IRQs/SysTick, clean+disable caches, set `VTOR`/`MSP`, branch.
   **The watchdog keeps running and QSPI stays in XIP**: the application must keep kicking the WDT and
   must not reconfigure QSPI away from memory-mapped mode while executing from it.
   Invalid → UART update loop.

## Update (UART, XMODEM-CRC, 115200 8N1)
The bootloader prints a banner and sends `C`; send `app_signed.bin` with any XMODEM-CRC client
(128 B or 1 KB blocks). 4 KB sectors are erased just before they are first written, data goes in
256 B page programs, then the image is validated and started.

**Flash while executing from flash:** erase/program must run with the XIP window off, so `nor_*` and the
`hal_qspi_*`/`hal_millis`/`hal_wdt_kick` functions are `RAMFUNC` (placed in `.ramfunc`, copied to RAM at
reset, called with `long_call`). A port's `RAMFUNC` hal functions must not call flash-resident code.

## Building an application
Link the app at `0x60020400` (vector table first), then
```
python3 tools/mkimage.py app.bin app_signed.bin --version 1
```
and put `app_signed.bin` at `0x60020000` (or send it over UART).

## Layout
| Path | Role |
|---|---|
| `src/boot.c`, `crc32.c` | image validation |
| `src/main.c`, `cm7.h`, `startup.c` | boot flow, jump, cache/NVIC handling, vectors |
| `src/xmodem.c` | XMODEM-CRC receiver |
| `src/nor.c`, `slot.c` | JEDEC SPI-NOR driver (RAMFUNC) and app-slot programming |
| `src/dma.c` | `dma_copy()`: GDMA + D-cache clean/invalidate (not used by the boot path yet) |
| `src/board.c` | init sequence and update loop |
| `src/hal.h` | **the porting interface**: clock, WDT, UART, QSPI, I2C, I3C, GDMA, cache |
| `platform/template/` | porting skeleton (see below) |
| `platform/sim/` | host model of the HAL incl. a strict NOR simulator |

## Porting to a SoC
`cp -r platform/template platform/<soc>`, fill every `TODO` (clock/PLL, WDT, UART, QSPI indirect mode,
I2C, I3C, GDMA, Boot ROM header), adjust `config.h`/linker to the real memory map, `make PLATFORM=<soc>`.
I2C/I3C are exposed as `hal_i2c_xfer`/`hal_i3c_xfer` for board setup (PMIC, EEPROM, hub); the boot path
does not use them.

## Build / test
```
make                  # arm-none-eabi-gcc, PLATFORM=template (links; peripherals are inert stubs)
make test             # host tests: validation, XMODEM, NOR driver, slot layer vs. simulated NOR
```

## Status
- Tested on the host: image validation, XMODEM (CRC error, duplicate, overflow, write failure, timeout),
  NOR driver and slot layer against a simulator that enforces write-enable, page wrap, bit-clear-only
  programming and XIP-off discipline. Cross-compiles cleanly (~6.8 KB incl. the 4 KB header pad).
- **Not tested on hardware**, and `platform/template` has no real peripheral code: the UART, QSPI, WDT,
  I2C, I3C, PLL and GDMA bodies are TODO until a concrete SoC is chosen. `src/dma.c` and the cache/jump
  code in `cm7.h`/`main.c` are ARM-only and are covered by compilation only.
