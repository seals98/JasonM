/* Bootloader configuration. Defaults match an STM32H743 (2 MB flash, 128 KB
 * sectors). Change these together with linker/bootloader.ld for other parts. */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ---- Flash layout ---- */
#define FLASH_BASE_ADDR     0x08000000u
#define BOOT_SIZE           0x00020000u              /* bootloader: sector 0   */
#define APP_SLOT_ADDR       0x08020000u              /* header lives here      */
#define APP_SLOT_SIZE       0x001E0000u              /* header + app           */
#define APP_HEADER_SIZE     0x400u                   /* keeps VTOR 1 KB aligned */
#define APP_VECTOR_ADDR     (APP_SLOT_ADDR + APP_HEADER_SIZE)

/* ---- RAM regions an application's initial MSP may point into (end address
 * inclusive of the "one past the top" value a full-descending stack uses). ---- */
#define RAM_REGIONS_INIT                                                       \
    { 0x20000000u, 0x20020000u }, /* DTCM    128 KB */                         \
    { 0x24000000u, 0x24080000u }, /* AXI     512 KB */                         \
    { 0x30000000u, 0x30048000u }, /* SRAM1-3 288 KB */

/* ---- Boot request: application writes this to .noinit RAM and resets to
 * force the bootloader to stay in update mode. ---- */
#define BOOT_REQUEST_MAGIC  0xB007B007u

/* ---- Image header ---- */
#define IMAGE_MAGIC         0x50414D4Au              /* "JMAP" little endian   */
#define IMAGE_HEADER_VER    1u

#endif
