/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
 *
 * File Name		: fts.c
 * Authors		: AMS(Analog Mems Sensor) Team
 * Description	: FTS Capacitive touch screen controller (FingerTipS)
 *
 ********************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
 * PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
 *******************************************************************************/

#include "fts_ts.h"

struct fts_ts_info *g_fts_info;

#ifdef USE_OPEN_CLOSE
static int fts_input_open(struct input_dev *dev);
static void fts_input_close(struct input_dev *dev);
#ifdef USE_OPEN_DWORK
static void fts_open_work(struct work_struct *work);
#endif
#endif

static int fts_stop_device(struct fts_ts_info *info, bool lpmode);
static int fts_start_device(struct fts_ts_info *info);

static void fts_reset(struct fts_ts_info *info, unsigned int ms);
static void fts_reset_work(struct work_struct *work);
static void fts_read_info_work(struct work_struct *work);
int fts_get_hf_data(struct fts_ts_info *info);

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
#include <linux/sec_ts_common.h>
static void tsp_dump(void);
static void dump_tsp_rawdata(struct work_struct *work);
struct delayed_work *p_debug_work;
#endif

#if (!defined(CONFIG_PM)) && !defined(USE_OPEN_CLOSE)
static int fts_suspend(struct i2c_client *client, pm_message_t mesg);
static int fts_resume(struct i2c_client *client);
#endif

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
static irqreturn_t fts_filter_interrupt(struct fts_ts_info *info);

static irqreturn_t fts_interrupt_handler(int irq, void *handle);

static ssize_t fts_secure_touch_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t fts_secure_touch_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t fts_secure_touch_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static struct device_attribute attrs[] = {
	__ATTR(secure_touch_enable, (0664),
			fts_secure_touch_enable_show,
			fts_secure_touch_enable_store),
	__ATTR(secure_touch, (0444),
			fts_secure_touch_show,
			NULL),
};

static ssize_t fts_secure_touch_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d", atomic_read(&info->st_enabled));
}

/*
 * Accept only "0" and "1" valid values.
 * "0" will reset the st_enabled flag, then wake up the reading process.
 * The bus driver is notified via pm_runtime that it is not required to stay
 * awake anymore.
 * It will also make sure the queue of events is emptied in the controller,
 * in case a touch happened in between the secure touch being disabled and
 * the local ISR being ungated.
 * "1" will set the st_enabled flag and clear the st_pending_irqs flag.
 * The bus driver is requested via pm_runtime to stay awake.
 */
static ssize_t fts_secure_touch_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	unsigned long value;
	int err = 0;

	if (count > 2) {
		input_err(true, &info->client->dev,
				"%s: cmd length is over (%s,%d)!!\n",
				__func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	err = kstrtoul(buf, 10, &value);
	if (err != 0) {
		input_err(true, &info->client->dev, "%s: failed to read:%d\n",
				__func__, err);
		return err;
	}

	err = count;

	switch (value) {
	case 0:
		if (atomic_read(&info->st_enabled) == 0) {
			input_err(true, &info->client->dev, "%s: secure_touch is not enabled, pending:%d\n",
					__func__, atomic_read(&info->st_pending_irqs));
			break;
		}

		pm_runtime_put_sync(info->client->adapter->dev.parent);

		atomic_set(&info->st_enabled, 0);

		sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");

		fts_delay(10);

		fts_interrupt_handler(info->client->irq, info);

		complete(&info->st_powerdown);
		complete(&info->st_interrupt);

		input_info(true, &info->client->dev, "%s: secure_touch is disabled\n", __func__);

		break;

	case 1:
		if (info->reset_is_on_going) {
			input_err(true, &info->client->dev, "%s: reset is on goning becuse i2c fail\n",
					__func__);
			return -EBUSY;
		}

		if (atomic_read(&info->st_enabled)) {
			input_err(true, &info->client->dev, "%s: secure_touch is already enabled, pending:%d\n",
					__func__, atomic_read(&info->st_pending_irqs));
			err = -EBUSY;
			break;
		}

		/* synchronize_irq -> disable_irq + enable_irq
		 * concern about timing issue.
		 */
		fts_interrupt_set(info, INT_DISABLE);

		/* Release All Finger */
		fts_release_all_finger(info);

		if (pm_runtime_get_sync(info->client->adapter->dev.parent) < 0) {
			input_err(true, &info->client->dev, "%s: pm_runtime_get failed\n", __func__);
			err = -EIO;
			fts_interrupt_set(info, INT_ENABLE);
			break;
		}

		reinit_completion(&info->st_powerdown);
		reinit_completion(&info->st_interrupt);
		atomic_set(&info->st_enabled, 1);
		atomic_set(&info->st_pending_irqs, 0);

		fts_interrupt_set(info, INT_ENABLE);

		input_info(true, &info->client->dev, "%s: secure_touch is enabled\n", __func__);

		break;

	default:
		input_err(true, &info->client->dev, "%s: unsupported value: %lu\n", __func__, value);
		err = -EINVAL;
		break;
	}

	return err;
}

static ssize_t fts_secure_touch_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&info->st_enabled) == 0) {
		input_err(true, &info->client->dev, "%s: secure_touch is not enabled, st_pending_irqs: %d\n",
				__func__, atomic_read(&info->st_pending_irqs));
		return -EBADF;
	}

	if (atomic_cmpxchg(&info->st_pending_irqs, -1, 0) == -1) {
		input_err(true, &info->client->dev, "%s: st_pending_irqs: %d\n",
				__func__, atomic_read(&info->st_pending_irqs));
		return -EINVAL;
	}

	if (atomic_cmpxchg(&info->st_pending_irqs, 1, 0) == 1) {
		val = 1;
		input_info(true, &info->client->dev, "%s: st_pending_irqs: %d, val: %d\n",
				__func__, atomic_read(&info->st_pending_irqs), val);
	}

	complete(&info->st_interrupt);

	return scnprintf(buf, PAGE_SIZE, "%u", val);
}

static void fts_secure_touch_init(struct fts_ts_info *info)
{
	init_completion(&info->st_powerdown);
	init_completion(&info->st_interrupt);
}

static void fts_secure_touch_stop(struct fts_ts_info *info, int blocking)
{
	if (atomic_read(&info->st_enabled)) {
		atomic_set(&info->st_pending_irqs, -1);
		sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");

		if (blocking)
			wait_for_completion_interruptible(&info->st_powerdown);
	}
}

static irqreturn_t fts_filter_interrupt(struct fts_ts_info *info)
{
	if (atomic_read(&info->st_enabled)) {
		if (atomic_cmpxchg(&info->st_pending_irqs, 0, 1) == 0) {
			sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");
		} else {
			input_info(true, &info->client->dev, "%s: st_pending_irqs: %d\n",
					__func__, atomic_read(&info->st_pending_irqs));
		}
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}
#endif

int fts_write_reg(struct fts_ts_info *info,
		u8 *reg, u16 num_com)
{
	struct i2c_msg xfer_msg[2];
	int ret;
	int retry = FTS_TS_I2C_RETRY_CNT;
	u8 *buff;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: Sensor stopped\n", __func__);
		return -EIO;
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif

	buff = kzalloc(num_com, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;
	memcpy(buff, reg, num_com);

	mutex_lock(&info->i2c_mutex);

	xfer_msg[0].addr = info->client->addr;
	xfer_msg[0].len = num_com;
	xfer_msg[0].flags = 0 | I2C_M_DMA_SAFE;
	xfer_msg[0].buf = buff;

	do {
		ret = i2c_transfer(info->client->adapter, xfer_msg, 1);
		if (ret < 0) {
			info->comm_err_count++;
			input_err(true, &info->client->dev,
					"%s failed(%d). ret:%d, addr:%x, cnt:%d\n",
					__func__, retry, ret, xfer_msg[0].addr, info->comm_err_count);
			usleep_range(10 * 1000, 10 * 1000);
		} else {
			break;
		}
	} while (--retry > 0);

	mutex_unlock(&info->i2c_mutex);

	if (retry == 0) {
		input_err(true, &info->client->dev, "%s: I2C read over retry limit\n", __func__);
		ret = -EIO;

		if (info->debug_string & FTS_DEBUG_SEND_UEVENT)
			sec_cmd_send_event_to_user(&info->sec, NULL, "RESULT=I2C");
#ifdef USE_POR_AFTER_I2C_RETRY
		if (info->probe_done && !info->reset_is_on_going)
			schedule_delayed_work(&info->reset_work, msecs_to_jiffies(10));
#endif
	}

	if (info->debug_string & FTS_DEBUG_PRINT_I2C_WRITE_CMD) {
		int i;

		pr_info("sec_input: i2c_cmd: W: ");
		for (i = 0; i < num_com; i++)
			pr_cont("%02X ", buff[i]);
		pr_cont("\n");

	}

	kfree(buff);

	return ret;
}

int fts_read_reg(struct fts_ts_info *info, u8 *reg, int cnum,
		u8 *buf, int num)
{
	struct i2c_msg xfer_msg[2];
	int ret;
	int retry = FTS_TS_I2C_RETRY_CNT;
	u8 *buff;
	u8 *msg_buff;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: Sensor stopped\n", __func__);
		return -EIO;
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif

	msg_buff = kzalloc(cnum, GFP_KERNEL);
	if (!msg_buff)
		return -ENOMEM;

	memcpy(msg_buff, reg, cnum);

	buff = kzalloc(num, GFP_KERNEL);
	if (!buff) {
		kfree(msg_buff);
		return -ENOMEM;
	}

	mutex_lock(&info->i2c_mutex);

	xfer_msg[0].addr = info->client->addr;
	xfer_msg[0].len = cnum;
	xfer_msg[0].flags = 0 | I2C_M_DMA_SAFE;
	xfer_msg[0].buf = msg_buff;

	xfer_msg[1].addr = info->client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags = I2C_M_RD | I2C_M_DMA_SAFE;
	xfer_msg[1].buf = buff;

	do {
		ret = i2c_transfer(info->client->adapter, xfer_msg, 2);
		if (ret < 0) {
			info->comm_err_count++;
			input_err(true, &info->client->dev,
					"%s failed(%d). ret:%d, addr:%x, cnt:%d\n",
					__func__, retry, ret, xfer_msg[0].addr, info->comm_err_count);
			usleep_range(10 * 1000, 10 * 1000);
		} else {
			break;
		}
	} while (--retry > 0);

	mutex_unlock(&info->i2c_mutex);

	if (retry == 0) {
		input_err(true, &info->client->dev, "%s: I2C read over retry limit\n", __func__);
		ret = -EIO;

		if (info->debug_string & FTS_DEBUG_SEND_UEVENT)
			sec_cmd_send_event_to_user(&info->sec, NULL, "RESULT=I2C");
#ifdef USE_POR_AFTER_I2C_RETRY
		if (info->probe_done && !info->reset_is_on_going)
			schedule_delayed_work(&info->reset_work, msecs_to_jiffies(10));
#endif
	}

	if (info->debug_string & FTS_DEBUG_PRINT_I2C_READ_CMD) {
		int i;

		pr_info("sec_input: i2c_cmd: R: ");
		for (i = 0; i < cnum; i++)
			pr_cont("%02X ", msg_buff[i]);
		pr_cont("|");
		for (i = 0; i < num; i++)
			pr_cont("%02X ", buff[i]);
		pr_cont("\n");

	}

	memcpy(buf, buff, num);
	kfree(msg_buff);
	kfree(buff);

	return ret;
}

static int fts_read_from_sponge(struct fts_ts_info *info,
		u16 offset, u8 *data, int length)
{
	u8 sponge_reg[3];
	u8 *buf;
	int rtn;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif

	mutex_lock(&info->sponge_mutex);
	offset += FTS_CMD_SPONGE_ACCESS;
	sponge_reg[0] = 0xAA;
	sponge_reg[1] = (offset >> 8) & 0xFF;
	sponge_reg[2] = offset & 0xFF;

	buf = kzalloc(length, GFP_KERNEL);
	if (buf == NULL) {
		rtn = -ENOMEM;
		goto out;
	}

	rtn = fts_read_reg(info, sponge_reg, 3, buf, length);
	if (rtn >= 0)
		memcpy(data, &buf[0], length);
	else
		input_err(true, &info->client->dev, "%s: failed\n", __func__);

	kfree(buf);
out:
	mutex_unlock(&info->sponge_mutex);
	return rtn;
}

/*
 * int fts_write_to_sponge(struct fts_ts_info *, u16 *, u8 *, int)
 * send command or write specific value to the sponge area.
 * sponge area means guest image or display lab firmware.. etc..
 */
static int fts_write_to_sponge(struct fts_ts_info *info,
		u16 offset, u8 *data, int length)
{
	u8 regAdd[3 + length];
	int ret = 0;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif

	mutex_lock(&info->sponge_mutex);
	offset += FTS_CMD_SPONGE_ACCESS;
	regAdd[0] = FTS_CMD_SPONGE_READ_WRITE_CMD;
	regAdd[1] = (offset >> 8) & 0xFF;
	regAdd[2] = offset & 0xFF;

	memcpy(&regAdd[3], &data[0], length);

	ret = fts_write_reg(info, &regAdd[0], 3 + length);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: sponge command is failed. ret: %d\n", __func__, ret);
		goto out;
	}

	// Notify Command
	regAdd[0] = FTS_CMD_SPONGE_NOTIFY_CMD;
	regAdd[1] = (offset >> 8) & 0xFF;
	regAdd[2] = offset & 0xFF;

	ret = fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: sponge notify is failed.\n", __func__);
		goto out;
	}

	input_info(true, &info->client->dev,
			"%s: sponge notify is OK[0x%02X].\n", __func__, *data);
out:
	mutex_unlock(&info->sponge_mutex);
	return ret;
}

int fts_check_custom_library(struct fts_ts_info *info)
{
	struct fts_sponge_information *sponge_info;

	u8 regAdd[3] = { 0xA4, 0x06, 0x91 };
	u8 data[sizeof(struct fts_sponge_information)] = { 0 };
	int ret = -1;

	fts_interrupt_set(info, INT_DISABLE);

	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 0);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto out;
	}

	fts_interrupt_set(info, INT_ENABLE);

	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x00;
	ret = fts_read_reg(info, &regAdd[0], 3, &data[0], sizeof(struct fts_sponge_information));
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto out;
	}

	sponge_info = (struct fts_sponge_information *) &data[0];

	input_info(true, &info->client->dev,
			"%s: SPONGE INFO: use:%d, ver:%d, model name %s\n",
			__func__, sponge_info->sponge_use,
			sponge_info->sponge_ver, sponge_info->sponge_model_name);

	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
			&info->lowpower_flag, sizeof(info->lowpower_flag));
	if (ret < 0)
		goto out;

	if (!info->board->support_fod)
		goto out;

	ret = info->fts_read_from_sponge(info, FTS_CMD_SPONGE_FOD_INFO, regAdd, 3);
	if (ret < 0)
		goto out;

	info->fod_x = regAdd[0];
	info->fod_y = regAdd[1];
	info->fod_vi_size = regAdd[2];
	input_info(true, &info->client->dev, "%s: fod_x:%d fod_y:%d fod_vi_size:%d\n",
			__func__, info->fod_x, info->fod_y, info->fod_vi_size);

