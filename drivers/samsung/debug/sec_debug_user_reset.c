// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_debug_user_reset.c
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

#include <asm/stacktrace.h>

#include <linux/device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/sec_debug.h>
#include <linux/sec_param.h>
#include <linux/sec_class.h>

#ifdef CONFIG_RKP_CFP_ROPP
#include <linux/rkp_cfp.h>
#endif
#ifdef CONFIG_CFP_ROPP
#include <linux/cfp.h>
#endif

#include <linux/qseecom.h>
#include "sec_debug_internal.h"

static char rr_str[][3] = {
	[USER_UPLOAD_CAUSE_SMPL]		= "SP",
	[USER_UPLOAD_CAUSE_WTSR]		= "WP",
	[USER_UPLOAD_CAUSE_WATCHDOG]		= "DP",
	[USER_UPLOAD_CAUSE_PANIC]		= "KP",
	[USER_UPLOAD_CAUSE_MANUAL_RESET]	= "MP",
	[USER_UPLOAD_CAUSE_POWER_RESET]		= "PP",
	[USER_UPLOAD_CAUSE_REBOOT]		= "RP",
	[USER_UPLOAD_CAUSE_BOOTLOADER_REBOOT]	= "BP",
	[USER_UPLOAD_CAUSE_POWER_ON]		= "NP",
	[USER_UPLOAD_CAUSE_THERMAL]		= "TP",
	[USER_UPLOAD_CAUSE_CP_CRASH]		= "CP",
	[USER_UPLOAD_CAUSE_UNKNOWN]		= "NP",
};

static char *klog_buf;
static uint32_t klog_size;
static char *klog_read_buf;
static struct debug_reset_header *klog_info;
static DEFINE_MUTEX(klog_mutex);

static char *summary_buf;
static struct debug_reset_header *summary_info;
static DEFINE_MUTEX(summary_mutex);
static unsigned int reset_reason = 0xFFEEFFEE;

static char *tzlog_buf;
static struct debug_reset_header *tzlog_info;
static DEFINE_MUTEX(tzlog_mutex);

static int reset_write_cnt = -1;

static char *auto_comment_buf;
static struct debug_reset_header *auto_comment_info;
static DEFINE_MUTEX(auto_comment_mutex);

static char *reset_history_buf;
static unsigned int reset_history_size;
static char *reset_history_read_buf;
static struct debug_reset_header *reset_history_info;
static DEFINE_MUTEX(reset_history_mutex);

static unsigned int lpm_mode;

static int check_lpm_mode(char *str)
{
	if (strncmp(str, "charger", 7) == 0)
		lpm_mode = 1;
	else
		lpm_mode = 0;

	return 0;
}
early_param("androidboot.mode", check_lpm_mode);

unsigned int sec_debug_get_reset_reason(void)
{
	return reset_reason;
}

int sec_debug_get_reset_write_cnt(void)
{
	return reset_write_cnt;
}

char *sec_debug_get_reset_reason_str(unsigned int reason)
{
	if (reason < USER_UPLOAD_CAUSE_MIN || reason > USER_UPLOAD_CAUSE_MAX)
		reason = USER_UPLOAD_CAUSE_UNKNOWN;

	return rr_str[reason];
}

static void sec_debug_update_reset_reason(uint32_t debug_partition_rr)
{
	static bool updated;

	if (!updated) {
		reset_reason = debug_partition_rr;
		updated = true;
		pr_info("partition[%d] result[%s]\n",
			debug_partition_rr,
			sec_debug_get_reset_reason_str(reset_reason));
	}
}

static void reset_reason_update_and_clear(void)
{
	ap_health_t *p_health;
	unsigned int rr_data;

	p_health = ap_health_data_read();
	if (p_health == NULL) {
		pr_err("p_health is NULL\n");
		return;
	}

	pr_info("done\n");
	rr_data = sec_debug_get_reset_reason();
	switch (rr_data) {
	case USER_UPLOAD_CAUSE_SMPL:
		p_health->daily_rr.sp++;
		p_health->rr.sp++;
		break;
	case USER_UPLOAD_CAUSE_WTSR:
		p_health->daily_rr.wp++;
		p_health->rr.wp++;
		break;
	case USER_UPLOAD_CAUSE_WATCHDOG:
		p_health->daily_rr.dp++;
		p_health->rr.dp++;
		break;
	case USER_UPLOAD_CAUSE_PANIC:
		p_health->daily_rr.kp++;
		p_health->rr.kp++;
		break;
	case USER_UPLOAD_CAUSE_MANUAL_RESET:
		p_health->daily_rr.mp++;
		p_health->rr.mp++;
		break;
	case USER_UPLOAD_CAUSE_POWER_RESET:
		p_health->daily_rr.pp++;
		p_health->rr.pp++;
		break;
	case USER_UPLOAD_CAUSE_REBOOT:
		p_health->daily_rr.rp++;
		p_health->rr.rp++;
		break;
	case USER_UPLOAD_CAUSE_THERMAL:
		p_health->daily_rr.tp++;
		p_health->rr.tp++;
		break;
	case USER_UPLOAD_CAUSE_CP_CRASH:
		p_health->daily_rr.cp++;
		p_health->rr.cp++;
		break;
	default:
		p_health->daily_rr.np++;
		p_health->rr.np++;
	}

	p_health->last_rst_reason = 0;
	ap_health_data_write(p_health);
}

