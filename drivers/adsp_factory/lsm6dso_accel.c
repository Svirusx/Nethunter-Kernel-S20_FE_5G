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
#ifdef CONFIG_SLPI_MOTOR
#ifdef CONFIG_SUPPORT_MOTOR_NOTIFIER
#include <linux/vib_notifier.h>
#endif
#include <linux/adsp/slpi_motor.h>
#endif
#define VENDOR "STM"
#define CHIP_ID "LSM6DSO"
#define ACCEL_ST_TRY_CNT 3
#define ACCEL_FACTORY_CAL_CNT 20
#define ACCEL_RAW_DATA_CNT 3
#define MAX_ACCEL_1G 4096

#define STM_LSM6DSO_INT_CHECK_RUNNING   4

#define MAX_MOTOR_STOP_TIMEOUT (8 * NSEC_PER_SEC)
/* Haptic Pattern A vibrate during 7ms.
 * touch, touchkey, operation feedback use this.
 * Do not call motor_workfunc when duration is 7ms.
 */
#define DURATION_SKIP	10
#define MOTOR_OFF	0
#define MOTOR_ON	1

#define ACCEL_FACTORY_CAL_PATH "/efs/FactoryApp/accel_factory_cal"

#ifdef CONFIG_SLPI_MOTOR
struct accel_motor_data {
#ifdef CONFIG_SUPPORT_MOTOR_NOTIFIER
	struct notifier_block motor_nb;
	struct hrtimer motor_stop_timer;
#endif
	struct workqueue_struct *slpi_motor_wq;
	struct work_struct work_slpi_motor;
	atomic_t motor_state;
};

struct accel_motor_data *pdata_motor;
#endif

struct accel_data {
	struct work_struct work_accel;
	struct workqueue_struct *accel_wq;
	struct adsp_data *dev_data;
	bool is_complete_cal;
	bool lpf_onoff;
	bool st_complete;
	int32_t raw_data[ACCEL_RAW_DATA_CNT];
	int32_t avg_data[ACCEL_RAW_DATA_CNT];
};

static struct accel_data *pdata;

static ssize_t accel_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t accel_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t sensor_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", "ADSP");
}

int get_accel_cal_data(int32_t *cal_data)
{
	struct file *factory_cal_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	factory_cal_filp = filp_open(ACCEL_FACTORY_CAL_PATH,
			O_RDONLY, 0440);

	if (IS_ERR(factory_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(factory_cal_filp);
		pr_err("[FACTORY] %s: open fail accel_factory_cal:%d\n",
			__func__, ret);
		return ret;
	}

	ret = vfs_read(factory_cal_filp, (char *)cal_data,
		ACCEL_RAW_DATA_CNT * sizeof(int32_t), &factory_cal_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd read fail:%d\n", __func__, ret);
		filp_close(factory_cal_filp, current->files);
		set_fs(old_fs);
		return ret;
	}

	filp_close(factory_cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

int set_accel_cal_data(int32_t *cal_data, bool first_booting)
{
	struct file *factory_cal_filp = NULL;
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

	factory_cal_filp = filp_open(ACCEL_FACTORY_CAL_PATH, flag, mode);

	if (IS_ERR(factory_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(factory_cal_filp);
		pr_err("[FACTORY] %s: open fail accel_factory_cal:%d\n",
			__func__, ret);
		return ret;
	}

	ret = vfs_write(factory_cal_filp, (char *)cal_data,
		ACCEL_RAW_DATA_CNT * sizeof(int32_t), &factory_cal_filp->f_pos);
	if (ret < 0)
		pr_err("[FACTORY] %s: fd write %d\n", __func__, ret);

	filp_close(factory_cal_filp, current->files);
	set_fs(old_fs);

	adsp_unicast(pdata->avg_data, sizeof(pdata->avg_data),
		MSG_ACCEL, 0, MSG_TYPE_SET_CAL_DATA);
	return ret;
}

static ssize_t accel_calibration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int32_t cal_data[ACCEL_RAW_DATA_CNT] = {0, };

	if (get_accel_cal_data(cal_data) > 0) {
		pr_info("[FACTORY] %s:  %d, %d, %d\n", __func__,
			cal_data[0], cal_data[1], cal_data[2]);
		if (cal_data[0] == 0 && cal_data[1] == 0 && cal_data[2] == 0)
			return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n",
				0, 0, 0, 0);
		else
			return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n",
				true, cal_data[0], cal_data[1], cal_data[2]);
	} else {
		pr_err("[FACTORY] %s: get_accel_cal_data fail\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n", 0, 0, 0, 0);
	}
}

static ssize_t accel_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);

	pdata->dev_data = data;
	if (sysfs_streq(buf, "0")) {
		mutex_lock(&data->accel_factory_mutex);
		memset(pdata->avg_data, 0, sizeof(pdata->avg_data));
		set_accel_cal_data(pdata->avg_data, false);
		mutex_unlock(&data->accel_factory_mutex);
	} else {
		pdata->is_complete_cal = false;
		queue_work(pdata->accel_wq, &pdata->work_accel);
		while (pdata->is_complete_cal == false) {
			pr_info("[FACTORY] %s: In factory cal\n", __func__);
			msleep(20);
		}
		mutex_lock(&data->accel_factory_mutex);
		set_accel_cal_data(pdata->avg_data, false);
		mutex_unlock(&data->accel_factory_mutex);
	}

	return size;
}

