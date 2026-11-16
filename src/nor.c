#include "nor.h"
#include "config.h"

#define CMD_WREN  0x06
#define CMD_RDSR  0x05
#define CMD_RDID  0x9F
#define CMD_PP    0x02
#define CMD_SE4K  0x20
#define SR_WIP    0x01

#define ERASE_TIMEOUT_MS 1000u
#define PROG_TIMEOUT_MS  100u

/* Everything here runs with XIP off, so it is all RAMFUNC and calls nothing but RAMFUNC hal code. */

RAMFUNC static int wait_ready(uint32_t timeout_ms)
{
    uint32_t t0 = hal_millis();
    uint8_t sr;
    for (;;) {
        if (hal_qspi_xfer(CMD_RDSR, 0, 0, 0, 0, &sr, 1) != 0) return -1;
        if (!(sr & SR_WIP)) return 0;
        hal_wdt_kick();
        if (hal_millis() - t0 > timeout_ms) return -1;
    }
}

RAMFUNC uint32_t nor_read_id(void)
{
    uint8_t id[3] = { 0, 0, 0 };
    hal_qspi_xip_exit();
    int rc = hal_qspi_xfer(CMD_RDID, 0, 0, 0, 0, id, 3);
    hal_qspi_xip_enter();
    return rc ? 0 : ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

RAMFUNC int nor_erase(uint32_t off)
{
    hal_qspi_xip_exit();
    int rc = hal_qspi_xfer(CMD_WREN, 0, 0, 0, 0, 0, 0);
    if (!rc) rc = hal_qspi_xfer(CMD_SE4K, 3, off, 0, 0, 0, 0);
    if (!rc) rc = wait_ready(ERASE_TIMEOUT_MS);
    hal_qspi_xip_enter();
    return rc;
}

RAMFUNC int nor_program(uint32_t off, const uint8_t *d, uint32_t len)
{
    int rc = 0;
    hal_qspi_xip_exit();
    while (len && !rc) {
        uint32_t n = NOR_PAGE_SIZE - (off & (NOR_PAGE_SIZE - 1u));   /* never cross a page */
        if (n > len) n = len;
        rc = hal_qspi_xfer(CMD_WREN, 0, 0, 0, 0, 0, 0);
        if (!rc) rc = hal_qspi_xfer(CMD_PP, 3, off, 0, d, 0, n);
        if (!rc) rc = wait_ready(PROG_TIMEOUT_MS);
        off += n; d += n; len -= n;
    }
    hal_qspi_xip_enter();
    return rc;
}

int nor_init(void)
{
    if (hal_qspi_init() != 0) return -1;
    uint32_t id = nor_read_id();
    return (id == 0 || id == 0xFFFFFFu) ? -1 : 0;
}
