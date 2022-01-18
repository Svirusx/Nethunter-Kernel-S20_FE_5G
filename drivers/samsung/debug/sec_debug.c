// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_debug.c
 *
 * COPYRIGHT(C) 2006-2019 Samsung Electronics Co., Ltd. All Right Reserved.
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

#include <linux/version.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input/qpnp-power-on.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/nmi.h>
#include <linux/notifier.h>
#include <linux/of_address.h>
#include <linux/oom.h>
#include <linux/reboot.h>
#include <linux/regulator/consumer.h>
#include <linux/seq_file.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <linux/memblock.h>
#include <soc/qcom/qseecom_scm.h>
#include <linux/qcom_scm.h>
#else
#include <linux/bootmem.h>
#include <asm/compiler.h>
#endif
#include <linux/soc/qcom/smem.h>
#include <soc/qcom/restart.h>

#include <asm/cacheflush.h>
#include <asm/stacktrace.h>
#include <asm/system_misc.h>

#include <linux/sec_debug.h>
#include <linux/sec_param.h>

#define CREATE_TRACE_POINTS
#include <trace/events/sec_debug.h>

#include "sec_debug_internal.h"

#ifdef CONFIG_ARM64
static inline void outer_flush_all(void) { }
#endif

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
unsigned long sec_delay_check __read_mostly = 1;
EXPORT_SYMBOL(sec_delay_check);
#endif

/* enable sec_debug feature */
static unsigned int sec_dbg_level;
static int force_upload;

static unsigned int enable = 1;
module_param_named(enable, enable, uint, 0644);

#ifdef CONFIG_SEC_SSR_DEBUG_LEVEL_CHK
static unsigned int enable_cp_debug = 1;
module_param_named(enable_cp_debug, enable_cp_debug, uint, 0644);
#endif

static unsigned int dump_sink;
module_param_named(dump_sink, dump_sink, uint, 0644);

static unsigned int reboot_multicmd;
module_param_named(reboot_multicmd, reboot_multicmd, uint, 0644);

uint64_t get_pa_dump_sink(void)
{
	return virt_to_phys(&dump_sink);
}

/* This is shared with msm-power off module. */
void __iomem *restart_reason;
static void __iomem *upload_cause;
#if IS_ENABLED(CONFIG_POWER_RESET_QCOM_DOWNLOAD_MODE)
static void __iomem *watchdog_base;
#define WDT0_RST        0x04
#define WDT0_EN         0x08
#define WDT0_BITE_TIME  0x14
#endif

void (*sec_nvmem_pon_write)(u8 pon_rr) = NULL;

DEFINE_PER_CPU(struct sec_debug_core_t, sec_debug_core_reg);
DEFINE_PER_CPU(struct sec_debug_mmu_reg_t, sec_debug_mmu_reg);
DEFINE_PER_CPU(enum sec_debug_upload_cause_t, sec_debug_upload_cause);

static void sec_debug_set_qc_dload_magic(int on)
{
	pr_info("on=%d\n", on);
	set_dload_mode(on);
}

#define PON_RESTART_REASON_NOT_HANDLE	PON_RESTART_REASON_MAX
#define RESTART_REASON_NOT_HANDLE	RESTART_REASON_END

/* This is shared with 'msm-poweroff.c'  module. */
static enum sec_restart_reason_t __iomem *qcom_restart_reason;

static void sec_debug_set_upload_magic(unsigned int magic)
{
	__pr_err("(%s) %x\n", __func__, magic);

	if (magic != RESTART_REASON_NORMAL)
		sec_debug_set_qc_dload_magic(1);
	else
		sec_debug_set_qc_dload_magic(0);

	__raw_writel(magic, qcom_restart_reason);

	flush_cache_all();
	outer_flush_all();
}

static int sec_debug_normal_reboot_handler(struct notifier_block *nb,
		unsigned long action, void *data)
{
	char recovery_cause[256] = { '\0', };
	char *ptr1, *ptr2;
	char index_char[3];
	unsigned int index;

	set_dload_mode(0);	/* set defalut (not upload mode) */

	sec_debug_set_upload_magic(RESTART_REASON_NORMAL);

	if (unlikely(!data))
		return 0;

	if ((action == SYS_RESTART) &&
		!strncmp((char *)data, "recovery", 8)) {
		sec_get_param(param_index_reboot_recovery_cause,
			      recovery_cause);
		if (!recovery_cause[0] || !strlen(recovery_cause)) {
			snprintf(recovery_cause, sizeof(recovery_cause),
				 "%s:%d ", current->comm, task_pid_nr(current));
			sec_set_param(param_index_reboot_recovery_cause,
				      recovery_cause);
		}
	}

	if ((action == SYS_RESTART) &&
		!strncmp((char *)data, "param", 5)) {
		ptr1 = strchr((char *)data, '_');
		ptr1 = ptr1 + 1;
		ptr2 = strchr((char *)ptr1, '_');
		memcpy(&index_char, ptr1 , ptr2-ptr1);
		index_char[ptr2-ptr1] = '\0';

		if(!kstrtouint(index_char, 0, &index)) {
			ptr2 = ptr2 + 1;
			if (*ptr2 == '0') {
				unsigned long value_number;
				if (!kstrtoul(ptr2 + 1, 0, &value_number))
					sec_set_param(index, &value_number);
			} else if (*ptr2 == '1') {
				char value_char[256] = {0,};
				ptr2 = ptr2 + 1;
				memcpy(value_char, ptr2, strlen ((char *)ptr2));
				sec_set_param(index, value_char);
			} else
				pr_info("invalid param index of reboot cmd.\n");
		} else
			pr_info("invalid param value of reboot cmd.\n");
	}

	return 0;
}