static int set_reset_reason_proc_show(struct seq_file *m, void *v)
{
	uint32_t rr_data = sec_debug_get_reset_reason();
	static uint32_t rr_cnt_update = 1;

	seq_printf(m, "%sON\n", sec_debug_get_reset_reason_str(rr_data));

	if (!lpm_mode) {
		if (rr_cnt_update) {
			reset_reason_update_and_clear();
			rr_cnt_update = 0;
		}
	}

	return 0;
}

static int sec_reset_reason_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_reset_reason_proc_show, NULL);
}

static const struct file_operations sec_reset_reason_proc_fops = {
	.open = sec_reset_reason_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static phys_addr_t sec_debug_reset_ex_info_paddr;
static unsigned sec_debug_reset_ex_info_size;
rst_exinfo_t *sec_debug_reset_ex_info;
ex_info_fault_t ex_info_fault[NR_CPUS];

void sec_debug_store_extc_idx(bool prefix)
{
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		if (p_ex_info->extc_idx == 0) {
			p_ex_info->extc_idx = get_sec_log_idx();
			if (prefix)
				p_ex_info->extc_idx += SEC_DEBUG_RESET_EXTRC_SIZE;
		}
	}
}

static void __sec_debug_store_bug_string(const char *fmt, ...)
{
	va_list args;
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		va_start(args, fmt);
		vsnprintf(p_ex_info->bug_buf,
			sizeof(p_ex_info->bug_buf), fmt, args);
		va_end(args);
	}
}

#define MAX_BUG_STRING_SIZE		56

void sec_debug_store_bug_string(const char *file, unsigned bug_line)
{
	const char *token;
	size_t length;

	if (!file)
		return;

	token = strstr(file, "kernel");
	if (!token)
		token = file;

	length = strlen(token);
	if (length > MAX_BUG_STRING_SIZE)
		__sec_debug_store_bug_string("%s %u",
				&token[length - MAX_BUG_STRING_SIZE],
				bug_line);
	else
		__sec_debug_store_bug_string("%s %u", token, bug_line);
}

void sec_debug_store_additional_dbg(enum extra_info_dbg_type type,
				    unsigned int value, const char *fmt, ...)
{
	va_list args;
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		switch (type) {
		case DBG_1_UFS_ERR:
			va_start(args, fmt);
			vsnprintf(p_ex_info->ufs_err,
					sizeof(p_ex_info->ufs_err), fmt, args);
			va_end(args);
			break;
		default:
			break;
		}
	}
}

static void sec_debug_init_panic_extra_info(void)
{
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		memset((void *)&sec_debug_reset_ex_info->kern_ex_info, 0,
			sizeof(sec_debug_reset_ex_info->kern_ex_info));
		p_ex_info->cpu = -1;
		pr_info("ex_info memory initialized size[%ld]\n",
			sizeof(kern_exinfo_t));
	}
}

static int __init sec_debug_ex_info_setup(char *str)
{
	unsigned int size = memparse(str, &str);
	int ret;

	if (size && (*str == '@')) {
		unsigned long long base = 0;

		ret = kstrtoull(++str, 0, &base);
		if (ret) {
			pr_err("failed to parse sec_dbg_ex_info\n");
			return ret;
		}

		sec_debug_reset_ex_info_paddr = base;
		sec_debug_reset_ex_info_size =
					(size + 0x1000 - 1) & ~(0x1000 - 1);

		pr_info("ex info phy=%pa, size=0x%x\n",
				&sec_debug_reset_ex_info_paddr,
				sec_debug_reset_ex_info_size);
	}
	return 1;
}
__setup("sec_dbg_ex_info=", sec_debug_ex_info_setup);

static int __init sec_debug_get_extra_info_region(void)
{
	if (!sec_debug_reset_ex_info_paddr || !sec_debug_reset_ex_info_size)
		return -1;

	sec_debug_reset_ex_info = ioremap_cache(sec_debug_reset_ex_info_paddr,
						sec_debug_reset_ex_info_size);

	if (!sec_debug_reset_ex_info) {
		pr_err("Failed to remap nocache ex info region\n");
		return -1;
	}

	sec_debug_init_panic_extra_info();

	return 0;
}
arch_initcall_sync(sec_debug_get_extra_info_region);

static phys_addr_t sec_debug_rdx_bootdev_paddr;
static u64 sec_debug_rdx_bootdev_size;
static DEFINE_MUTEX(rdx_bootdev_mutex);

