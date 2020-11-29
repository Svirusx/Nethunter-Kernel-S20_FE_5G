// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/uaccess.h>
#include <linux/soc/qcom/smem.h>
#include <asm/arch_timer.h>
#include "rpmh_master_stat.h"
#ifdef CONFIG_DSP_SLEEP_RECOVERY
#include <linux/adsp/adsp-loader.h>
#include <linux/adsp/slpi-loader.h>
#include <linux/cdsp-loader.h>
#include <linux/rtc.h>
#endif /* CONFIG_DSP_SLEEP_RECOVERY */

#define UNIT_DIST 0x14
#define REG_VALID 0x0
#define REG_DATA_LO 0x4
#define REG_DATA_HI 0x8

#define GET_ADDR(REG, UNIT_NO) (REG + (UNIT_DIST * UNIT_NO))

#ifdef CONFIG_SEC_PM
#define MSM_ARCH_TIMER_FREQ	19200000
#define GET_SEC(A)		((A) / (MSM_ARCH_TIMER_FREQ))
#define GET_MSEC(A)		(((A) / (MSM_ARCH_TIMER_FREQ / 1000)) % 1000)
#endif

enum master_smem_id {
	MPSS = 605,
	ADSP,
	CDSP,
	SLPI,
	GPU,
	DISPLAY,
	SLPI_ISLAND = 613,
};

enum master_pid {
	PID_APSS = 0,
	PID_MPSS = 1,
	PID_ADSP = 2,
	PID_SLPI = 3,
	PID_CDSP = 5,
	PID_GPU = PID_APSS,
	PID_DISPLAY = PID_APSS,
};

enum profile_data {
	POWER_DOWN_START,
	POWER_UP_END,
	POWER_DOWN_END,
	POWER_UP_START,
	NUM_UNIT,
};

struct msm_rpmh_master_data {
	char *master_name;
	enum master_smem_id smem_id;
	enum master_pid pid;
};

static const struct msm_rpmh_master_data rpmh_masters[] = {
	{"MPSS", MPSS, PID_MPSS},
	{"ADSP", ADSP, PID_ADSP},
	{"ADSP_ISLAND", SLPI_ISLAND, PID_ADSP},
	{"CDSP", CDSP, PID_CDSP},
	{"SLPI", SLPI, PID_SLPI},
	{"SLPI_ISLAND", SLPI_ISLAND, PID_SLPI},
	{"GPU", GPU, PID_GPU},
	{"DISPLAY", DISPLAY, PID_DISPLAY},
};

struct msm_rpmh_master_stats {
	uint32_t version_id;
	uint32_t counts;
	uint64_t last_entered;
	uint64_t last_exited;
	uint64_t accumulated_duration;
};

struct msm_rpmh_profile_unit {
	uint64_t value;
	uint64_t valid;
};

struct rpmh_master_stats_prv_data {
	struct kobj_attribute ka;
	struct kobject *kobj;
};

static struct msm_rpmh_master_stats apss_master_stats;
static void __iomem *rpmh_unit_base;

static DEFINE_MUTEX(rpmh_stats_mutex);
#ifdef CONFIG_DSP_SLEEP_RECOVERY
#ifdef CONFIG_DSP_SLEEP_DEBUG
#define MAX_COUNT 5
/* It is UTC time KST+24-9 */
#define START_H 0
#define END_H 24
#else
#define MAX_COUNT 60
/* It is UTC time KST+24-9 */
#define START_H 17
#define END_H 20
#endif

extern unsigned int tx_mck;
extern unsigned int tx_mck_div;
extern bool voice_activated;

struct _dsp_entry {
	char name[4];
	uint64_t entry_sec;
	uint64_t entry_msec;
	uint64_t prev_sec;
	uint64_t prev_msec;
	uint64_t error_count;
	uint64_t ssr_count;
	struct rtc_time tm_chk;
	int (*ssr)(void);
} DSP_ENTRY[3]; // 0 : ADSP, 1 : CDSP, 2 : SLPI
#endif