out:
	return ret;
}

int fts_set_press_property(struct fts_ts_info *info)
{
	int ret = 0;

	if (!info->board->support_fod)
		return 0;

	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_PRESS_PROPERTY,
			&info->press_prop, sizeof(info->press_prop));
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->press_prop);

	return ret;
}

void fts_delay(unsigned int ms)
{
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000);
	else
		msleep(ms);
}

void fts_command(struct fts_ts_info *info, u8 cmd, bool checkEcho)
{
	u8 regAdd = cmd;
	int ret = 0;

	fts_interrupt_set(info, INT_DISABLE);
	input_info(true, &info->client->dev, "%s: 0x%02X\n", __func__, regAdd);

	if (checkEcho)
		ret = fts_fw_wait_for_echo_event(info, &regAdd, 1, 0);
	else
		ret = fts_write_reg(info, &regAdd, 1);
	if (ret < 0)
		input_info(true, &info->client->dev,
				"%s: failed to write cmd, ret = %d\n", __func__, ret);

	fts_interrupt_set(info, INT_ENABLE);
}

int fts_set_scanmode(struct fts_ts_info *info, u8 scan_mode)
{
	u8 regAdd[3] = { FTS_CMD_SET_FUNCTION_ONOFF, FTS_FUNCTION_ENABLE_VSYNC, 0x00 };
	int rc;

	fts_interrupt_set(info, INT_DISABLE);

#ifndef CONFIG_SEC_FACTORY
	if (info->board->disable_vsync_scan)
#endif
	{
		rc = fts_write_reg(info, regAdd, 3);
		if (rc < 0)
			input_err(true, &info->client->dev,
					"%s: failed to disable vsync scan, ret = %d\n", __func__, rc);
		else
			input_info(true, &info->client->dev, "%s: disable vsync scan\n", __func__);
	}

	/* read vsync scan mode */
	rc = fts_read_reg(info, regAdd, 2, &info->vsync_scan, 1);
	if (rc < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to get vsync scan mode, ret = %d\n", __func__, rc);
		info->vsync_scan = 0;
	}

	/* set scan mode */
	regAdd[0] = 0xA0;
	regAdd[1] = 0x00;
	regAdd[2] = scan_mode;
	rc = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 0);
	if (rc < 0)
		input_err(true, &info->client->dev,
				"%s: failed to set scan mode, ret = %d\n", __func__, rc);

	fts_interrupt_set(info, INT_ENABLE);
	input_info(true, &info->client->dev, "%s: 0x%02X, vs:0x%02X\n",
			__func__, scan_mode, info->vsync_scan);

	return rc;
}

int fts_set_opmode(struct fts_ts_info *info, u8 mode)
{
	int ret;
	u8 regAdd[2] = {FTS_CMD_SET_GET_OPMODE, mode};

	fts_interrupt_set(info, INT_DISABLE);

	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 2, 0);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to set mode: %d\n", __func__, mode);
		goto out;
	}

	if (info->lowpower_flag & FTS_MODE_DOUBLETAP_WAKEUP) {
		regAdd[0] = FTS_CMD_WRITE_WAKEUP_GESTURE;
		regAdd[1] = 0x02;
		ret = fts_write_reg(info, &regAdd[0], 2);
		if (ret < 0)
			input_err(true, &info->client->dev, "%s: Failed to send aot\n", __func__);
	}

out:
	fts_interrupt_set(info, INT_ENABLE);
	return ret;
}

void fts_set_fod_finger_merge(struct fts_ts_info *info)
{
	int ret;
	u8 regAdd[2] = {FTS_CMD_SET_FOD_FINGER_MERGE, 0};

	if (!info->board->support_fod)
		return;

	if (info->lowpower_flag & FTS_MODE_PRESS)
		regAdd[1] = 1;
	else
		regAdd[1] = 0;

	mutex_lock(&info->sponge_mutex);
	input_info(true, &info->client->dev, "%s: %d\n", __func__, regAdd[1]);

	ret = fts_write_reg(info, regAdd, 2);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed\n", __func__);
	mutex_unlock(&info->sponge_mutex);
}

int fts_set_touch_function(struct fts_ts_info *info)
{
	u8 regAdd[3] = { 0 };
	int ret;

	input_info(true, &info->client->dev, "%s: 0x%04X\n", __func__, info->touch_functions);

	regAdd[0] = FTS_CMD_SET_GET_TOUCH_FUNCTION;
	regAdd[1] = (u8)(info->touch_functions & 0xFF);
	regAdd[2] = (u8)(info->touch_functions >> 8);
	ret = fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to send touch type\n", __func__);
	}

	return ret;
}

void fts_set_cover_type(struct fts_ts_info *info, bool enable)
{
	int ret;
	u8 regAdd[3] = {0};

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->cover_type);

	switch (info->cover_type) {
	case FTS_VIEW_WIRELESS:
	case FTS_VIEW_COVER:
	case FTS_VIEW_WALLET:
	case FTS_FLIP_WALLET:
	case FTS_LED_COVER:
	case FTS_MONTBLANC_COVER:
	case FTS_CLEAR_FLIP_COVER:
	case FTS_QWERTY_KEYBOARD_EUR:
	case FTS_QWERTY_KEYBOARD_KOR:
		info->cover_cmd = (u8)info->cover_type;
		break;
	case FTS_CHARGER_COVER:
	case FTS_COVER_NOTHING1:
	case FTS_COVER_NOTHING2:
	default:
		info->cover_cmd = 0;
		input_err(true, &info->client->dev, "%s: not change touch state, %d\n",
				__func__, info->cover_type);
		break;
	}

	if (enable) {
		regAdd[0] = FTS_CMD_SET_GET_COVERTYPE;
		regAdd[1] = info->cover_cmd;
		ret = fts_write_reg(info, &regAdd[0], 2);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: Failed to send covertype command: %d",
					__func__, info->cover_cmd);
		}

		info->touch_functions |= FTS_TOUCHTYPE_BIT_COVER;

	} else {
		info->touch_functions &= ~FTS_TOUCHTYPE_BIT_COVER;
	}

	fts_set_touch_function(info);
}

void fts_set_grip_type(struct fts_ts_info *info, u8 set_type)
{
	u8 mode = G_NONE;

	input_info(true, &info->client->dev, "%s: re-init grip(%d), edh:%d, edg:%d, lan:%d\n", __func__,
			set_type, info->grip_edgehandler_direction, info->grip_edge_range, info->grip_landscape_mode);

	/* edge handler */
	if (info->grip_edgehandler_direction != 0)
		mode |= G_SET_EDGE_HANDLER;

	if (set_type == GRIP_ALL_DATA) {
		/* edge */
		if (info->grip_edge_range != 60)
			mode |= G_SET_EDGE_ZONE;

		/* dead zone */
		if (info->grip_landscape_mode == 1)	/* default 0 mode, 32 */
			mode |= G_SET_LANDSCAPE_MODE;
		else
			mode |= G_SET_NORMAL_MODE;
	}

	if (mode)
		fts_set_grip_data_to_ic(info, mode);

}

static void fts_charger_mode(struct fts_ts_info *info)
{
	u8 regAdd[2] = {FTS_CMD_SET_GET_CHARGER_MODE, (u8)info->charger_mode};
	int ret;

	input_info(true, &info->client->dev, "%s: Set charger mode CMD[%02X]\n", __func__, regAdd[1]);
	ret = fts_write_reg(info, regAdd, 2);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
}

void fts_change_scan_rate(struct fts_ts_info *info, u8 rate)
{
	u8 regAdd[2] = {FTS_CMD_SET_GET_REPORT_RATE, rate};
	int ret = 0;

	ret = fts_write_reg(info, &regAdd[0], 2);

	input_dbg(true, &info->client->dev, "%s: scan rate (%d Hz), ret = %d\n", __func__, regAdd[1], ret);
}

void fts_interrupt_set(struct fts_ts_info *info, int enable)
{
	struct irq_desc *desc = irq_to_desc(info->irq);

	mutex_lock(&info->irq_mutex);

	if (enable) {
		while (desc->depth > 0)
			enable_irq(info->irq);
	} else {
		disable_irq(info->irq);
	}

	input_dbg(true, &info->client->dev, "%s: %s\n", __func__, enable ? "Enable" : "Disable");
	mutex_unlock(&info->irq_mutex);
}

#if 0
void fts_ic_interrupt_set(struct fts_ts_info *info, int enable)
{
	u8 regAdd[3] = { 0xA4, 0x01, 0x00 };

	if (enable)
		regAdd[2] = 0x01;
	else
		regAdd[2] = 0x00;

	fts_write_reg(info, &regAdd[0], 3);
	fts_delay(10);
}
#endif

static int fts_read_chip_id(struct fts_ts_info *info)
{
	u8 regAdd = FTS_READ_DEVICE_ID;
	u8 val[5] = {0};
	int ret;

	ret = fts_read_reg(info, &regAdd, 1, &val[0], 5);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		return -FTS_I2C_ERROR;
	}

	input_info(true, &info->client->dev, "%s: %c %c %02X %02X %02X\n",
			__func__, val[0], val[1], val[2], val[3], val[4]);

	if ((val[2] != FTS_ID0) && (val[3] != FTS_ID1))
		return -FTS_ERROR_INVALID_CHIP_ID;

	return FTS_NOT_ERROR;
}

static int fts_read_chip_id_hw(struct fts_ts_info *info)
{
	u8 regAdd[5] = { 0xFA, 0x20, 0x00, 0x00, 0x00 };
	u8 val[8] = {0};
	int ret;

	ret = fts_read_reg(info, regAdd, 5, &val[0], 8);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		return ret;
	}

	input_info(true, &info->client->dev, "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n",
			__func__, val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7]);

	if ((val[0] = FTS_ID1) && (val[1] == FTS_ID0))
		return FTS_NOT_ERROR;

	return -FTS_ERROR_INVALID_CHIP_ID;
}

static int fts_wait_for_ready(struct fts_ts_info *info)
{
	struct fts_event_status *p_event_status;
	int rc = -FTS_I2C_ERROR;
	u8 regAdd = FTS_READ_ONE_EVENT;
	u8 data[FTS_EVENT_SIZE];
	int retry = 0;
	int err_cnt = 0;

	mutex_lock(&info->wait_for);

	fts_interrupt_set(info, INT_DISABLE);

	memset(data, 0x0, FTS_EVENT_SIZE);

	while (fts_read_reg(info, &regAdd, 1, (u8 *)data, FTS_EVENT_SIZE) > 0) {
		p_event_status = (struct fts_event_status *) &data[0];

		if ((p_event_status->stype == FTS_EVENT_STATUSTYPE_INFORMATION) &&
				(p_event_status->status_id == FTS_INFO_READY_STATUS)) {
			if (rc == -FTS_ERROR_BROKEN_OSC_TRIM)
				break;

			rc = FTS_NOT_ERROR;
			break;
		}

		if (data[0] == FTS_EVENT_ERROR_REPORT) {
			input_err(true, &info->client->dev,
					"%s: Err detected %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X\n",
					__func__, data[0], data[1], data[2], data[3],
					data[4], data[5], data[6], data[7]);

			// check if config / cx / panel configuration area is corrupted
			if (((data[1] >= 0x20) && (data[1] <= 0x21)) || ((data[1] >= 0xA0) && (data[1] <= 0xA8))) {
				rc = -FTS_ERROR_FW_CORRUPTION;
				info->checksum_result = 1;
				info->fw_corruption = true;
				input_err(true, &info->client->dev, "%s: flash corruption:%02X\n",
						__func__, data[1]);
				break;
			}

			/*
			 * F3 24 / 25 / 29 / 2A / 2D / 34: the flash about OSC TRIM value is broken.
			 */
			if (data[1] == 0x24 || data[1] ==  0x25 || data[1] ==  0x29 || data[1] ==  0x2A || data[1] == 0x2D || data[1] == 0x34) {
				input_err(true, &info->client->dev, "%s: osc trim is broken\n", __func__);
				rc = -FTS_ERROR_BROKEN_OSC_TRIM;
				info->fw_corruption = true;
				info->osc_trim_error = true;
				//break;
			}

			if (err_cnt++ > 32) {
				rc = -FTS_ERROR_EVENT_ID;
				break;
			}
			continue;
		}

		if (retry++ > FTS_RETRY_COUNT * 15) {
			rc = -FTS_ERROR_TIMEOUT;
			if (data[0] == 0 && data[1] == 0 && data[2] == 0)
				rc = -FTS_ERROR_TIMEOUT_ZERO;

			input_err(true, &info->client->dev, "%s: Time Over\n", __func__);

			if (info->fts_power_state == FTS_POWER_STATE_LOWPOWER)
				schedule_delayed_work(&info->reset_work, msecs_to_jiffies(10));
			break;
		}
		fts_delay(20);
	}

	input_info(true, &info->client->dev,
			"%s: %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3],
			data[4], data[5], data[6], data[7]);

	fts_interrupt_set(info, INT_ENABLE);

	mutex_unlock(&info->wait_for);

	return rc;
}

int fts_systemreset(struct fts_ts_info *info, unsigned int msec)
{
	u8 regAdd[6] = { 0xFA, 0x20, 0x00, 0x00, 0x24, 0x81 };
	int rc;

	fts_interrupt_set(info, INT_DISABLE);

	info->osc_trim_error = false;

	rc = fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0)
		goto out;

	fts_delay(msec + 10);

	rc = fts_wait_for_ready(info);

out:
	input_info(true, &info->client->dev, "%s: %d\n", __func__, rc);

	fts_release_all_finger(info);
	fts_interrupt_set(info, INT_ENABLE);

	return rc;
}

int fts_fw_corruption_check(struct fts_ts_info *info)
{
	u8 regAdd[6] = { 0xFA, 0x20, 0x00, 0x00, 0x24, 0x81 };
	u8 val = 0;
	int rc;

	fts_interrupt_set(info, INT_DISABLE);

	/* fts_systemreset */
	rc = fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0) {
		rc = -FTS_I2C_ERROR;
		goto out;
	}
	fts_delay(10);

	/* Firmware Corruption Check */
	regAdd[0] = 0xFA;
	regAdd[1] = 0x20;
	regAdd[2] = 0x00;
	regAdd[3] = 0x00;
	regAdd[4] = 0x78;
	rc = fts_read_reg(info, regAdd, 5, &val, 1);
	if (rc < 0) {
		rc = -FTS_I2C_ERROR;
		goto out;
	}

	if (val & 0x03) { // Check if crc error
		input_err(true, &info->client->dev, "%s: firmware corruption. CRC status:%02X\n",
				__func__, val & 0x03);
		rc = -FTS_ERROR_FW_CORRUPTION;
	} else {
		rc = FTS_NOT_ERROR;
	}
