/* Application-slot programming used by the update protocol. Offsets are relative to APP_SLOT_ADDR. */
#ifndef SLOT_H
#define SLOT_H
#include <stdint.h>
void flash_begin(void);                                                   /* forget erase state */
int  flash_program(uint32_t offset, const uint8_t *data, uint32_t len);   /* erases lazily; 0 = ok */
void flash_end(void);                                                     /* make new contents visible to the CPU */
#endif
