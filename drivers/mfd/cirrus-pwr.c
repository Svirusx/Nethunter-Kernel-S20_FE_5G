/*
 * Power-management support for Cirrus Logic CS35L41 amplifier
 *
 * Copyright 2018 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/ktime.h>

#include <linux/mfd/cs35l41/core.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/power.h>

#define CIRRUS_PWR_VERSION "5.01.18"

#define CIRRUS_PWR_CLASS_NAME		"cirrus"
#define CIRRUS_PWR_DIR_NAME		"cirrus_pwr"
#define CIRRUS_PWR_WORKQ_NAME		"cirrus_pwr_wq"

#define CIRRUS_PWR_STATUS_DISABLED	0
#define	CIRRUS_PWR_STATUS_ENABLED	1
#define CIRRUS_PWR_STATUS_ERROR		3

#define CIRRUS_PWR_AMB_TEMP_OFFSET	500
#define CIRRUS_PWR_SCALING_Q15		846397

struct cirrus_pwr_t {
	struct class *pwr_class;
	struct device *dev;
	struct cirrus_mfd_amp *amps;
	int num_amps;
	struct mutex pwr_lock;
	struct delayed_work pwr_work;
	struct workqueue_struct *pwr_workqueue;
	unsigned int uptime_ms;
	unsigned int interval;
	unsigned int status;
	unsigned int target_min_time_ms;
	unsigned int target_temp[CIRRUS_MAX_AMPS];
	unsigned int exit_temp[CIRRUS_MAX_AMPS];
	unsigned int amb_temp[CIRRUS_MAX_AMPS];
	unsigned int spk_temp[CIRRUS_MAX_AMPS];
	unsigned int passport_enable[CIRRUS_MAX_AMPS];
	unsigned int global_enable;
	bool amp_active[CIRRUS_MAX_AMPS];
};

static struct cirrus_pwr_t *cirrus_pwr;
static struct attribute_group cirrus_pwr_attr_grp;

struct cirrus_mfd_amp *cirrus_pwr_get_amp_from_suffix(const char *suffix)
{
	int i;
	struct cirrus_mfd_amp *ret = NULL;

	if (cirrus_pwr == NULL || cirrus_pwr->amps == NULL)
		return NULL;

	dev_dbg(cirrus_pwr->dev, "%s: suffix = %s\n", __func__, suffix);

	for (i = 0; i < cirrus_pwr->num_amps; i++) {
		dev_dbg(cirrus_pwr->dev, "comparing %s & %s\n",
				cirrus_pwr->amps[i].mfd_suffix,
				suffix);
		if (strcmp(cirrus_pwr->amps[i].mfd_suffix, suffix) == 0)
			ret = &cirrus_pwr->amps[i];
	}

	return ret;
}

static unsigned int sqrt_q24(unsigned long int x)
{
	u32 root, remHi, remLo, testDiv, count;

	root = 0;
	remHi = 0;
	remLo = x;
	count = 24;

	do {
		remHi = (remHi << 2) | (remLo >> 30);
		remLo <<= 2;
		root <<= 1;
		testDiv = (root << 1) + 1;
		if (remHi >= testDiv) {
			remHi -= testDiv;
			root++;
		}
	} while (count-- != 0);

	return root; /* Q21 result */
}

static unsigned int convert_power(unsigned int power_squared)
{
	unsigned long long int power;

	power = sqrt_q24(power_squared*2);
	power *= CIRRUS_PWR_SCALING_Q15;

	dev_dbg(cirrus_pwr->dev,
			"converted power (%d W^2): %llu.%04llu W\n",
			power_squared,
			power >> 36,
			(power & (((1ull << 36) - 1ull))) *
			    10000 / (1ull << 36));

	power *= 1000;
	power >>= 28;

	dev_dbg(cirrus_pwr->dev,
		"converted power q8 mW: %d mW = 0x%x\n",
		(unsigned int)(power / 256), (unsigned int)(power));

	return (unsigned int)power;
}

