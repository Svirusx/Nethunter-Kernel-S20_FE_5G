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
#include <linux/adsp/slpi_motor.h>
#include <linux/adsp/ssc_ssr_reason.h>
#include <linux/string.h>
#ifdef CONFIG_VBUS_NOTIFIER
#include <linux/vbus_notifier.h> 
#endif 

#ifdef CONFIG_SUPPORT_DEVICE_MODE
#include <linux/hall.h>
#endif
#ifdef CONFIG_SUPPORT_SSC_SPU
#include <linux/adsp/ssc_spu.h>
#endif
#include "adsp.h"

#ifdef CONFIG_SUPPORT_SSC_MODE
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#endif

#include <linux/timekeeping32.h>
#include <linux/rtc.h>

#include <linux/sensor/sensors_core.h>
#include <linux/notifier.h>

#define HISTORY_CNT 5
#define NO_SSR 0xFF
#define SSR_REASON_LEN	128
#define TIME_LEN	24
#ifdef CONFIG_SEC_FACTORY
#define SLPI_STUCK "SLPI_STUCK"
#define SLPI_PASS "SLPI_PASS"
#define PROBE_FAIL "PROBE_FAIL"
#endif

#ifdef CONFIG_SUPPORT_DUAL_6AXIS
#define SUPPORT_DUAL_SENSOR "DUAL_GYRO"
#else
#define SUPPORT_DUAL_SENSOR "SINGLE_GYRO"
#endif

#define SENSOR_DUMP_CNT 4
#define SENSOR_DUMP_DONE "SENSOR_DUMP_DONE"

#define SENSOR_HW_REVISION_MAX 31

#ifdef CONFIG_SUPPORT_SSC_MODE
#ifdef CONFIG_SUPPORT_SSC_MODE_FOR_MAG
#define ANT_NFC_MST  0
#define ANT_NFC_ONLY 1
#define ANT_DUMMY    2
#endif
#endif

static int pid;
static char panic_msg[SSR_REASON_LEN];
static char ssr_history[HISTORY_CNT][TIME_LEN + SSR_REASON_LEN];
static unsigned char ssr_idx = NO_SSR;

#if defined(CONFIG_SUPPORT_DEVICE_MODE) || defined(CONFIG_SUPPORT_VIRTUAL_OPTIC)
static int32_t curr_fstate;
#endif

struct sdump_data {
	struct workqueue_struct *sdump_wq;
	struct work_struct work_sdump;
	struct adsp_data *dev_data;
};
struct sdump_data *pdata_sdump;

#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_SUPPORT_DUAL_6AXIS)
extern bool is_pretest(void);
#endif

static unsigned int sec_hw_rev(void);

#ifdef CONFIG_SUPPORT_AK0997X
#define SUPPORT_ANALOG_HALL (5)
struct ak09970_digital_hall_data {
	int32_t ref_x[19];
	int32_t ref_y[19];
	int32_t ref_z[19];
	int32_t flg_update;
	bool support_a_hall;
};
static struct ak09970_digital_hall_data *pdata;

void init_digital_hall_data()
{
	int i;

	for (i = 0; i < 19; i++) {
		pdata->ref_x[i] = 0;
		pdata->ref_y[i] = 0;
		pdata->ref_z[i] = 0;
	}

	pdata->flg_update = 0;
	pdata->support_a_hall = true;
}

int set_digital_hall_auto_cal_data(bool first_booting)
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
		pr_err("[FACTORY] %s: open fail dhall auto_cal_x_filp:%d\n",
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
		pr_err("[FACTORY] %s: auto_cal_x fd write:%d\n", __func__, ret);

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
		pr_err("[FACTORY] %s: open fail dhall auto_cal_y_filp:%d\n",
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
		pr_err("[FACTORY] %s: auto_cal_y fd write:%d\n", __func__, ret);

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
		pr_err("[FACTORY] %s: open fail dhall auto_cal_z_filp:%d\n",
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
		pr_err("[FACTORY] %s: auto_cal_z fd write:%d\n", __func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);

	kfree(write_buf);

	pr_info("[FACTORY] %s: saved", __func__);
	return ret;
}

void digital_hall_factory_auto_cal_init_work(void)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	int buf[58] = { 0, };
	char *auto_cal_buf = kzalloc(AUTO_CAL_FILE_BUF_LEN * sizeof(char),
		GFP_KERNEL);

	if (sec_hw_rev() < SUPPORT_ANALOG_HALL)
		pdata->support_a_hall = false;

	/* auto_cal X */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(DIGITAL_HALL_AUTO_CAL_X_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s - no digital_hall_auto_cal_x file\n",
			__func__);
		set_fs(old_fs);
		set_digital_hall_auto_cal_data(true);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error: auto_cal_x\n",
			__func__);
		set_fs(old_fs);
	} else {
		pr_info("[FACTORY] %s - already exist: auto_cal_x\n", __func__);

		ret = vfs_read(cal_filp, (char *)auto_cal_buf,
			AUTO_CAL_FILE_BUF_LEN * sizeof(char), &cal_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s - read fail:%d\n", __func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return;
		}

		ret = sscanf(auto_cal_buf,
			"%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
			&buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5],
			&buf[6], &buf[7], &buf[8], &buf[9], &buf[10], &buf[11],
			&buf[12], &buf[13], &buf[14], &buf[15], &buf[16], &buf[17],
			&buf[18], &buf[19]);


		if (ret != AUTO_CAL_DATA_NUM + 1) {
			pr_err("[FACTORY] %s - auto_cal_x: sscanf fail %d\n",
				__func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return;
		}

		pdata->flg_update = buf[0];
		memcpy(pdata->ref_x, &buf[1], sizeof(int32_t) * 19);
		pr_info("[FACTORY] %s: flg_update=%d\n",
			__func__, pdata->flg_update);
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
	}

	/* auto_cal Y */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(DIGITAL_HALL_AUTO_CAL_Y_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s - no digital_hall_auto_cal_y file\n",
			__func__);
		set_fs(old_fs);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error: auto_cal_y\n",
			__func__);
		set_fs(old_fs);
	} else {
		pr_info("[FACTORY] %s - already exist: auto_cal_y\n", __func__);

		ret = vfs_read(cal_filp, (char *)auto_cal_buf,
			AUTO_CAL_FILE_BUF_LEN * sizeof(char), &cal_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s - read fail:%d\n", __func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return;
		}

		ret = sscanf(auto_cal_buf,
			"%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
			&buf[20], &buf[21], &buf[22], &buf[23],	&buf[24],
			&buf[25], &buf[26], &buf[27], &buf[28], &buf[29],
			&buf[30], &buf[31], &buf[32], &buf[33], &buf[34],
			&buf[35], &buf[36], &buf[37], &buf[38]);


		if (ret != AUTO_CAL_DATA_NUM) {
			pr_err("[FACTORY] %s - auto_cal_y: sscanf fail %d\n",
				__func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return;
		}

		memcpy(pdata->ref_y, &buf[20], sizeof(int32_t) * 19);
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
	}

	/* auto_cal Z */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(DIGITAL_HALL_AUTO_CAL_Z_PATH, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s - no digital_hall_auto_cal_z file\n",
			__func__);
		set_fs(old_fs);
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error: auto_cal_z\n",
			__func__);
		set_fs(old_fs);
	} else {
		pr_info("[FACTORY] %s - already exist: auto_cal_z\n", __func__);

		ret = vfs_read(cal_filp, (char *)auto_cal_buf,
			AUTO_CAL_FILE_BUF_LEN * sizeof(char), &cal_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s - read fail:%d\n", __func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return;
		}

		ret = sscanf(auto_cal_buf,
			"%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
			&buf[39], &buf[40], &buf[41], &buf[42], &buf[43],
			&buf[44], &buf[45], &buf[46], &buf[47],	&buf[48],
			&buf[49], &buf[50], &buf[51], &buf[52], &buf[53],
			&buf[54], &buf[55], &buf[56], &buf[57]);


		if (ret != AUTO_CAL_DATA_NUM) {
			pr_err("[FACTORY] %s - auto_cal_z: sscanf fail %d\n",
				__func__, ret);
			filp_close(cal_filp, current->files);
			set_fs(old_fs);
			kfree(auto_cal_buf);
			return;
		}

		memcpy(pdata->ref_z, &buf[39], sizeof(int32_t) * 19);
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
	}
	kfree(auto_cal_buf);

	adsp_unicast(buf, sizeof(buf),
		MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_REGISTER);
}
#endif
/*
 * send ssr notice to ois mcu
 */
