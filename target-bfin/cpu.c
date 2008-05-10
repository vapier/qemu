/*
 * QEMU Blackfin CPU
 *
 * Copyright 2007-2015 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "cpu.h"
#include "qemu-common.h"
#include "migration/vmstate.h"

static void bfin_cpu_set_pc(CPUState *cs, vaddr value)
{
    BlackfinCPU *cpu = BFIN_CPU(cs);

    cpu->env.pc = value;
}

static bool bfin_cpu_has_work(CPUState *cpu)
{
    return cpu->interrupt_request & (CPU_INTERRUPT_HARD | CPU_INTERRUPT_NMI);
}

/* CPUClass::reset() */
static void bfin_cpu_reset(CPUState *cs)
{
    BlackfinCPU *cpu = BFIN_CPU(cs);
    BlackfinCPUClass *bcc = BFIN_CPU_GET_CLASS(cpu);
    CPUArchState *env = &cpu->env;

    bcc->parent_reset(cs);

    tlb_flush(cs, 1);

    env->pc = 0xEF000000;
}

static void bfin_cpu_initfn(Object *obj)
{
    CPUState *cs = CPU(obj);
    BlackfinCPU *cpu = BFIN_CPU(obj);
    CPUArchState *env = &cpu->env;

    cs->env_ptr = env;
    cpu_exec_init(cs, &error_abort);

    if (tcg_enabled()) {
        bfin_translate_init();
    }
}

static void bfin_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    BlackfinCPUClass *bcc = BFIN_CPU_GET_CLASS(dev);

    cpu_reset(cs);
    qemu_init_vcpu(cs);

    bcc->parent_realize(dev, errp);
}

static ObjectClass *bfin_cpu_class_by_name(const char *cpu_model)
{
    ObjectClass *oc;

    if (!cpu_model) {
        return NULL;
    }

    oc = object_class_by_name(cpu_model);
    if (!oc || !object_class_dynamic_cast(oc, TYPE_BLACKFIN_CPU) ||
        object_class_is_abstract(oc)) {
        return NULL;
    }
    return oc;
}

static void bfin_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    BlackfinCPUClass *bcc = BFIN_CPU_CLASS(oc);

    bcc->parent_realize = dc->realize;
    dc->realize = bfin_cpu_realizefn;

    bcc->parent_reset = cc->reset;
    cc->reset = bfin_cpu_reset;

    cc->class_by_name = bfin_cpu_class_by_name;
    cc->has_work = bfin_cpu_has_work;
    cc->do_interrupt = bfin_cpu_do_interrupt;
    cc->set_pc = bfin_cpu_set_pc;
    cc->gdb_read_register = bfin_cpu_gdb_read_register;
    cc->gdb_write_register = bfin_cpu_gdb_write_register;
    cc->dump_state = bfin_cpu_dump_state;
}

static const TypeInfo bfin_cpu_type_info = {
    .name = TYPE_BLACKFIN_CPU,
    .parent = TYPE_CPU,
    .instance_size = sizeof(BlackfinCPU),
    .instance_init = bfin_cpu_initfn,
    .abstract = true,
    .class_size = sizeof(BlackfinCPUClass),
    .class_init = bfin_cpu_class_init,
};

static void bf5xx_cpu_initfn(Object *obj)
{
}

typedef struct BlackfinCPUInfo {
    const char *name;
    void (*initfn)(Object *obj);
    void (*class_init)(ObjectClass *oc, void *data);
} BlackfinCPUInfo;

static const BlackfinCPUInfo bfin_cpus[] = {
    { .name = "any",   .initfn = bf5xx_cpu_initfn, },
};

static void bfin_cpu_register(const BlackfinCPUInfo *info)
{
    TypeInfo type_info = {
        .name = info->name,
        .parent = TYPE_BLACKFIN_CPU,
        .instance_size = sizeof(BlackfinCPU),
        .instance_init = info->initfn,
        .class_init = info->class_init,
        .class_size = sizeof(BlackfinCPUClass),
    };

    type_register_static(&type_info);
}

static void bfin_cpu_register_types(void)
{
    size_t i;

    type_register_static(&bfin_cpu_type_info);
    for (i = 0; i < ARRAY_SIZE(bfin_cpus); i++) {
        bfin_cpu_register(&bfin_cpus[i]);
    }
}

type_init(bfin_cpu_register_types)
