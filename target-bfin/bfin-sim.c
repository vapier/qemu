/*
 * Simulator for Analog Devices Blackfin processors.
 *
 * Copyright 2005-2016 Mike Frysinger
 * Copyright 2005-2011 Analog Devices, Inc.
 *
 * Licensed under the GPL 2 or later.
 */

#include "qemu/osdep.h"

#define TRACE_EXTRACT(fmt, args...) \
do { \
    if (1) { \
        qemu_log_mask(CPU_LOG_TB_CPU, "%s: " fmt "\n", __func__, ## args); \
    } \
} while (0)

static void
illegal_instruction(DisasContext *dc)
{
    cec_exception(dc, EXCP_UNDEF_INST);
}

static void
unhandled_instruction(DisasContext *dc, const char *insn)
{
    qemu_log_mask(LOG_UNIMP, "unhandled insn: %s\n", insn);
    illegal_instruction(dc);
}

typedef enum {
    c_0, c_1, c_4, c_2, c_uimm2, c_uimm3, c_imm3, c_pcrel4,
    c_imm4, c_uimm4s4, c_uimm4s4d, c_uimm4, c_uimm4s2, c_negimm5s4, c_imm5,
    c_imm5d, c_uimm5, c_imm6, c_imm7, c_imm7d, c_imm8, c_uimm8, c_pcrel8,
    c_uimm8s4, c_pcrel8s4, c_lppcrel10, c_pcrel10, c_pcrel12, c_imm16s4,
    c_luimm16, c_imm16, c_imm16d, c_huimm16, c_rimm16, c_imm16s2, c_uimm16s4,
    c_uimm16s4d, c_uimm16, c_pcrel24, c_uimm32, c_imm32, c_huimm32, c_huimm32e,
} const_forms_t;

static const struct {
    const char *name;
    const int nbits;
    const char reloc;
    const char issigned;
    const char pcrel;
    const char scale;
    const char offset;
    const char negative;
    const char positive;
    const char decimal;
    const char leading;
    const char exact;
} constant_formats[] = {
    { "0",          0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "1",          0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "4",          0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "2",          0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "uimm2",      2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "uimm3",      3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "imm3",       3, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "pcrel4",     4, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    { "imm4",       4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "uimm4s4",    4, 0, 0, 0, 2, 0, 0, 1, 0, 0, 0},
    { "uimm4s4d",   4, 0, 0, 0, 2, 0, 0, 1, 1, 0, 0},
    { "uimm4",      4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "uimm4s2",    4, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    { "negimm5s4",  5, 0, 1, 0, 2, 0, 1, 0, 0, 0, 0},
    { "imm5",       5, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "imm5d",      5, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
    { "uimm5",      5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "imm6",       6, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "imm7",       7, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "imm7d",      7, 0, 1, 0, 0, 0, 0, 0, 1, 3, 0},
    { "imm8",       8, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "uimm8",      8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "pcrel8",     8, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    { "uimm8s4",    8, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
    { "pcrel8s4",   8, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0},
    { "lppcrel10", 10, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    { "pcrel10",   10, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    { "pcrel12",   12, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    { "imm16s4",   16, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0},
    { "luimm16",   16, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "imm16",     16, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "imm16d",    16, 0, 1, 0, 0, 0, 0, 0, 1, 3, 0},
    { "huimm16",   16, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "rimm16",    16, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { "imm16s2",   16, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0},
    { "uimm16s4",  16, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
    { "uimm16s4d", 16, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0},
    { "uimm16",    16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "pcrel24",   24, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    { "uimm32",    32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "imm32",     32, 0, 1, 0, 0, 0, 0, 0, 1, 3, 0},
    { "huimm32",   32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { "huimm32e",  32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
};

#define HOST_LONG_WORD_SIZE (sizeof(long) * 8)
#define SIGNEXTEND(v, n) (((int32_t)(v) << (HOST_LONG_WORD_SIZE - (n))) \
                          >> (HOST_LONG_WORD_SIZE - (n)))

static uint32_t
fmtconst_val(const_forms_t cf, uint32_t x)
{
    /* Negative constants have an implied sign bit.  */
    if (constant_formats[cf].negative) {
        int nb = constant_formats[cf].nbits + 1;
        x = x | (1 << constant_formats[cf].nbits);
        x = SIGNEXTEND(x, nb);
    } else if (constant_formats[cf].issigned) {
        x = SIGNEXTEND(x, constant_formats[cf].nbits);
    }

    x += constant_formats[cf].offset;
    x <<= constant_formats[cf].scale;

    return x;
}

#define uimm16s4(x)  fmtconst_val(c_uimm16s4, x)
#define uimm16s4d(x) fmtconst_val(c_uimm16s4d, x)
#define pcrel4(x)    fmtconst_val(c_pcrel4, x)
#define pcrel8(x)    fmtconst_val(c_pcrel8, x)
#define pcrel8s4(x)  fmtconst_val(c_pcrel8s4, x)
#define pcrel10(x)   fmtconst_val(c_pcrel10, x)
#define pcrel12(x)   fmtconst_val(c_pcrel12, x)
#define negimm5s4(x) fmtconst_val(c_negimm5s4, x)
#define rimm16(x)    fmtconst_val(c_rimm16, x)
#define huimm16(x)   fmtconst_val(c_huimm16, x)
#define imm16(x)     fmtconst_val(c_imm16, x)
#define imm16d(x)    fmtconst_val(c_imm16d, x)
#define uimm2(x)     fmtconst_val(c_uimm2, x)
#define uimm3(x)     fmtconst_val(c_uimm3, x)
#define luimm16(x)   fmtconst_val(c_luimm16, x)
#define uimm4(x)     fmtconst_val(c_uimm4, x)
#define uimm5(x)     fmtconst_val(c_uimm5, x)
#define imm16s2(x)   fmtconst_val(c_imm16s2, x)
#define uimm8(x)     fmtconst_val(c_uimm8, x)
#define imm16s4(x)   fmtconst_val(c_imm16s4, x)
#define uimm4s2(x)   fmtconst_val(c_uimm4s2, x)
#define uimm4s4(x)   fmtconst_val(c_uimm4s4, x)
#define uimm4s4d(x)  fmtconst_val(c_uimm4s4d, x)
#define lppcrel10(x) fmtconst_val(c_lppcrel10, x)
#define imm3(x)      fmtconst_val(c_imm3, x)
#define imm4(x)      fmtconst_val(c_imm4, x)
#define uimm8s4(x)   fmtconst_val(c_uimm8s4, x)
#define imm5(x)      fmtconst_val(c_imm5, x)
#define imm5d(x)     fmtconst_val(c_imm5d, x)
#define imm6(x)      fmtconst_val(c_imm6, x)
#define imm7(x)      fmtconst_val(c_imm7, x)
#define imm7d(x)     fmtconst_val(c_imm7d, x)
#define imm8(x)      fmtconst_val(c_imm8, x)
#define pcrel24(x)   fmtconst_val(c_pcrel24, x)
#define uimm16(x)    fmtconst_val(c_uimm16, x)
#define uimm32(x)    fmtconst_val(c_uimm32, x)
#define imm32(x)     fmtconst_val(c_imm32, x)
#define huimm32(x)   fmtconst_val(c_huimm32, x)
#define huimm32e(x)  fmtconst_val(c_huimm32e, x)

/* Table C-4. Core Register Encoding Map */
const char * const greg_names[] = {
    "R0",    "R1",      "R2",     "R3",    "R4",    "R5",    "R6",     "R7",
    "P0",    "P1",      "P2",     "P3",    "P4",    "P5",    "SP",     "FP",
    "I0",    "I1",      "I2",     "I3",    "M0",    "M1",    "M2",     "M3",
    "B0",    "B1",      "B2",     "B3",    "L0",    "L1",    "L2",     "L3",
    "A0.X",  "A0.W",    "A1.X",   "A1.W",  "<res>", "<res>", "ASTAT",  "RETS",
    "<res>", "<res>",   "<res>",  "<res>", "<res>", "<res>", "<res>",  "<res>",
    "LC0",   "LT0",     "LB0",    "LC1",   "LT1",   "LB1",   "CYCLES", "CYCLES2",
    "USP",   "SEQSTAT", "SYSCFG", "RETI",  "RETX",  "RETN",  "RETE",   "EMUDAT",
};

const char *
get_allreg_name(int grp, int reg)
{
    return greg_names[(grp << 3) | reg];
}

static TCGv * const cpu_regs[] = {
    &cpu_dreg[0], &cpu_dreg[1], &cpu_dreg[2], &cpu_dreg[3],
    &cpu_dreg[4], &cpu_dreg[5], &cpu_dreg[6], &cpu_dreg[7],
    &cpu_preg[0], &cpu_preg[1], &cpu_preg[2], &cpu_preg[3],
    &cpu_preg[4], &cpu_preg[5], &cpu_preg[6], &cpu_preg[7],
    &cpu_ireg[0], &cpu_ireg[1], &cpu_ireg[2], &cpu_ireg[3],
    &cpu_mreg[0], &cpu_mreg[1], &cpu_mreg[2], &cpu_mreg[3],
    &cpu_breg[0], &cpu_breg[1], &cpu_breg[2], &cpu_breg[3],
    &cpu_lreg[0], &cpu_lreg[1], &cpu_lreg[2], &cpu_lreg[3],
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cpu_rets,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    &cpu_lcreg[0], &cpu_ltreg[0], &cpu_lbreg[0], &cpu_lcreg[1],
    &cpu_ltreg[1], &cpu_lbreg[1], &cpu_cycles[0], &cpu_cycles[1],
    &cpu_uspreg, &cpu_seqstat, &cpu_syscfg, &cpu_reti,
    &cpu_retx, &cpu_retn, &cpu_rete, &cpu_emudat,
};

static TCGv
get_allreg(DisasContext *dc, int grp, int reg)
{
    TCGv *ret = cpu_regs[(grp << 3) | reg];
    if (ret) {
        return *ret;
    }
    /* We need to update all the callers to handle a NULL return.
       Use abort here so that we don't chase down random crashes. */
    abort();
    illegal_instruction(dc);
}

static void
reg_check_sup(DisasContext *dc, int grp, int reg)
{
    if (grp == 7) {
        cec_require_supervisor(dc);
    }
}

#define gen_unextend_acc(acc) tcg_gen_andi_i64(acc, acc, 0xffffffffffull)
#define gen_extend_acc(acc) gen_extNsi_i64(acc, acc, 40)

/* Perform a multiplication of D registers SRC0 and SRC1, sign- or
   zero-extending the result to 64 bit.  H0 and H1 determine whether the
   high part or the low part of the source registers is used.  Store 1 in
   *PSAT if saturation occurs, 0 otherwise.  */
static TCGv
decode_multfunc_tl(DisasContext *dc, int h0, int h1, int src0, int src1,
                   int mmod, int MM, TCGv psat)
{
    TCGv s0, s1, val;

    s0 = tcg_temp_local_new();
    if (h0) {
        tcg_gen_shri_tl(s0, cpu_dreg[src0], 16);
    } else {
        tcg_gen_andi_tl(s0, cpu_dreg[src0], 0xffff);
    }

    s1 = tcg_temp_local_new();
    if (h1) {
        tcg_gen_shri_tl(s1, cpu_dreg[src1], 16);
    } else {
        tcg_gen_andi_tl(s1, cpu_dreg[src1], 0xffff);
    }

    if (MM) {
        tcg_gen_ext16s_tl(s0, s0);
    } else {
        switch (mmod) {
        case 0:
        case M_S2RND:
        case M_T:
        case M_IS:
        case M_ISS2:
        case M_IH:
        case M_W32:
            tcg_gen_ext16s_tl(s0, s0);
            tcg_gen_ext16s_tl(s1, s1);
            break;
        case M_FU:
        case M_IU:
        case M_TFU:
            break;
        default:
            illegal_instruction(dc);
        }
    }

    val = tcg_temp_local_new();
    tcg_gen_mul_tl(val, s0, s1);
    tcg_temp_free(s0);
    tcg_temp_free(s1);

    /* Perform shift correction if appropriate for the mode.  */
    tcg_gen_movi_tl(psat, 0);
    if (!MM && (mmod == 0 || mmod == M_T || mmod == M_S2RND || mmod == M_W32)) {
        TCGLabel *l, *endl;

        l = gen_new_label();
        endl = gen_new_label();

        tcg_gen_brcondi_tl(TCG_COND_NE, val, 0x40000000, l);
        if (mmod == M_W32) {
            tcg_gen_movi_tl(val, 0x7fffffff);
        } else {
            tcg_gen_movi_tl(val, 0x80000000);
        }
        tcg_gen_movi_tl(psat, 1);
        tcg_gen_br(endl);

        gen_set_label(l);
        tcg_gen_shli_tl(val, val, 1);

        gen_set_label(endl);
    }

    return val;
}

static TCGv_i64
decode_multfunc_i64(DisasContext *dc, int h0, int h1, int src0, int src1,
                    int mmod, int MM, TCGv psat)
{
    TCGv val;
    TCGv_i64 val1;
    TCGLabel *l;

    val = decode_multfunc_tl(dc, h0, h1, src0, src1, mmod, MM, psat);
    val1 = tcg_temp_local_new_i64();
    tcg_gen_extu_i32_i64(val1, val);
    tcg_temp_free(val);

    if (mmod == 0 || mmod == M_IS || mmod == M_T || mmod == M_S2RND ||
        mmod == M_ISS2 || mmod == M_IH || (MM && mmod == M_FU)) {
        gen_extNsi_i64(val1, val1, 40);
    }

    l = gen_new_label();
    tcg_gen_brcondi_tl(TCG_COND_EQ, psat, 0, l);
    tcg_gen_ext32u_i64(val1, val1);
    gen_set_label(l);

    return val1;
}

static void
saturate_s32(TCGv_i64 val, TCGv overflow)
{
    TCGLabel *l, *endl;

    endl = gen_new_label();

    l = gen_new_label();
    tcg_gen_brcondi_i64(TCG_COND_GE, val, -0x80000000ll, l);
    tcg_gen_movi_tl(overflow, 1);
    tcg_gen_movi_i64(val, 0x80000000);
    tcg_gen_br(endl);
    gen_set_label(l);

    l = gen_new_label();
    tcg_gen_brcondi_i64(TCG_COND_LE, val, 0x7fffffff, l);
    tcg_gen_movi_tl(overflow, 1);
    tcg_gen_movi_i64(val, 0x7fffffff);
    gen_set_label(l);

    gen_set_label(endl);
}

static TCGv
decode_macfunc(DisasContext *dc, int which, int op, int h0, int h1, int src0,
               int src1, int mmod, int MM, int fullword, int *overflow)
{
    /* XXX: Very incomplete.  */
    TCGv_i64 acc;

    if (mmod == 0 || mmod == M_T || mmod == M_IS || mmod == M_ISS2 ||
        mmod == M_S2RND || mmod == M_IH || mmod == M_W32) {
        gen_extend_acc(cpu_areg[which]);
    } else {
        gen_unextend_acc(cpu_areg[which]);
    }
    acc = cpu_areg[which];

    if (op != 3) {
        /* this can't saturate, so we don't keep track of the sat flag */
        TCGv tsat = tcg_temp_local_new();;
        TCGv_i64 res = decode_multfunc_i64(dc, h0, h1, src0, src1, mmod, MM,
                                           tsat);
        tcg_temp_free(tsat);

        /* Perform accumulation.  */
        switch (op) {
        case 0:
            tcg_gen_mov_i64(acc, res);
            break;
        case 1:
            tcg_gen_add_i64(acc, acc, res);
            break;
        case 2:
            tcg_gen_sub_i64(acc, acc, res);
            break;
        }
        tcg_temp_free_i64(res);

        /* XXX: Saturate.  */
    }

    TCGv tmp = tcg_temp_local_new();
    tcg_gen_extrl_i64_i32(tmp, acc);
    return tmp;
}

static void
decode_ProgCtrl_0(DisasContext *dc, uint16_t iw0)
{
    /* ProgCtrl
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |.prgfunc.......|.poprnd........|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int poprnd  = ((iw0 >> ProgCtrl_poprnd_bits) & ProgCtrl_poprnd_mask);
    int prgfunc = ((iw0 >> ProgCtrl_prgfunc_bits) & ProgCtrl_prgfunc_mask);

    TRACE_EXTRACT("poprnd:%i prgfunc:%i", poprnd, prgfunc);

    if (prgfunc == 0 && poprnd == 0) {
        /* NOP */;
    } else if (prgfunc == 1 && poprnd == 0) {
        /* RTS; */
        dc->is_jmp = DISAS_JUMP;
        dc->hwloop_callback = gen_hwloop_br_direct;
        dc->hwloop_data = &cpu_rets;
    } else if (prgfunc == 1 && poprnd == 1) {
        /* RTI; */
        cec_require_supervisor(dc);
    } else if (prgfunc == 1 && poprnd == 2) {
        /* RTX; */
        cec_require_supervisor(dc);
    } else if (prgfunc == 1 && poprnd == 3) {
        /* RTN; */
        cec_require_supervisor(dc);
    } else if (prgfunc == 1 && poprnd == 4) {
        /* RTE; */
        cec_require_supervisor(dc);
    } else if (prgfunc == 2 && poprnd == 0) {
        /* IDLE; */
        /* just NOP it */;
    } else if (prgfunc == 2 && poprnd == 3) {
        /* CSYNC; */
        /* just NOP it */;
    } else if (prgfunc == 2 && poprnd == 4) {
        /* SSYNC; */
        /* just NOP it */;
    } else if (prgfunc == 2 && poprnd == 5) {
        /* EMUEXCPT; */
        cec_exception(dc, EXCP_DEBUG);
    } else if (prgfunc == 3 && poprnd < 8) {
        /* CLI Dreg{poprnd}; */
        cec_require_supervisor(dc);
    } else if (prgfunc == 4 && poprnd < 8) {
        /* STI Dreg{poprnd}; */
        cec_require_supervisor(dc);
    } else if (prgfunc == 5 && poprnd < 8) {
        /* JUMP (Preg{poprnd}); */
        dc->is_jmp = DISAS_JUMP;
        dc->hwloop_callback = gen_hwloop_br_direct;
        dc->hwloop_data = &cpu_preg[poprnd];
    } else if (prgfunc == 6 && poprnd < 8) {
        /* CALL (Preg{poprnd}); */
        dc->is_jmp = DISAS_CALL;
        dc->hwloop_callback = gen_hwloop_br_direct;
        dc->hwloop_data = &cpu_preg[poprnd];
    } else if (prgfunc == 7 && poprnd < 8) {
        /* CALL (PC + Preg{poprnd}); */
        dc->is_jmp = DISAS_CALL;
        dc->hwloop_callback = gen_hwloop_br_pcrel;
        dc->hwloop_data = &cpu_preg[poprnd];
    } else if (prgfunc == 8 && poprnd < 8) {
        /* JUMP (PC + Preg{poprnd}); */
        dc->is_jmp = DISAS_JUMP;
        dc->hwloop_callback = gen_hwloop_br_pcrel;
        dc->hwloop_data = &cpu_preg[poprnd];
    } else if (prgfunc == 9) {
        /* RAISE imm{poprnd}; */
        /* int raise = uimm4 (poprnd); */
        cec_require_supervisor(dc);
    } else if (prgfunc == 10) {
        /* EXCPT imm{poprnd}; */
        int excpt = uimm4(poprnd);
        cec_exception(dc, excpt);
    } else if (prgfunc == 11 && poprnd < 6) {
        /* TESTSET (Preg{poprnd}); */
        /* Note: This is isn't really atomic.  But that's OK because the
         * hardware itself can't be relied upon.  It doesn't work with cached
         * memory (which Linux always runs under), and has some anomalies
         * which prevents it from working even with on-chip memory.  As such,
         * we'll just do a "stupid" implementation here.  */
        TCGv tmp = tcg_temp_new();
        tcg_gen_qemu_ld8u(tmp, cpu_preg[poprnd], dc->mem_idx);
        tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_cc, tmp, 0);
        tcg_gen_ori_tl(tmp, tmp, 0x80);
        tcg_gen_qemu_st8(tmp, cpu_preg[poprnd], dc->mem_idx);
        tcg_temp_free(tmp);
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_CaCTRL_0(DisasContext *dc, uint16_t iw0)
{
    /* CaCTRL
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 1 |.a.|.op....|.reg.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int a   = ((iw0 >> CaCTRL_a_bits) & CaCTRL_a_mask);
    int op  = ((iw0 >> CaCTRL_op_bits) & CaCTRL_op_mask);
    int reg = ((iw0 >> CaCTRL_reg_bits) & CaCTRL_reg_mask);

    TRACE_EXTRACT("a:%i op:%i reg:%i", a, op, reg);

    /*
     * PREFETCH [Preg{reg}];
     * PREFETCH [Preg{reg}++{a}];
     * FLUSHINV [Preg{reg}];
     * FLUSHINV [Preg{reg}++{a}];
     * FLUSH [Preg{reg}];
     * FLUSH [Preg{reg}++{a}];
     * IFLUSH [Preg{reg}];
     * IFLUSH [Preg{reg}++{a}];
     */

    /* No cache simulation, and we'll ignore the implicit CPLB aspects */

    if (a) {
        tcg_gen_addi_tl(cpu_preg[reg], cpu_preg[reg], BFIN_L1_CACHE_BYTES);
    }
}

static void
decode_PushPopReg_0(DisasContext *dc, uint16_t iw0)
{
    /* PushPopReg
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 |.W.|.grp.......|.reg.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int W   = ((iw0 >> PushPopReg_W_bits) & PushPopReg_W_mask);
    int grp = ((iw0 >> PushPopReg_grp_bits) & PushPopReg_grp_mask);
    int reg = ((iw0 >> PushPopReg_reg_bits) & PushPopReg_reg_mask);
    bool check_lb = false;
    TCGv treg, tmp;
    TCGv_i64 tmp64;

    TRACE_EXTRACT("W:%i grp:%i reg:%i", W, grp, reg);

    reg_check_sup(dc, grp, reg);

    /* Everything here needs to be aligned, so check once */
    gen_align_check(dc, cpu_spreg, 4, false);

    if (W == 0) {
        /* Dreg and Preg are not supported by this instruction */
        if (grp == 0 || grp == 1) {
            illegal_instruction(dc);
            return;
        }

        /* genreg{grp,reg} [SP++]; */
        if (grp == 4 && reg == 6) {
            /* Pop ASTAT */
            tmp = tcg_temp_new();
            tcg_gen_qemu_ld32u(tmp, cpu_spreg, dc->mem_idx);
            gen_astat_store(dc, tmp);
            tcg_temp_free(tmp);
        } else if (grp == 4 && (reg == 0 || reg == 2)) {
            /* Pop A#.X */
            tmp = tcg_temp_new();
            tcg_gen_qemu_ld8u(tmp, cpu_spreg, dc->mem_idx);
            tmp64 = tcg_temp_new_i64();
            tcg_gen_extu_i32_i64(tmp64, tmp);
            tcg_temp_free(tmp);

            tcg_gen_deposit_i64(cpu_areg[reg >> 1], cpu_areg[reg >> 1],
                                tmp64, 32, 32);
            tcg_temp_free_i64(tmp64);
        } else if (grp == 4 && (reg == 1 || reg == 3)) {
            /* Pop A#.W */
            tmp = tcg_temp_new();
            tcg_gen_qemu_ld32u(tmp, cpu_spreg, dc->mem_idx);
            tmp64 = tcg_temp_new_i64();
            tcg_gen_extu_i32_i64(tmp64, tmp);
            tcg_temp_free(tmp);

            tcg_gen_deposit_i64(cpu_areg[reg >> 1], cpu_areg[reg >> 1],
                                tmp64, 0, 32);
            tcg_temp_free_i64(tmp64);
        } else {
            check_lb = true;
            treg = get_allreg(dc, grp, reg);
            tcg_gen_qemu_ld32u(treg, cpu_spreg, dc->mem_idx);

            if (grp == 6 && (reg == 1 || reg == 4)) {
                /* LT loads auto clear the LSB */
                tcg_gen_andi_tl(treg, treg, ~1);
            }
        }

        /* Delay the SP update till the end in case an exception occurs.  */
        tcg_gen_addi_tl(cpu_spreg, cpu_spreg, 4);
        if (check_lb) {
            gen_maybe_lb_exit_tb(dc, treg);
        }
    } else {
        /* [--SP] = genreg{grp,reg}; */
        TCGv tmp_sp;

        /* Delay the SP update till the end in case an exception occurs.  */
        tmp_sp = tcg_temp_new();
        tcg_gen_subi_tl(tmp_sp, cpu_spreg, 4);
        if (grp == 4 && reg == 6) {
            /* Push ASTAT */
            tmp = tcg_temp_new();
            gen_astat_load(dc, tmp);
            tcg_gen_qemu_st32(tmp, tmp_sp, dc->mem_idx);
            tcg_temp_free(tmp);
        } else if (grp == 4 && (reg == 0 || reg == 2)) {
            /* Push A#.X */
            tmp64 = tcg_temp_new_i64();
            tcg_gen_shri_i64(tmp64, cpu_areg[reg >> 1], 32);
            tmp = tcg_temp_new();
            tcg_gen_extrl_i64_i32(tmp, tmp64);
            tcg_temp_free_i64(tmp64);
            /* A# is a 40 bit reg and A#.X refers to the top 8 bits.  But we
             * don't always mask out the top 24 bits during operations because
             * we're lazy during intermediate operations (like maintaining the
             * sign bit).  */
            tcg_gen_andi_tl(tmp, tmp, 0xff);
            tcg_gen_qemu_st32(tmp, tmp_sp, dc->mem_idx);
            tcg_temp_free(tmp);
        } else if (grp == 4 && (reg == 1 || reg == 3)) {
            /* Push A#.W */
            tmp = tcg_temp_new();
            tcg_gen_extrl_i64_i32(tmp, cpu_areg[reg >> 1]);
            tcg_gen_qemu_st32(tmp, tmp_sp, dc->mem_idx);
            tcg_temp_free(tmp);
        } else {
            treg = get_allreg(dc, grp, reg);
            tcg_gen_qemu_st32(treg, tmp_sp, dc->mem_idx);
        }
        tcg_gen_mov_tl(cpu_spreg, tmp_sp);
        tcg_temp_free(tmp_sp);
    }
}

static void
decode_PushPopMultiple_0(DisasContext *dc, uint16_t iw0)
{
    /* PushPopMultiple
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 0 | 0 | 0 | 1 | 0 |.d.|.p.|.W.|.dr........|.pr........|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int p  = ((iw0 >> PushPopMultiple_p_bits) & PushPopMultiple_p_mask);
    int d  = ((iw0 >> PushPopMultiple_d_bits) & PushPopMultiple_d_mask);
    int W  = ((iw0 >> PushPopMultiple_W_bits) & PushPopMultiple_W_mask);
    int dr = ((iw0 >> PushPopMultiple_dr_bits) & PushPopMultiple_dr_mask);
    int pr = ((iw0 >> PushPopMultiple_pr_bits) & PushPopMultiple_pr_mask);
    int i;
    TCGv tmp_sp;

    TRACE_EXTRACT("d:%i p:%i W:%i dr:%i pr:%i", d, p, W, dr, pr);

    if ((d == 0 && p == 0) || (p && imm5(pr) > 5) ||
        (d && !p && pr) || (p && !d && dr)) {
        illegal_instruction(dc);
        return;
    }

    /* Everything here needs to be aligned, so check once */
    gen_align_check(dc, cpu_spreg, 4, false);

    /* Delay the SP update till the end in case an exception occurs.  */
    tmp_sp = tcg_temp_new();
    tcg_gen_mov_tl(tmp_sp, cpu_spreg);
    if (W == 1) {
        /* [--SP] = ({d}R7:imm{dr}, {p}P5:imm{pr}); */
        if (d) {
            for (i = dr; i < 8; i++) {
                tcg_gen_subi_tl(tmp_sp, tmp_sp, 4);
                tcg_gen_qemu_st32(cpu_dreg[i], tmp_sp, dc->mem_idx);
            }
        }
        if (p) {
            for (i = pr; i < 6; i++) {
                tcg_gen_subi_tl(tmp_sp, tmp_sp, 4);
                tcg_gen_qemu_st32(cpu_preg[i], tmp_sp, dc->mem_idx);
            }
        }
    } else {
        /* ({d}R7:imm{dr}, {p}P5:imm{pr}) = [SP++]; */
        if (p) {
            for (i = 5; i >= pr; i--) {
                tcg_gen_qemu_ld32u(cpu_preg[i], tmp_sp, dc->mem_idx);
                tcg_gen_addi_tl(tmp_sp, tmp_sp, 4);
            }
        }
        if (d) {
            for (i = 7; i >= dr; i--) {
                tcg_gen_qemu_ld32u(cpu_dreg[i], tmp_sp, dc->mem_idx);
                tcg_gen_addi_tl(tmp_sp, tmp_sp, 4);
            }
        }
    }
    tcg_gen_mov_tl(cpu_spreg, tmp_sp);
    tcg_temp_free(tmp_sp);
}

static void
decode_ccMV_0(DisasContext *dc, uint16_t iw0)
{
    /* ccMV
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 0 | 0 | 0 | 1 | 1 |.T.|.d.|.s.|.dst.......|.src.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int s  = ((iw0 >> CCmv_s_bits) & CCmv_s_mask);
    int d  = ((iw0 >> CCmv_d_bits) & CCmv_d_mask);
    int T  = ((iw0 >> CCmv_T_bits) & CCmv_T_mask);
    int src = ((iw0 >> CCmv_src_bits) & CCmv_src_mask);
    int dst = ((iw0 >> CCmv_dst_bits) & CCmv_dst_mask);
    TCGv tmp, reg_src, reg_dst;

    TRACE_EXTRACT("T:%i d:%i s:%i dst:%i src:%i",
                  T, d, s, dst, src);

    /* IF !{T} CC DPreg{d,dst} = DPreg{s,src}; */
    reg_src = get_allreg(dc, s, src);
    reg_dst = get_allreg(dc, d, dst);
    tmp = tcg_const_tl(T);
    tcg_gen_movcond_tl(TCG_COND_EQ, reg_dst, cpu_cc, tmp, reg_src, reg_dst);
    tcg_temp_free(tmp);
}

static void
decode_CCflag_0(DisasContext *dc, uint16_t iw0)
{
    /* CCflag
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 0 | 0 | 1 |.I.|.opc.......|.G.|.y.........|.x.........|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int x = ((iw0 >> CCflag_x_bits) & CCflag_x_mask);
    int y = ((iw0 >> CCflag_y_bits) & CCflag_y_mask);
    int I = ((iw0 >> CCflag_I_bits) & CCflag_I_mask);
    int G = ((iw0 >> CCflag_G_bits) & CCflag_G_mask);
    int opc = ((iw0 >> CCflag_opc_bits) & CCflag_opc_mask);

    TRACE_EXTRACT("I:%i opc:%i G:%i y:%i x:%i",
                  I, opc, G, y, x);

    if (opc > 4) {
        TCGv_i64 tmp64;
        TCGCond cond;

        if (opc == 5 && I == 0 && G == 0) {
            /* CC = A0 == A1; */
            cond = TCG_COND_EQ;
        } else if (opc == 6 && I == 0 && G == 0) {
            /* CC = A0 < A1; */
            cond = TCG_COND_LT;
        } else if (opc == 7 && I == 0 && G == 0) {
            /* CC = A0 <= A1; */
            cond = TCG_COND_LE;
        } else {
            illegal_instruction(dc);
            return;
        }

        tmp64 = tcg_temp_new_i64();
        tcg_gen_setcond_i64(cond, tmp64, cpu_areg[0], cpu_areg[1]);
        tcg_gen_extrl_i64_i32(cpu_cc, tmp64);
        tcg_temp_free_i64(tmp64);
    } else {
        int issigned = opc < 3;
        uint32_t dst_imm = issigned ? imm3(y) : uimm3(y);
        TCGv src_reg = G ? cpu_preg[x] : cpu_dreg[x];
        TCGv dst_reg = G ? cpu_preg[y] : cpu_dreg[y];
        TCGv tmp;
        TCGCond cond;
        enum astat_ops astat_op;

        switch (opc) {
        default: /* shutup useless gcc warnings */
        case 0: /* signed == */
            cond = TCG_COND_EQ;
            break;
        case 1: /* signed < */
            cond = TCG_COND_LT;
            break;
        case 2: /* signed <= */
            cond = TCG_COND_LE;
            break;
        case 3: /* unsigned < */
            cond = TCG_COND_LTU;
            break;
        case 4: /* unsigned <= */
            cond = TCG_COND_LEU;
            break;
        }
        if (issigned) {
            astat_op = ASTAT_OP_COMPARE_SIGNED;
        } else {
            astat_op = ASTAT_OP_COMPARE_UNSIGNED;
        }

        if (I) {
            /* Compare to an immediate rather than a reg */
            tmp = tcg_const_tl(dst_imm);
            dst_reg = tmp;
        }
        tcg_gen_setcond_tl(cond, cpu_cc, src_reg, dst_reg);

        /* Pointer compares only touch CC.  */
        if (!G) {
            astat_queue_state2(dc, astat_op, src_reg, dst_reg);
        }

        if (I) {
            tcg_temp_free(tmp);
        }
    }
}

static void
decode_CC2dreg_0(DisasContext *dc, uint16_t iw0)
{
    /* CC2dreg
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 0 |.op....|.reg.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int op  = ((iw0 >> CC2dreg_op_bits) & CC2dreg_op_mask);
    int reg = ((iw0 >> CC2dreg_reg_bits) & CC2dreg_reg_mask);

    TRACE_EXTRACT("op:%i reg:%i", op, reg);

    if (op == 0) {
        /* Dreg{reg} = CC; */
        tcg_gen_mov_tl(cpu_dreg[reg], cpu_cc);
    } else if (op == 1) {
        /* CC = Dreg{reg}; */
        tcg_gen_setcondi_tl(TCG_COND_NE, cpu_cc, cpu_dreg[reg], 0);
    } else if (op == 3 && reg == 0) {
        /* CC = !CC; */
        tcg_gen_xori_tl(cpu_cc, cpu_cc, 1);
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_CC2stat_0(DisasContext *dc, uint16_t iw0)
{
    /* CC2stat
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 1 |.D.|.op....|.cbit..............|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int D    = ((iw0 >> CC2stat_D_bits) & CC2stat_D_mask);
    int op   = ((iw0 >> CC2stat_op_bits) & CC2stat_op_mask);
    int cbit = ((iw0 >> CC2stat_cbit_bits) & CC2stat_cbit_mask);
    TCGv tmp;

    TRACE_EXTRACT("D:%i op:%i cbit:%i", D, op, cbit);

    /* CC = CC; is invalid.  */
    if (cbit == 5) {
        illegal_instruction(dc);
        return;
    }

    gen_astat_update(dc, true);

    if (D == 0) {
        switch (op) {
        case 0: /* CC = ASTAT[cbit] */
            tcg_gen_ld_tl(cpu_cc, cpu_env, offsetof(CPUArchState, astat[cbit]));
            break;
        case 1: /* CC |= ASTAT[cbit] */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUArchState, astat[cbit]));
            tcg_gen_or_tl(cpu_cc, cpu_cc, tmp);
            tcg_temp_free(tmp);
            break;
        case 2: /* CC &= ASTAT[cbit] */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUArchState, astat[cbit]));
            tcg_gen_and_tl(cpu_cc, cpu_cc, tmp);
            tcg_temp_free(tmp);
            break;
        case 3: /* CC ^= ASTAT[cbit] */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUArchState, astat[cbit]));
            tcg_gen_xor_tl(cpu_cc, cpu_cc, tmp);
            tcg_temp_free(tmp);
            break;
        }
    } else {
        switch (op) {
        case 0: /* ASTAT[cbit] = CC */
            tcg_gen_st_tl(cpu_cc, cpu_env, offsetof(CPUArchState, astat[cbit]));
            break;
        case 1: /* ASTAT[cbit] |= CC */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUArchState, astat[cbit]));
            tcg_gen_or_tl(tmp, tmp, cpu_cc);
            tcg_gen_st_tl(tmp, cpu_env, offsetof(CPUArchState, astat[cbit]));
            tcg_temp_free(tmp);
            break;
        case 2: /* ASTAT[cbit] &= CC */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUArchState, astat[cbit]));
            tcg_gen_and_tl(tmp, tmp, cpu_cc);
            tcg_gen_st_tl(tmp, cpu_env, offsetof(CPUArchState, astat[cbit]));
            tcg_temp_free(tmp);
            break;
        case 3: /* ASTAT[cbit] ^= CC */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUArchState, astat[cbit]));
            tcg_gen_xor_tl(tmp, tmp, cpu_cc);
            tcg_gen_st_tl(tmp, cpu_env, offsetof(CPUArchState, astat[cbit]));
            tcg_temp_free(tmp);
            break;
        }
    }
}

