/*
 * Blackfin translation
 *
 * Copyright 2007-2011 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "cpu.h"
#include "exec-all.h"
#include "disas.h"
#include "tcg-op.h"
#include "qemu-common.h"
#include "opcode/bfin.h"

#include "helper.h"
#define GEN_HELPER 1
#include "helper.h"

/* We're making a call (which means we need to update RTS) */
#define DISAS_CALL 0xad0

static TCGv_ptr cpu_env;
static TCGv cpu_dreg[8];
static TCGv cpu_preg[8];
#define cpu_spreg cpu_preg[6]
#define cpu_fpreg cpu_preg[7]
static TCGv cpu_ireg[4];
static TCGv cpu_mreg[4];
static TCGv cpu_breg[4];
static TCGv cpu_lreg[4];
static TCGv_i64 cpu_areg[2];
//static TCGv cpu_axreg[2], cpu_awreg[2];
static TCGv cpu_rets;
static TCGv cpu_lcreg[2], cpu_ltreg[2], cpu_lbreg[2];
static TCGv cpu_cycles[2];
static TCGv cpu_uspreg;
static TCGv cpu_seqstat;
static TCGv cpu_syscfg;
static TCGv cpu_reti;
static TCGv cpu_retx;
static TCGv cpu_retn;
static TCGv cpu_rete;
static TCGv cpu_emudat;
static TCGv cpu_pc;
static TCGv cpu_cc;
static TCGv cpu_astat_op, cpu_astat_arg[3];

#include "gen-icount.h"

typedef struct bfin_def_t {
	const char *name;
} bfin_def_t;

void cpu_reset(CPUState *env)
{
	env->pc = 0xEF000000;
}

static inline void
bfin_tcg_new_set3(TCGv *tcgv, unsigned int cnt, unsigned int offbase,
                  const char * const *names)
{
	unsigned int i;
	for (i = 0; i < cnt; ++i)
		tcgv[i] = tcg_global_mem_new(TCG_AREG0, offbase + (i * 4), names[i]);
}
#define bfin_tcg_new_set2(tcgv, cnt, reg, name_idx) \
	bfin_tcg_new_set3(tcgv, cnt, offsetof(CPUState, reg), &greg_names[name_idx])
#define bfin_tcg_new_set(reg, name_idx) \
	bfin_tcg_new_set2(cpu_##reg, ARRAY_SIZE(cpu_##reg), reg, name_idx)
#define bfin_tcg_new(reg, name_idx) \
	bfin_tcg_new_set2(&cpu_##reg, 1, reg, name_idx)

extern const char * const greg_names[];
CPUState *cpu_init(const char *cpu_model)
{
	CPUState *env;
	static int tcg_initialized = 0;

	env = qemu_mallocz(sizeof(*env));
	if (!env)
		return NULL;

	cpu_exec_init(env);
	cpu_reset(env);

	if (tcg_initialized)
		return env;

	tcg_initialized = 1;

#define GEN_HELPER 2
#include "helper.h"

	cpu_env = tcg_global_reg_new_ptr(TCG_AREG0, "env");

	cpu_pc = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, pc), "PC");
	cpu_cc = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, astat[ASTAT_CC]), "CC");

	cpu_astat_op = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, astat_op), "astat_op");
	cpu_astat_arg[0] = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, astat_arg[0]), "astat_arg[0]");
	cpu_astat_arg[1] = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, astat_arg[1]), "astat_arg[1]");
	cpu_astat_arg[2] = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, astat_arg[2]), "astat_arg[2]");

	cpu_areg[0] = tcg_global_mem_new_i64(TCG_AREG0,
		offsetof(CPUState, areg[0]), "A0");
	cpu_areg[1] = tcg_global_mem_new_i64(TCG_AREG0,
		offsetof(CPUState, areg[1]), "A1");
/*
	cpu_axreg[0] = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, areg[0].x), greg_names[32]);
	cpu_axreg[1] = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, areg[1].x), greg_names[34]);
	cpu_awreg[0] = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, areg[0].w), greg_names[33]);
	cpu_awreg[1] = tcg_global_mem_new(TCG_AREG0,
		offsetof(CPUState, areg[1].w), greg_names[35]);
*/
//"A1");

	bfin_tcg_new_set(dreg, 0);
	bfin_tcg_new_set(preg, 8);
	bfin_tcg_new_set(ireg, 16);
	bfin_tcg_new_set(mreg, 20);
	bfin_tcg_new_set(breg, 24);
	bfin_tcg_new_set(lreg, 28);
