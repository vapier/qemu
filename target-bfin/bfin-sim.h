/* Simulator for Analog Devices Blackfin processers.

   Copyright (C) 2005-2010 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#ifndef _BFIN_SIM_H_
#define _BFIN_SIM_H_

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t bu8;
typedef uint16_t bu16;
typedef uint32_t bu32;
typedef uint64_t bu40;
typedef uint64_t bu64;
typedef int8_t bs8;
typedef int16_t bs16;
typedef int32_t bs32;
typedef int64_t bs40;
typedef int64_t bs64;

/* For dealing with parallel instructions, we must avoid changing our register
   file until all parallel insns have been simulated.  This queue of stores
   can be used to delay a modification.
   XXX: Should go and convert all 32 bit insns to use this.  */
struct store {
  bu32 *addr;
  bu32 val;
};

#define DREG(x)		dc->env->dreg[x]
#define PREG(x)		dc->env->preg[x]
#define SPREG		PREG (6)
#define FPREG		PREG (7)

#if 0
/* The KSP/USP handling wrt SP may not follow the hardware exactly (the hw
   looks at current mode and uses either SP or USP based on that.  We instead
   always operate on SP and mirror things in KSP and USP.  During a CEC
   transition, we take care of syncing the values.  This lowers the simulation
   complexity and speeds things up a bit.  */
struct bfin_cpu_state
{
  bu32 dpregs[16], iregs[4], mregs[4], bregs[4], lregs[4], cycles[3];
  bu32 ax[2], aw[2];
  bu32 lt[2], lc[2], lb[2];
  bu32 ksp, usp, seqstat, syscfg, rets, reti, retx, retn, rete;
  bu32 pc, emudat[2];
  /* These ASTAT flags need not be bu32, but it makes pointers easier.  */
  bu32 ac0, ac0_copy, ac1, an, aq;
  union { struct { bu32 av0;  bu32 av1;  }; bu32 av [2]; };
  union { struct { bu32 av0s; bu32 av1s; }; bu32 avs[2]; };
  bu32 az, cc, v, v_copy, vs;
  bu32 rnd_mod;
  bu32 v_internal;
  bu32 astat_reserved;

  /* Set by an instruction emulation function if we performed a jump.  We
     cannot compare oldpc to newpc as this ignores the "jump 0;" case.  */
  bool did_jump;

  /* Used by the CEC to figure out where to return to.  */
  bu32 insn_len;

  /* How many cycles did this insn take to complete ?  */
  bu32 cycle_delay;

  /* The pc currently being interpreted in parallel insns.  */
  bu32 multi_pc;

  /* Needed for supporting the DISALGNEXCPT instruction */
  int dis_algn_expt;

  /* See notes above for struct store.  */
  struct store stores[20];
  int n_stores;

#if (WITH_HW)
  /* Cache heavily used CPU-specific device pointers.  */
  void *cec_cache;
  void *evt_cache;
  void *mmu_cache;
  void *trace_cache;
#endif
};

#define REG_H_L(h, l)	(((h) & 0xffff0000) | ((l) & 0x0000ffff))

#define DREG(x)		(BFIN_CPU_STATE.dpregs[x])
#define PREG(x)		(BFIN_CPU_STATE.dpregs[x + 8])
#define SPREG		PREG (6)
#define FPREG		PREG (7)
#define IREG(x)		(BFIN_CPU_STATE.iregs[x])
#define MREG(x)		(BFIN_CPU_STATE.mregs[x])
#define BREG(x)		(BFIN_CPU_STATE.bregs[x])
#define LREG(x)		(BFIN_CPU_STATE.lregs[x])
#define AXREG(x)	(BFIN_CPU_STATE.ax[x])
#define AWREG(x)	(BFIN_CPU_STATE.aw[x])
#define CCREG		(BFIN_CPU_STATE.cc)
#define LCREG(x)	(BFIN_CPU_STATE.lc[x])
#define LTREG(x)	(BFIN_CPU_STATE.lt[x])
#define LBREG(x)	(BFIN_CPU_STATE.lb[x])
#define CYCLESREG	(BFIN_CPU_STATE.cycles[0])
#define CYCLES2REG	(BFIN_CPU_STATE.cycles[1])
#define CYCLES2SHDREG	(BFIN_CPU_STATE.cycles[2])
#define KSPREG		(BFIN_CPU_STATE.ksp)
#define USPREG		(BFIN_CPU_STATE.usp)
#define SEQSTATREG	(BFIN_CPU_STATE.seqstat)
#define SYSCFGREG	(BFIN_CPU_STATE.syscfg)
#define RETSREG		(BFIN_CPU_STATE.rets)
#define RETIREG		(BFIN_CPU_STATE.reti)
#define RETXREG		(BFIN_CPU_STATE.retx)
#define RETNREG		(BFIN_CPU_STATE.retn)
#define RETEREG		(BFIN_CPU_STATE.rete)
#define PCREG		(BFIN_CPU_STATE.pc)
#define EMUDAT_INREG	(BFIN_CPU_STATE.emudat[0])
#define EMUDAT_OUTREG	(BFIN_CPU_STATE.emudat[1])
#define INSN_LEN	(BFIN_CPU_STATE.insn_len)
#define CYCLE_DELAY	(BFIN_CPU_STATE.cycle_delay)
#define DIS_ALGN_EXPT	(BFIN_CPU_STATE.dis_algn_expt)

