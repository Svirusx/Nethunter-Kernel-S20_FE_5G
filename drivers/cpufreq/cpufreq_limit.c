/*
 * drivers/cpufreq/cpufreq_limit.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *	Minsung Kim <ms925.kim@samsung.com>
 *	Chiwoong Byun <woong.byun@samsung.com>
 *	 - 2014/10/24 Add HMP feature to support HMP
 *	SangYoung Son <hello.son@samsung.com>
 *	 - 2015/08/06 Support MSM8996(same freq table HMP(Gold, Silver))
 *	 - Use opp table for voltage power efficiency
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/cpufreq.h>
#include <linux/cpufreq_limit.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/suspend.h>
#include <linux/cpu.h>

#define MAX_BUF_SIZE	100
#define MIN(a, b)     (((a) < (b)) ? (a) : (b))
#define MAX(a, b)     (((a) > (b)) ? (a) : (b))

/*
 * reduce voltage gap between gold and silver cluster
 * find optimal silver clock from voltage of gold clock
 */
#undef USE_MATCH_CPU_VOL_FREQ
#if defined(USE_MATCH_CPU_VOL_FREQ)
#define USE_MATCH_CPU_VOL_MAX_FREQ	/* max_limit, thermal case */
#undef USE_MATCH_CPU_VOL_MIN_FREQ	/* min_limit, boosting case */
#endif

/* sched boost type from kernel/sched/sched.h */
#define NO_BOOST 0
#define FULL_THROTTLE_BOOST 1
#define CONSERVATIVE_BOOST 2
#define RESTRAINED_BOOST 3

static DEFINE_MUTEX(cpufreq_limit_lock);
static LIST_HEAD(cpufreq_limit_requests);

struct cpufreq_limit_parameter {
	struct cpufreq_frequency_table *cpuftbl_L;
	struct cpufreq_frequency_table *cpuftbl_b;
	unsigned int	unified_cpuftbl[50];
	unsigned int	freq_count;
	bool		table_initialized;

	unsigned int	ltl_cpu_policy;
	unsigned int	ltl_cpu_start;
	unsigned int	ltl_cpu_end;
	unsigned int	mid_cpu_policy;
	unsigned int	big_cpu_policy;
	unsigned int	big_cpu_start;
	unsigned int	big_cpu_end;

	unsigned int	big_min_freq;
	unsigned int	big_max_freq;
	unsigned int	ltl_min_freq;
	unsigned int	ltl_max_freq;

	unsigned int	ltl_min_lock;
	unsigned int	ltl_divider;

	bool			use_hotplug_out;
	unsigned int	hmp_boost_type;
	unsigned int	hmp_boost_active;
	int				hmp_prev_boost_type;

	/* set limit of max freq limit for guarantee performance */
	unsigned int	ltl_limit_max_freq;

	unsigned int	over_limit;
	int				cur_orig_min;
	int				cur_orig_max;
};

/* for SM8250 */
struct cpufreq_limit_parameter param = {
	.freq_count		= 0,
	.table_initialized	= false,

	.ltl_cpu_policy		= 0,
	.ltl_cpu_start		= 0,
	.ltl_cpu_end		= 3,
	.mid_cpu_policy		= 4,
	.big_cpu_policy		= 7,
	.big_cpu_start		= 4,
	.big_cpu_end		= 7,
	.ltl_min_freq		= 0,
	.ltl_max_freq		= 0,
	.big_min_freq		= 0,
	.big_max_freq		= 0,
	.ltl_min_lock		= 1113600 / 2, /* devide value is ltl_divider */

	.ltl_divider		= 2,
	.use_hotplug_out	= false,

	.hmp_boost_type		= CONSERVATIVE_BOOST,
	.hmp_boost_active	= 0,
	.hmp_prev_boost_type = 0,

	.ltl_limit_max_freq	= 1113600,

	.over_limit			= -1,
	.cur_orig_min		= -1,
	.cur_orig_max		= -1,
};

static struct cpufreq_limit_handle
	cpufreq_limit_handle_list[DVFS_MAX_ID] = {
	{
		.name = "none",
		.id = DVFS_NO_ID
	},
	{
		.name = "touch",
		.id = DVFS_TOUCH_ID
	},
	{
		.name = "finger",
		.id = DVFS_FINGER_ID
	},
	{
		.name = "multi-touch",
		.id = DVFS_MULTI_TOUCH_ID
	},
	{
		.name = "argos",
		.id = DVFS_ARGOS_ID
	},
#ifdef CONFIG_USB_AUDIO_ENHANCED_DETECT_TIME
	{
		.name = "boost-host",
		.id = DVFS_BOOST_HOST_ID
	},
#endif
	{
		.name = "user_min",
		.id = DVFS_USER_MIN_ID
	},
	{
		.name = "user_max",
		.id = DVFS_USER_MAX_ID
	},
};