//	bfin_tcg_new(axreg[0], 32);
//	bfin_tcg_new(awreg[0], 33);
//	bfin_tcg_new(axreg[1], 34);
//	bfin_tcg_new(awreg[1], 35);
	bfin_tcg_new(rets, 39);
	bfin_tcg_new(lcreg[0], 48);
	bfin_tcg_new(ltreg[0], 49);
	bfin_tcg_new(lbreg[0], 50);
	bfin_tcg_new(lcreg[1], 51);
	bfin_tcg_new(ltreg[1], 52);
	bfin_tcg_new(lbreg[1], 53);
	bfin_tcg_new_set(cycles, 54);
	bfin_tcg_new(uspreg, 56);
	bfin_tcg_new(seqstat, 57);
	bfin_tcg_new(syscfg, 58);
	bfin_tcg_new(reti, 59);
	bfin_tcg_new(retx, 60);
	bfin_tcg_new(retn, 61);
	bfin_tcg_new(rete, 62);
	bfin_tcg_new(emudat, 63);

	return env;
}

static const bfin_def_t bfin_defs[] = {
	{
		.name = "BF531",
	},{
		.name = "BF532",
	},{
		.name = "BF533",
	},{
		.name = "BF534",
	},{
		.name = "BF536",
	},{
		.name = "BF537",
	},{
		.name = "BF542",
	},{
		.name = "BF544",
	},{
		.name = "BF548",
	},{
		.name = "BF549",
	},{
		.name = "BF561",
	}
};

void bfin_cpu_list(FILE *f, int (*cpu_fprintf)(FILE *f, const char *fmt, ...))
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(bfin_defs); ++i)
		cpu_fprintf(f, "Blackfin %s\n", bfin_defs[i].name);
}

#define _astat_printf(bit) cpu_fprintf(f, "%s" #bit " ", (env->astat[ASTAT_##bit] ? "" : "~"))
void cpu_dump_state(CPUState *env, FILE *f,
                    int (*cpu_fprintf)(FILE *f, const char *fmt, ...),
                    int flags)
{
//	uint64_t iw;
//	target_ulong len;

	cpu_fprintf(f, "              SYSCFG: %04lx   SEQSTAT: %08x\n",
	            env->syscfg, env->seqstat);
	cpu_fprintf(f, "RETE: %08x  RETN: %08x  RETX: %08x\n",
	            env->rete, env->retn, env->retx);
	cpu_fprintf(f, "RETI: %08x  RETS: %08x   PC : %08x\n",
	            env->reti, env->rets, env->pc);
	cpu_fprintf(f, " R0 : %08x   R4 : %08x   P0 : %08x   P4 : %08x\n",
	            env->dreg[0], env->dreg[4], env->preg[0], env->preg[4]);
	cpu_fprintf(f, " R1 : %08x   R5 : %08x   P1 : %08x   P5 : %08x\n",
	            env->dreg[1], env->dreg[5], env->preg[1], env->preg[5]);
	cpu_fprintf(f, " R2 : %08x   R6 : %08x   P2 : %08x   SP : %08x\n",
	            env->dreg[2], env->dreg[6], env->preg[2], env->spreg);
	cpu_fprintf(f, " R3 : %08x   R7 : %08x   P3 : %08x   FP : %08x\n",
	            env->dreg[3], env->dreg[7], env->preg[3], env->fpreg);
	cpu_fprintf(f, " LB0: %08x   LT0: %08x   LC0: %08x\n",
	            env->lbreg[0], env->ltreg[0], env->lcreg[0]);
	cpu_fprintf(f, " LB1: %08x   LT1: %08x   LC1: %08x\n",
	            env->lbreg[1], env->ltreg[1], env->lcreg[1]);
	cpu_fprintf(f, " B0 : %08x   L0 : %08x   M0 : %08x   I0 : %08x\n",
	            env->breg[0], env->lreg[0], env->mreg[0], env->ireg[0]);
	cpu_fprintf(f, " B1 : %08x   L1 : %08x   M1 : %08x   I1 : %08x\n",
	            env->breg[1], env->lreg[1], env->mreg[1], env->ireg[1]);
	cpu_fprintf(f, " B2 : %08x   L2 : %08x   M2 : %08x   I2 : %08x\n",
	            env->breg[2], env->lreg[2], env->mreg[2], env->ireg[2]);
	cpu_fprintf(f, " B3 : %08x   L3 : %08x   M3 : %08x   I3 : %08x\n",
	            env->breg[3], env->lreg[3], env->mreg[3], env->ireg[3]);
//	cpu_fprintf(f, "A0.w: %08x  A0.x: %08x  A1.w: %08x  A1.x: %08x\n",
//	            env->areg[0].w, env->areg[0].x, env->areg[1].w, env->areg[1].x);
	cpu_fprintf(f, "  A0: %010lx                 A1: %010lx\n",
	            env->areg[0] & 0xffffffffff, env->areg[1] & 0xffffffffff);
	cpu_fprintf(f, " USP: %08x ASTAT: %08x   CC : %08x\n",
	            env->uspreg, bfin_astat_read(env), env->astat[ASTAT_CC]);
	cpu_fprintf(f, "ASTAT BITS: ");
	_astat_printf(VS);
	_astat_printf(V);
	_astat_printf(AV1S);
	_astat_printf(AV1);
	_astat_printf(AV0S);
	_astat_printf(AV0);
	_astat_printf(AC1);
	_astat_printf(AC0);
	_astat_printf(AQ);
	_astat_printf(CC);
	_astat_printf(V_COPY);
	_astat_printf(AC0_COPY);
	_astat_printf(AN);
	_astat_printf(AZ);
	cpu_fprintf(f, "\nASTAT CACHE:   OP: %02u   ARG: %08x %08x %08x\n",
	            env->astat_op, env->astat_arg[0], env->astat_arg[1], env->astat_arg[2]);
	cpu_fprintf(f, "              CYCLES: %08x %08x\n",
	            env->cycles[0], env->cycles[1]);

/*
	iw = ldq_code(env->pc);
	if ((iw & 0xc000) != 0xc000)
		len = 2;
	else if ((iw & BIT_MULTI_INS) && (iw & 0xe800) != 0xe800)
		len = 8;
	else
		len = 4;
	log_target_disas(env->pc, len, 0);
*/
}

