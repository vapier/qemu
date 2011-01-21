/*
 * Blackfin helpers
 *
 * Copyright 2007 Mike Frysinger
 * Copyright 2007 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "exec.h"
#include "helper.h"

void helper_raise_exception(uint32_t index, uint32_t pc)
{
    env->exception_index = index;
    if (pc != -1)
        env->pc = pc;
    cpu_loop_exit();
}

void helper_dbga_l(uint32_t pc, uint32_t actual, uint32_t expected)
{
    if ((actual & 0xffff) != expected)
        helper_raise_exception(EXCP_DBGA, pc);
}

void helper_dbga_h(uint32_t pc, uint32_t actual, uint32_t expected)
{
    if ((actual >> 16) != expected)
        helper_raise_exception(EXCP_DBGA, pc);
}

void helper_outc(uint32_t ch)
{
    putc(ch, stdout);
    if (ch == '\n')
        fflush(stdout);
}

void helper_dbg(uint32_t reg)
{
    printf("DBG: %#x\n", reg);
}

uint32_t helper_astat_load(void)
{
    return bfin_astat_read(env);
}

void helper_astat_store(uint32_t astat)
{
    bfin_astat_write(env, astat);
}

/* Count the number of bits set to 1 in the 32bit value */
uint32_t helper_ones(uint32_t val)
{
    uint32_t i;
    uint32_t ret;

    ret = 0;
    for (i = 0; i < 32; ++i)
        ret += !!(val & (1 << i));

    return ret;
}

/* Count number of leading bits that match the sign bit */
uint32_t helper_signbits(uint32_t val, uint32_t size)
{
    uint32_t mask = 1 << (size - 1);
    uint32_t bit = val & mask;
    uint32_t count = 0;

    for (;;) {
        mask >>= 1;
        bit >>= 1;
        if (mask == 0)
            break;
        if ((val & mask) != bit)
            break;
        ++count;
    }

    return count;
}
