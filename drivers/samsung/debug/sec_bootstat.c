// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_bootstat.c
 *
 * COPYRIGHT(C) 2014-2019 Samsung Electronics Co., Ltd. All Right Reserved.
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
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
#include <soc/qcom/boot_stats.h>
#endif

#include <linux/sec_class.h>
#include <linux/sec_bootstat.h>

#include "sec_debug_internal.h"

#define BOOT_EVT_PREFIX			"!@Boot"
#define BOOT_EVT_PREFIX_NONE		""
#define BOOT_EVT_PREFIX_PLATFORM	": "
#define BOOT_EVT_PREFIX_RIL		"_SVC : "
#define BOOT_EVT_PREFIX_DEBUG		"_DEBUG: "
#define BOOT_EVT_PREFIX_SYSTEMSERVER		"_SystemServer: "

#define DEFAULT_BOOT_STAT_FREQ		32768

#define DELAY_TIME_EBS		10000
#define MAX_EVENTS_EBS		100

static struct device *sec_bsp_dev;

static unsigned int __is_boot_recovery;
static bool bootcompleted = false;
static bool ebs_finished = false;
static unsigned int boot_complete_time = 0;
static int events_ebs = 0;

static const char *boot_prefix[16] = {
	BOOT_EVT_PREFIX BOOT_EVT_PREFIX_PLATFORM,
	BOOT_EVT_PREFIX BOOT_EVT_PREFIX_RIL,
	BOOT_EVT_PREFIX BOOT_EVT_PREFIX_DEBUG,
	BOOT_EVT_PREFIX BOOT_EVT_PREFIX_SYSTEMSERVER,
	BOOT_EVT_PREFIX_NONE
};

enum boot_events_prefix {
	EVT_PLATFORM,
	EVT_RIL,
	EVT_DEBUG,
	EVT_SYSTEMSERVER,
	EVT_INVALID,
};

enum boot_events_type {
	SYSTEM_START_UEFI,
	SYSTEM_START_LINUXLOADER,
	SYSTEM_START_LINUX,
	SYSTEM_START_INIT_PROCESS,
	PLATFORM_START_PRELOAD,
	PLATFORM_END_PRELOAD,
	PLATFORM_START_INIT_AND_LOOP,
	PLATFORM_START_PACKAGEMANAGERSERVICE,
	PLATFORM_END_PACKAGEMANAGERSERVICE,
	PLATFORM_START_NETWORK,
	PLATFORM_END_NETWORK,
	PLATFORM_END_INIT_AND_LOOP,
	PLATFORM_PERFORMENABLESCREEN,
	PLATFORM_ENABLE_SCREEN,
	PLATFORM_BOOT_COMPLETE,
	PLATFORM_FINISH_USER_UNLOCKED_COMPLETED,
	PLATFORM_SET_ICON_VISIBILITY,
	PLATFORM_LAUNCHER_ONCREATE,
	PLATFORM_LAUNCHER_ONRESUME,
	PLATFORM_LAUNCHER_LOADERTASK_RUN,
	PLATFORM_LAUNCHER_FINISHFIRSTBIND,
	PLATFORM_VOICE_SVC,
	PLATFORM_DATA_SVC,
	PLATFORM_PHONEAPP_ONCREATE,
	RIL_UNSOL_RIL_CONNECTED,
	RIL_SETRADIOPOWER_ON,
	RIL_SETUICCSUBSCRIPTION,
	RIL_SIM_RECORDSLOADED,
	RIL_RUIM_RECORDSLOADED,
	RIL_SETUPDATA_RECORDSLOADED,
	RIL_SETUPDATACALL,
	RIL_RESPONSE_SETUPDATACALL,
	RIL_DATA_CONNECTION_ATTACHED,
	RIL_DCT_IMSI_READY,
	RIL_COMPLETE_CONNECTION,
	RIL_CS_REG,
	RIL_GPRS_ATTACH,
#ifdef CONFIG_SEC_FACTORY
	FACTORY_BOOT_COMPLETE,
#endif /* CONFIG_SEC_FACTORY */
	NUM_BOOT_EVENTS,
};

struct boot_event {
	enum boot_events_type type;
	enum boot_events_prefix prefix;
	const char *string;
	unsigned int time;
	unsigned int ktime;
};

