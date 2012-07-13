/*
 * QEMU Blackfin CPU
 *
 * Copyright 2007-2012 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "cpu-qom.h"
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
    BfinCPU *cpu = BFIN_CPU(obj);
    CPUArchState *env = &cpu->env;

    cpu_exec_init(env);
}

static void bf5xx_cpu_initfn(Object *obj)
{
    BfinCPU *cpu = BFIN_CPU(obj);
}

typedef struct BfinCPUInfo {
    const char *name;
    void (*initfn)(Object *obj);
} BfinCPUInfo;

static const BfinCPUInfo bfin_cpus[] = {
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

    cc->reset = bfin_cpu_reset;
}

static void bfin_cpu_register(const BfinCPUInfo *info)
{
    TypeInfo type_info = {
        .name = info->name,
        .parent = TYPE_BFIN_CPU,
        .instance_size = sizeof(BfinCPU),
        .instance_init = info->initfn,
        .class_size = sizeof(BfinCPUClass),
    };

    type_register_static(&type_info);
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
    size_t i;

    type_register_static(&bfin_cpu_type_info);
    for (i = 0; i < ARRAY_SIZE(bfin_cpus); i++) {
        bfin_cpu_register(&bfin_cpus[i]);
    }
}

type_init(bfin_cpu_register_types)
