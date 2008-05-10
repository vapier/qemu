//Original:/testcases/core/c_dsp32shift_lhalf_rn/c_dsp32shift_lhalf_rn.dsp
// Spec Reference: dsp32shift lshift
# mach: bfin

.include "testutils.inc"
	start




// lshift : positive data, count (+)=left (half reg)
// d_lo = lshift (d_lo BY d_lo)
// RLx by RLx
imm32 r0, 0x00000000;
R0.L = -1;
imm32 r1, 0x00008001;
imm32 r2, 0x00008002;
imm32 r3, 0x00008003;
imm32 r4, 0x00008004;
imm32 r5, 0x00008005;
imm32 r6, 0x00008006;
imm32 r7, 0x00008007;
//rl0 = lshift (rl0 by rl0);
R1.L = LSHIFT R1.L BY R0.L;
R2.L = LSHIFT R2.L BY R0.L;
R3.L = LSHIFT R3.L BY R0.L;
R4.L = LSHIFT R4.L BY R0.L;
R5.L = LSHIFT R5.L BY R0.L;
R6.L = LSHIFT R6.L BY R0.L;
R7.L = LSHIFT R7.L BY R0.L;
//CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00004000;
CHECKREG r2, 0x00004001;
CHECKREG r3, 0x00004001;
CHECKREG r4, 0x00004002;
CHECKREG r5, 0x00004002;
CHECKREG r6, 0x00004003;
CHECKREG r7, 0x00004003;

imm32 r0, 0x00008001;
R1.L = -1;
imm32 r2, 0x00008002;
imm32 r3, 0x00008003;
imm32 r4, 0x00008004;
imm32 r5, 0x00008005;
imm32 r6, 0x00008006;
imm32 r7, 0x00008007;
R0.L = LSHIFT R0.L BY R1.L;
//rl1 = lshift (rl1 by rl1);
R2.L = LSHIFT R2.L BY R1.L;
R3.L = LSHIFT R3.L BY R1.L;
R4.L = LSHIFT R4.L BY R1.L;
R5.L = LSHIFT R5.L BY R1.L;
R6.L = LSHIFT R6.L BY R1.L;
R7.L = LSHIFT R7.L BY R1.L;
CHECKREG r0, 0x00004000;
//CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00004001;
CHECKREG r3, 0x00004001;
CHECKREG r4, 0x00004002;
CHECKREG r5, 0x00004002;
CHECKREG r6, 0x00004003;
CHECKREG r7, 0x00004003;


imm32 r0, 0x00008001;
imm32 r1, 0x00008001;
R2.L = -15;
imm32 r3, 0x00008003;
imm32 r4, 0x00008004;
imm32 r5, 0x00008005;
imm32 r6, 0x00008006;
imm32 r7, 0x00008007;
R0.L = LSHIFT R0.L BY R2.L;
R1.L = LSHIFT R1.L BY R2.L;
//rl2 = lshift (rl2 by rl2);
R3.L = LSHIFT R3.L BY R2.L;
R4.L = LSHIFT R4.L BY R2.L;
R5.L = LSHIFT R5.L BY R2.L;
R6.L = LSHIFT R6.L BY R2.L;
R7.L = LSHIFT R7.L BY R2.L;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000001;
//CHECKREG r2, 0x0000000f;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000001;

imm32 r0, 0x00008001;
imm32 r1, 0x00008001;
imm32 r2, 0x00008002;
R3.L = -16;
imm32 r4, 0x00008004;
imm32 r5, 0x00008005;
imm32 r6, 0x00008006;
imm32 r7, 0x00008007;
R0.L = LSHIFT R0.L BY R3.L;
R1.L = LSHIFT R1.L BY R3.L;
R2.L = LSHIFT R2.L BY R3.L;
//rl3 = lshift (rl3 by rl3);
R4.L = LSHIFT R4.L BY R3.L;
R5.L = LSHIFT R5.L BY R3.L;
R6.L = LSHIFT R6.L BY R3.L;
R7.L = LSHIFT R7.L BY R3.L;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000000;
//CHECKREG r3, 0x00000010;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;