static struct boot_event boot_initcall[] = {
	{0, EVT_INVALID, "early",},
	{0, EVT_INVALID, "core",},
	{0, EVT_INVALID, "postcore",},
	{0, EVT_INVALID, "arch",},
	{0, EVT_INVALID, "subsys",},
	{0, EVT_INVALID, "fs",},
	{0, EVT_INVALID, "device",},
	{0, EVT_INVALID, "late",},
	{0, EVT_INVALID, NULL,}
};

static int num_events;
static int boot_events_seq[NUM_BOOT_EVENTS];

static struct boot_event boot_events[] = {
	{SYSTEM_START_UEFI, EVT_INVALID,
			"Uefi start", 0, 0},
	{SYSTEM_START_LINUXLOADER, EVT_INVALID,
			"Linux loader start", 0, 0},
	{SYSTEM_START_LINUX, EVT_INVALID,
			"Linux start", 0, 0},
	{SYSTEM_START_INIT_PROCESS, EVT_PLATFORM,
			"start init process", 0, 0},
	{PLATFORM_START_PRELOAD, EVT_PLATFORM,
			"Begin of preload()", 0, 0},
	{PLATFORM_END_PRELOAD, EVT_PLATFORM,
			"End of preload()", 0, 0},
	{PLATFORM_START_INIT_AND_LOOP, EVT_PLATFORM,
			"Entered the Android system server!", 0, 0},
	{PLATFORM_START_PACKAGEMANAGERSERVICE, EVT_PLATFORM,
			"Start PackageManagerService", 0, 0},
	{PLATFORM_END_PACKAGEMANAGERSERVICE, EVT_PLATFORM,
			"End PackageManagerService", 0, 0},
	{PLATFORM_START_NETWORK, EVT_DEBUG,
			"start networkManagement", 0, 0},
	{PLATFORM_END_NETWORK, EVT_DEBUG,
			"end networkManagement", 0, 0},
	{PLATFORM_END_INIT_AND_LOOP, EVT_PLATFORM,
			"Loop forever", 0, 0},
	{PLATFORM_PERFORMENABLESCREEN, EVT_PLATFORM,
			"performEnableScreen", 0, 0},
	{PLATFORM_ENABLE_SCREEN, EVT_PLATFORM,
			"Enabling Screen!", 0, 0},
	{PLATFORM_BOOT_COMPLETE, EVT_PLATFORM,
			"bootcomplete", 0, 0},
	{PLATFORM_FINISH_USER_UNLOCKED_COMPLETED, EVT_DEBUG,
			"finishUserUnlockedCompleted", 0, 0},
	{PLATFORM_SET_ICON_VISIBILITY, EVT_PLATFORM,
			"setIconVisibility: ims_volte: [SHOW]", 0, 0},
	{PLATFORM_LAUNCHER_ONCREATE, EVT_DEBUG,
	        "Launcher.onCreate()", 0, 0},
	{PLATFORM_LAUNCHER_ONRESUME, EVT_DEBUG,
	        "Launcher.onResume()", 0, 0},
	{PLATFORM_LAUNCHER_LOADERTASK_RUN, EVT_DEBUG,
	        "Launcher.LoaderTask.run() start", 0, 0},
	{PLATFORM_LAUNCHER_FINISHFIRSTBIND, EVT_DEBUG,
	        "Launcher - FinishFirstBind", 0, 0},
	{PLATFORM_VOICE_SVC, EVT_PLATFORM,
			"Voice SVC is acquired", 0, 0},
	{PLATFORM_DATA_SVC, EVT_PLATFORM,
			"Data SVC is acquired", 0, 0},
	{PLATFORM_PHONEAPP_ONCREATE, EVT_RIL,
			"PhoneApp OnCrate", 0, 0},
	{RIL_UNSOL_RIL_CONNECTED, EVT_RIL,
			"RIL_UNSOL_RIL_CONNECTED", 0, 0},
	{RIL_SETRADIOPOWER_ON, EVT_RIL,
			"setRadioPower on", 0, 0},
	{RIL_SETUICCSUBSCRIPTION, EVT_RIL,
			"setUiccSubscription", 0, 0},
	{RIL_SIM_RECORDSLOADED, EVT_RIL,
			"SIM onAllRecordsLoaded", 0, 0},
	{RIL_RUIM_RECORDSLOADED, EVT_RIL,
			"RUIM onAllRecordsLoaded", 0, 0},
	{RIL_SETUPDATA_RECORDSLOADED, EVT_RIL,
			"SetupDataRecordsLoaded", 0, 0},
	{RIL_SETUPDATACALL, EVT_RIL,
			"setupDataCall", 0, 0},
	{RIL_RESPONSE_SETUPDATACALL, EVT_RIL,
			"Response setupDataCall", 0, 0},
	{RIL_DATA_CONNECTION_ATTACHED, EVT_RIL,
			"onDataConnectionAttached", 0, 0},
	{RIL_DCT_IMSI_READY, EVT_RIL,
			"IMSI Ready", 0, 0},
	{RIL_COMPLETE_CONNECTION, EVT_RIL,
			"completeConnection", 0, 0},
	{RIL_CS_REG, EVT_RIL,
			"CS Registered", 0, 0},
	{RIL_GPRS_ATTACH, EVT_RIL,
			"GPRS Attached", 0, 0},
#ifdef CONFIG_SEC_FACTORY
	{FACTORY_BOOT_COMPLETE, EVT_PLATFORM,
			"Factory Process [Boot Completed]", 0, 0},
#endif /* CONFIG_SEC_FACTORY */
	{0, EVT_INVALID, NULL, 0, 0},
};

