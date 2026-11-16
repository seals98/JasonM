/*
 * Example byte-oriented I2C master controller for QEMU ("jm-i2c").
 *
 * This is NOT a real SoC's controller: it is a deliberately small register map you replace with
 * the real one (see the README). It shows the three things every QEMU bus-master model needs:
 *   1. MMIO register read/write handlers      (jm_i2c_read / jm_i2c_write)
 *   2. translating register writes into QEMU's I2C bus API (i2c_start_*, i2c_send, i2c_recv, ...)
 *   3. registering the device (QOM type, vmstate, reset, "i2c" child bus)
 *
 * Register map (32-bit registers, offsets from the base address):
 *   0x00 CTRL    rw  bit0 EN
 *   0x04 ADDR    rw  [6:0] 7-bit target address
 *   0x08 CMD     wo  1 START_W  2 START_R  3 WRITE(TXDATA)  4 READ_ACK  5 READ_NACK  6 STOP
 *   0x0C TXDATA  rw  [7:0] byte for CMD=WRITE
 *   0x10 RXDATA  ro  [7:0] byte captured by CMD=READ_*
 *   0x14 STATUS  rw1c bit1 DONE (last command finished)  bit2 NACK  bit3 ERR (bad sequence / disabled)
 *   0x18 ID      ro  0x4A4D4932 ("JMI2")
 * Commands complete instantly in this model; firmware should still poll DONE, as it must on silicon.
 */
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "hw/i2c/jm_i2c.h"

#define R_CTRL   0x00
#define R_ADDR   0x04
#define R_CMD    0x08
#define R_TXDATA 0x0C
#define R_RXDATA 0x10
#define R_STATUS 0x14
#define R_ID     0x18

#define CTRL_EN      (1u << 0)
#define ST_DONE      (1u << 1)
#define ST_NACK      (1u << 2)
#define ST_ERR       (1u << 3)

enum { CMD_START_W = 1, CMD_START_R, CMD_WRITE, CMD_READ_ACK, CMD_READ_NACK, CMD_STOP };
enum { XFER_IDLE = 0, XFER_WRITE, XFER_READ };

static void jm_i2c_command(JmI2CState *s, uint32_t cmd)
{
    s->status &= ~ST_DONE;

    if (!(s->ctrl & CTRL_EN)) {
        s->status |= ST_ERR;
        goto done;
    }

    switch (cmd) {
    case CMD_START_W:
    case CMD_START_R: {
        bool recv = (cmd == CMD_START_R);
        /* A START while a transfer is open is a repeated START: QEMU handles it. */
        if (i2c_start_transfer(s->bus, s->addr & 0x7F, recv)) {
            s->status |= ST_NACK;          /* nobody answered this address */
            s->xfer = XFER_IDLE;
            i2c_end_transfer(s->bus);
        } else {
            s->xfer = recv ? XFER_READ : XFER_WRITE;
        }
        break;
    }
    case CMD_WRITE:
        if (s->xfer != XFER_WRITE) {
            s->status |= ST_ERR;
        } else if (i2c_send(s->bus, s->txdata & 0xFF)) {
            s->status |= ST_NACK;          /* target NACKed this data byte */
        }
        break;
    case CMD_READ_ACK:
    case CMD_READ_NACK:
        if (s->xfer != XFER_READ) {
            s->status |= ST_ERR;
        } else {
            s->rxdata = i2c_recv(s->bus);
            if (cmd == CMD_READ_NACK) {
                i2c_nack(s->bus);          /* "no more bytes": tell the target before STOP */
            }
        }
        break;
    case CMD_STOP:
        if (s->xfer != XFER_IDLE) {
            i2c_end_transfer(s->bus);
            s->xfer = XFER_IDLE;
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "jm-i2c: bad command %u\n", cmd);
        s->status |= ST_ERR;
        break;
    }
done:
    s->status |= ST_DONE;
}

static uint64_t jm_i2c_read(void *opaque, hwaddr offset, unsigned size)
{
    JmI2CState *s = JM_I2C(opaque);

    switch (offset) {
    case R_CTRL:   return s->ctrl;
    case R_ADDR:   return s->addr;
    case R_TXDATA: return s->txdata;
    case R_RXDATA: return s->rxdata & 0xFF;
    case R_STATUS: return s->status;
    case R_ID:     return 0x4A4D4932;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "jm-i2c: read of bad offset 0x%" HWADDR_PRIx "\n", offset);
        return 0;
    }
}

static void jm_i2c_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    JmI2CState *s = JM_I2C(opaque);

    switch (offset) {
    case R_CTRL:
        s->ctrl = value & CTRL_EN;
        if (!(s->ctrl & CTRL_EN) && s->xfer != XFER_IDLE) {   /* disabling aborts a transfer */
            i2c_end_transfer(s->bus);
            s->xfer = XFER_IDLE;
        }
        break;
    case R_ADDR:   s->addr = value & 0x7F; break;
    case R_CMD:    jm_i2c_command(s, value & 0xFF); break;
    case R_TXDATA: s->txdata = value & 0xFF; break;
    case R_STATUS: s->status &= ~(value & (ST_NACK | ST_ERR)); break;   /* write 1 to clear */
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "jm-i2c: write to bad offset 0x%" HWADDR_PRIx "\n", offset);
        break;
    }
}

static const MemoryRegionOps jm_i2c_ops = {
    .read = jm_i2c_read,
    .write = jm_i2c_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
};

static void jm_i2c_reset(DeviceState *dev)
{
    JmI2CState *s = JM_I2C(dev);

    if (s->xfer != XFER_IDLE) {
        i2c_end_transfer(s->bus);
    }
    s->ctrl = s->addr = s->txdata = s->rxdata = 0;
    s->status = 0;
    s->xfer = XFER_IDLE;
}

static void jm_i2c_realize(DeviceState *dev, Error **errp)
{
    JmI2CState *s = JM_I2C(dev);

    memory_region_init_io(&s->iomem, OBJECT(dev), &jm_i2c_ops, s, TYPE_JM_I2C, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
    s->bus = i2c_init_bus(dev, "i2c");     /* child bus: slaves attach here */
}

static const VMStateDescription jm_i2c_vmstate = {
    .name = TYPE_JM_I2C,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(ctrl, JmI2CState),
        VMSTATE_UINT32(addr, JmI2CState),
        VMSTATE_UINT32(txdata, JmI2CState),
        VMSTATE_UINT32(rxdata, JmI2CState),
        VMSTATE_UINT32(status, JmI2CState),
        VMSTATE_UINT32(xfer, JmI2CState),
        VMSTATE_END_OF_LIST()
    }
};

static void jm_i2c_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = jm_i2c_realize;
    dc->reset = jm_i2c_reset;
    dc->vmsd = &jm_i2c_vmstate;
    dc->desc = "Example I2C master controller";
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo jm_i2c_info = {
    .name = TYPE_JM_I2C,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JmI2CState),
    .class_init = jm_i2c_class_init,
};

static void jm_i2c_register_types(void)
{
    type_register_static(&jm_i2c_info);
}

type_init(jm_i2c_register_types)