// d_lo = ashft (d_hi BY d_lo)
// RHx by RLx
imm32 r0, 0x00000000;
imm32 r1, 0x80010000;
imm32 r2, 0x80020000;
imm32 r3, 0x80030000;
imm32 r4, 0x80040000;
imm32 r5, 0x80050000;
imm32 r6, 0x80060000;
imm32 r7, 0x80070000;
R0.L = LSHIFT R0.H BY R0.L;
R1.L = LSHIFT R1.H BY R0.L;
R2.L = LSHIFT R2.H BY R0.L;
R3.L = LSHIFT R3.H BY R0.L;
R4.L = LSHIFT R4.H BY R0.L;
R5.L = LSHIFT R5.H BY R0.L;
R6.L = LSHIFT R6.H BY R0.L;
R7.L = LSHIFT R7.H BY R0.L;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x80018001;
CHECKREG r2, 0x80028002;
CHECKREG r3, 0x80038003;
CHECKREG r4, 0x80048004;
CHECKREG r5, 0x80058005;
CHECKREG r6, 0x80068006;
CHECKREG r7, 0x80078007;

imm32 r0, 0x80010000;
R1.L = -1;
imm32 r2, 0x80020000;
imm32 r3, 0x80030000;
imm32 r4, 0x80040000;
imm32 r5, 0x80050000;
imm32 r6, 0x80060000;
imm32 r7, 0x80070000;
R0.L = LSHIFT R0.H BY R1.L;
//rl1 = lshift (rh1 by rl1);
R2.L = LSHIFT R2.H BY R1.L;
R3.L = LSHIFT R3.H BY R1.L;
R4.L = LSHIFT R4.H BY R1.L;
R5.L = LSHIFT R5.H BY R1.L;
R6.L = LSHIFT R6.H BY R1.L;
R7.L = LSHIFT R7.H BY R1.L;
CHECKREG r0, 0x80014000;
//CHECKREG r1, 0x00010001;
CHECKREG r2, 0x80024001;
CHECKREG r3, 0x80034001;
CHECKREG r4, 0x80044002;
CHECKREG r5, 0x80054002;
CHECKREG r6, 0x80064003;
CHECKREG r7, 0x80074003;


imm32 r0, 0xa0010000;
imm32 r1, 0xa0010000;
R2.L = -15;
imm32 r3, 0xa0030000;
imm32 r4, 0xa0040000;
imm32 r5, 0xa0050000;
imm32 r6, 0xa0060000;
imm32 r7, 0xa0070000;
R0.L = LSHIFT R0.H BY R2.L;
R1.L = LSHIFT R1.H BY R2.L;
//rl2 = lshift (rh2 by rl2);
R3.L = LSHIFT R3.H BY R2.L;
R4.L = LSHIFT R4.H BY R2.L;
R5.L = LSHIFT R5.H BY R2.L;
R6.L = LSHIFT R6.H BY R2.L;
R7.L = LSHIFT R7.H BY R2.L;
CHECKREG r0, 0xa0010001;
CHECKREG r1, 0xa0010001;
//CHECKREG r2, 0x2002000f;
CHECKREG r3, 0xa0030001;
CHECKREG r4, 0xa0040001;
CHECKREG r5, 0xa0050001;
CHECKREG r6, 0xa0060001;
CHECKREG r7, 0xa0070001;

imm32 r0, 0xb0010001;
imm32 r1, 0xb0010001;
imm32 r2, 0xb0020002;
R3.L = -16;
imm32 r4, 0xb0040004;
imm32 r5, 0xb0050005;
imm32 r6, 0xb0060006;
imm32 r7, 0xb0070007;
R0.L = LSHIFT R0.H BY R3.L;
R1.L = LSHIFT R1.H BY R3.L;
R2.L = LSHIFT R2.H BY R3.L;
//rl3 = lshift (rh3 by rl3);
R4.L = LSHIFT R4.H BY R3.L;
R5.L = LSHIFT R5.H BY R3.L;
R6.L = LSHIFT R6.H BY R3.L;
R7.L = LSHIFT R7.H BY R3.L;
CHECKREG r0, 0xb0010000;
CHECKREG r1, 0xb0010000;
CHECKREG r2, 0xb0020000;
//CHECKREG r3, 0x30030010;
CHECKREG r4, 0xb0040000;
CHECKREG r5, 0xb0050000;
CHECKREG r6, 0xb0060000;
CHECKREG r7, 0xb0070000;

