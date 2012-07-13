/*
 * QEMU Blackfin CPU
 *
 * Copyright 2007-2014 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "cpu.h"
#include "qemu-common.h"

static void bfin_cpu_set_pc(CPUState *cs, vaddr value)
{
    BlackfinCPU *cpu = BFIN_CPU(cs);

    cpu->env.pc = value;
}

/* CPUClass::reset() */
static void bfin_cpu_reset(CPUState *s)
{
    BlackfinCPU *cpu = BFIN_CPU(s);
    CPUArchState *env = &cpu->env;
    memset(env, 0, offsetof(CPUBfinState, breakpoints));

    env->pc = 0xEF000000;
}

static void bfin_cpu_initfn(Object *obj)
{
    CPUState *cs = CPU(obj);
    BlackfinCPU *cpu = BFIN_CPU(obj);
    CPUArchState *env = &cpu->env;

    cs->env_ptr = env;
    cpu_exec_init(env);
}

static void bfin_cpu_class_init(ObjectClass *oc, void *data)
{
    CPUClass *cc = CPU_CLASS(oc);

    cc->do_interrupt = bfin_cpu_do_interrupt;
    cc->reset = bfin_cpu_reset;
    cc->set_pc = bfin_cpu_set_pc;
    cc->gdb_read_register = bfin_cpu_gdb_read_register;
    cc->gdb_write_register = bfin_cpu_gdb_write_register;
    cc->dump_state = bfin_cpu_dump_state;
#ifndef CONFIG_USER_ONLY
    cc->get_phys_page_debug = bfin_cpu_get_phys_page_debug;
#endif
}

static const TypeInfo bfin_cpu_type_info = {
    .name = TYPE_BLACKFIN_CPU,
    .parent = TYPE_CPU,
    .instance_size = sizeof(BlackfinCPU),
    .instance_init = bfin_cpu_initfn,
    .abstract = false,
    .class_size = sizeof(BlackfinCPUClass),
    .class_init = bfin_cpu_class_init,
};

static void bfin_cpu_register_types(void)
{
    type_register_static(&bfin_cpu_type_info);
}

type_init(bfin_cpu_register_types)
