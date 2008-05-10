struct target_pt_regs {
	abi_ulong orig_pc;
	abi_ulong ipend;
	abi_ulong seqstat;
	abi_ulong rete;
	abi_ulong retn;
	abi_ulong retx;
	abi_ulong pc;		/* PC == RETI */
	abi_ulong rets;
	abi_ulong reserved;		/* Used as scratch during system calls */
	abi_ulong astat;
	abi_ulong lb1;
	abi_ulong lb0;
	abi_ulong lt1;
	abi_ulong lt0;
	abi_ulong lc1;
	abi_ulong lc0;
	abi_ulong a1w;
	abi_ulong a1x;
	abi_ulong a0w;
	abi_ulong a0x;
	abi_ulong b3;
	abi_ulong b2;
	abi_ulong b1;
	abi_ulong b0;
	abi_ulong l3;
	abi_ulong l2;
	abi_ulong l1;
	abi_ulong l0;
	abi_ulong m3;
	abi_ulong m2;
	abi_ulong m1;
	abi_ulong m0;
	abi_ulong i3;
	abi_ulong i2;
	abi_ulong i1;
	abi_ulong i0;
	abi_ulong usp;
	abi_ulong fp;
	abi_ulong p5;
	abi_ulong p4;
	abi_ulong p3;
	abi_ulong p2;
	abi_ulong p1;
	abi_ulong p0;
	abi_ulong r7;
	abi_ulong r6;
	abi_ulong r5;
	abi_ulong r4;
	abi_ulong r3;
	abi_ulong r2;
	abi_ulong r1;
	abi_ulong r0;
	abi_ulong orig_r0;
	abi_ulong orig_p0;
	abi_ulong syscfg;
};

#define UNAME_MACHINE "blackfin"

#define UNAME_MINIMUM_RELEASE "2.6.32"
