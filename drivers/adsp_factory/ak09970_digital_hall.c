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
#if defined(CONFIG_SEC_BLOOMXQ_PROJECT) && defined(CONFIG_DSP_SLEEP_RECOVERY)
#define FORCE_SSR_FOR_TIMEOUT
#endif
#ifdef FORCE_SSR_FOR_TIMEOUT
#include <linux/adsp/slpi-loader.h>
#define MAX_SSR_LIMIT 3
#define SSR_TRIGGER_CNT 10
#endif

#define VENDOR "AKM"
#define CHIP_ID "AK09970"
#define IDX_180_X 0
#define IDX_180_Y 1
#define IDX_180_Z 2
#define IDX_90_X 3
#define IDX_90_Y 4
#define IDX_90_Z 5
#define IDX_0_X 6
#define IDX_0_Y 7
#define IDX_0_Z 8
#define SPEC_180_X_IDX 3
#define SPEC_90_X_IDX 4
#define SPEC_0_X_IDX 5
#define SPEC_180_Y_IDX 6
#define SPEC_90_Y_IDX 7
#define SPEC_0_Y_IDX 8
#define SPEC_180_Z_IDX 9
#define SPEC_90_Z_IDX 10
#define SPEC_0_Z_IDX 11
#define SPEC_MIN_IDX 0
#define SPEC_MAX_IDX 1
#define PASS 0
#define FAIL (-1)
#define ERR_NO_CNT_CNTL2 (-2)
#define ERR_NO_CNT_WAIT (-3)
#define ERR_NO_CNT_READ (-4)
#define DEGREE_180 180
#define DEGREE_90 90
#define DEGREE_0 0
#define FAC_CAL_DATA_NUM 9
#define HALL_ANGLE_SPEC 15

#define LF_STREAM_AUTO_CAL_X_PATH "/data/digital_hall_auto_cal_x"
#define LF_STREAM_AUTO_CAL_Y_PATH "/data/digital_hall_auto_cal_y"
#define LF_STREAM_AUTO_CAL_Z_PATH "/data/digital_hall_auto_cal_z"

static int32_t spec[12][2] = {
	{170, 180}, {80, 110}, {0, 10}, //ref angle 180, 90, 0
	{-39640, 39640}, {-39640, 39640}, {-39640, 39640}, // X 180, 90, 0
	{-39640, 39640}, {-39640, 39640}, {-39640, 39640}, // Y 180, 90, 0
	{-39640, 39640}, {-39640, 39640}, {-39640, 39640} // Z 180, 90, 0
};

static int32_t test_spec[10][2] = {
	{150, 170}, {40, 60}, //ref angle 160, 50
	{-39640, 39640}, {-39640, 39640}, // X 160, 50
	{-39640, 39640}, {-39640, 39640}, // Y 160, 50
	{-39640, 39640}, {-39640, 39640}, // Z 160, 50
	{145, 175}, {35, 65} //hall angle 160, 50
};

int32_t curr_angle;

struct lf_stream_data {
	int32_t ref_x[19];
	int32_t ref_y[19];
	int32_t ref_z[19];
	int32_t flg_update;
};
static struct lf_stream_data *pdata;

struct autocal_data_force_update {
#ifdef CONFIG_SEC_FACTORY
	struct workqueue_struct *autocal_debug_wq;
	struct work_struct work_autocal_debug;
#endif
	struct hrtimer rftest_timer;
	struct work_struct work_rftest;
	struct workqueue_struct *rftest_wq;
	int32_t ref_x[19];
	int32_t ref_y[19];
	int32_t ref_z[19];
	int32_t flg_update;
	int32_t min_angle;
	int32_t max_angle;
	int32_t init_angle;
	short rftest_timer_enabled;
	int32_t flex_low;
	int32_t flex_cover_hi;
};
static struct autocal_data_force_update *auto_cal_data;

#ifdef CONFIG_SEC_FACTORY
void autocal_debug_work_func(struct work_struct *work)
{
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_CAL_DATA);
}
#endif

static enum hrtimer_restart rftest_timer_func(struct hrtimer *timer)
{
	queue_work(auto_cal_data->rftest_wq, &auto_cal_data->work_rftest);
	hrtimer_forward_now(&auto_cal_data->rftest_timer,
		ns_to_ktime(2000 * NSEC_PER_MSEC));
	return HRTIMER_RESTART;
}

static void rftest_work_func(struct work_struct *work)
{
	int32_t hall_angle = 0;

	get_hall_angle_data(&hall_angle);

	if (hall_angle < auto_cal_data->min_angle)
		auto_cal_data->min_angle = hall_angle;
	if (hall_angle > auto_cal_data->max_angle)
		auto_cal_data->max_angle = hall_angle;

	pr_info("[FACTORY] %s - curr/min/max = %d/%d/%d\n",
		__func__, hall_angle,
		auto_cal_data->min_angle, auto_cal_data->max_angle);
}

