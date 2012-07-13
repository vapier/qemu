/*
 * Common Blackfin device model code
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#ifndef BFIN_DEVICES_H
#define BFIN_DEVICES_H

#define mmr_size()      (sizeof(BfinMMRState) - mmr_base())
#define mmr_offset(mmr) (offsetof(BfinMMRState, mmr) - mmr_base())
#define mmr_idx(mmr)    (mmr_offset(mmr) / 4)
#define mmr_name(off)   (mmr_names[(off) / 4] ? : "<INV>")

#define HW_TRACE_WRITE() \
    trace_bfin_reg_memory_write(addr + s->iomem.addr, mmr_name(addr), size, \
                                value)
#define HW_TRACE_READ() \
    trace_bfin_reg_memory_read(addr + s->iomem.addr, mmr_name(addr), size)

typedef int16_t bs16;
typedef int32_t bs32;
typedef uint16_t bu16;
typedef uint32_t bu32;

#define BFIN_MMR_16(mmr) mmr, __pad_##mmr

#endif
