/*
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Authors:	James Gleeson <jagleeso@gmail.com>
 *		Wenbo Shen <wenbo.s@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_CFP_H
#define __ASM_CFP_H

#ifndef __ASSEMBLY__
#error "Only include this from assembly code"
#endif

#ifdef CONFIG_CFP_ROPP
#include <linux/cfp.h>
#include <asm/asm-offsets.h>

	.macro reset_sysreg
	//disable debug bcr
	mrs	RRX, dbgbcr5_el1
	and	RRX, RRX, # ~(1)
	msr	dbgbcr5_el1, RRX
	//load debug bvr
	msr	RRMK, xzr
	.endm
/*
 * disables and resets ccnt and init RRK to zero.
 */
	.macro ropp_primary_early_init
#ifdef CONFIG_CFP_ROPP_SYSREGKEY
	reset_sysreg
#endif
	/* Must initialize RRK to zero */
	mov	RRK, xzr
	.endm

/*
 * secondary core will start a forked thread, so rrk is already enc'ed
 * so only need to reload the master key and thread key
 */
	.macro ropp_secondary_init ti
#ifdef CONFIG_CFP_ROPP_SYSREGKEY
	reset_sysreg
	//load master key from uh
	ropp_load_mk
#endif
	//load thread key
	ropp_load_key \ti
	.endm

/*
 * For kernel_entry
 */
	.macro ropp_new_key, ti
#ifdef CONFIG_CFP_ROPP_SYSREGKEY
	push	x0, x1
	mrs	x1, DAIF
	msr	DAIFset, #0x3
	//dec the old thread key
	ldr	x0, [\ti, #TSK_TI_RRK]
	mrs	RRK, RRMK
	eor	RRK, x0, RRK
	//gen the new one
	mrs	x0, CNTPCT_EL0
	add	RRK, x0, RRK, lsl #0x8
	mrs	x0, RRMK
	eor	x0, RRK, x0
#ifdef SYSREG_DEBUG
	ldr	RRK, = ropp_fixed_key
	ldr	RRK, [RRK]
	mrs	x0, RRMK
	eor	x0, x0, RRK
#endif
	str	x0, [\ti, #TSK_TI_RRK]
	msr	DAIF, x1
	pop	x0, x1
#elif defined CONFIG_CFP_ROPP_RANDKEY
	mrs	RRK, CNTPCT_EL0
	add	RRK, RRK, RRK, lsl #32
	str	RRK, [\ti, #TSK_TI_RRK]
#elif defined CONFIG_CFP_ROPP_FIXKEY
	ldr	RRK, = ropp_fixed_key
	ldr	RRK, [RRK]
	str	RRK, [\ti, #TSK_TI_RRK]
#elif defined CONFIG_CFP_ROPP_ZEROKEY
	mov	RRK, xzr
	str	RRK, [\ti, #TSK_TI_RRK]
#else
	#error "Please choose one ROPP key scheme"
#endif
	.endm

/*
 * Load the key register from thread_info
 */
	.macro	ropp_load_key, ti
#ifdef CONFIG_CFP_ROPP_SYSREGKEY
	push	x0, x1
	ldr	x0, [\ti, #TSK_TI_RRK]
	mrs	x1, DAIF
	msr	DAIFset, #0x3
	mrs	RRK, RRMK
	eor	RRK, RRK, x0
	msr	DAIF, x1
	pop	x0, x1
#elif defined CONFIG_CFP_ROPP_RANDKEY
	ldr	RRK, [\ti, #TSK_TI_RRK]
#elif defined CONFIG_CFP_ROPP_FIXKEY
	ldr	RRK, [\ti, #TSK_TI_RRK]
#elif defined CONFIG_CFP_ROPP_ZEROKEY
	ldr	RRK, [\ti, #TSK_TI_RRK]
#endif
	.endm

	.macro ropp_load_mk
#ifdef CONFIG_UH
	push	x0, x1
	push	x2, x3
	push	x4, x5
	mov	x1, #0x10 //RKP_ROPP_RELOAD
	mov	x0, #0xc002 //UH_APP_CFP
	movk	x0, #0xc300, lsl #16
	smc	#0x0
	pop	x4, x5
	pop	x2, x3
	pop	x0, x1
#else
	push	x0, x1
	ldr	x0, = ropp_master_key
	ldr	x0, [x0]
	msr	RRMK, x0
	pop	x0, x1
#endif
	.endm

#endif //CONFIG_CFP_ROPP
#endif //__ASM_CFP_H