static ssize_t digital_hall_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t digital_hall_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t digital_hall_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s - start", __func__);

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL, 0, MSG_TYPE_ST_SHOW_DATA);

	while (!(data->ready_flag[MSG_TYPE_ST_SHOW_DATA] & 1 << MSG_DIGITAL_HALL) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_ST_SHOW_DATA] &= ~(1 << MSG_DIGITAL_HALL);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
                return snprintf(buf, PAGE_SIZE, "-1,0,0,0,0,0,0,0,0,0,0\n");
	}

	pr_info("[FACTORY] SW_RST: %d, ST_RES: %d, min: %d/%d/%d, max: %d/%d/%d, avg: %d/%d/%d\n",
		data->msg_buf[MSG_DIGITAL_HALL][0], data->msg_buf[MSG_DIGITAL_HALL][1],
		data->msg_buf[MSG_DIGITAL_HALL][2], data->msg_buf[MSG_DIGITAL_HALL][3],
		data->msg_buf[MSG_DIGITAL_HALL][4], data->msg_buf[MSG_DIGITAL_HALL][5],
		data->msg_buf[MSG_DIGITAL_HALL][6], data->msg_buf[MSG_DIGITAL_HALL][7],
		data->msg_buf[MSG_DIGITAL_HALL][8], data->msg_buf[MSG_DIGITAL_HALL][9],
		data->msg_buf[MSG_DIGITAL_HALL][10]);

	if (data->msg_buf[MSG_DIGITAL_HALL][1] != PASS)
		data->msg_buf[MSG_DIGITAL_HALL][1] = FAIL;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_DIGITAL_HALL][0], data->msg_buf[MSG_DIGITAL_HALL][1],
		data->msg_buf[MSG_DIGITAL_HALL][2], data->msg_buf[MSG_DIGITAL_HALL][3],
		data->msg_buf[MSG_DIGITAL_HALL][4], data->msg_buf[MSG_DIGITAL_HALL][5],
		data->msg_buf[MSG_DIGITAL_HALL][6], data->msg_buf[MSG_DIGITAL_HALL][7],
		data->msg_buf[MSG_DIGITAL_HALL][8], data->msg_buf[MSG_DIGITAL_HALL][9],
		data->msg_buf[MSG_DIGITAL_HALL][10]);
}

static ssize_t digital_hall_spec_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		spec[0][0], spec[0][1], spec[1][0], spec[1][1], spec[2][0], spec[2][1],
		spec[3][0], spec[3][1], spec[4][0], spec[4][1], spec[5][0], spec[5][1],
		spec[6][0], spec[6][1], spec[7][0], spec[7][1], spec[8][0], spec[8][1],
		spec[9][0], spec[9][1], spec[10][0], spec[10][1], spec[11][0], spec[11][1]);
}

static ssize_t digital_hall_test_spec_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		test_spec[0][0], test_spec[0][1], test_spec[1][0], test_spec[1][1],
		test_spec[2][0], test_spec[2][1], test_spec[3][0], test_spec[3][1],
		test_spec[4][0], test_spec[4][1], test_spec[5][0], test_spec[5][1],
		test_spec[6][0], test_spec[6][1], test_spec[7][0], test_spec[7][1],
		test_spec[8][0], test_spec[8][1], test_spec[9][0], test_spec[9][1]);
}

static ssize_t digital_hall_ref_angle_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int32_t min, max, result;

	min = curr_angle - 10;
	max = curr_angle + 10;
	result = PASS;

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s - st %d/%d, akm %d/%d, lf %d/%d, hall %d/%d/%d(uT)\n",
		__func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][3],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][4],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][5],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8]);

	if (data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0] < min ||
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0] > max)
		result = FAIL;

	return snprintf(buf, PAGE_SIZE, "%d,%d\n",
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0], result);
}

static ssize_t digital_hall_read_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
                return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s - st %d/%d, akm %d/%d, lf %d/%d, hall %d/%d/%d(uT)\n",
		__func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][3],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][4],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][5],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][3],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][4],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][5],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8]);
}