static void gen_astat_update(DisasContext *, bool);

static void gen_goto_tb(DisasContext *dc, int tb_num, TCGv dest)
{
	TranslationBlock *tb;
	tb = dc->tb;
/*
printf("%s: here\n", __func__);
	if ((tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK)) {
		tcg_gen_goto_tb(tb_num);
		tcg_gen_mov_tl(cpu_pc, dest);
		tcg_gen_exit_tb((long)tb + tb_num);
	} else */{
gen_astat_update(dc, false);
		tcg_gen_goto_tb(0);
		tcg_gen_mov_tl(cpu_pc, dest);
		tcg_gen_exit_tb(0);
	}
}

static void gen_gotoi_tb(DisasContext *dc, int tb_num, target_ulong dest)
{
	TCGv tmp = tcg_temp_local_new();
	tcg_gen_movi_tl(tmp, dest);
	gen_goto_tb(dc, tb_num, tmp);
	tcg_temp_free(tmp);
}

static void cec_exception(DisasContext *dc, int excp)
{
	TCGv tmp = tcg_const_tl(excp);
	TCGv pc = tcg_const_tl(dc->pc);
//	dc->env->pc = dc->pc;
	gen_helper_raise_exception(tmp, pc);
	tcg_temp_free(tmp);
	dc->is_jmp = DISAS_UPDATE;
}

static void cec_require_supervisor(DisasContext *dc)
{
#ifdef CONFIG_LINUX_USER
	cec_exception(dc, EXCP_ILL_SUPV);
#else
# error todo
#endif
}

/*
 * If a LB reg is written, we need to invalidate the two translation
 * blocks that could be affected -- the TB's referenced by the old LB
 * could have LC/LT handling which we no longer want, and the new LB
 * is probably missing LC/LT handling which we want.  In both cases,
 * we need to regenerate the block.
 */
static void gen_maybe_lb_exit_tb(DisasContext *dc, TCGv reg)
{
	if (!TCGV_EQUAL(reg, cpu_lbreg[0]) && !TCGV_EQUAL(reg, cpu_lbreg[1]))
		return;

	//tb_invalidate_phys_page_range
	dc->is_jmp = DISAS_UPDATE;
	/* XXX: Not entirely correct, but very few things load
	 *      directly into LB ... */
	gen_gotoi_tb(dc, 0, dc->pc + dc->insn_len);
}

static void gen_hwloop_default(DisasContext *dc, int loop)
{
	if (loop != -1)
		gen_goto_tb(dc, 0, cpu_ltreg[loop]);
}

static void _gen_hwloop_call(DisasContext *dc, int loop)
{
	if (dc->is_jmp != DISAS_CALL)
		return;

	if (loop == -1)
		tcg_gen_movi_tl(cpu_rets, dc->pc + dc->insn_len);
	else
		tcg_gen_mov_tl(cpu_rets, cpu_ltreg[loop]);
}

static void gen_hwloop_br_pcrel_cc(DisasContext *dc, int loop)
{
	int l;
	int pcrel = (unsigned long)dc->hwloop_data;
	int T = pcrel & 1;
	pcrel &= ~1;

	l = gen_new_label();
	tcg_gen_brcondi_tl(TCG_COND_NE, cpu_cc, T, l);
	gen_gotoi_tb(dc, 0, dc->pc + pcrel);
	gen_set_label(l);
	if (loop == -1)
		dc->hwloop_callback = gen_hwloop_default;
	else
		gen_hwloop_default(dc, loop);
}

