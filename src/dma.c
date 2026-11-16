#include "dma.h"
#include "cm7.h"

__attribute__((weak)) int gdma_copy_raw(void *dst, const void *src, uint32_t len)
{
    uint8_t *d = dst; const uint8_t *s = src;
    while (len--) *d++ = *s++;
    return 0;
}

int dma_copy(void *dst, const void *src, uint32_t len)
{
    dcache_clean_range(src, len);                 /* DMA must see the CPU's latest writes     */
    dcache_clean_range(dst, len);                 /* no dirty line may be evicted over the result */
    int rc = gdma_copy_raw(dst, src, len);
    dcache_invalidate_range(dst, len);            /* CPU must re-read what DMA wrote          */
    return rc;
}
