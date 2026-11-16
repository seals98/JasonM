/* Host model of the HAL: a JEDEC NOR behind hal_qspi_xfer that enforces real-flash rules
 * (write-enable, program only clears bits, no page wrap, 4 KB aligned erase, busy polling). */
#include <string.h>
#include "hal.h"
#include "config.h"
#include "sim.h"

uint8_t sim_nor[QSPI_SIZE];
unsigned sim_errors, sim_kicks;
int sim_xip = 1;
static int wel, busy;
static uint32_t ms;

void sim_nor_reset(void)
{
    memset(sim_nor, 0xFF, sizeof sim_nor);
    sim_errors = sim_kicks = 0; sim_xip = 1; wel = busy = 0;
}

void hal_clock_init(void) {}
uint32_t hal_millis(void) { return ++ms; }
void hal_wdt_start(uint32_t t) { (void)t; }
void hal_wdt_kick(void) { sim_kicks++; }
void hal_uart_init(uint32_t b) { (void)b; }
void hal_uart_deinit(void) {}
void hal_uart_putc(uint8_t c) { (void)c; }
int  hal_uart_getc(uint32_t t) { (void)t; return -1; }
int  hal_qspi_init(void) { return 0; }
void hal_cache_invalidate_all(void) {}
int  hal_i2c_xfer(uint8_t a, uint8_t b, const uint8_t *c, uint32_t d, uint8_t *e, uint32_t f)
{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; return -1; }
int  hal_i3c_xfer(uint8_t a, uint8_t b, const uint8_t *c, uint32_t d, uint8_t *e, uint32_t f)
{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; return -1; }
int  hal_gdma_copy(void *d, const void *s, uint32_t n) { memcpy(d, s, n); return 0; }
void hal_qspi_xip_exit(void)  { if (!sim_xip) sim_errors++; sim_xip = 0; }
void hal_qspi_xip_enter(void) { if (sim_xip) sim_errors++;  sim_xip = 1; }

int hal_qspi_xfer(uint8_t cmd, uint8_t ab, uint32_t addr, uint8_t dummy,
                  const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    (void)dummy;
    if (sim_xip) { sim_errors++; return -1; }               /* indirect command while mapped */
    switch (cmd) {
    case 0x06: wel = 1; return 0;
    case 0x05:
        *rx = (uint8_t)((busy > 0) | (wel << 1));
        if (busy > 0 && --busy == 0) wel = 0;
        return 0;
    case 0x9F: rx[0] = 0xEF; rx[1] = 0x40; rx[2] = 0x18; return 0;
    case 0x20:
        if (!wel || busy || ab != 3 || (addr & (NOR_ERASE_SIZE - 1u)) || addr >= QSPI_SIZE) { sim_errors++; return -1; }
        memset(sim_nor + addr, 0xFF, NOR_ERASE_SIZE);
        busy = 3; return 0;
    case 0x02:
        if (!wel || busy || ab != 3 || addr + len > QSPI_SIZE) { sim_errors++; return -1; }
        if ((addr & (NOR_PAGE_SIZE - 1u)) + len > NOR_PAGE_SIZE) sim_errors++;   /* would wrap on a real part */
        for (uint32_t i = 0; i < len; i++) {
            if (tx[i] & ~sim_nor[addr + i]) sim_errors++;                       /* 0 -> 1 needs an erase */
            sim_nor[addr + i] &= tx[i];
        }
        busy = 2; return 0;
    }
    sim_errors++;
    return -1;
}
