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

#define VENDOR "AMS"
#define CHIP_ID "TMD4906"

#define START_TEST	'0'
#define COUNT_TEST	'1'
#define REGISTER_TEST	'2'
#define DATA_TEST	'3'
#define STOP_TEST	'4'

#define MAX_COUNT 15

static u8 reg_id_table[MAX_COUNT][2] = {
	{0x81, 0}, {0x88, 1}, {0x8F, 2}, {0x96, 3}, {0x9D, 4},
	{0xA4, 5}, {0xAB, 6}, {0xB2, 7}, {0xB9, 8}, {0xC0, 9},
	{0xC7, 10}, {0xCE, 11}, {0xD5, 12}, {0xDC, 13}, {0xE3, 14}
};

enum {
	CMD_TYPE_MOBEAM_START,
	CMD_TYPE_MOBEAM_STOP,
	CMD_TYPE_MOBEAM_SEND_DATA,
	CMD_TYPE_MOBEAM_SEND_COUNT,
	CMD_TYPE_MOBEAM_SEND_REG,
};

static u8 is_beaming;

static ssize_t mobeam_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t mobeam_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

int get_mobeam_sidx(struct adsp_data *data)
{
	int ret = MSG_MOBEAM;
#ifdef CONFIG_SUPPORT_SUB_MOBEAM
	switch (data->fac_fstate) {
	case FSTATE_INACTIVE:
	case FSTATE_FAC_INACTIVE:
		ret = MSG_MOBEAM;
		break;
	case FSTATE_ACTIVE:
	case FSTATE_FAC_ACTIVE:
		ret = MSG_MOBEAM_SUB;
		break;
	default:
		break;
	}
#endif
	pr_info("[FACTORY] %s: idx:%d\n", __func__, ret);
	return ret;
}

static ssize_t barcode_emul_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t mobeam_idx = get_mobeam_sidx(data);
	uint8_t cnt = 0;
	char *send_buf;
	int len, buf_size, i;

	if (size <= 1) {
		pr_info("[FACTORY] %s - not enough size(%d)",
			__func__, (int)size);
		return size;
	}

	if (buf[0] == 0xFF && buf[1] != 0) {
		if (is_beaming)
			return size;
		is_beaming = 1;
		pr_info("[FACTORY] %s: start\n", __func__);
		adsp_unicast(NULL, 0, mobeam_idx, 0, MSG_TYPE_FACTORY_ENABLE);
		return size;
	} else if (buf[0] == 0xFF && buf[1] == 0) {
		if (is_beaming == 0)
			return size;
		is_beaming = 0;
		pr_info("[FACTORY] %s: stop\n", __func__);
		adsp_unicast(NULL, 0, mobeam_idx, 0, MSG_TYPE_FACTORY_DISABLE);
		return size;
	} else if (buf[0] == 0x00) {
		buf_size = 128;
		len = ((int)size - 2 > 128) ? 128 : ((int)size - 2);
		send_buf = kzalloc(buf_size + 1, GFP_KERNEL);
		send_buf[0] = CMD_TYPE_MOBEAM_SEND_DATA;
		memcpy(&send_buf[1], &buf[2], len);
	} else if (buf[0] == 0x80) {
		buf_size = 1;
		send_buf = kzalloc(buf_size + 1, GFP_KERNEL);
		send_buf[0] = CMD_TYPE_MOBEAM_SEND_COUNT;
		memcpy(&send_buf[1], &buf[1], buf_size);
	} else {
		if (size < 8) {
			pr_info("[FACTORY] %s - not enough size(%d)",
				__func__, (int)size);
			return size;
		}
		buf_size = 6;
		send_buf = kzalloc(buf_size + 1, GFP_KERNEL);
		send_buf[0] = CMD_TYPE_MOBEAM_SEND_REG;

		for (i = 0; i < MAX_COUNT; i++) {
			if (reg_id_table[i][0] == buf[0])
				send_buf[1] = reg_id_table[i][1];
		}
		send_buf[2] = buf[1];
		send_buf[3] = buf[2];
		send_buf[4] = buf[4];
		send_buf[5] = buf[5];
		send_buf[6] = buf[7];
	}

	pr_info("[FACTORY] %s: %d\n", __func__, send_buf[0]);

	adsp_unicast(send_buf, buf_size + 1,
		mobeam_idx, 0, MSG_TYPE_SET_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_SET_REGISTER] & 1 << mobeam_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_REGISTER] &= ~(1 << mobeam_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	kfree(send_buf);
	return size;
}

static ssize_t barcode_led_status_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", is_beaming);
}

static ssize_t barcode_ver_check_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", 15);
}

static ssize_t barcode_emul_test_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t size)
{
	if (buf[0] == START_TEST) {
		u8 send_buf[2] = {0xFF, 0x01};

		barcode_emul_store(dev, attr, send_buf, sizeof(send_buf));
	} else if (buf[0] == STOP_TEST) {
		u8 send_buf[2] = {0xFF, 0};

		barcode_emul_store(dev, attr, send_buf, sizeof(send_buf));
	} else if (buf[0] == COUNT_TEST) {
		u8 send_buf[2] = {0x80, 15};

		barcode_emul_store(dev, attr, send_buf, sizeof(send_buf));
	} else if (buf[0] == REGISTER_TEST) {
		u8 send_buf[8] = {0x81, 0xAC, 0xDB, 0x36,
			0x42, 0x85, 0x0A, 0xA8};

		barcode_emul_store(dev, attr, send_buf, sizeof(send_buf));
	} else {
		u8 send_buf[130] = {0x00, 0xAC, 0xDB, 0x36,
			0x42, 0x85, 0x0A, 0xA8, };

		barcode_emul_store(dev, attr, send_buf, sizeof(send_buf));
	}

	return size;
}

static DEVICE_ATTR(vendor, 0444, mobeam_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, mobeam_name_show, NULL);
static DEVICE_ATTR(barcode_send, 0220, NULL, barcode_emul_store);
static DEVICE_ATTR(barcode_led_status, 0444, barcode_led_status_show, NULL);
static DEVICE_ATTR(barcode_ver_check, 0444, barcode_ver_check_show, NULL);
static DEVICE_ATTR(barcode_test_send, 0220, NULL, barcode_emul_test_store);

static struct device_attribute *mobeam_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_barcode_send,
	&dev_attr_barcode_led_status,
	&dev_attr_barcode_ver_check,
	&dev_attr_barcode_test_send,
	NULL,
};

static int __init initialize_mobeam(void)
{
	pr_info("[FACTORY] %s\n", __func__);
	adsp_mobeam_register(mobeam_attrs);

	return 0;
}

static void __exit remove_mobeam(void)
{
	adsp_mobeam_unregister(mobeam_attrs);
	pr_info("[FACTORY] %s\n", __func__);
}

late_initcall(initialize_mobeam);
module_exit(remove_mobeam);
