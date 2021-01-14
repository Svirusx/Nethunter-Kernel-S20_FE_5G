/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/dirent.h>
#include "adsp.h"
#define VENDOR "CAPELLA"
#ifdef CONFIG_VEML3328_SUB_FACTORY
#define CHIP_ID "VEML3328_SUB"
#elif CONFIG_VEML3235_SUB_FACTORY
#define CHIP_ID "VEML3235_SUB"
#else
#define CHIP_ID "VEML3xxx_SUB"
#endif

/*************************************************************************/
/* factory Sysfs							 */
/*************************************************************************/
static ssize_t sub_light_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t sub_light_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t sub_light_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(NULL, 0, MSG_LIGHT_SUB, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_LIGHT_SUB) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_LIGHT_SUB);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		mutex_unlock(&data->light_factory_mutex);
		return snprintf(buf, PAGE_SIZE, "0,0,0,0,0,0\n");
	}

	mutex_unlock(&data->light_factory_mutex);
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_LIGHT_SUB][0], data->msg_buf[MSG_LIGHT_SUB][1],
		data->msg_buf[MSG_LIGHT_SUB][2], data->msg_buf[MSG_LIGHT_SUB][3],
		data->msg_buf[MSG_LIGHT_SUB][4], data->msg_buf[MSG_LIGHT_SUB][5]);
}

static ssize_t sub_light_get_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s: start\n", __func__);
	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(NULL, 0, MSG_LIGHT_SUB, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << MSG_LIGHT_SUB) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << MSG_LIGHT_SUB);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	mutex_unlock(&data->light_factory_mutex);
	return data->msg_buf[MSG_LIGHT_SUB][0];
}

static DEVICE_ATTR(vendor, 0444, sub_light_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, sub_light_name_show, NULL);
static DEVICE_ATTR(lux, 0444, sub_light_raw_data_show, NULL);
static DEVICE_ATTR(raw_data, 0444, sub_light_raw_data_show, NULL);
static DEVICE_ATTR(dhr_sensor_info, 0444, sub_light_get_dhr_sensor_info_show, NULL);

static struct device_attribute *light_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_lux,
	&dev_attr_raw_data,
	&dev_attr_dhr_sensor_info,
	NULL,
};

static int __init veml3xxx_sub_light_factory_init(void)
{
	adsp_factory_register(MSG_LIGHT_SUB, light_attrs);

	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

static void __exit veml3xxx_sub_light_factory_exit(void)
{
	adsp_factory_unregister(MSG_LIGHT_SUB);

	pr_info("[FACTORY] %s\n", __func__);
}
module_init(veml3xxx_sub_light_factory_init);
module_exit(veml3xxx_sub_light_factory_exit);
