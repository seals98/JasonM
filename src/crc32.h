#ifndef CRC32_H
#define CRC32_H
#include <stdint.h>
#include <stddef.h>
/* Standard CRC-32 (IEEE 802.3, zlib/Python zlib.crc32 compatible). */
uint32_t crc32_update(uint32_t crc, const void *data, size_t len);
static inline uint32_t crc32(const void *data, size_t len) { return crc32_update(0, data, len); }
#endif
