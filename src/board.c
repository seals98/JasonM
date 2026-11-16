/* Chip-independent board flow: init sequence and the UART/XMODEM update loop. */
#include "boot.h"
#include "config.h"
#include "hal.h"
#include "nor.h"
#include "slot.h"
#include "xmodem.h"

static int qspi_ok;

void board_init(void)
{
    hal_clock_init();                       /* 750 MHz */
    hal_wdt_start(WDT_TIMEOUT_MS);
    hal_uart_init(UART_BAUD);
    qspi_ok = (nor_init() == 0);
}

void board_deinit(void)
{
    hal_wdt_kick();                         /* the application inherits a fresh timeout */
    hal_uart_deinit();                      /* QSPI stays in XIP: the application runs from it */
}

static void puts_(const char *s) { while (*s) hal_uart_putc((uint8_t)*s++); }

static void puthex(uint32_t v)
{
    for (int i = 28; i >= 0; i -= 4) hal_uart_putc((uint8_t)"0123456789ABCDEF"[(v >> i) & 0xF]);
}

static const char *status_name(boot_status_t s)
{
    switch (s) {
    case BOOT_OK:               return "ok";
    case BOOT_ERR_MAGIC:        return "no image";
    case BOOT_ERR_HEADER_VER:   return "bad header version";
    case BOOT_ERR_SIZE:         return "bad size";
    case BOOT_ERR_CRC:          return "CRC mismatch";
    case BOOT_ERR_SP:           return "bad initial SP";
    case BOOT_ERR_RESET_VECTOR: return "bad reset vector";
    }
    return "?";
}

static int rx(uint32_t timeout_ms) { hal_wdt_kick(); return hal_uart_getc(timeout_ms); }

void board_update_loop(boot_status_t why)
{
    static const xmodem_io_t io = { rx, hal_uart_putc, flash_program };
    uint8_t buf[XMODEM_BUF_SIZE] __attribute__((aligned(32)));

    puts_("\r\nBootloader: app invalid (");
    puts_(status_name(why));
    puts_("). Send app_signed.bin with XMODEM-CRC, 115200 8N1.\r\n");
    if (!qspi_ok) puts_("WARNING: QSPI flash not detected\r\n");

    for (;;) {
        flash_begin();
        int n = xmodem_receive(&io, buf, APP_SLOT_SIZE);
        flash_end();

        if (n < 0) {
            puts_("\r\nTransfer failed, code -");
            hal_uart_putc((uint8_t)('0' + -n));
            puts_("\r\n");
            continue;
        }
        boot_status_t st = boot_validate((const uint8_t *)APP_SLOT_ADDR, APP_SLOT_ADDR, APP_SLOT_SIZE);
        puts_("\r\nReceived 0x"); puthex((uint32_t)n);
        puts_(" bytes: "); puts_(status_name(st)); puts_("\r\n");
        if (st == BOOT_OK) return;
    }
}
