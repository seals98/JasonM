/* GDMA support layer. Cache coherency (Cortex-M7 D-cache) is handled here, once;
 * the chip-specific part is only gdma_copy_raw(). */
#ifndef DMA_H
#define DMA_H
#include <stdint.h>

/* Memory-to-memory copy through the GDMA, blocking until complete.
 * Returns 0 on success. Buffers should be 32-byte aligned (see cm7.h). */
int dma_copy(void *dst, const void *src, uint32_t len);

/* Chip hook: program a GDMA channel for a mem->mem transfer and wait for completion.
 * The weak default copies with the CPU, so the bootloader works before the GDMA port exists. */
int gdma_copy_raw(void *dst, const void *src, uint32_t len);
#endif
