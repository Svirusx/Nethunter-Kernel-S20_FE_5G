/* drivers/input/sec_input/slsi/slsi_main.c
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

int sec_ts_i2c_write(struct sec_ts_data *ts, u8 reg, u8 *data, int len)
{
	u8 *buf;
	int ret;
	unsigned char retry;
	struct i2c_msg msg;
	int i;
	const int len_max = 0xffff;

	if (len + 1 > len_max) {
		input_err(true, &ts->client->dev,
				"%s: The i2c buffer size is exceeded.\n", __func__);
		return -ENOMEM;
	}

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buf = kzalloc(len + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	buf[0] = reg;
	memcpy(buf + 1, data, len);

	msg.addr = ts->client->addr;
	msg.flags = 0 | I2C_M_DMA_SAFE;
	msg.len = len + 1;
	msg.buf = buf;

	mutex_lock(&ts->i2c_mutex);
	for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
		ret = i2c_transfer(ts->client->adapter, &msg, 1);
		if (ret == 1)
			break;

		if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
			input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
			mutex_unlock(&ts->i2c_mutex);
			goto err;
		}

		usleep_range(1 * 1000, 1 * 1000);

		if (retry > 1) {
			input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retry + 1, ret);
			ts->comm_err_count++;
			if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT) {
				char result[32];

				snprintf(result, sizeof(result), "RESULT=I2C");
				sec_cmd_send_event_to_user(&ts->sec, NULL, result);
			}
		}
	}

	mutex_unlock(&ts->i2c_mutex);

	if (retry == SEC_TS_I2C_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
#ifdef USE_POR_AFTER_I2C_RETRY
		if (ts->probe_done && !ts->reset_is_on_going && !shutdown_is_on_going_tsp)
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
#endif
	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_I2C_WRITE_CMD) {
		pr_info("sec_input:i2c_cmd: W: %02X | ", reg);
		for (i = 0; i < len; i++)
			pr_cont("%02X ", data[i]);
		pr_cont("\n");
	}

	if (ret == 1) {
		kfree(buf);
		return 0;
	}
err:
	kfree(buf);
	return -EIO;
}

int sec_ts_i2c_read(struct sec_ts_data *ts, u8 reg, u8 *data, int len)
{
	u8 *buf;
	int ret;
	unsigned char retry;
	struct i2c_msg msg[2];
	int remain = len;
	int i;
	u8 *buff;

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buf = kzalloc(1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	buff = kzalloc(len, GFP_KERNEL);
	if (!buff) {
		kfree(buf);
		return -ENOMEM;
	}

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	buf[0] = reg;

	msg[0].addr = ts->client->addr;
	msg[0].flags = 0 | I2C_M_DMA_SAFE;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD | I2C_M_DMA_SAFE;
	msg[1].buf = buff;

	mutex_lock(&ts->i2c_mutex);
	if (len <= ts->i2c_burstmax) {
		msg[1].len = len;
		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(ts->client->adapter, msg, 2);
			if (ret == 2)
				break;
			usleep_range(1 * 1000, 1 * 1000);
			if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
				input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->i2c_mutex);
				goto err;
			}

			if (retry > 1) {
				input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n",
					__func__, retry + 1, ret);
				ts->comm_err_count++;
				if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT) {
					char result[32];

					snprintf(result, sizeof(result), "RESULT=I2C");
					sec_cmd_send_event_to_user(&ts->sec, NULL, result);
				}
			}
		}
	} else {
		/*
		 * I2C read buffer is 256 byte. do not support long buffer over than 256.
		 * So, try to seperate reading data about 256 bytes.
		 */
		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(ts->client->adapter, msg, 1);
			if (ret == 1)
				break;
			usleep_range(1 * 1000, 1 * 1000);
			if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
				input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->i2c_mutex);
				goto err;
			}

			if (retry > 1) {
				input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n",
					__func__, retry + 1, ret);
				ts->comm_err_count++;
			}
		}

		do {
			if (remain > ts->i2c_burstmax)
				msg[1].len = ts->i2c_burstmax;
			else
				msg[1].len = remain;

			remain -= ts->i2c_burstmax;

			for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
				ret = i2c_transfer(ts->client->adapter, &msg[1], 1);
				if (ret == 1)
					break;
				usleep_range(1 * 1000, 1 * 1000);
				if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
					input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
					mutex_unlock(&ts->i2c_mutex);
					goto err;
				}

				if (retry > 1) {
					input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n",
						__func__, retry + 1, ret);
					ts->comm_err_count++;
				}
			}
			msg[1].buf += msg[1].len;
		} while (remain > 0);
	}

	mutex_unlock(&ts->i2c_mutex);

	if (retry == SEC_TS_I2C_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: I2C read over retry limit\n", __func__);
		ret = -EIO;
#ifdef USE_POR_AFTER_I2C_RETRY
		if (ts->probe_done && !ts->reset_is_on_going && !shutdown_is_on_going_tsp)
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
#endif
	}

	memcpy(data, buff, len);
	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_I2C_READ_CMD) {
		pr_info("sec_input:i2c_cmd: R: %02X | ", reg);
		for (i = 0; i < len; i++)
			pr_cont("%02X ", data[i]);
		pr_cont("\n");
	}

	kfree(buf);
	kfree(buff);
	return ret;
err:
	kfree(buf);
	kfree(buff);
	return -EIO;
}

int sec_ts_i2c_write_burst(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;
	int retry;
	u8 *buf;
	const int len_max = 0xffff;

	if (len > len_max) {
		input_err(true, &ts->client->dev,
				"%s: The i2c buffer size is exceeded.\n", __func__);
		return -ENOMEM;
	}

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buf = kzalloc(len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, data, len);
	mutex_lock(&ts->i2c_mutex);

	for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
		ret = i2c_master_send_dmasafe(ts->client, buf , len);
		if (ret == len)
			break;

		usleep_range(1 * 1000, 1 * 1000);

		if (retry > 1) {
			input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retry + 1, ret);
			ts->comm_err_count++;
		}
	}

	mutex_unlock(&ts->i2c_mutex);
	if (retry == SEC_TS_I2C_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
	}

	kfree(buf);

	return ret;
}

int sec_ts_i2c_read_bulk(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;
	unsigned char retry;
	int remain = len;
	struct i2c_msg msg;
	u8 *buff;

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buff = kzalloc(len, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	msg.addr = ts->client->addr;
	msg.flags = I2C_M_RD | I2C_M_DMA_SAFE;
	msg.buf = buff;

	mutex_lock(&ts->i2c_mutex);

	do {
		if (remain > ts->i2c_burstmax)
			msg.len = ts->i2c_burstmax;
		else
			msg.len = remain;

		remain -= ts->i2c_burstmax;

		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(ts->client->adapter, &msg, 1);
			if (ret == 1)
				break;
			usleep_range(1 * 1000, 1 * 1000);

			if (retry > 1) {
				input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n",
					__func__, retry + 1, ret);
				ts->comm_err_count++;
			}
		}

		if (retry == SEC_TS_I2C_RETRY_CNT) {
			input_err(true, &ts->client->dev, "%s: I2C read over retry limit\n", __func__);
			ret = -EIO;

			break;
		}
		msg.buf += msg.len;
	} while (remain > 0);

	mutex_unlock(&ts->i2c_mutex);

	if (ret == 1) {
		memcpy(data, buff, len);
		kfree(buff);
		return 0;
	}

	kfree(buff);
	return -EIO;
}