out:
	fts_interrupt_set(info, INT_ENABLE);

	return rc;
}

int fts_get_sysinfo_data(struct fts_ts_info *info, u8 sysinfo_addr, u8 read_cnt, u8 *data)
{
	int ret;
	u8 *buff = NULL;

	u8 regAdd[3] = { 0xA4, 0x06, 0x01 }; // request system information

	fts_interrupt_set(info, INT_DISABLE);

	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 0);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: timeout wait for event\n", __func__);
		goto ERROR;
	}

	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = sysinfo_addr;

	buff = kzalloc(read_cnt, GFP_KERNEL);
	if (!buff) {
		ret = -ENOMEM;
		goto ERROR;
	}

	ret = fts_read_reg(info, &regAdd[0], 3, &buff[0], read_cnt);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
				__func__, ret);
		goto ERROR;
	}

	memcpy(data, &buff[0], read_cnt);

ERROR:
	if (buff)
		kfree(buff);
	fts_interrupt_set(info, INT_ENABLE);
	return ret;
}

int fts_get_version_info(struct fts_ts_info *info)
{
	int rc;
	u8 regAdd = FTS_READ_FW_VERSION;
	u8 data[FTS_VERSION_SIZE] = { 0 };

	memset(data, 0x0, FTS_VERSION_SIZE);

	rc = fts_read_reg(info, &regAdd, 1, (u8 *)data, FTS_VERSION_SIZE);

	info->fw_version_of_ic = (data[0] << 8) + data[1];
	info->config_version_of_ic = (data[2] << 8) + data[3];
	info->fw_main_version_of_ic = data[4] + (data[5] << 8);
	info->project_id_of_ic = data[6];
	info->ic_name_of_ic = data[7];
	info->module_version_of_ic = data[8];

	input_info(true, &info->client->dev,
			"%s: [IC] Firmware Ver: 0x%04X, Config Ver: 0x%04X, Main Ver: 0x%04X\n",
			__func__, info->fw_version_of_ic,
			info->config_version_of_ic, info->fw_main_version_of_ic);
	input_info(true, &info->client->dev,
			"%s: [IC] Project ID: 0x%02X, IC Name: 0x%02X, Module Ver: 0x%02X\n",
			__func__, info->project_id_of_ic,
			info->ic_name_of_ic, info->module_version_of_ic);

	return rc;
}

int fts_fix_active_mode(struct fts_ts_info *info, bool enable)
{
	u8 regAdd[3] = {0xA0, 0x00, 0x00};
	int ret;

	if (enable) {
		regAdd[1] = 0x03;
		regAdd[2] = 0x00;
	} else {
		regAdd[1] = 0x00;
		regAdd[2] = 0x01;
	}

	ret = fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0)
		input_info(true, &info->client->dev, "%s: err: %d\n", __func__, ret);
	else
		input_info(true, &info->client->dev, "%s: %s\n", __func__,
				enable ? "fix" : "release");

	fts_delay(10);

	return ret;
}

/* Added for samsung dependent codes such as Factory test,
 * Touch booster, Related debug sysfs.
 */
#include "fts_sec.c"


static ssize_t fts_tsp_cmoffset_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset, int position)
{
	struct fts_ts_info *info;
	static ssize_t retlen = 0;
	char *cmoffset_proc;
	ssize_t count;
	loff_t pos = *offset;

	if (!g_fts_info) {
		pr_err("%s %s: dev is null\n", SECLOG, __func__);
		return 0;
	}
	info = g_fts_info;

	switch (position) {
	case OFFSET_FW_SDC:
		cmoffset_proc = info->cmoffset_sdc_proc;
		break;
	case OFFSET_FW_SUB:
		cmoffset_proc = info->cmoffset_sub_proc;
		break;
	case OFFSET_FW_MAIN:
		cmoffset_proc = info->cmoffset_main_proc;
		break;
	default:
		return 0;
	}

	if (!cmoffset_proc)
		return 0;

	if (pos == 0)
#ifdef CONFIG_SEC_FACTORY
	{
		mutex_lock(&info->device_mutex);
		retlen = fts_get_cmoffset_dump(info, cmoffset_proc, position);
		mutex_unlock(&info->device_mutex);
	}
#else
		retlen = strlen(cmoffset_proc);
#endif

	if (pos >= retlen)
		return 0;

	count = min(len, (size_t)(retlen - pos));

	if (copy_to_user(buf, cmoffset_proc + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static ssize_t fts_tsp_cmoffset_sdc_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	pr_debug("%s called offset:%d\n", __func__, (int)*offset);
	return fts_tsp_cmoffset_read(file, buf, len, offset, OFFSET_FW_SDC);
}

static ssize_t fts_tsp_cmoffset_sub_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	pr_debug("%s called offset:%d\n", __func__, (int)*offset);
	return fts_tsp_cmoffset_read(file, buf, len, offset, OFFSET_FW_SUB);
}

static ssize_t fts_tsp_cmoffset_main_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	pr_debug("%s called offset:%d\n", __func__, (int)*offset);
	return fts_tsp_cmoffset_read(file, buf, len, offset, OFFSET_FW_MAIN);
}

static const struct file_operations tsp_cmoffset_sdc_file_ops = {
	.owner = THIS_MODULE,
	.read = fts_tsp_cmoffset_sdc_read,
	.llseek = generic_file_llseek,
};
static const struct file_operations tsp_cmoffset_sub_file_ops = {
	.owner = THIS_MODULE,
	.read = fts_tsp_cmoffset_sub_read,
	.llseek = generic_file_llseek,
};
static const struct file_operations tsp_cmoffset_main_file_ops = {
	.owner = THIS_MODULE,
	.read = fts_tsp_cmoffset_main_read,
	.llseek = generic_file_llseek,
};

static void fts_init_proc(struct fts_ts_info *info)
{
	struct proc_dir_entry *entry_sdc, *entry_sub, *entry_main;

	info->proc_size = (info->SenseChannelLength * 4 + 1) * info->ForceChannelLength + 1;

	info->cmoffset_sdc_proc = kzalloc(info->proc_size, GFP_KERNEL);
	if (!info->cmoffset_sdc_proc) {
		input_err(true, &info->client->dev, "%s: failed to alloc cmoffset_sdc_proc\n", __func__);
		return;
	}

	info->cmoffset_sub_proc = kzalloc(info->proc_size, GFP_KERNEL);
	if (!info->cmoffset_sub_proc) {
		input_err(true, &info->client->dev, "%s: failed to alloc cmoffset_sub_proc\n", __func__);
		goto err_alloc_sub;
	}

	info->cmoffset_main_proc = kzalloc(info->proc_size, GFP_KERNEL);
	if (!info->cmoffset_main_proc) {
		input_err(true, &info->client->dev, "%s: failed to alloc cmoffset_main_proc\n", __func__);
		goto err_alloc_main;
	}

	entry_sdc = proc_create("tsp_cmoffset_sdc", S_IFREG | S_IRUGO, NULL, &tsp_cmoffset_sdc_file_ops);
	if (!entry_sdc) {
		input_err(true, &info->client->dev, "%s: failed to create /proc/tsp_cmoffset_sdc\n", __func__);
		goto err;
	}
	proc_set_size(entry_sdc, info->proc_size);

	entry_sub = proc_create("tsp_cmoffset_sub", S_IFREG | S_IRUGO, NULL, &tsp_cmoffset_sub_file_ops);
	if (!entry_sub) {
		input_err(true, &info->client->dev, "%s: failed to create /proc/tsp_cmoffset_sub\n", __func__);
		goto err;
	}
	proc_set_size(entry_sub, info->proc_size);

	entry_main = proc_create("tsp_cmoffset_main", S_IFREG | S_IRUGO, NULL, &tsp_cmoffset_main_file_ops);
	if (!entry_main) {
		input_err(true, &info->client->dev, "%s: failed to create /proc/tsp_cmoffset_main\n", __func__);
		goto err;
	}
	proc_set_size(entry_main, info->proc_size);

	g_fts_info = info;
	input_info(true, &info->client->dev, "%s: done\n", __func__);
	return;
err:
	kfree(info->cmoffset_main_proc);
err_alloc_main:
	kfree(info->cmoffset_sub_proc);
err_alloc_sub:
	kfree(info->cmoffset_sdc_proc);

	info->cmoffset_sdc_proc = NULL;
	info->cmoffset_sub_proc = NULL;
	info->cmoffset_main_proc = NULL;

	input_err(true, &info->client->dev, "%s: failed\n", __func__);
}

int fts_get_hf_data(struct fts_ts_info *info)
{
	u8 regAdd[4] = { 0 };
	int ret;
	short min = 0x7FFF;
	short max = 0x8000;

	info->fts_systemreset(info, 0);

	fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	input_info(true, &info->client->dev, "%s\n", __func__);

	regAdd[0] = 0xA4;
	regAdd[1] = 0x04;
	regAdd[2] = 0xFF;
	regAdd[3] = 0x01;
	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 4, 100);
	if (ret < 0)
		goto out;

	ret = fts_read_frame(info, TYPE_RAW_DATA, &min, &max);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed to get rawdata\n", __func__);

out:
	info->fts_systemreset(info, 0);

	fts_set_cover_type(info, info->flip_enable);

	fts_delay(10);

	if (info->charger_mode) {
		fts_charger_mode(info);
		fts_delay(10);
	}

	if (!info->flip_enable)
		fts_set_scanmode(info, info->scan_mode);
	else
		fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);

	return ret;
}

int fts_get_miscal_data(struct fts_ts_info *info, short *min, short *max)
{
	u8 cmd = FTS_READ_ONE_EVENT;
	u8 regAdd[2] = { 0 };
	u8 data[FTS_EVENT_SIZE];
	int retry = 200, ret;
	int result = FTS_EVENT_ERROR_REPORT;
	short min_print = 0x7FFF;
	short max_print = 0x8000;

	info->fts_systemreset(info, 0);

	fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	mutex_lock(&info->wait_for);

	regAdd[0] = 0xC7;
	regAdd[1] = 0x02;

	ret = info->fts_write_reg(info, &regAdd[0], 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,
			"%s: failed to write miscal cmd\n", __func__);
		result = -EIO;
		mutex_unlock(&info->wait_for);
		goto error;
	}

	memset(data, 0x0, FTS_EVENT_SIZE);

	while (retry-- >= 0) {
		memset(data, 0x00, sizeof(data));
		ret = info->fts_read_reg(info, &cmd, 1, data, FTS_EVENT_SIZE);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: read failed: %d\n", __func__, ret);
			result = -EIO;
			mutex_unlock(&info->wait_for);
			goto error;
		}

		if (data[0] != 0x00)
			input_info(true, &info->client->dev, "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n",
					__func__, data[0], data[1], data[2], data[3],
					data[4], data[5], data[6], data[7]);

		fts_delay(10);

		if (data[0] == FTS_EVENT_PASS_REPORT || data[0] == FTS_EVENT_ERROR_REPORT) {
			result = data[0];
			*max = data[3] << 8 | data[2];
			*min = data[5] << 8 | data[4];
			break;
		}

		if (retry == 0) {
			result = -EIO;
			mutex_unlock(&info->wait_for);
			goto error;
		}
	}
	mutex_unlock(&info->wait_for);

	input_raw_info(true, &info->client->dev, "%s: %s %d,%d\n",
			__func__, data[0] == FTS_EVENT_PASS_REPORT ? "pass" : "fail", *min, *max);

	ret = fts_read_frame(info, TYPE_RAW_DATA, &min_print, &max_print);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed to get rawdata\n", __func__);

error:
	fts_reinit(info, false);

	return result;
}

int fts_osc_trim_recovery(struct fts_ts_info *info)
{
	u8 regaddr[3];
	int ret;

	input_info(true, &info->client->dev, "%s\n", __func__);

	fts_interrupt_set(info, INT_DISABLE);

	/*  OSC trim error recovery command. */
	regaddr[0] = 0xA4;
	regaddr[1] = 0x00;
	regaddr[2] = 0x05;
	ret = fts_fw_wait_for_echo_event(info, &regaddr[0], 3, 800);
	if (ret < 0) {
		ret = -FTS_ERROR_BROKEN_OSC_TRIM;
		goto out;
	}

	/* save panel configuration area */
	regaddr[0] = 0xA4;
	regaddr[1] = 0x05;
	regaddr[2] = 0x04;
	ret = fts_fw_wait_for_echo_event(info, &regaddr[0], 3, 200);
	if (ret < 0) {
		ret = -FTS_ERROR_BROKEN_OSC_TRIM;
		goto out;
	}

	fts_delay(500);
	ret = info->fts_systemreset(info, 0);
	fts_delay(50);

out:
	fts_interrupt_set(info, INT_ENABLE);

	return ret;
}

int fts_set_temp(struct fts_ts_info *info, bool bforced)
{
	int ret = 0;
	u8 temp_data = 0;
	u8 regAdd[2] = {0x3C, 0x00};

	if (!info->psy)
		info->psy = power_supply_get_by_name("battery");

	if (!info->psy) {
		input_err(true, &info->client->dev, "%s: Cannot find power supply\n", __func__);
		return -1;
	}

	ret = power_supply_get_property(info->psy, POWER_SUPPLY_PROP_TEMP, &info->psy_value);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: Couldn't get aicl settled value ret=%d\n", __func__, ret);
		return ret;
	}

	temp_data = (u8)(info->psy_value.intval / 10);
	if (bforced || info->tsp_temp_data != temp_data) {
		regAdd[1] = temp_data;
		ret = fts_write_reg(info, &regAdd[0], 2);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: Failed to write\n", __func__);
			return ret;
		}

		info->tsp_temp_data = temp_data;
		input_info(true, &info->client->dev, "%s set temperature:%d\n", __func__, (s8)temp_data);
	} else {
		input_dbg(true, &info->client->dev, "%s skip temperature:%d\n", __func__, (s8)temp_data);
	}

	return ret;
}

