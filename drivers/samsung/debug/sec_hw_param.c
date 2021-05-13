// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_hw_param.c
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

#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/of.h>

#include <soc/qcom/socinfo.h>

#include <linux/sec_smem.h>
#include <linux/sec_class.h>
#include <linux/sec_debug.h>

#ifdef CONFIG_SEC_PM
#include <linux/input/qpnp-power-on.h>
#endif

enum {
	CONVERSION_SOC_REV_1_0 = 0,
	CONVERSION_SOC_REV_2_0,
	CONVERSION_SOC_REV_2_1,
	CONVERSION_SOC_REV_2_2,
	CONVERSION_SOC_REV_1_1,
	CONVERSION_SOC_REV_UNKNOWN,
};      

#define DEFAULT_LEN_STR         1023
#define SPECIAL_LEN_STR         2047
#define EXTRA_LEN_STR           ((SPECIAL_LEN_STR) - (31 + 5))

#define default_scnprintf(buf, offset, fmt, ...)			\
do {									\
	offset += scnprintf(&(buf)[offset], DEFAULT_LEN_STR - (size_t)offset, \
			fmt, ##__VA_ARGS__);				\
} while (0)

#define special_scnprintf(buf, offset, fmt, ...)			\
do {									\
	offset += scnprintf(&(buf)[offset], SPECIAL_LEN_STR - (size_t)offset, \
			fmt, ##__VA_ARGS__);				\
} while (0)

#define extra_scnprintf(buf, offset, fmt, ...)				\
do {									\
	offset += scnprintf(&(buf)[offset], EXTRA_LEN_STR - (size_t)offset, \
			fmt, ##__VA_ARGS__);				\
} while (0)

static unsigned int system_rev __read_mostly;

static int __init sec_hw_rev_setup(char *p)
{
	int ret;

	ret = kstrtouint(p, 0, &system_rev);
	if (unlikely(ret < 0)) {
		pr_warn("androidboot.revision is malformed (%s)\n", p);
		return -EINVAL;
	}

	pr_info("androidboot.revision %x\n", system_rev);

	return 0;
}
early_param("androidboot.revision", sec_hw_rev_setup);

static void check_format(char *buf, ssize_t *size, int max_len_str)
{
	int i = 0, cnt = 0, pos = 0;

	if (!buf || *size <= 0)
		return;

	if (*size >= max_len_str)
		*size = max_len_str - 1;

	while (i < *size && buf[i]) {
		if (buf[i] == '"') {
			cnt++;
			pos = i;
		}

		if ((buf[i] < 0x20) || (buf[i] == 0x5C) || (buf[i] > 0x7E))
			buf[i] = ' ';
		i++;
	}

	if (cnt % 2) {
		if (pos == *size - 1) {
			buf[*size - 1] = '\0';
		} else {
			buf[*size - 1] = '"';
			buf[*size] = '\0';
		}
	}
}

static bool __is_ready_debug_reset_header(void)
{
	struct debug_reset_header *header = get_debug_reset_header();

	if (!header)
		return false;

	kfree(header);
	return true;
}

static bool __is_valid_reset_reason(unsigned int reset_reason)
{
	if (reset_reason < USER_UPLOAD_CAUSE_MIN ||
	    reset_reason > USER_UPLOAD_CAUSE_MAX)
		return false;

	if (reset_reason == USER_UPLOAD_CAUSE_MANUAL_RESET ||
	    reset_reason == USER_UPLOAD_CAUSE_REBOOT ||
	    reset_reason == USER_UPLOAD_CAUSE_BOOTLOADER_REBOOT ||
	    reset_reason == USER_UPLOAD_CAUSE_POWER_ON)
		return false;

	return true;
}

static ap_health_t *phealth;

static bool batt_info_cleaned;
static void clean_batt_info(void)
{
	if (phealth && !batt_info_cleaned) {
		memset(&phealth->battery, 0x0, sizeof(battery_health_t));
		batt_info_cleaned = true;
	}
}

void battery_last_dcvs(int cap, int volt, int temp, int curr)
{
	uint32_t tail = 0;

	if (phealth == NULL || !batt_info_cleaned)
		return;

	if ((phealth->battery.tail & 0xf) >= MAX_BATT_DCVS)
		phealth->battery.tail = 0x10;

	tail = phealth->battery.tail & 0xf;

	phealth->battery.batt[tail].ktime = local_clock();
	phealth->battery.batt[tail].cap = cap;
	phealth->battery.batt[tail].volt = volt;
	phealth->battery.batt[tail].temp = temp;
	phealth->battery.batt[tail].curr = curr;

	phealth->battery.tail++;
}

static ssize_t show_last_dcvs(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	unsigned int reset_reason;
	char *prefix[MAX_CLUSTER_NUM] = { "L3", "SC", "GC", "GP"};
	size_t i;

	if (!phealth)
		phealth = ap_health_data_read();

	if (!phealth) {
		pr_err("fail to get ap health info\n");
		return info_size;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		return info_size;

	default_scnprintf(buf, info_size, "\"RR\":\"%s\",",
			sec_debug_get_reset_reason_str(reset_reason));
	default_scnprintf(buf, info_size, "\"RWC\":\"%d\",",
			sec_debug_get_reset_write_cnt());

	for (i = 0; i < MAX_CLUSTER_NUM; i++)
		default_scnprintf(buf, info_size, "\"%sKHz\":\"%u\",",
				prefix[i],
				phealth->last_dcvs.apps[i].cpu_KHz);

	default_scnprintf(buf, info_size, "\"DDRKHz\":\"%u\",",
			phealth->last_dcvs.rpm.ddr_KHz);
	default_scnprintf(buf, info_size, "\"BCAP\":\"%d\",",
			phealth->last_dcvs.batt.cap);
	default_scnprintf(buf, info_size, "\"BVOL\":\"%d\",",
			phealth->last_dcvs.batt.volt);
	default_scnprintf(buf, info_size, "\"BTEM\":\"%d\",",
			phealth->last_dcvs.batt.temp);
	default_scnprintf(buf, info_size, "\"BCUR\":\"%d\",",
			phealth->last_dcvs.batt.curr);

	default_scnprintf(buf, info_size, "\"PON\":\"%llx\",",
			phealth->last_dcvs.pon.pon_reason);
	default_scnprintf(buf, info_size, "\"PONF\":\"%llx\",",
			phealth->last_dcvs.pon.fault_reason);

	/* remove the last ',' character */
	info_size--;

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(last_dcvs, 0440, show_last_dcvs, NULL);

static ssize_t store_ap_health(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char clear;
	int err;

	err = sscanf(buf, "%1c", &clear);
	if ((err <= 0) || (clear != 'c' && clear != 'C')) {
		pr_err("command error\n");
		return count;
	}

	if (!phealth)
		phealth = ap_health_data_read();

	if (!phealth) {
		pr_err("fail to get ap health info\n");
		return -EINVAL;
	}

	pr_info("clear ap_health_data by HQM %zu\n", sizeof(ap_health_t));

	/*++ add here need init data by HQM ++*/
	memset(&(phealth->daily_rr), 0, sizeof(reset_reason_t));
	memset(&(phealth->daily_cache), 0, sizeof(cache_health_t));
	memset(&(phealth->daily_pcie), 0, sizeof(pcie_health_t) * MAX_PCIE_NUM);

	ap_health_data_write(phealth);

	return count;
}

static ssize_t show_ap_health(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int cpu;
	size_t i;

	if (!phealth)
		phealth = ap_health_data_read();

	if (!phealth) {
		pr_err("fail to get ap health info\n");
		return info_size;
	}

	default_scnprintf(buf, info_size, "\"L1c\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		default_scnprintf(buf, info_size, "%d",
				phealth->cache.edac[cpu][0].ce_cnt);

		if (cpu == (num_present_cpus() - 1))
			default_scnprintf(buf, info_size, "\",");
		else
			default_scnprintf(buf, info_size, ",");
	}

	default_scnprintf(buf, info_size, "\"L1u\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		default_scnprintf(buf, info_size, "%d",
				phealth->cache.edac[cpu][0].ue_cnt);

		if (cpu == (num_present_cpus() - 1))
			default_scnprintf(buf, info_size, "\",");
		else
			default_scnprintf(buf, info_size, ",");
	}

	default_scnprintf(buf, info_size, "\"L2c\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		default_scnprintf(buf, info_size, "%d",
				phealth->cache.edac[cpu][1].ce_cnt);

		if (cpu == (num_present_cpus() - 1))
			default_scnprintf(buf, info_size, "\",");
		else
			default_scnprintf(buf, info_size, ",");
	}

	default_scnprintf(buf, info_size, "\"L2u\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		default_scnprintf(buf, info_size, "%d",
				phealth->cache.edac[cpu][1].ue_cnt);

		if (cpu == (num_present_cpus() - 1))
			default_scnprintf(buf, info_size, "\",");
		else
			default_scnprintf(buf, info_size, ",");
	}

	default_scnprintf(buf, info_size, "\"L3c\":\"%d\",",
			phealth->cache.edac_l3.ce_cnt);
	default_scnprintf(buf, info_size, "\"L3u\":\"%d\",",
			phealth->cache.edac_l3.ue_cnt);
	default_scnprintf(buf, info_size, "\"EDB\":\"%d\",",
			phealth->cache.edac_bus_cnt);
	default_scnprintf(buf, info_size, "\"LDc\":\"%d\",",
			phealth->cache.edac_llcc_data_ram.ce_cnt);
	default_scnprintf(buf, info_size, "\"LDu\":\"%d\",",
			phealth->cache.edac_llcc_data_ram.ue_cnt);
	default_scnprintf(buf, info_size, "\"LTc\":\"%d\",",
			phealth->cache.edac_llcc_tag_ram.ce_cnt);
	default_scnprintf(buf, info_size, "\"LTu\":\"%d\",",
			phealth->cache.edac_llcc_tag_ram.ue_cnt);

	default_scnprintf(buf, info_size, "\"dL1c\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		default_scnprintf(buf, info_size, "%d",
				phealth->daily_cache.edac[cpu][0].ce_cnt);

		if (cpu == (num_present_cpus() - 1))
			default_scnprintf(buf, info_size, "\",");
		else
			default_scnprintf(buf, info_size, ",");
	}

	default_scnprintf(buf, info_size, "\"dL1u\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		default_scnprintf(buf, info_size, "%d",
				phealth->daily_cache.edac[cpu][0].ue_cnt);

		if (cpu == (num_present_cpus() - 1))
			default_scnprintf(buf, info_size, "\",");
		else
			default_scnprintf(buf, info_size, ",");
	}

	default_scnprintf(buf, info_size, "\"dL2c\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		default_scnprintf(buf, info_size, "%d",
				phealth->daily_cache.edac[cpu][1].ce_cnt);

		if (cpu == (num_present_cpus() - 1))
			default_scnprintf(buf, info_size, "\",");
		else
			default_scnprintf(buf, info_size, ",");
	}

	default_scnprintf(buf, info_size, "\"dL2u\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		default_scnprintf(buf, info_size, "%d",
				phealth->daily_cache.edac[cpu][1].ue_cnt);

		if (cpu == (num_present_cpus() - 1))
			default_scnprintf(buf, info_size, "\",");
		else
			default_scnprintf(buf, info_size, ",");
	}

	default_scnprintf(buf, info_size, "\"dL3c\":\"%d\",",
			phealth->daily_cache.edac_l3.ce_cnt);
	default_scnprintf(buf, info_size, "\"dL3u\":\"%d\",",
			phealth->daily_cache.edac_l3.ue_cnt);
	default_scnprintf(buf, info_size, "\"dEDB\":\"%d\",",
			phealth->daily_cache.edac_bus_cnt);
	default_scnprintf(buf, info_size, "\"dLDc\":\"%d\",",
			phealth->daily_cache.edac_llcc_data_ram.ce_cnt);
	default_scnprintf(buf, info_size, "\"dLDu\":\"%d\",",
			phealth->daily_cache.edac_llcc_data_ram.ue_cnt);
	default_scnprintf(buf, info_size, "\"dLTc\":\"%d\",",
			phealth->daily_cache.edac_llcc_tag_ram.ce_cnt);
	default_scnprintf(buf, info_size, "\"dLTu\":\"%d\",",
			phealth->daily_cache.edac_llcc_tag_ram.ue_cnt);

	for (i = 0; i < MAX_PCIE_NUM; i++) {
		default_scnprintf(buf, info_size, "\"P%zuPF\":\"%d\",", i,
				phealth->pcie[i].phy_init_fail_cnt);
		default_scnprintf(buf, info_size, "\"P%zuLD\":\"%d\",", i,
				phealth->pcie[i].link_down_cnt);
		default_scnprintf(buf, info_size, "\"P%zuLF\":\"%d\",", i,
				phealth->pcie[i].link_up_fail_cnt);
		default_scnprintf(buf, info_size, "\"P%zuLT\":\"%x\",", i,
				phealth->pcie[i].link_up_fail_ltssm);

		default_scnprintf(buf, info_size, "\"dP%zuPF\":\"%d\",", i,
				phealth->daily_pcie[i].phy_init_fail_cnt);
		default_scnprintf(buf, info_size, "\"dP%zuLD\":\"%d\",", i,
				phealth->daily_pcie[i].link_down_cnt);
		default_scnprintf(buf, info_size, "\"dP%zuLF\":\"%d\",", i,
				phealth->daily_pcie[i].link_up_fail_cnt);
		default_scnprintf(buf, info_size, "\"dP%zuLT\":\"%x\",", i,
				phealth->daily_pcie[i].link_up_fail_ltssm);
	}

	default_scnprintf(buf, info_size, "\"dNP\":\"%d\",",
			phealth->daily_rr.np);
	default_scnprintf(buf, info_size, "\"dRP\":\"%d\",",
			phealth->daily_rr.rp);
	default_scnprintf(buf, info_size, "\"dMP\":\"%d\",",
			phealth->daily_rr.mp);
	default_scnprintf(buf, info_size, "\"dKP\":\"%d\",",
			phealth->daily_rr.kp);
	default_scnprintf(buf, info_size, "\"dDP\":\"%d\",",
			phealth->daily_rr.dp);
	default_scnprintf(buf, info_size, "\"dWP\":\"%d\",",
			phealth->daily_rr.wp);
	default_scnprintf(buf, info_size, "\"dTP\":\"%d\",",
			phealth->daily_rr.tp);
	default_scnprintf(buf, info_size, "\"dSP\":\"%d\",",
			phealth->daily_rr.sp);
	default_scnprintf(buf, info_size, "\"dPP\":\"%d\",",
			phealth->daily_rr.pp);
	default_scnprintf(buf, info_size, "\"dCP\":\"%d\"",
			phealth->daily_rr.cp);

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(ap_health, 0660, show_ap_health, store_ap_health);

