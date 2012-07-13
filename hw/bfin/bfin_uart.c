/*
 * Blackfin Universal Asynchronous Receiver/Transmitter (UART) model.
 * For "old style" UARTs on BF53x/etc... parts.
 *
 * Copyright 2007-2014 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "hw/hw.h"
#include "hw/sysbus.h"
#include "sysemu/char.h"
#include "trace.h"
#include "bfin_uart.h"
#include "bfin_devices.h"

#define TYPE_BFIN_UART "bfin_uart"
#define BFIN_UART(obj) OBJECT_CHECK(BfinUARTState, (obj), TYPE_BFIN_UART)

typedef struct BfinUARTState {
    SysBusDevice parent_obj;
    CharDriverState *chr;

    MemoryRegion iomem;

  /* This is aliased to DLH.  */
  bu16 ier;
  /* These are aliased to DLL.  */
  bu16 thr, rbr;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(dll);
  bu16 BFIN_MMR_16(dlh);
  bu16 BFIN_MMR_16(iir);
  bu16 BFIN_MMR_16(lcr);
  bu16 BFIN_MMR_16(mcr);
  bu16 BFIN_MMR_16(lsr);
  bu16 BFIN_MMR_16(msr);
  bu16 BFIN_MMR_16(scr);
  bu16 _pad0[2];
  bu16 BFIN_MMR_16(gctl);
} BfinUARTState;
#define BfinMMRState BfinUARTState
#define mmr_base() offsetof(BfinMMRState, dll)

static const char * const mmr_names[] =
{
  "UART_RBR/UART_THR", "UART_IER", "UART_IIR", "UART_LCR", "UART_MCR",
  "UART_LSR", "UART_MSR", "UART_SCR", "<INV>", "UART_GCTL",
};

#undef mmr_name
static const char *mmr_name(BfinUARTState *uart, bu32 idx)
{
  if (uart->lcr & DLAB)
    if (idx < 2)
      return idx == 0 ? "UART_DLL" : "UART_DLH";
  return mmr_names[idx];
}
#define mmr_name(off) mmr_name(s, (off) / 4)

static void bfin_uart_io_write(void *opaque, hwaddr addr,
                              uint64_t value, unsigned size)
{
    BfinUARTState *s = opaque;

    HW_TRACE_WRITE();

    switch (addr) {
    case mmr_offset(dll):
        if (s->lcr & DLAB) {
            s->dll = value;
        } else {
            unsigned char ch = value;
            qemu_chr_fe_write(s->chr, &ch, 1);
        }
        break;
    case mmr_offset(dlh):
        if (s->lcr & DLAB)
            s->dlh = value;
        else
            s->ier = value;
        break;
    case mmr_offset(iir):
    case mmr_offset(lsr):
      /* XXX: Writes are ignored ?  */
      break;
    case mmr_offset(lcr):
    case mmr_offset(mcr):
    case mmr_offset(scr):
    case mmr_offset(gctl):
      //*valuep = value;
      break;
    }
}

static uint64_t bfin_uart_io_read(void *opaque, hwaddr addr, unsigned size)
{
    BfinUARTState *s = opaque;

    HW_TRACE_READ();

    return 0;
}

static const MemoryRegionOps bfin_uart_io_ops = {
    .read = bfin_uart_io_read,
    .write = bfin_uart_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 2,
        .max_access_size = 2,
    },
};

static int bfin_uart_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    BfinUARTState *s = BFIN_UART(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &bfin_uart_io_ops, s, "uart", mmr_size());
    sysbus_init_mmio(sbd, &s->iomem);

    s->chr = qemu_char_get_next_serial();
    if (s->chr) {
//        qemu_chr_add_handlers(s->chr, juart_can_rx, juart_rx, juart_event, s);
    }

    return 0;
}

static void bfin_uart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = bfin_uart_init;
}

static TypeInfo bfin_uart_info = {
    .name          = "bfin_uart",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BfinUARTState),
    .class_init    = bfin_uart_class_init,
};

static void bfin_uart_register_types(void)
{
    type_register_static(&bfin_uart_info);
}

type_init(bfin_uart_register_types)
