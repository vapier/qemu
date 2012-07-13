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

static void bf5xx_cpu_initfn(Object *obj)
{
    BlackfinCPU *cpu = BFIN_CPU(obj);
}

typedef struct BlackfinCPUInfo {
    const char *name;
    void (*initfn)(Object *obj);
} BlackfinCPUInfo;

static const BlackfinCPUInfo bfin_cpus[] = {
    { .name = "bf504", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf506", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf512", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf514", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf516", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf518", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf522", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf523", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf524", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf525", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf526", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf527", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf531", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf532", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf533", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf534", .initfn = bf5xx_cpu_initfn, },
  /*{ .name = "bf535", .initfn = bf5xx_cpu_initfn, },*/
    { .name = "bf536", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf537", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf538", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf539", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf542", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf544", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf547", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf548", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf549", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf561", .initfn = bf5xx_cpu_initfn, },
    { .name = "bf592", .initfn = bf5xx_cpu_initfn, },
};

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

static void bfin_cpu_register(const BlackfinCPUInfo *info)
{
    TypeInfo type_info = {
        .name = info->name,
        .parent = TYPE_BLACKFIN_CPU,
        .instance_size = sizeof(BlackfinCPU),
        .instance_init = info->initfn,
        .class_size = sizeof(BlackfinCPUClass),
    };

    type_register_static(&type_info);
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
    size_t i;

    type_register_static(&bfin_cpu_type_info);
    for (i = 0; i < ARRAY_SIZE(bfin_cpus); i++) {
        bfin_cpu_register(&bfin_cpus[i]);
    }
}

type_init(bfin_cpu_register_types)
