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
#define VENDOR "AMS"
#ifdef CONFIG_SUPPORT_TMD4907_FACTORY
#define CHIP_ID "TMD4907"
#else
#define CHIP_ID "TMD4910"
#endif

enum {
	OPTION_TYPE_COPR_ENABLE,
	OPTION_TYPE_BOLED_ENABLE,
	OPTION_TYPE_LCD_ONOFF,
	OPTION_TYPE_GET_COPR,
	OPTION_TYPE_GET_CHIP_ID,
	OPTION_TYPE_SET_HALLIC_INFO,
	OPTION_TYPE_GET_LIGHT_CAL,
	OPTION_TYPE_SET_LIGHT_CAL,
	OPTION_TYPE_SET_LCD_VERSION,
	OPTION_TYPE_SET_UB_DISCONNECT,
	OPTION_TYPE_MAX
};

#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
#include "../../../techpack/display/msm/samsung/ss_panel_notify.h"
#endif
#ifdef CONFIG_SUPPORT_AMS_LIGHT_LCD_VERSION_DUALIZAION
#define LIGHT_LCD_TYPE_PATH "/sys/class/lcd/panel/lcd_type"
#define LIGHT_LCD_TYPE_STRING_LENGTH 11
#define LIGHT_LCD_VERSION_CHAR_NUM 9
#endif
#ifdef CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION
int ams_load_ub_cell_id_from_file(char *path, char *data_str);
int ams_save_ub_cell_id_to_efs(char *data_str, bool first_booting);
#define LIGHT_FACTORY_CAL_PATH "/efs/FactoryApp/light_factory_cal_v3"
#define LIGHT_UB_CELL_ID_PATH "/efs/FactoryApp/light_ub_cell_id_v3"
#define LIGHT_CAL_DATA_LENGTH 4
#define LIGHT_CAL_PASS 1
#define LIGHT_CAL_FAIL 0
#endif

/*************************************************************************/
/* factory Sysfs							 */
/*************************************************************************/
static ssize_t light_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t light_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

int get_light_sidx(struct adsp_data *data)
{
	int ret = MSG_LIGHT;
#ifdef CONFIG_SUPPORT_DUAL_OPTIC
	switch (data->fac_fstate) {
	case FSTATE_INACTIVE:
	case FSTATE_FAC_INACTIVE:
	case FSTATE_FAC_INACTIVE_2:
		ret = MSG_LIGHT;
		break;
	case FSTATE_ACTIVE:
	case FSTATE_FAC_ACTIVE:
		ret = MSG_LIGHT_SUB;
		break;
	default:
		break;
	}
#endif
	return ret;
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
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
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
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	mutex_unlock(&data->light_factory_mutex);
	return data->msg_buf[light_idx][0];
}

static ssize_t light_register_read_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);
	int cnt = 0;
	int32_t msg_buf[1];

	msg_buf[0] = data->temp_reg;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_GET_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_GET_REGISTER] & 1 << light_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_REGISTER] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	pr_info("[FACTORY] %s: [0x%x]: 0x%x\n",
		__func__, msg_buf[0], data->msg_buf[light_idx][0]);

	mutex_unlock(&data->light_factory_mutex);

	return snprintf(buf, PAGE_SIZE, "[0x%x]: 0x%x\n",
		msg_buf[0], data->msg_buf[light_idx][0]);
}