#define NO_OIS_STRUCT (-1)

#if defined(CONFIG_SUPPORT_DEVICE_MODE) && defined(CONFIG_SUPPORT_DUAL_OPTIC)
struct ssc_flip_data {
	struct workqueue_struct *ssc_flip_wq;
	struct work_struct work_ssc_flip;
	bool is_opend;
	bool only_update;
};
struct ssc_flip_data *pdata_ssc_flip;
#endif

#ifdef CONFIG_VBUS_NOTIFIER
#define CHARGE_START	true
#define CHARGE_END	false

struct ssc_charge_data {
	struct workqueue_struct *ssc_charge_wq;
	struct work_struct work_ssc_charge;
	bool is_charging;
};
struct ssc_charge_data *pdata_ssc_charge;
#endif

struct ois_sensor_interface{
	void *core;
	void (*ois_func)(void *);
};

static struct ois_sensor_interface ois_reset;

int ois_reset_register(struct ois_sensor_interface *ois)
{
	if (ois->core)
		ois_reset.core = ois->core;

	if (ois->ois_func)
		ois_reset.ois_func = ois->ois_func;

	if (!ois->core || !ois->ois_func) {
		pr_info("[FACTORY] %s - no ois struct\n", __func__);
		return NO_OIS_STRUCT;
	}

	return 0;
}
EXPORT_SYMBOL(ois_reset_register);

void ois_reset_unregister(void)
{
	ois_reset.core = NULL;
	ois_reset.ois_func = NULL;
}
EXPORT_SYMBOL(ois_reset_unregister);

#ifdef CONFIG_VBUS_NOTIFIER
void ssc_charge_work_func(struct work_struct *work)
{
	int32_t msg_buf[2];
	msg_buf[0] = OPTION_TYPE_SSC_CHARGING_STATE;
	msg_buf[1] = (int32_t)pdata_ssc_charge->is_charging;
	pr_info("[FACTORY] %s: msg_buf = %d\n", __func__, msg_buf[1]);
	adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
}

static int ssc_core_vbus_notifier(struct notifier_block *nb,
				unsigned long action, void *data)
{
	vbus_status_t vbus_type = *(vbus_status_t *) data;
	static int vbus_pre_attach;

	if (vbus_pre_attach == vbus_type)
		return 0;

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
	case STATUS_VBUS_LOW:
		pdata_ssc_charge->is_charging = 
			(vbus_type == STATUS_VBUS_HIGH) ? true : false;
		pr_info("vbus high:%d \n", (int)pdata_ssc_charge->is_charging);
		queue_work(pdata_ssc_charge->ssc_charge_wq,
			&pdata_ssc_charge->work_ssc_charge);                       
		break;
	default:
		pr_info("vbus skip attach = %d\n", vbus_type);
		break;
	}

	vbus_pre_attach = vbus_type;

	return 0;
}

void sns_vbus_init_work(void)
{
	pr_info("sns_vbus_init_work:%d \n", (int)pdata_ssc_charge->is_charging);
	queue_work(pdata_ssc_charge->ssc_charge_wq,
		&pdata_ssc_charge->work_ssc_charge);
}
#endif

/*************************************************************************/
/* factory Sysfs							 */
/*************************************************************************/

static char operation_mode_flag[11];
#if defined(CONFIG_SEC_WINNERLTE_PROJECT) || defined(CONFIG_SEC_WINNERX_PROJECT)
static int get_prox_sidx(struct adsp_data *data)
{
	int ret = MSG_PROX;
#ifdef CONFIG_SUPPORT_DUAL_OPTIC
	switch (data->fac_fstate) {
	case FSTATE_INACTIVE:
	case FSTATE_FAC_INACTIVE:
	case FSTATE_FAC_INACTIVE_2:
		ret = MSG_PROX;
		break;
	case FSTATE_ACTIVE:
	case FSTATE_FAC_ACTIVE:
		ret = MSG_PROX_SUB;
		break;
	default:
		break;
	}
#endif
	return ret;
}
#endif

#ifdef CONFIG_SUPPORT_SSC_SPU
extern int ver_buf[SSC_CNT_MAX][SSC_VC_MAX];
extern int fw_idx;
#endif
static ssize_t dumpstate_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t type[1];

	if (pid != 0) {
		pr_info("[FACTORY] to take the logs\n");
		type[0] = 0;
	} else {
		type[0] = 2;
		pr_info("[FACTORY] logging service was stopped %d\n", pid);
	}

	pr_info("[FACTORY] %s support_algo = %u\n", __func__, data->support_algo);

	return snprintf(buf, PAGE_SIZE, "SSC_CORE\n");
}