static void gen_hwloop_br_pcrel(DisasContext *dc, int loop)
{
	TCGv *reg = dc->hwloop_data;
	_gen_hwloop_call(dc, loop);
	tcg_gen_addi_tl(cpu_pc, *reg, dc->pc);
	gen_goto_tb(dc, 0, cpu_pc);
}

static void gen_hwloop_br_pcrel_imm(DisasContext *dc, int loop)
{
	int pcrel = (unsigned long)dc->hwloop_data;
	TCGv tmp;

	_gen_hwloop_call(dc, loop);
	tmp = tcg_const_tl(pcrel);
	tcg_gen_addi_tl(cpu_pc, tmp, dc->pc);
	tcg_temp_free(tmp);
	gen_goto_tb(dc, 0, cpu_pc);
}

static void gen_hwloop_br_direct(DisasContext *dc, int loop)
{
	TCGv *reg = dc->hwloop_data;
	_gen_hwloop_call(dc, loop);
	gen_goto_tb(dc, 0, *reg);
}

static void _gen_hwloop_check(DisasContext *dc, int loop, int l)
{
	tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_lcreg[loop], 0, l);
	tcg_gen_subi_tl(cpu_lcreg[loop], cpu_lcreg[loop], 1);
	tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_lcreg[loop], 0, l);
	dc->hwloop_callback(dc, loop);
}

static void gen_hwloop_check(DisasContext *dc)
{
	bool loop1, loop0;
	int endl;

	loop1 = (dc->pc == dc->env->lbreg[1]);
	loop0 = (dc->pc == dc->env->lbreg[0]);

	if (loop1 || loop0)
		endl = gen_new_label();

	if (loop1) {
		int l;
		if (loop0)
			l = gen_new_label();
		else
			l = endl;

		_gen_hwloop_check(dc, 1, l);

		if (loop0) {
			tcg_gen_br(endl);
			gen_set_label(l);
		}
	}

	if (loop0)
		_gen_hwloop_check(dc, 0, endl);

	if (loop1 || loop0)
		gen_set_label(endl);

	dc->hwloop_callback(dc, -1);
}

/* R#.L = reg */
static void gen_mov_l_tl(TCGv dst, TCGv src)
{
	tcg_gen_andi_tl(dst, dst, 0xffff0000);
	tcg_gen_andi_tl(src, src, 0xffff);
	tcg_gen_or_tl(dst, dst, src);
}

/* R#.L = imm32 */
/*
static void gen_movi_l_tl(TCGv dst, uint32_t src)
{
	tcg_gen_andi_tl(dst, dst, 0xffff0000);
	tcg_gen_ori_tl(dst, dst, src & 0xffff);
}
*/

/* R#.H = reg */
/* XXX: This modifies the source ... assumes it is a temp ... */
static void gen_mov_h_tl(TCGv dst, TCGv src)
{
	tcg_gen_andi_tl(dst, dst, 0xffff);
	tcg_gen_shli_tl(src, src, 16);
	tcg_gen_or_tl(dst, dst, src);
}

/* R#.H = imm32 */
/*
static void gen_movi_h_tl(TCGv dst, uint32_t src)
{
	tcg_gen_andi_tl(dst, dst, 0xffff);
	tcg_gen_ori_tl(dst, dst, src << 16);
}
*/

static void gen_extNs_tl(TCGv dst, TCGv src, TCGv n)
{
	/* Shift the sign bit up, and then back down */
	TCGv tmp = tcg_temp_new();
	tcg_gen_subfi_tl(tmp, 32, n);
	tcg_gen_shl_tl(dst, src, tmp);
	tcg_gen_sar_tl(dst, dst, tmp);
	tcg_temp_free(tmp);
}

static void gen_extNsi_tl(TCGv dst, TCGv src, uint32_t n)
{
	/* Shift the sign bit up, and then back down */
	tcg_gen_shli_tl(dst, src, 32 - n);
	tcg_gen_sari_tl(dst, dst, 32 - n);
}

#if 0
static void gen_extNu_tl(TCGv dst, TCGv src, TCGv n)
{
	/* Just mask off the higher bits */
	tcg_gen_andi_tl(dst, src, ~((1 << n) - 1));
}

static void gen_extNui_tl(TCGv dst, TCGv src, uint32_t n)
{
	/* Just mask off the higher bits */
	tcg_gen_andi_tl(dst, src, ~((1 << n) - 1));
}
#endif

static void gen_signbitsi_tl(TCGv dst, TCGv src, uint32_t size)
{
	TCGv tmp_size = tcg_const_tl(size);
	gen_helper_signbits(dst, src, tmp_size);
	tcg_temp_free(tmp_size);
}