void sec_ts_reinit(struct sec_ts_data *ts)
{
	u8 w_data[2] = {0x00, 0x00};
	int ret = 0;

	input_info(true, &ts->client->dev,
		"%s: charger=0x%x, touch_functions=0x%x, Power mode=0x%x\n",
		__func__, ts->charger_mode, ts->touch_functions, ts->power_status);

	ts->touch_noise_status = 0;
	ts->touch_pre_noise_status = 0;
	ts->wet_mode = 0;

	/* charger mode */
	if (ts->charger_mode != SEC_TS_BIT_CHARGER_MODE_NO) {
		w_data[0] = ts->charger_mode;
		ret = ts->sec_ts_i2c_write(ts, SET_TS_CMD_SET_CHARGER_MODE, &w_data[0], 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
					__func__, SET_TS_CMD_SET_CHARGER_MODE);
			return;
		}
	}

	/* Cover mode */
	if (ts->touch_functions & SEC_TS_BIT_SETFUNC_COVER) {
		w_data[0] = ts->cover_cmd;
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, &w_data[0], 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
					__func__, SEC_TS_CMD_SET_COVERTYPE);
			return;
		}

	}

	ret = sec_ts_set_touch_function(ts);
	if (ret < 0) { 
		input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
				__func__, SEC_TS_CMD_SET_TOUCHFUNCTION);
		return;
	}

	sec_ts_set_custom_library(ts);
	sec_ts_set_press_property(ts);

	if (ts->plat_data->support_fod && ts->fod_set_val) {
		sec_ts_set_fod_rect(ts);
	}

	/* Power mode */
	if (ts->power_status == SEC_TS_STATE_LPM) {
		w_data[0] = TO_LOWPOWER_MODE;
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &w_data[0], 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
					__func__, SEC_TS_CMD_SET_POWER_MODE);
			return;
		}

		sec_ts_delay(50);
		sec_ts_set_aod_rect(ts);
	} else {
		sec_ts_set_grip_type(ts, ONLY_EDGE_HANDLER);

		sec_ts_set_external_noise_mode(ts, EXT_NOISE_MODE_MAX);

		if (ts->brush_mode) {
			input_info(true, &ts->client->dev, "%s: set brush mode\n", __func__);
			ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_BRUSH_MODE, &ts->brush_mode, 1);
			if (ret < 0) {
				input_err(true, &ts->client->dev,
						"%s: failed to set brush mode\n", __func__);
				return;
			}
		}

		if (ts->touchable_area) {
			input_info(true, &ts->client->dev, "%s: set 16:9 mode\n", __func__);
			ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHABLE_AREA, &ts->touchable_area, 1);
			if (ret < 0) {
				input_err(true, &ts->client->dev,
						"%s: failed to set 16:9 mode\n", __func__);
				return;
			}
		}
	}

	if (ts->ed_enable) {
		input_info(true, &ts->client->dev, "%s: set ear detect mode\n", __func__);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_SET_EAR_DETECT_MODE, &ts->ed_enable, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: failed to set ear detect mode\n", __func__);
			return;
		}
	}
}

