/*
 * QEMU Blackfin CPU
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "exec/gdbstub.h"
#include "target-bfin/bfin-tdep.h"

int bfin_cpu_gdb_read_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    BlackfinCPU *cpu = BFIN_CPU(cs);
    CPUArchState *env = &cpu->env;
    uint32_t val;

    switch (n) {
    case BFIN_R0_REGNUM ... BFIN_R7_REGNUM:
        val = env->dreg[n - BFIN_R0_REGNUM];
        break;
    case BFIN_P0_REGNUM ... BFIN_FP_REGNUM:
        val = env->preg[n - BFIN_P0_REGNUM];
        break;
    case BFIN_I0_REGNUM ... BFIN_I3_REGNUM:
        val = env->ireg[n - BFIN_I0_REGNUM];
        break;
    case BFIN_M0_REGNUM ... BFIN_M3_REGNUM:
        val = env->mreg[n - BFIN_M0_REGNUM];
        break;
    case BFIN_B0_REGNUM ... BFIN_B3_REGNUM:
        val = env->breg[n - BFIN_B0_REGNUM];
        break;
    case BFIN_L0_REGNUM ... BFIN_L3_REGNUM:
        val = env->lreg[n - BFIN_L0_REGNUM];
        break;
    case BFIN_A0_DOT_X_REGNUM:
        val = (env->areg[0] >> 32) & 0xff;
        break;
    case BFIN_A0_DOT_W_REGNUM:
        val = env->areg[0];
        break;
    case BFIN_A1_DOT_X_REGNUM:
        val = (env->areg[1] >> 32) & 0xff;
        break;
    case BFIN_A1_DOT_W_REGNUM:
        val = env->areg[1];
        break;
    case BFIN_ASTAT_REGNUM:
        val = bfin_astat_read(env);
        break;
    case BFIN_RETS_REGNUM:
        val = env->rets;
        break;
    case BFIN_LC0_REGNUM:
        val = env->lcreg[0];
        break;
    case BFIN_LT0_REGNUM:
        val = env->ltreg[0];
        break;
    case BFIN_LB0_REGNUM:
        val = env->lbreg[0];
        break;
    case BFIN_LC1_REGNUM:
        val = env->lcreg[1];
        break;
    case BFIN_LT1_REGNUM:
        val = env->ltreg[1];
        break;
    case BFIN_LB1_REGNUM:
        val = env->lbreg[1];
        break;
    case BFIN_CYCLES_REGNUM ... BFIN_CYCLES2_REGNUM:
        val = env->cycles[n - BFIN_CYCLES_REGNUM];
        break;
    case BFIN_USP_REGNUM:
        val = env->uspreg;
        break;
    case BFIN_SEQSTAT_REGNUM:
        val = env->seqstat;
        break;
    case BFIN_SYSCFG_REGNUM:
        val = env->syscfg;
        break;
    case BFIN_RETI_REGNUM:
        val = env->reti;
        break;
    case BFIN_RETX_REGNUM:
        val = env->retx;
        break;
    case BFIN_RETN_REGNUM:
        val = env->retn;
        break;
    case BFIN_RETE_REGNUM:
        val = env->rete;
        break;
    case BFIN_PC_REGNUM:
        val = env->pc;
        break;
    default:
        return 0;
    }

    return gdb_get_regl(mem_buf, val);
}

int bfin_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    BlackfinCPU *cpu = BFIN_CPU(cs);
    CPUArchState *env = &cpu->env;
    target_ulong tmpl;
    int r = 4;

    tmpl = ldtul_p(mem_buf);

    switch (n) {
    case BFIN_R0_REGNUM ... BFIN_R7_REGNUM:
       env->dreg[n - BFIN_R0_REGNUM] = tmpl;
       break;
    case BFIN_P0_REGNUM ... BFIN_FP_REGNUM:
        env->preg[n - BFIN_P0_REGNUM] = tmpl;
        break;
    case BFIN_I0_REGNUM ... BFIN_I3_REGNUM:
        env->ireg[n - BFIN_I0_REGNUM] = tmpl;
        break;
    case BFIN_M0_REGNUM ... BFIN_M3_REGNUM:
        env->mreg[n - BFIN_M0_REGNUM] = tmpl;
        break;
    case BFIN_B0_REGNUM ... BFIN_B3_REGNUM:
        env->breg[n - BFIN_B0_REGNUM] = tmpl;
        break;
    case BFIN_L0_REGNUM ... BFIN_L3_REGNUM:
        env->lreg[n - BFIN_L0_REGNUM] = tmpl;
        break;
    case BFIN_A0_DOT_X_REGNUM:
        env->areg[0] = (env->areg[0] & 0xffffffff) | ((uint64_t)tmpl << 32);
        break;
    case BFIN_A0_DOT_W_REGNUM:
        env->areg[0] = (env->areg[0] & ~0xffffffff) | tmpl;
        break;
    case BFIN_A1_DOT_X_REGNUM:
        env->areg[1] = (env->areg[1] & 0xffffffff) | ((uint64_t)tmpl << 32);
        break;
    case BFIN_A1_DOT_W_REGNUM:
        env->areg[1] = (env->areg[1] & ~0xffffffff) | tmpl;
        break;
    case BFIN_ASTAT_REGNUM:
        bfin_astat_write(env, tmpl);
        break;
    case BFIN_RETS_REGNUM:
        env->rets = tmpl;
        break;
    case BFIN_LC0_REGNUM:
        env->lcreg[0] = tmpl;
        break;
    case BFIN_LT0_REGNUM:
        env->ltreg[0] = tmpl;
        break;
    case BFIN_LB0_REGNUM:
        env->lbreg[0] = tmpl;
        break;
    case BFIN_LC1_REGNUM:
        env->lcreg[1] = tmpl;
        break;
    case BFIN_LT1_REGNUM:
        env->ltreg[1] = tmpl;
        break;
    case BFIN_LB1_REGNUM:
        env->lbreg[1] = tmpl;
        break;
    case BFIN_CYCLES_REGNUM ... BFIN_CYCLES2_REGNUM:
        env->cycles[n - BFIN_CYCLES_REGNUM] = tmpl;
        break;
    case BFIN_USP_REGNUM:
        env->uspreg = tmpl;
        break;
    case BFIN_SEQSTAT_REGNUM:
        env->seqstat = tmpl;
        break;
    case BFIN_SYSCFG_REGNUM:
        env->syscfg = tmpl;
        break;
    case BFIN_RETI_REGNUM:
        env->reti = tmpl;
        break;
    case BFIN_RETX_REGNUM:
        env->retx = tmpl;
        break;
    case BFIN_RETN_REGNUM:
        env->retn = tmpl;
        break;
    case BFIN_RETE_REGNUM:
        env->rete = tmpl;
        break;
    case BFIN_PC_REGNUM:
        env->pc = tmpl;
        break;
    default:
        return 0;
    }

    return r;
}
