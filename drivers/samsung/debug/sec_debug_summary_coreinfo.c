// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_debug_summary_coreinfo.c
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

#include <linux/kernel.h>
#include <linux/sched.h>

#include <asm/stacktrace.h>
#include <asm/system_misc.h>
#include <asm/page.h>
#include <asm/sections.h>
#include <linux/timekeeper_internal.h>

#include <linux/sec_debug.h>

#include "sec_debug_summary_extern.h"
#include "sec_debug_internal.h"

static char *summary_coreinfo_data;
static int32_t summary_coreinfo_size;
static void *summary_coreinfo_note;

void summary_coreinfo_append_str(const char *fmt, ...)
{
	va_list args;
	char *buf = &summary_coreinfo_data[summary_coreinfo_size];
	int32_t remained;
	int32_t expected;

	remained = (int32_t)SUMMARY_COREINFO_BYTES - summary_coreinfo_size;
	if (remained < 0) {
		pr_warn("No space left\n");
		return;
	}

	va_start(args, fmt);
	expected = (int32_t)vsnprintf(buf, remained, fmt, args);
	va_end(args);

	if (expected >= remained || expected < 0) {
		pr_warn("Not enough space to append %s\n", buf);
		memset(buf, 0x0, remained);
		return;
	}

	summary_coreinfo_size += expected;
}

int __init summary_init_coreinfo(struct sec_debug_summary_data_apss *secdbg_apss)
{
	summary_coreinfo_data = (char *)get_zeroed_page(GFP_KERNEL);
	if (!summary_coreinfo_data) {
		pr_warn("Memory allocation for summary_coreinfo_data failed\n");
		return -ENOMEM;
	}

	/* TODO: will be used in future? */
	summary_coreinfo_note = alloc_pages_exact(SUMMARY_COREINFO_NOTE_SIZE,
						  GFP_KERNEL | __GFP_ZERO);
	if (!summary_coreinfo_note) {
		free_page((unsigned long)summary_coreinfo_data);
		summary_coreinfo_data = NULL;
		pr_warn("Memory allocation for summary_coreinfo_note failed\n");
		return -ENOMEM;
	}

	secdbg_apss->coreinfo.coreinfo_data = (uintptr_t)summary_coreinfo_data;
	secdbg_apss->coreinfo.coreinfo_size = (uintptr_t)&summary_coreinfo_size;
	secdbg_apss->coreinfo.coreinfo_note = (uintptr_t)summary_coreinfo_note;
	pr_info("coreinfo_data=0x%llx\n", secdbg_apss->coreinfo.coreinfo_data);

	SUMMARY_COREINFO_SYMBOL(runqueues);
	SUMMARY_COREINFO_STRUCT_SIZE(rq);
	SUMMARY_COREINFO_OFFSET(rq, clock);
	SUMMARY_COREINFO_OFFSET(rq, nr_running);

	SUMMARY_COREINFO_OFFSET(task_struct, prio);

	summary_coreinfo_append_str("OFFSET(tk_core.timekeeper)=%zu\n", 8);

	SUMMARY_COREINFO_OFFSET(timekeeper, xtime_sec);
#ifdef CONFIG_SCHED_WALT
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	SUMMARY_COREINFO_OFFSET(rq, cluster);
	SUMMARY_COREINFO_OFFSET(sched_cluster, cur_freq);
	SUMMARY_COREINFO_OFFSET(sched_cluster, max_mitigated_freq);
#else
	SUMMARY_COREINFO_OFFSET(rq, wrq);
	SUMMARY_COREINFO_OFFSET(walt_sched_cluster, cur_freq);
#endif
#endif

	return 0;
}