struct suspend_resume_event {
	unsigned int type;
	unsigned int state;
	const char *string;
	unsigned int cnt;
	unsigned int time;
	unsigned int ktime;
};

static unsigned int suspend_resume_cnt;

enum suspend_resume_type {
	TYPE_SUSPEND,
	TYPE_RESUME,
};
enum suspend_resume_state {
	SUSPEND_EVENT_1,
	SUSPEND_EVENT_2,
	RESUME_EVENT_1,
	RESUME_EVENT_2,
};

static struct suspend_resume_event suspend_resume_event[] = {
	{TYPE_SUSPEND, SUSPEND_EVENT_1, "Syncing FS+", 0, 0, 0},
	{TYPE_SUSPEND, SUSPEND_EVENT_2, "Syncing FS-", 0, 0, 0},
	{TYPE_RESUME, RESUME_EVENT_1, "Freeze User Process+", 0, 0, 0},
	{TYPE_RESUME, RESUME_EVENT_2, "Freeze User Process-", 0, 0, 0},
	{TYPE_SUSPEND, SUSPEND_EVENT_1, "Freeze Remaining+", 0, 0, 0},
	{TYPE_SUSPEND, SUSPEND_EVENT_1, "Freeze Remaining-", 0, 0, 0},
	{TYPE_SUSPEND, SUSPEND_EVENT_1, "Suspending console", 0, 0, 0},
	{0, 0, NULL, 0, 0, 0},
};

struct enhanced_boot_time {
	struct list_head next;
	char buf[MAX_LENGTH_OF_SYSTEMSERVER_LOG];
	unsigned int time;
	unsigned int ktime;
};

LIST_HEAD(device_init_time_list);
LIST_HEAD(systemserver_init_time_list);

LIST_HEAD(enhanced_boot_time_list);
static DEFINE_SPINLOCK(enhanced_boot_time_list_lock);

static int __init boot_recovery(char *str)
{
	int temp = 0;

	if (get_option(&str, &temp)) {
		__is_boot_recovery = temp;
		return 0;
	}

	return -EINVAL;
}
early_param("androidboot.boot_recovery", boot_recovery);

unsigned int is_boot_recovery(void)
{
	return __is_boot_recovery;
}