static ssize_t light_register_read_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int reg = 0;
	struct adsp_data *data = dev_get_drvdata(dev);

	if (sscanf(buf, "%3x", &reg) != 1) {
		pr_err("[FACTORY]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	data->temp_reg = reg;
	pr_info("[FACTORY] %s: [0x%x]\n", __func__, data->temp_reg);

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
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

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

#ifdef CONFIG_SUPPORT_SSC_AOD_RECT
static ssize_t light_set_aod_rect_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int32_t msg_buf[5] = {OPTION_TYPE_SSC_AOD_RECT, 0, 0, 0, 0};

	if (sscanf(buf, "%3d,%3d,%3d,%3d",
		&msg_buf[1], &msg_buf[2], &msg_buf[3], &msg_buf[4]) != 4) {
		pr_err("[FACTORY]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s: rect:%d,%d,%d,%d \n", __func__,
		msg_buf[1], msg_buf[2], msg_buf[3], msg_buf[4]);
	adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
	return size;
}
#endif

int light_panel_data_notify(struct notifier_block *nb,
	unsigned long val, void *v)
{
	static int32_t pre_bl_level = -1;
	struct adsp_data *data = adsp_get_struct_data();

	if (val == PANEL_EVENT_BL_CHANGED) {
		struct panel_bl_event_data *panel_data = v;
		int32_t brightness_data[2] = {0, };

#if defined(CONFIG_SEC_C1Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
		brightness_data[0] = panel_data->bl_level;
#else
		brightness_data[0] = panel_data->bl_level / 100;
#endif
		brightness_data[1] = panel_data->aor_data;
		data->brightness_info[0] = brightness_data[0];
		data->brightness_info[1] = brightness_data[1];

		if (brightness_data[0] == pre_bl_level)
			return 0;

		pre_bl_level = brightness_data[0];
#ifdef CONFIG_SUPPORT_DUAL_OPTIC
		adsp_unicast(brightness_data, sizeof(brightness_data),
			MSG_VIR_OPTIC, 0, MSG_TYPE_SET_CAL_DATA);
#ifdef CONFIG_SUPPORT_DUAL_DDI_COPR_FOR_LIGHT_SENSOR
		adsp_unicast(brightness_data, sizeof(brightness_data),
			MSG_DDI, 0, MSG_TYPE_SET_CAL_DATA);
#endif
#else
		schedule_work(&data->light_br_work);
#endif
		pr_info("[FACTORY] %s: %d, %d\n", __func__,
			brightness_data[0], brightness_data[1]);
	} else if (val == PANEL_EVENT_UB_CON_CHANGED) {
		struct panel_ub_con_event_data *panel_data = v;
		uint16_t light_idx = get_light_sidx(data);
		int32_t msg_buf[2];

		msg_buf[0] = OPTION_TYPE_SET_UB_DISCONNECT;
		msg_buf[1] = (int32_t)panel_data->state;

		pr_info("[FACTORY] %s: ub disconnected %d\n",
			__func__, msg_buf[1]);
		adsp_unicast(msg_buf, sizeof(msg_buf),
			light_idx, 0, MSG_TYPE_OPTION_DEFINE);
	}

	return 0;
}

static struct notifier_block light_panel_data_notifier = {
	.notifier_call = light_panel_data_notify,
	.priority = 1,
};
#endif /* CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR */

#ifdef CONFIG_SUPPORT_UNDER_PANEL_WITH_LIGHT_SENSOR
static ssize_t light_hallic_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
#ifndef CONFIG_SEC_FACTORY
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);
	int32_t msg_buf[2];
#endif
	int new_value;

	if (sysfs_streq(buf, "0"))
		new_value = 0;
	else if (sysfs_streq(buf, "1"))
		new_value = 1;
	else
		return size;

	pr_info("[FACTORY] %s: new_value %d\n", __func__, new_value);

#ifndef CONFIG_SEC_FACTORY
	msg_buf[0] = OPTION_TYPE_SET_HALLIC_INFO;
	msg_buf[1] = new_value;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_OPTION_DEFINE);
	mutex_unlock(&data->light_factory_mutex);
#endif
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

#if defined(CONFIG_SEC_C1Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
	if (new_value == 1) {
		schedule_delayed_work(&data->light_copr_debug_work,
			msecs_to_jiffies(1000));
		data->light_copr_debug_count = 0;
	} else {
		cancel_delayed_work_sync(&data->light_copr_debug_work);
	}
#endif

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_OPTION_DEFINE);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
#ifdef CONFIG_SUPPORT_DUAL_DDI_COPR_FOR_LIGHT_SENSOR
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_DDI, 0, MSG_TYPE_OPTION_DEFINE);
#endif
	mutex_unlock(&data->light_factory_mutex);

	return size;
}

static ssize_t light_circle_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_SEC_D1Q_PROJECT) || defined(CONFIG_SEC_D1XQ_PROJECT)
	return snprintf(buf, PAGE_SIZE, "41.3 7.1 2.4\n");
#elif defined(CONFIG_SEC_D2Q_PROJECT) || defined(CONFIG_SEC_D2XQ_PROJECT)
	return snprintf(buf, PAGE_SIZE, "43.8 6.7 2.4\n");
#elif defined(CONFIG_SEC_X1Q_PROJECT)
	return snprintf(buf, PAGE_SIZE, "45.1 8.2 2.4\n");
#elif defined(CONFIG_SEC_Y2Q_PROJECT)
	return snprintf(buf, PAGE_SIZE, "42.6 8.0 2.4\n");
#elif defined(CONFIG_SEC_Z3Q_PROJECT)
	return snprintf(buf, PAGE_SIZE, "44.0 8.0 2.4\n");
