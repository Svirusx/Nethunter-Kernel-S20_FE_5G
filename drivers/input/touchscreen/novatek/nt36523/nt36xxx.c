/*
 * Copyright (C) 2010 - 2017 Novatek, Inc.
 *
 * $Revision: 31202 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/input/mt.h>

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
#include <linux/completion.h>
#include <linux/atomic.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/input/sec_secure_touch.h>

#define SECURE_TOUCH_ENABLED	1
#define SECURE_TOUCH_DISABLED	0
#endif

#include "nt36xxx.h"

#define TOUCH_MAX_AREA_NUM		255
#define TOUCH_MAX_PRESSURE_NUM		1000


#define FINGER_ENTER			0x01
#define FINGER_MOVING			0x02
#define PALM_TOUCH			0x05
#define GLOVE_TOUCH			0x06

#define NVT_TS_I2C_RETRY_CNT		5
#define NVT_TS_LOCATION_DETECT_SIZE	6

#define TOUCH_PRINT_INFO_DWORK_TIME	30000	/* 30s */


int nvt_ts_sec_fn_init(struct nvt_ts_data *ts);
void nvt_ts_sec_fn_remove(struct nvt_ts_data *ts);

int nvt_ts_firmware_update_on_probe(struct nvt_ts_data *ts, bool bforce);

int nvt_ts_mode_restore(struct nvt_ts_data *ts);
int nvt_ts_mode_read(struct nvt_ts_data *ts);

static int nvt_ts_check_chip_ver_trim(struct nvt_ts_data *ts, uint32_t chip_ver_trim_addr);


#ifdef CONFIG_DISPLAY_SAMSUNG
extern int get_lcd_attached(char *mode);
#endif
#ifdef CONFIG_SAMSUNG_TUI
struct nvt_ts_data *tsp_info;
#endif
#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif

void nvt_interrupt_set(struct nvt_ts_data *ts, int enable)
{
	struct irq_desc *desc = irq_to_desc(ts->client->irq);

	mutex_lock(&ts->irq_mutex);

	if (enable) {
		while (desc->depth > 0)
			enable_irq(ts->client->irq);
	} else {
		disable_irq(ts->client->irq);
	}

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, enable ? "Enable" : "Disable");
	mutex_unlock(&ts->irq_mutex);
}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
static irqreturn_t secure_filter_interrupt(struct nvt_ts_data *ts)
{
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLED) {
		if (atomic_cmpxchg(&ts->secure_pending_irqs, 0, 1) == 0) {
			sysfs_notify(&ts->input_dev->dev.kobj, NULL, "secure_touch");
		} else {
			input_err(true, &ts->client->dev, "%s: pending irq:%d\n",
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
static ssize_t secure_touch_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nvt_ts_data *ts = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d", atomic_read(&ts->secure_enabled));
}

static ssize_t secure_touch_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct nvt_ts_data *ts = dev_get_drvdata(dev);
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
		/* Enable Secure World */
		if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLED) {
			input_err(true, &ts->client->dev,
					"%s: already enabled\n", __func__);
			return -EBUSY;
		}

		nvt_interrupt_set(ts, INT_DISABLE);

		if (pm_runtime_get_sync(ts->client->adapter->dev.parent) < 0) {
			nvt_interrupt_set(ts, INT_ENABLE);
			input_err(true, &ts->client->dev,
					"%s: failed to get pm_runtime\n", __func__);
			return -EIO;
		}

		reinit_completion(&ts->secure_powerdown);
		reinit_completion(&ts->secure_interrupt);

		msleep(10);

		atomic_set(&ts->secure_enabled, 1);
		atomic_set(&ts->secure_pending_irqs, 0);

		nvt_interrupt_set(ts, INT_ENABLE);

		input_info(true, &ts->client->dev,
				"%s: secure touch enable\n", __func__);
	} else if (!data) {
		/* Disable Secure World */
		if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_DISABLED) {
			input_err(true, &ts->client->dev,
					"%s: already disabled\n", __func__);
			return count;
		}

		pm_runtime_put_sync(ts->client->adapter->dev.parent);
		atomic_set(&ts->secure_enabled, 0);
		sysfs_notify(&ts->input_dev->dev.kobj, NULL, "secure_touch");

		msleep(10);

		complete(&ts->secure_powerdown);
		complete(&ts->secure_interrupt);

		input_info(true, &ts->client->dev,
				"%s: secure touch disable\n", __func__);

	} else {
		input_err(true, &ts->client->dev,
				"%s: unsupported value %ld\n", __func__, data);
		return -EINVAL;
	}

	return count;
}

static ssize_t secure_touch_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nvt_ts_data *ts = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_DISABLED) {
		input_err(true, &ts->client->dev, "%s: disabled\n", __func__);
		return -EBADF;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, -1, 0) == -1) {
		input_err(true, &ts->client->dev, "%s: pending irq -1\n", __func__);
		return -EINVAL;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, 1, 0) == 1)
		val = 1;

	input_info(true, &ts->client->dev, "%s: pending irq is %d\n",
			__func__, atomic_read(&ts->secure_pending_irqs));

	complete(&ts->secure_interrupt);

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

static void secure_touch_init(struct nvt_ts_data *ts)
{
	init_completion(&ts->secure_powerdown);
	init_completion(&ts->secure_interrupt);
}

static void secure_touch_stop(struct nvt_ts_data *ts, bool stop)
{
	if (atomic_read(&ts->secure_enabled)) {
		atomic_set(&ts->secure_pending_irqs, -1);

		sysfs_notify(&ts->input_dev->dev.kobj, NULL, "secure_touch");

		if (stop)
			wait_for_completion_interruptible(&ts->secure_powerdown);

		input_info(true, &ts->client->dev, "%s: %d\n", __func__, stop);
	}
}
#endif


int nvt_ts_i2c_read(struct nvt_ts_data *ts, u16 address, u8 *buf, u16 len)
{
	struct i2c_msg msgs[2];
	int ret = -1;
	int retries = 0;

	memset(&buf[1], 0x00, len - 1);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		return -EIO;
	}

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = address;
	msgs[0].len   = 1;
	msgs[0].buf   = &buf[0];

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = address;
	msgs[1].len   = len - 1;
	msgs[1].buf   = &buf[1];

	mutex_lock(&ts->i2c_mutex);

	while (retries < NVT_TS_I2C_RETRY_CNT) {
		ret = i2c_transfer(ts->client->adapter, msgs, 2);
		if (ret == 2)
			break;

		if (ts->power_status == POWER_OFF_STATUS) {
			input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
			mutex_unlock(&ts->i2c_mutex);
			ret = -EIO;

			goto err;
		}

		if (retries >= (NVT_TS_I2C_RETRY_CNT - 1)) {
			input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retries + 1, ret);
			ts->comm_err_count++;
		}

		retries++;
	}

	mutex_unlock(&ts->i2c_mutex);

	if (unlikely(retries == NVT_TS_I2C_RETRY_CNT))
		input_err(true, &ts->client->dev, "%s: I2C read over retry limit\n", __func__);

	if (ts->debug_flag & NVT_TS_DEBUG_PRINT_I2C_READ_CMD) {
		int i;

		pr_info("%s: i2c_cmd: R: %02X | ", SECLOG, buf[0]);
		for (i = 0; i < len - 1; i++)
			pr_cont("%02X ", buf[i + 1]);
		pr_cont("\n");
	}

err:
	return ret;
}