static void accel_work_func(struct work_struct *work)
{
	struct accel_data *data = container_of((struct work_struct *)work,
		struct accel_data, work_accel);
	int i;

	mutex_lock(&data->dev_data->accel_factory_mutex);
	memset(pdata->avg_data, 0, sizeof(pdata->avg_data));
	adsp_unicast(pdata->avg_data, sizeof(pdata->avg_data),
		MSG_ACCEL, 0, MSG_TYPE_SET_CAL_DATA);
	msleep(30); /* for init of bias */
	for (i = 0; i < ACCEL_FACTORY_CAL_CNT; i++) {
		msleep(20);
		get_accel_raw_data(pdata->raw_data);
		pdata->avg_data[0] += pdata->raw_data[0];
		pdata->avg_data[1] += pdata->raw_data[1];
		pdata->avg_data[2] += pdata->raw_data[2];
		pr_info("[FACTORY] %s:  %d, %d, %d\n", __func__,
			pdata->raw_data[0], pdata->raw_data[1],
			pdata->raw_data[2]);
	}

	for (i = 0; i < ACCEL_RAW_DATA_CNT; i++) {
		pdata->avg_data[i] /= ACCEL_FACTORY_CAL_CNT;
		pr_info("[FACTORY] %s: avg : %d\n",
			__func__, pdata->avg_data[i]);
	}

	if (pdata->avg_data[2] > 0)
		pdata->avg_data[2] -= MAX_ACCEL_1G;
	else if (pdata->avg_data[2] < 0)
		pdata->avg_data[2] += MAX_ACCEL_1G;

	mutex_unlock(&data->dev_data->accel_factory_mutex);
	pdata->is_complete_cal = true;

	return;
}

static ssize_t accel_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int retry = 0;

	pdata->st_complete = false;
