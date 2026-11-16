#ifndef XMODEM_H
#define XMODEM_H
#include <stdint.h>

typedef struct {
    int  (*getc)(uint32_t timeout_ms);                     /* byte or -1 on timeout */
    void (*putc)(uint8_t c);
    int  (*write)(uint32_t offset, const uint8_t *data, uint32_t len);   /* 0 = ok */
} xmodem_io_t;

enum { XM_ERR_TIMEOUT = -1, XM_ERR_CANCELLED = -2, XM_ERR_TOO_BIG = -3,
       XM_ERR_WRITE = -4, XM_ERR_SEQ = -5 };

#define XMODEM_BUF_SIZE 1024u

/* XMODEM-CRC receiver (128 B and 1 K blocks). buf must hold XMODEM_BUF_SIZE bytes.
 * Returns bytes written (incl. sender padding) or a negative XM_ERR_*. */
int xmodem_receive(const xmodem_io_t *io, uint8_t *buf, uint32_t max_size);
#endif
