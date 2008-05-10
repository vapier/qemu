/* Common Blackfin device stuff.

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

#ifndef DEVICES_H
#define DEVICES_H

#define BFIN_MMR_16(mmr) mmr, __pad_##mmr

static inline bu16 dv_load_2 (const void *ptr)
{
  const unsigned char *c = ptr;
  return (c[1] << 8) | (c[0]);
}

static inline void dv_store_2 (void *ptr, bu16 val)
{
  unsigned char *c = ptr;
  c[1] = val >> 8;
  c[0] = val;
}

static inline bu32 dv_load_4 (const void *ptr)
{
  const unsigned char *c = ptr;
  return (c[3] << 24) | (c[2] << 16) | dv_load_2 (ptr);
}

static inline void dv_store_4 (void *ptr, bu32 val)
{
  unsigned char *c = ptr;
  c[3] = val >> 24;
  c[2] = val >> 16;
  dv_store_2 (ptr, val);
}

static inline void *
dv_get_state (SIM_CPU *cpu, const char *device_name)
{
/*
  SIM_DESC sd = CPU_STATE (cpu);
  void *root = STATE_HW (sd);
  struct hw *hw = hw_tree_find_device (root, device_name);
  return hw_data (hw);
*/
  return NULL;
}

#define dv_bfin_hw_parse(sd, dv, DV) \
  do { \
    bu32 base = BFIN_COREMMR_##DV##_BASE; \
    bu32 size = BFIN_COREMMR_##DV##_SIZE; \
    sim_hw_parse (sd, "/core/bfin_"#dv"/reg %#x %i", base, size); \
  } while (0)

#define dv_bfin_invalid_mmr(hw, addr, size) \
  hw_abort (hw, "invalid MMR access %i bytes @%#x", size, addr)

#define dv_bfin_require_16(hw, addr, size) \
  do { \
    if ((size) != 2) \
      dv_bfin_invalid_mmr (hw, addr, size); \
  } while (0)

#endif
