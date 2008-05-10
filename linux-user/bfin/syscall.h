struct target_pt_regs {
	long orig_pc;
	long ipend;
	long seqstat;
	long rete;
	long retn;
	long retx;
	long pc;		/* PC == RETI */
	long rets;
	long reserved;		/* Used as scratch during system calls */
	long astat;
	long lb1;
	long lb0;
	long lt1;
	long lt0;
	long lc1;
	long lc0;
	long a1w;
	long a1x;
	long a0w;
	long a0x;
	long b3;
	long b2;
	long b1;
	long b0;
	long l3;
	long l2;
	long l1;
	long l0;
	long m3;
	long m2;
	long m1;
	long m0;
	long i3;
	long i2;
	long i1;
	long i0;
	long usp;
	long fp;
	long p5;
	long p4;
	long p3;
	long p2;
	long p1;
	long p0;
	long r7;
	long r6;
	long r5;
	long r4;
	long r3;
	long r2;
	long r1;
	long r0;
	long orig_r0;
	long orig_p0;
	long syscfg;
};

#define UNAME_MACHINE "blackfin"