static ssize_t operation_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s", operation_mode_flag);
}

static ssize_t operation_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < 10 && buf[i] != '\0'; i++)
		operation_mode_flag[i] = buf[i];
	operation_mode_flag[i] = '\0';

	if (!strncmp(operation_mode_flag, "restrict", 8)) {
		pr_info("[FACTORY] %s: set restrict_mode\n", __func__);
		data->restrict_mode = true;
	} else {
		pr_info("[FACTORY] %s: set normal_mode\n", __func__);
		data->restrict_mode = false;
	}
	pr_info("[FACTORY] %s: operation_mode_flag = %s\n", __func__,
		operation_mode_flag);
	return size;
}

static ssize_t mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	unsigned long timeout;
	unsigned long timeout_2;
	int32_t type[1];

	if (pid != 0) {
		timeout = jiffies + (2 * HZ);
		timeout_2 = jiffies + (10 * HZ);
		type[0] = 1;
		pr_info("[FACTORY] To stop logging %d\n", pid);
		adsp_unicast(type, sizeof(type), MSG_SSC_CORE, 0, MSG_TYPE_DUMPSTATE);

		while (pid != 0) {
			msleep(25);
			if (time_after(jiffies, timeout))
				pr_info("[FACTORY] %s: Timeout!!!\n", __func__);
			if (time_after(jiffies, timeout_2)) {
//				panic("force crash : ssc core\n");
				pr_info("[FACTORY] pid %d\n", pid);
				return snprintf(buf, PAGE_SIZE, "1\n");
			}
		}
	}

	return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int data = 0;
	int32_t type[1];
	if (kstrtoint(buf, 10, &data)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	if (data != 1) {
		pr_err("[FACTORY] %s: data was wrong\n", __func__);
		return -EINVAL;
	}

	if (pid != 0) {
		type[0] = 1;
		adsp_unicast(type, sizeof(type), MSG_SSC_CORE, 0, MSG_TYPE_DUMPSTATE);
	}

	return size;
}

static ssize_t pid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", pid);
}

static ssize_t pid_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int data = 0;

	if (kstrtoint(buf, 10, &data)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	pid = data;
	pr_info("[FACTORY] %s: pid %d\n", __func__, pid);

	return size;
}

static ssize_t remove_sensor_sysfs_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int type = MSG_SENSOR_MAX;
	struct adsp_data *data = dev_get_drvdata(dev);

	if (kstrtouint(buf, 10, &type)) {
		pr_err("[FACTORY] %s: kstrtouint fail\n", __func__);
		return -EINVAL;
	}

	if (type > PHYSICAL_SENSOR_SYSFS) {
		pr_err("[FACTORY] %s: Invalid type %u\n", __func__, type);
		return size;
	}

	pr_info("[FACTORY] %s: type = %u\n", __func__, type);

	mutex_lock(&data->remove_sysfs_mutex);
	adsp_factory_unregister(type);
	mutex_unlock(&data->remove_sysfs_mutex);

	return size;
}

static unsigned int system_rev __read_mostly;

static int __init sec_hw_rev_setup(char *p)
{
	int ret;

	ret = kstrtouint(p, 0, &system_rev);
	if (unlikely(ret < 0)) {
		pr_warn("androidboot.revision is malformed (%s)\n", p);
		return -EINVAL;
	}

	pr_info("androidboot.revision %x\n", system_rev);

	return 0;
}
early_param("androidboot.revision", sec_hw_rev_setup);

static unsigned int sec_hw_rev(void)
{
	return system_rev;
}

int ssc_system_rev_test;
static ssize_t ssc_hw_rev_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t hw_rev = sec_hw_rev();

	if(hw_rev > SENSOR_HW_REVISION_MAX)
		hw_rev = SENSOR_HW_REVISION_MAX;

	pr_info("[FACTORY] %s: ssc_rev:%d\n", __func__, hw_rev);

	if (!ssc_system_rev_test)
		return snprintf(buf, PAGE_SIZE, "%d\n", hw_rev);
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", ssc_system_rev_test);
}

static ssize_t ssc_hw_rev_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtoint(buf, 10, &ssc_system_rev_test)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s: system_rev_ssc %d\n", __func__, ssc_system_rev_test);

	return size;
}


void ssr_reason_call_back(char reason[], int len)
{
	struct timespec ts;
	struct rtc_time tm;

	if (len <= 0) {
		pr_info("[FACTORY] ssr %d\n", len);
		return;
	}
	memset(panic_msg, 0, SSR_REASON_LEN);
	strlcpy(panic_msg, reason, min(len, (int)(SSR_REASON_LEN - 1)));

	pr_info("[FACTORY] ssr %s\n", panic_msg);

	if (ois_reset.ois_func != NULL && ois_reset.core != NULL) {
		ois_reset.ois_func(ois_reset.core);
		pr_info("[FACTORY] %s - send ssr notice to ois mcu\n", __func__);
	} else {
		pr_info("[FACTORY] %s - no ois struct\n", __func__);
	}

	if (ssr_idx == HISTORY_CNT || ssr_idx == NO_SSR)
		ssr_idx = 0;

	memset(ssr_history[ssr_idx], 0, TIME_LEN + SSR_REASON_LEN);

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);

	sprintf(ssr_history[ssr_idx], "[%d%02d%02d %02d:%02d:%02d] ",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	strlcat(ssr_history[ssr_idx++], panic_msg, TIME_LEN + SSR_REASON_LEN);
}

static ssize_t ssr_msg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#ifdef CONFIG_SEC_FACTORY
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t raw_data_acc[3] = {0, };
	int32_t raw_data_mag[3] = {0, };
	int ret_acc = 0;
	int ret_mag = 0;
#ifdef CONFIG_SUPPORT_DUAL_6AXIS
	if (is_pretest())
		return snprintf(buf, PAGE_SIZE, "%s\n", panic_msg);
