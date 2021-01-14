// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_log_buf.c
 *
 * COPYRIGHT(C) 2010-2019 Samsung Electronics Co., Ltd. All Right Reserved.
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

#include <linux/console.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/rtc.h>

#include "sec_debug_internal.h"

/*
 * Example usage: sec_log=256K@0x45000000
 *
 * In above case, log_buf size is 256KB and its physical base address
 * is 0x45000000. Actually, *(int *)(base - 8) is log_magic and *(int
 * *)(base - 4) is log_ptr. Therefore we reserve (size + 8) bytes from
 * (base - 8)
 */

static struct sec_log_buf __iomem *s_log_buf;

static phys_addr_t sec_log_paddr;
static size_t sec_log_size;
static size_t sec_log_buf_size __read_mostly;
static bool sec_log_buf_init_done __read_mostly;

unsigned int get_sec_log_idx(void)
{
	return (unsigned int)s_log_buf->idx;
}

#if IS_ENABLED(CONFIG_SEC_LOG_STORE_LPM_KMSG)
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
#endif

#ifdef CONFIG_SEC_LOG_LAST_KMSG
static char *last_log_buf;
static size_t last_log_buf_size;

static int __init sec_log_setup(char *str)
{
	unsigned long long base;

	sec_log_size = (size_t)memparse(str, &str);

	if (!sec_log_size ||
	    (sec_log_size != roundup_pow_of_two(sec_log_size)) ||
	    (*str != '@'))
		goto __err;

	if (kstrtoull(++str, 16, &base))
		goto __err;

	sec_log_paddr = base;
	sec_log_buf_size = sec_log_size - offsetof(struct sec_log_buf, buf);

	return 0;

__err:
	sec_log_paddr = 0;
	sec_log_size = 0;
	sec_log_buf_size = 0;

	return 0;
}
early_param("sec_log", sec_log_setup);

static int __init __sec_last_kmsg_prepare(void)
{
	unsigned int max_size = sec_log_buf_size;
	unsigned int head;

	last_log_buf = vmalloc(sec_log_buf_size);
	if (!last_log_buf)
		return -ENOMEM;

	if (s_log_buf->idx > max_size) {
		head = s_log_buf->idx % sec_log_buf_size;
		memcpy_fromio(last_log_buf, &(s_log_buf->buf[head]),
				sec_log_buf_size - head);
		if (head != 0)
			memcpy_fromio(&(last_log_buf[sec_log_buf_size - head]),
			       s_log_buf->buf, head);
		last_log_buf_size = max_size;
	} else {
		memcpy_fromio(last_log_buf, s_log_buf->buf, s_log_buf->idx);
		last_log_buf_size = s_log_buf->idx;
	}

	return 0;
}

static ssize_t sec_last_log_buf_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= last_log_buf_size || !last_log_buf) {
		pr_info("pos %lld, size %zu\n", pos, last_log_buf_size);
		return 0;
	}

	count = min(len, (size_t) (last_log_buf_size - pos));
	if (copy_to_user(buf, last_log_buf + pos, count))
		return -EFAULT;

	*offset += count;

	return count;
}

static const struct file_operations last_log_buf_fops = {
	.owner	= THIS_MODULE,
	.read	= sec_last_log_buf_read,
};

#define LAST_LOG_BUF_NODE		"last_kmsg"

static inline int __sec_last_kmsg_init(void)
{
	struct proc_dir_entry *entry;
	int err;

	err = __sec_last_kmsg_prepare();
	if (err)
		return err;

	entry = proc_create_data(LAST_LOG_BUF_NODE, 0444, NULL,
			&last_log_buf_fops, NULL);
	if (!entry) {
		pr_warn("failed to create proc entry. ram console may be present\n");
		return 0;
	}

	proc_set_size(entry, last_log_buf_size);

	return 0;
}
#else /* CONFIG_SEC_LOG_LAST_KMSG */
static inline int __sec_last_kmsg_init(void)
{}
#endif


