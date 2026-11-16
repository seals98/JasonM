/* Firmware driver for the example jm-i2c controller (qemu/model/jm_i2c.c).
 * It implements the bootloader's hal_i2c_xfer() contract, so a real SoC port looks the same:
 * write tx_len bytes, then (repeated START) read rx_len bytes, then STOP. Returns 0, or -1 on NACK/error. */
#include <stdint.h>
#include "../../src/hal.h"

#define I2C_BASE(bus)  (0x40030000u + (uint32_t)(bus) * 0x1000u)
#define REG(bus, off)  (*(volatile uint32_t *)(I2C_BASE(bus) + (off)))

#define R_CTRL 0x00u
#define R_ADDR 0x04u
#define R_CMD  0x08u
#define R_TX   0x0Cu
#define R_RX   0x10u
#define R_ST   0x14u
#define R_ID   0x18u

#define ST_DONE (1u << 1)
#define ST_NACK (1u << 2)
#define ST_ERR  (1u << 3)

enum { START_W = 1, START_R, WRITE, READ_ACK, READ_NACK, STOP };

/* Issue one command and wait for DONE (poll with a bound: a real controller can hang on a stuck bus). */
static int command(uint8_t bus, uint32_t c)
{
    REG(bus, R_CMD) = c;
    for (uint32_t i = 0; i < 100000u; i++)
        if (REG(bus, R_ST) & ST_DONE) return (REG(bus, R_ST) & (ST_NACK | ST_ERR)) ? -1 : 0;
    return -1;
}

uint32_t jm_i2c_id(uint8_t bus) { return REG(bus, R_ID); }

int hal_i2c_xfer(uint8_t bus, uint8_t addr7, const uint8_t *tx, uint32_t tx_len,
                 uint8_t *rx, uint32_t rx_len)
{
    if (bus != 0) return -1;
    int rc = 0;

    REG(bus, R_CTRL) = 1;                       /* enable */
    REG(bus, R_ST) = ST_NACK | ST_ERR;          /* clear sticky flags */
    REG(bus, R_ADDR) = addr7;

    if (tx_len || !rx_len) {                    /* write phase (also the address-only probe) */
        rc = command(bus, START_W);
        for (uint32_t i = 0; !rc && i < tx_len; i++) {
            REG(bus, R_TX) = tx[i];
            rc = command(bus, WRITE);
        }
    }
    if (!rc && rx_len) {                        /* read phase, preceded by a repeated START */
        rc = command(bus, START_R);
        for (uint32_t i = 0; !rc && i < rx_len; i++) {
            rc = command(bus, i + 1 == rx_len ? READ_NACK : READ_ACK);
            if (!rc) rx[i] = (uint8_t)REG(bus, R_RX);
        }
    }
    (void)command(bus, STOP);                   /* always release the bus */
    REG(bus, R_ST) = ST_NACK | ST_ERR;
    return rc;
}
