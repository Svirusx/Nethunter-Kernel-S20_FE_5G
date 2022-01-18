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
#define CHIP_ID "TMD4907"

#ifdef CONFIG_SUPPORT_AMS_PROX_CALIBRATION
#define PROX_FACTORY_CAL_PATH "/efs/FactoryApp/prox_factory_cal"
#endif
#ifdef CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION
#define LIGHT_UB_CELL_ID_PATH "/efs/FactoryApp/light_ub_cell_id_v3"
#define SUB_LIGHT_UB_CELL_ID_PATH "/efs/FactoryApp/sub_light_ub_cell_id_v3"
#endif

#define PROX_AVG_COUNT 40
#define PROX_ALERT_THRESHOLD 200
#define PROX_TH_READ 0
#define PROX_TH_WRITE 1
#define BUFFER_MAX 128
#define PROX_REG_START 0x80
#define PROX_DETECT_HIGH_TH 16368
#define PROX_DETECT_LOW_TH 1000

extern unsigned int system_rev;

struct tmd4903_prox_data {
	struct hrtimer prox_timer;
	struct work_struct work_prox;
	struct workqueue_struct *prox_wq;
	struct adsp_data *dev_data;
	int min;
	int max;
	int avg;
	int val;
	int offset;
	int reg_backup[2];
	short avgwork_check;
	short avgtimer_enabled;
	short bBarcodeEnabled;
};

enum {
	PRX_THRESHOLD_DETECT_H,
	PRX_THRESHOLD_HIGH_DETECT_L,
	PRX_THRESHOLD_HIGH_DETECT_H,
	PRX_THRESHOLD_RELEASE_L,
};

enum {
	PROX_CMD_TYPE_GET_TRIM_CHECK,
	PROX_CMD_TYPE_GET_CAL_DATA,
#ifdef CONFIG_SUPPORT_PROX_INIT_CAL
	PROX_CMD_TYPE_INIT_CAL_DATA,
#endif
	PROX_CMD_TYPE_MAX

};

static struct tmd4903_prox_data *pdata;