// d_hi = ashft (d_lo BY d_lo)
// RLx by RLx
imm32 r0, 0x00000001;
imm32 r1, 0x00000001;
imm32 r2, 0x00000002;
imm32 r3, 0x00000003;
imm32 r4, 0x00000000;
imm32 r5, 0x00000005;
imm32 r6, 0x00000006;
imm32 r7, 0x00000007;
R0.H = LSHIFT R0.L BY R4.L;
R1.H = LSHIFT R1.L BY R4.L;
R2.H = LSHIFT R2.L BY R4.L;
R3.H = LSHIFT R3.L BY R4.L;
//rh4 = lshift (rl4 by rl4);
R5.H = LSHIFT R5.L BY R4.L;
R6.H = LSHIFT R6.L BY R4.L;
R7.H = LSHIFT R7.L BY R4.L;
CHECKREG r0, 0x00010001;
CHECKREG r1, 0x00010001;
CHECKREG r2, 0x00020002;
CHECKREG r3, 0x00030003;
//CHECKREG r4, 0x00040004;
CHECKREG r5, 0x00050005;
CHECKREG r6, 0x00060006;
CHECKREG r7, 0x00070007;

imm32 r0, 0x00008001;
imm32 r1, 0x00008001;
imm32 r2, 0x00008002;
imm32 r3, 0x00008003;
imm32 r4, 0x00008004;
R5.L = -1;
imm32 r6, 0x00008006;
imm32 r7, 0x00008007;
R0.H = LSHIFT R0.L BY R5.L;
R1.H = LSHIFT R1.L BY R5.L;
R2.H = LSHIFT R2.L BY R5.L;
R3.H = LSHIFT R3.L BY R5.L;
R4.H = LSHIFT R4.L BY R5.L;
//rh5 = lshift (rl5 by rl5);
R6.H = LSHIFT R6.L BY R5.L;
R7.H = LSHIFT R7.L BY R5.L;
CHECKREG r0, 0x40008001;
CHECKREG r1, 0x40008001;
CHECKREG r2, 0x40018002;
CHECKREG r3, 0x40018003;
CHECKREG r4, 0x40028004;
//CHECKREG r5, 0x00020005;
CHECKREG r6, 0x40038006;
CHECKREG r7, 0x40038007;


imm32 r0, 0x00009001;
imm32 r1, 0x00009001;
imm32 r2, 0x00009002;
imm32 r3, 0x00009003;
imm32 r4, 0x00009004;
imm32 r5, 0x00009005;
R6.L = -15;
imm32 r7, 0x00009007;
R0.H = LSHIFT R0.L BY R6.L;
R1.H = LSHIFT R1.L BY R6.L;
R2.H = LSHIFT R2.L BY R6.L;
R3.H = LSHIFT R3.L BY R6.L;
R4.H = LSHIFT R4.L BY R6.L;
R5.H = LSHIFT R5.L BY R6.L;
//rh6 = lshift (rl6 by rl6);
R7.H = LSHIFT R7.L BY R6.L;
CHECKREG r0, 0x00019001;
CHECKREG r1, 0x00019001;
CHECKREG r2, 0x00019002;
CHECKREG r3, 0x00019003;
CHECKREG r4, 0x00019004;
CHECKREG r5, 0x00019005;
//CHECKREG r6, 0x00006006;
CHECKREG r7, 0x00019007;

imm32 r0, 0x0000a001;
imm32 r1, 0x0000a001;
imm32 r2, 0x0000a002;
imm32 r3, 0x0000a003;
imm32 r4, 0x0000a004;
imm32 r5, 0x0000a005;
imm32 r6, 0x0000a006;
R7.L = -16;
R0.H = LSHIFT R0.L BY R7.L;
R1.H = LSHIFT R1.L BY R7.L;
R2.H = LSHIFT R2.L BY R7.L;
R3.H = LSHIFT R3.L BY R7.L;
R4.H = LSHIFT R4.L BY R7.L;
R5.H = LSHIFT R5.L BY R7.L;
R6.H = LSHIFT R6.L BY R7.L;
R7.H = LSHIFT R7.L BY R7.L;
CHECKREG r0, 0x0000a001;
CHECKREG r1, 0x0000a001;
CHECKREG r2, 0x0000a002;
CHECKREG r3, 0x0000a003;
CHECKREG r4, 0x0000a004;
CHECKREG r5, 0x0000a005;
CHECKREG r6, 0x0000a006;
//CHECKREG r7, 0x00007007;