static void
decode_BRCC_0(DisasContext *dc, uint16_t iw0)
{
    /* BRCC
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 0 | 1 |.T.|.B.|.offset................................|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int B = ((iw0 >> BRCC_B_bits) & BRCC_B_mask);
    int T = ((iw0 >> BRCC_T_bits) & BRCC_T_mask);
    int offset = ((iw0 >> BRCC_offset_bits) & BRCC_offset_mask);
    int pcrel = pcrel10(offset);

    TRACE_EXTRACT("T:%i B:%i offset:%#x", T, B, offset);

    /* IF !{T} CC JUMP imm{offset} (bp){B}; */
    dc->hwloop_callback = gen_hwloop_br_pcrel_cc;
    dc->hwloop_data = (void *)(unsigned long)(pcrel | T);
}

static void
decode_UJUMP_0(DisasContext *dc, uint16_t iw0)
{
    /* UJUMP
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 1 | 0 |.offset........................................|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int offset = ((iw0 >> UJump_offset_bits) & UJump_offset_mask);
    int pcrel = pcrel12(offset);

    TRACE_EXTRACT("offset:%#x", offset);

    /* JUMP.S imm{offset}; */
    dc->is_jmp = DISAS_JUMP;
    dc->hwloop_callback = gen_hwloop_br_pcrel_imm;
    dc->hwloop_data = (void *)(unsigned long)pcrel;
}

static void
decode_REGMV_0(DisasContext *dc, uint16_t iw0)
{
    /* REGMV
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 0 | 1 | 1 |.gd........|.gs........|.dst.......|.src.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int gs  = ((iw0 >> RegMv_gs_bits) & RegMv_gs_mask);
    int gd  = ((iw0 >> RegMv_gd_bits) & RegMv_gd_mask);
    int src = ((iw0 >> RegMv_src_bits) & RegMv_src_mask);
    int dst = ((iw0 >> RegMv_dst_bits) & RegMv_dst_mask);
    TCGv reg_src, reg_dst, tmp;
    TCGv_i64 tmp64;
    bool istmp;

    TRACE_EXTRACT("gd:%i gs:%i dst:%i src:%i",
                  gd, gs, dst, src);

    /* genreg{gd,dst} = genreg{gs,src}; */

    reg_check_sup(dc, gs, src);
    reg_check_sup(dc, gd, dst);

    if (gs == 4 && src == 6) {
        /* Reads of ASTAT */
        tmp = tcg_temp_new();
        gen_astat_load(dc, tmp);
        reg_src = tmp;
        istmp = true;
    } else if (gs == 4 && (src == 0 || src == 2)) {
        /* Reads of A#.X */
        tmp = tcg_temp_new();
        tmp64 = tcg_temp_new_i64();
        tcg_gen_shri_i64(tmp64, cpu_areg[src >> 1], 32);
        tcg_gen_extrl_i64_i32(tmp, tmp64);
        tcg_temp_free_i64(tmp64);
        tcg_gen_ext8s_tl(tmp, tmp);
        reg_src = tmp;
        istmp = true;
    } else if (gs == 4 && (src == 1 || src == 3)) {
        /* Reads of A#.W */
        tmp = tcg_temp_new();
        tcg_gen_extrl_i64_i32(tmp, cpu_areg[src >> 1]);
        reg_src = tmp;
        istmp = true;
    } else if (gs == 6 && src == 6) {
        /* Reads of CYCLES cascades changes w/CYCLES2. */
        tmp = tcg_temp_new();
        gen_helper_cycles_read(tmp, cpu_env);
        reg_src = tmp;
        istmp = true;
    } else {
        reg_src = get_allreg(dc, gs, src);
        istmp = false;
    }

    if (gd == 4 && dst == 6) {
        /* Writes to ASTAT */
        gen_astat_store(dc, reg_src);
    } else if (gd == 4 && (dst == 0 || dst == 2)) {
        /* Writes to A#.X */
        tmp64 = tcg_temp_new_i64();
        tcg_gen_extu_i32_i64(tmp64, reg_src);
        tcg_gen_deposit_i64(cpu_areg[dst >> 1], cpu_areg[dst >> 1], tmp64,
                            32, 8);
        tcg_temp_free_i64(tmp64);
    } else if (gd == 4 && (dst == 1 || dst == 3)) {
        /* Writes to A#.W */
        tcg_gen_andi_i64(cpu_areg[dst >> 1], cpu_areg[dst >> 1], 0xff00000000);
        tmp64 = tcg_temp_new_i64();
        tcg_gen_extu_i32_i64(tmp64, reg_src);
        tcg_gen_or_i64(cpu_areg[dst >> 1], cpu_areg[dst >> 1], tmp64);
        tcg_temp_free_i64(tmp64);
    } else if (gd == 6 && (dst == 1 || dst == 4)) {
        /* Writes to LT# */
        /* LT loads auto clear the LSB */
        tcg_gen_andi_tl(cpu_ltreg[dst >> 2], reg_src, ~1);
    } else {
        reg_dst = get_allreg(dc, gd, dst);
        tcg_gen_mov_tl(reg_dst, reg_src);
        gen_maybe_lb_exit_tb(dc, reg_dst);
    }

    if (istmp) {
        tcg_temp_free(tmp);
    }
}