static void gen_signbitsi_i64_i32(TCGv dst, TCGv_i64 src, uint32_t size)
{
	TCGv tmp_size = tcg_const_tl(size);
	gen_helper_signbits_64(dst, src, tmp_size);
	tcg_temp_free(tmp_size);
}

static void gen_abs_tl(TCGv ret, TCGv arg)
{
	int l = gen_new_label();
	tcg_gen_mov_tl(ret, arg);
	tcg_gen_brcondi_tl(TCG_COND_GE, arg, 0, l);
	tcg_gen_neg_tl(ret, ret);
	gen_set_label(l);
}

static void gen_abs_i64(TCGv_i64 ret, TCGv_i64 arg)
{
	int l = gen_new_label();
	tcg_gen_mov_i64(ret, arg);
	tcg_gen_brcondi_i64(TCG_COND_GE, arg, 0, l);
	tcg_gen_neg_i64(ret, ret);
	gen_set_label(l);
}

/* Common tail code for DIVQ/DIVS insns */
static void _gen_helper_divqs(TCGv pquo, TCGv r, TCGv aq, TCGv div)
{
	/*
	 * pquo <<= 1
	 * pquo |= aq
	 * pquo = (pquo & 0x1FFFF) | (r << 17)
	 */
	tcg_gen_shli_tl(pquo, pquo, 1);
	tcg_gen_or_tl(pquo, pquo, aq);
	tcg_gen_andi_tl(pquo, pquo, 0x1FFFF);
	tcg_gen_shli_tl(r, r, 17);
	tcg_gen_or_tl(pquo, pquo, r);

	tcg_temp_free(r);
	tcg_temp_free(aq);
	tcg_temp_free(div);
}

/* Common AQ ASTAT bit management for DIVQ/DIVS insns */
static void _gen_helper_divqs_st_aq(TCGv r, TCGv aq, TCGv div)
{
	/* aq = (r ^ div) >> 15 */
	tcg_gen_xor_tl(aq, r, div);
	tcg_gen_shri_tl(aq, aq, 15);
	tcg_gen_andi_tl(aq, aq, 1);
	tcg_gen_st_tl(aq, cpu_env, offsetof(CPUState, astat[ASTAT_AQ]));
}

/* DIVQ ( Dreg, Dreg ) ;
 * Based on AQ status bit, either add or subtract the divisor from
 * the dividend. Then set the AQ status bit based on the MSBs of the
 * 32-bit dividend and the 16-bit divisor. Left shift the dividend one
 * bit. Copy the logical inverse of AQ into the dividend LSB.
 */
static void gen_helper_divq(TCGv pquo, TCGv src)
{
	int l;
	TCGv af, r, aq, div;

	/* div = R#.L */
	div = tcg_temp_local_new();
	tcg_gen_ext16u_tl(div, src);

	/* af = pquo >> 16 */
	af = tcg_temp_local_new();
	tcg_gen_shri_tl(af, pquo, 16);

	/*
	 * we take this:
	 *  if (ASTAT_AQ)
	 *    r = div + af;
	 *  else
	 *    r = af - div;
	 *
	 * and turn it into:
	 *  r = div;
	 *  if (aq == 0)
	 *    r = -r;
	 *  r += af;
	 */
	aq = tcg_temp_local_new();
	tcg_gen_ld_tl(aq, cpu_env, offsetof(CPUState, astat[ASTAT_AQ]));

	l = gen_new_label();
	r = tcg_temp_local_new();
	tcg_gen_mov_tl(r, div);
	tcg_gen_brcondi_tl(TCG_COND_NE, aq, 0, l);
	tcg_gen_neg_tl(r, r);
	gen_set_label(l);
	tcg_gen_add_tl(r, r, af);

	tcg_temp_free(af);

	_gen_helper_divqs_st_aq(r, aq, div);

	/* aq = !aq */
	tcg_gen_xori_tl(aq, aq, 1);

	_gen_helper_divqs(pquo, r, aq, div);
}

/* DIVS ( Dreg, Dreg ) ;
 * Initialize for DIVQ. Set the AQ status bit based on the signs of
 * the 32-bit dividend and the 16-bit divisor. Left shift the dividend
 * one bit. Copy AQ into the dividend LSB.
 */
static void gen_helper_divs(TCGv pquo, TCGv src)
{
	TCGv r, aq, div;

	/* div = R#.L */
	div = tcg_temp_local_new();
	tcg_gen_ext16u_tl(div, src);

	/* r = pquo >> 16 */
	r = tcg_temp_local_new();
	tcg_gen_shri_tl(r, pquo, 16);

	aq = tcg_temp_local_new();

	_gen_helper_divqs_st_aq(r, aq, div);

	_gen_helper_divqs(pquo, r, aq, div);
}

