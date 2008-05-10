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
    // TODO: This should depend on access_type.
    cs->exception_index = EXCP_DCPLB_VIOLATE;
    return 1;
}

#endif

/* Try to fill the TLB and return an exception if error. If retaddr is
   NULL, it means that the function was called in C code (i.e. not
   from generated code or from helper.c) */
/* XXX: fix it to restore all registers */
bool bfin_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                       MMUAccessType access_type, int mmu_idx, bool probe,
                       uintptr_t retaddr)
{
#ifdef CONFIG_USER_ONLY
    // TODO: This should depend on access_type.
    cs->exception_index = EXCP_DCPLB_VIOLATE;
    cpu_loop_exit_restore(cs, retaddr);
#else

    int ret;

    ret = cpu_bfin_handle_mmu_fault(cs, address, access_type, mmu_idx);
    if (likely(!ret)) {
        return true;
    } else if (probe) {
        return false;
    } else {
        /* now we have a real cpu fault */
        cpu_restore_state(cs, retaddr, true);
        cpu_loop_exit(cs);
    }
#endif
}