static ssize_t digital_hall_test_read_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int result = PASS;
	int hall_angle = 0;
	int min = curr_angle - HALL_ANGLE_SPEC;
	int max = curr_angle + HALL_ANGLE_SPEC;

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
                return snprintf(buf, PAGE_SIZE, "-1,-1,-1,-1,-1,-1\n");
	}

	pr_info("[FACTORY] %s - st %d/%d, akm %d/%d, lf %d/%d, hall %d/%d/%d(uT)\n",
		__func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][3],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][4],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][5],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8]);

	hall_angle = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2];

	if (hall_angle < min || hall_angle > max) {
		pr_info("[FACTORY] %s - %d (%d, %d)\n", __func__,
			hall_angle, min, max);
		result = FAIL;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		result);
}

static ssize_t digital_hall_test_read_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (sysfs_streq(buf, "50")) {
		curr_angle = 50;
	} else if (sysfs_streq(buf, "160")) {
		curr_angle = 160;
	} else {
		pr_err("[FACTORY] %s - wrong degree !!!\n", __func__);
		return size;
	}

	pr_info("[FACTORY] %s - Test read at degree %d\n",
		__func__, curr_angle);

	return size;
}

static ssize_t reset_auto_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int reset_data[58] = { 0, };
	uint8_t cnt = 0;

	mutex_lock(&data->digital_hall_mutex);
	/* reset */
	adsp_unicast(reset_data, sizeof(reset_data),
		MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_REGISTER);

	/* read */
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < 3)
		msleep(30);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= 3) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0]);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0]);
}

int set_auto_cal_data_forced(bool first_booting)
{
	struct file *auto_cal_filp = NULL;
	mm_segment_t old_fs;
	int flag, ret = 0;
	umode_t mode = 0;
	char *write_buf = kzalloc(AUTO_CAL_FILE_BUF_LEN, GFP_KERNEL);

	if (first_booting) {
		flag = O_TRUNC | O_RDWR | O_CREAT;
		mode = 0600;
	} else {
		flag = O_RDWR;
		mode = 0660;
	}

	/* auto_cal X */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	auto_cal_filp = filp_open(DIGITAL_HALL_AUTO_CAL_X_PATH, flag, mode);

	if (IS_ERR(auto_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[FACTORY] %s: open fail auto_cal_x_filp:%d\n",
			__func__, ret);
		kfree(write_buf);
		return ret;
	}

	snprintf(write_buf, AUTO_CAL_FILE_BUF_LEN,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		auto_cal_data->flg_update, auto_cal_data->ref_x[0],
		auto_cal_data->ref_x[1], auto_cal_data->ref_x[2],
		auto_cal_data->ref_x[3], auto_cal_data->ref_x[4],
		auto_cal_data->ref_x[5], auto_cal_data->ref_x[6],
		auto_cal_data->ref_x[7], auto_cal_data->ref_x[8],
		auto_cal_data->ref_x[9], auto_cal_data->ref_x[10],
		auto_cal_data->ref_x[11], auto_cal_data->ref_x[12],
		auto_cal_data->ref_x[13], auto_cal_data->ref_x[14],
		auto_cal_data->ref_x[15], auto_cal_data->ref_x[16],
		auto_cal_data->ref_x[17], auto_cal_data->ref_x[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		AUTO_CAL_FILE_BUF_LEN * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[FACTORY] %s: auto_cal_x fd write:%d\n",
			__func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);
	memset(write_buf, 0, sizeof(char) * AUTO_CAL_FILE_BUF_LEN);

	/* auto_cal Y */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	auto_cal_filp = filp_open(DIGITAL_HALL_AUTO_CAL_Y_PATH, flag, mode);

	if (IS_ERR(auto_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[FACTORY] %s: open fail auto_cal_y_filp:%d\n",
			__func__, ret);
		kfree(write_buf);
		return ret;
	}

	snprintf(write_buf, AUTO_CAL_FILE_BUF_LEN,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		auto_cal_data->ref_y[0], auto_cal_data->ref_y[1],
		auto_cal_data->ref_y[2], auto_cal_data->ref_y[3],
		auto_cal_data->ref_y[4], auto_cal_data->ref_y[5],
		auto_cal_data->ref_y[6], auto_cal_data->ref_y[7],
		auto_cal_data->ref_y[8], auto_cal_data->ref_y[9],
		auto_cal_data->ref_y[10], auto_cal_data->ref_y[11],
		auto_cal_data->ref_y[12], auto_cal_data->ref_y[13],
		auto_cal_data->ref_y[14], auto_cal_data->ref_y[15],
		auto_cal_data->ref_y[16], auto_cal_data->ref_y[17],
		auto_cal_data->ref_y[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		AUTO_CAL_FILE_BUF_LEN * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[FACTORY] %s: auto_cal_y fd write:%d\n",
			__func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);
	memset(write_buf, 0, sizeof(char) * AUTO_CAL_FILE_BUF_LEN);

	/* auto_cal Z */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	auto_cal_filp = filp_open(DIGITAL_HALL_AUTO_CAL_Z_PATH, flag, mode);

	if (IS_ERR(auto_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[FACTORY] %s: open fail auto_cal_z_filp:%d\n",
			__func__, ret);
		kfree(write_buf);
		return ret;
	}

	snprintf(write_buf, AUTO_CAL_FILE_BUF_LEN,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		auto_cal_data->ref_z[0], auto_cal_data->ref_z[1],
		auto_cal_data->ref_z[2], auto_cal_data->ref_z[3],
		auto_cal_data->ref_z[4], auto_cal_data->ref_z[5],
		auto_cal_data->ref_z[6], auto_cal_data->ref_z[7],
		auto_cal_data->ref_z[8], auto_cal_data->ref_z[9],
		auto_cal_data->ref_z[10], auto_cal_data->ref_z[11],
		auto_cal_data->ref_z[12], auto_cal_data->ref_z[13],
		auto_cal_data->ref_z[14], auto_cal_data->ref_z[15],
		auto_cal_data->ref_z[16], auto_cal_data->ref_z[17],
		auto_cal_data->ref_z[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		AUTO_CAL_FILE_BUF_LEN * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[FACTORY] %s: auto_cal_z fd write:%d\n",
			__func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);

	kfree(write_buf);

	pr_info("[FACTORY] %s: saved", __func__);
	return ret;
}

static ssize_t check_auto_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s\n", __func__);

	mutex_lock(&data->digital_hall_mutex);
	/* Try to backup auto cal table */
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < 10)
		msleep(30);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= 10) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}


	auto_cal_data->flg_update = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0];
	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, auto_cal_data->flg_update);


	if (auto_cal_data->flg_update) {
		memcpy(auto_cal_data->ref_x, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1], sizeof(int32_t) * 19);
		memcpy(auto_cal_data->ref_y, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][20], sizeof(int32_t) * 19);
		memcpy(auto_cal_data->ref_z, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][39], sizeof(int32_t) * 19);
		set_auto_cal_data_forced(false);
	}