#ifdef CONFIG_SUPPORT_AMS_PROX_CALIBRATION
int prox_save_cal_data_to_efs(int32_t cal_data, bool first_booting)
{
	struct file *factory_cal_filp = NULL;
	mm_segment_t old_fs;
	int flag, ret = 0;
	umode_t mode = 0;

	if (cal_data < 0) {
		pr_err("[FACTORY] %s: cal value error: %d\n",
			__func__, cal_data);
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

	factory_cal_filp = filp_open(PROX_FACTORY_CAL_PATH, flag, mode);

	if (IS_ERR(factory_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(factory_cal_filp);
		pr_err("[FACTORY] %s: open fail prox_factory_cal:%d\n",
			__func__, ret);
		return ret;
	}

	ret = vfs_write(factory_cal_filp, (char *)&cal_data,
		sizeof(int32_t), &factory_cal_filp->f_pos);
	if (ret < 0)
		pr_err("[FACTORY] %s: fd write %d\n", __func__, ret);

	filp_close(factory_cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

void prox_send_cal_data(struct adsp_data *data, bool fac_cal)
{
	uint16_t prox_idx = MSG_PROX_SUB;
	int32_t msg = -1, cnt = 0;

	if (!fac_cal || (data->prox_cal == 0)) {
#ifndef CONFIG_SEC_FACTORY
		mutex_lock(&data->prox_factory_mutex);
		adsp_unicast(&msg, sizeof(int32_t),
			prox_idx, 0, MSG_TYPE_SET_CAL_DATA);
		while (!(data->ready_flag[MSG_TYPE_SET_CAL_DATA] &
			1 << prox_idx) && cnt++ < TIMEOUT_CNT)
			usleep_range(500, 550);
		if (cnt >= TIMEOUT_CNT)
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		data->ready_flag[MSG_TYPE_SET_CAL_DATA] &= ~(1 << prox_idx);
		mutex_unlock(&data->prox_factory_mutex);
		pr_info("[FACTORY] %s: Excute in-use cal: \n", __func__);
#else
		pr_info("[FACTORY] %s: No cal data (%d)\n",
			__func__, data->prox_cal);
#endif
	} else if (data->prox_cal > 0) {
		mutex_lock(&data->prox_factory_mutex);
		msg = data->prox_cal;
		adsp_unicast(&msg, sizeof(int32_t),
			prox_idx, 0, MSG_TYPE_SET_CAL_DATA);
		while (!(data->ready_flag[MSG_TYPE_SET_CAL_DATA] &
			1 << prox_idx) && cnt++ < TIMEOUT_CNT)
			usleep_range(500, 550);
		if (cnt >= TIMEOUT_CNT)
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		data->ready_flag[MSG_TYPE_SET_CAL_DATA] &= ~(1 << prox_idx);
		pdata->offset = data->msg_buf[prox_idx][0];
		mutex_unlock(&data->prox_factory_mutex);
		pr_info("[FACTORY] %s: Cal data: %d(%d)\n", __func__, msg, pdata->offset);
	} else {
		pr_info("[FACTORY] %s: No cal data\n", __func__);
	}
}

void prox_cal_init_work(struct adsp_data *data)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	data->prox_cal = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	cal_filp = filp_open(PROX_FACTORY_CAL_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s : no prox_factory_cal file\n", __func__);
		set_fs(old_fs);
		prox_save_cal_data_to_efs(data->prox_cal, true);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error\n", __func__);
		set_fs(old_fs);
	} else {
		ret = vfs_read(cal_filp, (char *)&data->prox_cal,
			sizeof(int32_t), &cal_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s: fd read fail:%d\n",
				__func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			return;
		}

		pr_info("[FACTORY] %s : already exist (cal data: %d)\n",
			__func__, data->prox_cal);
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
	}
}
#endif

static ssize_t prox_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t prox_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t prox_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (pdata->avgwork_check == 0) {
		get_sub_prox_raw_data(&pdata->val, &pdata->offset);
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", pdata->val);
}

static ssize_t prox_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", pdata->min,
		pdata->avg, pdata->max);
}

static ssize_t prox_avg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int new_value;

	if (sysfs_streq(buf, "0"))
		new_value = 0;
	else
		new_value = 1;

	if (new_value == pdata->avgtimer_enabled)
		return size;

	if (new_value == 0) {
		pdata->avgtimer_enabled = 0;
		hrtimer_cancel(&pdata->prox_timer);
		cancel_work_sync(&pdata->work_prox);
	} else {
		pdata->avgtimer_enabled = 1;
		pdata->dev_data = data;
		hrtimer_start(&pdata->prox_timer,
			ns_to_ktime(2000 * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	}

	return size;
}

static void prox_work_func(struct work_struct *work)
{
	int min = 0, max = 0, avg = 0;
	int i;

	pdata->avgwork_check = 1;
	for (i = 0; i < PROX_AVG_COUNT; i++) {
		msleep(20);

		get_sub_prox_raw_data(&pdata->val, &pdata->offset);
		avg += pdata->val;

		if (!i)
			min = pdata->val;
		else if (pdata->val < min)
			min = pdata->val;

		if (pdata->val > max)
			max = pdata->val;
	}
	avg /= PROX_AVG_COUNT;

	pdata->min = min;
	pdata->avg = avg;
	pdata->max = max;
	pdata->avgwork_check = 0;
}

static enum hrtimer_restart prox_timer_func(struct hrtimer *timer)
{
	queue_work(pdata->prox_wq, &pdata->work_prox);
	hrtimer_forward_now(&pdata->prox_timer,
		ns_to_ktime(2000 * NSEC_PER_MSEC));
	return HRTIMER_RESTART;
}

int get_prox_threshold(struct adsp_data *data, int type)
{
	uint8_t cnt = 0;
	uint16_t prox_idx = MSG_PROX_SUB;
	int32_t msg_buf[2];
	int ret = 0;

	msg_buf[0] = type;
	msg_buf[1] = 0;

	mutex_lock(&data->prox_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		prox_idx, 0, MSG_TYPE_GET_THRESHOLD);

	while (!(data->ready_flag[MSG_TYPE_GET_THRESHOLD] & 1 << prox_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_THRESHOLD] &= ~(1 << prox_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		mutex_unlock(&data->prox_factory_mutex);
		return ret;
	}

	ret = data->msg_buf[prox_idx][0];
	mutex_unlock(&data->prox_factory_mutex);

	return ret;
}

void set_prox_threshold(struct adsp_data *data, int type, int val)
{
	uint8_t cnt = 0;
	uint16_t prox_idx = MSG_PROX_SUB;
	int32_t msg_buf[2];

	msg_buf[0] = type;
	msg_buf[1] = val;

	mutex_lock(&data->prox_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		prox_idx, 0, MSG_TYPE_SET_THRESHOLD);

	while (!(data->ready_flag[MSG_TYPE_SET_THRESHOLD] & 1 << prox_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_THRESHOLD] &= ~(1 << prox_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	mutex_unlock(&data->prox_factory_mutex);
}

static ssize_t prox_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
#ifdef CONFIG_SUPPORT_AMS_PROX_CALIBRATION
	return snprintf(buf, PAGE_SIZE, "%d,0,0\n", data->prox_cal);
#else
	return snprintf(buf, PAGE_SIZE, "0,0,0\n");
#endif
}

static ssize_t prox_cal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
#ifdef CONFIG_SUPPORT_AMS_PROX_CALIBRATION
	struct adsp_data *data = dev_get_drvdata(dev);
	int cal_data, msg;
	uint16_t prox_idx = MSG_PROX_SUB;
	uint8_t cnt = 0;
	char *cur_id_str;

	if (sysfs_streq(buf, "1")) {
		msg = PROX_CMD_TYPE_GET_CAL_DATA;
#ifdef CONFIG_SUPPORT_PROX_INIT_CAL
	} else if (sysfs_streq(buf, "0")) {
		msg = PROX_CMD_TYPE_INIT_CAL_DATA;
#endif
	} else {
		pr_err("[FACTORY] %s: wrong value\n", __func__);
		return size;
	}

	pr_info("[FACTORY] %s: msg %d\n", __func__, msg);

	mutex_lock(&data->prox_factory_mutex);
	adsp_unicast(&msg, sizeof(int32_t), prox_idx, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << prox_idx) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << prox_idx);
	cal_data = data->msg_buf[prox_idx][0];
	mutex_unlock(&data->prox_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		if (cal_data > 0) {
			cur_id_str = kzalloc(AMS_UB_CELL_ID_INFO_STRING_LENGTH,
					GFP_KERNEL);
			prox_save_cal_data_to_efs(cal_data, false);
			pr_info("[FACTORY] %s: done! %d\n", __func__, cal_data);
			data->prox_cal = cal_data;
			prox_send_cal_data(data, true);
#ifdef CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION
			ams_load_ub_cell_id_from_file(UB_CELL_ID_PATH, cur_id_str);
			ams_save_ub_cell_id_to_efs(LIGHT_UB_CELL_ID_PATH, cur_id_str, false);

			ams_load_ub_cell_id_from_file(SUB_UB_CELL_ID_PATH, cur_id_str);
			ams_save_ub_cell_id_to_efs(SUB_LIGHT_UB_CELL_ID_PATH, cur_id_str, false);
#endif
			kfree(cur_id_str);
#ifdef CONFIG_SUPPORT_PROX_INIT_CAL
		} else if (msg == PROX_CMD_TYPE_INIT_CAL_DATA) {
			pr_info("[FACTORY] %s: init cal %d\n", __func__, cal_data);
			prox_save_cal_data_to_efs(cal_data, false);
			data->prox_cal = cal_data;
#endif
		} else {
			pr_info("[FACTORY] %s: fail! %d\n", __func__, cal_data);
		}
	}
#else
	pr_info("[FACTORY] %s: unsupported prox cal!\n", __func__);
#endif
	return size;
}

static ssize_t prox_thresh_high_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int thd;

	thd = get_prox_threshold(data, PRX_THRESHOLD_DETECT_H);
	pr_info("[FACTORY] %s: %d\n", __func__, thd);

	return snprintf(buf, PAGE_SIZE, "%d\n",	thd);
}

static ssize_t prox_thresh_high_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int thd = 0;

	if (kstrtoint(buf, 10, &thd)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return size;
	}

	set_prox_threshold(data, PRX_THRESHOLD_DETECT_H, thd);
	pr_info("[FACTORY] %s: %d\n", __func__, thd);

	return size;
}

