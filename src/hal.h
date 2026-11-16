/* Hardware abstraction layer. Everything chip specific lives behind these functions;
 * implement them in platform/<name>/. platform/sim is a host-side model used by the tests.
 *
 * RAMFUNC: code that runs while the QSPI XIP window is switched off (flash erase/program) must
 * execute from RAM, so these hal functions - and everything they call - must be RAMFUNC and may
 * not call into flash-resident code (no libc, no helper calls that were not forced into RAM). */
#ifndef HAL_H
#define HAL_H
#include <stdint.h>

#ifdef __arm__
#define RAMFUNC __attribute__((section(".ramfunc"), noinline, long_call))
#else
#define RAMFUNC
#endif

/* --- clock / time --- */
void     hal_clock_init(void);              /* PLL, flash/QSPI timing: reach CORE_CLOCK_HZ */
RAMFUNC uint32_t hal_millis(void);          /* free-running ms counter; advances by polling */

/* --- watchdog --- */
void     hal_wdt_start(uint32_t timeout_ms);
RAMFUNC void hal_wdt_kick(void);

/* --- UART (update console, 8N1) --- */
void     hal_uart_init(uint32_t baud);
void     hal_uart_deinit(void);             /* flush TX, disable, release pins */
void     hal_uart_putc(uint8_t c);
int      hal_uart_getc(uint32_t timeout_ms);   /* byte, or -1 on timeout */

/* --- QSPI controller ---
 * hal_qspi_init: bring the controller up WITHOUT disturbing XIP (the Boot ROM already runs it).
 * hal_qspi_xfer: one indirect-mode command: cmd [+ addr_bytes of addr] [+ dummy clocks] then
 *   tx or rx data (len bytes). Only valid between xip_exit and xip_enter. Returns 0 on success.
 * xip_exit/xip_enter: leave / re-enter memory-mapped read mode (drain prefetch, invalidate its buffers). */
int      hal_qspi_init(void);
RAMFUNC int  hal_qspi_xfer(uint8_t cmd, uint8_t addr_bytes, uint32_t addr, uint8_t dummy_clks,
                           const uint8_t *tx, uint8_t *rx, uint32_t len);
RAMFUNC void hal_qspi_xip_exit(void);
RAMFUNC void hal_qspi_xip_enter(void);

/* --- I2C / I3C master (PMIC, EEPROM, sensors, I3C hub). Not used by the boot path itself. --- */
int      hal_i2c_xfer(uint8_t bus, uint8_t addr7, const uint8_t *tx, uint32_t tx_len,
                      uint8_t *rx, uint32_t rx_len);
int      hal_i3c_xfer(uint8_t bus, uint8_t dyn_addr, const uint8_t *tx, uint32_t tx_len,
                      uint8_t *rx, uint32_t rx_len);

/* --- GDMA --- */
int      hal_gdma_copy(void *dst, const void *src, uint32_t len);   /* mem->mem, blocking, 0 = ok */

/* --- caches --- */
void     hal_cache_invalidate_all(void);    /* after the flash contents changed under the CPU */
#endif
