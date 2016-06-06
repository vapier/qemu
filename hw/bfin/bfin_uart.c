/*
 * Blackfin Universal Asynchronous Receiver/Transmitter (UART) model.
 * For "old style" UARTs on BF53x/etc... parts.
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "chardev/char-fe.h"
#include "trace-root.h"
#include "bfin_uart.h"
#include "bfin_devices.h"

#define TYPE_BFIN_UART "bfin_uart"
#define BFIN_UART(obj) OBJECT_CHECK(BfinUARTState, (obj), TYPE_BFIN_UART)

typedef struct BfinUARTState {
    SysBusDevice parent_obj;
    CharBackend chr;

    MemoryRegion iomem;

    unsigned char saved_byte;
    int saved_count;

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

static bu16
bfin_uart_write_byte(BfinUARTState *s, bu16 thr, bu16 mcr)
{
    unsigned char ch = thr;

    if (mcr & LOOP_ENA) {
        /* XXX: This probably doesn't work exactly right with
                external FIFOs ...  */
        s->saved_byte = thr;
        s->saved_count = 1;
    }

    qemu_chr_fe_write(&s->chr, &ch, 1);

    return thr;
}

static void bfin_uart_io_write(void *opaque, hwaddr addr,
                               uint64_t value, unsigned size)
{
    BfinUARTState *s = opaque;
    bu16 *valuep = (void *)((uintptr_t)s + mmr_base() + addr);

    HW_TRACE_WRITE();

    switch (addr) {
    case mmr_offset(dll):
        if (s->lcr & DLAB) {
            s->dll = value;
        } else {
            s->thr = bfin_uart_write_byte(s, value, s->mcr);

/*
            if (uart->ier & ETBEI) {
                hw_port_event (me, DV_PORT_TX, 1);
            }
*/
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
      s->lcr = value; break;
    case mmr_offset(mcr):
    case mmr_offset(scr):
    case mmr_offset(gctl):
      *valuep = value;
      break;
    }
}

static bu16
bfin_uart_get_next_byte(BfinUARTState *s, bu16 rbr, bu16 mcr, bool *fresh)
{
    bool _fresh;

    /* NB: The "uart" here may only use interal state.  */

    if (!fresh) {
        fresh = &_fresh;
    }

    *fresh = false;

    if (s->saved_count > 0) {
        *fresh = true;
        rbr = s->saved_byte;
        --s->saved_count;
    }

    /* RX is disconnected, so only return local data.  */
    if (!(mcr & LOOP_ENA)) {
        qemu_chr_fe_accept_input(&s->chr);
    }

    return rbr;
}

static uint64_t bfin_uart_io_read(void *opaque, hwaddr addr, unsigned size)
{
    BfinUARTState *s = opaque;
    bu16 *valuep = (void *)((uintptr_t)s + mmr_base() + addr);

    HW_TRACE_READ();

    switch (addr) {
    case mmr_offset(dll):
        if (s->lcr & DLAB) {
            return s->dll;
        } else {
            s->rbr = bfin_uart_get_next_byte(s, s->rbr, s->mcr, NULL);
            return s->rbr;
        }
    case mmr_offset(dlh):
        if (s->lcr & DLAB) {
            return s->dlh;
        } else {
            return s->ier;
        }
    case mmr_offset(lsr): {
        uint64_t ret;
        /* XXX: Reads are destructive on most parts, but not all ...  */
        s->lsr |= TEMT | THRE | (s->saved_count > 0 ? DR : 0); // bfin_uart_get_status (me);
        ret = s->lsr;
        s->lsr = 0;
        return ret;
    }
    case mmr_offset(iir):
        /* XXX: Reads are destructive ...  */
    case mmr_offset(lcr):
    case mmr_offset(mcr):
    case mmr_offset(scr):
    case mmr_offset(gctl):
        return *valuep;
    }

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

static Property bfin_uart_properties[] = {
    DEFINE_PROP_CHR("chardev", BfinUARTState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void uart_rx(void *opaque, const uint8_t *buf, int size)
{
    BfinUARTState *s = opaque;

    if (s->lsr & DR) {
        s->lsr |= OE;
    }

    s->lsr |= DR;
    s->saved_byte = *buf;
    s->saved_count = 1;

//    uart_update_irq(s);
}

static int uart_can_rx(void *opaque)
{
    BfinUARTState *s = opaque;

    return !(s->lsr & DR);
}

static void uart_event(void *opaque, int event)
{
}

static void uart_reset(DeviceState *d)
{
    BfinUARTState *s = BFIN_UART(d);

    s->ier = 0;
    s->thr = 0;
    s->rbr = 0;
    s->dll = 0x0001;
    s->dlh = 0;
    s->iir = 0x0001;
    s->lcr = 0;
    s->mcr = 0;
    s->lsr = 0x0060;
    s->msr = 0;
    s->scr = 0;
    s->gctl = 0;
}

static void bfin_uart_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    BfinUARTState *s = BFIN_UART(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &bfin_uart_io_ops, s, "uart", mmr_size());
    sysbus_init_mmio(sbd, &s->iomem);
}

static void bfin_uart_realize(DeviceState *dev, Error **errp)
{
    BfinUARTState *s = BFIN_UART(dev);

    qemu_chr_fe_set_handlers(&s->chr, uart_can_rx, uart_rx,
                             uart_event, NULL, s, NULL, true);
}

static const VMStateDescription vmstate_bfin_uart = {
    .name = "bfin-uart",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
//        VMSTATE_UINT32_ARRAY(regs, BfinUartState, R_MAX),
        VMSTATE_END_OF_LIST()
    }
};

static void bfin_uart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = uart_reset;
    dc->vmsd = &vmstate_bfin_uart;
    dc->props = bfin_uart_properties;
    dc->realize = bfin_uart_realize;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static TypeInfo bfin_uart_info = {
    .name          = TYPE_BFIN_UART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BfinUARTState),
    .instance_init = bfin_uart_init,
    .class_init    = bfin_uart_class_init,
};

static void bfin_uart_register_types(void)
{
    type_register_static(&bfin_uart_info);
}

type_init(bfin_uart_register_types)
