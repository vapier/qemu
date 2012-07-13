#ifndef QEMU_ARCH_INIT_H
#define QEMU_ARCH_INIT_H

enum {
    QEMU_ARCH_ALL        = -1,
    QEMU_ARCH_ALPHA      = 1 << 0,
    QEMU_ARCH_ARM        = 1 << 1,
    QEMU_ARCH_BFIN       = 1 << 2,
    QEMU_ARCH_CRIS       = 1 << 3,
    QEMU_ARCH_I386       = 1 << 4,
    QEMU_ARCH_M68K       = 1 << 5,
    QEMU_ARCH_LM32       = 1 << 6,
    QEMU_ARCH_MICROBLAZE = 1 << 7,
    QEMU_ARCH_MIPS       = 1 << 8,
    QEMU_ARCH_PPC        = 1 << 9,
    QEMU_ARCH_S390X      = 1 << 10,
    QEMU_ARCH_SH4        = 1 << 11,
    QEMU_ARCH_SPARC      = 1 << 12,
    QEMU_ARCH_XTENSA     = 1 << 13,
};

extern const uint32_t arch_type;

void select_soundhw(const char *optarg);
void do_acpitable_option(const char *optarg);
void do_smbios_option(const char *optarg);
void cpudef_init(void);
int audio_available(void);
void audio_init(ISABus *isa_bus, PCIBus *pci_bus);
int tcg_available(void);
int kvm_available(void);
int xen_available(void);

#endif
