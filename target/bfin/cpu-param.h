/*
 * Blackfin emulation
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#ifndef BFIN_CPU_PARAM_H
#define BFIN_CPU_PARAM_H 1

#define TARGET_LONG_BITS 32
/* Blackfin does 1K/4K/1M/4M, but for now only support 4k */
#define TARGET_PAGE_BITS 12
#define NB_MMU_MODES     2
#define TARGET_PHYS_ADDR_SPACE_BITS 32
#define TARGET_VIRT_ADDR_SPACE_BITS 32

#endif