static void
decode_ALU2op_0(DisasContext *dc, uint16_t iw0)
{
    /* ALU2op
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 1 | 0 | 0 | 0 | 0 |.opc...........|.src.......|.dst.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int src = ((iw0 >> ALU2op_src_bits) & ALU2op_src_mask);
    int opc = ((iw0 >> ALU2op_opc_bits) & ALU2op_opc_mask);
    int dst = ((iw0 >> ALU2op_dst_bits) & ALU2op_dst_mask);
    TCGLabel *l;
    TCGv tmp;

    TRACE_EXTRACT("opc:%i src:%i dst:%i", opc, src, dst);

    if (opc == 0) {
        /* Dreg{dst} >>>= Dreg{src}; */
        tmp = tcg_temp_local_new();

        /* Clip the shift magnitude to 31 bits */
        tcg_gen_movi_tl(tmp, 31);
        tcg_gen_movcond_tl(TCG_COND_LTU, tmp, cpu_dreg[src], tmp, cpu_dreg[src], tmp);

        tcg_gen_sar_tl(cpu_dreg[dst], cpu_dreg[dst], tmp);

        tcg_temp_free(tmp);

        astat_queue_state1(dc, ASTAT_OP_LSHIFT_RT32, cpu_dreg[dst]);
    } else if (opc == 1) {
        /* Dreg{dst} >>= Dreg{src}; */
        l = gen_new_label();
        tmp = tcg_temp_local_new();

        /* Clip the shift magnitude to 31 bits */
        tcg_gen_mov_tl(tmp, cpu_dreg[src]);
        tcg_gen_brcondi_tl(TCG_COND_LEU, tmp, 31, l);
        tcg_gen_movi_tl(tmp, 0);
        tcg_gen_mov_tl(cpu_dreg[dst], tmp);
        gen_set_label(l);

        tcg_gen_shr_tl(cpu_dreg[dst], cpu_dreg[dst], tmp);

        tcg_temp_free(tmp);

        astat_queue_state1(dc, ASTAT_OP_LSHIFT_RT32, cpu_dreg[dst]);
    } else if (opc == 2) {
        /* Dreg{dst} <<= Dreg{src}; */
        l = gen_new_label();
        tmp = tcg_temp_local_new();

        /* Clip the shift magnitude to 31 bits */
        tcg_gen_mov_tl(tmp, cpu_dreg[src]);
        tcg_gen_brcondi_tl(TCG_COND_LEU, tmp, 31, l);
        tcg_gen_movi_tl(tmp, 0);
        tcg_gen_mov_tl(cpu_dreg[dst], tmp);
        gen_set_label(l);

        tcg_gen_shl_tl(cpu_dreg[dst], cpu_dreg[dst], tmp);

        tcg_temp_free(tmp);

        astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst]);
    } else if (opc == 3) {
        /* Dreg{dst} *= Dreg{src}; */
        tcg_gen_mul_tl(cpu_dreg[dst], cpu_dreg[dst], cpu_dreg[src]);
    } else if (opc == 4 || opc == 5) {
        /* Dreg{dst} = (Dreg{dst} + Dreg{src}) << imm{opc}; */
        tcg_gen_add_tl(cpu_dreg[dst], cpu_dreg[dst], cpu_dreg[src]);
        tcg_gen_shli_tl(cpu_dreg[dst], cpu_dreg[dst], (opc - 3));
        astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst]);
    } else if (opc == 8) {
        /* DIVQ (Dreg, Dreg); */
        gen_divq(cpu_dreg[dst], cpu_dreg[src]);
    } else if (opc == 9) {
        /* DIVS (Dreg, Dreg); */
        gen_divs(cpu_dreg[dst], cpu_dreg[src]);
    } else if (opc == 10) {
        /* Dreg{dst} = Dreg_lo{src} (X); */
        tcg_gen_ext16s_tl(cpu_dreg[dst], cpu_dreg[src]);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 11) {
        /* Dreg{dst} = Dreg_lo{src} (Z); */
        tcg_gen_ext16u_tl(cpu_dreg[dst], cpu_dreg[src]);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 12) {
        /* Dreg{dst} = Dreg_byte{src} (X); */
        tcg_gen_ext8s_tl(cpu_dreg[dst], cpu_dreg[src]);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 13) {
        /* Dreg{dst} = Dreg_byte{src} (Z); */
        tcg_gen_ext8u_tl(cpu_dreg[dst], cpu_dreg[src]);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 14) {
        /* Dreg{dst} = -Dreg{src}; */
        /* XXX: Documentation isn't entirely clear about av0 and av1.  */
        tcg_gen_neg_tl(cpu_dreg[dst], cpu_dreg[src]);
        astat_queue_state1(dc, ASTAT_OP_NEGATE, cpu_dreg[dst]);
    } else if (opc == 15) {
        /* Dreg = ~Dreg; */
        tcg_gen_not_tl(cpu_dreg[dst], cpu_dreg[src]);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    }
}

static void
decode_PTR2op_0(DisasContext *dc, uint16_t iw0)
{
    /* PTR2op
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 1 | 0 | 0 | 0 | 1 | 0 |.opc.......|.src.......|.dst.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int src = ((iw0 >> PTR2op_src_bits) & PTR2op_dst_mask);
    int opc = ((iw0 >> PTR2op_opc_bits) & PTR2op_opc_mask);
    int dst = ((iw0 >> PTR2op_dst_bits) & PTR2op_dst_mask);

    TRACE_EXTRACT("opc:%i src:%i dst:%i", opc, src, dst);

    if (opc == 0) {
        /* Preg{dst} -= Preg{src}; */
        tcg_gen_sub_tl(cpu_preg[dst], cpu_preg[dst], cpu_preg[src]);
    } else if (opc == 1) {
        /* Preg{dst} = Preg{src} << 2; */
        tcg_gen_shli_tl(cpu_preg[dst], cpu_preg[src], 2);
    } else if (opc == 3) {
        /* Preg{dst} = Preg{src} >> 2; */
        tcg_gen_shri_tl(cpu_preg[dst], cpu_preg[src], 2);
    } else if (opc == 4) {
        /* Preg{dst} = Preg{src} >> 1; */
        tcg_gen_shri_tl(cpu_preg[dst], cpu_preg[src], 1);
    } else if (opc == 5) {
        /* Preg{dst} += Preg{src} (BREV); */
        gen_helper_add_brev(cpu_preg[dst], cpu_preg[dst], cpu_preg[src]);
    } else { /*(opc == 6 || opc == 7)*/
        /* Preg{dst} = (Preg{dst} + Preg{src}) << imm{opc}; */
        tcg_gen_add_tl(cpu_preg[dst], cpu_preg[dst], cpu_preg[src]);
        tcg_gen_shli_tl(cpu_preg[dst], cpu_preg[dst], (opc - 5));
    }
}

static void
decode_LOGI2op_0(DisasContext *dc, uint16_t iw0)
{
    /* LOGI2op
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 1 | 0 | 0 | 1 |.opc.......|.src...............|.dst.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int src = ((iw0 >> LOGI2op_src_bits) & LOGI2op_src_mask);
    int opc = ((iw0 >> LOGI2op_opc_bits) & LOGI2op_opc_mask);
    int dst = ((iw0 >> LOGI2op_dst_bits) & LOGI2op_dst_mask);
    int uimm = uimm5(src);

    TRACE_EXTRACT("opc:%i src:%i dst:%i", opc, src, dst);

    if (opc == 0) {
        /* CC = ! BITTST (Dreg{dst}, imm{uimm}); */
        tcg_gen_shri_tl(cpu_cc, cpu_dreg[dst], uimm);
        tcg_gen_not_tl(cpu_cc, cpu_cc);
        tcg_gen_andi_tl(cpu_cc, cpu_cc, 1);
    } else if (opc == 1) {
        /* CC = BITTST (Dreg{dst}, imm{uimm}); */
        tcg_gen_shri_tl(cpu_cc, cpu_dreg[dst], uimm);
        tcg_gen_andi_tl(cpu_cc, cpu_cc, 1);
    } else if (opc == 2) {
        /* BITSET (Dreg{dst}, imm{uimm}); */
        tcg_gen_ori_tl(cpu_dreg[dst], cpu_dreg[dst], 1 << uimm);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 3) {
        /* BITTGL (Dreg{dst}, imm{uimm}); */
        tcg_gen_xori_tl(cpu_dreg[dst], cpu_dreg[dst], 1 << uimm);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 4) {
        /* BITCLR (Dreg{dst}, imm{uimm}); */
        tcg_gen_andi_tl(cpu_dreg[dst], cpu_dreg[dst], ~(1 << uimm));
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 5) {
        /* Dreg{dst} >>>= imm{uimm}; */
        tcg_gen_sari_tl(cpu_dreg[dst], cpu_dreg[dst], uimm);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT_RT32, cpu_dreg[dst]);
    } else if (opc == 6) {
        /* Dreg{dst} >>= imm{uimm}; */
        tcg_gen_shri_tl(cpu_dreg[dst], cpu_dreg[dst], uimm);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT_RT32, cpu_dreg[dst]);
    } else { /*(opc == 7)*/
        /* Dreg{dst} <<= imm{uimm}; */
        tcg_gen_shli_tl(cpu_dreg[dst], cpu_dreg[dst], uimm);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst]);
    }
}

static void
decode_COMP3op_0(DisasContext *dc, uint16_t iw0)
{
    /* COMP3op
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 1 | 0 | 1 |.opc.......|.dst.......|.src1......|.src0......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int opc  = ((iw0 >> COMP3op_opc_bits) & COMP3op_opc_mask);
    int dst  = ((iw0 >> COMP3op_dst_bits) & COMP3op_dst_mask);
    int src0 = ((iw0 >> COMP3op_src0_bits) & COMP3op_src0_mask);
    int src1 = ((iw0 >> COMP3op_src1_bits) & COMP3op_src1_mask);
    TCGv tmp;

    TRACE_EXTRACT("opc:%i dst:%i src1:%i src0:%i",
                  opc, dst, src1, src0);

    tmp = tcg_temp_local_new();
    if (opc == 0) {
        /* Dreg{dst} = Dreg{src0} + Dreg{src1}; */
        tcg_gen_add_tl(tmp, cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state3(dc, ASTAT_OP_ADD32, tmp, cpu_dreg[src0],
                           cpu_dreg[src1]);
        tcg_gen_mov_tl(cpu_dreg[dst], tmp);
    } else if (opc == 1) {
        /* Dreg{dst} = Dreg{src0} - Dreg{src1}; */
        tcg_gen_sub_tl(tmp, cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state3(dc, ASTAT_OP_SUB32, tmp, cpu_dreg[src0],
                           cpu_dreg[src1]);
        tcg_gen_mov_tl(cpu_dreg[dst], tmp);
    } else if (opc == 2) {
        /* Dreg{dst} = Dreg{src0} & Dreg{src1}; */
        tcg_gen_and_tl(cpu_dreg[dst], cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 3) {
        /* Dreg{dst} = Dreg{src0} | Dreg{src1}; */
        tcg_gen_or_tl(cpu_dreg[dst], cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 4) {
        /* Dreg{dst} = Dreg{src0} ^ Dreg{src1}; */
        tcg_gen_xor_tl(cpu_dreg[dst], cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst]);
    } else if (opc == 5) {
        /* Preg{dst} = Preg{src0} + Preg{src1}; */
        /* If src0 == src1 this is disassembled as a shift by 1, but this
           distinction doesn't matter for our purposes */
        tcg_gen_add_tl(cpu_preg[dst], cpu_preg[src0], cpu_preg[src1]);
    } else { /*(opc == 6 || opc == 7)*/
        /* Preg{dst} = Preg{src0} + Preg{src1} << imm{opc}; */
        /* The dst/src0/src1 might all be the same register, so we need
           the temp here to avoid clobbering source values too early.
           This could be optimized a little, but for now we'll leave it. */
        tcg_gen_shli_tl(tmp, cpu_preg[src1], (opc - 5));
        tcg_gen_add_tl(cpu_preg[dst], cpu_preg[src0], tmp);
    }
    tcg_temp_free(tmp);
}

static void
decode_COMPI2opD_0(DisasContext *dc, uint16_t iw0)
{
    /* COMPI2opD
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 1 | 1 | 0 | 0 |.op|..src......................|.dst.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int op  = ((iw0 >> COMPI2opD_op_bits) & COMPI2opD_op_mask);
    int dst = ((iw0 >> COMPI2opD_dst_bits) & COMPI2opD_dst_mask);
    int src = ((iw0 >> COMPI2opD_src_bits) & COMPI2opD_src_mask);
    int imm = imm7(src);
    TCGv tmp;

    TRACE_EXTRACT("op:%i src:%i dst:%i", op, src, dst);

    if (op == 0) {
        /* Dreg{dst} = imm{src} (X); */
        tcg_gen_movi_tl(cpu_dreg[dst], imm);
    } else {
        /* Dreg{dst} += imm{src}; */
        tmp = tcg_const_tl(imm);
        tcg_gen_mov_tl(cpu_astat_arg[1], cpu_dreg[dst]);
        tcg_gen_add_tl(cpu_dreg[dst], cpu_astat_arg[1], tmp);
        astat_queue_state3(dc, ASTAT_OP_ADD32, cpu_dreg[dst], cpu_astat_arg[1],
                           tmp);
        tcg_temp_free(tmp);
    }
}

static void
decode_COMPI2opP_0(DisasContext *dc, uint16_t iw0)
{
    /* COMPI2opP
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 0 | 1 | 1 | 0 | 1 |.op|.src.......................|.dst.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int op  = ((iw0 >> COMPI2opP_op_bits) & COMPI2opP_op_mask);
    int src = ((iw0 >> COMPI2opP_src_bits) & COMPI2opP_src_mask);
    int dst = ((iw0 >> COMPI2opP_dst_bits) & COMPI2opP_dst_mask);
    int imm = imm7(src);

    TRACE_EXTRACT("op:%i src:%i dst:%i", op, src, dst);

    if (op == 0) {
        /* Preg{dst} = imm{src}; */
        tcg_gen_movi_tl(cpu_preg[dst], imm);
    } else {
        /* Preg{dst} += imm{src}; */
        tcg_gen_addi_tl(cpu_preg[dst], cpu_preg[dst], imm);
    }
}