#define MAX_EVENT_COUNT 31
static void sec_ts_read_event(struct sec_ts_data *ts)
{
	int ret;
	u8 t_id;
	u8 event_id;
	u8 left_event_count;
	u8 read_event_buff[MAX_EVENT_COUNT][SEC_TS_EVENT_BUFF_SIZE] = { { 0 } };
	u8 *event_buff;
	struct sec_ts_event_coordinate *p_event_coord;
	struct sec_ts_gesture_status *p_gesture_status;
	struct sec_ts_event_status *p_event_status;
	int curr_pos;
	int remain_event_count = 0;
	int pre_ttype = 0;
	int pre_action = 0;
	static bool error_report;
	u8 force_strength = 1;
	char location[SEC_TS_LOCATION_DETECT_SIZE] = { 0, };

	if (ts->power_status == SEC_TS_STATE_LPM) {
		wake_lock_timeout(&ts->wakelock, msecs_to_jiffies(500));

		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret == 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			return;
		}

		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			return;
		}

		input_info(true, &ts->client->dev, "%s: run LPM interrupt handler, %d\n", __func__, ret);
		/* run lpm interrupt handler */
	}

	ret = t_id = event_id = curr_pos = remain_event_count = 0;
	/* repeat READ_ONE_EVENT until buffer is empty(No event) */
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, (u8 *)read_event_buff[0], SEC_TS_EVENT_BUFF_SIZE);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c read one event failed\n", __func__);
		return;
	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_ONEEVENT)
		input_info(true, &ts->client->dev, "ONE: %02X %02X %02X %02X %02X %02X %02X %02X\n",
				read_event_buff[0][0], read_event_buff[0][1],
				read_event_buff[0][2], read_event_buff[0][3],
				read_event_buff[0][4], read_event_buff[0][5],
				read_event_buff[0][6], read_event_buff[0][7]);

	if (read_event_buff[0][0] == 0) {
		input_info(true, &ts->client->dev, "%s: event buffer is empty\n", __func__);
		return;
	}

	if((read_event_buff[0][0] & 0x3) == 0x3) {
		input_err(true, &ts->client->dev, "%s: event buffer not vaild %02X %02X %02X %02X %02X %02X %02X %02X\n",
				__func__, read_event_buff[0][0], read_event_buff[0][1],
				read_event_buff[0][2], read_event_buff[0][3],
				read_event_buff[0][4], read_event_buff[0][5],
				read_event_buff[0][6], read_event_buff[0][7]);

		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);

		sec_ts_unlocked_release_all_finger(ts);
		return;
	}

	left_event_count = read_event_buff[0][7] & 0x1F;
	remain_event_count = left_event_count;

	if (left_event_count > MAX_EVENT_COUNT - 1) {
		input_err(true, &ts->client->dev, "%s: event buffer overflow\n", __func__);

		/* write clear event stack command when read_event_count > MAX_EVENT_COUNT */
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);

		sec_ts_unlocked_release_all_finger(ts);

		return;
	}

	if (left_event_count > 0) {
		ret = sec_ts_i2c_read(ts, SEC_TS_READ_ALL_EVENT, (u8 *)read_event_buff[1],
				sizeof(u8) * (SEC_TS_EVENT_BUFF_SIZE) * (left_event_count));
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: i2c read one event failed\n", __func__);
			return;
		}
	}

	do {
		event_buff = read_event_buff[curr_pos];
		event_id = event_buff[0] & 0x3;

		if (ts->debug_flag & SEC_TS_DEBUG_PRINT_ALLEVENT)
			input_info(true, &ts->client->dev, "ALL: %02X %02X %02X %02X %02X %02X %02X %02X\n",
					event_buff[0], event_buff[1], event_buff[2], event_buff[3],
					event_buff[4], event_buff[5], event_buff[6], event_buff[7]);

		switch (event_id) {
		case SEC_TS_STATUS_EVENT:
			p_event_status = (struct sec_ts_event_status *)event_buff;

			/* tchsta == 0 && ttype == 0 && eid == 0 : buffer empty */
			if (p_event_status->stype > 0)
				input_info(true, &ts->client->dev, "%s: STATUS %x %x %x %x %x %x %x %x\n", __func__,
						event_buff[0], event_buff[1], event_buff[2],
						event_buff[3], event_buff[4], event_buff[5],
						event_buff[6], event_buff[7]);

			/* watchdog reset -> send SENSEON command */
			if ((p_event_status->stype == TYPE_STATUS_EVENT_INFO) &&
					(p_event_status->status_id == SEC_TS_ACK_BOOT_COMPLETE) &&
					(p_event_status->status_data_1 == 0x20)) {
				ts->ic_reset_count++;
				sec_ts_unlocked_release_all_finger(ts);

				sec_ts_reinit(ts);

				ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
				if (ret < 0)
					input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
			}

			/* event queue full-> all finger release */
			if ((p_event_status->stype == TYPE_STATUS_EVENT_ERR) &&
					(p_event_status->status_id == SEC_TS_ERR_EVENT_QUEUE_FULL)) {
				input_err(true, &ts->client->dev, "%s: IC Event Queue is full\n", __func__);
				sec_ts_unlocked_release_all_finger(ts);
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_ERR) &&
					(p_event_status->status_id == SEC_TS_ERR_EVENT_ESD)) {
				input_err(true, &ts->client->dev, "%s: ESD detected. run reset\n", __func__);
#ifdef USE_RESET_DURING_POWER_ON
				if (!shutdown_is_on_going_tsp)
					schedule_work(&ts->reset_work.work);
#endif
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_INFO) &&
					(p_event_status->status_id == SEC_TS_ACK_WET_MODE)) {
				ts->wet_mode = p_event_status->status_data_1;
				input_info(true, &ts->client->dev, "%s: water wet mode %d\n",
						__func__, ts->wet_mode);
				if (ts->wet_mode)
					ts->wet_count++;
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == SEC_TS_VENDOR_STATE_CHANGED)) {
				ts->print_info_currnet_mode = ((event_buff[2] & 0xFF) << 8) + (event_buff[3] & 0xFF);

				if (p_event_status->status_data_1 == 2 && p_event_status->status_data_2 == 2) {
					input_info(true, &ts->client->dev, "%s: Normal changed\n", __func__);
				} else if (p_event_status->status_data_1 == 5 && p_event_status->status_data_2 == 2) {
					input_info(true, &ts->client->dev, "%s: lp changed\n", __func__);
				} else if (p_event_status->status_data_1 == 6) {
					input_info(true, &ts->client->dev, "%s: sleep changed\n", __func__);
				}
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == SEC_TS_VENDOR_ACK_NOISE_STATUS_NOTI)) {
				ts->touch_noise_status = !!p_event_status->status_data_1;
				input_info(true, &ts->client->dev, "%s: TSP NOISE MODE %s[%02X %02X]\n",
						__func__, ts->touch_noise_status == 0 ? "OFF" : "ON",
						p_event_status->status_data_2, p_event_status->status_data_3);

				if (ts->touch_noise_status)
					ts->noise_count++;
			}
			
			if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == SEC_TS_VENDOR_ACK_PRE_NOISE_STATUS_NOTI)) {
				ts->touch_pre_noise_status = !!p_event_status->status_data_1;
				input_info(true, &ts->client->dev, "%s: TSP PRE NOISE MODE %s\n",
						__func__, ts->touch_pre_noise_status == 0 ? "OFF" : "ON");
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == SEC_TS_VENDOR_ACK_CHARGER_STATUS_NOTI)) {
				input_info(true, &ts->client->dev, "%s: TSP CHARGER MODE:%d\n",
						__func__, p_event_status->status_data_1);
			}

			if(ts->plat_data->support_ear_detect && ts->ed_enable){
				if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == STATUS_EVENT_VENDOR_PROXIMITY)) {
						input_info(true, &ts->client->dev, "%s: EAR_DETECT(%d)\n",
							__func__, p_event_status->status_data_1);
						input_report_abs(ts->input_dev_proximity, ABS_MT_CUSTOM,
									p_event_status->status_data_1);
						input_sync(ts->input_dev_proximity);
				}
			}
			break;

		case SEC_TS_COORDINATE_EVENT:
			if (ts->power_status != SEC_TS_STATE_POWER_ON) {
				input_err(true, &ts->client->dev,
						"%s: device is closed %x %x %x %x %x %x %x %x\n", __func__,
						event_buff[0], event_buff[1], event_buff[2],
						event_buff[3], event_buff[4], event_buff[5],
						event_buff[6], event_buff[7]);
				if (!error_report) {
					error_report = true;
				}
				break;
			}
			error_report = false;

			p_event_coord = (struct sec_ts_event_coordinate *)event_buff;

			t_id = (p_event_coord->tid - 1);

			if (t_id < MAX_SUPPORT_TOUCH_COUNT + MAX_SUPPORT_HOVER_COUNT) {
				pre_ttype = ts->coord[t_id].ttype;
				ts->coord[t_id].id = t_id;
				pre_action = ts->coord[t_id].action;
				ts->coord[t_id].action = p_event_coord->tchsta;
				ts->coord[t_id].x = (p_event_coord->x_11_4 << 4) | (p_event_coord->x_3_0);
				ts->coord[t_id].y = (p_event_coord->y_11_4 << 4) | (p_event_coord->y_3_0);
				ts->coord[t_id].z = p_event_coord->z & 0x3F;
				ts->coord[t_id].ttype = p_event_coord->ttype_3_2 << 2 | p_event_coord->ttype_1_0 << 0;
				ts->coord[t_id].major = p_event_coord->major;
				ts->coord[t_id].minor = p_event_coord->minor;

				if (!ts->coord[t_id].palm && (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM))
					ts->coord[t_id].palm_count++;

				ts->coord[t_id].palm = (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM);
				ts->coord[t_id].left_event = p_event_coord->left_event;

				ts->coord[t_id].noise_level = max(ts->coord[t_id].noise_level, p_event_coord->noise_level);
				ts->coord[t_id].max_strength = max(ts->coord[t_id].max_strength, p_event_coord->max_strength);
				ts->coord[t_id].hover_id_num = max(ts->coord[t_id].hover_id_num, (u8)p_event_coord->hover_id_num);
				ts->coord[t_id].noise_status = p_event_coord->noise_status;

				if (ts->coord[t_id].z <= 0)
					ts->coord[t_id].z = 1;

				if ((ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_NORMAL)
						|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM)
						|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_WET)
						|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_GLOVE)) {
					if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_RELEASE) {

						input_mt_slot(ts->input_dev, t_id);
						if (ts->plat_data->support_mt_pressure)
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
						input_report_abs(ts->input_dev, ABS_MT_CUSTOM, 0);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);

						if (ts->touch_count > 0)
							ts->touch_count--;
						if (ts->touch_count == 0) {
							input_report_key(ts->input_dev, BTN_TOUCH, 0);
							input_report_key(ts->input_dev, BTN_TOOL_FINGER, 0);
							ts->check_multi = 0;
							ts->print_info_cnt_release = 0;
						}

						location_detect(ts, location, ts->coord[t_id].x, ts->coord[t_id].y);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
						input_info(true, &ts->client->dev,
								"[R] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d mx:%d my:%d p:%d noise:(%x,%d%d) nlvl:%d, maxS:%d, hid:%d\n",
								t_id, location,
								ts->coord[t_id].x - ts->coord[t_id].p_x,
								ts->coord[t_id].y - ts->coord[t_id].p_y,
								ts->coord[t_id].mcount, ts->touch_count,
								ts->coord[t_id].x, ts->coord[t_id].y,
								ts->coord[t_id].max_energy_x, ts->coord[t_id].max_energy_y,
								ts->coord[t_id].palm_count,
								ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
								ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#else
						input_info(true, &ts->client->dev,
								"[R] tID:%d loc:%s dd:%d,%d mp:%d,%d mc:%d tc:%d p:%d noise:(%x,%d%d) nlvl:%d, maxS:%d, hid:%d\n",
								t_id, location,
								ts->coord[t_id].x - ts->coord[t_id].p_x,
								ts->coord[t_id].y - ts->coord[t_id].p_y,
								ts->coord[t_id].max_energy_x - ts->coord[t_id].p_x,
								ts->coord[t_id].max_energy_y - ts->coord[t_id].p_y,
								ts->coord[t_id].mcount, ts->touch_count,
								ts->coord[t_id].palm_count,
								ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
								ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#endif
						ts->coord[t_id].action = SEC_TS_COORDINATE_ACTION_NONE;
						ts->coord[t_id].mcount = 0;
						ts->coord[t_id].palm_count = 0;
						ts->coord[t_id].noise_level = 0;
						ts->coord[t_id].max_strength = 0;
						ts->coord[t_id].hover_id_num = 0;
					} else if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS) {
						ts->touch_count++;
						ts->all_finger_count++;
						ts->coord[t_id].max_energy_x = 0;
						ts->coord[t_id].max_energy_y = 0;

						input_mt_slot(ts->input_dev, t_id);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
						input_report_key(ts->input_dev, BTN_TOUCH, 1);
						input_report_key(ts->input_dev, BTN_TOOL_FINGER, 1);

						input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->coord[t_id].x);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->coord[t_id].y);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, ts->coord[t_id].major);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, ts->coord[t_id].minor);

						if (ts->brush_mode) {
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM,
									(p_event_coord->max_energy_flag << 16) | (force_strength << 8) | (ts->coord[t_id].z << 1) | ts->coord[t_id].palm);
						} else {
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM,
									(p_event_coord->max_energy_flag << 16) | (force_strength << 8) | (BRUSH_Z_DATA << 1) | ts->coord[t_id].palm);
						}

						if (ts->plat_data->support_mt_pressure)
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, ts->coord[t_id].z);

						if ((ts->touch_count > 4) && (ts->check_multi == 0)) {
							ts->check_multi = 1;
							ts->multi_count++;
						}
						location_detect(ts, location, ts->coord[t_id].x, ts->coord[t_id].y);
						ts->coord[t_id].p_x = ts->coord[t_id].x;
						ts->coord[t_id].p_y = ts->coord[t_id].y;

						if (p_event_coord->max_energy_flag) {
							ts->coord[t_id].max_energy_x = ts->coord[t_id].x;
							ts->coord[t_id].max_energy_y = ts->coord[t_id].y;
						}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
						input_info(true, &ts->client->dev,
								"[P] tID:%d.%d x:%d y:%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d\n",
								t_id, (ts->input_dev->mt->trkid - 1) & TRKID_MAX,
								ts->coord[t_id].x, ts->coord[t_id].y, ts->coord[t_id].z,
								ts->coord[t_id].major, ts->coord[t_id].minor,
								location, ts->touch_count,
								ts->coord[t_id].ttype,
								ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
								ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#else
						input_info(true, &ts->client->dev,
								"[P] tID:%d.%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d\n",
								t_id, (ts->input_dev->mt->trkid - 1) & TRKID_MAX,
								ts->coord[t_id].z, ts->coord[t_id].major,
								ts->coord[t_id].minor, location, ts->touch_count,
								ts->coord[t_id].ttype,
								ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
								ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#endif
					} else if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_MOVE) {
						if (p_event_coord->max_energy_flag) {
							ts->coord[t_id].max_energy_x = ts->coord[t_id].x;
							ts->coord[t_id].max_energy_y = ts->coord[t_id].y;
						}

						if (pre_action == SEC_TS_COORDINATE_ACTION_NONE || pre_action == SEC_TS_COORDINATE_ACTION_RELEASE) {
							location_detect(ts, location, ts->coord[t_id].x, ts->coord[t_id].y);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
							input_info(true, &ts->client->dev,
									"[M] tID:%d.%d x:%d y:%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d\n",
									t_id, ts->input_dev->mt->trkid & TRKID_MAX,
									ts->coord[t_id].x, ts->coord[t_id].y, ts->coord[t_id].z,
									ts->coord[t_id].major, ts->coord[t_id].minor, location, ts->touch_count,
									ts->coord[t_id].ttype, ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
									ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#else
							input_info(true, &ts->client->dev,
									"[M] tID:%d.%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d\n",
									t_id, ts->input_dev->mt->trkid & TRKID_MAX, ts->coord[t_id].z, 
									ts->coord[t_id].major, ts->coord[t_id].minor, location, ts->touch_count,
									ts->coord[t_id].ttype, ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
									ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#endif
						}

						input_mt_slot(ts->input_dev, t_id);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
						input_report_key(ts->input_dev, BTN_TOUCH, 1);
						input_report_key(ts->input_dev, BTN_TOOL_FINGER, 1);

						input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->coord[t_id].x);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->coord[t_id].y);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, ts->coord[t_id].major);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, ts->coord[t_id].minor);

						if (ts->brush_mode) {
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM,
									(p_event_coord->max_energy_flag << 16) |(force_strength << 8) | (ts->coord[t_id].z << 1) | ts->coord[t_id].palm);
						} else {
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM,
									(p_event_coord->max_energy_flag << 16) |(force_strength << 8) | (BRUSH_Z_DATA << 1) | ts->coord[t_id].palm);
						}

						if (ts->plat_data->support_mt_pressure)
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, ts->coord[t_id].z);
						ts->coord[t_id].mcount++;
					} else {
						input_dbg(true, &ts->client->dev,
								"%s: do not support coordinate action(%d)\n", __func__, ts->coord[t_id].action);
					}

					if ((ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS)
							|| (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_MOVE)) {
						if (ts->coord[t_id].ttype != pre_ttype) {
							input_info(true, &ts->client->dev, "%s : tID:%d ttype(%x->%x)\n",
									__func__, ts->coord[t_id].id,
									pre_ttype, ts->coord[t_id].ttype);
						}
					}
				} else {
					input_dbg(true, &ts->client->dev,
							"%s: do not support coordinate type(%d)\n", __func__, ts->coord[t_id].ttype);
				}
			} else {
				input_err(true, &ts->client->dev, "%s: tid(%d) is out of range\n", __func__, t_id);
			}
			break;

		case SEC_TS_GESTURE_EVENT:
			p_gesture_status = (struct sec_ts_gesture_status *)event_buff;

			switch (p_gesture_status->stype) {
			case SEC_TS_GESTURE_CODE_SWIPE:
				ts->scrub_id = SPONGE_EVENT_TYPE_SPAY;
				input_info(true, &ts->client->dev, "%s: SPAY: %d\n", __func__, ts->scrub_id);
				input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 1);
				ts->all_spay_count++;
				break;
			case SEC_TS_GESTURE_CODE_DOUBLE_TAP:
				if (p_gesture_status->gesture_id == SEC_TS_GESTURE_ID_AOD) {
					ts->scrub_id = SPONGE_EVENT_TYPE_AOD_DOUBLETAB;
					ts->scrub_x = (p_gesture_status->gesture_data_1 << 4)
								| (p_gesture_status->gesture_data_3 >> 4);
					ts->scrub_y = (p_gesture_status->gesture_data_2 << 4)
								| (p_gesture_status->gesture_data_3 & 0x0F);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
					input_info(true, &ts->client->dev, "%s: AOD: %d\n", __func__, ts->scrub_id);
#else
					input_info(true, &ts->client->dev, "%s: AOD: %d, %d, %d\n",
							__func__, ts->scrub_id, ts->scrub_x, ts->scrub_y);
#endif
					input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 1);
					ts->all_aod_tap_count++;
				} else if (p_gesture_status->gesture_id == SEC_TS_GESTURE_ID_DOUBLETAP_TO_WAKEUP) {
					input_info(true, &ts->client->dev, "%s: AOT\n", __func__);
					input_report_key(ts->input_dev, KEY_WAKEUP, 1);
					input_sync(ts->input_dev);
					input_report_key(ts->input_dev, KEY_WAKEUP, 0);
				}
				break;
			case SEC_TS_GESTURE_CODE_SINGLE_TAP:
				ts->scrub_id = SPONGE_EVENT_TYPE_SINGLE_TAP;
				ts->scrub_x = (p_gesture_status->gesture_data_1 << 4)
							| (p_gesture_status->gesture_data_3 >> 4);
				ts->scrub_y = (p_gesture_status->gesture_data_2 << 4)
							| (p_gesture_status->gesture_data_3 & 0x0F);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &ts->client->dev, "%s: SINGLE TAP: %d\n", __func__, ts->scrub_id);