static void sec_debug_free_rdx_bootdev(phys_addr_t paddr, u64 size)
{
/* caution : this fuction should be called in rdx_bootdev_mutex protected region. */
	int ret;

	pr_info("start (%pa, 0x%llx)\n", &paddr, size);

	if (!sec_debug_rdx_bootdev_paddr) {
		pr_err("reserved addr is null\n");
		goto out;
	}

	if (!sec_debug_rdx_bootdev_size) {
		pr_err("reserved size is zero\n");
		goto out;
	}

	if (paddr < sec_debug_rdx_bootdev_paddr) {
		pr_err("paddr is not valid\n");
		goto out;
	}

	if (paddr + size > sec_debug_rdx_bootdev_paddr + sec_debug_rdx_bootdev_size) {
		pr_err("range is not valid\n");
		goto out;
	}

	memset(phys_to_virt(paddr), 0, size);

	ret = memblock_free(paddr, size);
	if (ret) {
		pr_err("memblock_free failed (ret : %d)\n", ret);
		goto out;
	}

	free_reserved_area(phys_to_virt(paddr), phys_to_virt(paddr) + size, -1, "rdx_bootdev");

	if (sec_debug_rdx_bootdev_paddr == paddr) {
		sec_debug_rdx_bootdev_paddr = 0;
	}

	sec_debug_rdx_bootdev_size -= size;

out:
	pr_info("end\n");
}

static ssize_t sec_debug_rdx_bootdev_proc_write(struct file *file,
	const char __user *buf, size_t count, loff_t *ppos)
{
	int err = -ERANGE;
	struct fiemap *pfiemap;
	phys_addr_t paddr;
	u64 size;

	mutex_lock(&rdx_bootdev_mutex);

	paddr = sec_debug_rdx_bootdev_paddr;
	size = sec_debug_rdx_bootdev_size;

	if (!sec_debug_rdx_bootdev_paddr) {
		pr_err("sec_debug_rdx_bootdev_paddr is null\n");
		err = -EFAULT;
		goto out;
	}

	if (!buf) {
		err = -ENODEV;
	} else {
		if (count > sec_debug_rdx_bootdev_size) {
			pr_err("size is wrong %zu > %llu\n", count, sec_debug_rdx_bootdev_size);
			err = -EINVAL;
			goto out;
		}

		if (copy_from_user(phys_to_virt(sec_debug_rdx_bootdev_paddr),
				buf, count)) {
			pr_err("copy_from_user failed\n");
			err = -EFAULT;
			goto out;
		}

		pfiemap = phys_to_virt(sec_debug_rdx_bootdev_paddr) + SHA256_DIGEST_LENGTH;
		paddr = virt_to_phys(&pfiemap->fm_extents[pfiemap->fm_mapped_extents]);
		if (paddr <
			sec_debug_rdx_bootdev_paddr + sec_debug_rdx_bootdev_size) {
			paddr = ALIGN(paddr, PAGE_SIZE);
			size = paddr - sec_debug_rdx_bootdev_paddr;
			size = sec_debug_rdx_bootdev_size - size;
			err = 0;
		} else {
			pr_err("out of bound\n");
		}
	}
out:
	sec_debug_free_rdx_bootdev(paddr, size);
	mutex_unlock(&rdx_bootdev_mutex);
	return err < 0 ? err : count;
}

static const struct file_operations sec_debug_rdx_bootdev_fops = {
	.owner = THIS_MODULE,
	.write = sec_debug_rdx_bootdev_proc_write,
};

static int __init sec_debug_map_rdx_bootdev_region(void)
{
	struct device_node *parent, *node;
	int ret = 0;
	u64 temp[2];

	parent = of_find_node_by_path("/reserved-memory");
	if (!parent) {
		pr_err("failed to find reserved-memory node\n");
		return -EINVAL;
	}

	node = of_find_node_by_name(parent, "sec_debug_rdx_bootdev");
	if (!node) {
		pr_err("failed to find sec_debug_rdx_bootdev\n");
		return -EINVAL;
	}

	ret = of_property_read_u64_array(node, "reg", &temp[0], 2);
	if (ret) {
		pr_err("failed to get address from node\n");
		return -1;
	}

	mutex_lock(&rdx_bootdev_mutex);
	sec_debug_rdx_bootdev_paddr = temp[0];
	sec_debug_rdx_bootdev_size = temp[1];

	pr_info("sec_debug_rdx_bootdev : %pa, 0x%llx\n",
		&sec_debug_rdx_bootdev_paddr, sec_debug_rdx_bootdev_size);

	if (!sec_debug_rdx_bootdev_paddr || !sec_debug_rdx_bootdev_size) {
		pr_err("failed to get address from node\n");
		mutex_unlock(&rdx_bootdev_mutex);
		return -1;
	}

	if (!sec_debug_is_enabled()) {
		pr_info("sec_debug is not enabled\n");
		sec_debug_free_rdx_bootdev(sec_debug_rdx_bootdev_paddr, sec_debug_rdx_bootdev_size);
		mutex_unlock(&rdx_bootdev_mutex);
		return 0;
	}

	mutex_unlock(&rdx_bootdev_mutex);
	return 0;
}
arch_initcall_sync(sec_debug_map_rdx_bootdev_region);

struct debug_reset_header *get_debug_reset_header(void)
{
	struct debug_reset_header *header = NULL;
	static int get_state = DRH_STATE_INIT;

	if (get_state == DRH_STATE_INVALID)
		return NULL;

	header = kmalloc(sizeof(struct debug_reset_header), GFP_KERNEL);
	if (!header) {
		pr_err("fail - kmalloc for debug_reset_header\n");
		return NULL;
	}
	if (!read_debug_partition(debug_index_reset_summary_info, header)) {
		pr_err("fail - get param!! debug_reset_header\n");
		kfree(header);
		header = NULL;
		return NULL;
	}

