/*
 * Blackfin helpers
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"

#include "cpu.h"
#include "exec/exec-all.h"
#include "qemu/host-utils.h"

#if defined(CONFIG_USER_ONLY)

void bfin_cpu_do_interrupt(CPUState *cs)
{
    cs->exception_index = -1;
}

int cpu_handle_mmu_fault(CPUState *cs, target_ulong address,
                         MMUAccessType access_type, int mmu_idx)
{
    cs->exception_index = EXCP_DCPLB_VIOLATE;
    return 1;
}

#endif