int cirrus_pwr_amp_add(struct regmap *regmap_new, const char *mfd_suffix,
					const char *dsp_part_name)
{
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(mfd_suffix);

	if (cirrus_pwr){
		if (amp) {
			dev_info(cirrus_pwr->dev,
				"Amp added, suffix: %s dsp_part_name: %s\n",
				mfd_suffix, dsp_part_name);
			amp->regmap = regmap_new;
			amp->dsp_part_name = dsp_part_name;
		} else {
			dev_err(cirrus_pwr->dev,
				"No amp with suffix %s registered\n",
				mfd_suffix);
		}
	} else {
		return -EINVAL;
	}

	return 0;
}


int cirrus_pwr_set_params(bool global_enable, const char *mfd_suffix,
			unsigned int target_temp, unsigned int exit_temp)
{
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(mfd_suffix);

	if (!amp)
		return 0;

	cirrus_pwr->global_enable = global_enable;

	cirrus_pwr->target_temp[amp->index] = target_temp;
	cirrus_pwr->exit_temp[amp->index] = exit_temp;


	dev_info(cirrus_pwr->dev,
	"%s: global enable = %d, cs35l41%s, target temp = %d, exit temp = %d\n",
		__func__, global_enable,
		mfd_suffix,
		target_temp, exit_temp);

	return 0;
}

static void cirrus_pwr_passport_enable(struct regmap *regmap_enable,
							bool enable)
{
	if (regmap_enable)
		regmap_write(regmap_enable,
			CIRRUS_PWR_CSPL_PASSPORT_ENABLE,
			(uint)enable);
}

void cirrus_pwr_start(const char *mfd_suffix)
{
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(mfd_suffix);

	if (!cirrus_pwr)
		return;

	if (!amp)
		return;

	cirrus_pwr->amp_active[amp->index] = 1;

	if (!cirrus_pwr->global_enable)
		return;

	mutex_lock(&cirrus_pwr->pwr_lock);

	if (cirrus_pwr->status == CIRRUS_PWR_STATUS_ENABLED) {
		/* State machine already active on one amp */
		dev_dbg(cirrus_pwr->dev,
			"cirrus_pwr_start(), additional amp activated");
	} else {
		/* Init state machine */
		dev_dbg(cirrus_pwr->dev,
			"cirrus_pwr_start() Entering wait period.\n");
		cirrus_pwr->status = CIRRUS_PWR_STATUS_ENABLED;

		/* Queue state machine operation */
		queue_delayed_work(cirrus_pwr->pwr_workqueue,
				   &cirrus_pwr->pwr_work,
				   msecs_to_jiffies(cirrus_pwr->interval));
	}

	mutex_unlock(&cirrus_pwr->pwr_lock);
}
EXPORT_SYMBOL_GPL(cirrus_pwr_start);

void cirrus_pwr_stop(const char *mfd_suffix)
{
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(mfd_suffix);
	int i;
	bool amps_active = 0;

	if (!cirrus_pwr)
		return;

	if (!amp)
		return;

	cirrus_pwr->amp_active[amp->index] = 0;

	if (!cirrus_pwr->global_enable)
		return;

	mutex_lock(&cirrus_pwr->pwr_lock);

	for (i = 0; i < cirrus_pwr->num_amps; i++)
		amps_active |= cirrus_pwr->amp_active[i];

	if (amps_active) {
		/* One amp still active */
		dev_dbg(cirrus_pwr->dev,
			"Amp cs35l41%s deactivated\n", mfd_suffix);
	} else {
		/* Exit state machine */
		dev_dbg(cirrus_pwr->dev,
			"cirrus_pwr_stop(). Disabling PASSPORT\n");

		for (i = 0; i < cirrus_pwr->num_amps; i++) {
			cirrus_pwr_passport_enable(
				cirrus_pwr->amps[i].regmap, false);
			cirrus_pwr->passport_enable[i] = 0;
		}

		/* Reset state machine variables */
		cirrus_pwr->uptime_ms = 0;
		cirrus_pwr->status = CIRRUS_PWR_STATUS_DISABLED;

		/* cancel workqueue */
		if (delayed_work_pending(&cirrus_pwr->pwr_work))
			cancel_delayed_work(&cirrus_pwr->pwr_work);
	}

	mutex_unlock(&cirrus_pwr->pwr_lock);
}
EXPORT_SYMBOL_GPL(cirrus_pwr_stop);