#define for_each_dq(dq, max_dq)						\
	for (dq = 0; dq < max_dq; dq++)

#define for_each_cs(cs, max_cs)						\
	for (cs = 0; cs < max_cs; cs++)

#define for_each_cs_dq(cs, max_cs, dq, max_dq)				\
	for_each_cs(cs, max_cs)						\
		for_each_dq(dq, max_dq)

#define for_each_ch(ch, max_ch)						\
	for (ch = 0; ch < max_ch; ch++)

#define for_each_ch_dq(ch, max_ch, dq, max_dq)				\
	for_each_ch(ch, max_ch)						\
		for_each_dq(dq, max_dq)

#define for_each_ch_cs(ch, max_ch, cs, max_cs)				\
	for_each_ch(ch, max_ch)						\
		for_each_cs(cs, max_cs)

#define for_each_ch_cs_dq(ch, max_ch, cs, max_cs, dq, max_dq)		\
	for_each_ch(ch, max_ch)						\
		for_each_cs_dq(cs, max_cs, dq, max_dq)

static ssize_t show_ddr_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

	default_scnprintf(buf, info_size, "\"DDRV\":\"%s\",",
			get_ddr_vendor_name());
	default_scnprintf(buf, info_size, "\"DSF\":\"%d.%d\",",
			(get_ddr_DSF_version() >> 16) & 0xFFFF,
			get_ddr_DSF_version() & 0xFFFF);

	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		default_scnprintf(buf, info_size, "\"RW_%d_%d_%d\":\"%d\",",
				ch, cs, dq, get_ddr_rcw_tDQSCK(ch, cs, dq));
		default_scnprintf(buf, info_size, "\"WC_%d_%d_%d\":\"%d\",",
				ch, cs, dq, get_ddr_wr_coarseCDC(ch, cs, dq));
		default_scnprintf(buf, info_size, "\"WF_%d_%d_%d\":\"%d\",",
				ch, cs, dq, get_ddr_wr_fineCDC(ch, cs, dq));
	}

	/* remove the last ',' character */
	info_size--;

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(ddr_info, 0440, show_ddr_info, NULL);

