/*
 * Big-data logging support for Cirrus Logic CS35L41 codec
 *
 * Copyright 2017 Cirrus Logic
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
#include <linux/firmware.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/ktime.h>

#include <linux/mfd/cs35l41/core.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/big_data.h>
#include <linux/mfd/cs35l41/wmfw.h>

#define CIRRUS_BD_VERSION "5.01.18"

#define CIRRUS_BD_CLASS_NAME		"cirrus"
#define CIRRUS_BD_DIR_NAME		"cirrus_bd"

struct cirrus_bd_t {
	struct class *bd_class;
	struct device *dev;
	struct cirrus_mfd_amp *amps;
	int num_amps;
	const char *bd_suffixes[CIRRUS_MAX_AMPS];
	int max_temp_limit[CIRRUS_MAX_AMPS];	
	unsigned int max_exc[CIRRUS_MAX_AMPS];
	unsigned int over_exc_count[CIRRUS_MAX_AMPS];
	unsigned int max_temp[CIRRUS_MAX_AMPS];
	unsigned int max_temp_keep[CIRRUS_MAX_AMPS];
	unsigned int over_temp_count[CIRRUS_MAX_AMPS];
	unsigned int abnm_mute[CIRRUS_MAX_AMPS];
	unsigned long long int last_update;
};

struct cirrus_bd_ext cirrus_bd_data;
EXPORT_SYMBOL_GPL(cirrus_bd_data);

struct cirrus_bd_data_t {
	uint32_t max_temp;
	uint32_t max_exc;
	uint32_t over_temp_count;
	uint32_t over_exc_count;
	uint32_t abnm_mute;
};

struct cirrus_bd_t *cirrus_bd;
static struct attribute_group cirrus_bd_attr_grp;

int cirrus_bd_get_index_from_suffix(const char *suffix)
{
	int i;

	if (cirrus_bd == NULL || cirrus_bd->amps == NULL)
		return -EINVAL;

	dev_dbg(cirrus_bd->dev, "%s: suffix = %s\n", __func__, suffix);

	for (i = 0; i < cirrus_bd->num_amps; i++) {
		if (cirrus_bd->bd_suffixes[i]){
			dev_dbg(cirrus_bd->dev, "(bd) comparing %s & %s\n",
				cirrus_bd->bd_suffixes[i],
				suffix);
			if (strcmp(cirrus_bd->bd_suffixes[i], suffix) == 0) {
				return i;
			}
		}
	}

	for (i = 0; i < cirrus_bd->num_amps; i++) {
		dev_dbg(cirrus_bd->dev, "comparing %s & %s\n",
				cirrus_bd->amps[i].mfd_suffix,
				suffix);
		if (strcmp(cirrus_bd->amps[i].mfd_suffix, suffix) == 0)
			return i;
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(cirrus_bd_get_index_from_suffix);

struct cirrus_mfd_amp *cirrus_bd_get_amp_from_suffix(const char *suffix)
{
	int i;
	struct cirrus_mfd_amp *ret = NULL;

	if (cirrus_bd == NULL || cirrus_bd->amps == NULL)
		return NULL;

	dev_dbg(cirrus_bd->dev, "%s: suffix = %s\n", __func__, suffix);

	for (i = 0; i < cirrus_bd->num_amps; i++) {
		if (cirrus_bd->bd_suffixes[i]){
			dev_dbg(cirrus_bd->dev, "(bd) comparing %s & %s\n",
				cirrus_bd->bd_suffixes[i],
				suffix);
			if (strcmp(cirrus_bd->bd_suffixes[i], suffix) == 0) {
				return &cirrus_bd->amps[i];
			}
		}
	}

	for (i = 0; i < cirrus_bd->num_amps; i++) {
		dev_dbg(cirrus_bd->dev, "comparing %s & %s\n",
				cirrus_bd->amps[i].mfd_suffix,
				suffix);
		if (strcmp(cirrus_bd->amps[i].mfd_suffix, suffix) == 0)
			ret = &cirrus_bd->amps[i];
	}

	return ret;
}

int cirrus_bd_amp_add(struct regmap *regmap_new, const char *mfd_suffix,
				const char *dsp_part_name, int bd_max_temp)
{
	struct cirrus_mfd_amp *amp = cirrus_bd_get_amp_from_suffix(mfd_suffix);

	if (cirrus_bd){
		if (amp) {
			dev_info(cirrus_bd->dev,
				"Amp added, suffix: %s dsp_part_name: %s\n",
				mfd_suffix, dsp_part_name);
			amp->regmap = regmap_new;
			amp->dsp_part_name = dsp_part_name;
			if (bd_max_temp > 0)
				cirrus_bd->max_temp_limit[amp->index] =
								bd_max_temp - 1;
			else
				cirrus_bd->max_temp_limit[amp->index] = 99;			
		} else {
			dev_err(cirrus_bd->dev,
				"No amp with suffix %s registered\n",
				mfd_suffix);
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

void cirrus_bd_store_values(const char *mfd_suffix)
{
	struct cirrus_bd_data_t *data;
	struct cirrus_mfd_amp *amp = cirrus_bd_get_amp_from_suffix(mfd_suffix);
	struct regmap *regmap;
	int ret = 0;

	if (!amp)
		return;

	data = kzalloc(sizeof(struct cirrus_bd_data_t), GFP_KERNEL);
	if (data == NULL) {
		dev_err(cirrus_bd->dev,
			"%s: Failed to allocate\n", __func__);
		return;
	}

	regmap = amp->regmap;

	ret = regmap_bulk_read(regmap, CS35L41_BD_MAX_TEMP, data, 5);
	if (ret != 0) {
		dev_err(cirrus_bd->dev,
			"%s: Failed to read\n", __func__);
		goto err;
	}

	if (data->max_temp > (cirrus_bd->max_temp_limit[amp->index] *
			(1 << CS35L41_BD_TEMP_RADIX)) &&
			data->over_temp_count == 0)
		data->max_temp = (cirrus_bd->max_temp_limit[amp->index] *
			    (1 << CS35L41_BD_TEMP_RADIX));

	cirrus_bd->over_temp_count[amp->index] += data->over_temp_count;
	cirrus_bd->over_exc_count[amp->index] += data->over_exc_count;
	if (data->max_exc > cirrus_bd->max_exc[amp->index])
		cirrus_bd->max_exc[amp->index] = data->max_exc;
	if (data->max_temp > cirrus_bd->max_temp[amp->index])
		cirrus_bd->max_temp[amp->index] = data->max_temp;
	cirrus_bd->abnm_mute[amp->index] += data->abnm_mute;

	cirrus_bd->max_temp_keep[amp->index] = cirrus_bd->max_temp[amp->index];

	cirrus_bd->last_update = ktime_to_ns(ktime_get());

	dev_info(cirrus_bd->dev, "Values stored for cs35l41%s:\n", mfd_suffix);
	dev_info(cirrus_bd->dev, "Max Excursion:\t\t%d.%04d\n",
				cirrus_bd->max_exc[amp->index] >>
				CS35L41_BD_EXC_RADIX,
				(cirrus_bd->max_exc[amp->index] &
				(((1 << CS35L41_BD_EXC_RADIX) - 1))) *
				10000 / (1 << CS35L41_BD_EXC_RADIX));
	dev_info(cirrus_bd->dev, "Over Excursion Count:\t%d\n",
				cirrus_bd->over_exc_count[amp->index]);
	dev_info(cirrus_bd->dev, "Max Temp:\t\t\t%d.%04d\n",
				cirrus_bd->max_temp[amp->index] >>
				CS35L41_BD_TEMP_RADIX,
				(cirrus_bd->max_temp[amp->index] &
				(((1 << CS35L41_BD_TEMP_RADIX) - 1))) *
				10000 / (1 << CS35L41_BD_TEMP_RADIX));
	dev_info(cirrus_bd->dev, "Over Temp Count:\t\t%d\n",
				cirrus_bd->over_temp_count[amp->index]);
	dev_info(cirrus_bd->dev, "Abnormal Mute:\t\t%d\n",
				cirrus_bd->abnm_mute[amp->index]);
	dev_info(cirrus_bd->dev, "Timestamp:\t\t%llu\n",
				cirrus_bd->last_update);

	data->over_temp_count = 0;
	data->over_exc_count = 0;
	data->max_exc = 0;
	data->max_temp = 0;
	data->abnm_mute = 0;

	ret = regmap_bulk_write(regmap, CS35L41_BD_MAX_TEMP, data, 5);
	if (ret != 0) {
		dev_err(cirrus_bd->dev,
			"%s: Failed to clear\n", __func__);
		goto err;
	}
err:
	kfree(data);
}
EXPORT_SYMBOL_GPL(cirrus_bd_store_values);

/***** SYSFS Interfaces *****/