int nvt_ts_i2c_write(struct nvt_ts_data *ts, u16 address, u8 *buf, u16 len)
{
	struct i2c_msg msg;
	int ret = -1;
	int retries = 0;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		return -EIO;
	}

	msg.flags = !I2C_M_RD;
	msg.addr  = address;
	msg.len   = len;
	msg.buf   = buf;

	mutex_lock(&ts->i2c_mutex);

	while (retries < NVT_TS_I2C_RETRY_CNT) {
		ret = i2c_transfer(ts->client->adapter, &msg, 1);
		if (ret == 1)
			break;

		if (ts->power_status == POWER_OFF_STATUS) {
			input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
			mutex_unlock(&ts->i2c_mutex);
			ret = -EIO;
			goto err;
		}

		if (retries >= (NVT_TS_I2C_RETRY_CNT - 1)) {
			input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retries + 1, ret);
			ts->comm_err_count++;
		}

		retries++;
	}

	mutex_unlock(&ts->i2c_mutex);

	if (unlikely(retries == NVT_TS_I2C_RETRY_CNT))
		input_err(true, &ts->client->dev, "%s: I2C write over retry limit\n", __func__);

	if (ts->debug_flag & NVT_TS_DEBUG_PRINT_I2C_WRITE_CMD) {
		int i;

		pr_info("%s: i2c_cmd: W: ", SECLOG);
		for (i = 0; i < len; i++)
			pr_cont("%02X ", buf[i]);
		pr_cont("\n");
	}

err:
	return ret;
}

int32_t nvt_ts_set_page(struct nvt_ts_data *ts, uint16_t i2c_addr, uint32_t addr)
{
	uint8_t buf[4] = {0};

	buf[0] = 0xFF;	//set index/page/addr command
	buf[1] = (addr >> 16) & 0xFF;
	buf[2] = (addr >> 8) & 0xFF;

	return nvt_ts_i2c_write(ts, i2c_addr, buf, 3);
}

int nvt_ts_nt36523_ics_i2c_read(struct nvt_ts_data *ts, u32 address, u8 *data, u16 len)
{
	u8 buf[10];
	int i;
	int retry = 20, ret;

	//---set xdata index to SPI DMA Registers---
	ret = nvt_ts_set_page(ts, I2C_FW_Address, ts->mmap->SPI_DMA_LENGTH);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to set xdata index(SPI DMA reg)\n", __func__);
		return ret;
	}

	//---set length---
	buf[0] = (u8)(ts->mmap->SPI_DMA_LENGTH & 0xFF);
	buf[1] = (u8)((len - 1) & 0xFF);
	buf[2] = (u8)(((len - 1) >> 8) & 0xFF);
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set length\n", __func__);
		return ret;
	}

	//---set remote (ICS) address---
	buf[0] = (u8)(ts->mmap->SPI_DMA_REM_ADDR & 0xFF);
	buf[1] = (u8)(address & 0xFF);
	buf[2] = (u8)((address >> 8) & 0xFF);
	buf[3] = (u8)((address >> 16) & 0xFF);
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set remote addr\n", __func__);
		return ret;
	}

	//---set local (ICM) address---
	buf[0] = (u8)(ts->mmap->SPI_DMA_LOC_ADDR & 0xFF);
	buf[1] = (u8)(ts->mmap->LOC_RW_BUF_ADDR & 0xFF);
	buf[2] = (u8)((ts->mmap->LOC_RW_BUF_ADDR >> 8) & 0xFF);
	buf[3] = (u8)((ts->mmap->LOC_RW_BUF_ADDR >> 16) & 0xFF);
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set local addr\n", __func__);
		return ret;
	}

	//---trigger SPI_TX_READ_TRIGGER---
	buf[0] = (u8)(ts->mmap->SPI_TX_READ_TRIGGER & 0xFF);
	buf[1] = 0x01;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to trigger SPI_TX_READ_TRIGGER\n", __func__);
		return ret;
	}

	//---wait SPI DMA done---
	for (i = 0; i < retry; i++) {
		buf[0] = (u8)(ts->mmap->SPI_DMA_TX_INFO & 0xFF);
		buf[1] = 0xFF;
		ret = nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: failed to read SPI DMA done\n", __func__);
			return ret;
		}

		if (buf[1] == 0x00)
			break;

		msleep(10);
	}
	if (i == retry) {
		input_err(true, &ts->client->dev, "%s: write over retry limit\n", __func__);
		return -EIO;
	}

	//---read data from local (ICM) memory---
	ret = nvt_ts_set_page(ts, I2C_FW_Address, ts->mmap->LOC_RW_BUF_ADDR);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set page(ICM)\n", __func__);
		return ret;
	}

	data[0] = (u8)(ts->mmap->LOC_RW_BUF_ADDR & 0xFF);
	nvt_ts_i2c_read(ts, I2C_FW_Address, data, len);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read data\n", __func__);
		return ret;
	}

	return 0;
}

int nvt_ts_nt36523_ics_i2c_write(struct nvt_ts_data *ts, u32 address, u8 *data, u16 len)
{
	u8 buf[10];
	int i;
	int retry = 20, ret;

	//---set xdata index to local (ICM) temp memory---
	ret = nvt_ts_set_page(ts, I2C_FW_Address, ts->mmap->LOC_RW_BUF_ADDR);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set xdata index(ICM)\n", __func__);
		return ret;
	}

	//---write data to local (ICM) memory---
	data[0] = (u8)(ts->mmap->LOC_RW_BUF_ADDR & 0xFF);
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, data, len);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to write data\n", __func__);
		return ret;
	}

	//---set xdata index to SPI DMA Registers---
	ret = nvt_ts_set_page(ts, I2C_FW_Address, ts->mmap->SPI_DMA_LENGTH);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to set xdata index(SPI DMA reg)\n", __func__);
		return ret;
	}

	//---set length---
	buf[0] = (u8)(ts->mmap->SPI_DMA_LENGTH & 0xFF);
	buf[1] = (u8)((len - 1) & 0xFF);
	buf[2] = (u8)(((len - 1) >> 8) & 0xFF);
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set length\n", __func__);
		return ret;
	}

	//---set remote (ICS) address---
	buf[0] = (u8)(ts->mmap->SPI_DMA_REM_ADDR & 0xFF);
	buf[1] = (u8)(address & 0xFF);
	buf[2] = (u8)((address >> 8) & 0xFF);
	buf[3] = (u8)((address >> 16) & 0xFF);
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set remote addr\n", __func__);
		return ret;
	}

	//---set local (ICM) address---
	buf[0] = (u8)(ts->mmap->SPI_DMA_LOC_ADDR & 0xFF);
	buf[1] = (u8)(ts->mmap->LOC_RW_BUF_ADDR & 0xFF);
	buf[2] = (u8)((ts->mmap->LOC_RW_BUF_ADDR >> 8) & 0xFF);
	buf[3] = (u8)((ts->mmap->LOC_RW_BUF_ADDR >> 16) & 0xFF);
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set local addr\n", __func__);
		return ret;
	}

	//---trigger SPI_TX_WRITE_TRIGGER---
	buf[0] = (u8)(ts->mmap->SPI_TX_WRITE_TRIGGER & 0xFF);
	buf[1] = 0x01;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to trigger SPI_TX_WRITE_TRIGGER\n", __func__);
		return ret;
	}

	//---wait SPI DMA done---
	for (i = 0; i < retry; i++) {
		buf[0] = (u8)(ts->mmap->SPI_DMA_TX_INFO & 0xFF);
		buf[1] = 0xFF;
		ret = nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: failed to read SPI DMA done\n", __func__);
			return ret;
		}

		if (buf[1] == 0x00)
			break;

		msleep(10);
	}
	if (i == retry) {
		input_err(true, &ts->client->dev, "%s: write over retry limit\n", __func__);
		return -EIO;
	}

	return 0;
}

void nvt_ts_sw_reset_idle(struct nvt_ts_data *ts)
{
	u8 buf[4] = { 0 };

	buf[0]=0x00;
	buf[1]=0xA5;
	nvt_ts_i2c_write(ts, I2C_HW_Address, buf, 2);

	msleep(15);
}

void nvt_ts_bootloader_reset(struct nvt_ts_data *ts)
{
	u8 buf[4] = { 0 };

	buf[0] = 0x00;
	buf[1] = 0x69;
	nvt_ts_i2c_write(ts, I2C_HW_Address, buf, 2);

	msleep(35);
}