#ifdef CONFIG_SEC_FACTORY
	/* Print mx, my, mz buffer in SSC_DAEMON log */
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_CAL_DATA);
#endif
	return snprintf(buf, PAGE_SIZE, "%d\n", auto_cal_data->flg_update);
}

static ssize_t backup_restore_auto_cal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int new_value;
	int32_t auto_cal_buf[58] = { 0, };
	uint8_t cnt = 0;
#ifdef FORCE_SSR_FOR_TIMEOUT
	static uint8_t timeout_cnt;
	static uint8_t ssr_cnt;
#endif
	if (sysfs_streq(buf, "0"))
		new_value = 0;
	else if (sysfs_streq(buf, "1"))
		new_value = 1;
	else
		return size;

	pr_info("[FACTORY] %s: new_value %d\n", __func__, new_value);

	if (new_value) {
		mutex_lock(&data->digital_hall_mutex);
		adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_CAL_DATA);

		while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
			cnt++ < 3)
			msleep(30);

		data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
		mutex_unlock(&data->digital_hall_mutex);

		if (cnt >= 3) {
#ifdef FORCE_SSR_FOR_TIMEOUT
			if (ssr_cnt < MAX_SSR_LIMIT) {
				timeout_cnt++;

				pr_err("[FACTORY] %s: Timeout (%d, %d)!!!\n", __func__, timeout_cnt, ssr_cnt);
				if (timeout_cnt == SSR_TRIGGER_CNT) {
					timeout_cnt = 0;
					ssr_cnt++;
					slpi_ssr();
				}
			} else {
				pr_err("[FACTORY] %s: Timeout (%d, %d)!!!\n", __func__, timeout_cnt, ssr_cnt);
			}
#else
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
#endif
			return size;
		}

#ifdef FORCE_SSR_FOR_TIMEOUT
		timeout_cnt = 0;
		ssr_cnt = 0;
#endif
		pr_info("[FACTORY] %s: flg_update=%d\n", __func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0]);

		if (!data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0])
			return size;

		auto_cal_data->flg_update = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0];
		memcpy(auto_cal_data->ref_x, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1], sizeof(int32_t) * 19);
		memcpy(auto_cal_data->ref_y, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][20], sizeof(int32_t) * 19);
		memcpy(auto_cal_data->ref_z, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][39], sizeof(int32_t) * 19);

		pr_info("[FACTORY] %s: backup auto_cal\n", __func__);
		pr_info("[FACTORY] %s: %d/%d/%d/%d\n", __func__,
			auto_cal_data->flg_update, auto_cal_data->ref_x[18],
			auto_cal_data->ref_y[18], auto_cal_data->ref_z[18]);
		set_auto_cal_data_forced(false);