static int fts_init(struct fts_ts_info *info)
{
	u8 retry = 3;
	u8 regAdd[8] = { 0 };
	int rc;
	u8 data[FTS_EVENT_SIZE] = { 0 };

	rc = fts_read_reg(info, &regAdd[0], 1, (u8 *)data, FTS_EVENT_SIZE);
	if (rc == -ENOTCONN) {
		return rc;
	}

	do {
		info->checksum_result = 0;
		info->fw_corruption = false;

		rc = fts_fw_corruption_check(info);
		if (rc == -FTS_ERROR_FW_CORRUPTION) {
			info->checksum_result = 1;
			break;
		} else if (rc < 0) {
			goto reset;
		}

		rc = fts_wait_for_ready(info);
reset:
		if (rc < 0) {
			if (info->checksum_result) {
#ifdef CONFIG_SEC_FACTORY
				fts_execute_autotune(info, true);
				continue;
#else
				break;
#endif
			} else if (info->osc_trim_error) {
				break;
			} else if (rc == -FTS_ERROR_TIMEOUT_ZERO) {
				rc = fts_read_chip_id_hw(info);
				if (rc == FTS_NOT_ERROR) {
					info->checksum_result = 1;
					input_err(true, &info->client->dev,
							"%s: config corruption\n", __func__);
					break;
				}
			}
			fts_reset(info, 20);
		} else {
			break;
		}
	} while (--retry);

	fts_get_version_info(info);

	retry = 1;
	do {
		if (info->osc_trim_error) {
			rc = fts_osc_trim_recovery(info);
			if (rc < 0)
				input_err(true, &info->client->dev, "%s: Failed to recover osc trim\n", __func__);
		} else {
			break;
		}
	} while (retry--);

	if (!info->checksum_result && rc < 0) {
		input_err(true, &info->client->dev, "%s: Failed to system reset\n", __func__);
		//return -EIO;
	}

	if (info->checksum_result) {
		info->fw_version_of_ic = 0;
		info->config_version_of_ic = 0;
		info->fw_main_version_of_ic = 0;
	}

	rc = fts_read_chip_id(info);
	if (rc < 0) {
		fts_reset(info, 500);	/* Delay to discharge the IC from ESD or On-state.*/

		input_err(true, &info->client->dev, "%s: Reset caused by chip id error\n", __func__);

		rc = fts_read_chip_id(info);
		//if (rc < 0)
		//	return 1;
	}

	rc = fts_fw_update_on_probe(info);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: Failed to firmware update\n",
				__func__);
		return rc;
	}

	rc = fts_get_channel_info(info);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, rc);
		return rc;
	}
	input_info(true, &info->client->dev, "%s: Sense(%02d) Force(%02d)\n", __func__,
			info->SenseChannelLength, info->ForceChannelLength);

	info->pFrame = kzalloc(info->SenseChannelLength * info->ForceChannelLength * 2 + 1, GFP_KERNEL);
	if (!info->pFrame)
		return -ENOMEM;

	info->cx_data = kzalloc(info->SenseChannelLength * info->ForceChannelLength + 1, GFP_KERNEL);
	if (!info->cx_data) {
		kfree(info->pFrame);
		return -ENOMEM;
	}

	info->ito_result = kzalloc(FTS_ITO_RESULT_PRINT_SIZE, GFP_KERNEL);
	if (!info->ito_result) {
		kfree(info->cx_data);
		kfree(info->pFrame);
		return -ENOMEM;
	}

	/* fts driver set functional feature */
	info->touch_opmode = FTS_OPMODE_NORMAL;
	info->charger_mode = FTS_CHARGER_MODE_NORMAL;

#ifdef TCLM_CONCEPT
	info->tdata->external_factory = false;
#endif

	info->touch_functions = FTS_TOUCHTYPE_DEFAULT_ENABLE;
	fts_set_touch_function(info);
	fts_delay(10);

	fts_command(info, FTS_CMD_FORCE_CALIBRATION, true);

	fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	info->scan_mode = FTS_SCAN_MODE_DEFAULT;

	fts_set_scanmode(info, info->scan_mode);

	input_info(true, &info->client->dev, "%s: resolution:(IC)x:%d y:%d, (DT)x:%d,y:%d\n",
			__func__, info->ICXResolution, info->ICYResolution,
			info->board->max_x, info->board->max_y);

	input_info(true, &info->client->dev, "%s: Initialized\n", __func__);

	return 0;
}

static void fts_print_info(struct fts_ts_info *info)
{
	struct irq_desc *desc = irq_to_desc(info->irq);

	info->print_info_cnt_open++;

	if (info->print_info_cnt_open > 0xfff0)
		info->print_info_cnt_open = 0;

	if (info->touch_count == 0)
		info->print_info_cnt_release++;

	input_info(true, &info->client->dev,
			"noise:%x,%x chg:%d tc:%d iq:%d depth:%d lp:%x fn:%X wet:%d flip:%d vs:%x // v:%04X cal:%02X,C%02XT%04X.%4s%s Cal_flag:%s,%d // #%d %d\n",
			info->touch_noise_status, info->touch_noise_reason,
			info->charger_mode, info->touch_count,
			gpio_get_value(info->board->irq_gpio), desc->depth,
			info->lowpower_flag, info->touch_functions, info->wet_mode,
			info->flip_enable, info->vsync_scan,
			(info->module_version_of_ic << 8) | (info->fw_main_version_of_ic & 0xFF),
			info->test_result.data[0],
#ifdef TCLM_CONCEPT
			info->tdata->nvdata.cal_count, info->tdata->nvdata.tune_fix_ver,
			info->tdata->tclm_string[info->tdata->nvdata.cal_position].f_name,
			(info->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ",
			(info->tdata->nvdata.cal_fail_falg == SEC_CAL_PASS) ? "Success" : "Fail",
			info->tdata->nvdata.cal_fail_cnt,
#else
			0, 0, " ", " ", " ", 0,
#endif
			info->print_info_cnt_open, info->print_info_cnt_release);
}

static void fts_print_info_work(struct work_struct *work)
{
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
			work_print_info.work);

	if (!info->probe_done)
		return;

	fts_print_info(info);

	if (info->sec.cmd_is_running) {
		input_err(true, &info->client->dev, "%s: skip sec_ts_set_temp, cmd running\n", __func__);
	} else {
		if (info->touch_count) {
			info->tsp_temp_data_skip = true;
			input_err(true, &info->client->dev,
					"%s: skip sec_ts_set_temp, t_cnt(%d)\n", __func__, info->touch_count);
		} else {
			info->tsp_temp_data_skip = false;
			fts_set_temp(info, false);
		}
	}
	schedule_delayed_work(&info->work_print_info, msecs_to_jiffies(TOUCH_PRINT_INFO_DWORK_TIME));
}
/************************************************************
 *  720  * 1480 : <48 96 60> indicator: 24dp navigator:48dp edge:60px dpi=320
 * 1080  * 2220 :  4096 * 4096 : <133 266 341>  (approximately value)
 ************************************************************/

static void location_detect(struct fts_ts_info *info, char *loc, int x, int y)
{
	int i;

	for (i = 0; i < FTS_TS_LOCATION_DETECT_SIZE; ++i)
		loc[i] = 0;

	if (x < info->board->area_edge)
		strcat(loc, "E.");
	else if (x < (info->board->max_x - info->board->area_edge))
		strcat(loc, "C.");
	else
		strcat(loc, "e.");

	if (y < info->board->area_indicator)
		strcat(loc, "S");
	else if (y < (info->board->max_y - info->board->area_navigation))
		strcat(loc, "C");
	else
		strcat(loc, "N");
}

static const char finger_mode[10] = {'N', '1', '2', 'G', '4', 'P'};