int nvt_ts_check_fw_reset_state(struct nvt_ts_data *ts, RST_COMPLETE_STATE reset_state)
{
	u8 buf[8] = { 0 };
	int retry = 0;
	int ret;

	while (retry <= 100) {
		msleep(10);

		//---read reset state---
		buf[0] = EVENT_MAP_RESET_COMPLETE;
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 6);

		if ((buf[1] >= reset_state) && (buf[1] <= RESET_STATE_MAX)) {
			ret = 0;
			break;
		}
		retry++;
	}

	if (unlikely(retry > 100)) {
		input_err(true, &ts->client->dev, "%s: Time over\n", __func__);
		ret = -EPERM;
	}

	input_info(true, &ts->client->dev, "%s: %02X %02X %02X %02X %02X\n",
		__func__, buf[1], buf[2], buf[3], buf[4], buf[5]);

	return ret;
}

int nvt_ts_get_fw_info(struct nvt_ts_data *ts)
{
	struct nvt_ts_platdata *platdata = ts->platdata;
	u8 buf[10] = { 0 };
	u8 fw_ver, x_num, y_num;
	u16 abs_x_max, abs_y_max;
	int i;
	int ret = -EPERM;

	for (i = 0; i < 3; i++) {
		//---set xdata index to EVENT BUF ADDR---
		buf[0] = 0xFF;
		buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		//---read fw info---
		buf[0] = EVENT_MAP_FWINFO;
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 9);

		fw_ver = buf[1];
		x_num = buf[3];
		y_num = buf[4];
		abs_x_max = (u16)((buf[5] << 8) | buf[6]);
		abs_y_max = (u16)((buf[7] << 8) | buf[8]);

		//---clear x_num, y_num if fw info is broken---
		if ((buf[1] + buf[2]) == 0xFF) {
			ret = 0;
			break;
		}

		input_err(true, &ts->client->dev, "FW info is broken! fw_ver = 0x%02X, ~fw_ver = 0x%02X\n",
				buf[1], buf[2]);
		fw_ver = 0;
		x_num = platdata->x_num;
		y_num = platdata->y_num;
		abs_x_max = platdata->abs_x_max;
		abs_y_max = platdata->abs_y_max;
	}

	platdata->x_num = x_num;
	platdata->y_num = y_num;
	platdata->abs_x_max = abs_x_max;
	platdata->abs_y_max = abs_y_max;

	input_info(true, &ts->client->dev, "%s: fw_ver: %d, channel: (%d, %d), resolution: (%d,%d)\n",
		__func__, fw_ver, x_num, y_num, abs_x_max, abs_y_max);

	/* ic_name, project_id, panel, fw info */
	//---get chip id---
	ret = nvt_ts_check_chip_ver_trim(ts, CHIP_VER_TRIM_ADDR);
	if (ret) {
		input_info(true, &ts->client->dev, "%s: try to check from old chip ver trim address\n", __func__);
		ret = nvt_ts_check_chip_ver_trim(ts, CHIP_VER_TRIM_OLD_ADDR);
		if (ret) {
			input_err(true, &ts->client->dev, "chip is not identified\n");
		}
	}
	nvt_ts_bootloader_reset(ts);
	nvt_ts_check_fw_reset_state(ts, RESET_STATE_INIT);

	//---get project id---
	buf[0] = EVENT_MAP_PROJECTID;
	ret = nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvt_ts_i2c_read error(%d)\n", __func__, ret);
		return ret;
	}
	ts->fw_ver_ic[1] = buf[1];

	//---get panel id---
	buf[0] = EVENT_MAP_PANEL;
	ret = nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvt_ts_i2c_read error(%d)\n", __func__, ret);
		return ret;
	}
	ts->fw_ver_ic[2] = buf[1];

	//---get firmware version---
	buf[0] = EVENT_MAP_FWINFO;
	ret = nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvt_ts_i2c_read error(%d)\n", __func__, ret);
		return ret;
	}
	ts->fw_ver_ic[3] = buf[1];

	input_info(true, &ts->client->dev, "%s: fw_ver_ic = %02X%02X%02X%02X\n",
		__func__, ts->fw_ver_ic[0], ts->fw_ver_ic[1], ts->fw_ver_ic[2], ts->fw_ver_ic[3]);

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen wake up gesture key report function.

return:
	n.a.
*******************************************************/
void nvt_ts_wakeup_gesture_report(struct nvt_ts_data *ts, uint8_t *data)
{
	struct sponge_report_format *sponge_packet = (struct sponge_report_format *)(data + 2);

	if (sponge_packet->eid == NVT_GESTURE_EVENT) {
		switch (sponge_packet->gesturetype) {
		case NVT_GESTURE_TYPE_DOUBLE_TAP:
			if (sponge_packet->gestureid == NVT_GESTURE_ID_DOUBLETAP_TO_WAKEUP) {
				input_report_key(ts->input_dev, KEY_WAKEUP, 1);
				input_sync(ts->input_dev);
				input_report_key(ts->input_dev, KEY_WAKEUP, 0);
				input_sync(ts->input_dev);
				input_info(true, &ts->client->dev, "%s: DOUBLE TAP TO WAKE\n", __func__);
			}
			break;
		default:
			input_err(true, &ts->client->dev,
					"not supported gesture type:0x%02X, id:0x%02X\n",
					sponge_packet->gesturetype, sponge_packet->gestureid);
			break;
		}
	} else {
		input_info(true, &ts->client->dev,
				"%s: 0x%02X 0x%02X | 0x%02X 0x%02X "
				"0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
				__func__, data[0], data[1], data[2], data[3],
				data[4], data[5], data[6], data[7], data[8], data[9]);
	}
}

/************************************************************
*  720  * 1480 : <48 96 60> indicator: 24dp navigator:48dp edge:60px dpi=320
* 1080  * 2220 :  4096 * 4096 : <133 266 341>  (approximately value)
************************************************************/
static void location_detect(struct nvt_ts_data *ts, char *loc, int x, int y)
{
	int i;

	for (i = 0; i < NVT_TS_LOCATION_DETECT_SIZE; ++i)
		loc[i] = 0;

	if (x < ts->platdata->area_edge)
		strcat(loc, "E.");
	else if (x < (ts->platdata->abs_x_max - ts->platdata->area_edge))
		strcat(loc, "C.");
	else
		strcat(loc, "e.");

	if (y < ts->platdata->area_indicator)
		strcat(loc, "S");
	else if (y < (ts->platdata->abs_y_max - ts->platdata->area_navigation))
		strcat(loc, "C");
	else
		strcat(loc, "N");
}

static void nvt_ts_print_info(struct nvt_ts_data *ts)
{
	struct irq_desc *desc = irq_to_desc(ts->client->irq);

	if (!ts->print_info_cnt_open && !ts->print_info_cnt_release) {
		msleep(200);
		ts->sec_function = nvt_ts_mode_read(ts);
	}

	ts->print_info_cnt_open++;

	if (ts->print_info_cnt_open > 0xfff0)
		ts->print_info_cnt_open = 0;

	if (ts->touch_count == 0)
		ts->print_info_cnt_release++;

	input_info(true, &ts->client->dev,
			"tc:%d v:%02X%02X%02X mode:%04X noise:%d lp:%02X iq:%d depth:%d // #%d %d\n",
			ts->touch_count, ts->fw_ver_ic[1], ts->fw_ver_ic[2], ts->fw_ver_ic[3],
			ts->sec_function, ts->noise_mode, ts->lowpower_mode,
			gpio_get_value(ts->platdata->irq_gpio), desc->depth,
			ts->print_info_cnt_open, ts->print_info_cnt_release);
}

