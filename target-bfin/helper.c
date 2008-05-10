/*
 * Blackfin helpers
 *
 * Copyright 2007-2011 Mike Frysinger
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
#include "exec-all.h"
#include "host-utils.h"

#if defined(CONFIG_USER_ONLY)

void do_interrupt(CPUState *env)
{
    env->exception_index = -1;
}

int cpu_bfin_handle_mmu_fault(CPUState *env, target_ulong address, int rw,
                              int mmu_idx)
{
    env->exception_index = EXCP_DCPLB_VIOLATE;
    return 1;
}

#endif