#elif defined(CONFIG_SEC_C1Q_PROJECT)
	return snprintf(buf, PAGE_SIZE, "42.3 7.8 2.5\n");
#elif defined(CONFIG_SEC_C2Q_PROJECT)
	return snprintf(buf, PAGE_SIZE, "45.2 7.3 2.5\n");
#elif defined(CONFIG_SEC_BLOOMXQ_PROJECT)
	return snprintf(buf, PAGE_SIZE, "34.1 11.6 2.4\n");
#elif defined(CONFIG_SEC_F2Q_PROJECT)
#ifdef CONFIG_SUPPORT_DUAL_OPTIC
	struct adsp_data *data = dev_get_drvdata(dev);
	if (data->fac_fstate == FSTATE_INACTIVE || data->fac_fstate == FSTATE_FAC_INACTIVE || data->fac_fstate == FSTATE_FAC_INACTIVE_2)
		return snprintf(buf, PAGE_SIZE, "78.226 4.775 1.9 19.569 143.246 2.4\n");
	else
		return snprintf(buf, PAGE_SIZE, "45.647 4.801 3\n");
#else
	return snprintf(buf, PAGE_SIZE, "78.226 4.775 1.9\n");
#endif

#else
	return snprintf(buf, PAGE_SIZE, "0 0 0\n");
#endif
}
#endif /* CONFIG_SUPPORT_UNDER_PANEL_WITH_LIGHT_SENSOR */

#ifdef CONFIG_SUPPORT_DDI_COPR_FOR_LIGHT_SENSOR
static ssize_t light_read_copr_store(struct device *dev,
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
	msg_buf[0] = OPTION_TYPE_COPR_ENABLE;
	msg_buf[1] = new_value;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_OPTION_DEFINE);
	mutex_unlock(&data->light_factory_mutex);

	return size;
}

static ssize_t light_read_copr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t cmd = OPTION_TYPE_GET_COPR;
	uint16_t light_idx = get_light_sidx(data);
	uint8_t cnt = 0;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(&cmd, sizeof(cmd), light_idx, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << light_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		mutex_unlock(&data->light_factory_mutex);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s: %d\n", __func__, data->msg_buf[light_idx][4]);
	mutex_unlock(&data->light_factory_mutex);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->msg_buf[light_idx][4]);
}

static ssize_t light_copr_roix_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t light_idx = get_light_sidx(data);
	uint8_t cnt = 0;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(NULL, 0, light_idx, 0, MSG_TYPE_GET_DUMP_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_GET_DUMP_REGISTER] & 1 << light_idx)
		&& cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DUMP_REGISTER] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		mutex_unlock(&data->light_factory_mutex);
		return snprintf(buf, PAGE_SIZE, "-1,-1,-1,-1\n");
	}

	pr_info("[FACTORY] %s: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
		data->msg_buf[light_idx][0], data->msg_buf[light_idx][1],
		data->msg_buf[light_idx][2], data->msg_buf[light_idx][3],
		data->msg_buf[light_idx][4], data->msg_buf[light_idx][5],
		data->msg_buf[light_idx][6], data->msg_buf[light_idx][7],
		data->msg_buf[light_idx][8], data->msg_buf[light_idx][9],
		data->msg_buf[light_idx][10], data->msg_buf[light_idx][11]);

	mutex_unlock(&data->light_factory_mutex);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[light_idx][0], data->msg_buf[light_idx][1],
		data->msg_buf[light_idx][2], data->msg_buf[light_idx][3],
		data->msg_buf[light_idx][4], data->msg_buf[light_idx][5],
		data->msg_buf[light_idx][6], data->msg_buf[light_idx][7],
		data->msg_buf[light_idx][8], data->msg_buf[light_idx][9],
		data->msg_buf[light_idx][10], data->msg_buf[light_idx][11]);
}

static ssize_t light_test_copr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t cmd = OPTION_TYPE_GET_COPR;
	uint16_t light_idx = get_light_sidx(data);
	uint8_t cnt = 0;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(&cmd, sizeof(cmd), light_idx, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << light_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		mutex_unlock(&data->light_factory_mutex);
		return snprintf(buf, PAGE_SIZE, "-1,-1,-1,-1\n");
	}

	pr_info("[FACTORY] %s: %d,%d,%d,%d\n", __func__,
		data->msg_buf[light_idx][0], data->msg_buf[light_idx][1],
		data->msg_buf[light_idx][2], data->msg_buf[light_idx][3]);

	mutex_unlock(&data->light_factory_mutex);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n",
		data->msg_buf[light_idx][0], data->msg_buf[light_idx][1],
		data->msg_buf[light_idx][2], data->msg_buf[light_idx][3]);
}