#define _gen_astat_store(bit, reg) tcg_gen_st_tl(reg, cpu_env, offsetof(CPUState, astat[bit]))

static void _gen_astat_update_az(TCGv reg, TCGv tmp)
{
	tcg_gen_setcondi_tl(TCG_COND_EQ, tmp, reg, 0);
	_gen_astat_store(ASTAT_AZ, tmp);
}

static void _gen_astat_update_an(TCGv reg, TCGv tmp, uint32_t len)
{
	tcg_gen_setcondi_tl(TCG_COND_GEU, tmp, reg, 1 << (len - 1));
	_gen_astat_store(ASTAT_AN, tmp);
}

static void _gen_astat_update_nz(TCGv reg, TCGv tmp)
{
	_gen_astat_update_az(reg, tmp);
	_gen_astat_update_an(reg, tmp, 32);
}

static void gen_astat_update(DisasContext *dc, bool clear)
{
	TCGv tmp = tcg_temp_local_new();

	/* XXX: Might not be correct ... */
//	if (dc->astat_op == ASTAT_OP_DYNAMIC)
//		dc->astat_op = dc->env->astat_op;

	switch (dc->astat_op) {
	case ASTAT_OP_ABS:	/* [0] = ABS( [1] ) */
		_gen_astat_update_nz(cpu_astat_arg[0], tmp);
		break;

	case ASTAT_OP_ADD32:	/* [0] = [1] + [2] */
		tcg_gen_not_tl(tmp, cpu_astat_arg[1]);
		tcg_gen_setcond_tl(TCG_COND_LTU, tmp, tmp, cpu_astat_arg[2]);
		_gen_astat_store(ASTAT_AC0, tmp);
		_gen_astat_store(ASTAT_AC0_COPY, tmp);
		_gen_astat_update_nz(cpu_astat_arg[0], tmp);
		break;

	case ASTAT_OP_COMPARE_SIGNED: {
		TCGv flgs, flgo, overflow, flgn, res = tcg_temp_new();
		tcg_gen_sub_tl(res, cpu_astat_arg[0], cpu_astat_arg[1]);
		_gen_astat_update_az(res, tmp);
		tcg_gen_setcond_tl(TCG_COND_LEU, tmp, cpu_astat_arg[1], cpu_astat_arg[0]);
		_gen_astat_store(ASTAT_AC0, tmp);
		_gen_astat_store(ASTAT_AC0_COPY, tmp);
		/* XXX: This has got to be simpler ... */
		/* int flgs = srcop >> 31; */
		flgs = tcg_temp_new();
		tcg_gen_shri_tl(flgs, cpu_astat_arg[0], 31);
		/* int flgo = dstop >> 31; */
		flgo = tcg_temp_new();
		tcg_gen_shri_tl(flgo, cpu_astat_arg[1], 31);
		/* int flgn = result >> 31; */
		flgn = tcg_temp_new();
		tcg_gen_shri_tl(flgn, res, 31);
		/* int overflow = (flgs ^ flgo) & (flgn ^ flgs); */
		overflow = tcg_temp_new();
		tcg_gen_xor_tl(tmp, flgs, flgo);
		tcg_gen_xor_tl(overflow, flgn, flgs);
		tcg_gen_and_tl(overflow, tmp, overflow);
		/* an = (flgn && !overflow) || (!flgn && overflow); */
		tcg_gen_not_tl(tmp, overflow);
		tcg_gen_and_tl(tmp, flgn, tmp);
		tcg_gen_not_tl(res, flgn);
		tcg_gen_and_tl(res, res, overflow);
		tcg_gen_or_tl(tmp, tmp, res);
		tcg_temp_free(flgn);
		tcg_temp_free(overflow);
		tcg_temp_free(flgo);
		tcg_temp_free(flgs);
		tcg_temp_free(res);
		_gen_astat_store(ASTAT_AN, tmp);
		break;
	}

	case ASTAT_OP_COMPARE_UNSIGNED:
		tcg_gen_sub_tl(tmp, cpu_astat_arg[0], cpu_astat_arg[1]);
		_gen_astat_update_az(tmp, tmp);
		tcg_gen_setcond_tl(TCG_COND_LEU, tmp, cpu_astat_arg[1], cpu_astat_arg[0]);
		_gen_astat_store(ASTAT_AC0, tmp);
		_gen_astat_store(ASTAT_AC0_COPY, tmp);
		tcg_gen_setcond_tl(TCG_COND_GTU, tmp, cpu_astat_arg[1], cpu_astat_arg[0]);
		_gen_astat_store(ASTAT_AN, tmp);
		break;

	case ASTAT_OP_LOGICAL:
		tcg_gen_movi_tl(tmp, 0);
		/* AC0 is correct ? */
		_gen_astat_store(ASTAT_AC0, tmp);
		_gen_astat_store(ASTAT_AC0_COPY, tmp);
		_gen_astat_store(ASTAT_V, tmp);
		_gen_astat_store(ASTAT_V_COPY, tmp);
		_gen_astat_update_nz(cpu_astat_arg[0], tmp);
		break;

	case ASTAT_OP_LSHIFT:
		_gen_astat_update_az(cpu_astat_arg[0], tmp);
		/* XXX: should be checking bit shifted */
		tcg_gen_setcondi_tl(TCG_COND_GEU, tmp, cpu_astat_arg[0], 0x80000000);
		_gen_astat_store(ASTAT_AN, tmp);
		/* XXX: No saturation handling ... */
		tcg_gen_movi_tl(tmp, 0);
		_gen_astat_store(ASTAT_V, tmp);
		_gen_astat_store(ASTAT_V_COPY, tmp);
		break;

	case ASTAT_OP_LSHIFT_RT:
		_gen_astat_update_az(cpu_astat_arg[0], tmp);
		/* XXX: should be checking bit shifted */
		tcg_gen_setcondi_tl(TCG_COND_GEU, tmp, cpu_astat_arg[0], 0x80000000);
		_gen_astat_store(ASTAT_AN, tmp);
		tcg_gen_movi_tl(tmp, 0);
		_gen_astat_store(ASTAT_V, tmp);
		_gen_astat_store(ASTAT_V_COPY, tmp);
		break;

	case ASTAT_OP_MIN_MAX:	/* [0] = MAX/MIN( [1], [2] ) */
		tcg_gen_movi_tl(tmp, 0);
		_gen_astat_store(ASTAT_V, tmp);
		_gen_astat_store(ASTAT_V_COPY, tmp);
		_gen_astat_update_nz(cpu_astat_arg[0], tmp);
		break;

	case ASTAT_OP_SUB32:	/* [0] = [1] - [2] */
		tcg_gen_setcond_tl(TCG_COND_LEU, tmp, cpu_astat_arg[2], cpu_astat_arg[1]);
		_gen_astat_store(ASTAT_AC0, tmp);
		_gen_astat_store(ASTAT_AC0_COPY, tmp);
		_gen_astat_update_nz(cpu_astat_arg[0], tmp);
		break;

	default:
		fprintf(stderr, "qemu: unhandled astat op %u\n", dc->astat_op);
		abort();
	case ASTAT_OP_DYNAMIC:
	case ASTAT_OP_NONE:
		break;
	}

	tcg_temp_free(tmp);

	if (clear)
		dc->astat_op = ASTAT_OP_NONE;
//	tcg_gen_movi_tl(cpu_astat_op, dc->astat_op);
}