// d_lo = ashft (d_hi BY d_lo)
// RHx by RLx
imm32 r0, 0x80010000;
imm32 r1, 0x80010000;
imm32 r2, 0x80020000;
imm32 r3, 0x80030000;
R4.L = -1;
imm32 r5, 0x80050000;
imm32 r6, 0x80060000;
imm32 r7, 0x80070000;
R0.H = LSHIFT R0.H BY R4.L;
R1.H = LSHIFT R1.H BY R4.L;
R2.H = LSHIFT R2.H BY R4.L;
R3.H = LSHIFT R3.H BY R4.L;
//rh4 = lshift (rh4 by rl4);
R5.H = LSHIFT R5.H BY R4.L;
R6.H = LSHIFT R6.H BY R4.L;
R7.H = LSHIFT R7.H BY R4.L;
CHECKREG r0, 0x40000000;
CHECKREG r1, 0x40000000;
CHECKREG r2, 0x40010000;
CHECKREG r3, 0x40010000;
//CHECKREG r4, 0x00020000;
CHECKREG r5, 0x40020000;
CHECKREG r6, 0x40030000;
CHECKREG r7, 0x40030000;

imm32 r0, 0x80010000;
imm32 r1, 0x80010000;
imm32 r2, 0x80020000;
imm32 r3, 0x80030000;
imm32 r4, 0x80040000;
R5.L = -1;
imm32 r6, 0x80060000;
imm32 r7, 0x80070000;
R0.H = LSHIFT R0.H BY R5.L;
R1.H = LSHIFT R1.H BY R5.L;
R2.H = LSHIFT R2.H BY R5.L;
R3.H = LSHIFT R3.H BY R5.L;
R4.H = LSHIFT R4.H BY R5.L;
//rh5 = lshift (rh5 by rl5);
R6.H = LSHIFT R6.H BY R5.L;
R7.H = LSHIFT R7.H BY R5.L;
CHECKREG r0, 0x40000000;
CHECKREG r1, 0x40000000;
CHECKREG r2, 0x40010000;
CHECKREG r3, 0x40010000;
CHECKREG r4, 0x40020000;
//CHECKREG r5, 0x28020000;
CHECKREG r6, 0x40030000;
CHECKREG r7, 0x40030000;


imm32 r0, 0xd0010000;
imm32 r1, 0xd0010000;
imm32 r2, 0xd0020000;
imm32 r3, 0xd0030000;
imm32 r4, 0xd0040000;
imm32 r5, 0xd0050000;
R6.L = -15;
imm32 r7, 0xd0070000;
R0.L = LSHIFT R0.H BY R6.L;
R1.L = LSHIFT R1.H BY R6.L;
R2.L = LSHIFT R2.H BY R6.L;
R3.L = LSHIFT R3.H BY R6.L;
R4.L = LSHIFT R4.H BY R6.L;
R5.L = LSHIFT R5.H BY R6.L;
//rl6 = lshift (rh6 by rl6);
R7.L = LSHIFT R7.H BY R6.L;
CHECKREG r0, 0xd0010001;
CHECKREG r1, 0xd0010001;
CHECKREG r2, 0xd0020001;
CHECKREG r3, 0xd0030001;
CHECKREG r4, 0xd0040001;
CHECKREG r5, 0xd0050001;
//CHECKREG r6, 0x60060000;
CHECKREG r7, 0xd0070001;

imm32 r0, 0xe0010000;
imm32 r1, 0xe0010000;
imm32 r2, 0xe0020000;
imm32 r3, 0xe0030000;
imm32 r4, 0xe0040000;
imm32 r5, 0xe0050000;
imm32 r6, 0xe0060000;
R7.L = -16;
R0.H = LSHIFT R0.H BY R7.L;
R1.H = LSHIFT R1.H BY R7.L;
R2.H = LSHIFT R2.H BY R7.L;
R3.H = LSHIFT R3.H BY R7.L;
R4.H = LSHIFT R4.H BY R7.L;
R5.H = LSHIFT R5.H BY R7.L;
R6.H = LSHIFT R6.H BY R7.L;
//rh7 = lshift (rh7 by rl7);
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
//CHECKREG r7, -16;

pass