#else
				input_info(true, &ts->client->dev, "%s: SINGLE TAP: %d, %d, %d\n",
						__func__, ts->scrub_id, ts->scrub_x, ts->scrub_y);
#endif
				input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 1);
				break;
			case SEC_TS_GESTURE_CODE_PRESS:
				input_info(true, &ts->client->dev, "%s: FOD: %sPRESS\n",
						__func__, p_gesture_status->gesture_id ? "" : "LONG");
				break;
			}

			input_sync(ts->input_dev);
			input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 0);
			break;
		default:
			input_err(true, &ts->client->dev, "%s: unknown event %x %x %x %x %x %x\n", __func__,
					event_buff[0], event_buff[1], event_buff[2],
					event_buff[3], event_buff[4], event_buff[5]);
			break;
		}

		curr_pos++;
		remain_event_count--;
	} while (remain_event_count >= 0);

	input_sync(ts->input_dev);

	if(ts->touch_count == 0 && ts->tsp_temp_data_skip){
		ts->tsp_temp_data_skip = false;
		sec_ts_set_temp(ts, false);
		input_err(true, &ts->client->dev, "%s: sec_ts_set_temp, no touch\n", __func__);	
	}
}

irqreturn_t sec_ts_irq_thread(int irq, void *ptr)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)ptr;

	if (gpio_get_value(ts->plat_data->irq_gpio) == 1)
		return IRQ_HANDLED;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (secure_filter_interrupt(ts) == IRQ_HANDLED) {
		wait_for_completion_interruptible_timeout(&ts->secure_interrupt,
				msecs_to_jiffies(5 * MSEC_PER_SEC));

		input_info(true, &ts->client->dev,
				"%s: secure interrupt handled\n", __func__);

		return IRQ_HANDLED;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return IRQ_HANDLED;
#endif
	mutex_lock(&ts->eventlock);

	sec_ts_read_event(ts);

	mutex_unlock(&ts->eventlock);

	return IRQ_HANDLED;
}