static ssize_t light_ddi_spi_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t cmd = OPTION_TYPE_GET_CHIP_ID;
	uint16_t light_idx = get_light_sidx(data);
	uint8_t cnt = 0;

#ifdef CONFIG_SUPPORT_DUAL_DDI_COPR_FOR_LIGHT_SENSOR
	light_idx = MSG_DDI;
#endif
	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(&cmd, sizeof(cmd), light_idx, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << light_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		mutex_unlock(&data->light_factory_mutex);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s: %d\n", __func__, data->msg_buf[light_idx][0]);

	mutex_unlock(&data->light_factory_mutex);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->msg_buf[light_idx][0]);
}

static ssize_t light_boled_enable_store(struct device *dev,
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
	msg_buf[0] = OPTION_TYPE_BOLED_ENABLE;
	msg_buf[1] = new_value;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_OPTION_DEFINE);
	mutex_unlock(&data->light_factory_mutex);

	return size;
}
#endif /* CONFIG_SUPPORT_DDI_COPR_FOR_LIGHT_SENSOR */

#ifdef CONFIG_SUPPORT_AMS_LIGHT_LCD_VERSION_DUALIZAION
void light_lcd_version_dualization(struct adsp_data *data)
{
	struct file *file_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0, lcd_ver = -1;
	int32_t msg_buf[2];
	char *ver_str = kzalloc(LIGHT_LCD_TYPE_STRING_LENGTH, GFP_KERNEL);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	file_filp = filp_open(LIGHT_LCD_TYPE_PATH, O_RDONLY, 0440);

	if (IS_ERR(file_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(file_filp);
		pr_err("[FACTORY] %s: open fail:%d\n", __func__, ret);
	} else {
		ret = vfs_read(file_filp, (char *)ver_str,
			sizeof(char) * LIGHT_LCD_TYPE_STRING_LENGTH,
			&file_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s: fd read fail(%s): %d\n",
				__func__, ret);
		} else {
			pr_info("[FACTORY] %s: data: %s\n", __func__, ver_str);
			lcd_ver = ver_str[LIGHT_LCD_VERSION_CHAR_NUM] - '0';
		}
		filp_close(file_filp, current->files);
		set_fs(old_fs);
	}
	kfree(ver_str);

	mutex_lock(&data->light_factory_mutex);
	msg_buf[0] = OPTION_TYPE_SET_LCD_VERSION;
	msg_buf[1] = lcd_ver;
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_LIGHT, 0, MSG_TYPE_OPTION_DEFINE);
	mutex_unlock(&data->light_factory_mutex);
	pr_info("[FACTORY] %s: lcd version: %d\n", __func__, lcd_ver);
}
#endif /* CONFIG_SUPPORT_AMS_LIGHT_LCD_VERSION_DUALIZAION */

