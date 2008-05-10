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
	uint32_t *ptr = (uint32_t *)ul_ptr;
	uint16_t *usptr = (uint16_t *)ptr;
	int type = (relval >> 26) & 7;
	abi_ulong val;

	switch (type) {
		case 0: /* FLAT_BFIN_RELOC_TYPE_16_BIT */
		case 1: /* FLAT_BFIN_RELOC_TYPE_16H_BIT */
			val = *usptr;
			val += *persistent;
			break;
		case 2: /* FLAT_BFIN_RELOC_TYPE_32_BIT */
			val = *ptr;
			break;
		default:
			fprintf(stderr, "BINFMT_FLAT: Unknown relocation type %x\n", type);
			abort();
			break;
	}

	/*
	 * Stack-relative relocs contain the offset into the stack, we
	 * have to add the stack's start address here and return 1 from
	 * flat_addr_absolute to prevent the normal address calculations
	 */
	if (relval & (1 << 29))
		abort();
//		return val + current->mm->context.end_brk;

#ifndef TARGET_WORDS_BIGENDIAN
	if ((flags & FLAT_FLAG_GOTPIC) == 0)
		val = bswap32(val);
#endif

	return val;
}

static int
flat_put_addr_at_rp(abi_ulong ptr, abi_ulong addr, abi_ulong relval)
{
	uint16_t *usptr = (uint16_t *)ptr;
	//uint16_t usptr = ptr;
	int type = (relval >> 26) & 7;
	switch (type) {
		case 0: return put_user_ual(addr, usptr);
		case 1: return put_user_ual(addr >> 16, usptr);
		case 2: return put_user_ual(addr, ptr);
	}
	abort();
	return 1;
}