static void
decode_LDSTpmod_0(DisasContext *dc, uint16_t iw0)
{
    /* LDSTpmod
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 0 | 0 | 0 |.W.|.aop...|.reg.......|.idx.......|.ptr.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int W   = ((iw0 >> LDSTpmod_W_bits) & LDSTpmod_W_mask);
    int aop = ((iw0 >> LDSTpmod_aop_bits) & LDSTpmod_aop_mask);
    int idx = ((iw0 >> LDSTpmod_idx_bits) & LDSTpmod_idx_mask);
    int ptr = ((iw0 >> LDSTpmod_ptr_bits) & LDSTpmod_ptr_mask);
    int reg = ((iw0 >> LDSTpmod_reg_bits) & LDSTpmod_reg_mask);
    TCGv tmp;

    TRACE_EXTRACT("W:%i aop:%i reg:%i idx:%i ptr:%i",
                  W, aop, reg, idx, ptr);

    if (aop == 1 && W == 0 && idx == ptr) {
        /* Dreg_lo{reg} = W[Preg{ptr}]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_preg[ptr]);
        tcg_gen_deposit_tl(cpu_dreg[reg], cpu_dreg[reg], tmp, 0, 16);
        tcg_temp_free(tmp);
    } else if (aop == 2 && W == 0 && idx == ptr) {
        /* Dreg_hi{reg} = W[Preg{ptr}]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_preg[ptr]);
        tcg_gen_deposit_tl(cpu_dreg[reg], cpu_dreg[reg], tmp, 16, 16);
        tcg_temp_free(tmp);
    } else if (aop == 1 && W == 1 && idx == ptr) {
        /* W[Preg{ptr}] = Dreg_lo{reg}; */
        gen_aligned_qemu_st16(dc, cpu_dreg[reg], cpu_preg[ptr]);
    } else if (aop == 2 && W == 1 && idx == ptr) {
        /* W[Preg{ptr}] = Dreg_hi{reg}; */
        tmp = tcg_temp_local_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        gen_aligned_qemu_st16(dc, tmp, cpu_preg[ptr]);
        tcg_temp_free(tmp);
    } else if (aop == 0 && W == 0) {
        /* Dreg{reg} = [Preg{ptr} ++ Preg{idx}]; */
        gen_aligned_qemu_ld32u(dc, cpu_dreg[reg], cpu_preg[ptr]);
        if (ptr != idx) {
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        }
    } else if (aop == 1 && W == 0) {
        /* Dreg_lo{reg} = W[Preg{ptr} ++ Preg{idx}]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_preg[ptr]);
        tcg_gen_deposit_tl(cpu_dreg[reg], cpu_dreg[reg], tmp, 0, 16);
        if (ptr != idx) {
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        }
        tcg_temp_free(tmp);
    } else if (aop == 2 && W == 0) {
        /* Dreg_hi{reg} = W[Preg{ptr} ++ Preg{idx}]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_preg[ptr]);
        tcg_gen_deposit_tl(cpu_dreg[reg], cpu_dreg[reg], tmp, 16, 16);
        if (ptr != idx) {
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        }
        tcg_temp_free(tmp);
    } else if (aop == 3 && W == 0) {
        /* R%i = W[Preg{ptr} ++ Preg{idx}] (Z); */
        gen_aligned_qemu_ld16u(dc, cpu_dreg[reg], cpu_preg[ptr]);
        if (ptr != idx) {
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        }
    } else if (aop == 3 && W == 1) {
        /* R%i = W[Preg{ptr} ++ Preg{idx}] (X); */
        gen_aligned_qemu_ld16s(dc, cpu_dreg[reg], cpu_preg[ptr]);
        if (ptr != idx) {
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        }
    } else if (aop == 0 && W == 1) {
        /* [Preg{ptr} ++ Preg{idx}] = R%i; */
        gen_aligned_qemu_st32(dc, cpu_dreg[reg], cpu_preg[ptr]);
        if (ptr != idx) {
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        }
    } else if (aop == 1 && W == 1) {
        /* W[Preg{ptr} ++ Preg{idx}] = Dreg_lo{reg}; */
        gen_aligned_qemu_st16(dc, cpu_dreg[reg], cpu_preg[ptr]);
        if (ptr != idx) {
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        }
    } else if (aop == 2 && W == 1) {
        /* W[Preg{ptr} ++ Preg{idx}] = Dreg_hi{reg}; */
        tmp = tcg_temp_local_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        gen_aligned_qemu_st16(dc, tmp, cpu_preg[ptr]);
        if (ptr != idx) {
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        }
        tcg_temp_free(tmp);
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_dagMODim_0(DisasContext *dc, uint16_t iw0)
{
    /* dagMODim
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 0 | 0 | 1 | 1 | 1 | 1 | 0 |.br| 1 | 1 |.op|.m.....|.i.....|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int i  = ((iw0 >> DagMODim_i_bits) & DagMODim_i_mask);
    int m  = ((iw0 >> DagMODim_m_bits) & DagMODim_m_mask);
    int br = ((iw0 >> DagMODim_br_bits) & DagMODim_br_mask);
    int op = ((iw0 >> DagMODim_op_bits) & DagMODim_op_mask);

    TRACE_EXTRACT("br:%i op:%i m:%i i:%i", br, op, m, i);

    if (op == 0 && br == 1) {
        /* Ireg{i} += Mreg{m} (BREV); */
        gen_helper_add_brev(cpu_ireg[i], cpu_ireg[i], cpu_mreg[m]);
    } else if (op == 0) {
        /* Ireg{i} += Mreg{m}; */
        gen_dagadd(dc, i, cpu_mreg[m]);
    } else if (op == 1 && br == 0) {
        /* Ireg{i} -= Mreg{m}; */
        gen_dagsub(dc, i, cpu_mreg[m]);
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_dagMODik_0(DisasContext *dc, uint16_t iw0)
{
    /* dagMODik
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 0 | 0 | 1 | 1 | 1 | 1 | 1 | 0 | 1 | 1 | 0 |.op....|.i.....|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int i  = ((iw0 >> DagMODik_i_bits) & DagMODik_i_mask);
    int op = ((iw0 >> DagMODik_op_bits) & DagMODik_op_mask);
    int mod = (op & 2) + 2;

    TRACE_EXTRACT("op:%i i:%i", op, i);

    if (op & 1) {
        /* Ireg{i} -= 2 or 4; */
        gen_dagsubi(dc, i, mod);
    } else {
        /* Ireg{i} += 2 or 4; */
        gen_dagaddi(dc, i, mod);
    }
}

static void
disalgnexcpt_ld32u(DisasContext *dc, TCGv ret, TCGv addr)
{
    if (dc->disalgnexcpt) {
        TCGv tmp = tcg_temp_new();
        tcg_gen_andi_tl(tmp, addr, ~0x3);
        tcg_gen_qemu_ld32u(ret, tmp, dc->mem_idx);
        tcg_temp_free(tmp);
    } else {
        gen_aligned_qemu_ld32u(dc, ret, addr);
    }
}

static void
decode_dspLDST_0(DisasContext *dc, uint16_t iw0)
{
    /* dspLDST
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 0 | 0 | 1 | 1 | 1 |.W.|.aop...|.m.....|.i.....|.reg.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int i   = ((iw0 >> DspLDST_i_bits) & DspLDST_i_mask);
    int m   = ((iw0 >> DspLDST_m_bits) & DspLDST_m_mask);
    int W   = ((iw0 >> DspLDST_W_bits) & DspLDST_W_mask);
    int aop = ((iw0 >> DspLDST_aop_bits) & DspLDST_aop_mask);
    int reg = ((iw0 >> DspLDST_reg_bits) & DspLDST_reg_mask);
    TCGv tmp;

    TRACE_EXTRACT("aop:%i m:%i i:%i reg:%i", aop, m, i, reg);

    if (aop == 0 && W == 0 && m == 0) {
        /* Dreg{reg} = [Ireg{i}++]; */
        disalgnexcpt_ld32u(dc, cpu_dreg[reg], cpu_ireg[i]);
        gen_dagaddi(dc, i, 4);
    } else if (aop == 0 && W == 0 && m == 1) {
        /* Dreg_lo{reg} = W[Ireg{i}++]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_ireg[i]);
        gen_mov_l_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
        gen_dagaddi(dc, i, 2);
    } else if (aop == 0 && W == 0 && m == 2) {
        /* Dreg_hi{reg} = W[Ireg{i}++]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_ireg[i]);
        gen_mov_h_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
        gen_dagaddi(dc, i, 2);
    } else if (aop == 1 && W == 0 && m == 0) {
        /* Dreg{reg} = [Ireg{i}--]; */
        disalgnexcpt_ld32u(dc, cpu_dreg[reg], cpu_ireg[i]);
        gen_dagsubi(dc, i, 4);
    } else if (aop == 1 && W == 0 && m == 1) {
        /* Dreg_lo{reg} = W[Ireg{i}--]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_ireg[i]);
        gen_mov_l_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
        gen_dagsubi(dc, i, 2);
    } else if (aop == 1 && W == 0 && m == 2) {
        /* Dreg_hi{reg} = W[Ireg{i}--]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_ireg[i]);
        gen_mov_h_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
        gen_dagsubi(dc, i, 2);
    } else if (aop == 2 && W == 0 && m == 0) {
        /* Dreg{reg} = [Ireg{i}]; */
        disalgnexcpt_ld32u(dc, cpu_dreg[reg], cpu_ireg[i]);
    } else if (aop == 2 && W == 0 && m == 1) {
        /* Dreg_lo{reg} = W[Ireg{i}]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_ireg[i]);
        gen_mov_l_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
    } else if (aop == 2 && W == 0 && m == 2) {
        /* Dreg_hi{reg} = W[Ireg{i}]; */
        tmp = tcg_temp_local_new();
        gen_aligned_qemu_ld16u(dc, tmp, cpu_ireg[i]);
        gen_mov_h_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
    } else if (aop == 0 && W == 1 && m == 0) {
        /* [Ireg{i}++] = Dreg{reg}; */
        gen_aligned_qemu_st32(dc, cpu_dreg[reg], cpu_ireg[i]);
        gen_dagaddi(dc, i, 4);
    } else if (aop == 0 && W == 1 && m == 1) {
        /* W[Ireg{i}++] = Dreg_lo{reg}; */
        gen_aligned_qemu_st16(dc, cpu_dreg[reg], cpu_ireg[i]);
        gen_dagaddi(dc, i, 2);
    } else if (aop == 0 && W == 1 && m == 2) {
        /* W[Ireg{i}++] = Dreg_hi{reg}; */
        tmp = tcg_temp_local_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        gen_aligned_qemu_st16(dc, tmp, cpu_ireg[i]);
        tcg_temp_free(tmp);
        gen_dagaddi(dc, i, 2);
    } else if (aop == 1 && W == 1 && m == 0) {
        /* [Ireg{i}--] = Dreg{reg}; */
        gen_aligned_qemu_st32(dc, cpu_dreg[reg], cpu_ireg[i]);
        gen_dagsubi(dc, i, 4);
    } else if (aop == 1 && W == 1 && m == 1) {
        /* W[Ireg{i}--] = Dreg_lo{reg}; */
        gen_aligned_qemu_st16(dc, cpu_dreg[reg], cpu_ireg[i]);
        gen_dagsubi(dc, i, 2);
    } else if (aop == 1 && W == 1 && m == 2) {
        /* W[Ireg{i}--] = Dreg_hi{reg}; */
        tmp = tcg_temp_local_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        gen_aligned_qemu_st16(dc, tmp, cpu_ireg[i]);
        tcg_temp_free(tmp);
        gen_dagsubi(dc, i, 2);
    } else if (aop == 2 && W == 1 && m == 0) {
        /* [Ireg{i}] = Dreg{reg}; */
        gen_aligned_qemu_st32(dc, cpu_dreg[reg], cpu_ireg[i]);
    } else if (aop == 2 && W == 1 && m == 1) {
        /* W[Ireg{i}] = Dreg_lo{reg}; */
        gen_aligned_qemu_st16(dc, cpu_dreg[reg], cpu_ireg[i]);
    } else if (aop == 2 && W == 1 && m == 2) {
        /* W[Ireg{i}] = Dreg_hi{reg}; */
        tmp = tcg_temp_local_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        gen_aligned_qemu_st16(dc, tmp, cpu_ireg[i]);
        tcg_temp_free(tmp);
    } else if (aop == 3 && W == 0) {
        /* Dreg{reg} = [Ireg{i} ++ Mreg{m}]; */
        disalgnexcpt_ld32u(dc, cpu_dreg[reg], cpu_ireg[i]);
        gen_dagadd(dc, i, cpu_mreg[m]);
    } else if (aop == 3 && W == 1) {
        /* [Ireg{i} ++ Mreg{m}] = Dreg{reg}; */
        gen_aligned_qemu_st32(dc, cpu_dreg[reg], cpu_ireg[i]);
        gen_dagadd(dc, i, cpu_mreg[m]);
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_LDST_0(DisasContext *dc, uint16_t iw0)
{
    /* LDST
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 0 | 0 | 1 |.sz....|.W.|.aop...|.Z.|.ptr.......|.reg.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int Z   = ((iw0 >> LDST_Z_bits) & LDST_Z_mask);
    int W   = ((iw0 >> LDST_W_bits) & LDST_W_mask);
    int sz  = ((iw0 >> LDST_sz_bits) & LDST_sz_mask);
    int aop = ((iw0 >> LDST_aop_bits) & LDST_aop_mask);
    int reg = ((iw0 >> LDST_reg_bits) & LDST_reg_mask);
    int ptr = ((iw0 >> LDST_ptr_bits) & LDST_ptr_mask);

    TRACE_EXTRACT("sz:%i W:%i aop:%i Z:%i ptr:%i reg:%i",
                  sz, W, aop, Z, ptr, reg);

    if (aop == 3) {
        illegal_instruction(dc);
        return;
    }

    if (W == 0) {
        if (sz == 0 && Z == 0) {
            /* Dreg{reg} = [Preg{ptr}{aop}]; */
            gen_aligned_qemu_ld32u(dc, cpu_dreg[reg], cpu_preg[ptr]);
        } else if (sz == 0 && Z == 1) {
            /* Preg{reg} = [Preg{ptr}{aop}]; */
            gen_aligned_qemu_ld32u(dc, cpu_preg[reg], cpu_preg[ptr]);
        } else if (sz == 1 && Z == 0) {
            /* Dreg{reg} = W[Preg{ptr}{aop}] (Z); */
            gen_aligned_qemu_ld16u(dc, cpu_dreg[reg], cpu_preg[ptr]);
        } else if (sz == 1 && Z == 1) {
            /* Dreg{reg} = W[Preg{ptr}{aop}] (X); */
            gen_aligned_qemu_ld16s(dc, cpu_dreg[reg], cpu_preg[ptr]);
        } else if (sz == 2 && Z == 0) {
            /* Dreg{reg} = B[Preg{ptr}{aop}] (Z); */
            tcg_gen_qemu_ld8u(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        } else if (sz == 2 && Z == 1) {
            /* Dreg{reg} = B[Preg{ptr}{aop}] (X); */
            tcg_gen_qemu_ld8s(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        } else {
            illegal_instruction(dc);
            return;
        }
    } else {
        if (sz == 0 && Z == 0) {
            /* [Preg{ptr}{aop}] = Dreg{reg}; */
            gen_aligned_qemu_st32(dc, cpu_dreg[reg], cpu_preg[ptr]);
        } else if (sz == 0 && Z == 1) {
            /* [Preg{ptr}{aop}] = Preg{reg}; */
            gen_aligned_qemu_st32(dc, cpu_preg[reg], cpu_preg[ptr]);
        } else if (sz == 1 && Z == 0) {
            /* W[Preg{ptr}{aop}] = Dreg{reg}; */
            gen_aligned_qemu_st16(dc, cpu_dreg[reg], cpu_preg[ptr]);
        } else if (sz == 2 && Z == 0) {
            /* B[Preg{ptr}{aop}] = Dreg{reg}; */
            tcg_gen_qemu_st8(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        } else {
            illegal_instruction(dc);
            return;
        }
    }

    if (aop == 0) {
        tcg_gen_addi_tl(cpu_preg[ptr], cpu_preg[ptr], 1 << (2 - sz));
    }
    if (aop == 1) {
        tcg_gen_subi_tl(cpu_preg[ptr], cpu_preg[ptr], 1 << (2 - sz));
    }
}

static void
decode_LDSTiiFP_0(DisasContext *dc, uint16_t iw0)
{
    /* LDSTiiFP
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 0 | 1 | 1 | 1 | 0 |.W.|.offset............|.reg...........|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    /* This isn't exactly a grp:reg as this insn only supports Dregs & Pregs,
       but for our usage, its functionality the same thing.  */
    int grp = ((iw0 >> 3) & 0x1);
    int reg = ((iw0 >> LDSTiiFP_reg_bits) & 0x7 /*LDSTiiFP_reg_mask*/);
    int offset = ((iw0 >> LDSTiiFP_offset_bits) & LDSTiiFP_offset_mask);
    int W = ((iw0 >> LDSTiiFP_W_bits) & LDSTiiFP_W_mask);
    uint32_t imm = negimm5s4(offset);
    TCGv treg = get_allreg(dc, grp, reg);
    TCGv ea;

    TRACE_EXTRACT("W:%i offset:%#x grp:%i reg:%i",
                  W, offset, grp, reg);

    ea = tcg_temp_local_new();
    tcg_gen_addi_tl(ea, cpu_fpreg, imm);
    gen_align_check(dc, ea, 4, false);
    if (W == 0) {
        /* DPreg{reg} = [FP + imm{offset}]; */
        tcg_gen_qemu_ld32u(treg, ea, dc->mem_idx);
    } else {
        /* [FP + imm{offset}] = DPreg{reg}; */
        tcg_gen_qemu_st32(treg, ea, dc->mem_idx);
    }
    tcg_temp_free(ea);
}

static void
decode_LDSTii_0(DisasContext *dc, uint16_t iw0)
{
    /* LDSTii
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 0 | 1 |.W.|.op....|.offset........|.ptr.......|.reg.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int reg = ((iw0 >> LDSTii_reg_bit) & LDSTii_reg_mask);
    int ptr = ((iw0 >> LDSTii_ptr_bit) & LDSTii_ptr_mask);
    int offset = ((iw0 >> LDSTii_offset_bit) & LDSTii_offset_mask);
    int op = ((iw0 >> LDSTii_op_bit) & LDSTii_op_mask);
    int W = ((iw0 >> LDSTii_W_bit) & LDSTii_W_mask);
    uint32_t imm;
    TCGv ea;

    TRACE_EXTRACT("W:%i op:%i offset:%#x ptr:%i reg:%i",
                  W, op, offset, ptr, reg);

    if (op == 0 || op == 3) {
        imm = uimm4s4(offset);
    } else {
        imm = uimm4s2(offset);
    }

    ea = tcg_temp_local_new();
    tcg_gen_addi_tl(ea, cpu_preg[ptr], imm);
    if (W == 0) {
        if (op == 0) {
            /* Dreg{reg} = [Preg{ptr} + imm{offset}]; */
            gen_aligned_qemu_ld32u(dc, cpu_dreg[reg], ea);
        } else if (op == 1) {
            /* Dreg{reg} = W[Preg{ptr} + imm{offset}] (Z); */
            gen_aligned_qemu_ld16u(dc, cpu_dreg[reg], ea);
        } else if (op == 2) {
            /* Dreg{reg} = W[Preg{ptr} + imm{offset}] (X); */
            gen_aligned_qemu_ld16s(dc, cpu_dreg[reg], ea);
        } else if (op == 3) {
            /* P%i = [Preg{ptr} + imm{offset}]; */
            gen_aligned_qemu_ld32u(dc, cpu_preg[reg], ea);
        }
    } else {
        if (op == 0) {
            /* [Preg{ptr} + imm{offset}] = Dreg{reg}; */
            gen_aligned_qemu_st32(dc, cpu_dreg[reg], ea);
        } else if (op == 1) {
            /* W[Preg{ptr} + imm{offset}] = Dreg{reg}; */
            gen_aligned_qemu_st16(dc, cpu_dreg[reg], ea);
        } else if (op == 3) {
            /* [Preg{ptr} + imm{offset}] = P%i; */
            gen_aligned_qemu_st32(dc, cpu_preg[reg], ea);
        } else {
            illegal_instruction(dc);
        }
    }
    tcg_temp_free(ea);
}

static void
decode_LoopSetup_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* LoopSetup
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 1 |.rop...|.c.|.soffset.......|
       |.reg...........| - | - |.eoffset...............................|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int c   = ((iw0 >> (LoopSetup_c_bits - 16)) & LoopSetup_c_mask);
    int reg = ((iw1 >> LoopSetup_reg_bits) & LoopSetup_reg_mask);
    int rop = ((iw0 >> (LoopSetup_rop_bits - 16)) & LoopSetup_rop_mask);
    int soffset = ((iw0 >> (LoopSetup_soffset_bits - 16)) &
                   LoopSetup_soffset_mask);
    int eoffset = ((iw1 >> LoopSetup_eoffset_bits) & LoopSetup_eoffset_mask);
    int spcrel = pcrel4(soffset);
    int epcrel = lppcrel10(eoffset);

    TRACE_EXTRACT("rop:%i c:%i soffset:%i reg:%i eoffset:%i",
                  rop, c, soffset, reg, eoffset);

    if (rop == 0) {
        /* LSETUP (imm{soffset}, imm{eoffset}) LCreg{c}; */;
    } else if (rop == 1 && reg <= 7) {
        /* LSETUP (imm{soffset}, imm{eoffset}) LCreg{c} = Preg{reg}; */
        tcg_gen_mov_tl(cpu_lcreg[c], cpu_preg[reg]);
    } else if (rop == 3 && reg <= 7) {
        /* LSETUP (imm{soffset}, imm{eoffset}) LCreg{c} = Preg{reg} >> 1; */
        tcg_gen_shri_tl(cpu_lcreg[c], cpu_preg[reg], 1);
    } else {
        illegal_instruction(dc);
        return;
    }

    tcg_gen_movi_tl(cpu_ltreg[c], dc->pc + spcrel);
    tcg_gen_movi_tl(cpu_lbreg[c], dc->pc + epcrel);
    gen_gotoi_tb(dc, 0, dc->pc + 4);
}

static void
decode_LDIMMhalf_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* LDIMMhalf
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 1 | 0 | 0 | 0 | 0 | 1 |.Z.|.H.|.S.|.grp...|.reg.......|
       |.hword.........................................................|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int H = ((iw0 >> (LDIMMhalf_H_bits - 16)) & LDIMMhalf_H_mask);
    int Z = ((iw0 >> (LDIMMhalf_Z_bits - 16)) & LDIMMhalf_Z_mask);
    int S = ((iw0 >> (LDIMMhalf_S_bits - 16)) & LDIMMhalf_S_mask);
    int reg = ((iw0 >> (LDIMMhalf_reg_bits - 16)) & LDIMMhalf_reg_mask);
    int grp = ((iw0 >> (LDIMMhalf_grp_bits - 16)) & LDIMMhalf_grp_mask);
    int hword = ((iw1 >> LDIMMhalf_hword_bits) & LDIMMhalf_hword_mask);
    uint32_t val;
    TCGv treg, tmp;

    TRACE_EXTRACT("Z:%i H:%i S:%i grp:%i reg:%i hword:%#x",
                  Z, H, S, grp, reg, hword);

    treg = get_allreg(dc, grp, reg);
    if (S == 1) {
        val = imm16(hword);
    } else {
        val = luimm16(hword);
    }

    if (H == 0 && S == 1 && Z == 0) {
        /* genreg{grp,reg} = imm{hword} (X); */
        /* Take care of immediate sign extension ourselves */
        tcg_gen_movi_tl(treg, (int16_t)val);
    } else if (H == 0 && S == 0 && Z == 1) {
        /* genreg{grp,reg} = imm{hword} (Z); */
        tcg_gen_movi_tl(treg, val);
    } else if (H == 0 && S == 0 && Z == 0) {
        /* genreg_lo{grp,reg} = imm{hword}; */
        tmp = tcg_temp_new();
        tcg_gen_movi_tl(tmp, val);
        tcg_gen_deposit_tl(treg, treg, tmp, 0, 16);
        tcg_temp_free(tmp);
    } else if (H == 1 && S == 0 && Z == 0) {
        /* genreg_hi{grp,reg} = imm{hword}; */
        tmp = tcg_temp_new();
        tcg_gen_movi_tl(tmp, val);
        tcg_gen_deposit_tl(treg, treg, tmp, 16, 16);
        tcg_temp_free(tmp);
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_CALLa_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* CALLa
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 1 | 0 | 0 | 0 | 1 |.S.|.msw...........................|
       |.lsw...........................................................|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int S   = ((iw0 >> (CALLa_S_bits - 16)) & CALLa_S_mask);
    int lsw = ((iw1 >> 0) & 0xffff);
    int msw = ((iw0 >> 0) & 0xff);
    int pcrel = pcrel24((msw << 16) | lsw);

    TRACE_EXTRACT("S:%i msw:%#x lsw:%#x", S, msw, lsw);

    if (S == 1) {
        /* CALL imm{pcrel}; */
        dc->is_jmp = DISAS_CALL;
    } else {
        /* JUMP.L imm{pcrel}; */
        dc->is_jmp = DISAS_JUMP;
    }
    dc->hwloop_callback = gen_hwloop_br_pcrel_imm;
    dc->hwloop_data = (void *)(unsigned long)pcrel;
}

static void
decode_LDSTidxI_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* LDSTidxI
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 1 | 0 | 0 | 1 |.W.|.Z.|.sz....|.ptr.......|.reg.......|
       |.offset........................................................|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int Z = ((iw0 >> (LDSTidxI_Z_bits - 16)) & LDSTidxI_Z_mask);
    int W = ((iw0 >> (LDSTidxI_W_bits - 16)) & LDSTidxI_W_mask);
    int sz = ((iw0 >> (LDSTidxI_sz_bits - 16)) & LDSTidxI_sz_mask);
    int reg = ((iw0 >> (LDSTidxI_reg_bits - 16)) & LDSTidxI_reg_mask);
    int ptr = ((iw0 >> (LDSTidxI_ptr_bits - 16)) & LDSTidxI_ptr_mask);
    int offset = ((iw1 >> LDSTidxI_offset_bits) & LDSTidxI_offset_mask);
    uint32_t imm_16s4 = imm16s4(offset);
    uint32_t imm_16s2 = imm16s2(offset);
    uint32_t imm_16 = imm16(offset);
    TCGv ea;

    TRACE_EXTRACT("W:%i Z:%i sz:%i ptr:%i reg:%i offset:%#x",
                  W, Z, sz, ptr, reg, offset);

    ea = tcg_temp_local_new();
    if (sz == 0) {
        tcg_gen_addi_tl(ea, cpu_preg[ptr], imm_16s4);
    } else if (sz == 1) {
        tcg_gen_addi_tl(ea, cpu_preg[ptr], imm_16s2);
    } else if (sz == 2) {
        tcg_gen_addi_tl(ea, cpu_preg[ptr], imm_16);
    } else {
        illegal_instruction(dc);
    }

    if (W == 0) {
        if (sz == 0 && Z == 0) {
            /* Dreg{reg} = [Preg{ptr] + imm{offset}]; */
            gen_aligned_qemu_ld32u(dc, cpu_dreg[reg], ea);
        } else if (sz == 0 && Z == 1) {
            /* Preg{reg} = [Preg{ptr] + imm{offset}]; */
            gen_aligned_qemu_ld32u(dc, cpu_preg[reg], ea);
        } else if (sz == 1 && Z == 0) {
            /* Dreg{reg} = W[Preg{ptr] + imm{offset}] (Z); */
            gen_aligned_qemu_ld16u(dc, cpu_dreg[reg], ea);
        } else if (sz == 1 && Z == 1) {
            /* Dreg{reg} = W[Preg{ptr} imm{offset}] (X); */
            gen_aligned_qemu_ld16s(dc, cpu_dreg[reg], ea);
        } else if (sz == 2 && Z == 0) {
            /* Dreg{reg} = B[Preg{ptr} + imm{offset}] (Z); */
            tcg_gen_qemu_ld8u(cpu_dreg[reg], ea, dc->mem_idx);
        } else if (sz == 2 && Z == 1) {
            /* Dreg{reg} = B[Preg{ptr} + imm{offset}] (X); */
            tcg_gen_qemu_ld8s(cpu_dreg[reg], ea, dc->mem_idx);
        }
    } else {
        if (sz == 0 && Z == 0) {
            /* [Preg{ptr} + imm{offset}] = Dreg{reg}; */
            gen_aligned_qemu_st32(dc, cpu_dreg[reg], ea);
        } else if (sz == 0 && Z == 1) {
            /* [Preg{ptr} + imm{offset}] = Preg{reg}; */
            gen_aligned_qemu_st32(dc, cpu_preg[reg], ea);
        } else if (sz == 1 && Z == 0) {
            /* W[Preg{ptr} + imm{offset}] = Dreg{reg}; */
            gen_aligned_qemu_st16(dc, cpu_dreg[reg], ea);
        } else if (sz == 2 && Z == 0) {
            /* B[Preg{ptr} + imm{offset}] = Dreg{reg}; */
            tcg_gen_qemu_st8(cpu_dreg[reg], ea, dc->mem_idx);
        } else {
            illegal_instruction(dc);
        }
    }

    tcg_temp_free(ea);
}

