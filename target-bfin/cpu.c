/*
 * QEMU Blackfin CPU
 *
 * Copyright 2007-2013 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "cpu.h"
#include "qemu-common.h"


/* CPUClass::reset() */
static void bfin_cpu_reset(CPUState *s)
{
    BfinCPU *cpu = BFIN_CPU(s);
    CPUArchState *env = &cpu->env;

    env->pc = 0xEF000000;
}

static void bfin_cpu_initfn(Object *obj)
{
    CPUState *cs = CPU(obj);
    BfinCPU *cpu = BFIN_CPU(obj);
    CPUArchState *env = &cpu->env;

    cs->env_ptr = env;
    cpu_exec_init(env);
}

static void bfin_cpu_class_init(ObjectClass *oc, void *data)
{
    CPUClass *cc = CPU_CLASS(oc);

    cc->reset = bfin_cpu_reset;
}

static const TypeInfo bfin_cpu_type_info = {
    .name = TYPE_BFIN_CPU,
    .parent = TYPE_CPU,
    .instance_size = sizeof(BfinCPU),
    .instance_init = bfin_cpu_initfn,
    .abstract = false,
    .class_size = sizeof(BfinCPUClass),
    .class_init = bfin_cpu_class_init,
};

static void bfin_cpu_register_types(void)
{
    type_register_static(&bfin_cpu_type_info);
}

type_init(bfin_cpu_register_types)