static int sec_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sec_ts_data *ts;
	struct sec_ts_plat_data *pdata;
	struct sec_tclm_data *tdata = NULL;
	int ret = 0;
	bool force_update = false;
	bool valid_firmware_integrity = false;
	unsigned char data[5] = { 0 };
	unsigned char deviceID[5] = { 0 };
	unsigned char result = 0;

	input_info(true, &client->dev, "%s\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev, "%s: EIO err!\n", __func__);
		return -EIO;
	}

	/* parse dt */
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_ts_plat_data), GFP_KERNEL);

		if (!pdata) {
			input_err(true, &client->dev, "%s: Failed to allocate platform data\n", __func__);
			goto error_allocate_pdata;
		}

		client->dev.platform_data = pdata;

		ret = sec_ts_parse_dt(client);
		if (ret) {
			input_err(true, &client->dev, "%s: Failed to parse dt\n", __func__);
			goto error_allocate_mem;
		}
		tdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_tclm_data), GFP_KERNEL);
		if (!tdata)
			goto error_allocate_tdata;

		sec_tclm_parse_dt(client, tdata);
	} else {
		pdata = client->dev.platform_data;
		if (!pdata) {
			input_err(true, &client->dev, "%s: No platform data found\n", __func__);
			goto error_allocate_pdata;
		}
	}

	if (!pdata->power) {
		input_err(true, &client->dev, "%s: No power contorl found\n", __func__);
		goto error_allocate_mem;
	}

	pdata->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(pdata->pinctrl))
		input_err(true, &client->dev, "%s: could not get pinctrl\n", __func__);

	ts = kzalloc(sizeof(struct sec_ts_data), GFP_KERNEL);
	if (!ts)
		goto error_allocate_mem;

	ts->client = client;
	ts->plat_data = pdata;
	ts->crc_addr = 0x0001FE00;
	ts->fw_addr = 0x00002000;
	ts->para_addr = 0x18000;
	ts->flash_page_size = SEC_TS_FW_BLK_SIZE_DEFAULT;
	ts->sec_ts_i2c_read = sec_ts_i2c_read;
	ts->sec_ts_i2c_write = sec_ts_i2c_write;
	ts->sec_ts_i2c_write_burst = sec_ts_i2c_write_burst;
	ts->sec_ts_i2c_read_bulk = sec_ts_i2c_read_bulk;
	ts->i2c_burstmax = pdata->i2c_burstmax;
	ts->tdata = tdata;
	if (!ts->tdata)
		goto err_null_tdata;