struct cpufreq_limit_handle *cpufreq_limit_get_handle(int id)
{
	return &cpufreq_limit_handle_list[id];
}

void cpufreq_limit_corectl(int freq)
{
	unsigned int cpu;
	bool bigout = false;

	/* if div is 1, do not core control */
	if (param.ltl_divider == 1)
		return;

	if (!param.use_hotplug_out)
		return;

	if ((freq > -1) && (freq < param.big_min_freq))
		bigout = true;

	for_each_possible_cpu(cpu) {
		if ((cpu >= param.big_cpu_start) &&
			(cpu <= param.big_cpu_end)) {

			if (bigout)
				cpu_down(cpu);
			else
				cpu_up(cpu);
		}
	}
}

static bool cpufreq_limit_make_table(void)
{
	int i, count = 0;
	int freq_count = 0;
	unsigned int freq;
	bool ret = false;

	/* big cluster table */
	if (!param.cpuftbl_b)
		goto little;

	for (i = 0; param.cpuftbl_b[i].frequency != CPUFREQ_TABLE_END; i++)
		count = i;

	for (i = count; i >= 0; i--) {
		freq = param.cpuftbl_b[i].frequency;

		if (freq == CPUFREQ_ENTRY_INVALID)
			continue;

		if (freq < param.big_min_freq ||
				freq > param.big_max_freq)
			continue;

		param.unified_cpuftbl[freq_count++] = freq;
	}

	/* if div is 1, use only big cluster freq table */
	if (param.ltl_divider == 1)
		goto done;

little:
	/* LITTLE cluster table */
	if (!param.cpuftbl_L)
		goto done;

	for (i = 0; param.cpuftbl_L[i].frequency != CPUFREQ_TABLE_END; i++)
		count = i;

	for (i = count; i >= 0; i--) {
		freq = param.cpuftbl_L[i].frequency / param.ltl_divider;

		if (freq == CPUFREQ_ENTRY_INVALID)
			continue;

		if (freq < param.ltl_min_freq ||
				freq > param.ltl_max_freq)
			continue;

		param.unified_cpuftbl[freq_count++] = freq;
	}

done:
	if (freq_count) {
		pr_debug("%s: unified table is made\n", __func__);
		param.freq_count = freq_count;
		ret = true;
	} else {
		pr_err("%s: cannot make unified table\n", __func__);
	}

	return ret;
}

/**
 * cpufreq_limit_set_table - cpufreq table from dt via qcom-cpufreq
 */