static void
decode_linkage_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* linkage
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 1 | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |.R.|
       |.framesize.....................................................|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int R = ((iw0 >> (Linkage_R_bits - 16)) & Linkage_R_mask);
    int framesize = ((iw1 >> Linkage_framesize_bits) & Linkage_framesize_mask);
    TCGv tmp_sp;

    TRACE_EXTRACT("R:%i framesize:%#x", R, framesize);

    /* XXX: Should do alignment checks of fp/sp */

    /* Delay the SP update till the end in case an exception occurs.  */
    tmp_sp = tcg_temp_new();
    tcg_gen_mov_tl(tmp_sp, cpu_spreg);
    if (R == 0) {
        /* LINK imm{framesize}; */
        int size = uimm16s4(framesize);
        tcg_gen_subi_tl(tmp_sp, tmp_sp, 4);
        tcg_gen_qemu_st32(cpu_rets, tmp_sp, dc->mem_idx);
        tcg_gen_subi_tl(tmp_sp, tmp_sp, 4);
        tcg_gen_qemu_st32(cpu_fpreg, tmp_sp, dc->mem_idx);
        tcg_gen_mov_tl(cpu_fpreg, tmp_sp);
        tcg_gen_subi_tl(cpu_spreg, tmp_sp, size);
    } else if (framesize == 0) {
        /* UNLINK; */
        /* Restore SP from FP.  */
        tcg_gen_mov_tl(tmp_sp, cpu_fpreg);
        tcg_gen_qemu_ld32u(cpu_fpreg, tmp_sp, dc->mem_idx);
        tcg_gen_addi_tl(tmp_sp, tmp_sp, 4);
        tcg_gen_qemu_ld32u(cpu_rets, tmp_sp, dc->mem_idx);
        tcg_gen_addi_tl(cpu_spreg, tmp_sp, 4);
    } else {
        illegal_instruction(dc);
    }
    tcg_temp_free(tmp_sp);
}

