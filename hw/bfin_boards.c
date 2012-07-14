/*
 * Blackfin board models
 *
 * Copyright 2007-2012 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#include "hw.h"
#include "boards.h"
#include "loader.h"
#include "elf.h"
#include "exec-memory.h"

struct bfin_memory_layout {
    target_phys_addr_t addr;
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

static void bfin_memory_init(const char *name, target_phys_addr_t base, ram_addr_t size)
{
    MemoryRegion *address_space_mem =  get_system_memory();
    MemoryRegion *mem = g_new(MemoryRegion, 1);

    memory_region_init_ram(mem, name, size);
    vmstate_register_ram_global(mem);
    memory_region_add_subregion(address_space_mem, base, mem);
}

static void bfin_common_init(const struct bfin_memory_layout mem_layout[],
                             ram_addr_t ram_size, const char *cpu_model,
                             const char *kernel_filename, const char *kernel_cmdline)
{
    CPUArchState *env;
    size_t i;

    for (i = 0; mem_layout[i].len; ++i) {
        bfin_memory_init(mem_layout[i].name, mem_layout[i].addr, mem_layout[i].len);
    }
    ram_size *= 1024 * 1024;
    bfin_memory_init(mem_layout[i].name, mem_layout[i].addr, ram_size);

    env = cpu_init(cpu_model);

    if (kernel_filename) {
        uint64_t entry;
        long kernel_size;

        kernel_size = load_elf(kernel_filename, NULL, NULL, &entry, NULL, NULL,
                               0, ELF_MACHINE, 0);
        if (kernel_size < 0) {
            kernel_size = load_image_targphys(kernel_filename, 0, ram_size);
        }
        if (kernel_size < 0) {
            fprintf(stderr, "qemu: could not load kernel '%s'\n",
                    kernel_filename);
            exit(1);
        }
        env->pc = entry;
    }

    if (kernel_cmdline)
        /* XXX: todo */{}
}

static void bf537_stamp_init(ram_addr_t ram_size_not_used,
                             const char *boot_device,
                             const char *kernel_filename,
                             const char *kernel_cmdline,
                             const char *initrd_filename, const char *cpu_model)
{
    bfin_common_init(bf537_mem, 64, "bf537", kernel_filename, kernel_cmdline);
}

static QEMUMachine bf537_stamp_machine = {
    .name = "bf537-stamp",
    .desc = "Analog Devices Blackfin 537 STAMP",
    .init = bf537_stamp_init,
    .is_default = 1
};

static void bfin_boards_machine_init(void)
{
    qemu_register_machine(&bf537_stamp_machine);
}

machine_init(bfin_boards_machine_init);