void sec_debug_update_dload_mode(const int restart_mode, const int in_panic)
{
	if (!in_panic && restart_mode != RESTART_DLOAD)
		sec_debug_set_upload_magic(RESTART_REASON_NORMAL);
}

static inline void __sec_debug_set_restart_reason(
				enum sec_restart_reason_t __r)
{
	__raw_writel((u32)__r, qcom_restart_reason);
}

static enum pon_restart_reason __pon_restart_rory_start(
				unsigned long opt_code)
{
	return (PON_RESTART_REASON_RORY_START | opt_code);
}

static enum pon_restart_reason __pon_restart_set_debug_level(
				unsigned long opt_code)
{
	switch (opt_code) {
	case ANDROID_DEBUG_LEVEL_LOW:
		return PON_RESTART_REASON_DBG_LOW;
	case ANDROID_DEBUG_LEVEL_MID:
		return PON_RESTART_REASON_DBG_MID;
	case ANDROID_DEBUG_LEVEL_HIGH:
		return PON_RESTART_REASON_DBG_HIGH;
	}

	return PON_RESTART_REASON_UNKNOWN;
}

static enum pon_restart_reason __pon_restart_set_cpdebug(
				unsigned long opt_code)
{
	if (opt_code == ANDROID_CP_DEBUG_ON)
		return PON_RESTART_REASON_CP_DBG_ON;
	else if (opt_code == ANDROID_CP_DEBUG_OFF)
		return PON_RESTART_REASON_CP_DBG_OFF;

	return PON_RESTART_REASON_UNKNOWN;
}

static enum pon_restart_reason __pon_restart_force_upload(
				unsigned long opt_code)
{
	return (opt_code) ?
		PON_RESTART_REASON_FORCE_UPLOAD_ON :
		PON_RESTART_REASON_FORCE_UPLOAD_OFF;
}

static enum pon_restart_reason __pon_restart_dump_sink(
				unsigned long opt_code)
{
	switch (opt_code) {
	case DUMP_SINK_TO_BOOTDEV:
		return PON_RESTART_REASON_DUMP_SINK_BOOTDEV;
	case DUMP_SINK_TO_SDCARD:
		return PON_RESTART_REASON_DUMP_SINK_SDCARD;
	default:
		return PON_RESTART_REASON_DUMP_SINK_USB;
	}

	return PON_RESTART_REASON_UNKNOWN;
}

static enum pon_restart_reason __pon_restart_cdsp_signoff(
				unsigned long opt_code)
{
	switch (opt_code) {
	case CDSP_SIGNOFF_ON:
		return PON_RESTART_REASON_CDSP_ON;
	case CDSP_SIGNOFF_BLOCK:
		return PON_RESTART_REASON_CDSP_BLOCK;
	}

	return PON_RESTART_REASON_UNKNOWN;
}


#ifdef CONFIG_MUIC_SUPPORT_RUSTPROOF
static enum pon_restart_reason __pon_restart_swsel(
				unsigned long opt_code)
{
	unsigned long value =
		 (((opt_code & 0x8) >> 1) | opt_code) & 0x7;

	return (PON_RESTART_REASON_SWITCHSEL | value);
}
#endif

/* FIXME: QC's linux-5.4.y does not have 'arm_pm_restart' call back in
 * arch/arm64/kernel/process.c file.
 * This is an weak symbol having a same name and this will be eliminated
 * while linking if the kernel has a 'arm_pm_restart' and if it does't
 * this variable has a 'NULL' value by default.
 */
void (*arm_pm_restart)(enum reboot_mode reboot_mode, const char *cmd) __weak;

static inline void sec_debug_pm_restart(char *cmd)
{
	__pr_err("(%s) %s %s\n", __func__,
		init_uts_ns.name.release, init_uts_ns.name.version);
	__pr_err("(%s) rebooting...\n", __func__);
	flush_cache_all();
	outer_flush_all();

#if IS_ENABLED(CONFIG_POWER_RESET_QCOM_DOWNLOAD_MODE)
	if (!sec_nvmem_pon_write && watchdog_base) {
		pr_emerg("Causing a watchdog bite!\n");
		__raw_writel(1, watchdog_base + WDT0_EN);
		mb();
		__raw_writel(1, watchdog_base + WDT0_BITE_TIME);
		/* Mke sure bite time is written before we reset */
		mb();
		__raw_writel(1, watchdog_base + WDT0_RST);
		/* Make sure we wait only after reset */
		mb();

		/* while (1) ; */
		asm volatile ("b .");
	}
#endif
	if (arm_pm_restart)
		arm_pm_restart(REBOOT_COLD, cmd);
	else
		do_kernel_restart(cmd);

	/* while (1) ; */
	asm volatile ("b .");
}

static void __sec_debug_hw_reset(void)
{
	sec_debug_pm_restart("sec_debug_hw_reset");
}

#ifndef CONFIG_SEC_LOG_STORE_LAST_KMSG
static bool sec_check_leave_reboot_reason(enum pon_restart_reason pon_rr)
{
	bool result = false;

	result = (pon_rr == PON_RESTART_REASON_MULTICMD) || result;
	result = (pon_rr == PON_RESTART_REASON_LIMITED_DRAM_SETTING) || result;

	return result;
}
#endif