	if (get_state != DRH_STATE_VALID) {
		if (header->write_times == header->read_times) {
			pr_err("untrustworthy debug_reset_header\n");
			get_state = DRH_STATE_INVALID;
			kfree(header);
			header = NULL;
			return NULL;
		}
		reset_write_cnt = header->write_times;
		get_state = DRH_STATE_VALID;
	}

	return header;
}

static int set_debug_reset_header(struct debug_reset_header *header)
{
	int ret = 0;
	static int set_state = DRH_STATE_INIT;

	if (set_state == DRH_STATE_VALID) {
		pr_info("debug_reset_header working well\n");
		return ret;
	}

	if ((header->write_times - 1) == header->read_times) {
		pr_info("debug_reset_header working well\n");
		header->read_times++;
	} else {
		pr_info("debug_reset_header read[%d] and write[%d] work sync error.\n",
				header->read_times, header->write_times);
		header->read_times = header->write_times;
	}

	if (!write_debug_partition(debug_index_reset_summary_info, header)) {
		pr_err("fail - set param!! debug_reset_header\n");
		ret = -ENOENT;
	} else {
		set_state = DRH_STATE_VALID;
	}

	return ret;
}

static int sec_reset_summary_info_init(void)
{
	int ret = 0;

	if (summary_buf != NULL)
		return true;

	if (summary_info != NULL) {
		pr_err("already memory alloc for summary_info\n");
		return -EINVAL;
	}

	summary_info = get_debug_reset_header();
	if (summary_info == NULL)
		return -EINVAL;

	if (summary_info->summary_size > SEC_DEBUG_RESET_SUMMARY_SIZE) {
		pr_err("summary_size has problem.\n");
		ret = -EINVAL;
		goto error_summary_info;
	}

	summary_buf = vmalloc(SEC_DEBUG_RESET_SUMMARY_SIZE);
	if (!summary_buf) {
		pr_err("fail - kmalloc for summary_buf\n");
		ret = -ENOMEM;
		goto error_summary_info;
	}
	if (!read_debug_partition(debug_index_reset_summary, summary_buf)) {
		pr_err("fail - get param!! summary data\n");
		ret = -ENOENT;
		goto error_summary_buf;
	}

	pr_info("w[%d] r[%d] idx[%d] size[%d]\n",
			summary_info->write_times, summary_info->read_times,
			summary_info->ap_klog_idx, summary_info->summary_size);

	return ret;

error_summary_buf:
	vfree(summary_buf);
error_summary_info:
	kfree(summary_info);

	return ret;
}

static int sec_reset_summary_completed(void)
{
	int ret = 0;

	ret = set_debug_reset_header(summary_info);

	vfree(summary_buf);
	kfree(summary_info);

	summary_info = NULL;
	summary_buf = NULL;
	pr_info("finish\n");

	return ret;
}

static ssize_t sec_reset_summary_info_proc_read(struct file *file,
			char __user *buf, size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	mutex_lock(&summary_mutex);
	if (sec_reset_summary_info_init() < 0) {
		mutex_unlock(&summary_mutex);
		return -ENOENT;
	}

	if ((pos >= summary_info->summary_size) ||
	    (pos >= SEC_DEBUG_RESET_SUMMARY_SIZE)) {
		pr_info("pos %lld, size %d\n", pos, summary_info->summary_size);
		sec_reset_summary_completed();
		mutex_unlock(&summary_mutex);
		return 0;
	}

	count = min(len, (size_t)(summary_info->summary_size - pos));
	if (copy_to_user(buf, summary_buf + pos, count)) {
		mutex_unlock(&summary_mutex);
		return -EFAULT;
	}

	*offset += count;
	mutex_unlock(&summary_mutex);
	return count;
}

static const struct file_operations sec_reset_summary_info_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_summary_info_proc_read,
};

static int sec_reset_klog_init(void)
{
	int ret = 0;
	uint32_t klog_buf_max_size, last_idx;
	char *log_src;

	if ((klog_read_buf != NULL) && (klog_buf != NULL))
		return true;

	if (klog_info != NULL) {
		pr_err("already memory alloc for klog_info\n");
		return -EINVAL;
	}

	klog_info = get_debug_reset_header();
	if (klog_info == NULL)
		return -EINVAL;

	klog_read_buf = vmalloc(SEC_DEBUG_RESET_KLOG_SIZE);
	if (!klog_read_buf) {
		pr_err("fail - vmalloc for klog_read_buf\n");
		ret = -ENOMEM;
		goto error_klog_info;
	}
	if (!read_debug_partition(debug_index_reset_klog, klog_read_buf)) {
		pr_err("fail - get param!! summary data\n");
		ret = -ENOENT;
		goto error_klog_read_buf;
	}

	pr_info("magic[0x%x], idx[%u, %u]\n",
		((struct sec_log_buf *)klog_read_buf)->magic,
		((struct sec_log_buf *)klog_read_buf)->idx, klog_info->ap_klog_idx);

	if (((struct sec_log_buf *)klog_read_buf)->magic == SEC_LOG_MAGIC) {
		last_idx = max_t(uint32_t,
			((struct sec_log_buf *)klog_read_buf)->idx, klog_info->ap_klog_idx);
		log_src = klog_read_buf + offsetof(struct sec_log_buf, buf);
	} else {
		last_idx = klog_info->ap_klog_idx;
		log_src = klog_read_buf;
	}

	klog_buf_max_size = SEC_DEBUG_RESET_KLOG_SIZE - offsetof(struct sec_log_buf, buf);
	klog_size = min_t(uint32_t, klog_buf_max_size, last_idx);

	pr_debug("klog_size(0x%x), klog_buf_max_size(0x%x)\n",
		klog_size, klog_buf_max_size);

	klog_buf = vmalloc(klog_size);
	if (!klog_buf) {
		pr_err("fail - vmalloc for klog_buf\n");
		ret = -ENOMEM;
		goto error_klog_read_buf;
	}

	if (klog_size && klog_buf && klog_read_buf) {
		uint32_t idx = last_idx % klog_buf_max_size, len = 0;

		if (last_idx > klog_buf_max_size) {
			len = klog_buf_max_size - idx;
			memcpy(klog_buf, log_src + idx, len);
		}

		memcpy(klog_buf + len, log_src, idx);
	}

	return ret;

error_klog_read_buf:
	vfree(klog_read_buf);
error_klog_info:
	kfree(klog_info);

	return ret;
}

