/*
 * Blackfin helpers
 *
 * Copyright 2007 Mike Frysinger
 * Copyright 2007 Analog Devices, Inc.
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
#include "exec-all.h"
#include "host-utils.h"

#if defined(CONFIG_USER_ONLY)

void do_interrupt(CPUState *env)
{
	env->exception_index = -1;
}

int cpu_bfin_handle_mmu_fault(CPUState *env, target_ulong address, int rw,
                              int mmu_idx, int is_softmmu)
{
    env->exception_index = EXCP_DCPLB_VIOLATE;
//    fprintf(stderr, "fault at %#x : %#x (%s)\n", env->pc, address, rw ? "read" : "write");
//    cpu_dump_state(env, stderr, fprintf, 0);
    return 1;
}

#else

target_phys_addr_t cpu_get_phys_page_debug(CPUState *env, target_ulong addr)
{
	return addr;
}

#endif