void sec_debug_update_restart_reason(const char *cmd, const int in_panic,
		const int restart_mode)
{
	struct __magic {
		const char *cmd;
		enum pon_restart_reason pon_rr;
		enum sec_restart_reason_t sec_rr;
		enum pon_restart_reason (*func)(unsigned long opt_code);
	} magic[] = {
		{ "sec_debug_hw_reset",
			PON_RESTART_REASON_NOT_HANDLE,
			RESTART_REASON_SEC_DEBUG_MODE, NULL},
		{ "download",
			PON_RESTART_REASON_DOWNLOAD,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "nvbackup",
			PON_RESTART_REASON_NVBACKUP,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "nvrestore",
			PON_RESTART_REASON_NVRESTORE,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "nverase",
			PON_RESTART_REASON_NVERASE,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "nvrecovery",
			PON_RESTART_REASON_NVRECOVERY,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "cpmem_on",
			PON_RESTART_REASON_CP_MEM_RESERVE_ON,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "cpmem_off",
			PON_RESTART_REASON_CP_MEM_RESERVE_OFF,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "mbsmem_on",
			PON_RESTART_REASON_MBS_MEM_RESERVE_ON,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "mbsmem_off",
			PON_RESTART_REASON_MBS_MEM_RESERVE_OFF,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "GlobalActions restart",
			PON_RESTART_REASON_NORMALBOOT,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "userrequested",
			PON_RESTART_REASON_NORMALBOOT,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "silent.sec",
			PON_RESTART_REASON_NORMALBOOT,
			RESTART_REASON_NOT_HANDLE, NULL },
		{ "oem-",
			PON_RESTART_REASON_UNKNOWN,
			RESTART_REASON_NORMAL, NULL},
		{ "sud",
			PON_RESTART_REASON_UNKNOWN,
			RESTART_REASON_NORMAL, __pon_restart_rory_start},
		{ "debug",
			PON_RESTART_REASON_UNKNOWN,
			RESTART_REASON_NORMAL, __pon_restart_set_debug_level},
		{ "cpdebug",
			PON_RESTART_REASON_UNKNOWN,
			RESTART_REASON_NORMAL, __pon_restart_set_cpdebug},
		{ "forceupload",
			PON_RESTART_REASON_UNKNOWN,
			RESTART_REASON_NORMAL, __pon_restart_force_upload},
		{ "dump_sink",
			PON_RESTART_REASON_UNKNOWN,
			RESTART_REASON_NORMAL, __pon_restart_dump_sink},
		{ "signoff",
			PON_RESTART_REASON_UNKNOWN,
			RESTART_REASON_NORMAL, __pon_restart_cdsp_signoff},
		{ "cross_fail",
			PON_RESTART_REASON_CROSS_FAIL,
			RESTART_REASON_NORMAL, NULL},
		{ "from_fastboot",
			PON_RESTART_REASON_NORMALBOOT,
			RESTART_REASON_NOT_HANDLE, NULL},
		{ "disallow,fastboot",
			PON_RESTART_REASON_NORMALBOOT,
			RESTART_REASON_NOT_HANDLE, NULL},
#ifdef CONFIG_MUIC_SUPPORT_RUSTPROOF
		{ "swsel",
			PON_RESTART_REASON_UNKNOWN,
			RESTART_REASON_NORMAL, __pon_restart_swsel},
#endif
#ifdef CONFIG_SEC_PERIPHERAL_SECURE_CHK
		{ "peripheral_hw_reset",
			PON_RESTART_REASON_SECURE_CHECK_FAIL,
			RESTART_REASON_NORMAL, NULL},
#endif
#ifdef CONFIG_SEC_QUEST_UEFI_USER
		{ "user_quefi_quest",
			PON_RESTART_REASON_QUEST_QUEFI_USER_START,
			RESTART_REASON_NORMAL, NULL},
		{ "user_suefi_quest",
			PON_RESTART_REASON_QUEST_SUEFI_USER_START,
			RESTART_REASON_NORMAL, NULL},			
#endif
#ifdef CONFIG_SEC_QUEST_DDR_SCAN_USER
		{ "user_dram_test",
			PON_RESTART_REASON_USER_DRAM_TEST,
			RESTART_REASON_NORMAL, NULL},
#endif			
		{ "multicmd",
			PON_RESTART_REASON_MULTICMD,
			RESTART_REASON_NORMAL, NULL},
		{ "dram",
			PON_RESTART_REASON_LIMITED_DRAM_SETTING,
			RESTART_REASON_NORMAL, NULL}
	};
	enum pon_restart_reason pon_rr = PON_RESTART_REASON_UNKNOWN;
	enum sec_restart_reason_t sec_rr = RESTART_REASON_NORMAL;
	char cmd_buf[256];
	size_t i;

	if (!in_panic && restart_mode != RESTART_DLOAD)
		pon_rr = PON_RESTART_REASON_NORMALBOOT;

#ifndef CONFIG_SEC_LOG_STORE_LAST_KMSG
__next_cmd:
#endif
	if (!cmd)
		__pr_err("(%s) reboot cmd : NULL\n", __func__);
	else if (!strlen(cmd))
		__pr_err("(%s) reboot cmd : Zero Length\n", __func__);
	else {
		strlcpy(cmd_buf, cmd, ARRAY_SIZE(cmd_buf));
		__pr_err("(%s) reboot cmd : %s\n", __func__, cmd_buf);
	}

	if (!cmd || !strlen(cmd) || !strncmp(cmd, "adb", 3))
		goto __done;

	for (i = 0; i < ARRAY_SIZE(magic); i++) {
		size_t len = strlen(magic[i].cmd);

		if (strncmp(cmd, magic[i].cmd, len))
			continue;

#ifndef CONFIG_SEC_LOG_STORE_LAST_KMSG
		if (sec_check_leave_reboot_reason(magic[i].pon_rr)) {
			cmd = cmd + len + 1;
			goto __next_cmd;
		}
#endif
		pon_rr = magic[i].pon_rr;
		sec_rr = magic[i].sec_rr;

		if (magic[i].func != NULL) {
			unsigned long opt_code;
			char *ptr = strstr(cmd_buf, ":");

			if (ptr != NULL)
				*ptr = '\0';

			if (!kstrtoul(cmd_buf + len, 0, &opt_code))
				pon_rr = magic[i].func(opt_code);
		}

		goto __done;
	}

	__sec_debug_set_restart_reason(RESTART_REASON_REBOOT);

	return;

__done:
	if (pon_rr != PON_RESTART_REASON_NOT_HANDLE) {
		if (sec_nvmem_pon_write)
			sec_nvmem_pon_write(pon_rr);
		else
			qpnp_pon_set_restart_reason(pon_rr);
	}
	if (sec_rr != RESTART_REASON_NOT_HANDLE)
		__sec_debug_set_restart_reason(sec_rr);
	__pr_err("(%s) restart_reason = 0x%x(0x%x)\n",
	       __func__, sec_rr, pon_rr);
}