RETRY_ACCEL_SELFTEST:
	adsp_unicast(NULL, 0, MSG_ACCEL, 0, MSG_TYPE_ST_SHOW_DATA);

	while (!(data->ready_flag[MSG_TYPE_ST_SHOW_DATA] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		msleep(25);

	data->ready_flag[MSG_TYPE_ST_SHOW_DATA] &= ~(1 << MSG_ACCEL);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		data->msg_buf[MSG_ACCEL][1] = -1;
	}

	pr_info("[FACTORY] %s : init = %d, result = %d, XYZ = %d, %d, %d, nXYZ = %d, %d, %d\n",
		__func__, data->msg_buf[MSG_ACCEL][0],
		data->msg_buf[MSG_ACCEL][1], data->msg_buf[MSG_ACCEL][2],
		data->msg_buf[MSG_ACCEL][3], data->msg_buf[MSG_ACCEL][4],
		data->msg_buf[MSG_ACCEL][5], data->msg_buf[MSG_ACCEL][6],
		data->msg_buf[MSG_ACCEL][7]);

	if (data->msg_buf[MSG_ACCEL][1] == 1) {
		pr_info("[FACTORY] %s : Pass - result = %d, retry = %d\n",
			__func__, data->msg_buf[MSG_ACCEL][1], retry);
	} else {
		data->msg_buf[MSG_ACCEL][1] = -5;
		pr_err("[FACTORY] %s : Fail - result = %d, retry = %d\n",
			__func__, data->msg_buf[MSG_ACCEL][1], retry);

		if (retry < ACCEL_ST_TRY_CNT &&
			data->msg_buf[MSG_ACCEL][2] == 0) {
			retry++;
			msleep(200);
			cnt = 0;
			pr_info("[FACTORY] %s: retry\n", __func__);
			goto RETRY_ACCEL_SELFTEST;
		}
	}

	pdata->st_complete = true;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d\n",
			data->msg_buf[MSG_ACCEL][1],
			(int)abs(data->msg_buf[MSG_ACCEL][2]),
			(int)abs(data->msg_buf[MSG_ACCEL][3]),
			(int)abs(data->msg_buf[MSG_ACCEL][4]),
			(int)abs(data->msg_buf[MSG_ACCEL][5]),
			(int)abs(data->msg_buf[MSG_ACCEL][6]),
			(int)abs(data->msg_buf[MSG_ACCEL][7]));
}

static ssize_t accel_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t raw_data[ACCEL_RAW_DATA_CNT] = {0, };
	static int32_t prev_raw_data[ACCEL_RAW_DATA_CNT] = {0, };
	int ret = 0;

	if (pdata->st_complete == false) {
		pr_info("[FACTORY] %s: selftest is running\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
			raw_data[0], raw_data[1], raw_data[2]);
	}

	mutex_lock(&data->accel_factory_mutex);
	ret = get_accel_raw_data(raw_data);
	mutex_unlock(&data->accel_factory_mutex);

	if (!ret) {
		memcpy(prev_raw_data, raw_data, sizeof(int32_t) * 3);
	} else if (!pdata->lpf_onoff) {
		pr_err("[FACTORY] %s: using prev data!!!\n", __func__);
		memcpy(raw_data, prev_raw_data, sizeof(int32_t) * 3);
	} else {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	}

#ifdef CONFIG_SEC_FACTORY
	pr_info("[FACTORY] %s: %d, %d, %d\n", __func__,
		raw_data[0], raw_data[1], raw_data[2]);
#endif

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		raw_data[0], raw_data[1], raw_data[2]);
}

static ssize_t accel_reactive_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	bool success = false;
	int32_t msg_buf = 0;

	mutex_lock(&data->accel_factory_mutex);
	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL,
		0, MSG_TYPE_GET_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_GET_REGISTER] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_REGISTER] &= ~(1 << MSG_ACCEL);
	mutex_unlock(&data->accel_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d\n", (int)success);
	}

	pr_info("[FACTORY]: %s - %d\n", __func__,
		data->msg_buf[MSG_ACCEL][0]);

	if (data->msg_buf[MSG_ACCEL][0] == 0)
		success = true;

	return snprintf(buf, PAGE_SIZE, "%d\n", (int)success);
}

