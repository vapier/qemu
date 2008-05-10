/* Blackfin Core Event Controller (CEC) model.

   Copyright (C) 2010 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "sim-main.h"
#include "devices.h"
#include "dv-bfin_cec.h"
#include "dv-bfin_evt.h"

struct bfin_cec
{
  bu32 base;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 evt_override, imask, ipend, ilat, iprio;
};
#define mmr_base()      offsetof(struct bfin_cec, evt_override)
#define mmr_offset(mmr) (offsetof(struct bfin_cec, mmr) - mmr_base())

static void
_cec_imask_write (struct bfin_cec *cec, bu32 value)
{
  cec->imask = (value & IVG_MASKABLE_B) | (cec->imask & IVG_UNMASKABLE_B);
}
#if 0
static unsigned
bfin_cec_io_write_buffer (struct hw *me, const void *source,
			  int space, unsigned_word addr, unsigned nr_bytes)
{
  struct bfin_cec *cec = hw_data (me);
  bu32 value;

  value = dv_load_4(source);

  HW_TRACE ((me, "write to 0x%08lx length %d with 0x%x", (long) addr,
	     (int) nr_bytes, value));

  switch (addr - cec->base)
    {
    case mmr_offset(evt_override):
      cec->evt_override = value;
      break;
    case mmr_offset(imask):
      _cec_imask_write (cec, value);
      break;
    case mmr_offset(ipend):
    case mmr_offset(ilat):
      /* read-only registers */
      break;
    case mmr_offset(iprio):
      cec->iprio = (value & IVG_UNMASKABLE_B);
      break;
    }

  return nr_bytes;
}

static unsigned
bfin_cec_io_read_buffer (struct hw *me, void *dest,
			 int space, unsigned_word addr, unsigned nr_bytes)
{
  struct bfin_cec *cec = hw_data (me);
  bu32 *value;

  HW_TRACE ((me, "read 0x%08lx length %d", (long) addr, (int) nr_bytes));

  value = &cec->evt_override + (addr - cec->base) / 4;
  dv_store_4 (dest, *value);

  return nr_bytes;
}

