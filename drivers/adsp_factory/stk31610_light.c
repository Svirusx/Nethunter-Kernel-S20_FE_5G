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

#define STK31610 's'
#define TCS3407 'T'

#define VENDOR_STK31610 "Sensortek"
#define CHIP_ID_STK31610 "STK31610"
#define VENDOR_TCS3407 "AMS"
#define CHIP_ID_TCS3407 "TCS3407"


#define ASCII_TO_DEC(x) (x - 48)

int brightness;
int chip_id = 0;// 0 : STK31610, 1 : TCS3407

enum {
	OPTION_TYPE_COPR_ENABLE,
	OPTION_TYPE_BOLED_ENABLE,
	OPTION_TYPE_LCD_ONOFF,
	OPTION_TYPE_GET_COPR,
	OPTION_TYPE_GET_CHIP_ID,
	OPTION_TYPE_MAX
};

#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
#include "../../../techpack/display/msm/samsung/ss_panel_notify.h"
#endif

/*************************************************************************/
/* factory Sysfs							 */
/*************************************************************************/
static ssize_t light_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[10] = {0,};

	if (chip_id == 1)
		strcpy(temp, VENDOR_TCS3407);
	else
		strcpy(temp, VENDOR_STK31610);

	return snprintf(buf, PAGE_SIZE, "%s\n", temp);
}

static ssize_t light_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[10] = {0,};

	if (chip_id == 1)
		strcpy(temp, CHIP_ID_TCS3407);
	else
		strcpy(temp, CHIP_ID_STK31610);

	return snprintf(buf, PAGE_SIZE, "%s\n", temp);
}

void light_set_name_vendor(int32_t buf)
{
	pr_info("[FACTORY] %s: name = %d\n", __func__, buf);
	if (buf == STK31610)
		chip_id = 0;
	else if (buf == TCS3407)
		chip_id = 1;
	else
		pr_info("[FACTORY] %s: wrong sensor name %d\n", __func__, buf);
}

int get_light_sidx(struct adsp_data *data)
{
	return MSG_LIGHT;
}

static ssize_t light_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);
	uint8_t cnt = 0;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(NULL, 0, light_idx, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << light_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout\n", __func__);
		mutex_unlock(&data->light_factory_mutex);
		return snprintf(buf, PAGE_SIZE, "0,0,0,0,0,0\n");
	}

	mutex_unlock(&data->light_factory_mutex);
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[light_idx][0], data->msg_buf[light_idx][1],
		data->msg_buf[light_idx][2], data->msg_buf[light_idx][3],
		data->msg_buf[light_idx][4], data->msg_buf[light_idx][5]);
}

static ssize_t light_get_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s: start\n", __func__);
	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(NULL, 0, light_idx, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << light_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout\n", __func__);

	mutex_unlock(&data->light_factory_mutex);
	return data->msg_buf[light_idx][0];
}

static ssize_t light_brightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: %d\n", __func__, brightness);
	return snprintf(buf, PAGE_SIZE, "%d\n", brightness);
}

static ssize_t light_brightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);

	brightness = ASCII_TO_DEC(buf[0]) * 100 + ASCII_TO_DEC(buf[1]) * 10 + ASCII_TO_DEC(buf[2]);
	pr_info("[FACTORY] %s: %d\n", __func__, brightness);

	adsp_unicast(&brightness, sizeof(brightness), light_idx, 0, MSG_TYPE_SET_CAL_DATA);

	pr_info("[FACTORY] %s: done\n", __func__);

	return size;
}

static ssize_t light_lcd_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);
	int32_t msg_buf[2];
	int new_value;

	if (sysfs_streq(buf, "0"))
		new_value = 0;
	else if (sysfs_streq(buf, "1"))
		new_value = 1;
	else
		return size;

	pr_info("[FACTORY] %s: new_value %d\n", __func__, new_value);
	msg_buf[0] = OPTION_TYPE_LCD_ONOFF;
	msg_buf[1] = new_value;

	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_OPTION_DEFINE);

	pr_info("[FACTORY] %s: done\n", __func__);

	return size;
}

static ssize_t light_circle_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_SEC_R8Q_PROJECT)
	return snprintf(buf, PAGE_SIZE, "25.80 2.83 2.3\n");
#else
	return snprintf(buf, PAGE_SIZE, "0 0 0\n");
#endif
}

static ssize_t light_register_read_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);
	int cnt = 0;
	int32_t msg_buf[1];

	msg_buf[0] = data->msg_buf[light_idx][MSG_LIGHT_MAX - 1];

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_GET_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_GET_REGISTER] & 1 << light_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_REGISTER] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout\n", __func__);

	pr_info("[FACTORY] %s: [0x%x]: 0x%x\n",
		__func__, data->msg_buf[light_idx][MSG_LIGHT_MAX - 1],
		data->msg_buf[light_idx][0]);

	mutex_unlock(&data->light_factory_mutex);

	return snprintf(buf, PAGE_SIZE, "[0x%x]: 0x%x\n",
		data->msg_buf[light_idx][MSG_LIGHT_MAX - 1],
		data->msg_buf[light_idx][0]);
}