#ifdef CONFIG_SUPPORT_SSC_AOD_RECT
void light_rect_init_work(void)
{
	int32_t rect_msg[5] = {OPTION_TYPE_SSC_AOD_LIGHT_CIRCLE, 546, 170, 576, 200};
	adsp_unicast(rect_msg, sizeof(rect_msg),
		MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
}
#endif

#ifdef CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION
int light_get_cal_data(int32_t *cal_data)
{
	struct file *factory_cal_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	factory_cal_filp = filp_open(LIGHT_FACTORY_CAL_PATH, O_RDONLY, 0440);

	if (IS_ERR(factory_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(factory_cal_filp);
		pr_err("[FACTORY] %s: open fail light_factory_cal:%d\n",
			__func__, ret);
		return ret;
	}

	ret = vfs_read(factory_cal_filp, (char *)cal_data,
		sizeof(int32_t) * LIGHT_CAL_DATA_LENGTH,
		&factory_cal_filp->f_pos);
	if (ret < 0)
		pr_err("[FACTORY] %s: fd read fail:%d\n", __func__, ret);

	filp_close(factory_cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

int light_set_cal_data(int32_t *cal_data, bool first_booting)
{
	struct file *factory_cal_filp = NULL;
	mm_segment_t old_fs;
	int flag, ret = 0;
	umode_t mode = 0;

	if (cal_data[0] < 0) {
		pr_err("[FACTORY] %s: cal value error: %d, %d\n",
			__func__, cal_data[0], cal_data[1]);
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (first_booting) {
		flag = O_TRUNC | O_RDWR | O_CREAT;
		mode = 0600;
	} else {
		flag = O_RDWR;
		mode = 0660;
	}

	factory_cal_filp = filp_open(LIGHT_FACTORY_CAL_PATH, flag, mode);

	if (IS_ERR(factory_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(factory_cal_filp);
		pr_err("[FACTORY] %s: open fail light_factory_cal:%d\n",
			__func__, ret);
		return ret;
	}

	ret = vfs_write(factory_cal_filp, (char *)cal_data,
		sizeof(int32_t) * LIGHT_CAL_DATA_LENGTH,
		&factory_cal_filp->f_pos);
	if (ret < 0)
		pr_err("[FACTORY] %s: fd write %d\n", __func__, ret);

	filp_close(factory_cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

int ams_load_ub_cell_id_from_file(char *path, char *data_str)
{
	struct file *file_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	file_filp = filp_open(path, O_RDONLY, 0440);

	if (IS_ERR(file_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(file_filp);
		pr_err("[FACTORY] %s: open fail(%s):%d\n", __func__, path, ret);
		return ret;
	}

	ret = vfs_read(file_filp, (char *)data_str,
		sizeof(char) * AMS_UB_CELL_ID_INFO_STRING_LENGTH,
		&file_filp->f_pos);
	if (ret < 0)
		pr_err("[FACTORY] %s: fd read fail(%s): %d\n",
			__func__, path, ret);
	else
		pr_info("[FACTORY] %s: data(%s): %s\n",
			__func__, path, data_str);

	filp_close(file_filp, current->files);
	set_fs(old_fs);

	return ret;
}

int ams_save_ub_cell_id_to_efs(char *data_str, bool first_booting)
{
	struct file *file_filp = NULL;
	mm_segment_t old_fs;
	int flag, ret = 0;
	umode_t mode = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (first_booting) {
		flag = O_TRUNC | O_RDWR | O_CREAT;
		mode = 0600;
	} else {
		flag = O_RDWR;
		mode = 0660;
	}

	file_filp = filp_open(LIGHT_UB_CELL_ID_PATH, flag, mode);
	if (IS_ERR(file_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(file_filp);
		pr_err("[FACTORY] %s: open fail light_factory_cal(%s): %d\n",
			__func__, LIGHT_UB_CELL_ID_PATH, ret);
		return ret;
	}

	ret = vfs_write(file_filp, (char *)data_str,
		sizeof(char) * AMS_UB_CELL_ID_INFO_STRING_LENGTH,
		&file_filp->f_pos);
	if (ret < 0)
		pr_err("[FACTORY] %s: fd write fail(%s): %d\n",
			__func__, LIGHT_UB_CELL_ID_PATH, ret);

	filp_close(file_filp, current->files);
	set_fs(old_fs);

	pr_info("[FACTORY] %s(%s): %s\n", __func__,
		LIGHT_UB_CELL_ID_PATH, data_str);

	return ret;
}

#if defined(CONFIG_SEC_C1Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
void light_copr_debug_work_func(struct work_struct *work)
{
	struct adsp_data *data = container_of((struct delayed_work *)work,
		struct adsp_data, light_copr_debug_work);
	uint16_t light_idx = get_light_sidx(data);
	uint8_t cnt = 0;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(NULL, 0, light_idx, 0, MSG_TYPE_GET_DUMP_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_GET_DUMP_REGISTER] & 1 << light_idx)
		&& cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DUMP_REGISTER] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[SSC_FAC] %s: Timeout!!!\n", __func__);
		mutex_unlock(&data->light_factory_mutex);
		return;
	}

	pr_info("[SSC_FAC] %s: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
		data->msg_buf[light_idx][0], data->msg_buf[light_idx][1],
		data->msg_buf[light_idx][2], data->msg_buf[light_idx][3],
		data->msg_buf[light_idx][4], data->msg_buf[light_idx][5],
		data->msg_buf[light_idx][6], data->msg_buf[light_idx][7],
		data->msg_buf[light_idx][8], data->msg_buf[light_idx][9],
		data->msg_buf[light_idx][10], data->msg_buf[light_idx][11]);

	mutex_unlock(&data->light_factory_mutex);

	if (data->light_copr_debug_count++ < 5)
		schedule_delayed_work(&data->light_copr_debug_work,
			msecs_to_jiffies(1000));
}
#endif

void light_cal_read_work_func(struct work_struct *work)
{
	struct adsp_data *data = container_of((struct delayed_work *)work,
		struct adsp_data, light_cal_work);
	int32_t msg_buf[LIGHT_CAL_DATA_LENGTH];
	char *cur_data_str = kzalloc(AMS_UB_CELL_ID_INFO_STRING_LENGTH,
				GFP_KERNEL);

#ifdef CONFIG_SUPPORT_AMS_LIGHT_LCD_VERSION_DUALIZAION
	light_lcd_version_dualization(data);
	msleep(30);
#endif

	ams_load_ub_cell_id_from_file(UB_CELL_ID_PATH, cur_data_str);
	if (strcmp(data->light_ub_id, cur_data_str) == 0) {
		pr_info("[FACTORY] %s : UB is matched\n", __func__);
	} else {
		pr_info("[FACTORY] %s : UB is not matched\n", __func__);
#if defined(CONFIG_SUPPORT_AMS_PROX_CALIBRATION) && !defined (CONFIG_SEC_FACTORY)
		prox_send_cal_data(data, false);
#endif
		kfree(cur_data_str);
		return;
	}
	kfree(cur_data_str);

#ifdef CONFIG_SUPPORT_AMS_PROX_CALIBRATION
	prox_send_cal_data(data, true);
#endif

	if ((data->light_cal_result == LIGHT_CAL_PASS) &&
		(data->light_cal1 >= 0) && (data->light_cal2 >= 0)) {
		mutex_lock(&data->light_factory_mutex);
		msg_buf[0] = OPTION_TYPE_SET_LIGHT_CAL;
		msg_buf[1] = data->light_cal1;
		msg_buf[2] = data->light_cal2;
		msg_buf[3] = data->copr_w;
		adsp_unicast(msg_buf, sizeof(msg_buf),
				MSG_LIGHT, 0, MSG_TYPE_OPTION_DEFINE);
		mutex_unlock(&data->light_factory_mutex);
		pr_info("[FACTORY] %s: Cal1: %d, Cal2: %d, COPR_W: %d)\n",
			__func__, msg_buf[1], msg_buf[2], msg_buf[3]);
	}
}

void light_cal_init_work(struct adsp_data *data)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int32_t init_data[LIGHT_CAL_DATA_LENGTH] = {LIGHT_CAL_FAIL, -1, -1, -1};
	int ret = 0;
	char *temp_str;

	data->light_cal_result = LIGHT_CAL_FAIL;
	data->light_cal1 = -1;
	data->light_cal2 = -1;
	data->copr_w = -1;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(LIGHT_FACTORY_CAL_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s : no light_factory_cal file\n", __func__);
		light_set_cal_data(init_data, true);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error\n", __func__);
	} else {
		ret = vfs_read(cal_filp, (char *)init_data,
			sizeof(int32_t) * LIGHT_CAL_DATA_LENGTH,
			&cal_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s: fd read fail:%d\n",
				__func__, ret);
		} else {
			pr_info("[FACTORY] %s : already exist (P/F: %d, Cal1: %d, Cal2: %d, COPR_W: %d)\n",
				__func__, init_data[0], init_data[1],
				init_data[2], init_data[3]);
			data->light_cal_result = init_data[0];
			data->light_cal1 = init_data[1];
			data->light_cal2 = init_data[2];
			data->copr_w = init_data[3];
		}
		filp_close(cal_filp, current->files);
	}

	cal_filp = filp_open(LIGHT_UB_CELL_ID_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s : no light_ub_efs file\n", __func__);
		temp_str = kzalloc(AMS_UB_CELL_ID_INFO_STRING_LENGTH,
			GFP_KERNEL);
		ams_save_ub_cell_id_to_efs(temp_str, true);
		kfree(temp_str);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - ub filp_open error\n", __func__);
	} else {
		ret = vfs_read(cal_filp, (char *)data->light_ub_id,
			sizeof(char) * AMS_UB_CELL_ID_INFO_STRING_LENGTH,
			&cal_filp->f_pos);
		if (ret < 0)
			pr_err("[FACTORY] %s: ub read fail:%d\n",
				__func__, ret);
		else
			pr_info("[FACTORY] %s : already ub efs exist %s\n",
				__func__, data->light_ub_id);

		filp_close(cal_filp, current->files);
	}

	set_fs(old_fs);
	schedule_delayed_work(&data->light_cal_work, msecs_to_jiffies(8000));
}

static ssize_t light_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t msg_buf[LIGHT_CAL_DATA_LENGTH], cnt = 0;
	int32_t cal_data[LIGHT_CAL_DATA_LENGTH] = {LIGHT_CAL_FAIL, -1, -1, -1};
	int32_t cmd = OPTION_TYPE_GET_LIGHT_CAL, cur_lux, ret;
	uint16_t light_idx = get_light_sidx(data);

	ret = light_get_cal_data(cal_data);

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(&cmd, sizeof(cmd), light_idx, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA]
		& 1 << light_idx) && cnt++ < TIMEOUT_CNT)
		usleep_range(1000, 1100);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		cur_lux = -1;
	} else {
		cur_lux = data->msg_buf[light_idx][4];
	}

	msg_buf[0] = OPTION_TYPE_SET_LIGHT_CAL;
	msg_buf[1] = cal_data[1];
	msg_buf[2] = cal_data[2];
	msg_buf[3] = cal_data[3];

	if ((ret >= 0) && (cal_data[0] == LIGHT_CAL_PASS) && (cal_data[1] >= 0)
		&& (cal_data[2] >= 0))
		adsp_unicast(msg_buf, sizeof(msg_buf),
			light_idx, 0, MSG_TYPE_OPTION_DEFINE);
	mutex_unlock(&data->light_factory_mutex);

	pr_info("[FACTORY] %s: cal_data (P/F: %d, Cal1: %d, Cal2: %d, COPR_W: %d, cur lux: %d)\n",
		__func__, cal_data[0], cal_data[1],
		cal_data[2], cal_data[3], cur_lux);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		LIGHT_CAL_PASS, cal_data[2], cur_lux);
}

