/* Bootloader configuration for a generic Cortex-M7 SoC:
 *   750 MHz core, 2 MB RAM, Boot ROM, QSPI NOR (XIP), UART, WDT, I2C, I3C, GDMA.
 * Adjust the addresses to the SoC's memory map (and linker/bootloader.ld to match). */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ---- Clock ---- */
#define CORE_CLOCK_HZ       750000000u
_Static_assert(CORE_CLOCK_HZ / 1000u - 1u <= 0xFFFFFFu, "SysTick 1 ms reload exceeds 24 bits");

/* ---- RAM: one contiguous 2 MB region. An application's initial MSP must lie in
 * (RAM_BASE, RAM_END]. ---- */
#define RAM_BASE            0x20000000u
#define RAM_SIZE            0x00200000u
#define RAM_END             (RAM_BASE + RAM_SIZE)

/* ---- QSPI NOR flash, memory-mapped (XIP) window. Boot ROM loads/starts the bootloader from
 * the start of this window; the application slot follows it. ---- */
#define QSPI_BASE_ADDR      0x60000000u
#define QSPI_SIZE           0x01000000u              /* 16 MB device */
#define BOOT_ROM_HDR_SIZE   0x00001000u              /* reserved for the Boot ROM's image header */
#define BOOT_SIZE           0x00020000u              /* bootloader region: 128 KB */
#define NOR_ERASE_SIZE      0x1000u                  /* 4 KB sector erase (cmd 0x20) */
#define NOR_PAGE_SIZE       0x100u                   /* 256 B page program */

#define APP_SLOT_ADDR       (QSPI_BASE_ADDR + BOOT_SIZE)       /* header lives here  */
#define APP_SLOT_SIZE       (QSPI_SIZE - BOOT_SIZE)            /* header + app       */
#define APP_HEADER_SIZE     0x400u                             /* keeps VTOR 1 KB aligned */
#define APP_VECTOR_ADDR     (APP_SLOT_ADDR + APP_HEADER_SIZE)

/* ---- Peripherals ---- */
#define UART_BAUD           115200u
#define WDT_TIMEOUT_MS      5000u    /* started by the bootloader; the application must keep kicking it */

/* ---- Boot request: application writes this to .noinit RAM and resets to
 * force the bootloader to stay in update mode. ---- */
#define BOOT_REQUEST_MAGIC  0xB007B007u

/* ---- Image header ---- */
#define IMAGE_MAGIC         0x50414D4Au              /* "JMAP" little endian */
#define IMAGE_HEADER_VER    1u

#endif