#define PARAM0_IVALID			1
#define PARAM0_LESS_THAN_0		2

#if defined(CONFIG_SEC_NOEYEINFO)
static ssize_t eye_rd_info_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        ssize_t info_size = 0;

        default_scnprintf(buf, info_size, "\"DDRV\":\"%s\",",
                        get_ddr_vendor_name());
        default_scnprintf(buf, info_size, "\"DSF\":\"%d.%d\",",
                        (get_ddr_DSF_version() >> 16) & 0xFFFF,
                        get_ddr_DSF_version() & 0xFFFF);

        default_scnprintf(buf, info_size, "\"REV1\":\"%02x\",", get_ddr_revision_id_1());
        default_scnprintf(buf, info_size, "\"REV2\":\"%02x\",", get_ddr_revision_id_2());
        default_scnprintf(buf, info_size, "\"SIZE\":\"%02x\",", get_ddr_total_density());

        /* remove the last ',' character */
        info_size--;

        check_format(buf, &info_size, DEFAULT_LEN_STR);

        return info_size;
}
static DEVICE_ATTR(eye_rd_info, 0440, eye_rd_info_show, NULL);
#else
static ssize_t eye_rd_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

	default_scnprintf(buf, info_size, "\"DDRV\":\"%s\",",
			get_ddr_vendor_name());
	default_scnprintf(buf, info_size, "\"DSF\":\"%d.%d\",",
			(get_ddr_DSF_version() >> 16) & 0xFFFF,
			get_ddr_DSF_version() & 0xFFFF);

	default_scnprintf(buf, info_size, "\"REV1\":\"%02x\",", get_ddr_revision_id_1());
	default_scnprintf(buf, info_size, "\"REV2\":\"%02x\",", get_ddr_revision_id_2());
	default_scnprintf(buf, info_size, "\"SIZE\":\"%02x\",", get_ddr_total_density());

	default_scnprintf(buf, info_size, "\"CNT\":\"%d\",",
			ddr_get_small_eye_detected());

	default_scnprintf(buf, info_size, "\"rd_width\":\"R1\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		default_scnprintf(buf, info_size, "\"RW%d_%d_%d\":\"%d\",",
				ch, cs, dq,
				ddr_get_rd_pr_width(ch, cs, dq));
	}

	default_scnprintf(buf, info_size, "\"rd_height\":\"R2\",");
	for_each_ch_cs(ch, NUM_CH, cs, NUM_CS) {
		default_scnprintf(buf, info_size, "\"RH%d_%d_x\":\"%d\",",
				ch, cs,
				ddr_get_rd_min_eye_height(ch, cs));
	}

	default_scnprintf(buf, info_size, "\"rd_vref\":\"Rx\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		default_scnprintf(buf, info_size, "\"RV%d_%d_%d\":\"%d\",",
				ch, cs, dq,
				ddr_get_rd_best_vref(ch, cs, dq));
	}

	/* remove the last ',' character */
	info_size--;

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(eye_rd_info, 0440, eye_rd_info_show, NULL);