static ssize_t prox_thresh_low_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int thd;

	thd = get_prox_threshold(data, PRX_THRESHOLD_RELEASE_L);
	pr_info("[FACTORY] %s: %d\n", __func__, thd);

	return snprintf(buf, PAGE_SIZE, "%d\n",	thd);
}

static ssize_t prox_thresh_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int thd = 0;

	if (kstrtoint(buf, 10, &thd)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return size;
	}

	set_prox_threshold(data, PRX_THRESHOLD_RELEASE_L, thd);
	pr_info("[FACTORY] %s: %d\n", __func__, thd);

	return size;
}

#ifdef CONFIG_SUPPORT_PROX_AUTO_CAL
static ssize_t prox_thresh_detect_high_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int thd;

	thd = get_prox_threshold(data, PRX_THRESHOLD_HIGH_DETECT_H);
	pr_info("[FACTORY] %s: %d\n", __func__, thd);

	return snprintf(buf, PAGE_SIZE, "%d\n",	thd);
}

static ssize_t prox_thresh_detect_high_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int thd = 0;

	if (kstrtoint(buf, 10, &thd)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return size;
	}

	set_prox_threshold(data, PRX_THRESHOLD_HIGH_DETECT_H, thd);
	pr_info("[FACTORY] %s: %d\n", __func__, thd);

	return size;
}

