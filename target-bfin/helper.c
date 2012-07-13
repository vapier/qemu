/*
 * Blackfin helpers
 *
 * Copyright 2007-2014 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <assert.h>

#include "config.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "qemu/host-utils.h"

#if defined(CONFIG_USER_ONLY)

void bfin_cpu_do_interrupt(CPUState *cs)
{
    cs->exception_index = -1;
}

int cpu_handle_mmu_fault(CPUState *cs, target_ulong address, int rw,
                         int mmu_idx)
{
    cs->exception_index = EXCP_DCPLB_VIOLATE;
    return 1;
}

#else

void bfin_cpu_do_interrupt(CPUState *cs)
{
    BlackfinCPU *cpu = BFIN_CPU(cs);
    CPUBfinState *env = &cpu->env;

    qemu_log_mask(CPU_LOG_INT,
        "exception at pc=%x type=%x\n", env->pc, env->exception_index);

    switch (env->exception_index) {
    default:
        cpu_abort(env, "unhandled exception type=%d\n",
                  env->exception_index);
        break;
    }
}

hwaddr bfin_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    return addr & TARGET_PAGE_MASK;
}

int cpu_handle_mmu_fault(CPUArchState *env, target_ulong address, int rw,
                         int mmu_idx)
{
    int prot;

    address &= TARGET_PAGE_MASK;
    prot = PAGE_BITS;
    tlb_set_page(env, address, address, prot, mmu_idx, TARGET_PAGE_SIZE);

    return 0;
}

#endif