#endif
	if (!data->sysfs_created[MSG_ACCEL] && !data->sysfs_created[MSG_MAG]
		&& !data->sysfs_created[MSG_PRESSURE]) {
		pr_err("[FACTORY] %s: sensor probe fail\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%s\n", PROBE_FAIL);
	}

	mutex_lock(&data->accel_factory_mutex);
	ret_acc = get_accel_raw_data(raw_data_acc);
	mutex_unlock(&data->accel_factory_mutex);

	ret_mag = get_mag_raw_data(raw_data_mag);

	pr_info("[FACTORY] %s: accel(%d, %d, %d), mag(%d, %d, %d)\n", __func__,
		raw_data_acc[0], raw_data_acc[1], raw_data_acc[2],
		raw_data_mag[0], raw_data_mag[1], raw_data_mag[2]);

	if (ret_acc == -1 && ret_mag == -1) {
		pr_err("[FACTORY] %s: SLPI stuck, check hal log\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%s\n", SLPI_STUCK);
	} else {
		return snprintf(buf, PAGE_SIZE, "%s\n", SLPI_PASS);
	}
#else
	return snprintf(buf, PAGE_SIZE, "%s\n", panic_msg);
#endif
}

static ssize_t ssr_reset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int32_t type[1];

	type[0] = 0xff;

	adsp_unicast(type, sizeof(type), MSG_SSC_CORE, 0, MSG_TYPE_DUMPSTATE);
	pr_info("[FACTORY] %s\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", "Success");
}

static ssize_t support_algo_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int support_algo = 0;
	struct adsp_data *data = dev_get_drvdata(dev);

	if (kstrtouint(buf, 10, &support_algo)) {
		pr_err("[FACTORY] %s: kstrtouint fail\n", __func__);
		return -EINVAL;
	}

        data->support_algo = (uint32_t)support_algo;
	pr_info("[FACTORY] %s support_algo = %u\n", __func__, support_algo);

	return size;
}

#ifdef CONFIG_SUPPORT_DEVICE_MODE
#ifdef CONFIG_SUPPORT_DUAL_OPTIC
void ssc_flip_work_func(struct work_struct *work)
{
	int32_t msg_buf[2];
	if (pdata_ssc_flip->only_update) {
		msg_buf[0] = VOPTIC_OP_CMD_SSC_FLIP_UPDATE;
		pdata_ssc_flip->only_update = false;
	} else {
		msg_buf[0] = VOPTIC_OP_CMD_SSC_FLIP;
	}
	msg_buf[1] = (int32_t)curr_fstate;
	pr_info("[FACTORY] %s: msg_buf = %d\n", __func__, msg_buf[1]);
	adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_VIR_OPTIC, 0, MSG_TYPE_OPTION_DEFINE);

#ifdef CONFIG_SUPPORT_DUAL_DDI_COPR_FOR_LIGHT_SENSOR
	msg_buf[0] += 10;
	adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_DDI, 0, MSG_TYPE_OPTION_DEFINE);
#endif
}

void sns_flip_init_work(void)
{
	pr_info("sns_flip_init_work:%d \n", (int)curr_fstate);
	queue_work(pdata_ssc_flip->ssc_flip_wq,
		&pdata_ssc_flip->work_ssc_flip);
}
#endif

int sns_device_mode_notify(struct notifier_block *nb,
	unsigned long flip_state, void *v)
{
	struct adsp_data *data = container_of(nb, struct adsp_data, adsp_nb);
	
	pr_info("[FACTORY] %s - before device mode curr:%d, fstate:%d",
		__func__, curr_fstate, data->fac_fstate);

	data->fac_fstate = curr_fstate = (int32_t)flip_state;
	pr_info("[FACTORY] %s - after device mode curr:%d, fstate:%d",
		__func__, curr_fstate, data->fac_fstate);

	if(curr_fstate == 0)
		adsp_unicast(NULL, 0, MSG_SSC_CORE, 0, MSG_TYPE_FACTORY_ENABLE);
	else
		adsp_unicast(NULL, 0, MSG_SSC_CORE, 0, MSG_TYPE_FACTORY_DISABLE);

#ifdef CONFIG_SUPPORT_DUAL_OPTIC
        // send the flip state by qmi.
	queue_work(pdata_ssc_flip->ssc_flip_wq, &pdata_ssc_flip->work_ssc_flip);
#endif
	return 0;
}

void sns_device_mode_init_work(void)
{
	if(curr_fstate == 0)
		adsp_unicast(NULL, 0, MSG_SSC_CORE, 0, MSG_TYPE_FACTORY_ENABLE);
	else
		adsp_unicast(NULL, 0, MSG_SSC_CORE, 0, MSG_TYPE_FACTORY_DISABLE);
}
#endif

#ifdef CONFIG_SUPPORT_VIRTUAL_OPTIC
static ssize_t fac_fstate_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t fstate[2] = {VOPTIC_OP_CMD_FAC_FLIP, 0};
	
	if (sysfs_streq(buf, "0"))
		data->fac_fstate = fstate[1] = curr_fstate;
	else if (sysfs_streq(buf, "1"))
		data->fac_fstate = fstate[1] = FSTATE_FAC_INACTIVE;
	else if (sysfs_streq(buf, "2"))
		data->fac_fstate = fstate[1] = FSTATE_FAC_ACTIVE;
	else if (sysfs_streq(buf, "3"))
		data->fac_fstate = fstate[1] = FSTATE_FAC_INACTIVE_2;
	else
		data->fac_fstate = fstate[1] = curr_fstate;

	adsp_unicast(fstate, sizeof(fstate),
		MSG_VIR_OPTIC, 0, MSG_TYPE_OPTION_DEFINE);
	pr_info("[FACTORY] %s - Factory flip state:%d",
		__func__, fstate[0]);

	return size;
}
#endif

#if defined(CONFIG_SUPPORT_DEVICE_MODE) && defined(CONFIG_SUPPORT_DUAL_OPTIC)
static ssize_t update_ssc_flip_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pdata_ssc_flip->only_update = true;
	queue_work(pdata_ssc_flip->ssc_flip_wq, &pdata_ssc_flip->work_ssc_flip);
	pr_info("[FACTORY] %s", __func__);

	return size;
}
#endif

static ssize_t support_dual_sensor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: %s\n", __func__, SUPPORT_DUAL_SENSOR);
	return snprintf(buf, PAGE_SIZE, "%s\n", SUPPORT_DUAL_SENSOR);
}

#if defined(CONFIG_SUPPORT_BHL_COMPENSATION_FOR_LIGHT_SENSOR) || \
	defined(CONFIG_SUPPORT_BRIGHT_SYSFS_COMPENSATION_LUX) || \
	defined(CONFIG_SUPPORT_AK0997X)