static ssize_t eye_dcc_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

	default_scnprintf(buf, info_size, "\"dqs_dcc\":\"D3\",");
	for_each_ch_dq(ch, NUM_CH, dq, NUM_DQ_PCH) {
		default_scnprintf(buf, info_size, "\"DS%d_x_%d\":\"%d\",",
				ch, dq,
				ddr_get_dqs_dcc_adj(ch, dq));

	}

	default_scnprintf(buf, info_size, "\"dq_dcc\":\"D5\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		default_scnprintf(buf, info_size, "\"DQ%d_%d_%d\":\"%d\",",
				ch, cs, dq,
				ddr_get_dq_dcc_abs(ch, cs, dq));
	}

	/* remove the last ',' character */
	info_size--;

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(eye_dcc_info, 0440, eye_dcc_info_show, NULL);

static ssize_t eye_wr1_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

	default_scnprintf(buf, info_size, "\"wr_width\":\"W1\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		default_scnprintf(buf, info_size, "\"WW%d_%d_%d\":\"%d\",",
				ch, cs, dq,
				ddr_get_wr_pr_width(ch, cs, dq));
	}

	default_scnprintf(buf, info_size, "\"wr_height\":\"W2\",");
	for_each_ch_cs(ch, NUM_CH, cs, NUM_CS) {
		default_scnprintf(buf, info_size, "\"WH%d_%d_x\":\"%d\",",
				ch, cs,
				ddr_get_wr_min_eye_height(ch, cs));
	}

	default_scnprintf(buf, info_size, "\"wr_vref\":\"Wx\",");
	for_each_ch_cs(ch, NUM_CH, cs, NUM_CS) {
		default_scnprintf(buf, info_size, "\"WV%d_%d_x\":\"%d\",",
				ch, cs,
				ddr_get_wr_best_vref(ch, cs));
	}

	/* remove the last ',' character */
	info_size--;

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(eye_wr1_info, 0440, eye_wr1_info_show, NULL);

static ssize_t eye_wr2_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

	default_scnprintf(buf, info_size, "\"wr_topw\":\"W4\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		default_scnprintf(buf, info_size, "\"WT%d_%d_%d\":\"%d\",",
				ch, cs, dq,
				ddr_get_wr_vmax_to_vmid(ch, cs, dq));
	}

	default_scnprintf(buf, info_size, "\"wr_botw\":\"W4\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		default_scnprintf(buf, info_size, "\"WB%d_%d_%d\":\"%d\",",
				ch, cs, dq,
				ddr_get_wr_vmid_to_vmin(ch, cs, dq));
	}

	/* remove the last ',' character */
	info_size--;

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(eye_wr2_info, 0440, eye_wr2_info_show, NULL);
#endif

static int get_param0(const char *name)
{
	struct device_node *np = of_find_node_by_path("/soc/sec_hw_param");
	u32 val;
	int ret;

	if (!np) {
		pr_err("No sec_hw_param found\n");
		return -PARAM0_IVALID;
	}

	ret = of_property_read_u32(np, name, &val);
	if (ret) {
		pr_err("failed to get %s from node\n", name);
		return -PARAM0_LESS_THAN_0;
	}

	return val;
}

char * get_soc_hw_rev(void)
{
	uint32_t soc_value;
	soc_value = get_param0("soc_rev");

	switch (soc_value) {
		case CONVERSION_SOC_REV_1_0:
			return "1.0";
			break;
		case CONVERSION_SOC_REV_2_0:
			return "2.0";
			break;
		case CONVERSION_SOC_REV_2_1:
			return "2.1";
			break;
		case CONVERSION_SOC_REV_2_2:
			return "2.2";
			break;
		case CONVERSION_SOC_REV_1_1:
			return "1.1";
			break;
		default:
			return "unknown";
			break;
	}
}

static ssize_t show_ap_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	default_scnprintf(buf, info_size, "\"HW_REV\":\"%d\"", system_rev);
	default_scnprintf(buf, info_size, ",\"SoC_ID\":\"%u\"", socinfo_get_id());
	default_scnprintf(buf, info_size, ",\"SoC_REV\":\"%s\"", get_soc_hw_rev());

	default_scnprintf(buf, info_size, ",\"GC_PRM\":\"%d\"",	get_param0("g_iddq"));
	default_scnprintf(buf, info_size, ",\"GC_OPV_3\":\"%d\"", get_param0("olv_perf_t"));

	default_scnprintf(buf, info_size, ",\"DOUR\":\"%d\"", get_param0("dour"));
	default_scnprintf(buf, info_size, ",\"DOUB\":\"%d\"", get_param0("doub"));

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(ap_info, 0440, show_ap_info, NULL);