void sec_debug_set_upload_cause(enum sec_debug_upload_cause_t type)
{
	if (unlikely(!upload_cause)) {
		pr_err("upload cause address unmapped.\n");
	} else {
		int cpu = get_cpu();
		per_cpu(sec_debug_upload_cause, cpu) = type;
		__raw_writel(type, upload_cause);
		put_cpu();

		pr_emerg("%x\n", type);
	}
}

enum sec_debug_upload_cause_t sec_debug_get_upload_cause(void)
{
	if (unlikely(!upload_cause)) {
		pr_err("upload cause address unmapped.\n");
		return UPLOAD_CAUSE_INIT;
	}

	return readl(upload_cause);
}

#ifdef CONFIG_SEC_PERIPHERAL_SECURE_CHK
void sec_peripheral_secure_check_fail(void)
{
	if (!sec_debug_is_enabled()) {
		sec_debug_set_qc_dload_magic(0);
		sec_debug_pm_restart("peripheral_hw_reset");
		/* never reach here */
	}

	panic("subsys - modem secure check fail");
}

void sec_modem_loading_fail_to_bootloader(void)
{
	if (!sec_debug_is_enabled()) {
		sec_debug_set_qc_dload_magic(0);
		sec_debug_pm_restart("peripheral_hw_reset");
		/* never reach here */
	}
}
#endif

#ifdef CONFIG_SEC_USER_RESET_DEBUG
static inline void sec_debug_store_backtrace(void)
{
	unsigned long flags;

	local_irq_save(flags);
	sec_debug_backtrace();
	local_irq_restore(flags);
}
#endif /* CONFIG_SEC_USER_RESET_DEBUG */

enum sec_debug_strncmp_func {
	SEC_STRNCMP = 0,
	SEC_STRNSTR,
	SEC_STRNCASECMP,
};

static inline bool __sec_debug_strncmp(const char *s1, const char *s2,
		size_t len, enum sec_debug_strncmp_func func)
{
	switch (func) {
	case SEC_STRNCMP:
		return !strncmp(s1, s2, len);
	case SEC_STRNSTR:
		return !!strnstr(s1, s2, len);
	case SEC_STRNCASECMP:
		return !strncasecmp(s1, s2, len);
	}

	pr_warn("%d is not a valid strncmp function!\n", (int)func);

	return false;
}

struct __upload_cause {
	const char *msg;
	enum sec_debug_upload_cause_t type;
	enum sec_debug_strncmp_func func;
};

