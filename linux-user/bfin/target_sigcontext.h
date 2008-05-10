/*
 * Copyright 2004-2008 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef TARGET_SIGCONTEXT_H
#define TARGET_SIGCONTEXT_H

/* Add new entries at the end of the structure only.  */
struct target_sigcontext {
	abi_ulong sc_r0;
	abi_ulong sc_r1;
	abi_ulong sc_r2;
	abi_ulong sc_r3;
	abi_ulong sc_r4;
	abi_ulong sc_r5;
	abi_ulong sc_r6;
	abi_ulong sc_r7;
	abi_ulong sc_p0;
	abi_ulong sc_p1;
	abi_ulong sc_p2;
	abi_ulong sc_p3;
	abi_ulong sc_p4;
	abi_ulong sc_p5;
	abi_ulong sc_usp;
	abi_ulong sc_a0w;
	abi_ulong sc_a1w;
	abi_ulong sc_a0x;
	abi_ulong sc_a1x;
	abi_ulong sc_astat;
	abi_ulong sc_rets;
	abi_ulong sc_pc;
	abi_ulong sc_retx;
	abi_ulong sc_fp;
	abi_ulong sc_i0;
	abi_ulong sc_i1;
	abi_ulong sc_i2;
	abi_ulong sc_i3;
	abi_ulong sc_m0;
	abi_ulong sc_m1;
	abi_ulong sc_m2;
	abi_ulong sc_m3;
	abi_ulong sc_l0;
	abi_ulong sc_l1;
	abi_ulong sc_l2;
	abi_ulong sc_l3;
	abi_ulong sc_b0;
	abi_ulong sc_b1;
	abi_ulong sc_b2;
	abi_ulong sc_b3;
	abi_ulong sc_lc0;
	abi_ulong sc_lc1;
	abi_ulong sc_lt0;
	abi_ulong sc_lt1;
	abi_ulong sc_lb0;
	abi_ulong sc_lb1;
	abi_ulong sc_seqstat;
};

#endif