static void sec_reset_klog_completed(void)
{
	set_debug_reset_header(klog_info);

	vfree(klog_buf);
	vfree(klog_read_buf);
	kfree(klog_info);

	klog_info = NULL;
	klog_buf = NULL;
	klog_read_buf = NULL;
	klog_size = 0;

	pr_info("finish\n");
}

static ssize_t sec_reset_klog_proc_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	mutex_lock(&klog_mutex);
	if (sec_reset_klog_init() < 0) {
		mutex_unlock(&klog_mutex);
		return -ENOENT;
	}

	if (pos >= klog_size) {
		pr_info("pos %lld, size %d\n", pos, klog_size);
		sec_reset_klog_completed();
		mutex_unlock(&klog_mutex);
		return 0;
	}

	count = min(len, (size_t)(klog_size - pos));
	if (copy_to_user(buf, klog_buf + pos, count)) {
		mutex_unlock(&klog_mutex);
		return -EFAULT;
	}

	*offset += count;
	mutex_unlock(&klog_mutex);
	return count;
}

static const struct file_operations sec_reset_klog_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_klog_proc_read,
};

static int sec_reset_tzlog_init(void)
{
	int ret = 0;

	if (tzlog_buf != NULL)
		return true;

	if (tzlog_info != NULL) {
		pr_err("already memory alloc for tzlog_info\n");
		return -EINVAL;
	}

	tzlog_info = get_debug_reset_header();
	if (tzlog_info == NULL)
		return -EINVAL;

	if (tzlog_info->stored_tzlog == 0) {
		pr_err("The target didn't run SDI operation\n");
		ret = -EINVAL;
		goto error_tzlog_info;
	}

	tzlog_buf = vmalloc(SEC_DEBUG_RESET_TZLOG_SIZE);
	if (!tzlog_buf) {
		pr_err("fail - vmalloc for tzlog_read_buf\n");
		ret = -ENOMEM;
		goto error_tzlog_info;
	}
	if (!read_debug_partition(debug_index_reset_tzlog, tzlog_buf)) {
		pr_err("fail - get param!! tzlog data\n");
		ret = -ENOENT;
		goto error_tzlog_buf;
	}

	return ret;

error_tzlog_buf:
	vfree(tzlog_buf);
error_tzlog_info:
	kfree(tzlog_info);

	return ret;
}

static void sec_reset_tzlog_completed(void)
{
	set_debug_reset_header(tzlog_info);

	vfree(tzlog_buf);
	kfree(tzlog_info);

	tzlog_info = NULL;
	tzlog_buf = NULL;

	pr_info("finish\n");
}

static ssize_t sec_reset_tzlog_proc_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	mutex_lock(&tzlog_mutex);
	if (sec_reset_tzlog_init() < 0) {
		mutex_unlock(&tzlog_mutex);
		return -ENOENT;
	}

	if (pos >= SEC_DEBUG_RESET_TZLOG_SIZE) {
		pr_info("pos %lld, size %d\n", pos, SEC_DEBUG_RESET_TZLOG_SIZE);
		sec_reset_tzlog_completed();
		mutex_unlock(&tzlog_mutex);
		return 0;
	}

	count = min(len, (size_t)(SEC_DEBUG_RESET_TZLOG_SIZE - pos));
	if (copy_to_user(buf, tzlog_buf + pos, count)) {
		mutex_unlock(&tzlog_mutex);
		return -EFAULT;
	}

	*offset += count;
	mutex_unlock(&tzlog_mutex);
	return count;
}

static const struct file_operations sec_reset_tzlog_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_tzlog_proc_read,
};

static int set_store_lastkmsg_proc_show(struct seq_file *m, void *v)
{
	struct debug_reset_header *check_store = NULL;

	if (check_store != NULL) {
		pr_err("already memory alloc for check_store\n");
		return -EINVAL;
	}

	check_store = get_debug_reset_header();
	if (check_store == NULL)
		seq_puts(m, "0\n");
	else
		seq_puts(m, "1\n");

	if (check_store != NULL) {
		kfree(check_store);
		check_store = NULL;
	}

	return 0;
}