static ssize_t light_cal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t cmd = OPTION_TYPE_GET_LIGHT_CAL, new_value;
	int32_t cal_data[LIGHT_CAL_DATA_LENGTH] = {LIGHT_CAL_FAIL, -1, -1, -1};
	int32_t msg_buf[LIGHT_CAL_DATA_LENGTH] = {0, 0, 0, 0};
	uint16_t light_idx = get_light_sidx(data);
	uint8_t cnt = 0;
	char *cur_id_str;

	if (sysfs_streq(buf, "0"))
		new_value = 0;
	else if (sysfs_streq(buf, "1"))
		new_value = 1;
	else
		return size;

	pr_info("[FACTORY] %s: cmd: %d\n", __func__, new_value);

	if (new_value == 1) {
		mutex_lock(&data->light_factory_mutex);
		adsp_unicast(&cmd, sizeof(cmd), light_idx, 0,
			MSG_TYPE_GET_CAL_DATA);

		while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA]
			& 1 << light_idx) && cnt++ < TIMEOUT_CNT)
			usleep_range(1000, 1100);

		data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << light_idx);
		mutex_unlock(&data->light_factory_mutex);

		if (cnt >= TIMEOUT_CNT) {
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
			return size;
		}

		cal_data[0] = data->msg_buf[light_idx][0];
		cal_data[1] = data->msg_buf[light_idx][1];
		cal_data[2] = data->msg_buf[light_idx][2];
		cal_data[3] = data->msg_buf[light_idx][3];

		pr_info("[FACTORY] %s: (P/F: %d, Cal1: %d, Cal2: %d, COPR_W: %d)\n",
			__func__, data->msg_buf[light_idx][0],
			data->msg_buf[light_idx][1],
			data->msg_buf[light_idx][2],
			data->msg_buf[light_idx][3]);

		if (cal_data[0] == LIGHT_CAL_PASS) {
			data->light_cal_result = LIGHT_CAL_PASS;
			data->light_cal1 = msg_buf[1] = cal_data[1];
			data->light_cal2 = msg_buf[2] = cal_data[2];
			data->copr_w = msg_buf[3] = cal_data[3];
		} else {
			return size;
		}

		cur_id_str = kzalloc(AMS_UB_CELL_ID_INFO_STRING_LENGTH,
			GFP_KERNEL);
		ams_load_ub_cell_id_from_file(UB_CELL_ID_PATH, cur_id_str);
		ams_save_ub_cell_id_to_efs(cur_id_str, false);
		kfree(cur_id_str);
	} else {
		mutex_lock(&data->light_factory_mutex);
		adsp_unicast(data->brightness_info,
			sizeof(data->brightness_info),
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

		data->light_cal_result = cal_data[0] = LIGHT_CAL_FAIL;
		data->light_cal1 = cal_data[1] = msg_buf[1] = 0;
		data->light_cal2 = cal_data[2] = msg_buf[2] = 0;
		data->copr_w = cal_data[3] = msg_buf[3] = 0;
	}

	light_set_cal_data(cal_data, false);

	mutex_lock(&data->light_factory_mutex);
	msg_buf[0] = OPTION_TYPE_SET_LIGHT_CAL;
	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_OPTION_DEFINE);
	mutex_unlock(&data->light_factory_mutex);

	return size;
}