static void cirrus_pwr_work(struct work_struct *work)
{
	int i;
	struct cirrus_mfd_amp *amp;

	mutex_lock(&cirrus_pwr->pwr_lock);

	/* Run state machine and enable/disable Passport accordingly */

	switch (cirrus_pwr->status) {
	case CIRRUS_PWR_STATUS_ENABLED:
		cirrus_pwr->uptime_ms += cirrus_pwr->interval;

		if (cirrus_pwr->uptime_ms <= cirrus_pwr->target_min_time_ms) {
			dev_dbg(cirrus_pwr->dev,
				"Waiting for min time... (%d / %d ms)\n",
				cirrus_pwr->uptime_ms,
				cirrus_pwr->target_min_time_ms);
			break;
		}

		/* Enabled and > min time */
		/* Evaluate temp for each amp and enable/disable Passport */

		for (i = 0; i < cirrus_pwr->num_amps; i++) {

			amp = &cirrus_pwr->amps[i];

			dev_dbg(cirrus_pwr->dev,
				"Amp cs35l41%s\n", amp->mfd_suffix);
			dev_dbg(cirrus_pwr->dev,
				"Spk Temp:\t%d.%d C\t(Target: %d.%d C)\n",
					cirrus_pwr->spk_temp[i] / 100,
					cirrus_pwr->spk_temp[i] % 100,
					cirrus_pwr->target_temp[i] / 100,
					cirrus_pwr->target_temp[i] % 100);
			dev_dbg(cirrus_pwr->dev, "Amb Temp:\t%d.%d\n",
					cirrus_pwr->amb_temp[i] / 100,
					cirrus_pwr->amb_temp[i] % 100);

			if (cirrus_pwr->amp_active[i]) {
				if (cirrus_pwr->passport_enable[i]) {
					/* Evaluate exit criteria */
					if (cirrus_pwr->spk_temp[i] <
						cirrus_pwr->exit_temp[i]) {
						cirrus_pwr_passport_enable(
							amp->regmap,
							false);
						dev_info(cirrus_pwr->dev,
						"Amp cs35l41%s below exit temp. Disabling PASSPORT\n",
						amp->mfd_suffix);
						cirrus_pwr->passport_enable[i] = 0;
					}
				} else {
					/* Evaluate entry criteria */
					if ((cirrus_pwr->amb_temp[i] +
							CIRRUS_PWR_AMB_TEMP_OFFSET <
							cirrus_pwr->spk_temp[i]) &&
						(cirrus_pwr->spk_temp[i] >
						cirrus_pwr->target_temp[i])) {
						cirrus_pwr_passport_enable(
							amp->regmap, true);
						dev_info(cirrus_pwr->dev,
						"Amp cs35l41%s above target temp and ambient + 5.\n",
						amp->mfd_suffix);
						dev_info(cirrus_pwr->dev, "Enabling PASSPORT\n");
						cirrus_pwr->passport_enable[i] = 1;
					}

				}
			}

			dev_dbg(cirrus_pwr->dev, "Amp cs35l41%s: Passport %s\n",
					amp->mfd_suffix,
					cirrus_pwr->passport_enable[i] ?
					"Enabled" : "Disabled");
		}

		break;
	case CIRRUS_PWR_STATUS_ERROR:
	case CIRRUS_PWR_STATUS_DISABLED:
	default:
		break;
	}

	mutex_unlock(&cirrus_pwr->pwr_lock);

	/* Queue next operation */
	if (cirrus_pwr->global_enable) {
		queue_delayed_work(cirrus_pwr->pwr_workqueue,
			   &cirrus_pwr->pwr_work,
			   msecs_to_jiffies(cirrus_pwr->interval));
	}
}

