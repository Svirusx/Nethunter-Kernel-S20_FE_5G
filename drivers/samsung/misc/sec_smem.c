// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/misc/samsung/sec_smem.c
 *
 * COPYRIGHT(C) 2015-2019 Samsung Electronics Co., Ltd. All Right Reserved.
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

#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/version.h>

#if defined(CONFIG_MSM_SMEM)
#include <soc/qcom/smem.h>
#else
#include <linux/soc/qcom/smem.h>
#endif

#include <linux/sec_smem.h>
#include <linux/sec_debug.h>

#include <asm/arch_timer.h>

#include "../sec_kcompat.h"

#include <linux/topology.h>

#define SUSPEND	0x1
#define RESUME	0x0

static ap_health_t *p_health;

static char *lpddr4_manufacture_name[] = {
	"NA",
	"SEC"/* Samsung */,
	"NA",
	"NA",
	"NA",
	"NAN" /* Nanya */,
	"HYN" /* SK hynix */,
	"NA",
	"WIN" /* Winbond */,
	"ESM" /* ESMT */,
	"NA",
	"NA",
	"NA",
	"NA",
	"NA",
	"MIC" /* Micron */,
};

static void *__get_ddr_smem_entry(unsigned int id)
{
	void *entry;
	size_t size = 0;

	entry = qcom_smem_get(QCOM_SMEM_HOST_ANY, id, &size);
	if (!size)
		pr_err("entry size is zero\n");

	return entry;
}

uint8_t get_ddr_info(uint8_t type) {
	sec_smem_id_vendor0_v2_t *vendor0 = __get_ddr_smem_entry(SMEM_ID_VENDOR0);

	if (IS_ERR_OR_NULL(vendor0)) {
		pr_err("SMEM_ID_VENDOR0 get entry error\n");
		return 0;
	}

	return (vendor0->ddr_vendor >> type) & 0xFF;
}

char *get_ddr_vendor_name(void)
{
	sec_smem_id_vendor0_v2_t *vendor0;
	size_t lpddr4_manufacture;

	vendor0 = __get_ddr_smem_entry(SMEM_ID_VENDOR0);
	if (IS_ERR_OR_NULL(vendor0)) {
		pr_err("SMEM_ID_VENDOR0 get entry error\n");
		return 0;
	}

	lpddr4_manufacture =
		vendor0->ddr_vendor % ARRAY_SIZE(lpddr4_manufacture_name);

	return lpddr4_manufacture_name[lpddr4_manufacture];
}

uint32_t get_ddr_DSF_version(void)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.version;
}

uint8_t get_ddr_revision_id_1(void)
{
	return get_ddr_info(DDR_IFNO_REVISION_ID1);
}

uint8_t get_ddr_revision_id_2(void)
{
	return get_ddr_info(DDR_IFNO_REVISION_ID2);
}

uint8_t get_ddr_total_density(void)
{
	return get_ddr_info(DDR_IFNO_TOTAL_DENSITY);
}

uint8_t get_ddr_rcw_tDQSCK(uint32_t ch, uint32_t cs, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.rcw.tDQSCK[ch][cs][dq];
}

uint8_t get_ddr_wr_coarseCDC(uint32_t ch, uint32_t cs, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.wr_dqdqs.coarse_cdc[ch][cs][dq];
}

uint8_t get_ddr_wr_fineCDC(uint32_t ch, uint32_t cs, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.wr_dqdqs.fine_cdc[ch][cs][dq];
}

// SED
uint8_t ddr_get_wr_pr_width(uint32_t ch, uint32_t cs, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.wr_pr_width[ch][cs][dq];
}

uint8_t ddr_get_wr_min_eye_height(uint32_t ch, uint32_t cs)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.wr_min_eye_height[ch][cs];
}

uint8_t ddr_get_wr_best_vref(uint32_t ch, uint32_t cs)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.wr_best_vref[ch][cs];
}

uint8_t ddr_get_wr_vmax_to_vmid(uint32_t ch, uint32_t cs, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.wr_vmax_to_vmid[ch][cs][dq];
}

uint8_t ddr_get_wr_vmid_to_vmin(uint32_t ch, uint32_t cs, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.wr_vmid_to_vmin[ch][cs][dq];
}

uint8_t ddr_get_dqs_dcc_adj(uint32_t ch, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.dqs_dcc_adj[ch][dq];
}

uint8_t ddr_get_rd_pr_width(uint32_t ch, uint32_t cs, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.rd_pr_width[ch][cs][dq];
}

uint8_t ddr_get_rd_min_eye_height(uint32_t ch, uint32_t cs)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.rd_min_eye_height[ch][cs];
}

uint8_t ddr_get_rd_best_vref(uint32_t ch, uint32_t cs, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (!vendor1) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.rd_best_vref[ch][cs][dq];
}

uint8_t ddr_get_dq_dcc_abs(uint32_t ch, uint32_t cs, uint32_t dq)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (!vendor1) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.dq_dcc_abs[ch][cs][dq];
}

uint16_t ddr_get_small_eye_detected(void)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (!vendor1) {
		pr_err("SMEM_ID_VENDOR1 get entry error\n");
		return 0;
	}

	return vendor1->ddr_training.dqdqs_eye.small_eye_detected;
}

#ifdef CONFIG_SEC_DEBUG_APPS_CLK_LOGGING
typedef struct {
	uint64_t ktime;
	uint64_t qtime;
	uint64_t rate;
} apps_clk_log_t;

#define MAX_CLK_LOG_CNT (10)