static void
_astat_queue_state(DisasContext *dc, enum astat_ops op, unsigned int num,
                   TCGv arg0, TCGv arg1, TCGv arg2)
{
	dc->astat_op = op;
	tcg_gen_movi_tl(cpu_astat_op, dc->astat_op);

	tcg_gen_mov_tl(cpu_astat_arg[0], arg0);
	if (num > 1)
		tcg_gen_mov_tl(cpu_astat_arg[1], arg1);
	else
		tcg_gen_discard_tl(cpu_astat_arg[1]);
	if (num > 2)
		tcg_gen_mov_tl(cpu_astat_arg[2], arg2);
	else
		tcg_gen_discard_tl(cpu_astat_arg[2]);

	//gen_astat_update(dc, true);
}
#define astat_queue_state1(dc, op, arg0)             _astat_queue_state(dc, op, 1, arg0, arg0, arg0)
#define astat_queue_state2(dc, op, arg0, arg1)       _astat_queue_state(dc, op, 2, arg0, arg1, arg1)
#define astat_queue_state3(dc, op, arg0, arg1, arg2) _astat_queue_state(dc, op, 3, arg0, arg1, arg2)

static void gen_astat_load(DisasContext *dc, TCGv reg)
{
	gen_astat_update(dc, true);
	gen_helper_astat_load(reg);
}

static void gen_astat_store(DisasContext *dc, TCGv reg)
{
	unsigned int i;

	gen_helper_astat_store(reg);

	dc->astat_op = ASTAT_OP_NONE;
	tcg_gen_movi_tl(cpu_astat_op, dc->astat_op);

	for (i = 0; i < ARRAY_SIZE(cpu_astat_arg); ++i)
		tcg_gen_discard_tl(cpu_astat_arg[i]);
}

static void interp_insn_bfin(DisasContext *dc);

static void check_breakpoint(CPUState *env, DisasContext *dc)
{
	CPUBreakpoint *bp;

	if (unlikely(!QTAILQ_EMPTY(&env->breakpoints))) {
		QTAILQ_FOREACH(bp, &env->breakpoints, entry) {
			if (bp->pc == dc->pc) {
				cec_exception(dc, EXCP_DEBUG);
				dc->is_jmp = DISAS_UPDATE;
			 }
		}
	}
}