static u8 fts_event_handler_type_b(struct fts_ts_info *info)
{
	u8 regAdd;
	int left_event_count = 0;
	int EventNum = 0;
	u8 TouchID = 0, event_id = 0;

	u8 data[FTS_FIFO_MAX * FTS_EVENT_SIZE] = {0};

	u8 *event_buff;
	struct fts_event_coordinate *p_event_coord;
	struct fts_gesture_status *p_gesture_status;
	struct fts_event_status *p_event_status;

	u8 prev_action = 0;
	char location[FTS_TS_LOCATION_DETECT_SIZE] = { 0 };

	regAdd = FTS_READ_ONE_EVENT;
	fts_read_reg(info, &regAdd, 1, (u8 *)&data[0 * FTS_EVENT_SIZE], FTS_EVENT_SIZE);
	left_event_count = (data[7] & 0x3F);

	if (left_event_count >= FTS_FIFO_MAX)
		left_event_count = FTS_FIFO_MAX - 1;

	if (left_event_count > 0) {
		regAdd = FTS_READ_ALL_EVENT;
		fts_read_reg(info, &regAdd, 1, (u8 *)&data[1 * FTS_EVENT_SIZE], FTS_EVENT_SIZE * (left_event_count));
	}

	do {
		/* for event debugging */
		if (info->debug_string & 0x1)
			input_info(true, &info->client->dev, "[%d] %02X %02X %02X %02X %02X %02X %02X %02X\n",
					EventNum, data[EventNum * FTS_EVENT_SIZE+0], data[EventNum * FTS_EVENT_SIZE+1],
					data[EventNum * FTS_EVENT_SIZE+2], data[EventNum * FTS_EVENT_SIZE+3],
					data[EventNum * FTS_EVENT_SIZE+4], data[EventNum * FTS_EVENT_SIZE+5],
					data[EventNum * FTS_EVENT_SIZE+6], data[EventNum * FTS_EVENT_SIZE+7]);

		event_buff = (u8 *) &data[EventNum * FTS_EVENT_SIZE];
		event_id = event_buff[0] & 0x3;

		switch (event_id) {
		case FTS_STATUS_EVENT:
			p_event_status = (struct fts_event_status *)event_buff;

			if (p_event_status->stype > 0)
				input_info(true, &info->client->dev, "%s: STATUS %02X %02X %02X %02X %02X %02X %02X %02X\n",
						__func__, event_buff[0], event_buff[1], event_buff[2],
						event_buff[3], event_buff[4], event_buff[5],
						event_buff[6], event_buff[7]);

			if ((p_event_status->stype == FTS_EVENT_STATUSTYPE_ERROR) &&
					(p_event_status->status_id == FTS_ERR_EVENT_QUEUE_FULL)) {
				input_err(true, &info->client->dev, "%s: IC Event Queue is full\n", __func__);
				fts_release_all_finger(info);
			}

			if ((p_event_status->stype == FTS_EVENT_STATUSTYPE_ERROR) &&
					(p_event_status->status_id == FTS_ERR_EVENT_ESD)) {
				input_err(true, &info->client->dev, "%s: ESD detected. run reset\n", __func__);
				if (!info->reset_is_on_going)
					schedule_delayed_work(&info->reset_work, msecs_to_jiffies(10));
			}

			if ((p_event_status->stype == FTS_EVENT_STATUSTYPE_INFORMATION) &&
					(p_event_status->status_id == FTS_INFO_WET_MODE)) {
				info->wet_mode = p_event_status->status_data_1;

				input_info(true, &info->client->dev, "%s: WET MODE %s[%d]\n",
						__func__, info->wet_mode == 0 ? "OFF" : "ON",
						p_event_status->status_data_1);

				if (info->wet_mode)
					info->wet_count++;
			}

			if ((p_event_status->stype == FTS_EVENT_STATUSTYPE_INFORMATION) &&
					(p_event_status->status_id == FTS_INFO_NOISE_MODE)) {
				info->touch_noise_status = p_event_status->status_data_1;

				input_info(true, &info->client->dev, "%s: NOISE MODE %s[%02X]\n",
						__func__, info->touch_noise_status == 0 ? "OFF" : "ON",
						p_event_status->status_data_1);

				if (info->touch_noise_status)
					info->noise_count++;
			}

			break;

		case FTS_COORDINATE_EVENT:
			p_event_coord = (struct fts_event_coordinate *) event_buff;

			TouchID = p_event_coord->tid;
			if (TouchID >= FINGER_MAX) {
				input_err(true, &info->client->dev,
						"%s: tid(%d) is out of supported max finger number\n",
						__func__, TouchID);
				break;
			}

			info->finger[TouchID].prev_ttype = info->finger[TouchID].ttype;
			prev_action = info->finger[TouchID].action;
			info->finger[TouchID].id = TouchID;
			info->finger[TouchID].action = p_event_coord->tchsta;
			info->finger[TouchID].x = (p_event_coord->x_11_4 << 4) | (p_event_coord->x_3_0);
			info->finger[TouchID].y = (p_event_coord->y_11_4 << 4) | (p_event_coord->y_3_0);
			info->finger[TouchID].z = p_event_coord->z & 0x3F;
			info->finger[TouchID].ttype = p_event_coord->ttype_3_2 << 2 |
							p_event_coord->ttype_1_0 << 0;
			info->finger[TouchID].major = p_event_coord->major;
			info->finger[TouchID].minor = p_event_coord->minor;

			if (!info->finger[TouchID].palm &&
					info->finger[TouchID].ttype == FTS_EVENT_TOUCHTYPE_PALM)
				info->finger[TouchID].palm_count++;

			info->finger[TouchID].palm = (info->finger[TouchID].ttype == FTS_EVENT_TOUCHTYPE_PALM);
			info->finger[TouchID].left_event = p_event_coord->left_event;

			info->finger[TouchID].noise_level = p_event_coord->noise_level;
			info->finger[TouchID].max_strength = max(info->finger[TouchID].max_strength, p_event_coord->max_strength);
			info->finger[TouchID].hover_id_num = max(info->finger[TouchID].hover_id_num, (u8)p_event_coord->hover_id_num);

			if (info->finger[TouchID].z <= 0)
				info->finger[TouchID].z = 1;

			if ((info->finger[TouchID].ttype == FTS_EVENT_TOUCHTYPE_NORMAL) ||
					(info->finger[TouchID].ttype == FTS_EVENT_TOUCHTYPE_PALM)   ||
					(info->finger[TouchID].ttype == FTS_EVENT_TOUCHTYPE_WET)    ||
					(info->finger[TouchID].ttype == FTS_EVENT_TOUCHTYPE_GLOVE)) {

				location_detect(info, location, info->finger[TouchID].x, info->finger[TouchID].y);

				if (info->finger[TouchID].action == FTS_COORDINATE_ACTION_RELEASE) {
					input_mt_slot(info->input_dev, TouchID);

					if (info->board->support_mt_pressure)
						input_report_abs(info->input_dev, ABS_MT_PRESSURE, 0);

					input_report_abs(info->input_dev, ABS_MT_CUSTOM, 0);

					input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 0);

					if (info->touch_count > 0)
						info->touch_count--;

					if (info->touch_count == 0) {
						input_report_key(info->input_dev, BTN_TOUCH, 0);
						input_report_key(info->input_dev, BTN_TOOL_FINGER, 0);
						info->check_multi = 0;
						info->print_info_cnt_release = 0;
					}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
					input_info(true, &info->client->dev,
							"[R] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d p:%d nlvl:%d maxS:%d hid:%d\n",
							TouchID, location,
							info->finger[TouchID].x - info->finger[TouchID].p_x,
							info->finger[TouchID].y - info->finger[TouchID].p_y,
							info->finger[TouchID].mcount, info->touch_count,
							info->finger[TouchID].x, info->finger[TouchID].y,
							info->finger[TouchID].palm_count, info->finger[TouchID].noise_level,
							info->finger[TouchID].max_strength, info->finger[TouchID].hover_id_num);
#else
					input_info(true, &info->client->dev,
							"[R] tID:%d loc:%s dd:%d,%d mc:%d tc:%d p:%d nlvl:%d maxS:%d hid:%d\n",
							TouchID, location,
							info->finger[TouchID].x - info->finger[TouchID].p_x,
							info->finger[TouchID].y - info->finger[TouchID].p_y,
							info->finger[TouchID].mcount, info->touch_count,
							info->finger[TouchID].palm_count, info->finger[TouchID].noise_level,
							info->finger[TouchID].max_strength, info->finger[TouchID].hover_id_num);
#endif

					info->finger[TouchID].action = FTS_COORDINATE_ACTION_NONE;
					info->finger[TouchID].mcount = 0;
					info->finger[TouchID].palm_count = 0;
					info->finger[TouchID].noise_level = 0;
					info->finger[TouchID].max_strength = 0;
					info->finger[TouchID].hover_id_num = 0;

				} else if (info->finger[TouchID].action == FTS_COORDINATE_ACTION_PRESS) {

					info->touch_count++;
					info->all_finger_count++;

					info->finger[TouchID].p_x = info->finger[TouchID].x;
					info->finger[TouchID].p_y = info->finger[TouchID].y;

					input_mt_slot(info->input_dev, TouchID);
					input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 1);
					input_report_key(info->input_dev, BTN_TOUCH, 1);
					input_report_key(info->input_dev, BTN_TOOL_FINGER, 1);

					input_report_abs(info->input_dev, ABS_MT_POSITION_X, info->finger[TouchID].x);
					input_report_abs(info->input_dev, ABS_MT_POSITION_Y, info->finger[TouchID].y);
					input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR,
								info->finger[TouchID].major);
					input_report_abs(info->input_dev, ABS_MT_TOUCH_MINOR,
								info->finger[TouchID].minor);

					if (info->brush_mode)
						input_report_abs(info->input_dev, ABS_MT_CUSTOM,
									(info->finger[TouchID].z << 1) |
									info->finger[TouchID].palm);
					else
						input_report_abs(info->input_dev, ABS_MT_CUSTOM,
									(BRUSH_Z_DATA << 1) |
									info->finger[TouchID].palm);

					if (info->board->support_mt_pressure)
						input_report_abs(info->input_dev, ABS_MT_PRESSURE,
									info->finger[TouchID].z);

					if ((info->touch_count > 4) && (info->check_multi == 0)) {
						info->check_multi = 1;
						info->multi_count++;
					}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
					input_info(true, &info->client->dev,
							"[P] tID:%d.%d x:%d y:%d z:%d major:%d minor:%d loc:%s tc:%d type:%d p:%d nlvl:%d maxS:%d hid:%d\n",
							TouchID, (info->input_dev->mt->trkid - 1) & TRKID_MAX,
							info->finger[TouchID].x, info->finger[TouchID].y,
							info->finger[TouchID].z,
							info->finger[TouchID].major, info->finger[TouchID].minor,
							location, info->touch_count, info->finger[TouchID].ttype,
							info->finger[TouchID].palm_count, info->finger[TouchID].noise_level,
							info->finger[TouchID].max_strength, info->finger[TouchID].hover_id_num);
#else
					input_info(true, &info->client->dev,
							"[P] tID:%d.%d z:%d major:%d minor:%d loc:%s tc:%d type:%d p:%d nlvl:%d maxS:%d hid:%d\n",
							TouchID, (info->input_dev->mt->trkid - 1) & TRKID_MAX,
							info->finger[TouchID].z,
							info->finger[TouchID].major, info->finger[TouchID].minor,
							location, info->touch_count, info->finger[TouchID].ttype,
							info->finger[TouchID].palm_count, info->finger[TouchID].noise_level,
							info->finger[TouchID].max_strength, info->finger[TouchID].hover_id_num);
#endif
				} else if (info->finger[TouchID].action == FTS_COORDINATE_ACTION_MOVE) {
					if (info->touch_count == 0) {
						input_err(true, &info->client->dev, "%s: touch count 0\n", __func__);
						fts_release_all_finger(info);
						break;
					}

					if (prev_action == FTS_COORDINATE_ACTION_NONE) {
						input_err(true, &info->client->dev,
								"%s: previous state is released but point is moved\n",
								__func__);
						break;
					}

					input_mt_slot(info->input_dev, TouchID);
					input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 1);
					input_report_key(info->input_dev, BTN_TOUCH, 1);
					input_report_key(info->input_dev, BTN_TOOL_FINGER, 1);

					input_report_abs(info->input_dev, ABS_MT_POSITION_X, info->finger[TouchID].x);
					input_report_abs(info->input_dev, ABS_MT_POSITION_Y, info->finger[TouchID].y);
					input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR,
								info->finger[TouchID].major);
					input_report_abs(info->input_dev, ABS_MT_TOUCH_MINOR,
								info->finger[TouchID].minor);

					if (info->brush_mode)
						input_report_abs(info->input_dev, ABS_MT_CUSTOM,
									(info->finger[TouchID].z << 1) |
									info->finger[TouchID].palm);
					else
						input_report_abs(info->input_dev, ABS_MT_CUSTOM,
									(BRUSH_Z_DATA << 1) |
									info->finger[TouchID].palm);

					if (info->board->support_mt_pressure)
						input_report_abs(info->input_dev, ABS_MT_PRESSURE,
									info->finger[TouchID].z);

					info->finger[TouchID].mcount++;
				} else {
					input_dbg(true, &info->client->dev,
							"%s: do not support coordinate action(%d)\n",
							__func__, info->finger[TouchID].action);
				}

				if (info->finger[TouchID].prev_ttype != info->finger[TouchID].ttype)
					input_info(true, &info->client->dev, "%s: tID:%d ttype(%c->%c) : %s\n",
							__func__, info->finger[TouchID].id,
							finger_mode[info->finger[TouchID].prev_ttype],
							finger_mode[info->finger[TouchID].ttype],
							info->finger[TouchID].action == FTS_COORDINATE_ACTION_PRESS ? "P" :
							info->finger[TouchID].action == FTS_COORDINATE_ACTION_MOVE ? "M" : "R");

			} else {
				input_dbg(true, &info->client->dev,
						"%s: do not support coordinate type(%d)\n",
						__func__, info->finger[TouchID].ttype);
			}

			break;
		case FTS_GESTURE_EVENT:
			p_gesture_status = (struct fts_gesture_status *)event_buff;
			input_info(true, &info->client->dev, "%s: [GESTURE] type:%X sf:%X id:%X | %X, %X, %X, %X\n",
				__func__, p_gesture_status->stype, p_gesture_status->sf, p_gesture_status->gesture_id,
				p_gesture_status->gesture_data_1, p_gesture_status->gesture_data_2,
				p_gesture_status->gesture_data_3, p_gesture_status->gesture_data_4);

			if (p_gesture_status->sf == FTS_GESTURE_SAMSUNG_FEATURE) {
				switch (p_gesture_status->stype) {
				case FTS_SPONGE_EVENT_SWIPE_UP:
					info->scrub_id = SPONGE_EVENT_TYPE_SPAY;
					input_info(true, &info->client->dev, "%s: SPAY\n", __func__);
					input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
					input_sync(info->input_dev);
					input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
					break;
				case FTS_SPONGE_EVENT_DOUBLETAP:
					if (p_gesture_status->gesture_id == FTS_SPONGE_EVENT_GESTURE_ID_AOD) {
						info->scrub_id = SPONGE_EVENT_TYPE_AOD_DOUBLETAB;
						info->scrub_x = (p_gesture_status->gesture_data_1 << 4) | (p_gesture_status->gesture_data_3 >> 4);
						info->scrub_y = (p_gesture_status->gesture_data_2 << 4) | (p_gesture_status->gesture_data_3 & 0x0F);

						input_info(true, &info->client->dev, "%s: AOD\n", __func__);
						input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
						input_sync(info->input_dev);
						input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
					} else if (p_gesture_status->gesture_id == FTS_SPONGE_EVENT_GESTURE_ID_DOUBLETAP_TO_WAKEUP) {
						input_report_key(info->input_dev, KEY_WAKEUP, 1);
						input_sync(info->input_dev);
						input_report_key(info->input_dev, KEY_WAKEUP, 0);
						input_info(true, &info->client->dev, "%s: DOUBLE TAP TO WAKEUP\n", __func__);
					}
					break;
				case FTS_SPONGE_EVENT_SINGLETAP:
					info->scrub_id = SPONGE_EVENT_TYPE_SINGLE_TAP;
					info->scrub_x = (p_gesture_status->gesture_data_1 << 4) | (p_gesture_status->gesture_data_3 >> 4);
					info->scrub_y = (p_gesture_status->gesture_data_2 << 4) | (p_gesture_status->gesture_data_3 & 0x0F);

					input_info(true, &info->client->dev, "%s: SINGLE TAP\n", __func__);
					input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
					input_sync(info->input_dev);
					input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
					break;
				case FTS_SPONGE_EVENT_PRESS:
					if (p_gesture_status->gesture_id == FTS_SPONGE_EVENT_GESTURE_ID_FOD_LONG ||
							p_gesture_status->gesture_id == FTS_SPONGE_EVENT_GESTURE_ID_FOD_NORMAL) {
						info->scrub_id = SPONGE_EVENT_TYPE_FOD;
						input_info(true, &info->client->dev, "%s: FOD %sPRESS\n",
								__func__, p_gesture_status->gesture_id ? "" : "LONG");
					} else if (p_gesture_status->gesture_id == FTS_SPONGE_EVENT_GESTURE_ID_FOD_RELEASE) {
						info->scrub_id = SPONGE_EVENT_TYPE_FOD_RELEASE;
						input_info(true, &info->client->dev, "%s: FOD RELEASE\n", __func__);
					} else if (p_gesture_status->gesture_id == FTS_SPONGE_EVENT_GESTURE_ID_FOD_OUT) {
						info->scrub_id = SPONGE_EVENT_TYPE_FOD_OUT;
						input_info(true, &info->client->dev, "%s: FOD OUT\n", __func__);
					} else {
						input_info(true, &info->client->dev, "%s: invalid id %d\n",
								__func__, p_gesture_status->gesture_id);
						break;
					}
					input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
					input_sync(info->input_dev);
					input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
					break;
				}
			}
			break;
		case FTS_VENDOR_EVENT: // just print message for debugging
			if (event_buff[1] == 0x01) {  // echo event
				input_info(true, &info->client->dev,
						"%s: echo event %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
						event_buff[0], event_buff[1], event_buff[2],
						event_buff[3], event_buff[4], event_buff[5],
						event_buff[6], event_buff[7]);
			} else {
				input_info(true, &info->client->dev,
						"%s: %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
						event_buff[0], event_buff[1], event_buff[2],
						event_buff[3], event_buff[4], event_buff[5],
						event_buff[6], event_buff[7]);
#if defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				/* do not reset when it is not ship build. because it need to be debugged */
				if ((event_buff[0] == FTS_EVENT_ERROR_REPORT) && (event_buff[1] <= 0x05 || (event_buff[1] >= 0x12 && event_buff[1] <= 0x17))) {
					input_err(true, &info->client->dev, "%s: abnormal error detected, do reset\n", __func__);
					if (!info->reset_is_on_going)
						schedule_delayed_work(&info->reset_work, msecs_to_jiffies(10));
				}
#endif
			}
			break;
		default:
			input_info(true, &info->client->dev,
					"%s: unknown event %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
					event_buff[0], event_buff[1], event_buff[2], event_buff[3],
					event_buff[4], event_buff[5], event_buff[6], event_buff[7]);
			break;
		}

		EventNum++;
		left_event_count--;
	} while (left_event_count >= 0);

	input_sync(info->input_dev);

	if (info->touch_count == 0 && info->tsp_temp_data_skip) {
		info->tsp_temp_data_skip = false;
		fts_set_temp(info, false);
		input_err(true, &info->client->dev, "%s: fts_set_temp, no touch\n", __func__);
	}

	return 0;
}


#ifdef CONFIG_VBUS_NOTIFIER
static int fts_vbus_notification(struct notifier_block *nb, unsigned long cmd, void *data)
{
	struct fts_ts_info *info = container_of(nb, struct fts_ts_info, vbus_nb);
	vbus_status_t vbus_type = *(vbus_status_t *)data;

	if (info->shutdown_is_on_going)
		return 0;

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
		info->charger_mode = FTS_CHARGER_MODE_WIRE_CHARGER;
		break;
	case STATUS_VBUS_LOW:
		info->charger_mode = FTS_CHARGER_MODE_NORMAL;
		break;
	default:
		goto out;
	}

	input_info(true, &info->client->dev, "%s: %sconnected\n",
			__func__, info->charger_mode == FTS_CHARGER_MODE_NORMAL ? "dis" : "");

	fts_charger_mode(info);

out:
	return 0;
}
#endif

/**
 * fts_interrupt_handler()
 *
 * Called by the kernel when an interrupt occurs (when the sensor
 * asserts the attention irq).
 *
 * This function is the ISR thread and handles the acquisition
 * and the reporting of finger data when the presence of fingers
 * is detected.
 */
static irqreturn_t fts_interrupt_handler(int irq, void *handle)
{
	struct fts_ts_info *info = handle;

	int ret;

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	if (fts_filter_interrupt(info) == IRQ_HANDLED) {
		ret = wait_for_completion_interruptible_timeout((&info->st_interrupt),
				msecs_to_jiffies(10 * MSEC_PER_SEC));
		return IRQ_HANDLED;
	}
#endif

	/* in LPM, waiting blsp block resume */
	if (info->fts_power_state == FTS_POWER_STATE_LOWPOWER) {
		input_dbg(true, &info->client->dev, "%s: run LPM interrupt handler\n", __func__);

		wake_lock_timeout(&info->wakelock, msecs_to_jiffies(3 * MSEC_PER_SEC));
		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(&info->resume_done, msecs_to_jiffies(3 * MSEC_PER_SEC));
		if (ret == 0) {
			input_err(true, &info->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			return IRQ_NONE;
		}

		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			return IRQ_NONE;
		}

		input_info(true, &info->client->dev, "%s: run LPM interrupt handler, %d\n", __func__, ret);
		/* run lpm interrupt handler */
	}

	mutex_lock(&info->eventlock);

	ret = fts_event_handler_type_b(info);

	mutex_unlock(&info->eventlock);

	return IRQ_HANDLED;
}

int fts_irq_enable(struct fts_ts_info *info,
		bool enable)
{
	int retval = 0;

	if (enable) {
		if (info->irq_enabled)
			return retval;

		retval = request_threaded_irq(info->irq, NULL,
				fts_interrupt_handler, info->board->irq_type,
				FTS_TS_DRV_NAME, info);
		if (retval < 0) {
			input_err(true, &info->client->dev,
					"%s: Failed to create irq thread %d\n",
					__func__, retval);
			return retval;
		}

		info->irq_enabled = true;
	} else {
		if (info->irq_enabled) {
			fts_interrupt_set(info, INT_DISABLE);
			free_irq(info->irq, info);
			info->irq_enabled = false;
		}
	}

	return retval;
}