static void
decode_dsp32mac_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* XXX: Very incomplete.  */
    /* dsp32mac
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 0 | 0 |.M.| 0 | 0 |.mmod..........|.MM|.P.|.w1|.op1...|
       |.h01|.h11|.w0|.op0...|.h00|.h10|.dst.......|.src0......|.src1..|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int op1  = ((iw0 >> (DSP32Mac_op1_bits - 16)) & DSP32Mac_op1_mask);
    int w1   = ((iw0 >> (DSP32Mac_w1_bits - 16)) & DSP32Mac_w1_mask);
    int P    = ((iw0 >> (DSP32Mac_p_bits - 16)) & DSP32Mac_p_mask);
    int MM   = ((iw0 >> (DSP32Mac_MM_bits - 16)) & DSP32Mac_MM_mask);
    int mmod = ((iw0 >> (DSP32Mac_mmod_bits - 16)) & DSP32Mac_mmod_mask);
    int M    = ((iw0 >> (DSP32Mac_M_bits - 16)) & DSP32Mac_M_mask);
    int w0   = ((iw1 >> DSP32Mac_w0_bits) & DSP32Mac_w0_mask);
    int src0 = ((iw1 >> DSP32Mac_src0_bits) & DSP32Mac_src0_mask);
    int src1 = ((iw1 >> DSP32Mac_src1_bits) & DSP32Mac_src1_mask);
    int dst  = ((iw1 >> DSP32Mac_dst_bits) & DSP32Mac_dst_mask);
    int h10  = ((iw1 >> DSP32Mac_h10_bits) & DSP32Mac_h10_mask);
    int h00  = ((iw1 >> DSP32Mac_h00_bits) & DSP32Mac_h00_mask);
    int op0  = ((iw1 >> DSP32Mac_op0_bits) & DSP32Mac_op0_mask);
    int h11  = ((iw1 >> DSP32Mac_h11_bits) & DSP32Mac_h11_mask);
    int h01  = ((iw1 >> DSP32Mac_h01_bits) & DSP32Mac_h01_mask);

    int v_i = 0;
    TCGv res;

    TRACE_EXTRACT("M:%i mmod:%i MM:%i P:%i w1:%i op1:%i h01:%i h11:%i "
                  "w0:%i op0:%i h00:%i h10:%i dst:%i src0:%i src1:%i",
                  M, mmod, MM, P, w1, op1, h01, h11, w0, op0, h00, h10,
                  dst, src0, src1);

    res = tcg_temp_local_new();
    tcg_gen_mov_tl(res, cpu_dreg[dst]);
    if (w1 == 1 || op1 != 3) {
        TCGv res1 = decode_macfunc(dc, 1, op1, h01, h11, src0, src1, mmod, MM,
                                   P, &v_i);
        if (w1) {
            if (P) {
                tcg_gen_mov_tl(cpu_dreg[dst + 1], res1);
            } else {
                gen_mov_h_tl(res, res1);
            }
        }
        tcg_temp_free(res1);
    }
    if (w0 == 1 || op0 != 3) {
        TCGv res0 = decode_macfunc(dc, 0, op0, h00, h10, src0, src1, mmod, 0,
                                   P, &v_i);
        if (w0) {
            if (P) {
                tcg_gen_mov_tl(cpu_dreg[dst], res0);
            } else {
                gen_mov_l_tl(res, res0);
            }
        }
        tcg_temp_free(res0);
    }

    if (!P && (w0 || w1)) {
        tcg_gen_mov_tl(cpu_dreg[dst], res);
    }

    tcg_temp_free(res);
}

static void
decode_dsp32mult_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* XXX: Very incomplete.  */

    /* dsp32mult
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 0 | 0 |.M.| 0 | 1 |.mmod..........|.MM|.P.|.w1|.op1...|
       |.h01|.h11|.w0|.op0...|.h00|.h10|.dst.......|.src0......|.src1..|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int op1  = ((iw0 >> (DSP32Mac_op1_bits - 16)) & DSP32Mac_op1_mask);
    int w1   = ((iw0 >> (DSP32Mac_w1_bits - 16)) & DSP32Mac_w1_mask);
    int P    = ((iw0 >> (DSP32Mac_p_bits - 16)) & DSP32Mac_p_mask);
    int MM   = ((iw0 >> (DSP32Mac_MM_bits - 16)) & DSP32Mac_MM_mask);
    int mmod = ((iw0 >> (DSP32Mac_mmod_bits - 16)) & DSP32Mac_mmod_mask);
    int M    = ((iw0 >> (DSP32Mac_M_bits - 16)) & DSP32Mac_M_mask);
    int w0   = ((iw1 >> DSP32Mac_w0_bits) & DSP32Mac_w0_mask);
    int src0 = ((iw1 >> DSP32Mac_src0_bits) & DSP32Mac_src0_mask);
    int src1 = ((iw1 >> DSP32Mac_src1_bits) & DSP32Mac_src1_mask);
    int dst  = ((iw1 >> DSP32Mac_dst_bits) & DSP32Mac_dst_mask);
    int h10  = ((iw1 >> DSP32Mac_h10_bits) & DSP32Mac_h10_mask);
    int h00  = ((iw1 >> DSP32Mac_h00_bits) & DSP32Mac_h00_mask);
    int op0  = ((iw1 >> DSP32Mac_op0_bits) & DSP32Mac_op0_mask);
    int h11  = ((iw1 >> DSP32Mac_h11_bits) & DSP32Mac_h11_mask);
    int h01  = ((iw1 >> DSP32Mac_h01_bits) & DSP32Mac_h01_mask);

    TCGv res;
    TCGv sat0, sat1;

    TRACE_EXTRACT("M:%i mmod:%i MM:%i P:%i w1:%i op1:%i h01:%i h11:%i "
                  "w0:%i op0:%i h00:%i h10:%i dst:%i src0:%i src1:%i",
                  M, mmod, MM, P, w1, op1, h01, h11, w0, op0, h00, h10,
                  dst, src0, src1);

    if (w1 == 0 && w0 == 0) {
        illegal_instruction(dc);
        return;
    }
    if (((1 << mmod) & (P ? 0x313 : 0x1b57)) == 0) {
        illegal_instruction(dc);
        return;
    }
    if (P && ((dst & 1) || (op1 != 0) || (op0 != 0) ||
              !is_macmod_pmove(mmod))) {
        illegal_instruction(dc);
        return;
    }
    if (!P && ((op1 != 0) || (op0 != 0) || !is_macmod_hmove(mmod))) {
        illegal_instruction(dc);
        return;
    }

    res = tcg_temp_local_new();
    tcg_gen_mov_tl(res, cpu_dreg[dst]);

    sat1 = tcg_temp_local_new();

    if (w1) {
        TCGv res1 = decode_multfunc_tl(dc, h01, h11, src0, src1, mmod, MM,
                                       sat1);
        if (P) {
            tcg_gen_mov_tl(cpu_dreg[dst + 1], res1);
        } else {
            gen_mov_h_tl(res, res1);
        }
        tcg_temp_free(res1);
    }

    sat0 = tcg_temp_local_new();

    if (w0) {
        TCGv res0 = decode_multfunc_tl(dc, h00, h10, src0, src1, mmod, 0,
                                       sat0);
        if (P) {
            tcg_gen_mov_tl(cpu_dreg[dst], res0);
        } else {
            gen_mov_l_tl(res, res0);
        }
        tcg_temp_free(res0);
    }

    if (!P && (w0 || w1)) {
        tcg_gen_mov_tl(cpu_dreg[dst], res);
    }

    tcg_temp_free(sat0);
    tcg_temp_free(sat1);
    tcg_temp_free(res);
}

static void
decode_dsp32alu_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* dsp32alu
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 0 | 0 |.M.| 1 | 0 | - | - | - |.HL|.aopcde............|
       |.aop...|.s.|.x.|.dst0......|.dst1......|.src0......|.src1......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int s    = ((iw1 >> DSP32Alu_s_bits) & DSP32Alu_s_mask);
    int x    = ((iw1 >> DSP32Alu_x_bits) & DSP32Alu_x_mask);
    int aop  = ((iw1 >> DSP32Alu_aop_bits) & DSP32Alu_aop_mask);
    int src0 = ((iw1 >> DSP32Alu_src0_bits) & DSP32Alu_src0_mask);
    int src1 = ((iw1 >> DSP32Alu_src1_bits) & DSP32Alu_src1_mask);
    int dst0 = ((iw1 >> DSP32Alu_dst0_bits) & DSP32Alu_dst0_mask);
    int dst1 = ((iw1 >> DSP32Alu_dst1_bits) & DSP32Alu_dst1_mask);
    int M    = ((iw0 >> (DSP32Alu_M_bits - 16)) & DSP32Alu_M_mask);
    int HL   = ((iw0 >> (DSP32Alu_HL_bits - 16)) & DSP32Alu_HL_mask);
    int aopcde = ((iw0 >> (DSP32Alu_aopcde_bits - 16)) & DSP32Alu_aopcde_mask);
    TCGv tmp;
    TCGv_i64 tmp64;

    TRACE_EXTRACT("M:%i HL:%i aopcde:%i aop:%i s:%i x:%i dst0:%i "
                  "dst1:%i src0:%i src1:%i",
                  M, HL, aopcde, aop, s, x, dst0, dst1, src0, src1);

    if ((aop == 0 || aop == 2) && aopcde == 9 && HL == 0 && s == 0) {
        int a = aop >> 1;
        /* Areg_lo{a} = Dreg_lo{src0}; */
        tmp64 = tcg_temp_new_i64();
        tcg_gen_extu_i32_i64(tmp64, cpu_dreg[src0]);
        tcg_gen_deposit_i64(cpu_areg[a], cpu_areg[a], tmp64, 0, 16);
        tcg_temp_free_i64(tmp64);
    } else if ((aop == 0 || aop == 2) && aopcde == 9 && HL == 1 && s == 0) {
        int a = aop >> 1;
        /* Areg_hi{a} = Dreg_hi{src0}; */
        tcg_gen_andi_i64(cpu_areg[a], cpu_areg[a], 0xff0000ffff);
        tmp64 = tcg_temp_new_i64();
        tcg_gen_extu_i32_i64(tmp64, cpu_dreg[src0]);
        tcg_gen_andi_i64(tmp64, tmp64, 0xffff0000);
        tcg_gen_or_i64(cpu_areg[a], cpu_areg[a], tmp64);
        tcg_temp_free_i64(tmp64);
    } else if ((aop == 1 || aop == 0) && aopcde == 5) {
        /* Dreg{dst0}_hi{HL==0} = Dreg{src0} +{aop==0} Dreg{src1} (RND12); */
        /* Dreg{dst0}_lo{HL==1} = Dreg{src0} +{aop==0} Dreg{src1} (RND12); */
        /* Dreg{dst0}_hi{HL==0} = Dreg{src0} -{aop==1} Dreg{src1} (RND12); */
        /* Dreg{dst0}_lo{HL==1} = Dreg{src0} -{aop==1} Dreg{src1} (RND12); */
        unhandled_instruction(dc, "Dreg +/- RND12");
    } else if ((aop == 2 || aop == 3) && aopcde == 5) {
        /* Dreg{dst0}_hi{HL==0} = Dreg{src0} +{aop==0} Dreg{src1} (RND20); */
        /* Dreg{dst0}_lo{HL==1} = Dreg{src0} +{aop==0} Dreg{src1} (RND20); */
        /* Dreg{dst0}_hi{HL==0} = Dreg{src0} -{aop==1} Dreg{src1} (RND20); */
        /* Dreg{dst0}_lo{HL==1} = Dreg{src0} -{aop==1} Dreg{src1} (RND20); */
        unhandled_instruction(dc, "Dreg +/- RND20");
    } else if (aopcde == 2 || aopcde == 3) {
        /* Dreg{dst0}_lo{HL==0} = Dreg{src0}_lo{!aop&2} +{aopcde==2}
                                  Dreg{src1}_lo{!aop&1} (amod1(s,x)); */
        /* Dreg{dst0}_hi{HL==1} = Dreg{src0}_hi{aop&2} -{aopcde==3}
                                  Dreg{src1}_hi{aop&1} (amod1(s,x)); */
        TCGv s1, s2, d;

        s1 = tcg_temp_new();
        if (aop & 2) {
            tcg_gen_shri_tl(s1, cpu_dreg[src0], 16);
        } else {
            tcg_gen_ext16u_tl(s1, cpu_dreg[src0]);
        }

        s2 = tcg_temp_new();
        if (aop & 1) {
            tcg_gen_shri_tl(s2, cpu_dreg[src1], 16);
        } else {
            tcg_gen_ext16u_tl(s2, cpu_dreg[src1]);
        }

        d = tcg_temp_new();
        if (aopcde == 2) {
            tcg_gen_add_tl(d, s1, s2);
        } else {
            tcg_gen_sub_tl(d, s1, s2);
        }
        tcg_gen_andi_tl(d, d, 0xffff);

        tcg_temp_free(s1);
        tcg_temp_free(s2);

        if (HL) {
            tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0], 0xffff);
            tcg_gen_shli_tl(d, d, 16);
            tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], d);
        } else {
            tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0], 0xffff0000);
            tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], d);
        }
        tcg_temp_free(d);
    } else if ((aop == 0 || aop == 2) && aopcde == 9 && s == 1) {
        int a = aop >> 1;
        /* Areg{a} = Dreg{src0}; */
        tcg_gen_ext_i32_i64(cpu_areg[a], cpu_dreg[src0]);
    } else if ((aop == 1 || aop == 3) && aopcde == 9 && s == 0) {
        int a = aop >> 1;
        /* Areg_x{a} = Dreg_lo{src0}; */
        tmp64 = tcg_temp_new_i64();
        tcg_gen_extu_i32_i64(tmp64, cpu_dreg[src0]);
        tcg_gen_deposit_i64(cpu_areg[a], cpu_areg[a], tmp64, 32, 8);
        tcg_temp_free_i64(tmp64);
    } else if (aop == 3 && aopcde == 11 && (s == 0 || s == 1)) {
        /* A0 -= A0 (W32){s==1}; */
        tcg_gen_sub_i64(cpu_areg[0], cpu_areg[0], cpu_areg[1]);

        if (s == 1) {
            unhandled_instruction(dc, "A0 -= A1 (W32)");
        }
    } else if ((aop == 0 || aop == 1) && aopcde == 22) {
        /* Dreg{dst0} = BYTEOP2P (Dreg{src0+1}:Dreg{src0},
                                  Dreg{src1+1}:Dreg{src1} (mode); */
        /* modes[HL + (aop << 1)] = { rndl, rndh, tl, th }; */
        /* (modes, r) s==1 */
        unhandled_instruction(dc, "BYTEOP2P");
    } else if ((aop == 0 || aop == 1) && s == 0 && aopcde == 8) {
        /* Areg{aop} = 0; */
        tcg_gen_movi_i64(cpu_areg[0], 0);
    } else if (aop == 2 && s == 0 && aopcde == 8) {
        /* A1 = A0 = 0; */
        tcg_gen_movi_i64(cpu_areg[0], 0);
        tcg_gen_mov_i64(cpu_areg[1], cpu_areg[0]);
    } else if ((aop == 0 || aop == 1 || aop == 2) && s == 1 && aopcde == 8) {
        /* A0 = A0 (S); {aop==0} */
        /* A1 = A1 (S); {aop==1} */
        /* A1 = A1 (S), A0 = A0 (S); {aop==2} */
        TCGv sat0, sat1;

        sat0 = tcg_temp_local_new();
        tcg_gen_movi_tl(sat0, 0);
        if (aop == 0 || aop == 2) {
            gen_extend_acc(cpu_areg[0]);
            saturate_s32(cpu_areg[0], sat0);
            tcg_gen_ext32s_i64(cpu_areg[0], cpu_areg[0]);
        }

        sat1 = tcg_temp_local_new();
        tcg_gen_movi_tl(sat1, 0);
        if (aop == 1 || aop == 2) {
            gen_extend_acc(cpu_areg[1]);
            saturate_s32(cpu_areg[1], sat1);
            tcg_gen_ext32s_i64(cpu_areg[0], cpu_areg[0]);
        }

        tcg_temp_free(sat1);
        tcg_temp_free(sat0);

        /* XXX: missing ASTAT update */
    } else if (aop == 3 && (s == 0 || s == 1) && aopcde == 8) {
        /* Areg{s} = Areg{!s}; */
        tcg_gen_mov_i64(cpu_areg[s], cpu_areg[!s]);
    } else if (aop == 3 && HL == 0 && aopcde == 16) {
        /* A1 = ABS A1 , A0 = ABS A0; */
        int i;
        /* XXX: Missing ASTAT updates and saturation */
        for (i = 0; i < 2; ++i) {
            gen_abs_i64(cpu_areg[i], cpu_areg[i]);
        }
    } else if (aop == 0 && aopcde == 23) {
        unhandled_instruction(dc, "BYTEOP3P");
    } else if ((aop == 0 || aop == 1) && aopcde == 16) {
        /* Areg{HL} = ABS Areg{aop}; */

        /* XXX: Missing ASTAT updates */
        /* XXX: Missing saturation */
        gen_abs_i64(cpu_areg[aop], cpu_areg[aop]);
    } else if (aop == 3 && aopcde == 12) {
        /* Dreg{dst0}_lo{HL==0} = Dreg{src0} (RND); */
        /* Dreg{dst0}_hi{HL==1} = Dreg{src0} (RND); */
        unhandled_instruction(dc, "Dreg (RND)");
    } else if (aop == 3 && HL == 0 && aopcde == 15) {
        /* Dreg{dst0} = -Dreg{src0} (V); */
        unhandled_instruction(dc, "Dreg = -Dreg (V)");
    } else if (aop == 3 && HL == 0 && aopcde == 14) {
        /* A1 = -A1 , A0 = -A0; */
        tcg_gen_neg_i64(cpu_areg[1], cpu_areg[1]);
        tcg_gen_neg_i64(cpu_areg[0], cpu_areg[0]);
        /* XXX: what ASTAT flags need updating ?  */
    } else if ((aop == 0 || aop == 1) && (HL == 0 || HL == 1) &&
               aopcde == 14) {
        /* Areg{HL} = -Areg{aop}; */
        tcg_gen_neg_i64(cpu_areg[HL], cpu_areg[aop]);
        /* XXX: Missing ASTAT updates */
    } else if (aop == 0 && aopcde == 12) {
        /* Dreg_lo{dst0} = Dreg_hi{dst0} =
                 SIGN(Dreg_hi{src0} * Dreg_hi{src1} +
                 SIGN(Dreg_lo{src0} * Dreg_lo{src1} */
        TCGLabel *l;
        TCGv tmp1_hi, tmp1_lo;

        tmp1_hi = tcg_temp_local_new();
        /* if ((src0_hi >> 15) & 1) tmp1_hi = -src1_hi; */
        tcg_gen_sari_tl(tmp1_hi, cpu_dreg[src1], 16);
        l = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_GE, cpu_dreg[src0], 0, l);
        tcg_gen_neg_tl(tmp1_hi, tmp1_hi);
        gen_set_label(l);

        tmp = tcg_temp_local_new();
        tmp1_lo = tcg_temp_local_new();
        /* if ((src0_lo >> 15) & 1) tmp1_lo = -src1_lo; */
        tcg_gen_ext16s_tl(tmp, cpu_dreg[src0]);
        tcg_gen_ext16s_tl(tmp1_lo, cpu_dreg[src1]);
        l = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_GE, tmp, 0, l);
        tcg_gen_neg_tl(tmp1_lo, tmp1_lo);
        gen_set_label(l);

        tcg_temp_free(tmp);

        tcg_gen_add_tl(tmp1_hi, tmp1_hi, tmp1_lo);
        tcg_gen_shli_tl(cpu_dreg[dst0], tmp1_hi, 16);
        gen_mov_l_tl(cpu_dreg[dst0], tmp1_hi);

        tcg_temp_free(tmp1_lo);
        tcg_temp_free(tmp1_hi);
    } else if (aopcde == 0) {
        /* Dreg{dst0} = Dreg{src0} -{aop&2}+{!aop&2}|-{aop&1}+{!aop&1}
                        Dreg{src1} (amod0); */
        TCGv s0, s1, t0, t1;

        if (s || x) {
            unhandled_instruction(dc, "S/CO/SCO with +|+/-|-");
        }

        s0 = tcg_temp_local_new();
        s1 = tcg_temp_local_new();

        t0 = tcg_temp_local_new();
        tcg_gen_shri_tl(s0, cpu_dreg[src0], 16);
        tcg_gen_shri_tl(s1, cpu_dreg[src1], 16);
        if (aop & 2) {
            tcg_gen_sub_tl(t0, s0, s1);
        } else {
            tcg_gen_add_tl(t0, s0, s1);
        }

        t1 = tcg_temp_local_new();
        tcg_gen_andi_tl(s0, cpu_dreg[src0], 0xffff);
        tcg_gen_andi_tl(s1, cpu_dreg[src1], 0xffff);
        if (aop & 1) {
            tcg_gen_sub_tl(t1, s0, s1);
        } else {
            tcg_gen_add_tl(t1, s0, s1);
        }

        tcg_temp_free(s1);
        tcg_temp_free(s0);

        astat_queue_state2(dc, ASTAT_OP_VECTOR_ADD_ADD + aop, t0, t1);

        if (x) {
            /* dst0.h = t1; dst0.l = t0 */
            tcg_gen_ext16u_tl(cpu_dreg[dst0], t0);
            tcg_gen_shli_tl(t1, t1, 16);
            tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], t1);
        } else {
            /* dst0.h = t0; dst0.l = t1 */
            tcg_gen_ext16u_tl(cpu_dreg[dst0], t1);
            tcg_gen_shli_tl(t0, t0, 16);
            tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], t0);
        }

        tcg_temp_free(t0);
        tcg_temp_free(t1);
        /* XXX: missing ASTAT update */
    } else if (aop == 1 && aopcde == 12) {
        /* Dreg{dst1} = A1.L + A1.H, Dreg{dst0} = A0.L + A0.H; */
        TCGv al, ah;

        al = tcg_temp_local_new();
        ah = tcg_temp_local_new();
        tcg_gen_extrl_i64_i32(ah, cpu_areg[0]);
        tcg_gen_ext16u_tl(al, ah);
        tcg_gen_shri_tl(ah, ah, 16);
        tcg_gen_add_tl(cpu_dreg[dst0], al, ah);
        tcg_temp_free(al);
        tcg_temp_free(ah);
        tcg_gen_ext16s_tl(cpu_dreg[dst0], cpu_dreg[dst0]);

        al = tcg_temp_local_new();
        ah = tcg_temp_local_new();
        tcg_gen_extrl_i64_i32(ah, cpu_areg[1]);
        tcg_gen_ext16u_tl(al, ah);
        tcg_gen_shri_tl(ah, ah, 16);
        tcg_gen_add_tl(cpu_dreg[dst1], al, ah);
        tcg_temp_free(al);
        tcg_temp_free(ah);
        tcg_gen_ext16s_tl(cpu_dreg[dst1], cpu_dreg[dst1]);

        /* XXX: ASTAT ?  */
    } else if (aopcde == 1) {
        /* XXX: missing ASTAT update */
        unhandled_instruction(dc, "Dreg +|+ Dreg, Dreg -|- Dreg");
    } else if ((aop == 0 || aop == 1 || aop == 2) && aopcde == 11) {
        /* Dreg{dst0} = (A0 += A1); {aop==0} */
        /* Dreg{dst0}_lo{HL==0} = (A0 += A1); {aop==1} */
        /* Dreg{dst0}_hi{HL==1} = (A0 += A1); {aop==1} */
        /* (A0 += A1); {aop==2} */

        tcg_gen_add_i64(cpu_areg[0], cpu_areg[0], cpu_areg[1]);

        if (aop == 2 && s == 1) {   /* A0 += A1 (W32) */
            unhandled_instruction(dc, "A0 += A1 (W32)");
        }

        /* XXX: missing saturation support */
        if (aop == 0) {
            /* Dregs = A0 += A1 */
            tcg_gen_extrl_i64_i32(cpu_dreg[dst0], cpu_areg[0]);
        } else if (aop == 1) {
            /* Dregs_lo = A0 += A1 */
            tmp = tcg_temp_new();
            tcg_gen_extrl_i64_i32(tmp, cpu_areg[0]);
            gen_mov_l_tl(cpu_dreg[dst0], tmp);
            tcg_temp_free(tmp);
        }
    } else if ((aop == 0 || aop == 1) && aopcde == 10) {
        /* Dreg_lo{dst0} = Areg_x{aop}; */
        tmp = tcg_temp_new();
        tmp64 = tcg_temp_new_i64();
        tcg_gen_shri_i64(tmp64, cpu_areg[aop], 32);
        tcg_gen_extrl_i64_i32(tmp, tmp64);
        tcg_temp_free_i64(tmp64);
        tcg_gen_ext8s_tl(tmp, tmp);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if (aop == 0 && aopcde == 4) {
        /* Dreg{dst0} = Dreg{src0} + Dreg{src1} (amod1(s,x)); */
        tcg_gen_add_tl(cpu_dreg[dst0], cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state3(dc, ASTAT_OP_ADD32, cpu_dreg[dst0], cpu_dreg[src0],
                           cpu_dreg[src1]);
    } else if (aop == 1 && aopcde == 4) {
        /* Dreg{dst0} = Dreg{src0} - Dreg{src1} (amod1(s,x)); */
        tcg_gen_sub_tl(cpu_dreg[dst0], cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state3(dc, ASTAT_OP_SUB32, cpu_dreg[dst0], cpu_dreg[src0],
                           cpu_dreg[src1]);
    } else if (aop == 2 && aopcde == 4) {
        /* Dreg{dst1} = Dreg{src0} + Dreg{src1},
           Dreg{dst0} = Dreg{src0} - Dreg{src1} (amod1(s,x)); */
        if (dst1 == src0 || dst1 == src1) {
            tmp = tcg_temp_new();
        } else {
            tmp = cpu_dreg[dst1];
        }
        tcg_gen_add_tl(tmp, cpu_dreg[src0], cpu_dreg[src1]);
        tcg_gen_sub_tl(cpu_dreg[dst0], cpu_dreg[src0], cpu_dreg[src1]);
        if (dst1 == src0 || dst1 == src1) {
            tcg_gen_mov_tl(cpu_dreg[dst1], tmp);
            tcg_temp_free(tmp);
        }
        /* XXX: Missing ASTAT updates */
    } else if ((aop == 0 || aop == 1) && aopcde == 17) {
        unhandled_instruction(dc, "Dreg = Areg + Areg, Dreg = Areg - Areg");
    } else if (aop == 0 && aopcde == 18) {
        unhandled_instruction(dc, "SAA");
    } else if (aop == 3 && aopcde == 18) {
        dc->disalgnexcpt = true;
    } else if ((aop == 0 || aop == 1) && aopcde == 20) {
        unhandled_instruction(dc, "BYTEOP1P");
    } else if (aop == 0 && aopcde == 21) {
        unhandled_instruction(dc, "BYTEOP16P");
    } else if (aop == 1 && aopcde == 21) {
        unhandled_instruction(dc, "BYTEOP16M");
    } else if ((aop == 0 || aop == 1) && aopcde == 7) {
        /* Dreg{dst0} = MIN{aop==1} (Dreg{src0}, Dreg{src1}); */
        /* Dreg{dst0} = MAX{aop==0} (Dreg{src0}, Dreg{src1}); */
        int _src0, _src1;
        TCGCond cond;

        if (aop == 0) {
            cond = TCG_COND_LT;
        } else {
            cond = TCG_COND_GE;
        }

        /* src/dst regs might be the same, so we need to handle that */
        if (dst0 == src1) {
            _src0 = src1, _src1 = src0;
        } else {
            _src0 = src0, _src1 = src1;
        }

        tcg_gen_movcond_tl(cond, cpu_dreg[dst0], cpu_dreg[_src1],
                           cpu_dreg[_src0], cpu_dreg[_src0], cpu_dreg[_src1]);

        astat_queue_state1(dc, ASTAT_OP_MIN_MAX, cpu_dreg[dst0]);
    } else if (aop == 2 && aopcde == 7) {
        /* Dreg{dst0} = ABS Dreg{src0}; */

        /* XXX: Missing saturation support (and ASTAT V/VS) */
        gen_abs_tl(cpu_dreg[dst0], cpu_dreg[src0]);

        astat_queue_state2(dc, ASTAT_OP_ABS, cpu_dreg[dst0], cpu_dreg[src0]);
    } else if (aop == 3 && aopcde == 7) {
        /* Dreg{dst0} = -Dreg{src0} (amod1(s,0)); */
        TCGLabel *l, *endl;

        l = gen_new_label();
        endl = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_NE, cpu_dreg[src0], 0x80000000, l);
        if (s) {
            tcg_gen_movi_tl(cpu_dreg[dst0], 0x7fffffff);
            tmp = tcg_const_tl(1);
            _gen_astat_store(ASTAT_V, tmp);
            _gen_astat_store(ASTAT_V_COPY, tmp);
            _gen_astat_store(ASTAT_VS, tmp);
            tcg_temp_free(tmp);
        } else {
            tcg_gen_movi_tl(cpu_dreg[dst0], 0x80000000);
        }

        gen_set_label(l);
        tcg_gen_neg_tl(cpu_dreg[dst0], cpu_dreg[src0]);
        gen_set_label(endl);
        astat_queue_state2(dc, ASTAT_OP_NEGATE, cpu_dreg[dst0], cpu_dreg[src0]);
    } else if (aop == 2 && aopcde == 6) {
        /* Dreg{dst0} = ABS Dreg{src0} (V); */
        TCGv tmp0;

        tmp = tcg_temp_local_new();
        tcg_gen_sari_tl(tmp, cpu_dreg[src0], 16);
        gen_abs_tl(tmp, tmp);

        tmp0 = tcg_temp_local_new();
        tcg_gen_ext16s_tl(tmp0, cpu_dreg[src0]);
        gen_abs_tl(tmp0, tmp0);

        astat_queue_state2(dc, ASTAT_OP_ABS_VECTOR, tmp0, tmp);

        tcg_gen_deposit_tl(cpu_dreg[dst0], tmp0, tmp, 16, 16);

        tcg_temp_free(tmp0);
        tcg_temp_free(tmp);
    } else if ((aop == 0 || aop == 1) && aopcde == 6) {
        /* Dreg{dst0} = MAX{aop==0} (Dreg{src0}, Dreg{src1}) (V); */
        /* Dreg{dst0} = MIN{aop==1} (Dreg{src0}, Dreg{src1}) (V); */
        /* src/dst regs might be the same, so we need to handle that */
        TCGCond cond;
        TCGv tmp0, tmp1;

        cond = aop == 1 ? TCG_COND_LE : TCG_COND_GE;

        tmp = tcg_temp_local_new();
        tmp0 = tcg_temp_local_new();
        tmp1 = tcg_temp_local_new();

        /* First do top 16bit pair */
        tcg_gen_andi_tl(tmp0, cpu_dreg[src0], 0xffff0000);
        tcg_gen_andi_tl(tmp1, cpu_dreg[src1], 0xffff0000);
        tcg_gen_movcond_tl(cond, tmp0, tmp0, tmp1, tmp0, tmp1);

        /* Then bottom 16bit pair */
        tcg_gen_ext16s_tl(tmp, cpu_dreg[src0]);
        tcg_gen_ext16s_tl(tmp1, cpu_dreg[src1]);
        tcg_gen_movcond_tl(cond, tmp, tmp, tmp1, tmp, tmp1);

        astat_queue_state2(dc, ASTAT_OP_MIN_MAX_VECTOR, tmp0, tmp);

        /* Then combine them */
        tcg_gen_andi_tl(tmp, tmp, 0xffff);
        tcg_gen_or_tl(cpu_dreg[dst0], tmp0, tmp);

        tcg_temp_free(tmp1);
        tcg_temp_free(tmp0);
        tcg_temp_free(tmp);
    } else if (aop == 0 && aopcde == 24) {
        TCGv dst;
        /* Dreg{dst0} BYTEPACK (Dreg{src0}, Dreg{src1}); */

        /* XXX: could optimize a little if dst0 is diff from src0 or src1 */
        /* dst |= (((src0 >>  0) & 0xff) <<  0) */
        dst = tcg_temp_new();
        tcg_gen_andi_tl(dst, cpu_dreg[src0], 0xff);
        tmp = tcg_temp_new();
        /* dst |= (((src0 >> 16) & 0xff) <<  8) */
        tcg_gen_andi_tl(tmp, cpu_dreg[src0], 0xff0000);
        tcg_gen_shri_tl(tmp, tmp, 8);
        tcg_gen_or_tl(dst, dst, tmp);
        /* dst |= (((src1 >>  0) & 0xff) << 16) */
        tcg_gen_andi_tl(tmp, cpu_dreg[src1], 0xff);
        tcg_gen_shli_tl(tmp, tmp, 16);
        tcg_gen_or_tl(dst, dst, tmp);
        /* dst |= (((src1 >> 16) & 0xff) << 24) */
        tcg_gen_andi_tl(tmp, cpu_dreg[src1], 0xff0000);
        tcg_gen_shli_tl(tmp, tmp, 8);
        tcg_gen_or_tl(cpu_dreg[dst0], dst, tmp);
        tcg_temp_free(tmp);
        tcg_temp_free(dst);
    } else if (aop == 1 && aopcde == 24) {
        /* (Dreg{dst1}, Dreg{dst0} = BYTEUNPACK Dreg{src0+1}:{src0} (R){s}; */
        TCGv lo, hi;
        TCGv_i64 tmp64_2;

        if (s) {
            hi = cpu_dreg[src0], lo = cpu_dreg[src0 + 1];
        } else {
            hi = cpu_dreg[src0 + 1], lo = cpu_dreg[src0];
        }

        /* Create one field of the two regs */
        tmp64 = tcg_temp_local_new_i64();
        tcg_gen_extu_i32_i64(tmp64, hi);
        tcg_gen_shli_i64(tmp64, tmp64, 32);
        tmp64_2 = tcg_temp_local_new_i64();
        tcg_gen_extu_i32_i64(tmp64_2, lo);
        tcg_gen_or_i64(tmp64, tmp64, tmp64_2);

        /* Adjust the two regs field by the Ireg[0] order */
        tcg_gen_extu_i32_i64(tmp64_2, cpu_ireg[0]);
        tcg_gen_andi_i64(tmp64_2, tmp64_2, 0x3);
        tcg_gen_shli_i64(tmp64_2, tmp64_2, 3);    /* multiply by 8 */
        tcg_gen_shr_i64(tmp64, tmp64, tmp64_2);
        tcg_temp_free_i64(tmp64_2);

        /* Now that the 4 bytes we want are in the low 32bit, truncate */
        tmp = tcg_temp_local_new();
        tcg_gen_extrl_i64_i32(tmp, tmp64);
        tcg_temp_free_i64(tmp64);

        /* Load bytea into dst0 */
        tcg_gen_andi_tl(cpu_dreg[dst0], tmp, 0xff);
        /* Load byted into dst1 */
        tcg_gen_shri_tl(cpu_dreg[dst1], tmp, 8);
        tcg_gen_andi_tl(cpu_dreg[dst1], cpu_dreg[dst1], 0xff0000);
        /* Load byteb into dst0 */
        tcg_gen_shli_tl(tmp, tmp, 8);
        tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], tmp);
        tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0], 0xff00ff);
        /* Load bytec into dst1 */
        tcg_gen_shri_tl(tmp, tmp, 24);
        tcg_gen_or_tl(cpu_dreg[dst1], cpu_dreg[dst1], tmp);
        tcg_gen_andi_tl(cpu_dreg[dst1], cpu_dreg[dst1], 0xff00ff);
        tcg_temp_free(tmp);
    } else if (aopcde == 13) {
        TCGLabel *l;
        TCGv a_lo;
        TCGCond conds[] = {
            /* GT */ TCG_COND_LE,
            /* GE */ TCG_COND_LT,
            /* LT */ TCG_COND_GE,
            /* LE */ TCG_COND_GT,
        };

        /* (Dreg{dst1}, Dreg{dst0}) = SEARCH Dreg{src0} (mode{aop}); */

        a_lo = tcg_temp_local_new();
        tmp = tcg_temp_local_new();

        /* Compare A1 to Dreg_hi{src0} */
        tcg_gen_extrl_i64_i32(a_lo, cpu_areg[1]);
        tcg_gen_ext16s_tl(a_lo, a_lo);
        tcg_gen_sari_tl(tmp, cpu_dreg[src0], 16);

        l = gen_new_label();
        tcg_gen_brcond_tl(conds[aop], tmp, a_lo, l);
        /* Move Dreg_hi{src0} into A0 */
        tcg_gen_ext_i32_i64(cpu_areg[1], tmp);
        /* Move Preg{0} into Dreg{dst1} */
        tcg_gen_mov_tl(cpu_dreg[dst1], cpu_preg[0]);
        gen_set_label(l);

        /* Compare A0 to Dreg_lo{src0} */
        tcg_gen_extrl_i64_i32(a_lo, cpu_areg[0]);
        tcg_gen_ext16s_tl(a_lo, a_lo);
        tcg_gen_ext16s_tl(tmp, cpu_dreg[src0]);

        l = gen_new_label();
        tcg_gen_brcond_tl(conds[aop], tmp, a_lo, l);
        /* Move Dreg_lo{src0} into A0 */
        tcg_gen_ext_i32_i64(cpu_areg[0], tmp);
        /* Move Preg{0} into Dreg{dst0} */
        tcg_gen_mov_tl(cpu_dreg[dst0], cpu_preg[0]);
        gen_set_label(l);

        tcg_temp_free(a_lo);
        tcg_temp_free(tmp);
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_dsp32shift_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* dsp32shift
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 0 | 0 |.M.| 1 | 1 | 0 | 0 | - | - |.sopcde............|
       |.sop...|.HLs...|.dst0......| - | - | - |.src0......|.src1......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int HLs  = ((iw1 >> DSP32Shift_HLs_bits) & DSP32Shift_HLs_mask);
    int sop  = ((iw1 >> DSP32Shift_sop_bits) & DSP32Shift_sop_mask);
    int src0 = ((iw1 >> DSP32Shift_src0_bits) & DSP32Shift_src0_mask);
    int src1 = ((iw1 >> DSP32Shift_src1_bits) & DSP32Shift_src1_mask);
    int dst0 = ((iw1 >> DSP32Shift_dst0_bits) & DSP32Shift_dst0_mask);
    int sopcde = ((iw0 >> (DSP32Shift_sopcde_bits - 16)) &
                  DSP32Shift_sopcde_mask);
    int M = ((iw0 >> (DSP32Shift_M_bits - 16)) & DSP32Shift_M_mask);
    TCGv tmp;
    TCGv_i64 tmp64;

    TRACE_EXTRACT("M:%i sopcde:%i sop:%i HLs:%i dst0:%i src0:%i src1:%i",
                  M, sopcde, sop, HLs, dst0, src0, src1);

    if ((sop == 0 || sop == 1) && sopcde == 0) {
        TCGLabel *l, *endl;
        TCGv val;

        /* Dreg{dst0}_hi{HLs&2} = ASHIFT Dreg{src1}_hi{HLs&1} BY
                                         Dreg_lo{src0} (S){sop==1}; */
        /* Dreg{dst0}_lo{!HLs&2} = ASHIFT Dreg{src1}_lo{!HLs&1} BY
                                          Dreg_lo{src0} (S){sop==1}; */

        tmp = tcg_temp_local_new();
        gen_extNsi_tl(tmp, cpu_dreg[src0], 6);

        val = tcg_temp_local_new();
        if (HLs & 1) {
            tcg_gen_sari_tl(val, cpu_dreg[src1], 16);
        } else {
            tcg_gen_ext16s_tl(val, cpu_dreg[src1]);
        }

        /* Positive shift magnitudes produce Logical Left shifts.
         * Negative shift magnitudes produce Arithmetic Right shifts.
         */
        endl = gen_new_label();
        l = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_GE, tmp, 0, l);
        tcg_gen_neg_tl(tmp, tmp);
        tcg_gen_sar_tl(val, val, tmp);
        astat_queue_state1(dc, ASTAT_OP_ASHIFT16, val);
        tcg_gen_br(endl);
        gen_set_label(l);
        tcg_gen_shl_tl(val, val, tmp);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT16, val);
        gen_set_label(endl);

        if (HLs & 2) {
            gen_mov_h_tl(cpu_dreg[dst0], val);
        } else {
            gen_mov_l_tl(cpu_dreg[dst0], val);
        }

        tcg_temp_free(val);
        tcg_temp_free(tmp);

        /* XXX: Missing V updates */
    } else if (sop == 2 && sopcde == 0) {
        TCGLabel *l, *endl;
        TCGv val;

        /* Dreg{dst0}_hi{HLs&2} = LSHIFT Dreg{src1}_hi{HLs&1} BY
                                         Dreg_lo{src0}; */
        /* Dreg{dst0}_lo{!HLs&2} = LSHIFT Dreg{src1}_lo{!HLs&1} BY
                                          Dreg_lo{src0}; */

        tmp = tcg_temp_local_new();
        gen_extNsi_tl(tmp, cpu_dreg[src0], 6);

        val = tcg_temp_local_new();
        if (HLs & 1) {
            tcg_gen_shri_tl(val, cpu_dreg[src1], 16);
        } else {
            tcg_gen_ext16u_tl(val, cpu_dreg[src1]);
        }

        /* Negative shift magnitudes means shift right */
        endl = gen_new_label();
        l = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_GE, tmp, 0, l);
        tcg_gen_neg_tl(tmp, tmp);
        tcg_gen_shr_tl(val, val, tmp);
        tcg_gen_br(endl);
        gen_set_label(l);
        tcg_gen_shl_tl(val, val, tmp);
        gen_set_label(endl);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT16, val);

        if (HLs & 2) {
            gen_mov_h_tl(cpu_dreg[dst0], val);
        } else {
            gen_mov_l_tl(cpu_dreg[dst0], val);
        }

        tcg_temp_free(val);
        tcg_temp_free(tmp);

        /* XXX: Missing AZ/AN/V updates */
    } else if (sop == 2 && sopcde == 3 && (HLs == 1 || HLs == 0)) {
        /* Areg{HLs} = ROT Areg{HLs} BY Dreg_lo{src0}; */
        tmp64 = tcg_temp_local_new_i64();
        tcg_gen_extu_i32_i64(tmp64, cpu_dreg[src0]);
        tcg_gen_ext16s_i64(tmp64, tmp64);
        gen_rot_i64(cpu_areg[HLs], cpu_areg[HLs], tmp64);
        tcg_temp_free_i64(tmp64);
    } else if (sop == 0 && sopcde == 3 && (HLs == 0 || HLs == 1)) {
        /* Areg{HLs} = ASHIFT Areg{HLs} BY Dregs_lo{src0}; */
        unhandled_instruction(dc, "ASHIFT ACC");
    } else if (sop == 1 && sopcde == 3 && (HLs == 0 || HLs == 1)) {
        /* Areg{HLs} = LSHIFT Areg{HLs} BY Dregs_lo{src0}; */
        unhandled_instruction(dc, "LSHIFT ACC");
    } else if ((sop == 0 || sop == 1) && sopcde == 1) {
        /* Dreg{dst0} = ASHIFT Dreg{src1} BY Dreg{src0} (V){sop==0}; */
        /* Dreg{dst0} = ASHIFT Dreg{src1} BY Dreg{src0} (V,S){sop==1}; */
        unhandled_instruction(dc, "ASHIFT V");
    } else if ((sop == 0 || sop == 1 || sop == 2) && sopcde == 2) {
        /* Dreg{dst0} = [LA]SHIFT Dreg{src1} BY Dreg_lo{src0} (opt_S); */
        /* sop == 1 : opt_S */
        TCGLabel *l, *endl;

        /* XXX: Missing V/VS update */
        if (sop == 1) {
            unhandled_instruction(dc, "[AL]SHIFT with (S)");
        }

        tmp = tcg_temp_local_new();
        gen_extNsi_tl(tmp, cpu_dreg[src0], 6);

        /* Negative shift means logical or arith shift right */
        endl = gen_new_label();
        l = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_GE, tmp, 0, l);
        tcg_gen_neg_tl(tmp, tmp);
        if (sop == 2) {
            tcg_gen_shr_tl(cpu_dreg[dst0], cpu_dreg[src1], tmp);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT_RT32, cpu_dreg[dst0]);
        } else {
            tcg_gen_sar_tl(cpu_dreg[dst0], cpu_dreg[src1], tmp);
            astat_queue_state1(dc, ASTAT_OP_ASHIFT32, cpu_dreg[dst0]);
        }
        tcg_gen_br(endl);

        /* Positive shift is a logical left shift */
        gen_set_label(l);
        tcg_gen_shl_tl(cpu_dreg[dst0], cpu_dreg[src1], tmp);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst0]);
        gen_set_label(endl);

        tcg_temp_free(tmp);
    } else if (sop == 3 && sopcde == 2) {
        /* Dreg{dst0} = ROT Dreg{src1} BY Dreg_lo{src0}; */
        tmp = tcg_temp_local_new();
        tcg_gen_ext16s_tl(tmp, cpu_dreg[src0]);
        gen_rot_tl(cpu_dreg[dst0], cpu_dreg[src1], tmp);
        tcg_temp_free(tmp);
    } else if (sop == 2 && sopcde == 1) {
        /* Dreg{dst0} = LSHIFT Dreg{src1} BY Dreg_lo{src0} (V); */
        unhandled_instruction(dc, "LSHIFT (V)");
    } else if (sopcde == 4) {
        /* Dreg{dst0} = PACK (Dreg{src1}_hi{sop&2}, Dreg{src0}_hi{sop&1}); */
        /* Dreg{dst0} = PACK (Dreg{src1}_lo{!sop&2}, Dreg{src0}_lo{!sop&1}); */
        TCGv tmph;
        tmp = tcg_temp_new();
        if (sop & 1) {
            tcg_gen_shri_tl(tmp, cpu_dreg[src0], 16);
        } else {
            tcg_gen_andi_tl(tmp, cpu_dreg[src0], 0xffff);
        }
        tmph = tcg_temp_new();
        if (sop & 2) {
            tcg_gen_andi_tl(tmph, cpu_dreg[src1], 0xffff0000);
        } else {
            tcg_gen_shli_tl(tmph, cpu_dreg[src1], 16);
        }
        tcg_gen_or_tl(cpu_dreg[dst0], tmph, tmp);
        tcg_temp_free(tmph);
        tcg_temp_free(tmp);
    } else if (sop == 0 && sopcde == 5) {
        /* Dreg_lo{dst0} = SIGNBITS Dreg{src1}; */
        tmp = tcg_temp_new();
        gen_helper_signbits_32(tmp, cpu_dreg[src1]);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if (sop == 1 && sopcde == 5) {
        /* Dreg_lo{dst0} = SIGNBITS Dreg_lo{src1}; */
        tmp = tcg_temp_new();
        gen_helper_signbits_16(tmp, cpu_dreg[src1]);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if (sop == 2 && sopcde == 5) {
        /* Dreg_lo{dst0} = SIGNBITS Dreg_hi{src1}; */
        tmp = tcg_temp_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[src1], 16);
        gen_helper_signbits_16(tmp, tmp);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if ((sop == 0 || sop == 1) && sopcde == 6) {
        /* Dreg_lo{dst0} = SIGNBITS Areg{sop}; */
        tmp = tcg_temp_new();
        gen_helper_signbits_40(tmp, cpu_areg[sop]);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if (sop == 3 && sopcde == 6) {
        /* Dreg_lo{dst0} = ONES Dreg{src1}; */
        tmp = tcg_temp_new();
        gen_helper_ones(tmp, cpu_dreg[src1]);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if (sop == 0 && sopcde == 7) {
        /* Dreg_lo{dst0} = EXPADJ( Dreg{src1}, Dreg_lo{src0}); */
        unhandled_instruction(dc, "EXPADJ");
    } else if (sop == 1 && sopcde == 7) {
        /* Dreg_lo{dst0} = EXPADJ( Dreg{src1}, Dreg_lo{src0}) (V); */
        unhandled_instruction(dc, "EXPADJ (V)");
    } else if (sop == 2 && sopcde == 7) {
        /* Dreg_lo{dst0} = EXPADJ( Dreg_lo{src1}, Dreg_lo{src0}) (V); */
        unhandled_instruction(dc, "EXPADJ");
    } else if (sop == 3 && sopcde == 7) {
        /* Dreg_lo{dst0} = EXPADJ( Dreg_hi{src1}, Dreg_lo{src0}); */
        unhandled_instruction(dc, "EXPADJ");
    } else if (sop == 0 && sopcde == 8) {
        /* BITMUX (Dreg{src0}, Dreg{src1}, A0) (ASR); */
        unhandled_instruction(dc, "BITMUX");
    } else if (sop == 1 && sopcde == 8) {
        /* BITMUX (Dreg{src0}, Dreg{src1}, A0) (ASL); */
        unhandled_instruction(dc, "BITMUX");
    } else if ((sop == 0 || sop == 1) && sopcde == 9) {
        /* Dreg_lo{dst0} = VIT_MAX (Dreg{src1}) (ASL){sop==0}; */
        /* Dreg_lo{dst0} = VIT_MAX (Dreg{src1}) (ASR){sop==1}; */
        TCGv sl, sh;
        TCGLabel *l;

        gen_extend_acc(cpu_areg[0]);
        if (sop & 1) {
            tcg_gen_shri_i64(cpu_areg[0], cpu_areg[0], 1);
        } else {
            tcg_gen_shli_i64(cpu_areg[0], cpu_areg[0], 1);
        }

        sl = tcg_temp_local_new();
        sh = tcg_temp_local_new();
        tmp = tcg_temp_local_new();

        tcg_gen_ext16s_tl(sl, cpu_dreg[src1]);
        tcg_gen_sari_tl(sh, cpu_dreg[src1], 16);
        /* Hrm, can't this sub be inlined in the branch ? */
        tcg_gen_sub_tl(tmp, sh, sl);
        tcg_gen_andi_tl(tmp, tmp, 0x8000);
        l = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_NE, tmp, 0, l);
        tcg_gen_mov_tl(sl, sh);
        tcg_gen_ori_i64(cpu_areg[0], cpu_areg[0], (sop & 1) ? 0x80000000 : 1);
        gen_set_label(l);

        gen_mov_l_tl(cpu_dreg[dst0], sl);

        tcg_temp_free(tmp);
        tcg_temp_free(sh);
        tcg_temp_free(sl);
    } else if ((sop == 2 || sop == 3) && sopcde == 9) {
        /* Dreg{dst0} = VIT_MAX (Dreg{src1}, Dreg{src0}) (ASL){sop==0}; */
        /* Dreg{dst0} = VIT_MAX (Dreg{src1}, Dreg{src0}) (ASR){sop==1}; */
        TCGv sl, sh, dst;
        TCGLabel *l;

        gen_extend_acc(cpu_areg[0]);
        if (sop & 1) {
            tcg_gen_shri_i64(cpu_areg[0], cpu_areg[0], 2);
        } else {
            tcg_gen_shli_i64(cpu_areg[0], cpu_areg[0], 2);
        }

        sl = tcg_temp_local_new();
        sh = tcg_temp_local_new();
        tmp = tcg_temp_local_new();

        tcg_gen_ext16s_tl(sl, cpu_dreg[src1]);
        tcg_gen_sari_tl(sh, cpu_dreg[src1], 16);

        /* Hrm, can't this sub be inlined in the branch ? */
        tcg_gen_sub_tl(tmp, sh, sl);
        tcg_gen_andi_tl(tmp, tmp, 0x8000);
        l = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_NE, tmp, 0, l);
        tcg_gen_mov_tl(sl, sh);
        tcg_gen_ori_i64(cpu_areg[0], cpu_areg[0], (sop & 1) ? 0x80000000 : 1);
        gen_set_label(l);

        /* The dst might be a src reg */
        if (dst0 == src0) {
            dst = tcg_temp_local_new();
        } else {
            dst = cpu_dreg[dst0];
        }

        tcg_gen_shli_tl(dst, sl, 16);

        tcg_gen_ext16s_tl(sl, cpu_dreg[src0]);
        tcg_gen_sari_tl(sh, cpu_dreg[src0], 16);
        /* Hrm, can't this sub be inlined in the branch ? */
        tcg_gen_sub_tl(tmp, sh, sl);
        tcg_gen_andi_tl(tmp, tmp, 0x8000);
        l = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_NE, tmp, 0, l);
        tcg_gen_mov_tl(sl, sh);
        tcg_gen_ori_i64(cpu_areg[0], cpu_areg[0], (sop & 1) ? 0x40000000 : 2);
        gen_set_label(l);

        gen_mov_l_tl(dst, sl);

        if (dst0 == src0) {
            tcg_gen_mov_tl(cpu_dreg[dst0], dst);
            tcg_temp_free(dst);
        }

        tcg_temp_free(tmp);
        tcg_temp_free(sh);
        tcg_temp_free(sl);
    } else if ((sop == 0 || sop == 1) && sopcde == 10) {
        /* Dreg{dst0} = EXTRACT (Dreg{src1}, Dreg_lo{src0}) (X{sop==1}); */
        /* Dreg{dst0} = EXTRACT (Dreg{src1}, Dreg_lo{src0}) (Z{sop==0}); */
        TCGv mask, x, sgn;

        /* mask = 1 << (src0 & 0x1f) */
        tmp = tcg_temp_new();
        tcg_gen_andi_tl(tmp, cpu_dreg[src0], 0x1f);
        mask = tcg_temp_local_new();
        tcg_gen_movi_tl(mask, 1);
        tcg_gen_shl_tl(mask, mask, tmp);
        tcg_temp_free(tmp);
        if (sop) {
            /* sgn = mask >> 1 */
            sgn = tcg_temp_local_new();
            tcg_gen_shri_tl(sgn, mask, 1);
        }
        /* mask -= 1 */
        tcg_gen_subi_tl(mask, mask, 1);

        /* x = src1 >> ((src0 >> 8) & 0x1f) */
        tmp = tcg_temp_new();
        x = tcg_temp_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[src0], 8);
        tcg_gen_andi_tl(tmp, tmp, 0x1f);
        tcg_gen_shr_tl(x, cpu_dreg[src1], tmp);
        tcg_temp_free(tmp);
        /* dst0 = x & mask */
        tcg_gen_and_tl(cpu_dreg[dst0], x, mask);
        tcg_temp_free(x);

        if (sop) {
            /* if (dst0 & sgn) dst0 |= ~mask */
            TCGLabel *l;
            l = gen_new_label();
            tmp = tcg_temp_new();
            tcg_gen_and_tl(tmp, cpu_dreg[dst0], sgn);
            tcg_gen_brcondi_tl(TCG_COND_EQ, tmp, 0, l);
            tcg_gen_not_tl(mask, mask);
            tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], mask);
            gen_set_label(l);
            tcg_temp_free(sgn);
            tcg_temp_free(tmp);
        }

        tcg_temp_free(mask);

        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst0]);
    } else if ((sop == 2 || sop == 3) && sopcde == 10) {
        /* The first dregs is the "background" while the second dregs is the
         * "foreground".  The fg reg is used to overlay the bg reg and is:
         * | nnnn nnnn | nnnn nnnn | xxxp pppp | xxxL LLLL |
         *  n = the fg bit field
         *  p = bit position in bg reg to start LSB of fg field
         *  L = number of fg bits to extract
         * Using (X) sign-extends the fg bit field.
         */
        TCGv fg, bg, len, mask, fgnd, shft;

        /* Dreg{dst0} = DEPOSIT (Dreg{src1}, Dreg{src0}) (X){sop==3}; */
        fg = cpu_dreg[src0];
        bg = cpu_dreg[src1];

        len = tcg_temp_new();
        tcg_gen_andi_tl(len, fg, 0x1f);

        mask = tcg_temp_new();
        tcg_gen_movi_tl(mask, 1);
        tcg_gen_shl_tl(mask, mask, len);
        tcg_gen_subi_tl(mask, mask, 1);
        tcg_gen_andi_tl(mask, mask, 0xffff);

        fgnd = tcg_temp_new();
        tcg_gen_shri_tl(fgnd, fg, 16);
        tcg_gen_and_tl(fgnd, fgnd, mask);

        shft = tcg_temp_new();
        tcg_gen_shri_tl(shft, fg, 8);
        tcg_gen_andi_tl(shft, shft, 0x1f);

        if (sop == 3) {
            /* Sign extend the fg bit field.  */
            tcg_gen_movi_tl(mask, -1);
            gen_extNs_tl(fgnd, fgnd, len);
        }
        tcg_gen_shl_tl(fgnd, fgnd, shft);
        tcg_gen_shl_tl(mask, mask, shft);
        tcg_gen_not_tl(mask, mask);
        tcg_gen_and_tl(mask, bg, mask);

        tcg_gen_or_tl(cpu_dreg[dst0], mask, fgnd);

        tcg_temp_free(shft);
        tcg_temp_free(fgnd);
        tcg_temp_free(mask);
        tcg_temp_free(len);

        astat_queue_state1(dc, ASTAT_OP_LOGICAL, cpu_dreg[dst0]);
    } else if (sop == 0 && sopcde == 11) {
        /* Dreg_lo{dst0} = CC = BXORSHIFT (A0, Dreg{src0}); */
        unhandled_instruction(dc, "BXORSHIFT");
    } else if (sop == 1 && sopcde == 11) {
        /* Dreg_lo{dst0} = CC = BXOR (A0, Dreg{src0}); */
        unhandled_instruction(dc, "BXOR");
    } else if (sop == 0 && sopcde == 12) {
        /* A0 = BXORSHIFT (A0, A1, CC); */
        unhandled_instruction(dc, "BXORSHIFT");
    } else if (sop == 1 && sopcde == 12) {
        /* Dreg_lo{dst0} = CC = BXOR (A0, A1, CC); */
        unhandled_instruction(dc, "CC = BXOR");
    } else if ((sop == 0 || sop == 1 || sop == 2) && sopcde == 13) {
        int shift = (sop + 1) * 8;
        TCGv tmp2;
        /* Dreg{dst0} = ALIGN{shift} (Dreg{src1}, Dreg{src0}); */
        /* XXX: could be optimized a bit if dst0 is not src1 or src0 */
        tmp = tcg_temp_new();
        tmp2 = tcg_temp_new();
        tcg_gen_shli_tl(tmp, cpu_dreg[src1], 32 - shift);
        tcg_gen_shri_tl(tmp2, cpu_dreg[src0], shift);
        tcg_gen_or_tl(cpu_dreg[dst0], tmp, tmp2);
        tcg_temp_free(tmp2);
        tcg_temp_free(tmp);
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_dsp32shiftimm_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* dsp32shiftimm
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 0 | 0 |.M.| 1 | 1 | 0 | 1 | - | - |.sopcde............|
       |.sop...|.HLs...|.dst0......|.immag.................|.src1......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int src1     = ((iw1 >> DSP32ShiftImm_src1_bits) & DSP32ShiftImm_src1_mask);
    int sop      = ((iw1 >> DSP32ShiftImm_sop_bits) & DSP32ShiftImm_sop_mask);
    int bit8     = ((iw1 >> 8) & 0x1);
    int immag    = ((iw1 >> DSP32ShiftImm_immag_bits) &
                    DSP32ShiftImm_immag_mask);
    int newimmag = (-(iw1 >> DSP32ShiftImm_immag_bits) &
                    DSP32ShiftImm_immag_mask);
    int dst0     = ((iw1 >> DSP32ShiftImm_dst0_bits) &
                    DSP32ShiftImm_dst0_mask);
    int M        = ((iw0 >> (DSP32ShiftImm_M_bits - 16)) &
                    DSP32ShiftImm_M_mask);
    int sopcde   = ((iw0 >> (DSP32ShiftImm_sopcde_bits - 16)) &
                    DSP32ShiftImm_sopcde_mask);
    int HLs      = ((iw1 >> DSP32ShiftImm_HLs_bits) & DSP32ShiftImm_HLs_mask);
    TCGv tmp;

    TRACE_EXTRACT("M:%i sopcde:%i sop:%i HLs:%i dst0:%i immag:%#x src1:%i",
                  M, sopcde, sop, HLs, dst0, immag, src1);

    if (sopcde == 0) {
        tmp = tcg_temp_new();

        if (HLs & 1) {
            if (sop == 0) {
                tcg_gen_sari_tl(tmp, cpu_dreg[src1], 16);
            } else {
                tcg_gen_shri_tl(tmp, cpu_dreg[src1], 16);
            }
        } else {
            if (sop == 0) {
                tcg_gen_ext16s_tl(tmp, cpu_dreg[src1]);
            } else {
                tcg_gen_ext16u_tl(tmp, cpu_dreg[src1]);
            }
        }

        if (sop == 0) {
            /* dregs_hi/lo = dregs_hi/lo >>> imm4 */
            tcg_gen_sari_tl(tmp, tmp, newimmag);
            astat_queue_state1(dc, ASTAT_OP_ASHIFT16, tmp);
        } else if (sop == 1 && bit8 == 0) {
            /*  dregs_hi/lo = dregs_hi/lo << imm4 (S) */
            tcg_gen_shli_tl(tmp, tmp, immag);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT16, tmp);
        } else if (sop == 1 && bit8) {
            /* dregs_hi/lo = dregs_hi/lo >>> imm4 (S) */
            tcg_gen_shri_tl(tmp, tmp, immag);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT16, tmp);
        } else if (sop == 2 && bit8) {
            /* dregs_hi/lo = dregs_hi/lo >> imm4 */
            tcg_gen_shri_tl(tmp, tmp, newimmag);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT16, tmp);
        } else if (sop == 2 && bit8 == 0) {
            /* dregs_hi/lo = dregs_hi/lo << imm4 */
            tcg_gen_shli_tl(tmp, tmp, immag);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT16, tmp);
        } else {
            illegal_instruction(dc);
        }

        if (HLs & 2) {
            gen_mov_h_tl(cpu_dreg[dst0], tmp);
        } else {
            gen_mov_l_tl(cpu_dreg[dst0], tmp);
        }

        tcg_temp_free(tmp);
    } else if (sop == 2 && sopcde == 3 && (HLs == 1 || HLs == 0)) {
        /* Areg{HLs} = ROT Areg{HLs} BY imm{immag}; */
        int shift = imm6(immag);
        gen_roti_i64(cpu_areg[HLs], cpu_areg[HLs], shift);
    } else if (sop == 0 && sopcde == 3 && bit8 == 1) {
        /* Arithmetic shift, so shift in sign bit copies */
        int shift = uimm5(newimmag);
        HLs = !!HLs;

        /* Areg{HLs} = Aregs{HLs} >>> imm{newimmag}; */
        tcg_gen_sari_i64(cpu_areg[HLs], cpu_areg[HLs], shift);
    } else if ((sop == 0 && sopcde == 3 && bit8 == 0) ||
               (sop == 1 && sopcde == 3)) {
        int shiftup = uimm5(immag);
        int shiftdn = uimm5(newimmag);
        HLs = !!HLs;

        if (sop == 0) {
            /* Areg{HLs} = Aregs{HLs} <<{sop} imm{immag}; */
            tcg_gen_shli_i64(cpu_areg[HLs], cpu_areg[HLs], shiftup);
        } else {
            /* Areg{HLs} = Aregs{HLs} >>{sop} imm{newimmag}; */
            tcg_gen_shri_i64(cpu_areg[HLs], cpu_areg[HLs], shiftdn);
        }

        /* XXX: Missing ASTAT update */
    } else if (sop == 1 && sopcde == 1 && bit8 == 0) {
        /* Dreg{dst0} = Dreg{src1} << imm{immag} (V, S); */
        unhandled_instruction(dc, "Dreg = Dreg << imm (V,S)");
    } else if (sop == 2 && sopcde == 1 && bit8 == 1) {
        /* Dreg{dst0} = Dreg{src1} >> imm{count} (V); */
        int count = imm5(newimmag);

        /* XXX: No ASTAT handling */
        if (count > 0 && count <= 15) {
            tcg_gen_shri_tl(cpu_dreg[dst0], cpu_dreg[src1], count);
            tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0],
                            0xffff0000 | ((1 << (16 - count)) - 1));
        } else if (count) {
            tcg_gen_movi_tl(cpu_dreg[dst0], 0);
        }
    } else if (sop == 2 && sopcde == 1 && bit8 == 0) {
        /* Dreg{dst0} = Dreg{src1} << imm{count} (V); */
        int count = imm5(immag);

        /* XXX: No ASTAT handling */
        if (count > 0 && count <= 15) {
            tcg_gen_shli_tl(cpu_dreg[dst0], cpu_dreg[src1], count);
            tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0],
                            ~(((1 << count) - 1) << 16));
        } else if (count) {
            tcg_gen_movi_tl(cpu_dreg[dst0], 0);
        }
    } else if (sopcde == 1 && (sop == 0 || (sop == 1 && bit8 == 1))) {
        /* Dreg{dst0} = Dreg{src1} >>> imm{newimmag} (V){sop==0}; */
        /* Dreg{dst0} = Dreg{src1} >>> imm{newimmag} (V,S){sop==1}; */
        int count = uimm5(newimmag);

        if (sop == 1) {
            unhandled_instruction(dc, "ashiftrt (S)");
        }

        /* XXX: No ASTAT handling */
        if (count > 0 && count <= 15) {
            tmp = tcg_temp_new();
            tcg_gen_ext16s_tl(tmp, cpu_dreg[src1]);
            tcg_gen_sari_tl(tmp, tmp, count);
            tcg_gen_andi_tl(tmp, tmp, 0xffff);
            tcg_gen_sari_tl(cpu_dreg[dst0], cpu_dreg[src1], count);
            tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0], 0xffff0000);
            tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], tmp);
            tcg_temp_free(tmp);
        } else if (count) {
            unhandled_instruction(dc, "ashiftrt (S)");
        }
    } else if (sop == 1 && sopcde == 2) {
        /* Dreg{dst0} = Dreg{src1} << imm{count} (S); */
        int count = imm6(immag);
        tcg_gen_shli_tl(cpu_dreg[dst0], cpu_dreg[src1], -count);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst0]);
    } else if (sop == 2 && sopcde == 2) {
        /* Dreg{dst0} = Dreg{src1} >> imm{count}; */
        int count = imm6(newimmag);
        if (count < 0) {
            tcg_gen_shli_tl(cpu_dreg[dst0], cpu_dreg[src1], -count);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst0]);
        } else {
            tcg_gen_shri_tl(cpu_dreg[dst0], cpu_dreg[src1], count);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT_RT32, cpu_dreg[dst0]);
        }
    } else if (sop == 3 && sopcde == 2) {
        /* Dreg{dst0} = ROT Dreg{src1} BY imm{shift}; */
        int shift = imm6(immag);
        gen_roti_tl(cpu_dreg[dst0], cpu_dreg[src1], shift);
    } else if (sop == 0 && sopcde == 2) {
        /* Dreg{dst0} = Dreg{src1} >>> imm{count}; */
        int count = imm6(newimmag);

        /* Negative shift magnitudes produce Logical Left shifts.
         * Positive shift magnitudes produce Arithmetic Right shifts.
         */
        if (count < 0) {
            tcg_gen_shli_tl(cpu_dreg[dst0], cpu_dreg[src1], -count);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst0]);
        } else {
            tcg_gen_sari_tl(cpu_dreg[dst0], cpu_dreg[src1], count);
            astat_queue_state1(dc, ASTAT_OP_ASHIFT32, cpu_dreg[dst0]);
        }
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_psedoDEBUG_0(DisasContext *dc, uint16_t iw0)
{
    /* psedoDEBUG
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 1 | 1 | 1 | 0 | 0 | 0 |.fn....|.grp.......|.reg.......|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int fn  = ((iw0 >> PseudoDbg_fn_bits) & PseudoDbg_fn_mask);
    int grp = ((iw0 >> PseudoDbg_grp_bits) & PseudoDbg_grp_mask);
    int reg = ((iw0 >> PseudoDbg_reg_bits) & PseudoDbg_reg_mask);

    TRACE_EXTRACT("fn:%i grp:%i reg:%i", fn, grp, reg);

    if ((reg == 0 || reg == 1) && fn == 3) {
        /* DBG Areg{reg}; */
        TCGv tmp = tcg_const_tl(reg);
        gen_helper_dbg_areg(cpu_areg[reg], tmp);
        tcg_temp_free(tmp);
    } else if (reg == 3 && fn == 3) {
        /* ABORT; */
        cec_exception(dc, EXCP_ABORT);
    } else if (reg == 4 && fn == 3) {
        /* HLT; */
        cec_exception(dc, EXCP_HLT);
    } else if (reg == 5 && fn == 3) {
        unhandled_instruction(dc, "DBGHALT");
    } else if (reg == 6 && fn == 3) {
        unhandled_instruction(dc, "DBGCMPLX (dregs)");
    } else if (reg == 7 && fn == 3) {
        unhandled_instruction(dc, "DBG");
    } else if (grp == 0 && fn == 2) {
        /* OUTC Dreg{reg}; */
        gen_helper_outc(cpu_dreg[reg]);
    } else if (fn == 0) {
        /* DBG allreg{grp,reg}; */
        bool istmp;
        TCGv tmp;
        TCGv tmp_grp = tcg_const_tl(grp);
        TCGv tmp_reg = tcg_const_tl(reg);

        if (grp == 4 && reg == 6) {
            /* ASTAT */
            tmp = tcg_temp_new();
            gen_astat_load(dc, tmp);
            istmp = true;
        } else {
            tmp = get_allreg(dc, grp, reg);
            istmp = false;
        }

        gen_helper_dbg(tmp, tmp_grp, tmp_reg);

        if (istmp) {
            tcg_temp_free(tmp);
        }
        tcg_temp_free(tmp_reg);
        tcg_temp_free(tmp_grp);
    } else if (fn == 1) {
        unhandled_instruction(dc, "PRNT allregs");
    } else {
        illegal_instruction(dc);
    }
}

