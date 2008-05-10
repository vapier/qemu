/*
 * Emulation of Linux signals
 *
 * Copyright 2018 Mike Frysinger
 *
 * Licensed under the GPL-2
 */

#include "qemu/osdep.h"
#include "qemu.h"
#include "target_sigcontext.h"
#include "target_signal.h"
#include "target_ucontext.h"
#include "signal-common.h"
#include "linux-user/trace.h"

struct target_rt_sigframe {
    int32_t sig;
    abi_ulong pinfo;
    abi_ulong puc;
    /* This is no longer needed by the kernel, but unfortunately userspace
     * code expects it to be there.  */
    char retcode[8];
    target_siginfo_t info;
    struct target_ucontext uc;
};

struct fdpic_func_descriptor {
    abi_ulong text;
    abi_ulong GOT;
};

#define rreg dreg

static inline void
target_rt_restore_sigcontext(CPUArchState *env, struct target_sigcontext *sc, long *ret)
{
    uint32_t reg;

#define RESTORE2(e, x) __get_user(env->e, &sc->sc_##x)
#define RESTOREA(r, i) RESTORE2(r##reg[i], r##i)
#define RESTORE(x) RESTORE2(x, x)

    /* restore passed registers */
    RESTOREA(r, 0); RESTOREA(r, 1); RESTOREA(r, 2); RESTOREA(r, 3);
    RESTOREA(r, 4); RESTOREA(r, 5); RESTOREA(r, 6); RESTOREA(r, 7);
    RESTOREA(p, 0); RESTOREA(p, 1); RESTOREA(p, 2); RESTOREA(p, 3);
    RESTOREA(p, 4); RESTOREA(p, 5);
    RESTORE2(spreg, usp);
    __get_user(reg, &sc->sc_a0x);
    env->areg[0] = reg;
    __get_user(reg, &sc->sc_a0w);
    env->areg[0] = (env->areg[0] << 32) | reg;
    __get_user(reg, &sc->sc_a1x);
    env->areg[1] = reg;
    __get_user(reg, &sc->sc_a1w);
    env->areg[1] = (env->areg[1] << 32) | reg;
    __get_user(reg, &sc->sc_astat);
    bfin_astat_write(env, reg);
    RESTORE(rets);
    RESTORE(pc);
    RESTORE(retx);
    RESTORE2(fpreg, fp);
    RESTOREA(i, 0); RESTOREA(i, 1); RESTOREA(i, 2); RESTOREA(i, 3);
    RESTOREA(m, 0); RESTOREA(m, 1); RESTOREA(m, 2); RESTOREA(m, 3);
    RESTOREA(l, 0); RESTOREA(l, 1); RESTOREA(l, 2); RESTOREA(l, 3);
    RESTOREA(b, 0); RESTOREA(b, 1); RESTOREA(b, 2); RESTOREA(b, 3);
    RESTOREA(lc, 0); RESTOREA(lc, 1);
    RESTOREA(lt, 0); RESTOREA(lt, 1);
    RESTOREA(lb, 0); RESTOREA(lb, 1);
    RESTORE(seqstat);

    *ret = env->dreg[0];
}

static inline void
target_rt_setup_sigcontext(struct target_sigcontext *sc, CPUArchState *env)
{
#define SETUP2(e, x) __put_user(env->e, &sc->sc_##x)
#define SETUPA(r, i) SETUP2(r##reg[i], r##i)
#define SETUP(x) SETUP2(x, x)

    SETUPA(r, 0); SETUPA(r, 1); SETUPA(r, 2); SETUPA(r, 3);
    SETUPA(r, 4); SETUPA(r, 5); SETUPA(r, 6); SETUPA(r, 7);
    SETUPA(p, 0); SETUPA(p, 1); SETUPA(p, 2); SETUPA(p, 3);
    SETUPA(p, 4); SETUPA(p, 5);
    SETUP2(spreg, usp);
    __put_user((uint32_t)env->areg[0], &sc->sc_a0w);
    __put_user((uint32_t)env->areg[1], &sc->sc_a1w);
    __put_user((uint32_t)(env->areg[0] >> 32), &sc->sc_a0x);
    __put_user((uint32_t)(env->areg[1] >> 32), &sc->sc_a1x);
    __put_user(bfin_astat_read(env), &sc->sc_astat);
    SETUP(rets);
    SETUP(pc);
    SETUP(retx);
    SETUP2(fpreg, fp);
    SETUPA(i, 0); SETUPA(i, 1); SETUPA(i, 2); SETUPA(i, 3);
    SETUPA(m, 0); SETUPA(m, 1); SETUPA(m, 2); SETUPA(m, 3);
    SETUPA(l, 0); SETUPA(l, 1); SETUPA(l, 2); SETUPA(l, 3);
    SETUPA(b, 0); SETUPA(b, 1); SETUPA(b, 2); SETUPA(b, 3);
    SETUPA(lc, 0); SETUPA(lc, 1);
    SETUPA(lt, 0); SETUPA(lt, 1);
    SETUPA(lb, 0); SETUPA(lb, 1);
    SETUP(seqstat);
}