static ssize_t lcd_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int new_value;
#if defined(CONFIG_SUPPORT_BHL_COMPENSATION_FOR_LIGHT_SENSOR) || \
	defined(CONFIG_SUPPORT_BRIGHT_SYSFS_COMPENSATION_LUX)
	uint16_t light_idx = get_light_sidx(data);
	int32_t msg_buf[2];
#endif
#ifdef CONFIG_SUPPORT_AK0997X
	uint8_t cnt = 0;
#endif

	if (sysfs_streq(buf, "0"))
		new_value = 0;
	else if (sysfs_streq(buf, "1"))
		new_value = 1;
	else
		return size;

	pr_info("[FACTORY] %s: new_value %d\n", __func__, new_value);
#if defined(CONFIG_SUPPORT_BHL_COMPENSATION_FOR_LIGHT_SENSOR) || \
	defined(CONFIG_SUPPORT_BRIGHT_SYSFS_COMPENSATION_LUX)
	msg_buf[0] = OPTION_TYPE_LCD_ONOFF;
	msg_buf[1] = new_value;

	mutex_lock(&data->light_factory_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		light_idx, 0, MSG_TYPE_OPTION_DEFINE);
	mutex_unlock(&data->light_factory_mutex);
#endif
#ifdef CONFIG_SUPPORT_AK0997X
	if (new_value && !pdata->support_a_hall) {
		adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_CAL_DATA);

		while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
			cnt++ < 3)
			msleep(30);

		data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);

		if (cnt >= 3) {
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
			return size;
		}

		pdata->flg_update = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0];
		pr_info("[FACTORY] %s: flg_update=%d\n", __func__, pdata->flg_update);

		if (!pdata->flg_update)
			return size;

		memcpy(pdata->ref_x, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1], sizeof(int32_t) * 19);
		memcpy(pdata->ref_y, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][20], sizeof(int32_t) * 19);
		memcpy(pdata->ref_z, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][39], sizeof(int32_t) * 19);

		set_digital_hall_auto_cal_data(false);
	}
#endif
	return size;
}
#endif
static ssize_t algo_lcd_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int32_t msg_buf[2] = {0, };
	int cmd_type;

	if (kstrtoint(buf, 10, &cmd_type)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return size;
	}

	switch (cmd_type) {
	case COMMON_DATA_SET_ABS_ON:
	case COMMON_DATA_SET_ABS_OFF:
		msg_buf[0] = OPTION_TYPE_SSC_ABS_LCD_TYPE;
		msg_buf[1] = (cmd_type == COMMON_DATA_SET_ABS_ON) ? 1 : 0;
		break;
	case COMMON_DATA_SET_LCD_INTENT_ON:
	case COMMON_DATA_SET_LCD_INTENT_OFF:
		msg_buf[0] = OPTION_TYPE_SSC_LCD_INTENT_TYPE;
		msg_buf[1] = (cmd_type == COMMON_DATA_SET_LCD_INTENT_ON) ? 1 : 0;
		break;
	default:
		pr_info("[FACTORY] %s: Not support:%d \n", __func__, cmd_type);
		return size;
	}

	pr_info("[FACTORY] %s: lcd:%d,%d\n", __func__, msg_buf[0], msg_buf[1]);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);

	return size;
}

static void print_sensor_dump(struct adsp_data *data, int sensor)
{
	int i = 0;

	switch (sensor) {
	case MSG_ACCEL:
#ifdef CONFIG_SUPPORT_DUAL_6AXIS
	case MSG_ACCEL_SUB:
#endif
		for (i = 0; i < 8; i++) {
			pr_info("[FACTORY] %s - %d: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
				__func__, sensor,
				data->msg_buf[sensor][i * 16 + 0],
				data->msg_buf[sensor][i * 16 + 1],
				data->msg_buf[sensor][i * 16 + 2],
				data->msg_buf[sensor][i * 16 + 3],
				data->msg_buf[sensor][i * 16 + 4],
				data->msg_buf[sensor][i * 16 + 5],
				data->msg_buf[sensor][i * 16 + 6],
				data->msg_buf[sensor][i * 16 + 7],
				data->msg_buf[sensor][i * 16 + 8],
				data->msg_buf[sensor][i * 16 + 9],
				data->msg_buf[sensor][i * 16 + 10],
				data->msg_buf[sensor][i * 16 + 11],
				data->msg_buf[sensor][i * 16 + 12],
				data->msg_buf[sensor][i * 16 + 13],
				data->msg_buf[sensor][i * 16 + 14],
				data->msg_buf[sensor][i * 16 + 15]);
		}
		break;
	case MSG_MAG:
		pr_info("[FACTORY] %s - %d: [00h-03h] %02x,%02x,%02x,%02x [10h-16h,18h] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x [30h-32h] %02x,%02x,%02x\n",
			__func__, sensor,
			data->msg_buf[sensor][0], data->msg_buf[sensor][1],
			data->msg_buf[sensor][2], data->msg_buf[sensor][3],
			data->msg_buf[sensor][4], data->msg_buf[sensor][5],
			data->msg_buf[sensor][6], data->msg_buf[sensor][7],
			data->msg_buf[sensor][8], data->msg_buf[sensor][9],
			data->msg_buf[sensor][10], data->msg_buf[sensor][11],
			data->msg_buf[sensor][12], data->msg_buf[sensor][13],
			data->msg_buf[sensor][14]);
		break;
	case MSG_PRESSURE:
		for (i = 0; i < 7; i++) {
			pr_info("[FACTORY] %s - %d: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
				__func__, sensor,
				data->msg_buf[sensor][i * 16 + 0],
				data->msg_buf[sensor][i * 16 + 1],
				data->msg_buf[sensor][i * 16 + 2],
				data->msg_buf[sensor][i * 16 + 3],
				data->msg_buf[sensor][i * 16 + 4],
				data->msg_buf[sensor][i * 16 + 5],
				data->msg_buf[sensor][i * 16 + 6],
				data->msg_buf[sensor][i * 16 + 7],
				data->msg_buf[sensor][i * 16 + 8],
				data->msg_buf[sensor][i * 16 + 9],
				data->msg_buf[sensor][i * 16 + 10],
				data->msg_buf[sensor][i * 16 + 11],
				data->msg_buf[sensor][i * 16 + 12],
				data->msg_buf[sensor][i * 16 + 13],
				data->msg_buf[sensor][i * 16 + 14],
				data->msg_buf[sensor][i * 16 + 15]);
		}
		pr_info("[FACTORY] %s - %d: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
			__func__, sensor,
			data->msg_buf[sensor][i * 16 + 0],
			data->msg_buf[sensor][i * 16 + 1],
			data->msg_buf[sensor][i * 16 + 2],
			data->msg_buf[sensor][i * 16 + 3],
			data->msg_buf[sensor][i * 16 + 4],
			data->msg_buf[sensor][i * 16 + 5],
			data->msg_buf[sensor][i * 16 + 6],
			data->msg_buf[sensor][i * 16 + 7]);
		break;
	case MSG_LIGHT:
		pr_info("[FACTORY] %s - %d: %d,%d,%d,%d,%d,%d,%d [P]:%d,%d,%d [L]:%d,%d,%d [E]:%d,%d,%d\n",
			__func__, sensor,
			data->msg_buf[sensor][0], data->msg_buf[sensor][1],
			data->msg_buf[sensor][2], data->msg_buf[sensor][3],
			data->msg_buf[sensor][4], data->msg_buf[sensor][5],
			data->msg_buf[sensor][6], data->msg_buf[sensor][7],
			data->msg_buf[sensor][8], data->msg_buf[sensor][9],
			data->msg_buf[sensor][10], data->msg_buf[sensor][11],
			data->msg_buf[sensor][12], data->msg_buf[sensor][13],
			data->msg_buf[sensor][14], data->msg_buf[sensor][15]);
#if defined (CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION) && (CONFIG_SUPPORT_AMS_PROX_CALIBRATION)
		pr_info("[FACTORY] %s - %d: [L] R:%d, %d, %d, W:%d, P:%d\n",
			__func__, sensor, data->light_cal_result,
			data->light_cal1, data->light_cal2,
			data->copr_w, data->prox_cal);
#endif
		break;
	default:
		break;
	}
}

