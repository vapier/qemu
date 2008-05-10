/*
 * QEMU Blackfin CPU
 *
 * Copyright 2007-2013 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#ifndef QEMU_BFIN_CPU_QOM_H
#define QEMU_BFIN_CPU_QOM_H

#include "qom/cpu.h"

#define TYPE_BFIN_CPU "bfin-cpu"

#define BFIN_CPU_CLASS(klass) \
    OBJECT_CLASS_CHECK(BfinCPUClass, (klass), TYPE_BFIN_CPU)
#define BFIN_CPU(obj) \
    OBJECT_CHECK(BfinCPU, (obj), TYPE_BFIN_CPU)
#define BFIN_CPU_GET_CLASS(obj) \
    OBJECT_GET_CLASS(BfinCPUClass, (obj), TYPE_BFIN_CPU)

/**
 * BfinCPUClass:
 * @parent_reset: The parent class' reset handler.
 *
 * An Bfin CPU model.
 */
typedef struct BfinCPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    void (*parent_reset)(CPUState *cpu);
} BfinCPUClass;

/**
 * BfinCPU:
 * @env: #CPUArchState
 *
 * An Bfin CPU.
 */
typedef struct BfinCPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUArchState env;
} BfinCPU;

static inline BfinCPU *bfin_env_get_cpu(CPUArchState *env)
{
    return BFIN_CPU(container_of(env, BfinCPU, env));
}

#define ENV_GET_CPU(e) CPU(bfin_env_get_cpu(e))

#define ENV_OFFSET offsetof(BfinCPU, env)

#endif