static ssize_t prox_thresh_detect_low_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int thd;

	thd = get_prox_threshold(data, PRX_THRESHOLD_HIGH_DETECT_L);
	pr_info("[FACTORY] %s: %d\n", __func__, thd);

	return snprintf(buf, PAGE_SIZE, "%d\n",	thd);
}

static ssize_t prox_thresh_detect_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int thd = 0;

	if (kstrtoint(buf, 10, &thd)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return size;
	}

	set_prox_threshold(data, PRX_THRESHOLD_HIGH_DETECT_L, thd);
	pr_info("[FACTORY] %s: %d\n", __func__, thd);

	return size;
}
#endif

static ssize_t barcode_emul_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", pdata->bBarcodeEnabled);
}

static ssize_t barcode_emul_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int iRet;
	int64_t dEnable;

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	if (dEnable)
		pdata->bBarcodeEnabled = 1;
	else
		pdata->bBarcodeEnabled = 0;

	return size;
}

static ssize_t prox_cancel_pass_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#ifdef CONFIG_SUPPORT_AMS_PROX_CALIBRATION
	struct adsp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", (data->prox_cal > 0) ? 1 : 0);
#else
	return snprintf(buf, PAGE_SIZE, "1\n");
#endif
}

static ssize_t prox_default_trim_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", pdata->offset);
}

static ssize_t prox_alert_thresh_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", PROX_ALERT_THRESHOLD);
}

