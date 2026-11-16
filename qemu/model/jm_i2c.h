/* Example I2C master controller ("jm-i2c") for QEMU - register map in jm_i2c.c. */
#ifndef HW_I2C_JM_I2C_H
#define HW_I2C_JM_I2C_H

#include "hw/sysbus.h"
#include "hw/i2c/i2c.h"
#include "qom/object.h"

#define TYPE_JM_I2C "jm-i2c"
OBJECT_DECLARE_SIMPLE_TYPE(JmI2CState, JM_I2C)

struct JmI2CState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    I2CBus *bus;

    uint32_t ctrl;      /* CTRL   */
    uint32_t addr;      /* ADDR   */
    uint32_t txdata;    /* TXDATA */
    uint32_t rxdata;    /* RXDATA */
    uint32_t status;    /* STATUS */
    uint32_t xfer;      /* 0 = idle, 1 = write transfer open, 2 = read transfer open */
};

#endif
