/*
 * Blackfin execution defines
 *
 * Copyright 2007-2011 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#ifndef EXEC_BFIN_H
#define EXEC_BFIN_H

#include "config.h"
#include "dyngen-exec.h"

register struct CPUBFINState *env asm(AREG0);

#include "cpu.h"
#include "exec-all.h"

static inline int cpu_has_work(CPUState *env)
{
    return (env->interrupt_request & (CPU_INTERRUPT_HARD | CPU_INTERRUPT_NMI));
}

static inline int cpu_halted(CPUState *env)
{
    if (!env->halted)
        return 0;
    if (env->interrupt_request & CPU_INTERRUPT_HARD) {
        env->halted = 0;
        return 0;
    }
    return EXCP_HALTED;
}

#endif