struct __upload_cause upload_cause_st[] = {
	{ UPLOAD_MSG_USER_FAULT, UPLOAD_CAUSE_USER_FAULT, SEC_STRNCMP },
	{ UPLOAD_MSG_CRASH_KEY, UPLOAD_CAUSE_FORCED_UPLOAD, SEC_STRNCMP },
	{ UPLOAD_MSG_USER_CRASH_KEY, UPLOAD_CAUSE_USER_FORCED_UPLOAD, SEC_STRNCMP },
	{ UPLOAD_MSG_LONG_KEY_PRESS, UPLOAD_CAUSE_POWER_LONG_PRESS, SEC_STRNCMP },
	{ UPLOAD_MSG_PF_WD_BITE, UPLOAD_CAUSE_PF_WD_BITE, SEC_STRNSTR },
	{ UPLOAD_MSG_PF_WD_INIT_FAIL, UPLOAD_CAUSE_PF_WD_INIT_FAIL, SEC_STRNCMP },
	{ UPLOAD_MSG_PF_WD_RESTART_FAIL, UPLOAD_CAUSE_PF_WD_RESTART_FAIL, SEC_STRNCMP },
	{ UPLOAD_MSG_PF_WD_KICK_FAIL, UPLOAD_CAUSE_PF_WD_KICK_FAIL, SEC_STRNCMP },
	{ "taking too long", UPLOAD_CAUSE_SUBSYS_IF_TIMEOUT, SEC_STRNSTR },
	{ "CP Crash", UPLOAD_CAUSE_CP_ERROR_FATAL, SEC_STRNCMP },
	{ "adsp", UPLOAD_CAUSE_ADSP_ERROR_FATAL, SEC_STRNSTR },
	{ "slpi", UPLOAD_CAUSE_SLPI_ERROR_FATAL, SEC_STRNSTR },
	{ "spss", UPLOAD_CAUSE_SPSS_ERROR_FATAL, SEC_STRNSTR },
	{ "npu", UPLOAD_CAUSE_NPU_ERROR_FATAL, SEC_STRNSTR },
	{ "cdsp", UPLOAD_CAUSE_CDSP_ERROR_FATAL, SEC_STRNSTR },
	{ "MDM Crash", UPLOAD_CAUSE_MDM_ERROR_FATAL, SEC_STRNCMP },
	{ "unrecoverable external_modem", UPLOAD_CAUSE_MDM_CRITICAL_FATAL, SEC_STRNSTR },
	{ "external_modem", UPLOAD_CAUSE_MDM_ERROR_FATAL, SEC_STRNSTR },
	{ "esoc0 crashed", UPLOAD_CAUSE_MDM_ERROR_FATAL, SEC_STRNSTR },
	{ "modem", UPLOAD_CAUSE_MODEM_RST_ERR, SEC_STRNSTR },
	{ "riva", UPLOAD_CAUSE_RIVA_RST_ERR, SEC_STRNSTR },
	{ "lpass", UPLOAD_CAUSE_LPASS_RST_ERR, SEC_STRNSTR },
	{ "dsps", UPLOAD_CAUSE_DSPS_RST_ERR, SEC_STRNSTR },
	{ "subsys", UPLOAD_CAUSE_PERIPHERAL_ERR, SEC_STRNCASECMP },
	{ "SMPL", UPLOAD_CAUSE_SMPL, SEC_STRNSTR },
#if defined(CONFIG_SEC_QUEST)
	{ "crypto_test", UPLOAD_CAUSE_QUEST_CRYPTO, SEC_STRNCMP },
	{ "icache_test", UPLOAD_CAUSE_QUEST_ICACHE, SEC_STRNCMP },
	{ "ccoherency_test", UPLOAD_CAUSE_QUEST_CACHECOHERENCY, SEC_STRNCMP },
	{ "suspend_test", UPLOAD_CAUSE_QUEST_SUSPEND, SEC_STRNCMP },
	{ "vddmin_test", UPLOAD_CAUSE_QUEST_VDDMIN, SEC_STRNCMP },
	{ "qmesa_ddr_test", UPLOAD_CAUSE_QUEST_QMESADDR, SEC_STRNCMP },
	{ "qmesa_cache_test", UPLOAD_CAUSE_QUEST_QMESACACHE, SEC_STRNCMP },
	{ "pmic_test", UPLOAD_CAUSE_QUEST_PMIC, SEC_STRNCMP },
	{ "ufs_test", UPLOAD_CAUSE_QUEST_UFS, SEC_STRNCMP },
	{ "sdcard_test", UPLOAD_CAUSE_QUEST_SDCARD, SEC_STRNCMP },
	{ "sensor_test", UPLOAD_CAUSE_QUEST_SENSOR, SEC_STRNCMP },
	{ "sensorprobe_test", UPLOAD_CAUSE_QUEST_SENSORPROBE, SEC_STRNCMP },
	{ "naturescene_test", UPLOAD_CAUSE_QUEST_NATURESCENE, SEC_STRNCMP },
	{ "a75g_test", UPLOAD_CAUSE_QUEST_A75G, SEC_STRNCMP },
	{ "q65g_test", UPLOAD_CAUSE_QUEST_Q65G, SEC_STRNCMP },
	{ "thermal_test", UPLOAD_CAUSE_QUEST_THERMAL, SEC_STRNCMP },
	{ "qdaf_fail", UPLOAD_CAUSE_QUEST_QDAF_FAIL, SEC_STRNCMP },
	{ "zip_unzip_test", UPLOAD_CAUSE_QUEST_ZIP_UNZIP, SEC_STRNCMP },
	{ "quest_fail", UPLOAD_CAUSE_QUEST_FAIL, SEC_STRNCMP },
	{ "aoss_thermal_diff", UPLOAD_CAUSE_QUEST_AOSSTHERMALDIFF, SEC_STRNCMP },
	{ "stressapptest_test", UPLOAD_CAUSE_QUEST_STRESSAPPTEST, SEC_STRNCMP },	
#endif
};

void sec_debug_upload_cause_str(enum sec_debug_upload_cause_t type,
		char *str, size_t len)
{
	size_t i;

	if (type == UPLOAD_CAUSE_KERNEL_PANIC)
		strlcpy(str, "kernel_panic", len);
	else if (type == UPLOAD_CAUSE_INIT)
		strlcpy(str, "unknown_error", len);
	else if (type == UPLOAD_CAUSE_NON_SECURE_WDOG_BARK)
		strlcpy(str, "watchdog_bark", len);
	else {
		for (i = 0; i < ARRAY_SIZE(upload_cause_st); i++) {
			if (type == upload_cause_st[i].type) {
				snprintf(str, len, "%s", upload_cause_st[i].msg);
				return;
			}
		}
		strlcpy(str, "unknown_reset", len);
	}
}

static bool sec_debug_platform_lockup_suspected(char *panic_msg)
{
	if (!strcmp(panic_msg, UPLOAD_MSG_CRASH_KEY) ||
		!strcmp(panic_msg, UPLOAD_MSG_USER_CRASH_KEY) ||
		!strcmp(panic_msg, UPLOAD_MSG_LONG_KEY_PRESS) ||
		!strncmp(panic_msg, UPLOAD_MSG_PF_WD_BITE, strlen(UPLOAD_MSG_PF_WD_BITE)) ||
		!strcmp(panic_msg, UPLOAD_MSG_PF_WD_INIT_FAIL) ||
		!strcmp(panic_msg, UPLOAD_MSG_PF_WD_RESTART_FAIL) ||
		!strcmp(panic_msg, UPLOAD_MSG_PF_WD_KICK_FAIL))
		return true;

	return false;
}

