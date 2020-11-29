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
#include "adsp.h"
#define VENDOR "STM"
#define CHIP_ID "LPS22HH"

#define CALIBRATION_FILE_PATH "/efs/FactoryApp/baro_delta"

#define	PR_MAX 8388607 /* 24 bit 2'compl */
#define	PR_MIN -8388608
#define SNS_SUCCESS 0
#define ST_PASS 1
#define ST_FAIL 0

static int sea_level_pressure;
static int pressure_cal;

static ssize_t pressure_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t pressure_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t sea_level_pressure_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sea_level_pressure);
}

static ssize_t sea_level_pressure_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (sscanf(buf, "%10d", &sea_level_pressure) != 1) {
		pr_err("[FACTORY] %s: sscanf error\n", __func__);
		return size;
	}

	sea_level_pressure = sea_level_pressure / 100;

	pr_info("[FACTORY] %s: sea_level_pressure = %d\n", __func__,
		sea_level_pressure);

	return size;
}

/*
int pressure_open_calibration(struct adsp_data *data)
{
	int error = 0;

	return error;
}
*/

static ssize_t pressure_cabratioin_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int temp = 0, error = 0;

	error = kstrtoint(buf, 10, &temp);
	if (error < 0) {
		pr_err("[FACTORY] %s : kstrtoint failed.(%d)", __func__, error);
		return error;
	}

	if (temp < PR_MIN || temp > PR_MAX)
		return -EINVAL;

	pressure_cal = temp;

	return size;
}

static ssize_t pressure_cabratioin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	//struct adsp_data *data = dev_get_drvdata(dev);

	//pressure_open_calibration(data);

	return snprintf(buf, PAGE_SIZE, "%d\n", pressure_cal);
}

static ssize_t temperature_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_PRESSURE_TEMP, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] &
		1 << MSG_PRESSURE_TEMP) && cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_PRESSURE_TEMP);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-99\n");
	}
	return snprintf(buf, PAGE_SIZE, "%d\n",
		data->msg_buf[MSG_PRESSURE_TEMP][0]);
}

static ssize_t selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_PRESSURE, 0, MSG_TYPE_ST_SHOW_DATA);

	while (!(data->ready_flag[MSG_TYPE_ST_SHOW_DATA] &
		1 << MSG_PRESSURE) && cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_ST_SHOW_DATA] &= ~(1 << MSG_PRESSURE);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "0\n");
	}

	pr_info("[FACTORY] %s : P:%d, T:%d, RES:%d\n",
		__func__, data->msg_buf[MSG_PRESSURE][0],
		data->msg_buf[MSG_PRESSURE][1], data->msg_buf[MSG_PRESSURE][2]);

	if (SNS_SUCCESS == data->msg_buf[MSG_PRESSURE][2])
		return snprintf(buf, PAGE_SIZE, "%d\n", ST_PASS);
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", ST_FAIL);
}

static ssize_t pressure_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int i = 0;

	adsp_unicast(NULL, 0, MSG_PRESSURE, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << MSG_PRESSURE) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << MSG_PRESSURE);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		for (i = 0; i < 7; i++) {
			pr_info("[FACTORY] %s - %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
				__func__,
				data->msg_buf[MSG_PRESSURE][i * 16 + 0],
				data->msg_buf[MSG_PRESSURE][i * 16 + 1],
				data->msg_buf[MSG_PRESSURE][i * 16 + 2],
				data->msg_buf[MSG_PRESSURE][i * 16 + 3],
				data->msg_buf[MSG_PRESSURE][i * 16 + 4],
				data->msg_buf[MSG_PRESSURE][i * 16 + 5],
				data->msg_buf[MSG_PRESSURE][i * 16 + 6],
				data->msg_buf[MSG_PRESSURE][i * 16 + 7],
				data->msg_buf[MSG_PRESSURE][i * 16 + 8],
				data->msg_buf[MSG_PRESSURE][i * 16 + 9],
				data->msg_buf[MSG_PRESSURE][i * 16 + 10],
				data->msg_buf[MSG_PRESSURE][i * 16 + 11],
				data->msg_buf[MSG_PRESSURE][i * 16 + 12],
				data->msg_buf[MSG_PRESSURE][i * 16 + 13],
				data->msg_buf[MSG_PRESSURE][i * 16 + 14],
				data->msg_buf[MSG_PRESSURE][i * 16 + 15]);
		}
		pr_info("[FACTORY] %s - %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
			__func__, data->msg_buf[MSG_PRESSURE][i * 16 + 0],
			data->msg_buf[MSG_PRESSURE][i * 16 + 1],
			data->msg_buf[MSG_PRESSURE][i * 16 + 2],
			data->msg_buf[MSG_PRESSURE][i * 16 + 3],
			data->msg_buf[MSG_PRESSURE][i * 16 + 4],
			data->msg_buf[MSG_PRESSURE][i * 16 + 5],
			data->msg_buf[MSG_PRESSURE][i * 16 + 6],
			data->msg_buf[MSG_PRESSURE][i * 16 + 7]);
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", "Done");
}

static DEVICE_ATTR(vendor, 0444, pressure_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, pressure_name_show, NULL);
static DEVICE_ATTR(calibration, 0664,
	pressure_cabratioin_show, pressure_cabratioin_store);
static DEVICE_ATTR(sea_level_pressure, 0664,
	sea_level_pressure_show, sea_level_pressure_store);
static DEVICE_ATTR(temperature, 0444, temperature_show, NULL);
static DEVICE_ATTR(selftest, 0444, selftest_show, NULL);
#ifdef CONFIG_SEC_FACTORY
static DEVICE_ATTR(dhr_sensor_info, 0444,
	pressure_dhr_sensor_info_show, NULL);
#else
static DEVICE_ATTR(dhr_sensor_info, 0440,
	pressure_dhr_sensor_info_show, NULL);
#endif

static struct device_attribute *pressure_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_calibration,
	&dev_attr_sea_level_pressure,
	&dev_attr_temperature,
	&dev_attr_selftest,
	&dev_attr_dhr_sensor_info,
	NULL,
};

static int __init lps22hh_pressure_factory_init(void)
{
	adsp_factory_register(MSG_PRESSURE, pressure_attrs);

	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

static void __exit lps22hh_pressure_factory_exit(void)
{
	adsp_factory_unregister(MSG_PRESSURE);

	pr_info("[FACTORY] %s\n", __func__);
}

module_init(lps22hh_pressure_factory_init);
module_exit(lps22hh_pressure_factory_exit);