#ifdef CONFIG_SEC_PM
void debug_masterstats_show(char *annotation)
{
	int i = 0;
	size_t size = 0;
	struct msm_rpmh_master_stats *record = NULL;
	uint64_t accumulated_duration;
	unsigned int duration_sec, duration_msec;
	char buf[256];
	char *buf_ptr = buf;
#ifdef CONFIG_DSP_SLEEP_RECOVERY
	struct timespec ts;
	struct _dsp_entry *dsp_entry;
#endif

	mutex_lock(&rpmh_stats_mutex);

	buf_ptr += sprintf(buf_ptr, "PM: %s: ", annotation);
	/* Read SMEM data written by other masters */
	for (i = 0; i < ARRAY_SIZE(rpmh_masters); i++) {
		record = (struct msm_rpmh_master_stats *) qcom_smem_get(
					rpmh_masters[i].pid,
					rpmh_masters[i].smem_id, &size);

		if (!IS_ERR_OR_NULL(record)) {
			accumulated_duration = record->accumulated_duration;
			if (record->last_entered > record->last_exited)
				accumulated_duration +=
					(arch_counter_get_cntvct() -
						record->last_entered);

			if (accumulated_duration == record->accumulated_duration)
				buf_ptr += sprintf(buf_ptr, "*");

			duration_sec = GET_SEC(accumulated_duration);
			duration_msec = GET_MSEC(accumulated_duration);
#ifdef CONFIG_DSP_SLEEP_RECOVERY
			dsp_entry = (!strcmp(rpmh_masters[i].master_name, "ADSP")) ? &DSP_ENTRY[0] :
				(!strcmp(rpmh_masters[i].master_name, "CDSP")) ? &DSP_ENTRY[1] :
				(!strcmp(rpmh_masters[i].master_name, "SLPI")) ? &DSP_ENTRY[2] : NULL;

			if (dsp_entry != NULL) {
				if(!strcmp(annotation, "entry")) {
					dsp_entry->entry_sec = duration_sec;
					dsp_entry->entry_msec = duration_msec;
				} else if(!strcmp(annotation, "exit")) {
					getnstimeofday(&ts);
					rtc_time_to_tm(ts.tv_sec, &dsp_entry->tm_chk);

					if ((!voice_activated && !strncmp(dsp_entry->name, "adsp", 4)) ||
						!strncmp(dsp_entry->name, "cdsp", 4) ||
						!strncmp(dsp_entry->name, "slpi", 4)) {
						/* Error detected if exit duration is same as entry */
						if ((duration_sec == dsp_entry->entry_sec &&
											duration_msec == dsp_entry->entry_msec)) {
							/* increase count if entry duration is same as prev exit */
							if (dsp_entry->prev_sec == dsp_entry->entry_sec &&
											dsp_entry->prev_msec == dsp_entry->entry_msec)
								dsp_entry->error_count++;
							/* set count as 1 when aDSP entered sleep just before */
							else
								dsp_entry->error_count = 1;

							pr_info("%s non-sleep count %d, tx_mck %d, tx_mck_div %d, ssr count %d\n",
								rpmh_masters[i].master_name, dsp_entry->error_count, tx_mck, tx_mck_div,
								dsp_entry->ssr_count);
						} else {
							dsp_entry->error_count = 0;
						}
					}
					dsp_entry->prev_sec = duration_sec;
					dsp_entry->prev_msec = duration_msec;
				}
			}
#endif
			buf_ptr += sprintf(buf_ptr, "%s(%d, %u.%u), ",
					rpmh_masters[i].master_name,
					record->counts,
					duration_sec, duration_msec);
		} else {
			continue;
		}
	}

	buf_ptr--;
	buf_ptr--;
	buf_ptr += sprintf(buf_ptr, "\n");
	mutex_unlock(&rpmh_stats_mutex);

	printk(KERN_INFO "%s", buf);

#ifdef CONFIG_DSP_SLEEP_RECOVERY
	// 0 : ADSP, 1 : CDSP, 2 : SLPI
	for (i = 0; i < sizeof(DSP_ENTRY) / sizeof(struct _dsp_entry); i++) {
		dsp_entry = &DSP_ENTRY[i];

		if(dsp_entry->error_count > MAX_COUNT) {
#ifdef CONFIG_DSP_SLEEP_RECOVERY_FOR_ADSP
			if(dsp_entry->tm_chk.tm_hour >= START_H &&
					dsp_entry->tm_chk.tm_hour < END_H &&
					!(strncmp(dsp_entry->name, "adsp", 4)))
				dsp_entry->ssr = &adsp_ssr;
#endif

			if (!strncmp(dsp_entry->name, "cdsp", 4))
				dsp_entry->ssr = &cdsp_ssr;

			if (!strncmp(dsp_entry->name, "slpi", 4))
				dsp_entry->ssr = &slpi_ssr;

			if (dsp_entry->ssr)
				dsp_entry->ssr();
			dsp_entry->ssr_count++;
			dsp_entry->error_count = 0;
		}
	}
#endif
}
EXPORT_SYMBOL(debug_masterstats_show);
#endif

