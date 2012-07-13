/*
 * Blackfin Event Vector Table (EVT) model.
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "trace-root.h"
#include "bfin_devices.h"

#define TYPE_BFIN_EVT "bfin_evt"
#define BFIN_EVT(obj) OBJECT_CHECK(BfinEVTState, (obj), TYPE_BFIN_EVT)

typedef struct BfinEVTState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 evt[16];
} BfinEVTState;
#define BfinMMRState BfinEVTState
#define mmr_base() offsetof(BfinMMRState, evt[0])

static const char * const mmr_names[0x2000 / 4] =
{
  "EVT0", "EVT1", "EVT2", "EVT3", "EVT4", "EVT5", "EVT6", "EVT7", "EVT8",
  "EVT9", "EVT10", "EVT11", "EVT12", "EVT13", "EVT14", "EVT15",
};

static void bfin_evt_io_write(void *opaque, hwaddr addr,
                              uint64_t value, unsigned size)
{
    BfinEVTState *s = opaque;
    bu32 *valuep = (void *)((uintptr_t)s + mmr_base() + addr);

    HW_TRACE_WRITE();

    *valuep = value;
}

static uint64_t bfin_evt_io_read(void *opaque, hwaddr addr, unsigned size)
{
    BfinEVTState *s = opaque;
    bu32 *valuep = (void *)((uintptr_t)s + mmr_base() + addr);

    HW_TRACE_READ();

    return *valuep;
}

static const MemoryRegionOps bfin_evt_io_ops = {
    .read = bfin_evt_io_read,
    .write = bfin_evt_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static int bfin_evt_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    BfinEVTState *s = BFIN_EVT(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &bfin_evt_io_ops, s, "evt", mmr_size());
    sysbus_init_mmio(sbd, &s->iomem);

    return 0;
}

static void bfin_evt_class_init(ObjectClass *klass, void *data)
{
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = bfin_evt_init;
}

static TypeInfo bfin_evt_info = {
    .name          = TYPE_BFIN_EVT,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BfinEVTState),
    .class_init    = bfin_evt_class_init,
};

static void bfin_evt_register_types(void)
{
    type_register_static(&bfin_evt_info);
}

type_init(bfin_evt_register_types)