void nvt_ts_release_all_finger(struct nvt_ts_data *ts)
{
	char location[NVT_TS_LOCATION_DETECT_SIZE] = { 0, };
	int i = 0;

	for (i = 0; i < ts->platdata->max_touch_num; i++) {
		location_detect(ts, location, ts->coords[i].x, ts->coords[i].y);

		input_mt_slot(ts->input_dev, i);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_CUSTOM, 0);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

		if (ts->coords[i].p_press) {
			ts->touch_count--;

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &ts->client->dev,
				"[RA] tId:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d\n",
				i, location, ts->coords[i].p_x - ts->coords[i].x,
				ts->coords[i].p_y - ts->coords[i].y,
				ts->coords[i].move_count, ts->touch_count,
				ts->coords[i].x, ts->coords[i].y);
#else
			input_info(true, &ts->client->dev,
				"[RA] tId:%d loc:%s dd:%d,%d mc:%d tc:%d\n",
				i, location, ts->coords[i].p_x - ts->coords[i].x,
				ts->coords[i].p_y - ts->coords[i].y,
				ts->coords[i].move_count, ts->touch_count);
#endif
			ts->coords[i].p_press = false;
			ts->coords[i].press = false;
		}
	}

	input_mt_slot(ts->input_dev, 0);

	input_report_key(ts->input_dev, BTN_TOUCH, false);
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, false);

	input_sync(ts->input_dev);

	ts->check_multi = false;
	ts->touch_count = 0;
}

static void nvt_ts_print_coord(struct nvt_ts_data *ts)
{
	int i;
	char location[NVT_TS_LOCATION_DETECT_SIZE] = { 0, };

	for (i = 0; i < TOUCH_MAX_FINGER_NUM; i++) {
		location_detect(ts, location, ts->coords[i].x, ts->coords[i].y);

		if (ts->coords[i].press && !ts->coords[i].p_press) {
			ts->touch_count++;
			ts->all_finger_count++;

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &ts->client->dev,
				"[P] tId:%d.%d x:%d y:%d p:%d major:%d minor:%d loc:%s tc:%d type:%X\n",
				i, (ts->input_dev->mt->trkid - 1) & TRKID_MAX,
				ts->coords[i].x, ts->coords[i].y, ts->coords[i].palm,
				ts->coords[i].w_major, ts->coords[i].w_minor,
				location, ts->touch_count, ts->coords[i].status);
#else
			input_info(true, &ts->client->dev,
				"[P] tId:%d.%d p:%d major:%d minor:%d loc:%s tc:%d type:%X\n",
				i, (ts->input_dev->mt->trkid - 1) & TRKID_MAX, ts->coords[i].palm,
				ts->coords[i].w_major, ts->coords[i].w_minor, location, ts->touch_count,
				ts->coords[i].status);
#endif
			ts->coords[i].p_press = ts->coords[i].press;
			ts->coords[i].p_x = ts->coords[i].x;
			ts->coords[i].p_y = ts->coords[i].y;

			ts->coords[i].p_status = 0;
			ts->coords[i].move_count = 0;

		} else if (!ts->coords[i].press && ts->coords[i].p_press) {
			ts->touch_count--;

			if (!ts->touch_count)
				ts->print_info_cnt_release = 0;

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &ts->client->dev,
				"[R] tId:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d\n",
				i, location, ts->coords[i].p_x - ts->coords[i].x,
				ts->coords[i].p_y - ts->coords[i].y,
				ts->coords[i].move_count, ts->touch_count,
				ts->coords[i].x, ts->coords[i].y);
#else
			input_info(true, &ts->client->dev,
				"[R] tId:%d loc:%s dd:%d,%d mc:%d tc:%d\n",
				i, location, ts->coords[i].p_x - ts->coords[i].x,
				ts->coords[i].p_y - ts->coords[i].y,
				ts->coords[i].move_count, ts->touch_count);
#endif
			ts->coords[i].p_press = false;
		} else if (ts->coords[i].press) {
			if (ts->coords[i].p_status && (ts->coords[i].status != ts->coords[i].p_status)) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				input_info(true, &ts->client->dev,
					"[C] tId:%d x:%d y:%d p:%d major:%d minor:%d tc:%d type:%X\n",
					i, ts->coords[i].x, ts->coords[i].y, ts->coords[i].palm,
					ts->coords[i].w_major, ts->coords[i].w_minor,
					ts->touch_count, ts->coords[i].status);
#else
				input_info(true, &ts->client->dev,
					"[C] tId:%d p:%d major:%d minor:%d tc:%d type:%X\n",
					i, ts->coords[i].palm, ts->coords[i].w_major, ts->coords[i].w_minor,
					ts->touch_count, ts->coords[i].status);
#endif
			}

			ts->coords[i].p_status = ts->coords[i].status;
			ts->coords[i].move_count++;
		}
	}
}

#define POINT_DATA_LEN 108
static void nvt_ts_event_handler(struct nvt_ts_data *ts)
{
	struct nvt_ts_platdata *platdata = ts->platdata;
	u8 point_data[POINT_DATA_LEN + 1] = { 0 };
	bool press_id[TOUCH_MAX_FINGER_NUM] = { 0 };
	uint32_t position;
	u16 x, y, p;
	u8 w_major, w_minor, palm;
	u8 id, status;
	int finger_cnt;
	int i;
	int ret;

	if (ts->power_status == POWER_LPM_STATUS) {
		wake_lock_timeout(&ts->wakelock, msecs_to_jiffies(500));
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret == 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			return;
		}

		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			return;
		}
		input_info(true, &ts->client->dev, "%s: run LPM interrupt handler\n", __func__);
	}

	ret = nvt_ts_i2c_read(ts, I2C_FW_Address, point_data, POINT_DATA_LEN + 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "nvt_ts_i2c_read failed.(%d)\n", ret);
		return;
	}
/*
	//--- dump I2C buf ---
	for (i = 0; i < 10; i++) {
		printk("[sec_input] %02X %02X %02X %02X %02X %02X  ", point_data[1+i*6], point_data[2+i*6], point_data[3+i*6], point_data[4+i*6], point_data[5+i*6], point_data[6+i*6]);
	}
	printk("\n");
*/

	if (ts->power_status == POWER_LPM_STATUS) {
		nvt_ts_wakeup_gesture_report(ts, point_data);
		return;
	}

	for (i = 0, finger_cnt = 0; i < platdata->max_touch_num; i++) {
		position = 1 + 6 * i;
		id = (uint8_t)(point_data[position + 0] >> 3);
		if (!id || (id > platdata->max_touch_num))
			continue;

		id = id - 1;
		status = (point_data[position] & 0x07);
		if ((status == FINGER_ENTER) || (status == FINGER_MOVING) || (status == GLOVE_TOUCH) || (status == PALM_TOUCH)) {
			ts->coords[id].status = status;
			press_id[id] = ts->coords[id].press = true;
			x = ts->coords[id].x = (u16)(point_data[position + 1] << 4) + (u16)(point_data[position + 3] >> 4);
			y = ts->coords[id].y = (u16)(point_data[position + 2] << 4) + (u16)(point_data[position + 3] & 0x0F);
			if ((x < 0) || (y < 0) || (x > platdata->abs_x_max) || (y > platdata->abs_y_max)) {
				input_err(true, &ts->client->dev, "%s: invaild coordinate (%d, %d)\n",
					__func__, x, y);
				continue;
			}

			// width major
			w_major = point_data[position + 4];
			if (!w_major)
				w_major = 1;
			ts->coords[id].w_major = w_major;

			// with minor
			w_minor = point_data[i + 99];
			if (!w_minor)
				w_minor = 1;
			ts->coords[id].w_minor = w_minor;

			// palm
			palm = (status == PALM_TOUCH) ? 1 : 0;
			ts->coords[id].palm = palm;

			// pressure
			if (i < 2) {
				p = (u16)(point_data[position + 5]) + (uint32_t)(point_data[i + 63] << 8);
				if (p > TOUCH_MAX_PRESSURE_NUM)
					p = TOUCH_MAX_PRESSURE_NUM;
			} else {
				p = (u16)(point_data[position + 5]);
			}
			if (!p)
				p = 1;
			ts->coords[id].p = p;

			input_mt_slot(ts->input_dev, id);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);

			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w_major);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, w_minor);
			input_report_abs(ts->input_dev, ABS_MT_CUSTOM, palm);
			//input_report_abs(ts->input_dev, ABS_MT_PRESSURE, p);

			finger_cnt++;
		}
	}

	for (i = 0; i < platdata->max_touch_num; i++) {
		if (!press_id[i] && ts->coords[i].p_press) {
			ts->coords[i].press = false;

			input_mt_slot(ts->input_dev, i);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, 0);
			input_report_abs(ts->input_dev, ABS_MT_CUSTOM, 0);
			//input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
		}
	}

	input_report_key(ts->input_dev, BTN_TOUCH, (finger_cnt > 0));
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, (finger_cnt > 0));

	input_sync(ts->input_dev);

	if (!finger_cnt) {
		ts->check_multi = false;
	} else if ((finger_cnt > 4) && !ts->check_multi) {
		ts->check_multi = true;
		ts->multi_count++;
	}

	nvt_ts_print_coord(ts);
}

