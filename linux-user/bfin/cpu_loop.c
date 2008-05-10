/*
 * qemu user cpu loop
 *
 * Copyright 2018 Mike Frysinger
 *
 * Licensed under the GPL-2
 */

#include "qemu/osdep.h"
#include "qemu.h"
#include "cpu_loop-common.h"
#include "disas/disas.h"

void cpu_loop(CPUArchState *env)
{
    CPUState *cs = ENV_GET_CPU(env);
    int trapnr, gdbsig;
    target_siginfo_t info;

    for (;;) {
        cpu_exec_start(cs);
        trapnr = cpu_exec(cs);
        cpu_exec_end(cs);
        gdbsig = 0;

        switch (trapnr) {
        case EXCP_SYSCALL:
            env->pc += 2;
            env->dreg[0] = do_syscall(env,
                                      env->preg[0],
                                      env->dreg[0],
                                      env->dreg[1],
                                      env->dreg[2],
                                      env->dreg[3],
                                      env->dreg[4],
                                      env->dreg[5], 0, 0);
            break;
        case EXCP_INTERRUPT:
            /* just indicate that signals should be handled asap */
            break;
        case EXCP_DEBUG:
            /* XXX: does this handle hwloops ? */
            /*env->pc += 2;*/
            /* EMUEXCPT signals debugger only if attached; NOP otherwise */
            gdbsig = TARGET_SIGTRAP;
            break;
        case EXCP_SOFT_BP:
            {
                int sig = gdb_handlesig(cs, TARGET_SIGTRAP);
                if (sig) {
                    info.si_signo = sig;
                    info.si_errno = 0;
                    info.si_code = TARGET_TRAP_BRKPT;
                    queue_signal(env, info.si_signo, QEMU_SI_FAULT, &info);
                }
            }
            break;
        case EXCP_HLT:
            do_syscall(env, TARGET_NR_exit, 0, 0, 0, 0, 0, 0, 0, 0);
            break;
        case EXCP_ABORT:
            do_syscall(env, TARGET_NR_exit, 1, 0, 0, 0, 0, 0, 0, 0);
            break;
        case EXCP_DBGA:
            fprintf(stderr, "qemu: DBGA failed\n");
            cpu_dump_state(cs, stderr, fprintf, 0);
            gdbsig = TARGET_SIGABRT;
            break;
        case EXCP_UNDEF_INST:
            fprintf(stderr, "qemu: unhandled insn @ %#x\n", env->pc);
            log_target_disas(cs, env->pc, 8);
            gdbsig = TARGET_SIGILL;
            break;
        case EXCP_DCPLB_VIOLATE:
            fprintf(stderr, "qemu: memory violation @ %#x\n", env->pc);
            log_target_disas(cs, env->pc, 8);
            cpu_dump_state(cs, stderr, fprintf, 0);
            gdbsig = TARGET_SIGSEGV;
            break;
        case EXCP_ILL_SUPV:
            fprintf(stderr, "qemu: supervisor mode required @ %#x\n", env->pc);
            log_target_disas(cs, env->pc, 8);
            gdbsig = TARGET_SIGILL;
            break;
        case EXCP_MISALIG_INST:
            fprintf(stderr, "qemu: unaligned insn fetch @ %#x\n", env->pc);
            log_target_disas(cs, env->pc, 8);
            cpu_dump_state(cs, stderr, fprintf, 0);
            gdbsig = TARGET_SIGSEGV;
            break;
        case EXCP_DATA_MISALGIN:
            fprintf(stderr, "qemu: unaligned data fetch @ %#x\n", env->pc);
            log_target_disas(cs, env->pc, 8);
            cpu_dump_state(cs, stderr, fprintf, 0);
            gdbsig = TARGET_SIGSEGV;
            break;
        default:
            fprintf(stderr, "qemu: unhandled CPU exception %#x - aborting\n",
                    trapnr);
            cpu_dump_state(cs, stderr, fprintf, 0);
            gdbsig = TARGET_SIGILL;
            break;
        }

        if (gdbsig) {
            gdb_handlesig(cs, gdbsig);
            /* XXX: should we let people continue if gdb handles the signal ? */
            if (gdbsig != TARGET_SIGTRAP) {
                exit(1);
            }
        }

        process_pending_signals(env);
    }
}

void target_cpu_copy_regs(CPUArchState *env, struct target_pt_regs *regs)
{
    CPUState *cpu = ENV_GET_CPU(env);
    TaskState *ts = cpu->opaque;
    struct image_info *info = ts->info;

    env->personality = info->personality;
    env->dreg[0] = regs->r0;
    env->dreg[1] = regs->r1;
    env->dreg[2] = regs->r2;
    env->dreg[3] = regs->r3;
    env->dreg[4] = regs->r4;
    env->dreg[5] = regs->r5;
    env->dreg[6] = regs->r6;
    env->dreg[7] = regs->r7;
    env->preg[0] = regs->p0;
    env->preg[1] = regs->p1;
    env->preg[2] = regs->p2;
    env->preg[3] = regs->p3;
    env->preg[4] = regs->p4;
    env->preg[5] = regs->p5;
    env->spreg = regs->usp;
    env->fpreg = regs->fp;
    env->uspreg = regs->usp;
    env->breg[0] = regs->b0;
    env->breg[1] = regs->b1;
    env->breg[2] = regs->b2;
    env->breg[3] = regs->b3;
    env->lreg[0] = regs->l0;
    env->lreg[1] = regs->l1;
    env->lreg[2] = regs->l2;
    env->lreg[3] = regs->l3;
    env->mreg[0] = regs->m0;
    env->mreg[1] = regs->m1;
    env->mreg[2] = regs->m2;
    env->mreg[3] = regs->m3;
    env->ireg[0] = regs->i0;
    env->ireg[1] = regs->i1;
    env->ireg[2] = regs->i2;
    env->ireg[3] = regs->i3;
    env->lcreg[0] = regs->lc0;
    env->ltreg[0] = regs->lt0;
    env->lbreg[0] = regs->lb0;
    env->lcreg[1] = regs->lc1;
    env->ltreg[1] = regs->lt1;
    env->lbreg[1] = regs->lb1;
    env->areg[0] = ((uint64_t)regs->a0x << 32) | regs->a0w;
    env->areg[1] = ((uint64_t)regs->a1x << 32) | regs->a1w;
    env->rets = regs->rets;
    env->rete = regs->rete;
    env->retn = regs->retn;
    env->retx = regs->retx;
    env->pc = regs->pc;
    bfin_astat_write(env, regs->astat);
}
