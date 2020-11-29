// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_init_log.c
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

#include <linux/console.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include <linux/sec_debug.h>

#define INIT_LOG_BUF_SZ_SHIFT		17

static size_t buf_indx;
static size_t buf_mask;
static char *init_log_buffer;

static const char *_psinfo_magic = "]  [";
static uint32_t psinfo_magic __read_mostly;

/* 0         1         2
 * 012345678901234567890123456789
 *          0         1         2
 *          012345678901234567890123456789
 * [6:           init:    1]
 */

#define PSINFO_MAGIC_0_OFFSET		9
#define PSINFO_MAGIC_1_OFFSET		17

static const char *_pid_magic = "     init:    1]";
static uint64_t pid_magic[2] __read_mostly;

/* 0         1         2         3         4
 * 01234567890123456789012345678901234567890123456789
 * [   10.491222]  [6:           init:    1] init: Service 'qcom-c_main-sh' (pid 827) exited with status 0
 * [   10.491386]  [6:           init:    1] init: Service 'vendor.dataadpl' (pid 1123) received signal 9
 * [   10.495145]  [5:    kworker/5:1:  307] bolero-codec soc:qcom,msm-audio-apr:qcom,q6core-audio:bolero-cdc: ASoC: unknown pin WSA AIF VI
 * [   10.495541]  [5:    kworker/5:1:  307] tfa9xxx 7-0034: tfa9xxx_set_fmt: fmt=0x4001
 */

#define PSINFO_MAGIC_RANGE_BEGIN	13
#define PSINFO_MAGIC_RANGE_END		20

static bool __is_log_from_init(const char *str)
{
	size_t i;
	size_t psinfo_start = 0;

	for (i = PSINFO_MAGIC_RANGE_BEGIN; i <= PSINFO_MAGIC_RANGE_END; i++) {
		if (psinfo_magic == *(uint32_t *)(&str[i])) {
			psinfo_start = i + 3;
			break;
		}
	}

	if (!psinfo_start)
		return false;

	if (pid_magic[0] == *(uint64_t *)(&str[psinfo_start + PSINFO_MAGIC_0_OFFSET]) &&
	    pid_magic[1] == *(uint64_t *)(&str[psinfo_start + PSINFO_MAGIC_1_OFFSET]))
		return true;

	return false;
}

static __always_inline void __sec_init_log_buf_write(const char *s,
		unsigned int count)
{
	unsigned int f_len, s_len, remain_space;
	size_t buf_pos = buf_indx & buf_mask;

	if (unlikely(!init_log_buffer))
		return;

	remain_space = buf_mask + 1 - buf_pos;
	f_len = min(count, remain_space);
	memcpy(init_log_buffer + buf_pos, s, f_len);

	s_len = count - f_len;

	if (unlikely(s_len))
		memcpy(init_log_buffer, s + f_len, s_len);

	buf_indx += count;
}

static void sec_init_log_buf_write_console(struct console *console, const char *s,
		unsigned int count)
{
	if (likely(!__is_log_from_init(s)))
		return;

	__sec_init_log_buf_write(s, count);
}

#ifdef CONFIG_SEC_LOG_BUF_NO_CONSOLE
void sec_init_log_buf_write(const char *s, unsigned int count)
{
	if (likely(!sec_debug_is_enabled()))
		return;

	__sec_init_log_buf_write(s, count);
}
#endif

static struct console sec_init_log_console = {
	.name	= "sec_init_log",
	.write	= sec_init_log_buf_write_console,
	.flags	= CON_ENABLED | CON_ANYTIME,
	.index	= -1,
};

static inline void __sec_init_log_buf_init_local_constant(void)
{
	psinfo_magic = *(uint32_t *)_psinfo_magic;

	pid_magic[0] = *(uint64_t *)(&_pid_magic[0]);
	pid_magic[1] = *(uint64_t *)(&_pid_magic[8]);
}

static int __init sec_init_log_buf_init(void)
{
	size_t size;

	if (likely(!sec_debug_is_enabled()))
		return 0;

	size = 1 << INIT_LOG_BUF_SZ_SHIFT;

	init_log_buffer = vmalloc(size);
	if (unlikely(!init_log_buffer)) {
		pr_warn("failed to alloc memory!\n");
		return -ENOMEM;
	}

	buf_mask = size - 0x1;

	pr_info("%zu@%p - (mask:0x%zx)\n", size, init_log_buffer, buf_mask);

	__sec_init_log_buf_init_local_constant();

	if (IS_ENABLED(CONFIG_SEC_LOG_BUF_CONSOLE))
		register_console(&sec_init_log_console);

	return 0;
}
device_initcall(sec_init_log_buf_init);
