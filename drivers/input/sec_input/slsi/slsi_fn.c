/* drivers/input/sec_input/slsi/slsi_fn.c
 *
 * Copyright (C) 2011 Samsung Electronics Co., Ltd.
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "slsi_dev.h"
#include "slsi_reg.h"

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
irqreturn_t secure_filter_interrupt(struct sec_ts_data *ts)
{
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		if (atomic_cmpxchg(&ts->secure_pending_irqs, 0, 1) == 0) {
			sysfs_notify(&ts->input_dev->dev.kobj, NULL, "secure_touch");

#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
			complete(&ts->st_irq_received);
#endif
		} else {
			input_info(true, &ts->client->dev, "%s: pending irq:%d\n",
					__func__, (int)atomic_read(&ts->secure_pending_irqs));
		}

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

/**
 * Sysfs attr group for secure touch & interrupt handler for Secure world.
 * @atomic : syncronization for secure_enabled
 * @pm_runtime : set rpm_resume or rpm_ilde
 */
ssize_t secure_touch_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d", atomic_read(&ts->secure_enabled));
}

ssize_t secure_touch_enable_store(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	int ret;
	unsigned long data;

	if (count > 2) {
		input_err(true, &ts->client->dev,
				"%s: cmd length is over (%s,%d)!!\n",
				__func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	ret = kstrtoul(buf, 10, &data);
	if (ret != 0) {
		input_err(true, &ts->client->dev, "%s: failed to read:%d\n",
				__func__, ret);
		return -EINVAL;
	}

	if (data == 1) {
		if (ts->reset_is_on_going) {
			input_err(true, &ts->client->dev, "%s: reset is on going because i2c fail\n", __func__);
			return -EBUSY;
		}

		/* Enable Secure World */
		if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
			input_err(true, &ts->client->dev, "%s: already enabled\n", __func__);
			return -EBUSY;
		}

		sec_ts_delay(200);
		
		/* syncronize_irq -> disable_irq + enable_irq
		 * concern about timing issue.
		 */
		disable_irq(ts->client->irq);

		/* Fix normal active mode : idle mode is failed to i2c for 1 time */
		ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
		if (ret < 0) {
			enable_irq(ts->client->irq);
			input_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
					__func__);
			return -EIO;
		}

		/* Release All Finger */
		sec_ts_unlocked_release_all_finger(ts);

		if (pm_runtime_get_sync(ts->client->adapter->dev.parent) < 0) {
			enable_irq(ts->client->irq);
			input_err(true, &ts->client->dev, "%s: failed to get pm_runtime\n", __func__);
			return -EIO;
		}
		reinit_completion(&ts->secure_powerdown);
		reinit_completion(&ts->secure_interrupt);
#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
		reinit_completion(&ts->st_irq_received);
#endif
		atomic_set(&ts->secure_enabled, 1);
		atomic_set(&ts->secure_pending_irqs, 0);

		enable_irq(ts->client->irq);

		input_info(true, &ts->client->dev, "%s: secure touch enable\n", __func__);
	} else if (data == 0) {
		/* Disable Secure World */
		if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_DISABLE) {
			input_err(true, &ts->client->dev, "%s: already disabled\n", __func__);
			return count;
		}

		sec_ts_delay(200);

		pm_runtime_put_sync(ts->client->adapter->dev.parent);
		atomic_set(&ts->secure_enabled, 0);

		sysfs_notify(&ts->input_dev->dev.kobj, NULL, "secure_touch");

		sec_ts_delay(10);

		sec_ts_irq_thread(ts->client->irq, ts);
		complete(&ts->secure_interrupt);
		complete(&ts->secure_powerdown);
#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
		complete(&ts->st_irq_received);
#endif

		input_info(true, &ts->client->dev, "%s: secure touch disable\n", __func__);

		ret = sec_ts_release_tmode(ts);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to release tmode\n",
					__func__);
			return -EIO;
		}

	} else {
		input_err(true, &ts->client->dev, "%s: unsupport value:%d\n", __func__, data);
		return -EINVAL;
	}

	return count;
}

#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
int secure_get_irq(struct device *dev)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_DISABLE) {
		input_err(true, &ts->client->dev, "%s: disabled\n", __func__);
		return -EBADF;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, -1, 0) == -1) {
		input_err(true, &ts->client->dev, "%s: pending irq -1\n", __func__);
		return -EINVAL;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, 1, 0) == 1)
		val = 1;

	input_err(true, &ts->client->dev, "%s: pending irq is %d\n",
			__func__, atomic_read(&ts->secure_pending_irqs));

	complete(&ts->secure_interrupt);

	return val;
}
#endif

ssize_t secure_touch_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_DISABLE) {
		input_err(true, &ts->client->dev, "%s: disabled\n", __func__);
		return -EBADF;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, -1, 0) == -1) {
		input_err(true, &ts->client->dev, "%s: pending irq -1\n", __func__);
		return -EINVAL;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, 1, 0) == 1) {
		val = 1;
		input_err(true, &ts->client->dev, "%s: pending irq is %d\n",
				__func__, atomic_read(&ts->secure_pending_irqs));
	}

	complete(&ts->secure_interrupt);

	return snprintf(buf, PAGE_SIZE, "%u", val);
}

ssize_t secure_ownership_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "1");
}

int secure_touch_init(struct sec_ts_data *ts)
{
	input_info(true, &ts->client->dev, "%s\n", __func__);

	init_completion(&ts->secure_interrupt);
	init_completion(&ts->secure_powerdown);
#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
	init_completion(&ts->st_irq_received);
	register_tui_hal_ts(&ts->input_dev->dev, &ts->secure_enabled,
			&ts->st_irq_received, secure_get_irq,
			secure_touch_enable_store);
#endif

	return 0;
}

void secure_touch_stop(struct sec_ts_data *ts, bool stop)
{
	if (atomic_read(&ts->secure_enabled)) {
		atomic_set(&ts->secure_pending_irqs, -1);

		sysfs_notify(&ts->input_dev->dev.kobj, NULL, "secure_touch");

#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
		complete(&ts->st_irq_received);
#endif

		if (stop)
			wait_for_completion_interruptible(&ts->secure_powerdown);

		input_info(true, &ts->client->dev, "%s: %d\n", __func__, stop);
	}
}

static DEVICE_ATTR(secure_touch_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
		secure_touch_enable_show, secure_touch_enable_store);
static DEVICE_ATTR(secure_touch, S_IRUGO, secure_touch_show, NULL);
static DEVICE_ATTR(secure_ownership, S_IRUGO, secure_ownership_show, NULL);
static struct attribute *secure_attr[] = {
	&dev_attr_secure_touch_enable.attr,
	&dev_attr_secure_touch.attr,
	&dev_attr_secure_ownership.attr,
	NULL,
};

static struct attribute_group secure_attr_group = {
	.attrs = secure_attr,
};
#endif

int sec_ts_read_from_sponge(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;

	mutex_lock(&ts->sponge_mutex);
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_READ_PARAM, data, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to read sponge command\n", __func__);

	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SPONGE_READ_PARAM, (u8 *)data, len);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to read sponge command\n", __func__);
	mutex_unlock(&ts->sponge_mutex);

	return ret;
}

int sec_ts_write_to_sponge(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;

	mutex_lock(&ts->sponge_mutex);
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_WRITE_PARAM, data, len);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write offset\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_NOTIFY_PACKET, NULL, 0);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to send notify\n", __func__);
	mutex_unlock(&ts->sponge_mutex);

	return ret;
}

void sec_ts_set_utc_sponge(struct sec_ts_data *ts)
{
	struct timeval current_time;
	int ret;
	u8 data[6] = {SEC_TS_CMD_SPONGE_OFFSET_UTC, 0};

	do_gettimeofday(&current_time);
	data[2] = (0xFF & (u8)((current_time.tv_sec) >> 0));
	data[3] = (0xFF & (u8)((current_time.tv_sec) >> 8));
	data[4] = (0xFF & (u8)((current_time.tv_sec) >> 16));
	data[5] = (0xFF & (u8)((current_time.tv_sec) >> 24));
	input_info(true, &ts->client->dev, "Write UTC to Sponge = %X\n", (int)(current_time.tv_sec));

	ret = ts->sec_ts_write_sponge(ts, data, 6);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write sponge\n", __func__);
}

void sec_ts_set_prox_power_off(struct sec_ts_data *ts, u8 data)
{
	int ret;

	input_info(true, &ts->client->dev, "Write prox set = %d\n", data);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_PROX_POWER_OFF, &data, 1);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write prox_power_off set\n", __func__);
}

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
void sec_ts_check_rawdata(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data, ghost_check.work);

	if (ts->tsp_dump_lock == 1) {
		input_err(true, &ts->client->dev, "%s: ignored ## already checking..\n", __func__);
		return;
	}
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: ignored ## IC is power off\n", __func__);
		return;
	}

	sec_ts_run_rawdata_all(ts, true);
}

void dump_tsp_log(void)
{
	pr_info("%s: %s %s: start\n", SEC_TS_I2C_NAME, SECLOG, __func__);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		pr_err("%s: %s %s: ignored ## lpm charging Mode!!\n", SEC_TS_I2C_NAME, SECLOG, __func__);
		return;
	}
#endif
	if (p_ghost_check == NULL) {
		pr_err("%s: %s %s: ignored ## tsp probe fail!!\n", SEC_TS_I2C_NAME, SECLOG, __func__);
		return;
	}

	if (!shutdown_is_on_going_tsp)
		schedule_delayed_work(p_ghost_check, msecs_to_jiffies(100));
}
#endif


void sec_ts_delay(unsigned int ms)
{
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000);
	else
		msleep(ms);
}

int sec_ts_set_touch_function(struct sec_ts_data *ts)
{
	int ret = 0;

	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&(ts->touch_functions), 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
				__func__, SEC_TS_CMD_SET_TOUCHFUNCTION);

	if (!shutdown_is_on_going_tsp)
		schedule_delayed_work(&ts->work_read_functions, msecs_to_jiffies(30));

	return ret;
}

void sec_ts_get_touch_function(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
			work_read_functions.work);
	int ret = 0;

	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&(ts->ic_status), 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read touch functions(%d)\n",
				__func__, ret);
		return;
	}

	input_info(true, &ts->client->dev,
			"%s: touch_functions:%x ic_status:%x\n", __func__,
			ts->touch_functions, ts->ic_status);
}