static int fts_power_ctrl(void *data, bool on)
{
	struct fts_ts_info *info = (struct fts_ts_info *)data;
	const struct fts_i2c_platform_data *pdata = info->board;
	struct device *dev = &info->client->dev;
	struct regulator *regulator_dvdd = NULL;
	struct regulator *regulator_avdd = NULL;
	static bool enabled;
	int retval = 0;

	if (enabled == on)
		return retval;

	regulator_dvdd = regulator_get(NULL, pdata->regulator_dvdd);
	if (IS_ERR_OR_NULL(regulator_dvdd)) {
		input_err(true, dev, "%s: Failed to get %s regulator\n",
				__func__, pdata->regulator_dvdd);
		goto out;
	}

	regulator_avdd = regulator_get(NULL, pdata->regulator_avdd);
	if (IS_ERR_OR_NULL(regulator_avdd)) {
		input_err(true, dev, "%s: Failed to get %s regulator\n",
				__func__, pdata->regulator_avdd);
		goto out;
	}

	if (on) {
		retval = regulator_set_load(regulator_avdd, 100000);
		if (retval) {
			input_err(true, dev, "%s: Failed to set load avdd: %d\n", __func__, retval);
			goto out;
		}
		retval = regulator_enable(regulator_avdd);
		if (retval) {
			input_err(true, dev, "%s: Failed to enable avdd: %d\n", __func__, retval);
			regulator_disable(regulator_avdd);
			regulator_set_load(regulator_avdd, 0);
			goto out;
		}

		fts_delay(1);

		retval = regulator_enable(regulator_dvdd);
		if (retval) {
			input_err(true, dev, "%s: Failed to enable vdd: %d\n", __func__, retval);
			regulator_disable(regulator_dvdd);
			regulator_disable(regulator_avdd);
			regulator_set_load(regulator_avdd, 0);
			goto out;
		}

		if (!IS_ERR_OR_NULL(pdata->pins_default)) {
			retval = pinctrl_select_state(pdata->pinctrl, pdata->pins_default);
			if (retval < 0)
				input_err(true, dev, "%s: Failed to configure tsp_attn pin\n", __func__);
		}
		fts_delay(5);
	} else {
		regulator_disable(regulator_dvdd);
		regulator_disable(regulator_avdd);
		regulator_set_load(regulator_avdd, 0);

		if (!IS_ERR_OR_NULL(pdata->pins_sleep)) {
			retval = pinctrl_select_state(pdata->pinctrl, pdata->pins_sleep);
			if (retval < 0)
				input_err(true, dev, "%s: Failed to configure tsp_attn pin\n", __func__);
		}
	}

	enabled = on;

	input_err(true, dev, "%s: %s: avdd:%s, dvdd:%s\n", __func__, on ? "on" : "off",
			regulator_is_enabled(regulator_avdd) ? "on" : "off",
			regulator_is_enabled(regulator_dvdd) ? "on" : "off");

out:
	regulator_put(regulator_dvdd);
	regulator_put(regulator_avdd);

	return retval;
}

static int fts_parse_dt(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct fts_i2c_platform_data *pdata = dev->platform_data;
	struct device_node *np = dev->of_node;
	u32 coords[2];
	u32 px_zone[3] = { 0 };
	u32 ic_match_value;
	int retval = 0;
	int lcdtype = 0;
#if defined(CONFIG_EXYNOS_DECON_FB)
	int connected;
#endif

	pdata->tsp_icid = of_get_named_gpio(np, "stm,tsp-icid_gpio", 0);
	if (gpio_is_valid(pdata->tsp_icid)) {
		input_info(true, dev, "%s: TSP_ICID : %d\n", __func__, gpio_get_value(pdata->tsp_icid));
		if (of_property_read_u32(np, "stm,icid_match_value", &ic_match_value)) {
			input_err(true, dev, "%s: Failed to get icid match value\n", __func__);
			return -EINVAL;
		}

		input_err(true, dev, "%s: IC matched value : %d\n", __func__, ic_match_value);

		if (gpio_get_value(pdata->tsp_icid) != ic_match_value) {
			input_err(true, dev, "%s: Do not match TSP_ICID\n", __func__);
			return -EINVAL;
		}
	} else {
		input_err(true, dev, "%s: skipped to get tsp-icid gpio\n", __func__);
	}

	if (gpio_is_valid(pdata->tsp_icid)) {
		retval = gpio_request(pdata->tsp_icid, "TSP_ICID");
		if (retval)
			input_err(true, dev, "%s: Unable to request tsp_icid [%d]\n", __func__, pdata->tsp_icid);
	}

	pdata->tsp_id = of_get_named_gpio(np, "stm,tsp-id_gpio", 0);
	if (gpio_is_valid(pdata->tsp_id))
		input_info(true, dev, "%s: TSP_ID : %d\n", __func__, gpio_get_value(pdata->tsp_id));
	else
		input_err(true, dev, "%s: skipped to get tsp-id gpio\n", __func__);

	if (gpio_is_valid(pdata->tsp_id)) {
		retval = gpio_request(pdata->tsp_id, "TSP_ID");
		if (retval)
			input_err(true, dev, "%s: Unable to request tsp_id [%d]\n", __func__, pdata->tsp_id);
	}

	pdata->device_id = of_get_named_gpio(np, "stm,device_gpio", 0);
	if (gpio_is_valid(pdata->device_id))
		input_info(true, dev, "%s: Device ID : %d\n", __func__, gpio_get_value(pdata->device_id));
	else
		input_err(true, dev, "%s: skipped to get device-id gpio\n", __func__);

	pdata->irq_gpio = of_get_named_gpio(np, "stm,irq_gpio", 0);
	if (gpio_is_valid(pdata->irq_gpio)) {
		retval = gpio_request_one(pdata->irq_gpio, GPIOF_DIR_IN, "stm,tsp_int");
		if (retval) {
			input_err(true, dev, "%s: Unable to request tsp_int [%d]\n", __func__, pdata->irq_gpio);
			return -EINVAL;
		}
	} else {
		input_err(true, dev, "%s: Failed to get irq gpio\n", __func__);
		return -EINVAL;
	}
	client->irq = gpio_to_irq(pdata->irq_gpio);

	if (of_property_read_u32(np, "stm,irq_type", &pdata->irq_type)) {
		input_err(true, dev, "%s: Failed to get irq_type property\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_u32_array(np, "stm,max_coords", coords, 2)) {
		input_err(true, dev, "%s: Failed to get max_coords property\n", __func__);
		return -EINVAL;
	}
	pdata->max_x = coords[0];
	pdata->max_y = coords[1];

	if (of_property_read_u32_array(np, "stm,display_resolution", coords, 2)) {
		input_err(true, dev,
				"%s: display_resolution is not set. set to same as max_coords\n",
				__func__);
		pdata->display_x = pdata->max_x;
		pdata->display_y = pdata->max_y;
	} else {
		pdata->display_x = coords[0];
		pdata->display_y = coords[1];
	}

	if (of_property_read_string(np, "stm,regulator_dvdd", &pdata->regulator_dvdd)) {
		input_err(true, dev, "%s: Failed to get regulator_dvdd name property\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_string(np, "stm,regulator_avdd", &pdata->regulator_avdd)) {
		input_err(true, dev, "%s: Failed to get regulator_avdd name property\n", __func__);
		return -EINVAL;
	}
	pdata->power = fts_power_ctrl;

	/* Optional parmeters(those values are not mandatory)
	 * do not return error value even if fail to get the value
	 */
	if (gpio_is_valid(pdata->tsp_id))
		of_property_read_string_index(np, "stm,firmware_name", gpio_get_value(pdata->tsp_id),
						&pdata->firmware_name);
	else
		of_property_read_string_index(np, "stm,firmware_name", 0, &pdata->firmware_name);

	pdata->support_dex = of_property_read_bool(np, "support_dex_mode");

	of_property_read_u32(np, "stm,bringup", &pdata->bringup);

	pdata->enable_settings_aot = of_property_read_bool(np, "stm,enable_settings_aot");
	pdata->sync_reportrate_120 = of_property_read_bool(np, "sync-reportrate-120");
	pdata->support_open_short_test = of_property_read_bool(np, "support_open_short_test");
	pdata->support_mis_calibration_test = of_property_read_bool(np, "support_mis_calibration_test");

	pdata->enable_vbus_noti = of_property_read_bool(np, "stm,enable_vbus_noti");

	pdata->support_fod = of_property_read_bool(np, "stm,support_fod");

#ifdef CONFIG_SEC_FACTORY
	pdata->support_mt_pressure = true;
#endif

	pdata->chip_on_board = of_property_read_bool(np, "stm,chip_on_board");

	pdata->disable_vsync_scan = of_property_read_bool(np, "stm,disable_vsync_scan");

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	of_property_read_u32(np, "stm,ss_touch_num", &pdata->ss_touch_num);
	input_info(true, dev, "%s: ss_touch_num:%d\n", __func__, pdata->ss_touch_num);
#endif

#if defined(CONFIG_DISPLAY_SAMSUNG)
	lcdtype = get_lcd_attached("GET");
	if (lcdtype == 0xFFFFFF) {
		input_err(true, &client->dev, "%s: lcd is not attached\n", __func__);
		if (!pdata->chip_on_board)
			return -ENODEV;
	}
#endif

#if defined(CONFIG_EXYNOS_DECON_FB)
	connected = get_lcd_info("connected");
	if (connected < 0) {
		input_err(true, dev, "%s: Failed to get lcd info\n", __func__);
		if (!pdata->chip_on_board)
			return -EINVAL;
	}

	if (!connected) {
		input_err(true, &client->dev, "%s: lcd is disconnected\n", __func__);
		if (!pdata->chip_on_board)
			return -ENODEV;
	}

	input_info(true, &client->dev, "%s: lcd is connected\n", __func__);

	lcdtype = get_lcd_info("id");
	if (lcdtype < 0) {
		input_err(true, dev, "%s: Failed to get lcd info\n", __func__);
		if (!pdata->chip_on_board)
			return -EINVAL;
	}
#endif
	input_info(true, &client->dev, "%s: lcdtype 0x%08X\n", __func__, lcdtype);

	if (of_property_read_u32_array(np, "stm,area-size", px_zone, 3)) {
		input_info(true, &client->dev, "Failed to get zone's size\n");
		pdata->area_indicator = 48;
		pdata->area_navigation = 96;
		pdata->area_edge = 60;
	} else {
		pdata->area_indicator = px_zone[0];
		pdata->area_navigation = px_zone[1];
		pdata->area_edge = px_zone[2];
	}
	input_info(true, dev, "%s: zone's size - indicator:%d, navigation:%d, edge:%d\n",
			__func__, pdata->area_indicator, pdata->area_navigation, pdata->area_edge);

	input_err(true, dev,
			"%s: irq :%d, irq_type: 0x%04x, max[x,y]: [%d,%d], display:%d,%d,"
			" dex: %d, RR120Hz: %d, aot: %d%s, vbus: %d, fod:%d\n",
			__func__, pdata->irq_gpio, pdata->irq_type, pdata->max_x, pdata->max_y,
			pdata->display_x, pdata->display_y, pdata->support_dex,
			pdata->sync_reportrate_120, pdata->enable_settings_aot,
			pdata->chip_on_board ? ", COB type" : "", pdata->enable_vbus_noti,
			pdata->support_fod);

	return retval;
}

#ifdef TCLM_CONCEPT
static void sec_tclm_parse_dt(struct i2c_client *client, struct sec_tclm_data *tdata)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "stm,tclm_level", &tdata->tclm_level) < 0) {
		tdata->tclm_level = 0;
		input_err(true, dev, "%s: Failed to get tclm_level property\n", __func__);
	}

	if (of_property_read_u32(np, "stm,afe_base", &tdata->afe_base) < 0) {
		tdata->afe_base = 0;
		input_err(true, dev, "%s: Failed to get afe_base property\n", __func__);
	}

	tdata->support_tclm_test = of_property_read_bool(np, "support_tclm_test_only_for_debug_do_not_submit");

	input_err(true, &client->dev, "%s: tclm_level %d, sec_afe_base %d\n", __func__, tdata->tclm_level, tdata->afe_base);
}
#endif

static int fts_setup_drv_data(struct i2c_client *client)
{
	int retval = 0;
	struct fts_i2c_platform_data *pdata;
	struct fts_ts_info *info;
	struct sec_tclm_data *tdata = NULL;

	/* parse dt */
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct fts_i2c_platform_data), GFP_KERNEL);

		if (!pdata) {
			input_err(true, &client->dev, "%s: Failed to allocate platform data\n", __func__);
			return -ENOMEM;
		}

		client->dev.platform_data = pdata;
		retval = fts_parse_dt(client);
		if (retval) {
			input_err(true, &client->dev, "%s: Failed to parse dt\n", __func__);
			return retval;
		}

		tdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_tclm_data), GFP_KERNEL);
		if (!tdata)
			return -ENOMEM;

#ifdef TCLM_CONCEPT
		sec_tclm_parse_dt(client, tdata);