static ssize_t show_extra_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t offset = 0;
	unsigned long long rem_nsec;
	unsigned long long ts_nsec;
	unsigned int reset_reason;
	rst_exinfo_t *p_rst_exinfo = NULL;
	_kern_ex_info_t *p_kinfo = NULL;
	int cpu = -1;

	if (!__is_ready_debug_reset_header()) {
		pr_info("updated nothing.\n");
		goto out;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		goto out;

	p_rst_exinfo = kmalloc(sizeof(rst_exinfo_t), GFP_KERNEL);
	if (!p_rst_exinfo)
		goto out;

	if (!read_debug_partition(debug_index_reset_ex_info, p_rst_exinfo)) {
		pr_err("fail - get param!!\n");
		goto out;
	}

	p_kinfo = &p_rst_exinfo->kern_ex_info.info;
	cpu = p_kinfo->cpu;

	extra_scnprintf(buf, offset, "\"RR\":\"%s\",",
			sec_debug_get_reset_reason_str(reset_reason));
	extra_scnprintf(buf, offset, "\"RWC\":\"%d\",",
			sec_debug_get_reset_write_cnt());

	ts_nsec = p_kinfo->ktime;
	rem_nsec = do_div(ts_nsec, 1000000000ULL);

	extra_scnprintf(buf, offset, "\"KTIME\":\"%llu.%06llu\",",
			ts_nsec, rem_nsec / 1000ULL);
	extra_scnprintf(buf, offset, "\"CPU\":\"%d\",",
			p_kinfo->cpu);
	extra_scnprintf(buf, offset, "\"TASK\":\"%s\",",
			p_kinfo->task_name);

	if (p_kinfo->smmu.fsr) {
		extra_scnprintf(buf, offset, "\"SDN\":\"%s\",",
				p_kinfo->smmu.dev_name);
		extra_scnprintf(buf, offset, "\"FSR\":\"%x\",",
				p_kinfo->smmu.fsr);

		if (p_kinfo->smmu.fsynr0) {
			extra_scnprintf(buf, offset, "\"FSY0\":\"%x\",",
					p_kinfo->smmu.fsynr0);
			extra_scnprintf(buf, offset, "\"FSY1\":\"%x\",",
					p_kinfo->smmu.fsynr1);
			extra_scnprintf(buf, offset, "\"IOVA\":\"%08lx\",",
					p_kinfo->smmu.iova);
			extra_scnprintf(buf, offset, "\"FAR\":\"%016lx\",",
					p_kinfo->smmu.far);
			extra_scnprintf(buf, offset, "\"SMN\":\"%s\",",
					p_kinfo->smmu.mas_name);
			extra_scnprintf(buf, offset, "\"CB\":\"%d\",",
					p_kinfo->smmu.cbndx);
			extra_scnprintf(buf, offset, "\"SOFT\":\"%016llx\",",
					p_kinfo->smmu.phys_soft);
			extra_scnprintf(buf, offset, "\"ATOS\":\"%016llx\",",
					p_kinfo->smmu.phys_atos);
			extra_scnprintf(buf, offset, "\"SID\":\"%x\",",
					p_kinfo->smmu.sid);
		}
	}

	if (p_kinfo->badmode.esr) {
		extra_scnprintf(buf, offset,
				"\"BDR\":\"%08x\",", p_kinfo->badmode.reason);
		extra_scnprintf(buf, offset,
				"\"BDRS\":\"%s\",", p_kinfo->badmode.handler_str);
		extra_scnprintf(buf, offset,
				"\"BDE\":\"%08x\",", p_kinfo->badmode.esr);
		extra_scnprintf(buf, offset,
				"\"BDES\":\"%s\",", p_kinfo->badmode.esr_str);
	}

	if ((cpu > -1) && (cpu < num_present_cpus())) {
		if (p_kinfo->fault[cpu].esr) {
			extra_scnprintf(buf, offset, "\"ESR\":\"%08x\",",
					p_kinfo->fault[cpu].esr);
			extra_scnprintf(buf, offset, "\"FNM\":\"%s\",",
					p_kinfo->fault[cpu].str);
			extra_scnprintf(buf, offset, "\"FV1\":\"%016llx\",",
					p_kinfo->fault[cpu].var1);
			extra_scnprintf(buf, offset, "\"FV2\":\"%016llx\",",
					p_kinfo->fault[cpu].var2);
		}

		extra_scnprintf(buf, offset,
				"\"FAULT\":\"pgd=%016llx VA=%016llx ",
				p_kinfo->fault[cpu].pte[0],
				p_kinfo->fault[cpu].pte[1]);
		extra_scnprintf(buf, offset, "*pgd=%016llx *pud=%016llx ",
				p_kinfo->fault[cpu].pte[2],
				p_kinfo->fault[cpu].pte[3]);
		extra_scnprintf(buf, offset, "*pmd=%016llx *pte=%016llx\",",
				p_kinfo->fault[cpu].pte[4],
				p_kinfo->fault[cpu].pte[5]);
	}

	extra_scnprintf(buf, offset, "\"BUG\":\"%s\",", p_kinfo->bug_buf);
	if (strlen(p_kinfo->panic_buf)) {
		extra_scnprintf(buf, offset, "\"PANIC\":\"%s\",", p_kinfo->panic_buf);
	} else {
		if (p_rst_exinfo->uc_ex_info.magic == RESTART_REASON_SEC_DEBUG_MODE) {     
			extra_scnprintf(buf, offset, "\"PANIC\":\"UCS-%s\",", p_rst_exinfo->uc_ex_info.str);
		}
	}
	extra_scnprintf(buf, offset, "\"PC\":\"%s\",", p_kinfo->pc);
	extra_scnprintf(buf, offset, "\"LR\":\"%s\",", p_kinfo->lr);
	extra_scnprintf(buf, offset, "\"UFS\":\"%s\",", p_kinfo->ufs_err);
	extra_scnprintf(buf, offset, "\"ROT\":\"W%dC%d\",",
			get_param0("wb"), get_param0("ddi"));
	extra_scnprintf(buf, offset, "\"STACK\":\"%s\"", p_kinfo->backtrace);

out:
	kfree(p_rst_exinfo);

	check_format(buf, &offset, EXTRA_LEN_STR);

	return offset;
}
static DEVICE_ATTR(extra_info, 0440, show_extra_info, NULL);

