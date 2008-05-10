//Original:/testcases/core/c_dsp32shiftim_af_s/c_dsp32shiftim_af_s.dsp
# mach: bfin

.include "testutils.inc"
	start


// Spec Reference: dsp32shiftimm ashift: ashift saturated

imm32 r0, 0x3a1230001;
imm32 r1, 0x1e345678;
imm32 r2, 0x23f56789;
imm32 r3, 0x34db789a;
imm32 r4, 0x85a7a9ab;
imm32 r5, 0x967c9abc;
imm32 r6, 0xa78dabcd;
imm32 r7, 0xb8914cde;
R6 = R0 >>> 1;
R7 = R1 >>> 3;
R0 = R2 >>> 7;
R1 = R3 >>> 8;
R2 = R4 >>> 15;
R3 = R5 >>> 24;
R4 = R6 >>> 31;
R5 = R7 >>> 20;
CHECKREG r0, 0x0047EACF;
CHECKREG r1, 0x0034DB78;
CHECKREG r2, 0xFFFF0B4F;
CHECKREG r3, 0xFFFFFF96;
CHECKREG r4, 0xFFFFFFFF;
CHECKREG r5, 0x0000003C;
CHECKREG r6, 0xD0918000;
CHECKREG r7, 0x03C68ACF;



pass