static irqreturn_t nvt_ts_irq_thread(int32_t irq, void *dev_id)
{
	struct nvt_ts_data *ts = (struct nvt_ts_data *)dev_id;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (IRQ_HANDLED == secure_filter_interrupt(ts)) {
		wait_for_completion_interruptible_timeout(&ts->secure_interrupt,
				msecs_to_jiffies(5 * MSEC_PER_SEC));

		input_info(true, &ts->client->dev,
				"%s: secure interrupt handled\n", __func__);

		return IRQ_HANDLED;
	}
#endif

	nvt_ts_event_handler(ts);

	return IRQ_HANDLED;
}

void nvt_ts_stop_crc_reboot(struct nvt_ts_data *ts)
{
	u8 buf[8] = { 0 };
	int retry = 0;

	//read dummy buffer to check CRC fail reboot is happening or not

	//---change I2C index to prevent geting 0xFF, but not 0xFC---
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0xF6;
	nvt_ts_i2c_write(ts, I2C_BLDR_Address, buf, 3);

	//---read to check if buf is 0xFC which means IC is in CRC reboot ---
	buf[0] = 0x4E;
	nvt_ts_i2c_read(ts, I2C_BLDR_Address, buf, 4);

	if ((buf[1] == 0xFC) ||
		((buf[1] == 0xFF) && (buf[2] == 0xFF) && (buf[3] == 0xFF))) {

		//IC is in CRC fail reboot loop, needs to be stopped!
		for (retry = 5; retry > 0; retry--) {

			//---write i2c cmds to reset idle : 1st---
			buf[0]=0x00;
			buf[1]=0xA5;
			nvt_ts_i2c_write(ts, I2C_HW_Address, buf, 2);

			//---write i2c cmds to reset idle : 2rd---
			buf[0]=0x00;
			buf[1]=0xA5;
			nvt_ts_i2c_write(ts, I2C_HW_Address, buf, 2);
			msleep(1);

			//---clear CRC_ERR_FLAG---
			buf[0] = 0xFF;
			buf[1] = 0x03;
			buf[2] = 0xF1;
			nvt_ts_i2c_write(ts, I2C_BLDR_Address, buf, 3);

			buf[0] = 0x35;
			buf[1] = 0xA5;
			nvt_ts_i2c_write(ts, I2C_BLDR_Address, buf, 2);

			//---check CRC_ERR_FLAG---
			buf[0] = 0xFF;
			buf[1] = 0x03;
			buf[2] = 0xF1;
			nvt_ts_i2c_write(ts, I2C_BLDR_Address, buf, 3);

			buf[0] = 0x35;
			nvt_ts_i2c_read(ts, I2C_BLDR_Address, buf, 2);

			if (buf[1] == 0xA5)
				break;
		}

		if (retry == 0)
			input_err(true, &ts->client->dev,
				"failed to stop CRC auto reboot [0x%02X]\n", buf[1]);
	}

	return;
}

static int nvt_ts_check_chip_ver_trim(struct nvt_ts_data *ts, uint32_t chip_ver_trim_addr)
{
	uint8_t buf[8] = {0};
	int32_t retry = 0;
	int32_t list = 0;
	int32_t i = 0;
	int32_t found_nvt_chip = 0;
	int32_t ret = -1;

	nvt_ts_bootloader_reset(ts); // NOT in retry loop

	//---Check for 5 times---
	for (retry = 5; retry > 0; retry--) {
		nvt_ts_sw_reset_idle(ts);

		buf[0] = 0x00;
		buf[1] = 0x35;
		nvt_ts_i2c_write(ts, I2C_HW_Address, buf, 2);
		msleep(10);

		nvt_ts_set_page(ts, I2C_BLDR_Address, chip_ver_trim_addr);

		buf[0] = chip_ver_trim_addr & 0xFF;
		buf[1] = 0x00;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		buf[6] = 0x00;
		nvt_ts_i2c_read(ts, I2C_BLDR_Address, buf, 7);
		input_info(true, &ts->client->dev, "IC version: %02X %02X %02X %02X %02X %02X\n",
				buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

		//---put chip id value to fw_ver_ic---
		ts->fw_ver_ic[0] = ((buf[6] & 0x0F) << 4) | ((buf[5] & 0xF0) >> 4);

		//---Stop CRC check to prevent IC auto reboot---
		if ((buf[1] == 0xFC) ||
			((buf[1] == 0xFF) && (buf[2] == 0xFF) && (buf[3] == 0xFF))) {
			nvt_ts_stop_crc_reboot(ts);
			continue;
		}

		// compare read chip id on supported list
		for (list = 0; list < (sizeof(trim_id_table) / sizeof(struct nvt_ts_trim_id_table)); list++) {
			found_nvt_chip = 0;

			// compare each byte
			for (i = 0; i < NVT_ID_BYTE_MAX; i++) {
				if (trim_id_table[list].mask[i]) {
					if (buf[i + 1] != trim_id_table[list].id[i])
						break;
				}
			}

			if (i == NVT_ID_BYTE_MAX) {
				found_nvt_chip = 1;
			}

			if (found_nvt_chip) {
				input_info(true, &ts->client->dev, "found match ic version\n");
				ts->mmap = trim_id_table[list].mmap;
				ts->carrier_system = trim_id_table[list].hwinfo->carrier_system;
				ret = 0;
				goto out;
			} else {
				ts->mmap = &NT36525_memory_map; /* default value */
				ret = -ENODEV;
			}
		}

		msleep(10);
	}

out:
	return ret;
}

static int nvt_ts_input_open(struct input_dev *dev)
{
	struct nvt_ts_data *ts = input_get_drvdata(dev);

	if (ts->power_status == POWER_ON_STATUS) {
		input_dbg(true, &ts->client->dev, "%s: already power on\n", __func__);
		return 0;
	}

	input_info(true, &ts->client->dev, "%s\n", __func__);

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	secure_touch_stop(ts, 0);
#endif

	mutex_lock(&ts->lock);

	if (ts->power_status == POWER_OFF_STATUS)
		nvt_interrupt_set(ts, INT_ENABLE);
	else if (ts->power_status == POWER_LPM_STATUS && device_may_wakeup(&ts->client->dev))
		disable_irq_wake(ts->client->irq);

	ts->power_status = POWER_ON_STATUS;

	if (nvt_ts_check_fw_reset_state(ts, RESET_STATE_REK)) {
		input_err(true, &ts->client->dev,
				"%s: FW is not ready! Try to bootloader reset...\n", __func__);

		nvt_ts_bootloader_reset(ts);
		nvt_ts_check_fw_reset_state(ts, RESET_STATE_REK);
	}

	nvt_ts_mode_restore(ts);

	mutex_unlock(&ts->lock);

	cancel_delayed_work(&ts->work_print_info);
	ts->print_info_cnt_open = 0;
	ts->print_info_cnt_release = 0;
	schedule_work(&ts->work_print_info.work);

	return 0;
}

#if defined(CONFIG_SEC_FACTORY)
extern int ub_con_det_status(int index);
#endif

static int nvt_ts_open(struct nvt_ts_data *ts)
{
	input_dbg(false, &ts->client->dev, "%s\n", __func__);

#if defined(CONFIG_SEC_FACTORY)
	if (ub_con_det_status(0)) {
		input_info(true, &ts->client->dev, "%s: lcd is not attached\n", __func__);
		return 0;
	}
#endif

	nvt_ts_input_open(ts->input_dev);

	return 0;
}

static void nvt_ts_set_utc_sponge(struct nvt_ts_data *ts)
{
	struct timeval current_time;
	int ret;
	u8 data[4] = { 0, };

	do_gettimeofday(&current_time);
	data[0] = (0xFF & (u8)((current_time.tv_sec) >> 0));
	data[1] = (0xFF & (u8)((current_time.tv_sec) >> 8));
	data[2] = (0xFF & (u8)((current_time.tv_sec) >> 16));
	data[3] = (0xFF & (u8)((current_time.tv_sec) >> 24));
	input_info(true, &ts->client->dev, "Write UTC to Sponge = %X\n", (int)(current_time.tv_sec));

	ret = nvt_ts_write_to_sponge(ts, data, SPONGE_UTC, 4);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write sponge\n", __func__);
}

static void nvt_ts_close(struct nvt_ts_data *ts)
{
	u16 mode;
	char buf[2];
	int ret;

#if defined(CONFIG_SEC_FACTORY)
	if (ub_con_det_status(0)) {
		input_info(true, &ts->client->dev, "%s: lcd is not attached\n", __func__);
		return ;
	}
#endif

	if (!ts->lowpower_mode && ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: already power off\n", __func__);
		return;
 	} else if (ts->lowpower_mode && ts->power_status == POWER_LPM_STATUS) {
		input_err(true, &ts->client->dev, "%s: already in LP mode\n", __func__);
		return;
	}

	cancel_delayed_work(&ts->work_print_info);
	nvt_ts_print_info(ts);

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	secure_touch_stop(ts, 1);
#endif
#ifdef CONFIG_SAMSUNG_TUI
	stui_cancel_session();
#endif
	if (ts->lowpower_mode) {
		/* write UTC data to sponge */
		nvt_ts_set_utc_sponge(ts);

		ret = nvt_ts_write_to_sponge(ts, &ts->lowpower_mode, SPONGE_LP_FEATURE, 1);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: failed to write LP feature\n", __func__);
	}

	mutex_lock(&ts->lock);

	mode = nvt_ts_mode_read(ts);
	if (ts->sec_function != mode) {
		input_info(true, &ts->client->dev, "%s: func mode 0x%04X -> 0x%04X",
			__func__, ts->sec_function, mode);
		ts->sec_function = mode;
	}

	if (ts->lowpower_mode) {
		//---write command to enter "wakeup gesture mode"---
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = NVT_CMD_GESTURE_MODE;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);

		if (device_may_wakeup(&ts->client->dev))
			enable_irq_wake(ts->client->irq);

		ts->power_status = POWER_LPM_STATUS;

		input_info(true, &ts->client->dev, "%s: enter lp mode, 0x%02X\n",
				__func__, ts->lowpower_mode);
		if (ts->platdata->scanoff_cover_close && ts->flip_enable) {
			buf[0] = EVENT_MAP_HOST_CMD;
			buf[1] = NVT_CMD_DEEP_SLEEP_MODE;
			nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
			input_info(true, &ts->client->dev, "%s: deep sleep mode for cover\n", __func__);
		}
	} else {
		//---write i2c command to enter "deep sleep mode"---
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = NVT_CMD_DEEP_SLEEP_MODE;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);

		nvt_interrupt_set(ts, INT_DISABLE);

		ts->power_status = POWER_OFF_STATUS;

		input_info(true, &ts->client->dev, "%s: deep sleep mode, lp:0x%02X, flip:%d\n",
				__func__, ts->lowpower_mode, ts->flip_enable);
	}
	ts->untouchable_area = false;

	msleep(50);

	nvt_ts_release_all_finger(ts);

	mutex_unlock(&ts->lock);
}