static ssize_t show_extrb_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t offset = 0;
	int idx, cnt, max_cnt;
	unsigned long rem_nsec;
	u64 ts_nsec;
	unsigned int reset_reason;
	char *tz_cpu_status[6] = { "NA", "R", "PC", "WB", "IW", "IWT" };

	rst_exinfo_t *p_rst_exinfo = NULL;
	__rpm_log_t *pRPMlog = NULL;

	if (!__is_ready_debug_reset_header()) {
		pr_info("updated nothing.\n");
		goto out;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		goto out;

	p_rst_exinfo = kmalloc(sizeof(rst_exinfo_t), GFP_KERNEL);
	if (!p_rst_exinfo) {
		pr_err("fail - kmalloc\n");
		goto out;
	}

	if (!read_debug_partition(debug_index_reset_ex_info, p_rst_exinfo)) {
		pr_err("fail - get param!!\n");
		goto out;
	}

	special_scnprintf(buf, offset, "\"RR\":\"%s\",",
			sec_debug_get_reset_reason_str(reset_reason));

	special_scnprintf(buf, offset, "\"RWC\":\"%d\",",
			sec_debug_get_reset_write_cnt());

	if (p_rst_exinfo->rpm_ex_info.info.magic == RPM_EX_INFO_MAGIC
		 && p_rst_exinfo->rpm_ex_info.info.nlog > 0) {
		special_scnprintf(buf, offset, "\"RPM\":\"");

		if (p_rst_exinfo->rpm_ex_info.info.nlog > 5) {
			idx = p_rst_exinfo->rpm_ex_info.info.nlog % 5;
			max_cnt = 5;
		} else {
			idx = 0;
			max_cnt = p_rst_exinfo->rpm_ex_info.info.nlog;
		}

		for (cnt  = 0; cnt < max_cnt; cnt++, idx++) {
			pRPMlog = &p_rst_exinfo->rpm_ex_info.info.log[idx % 5];

			ts_nsec = pRPMlog->nsec;
			rem_nsec = do_div(ts_nsec, 1000000000);

			special_scnprintf(buf, offset, "%lu.%06lu ",
					(unsigned long)ts_nsec, rem_nsec / 1000);

			special_scnprintf(buf, offset, "%s ", pRPMlog->msg);

			special_scnprintf(buf, offset, "%x %x %x %x",
					pRPMlog->arg[0], pRPMlog->arg[1],
					pRPMlog->arg[2], pRPMlog->arg[3]);

			if (cnt == max_cnt - 1)
				special_scnprintf(buf, offset, "\",");
			else
				special_scnprintf(buf, offset, "/");
		}
	}
	special_scnprintf(buf, offset, "\"TZ_RR\":\"%s\"",
			p_rst_exinfo->tz_ex_info.msg);

	special_scnprintf(buf, offset, ",\"TZBC\":\"");
	max_cnt = sizeof(p_rst_exinfo->tz_ex_info.cpu_status);
	max_cnt = max_cnt / sizeof(p_rst_exinfo->tz_ex_info.cpu_status[0]);
	for (cnt = 0; cnt < max_cnt; cnt++) {
		if (p_rst_exinfo->tz_ex_info.cpu_status[cnt] >
		    INVALID_WARM_TERM_ENTRY_EXIT_COUNT)
			special_scnprintf(buf, offset, "%s", tz_cpu_status[0]);
		else
			special_scnprintf(buf, offset, "%s",
					tz_cpu_status[p_rst_exinfo->tz_ex_info.cpu_status[cnt]]);

		if (cnt != max_cnt - 1)
			special_scnprintf(buf, offset, ",");
	}
	special_scnprintf(buf, offset, "\"");

	special_scnprintf(buf, offset,
			",\"PIMEM\":\"0x%08x,0x%08x,0x%08x,0x%08x\"",
			p_rst_exinfo->pimem_info.esr,
			p_rst_exinfo->pimem_info.ear0,
			p_rst_exinfo->pimem_info.esr_sdi,
			p_rst_exinfo->pimem_info.ear0_sdi);

	special_scnprintf(buf, offset,
			",\"COST\":\"%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\"",
			(long int)p_rst_exinfo->cpu_stuck_info.cpu[0],
			(long int)p_rst_exinfo->cpu_stuck_info.cpu[1],
			(long int)p_rst_exinfo->cpu_stuck_info.cpu[2],
			(long int)p_rst_exinfo->cpu_stuck_info.cpu[3],
			(long int)p_rst_exinfo->cpu_stuck_info.cpu[4],
			(long int)p_rst_exinfo->cpu_stuck_info.cpu[5],
			(long int)p_rst_exinfo->cpu_stuck_info.cpu[6],
			(long int)p_rst_exinfo->cpu_stuck_info.cpu[7]);

	special_scnprintf(buf, offset, ",\"HYC\":\"%d\"",
			p_rst_exinfo->hyp_ex_info.s2_fault_counter);
	special_scnprintf(buf, offset, ",\"HYP\":\"%s\"",
			p_rst_exinfo->hyp_ex_info.msg);

	special_scnprintf(buf, offset, ",\"LPM\":\"");
	max_cnt = sizeof(p_rst_exinfo->kern_ex_info.info.lpm_state);
	max_cnt = max_cnt/sizeof(p_rst_exinfo->kern_ex_info.info.lpm_state[0]);
	for (idx = 0; idx < max_cnt; idx++) {
		special_scnprintf(buf, offset, "%x",
				p_rst_exinfo->kern_ex_info.info.lpm_state[idx]);
		if (idx != max_cnt - 1)
			special_scnprintf(buf, offset, ",");
	}
	special_scnprintf(buf, offset, "\"");

	special_scnprintf(buf, offset, ",\"PKO\":\"%x\"",
			p_rst_exinfo->kern_ex_info.info.pko);

	special_scnprintf(buf, offset, ",\"LR\":\"");
	max_cnt = sizeof(p_rst_exinfo->kern_ex_info.info.lr_val);
	max_cnt = max_cnt/sizeof(p_rst_exinfo->kern_ex_info.info.lr_val[0]);
	for (idx = 0; idx < max_cnt; idx++) {
		special_scnprintf(buf, offset, "%llx",
				p_rst_exinfo->kern_ex_info.info.lr_val[idx]);
		if (idx != max_cnt - 1)
			special_scnprintf(buf, offset, ",");
	}
	special_scnprintf(buf, offset, "\"");

	special_scnprintf(buf, offset, ",\"PC\":\"");
	max_cnt = sizeof(p_rst_exinfo->kern_ex_info.info.pc_val);
	max_cnt = max_cnt/sizeof(p_rst_exinfo->kern_ex_info.info.pc_val[0]);
	for (idx = 0; idx < max_cnt; idx++) {
		special_scnprintf(buf, offset, "%llx",
				p_rst_exinfo->kern_ex_info.info.pc_val[idx]);
		if (idx != max_cnt - 1)
			special_scnprintf(buf, offset, ",");
	}
	special_scnprintf(buf, offset, "\"");

out:
	kfree(p_rst_exinfo);

	check_format(buf, &offset, SPECIAL_LEN_STR);

	return offset;
}

static DEVICE_ATTR(extrb_info, 0440, show_extrb_info, NULL);