int sec_ts_wait_for_ready(struct sec_ts_data *ts, unsigned int ack)
{
	int rc = -1;
	int retry = 0;
	u8 tBuff[SEC_TS_EVENT_BUFF_SIZE] = {0,};

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		if (gpio_get_value(ts->plat_data->irq_gpio) == 0) {
			if (sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, tBuff, SEC_TS_EVENT_BUFF_SIZE) > 0) {
				if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_INFO) {
					if (tBuff[1] == ack) {
						rc = 0;
						break;
					}
				} else if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_VENDOR_INFO) {
					if (tBuff[1] == ack) {
						rc = 0;
						break;
					}
				}
			}
		}
		sec_ts_delay(20);
		retry++;
	}

	if (retry > SEC_TS_WAIT_RETRY_CNT)
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);

	input_info(true, &ts->client->dev,
			"%s: %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X [%d]\n",
			__func__, tBuff[0], tBuff[1], tBuff[2], tBuff[3],
			tBuff[4], tBuff[5], tBuff[6], tBuff[7], retry);

	return rc;
}

int sec_ts_read_calibration_report(struct sec_ts_data *ts)
{
	int ret;
	u8 buf[5] = { 0 };

	buf[0] = SEC_TS_READ_CALIBRATION_REPORT;

	ret = sec_ts_i2c_read(ts, buf[0], &buf[1], 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read, %d\n", __func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev, "%s: count:%d, pass count:%d, fail count:%d, status:0x%X\n",
			__func__, buf[1], buf[2], buf[3], buf[4]);

	return buf[4];
}

void sec_ts_print_info(struct sec_ts_data *ts)
{
	if (!ts)
		return;

	if (!ts->client)
		return;

	ts->print_info_cnt_open++;

	if (ts->print_info_cnt_open > 0xfff0)
		ts->print_info_cnt_open = 0;

	if (ts->touch_count == 0)
		ts->print_info_cnt_release++;

	input_info(true, &ts->client->dev,
			"mode:%04X tc:%d noise:%d%d ext_n:%d wet:%d wc:%x(%d) lp:(%x) fod:%d D%05X fn:%04X/%04X ED:%d // v:%02X%02X cal:%02X(%02X) C%02XT%04X.%4s%s Cal_flag:%s fail_cnt:%d // sip:%d id(%d,%d) tmp(%d)// #%d %d\n",
			ts->print_info_currnet_mode, ts->touch_count,
			ts->touch_noise_status, ts->touch_pre_noise_status,
			ts->external_noise_mode, ts->wet_mode,
			ts->charger_mode, ts->force_charger_mode,
			ts->lowpower_mode, ts->fod_set_val, ts->defect_probability,
			ts->touch_functions, ts->ic_status, ts->ed_enable,
			ts->plat_data->img_version_of_ic[2], ts->plat_data->img_version_of_ic[3],
			ts->cal_status, ts->nv,
#ifdef TCLM_CONCEPT
			ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver,
			ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].f_name,
			(ts->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ",
			(ts->tdata->nvdata.cal_fail_falg == SEC_CAL_PASS) ? "Success" : "Fail",
			ts->tdata->nvdata.cal_fail_cnt,
#else
			0,0," "," "," ",0,
#endif
			ts->sip_mode, ts->tspid_val, ts->tspicid_val,
			ts->tsp_temp_data,
			ts->print_info_cnt_open, ts->print_info_cnt_release);
}

/************************************************************
*  720  * 1480 : <48 96 60> indicator: 24dp navigator:48dp edge:60px dpi=320
* 1080  * 2220 :  4096 * 4096 : <133 266 341>  (approximately value)
************************************************************/
void location_detect(struct sec_ts_data *ts, char *loc, int x, int y)
{
	int i;

	for (i = 0; i < SEC_TS_LOCATION_DETECT_SIZE; ++i)
		loc[i] = 0;

	if (x < ts->plat_data->area_edge)
		strcat(loc, "E.");
	else if (x < (ts->plat_data->max_x - ts->plat_data->area_edge))
		strcat(loc, "C.");
	else
		strcat(loc, "e.");

	if (y < ts->plat_data->area_indicator)
		strcat(loc, "S");
	else if (y < (ts->plat_data->max_y - ts->plat_data->area_navigation))
		strcat(loc, "C");
	else
		strcat(loc, "N");
}

int sec_ts_set_cover_type(struct sec_ts_data *ts, bool enable)
{
	int ret;

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, ts->cover_type);

	switch (ts->cover_type) {
	case SEC_TS_VIEW_WIRELESS:
	case SEC_TS_VIEW_COVER:
	case SEC_TS_VIEW_WALLET:
	case SEC_TS_FLIP_WALLET:
	case SEC_TS_LED_COVER:
	case SEC_TS_MONTBLANC_COVER:
	case SEC_TS_CLEAR_FLIP_COVER:
	case SEC_TS_QWERTY_KEYBOARD_KOR:
	case SEC_TS_QWERTY_KEYBOARD_US:
	case SEC_TS_CLEAR_SIDE_VIEW_COVER:
		ts->cover_cmd = (u8)ts->cover_type;
		break;
	case SEC_TS_CHARGER_COVER:
	case SEC_TS_COVER_NOTHING1:
	case SEC_TS_COVER_NOTHING2:
	default:
		ts->cover_cmd = 0;
		input_err(true, &ts->client->dev, "%s: not chage touch state, %d\n",
				__func__, ts->cover_type);
		break;
	}

	if (enable)
		ts->touch_functions = (ts->touch_functions | SEC_TS_BIT_SETFUNC_COVER | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);
	else
		ts->touch_functions = ((ts->touch_functions & (~SEC_TS_BIT_SETFUNC_COVER)) | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: pwr off, close:%d, status:%x\n", __func__,
				enable, ts->touch_functions);
		goto cover_enable_err;
	}

	if (enable) {
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, &ts->cover_cmd, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send covertype command: %d", __func__, ts->cover_cmd);
			goto cover_enable_err;
		}
	}

	ret = sec_ts_set_touch_function(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Failed to send command", __func__);
		goto cover_enable_err;
	}

	input_info(true, &ts->client->dev, "%s: close:%d, status:%x\n", __func__,
			enable, ts->touch_functions);

	return 0;

cover_enable_err:
	return -EIO;
}
EXPORT_SYMBOL(sec_ts_set_cover_type);

void sec_ts_set_grip_type(struct sec_ts_data *ts, u8 set_type)
{
	u8 mode = G_NONE;

	input_info(true, &ts->client->dev, "%s: re-init grip(%d), edh:%d, edg:%d, lan:%d\n", __func__,
			set_type, ts->grip_edgehandler_direction, ts->grip_edge_range, ts->grip_landscape_mode);

	/* edge handler */
	if (ts->grip_edgehandler_direction != 0)
		mode |= G_SET_EDGE_HANDLER;

	if (set_type == GRIP_ALL_DATA) {
		/* edge */
		if (ts->grip_edge_range != 60)
			mode |= G_SET_EDGE_ZONE;

		/* dead zone */
		if (ts->grip_landscape_mode == 1)	/* default 0 mode, 32 */
			mode |= G_SET_LANDSCAPE_MODE;
		else
			mode |= G_SET_NORMAL_MODE;
	}

	if (mode)
		set_grip_data_to_ic(ts, mode);
}

struct sec_ts_data *g_ts;

static ssize_t sec_ts_tsp_cmoffset_all_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	struct sec_ts_data *ts;
	static ssize_t retlen = 0;
	ssize_t retlen_sdc = 0, retlen_miscal = 0, retlen_main = 0;
	ssize_t count = 0;
	loff_t pos = *offset;
#if defined(CONFIG_SEC_FACTORY)
	int ret;
#endif

	if (!g_ts) {
		pr_err("%s %s: dev is null\n", SECLOG, __func__);
		return 0;
	}
	ts = g_ts;

	if (ts->proc_cmoffset_size == 0) {
		pr_err("%s %s: proc_cmoffset_size is 0\n", SECLOG, __func__);
		return 0;
	}

	if (pos == 0) {
#if defined(CONFIG_SEC_FACTORY)
		ret = get_cmoffset_dump_all(ts, ts->cmoffset_sdc_proc, OFFSET_FW_SDC);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_cmoffset_dump SDC fail use boot time value\n", __func__);
		ret = get_cmoffset_dump_all(ts, ts->cmoffset_main_proc, OFFSET_FW_MAIN);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_cmoffset_dump MAIN fail use boot time value\n", __func__);

		ret = get_miscal_dump(ts, ts->miscal_proc);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : get_miscal fail use boot time value\n", __func__);
#endif
		retlen_sdc = strlen(ts->cmoffset_sdc_proc);
		retlen_main = strlen(ts->cmoffset_main_proc);
		retlen_miscal = strlen(ts->miscal_proc);

		ts->cmoffset_all_proc = kzalloc(ts->proc_cmoffset_all_size, GFP_KERNEL);
		if (!ts->cmoffset_all_proc){
			input_err(true, &ts->client->dev, "%s : kzalloc fail (cmoffset_all_proc)\n", __func__);
			return count;
		}

		strlcat(ts->cmoffset_all_proc, ts->cmoffset_sdc_proc, ts->proc_cmoffset_all_size);
		strlcat(ts->cmoffset_all_proc, ts->cmoffset_main_proc, ts->proc_cmoffset_all_size);
		strlcat(ts->cmoffset_all_proc, ts->miscal_proc, ts->proc_cmoffset_all_size);

		retlen = strlen(ts->cmoffset_all_proc);

		input_info(true, &ts->client->dev, "%s : retlen[%d], retlen_sdc[%d], retlen_main[%d] retlen_miscal[%d]\n",
						__func__, retlen, retlen_sdc, retlen_main, retlen_miscal);
	}

	if (pos >= retlen)
		return 0;

	count = min(len, (size_t)(retlen - pos));

	input_info(true, &ts->client->dev, "%s : total:%d pos:%d count:%d\n", __func__, retlen, pos, count);

	if (copy_to_user(buf, ts->cmoffset_all_proc + pos, count)) {
		input_err(true, &ts->client->dev, "%s : copy_to_user error!\n", __func__, retlen, pos);
		return -EFAULT;
	}

	*offset += count;

	if (count < len) {
		input_info(true, &ts->client->dev, "%s : print all & free cmoffset_all_proc [%d][%d]\n",
					__func__, retlen, offset);
		if (ts->cmoffset_all_proc)
			kfree(ts->cmoffset_all_proc);
		retlen = 0;
	}

	return count;
}

static ssize_t sec_ts_tsp_fail_hist_all_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	struct sec_ts_data *ts;

	static ssize_t retlen = 0;
	ssize_t retlen_sdc = 0, retlen_sub = 0, retlen_main = 0;
	ssize_t count = 0;
	loff_t pos = *offset;
#if defined(CONFIG_SEC_FACTORY)
	int ret;