static ssize_t accel_reactive_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t msg_buf;
	uint8_t cnt = 0;

	if (sysfs_streq(buf, "1"))
		pr_info("[FACTORY]: %s - on\n", __func__);
	else if (sysfs_streq(buf, "0"))
		pr_info("[FACTORY]: %s - off\n", __func__);
	else if (sysfs_streq(buf, "2")) {
		pr_info("[FACTORY]: %s - factory\n", __func__);
		msg_buf = 1;

		mutex_lock(&data->accel_factory_mutex);
		adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL,
			0, MSG_TYPE_GET_REGISTER);

		while (!(data->ready_flag[MSG_TYPE_GET_REGISTER] & 1 << MSG_ACCEL) &&
			cnt++ < TIMEOUT_CNT)
			usleep_range(500, 550);

		data->ready_flag[MSG_TYPE_GET_REGISTER] &= ~(1 << MSG_ACCEL);
		mutex_unlock(&data->accel_factory_mutex);

		if (cnt >= TIMEOUT_CNT) {
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
			return size;
		}

		if (data->msg_buf[MSG_ACCEL][0] == STM_LSM6DSO_INT_CHECK_RUNNING)
			pr_info("[FACTORY]: %s - STM_LSM6DSO_INT_CHECK_RUNNING\n", __func__);
		else
			pr_info("[FACTORY]: %s - Something wrong\n", __func__);
	}

	return size;
}

static ssize_t accel_lowpassfilter_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int32_t msg_buf;

	if (sysfs_streq(buf, "1")) {
		msg_buf = 1;
	} else if (sysfs_streq(buf, "0")) {
		msg_buf = 0;
#ifdef CONFIG_SEC_FACTORY
	} else if (sysfs_streq(buf, "2")) {
		msg_buf = 2;
		pr_info("[FACTORY] %s: Pretest\n", __func__);
#endif
	} else {
		pr_info("[FACTORY] %s: wrong value\n", __func__);
		return size;
	}

	mutex_lock(&data->accel_factory_mutex);
	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL,
		0, MSG_TYPE_SET_ACCEL_LPF);

	while (!(data->ready_flag[MSG_TYPE_SET_ACCEL_LPF] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_ACCEL_LPF] &= ~(1 << MSG_ACCEL);
	mutex_unlock(&data->accel_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return size;
	}

	pdata->lpf_onoff = (bool)data->msg_buf[MSG_ACCEL][0];

	pr_info("[FACTORY] %s: %d, 0x0A:%02x 0x0D:%02x 0x10:%02x\n", __func__,
		data->msg_buf[MSG_ACCEL][0], data->msg_buf[MSG_ACCEL][1],
		data->msg_buf[MSG_ACCEL][2], data->msg_buf[MSG_ACCEL][3]);

	return size;
}

#ifdef CONFIG_SLPI_MOTOR
#ifdef CONFIG_SUPPORT_MOTOR_NOTIFIER
int ssc_motor_notify(struct notifier_block *nb,
	unsigned long enable, void *v)
{
	if(enable == 1) {
		if (atomic_read(&pdata_motor->motor_state) == MOTOR_OFF) {
			atomic_set(&pdata_motor->motor_state, MOTOR_ON);
			queue_work(pdata_motor->slpi_motor_wq,
				&pdata_motor->work_slpi_motor);                       
			pr_info("[FACTORY] %s: Send Motor enable\n", __func__);
		} else {
			hrtimer_cancel(&pdata_motor->motor_stop_timer);
			hrtimer_start(&pdata_motor->motor_stop_timer,
				ns_to_ktime(MAX_MOTOR_STOP_TIMEOUT),
				HRTIMER_MODE_REL);
			pr_info("[FACTORY] %s: Reset Motor timer\n", __func__);
		}
	} else {
		pr_info("[FACTORY] %s: Not support 0 value\n", __func__);
	}

	return 0;
}

