/*
 * Blackfin emulation
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#ifndef CPU_BFIN_H
#define CPU_BFIN_H

struct DisasContext;

#define TARGET_LONG_BITS 32

#define ELF_MACHINE EM_BLACKFIN

#define CPUArchState struct CPUBfinState

#include "qemu-common.h"
#include "exec/cpu-defs.h"

#define TARGET_HAS_ICE 1

/* These exceptions correspond directly to hardware levels.  */
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
/* These are QEMU internal ones (must be larger than 0xFF).  */
#define EXCP_ABORT          0x100
#define EXCP_DBGA           0x101
#define EXCP_OUTC           0x102
#define EXCP_FIXED_CODE     0x103

#define CPU_INTERRUPT_NMI   CPU_INTERRUPT_TGT_EXT_1

#define BFIN_L1_CACHE_BYTES 32

/* Blackfin does 1K/4K/1M/4M, but for now only support 4k */
#define TARGET_PAGE_BITS    12
#define NB_MMU_MODES        2

#define TARGET_PHYS_ADDR_SPACE_BITS 32
#define TARGET_VIRT_ADDR_SPACE_BITS 32

/* Indexes into astat array; matches bitpos in hardware too */
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
    ASTAT_AV0S,
    ASTAT_AV1,
    ASTAT_AV1S,
    ASTAT_V = 24,
    ASTAT_VS
};

typedef struct CPUBfinState {
    int personality;

    uint32_t dreg[8];
    uint32_t preg[8];
    uint32_t ireg[4];
    uint32_t mreg[4];
    uint32_t breg[4];
    uint32_t lreg[4];
    uint64_t areg[2];
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
    uint32_t astat_op, astat_arg[3];

    CPU_COMMON
} CPUBfinState;
#define spreg preg[6]
#define fpreg preg[7]

#include "qom/cpu.h"

#define TYPE_BLACKFIN_CPU "bfin-cpu"

#define BFIN_CPU_CLASS(klass) \
    OBJECT_CLASS_CHECK(BlackfinCPUClass, (klass), TYPE_BLACKFIN_CPU)
#define BFIN_CPU(obj) \
    OBJECT_CHECK(BlackfinCPU, (obj), TYPE_BLACKFIN_CPU)
#define BFIN_CPU_GET_CLASS(obj) \
    OBJECT_GET_CLASS(BlackfinCPUClass, (obj), TYPE_BLACKFIN_CPU)

/**
 * BlackfinCPUClass:
 * @parent_reset: The parent class' reset handler.
 *
 * A Blackfin CPU model.
 */
typedef struct BlackfinCPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    DeviceRealize parent_realize;
    void (*parent_reset)(CPUState *cpu);

    const char *name;
} BlackfinCPUClass;

/**
 * BlackfinCPU:
 * @env: #CPUArchState
 *
 * A Blackfin CPU.
 */
typedef struct BlackfinCPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUArchState env;
} BlackfinCPU;

static inline BlackfinCPU *bfin_env_get_cpu(CPUArchState *env)
{
    return container_of(env, BlackfinCPU, env);
}

#define ENV_GET_CPU(e) CPU(bfin_env_get_cpu(e))

#define ENV_OFFSET offsetof(BlackfinCPU, env)

#ifndef CONFIG_USER_ONLY
extern const struct VMStateDescription vmstate_bfin_cpu;
#endif

static inline BlackfinCPU *cpu_bfin_init(const char *cpu_model)
{
    return BFIN_CPU(cpu_generic_init(TYPE_BLACKFIN_CPU, cpu_model));
}

#define cpu_init(cpu_model) CPU(cpu_bfin_init(cpu_model))

#define cpu_list cpu_bfin_list
#define cpu_signal_handler cpu_bfin_signal_handler

void bfin_cpu_do_interrupt(CPUState *cpu);

void bfin_cpu_dump_state(CPUState *cs, FILE *f, fprintf_function cpu_fprintf,
                         int flags);
hwaddr bfin_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
int bfin_cpu_gdb_read_register(CPUState *cpu, uint8_t *buf, int reg);
int bfin_cpu_gdb_write_register(CPUState *cpu, uint8_t *buf, int reg);

static inline uint32_t bfin_astat_read(CPUArchState *env)
{
    unsigned int i, ret;

    ret = 0;
    for (i = 0; i < 32; ++i) {
        ret |= (env->astat[i] << i);
    }

    return ret;
}

static inline void bfin_astat_write(CPUArchState *env, uint32_t astat)
{
    unsigned int i;
    for (i = 0; i < 32; ++i) {
        env->astat[i] = (astat >> i) & 1;
    }
}

enum astat_ops {
    ASTAT_OP_NONE,
    ASTAT_OP_DYNAMIC,
    ASTAT_OP_ABS,
    ASTAT_OP_ABS_VECTOR,
    ASTAT_OP_ADD16,
    ASTAT_OP_ADD32,
    ASTAT_OP_ASHIFT16,
    ASTAT_OP_ASHIFT32,
    ASTAT_OP_COMPARE_SIGNED,
    ASTAT_OP_COMPARE_UNSIGNED,
    ASTAT_OP_LOGICAL,
    ASTAT_OP_LSHIFT16,
    ASTAT_OP_LSHIFT32,
    ASTAT_OP_LSHIFT_RT16,
    ASTAT_OP_LSHIFT_RT32,
    ASTAT_OP_MIN_MAX,
    ASTAT_OP_MIN_MAX_VECTOR,
    ASTAT_OP_NEGATE,
    ASTAT_OP_SUB16,
    ASTAT_OP_SUB32,
    ASTAT_OP_VECTOR_ADD_ADD,    /* +|+ */
    ASTAT_OP_VECTOR_ADD_SUB,    /* +|- */
    ASTAT_OP_VECTOR_SUB_SUB,    /* -|- */
    ASTAT_OP_VECTOR_SUB_ADD,    /* -|+ */
};

void cpu_list(FILE *f, fprintf_function cpu_fprintf);
int cpu_bfin_signal_handler(int host_signum, void *pinfo, void *puc);
void bfin_translate_init(void);

extern const char * const greg_names[];
extern const char *get_allreg_name(int grp, int reg);

#include "dv-bfin_cec.h"

/* */
#define MMU_MODE0_SUFFIX _kernel
#define MMU_MODE1_SUFFIX _user
#define MMU_USER_IDX 1
static inline int cpu_mmu_index(CPUArchState *env, bool ifetch)
{
    return !cec_is_supervisor_mode(env);
}

int cpu_bfin_handle_mmu_fault(CPUState *cs, target_ulong address,
                              MMUAccessType access_type, int mmu_idx);
#define cpu_handle_mmu_fault cpu_bfin_handle_mmu_fault

#include "exec/cpu-all.h"

#include "exec/exec-all.h"

static inline void cpu_pc_from_tb(CPUArchState *env, TranslationBlock *tb)
{
    env->pc = tb->pc;
}

static inline void cpu_get_tb_cpu_state(CPUArchState *env, target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    *pc = env->pc;
    *cs_base = 0;
    *flags = env->astat[ASTAT_RND_MOD];
}

#endif