#endif

	if (!g_ts) {
		pr_err("%s %s: dev is null\n", SECLOG, __func__);
		return 0;
	}
	ts = g_ts;

	if (ts->proc_fail_hist_size == 0) {
		pr_err("%s %s: proc_fail_hist_size is 0\n", SECLOG, __func__);
		return 0;
	}

	if (pos == 0) {
#if defined(CONFIG_SEC_FACTORY)
		ret = get_selftest_fail_hist_dump_all(ts, ts->fail_hist_sdc_proc, OFFSET_FW_SDC);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_fail_hist_dump SDC fail use boot time value\n", __func__);
		ret = get_selftest_fail_hist_dump_all(ts, ts->fail_hist_sub_proc, OFFSET_FW_SUB);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_fail_hist_dump SUB fail use boot time value\n", __func__);
		ret = get_selftest_fail_hist_dump_all(ts, ts->fail_hist_main_proc, OFFSET_FW_MAIN);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_fail_hist_dump MAIN fail use boot time value\n", __func__);
#endif
		retlen_sdc = strlen(ts->fail_hist_sdc_proc);
		retlen_sub = strlen(ts->fail_hist_sub_proc);
		retlen_main = strlen(ts->fail_hist_main_proc);

		ts->fail_hist_all_proc = kzalloc(ts->proc_fail_hist_all_size, GFP_KERNEL);
		if (!ts->fail_hist_all_proc){
			input_err(true, &ts->client->dev, "%s : kzalloc fail (fail_hist_all_proc)\n", __func__);
			return count;
		}

		strlcat(ts->fail_hist_all_proc, ts->fail_hist_sdc_proc, ts->proc_fail_hist_all_size);
		strlcat(ts->fail_hist_all_proc, ts->fail_hist_sub_proc, ts->proc_fail_hist_all_size);
		strlcat(ts->fail_hist_all_proc, ts->fail_hist_main_proc, ts->proc_fail_hist_all_size);

		retlen = strlen(ts->fail_hist_all_proc);

		input_info(true, &ts->client->dev, "%s : retlen[%d], retlen_sdc[%d], retlen_sub[%d], retlen_main[%d]\n",
						__func__, retlen, retlen_sdc, retlen_sub, retlen_main);
	}

	if (pos >= retlen)
		return 0;

	count = min(len, (size_t)(retlen - pos));

	input_info(true, &ts->client->dev, "%s : total:%d pos:%d count:%d\n", __func__, retlen, pos, count);

	if (copy_to_user(buf, ts->fail_hist_all_proc + pos, count)) {
		input_err(true, &ts->client->dev, "%s : copy_to_user error!\n", __func__, retlen, pos);
		return -EFAULT;
	}

	*offset += count;

	if (count < len) {
		input_info(true, &ts->client->dev, "%s : print all & free fail_hist_all_proc [%d][%d]\n",
					__func__, retlen, offset);
		if (ts->fail_hist_all_proc)
			kfree(ts->fail_hist_all_proc);
		retlen = 0;
	}

	return count;
}

static ssize_t sec_ts_tsp_cmoffset_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	pr_info("[sec_input] %s called offset:%d\n", __func__, (int)*offset);
	return sec_ts_tsp_cmoffset_all_read(file, buf, len, offset);
}

static ssize_t sec_ts_tsp_fail_hist_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	pr_info("[sec_input] %s called fail_hist:%d\n", __func__, (int)*offset);
	return sec_ts_tsp_fail_hist_all_read(file, buf, len, offset);
}

static const struct file_operations tsp_cmoffset_all_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_ts_tsp_cmoffset_read,
	.llseek = generic_file_llseek,
};

static const struct file_operations tsp_fail_hist_all_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_ts_tsp_fail_hist_read,
	.llseek = generic_file_llseek,
};

void sec_ts_init_proc(struct sec_ts_data *ts)
{
	struct proc_dir_entry *entry_cmoffset_all;
	struct proc_dir_entry *entry_fail_hist_all;

	ts->proc_cmoffset_size = (ts->tx_count * ts->rx_count * 4 + 100) * 4;	/* cm1 cm2 cm3 miscal*/
	ts->proc_cmoffset_all_size = ts->proc_cmoffset_size * 2;	/* sdc main */

	ts->proc_fail_hist_size = ((ts->tx_count + ts->rx_count) * 4 + 100) * 6;	/* have to check */
	ts->proc_fail_hist_all_size = ts->proc_fail_hist_size * 3;	/* sdc sub main */

	ts->cmoffset_sdc_proc = kzalloc(ts->proc_cmoffset_size, GFP_KERNEL);
	if (!ts->cmoffset_sdc_proc)
		return;

	ts->cmoffset_main_proc = kzalloc(ts->proc_cmoffset_size, GFP_KERNEL);
	if (!ts->cmoffset_main_proc)
		goto err_alloc_main;
	
	ts->miscal_proc= kzalloc(ts->proc_cmoffset_size, GFP_KERNEL);
	if (!ts->miscal_proc)
		goto err_alloc_miscal;

	ts->fail_hist_sdc_proc = kzalloc(ts->proc_fail_hist_size, GFP_KERNEL);
	if (!ts->fail_hist_sdc_proc)
		goto err_alloc_fail_hist_sdc;

	ts->fail_hist_sub_proc = kzalloc(ts->proc_fail_hist_size, GFP_KERNEL);
	if (!ts->fail_hist_sub_proc)
		goto err_alloc_fail_hist_sub;

	ts->fail_hist_main_proc = kzalloc(ts->proc_fail_hist_size, GFP_KERNEL);
	if (!ts->fail_hist_main_proc)
		goto err_alloc_fail_hist_main;

	entry_cmoffset_all = proc_create("tsp_cmoffset_all", S_IFREG | S_IRUGO, NULL, &tsp_cmoffset_all_file_ops);
	if (!entry_cmoffset_all) {
		input_err(true, &ts->client->dev, "%s: failed to create /proc/tsp_cmoffset_all\n", __func__);
		goto err_cmoffset_proc_create;
	}
	proc_set_size(entry_cmoffset_all, ts->proc_cmoffset_all_size);

	entry_fail_hist_all = proc_create("tsp_fail_hist_all", S_IFREG | S_IRUGO, NULL, &tsp_fail_hist_all_file_ops);
	if (!entry_fail_hist_all) {
		input_err(true, &ts->client->dev, "%s: failed to create /proc/tsp_fail_hist_all\n", __func__);
		goto err_fail_hist_proc_create;
	}
	proc_set_size(entry_fail_hist_all, ts->proc_fail_hist_all_size);

	g_ts = ts;
	input_info(true, &ts->client->dev, "%s: done\n", __func__);
	return;

err_fail_hist_proc_create:
	proc_remove(entry_cmoffset_all);
err_cmoffset_proc_create:
	kfree(ts->fail_hist_main_proc);
err_alloc_fail_hist_main:
	kfree(ts->fail_hist_sub_proc);
err_alloc_fail_hist_sub:
	kfree(ts->fail_hist_sdc_proc);
err_alloc_fail_hist_sdc:
	kfree(ts->miscal_proc);
err_alloc_miscal:
	kfree(ts->cmoffset_main_proc);
err_alloc_main:
	kfree(ts->cmoffset_sdc_proc);

	ts->cmoffset_sdc_proc = NULL;
	ts->cmoffset_main_proc = NULL;
	ts->miscal_proc = NULL;
	ts->cmoffset_all_proc = NULL;
	ts->proc_cmoffset_size = 0;
	ts->proc_cmoffset_all_size = 0;

	ts->fail_hist_sdc_proc = NULL;
	ts->fail_hist_sub_proc = NULL;
	ts->fail_hist_main_proc = NULL;
	ts->fail_hist_all_proc = NULL;
	ts->proc_fail_hist_size = 0;
	ts->proc_fail_hist_all_size = 0;

	input_err(true, &ts->client->dev, "%s: failed\n", __func__);
}
/* for debugging--------------------------------------------------------------------------------------*/
int sec_ts_pinctrl_configure(struct sec_ts_data *ts, bool enable)
{
	struct pinctrl_state *state;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, enable ? "ACTIVE" : "SUSPEND");

	if (enable) {
		state = pinctrl_lookup_state(ts->plat_data->pinctrl, "on_state");
		if (IS_ERR(ts->plat_data->pinctrl))
			input_err(true, &ts->client->dev, "%s: could not get active pinstate\n", __func__);
	} else {
		state = pinctrl_lookup_state(ts->plat_data->pinctrl, "off_state");
		if (IS_ERR(ts->plat_data->pinctrl))
			input_err(true, &ts->client->dev, "%s: could not get suspend pinstate\n", __func__);
	}

	if (!IS_ERR_OR_NULL(state))
		return pinctrl_select_state(ts->plat_data->pinctrl, state);

	return 0;
}

int sec_ts_power(void *data, bool on)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)data;
	const struct sec_ts_plat_data *pdata = ts->plat_data;
	struct regulator *regulator_dvdd = NULL;
	struct regulator *regulator_avdd = NULL;
	static bool enabled;
	int ret = 0;

	if (enabled == on)
		return ret;

	regulator_dvdd = regulator_get(NULL, pdata->regulator_dvdd);
	if (IS_ERR_OR_NULL(regulator_dvdd)) {
		input_err(true, &ts->client->dev, "%s: Failed to get %s regulator.\n",
				__func__, pdata->regulator_dvdd);
		ret = PTR_ERR(regulator_dvdd);
		goto error;
	}

	regulator_avdd = regulator_get(NULL, pdata->regulator_avdd);
	if (IS_ERR_OR_NULL(regulator_avdd)) {
		input_err(true, &ts->client->dev, "%s: Failed to get %s regulator.\n",
				__func__, pdata->regulator_avdd);
		ret = PTR_ERR(regulator_avdd);
		goto error;
	}

	if (on) {
		ret = regulator_enable(regulator_dvdd);
		if (ret) {
			input_err(true, &ts->client->dev, "%s: Failed to enable avdd: %d\n", __func__, ret);
			goto out;
		}

		sec_ts_delay(1);

		ret = regulator_enable(regulator_avdd);
		if (ret) {
			input_err(true, &ts->client->dev, "%s: Failed to enable vdd: %d\n", __func__, ret);
			goto out;
		}
	} else {
		regulator_disable(regulator_avdd);
		sec_ts_delay(4);
		regulator_disable(regulator_dvdd);
	}

	enabled = on;

out:
	input_err(true, &ts->client->dev, "%s: %s: avdd:%s, dvdd:%s\n", __func__, on ? "on" : "off",
			regulator_is_enabled(regulator_avdd) ? "on" : "off",
			regulator_is_enabled(regulator_dvdd) ? "on" : "off");

error:
	regulator_put(regulator_dvdd);
	regulator_put(regulator_avdd);

	return ret;
}