static enum hrtimer_restart motor_stop_timer_func(struct hrtimer *timer)
{
	pr_info("[FACTORY] %s\n", __func__);
	atomic_set(&pdata_motor->motor_state, MOTOR_OFF);
	queue_work(pdata_motor->slpi_motor_wq, &pdata_motor->work_slpi_motor);

	return HRTIMER_NORESTART;
}
#else
int setSensorCallback(int state, int duration)
{
	pr_info("[FACTORY] %s IN: state = %d, duration = %d\n",
		__func__, (int)atomic_read(&pdata_motor->motor_state), duration);

	if ((duration > MOTOR_OFF && duration <= DURATION_SKIP) || !pdata->lpf_onoff)
		return 0;

	if (atomic_read(&pdata_motor->motor_state) != state) {
		pr_info("[FACTORY] %s: state = %d/%d, duration = %d\n",
			__func__, (int)atomic_read(&pdata_motor->motor_state),
			state, duration);
		atomic_set(&pdata_motor->motor_state, state);
		queue_work(pdata_motor->slpi_motor_wq,
			&pdata_motor->work_slpi_motor);
	}

	return 0;
}
#endif
void slpi_motor_work_func(struct work_struct *work)
{
	int32_t msg_buf = 0;

	if (atomic_read(&pdata_motor->motor_state) == MOTOR_ON) {
#ifdef CONFIG_SUPPORT_MOTOR_NOTIFIER
		hrtimer_start(&pdata_motor->motor_stop_timer,
			ns_to_ktime(MAX_MOTOR_STOP_TIMEOUT),
			HRTIMER_MODE_REL);
#endif
		msg_buf = 1;
	} else if (atomic_read(&pdata_motor->motor_state) == MOTOR_OFF) {
#ifdef CONFIG_SUPPORT_MOTOR_NOTIFIER
		hrtimer_cancel(&pdata_motor->motor_stop_timer);
#endif
		msg_buf = 0;
	} else {
		pr_info("[FACTORY] %s: invalid state %d\n",
			__func__, (int)atomic_read(&pdata_motor->motor_state));
	}

	pr_info("[FACTORY] %s: msg_buf = %d\n", __func__, msg_buf);

	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL,
		0, MSG_TYPE_SET_ACCEL_MOTOR);
}
#endif

static ssize_t accel_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	char ctrl1_xl = 0;
	uint8_t fullscale = 0;

	adsp_unicast(NULL, 0, MSG_ACCEL, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << MSG_ACCEL);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		ctrl1_xl = data->msg_buf[MSG_ACCEL][16];

		ctrl1_xl &= 0xC;

		switch (ctrl1_xl) {
		case 0xC:
			fullscale = 8;
			break;
		case 0x8:
			fullscale = 4;
			break;
		case 0x4:
			fullscale = 16;
			break;
		case 0:
			fullscale = 2;
			break;
		default:
			break;
		}
	}
	pr_info("[FACTORY] %s: f/s %u\n", __func__, fullscale);

	return snprintf(buf, PAGE_SIZE, "\"FULL_SCALE\":\"%uG\"\n", fullscale);
}

static DEVICE_ATTR(name, 0444, accel_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, accel_vendor_show, NULL);
static DEVICE_ATTR(type, 0444, sensor_type_show, NULL);
static DEVICE_ATTR(calibration, 0664,
	accel_calibration_show, accel_calibration_store);
static DEVICE_ATTR(selftest, 0440,
	accel_selftest_show, NULL);
static DEVICE_ATTR(raw_data, 0444, accel_raw_data_show, NULL);
static DEVICE_ATTR(reactive_alert, 0664,
	accel_reactive_show, accel_reactive_store);
static DEVICE_ATTR(lowpassfilter, 0220,
	NULL, accel_lowpassfilter_store);
#ifdef CONFIG_SEC_FACTORY
static DEVICE_ATTR(dhr_sensor_info, 0444,
	accel_dhr_sensor_info_show, NULL);
