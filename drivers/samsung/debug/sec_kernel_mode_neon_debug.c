// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_kernel_mode_neon_debug.c
 *
 * COPYRIGHT(C) 2019 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/sched.h>
#include <linux/version.h>

#include <asm/fpsimd.h>

#include "sec_debug_internal.h"

static void __fpsimd_context_check_dummy(struct task_struct *next)
{
}

static void __fpsimd_context_check(struct task_struct *next)
{
	size_t i;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
	struct fpsimd_state current_st, *saved_st;
	saved_st = &next->thread.fpsimd_state;
#else
	struct user_fpsimd_state current_st, *saved_st;
	saved_st = &next->thread.uw.fpsimd_state;
#endif
	fpsimd_save_state(&current_st);

	for (i = 0; i < ARRAY_SIZE(current_st.vregs); i++) {
		if (unlikely((current_st.vregs[i] != saved_st->vregs[i])))
			BUG();
	}

	if (unlikely((current_st.fpsr != saved_st->fpsr) ||
		     (current_st.fpcr != saved_st->fpcr)))
		BUG();
}

void (*fpsimd_context_check)(struct task_struct *next) __read_mostly;

static int __init sec_net_debug_init_default_call_back(void)
{
	fpsimd_context_check = __fpsimd_context_check_dummy;

	return 0;
}
pure_initcall(sec_net_debug_init_default_call_back);

static int __init sec_neon_debug_init(void)
{
	/* 'fpsimd_context_check' should not be 'NULL'
	 * when this initcall is called */
	BUG_ON(!fpsimd_context_check);

	if (sec_debug_is_enabled())
		fpsimd_context_check = __fpsimd_context_check;

	return 0;
}
arch_initcall_sync(sec_neon_debug_init);
