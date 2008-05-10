/*
 * Blackfin translation
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "disas/disas.h"
#include "exec/cpu_ldst.h"
#include "exec/exec-all.h"
#include "tcg-op.h"
#include "qemu-common.h"
#include "opcode/bfin.h"

#include "exec/helper-proto.h"
#include "exec/helper-gen.h"

#include "trace-tcg.h"
#include "exec/log.h"

typedef void (*hwloop_callback)(struct DisasContext *dc, int loop);

typedef struct DisasContext {
    CPUArchState *env;
    struct TranslationBlock *tb;
    /* The current PC we're decoding (could be middle of parallel insn) */
    target_ulong pc;
    /* Length of current insn (2/4/8) */
    target_ulong insn_len;

    /* For delayed ASTAT handling */
    enum astat_ops astat_op;

    /* For hardware loop processing */
    hwloop_callback hwloop_callback;
    void *hwloop_data;

    /* Was a DISALGNEXCPT used in this parallel insn ? */
    int disalgnexcpt;

    int is_jmp;
    int mem_idx;
} DisasContext;

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
static TCGv cpu_astat_arg[3];

#include "exec/gen-icount.h"

static inline void
bfin_tcg_new_set3(TCGv *tcgv, unsigned int cnt, unsigned int offbase,
                  const char * const *names)
{
    unsigned int i;
    for (i = 0; i < cnt; ++i) {
        tcgv[i] = tcg_global_mem_new(cpu_env, offbase + (i * 4), names[i]);
    }
}
#define bfin_tcg_new_set2(tcgv, cnt, reg, name_idx) \
    bfin_tcg_new_set3(tcgv, cnt, offsetof(CPUArchState, reg), \
                      &greg_names[name_idx])
