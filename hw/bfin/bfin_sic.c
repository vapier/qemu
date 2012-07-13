/*
 * Blackfin System Interrupt Controller (SIC) model.
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

#define TYPE_BFIN_SIC "bfin_sic"
#define BFIN_SIC(obj) OBJECT_CHECK(BfinSICState, (obj), TYPE_BFIN_SIC)

typedef struct BfinSICState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(swrst);
  bu16 BFIN_MMR_16(syscr);
  bu16 BFIN_MMR_16(rvect);  /* XXX: BF59x has a 32bit AUX_REVID here.  */
  union {
    struct {
      bu32 imask0;
      bu32 iar0, iar1, iar2, iar3;
      bu32 isr0, iwr0;
      bu32 _pad0[9];
      bu32 imask1;
      bu32 iar4, iar5, iar6, iar7;
      bu32 isr1, iwr1;
    } bf52x;
    struct {
      bu32 imask;
      bu32 iar0, iar1, iar2, iar3;
      bu32 isr, iwr;
    } bf537;
    struct {
      bu32 imask0, imask1, imask2;
      bu32 isr0, isr1, isr2;
      bu32 iwr0, iwr1, iwr2;
      bu32 iar0, iar1, iar2, iar3;
      bu32 iar4, iar5, iar6, iar7;
      bu32 iar8, iar9, iar10, iar11;
    } bf54x;
    struct {
      bu32 imask0, imask1;
      bu32 iar0, iar1, iar2, iar3;
      bu32 iar4, iar5, iar6, iar7;
      bu32 isr0, isr1;
      bu32 iwr0, iwr1;
    } bf561;
  };
} BfinSICState;
#define BfinMMRState BfinSICState
#define mmr_base() offsetof(BfinMMRState, swrst)

static const char * const mmr_names[] =
{
  "SWRST", "SYSCR", "SIC_RVECT", "SIC_IMASK", "SIC_IAR0", "SIC_IAR1",
  "SIC_IAR2", "SIC_IAR3", "SIC_ISR", "SIC_IWR",
};

static void bfin_sic_io_write(void *opaque, hwaddr addr,
                              uint64_t value, unsigned size)
{
    BfinSICState *s = opaque;

    HW_TRACE_WRITE();
}

static uint64_t bfin_sic_io_read(void *opaque, hwaddr addr, unsigned size)
{
    BfinSICState *s = opaque;

    HW_TRACE_READ();

    return 0;
}

static const MemoryRegionOps bfin_sic_io_ops = {
    .read = bfin_sic_io_read,
    .write = bfin_sic_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 2,
        .max_access_size = 4,
    },
};

static int bfin_sic_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    BfinSICState *s = BFIN_SIC(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &bfin_sic_io_ops, s, "sic", mmr_size());
    sysbus_init_mmio(sbd, &s->iomem);

    return 0;
}

static void bfin_sic_class_init(ObjectClass *klass, void *data)
{
//    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = bfin_sic_init;
}

static TypeInfo bfin_sic_info = {
    .name          = "bfin_sic",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BfinSICState),
    .class_init    = bfin_sic_class_init,
};

static void bfin_sic_register_types(void)
{
    type_register_static(&bfin_sic_info);
}

type_init(bfin_sic_register_types)
