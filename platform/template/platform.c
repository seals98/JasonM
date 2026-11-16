/* Porting template: one SoC's implementation of src/hal.h.
 *
 * What is REAL here is SoC independent (Cortex-M7 core: SysTick time base, cache maintenance,
 * a CPU fallback for GDMA). Everything marked TODO needs the SoC's reference manual; until it is
 * filled in the bootloader links and runs the validation/jump path, but the UART, QSPI, WDT, I2C,
 * I3C and clock functions are inert. Copy this directory to platform/<soc>/ and `make PLATFORM=<soc>`. */
#include <stdint.h>
#include "hal.h"
#include "config.h"
#include "cm7.h"

/* Boot ROM image header (FCB/IVT/DCD...). TODO: emit what the SoC's Boot ROM expects; it is
 * placed at the start of QSPI and must fit in BOOT_ROM_HDR_SIZE. */
__attribute__((section(".boot_header"), used))
const uint32_t boot_rom_header[4] = { 0, 0, 0, 0 };

/* ---- clock / time ---- */
void hal_clock_init(void)
{
    /* TODO: enable the PLL(s) for CORE_CLOCK_HZ = 750 MHz: raise core voltage, set flash/QSPI/bus
     * dividers and wait states, switch the core clock source, wait for lock. */
    systick_start_1khz(CORE_CLOCK_HZ);     /* time base derives from the core clock */
}

RAMFUNC uint32_t hal_millis(void)
{
    static uint32_t ms;
    if (SYST_CSR & (1u << 16)) ms++;       /* COUNTFLAG, cleared by this read */
    return ms;
}

/* ---- watchdog ---- */
void hal_wdt_start(uint32_t timeout_ms) { (void)timeout_ms; /* TODO: unlock, set reload, enable */ }
RAMFUNC void hal_wdt_kick(void)         { /* TODO: write the SoC's reload/service key */ }

/* ---- UART ---- */
void hal_uart_init(uint32_t baud)       { (void)baud; /* TODO: clocks, pinmux, divisor, 8N1, enable RX/TX */ }
void hal_uart_deinit(void)              { /* TODO: wait TX idle, disable, pins/clock to reset state */ }
void hal_uart_putc(uint8_t c)           { (void)c;    /* TODO: wait TX ready, write data register */ }
int  hal_uart_getc(uint32_t timeout_ms)
{
    uint32_t t0 = hal_millis();
    for (;;) {
        /* TODO: if RX ready (clear overrun/framing errors) return the received byte. */
        if (hal_millis() - t0 >= timeout_ms) return -1;
    }
}

/* ---- QSPI ---- */
int hal_qspi_init(void) { /* TODO: pinmux/clock for indirect mode; do NOT break the running XIP. */ return 0; }

RAMFUNC int hal_qspi_xfer(uint8_t cmd, uint8_t addr_bytes, uint32_t addr, uint8_t dummy_clks,
                          const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    (void)cmd; (void)addr_bytes; (void)addr; (void)dummy_clks; (void)tx; (void)rx; (void)len;
    /* TODO: program the controller's command/address/dummy/data phases, kick, poll completion, move
     * data via FIFO. Must run from RAM and must not call flash-resident code. */
    return -1;
}

RAMFUNC void hal_qspi_xip_exit(void)  { /* TODO: switch to indirect mode, drain the prefetch buffer */ dsb(); isb(); }
RAMFUNC void hal_qspi_xip_enter(void) { /* TODO: restore memory-mapped read mode, flush AHB buffer */   dsb(); isb(); }

/* ---- I2C / I3C ---- */
int hal_i2c_xfer(uint8_t bus, uint8_t addr7, const uint8_t *tx, uint32_t tx_len, uint8_t *rx, uint32_t rx_len)
{ (void)bus; (void)addr7; (void)tx; (void)tx_len; (void)rx; (void)rx_len; /* TODO */ return -1; }

int hal_i3c_xfer(uint8_t bus, uint8_t dyn_addr, const uint8_t *tx, uint32_t tx_len, uint8_t *rx, uint32_t rx_len)
{ (void)bus; (void)dyn_addr; (void)tx; (void)tx_len; (void)rx; (void)rx_len; /* TODO: ENTDAA, private transfers */ return -1; }

/* ---- GDMA ---- */
int hal_gdma_copy(void *dst, const void *src, uint32_t len)
{
    /* TODO: program a channel (src, dst, len, mem->mem, burst), start, wait for transfer-complete,
     * check the error flag. Until then fall back to the CPU so callers still work. */
    uint8_t *d = dst; const uint8_t *s = src;
    while (len--) *d++ = *s++;
    return 0;
}

/* ---- caches ---- */
void hal_cache_invalidate_all(void)
{
    dcache_invalidate_all();
    icache_invalidate_all();
}
