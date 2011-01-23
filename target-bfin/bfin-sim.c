/* Simulator for Analog Devices Blackfin processors.
 *
 * Copyright 2005-2011 Mike Frysinger
 * Copyright 2005-2011 Analog Devices, Inc.
 *
 * Licensed under the GPL 2 or later.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define TRACE_INSN(cpu, fmt, args...) do { if (0) qemu_log_mask(CPU_LOG_TB_IN_ASM, fmt "\n", ## args); } while (0)
#define TRACE_EXTRACT(fmt, args...) do { if (1) qemu_log_mask(CPU_LOG_TB_CPU, fmt "\n", ## args); } while (0)

static void
illegal_instruction(DisasContext *dc)
{
    cec_exception(dc, EXCP_UNDEF_INST);
}

/*
static void
illegal_instruction_combination(DisasContext *dc)
{
    cec_exception(dc, EXCP_ILL_INST);
}
*/

static void
unhandled_instruction(DisasContext *dc, const char *insn)
{
    fprintf(stderr, "unhandled insn: %s\n", insn);
    illegal_instruction(dc);
}

#define M_S2RND 1
#define M_T     2
#define M_W32   3
#define M_FU    4
#define M_TFU   6
#define M_IS    8
#define M_ISS2  9
#define M_IH    11
#define M_IU    12

/* Valid flag settings */
#define is_macmod_pmove(x) \
    (((x) == 0)       || \
     ((x) == M_IS)    || \
     ((x) == M_FU)    || \
     ((x) == M_S2RND) || \
     ((x) == M_ISS2)  || \
     ((x) == M_IU))

#define is_macmod_hmove(x) \
    (((x) == 0)       || \
     ((x) == M_IS)    || \
     ((x) == M_FU)    || \
     ((x) == M_IU)    || \
     ((x) == M_T)     || \
     ((x) == M_TFU)   || \
     ((x) == M_S2RND) || \
     ((x) == M_ISS2)  || \
     ((x) == M_IH))

typedef enum {
    c_0, c_1, c_4, c_2, c_uimm2, c_uimm3, c_imm3, c_pcrel4,
    c_imm4, c_uimm4s4, c_uimm4s4d, c_uimm4, c_uimm4s2, c_negimm5s4, c_imm5, c_imm5d, c_uimm5, c_imm6,
    c_imm7, c_imm7d, c_imm8, c_uimm8, c_pcrel8, c_uimm8s4, c_pcrel8s4, c_lppcrel10, c_pcrel10,
    c_pcrel12, c_imm16s4, c_luimm16, c_imm16, c_imm16d, c_huimm16, c_rimm16, c_imm16s2, c_uimm16s4,
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
#define SIGNEXTEND(v, n) (((int32_t)(v) << (HOST_LONG_WORD_SIZE - (n))) >> (HOST_LONG_WORD_SIZE - (n)))

static uint32_t
fmtconst_val(const_forms_t cf, uint32_t x)
{
    /* Negative constants have an implied sign bit.  */
    if (constant_formats[cf].negative) {
        int nb = constant_formats[cf].nbits + 1;
        x = x | (1 << constant_formats[cf].nbits);
        x = SIGNEXTEND(x, nb);
    } else if (constant_formats[cf].issigned)
        x = SIGNEXTEND(x, constant_formats[cf].nbits);

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

/*
static bool
reg_is_reserved(int grp, int reg)
{
    return (grp == 4 && (reg == 4 || reg == 5)) || (grp == 5);
}
*/

static TCGv * const cpu_regs[] = {
    &cpu_dreg[0], &cpu_dreg[1], &cpu_dreg[2], &cpu_dreg[3], &cpu_dreg[4], &cpu_dreg[5], &cpu_dreg[6], &cpu_dreg[7],
    &cpu_preg[0], &cpu_preg[1], &cpu_preg[2], &cpu_preg[3], &cpu_preg[4], &cpu_preg[5], &cpu_preg[6], &cpu_preg[7],
    &cpu_ireg[0], &cpu_ireg[1], &cpu_ireg[2], &cpu_ireg[3], &cpu_mreg[0], &cpu_mreg[1], &cpu_mreg[2], &cpu_mreg[3],
    &cpu_breg[0], &cpu_breg[1], &cpu_breg[2], &cpu_breg[3], &cpu_lreg[0], &cpu_lreg[1], &cpu_lreg[2], &cpu_lreg[3],
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cpu_rets,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    &cpu_lcreg[0], &cpu_ltreg[0], &cpu_lbreg[0], &cpu_lcreg[1], &cpu_ltreg[1], &cpu_lbreg[1], &cpu_cycles[0], &cpu_cycles[1],
    &cpu_uspreg, &cpu_seqstat, &cpu_syscfg, &cpu_reti, &cpu_retx, &cpu_retn, &cpu_rete, &cpu_emudat,
};

static TCGv
get_allreg(DisasContext *dc, int grp, int reg)
{
    TCGv *ret = cpu_regs[(grp << 3) | reg];
    if (ret)
       return *ret;
    abort();
    illegal_instruction(dc);
}

#if 0
static const char *
amod0amod2 (int s0, int x0, int aop0)
{
  if (s0 == 1 && x0 == 0 && aop0 == 0)
    return " (S)";
  else if (s0 == 0 && x0 == 1 && aop0 == 0)
    return " (CO)";
  else if (s0 == 1 && x0 == 1 && aop0 == 0)
    return " (SCO)";
  else if (s0 == 0 && x0 == 0 && aop0 == 2)
    return " (ASR)";
  else if (s0 == 1 && x0 == 0 && aop0 == 2)
    return " (S, ASR)";
  else if (s0 == 0 && x0 == 1 && aop0 == 2)
    return  " (CO, ASR)";
  else if (s0 == 1 && x0 == 1 && aop0 == 2)
    return " (SCO, ASR)";
  else if (s0 == 0 && x0 == 0 && aop0 == 3)
    return " (ASL)";
  else if (s0 == 1 && x0 == 0 && aop0 == 3)
    return " (S, ASL)";
  else if (s0 == 0 && x0 == 1 && aop0 == 3)
    return " (CO, ASL)";
  else if (s0 == 1 && x0 == 1 && aop0 == 3)
    return " (SCO, ASL)";
  return "";
}
#endif

static const char *
amod1 (int s0, int x0)
{
  if (s0 == 0 && x0 == 0)
    return " (NS)";
  else if (s0 == 1 && x0 == 0)
    return " (S)";
  return "";
}

static const char *
amod0 (int s0, int x0)
{
  if (s0 == 1 && x0 == 0)
    return " (S)";
  else if (s0 == 0 && x0 == 1)
    return " (CO)";
  else if (s0 == 1 && x0 == 1)
    return " (SCO)";
  return "";
}

static void
reg_check_sup(DisasContext *dc, int grp, int reg)
{
    if (grp == 7)
        cec_require_supervisor(dc);
}

/* Perform a multiplication of D registers SRC0 and SRC1, sign- or
   zero-extending the result to 64 bit.  H0 and H1 determine whether the
   high part or the low part of the source registers is used.  Store 1 in
   *PSAT if saturation occurs, 0 otherwise.  */
static TCGv
decode_multfunc_tl(DisasContext *dc, int h0, int h1, int src0, int src1,
                   int mmod, int MM, int *psat)
{
    TCGv s0, s1, val;

    s0 = tcg_temp_local_new();
    if (h0)
        tcg_gen_shri_tl(s0, cpu_dreg[src0], 16);
    else
        tcg_gen_andi_tl(s0, cpu_dreg[src0], 0xffff);

    s1 = tcg_temp_local_new();
    if (h1)
        tcg_gen_shri_tl(s1, cpu_dreg[src1], 16);
    else
        tcg_gen_andi_tl(s1, cpu_dreg[src1], 0xffff);

    if (MM)
        tcg_gen_ext16s_tl(s0, s0);
    else
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

    val = tcg_temp_local_new();
    tcg_gen_mul_tl(val, s0, s1);
    tcg_temp_free(s0);
    tcg_temp_free(s1);

    /* Perform shift correction if appropriate for the mode.  */
    *psat = 0;
    if (!MM && (mmod == 0 || mmod == M_T || mmod == M_S2RND || mmod == M_W32)) {
        int l, endl;

        l = gen_new_label();
        endl = gen_new_label();

        tcg_gen_brcondi_tl(TCG_COND_NE, val, 0x40000000, l);
        if (mmod == M_W32)
            tcg_gen_movi_tl(val, 0x7fffffff);
        else
            tcg_gen_movi_tl(val, 0x80000000);
//          *psat = 1;
        tcg_gen_br(endl);

        gen_set_label(l);
        tcg_gen_shli_tl(val, val, 1);

        gen_set_label(endl);
    }

    return val;
}

static TCGv_i64
decode_multfunc_i64(DisasContext *dc, int h0, int h1, int src0, int src1,
                    int mmod, int MM, int *psat)
{
    TCGv val;
    TCGv_i64 val1;

    val = decode_multfunc_tl(dc, h0, h1, src0, src1, mmod, MM, psat);
    val1 = tcg_temp_local_new_i64();
    tcg_gen_extu_i32_i64(val1, val);
    tcg_temp_free(val);

    if (mmod == 0 || mmod == M_IS || mmod == M_T || mmod == M_S2RND ||
        mmod == M_ISS2 || mmod == M_IH || (MM && mmod == M_FU)) {
        /* Shift the sign bit up, and then back down */
        tcg_gen_shli_i64(val1, val1, 64 - 40);
        tcg_gen_sari_i64(val1, val1, 64 - 40);
    }

//  if (*psat)
//    val1 &= 0xFFFFFFFFull;

  return val1;
}

#if 0
static bu40
saturate_s40_astat (bu64 val, bu32 *v)
{
  if ((bs64)val < -((bs64)1 << 39))
    {
      *v = 1;
      return -((bs64)1 << 39);
    }
  else if ((bs64)val >= ((bs64)1 << 39) - 1)
    {
      *v = 1;
      return ((bu64)1 << 39) - 1;
    }
  *v = 0; /* no overflow */
  return val;
}

static bu40
saturate_s40 (bu64 val)
{
  bu32 v;
  return saturate_s40_astat (val, &v);
}

static bu32
saturate_s32(bu64 val, bu32 *overflow)
{
  if ((bs64)val < -0x80000000ll)
    {
      if (overflow)
	*overflow = 1;
      return 0x80000000;
    }
  if ((bs64)val > 0x7fffffff)
    {
      if (overflow)
	*overflow = 1;
      return 0x7fffffff;
    }
  return val;
}

static bu32
saturate_u32(bu64 val, bu32 *overflow)
{
  if (val > 0xffffffff)
    {
      if (overflow)
	*overflow = 1;
      return 0xffffffff;
    }
  return val;
}

static bu32
saturate_u16(bu64 val, bu32 *overflow)
{
  if (val > 0xffff)
    {
      if (overflow)
	*overflow = 1;
      return 0xffff;
    }
  return val;
}

static bu64
rnd16(bu64 val)
{
  bu64 sgnbits;

  /* FIXME: Should honour rounding mode.  */
  if ((val & 0xffff) > 0x8000
      || ((val & 0xffff) == 0x8000 && (val & 0x10000)))
    val += 0x8000;

  sgnbits = val & 0xffff000000000000ull;
  val >>= 16;
  return val | sgnbits;
}

static bu64
trunc16(bu64 val)
{
  bu64 sgnbits = val & 0xffff000000000000ull;
  val >>= 16;
  return val | sgnbits;
}

/* Extract a 16 or 32 bit value from a 64 bit multiplication result.
   These 64 bits must be sign- or zero-extended properly from the source
   we want to extract, either a 32 bit multiply or a 40 bit accumulator.  */
static TCGv
extract_mult(DisasContext *dc, bu64 res, int mmod, int MM,
             int fullword, int *overflow)
{
    if (fullword)
        switch (mmod) {
        case 0:
        case M_IS:
            return saturate_s32(res, overflow);
        case M_FU:
            if (MM)
                return saturate_s32(res, overflow);
            return saturate_u32(res, overflow);
        case M_S2RND:
        case M_ISS2:
            return saturate_s32(res << 1, overflow);
        default:
            illegal_instruction(dc);
        }
    else
        switch (mmod) {
        case 0:
        case M_W32:
            return saturate_s16(rnd16(res), overflow);
        case M_IH:
            return saturate_s32(rnd16(res), overflow) & 0xFFFF;
        case M_IS:
            return saturate_s16(res, overflow);
        case M_FU:
            if (MM)
                return saturate_s16(rnd16(res), overflow);
            return saturate_u16(rnd16(res), overflow);
        case M_IU:
            if (MM)
                return saturate_s16(res, overflow);
            return saturate_u16(res, overflow);

        case M_T:
            return saturate_s16(trunc16(res), overflow);
        case M_TFU:
            return saturate_u16(trunc16(res), overflow);

        case M_S2RND:
            return saturate_s16(rnd16(res << 1), overflow);
        case M_ISS2:
            return saturate_s16(res << 1, overflow);
        default:
            illegal_instruction(dc);
        }
}
#endif

static TCGv
decode_macfunc (DisasContext *dc, int which, int op, int h0, int h1, int src0,
                int src1, int mmod, int MM, int fullword, int *overflow)
{
  TCGv_i64 acc;
//  bu32 sat = 0,
  int tsat;

  /* Sign extend accumulator if necessary, otherwise unsigned */
  if (mmod == 0 || mmod == M_T || mmod == M_IS || mmod == M_ISS2 || mmod == M_S2RND || mmod == M_IH || mmod == M_W32)
{}
    //acc = get_extended_acc (cpu, which);
  else
{}
    //acc = get_unextended_acc (cpu, which);
  acc = cpu_areg[which];

//  if (MM && (mmod == M_T || mmod == M_IS || mmod == M_ISS2 || mmod == M_S2RND || mmod == M_IH || mmod == M_W32))
//    acc |= -(acc & 0x80000000);

    if (op != 3) {
//      bu8 sgn0 = (acc >> 31) & 1;
        /* this can't saturate, so we don't keep track of the sat flag */
        TCGv_i64 res = decode_multfunc_i64(dc, h0, h1, src0, src1, mmod, MM, &tsat);

//      res64 = tcg_temp_local_new_i64();
//      tcg_gen_extu_i32_i64(res64, res);

        /* Perform accumulation.  */
        switch (op) {
        case 0:
            tcg_gen_mov_i64(acc, res);
//          sgn0 = (acc >> 31) & 1;
            break;
        case 1:
            tcg_gen_add_i64(acc, acc, res);
            break;
        case 2:
//          acc = acc - res;
            tcg_gen_sub_i64(acc, acc, res);
            break;
        }
        tcg_temp_free_i64(res);

      /* Saturate.  */
/*
      switch (mmod)
	{
	case 0:
	case M_T:
	case M_IS:
	case M_ISS2:
	case M_S2RND:
	  if ((bs64)acc < -((bs64)1 << 39))
	    acc = -((bu64)1 << 39), sat = 1;
	  else if ((bs64)acc > 0x7fffffffffll)
	    acc = 0x7fffffffffull, sat = 1;
	  break;
	case M_TFU:
	  if (!MM && acc > 0xFFFFFFFFFFull)
	    acc = 0x0, sat = 1;
	  if (MM && acc > 0xFFFFFFFF)
	    acc &= 0xFFFFFFFF;
	  break;
	case M_IU:
	  if (acc & 0x8000000000000000ull)
	    acc = 0x0, sat = 1;
	  if (acc > 0xFFFFFFFFFFull)
	    acc &= 0xFFFFFFFFFFull, sat = 1;
	  if (MM && acc > 0xFFFFFFFF)
	    acc &= 0xFFFFFFFF;
	  if (acc & 0x80000000)
	    acc |= 0xffffffff00000000ull;
	  break;
	case M_FU:
	  if (!MM && (bs64)acc < 0)
	    acc = 0x0, sat = 1;
	  if (MM && (bs64)acc < -((bs64)1 << 39))
	    acc = -((bu64)1 << 39), sat = 1;
	  if (!MM && (bs64)acc > (bs64)0xFFFFFFFFFFll)
	    acc = 0xFFFFFFFFFFull, sat = 1;
	  if (MM && acc > 0xFFFFFFFFFFull)
	    acc &= 0xFFFFFFFFFFull;
	  if (MM && acc & 0x80000000)
	    acc |= 0xffffffff00000000ull;
	  break;
	case M_IH:
	  if ((bs64)acc < -0x80000000ll)
	    acc = -0x80000000ull, sat = 1;
	  else if ((bs64)acc >= 0x7fffffffll)
	    acc = 0x7fffffffull, sat = 1;
	  break;
	case M_W32:
	  if (sgn0 && (sgn0 != ((acc >> 31) & 1)) && (((acc >> 32) & 0xFF) == 0xff))
	    acc = 0x80000000;
	  acc &= 0xffffffff;
	  if (acc & 0x80000000)
	    acc |= 0xffffffff00000000ull;
	  break;
	default:
	  illegal_instruction(dc);
	}
*/
    }

/*
  STORE (AXREG (which), (acc >> 32) & 0xff);
  STORE (AWREG (which), acc & 0xffffffff);
  STORE (ASTATREG (av[which]), sat);
  if (sat)
    STORE (ASTATREG (avs[which]), sat);
*/

//  return extract_mult (cpu, acc, mmod, MM, fullword, overflow);
  TCGv tmp = tcg_temp_local_new();
  tcg_gen_trunc_i64_i32(tmp, acc);
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

    TRACE_EXTRACT("%s: poprnd:%i prgfunc:%i", __func__, poprnd, prgfunc);

    if (prgfunc == 0 && poprnd == 0)
        /* NOP */;
    else if (prgfunc == 1 && poprnd == 0) {
        /* RTS; */
        dc->is_jmp = DISAS_JUMP;
        dc->hwloop_callback = gen_hwloop_br_direct;
        dc->hwloop_data = &cpu_rets;
    } else if (prgfunc == 1 && poprnd == 1)
        /* RTI; */
        cec_require_supervisor (dc);
    else if (prgfunc == 1 && poprnd == 2)
        /* RTX; */
        cec_require_supervisor (dc);
    else if (prgfunc == 1 && poprnd == 3)
        /* RTN; */
        cec_require_supervisor (dc);
    else if (prgfunc == 1 && poprnd == 4)
        /* RTE; */
        cec_require_supervisor (dc);
    else if (prgfunc == 2 && poprnd == 0)
        /* IDLE; */
        /* just NOP it */;
    else if (prgfunc == 2 && poprnd == 3)
        /* CSYNC; */
        /* just NOP it */;
    else if (prgfunc == 2 && poprnd == 4)
        /* SSYNC; */
        /* just NOP it */;
    else if (prgfunc == 2 && poprnd == 5)
        /* EMUEXCPT; */
        cec_exception(dc, EXCP_DEBUG);
    else if (prgfunc == 3 && poprnd < 8)
        /* CLI Dreg{poprnd}; */
        cec_require_supervisor (dc);
    else if (prgfunc == 4 && poprnd < 8)
        /* STI Dreg{poprnd}; */
        cec_require_supervisor (dc);
    else if (prgfunc == 5 && poprnd < 8) {
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
        cec_require_supervisor (dc);
    } else if (prgfunc == 10) {
        /* EXCPT imm{poprnd}; */
        int excpt = uimm4 (poprnd);
        cec_exception(dc, excpt);
    } else if (prgfunc == 11 && poprnd < 6) {
        /* TESTSET (Preg{poprnd}); */
        TCGv tmp = tcg_temp_new();
        tcg_gen_qemu_ld8u(tmp, cpu_preg[poprnd], dc->mem_idx);
        tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_cc, tmp, 0);
        tcg_gen_ori_tl(tmp, tmp, 0x80);
        tcg_gen_qemu_st8(tmp, cpu_preg[poprnd], dc->mem_idx);
        tcg_temp_free(tmp);
    } else
        illegal_instruction(dc);
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

    TRACE_EXTRACT("%s: a:%i op:%i reg:%i", __func__, a, op, reg);

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

    if (a)
        tcg_gen_addi_tl(cpu_preg[reg], cpu_preg[reg], BFIN_L1_CACHE_BYTES);
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
    TCGv treg, tmp;
    TCGv_i64 tmp64;

    TRACE_EXTRACT("%s: W:%i grp:%i reg:%i", __func__, W, grp, reg);

    /* Can't push/pop reserved registers  */
    /*if (reg_is_reserved(grp, reg))
        illegal_instruction(dc);*/

    reg_check_sup(dc, grp, reg);

    if (W == 0) {
        /* Dreg and Preg are not supported by this instruction */
        /*if (grp == 0 || grp == 1)
            illegal_instruction(dc);*/

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
            tcg_gen_qemu_ld32u(tmp, cpu_spreg, dc->mem_idx);
            tcg_gen_andi_tl(tmp, tmp, 0xff);
            tmp64 = tcg_temp_new_i64();
            tcg_gen_extu_i32_i64(tmp64, tmp);
            tcg_temp_free(tmp);

            tcg_gen_andi_i64(cpu_areg[reg >> 1], cpu_areg[reg >> 1], 0xffffffff);
            tcg_gen_shli_i64(tmp64, tmp64, 32);
            tcg_gen_or_i64(cpu_areg[reg >> 1], cpu_areg[reg >> 1], tmp64);
            tcg_temp_free_i64(tmp64);
        } else if (grp == 4 && (reg == 1 || reg == 3)) {
            /* Pop A#.W */
            tcg_gen_andi_i64(cpu_areg[reg >> 1], cpu_areg[reg >> 1], 0xff00000000);
            tmp = tcg_temp_new();
            tcg_gen_qemu_ld32u(tmp, cpu_spreg, dc->mem_idx);
            tmp64 = tcg_temp_new_i64();
            tcg_gen_extu_i32_i64(tmp64, tmp);
            tcg_temp_free(tmp);
            tcg_gen_or_i64(cpu_areg[reg >> 1], cpu_areg[reg >> 1], tmp64);
            tcg_temp_free_i64(tmp64);
        } else {
            treg = get_allreg(dc, grp, reg);
            tcg_gen_qemu_ld32u(treg, cpu_spreg, dc->mem_idx);

            if (grp == 6 && (reg == 1 || reg == 4))
                /* LT loads auto clear the LSB */
                tcg_gen_andi_tl(treg, treg, ~1);
        }

        tcg_gen_addi_tl(cpu_spreg, cpu_spreg, 4);
        gen_maybe_lb_exit_tb(dc, treg);
    } else {
        /* [--SP] = genreg{grp,reg}; */

        tcg_gen_subi_tl(cpu_spreg, cpu_spreg, 4);
        if (grp == 4 && reg == 6) {
            /* Push ASTAT */
            tmp = tcg_temp_new();
            gen_astat_load(dc, tmp);
            tcg_gen_qemu_st32(tmp, cpu_spreg, dc->mem_idx);
            tcg_temp_free(tmp);
        } else if (grp == 4 && (reg == 0 || reg == 2)) {
            /* Push A#.X */
            tmp64 = tcg_temp_new_i64();
            tcg_gen_shri_i64(tmp64, cpu_areg[reg >> 1], 32);
            tmp = tcg_temp_new();
            tcg_gen_trunc_i64_i32(tmp, tmp64);
            tcg_temp_free_i64(tmp64);
            tcg_gen_andi_tl(tmp, tmp, 0xff);
            tcg_gen_qemu_st32(tmp, cpu_spreg, dc->mem_idx);
            tcg_temp_free(tmp);
        } else if (grp == 4 && (reg == 1 || reg == 3)) {
            /* Push A#.W */
            tmp = tcg_temp_new();
            tcg_gen_trunc_i64_i32(tmp, cpu_areg[reg >> 1]);
            tcg_gen_qemu_st32(tmp, cpu_spreg, dc->mem_idx);
            tcg_temp_free(tmp);
        } else {
            treg = get_allreg(dc, grp, reg);
            tcg_gen_qemu_st32(treg, cpu_spreg, dc->mem_idx);
        }
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

    TRACE_EXTRACT("%s: d:%i p:%i W:%i dr:%i pr:%i", __func__, d, p, W, dr, pr);

    if ((d == 0 && p == 0) || (p && imm5(pr) > 5) ||
        (d && !p && pr) || (p && !d && dr))
        illegal_instruction(dc);

    if (W == 1) {
        /* [--SP] = ({d}R7:imm{dr}, {p}P5:imm{pr}); */
        if (d)
            for (i = dr; i < 8; i++) {
                tcg_gen_subi_tl(cpu_spreg, cpu_spreg, 4);
                tcg_gen_qemu_st32(cpu_dreg[i], cpu_spreg, dc->mem_idx);
            }
        if (p)
            for (i = pr; i < 6; i++) {
                tcg_gen_subi_tl(cpu_spreg, cpu_spreg, 4);
                tcg_gen_qemu_st32(cpu_preg[i], cpu_spreg, dc->mem_idx);
            }
    } else {
        /* ({d}R7:imm{dr}, {p}P5:imm{pr}) = [SP++]; */
        if (p)
            for (i = 5; i >= pr; i--) {
                tcg_gen_qemu_ld32u(cpu_preg[i], cpu_spreg, dc->mem_idx);
                tcg_gen_addi_tl(cpu_spreg, cpu_spreg, 4);
            }
        if (d)
            for (i = 7; i >= dr; i--) {
                tcg_gen_qemu_ld32u(cpu_dreg[i], cpu_spreg, dc->mem_idx);
                tcg_gen_addi_tl(cpu_spreg, cpu_spreg, 4);
            }
    }
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
    int l;
    TCGv reg_src, reg_dst;

    TRACE_EXTRACT("%s: T:%i d:%i s:%i dst:%i src:%i",
                  __func__, T, d, s, dst, src);

    /* IF !{T} CC DPreg{d,dst} = DPreg{s,src}; */
    reg_src = get_allreg(dc, s, src);
    reg_dst = get_allreg(dc, d, dst);
    l = gen_new_label();
    tcg_gen_brcondi_tl(TCG_COND_NE, cpu_cc, T, l);
    tcg_gen_mov_tl(reg_dst, reg_src);
    gen_set_label(l);
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

    TRACE_EXTRACT("%s: I:%i opc:%i G:%i y:%i x:%i",
                  __func__, I, opc, G, y, x);

    if (opc > 4) {
        TCGv_i64 tmp64;
        TCGCond cond;

        /*if (x != 0 || y != 0)
            illegal_instruction(dc);*/

        if (opc == 5 && I == 0 && G == 0)
            /* CC = A0 == A1; */
            cond = TCG_COND_EQ;
        else if (opc == 6 && I == 0 && G == 0)
            /* CC = A0 < A1; */
            cond = TCG_COND_LT;
        else if (opc == 7 && I == 0 && G == 0)
            /* CC = A0 <= A1; */
            cond = TCG_COND_LE;
        else
            illegal_instruction(dc);

        tmp64 = tcg_temp_new_i64();
        tcg_gen_setcond_i64(cond, tmp64, cpu_areg[0], cpu_areg[1]);
        tcg_gen_trunc_i64_i32(cpu_cc, tmp64);
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
        if (issigned)
            astat_op = ASTAT_OP_COMPARE_SIGNED;
        else
            astat_op = ASTAT_OP_COMPARE_UNSIGNED;

        if (I) {
            /* Compare to an immediate rather than a reg */
            tmp = tcg_const_tl(dst_imm);
            dst_reg = tmp;
        }
        tcg_gen_setcond_tl(cond, cpu_cc, src_reg, dst_reg);

        /* Pointer compares only touch CC.  */
        if (!G)
            astat_queue_state2(dc, astat_op, src_reg, dst_reg);

        if (I)
            tcg_temp_free(tmp);
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

    TRACE_EXTRACT("%s: op:%i reg:%i", __func__, op, reg);

    if (op == 0)
        /* Dreg{reg} = CC; */
        tcg_gen_mov_tl(cpu_dreg[reg], cpu_cc);
    else if (op == 1)
        /* CC = Dreg{reg}; */
        tcg_gen_setcondi_tl(TCG_COND_NE, cpu_cc, cpu_dreg[reg], 0);
    else if (op == 3 && reg == 0)
        /* CC = !CC; */
        tcg_gen_xori_tl(cpu_cc, cpu_cc, 1);
    else
        illegal_instruction(dc);
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

    TRACE_EXTRACT("%s: D:%i op:%i cbit:%i", __func__, D, op, cbit);

    /* CC = CC; is invalid.  */
    if (cbit == 5)
        illegal_instruction(dc);

    gen_astat_update(dc, true);

    if (D == 0)
        switch (op) {
        case 0: /* CC = ASTAT[cbit] */
            tcg_gen_ld_tl(cpu_cc, cpu_env, offsetof(CPUState, astat[cbit]));
            break;
        case 1: /* CC |= ASTAT[cbit] */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUState, astat[cbit]));
            tcg_gen_or_tl(cpu_cc, cpu_cc, tmp);
            tcg_temp_free(tmp);
            break;
        case 2: /* CC &= ASTAT[cbit] */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUState, astat[cbit]));
            tcg_gen_and_tl(cpu_cc, cpu_cc, tmp);
            tcg_temp_free(tmp);
            break;
        case 3: /* CC ^= ASTAT[cbit] */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUState, astat[cbit]));
            tcg_gen_xor_tl(cpu_cc, cpu_cc, tmp);
            tcg_temp_free(tmp);
            break;
        }
    else
        switch (op) {
        case 0: /* ASTAT[cbit] = CC */
            tcg_gen_st_tl(cpu_cc, cpu_env, offsetof(CPUState, astat[cbit]));
            break;
        case 1: /* ASTAT[cbit] |= CC */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUState, astat[cbit]));
            tcg_gen_or_tl(tmp, tmp, cpu_cc);
            tcg_gen_st_tl(tmp, cpu_env, offsetof(CPUState, astat[cbit]));
            tcg_temp_free(tmp);
            break;
        case 2: /* ASTAT[cbit] &= CC */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUState, astat[cbit]));
            tcg_gen_and_tl(tmp, tmp, cpu_cc);
            tcg_gen_st_tl(tmp, cpu_env, offsetof(CPUState, astat[cbit]));
            tcg_temp_free(tmp);
            break;
        case 3: /* ASTAT[cbit] ^= CC */
            tmp = tcg_temp_new();
            tcg_gen_ld_tl(tmp, cpu_env, offsetof(CPUState, astat[cbit]));
            tcg_gen_xor_tl(tmp, tmp, cpu_cc);
            tcg_gen_st_tl(tmp, cpu_env, offsetof(CPUState, astat[cbit]));
            tcg_temp_free(tmp);
            break;
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

    TRACE_EXTRACT("%s: T:%i B:%i offset:%#x", __func__, T, B, offset);

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

    TRACE_EXTRACT("%s: offset:%#x", __func__, offset);

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

    TRACE_EXTRACT("%s: gd:%i gs:%i dst:%i src:%i",
                  __func__, gd, gs, dst, src);

    /* genreg{gd,dst} = genreg{gs,src}; */

    reg_check_sup(dc, gs, src);
    reg_check_sup(dc, gd, dst);