#ifdef TCLM_CONCEPT
	sec_tclm_initialize(ts->tdata);
	ts->tdata->client = ts->client;
	ts->tdata->tclm_read = sec_tclm_data_read;
	ts->tdata->tclm_write = sec_tclm_data_write;
	ts->tdata->tclm_execute_force_calibration = sec_tclm_execute_force_calibration;
	ts->tdata->tclm_parse_dt = sec_tclm_parse_dt;
#endif

#ifdef USE_POWER_RESET_WORK
	INIT_DELAYED_WORK(&ts->reset_work, sec_ts_reset_work);
#endif
	INIT_DELAYED_WORK(&ts->work_read_info, sec_ts_read_info_work);
	INIT_DELAYED_WORK(&ts->work_print_info, sec_ts_print_info_work);
	INIT_DELAYED_WORK(&ts->work_read_functions, sec_ts_get_touch_function);

	i2c_set_clientdata(client, ts);

	if (gpio_is_valid(ts->plat_data->tsp_id))
		ts->tspid_val = gpio_get_value(ts->plat_data->tsp_id);

	if (gpio_is_valid(ts->plat_data->tsp_icid))
		ts->tspicid_val = gpio_get_value(ts->plat_data->tsp_icid);

	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		input_err(true, &ts->client->dev, "%s: allocate device err!\n", __func__);
		ret = -ENOMEM;
		goto err_allocate_input_dev;
	}

	if (ts->plat_data->support_dex) {
		ts->input_dev_pad = input_allocate_device();
		if (!ts->input_dev_pad) {
			input_err(true, &ts->client->dev, "%s: allocate device err!\n", __func__);
			ret = -ENOMEM;
			goto err_allocate_input_dev_pad;
		}
	}

	if (ts->plat_data->support_ear_detect) {
		ts->input_dev_proximity = input_allocate_device();
		if (!ts->input_dev_proximity) {
			input_err(true, &ts->client->dev, "%s: allocate input_dev_proximity err!\n", __func__);
			ret = -ENOMEM;
			goto err_allocate_input_dev_proximity;
		}
	}

	ts->touch_count = 0;
	ts->sec_ts_i2c_write = sec_ts_i2c_write;
	ts->sec_ts_i2c_read = sec_ts_i2c_read;
	ts->sec_ts_read_sponge = sec_ts_read_from_sponge;
	ts->sec_ts_write_sponge = sec_ts_write_to_sponge;

	mutex_init(&ts->device_mutex);
	mutex_init(&ts->i2c_mutex);
	mutex_init(&ts->eventlock);
	mutex_init(&ts->modechange);
	mutex_init(&ts->sponge_mutex);

	wake_lock_init(&ts->wakelock, WAKE_LOCK_SUSPEND, "tsp_wakelock");
	init_completion(&ts->resume_done);
	complete_all(&ts->resume_done);

	input_info(true, &client->dev, "%s: init resource\n", __func__);

	sec_ts_pinctrl_configure(ts, true);

	/* power enable */
	sec_ts_power(ts, true);
	if (!pdata->regulator_boot_on)
		sec_ts_delay(TOUCH_POWER_ON_DWORK_TIME);
	ts->power_status = SEC_TS_STATE_POWER_ON;
	ts->tdata->external_factory = false;

	sec_ts_wait_for_ready(ts, SEC_TS_ACK_BOOT_COMPLETE);

	input_info(true, &client->dev, "%s: power enable\n", __func__);

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_DEVICE_ID, deviceID, 5);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: failed to read device ID(%d)\n", __func__, ret);
	else
		input_info(true, &ts->client->dev,
				"%s: TOUCH DEVICE ID : %02X, %02X, %02X, %02X, %02X\n", __func__,
				deviceID[0], deviceID[1], deviceID[2], deviceID[3], deviceID[4]);

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_FIRMWARE_INTEGRITY, &result, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to integrity check (%d)\n", __func__, ret);
	} else {
		if (result & 0x80) {
			valid_firmware_integrity = true;
		} else if (result & 0x40) {
			valid_firmware_integrity = false;
			input_err(true, &ts->client->dev, "%s: invalid firmware (0x%x)\n", __func__, result);
		} else {
			valid_firmware_integrity = false;
			input_err(true, &ts->client->dev, "%s: invalid integrity result (0x%x)\n", __func__, result);
		}
	}

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, &data[0], 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
	} else {
		ret = sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, &data[1], 4);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: failed to touch status(%d)\n",
					__func__, ret);
		}
	}
	input_info(true, &ts->client->dev,
			"%s: TOUCH STATUS : %02X || %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3], data[4]);

	if (data[0] == SEC_TS_STATUS_BOOT_MODE)
		ts->checksum_result = 1;

	if ((data[0] == SEC_TS_STATUS_APP_MODE && data[2] == TOUCH_SYSTEM_MODE_FLASH) ||
			(valid_firmware_integrity == false))
		force_update = true;
	else
		force_update = false;

	ret = sec_ts_firmware_update_on_probe(ts, force_update);
	if (ret < 0)
		goto err_init;

	ret = sec_ts_read_information(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to read information 0x%x\n", __func__, ret);
		goto err_init;
	}

	ts->touch_functions |= SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC;
	ret = sec_ts_set_touch_function(ts);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to send touch func_mode command", __func__);

	/* Sense_on */
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
		goto err_init;
	}

	ts->pFrame = kzalloc(ts->tx_count * ts->rx_count * 2, GFP_KERNEL);
	if (!ts->pFrame) {
		ret = -ENOMEM;
		goto err_allocate_frame;
	}

	sec_ts_init_proc(ts);

	if (ts->plat_data->support_dex) {
		ts->input_dev_pad->name = "sec_touchpad";
		sec_ts_set_input_prop(ts, ts->input_dev_pad, INPUT_PROP_POINTER);
	}

	if (ts->plat_data->support_ear_detect) {
		ts->input_dev_proximity->name = "sec_touchproximity";
		sec_ts_set_input_prop_proximity(ts, ts->input_dev_proximity);
	}

	ts->input_dev->name = "sec_touchscreen";
	sec_ts_set_input_prop(ts, ts->input_dev, INPUT_PROP_DIRECT);
#ifdef USE_OPEN_CLOSE
	ts->input_dev->open = sec_ts_input_open;
	ts->input_dev->close = sec_ts_input_close;
