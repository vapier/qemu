/*
 * Blackfin Trace (TBUF) model.
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

#define TYPE_BFIN_TRACE "bfin_trace"
#define BFIN_TRACE(obj) OBJECT_CHECK(BfinTraceState, (obj), TYPE_BFIN_TRACE)

/* Note: The circular buffering here might look a little buggy wrt mid-reads
         and consuming the top entry, but this is simulating hardware behavior.
         The hardware is simple, dumb, and fast.  Don't write dumb Blackfin
         software and you won't have a problem.  */

/* The hardware is limited to 16 entries and defines TBUFCTL.  Let's extend it ;).  */
#ifndef SIM_BFIN_TRACE_DEPTH
#define SIM_BFIN_TRACE_DEPTH 6
#endif
#define SIM_BFIN_TRACE_LEN (1 << SIM_BFIN_TRACE_DEPTH)
#define SIM_BFIN_TRACE_LEN_MASK (SIM_BFIN_TRACE_LEN - 1)

struct bfin_trace_entry {
    bu32 src, dst;
};

typedef struct BfinTraceState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

    struct bfin_trace_entry buffer[SIM_BFIN_TRACE_LEN];
    int top, bottom;
    bool mid;

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

/* Ugh, circular buffers.  */
#define TBUF_LEN(t) ((t)->top - (t)->bottom)
#define TBUF_IDX(i) ((i) & SIM_BFIN_TRACE_LEN_MASK)
/* TOP is the next slot to fill.  */
#define TBUF_TOP(t) (&(t)->buffer[TBUF_IDX((t)->top)])
/* LAST is the latest valid slot.  */
#define TBUF_LAST(t) (&(t)->buffer[TBUF_IDX((t)->top - 1)])
/* LAST_LAST is the second-to-last valid slot.  */
#define TBUF_LAST_LAST(t) (&(t)->buffer[TBUF_IDX((t)->top - 2)])

static void bfin_trace_io_write(void *opaque, hwaddr addr,
                                uint64_t value, unsigned size)
{
    BfinTraceState *s = opaque;

    HW_TRACE_WRITE();

    switch (addr) {
    case mmr_offset(tbufctl):
        s->tbufctl = value;
        break;
    case mmr_offset(tbufstat):
    case mmr_offset(tbuf):
        /* Discard writes to these.  */
        break;
    default:
        /* TODO: Throw an invalid mmr exception.  */
        break;
    }
}

static uint64_t bfin_trace_io_read(void *opaque, hwaddr addr, unsigned size)
{
    BfinTraceState *s = opaque;

    HW_TRACE_READ();

    switch (addr) {
    case mmr_offset(tbufctl):
        return s->tbufctl;
    case mmr_offset(tbufstat):
        /* Hardware is limited to 16 entries, so to stay compatible with
           software, limit the value to 16.  For software algorithms that
           keep reading while (TBUFSTAT != 0), they'll get all of it.  */
        return MIN(TBUF_LEN(s), 16);
    case mmr_offset(tbuf):
        /* XXX: Implement this.  */
        return 0;
    default:
        /* TODO: Throw an invalid mmr exception.  */
        return 0;
    }
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