int sec_ts_parse_dt(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct sec_ts_plat_data *pdata = dev->platform_data;
	struct device_node *np = dev->of_node;
	u32 coords[2];
	int ret = 0;
	int count = 0;
	u32 ic_match_value;
	int lcdtype = 0;
#if defined(CONFIG_EXYNOS_DPU30)
	int connected;
#endif
	u32 px_zone[3] = { 0 };

	pdata->tsp_icid = of_get_named_gpio(np, "sec,tsp-icid_gpio", 0);
	if (gpio_is_valid(pdata->tsp_icid)) {
		input_info(true, dev, "%s: TSP_ICID : %d\n", __func__, gpio_get_value(pdata->tsp_icid));
		if (of_property_read_u32(np, "sec,icid_match_value", &ic_match_value)) {
			input_err(true, dev, "%s: Failed to get icid match value\n", __func__);
			return -EINVAL;
		}

		if (gpio_get_value(pdata->tsp_icid) != ic_match_value) {
			input_err(true, dev, "%s: Do not match TSP_ICID\n", __func__);
			return -EINVAL;
		}
	} else {
		input_err(true, dev, "%s: Failed to get tsp-icid gpio\n", __func__);
	}

	pdata->irq_gpio = of_get_named_gpio(np, "sec,irq_gpio", 0);
	if (gpio_is_valid(pdata->irq_gpio)) {
		ret = gpio_request_one(pdata->irq_gpio, GPIOF_DIR_IN, "sec,tsp_int");
		if (ret) {
			input_err(true, &client->dev, "%s: Unable to request tsp_int [%d]\n", __func__, pdata->irq_gpio);
			return -EINVAL;
		}
	} else {
		input_err(true, &client->dev, "%s: Failed to get irq gpio\n", __func__);
		return -EINVAL;
	}

	client->irq = gpio_to_irq(pdata->irq_gpio);

	if (of_property_read_u32(np, "sec,irq_type", &pdata->irq_type)) {
		input_err(true, dev, "%s: Failed to get irq_type property\n", __func__);
		pdata->irq_type = IRQF_TRIGGER_LOW | IRQF_ONESHOT;
	} else {
		input_info(true, dev, "%s: irq_type property:%X, %d\n", __func__,
				pdata->irq_type, pdata->irq_type);
	}

	if (of_property_read_u32(np, "sec,i2c-burstmax", &pdata->i2c_burstmax)) {
		input_dbg(false, &client->dev, "%s: Failed to get i2c_burstmax property\n", __func__);
		pdata->i2c_burstmax = 0xffff;
	}

	if (of_property_read_u32_array(np, "sec,max_coords", coords, 2)) {
		input_err(true, &client->dev, "%s: Failed to get max_coords property\n", __func__);
		return -EINVAL;
	}
	pdata->max_x = coords[0] - 1;
	pdata->max_y = coords[1] - 1;

	if (of_property_read_u32(np, "sec,bringup", &pdata->bringup) < 0)
		pdata->bringup = 0;

	pdata->tsp_id = of_get_named_gpio(np, "sec,tsp-id_gpio", 0);
	if (gpio_is_valid(pdata->tsp_id))
		input_info(true, dev, "%s: TSP_ID : %d\n", __func__, gpio_get_value(pdata->tsp_id));
	else
		input_err(true, dev, "%s: Failed to get tsp-id gpio\n", __func__);

	count = of_property_count_strings(np, "sec,firmware_name");
	if (count <= 0) {
		pdata->firmware_name = NULL;
	} else {
		if (gpio_is_valid(pdata->tsp_id) && gpio_get_value(pdata->tsp_id)) {
			of_property_read_string_index(np, "sec,firmware_name", gpio_get_value(pdata->tsp_id), &pdata->firmware_name);
			if (pdata->bringup == 4)
				pdata->bringup = 2;
		} else {
			of_property_read_string_index(np, "sec,firmware_name", 0, &pdata->firmware_name);
			if (pdata->bringup == 4)
				pdata->bringup = 3;
		}
	}

#if defined(CONFIG_DISPLAY_SAMSUNG)
	lcdtype = get_lcd_attached("GET");
	if (lcdtype == 0xFFFFFF) {
		input_err(true, &client->dev, "%s: lcd is not attached\n", __func__);
		return -ENODEV;
	}
#endif

#if defined(CONFIG_EXYNOS_DPU30)
	connected = get_lcd_info("connected");
	if (connected < 0) {
		input_err(true, dev, "%s: Failed to get lcd info\n", __func__);
		return -EINVAL;
	}

	if (!connected) {
		input_err(true, &client->dev, "%s: lcd is disconnected\n", __func__);
		return -ENODEV;
	}

	input_info(true, &client->dev, "%s: lcd is connected\n", __func__);

	lcdtype = get_lcd_info("id");
	if (lcdtype < 0) {
		input_err(true, dev, "%s: Failed to get lcd info\n", __func__);
		return -EINVAL;
	}
#endif
	input_info(true, &client->dev, "%s: lcdtype 0x%08X\n", __func__, lcdtype);

	if (of_property_read_string(np, "sec,regulator_dvdd", &pdata->regulator_dvdd)) {
		input_err(true, dev, "%s: Failed to get regulator_dvdd name property\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_string(np, "sec,regulator_avdd", &pdata->regulator_avdd)) {
		input_err(true, dev, "%s: Failed to get regulator_avdd name property\n", __func__);
		return -EINVAL;
	}

	pdata->power = sec_ts_power;

	pdata->regulator_boot_on = of_property_read_bool(np, "sec,regulator_boot_on");
	pdata->support_dex = of_property_read_bool(np, "support_dex_mode");
	pdata->support_fod = of_property_read_bool(np, "support_fod");
	pdata->enable_settings_aot = of_property_read_bool(np, "enable_settings_aot");
	pdata->sync_reportrate_120 = of_property_read_bool(np, "sync-reportrate-120");
	pdata->support_ear_detect = of_property_read_bool(np, "support_ear_detect_mode");
	pdata->support_open_short_test = of_property_read_bool(np, "support_open_short_test");
	pdata->support_mis_calibration_test = of_property_read_bool(np, "support_mis_calibration_test");

	if (of_property_read_u32_array(np, "sec,area-size", px_zone, 3)) {
		input_info(true, &client->dev, "Failed to get zone's size\n");
		pdata->area_indicator = 48;
		pdata->area_navigation = 96;
		pdata->area_edge = 60;
	} else {
		pdata->area_indicator = px_zone[0];
		pdata->area_navigation = px_zone[1];
		pdata->area_edge = px_zone[2];
	}
	input_info(true, &client->dev, "%s : zone's size - indicator:%d, navigation:%d, edge:%d\n",
		__func__, pdata->area_indicator, pdata->area_navigation ,pdata->area_edge);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	of_property_read_u32(np, "sec,ss_touch_num", &pdata->ss_touch_num);
	input_err(true, dev, "%s: ss_touch_num:%d\n", __func__, pdata->ss_touch_num);
#endif
#ifdef CONFIG_SEC_FACTORY
	pdata->support_mt_pressure = true;
#endif
	input_err(true, &client->dev, "%s: i2c buffer limit: %d, lcd_id:%06X, bringup:%d, FW:%s(%d),"
			" id:%d,%d, dex:%d, max(%d/%d), FOD:%d, AOT:%d, ED:%d\n",
			__func__, pdata->i2c_burstmax, lcdtype, pdata->bringup, pdata->firmware_name,
			count, pdata->tsp_id, pdata->tsp_icid,
			pdata->support_dex, pdata->max_x, pdata->max_y,
			pdata->support_fod, pdata->enable_settings_aot, pdata->support_ear_detect);
	return ret;
}

void sec_tclm_parse_dt(struct i2c_client *client, struct sec_tclm_data *tdata)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "sec,tclm_level", &tdata->tclm_level) < 0) {
		tdata->tclm_level = 0;
		input_err(true, dev, "%s: Failed to get tclm_level property\n", __func__);
	}

	if (of_property_read_u32(np, "sec,afe_base", &tdata->afe_base) < 0) {
		tdata->afe_base = 0;
		input_err(true, dev, "%s: Failed to get afe_base property\n", __func__);
	}

	tdata->support_tclm_test = of_property_read_bool(np, "support_tclm_test");

	input_err(true, &client->dev, "%s: tclm_level %d, sec_afe_base %04X\n", __func__, tdata->tclm_level, tdata->afe_base);
}

int sec_ts_read_information(struct sec_ts_data *ts)
{
	unsigned char data[13] = { 0 };
	int ret;

	memset(data, 0x0, 3);
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_ID, data, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read device id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: %X, %X, %X\n",
			__func__, data[0], data[1], data[2]);
	memset(data, 0x0, 11);
	ret = sec_ts_i2c_read(ts,  SEC_TS_READ_PANEL_INFO, data, 11);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
		return ret;
	}

	/* Set X,Y Resolution from IC information. */
	if (((data[0] << 8) | data[1]) > 0)
		ts->plat_data->max_x = ((data[0] << 8) | data[1]) - 1;

	if (((data[2] << 8) | data[3]) > 0)
		ts->plat_data->max_y = ((data[2] << 8) | data[3]) - 1;

	/* Set X,Y Display Resolution from IC information. */
	ts->plat_data->display_x = ((data[4] << 8) | data[5]) - 1;
	ts->plat_data->display_y = ((data[6] << 8) | data[7]) - 1;

	ts->tx_count = min((int)data[8], TOUCH_TX_CHANNEL_NUM);
	ts->rx_count = min((int)data[9], TOUCH_RX_CHANNEL_NUM);

	input_info(true, &ts->client->dev,
			"%s: nTX:%d, nRX:%d,  rX:%d, rY:%d, dX:%d, dY:%d\n",
			__func__, ts->tx_count , ts->rx_count,
			ts->plat_data->max_x, ts->plat_data->max_y,
			ts->plat_data->display_x, ts->plat_data->display_y);

	data[0] = 0;
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, data, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: STATUS : %X\n",
			__func__, data[0]);

	memset(data, 0x0, 4);
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, data, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: TOUCH STATUS : %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3]);
	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_TOUCHFUNCTION,  (u8 *)&(ts->touch_functions), 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read touch functions(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: Functions : %02X\n",
			__func__, ts->touch_functions);

	return ret;
}