static ssize_t cirrus_bd_version_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, CIRRUS_BD_VERSION "\n");
}

static ssize_t cirrus_bd_version_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_max_exc_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("max_exc")]);
	struct cirrus_mfd_amp *amp = cirrus_bd_get_amp_from_suffix(suffix);
	int ret;

	if (!amp)
		return 0;

	ret = sprintf(buf, "%d.%04d\n",
			cirrus_bd->max_exc[amp->index] >> CS35L41_BD_EXC_RADIX,
			(cirrus_bd->max_exc[amp->index] &
			(((1 << CS35L41_BD_EXC_RADIX) - 1))) *
			10000 / (1 << CS35L41_BD_EXC_RADIX));

	cirrus_bd->max_exc[amp->index] = 0;
	return ret;
}

static ssize_t cirrus_bd_max_exc_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_over_exc_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("over_exc_count")]);
	struct cirrus_mfd_amp *amp = cirrus_bd_get_amp_from_suffix(suffix);
	int ret;

	if (!amp)
		return 0;

	ret = sprintf(buf, "%d\n", cirrus_bd->over_exc_count[amp->index]);

	cirrus_bd->over_exc_count[amp->index] = 0;
	return ret;
}

static ssize_t cirrus_bd_over_exc_count_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_max_temp_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("max_temp")]);
	struct cirrus_mfd_amp *amp = cirrus_bd_get_amp_from_suffix(suffix);
	int ret;

	if (!amp)
		return 0;

	ret = sprintf(buf, "%d.%04d\n",
			cirrus_bd->max_temp[amp->index] >> CS35L41_BD_TEMP_RADIX,
			(cirrus_bd->max_temp[amp->index] &
			(((1 << CS35L41_BD_TEMP_RADIX) - 1))) *
			10000 / (1 << CS35L41_BD_TEMP_RADIX));

	cirrus_bd->max_temp[amp->index] = 0;
	return ret;
}

