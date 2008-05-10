/*
 * Blackfin specific CPU ABI and functions for linux-user
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#ifndef TARGET_CPU_H
#define TARGET_CPU_H

static inline void cpu_clone_regs(CPUArchState *env, target_ulong newsp)
{
    if (newsp)
        env->spreg = newsp;
}

static inline void cpu_set_tls(CPUArchState *env, target_ulong newtls)
{
    /* Blackfin does not do TLS currently. */
}

#endif
