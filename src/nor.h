/* JEDEC SPI NOR driver (cmds 0x06/0x05/0x9F/0x20/0x02) on top of hal_qspi_xfer.
 * All offsets are relative to the start of the flash (QSPI_BASE_ADDR). */
#ifndef NOR_H
#define NOR_H
#include <stdint.h>
#include "hal.h"

int  nor_init(void);                          /* 0 = flash answered with a plausible JEDEC ID */
RAMFUNC uint32_t nor_read_id(void);           /* manufacturer<<16 | type<<8 | capacity */
RAMFUNC int nor_erase(uint32_t off);          /* one NOR_ERASE_SIZE sector; off must be aligned */
RAMFUNC int nor_program(uint32_t off, const uint8_t *data, uint32_t len);   /* range must be erased */
#endif
