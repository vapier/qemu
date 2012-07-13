/*
 * Blackfin Memory Management Unit (MMU) model.
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

#define TYPE_BFIN_MMU "bfin_mmu"
#define BFIN_MMU(obj) OBJECT_CHECK(BfinMMUState, (obj), TYPE_BFIN_MMU)

typedef struct BfinMMUState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 sram_base_address;

  bu32 dmem_control, dcplb_fault_status, dcplb_fault_addr;
  char _dpad0[0x100 - 0x0 - (4 * 4)];
  bu32 dcplb_addr[16];
  char _dpad1[0x200 - 0x100 - (4 * 16)];
  bu32 dcplb_data[16];
  char _dpad2[0x300 - 0x200 - (4 * 16)];
  bu32 dtest_command;
  char _dpad3[0x400 - 0x300 - (4 * 1)];
  bu32 dtest_data[2];

  char _dpad4[0x1000 - 0x400 - (4 * 2)];

  bu32 idk; /* Filler MMR; hardware simply ignores.  */
  bu32 imem_control, icplb_fault_status, icplb_fault_addr;
  char _ipad0[0x100 - 0x0 - (4 * 4)];
  bu32 icplb_addr[16];
  char _ipad1[0x200 - 0x100 - (4 * 16)];
  bu32 icplb_data[16];
  char _ipad2[0x300 - 0x200 - (4 * 16)];
  bu32 itest_command;
  char _ipad3[0x400 - 0x300 - (4 * 1)];
  bu32 itest_data[2];
} BfinMMUState;
#define BfinMMRState BfinMMUState
#define mmr_base() offsetof(BfinMMRState, sram_base_address)

static const char * const mmr_names[0x2000 / 4] =
{
  "SRAM_BASE_ADDRESS", "DMEM_CONTROL", "DCPLB_FAULT_STATUS", "DCPLB_FAULT_ADDR",
  [mmr_idx (dcplb_addr[0])] = "DCPLB_ADDR0",
  "DCPLB_ADDR1", "DCPLB_ADDR2", "DCPLB_ADDR3", "DCPLB_ADDR4", "DCPLB_ADDR5",
  "DCPLB_ADDR6", "DCPLB_ADDR7", "DCPLB_ADDR8", "DCPLB_ADDR9", "DCPLB_ADDR10",
  "DCPLB_ADDR11", "DCPLB_ADDR12", "DCPLB_ADDR13", "DCPLB_ADDR14", "DCPLB_ADDR15",
  [mmr_idx (dcplb_data[0])] = "DCPLB_DATA0",
  "DCPLB_DATA1", "DCPLB_DATA2", "DCPLB_DATA3", "DCPLB_DATA4", "DCPLB_DATA5",
  "DCPLB_DATA6", "DCPLB_DATA7", "DCPLB_DATA8", "DCPLB_DATA9", "DCPLB_DATA10",
  "DCPLB_DATA11", "DCPLB_DATA12", "DCPLB_DATA13", "DCPLB_DATA14", "DCPLB_DATA15",
  [mmr_idx (dtest_command)] = "DTEST_COMMAND",
  [mmr_idx (dtest_data[0])] = "DTEST_DATA0", "DTEST_DATA1",
  [mmr_idx (imem_control)] = "IMEM_CONTROL", "ICPLB_FAULT_STATUS", "ICPLB_FAULT_ADDR",
  [mmr_idx (icplb_addr[0])] = "ICPLB_ADDR0",
  "ICPLB_ADDR1", "ICPLB_ADDR2", "ICPLB_ADDR3", "ICPLB_ADDR4", "ICPLB_ADDR5",
  "ICPLB_ADDR6", "ICPLB_ADDR7", "ICPLB_ADDR8", "ICPLB_ADDR9", "ICPLB_ADDR10",
  "ICPLB_ADDR11", "ICPLB_ADDR12", "ICPLB_ADDR13", "ICPLB_ADDR14", "ICPLB_ADDR15",
  [mmr_idx (icplb_data[0])] = "ICPLB_DATA0",
  "ICPLB_DATA1", "ICPLB_DATA2", "ICPLB_DATA3", "ICPLB_DATA4", "ICPLB_DATA5",
  "ICPLB_DATA6", "ICPLB_DATA7", "ICPLB_DATA8", "ICPLB_DATA9", "ICPLB_DATA10",
  "ICPLB_DATA11", "ICPLB_DATA12", "ICPLB_DATA13", "ICPLB_DATA14", "ICPLB_DATA15",
  [mmr_idx (itest_command)] = "ITEST_COMMAND",
  [mmr_idx (itest_data[0])] = "ITEST_DATA0", "ITEST_DATA1",
};

static void bfin_mmu_io_write(void *opaque, hwaddr addr,
                              uint64_t value, unsigned size)
{
    BfinMMUState *s = opaque;

    HW_TRACE_WRITE();
}

static uint64_t bfin_mmu_io_read(void *opaque, hwaddr addr, unsigned size)
{
    BfinMMUState *s = opaque;

    HW_TRACE_READ();

    return 0;
}

static const MemoryRegionOps bfin_mmu_io_ops = {
    .read = bfin_mmu_io_read,
    .write = bfin_mmu_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static int bfin_mmu_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    BfinMMUState *s = BFIN_MMU(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &bfin_mmu_io_ops, s, "mmu", mmr_size());
    sysbus_init_mmio(sbd, &s->iomem);

    return 0;
}

static void bfin_mmu_class_init(ObjectClass *klass, void *data)
{
//    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = bfin_mmu_init;
}

static TypeInfo bfin_mmu_info = {
    .name          = "bfin_mmu",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BfinMMUState),
    .class_init    = bfin_mmu_class_init,
};

static void bfin_mmu_register_types(void)
{
    type_register_static(&bfin_mmu_info);
}

type_init(bfin_mmu_register_types)