static void nvt_ts_set_input_value(struct nvt_ts_data *ts, struct input_dev *input_dev, u8 propbit)
{
	if (propbit == INPUT_MT_POINTER)
		input_dev->name = "sec_touchpad";
	else
		input_dev->name = "sec_touchscreen";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &ts->client->dev;

	input_mt_init_slots(input_dev, TOUCH_MAX_FINGER_NUM, propbit);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ts->platdata->abs_x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ts->platdata->abs_y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, TOUCH_MAX_AREA_NUM, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR, 0, TOUCH_MAX_AREA_NUM, 0, 0);
	//input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, TOUCH_MAX_PRESSURE_NUM, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_CUSTOM, 0, 1, 0, 0);

	input_set_capability(input_dev, EV_KEY, BTN_TOUCH);
	input_set_capability(input_dev, EV_KEY, BTN_TOOL_FINGER);

	input_set_capability(input_dev, EV_KEY, KEY_WAKEUP);

	if (propbit == INPUT_MT_DIRECT)
		ts->input_dev = input_dev;

	input_set_drvdata(input_dev, ts);
}

#if defined(CONFIG_DISPLAY_SAMSUNG)
static int drm_notifier_callback(struct notifier_block *nb, unsigned long event, void *data)
{
	int *blank;
	struct nvt_ts_data *ts = container_of(nb, struct nvt_ts_data, drm_notif);

	if (event != FB_EVENT_BLANK)
		return 0;

	blank = data;
	input_dbg(true, &ts->client->dev, "%s: 0x%02X\n", __func__, *blank);

	if (*blank == FB_BLANK_POWERDOWN)
		nvt_ts_close(ts);
	else if (*blank == FB_BLANK_UNBLANK)
		nvt_ts_open(ts);

	return NOTIFY_OK;
}
#endif

#if defined(CONFIG_FB)
static int fb_notifier_cb(struct notifier_block *nb, unsigned long event, void *data)
{
	struct nvt_ts_data *ts = container_of(nb, struct nvt_ts_data, fb_nb);
	struct fb_event *evdata = data;
	int *blank;

	if (!evdata || !evdata->data)
		return 0;

	blank = evdata->data;
	if (event == FB_EARLY_EVENT_BLANK) {
		if (*blank == FB_BLANK_POWERDOWN)
			nvt_ts_close(ts);
	} else if (event == FB_EVENT_BLANK) {
		if (*blank == FB_BLANK_UNBLANK)
			nvt_ts_open(ts);
	}

	return 0;
}
#endif

static void nvt_ts_print_info_work(struct work_struct *work)
{
	struct nvt_ts_data *ts = container_of(work, struct nvt_ts_data,
			work_print_info.work);

	if (ts->power_status == POWER_ON_STATUS)
		nvt_ts_print_info(ts);

	schedule_delayed_work(&ts->work_print_info, msecs_to_jiffies(TOUCH_PRINT_INFO_DWORK_TIME));
}

static void nvt_ts_read_info_work(struct work_struct *work)
{
	struct nvt_ts_data *ts = container_of(work, struct nvt_ts_data, work_read_info.work);

	input_info(true, &ts->client->dev, "%s\n", __func__);

	nvt_ts_run_rawdata_all(ts);

	schedule_work(&ts->work_print_info.work);
}