/***** SYSFS Interfaces *****/

static ssize_t cirrus_pwr_version_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, CIRRUS_PWR_VERSION "\n");
}

static ssize_t cirrus_pwr_version_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return size;
}

static ssize_t cirrus_pwr_uptime_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->uptime_ms);
}

static ssize_t cirrus_pwr_uptime_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return size;
}

static ssize_t cirrus_pwr_power_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("value")]);
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(suffix);
	unsigned int power_squared;
	unsigned int power = 0;

	if (!amp)
		return 0;

	if (cirrus_pwr->amp_active[amp->index]) {
		regmap_read(amp->regmap,
			CIRRUS_PWR_CSPL_OUTPUT_POWER_SQ,
			&power_squared);
		power = convert_power(power_squared);
	}

	return sprintf(buf, "%x\n", power);
}

static ssize_t cirrus_pwr_power_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return size;
}

static ssize_t cirrus_pwr_interval_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->interval);
}

static ssize_t cirrus_pwr_interval_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->interval))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	switch (cirrus_pwr->status) {
	case CIRRUS_PWR_STATUS_DISABLED:
		return sprintf(buf, "Disabled\n");
	case CIRRUS_PWR_STATUS_ENABLED:
		return sprintf(buf, "Enabled\n");
	case CIRRUS_PWR_STATUS_ERROR:
		return sprintf(buf, "Error\n");
	default:
		return sprintf(buf, "\n");
	}
}

static ssize_t cirrus_pwr_status_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return size;
}

static ssize_t cirrus_pwr_target_min_time_ms_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->target_min_time_ms);
}

static ssize_t cirrus_pwr_target_min_time_ms_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->target_min_time_ms))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_target_temp_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("target_temp")]);
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(suffix);

	if (!amp)
		return 0;

	return sprintf(buf, "%d\n", cirrus_pwr->target_temp[amp->index]);
}

static ssize_t cirrus_pwr_target_temp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	const char *suffix = &(attr->attr.name[strlen("target_temp")]);
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(suffix);

	if (!amp)
		return 0;

	if (kstrtou32(buf, 0, &cirrus_pwr->target_temp[amp->index]))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_exit_temp_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("exit_temp")]);
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(suffix);

	if (!amp)
		return 0;

	return sprintf(buf, "%d\n", cirrus_pwr->exit_temp[amp->index]);
}

static ssize_t cirrus_pwr_exit_temp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	const char *suffix = &(attr->attr.name[strlen("exit_temp")]);
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(suffix);

	if (!amp)
		return 0;

	if (kstrtou32(buf, 0, &cirrus_pwr->exit_temp[amp->index]))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_amb_temp_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("amb_temp")]);
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(suffix);

	if (!amp)
		return 0;

	return sprintf(buf, "%d\n", cirrus_pwr->amb_temp[amp->index]);
}

static ssize_t cirrus_pwr_amb_temp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	const char *suffix = &(attr->attr.name[strlen("amb_temp")]);
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(suffix);

	if (!amp)
		return 0;

	if (kstrtou32(buf, 0, &cirrus_pwr->amb_temp[amp->index]))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_spk_temp_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("spk_t")]);
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(suffix);

	if (!amp)
		return 0;

	return sprintf(buf, "%d\n", cirrus_pwr->spk_temp[amp->index]);
}

static ssize_t cirrus_pwr_spk_temp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	const char *suffix = &(attr->attr.name[strlen("spk_t")]);
	struct cirrus_mfd_amp *amp = cirrus_pwr_get_amp_from_suffix(suffix);

	if (!amp)
		return 0;

	if (kstrtou32(buf, 0, &cirrus_pwr->spk_temp[amp->index]))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_global_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->global_enable);
}

