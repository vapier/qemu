/*
 * QEMU Blackfin CPU
 *
 * Copyright 2007-2014 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#ifndef QEMU_BFIN_CPU_QOM_H
#define QEMU_BFIN_CPU_QOM_H

#include "qom/cpu.h"

#define TYPE_BLACKFIN_CPU "bfin-cpu"

#define BFIN_CPU_CLASS(klass) \
    OBJECT_CLASS_CHECK(BlackfinCPUClass, (klass), TYPE_BLACKFIN_CPU)
#define BFIN_CPU(obj) \
    OBJECT_CHECK(BlackfinCPU, (obj), TYPE_BLACKFIN_CPU)
#define BFIN_CPU_GET_CLASS(obj) \
    OBJECT_GET_CLASS(BlackfinCPUClass, (obj), TYPE_BLACKFIN_CPU)

/**
 * BlackfinCPUClass:
 * @parent_reset: The parent class' reset handler.
 *
 * A Blackfin CPU model.
 */
typedef struct BlackfinCPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    void (*parent_reset)(CPUState *cpu);
} BlackfinCPUClass;

/**
 * BlackfinCPU:
 * @env: #CPUArchState
 *
 * A Blackfin CPU.
 */
typedef struct BlackfinCPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUArchState env;
} BlackfinCPU;

static inline BlackfinCPU *bfin_env_get_cpu(CPUArchState *env)
{
    return container_of(env, BlackfinCPU, env);
}

#define ENV_GET_CPU(e) CPU(bfin_env_get_cpu(e))

#define ENV_OFFSET offsetof(BlackfinCPU, env)

void bfin_cpu_do_interrupt(CPUState *cpu);

void bfin_cpu_dump_state(CPUState *cs, FILE *f, fprintf_function cpu_fprintf,
                         int flags);
int bfin_cpu_gdb_read_register(CPUState *cpu, uint8_t *buf, int reg);
int bfin_cpu_gdb_write_register(CPUState *cpu, uint8_t *buf, int reg);

#endif
