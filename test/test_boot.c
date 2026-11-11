#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "boot.h"
#include "config.h"
#include "crc32.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static uint8_t slot[APP_SLOT_SIZE];

static void make(uint32_t sp, uint32_t rst, uint32_t size)
{
    memset(slot, 0xFF, sizeof slot);
    uint32_t *vt = (uint32_t *)(slot + APP_HEADER_SIZE);
    vt[0] = sp; vt[1] = rst;
    for (uint32_t i = 2; i < size / 4; i++) vt[i] = i;
    image_header_t *h = (image_header_t *)slot;
    memset(h, 0, sizeof *h);
    h->magic = IMAGE_MAGIC; h->header_ver = IMAGE_HEADER_VER; h->image_size = size;
    h->crc32 = crc32(slot + APP_HEADER_SIZE, size);
}

int main(void)
{
    CHECK(crc32("123456789", 9) == 0xCBF43926u);

    make(0x20020000u, APP_VECTOR_ADDR + 0x101u, 0x1000);
    CHECK(boot_validate(slot, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_OK);

    slot[APP_HEADER_SIZE + 0x200] ^= 1;
    CHECK(boot_validate(slot, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_ERR_CRC);

    make(0x20020000u, APP_VECTOR_ADDR + 0x101u, 0x1000);
    ((image_header_t *)slot)->magic = 0;
    CHECK(boot_validate(slot, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_ERR_MAGIC);

    make(0x20020000u, APP_VECTOR_ADDR + 0x101u, 0x1000);
    ((image_header_t *)slot)->image_size = APP_SLOT_SIZE;
    CHECK(boot_validate(slot, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_ERR_SIZE);

    make(0x08000000u, APP_VECTOR_ADDR + 0x101u, 0x1000);       /* SP in flash */
    CHECK(boot_validate(slot, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_ERR_SP);
    make(0x20020004u, APP_VECTOR_ADDR + 0x101u, 0x1000);       /* unaligned SP */
    CHECK(boot_validate(slot, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_ERR_SP);

    make(0x20020000u, APP_VECTOR_ADDR + 0x100u, 0x1000);       /* no Thumb bit */
    CHECK(boot_validate(slot, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_ERR_RESET_VECTOR);
    make(0x20020000u, 0x08000101u, 0x1000);                    /* points into bootloader */
    CHECK(boot_validate(slot, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_ERR_RESET_VECTOR);

    puts(fails ? "FAILED" : "all tests passed");
    return fails != 0;
}
