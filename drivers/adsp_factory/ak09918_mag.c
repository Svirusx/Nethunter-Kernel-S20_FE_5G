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
#define VENDOR "AKM"
#define CHIP_ID "AK09918"

#define MAG_ST_TRY_CNT 3
#define ABS(x) (((x)>0)?(x):-(x))
#define AKM_ST_FAIL (-1)

static ssize_t mag_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t mag_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t mag_check_cntl(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "OK\n");
}

static ssize_t mag_check_registers(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static ssize_t mag_get_asa(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* Do not have Fuserom */
	return snprintf(buf, PAGE_SIZE, "%u,%u,%u\n", 128, 128, 128);
}

static ssize_t mag_get_status(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* Do not have Fuserom */
	return snprintf(buf, PAGE_SIZE, "%s\n", "OK");
}

static ssize_t mag_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_MAG, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_MAG) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_MAG);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "0,0,0\n");
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->msg_buf[MSG_MAG][0],
		data->msg_buf[MSG_MAG][1],
		data->msg_buf[MSG_MAG][2]);
}

static ssize_t mag_raw_data_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	adsp_unicast(NULL, 0, MSG_MAG_CAL, 0, MSG_TYPE_FACTORY_ENABLE);
	msleep(20);
	adsp_unicast(NULL, 0, MSG_MAG_CAL, 0, MSG_TYPE_SET_CAL_DATA);
	msleep(20);
	adsp_unicast(NULL, 0, MSG_MAG_CAL, 0, MSG_TYPE_FACTORY_DISABLE);

	return size;
}

static ssize_t mag_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int retry = 0, i;
	int abs_adc_sum = 0, abs_adc_x = 0, abs_adc_y = 0, abs_adc_z = 0;
	int st_status = 0;

RETRY_MAG_SELFTEST:
	pr_info("[FACTORY] %s - start", __func__);
	adsp_unicast(NULL, 0, MSG_MAG, 0, MSG_TYPE_ST_SHOW_DATA);

	while (!(data->ready_flag[MSG_TYPE_ST_SHOW_DATA] & 1 << MSG_MAG) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_ST_SHOW_DATA] &= ~(1 << MSG_MAG);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		data->msg_buf[MSG_MAG][0] = -1;
	}

	if (!(data->msg_buf[MSG_MAG][0] == 0)) {
		if (retry < MAG_ST_TRY_CNT) {
			retry++;
			for (i = 0; i < 10; i++)
				data->msg_buf[MSG_MAG][i] = 0;

			msleep(100);
			cnt = 0;
			pr_info("[FACTORY] %s - retry %d", __func__, retry);
			goto RETRY_MAG_SELFTEST;
		}

		adsp_unicast(NULL, 0, MSG_MAG_CAL, 0, MSG_TYPE_FACTORY_ENABLE);
		msleep(20);
		adsp_unicast(NULL, 0, MSG_MAG_CAL, 0, MSG_TYPE_SET_CAL_DATA);
		msleep(20);
		adsp_unicast(NULL, 0, MSG_MAG_CAL, 0, MSG_TYPE_FACTORY_DISABLE);
		return snprintf(buf, PAGE_SIZE, "-1,0,0,0,0,0,0,0,0,0\n");
	}

	if (data->msg_buf[MSG_MAG][1] != 0) {
		pr_info("[FACTORY] %s - msg_buf[1] 0x%x", __func__, data->msg_buf[MSG_MAG][1]);
		st_status = AKM_ST_FAIL;
	}
	pr_info("[FACTORY] status=%d, st_status=%d, st_x=%d, st_y=%d, st_z=%d\n dac=%d, adc=%d, adc_x=%d, adc_y=%d, adc_z=%d\n",
		data->msg_buf[MSG_MAG][0], st_status,
		data->msg_buf[MSG_MAG][2], data->msg_buf[MSG_MAG][3],
		data->msg_buf[MSG_MAG][4], data->msg_buf[MSG_MAG][5],
		data->msg_buf[MSG_MAG][6], data->msg_buf[MSG_MAG][7],
		data->msg_buf[MSG_MAG][8], data->msg_buf[MSG_MAG][9]);

	abs_adc_x = ABS(data->msg_buf[MSG_MAG][7]);
	abs_adc_y = ABS(data->msg_buf[MSG_MAG][8]);
	abs_adc_z = ABS(data->msg_buf[MSG_MAG][9]);
	abs_adc_sum = abs_adc_x + abs_adc_y + abs_adc_z;

	if (abs_adc_sum >= 26666) {
		pr_info("[FACTORY] abs_adc_sum is higher then 40Gauss");
		st_status = AKM_ST_FAIL;
	}

	adsp_unicast(NULL, 0, MSG_MAG_CAL, 0, MSG_TYPE_FACTORY_ENABLE);
	msleep(20);
	adsp_unicast(NULL, 0, MSG_MAG_CAL, 0, MSG_TYPE_SET_CAL_DATA);
	msleep(20);
	adsp_unicast(NULL, 0, MSG_MAG_CAL, 0, MSG_TYPE_FACTORY_DISABLE);
	return snprintf(buf, PAGE_SIZE,	"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_MAG][0], st_status,
		data->msg_buf[MSG_MAG][2], data->msg_buf[MSG_MAG][3],
		data->msg_buf[MSG_MAG][4], data->msg_buf[MSG_MAG][5],
		data->msg_buf[MSG_MAG][6], data->msg_buf[MSG_MAG][7],
		data->msg_buf[MSG_MAG][8], data->msg_buf[MSG_MAG][9]);
}