#endif
	} else {
		pdata = client->dev.platform_data;
	}

	if (!pdata) {
		input_err(true, &client->dev, "%s: No platform data found\n", __func__);
		return -EINVAL;
	}
	if (!pdata->power) {
		input_err(true, &client->dev, "%s: No power contorl found\n", __func__);
		return -EINVAL;
	}

	pdata->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(pdata->pinctrl)) {
		input_err(true, &client->dev, "%s: could not get pinctrl\n", __func__);
		return PTR_ERR(pdata->pinctrl);
	}

	pdata->pins_default = pinctrl_lookup_state(pdata->pinctrl, "on_state");
	if (IS_ERR(pdata->pins_default))
		input_err(true, &client->dev, "%s: could not get default pinstate\n", __func__);

	pdata->pins_sleep = pinctrl_lookup_state(pdata->pinctrl, "off_state");
	if (IS_ERR(pdata->pins_sleep))
		input_err(true, &client->dev, "%s: could not get sleep pinstate\n", __func__);

	info = kzalloc(sizeof(struct fts_ts_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->client = client;
	info->board = pdata;
	info->irq = client->irq;
	info->irq_type = info->board->irq_type;
	info->irq_enabled = false;
	info->stop_device = fts_stop_device;
	info->start_device = fts_start_device;
	info->fts_command = fts_command;
	info->fts_read_reg = fts_read_reg;
	info->fts_write_reg = fts_write_reg;
	info->fts_systemreset = fts_systemreset;
	info->fts_get_version_info = fts_get_version_info;
	info->fts_get_sysinfo_data = fts_get_sysinfo_data;
	info->fts_wait_for_ready = fts_wait_for_ready;
	info->fts_read_from_sponge = fts_read_from_sponge;
	info->fts_write_to_sponge = fts_write_to_sponge;

	info->tdata = tdata;
	if (!info->tdata) {
		input_err(true, &client->dev, "%s: No tclm data found\n", __func__);
		kfree(info);
		return -EINVAL;
	}

#ifdef TCLM_CONCEPT
	sec_tclm_initialize(info->tdata);
	info->tdata->client = info->client;
	info->tdata->tclm_read = fts_tclm_data_read;
	info->tdata->tclm_write = fts_tclm_data_write;
	info->tdata->tclm_execute_force_calibration = fts_tclm_execute_force_calibration;
	info->tdata->tclm_parse_dt = sec_tclm_parse_dt;
#endif

#ifdef USE_OPEN_DWORK
	INIT_DELAYED_WORK(&info->open_work, fts_open_work);
#endif
	info->delay_time = 300;
	INIT_DELAYED_WORK(&info->reset_work, fts_reset_work);
	INIT_DELAYED_WORK(&info->work_read_info, fts_read_info_work);
	INIT_DELAYED_WORK(&info->work_print_info, fts_print_info_work);

	i2c_set_clientdata(client, info);

	if (pdata->get_ddi_type) {
		info->ddi_type = pdata->get_ddi_type();
		input_info(true, &client->dev, "%s: DDI Type is %s[%d]\n",
				__func__, info->ddi_type ? "MAGNA" : "SDC", info->ddi_type);
	}

	return retval;
}

static void fts_set_input_prop(struct fts_ts_info *info, struct input_dev *dev, u8 propbit)
{
	static char fts_ts_phys[64] = { 0 };

	dev->dev.parent = &info->client->dev;

	snprintf(fts_ts_phys, sizeof(fts_ts_phys), "%s/input1",
			dev->name);
	dev->phys = fts_ts_phys;
	dev->id.bustype = BUS_I2C;

	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_KEY, dev->evbit);
	set_bit(EV_ABS, dev->evbit);
	set_bit(propbit, dev->propbit);
	set_bit(BTN_TOUCH, dev->keybit);
	set_bit(BTN_TOOL_FINGER, dev->keybit);
	set_bit(KEY_BLACK_UI_GESTURE, dev->keybit);
	set_bit(KEY_WAKEUP, dev->keybit);

	input_set_abs_params(dev, ABS_MT_POSITION_X,
			0, info->board->max_x, 0, 0);
	input_set_abs_params(dev, ABS_MT_POSITION_Y,
			0, info->board->max_y, 0, 0);
#ifdef CONFIG_SEC_FACTORY
	input_set_abs_params(dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
#else
	if (info->board->support_mt_pressure)
		input_set_abs_params(dev, ABS_MT_PRESSURE, 0, 10000, 0, 0);
#endif

	input_set_abs_params(dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MINOR, 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);

	if (propbit == INPUT_PROP_POINTER)
		input_mt_init_slots(dev, FINGER_MAX, INPUT_MT_POINTER);
	else
		input_mt_init_slots(dev, FINGER_MAX, INPUT_MT_DIRECT);

	input_set_drvdata(dev, info);
}

static int fts_input_notify_call(struct notifier_block *n, unsigned long data, void *v)
{
	struct fts_ts_info *info = container_of(n, struct fts_ts_info, fts_input_nb);
	int ret = 0;
	u8 regAdd[3] = { FTS_CMD_SET_FUNCTION_ONOFF, FTS_FUNCTION_SET_HOVER_DETECTION, 0 };

	switch (data) {
	case NOTIFIER_WACOM_PEN_HOVER_IN:
		regAdd[2] = 1;
		ret = fts_write_reg(info, regAdd, 3);
		input_info(true, &info->client->dev, "%s: pen hover in detect, ret=%d\n", __func__, ret);
		break;
	case NOTIFIER_WACOM_PEN_HOVER_OUT:
		regAdd[2] = 0;
		ret = fts_write_reg(info, regAdd, 3);
		input_info(true, &info->client->dev, "%s: pen hover out detect, ret=%d\n", __func__, ret);
		break;
	default:
		break;
	}
	return ret;
}

static int fts_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
	int retval;
	struct fts_ts_info *info = NULL;
	int i = 0;

	input_info(true, &client->dev, "%s: FTS Driver [70%s]\n", __func__,
			FTS_TS_DRV_VERSION);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev, "%s: FTS err = EIO!\n", __func__);
		return -EIO;
	}
#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		input_err(true, &client->dev, "%s: Do not load driver due to : lpm %d\n",
				__func__, lpcharge);
		return -ENODEV;
	}
#endif
	/* Build up driver data */
	retval = fts_setup_drv_data(client);
	if (retval < 0) {
		input_err(true, &client->dev, "%s: Failed to set up driver data\n", __func__);
		goto err_setup_drv_data;
	}

	info = (struct fts_ts_info *)i2c_get_clientdata(client);
	if (!info) {
		input_err(true, &client->dev, "%s: Failed to get driver data\n", __func__);
		retval = -ENODEV;
		goto err_get_drv_data;
	}
	i2c_set_clientdata(client, info);

	info->probe_done = false;

	if (info->board->power)
		info->board->power(info, true);
	info->fts_power_state = FTS_POWER_STATE_ACTIVE;

	info->input_dev = input_allocate_device();
	if (!info->input_dev) {
		input_err(true, &info->client->dev, "%s: Failed to alloc input_dev\n", __func__);
		retval = -ENOMEM;
		goto err_input_allocate_device;
	}

	if (info->board->support_dex) {
		info->input_dev_pad = input_allocate_device();
		if (!info->input_dev_pad) {
			input_err(true, &info->client->dev, "%s: Failed to alloc input_dev\n", __func__);
			retval = -ENOMEM;
			goto err_input_pad_allocate_device;
		}
	}

	mutex_init(&info->device_mutex);
	mutex_init(&info->i2c_mutex);
	mutex_init(&info->irq_mutex);
	mutex_init(&info->eventlock);
	mutex_init(&info->sponge_mutex);
	mutex_init(&info->wait_for);

	retval = fts_init(info);
	if (retval < 0) {
		input_err(true, &info->client->dev, "%s: fts_init fail!\n", __func__);
		goto err_fts_init;
	}

	fts_init_proc(info);

	mutex_lock(&info->device_mutex);
	info->reinit_done = true;
	mutex_unlock(&info->device_mutex);

	wake_lock_init(&info->wakelock, WAKE_LOCK_SUSPEND, "tsp_wakelock");
	init_completion(&info->resume_done);
	complete_all(&info->resume_done);

	if (info->board->support_dex) {
		info->input_dev_pad->name = "sec_touchpad";
		fts_set_input_prop(info, info->input_dev_pad, INPUT_PROP_POINTER);
	}

	info->input_dev->name = "sec_touchscreen";
	fts_set_input_prop(info, info->input_dev, INPUT_PROP_DIRECT);
#ifdef USE_OPEN_CLOSE
	info->input_dev->open = fts_input_open;
	info->input_dev->close = fts_input_close;
#endif
	info->input_dev_touch = info->input_dev;

	retval = input_register_device(info->input_dev);
	if (retval) {
		input_err(true, &info->client->dev, "%s: input_register_device fail!\n", __func__);
		goto err_register_input;
	}
	if (info->board->support_dex) {
		retval = input_register_device(info->input_dev_pad);
		if (retval) {
			input_err(true, &info->client->dev, "%s: input_register_device fail!\n", __func__);
			goto err_register_input_pad;
		}
	}

	for (i = 0; i < FINGER_MAX; i++) {
		info->finger[i].action = FTS_COORDINATE_ACTION_NONE;
		info->finger[i].mcount = 0;
	}

	retval = fts_irq_enable(info, true);
	if (retval < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to enable attention interrupt\n",
				__func__);
		goto err_enable_irq;
	}

	retval = sec_cmd_init(&info->sec, ft_commands,
			ARRAY_SIZE(ft_commands), SEC_CLASS_DEVT_TSP);
	if (retval < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to sec_cmd_init\n", __func__);
		retval = -ENODEV;
		goto err_sec_cmd;
	}

	retval = sysfs_create_group(&info->sec.fac_dev->kobj,
			&sec_touch_factory_attr_group);
	if (retval < 0) {
		input_err(true, &info->client->dev, "%s: Failed to create sysfs group\n", __func__);
		goto err_sysfs;
	}

	retval = sysfs_create_link(&info->sec.fac_dev->kobj,
			&info->input_dev->dev.kobj, "input");
	if (retval < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to create link\n", __func__);
		goto err_sysfs;
	}

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	for (i = 0; i < (int)ARRAY_SIZE(attrs); i++) {
		retval = sysfs_create_file(&info->input_dev->dev.kobj,
				&attrs[i].attr);
		if (retval < 0) {
			input_err(true, &info->client->dev,
					"%s: Failed to create sysfs attributes\n",
					__func__);
		}
	}

	fts_secure_touch_init(info);
#endif

	device_init_wakeup(&client->dev, true);

	fts_check_custom_library(info);

	schedule_delayed_work(&info->work_read_info, msecs_to_jiffies(100));

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	dump_callbacks.inform_dump = tsp_dump;
	INIT_DELAYED_WORK(&info->debug_work, dump_tsp_rawdata);
	p_debug_work = &info->debug_work;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	info->ss_drv = sec_secure_touch_register(info, info->board->ss_touch_num, &info->input_dev->dev.kobj);
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	if (info->board->enable_vbus_noti) {
		retval = vbus_notifier_register(&info->vbus_nb,
						fts_vbus_notification, VBUS_NOTIFY_DEV_CHARGER);
		if (retval < 0)
			input_err(true, &client->dev, "%s: vbus notifier register failed %d\n",
					__func__, retval);
	}
#endif

	sec_input_register_notify(&info->fts_input_nb, fts_input_notify_call, 1);

	info->probe_done = true;
	input_info(true, &info->client->dev, "%s: done\n", __func__);
	input_log_fix();

	return 0;

err_sysfs:
	sec_cmd_exit(&info->sec, SEC_CLASS_DEVT_TSP);
err_sec_cmd:
	if (info->irq_enabled)
		fts_irq_enable(info, false);
err_enable_irq:
	if (info->board->support_dex) {
		input_unregister_device(info->input_dev_pad);
		info->input_dev_pad = NULL;
	}
err_register_input_pad:
	input_unregister_device(info->input_dev);
	info->input_dev = NULL;
	info->input_dev_touch = NULL;
err_register_input:
	wake_lock_destroy(&info->wakelock);

	kfree(info->ito_result);
	kfree(info->cx_data);
	kfree(info->pFrame);
err_fts_init:
	mutex_destroy(&info->sponge_mutex);
	mutex_destroy(&info->device_mutex);
	mutex_destroy(&info->i2c_mutex);
	if (info->board->support_dex) {
		if (info->input_dev_pad)
			input_free_device(info->input_dev_pad);
	}
err_input_pad_allocate_device:
	if (info->input_dev)
		input_free_device(info->input_dev);
err_input_allocate_device:
	if (info->board->power)
		info->board->power(info, false);
	if (gpio_is_valid(info->board->irq_gpio))
		gpio_free(info->board->irq_gpio);

	g_fts_info = NULL;
	kfree(info);
err_get_drv_data:
err_setup_drv_data:
	input_err(true, &client->dev, "%s: failed(%d)\n", __func__, retval);
	input_log_fix();
	return retval;
}

static int fts_remove(struct i2c_client *client)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);
#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	int i = 0;
#endif

	input_info(true, &info->client->dev, "%s\n", __func__);
	info->shutdown_is_on_going = true;

	sec_input_unregister_notify(&info->fts_input_nb);

	disable_irq_nosync(info->client->irq);
	free_irq(info->client->irq, info);

	cancel_delayed_work_sync(&info->work_print_info);
	cancel_delayed_work_sync(&info->work_read_info);
	cancel_delayed_work_sync(&info->reset_work);
	cancel_delayed_work_sync(&info->work_print_info);

	wake_lock_destroy(&info->wakelock);

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	for (i = 0; i < (int)ARRAY_SIZE(attrs); i++) {
		sysfs_remove_file(&info->input_dev->dev.kobj,
				&attrs[i].attr);
	}
#endif

	sysfs_remove_link(&info->sec.fac_dev->kobj, "input");
	sysfs_remove_group(&info->sec.fac_dev->kobj,
			&sec_touch_factory_attr_group);
	sec_cmd_exit(&info->sec, SEC_CLASS_DEVT_TSP);

	kfree(info->ito_result);
	kfree(info->cx_data);
	kfree(info->pFrame);

#ifdef CONFIG_VBUS_NOTIFIER
	if (info->board->enable_vbus_noti)
		vbus_notifier_unregister(&info->vbus_nb);
#endif

	if (info->board->support_dex) {
		input_mt_destroy_slots(info->input_dev_pad);
		input_unregister_device(info->input_dev_pad);
	}
	info->input_dev_pad = NULL;

	info->input_dev = info->input_dev_touch;
	input_mt_destroy_slots(info->input_dev);
	input_unregister_device(info->input_dev);
	info->input_dev = NULL;
	info->input_dev_touch = NULL;

	if (info->board->power)
		info->board->power(info, false);
	info->shutdown_is_on_going = false;

	g_fts_info = NULL;
	kfree(info);

	return 0;
}

#ifdef USE_OPEN_CLOSE
#ifdef USE_OPEN_DWORK
static void fts_open_work(struct work_struct *work)
{
	int retval;
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
			open_work.work);

	input_info(true, &info->client->dev, "%s\n", __func__);

	retval = fts_start_device(info);
	if (retval < 0)
		input_err(true, &info->client->dev,
				"%s: Failed to start device\n", __func__);
}
#endif
static int fts_input_open(struct input_dev *dev)
{
	struct fts_ts_info *info = input_get_drvdata(dev);
	int retval;

	if (!info->probe_done) {
		input_dbg(true, &info->client->dev, "%s: not probe\n", __func__);
		goto out;
	}

	if (!info->info_work_done) {
		input_err(true, &info->client->dev, "%s: not finished info work\n", __func__);
		goto out;
	}

	input_dbg(false, &info->client->dev, "%s\n", __func__);

#ifdef USE_OPEN_DWORK
	schedule_delayed_work(&info->open_work,
			msecs_to_jiffies(TOUCH_OPEN_DWORK_TIME));
#else
	retval = fts_start_device(info);
	if (retval < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to start device\n", __func__);
		goto out;
	}
#endif

	if (info->fix_active_mode)
		fts_fix_active_mode(info, true);

	fts_set_temp(info, true);
out:
	cancel_delayed_work(&info->work_print_info);
	info->print_info_cnt_open = 0;
	info->print_info_cnt_release = 0;
	schedule_work(&info->work_print_info.work);

	return 0;
}

