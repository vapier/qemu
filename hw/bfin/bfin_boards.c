/*
 * Blackfin board models
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"
#include "hw/hw.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "elf.h"
#include "hw/sysbus.h"
#include "sysemu/char.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"

struct bfin_memory_layout {
    hwaddr addr;
    ram_addr_t len;
    const char *name;
};
#define LAYOUT(_addr, _len, _name) { .addr = _addr, .len = _len, .name = _name, }

static const struct bfin_memory_layout bf537_mem[] =
{
    LAYOUT(0xFF800000, 0x4000, "L1 Data A"),
    LAYOUT(0xFF804000, 0x4000, "Data A Cache"),
    LAYOUT(0xFF900000, 0x4000, "Data B"),
    LAYOUT(0xFF904000, 0x4000, "Data B Cache"),
    LAYOUT(0xFFA00000, 0x8000, "Inst A"),
    LAYOUT(0xFFA08000, 0x4000, "Inst B"),
    LAYOUT(0xFFA10000, 0x4000, "Inst Cache"),
    LAYOUT(0, 0, "SDRAM"),
};

static void bfin_memory_init(const struct bfin_memory_layout mem_layout[], ram_addr_t ram_size)
{
    MemoryRegion *address_space_mem = get_system_memory();
    MemoryRegion *mem;
    size_t i;

    for (i = 0; mem_layout[i].len; ++i) {
        mem = g_new(MemoryRegion, 1);
        memory_region_init_ram(mem, NULL, mem_layout[i].name, mem_layout[i].len,
                               &error_abort);
        vmstate_register_ram_global(mem);
        memory_region_add_subregion(address_space_mem, mem_layout[i].addr, mem);
    }

    if (ram_size) {
        mem = g_new(MemoryRegion, 1);
        memory_region_init_ram(mem, NULL, mem_layout[i].name, ram_size,
                               &error_abort);
        vmstate_register_ram_global(mem);
        memory_region_add_subregion(address_space_mem, mem_layout[i].addr, mem);
    }

    /* Address space reserved for on-chip (system) devices */
    mem = g_new(MemoryRegion, 1);
    memory_region_init_reservation(mem, NULL, "System MMRs", 0x200000);
    memory_region_add_subregion(address_space_mem, 0xFFC00000, mem);

    /* Address space reserved for on-chip (core) devices */
    mem = g_new(MemoryRegion, 1);
    memory_region_init_reservation(mem, NULL, "Core MMRs", 0x200000);
    memory_region_add_subregion(address_space_mem, 0xFFE00000, mem);
}

static void bfin_device_init(void)
{
    /* Core peripherals */
    sysbus_create_simple("bfin_mmu", 0xFFE00000, NULL);
    sysbus_create_simple("bfin_evt", 0xFFE02000, NULL);
    sysbus_create_simple("bfin_trace", 0xFFE06000, NULL);

    /* System peripherals */
    /* XXX: BF537-specific */
    sysbus_create_simple("bfin_pll", 0xFFC00000, NULL);
    sysbus_create_simple("bfin_sic", 0xFFC00100, NULL);
    qemu_chr_new("bfin_uart0", "null");
    sysbus_create_simple("bfin_uart", 0xFFC00400, NULL);
    qemu_chr_new("bfin_uart1", "null");
    sysbus_create_simple("bfin_uart", 0xFFC02000, NULL);
}

static void bfin_common_init(const struct bfin_memory_layout mem_layout[],
                             ram_addr_t ram_size, const char *cpu_model,
                             const char *kernel_filename, const char *kernel_cmdline)
{
    CPUState *cs;
    int n;

    ram_size *= 1024 * 1024;
    bfin_memory_init(mem_layout, ram_size);
    bfin_device_init();

    for (n = 0; n < smp_cpus; n++) {
        cs = cpu_init(cpu_model);
        if (cs == NULL) {
            fprintf(stderr, "Unable to find CPU definition!\n");
            exit(1);
        }
    }

    if (kernel_filename) {
        uint64_t entry;
        long kernel_size;
        /* TODO: Not SMP safe.  */
        CPUArchState *env = cs->env_ptr;

        kernel_size = load_elf(kernel_filename, NULL, NULL, &entry, NULL, NULL,
                               0, ELF_MACHINE, 0, 0);
        if (kernel_size < 0) {
            kernel_size = load_image_targphys(kernel_filename, 0, ram_size);
        }
        if (kernel_size < 0) {
            fprintf(stderr, "qemu: could not load kernel '%s'\n",
                    kernel_filename);
            exit(1);
        }
        env->pc = entry;

        if (kernel_cmdline) {
            pstrcpy_targphys("cmdline", kernel_size, -1, kernel_cmdline);
        }
    }
}

static void bf537_stamp_init(MachineState *machine)
{
    bfin_common_init(bf537_mem, 64, "bf537", machine->kernel_filename,
                     machine->kernel_cmdline);
}

static void bf537_stamp_machine_init(MachineClass *mc)
{
    mc->desc = "Analog Devices Blackfin ADSP-BF537 STAMP";
    mc->init = bf537_stamp_init;
    mc->max_cpus = 1;
    mc->is_default = 1;
}
DEFINE_MACHINE("bf537-stamp", bf537_stamp_machine_init)

static void bf537_ezkit_machine_init(MachineClass *mc)
{
    mc->desc = "Analog Devices Blackfin ADSP-BF537 EZ-KIT";
    mc->init = bf537_stamp_init;
    mc->max_cpus = 1;
    mc->is_default = 0;
}
DEFINE_MACHINE("bf537-ezkit", bf537_ezkit_machine_init)