static int sec_boot_stat_proc_show(struct seq_file *m, void *v)
{
	size_t i;
	unsigned long delta;
	unsigned long freq = (unsigned long)get_boot_stat_freq();
	unsigned long time, ktime, prev_time;
	char boot_string[256];
	struct device_init_time_entry *entry;
	struct systemserver_init_time_entry *systemserver_entry;

	if (!freq)
		freq = DEFAULT_BOOT_STAT_FREQ;

	seq_printf(m, "%-48s%s%13s%13s\n", "boot event", "time(msec)",
				"ktime(msec)", "delta(msec)");
	seq_puts(m, "------------------------------------------");
	seq_puts(m, "-----------------------------------------\n");

	/* print boot_events logged */
	for (i = 0, prev_time = 0; i < num_events; i++) {
		int seq = boot_events_seq[i];

		time = (unsigned long)boot_events[seq].time * 1000 / freq;
		ktime = (unsigned long)boot_events[seq].ktime;
		delta = i ? time - prev_time : 0;

		snprintf(boot_string, ARRAY_SIZE(boot_string), "%s%s",
				boot_prefix[boot_events[seq].prefix],
				boot_events[seq].string);

		seq_printf(m, "%-45s : %9lu    %9lu    %9lu\n",
				boot_string, time, ktime, delta);

		prev_time = time;
	}

	seq_puts(m, "------------------------------------------");
	seq_puts(m, "-----------------------------------------\n");
	seq_puts(m, "kernel extra info\n\n");

	for (i = 0, prev_time = 0; boot_initcall[i].string; i++) {
		time = (unsigned long)boot_initcall[i].time * 1000 / freq;
		ktime = (unsigned long)boot_initcall[i].ktime;
		delta = ktime - prev_time;

		seq_printf(m, "%-45s : %9lu    %9lu    %9lu\n",
				boot_initcall[i].string, time, ktime, delta);

		prev_time = ktime;
	}

	seq_printf(m, "\ndevice init time over %d ms\n\n",
			DEVICE_INIT_TIME_100MS / 1000);

	list_for_each_entry(entry, &device_init_time_list, next)
		seq_printf(m, "%-20s : %lld usces\n",
			   entry->buf, entry->duration);

	seq_puts(m, "------------------------------------------");
	seq_puts(m, "-----------------------------------------\n");
	seq_puts(m, "SystemServer services that took long time\n\n");
	list_for_each_entry(systemserver_entry,
			&systemserver_init_time_list, next)
		seq_printf(m, "%s\n", systemserver_entry->buf);

	return 0;
}

static int sec_boot_stat_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_boot_stat_proc_show, NULL);
}

