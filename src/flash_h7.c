/* STM32H742/743/753 internal flash driver (2 banks x 8 sectors x 128 KB).
 * Programming unit is a 256-bit flash word (32 bytes). */
#include "hal.h"
#include "config.h"
#include "cm7.h"

#define FLASH_R_BASE   0x52002000u
#define BANK2_ADDR     0x08100000u
#define BANK_REG_STEP  0x100u

#define KEYR(b)  REG32(FLASH_R_BASE + 0x04u + (b) * BANK_REG_STEP)
#define CR(b)    REG32(FLASH_R_BASE + 0x0Cu + (b) * BANK_REG_STEP)
#define SR(b)    REG32(FLASH_R_BASE + 0x10u + (b) * BANK_REG_STEP)
#define CCR(b)   REG32(FLASH_R_BASE + 0x14u + (b) * BANK_REG_STEP)

#define CR_LOCK   (1u << 0)
#define CR_PG     (1u << 1)
#define CR_SER    (1u << 2)
#define CR_PSIZE  (3u << 4)          /* x64 parallelism */
#define CR_START  (1u << 7)
#define CR_SNB(n) ((uint32_t)(n) << 8)
#define CR_SNB_M  (7u << 8)

#define SR_BSY    (1u << 0)
#define SR_QW     (1u << 2)
#define SR_EOP    (1u << 16)
/* WRPERR PGSERR STRBERR INCERR OPERR RDPERR RDSERR SNECCERR DBECCERR */
#define SR_ERRORS (0x07EE0000u & ~SR_EOP)
#define CCR_ALL   0x1FFF0000u        /* clear EOP, error and CRC flags */

#define KEY1 0x45670123u
#define KEY2 0xCDEF89ABu

static uint32_t erased_end;          /* slot addresses below this are erased */

static unsigned bank_of(uint32_t addr) { return addr >= BANK2_ADDR; }

static int wait_idle(unsigned b, uint32_t timeout_ms)
{
    uint32_t t0 = millis();
    while (SR(b) & (SR_BSY | SR_QW))
        if (millis() - t0 > timeout_ms) return -1;
    return (SR(b) & SR_ERRORS) ? -1 : 0;
}

static int erase_sector(uint32_t addr)
{
    unsigned b = bank_of(addr);
    uint32_t sn = ((addr - (b ? BANK2_ADDR : FLASH_BASE_ADDR)) / FLASH_SECTOR_SIZE) & 7u;

    if (wait_idle(b, 100) != 0) { CCR(b) = CCR_ALL; if (wait_idle(b, 100) != 0) return -1; }
    CCR(b) = CCR_ALL;
    CR(b) = (CR(b) & ~CR_SNB_M) | CR_SER | CR_PSIZE | CR_SNB(sn);
    CR(b) |= CR_START;
    int rc = wait_idle(b, 5000);
    CR(b) &= ~(CR_SER | CR_SNB_M);
    CCR(b) = CCR_ALL;
    return rc;
}

static int program_word(uint32_t addr, const uint32_t *src)   /* 32 bytes, 32-byte aligned */
{
    unsigned b = bank_of(addr);
    volatile uint32_t *dst = (volatile uint32_t *)addr;

    CCR(b) = CCR_ALL;
    CR(b) |= CR_PG | CR_PSIZE;
    for (int i = 0; i < 8; i++) dst[i] = src[i];
    dsb();
    int rc = wait_idle(b, 100);
    CR(b) &= ~CR_PG;
    CCR(b) = CCR_ALL;
    return rc;
}

void flash_begin(void)
{
    for (unsigned b = 0; b < 2; b++)
        if (CR(b) & CR_LOCK) { KEYR(b) = KEY1; KEYR(b) = KEY2; }
    erased_end = APP_SLOT_ADDR;
}

int flash_program(uint32_t offset, const uint8_t *data, uint32_t len)
{
    uint32_t addr = APP_SLOT_ADDR + offset;
    if (offset + len > APP_SLOT_SIZE || (addr & 31u)) return -1;

    while (addr + len > erased_end) {                 /* erase sectors just before first use */
        if (erase_sector(erased_end) != 0) return -1;
        erased_end += FLASH_SECTOR_SIZE;
    }
    for (uint32_t done = 0; done < len; done += 32) {
        uint32_t w[8] = { ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u };
        uint32_t n = (len - done < 32) ? len - done : 32;
        for (uint32_t i = 0; i < n; i++) ((uint8_t *)w)[i] = data[done + i];
        if (program_word(addr + done, w) != 0) return -1;
    }
    return 0;
}

void flash_end(void)
{
    for (unsigned b = 0; b < 2; b++) CR(b) |= CR_LOCK;
    dcache_invalidate_all();             /* CPU must re-read what was just programmed */
    icache_invalidate_all();
}
