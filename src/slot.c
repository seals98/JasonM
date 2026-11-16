#include "slot.h"
#include "config.h"
#include "nor.h"

#define SLOT_FLASH_OFF  (APP_SLOT_ADDR - QSPI_BASE_ADDR)

static uint32_t erased_end;            /* slot offsets below this are already erased */

void flash_begin(void) { erased_end = 0; }

int flash_program(uint32_t offset, const uint8_t *data, uint32_t len)
{
    if (offset + len > APP_SLOT_SIZE) return -1;
    while (offset + len > erased_end) {                  /* erase sectors just before first use */
        if (nor_erase(SLOT_FLASH_OFF + erased_end) != 0) return -1;
        erased_end += NOR_ERASE_SIZE;
    }
    return nor_program(SLOT_FLASH_OFF + offset, data, len);
}

void flash_end(void) { hal_cache_invalidate_all(); }
