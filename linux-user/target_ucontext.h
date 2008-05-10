/* This is the asm-generic/ucontext.h version */

#ifndef TARGET_UCONTEXT_H
#define TARGET_UCONTEXT_H

struct target_ucontext {
    abi_ulong                tuc_flags;
    abi_ulong                tuc_link;
    target_stack_t           tuc_stack;
    struct target_sigcontext tuc_mcontext;
    target_sigset_t          tuc_sigmask;    /* mask last for extensibility */
};

#endif
