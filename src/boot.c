#include "boot.h"
#include "config.h"
#include "crc32.h"

static int in_ram(uint32_t sp)
{
    static const struct { uint32_t lo, hi; } r[] = { RAM_REGIONS_INIT };
    for (unsigned i = 0; i < sizeof r / sizeof r[0]; i++)
        if (sp > r[i].lo && sp <= r[i].hi)   /* top-of-stack may equal the end */
            return 1;
    return 0;
}

boot_status_t boot_validate(const uint8_t *mem, uint32_t addr, uint32_t size)
{
    const image_header_t *h = (const image_header_t *)mem;

    if (h->magic != IMAGE_MAGIC)           return BOOT_ERR_MAGIC;
    if (h->header_ver != IMAGE_HEADER_VER) return BOOT_ERR_HEADER_VER;
    if (h->image_size < 8u || h->image_size > size - APP_HEADER_SIZE)
                                           return BOOT_ERR_SIZE;
    if (crc32(mem + APP_HEADER_SIZE, h->image_size) != h->crc32)
                                           return BOOT_ERR_CRC;

    const uint32_t *vt = (const uint32_t *)(mem + APP_HEADER_SIZE);
    uint32_t sp = vt[0], rst = vt[1];
    uint32_t app_lo = addr + APP_HEADER_SIZE, app_hi = addr + APP_HEADER_SIZE + h->image_size;

    if (!in_ram(sp) || (sp & 7u))          return BOOT_ERR_SP;
    if (!(rst & 1u) || (rst & ~1u) < app_lo || (rst & ~1u) >= app_hi)
                                           return BOOT_ERR_RESET_VECTOR;
    return BOOT_OK;
}