#else
static DEVICE_ATTR(dhr_sensor_info, 0440,
	accel_dhr_sensor_info_show, NULL);
#endif

static struct device_attribute *acc_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_type,
	&dev_attr_calibration,
	&dev_attr_selftest,
	&dev_attr_raw_data,
	&dev_attr_reactive_alert,
	&dev_attr_lowpassfilter,
	&dev_attr_dhr_sensor_info,
	NULL,
};

void accel_factory_init_work(void)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int32_t zero_data[ACCEL_RAW_DATA_CNT] = {0, };
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(ACCEL_FACTORY_CAL_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s : no accel_factory_cal file\n", __func__);
		set_fs(old_fs);
		set_accel_cal_data(zero_data, true);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error\n", __func__);
		set_fs(old_fs);
	} else {
		pr_info("[FACTORY] %s : already exist\n", __func__);
		ret = vfs_read(cal_filp, (char *)zero_data,
			ACCEL_RAW_DATA_CNT * sizeof(int32_t), &cal_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s: fd read fail:%d\n", __func__, ret);
			zero_data[0] = zero_data[1] = zero_data[2] = 0;
			adsp_unicast(zero_data, sizeof(zero_data),
				MSG_ACCEL, 0, MSG_TYPE_SET_CAL_DATA);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			return;
		}
		adsp_unicast(zero_data, sizeof(zero_data),
			MSG_ACCEL, 0, MSG_TYPE_SET_CAL_DATA);
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
	}
}

static int __init lsm6dso_accel_factory_init(void)
{
	adsp_factory_register(MSG_ACCEL, acc_attrs);
#ifdef CONFIG_SLPI_MOTOR
	pdata_motor = kzalloc(sizeof(*pdata_motor), GFP_KERNEL);

	if (pdata_motor == NULL)
		return -ENOMEM;

#ifdef CONFIG_SUPPORT_MOTOR_NOTIFIER
	pdata_motor->motor_nb.notifier_call = ssc_motor_notify,
	pdata_motor->motor_nb.priority = 1,
	vib_notifier_register(&pdata_motor->motor_nb);

	hrtimer_init(&pdata_motor->motor_stop_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pdata_motor->motor_stop_timer.function = motor_stop_timer_func;
#endif
	pdata_motor->slpi_motor_wq =
		create_singlethread_workqueue("slpi_motor_wq");

	if (pdata_motor->slpi_motor_wq == NULL) {
		pr_err("[FACTORY]: %s - could not create motor wq\n", __func__);
		kfree(pdata_motor);
		return -ENOMEM;
	}

	INIT_WORK(&pdata_motor->work_slpi_motor, slpi_motor_work_func);

	atomic_set(&pdata_motor->motor_state, MOTOR_OFF);
#endif
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	pdata->accel_wq = create_singlethread_workqueue("accel_wq");
	INIT_WORK(&pdata->work_accel, accel_work_func);

	pdata->lpf_onoff = true;
	pdata->st_complete = true;
	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

static void __exit lsm6dso_accel_factory_exit(void)
{
	adsp_factory_unregister(MSG_ACCEL);
#ifdef CONFIG_SLPI_MOTOR
#ifdef CONFIG_SUPPORT_MOTOR_NOTIFIER
	if (atomic_read(&pdata_motor->motor_state) == MOTOR_ON)
		hrtimer_cancel(&pdata_motor->motor_stop_timer);
#endif
	if (pdata_motor != NULL && pdata_motor->slpi_motor_wq != NULL) {
		cancel_work_sync(&pdata_motor->work_slpi_motor);
		destroy_workqueue(pdata_motor->slpi_motor_wq);
		pdata_motor->slpi_motor_wq = NULL;
		kfree(pdata_motor);
	}
#endif
	pr_info("[FACTORY] %s\n", __func__);
}
module_init(lsm6dso_accel_factory_init);
module_exit(lsm6dso_accel_factory_exit);
