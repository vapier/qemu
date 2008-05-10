/*
 * Blackfin execution defines
 *
 * Copyright 2007 Mike Frysinger
 * Copyright 2007 Analog Devices, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef EXEC_BFIN_H
#define EXEC_BFIN_H

#include "config.h"
#include "dyngen-exec.h"

register struct CPUBFINState *env asm(AREG0);

#include "cpu.h"
#include "exec-all.h"

#ifndef CONFIG_USER_ONLY
#include "softmmu_exec.h"
#endif

#define RETURN() __asm__ __volatile__("")

static inline void regs_to_env(void)
{
}

static inline void env_to_regs(void)
{
}

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
