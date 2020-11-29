/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *	Minsung Kim <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_CPUFREQ_LIMIT_H__
#define __LINUX_CPUFREQ_LIMIT_H__

struct cpufreq_limit_handle;

struct cpufreq_limit_handle *cpufreq_limit_get(unsigned long min_freq,
		unsigned long max_freq, char *label);
int cpufreq_limit_put(struct cpufreq_limit_handle *handle);
void cpufreq_limit_corectl(int val);

static inline
struct cpufreq_limit_handle *cpufreq_limit_min_freq(unsigned long min_freq,
						    char *label)
{
	return cpufreq_limit_get(min_freq, 0, label);
}

static inline
struct cpufreq_limit_handle *cpufreq_limit_max_freq(unsigned long max_freq,
						    char *label)
{
	return cpufreq_limit_get(0, max_freq, label);
}

ssize_t cpufreq_limit_get_table(char *buf);

#endif /* __LINUX_CPUFREQ_LIMIT_H__ */
