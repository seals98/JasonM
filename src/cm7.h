/* Minimal Cortex-M7 register access (no CMSIS dependency). */
#ifndef CM7_H
#define CM7_H
#include <stdint.h>

#define REG32(a) (*(volatile uint32_t *)(a))

#define SYST_CSR    REG32(0xE000E010u)
#define NVIC_ICER(n) REG32(0xE000E180u + 4u * (n))
#define NVIC_ICPR(n) REG32(0xE000E280u + 4u * (n))
#define SCB_VTOR    REG32(0xE000ED08u)
#define SCB_CCR     REG32(0xE000ED14u)
#define SCB_CPACR   REG32(0xE000ED88u)
#define SCB_ICIALLU REG32(0xE000EF50u)
#define SCB_DCISW   REG32(0xE000EF60u)
#define SCB_DCCSW   REG32(0xE000EF6Cu)
#define SCB_DCCISW  REG32(0xE000EF74u)
#define SCB_CSSELR  REG32(0xE000ED84u)
#define SCB_CCSIDR  REG32(0xE000ED80u)
#define SCB_BPIALL  REG32(0xE000EF78u)

#define SCB_CCR_IC  (1u << 17)
#define SCB_CCR_DC  (1u << 16)

static inline void dsb(void) { __asm volatile("dsb 0xF" ::: "memory"); }
static inline void isb(void) { __asm volatile("isb 0xF" ::: "memory"); }
static inline void irq_disable(void) { __asm volatile("cpsid i" ::: "memory"); }

/* Walk all D-cache sets/ways. op: address of SCB_DCISW / DCCSW / DCCISW. */
static inline void dcache_by_setway(volatile uint32_t *op)
{
    SCB_CSSELR = 0;                       /* level 1 data cache */
    dsb();
    uint32_t ccsidr = SCB_CCSIDR;
    uint32_t sets = (ccsidr >> 13) & 0x7FFFu;
    uint32_t ways = (ccsidr >> 3) & 0x3FFu;
    for (int32_t s = (int32_t)sets; s >= 0; s--)
        for (int32_t w = (int32_t)ways; w >= 0; w--)
            *op = ((uint32_t)s << 5) | ((uint32_t)w << 30);   /* M7: 32-byte lines, 4 ways */
    dsb();
    isb();
}

static inline void icache_enable(void)
{
    if (SCB_CCR & SCB_CCR_IC) return;
    dsb(); isb();
    SCB_ICIALLU = 0; dsb(); isb();
    SCB_CCR |= SCB_CCR_IC; dsb(); isb();
}

static inline void dcache_enable(void)
{
    if (SCB_CCR & SCB_CCR_DC) return;
    dcache_by_setway(&SCB_DCISW);         /* invalidate all */
    SCB_CCR |= SCB_CCR_DC; dsb(); isb();
}

/* Clean+invalidate then disable, so the application starts with coherent memory. */
static inline void caches_disable(void)
{
    if (SCB_CCR & SCB_CCR_DC) {
        SCB_CCR &= ~SCB_CCR_DC; dsb(); isb();
        dcache_by_setway(&SCB_DCCISW);
    }
    if (SCB_CCR & SCB_CCR_IC) {
        SCB_CCR &= ~SCB_CCR_IC; dsb(); isb();
    }
    SCB_ICIALLU = 0; SCB_BPIALL = 0; dsb(); isb();
}

#define SYST_RVR    REG32(0xE000E014u)
#define SYST_CVR    REG32(0xE000E018u)

/* Drop every D-cache line without writing back (bootloader keeps no dirty data in cached RAM). */
static inline void dcache_invalidate_all(void)
{
    if (SCB_CCR & SCB_CCR_DC) dcache_by_setway(&SCB_DCISW);
}
static inline void icache_invalidate_all(void) { SCB_ICIALLU = 0; dsb(); isb(); }
#endif
