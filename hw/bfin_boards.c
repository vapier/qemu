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

static void bf537_stamp_init(ram_addr_t ram_size_not_used,
                             const char *boot_device,
                             const char *kernel_filename,
                             const char *kernel_cmdline,
                             const char *initrd_filename, const char *cpu_model)
{
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