#if IS_ENABLED(CONFIG_SEC_LOG_STORE_LPM_KMSG)
static int sec_log_store_lpm_kmsg(
		struct notifier_block *nb, unsigned long action, void *data)
{
	switch (action) {
		case DBG_PART_DRV_INIT_DONE:
			if (lpm_mode && sec_log_buf_init_done) {
				pr_info("booting lpm mode, store klog\n");
				write_debug_partition(debug_index_reset_lpm_klog, s_log_buf);
			}
			break;
		default:
			return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block sec_log_store_lpm_kmsg_notifier = {
	.notifier_call = sec_log_store_lpm_kmsg,
};
#endif

#ifdef CONFIG_SEC_LOG_STORE_LAST_KMSG
static int sec_log_store(struct notifier_block *nb,
		unsigned long action, void *data)
{
	char cmd[256] = { 0, };
	struct rtc_time tm;
	struct rtc_device *rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);

	if (!rtc) {
		pr_info("unable to open rtc device (%s)\n", CONFIG_RTC_HCTOSYS_DEVICE);
	} else {
		if (rtc_read_time(rtc, &tm)) 
			dev_err(rtc->dev.parent, "hctosys: unable to read the hardware clock\n");
		else
			pr_info("RTC(%lld)\n", rtc_tm_to_time64(&tm));
	}

	if (!sec_log_buf_init_done)
		return 0;

	if (data && strlen(data))
		snprintf(cmd, sizeof(cmd), "%s", (char *)data);

	switch (action) {
	case SYS_RESTART:
	case SYS_POWER_OFF:
		pr_info("%s, %s\n",
				action == SYS_RESTART ? "reboot" : "power off", cmd);
		write_debug_partition(debug_index_reset_klog, s_log_buf);
		break;
	}

	return 0;
}

static struct notifier_block sec_log_notifier = {
	.notifier_call = sec_log_store,
	.priority = -2,
};
#endif

static __always_inline void __sec_log_buf_write(const char *s, unsigned int count)
{
	unsigned int f_len, s_len, remain_space;
	size_t idx;

	if (unlikely(!sec_log_buf_init_done))
		return;

	idx = s_log_buf->idx % sec_log_buf_size;
	remain_space = sec_log_buf_size - idx;
	f_len = min(count, remain_space);
	memcpy_toio(&(s_log_buf->buf[idx]), s, f_len);

	s_len = count - f_len;
	if (unlikely(s_len))
		memcpy_toio(s_log_buf->buf, &s[f_len], s_len);

	s_log_buf->idx += (uint32_t)count;
}

static void sec_log_buf_write_console(struct console *console, const char *s,
		unsigned int count)
{
	__sec_log_buf_write(s, count);
}

#ifdef CONFIG_SEC_LOG_BUF_NO_CONSOLE
void sec_log_buf_write(const char *s, unsigned int count)
{
	__sec_log_buf_write(s, count);
}
#endif

static struct console sec_log_console = {
	.name	= "sec_log_buf",
	.write	= sec_log_buf_write_console,
	.flags	= CON_PRINTBUFFER | CON_ENABLED | CON_ANYTIME,
	.index	= -1,
};

static inline int __sec_log_buf_prepare(void)
{
	pr_info("printk virtual addrs phy=%pa\n", &sec_log_paddr);

	if (sec_debug_is_enabled())
		s_log_buf = ioremap_wc(sec_log_paddr, sec_log_size);
	else
		s_log_buf = ioremap_cache(sec_log_paddr, sec_log_size);

	if (!s_log_buf) {
		pr_err("failed to ioremap log region\n");
		return -ENOMEM;
	}

	pr_info("buf base virtual addrs 0x%p phy=%pa\n",
			s_log_buf, &sec_log_paddr);

	if (s_log_buf->magic != SEC_LOG_MAGIC) {
		pr_warn("sec_log_magic is not valid : 0x%x at 0x%p\n",
				s_log_buf->magic, &(s_log_buf->magic));
		s_log_buf->magic = SEC_LOG_MAGIC;
		s_log_buf->idx = 0;
		s_log_buf->prev_idx = 0;
	}

	return 0;
}

static int __init sec_log_buf_init(void)
{
	int err;

	err = __sec_log_buf_prepare();
	if (err)
		return err;

#ifdef CONFIG_SEC_LOG_STORE_LAST_KMSG
	register_reboot_notifier(&sec_log_notifier);
#endif

#if IS_ENABLED(CONFIG_SEC_LOG_STORE_LPM_KMSG)
	dbg_partition_notifier_register(&sec_log_store_lpm_kmsg_notifier);
#endif

	err = __sec_last_kmsg_init();
	if (err)
		pr_warn("failed to create %s\n", LAST_LOG_BUF_NODE);

	if (IS_ENABLED(CONFIG_SEC_LOG_BUF_CONSOLE)) {
		sec_log_buf_init_done = true;
		register_console(&sec_log_console);
	} else
		sec_log_buf_pull_early_buffer(&sec_log_buf_init_done);

	return 0;
}
core_initcall_sync(sec_log_buf_init);