static ssize_t cirrus_bd_max_temp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_max_temp_keep_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("max_temp_keep")]);
	struct cirrus_mfd_amp *amp = cirrus_bd_get_amp_from_suffix(suffix);
	int ret;

	if (!amp)
		return 0;

	ret = sprintf(buf, "%d.%04d\n",
			cirrus_bd->max_temp_keep[amp->index] >> CS35L41_BD_TEMP_RADIX,
			(cirrus_bd->max_temp_keep[amp->index] &
			(((1 << CS35L41_BD_TEMP_RADIX) - 1))) *
			10000 / (1 << CS35L41_BD_TEMP_RADIX));

	return ret;
}

static ssize_t cirrus_bd_max_temp_keep_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_over_temp_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("over_temp_count")]);
	struct cirrus_mfd_amp *amp = cirrus_bd_get_amp_from_suffix(suffix);
	int ret;

	if (!amp)
		return 0;

	ret = sprintf(buf, "%d\n", cirrus_bd->over_temp_count[amp->index]);

	cirrus_bd->over_temp_count[amp->index] = 0;
	return ret;
}

static ssize_t cirrus_bd_over_temp_count_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_abnm_mute_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	const char *suffix = &(attr->attr.name[strlen("abnm_mute")]);
	struct cirrus_mfd_amp *amp = cirrus_bd_get_amp_from_suffix(suffix);
	int ret;

	if (!amp)
		return 0;

	ret = sprintf(buf, "%d\n", cirrus_bd->abnm_mute[amp->index]);

	cirrus_bd->abnm_mute[amp->index] = 0;
	return ret;
}

