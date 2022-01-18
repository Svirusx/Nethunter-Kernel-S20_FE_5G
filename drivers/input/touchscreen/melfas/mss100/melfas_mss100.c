/*
 * MELFAS MMS400 Touchscreen
 *
 * Copyright (C) 2014 MELFAS Inc.
 *
 */

#include "melfas_mss100.h"

#if MMS_USE_NAP_MODE
struct wake_lock mms_wake_lock;
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
#include <linux/t-base-tui.h>
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
extern int tui_force_close(uint32_t arg);
struct mms_ts_info *tui_tsp_info;
#endif
#ifdef CONFIG_SAMSUNG_TUI
struct mms_ts_info *tsp_info;
#endif

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
static irqreturn_t mms_interrupt(int irq, void *dev_id);

static irqreturn_t secure_filter_interrupt(struct mms_ts_info *info)
{
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLE) {
		if (atomic_cmpxchg(&info->secure_pending_irqs, 0, 1) == 0)
			sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");
		else
			input_info(true, &info->client->dev, "%s: pending irq:%d\n",
					__func__, (int)atomic_read(&info->secure_pending_irqs));

		return IRQ_HANDLED;
	}
	return IRQ_NONE;

}

/**
 * Sysfs attr group for secure touch & interrupt handler for Secure world.
 * @atomic : syncronization for secure_enabled
 * @pm_runtime : set rpm_resume or rpm_ilde
 */
static ssize_t secure_touch_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d", atomic_read(&info->secure_enabled));
}

static ssize_t secure_touch_enable_store(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	int ret;
	unsigned long data;

	if (count > 2) {
		input_err(true, &info->client->dev,
				"%s: cmd length is over (%s,%d)!!\n",
				__func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	ret = kstrtoul(buf, 10, &data);
	if (ret != 0) {
		input_err(true, &info->client->dev, "%s: failed to read:%d\n",
				__func__, ret);
		return -EINVAL;
	}

	if (data == 1) {
		if (info->reset_is_on_going) {
			input_err(true, &info->client->dev, "%s: reset is on going because i2c fail\n", __func__);
			return -EBUSY;
		}

		/* Enable Secure World */
		if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLE) {
			input_err(true, &info->client->dev, "%s: already enabled\n", __func__);
			return -EBUSY;
		}

		/* syncronize_irq -> disable_irq + enable_irq
		 * concern about timing issue.
		 */
		disable_irq(info->client->irq);

		/* Release All Finger */

		if (pm_runtime_get_sync(info->client->adapter->dev.parent) < 0) {
			enable_irq(info->client->irq);
			input_err(true, &info->client->dev, "%s: failed to get pm_runtime\n", __func__);
			return -EIO;
		}

		msleep(200);
		reinit_completion(&info->secure_powerdown);
		reinit_completion(&info->secure_interrupt);
		atomic_set(&info->secure_enabled, 1);
		atomic_set(&info->secure_pending_irqs, 0);

		enable_irq(info->client->irq);

		input_info(true, &info->client->dev, "%s: secure touch enable\n", __func__);
	} else if (data == 0) {
		/* Disable Secure World */
		if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_DISABLE) {
			input_err(true, &info->client->dev, "%s: already disabled\n", __func__);
			return count;
		}

		pm_runtime_put_sync(info->client->adapter->dev.parent);

		/* add delay*/
		msleep(200);
		atomic_set(&info->secure_enabled, 0);

		sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");

		mms_interrupt(info->client->irq, info);
		complete(&info->secure_interrupt);
		complete(&info->secure_powerdown);

		input_info(true, &info->client->dev, "%s: secure touch disable\n", __func__);
	} else {
		input_err(true, &info->client->dev, "%s: unsupport value:%ld\n", __func__, data);
		return -EINVAL;
	}

	return count;
}

static ssize_t secure_touch_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_DISABLE) {
		input_err(true, &info->client->dev, "%s: disabled\n", __func__);
		return -EBADF;
	}

	if (atomic_cmpxchg(&info->secure_pending_irqs, -1, 0) == -1) {
		input_err(true, &info->client->dev, "%s: pending irq -1\n", __func__);
		return -EINVAL;
	}

	if (atomic_cmpxchg(&info->secure_pending_irqs, 1, 0) == 1) {
		val = 1;
		input_err(true, &info->client->dev, "%s: pending irq is %d\n",
				__func__, atomic_read(&info->secure_pending_irqs));
	}

	complete(&info->secure_interrupt);

	return snprintf(buf, PAGE_SIZE, "%u", val);
}

static DEVICE_ATTR(secure_touch_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
		secure_touch_enable_show, secure_touch_enable_store);
static DEVICE_ATTR(secure_touch, S_IRUGO, secure_touch_show, NULL);

static struct attribute *secure_attr[] = {
	&dev_attr_secure_touch_enable.attr,
	&dev_attr_secure_touch.attr,
	NULL,
};

static struct attribute_group secure_attr_group = {
	.attrs = secure_attr,
};

static int secure_touch_init(struct mms_ts_info *info)
{
	input_info(true, &info->client->dev, "%s\n", __func__);

	if (sysfs_create_group(&info->input_dev->dev.kobj, &secure_attr_group) < 0) {
		input_err(true, &info->client->dev, "%s: do not make secure group\n", __func__);
		return -ENODEV;
	}

	init_completion(&info->secure_interrupt);
	init_completion(&info->secure_powerdown);

	return 0;
}

static void secure_touch_stop(struct mms_ts_info *info, bool stop)
{
	if (atomic_read(&info->secure_enabled)) {
		atomic_set(&info->secure_pending_irqs, -1);

		sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");

		if (stop)
			wait_for_completion_interruptible(&info->secure_powerdown);

		input_info(true, &info->client->dev, "%s: %d\n", __func__, stop);
	}
}
#endif

/**
 * Reboot chip
 *
 * Caution : IRQ must be disabled before mms_power_reboot and enabled after mms_power_reboot.
 */
void mms_power_reboot(struct mms_ts_info *info)
{
	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return;
	}
#endif
	mutex_lock(&info->modechange);

	mms_power_control(info, 0);
	mms_power_control(info, 1);

	mutex_unlock(&info->modechange);

	input_info(true, &info->client->dev, "%s [DONE]\n", __func__);
}

/** reinit **/
void mms_reset_work(struct work_struct *work)
{
	struct mms_ts_info *info = container_of(work, struct mms_ts_info,
			reset_work.work);
	int ret;

	input_info(true, &info->client->dev, "%s [START]\n", __func__);
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return;
	}
