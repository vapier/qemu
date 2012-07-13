/*
 * Blackfin cpu save/load logic
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "hw/hw.h"
#include "hw/boards.h"

#define VMSTATE_AUTO32_ARRAY(member) \
    VMSTATE_UINT32_ARRAY(member, CPUArchState, ARRAY_SIZE(((CPUArchState *)NULL)->member))
#define VMSTATE_AUTO64_ARRAY(member) \
    VMSTATE_UINT64_ARRAY(member, CPUArchState, ARRAY_SIZE(((CPUArchState *)NULL)->member))
#define _VMSTATE_UINT32(member) \
    VMSTATE_UINT32(member, CPUArchState)
const VMStateDescription vmstate_bfin_cpu = {
    .name = "cpu",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_AUTO32_ARRAY(dreg),
        VMSTATE_AUTO32_ARRAY(preg),
        VMSTATE_AUTO32_ARRAY(ireg),
        VMSTATE_AUTO32_ARRAY(mreg),
        VMSTATE_AUTO32_ARRAY(breg),
        VMSTATE_AUTO32_ARRAY(lreg),
        VMSTATE_AUTO64_ARRAY(areg),
        _VMSTATE_UINT32(rets),
        VMSTATE_AUTO32_ARRAY(lcreg),
        VMSTATE_AUTO32_ARRAY(ltreg),
        VMSTATE_AUTO32_ARRAY(lbreg),
        VMSTATE_AUTO32_ARRAY(cycles),
        _VMSTATE_UINT32(uspreg),
        _VMSTATE_UINT32(seqstat),
        _VMSTATE_UINT32(syscfg),
        _VMSTATE_UINT32(reti),
        _VMSTATE_UINT32(retx),
        _VMSTATE_UINT32(retn),
        _VMSTATE_UINT32(rete),
        _VMSTATE_UINT32(emudat),
        _VMSTATE_UINT32(pc),
        VMSTATE_AUTO32_ARRAY(astat),
        _VMSTATE_UINT32(astat_op),
        VMSTATE_AUTO32_ARRAY(astat_arg),
        VMSTATE_END_OF_LIST()
    }
};