static int nvt_ts_gpio_config(struct device *dev, struct nvt_ts_platdata *platdata)
{
	int ret;

	ret = devm_gpio_request(dev, platdata->irq_gpio, "nvt_irq");
	if (ret) {
		input_err(true, dev, "failed to request nvt_irq [%d]\n", ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_OF
static struct nvt_ts_platdata *nvt_ts_parse_dt(struct device *dev)
{
	struct nvt_ts_platdata *platdata;
	struct device_node *np = dev->of_node;
	int tmp[3];
	int ret;
#if !defined(CONFIG_EXYNOS_DPU20)
	int lcdtype = 0;
#endif
#if defined(CONFIG_EXYNOS_DECON_FB)
	int connected;
#endif

	if (!np)
		return ERR_PTR(-ENODEV);

#if defined(CONFIG_DISPLAY_SAMSUNG)
	lcdtype = get_lcd_attached("GET");
	if (lcdtype == 0xFFFFFF) {
		input_err(true, dev, "lcd is not attached\n");
		return ERR_PTR(-ENODEV);
	}
#endif

#if defined(CONFIG_EXYNOS_DECON_FB)
	connected = get_lcd_info("connected");
	if (connected < 0) {
		input_err(true, dev, "failed to get lcd connected info\n");
		return ERR_PTR(-EINVAL);
	}

	if (!connected) {
		input_err(true, dev, "lcd is not attached\n");
		return ERR_PTR(-ENODEV);
	}

	lcdtype = get_lcd_info("id");
	if (lcdtype < 0) {
		input_err(true, dev, "failed to get lcd id info\n");
		return ERR_PTR(-EINVAL);
	}
#endif

#if defined(CONFIG_EXYNOS_DPU20)
	if (lcdtype == 0) {
		input_err(true, dev, "lcd is not attached\n");
		return ERR_PTR(-ENODEV);
	}
#endif
	input_info(true, dev, "lcdtype 0x%08X\n", lcdtype);

	platdata = devm_kzalloc(dev, sizeof(*platdata), GFP_KERNEL);
	if (!platdata)
		return ERR_PTR(-ENOMEM);

	platdata->irq_gpio = of_get_named_gpio_flags(np, "novatek,irq-gpio", 0, &platdata->irq_flags);
	if (!gpio_is_valid(platdata->irq_gpio)) {
		input_err(true, dev, "failed to get irq-gpio(%d)\n", platdata->irq_gpio);
		return ERR_PTR(platdata->irq_gpio);
	} else if (!platdata->irq_flags) {
		platdata->irq_flags = IRQF_TRIGGER_LOW | IRQF_ONESHOT;
	}

	ret = of_property_read_string(np, "novatek,firmware_name", &platdata->firmware_name);
	if (ret)
		platdata->firmware_name = NULL;

	ret = of_property_read_u32_array(np, "novatek,resolution", tmp, 2);
	if (!ret) {
		platdata->abs_x_max = tmp[0];
		platdata->abs_y_max = tmp[1];
	} else {
		platdata->abs_x_max = DEFAULT_MAX_WIDTH;
		platdata->abs_x_max = DEFAULT_MAX_HEIGHT;
	}

	ret = of_property_read_u32_array(np, "novatek,max_touch_num", tmp, 1);
	if (!ret)
		platdata->max_touch_num = tmp[0];
	else
		platdata->max_touch_num = 10;

	if (of_property_read_u32_array(np, "novatek,area-size", tmp, 3)) {
		platdata->area_indicator = 133;
		platdata->area_navigation = 266;
		platdata->area_edge = 341;
	} else {
		platdata->area_indicator = tmp[0];
		platdata->area_navigation = tmp[1];
		platdata->area_edge = tmp[2];
	}

	platdata->support_dex = of_property_read_bool(np, "support_dex_mode");
	platdata->enable_settings_aot = of_property_read_bool(np, "enable_settings_aot");
	platdata->scanoff_cover_close = of_property_read_bool(np, "novatek,scanoff_when_cover_closed");
	platdata->enable_glove_mode = of_property_read_bool(np, "novatek,enable_glove_mode");
	platdata->enable_sysinput_enabled = of_property_read_bool(np, "enable_sysinput_enabled");

	if (of_property_read_u32(np, "novatek,bringup", &platdata->bringup) < 0)
		platdata->bringup = 0;
	platdata->check_fw_project_id = of_property_read_bool(np, "check_fw_project_id");

	input_info(true, dev, "zone's size - indicator:%d, navigation:%d, edge:%d\n",
		platdata->area_indicator, platdata->area_navigation, platdata->area_edge);

	input_info(true, dev, "bringup:%d, resolution: (%d, %d), firware_name: %s\n",
		platdata->bringup, platdata->abs_x_max, platdata->abs_y_max,
		platdata->firmware_name);

	return platdata;
}
#else
static struct nvt_ts_platdata *nvt_ts_parse_dt(struct device *dev)
{
	input_err(true, &dev, "no platform data specified\n");

	return ERR_PTR(-EINVAL);
}
#endif

int nvt_ts_read_from_sponge(struct nvt_ts_data *ts, u8 *data, int addr_offset, int len)
{
	int ret;
	u8 *buf;
	u32 target_addr;

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		return -EIO;
	}

	buf = kzalloc(len + 1, GFP_KERNEL);
	if (!buf) {
		input_err(true, &ts->client->dev, "%s: mem alloc failed\n", __func__);
		return -ENOMEM;
	}

	target_addr = ts->mmap->SPONGE_LIB_REG + addr_offset;

	mutex_lock(&ts->lock);

	nvt_ts_set_page(ts, I2C_FW_Address, target_addr);

	buf[0] = (u8)(target_addr & 0xFF);
	ret = nvt_ts_i2c_read(ts, I2C_FW_Address, buf, len + 1);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to read sponge command\n", __func__);
	else
		memcpy(data, buf + 1, len);

	nvt_ts_set_page(ts, I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR);

	mutex_unlock(&ts->lock);
	kfree(buf);
	return ret;
}

int nvt_ts_write_to_sponge(struct nvt_ts_data *ts, u8 *data, int addr_offset, int len)
{
	int ret;
	u8 *buf;
	u32 target_addr;

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		return -EIO;
	}

	buf = kzalloc(len + 1, GFP_KERNEL);
	if (!buf) {
		input_err(true, &ts->client->dev, "%s: mem alloc failed\n", __func__);
		return -ENOMEM;
	}

	target_addr = ts->mmap->SPONGE_LIB_REG + addr_offset;

	mutex_lock(&ts->lock);

	//Sponge Lib Reg
	nvt_ts_set_page(ts, I2C_FW_Address, target_addr);

	buf[0] = (u8)(target_addr & 0xFF);
	memcpy(buf + 1, data, len);

	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, len + 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Failed to write offset\n", __func__);
		nvt_ts_set_page(ts, I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR);
		mutex_unlock(&ts->lock);
		kfree(buf);
		return ret;
	}

	//Sponge Event Sync
	nvt_ts_set_page(ts, I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR);

	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x7D;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to send notify\n", __func__);

	mutex_unlock(&ts->lock);
	kfree(buf);
	return ret;
}

static int nvt_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct nvt_ts_data *ts;
	struct nvt_ts_platdata *platdata = dev_get_platdata(&client->dev);
	struct input_dev *input_dev;
	int ret;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev, "I2C functionality not supported\n");
		return -EIO;
	}

	ts = devm_kzalloc(&client->dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	if (!platdata) {
		platdata = nvt_ts_parse_dt(&client->dev);
		if (IS_ERR(platdata))
			return PTR_ERR(platdata);
	}

	input_dev = devm_input_allocate_device(&client->dev);
	if (!input_dev)
		return -ENOMEM;

	if (platdata->support_dex) {
		ts->input_dev_pad = input_allocate_device();
		if (!ts->input_dev_pad)
			return -ENOMEM;
	}

	ret = nvt_ts_gpio_config(&client->dev, platdata);
	if (ret)
		return ret;

	ts->client = client;
	ts->platdata = platdata;

	i2c_set_clientdata(client, ts);

	mutex_init(&ts->lock);
	mutex_init(&ts->i2c_mutex);
	mutex_init(&ts->irq_mutex);

	wake_lock_init(&ts->wakelock, WAKE_LOCK_SUSPEND, "tsp_wakelock");

	init_completion(&ts->resume_done);
	complete_all(&ts->resume_done);

	INIT_DELAYED_WORK(&ts->work_print_info, nvt_ts_print_info_work);
	INIT_DELAYED_WORK(&ts->work_read_info, nvt_ts_read_info_work);

	ts->power_status = POWER_ON_STATUS;
	/* need 10ms delay after POR(power on reset) */
	//msleep(10);

	/* check chip version trim */
	ret = nvt_ts_check_chip_ver_trim(ts, CHIP_VER_TRIM_ADDR);
	if (ret) {
		input_info(true, &ts->client->dev, "%s: try to check from old chip ver trim address\n", __func__);
		ret = nvt_ts_check_chip_ver_trim(ts, CHIP_VER_TRIM_OLD_ADDR);
		if (ret) {
			input_err(true, &ts->client->dev, "chip is not identified\n");
			ret = -EINVAL;
			goto err_check_trim;
		}
	}

	//nvt_ts_bootloader_reset(ts);
	//nvt_ts_check_fw_reset_state(ts, RESET_STATE_INIT);

	ret = nvt_ts_firmware_update_on_probe(ts, false);
	if (ret) {
		input_err(true, &client->dev, "failed to firmware update\n");
		goto err_fw_update;
	}

	nvt_ts_get_fw_info(ts);

	nvt_ts_set_input_value(ts, input_dev, INPUT_MT_DIRECT);
	ret = input_register_device(input_dev);
	if (ret) {
		input_err(true, &client->dev, "failed to register input device\n");
		goto err_input_regi_dev;
	}

	if (platdata->support_dex) {
		nvt_ts_set_input_value(ts, ts->input_dev_pad, INPUT_MT_POINTER);
		ret = input_register_device(ts->input_dev_pad);
		if (ret) {
			input_err(true, &client->dev, "failed to register input device\n");
			goto err_input_regi_dev;
		}
	}

	//ts->input_dev->open = nvt_ts_input_open;

	ts->sec_function = nvt_ts_mode_read(ts);
	input_info(true, &ts->client->dev, "%s: default func mode 0x%04X\n", __func__, ts->sec_function);

	client->irq = gpio_to_irq(platdata->irq_gpio);
	ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, nvt_ts_irq_thread,
					platdata->irq_flags, client->name, ts);
	if (ret < 0) {
		input_err(true, &client->dev, "failed to request irq\n");
		goto err_regi_irq;
	}