static int sec_store_lastkmsg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_store_lastkmsg_proc_show, NULL);
}

static const struct file_operations sec_store_lastkmsg_proc_fops = {
	.open = sec_store_lastkmsg_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void sec_restore_modem_reset_data(void)
{
	void *p_modem = sec_debug_summary_get_modem();
	struct debug_reset_header *header = get_debug_reset_header();

	if (!header) {
		pr_info("updated nothing.\n");
		return;
	}

	if ((sec_debug_get_reset_reason() != USER_UPLOAD_CAUSE_PANIC)
			&& (sec_debug_get_reset_reason() != USER_UPLOAD_CAUSE_CP_CRASH)) {
		pr_info("it was not kernel panic/cp crash.\n");
		return;
	}

	if (p_modem) {
		read_debug_partition(debug_index_modem_info, p_modem);
		pr_info("complete.\n");
	} else {
		pr_info("skip.\n");
	}
}

void __deprecated sec_debug_summary_modem_print(void)
{
	if ((sec_debug_get_reset_reason() != USER_UPLOAD_CAUSE_PANIC)
			&& (sec_debug_get_reset_reason() != USER_UPLOAD_CAUSE_CP_CRASH)) {
		pr_info("it was not kernel panic/cp crash.\n");
		return;
	}

	pr_info("0x%016lx\n",
		(unsigned long)sec_debug_summary_get_modem());

	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
		sec_debug_summary_get_modem(),
		0x190, 1);
}

static int sec_auto_comment_info_init(void)
{
	int ret = 0;

	if (auto_comment_buf != NULL)
		return true;

	auto_comment_info = get_debug_reset_header();
	if (auto_comment_info == NULL) {
		pr_err("cannot get auto_comment_info of debug_reset_header!\n");
		ret = -EINVAL;
		goto error_auto_comment;
	}

	if (auto_comment_info->auto_comment_size == 0) {
		pr_err("auto_comment_size [%d], just return!\n",
				auto_comment_info->auto_comment_size);
		ret = -ENODATA;
		goto error_auto_comment_info;
	}

	auto_comment_buf = kmalloc(auto_comment_info->auto_comment_size,
					GFP_KERNEL);
	if (!auto_comment_buf) {
		pr_err("fail - kmalloc for auto_comment_buf\n");
		ret = -ENOMEM;
		goto error_auto_comment_info;
	}

	pr_info("auto_comment_size[%d], auto_comment_buf[0x%p]\n",
			auto_comment_info->auto_comment_size, auto_comment_buf);

	if (!read_debug_partition(debug_index_auto_comment, auto_comment_buf)) {
		pr_err("fail - get param!! auto_comment data\n");
		ret = -ENOENT;
		goto error_auto_comment_buf;
	}

	return ret;

error_auto_comment_buf:
	kfree(auto_comment_buf);
	auto_comment_buf = NULL;
error_auto_comment_info:
	kfree(auto_comment_info);
	auto_comment_info = NULL;
error_auto_comment:
	return ret;
}

static int sec_reset_auto_comment_completed(void)
{
	int ret = 0;

	ret = set_debug_reset_header(auto_comment_info);

	kfree(auto_comment_info);
	kfree(auto_comment_buf);

	auto_comment_info = NULL;
	auto_comment_buf = NULL;
	pr_info("finish\n");

	return ret;
}

static ssize_t sec_auto_comment_info_proc_read(struct file *file,
			char __user *buf, size_t len, loff_t *offset)
{
	int ret = 0;
	loff_t pos = *offset;
	ssize_t count;

	pr_info("start!! offset[%zd]\n", (ssize_t)*offset);

	mutex_lock(&auto_comment_mutex);
	ret = sec_auto_comment_info_init();
	if (ret < 0) {
		mutex_unlock(&auto_comment_mutex);
		if (ret == -ENODATA)
			return 0;
		else
			return ret;
	}

	if (pos >= auto_comment_info->auto_comment_size) {
		pr_info("finished: pos %lld, size %d\n",
				pos, auto_comment_info->auto_comment_size);
		sec_reset_auto_comment_completed();
		mutex_unlock(&auto_comment_mutex);
		return 0;
	}

	count = min(len, (size_t)(auto_comment_info->auto_comment_size - pos));
	if (copy_to_user(buf, auto_comment_buf + pos, count)) {
		mutex_unlock(&auto_comment_mutex);
		return -EFAULT;
	}

	*offset += count;
	mutex_unlock(&auto_comment_mutex);
	return count;
}

static const struct file_operations sec_auto_comment_info_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_auto_comment_info_proc_read,
};

static int set_reset_rwc_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d", sec_debug_get_reset_write_cnt());

	return 0;
}

static int sec_reset_rwc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_reset_rwc_proc_show, NULL);
}