static int sec_debug_panic_handler(struct notifier_block *nb,
		unsigned long l, void *buf)
{
#define MAX_STR_LEN 80
	size_t len, i;

	emerg_pet_watchdog(); /* CTC-should be modify */
#ifdef CONFIG_SEC_USER_RESET_DEBUG
	sec_debug_store_backtrace();
#endif
	sec_debug_set_upload_magic(RESTART_REASON_SEC_DEBUG_MODE);

	__pr_err("%s :%s\n", __func__, (char *)buf);

	for (i = 0; i < ARRAY_SIZE(upload_cause_st); i++) {
		len = strnlen(buf, MAX_STR_LEN);
		if (__sec_debug_strncmp(buf, upload_cause_st[i].msg, len,
					upload_cause_st[i].func)) {
			sec_debug_set_upload_cause(upload_cause_st[i].type);
			break;
		}
	}

	if (i == ARRAY_SIZE(upload_cause_st))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_KERNEL_PANIC);

	if (!sec_debug_is_enabled()) {
#ifdef CONFIG_SEC_DEBUG_LOW_LOG
		__sec_debug_hw_reset();
#endif
		/* SEC will get reset_summary.html in debug low.
		 * reset_summary.html need more information about abnormal reset
		 * or kernel panic.
		 * So we skip as below
		 */
		/* return -EPERM; */
	}

	/* enable after SSR feature */
	/* ssr_panic_handler_for_sec_dbg(); */

	/* platform lockup suspected, it needs more info */
	if (sec_debug_platform_lockup_suspected((char *)buf)) {
		show_state_filter(TASK_UNINTERRUPTIBLE);
		dump_memory_info();
		dump_cpu_stat();
	}

	/* save context here so that function call after this point doesn't
	 * corrupt stacks below the saved sp
	 */
	sec_debug_save_context();
	__sec_debug_hw_reset();

	return 0;
}

void sec_debug_prepare_for_wdog_bark_reset(void)
{
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	sec_delay_check = 0;
#endif
	sec_debug_set_upload_magic(RESTART_REASON_SEC_DEBUG_MODE);
	sec_debug_set_upload_cause(UPLOAD_CAUSE_NON_SECURE_WDOG_BARK);
}

static struct notifier_block nb_reboot_block = {
	.notifier_call = sec_debug_normal_reboot_handler,
};

static struct notifier_block nb_panic_block = {
	.notifier_call = sec_debug_panic_handler,
	.priority = -1,
};

#ifdef CONFIG_OF
static int __init __sec_debug_dt_addr_init(void)
{
	struct device_node *np;

	/* Using bottom of sec_dbg DDR address range
	 * for writing restart reason
	 */
	np = of_find_compatible_node(NULL, NULL,
			"qcom,msm-imem-restart_reason");
	if (unlikely(!np)) {
		pr_err("unable to find DT imem restart reason node\n");
		return -ENODEV;
	}

	qcom_restart_reason = of_iomap(np, 0);
	if (unlikely(!qcom_restart_reason)) {
		pr_err("unable to map imem restart reason offset\n");
		return -ENODEV;
	}

	/* check restart_reason address here */
	pr_emerg("restart_reason addr : 0x%p(0x%llx)\n",
			qcom_restart_reason,
			(unsigned long long)virt_to_phys(qcom_restart_reason));

	/* Using bottom of sec_dbg DDR address range for writing upload_cause */
	np = of_find_compatible_node(NULL, NULL, "qcom,msm-imem-upload_cause");
	if (unlikely(!np)) {
		pr_err("unable to find DT imem upload cause node\n");
		return -ENODEV;
	}

	upload_cause = of_iomap(np, 0);
	if (unlikely(!upload_cause)) {
		pr_err("unable to map imem upload_cause offset\n");
		return -ENODEV;
	}

	/* check upload_cause here */
	pr_emerg("upload_cause addr : 0x%p(0x%llx)\n", upload_cause,
			(unsigned long long)virt_to_phys(upload_cause));

#if IS_ENABLED(CONFIG_POWER_RESET_QCOM_DOWNLOAD_MODE)
	np = of_find_compatible_node(NULL, NULL, "qcom,msm-watchdog");
	if (unlikely(!np)) {
		pr_err("unable to find DT qcom,msm-watchdog node\n");
		return -ENODEV;
	}

	if (of_device_is_available(np)) {
		watchdog_base = of_iomap(np, 0);
		if (unlikely(!watchdog_base)) {
			pr_err("unable to map watchdog_base offset\n");
			return -ENODEV;
		}

		/* check upload_cause here */
		pr_emerg("watchdog_base addr : 0x%p(0x%llx)\n", watchdog_base,
				(unsigned long long)virt_to_phys(watchdog_base));
	}
#endif

	return 0;
}
#else /* CONFIG_OF */
static int __init __sec_debug_dt_addr_init(void) { return 0; }
#endif /* CONFIG_OF */

static int __init force_upload_setup(char *en)
{
	get_option(&en, &force_upload);
	return 0;
}
early_param("androidboot.force_upload", force_upload_setup);

/* for sec debug level */
static int __init sec_debug_level_setup(char *str)
{
	get_option(&str, &sec_dbg_level);
	return 0;
}
early_param("androidboot.debug_level", sec_debug_level_setup);

static int __init sec_debug_init(void)
{
	int ret;

	ret = __sec_debug_dt_addr_init();
	if (unlikely(ret < 0))
		return ret;

	register_reboot_notifier(&nb_reboot_block);
	atomic_notifier_chain_register(&panic_notifier_list, &nb_panic_block);

	sec_debug_set_upload_magic(RESTART_REASON_SEC_DEBUG_MODE);
	sec_debug_set_upload_cause(UPLOAD_CAUSE_INIT);

	/* TODO: below code caused reboot fail when debug level LOW */
	switch (sec_dbg_level) {
	case ANDROID_DEBUG_LEVEL_LOW:
#ifdef CONFIG_SEC_FACTORY
	case ANDROID_DEBUG_LEVEL_MID:
#endif

#if defined(CONFIG_ARCH_ATOLL) || defined(CONFIG_ARCH_SEC_SM7150)
		if (!force_upload)
			qcom_scm_disable_sdi();
#endif
		break;
	}

#ifdef CONFIG_SEC_LOG_STORE_LAST_KMSG
	if (reboot_multicmd)
		reboot_multicmd = 1;
	else
		reboot_multicmd = 0;
#else
	reboot_multicmd = 0;
#endif
	return 0;
}
arch_initcall_sync(sec_debug_init);