static ssize_t show_extrc_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t offset = 0;
	unsigned int reset_reason;
	unsigned int i = 0;
	char *extrc_buf = NULL;

	if (!__is_ready_debug_reset_header()) {
		pr_info("updated nothing.\n");
		goto out;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		goto out;

	extrc_buf = kmalloc(SEC_DEBUG_RESET_EXTRC_SIZE, GFP_KERNEL);
	if (!extrc_buf) {
		pr_err("fail - kmalloc\n");
		goto out;
	}

	if (!read_debug_partition(debug_index_reset_extrc_info, extrc_buf)) {
		pr_err("fail - get param!!\n");
		goto out;
	}

	special_scnprintf(buf, offset, "\"RR\":\"%s\",",
			sec_debug_get_reset_reason_str(reset_reason));

	special_scnprintf(buf, offset, "\"RWC\":\"%d\",",
			sec_debug_get_reset_write_cnt());

	/* check " character and then change ' character */
	for (i = 0; i < SEC_DEBUG_RESET_EXTRC_SIZE; i++) {
		if (extrc_buf[i] == '"')
			extrc_buf[i] = '\'';
		if (extrc_buf[i] == '\0')
			break;
	}

	extrc_buf[SEC_DEBUG_RESET_EXTRC_SIZE-1] = '\0';

	special_scnprintf(buf, offset, "\"LKL\":\"%s\"", &extrc_buf[offset]);
out:
	kfree(extrc_buf);

	check_format(buf, &offset, SPECIAL_LEN_STR);

	return offset;
}

static DEVICE_ATTR(extrc_info, 0440, show_extrc_info, NULL);

static ssize_t show_extrt_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t offset = 0;
	unsigned int reset_reason;
	struct tzdbg_t *tz_diag_info = NULL;
	struct tzdbg_log_v9_2_t *log_v9_2 = NULL; 
	unsigned int i = 0;

	if (!__is_ready_debug_reset_header()) {
		pr_info("updated nothing.\n");
		goto out;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		goto out;

	tz_diag_info = kmalloc(SEC_DEBUG_RESET_TZLOG_SIZE, GFP_KERNEL);
	if (!tz_diag_info) {
		pr_err("%s : fail - kmalloc\n", __func__);
		goto out;
	}

	if (!read_debug_partition(debug_index_reset_tzlog, tz_diag_info)) {
		pr_err("%s : fail - get param!!\n", __func__);
		goto out;
	}

	special_scnprintf(buf, offset, "\"RR\":\"%s\",",
			sec_debug_get_reset_reason_str(reset_reason));

	special_scnprintf(buf, offset, "\"RWC\":\"%d\",",
			sec_debug_get_reset_write_cnt());

	if (tz_diag_info->magic_num != TZ_DIAG_LOG_MAGIC) {
		special_scnprintf(buf, offset,
				"\"ERR\":\"tzlog magic num error\"");
	} else {
		if (tz_diag_info->ring_len > 0x200) {	/* 512 byte */
			special_scnprintf(buf, offset,
					"\"ERR\":\"tzlog size over\"");
		} else {
			special_scnprintf(buf, offset, "\"TZDA\":\"");
			special_scnprintf(buf, offset, "%08X%08X%08X",
					tz_diag_info->magic_num,
					tz_diag_info->version,
					tz_diag_info->cpu_count);
			special_scnprintf(buf, offset, "%08X%08X%08X",
					tz_diag_info->ring_off,
					tz_diag_info->ring_len,
					tz_diag_info->num_interrupts);

			for (i = 0; i < TZBSP_AES_256_ENCRYPTED_KEY_SIZE; i++)
				special_scnprintf(buf, offset, "%02X",
						tz_diag_info->key[i]);

			for (i = 0; i < TZBSP_NONCE_LEN; i++)
				special_scnprintf(buf, offset, "%02X",
						tz_diag_info->nonce[i]);

			for (i = 0; i < TZBSP_TAG_LEN; i++)
				special_scnprintf(buf, offset, "%02X",
						tz_diag_info->tag[i]);

			if (tz_diag_info->version >= TZBSP_DIAG_VERSION_V9_2) {  // sm8350
				log_v9_2 = (struct tzdbg_log_v9_2_t *)&tz_diag_info->ring_buffer;
				special_scnprintf(buf, offset, "%08X%08X",
						log_v9_2->log_pos.wrap,
						log_v9_2->log_pos.offset);

				for (i = 0; i < tz_diag_info->ring_len; i++)
					special_scnprintf(buf, offset, "%02X",
							log_v9_2->log_buf[i]);

			} else {
				special_scnprintf(buf, offset, "%04X%04X",
						tz_diag_info->ring_buffer.log_pos.wrap,
						tz_diag_info->ring_buffer.log_pos.offset);

				for (i = 0; i < tz_diag_info->ring_len; i++)
					special_scnprintf(buf, offset, "%02X",
							tz_diag_info->ring_buffer.log_buf[i]);
			}

			special_scnprintf(buf, offset, "\"");
		}
	}

out:
	kfree(tz_diag_info);

	check_format(buf, &offset, SPECIAL_LEN_STR);

	return offset;
}

static DEVICE_ATTR(extrt_info, 0440, show_extrt_info, NULL);

static ssize_t show_extrm_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t offset = 0;
	unsigned int reset_reason;
	char *extrm_buf = NULL;

	if (!__is_ready_debug_reset_header()) {
		pr_info("updated nothing.\n");
		goto out;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		goto out;

	extrm_buf = kmalloc(SEC_DEBUG_RESET_ETRM_SIZE, GFP_KERNEL);
	if (!extrm_buf) {
		pr_err("%s : fail - kmalloc\n", __func__);
		goto out;
	}

	if (!read_debug_partition(debug_index_reset_rkplog, extrm_buf)) {
		pr_err("%s : fail - get param!!\n", __func__);
		goto out;
	}

	offset += scnprintf((char*)(buf + offset), SPECIAL_LEN_STR - offset,
			"\"RR\":\"%s\",", sec_debug_get_reset_reason_str(reset_reason));

	offset += scnprintf((char*)(buf + offset), SPECIAL_LEN_STR - offset,
			"\"RWC\":\"%d\",", sec_debug_get_reset_write_cnt());

	extrm_buf[SEC_DEBUG_RESET_ETRM_SIZE-1] = '\0';

	offset += scnprintf((char*)(buf + offset), SPECIAL_LEN_STR - offset,
			"\"RKP\":\"%s\"", &extrm_buf[offset]);
out:
	if (extrm_buf)
		kfree(extrm_buf);

	check_format(buf, &offset, SPECIAL_LEN_STR);
	return offset;

}

static DEVICE_ATTR(extrm_info, 0440, show_extrm_info, NULL);

