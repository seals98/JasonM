#include "dma.h"
#include "hal.h"
#include "cm7.h"

int dma_copy(void *dst, const void *src, uint32_t len)
{
    dcache_clean_range(src, len);                 /* DMA must see the CPU's latest writes       */
    dcache_clean_range(dst, len);                 /* no dirty line may be evicted over the result */
    int rc = hal_gdma_copy(dst, src, len);
    dcache_invalidate_range(dst, len);            /* CPU must re-read what DMA wrote            */
    return rc;
}
