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
    cpu_exec_init(env);
}

static void bfin_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    BlackfinCPUClass *bcc = BFIN_CPU_GET_CLASS(dev);

    cpu_reset(cs);
    qemu_init_vcpu(cs);

    bcc->parent_realize(dev, errp);
}

static gint bfin_cpu_name_compare(gconstpointer a, gconstpointer b)
{
    const BlackfinCPUClass *bcc = BFIN_CPU_CLASS(a);
    const char *name = b;

    return strcasecmp(bcc->name, name);
}

static ObjectClass *bfin_cpu_class_by_name(const char *cpu_model)
{
    ObjectClass *oc;
    GSList *list, *item;

    if (cpu_model == NULL) {
        return NULL;
    }
    if (strcasecmp(cpu_model, "any") == 0) {
        return object_class_by_name("bf537");
    }

    oc = object_class_by_name(cpu_model);
    if (oc != NULL && object_class_dynamic_cast(oc, TYPE_BLACKFIN_CPU) != NULL
        && !object_class_is_abstract(oc)) {
        return oc;
    }

    oc = NULL;
    list = object_class_get_list(TYPE_BLACKFIN_CPU, false);
    item = g_slist_find_custom(list, cpu_model, bfin_cpu_name_compare);
    if (item != NULL) {
        oc = item->data;
    }
    g_slist_free(list);
    return oc;
}

static const VMStateDescription vmstate_bfin_cpu = {
    .name = "cpu",
    .unmigratable = 1,
};

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
#ifndef CONFIG_USER_ONLY
    cc->get_phys_page_debug = bfin_cpu_get_phys_page_debug;
#endif
    dc->vmsd = &vmstate_bfin_cpu;
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

static void bf5xx_cpu_initfn(Object *obj)
{
    BlackfinCPU *cpu = BFIN_CPU(obj);
abort();
}

static void bf5xx_class_init(ObjectClass *oc, void *data)
{
    BlackfinCPUClass *bcc = BFIN_CPU_CLASS(oc);
abort();
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

static void bfin_cpu_register(const BlackfinCPUInfo *info)
{
    TypeInfo type_info = {
        .name = info->name,
        .parent = TYPE_BLACKFIN_CPU,
        .instance_size = sizeof(BlackfinCPU),
        .instance_init = info->initfn,
        .class_init = bf5xx_class_init,
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