static void fts_input_close(struct input_dev *dev)
{
	struct fts_ts_info *info = input_get_drvdata(dev);

	if (!info->probe_done || info->shutdown_is_on_going) {
		input_dbg(false, &info->client->dev, "%s: not probe\n", __func__);
		return;
	}

	if (!info->info_work_done) {
		input_err(true, &info->client->dev, "%s: not finished info work\n", __func__);
		return;
	}

	input_dbg(false, &info->client->dev, "%s\n", __func__);

#ifdef USE_OPEN_DWORK
	cancel_delayed_work(&info->open_work);
#endif
	cancel_delayed_work(&info->reset_work);
	cancel_delayed_work(&info->work_print_info);

	if (info->touch_aging_mode) {
		info->touch_aging_mode = false;
		fts_reinit(info, false);
	}

	fts_print_info(info);

	if (info->prox_power_off)
		fts_stop_device(info, 0);
	else
		fts_stop_device(info, info->lowpower_flag || info->fod_lp_mode);
	info->prox_power_off = 0;
	info->fw_corruption = false;
	info->osc_trim_error = false;
}
#endif

void fts_reinit(struct fts_ts_info *info, bool delay)
{
	u8 retry = 3;
	int rc = 0;

	if (delay) {
		rc = fts_wait_for_ready(info);
		if (rc < 0) {
			input_err(true, &info->client->dev, "%s: Failed to wait for ready\n", __func__);
			return;
		}
		rc = fts_read_chip_id(info);
		if (rc < 0) {
			input_err(true, &info->client->dev, "%s: Failed to read chip id\n", __func__);
			return;
		}
	}

	do {
		rc = fts_systemreset(info, 0);
		if (rc < 0)
			fts_reset(info, 20);
		else
			break;
	} while (--retry);

	if (retry == 0) {
		input_err(true, &info->client->dev, "%s: Failed to system reset\n", __func__);
		return;
	}

	fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);
	fts_release_all_finger(info);

	fts_set_cover_type(info, info->flip_enable);

	if (info->charger_mode)
		fts_charger_mode(info);

	/* need to add new command (dex mode ~ touchable area) */
	fts_set_external_noise_mode(info, EXT_NOISE_MODE_MAX);

	fts_set_fod_rect(info);

	if (info->brush_mode) {
		u8 regAdd[3] = {FTS_CMD_SET_FUNCTION_ONOFF, FTS_FUNCTION_ENABLE_BRUSH_MODE, info->brush_mode};

		input_info(true, &info->client->dev, "%s: set brush mode\n", __func__);
		if (fts_write_reg(info, regAdd, 3) < 0)
			input_err(true, &info->client->dev, "%s: brush_enable failed\n", __func__);
	}

	if (info->touchable_area) {
		u8 regAdd[3] = {FTS_CMD_SET_FUNCTION_ONOFF, FTS_FUNCTION_SET_TOUCHABLE_AREA, info->touchable_area};

		input_info(true, &info->client->dev, "%s: set 16:9 mode\n", __func__);
		if (fts_write_reg(info, regAdd, 3) < 0)
			input_err(true, &info->client->dev, "%s: set_touchable_area failed\n", __func__);
	}

	/* because edge and dead zone will recover soon */
	fts_set_grip_type(info, ONLY_EDGE_HANDLER);

	fts_delay(50);

	if (!info->flip_enable)
		fts_set_scanmode(info, info->scan_mode);
	else
		fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);
}

void fts_release_all_finger(struct fts_ts_info *info)
{
	int i;

	for (i = 0; i < FINGER_MAX; i++) {
		input_mt_slot(info->input_dev, i);

		if (info->board->support_mt_pressure)
			input_report_abs(info->input_dev, ABS_MT_PRESSURE, 0);

		input_report_abs(info->input_dev, ABS_MT_CUSTOM, 0);

		input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 0);

		if ((info->finger[i].action == FTS_COORDINATE_ACTION_PRESS) ||
				(info->finger[i].action == FTS_COORDINATE_ACTION_MOVE)) {
			input_info(true, &info->client->dev,
					"[RA] tID:%d mc:%d tc:%d\n",
					i, info->finger[i].mcount, info->touch_count);
		}

		info->finger[i].action = FTS_COORDINATE_ACTION_NONE;
		info->finger[i].mcount = 0;
		info->finger[i].palm_count = 0;
		info->finger[i].noise_level = 0;
		info->finger[i].max_strength = 0;
		info->finger[i].hover_id_num = 0;
	}

	info->touch_count = 0;

	input_report_key(info->input_dev, BTN_TOUCH, 0);
	input_report_key(info->input_dev, BTN_TOOL_FINGER, 0);

	input_sync(info->input_dev);

	info->check_multi = 0;
}

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
static void dump_tsp_rawdata(struct work_struct *work)
{
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
			debug_work.work);
	int i;

	if (info->rawdata_read_lock) {
		input_err(true, &info->client->dev, "%s: ignored ## already checking..\n", __func__);
		return;
	}

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: ignored ## IC is power off\n", __func__);
		return;
	}

	info->rawdata_read_lock = true;
	input_info(true, &info->client->dev, "%s: ## run CX data ##, %d\n", __func__, __LINE__);
	run_cx_data_read((void *)&info->sec);

	for (i = 0; i < 5; i++) {
		input_info(true, &info->client->dev, "%s: ## run Raw Cap data ##, %d\n", __func__, __LINE__);
		run_rawcap_read((void *)&info->sec);

		input_info(true, &info->client->dev, "%s: ## run Delta ##, %d\n", __func__, __LINE__);
		run_delta_read((void *)&info->sec);
		fts_delay(50);
	}
	input_info(true, &info->client->dev, "%s: ## Done ##, %d\n", __func__, __LINE__);

	info->rawdata_read_lock = false;
}

static void tsp_dump(void)
{
#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge)
		return;
#endif
	if (!p_debug_work)
		return;
	pr_err("%s: %s %s: start\n", FTS_TS_DRV_NAME, SECLOG, __func__);
	schedule_delayed_work(p_debug_work, msecs_to_jiffies(100));
}
#endif

static void fts_reset(struct fts_ts_info *info, unsigned int ms)
{
	input_info(true, &info->client->dev, "%s: Recover IC, discharge time:%d\n", __func__, ms);

	fts_interrupt_set(info, INT_DISABLE);

	if (info->board->power)
		info->board->power(info, false);

	fts_delay(ms);

	if (info->board->power)
		info->board->power(info, true);

	fts_delay(5);

	fts_interrupt_set(info, INT_ENABLE);
}

static void fts_reset_work(struct work_struct *work)
{
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
			reset_work.work);

	if (info->debug_string & FTS_DEBUG_SEND_UEVENT)
		sec_cmd_send_event_to_user(&info->sec, NULL, "RESULT=RESET");

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev, "%s: secure touch enabled\n",
				__func__);
		return;
	}
#endif

	input_info(true, &info->client->dev, "%s: Call Power-Off to recover IC\n", __func__);
	info->reset_is_on_going = true;

	wake_lock(&info->wakelock);

	fts_stop_device(info, false);

	msleep(100);	/* Delay to discharge the IC from ESD or On-state.*/
	if (fts_start_device(info) < 0)
		input_err(true, &info->client->dev, "%s: Failed to start device\n", __func__);

	if (info->input_dev_touch->disabled) {
		input_err(true, &info->client->dev, "%s: call input_close\n", __func__);

		fts_stop_device(info, info->lowpower_flag || info->fod_lp_mode);

		if (info->lowpower_flag & FTS_MODE_AOD)
			fts_set_aod_rect(info);
	}
	fts_set_press_property(info);
	fts_set_fod_finger_merge(info);

	info->reset_is_on_going = false;
	wake_unlock(&info->wakelock);
}

static void fts_read_info_work(struct work_struct *work)
{
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
			work_read_info.work);
	short minval = 0x7FFF;
	short maxval = 0x8000;
	int ret;

	input_info(true, &info->client->dev, "%s\n", __func__);
#ifdef TCLM_CONCEPT
	ret = sec_tclm_check_cal_case(info->tdata);
	input_info(true, &info->client->dev, "%s: sec_tclm_check_cal_case ret: %d\n", __func__, ret);
#endif

	ret = fts_get_tsp_test_result(info);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed to get result\n",
				__func__);

	input_raw_info(true, &info->client->dev, "%s: fac test result %02X\n",
				__func__, info->test_result.data[0]);

	fts_panel_ito_test(info, OPEN_SHORT_CRACK_TEST);

	fts_read_frame(info, TYPE_BASELINE_DATA, &minval, &maxval);

	fts_get_cmoffset_dump(info, info->cmoffset_sdc_proc, OFFSET_FW_SDC);
	fts_get_cmoffset_dump(info, info->cmoffset_sub_proc, OFFSET_FW_SUB);
	fts_get_cmoffset_dump(info, info->cmoffset_main_proc, OFFSET_FW_MAIN);
	fts_get_hf_data(info);
	fts_get_miscal_data(info, &minval, &maxval);

	info->info_work_done = true;

	schedule_work(&info->work_print_info.work);

	input_info(true, &info->client->dev, "%s done\n", __func__);
}

void fts_set_utc_sponge(struct fts_ts_info *info)
{
	struct timeval current_time;
	u8 data[4];

	do_gettimeofday(&current_time);
	data[0] = (u8)(current_time.tv_sec >> 0 & 0xFF);
	data[1] = (u8)(current_time.tv_sec >> 8 & 0xFF);
	data[2] = (u8)(current_time.tv_sec >> 16 & 0xFF);
	data[3] = (u8)(current_time.tv_sec >> 24 & 0xFF);

	fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_UTC, data, sizeof(data));
}

static int fts_stop_device(struct fts_ts_info *info, bool lpmode)
{
	input_info(true, &info->client->dev, "%s\n", __func__);

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	fts_secure_touch_stop(info, 1);
#endif
	mutex_lock(&info->device_mutex);

#ifdef TCLM_CONCEPT
	sec_tclm_debug_info(info->tdata);
#endif

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: already power off\n", __func__);
		goto out;
	}

	info->touch_noise_status = 0;
	info->touch_noise_reason = 0;
	info->wet_mode = 0;

	if (lpmode) {
		input_info(true, &info->client->dev, "%s: lowpower flag:0x%02X\n", __func__, info->lowpower_flag);

		if (info->fix_active_mode)
			fts_fix_active_mode(info, false);

		if (device_may_wakeup(&info->client->dev))
			enable_irq_wake(info->irq);

		fts_release_all_finger(info);

		fts_set_opmode(info, FTS_OPMODE_LOWPOWER);
		if (info->flip_enable)
			fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);

		info->fts_power_state = FTS_POWER_STATE_LOWPOWER;

		fts_set_utc_sponge(info);

		info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
				&info->lowpower_flag, sizeof(info->lowpower_flag));
	} else {
		fts_interrupt_set(info, INT_DISABLE);
		fts_release_all_finger(info);

		info->fts_power_state = FTS_POWER_STATE_POWERDOWN;

		if (info->board->power)
			info->board->power(info, false);
	}
out:

	mutex_unlock(&info->device_mutex);
	return 0;
}

static int fts_start_device(struct fts_ts_info *info)
{
	input_info(true, &info->client->dev, "%s%s\n",
			__func__, info->fts_power_state ? ": exit low power mode" : "");

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	fts_secure_touch_stop(info, 1);
#endif
	mutex_lock(&info->device_mutex);

	if (info->fts_power_state == FTS_POWER_STATE_ACTIVE) {
		input_err(true, &info->client->dev, "%s: already power on\n", __func__);
		goto out;
	}

	fts_release_all_finger(info);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		if (info->board->power)
			info->board->power(info, true);

		info->reinit_done = false;
		info->fts_power_state = FTS_POWER_STATE_ACTIVE;
		fts_reinit(info, true);
		info->reinit_done = true;
	} else {	/* FTS_POWER_STATE_LOWPOWER */
		int ret;

		ret = fts_set_opmode(info, FTS_OPMODE_NORMAL);
		if (ret < 0) {
			info->reinit_done = false;
			fts_reinit(info, false);
			info->reinit_done = true;
		}

		if (device_may_wakeup(&info->client->dev))
			disable_irq_wake(info->irq);
	}
	info->fts_power_state = FTS_POWER_STATE_ACTIVE;

	info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
			&info->lowpower_flag, sizeof(info->lowpower_flag));

out:
	mutex_unlock(&info->device_mutex);

	fts_charger_mode(info);

	return 0;
}

static void fts_shutdown(struct i2c_client *client)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);

	input_info(true, &info->client->dev, "%s\n", __func__);

	fts_remove(client);
}

#ifdef CONFIG_PM
static int fts_pm_suspend(struct device *dev)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);

	input_dbg(true, &info->client->dev, "%s\n", __func__);

	reinit_completion(&info->resume_done);

	return 0;
}

static int fts_pm_resume(struct device *dev)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);

	input_dbg(true, &info->client->dev, "%s\n", __func__);

	complete_all(&info->resume_done);

	return 0;
}
#endif

#if (!defined(CONFIG_PM)) && !defined(USE_OPEN_CLOSE)
static int fts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);

	input_dbg(true, &info->client->dev, "%s\n", __func__);

	fts_stop_device(info, info->lowpower_flag || info->fod_lp_mode);

	return 0;
}

static int fts_resume(struct i2c_client *client)
{

	struct fts_ts_info *info = i2c_get_clientdata(client);

	input_dbg(true, &info->client->dev, "%s\n", __func__);

	fts_start_device(info);

	return 0;
}
#endif

static const struct i2c_device_id fts_device_id[] = {
	{FTS_TS_DRV_NAME, 0},
	{}
};

#ifdef CONFIG_PM
static const struct dev_pm_ops fts_dev_pm_ops = {
	.suspend = fts_pm_suspend,
	.resume = fts_pm_resume,
};
#endif

#ifdef CONFIG_OF
static const struct of_device_id fts_match_table[] = {
	{.compatible = "stm,fts_touch",},
	{},
};
#else
#define fts_match_table NULL
#endif

static struct i2c_driver fts_i2c_driver = {
	.driver = {
		.name = FTS_TS_DRV_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = fts_match_table,
#endif
#ifdef CONFIG_PM
		.pm = &fts_dev_pm_ops,
#endif
	},
	.probe = fts_probe,
	.remove = fts_remove,
	.shutdown = fts_shutdown,
#if (!defined(CONFIG_PM)) && !defined(USE_OPEN_CLOSE)
	.suspend = fts_suspend,
	.resume = fts_resume,
#endif
	.id_table = fts_device_id,
};

static int __init fts_driver_init(void)
{
	return i2c_add_driver(&fts_i2c_driver);
}

static void __exit fts_driver_exit(void)
{
	i2c_del_driver(&fts_i2c_driver);
}

MODULE_DESCRIPTION("STMicroelectronics MultiTouch IC Driver");
MODULE_AUTHOR("STMicroelectronics, Inc.");
MODULE_LICENSE("GPL v2");

module_init(fts_driver_init);
module_exit(fts_driver_exit);