static const struct file_operations sec_boot_stat_proc_fops = {
	.open    = sec_boot_stat_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

void sec_bootstat_add_initcall(const char *s)
{
	size_t i = 0;
	u64 t;

	while (boot_initcall[i].string != NULL) {
		if (!strcmp(s, boot_initcall[i].string)) {
			t = local_clock();
			do_div(t, 1000000ULL);
			boot_initcall[i].ktime = (unsigned int)t;
			break;
		}
		i = i + 1;
	}
}

void sec_boot_stat_record_systemserver(const char *c)
{
	struct systemserver_init_time_entry *entry;

	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return;

	strlcpy(entry->buf, c, MAX_LENGTH_OF_SYSTEMSERVER_LOG);

	entry->buf[MAX_LENGTH_OF_SYSTEMSERVER_LOG - 1] = 0;

	list_add(&entry->next, &systemserver_init_time_list);
}

void sec_boot_stat_record(int idx, int time)
{
	u64 t;

	boot_events[idx].time = time;
	if (idx >= SYSTEM_START_INIT_PROCESS && idx < NUM_BOOT_EVENTS) {
		t = local_clock();
		do_div(t, 1000000ULL);
		boot_events[idx].ktime = (unsigned int)t;
	}
	boot_events_seq[num_events++] = idx;
}

void sec_enhanced_boot_stat_record(const char *buf)
{
	u64 t;
	int time = get_boot_stat_time();
	struct enhanced_boot_time *entry;
	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return;
	strncpy(entry->buf, buf, MAX_LENGTH_OF_SYSTEMSERVER_LOG);
	entry->buf[MAX_LENGTH_OF_SYSTEMSERVER_LOG - 1] = 0;
	entry->time = time;
	t = local_clock();
	do_div(t, 1000000ULL);
	entry->ktime = (unsigned int)t;
	spin_lock(&enhanced_boot_time_list_lock);
	list_add(&entry->next, &enhanced_boot_time_list);
	spin_unlock(&enhanced_boot_time_list_lock);
	events_ebs++;
}

void sec_boot_stat_add(const char *c)
{
	size_t i;
	unsigned int prefix;
	char *android_log;
	u64 t;

	// Collect Boot_EBS from java side
	if (bootcompleted && !ebs_finished) {
		t = local_clock();
		do_div(t, 1000000ULL);
		if (t - boot_complete_time >= DELAY_TIME_EBS)
			ebs_finished = true;
	}
	if (!ebs_finished && events_ebs < MAX_EVENTS_EBS) {
		if(!strncmp(c, "!@Boot_EBS: ", 12)) {
			sec_enhanced_boot_stat_record(c + 12);
			return;
		}

		if(!strncmp(c, "!@Boot_EBS_", 11)) {
			sec_enhanced_boot_stat_record(c);
			return;
		}
	}

	if (strncmp(c, BOOT_EVT_PREFIX, strlen(BOOT_EVT_PREFIX)))
		return;

	android_log = (char *)(c + strlen(BOOT_EVT_PREFIX));
	if (!strncmp(android_log, BOOT_EVT_PREFIX_PLATFORM, 2)) {
		prefix = EVT_PLATFORM;
		android_log = (char *)(android_log + 2);
		if (!strncmp(android_log, "bootcomplete", 12)) {
			bootcompleted = true;
			t = local_clock();
			do_div(t, 1000000ULL);
			boot_complete_time = (unsigned int)t;
		}
	} else if (!strncmp(android_log, BOOT_EVT_PREFIX_RIL, 7)) {
		prefix = EVT_RIL;
		android_log = (char *)(android_log + 7);
	} else if (!strncmp(android_log, BOOT_EVT_PREFIX_DEBUG, 8)) {
		prefix = EVT_DEBUG;
		android_log = (char *)(android_log + 8);
	} else if (!strncmp(android_log, BOOT_EVT_PREFIX_SYSTEMSERVER, 15)) {
		prefix = EVT_SYSTEMSERVER;
		android_log = (char *)(android_log + 15);
		if (bootcompleted == false)
			sec_boot_stat_record_systemserver(android_log);
		return;
	} else
		return;

	for (i = 0; boot_events[i].string; i++) {
		if (!strcmp(android_log, boot_events[i].string)) {
			if (!boot_events[i].time)
				sec_boot_stat_record(i, get_boot_stat_time());
			break;
		}
	}
}

static int sec_enhanced_boot_stat_proc_show(struct seq_file *m, void *v)
{
	size_t i;
	unsigned long delta;
	unsigned long freq = (unsigned long)get_boot_stat_freq();
	unsigned long time, ktime, prev_time;
	struct enhanced_boot_time *enhanced_boot_time_entry;

	if (!freq)
		freq = DEFAULT_BOOT_STAT_FREQ;

	seq_printf(m, "%-90s %6s %6s %6s\n", "boot event", "time",
				"ktime", "delta");

	seq_puts(m, "--------------------------------------------------------");
	seq_puts(m, "-------------------------------------------------------\n");
	seq_puts(m, "BOOTLOADER - KERNEL\n");
	seq_puts(m, "--------------------------------------------------------");
	seq_puts(m, "-------------------------------------------------------\n");

	for (i = 0, prev_time = 0; boot_initcall[i].string; i++) {
		time = (unsigned long)boot_initcall[i].time * 1000 / freq;
		ktime = (unsigned long)boot_initcall[i].ktime;
		delta = ktime - prev_time;

		seq_printf(m, "%-90s %6lu %6lu %6lu\n",
				boot_initcall[i].string, time, ktime, delta);

		prev_time = ktime;
	}

	seq_puts(m, "--------------------------------------------------------");
	seq_puts(m, "-------------------------------------------------------\n");
	seq_puts(m, "FRAMEWORK\n");
	seq_puts(m, "--------------------------------------------------------");
	seq_puts(m, "-------------------------------------------------------\n");

	list_for_each_entry_reverse (enhanced_boot_time_entry, &enhanced_boot_time_list, next) {
		time = (unsigned long)enhanced_boot_time_entry->time * 1000 / freq;
		ktime = (unsigned long)enhanced_boot_time_entry->ktime;
		if (enhanced_boot_time_entry->buf[0] == '!') {
			delta = ktime - prev_time;
			seq_printf(m, "%-90s %6lu %6lu %6lu\n", enhanced_boot_time_entry->buf, time, ktime, delta);
			prev_time = ktime;
		} else {
			seq_printf(m, "%-90s %6lu %6lu\n", enhanced_boot_time_entry->buf, time, ktime);
		}
	}

	return 0;
}

static int sec_enhanced_boot_stat_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_enhanced_boot_stat_proc_show, NULL);
}

static const struct file_operations sec_enhanced_boot_stat_proc_fops = {
	.open    = sec_enhanced_boot_stat_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int sec_suspend_resume_proc_show(struct seq_file *m, void *v)
{
	unsigned long freq = (unsigned long)get_boot_stat_freq();
	size_t i;

	if (!freq)
		freq = DEFAULT_BOOT_STAT_FREQ;

	seq_printf(m, "%-53s : %6s    %s\n",
			"Suspend Resume Progress(Count)", "time", "ktime");
	seq_puts(m, "------------------------------------------");
	seq_puts(m, "-----------------------------------------\n");

	for (i = 0; suspend_resume_event[i].string != NULL; i++) {
		seq_printf(m, "%-45s(%6d) : %6d    %6d\n",
			   suspend_resume_event[i].string,
			   suspend_resume_event[i].cnt,
			   suspend_resume_event[i].time * 1000 / (int)freq,
			   suspend_resume_event[i].ktime);
	}

	return 0;
}

static int sec_suspend_resume_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_suspend_resume_proc_show, NULL);
}

