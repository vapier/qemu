/*
 * uClinux flat-format executables
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2003-2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2
 */

#define FLAT_BFIN_RELOC_TYPE_16_BIT  0
#define FLAT_BFIN_RELOC_TYPE_16H_BIT 1
#define FLAT_BFIN_RELOC_TYPE_32_BIT  2

#define flat_argvp_envp_on_stack()                0
#define flat_reloc_valid(reloc, size)             ((reloc) <= (size))
#define flat_old_ram_flag(flag)                   (flag)
#define flat_get_relocate_addr(relval)            ((relval) & 0x03ffffff)

static inline int flat_set_persistent(abi_ulong relval, abi_ulong *persistent)
{
    int type = (relval >> 26) & 7;
    if (type == 3) {
        *persistent = relval << 16;
        return 1;
    }
    return 0;
}

static abi_ulong
flat_get_addr_from_rp(abi_ulong ul_ptr, abi_ulong relval, abi_ulong flags, abi_ulong *persistent)
{
    int type = (relval >> 26) & 7;
    abi_ulong val;

#ifdef DEBUG
    printf("%s:%i: ptr:%8x relval:%8x type:%x flags:%x persistent:%x",
           __func__, __LINE__, ul_ptr, relval, type, flags, *persistent);
#endif

    switch (type) {
    case FLAT_BFIN_RELOC_TYPE_16_BIT:
    case FLAT_BFIN_RELOC_TYPE_16H_BIT:
        if (get_user_u16(val, ul_ptr)) {
            fprintf(stderr, "BINFMT_FLAT: unable to read reloc at %#x\n", ul_ptr);
            abort();
        }
        val += *persistent;
        break;
    case FLAT_BFIN_RELOC_TYPE_32_BIT:
        if (get_user_u32(val, ul_ptr)) {
            fprintf(stderr, "BINFMT_FLAT: unable to read reloc at %#x\n", ul_ptr);
            abort();
        }
        break;
    default:
        fprintf(stderr, "BINFMT_FLAT: Unknown relocation type %x\n", type);
        abort();
        break;
    }
#ifdef DEBUG
    printf(" val:%x\n", val);
#endif

    /*
     * Stack-relative relocs contain the offset into the stack, we
     * have to add the stack's start address here and return 1 from
     * flat_addr_absolute to prevent the normal address calculations
     */
    if (relval & (1 << 29)) {
        fprintf(stderr, "BINFMT_FLAT: stack relocs not supported\n");
        abort();
        /*return val + current->mm->context.end_brk;*/
    }

    if ((flags & FLAT_FLAG_GOTPIC) == 0)
        val = ntohl(val);

    return val;
}

static int
flat_put_addr_at_rp(abi_ulong ptr, abi_ulong addr, abi_ulong relval)
{
    int type = (relval >> 26) & 7;

    switch (type) {
    case FLAT_BFIN_RELOC_TYPE_16_BIT:  return put_user_u16(addr, ptr);
    case FLAT_BFIN_RELOC_TYPE_16H_BIT: return put_user_u16(addr >> 16, ptr);
    case FLAT_BFIN_RELOC_TYPE_32_BIT:  return put_user_u32(addr, ptr);
    }

    abort();
}
