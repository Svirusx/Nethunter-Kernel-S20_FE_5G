/*
 * Copyright (C) 2010 - 2017 Novatek, Inc.
 *
 * $Revision: 31202 $
 * $Date: 2018-07-23 10:56:09 +0800 (ÒŒÏÈ, 23 ˆ“Í≈ 2018) $
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


#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "nt36xxx.h"


#define DEVICE_NAME	"NVTflash"

static ssize_t nvt_ts_flash_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
	struct nvt_ts_data *ts = PDE_DATA(file_inode(file));
	u8 str[68] = {0};
	u8 i2c_wr = 0;
	int ret = -1;
	int retries;

	if (count > sizeof(str)) {
		input_err(true, &ts->client->dev, "%s: error count=%zu\n", __func__, count);
		return -EFAULT;
	}

	if (copy_from_user(str, buff, count)) {
		input_err(true, &ts->client->dev, "%s: copy from user error\n", __func__);
		return -EFAULT;
	}

	retries = 0;
	i2c_wr = str[0] >> 7;

	if (!i2c_wr) {
		while (retries < 20) {
			ret = nvt_ts_i2c_write(ts, (str[0] & 0x7F), &str[2], str[1]);
			if (ret == 1) {
				break;
			} else {
				input_err(true, &ts->client->dev, "%s: error, retries=%d, ret=%d\n",
					__func__, retries, ret);
			}

			retries++;
		}

		if (unlikely(retries == 20)) {
			input_err(true, &ts->client->dev, "%s: error, ret = %d\n",
				__func__, ret);
			return -EIO;
		}

		return ret;
	} else if (i2c_wr == 1) {	//I2C read
		while (retries < 20) {
			ret = nvt_ts_i2c_read(ts, (str[0] & 0x7F), &str[2], str[1]);
			if (ret == 2) {
				break;
			} else {
				input_err(true, &ts->client->dev, "%s: error, retries=%d, ret=%d\n",
					__func__, retries, ret);
			}

			retries++;
		}

		// copy buff to user if i2c transfer
		if (retries < 20) {
			if (copy_to_user(buff, str, count))
				return -EFAULT;
		}

		if (unlikely(retries == 20)) {
			input_err(true, &ts->client->dev, "%s: error, ret = %d\n",
					__func__, ret);
			return -EIO;
		}

		return ret;
	} else {
		input_err(true, &ts->client->dev, "%s: Call error, str[0]=%d\n",
				__func__, str[0]);
		return -EFAULT;
	}
}

static const struct file_operations nvt_ts_flash_fops = {
	.owner = THIS_MODULE,
	.read = nvt_ts_flash_read,
};

void nvt_ts_flash_proc_init(struct nvt_ts_data *ts)
{
	struct proc_dir_entry *proc_entry = proc_create_data(DEVICE_NAME, 0444, NULL, &nvt_ts_flash_fops, ts);

	if (!proc_entry)
		input_err(true, &ts->client->dev, "%s: failed to create proc\n", __func__);
	else
		input_info(true, &ts->client->dev, "%s: create %s\n", __func__, DEVICE_NAME);
}

void nvt_ts_flash_proc_remove(void)
{
	remove_proc_entry(DEVICE_NAME, NULL);
}