static void
gen_intermediate_code_internal(CPUState *env, TranslationBlock *tb,
                               int search_pc)
{
	uint16_t *gen_opc_end;
	uint32_t pc_start;
	int j, lj;
	struct DisasContext ctx;
	struct DisasContext *dc = &ctx;
	uint32_t next_page_start;
	int num_insns;
	int max_insns;

	qemu_log_try_set_file(stderr);

	pc_start = tb->pc;
	dc->env = env;
	dc->tb = tb;
	/* XXX: handle super/user mode here.  */
	dc->mem_idx = 0;

	gen_opc_end = gen_opc_buf + OPC_MAX_SIZE;

	dc->is_jmp = DISAS_NEXT;
	dc->pc = pc_start;
	dc->astat_op = ASTAT_OP_DYNAMIC;
	dc->hwloop_callback = gen_hwloop_default;
//	dc->singlestep_enabled = env->singlestep_enabled;
//	dc->cpustate_changed = 0;

	next_page_start = (pc_start & TARGET_PAGE_MASK) + TARGET_PAGE_SIZE;
	lj = -1;
	num_insns = 0;
	max_insns = tb->cflags & CF_COUNT_MASK;
	if (max_insns == 0)
		max_insns = CF_COUNT_MASK;

	gen_icount_start();
	do {
#ifdef CONFIG_USER_ONLY
		/* Intercept jump to the magic kernel page.  */
		if ((dc->pc & 0xFFFFFF00) == 0x400) {
		}
#endif
		check_breakpoint(env, dc);

		if (search_pc) {
			j = gen_opc_ptr - gen_opc_buf;
			if (lj < j) {
				lj++;
				while (lj < j)
					gen_opc_instr_start[lj++] = 0;
			}
			gen_opc_pc[lj] = dc->pc;
			gen_opc_instr_start[lj] = 1;
			gen_opc_icount[lj] = num_insns;
		}

		if (num_insns + 1 == max_insns && (tb->cflags & CF_LAST_IO))
			gen_io_start();

		if (unlikely(qemu_loglevel_mask(CPU_LOG_TB_OP)))
			tcg_gen_debug_insn_start(dc->pc);

		interp_insn_bfin(dc);
		gen_hwloop_check(dc);
		dc->pc += dc->insn_len;

		++num_insns;
	} while (!dc->is_jmp && //!dc->cpustate_changed
		gen_opc_ptr < gen_opc_end &&
		!env->singlestep_enabled &&
		!singlestep &&
		dc->pc < next_page_start &&
		num_insns < max_insns);

	if (tb->cflags & CF_LAST_IO)
		gen_io_end();

	if (unlikely(env->singlestep_enabled)) {
		cec_exception(dc, EXCP_DEBUG);
//		if (dc->is_jmp == DISAS_NEXT)
//			tcg_gen_movi_tl(cpu_pc, dc->pc);
	} else {
		switch (dc->is_jmp) {
			case DISAS_NEXT:
				gen_gotoi_tb(dc, 1, dc->pc);
				break;
			default:
			case DISAS_UPDATE:
				/* indicate that the hash table must be used
				   to find the next TB */
				tcg_gen_exit_tb(0);
				break;
			case DISAS_CALL:
			case DISAS_JUMP:
			case DISAS_TB_JUMP:
				/* nothing more to generate */
				break;
		}
	}

	gen_icount_end(tb, num_insns);
	*gen_opc_ptr = INDEX_op_end;

	if (search_pc) {
		j = gen_opc_ptr - gen_opc_buf;
		lj++;
		while (lj <= j)
			gen_opc_instr_start[lj++] = 0;
	} else {
		tb->size = dc->pc - pc_start;
		tb->icount = num_insns;
	}

#ifdef DEBUG_DISAS
    if (qemu_loglevel_mask(CPU_LOG_TB_IN_ASM)) {
        qemu_log("----------------\n");
        qemu_log("IN: %s\n", lookup_symbol(pc_start));
        log_target_disas(pc_start, dc->pc - pc_start, 0);
        qemu_log("\n");
	}
#endif
}

void gen_intermediate_code(CPUState *env, struct TranslationBlock *tb)
{
	gen_intermediate_code_internal(env, tb, 0);
}

void gen_intermediate_code_pc(CPUState *env, struct TranslationBlock *tb)
{
	gen_intermediate_code_internal(env, tb, 1);
}

void gen_pc_load(CPUState *env, TranslationBlock *tb,
                 unsigned long searched_pc, int pc_pos, void *puc)
{
	env->pc = gen_opc_pc[pc_pos];
}

#include "bfin-sim.c"
