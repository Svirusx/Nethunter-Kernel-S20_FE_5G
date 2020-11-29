/*
 * arch/arm64/include/asm/sec_debug.h
 *
 * COPYRIGHT(C) 2006-2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SEC_DEBUG_ARM64_H
#define SEC_DEBUG_ARM64_H

#if defined(CONFIG_ARM64) && defined(CONFIG_SEC_DEBUG)

struct sec_debug_mmu_reg_t {
	uint64_t TTBR0_EL1;
	uint64_t TTBR1_EL1;
	uint64_t TCR_EL1;
	uint64_t MAIR_EL1;
	uint64_t ATCR_EL1;
	uint64_t AMAIR_EL1;

	uint64_t HSTR_EL2;
	uint64_t HACR_EL2;
	uint64_t TTBR0_EL2;
	uint64_t VTTBR_EL2;
	uint64_t TCR_EL2;
	uint64_t VTCR_EL2;
	uint64_t MAIR_EL2;
	uint64_t ATCR_EL2;

	uint64_t TTBR0_EL3;
	uint64_t MAIR_EL3;
	uint64_t ATCR_EL3;
};

/* ARM CORE regs mapping structure */
struct sec_debug_core_t {
	/* COMMON */
	uint64_t x0;
	uint64_t x1;
	uint64_t x2;
	uint64_t x3;
	uint64_t x4;
	uint64_t x5;
	uint64_t x6;
	uint64_t x7;
	uint64_t x8;
	uint64_t x9;
	uint64_t x10;
	uint64_t x11;
	uint64_t x12;
	uint64_t x13;
	uint64_t x14;
	uint64_t x15;
	uint64_t x16;
	uint64_t x17;
	uint64_t x18;
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t x29;		/* sp */
	uint64_t x30;		/* lr */

	uint64_t pc;		/* pc */
	uint64_t cpsr;		/* cpsr */

	/* EL0 */
	uint64_t sp_el0;

	/* EL1 */
	uint64_t sp_el1;
	uint64_t elr_el1;
	uint64_t spsr_el1;

	/* EL2 */
	uint64_t sp_el2;
	uint64_t elr_el2;
	uint64_t spsr_el2;

	/* EL3 */
	/* uint64_t sp_el3; */
	/* uint64_t elr_el3; */
	/* uint64_t spsr_el3; */
};

#define READ_SPECIAL_REG(x) ({ \
	uint64_t val; \
	asm volatile ("mrs %0, " # x : "=r"(val)); \
	val; \
})

static inline void sec_debug_save_mmu_reg(struct sec_debug_mmu_reg_t *mmu_reg)
{
	uint64_t pstate, which_el;

	pstate = READ_SPECIAL_REG(CurrentEl);
	which_el = pstate & PSR_MODE_MASK;

	/* pr_emerg("%s: sec_debug EL mode=%d\n", __func__,which_el); */

	mmu_reg->TTBR0_EL1 = READ_SPECIAL_REG(TTBR0_EL1);
	mmu_reg->TTBR1_EL1 = READ_SPECIAL_REG(TTBR1_EL1);
	mmu_reg->TCR_EL1 = READ_SPECIAL_REG(TCR_EL1);
	mmu_reg->MAIR_EL1 = READ_SPECIAL_REG(MAIR_EL1);
	mmu_reg->AMAIR_EL1 = READ_SPECIAL_REG(AMAIR_EL1);
}

static inline void sec_debug_save_core_reg(struct sec_debug_core_t *core_reg)
{
	uint64_t pstate,which_el;

	pstate = READ_SPECIAL_REG(CurrentEl);
	which_el = pstate & PSR_MODE_MASK;

	/* pr_emerg("%s: sec_debug EL mode=%d\n", __func__,which_el); */

	asm volatile (
		"str x0, [%0,#0]\n\t"			/* x0 is pushed first to core_reg */
		"mov x0, %0\n\t"
		"add x0, x0, 0x8\n\t"
		"stp x1, x2, [x0], #0x10\n\t"
		"stp x3, x4, [x0], #0x10\n\t"
		"stp x5, x6, [x0], #0x10\n\t"
		"stp x7, x8, [x0], #0x10\n\t"
		"stp x9, x10, [x0], #0x10\n\t"
		"stp x11, x12, [x0], #0x10\n\t"
		"stp x13, x14, [x0], #0x10\n\t"
#if (!defined CONFIG_CFP_ROPP) || (defined CONFIG_CFP_TEST)
		"stp x15, x16, [x0], #0x10\n\t"
		"stp x17, x18, [x0], #0x10\n\t"
#else
		"stp x15, x15, [x0], #0x10\n\t"
		"stp x18, x18, [x0], #0x10\n\t"
#endif
		"stp x19, x20, [x0], #0x10\n\t"
		"stp x21, x22, [x0], #0x10\n\t"
		"stp x23, x24, [x0], #0x10\n\t"
		"stp x25, x26, [x0], #0x10\n\t"
		"stp x27, x28, [x0], #0x10\n\t"
		"stp x29, x30, [x0], #0x10\n\t"

		/* pc */
		"adr x1, .\n\t"

		/* pstate */
		"mrs x15, NZCV\n\t"
		"bic x15, x15, #0xFFFFFFFF0FFFFFFF\n\t"
		"mrs x9, DAIF\n\t"
		"bic x9, x9, #0xFFFFFFFFFFFFFC3F\n\t"
		"orr x15, x15, x9\n\t"
		"mrs x10, CurrentEL\n\t"
		"bic x10, x10, #0xFFFFFFFFFFFFFFF3\n\t"
		"orr x15, x15, x10\n\t"
		"mrs x11, SPSel\n\t"
		"bic x11, x11, #0xFFFFFFFFFFFFFFFE\n\t"
		"orr x15, x15, x11\n\t"

		/* store pc & pstate */
		"stp x1, x15, [x0], #0x10\n\t"

		:				/* output */
		: "r"(core_reg)			/* input */
		: "%x0", "%x1"			/* clobbered registers */
	);

	core_reg->sp_el0 = READ_SPECIAL_REG(sp_el0);

	if(which_el >= PSR_MODE_EL2t){
		core_reg->sp_el0 = READ_SPECIAL_REG(sp_el1);
		core_reg->elr_el1 = READ_SPECIAL_REG(elr_el1);
		core_reg->spsr_el1 = READ_SPECIAL_REG(spsr_el1);
		core_reg->sp_el2 = READ_SPECIAL_REG(sp_el2);
		core_reg->elr_el2 = READ_SPECIAL_REG(elr_el2);
		core_reg->spsr_el2 = READ_SPECIAL_REG(spsr_el2);
	}
}

#endif /* defined(CONFIG_ARM64) && defined(CONFIG_SEC_DEBUG) */

#endif /* SEC_DEBUG_ARM64_H */
