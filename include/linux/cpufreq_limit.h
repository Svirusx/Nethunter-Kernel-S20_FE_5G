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

struct cpufreq_limit_handle {
	struct list_head node;
	unsigned long freq;
	u64 start_msecs;
	bool requested;
	int id;
	char *name;
};

int cpufreq_limit_get(unsigned long freq, struct cpufreq_limit_handle *handle);
int cpufreq_limit_put(struct cpufreq_limit_handle *handle);
ssize_t cpufreq_limit_get_table(char *buf);
struct cpufreq_limit_handle *cpufreq_limit_get_handle(int id);
int cpufreq_limit_get_cur_min(void);
int cpufreq_limit_get_cur_max(void);
unsigned int cpufreq_limit_get_over_limit(void);
void cpufreq_limit_set_over_limit(unsigned int val);
void cpufreq_limit_corectl(int val);

#endif /* __LINUX_CPUFREQ_LIMIT_H__ */