static ssize_t cirrus_bd_abnm_mute_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_store_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return 0;
}

static ssize_t cirrus_bd_store_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int store;
	int ret = kstrtos32(buf, 10, &store);
	const char *suffix = &(attr->attr.name[strlen("store")]);
	struct cirrus_mfd_amp *amp = cirrus_bd_get_amp_from_suffix(suffix);
	
	if (ret == 0 && store == 1 && amp)
		cirrus_bd_store_values(suffix);

	return size;
}

static DEVICE_ATTR(version, 0444, cirrus_bd_version_show,
				cirrus_bd_version_store);

static struct attribute *cirrus_bd_attr_base[] = {
	&dev_attr_version.attr,
	NULL,
};

static struct device_attribute generic_amp_attrs[CIRRUS_BD_NUM_ATTRS_AMP] = {
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0444)},
		.show = cirrus_bd_max_exc_show,
		.store = cirrus_bd_max_exc_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0444)},
		.show = cirrus_bd_over_exc_count_show,
		.store = cirrus_bd_over_exc_count_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0444)},
		.show = cirrus_bd_max_temp_show,
		.store = cirrus_bd_max_temp_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0444)},
		.show = cirrus_bd_max_temp_keep_show,
		.store = cirrus_bd_max_temp_keep_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0444)},
		.show = cirrus_bd_over_temp_count_show,
		.store = cirrus_bd_over_temp_count_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0444)},
		.show = cirrus_bd_abnm_mute_show,
		.store = cirrus_bd_abnm_mute_store,
	},
	{
		.attr = {.mode = VERIFY_OCTAL_PERMISSIONS(0644)},
		.show = cirrus_bd_store_show,
		.store = cirrus_bd_store_store,
	},
};

static const char *generic_amp_attr_names[CIRRUS_BD_NUM_ATTRS_AMP] = {
	"max_exc",
	"over_exc_count",
	"max_temp",
	"max_temp_keep",
	"over_temp_count",
	"abnm_mute",
	"store"
};

static struct device_attribute
		amp_attrs_prealloc[CIRRUS_MAX_AMPS][CIRRUS_BD_NUM_ATTRS_AMP];
static char attr_names_prealloc[CIRRUS_MAX_AMPS][CIRRUS_BD_NUM_ATTRS_AMP][30];

struct device_attribute *cirrus_bd_create_amp_attrs(const char *mfd_suffix,
							const char *bd_suffix,
							int index)
{
	struct device_attribute *amp_attrs_new;
	int i, suffix_len;
	const char *suffix;

	suffix = (bd_suffix) ? bd_suffix : mfd_suffix;
	suffix_len = strlen(suffix);

	amp_attrs_new = &(amp_attrs_prealloc[index][0]);
	if (amp_attrs_new == NULL)
		return amp_attrs_new;

	memcpy(amp_attrs_new, &generic_amp_attrs,
		sizeof(struct device_attribute) *
		CIRRUS_BD_NUM_ATTRS_AMP);

	for (i = 0; i < CIRRUS_BD_NUM_ATTRS_AMP - 1; i++) {
		amp_attrs_new[i].attr.name = attr_names_prealloc[index][i];
		snprintf((char *)amp_attrs_new[i].attr.name,
			strlen(generic_amp_attr_names[i]) + suffix_len + 1,
			"%s%s", generic_amp_attr_names[i], suffix);
	}

	/* "store" is special and will always be assigned the MFD suffix */
	amp_attrs_new[CIRRUS_BD_NUM_ATTRS_AMP - 1].attr.name =
			attr_names_prealloc[index][CIRRUS_BD_NUM_ATTRS_AMP - 1];
	snprintf((char *)amp_attrs_new[CIRRUS_BD_NUM_ATTRS_AMP - 1].attr.name,
		strlen("store") + strlen(mfd_suffix) + 1,
		"%s%s", "store", mfd_suffix);

	return amp_attrs_new;
}