static ssize_t prox_register_read_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t prox_idx = MSG_PROX_SUB;
	int cnt = 0;
	int32_t msg_buf[1];

	msg_buf[0] = pdata->reg_backup[0];

	mutex_lock(&data->prox_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		prox_idx, 0, MSG_TYPE_GET_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_GET_REGISTER] & 1 << prox_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_REGISTER] &= ~(1 << prox_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	pdata->reg_backup[1] = data->msg_buf[prox_idx][0];
	pr_info("[FACTORY] %s: [0x%x]: %d\n",
		__func__, pdata->reg_backup[0], pdata->reg_backup[1]);

	mutex_unlock(&data->prox_factory_mutex);

	return snprintf(buf, PAGE_SIZE, "%d\n", pdata->reg_backup[1]);
}

static ssize_t prox_register_read_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int reg = 0;

	if (sscanf(buf, "%3d", &reg) != 1) {
		pr_err("[FACTORY]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	pdata->reg_backup[0] = reg;
	pr_info("[FACTORY] %s: [0x%x]\n", __func__, pdata->reg_backup[0]);

	return size;
}

static ssize_t prox_register_write_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t prox_idx = MSG_PROX_SUB;
	int cnt = 0;
	int32_t msg_buf[2];

	if (sscanf(buf, "%3d,%5d", &msg_buf[0], &msg_buf[1]) != 2) {
		pr_err("[FACTORY]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	mutex_lock(&data->prox_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		prox_idx, 0, MSG_TYPE_SET_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_SET_REGISTER] & 1 << prox_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_REGISTER] &= ~(1 << prox_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	pdata->reg_backup[0] = msg_buf[0];
	pr_info("[FACTORY] %s: 0x%x - %d\n",
		__func__, msg_buf[0], data->msg_buf[prox_idx][0]);
	mutex_unlock(&data->prox_factory_mutex);

	return size;
}

static ssize_t prox_light_get_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t prox_idx = MSG_PROX_SUB;
	uint8_t cnt = 0;
	int offset = 0;
	int32_t *info = data->msg_buf[prox_idx];

	mutex_lock(&data->prox_factory_mutex);
	adsp_unicast(NULL, 0, prox_idx, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << prox_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << prox_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	pr_info("[FACTORY] %d,%d,%d,%d,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%d\n",
		info[0], info[1], info[2], info[3], info[4], info[5],
		info[6], info[7], info[8], info[9], info[10], info[11]);

	offset += snprintf(buf + offset, PAGE_SIZE - offset,
		"\"THD\":\"%d %d %d %d\",", info[0], info[1], info[2], info[3]);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
		"\"PDRIVE_CURRENT\":\"%02x\",", info[4]);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
		"\"PERSIST_TIME\":\"%02x\",", info[5]);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
		"\"PPULSE\":\"%02x\",", info[6]);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
		"\"PGAIN\":\"%02x\",", info[7]);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
		"\"PTIME\":\"%02x\",", info[8]);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
		"\"PPLUSE_LEN\":\"%02x\",", info[9]);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
		"\"ATIME\":\"%02x\",", info[10]);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
		"\"POFFSET\":\"%d\"\n", info[11]);

	mutex_unlock(&data->prox_factory_mutex);
	return offset;
}

static ssize_t prox_wakelock_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}

static ssize_t prox_trim_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint16_t prox_idx = MSG_PROX_SUB;
	int cnt = 0;
	int32_t msg = PROX_CMD_TYPE_GET_TRIM_CHECK;

	mutex_lock(&data->prox_factory_mutex);
	adsp_unicast(&msg, sizeof(int32_t), prox_idx, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << prox_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << prox_idx);
	mutex_unlock(&data->prox_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "NG\n");
	}

	pr_info("[FACTORY] %s: [%s]: 0x%x, 0x%x\n",
		__func__, (data->msg_buf[prox_idx][0] > 0) ? "TRIM" : "UNTRIM",
		(uint16_t)data->msg_buf[prox_idx][1],
		(uint16_t)data->msg_buf[prox_idx][2]);

	return snprintf(buf, PAGE_SIZE, "%s\n",
		(data->msg_buf[prox_idx][0] > 0) ? "TRIM" : "UNTRIM");
}