static void
attach_bfin_cec_regs (struct hw *me, struct bfin_cec *cec)
{
  unsigned_word attach_address;
  int attach_space;
  unsigned attach_size;
  reg_property_spec reg;

  if (hw_find_property (me, "reg") == NULL)
    hw_abort (me, "Missing \"reg\" property");

  if (!hw_find_reg_array_property (me, "reg", 0, &reg))
    hw_abort (me, "\"reg\" property must contain three addr/size entries");

  hw_unit_address_to_attach_address (hw_parent (me),
				     &reg.address,
				     &attach_space, &attach_address, me);
  hw_unit_size_to_attach_size (hw_parent (me), &reg.size, &attach_size, me);

  if (attach_size != BFIN_COREMMR_CEC_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_COREMMR_CEC_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  cec->base = attach_address;
}

static void
bfin_cec_finish (struct hw *me)
{
  struct bfin_cec *cec;

  cec = HW_ZALLOC (me, struct bfin_cec);

  set_hw_data (me, cec);
  set_hw_io_read_buffer (me, bfin_cec_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_cec_io_write_buffer);

  attach_bfin_cec_regs (me, cec);

  /* Initialize the CEC.  */
  cec->imask = IVG_UNMASKABLE_B;
  cec->ipend = IVG_RST_B | IVG_IRPTEN_B;
}

const struct hw_descriptor dv_bfin_cec_descriptor[] = {
  {"bfin_cec", bfin_cec_finish,},
  {NULL},
};
#endif

struct bfin_cec cec_state;
#define CEC_STATE(cpu) &cec_state
//#define CEC_STATE(cpu) ((struct bfin_cec *) dv_get_state (cpu, "/core/bfin_cec"))

#define __cec_get_ivg(cec, mmr) (ffs ((cec)->mmr & ~IVG_IRPTEN_B) - 1)
#define _cec_get_ivg(cec) __cec_get_ivg (cec, ipend)

int
cec_get_ivg (SIM_CPU *cpu)
{
  return _cec_get_ivg (CEC_STATE (cpu));
}

static bool
_cec_is_supervisor_mode (struct bfin_cec *cec)
{
  return (cec->ipend & ~(IVG_EMU_B | IVG_IRPTEN_B));
}
bool
cec_is_supervisor_mode (SIM_CPU *cpu)
{
  if (STATE_ENVIRONMENT (CPU_STATE (cpu)) == OPERATING_ENVIRONMENT)
    return _cec_is_supervisor_mode (CEC_STATE (cpu));
  else
    return true;
}
static bool
_cec_is_user_mode (struct bfin_cec *cec)
{
  return !_cec_is_supervisor_mode (cec);
}
bool
cec_is_user_mode (SIM_CPU *cpu)
{
  if (STATE_ENVIRONMENT (CPU_STATE (cpu)) == OPERATING_ENVIRONMENT)
    return !cec_is_supervisor_mode (cpu);
  else
    return false;
}
static void
_cec_require_supervisor (SIM_CPU *cpu, struct bfin_cec *cec)
{
  if (_cec_is_user_mode (cec))
    cec_exception (cpu, VEC_ILL_RES);
}
void
cec_require_supervisor (SIM_CPU *cpu)
{
  _cec_require_supervisor (cpu, CEC_STATE (cpu));
}

static void _cec_raise (SIM_CPU *, struct bfin_cec *, int);

#define excp_to_sim_halt(reason, sigrc) \
  sim_engine_halt (CPU_STATE (cpu), cpu, NULL, PCREG, reason, sigrc)
void
cec_exception (SIM_CPU *cpu, int excp)
{
//  SIM_DESC sd = CPU_STATE (cpu);
  int sigrc = -1;

  TRACE_EVENTS (cpu, "processing exception %#x in EVT%i", excp,
		cec_get_ivg (cpu));

  /* Ideally what would happen here for real hardware exceptions (not
   * fake sim ones) is that:
   *  - For service exceptions (excp <= 0x11):
   *     RETX is the _next_ PC which can be tricky with jumps/hardware loops/...
   *  - For error exceptions (excp > 0x11):
   *     RETX is the _current_ PC (i.e. the one causing the exception)
   *  - PC is loaded with EVT3 MMR
   *  - ILAT/IPEND in CEC is updated depending on current IVG level
   *  - the fault address MMRs get updated with data/instruction info
   *  - Execution continues on in the EVT3 handler
   */

  /* Handle simulator exceptions first.  */
  switch (excp)
    {
    case VEC_SIM_HLT:
      excp_to_sim_halt (sim_exited, 0);
      return;
    case VEC_SIM_ABORT:
      excp_to_sim_halt (sim_exited, 1);
      return;
    }

  if (excp <= 0x3f)
    {
      SET_SEQSTATREG ((SEQSTATREG & ~0x3f) | excp);
      if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
	{
	  _cec_raise (cpu, CEC_STATE (cpu), IVG_EVX);
	  return;
	}
    }

  TRACE_EVENTS (cpu, "running virtual exception handler");

  switch (excp)
    {
    case VEC_SYS:
      bfin_trap (cpu);
      break;

    case VEC_EXCPT01:	/* userspace gdb breakpoint */
      sigrc = SIM_SIGTRAP;
      break;

    case VEC_UNDEF_I:	/* undefined instruction */
      sigrc = SIM_SIGILL;
      break;

    case VEC_ILL_RES:	/* illegal supervisor resource */
    case VEC_MISALI_I:	/* misaligned instruction */
      sigrc = SIM_SIGBUS;
      break;

    case VEC_CPLB_M:
    case VEC_CPLB_I_M:
      sigrc = SIM_SIGSEGV;
      break;

    default:
      sim_io_eprintf (sd, "Unhandled exception %#x at 0x%08x\n", excp, PCREG);
      sigrc = SIM_SIGILL;
      break;
    }

  if (sigrc != -1)
    excp_to_sim_halt (sim_stopped, sigrc);
}

bu32 cec_cli (SIM_CPU *cpu)
{
  struct bfin_cec *cec = CEC_STATE (cpu);
  bu32 old_mask;

  _cec_require_supervisor (cpu, cec);

  /* XXX: what about IPEND[4] ?  */
  old_mask = cec->imask;
  _cec_imask_write (cec, 0);

  TRACE_EVENTS (cpu, "CLI changed IMASK from %#x to %#x", old_mask, cec->imask);

  return old_mask;
}

void cec_sti (SIM_CPU *cpu, bu32 ints)
{
  struct bfin_cec *cec = CEC_STATE (cpu);
  bu32 old_mask;

  _cec_require_supervisor (cpu, cec);

  /* XXX: what about IPEND[4] ?  */
  old_mask = cec->imask;
  _cec_imask_write (cec, ints);

  TRACE_EVENTS (cpu, "STI changed IMASK from %#x to %#x", old_mask, cec->imask);
}

static void
cec_irpten_enable (SIM_CPU *cpu, struct bfin_cec *cec)
{
  /* Globally mask interrupts.  */
  TRACE_EVENTS (cpu, "setting IPEND[4] to globally mask interrupts");
  cec->ipend |= IVG_IRPTEN_B;
}

static void
cec_irpten_disable (SIM_CPU *cpu, struct bfin_cec *cec)
{
  /* Clear global interrupt mask.  */
  TRACE_EVENTS (cpu, "clearing IPEND[4] to not globally mask interrupts");
  cec->ipend &= ~IVG_IRPTEN_B;
}

static void
_cec_raise (SIM_CPU *cpu, struct bfin_cec *cec, int ivg)
{
//  SIM_DESC sd = CPU_STATE (cpu);
  int curr_ivg = _cec_get_ivg (cec);
  bool snen;
  bool irpten;

  TRACE_EVENTS (cpu, "processing request for EVT%i while at EVT%i",
		ivg, curr_ivg);

  irpten = (cec->ipend & IVG_IRPTEN_B);
  snen = (SYSCFGREG & SYSCFG_SNEN);

  if (curr_ivg == -1)
    curr_ivg = IVG_USER;

  /* Just check for higher latched interrupts.  */
  if (ivg == -1)
    {
      if (irpten)
	goto done; /* All interrupts are masked anyways.  */

      ivg = __cec_get_ivg (cec, ilat);
      if (ivg < 0)
	goto done; /* Nothing latched.  */

      if (ivg > curr_ivg)
	goto done; /* Nothing higher latched.  */

      if (!snen && ivg == curr_ivg)
	goto done; /* Self nesting disabled.  */

      /* Still here, so fall through to raise to higher pending.  */
    }

  cec->ilat |= (1 << ivg);

  if (ivg == IVG_EMU || ivg == IVG_RST)
    goto process_int;
  if (ivg <= IVG_EVX && curr_ivg <= IVG_EVX && ivg >= curr_ivg)
    {
      /* Double fault ! :( */
      /* XXX: should there be some status registers we update ? */
      sim_io_error (sd, "%s: double fault at 0x%08x ! :(", __func__, PCREG);
      excp_to_sim_halt (sim_stopped, SIM_SIGABRT);
    }
  else if (irpten && curr_ivg != IVG_USER)
    {
      /* Interrupts are globally masked.  */
    }
  else if (!(cec->imask & (1 << ivg)))
    {
      /* This interrupt is masked.  */
    }
  else if (ivg < curr_ivg || (snen && ivg == curr_ivg))
    {
      /* Do transition! */
      bu32 oldpc;

 process_int:
      cec->ipend |= (1 << ivg);
      cec->ilat &= ~(1 << ivg);

      switch (ivg)
	{
	case IVG_EMU:
	  /* Signal the JTAG ICE.  */
	  /* XXX: what happens with 'raise 0' ?  */
	  SET_RETEREG (PCREG);
	  excp_to_sim_halt (sim_stopped, SIM_SIGTRAP);
	  break;
	case IVG_RST:
	  /* XXX: this should reset the core.  */
	  sim_io_error (sd, "%s: raise 1 (core reset) not implemented", __func__);
	  break;
	case IVG_NMI:
	  SET_RETNREG (PCREG);
	  break;
	case IVG_EVX:
	  SET_RETXREG (PCREG);
	  break;
	case IVG_IRPTEN:
	  /* XXX: what happens with 'raise 4' ?  */
	  sim_io_error (sd, "%s: what to do with 'raise 4' ?", __func__);
	  break;
	default:
	  SET_RETIREG (PCREG | (ivg == curr_ivg ? 1 : 0));
	  break;
	}

      oldpc = PCREG;

      /* If EVT_OVERRIDE is in effect, use the reset address.  */
      if ((cec->evt_override & 0x1ff) & (1 << ivg))
	SET_PCREG (cec_get_reset_evt (cpu));
      else
	SET_PCREG (cec_get_evt (cpu, ivg));

      TRACE_BRANCH (cpu, "CEC changed PC (to EVT%i): %#x -> %#x",
		    ivg, oldpc, PCREG);

      /* Enable the global interrupt mask upon interrupt entry.  */
      cec_irpten_enable (cpu, cec);
    }

  /* When going from user to super, we set LSB in LB regs to avoid
     misbehavior and/or malicious code.  */
  if (curr_ivg == IVG_USER)
    {
      int i;
      for (i = 0; i < 2; ++i)
	if (!(LBREG (i) & 1))
	  SET_LBREG (i, LBREG (i) | 1);
    }

 done:
  TRACE_EVENTS (cpu, "now at EVT%i", _cec_get_ivg (cec));
}

void
cec_raise (SIM_CPU *cpu, int ivg)
{
  struct bfin_cec *cec = CEC_STATE (cpu);

  if (ivg > IVG15 || ivg < -1)
    sim_io_error (CPU_STATE (cpu), "%s: ivg %i out of range !", __func__, ivg);

  _cec_require_supervisor (cpu, cec);

  _cec_raise (cpu, cec, ivg);
}

void
cec_return (SIM_CPU *cpu, int ivg)
{
  struct bfin_cec *cec = CEC_STATE (cpu);
  int curr_ivg;
  bu32 oldpc;

  /* XXX: This isn't entirely correct ...  */
  cec->ipend &= ~IVG_EMU_B;

  curr_ivg = _cec_get_ivg (cec);
  if (ivg == -1)
    ivg = curr_ivg;

  TRACE_EVENTS (cpu, "returning from EVT%i (should be EVT%i)", curr_ivg, ivg);

  if (ivg > IVG15 || ivg < 0)
    sim_io_error (CPU_STATE (cpu), "%s: ivg %i out of range !", __func__, ivg);

  _cec_require_supervisor (cpu, cec);

  oldpc = PCREG;

  switch (ivg)
    {
    case IVG_EMU:
      /* RTE -- only valid in emulation mode.  */
      /* XXX: What does the hardware do ?  */
      if (curr_ivg != IVG_EMU)
	cec_exception (cpu, VEC_ILL_RES);
      SET_PCREG (RETEREG);
      break;
    case IVG_NMI:
      /* RTN -- only valid in NMI.  */
      /* XXX: What does the hardware do ?  */
      if (curr_ivg != IVG_NMI)
	cec_exception (cpu, VEC_ILL_RES);
      SET_PCREG (RETNREG);
      break;
    case IVG_EVX:
      /* RTX -- only valid in exception.  */
      /* XXX: What does the hardware do ?  */
      if (curr_ivg != IVG_EVX)
	cec_exception (cpu, VEC_ILL_RES);
      SET_PCREG (RETXREG);
      break;
    default:
      /* RTI -- not valid in emulation, nmi, exception, or user.  */
      /* XXX: What does the hardware do ?  */
      if (curr_ivg < IVG_IVHW && curr_ivg != IVG_RST)
	cec_exception (cpu, VEC_ILL_RES);
      SET_PCREG (RETIREG);
      break;
    case IVG_IRPTEN:
      /* XXX: Is this even possible ?  */
      excp_to_sim_halt (sim_stopped, SIM_SIGABRT);
      break;
    }

  /* XXX: Does this nested trick work on EMU/NMI/EVX ?  */
  if (PCREG & 1)
    /* XXX: Delayed clear shows bad PCREG register trace above ?  */
    PCREG &= ~1;
  else
    cec->ipend &= ~(1 << ivg);

  TRACE_BRANCH (cpu, "CEC changed PC (from EVT%i): %#x -> %#x",
		ivg, oldpc, PCREG);

  /* Disable global interrupt mask to let any interrupt take over.  */
  cec_irpten_disable (cpu, cec);

  /* When going from super to user, we clear LSB in LB regs in case
     it was set on the transition up.  */
  if (_cec_get_ivg (cec) == -1)
    {
      int i;
      for (i = 0; i < 2; ++i)
	if (LBREG (i) & 1)
	  SET_LBREG (i, LBREG (i) & ~1);
    }

  /* Check for pending interrupts before we return to usermode.  */
  _cec_raise (cpu, cec, -1);
}

void
cec_push_reti (SIM_CPU *cpu)
{
  /* XXX: Need to check hardware with popped RETI value
     and bit 1 is set (when handling nested interrupts).
     Also need to check behavior wrt SNEN in SYSCFG.  */
  struct bfin_cec *cec = CEC_STATE (cpu);
  TRACE_EVENTS (cpu, "pushing RETI");
  cec_irpten_disable (cpu, cec);
  /* Check for pending interrupts.  */
  _cec_raise (cpu, cec, -1);
}

void
cec_pop_reti (SIM_CPU *cpu)
{
  /* XXX: Need to check hardware with popped RETI value
     and bit 1 is set (when handling nested interrupts).
     Also need to check behavior wrt SNEN in SYSCFG.  */
  struct bfin_cec *cec = CEC_STATE (cpu);
  TRACE_EVENTS (cpu, "popping RETI");
  cec_irpten_enable (cpu, cec);
}