int cirrus_bd_init(struct class *cirrus_amp_class, int num_amps,
					const char **mfd_suffixes,
					const char **bd_suffixes)
{
	int ret, i, j;
	struct device_attribute *new_attrs;

	cirrus_bd = kzalloc(sizeof(struct cirrus_bd_t), GFP_KERNEL);
	if (cirrus_bd == NULL)
		return -ENOMEM;

	cirrus_bd->amps = kzalloc(sizeof(struct cirrus_mfd_amp) * num_amps,
								GFP_KERNEL);
	if (cirrus_bd->amps == NULL) {
		kfree(cirrus_bd);
		return -ENOMEM;
	}

	cirrus_bd->num_amps = num_amps;

	for (i = 0; i < num_amps; i++) {
		cirrus_bd->amps[i].mfd_suffix = mfd_suffixes[i];
		cirrus_bd->amps[i].index = i;
		cirrus_bd->bd_suffixes[i] = bd_suffixes[i];

		cirrus_bd->max_exc[i] = 0;
		cirrus_bd->over_exc_count[i] = 0;
		cirrus_bd->max_temp[i] = 0;
		cirrus_bd->over_temp_count[i] = 0;
		cirrus_bd->abnm_mute[i] = 0;
	}

	cirrus_bd_data.max_exc = cirrus_bd->max_exc;
	cirrus_bd_data.over_exc_count = cirrus_bd->over_exc_count;
	cirrus_bd_data.max_temp = cirrus_bd->max_temp;
	cirrus_bd_data.max_temp_keep = cirrus_bd->max_temp_keep;
	cirrus_bd_data.over_temp_count = cirrus_bd->over_temp_count;
	cirrus_bd_data.abnm_mute = cirrus_bd->abnm_mute;

	cirrus_bd->bd_class = cirrus_amp_class;

	cirrus_bd->dev = device_create(cirrus_bd->bd_class, NULL, 1, NULL,
						CIRRUS_BD_DIR_NAME);
	if (IS_ERR(cirrus_bd->dev)) {
		ret = PTR_ERR(cirrus_bd->dev);
		pr_err("Failed to create cirrus_bd device\n");
		class_destroy(cirrus_bd->bd_class);
		goto err;
	}

	cirrus_bd_attr_grp.attrs = kzalloc(sizeof(struct attribute *) *
					(CIRRUS_BD_NUM_ATTRS_AMP * num_amps +
					CIRRUS_BD_NUM_ATTRS_BASE + 1),
							GFP_KERNEL);
	for (i = 0; i < num_amps; i++) {
		new_attrs = cirrus_bd_create_amp_attrs(mfd_suffixes[i],
							bd_suffixes[i], i);
		for (j = 0; j < CIRRUS_BD_NUM_ATTRS_AMP; j++) {
			dev_dbg(cirrus_bd->dev, "New attribute: %s\n",
				new_attrs[j].attr.name);
			cirrus_bd_attr_grp.attrs[i * CIRRUS_BD_NUM_ATTRS_AMP
						 + j] = &new_attrs[j].attr;
		}
	}

	memcpy(&cirrus_bd_attr_grp.attrs[num_amps * CIRRUS_BD_NUM_ATTRS_AMP],
		cirrus_bd_attr_base, sizeof(struct attribute *) *
					CIRRUS_BD_NUM_ATTRS_BASE);
	cirrus_bd_attr_grp.attrs[num_amps * CIRRUS_BD_NUM_ATTRS_AMP +
			CIRRUS_BD_NUM_ATTRS_BASE] = NULL;

	ret = sysfs_create_group(&cirrus_bd->dev->kobj, &cirrus_bd_attr_grp);
	if (ret) {
		dev_err(cirrus_bd->dev, "Failed to create sysfs group\n");
		goto err;
	}

	return 0;
err:
	kfree(cirrus_bd->amps);
	kfree(cirrus_bd);
	return ret;
}

void cirrus_bd_exit(void)
{
	kfree(cirrus_bd->amps);
	kfree(cirrus_bd);
}