static const struct file_operations sec_reset_rwc_proc_fops = {
	.open = sec_reset_rwc_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sec_reset_history_init(void)
{
	int ret = 0, idx = 0, copy_cnt = 0, i = 0, offset = 0;

	if ((reset_history_buf != NULL) && (reset_history_read_buf != NULL))
		return true;

	if (reset_history_info != NULL) {
		pr_err("%s : already memory alloc for reset_history_info\n", __func__);
		return -EINVAL;
	}

	reset_history_info = get_debug_reset_header();
	if (reset_history_info == NULL)
		return -EINVAL;

	if (reset_history_info->reset_history_valid != RESTART_REASON_SEC_DEBUG_MODE
			|| reset_history_info->reset_history_cnt == 0) {
		pr_err("%s : reset_history no data \n", __func__);
		ret = -ENODATA;
		goto error_reset_history_info;
	}

	reset_history_read_buf = vmalloc(SEC_DEBUG_RESET_HISTORY_SIZE);
	if (!reset_history_read_buf) {
		pr_err("%s : fail - vmalloc for reset_history_read_buf\n", __func__);
		ret = -ENOMEM;
		goto error_reset_history_info;
	}
	if (!read_debug_partition(debug_index_reset_history, reset_history_read_buf)) {
		pr_err("%s : fail - get param!! reset_history data\n", __func__);
		ret = -ENOENT;
		goto error_reset_history_buf;
	}

	if (reset_history_info->reset_history_cnt >= SEC_DEBUG_RESET_HISTORY_MAX_CNT) {
		copy_cnt = SEC_DEBUG_RESET_HISTORY_MAX_CNT;
	} else {
		copy_cnt = reset_history_info->reset_history_cnt;
	}

	reset_history_buf = vmalloc(SEC_DEBUG_RESET_HISTORY_SIZE);
	if (!reset_history_buf) {
		pr_err("%s : fail - vmalloc for reset_history_buf\n", __func__);
		ret = -ENOMEM;
		goto error_reset_history_buf;
	}

	for (i = 0; i < copy_cnt; i++) {
		idx = (reset_history_info->reset_history_cnt - 1 - i) % SEC_DEBUG_RESET_HISTORY_MAX_CNT;
		offset += scnprintf((char*)(reset_history_buf + offset), SEC_DEBUG_AUTO_COMMENT_SIZE,
				"%s\n\n\n", &reset_history_read_buf[idx * SEC_DEBUG_AUTO_COMMENT_SIZE]);
	}
	reset_history_size = (offset > SEC_DEBUG_RESET_HISTORY_SIZE) ? SEC_DEBUG_RESET_HISTORY_SIZE : offset;

	return ret;

error_reset_history_buf:
	vfree(reset_history_read_buf);
error_reset_history_info:
	kfree(reset_history_info);

	return ret;
}

static void sec_reset_history_completed(void)
{
	set_debug_reset_header(reset_history_info);

	vfree(reset_history_buf);
	vfree(reset_history_read_buf);
	kfree(reset_history_info);

	reset_history_info = NULL;
	reset_history_buf = NULL;
	reset_history_read_buf = NULL;
	reset_history_size = 0;

	pr_info("%s finish\n", __func__);
}

static ssize_t sec_reset_history_proc_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	int ret = 0;
	ssize_t count;

	mutex_lock(&reset_history_mutex);
	ret = sec_reset_history_init();
	if (ret < 0) {
		mutex_unlock(&reset_history_mutex);
		if (ret == -ENODATA)
			return 0;
		else
			return ret;
	}

	if (pos >= reset_history_size) {
		pr_info("%s : pos %lld, size %d\n", __func__, pos, reset_history_size);
		sec_reset_history_completed();
		mutex_unlock(&reset_history_mutex);
		return 0;
	}

	count = min(len, (size_t)(reset_history_size - pos));
	if (copy_to_user(buf, reset_history_buf + pos, count)) {
		mutex_unlock(&reset_history_mutex);
		return -EFAULT;
	}

	*offset += count;
	mutex_unlock(&reset_history_mutex);
	return count;
}

static const struct file_operations sec_reset_history_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_history_proc_read,
};

static int sec_reset_reason_dbg_part_notifier_callback(
		struct notifier_block *nfb, unsigned long action, void *data)
{
	ap_health_t *p_health;
	uint32_t rr_data;