#define bfin_tcg_new_set(reg, name_idx) \
    bfin_tcg_new_set2(cpu_##reg, ARRAY_SIZE(cpu_##reg), reg, name_idx)
#define bfin_tcg_new(reg, name_idx) \
    bfin_tcg_new_set2(&cpu_##reg, 1, reg, name_idx)

void bfin_translate_init(void)
{
    cpu_env = tcg_global_reg_new_ptr(TCG_AREG0, "env");

    cpu_pc = tcg_global_mem_new(cpu_env,
        offsetof(CPUArchState, pc), "PC");
    cpu_cc = tcg_global_mem_new(cpu_env,
        offsetof(CPUArchState, astat[ASTAT_CC]), "CC");

    cpu_astat_arg[0] = tcg_global_mem_new(cpu_env,
        offsetof(CPUArchState, astat_arg[0]), "astat_arg[0]");
    cpu_astat_arg[1] = tcg_global_mem_new(cpu_env,
        offsetof(CPUArchState, astat_arg[1]), "astat_arg[1]");
    cpu_astat_arg[2] = tcg_global_mem_new(cpu_env,
        offsetof(CPUArchState, astat_arg[2]), "astat_arg[2]");

    cpu_areg[0] = tcg_global_mem_new_i64(cpu_env,
        offsetof(CPUArchState, areg[0]), "A0");
    cpu_areg[1] = tcg_global_mem_new_i64(cpu_env,
        offsetof(CPUArchState, areg[1]), "A1");

    bfin_tcg_new_set(dreg, 0);
    bfin_tcg_new_set(preg, 8);
    bfin_tcg_new_set(ireg, 16);
    bfin_tcg_new_set(mreg, 20);
    bfin_tcg_new_set(breg, 24);
    bfin_tcg_new_set(lreg, 28);
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
}

#define _astat_printf(bit) cpu_fprintf(f, "%s" #bit " ", \
                                       (env->astat[ASTAT_##bit] ? "" : "~"))
void bfin_cpu_dump_state(CPUState *cs, FILE *f, fprintf_function cpu_fprintf,
                         int flags)
{
    BlackfinCPU *cpu = BFIN_CPU(cs);
    CPUBfinState *env = &cpu->env;

    cpu_fprintf(f, "              SYSCFG: %04x   SEQSTAT: %08x\n",
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
    cpu_fprintf(f, "  A0: %010"PRIx64"                  A1: %010"PRIx64"\n",
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
                env->astat_op, env->astat_arg[0], env->astat_arg[1],
                env->astat_arg[2]);
    cpu_fprintf(f, "              CYCLES: %08x %08x\n",
                env->cycles[0], env->cycles[1]);
}

static void gen_astat_update(DisasContext *, bool);

static void gen_goto_tb(DisasContext *dc, int tb_num, TCGv dest)
{
    gen_astat_update(dc, false);
    tcg_gen_mov_tl(cpu_pc, dest);
    tcg_gen_exit_tb(0);
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
    gen_helper_raise_exception(cpu_env, tmp, pc);
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

static void gen_align_check(DisasContext *dc, TCGv addr, uint32_t len,
                            bool inst)
{
    TCGv excp, pc, tmp;

    /* XXX: This should be made into a runtime option.  It adds likes
            10% overhead to memory intensive apps (like mp3 decoding). */
    if (1) {
        return;
    }

    excp = tcg_const_tl(inst ? EXCP_MISALIG_INST : EXCP_DATA_MISALGIN);
    pc = tcg_const_tl(dc->pc);
    tmp = tcg_const_tl(len);
    gen_helper_memalign(cpu_env, excp, pc, addr, tmp);
    tcg_temp_free(tmp);
    tcg_temp_free(pc);
    tcg_temp_free(excp);
}

static void gen_aligned_qemu_ld16u(DisasContext *dc, TCGv ret, TCGv addr)
{
    gen_align_check(dc, addr, 2, false);
    tcg_gen_qemu_ld16u(ret, addr, dc->mem_idx);
}

static void gen_aligned_qemu_ld16s(DisasContext *dc, TCGv ret, TCGv addr)
{
    gen_align_check(dc, addr, 2, false);
    tcg_gen_qemu_ld16s(ret, addr, dc->mem_idx);
}

static void gen_aligned_qemu_ld32u(DisasContext *dc, TCGv ret, TCGv addr)
{
    gen_align_check(dc, addr, 4, false);
    tcg_gen_qemu_ld32u(ret, addr, dc->mem_idx);
}

static void gen_aligned_qemu_st16(DisasContext *dc, TCGv val, TCGv addr)
{
    gen_align_check(dc, addr, 2, false);
    tcg_gen_qemu_st16(val, addr, dc->mem_idx);
}

static void gen_aligned_qemu_st32(DisasContext *dc, TCGv val, TCGv addr)
{
    gen_align_check(dc, addr, 4, false);
    tcg_gen_qemu_st32(val, addr, dc->mem_idx);
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
    if (!TCGV_EQUAL(reg, cpu_lbreg[0]) && !TCGV_EQUAL(reg, cpu_lbreg[1])) {
        return;
    }

    /* tb_invalidate_phys_page_range */
    dc->is_jmp = DISAS_UPDATE;
    /* XXX: Not entirely correct, but very few things load
     *      directly into LB ... */
    gen_gotoi_tb(dc, 0, dc->pc + dc->insn_len);
}

static void gen_hwloop_default(DisasContext *dc, int loop)
{
    if (loop != -1) {
        gen_goto_tb(dc, 0, cpu_ltreg[loop]);
    }
}

static void _gen_hwloop_call(DisasContext *dc, int loop)
{
    if (dc->is_jmp != DISAS_CALL) {
        return;
    }

    if (loop == -1) {
        tcg_gen_movi_tl(cpu_rets, dc->pc + dc->insn_len);
    } else {
        tcg_gen_mov_tl(cpu_rets, cpu_ltreg[loop]);
    }
}

static void gen_hwloop_br_pcrel_cc(DisasContext *dc, int loop)
{
    TCGLabel *l;
    int pcrel = (unsigned long)dc->hwloop_data;
    int T = pcrel & 1;
    pcrel &= ~1;

    l = gen_new_label();
    tcg_gen_brcondi_tl(TCG_COND_NE, cpu_cc, T, l);
    gen_gotoi_tb(dc, 0, dc->pc + pcrel);
    gen_set_label(l);
    if (loop == -1) {
        dc->hwloop_callback = gen_hwloop_default;
    } else {
        gen_hwloop_default(dc, loop);
    }
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

static void _gen_hwloop_check(DisasContext *dc, int loop, TCGLabel *l)
{
    tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_lcreg[loop], 0, l);
    tcg_gen_subi_tl(cpu_lcreg[loop], cpu_lcreg[loop], 1);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_lcreg[loop], 0, l);
    dc->hwloop_callback(dc, loop);
}

static void gen_hwloop_check(DisasContext *dc)
{
    bool loop1, loop0;
    TCGLabel *endl;

    loop1 = (dc->pc == dc->env->lbreg[1]);
    loop0 = (dc->pc == dc->env->lbreg[0]);

    if (loop1 || loop0) {
        endl = gen_new_label();
    }

    if (loop1) {
        TCGLabel *l;
        if (loop0) {
            l = gen_new_label();
        } else {
            l = endl;
        }

        _gen_hwloop_check(dc, 1, l);

        if (loop0) {
            tcg_gen_br(endl);
            gen_set_label(l);
        }
    }

    if (loop0) {
        _gen_hwloop_check(dc, 0, endl);
    }

    if (loop1 || loop0) {
        gen_set_label(endl);
    }

    dc->hwloop_callback(dc, -1);
}

/* R#.L = reg; R#.H = reg; */
/* XXX: This modifies the low source ... assumes it is a temp ... */
/*
static void gen_mov_l_h_tl(TCGv dst, TCGv srcl, TCGv srch)
{
    tcg_gen_shli_tl(dst, srch, 16);
    tcg_gen_andi_tl(srcl, srcl, 0xffff);
    tcg_gen_or_tl(dst, dst, srcl);
}
*/

/* R#.L = reg */
static void gen_mov_l_tl(TCGv dst, TCGv src)
{
    tcg_gen_deposit_tl(dst, dst, src, 0, 16);
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
static void gen_mov_h_tl(TCGv dst, TCGv src)
{
    tcg_gen_deposit_tl(dst, dst, src, 16, 16);
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

static void gen_extNsi_i64(TCGv_i64 dst, TCGv_i64 src, uint32_t n)
{
    /* Shift the sign bit up, and then back down */
    tcg_gen_shli_i64(dst, src, 64 - n);
    tcg_gen_sari_i64(dst, dst, 64 - n);
}

static void gen_abs_tl(TCGv ret, TCGv arg)
{
    TCGLabel *l = gen_new_label();
    tcg_gen_mov_tl(ret, arg);
    tcg_gen_brcondi_tl(TCG_COND_GE, arg, 0, l);
    tcg_gen_neg_tl(ret, ret);
    gen_set_label(l);
}

static void gen_abs_i64(TCGv_i64 ret, TCGv_i64 arg)
{
    TCGLabel *l = gen_new_label();
    tcg_gen_mov_i64(ret, arg);
    tcg_gen_brcondi_i64(TCG_COND_GE, arg, 0, l);
    tcg_gen_neg_i64(ret, ret);
    gen_set_label(l);
}

/* Common tail code for DIVQ/DIVS insns */
static void _gen_divqs(TCGv pquo, TCGv r, TCGv aq, TCGv div)
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
static void _gen_divqs_st_aq(TCGv r, TCGv aq, TCGv div)
{
    /* aq = (r ^ div) >> 15 */
    tcg_gen_xor_tl(aq, r, div);
    tcg_gen_shri_tl(aq, aq, 15);
    tcg_gen_andi_tl(aq, aq, 1);
    tcg_gen_st_tl(aq, cpu_env, offsetof(CPUArchState, astat[ASTAT_AQ]));
}

/* DIVQ ( Dreg, Dreg ) ;
 * Based on AQ status bit, either add or subtract the divisor from
 * the dividend. Then set the AQ status bit based on the MSBs of the
 * 32-bit dividend and the 16-bit divisor. Left shift the dividend one
 * bit. Copy the logical inverse of AQ into the dividend LSB.
 */
static void gen_divq(TCGv pquo, TCGv src)
{
    TCGLabel *l;
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
    tcg_gen_ld_tl(aq, cpu_env, offsetof(CPUArchState, astat[ASTAT_AQ]));

    l = gen_new_label();
    r = tcg_temp_local_new();
    tcg_gen_mov_tl(r, div);
    tcg_gen_brcondi_tl(TCG_COND_NE, aq, 0, l);
    tcg_gen_neg_tl(r, r);
    gen_set_label(l);
    tcg_gen_add_tl(r, r, af);

    tcg_temp_free(af);

    _gen_divqs_st_aq(r, aq, div);

    /* aq = !aq */
    tcg_gen_xori_tl(aq, aq, 1);

    _gen_divqs(pquo, r, aq, div);
}

/* DIVS ( Dreg, Dreg ) ;
 * Initialize for DIVQ. Set the AQ status bit based on the signs of
 * the 32-bit dividend and the 16-bit divisor. Left shift the dividend
 * one bit. Copy AQ into the dividend LSB.
 */
static void gen_divs(TCGv pquo, TCGv src)
{
    TCGv r, aq, div;

    /* div = R#.L */
    div = tcg_temp_local_new();
    tcg_gen_ext16u_tl(div, src);

    /* r = pquo >> 16 */
    r = tcg_temp_local_new();
    tcg_gen_shri_tl(r, pquo, 16);

    aq = tcg_temp_local_new();

    _gen_divqs_st_aq(r, aq, div);

    _gen_divqs(pquo, r, aq, div);
}

/* Reg = ROT reg BY reg/imm
 * The Blackfin rotate is not like the TCG rotate.  It shifts through the
 * CC bit too giving it 33 bits to play with.  So we have to reduce things
 * to shifts ourself.
 */
static void gen_rot_tl(TCGv dst, TCGv src, TCGv orig_shift)
{
    uint32_t nbits = 32;
    TCGv shift, ret, tmp, tmp_shift;
    TCGLabel *l, *endl;

    /* shift = CLAMP (shift, -nbits, nbits); */

    endl = gen_new_label();

    /* if (shift == 0) */
    l = gen_new_label();
    tcg_gen_brcondi_tl(TCG_COND_NE, orig_shift, 0, l);
    tcg_gen_mov_tl(dst, src);
    tcg_gen_br(endl);
    gen_set_label(l);

    /* Reduce everything to rotate left */
    shift = tcg_temp_local_new();
    tcg_gen_mov_tl(shift, orig_shift);
    l = gen_new_label();
    tcg_gen_brcondi_tl(TCG_COND_GE, shift, 0, l);
    tcg_gen_addi_tl(shift, shift, nbits + 1);
    gen_set_label(l);

    if (TCGV_EQUAL(dst, src)) {
        ret = tcg_temp_local_new();
    } else {
        ret = dst;
    }

    /* ret = shift == nbits ? 0 : val << shift; */
    tcg_gen_movi_tl(ret, 0);
    l = gen_new_label();
    tcg_gen_brcondi_tl(TCG_COND_EQ, shift, nbits, l);
    tcg_gen_shl_tl(ret, src, shift);
    gen_set_label(l);

    /* ret |= shift == 1 ? 0 : val >> ((nbits + 1) - shift); */
    l = gen_new_label();
    tcg_gen_brcondi_tl(TCG_COND_EQ, shift, 1, l);
    tmp = tcg_temp_new();
    tmp_shift = tcg_temp_new();
    tcg_gen_subfi_tl(tmp_shift, nbits + 1, shift);
    tcg_gen_shr_tl(tmp, src, tmp_shift);
    tcg_gen_or_tl(ret, ret, tmp);
    tcg_temp_free(tmp_shift);
    tcg_temp_free(tmp);
    gen_set_label(l);

    /* Then add in and output feedback via the CC register */
    tcg_gen_subi_tl(shift, shift, 1);
    tcg_gen_shl_tl(cpu_cc, cpu_cc, shift);
    tcg_gen_or_tl(ret, ret, cpu_cc);
    tcg_gen_subfi_tl(shift, nbits - 1, shift);
    tcg_gen_shr_tl(cpu_cc, src, shift);
    tcg_gen_andi_tl(cpu_cc, cpu_cc, 1);

    if (TCGV_EQUAL(dst, src)) {
        tcg_gen_mov_tl(dst, ret);
        tcg_temp_free(ret);
    }

    tcg_temp_free(shift);
    gen_set_label(endl);
}

static void gen_roti_tl(TCGv dst, TCGv src, int32_t shift)
{
    uint32_t nbits = 32;
    TCGv ret;

    /* shift = CLAMP (shift, -nbits, nbits); */

    if (shift == 0) {
        tcg_gen_mov_tl(dst, src);
        return;
    }

    /* Reduce everything to rotate left */
    if (shift < 0) {
        shift += nbits + 1;
    }

    if (TCGV_EQUAL(dst, src)) {
        ret = tcg_temp_new();
    } else {
        ret = dst;
    }

    /* First rotate the main register */
    if (shift == nbits) {
        tcg_gen_movi_tl(ret, 0);
    } else {
        tcg_gen_shli_tl(ret, src, shift);
    }
    if (shift != 1) {
        TCGv tmp = tcg_temp_new();
        tcg_gen_shri_tl(tmp, src, (nbits + 1) - shift);
        tcg_gen_or_tl(ret, ret, tmp);
        tcg_temp_free(tmp);
    }

    /* Then add in and output feedback via the CC register */
    tcg_gen_shli_tl(cpu_cc, cpu_cc, shift - 1);
    tcg_gen_or_tl(ret, ret, cpu_cc);
    tcg_gen_shri_tl(cpu_cc, src, nbits - shift);
    tcg_gen_andi_tl(cpu_cc, cpu_cc, 1);

    if (TCGV_EQUAL(dst, src)) {
        tcg_gen_mov_tl(dst, ret);
        tcg_temp_free(ret);
    }
}

static void gen_rot_i64(TCGv_i64 dst, TCGv_i64 src, TCGv_i64 orig_shift)
{
    uint32_t nbits = 40;
    TCGv_i64 shift, ret, tmp, tmp_shift, cc64;
    TCGLabel *l, *endl;

    /* shift = CLAMP (shift, -nbits, nbits); */

    endl = gen_new_label();

    /* if (shift == 0) */
    l = gen_new_label();
    tcg_gen_brcondi_i64(TCG_COND_NE, orig_shift, 0, l);
    tcg_gen_mov_i64(dst, src);
    tcg_gen_br(endl);
    gen_set_label(l);

    /* Reduce everything to rotate left */
    shift = tcg_temp_local_new_i64();
    tcg_gen_mov_i64(shift, orig_shift);
    l = gen_new_label();
    tcg_gen_brcondi_i64(TCG_COND_GE, shift, 0, l);
    tcg_gen_addi_i64(shift, shift, nbits + 1);
    gen_set_label(l);

    if (TCGV_EQUAL_I64(dst, src)) {
        ret = tcg_temp_local_new_i64();
    } else {
        ret = dst;
    }

    /* ret = shift == nbits ? 0 : val << shift; */
    tcg_gen_movi_i64(ret, 0);
    l = gen_new_label();
    tcg_gen_brcondi_i64(TCG_COND_EQ, shift, nbits, l);
    tcg_gen_shl_i64(ret, src, shift);
    gen_set_label(l);

    /* ret |= shift == 1 ? 0 : val >> ((nbits + 1) - shift); */
    l = gen_new_label();
    tcg_gen_brcondi_i64(TCG_COND_EQ, shift, 1, l);
    tmp = tcg_temp_new_i64();
    tmp_shift = tcg_temp_new_i64();
    tcg_gen_subfi_i64(tmp_shift, nbits + 1, shift);
    tcg_gen_shr_i64(tmp, src, tmp_shift);
    tcg_gen_or_i64(ret, ret, tmp);
    tcg_temp_free_i64(tmp_shift);
    tcg_temp_free_i64(tmp);
    gen_set_label(l);

    /* Then add in and output feedback via the CC register */
    cc64 = tcg_temp_new_i64();
    tcg_gen_ext_i32_i64(cc64, cpu_cc);
    tcg_gen_subi_i64(shift, shift, 1);
    tcg_gen_shl_i64(cc64, cc64, shift);
    tcg_gen_or_i64(ret, ret, cc64);
    tcg_gen_subfi_i64(shift, nbits - 1, shift);
    tcg_gen_shr_i64(cc64, src, shift);
    tcg_gen_andi_i64(cc64, cc64, 1);
    tcg_gen_extrl_i64_i32(cpu_cc, cc64);
    tcg_temp_free_i64(cc64);

    if (TCGV_EQUAL_I64(dst, src)) {
        tcg_gen_mov_i64(dst, ret);
        tcg_temp_free_i64(ret);
    }

    tcg_temp_free_i64(shift);
    gen_set_label(endl);
}

static void gen_roti_i64(TCGv_i64 dst, TCGv_i64 src, int32_t shift)
{
    uint32_t nbits = 40;
    TCGv_i64 ret, cc64;

    /* shift = CLAMP (shift, -nbits, nbits); */

    if (shift == 0) {
        tcg_gen_mov_i64(dst, src);
        return;
    }

    /* Reduce everything to rotate left */
    if (shift < 0) {
        shift += nbits + 1;
    }

    if (TCGV_EQUAL_I64(dst, src)) {
        ret = tcg_temp_new_i64();
    } else {
        ret = dst;
    }

    /* First rotate the main register */
    if (shift == nbits) {
        tcg_gen_movi_i64(ret, 0);
    } else {
        tcg_gen_shli_i64(ret, src, shift);
    }
    if (shift != 1) {
        TCGv_i64 tmp = tcg_temp_new_i64();
        tcg_gen_shri_i64(tmp, src, (nbits + 1) - shift);
        tcg_gen_or_i64(ret, ret, tmp);
        tcg_temp_free_i64(tmp);
    }

    /* Then add in and output feedback via the CC register */
    cc64 = tcg_temp_new_i64();
    tcg_gen_ext_i32_i64(cc64, cpu_cc);
    tcg_gen_shli_i64(cc64, cc64, shift - 1);
    tcg_gen_or_i64(ret, ret, cc64);
    tcg_gen_shri_i64(cc64, src, nbits - shift);
    tcg_gen_andi_i64(cc64, cc64, 1);
    tcg_gen_extrl_i64_i32(cpu_cc, cc64);
    tcg_temp_free_i64(cc64);

    if (TCGV_EQUAL_I64(dst, src)) {
        tcg_gen_mov_i64(dst, ret);
        tcg_temp_free_i64(ret);
    }
}

/* This is a bit crazy, but we want to simulate the hardware behavior exactly
   rather than worry about the circular buffers being used correctly.  Which
   isn't to say there isn't room for improvement here, just that we want to
   be conservative.  See also dagsub().  */
static void gen_dagadd(DisasContext *dc, int dagno, TCGv M)
{
    TCGLabel *l, *endl;

    /* Optimize for when circ buffers are not used */
    l = gen_new_label();
    endl = gen_new_label();
    tcg_gen_brcondi_tl(TCG_COND_NE, cpu_lreg[dagno], 0, l);
    tcg_gen_add_tl(cpu_ireg[dagno], cpu_ireg[dagno], M);
    tcg_gen_br(endl);
    gen_set_label(l);

    /* Fallback to the big guns */
    gen_helper_dagadd(cpu_ireg[dagno], cpu_ireg[dagno],
                      cpu_lreg[dagno], cpu_breg[dagno], M);

    gen_set_label(endl);
}

static void gen_dagaddi(DisasContext *dc, int dagno, uint32_t M)
{
    TCGv m = tcg_temp_local_new();
    tcg_gen_movi_tl(m, M);
    gen_dagadd(dc, dagno, m);
    tcg_temp_free(m);
}

/* See dagadd() notes above.  */
static void gen_dagsub(DisasContext *dc, int dagno, TCGv M)
{
    TCGLabel *l, *endl;

    /* Optimize for when circ buffers are not used */
    l = gen_new_label();
    endl = gen_new_label();
    tcg_gen_brcondi_tl(TCG_COND_NE, cpu_lreg[dagno], 0, l);
    tcg_gen_sub_tl(cpu_ireg[dagno], cpu_ireg[dagno], M);
    tcg_gen_br(endl);
    gen_set_label(l);

    /* Fallback to the big guns */
    gen_helper_dagsub(cpu_ireg[dagno], cpu_ireg[dagno],
                      cpu_lreg[dagno], cpu_breg[dagno], M);

    gen_set_label(endl);
}

static void gen_dagsubi(DisasContext *dc, int dagno, uint32_t M)
{
    TCGv m = tcg_temp_local_new();
    tcg_gen_movi_tl(m, M);
    gen_dagsub(dc, dagno, m);
    tcg_temp_free(m);
}

#define _gen_astat_store(bit, reg) \
    tcg_gen_st_tl(reg, cpu_env, offsetof(CPUArchState, astat[bit]))

static void _gen_astat_update_az(TCGv reg, TCGv tmp)
{
    tcg_gen_setcondi_tl(TCG_COND_EQ, tmp, reg, 0);
    _gen_astat_store(ASTAT_AZ, tmp);
}

static void _gen_astat_update_az2(TCGv reg, TCGv reg2, TCGv tmp)
{
    TCGv tmp2 = tcg_temp_new();
    tcg_gen_setcondi_tl(TCG_COND_EQ, tmp, reg, 0);
    tcg_gen_setcondi_tl(TCG_COND_EQ, tmp2, reg2, 0);
    tcg_gen_or_tl(tmp, tmp, tmp2);
    tcg_temp_free(tmp2);
    _gen_astat_store(ASTAT_AZ, tmp);
}

static void _gen_astat_update_an(TCGv reg, TCGv tmp, uint32_t len)
{
    tcg_gen_setcondi_tl(TCG_COND_GEU, tmp, reg, 1 << (len - 1));
    _gen_astat_store(ASTAT_AN, tmp);
}

static void _gen_astat_update_an2(TCGv reg, TCGv reg2, TCGv tmp, uint32_t len)
{
    TCGv tmp2 = tcg_temp_new();
    tcg_gen_setcondi_tl(TCG_COND_GEU, tmp, reg, 1 << (len - 1));
    tcg_gen_setcondi_tl(TCG_COND_GEU, tmp2, reg2, 1 << (len - 1));
    tcg_gen_or_tl(tmp, tmp, tmp2);
    tcg_temp_free(tmp2);
    _gen_astat_store(ASTAT_AN, tmp);
}

static void _gen_astat_update_nz(TCGv reg, TCGv tmp, uint32_t len)
{
    _gen_astat_update_az(reg, tmp);
    _gen_astat_update_an(reg, tmp, len);
}

static void _gen_astat_update_nz2(TCGv reg, TCGv reg2, TCGv tmp, uint32_t len)
{
    _gen_astat_update_az2(reg, reg2, tmp);
    _gen_astat_update_an2(reg, reg2, tmp, len);
}

static void gen_astat_update(DisasContext *dc, bool clear)
{
    TCGv tmp = tcg_temp_local_new();
    uint32_t len = 16;

    switch (dc->astat_op) {
    case ASTAT_OP_ABS:    /* [0] = ABS( [1] ) */
        len = 32;
        /* XXX: Missing V/VS updates */
        _gen_astat_update_nz(cpu_astat_arg[0], tmp, len);
        break;

    case ASTAT_OP_ABS_VECTOR: /* [0][1] = ABS( [2] ) (V) */
        /* XXX: Missing V/VS updates */
        _gen_astat_update_nz2(cpu_astat_arg[0], cpu_astat_arg[1], tmp, len);
        break;

    case ASTAT_OP_ADD32:    /* [0] = [1] + [2] */
        /* XXX: Missing V/VS updates */
        len = 32;
        tcg_gen_not_tl(tmp, cpu_astat_arg[1]);
        tcg_gen_setcond_tl(TCG_COND_LTU, tmp, tmp, cpu_astat_arg[2]);
        _gen_astat_store(ASTAT_AC0, tmp);
        _gen_astat_store(ASTAT_AC0_COPY, tmp);
        _gen_astat_update_nz(cpu_astat_arg[0], tmp, 32);
        break;

    case ASTAT_OP_ASHIFT32:
        len *= 2;
    case ASTAT_OP_ASHIFT16:
        tcg_gen_movi_tl(tmp, 0);
        /* Need to update AC0 ? */
        _gen_astat_store(ASTAT_V, tmp);
        _gen_astat_store(ASTAT_V_COPY, tmp);
        _gen_astat_update_nz(cpu_astat_arg[0], tmp, len);
        break;

    case ASTAT_OP_COMPARE_SIGNED: {
        TCGv flgs, flgo, overflow, flgn, res = tcg_temp_new();
        tcg_gen_sub_tl(res, cpu_astat_arg[0], cpu_astat_arg[1]);
        _gen_astat_update_az(res, tmp);
        tcg_gen_setcond_tl(TCG_COND_LEU, tmp, cpu_astat_arg[1],
                           cpu_astat_arg[0]);
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
        tcg_gen_setcond_tl(TCG_COND_LEU, tmp, cpu_astat_arg[1],
                           cpu_astat_arg[0]);
        _gen_astat_store(ASTAT_AC0, tmp);
        _gen_astat_store(ASTAT_AC0_COPY, tmp);
        tcg_gen_setcond_tl(TCG_COND_GTU, tmp, cpu_astat_arg[1],
                           cpu_astat_arg[0]);
        _gen_astat_store(ASTAT_AN, tmp);
        break;

    case ASTAT_OP_LOGICAL:
        len = 32;
        tcg_gen_movi_tl(tmp, 0);
        /* AC0 is correct ? */
        _gen_astat_store(ASTAT_AC0, tmp);
        _gen_astat_store(ASTAT_AC0_COPY, tmp);
        _gen_astat_store(ASTAT_V, tmp);
        _gen_astat_store(ASTAT_V_COPY, tmp);
        _gen_astat_update_nz(cpu_astat_arg[0], tmp, len);
        break;

    case ASTAT_OP_LSHIFT32:
        len *= 2;
    case ASTAT_OP_LSHIFT16:
        _gen_astat_update_az(cpu_astat_arg[0], tmp);
        /* XXX: should be checking bit shifted */
        tcg_gen_setcondi_tl(TCG_COND_GEU, tmp, cpu_astat_arg[0],
                            1 << (len - 1));
        _gen_astat_store(ASTAT_AN, tmp);
        /* XXX: No saturation handling ... */
        tcg_gen_movi_tl(tmp, 0);
        _gen_astat_store(ASTAT_V, tmp);
        _gen_astat_store(ASTAT_V_COPY, tmp);
        break;

    case ASTAT_OP_LSHIFT_RT32:
        len *= 2;
    case ASTAT_OP_LSHIFT_RT16:
        _gen_astat_update_az(cpu_astat_arg[0], tmp);
        /* XXX: should be checking bit shifted */
        tcg_gen_setcondi_tl(TCG_COND_GEU, tmp, cpu_astat_arg[0],
                            1 << (len - 1));
        _gen_astat_store(ASTAT_AN, tmp);
        tcg_gen_movi_tl(tmp, 0);
        _gen_astat_store(ASTAT_V, tmp);
        _gen_astat_store(ASTAT_V_COPY, tmp);
        break;

    case ASTAT_OP_MIN_MAX:    /* [0] = MAX/MIN( [1], [2] ) */
        tcg_gen_movi_tl(tmp, 0);
        _gen_astat_store(ASTAT_V, tmp);
        _gen_astat_store(ASTAT_V_COPY, tmp);
        _gen_astat_update_nz(cpu_astat_arg[0], tmp, 32);
        break;

    case ASTAT_OP_MIN_MAX_VECTOR: /* [0][1] = MAX/MIN( [2], [3] ) (V) */
        tcg_gen_movi_tl(tmp, 0);
        _gen_astat_store(ASTAT_V, tmp);
        _gen_astat_store(ASTAT_V_COPY, tmp);
        tcg_gen_sari_tl(cpu_astat_arg[0], cpu_astat_arg[0], 16);
        _gen_astat_update_nz2(cpu_astat_arg[0], cpu_astat_arg[1], tmp, 16);
        break;

    case ASTAT_OP_NEGATE:    /* [0] = -[1] */
        len = 32;
        _gen_astat_update_nz(cpu_astat_arg[0], tmp, 32);
        tcg_gen_setcondi_tl(TCG_COND_EQ, tmp, cpu_astat_arg[0], 1 << (len - 1));
        _gen_astat_store(ASTAT_V, tmp);
        /* XXX: Should "VS |= V;" */
        tcg_gen_setcondi_tl(TCG_COND_EQ, tmp, cpu_astat_arg[0], 0);
        _gen_astat_store(ASTAT_AC0, tmp);
        break;

    case ASTAT_OP_SUB32:    /* [0] = [1] - [2] */
        len = 32;
        /* XXX: Missing V/VS updates */
        tcg_gen_setcond_tl(TCG_COND_LEU, tmp, cpu_astat_arg[2],
                           cpu_astat_arg[1]);
        _gen_astat_store(ASTAT_AC0, tmp);
        _gen_astat_store(ASTAT_AC0_COPY, tmp);
        _gen_astat_update_nz(cpu_astat_arg[0], tmp, len);
        break;

    case ASTAT_OP_VECTOR_ADD_ADD:    /* [0][1] = [2] +|+ [3] */
    case ASTAT_OP_VECTOR_ADD_SUB:    /* [0][1] = [2] +|- [3] */
    case ASTAT_OP_VECTOR_SUB_SUB:    /* [0][1] = [2] -|- [3] */
    case ASTAT_OP_VECTOR_SUB_ADD:    /* [0][1] = [2] -|+ [3] */
        _gen_astat_update_az2(cpu_astat_arg[0], cpu_astat_arg[1], tmp);
        /* Need AN, AC0/AC1, V */
        break;

    default:
        fprintf(stderr, "qemu: unhandled astat op %u\n", dc->astat_op);
        abort();
    case ASTAT_OP_DYNAMIC:
    case ASTAT_OP_NONE:
        break;
    }

    tcg_temp_free(tmp);

    if (clear) {
        dc->astat_op = ASTAT_OP_NONE;
    }
}

static void
_astat_queue_state(DisasContext *dc, enum astat_ops op, unsigned int num,
                   TCGv arg0, TCGv arg1, TCGv arg2)
{
    dc->astat_op = op;

    tcg_gen_mov_tl(cpu_astat_arg[0], arg0);
    if (num > 1) {
        tcg_gen_mov_tl(cpu_astat_arg[1], arg1);
    } else {
        tcg_gen_discard_tl(cpu_astat_arg[1]);
    }
    if (num > 2) {
        tcg_gen_mov_tl(cpu_astat_arg[2], arg2);
    } else {
        tcg_gen_discard_tl(cpu_astat_arg[2]);
    }
}
#define astat_queue_state1(dc, op, arg0) \
    _astat_queue_state(dc, op, 1, arg0, arg0, arg0)
#define astat_queue_state2(dc, op, arg0, arg1) \
    _astat_queue_state(dc, op, 2, arg0, arg1, arg1)
#define astat_queue_state3(dc, op, arg0, arg1, arg2) \
    _astat_queue_state(dc, op, 3, arg0, arg1, arg2)

static void gen_astat_load(DisasContext *dc, TCGv reg)
{
    gen_astat_update(dc, true);
    gen_helper_astat_load(reg, cpu_env);
}

static void gen_astat_store(DisasContext *dc, TCGv reg)
{
    unsigned int i;

    gen_helper_astat_store(cpu_env, reg);

    dc->astat_op = ASTAT_OP_NONE;

    for (i = 0; i < ARRAY_SIZE(cpu_astat_arg); ++i) {
        tcg_gen_discard_tl(cpu_astat_arg[i]);
    }
}

static void interp_insn_bfin(DisasContext *dc);

void gen_intermediate_code(CPUArchState *env, TranslationBlock *tb)
{
    BlackfinCPU *cpu = bfin_env_get_cpu(env);
    CPUState *cs = CPU(cpu);
    uint32_t pc_start;
    struct DisasContext ctx;
    struct DisasContext *dc = &ctx;
    uint32_t next_page_start;
    int num_insns;
    int max_insns;

    pc_start = tb->pc;
    dc->env = env;
    dc->tb = tb;
    /* XXX: handle super/user mode here.  */
    dc->mem_idx = 0;

    dc->is_jmp = DISAS_NEXT;
    dc->pc = pc_start;
    dc->astat_op = ASTAT_OP_DYNAMIC;
    dc->hwloop_callback = gen_hwloop_default;
    dc->disalgnexcpt = 1;

    next_page_start = (pc_start & TARGET_PAGE_MASK) + TARGET_PAGE_SIZE;
    num_insns = 0;
    max_insns = tb->cflags & CF_COUNT_MASK;
    if (max_insns == 0) {
        max_insns = CF_COUNT_MASK;
    }
    if (max_insns > TCG_MAX_INSNS) {
        max_insns = TCG_MAX_INSNS;
    }

    gen_tb_start(tb);
    do {
#ifdef CONFIG_USER_ONLY
        /* Intercept jump to the magic kernel page.  The personality check is a
         * bit of a hack that let's us run standalone ELFs (which are all of the
         * tests.  This way only Linux ELFs get caught here.  */
        if (((dc->env->personality & 0xff/*PER_MASK*/) == 0/*PER_LINUX*/) &&
            (dc->pc & 0xFFFFFF00) == 0x400) {
            cec_exception(dc, EXCP_FIXED_CODE);
            break;
        }
#endif

        if (unlikely(cpu_breakpoint_test(cs, ctx.pc, BP_ANY))) {
            cec_exception(dc, EXCP_DEBUG);
            dc->is_jmp = DISAS_UPDATE;
        }
        tcg_gen_insn_start(dc->pc);
        ++num_insns;

        if (num_insns == max_insns && (tb->cflags & CF_LAST_IO)) {
            gen_io_start();
        }

        interp_insn_bfin(dc);
        gen_hwloop_check(dc);
        dc->pc += dc->insn_len;
    } while (!dc->is_jmp &&
             !tcg_op_buf_full() &&
             !cs->singlestep_enabled &&
             !singlestep &&
             dc->pc < next_page_start &&
             num_insns < max_insns);

    if (tb->cflags & CF_LAST_IO) {
        gen_io_end();
    }

    if (unlikely(cs->singlestep_enabled)) {
        cec_exception(dc, EXCP_DEBUG);
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

    gen_tb_end(tb, num_insns);

    tb->size = dc->pc - pc_start;
    tb->icount = num_insns;

#ifdef DEBUG_DISAS
    if (qemu_loglevel_mask(CPU_LOG_TB_IN_ASM)) {
        qemu_log("----------------\n");
        qemu_log("IN: %s\n", lookup_symbol(pc_start));
        log_target_disas(cs, pc_start, dc->pc - pc_start, 0);
        qemu_log("\n");
    }
#endif
}

void restore_state_to_opc(CPUArchState *env, TranslationBlock *tb,
                          target_ulong *data)
{
    env->pc = data[0];
}

#include "bfin-sim.c"