static void
decode_psedoOChar_0(DisasContext *dc, uint16_t iw0)
{
    /* psedoOChar
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 1 | 1 | 1 | 0 | 0 | 1 |.ch............................|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int ch = ((iw0 >> PseudoChr_ch_bits) & PseudoChr_ch_mask);
    TCGv tmp;

    TRACE_EXTRACT("ch:%#x", ch);

    /* OUTC imm{ch}; */
    tmp = tcg_temp_new();
    tcg_gen_movi_tl(tmp, ch);
    gen_helper_outc(tmp);
    tcg_temp_free(tmp);
}

static void
decode_psedodbg_assert_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
    /* psedodbg_assert
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
       | 1 | 1 | 1 | 1 | 0 | - | - | - | dbgop |.grp.......|.regtest...|
       |.expected......................................................|
       +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
    int expected = ((iw1 >> PseudoDbg_Assert_expected_bits) &
                    PseudoDbg_Assert_expected_mask);
    int dbgop    = ((iw0 >> (PseudoDbg_Assert_dbgop_bits - 16)) &
                    PseudoDbg_Assert_dbgop_mask);
    int grp      = ((iw0 >> (PseudoDbg_Assert_grp_bits - 16)) &
                    PseudoDbg_Assert_grp_mask);
    int regtest  = ((iw0 >> (PseudoDbg_Assert_regtest_bits - 16)) &
                    PseudoDbg_Assert_regtest_mask);
    TCGv reg, exp, pc;
    bool istmp;

    TRACE_EXTRACT("dbgop:%i grp:%i regtest:%i expected:%#x",
                  dbgop, grp, regtest, expected);

    if (dbgop == 0 || dbgop == 2) {
        /* DBGA (genreg_lo{grp,regtest}, imm{expected} */
        /* DBGAL (genreg{grp,regtest}, imm{expected} */
    } else if (dbgop == 1 || dbgop == 3) {
        /* DBGA (genreg_hi{grp,regtest}, imm{expected} */
        /* DBGAH (genreg{grp,regtest}, imm{expected} */
    } else {
        illegal_instruction(dc);
        return;
    }

    if (grp == 4 && regtest == 6) {
        /* ASTAT */
        reg = tcg_temp_new();
        gen_astat_load(dc, reg);
        istmp = true;
    } else if (grp == 4 && (regtest == 0 || regtest == 2)) {
        /* A#.X */
        TCGv_i64 tmp64 = tcg_temp_new_i64();
        reg = tcg_temp_new();
        tcg_gen_shri_i64(tmp64, cpu_areg[regtest >> 1], 32);
        tcg_gen_andi_i64(tmp64, tmp64, 0xff);
        tcg_gen_extrl_i64_i32(reg, tmp64);
        tcg_temp_free_i64(tmp64);
        istmp = true;
    } else if (grp == 4 && (regtest == 1 || regtest == 3)) {
        /* A#.W */
        reg = tcg_temp_new();
        tcg_gen_extrl_i64_i32(reg, cpu_areg[regtest >> 1]);
        istmp = true;
    } else {
        reg = get_allreg(dc, grp, regtest);
        istmp = false;
    }

    exp = tcg_const_tl(expected);
    pc = tcg_const_tl(dc->pc);
    if (dbgop & 1) {
        gen_helper_dbga_h(cpu_env, pc, reg, exp);
    } else {
        gen_helper_dbga_l(cpu_env, pc, reg, exp);
    }

    if (istmp) {
        tcg_temp_free(reg);
    }
    tcg_temp_free(pc);
    tcg_temp_free(exp);
}

