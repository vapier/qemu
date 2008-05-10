/* Blackfin Memory Management Unit (MMU) model.

   Copyright (C) 2010 Free Software Foundation, Inc.
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef DV_BFIN_MMU_H
#define DV_BFIN_MMU_H

#define BFIN_COREMMR_MMU_BASE	0xFFE00000
#define BFIN_COREMMR_MMU_SIZE	0x2000

void mmu_check_addr (SIM_CPU *, bu32 addr, bool write, bool inst, int size);
void mmu_process_fault (SIM_CPU *, bu32 addr, bool write, bool inst, bool unaligned);

#endif
