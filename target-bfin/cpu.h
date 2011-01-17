/*
 * Blackfin emulation
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

#ifndef CPU_BFIN_H
#define CPU_BFIN_H

struct DisasContext;

#define TARGET_LONG_BITS 32

#include "cpu-defs.h"

#define TARGET_HAS_ICE 1

#define ELF_MACHINE	EM_BLACKFIN

#define EXCP_SYSCALL        0
#define EXCP_SOFT_BP        1
#define EXCP_STACK_OVERFLOW 3
#define EXCP_SINGLE_STEP    0x10
#define EXCP_TRACE_FULL     0x11
#define EXCP_UNDEF_INST     0x21
#define EXCP_ILL_INST       0x22
#define EXCP_DCPLB_VIOLATE  0x23
#define EXCP_DATA_MISALGIN  0x24
#define EXCP_UNRECOVERABLE  0x25
#define EXCP_DCPLB_MISS     0x26
#define EXCP_DCPLB_MULT     0x27
#define EXCP_EMU_WATCH      0x28
#define EXCP_MISALIG_INST   0x2a
#define EXCP_ICPLB_PROT     0x2b
#define EXCP_ICPLB_MISS     0x2c
#define EXCP_ICPLB_MULT     0x2d
#define EXCP_ILL_SUPV       0x2e
#define EXCP_ABORT          0x100
#define EXCP_DBGA           0x101
#define EXCP_OUTC           0x102

/* Blackfin does 1K/4K/1M/4M, but for now only support 4k */
#define TARGET_PAGE_BITS 12
#define NB_MMU_MODES 2

#define TARGET_PHYS_ADDR_SPACE_BITS 32
#define TARGET_VIRT_ADDR_SPACE_BITS 32

#define CPUState struct CPUBFINState
#define cpu_init cpu_bfin_init
#define cpu_exec cpu_bfin_exec
#define cpu_gen_code cpu_bfin_gen_code
#define cpu_signal_handler cpu_bfin_signal_handler

enum {
	ASTAT_AZ = 0,
	ASTAT_AN,
	ASTAT_AC0_COPY,
	ASTAT_V_COPY,
	ASTAT_CC = 5,
	ASTAT_AQ,
	ASTAT_RND_MOD = 8,
	ASTAT_AC0 = 12,
	ASTAT_AC1,
	ASTAT_AV0 = 16,
	ASTAT_AVOS,
	ASTAT_AV1,
	ASTAT_AVS1,
	ASTAT_V = 24,
	ASTAT_VS
};

typedef struct CPUBFINState {
	CPU_COMMON
	uint32_t dreg[8];
	uint32_t preg[8];
	uint32_t ireg[4];
	uint32_t mreg[4];
	uint32_t breg[4];
	uint32_t lreg[4];
	uint32_t axreg[2], awreg[2];
	uint32_t rets;
	uint32_t lcreg[2], ltreg[2], lbreg[2];
	uint32_t cycles[2];
	uint32_t uspreg;
	uint32_t seqstat;
	uint32_t syscfg;
	uint32_t reti;
	uint32_t retx;
	uint32_t retn;
	uint32_t rete;
	uint32_t emudat;
	uint32_t pc;

	/* ASTAT bits; broken up for speeeeeeeed */
	uint32_t astat[32];
	/* ASTAT delayed helpers */
	uint32_t astat_op, astat_src[2];
} CPUBFINState;
#define spreg preg[6]
#define fpreg preg[7]

static inline uint32_t bfin_astat_read(CPUState *env)
{
	unsigned int i, ret;

	ret = 0;
	for (i = 0; i < 32; ++i)
		ret |= (env->astat[i] << i);

	return ret;
}

static inline void bfin_astat_write(CPUState *env, uint32_t astat)
{
	unsigned int i;
	for (i = 0; i < 32; ++i)
		env->astat[i] = !!(astat & (1 << i));
}

enum astat_ops {
	ASTAT_OP_UNKNOWN,

	ASTAT_OP_ADD,
};

typedef void (*hwloop_callback)(struct DisasContext *dc, int loop);

typedef struct DisasContext {
	CPUState *env;
	struct TranslationBlock *tb;
	/* The current PC we're decoding (could be middle of parallel insn) */
	target_ulong pc;
	/* Length of current insn (2/4/8) */
	target_ulong insn_len;

	enum astat_ops astat_op;
	hwloop_callback hwloop_callback;
	void *hwloop_data;
	int did_hwloop;
	int is_jmp;
	int mem_idx;
} DisasContext;

void do_interrupt(CPUState *env);
CPUState *cpu_init(const char *cpu_model);
int cpu_exec(CPUState *s);
int cpu_bfin_signal_handler(int host_signum, void *pinfo, void *puc);
void bfin_cpu_list(FILE *f, int (*cpu_fprintf)(FILE *f, const char *fmt, ...));

#define MMU_MODE0_SUFFIX _kernel
#define MMU_MODE1_SUFFIX _user
#define MMU_KERNEL_IDX  0
#define MMU_USER_IDX    1

#ifndef CONFIG_USER_ONLY
static inline int cpu_mmu_index (CPUState *env)
{
	if (1) //=env->ipend)
		return MMU_KERNEL_IDX;
	else
		return MMU_USER_IDX;
#if 0
        /* Are we in nommu mode?.  */
        if (!(env->sregs[SR_MSR] & MSR_VM))
            return MMU_NOMMU_IDX;

	if (env->sregs[SR_MSR] & MSR_UM)
            return MMU_USER_IDX;
        return MMU_KERNEL_IDX;
#endif
}
#endif

int cpu_bfin_handle_mmu_fault(CPUState *env, target_ulong address, int rw,
                              int mmu_idx, int is_softmmu);
#define cpu_handle_mmu_fault cpu_bfin_handle_mmu_fault

#if defined(CONFIG_USER_ONLY)
static inline void cpu_clone_regs(CPUState *env, target_ulong newsp)
{
	if (newsp)
		env->spreg = newsp;
	//SET_DREG (0, 0);
}
#endif

static inline void cpu_set_tls(CPUState *env, target_ulong newtls)
{
}

static inline int cpu_interrupts_enabled(CPUState *env)
{
	return 1; //env->[SR_MSR] & MSR_IE;
}

#include "cpu-all.h"
#include "exec-all.h"

static inline void cpu_pc_from_tb(CPUState *env, TranslationBlock *tb)
{
	env->pc = tb->pc;
}

static inline target_ulong cpu_get_pc(CPUState *env)
{
	return env->pc;
}

static inline void cpu_get_tb_cpu_state(CPUState *env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
	*pc = env->pc;
	*cs_base = 0;
	//env->iflags |= 0; //env->sregs[SR_MSR] & MSR_EE;
	*flags = 1; //env->iflags & 1; //IFLAGS_TB_MASK;
}

#endif