#ifdef CONFIG_SEC_FACTORY
		queue_work(auto_cal_data->autocal_debug_wq,
			&auto_cal_data->work_autocal_debug);
#endif
	} else {
		if (auto_cal_data->flg_update == 0) {
			pr_info("[FACTORY] %s: flg_update is zero\n", __func__);
			return size;
		}
		auto_cal_buf[0] = auto_cal_data->flg_update;
		memcpy(&auto_cal_buf[1], auto_cal_data->ref_x, sizeof(int32_t) * 19);
		memcpy(&auto_cal_buf[20], auto_cal_data->ref_y, sizeof(int32_t) * 19);
		memcpy(&auto_cal_buf[39], auto_cal_data->ref_z, sizeof(int32_t) * 19);

		pr_info("[FACTORY] %s: restore auto_cal\n", __func__);
		pr_info("[FACTORY] %s: %d/%d/%d/%d\n", __func__,
			auto_cal_buf[0], auto_cal_buf[1],
			auto_cal_buf[20], auto_cal_buf[39]);
		adsp_unicast(auto_cal_buf, sizeof(auto_cal_buf),
			MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_REGISTER);
	}

	return size;
}

static ssize_t rf_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s - init %d, min %d, max %d\n", __func__,
		auto_cal_data->init_angle,
		auto_cal_data->min_angle,
		auto_cal_data->max_angle);

	return snprintf(buf, PAGE_SIZE,	"%d,%d,%d\n",
		auto_cal_data->init_angle,
		auto_cal_data->min_angle,
		auto_cal_data->max_angle);
}

static ssize_t rf_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int new_value;
	int32_t angle = 0;

	if (sysfs_streq(buf, "1"))
		new_value = 1;
	else
		new_value = 0;

	if (new_value == auto_cal_data->rftest_timer_enabled)
		return size;

	if (new_value == 1) {
		auto_cal_data->rftest_timer_enabled = 1;
		get_hall_angle_data(&angle);
		auto_cal_data->init_angle = angle;
		hrtimer_start(&auto_cal_data->rftest_timer,
			ns_to_ktime(2000 * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	} else {
		auto_cal_data->rftest_timer_enabled = 0;
		hrtimer_cancel(&auto_cal_data->rftest_timer);
		cancel_work_sync(&auto_cal_data->work_rftest);
		auto_cal_data->init_angle = 0;
		auto_cal_data->min_angle = 180;
		auto_cal_data->max_angle = 0;
	}

	pr_info("[FACTORY] %s - %d. init_angle %d\n", __func__,
		auto_cal_data->rftest_timer_enabled,
		auto_cal_data->init_angle);

	return size;
}

static ssize_t lf_stream_reset_auto_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int reset_data[58] = { 0, };
	uint8_t cnt = 0;

	mutex_lock(&data->digital_hall_mutex);
	/* reset */
	adsp_unicast(reset_data, sizeof(reset_data),
		MSG_LF_STREAM, 0, MSG_TYPE_SET_REGISTER);

	/* read */
	adsp_unicast(NULL, 0, MSG_LF_STREAM, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_LF_STREAM) &&
		cnt++ < 3)
		msleep(30);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_LF_STREAM);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= 3) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, data->msg_buf[MSG_LF_STREAM][0]);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		data->msg_buf[MSG_LF_STREAM][0]);
}