static void print_ssr_history(void)
{
	int i;

	if (ssr_idx == NO_SSR) {
		pr_info("[FACTORY] No SSR history\n");
		return;
	}

	pr_info("[FACTORY] Print SSR history\n");
	for (i = 0; i < HISTORY_CNT; i++) {
		if (ssr_history[i][0] == '\0')
			break;

		pr_info("[FACTORY] %s\n", ssr_history[i]);
	}
}

static BLOCKING_NOTIFIER_HEAD(sensordump_notifier_list);
int sensordump_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&sensordump_notifier_list, nb);
}
EXPORT_SYMBOL(sensordump_notifier_register);

int sensordump_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&sensordump_notifier_list, nb);
}
EXPORT_SYMBOL(sensordump_notifier_unregister);

int sensordump_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&sensordump_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(sensordump_notifier_call_chain);

void sensor_dump_work_func(struct work_struct *work)
{
	struct sdump_data *sensor_dump_data = container_of((struct work_struct *)work,
		struct sdump_data, work_sdump);
	struct adsp_data *data = sensor_dump_data->dev_data;
#ifdef CONFIG_SUPPORT_DUAL_6AXIS
	int sensor_type[SENSOR_DUMP_CNT] = { MSG_ACCEL, MSG_MAG, MSG_PRESSURE, MSG_ACCEL_SUB };
#else
	int sensor_type[SENSOR_DUMP_CNT] = { MSG_ACCEL, MSG_MAG, MSG_PRESSURE, MSG_LIGHT };
#endif
#ifdef CONFIG_SUPPORT_SSC_SPU
	int32_t idx = (fw_idx == SSC_ORI_AF_SPU_FAIL) ? SSC_ORI:fw_idx;
#endif
	int32_t msg_buf[2] = {OPTION_TYPE_SSC_DUMP_TYPE, 0};
	int i, cnt = 0;

	sensordump_notifier_call_chain(1, NULL);

#ifdef CONFIG_SUPPORT_AK0997X
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < 3)
		msleep(30);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);

	if (cnt >= 3) {
		pr_err("[FACTORY] %s: Read D/Hall Auto Cal Table Timeout!!!\n", __func__);
	}

	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0]);
#endif

	for (i = 0; i < SENSOR_DUMP_CNT; i++) {
		if (!data->sysfs_created[sensor_type[i]]) {
			pr_info("[FACTORY] %s: %d was not probed\n",
				__func__, sensor_type[i]);
			continue;
		}
		pr_info("[FACTORY] %s: %d\n", __func__, sensor_type[i]);
		cnt = 0;
		adsp_unicast(NULL, 0, sensor_type[i], 0, MSG_TYPE_GET_DHR_INFO);
		while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << sensor_type[i]) &&
			cnt++ < TIMEOUT_DHR_CNT)
			msleep(20);

		data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << sensor_type[i]);

		if (cnt >= TIMEOUT_DHR_CNT) {
			pr_err("[FACTORY] %s: %d Timeout!!!\n",
				__func__, sensor_type[i]);
		} else {
			print_sensor_dump(data, sensor_type[i]);
			msleep(200);
		}
	}

#if defined(CONFIG_SEC_WINNERLTE_PROJECT) || defined(CONFIG_SEC_WINNERX_PROJECT)
	cnt = 0;
	adsp_unicast(NULL, 0, get_prox_sidx(data), 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << get_prox_sidx(data)) &&
		cnt++ < TIMEOUT_DHR_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << get_prox_sidx(data));

	if (cnt >= TIMEOUT_DHR_CNT)
		pr_err("[FACTORY] %s: prox(%d) dhr_sensor_info Timeout!!!\n",
			__func__, get_prox_sidx(data));
#endif
#ifdef CONFIG_SUPPORT_SSC_SPU
	pr_info("SLPI firmware ver, idx:%d, CL:%d, %d-%d-%d, %d:%d:%d.%d\n", 
		fw_idx, ver_buf[idx][SSC_CL], ver_buf[idx][SSC_YEAR],
		ver_buf[idx][SSC_MONTH], ver_buf[idx][SSC_DATE],
		ver_buf[idx][SSC_HOUR], ver_buf[idx][SSC_MIN],
		ver_buf[idx][SSC_SEC], ver_buf[idx][SSC_MSEC]);
#endif
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);

	adsp_unicast(NULL, 0, MSG_REG_SNS, 0, MSG_TYPE_OPTION_DEFINE);

#ifdef CONFIG_SUPPORT_SSC_SPU
	pr_info("SLPI firmware ver, idx:%d, CL:%d, %d-%d-%d, %d:%d:%d.%d\n", 
		fw_idx, ver_buf[idx][SSC_CL], ver_buf[idx][SSC_YEAR],
		ver_buf[idx][SSC_MONTH], ver_buf[idx][SSC_DATE],
		ver_buf[idx][SSC_HOUR], ver_buf[idx][SSC_MIN],
		ver_buf[idx][SSC_SEC], ver_buf[idx][SSC_MSEC]);