#endif
	ts->input_dev_touch = ts->input_dev;

	ret = input_register_device(ts->input_dev);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: Unable to register %s input device\n", __func__, ts->input_dev->name);
		goto err_input_register_device;
	}

	if (ts->plat_data->support_dex) {
		ret = input_register_device(ts->input_dev_pad);
		if (ret) {
			input_err(true, &ts->client->dev, "%s: Unable to register %s input device\n", __func__, ts->input_dev_pad->name);
			goto err_input_pad_register_device;
		}
	}

	if (ts->plat_data->support_ear_detect) {
		ret = input_register_device(ts->input_dev_proximity);
		if (ret) {
			input_err(true, &ts->client->dev, "%s: Unable to register %s input device\n", __func__, ts->input_dev_proximity->name);
			goto err_input_proximity_register_device;
		}
	}

	input_info(true, &ts->client->dev, "%s: request_irq = %d\n", __func__, client->irq);

	ret = request_threaded_irq(client->irq, NULL, sec_ts_irq_thread,
			ts->plat_data->irq_type, SEC_TS_I2C_NAME, ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Unable to request threaded irq\n", __func__);
		goto err_irq;
	}

#ifdef CONFIG_SAMSUNG_TUI
	tsp_info = ts;
#endif
	/* need remove below resource @ remove driver */
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	sec_ts_raw_device_init(ts);
#endif
	sec_ts_fn_init(ts);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (sysfs_create_group(&ts->input_dev->dev.kobj, &secure_attr_group) < 0)
		input_err(true, &ts->client->dev, "%s: do not make secure group\n", __func__);
	else
		secure_touch_init(ts);
#endif
	sec_ts_check_custom_library(ts);

	device_init_wakeup(&client->dev, true);

	if (!shutdown_is_on_going_tsp)
		schedule_delayed_work(&ts->work_read_info, msecs_to_jiffies(50));

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	dump_callbacks.inform_dump = dump_tsp_log;
	INIT_DELAYED_WORK(&ts->ghost_check, sec_ts_check_rawdata);
	p_ghost_check = &ts->ghost_check;
#endif

	/* register dev for ltp */
	sec_ts_ioctl_init(ts);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	sec_secure_touch_register(ts, ts->plat_data->ss_touch_num, &ts->input_dev->dev.kobj);
#endif

	ts->psy = power_supply_get_by_name("battery");
	if (!ts->psy)
		input_err(true, &ts->client->dev, "%s: Cannot find power supply\n", __func__);

	ts->probe_done = true;

	input_err(true, &ts->client->dev, "%s: done\n", __func__);
	input_log_fix();

	return 0;

	/* need to be enabled when new goto statement is added */
#if 0
	sec_ts_fn_remove(ts);
	free_irq(client->irq, ts);
#endif
err_irq:
	if (ts->plat_data->support_ear_detect) {
		input_unregister_device(ts->input_dev_proximity);
		ts->input_dev_proximity = NULL;
	}
err_input_proximity_register_device:
	if (ts->plat_data->support_dex) {
		input_unregister_device(ts->input_dev_pad);
		ts->input_dev_pad = NULL;
	}
err_input_pad_register_device:
	input_unregister_device(ts->input_dev);
	ts->input_dev = NULL;
	ts->input_dev_touch = NULL;
err_input_register_device:
	kfree(ts->pFrame);
err_allocate_frame:
err_init:
	wake_lock_destroy(&ts->wakelock);
	sec_ts_power(ts, false);
	if (ts->plat_data->support_ear_detect) {
		if (ts->input_dev_proximity)
			input_free_device(ts->input_dev_proximity);
	}
err_allocate_input_dev_proximity:
	if (ts->plat_data->support_dex) {
		if (ts->input_dev_pad)
			input_free_device(ts->input_dev_pad);
	}
err_allocate_input_dev_pad:
	if (ts->input_dev)
		input_free_device(ts->input_dev);
err_allocate_input_dev:
err_null_tdata:
	kfree(ts);

error_allocate_mem:
	if (gpio_is_valid(pdata->irq_gpio))
		gpio_free(pdata->irq_gpio);
	if (gpio_is_valid(pdata->tsp_id))
		gpio_free(pdata->tsp_id);
	if (gpio_is_valid(pdata->tsp_icid))
		gpio_free(pdata->tsp_icid);

error_allocate_tdata:
error_allocate_pdata:
	if (ret == -ECONNREFUSED)
		sec_ts_delay(100);
	ret = -ENODEV;
#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	p_ghost_check = NULL;
#endif
#ifdef CONFIG_SAMSUNG_TUI
	tsp_info = NULL;
#endif
	input_err(true, &client->dev, "%s: failed(%d)\n", __func__, ret);
	input_log_fix();
	return ret;
}

#ifdef USE_OPEN_CLOSE
int sec_ts_input_open(struct input_dev *dev)
{
	struct sec_ts_data *ts = input_get_drvdata(dev);
	int ret;

	if (!ts->info_work_done) {
		input_err(true, &ts->client->dev, "%s not finished info work\n", __func__);
		return 0;
	}

	mutex_lock(&ts->modechange);

	ts->input_closed = false;
	ts->prox_power_off = 0;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	secure_touch_stop(ts, 0);
#endif

	if (ts->power_status == SEC_TS_STATE_LPM) {
#ifdef USE_RESET_EXIT_LPM
		if (!shutdown_is_on_going_tsp)
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
#else
		sec_ts_set_lowpowermode(ts, TO_TOUCH_MODE);
		sec_ts_set_grip_type(ts, ONLY_EDGE_HANDLER);
#endif
	} else {
		ret = sec_ts_start_device(ts);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: Failed to start device\n", __func__);
	}

	if (ts->fix_active_mode)
		sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);

	sec_ts_set_temp(ts, true);

	mutex_unlock(&ts->modechange);

	cancel_delayed_work(&ts->work_print_info);
	ts->print_info_cnt_open = 0;
	ts->print_info_cnt_release = 0;
	if (!shutdown_is_on_going_tsp)
		schedule_work(&ts->work_print_info.work);
	return 0;
}

void sec_ts_input_close(struct input_dev *dev)
{
	struct sec_ts_data *ts = input_get_drvdata(dev);

	if (!ts->info_work_done) {
		input_err(true, &ts->client->dev, "%s not finished info work\n", __func__);
		return;
	}
	if (ts->shutdown_is_on_going) {
		input_err(true, &ts->client->dev, "%s shutdown was called\n", __func__);
		return;
	}

	mutex_lock(&ts->modechange);

	ts->input_closed = true;
	ts->sip_mode = 0;

#ifdef TCLM_CONCEPT
	sec_tclm_debug_info(ts->tdata);
#endif
#ifdef MINORITY_REPORT
	minority_report_sync_latest_value(ts);
#endif
	cancel_delayed_work(&ts->work_print_info);
	sec_ts_print_info(ts);
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	secure_touch_stop(ts, 1);
#endif
#ifdef CONFIG_SAMSUNG_TUI
	stui_cancel_session();
#endif

#ifdef USE_POWER_RESET_WORK
	cancel_delayed_work(&ts->reset_work);
#endif

	if (ts->lowpower_mode || ts->ed_enable)
		sec_ts_set_lowpowermode(ts, TO_LOWPOWER_MODE);
	else
		sec_ts_stop_device(ts);

	mutex_unlock(&ts->modechange);
}
#endif

