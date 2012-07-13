/*
 * QEMU Blackfin CPU
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "monitor/monitor.h"
#include "monitor/hmp-target.h"
#include "hmp.h"

const MonitorDef monitor_defs[] = {
#define _REG(name, var) { name, offsetof(CPUArchState, var) }
#define REG(name) _REG(#name, name)
#define _REG_ARR(name, idx, arr) _REG(#name #idx, arr[idx])
#define REG_ARR(name, idx) _REG_ARR(name, idx, name##reg)
    _REG_ARR(r, 0, dreg),
    _REG_ARR(r, 1, dreg),
    _REG_ARR(r, 2, dreg),
    _REG_ARR(r, 3, dreg),
    _REG_ARR(r, 4, dreg),
    _REG_ARR(r, 5, dreg),
    _REG_ARR(r, 6, dreg),
    _REG_ARR(r, 7, dreg),
    REG_ARR(p, 0),
    REG_ARR(p, 1),
    REG_ARR(p, 2),
    REG_ARR(p, 3),
    REG_ARR(p, 4),
    REG_ARR(p, 5),
    REG_ARR(i, 0),
    REG_ARR(i, 1),
    REG_ARR(i, 2),
    REG_ARR(i, 3),
    REG_ARR(m, 0),
    REG_ARR(m, 1),
    REG_ARR(m, 2),
    REG_ARR(m, 3),
    REG_ARR(b, 0),
    REG_ARR(b, 1),
    REG_ARR(b, 2),
    REG_ARR(b, 3),
    REG_ARR(l, 0),
    REG_ARR(l, 1),
    REG_ARR(l, 2),
    REG_ARR(l, 3),
    REG(rets),
    REG_ARR(lc, 0),
    REG_ARR(lc, 1),
    REG_ARR(lt, 0),
    REG_ARR(lt, 1),
    REG_ARR(lb, 0),
    REG_ARR(lb, 1),
    _REG_ARR(cycles, 0, cycles),
    _REG_ARR(cycles, 1, cycles),
    _REG("usp", uspreg),
    _REG("fp", fpreg),
    _REG("sp", spreg),
    REG(seqstat),
    REG(syscfg),
    REG(reti),
    REG(retx),
    REG(retn),
    REG(rete),
    REG(emudat),
    REG(pc),
#undef REG_ARR
#undef _REG_ARR
#undef REG
#undef _REG
};

const MonitorDef *target_monitor_defs(void)
{
    return monitor_defs;
}