int set_lf_stream_auto_cal_data(bool first_booting)
{
	struct file *auto_cal_filp = NULL;
	mm_segment_t old_fs;
	int flag, ret = 0;
	umode_t mode = 0;
	char *write_buf = kzalloc(AUTO_CAL_FILE_BUF_LEN, GFP_KERNEL);

	if (first_booting) {
		flag = O_TRUNC | O_RDWR | O_CREAT;
		mode = 0600;
	} else {
		flag = O_RDWR;
		mode = 0660;
	}

	/* auto_cal X */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	auto_cal_filp = filp_open(LF_STREAM_AUTO_CAL_X_PATH, flag, mode);

	if (IS_ERR(auto_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[FACTORY] %s: open fail lf_stream auto_cal_x_filp:%d\n",
			__func__, ret);
		kfree(write_buf);
		return ret;
	}

	snprintf(write_buf, AUTO_CAL_FILE_BUF_LEN,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		pdata->flg_update, pdata->ref_x[0], pdata->ref_x[1],
		pdata->ref_x[2], pdata->ref_x[3], pdata->ref_x[4],
		pdata->ref_x[5], pdata->ref_x[6], pdata->ref_x[7],
		pdata->ref_x[8], pdata->ref_x[9], pdata->ref_x[10],
		pdata->ref_x[11], pdata->ref_x[12], pdata->ref_x[13],
		pdata->ref_x[14], pdata->ref_x[15], pdata->ref_x[16],
		pdata->ref_x[17], pdata->ref_x[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		AUTO_CAL_FILE_BUF_LEN * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[FACTORY] %s: lf_stream auto_cal_x fd write:%d\n",
			__func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);

	/* auto_cal Y */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	auto_cal_filp = filp_open(LF_STREAM_AUTO_CAL_Y_PATH, flag, mode);

	if (IS_ERR(auto_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[FACTORY] %s: open fail lf_stream auto_cal_y_filp:%d\n",
			__func__, ret);
		kfree(write_buf);
		return ret;
	}

	snprintf(write_buf, AUTO_CAL_FILE_BUF_LEN,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		pdata->ref_y[0], pdata->ref_y[1], pdata->ref_y[2],
		pdata->ref_y[3], pdata->ref_y[4], pdata->ref_y[5],
		pdata->ref_y[6], pdata->ref_y[7], pdata->ref_y[8],
		pdata->ref_y[9], pdata->ref_y[10], pdata->ref_y[11],
		pdata->ref_y[12], pdata->ref_y[13], pdata->ref_y[14],
		pdata->ref_y[15], pdata->ref_y[16], pdata->ref_y[17],
		pdata->ref_y[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		AUTO_CAL_FILE_BUF_LEN * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[FACTORY] %s: lf_stream auto_cal_y fd write:%d\n",
			__func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);

	/* auto_cal Z */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	auto_cal_filp = filp_open(LF_STREAM_AUTO_CAL_Z_PATH, flag, mode);

	if (IS_ERR(auto_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[FACTORY] %s: open fail lf_stream auto_cal_z_filp:%d\n",
			__func__, ret);
		kfree(write_buf);
		return ret;
	}

	snprintf(write_buf, AUTO_CAL_FILE_BUF_LEN,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		pdata->ref_z[0], pdata->ref_z[1], pdata->ref_z[2],
		pdata->ref_z[3], pdata->ref_z[4], pdata->ref_z[5],
		pdata->ref_z[6], pdata->ref_z[7], pdata->ref_z[8],
		pdata->ref_z[9], pdata->ref_z[10], pdata->ref_z[11],
		pdata->ref_z[12], pdata->ref_z[13], pdata->ref_z[14],
		pdata->ref_z[15], pdata->ref_z[16], pdata->ref_z[17],
		pdata->ref_z[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		AUTO_CAL_FILE_BUF_LEN * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[FACTORY] %s: lf_stream auto_cal_z fd write:%d\n",
			__func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);

	kfree(write_buf);

	pr_info("[FACTORY] %s: saved", __func__);
	return ret;
}

static ssize_t lf_stream_auto_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s\n", __func__);

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_LF_STREAM, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_LF_STREAM) &&
		cnt++ < 3)
		msleep(30);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_LF_STREAM);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= 3) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE,
		"lf_stream autocal save failed\n");
	}

	pdata->flg_update = data->msg_buf[MSG_LF_STREAM][0];
	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, pdata->flg_update);

	if (!pdata->flg_update)
		return snprintf(buf, PAGE_SIZE, "flg_update is not true\n");

	memcpy(pdata->ref_x, &data->msg_buf[MSG_LF_STREAM][1], sizeof(int32_t) * 19);
	memcpy(pdata->ref_y, &data->msg_buf[MSG_LF_STREAM][20], sizeof(int32_t) * 19);
	memcpy(pdata->ref_z, &data->msg_buf[MSG_LF_STREAM][39], sizeof(int32_t) * 19);

	set_lf_stream_auto_cal_data(false);

	return snprintf(buf, PAGE_SIZE,
		"lf_stream autocal was saved in file system\n");
}

