/* GDMA with Cortex-M7 D-cache coherency handled once, here. */
#ifndef DMA_H
#define DMA_H
#include <stdint.h>
/* Blocking mem->mem copy through the GDMA. Buffers should be 32-byte aligned (see cm7.h). */
int dma_copy(void *dst, const void *src, uint32_t len);
#endif
