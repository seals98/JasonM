#include "xmodem.h"

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#define START_POLLS  60     /* 'C' once per second before giving up */
#define MAX_RETRIES  10

static uint16_t crc16(const uint8_t *p, uint32_t n)
{
    uint16_t crc = 0;
    while (n--) {
        crc ^= (uint16_t)*p++ << 8;
        for (int i = 0; i < 8; i++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}

static void cancel(const xmodem_io_t *io)
{
    io->putc(CAN); io->putc(CAN); io->putc(CAN);
}

int xmodem_receive(const xmodem_io_t *io, uint8_t *buf, uint32_t max_size)
{
    uint32_t offset = 0;
    uint8_t expect = 1;
    int started = 0, polls = 0, retries = 0;

    for (;;) {
        int c;
        if (!started) {
            io->putc('C');
            c = io->getc(1000);
            if (c < 0) {
                if (++polls >= START_POLLS) return XM_ERR_TIMEOUT;
                continue;
            }
        } else {
            c = io->getc(1000);
            if (c < 0) goto retry;
        }

        if (c == EOT) {
            if (!started) continue;
            io->putc(ACK);
            return (int)offset;
        }
        if (c == CAN) {
            if (io->getc(1000) == CAN) return XM_ERR_CANCELLED;
            continue;
        }
        if (c != SOH && c != STX) continue;       /* line noise */

        started = 1;
        uint32_t len = (c == SOH) ? 128u : 1024u;
        int blk = io->getc(1000), inv = io->getc(1000);
        if (blk < 0 || inv < 0 || (blk ^ inv) != 0xFF) goto retry;

        for (uint32_t i = 0; i < len; i++) {
            int b = io->getc(1000);
            if (b < 0) goto retry;
            buf[i] = (uint8_t)b;
        }
        int hi = io->getc(1000), lo = io->getc(1000);
        if (hi < 0 || lo < 0 || crc16(buf, len) != (uint16_t)((hi << 8) | lo)) goto retry;

        if ((uint8_t)blk == expect) {
            if (offset + len > max_size) { cancel(io); return XM_ERR_TOO_BIG; }
            if (io->write(offset, buf, len) != 0) { cancel(io); return XM_ERR_WRITE; }
            offset += len;
            expect++;
            retries = 0;
            io->putc(ACK);
        } else if ((uint8_t)blk == (uint8_t)(expect - 1)) {
            io->putc(ACK);                        /* sender missed our ACK: already stored */
        } else {
            cancel(io);
            return XM_ERR_SEQ;
        }
        continue;

    retry:
        while (io->getc(100) >= 0) {}             /* let the line go idle before NAK */
        if (++retries > MAX_RETRIES) { cancel(io); return XM_ERR_TIMEOUT; }
        io->putc(NAK);
    }
}
