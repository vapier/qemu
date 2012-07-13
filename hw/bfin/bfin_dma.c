/*
 * Blackfin Direct Memory Access (DMA) Channel model.
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "trace.h"
#include "bfin_devices.h"

#define TYPE_BFIN_DMA "bfin_dma"
#define BFIN_DMA(obj) OBJECT_CHECK(BfinDMAState, (obj), TYPE_BFIN_DMA)

typedef struct BfinDMAState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

  /* Order after here is important -- matches hardware MMR layout.  */
  union {
    struct { bu16 ndpl, ndph; };
    bu32 next_desc_ptr;
  };
  union {
    struct { bu16 sal, sah; };
    bu32 start_addr;
  };
  bu16 BFIN_MMR_16 (config);
  bu32 _pad0;
  bu16 BFIN_MMR_16 (x_count);
  bs16 BFIN_MMR_16 (x_modify);
  bu16 BFIN_MMR_16 (y_count);
  bs16 BFIN_MMR_16 (y_modify);
  bu32 curr_desc_ptr, curr_addr;
  bu16 BFIN_MMR_16 (irq_status);
  bu16 BFIN_MMR_16 (peripheral_map);
  bu16 BFIN_MMR_16 (curr_x_count);
  bu32 _pad1;
  bu16 BFIN_MMR_16 (curr_y_count);
  bu32 _pad2;
} BfinDMAState;
#define BfinMMRState BfinDMAState
#define mmr_base() offsetof(BfinMMRState, next_desc_ptr)

static const char * const mmr_names[] =
{
  "NEXT_DESC_PTR", "START_ADDR", "CONFIG", "<INV>", "X_COUNT", "X_MODIFY",
  "Y_COUNT", "Y_MODIFY", "CURR_DESC_PTR", "CURR_ADDR", "IRQ_STATUS",
  "PERIPHERAL_MAP", "CURR_X_COUNT", "<INV>", "CURR_Y_COUNT", "<INV>",
};

static void bfin_dma_io_write(void *opaque, hwaddr addr,
                              uint64_t value, unsigned size)
{
    BfinDMAState *s = opaque;

    HW_TRACE_WRITE();
}

static uint64_t bfin_dma_io_read(void *opaque, hwaddr addr, unsigned size)
{
    BfinDMAState *s = opaque;

    HW_TRACE_READ();

    return 0;
}

static const MemoryRegionOps bfin_dma_io_ops = {
    .read = bfin_dma_io_read,
    .write = bfin_dma_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 2,
        .max_access_size = 4,
    },
};

static int bfin_dma_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    BfinDMAState *s = BFIN_DMA(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &bfin_dma_io_ops, s, "dma", mmr_size());
    sysbus_init_mmio(sbd, &s->iomem);

    return 0;
}

static void bfin_dma_class_init(ObjectClass *klass, void *data)
{
//    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = bfin_dma_init;
}

static TypeInfo bfin_dma_info = {
    .name          = "bfin_dma",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BfinDMAState),
    .class_init    = bfin_dma_class_init,
};

static void bfin_dma_register_types(void)
{
    type_register_static(&bfin_dma_info);
}

type_init(bfin_dma_register_types)
