/* Bootloader configuration. Flash defaults match an STM32H743 (2 MB flash, 128 KB
 * sectors); RAM is set to 2 MB. Change these together with linker/bootloader.ld for other parts. */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ---- Flash layout ---- */
#define FLASH_BASE_ADDR     0x08000000u
#define BOOT_SIZE           0x00020000u              /* bootloader: sector 0   */
#define APP_SLOT_ADDR       0x08020000u              /* header lives here      */
#define APP_SLOT_SIZE       0x001E0000u              /* header + app           */
#define APP_HEADER_SIZE     0x400u                   /* keeps VTOR 1 KB aligned */
#define FLASH_SECTOR_SIZE  0x00020000u              /* erase granularity      */
#define APP_VECTOR_ADDR     (APP_SLOT_ADDR + APP_HEADER_SIZE)

/* ---- RAM: one contiguous 2 MB region. An application's initial MSP must lie in
 * (RAM_BASE, RAM_END]; the top-of-stack may equal RAM_END (full-descending stack). ---- */
#define RAM_BASE            0x20000000u
#define RAM_SIZE            0x00200000u              /* 2 MB */
#define RAM_END             (RAM_BASE + RAM_SIZE)

/* ---- Boot request: application writes this to .noinit RAM and resets to
 * force the bootloader to stay in update mode. ---- */
#define BOOT_REQUEST_MAGIC  0xB007B007u

/* ---- Image header ---- */
#define IMAGE_MAGIC         0x50414D4Au              /* "JMAP" little endian   */
#define IMAGE_HEADER_VER    1u

#endif