static ssize_t light_register_read_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int reg = 0;
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);

	if (sscanf(buf, "%3x", &reg) != 1) {
		pr_err("[FACTORY]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	data->msg_buf[light_idx][MSG_LIGHT_MAX - 1] = reg;
	pr_info("[FACTORY] %s: [0x%x]\n", __func__,
		data->msg_buf[light_idx][MSG_LIGHT_MAX - 1]);

	return size;
}

static ssize_t light_register_write_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);
	int cnt = 0;
	int32_t msg_buf[2];

	if (sscanf(buf, "%3x,%3x", &msg_buf[0], &msg_buf[1]) != 2) {
		pr_err("[FACTORY]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_SET_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_SET_REGISTER] & 1 << light_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_REGISTER] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout\n", __func__);

	data->msg_buf[light_idx][MSG_LIGHT_MAX - 1] = msg_buf[0];
	pr_info("[FACTORY] %s: 0x%x - 0x%x\n",
		__func__, msg_buf[0], data->msg_buf[light_idx][0]);
	mutex_unlock(&data->light_factory_mutex);

	return size;
}

#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
void light_brightness_work_func(struct work_struct *work)
{
	struct adsp_data *data = container_of((struct work_struct*)work,
		struct adsp_data, light_br_work);
	int cnt = 0;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(data->brightness_info, sizeof(data->brightness_info),
		MSG_LIGHT, 0, MSG_TYPE_SET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_SET_CAL_DATA] & 1 << MSG_LIGHT) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_CAL_DATA] &= ~(1 << MSG_LIGHT);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	else
		pr_info("[FACTORY] %s: set br %d\n",
			__func__, data->msg_buf[MSG_LIGHT][0]);

	mutex_unlock(&data->light_factory_mutex);
}

int light_panel_data_notify(struct notifier_block *nb,
	unsigned long val, void *v)
{
	struct panel_bl_event_data *panel_data = v;
	struct adsp_data *data;
	static int32_t pre_bl_level = -1;
	static int32_t pre_bl_idx = -1;
	int32_t brightness_data[3] = {0, };

	if (val == PANEL_EVENT_BL_CHANGED) {
		data = adsp_get_struct_data();
		brightness_data[0] = panel_data->bl_level;
		brightness_data[1] = panel_data->aor_data;
		brightness_data[2] = panel_data->display_idx;
		data->brightness_info[0] = brightness_data[0];
		data->brightness_info[1] = brightness_data[1];
		data->brightness_info[2] = brightness_data[2];

		if (brightness_data[0] == pre_bl_level && brightness_data[2] == pre_bl_idx)
			return 0;

		pre_bl_level = brightness_data[0];
		pre_bl_idx = brightness_data[2];

		schedule_work(&data->light_br_work);

		pr_info("[FACTORY] %s: %d, %d, %d\n", __func__,
			brightness_data[0], brightness_data[1], brightness_data[2]);
	}

	return 0;
}

static struct notifier_block light_panel_data_notifier = {
	.notifier_call = light_panel_data_notify,
	.priority = 1,
};
#endif /* CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR */

static DEVICE_ATTR(lcd_onoff, 0220, NULL, light_lcd_onoff_store);
static DEVICE_ATTR(light_circle, 0444, light_circle_show, NULL);
static DEVICE_ATTR(register_write, 0220, NULL, light_register_write_store);
static DEVICE_ATTR(register_read, 0664,
		light_register_read_show, light_register_read_store);
static DEVICE_ATTR(vendor, 0444, light_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, light_name_show, NULL);
static DEVICE_ATTR(lux, 0444, light_raw_data_show, NULL);
static DEVICE_ATTR(raw_data, 0444, light_raw_data_show, NULL);
static DEVICE_ATTR(dhr_sensor_info, 0444, light_get_dhr_sensor_info_show, NULL);
static DEVICE_ATTR(brightness, 0664, light_brightness_show, light_brightness_store);

static struct device_attribute *light_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_lux,
	&dev_attr_raw_data,
	&dev_attr_dhr_sensor_info,
	&dev_attr_brightness,
	&dev_attr_lcd_onoff,
	&dev_attr_light_circle,
	&dev_attr_register_write,
	&dev_attr_register_read,
	NULL,
};

static int __init stk31610_light_factory_init(void)
{
	chip_id = 0;
	adsp_factory_register(MSG_LIGHT, light_attrs);
#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
	ss_panel_notifier_register(&light_panel_data_notifier);
#endif
	pr_info("[FACTORY] %s\n", __func__);

	brightness = 0;

	return 0;
}

static void __exit stk31610_light_factory_exit(void)
{
	adsp_factory_unregister(MSG_LIGHT);
#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
	ss_panel_notifier_unregister(&light_panel_data_notifier);
#endif

	pr_info("[FACTORY] %s\n", __func__);
}
module_init(stk31610_light_factory_init);
module_exit(stk31610_light_factory_exit);
