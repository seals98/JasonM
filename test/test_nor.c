#include <stdio.h>
#include <string.h>
#include "nor.h"
#include "slot.h"
#include "config.h"
#include "sim.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    sim_nor_reset();
    CHECK(nor_init() == 0);
    CHECK(nor_read_id() == 0xEF4018u);

    /* program a buffer that starts mid-page and spans two sectors */
    static uint8_t data[6000];
    for (unsigned i = 0; i < sizeof data; i++) data[i] = (uint8_t)(i * 7 + 1);
    CHECK(nor_erase(0x10000) == 0 && nor_erase(0x11000) == 0);
    CHECK(nor_program(0x10000 + 0xF0, data, sizeof data) == 0);
    CHECK(memcmp(sim_nor + 0x10000 + 0xF0, data, sizeof data) == 0);
    CHECK(sim_nor[0x10000] == 0xFF && sim_nor[0x10000 + 0xEF] == 0xFF);   /* neighbours untouched */
    CHECK(sim_errors == 0 && sim_xip == 1);

    /* erase clears again; programming without erase is flagged by the model */
    CHECK(nor_erase(0x10000) == 0);
    CHECK(sim_nor[0x10000 + 0xF0] == 0xFF);
    CHECK(nor_program(0x11000, data, 16) == 0);
    unsigned before = sim_errors;
    nor_program(0x11000, data + 1, 16);
    CHECK(sim_errors > before);

    /* slot layer: lazy erase just ahead of the write pointer, bounds checked */
    sim_nor_reset();
    flash_begin();
    CHECK(flash_program(0, data, 1024) == 0);
    CHECK(flash_program(1024, data, 5000) == 0);
    CHECK(memcmp(sim_nor + (APP_SLOT_ADDR - QSPI_BASE_ADDR) + 1024, data, 5000) == 0);
    CHECK(sim_nor[APP_SLOT_ADDR - QSPI_BASE_ADDR + 8192] == 0xFF);        /* not yet erased/written region intact */
    CHECK(flash_program(APP_SLOT_SIZE - 16, data, 32) != 0);
    CHECK(sim_errors == 0);

    puts(fails ? "FAILED" : "nor tests passed");
    return fails != 0;
}