int sec_ts_set_custom_library(struct sec_ts_data *ts)
{
	u8 data[3] = { 0 };
	int ret;
	u8 force_fod_enable = 0;

#ifdef CONFIG_SEC_FACTORY
	/* enable FOD when LCD on state */
	if (ts->plat_data->support_fod && ts->input_closed == false)
		force_fod_enable = SEC_TS_MODE_SPONGE_PRESS;
#endif

	input_err(true, &ts->client->dev, "%s: Sponge (0x%02x)%s\n",
			__func__, ts->lowpower_mode,
			force_fod_enable ? ", force fod enable" : "");

	data[2] = ts->lowpower_mode | force_fod_enable;

	ret = ts->sec_ts_write_sponge(ts, data, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write sponge\n", __func__);

	return ret;
}

int sec_ts_check_custom_library(struct sec_ts_data *ts)
{
	u8 data[10] = { 0 };
	int ret = -1;

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SPONGE_GET_INFO, &data[0], 10);

	input_info(true, &ts->client->dev,
			"%s: (%d) %c%c%c%c, || %02X, %02X, %02X, %02X, || %02X, %02X\n",
			__func__, ret, data[0], data[1], data[2], data[3], data[4],
			data[5], data[6], data[7], data[8], data[9]);

	sec_ts_set_custom_library(ts);

	return ret;
}

void sec_ts_set_input_prop(struct sec_ts_data *ts, struct input_dev *dev, u8 propbit)
{
	static char sec_ts_phys[64] = { 0 };

	snprintf(sec_ts_phys, sizeof(sec_ts_phys), "%s/input1",
			dev->name);
	dev->phys = sec_ts_phys;
	dev->id.bustype = BUS_I2C;
	dev->dev.parent = &ts->client->dev;

	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_KEY, dev->evbit);
	set_bit(EV_ABS, dev->evbit);
	set_bit(EV_SW, dev->evbit);
	set_bit(BTN_TOUCH, dev->keybit);
	set_bit(BTN_TOOL_FINGER, dev->keybit);
	set_bit(KEY_BLACK_UI_GESTURE, dev->keybit);
	set_bit(KEY_INT_CANCEL, dev->keybit);

	set_bit(propbit, dev->propbit);
	set_bit(KEY_WAKEUP, dev->keybit);

	input_set_abs_params(dev, ABS_MT_POSITION_X, 0, ts->plat_data->max_x, 0, 0);
	input_set_abs_params(dev, ABS_MT_POSITION_Y, 0, ts->plat_data->max_y, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MINOR, 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);
	if (ts->plat_data->support_mt_pressure)
		input_set_abs_params(dev, ABS_MT_PRESSURE, 0, 255, 0, 0);

	if (propbit == INPUT_PROP_POINTER)
		input_mt_init_slots(dev, MAX_SUPPORT_TOUCH_COUNT, INPUT_MT_POINTER);
	else
		input_mt_init_slots(dev, MAX_SUPPORT_TOUCH_COUNT, INPUT_MT_DIRECT);

	input_set_drvdata(dev, ts);
}

void sec_ts_set_input_prop_proximity(struct sec_ts_data *ts, struct input_dev *dev)
{
	static char sec_ts_phys[64] = { 0 };

	snprintf(sec_ts_phys, sizeof(sec_ts_phys), "%s/input1", dev->name);
	dev->phys = sec_ts_phys;
	dev->id.bustype = BUS_I2C;
	dev->dev.parent = &ts->client->dev;

	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_SW, dev->evbit);

	set_bit(INPUT_PROP_DIRECT, dev->propbit);

	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);
	input_set_drvdata(dev, ts);
}

void sec_ts_unlocked_release_all_finger(struct sec_ts_data *ts)
{
	int i;
	char location[SEC_TS_LOCATION_DETECT_SIZE] = { 0, };

	for (i = 0; i < MAX_SUPPORT_TOUCH_COUNT; i++) {
		input_mt_slot(ts->input_dev, i);
		input_report_abs(ts->input_dev, ABS_MT_CUSTOM, 0);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

		if ((ts->coord[i].action == SEC_TS_COORDINATE_ACTION_PRESS) ||
				(ts->coord[i].action == SEC_TS_COORDINATE_ACTION_MOVE)) {
			location_detect(ts, location, ts->coord[i].x, ts->coord[i].y);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &ts->client->dev,
					"[RA] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d p:%d noise:(%x,%d%d)\n",
					i, location,
					ts->coord[i].x - ts->coord[i].p_x,
					ts->coord[i].y - ts->coord[i].p_y,
					ts->coord[i].mcount, ts->touch_count,
					ts->coord[i].x, ts->coord[i].y,
					ts->coord[i].palm_count,
					ts->coord[i].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status);
#else
			input_info(true, &ts->client->dev,
					"[RA] tID:%d loc:%s dd:%d,%d mc:%d tc:%d p:%d noise:(%x,%d%d)\n",
					i, location,
					ts->coord[i].x - ts->coord[i].p_x,
					ts->coord[i].y - ts->coord[i].p_y,
					ts->coord[i].mcount, ts->touch_count,
					ts->coord[i].palm_count,
					ts->coord[i].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status);
#endif
			ts->coord[i].action = SEC_TS_COORDINATE_ACTION_RELEASE;
		}
		ts->coord[i].mcount = 0;
		ts->coord[i].palm_count = 0;
		ts->coord[i].noise_level = 0;
		ts->coord[i].max_strength = 0;
		ts->coord[i].hover_id_num = 0;
	}

	input_mt_slot(ts->input_dev, 0);

	input_report_key(ts->input_dev, BTN_TOUCH, false);
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, false);
	ts->touch_count = 0;
	ts->check_multi = 0;

	input_sync(ts->input_dev);
}

void sec_ts_locked_release_all_finger(struct sec_ts_data *ts)
{
	int i;
	char location[6] = { 0, };

	mutex_lock(&ts->eventlock);

	for (i = 0; i < MAX_SUPPORT_TOUCH_COUNT; i++) {
		input_mt_slot(ts->input_dev, i);
		input_report_abs(ts->input_dev, ABS_MT_CUSTOM, 0);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

		if ((ts->coord[i].action == SEC_TS_COORDINATE_ACTION_PRESS) ||
				(ts->coord[i].action == SEC_TS_COORDINATE_ACTION_MOVE)) {
			location_detect(ts, location, ts->coord[i].x, ts->coord[i].y);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &ts->client->dev,
					"[RA] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d p:%d noise:(%x,%d%d)\n",
					i, location,
					ts->coord[i].x - ts->coord[i].p_x,
					ts->coord[i].y - ts->coord[i].p_y,
					ts->coord[i].mcount, ts->touch_count,
					ts->coord[i].x, ts->coord[i].y,
					ts->coord[i].palm_count,
					ts->coord[i].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status);
#else
			input_info(true, &ts->client->dev,
					"[RA] tID:%d loc:%s dd:%d,%d mc:%d tc:%d p:%d noise:(%x,%d%d)\n",
					i, location,
					ts->coord[i].x - ts->coord[i].p_x,
					ts->coord[i].y - ts->coord[i].p_y,
					ts->coord[i].mcount, ts->touch_count,
					ts->coord[i].palm_count,
					ts->coord[i].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status);
#endif
			ts->coord[i].action = SEC_TS_COORDINATE_ACTION_RELEASE;
		}
		ts->coord[i].mcount = 0;
		ts->coord[i].palm_count = 0;
		ts->coord[i].noise_level = 0;
		ts->coord[i].max_strength = 0;
		ts->coord[i].hover_id_num = 0;
	}

	input_mt_slot(ts->input_dev, 0);

	input_report_key(ts->input_dev, BTN_TOUCH, false);
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, false);
	ts->touch_count = 0;
	ts->check_multi = 0;

	input_sync(ts->input_dev);

	mutex_unlock(&ts->eventlock);
}

#ifdef USE_POWER_RESET_WORK
void sec_ts_reset_work(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
			reset_work.work);
	int ret;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev, "%s: secure touch enabled\n", __func__);
		return;
	}
#endif
	if (ts->reset_is_on_going) {
		input_err(true, &ts->client->dev, "%s: reset is ongoing\n", __func__);
		return;
	}

	mutex_lock(&ts->modechange);
	wake_lock(&ts->wakelock);

	ts->reset_is_on_going = true;
	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_ts_stop_device(ts);

	sec_ts_delay(30);

	ret = sec_ts_start_device(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to reset, ret:%d\n", __func__, ret);
		ts->reset_is_on_going = false;
		cancel_delayed_work(&ts->reset_work);
		if (!shutdown_is_on_going_tsp)
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
		mutex_unlock(&ts->modechange);

		if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT) {
			char result[32];

			snprintf(result, sizeof(result), "RESULT=RESET");
			sec_cmd_send_event_to_user(&ts->sec, NULL, result);
		}

		wake_unlock(&ts->wakelock);

		return;
	}

	if (ts->input_dev_touch->disabled) {
		input_err(true, &ts->client->dev, "%s: call input_close\n", __func__);

		if (ts->lowpower_mode || ts->ed_enable) {
			ret = sec_ts_set_lowpowermode(ts, TO_LOWPOWER_MODE);
			if (ret < 0) {
				input_err(true, &ts->client->dev, "%s: failed to reset, ret:%d\n", __func__, ret);
				ts->reset_is_on_going = false;
				cancel_delayed_work(&ts->reset_work);
				if (!shutdown_is_on_going_tsp)
					schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
				mutex_unlock(&ts->modechange);
				wake_unlock(&ts->wakelock);
				return;
			}

			sec_ts_set_aod_rect(ts);
		} else {
			sec_ts_stop_device(ts);
		}
	}

	ts->reset_is_on_going = false;
	mutex_unlock(&ts->modechange);

	if (ts->power_status == SEC_TS_STATE_POWER_ON) {
		if (ts->fix_active_mode)
			sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	}

	if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT) {
		char result[32];

		snprintf(result, sizeof(result), "RESULT=RESET");
		sec_cmd_send_event_to_user(&ts->sec, NULL, result);
	}

	wake_unlock(&ts->wakelock);
}
#endif

void sec_ts_print_info_work(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
			work_print_info.work);
	sec_ts_print_info(ts);

	if (ts->sec.cmd_is_running) {
		input_err(true, &ts->client->dev, "%s: skip sec_ts_set_temp, cmd running\n", __func__);	
	} else {
		if (ts->touch_count) {
			ts->tsp_temp_data_skip = true;
			input_err(true, &ts->client->dev, "%s: skip sec_ts_set_temp, t_cnt(%d)\n", __func__, ts->touch_count);	
		} else {
			ts->tsp_temp_data_skip = false;
			sec_ts_set_temp(ts, false);
		}
	}
	if (!shutdown_is_on_going_tsp)
		schedule_delayed_work(&ts->work_print_info, msecs_to_jiffies(TOUCH_PRINT_INFO_DWORK_TIME));
}

void sec_ts_read_info_work(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
			work_read_info.work);