static ssize_t cirrus_pwr_global_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned int enable;
	int i;

	if (kstrtou32(buf, 0, &enable)) {
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
		return size;
	}

	cirrus_pwr->global_enable = enable;

	if (enable == 0 &&
		cirrus_pwr->status == CIRRUS_PWR_STATUS_ENABLED) {
		/* Stop all amps */
		for (i = 0; i < cirrus_pwr->num_amps; i++)
			cirrus_pwr_stop(cirrus_pwr->amps[i].mfd_suffix);
	}

	return size;
}

static DEVICE_ATTR(version, 0444, cirrus_pwr_version_show,
				cirrus_pwr_version_store);
static DEVICE_ATTR(uptime, 0444, cirrus_pwr_uptime_show,
				cirrus_pwr_uptime_store);
static DEVICE_ATTR(global_enable, 0664, cirrus_pwr_global_enable_show,
				cirrus_pwr_global_enable_store);
static DEVICE_ATTR(interval, 0664, cirrus_pwr_interval_show,
				cirrus_pwr_interval_store);
static DEVICE_ATTR(status, 0664, cirrus_pwr_status_show,
				cirrus_pwr_status_store);
static DEVICE_ATTR(target_min_time_ms, 0664, cirrus_pwr_target_min_time_ms_show,
				cirrus_pwr_target_min_time_ms_store);

static struct attribute *cirrus_pwr_attr_base[] = {
	&dev_attr_version.attr,
	&dev_attr_uptime.attr,
	&dev_attr_interval.attr,
	&dev_attr_status.attr,
	&dev_attr_target_min_time_ms.attr,
	&dev_attr_global_enable.attr,
	NULL,
};

static struct device_attribute generic_amp_attrs[CIRRUS_PWR_NUM_ATTRS_AMP] = {
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0444)},
		.show = cirrus_pwr_power_show,
		.store = cirrus_pwr_power_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0664)},
		.show = cirrus_pwr_target_temp_show,
		.store = cirrus_pwr_target_temp_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0664)},
		.show = cirrus_pwr_exit_temp_show,
		.store = cirrus_pwr_exit_temp_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0664)},
		.show = cirrus_pwr_amb_temp_show,
		.store = cirrus_pwr_amb_temp_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0664)},
		.show = cirrus_pwr_spk_temp_show,
		.store = cirrus_pwr_spk_temp_store,
	},
};

static const char *generic_amp_attr_names[CIRRUS_PWR_NUM_ATTRS_AMP] = {
	"value",
	"target_temp",
	"exit_temp",
	"env_temp",
	"spk_t",
};

static struct device_attribute
		amp_attrs_prealloc[CIRRUS_MAX_AMPS][CIRRUS_PWR_NUM_ATTRS_AMP];
static char attr_names_prealloc[CIRRUS_MAX_AMPS][CIRRUS_PWR_NUM_ATTRS_AMP][20];

struct device_attribute *cirrus_pwr_create_amp_attrs(const char *mfd_suffix,
							int index)
{
	struct device_attribute *amp_attrs_new;
	int i, suffix_len = strlen(mfd_suffix);

	amp_attrs_new = &(amp_attrs_prealloc[index][0]);
	if (amp_attrs_new == NULL)
		return amp_attrs_new;

	memcpy(amp_attrs_new, &generic_amp_attrs,
		sizeof(struct device_attribute) *
		CIRRUS_PWR_NUM_ATTRS_AMP);

	for (i = 0; i < CIRRUS_PWR_NUM_ATTRS_AMP; i++) {
		amp_attrs_new[i].attr.name = attr_names_prealloc[index][i];
		snprintf((char *)amp_attrs_new[i].attr.name,
			strlen(generic_amp_attr_names[i]) + suffix_len + 1,
			"%s%s", generic_amp_attr_names[i], mfd_suffix);
	}

	return amp_attrs_new;
}