static ssize_t light_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t cnt = 0, cmd = OPTION_TYPE_GET_LIGHT_CAL;
	int32_t test_value;
	uint16_t light_idx = get_light_sidx(data);

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(&cmd, sizeof(cmd), light_idx, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA]
		& 1 << light_idx) && cnt++ < TIMEOUT_CNT)
		usleep_range(1000, 1100);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << light_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		test_value = -1;
	} else {
		test_value = data->msg_buf[light_idx][2];
	}

	mutex_unlock(&data->light_factory_mutex);

	pr_info("[FACTORY] %s: test_data (Cal1: %d, Cal2: %d, COPR_W: %d, 16ms lux: %d)\n",
		__func__, data->light_cal1, data->light_cal2,
		data->copr_w, test_value);

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d\n",
		data->light_cal1, data->light_cal2, data->copr_w, test_value);
}

static DEVICE_ATTR(light_cal, 0664, light_cal_show, light_cal_store);
static DEVICE_ATTR(light_test, 0444, light_test_show, NULL);
#endif /* CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION */

#ifdef CONFIG_SUPPORT_UNDER_PANEL_WITH_LIGHT_SENSOR
static DEVICE_ATTR(lcd_onoff, 0220, NULL, light_lcd_onoff_store);
static DEVICE_ATTR(hallic_info, 0220, NULL, light_hallic_info_store);
static DEVICE_ATTR(light_circle, 0444, light_circle_show, NULL);
#endif
#ifdef CONFIG_SUPPORT_DDI_COPR_FOR_LIGHT_SENSOR
static DEVICE_ATTR(read_copr, 0664,
		light_read_copr_show, light_read_copr_store);