#if 0
    /* Reserved slots cannot be a src/dst.  */
    if (reg_is_reserved(gs, src) || reg_is_reserved(gd, dst))
        goto invalid_move;

    /* Standard register moves  */
    if ((gs < 2) ||             /* Dregs/Pregs as source  */
        (gd < 2) ||             /* Dregs/Pregs as dest    */
        (gs == 4 && src < 4) || /* Accumulators as source */
        (gd == 4 && dst < 4 && (gs < 4)) || /* Accumulators as dest   */
        (gs == 7 && src == 7 && !(gd == 4 && dst < 4)) || /* EMUDAT as src */
        (gd == 7 && dst == 7))                            /* EMUDAT as dest */
        goto valid_move;

    /* dareg = dareg (IMBL) */
    if (gs < 4 && gd < 4)
        goto valid_move;

    /* USP can be src to sysregs, but not dagregs.  */
    if ((gs == 7 && src == 0) && (gd >= 4))
        goto valid_move;

    /* USP can move between genregs (only check Accumulators).  */
    if (((gs == 7 && src == 0) && (gd == 4 && dst < 4)) ||
        ((gd == 7 && dst == 0) && (gs == 4 && src < 4)))
        goto valid_move;

    /* Still here ?  Invalid reg pair.  */
 invalid_move:
    illegal_instruction(dc);

 valid_move:
#endif
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
        tcg_gen_trunc_i64_i32(tmp, tmp64);
        tcg_temp_free_i64(tmp64);
        tcg_gen_ext8s_tl(tmp, tmp);
        reg_src = tmp;
        istmp = true;
    } else if (gs == 4 && (src == 1 || src == 3)) {
        /* Reads of A#.W */
        tmp = tcg_temp_new();
        tcg_gen_trunc_i64_i32(tmp, cpu_areg[src >> 1]);
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
        tcg_gen_andi_i64(cpu_areg[dst >> 1], cpu_areg[dst >> 1], 0xffffffff);
        tcg_gen_extu_i32_i64(tmp64, reg_src);
        tcg_gen_andi_i64(tmp64, tmp64, 0xff);
        tcg_gen_shli_i64(tmp64, tmp64, 32);
        tcg_gen_or_i64(cpu_areg[dst >> 1], cpu_areg[dst >> 1], tmp64);
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

    if (istmp)
        tcg_temp_free(tmp);
}