#endif

	if (info->reset_is_on_going) {
		input_err(true, &info->client->dev, "%s: reset is ongoing\n", __func__);
		return;
	}

	mutex_lock(&info->modechange);
	info->reset_is_on_going = true;

	mms_disable(info);
	ret = mms_enable(info);
	if (ret) {
		input_err(true, &info->client->dev, "%s: failed to reset\n", __func__);
		info->reset_is_on_going = false;
		cancel_delayed_work(&info->reset_work);
		schedule_delayed_work(&info->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
		mutex_unlock(&info->modechange);
		return;
	}

	if (info->input_dev->disabled) {
		if (info->prox_power_off) {
			input_report_key(info->input_dev, KEY_INT_CANCEL, 1);
			input_sync(info->input_dev);
			input_report_key(info->input_dev, KEY_INT_CANCEL, 0);
			input_sync(info->input_dev);
		}

		if (info->lowpower_mode || ((info->ed_enable || info->pocket_enable) && info->dtdata->support_protos) ||
			info->fod_lp_mode) {
			ret = mms_lowpower_mode(info, TO_LOWPOWER_MODE);
			if (ret < 0) {
				input_err(true, &info->client->dev, "%s: failed to reset\n", __func__);
				info->reset_is_on_going = false;
				cancel_delayed_work(&info->reset_work);
				schedule_delayed_work(&info->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
				mutex_unlock(&info->modechange);
				return;
			}
			mms_set_aod_rect(info);
		} else {
			mms_disable(info);
		}

		info->noise_mode = 0;
		info->wet_mode = 0;
	}
	info->reset_is_on_going = false;
	mutex_unlock(&info->modechange);
	input_info(true, &info->client->dev, "%s [DONE]\n", __func__);
}
/**
 * I2C Read
 */
int mms_i2c_read(struct mms_ts_info *info, char *write_buf, unsigned int write_len,
				char *read_buf, unsigned int read_len)
{
	int retry = I2C_RETRY_COUNT;
	int res;

	struct i2c_msg msg[] = {
		{
			.addr = info->client->addr,
			.flags = 0,
			.buf = write_buf,
			.len = write_len,
		}, {
			.addr = info->client->addr,
			.flags = I2C_M_RD,
			.buf = read_buf,
			.len = read_len,
		},
	};

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &info->client->dev,
			"%s TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif

	if (info->power_status == STATE_POWER_OFF) {
		input_err(true, &info->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto ERROR;
	}

	while (retry--) {
		res = i2c_transfer(info->client->adapter, msg, ARRAY_SIZE(msg));

		if (res == ARRAY_SIZE(msg)) {
			goto DONE;
		} else if (res < 0) {
			input_err(true, &info->client->dev,
				"%s [ERROR] i2c_transfer - errno[%d]\n", __func__, res);
			info->comm_err_count++;
		} else if (res != ARRAY_SIZE(msg)) {
			input_err(true, &info->client->dev,
				"%s [ERROR] i2c_transfer - result[%d]\n",
				__func__, res);
			info->comm_err_count++;
		} else {
			input_err(true, &info->client->dev,
				"%s [ERROR] unknown error [%d]\n", __func__, res);
			info->comm_err_count++;
		}
	}

	goto ERROR_REBOOT;

ERROR_REBOOT:
	if (info->probe_done && !info->reset_is_on_going)
		schedule_delayed_work(&info->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
ERROR:
	return 1;

DONE:
	return 0;
}

/**
 * I2C Write
 */
int mms_i2c_write(struct mms_ts_info *info, char *write_buf, unsigned int write_len)
{
	int retry = I2C_RETRY_COUNT;
	int res;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &info->client->dev,
			"%s TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif

	if (info->power_status == STATE_POWER_OFF) {
		input_err(true, &info->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto ERROR;
	}

	while (retry--) {
		res = i2c_master_send(info->client, write_buf, write_len);

		if (res == write_len) {
			goto DONE;
		} else if (res < 0) {
			input_err(true, &info->client->dev,
				"%s [ERROR] i2c_master_send - errno [%d]\n", __func__, res);
			info->comm_err_count++;
		} else if (res != write_len) {
			input_err(true, &info->client->dev,
				"%s [ERROR] length mismatch - write[%d] result[%d]\n",
				__func__, write_len, res);
			info->comm_err_count++;
		} else {
			input_err(true, &info->client->dev,
				"%s [ERROR] unknown error [%d]\n", __func__, res);
			info->comm_err_count++;
		}
	}

	goto ERROR_REBOOT;

ERROR_REBOOT:
	if (info->probe_done && !info->reset_is_on_going)	
		schedule_delayed_work(&info->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
ERROR:
	return 1;

DONE:
	return 0;
}

int mms_reinit(struct mms_ts_info *info)
{
	u8 wbuf[4];
	int ret = 0;
	u8 mode = G_NONE;

	input_info(true, &info->client->dev, "%s: start reinit\n", __func__);

	if (info->disable_esd == true) {
		ret = mms_disable_esd_alert(info);
		if (ret) {
			input_err(true, &info->client->dev, "%s: failed to disable_esd_alert\n", __func__);
			return -EINVAL;
		}
	}

#ifdef CONFIG_VBUS_NOTIFIER
	if (info->ta_stsatus) {
		ret = mms_charger_attached(info, true);
		if (ret) {
			input_err(true, &info->client->dev, "%s: failed to mms_charger_attached\n", __func__);
			return -EINVAL;
		}
	}
#endif
#ifdef COVER_MODE
	if (info->cover_mode) {
		ret = set_cover_type(info);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: failed to set cover_mode\n", __func__);
			return -EINVAL;
		}
	}
#endif

	if (info->ed_enable) {
		wbuf[0] = MIP_R0_CTRL;
		wbuf[1] = MIP_R1_CTRL_PROXIMITY;
		wbuf[2] = 1;
	
		input_info(true, &info->client->dev, "%s: set ed_enable\n", __func__);

		ret = mms_i2c_write(info, wbuf, 3);
		if (ret) {
			input_err(true, &info->client->dev, "%s: failed to set ed_enable\n", __func__);
			return -EINVAL;
		}
	}

	if (info->pocket_enable) {
		wbuf[0] = MIP_R0_CTRL;
		wbuf[1] = MIP_R1_CTRL_POCKET_MODE;
		wbuf[2] = 1;

		ret = mms_i2c_write(info, wbuf, 3);
		if (ret) {
			input_err(true, &info->client->dev, "%s: failed to set pocket mode enable\n", __func__);
			return -EINVAL;
		}
	}

#ifdef GLOVE_MODE
	if (info->glove_mode) {
		wbuf[0] = MIP_R0_CTRL;
		wbuf[1] = MIP_R1_CTRL_GLOVE_MODE;
		wbuf[2] = 1;
	
		input_info(true, &info->client->dev, "%s: set glove mode/n", __func__);

		ret = mms_i2c_write(info, wbuf, 3);
		if (ret) {
			input_err(true, &info->client->dev, "%s: failed to set glove mode\n", __func__);
			return -EINVAL;
		}
	}
#endif

	mode = mode | G_SET_EDGE_HANDLER;
	set_grip_data_to_ic(info, mode);

	ret = mms_set_fod_rect(info);
	if (ret < 0) {
		input_info(true, &info->client->dev, "%s: failed/n", __func__);
		return -EINVAL;
	}

	input_info(true, &info->client->dev, "%s: done reinit\n", __func__);

	return ret;
}

/**
 * Enable device
 */
int mms_enable(struct mms_ts_info *info)
{
	int ret = 0;

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	if (info->power_status == STATE_POWER_ON) {
		input_err(true, &info->client->dev,
			"%s : already power on\n", __func__);
		return 0;
	}

	mutex_lock(&info->lock);

	if (info->probe_done)
		mms_power_control(info, 1);

	info->power_status = STATE_POWER_ON;

	enable_irq(info->client->irq);

	ret = mms_reinit(info);
	if (ret < 0)
		input_info(true, &info->client->dev, "%s: failed\n", __func__);

	mutex_unlock(&info->lock);

	input_info(true, &info->client->dev, "%s [DONE]\n", __func__);
	return ret;
}

/**
 * Disable device
 */
int mms_disable(struct mms_ts_info *info)
{
	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	if (info->power_status == STATE_POWER_OFF) {
		input_err(true, &info->client->dev,
			"%s : already power off\n", __func__);
		return 0;
	}

	mutex_lock(&info->lock);

	disable_irq(info->client->irq);

	info->power_status = STATE_POWER_OFF;

	mms_clear_input(info);
	mms_power_control(info, 0);

	mutex_unlock(&info->lock);

	input_info(true, &info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

#if MMS_USE_INPUT_OPEN_CLOSE
/**
 * Open input device
 */
static int mms_input_open(struct input_dev *dev)
{
	struct mms_ts_info *info = input_get_drvdata(dev);

	if (!info->probe_done) {
		input_err(true, &info->client->dev, "%s not finished init\n", __func__);
		return 0;
	}

	if (!info->info_work_done) {
		input_err(true, &info->client->dev, "%s not finished info work\n", __func__);
		return 0;
	}

	mutex_lock(&info->modechange);

	input_info(true, &info->client->dev, "%s %s\n",
			__func__, info->lowpower_mode ? "exit LPM mode" : "");

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
		input_err(true, &info->client->dev, "%s TUI cancel event call!\n", __func__);
		msleep(100);
		tui_force_close(1);
		msleep(200);
		if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
			input_err(true, &info->client->dev, "%s TUI flag force clear!\n",	__func__);
			trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
			trustedui_set_mode(TRUSTEDUI_MODE_OFF);
		}
	}
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	secure_touch_stop(info, 0);
#endif
	info->prox_power_off = 0;

	/* Clear Ear Detection event*/
 	if (info->input_dev_proximity) {
 		input_report_abs(info->input_dev_proximity, ABS_MT_CUSTOM, 0);
 		input_sync(info->input_dev_proximity);
 	}

	if (info->power_status == STATE_LPM) {
		mms_lowpower_mode(info, TO_TOUCH_MODE);
#ifdef CONFIG_VBUS_NOTIFIER
		if (info->ta_stsatus)
			mms_charger_attached(info, true);
#endif
	} else {
		mms_enable(info);
	}
	mutex_unlock(&info->modechange);

	cancel_delayed_work(&info->work_print_info);
	info->print_info_cnt_open = 0;
	info->print_info_cnt_release = 0;
	schedule_work(&info->work_print_info.work);

	return 0;
}

/**
 * Close input device
 */
static void mms_input_close(struct input_dev *dev)
{
	struct mms_ts_info *info = input_get_drvdata(dev);

	if (!info->info_work_done) {
		input_err(true, &info->client->dev, "%s not finished info work\n", __func__);
		return;
	}

	mutex_lock(&info->modechange);

	input_info(true, &info->client->dev, "%s %s\n",
			__func__, info->lowpower_mode ? "enter LPM mode" : "");

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
		input_err(true, &info->client->dev, "%s TUI cancel event call!\n", __func__);
		msleep(100);
		tui_force_close(1);
		msleep(200);
		if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
			input_err(true, &info->client->dev, "%s TUI flag force clear!\n", __func__);
			trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
			trustedui_set_mode(TRUSTEDUI_MODE_OFF);
		}
	}
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	secure_touch_stop(info, 1);
#endif

	cancel_delayed_work(&info->reset_work);

	if (info->prox_power_off) {
		input_report_key(info->input_dev, KEY_INT_CANCEL, 1);
		input_sync(info->input_dev);
		input_report_key(info->input_dev, KEY_INT_CANCEL, 0);
		input_sync(info->input_dev);
	}

	if (info->lowpower_mode || ((info->ed_enable || info->pocket_enable) && info->dtdata->support_protos) ||
		info->fod_lp_mode)
		mms_lowpower_mode(info, TO_LOWPOWER_MODE);
	else
		mms_disable(info);

	info->noise_mode = 0;
	info->wet_mode = 0;

	mutex_unlock(&info->modechange);
	cancel_delayed_work(&info->work_print_info);
}
#endif

/**
 * Get ready status
 */
int mms_get_ready_status(struct mms_ts_info *info)
{
	u8 wbuf[16];
	u8 rbuf[16];
	int ret = 0;

	input_dbg(false, &info->client->dev, "%s [START]\n", __func__);

	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_READY_STATUS;
	if (mms_i2c_read(info, wbuf, 2, rbuf, 1)) {
		input_err(true, &info->client->dev, "%s [ERROR] mms_i2c_read\n", __func__);
		goto ERROR;
	}
	ret = rbuf[0];

	//check status
	if ((ret == MIP_CTRL_STATUS_NONE) || (ret == MIP_CTRL_STATUS_LOG)
		|| (ret == MIP_CTRL_STATUS_READY)) {
		input_dbg(false, &info->client->dev, "%s - status [0x%02X]\n", __func__, ret);
	} else{
		input_err(true, &info->client->dev,
			"%s [ERROR] Unknown status [0x%02X]\n", __func__, ret);
		goto ERROR;
	}

	if (ret == MIP_CTRL_STATUS_LOG) {
		//skip log event
		wbuf[0] = MIP_R0_LOG;
		wbuf[1] = MIP_R1_LOG_TRIGGER;
		wbuf[2] = 0;
		if (mms_i2c_write(info, wbuf, 3))
			input_err(true, &info->client->dev, "%s [ERROR] mms_i2c_write\n", __func__);
	}

	input_dbg(false, &info->client->dev, "%s [DONE]\n", __func__);
	return ret;

ERROR:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/**
 * Read chip firmware version
 */
int mms_get_fw_version(struct mms_ts_info *info, u8 *ver_buf)
{
	u8 rbuf[8];
	u8 wbuf[2];
	int i;

	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_VERSION_BOOT;
	if (mms_i2c_read(info, wbuf, 2, rbuf, 8)) {
		goto ERROR;
	};

	for (i = 0; i < (MMS_FW_MAX_SECT_NUM * 2); i++) {
		ver_buf[i] = rbuf[i];
	}

	info->fw_model_ver_ic = (ver_buf[4] << 8 | ver_buf[5]);
	info->fw_ver_ic = (ver_buf[6] << 8 | ver_buf[7]);

	input_info(true, &info->client->dev,
			"%s: boot:%x.%x core:%x.%x %x.%x version:%x.%x\n",
			__func__, ver_buf[0], ver_buf[1], ver_buf[2], ver_buf[3], ver_buf[4],
			ver_buf[5], ver_buf[6], ver_buf[7]);

	return 0;

ERROR:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/**
 * Read chip firmware version for u16
 */
int mms_get_fw_version_u16(struct mms_ts_info *info, u16 *ver_buf_u16)
{
	u8 rbuf[8];
	int i;

	if (mms_get_fw_version(info, rbuf))
		goto ERROR;

	for (i = 0; i < MMS_FW_MAX_SECT_NUM; i++)
		ver_buf_u16[i] = (rbuf[i * 2 + 1] << 8) | rbuf[i * 2];

	return 0;

ERROR:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/**
 * Disable ESD alert
 */
int mms_disable_esd_alert(struct mms_ts_info *info)
{
	u8 wbuf[4];
	u8 rbuf[4];

	input_info(true, &info->client->dev, "%s [START]\n", __func__);

	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_DISABLE_ESD_ALERT;
	wbuf[2] = 1;
	if (mms_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] mms_i2c_write\n", __func__);
		goto ERROR;
	}

	if (mms_i2c_read(info, wbuf, 2, rbuf, 1)) {
		input_err(true, &info->client->dev, "%s [ERROR] mms_i2c_read\n", __func__);
		goto ERROR;
	}

	if (rbuf[0] != 1) {
		input_info(true, &info->client->dev, "%s [ERROR] failed\n", __func__);
		goto ERROR;
	}

	input_info(true, &info->client->dev, "%s [DONE]\n", __func__);
	return 0;

ERROR:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/**
 * Alert event handler - ESD
 */
static int mms_alert_handler_esd(struct mms_ts_info *info, u8 *rbuf)
{
	u8 frame_cnt = rbuf[1];

	input_info(true, &info->client->dev, "%s [START] - frame_cnt[%d]\n",
		__func__, frame_cnt);

	if (frame_cnt == 0) {
		//sensor crack, not ESD
		info->esd_cnt++;
		input_info(true, &info->client->dev, "%s - esd_cnt[%d]\n",
			__func__, info->esd_cnt);

		if (info->disable_esd == true) {
			mms_disable_esd_alert(info);
		} else if (info->esd_cnt > ESD_COUNT_FOR_DISABLE) {
			//Disable ESD alert
			if (mms_disable_esd_alert(info))
				input_err(true, &info->client->dev,
					"%s - fail to disable esd alert\n", __func__);
			else
				info->disable_esd = true;
		} else {
			//Reset chip
			if (!info->reset_is_on_going)
				schedule_work(&info->reset_work.work);
		}
	} else {
		//ESD detected
		//Reset chip
		if (!info->reset_is_on_going)
			schedule_work(&info->reset_work.work);
		info->esd_cnt = 0;
	}

	input_info(true, &info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

/*
 * Alert event handler - SRAM failure
 */
static int mms_alert_handler_sram(struct mms_ts_info *info, u8 *data)
{
	int i;

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	info->sram_addr_num = (unsigned int) (data[0] | (data[1] << 8));
	input_info(true, &info->client->dev, "%s - sram_addr_num [%d]\n", __func__, info->sram_addr_num);

	if (info->sram_addr_num > 8) {
		input_err(true, &info->client->dev, "%s [ERROR] sram_addr_num [%d]\n",
						__func__, info->sram_addr_num);
		goto error;
	}

	for (i = 0; i < info->sram_addr_num; i++) {
		info->sram_addr[i] = data[2 + 4 * i] | (data[2 + 4 * i + 1] << 8) |
							(data[2 + 4 * i + 2] << 16) | (data[2 + 4 * i + 3] << 24);
		input_info(true, &info->client->dev, "%s - sram_addr #%d [0x%08X]\n",
						__func__, i, info->sram_addr[i]);
	}
	for (i = info->sram_addr_num; i < 8; i++) {
		info->sram_addr[i] = 0;
		input_info(true, &info->client->dev, "%s - sram_addr #%d [0x%08X]\n",
						__func__, i, info->sram_addr[i]);
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Alert event handler - Custom
 */
int mms_alert_handler_sponge(struct mms_ts_info *info, u8 *rbuf, u8 size)
{
	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	if (rbuf[1] == 2) {
		if (mms_custom_event_handler(info, rbuf, size)) {
			dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
		}
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

static int mms_alert_handler_pocket_mode_state(struct mms_ts_info *info, u8 data)
{
	input_info(true, &info->client->dev, "%s: pocket event(%d)\n", __func__, data);

	if (data == IN_POCKET || data == OUT_POCKET) {
		input_report_abs(info->input_dev_proximity, ABS_MT_CUSTOM, data);
		input_sync(info->input_dev_proximity);
	}

	return 0;
}

static int mms_alert_handler_proximity_state(struct mms_ts_info *info, u8 data)
{
	
	if (!info->dtdata->support_protos) {
		if (data == 4 || data == 5) {
			input_info(true, &info->client->dev, "%s: not support protos %d\n", __func__, data);
			return 0;
		}
	}

	input_info(true, &info->client->dev, "%s: hover %d\n", __func__, data);
	info->hover_event = data;

	input_report_abs(info->input_dev_proximity, ABS_MT_CUSTOM, data);
	input_sync(info->input_dev_proximity);
	return 0;
}

/*
 * Alert event handler - mode state
 */
#define ENTER_NOISE_MODE	0
#define EXIT_NOISE_MODE		1
#define ENTER_WET_MODE		2
#define EXIT_WET_MODE		3
#define MODE_STATE_VSYNC_ON	4
#define MODE_STATE_VSYNC_OFF	5

static int mms_alert_handler_mode_state(struct mms_ts_info *info, u8 data)
{
	if (data == ENTER_NOISE_MODE) {
		input_info(true, &info->client->dev, "%s: NOISE ON[%d]\n", __func__, data);
		info->noise_mode = 1;
	} else if (data == EXIT_NOISE_MODE) {
		input_info(true, &info->client->dev, "%s: NOISE OFF[%d]\n", __func__, data);
		info->noise_mode = 0;
	} else if (data == ENTER_WET_MODE) {
		input_info(true, &info->client->dev, "%s: WET MODE ON[%d]\n", __func__, data);
		info->wet_mode = 1;
	} else if (data == EXIT_WET_MODE) {
		input_info(true, &info->client->dev, "%s: WET MODE OFF[%d]\n", __func__, data);
		info->wet_mode = 0;
	} else if (data == MODE_STATE_VSYNC_ON) {
		input_info(true, &info->client->dev, "%s: VSYNC ON[%d]\n", __func__, data);
	} else if (data == MODE_STATE_VSYNC_OFF) {
		input_info(true, &info->client->dev, "%s: VSYNC OFF[%d]\n", __func__, data);
	} else {
		input_info(true, &info->client->dev, "%s: NOT DEFINED[%d]\n", __func__, data);
		return 1;
	}

	return 0;
}

#ifdef CONFIG_VBUS_NOTIFIER
int mms_charger_attached(struct mms_ts_info *info, bool status)
{
	u8 wbuf[4];
	int ret = 0;

	input_info(true, &info->client->dev, "%s [START] %s\n", __func__, status ? "connected" : "disconnected");

	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_CHARGER_MODE;
	wbuf[2] = status;

	if ((status == 0) || (status == 1)) {
		ret = mms_i2c_write(info, wbuf, 3);
		if (ret)
			input_err(true, &info->client->dev, "%s [ERROR] mms_i2c_write\n", __func__);
		else
			input_info(true, &info->client->dev, "%s - value[%d]\n", __func__, wbuf[2]);
	} else {
		input_err(true, &info->client->dev, "%s [ERROR] Unknown value[%d]\n", __func__, status);
	}
	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return ret;
}
#endif

/**
 * Interrupt handler
 */
static irqreturn_t mms_interrupt(int irq, void *dev_id)
{
	struct mms_ts_info *info = dev_id;
	struct i2c_client *client = info->client;
	unsigned int packet_size = info->event_size * MAX_FINGER_NUM + 1;
	u8 wbuf[8];
	u8 *rbuf;
	unsigned int size = 0;
	u8 category = 0;
	u8 alert_type = 0;
	int ret;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (secure_filter_interrupt(info) == IRQ_HANDLED) {
		wait_for_completion_interruptible_timeout(&info->secure_interrupt,
				msecs_to_jiffies(5 * MSEC_PER_SEC));

		input_info(true, &info->client->dev,
				"%s: secure interrupt handled\n", __func__);

		return IRQ_HANDLED;
	}
#endif

	rbuf = kzalloc(packet_size, GFP_KERNEL);
	if (!rbuf) {
		input_err(true, &info->client->dev,
				"%s: Failed to kmalloc\n", __func__);
		return IRQ_HANDLED;
	}

	if (info->power_status == STATE_LPM) {
		pm_wakeup_event(info->input_dev->dev.parent, 1000);

		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(&info->resume_done, msecs_to_jiffies(500));
		if (ret == 0) {
			input_err(true, &info->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			kfree(rbuf);
			return IRQ_HANDLED;
		} else if (ret < 0) {
			input_err(true, &info->client->dev, "%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			kfree(rbuf);
			return IRQ_HANDLED;
		}

		input_info(true, &info->client->dev, "%s: run LPM interrupt handler, %d\n", __func__, ret);
		/* run lpm interrupt handler */
	}

	input_dbg(false, &client->dev, "%s [START]\n", __func__);

	//Read packet info
	wbuf[0] = MIP_R0_EVENT;
	wbuf[1] = MIP_R1_EVENT_PACKET_INFO;
	if (mms_i2c_read(info, wbuf, 2, rbuf, 1)) {
		input_err(true, &client->dev, "%s [ERROR] Read packet info\n", __func__);
		goto ERROR;
	}

	input_dbg(false, &client->dev, "%s - info [0x%02X]\n", __func__, rbuf[0]);

	//Check event
	category = ((rbuf[0] >> 7) & 0x1);
	if (info->event_size_type == 1 && category == 0)
		size = (rbuf[0] & 0x7F) * info->event_size;
	else
		size = (rbuf[0] & 0x7F);

	input_dbg(false, &client->dev, "%s - packet info : size[%d] category[%d]\n", __func__, size, category);

	if ((size == 0) || (size > packet_size)) {
		input_err(true, &client->dev, "%s [ERROR] packet size = %d\n", __func__, size);

		size = packet_size;

		wbuf[0] = MIP_R0_EVENT;
		wbuf[1] = MIP_R1_EVENT_PACKET_DATA;
		mms_i2c_read(info, wbuf, 2, rbuf, size);

		mms_clear_input(info);
		goto ERROR;
	}

	//Read packet data
	wbuf[0] = MIP_R0_EVENT;
	wbuf[1] = MIP_R1_EVENT_PACKET_DATA;
	if (mms_i2c_read(info, wbuf, 2, rbuf, size)) {
		input_err(true, &client->dev, "%s [ERROR] Read packet data\n", __func__);
		goto ERROR;
	}

	if (category == 0) {
		//Touch event
		info->esd_cnt = 0;
		mms_input_event_handler(info, size, rbuf);
	} else {
		//Alert event
		alert_type = rbuf[0];

		input_dbg(true, &client->dev, "%s - alert type [%d]\n", __func__, alert_type);

		if (alert_type == MIP_ALERT_ESD) {
			//ESD detection
			if (mms_alert_handler_esd(info, rbuf))
				goto ERROR;
		} else if (alert_type == MIP_ALERT_SPONGE_GESTURE) {
			if (mms_alert_handler_sponge(info, rbuf, size))
				goto ERROR;
		} else if (alert_type == MIP_ALERT_SRAM_FAILURE) {
			//SRAM failure
			if (mms_alert_handler_sram(info, &rbuf[1]))
				goto ERROR;
		} else if (alert_type == MIP_ALERT_MODE_STATE) {
			if (mms_alert_handler_mode_state(info, rbuf[1]))
				goto ERROR;
		} else if (alert_type == MIP_ALERT_POCKET_MODE_STATE) {
			if (mms_alert_handler_pocket_mode_state(info, rbuf[1]))
				goto ERROR;
		} else if (alert_type == MIP_ALERT_PROXIMITY_STATE) {
			if (mms_alert_handler_proximity_state(info, rbuf[1]))
				goto ERROR;
		} else {
			input_err(true, &client->dev, "%s [ERROR] Unknown alert type [%d]\n",
				__func__, alert_type);
			goto ERROR;
		}
	}

	input_dbg(false, &client->dev, "%s [DONE]\n", __func__);
	kfree(rbuf);
	return IRQ_HANDLED;

ERROR:
	input_err(true, &client->dev, "%s [ERROR]\n", __func__);
	if (RESET_ON_EVENT_ERROR) {
		input_info(true, &client->dev, "%s - Reset on error\n", __func__);
		schedule_delayed_work(&info->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
	}
	kfree(rbuf);
	return IRQ_HANDLED;
}

/**
 * Update firmware from kernel built-in binary
 */
int mms_fw_update_from_kernel(struct mms_ts_info *info, bool force, bool on_probe)
{
	const char *fw_name = info->dtdata->fw_name;
	const struct firmware *fw;
	int retires = 3;
	int ret;
	u8 rbuf[8];

	if (info->dtdata->bringup == 1) {
		input_err(true, &info->client->dev, "%s: bringup. do not update\n", __func__);
		return 0; 
	}

	if (!fw_name) {
		input_err(true, &info->client->dev, "%s fw_name does not exist\n", __func__);
		return -ENOENT;
	}

	input_info(true, &info->client->dev, "%s [START]\n", __func__);

	/* Get firmware */
	if (request_firmware(&fw, fw_name, &info->client->dev) != 0) {
		input_err(true, &info->client->dev, "%s: firmware is not available\n", __func__);
		return -EIO;
	}

	input_info(true, &info->client->dev, "%s: request firmware done! size = %d\n", __func__, (int)fw->size);

	mutex_lock(&info->lock);
	disable_irq(info->client->irq);
	mms_clear_input(info);

	mms_get_fw_version(info, rbuf);
	/* Update fw */
	do {
		ret = mip4_ts_flash_fw(info, fw->data, fw->size, force, true, on_probe);
		if (ret >= FW_ERR_NONE)
			break;
	} while (--retires);

	if (!retires) {
		input_err(true, &info->client->dev, "%s mms_flash_fw failed\n", __func__);
		ret = -EIO;
	}

	release_firmware(fw);

	/* Enable IRQ */
	enable_irq(info->client->irq);
	mutex_unlock(&info->lock);

	input_info(true, &info->client->dev, "%s %d\n", __func__, ret);
	return ret;

}

/**
 * Update firmware from external storage
 */
extern int long spu_firmware_signature_verify(const char* fw_name, const u8* fw_data, const long fw_size);
int mms_fw_update_from_storage(struct mms_ts_info *info, bool force, bool signing, const char *file_path)
{
	struct file *fp;
	mm_segment_t old_fs;
	size_t fw_size, nread;
	int ret = 0;
	size_t spu_fw_size;
	size_t spu_ret = 0;

	input_info(true, &info->client->dev, "%s [START]\n", __func__);

	//Disable IRQ
	mutex_lock(&info->lock);
	disable_irq(info->client->irq);
	mms_clear_input(info);

	//Get firmware
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_path, O_RDONLY, 0400);
	if (IS_ERR(fp)) {
		input_err(true, &info->client->dev, "%s [ERROR] file_open - path[%s]\n",
			__func__, file_path);
		ret = FW_ERR_FILE_OPEN;
		set_fs(old_fs);
		enable_irq(info->client->irq);
		mutex_unlock(&info->lock);
		return FW_ERR_FILE_OPEN;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (signing) {
		/* name 3, digest 32, signature 512 */
		spu_fw_size = fw_size;
		fw_size -= SPU_METADATA_SIZE(TSP);
	}

	if (fw_size > 0) {
		unsigned char *fw_data;
		unsigned char *spu_fw_data;

		fw_data = kzalloc(fw_size, GFP_KERNEL);

		if (signing) {
			spu_fw_data = kzalloc(spu_fw_size, GFP_KERNEL);
			nread = vfs_read(fp, (char __user *)spu_fw_data, spu_fw_size, &fp->f_pos);
			input_info(true, &info->client->dev, "%s - path [%s] size [%zu]\n",
					__func__, file_path, spu_fw_size);

			if (nread != spu_fw_size) {
				input_err(true, &info->client->dev, "%s [ERROR] vfs_read - size[%zu] read[%zu]\n",
					__func__, fw_size, nread);
				ret = FW_ERR_FILE_READ;
				kfree(spu_fw_data);
				kfree(fw_data);
				goto ERROR;
			}

			spu_ret = spu_firmware_signature_verify("TSP", spu_fw_data, spu_fw_size);
			if (spu_ret != fw_size) {
				input_err(true, &info->client->dev, "%s: signature verify failed, %zu\n",
						__func__, spu_ret);
				ret = -EINVAL;
				kfree(spu_fw_data);
				kfree(fw_data);
				goto ERROR;
			}

			memcpy(fw_data, spu_fw_data, fw_size);
			kfree(spu_fw_data);
		} else {
			nread = vfs_read(fp, (char __user *)fw_data, fw_size, &fp->f_pos);
			input_info(true, &info->client->dev, "%s - path [%s] size [%zu]\n",
				__func__, file_path, fw_size);

			if (nread != fw_size) {
				input_err(true, &info->client->dev, "%s [ERROR] vfs_read - size[%zu] read[%zu]\n",
					__func__, fw_size, nread);
				ret = FW_ERR_FILE_READ;
				kfree(fw_data);
				goto ERROR;
			}
		}

		ret = mip4_ts_flash_fw(info, fw_data, fw_size, force, true, false);
		kfree(fw_data);
	} else {
		input_err(true, &info->client->dev, "%s [ERROR] fw_size [%zu]\n", __func__, fw_size);
		ret = FW_ERR_FILE_READ;
	}

ERROR:
	filp_close(fp, current->files);

	set_fs(old_fs);

	/* Enable IRQ */
	enable_irq(info->client->irq);
	mutex_unlock(&info->lock);

	input_err(true, &info->client->dev, "%s [DONE]\n", __func__);

	return ret;
}

#if MMS_USE_DEV_MODE
static ssize_t mms_sys_fw_update(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);
	int result = 0;
	u8 data[255];
	int ret = 0;

	memset(info->print_buf, 0, PAGE_SIZE);

	input_info(true, &info->client->dev, "%s [START]\n", __func__);

	ret = mms_fw_update_from_storage(info, true, NORMAL, TSP_PATH_EXTERNAL_FW);

	switch (ret) {
	case FW_ERR_NONE:
		sprintf(data, "F/W update success\n");
		break;
	case FW_ERR_UPTODATE:
		sprintf(data, "F/W is already up-to-date\n");
		break;
	case FW_ERR_DOWNLOAD:
		sprintf(data, "F/W update failed : Download error\n");
		break;
	case FW_ERR_FILE_TYPE:
		sprintf(data, "F/W update failed : File type error\n");
		break;
	case FW_ERR_FILE_OPEN:
		sprintf(data, "F/W update failed : File open error [%s]\n", TSP_PATH_EXTERNAL_FW);
		break;
	case FW_ERR_FILE_READ:
		sprintf(data, "F/W update failed : File read error\n");
		break;
	default:
		sprintf(data, "F/W update failed\n");
		break;
	}

	input_info(true, &info->client->dev, "%s [DONE]\n", __func__);

	strcat(info->print_buf, data);
	result = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return result;
}
static DEVICE_ATTR(fw_update, S_IWUSR | S_IWGRP, mms_sys_fw_update, NULL);

/**
 * Sysfs attr info
 */
static struct attribute *mms_attrs[] = {
	&dev_attr_fw_update.attr,
	NULL,
};

/**
 * Sysfs attr group info
 */
static const struct attribute_group mms_attr_group = {
	.attrs = mms_attrs,
};
#endif

/**
 * Initial config
 */
static int mms_init_config(struct mms_ts_info *info)
{
	u8 wbuf[8];
	u8 rbuf[32];

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	/* read product name */
	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_PRODUCT_NAME;
	mms_i2c_read(info, wbuf, 2, rbuf, 16);
	memcpy(info->product_name, rbuf, 16);
	input_info(true, &info->client->dev, "%s - product_name[%s]\n",
		__func__, info->product_name);

	/* read fw version */
	mms_get_fw_version(info, rbuf);

	info->max_x = info->dtdata->max_x;
	info->max_y = info->dtdata->max_y;
	info->node_x = info->dtdata->node_x;
	info->node_y = info->dtdata->node_y;
	info->node_key = info->dtdata->node_key;

	input_info(true, &info->client->dev, "%s - max_x[%d] max_y[%d]\n",
		__func__, info->max_x, info->max_y);
	input_info(true, &info->client->dev, "%s - node_x[%d] node_y[%d] node_key[%d]\n",
		__func__, info->node_x, info->node_y, info->node_key);

#if MMS_USE_TOUCHKEY
	/* Enable touchkey */
	if (info->node_key > 0)
		info->tkey_enable = true;
#endif

	/* read event format */
#if 0
	memset(rbuf, 0x00, sizeof(rbuf));
	wbuf[0] = MIP_R0_EVENT;
	wbuf[1] = MIP_R1_EVENT_FORMAT;
	ret = mms_i2c_read(info, wbuf, 2, rbuf, 4);
	if (ret == 1) {
		input_info(true, &info->client->dev, "%s - read event format fail\n", __func__);
		info->event_format = info->dtdata->event_format;
		info->event_size = info->dtdata->event_size;
		info->event_size_type = info->dtdata->event_size_type;
	} else {
		info->event_format = (int)(rbuf[0] | rbuf[1] << 8);
		info->event_size = rbuf[2];
		info->event_size_type = rbuf[3] & 0x1;
	}
#endif

	info->event_format = info->dtdata->event_format;
	info->event_size = info->dtdata->event_size;
	info->event_size_type = info->dtdata->event_size_type;

	input_info(true, &info->client->dev, "%s event_format[%d] event_size[%d] event_size_type[%d]\n",
				__func__, info->event_format, info->event_size, info->event_size_type);

	/* sponge fod info */
	info->fod_tx = info->dtdata->fod_tx;
	info->fod_rx = info->dtdata->fod_rx;
	info->fod_vi_size = info->dtdata->fod_vi_size;

	input_info(true, &info->client->dev, "%s fod_tx[%d] fod_rx[%d] fod_vi_size[%d]\n",
				__func__, info->fod_tx, info->fod_rx, info->fod_vi_size);

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

static void mms_run_rawdata(struct mms_ts_info *info, bool on_probe)
{
	info->tsp_dump_lock = 1;
	input_raw_data_clear();
	input_raw_info(true, &info->client->dev, "%s: start ##\n", __func__);

	input_raw_info(true, &info->client->dev, "%s - max_x[%d] max_y[%d]\n",
		__func__, info->max_x, info->max_y);
	input_raw_info(true, &info->client->dev, "%s - node_x[%d] node_y[%d] node_key[%d]\n",
		__func__, info->node_x, info->node_y, info->node_key);
	input_raw_info(true, &info->client->dev, "%s event_format[%d] event_size[%d]\n",
				__func__, info->event_format, info->event_size);
	input_raw_info(true, &info->client->dev, "%s fod_tx[%d] fod_rx[%d] fod_vi_size[%d]\n",
				__func__, info->fod_tx, info->fod_rx, info->fod_vi_size);

	if (!on_probe) {
		if (mms_get_image(info, MIP_IMG_TYPE_INTENSITY)) {
			input_err(true, &info->client->dev, "%s intensity error\n", __func__);
			goto out;
		}
	}

	if (mms_run_test(info, MIP_TEST_TYPE_CM)) {
		input_err(true, &info->client->dev, "%s cm error\n", __func__);
		goto out;
	}
	minority_report_sync_latest_value(info);

	if (mms_run_test(info, MIP_TEST_TYPE_CP)) {
		input_err(true, &info->client->dev, "%s cp error\n", __func__);
		goto out;
	}

	if (mms_run_test(info, MIP_TEST_TYPE_CP_SHORT)) {
		input_err(true, &info->client->dev, "%s cp short error\n", __func__);
		goto out;
	}

	if (mms_run_test(info, MIP_TEST_TYPE_CP_LPM)) {
		input_err(true, &info->client->dev, "%s cp lpm error\n", __func__);
		goto out;
	}

	if (on_probe) {
		if (mms_run_test(info, MIP_TEST_TYPE_CM_ABS)) {
			input_err(true, &info->client->dev, "%s cm abs error\n", __func__);
			goto out;
		}

		if (mms_run_test(info, MIP_TEST_TYPE_CM_JITTER)) {
			input_err(true, &info->client->dev, "%s cm_jitter error\n", __func__);
			goto out;
		}
	}

out:
	input_raw_info(true, &info->client->dev, "%s: done ##\n", __func__);
	info->tsp_dump_lock = 0;
}

static void mms_read_info_work(struct work_struct *work)
{
	struct mms_ts_info *info = container_of(work, struct mms_ts_info, 
			work_read_info.work);

	input_log_fix();
	mms_run_rawdata(info, true);
	info->info_work_done = true;
}
static void mms_set_input_prop_proximity(struct mms_ts_info *info, struct input_dev *dev)
{
	static char mms_phys[64] = { 0 };

	snprintf(mms_phys, sizeof(mms_phys), "%s/input1", dev->name);
	dev->phys = mms_phys;
	dev->id.bustype = BUS_I2C;
	dev->dev.parent = &info->client->dev;

	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_SW, dev->evbit);

	set_bit(INPUT_PROP_DIRECT, dev->propbit);

	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);
	input_set_drvdata(dev, info);
}

static void mms_ts_print_info_work(struct work_struct *work)
{
	struct mms_ts_info *info = container_of(work, struct mms_ts_info, work_print_info.work);

#ifdef TCLM_CONCEPT
	input_info(true, &info->client->dev, "mode:%04X, tc:%d, noise:%x,%x, wet:%d wc:%x, lp:%x D%05X fn:%04X/%04X // v:%02X%02X cal:%02X(%02X) C%02XT%04X.%4s%s Cal_flag:%s,%d // %d %d\n",
		0, info->touch_count, info->noise_mode, 0, info->wet_mode,
		0/*wireless charger*/, info->lowpower_mode/*lp*/, info->defect_probability, 0/*touch function*/, info->power_status,
		info->fw_ver_ic >> 8, info->fw_ver_ic & 0xFF,
		0, 0, 0, 0, " ", " ", " ", 0/*TCLM*/,
		info->print_info_cnt_open, info->print_info_cnt_release);
#else
	input_info(true, &info->client->dev, "mode:%04X, tc:%d, noise:%x,%x, wet:%d wc:%x, lp:%x D%05X fn:%04X/%04X // v:%02X%02X cal:NOCAL // %d %d\n",
		0, info->touch_count, info->noise_mode, 0, info->wet_mode,
		0/*wireless charger*/, info->lowpower_mode/*lp*/, info->defect_probability, 0/*touch function*/, info->power_status,
		info->fw_ver_ic >> 8, info->fw_ver_ic & 0xFF,
		info->print_info_cnt_open, info->print_info_cnt_release);

#endif
	info->print_info_cnt_open++;
	if (info->touch_count == 0)
		info->print_info_cnt_release++;

	schedule_delayed_work(&info->work_print_info, msecs_to_jiffies(30 * 1000));
}

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
static void sec_ts_check_rawdata(struct work_struct *work)
{
	struct mms_ts_info *info = container_of(work, struct mms_ts_info, 
			ghost_check.work);

	if (info->tsp_dump_lock == 1) {
		input_err(true, &info->client->dev, "%s: ignored ## already checking..\n", __func__);
		return;
	}
	if (info->power_status == STATE_POWER_OFF) {
		input_err(true, &info->client->dev, "%s: ignored ## IC is power off\n", __func__);
		return;
	}

	mms_run_rawdata(info, false);
}

static void dump_tsp_log(void)
{
	pr_info("%s: %s %s: start\n", MMS_DEVICE_NAME, SECLOG, __func__);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		pr_err("%s: %s %s: ignored ## lpm charging Mode!!\n", MMS_DEVICE_NAME, SECLOG, __func__);
		return;
	}
#endif
	if (p_ghost_check == NULL) {
		pr_err("%s: %s %s: ignored ## tsp probe fail!!\n", MMS_DEVICE_NAME, SECLOG, __func__);
		return;
	}
	schedule_delayed_work(p_ghost_check, msecs_to_jiffies(100));
}
#endif

/**
 * Initialize driver
 */
static int mms_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct mms_ts_info *info;
	int ret = 0;

	input_err(true, &client->dev, "%s\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev,
			"%s EIO err!\n", __func__);
		return -EIO;
	}

	info = kzalloc(sizeof(struct mms_ts_info), GFP_KERNEL);
	if (!info) {
		goto err_mem_alloc;
	}

	info->client = client;

	if (client->dev.of_node) {
		info->dtdata  =
			devm_kzalloc(&client->dev,
				sizeof(struct mms_dt_data), GFP_KERNEL);
		if (!info->dtdata) {
			input_err(true, &client->dev,
				"%s: Failed to parse dt\n", __func__);
			goto err_devm_alloc;
		}
		mms_parse_dt(&client->dev, info);
	} else {
		input_err(true, &client->dev, "%s: No platform data found\n", __func__);
		ret = -EINVAL;
		goto err_platform_data;
	}

	info->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(info->pinctrl)) {
		input_err(true, &client->dev, "%s: Failed to get pinctrl data\n", __func__);
		ret = PTR_ERR(info->pinctrl);
		goto err_platform_data;
	}

	info->touch_count = 0;

	INIT_DELAYED_WORK(&info->reset_work, mms_reset_work);
	INIT_DELAYED_WORK(&info->work_print_info, mms_ts_print_info_work);
	INIT_DELAYED_WORK(&info->work_read_info, mms_read_info_work);

	i2c_set_clientdata(client, info);

	info->input_dev = input_allocate_device();
	if (!info->input_dev) {
		input_err(true, &client->dev, "%s: allocate device err!\n", __func__);
		ret = -ENOMEM;
		goto err_input_alloc;
	}

	if (info->dtdata->support_ear_detect) {
		info->input_dev_proximity = input_allocate_device();
		if (!info->input_dev_proximity) {
			input_err(true, &info->client->dev, "%s: allocate input_dev_proximity err!\n", __func__);
			ret = -ENOMEM;
			goto err_allocate_input_dev_proximity;
		}
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	tui_tsp_info = info;
#endif
#ifdef CONFIG_SAMSUNG_TUI
	tsp_info = info;
#endif

	mutex_init(&info->lock);
	mutex_init(&info->modechange);
	mutex_init(&info->sponge_mutex);

	init_completion(&info->resume_done);
	complete_all(&info->resume_done);
		
#if MMS_USE_NAP_MODE
	wake_lock_init(&mms_wake_lock, WAKE_LOCK_SUSPEND, "mms_wake_lock");
#endif

	mms_power_control(info, 1);
	msleep(1000);

	info->power_status = STATE_POWER_ON;

	ret = mms_fw_update_from_kernel(info, false, true);
	if (ret < 0) {
		input_err(true, &client->dev, "%s [ERROR] mms_fw_update_from_kernel\n", __func__);
		goto err_fw_update;
	}

	mms_init_config(info);

	mms_config_input(info);

#if MMS_USE_INPUT_OPEN_CLOSE
	info->input_dev->open = mms_input_open;
	info->input_dev->close = mms_input_close;
#endif

	if (info->dtdata->support_ear_detect) {
		info->input_dev_proximity->name = "sec_touchproximity";
		mms_set_input_prop_proximity(info, info->input_dev_proximity);
	}

	ret = input_register_device(info->input_dev);
	if (ret) {
		input_err(true, &client->dev, "%s %s: Unable to register input device\n", __func__, info->input_dev->name);
		ret = -EIO;
		goto err_input_register_device;
	}

	if (info->dtdata->support_ear_detect) {
		ret = input_register_device(info->input_dev_proximity);
		if (ret) {
			input_err(true, &info->client->dev, "%s: Unable to register %s input device\n",
							__func__, info->input_dev_proximity->name);
			goto err_input_proximity_register_device;
		}
	}

#ifdef USE_TSP_TA_CALLBACKS
	info->register_cb = mms_register_callback;
	info->callbacks.inform_charger = mms_charger_status_cb;
	if (info->register_cb)
		info->register_cb(&info->callbacks);
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	vbus_notifier_register(&info->vbus_nb, mms_vbus_notification,
				VBUS_NOTIFY_DEV_CHARGER);
#endif

	ret = request_threaded_irq(client->irq, NULL, mms_interrupt,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT, MMS_DEVICE_NAME, info);
	if (ret) {
		input_err(true, &client->dev, "%s [ERROR] request_threaded_irq\n", __func__);
		goto err_request_irq;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	trustedui_set_tsp_irq(client->irq);
	input_err(true, &info->client->dev, "%s[%d] called!\n",
		__func__, cleint->irq);
#endif

#if MMS_USE_DEV_MODE
	if (mms_dev_create(info)) {
		input_err(true, &client->dev, "%s [ERROR] mms_dev_create\n", __func__);
		ret = -EAGAIN;
		goto err_test_dev_create;
	}

	info->class = class_create(THIS_MODULE, MMS_DEVICE_NAME);
	device_create(info->class, NULL, info->mms_dev, NULL, MMS_DEVICE_NAME);
#endif

#if MMS_USE_TEST_MODE
	if (mms_sysfs_create(info)) {
		input_err(true, &client->dev, "%s [ERROR] mms_sysfs_create\n", __func__);
		ret = -EAGAIN;
		goto err_test_sysfs_create;
	}
#endif

	if (mms_sysfs_cmd_create(info)) {
		input_err(true, &client->dev, "%s [ERROR] mms_sysfs_cmd_create\n", __func__);
		ret = -EAGAIN;
		goto err_fac_cmd_create;
	}

#if MMS_USE_DEV_MODE
	if (sysfs_create_group(&client->dev.kobj, &mms_attr_group)) {
		input_err(true, &client->dev, "%s [ERROR] sysfs_create_group\n", __func__);
		ret = -EAGAIN;
		goto err_create_attr_group;
	}

	if (sysfs_create_link(NULL, &client->dev.kobj, MMS_DEVICE_NAME)) {
		input_err(true, &client->dev, "%s [ERROR] sysfs_create_link\n", __func__);
		ret = -EAGAIN;
		goto err_create_dev_link;
	}
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	secure_touch_init(info);
#endif

	schedule_delayed_work(&info->work_read_info, msecs_to_jiffies(5000));

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	dump_callbacks.inform_dump = dump_tsp_log;
	INIT_DELAYED_WORK(&info->ghost_check, sec_ts_check_rawdata);
	p_ghost_check = &info->ghost_check;
#endif

	device_init_wakeup(&client->dev, true);
	info->probe_done = true;
	info->power_status = STATE_POWER_ON;
	input_info(true, &client->dev,
		"MELFAS %s Touchscreen is initialized successfully\n", CHIP_NAME);
	input_log_fix();
	schedule_work(&info->work_print_info.work);
	return 0;

#if MMS_USE_DEV_MODE
err_create_dev_link:
	sysfs_remove_group(&client->dev.kobj, &mms_attr_group);
err_create_attr_group:
#endif
	mms_sysfs_cmd_remove(info);
err_fac_cmd_create:

#if MMS_USE_TEST_MODE
	mms_sysfs_remove(info);
err_test_sysfs_create:
#endif
#if MMS_USE_DEV_MODE
	device_destroy(info->class, info->mms_dev);
	class_destroy(info->class);
err_test_dev_create:
#endif
	free_irq(client->irq, info);
err_request_irq:
	if (info->dtdata->support_ear_detect) {
		input_unregister_device(info->input_dev_proximity);
		info->input_dev_proximity = NULL;
	}
err_input_proximity_register_device:	
	input_unregister_device(info->input_dev);
	info->input_dev = NULL;
err_input_register_device:
err_fw_update:
#if MMS_USE_NAP_MODE
	wake_lock_destroy(&mms_wake_lock);
#endif
	mutex_destroy(&info->lock);
	mutex_destroy(&info->modechange);
	mutex_destroy(&info->sponge_mutex);
	mms_power_control(info, 0);
	if (info->dtdata->support_ear_detect) {
		if (info->input_dev_proximity)
			input_free_device(info->input_dev_proximity);
	}
err_allocate_input_dev_proximity:
	if (info->input_dev)
		input_free_device(info->input_dev);
err_input_alloc:
	cancel_delayed_work_sync(&info->work_read_info);
	cancel_delayed_work_sync(&info->work_print_info);
	cancel_delayed_work_sync(&info->reset_work);
err_platform_data:
err_devm_alloc: 
	kfree(info);
err_mem_alloc:
	pr_err("MELFAS %s Touchscreen initialization failed.\n", CHIP_NAME);
	input_log_fix();	
	return ret;
}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
void mms_trustedui_mode_on(void)
{
	pr_err("%s, release all finger..\n", __func__);
	mms_clear_input(tui_tsp_info);
}
#endif

/**
 * Remove driver
 */
static int mms_remove(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	if (client->irq >= 0)
		free_irq(client->irq, info);

	mms_sysfs_cmd_remove(info);

#if MMS_USE_TEST_MODE
	mms_sysfs_remove(info);
#endif

#if MMS_USE_DEV_MODE
	sysfs_remove_group(&info->client->dev.kobj, &mms_attr_group);
	sysfs_remove_link(NULL, MMS_DEVICE_NAME);
	device_destroy(info->class, info->mms_dev);
	class_destroy(info->class);
#endif
	cancel_delayed_work_sync(&info->work_read_info);
	cancel_delayed_work_sync(&info->work_print_info);
	cancel_delayed_work_sync(&info->reset_work);

#if MMS_USE_NAP_MODE
	wake_lock_destroy(&mms_wake_lock);
#endif
	mutex_destroy(&info->lock);
	mutex_destroy(&info->modechange);
	mutex_destroy(&info->sponge_mutex);

	if (info->dtdata->support_ear_detect) {
		input_mt_destroy_slots(info->input_dev_proximity);
		input_unregister_device(info->input_dev_proximity);
	}

	input_unregister_device(info->input_dev);

	kfree(info->fw_name);
	kfree(info);

	return 0;
}

static void mms_shutdown(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	input_err(true, &info->client->dev, "%s\n", __func__);

	cancel_delayed_work_sync(&info->work_read_info);
	cancel_delayed_work_sync(&info->work_print_info);

	mms_disable(info);
}

#ifdef CONFIG_PM
static int mms_suspend(struct device *dev)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	
	reinit_completion(&info->resume_done);

	return 0;
}

static int mms_resume(struct device *dev)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);

	complete_all(&info->resume_done);

	return 0;
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
	mms_clear_input(tsp_info);

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

static const struct dev_pm_ops mms_dev_pm_ops = {
	.suspend = mms_suspend,
	.resume = mms_resume,
};
#endif

/**
 * Device tree match table
 */
static const struct of_device_id mms_match_table[] = {
	{ .compatible = "melfas,mms_ts",},
	{},
};
MODULE_DEVICE_TABLE(of, mms_match_table);

/**
 * I2C Device ID
 */
static const struct i2c_device_id mms_id[] = {
	{MMS_DEVICE_NAME, 0},
};
MODULE_DEVICE_TABLE(i2c, mms_id);

/**
 * I2C driver info
 */
static struct i2c_driver mms_driver = {
	.id_table	= mms_id,
	.probe = mms_probe,
	.remove = mms_remove,
	.shutdown = mms_shutdown,
	.driver = {
		.name = MMS_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mms_match_table,
#ifdef CONFIG_PM
		.pm = &mms_dev_pm_ops,
#endif
	},
};

/**
 * Init driver
 */
static int __init mms_init(void)
{
	pr_err("%s\n", __func__);
#if defined(CONFIG_SAMSUNG_LPM_MODE)
	if (poweroff_charging) {
		pr_notice("%s : LPM Charging Mode!!\n", __func__);
		return -ENODEV;
	}
#endif

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		pr_err("%s %s: Do not load driver due to : lpm %d\n",
				SECLOG, __func__, lpcharge);
		return -ENODEV;
	}
#endif

	return i2c_add_driver(&mms_driver);
}

/**
 * Exit driver
 */
static void __exit mms_exit(void)
{
	i2c_del_driver(&mms_driver);
}

late_initcall(mms_init);
module_exit(mms_exit);

MODULE_DESCRIPTION("MELFAS MMS400 Touchscreen");
MODULE_VERSION("2014.12.05");
MODULE_AUTHOR("Jee, SangWon <jeesw@melfas.com>");
MODULE_LICENSE("GPL");
