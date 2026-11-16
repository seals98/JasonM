/* Bare-metal test of the jm-i2c model: talks to the AT24C EEPROM at 0x50 and a missing device at 0x51.
 * Prints PASS/FAIL on the UART and exits QEMU through semihosting with the failure count as exit code. */
#include <stdint.h>
#include "../../src/hal.h"

uint32_t jm_i2c_id(uint8_t bus);

#define UART_DATA (*(volatile uint32_t *)0x40004000u)
#define UART_CTRL (*(volatile uint32_t *)0x40004008u)
#define UART_BAUD (*(volatile uint32_t *)0x40004010u)

static void puts_(const char *s) { while (*s) UART_DATA = (uint8_t)*s++; }

static int failures;
static void check(const char *name, int ok)
{
    puts_(ok ? "PASS  " : "FAIL  ");
    puts_(name);
    puts_("\r\n");
    if (!ok) failures++;
}

static void semihost_exit(int code)             /* SYS_EXIT_EXTENDED (0x20); needs  -semihosting */
{
    static volatile uint32_t block[2];
    block[0] = 0x20026;                         /* ADP_Stopped_ApplicationExit */
    block[1] = (uint32_t)code;
    register uint32_t r0 __asm("r0") = 0x20;
    register volatile uint32_t *r1 __asm("r1") = block;
    __asm volatile("bkpt 0xAB" :: "r"(r0), "r"(r1) : "memory");
}

void Reset_Handler(void)
{
    UART_BAUD = 16; UART_CTRL = 1;
    puts_("jm-i2c demo\r\n");

    check("controller ID is JMI2", jm_i2c_id(0) == 0x4A4D4932u);
    check("probe 0x50 (EEPROM) ACKs", hal_i2c_xfer(0, 0x50, 0, 0, 0, 0) == 0);
    check("probe 0x51 (nothing) NACKs", hal_i2c_xfer(0, 0x51, 0, 0, 0, 0) != 0);

    static const uint8_t wr[] = { 0x10, 'J', 'M', '7', '!' };      /* word address, then data */
    check("write 4 bytes @0x10", hal_i2c_xfer(0, 0x50, wr, sizeof wr, 0, 0) == 0);

    uint8_t addr = 0x10, back[4] = { 0 };
    check("read back with repeated START", hal_i2c_xfer(0, 0x50, &addr, 1, back, 4) == 0);
    check("data matches", back[0] == 'J' && back[1] == 'M' && back[2] == '7' && back[3] == '!');

    uint8_t other = 0x20, blank[2] = { 0, 0 };
    check("untouched cells read 0", hal_i2c_xfer(0, 0x50, &other, 1, blank, 2) == 0 && blank[0] == 0 && blank[1] == 0);

    check("read from missing device fails", hal_i2c_xfer(0, 0x51, &addr, 1, back, 4) != 0);
    check("controller still works afterwards", hal_i2c_xfer(0, 0x50, &addr, 1, back, 1) == 0 && back[0] == 'J');

    puts_(failures ? "RESULT: FAIL\r\n" : "RESULT: PASS\r\n");
    semihost_exit(failures);
    for (;;) {}
}

__attribute__((section(".vectors"), used))
void *const vectors[] = { (void *)0x20400000, Reset_Handler };