static void
clipi(DisasContext *dc, TCGv *reg, TCGv *tmp, uint32_t limit)
{
    int l = gen_new_label();
    *tmp = tcg_temp_local_new();
    tcg_gen_mov_tl(*tmp, *reg);
    tcg_gen_brcondi_tl(TCG_COND_LEU, *tmp, limit, l);
    tcg_gen_movi_tl(*tmp, limit);
    gen_set_label(l);
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
    int l;
    TCGv tmp;

    TRACE_EXTRACT("%s: opc:%i src:%i dst:%i", __func__, opc, src, dst);

    if (opc == 0) {
        /* Dreg{dst} >>>= Dreg{src}; */
        clipi (dc, &cpu_dreg[src], &tmp, 31);
        tcg_gen_sar_tl(cpu_dreg[dst], cpu_dreg[dst], tmp);
        tcg_temp_free(tmp);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT_RT32, cpu_dreg[dst]);
    } else if (opc == 1) {
        /* Dreg{dst} >>= Dreg{src}; */
/*
        if (DREG (src) <= 0x1F)
            val = lshiftrt (cpu, DREG (dst), DREG (src), 32);
        else
            val = 0;
        SET_DREG (dst, val);
*/
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
//      SET_DREG (dst, lshift (cpu, DREG (dst), DREG (src), 32, 0));
//      clipi (dc, &cpu_dreg[src], &tmp, 31);
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
    } else if (opc == 8)
        /* DIVQ (Dreg, Dreg); */
        gen_divq(cpu_dreg[dst], cpu_dreg[src]);
    else if (opc == 9)
        /* DIVS (Dreg, Dreg); */
        gen_divs(cpu_dreg[dst], cpu_dreg[src]);
    else if (opc == 10) {
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

    TRACE_EXTRACT("%s: opc:%i src:%i dst:%i", __func__, opc, src, dst);

    if (opc == 0)
        /* Preg{dst} -= Preg{src}; */
        tcg_gen_sub_tl(cpu_preg[dst], cpu_preg[dst], cpu_preg[src]);
    else if (opc == 1)
        /* Preg{dst} = Preg{src} << 2; */
        tcg_gen_shli_tl(cpu_preg[dst], cpu_preg[src], 2);
    else if (opc == 3)
        /* Preg{dst} = Preg{src} >> 2; */
        tcg_gen_shri_tl(cpu_preg[dst], cpu_preg[src], 2);
    else if (opc == 4)
        /* Preg{dst} = Preg{src} >> 1; */
        tcg_gen_shri_tl(cpu_preg[dst], cpu_preg[src], 1);
    else if (opc == 5)
        /* Preg{dst} += Preg{src} (BREV); */
        gen_helper_add_brev(cpu_preg[dst], cpu_preg[dst], cpu_preg[src]);
    else /*if (opc == 6 || opc == 7)*/ {
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
    TCGv tmp;

    TRACE_EXTRACT("%s: opc:%i src:%i dst:%i", __func__, opc, src, dst);

    if (opc == 0) {
        /* CC = ! BITTST (Dreg{dst}, imm{uimm}); */
        tmp = tcg_temp_new();
        tcg_gen_movi_tl(tmp, 1 << uimm);
        tcg_gen_and_tl(tmp, tmp, cpu_dreg[dst]);
        tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_cc, tmp, 0);
        tcg_temp_free(tmp);
    } else if (opc == 1) {
        /* CC = BITTST (Dreg{dst}, imm{uimm}); */
        tmp = tcg_temp_new();
        tcg_gen_movi_tl(tmp, 1 << uimm);
        tcg_gen_and_tl(tmp, tmp, cpu_dreg[dst]);
        tcg_gen_setcondi_tl(TCG_COND_NE, cpu_cc, tmp, 0);
        tcg_temp_free(tmp);
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
    } else /*if (opc == 7)*/ {
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

    TRACE_EXTRACT("%s: opc:%i dst:%i src1:%i src0:%i",
                  __func__, opc, dst, src1, src0);

    tmp = tcg_temp_local_new();
    if (opc == 0) {
        /* Dreg{dst} = Dreg{src0} + Dreg{src1}; */
        tcg_gen_add_tl(tmp, cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state3(dc, ASTAT_OP_ADD32, tmp, cpu_dreg[src0], cpu_dreg[src1]);
        tcg_gen_mov_tl(cpu_dreg[dst], tmp);
    } else if (opc == 1) {
        /* Dreg{dst} = Dreg{src0} - Dreg{src1}; */
        tcg_gen_sub_tl(tmp, cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state3(dc, ASTAT_OP_SUB32, tmp, cpu_dreg[src0], cpu_dreg[src1]);
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
    } else /*if (opc == 6 || opc == 7)*/ {
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

    TRACE_EXTRACT("%s: op:%i src:%i dst:%i", __func__, op, src, dst);

    if (op == 0) {
        /* Dreg{dst} = imm{src} (X); */
        tcg_gen_movi_tl(cpu_dreg[dst], imm);
    } else {
        /* Dreg{dst} += imm{src}; */
        tmp = tcg_const_tl(imm);
        tcg_gen_mov_tl(cpu_astat_arg[1], cpu_dreg[dst]);
        tcg_gen_add_tl(cpu_dreg[dst], cpu_astat_arg[1], tmp);
        astat_queue_state3(dc, ASTAT_OP_ADD32, cpu_dreg[dst], cpu_astat_arg[1], tmp);
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

    TRACE_EXTRACT("%s: op:%i src:%i dst:%i", __func__, op, src, dst);

    if (op == 0)
        /* Preg{dst} = imm{src}; */
        tcg_gen_movi_tl(cpu_preg[dst], imm);
    else
        /* Preg{dst} += imm{src}; */
        tcg_gen_addi_tl(cpu_preg[dst], cpu_preg[dst], imm);
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

    TRACE_EXTRACT("%s: W:%i aop:%i reg:%i idx:%i ptr:%i",
                  __func__, W, aop, reg, idx, ptr);

    if (aop == 1 && W == 0 && idx == ptr) {
        /* Dreg_lo{reg} = W[Preg{ptr}]; */
        tmp = tcg_temp_new();
        tcg_gen_andi_tl(cpu_dreg[reg], cpu_dreg[reg], 0xffff0000);
        tcg_gen_qemu_ld16u(tmp, cpu_preg[ptr], dc->mem_idx);
        tcg_gen_or_tl(cpu_dreg[reg], cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
    } else if (aop == 2 && W == 0 && idx == ptr) {
        /* Dreg_hi{reg} = W[Preg{ptr}]; */
        tmp = tcg_temp_new();
        tcg_gen_andi_tl(cpu_dreg[reg], cpu_dreg[reg], 0xffff);
        tcg_gen_qemu_ld16u(tmp, cpu_preg[ptr], dc->mem_idx);
        tcg_gen_shli_tl(tmp, tmp, 16);
        tcg_gen_or_tl(cpu_dreg[reg], cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
    } else if (aop == 1 && W == 1 && idx == ptr) {
        /* W[Preg{ptr}] = Dreg_lo{reg}; */
        tcg_gen_qemu_st16(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
    } else if (aop == 2 && W == 1 && idx == ptr) {
        /* W[Preg{ptr}] = Dreg_hi{reg}; */
        tmp = tcg_temp_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        tcg_gen_qemu_st16(tmp, cpu_preg[ptr], dc->mem_idx);
        tcg_temp_free(tmp);
    } else if (aop == 0 && W == 0) {
        /* Dreg{reg} = [Preg{ptr} ++ Preg{idx}]; */
        tcg_gen_qemu_ld32u(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        if (ptr != idx)
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
    } else if (aop == 1 && W == 0) {
        /* Dreg_lo{reg} = W[Preg{ptr} ++ Preg{idx}]; */
        tmp = tcg_temp_new();
        tcg_gen_andi_tl(cpu_dreg[reg], cpu_dreg[reg], 0xffff0000);
        tcg_gen_qemu_ld16u(tmp, cpu_preg[ptr], dc->mem_idx);
        tcg_gen_or_tl(cpu_dreg[reg], cpu_dreg[reg], tmp);
        if (ptr != idx)
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        tcg_temp_free(tmp);
    } else if (aop == 2 && W == 0) {
        /* Dreg_hi{reg} = W[Preg{ptr} ++ Preg{idx}]; */
        tmp = tcg_temp_new();
        tcg_gen_andi_tl(cpu_dreg[reg], cpu_dreg[reg], 0xffff);
        tcg_gen_qemu_ld16u(tmp, cpu_preg[ptr], dc->mem_idx);
        tcg_gen_shli_tl(tmp, tmp, 16);
        tcg_gen_or_tl(cpu_dreg[reg], cpu_dreg[reg], tmp);
        if (ptr != idx)
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        tcg_temp_free(tmp);
    } else if (aop == 3 && W == 0) {
        /* R%i = W[Preg{ptr} ++ Preg{idx}] (Z); */
        tcg_gen_qemu_ld16u(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        if (ptr != idx)
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
    } else if (aop == 3 && W == 1) {
        /* R%i = W[Preg{ptr} ++ Preg{idx}] (X); */
        tcg_gen_qemu_ld16s(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        if (ptr != idx)
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
    } else if (aop == 0 && W == 1) {
        /* [Preg{ptr} ++ Preg{idx}] = R%i; */
        tcg_gen_qemu_st32(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        if (ptr != idx)
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
    } else if (aop == 1 && W == 1) {
        /* W[Preg{ptr} ++ Preg{idx}] = Dreg_lo{reg}; */
        tcg_gen_qemu_st16(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        if (ptr != idx)
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
    } else if (aop == 2 && W == 1) {
        /* W[Preg{ptr} ++ Preg{idx}] = Dreg_hi{reg}; */
        tmp = tcg_temp_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        tcg_gen_qemu_st16(tmp, cpu_preg[ptr], dc->mem_idx);
        if (ptr != idx)
            tcg_gen_add_tl(cpu_preg[ptr], cpu_preg[ptr], cpu_preg[idx]);
        tcg_temp_free(tmp);
    } else
        illegal_instruction(dc);
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

    TRACE_EXTRACT("%s: br:%i op:%i m:%i i:%i", __func__, br, op, m, i);

    if (op == 0 && br == 1)
        /* Ireg{i} += Mreg{m} (BREV); */
        gen_helper_add_brev(cpu_ireg[i], cpu_ireg[i], cpu_mreg[m]);
    else if (op == 0)
        /* Ireg{i} += Mreg{m}; */
        gen_dagadd (dc, i, cpu_mreg[m]);
    else if (op == 1 && br == 0)
        /* Ireg{i} -= Mreg{m}; */
        gen_dagsub (dc, i, cpu_mreg[m]);
    else
        illegal_instruction(dc);
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

    TRACE_EXTRACT("%s: op:%i i:%i", __func__, op, i);

    if (op & 1)
        /* Ireg{i} -= 2 or 4; */
        gen_dagsubi (dc, i, mod);
    else
        /* Ireg{i} += 2 or 4; */
        gen_dagaddi (dc, i, mod);
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

    TRACE_EXTRACT("%s: aop:%i m:%i i:%i reg:%i", __func__, aop, m, i, reg);

    if (aop == 0 && W == 0 && m == 0) {
        /* Dreg{reg} = [Ireg{i}++]; */
        /* XXX: No DISALGNEXCPT support */
        tcg_gen_qemu_ld32u(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
        gen_dagaddi (dc, i, 4);
    } else if (aop == 0 && W == 0 && m == 1) {
        /* Dreg_lo{reg} = W[Ireg{i}++]; */
        tmp = tcg_temp_new();
        tcg_gen_qemu_ld16u(tmp, cpu_ireg[i], dc->mem_idx);
        gen_mov_l_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
        gen_dagaddi (dc, i, 2);
    } else if (aop == 0 && W == 0 && m == 2) {
        /* Dreg_hi{reg} = W[Ireg{i}++]; */
        tmp = tcg_temp_new();
        tcg_gen_qemu_ld16u(tmp, cpu_ireg[i], dc->mem_idx);
        gen_mov_h_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
        gen_dagaddi (dc, i, 2);
    } else if (aop == 1 && W == 0 && m == 0) {
        /* Dreg{reg} = [Ireg{i}--]; */
        /* XXX: No DISALGNEXCPT support */
        tcg_gen_qemu_ld32u(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
        gen_dagsubi (dc, i, 4);
    } else if (aop == 1 && W == 0 && m == 1) {
        /* Dreg_lo{reg} = W[Ireg{i}--]; */
        tmp = tcg_temp_new();
        tcg_gen_qemu_ld16u(tmp, cpu_ireg[i], dc->mem_idx);
        gen_mov_l_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
        gen_dagsubi (dc, i, 2);
    } else if (aop == 1 && W == 0 && m == 2) {
        /* Dreg_hi{reg} = W[Ireg{i}--]; */
        tmp = tcg_temp_new();
        tcg_gen_qemu_ld16u(tmp, cpu_ireg[i], dc->mem_idx);
        gen_mov_h_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
        gen_dagsubi (dc, i, 2);
    } else if (aop == 2 && W == 0 && m == 0) {
        /* Dreg{reg} = [Ireg{i}]; */
        /* XXX: No DISALGNEXCPT support */
        tcg_gen_qemu_ld32u(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
    } else if (aop == 2 && W == 0 && m == 1) {
        /* Dreg_lo{reg} = W[Ireg{i}]; */
        tmp = tcg_temp_new();
        tcg_gen_qemu_ld16u(tmp, cpu_ireg[i], dc->mem_idx);
        gen_mov_l_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
    } else if (aop == 2 && W == 0 && m == 2) {
        /* Dreg_hi{reg} = W[Ireg{i}]; */
        tmp = tcg_temp_new();
        tcg_gen_qemu_ld16u(tmp, cpu_ireg[i], dc->mem_idx);
        gen_mov_h_tl(cpu_dreg[reg], tmp);
        tcg_temp_free(tmp);
    } else if (aop == 0 && W == 1 && m == 0) {
        /* [Ireg{i}++] = Dreg{reg}; */
        tcg_gen_qemu_st32(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
        gen_dagaddi (dc, i, 4);
    } else if (aop == 0 && W == 1 && m == 1) {
        /* W[Ireg{i}++] = Dreg_lo{reg}; */
        tcg_gen_qemu_st16(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
        gen_dagaddi (dc, i, 2);
    } else if (aop == 0 && W == 1 && m == 2) {
        /* W[Ireg{i}++] = Dreg_hi{reg}; */
        tmp = tcg_temp_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        tcg_gen_qemu_st16(tmp, cpu_ireg[i], dc->mem_idx);
        tcg_temp_free(tmp);
        gen_dagaddi (dc, i, 2);
    } else if (aop == 1 && W == 1 && m == 0) {
        /* [Ireg{i}--] = Dreg{reg}; */
        tcg_gen_qemu_st32(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
        gen_dagsubi (dc, i, 4);
    } else if (aop == 1 && W == 1 && m == 1) {
        /* W[Ireg{i}--] = Dreg_lo{reg}; */
        tcg_gen_qemu_st16(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
        gen_dagsubi (dc, i, 2);
    } else if (aop == 1 && W == 1 && m == 2) {
        /* W[Ireg{i}--] = Dreg_hi{reg}; */
        tmp = tcg_temp_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        tcg_gen_qemu_st16(tmp, cpu_ireg[i], dc->mem_idx);
        tcg_temp_free(tmp);
        gen_dagsubi (dc, i, 2);
    } else if (aop == 2 && W == 1 && m == 0) {
        /* [Ireg{i}] = Dreg{reg}; */
        tcg_gen_qemu_st32(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
    } else if (aop == 2 && W == 1 && m == 1) {
        /* W[Ireg{i}] = Dreg_lo{reg}; */
        tcg_gen_qemu_st16(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
    } else if (aop == 2 && W == 1 && m == 2) {
        /* W[Ireg{i}] = Dreg_hi{reg}; */
        tmp = tcg_temp_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[reg], 16);
        tcg_gen_qemu_st16(tmp, cpu_ireg[i], dc->mem_idx);
        tcg_temp_free(tmp);
    } else if (aop == 3 && W == 0) {
        /* Dreg{reg} = [Ireg{i} ++ Mreg{m}]; */
        /* XXX: No DISALGNEXCPT support */
        tcg_gen_qemu_ld32u(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
        gen_dagadd (dc, i, cpu_mreg[m]);
    } else if (aop == 3 && W == 1) {
        /* [Ireg{i} ++ Mreg{m}] = Dreg{reg}; */
        tcg_gen_qemu_st32(cpu_dreg[reg], cpu_ireg[i], dc->mem_idx);
        gen_dagadd (dc, i, cpu_mreg[m]);
    } else
        illegal_instruction(dc);
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

    TRACE_EXTRACT("%s: sz:%i W:%i aop:%i Z:%i ptr:%i reg:%i",
                  __func__, sz, W, aop, Z, ptr, reg);

    if (aop == 3)
        illegal_instruction(dc);

    if (W == 0) {
        if (sz == 0 && Z == 0)
            /* Dreg{reg} = [Preg{ptr}{aop}]; */
            tcg_gen_qemu_ld32u(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        else if (sz == 0 && Z == 1) {
            /* Preg{reg} = [Preg{ptr}{aop}]; */
            /*if (aop < 2 && ptr == reg)
                illegal_instruction_combination(dc);*/
            tcg_gen_qemu_ld32u(cpu_preg[reg], cpu_preg[ptr], dc->mem_idx);
        } else if (sz == 1 && Z == 0)
            /* Dreg{reg} = W[Preg{ptr}{aop}] (Z); */
            tcg_gen_qemu_ld16u(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        else if (sz == 1 && Z == 1)
            /* Dreg{reg} = W[Preg{ptr}{aop}] (X); */
            tcg_gen_qemu_ld16s(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        else if (sz == 2 && Z == 0)
            /* Dreg{reg} = B[Preg{ptr}{aop}] (Z); */
            tcg_gen_qemu_ld8u(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        else if (sz == 2 && Z == 1)
            /* Dreg{reg} = B[Preg{ptr}{aop}] (X); */
            tcg_gen_qemu_ld8s(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        else
            illegal_instruction(dc);
    } else {
        if (sz == 0 && Z == 0)
            /* [Preg{ptr}{aop}] = Dreg{reg}; */
            tcg_gen_qemu_st32(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        else if (sz == 0 && Z == 1)
            /* [Preg{ptr}{aop}] = Preg{reg}; */
            tcg_gen_qemu_st32(cpu_preg[reg], cpu_preg[ptr], dc->mem_idx);
        else if (sz == 1 && Z == 0)
            /* W[Preg{ptr}{aop}] = Dreg{reg}; */
            tcg_gen_qemu_st16(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        else if (sz == 2 && Z == 0)
            /* B[Preg{ptr}{aop}] = Dreg{reg}; */
            tcg_gen_qemu_st8(cpu_dreg[reg], cpu_preg[ptr], dc->mem_idx);
        else
            illegal_instruction(dc);
    }

    if (aop == 0)
        tcg_gen_addi_tl(cpu_preg[ptr], cpu_preg[ptr], 1 << (2 - sz));
    if (aop == 1)
        tcg_gen_subi_tl(cpu_preg[ptr], cpu_preg[ptr], 1 << (2 - sz));
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

    TRACE_EXTRACT("%s: W:%i offset:%#x grp:%i reg:%i",
                  __func__, W, offset, grp, reg);

    ea = tcg_temp_new();
    tcg_gen_addi_tl(ea, cpu_fpreg, imm);
    if (W == 0)
        /* DPreg{reg} = [FP + imm{offset}]; */
        tcg_gen_qemu_ld32u(treg, ea, dc->mem_idx);
    else
        /* [FP + imm{offset}] = DPreg{reg}; */
        tcg_gen_qemu_st32(treg, ea, dc->mem_idx);
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

    TRACE_EXTRACT("%s: W:%i op:%i offset:%#x ptr:%i reg:%i",
                  __func__, W, op, offset, ptr, reg);

    if (op == 0 || op == 3)
        imm = uimm4s4(offset);
    else
        imm = uimm4s2(offset);

    ea = tcg_temp_new();
    tcg_gen_addi_tl(ea, cpu_preg[ptr], imm);
    if (W == 0) {
        if (op == 0)
            /* Dreg{reg} = [Preg{ptr} + imm{offset}]; */
            tcg_gen_qemu_ld32u(cpu_dreg[reg], ea, dc->mem_idx);
        else if (op == 1)
            /* Dreg{reg} = W[Preg{ptr} + imm{offset}] (Z); */
            tcg_gen_qemu_ld16u(cpu_dreg[reg], ea, dc->mem_idx);
        else if (op == 2)
            /* Dreg{reg} = W[Preg{ptr} + imm{offset}] (X); */
            tcg_gen_qemu_ld16s(cpu_dreg[reg], ea, dc->mem_idx);
        else if (op == 3)
            /* P%i = [Preg{ptr} + imm{offset}]; */
            tcg_gen_qemu_ld32u(cpu_preg[reg], ea, dc->mem_idx);
    } else {
        if (op == 0)
            /* [Preg{ptr} + imm{offset}] = Dreg{reg}; */
            tcg_gen_qemu_st32(cpu_dreg[reg], ea, dc->mem_idx);
        else if (op == 1)
            /* W[Preg{ptr} + imm{offset}] = Dreg{reg}; */
            tcg_gen_qemu_st16(cpu_dreg[reg], ea, dc->mem_idx);
        else if (op == 3)
            /* [Preg{ptr} + imm{offset}] = P%i; */
            tcg_gen_qemu_st32(cpu_preg[reg], ea, dc->mem_idx);
        else
            illegal_instruction(dc);
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
    int soffset = ((iw0 >> (LoopSetup_soffset_bits - 16)) & LoopSetup_soffset_mask);
    int eoffset = ((iw1 >> LoopSetup_eoffset_bits) & LoopSetup_eoffset_mask);
    int spcrel = pcrel4(soffset);
    int epcrel = lppcrel10(eoffset);

    TRACE_EXTRACT("%s: rop:%i c:%i soffset:%i reg:%i eoffset:%i",
                  __func__, rop, c, soffset, reg, eoffset);

    if (rop == 0)
        /* LSETUP (imm{soffset}, imm{eoffset}) LCreg{c}; */;
    else if (rop == 1 && reg <= 7)
        /* LSETUP (imm{soffset}, imm{eoffset}) LCreg{c} = Preg{reg}; */
        tcg_gen_mov_tl(cpu_lcreg[c], cpu_preg[reg]);
    else if (rop == 3 && reg <= 7)
        /* LSETUP (imm{soffset}, imm{eoffset}) LCreg{c} = Preg{reg} >> 1; */
        tcg_gen_shri_tl(cpu_lcreg[c], cpu_preg[reg], 1);
    else
        illegal_instruction(dc);

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
    TCGv treg;

    TRACE_EXTRACT("%s: Z:%i H:%i S:%i grp:%i reg:%i hword:%#x",
                  __func__, Z, H, S, grp, reg, hword);

    treg = get_allreg(dc, grp, reg);
    if (S == 1)
        val = imm16(hword);
    else
        val = luimm16(hword);

    if (H == 0 && S == 1 && Z == 0)
        /* genreg{grp,reg} = imm{hword} (X); */
        /* Take care of immediate sign extension ourselves */
        tcg_gen_movi_i32(treg, (int16_t)val);
    else if (H == 0 && S == 0 && Z == 1)
        /* genreg{grp,reg} = imm{hword} (Z); */
        tcg_gen_movi_i32(treg, val);
    else if (H == 0 && S == 0 && Z == 0) {
        /* genreg_lo{grp,reg} = imm{hword}; */
        /* XXX: Convert this to a helper.  */
        tcg_gen_andi_tl(treg, treg, 0xffff0000);
        tcg_gen_ori_tl(treg, treg, val);
    } else if (H == 1 && S == 0 && Z == 0) {
        /* genreg_hi{grp,reg} = imm{hword}; */
        /* XXX: Convert this to a helper.  */
        tcg_gen_andi_tl(treg, treg, 0xffff);
        tcg_gen_ori_tl(treg, treg, val << 16);
    } else
        illegal_instruction(dc);
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

    TRACE_EXTRACT("%s: S:%i msw:%#x lsw:%#x", __func__, S, msw, lsw);

    if (S == 1)
        /* CALL imm{pcrel}; */
        dc->is_jmp = DISAS_CALL;
    else
        /* JUMP.L imm{pcrel}; */
        dc->is_jmp = DISAS_JUMP;
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

    TRACE_EXTRACT("%s: W:%i Z:%i sz:%i ptr:%i reg:%i offset:%#x",
                  __func__, W, Z, sz, ptr, reg, offset);

    ea = tcg_temp_new();
    if (sz == 0)
        tcg_gen_addi_tl(ea, cpu_preg[ptr], imm_16s4);
    else if (sz == 1)
        tcg_gen_addi_tl(ea, cpu_preg[ptr], imm_16s2);
    else if (sz == 2)
        tcg_gen_addi_tl(ea, cpu_preg[ptr], imm_16);
    else
        illegal_instruction(dc);

    if (W == 0) {
        if (sz == 0 && Z == 0)
            /* Dreg{reg} = [Preg{ptr] + imm{offset}]; */
            tcg_gen_qemu_ld32u(cpu_dreg[reg], ea, dc->mem_idx);
        else if (sz == 0 && Z == 1)
            /* Preg{reg} = [Preg{ptr] + imm{offset}]; */
            tcg_gen_qemu_ld32u(cpu_preg[reg], ea, dc->mem_idx);
        else if (sz == 1 && Z == 0)
            /* Dreg{reg} = W[Preg{ptr] + imm{offset}] (Z); */
            tcg_gen_qemu_ld16u(cpu_dreg[reg], ea, dc->mem_idx);
        else if (sz == 1 && Z == 1)
            /* Dreg{reg} = W[Preg{ptr} imm{offset}] (X); */
            tcg_gen_qemu_ld16s(cpu_dreg[reg], ea, dc->mem_idx);
        else if (sz == 2 && Z == 0)
            /* Dreg{reg} = B[Preg{ptr} + imm{offset}] (Z); */
            tcg_gen_qemu_ld8u(cpu_dreg[reg], ea, dc->mem_idx);
        else if (sz == 2 && Z == 1)
            /* Dreg{reg} = B[Preg{ptr} + imm{offset}] (X); */
            tcg_gen_qemu_ld8s(cpu_dreg[reg], ea, dc->mem_idx);
    } else {
        if (sz == 0 && Z == 0)
            /* [Preg{ptr} + imm{offset}] = Dreg{reg}; */
            tcg_gen_qemu_st32(cpu_dreg[reg], ea, dc->mem_idx);
        else if (sz == 0 && Z == 1)
            /* [Preg{ptr} + imm{offset}] = Preg{reg}; */
            tcg_gen_qemu_st32(cpu_preg[reg], ea, dc->mem_idx);
        else if (sz == 1 && Z == 0)
            /* W[Preg{ptr} + imm{offset}] = Dreg{reg}; */
            tcg_gen_qemu_st16(cpu_dreg[reg], ea, dc->mem_idx);
        else if (sz == 2 && Z == 0)
            /* B[Preg{ptr} + imm{offset}] = Dreg{reg}; */
            tcg_gen_qemu_st8(cpu_dreg[reg], ea, dc->mem_idx);
        else
            illegal_instruction(dc);
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

    TRACE_EXTRACT("%s: R:%i framesize:%#x", __func__, R, framesize);

    if (R == 0) {
        /* LINK imm{framesize}; */
        int size = uimm16s4(framesize);
        tcg_gen_subi_tl(cpu_spreg, cpu_spreg, 4);
        tcg_gen_qemu_st32(cpu_rets, cpu_spreg, dc->mem_idx);
        tcg_gen_subi_tl(cpu_spreg, cpu_spreg, 4);
        tcg_gen_qemu_st32(cpu_fpreg, cpu_spreg, dc->mem_idx);
        tcg_gen_mov_tl(cpu_fpreg, cpu_spreg);
        tcg_gen_subi_tl(cpu_spreg, cpu_spreg, size);
    } else if (framesize == 0) {
        /* UNLINK; */
        /* Restore SP from FP.  */
        tcg_gen_mov_tl(cpu_spreg, cpu_fpreg);
        tcg_gen_qemu_ld32u(cpu_fpreg, cpu_spreg, dc->mem_idx);
        tcg_gen_addi_tl(cpu_spreg, cpu_spreg, 4);
        tcg_gen_qemu_ld32u(cpu_rets, cpu_spreg, dc->mem_idx);
        tcg_gen_addi_tl(cpu_spreg, cpu_spreg, 4);
    } else
        illegal_instruction(dc);
}

static void
decode_dsp32mac_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
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

//  bu32 res = DREG (dst);
  int v_i = 0; //, zero = 0;
  TCGv res;

  TRACE_EXTRACT("%s: M:%i mmod:%i MM:%i P:%i w1:%i op1:%i h01:%i h11:%i "
		      "w0:%i op0:%i h00:%i h10:%i dst:%i src0:%i src1:%i",
		 __func__, M, mmod, MM, P, w1, op1, h01, h11, w0, op0, h00, h10,
		 dst, src0, src1);

  /*
  if (w0 == 0 && w1 == 0 && op1 == 3 && op0 == 3)
    illegal_instruction(dc);

  if (op1 == 3 && MM)
    illegal_instruction(dc);

  if ((w1 || w0) && mmod == M_W32)
    illegal_instruction(dc);

  if (((1 << mmod) & (P ? 0x131b : 0x1b5f)) == 0)
    illegal_instruction(dc);
  */

  /* XXX: Missing TRACE_INSN - this is as good as it gets for now  */
  if (w0 && w1 && P)
    TRACE_INSN (cpu, "R%i = macfunc, R%i = macfunc", dst + 1, dst);
  else if (w0 && P)
    TRACE_INSN (cpu, "R%i = macfunc", dst);
  else if (w1 && P)
    TRACE_INSN (cpu, "R%i = macfunc", dst + 1);
  else if (w0 && !P)
    TRACE_INSN (cpu, "R%i.L = macfunc", dst);
  else if (w1 && !P)
    TRACE_INSN (cpu, "R%i.H = macfunc", dst);
  else if (!w0 && !w1 && (op1 != 3 && op0 != 3))
    TRACE_INSN (cpu, "A0 = macfunc, A1 = macfunc");
  else if (!w0 && w1 && (op1 != 3 && op0 != 3))
    TRACE_INSN (cpu, "A1 = macfunc");
  else if (w0 && !w1 && (op1 != 3 && op0 != 3))
    TRACE_INSN (cpu, "A0 = macfunc");

  res = tcg_temp_local_new();
  tcg_gen_mov_tl(res, cpu_dreg[dst]);
  if (w1 == 1 || op1 != 3)
    {
      TCGv res1 = decode_macfunc (dc, 1, op1, h01, h11, src0, src1, mmod, MM, P, &v_i);
//      if (op1 == 3)
//	zero = !!(res1 == 0);
      if (w1)
	{
	  if (P)
//	    STORE (DREG (dst + 1), res1);
	    tcg_gen_mov_tl(cpu_dreg[dst + 1], res1);
	  else
	    {
//	      if (res1 & 0xffff0000)
//		illegal_instruction(dc);
//	      res = REG_H_L (res1 << 16, res);
	      gen_mov_h_tl(res, res1);
	    }
	}
      tcg_temp_free(res1);
//unhandled_instruction(dc, "dsp32mac 1");
    }
  if (w0 == 1 || op0 != 3)
    {
      TCGv res0 = decode_macfunc (dc, 0, op0, h00, h10, src0, src1, mmod, 0, P, &v_i);
//      if (op1 == 3)
//	zero |= !!(res0 == 0);
      if (w0)
	{
	  if (P)
//	    STORE (DREG (dst), res0);
	    tcg_gen_mov_tl(cpu_dreg[dst], res0);
	  else
	    {
//	      if (res0 & 0xffff0000)
//		illegal_instruction(dc);
//	      res = REG_H_L (res, res0);
	      gen_mov_l_tl(res, res0);
	    }
	}
      tcg_temp_free(res0);
//unhandled_instruction(dc, "dsp32mac 2");
    }

  if (!P && (w0 || w1))
    {
      tcg_gen_mov_tl(cpu_dreg[dst], res);
/*
      STORE (DREG (dst), res);
      SET_ASTATREG (v, v_i);
      if (v_i)
	SET_ASTATREG (vs, v_i);
*/
    }
  else if (P)
    {
/*
      SET_ASTATREG (v, v_i);
      if (v_i)
	SET_ASTATREG (vs, v_i);
*/
    }
  if (op0 == 3 || op1 == 3)
{}
//    SET_ASTATREG (az, zero);

  tcg_temp_free(res);
}

static void
decode_dsp32mult_0(DisasContext *dc, uint16_t iw0, uint16_t iw1)
{
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

//  bu32 res = DREG (dst);
    TCGv res;
    int sat0 = 0, sat1 = 0;

    TRACE_EXTRACT("%s: M:%i mmod:%i MM:%i P:%i w1:%i op1:%i h01:%i h11:%i "
                  "w0:%i op0:%i h00:%i h10:%i dst:%i src0:%i src1:%i",
                  __func__, M, mmod, MM, P, w1, op1, h01, h11, w0, op0, h00, h10,
                  dst, src0, src1);

    if (w1 == 0 && w0 == 0)
        illegal_instruction(dc);
    if (((1 << mmod) & (P ? 0x313 : 0x1b57)) == 0)
        illegal_instruction(dc);
    if (P && ((dst & 1) || (op1 != 0) || (op0 != 0) || !is_macmod_pmove (mmod)))
        illegal_instruction(dc);
    if (!P && ((op1 != 0) || (op0 != 0) || !is_macmod_hmove (mmod)))
        illegal_instruction(dc);

  if (w0 && w1 && P)
    TRACE_INSN (cpu, "R%i:%i = dsp32mult", dst + 1, dst);
  else if (w0 && w1 && !P)
    TRACE_INSN (cpu, "R%i.L = R%i.%s * R%i.%s, R%i.H = R%i.%s * R%i.%s;",
		dst, src0, h01 ? "L" : "H" , src1, h11 ? "L" : "H",
		dst, src0, h00 ? "L" : "H" , src1, h10 ? "L" : "H");
  else if (w0 && P)
    TRACE_INSN (cpu, "R%i = R%i.%s * R%i.%s;",
		dst, src0, h00 ? "L" : "H" , src1, h10 ? "L" : "H");
  else if (w1 && P)
    TRACE_INSN (cpu, "R%i = R%i.%s * R%i.%s;",
		dst + 1, src0, h01 ? "L" : "H" , src1, h11 ? "L" : "H");
  else if (w0 && !P)
    TRACE_INSN (cpu, "R%i.L = R%i.%s * R%i.%s;",
		dst, src0, h00 ? "L" : "H" , src1, h10 ? "L" : "H");
  else if (w1 && !P)
    TRACE_INSN (cpu, "R%i.H = R%i.%s * R%i.%s;",
		dst, src0, h01 ? "L" : "H" , src1, h11 ? "L" : "H");

  res = tcg_temp_local_new();
  tcg_gen_mov_tl(res, cpu_dreg[dst]);
  if (w1)
    {
      TCGv r = decode_multfunc_tl(dc, h01, h11, src0, src1, mmod, MM, &sat1);
#define res1 r
//      bu32 res1 = extract_mult (dc, r, mmod, MM, P, NULL);
      if (P)
	//STORE (DREG (dst + 1), res1);
	tcg_gen_mov_tl(cpu_dreg[dst + 1], res1);
      else
	{
//	  if (res1 & 0xFFFF0000)
//	    illegal_instruction(dc);
//	  res = REG_H_L (res1 << 16, res);
	  gen_mov_h_tl(res, res1);
	}
      tcg_temp_free(r);
//unhandled_instruction(dc,  "dsp32mult 1");
#undef res1
    }

  if (w0)
    {
      TCGv r = decode_multfunc_tl(dc, h00, h10, src0, src1, mmod, 0, &sat0);
#define res0 r
//      bu32 res0 = extract_mult (dc, r, mmod, 0, P, NULL);
      if (P)
	//STORE (DREG (dst), res0);
	tcg_gen_mov_tl(cpu_dreg[dst], res0);
      else
	{
//	  if (res0 & 0xFFFF0000)
//	    illegal_instruction(dc);
//	  res = REG_H_L (res, res0);
	  gen_mov_l_tl(res, res0);
	}
      tcg_temp_free(r);
//unhandled_instruction(dc,  "dsp32mult 2");
#undef res0
    }

  if (!P && (w0 || w1))
//    STORE (DREG (dst), res);
    tcg_gen_mov_tl(cpu_dreg[dst], res);

  if (w0 || w1)
    {
/*
      STORE (ASTATREG (v), sat0 | sat1);
      STORE (ASTATREG (v_copy), sat0 | sat1);
      if (sat0 | sat1)
	STORE (ASTATREG (vs), 1);
*/
    }

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

    TRACE_EXTRACT("%s: M:%i HL:%i aopcde:%i aop:%i s:%i x:%i dst0:%i "
                  "dst1:%i src0:%i src1:%i",
                  __func__, M, HL, aopcde, aop, s, x, dst0, dst1, src0, src1);

    if ((aop == 0 || aop == 2) && aopcde == 9 && HL == 0 && s == 0) {
        int a = aop >> 1;
        /* Areg_lo{a} = Dreg_lo{src0}; */
//      SET_AWREG (a, REG_H_L (AWREG (a), DREG (src0)));
        tcg_gen_andi_i64(cpu_areg[a], cpu_areg[a], ~0xffff);
        tmp64 = tcg_temp_new_i64();
        tcg_gen_extu_i32_i64(tmp64, cpu_dreg[src0]);
        tcg_gen_andi_i64(tmp64, tmp64, 0xffff);
        tcg_gen_or_i64(cpu_areg[a], cpu_areg[a], tmp64);
        tcg_temp_free_i64(tmp64);
    } else if ((aop == 0 || aop == 2) && aopcde == 9 && HL == 1 && s == 0) {
        int a = aop >> 1;
        /* Areg_hi{a} = Dreg_hi{src0}; */
//      SET_AWREG (a, REG_H_L (DREG (src0), AWREG (a)));
        tcg_gen_andi_i64(cpu_areg[a], cpu_areg[a], ~0xffff0000);
        tmp64 = tcg_temp_new_i64();
        tcg_gen_extu_i32_i64(tmp64, cpu_dreg[src0]);
        tcg_gen_andi_i64(tmp64, tmp64, 0xffff0000);
        tcg_gen_or_i64(cpu_areg[a], cpu_areg[a], tmp64);
        tcg_temp_free_i64(tmp64);
    }
#if 0
  else if ((aop == 1 || aop == 0) && aopcde == 5)
    {
      bs32 val0 = DREG (src0);
      bs32 val1 = DREG (src1);
      bs32 res;
      bs32 signRes;
      bs32 ovX, sBit1, sBit2, sBitRes1, sBitRes2;

      TRACE_INSN (cpu, "R%i.%s = R%i %s R%i (RND12)", dst0, HL ? "L" : "H",
		  src0, aop & 0x1 ? "-" : "+", src1);

      /* If subtract, just invert and add one */
      if (aop & 0x1)
	val1= ~val1 + 1;

      /* Get the sign bits, since we need them later */
      sBit1 = !!(val0 & 0x80000000);
      sBit2 = !!(val1 & 0x80000000);

      res = val0 + val1;

      sBitRes1 = !!(res & 0x80000000);
      /* Round to the 12th bit */
      res += 0x0800;
      sBitRes2 = !!(res & 0x80000000);

      signRes = res;
      signRes >>= 27;

      /* Overflow if
       * pos + pos = neg
       * neg + neg = pos
       * positive_res + positive_round = neg
       * shift and upper 4 bits where not the same
       */
      if ((!(sBit1 ^ sBit2) && (sBit1 ^ sBitRes1)) ||
	  (!sBit1 && !sBit2 && sBitRes2) ||
	  ((signRes != 0) && (signRes != -1)))
	{
	  /* Both X1 and X2 Neg res is neg overflow */
	  if (sBit1 && sBit2)
	    res = 0x80000000;
	  /* Both X1 and X2 Pos res is pos overflow */
	  else if (!sBit1 && !sBit2)
	    res = 0x7FFFFFFF;
	  /* Pos+Neg or Neg+Pos take the sign of the result */
	  else if (sBitRes1)
	    res = 0x80000000;
	  else
	    res = 0x7FFFFFFF;

	  ovX = 1;
	}
      else
	{
	  /* Shift up now after overflow detection */
	  ovX = 0;
	  res <<= 4;
	}

      res >>= 16;

      if (HL)
	STORE (DREG (dst0), REG_H_L (res << 16, DREG (dst0)));
      else
	STORE (DREG (dst0), REG_H_L (DREG (dst0), res));

      SET_ASTATREG (az, res == 0);
      SET_ASTATREG (an, res & 0x8000);
      SET_ASTATREG (v, ovX);
      if (ovX)
	SET_ASTATREG (vs, ovX);
    }
  else if ((aop == 2 || aop == 3) && aopcde == 5)
    {
      bs32 val0 = DREG (src0);
      bs32 val1 = DREG (src1);
      bs32 res;

      TRACE_INSN (cpu, "R%i.%s = R%i %s R%i (RND20)", dst0, HL ? "L" : "H",
		  src0, aop & 0x1 ? "-" : "+", src1);

      /* If subtract, just invert and add one */
      if (aop & 0x1)
	val1= ~val1 + 1;

      res = (val0 >> 4) + (val1 >> 4) + (((val0 & 0xf) + (val1 & 0xf)) >> 4);
      res += 0x8000;
      /* Don't sign extend during the shift */
      res = ((bu32)res >> 16);

      /* Don't worry about overflows, since we are shifting right */

      if (HL)
	STORE (DREG (dst0), REG_H_L (res << 16, DREG (dst0)));
      else
	STORE (DREG (dst0), REG_H_L (DREG (dst0), res));

      SET_ASTATREG (az, res == 0);
      SET_ASTATREG (an, res & 0x8000);
      SET_ASTATREG (v, 0);
    }
#endif
  else if (aopcde == 2 || aopcde == 3)
    {
//      bu32 s1, s2, val, ac0_i = 0, v_i = 0;
      TCGv s1, s2, d;

      TRACE_INSN (cpu, "R%i.%c = R%i.%c %c R%i.%c%s;",
		  dst0, HL ? 'H' : 'L',
		  src0, (aop & 2) ? 'H' : 'L',
		  (aopcde == 2) ? '+' : '-',
		  src1, (aop & 1) ? 'H' : 'L',
		  amod1 (s, x));

      s1 = tcg_temp_new();
      if (aop & 2)
	tcg_gen_shri_tl(s1, cpu_dreg[src0], 16);
      else
	tcg_gen_ext16u_tl(s1, cpu_dreg[src0]);

      s2 = tcg_temp_new();
      if (aop & 1)
	tcg_gen_shri_tl(s2, cpu_dreg[src1], 16);
      else
	tcg_gen_ext16u_tl(s2, cpu_dreg[src1]);

      d = tcg_temp_new();
      if (aopcde == 2)
	tcg_gen_add_tl(d, s1, s2);
      else
	tcg_gen_sub_tl(d, s1, s2);
      tcg_gen_andi_tl(d, d, 0xffff);

      if (HL)
	{
	  tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0], 0xffff);
	  tcg_gen_shli_tl(d, d, 16);
	  tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], d);
	}
      else
	{
	  tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0], 0xffff0000);
	  tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], d);
	}
/*
      s1 = DREG (src0);
      s2 = DREG (src1);
      if (aop & 1)
	s2 >>= 16;
      if (aop & 2)
	s1 >>= 16;

      if (aopcde == 2)
	val = add16(cpu, s1, s2, &ac0_i, &v_i, 0, 0, s, 0);
      else
	val = sub16(cpu, s1, s2, &ac0_i, &v_i, 0, 0, s, 0);

      SET_ASTATREG (ac0, ac0_i);
      SET_ASTATREG (v, v_i);
      if (HL)
	SET_DREG_H (dst0, val << 16);
      else
	SET_DREG_L (dst0, val);

      SET_ASTATREG (an, val & 0x8000);
*/
    }
  else if ((aop == 0 || aop == 2) && aopcde == 9 && s == 1)
    {
      int a = aop >> 1;
      TRACE_INSN (cpu, "A%i = R%i;", a, src0);
//      SET_AREG32 (a, DREG (src0));
      tcg_gen_ext_i32_i64(cpu_areg[a], cpu_dreg[src0]);
    }
  else if ((aop == 1 || aop == 3) && aopcde == 9 && s == 0)
    {
      int a = aop >> 1;
      TRACE_INSN (cpu, "A%i.X = R%i.L;", a, src0);
//      SET_AXREG (a, (bs8)DREG (src0));
      tmp64 = tcg_temp_new_i64();
      tcg_gen_extu_i32_i64(tmp64, cpu_dreg[src0]);
      tcg_gen_ext8s_i64(tmp64, tmp64);
      tcg_gen_shri_i64(tmp64, tmp64, 32);
      tcg_gen_andi_i64(cpu_areg[a], cpu_areg[a], 0xffffffff);
      tcg_gen_or_i64(cpu_areg[a], cpu_areg[a], tmp64);
      tcg_temp_free_i64(tmp64);
    }
  else if (aop == 3 && aopcde == 11 && (s == 0 || s == 1))
    {
//      bu64 acc0 = get_extended_acc (cpu, 0);
//      bu64 acc1 = get_extended_acc (cpu, 1);
//      bu32 carry = (bu40)acc1 < (bu40)acc0;
//      bu32 sat = 0;

      TRACE_INSN (cpu, "A0 -= A1%s;", s ? " (W32)" : "");

/*
      acc0 -= acc1;
      if ((bs64)acc0 < -0x8000000000ll)
	acc0 = -0x8000000000ull, sat = 1;
      else if ((bs64)acc0 >= 0x7fffffffffll)
	acc0 = 0x7fffffffffull, sat = 1;
*/
      tcg_gen_sub_i64(cpu_areg[0], cpu_areg[0], cpu_areg[1]);

      if (s == 1)
	{
unhandled_instruction(dc, "A0 -= A1 (W32)");
//	  if (acc0 & (bu64)0x8000000000ll)
//	    acc0 &= 0x80ffffffffll, sat = 1;
//	  else
//	    acc0 &= 0xffffffffll;
	}
/*
      STORE (AXREG (0), (acc0 >> 32) & 0xff);
      STORE (AWREG (0), acc0 & 0xffffffff);
      STORE (ASTATREG (az), acc0 == 0);
      STORE (ASTATREG (an), !!(acc0 & (bu64)0x8000000000ll));
      STORE (ASTATREG (ac0), carry);
      STORE (ASTATREG (ac0_copy), carry);
      STORE (ASTATREG (av0), sat);
      if (sat)
	STORE (ASTATREG (av0s), sat);
*/
    }
#if 0
  else if ((aop == 0 || aop == 1) && aopcde == 22)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;
      bu32 tmp0, tmp1, i;
      const char * const opts[] = { "rndl", "rndh", "tl", "th" };

      TRACE_INSN (cpu, "R%i = BYTEOP2P (R%i:%i, R%i:%i) (%s%s);", dst0,
		  src0 + 1, src0, src1 + 1, src1, opts[HL + (aop << 1)],
		  s ? ", r" : "");

      if (src0 == src1)
	illegal_instruction_combination(dc);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (0) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (0) & 3);
	}

      i = !aop * 2;
      tmp0 = ((((s1 >>  8) & 0xff) + ((s1 >>  0) & 0xff) +
	       ((s0 >>  8) & 0xff) + ((s0 >>  0) & 0xff) + i) >> 2) & 0xff;
      tmp1 = ((((s1 >> 24) & 0xff) + ((s1 >> 16) & 0xff) +
	       ((s0 >> 24) & 0xff) + ((s0 >> 16) & 0xff) + i) >> 2) & 0xff;
      SET_DREG (dst0, (tmp1 << (16 + (HL * 8))) | (tmp0 << (HL * 8)));
    }
#endif
  else if ((aop == 0 || aop == 1) && s == 0 && aopcde == 8)
    {
      TRACE_INSN (cpu, "A%i = 0;", aop);
//      SET_AREG (aop, 0);
//      tcg_gen_movi_tl(cpu_awreg[aop], 0);
//      tcg_gen_mov_tl(cpu_axreg[aop], cpu_awreg[aop]);
      tcg_gen_movi_i64(cpu_areg[0], 0);
    }
  else if (aop == 2 && s == 0 && aopcde == 8)
    {
      TRACE_INSN (cpu, "A1 = A0 = 0;");
//      SET_AREG (0, 0);
//      SET_AREG (1, 0);
/*
      tcg_gen_movi_tl(cpu_awreg[0], 0);
      tcg_gen_mov_tl(cpu_axreg[0], cpu_awreg[0]);
      tcg_gen_mov_tl(cpu_axreg[1], cpu_axreg[0]);
      tcg_gen_mov_tl(cpu_awreg[1], cpu_awreg[0]);
*/
      tcg_gen_movi_i64(cpu_areg[0], 0);
      tcg_gen_mov_i64(cpu_areg[1], cpu_areg[0]);
    }
#if 0
  else if ((aop == 0 || aop == 1 || aop == 2) && s == 1 && aopcde == 8)
    {
      bs40 acc0 = get_extended_acc (cpu, 0);
      bs40 acc1 = get_extended_acc (cpu, 1);
      bu32 sat;

      if (aop == 0 || aop == 1)
	TRACE_INSN (cpu, "A%i = A%i (S);", aop, aop);
      else
	TRACE_INSN (cpu, "A1 = A1 (S), A0 = A0 (S);");

      if (aop == 0 || aop == 2)
	{
	  sat = 0;
	  acc0 = saturate_s32(acc0, &sat);
	  acc0 |= -(acc0 & 0x80000000ull);
	  SET_AXREG (0, (acc0 >> 31) & 0xFF);
	  SET_AWREG (0, acc0 & 0xFFFFFFFF);
	  SET_ASTATREG (av0, sat);
	  if (sat)
	    SET_ASTATREG (av0s, sat);
	}
      else
	acc0 = 1;

      if (aop == 1 || aop == 2)
	{
	  sat = 0;
	  acc1 = saturate_s32(acc1, &sat);
	  acc1 |= -(acc1 & 0x80000000ull);
	  SET_AXREG (1, (acc1 >> 31) & 0xFF);
	  SET_AWREG (1, acc1 & 0xFFFFFFFF);
	  SET_ASTATREG (av1, sat);
	  if (sat)
	    SET_ASTATREG (av1s, sat);
	}
      else
	acc1 = 1;

      SET_ASTATREG (az, (acc0 == 0) || (acc1 == 0));
      SET_ASTATREG (an, ((acc0 >> 31) & 1) || ((acc1 >> 31) & 1) );
    }
#endif
  else if (aop == 3 && (s == 0 || s == 1) && aopcde == 8)
    {
      TRACE_INSN (cpu, "A%i = A%i;", s, !s);
//      SET_AXREG (s, AXREG (!s));
//      SET_AWREG (s, AWREG (!s));
//      tcg_gen_mov_tl(cpu_axreg[s], cpu_axreg[!s]);
//      tcg_gen_mov_tl(cpu_awreg[s], cpu_awreg[!s]);
      tcg_gen_mov_i64(cpu_areg[s], cpu_areg[!s]);
    }
  else if (aop == 3 && HL == 0 && aopcde == 16)
    {
      int i;
//      bu32 az;

      TRACE_INSN (cpu, "A1 = ABS A1 , A0 = ABS A0;");

      /* XXX: Missing ASTAT updates */
//      az = 0;
      for (i = 0; i < 2; ++i)
	{
/*
	  bu32 av;
	  bs40 acc = get_extended_acc (cpu, i);

	  if (acc >> 39)
	    acc = -acc;
	  av = acc == ((bs40)1 << 39);
	  if (av)
	    acc = ((bs40)1 << 39) - 1;

	  SET_AREG (i, acc);
	  SET_ASTATREG (av[i], av);
	  if (av)
	    SET_ASTATREG (avs[i], av);
	  az |= (acc == 0);
*/
	  /* XXX: Missing saturation */
	  gen_abs_i64(cpu_areg[i], cpu_areg[i]);
	}
//      SET_ASTATREG (az, az);
//      SET_ASTATREG (an, 0);
    }
#if 0
  else if (aop == 0 && aopcde == 23)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;
      bs32 tmp0, tmp1;

      TRACE_INSN (cpu, "R%i = BYTEOP3P (R%i:%i, R%i:%i) (%s%s);", dst0,
		  src0 + 1, src0, src1 + 1, src1, HL ? "HI" : "LO",
		  s ? ", R" : "");

      if (src0 == src1)
	illegal_instruction_combination(dc);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      tmp0 = (bs32)(bs16)(s0 >>  0) + ((s1 >> ( 0 + (8 * !HL))) & 0xff);
      tmp1 = (bs32)(bs16)(s0 >> 16) + ((s1 >> (16 + (8 * !HL))) & 0xff);
      SET_DREG (dst0, (CLAMP (tmp0, 0, 255) << ( 0 + (8 * HL))) |
		      (CLAMP (tmp1, 0, 255) << (16 + (8 * HL))));
    }
#endif
  else if ((aop == 0 || aop == 1) && aopcde == 16)
    {
//      bu32 av;
//      bs40 acc;

      TRACE_INSN (cpu, "A%i = ABS A%i;", HL, aop);

      /* XXX: Missing saturation */
      gen_abs_i64(cpu_areg[aop], cpu_areg[aop]);

      /* XXX: Missing ASTAT updates */
/*
      acc = get_extended_acc (cpu, aop);
      if (acc >> 39)
	acc = -acc;
      av = acc == ((bs40)1 << 39);
      if (av)
	acc = ((bs40)1 << 39) - 1;
      SET_AREG (HL, acc);

      SET_ASTATREG (av[HL], av);
      if (av)
	SET_ASTATREG (avs[HL], av);
      SET_ASTATREG (az, acc == 0);
      SET_ASTATREG (an, 0);
*/
    }
#if 0
  else if (aop == 3 && aopcde == 12)
    {
      bs32 res = DREG (src0);
      bs32 ovX;
      bool sBit_a, sBit_b;

      TRACE_INSN (cpu, "R%i.%s = R%i (RND);", dst0, HL == 0 ? "L" : "H", src0);

      sBit_b = !!(res & 0x80000000);

      res += 0x8000;
      sBit_a = !!(res & 0x80000000);

      /* Overflow if the sign bit changed when we rounded */
      if ((res >> 16) && (sBit_b != sBit_a))
	{
	  ovX = 1;
	  if (!sBit_b)
	    res = 0x7FFF;
	  else
	    res = 0x8000;
	}
      else
	{
	  res = res >> 16;
	  ovX = 0;
	}

      if (!HL)
	SET_DREG (dst0, REG_H_L (DREG (dst0), res));
      else
	SET_DREG (dst0, REG_H_L (res << 16, DREG (dst0)));

      SET_ASTATREG (az, res == 0);
      SET_ASTATREG (an, res < 0);
      SET_ASTATREG (v, ovX);
      if (ovX)
	SET_ASTATREG (vs, ovX);
    }
  else if (aop == 3 && HL == 0 && aopcde == 15)
    {
      bu32 hi = (-(bs16)(DREG (src0) >> 16)) << 16;
      bu32 lo = (-(bs16)(DREG (src0) & 0xFFFF)) & 0xFFFF;
      int v, ac0, ac1;

      TRACE_INSN (cpu, "R%i = -R%i (V);", dst0, src0);

      v = ac0 = ac1 = 0;

      if (hi == 0x80000000)
	{
	  hi = 0x7fff0000;
	  v = 1;
	}
      else if (hi == 0)
	ac1 = 1;

      if (lo == 0x8000)
	{
	  lo = 0x7fff;
	  v = 1;
	}
      else if (lo == 0)
	ac0 = 1;

      SET_DREG (dst0, hi | lo);

      SET_ASTATREG (v, v);
      if (v)
	SET_ASTATREG (vs, 1);
      SET_ASTATREG (ac0, ac0);
      SET_ASTATREG (ac1, ac1);
      setflags_nz_2x16(cpu, DREG (dst0));
    }
#endif
  else if (aop == 3 && HL == 0 && aopcde == 14)
    {
      TRACE_INSN (cpu, "A1 = - A1 , A0 = - A0;");

//      SET_AREG (0, saturate_s40 (-get_extended_acc (cpu, 0)));
//      SET_AREG (1, saturate_s40 (-get_extended_acc (cpu, 1)));
      tcg_gen_neg_i64(cpu_areg[1], cpu_areg[1]);
      tcg_gen_neg_i64(cpu_areg[0], cpu_areg[0]);
      /* XXX: what ASTAT flags need updating ?  */
    }
  else if ((aop == 0 || aop == 1) && (HL == 0 || HL == 1) && aopcde == 14)
    {
//      bs40 src_acc = get_extended_acc (cpu, aop);

      TRACE_INSN (cpu, "A%i = - A%i;", HL, aop);

//      SET_AREG (HL, saturate_s40 (-src_acc));

      tcg_gen_neg_i64(cpu_areg[HL], cpu_areg[aop]);

/*
      SET_ASTATREG (az, AWREG (HL) == 0 && AXREG (HL) == 0);
      SET_ASTATREG (an, AXREG (HL) >> 7);
      SET_ASTATREG (ac0, src_acc == 0);
      if (HL == 0)
	{
	  SET_ASTATREG (av0, src_acc < 0);
	  if (ASTATREG (av0))
	    SET_ASTATREG (av0s, 1);
	}
      else
	{
	  SET_ASTATREG (av1, src_acc < 0);
	  if (ASTATREG (av1))
	    SET_ASTATREG (av1s, 1);
	}
*/
    }
#if 0
  else if (aop == 0 && aopcde == 12)
    {
      bs16 tmp0_hi = DREG (src0) >> 16;
      bs16 tmp0_lo = DREG (src0);
      bs16 tmp1_hi = DREG (src1) >> 16;
      bs16 tmp1_lo = DREG (src1);

      TRACE_INSN (cpu, "R%i.L = R%i.H = SIGN(R%i.H) * R%i.H + SIGN(R%i.L) & R%i.L;",
		  dst0, dst0, src0, src1, src0, src1);

      if ((tmp0_hi >> 15) & 1)
	tmp1_hi = ~tmp1_hi + 1;

      if ((tmp0_lo >> 15) & 1)
	tmp1_lo = ~tmp1_lo + 1;

      tmp1_hi = tmp1_hi + tmp1_lo;

      STORE (DREG (dst0), REG_H_L (tmp1_hi << 16, tmp1_hi));
    }
#endif
  else if (aopcde == 0)
    {
/*
      bu32 s0 = DREG (src0);
      bu32 s1 = DREG (src1);
      bu32 s0h = s0 >> 16;
      bu32 s0l = s0 & 0xFFFF;
      bu32 s1h = s1 >> 16;
      bu32 s1l = s1 & 0xFFFF;
      bu32 t0, t1;
      bu32 ac1_i = 0, ac0_i = 0, v_i = 0, z_i = 0, n_i = 0;
*/
      TCGv s0, s1, t0, t1;

      TRACE_INSN (cpu, "R%i = R%i %c|%c R%i%s;", dst0, src0,
		  (aop & 2) ? '-' : '+', (aop & 1) ? '-' : '+', src1,
		  amod0 (s, x));

      if (s || x)
	unhandled_instruction(dc, "S/CO/SCO with +|+/-|-");

      s0 = tcg_temp_local_new();
      s1 = tcg_temp_local_new();

/*
      if (aop & 2)
	t0 = sub16(cpu, s0h, s1h, &ac1_i, &v_i, &z_i, &n_i, s, 0);
      else
	t0 = add16(cpu, s0h, s1h, &ac1_i, &v_i, &z_i, &n_i, s, 0);
*/

      t0 = tcg_temp_local_new();
      tcg_gen_shri_tl(s0, cpu_dreg[src0], 16);
      tcg_gen_shri_tl(s1, cpu_dreg[src1], 16);
      if (aop & 2)
	tcg_gen_sub_tl(t0, s0, s1);
      else
	tcg_gen_add_tl(t0, s0, s1);

/*
      if (aop & 1)
	t1 = sub16(cpu, s0l, s1l, &ac0_i, &v_i, &z_i, &n_i, s, 0);
      else
	t1 = add16(cpu, s0l, s1l, &ac0_i, &v_i, &z_i, &n_i, s, 0);
*/

      t1 = tcg_temp_local_new();
      tcg_gen_andi_tl(s0, cpu_dreg[src0], 0xffff);
      tcg_gen_andi_tl(s1, cpu_dreg[src1], 0xffff);
      if (aop & 1)
	tcg_gen_sub_tl(t1, s0, s1);
      else
	tcg_gen_add_tl(t1, s0, s1);

      tcg_temp_free(s0);
      tcg_temp_free(s1);

      astat_queue_state2(dc, ARRAY_OP_VECTOR_ADD_ADD + aop, t0, t1);

/*
      SET_ASTATREG (ac1, ac1_i);
      SET_ASTATREG (ac0, ac0_i);
      SET_ASTATREG (az, z_i);
      SET_ASTATREG (an, n_i);
      SET_ASTATREG (v, v_i);
      if (v_i)
	SET_ASTATREG (vs, v_i);
*/

      if (x)
	{
	  /* dst0.h = t1; dst0.l = t0 */
	  tcg_gen_ext16u_tl(cpu_dreg[dst0], t0);
	  tcg_gen_shli_tl(t1, t1, 16);
	  tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], t1);
	}
      else
	{
	  /* dst0.h = t0; dst0.l = t1 */
	  tcg_gen_ext16u_tl(cpu_dreg[dst0], t1);
	  tcg_gen_shli_tl(t0, t0, 16);
	  tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], t0);
	}
    }
#if 0
  else if (aop == 1 && aopcde == 12)
    {
      bu32 val0 = (((AWREG (0) & 0xFFFF0000) >> 16) + (AWREG (0) & 0xFFFF)) & 0xFFFF;
      bu32 val1 = (((AWREG (1) & 0xFFFF0000) >> 16) + (AWREG (1) & 0xFFFF)) & 0xFFFF;

      TRACE_INSN (cpu, "R%i = A1.L + A1.H, R%i = A0.L + A0.H;", dst1, dst0);

      if (dst0 == dst1)
	illegal_instruction_combination(dc);

      if (val0 & 0x8000)
	val0 |= 0xFFFF0000;

      if (val1 & 0x8000)
	val1 |= 0xFFFF0000;

      SET_DREG (dst0, val0);
      SET_DREG (dst1, val1);
      /* XXX: ASTAT ?  */
    }
  else if (aopcde == 1)
    {
      bu32 d0, d1;
      bu32 x0, x1;
      bu16 s0L =  (DREG (src0) & 0xFFFF);
      bu16 s0H = ((DREG (src0) >> 16) & 0xFFFF);
      bu16 s1L =  (DREG (src1) & 0xFFFF);
      bu16 s1H = ((DREG (src1) >> 16) & 0xFFFF);
      bu32 v_i = 0, n_i = 0, z_i = 0;

      TRACE_INSN (cpu, "R%i = R%i %s R%i, R%i = R%i %s R%i%s;",
		  dst1, src0, HL ? "+|-" : "+|+", src1,
		  dst0, src0, HL ? "-|+" : "-|-", src1,
		  amod0amod2(s, x, aop));

      if (dst0 == dst1)
	illegal_instruction_combination(dc);

      if (HL == 0)
	{
	  x0 = add16(cpu, s0H, s1H, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  x1 = add16(cpu, s0L, s1L, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  d1 = (x0 << 16) | x1;

	  x0 = sub16(cpu, s0H, s1H, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  x1 = sub16(cpu, s0L, s1L, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  if (x == 0)
	    d0 =(x0 << 16) | x1;
	  else
	    d0 = (x1 << 16) | x0;
	}
      else
	{
	  x0 = add16(cpu, s0H, s1H, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  x1 = sub16(cpu, s0L, s1L, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  d1 = (x0 << 16) | x1;

	  x0 = sub16(cpu, s0H, s1H, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  x1 = add16(cpu, s0L, s1L, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  if (x == 0)
	    d0 = (x0 << 16) | x1;
	  else
	    d0 = (x1 << 16) | x0;
	}
      SET_ASTATREG (az, z_i);
      SET_ASTATREG (an, n_i);
      SET_ASTATREG (v, v_i);
      if (v_i)
	SET_ASTATREG (vs, v_i);

      STORE (DREG (dst0), d0);
      STORE (DREG (dst1), d1);
    }
#endif
  else if ((aop == 0 || aop == 1 || aop == 2) && aopcde == 11)
    {
/*
      bs40 acc0 = get_extended_acc (cpu, 0);
      bs40 acc1 = get_extended_acc (cpu, 1);
      bu32 v, dreg, sat = 0;
      bu32 carry = !!((bu40)~acc1 < (bu40)acc0);
*/

      if (aop == 0)
	TRACE_INSN (cpu, "R%i = (A0 += A1);", dst0);
      else if (aop == 1)
	TRACE_INSN (cpu, "R%i.%c = (A0 += A1);", dst0, HL ? 'H' : 'L');
      else
	TRACE_INSN (cpu, "A0 += A1%s;", s ? " (W32)" : "");

/*
      acc0 += acc1;
      acc0 = saturate_s40_astat (acc0, &v);
*/

      tcg_gen_add_i64(cpu_areg[0], cpu_areg[0], cpu_areg[1]);

      if (aop == 2 && s == 1)   /* A0 += A1 (W32) */
	{
unhandled_instruction(dc, "A0 += A1 (W32)");
//	  if (acc0 & (bs40)0x8000000000ll)
//	    acc0 &= 0x80ffffffffll;
//	  else
//	    acc0 &= 0xffffffffll;
	}
/*
      STORE (AXREG (0), acc0 >> 32);
      STORE (AWREG (0), acc0);
      SET_ASTATREG (av0, v && acc1);
      if (v)
	SET_ASTATREG (av0s, v);
*/

      if (aop == 0)
	{
	  /* Dregs = A0 += A1 */
	  tcg_gen_trunc_i64_i32(cpu_dreg[dst0], cpu_areg[0]);
	}
      else if (aop == 1)
	{
	  /* Dregs_lo = A0 += A1 */
	  tmp = tcg_temp_new();
	  tcg_gen_trunc_i64_i32(tmp, cpu_areg[0]);
	  gen_mov_l_tl(cpu_dreg[dst0], tmp);
	  tcg_temp_free(tmp);
	}

#if 0
      if (aop == 0 || aop == 1)
	{
	  if (aop)	/* Dregs_lo = A0 += A1 */
	    {
	      dreg = saturate_s32(rnd16(acc0) << 16, &sat);
	      if (HL)
		STORE (DREG (dst0), REG_H_L (dreg, DREG (dst0)));
	      else
		STORE (DREG (dst0), REG_H_L (DREG (dst0), dreg >> 16));
	    }
	  else		/* Dregs = A0 += A1 */
	    {
	      dreg = saturate_s32(acc0, &sat);
	      STORE (DREG (dst0), dreg);
	    }

	  STORE (ASTATREG (az), dreg == 0);
	  STORE (ASTATREG (an), !!(dreg & 0x80000000));
	  STORE (ASTATREG (ac0), carry);
	  STORE (ASTATREG (ac0_copy), carry);
	  STORE (ASTATREG (v), sat);
	  STORE (ASTATREG (v_copy), sat);
	  if (sat)
	    STORE (ASTATREG (vs), sat);
	}
      else
	{
	  STORE (ASTATREG (az), acc0 == 0);
	  STORE (ASTATREG (an), !!(acc0 & 0x8000000000ull));
	  STORE (ASTATREG (ac0), carry);
	  STORE (ASTATREG (ac0_copy), carry);
	}
#endif
    } else if ((aop == 0 || aop == 1) && aopcde == 10) {
        /* Dreg_lo{dst0} = Areg_x{aop}; */
//      SET_DREG_L (dst0, (bs8)AXREG (aop));
        tmp = tcg_temp_new();
        tmp64 = tcg_temp_new_i64();
        tcg_gen_shri_i64(tmp64, cpu_areg[aop], 32);
        tcg_gen_trunc_i64_i32(tmp, tmp64);
        tcg_temp_free_i64(tmp64);
        tcg_gen_ext8s_tl(tmp, tmp);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if (aop == 0 && aopcde == 4) {
        /* Dreg{dst0} = Dreg{src0} + Dreg{src1} (amod1(s,x)); */
//      SET_DREG (dst0, add32 (cpu, DREG (src0), DREG (src1), 1, s));
        tcg_gen_add_tl(cpu_dreg[dst0], cpu_dreg[src0], cpu_dreg[src1]);
    } else if (aop == 1 && aopcde == 4) {
        /* Dreg{dst0} = Dreg{src0} - Dreg{src1} (amod1(s,x)); */
//      SET_DREG (dst0, sub32 (cpu, DREG (src0), DREG (src1), 1, s, 0));
        tcg_gen_sub_tl(cpu_dreg[dst0], cpu_dreg[src0], cpu_dreg[src1]);
        astat_queue_state3(dc, ASTAT_OP_SUB32, cpu_dreg[dst0], cpu_dreg[src0], cpu_dreg[src1]);
    } else if (aop == 2 && aopcde == 4) {
        /* Dreg{dst1} = Dreg{src0} + Dreg{src1}, Dreg{dst0} = Dreg{src0} - Dreg{src1} (amod1(s,x)); */

        /*if (dst0 == dst1)
            illegal_instruction_combination(dc);*/

//      STORE (DREG (dst1), add32 (cpu, DREG (src0), DREG (src1), 1, s));
        tcg_gen_add_tl(cpu_dreg[dst1], cpu_dreg[src0], cpu_dreg[src1]);
//      STORE (DREG (dst0), sub32 (cpu, DREG (src0), DREG (src1), 1, s, 1));
        tcg_gen_sub_tl(cpu_dreg[dst0], cpu_dreg[src0], cpu_dreg[src1]);
        /* XXX: Missing ASTAT updates */
    }
#if 0
  else if ((aop == 0 || aop == 1) && aopcde == 17)
    {
      bs40 acc0 = get_extended_acc (cpu, 0);
      bs40 acc1 = get_extended_acc (cpu, 1);
      bs40 val0, val1, sval0, sval1;
      bu32 sat, sat_i;

      TRACE_INSN (cpu, "R%i = A%i + A%i, R%i = A%i - A%i%s",
		  dst1, !aop, aop, dst0, !aop, aop, amod1 (s, x));

      if (dst0 == dst1)
	illegal_instruction_combination(dc);

      val1 = acc0 + acc1;
      if (aop)
	val0 = acc0 - acc1;
      else
	val0 = acc1 - acc0;

      sval0 = saturate_s32(val0, &sat);
      sat_i = sat;
      sval1 = saturate_s32(val1, &sat);
      sat_i |= sat;
      if (s)
	{
	  val0 = sval0;
	  val1 = sval1;
	}

      STORE (DREG (dst0), val0);
      STORE (DREG (dst1), val1);
      SET_ASTATREG (v, sat_i);
      if (sat_i)
	SET_ASTATREG (vs, sat_i);
      SET_ASTATREG (an, val0 & 0x80000000 || val1 & 0x80000000);
      SET_ASTATREG (az, val0 == 0 || val1 == 0);
      SET_ASTATREG (ac1, (bu40)~acc0 < (bu40)acc1);
      if (aop)
	SET_ASTATREG (ac0, !!((bu40)acc1 <= (bu40)acc0));
      else
	SET_ASTATREG (ac0, !!((bu40)acc0 <= (bu40)acc1));
    }
  else if (aop == 0 && aopcde == 18)
    {
      bu40 acc0 = get_extended_acc (cpu, 0);
      bu40 acc1 = get_extended_acc (cpu, 1);
      bu32 s0L = DREG (src0);
      bu32 s0H = DREG (src0 + 1);
      bu32 s1L = DREG (src1);
      bu32 s1H = DREG (src1 + 1);
      bu32 s0, s1;
      bs16 tmp0, tmp1, tmp2, tmp3;

      /* This instruction is only defined for register pairs R1:0 and R3:2 */
      if (!((src0 == 0 || src0 == 2) && (src1 == 0 || src1 == 2)))
	illegal_instruction(dc);

      TRACE_INSN (cpu, "SAA (R%i:%i, R%i:%i)%s", src0 + 1, src0,
		  src1 + 1, src1, s ? " (R)" :"");

      /* Bit s determines the order of the two registers from a pair:
       * if s=0 the low-order bytes come from the low reg in the pair,
       * and if s=1 the low-order bytes come from the high reg.
       */

      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      /* find the absolute difference between pairs,
       * make it absolute, then add it to the existing accumulator half
       */
      /* Byte 0 */
      tmp0  = ((s0 << 24) >> 24) - ((s1 << 24) >> 24);
      tmp1  = ((s0 << 16) >> 24) - ((s1 << 16) >> 24);
      tmp2  = ((s0 <<  8) >> 24) - ((s1 <<  8) >> 24);
      tmp3  = ((s0 <<  0) >> 24) - ((s1 <<  0) >> 24);

      tmp0  = (tmp0 < 0) ? -tmp0 : tmp0;
      tmp1  = (tmp1 < 0) ? -tmp1 : tmp1;
      tmp2  = (tmp2 < 0) ? -tmp2 : tmp2;
      tmp3  = (tmp3 < 0) ? -tmp3 : tmp3;

      s0L = saturate_u16((bu32)tmp0 + ( acc0        & 0xffff), 0);
      s0H = saturate_u16((bu32)tmp1 + ((acc0 >> 16) & 0xffff), 0);
      s1L = saturate_u16((bu32)tmp2 + ( acc1        & 0xffff), 0);
      s1H = saturate_u16((bu32)tmp3 + ((acc1 >> 16) & 0xffff), 0);

      STORE (AWREG (0), (s0H << 16) | (s0L & 0xFFFF));
      STORE (AXREG (0), 0);
      STORE (AWREG (1), (s1H << 16) | (s1L & 0xFFFF));
      STORE (AXREG (1), 0);
    }
#endif
    else if (aop == 3 && aopcde == 18)
        /* DISALGNEXCPT; */
        unhandled_instruction(dc, "DISALGNEXCPT");
#if 0
  else if ((aop == 0 || aop == 1) && aopcde == 20)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;
      const char * const opts[] = { "", " (R)", " (T)", " (T, R)" };

      TRACE_INSN (cpu, "R%i = BYTEOP1P (R%i:%i, R%i:%i)%s;", dst0,
		  src0 + 1, src0, src1 + 1, src1, opts[s + (aop << 1)]);

      if (src0 == src1)
	illegal_instruction_combination(dc);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      SET_DREG (dst0,
		(((((s0 >>  0) & 0xff) + ((s1 >>  0) & 0xff) + !aop) >> 1) <<  0) |
		(((((s0 >>  8) & 0xff) + ((s1 >>  8) & 0xff) + !aop) >> 1) <<  8) |
		(((((s0 >> 16) & 0xff) + ((s1 >> 16) & 0xff) + !aop) >> 1) << 16) |
		(((((s0 >> 24) & 0xff) + ((s1 >> 24) & 0xff) + !aop) >> 1) << 24));
    }
  else if (aop == 0 && aopcde == 21)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;

      TRACE_INSN (cpu, "(R%i, R%i) = BYTEOP16P (R%i:%i, R%i:%i)%s;", dst1, dst0,
		  src0 + 1, src0, src1 + 1, src1, s ? " (R)" : "");

      if (dst0 == dst1)
	illegal_instruction_combination(dc);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      SET_DREG (dst0,
		((((s0 >>  0) & 0xff) + ((s1 >>  0) & 0xff)) <<  0) |
		((((s0 >>  8) & 0xff) + ((s1 >>  8) & 0xff)) << 16));
      SET_DREG (dst1,
		((((s0 >> 16) & 0xff) + ((s1 >> 16) & 0xff)) <<  0) |
		((((s0 >> 24) & 0xff) + ((s1 >> 24) & 0xff)) << 16));
    }
  else if (aop == 1 && aopcde == 21)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;

      TRACE_INSN (cpu, "(R%i, R%i) = BYTEOP16M (R%i:%i, R%i:%i)%s;", dst1, dst0,
		  src0 + 1, src0, src1 + 1, src1, s ? " (R)" : "");

      if (dst0 == dst1)
	illegal_instruction_combination(dc);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      SET_DREG (dst0,
		(((((s0 >>  0) & 0xff) - ((s1 >>  0) & 0xff)) <<  0) & 0xffff) |
		(((((s0 >>  8) & 0xff) - ((s1 >>  8) & 0xff)) << 16)));
      SET_DREG (dst1,
		(((((s0 >> 16) & 0xff) - ((s1 >> 16) & 0xff)) <<  0) & 0xffff) |
		(((((s0 >> 24) & 0xff) - ((s1 >> 24) & 0xff)) << 16)));
    }
#endif
    else if (aop == 1 && aopcde == 7) {
        int l;
        /* Dreg{dst0} = MIN (Dreg{src0}, Dreg{src1}); */
        /* Source and dest regs might be the same, so we can't clobber;
           XXX: Well, we could, but we need the logic here to be smarter */
        tmp = tcg_temp_local_new();
        l = gen_new_label();
        tcg_gen_mov_tl(tmp, cpu_dreg[src0]);
        tcg_gen_brcond_tl(TCG_COND_GE, cpu_dreg[src1], cpu_dreg[src0], l);
        tcg_gen_mov_tl(tmp, cpu_dreg[src1]);
        gen_set_label(l);
        tcg_gen_mov_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);

        astat_queue_state1(dc, ASTAT_OP_MIN_MAX, cpu_dreg[dst0]);
    } else if (aop == 0 && aopcde == 7) {
        int l;
        /* Dreg{dst0} = MAX (Dreg{src0}, Dreg{src1}); */
        /* Source and dest regs might be the same, so we can't clobber;
           XXX: Well, we could, but we need the logic here to be smarter */
        tmp = tcg_temp_local_new();
        l = gen_new_label();
        tcg_gen_mov_tl(tmp, cpu_dreg[src0]);
        tcg_gen_brcond_tl(TCG_COND_LT, cpu_dreg[src1], cpu_dreg[src0], l);
        tcg_gen_mov_tl(tmp, cpu_dreg[src1]);
        gen_set_label(l);
        tcg_gen_mov_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);

        astat_queue_state1(dc, ASTAT_OP_MIN_MAX, cpu_dreg[dst0]);
    } else if (aop == 2 && aopcde == 7) {
        /* Dreg{dst0} = ABS Dreg{src0}; */

        /* XXX: Missing saturation support (and ASTAT V/VS) */
        gen_abs_tl(cpu_dreg[dst0], cpu_dreg[src0]);

        astat_queue_state2(dc, ASTAT_OP_ABS, cpu_dreg[dst0], cpu_dreg[src0]);
    }
#if 0
  else if (aop == 3 && aopcde == 7)
    {
      bu32 val = DREG (src0);

      TRACE_INSN (cpu, "R%i = - R%i %s;", dst0, src0, amod1 (s, 0));

      if (s && val == 0x80000000)
	{
	  val = 0x7fffffff;
	  SET_ASTATREG (v, 1);
	  SET_ASTATREG (vs, 1);
	}
      else if (val == 0x80000000)
	val = 0x80000000;
      else
	val = -val;
      SET_DREG (dst0, val);

      SET_ASTATREG (az, val == 0);
      SET_ASTATREG (an, val & 0x80000000);
    }
  else if (aop == 2 && aopcde == 6)
    {
      bu32 in = DREG (src0);
      bu32 hi = (in & 0x80000000 ? (bu32)-(bs16)(in >> 16) : in >> 16) << 16;
      bu32 lo = (in & 0x8000 ? (bu32)-(bs16)(in & 0xFFFF) : in) & 0xFFFF;
      int v;

      TRACE_INSN (cpu, "R%i = ABS R%i (V);", dst0, src0);

      v = 0;
      if (hi == 0x80000000)
	{
	  hi = 0x7fff0000;
	  v = 1;
	}
      if (lo == 0x8000)
	{
	  lo = 0x7fff;
	  v = 1;
	}
      SET_DREG (dst0, hi | lo);

      SET_ASTATREG (v, v);
      if (v)
	SET_ASTATREG (vs, 1);
      setflags_nz_2x16(cpu, DREG (dst0));
    }
  else if (aop == 1 && aopcde == 6)
    {
      TRACE_INSN (cpu, "R%i = MIN (R%i, R%i) (V);", dst0, src0, src1);
      SET_DREG (dst0, min2x16(cpu, DREG (src0), DREG (src1)));
    }
  else if (aop == 0 && aopcde == 6)
    {
      TRACE_INSN (cpu, "R%i = MAX (R%i, R%i) (V);", dst0, src0, src1);
      SET_DREG (dst0, max2x16(cpu, DREG (src0), DREG (src1)));
    }
#endif
    else if (aop == 0 && aopcde == 24) {
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

        /*if (dst0 == dst1)
            illegal_instruction_combination(dc);*/

        if (s)
            hi = cpu_dreg[src0], lo = cpu_dreg[src0 + 1];
        else
            hi = cpu_dreg[src0 + 1], lo = cpu_dreg[src0];

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
        tcg_gen_trunc_i64_i32(tmp, tmp64);
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
        int l;
        TCGv a_lo;
        TCGCond conds[] = {
            /* GT */ TCG_COND_LE,
            /* GE */ TCG_COND_LT,
            /* LT */ TCG_COND_GE,
            /* LE */ TCG_COND_GT,
        };

        /* (Dreg{dst1}, Dreg{dst0}) = SEARCH Dreg{src0} (mode{aop}); */

        /*if (dst0 == dst1)
            illegal_instruction_combination(dc);*/

        a_lo = tcg_temp_local_new();
        tmp = tcg_temp_local_new();

        /* Compare A1 to Dreg_hi{src0} */
        tcg_gen_trunc_i64_i32(a_lo, cpu_areg[1]);
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
        tcg_gen_trunc_i64_i32(a_lo, cpu_areg[0]);
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
    } else
        illegal_instruction(dc);
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
    int sopcde = ((iw0 >> (DSP32Shift_sopcde_bits - 16)) & DSP32Shift_sopcde_mask);
    int M = ((iw0 >> (DSP32Shift_M_bits - 16)) & DSP32Shift_M_mask);
    TCGv tmp;

    TRACE_EXTRACT("%s: M:%i sopcde:%i sop:%i HLs:%i dst0:%i src0:%i src1:%i",
                  __func__, M, sopcde, sop, HLs, dst0, src0, src1);

    if ((sop == 0 || sop == 1) && sopcde == 0) {
        int l, endl;
        TCGv val;

        /* Dreg{dst0}_hi{HLs&2} = ASHIFT Dreg{src1}_hi{HLs&1} BY Dreg_lo{src0} (S){sop==1}; */
        /* Dreg{dst0}_lo{!HLs&2} = ASHIFT Dreg{src1}_lo{!HLs&1} BY Dreg_lo{src0} (S){sop==1}; */

        tmp = tcg_temp_local_new();
        gen_extNsi_tl(tmp, cpu_dreg[src0], 6);

        val = tcg_temp_local_new();
        if (HLs & 1)
            tcg_gen_sari_tl(val, cpu_dreg[src1], 16);
        else
            tcg_gen_ext16s_tl(val, cpu_dreg[src1]);

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

        if (HLs & 2)
            gen_mov_h_tl(cpu_dreg[dst0], val);
        else
            gen_mov_l_tl(cpu_dreg[dst0], val);

        tcg_temp_free(val);
        tcg_temp_free(tmp);

        /* XXX: Missing V updates */
    } else if (sop == 2 && sopcde == 0) {
        int l, endl;
        TCGv val;

        /* Dreg{dst0}_hi{HLs&2} = LSHIFT Dreg{src1}_hi{HLs&1} BY Dreg_lo{src0}; */
        /* Dreg{dst0}_lo{!HLs&2} = LSHIFT Dreg{src1}_lo{!HLs&1} BY Dreg_lo{src0}; */

        tmp = tcg_temp_local_new();
        gen_extNsi_tl(tmp, cpu_dreg[src0], 6);

        val = tcg_temp_local_new();
        if (HLs & 1)
            tcg_gen_shri_tl(val, cpu_dreg[src1], 16);
        else
            tcg_gen_ext16u_tl(val, cpu_dreg[src1]);

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

        if (HLs & 2)
            gen_mov_h_tl(cpu_dreg[dst0], val);
        else
            gen_mov_l_tl(cpu_dreg[dst0], val);

        tcg_temp_free(val);
        tcg_temp_free(tmp);

        /* XXX: Missing AZ/AN/V updates */
#if 0
    } else if (sop == 2 && sopcde == 3 && (HLs == 1 || HLs == 0)) {
      int shift = imm6(DREG (src0) & 0xFFFF);
      bu32 cc = CCREG;
      bu40 acc = get_unextended_acc (cpu, HLs);

      TRACE_INSN (cpu, "A%i = ROT A%i BY R%i.L;", HLs, HLs, src0);

      acc = rot40 (acc, shift, &cc);
      SET_AREG (HLs, acc);
      if (shift)
	SET_CCREG (cc);
    }
  else if (sop == 0 && sopcde == 3 && (HLs == 0 || HLs == 1))
    {
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;
      bu64 val = get_extended_acc (cpu, HLs);

      HLs = !!HLs;
      TRACE_INSN (cpu, "A%i = ASHIFT A%i BY R%i.L;", HLs, HLs, src0);

      if (shft <= 0)
	val = ashiftrt (cpu, val, -shft, 40);
      else
	val = lshift (cpu, val, shft, 40, 0);

      STORE (AXREG (HLs), (val >> 32) & 0xff);
      STORE (AWREG (HLs), (val & 0xffffffff));
    }
  else if (sop == 1 && sopcde == 3 && (HLs == 0 || HLs == 1))
    {
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;
      bu64 val;

      HLs = !!HLs;
      TRACE_INSN (cpu, "A%i = LSHIFT A%i BY R%i.L;", HLs, HLs, src0);
      val = get_extended_acc (cpu, HLs);

      if (shft <= 0)
	val = lshiftrt (cpu, val, -shft, 40);
      else
	val = lshift (cpu, val, shft, 40, 0);

      STORE (AXREG (HLs), (val >> 32) & 0xff);
      STORE (AWREG (HLs), (val & 0xffffffff));
    }
  else if ((sop == 0 || sop == 1) && sopcde == 1)
    {
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;
      bu16 val0, val1;
      bu32 astat;

      TRACE_INSN (cpu, "R%i = ASHIFT R%i BY R%i.L (V%s);",
		  dst0, src1, src0, sop == 1 ? ",S" : "");

      val0 = (bu16)DREG (src1) & 0xFFFF;
      val1 = (bu16)((DREG (src1) & 0xFFFF0000) >> 16);

      if (shft <= 0)
	{
	  val0 = ashiftrt (cpu, val0, -shft, 16);
	  astat = ASTAT;
	  val1 = ashiftrt (cpu, val1, -shft, 16);
	}
      else
	{
	  val0 = lshift (cpu, val0, shft, 16, sop == 1);
	  astat = ASTAT;
	  val1 = lshift (cpu, val1, shft, 16, sop == 1);
	}
      SET_ASTAT (ASTAT | astat);
      STORE (DREG (dst0), (val1 << 16) | val0);
#endif
    } else if ((sop == 0 || sop == 1 || sop == 2) && sopcde == 2) {
        /* Dreg{dst0} = [LA]SHIFT Dreg{src1} BY Dreg_lo{src0} (opt_S); */
        /* sop == 1 : opt_S */
        int l, endl;

        /* XXX: Missing V/VS update */
        if (sop == 1)
            unhandled_instruction(dc, "[AL]SHIFT with (S)");

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
        int l;

/*
        int shift = imm6(DREG (src0) & 0xFFFF);
        bu32 src = DREG (src1);
        bu32 ret, cc = CCREG;

        ret = rot32 (src, shift, &cc);
        STORE (DREG (dst0), ret);
        if (shift)
            SET_CCREG (cc);
*/

        tmp = tcg_temp_new();
        tcg_gen_andi_tl(tmp, cpu_dreg[src1], 0xffff);
        tcg_gen_rotr_tl(cpu_dreg[dst0], cpu_dreg[src1], tmp);
        l = gen_new_label();
        tcg_gen_brcondi_tl(TCG_COND_NE, tmp, 0, l);
        gen_set_label(l);
        tcg_temp_free(tmp);
    }
#if 0
  else if (sop == 2 && sopcde == 1)
    {
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;
      bu16 val0, val1;
      bu32 astat;

      TRACE_INSN (cpu, "R%i = LSHIFT R%i BY R%i.L (V);", dst0, src1, src0);

      val0 = (bu16)DREG (src1) & 0xFFFF;
      val1 = (bu16)((DREG (src1) & 0xFFFF0000) >> 16);

      if (shft <= 0)
	{
	  val0 = lshiftrt (cpu, val0, -shft, 16);
	  astat = ASTAT;
	  val1 = lshiftrt (cpu, val1, -shft, 16);
	}
      else
	{
	  val0 = lshift (cpu, val0, shft, 16, 0);
	  astat = ASTAT;
	  val1 = lshift (cpu, val1, shft, 16, 0);
	}
      SET_ASTAT (ASTAT | astat);
      STORE (DREG (dst0), (val1 << 16) | val0);
    }
#endif
    else if (sopcde == 4) {
        TCGv tmph;
//      bu32 sv0 = DREG (src0);
//      bu32 sv1 = DREG (src1);
        /* Dreg{dst0} = PACK (Dreg{src1}_hi{sop&2}, Dreg{src0}_hi{sop&1}); */
        /* Dreg{dst0} = PACK (Dreg{src1}_lo{!sop&2}, Dreg{src0}_lo{!sop&1}); */
//      if (sop & 1)
//          sv0 >>= 16;
//      if (sop & 2)
//          sv1 >>= 16;
//      SET_DREG (dst0, (sv1 << 16) | (sv0 & 0xFFFF));
        tmp = tcg_temp_new();
        if (sop & 1)
            tcg_gen_shri_tl(tmp, cpu_dreg[src0], 16);
        else
            tcg_gen_andi_tl(tmp, cpu_dreg[src0], 0xffff);
        tmph = tcg_temp_new();
        if (sop & 2)
            tcg_gen_andi_tl(tmph, cpu_dreg[src1], 0xffff0000);
        else
            tcg_gen_shli_tl(tmph, cpu_dreg[src1], 16);
        tcg_gen_or_tl(cpu_dreg[dst0], tmph, tmp);
        tcg_temp_free(tmph);
        tcg_temp_free(tmp);
    } else if (sop == 0 && sopcde == 5) {
        /* Dreg_lo{dst0} = SIGNBITS Dreg{src1}; */
        tmp = tcg_temp_new();
        gen_signbitsi_tl(tmp, cpu_dreg[src1], 32);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if (sop == 1 && sopcde == 5) {
        /* Dreg_lo{dst0} = SIGNBITS Dreg_lo{src1}; */
        tmp = tcg_temp_new();
        gen_signbitsi_tl(tmp, cpu_dreg[src1], 16);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if (sop == 2 && sopcde == 5) {
        /* Dreg_lo{dst0} = SIGNBITS Dreg_hi{src1}; */
        tmp = tcg_temp_new();
        tcg_gen_shri_tl(tmp, cpu_dreg[src1], 16);
        gen_signbitsi_tl(tmp, tmp, 16);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if ((sop == 0 || sop == 1) && sopcde == 6) {
        /* Dreg_lo{dst0} = SIGNBITS Areg{sop}; */
        tmp = tcg_temp_new();
        gen_signbitsi_i64_i32(tmp, cpu_areg[sop], 40);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    } else if (sop == 3 && sopcde == 6) {
        /* Dreg_lo{dst0} = ONES Dreg{src1}; */
        tmp = tcg_temp_new();
        gen_helper_ones(tmp, cpu_dreg[src1]);
        gen_mov_l_tl(cpu_dreg[dst0], tmp);
        tcg_temp_free(tmp);
    }
#if 0
  else if (sop == 0 && sopcde == 7)
    {
      bu16 sv1 = (bu16)signbits (DREG (src1), 32);
      bu16 sv0 = (bu16)DREG (src0);
      bu16 dst_lo;

      TRACE_INSN (cpu, "R%i.L = EXPADJ (R%i, R%i.L);", dst0, src1, src0);

      if ((sv1 & 0x1f) < (sv0 & 0x1f))
	dst_lo = sv1;
      else
	dst_lo = sv0;
      STORE (DREG (dst0), REG_H_L (DREG (dst0), dst_lo));
    }
  else if (sop == 1 && sopcde == 7)
    {
      /*
       * Exponent adjust on two 16-bit inputs. Select smallest norm
       * among 3 inputs
       */
      bs16 src1_hi = (DREG (src1) & 0xFFFF0000) >> 16;
      bs16 src1_lo = (DREG (src1) & 0xFFFF);
      bu16 src0_lo = (DREG (src0) & 0xFFFF);
      bu16 tmp_hi, tmp_lo, tmp;

      TRACE_INSN (cpu, "R%i.L = EXPADJ (R%i, R%i.L) (V);", dst0, src1, src0);

      tmp_hi = signbits (src1_hi, 16);
      tmp_lo = signbits (src1_lo, 16);

      if ((tmp_hi & 0xf) < (tmp_lo & 0xf))
	if ((tmp_hi & 0xf) < (src0_lo & 0xf))
	  tmp = tmp_hi;
	else
	  tmp = src0_lo;
      else
	if ((tmp_lo & 0xf) < (src0_lo & 0xf))
	  tmp = tmp_lo;
	else
	  tmp = src0_lo;
      STORE (DREG (dst0), REG_H_L (DREG (dst0), tmp));
    }
  else if (sop == 2 && sopcde == 7)
    {
      /*
       * exponent adjust on single 16-bit register
       */
      bu16 tmp;
      bu16 src0_lo = (bu16)(DREG (src0) & 0xFFFF);

      TRACE_INSN (cpu, "R%i.L = EXPADJ (R%i.L, R%i.L);", dst0, src1, src0);

      tmp = signbits (DREG (src1) & 0xFFFF, 16);

      if ((tmp & 0xf) < (src0_lo & 0xf))
	SET_DREG_L (dst0, tmp);
      else
	SET_DREG_L (dst0, src0_lo);
    }
  else if (sop == 3 && sopcde == 7)
    {
      bu16 tmp;
      bu16 src0_lo = (bu16)(DREG (src0) & 0xFFFF);

      TRACE_INSN (cpu, "R%i.L = EXPADJ (R%i.H, R%i.L);", dst0, src1, src0);

      tmp = signbits ((DREG (src1) & 0xFFFF0000) >> 16, 16);

      if ((tmp & 0xf) < (src0_lo & 0xf))
	SET_DREG_L (dst0, tmp);
      else
	SET_DREG_L (dst0, src0_lo);
    }
  else if (sop == 0 && sopcde == 8)
    {
      bu64 acc = get_unextended_acc (cpu, 0);
      bu32 s0, s1;

      TRACE_INSN (cpu, "BITMUX (R%i, R%i, A0) (ASR);", src0, src1);

      if (src0 == src1)
	illegal_instruction_combination(dc);

      s0 = DREG (src0);
      s1 = DREG (src1);
      acc = (acc >> 2) |
	(((bu64)s0 & 1) << 38) |
	(((bu64)s1 & 1) << 39);
      SET_DREG (src0, s0 >> 1);
      SET_DREG (src1, s1 >> 1);

      SET_AREG (0, acc);
    }
  else if (sop == 1 && sopcde == 8)
    {
      bu64 acc = get_unextended_acc (cpu, 0);
      bu32 s0, s1;

      TRACE_INSN (cpu, "BITMUX (R%i, R%i, A0) (ASL);", src0, src1);

      if (src0 == src1)
	illegal_instruction_combination(dc);

      s0 = DREG (src0);
      s1 = DREG (src1);
      acc = (acc << 2) |
	((s0 >> 31) & 1) |
	((s1 >> 30) & 2);
      SET_DREG (src0, s0 << 1);
      SET_DREG (src1, s1 << 1);

      SET_AREG (0, acc);
    }
  else if ((sop == 0 || sop == 1) && sopcde == 9)
    {
      bs40 acc0 = get_extended_acc (cpu, 0);
      bs16 sL, sH, out;

      TRACE_INSN (cpu, "R%i.L = VIT_MAX (R%i) (AS%c);",
		  dst0, src1, sop & 1 ? 'R' : 'L');

      sL = DREG (src1);
      sH = DREG (src1) >> 16;

      if (sop & 1)
	acc0 >>= 1;
      else
	acc0 <<= 1;

      if (((sH - sL) & 0x8000) == 0)
	{
	  out = sH;
	  acc0 |= (sop & 1) ? 0x80000000 : 1;
	}
      else
	out = sL;

      SET_AREG (0, acc0);
      SET_DREG (dst0, REG_H_L (DREG (dst0), out));
    }
  else if ((sop == 2 || sop == 3) && sopcde == 9)
    {
      bs40 acc0 = get_extended_acc (cpu, 0);
      bs16 s0L, s0H, s1L, s1H, out0, out1;

      TRACE_INSN (cpu, "R%i = VIT_MAX (R%i, R%i) (AS%c);",
		  dst0, src1, src0, sop & 1 ? 'R' : 'L');

      s0L = DREG (src0);
      s0H = DREG (src0) >> 16;
      s1L = DREG (src1);
      s1H = DREG (src1) >> 16;

      if (sop & 1)
	acc0 >>= 2;
      else
	acc0 <<= 2;

      if (((s0H - s0L) & 0x8000) == 0)
	{
	  out0 = s0H;
	  acc0 |= (sop & 1) ? 0x40000000 : 2;
	}
      else
	out0 = s0L;

      if (((s1H - s1L) & 0x8000) == 0)
	{
	  out1 = s1H;
	  acc0 |= (sop & 1) ? 0x80000000 : 1;
	}
      else
	out1 = s1L;

      SET_AREG (0, acc0);
      SET_DREG (dst0, REG_H_L (out1 << 16, out0));
    }
#endif
    else if ((sop == 0 || sop == 1) && sopcde == 10) {
        TCGv mask, x, sgn;
        /* Dreg{dst0} = EXTRACT (Dreg{src1}, Dreg_lo{src0}) (X{sop==1}); */
        /* Dreg{dst0} = EXTRACT (Dreg{src1}, Dreg_lo{src0}) (Z{sop==0}); */

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
            int l;
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
    }
#if 0
    else if (sop == 0 && sopcde == 11) {
        bu64 acc0 = get_unextended_acc (cpu, 0);

        /* Dreg_lo{dst0} = CC = BXORSHIFT (A0, Dreg{src0}); */

        acc0 <<= 1;
        SET_CCREG (xor_reduce (acc0, DREG (src0)));
        SET_DREG (dst0, REG_H_L (DREG (dst0), CCREG));
        SET_AREG (0, acc0);
    } else if (sop == 1 && sopcde == 11) {
        bu64 acc0 = get_unextended_acc (cpu, 0);

        /* Dreg_lo{dst0} = CC = BXOR (A0, Dreg{src0}); */

        SET_CCREG (xor_reduce (acc0, DREG (src0)));
        SET_DREG (dst0, REG_H_L (DREG (dst0), CCREG));
    } else if (sop == 0 && sopcde == 12) {
        bu64 acc0 = get_unextended_acc (cpu, 0);
        bu64 acc1 = get_unextended_acc (cpu, 1);

        /* A0 = BXORSHIFT (A0, A1, CC); */

        acc0 = (acc0 << 1) | (CCREG ^ xor_reduce (acc0, acc1));
        SET_AREG (0, acc0);
    } else if (sop == 1 && sopcde == 12) {
        bu64 acc0 = get_unextended_acc (cpu, 0);
        bu64 acc1 = get_unextended_acc (cpu, 1);

        /* Dreg_lo{dst0} = CC = BXOR (A0, A1, CC); */

        SET_CCREG (CCREG ^ xor_reduce (acc0, acc1));
        acc0 = (acc0 << 1) | CCREG;
        SET_DREG (dst0, REG_H_L (DREG (dst0), CCREG));
    }
#endif
    else if ((sop == 0 || sop == 1 || sop == 2) && sopcde == 13) {
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
    } else
        illegal_instruction(dc);
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
    int immag    = ((iw1 >> DSP32ShiftImm_immag_bits) & DSP32ShiftImm_immag_mask);
    int newimmag = (-(iw1 >> DSP32ShiftImm_immag_bits) & DSP32ShiftImm_immag_mask);
    int dst0     = ((iw1 >> DSP32ShiftImm_dst0_bits) & DSP32ShiftImm_dst0_mask);
    int M        = ((iw0 >> (DSP32ShiftImm_M_bits - 16)) & DSP32ShiftImm_M_mask);
    int sopcde   = ((iw0 >> (DSP32ShiftImm_sopcde_bits - 16)) & DSP32ShiftImm_sopcde_mask);
    int HLs      = ((iw1 >> DSP32ShiftImm_HLs_bits) & DSP32ShiftImm_HLs_mask);
    TCGv tmp;

    TRACE_EXTRACT("%s: M:%i sopcde:%i sop:%i HLs:%i dst0:%i immag:%#x src1:%i",
                  __func__, M, sopcde, sop, HLs, dst0, immag, src1);

    if (sopcde == 0) {
        tmp = tcg_temp_new();

        if (HLs & 1) {
            if (sop == 0)
                tcg_gen_sari_tl(tmp, cpu_dreg[src1], 16);
            else
                tcg_gen_shri_tl(tmp, cpu_dreg[src1], 16);
        } else {
            if (sop == 0)
                tcg_gen_ext16s_tl(tmp, cpu_dreg[src1]);
            else
                tcg_gen_ext16u_tl(tmp, cpu_dreg[src1]);
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
        } else
            illegal_instruction(dc);

        if (HLs & 2)
            gen_mov_h_tl(cpu_dreg[dst0], tmp);
        else
            gen_mov_l_tl(cpu_dreg[dst0], tmp);

        tcg_temp_free(tmp);
#if 0
    } else if (sop == 2 && sopcde == 3 && (HLs == 1 || HLs == 0)) {
        int shift = imm6(immag);
//        bu32 cc = CCREG;
//        bu40 acc = get_unextended_acc (cpu, HLs);

        /* Areg{HLs} = ROT Areg{HLs} BY imm{immag}; */
/*
        acc = rot40 (acc, shift, &cc);
        SET_AREG (HLs, acc);
        if (shift)
            SET_CCREG (cc);
*/
        tcg_gen_rotri_i64(cpu_areg[HLs], cpu_areg[HLs], shift);
        if (shift) {
            //tcg_gen_
        }
#endif
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

        if (sop == 0)
            /* Areg{HLs} = Aregs{HLs} <<{sop} imm{immag}; */
            tcg_gen_shli_i64(cpu_areg[HLs], cpu_areg[HLs], shiftup);
        else
            /* Areg{HLs} = Aregs{HLs} >>{sop} imm{newimmag}; */
            tcg_gen_shri_i64(cpu_areg[HLs], cpu_areg[HLs], shiftdn);

//      SET_AREG (HLs, acc);
//      SET_ASTATREG (an, !!(acc & 0x8000000000ull));
//      SET_ASTATREG (az, acc == 0);
    }
#if 0
    else if (sop == 1 && sopcde == 1 && bit8 == 0) {
        int count = imm5(immag);
        bu16 val0 = DREG (src1) >> 16;
        bu16 val1 = DREG (src1) & 0xFFFF;
        bu32 astat;

        TRACE_INSN (cpu, "R%i = R%i << %i (V,S);", dst0, src1, count);
        val0 = lshift (cpu, val0, count, 16, 1);
        astat = ASTAT;
        val1 = lshift (cpu, val1, count, 16, 1);
        SET_ASTAT (ASTAT | astat);

        STORE (DREG (dst0), (val0 << 16) | val1);
    }
#endif
    else if (sop == 2 && sopcde == 1 && bit8 == 1) {
        int count = imm5(newimmag);
//      bu16 val0 = DREG (src1) & 0xFFFF;
//      bu16 val1 = DREG (src1) >> 16;
//      bu32 astat;

        /* Dreg{dst0} = Dreg{src1} >> imm{count} (V); */
/*
        val0 = lshiftrt (cpu, val0, count, 16);
        astat = ASTAT;
        val1 = lshiftrt (cpu, val1, count, 16);
        SET_ASTAT (ASTAT | astat);

        STORE (DREG (dst0), val0 | (val1 << 16));
*/
        if (count > 0 && count <= 15) {
            tcg_gen_shri_tl(cpu_dreg[dst0], cpu_dreg[src1], count);
            tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0],
                            0xffff0000 | ((1 << (16 - count)) - 1));
        } else if (count)
            tcg_gen_movi_tl(cpu_dreg[dst0], 0);
    } else if (sop == 2 && sopcde == 1 && bit8 == 0) {
        int count = imm5(immag);
//      bu16 val0 = DREG (src1) & 0xFFFF;
//      bu16 val1 = DREG (src1) >> 16;
//      bu32 astat;

        /* Dreg{dst0} = Dreg{src1} << imm{count} (V); */
/*
        val0 = lshift (cpu, val0, count, 16, 0);
        astat = ASTAT;
        val1 = lshift (cpu, val1, count, 16, 0);
        SET_ASTAT (ASTAT | astat);

        STORE (DREG (dst0), val0 | (val1 << 16));
*/
        /* XXX: No ASTAT handling */
        if (count > 0 && count <= 15) {
            tcg_gen_shli_tl(cpu_dreg[dst0], cpu_dreg[src1], count);
            tcg_gen_andi_tl(cpu_dreg[dst0], cpu_dreg[dst0],
                            ~(((1 << count) - 1) << 16));
        } else if (count)
            tcg_gen_movi_tl(cpu_dreg[dst0], 0);
    } else if (sopcde == 1 && (sop == 0 || (sop == 1 && bit8 == 1))) {
        int count = uimm5(newimmag);
//      bu16 val0 = DREG (src1) & 0xFFFF;
//      bu16 val1 = DREG (src1) >> 16;
//      bu32 astat;

        TRACE_INSN (cpu, "R%i = R%i >>> %i %s;", dst0, src1, count,
		  sop == 0 ? "(V)" : "(V,S)");

        if (sop == 1)
            unhandled_instruction(dc, "ashiftrt (S)");

/*
        val0 = ashiftrt (cpu, val0, count, 16);
        astat = ASTAT;
        val1 = ashiftrt (cpu, val1, count, 16);
        SET_ASTAT (ASTAT | astat);

        STORE (DREG (dst0), REG_H_L (val1 << 16, val0));
*/
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
        } else if (count)
            unhandled_instruction(dc, "ashiftrt (S)");
    } else if (sop == 1 && sopcde == 2) {
        int count = imm6(immag);

        /* Dreg{dst0} = Dreg{src1} << imm{count} (S); */
        //STORE (DREG (dst0), lshift (cpu, DREG (src1), count, 32, 1));
        tcg_gen_shli_tl(cpu_dreg[dst0], cpu_dreg[src1], -count);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst0]);
    } else if (sop == 2 && sopcde == 2) {
        int count = imm6(newimmag);

        /* Dreg{dst0} = Dreg{src1} >> imm{count}; */
        if (count < 0) {
            tcg_gen_shli_tl(cpu_dreg[dst0], cpu_dreg[src1], -count);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst0]);
        } else {
            tcg_gen_shri_tl(cpu_dreg[dst0], cpu_dreg[src1], count);
            astat_queue_state1(dc, ASTAT_OP_LSHIFT_RT32, cpu_dreg[dst0]);
        }
    } else if (sop == 3 && sopcde == 2) {
        int shift = imm6(immag);

/*
        bu32 src = DREG (src1);
        bu32 ret, cc = CCREG;

        ret = rot32 (src, shift, &cc);
        STORE (DREG (dst0), ret);
        if (shift)
            SET_CCREG (cc);
*/
        /* Dreg{dst0} = ROT Dreg{src1} BY imm{shift}; */
        tcg_gen_rotri_tl(cpu_dreg[dst0], cpu_dreg[src1], shift);
        if (shift) {
            tcg_gen_shli_tl(cpu_cc, cpu_cc, shift - 1);
            tcg_gen_or_tl(cpu_dreg[dst0], cpu_dreg[dst0], cpu_cc);
            tcg_gen_shri_tl(cpu_cc, cpu_dreg[src1], shift - 1);
            tcg_gen_andi_tl(cpu_cc, cpu_cc, 1);
        }
    } else if (sop == 0 && sopcde == 2) {
        int count = imm6(newimmag);

/*
        if (count < 0)
            STORE (DREG (dst0), lshift (cpu, DREG (src1), -count, 32, 0));
        else
            STORE (DREG (dst0), ashiftrt (cpu, DREG (src1), count, 32));
*/
        /* Dreg{dst0} = Dreg{src1} >>> imm{count}; */
        tcg_gen_sari_tl(cpu_dreg[dst0], cpu_dreg[src1], count);
        astat_queue_state1(dc, ASTAT_OP_LSHIFT32, cpu_dreg[dst0]);
    } else
        illegal_instruction(dc);
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

    TRACE_EXTRACT("%s: fn:%i grp:%i reg:%i", __func__, fn, grp, reg);

    if ((reg == 0 || reg == 1) && fn == 3)
        /* DBG Areg{reg}; */
        unhandled_instruction(dc, "DBG Areg");
    else if (reg == 3 && fn == 3)
        /* ABORT; */
        cec_exception(dc, EXCP_ABORT);
    else if (reg == 4 && fn == 3)
        /* HLT; */
        cec_exception(dc, EXCP_HLT);
    else if (reg == 5 && fn == 3)
        unhandled_instruction(dc, "DBGHALT");
    else if (reg == 6 && fn == 3)
        unhandled_instruction(dc, "DBGCMPLX (dregs)");
    else if (reg == 7 && fn == 3)
        unhandled_instruction(dc, "DBG");
    else if (grp == 0 && fn == 2)
        /* OUTC Dreg{reg}; */
        gen_helper_outc(cpu_dreg[reg]);
    else if (fn == 0)
        /* DBG allreg{grp,reg}; */
        unhandled_instruction(dc, "DBG allregs");
    else if (fn == 1)
        unhandled_instruction(dc, "PRNT allregs");
    else
        illegal_instruction(dc);
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

    TRACE_EXTRACT("%s: ch:%#x", __func__, ch);

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
    int expected = ((iw1 >> PseudoDbg_Assert_expected_bits) & PseudoDbg_Assert_expected_mask);
    int dbgop    = ((iw0 >> (PseudoDbg_Assert_dbgop_bits - 16)) & PseudoDbg_Assert_dbgop_mask);
    int grp      = ((iw0 >> (PseudoDbg_Assert_grp_bits - 16)) & PseudoDbg_Assert_grp_mask);
    int regtest  = ((iw0 >> (PseudoDbg_Assert_regtest_bits - 16)) & PseudoDbg_Assert_regtest_mask);
    const char *dbg_name, *dbg_appd;
    TCGv reg, exp, pc;

    TRACE_EXTRACT("%s: dbgop:%i grp:%i regtest:%i expected:%#x",
                  __func__, dbgop, grp, regtest, expected);

    if (dbgop == 0 || dbgop == 2) {
        /* DBGA (genreg_lo{grp,regtest}, imm{expected} */
        /* DBGAL (genreg{grp,regtest}, imm{expected} */
        dbg_name = dbgop == 0 ? "DBGA" : "DBGAL";
        dbg_appd = dbgop == 0 ? ".L" : "";
    } else if (dbgop == 1 || dbgop == 3) {
        /* DBGA (genreg_hi{grp,regtest}, imm{expected} */
        /* DBGAH (genreg{grp,regtest}, imm{expected} */
        dbg_name = dbgop == 1 ? "DBGA" : "DBGAH";
        dbg_appd = dbgop == 1 ? ".H" : "";
    } else
        illegal_instruction(dc);

    reg = get_allreg(dc, grp, regtest);
    exp = tcg_temp_new();
    tcg_gen_movi_tl(exp, expected);
    pc = tcg_const_tl(dc->pc);
    if (dbgop & 1)
        gen_helper_dbga_h(pc, reg, exp);
    else
        gen_helper_dbga_l(pc, reg, exp);
    tcg_temp_free(pc);
    tcg_temp_free(exp);
}

/* Interpret a single 16bit/32bit insn; no parallel insn handling */
static void
_interp_insn_bfin(DisasContext *dc, target_ulong pc)
{
    uint16_t iw0, iw1;

    iw0 = lduw_code(pc);
    if ((iw0 & 0xc000) != 0xc000) {
        /* 16-bit opcode */
        dc->insn_len = 2;

        TRACE_EXTRACT("%s: iw0:%#x", __func__, iw0);
        if ((iw0 & 0xFF00) == 0x0000)
            decode_ProgCtrl_0(dc, iw0);
        else if ((iw0 & 0xFFC0) == 0x0240)
            decode_CaCTRL_0(dc, iw0);
        else if ((iw0 & 0xFF80) == 0x0100)
            decode_PushPopReg_0(dc, iw0);
        else if ((iw0 & 0xFE00) == 0x0400)
            decode_PushPopMultiple_0(dc, iw0);
        else if ((iw0 & 0xFE00) == 0x0600)
            decode_ccMV_0(dc, iw0);
        else if ((iw0 & 0xF800) == 0x0800)
            decode_CCflag_0(dc, iw0);
        else if ((iw0 & 0xFFE0) == 0x0200)
            decode_CC2dreg_0(dc, iw0);
        else if ((iw0 & 0xFF00) == 0x0300)
            decode_CC2stat_0(dc, iw0);
        else if ((iw0 & 0xF000) == 0x1000)
            decode_BRCC_0(dc, iw0);
        else if ((iw0 & 0xF000) == 0x2000)
            decode_UJUMP_0(dc, iw0);
        else if ((iw0 & 0xF000) == 0x3000)
            decode_REGMV_0(dc, iw0);
        else if ((iw0 & 0xFC00) == 0x4000)
            decode_ALU2op_0(dc, iw0);
        else if ((iw0 & 0xFE00) == 0x4400)
            decode_PTR2op_0(dc, iw0);
        else if ((iw0 & 0xF800) == 0x4800)
            decode_LOGI2op_0(dc, iw0);
        else if ((iw0 & 0xF000) == 0x5000)
            decode_COMP3op_0(dc, iw0);
        else if ((iw0 & 0xF800) == 0x6000)
            decode_COMPI2opD_0(dc, iw0);
        else if ((iw0 & 0xF800) == 0x6800)
            decode_COMPI2opP_0(dc, iw0);
        else if ((iw0 & 0xF000) == 0x8000)
            decode_LDSTpmod_0(dc, iw0);
        else if ((iw0 & 0xFF60) == 0x9E60)
            decode_dagMODim_0(dc, iw0);
        else if ((iw0 & 0xFFF0) == 0x9F60)
            decode_dagMODik_0(dc, iw0);
        else if ((iw0 & 0xFC00) == 0x9C00)
            decode_dspLDST_0(dc, iw0);
        else if ((iw0 & 0xF000) == 0x9000)
            decode_LDST_0(dc, iw0);
        else if ((iw0 & 0xFC00) == 0xB800)
            decode_LDSTiiFP_0(dc, iw0);
        else if ((iw0 & 0xE000) == 0xA000)
            decode_LDSTii_0(dc, iw0);
        else {
            TRACE_EXTRACT("%s: no matching 16-bit pattern", __func__);
            illegal_instruction(dc);
        }
        return;
    }

    /* Grab the next 16 bits to determine if it's a 32-bit or 64-bit opcode */
    iw1 = lduw_code(pc + 2);
    if ((iw0 & BIT_MULTI_INS) && (iw0 & 0xe800) != 0xe800 /* not linkage */)
        dc->insn_len = 8;
    else
        dc->insn_len = 4;

    TRACE_EXTRACT("%s: iw0:%#x iw1:%#x insn_len:%i", __func__,
                  iw0, iw1, dc->insn_len);

    if ((iw0 & 0xf7ff) == 0xc003 && iw1 == 0x1800)
        /* MNOP; */;
    else if (((iw0 & 0xFF80) == 0xE080) && ((iw1 & 0x0C00) == 0x0000))
        decode_LoopSetup_0(dc, iw0, iw1);
    else if (((iw0 & 0xFF00) == 0xE100) && ((iw1 & 0x0000) == 0x0000))
        decode_LDIMMhalf_0(dc, iw0, iw1);
    else if (((iw0 & 0xFE00) == 0xE200) && ((iw1 & 0x0000) == 0x0000))
        decode_CALLa_0(dc, iw0, iw1);
    else if (((iw0 & 0xFC00) == 0xE400) && ((iw1 & 0x0000) == 0x0000))
        decode_LDSTidxI_0(dc, iw0, iw1);
    else if (((iw0 & 0xFFFE) == 0xE800) && ((iw1 & 0x0000) == 0x0000))
        decode_linkage_0(dc, iw0, iw1);
    else if (((iw0 & 0xF600) == 0xC000) && ((iw1 & 0x0000) == 0x0000))
        decode_dsp32mac_0(dc, iw0, iw1);
    else if (((iw0 & 0xF600) == 0xC200) && ((iw1 & 0x0000) == 0x0000))
        decode_dsp32mult_0(dc, iw0, iw1);
    else if (((iw0 & 0xF7C0) == 0xC400) && ((iw1 & 0x0000) == 0x0000))
        decode_dsp32alu_0(dc, iw0, iw1);
    else if (((iw0 & 0xF7E0) == 0xC600) && ((iw1 & 0x01C0) == 0x0000))
        decode_dsp32shift_0(dc, iw0, iw1);
    else if (((iw0 & 0xF7E0) == 0xC680) && ((iw1 & 0x0000) == 0x0000))
        decode_dsp32shiftimm_0(dc, iw0, iw1);
    else if ((iw0 & 0xFF00) == 0xF800)
        decode_psedoDEBUG_0(dc, iw0), dc->insn_len = 2;
    else if ((iw0 & 0xFF00) == 0xF900)
        decode_psedoOChar_0(dc, iw0), dc->insn_len = 2;
    else if (((iw0 & 0xFF00) == 0xF000) && ((iw1 & 0x0000) == 0x0000))
        decode_psedodbg_assert_0(dc, iw0, iw1);
    else {
        TRACE_EXTRACT("%s: no matching 32-bit pattern", __func__);
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
        /* Reset back for higher levels to process branches */
        dc->insn_len = 8;
    }
}