static DEVICE_ATTR(test_copr, 0444, light_test_copr_show, NULL);
static DEVICE_ATTR(boled_enable, 0220, NULL, light_boled_enable_store);
static DEVICE_ATTR(copr_roix, 0444, light_copr_roix_show, NULL);
static DEVICE_ATTR(sensorhub_ddi_spi_check, 0444,
		light_ddi_spi_check_show, NULL);
#endif
static DEVICE_ATTR(register_write, 0220, NULL, light_register_write_store);
static DEVICE_ATTR(register_read, 0664,
		light_register_read_show, light_register_read_store);
static DEVICE_ATTR(vendor, 0444, light_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, light_name_show, NULL);
static DEVICE_ATTR(lux, 0444, light_raw_data_show, NULL);
static DEVICE_ATTR(raw_data, 0444, light_raw_data_show, NULL);
static DEVICE_ATTR(dhr_sensor_info, 0444, light_get_dhr_sensor_info_show, NULL);
#ifdef CONFIG_SUPPORT_SSC_AOD_RECT
static DEVICE_ATTR(set_aod_rect, 0220, NULL, light_set_aod_rect_store);
#endif

static struct device_attribute *light_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_lux,
	&dev_attr_raw_data,
	&dev_attr_dhr_sensor_info,
	&dev_attr_register_write,
	&dev_attr_register_read,
#ifdef CONFIG_SUPPORT_UNDER_PANEL_WITH_LIGHT_SENSOR
	&dev_attr_lcd_onoff,
	&dev_attr_hallic_info,
	&dev_attr_light_circle,
#endif
#ifdef CONFIG_SUPPORT_DDI_COPR_FOR_LIGHT_SENSOR
	&dev_attr_read_copr,
	&dev_attr_test_copr,
	&dev_attr_boled_enable,
	&dev_attr_copr_roix,
	&dev_attr_sensorhub_ddi_spi_check,
#endif
#ifdef CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION
	&dev_attr_light_cal,
	&dev_attr_light_test,
#endif
#ifdef CONFIG_SUPPORT_SSC_AOD_RECT
	&dev_attr_set_aod_rect,
#endif
	NULL,
};

static int __init tmd490x_light_factory_init(void)
{
	adsp_factory_register(MSG_LIGHT, light_attrs);
#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
	ss_panel_notifier_register(&light_panel_data_notifier);
#endif
	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

static void __exit tmd490x_light_factory_exit(void)
{
	adsp_factory_unregister(MSG_LIGHT);
#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
	ss_panel_notifier_unregister(&light_panel_data_notifier);
#endif
	pr_info("[FACTORY] %s\n", __func__);
}
module_init(tmd490x_light_factory_init);
module_exit(tmd490x_light_factory_exit);