#ifdef TCLM_CONCEPT
	int ret;

	disable_irq(ts->client->irq);

	ret = sec_tclm_check_cal_case(ts->tdata);
	input_info(true, &ts->client->dev, "%s: sec_tclm_check_cal_case ret: %d \n", __func__, ret);

	enable_irq(ts->client->irq);
#endif
	ts->nv = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_FAC_RESULT);
	input_info(true, &ts->client->dev, "%s: fac_nv:%02X\n", __func__, ts->nv);
	input_log_fix();

	sec_ts_run_rawdata_all(ts, false);

	/* read cmoffset & fail history data at booting time */
	if (ts->proc_cmoffset_size) {
		get_cmoffset_dump_all(ts, ts->cmoffset_sdc_proc, OFFSET_FW_SDC);
		get_cmoffset_dump_all(ts, ts->cmoffset_main_proc, OFFSET_FW_MAIN);
		get_miscal_dump(ts, ts->miscal_proc);
	} else {
		input_err(true, &ts->client->dev, "%s: read cmoffset fail : alloc fail\n", __func__);
	}

	if (ts->proc_fail_hist_size) {
		get_selftest_fail_hist_dump_all(ts, ts->fail_hist_sdc_proc, OFFSET_FW_SDC);
		get_selftest_fail_hist_dump_all(ts, ts->fail_hist_sub_proc, OFFSET_FW_SUB);
		get_selftest_fail_hist_dump_all(ts, ts->fail_hist_main_proc, OFFSET_FW_MAIN);
	} else {
		input_err(true, &ts->client->dev, "%s: read fail-history fail : alloc fail\n", __func__);
	}

	ts->info_work_done = true;

	if (ts->shutdown_is_on_going) {
		input_err(true, &ts->client->dev, "%s done, do not run work\n", __func__);
		return;
	}

	if (!shutdown_is_on_going_tsp)
		schedule_work(&ts->work_print_info.work);

}

int sec_ts_set_lowpowermode(struct sec_ts_data *ts, u8 mode)
{
	int ret;
	int retrycnt = 0;
	char para = 0;

	input_err(true, &ts->client->dev, "%s: %s(%X)\n", __func__,
			mode == TO_LOWPOWER_MODE ? "ENTER" : "EXIT", ts->lowpower_mode);

	if (mode) {
		if (ts->prox_power_off) 
			sec_ts_set_prox_power_off(ts, 1);

		sec_ts_set_utc_sponge(ts);

		ret = sec_ts_set_custom_library(ts);
		if (ret < 0)
			goto i2c_error;
	} else {
		if (!shutdown_is_on_going_tsp)
			schedule_work(&ts->work_read_functions.work);
		sec_ts_set_prox_power_off(ts, 0);
	}

retry_pmode:
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &mode, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed\n", __func__);
		goto i2c_error;
	}

	sec_ts_delay(50);

	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read power mode failed!\n", __func__);
		goto i2c_error;
	} else {
		input_info(true, &ts->client->dev, "%s: power mode - write(%d) read(%d)\n", __func__, mode, para);
	}

	if (mode != para) {
		retrycnt++;
		ts->mode_change_failed_count++;
		if (retrycnt < 5)
			goto retry_pmode;
	}

	if (mode) {
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);
			goto i2c_error;
		}
	}

	sec_ts_locked_release_all_finger(ts);

	if (device_may_wakeup(&ts->client->dev)) {
		if (mode)
			enable_irq_wake(ts->client->irq);
		else
			disable_irq_wake(ts->client->irq);
	}

	if (mode == TO_LOWPOWER_MODE)
		ts->power_status = SEC_TS_STATE_LPM;
	else
		ts->power_status = SEC_TS_STATE_POWER_ON;

i2c_error:
	input_info(true, &ts->client->dev, "%s: end %d\n", __func__, ret);

	return ret;
}

#ifdef CONFIG_SAMSUNG_TUI
extern int stui_i2c_lock(struct i2c_adapter *adap);
extern int stui_i2c_unlock(struct i2c_adapter *adap);

int stui_tsp_enter(void)
{
	int ret = 0;

	if (!tsp_info)
		return -EINVAL;

	disable_irq(tsp_info->client->irq);
	sec_ts_unlocked_release_all_finger(tsp_info);

	ret = stui_i2c_lock(tsp_info->client->adapter);
	if (ret) {
		pr_err("[STUI] stui_i2c_lock failed : %d\n", ret);
		enable_irq(tsp_info->client->irq);
		return -1;
	}

	return 0;
}

int stui_tsp_exit(void)
{
	int ret = 0;

	if (!tsp_info)
		return -EINVAL;

	ret = stui_i2c_unlock(tsp_info->client->adapter);
	if (ret)
		pr_err("[STUI] stui_i2c_unlock failed : %d\n", ret);

	enable_irq(tsp_info->client->irq);

	return ret;
}
#endif

int sec_ts_fix_tmode(struct sec_ts_data *ts, u8 mode, u8 state)
{
	int ret;
	u8 onoff[1] = {STATE_MANAGE_OFF};
	u8 tBuff[2] = { mode, state };

	input_info(true, &ts->client->dev, "%s\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, onoff, 1);
	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, tBuff, sizeof(tBuff));
	sec_ts_delay(20);

	return ret;
}

int sec_ts_p2p_tmode(struct sec_ts_data *ts)
{
	int ret;
	u8 mode[3] = {0x2F, 0x00, 0xDE};
	char para = TO_SELFTEST_MODE;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	sec_ts_delay(30);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_P2P_MODE, mode, sizeof(mode));
	sec_ts_delay(30);

	return ret;
}

int execute_p2ptest(struct sec_ts_data *ts)
{
	int ret;
	u8 test[2] = {0x00, 0x32};
	u8 tBuff[10] = {0};
	int retry = 0;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_P2P_TEST, test, sizeof(test));
	sec_ts_delay(600);

	ret = -1;

	while (ts->sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, tBuff, 8)) {
		if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_VENDOR_INFO) {
			if (tBuff[1] == SEC_TS_VENDOR_ACK_CMR_TEST_DONE) {
				ts->cm_raw_set_avg_max = (tBuff[2] & 0xFF) << 8 | (tBuff[3] & 0xFF);
				ts->cm_raw_set_avg_min = (tBuff[4] & 0xFF) << 8 | (tBuff[5] & 0xFF);
				ts->cm_raw_set_p2p = (tBuff[7] & 0xC0) << 2 | (tBuff[6] & 0xFF);
			} else if (tBuff[1] == SEC_TS_VENDOR_ACK_CSR_TX_TEST_DONE) {
				ts->self_raw_set_avg_tx_max = (tBuff[2] & 0xFF) << 8 | (tBuff[3] & 0xFF);
				ts->self_raw_set_avg_tx_min = (tBuff[4] & 0xFF) << 8 | (tBuff[5] & 0xFF);
				ts->self_raw_set_p2p_tx_diff = (tBuff[7] & 0xC0) << 2 | (tBuff[6] & 0xFF);
			} else if (tBuff[1] == SEC_TS_VENDOR_ACK_CSR_RX_TEST_DONE) {
				ts->self_raw_set_avg_rx_max = (tBuff[2] & 0xFF) << 8 | (tBuff[3] & 0xFF);
				ts->self_raw_set_avg_rx_min = (tBuff[4] & 0xFF) << 8 | (tBuff[5] & 0xFF);
				ts->self_raw_set_p2p_rx_diff = (tBuff[7] & 0xC0) << 2 | (tBuff[6] & 0xFF);
			} else if (tBuff[1] == SEC_TS_VENDOR_ACK_CMR_KEY_TEST_DONE) {
				ts->cm_raw_key_p2p_max = (tBuff[2] & 0xFF) << 8 | (tBuff[3] & 0xFF);
				ts->cm_raw_key_p2p_min = (tBuff[4] & 0xFF) << 8 | (tBuff[5] & 0xFF);
				ts->cm_raw_key_p2p_diff = (tBuff[7] & 0xC0) << 2 | (tBuff[6] & 0xFF);
			} else if (tBuff[1] == SEC_TS_VENDOR_ACK_RX_NODE_GAP_TEST_DONE) {
				ts->cm_raw_set_p2p_gap_y = (tBuff[2] & 0xFF) << 8 | (tBuff[3] & 0xFF);
				ts->cm_raw_set_p2p_gap_y_result = (tBuff[4] & 0x01);
			}
		}

		if ((tBuff[7] & 0x3F) == 0x00) {
			input_info(true, &ts->client->dev, "%s: left event is 0\n", __func__);
			ret = 0;
			break;
		}

		if (retry++ > SEC_TS_WAIT_RETRY_CNT) {
			input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
			break;
		}
		sec_ts_delay(20);
	}
	return ret;
}

int sec_ts_release_tmode(struct sec_ts_data *ts)
{
	int ret;
	u8 onoff[1] = {STATE_MANAGE_ON};

	input_info(true, &ts->client->dev, "%s\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, onoff, 1);
	sec_ts_delay(20);

	return ret;
}

/* Use TSP NV area
 * buff[0] : offset from user NVM storage
 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
 * buff[2] : write data
 * buff[..] : cont.
 */
void set_tsp_nvm_data_clear(struct sec_ts_data *ts, u8 offset)
{
	char buff[4] = { 0 };
	int ret;

	input_dbg(true, &ts->client->dev, "%s\n", __func__);

	buff[0] = offset;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: nvm write failed. ret: %d\n", __func__, ret);

	sec_ts_delay(20);
}

int get_tsp_nvm_data(struct sec_ts_data *ts, u8 offset)
{
	char buff[2] = { 0 };
	int ret;

	/* SENSE OFF -> CELAR EVENT STACK -> READ NV -> SENSE ON */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_OFF, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write Sense_off\n", __func__);
		goto out_nvm;
	}

	input_dbg(false, &ts->client->dev, "%s: SENSE OFF\n", __func__);

	sec_ts_delay(100);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);
		goto out_nvm;
	}

	input_dbg(false, &ts->client->dev, "%s: CLEAR EVENT STACK\n", __func__);

	sec_ts_delay(100);

	sec_ts_locked_release_all_finger(ts);

	/* send NV data using command
	 * Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 */
	memset(buff, 0x00, 2);
	buff[0] = offset;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	sec_ts_delay(20);

	/* read NV data
	 * Use TSP NV area : in this model, use only one byte
	 */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_NVM, buff, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	input_info(true, &ts->client->dev, "%s: offset:%u  data:%02X\n", __func__, offset, buff[0]);

out_nvm:
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);

	sec_ts_delay(300);

	input_dbg(false, &ts->client->dev, "%s: SENSE ON\n", __func__);

	return buff[0];
}