static ssize_t mag_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_MAG, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << MSG_MAG) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << MSG_MAG);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		pr_info("[FACTORY] %s - [00h-03h] %02x,%02x,%02x,%02x [10h-16h,18h] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x [30h-32h] %02x,%02x,%02x\n",
			__func__,
			data->msg_buf[MSG_MAG][0], data->msg_buf[MSG_MAG][1],
			data->msg_buf[MSG_MAG][2], data->msg_buf[MSG_MAG][3],
			data->msg_buf[MSG_MAG][4], data->msg_buf[MSG_MAG][5],
			data->msg_buf[MSG_MAG][6], data->msg_buf[MSG_MAG][7],
			data->msg_buf[MSG_MAG][8], data->msg_buf[MSG_MAG][9],
			data->msg_buf[MSG_MAG][10], data->msg_buf[MSG_MAG][11],
			data->msg_buf[MSG_MAG][12], data->msg_buf[MSG_MAG][13],
			data->msg_buf[MSG_MAG][14]);
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", "Done");
}

static DEVICE_ATTR(name, 0444, mag_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, mag_vendor_show, NULL);
static DEVICE_ATTR(raw_data, 0664, mag_raw_data_show, mag_raw_data_store);
static DEVICE_ATTR(adc, 0444, mag_raw_data_show, NULL);
static DEVICE_ATTR(dac, 0444, mag_check_cntl, NULL);
static DEVICE_ATTR(chk_registers, 0444, mag_check_registers, NULL);
static DEVICE_ATTR(selftest, 0440, mag_selftest_show, NULL);
static DEVICE_ATTR(asa, 0444, mag_get_asa, NULL);
static DEVICE_ATTR(status, 0444, mag_get_status, NULL);
#ifdef CONFIG_SEC_FACTORY
static DEVICE_ATTR(dhr_sensor_info, 0444, mag_dhr_sensor_info_show, NULL);
#else
static DEVICE_ATTR(dhr_sensor_info, 0440, mag_dhr_sensor_info_show, NULL);
#endif

static struct device_attribute *mag_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_raw_data,
	&dev_attr_adc,
	&dev_attr_dac,
	&dev_attr_chk_registers,
	&dev_attr_selftest,
	&dev_attr_asa,
	&dev_attr_status,
	&dev_attr_dhr_sensor_info,
	NULL,
};

static int __init ak09918_factory_init(void)
{
	adsp_factory_register(MSG_MAG, mag_attrs);

	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

static void __exit ak09918_factory_exit(void)
{
	adsp_factory_unregister(MSG_MAG);

	pr_info("[FACTORY] %s\n", __func__);
}

module_init(ak09918_factory_init);
module_exit(ak09918_factory_exit);