#undef rreg

static inline abi_ulong
get_sigframe(struct target_sigaction *ka, CPUArchState *env, size_t frame_size)
{
    abi_ulong usp;

    /* Default to using normal stack.  */
    usp = env->spreg;

    /* This is the X/Open sanctioned signal stack switching.  */
    if ((ka->sa_flags & TARGET_SA_ONSTACK) && (sas_ss_flags(usp) == 0)) {
        usp = target_sigaltstack_used.ss_sp + target_sigaltstack_used.ss_size;
    }

    return ((usp - frame_size) & -8UL);
}

void setup_rt_frame(int sig, struct target_sigaction *ka,
                    target_siginfo_t *info,
                    target_sigset_t *set, CPUArchState *env)
{
    struct target_rt_sigframe *frame;
    abi_ulong frame_addr;
    abi_ulong info_addr;
    abi_ulong uc_addr;
    int i;

    frame_addr = get_sigframe(ka, env, sizeof(*frame));
    trace_user_setup_rt_frame(env, frame_addr);
    if (!lock_user_struct(VERIFY_WRITE, frame, frame_addr, 0))
        goto give_sigsegv;

    __put_user(sig, &frame->sig);

    info_addr = frame_addr + offsetof(struct target_rt_sigframe, info);
    __put_user(info_addr, &frame->pinfo);
    uc_addr = frame_addr + offsetof(struct target_rt_sigframe, uc);
    __put_user(uc_addr, &frame->puc);
    tswap_siginfo(&frame->info, info);

    /* Create the ucontext.  */
    __put_user(0, &frame->uc.tuc_flags);
    __put_user(0, &frame->uc.tuc_link);
    __put_user(target_sigaltstack_used.ss_sp, &frame->uc.tuc_stack.ss_sp);
    __put_user(sas_ss_flags(env->spreg), &frame->uc.tuc_stack.ss_flags);
    __put_user(target_sigaltstack_used.ss_size, &frame->uc.tuc_stack.ss_size);
    target_rt_setup_sigcontext(&frame->uc.tuc_mcontext, env);

    for (i = 0; i < TARGET_NSIG_WORDS; i++) {
        __put_user(set->sig[i], &frame->uc.tuc_sigmask.sig[i]);
    }

    /* Set up registers for signal handler */
    env->spreg = frame_addr;
    if (env->personality & 0x0080000/*FDPIC_FUNCPTRS*/) {
        struct fdpic_func_descriptor *funcptr;
        if (!lock_user_struct(VERIFY_READ, funcptr, ka->_sa_handler, 1))
            goto give_sigsegv;
        __get_user(env->pc, &funcptr->text);
        __get_user(env->preg[3], &funcptr->GOT);
        unlock_user_struct(funcptr, ka->_sa_handler, 0);
    } else {
        env->pc = ka->_sa_handler;
    }
    env->rets = TARGET_SIGRETURN_STUB;

    env->dreg[0] = frame->sig;
    env->dreg[1] = info_addr;
    env->dreg[2] = uc_addr;

    unlock_user_struct(frame, frame_addr, 1);
    return;

 give_sigsegv:
    unlock_user_struct(frame, frame_addr, 1);
    force_sigsegv(sig);
}

void setup_frame(int sig, struct target_sigaction *ka,
                 target_sigset_t *set, CPUArchState *env)
{
    target_siginfo_t info;
    setup_rt_frame(sig, ka, &info, set, env);
}

long do_sigreturn(CPUArchState *env)
{
    trace_user_do_sigreturn(env, env->spreg);
    fprintf(stderr, "do_sigreturn: not implemented\n");
    return -TARGET_ENOSYS;
}

/* NB: This version should work for any arch ... */
long do_rt_sigreturn(CPUArchState *env)
{
    long ret;
    abi_ulong frame_addr = env->spreg;
    struct target_rt_sigframe *frame;
    sigset_t host_set;

    trace_user_do_rt_sigreturn(env, frame_addr);

    if (!lock_user_struct(VERIFY_READ, frame, frame_addr, 1))
        goto badframe;

    target_to_host_sigset(&host_set, &frame->uc.tuc_sigmask);
    sigprocmask(SIG_SETMASK, &host_set, NULL);

    target_rt_restore_sigcontext(env, &frame->uc.tuc_mcontext, &ret);

    if (do_sigaltstack(frame_addr + offsetof(struct target_rt_sigframe,
                                             uc.tuc_stack),
                       0, get_sp_from_cpustate(env)) == -EFAULT) {
        goto badframe;
    }

    unlock_user_struct(frame, frame_addr, 0);
    return ret;

 badframe:
    unlock_user_struct(frame, frame_addr, 0);
    force_sig(TARGET_SIGSEGV);
    return 0;
}