int cirrus_pwr_init(struct class *cirrus_amp_class, int num_amps,
					const char **mfd_suffixes)
{
	int ret = 0, i, j;
	struct device_attribute *new_attrs;

	cirrus_pwr = kzalloc(sizeof(struct cirrus_pwr_t), GFP_KERNEL);
	if (cirrus_pwr == NULL)
		return -ENOMEM;

	cirrus_pwr->amps = kzalloc(sizeof(struct cirrus_mfd_amp) * num_amps,
								GFP_KERNEL);
	if (cirrus_pwr->amps == NULL) {
		kfree(cirrus_pwr);
		return -ENOMEM;
	}

	cirrus_pwr->num_amps = num_amps;

	for (i = 0; i < num_amps; i++) {
		cirrus_pwr->amps[i].mfd_suffix = mfd_suffixes[i];
		cirrus_pwr->amps[i].index = i;

		cirrus_pwr->amb_temp[i] = 2500;
		cirrus_pwr->spk_temp[i] = 2500;
		cirrus_pwr->target_temp[i] = 3400;
		cirrus_pwr->exit_temp[i] = 3250;
		cirrus_pwr->passport_enable[i] = 0;
	}

	cirrus_pwr->pwr_class = cirrus_amp_class;

	cirrus_pwr->dev = device_create(cirrus_pwr->pwr_class, NULL, 1, NULL,
						CIRRUS_PWR_DIR_NAME);
	if (IS_ERR(cirrus_pwr->dev)) {
		ret = PTR_ERR(cirrus_pwr->dev);
		pr_err("Failed to create cirrus_pwr device\n");
		class_destroy(cirrus_pwr->pwr_class);
		goto err;
	}

	cirrus_pwr_attr_grp.attrs = kzalloc(sizeof(struct attribute *) *
					(CIRRUS_PWR_NUM_ATTRS_AMP * num_amps +
					CIRRUS_PWR_NUM_ATTRS_BASE + 1),
							GFP_KERNEL);
	for (i = 0; i < num_amps; i++) {
		new_attrs = cirrus_pwr_create_amp_attrs(mfd_suffixes[i], i);
		for (j = 0; j < CIRRUS_PWR_NUM_ATTRS_AMP; j++) {
			dev_dbg(cirrus_pwr->dev, "New attribute: %s\n",
				new_attrs[j].attr.name);
			cirrus_pwr_attr_grp.attrs[i * CIRRUS_PWR_NUM_ATTRS_AMP
						 + j] = &new_attrs[j].attr;
		}
	}

	memcpy(&cirrus_pwr_attr_grp.attrs[num_amps * CIRRUS_PWR_NUM_ATTRS_AMP],
		cirrus_pwr_attr_base, sizeof(struct attribute *) *
					CIRRUS_PWR_NUM_ATTRS_BASE);
	cirrus_pwr_attr_grp.attrs[num_amps * CIRRUS_PWR_NUM_ATTRS_AMP +
			CIRRUS_PWR_NUM_ATTRS_BASE] = NULL;

	cirrus_pwr->pwr_workqueue = create_singlethread_workqueue(
						CIRRUS_PWR_WORKQ_NAME);
	if (cirrus_pwr->pwr_workqueue == NULL) {
		dev_err(cirrus_pwr->dev, "Failed to create workqueue\n");
		ret = -ENOENT;
		goto err;
	}

	cirrus_pwr->interval = 10000;
	cirrus_pwr->uptime_ms = 0;
	cirrus_pwr->target_min_time_ms = 300000;
	cirrus_pwr->global_enable = 1;

	ret = sysfs_create_group(&cirrus_pwr->dev->kobj, &cirrus_pwr_attr_grp);
	if (ret) {
		dev_err(cirrus_pwr->dev, "Failed to create sysfs group\n");
		goto err;
	}

	mutex_init(&cirrus_pwr->pwr_lock);
	INIT_DELAYED_WORK(&cirrus_pwr->pwr_work,
						cirrus_pwr_work);

	return 0;
err:
	kfree(cirrus_pwr);
	return ret;
}

void cirrus_pwr_exit(void)
{
	kfree(cirrus_pwr);
}

