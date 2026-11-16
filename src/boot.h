#ifndef BOOT_H
#define BOOT_H
#include <stdint.h>

/* Placed at APP_SLOT_ADDR (offset 0). Remaining bytes up to APP_HEADER_SIZE are padding. */
typedef struct {
    uint32_t magic;        /* IMAGE_MAGIC                                   */
    uint32_t header_ver;   /* IMAGE_HEADER_VER                              */
    uint32_t image_size;   /* bytes of application following the header     */
    uint32_t crc32;        /* CRC-32 of those image_size bytes              */
    uint32_t app_version;  /* free-form, informational                      */
    uint32_t reserved[3];
} image_header_t;

typedef enum {
    BOOT_OK = 0,
    BOOT_ERR_MAGIC,
    BOOT_ERR_HEADER_VER,
    BOOT_ERR_SIZE,
    BOOT_ERR_CRC,
    BOOT_ERR_SP,
    BOOT_ERR_RESET_VECTOR,
} boot_status_t;

/* Validate an application slot.
 *   mem  : where the slot can be read from (== (const void *)addr on target)
 *   addr : the slot's nominal address, used for range checks
 *   size : slot size in bytes (header included) */
boot_status_t boot_validate(const uint8_t *mem, uint32_t addr, uint32_t size);

/* Hand control to the vector table at vt_addr. Never returns. */
void boot_jump(uint32_t vt_addr) __attribute__((noreturn));

/* Board hooks (weak defaults in board.c). */
void board_clock_init(void);   /* optional: raise core clock to CORE_CLOCK_HZ */
void board_init(void);
void board_deinit(void);
void board_update_loop(boot_status_t why);   /* must not return unless a new image was installed */

#endif
