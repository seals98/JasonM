#include "boot.h"
#include "config.h"
#include "cm7.h"
#include "hal.h"

/* An application sets this to BOOT_REQUEST_MAGIC and resets to stay in the bootloader. */
volatile uint32_t boot_request __attribute__((section(".noinit")));

void boot_jump(uint32_t vt)
{
    uint32_t sp  = ((const uint32_t *)vt)[0];
    uint32_t rst = ((const uint32_t *)vt)[1];

    irq_disable();
    SYST_CSR = 0;                                   /* stop SysTick */
    for (unsigned i = 0; i < 16; i++) {             /* up to 496 external IRQs */
        NVIC_ICER(i) = 0xFFFFFFFFu;
        NVIC_ICPR(i) = 0xFFFFFFFFu;
    }
    caches_disable();
    SCB_VTOR = vt;
    dsb(); isb();

    /* CONTROL is already 0 (MSP, privileged) coming out of reset. */
    __asm volatile("msr msp, %0\n\tbx %1" :: "r"(sp), "r"(rst) : "memory");
    for (;;) {}
}

int main(void)
{
    board_init();
    icache_enable();                                /* faster CRC loop over flash */
    dcache_enable();

    boot_status_t st = BOOT_OK;
    int stay = (boot_request == BOOT_REQUEST_MAGIC);
    boot_request = 0;

    for (;;) {
        hal_wdt_kick();
        if (!stay) {
            st = boot_validate((const uint8_t *)APP_SLOT_ADDR, APP_SLOT_ADDR, APP_SLOT_SIZE);
            if (st == BOOT_OK) {
                board_deinit();
                boot_jump(APP_VECTOR_ADDR);
            }
        }
        board_update_loop(stay ? BOOT_OK : st);     /* returns after a new image is written */
        stay = 0;
        caches_disable();                           /* re-read fresh flash contents */
        icache_enable();
        dcache_enable();
    }
}