#if defined(CONFIG_DISPLAY_SAMSUNG)
	ts->drm_notif.notifier_call = drm_notifier_callback;
	ret = msm_drm_register_notifier_client(&ts->drm_notif);
	if (ret) {
		input_err(true, &client->dev, "failed to register msm drm notifier ret=%d\n", ret);
		goto err_regi_drm_notif;
	}
#endif

#if defined(CONFIG_FB)
	ts->fb_nb.notifier_call = fb_notifier_cb;
	ret = fb_register_client(&ts->fb_nb);
	if (ret) {
		input_err(true, &client->dev, "failed to register fb notifier ret=%d\n", ret);
		goto err_regi_fb_notif;
	}
#endif

	ret = nvt_ts_sec_fn_init(ts);
	if (ret) {
		input_err(true, &client->dev, "failed to init for factory function\n");
		goto err_init_sec_fn;
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	ts->ss_touch_num = 1;
	if (sysfs_create_group(&ts->input_dev->dev.kobj, &secure_attr_group) >= 0)
		sec_secure_touch_register(ts, ts->ss_touch_num, &ts->input_dev->dev.kobj);
	else
		input_err(true, &client->dev, "failed create sysfs group\n");

	secure_touch_init(ts);
#endif

#ifdef CONFIG_SAMSUNG_TUI
	tsp_info = ts;
#endif

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	nvt_ts_flash_proc_init(ts);
#endif
	device_init_wakeup(&client->dev, true);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge) {
		input_info(true, &client->dev, "%s: enter sleep mode in lpcharge %d\n",
				__func__, lpcharge);
		nvt_ts_close(ts);
	} else {
		schedule_delayed_work(&ts->work_read_info, msecs_to_jiffies(50));
	}
#else
	schedule_delayed_work(&ts->work_read_info, msecs_to_jiffies(50));
#endif

	input_err(true, &client->dev, "initialization is done\n");

	input_log_fix();

	return 0;

err_init_sec_fn:
#if defined(CONFIG_FB)
	fb_unregister_client(&ts->fb_nb);
err_regi_fb_notif:
#endif
#if defined(CONFIG_DISPLAY_SAMSUNG)
	msm_drm_unregister_notifier_client(&ts->drm_notif);
err_regi_drm_notif:
#endif
err_regi_irq:
err_input_regi_dev:
err_fw_update:
err_check_trim:
	wake_lock_destroy(&ts->wakelock);
	mutex_destroy(&ts->lock);
	mutex_destroy(&ts->i2c_mutex);
	mutex_destroy(&ts->irq_mutex);

	input_err(true, &client->dev, "failed to initialization\n");

	input_log_fix();

	return ret;
}

#ifdef CONFIG_PM
static int nvt_ts_suspend(struct device *dev)
{
	struct nvt_ts_data *ts = dev_get_drvdata(dev);

	reinit_completion(&ts->resume_done);
	return 0;
}

static int nvt_ts_resume(struct device *dev)
{
	struct nvt_ts_data *ts = dev_get_drvdata(dev);

	complete_all(&ts->resume_done);
	return 0;
}

static SIMPLE_DEV_PM_OPS(nvt_ts_dev_pm_ops, nvt_ts_suspend, nvt_ts_resume);
#endif

static void nvt_ts_shutdown(struct i2c_client *client)
{
	struct nvt_ts_data *ts = i2c_get_clientdata(client);

#if defined(CONFIG_FB)
	fb_unregister_client(&ts->fb_nb);
#endif
#if defined(CONFIG_DISPLAY_SAMSUNG)
	msm_drm_unregister_notifier_client(&ts->drm_notif);
#endif

	if (client->irq)
		free_irq(client->irq, ts);

	cancel_delayed_work_sync(&ts->work_read_info);
	cancel_delayed_work_sync(&ts->work_print_info);

	wake_lock_destroy(&ts->wakelock);
	mutex_destroy(&ts->lock);
	mutex_destroy(&ts->i2c_mutex);
	mutex_destroy(&ts->irq_mutex);

	input_info(true, &ts->client->dev, "%s\n", __func__);
}

static int nvt_ts_remove(struct i2c_client *client)
{
	struct nvt_ts_data *ts = i2c_get_clientdata(client);
#if defined(CONFIG_FB)
	fb_unregister_client(&ts->fb_nb);
#endif
#if defined(CONFIG_DISPLAY_SAMSUNG)
	msm_drm_unregister_notifier_client(&ts->drm_notif);
#endif

	cancel_delayed_work_sync(&ts->work_read_info);
	cancel_delayed_work_sync(&ts->work_print_info);

	wake_lock_destroy(&ts->wakelock);
	mutex_destroy(&ts->lock);
	mutex_destroy(&ts->i2c_mutex);
	mutex_destroy(&ts->irq_mutex);

	input_info(true, &ts->client->dev, "%s\n", __func__);

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

	nvt_interrupt_set(tsp_info, INT_DISABLE);

	ret = stui_i2c_lock(tsp_info->client->adapter);
	if (ret) {
		pr_err("[STUI] stui_i2c_lock failed : %d\n", ret);
		nvt_interrupt_set(tsp_info, INT_ENABLE);
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

	nvt_interrupt_set(tsp_info, INT_ENABLE);

	return ret;
}
#endif

static const struct i2c_device_id nvt_ts_id[] = {
	{ NVT_I2C_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id nvt_match_table[] = {
	{ .compatible = "novatek,nvt-ts",},
	{ },
};
#endif

static struct i2c_driver nvt_i2c_driver = {
	.driver = {
		.name	= NVT_I2C_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &nvt_ts_dev_pm_ops,
#endif
		.of_match_table = of_match_ptr(nvt_match_table),
	},
	.probe		= nvt_ts_probe,
	.remove		= nvt_ts_remove,
	.shutdown	= nvt_ts_shutdown,
	.id_table	= nvt_ts_id,
};

static int __init nvt_ts_driver_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&nvt_i2c_driver);
	if (ret)
		pr_err("%s: %s: failed to add i2c driver\n", SECLOG, __func__);

	return ret;
}

static void __exit nvt_ts_driver_exit(void)
{
	i2c_del_driver(&nvt_i2c_driver);
}

late_initcall_sync(nvt_ts_driver_init);
module_exit(nvt_ts_driver_exit);

MODULE_DESCRIPTION("Novatek Touchscreen Driver");
MODULE_LICENSE("GPL");