#endif

	print_ssr_history();
}

static ssize_t sensor_dump_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);

	pdata_sdump->dev_data = data;
	queue_work(pdata_sdump->sdump_wq, &pdata_sdump->work_sdump);

	return snprintf(buf, PAGE_SIZE, "%s\n", SENSOR_DUMP_DONE);
}

#ifdef CONFIG_SUPPORT_SSC_SPU
static ssize_t ssc_firmware_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int32_t idx = (fw_idx == SSC_ORI_AF_SPU_FAIL) ? SSC_ORI:fw_idx;

	pr_info("idx:%d, CL:%d, %d-%d-%d, %d:%d:%d.%d\n", 
		fw_idx, ver_buf[idx][SSC_CL], ver_buf[idx][SSC_YEAR],
		ver_buf[idx][SSC_MONTH], ver_buf[idx][SSC_DATE],
		ver_buf[idx][SSC_HOUR], ver_buf[idx][SSC_MIN],
		ver_buf[idx][SSC_SEC], ver_buf[idx][SSC_MSEC]);

	return snprintf(buf, PAGE_SIZE, "idx:%d, CL:%d, %d-%d-%d, %d:%d:%d.%d\n",
		fw_idx, ver_buf[idx][SSC_CL], ver_buf[idx][SSC_YEAR],
		ver_buf[idx][SSC_MONTH], ver_buf[idx][SSC_DATE],
		ver_buf[idx][SSC_HOUR], ver_buf[idx][SSC_MIN],
		ver_buf[idx][SSC_SEC], ver_buf[idx][SSC_MSEC]);
}
#endif
#ifdef CONFIG_SUPPORT_SSC_MODE
static int ssc_mode;
static ssize_t ssc_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] ssc_mode:%d\n", ssc_mode);

	return snprintf(buf, PAGE_SIZE, "%d\n", ssc_mode);
}

static ssize_t ssc_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtoint(buf, 10, &ssc_mode)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] ssc_mode %d\n", ssc_mode);

	return size;
}
#endif

static DEVICE_ATTR(dumpstate, 0440, dumpstate_show, NULL);
static DEVICE_ATTR(operation_mode, 0664,
	operation_mode_show, operation_mode_store);
static DEVICE_ATTR(mode, 0660, mode_show, mode_store);
static DEVICE_ATTR(ssc_pid, 0660, pid_show, pid_store);
static DEVICE_ATTR(remove_sysfs, 0220, NULL, remove_sensor_sysfs_store);
static DEVICE_ATTR(ssc_hw_rev, 0664, ssc_hw_rev_show, ssc_hw_rev_store);
static DEVICE_ATTR(ssr_msg, 0440, ssr_msg_show, NULL);
static DEVICE_ATTR(ssr_reset, 0440, ssr_reset_show, NULL);
static DEVICE_ATTR(support_algo, 0220, NULL, support_algo_store);
#ifdef CONFIG_SUPPORT_VIRTUAL_OPTIC
static DEVICE_ATTR(fac_fstate, 0220, NULL, fac_fstate_store);
#endif
#if defined(CONFIG_SUPPORT_DEVICE_MODE) && defined(CONFIG_SUPPORT_DUAL_OPTIC)
static DEVICE_ATTR(update_ssc_flip, 0220, NULL, update_ssc_flip_store);
#endif
static DEVICE_ATTR(support_dual_sensor, 0440, support_dual_sensor_show, NULL);
static DEVICE_ATTR(algo_lcd_onoff, 0220, NULL, algo_lcd_onoff_store);
static DEVICE_ATTR(sensor_dump, 0444, sensor_dump_show, NULL);
#ifdef CONFIG_SUPPORT_SSC_SPU
static DEVICE_ATTR(ssc_firmware_info, 0440, ssc_firmware_info_show, NULL);
#endif
#ifdef CONFIG_SUPPORT_SSC_MODE
static DEVICE_ATTR(ssc_mode, 0664, ssc_mode_show, ssc_mode_store);
#endif
#if defined(CONFIG_SUPPORT_BHL_COMPENSATION_FOR_LIGHT_SENSOR) || \
	defined(CONFIG_SUPPORT_BRIGHT_SYSFS_COMPENSATION_LUX) || \
	defined(CONFIG_SUPPORT_AK0997X)
static DEVICE_ATTR(lcd_onoff, 0220, NULL, lcd_onoff_store);
#endif
static struct device_attribute *core_attrs[] = {
	&dev_attr_dumpstate,
	&dev_attr_operation_mode,
	&dev_attr_mode,
	&dev_attr_ssc_pid,
	&dev_attr_remove_sysfs,
	&dev_attr_ssc_hw_rev,
	&dev_attr_ssr_msg,
	&dev_attr_ssr_reset,
	&dev_attr_support_algo,
#ifdef CONFIG_SUPPORT_VIRTUAL_OPTIC	
	&dev_attr_fac_fstate,
#endif
#if defined(CONFIG_SUPPORT_DEVICE_MODE) && defined(CONFIG_SUPPORT_DUAL_OPTIC)
	&dev_attr_update_ssc_flip,
#endif
	&dev_attr_support_dual_sensor,
	&dev_attr_algo_lcd_onoff,
	&dev_attr_sensor_dump,
#ifdef CONFIG_SUPPORT_SSC_SPU
	&dev_attr_ssc_firmware_info,
#endif
#ifdef CONFIG_SUPPORT_SSC_MODE
	&dev_attr_ssc_mode,
#endif
#if defined(CONFIG_SUPPORT_BHL_COMPENSATION_FOR_LIGHT_SENSOR) || \
	defined(CONFIG_SUPPORT_BRIGHT_SYSFS_COMPENSATION_LUX) || \
	defined(CONFIG_SUPPORT_AK0997X)
	&dev_attr_lcd_onoff,
#endif
	NULL,
};

