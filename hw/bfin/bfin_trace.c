/*
 * Blackfin Trace (TBUF) model.
 *
 * Copyright 2007-2014 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "hw/hw.h"
#include "hw/sysbus.h"
#include "trace.h"
#include "bfin_devices.h"

#define TYPE_BFIN_TRACE "bfin_trace"
#define BFIN_TRACE(obj) OBJECT_CHECK(BfinTraceState, (obj), TYPE_BFIN_TRACE)

typedef struct BfinTraceState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 tbufctl, tbufstat;
  char _pad[0x100 - 0x8];
  bu32 tbuf;
} BfinTraceState;
#define BfinMMRState BfinTraceState
#define mmr_base() offsetof(BfinMMRState, tbufctl)

static const char * const mmr_names[] =
{
  "TBUFCTL", "TBUFSTAT", [mmr_offset (tbuf) / 4] = "TBUF",
};

static void bfin_trace_io_write(void *opaque, hwaddr addr,
                                uint64_t value, unsigned size)
{
    BfinTraceState *s = opaque;

    HW_TRACE_WRITE();
}

static uint64_t bfin_trace_io_read(void *opaque, hwaddr addr, unsigned size)
{
    BfinTraceState *s = opaque;

    HW_TRACE_READ();

    return 0;
}

static const MemoryRegionOps bfin_trace_io_ops = {
    .read = bfin_trace_io_read,
    .write = bfin_trace_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static int bfin_trace_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    BfinTraceState *s = BFIN_TRACE(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &bfin_trace_io_ops, s, "trace", mmr_size());
    sysbus_init_mmio(sbd, &s->iomem);

    return 0;
}

static void bfin_trace_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = bfin_trace_init;
}

static TypeInfo bfin_trace_info = {
    .name          = "bfin_trace",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BfinTraceState),
    .class_init    = bfin_trace_class_init,
};

static void bfin_trace_register_types(void)
{
    type_register_static(&bfin_trace_info);
}

type_init(bfin_trace_register_types)
