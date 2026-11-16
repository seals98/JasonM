#include <stdio.h>
#include <string.h>
#include "xmodem.h"
#include "boot.h"
#include "config.h"
#include "crc32.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static uint16_t crc16(const uint8_t *p, size_t n)
{
    uint16_t c = 0;
    while (n--) { c ^= (uint16_t)*p++ << 8; for (int i = 0; i < 8; i++) c = (c & 0x8000) ? (c << 1) ^ 0x1021 : c << 1; }
    return c;
}

/* ---- reactive XMODEM sender: advances on ACK, resends on NAK ---- */
#define MAXPKT 4096
static struct { uint8_t b[1030]; int len; } pkt[MAXPKT];
static int npkt, idx, pos, corrupt_idx, corrupt_active, dup_idx, dup_done;
static int acks, naks, cans;

static void build(const uint8_t *data, size_t n, int blk)
{
    npkt = 0; uint8_t seq = 1;
    for (size_t off = 0; off < n; off += blk, seq++) {
        uint8_t *p = pkt[npkt].b;
        p[0] = blk == 128 ? 0x01 : 0x02; p[1] = seq; p[2] = ~seq;
        size_t m = n - off < (size_t)blk ? n - off : (size_t)blk;
        memset(p + 3, 0x1A, blk); memcpy(p + 3, data + off, m);
        uint16_t c = crc16(p + 3, blk);
        p[3 + blk] = c >> 8; p[4 + blk] = c & 0xFF;
        pkt[npkt++].len = 5 + blk;
    }
    pkt[npkt].b[0] = 0x04; pkt[npkt++].len = 1;           /* EOT */
    idx = pos = 0; corrupt_idx = dup_idx = -1; corrupt_active = dup_done = 0;
    acks = naks = cans = 0;
}

static void m_putc(uint8_t c)
{
    if (c == 0x06) { acks++; if (idx == dup_idx && !dup_done) { dup_done = 1; pos = 0; return; } idx++; pos = 0; }
    else if (c == 0x15) { naks++; corrupt_active = 0; pos = 0; }
    else if (c == 0x18) cans++;
}
static int m_getc(uint32_t t)
{
    (void)t;
    if (idx >= npkt || pos >= pkt[idx].len) return -1;
    uint8_t v = pkt[idx].b[pos];
    if (idx == corrupt_idx && corrupt_active && pos == 20) v ^= 0x40;
    pos++;
    return v;
}

static uint8_t flash[APP_SLOT_SIZE];
static int m_write(uint32_t off, const uint8_t *d, uint32_t n)
{
    if (off + n > sizeof flash) return -1;
    memcpy(flash + off, d, n); return 0;
}
static int fail_write(uint32_t off, const uint8_t *d, uint32_t n) { (void)off; (void)d; (void)n; return -1; }

static const xmodem_io_t io = { m_getc, m_putc, m_write };

static size_t make_image(uint8_t *out, uint32_t app_size)
{
    memset(out, 0xFF, APP_HEADER_SIZE);
    uint32_t *vt = (uint32_t *)(out + APP_HEADER_SIZE);
    vt[0] = 0x20020000u; vt[1] = APP_VECTOR_ADDR + 0x101u;
    for (uint32_t i = 2; i < app_size / 4; i++) vt[i] = i * 2654435761u;
    image_header_t h = { IMAGE_MAGIC, IMAGE_HEADER_VER, app_size, crc32(vt, app_size), 1, {0} };
    memcpy(out, &h, sizeof h);
    return APP_HEADER_SIZE + app_size;
}

static void run(int blk)
{
    static uint8_t img[APP_HEADER_SIZE + 5000];
    uint8_t buf[XMODEM_BUF_SIZE];
    size_t n = make_image(img, 5000);

    /* clean transfer -> flash holds a valid image */
    memset(flash, 0xFF, sizeof flash); build(img, n, blk);
    int r = xmodem_receive(&io, buf, APP_SLOT_SIZE);
    CHECK(r >= (int)n && r - (int)n < blk);
    CHECK(memcmp(flash, img, n) == 0);
    CHECK(boot_validate(flash, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_OK);
    CHECK(naks == 0 && cans == 0 && acks == npkt);

    /* corrupted packet -> NAK + retransmit, result still identical */
    memset(flash, 0xFF, sizeof flash); build(img, n, blk);
    corrupt_idx = 2; corrupt_active = 1;
    r = xmodem_receive(&io, buf, APP_SLOT_SIZE);
    CHECK(r >= (int)n && naks == 1);
    CHECK(boot_validate(flash, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_OK);

    /* sender repeats a block (lost ACK) -> duplicate is ACKed but not re-written */
    memset(flash, 0xFF, sizeof flash); build(img, n, blk);
    dup_idx = 1;
    r = xmodem_receive(&io, buf, APP_SLOT_SIZE);
    CHECK(r >= (int)n && acks == npkt + 1);
    CHECK(boot_validate(flash, APP_SLOT_ADDR, APP_SLOT_SIZE) == BOOT_OK);

    /* image bigger than the slot -> cancelled */
    build(img, n, blk);
    CHECK(xmodem_receive(&io, buf, 1000) == XM_ERR_TOO_BIG && cans == 3);

    /* flash write failure -> cancelled */
    xmodem_io_t bad = { m_getc, m_putc, fail_write };
    build(img, n, blk);
    CHECK(xmodem_receive(&bad, buf, APP_SLOT_SIZE) == XM_ERR_WRITE && cans == 3);
}

static int silent_getc(uint32_t t) { (void)t; return -1; }
static int polls;
static void count_putc(uint8_t c) { if (c == 'C') polls++; }

int main(void)
{
    run(1024);
    run(128);

    uint8_t buf[XMODEM_BUF_SIZE];
    xmodem_io_t quiet = { silent_getc, count_putc, m_write };
    CHECK(xmodem_receive(&quiet, buf, APP_SLOT_SIZE) == XM_ERR_TIMEOUT && polls == 60);

    puts(fails ? "FAILED" : "xmodem tests passed");
    return fails != 0;
}