typedef struct {
	uint32_t max_cnt;
	uint32_t index;
	apps_clk_log_t log[MAX_CLK_LOG_CNT];
} cpuclk_log_t;

static cpuclk_log_t cpuclk_log[MAX_CLUSTER_NUM] = {
	[0] = {.max_cnt = MAX_CLK_LOG_CNT,},
	[1] = {.max_cnt = MAX_CLK_LOG_CNT,},
	[2] = {.max_cnt = MAX_CLK_LOG_CNT,},
	[3] = {.max_cnt = MAX_CLK_LOG_CNT,},
};

static void __always_inline __sec_smem_cpuclk_log_raw(size_t slot, unsigned long rate)
{
	cpuclk_log_t *clk = &cpuclk_log[slot];
	uint64_t idx = clk->index;
	apps_clk_log_t *log = &clk->log[idx];

	log->ktime = local_clock();
	log->qtime = arch_timer_read_counter();
	log->rate = rate;
	clk->index = (clk->index + 1) % MAX_CLK_LOG_CNT;
}

void sec_smem_cpuclk_log_raw(size_t slot, unsigned long rate)
{
	__sec_smem_cpuclk_log_raw(slot, rate);
}

void sec_smem_clk_osm_add_log_cpufreq(unsigned int cpu,
		unsigned long rate, const char *name)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0)
	unsigned int domain = cpu_topology[cpu].cluster_id;
#else
	unsigned int domain = cpu_topology[cpu].package_id;
#endif
	if (!WARN((domain + 1) < PWR_CLUSTER || (domain + 1) > PRIME_CLUSTER,
			"%s : invalid cluster_num(%u), dbg_name(%s)\n",
			__func__, domain + 1, name)) {
		__sec_smem_cpuclk_log_raw(domain + 1, rate);
	}
}

void sec_smem_clk_osm_add_log_l3(unsigned long rate)
{
	__sec_smem_cpuclk_log_raw(L3, rate);
}
#endif /* CONFIG_SEC_DEBUG_APPS_CLK_LOGGING */

// ssuk end
static int sec_smem_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	SEC_SMEM_ID_VEN1_TYPE *vendor1 = platform_get_drvdata(pdev);

#if (CONFIG_SEC_SMEM_VENDOR1_VERSION >= 7)
	vendor1->share.ap_suspended = SUSPEND;
#else
	vendor1->ven1_v2.ap_suspended = SUSPEND;
#endif

	pr_debug("smem_vendor1 ap_suspended - SUSPEND\n");
	return 0;
}

static int sec_smem_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	SEC_SMEM_ID_VEN1_TYPE *vendor1 = platform_get_drvdata(pdev);

#if (CONFIG_SEC_SMEM_VENDOR1_VERSION >= 7)
	vendor1->share.ap_suspended = RESUME;
#else
	vendor1->ven1_v2.ap_suspended = RESUME;
#endif

	pr_debug("smem_vendor1 ap_suspended - RESUME\n");
	return 0;
}

static int sec_smem_probe(struct platform_device *pdev)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
	if (PTR_ERR(vendor1) == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (IS_ERR_OR_NULL(vendor1)) {
		pr_err("SMEM_ID_VENDOR1 get entry error(%ld)\n", PTR_ERR(vendor1));
		panic("sec_smem_probe fail");
		return -EINVAL;
	}

#ifdef CONFIG_SEC_DEBUG_APPS_CLK_LOGGING
	vendor1->apps_stat.clk = (void *)virt_to_phys(&cpuclk_log);
	pr_err("vendor1->apps_stat.clk = %p\n", vendor1->apps_stat.clk);
#endif

	platform_set_drvdata(pdev, vendor1);
	pr_info("loading success!!\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id sec_smem_dt_ids[] = {
	{ .compatible = "samsung,sec-smem" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_smem_dt_ids);
#endif /* CONFIG_OF */

static const struct dev_pm_ops sec_smem_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sec_smem_suspend, sec_smem_resume)
};

struct platform_driver sec_smem_driver = {
	.probe		= sec_smem_probe,
	.driver		= {
		.name = "sec_smem",
		.owner	= THIS_MODULE,
		.pm = &sec_smem_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = sec_smem_dt_ids,
#endif
	},
};

static int sec_smem_dbg_part_notifier_callback(struct notifier_block *nfb,
		unsigned long action, void *data)
{
	SEC_SMEM_ID_VEN1_TYPE *vendor1;

	switch (action) {
	case DBG_PART_DRV_INIT_DONE:
		p_health = ap_health_data_read();
		if (!p_health)
			return NOTIFY_DONE;

		vendor1 = __get_ddr_smem_entry(SMEM_ID_VENDOR1);
		if (IS_ERR_OR_NULL(vendor1))
			return NOTIFY_DONE;

		vendor1->ap_health = (void *)virt_to_phys(p_health);
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block sec_smem_dbg_part_notifier = {
	.notifier_call = sec_smem_dbg_part_notifier_callback,
};

static int __init sec_smem_init(void)
{
	int err;

	err = platform_driver_register(&sec_smem_driver);
	if (err)
		pr_err("Failed to register platform driver: %d\n", err);

	dbg_partition_notifier_register(&sec_smem_dbg_part_notifier);

	return 0;
}
device_initcall(sec_smem_init);

static void __exit sec_smem_exit(void)
{
	platform_driver_unregister(&sec_smem_driver);
}
module_exit(sec_smem_exit);

MODULE_DESCRIPTION("SEC SMEM");
MODULE_LICENSE("GPL v2");