bool sec_debug_is_enabled(void)
{
	switch (sec_dbg_level) {
	case ANDROID_DEBUG_LEVEL_LOW:
#ifdef CONFIG_SEC_FACTORY
	case ANDROID_DEBUG_LEVEL_MID:
#endif
		return !!(force_upload);
	}

	return !!(enable);
}

unsigned int sec_debug_level(void)
{
	return sec_dbg_level;
}

#ifdef CONFIG_SEC_SSR_DEBUG_LEVEL_CHK
int sec_debug_is_enabled_for_ssr(void)
{
	return enable_cp_debug;
}
#endif

static inline void __dump_one_task_info(struct task_struct *tsk,
		bool is_process)
{
	char stat_array[] = { 'R', 'S', 'D' };
	char stat_ch;

	stat_ch = tsk->state <= TASK_UNINTERRUPTIBLE ?
		stat_array[tsk->state] : '?';
	__pr_info("%8d  %8llu  %8llu  %16llu  %c (%ld)  %px  %c %s\n",
		tsk->pid, (unsigned long long)tsk->utime,
		(unsigned long long)tsk->stime,
		tsk->se.exec_start, stat_ch, tsk->state,
		tsk, is_process ? '*' : ' ', tsk->comm);

	if (tsk->state == TASK_RUNNING || tsk->state == TASK_UNINTERRUPTIBLE)
		show_stack(tsk, NULL);
}

#define __HLINE_LEFT	" -------------------------------------------"
#define __HLINE_RIGHT	"--------------------------------------------\n"

void dump_memory_info(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	show_mem(0, NULL);
#else
	show_mem(0);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	dump_tasks(NULL, NULL);
#endif
}

void dump_all_task_info(void)
{
	struct task_struct *p;
	struct task_struct *t;

	__pr_info("\n");
	__pr_info(" current proc : %d %s\n",
			current->pid, current->comm);
	__pr_info(__HLINE_LEFT __HLINE_RIGHT);
	__pr_info("%8s  %8s  %8s  %16s  %5s  %s\n",
			"pid", "uTime", "sTime", "exec(ns)",
			"stat", "task_struct");
	__pr_info(__HLINE_LEFT __HLINE_RIGHT);

	for_each_process_thread(p, t) {
		__dump_one_task_info(t, p == t);
	}

	__pr_info(__HLINE_LEFT __HLINE_RIGHT);
}

/* TODO: this is a modified version of 'show_stat' in 'fs/proc/stat.c'
 * this function should be adapted for each kernel version
 */
#ifndef arch_irq_stat_cpu
#define arch_irq_stat_cpu(cpu) 0
#endif
#ifndef arch_irq_stat
#define arch_irq_stat() 0
#endif

void dump_cpu_stat(void)
{
	int i, j;
	u64 user, nice, system, idle, iowait, irq, softirq, steal;
	u64 guest, guest_nice;
	u64 sum = 0;
	u64 sum_softirq = 0;
	unsigned int per_softirq_sums[NR_SOFTIRQS] = {0};
	struct timespec64 boottime;

	user = nice = system = idle = iowait =
		irq = softirq = steal = 0;
	guest = guest_nice = 0;
	getboottime64(&boottime);

	for_each_possible_cpu(i) {
		user += kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice += kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system += kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle += kcpustat_cpu(i).cpustat[CPUTIME_IDLE];
		iowait += kcpustat_cpu(i).cpustat[CPUTIME_IOWAIT];
		irq += kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq += kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal += kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest += kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice += kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];
		sum += kstat_cpu_irqs_sum(i);
		sum += arch_irq_stat_cpu(i);

		for (j = 0; j < NR_SOFTIRQS; j++) {
			unsigned int softirq_stat = kstat_softirqs_cpu(j, i);

			per_softirq_sums[j] += softirq_stat;
			sum_softirq += softirq_stat;
		}
	}
	sum += arch_irq_stat();

	__pr_info("\n");
	__pr_info(__HLINE_LEFT __HLINE_RIGHT);
	__pr_info("      %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s\n",
			"user", "nice", "system", "idle", "iowait", "irq",
			"softirq", "steal", "guest", "guest_nice");
	__pr_info("cpu   %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu\n",
			nsec_to_clock_t(user),
			nsec_to_clock_t(nice),
			nsec_to_clock_t(system),
			nsec_to_clock_t(idle),
			nsec_to_clock_t(iowait),
			nsec_to_clock_t(irq),
			nsec_to_clock_t(softirq),
			nsec_to_clock_t(steal),
			nsec_to_clock_t(guest),
			nsec_to_clock_t(guest_nice));

	for_each_online_cpu(i) {
		/* Copy values here to work around gcc-2.95.3, gcc-2.96 */
		user = kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice = kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system = kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle = kcpustat_cpu(i).cpustat[CPUTIME_IDLE];
		iowait = kcpustat_cpu(i).cpustat[CPUTIME_IOWAIT];
		irq = kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq = kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal = kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest = kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice = kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];
		__pr_info("cpu%-2d %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu\n",
				i,
				nsec_to_clock_t(user),
				nsec_to_clock_t(nice),
				nsec_to_clock_t(system),
				nsec_to_clock_t(idle),
				nsec_to_clock_t(iowait),
				nsec_to_clock_t(irq),
				nsec_to_clock_t(softirq),
				nsec_to_clock_t(steal),
				nsec_to_clock_t(guest),
				nsec_to_clock_t(guest_nice));
	}

	__pr_info(__HLINE_LEFT __HLINE_RIGHT);
	__pr_info("intr %llu\n", (unsigned long long)sum);

	/* sum again ? it could be updated? */
	for_each_irq_nr(j)
		if (kstat_irqs(j))
			__pr_info(" irq-%d : %u\n", j, kstat_irqs(j));

	__pr_info(__HLINE_LEFT __HLINE_RIGHT);
	__pr_info("\nctxt %llu\n"
		"btime %llu\n"
		"processes %lu\n"
		"procs_running %lu\n"
		"procs_blocked %lu\n",
		nr_context_switches(),
		(unsigned long long)boottime.tv_sec,
		total_forks,
		nr_running(),
		nr_iowait());

	__pr_info(__HLINE_LEFT __HLINE_RIGHT);
	__pr_info("softirq %llu\n", (unsigned long long)sum_softirq);

	for (i = 0; i < NR_SOFTIRQS; i++)
		__pr_info(" softirq-%d : %u\n", i, per_softirq_sums[i]);
	__pr_info("\n");

	__pr_info(__HLINE_LEFT __HLINE_RIGHT);
}

