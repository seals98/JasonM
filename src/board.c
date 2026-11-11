/* Weak board hooks. Override these in your board support code
 * (clock tree, UART/USB/QSPI, status LED, flash driver, ...). */
#include "boot.h"

__attribute__((weak)) void board_init(void) {}
__attribute__((weak)) void board_deinit(void) {}

/* Default: nothing to update with, so just halt. A real board should receive
 * an image (UART/USB/Ethernet/SD), program it at APP_SLOT_ADDR and return. */
__attribute__((weak)) void board_update_loop(boot_status_t why)
{
    (void)why;
    for (;;) { __asm volatile("wfi"); }
}