static int sec_ts_remove(struct i2c_client *client)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);

	input_info(true, &ts->client->dev, "%s\n", __func__);
	ts->shutdown_is_on_going = true;
	shutdown_is_on_going_tsp = true;

	sec_ts_ioctl_remove(ts);

	disable_irq_nosync(ts->client->irq);
	cancel_delayed_work_sync(&ts->work_read_info);
	cancel_delayed_work_sync(&ts->work_print_info);
	cancel_delayed_work_sync(&ts->work_read_functions);
	free_irq(ts->client->irq, ts);
	input_info(true, &ts->client->dev, "%s: irq disabled\n", __func__);

#ifdef USE_POWER_RESET_WORK
	cancel_delayed_work_sync(&ts->reset_work);
	flush_delayed_work(&ts->reset_work);

	input_info(true, &ts->client->dev, "%s: flush queue\n", __func__);
#endif
	sec_ts_fn_remove(ts);

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	p_ghost_check = NULL;
#endif
	device_init_wakeup(&client->dev, false);
	wake_lock_destroy(&ts->wakelock);

	ts->lowpower_mode = false;
	ts->probe_done = false;

	if (ts->plat_data->support_dex) {
		input_mt_destroy_slots(ts->input_dev_pad);
		input_unregister_device(ts->input_dev_pad);
	}
	if (ts->plat_data->support_ear_detect) {
		input_mt_destroy_slots(ts->input_dev_proximity);
		input_unregister_device(ts->input_dev_proximity);
	}

	ts->input_dev = ts->input_dev_touch;
	input_mt_destroy_slots(ts->input_dev);
	input_unregister_device(ts->input_dev);

	ts->input_dev_pad = NULL;
	ts->input_dev = NULL;
	ts->input_dev_touch = NULL;
	ts->plat_data->power(ts, false);

#ifdef CONFIG_SAMSUNG_TUI
	tsp_info = NULL;
#endif

	kfree(ts);
	return 0;
}

static void sec_ts_shutdown(struct i2c_client *client)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);

	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_ts_remove(client);
}

int sec_ts_stop_device(struct sec_ts_data *ts)
{
	input_info(true, &ts->client->dev, "%s\n", __func__);

	mutex_lock(&ts->device_mutex);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: already power off\n", __func__);
		goto out;
	}

	disable_irq(ts->client->irq);

	ts->power_status = SEC_TS_STATE_POWER_OFF;

	if (ts->prox_power_off) {
		input_report_key(ts->input_dev, KEY_INT_CANCEL, 1);
		input_sync(ts->input_dev);
		input_report_key(ts->input_dev, KEY_INT_CANCEL, 0);
		input_sync(ts->input_dev);
	}

	sec_ts_locked_release_all_finger(ts);

	ts->plat_data->power(ts, false);

	sec_ts_pinctrl_configure(ts, false);

out:
	mutex_unlock(&ts->device_mutex);
	return 0;
}

int sec_ts_start_device(struct sec_ts_data *ts)
{
	int ret = -1;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_ts_pinctrl_configure(ts, true);

	mutex_lock(&ts->device_mutex);

	if (ts->power_status == SEC_TS_STATE_POWER_ON) {
		input_err(true, &ts->client->dev, "%s: already power on\n", __func__);
		goto out;
	}

	sec_ts_locked_release_all_finger(ts);

	ts->plat_data->power(ts, true);
	sec_ts_delay(TOUCH_POWER_ON_DWORK_TIME);
	ts->power_status = SEC_TS_STATE_POWER_ON;
	ts->touch_noise_status = 0;
	ts->touch_pre_noise_status = 0;

	ret = sec_ts_wait_for_ready(ts, SEC_TS_ACK_BOOT_COMPLETE);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: Failed to wait_for_ready\n", __func__);
		goto err;
	}

	sec_ts_reinit(ts);

err:
	/* Sense_on */
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);

	enable_irq(ts->client->irq);
out:
	mutex_unlock(&ts->device_mutex);
	return ret;
}

#ifdef CONFIG_PM
static int sec_ts_pm_suspend(struct device *dev)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
#if 0
	int retval;

	if (ts->input_dev) {
		retval = mutex_lock_interruptible(&ts->input_dev->mutex);
		if (retval) {
			input_err(true, &ts->client->dev,
					"%s : mutex error\n", __func__);
			goto out;
		}

		if (!ts->input_dev->disabled) {
			ts->input_dev->disabled = true;
			if (ts->input_dev->users && ts->input_dev->close) {
				input_err(true, &ts->client->dev,
						"%s called without input_close\n",
						__func__);
				ts->input_dev->close(ts->input_dev);
			}
			ts->input_dev->users = 0;
		}

		mutex_unlock(&ts->input_dev->mutex);
	}

out:
#endif
	if (ts->lowpower_mode)
		reinit_completion(&ts->resume_done);

	return 0;
}

static int sec_ts_pm_resume(struct device *dev)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);

	if (ts->lowpower_mode)
		complete_all(&ts->resume_done);

	return 0;
}
#endif

static const struct i2c_device_id sec_ts_id[] = {
	{ SEC_TS_I2C_NAME, 0 },
	{ },
};

#ifdef CONFIG_PM
static const struct dev_pm_ops sec_ts_dev_pm_ops = {
	.suspend = sec_ts_pm_suspend,
	.resume = sec_ts_pm_resume,
};
#endif

#ifdef CONFIG_OF
static const struct of_device_id sec_ts_match_table[] = {
	{ .compatible = "sec,sec_ts",},
	{ },
};
#else
#define sec_ts_match_table NULL
#endif

static struct i2c_driver sec_ts_driver = {
	.probe		= sec_ts_probe,
	.remove		= sec_ts_remove,
	.shutdown	= sec_ts_shutdown,
	.id_table	= sec_ts_id,
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SEC_TS_I2C_NAME,
#ifdef CONFIG_OF
		.of_match_table = sec_ts_match_table,
#endif
#ifdef CONFIG_PM
		.pm = &sec_ts_dev_pm_ops,
#endif
	},
};

static int __init sec_ts_init(void)
{
#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		pr_err("%s %s: Do not load driver due to : lpm %d\n",
				SECLOG, __func__, lpcharge);
		return -ENODEV;
	}
#endif
	pr_err("%s %s\n", SECLOG, __func__);

	return i2c_add_driver(&sec_ts_driver);
}

static void __exit sec_ts_exit(void)
{
	i2c_del_driver(&sec_ts_driver);
}

MODULE_AUTHOR("Hyobae, Ahn<hyobae.ahn@samsung.com>");
MODULE_DESCRIPTION("Samsung Electronics TouchScreen driver");
MODULE_LICENSE("GPL");

module_init(sec_ts_init);
module_exit(sec_ts_exit);