static ssize_t msm_rpmh_master_stats_print_data(char *prvbuf, ssize_t length,
				struct msm_rpmh_master_stats *record,
				const char *name)
{
	uint64_t accumulated_duration = record->accumulated_duration;
	/*
	 * If a master is in sleep when reading the sleep stats from SMEM
	 * adjust the accumulated sleep duration to show actual sleep time.
	 * This ensures that the displayed stats are real when used for
	 * the purpose of computing battery utilization.
	 */
	if (record->last_entered > record->last_exited)
		accumulated_duration +=
				(arch_counter_get_cntvct()
				- record->last_entered);

	return scnprintf(prvbuf, length, "%s\n\tVersion:0x%x\n"
			"\tSleep Count:0x%x\n"
			"\tSleep Last Entered At:0x%llx\n"
			"\tSleep Last Exited At:0x%llx\n"
			"\tSleep Accumulated Duration:0x%llx\n\n",
			name, record->version_id, record->counts,
			record->last_entered, record->last_exited,
			accumulated_duration);
}

static ssize_t msm_rpmh_master_stats_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	ssize_t length;
	int i = 0;
	size_t size = 0;
	struct msm_rpmh_master_stats *record = NULL;

	mutex_lock(&rpmh_stats_mutex);

	/* First Read APSS master stats */

	length = msm_rpmh_master_stats_print_data(buf, PAGE_SIZE,
						&apss_master_stats, "APSS");

	/* Read SMEM data written by other masters */

	for (i = 0; i < ARRAY_SIZE(rpmh_masters); i++) {
		record = (struct msm_rpmh_master_stats *) qcom_smem_get(
					rpmh_masters[i].pid,
					rpmh_masters[i].smem_id, &size);
		if (!IS_ERR_OR_NULL(record) && (PAGE_SIZE - length > 0))
			length += msm_rpmh_master_stats_print_data(
					buf + length, PAGE_SIZE - length,
					record,
					rpmh_masters[i].master_name);
	}

	mutex_unlock(&rpmh_stats_mutex);

	return length;
}

static inline void msm_rpmh_apss_master_stats_update(
				struct msm_rpmh_profile_unit *profile_unit)
{
	apss_master_stats.counts++;
	apss_master_stats.last_entered = profile_unit[POWER_DOWN_END].value;
	apss_master_stats.last_exited = profile_unit[POWER_UP_START].value;
	apss_master_stats.accumulated_duration +=
					(apss_master_stats.last_exited
					- apss_master_stats.last_entered);
}