static const struct file_operations sec_suspend_resume_proc_fops = {
	.open    = sec_suspend_resume_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

void sec_bootstat_suspend_resume_add(const char *c)
{
	size_t i;
	u64 t;

	for (i = 0; suspend_resume_event[i].string != NULL; i++) {
		if (!strcmp(c, suspend_resume_event[i].string)) {
			if (!strcmp(c, "Suspending console"))
				suspend_resume_cnt =
					(suspend_resume_cnt + 1) % 1000;

			suspend_resume_event[i].cnt = suspend_resume_cnt;
			suspend_resume_event[i].time = get_boot_stat_time();
			t = local_clock();
			do_div(t, 1000000ULL);
			suspend_resume_event[i].ktime = (unsigned int)t;
			break;
		}
	}
}

static ssize_t store_boot_stat(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (!ebs_finished && events_ebs < MAX_EVENTS_EBS) {
		if (!strncmp(buf, "!@Boot_EBS_", 11)) {
			sec_enhanced_boot_stat_record(buf);
			return count;
		}
		if (!strncmp(buf, "!@Boot_EBS: ", 12)) {
			sec_enhanced_boot_stat_record(buf + 12);
			return count;
		}
	}

	if (!strncmp(buf, "!@Boot: start init process",
		     strlen("!@Boot: start init process")))
		sec_boot_stat_record(SYSTEM_START_INIT_PROCESS,
			get_boot_stat_time());

	return count;
}
static DEVICE_ATTR(boot_stat, 0220, NULL, store_boot_stat);

static ssize_t store_suspend_resume(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	static uint64_t t;

	t++;

	return count;
}
static DEVICE_ATTR(suspend_resume, 0220, NULL, store_suspend_resume);

static struct attribute *sec_bsp_attributes[] = {
	&dev_attr_boot_stat.attr,
	&dev_attr_suspend_resume.attr,
	NULL,
};

static const struct attribute_group sec_bsp_attribute_group = {
	.attrs = sec_bsp_attributes,
};

static int __init sec_bsp_init(void)
{
	int err;
	struct proc_dir_entry *entry;
	struct proc_dir_entry *enhanced_entry;

	entry = proc_create("boot_stat", 0444, NULL,
				&sec_boot_stat_proc_fops);
	if (!entry)
		return -ENOMEM;

	sec_boot_stat_record(SYSTEM_START_UEFI, bs_uefi_start);
	sec_boot_stat_record(SYSTEM_START_LINUXLOADER, bs_linuxloader_start);
	sec_boot_stat_record(SYSTEM_START_LINUX, bs_linux_start);

	enhanced_entry = proc_create("enhanced_boot_stat", 0444, NULL,
				&sec_enhanced_boot_stat_proc_fops);
	if (!enhanced_entry)
		return -ENOMEM;

	sec_bsp_dev = ___sec_device_create(NULL, "bsp");
	if (unlikely(IS_ERR(sec_bsp_dev))) {
		pr_err("Failed to create devce\n");
		err = PTR_ERR(sec_bsp_dev);
		goto err_dev_create;
	}

	err = sysfs_create_group(&sec_bsp_dev->kobj, &sec_bsp_attribute_group);
	if (unlikely(err)) {
		pr_err("Failed to create device files!\n");
		goto err_dev_create_file;
	}

	/* Power State Logging */
	entry = proc_create("suspend_resume", 0444, NULL,
			&sec_suspend_resume_proc_fops);
	if (unlikely(!entry)) {
		err =  -ENOMEM;
		goto err_dev_crate_proc;
	}

	return 0;

err_dev_crate_proc:
	sysfs_remove_group(&sec_bsp_dev->kobj, &sec_bsp_attribute_group);
err_dev_create_file:
	sec_device_destroy(sec_bsp_dev->devt);
err_dev_create:
	proc_remove(entry);
	return err;
}
module_init(sec_bsp_init);
