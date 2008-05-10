# R0 and P0 are used as tmps, consider them call clobbered by these macros.

# To build for hardware, use:
# bfin-linux-uclibc-gcc -nostdlib -g -Wa,--defsym,BFIN_HOST=1 foo.s

	.macro start
	.text
	# Pad with EMUEXCPT to make sure "jump to 0" always fails
__panic:
	.rep 0xf
	.word 0x0025
	.endr
	abort;
	jump __panic;

	.global __pass
__pass:
	write 1, _passmsg, 5
	exit 0
.ifdef BFIN_JTAG
__emu_out:
	/* DBGSTAT */
	P0.L = 0x5008;
	P0.H = 0xFFE0;
1:
	R7 = [P0];
	CC = BITTST (R7,0);
	IF CC JUMP 1b;
	EMUDAT = R0;
2:
	R7 = [P0];
	CC = BITTST (R7,0);
	IF CC JUMP 2b;
	RTS;
.endif
	.global __fail
__fail:
.ifndef BFIN_HOST
	P0.H = _rets;
	P0.L = _rets;
	R0 = RETS;
	R0 += -4;
	P1 = 8;
	R2 = '9';
	LSETUP (1f, 3f) LC0 = P1;
1:
	R1 = R0;
	R1 >>= 28;
	R1 += 0x30;
	CC = R2 < R1;
	IF !CC jump 2f;
	R1 += 7;
2:
	B[P0++] = R1;
3:
	R0 <<= 4;

	write 1, _failmsg, 22
.else
	write 1, _failmsg, 5
.endif
	exit 1

.ifndef BFIN_HOST
	.data
_failmsg:
	.ascii "fail at PC=0x"
_rets:
	.ascii "12345678\n"
_passmsg:
	.ascii "pass\n"
	.align 4
_params:
	.long 0
	.long 0
	.long 0
	.long 0

	.text
	.global __start
__start:
.else
.global ___uClibc_main;
___uClibc_main:
.global _main;
_main:
.endif
	.endm

	.macro system_call nr:req
	P0 = \nr (X);
	EXCPT 0;
	.endm

	.macro exit rc:req
	R0 = \rc (X);
.ifndef BFIN_HOST
	P0.H = _params;
	P0.L = _params;
	[P0] = R0;
	R0 = P0;
.endif
	system_call 1
	.endm

	.macro pass
	dbg_pass
#	CALL __pass;
	.endm

	.macro fail
	dbg_fail
#	CALL __fail;
	.endm

	.macro write fd:req, buf:req, count:req
.ifndef BFIN_HOST
	P0.H = _params;
	P0.L = _params;
	R0 = \fd (X);
	[P0] = R0;
	R0.H = \buf;
	R0.L = \buf;
	[P0 + 4] = R0;
	R0 = \count (X);
	[P0 + 8] = R0;
	R0 = P0;
	system_call 5
.endif
	.endm

	.macro outc_str ch:req, more:vararg
	OUTC \ch;
	.ifnb \more
	outc_str \more
	.endif
	.endm
	.macro dbg_pass
.ifdef BFIN_JTAG
	R0 = 6;
	CALL __emu_out;
	R0.L = 0x6170;	/* 'p'=0x70  'a'=0x70 */
	R0.H = 0x7373;	/* 's'=0x73 */
	CALL __emu_out;

	R0.L = 0x0A;	/* newline */
	R0.H = 0x0000;
	CALL __emu_out;  
1:
	EMUEXCPT;
	JUMP 1b;
.else
	outc_str 'p', 'a', 's', 's', '\n'
	HLT;
.endif
	.endm
	.macro dbg_fail
.ifdef BFIN_JTAG
	R0 = 6;
	CALL __emu_out;
	R0.L = 0x6166;  /* 'f'=0x66  'a'=0x61 */
	R0.H = 0x6c69;  /* 'i'=0x69  'l'=0x6c */
	CALL __emu_out;

	R0.L = 0x0A;    /* newline */
	R0.H = 0x0000;
	CALL __emu_out;
1:
	EMUEXCPT;
	JUMP 1b;
.else
	outc_str 'f', 'a', 'i', 'l', '\n'
.endif
	ABORT;
	.endm

	.macro imm32 reg:req, val:req
	\reg\().L = ((\val) & 0xffff);
	\reg\().H = (((\val) >> 16) & 0xffff);
	.endm

	.macro dmm32 reg:req, val:req
	[--SP] = R0;
	imm32 R0, \val
	\reg = R0;
	R0 = [SP++];
	.endm

.ifndef BFIN_HOST
	.macro loadsym reg:req, sym:req, offset=0
	\reg\().L = (\sym\() + \offset\());
	\reg\().H = (\sym\() + \offset\());
	.endm
.else
	.macro loadsym reg:req, sym:req, offset=0
	[--SP] = R0;
	R0 = [P3 + \sym\()@GOT17M4];
	.if \offset
	[--SP] = R1;
	R1 = \offset\() (Z);
	R0 = R0 + R1;
	R1 = [SP++];
	.endif
	\reg = R0;
	R0 = [SP++];
	.endm
.endif

	.macro CHECKREG reg:req, val:req
	DBGA (\reg\().L, ((\val) & 0xffff));
	DBGA (\reg\().H, (((\val) >> 16) & 0xffff));
	.endm

	.macro __init_regs reg:req, max:req, x:req, val:req
	.ifle (\x - \max)
	\reg\()\x\().L = ((\val) & 0xffff);
	\reg\()\x\().H = (((\val) >> 16) & 0xffff);
	.endif
	.endm
	.macro _init_regs reg:req, max:req, val:req
	__init_regs \reg, \max, 0, \val
	__init_regs \reg, \max, 1, \val
	__init_regs \reg, \max, 2, \val
	__init_regs \reg, \max, 3, \val
	__init_regs \reg, \max, 4, \val
	__init_regs \reg, \max, 5, \val
	__init_regs \reg, \max, 6, \val
	__init_regs \reg, \max, 7, \val
	.endm

	.macro init_r_regs val:req
	_init_regs R, 7, \val
	.endm
	.macro init_p_regs val:req
	_init_regs P, 5, \val
	.endm
	.macro init_b_regs val:req
	_init_regs B, 3, \val
	.endm
	.macro init_i_regs val:req
	_init_regs I, 3, \val
	.endm
	.macro init_l_regs val:req
	_init_regs L, 3, \val
	.endm
	.macro init_m_regs val:req
	_init_regs M, 3, \val
	.endm

	// the test framework needs things to be quiet, so don't
	// print things out by default.
	.macro _DBG reg:req
	//DBG \reg;
	.endm

	.macro _DBGCMPLX reg:req
	//
	.endm