char *erased_command_line;

char *__init sec_debug_get_erased_command_line(void)
{
	size_t cmdline_len;
	size_t len;
	char *token_begin, *token_end, *to_be_zero;
	const char *to_be_deleted[] = {
		"ap_serial=0x",
		"serialno=",
		"em.did=",
		"androidboot.un=W",
		"androidboot.un=H",
	};
	size_t i;

	if (erased_command_line)
		return erased_command_line;

	cmdline_len = strlen(boot_command_line) + 1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
	erased_command_line = memblock_alloc(cmdline_len, SMP_CACHE_BYTES);
#else
	erased_command_line = memblock_virt_alloc(cmdline_len, 0);
#endif
	strlcpy(erased_command_line, saved_command_line, cmdline_len);

	for (i = 0; i < ARRAY_SIZE(to_be_deleted); i++) {
		len = strlen(to_be_deleted[i]);
		token_begin = strstr(erased_command_line,
				to_be_deleted[i]);
		if (!token_begin)
			continue;

		token_end = strstr(token_begin, " ");
		if (!token_end)	/* last command line option */
			token_end = &(erased_command_line[cmdline_len - 1]);
		to_be_zero = &(token_begin[len]);
		while (to_be_zero < token_end)
			*to_be_zero++ = '0';
	}

	return erased_command_line;
}

/* FIXME: this variable is not referenced anywhere */
static unsigned int runtime_debug_val;
module_param_named(runtime_debug_val, runtime_debug_val, uint, 0644);

#ifdef CONFIG_SEC_FILE_LEAK_DEBUG
static inline int __sec_debug_print_file_list(void)
{
	size_t i;
	unsigned int nCnt;
	struct file *file;
	struct files_struct *files = current->files;
	const char *pRootName;
	const char *pFileName;
	int ret = 0;

	nCnt = files->fdt->max_fds;

	pr_err(" [Opened file list of process %s(PID:%d, TGID:%d) :: %d]\n",
		current->group_leader->comm, current->pid, current->tgid, nCnt);

	for (i = 0; i < nCnt; i++) {
		rcu_read_lock();
		file = fcheck_files(files, i);

		pRootName = NULL;
		pFileName = NULL;

		if (file) {
			if (file->f_path.mnt &&
			    file->f_path.mnt->mnt_root &&
			    file->f_path.mnt->mnt_root->d_name.name)
				pRootName =
					file->f_path.mnt->mnt_root->d_name.name;

			if (file->f_path.dentry &&
			    file->f_path.dentry->d_name.name)
				pFileName = file->f_path.dentry->d_name.name;

			pr_err("[%04zd]%s%s\n", i,
					pRootName ? pRootName : "null",
					pFileName ? pFileName : "null");
			ret++;
		}
		rcu_read_unlock();
	}

	return (ret == nCnt) ? 1 : 0;
}

/* FIXME: this function is not referenced anywhere */
void __deprecated sec_debug_EMFILE_error_proc(void)
{
	pr_err("Too many open files(%d:%s)\n",
		current->tgid, current->group_leader->comm);

	if (!sec_debug_is_enabled())
		return;

	/* We check EMFILE error in only "system_server",
	 * "mediaserver" and "surfaceflinger" process.
	 */
	if (!strncmp(current->group_leader->comm,
			"system_server", ARRAY_SIZE("system_server")) ||
	    !strncmp(current->group_leader->comm,
			"mediaserver", ARRAY_SIZE("mediaserver")) ||
	    !strncmp(current->group_leader->comm,
			"surfaceflinger", ARRAY_SIZE("surfaceflinger"))) {
		if (__sec_debug_print_file_list() == 1)
			panic("Too many open files");
	}
}
#endif /* CONFIG_SEC_FILE_LEAK_DEBUG */

/* FIXME: this function is not referenced anywhere */
void __deprecated sec_debug_print_model(struct seq_file *m,
		const char *cpu_name)
{
	u32 cpuid = read_cpuid_id();

	seq_printf(m, "model name\t: %s rev %d (%s)\n",
		   cpu_name, cpuid & 15, ELF_PLATFORM);
}

/* FIXME: this function is not referenced anywhere */
void __deprecated sec_debug_set_thermal_upload(void)
{
	pr_emerg("set thermal upload cause\n");
	sec_debug_set_upload_magic(0x776655ee);
	sec_debug_set_upload_cause(UPLOAD_CAUSE_POWER_THERMAL_RESET);
}
