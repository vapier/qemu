/*
 * Blackfin helpers
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"

#include "cpu.h"
#include "exec/exec-all.h"
#include "qemu/host-utils.h"

#if defined(CONFIG_USER_ONLY)

void bfin_cpu_do_interrupt(CPUState *cs)
{
    cs->exception_index = -1;
}

int cpu_handle_mmu_fault(CPUState *cs, target_ulong address,
                         MMUAccessType access_type, int mmu_idx)
{
    cs->exception_index = EXCP_DCPLB_VIOLATE;
    return 1;
}

#else

void bfin_cpu_do_interrupt(CPUState *cs)
{
    BlackfinCPU *cpu = BFIN_CPU(cs);
    CPUBfinState *env = &cpu->env;

    qemu_log_mask(CPU_LOG_INT,
        "exception at pc=%x type=%x\n", env->pc, cs->exception_index);

    switch (cs->exception_index) {
    default:
        cpu_abort(cs, "unhandled exception type=%d\n",
                  cs->exception_index);
        break;
    }
}

hwaddr bfin_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    return addr & TARGET_PAGE_MASK;
}

int cpu_handle_mmu_fault(CPUState *cs, target_ulong address,
                         MMUAccessType access_type, int mmu_idx)
{
    int prot;

    address &= TARGET_PAGE_MASK;
    prot = PAGE_BITS;
    tlb_set_page(cs, address, address, prot, mmu_idx, TARGET_PAGE_SIZE);

    return 0;
}

#endif

/* Sort alphabetically by type name, except for "any". */
static gint cpu_list_compare(gconstpointer a, gconstpointer b)
{
    ObjectClass *class_a = (ObjectClass *)a;
    ObjectClass *class_b = (ObjectClass *)b;
    const char *name_a, *name_b;

    name_a = object_class_get_name(class_a);
    name_b = object_class_get_name(class_b);
    if (strcmp(name_a, "any") == 0) {
        return 1;
    } else if (strcmp(name_b, "any") == 0) {
        return -1;
    } else {
        return strcmp(name_a, name_b);
    }
}

static void cpu_list_entry(gpointer data, gpointer user_data)
{
    ObjectClass *oc = data;
    CPUListState *s = user_data;

    (*s->cpu_fprintf)(s->file, "  %s\n",
                      object_class_get_name(oc));
}

void cpu_bfin_list(FILE *f, fprintf_function cpu_fprintf)
{
    CPUListState s = {
        .file = f,
        .cpu_fprintf = cpu_fprintf,
    };
    GSList *list;

    list = object_class_get_list(TYPE_BLACKFIN_CPU, false);
    list = g_slist_sort(list, cpu_list_compare);
    (*cpu_fprintf)(f, "Available CPUs:\n");
    g_slist_foreach(list, cpu_list_entry, &s);
    g_slist_free(list);
}