#define EXCAUSE_SHIFT		0
#define EXCAUSE_MASK		(0x3f << EXCAUSE_SHIFT)
#define EXCAUSE			((SEQSTATREG & EXCAUSE_MASK) >> EXCAUSE_SHIFT)
#define HWERRCAUSE_SHIFT	14
#define HWERRCAUSE_MASK		(0x1f << HWERRCAUSE_SHIFT)
#define HWERRCAUSE		((SEQSTATREG & HWERRCAUSE_MASK) >> HWERRCAUSE_SHIFT)

#define _SET_CORE32REG_IDX(reg, p, x, val) \
  do { \
    bu32 __v = (val); \
    TRACE_REGISTER (cpu, "wrote "#p"%i = %#x", x, __v); \
    reg = __v; \
  } while (0)
#define SET_DREG(x, val) _SET_CORE32REG_IDX (DREG (x), R, x, val)
#define SET_PREG(x, val) _SET_CORE32REG_IDX (PREG (x), P, x, val)
#define SET_IREG(x, val) _SET_CORE32REG_IDX (IREG (x), I, x, val)
#define SET_MREG(x, val) _SET_CORE32REG_IDX (MREG (x), M, x, val)
#define SET_BREG(x, val) _SET_CORE32REG_IDX (BREG (x), B, x, val)
#define SET_LREG(x, val) _SET_CORE32REG_IDX (LREG (x), L, x, val)
#define SET_LCREG(x, val) _SET_CORE32REG_IDX (LCREG (x), LC, x, val)
#define SET_LTREG(x, val) _SET_CORE32REG_IDX (LTREG (x), LT, x, val)
#define SET_LBREG(x, val) _SET_CORE32REG_IDX (LBREG (x), LB, x, val)

#define SET_DREG_L_H(x, l, h) SET_DREG (x, REG_H_L (h, l))
#define SET_DREG_L(x, l) SET_DREG (x, REG_H_L (DREG (x), l))
#define SET_DREG_H(x, h) SET_DREG (x, REG_H_L (h, DREG (x)))

#define _SET_CORE32REG_ALU(reg, p, x, val) \
  do { \
    bu32 __v = (val); \
    TRACE_REGISTER (cpu, "wrote A%i"#p" = %#x", x, __v); \
    reg = __v; \
  } while (0)
#define SET_AXREG(x, val) _SET_CORE32REG_ALU (AXREG (x), X, x, val)
#define SET_AWREG(x, val) _SET_CORE32REG_ALU (AWREG (x), W, x, val)

#define SET_AREG(x, val) \
  do { \
    bu40 __a = (val); \
    SET_AXREG (x, (__a >> 32) & 0xff); \
    SET_AWREG (x, __a); \
  } while (0)
#define SET_AREG32(x, val) \
  do { \
    SET_AWREG (x, val); \
    SET_AXREG (x, -(AWREG (x) >> 31)); \
  } while (0)

#define _SET_CORE32REG(reg, val) \
  do { \
    bu32 __v = (val); \
    TRACE_REGISTER (cpu, "wrote "#reg" = %#x", __v); \
    reg##REG = __v; \
  } while (0)
#define SET_FPREG(val) _SET_CORE32REG (FP, val)
#define SET_SPREG(val) _SET_CORE32REG (SP, val)
#define SET_CYCLESREG(val) _SET_CORE32REG (CYCLES, val)
#define SET_CYCLES2REG(val) _SET_CORE32REG (CYCLES2, val)
#define SET_CYCLES2SHDREG(val) _SET_CORE32REG (CYCLES2SHD, val)
#define SET_KSPREG(val) _SET_CORE32REG (KSP, val)
#define SET_USPREG(val) _SET_CORE32REG (USP, val)
#define SET_SYSCFGREG(val) _SET_CORE32REG (SYSCFG, val)
#define SET_RETSREG(val) _SET_CORE32REG (RETS, val)
#define SET_RETIREG(val) _SET_CORE32REG (RETI, val)
#define SET_RETXREG(val) _SET_CORE32REG (RETX, val)
#define SET_RETNREG(val) _SET_CORE32REG (RETN, val)
#define SET_RETEREG(val) _SET_CORE32REG (RETE, val)
#define SET_PCREG(val) _SET_CORE32REG (PC, val)

#define _SET_CORE32REGFIELD(reg, field, val, mask, shift) \
  do { \
    bu32 __f = (val); \
    bu32 __v = ((reg##REG) & ~(mask)) | (__f << (shift)); \
    TRACE_REGISTER (cpu, "wrote "#field" = %#x ("#reg" = %#x)", __f, __v); \
    reg##REG = __v; \
  } while (0)
#define SET_SEQSTATREG(val)   _SET_CORE32REG (SEQSTAT, val)
#define SET_EXCAUSE(excp)     _SET_CORE32REGFIELD (SEQSTAT, EXCAUSE, excp, EXCAUSE_MASK, EXCAUSE_SHIFT)
#define SET_HWERRCAUSE(hwerr) _SET_CORE32REGFIELD (SEQSTAT, HWERRCAUSE, hwerr, HWERRCAUSE_MASK, HWERRCAUSE_SHIFT)
#endif

//extern void bfin_syscall (SIM_CPU *);
//extern bu32 hwloop_get_next_pc (SIM_CPU *, bu32, bu32);

#define PROFILE_COUNT_INSN(...)
#define _TRACE_STUB(cpu, fmt, args...) do { if (0) printf (fmt, ## args); } while (0)
#define TRACE_INSN(cpu, fmt, args...) do { if (0) qemu_log_mask(CPU_LOG_TB_IN_ASM, fmt "\n", ## args); } while (0)
#define TRACE_DECODE(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_EXTRACT(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_CORE(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_EVENTS(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_BRANCH(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_REGISTER(...) _TRACE_STUB(__VA_ARGS__)

#define BFIN_L1_CACHE_BYTES       32

#endif