int get_tsp_nvm_data_by_size(struct sec_ts_data *ts, u8 offset, int length, u8 *data)
{
	char *buff = NULL;
	int ret;

	buff = kzalloc(length, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	input_info(true, &ts->client->dev, "%s: offset:%u, length:%d, size:%d\n", __func__, offset, length, sizeof(data));

	/* SENSE OFF -> CELAR EVENT STACK -> READ NV -> SENSE ON */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_OFF, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write Sense_off\n", __func__);
		goto out_nvm;
	}

	input_dbg(true, &ts->client->dev, "%s: SENSE OFF\n", __func__);

	sec_ts_delay(100);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);
		goto out_nvm;
	}

	input_dbg(true, &ts->client->dev, "%s: CLEAR EVENT STACK\n", __func__);

	sec_ts_delay(100);

	sec_ts_locked_release_all_finger(ts);

	/* send NV data using command
	 * Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 */
	memset(buff, 0x00, 2);
	buff[0] = offset;
	buff[1] = length - 1;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	sec_ts_delay(20);

	/* read NV data
	 * Use TSP NV area : in this model, use only one byte
	 */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_NVM, buff, length);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	memcpy(data, buff, length);

out_nvm:
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);

	sec_ts_delay(300);

	input_dbg(true, &ts->client->dev, "%s: SENSE ON\n", __func__);

	kfree(buff);

	return ret;
}

int set_tsp_nvm_data_by_size(struct sec_ts_data *ts, u8 reg, int size, u8 *data)
{
	int rc;
	u8 buff[SEC_CMD_STR_LEN] = {0};

	buff[0] = reg;
	buff[1] = size - 1;	/* 1bytes */
	memcpy(&buff[2], data, size);
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, size + 2);
	if (rc < 0) {
		input_err(true, &ts->client->dev,
			"%s: nvm write failed. ret: %d\n", __func__, rc);
	}
	sec_ts_delay(20);
	return rc;
}

int sec_tclm_data_read(struct i2c_client *client, int address)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);
	int ret = 0;
	u8 buff[4];
	u8 nbuff[SEC_TS_NVM_OFFSET_LENGTH - SEC_TS_NVM_OFFSET_CAL_COUNT];

	switch (address) {
	case SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER:
		sec_ts_delay(100);
		ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_IMG_VERSION, buff, 4);
		if (ret < 0)
			return ret;
		input_err(true, &ts->client->dev, "%s SEC_TS_READ_IMG_VERSION buff[2]:%02X buff[3]:%02X \n",
			__func__, buff[2], buff[3]);
		ret = (buff[2] << 8) | buff[3];
		return ret;
	case SEC_TCLM_NVM_ALL_DATA:
		memset(&ts->tdata->nvdata, 0x00, sizeof(struct sec_tclm_nvdata));
		
		ret = get_tsp_nvm_data_by_size(ts, SEC_TS_NVM_OFFSET_CAL_COUNT, sizeof(struct sec_tclm_nvdata), nbuff);
		if (ret < 0)
			return ret;

		memcpy(&ts->tdata->nvdata, nbuff, sizeof(struct sec_tclm_nvdata));
		return ret;
	case SEC_TCLM_NVM_TEST:
		input_info(true, &ts->client->dev, "%s: dt: tclm_level [%d] afe_base [%04X]\n",
			__func__, ts->tdata->tclm_level, ts->tdata->afe_base);
		ret = get_tsp_nvm_data_by_size(ts, SEC_TS_NVM_TOTAL_OFFSET_LENGTH + SEC_TCLM_NVM_OFFSET,
			SEC_TCLM_NVM_OFFSET_LENGTH, ts->tdata->tclm);
		if (ts->tdata->tclm[0] != 0xFF) {
			ts->tdata->tclm_level = ts->tdata->tclm[0];
			ts->tdata->afe_base = (ts->tdata->tclm[1] << 8) | ts->tdata->tclm[2];
		input_info(true, &ts->client->dev, "%s: nv: tclm_level [%d] afe_base [%04X]\n",
			__func__, ts->tdata->tclm_level, ts->tdata->afe_base);
		}
		return ret;
	default:
		return ret;
	}
}

int sec_tclm_data_write(struct i2c_client *client, int address)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);
	int ret = 1;
	u8 nbuff[SEC_TS_NVM_OFFSET_LENGTH - SEC_TS_NVM_OFFSET_CAL_COUNT];

	memset(nbuff, 0x00, sizeof(struct sec_tclm_nvdata));
	switch (address) {
	case SEC_TCLM_NVM_ALL_DATA:
		memcpy(nbuff, &ts->tdata->nvdata, sizeof(struct sec_tclm_nvdata));
		ret = set_tsp_nvm_data_by_size(ts, SEC_TS_NVM_OFFSET_CAL_COUNT, sizeof(struct sec_tclm_nvdata), nbuff);
		return ret;
	case SEC_TCLM_NVM_TEST:
		ret = set_tsp_nvm_data_by_size(ts, SEC_TS_NVM_TOTAL_OFFSET_LENGTH + SEC_TCLM_NVM_OFFSET,
			SEC_TCLM_NVM_OFFSET_LENGTH, ts->tdata->tclm);
		return ret;
	default:
		return ret;
	}
}

#ifdef MINORITY_REPORT
/*	ts->defect_probability is FFFFF
 *
 *	0	is 100% normal.
 *	1~9	is normal, but need check
 *	A~E	is abnormal, must check
 *	F	is Device defect
 *
 *	F----	: ito
 *	-F---	: rawdata
 *	--A--	: crc
 *	---A-	: i2c_err
 *	----A	: wet
 */
void minority_report_calculate_rawdata(struct sec_ts_data *ts)
{
	int ii, jj;
	int temp = 0;
	int max = -30000;
	int min = 30000;
	int node_gap = 1;
	short tx_max[TOUCH_TX_CHANNEL_NUM] = { 0 };
	short tx_min[TOUCH_TX_CHANNEL_NUM] = { 0 };
	short rx_max[TOUCH_RX_CHANNEL_NUM] = { 0 };
	short rx_min[TOUCH_RX_CHANNEL_NUM] = { 0 };

	for (ii = 0; ii < TOUCH_TX_CHANNEL_NUM; ii++) {
		tx_max[ii] = -30000;
		tx_min[ii] = 30000;
	}

	for (ii = 0; ii < TOUCH_RX_CHANNEL_NUM; ii++) {
		rx_max[ii] = -30000;
		rx_min[ii] = 30000;
	}

	if (!ts->tx_count) {
		ts->item_rawdata = 0xD;
		return;
	}

	for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
		if (max < ts->pFrame[ii]) {
			ts->max_ambient = max = ts->pFrame[ii];
			ts->max_ambient_channel_tx = ii % ts->tx_count;
			ts->max_ambient_channel_rx = ii / ts->tx_count;
		}
		if (min > ts->pFrame[ii]) {
			ts->min_ambient = min = ts->pFrame[ii];
			ts->min_ambient_channel_tx = ii % ts->tx_count;
			ts->min_ambient_channel_rx = ii / ts->tx_count;
		}

		if ((ii + 1) % (ts->tx_count) != 0) {
			temp = ts->pFrame[ii] - ts->pFrame[ii+1];
			if (temp < 0)
				temp = -temp;
			if (temp > node_gap)
				node_gap = temp;
		}

		if (ii < (ts->rx_count - 1) * ts->tx_count) {
			temp = ts->pFrame[ii] - ts->pFrame[ii + ts->tx_count];
			if (temp < 0)
				temp = -temp;
			if (temp > node_gap)
				node_gap = temp;
		}

		ts->ambient_rx[ii / ts->tx_count] += ts->pFrame[ii];
		ts->ambient_tx[ii % ts->tx_count] += ts->pFrame[ii];

		rx_max[ii / ts->tx_count] = max(rx_max[ii / ts->tx_count], ts->pFrame[ii]);
		rx_min[ii / ts->tx_count] = min(rx_min[ii / ts->tx_count], ts->pFrame[ii]);
		tx_max[ii % ts->tx_count] = max(tx_max[ii % ts->tx_count], ts->pFrame[ii]);
		tx_min[ii % ts->tx_count] = min(tx_min[ii % ts->tx_count], ts->pFrame[ii]);

	}

	for (ii = 0; ii < ts->tx_count; ii++)
		ts->ambient_tx[ii] /= ts->rx_count;

	for (ii = 0; ii < ts->rx_count; ii++)
		ts->ambient_rx[ii] /= ts->tx_count;

	for (ii = 0; ii < ts->rx_count; ii++)
		ts->ambient_rx_delta[ii] = rx_max[ii] - rx_min[ii];

	for (jj = 0; jj < ts->tx_count; jj++)
		ts->ambient_tx_delta[jj] = tx_max[jj] - tx_min[jj];

	if (max >= 80 || min <= -80)
		ts->item_rawdata = 0xF;
	else if ((max >= 50 || min <= -50) && (node_gap > 40))
		ts->item_rawdata = 0xC;
	else if (max >= 50 || min <= -50)
		ts->item_rawdata = 0xB;
	else if (node_gap > 40)
		ts->item_rawdata = 0xA;
	else if ((max >= 40 || min <= -40) && (node_gap > 30))
		ts->item_rawdata = 0x3;
	else if (max >= 40 || min <= -40)
		ts->item_rawdata = 0x2;
	else if (node_gap > 30)
		ts->item_rawdata = 0x1;
	else
		ts->item_rawdata = 0;

	input_info(true, &ts->client->dev, "%s min:%d,max:%d,node gap:%d =>%X\n",
			__func__, min, max, node_gap, ts->item_rawdata);

}

void minority_report_calculate_ito(struct sec_ts_data *ts)
{

	if (ts->ito_test[0] ||  ts->ito_test[1] || ts->ito_test[2] || ts->ito_test[3])
		ts->item_ito = 0xF;
	else
		ts->item_ito = 0;
}

u8 minority_report_check_count(int value)
{
	u8 ret;

	if (value > 160)
		ret = 0xA;
	else if (value > 90)
		ret = 3;
	else if (value > 40)
		ret = 2;
	else if (value > 10)
		ret = 1;
	else
		ret = 0;

	return ret;
}

void minority_report_sync_latest_value(struct sec_ts_data *ts)
{
	u32 temp = 0;

	/* crc */
	if (ts->checksum_result == 1)
		ts->item_crc = 0xA;

	/* i2c_err */
	ts->item_i2c_err = minority_report_check_count(ts->comm_err_count);

	/* wet */
	ts->item_wet = minority_report_check_count(ts->wet_count);

	temp |= (ts->item_ito & 0xF) << 16;
	temp |= (ts->item_rawdata & 0xF) << 12;
	temp |= (ts->item_crc & 0xF) << 8;
	temp |= (ts->item_i2c_err & 0xF) << 4;
	temp |= (ts->item_wet & 0xF);

	ts->defect_probability = temp;
}
#endif