#ifdef CONFIG_SUPPORT_SSC_MODE
static int ssc_core_probe(struct platform_device *pdev)
{
#ifdef CONFIG_SUPPORT_SSC_MODE_FOR_MAG
	struct device *dev = &pdev->dev;

#ifdef CONFIG_OF
	if (dev->of_node) {
		struct device_node *np = dev->of_node;
		int check_mst_gpio, check_nfc_gpio;
		int value_mst = 0, value_nfc = 0;

		check_mst_gpio = of_get_named_gpio_flags(np, "ssc_core,mst_gpio", 0, NULL);
		if(check_mst_gpio >= 0)
			value_mst = gpio_get_value(check_mst_gpio);

		if (value_mst == 1) {
			ssc_mode = ANT_NFC_MST;
			return 0;
		}

		check_nfc_gpio = of_get_named_gpio_flags(np, "ssc_core,nfc_gpio", 0, NULL);
		if(check_nfc_gpio >= 0)
			value_nfc = gpio_get_value(check_nfc_gpio);

		if (value_nfc == 1) {
			ssc_mode = ANT_NFC_ONLY;
			return 0;
		}
	}
#endif
	ssc_mode = ANT_DUMMY;
#endif
	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id ssc_core_dt_ids[] = {
	{ .compatible = "ssc_core" },
	{ },
};
MODULE_DEVICE_TABLE(of, ssc_core_dt_ids);
#endif /* CONFIG_OF */

static struct platform_driver ssc_core_driver = {
	.probe		= ssc_core_probe,
	.driver		= {
		.name	= "ssc_core",
		.owner	= THIS_MODULE,
#if defined(CONFIG_OF)
		.of_match_table	= ssc_core_dt_ids,
#endif /* CONFIG_OF */
	}
};
#endif

static int __init core_factory_init(void)
{
#if defined(CONFIG_SUPPORT_DEVICE_MODE) || defined(CONFIG_VBUS_NOTIFIER)
	struct adsp_data *data = adsp_ssc_core_register(MSG_SSC_CORE, core_attrs);
	pr_info("[FACTORY] %s\n", __func__);
#if defined(CONFIG_SUPPORT_DEVICE_MODE) && defined(CONFIG_FOLDER_HALL)
	data->adsp_nb.notifier_call = sns_device_mode_notify,
	data->adsp_nb.priority = 1,
        hall_ic_register_notify(&data->adsp_nb);
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	vbus_notifier_register(&data->vbus_nb,
		ssc_core_vbus_notifier, VBUS_NOTIFY_DEV_CHARGER);
#endif
#else
	adsp_factory_register(MSG_SSC_CORE, core_attrs);
#endif
#if defined(CONFIG_SUPPORT_DEVICE_MODE) && defined(CONFIG_SUPPORT_DUAL_OPTIC)
	pdata_ssc_flip = kzalloc(sizeof(*pdata_ssc_flip), GFP_KERNEL);
	if (pdata_ssc_flip == NULL)
		return -ENOMEM;

	pdata_ssc_flip->ssc_flip_wq =
		create_singlethread_workqueue("ssc_flip_wq");
	if (pdata_ssc_flip->ssc_flip_wq == NULL) {
		pr_err("[FACTORY]: %s - couldn't create ssc charge wq\n", __func__);
		kfree(pdata_ssc_flip);
		return -ENOMEM;
	}
	INIT_WORK(&pdata_ssc_flip->work_ssc_flip, ssc_flip_work_func);
#endif

#ifdef CONFIG_SUPPORT_SSC_MODE
	platform_driver_register(&ssc_core_driver);
#endif

	pdata_sdump = kzalloc(sizeof(*pdata_sdump), GFP_KERNEL);
	if (pdata_sdump == NULL)
		return -ENOMEM;

	pdata_sdump->sdump_wq = create_singlethread_workqueue("sdump_wq");
	if (pdata_sdump->sdump_wq == NULL) {
		pr_err("[FACTORY]: %s - could not create sdump wq\n", __func__);
		kfree(pdata_sdump);
		return -ENOMEM;
	}
	INIT_WORK(&pdata_sdump->work_sdump, sensor_dump_work_func);
	pr_info("[FACTORY] %s\n", __func__);

#ifdef CONFIG_SUPPORT_AK0997X
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	init_digital_hall_data();
#endif

#ifdef CONFIG_VBUS_NOTIFIER
	pdata_ssc_charge = kzalloc(sizeof(*pdata_ssc_charge), GFP_KERNEL);
	if (pdata_ssc_charge == NULL)
		return -ENOMEM;

	pdata_ssc_charge->ssc_charge_wq =
		create_singlethread_workqueue("ssc_charge_wq");
	if (pdata_ssc_charge->ssc_charge_wq == NULL) {
		pr_err("[FACTORY]: %s - couldn't create ssc charge wq\n", __func__);
		kfree(pdata_ssc_charge);
		return -ENOMEM;
	}
	INIT_WORK(&pdata_ssc_charge->work_ssc_charge, ssc_charge_work_func);
	pdata_ssc_charge->is_charging = false;
#endif	
	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

static void __exit core_factory_exit(void)
{
#if defined(CONFIG_SUPPORT_DEVICE_MODE) || defined(CONFIG_VBUS_NOTIFIER)
	struct adsp_data *data = adsp_ssc_core_unregister(MSG_SSC_CORE);;
#if defined(CONFIG_SUPPORT_DEVICE_MODE) && defined(CONFIG_FOLDER_HALL)
	hall_ic_unregister_notify(&data->adsp_nb);
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	vbus_notifier_unregister(&data->vbus_nb);
#endif
#else
	adsp_factory_unregister(MSG_SSC_CORE);
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	if (pdata_ssc_charge != NULL && pdata_ssc_charge->ssc_charge_wq != NULL) {
		cancel_work_sync(&pdata_ssc_charge->work_ssc_charge);
		destroy_workqueue(pdata_ssc_charge->ssc_charge_wq);
		pdata_ssc_charge->ssc_charge_wq = NULL;
	}
#endif

#if defined(CONFIG_SUPPORT_DEVICE_MODE) && defined(CONFIG_SUPPORT_DUAL_OPTIC)
	if (pdata_ssc_flip != NULL && pdata_ssc_flip->ssc_flip_wq != NULL) {
		cancel_work_sync(&pdata_ssc_flip->work_ssc_flip);
		destroy_workqueue(pdata_ssc_flip->ssc_flip_wq);
		pdata_ssc_flip->ssc_flip_wq = NULL;
	}
#endif

#ifdef CONFIG_SUPPORT_SSC_MODE
	platform_driver_unregister(&ssc_core_driver);
#endif
	if (pdata_sdump->sdump_wq)
		destroy_workqueue(pdata_sdump->sdump_wq);
	if (pdata_sdump)
		kfree(pdata_sdump);

	pr_info("[FACTORY] %s\n", __func__);
}
module_init(core_factory_init);
module_exit(core_factory_exit);