void msm_rpmh_master_stats_update(void)
{
	int i;
	struct msm_rpmh_profile_unit profile_unit[NUM_UNIT];

	if (!rpmh_unit_base)
		return;

	for (i = POWER_DOWN_END; i < NUM_UNIT; i++) {
		profile_unit[i].valid = readl_relaxed(rpmh_unit_base +
						GET_ADDR(REG_VALID, i));

		/*
		 * Do not update APSS stats if valid bit is not set.
		 * It means APSS did not execute cx-off sequence.
		 * This can be due to fall through at some point.
		 */

		if (!(profile_unit[i].valid & BIT(REG_VALID)))
			return;

		profile_unit[i].value = readl_relaxed(rpmh_unit_base +
						GET_ADDR(REG_DATA_LO, i));
		profile_unit[i].value |= ((uint64_t)
					readl_relaxed(rpmh_unit_base +
					GET_ADDR(REG_DATA_HI, i)) << 32);
	}
	msm_rpmh_apss_master_stats_update(profile_unit);
}
EXPORT_SYMBOL(msm_rpmh_master_stats_update);

static int msm_rpmh_master_stats_probe(struct platform_device *pdev)
{
	struct rpmh_master_stats_prv_data *prvdata = NULL;
	struct kobject *rpmh_master_stats_kobj = NULL;
	int ret = -ENOMEM;

	prvdata = devm_kzalloc(&pdev->dev, sizeof(*prvdata), GFP_KERNEL);
	if (!prvdata)
		return ret;

	rpmh_master_stats_kobj = kobject_create_and_add(
					"rpmh_stats",
					power_kobj);
	if (!rpmh_master_stats_kobj)
		return ret;

	prvdata->kobj = rpmh_master_stats_kobj;

	sysfs_attr_init(&prvdata->ka.attr);
	prvdata->ka.attr.mode = 0444;
	prvdata->ka.attr.name = "master_stats";
	prvdata->ka.show = msm_rpmh_master_stats_show;
	prvdata->ka.store = NULL;

	ret = sysfs_create_file(prvdata->kobj, &prvdata->ka.attr);
	if (ret) {
		pr_err("sysfs_create_file failed\n");
		goto fail_sysfs;
	}

	rpmh_unit_base = of_iomap(pdev->dev.of_node, 0);
	if (!rpmh_unit_base) {
		pr_err("Failed to get rpmh_unit_base\n");
		ret = -ENOMEM;
		goto fail_iomap;
	}

	apss_master_stats.version_id = 0x1;
	platform_set_drvdata(pdev, prvdata);

#ifdef CONFIG_DSP_SLEEP_RECOVERY
	strncpy(DSP_ENTRY[0].name, "adsp", 4);
	strncpy(DSP_ENTRY[1].name, "cdsp", 4);
	strncpy(DSP_ENTRY[2].name, "slpi", 4);
#endif
	return ret;

fail_iomap:
	sysfs_remove_file(prvdata->kobj, &prvdata->ka.attr);
fail_sysfs:
	kobject_put(prvdata->kobj);
	return ret;
}

static int msm_rpmh_master_stats_remove(struct platform_device *pdev)
{
	struct rpmh_master_stats_prv_data *prvdata;

	prvdata = (struct rpmh_master_stats_prv_data *)
				platform_get_drvdata(pdev);

	sysfs_remove_file(prvdata->kobj, &prvdata->ka.attr);
	kobject_put(prvdata->kobj);
	platform_set_drvdata(pdev, NULL);
	iounmap(rpmh_unit_base);
	rpmh_unit_base = NULL;

	return 0;
}

static const struct of_device_id rpmh_master_table[] = {
	{.compatible = "qcom,rpmh-master-stats-v1"},
	{},
};

static struct platform_driver msm_rpmh_master_stats_driver = {
	.probe	= msm_rpmh_master_stats_probe,
	.remove = msm_rpmh_master_stats_remove,
	.driver = {
		.name = "msm_rpmh_master_stats",
		.of_match_table = rpmh_master_table,
	},
};

module_platform_driver(msm_rpmh_master_stats_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MSM RPMH Master Statistics driver");
MODULE_ALIAS("platform:msm_rpmh_master_stat_log");