#include "linux-fixed-code.h"

static uint32_t bfin_lduw_code(DisasContext *dc, target_ulong pc)
{
#ifdef CONFIG_USER_ONLY
    /* Intercept jump to the magic kernel page */
    if (((dc->env->personality & 0xff/*PER_MASK*/) == 0/*PER_LINUX*/) &&
        (pc & 0xFFFFFF00) == 0x400) {
        uint32_t off = pc - 0x400;
        if (off < sizeof(bfin_linux_fixed_code)) {
            return ((uint16_t)bfin_linux_fixed_code[off + 1] << 8) |
                   bfin_linux_fixed_code[off];
        }
    }
#endif

    return cpu_lduw_code(dc->env, pc);
}

/* Interpret a single 16bit/32bit insn; no parallel insn handling */
static void
_interp_insn_bfin(DisasContext *dc, target_ulong pc)
{
    uint16_t iw0, iw1;

    iw0 = bfin_lduw_code(dc, pc);
    if ((iw0 & 0xc000) != 0xc000) {
        /* 16-bit opcode */
        dc->insn_len = 2;

        TRACE_EXTRACT("iw0:%#x", iw0);
        if ((iw0 & 0xFF00) == 0x0000) {
            decode_ProgCtrl_0(dc, iw0);
        } else if ((iw0 & 0xFFC0) == 0x0240) {
            decode_CaCTRL_0(dc, iw0);
        } else if ((iw0 & 0xFF80) == 0x0100) {
            decode_PushPopReg_0(dc, iw0);
        } else if ((iw0 & 0xFE00) == 0x0400) {
            decode_PushPopMultiple_0(dc, iw0);
        } else if ((iw0 & 0xFE00) == 0x0600) {
            decode_ccMV_0(dc, iw0);
        } else if ((iw0 & 0xF800) == 0x0800) {
            decode_CCflag_0(dc, iw0);
        } else if ((iw0 & 0xFFE0) == 0x0200) {
            decode_CC2dreg_0(dc, iw0);
        } else if ((iw0 & 0xFF00) == 0x0300) {
            decode_CC2stat_0(dc, iw0);
        } else if ((iw0 & 0xF000) == 0x1000) {
            decode_BRCC_0(dc, iw0);
        } else if ((iw0 & 0xF000) == 0x2000) {
            decode_UJUMP_0(dc, iw0);
        } else if ((iw0 & 0xF000) == 0x3000) {
            decode_REGMV_0(dc, iw0);
        } else if ((iw0 & 0xFC00) == 0x4000) {
            decode_ALU2op_0(dc, iw0);
        } else if ((iw0 & 0xFE00) == 0x4400) {
            decode_PTR2op_0(dc, iw0);
        } else if ((iw0 & 0xF800) == 0x4800) {
            decode_LOGI2op_0(dc, iw0);
        } else if ((iw0 & 0xF000) == 0x5000) {
            decode_COMP3op_0(dc, iw0);
        } else if ((iw0 & 0xF800) == 0x6000) {
            decode_COMPI2opD_0(dc, iw0);
        } else if ((iw0 & 0xF800) == 0x6800) {
            decode_COMPI2opP_0(dc, iw0);
        } else if ((iw0 & 0xF000) == 0x8000) {
            decode_LDSTpmod_0(dc, iw0);
        } else if ((iw0 & 0xFF60) == 0x9E60) {
            decode_dagMODim_0(dc, iw0);
        } else if ((iw0 & 0xFFF0) == 0x9F60) {
            decode_dagMODik_0(dc, iw0);
        } else if ((iw0 & 0xFC00) == 0x9C00) {
            decode_dspLDST_0(dc, iw0);
        } else if ((iw0 & 0xF000) == 0x9000) {
            decode_LDST_0(dc, iw0);
        } else if ((iw0 & 0xFC00) == 0xB800) {
            decode_LDSTiiFP_0(dc, iw0);
        } else if ((iw0 & 0xE000) == 0xA000) {
            decode_LDSTii_0(dc, iw0);
        } else {
            TRACE_EXTRACT("no matching 16-bit pattern");
            illegal_instruction(dc);
        }
        return;
    }

    /* Grab the next 16 bits to determine if it's a 32-bit or 64-bit opcode */
    iw1 = bfin_lduw_code(dc, pc + 2);
    if ((iw0 & BIT_MULTI_INS) && (iw0 & 0xe800) != 0xe800 /* not linkage */) {
        dc->insn_len = 8;
    } else {
        dc->insn_len = 4;
    }

    TRACE_EXTRACT("iw0:%#x iw1:%#x insn_len:%i",
                  iw0, iw1, dc->insn_len);

    if ((iw0 & 0xf7ff) == 0xc003 && iw1 == 0x1800) {
        /* MNOP; */;
    } else if (((iw0 & 0xFF80) == 0xE080) && ((iw1 & 0x0C00) == 0x0000)) {
        decode_LoopSetup_0(dc, iw0, iw1);
    } else if (((iw0 & 0xFF00) == 0xE100) && ((iw1 & 0x0000) == 0x0000)) {
        decode_LDIMMhalf_0(dc, iw0, iw1);
    } else if (((iw0 & 0xFE00) == 0xE200) && ((iw1 & 0x0000) == 0x0000)) {
        decode_CALLa_0(dc, iw0, iw1);
    } else if (((iw0 & 0xFC00) == 0xE400) && ((iw1 & 0x0000) == 0x0000)) {
        decode_LDSTidxI_0(dc, iw0, iw1);
    } else if (((iw0 & 0xFFFE) == 0xE800) && ((iw1 & 0x0000) == 0x0000)) {
        decode_linkage_0(dc, iw0, iw1);
    } else if (((iw0 & 0xF600) == 0xC000) && ((iw1 & 0x0000) == 0x0000)) {
        decode_dsp32mac_0(dc, iw0, iw1);
    } else if (((iw0 & 0xF600) == 0xC200) && ((iw1 & 0x0000) == 0x0000)) {
        decode_dsp32mult_0(dc, iw0, iw1);
    } else if (((iw0 & 0xF7C0) == 0xC400) && ((iw1 & 0x0000) == 0x0000)) {
        decode_dsp32alu_0(dc, iw0, iw1);
    } else if (((iw0 & 0xF7E0) == 0xC600) && ((iw1 & 0x01C0) == 0x0000)) {
        decode_dsp32shift_0(dc, iw0, iw1);
    } else if (((iw0 & 0xF7E0) == 0xC680) && ((iw1 & 0x0000) == 0x0000)) {
        decode_dsp32shiftimm_0(dc, iw0, iw1);
    } else if ((iw0 & 0xFF00) == 0xF800) {
        decode_psedoDEBUG_0(dc, iw0), dc->insn_len = 2;
    } else if ((iw0 & 0xFF00) == 0xF900) {
        decode_psedoOChar_0(dc, iw0), dc->insn_len = 2;
    } else if (((iw0 & 0xFF00) == 0xF000) && ((iw1 & 0x0000) == 0x0000)) {
        decode_psedodbg_assert_0(dc, iw0, iw1);
    } else {
        TRACE_EXTRACT("no matching 32-bit pattern");
        illegal_instruction(dc);
    }
}

/* Interpret a single Blackfin insn; breaks up parallel insns */
static void
interp_insn_bfin(DisasContext *dc)
{
    _interp_insn_bfin(dc, dc->pc);

    /* Proper display of multiple issue instructions */
    if (dc->insn_len == 8) {
        _interp_insn_bfin(dc, dc->pc + 4);
        _interp_insn_bfin(dc, dc->pc + 6);
        dc->disalgnexcpt = 0;
        /* Reset back for higher levels to process branches */
        dc->insn_len = 8;
    }
}