static int sec_hw_param_dbg_part_notifier_callback(
	struct notifier_block *nfb, unsigned long action, void *data)
{
	switch (action) {
	case DBG_PART_DRV_INIT_DONE:
		phealth = ap_health_data_read();
		clean_batt_info();
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct attribute *sec_hw_param_attributes[] = {
	&dev_attr_ap_info.attr,
	&dev_attr_ddr_info.attr,
	&dev_attr_eye_rd_info.attr,
#if !defined(CONFIG_SEC_NOEYEINFO)
	&dev_attr_eye_wr1_info.attr,
	&dev_attr_eye_wr2_info.attr,
	&dev_attr_eye_dcc_info.attr,
#endif
	&dev_attr_ap_health.attr,
	&dev_attr_last_dcvs.attr,
	&dev_attr_extra_info.attr,
	&dev_attr_extrb_info.attr,
	&dev_attr_extrc_info.attr,
	&dev_attr_extrt_info.attr,
	&dev_attr_extrm_info.attr,
	NULL,
};

static const struct attribute_group sec_hw_param_attribute_group = {
	.attrs = sec_hw_param_attributes,
};

#define EXTEND_RR_SIZE		150
static int sec_errp_extra_show(struct seq_file *m, void *v)
{
	ssize_t offset = 0;
	unsigned int reset_reason;
	rst_exinfo_t *p_rst_exinfo = NULL;
	_kern_ex_info_t *p_kinfo = NULL;
	int cpu = -1;
	char upload_cause_str[80] = {0,};
	char buf[EXTEND_RR_SIZE] = {0, };

	if (!__is_ready_debug_reset_header()) {
		pr_info("updated nothing.\n");
		goto out;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		goto out;

	p_rst_exinfo = kmalloc(sizeof(rst_exinfo_t), GFP_KERNEL);
	if (!p_rst_exinfo)
		goto out;

	if (!read_debug_partition(debug_index_reset_ex_info, p_rst_exinfo)) {
		pr_err("fail - get param!!\n");
		goto out;
	}
	p_kinfo = &p_rst_exinfo->kern_ex_info.info;
	cpu = p_kinfo->cpu;

	offset += scnprintf((char*)(buf + offset), EXTEND_RR_SIZE - offset,
			"RWC:%d", sec_debug_get_reset_write_cnt());

	if (reset_reason == USER_UPLOAD_CAUSE_SMPL) {
		if (strstr(p_kinfo->panic_buf, "SMPL")) {
			offset += scnprintf((char*)(buf + offset), EXTEND_RR_SIZE - offset,
				" PANIC:%s", p_kinfo->panic_buf);
		}
		goto out;
	}

	if (p_rst_exinfo->uc_ex_info.magic == RESTART_REASON_SEC_DEBUG_MODE) {
		offset += scnprintf((char*)(buf + offset), EXTEND_RR_SIZE - offset,
				" UPLOAD:%s", p_rst_exinfo->uc_ex_info.str);
	} else {
		sec_debug_upload_cause_str(p_kinfo->upload_cause,
				upload_cause_str, sizeof(upload_cause_str));
		offset += scnprintf((char*)(buf + offset), EXTEND_RR_SIZE - offset,
				" UPLOAD:%s_0x%x", upload_cause_str, p_kinfo->upload_cause);
	}

	if (reset_reason == USER_UPLOAD_CAUSE_WATCHDOG) {
		goto out;
	} else if (reset_reason == USER_UPLOAD_CAUSE_WTSR
			|| reset_reason == USER_UPLOAD_CAUSE_POWER_RESET) {
		offset += scnprintf((char*)(buf + offset), EXTEND_RR_SIZE - offset,
				" TZ_RR:%s", p_rst_exinfo->tz_ex_info.msg);
		goto out;
	}

	offset += scnprintf((char*)(buf + offset), EXTEND_RR_SIZE - offset,
			" PANIC:%s", p_kinfo->panic_buf);
	offset += scnprintf((char*)(buf + offset), EXTEND_RR_SIZE - offset,
			" PC:%s", p_kinfo->pc);
	offset += scnprintf((char*)(buf + offset), EXTEND_RR_SIZE - offset,
			" LR:%s", p_kinfo->lr);
out:
	kfree(p_rst_exinfo);

	if (offset != 0)
		seq_puts(m, buf);

	return 0;
}

#ifdef CONFIG_SEC_PM
static ssize_t show_pwrsrc(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0; //sec_get_pwrsrc(buf);
}
static DEVICE_ATTR(pwrsrc, 0440, show_pwrsrc, NULL);
#endif
static struct attribute *sec_reset_reason_attributes[] = {
#ifdef CONFIG_SEC_PM
	&dev_attr_pwrsrc.attr,
#endif
	NULL,
};

static const struct attribute_group sec_reset_reason_attribute_group = {
	.attrs = sec_reset_reason_attributes,
};

static int sec_errp_extra_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_errp_extra_show, NULL);
}

static const struct file_operations sec_errp_extra_proc_fops = {
	.open = sec_errp_extra_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static struct notifier_block sec_hw_param_dbg_part_notifier = {
	.notifier_call = sec_hw_param_dbg_part_notifier_callback,
};

static int __init sec_hw_param_init(void)
{
	struct proc_dir_entry *entry;
	struct device *sec_hw_param_dev;
	struct device *sec_reset_reason_dev;
	int err_hw_param;
	int err_errp_extra = 0;

	dbg_partition_notifier_register(&sec_hw_param_dbg_part_notifier);

	sec_hw_param_dev = ___sec_device_create(NULL, "sec_hw_param");
	if (IS_ERR(sec_hw_param_dev)) {
		pr_err("Failed to create devce\n");
		return -ENODEV;
	}

	err_hw_param = sysfs_create_group(&sec_hw_param_dev->kobj,
			&sec_hw_param_attribute_group);
	if (unlikely(err_hw_param)) {
		pr_err("Failed to create device files!\n");
		sec_device_destroy(sec_hw_param_dev->devt);
	}

	sec_reset_reason_dev = ___sec_device_create(NULL, "sec_reset_reason");
	if (IS_ERR(sec_reset_reason_dev)) {
		pr_err("Failed to create devce\n");
		return -ENODEV;
	}

	entry = proc_create("extra", S_IWUGO, NULL,
			&sec_errp_extra_proc_fops);
	if (unlikely(!entry))
		err_errp_extra = -ENODEV;

	return (err_hw_param | err_errp_extra);
}
device_initcall(sec_hw_param_init);
