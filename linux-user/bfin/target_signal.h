/*
 * Blackfin signal defines.
 *
 * Copyright 2007-2018 Mike Frysinger
 * Copyright 2003-2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2
 */

#ifndef BFIN_TARGET_SIGNAL_H
#define BFIN_TARGET_SIGNAL_H

#include "cpu.h"

/* this struct defines a stack used during syscall handling */

typedef struct target_sigaltstack {
	abi_ulong ss_sp;
	abi_long ss_flags;
	abi_ulong ss_size;
} target_stack_t;


/*
 * sigaltstack controls
 */
#define TARGET_SS_ONSTACK	1
#define TARGET_SS_DISABLE	2

#define TARGET_MINSIGSTKSZ	2048
#define TARGET_SIGSTKSZ		8192

#include "../generic/signal.h"

static inline abi_ulong get_sp_from_cpustate(CPUArchState *env)
{
	return env->spreg;
}

#define TARGET_ARCH_HAS_SETUP_FRAME

#define TARGET_SIGRETURN_STUB	0x400

#endif
