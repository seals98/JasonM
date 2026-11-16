/* Board support for STM32H743 (default pins = Nucleo-H743ZI: USART3 on PD8/PD9, ST-Link VCP).
 * Runs on the reset clock (HSI 64 MHz). Overrides the weak hooks in board.c. */
#include "boot.h"
#include "config.h"
#include "hal.h"
#include "xmodem.h"
#include "cm7.h"

#define CORE_HZ       64000000u
#define BAUD          115200u

#define RCC_AHB4ENR   REG32(0x58024400u + 0xE0u)
#define RCC_APB1LENR  REG32(0x58024400u + 0xE8u)
#define GPIOD_MODER   REG32(0x58020C00u + 0x00u)
#define GPIOD_AFRH    REG32(0x58020C00u + 0x24u)
#define U3(off)       REG32(0x40004800u + (off))
#define U3_CR1 0x00u
#define U3_BRR 0x0Cu
#define U3_ISR 0x1Cu
#define U3_ICR 0x20u
#define U3_RDR 0x24u
#define U3_TDR 0x28u

/* ---- time base: SysTick free-running at 1 kHz, polled (no interrupt) ---- */
uint32_t millis(void)
{
    static uint32_t ms;
    if (SYST_CSR & (1u << 16)) ms++;      /* COUNTFLAG, cleared by this read */
    return ms;
}

/* ---- UART ---- */
void uart_putc(uint8_t c)
{
    while (!(U3(U3_ISR) & (1u << 7))) {}  /* TXE */
    U3(U3_TDR) = c;
}

int uart_getc(uint32_t timeout_ms)
{
    uint32_t t0 = millis();
    for (;;) {
        uint32_t isr = U3(U3_ISR);
        if (isr & 0xFu) U3(U3_ICR) = 0xFu; /* clear PE/FE/NE/ORE so reception continues */
        if (isr & (1u << 5)) return (int)(U3(U3_RDR) & 0xFFu);
        if (millis() - t0 >= timeout_ms) return -1;
    }
}

void uart_puts(const char *s) { while (*s) uart_putc((uint8_t)*s++); }

static void uart_puthex(uint32_t v)
{
    for (int i = 28; i >= 0; i -= 4) uart_putc("0123456789ABCDEF"[(v >> i) & 0xF]);
}

void board_init(void)
{
    systick_start_1khz(CORE_HZ);

    RCC_AHB4ENR  |= (1u << 3);            /* GPIOD */
    RCC_APB1LENR |= (1u << 18);           /* USART3 */
    (void)RCC_APB1LENR;                   /* read back: let the clock settle */
    GPIOD_MODER = (GPIOD_MODER & ~(0xFu << 16)) | (0xAu << 16);   /* PD8/PD9 alternate fn */
    GPIOD_AFRH  = (GPIOD_AFRH & ~0xFFu) | 0x77u;                  /* AF7 */

    U3(U3_CR1) = 0;
    U3(U3_BRR) = CORE_HZ / BAUD;
    U3(U3_CR1) = (1u << 0) | (1u << 2) | (1u << 3);               /* UE | RE | TE */
}

void board_deinit(void)
{
    while (!(U3(U3_ISR) & (1u << 6))) {}  /* TC: flush last byte */
    U3(U3_CR1) = 0;
    RCC_APB1LENR &= ~(1u << 18);
    GPIOD_MODER |= (0xFu << 16);          /* back to analog (reset state) */
    GPIOD_AFRH  &= ~0xFFu;
    RCC_AHB4ENR &= ~(1u << 3);
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

static int slot_write(uint32_t off, const uint8_t *d, uint32_t n) { return flash_program(off, d, n); }

void board_update_loop(boot_status_t why)
{
    static const xmodem_io_t io = { uart_getc, uart_putc, slot_write };
    uint8_t buf[XMODEM_BUF_SIZE] __attribute__((aligned(4)));

    uart_puts("\r\nBootloader: app invalid (");
    uart_puts(status_name(why));
    uart_puts("). Send app_signed.bin with XMODEM-CRC, 115200 8N1.\r\n");

    for (;;) {
        flash_begin();
        int n = xmodem_receive(&io, buf, APP_SLOT_SIZE);
        flash_end();

        if (n < 0) {
            uart_puts("\r\nTransfer failed, code -");
            uart_putc((uint8_t)('0' + -n));
            uart_puts("\r\n");
            continue;
        }
        boot_status_t st = boot_validate((const uint8_t *)APP_SLOT_ADDR, APP_SLOT_ADDR, APP_SLOT_SIZE);
        uart_puts("\r\nReceived 0x"); uart_puthex((uint32_t)n);
        uart_puts(" bytes: "); uart_puts(status_name(st)); uart_puts("\r\n");
        if (st == BOOT_OK) return;
    }
}
