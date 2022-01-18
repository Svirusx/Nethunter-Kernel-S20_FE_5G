// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_debug_pwdt.c
 *
 * COPYRIGHT(C) 2018-2019 Samsung Electronics Co., Ltd. All Right Reserved.
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

#include <linux/freezer.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include "sec_debug_internal.h"

#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "sec_debug."

#define SEC_DEBUG_MAX_PWDT_RESTART_CNT		20
#define SEC_DEBUG_MAX_PWDT_SYNC_CNT		40
#define SEC_DEBUG_MAX_PWDT_INIT_CNT		200

#define __PWDT_BOOT_TYPE_RECOVERY		0
#define __PWDT_BOOT_TYPE_CHARGER		1
#define __PWDT_BOOT_TYPE_NOT_VERIFIEDBOOT	2

#define PWDT_BOOT_TYPE_NORMAL			0
#define PWDT_BOOT_TYPE_RECOVERY			(1 << __PWDT_BOOT_TYPE_RECOVERY)
#define PWDT_BOOT_TYPE_CHARGER			(1 << __PWDT_BOOT_TYPE_CHARGER)
#define PWDT_BOOT_TYPE_NOT_VERIFIEDBOOT		(1 << __PWDT_BOOT_TYPE_NOT_VERIFIEDBOOT)

static unsigned int pwdt_boot_type;

static int __init __pwdt_androidboot_boot_recovery(char *str)
{
	int boot_recovery = 0;
	int ret = -EINVAL;

	if (get_option(&str, &boot_recovery)) {
		pwdt_boot_type |= boot_recovery ? PWDT_BOOT_TYPE_RECOVERY : 0;
		ret = 0;
	}

	return ret;
}
early_param("androidboot.boot_recovery", __pwdt_androidboot_boot_recovery);

static int __init __pwdt_androidboot_mode(char *str)
{
	int ret = -EINVAL;

	if (!strncmp(str, "charger", ARRAY_SIZE("charger"))) {
		pwdt_boot_type |= PWDT_BOOT_TYPE_CHARGER;
		ret = 0;
	}

	return ret;
}
early_param("androidboot.mode", __pwdt_androidboot_mode);

static int __init __pwdt_androidboot_verifiedbootstate(char *str)
{
	int ret = -EINVAL;

	if (!strncmp(str, "orange", ARRAY_SIZE("orange"))) {
		pwdt_boot_type |= PWDT_BOOT_TYPE_NOT_VERIFIEDBOOT;
		ret = 0;
	}

	return ret;
}
early_param("androidboot.verifiedbootstate", __pwdt_androidboot_verifiedbootstate);

static unsigned long pwdt_start_ms;
module_param_named(pwdt_start_ms, pwdt_start_ms, ulong, 0644);

static unsigned long pwdt_end_ms;
module_param_named(pwdt_end_ms, pwdt_end_ms, ulong, 0644);

static unsigned int pwdt_pid;
module_param_named(pwdt_pid, pwdt_pid, uint, 0644);

static unsigned long pwdt_sync_cnt;
module_param_named(pwdt_sync_cnt, pwdt_sync_cnt, ulong, 0644);

static unsigned long pwdt_sync_delay;
static unsigned long pwdt_restart_delay;
static unsigned long pwdt_init_delay;
static unsigned long last_sync_cnt;

static inline bool __sec_debug_pwdt_is_not_initialized(void)
{
	return pwdt_pid == 0 && pwdt_start_ms == 0;
}

static inline void __sec_debug_inc_pwdt_init_delay(void)
{
	if (pwdt_init_delay++ >= SEC_DEBUG_MAX_PWDT_INIT_CNT)
		panic(UPLOAD_MSG_PF_WD_INIT_FAIL);
}

static inline bool __sec_debug_pwdt_is_killed(void)
{
	return pwdt_pid != 0 && pwdt_start_ms == 0;
}

static inline void __sec_debug_inc_pwdt_restart_delay(void)
{
	if (unlikely(pm_freezing == true)) {
		return;
	}

	if (pwdt_restart_delay++ >= SEC_DEBUG_MAX_PWDT_RESTART_CNT)
		panic(UPLOAD_MSG_PF_WD_RESTART_FAIL);
}

static inline bool __sec_debug_pwdt_sync_is_not_updated(void)
{
	return last_sync_cnt == pwdt_sync_cnt;
}

static inline void __sec_debug_pwdt_sync_update(void)
{
	last_sync_cnt = pwdt_sync_cnt;
	pwdt_sync_delay = 0;
}

static inline void __sec_debug_inc_pwdt_pwdt_sync_delay(void)
{
	/* pwdt_sync_cnt is updated in every 30s,
	 * but sec_debug_check_pwdt is invoked in every 10s
	 * kernel watchdog will trigger Panic
	 * if platform watchdog couldnot update sync_cnt for 400secs
	 */
	if (pwdt_sync_delay++ >= SEC_DEBUG_MAX_PWDT_SYNC_CNT)
		panic(UPLOAD_MSG_PF_WD_KICK_FAIL);
}

static void __sec_debug_check_pwdt_sync(void)
{
	struct task_struct *wdt_tsk;
	bool frozen_wdt_tsk, freezing_wdt_tsk;

	pwdt_init_delay = 0;
	pwdt_restart_delay = 0;

	rcu_read_lock();
	wdt_tsk = find_task_by_vpid(pwdt_pid);

	/* if cannot find platform watchdog thread,
	 * it might be killed by system_crash or zygote, We ignored this case.
	 */
	if (!wdt_tsk) {
		rcu_read_unlock();
		__sec_debug_pwdt_sync_update();
		pr_warn("cannot find watchdog thread!!\n");

		return;
	}

	get_task_struct(wdt_tsk);
	rcu_read_unlock();

	frozen_wdt_tsk = frozen(wdt_tsk);
	freezing_wdt_tsk = freezing(wdt_tsk);

	put_task_struct(wdt_tsk);

	if (unlikely(frozen_wdt_tsk || freezing_wdt_tsk)) {
		pr_info("wdt_task is frozen : [%u],[%u]\n",
				frozen_wdt_tsk, freezing_wdt_tsk);
		__sec_debug_pwdt_sync_update();
	} else {
		if (__sec_debug_pwdt_sync_is_not_updated())
			__sec_debug_inc_pwdt_pwdt_sync_delay();
		else
			__sec_debug_pwdt_sync_update();
	}
}

void sec_debug_check_pwdt(void)
{
	if (unlikely(pwdt_boot_type != PWDT_BOOT_TYPE_NORMAL))
		return;

	pr_info("pid[%u], start_ms[%lu], sync_cnt[%lu], "
		"restart_delay[%lu], init_dealy[%lu], sync_delay[%lu]\n",
		pwdt_pid, pwdt_start_ms, pwdt_sync_cnt,
		pwdt_restart_delay, pwdt_init_delay, pwdt_sync_delay);

	if (__sec_debug_pwdt_is_not_initialized())
		__sec_debug_inc_pwdt_init_delay();
	else if (__sec_debug_pwdt_is_killed()) {
		if (unlikely(!pwdt_restart_delay))
			pr_warn("pwdt has been killed!, start_ms[%lu], end_ms[%lu]\n",
					pwdt_start_ms, pwdt_end_ms);

		__sec_debug_inc_pwdt_restart_delay();
	} else
		__sec_debug_check_pwdt_sync();
}
