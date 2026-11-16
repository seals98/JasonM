#include "boot.h"
#include "config.h"
#include "crc32.h"
#include "hal.h"

/* CRC over a possibly multi-megabyte image: keep the watchdog fed. */
static uint32_t crc32_wdt(const uint8_t *p, uint32_t n)
{
    uint32_t c = 0;
    while (n) {
        uint32_t k = n > 4096u ? 4096u : n;
        c = crc32_update(c, p, k);
        hal_wdt_kick();
        p += k; n -= k;
    }
    return c;
}

static int in_ram(uint32_t sp)
{
    return sp > RAM_BASE && sp <= RAM_END;      /* top-of-stack may equal RAM_END */
}

boot_status_t boot_validate(const uint8_t *mem, uint32_t addr, uint32_t size)
{
    const image_header_t *h = (const image_header_t *)mem;

    if (h->magic != IMAGE_MAGIC)           return BOOT_ERR_MAGIC;
    if (h->header_ver != IMAGE_HEADER_VER) return BOOT_ERR_HEADER_VER;
    if (h->image_size < 8u || h->image_size > size - APP_HEADER_SIZE)
                                           return BOOT_ERR_SIZE;
    if (crc32_wdt(mem + APP_HEADER_SIZE, h->image_size) != h->crc32)
                                           return BOOT_ERR_CRC;

    const uint32_t *vt = (const uint32_t *)(mem + APP_HEADER_SIZE);
    uint32_t sp = vt[0], rst = vt[1];
    uint32_t app_lo = addr + APP_HEADER_SIZE, app_hi = addr + APP_HEADER_SIZE + h->image_size;

    if (!in_ram(sp) || (sp & 7u))          return BOOT_ERR_SP;
    if (!(rst & 1u) || (rst & ~1u) < app_lo || (rst & ~1u) >= app_hi)
                                           return BOOT_ERR_RESET_VECTOR;
    return BOOT_OK;
}