void cpufreq_limit_set_table(int cpu, struct cpufreq_frequency_table *ftbl)
{
	int i, count = 0;
	unsigned int max_freq_b = 0, min_freq_b = UINT_MAX;
	unsigned int max_freq_l = 0, min_freq_l = UINT_MAX;

	if (param.table_initialized)
		return;

	if (cpu == param.ltl_cpu_policy)
		param.cpuftbl_L = ftbl;
	else if (cpu == param.big_cpu_policy)
		param.cpuftbl_b = ftbl;

	if (!param.cpuftbl_L)
		return;

	if (!param.cpuftbl_b)
		return;

	pr_info("%s: freq table is ready, update config\n", __func__);

	/* update little config */
	for (i = 0; param.cpuftbl_L[i].frequency != CPUFREQ_TABLE_END; i++)
		count = i;

	for (i = count; i >= 0; i--) {
		if (param.cpuftbl_L[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;

		if (param.cpuftbl_L[i].frequency < min_freq_l)
			min_freq_l = param.cpuftbl_L[i].frequency;

		if (param.cpuftbl_L[i].frequency > max_freq_l)
			max_freq_l = param.cpuftbl_L[i].frequency;
	}

	if (!param.ltl_min_freq)
		param.ltl_min_freq = min_freq_l / param.ltl_divider;
	if (!param.ltl_max_freq)
		param.ltl_max_freq = max_freq_l / param.ltl_divider;

	/* update big config */
	for (i = 0; param.cpuftbl_b[i].frequency != CPUFREQ_TABLE_END; i++)
		count = i;

	for (i = count; i >= 0; i--) {
		if (param.cpuftbl_b[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;

		if ((param.cpuftbl_b[i].frequency < min_freq_b) &&
			(param.cpuftbl_b[i].frequency > param.ltl_max_freq))
			min_freq_b = param.cpuftbl_b[i].frequency;

		if (param.cpuftbl_b[i].frequency > max_freq_b)
			max_freq_b = param.cpuftbl_b[i].frequency;
	}

	if (!param.big_min_freq)
		param.big_min_freq = min_freq_b;
	if (!param.big_max_freq)
		param.big_max_freq = max_freq_b;

	pr_info("%s: updated: little(%u-%u), big(%u-%u)\n", __func__,
			param.ltl_min_freq, param.ltl_max_freq,
			param.big_min_freq, param.big_max_freq);

	param.table_initialized = cpufreq_limit_make_table();
}

/**
 * cpufreq_limit_get_table - fill the cpufreq table to support HMP
 * @buf		a buf that has been requested to fill the cpufreq table
 */
ssize_t cpufreq_limit_get_table(char *buf)
{
	ssize_t len = 0;
	int i = 0;

	if (!param.freq_count)
		return len;

	for (i = 0; i < param.freq_count; i++)
		len += snprintf(buf + len, MAX_BUF_SIZE, "%u ",
					param.unified_cpuftbl[i]);

	len--;
	len += snprintf(buf + len, MAX_BUF_SIZE, "\n");

	pr_info("%s: %s\n", __func__, buf);

	return len;
}

static inline int is_little(unsigned int cpu)
{
	return cpu >= param.ltl_cpu_start &&
			cpu <= param.ltl_cpu_end;
}

static inline int is_big(unsigned int cpu)
{
	return cpu >= param.big_cpu_start &&
			cpu <= param.big_cpu_end;
}

static int set_ltl_divider(struct cpufreq_policy *policy, unsigned long *v)
{
	if (is_little(policy->cpu))
		*v /= param.ltl_divider;

	return 0;
}

static int cpufreq_limit_hmp_boost(int enable)
{
	unsigned int ret = 0;

	pr_debug("%s: enable=%d, prev_type=%d, type=%d, active=%d\n",
		__func__, enable,
		param.hmp_prev_boost_type, param.hmp_boost_type,
		param.hmp_boost_active);

	if (enable) {
		if ((param.hmp_boost_type && !param.hmp_boost_active) ||
			(param.hmp_boost_active &&
			(param.hmp_boost_type !=
				param.hmp_prev_boost_type))) {
			param.hmp_boost_active = enable;

			/* release previous boost type
			 * before enable new boost type
			 */
			if (param.hmp_prev_boost_type != NO_BOOST) {
				ret = sched_set_boost(
					param.hmp_prev_boost_type * -1);
				if (ret)
					pr_err("%s: HMP boost enable failed\n",
								__func__);
			}

			/* set prev boost type */
			param.hmp_prev_boost_type =
					param.hmp_boost_type;

			ret = sched_set_boost(param.hmp_boost_type);
			if (ret)
				pr_err("%s: HMP boost enable failed\n",
								__func__);
		}
	} else {
		if (param.hmp_boost_type && param.hmp_boost_active) {
			param.hmp_boost_active = 0;
			ret = sched_set_boost(
				param.hmp_prev_boost_type * -1);
			param.hmp_prev_boost_type = NO_BOOST;
			if (ret)
				pr_err("%s: HMP boost disable failed\n",
								__func__);
		}
	}

	return ret;
}

static int cpufreq_limit_adjust_freq(struct cpufreq_policy *policy,
		unsigned long *min, unsigned long *max)
{
	unsigned int hmp_boost_active = 0;

	pr_debug("%s+: cpu%d, min(%lu), max(%lu)\n", __func__,
					policy->cpu, *min, *max);

	if (is_little(policy->cpu)) {
		if (*min >= param.big_min_freq)
			*min = param.ltl_min_lock * param.ltl_divider;
		else
			*min *= param.ltl_divider;

		if (*max >= param.big_min_freq)
			*max = policy->user_policy.max;
		else
			*max *= param.ltl_divider;
	} else {
		if (*min >= param.big_min_freq) {
			hmp_boost_active = 1;
		} else {
			*min = policy->user_policy.min;
			hmp_boost_active = 0;
		}

		if (*max >= param.big_min_freq) {
			pr_debug("%s: big_min_freq(%u), max(%lu)\n", __func__,
				param.big_min_freq, *max);
		} else {
			*max = policy->user_policy.min;
			hmp_boost_active = 0;
		}

		cpufreq_limit_hmp_boost(hmp_boost_active);
	}

	pr_debug("%s-: cpu%d, min(%lu), max(%lu)\n", __func__,
					policy->cpu, *min, *max);

	return 0;
}

int cpufreq_limit_get(unsigned long freq,
						struct cpufreq_limit_handle *handle)
{
	ktime_t curr_time;
	u64 msecs64;

	if (!param.table_initialized)
		return -EAGAIN;

	mutex_lock(&cpufreq_limit_lock);

	handle->freq = freq;
	curr_time = ktime_get();
	msecs64 = ktime_to_ns(curr_time);
	do_div(msecs64, NSEC_PER_MSEC);
	handle->start_msecs = msecs64;
	if (!handle->requested) {
		list_add_tail(&handle->node, &cpufreq_limit_requests);
		handle->requested = true;
	}

	pr_debug("%s: %s, %lu\n", __func__, handle->name, handle->freq);

	mutex_unlock(&cpufreq_limit_lock);

	cpufreq_update_policy(param.ltl_cpu_policy);
	cpufreq_update_policy(param.mid_cpu_policy);
	cpufreq_update_policy(param.big_cpu_policy);

	return 0;
}

int cpufreq_limit_put(struct cpufreq_limit_handle *handle)
{
	pr_debug("%s: %s, %lu\n", __func__, handle->name, handle->freq);

	mutex_lock(&cpufreq_limit_lock);
	if (handle->requested) {
		list_del(&handle->node);
		handle->requested = false;
	}
	mutex_unlock(&cpufreq_limit_lock);

	cpufreq_update_policy(param.ltl_cpu_policy);
	cpufreq_update_policy(param.mid_cpu_policy);
	cpufreq_update_policy(param.big_cpu_policy);

	return 0;
}

int cpufreq_limit_get_cur_min(void)
{
	return param.cur_orig_min;
}

int cpufreq_limit_get_cur_max(void)
{
	return param.cur_orig_max;
}

void cpufreq_limit_set_over_limit(unsigned int val)
{
	param.over_limit = val;

	cpufreq_update_policy(param.ltl_cpu_policy);
	cpufreq_update_policy(param.mid_cpu_policy);
	cpufreq_update_policy(param.big_cpu_policy);
}

unsigned int cpufreq_limit_get_over_limit(void)
{
	return param.over_limit;
}

static int cpufreq_limit_notifier_policy(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct cpufreq_policy *policy = data;
	struct cpufreq_limit_handle *handle;
	unsigned long min = 0, max = ULONG_MAX;
	int min_id = DVFS_NO_ID;
#if defined(USE_MATCH_CPU_VOL_MIN_FREQ)
	unsigned long adjust_min = 0;
#endif
#if defined(USE_MATCH_CPU_VOL_MAX_FREQ)
	unsigned long adjust_max = 0;
#endif

	if (val != CPUFREQ_ADJUST)
		goto done;

	mutex_lock(&cpufreq_limit_lock);

	list_for_each_entry(handle, &cpufreq_limit_requests, node) {
		if (handle->id == DVFS_USER_MAX_ID && handle->freq < max)
			max = handle->freq;
		else if (handle->freq > min) {
			min = handle->freq;
			min_id = handle->id;
		}
	}

	pr_debug("++ CPUFREQ(%d): %s: user(%d ~ %d), pol(%d ~ %d), set(%lu ~ %lu), over(%d)\n",
		policy->cpu, __func__,
		policy->user_policy.min, policy->user_policy.max, policy->min, policy->max, min, max, param.over_limit);

	mutex_unlock(&cpufreq_limit_lock);

#if defined(USE_MATCH_CPU_VOL_FREQ)
	if (is_little(policy->cpu)) {
#if defined(USE_MATCH_CPU_VOL_MIN_FREQ)
		/* boost case */
		if (min)
			adjust_min = msm_match_cpu_voltage_btol(min);

		if (adjust_min)
			min = adjust_min;
#endif
#if defined(USE_MATCH_CPU_VOL_MAX_FREQ)
		/* thermal case */
		if (max && (max < param.ltl_max_freq) &&
			(max >= param.ltl_limit_max_freq)) {
			adjust_max = msm_match_cpu_voltage_btol(max);

			if (adjust_max)
				max = MAX(adjust_max,
					param.ltl_limit_max_freq);
		}
#endif
	}
#endif

	param.cur_orig_min = min;
	param.cur_orig_max = max;

	if (!min && max == ULONG_MAX) {
		cpufreq_limit_hmp_boost(0);
		param.cur_orig_min = -1;
		param.cur_orig_max = -1;
		goto done;
	}

	if (!min) {
		min = policy->user_policy.min;
		set_ltl_divider(policy, &min);
		param.cur_orig_min = -1;
	}
	if (max == ULONG_MAX) {
		max = policy->user_policy.max;
		set_ltl_divider(policy, &max);
		param.cur_orig_max = -1;
	}

	/*
	 * ltl_divider 1 means,
	 * little and big has similar power and performance case
	 * e.g. MSM8996 silver and gold
	 */
	if (param.ltl_divider == 1) {
		if (is_big(policy->cpu)) {
			/* sched_boost scenario */
			if (min > param.ltl_max_freq)
				cpufreq_limit_hmp_boost(1);
			else
				cpufreq_limit_hmp_boost(0);
		}
	} else {
		cpufreq_limit_adjust_freq(policy, &min, &max);
	}

	pr_debug("%s: limiting cpu%d cpufreq to %lu-%lu\n", __func__,
			policy->cpu, min, max);

	/* for over limit */
	if (param.over_limit != -1 && param.cur_orig_max != -1
		&& (int)param.over_limit > param.cur_orig_max
		&& (param.cur_orig_min >= param.big_min_freq)
		&& (min_id == DVFS_USER_MIN_ID || min_id == DVFS_TOUCH_ID)) {
		max = param.over_limit;

		pr_debug("%s: min over max: cpu%d cpufreq to %lu-%lu\n", __func__,
				policy->cpu, min, max);
	}

	cpufreq_verify_within_limits(policy, min, max);

	pr_debug("%s: limited cpu%d cpufreq to %u-%u\n", __func__,
			policy->cpu, policy->min, policy->max);
done:
	pr_debug("-- CPUFREQ(%d): %s: user(%d ~ %d), pol(%d ~ %d), set(%lu ~ %lu), over(%d)\n",
		policy->cpu, __func__,
		policy->user_policy.min, policy->user_policy.max, policy->min, policy->max, min, max, param.over_limit);
	return 0;
}

static struct notifier_block notifier_policy_block = {
	.notifier_call = cpufreq_limit_notifier_policy,
	.priority = -1,
};

/************************** sysfs begin ************************/
static ssize_t show_cpufreq_limit_requests(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct cpufreq_limit_handle *handle;
	ssize_t len = 0;
	ktime_t curr_time;
	u64 msecs64, sincesecs64;

	curr_time = ktime_get();
	msecs64 = ktime_to_ns(curr_time);
	do_div(msecs64, NSEC_PER_MSEC);

	mutex_lock(&cpufreq_limit_lock);
	len += snprintf(buf + len, MAX_BUF_SIZE,
			"%s\t\t%s\t\t%s\n",
			"name", "freq", "since");

	list_for_each_entry(handle, &cpufreq_limit_requests, node) {
		sincesecs64 = msecs64 - handle->start_msecs;

		len += snprintf(buf + len, MAX_BUF_SIZE,
				"%s\t\t%lu\t\t%llu\n", handle->name,
				handle->freq, sincesecs64);
	}
	mutex_unlock(&cpufreq_limit_lock);

	return len;
}

static struct kobj_attribute cpufreq_limit_requests_attr =
	__ATTR(requests, 0444, show_cpufreq_limit_requests, NULL);

#define MAX_ATTRIBUTE_NUM 12

#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)		\
{									\
	return snprintf(buf, MAX_BUF_SIZE, "%u\n", param.object);	\
}

#define store_one(file_name, object)					\
static ssize_t store_##file_name					\
(struct kobject *a, struct kobj_attribute *b, const char *buf, size_t count)\
{									\
	int ret;							\
									\
	ret = kstrtouint(buf, 0, &param.object);			\
	if (ret != 1)							\
		return -EINVAL;						\
									\
	return count;							\
}

static ssize_t show_ltl_cpu_num(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_SIZE, "%u-%u\n",
		param.ltl_cpu_start, param.ltl_cpu_end);
}

static ssize_t show_big_cpu_num(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_SIZE, "%u-%u\n",
		param.big_cpu_start, param.big_cpu_end);
}

show_one(big_min_freq, big_min_freq);
show_one(big_max_freq, big_max_freq);
show_one(ltl_min_freq, ltl_min_freq);
show_one(ltl_max_freq, ltl_max_freq);
show_one(ltl_min_lock, ltl_min_lock);
show_one(ltl_divider, ltl_divider);
show_one(hmp_boost_type, hmp_boost_type);
show_one(hmp_prev_boost_type, hmp_prev_boost_type);

static ssize_t store_ltl_cpu_num(struct kobject *a, struct kobj_attribute *b,
				const char *buf, size_t count)
{
	unsigned int input, input2;
	int ret;

	ret = sscanf(buf, "%u-%u", &input, &input2);
	if (ret != 2)
		return -EINVAL;

	if (input >= MAX_ATTRIBUTE_NUM || input2 >= MAX_ATTRIBUTE_NUM)
		return -EINVAL;

	pr_info("%s: %u-%u, ret=%d\n", __func__, input, input2, ret);

	param.ltl_cpu_start = input;
	param.ltl_cpu_end = input2;

	return count;
}

static ssize_t store_big_cpu_num(struct kobject *a, struct kobj_attribute *b,
				const char *buf, size_t count)
{
	unsigned int input, input2;
	int ret;

	ret = sscanf(buf, "%u-%u", &input, &input2);
	if (ret != 2)
		return -EINVAL;

	if (input >= MAX_ATTRIBUTE_NUM || input2 >= MAX_ATTRIBUTE_NUM)
		return -EINVAL;

	pr_info("%s: %u-%u, ret=%d\n", __func__, input, input2, ret);

	param.big_cpu_start = input;
	param.big_cpu_end = input2;

	return count;
}

store_one(big_min_freq, big_min_freq);
store_one(big_max_freq, big_max_freq);
store_one(ltl_min_freq, ltl_min_freq);
store_one(ltl_max_freq, ltl_max_freq);
store_one(ltl_min_lock, ltl_min_lock);

static ssize_t store_ltl_divider(struct kobject *a, struct kobj_attribute *b,
				const char *buf, size_t count)
{
	unsigned int input = 0;
	int ret;

	ret = kstrtouint(buf, 0, &input);
	if (ret != 1)
		return -EINVAL;

	if (input >= MAX_ATTRIBUTE_NUM)
		return -EINVAL;

	param.ltl_divider = input;

	return count;
}

static ssize_t store_hmp_boost_type(struct kobject *a, struct kobj_attribute *b,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = kstrtouint(buf, 0, &input);
	param.hmp_boost_type = input;

	return count;
}

define_one_global_rw(ltl_cpu_num);
define_one_global_rw(big_cpu_num);
define_one_global_rw(big_min_freq);
define_one_global_rw(big_max_freq);
define_one_global_rw(ltl_min_freq);
define_one_global_rw(ltl_max_freq);
define_one_global_rw(ltl_min_lock);
define_one_global_rw(ltl_divider);
define_one_global_rw(hmp_boost_type);
define_one_global_ro(hmp_prev_boost_type);

static struct attribute *limit_attributes[] = {
	&cpufreq_limit_requests_attr.attr,
	&ltl_cpu_num.attr,
	&big_cpu_num.attr,
	&big_min_freq.attr,
	&big_max_freq.attr,
	&ltl_min_freq.attr,
	&ltl_max_freq.attr,
	&ltl_min_lock.attr,
	&ltl_divider.attr,
	&hmp_boost_type.attr,
	&hmp_prev_boost_type.attr,
	NULL,
};

static struct attribute_group limit_attr_group = {
	.attrs = limit_attributes,
	.name = "cpufreq_limit",
};
/************************** sysfs end ************************/

static int __init cpufreq_limit_init(void)
{
	int ret;

	ret = cpufreq_register_notifier(&notifier_policy_block,
				CPUFREQ_POLICY_NOTIFIER);
	if (ret)
		return ret;

	if (cpufreq_global_kobject) {
		pr_debug("%s: sysfs_create_group\n", __func__);
		ret = sysfs_create_group(cpufreq_global_kobject,
				&limit_attr_group);
		if (ret)
			pr_err("%s: failed\n", __func__);
	}

	return ret;
}

static void __exit cpufreq_limit_exit(void)
{
	cpufreq_unregister_notifier(&notifier_policy_block,
			CPUFREQ_POLICY_NOTIFIER);

	sysfs_remove_group(cpufreq_global_kobject, &limit_attr_group);
}

MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("'cpufreq_limit' - A driver to limit cpu frequency");
MODULE_LICENSE("GPL");

module_init(cpufreq_limit_init);
module_exit(cpufreq_limit_exit);