static DEVICE_ATTR(vendor, 0444, prox_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, prox_name_show, NULL);
static DEVICE_ATTR(state, 0444, prox_raw_data_show, NULL);
static DEVICE_ATTR(raw_data, 0444, prox_raw_data_show, NULL);
static DEVICE_ATTR(prox_avg, 0664,
	prox_avg_show, prox_avg_store);
static DEVICE_ATTR(prox_cal, 0664,
	prox_cal_show, prox_cal_store);
static DEVICE_ATTR(thresh_high, 0664,
	prox_thresh_high_show, prox_thresh_high_store);
static DEVICE_ATTR(thresh_low, 0664,
	prox_thresh_low_show, prox_thresh_low_store);
static DEVICE_ATTR(register_write, 0220,
	NULL, prox_register_write_store);
static DEVICE_ATTR(register_read, 0664,
	prox_register_read_show, prox_register_read_store);
static DEVICE_ATTR(barcode_emul_en, 0664,
	barcode_emul_enable_show, barcode_emul_enable_store);
static DEVICE_ATTR(prox_offset_pass, 0444, prox_cancel_pass_show, NULL);
static DEVICE_ATTR(prox_trim, 0444, prox_default_trim_show, NULL);

#ifdef CONFIG_SUPPORT_PROX_AUTO_CAL
static DEVICE_ATTR(thresh_detect_high, 0664,
	prox_thresh_detect_high_show, prox_thresh_detect_high_store);
static DEVICE_ATTR(thresh_detect_low, 0664,
	prox_thresh_detect_low_show, prox_thresh_detect_low_store);
#endif
static DEVICE_ATTR(prox_alert_thresh, 0444, prox_alert_thresh_show, NULL);
static DEVICE_ATTR(dhr_sensor_info, 0440,
	prox_light_get_dhr_sensor_info_show, NULL);
static DEVICE_ATTR(prox_wakelock, 0220, NULL, prox_wakelock_store);
static DEVICE_ATTR(trim_check, 0444, prox_trim_check_show, NULL);

static struct device_attribute *prox_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_state,
	&dev_attr_raw_data,
	&dev_attr_prox_avg,
	&dev_attr_prox_cal,
	&dev_attr_thresh_high,
	&dev_attr_thresh_low,
	&dev_attr_barcode_emul_en,
	&dev_attr_prox_offset_pass,
	&dev_attr_prox_trim,
#ifdef CONFIG_SUPPORT_PROX_AUTO_CAL
	&dev_attr_thresh_detect_high,
	&dev_attr_thresh_detect_low,
#endif
	&dev_attr_prox_alert_thresh,
	&dev_attr_dhr_sensor_info,
	&dev_attr_register_write,
	&dev_attr_register_read,
	&dev_attr_prox_wakelock,
	&dev_attr_trim_check,
	NULL,
};

static int __init tmd490x_prox_factory_init(void)
{
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	adsp_factory_register(MSG_PROX, prox_attrs);
	pr_info("[FACTORY] %s\n", __func__);

	hrtimer_init(&pdata->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pdata->prox_timer.function = prox_timer_func;
	pdata->prox_wq = create_singlethread_workqueue("prox_wq");

	/* this is the thread function we run on the work queue */
	INIT_WORK(&pdata->work_prox, prox_work_func);

	pdata->avgwork_check = 0;
	pdata->avgtimer_enabled = 0;
	pdata->avg = 0;
	pdata->min = 0;
	pdata->max = 0;
	pdata->offset = 0;
	return 0;
}

static void __exit tmd490x_prox_factory_exit(void)
{
	if (pdata->avgtimer_enabled == 1) {
		hrtimer_cancel(&pdata->prox_timer);
		cancel_work_sync(&pdata->work_prox);
	}
	destroy_workqueue(pdata->prox_wq);
	adsp_factory_unregister(MSG_PROX);
	kfree(pdata);
	pr_info("[FACTORY] %s\n", __func__);
}

module_init(tmd490x_prox_factory_init);
module_exit(tmd490x_prox_factory_exit);