static ssize_t lf_stream_auto_cal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	int tmp[58] = { 0, };
	char *auto_cal_buf = kzalloc(AUTO_CAL_FILE_BUF_LEN * sizeof(char),
		GFP_KERNEL);

	pr_info("[FACTORY] %s - lf_stream autocal restor start!!!\n", __func__);

	/* auto_cal X */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(LF_STREAM_AUTO_CAL_X_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s - no lf_stream_auto_cal_x file\n",
			__func__);
		set_fs(old_fs);
		set_lf_stream_auto_cal_data(true);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error: lf_stream_auto_cal_x\n",
			__func__);
		set_fs(old_fs);
	} else {
		pr_info("[FACTORY] %s - already exist: lf_stream_auto_cal_x\n", __func__);

		ret = vfs_read(cal_filp, (char *)auto_cal_buf,
			AUTO_CAL_FILE_BUF_LEN * sizeof(char), &cal_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s - read fail:%d\n", __func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return size;
		}

		ret = sscanf(auto_cal_buf,
			"%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
			&tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5],
			&tmp[6], &tmp[7], &tmp[8], &tmp[9], &tmp[10], &tmp[11],
			&tmp[12], &tmp[13], &tmp[14], &tmp[15], &tmp[16], &tmp[17],
			&tmp[18], &tmp[19]);


		if (ret != AUTO_CAL_DATA_NUM + 1) {
			pr_err("[FACTORY] %s - lf_stream_auto_cal_x: sscanf fail %d\n",
				__func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return size;
		}

		pdata->flg_update = tmp[0];
		memcpy(pdata->ref_x, &tmp[1], sizeof(int32_t) * 19);
		pr_info("[FACTORY] %s: flg_update=%d\n",
			__func__, pdata->flg_update);
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
	}

	/* auto_cal Y */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(LF_STREAM_AUTO_CAL_Y_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s - no lf_stream_auto_cal_y file\n",
			__func__);
		set_fs(old_fs);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error: lf_stream_auto_cal_y\n",
			__func__);
		set_fs(old_fs);
	} else {
		pr_info("[FACTORY] %s - already exist: lf_stream_auto_cal_y\n", __func__);

		ret = vfs_read(cal_filp, (char *)auto_cal_buf,
			AUTO_CAL_FILE_BUF_LEN * sizeof(char), &cal_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s - read fail:%d\n", __func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return size;
		}

		ret = sscanf(auto_cal_buf,
			"%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
			&tmp[20], &tmp[21], &tmp[22], &tmp[23], &tmp[24],
			&tmp[25], &tmp[26], &tmp[27], &tmp[28], &tmp[29],
			&tmp[30], &tmp[31], &tmp[32], &tmp[33], &tmp[34],
			&tmp[35], &tmp[36], &tmp[37], &tmp[38]);


		if (ret != AUTO_CAL_DATA_NUM) {
			pr_err("[FACTORY] %s - lf_stream_auto_cal_y: sscanf fail %d\n",
				__func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return size;
		}

		memcpy(pdata->ref_y, &tmp[20], sizeof(int32_t) * 19);
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
	}

	/* auto_cal Z */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(LF_STREAM_AUTO_CAL_Z_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s - no lf_stream_auto_cal_z file\n",
			__func__);
		set_fs(old_fs);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error: lf_stream_auto_cal_z\n",
			__func__);
		set_fs(old_fs);
	} else {
		pr_info("[FACTORY] %s - already exist: lf_stream_auto_cal_z\n", __func__);

		ret = vfs_read(cal_filp, (char *)auto_cal_buf,
			AUTO_CAL_FILE_BUF_LEN * sizeof(char), &cal_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s - read fail:%d\n", __func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return size;
		}

		ret = sscanf(auto_cal_buf,
			"%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
			&tmp[39], &tmp[40], &tmp[41], &tmp[42], &tmp[43],
			&tmp[44], &tmp[45], &tmp[46], &tmp[47], &tmp[48],
			&tmp[49], &tmp[50], &tmp[51], &tmp[52], &tmp[53],
			&tmp[54], &tmp[55], &tmp[56], &tmp[57]);


		if (ret != AUTO_CAL_DATA_NUM) {
			pr_err("[FACTORY] %s - lf_stream_auto_cal_z: sscanf fail %d\n",
				__func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return size;
		}

		memcpy(pdata->ref_z, &tmp[39], sizeof(int32_t) * 19);
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
	}
	kfree(auto_cal_buf);

	adsp_unicast(tmp, sizeof(tmp),
		MSG_LF_STREAM, 0, MSG_TYPE_SET_REGISTER);

	pr_info("[FACTORY] %s - lf_stream autocal was restored!!!\n", __func__);
	return size;
}

static ssize_t flexcover_thd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: Curr THD %d/%d\n", __func__,
		auto_cal_data->flex_low, auto_cal_data->flex_cover_hi);

	return snprintf(buf, PAGE_SIZE,	"%d,%d\n",
		auto_cal_data->flex_low, auto_cal_data->flex_cover_hi);
}

