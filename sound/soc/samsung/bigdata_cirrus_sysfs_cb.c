/*
 *  bigdata_cirrus_sysfs_cb.c
 *  Copyright (c) Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 */

#include <sound/soc.h>
#include <linux/mfd/cs35l41/big_data.h>
#include <sound/samsung/sec_audio_sysfs.h>
#include "bigdata_cirrus_sysfs_cb.h"

static struct snd_soc_component *cirrus_amp_component = NULL;

static const char *cirrus_amp_suffix[AMP_ID_MAX] = {
	[AMP_0] = "_0",
	[AMP_1] = "_1",
	[AMP_2] = "_2",
	[AMP_3] = "_3",
};

static int get_cirrus_amp_temperature_max(enum amp_id id)
{
	struct snd_soc_component *component = cirrus_amp_component;
	int value = 0;
	int cirrus_bd_index = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	dev_dbg(component->dev, "%s: %d\n", __func__, id);

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id\n", __func__);
		return -EINVAL;
	}

	cirrus_bd_index = cirrus_bd_get_index_from_suffix(cirrus_amp_suffix[id]);
	if (cirrus_bd_index < 0) {
		dev_err(component->dev, "%s: invalid suffix\n", __func__);
		return -EINVAL;
	}

	value = cirrus_bd_data.max_temp[cirrus_bd_index] >> CS35L41_BD_TEMP_RADIX;
	cirrus_bd_data.max_temp[cirrus_bd_index] = 0;

	dev_info(component->dev, "%s: cirrus_bd_index %d value %d\n", __func__, cirrus_bd_index, value);

	return value;
}

static int get_cirrus_amp_temperature_keep_max(enum amp_id id)
{
	struct snd_soc_component *component = cirrus_amp_component;
	int value = 0;
	int cirrus_bd_index = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	dev_dbg(component->dev, "%s: %d\n", __func__, id);

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id\n", __func__);
		return -EINVAL;
	}

	cirrus_bd_index = cirrus_bd_get_index_from_suffix(cirrus_amp_suffix[id]);
	if (cirrus_bd_index < 0) {
		dev_err(component->dev, "%s: invalid suffix\n", __func__);
		return -EINVAL;
	}

	value = cirrus_bd_data.max_temp_keep[cirrus_bd_index] >> CS35L41_BD_TEMP_RADIX;

	dev_info(component->dev, "%s: cirrus_bd_index %d value %d\n", __func__, cirrus_bd_index, value);

	return value;
}

static int get_cirrus_amp_temperature_overcount(enum amp_id id)
{
	struct snd_soc_component *component = cirrus_amp_component;
	int value = 0;
	int cirrus_bd_index = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	dev_dbg(component->dev, "%s: %d\n", __func__, id);

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id\n", __func__);
		return -EINVAL;
	}

	cirrus_bd_index = cirrus_bd_get_index_from_suffix(cirrus_amp_suffix[id]);
	if (cirrus_bd_index < 0) {
		dev_err(component->dev, "%s: invalid suffix\n", __func__);
		return -EINVAL;
	}

	value = cirrus_bd_data.over_temp_count[cirrus_bd_index];
	cirrus_bd_data.over_temp_count[cirrus_bd_index] = 0;

	dev_info(component->dev, "%s: cirrus_bd_index %d value %d\n", __func__, cirrus_bd_index, value);

	return value;
}

static int get_cirrus_amp_excursion_max(enum amp_id id)
{
	struct snd_soc_component *component = cirrus_amp_component;
	int value = 0;
	int cirrus_bd_index = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	dev_dbg(component->dev, "%s: %d\n", __func__, id);

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id\n", __func__);
		return -EINVAL;
	}

	cirrus_bd_index = cirrus_bd_get_index_from_suffix(cirrus_amp_suffix[id]);
	if (cirrus_bd_index < 0) {
		dev_err(component->dev, "%s: invalid suffix\n", __func__);
		return -EINVAL;
	}

	value = (cirrus_bd_data.max_exc[cirrus_bd_index] &
				(((1 << CS35L41_BD_EXC_RADIX) - 1))) *
				10000 / (1 << CS35L41_BD_EXC_RADIX);
	cirrus_bd_data.max_exc[cirrus_bd_index] = 0;

	dev_info(component->dev, "%s: cirrus_bd_index %d value %d\n", __func__, cirrus_bd_index, value);

	return value;
}

static int get_cirrus_amp_excursion_overcount(enum amp_id id)
{
	struct snd_soc_component *component = cirrus_amp_component;
	int value = 0;
	int cirrus_bd_index = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	dev_dbg(component->dev, "%s: %d\n", __func__, id);

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id\n", __func__);
		return -EINVAL;
	}

	cirrus_bd_index = cirrus_bd_get_index_from_suffix(cirrus_amp_suffix[id]);
	if (cirrus_bd_index < 0) {
		dev_err(component->dev, "%s: invalid suffix\n", __func__);
		return -EINVAL;
	}

	value = cirrus_bd_data.over_exc_count[cirrus_bd_index];
	cirrus_bd_data.over_exc_count[cirrus_bd_index] = 0;

	dev_info(component->dev, "%s: cirrus_bd_index %d value %d\n", __func__, cirrus_bd_index, value);

	return value;
}

void register_cirrus_bigdata_cb(struct snd_soc_component *component)
{
	cirrus_amp_component = component;

	dev_info(component->dev, "%s\n", __func__);

	audio_register_temperature_max_cb(get_cirrus_amp_temperature_max);
	audio_register_temperature_keep_max_cb(get_cirrus_amp_temperature_keep_max);
	audio_register_temperature_overcount_cb(get_cirrus_amp_temperature_overcount);
	audio_register_excursion_max_cb(get_cirrus_amp_excursion_max);
	audio_register_excursion_overcount_cb(get_cirrus_amp_excursion_overcount);
}
EXPORT_SYMBOL_GPL(register_cirrus_bigdata_cb);