void sec_ts_swap(u8 *a, u8 *b)
{
	u8 temp = *a;

	*a = *b;
	*b = temp;
}

void rearrange_sft_result(u8 *data, int length)
{
	int i, nlength;

	nlength = length - (length % 4);

	for (i = 0; i < nlength; i += 4) {
		sec_ts_swap(&data[i], &data[i + 3]);
		sec_ts_swap(&data[i + 1], &data[i + 2]);
	}
}

int sec_ts_set_temp(struct sec_ts_data *ts, bool bforced)
{
	int ret = 0;
	u8 temp_data = 0;

	if (!ts->psy)
		ts->psy = power_supply_get_by_name("battery");

	if (!ts->psy) {
		input_err(true, &ts->client->dev, "%s: Cannot find power supply\n", __func__);
		return -1;
	}

	ret = power_supply_get_property(ts->psy, POWER_SUPPLY_PROP_TEMP, &ts->psy_value);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Couldn't get aicl settled value ret=%d\n", __func__, ret);
		return ret;
	}

	temp_data = (u8)(ts->psy_value.intval / 10);
	if (bforced || ts->tsp_temp_data != temp_data) {
		ret = ts->sec_ts_i2c_write(ts, SET_TS_CMD_SET_LOWTEMPERATURE_MODE, &temp_data, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to write\n", __func__);
			return ret;
		}
		ts->tsp_temp_data = temp_data;
		input_info(true, &ts->client->dev, "%s set temperature:%d\n", __func__, (s8)temp_data);
	} else {
		input_dbg(true, &ts->client->dev, "%s skip temperature:%d\n", __func__, (s8)temp_data);
	}

	return ret;
}

int sec_ts_set_aod_rect(struct sec_ts_data *ts)
{
	u8 data[10] = {0x02, 0};
	int ret, i;

	if (!(ts->lowpower_mode & SEC_TS_MODE_SPONGE_AOD))
		return 0;

	for (i = 0; i < 4; i++) {
		data[i * 2 + 2] = ts->rect_data[i] & 0xFF;
		data[i * 2 + 3] = (ts->rect_data[i] >> 8) & 0xFF;
	}

	ret = ts->sec_ts_write_sponge(ts, data, 10);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write sponge\n", __func__);

	if (ts->power_status == SEC_TS_STATE_LPM) {
		if (ts->rect_data[0] == 0 && ts->rect_data[1] == 0 &&
			ts->rect_data[2] == 0 && ts->rect_data[3] == 0 ) {
	
			data[0] = SEC_TS_CMD_LPM_AOD_OFF;
		} else {
			data[0] = SEC_TS_CMD_LPM_AOD_ON;
		}
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_LPM_AOD_OFF_ON, &data[0], 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send aod off_on cmd\n", __func__);
		}
	}

	return ret;
}

int sec_ts_set_press_property(struct sec_ts_data *ts)
{
	u8 data[3] = { SEC_TS_CMD_SPONGE_PRESS_PROPERTY, 0 };
	int ret;

	if (!ts->plat_data->support_fod)
		return 0;

	data[2] = ts->press_prop;

	ret = ts->sec_ts_write_sponge(ts, data, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write sponge\n", __func__);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, ts->press_prop);

	return ret;
}

int sec_ts_set_fod_rect(struct sec_ts_data *ts)
{
	u8 data[10] = {0x4b, 0};
	int ret, i;

	input_info(true, &ts->client->dev, "%s: l:%d, t:%d, r:%d, b:%d\n",
		__func__, ts->fod_rect_data[0], ts->fod_rect_data[1],
		ts->fod_rect_data[2], ts->fod_rect_data[3]);

	for (i = 0; i < 4; i++) {
		data[i * 2 + 2] = ts->fod_rect_data[i] & 0xFF;
		data[i * 2 + 3] = (ts->fod_rect_data[i] >> 8) & 0xFF;
	}

	ret = ts->sec_ts_write_sponge(ts, data, 10);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write sponge\n", __func__);

	return ret;
}

/*
 *	flag     1  :  set edge handler
 *		2  :  set (portrait, normal) edge zone data
 *		4  :  set (portrait, normal) dead zone data
 *		8  :  set landscape mode data
 *		16 :  mode clear
 *	data
 *		0xAA, FFF (y start), FFF (y end),  FF(direction)
 *		0xAB, FFFF (edge zone)
 *		0xAC, FF (up x), FF (down x), FFFF (y)
 *		0xAD, FF (mode), FFF (edge), FFF (dead zone x), FF (dead zone top y), FF (dead zone bottom y)
 *	case
 *		edge handler set :  0xAA....
 *		booting time :  0xAA...  + 0xAB...
 *		normal mode : 0xAC...  (+0xAB...)
 *		landscape mode : 0xAD...
 *		landscape -> normal (if same with old data) : 0xAD, 0
 *		landscape -> normal (etc) : 0xAC....  + 0xAD, 0
 */

void set_grip_data_to_ic(struct sec_ts_data *ts, u8 flag)
{
	u8 data[8] = { 0 };

	input_info(true, &ts->client->dev, "%s: flag: %02X (clr,lan,nor,edg,han)\n", __func__, flag);

	if (flag & G_SET_EDGE_HANDLER) {
		if (ts->grip_edgehandler_direction == 0) {
			data[0] = 0x0;
			data[1] = 0x0;
			data[2] = 0x0;
			data[3] = 0x0;
		} else {
			data[0] = (ts->grip_edgehandler_start_y >> 4) & 0xFF;
			data[1] = (ts->grip_edgehandler_start_y << 4 & 0xF0) | ((ts->grip_edgehandler_end_y >> 8) & 0xF);
			data[2] = ts->grip_edgehandler_end_y & 0xFF;
			data[3] = ts->grip_edgehandler_direction & 0x3;
		}
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_EDGE_HANDLER, data, 4);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X,%02X,%02X,%02X\n",
				__func__, SEC_TS_CMD_EDGE_HANDLER, data[0], data[1], data[2], data[3]);
	}

	if (flag & G_SET_EDGE_ZONE) {
		data[0] = (ts->grip_edge_range >> 8) & 0xFF;
		data[1] = ts->grip_edge_range  & 0xFF;
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_EDGE_AREA, data, 2);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X,%02X\n",
				__func__, SEC_TS_CMD_EDGE_AREA, data[0], data[1]);
	}

	if (flag & G_SET_NORMAL_MODE) {
		data[0] = ts->grip_deadzone_up_x & 0xFF;
		data[1] = ts->grip_deadzone_dn_x & 0xFF;
		data[2] = (ts->grip_deadzone_y >> 8) & 0xFF;
		data[3] = ts->grip_deadzone_y & 0xFF;
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_DEAD_ZONE, data, 4);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X,%02X,%02X,%02X\n",
				__func__, SEC_TS_CMD_DEAD_ZONE, data[0], data[1], data[2], data[3]);
	}

	if (flag & G_SET_LANDSCAPE_MODE) {
		data[0] = ts->grip_landscape_mode & 0x1;
		data[1] = (ts->grip_landscape_edge >> 4) & 0xFF;
		data[2] = (ts->grip_landscape_edge << 4 & 0xF0) | ((ts->grip_landscape_deadzone >> 8) & 0xF);
		data[3] = ts->grip_landscape_deadzone & 0xFF;
		data[4] = ts->grip_landscape_top_deadzone & 0xFF;
		data[5] = ts->grip_landscape_bottom_deadzone & 0xFF;
		data[6] = ts->grip_landscape_top_gripzone & 0xFF;
		data[7] = ts->grip_landscape_bottom_gripzone & 0xFF;
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_LANDSCAPE_MODE, data, 8);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X,%02X,%02X,%02X, %02X,%02X,%02X,%02X\n",
				__func__, SEC_TS_CMD_LANDSCAPE_MODE, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
	}

	if (flag & G_CLR_LANDSCAPE_MODE) {
		data[0] = ts->grip_landscape_mode;
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_LANDSCAPE_MODE, data, 1);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X\n",
				__func__, SEC_TS_CMD_LANDSCAPE_MODE, data[0]);
	}
}

/*
 * Enable or disable external_noise_mode
 *
 * If mode has EXT_NOISE_MODE_MAX,
 * then write enable cmd for all enabled mode. (set as ts->external_noise_mode bit value)
 * This routine need after IC power reset. TSP IC need to be re-wrote all enabled modes.
 *
 * Else if mode has specific value like EXT_NOISE_MODE_MONITOR,
 * then write enable/disable cmd about for that mode's latest setting value.
 *
 * If you want to add new mode,
 * please define new enum value like EXT_NOISE_MODE_MONITOR,
 * then set cmd for that mode like below. (it is in this function)
 * noise_mode_cmd[EXT_NOISE_MODE_MONITOR] = SEC_TS_CMD_SET_MONITOR_NOISE_MODE;
 */
int sec_ts_set_external_noise_mode(struct sec_ts_data *ts, u8 mode)
{
	int i, ret, fail_count = 0;
	u8 mode_bit_to_set, check_bit, mode_enable;
	u8 noise_mode_cmd[EXT_NOISE_MODE_MAX] = { 0 };

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		return -ENODEV;
	}

	if (mode == EXT_NOISE_MODE_MAX) {
		/* write all enabled mode */
		mode_bit_to_set = ts->external_noise_mode;
	} else {
		/* make enable or disable the specific mode */
		mode_bit_to_set = 1 << mode;
	}

	input_info(true, &ts->client->dev, "%s: %sable %d\n", __func__,
			ts->external_noise_mode & mode_bit_to_set ? "en" : "dis", mode_bit_to_set);

	/* set cmd for each mode */
	noise_mode_cmd[EXT_NOISE_MODE_MONITOR] = SEC_TS_CMD_SET_MONITOR_NOISE_MODE;

	/* write mode */
	for (i = EXT_NOISE_MODE_NONE + 1; i < EXT_NOISE_MODE_MAX; i++) {
		check_bit = 1 << i;
		if (mode_bit_to_set & check_bit) {
			mode_enable = !!(ts->external_noise_mode & check_bit);
			ret = ts->sec_ts_i2c_write(ts, noise_mode_cmd[i], &mode_enable, 1);
			if (ret < 0) {
				input_err(true, &ts->client->dev, "%s: failed to set 0x%02X %d\n",
						__func__, noise_mode_cmd[i], mode_enable);
				fail_count++;
			}
		}
	}

	if (fail_count != 0)
		return -EIO;
	else
		return 0;
}

MODULE_LICENSE("GPL");
