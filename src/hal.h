/* Board/chip services the update path needs. Implemented in board_stm32h7.c / flash_h7.c. */
#ifndef HAL_H
#define HAL_H
#include <stdint.h>

uint32_t millis(void);               /* free-running ms counter; call it while polling */
void     uart_putc(uint8_t c);
int      uart_getc(uint32_t timeout_ms);   /* byte, or -1 on timeout */
void     uart_puts(const char *s);

/* Application-slot programming. Offsets are relative to APP_SLOT_ADDR. */
void flash_begin(void);              /* unlock, forget erase state */
int  flash_program(uint32_t offset, const uint8_t *data, uint32_t len);  /* 0 = ok; erases lazily */
void flash_end(void);                /* lock, invalidate caches */
#endif