static ssize_t flexcover_thd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int cnt = 0;
	int32_t msg_buf[1];

	if (sscanf(buf, "%2d", &msg_buf[0]) != 1) {
		pr_err("[FACTORY]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s: msg_buf[0] = %d\n", __func__, msg_buf[0]);

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_THRESHOLD);

	while (!(data->ready_flag[MSG_TYPE_SET_THRESHOLD] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_THRESHOLD] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		auto_cal_data->flex_low = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0];
		auto_cal_data->flex_cover_hi = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1];
	}

	pr_info("[FACTORY] %s: New THD %d/%d\n", __func__,
		auto_cal_data->flex_low, auto_cal_data->flex_cover_hi);

	mutex_unlock(&data->digital_hall_mutex);

	return size;
}

static DEVICE_ATTR(name, 0444, digital_hall_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, digital_hall_vendor_show, NULL);
static DEVICE_ATTR(selftest, 0440, digital_hall_selftest_show, NULL);
static DEVICE_ATTR(spec, 0440, digital_hall_spec_show, NULL);
static DEVICE_ATTR(test_spec, 0440, digital_hall_test_spec_show, NULL);
static DEVICE_ATTR(ref_angle, 0440, digital_hall_ref_angle_show, NULL);
static DEVICE_ATTR(read_data, 0440, digital_hall_read_data_show, NULL);
static DEVICE_ATTR(test_read, 0660,
	digital_hall_test_read_show, digital_hall_test_read_store);
static DEVICE_ATTR(reset_auto_cal, 0440, reset_auto_cal_show, NULL);
static DEVICE_ATTR(check_auto_cal, 0440, check_auto_cal_show, NULL);
static DEVICE_ATTR(backup_restore_auto_cal, 0220,
	NULL, backup_restore_auto_cal_store);
static DEVICE_ATTR(lf_stream_reset_auto_cal, 0440,
	lf_stream_reset_auto_cal_show, NULL);
static DEVICE_ATTR(lf_stream_auto_cal, 0660,
	lf_stream_auto_cal_show, lf_stream_auto_cal_store);
static DEVICE_ATTR(rf_test, 0660, rf_test_show, rf_test_store);
static DEVICE_ATTR(flexcover_thd, 0660, flexcover_thd_show, flexcover_thd_store);

static struct device_attribute *digital_hall_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_selftest,
	&dev_attr_spec,
	&dev_attr_test_spec,
	&dev_attr_ref_angle,
	&dev_attr_read_data,
	&dev_attr_test_read,
	&dev_attr_reset_auto_cal,
	&dev_attr_check_auto_cal,
	&dev_attr_backup_restore_auto_cal,
	&dev_attr_lf_stream_auto_cal,
	&dev_attr_lf_stream_reset_auto_cal,
	&dev_attr_rf_test,
	&dev_attr_flexcover_thd,
	NULL,
};

static int __init ak09970_factory_init(void)
{
	adsp_factory_register(MSG_DIGITAL_HALL, digital_hall_attrs);
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	auto_cal_data = kzalloc(sizeof(*auto_cal_data), GFP_KERNEL);

	hrtimer_init(&auto_cal_data->rftest_timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	auto_cal_data->rftest_timer.function = rftest_timer_func;
	auto_cal_data->rftest_wq =
		create_singlethread_workqueue("hall_angle_rftest_wq");
	INIT_WORK(&auto_cal_data->work_rftest, rftest_work_func);

	auto_cal_data->init_angle = 0;
	auto_cal_data->min_angle = 180;
	auto_cal_data->max_angle = 0;
	auto_cal_data->rftest_timer_enabled = 0;
	auto_cal_data->flex_low = 80;
	auto_cal_data->flex_cover_hi = 70;

#ifdef CONFIG_SEC_FACTORY
	auto_cal_data->autocal_debug_wq =
		create_singlethread_workqueue("autocal_dbg_wq");
	if (auto_cal_data->autocal_debug_wq == NULL) {
		pr_err("[FACTORY]: %s - could not create autocal_dbg_wq",
			__func__);
	}
	INIT_WORK(&auto_cal_data->work_autocal_debug, autocal_debug_work_func);
#endif
	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

static void __exit ak09970_factory_exit(void)
{
	if (auto_cal_data->rftest_timer_enabled == 1) {
		hrtimer_cancel(&auto_cal_data->rftest_timer);
		cancel_work_sync(&auto_cal_data->work_rftest);
	}
	destroy_workqueue(auto_cal_data->rftest_wq);

#ifdef CONFIG_SEC_FACTORY
	if (auto_cal_data->autocal_debug_wq)
		destroy_workqueue(auto_cal_data->autocal_debug_wq);
#endif
	adsp_factory_unregister(MSG_DIGITAL_HALL);
	if (pdata)
		kfree(pdata);
	if (auto_cal_data)
		kfree(auto_cal_data);

	pr_info("[FACTORY] %s\n", __func__);
}

module_init(ak09970_factory_init);
module_exit(ak09970_factory_exit);