	switch (action) {
	case DBG_PART_DRV_INIT_DONE:
		p_health = ap_health_data_read();
		if (!p_health)
			return NOTIFY_DONE;

		sec_debug_update_reset_reason(
					p_health->last_rst_reason);
		rr_data = sec_debug_get_reset_reason();
		sec_restore_modem_reset_data();
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block sec_reset_reason_dbg_part_notifier = {
	.notifier_call = sec_reset_reason_dbg_part_notifier_callback,
};

static int __init sec_debug_reset_reason_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("reset_reason", 0444, NULL,
		&sec_reset_reason_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("reset_summary", 0444, NULL,
			&sec_reset_summary_info_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("reset_klog", 0444, NULL,
			&sec_reset_klog_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("reset_tzlog", 0444, NULL,
			&sec_reset_tzlog_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("store_lastkmsg", 0444, NULL,
			&sec_store_lastkmsg_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("auto_comment", 0444, NULL,
			&sec_auto_comment_info_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("reset_rwc", 0444, NULL,
			&sec_reset_rwc_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("reset_history", 0444, NULL,
			&sec_reset_history_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("rdx_bootdev", 0444, NULL,
			&sec_debug_rdx_bootdev_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	dbg_partition_notifier_register(&sec_reset_reason_dbg_part_notifier);

	return 0;
}
device_initcall(sec_debug_reset_reason_init);

static struct device *sec_debug_dev = NULL;

static ssize_t show_FMM_lock(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        int lock=0;
	char str[30];

        sec_get_param(param_index_FMM_lock, &lock);
        snprintf(str,sizeof(str),"FMM lock : [%s]\n", lock?"ON":"OFF");

        return scnprintf(buf, sizeof(str), "%s", str);
}

static ssize_t store_FMM_lock(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
        int lock;

	sscanf(buf, "%d", &lock);
	if(lock)
		lock = FMMLOCK_MAGIC_NUM;
	else
		lock = 0;

	pr_err("FMM lock[%s]\n", lock?"ON":"OFF");
        sec_set_param(param_index_FMM_lock, &lock);

        return count;
}

static DEVICE_ATTR(FMM_lock, 0660, show_FMM_lock, store_FMM_lock);

static int __init sec_debug_FMM_lock_init(void)
{
        int ret;

	if(!sec_debug_dev){
		/* create sec_debug_dev */
		sec_debug_dev = ___sec_device_create(NULL, "sec_debug");
		if (IS_ERR(sec_debug_dev)) {
			pr_err("Failed to create device for sec_debug\n");
			return PTR_ERR(sec_debug_dev);
		}
	}

        ret = sysfs_create_file(&sec_debug_dev->kobj, &dev_attr_FMM_lock.attr);
        if (ret) {
                pr_err("Failed to create sysfs group for sec_debug\n");
                sec_device_destroy(sec_debug_dev->devt);
                sec_debug_dev = NULL;
                return ret;
        }

        return 0;
}
device_initcall(sec_debug_FMM_lock_init);


static ssize_t show_recovery_cause(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char recovery_cause[256];

	sec_get_param(param_index_reboot_recovery_cause, recovery_cause);
	pr_info("%s\n", recovery_cause);

	return scnprintf(buf, PAGE_SIZE, "%s", recovery_cause);
}

static ssize_t store_recovery_cause(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	char recovery_cause[256];

	if (count > sizeof(recovery_cause)) {
		pr_err("input buffer length is out of range.\n");
		return -EINVAL;
	}
	snprintf(recovery_cause, sizeof(recovery_cause), "%s:%d ",
			current->comm, task_pid_nr(current));
	if (strlen(recovery_cause) + strlen(buf) >= sizeof(recovery_cause)) {
		pr_err("input buffer length is out of range.\n");
		return -EINVAL;
	}
	strlcat(recovery_cause, buf, sizeof(recovery_cause));
	sec_set_param(param_index_reboot_recovery_cause, recovery_cause);
	pr_info("%s\n", recovery_cause);

	return count;
}

static DEVICE_ATTR(recovery_cause, 0660,
		show_recovery_cause, store_recovery_cause);

static struct device *sec_debug_dev;

static int __init sec_debug_recovery_reason_init(void)
{
	int ret;

	/* create sysfs for reboot_recovery_cause */
	if (!sec_debug_dev) {
		sec_debug_dev = ___sec_device_create(NULL, "sec_debug");
		if (IS_ERR(sec_debug_dev)) {
			pr_err("Failed to create device for sec_debug\n");
			return PTR_ERR(sec_debug_dev);
		}
	}

	ret = sysfs_create_file(&sec_debug_dev->kobj,
			&dev_attr_recovery_cause.attr);
	if (ret) {
		pr_err("Failed to create sysfs group for sec_debug\n");
		sec_device_destroy(sec_debug_dev->devt);
		sec_debug_dev = NULL;
		return ret;
	}

	return 0;
}
device_initcall(sec_debug_recovery_reason_init);

static void _sec_debug_store_backtrace(unsigned long where)
{
	static int offset;
	unsigned int max_size = 0;
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		max_size = (uintptr_t)&sec_debug_reset_ex_info->rpm_ex_info.info
			- (uintptr_t)p_ex_info->backtrace;

		if (max_size <= offset)
			return;

		if (offset)
			offset += scnprintf(p_ex_info->backtrace + offset,
					 max_size - offset, " > ");

		offset += scnprintf(p_ex_info->backtrace+offset,
					max_size - offset,
					"%pS", (void *)where);
	}
}

void sec_debug_backtrace(void)
{
	static int once;
	struct stackframe frame;
	int skip_callstack = 0;
#if defined (CONFIG_CFP_ROPP) || defined(CONFIG_RKP_CFP_ROPP)
	unsigned long where = 0x0;
#endif

	if (!once++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
		start_backtrace(&frame, (unsigned long)__builtin_frame_address(0), (unsigned long)sec_debug_backtrace);
#else
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.pc = (unsigned long)sec_debug_backtrace;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
		frame.sp = current_stack_pointer;
#endif
#endif
		while (1) {
			int ret;

			ret = unwind_frame(current, &frame);
			if (ret < 0)
				break;

			if (skip_callstack++ > 3) {
#if defined (CONFIG_CFP_ROPP) || defined(CONFIG_RKP_CFP_ROPP)
				where = frame.pc;
				if (where>>40 != 0xffffff)
					where = ropp_enable_backtrace(where,
							current);

				_sec_debug_store_backtrace(where);
#else
				_sec_debug_store_backtrace(frame.pc);
#endif
			}
		}
	}
}
