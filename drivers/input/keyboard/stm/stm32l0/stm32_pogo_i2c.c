/******************** (C) COPYRIGHT 2019 Samsung Electronics ********************
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

#include "stm32_pogo_i2c.h"

static struct stm32_pogo_notifier pogo_notifier;

struct stm32_tc_resolution g_tc_resolution;

void pogo_get_tc_resolution(int *x, int *y)
{
	*x = g_tc_resolution.x;
	*y = g_tc_resolution.y;
}
EXPORT_SYMBOL(pogo_get_tc_resolution);

static void pogo_set_conn_state(int state)
{
	if (state)
		pogo_notifier.conn = POGO_NOTIFIER_ID_ATTACHED;
	else
		pogo_notifier.conn = POGO_NOTIFIER_ID_DETACHED;
	pr_info("%s: 0x%02X\n", __func__, pogo_notifier.conn);
}

void stm32_delay(int ms)
{
	if (ms > 20)
		msleep(ms);
	else
		usleep_range(ms * 1000, ms * 1000);
}

static void stm32_print_info(struct stm32_dev *stm32)
{
	struct irq_desc *desc = irq_to_desc(stm32->dev_irq);

	input_info(true, &stm32->client->dev, "mcu_fw(bin):%x, mcu_fw(ic):%x, %s_v%d.%d.%d.%d, "
			"TC_v%02X%02X.%X, con:%d/%d, int:%d, depth:%d, rst:%d, hall:%d\n",
			stm32->mdata.phone_ver[3], stm32->mdata.ic_ver[3], stm32->dtdata->model_name,
			stm32->ic_fw_ver.fw_major_ver, stm32->ic_fw_ver.fw_minor_ver,
			stm32->ic_fw_ver.model_id, stm32->ic_fw_ver.hw_rev,
			stm32->tc_fw_ver_of_ic.major_ver, stm32->tc_fw_ver_of_ic.minor_ver,
			stm32->tc_fw_ver_of_ic.data_ver,
			stm32->connect_state, gpio_get_value(stm32->dtdata->gpio_conn),
			gpio_get_value(stm32->dtdata->gpio_int), desc->depth, stm32->reset_count, stm32->hall_closed);
}

static void stm32_print_info_work(struct work_struct *work)
{
	struct stm32_dev *stm32 = container_of(work, struct stm32_dev,
			print_info_work.work);

	stm32_print_info(stm32);
	schedule_delayed_work(&stm32->print_info_work, STM32_PRINT_INFO_DELAY);
}

int pogo_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
				pogo_notifier_device_t listener)
{
	struct pogo_data_struct send_data;
	int ret = 0;

	pr_info("%s %s %s: listener=%d register\n", SECLOG, STM32_DRV_NAME, __func__, listener);

	SET_POGO_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&pogo_notifier.pogo_notifier_call_chain, nb);
	if (ret < 0)
		pr_err("%s %s %s: blocking_notifier_chain_register error(%d)\n",
				SECLOG, STM32_DRV_NAME, __func__, ret);

	send_data.size = 0;
	send_data.data = 0;
	nb->notifier_call(nb, pogo_notifier.conn, &send_data);

	return ret;
}

int pogo_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s %s %s: listener=%d unregister\n", SECLOG, STM32_DRV_NAME,
		__func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&pogo_notifier.pogo_notifier_call_chain, nb);
	if (ret < 0)
		pr_err("%s %s %s: blocking_notifier_chain_unregister error(%d)\n",
				SECLOG, STM32_DRV_NAME, __func__, ret);
	DESTROY_POGO_NOTIFIER_BLOCK(nb);

	return ret;
}

static int pogo_notifier_notify(struct stm32_dev *stm32, pogo_notifier_id_t notify_id,
					char *data, int len)
{
	int ret = 0;
	struct pogo_data_struct send_data;

	if (stm32->debug_level & STM32_DEBUG_LEVEL_IRQ_DATA_LOG)
		input_info(true, &stm32->client->dev,
				"%s: ID=%d, len=%d\n", __func__, notify_id, len);

	if (len > STM32_MAX_EVENT_SIZE) {
		input_err(true, &stm32->client->dev, "%s: length is too long %d\n",
				__func__, len);
		return -EINVAL;
	}

	if (len > 0 && data && (stm32->debug_level & STM32_DEBUG_LEVEL_IRQ_DATA_LOG)) {
		int i;

		pr_info("%s %s %s: size:%d, ",
			SECLOG, STM32_DRV_NAME, __func__, len);
		for (i = 0; i < len; i++)
			pr_cont("%02X ", data[i]);
		pr_cont("\n");
	}

	send_data.size = len;
	send_data.data = data;

	ret = blocking_notifier_call_chain(&pogo_notifier.pogo_notifier_call_chain,
						notify_id, &send_data);

	switch (ret) {
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
		input_err(true, &stm32->client->dev,
				"%s: notify error occur(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_DONE:
	case NOTIFY_OK:
		input_dbg(false, &stm32->client->dev,
				"%s: notify done(0x%x)\n", __func__, ret);
		break;
	default:
		input_err(true, &stm32->client->dev,
				"%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}

static int stm32_dev_regulator(struct stm32_dev *device_data, int onoff);
static void stm32_enable_irq(struct stm32_dev *stm32, int enable);

static void stm32_power_reset(struct stm32_dev *stm32)
{
	if (stm32->reset_count < 100000)
		stm32->reset_count++;
	input_err(true, &stm32->client->dev, "%s, %d\n", __func__, stm32->reset_count);

	gpio_direction_output(stm32->dtdata->mcu_nrst, 0);
	stm32_delay(3);
	gpio_direction_output(stm32->dtdata->mcu_nrst, 1);
	stm32_delay(10);
}

int stm32_i2c_read_bulk(struct i2c_client *client, u8 *data, u8 length)
{
	struct stm32_dev *device_data = i2c_get_clientdata(client);
	int ret;
	int retry = 3;
	struct i2c_msg msgs;
	u8 *buf;

	reinit_completion(&device_data->i2c_done);

	if ((!device_data->connect_state) && (client->addr != 0x51) && mutex_is_locked(&device_data->conn_lock)) {
		input_err(true, &client->dev, "%s: not connected\n", __func__);
		complete_all(&device_data->i2c_done);
		return -ENODEV;
	}

	buf = kzalloc(length, GFP_KERNEL);
	if (!buf) {
		complete_all(&device_data->i2c_done);
		return -ENOMEM;
	}

	msgs.addr = client->addr;
	msgs.flags = client->flags | I2C_M_RD | I2C_M_DMA_SAFE;;
	msgs.len = length;
	msgs.buf = buf;

	while (retry--) {
		ret = i2c_transfer(client->adapter, &msgs, 1);
		if (ret != 1) {
			pogo_notifier_notify(device_data, POGO_NOTIFIER_ID_RESET, 0, 0);
			input_err(true, &client->dev, "scl:%d, sda:%d, int:%d\n",
					gpio_get_value(device_data->dtdata->gpio_scl),
					gpio_get_value(device_data->dtdata->gpio_sda),
					gpio_get_value(device_data->dtdata->gpio_int));
			ret = -EIO;
			stm32_delay(10);
			if ((!device_data->connect_state) && (client->addr != 0x51) && mutex_is_locked(&device_data->conn_lock)) {
				input_err(true, &client->dev, "%s: connect_state=%d\n", __func__, device_data->connect_state);
				kfree(buf);
				complete_all(&device_data->i2c_done);
				return ret;
			}
			continue;
		}
		break;
	}

	if (device_data->debug_level & STM32_DEBUG_LEVEL_I2C_LOG) {
		int i;

		pr_info("%s %s: ", SECLOG, __func__);
		for (i = 0; i < length; i++)
			pr_cont("%02X ", buf[i]);
		pr_cont("\n");
	}

	memcpy(data, buf, length);

	kfree(buf);
	if ((ret < 0) && (client->addr != 0x51)) 
		stm32_power_reset(device_data);

	complete_all(&device_data->i2c_done);
	return ret;
}

int stm32_i2c_write_burst(struct i2c_client *client, u8 *data, int len)
{
	struct stm32_dev *device_data = i2c_get_clientdata(client);
	int ret;
	int retry = 3;
	u8 *buf;
	reinit_completion(&device_data->i2c_done);

	if ((!device_data->connect_state) && (client->addr != 0x51) && mutex_is_locked(&device_data->conn_lock)) {
		input_err(true, &client->dev, "%s: not connected\n", __func__);
		complete_all(&device_data->i2c_done);
		return -ENODEV;
	}

	if (len > STM32_DEV_FW_UPDATE_PACKET_SIZE) {
		input_err(true, &client->dev,
				"%s: The i2c buffer size is exceeded.\n", __func__);
		complete_all(&device_data->i2c_done);
		return -ENOMEM;
	}

	buf = kzalloc(len, GFP_KERNEL);
	if (!buf) {
		complete_all(&device_data->i2c_done);
		return -ENOMEM;
	}

	memcpy(buf, data, len);

	for (retry = 0; retry < STM32_DEV_I2C_RETRY_CNT; retry++) {
		ret = i2c_master_send_dmasafe(client, buf , len);
		if (ret == len)
			break;

		usleep_range(1 * 1000, 1 * 1000);

		
		if ((!device_data->connect_state) && (client->addr != 0x51) && mutex_is_locked(&device_data->conn_lock)) {
			input_err(true, &client->dev, "%s: connect_state=%d\n", __func__, device_data->connect_state);
			kfree(buf);
			complete_all(&device_data->i2c_done);
			return ret;
		}

		if (retry > 1)
			input_err(true, &client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retry + 1, ret);
	}

	if (retry == STM32_DEV_I2C_RETRY_CNT) {
		input_err(true, &client->dev, "%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
	}

	if (device_data->debug_level & STM32_DEBUG_LEVEL_I2C_LOG) {
		int i;

		pr_info("%s %s: ", SECLOG, __func__);
		for (i = 0; i < len; i++)
			pr_cont("%02X ", buf[i]);
		pr_cont("\n");
	}

	kfree(buf);

	if ((ret < 0) && (client->addr != 0x51))
		stm32_power_reset(device_data);

	complete_all(&device_data->i2c_done);
	
	return ret;
}

static int stm32_i2c_header_write(struct i2c_client *client, u8 ep_id, u16 payload_size)
{
	u8 buff[3] = { 0 };

	payload_size += 3;

	buff[0] = payload_size & 0xFF;
	buff[1] = (payload_size >> 8) & 0xFF;
	buff[2] = ep_id;

	return stm32_i2c_write_burst(client, buff, 3);
}

static int stm32_i2c_reg_write(struct i2c_client *client, u8 ep_id, u8 regaddr)
{
	struct stm32_dev *stm32 = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&stm32->i2c_lock);
	ret = stm32_i2c_header_write(client, ep_id, 1);
	if (ret < 0)
		goto out;

	ret = stm32_i2c_write_burst(client, &regaddr, 1);

out:
	mutex_unlock(&stm32->i2c_lock);
	return ret;
}

static int stm32_i2c_reg_read(struct i2c_client *client, u8 ep_id, u8 regaddr, u16 length, u8 *values)
{
	struct stm32_dev *stm32 = i2c_get_clientdata(client);
	int ret;
	u16 payload_size = 0;
	u8 buff[3] = { 0 };

	mutex_lock(&stm32->i2c_lock);
	ret = stm32_i2c_header_write(client, ep_id, 1);
	if (ret < 0)
		goto out;

	ret = stm32_i2c_write_burst(client, &regaddr, 1);
	if (ret < 0)
		goto out;
	
	ret = stm32_i2c_read_bulk(client, buff, 3);
	if (ret < 0)
		goto out;

	memcpy(&payload_size, buff, 2);
	payload_size -= 3;
	if (payload_size == 0 || payload_size > 0xFF) {
		input_err(true, &client->dev, "wrong value of payload size %d\n", payload_size);
		ret = -EIO;
		goto out;
	}

	if (payload_size > length) {
		input_err(true, &client->dev, "payload size %d is over buff length %d\n",
				payload_size, length);
		ret = -EIO;
		goto out;
	}

	ret = stm32_i2c_read_bulk(client, values, payload_size);
out:
	mutex_unlock(&stm32->i2c_lock);
	return ret;
}

static int stm32_dev_regulator(struct stm32_dev *device_data, int onoff)
{
	struct device *dev = &device_data->client->dev;
	int ret = 0;

	if (IS_ERR_OR_NULL(device_data->dtdata->vdd_vreg))
		return ret;

	if (onoff) {
		if (!regulator_is_enabled(device_data->dtdata->vdd_vreg)) {
			ret = regulator_enable(device_data->dtdata->vdd_vreg);
			if (ret) {
				input_err(true, dev,
						"%s: Failed to enable vddo: %d\n",
						__func__, ret);
				return ret;
			}
		} else {
			input_err(true, dev, "%s: vdd is already enabled\n", __func__);
		}
	} else {
		if (regulator_is_enabled(device_data->dtdata->vdd_vreg)) {
			ret = regulator_disable(device_data->dtdata->vdd_vreg);
			if (ret) {
				input_err(true, dev,
						"%s: Failed to disable vddo: %d\n",
						__func__, ret);
				return ret;
			}
		} else {
			input_err(true, dev, "%s: vdd is already disabled\n", __func__);
		}
	}

	input_err(true, dev, "%s %s: vdd:%s\n", __func__, onoff ? "on" : "off",
			regulator_is_enabled(device_data->dtdata->vdd_vreg) ? "on" : "off");

	return ret;
}

static void stm32_enable_irq(struct stm32_dev *stm32, int enable)
{
	static int depth;

	if (enable != INT_ENABLE && enable != INT_DISABLE_NOSYNC && enable != INT_DISABLE_SYNC)
		return;

	mutex_lock(&stm32->irq_lock);
	if (enable == INT_ENABLE) {
		if (depth) {
			--depth;
			enable_irq(stm32->dev_irq);
			input_info(true, &stm32->client->dev, "%s: enable irq\n", __func__);
		}
	} else {
		if (!depth) {
			++depth;
			if (enable == INT_DISABLE_NOSYNC)
				disable_irq_nosync(stm32->dev_irq);
			else
				disable_irq(stm32->dev_irq);
			input_info(true, &stm32->client->dev, "%s: disable irq %ssync\n",
				__func__, enable == INT_DISABLE_NOSYNC ? "no" : "");
		}
	}
	mutex_unlock(&stm32->irq_lock);
}

static int stm32_read_crc(struct stm32_dev *stm32)
{
	int ret;
	u8 rbuf[4] = { 0 };

	ret = stm32_i2c_reg_read(stm32->client, ID_MCU, STM32_CMD_CHECK_CRC, 4, rbuf);
	if (ret < 0)
		return ret;

	stm32->crc_of_ic = rbuf[3] << 24 | rbuf[2] << 16 | rbuf[1] << 8 | rbuf[0];

	input_info(true, &stm32->client->dev,
			"%s: [IC] BOOT CRC32 = 0x%08X\n", __func__, stm32->crc_of_ic);
	return 0;
}

static int stm32_read_version(struct stm32_dev *stm32)
{
	int ret;
	u8 rbuf[4] = { 0 };

	ret = stm32_i2c_reg_read(stm32->client, ID_MCU, STM32_CMD_CHECK_VERSION, 4, rbuf);
	if (ret < 0)
		return ret;

	stm32->ic_fw_ver.hw_rev = rbuf[0];
	stm32->ic_fw_ver.model_id = rbuf[1];
	stm32->ic_fw_ver.fw_minor_ver = rbuf[2];
	stm32->ic_fw_ver.fw_major_ver = rbuf[3];

	input_info(true, &stm32->client->dev,
			"%s: [IC] version:%d.%d, model_id:%02d, hw_rev:%02d\n",
			__func__, stm32->ic_fw_ver.fw_major_ver, stm32->ic_fw_ver.fw_minor_ver,
			stm32->ic_fw_ver.model_id, stm32->ic_fw_ver.hw_rev);
	return 0;
}

static int stm32_read_tc_resolution(struct stm32_dev *stm32)
{
	int ret;
	u8 rbuf[4] = { 0 };

	ret = stm32_i2c_reg_read(stm32->client, ID_TOUCHPAD, STM32_CMD_GET_TC_RESOLUTION, 4, rbuf);
	if (ret < 0)
		return ret;

	g_tc_resolution.x = rbuf[3] << 8 | rbuf[2];
	g_tc_resolution.y = rbuf[1] << 8 | rbuf[0];

	input_info(true, &stm32->client->dev,
			"%s: x:%d, y:%d\n", __func__, g_tc_resolution.x, g_tc_resolution.y);
	return 0;
}

static int stm32_read_tc_crc(struct stm32_dev *stm32)
{
	int ret;
	u8 rbuf[2] = { 0 };

	ret = stm32_i2c_reg_read(stm32->client, ID_TOUCHPAD, STM32_CMD_GET_TC_FW_CRC16, 2, rbuf);
	if (ret < 0)
		return ret;

	stm32->tc_crc = rbuf[1] << 8 | rbuf[0];

	input_info(true, &stm32->client->dev,
			"%s: [TC IC] CRC = 0x%04X\n", __func__, stm32->tc_crc);
	return 0;
}

static int stm32_read_tc_version(struct stm32_dev *stm32)
{
	int ret;
	u8 rbuf[6] = { 0 };

	ret = stm32_i2c_reg_read(stm32->client, ID_TOUCHPAD, STM32_CMD_GET_TC_FW_VERSION, 6, rbuf);
	if (ret < 0)
		return ret;

	stm32->tc_fw_ver_of_ic.minor_ver = rbuf[5] << 8 | rbuf[4];
	stm32->tc_fw_ver_of_ic.major_ver = rbuf[3] << 8 | rbuf[2];
	stm32->tc_fw_ver_of_ic.data_ver = rbuf[1] << 8 | rbuf[0];

	input_info(true, &stm32->client->dev,
			"%s: [TC IC] version:0x%02X%02X, data ver:0x%X\n",
			__func__, stm32->tc_fw_ver_of_ic.major_ver,
			stm32->tc_fw_ver_of_ic.minor_ver,
			stm32->tc_fw_ver_of_ic.data_ver);
	return 0;
}

u32 stm32_crc32(u8 *src, size_t len) {
	u8 *bp;
	size_t idx;
	u32 crc = 0xFFFFFFFF;

	for (idx = 0; idx < len; idx++) {
		bp = src  + (idx ^ 0x3);
		crc = (crc << 8) ^ crctab[((crc >> 24) ^ *bp) & 0xFF];
	}
	return crc;
}

static void stm32_get_fw_info_of_bin(struct stm32_dev *stm32)
{
	if (!stm32->fw)
		return;

	memcpy(stm32->fw_header, &stm32->fw->data[STM32_FW_APP_CFG_OFFSET],
		sizeof(struct stm32_fw_header));
	stm32->crc_of_bin = stm32_crc32((u8 *)stm32->fw->data, stm32->fw->size);
	
	input_info(true, &stm32->client->dev,
			"%s: [BIN] %s, version:%d.%d, model_id:%d, hw_rev:%d, CRC:0x%X\n",
			__func__, stm32->fw_header->magic_word,
			stm32->fw_header->boot_app_ver.fw_major_ver,
			stm32->fw_header->boot_app_ver.fw_minor_ver,
			stm32->fw_header->boot_app_ver.model_id,
			stm32->fw_header->boot_app_ver.hw_rev, stm32->crc_of_bin);
}

static void stm32_get_tc_fw_info_of_bin(struct stm32_dev *stm32)
{
	if (!stm32->tc_fw)
		return;

	stm32->tc_fw_ver_of_bin.data_ver = (u16)(stm32->tc_fw->data[52] | (stm32->tc_fw->data[53] << 8));
	stm32->tc_fw_ver_of_bin.major_ver = (u16)(stm32->tc_fw->data[56] | (stm32->tc_fw->data[57] << 8));
	stm32->tc_fw_ver_of_bin.minor_ver = (u16)(stm32->tc_fw->data[60] | (stm32->tc_fw->data[61] << 8));

	input_info(true, &stm32->client->dev,
			"%s: [TC BIN] version:0x%02X%02X, data_ver:0x%X\n",
			__func__, stm32->tc_fw_ver_of_bin.major_ver,
			stm32->tc_fw_ver_of_bin.minor_ver,
			stm32->tc_fw_ver_of_bin.data_ver);
}

static int stm32_load_fw_from_fota(struct stm32_dev *stm32, u8 ed_id)
{
	struct firmware *fw;
	struct file *fp;
	mm_segment_t old_fs;
	long fw_size, nread;
	int error = 0;
	char fw_path[128] = { 0 };

	if (ed_id == ID_MCU) {
		fw = stm32->fw;
	} else if (ed_id == ID_TOUCHPAD) {
		fw = stm32->tc_fw;
	} else {
		input_err(true, &stm32->client->dev, "%s: wrong ed_id %d\n", __func__, ed_id);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	snprintf(fw_path, sizeof(fw_path), "%s/%s%s.bin",
			STM32_FOTA_BIN_PATH, stm32->dtdata->model_name,
			ed_id == ID_TOUCHPAD ? "TOUCH" : "");

	input_info(true, &stm32->client->dev, "%s: path:%s\n", __func__, fw_path);

	fp = filp_open(fw_path, O_RDONLY, 0400);
	if (IS_ERR(fp)) {
		input_err(true, &stm32->client->dev, "%s: failed to open fw\n", __func__);
		error = -ENOENT;
		goto err_open;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;
	if (fw_size <= 0) {
		error = -ENOENT;
		goto err_get_size;
	}

	fw->size = (size_t)fw_size;
	input_info(true, &stm32->client->dev, "%s: size %ld Bytes\n", __func__, fw_size);

	nread = vfs_read(fp, (char __user *)fw->data, fw_size, &fp->f_pos);
	if (nread != fw_size) {
		input_err(true, &stm32->client->dev,
				"%s: failed to read firmware file, nread %ld Bytes\n",
				__func__, nread);
		error = -EIO;
		goto err_get_size;
	}

	if (ed_id == ID_MCU)
		stm32_get_fw_info_of_bin(stm32);
	else if (ed_id == ID_TOUCHPAD)
		stm32_get_tc_fw_info_of_bin(stm32);

err_get_size:
	filp_close(fp, NULL);
err_open:
	set_fs(old_fs);
	return error;
}

static bool stm32_check_fw_update(struct stm32_dev *stm32)
{
	u16 fw_ver_bin, fw_ver_ic;

	if (stm32_read_version(stm32) < 0) {
		input_err(true, &stm32->client->dev, "%s: failed to read version\n", __func__);
		return false;
	}

	if (stm32_read_crc(stm32) < 0) {
		input_err(true, &stm32->client->dev, "%s: failed to read crc\n", __func__);
		return false;
	}

	if (stm32_load_fw_from_fota(stm32, ID_MCU) < 0) {
		input_err(true, &stm32->client->dev, "%s: failed to read bin data\n", __func__);
		return false;
	}

	fw_ver_bin = stm32->fw_header->boot_app_ver.fw_major_ver << 8 |
			stm32->fw_header->boot_app_ver.fw_minor_ver;
	fw_ver_ic = stm32->ic_fw_ver.fw_major_ver << 8 | stm32->ic_fw_ver.fw_minor_ver;

	input_info(true, &stm32->client->dev,
			"%s: [BIN] V:%d.%d CRC:0x%X / [IC] V:%d.%d CRC:0x%X\n",
			__func__, stm32->fw_header->boot_app_ver.fw_major_ver,
			stm32->fw_header->boot_app_ver.fw_minor_ver, stm32->crc_of_bin,
			stm32->ic_fw_ver.fw_major_ver, stm32->ic_fw_ver.fw_minor_ver,
			stm32->crc_of_ic);

	if (strncmp(STM32_MAGIC_WORD, stm32->fw_header->magic_word, 7) != 0) {
		input_info(true, &stm32->client->dev, "%s: binary file is wrong : %s\n",
				__func__, stm32->fw_header->magic_word);
		return false;
	}

	if (0) {//stm32->fw_header->boot_app_ver.model_id != stm32->ic_fw_ver.model_id) {
		input_info(true, &stm32->client->dev,
				"%s: [BIN] %d / [IC] %d : diffrent model id. do not update\n",
				__func__, stm32->fw_header->boot_app_ver.model_id,
				stm32->ic_fw_ver.model_id);
		return false;
	}

	if (fw_ver_bin > fw_ver_ic) {
		input_info(true, &stm32->client->dev, "%s: need to update fw\n", __func__);
		return true;
	} else if (fw_ver_bin == fw_ver_ic && stm32->crc_of_bin != stm32->crc_of_ic) {
		input_err(true, &stm32->client->dev,
				"%s: CRC mismatch! need to update fw\n", __func__);
		return true;
	} else {
		input_info(true, &stm32->client->dev, "%s: skip fw update\n", __func__);
		return false;
	}
}

static int stm32_set_mode(struct stm32_dev *stm32, int mode)
{
	int ret;
	u8 buff;
	u8 cmd;

	if (mode == MODE_APP)
		cmd = STM32_CMD_ABORT;
	else if (mode == MODE_DFU)
		cmd = STM32_CMD_ENTER_DFU_MODE;
	else
		return -EINVAL;

	ret = stm32_i2c_reg_read(stm32->client, ID_MCU, STM32_CMD_GET_MODE, 1, &buff);
	if (ret < 0) {
		input_err(true, &stm32->client->dev,
				"%s: failed to read mode, %d\n", __func__, ret);
		return ret;
	}

	input_info(true, &stm32->client->dev, "[MODE] %d\n", buff);

	if (buff != mode && buff != MODE_EXCEPTION) {
		stm32->hall_flag = false;
		input_info(true, &stm32->client->dev, "%s: enter %s mode\n",
				__func__, mode == MODE_APP ? "APP" : "DFU");
		ret = stm32_i2c_reg_write(stm32->client, ID_MCU, cmd);
	} else if (buff == MODE_EXCEPTION) {
		stm32->hall_flag = true;
	}

	stm32_delay(200);

	return ret;
}

static void stm32_check_ic_work(struct work_struct *work)
{
	struct stm32_dev *stm32 = container_of((struct delayed_work *)work,
			struct stm32_dev, check_ic_work);
	int ret;

	if (!stm32->check_ic_flag) {
		if (stm32->hall_closed) {
			input_info(true, &stm32->client->dev,
					"%s: cover closed or fold backward\n", __func__);
			return;
		}

		ret = stm32_read_version(stm32);
		if (ret < 0) {
			input_err(true, &stm32->client->dev,
				"%s: failed to read version, %d\n", __func__, ret);
			return;
		}
	}

	ret = stm32_set_mode(stm32, MODE_APP);
	if (ret < 0) {
		input_err(true, &stm32->client->dev,
				"%s: failed to set APP mode\n", __func__);
		return;
	}

	if (!stm32->hall_flag) {
		ret = stm32_read_crc(stm32);
		if (ret < 0) {
			input_err(true, &stm32->client->dev,
					"%s: failed to get CRC\n", __func__);
			return;
		}

		ret = stm32_read_tc_version(stm32);
		if (ret < 0) {
			input_err(true, &stm32->client->dev,
					"%s: failed to read TC version\n", __func__);
			return;
		}

		ret = stm32_read_tc_crc(stm32);
		if (ret < 0) {
			input_err(true, &stm32->client->dev,
					"%s: failed to read TC CRC\n", __func__);
			return;
		}

		ret = stm32_read_tc_resolution(stm32);
		if (ret < 0) {
			input_err(true, &stm32->client->dev,
					"%s: failed to read TC resolution\n", __func__);
			return;
		}
	}

	stm32->check_ic_flag = false;
}

static int stm32_dev_kpc_enable(struct stm32_dev *stm32)
{
	stm32_enable_irq(stm32, INT_ENABLE);
	return 0;
}

static void stm32_send_conn_noti(struct stm32_dev *stm32)
{
	char str_conn[16] = { 0 };
	char *event[2] = { str_conn, NULL };

	
	if (stm32->connect_state)
		pogo_notifier_notify(stm32, POGO_NOTIFIER_ID_ATTACHED, 0, 0);
	else
		pogo_notifier_notify(stm32, POGO_NOTIFIER_ID_DETACHED, 0, 0);
	snprintf(str_conn, 16, "CONNECT=%d", stm32->connect_state);
	kobject_uevent_env(&stm32->sec_pogo->kobj, KOBJ_CHANGE, event);
	input_info(true, &stm32->client->dev, "%s: send uevent : %s\n", __func__, event[0]);
}
static void stm32_dev_int_proc(struct stm32_dev *stm32)
{
	int ret = 0;
	u8 event_data[STM32_MAX_EVENT_SIZE] = { 0 };
	u16 payload_size;
	u8 buff[3] = { 0 };

#ifdef CONFIG_PM_SLEEP
	if (!stm32->resume_done.done) {
		wake_lock_timeout(&stm32->stm_wake_lock, msecs_to_jiffies(3 * MSEC_PER_SEC));
		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(&stm32->resume_done,
				msecs_to_jiffies(5 * MSEC_PER_SEC));
		if (ret == 0) {
			input_err(true, &stm32->client->dev,
					"%s: LPM: pm resume is not handled [timeout]\n",
					__func__);
			return;
		}
	}
#endif
	mutex_lock(&stm32->i2c_lock);

	ret = stm32_i2c_header_write(stm32->client, ID_MCU, 0);
	if (ret < 0)
		goto out_i2c_lock;

	ret = stm32_i2c_read_bulk(stm32->client, buff, 3);
	if (ret < 0)
		goto out_i2c_lock;

	memcpy(&payload_size, buff, 2);
	payload_size -= 3;

	if (!payload_size || payload_size > STM32_MAX_EVENT_SIZE) {
		mutex_unlock(&stm32->i2c_lock);
		ret = stm32_read_version(stm32);
		if (ret < 0)
			input_err(true, &stm32->client->dev,
					"%s: failed to read version\n", __func__);
		stm32->check_ic_flag = true;
		schedule_delayed_work(&stm32->check_ic_work, msecs_to_jiffies(10));
		stm32->check_conn_flag = true;
		stm32_send_conn_noti(stm32);
		input_info(true, &stm32->client->dev,"%s: sendconn\n", __func__);
		return;
	}

	ret = stm32_i2c_read_bulk(stm32->client, event_data, payload_size);
	if (ret < 0)
		goto out_i2c_lock;

	/* burst noise & impulse noise */
	if (event_data[0] == 0x03 && event_data[1] == 0x00 && event_data[2] == 0x05)
		goto out_i2c_lock;

	mutex_unlock(&stm32->i2c_lock);

	if (buff[2] > POGO_NOTIFIER_EVENTID_MCU && buff[2] < POGO_NOTIFIER_EVENTID_MAX)
		pogo_notifier_notify(stm32, buff[2], event_data, payload_size);
	else
		input_err(true, &stm32->client->dev,
				"%s: wrong id %d, size %d\n", __func__, buff[2], payload_size);

	/* release all event if hall ic event occured */
	if (buff[2] == POGO_NOTIFIER_EVENTID_HALL) {
		pogo_notifier_notify(stm32, POGO_NOTIFIER_ID_RESET, 0, 0);
		stm32->hall_closed = (event_data[0] != 2);
		if (stm32->hall_flag && !stm32->hall_closed) {
			input_info(true, &stm32->client->dev,
				"%s: hall_flag %d, hall_closed:%d\n", __func__, stm32->hall_flag, stm32->hall_closed);
			schedule_work(&stm32->check_ic_work.work);
			stm32->hall_flag = false;
		}
	}

	return;

out_i2c_lock:
	mutex_unlock(&stm32->i2c_lock);
}

static irqreturn_t stm32_dev_isr(int irq, void *dev_id)
{
	struct stm32_dev *stm32 = (struct stm32_dev *)dev_id;

	if (gpio_get_value(stm32->dtdata->gpio_int))
		return IRQ_HANDLED;

	mutex_lock(&stm32->dev_lock);
	stm32_dev_int_proc(stm32);
	mutex_unlock(&stm32->dev_lock);

	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
static int stm32_parse_dt(struct device *dev,
		struct stm32_dev *device_data)
{
	struct device_node *np = dev->of_node;
	int ret;

	device_data->dtdata->gpio_int = of_get_named_gpio(np, "stm32,irq_gpio", 0);
	if (!gpio_is_valid(device_data->dtdata->gpio_int)) {
		input_err(true, dev, "unable to get gpio_int\n");
		return -EIO;
	}

	device_data->dtdata->gpio_sda = of_get_named_gpio(np, "stm32,sda_gpio", 0);
	if (!gpio_is_valid(device_data->dtdata->gpio_sda)) {
		input_err(true, dev, "unable to get gpio_sda\n");
		return -EIO;
	}

	device_data->dtdata->gpio_scl = of_get_named_gpio(np, "stm32,scl_gpio", 0);
	if (!gpio_is_valid(device_data->dtdata->gpio_scl)) {
		input_err(true, dev, "unable to get gpio_scl\n");
		return -EIO;
	}

	device_data->dtdata->gpio_conn = of_get_named_gpio(np, "stm32,irq_conn", 0);
	if (!gpio_is_valid(device_data->dtdata->gpio_conn)) {
		input_err(true, dev, "unable to get irq_conn\n");
		return -EIO;
	}

	device_data->dtdata->mcu_swclk = of_get_named_gpio(np, "stm32,mcu_swclk", 0);
	if (!gpio_is_valid(device_data->dtdata->mcu_swclk)) {
		input_err(true, dev, "unable to get mcu_swclk\n");
		return -EIO;
	}

	
	device_data->dtdata->mcu_nrst = of_get_named_gpio(np, "stm32,mcu_nrst", 0);
	if (!gpio_is_valid(device_data->dtdata->mcu_nrst)) {
		input_err(true, dev, "unable to get mcu_nrst\n");
		return -EIO;
	}

	if (of_property_read_u32(np, "stm32,irq_type", &device_data->dtdata->irq_type)) {
		input_err(true, dev, "%s: Failed to get irq_type property\n", __func__);
		device_data->dtdata->irq_type = IRQF_TRIGGER_LOW | IRQF_ONESHOT;
	} else {
		input_info(true, dev, "%s: irq_type property:%X, %d\n", __func__,
				device_data->dtdata->irq_type, device_data->dtdata->irq_type);
	}

	if (of_property_read_u32(np, "stm32,irq_conn_type", &device_data->dtdata->irq_conn_type)) {
		input_err(true, dev, "%s: Failed to get irq_conn_type property\n", __func__);
		device_data->dtdata->irq_conn_type = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_ONESHOT;
	} else {
		input_info(true, dev, "%s: irq_conn_type property:%X, %d\n", __func__,
				device_data->dtdata->irq_conn_type, device_data->dtdata->irq_conn_type);
	}

	if (of_property_read_bool(np, "stm32_vddo-supply")) {
		device_data->dtdata->vdd_vreg = devm_regulator_get(&device_data->client->dev,
									"stm32_vddo");
		if (IS_ERR(device_data->dtdata->vdd_vreg)) {
			ret = PTR_ERR(device_data->dtdata->vdd_vreg);
			input_err(true, &device_data->client->dev,
					"%s: could not get vddo, rc = %d\n",
					__func__, ret);
			device_data->dtdata->vdd_vreg = NULL;
		}
	} else {
		input_err(true, &device_data->client->dev, "%s: not use regulator\n", __func__);
	}

	ret = of_property_read_string(np, "stm32,model_name", &device_data->dtdata->model_name);
	if (ret) {
		input_err(true, dev, "unable to get model_name\n");
		return ret;
	}

	ret = of_property_read_string(np, "stm32,fw_name", &device_data->dtdata->mcu_fw_name);
	if (ret) {
		input_err(true, dev, "unable to get model_name\n");
		return ret;
	}

	if (of_property_read_u32(np, "stm32,i2c-burstmax", &device_data->dtdata->i2c_burstmax)) {
		input_dbg(false, dev, "%s: Failed to get i2c_burstmax property\n", __func__);
		device_data->dtdata->i2c_burstmax = STM32_MAX_EVENT_SIZE;
	}

	input_info(true, dev, "%s: scl: %d, sda: %d, int:%d, conn: %d, mcu_swclk: %d, mcu_nrst: %d model_name:%s mcu_fw_name:%s i2c-burstmax:%d\n",
			__func__, device_data->dtdata->gpio_scl, device_data->dtdata->gpio_sda,
			device_data->dtdata->gpio_int, device_data->dtdata->gpio_conn, 
			device_data->dtdata->mcu_nrst, device_data->dtdata->mcu_swclk,
			device_data->dtdata->model_name, device_data->dtdata->mcu_fw_name,
			device_data->dtdata->i2c_burstmax);
	return 0;
}
#else
static int stm32_parse_dt(struct device *dev,
		struct stm32_dev *device_data)
{
	return -ENODEV;
}
#endif

static int stm32_keyboard_start(struct stm32_dev *data)
{
	int ret = 0;

	if (!data)
		return -ENODEV;

	ret = stm32_dev_regulator(data, 1);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: regulator on error\n", __func__);
		goto out;
	}

	stm32_delay(50);
	ret = stm32_dev_kpc_enable(data);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: enable error\n", __func__);
		goto out;
	}

	input_info(true, &data->client->dev, "%s done\n", __func__);
	return 0;
out:
	input_err(true, &data->client->dev, "%s: failed. int:%d, sda:%d, scl:%d\n", __func__,
			gpio_get_value(data->dtdata->gpio_int),
			gpio_get_value(data->dtdata->gpio_sda),
			gpio_get_value(data->dtdata->gpio_scl));

	stm32_enable_irq(data, INT_DISABLE_NOSYNC);

	if (stm32_dev_regulator(data, 0) < 0)
		input_err(true, &data->client->dev, "%s: regulator off error\n", __func__);

	return ret;
}

static int stm32_keyboard_stop(struct stm32_dev *data)
{
	int ret = 0;

	if (!data)
		return -ENODEV;

	stm32_enable_irq(data, INT_DISABLE_NOSYNC);

	ret = stm32_dev_regulator(data, 0);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: regulator off error\n", __func__);
		goto out;
	}

out:
	input_info(true, &data->client->dev, "%s: %s\n", __func__, ret < 0 ? "failed" : "done");
	return ret;
}

static void stm32_keyboard_connect(struct stm32_dev *data)
{
	mutex_lock(&data->dev_lock);

	input_info(true, &data->client->dev, "%s: %d\n", __func__, data->connect_state);
	data->hall_closed = false;

#ifndef CONFIG_SEC_FACTORY
	if (!data->resume_done.done)
		wake_lock_timeout(&data->stm_wake_lock, msecs_to_jiffies(3 * MSEC_PER_SEC));
#endif
	cancel_delayed_work(&data->check_ic_work);

	if (data->connect_state) {
		stm32_keyboard_start(data);
		schedule_delayed_work(&data->check_init_work, 500);
	} else {
		stm32_keyboard_stop(data);
		data->check_conn_flag = false;
	}

	pogo_set_conn_state(data->connect_state);

	if (!data->connect_state)
		cancel_delayed_work(&data->print_info_work);
	else
		schedule_delayed_work(&data->print_info_work, STM32_PRINT_INFO_DELAY);

	mutex_unlock(&data->dev_lock);

}

static void stm32_check_conn_work(struct work_struct *work)
{
	struct stm32_dev *data = container_of(work, struct stm32_dev,
			check_conn_work.work);
	int current_conn_state;

	mutex_lock(&data->conn_lock);
	current_conn_state = gpio_get_value(data->dtdata->gpio_conn);
	input_info(true, &data->client->dev,"%s: con:%d, current:%d\n", __func__,
					data->connect_state, current_conn_state);
	if (data->connect_state) {
		if (!current_conn_state) {
			data->connect_state = current_conn_state;
			stm32_keyboard_connect(data);
			stm32_send_conn_noti(data);
		} else {
			stm32_dev_regulator(data, 1);
		}
	} else {
		 if (current_conn_state) {
			data->connect_state = current_conn_state;
			stm32_keyboard_connect(data);
		}
	}
	mutex_unlock(&data->conn_lock);
}

static void stm32_check_init_work(struct work_struct *work)
{
	struct stm32_dev *data = container_of((struct delayed_work *)work,
			struct stm32_dev, check_init_work);

	if (!data->check_conn_flag) {
		input_info(true, &data->client->dev, "%s: start\n", __func__);
		data->connect_state = 0;
		stm32_keyboard_connect(data);
	}
		
}

static irqreturn_t stm32_conn_isr(int irq, void *dev_id)
{
	struct stm32_dev *data = (struct stm32_dev *)dev_id;
#ifdef CONFIG_PM_SLEEP
	int ret = 0;

	wake_lock_timeout(&data->stm_wake_lock, msecs_to_jiffies(3 * MSEC_PER_SEC));
	if (!data->resume_done.done) {
		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(&data->resume_done,
				msecs_to_jiffies(5 * MSEC_PER_SEC));
		if (ret == 0) {
			input_err(true, &data->client->dev,
					"%s: LPM: pm resume is not handled [timeout]\n",
					__func__);
			return IRQ_HANDLED;
		}
	}
#endif
	cancel_delayed_work_sync(&data->check_conn_work);
	input_info(true, &data->client->dev,"%s\n", __func__);
	if (!gpio_get_value(data->dtdata->gpio_conn)) {
		stm32_dev_regulator(data, 0);
		schedule_delayed_work(&data->check_conn_work, msecs_to_jiffies(250));
	} else {
		schedule_work(&data->check_conn_work.work);
	}

	return IRQ_HANDLED;
}

static ssize_t keyboard_connected_store(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	struct stm32_dev *data = dev_get_drvdata(dev);
	int onoff, err;

	if (strlen(buf) > 2) {
		input_err(true, &data->client->dev, "%s: cmd lenth is over %d\n",
				__func__, (int)strlen(buf));
		return -EINVAL;
	}

	err = kstrtoint(buf, 10, &onoff);
	if (err)
		return err;

	if (data->connect_state == !!onoff)
		return size;

	data->connect_state = !!onoff;
	input_info(true, &data->client->dev, "%s: current %d \n",
			__func__, data->connect_state);
	stm32_keyboard_connect(data);
	
	return size;
}

static ssize_t keyboard_connected_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *data = dev_get_drvdata(dev);

	input_info(true, dev, "%s: %d\n", __func__, data->connect_state);

	return snprintf(buf, 5, "%d\n", data->connect_state);
}

static ssize_t hw_reset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *data = dev_get_drvdata(dev);

	input_info(true, dev, "%s \n", __func__);
	gpio_direction_output(data->dtdata->mcu_nrst, 0);
	stm32_delay(3);
	gpio_direction_output(data->dtdata->mcu_nrst, 1);

	return snprintf(buf, 5, "%d\n", data->dtdata->mcu_nrst);
}

static ssize_t pogo_get_fw_ver_ic(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	char buff[256] = { 0 };

	if (stm32->hall_closed || !stm32->connect_state)
		return snprintf(buf, 3, "NG");

	snprintf(buff, sizeof(buff), "%s_v%d.%d.%d.%d",
			stm32->dtdata->model_name,
			stm32->ic_fw_ver.fw_major_ver,
			stm32->ic_fw_ver.fw_minor_ver,
			stm32->ic_fw_ver.model_id,
			stm32->ic_fw_ver.hw_rev);

	input_info(true, &stm32->client->dev, "%s: %s\n", __func__, buff);
	return snprintf(buf, sizeof(buff), "%s", buff);
}

static ssize_t pogo_get_fw_ver_bin(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	char buff[256] = { 0 };

	if (stm32_load_fw_from_fota(stm32, ID_MCU) < 0) {
		input_err(true, &stm32->client->dev, "%s: failed to read bin data\n", __func__);
		return snprintf(buf, sizeof(buff), "NG");
	}

	snprintf(buff, sizeof(buff), "%s_v%d.%d.%d.%d",
			stm32->dtdata->model_name,
			stm32->fw_header->boot_app_ver.fw_major_ver,
			stm32->fw_header->boot_app_ver.fw_minor_ver,
			stm32->fw_header->boot_app_ver.model_id,
			stm32->fw_header->boot_app_ver.hw_rev);

	input_info(true, &stm32->client->dev, "%s: %s\n", __func__, buff);
	return snprintf(buf, sizeof(buff), "%s", buff);
}

static ssize_t pogo_get_crc(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	char buff[256] = { 0 };

	if (stm32->hall_closed || !stm32->connect_state)
		return snprintf(buf, 3, "NG");

	snprintf(buff, sizeof(buff), "%08X", stm32->crc_of_ic);

	input_info(true, &stm32->client->dev, "%s: %s\n", __func__, buff);
	return snprintf(buf, sizeof(buff), "%s", buff);
}

static ssize_t pogo_check_fw_update(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);

	if (stm32_check_fw_update(stm32))
		return snprintf(buf, 3, "OK");
	else
		return snprintf(buf, 3, "NG");
}

static int stm32_write_fw(struct stm32_dev *stm32, u8 ed_id, u16 page_size)
{
	const struct firmware *fw;
	int page_num, page_index = 0, ret, retry = 3, last_page_size;
	u8 buff[3] = { 0 };

	if (ed_id == ID_MCU) {
		fw = stm32->fw;
	} else if (ed_id == ID_TOUCHPAD) {
		fw = stm32->tc_fw;
	} else {
		input_err(true, &stm32->client->dev, "%s: wrong ed_id %d\n", __func__, ed_id);
		return -EINVAL;
	}

	if (!fw) {
		input_err(true, &stm32->client->dev, "%s: fw is null\n", __func__);
		return -ENOENT;
	}

	page_num = fw->size / page_size;
	last_page_size = fw->size % page_size;

	if (last_page_size == 0) {
		page_num--;
		last_page_size = page_size;
	}

	input_info(true, &stm32->client->dev,
			"%s: fw size: %ld, page size: %d, page num: %d, last page size: %d, packet size: %d\n",
			__func__, fw->size, page_size, page_num, last_page_size, STM32_CFG_PACKET_SIZE);

	while (page_index <= page_num) {
		u8 *pn_data = (u8 *)&fw->data[page_index * page_size];
		u8 send_data[STM32_CFG_PACKET_SIZE + 5] = { 0 };
		u32 page_crc;
		u16 page_index_cmp;
		int delayms = 20;/* test */

		if (ed_id == ID_TOUCHPAD && page_index == page_num) {
			send_data[0] = STM32_CMD_WRITE_LAST_PAGE;
			page_size = last_page_size;
			delayms = 500;
		} else {
			send_data[0] = STM32_CMD_WRITE_PAGE;
		}

		memcpy(&send_data[1], pn_data, page_size);
		page_crc = stm32_crc32(pn_data, page_size);
		memcpy(&send_data[page_size + 1], &page_crc, 4);

		input_dbg(false, &stm32->client->dev, "%s: write page %d, size %d, CRC 0x%08X\n",
				__func__, page_index, page_size, page_crc);

		ret = stm32_i2c_header_write(stm32->client, ed_id, page_size + 5);
		if (ret < 0)
			return ret;

		ret = stm32_i2c_write_burst(stm32->client, send_data, page_size + 5);
		if (ret < 0)
			return ret;

		stm32_delay(delayms);

		/* read page index */
		ret = stm32_i2c_read_bulk(stm32->client, buff, 3);
		if (ret < 0)
			return ret;
		ret = stm32_i2c_read_bulk(stm32->client, buff, 2);
		if (ret < 0)
			return ret;
		memcpy(&page_index_cmp, buff, 2);
		page_index++;

		if (page_index == page_index_cmp) {
			retry = 3;
		} else {
			page_index--;
			input_err(true, &stm32->client->dev,
					"%s: page %d(%d) is not programmed, retry %d\n",
					__func__, page_index, page_index_cmp, 3 - retry);
			retry--;
		}

		if (retry < 0) {
			input_err(true, &stm32->client->dev, "%s: failed\n", __func__);
			return -EIO;
		}
	}

	input_info(true, &stm32->client->dev, "%s: done\n", __func__);
	return 0;
}

static int stm32_fw_update(struct stm32_dev *stm32)
{
	int ret;
	u8 buff[4] = { 0 };
	u16 page_size;
	struct stm32_fw_version target_fw;
	u32 target_crc;

	if (!stm32->fw)
		return -ENOENT;

	if (strncmp(STM32_MAGIC_WORD, stm32->fw_header->magic_word, 7) != 0) {
		input_info(true, &stm32->client->dev, "%s: firmware file is wrong : %s\n",
				__func__, stm32->fw_header->magic_word);
		return -ENOENT;
	}

	ret = stm32_set_mode(stm32, MODE_DFU);
	if (ret < 0) {
		input_err(true, &stm32->client->dev,
				"%s: failed to set DFU mode\n", __func__);
		return ret;
	}

	stm32_enable_irq(stm32, INT_DISABLE_SYNC);

	stm32_delay(1000);

	ret = stm32_read_version(stm32);
	if (ret < 0)
		goto out;

	ret = stm32_i2c_reg_read(stm32->client, ID_MCU, STM32_CMD_GET_PROTOCOL_VERSION, 2, buff);
	if (ret < 0)
		goto out;
	input_info(true, &stm32->client->dev,
			"%s: protocol ver: 0x%02X%02X\n", __func__, buff[1], buff[0]);

	ret = stm32_i2c_reg_read(stm32->client, ID_MCU, STM32_CMD_GET_PAGE_SIZE, 2, buff);
	if (ret < 0)
		goto out;

	memcpy(&page_size, &buff[0], 2);

	/* disable write protection of target bank & erase the target bank */
	ret = stm32_i2c_reg_read(stm32->client, ID_MCU, STM32_CMD_START_FW_UPGRADE, 1, buff);
	if (ret < 0 || buff[0] == 0xFF)
		goto out;
	input_info(true, &stm32->client->dev, "%s: bank %02X is erased\n", __func__, buff[0]);

	mutex_lock(&stm32->i2c_lock);
	ret = stm32_write_fw(stm32, ID_MCU, page_size);
	mutex_unlock(&stm32->i2c_lock);
	if (ret < 0)
		goto out;

	/* check fw version & crc of target bank */
	ret = stm32_i2c_reg_read(stm32->client, ID_MCU, STM32_CMD_GET_TARGET_FW_VERSION, 4, buff);
	if (ret < 0)
		goto out;

	target_fw.hw_rev = buff[0];
	target_fw.model_id = buff[1];
	target_fw.fw_minor_ver = buff[2];
	target_fw.fw_major_ver = buff[3];

	input_info(true, &stm32->client->dev, "%s: [TARGET] version:%d.%d, model_id:%d\n",
			__func__, target_fw.fw_major_ver,
			target_fw.fw_minor_ver, target_fw.model_id);

	if (target_fw.fw_major_ver != stm32->fw_header->boot_app_ver.fw_major_ver ||
		target_fw.fw_minor_ver != stm32->fw_header->boot_app_ver.fw_minor_ver) {
		input_err(true, &stm32->client->dev, "%s: version mismatch!\n", __func__);
		goto out;
	}

	ret = stm32_i2c_reg_read(stm32->client, ID_MCU, STM32_CMD_GET_TARGET_FW_CRC32, 4, buff);
	if (ret < 0)
		goto out;

	target_crc = buff[3] << 24 | buff[2] << 16 | buff[1] << 8 | buff[0];

	input_info(true, &stm32->client->dev, "%s: [TARGET] CRC:0x%08X\n", __func__, target_crc);
	if (target_crc != stm32->crc_of_bin) {
		input_err(true, &stm32->client->dev, "%s: CRC mismatch!\n", __func__);
		goto out;
	}

	/* leave the DFU mode with switching to target bank */
	ret = stm32_i2c_reg_write(stm32->client, ID_MCU, STM32_CMD_GO);
	if (ret < 0)
		goto out;

	input_info(true, &stm32->client->dev,
			"%s: [APP MODE] Start with target bank\n", __func__);

	if (stm32->connect_state)
		stm32_enable_irq(stm32, INT_ENABLE);
	return 0;

out:
	stm32_delay(1000);

	/* leave the DFU mode without switching to target bank */
	ret = stm32_i2c_reg_write(stm32->client, ID_MCU, STM32_CMD_ABORT);
	if (ret < 0)
		return ret;

	input_info(true, &stm32->client->dev,
			"%s: [APP MODE] Start with boot bank\n", __func__);

	if (stm32->connect_state)
		stm32_enable_irq(stm32, INT_ENABLE);

	return -EIO;
}

static ssize_t pogo_debug_level_store(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	int ret, level;

	ret = kstrtoint(buf, 10, &level);
	if (ret)
		return ret;

	stm32->debug_level = level;

	input_info(true, &stm32->client->dev, "%s: %d\n", __func__, stm32->debug_level);
	return size;
}

static ssize_t pogo_debug_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);

	input_info(true, &stm32->client->dev, "%s: %d\n", __func__, stm32->debug_level);

	return snprintf(buf, 5, "%d\n", stm32->debug_level);
}

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
static ssize_t pogo_i2c_write(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	int ret, reg;

	ret = kstrtoint(buf, 0, &reg);
	if (ret)
		return ret;

	if (reg > 0xFF)
		return -EIO;

	ret = stm32_i2c_reg_write(stm32->client, ID_MCU, reg);

	input_info(true, &stm32->client->dev, "%s: write 0x%02X, ret: %d\n", __func__, reg, ret);
	return size;
}

static ssize_t pogo_i2c_read(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	int ret, reg;
	u8 buff[256] = { 0 };

	ret = kstrtoint(buf, 0, &reg);
	if (ret)
		return ret;

	if (reg > 0xFF)
		return -EIO;

	ret = stm32_i2c_reg_read(stm32->client, ID_MCU, reg, 255, buff);

	input_info(true, &stm32->client->dev, "%s: read 0x%02X, ret: %d\n", __func__, reg, ret);
	return size;
}
#endif

static ssize_t pogo_get_tc_fw_ver_bin(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	char buff[256] = { 0 };

	if (stm32_load_fw_from_fota(stm32, ID_TOUCHPAD) < 0) {
		input_err(true, &stm32->client->dev, "%s: failed to read bin data\n", __func__);
		return snprintf(buf, sizeof(buff), "NG");
	}

	snprintf(buff, sizeof(buff), "%s-TC_v%02X%02X",
			stm32->dtdata->model_name,
			stm32->tc_fw_ver_of_bin.major_ver & 0xFF,
			stm32->tc_fw_ver_of_bin.minor_ver & 0xFF);

	input_info(true, &stm32->client->dev, "%s: %s\n", __func__, buff);
	return snprintf(buf, sizeof(buff), "%s", buff);
}

static ssize_t pogo_get_tc_fw_ver_ic(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	char buff[256] = { 0 };

	mutex_lock(&stm32->dev_lock);

	if (stm32->hall_closed || !stm32->connect_state)
		goto NG;

	if (stm32->tc_crc == STM32_TC_CRC_OK)
		snprintf(buff, sizeof(buff), "%s-TC_v%02X%02X",
				stm32->dtdata->model_name,
				stm32->tc_fw_ver_of_ic.major_ver & 0xFF,
				stm32->tc_fw_ver_of_ic.minor_ver & 0xFF);
	else
		snprintf(buff, sizeof(buff), "%s-TC_v0000",
				stm32->dtdata->model_name);

	input_info(true, &stm32->client->dev, "%s: %s\n", __func__, buff);
	mutex_unlock(&stm32->dev_lock);

	return snprintf(buf, sizeof(buff), "%s", buff);

NG:
	snprintf(buff, sizeof(buff), "NG");

	input_info(true, &stm32->client->dev, "%s: %s\n", __func__, buff);
	mutex_unlock(&stm32->dev_lock);

	return snprintf(buf, sizeof(buff), "%s", buff);
}

static ssize_t pogo_get_tc_crc(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	char buff[256] = { 0 };

	if (stm32->hall_closed || !stm32->connect_state)
		return snprintf(buf, 3, "NG");

	snprintf(buff, sizeof(buff), "%04X", stm32->tc_crc);

	input_info(true, &stm32->client->dev, "%s: %s\n", __func__, buff);
	return snprintf(buf, sizeof(buff), "%s", buff);
}

static int stm32_tc_fw_update(struct stm32_dev *stm32)
{
	int ret;
	u8 buff[3] = { 0 };
	u16 page_size;

	ret = stm32_set_mode(stm32, MODE_DFU);
	if (ret < 0) {
		input_err(true, &stm32->client->dev,
				"%s: failed to set DFU mode\n", __func__);
		return ret;
	}

	stm32_enable_irq(stm32, INT_DISABLE_SYNC);

	stm32_delay(1000);

	ret = stm32_read_version(stm32);
	if (ret < 0)
		goto out;

	ret = stm32_read_tc_version(stm32);
	if (ret < 0)
		goto out;

	ret = stm32_read_tc_crc(stm32);
	if (ret < 0)
		goto out;

	ret = stm32_i2c_reg_read(stm32->client, ID_TOUCHPAD, STM32_CMD_GET_PAGE_SIZE, 2, buff);
	if (ret < 0)
		goto out;

	memcpy(&page_size, &buff[0], 2);

	ret = stm32_i2c_reg_write(stm32->client, ID_TOUCHPAD, STM32_CMD_START_FW_UPGRADE);
	if (ret < 0)
		goto out;

	stm32_delay(800);

	ret = stm32_i2c_read_bulk(stm32->client, buff, 3);
	if (ret < 0)
		goto out;

	ret = stm32_i2c_read_bulk(stm32->client, buff, 1);
	if (ret < 0 || buff[0] != TC_FW_RES_OK) {
		ret = -EIO;
		input_err(true, &stm32->client->dev,
				"%s: tc fw update cmd is failed, 0x%02X\n", __func__, buff[0]);
		goto out;
	}

	input_info(true, &stm32->client->dev, "%s: start to update\n", __func__);

	mutex_lock(&stm32->i2c_lock);
	ret = stm32_write_fw(stm32, ID_TOUCHPAD, page_size);
	mutex_unlock(&stm32->i2c_lock);
	if (ret < 0)
		goto out;

	/* check fw version & crc of touch controller */
	ret = stm32_read_tc_version(stm32);
	if (ret < 0)
		goto out;

	ret = stm32_read_tc_crc(stm32);
	if (ret < 0)
		goto out;

	if (stm32->tc_crc != STM32_TC_CRC_OK) {
		ret = -EIO;
		input_err(true, &stm32->client->dev, "%s: CRC mismatch!\n", __func__);
		goto out;
	}

out:
	if (stm32_i2c_reg_write(stm32->client, ID_MCU, STM32_CMD_ABORT) < 0)
		return -EIO;

	input_info(true, &stm32->client->dev,
			"%s: [APP MODE] Start with boot bank\n", __func__);

	if (stm32->connect_state)
		stm32_enable_irq(stm32, INT_ENABLE);

	return ret;
}

static ssize_t pogo_fw_update(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	int ret, param;

	if (stm32->hall_closed || !stm32->connect_state)
		return -ENODEV;

	ret = kstrtoint(buf, 10, &param);
	if (ret)
		return ret;

	input_info(true, &stm32->client->dev, "%s \n", __func__);

	switch (param) {
	case FW_UPDATE_MCU:
		ret = stm32_load_fw_from_fota(stm32, ID_MCU);
		if (ret < 0)
			return ret;

		ret = stm32_fw_update(stm32);
		if (ret < 0)
			return ret;
		else
			return size;
	case FW_UPDATE_TC:
		ret = stm32_load_fw_from_fota(stm32, ID_TOUCHPAD);
		if (ret < 0)
			return ret;
	
		ret = stm32_tc_fw_update(stm32);
		if (ret < 0)
			return ret;
		else
			return size;
	default:
		return -EINVAL;
	}
}

static ssize_t stm32_dev_fw_update(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	int ret;
	
	ret = stm32_dev_firmware_update_menu(stm32, 1);

	if (!ret)
		return snprintf(buf, 3, "OK");
	else
		return snprintf(buf, 3, "NG");
}

static ssize_t get_mcu_fw_ver(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stm32_dev *stm32 = dev_get_drvdata(dev);
	int ret = 0;

	if (stm32->connect_state)
		return snprintf(buf, 3, "NG");

	ret = stm32_dev_get_ic_ver(stm32);
	if (ret < 0)
		return snprintf(buf, 3, "NG");

	if (stm32->mdata.phone_ver[3] == stm32->mdata.ic_ver[3])
		return snprintf(buf, 3, "%2x", stm32->mdata.ic_ver[3]);
	else
		return snprintf(buf, 3, "NG");
}

static DEVICE_ATTR(keyboard_connected, 0644, keyboard_connected_show, keyboard_connected_store);
static DEVICE_ATTR(hw_reset, 0444, hw_reset_show, NULL);
static DEVICE_ATTR(get_fw_ver_bin, 0444, pogo_get_fw_ver_bin, NULL);
static DEVICE_ATTR(get_fw_ver_ic, 0444, pogo_get_fw_ver_ic, NULL);
static DEVICE_ATTR(get_crc, 0444, pogo_get_crc, NULL);
static DEVICE_ATTR(check_fw_update, 0444, pogo_check_fw_update, NULL);
static DEVICE_ATTR(fw_update, 0220, NULL, pogo_fw_update);
static DEVICE_ATTR(fw_mcu_update, 0444, stm32_dev_fw_update, NULL);
static DEVICE_ATTR(debug, 0644, pogo_debug_level_show, pogo_debug_level_store);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
static DEVICE_ATTR(write_cmd, 0200, NULL, pogo_i2c_write);
static DEVICE_ATTR(read_cmd, 0200, NULL, pogo_i2c_read);
#endif
static DEVICE_ATTR(get_tc_fw_ver_bin, 0444, pogo_get_tc_fw_ver_bin, NULL);
static DEVICE_ATTR(get_tc_fw_ver_ic, 0444, pogo_get_tc_fw_ver_ic, NULL);
static DEVICE_ATTR(get_tc_crc, 0444, pogo_get_tc_crc, NULL);
static DEVICE_ATTR(get_mcu_fw_ver, 0444, get_mcu_fw_ver, NULL);


static struct attribute *key_attributes[] = {
	&dev_attr_keyboard_connected.attr,
	&dev_attr_hw_reset.attr,
	&dev_attr_get_fw_ver_bin.attr,
	&dev_attr_get_fw_ver_ic.attr,
	&dev_attr_get_crc.attr,
	&dev_attr_check_fw_update.attr,
	&dev_attr_fw_update.attr,
	&dev_attr_fw_mcu_update.attr,
	&dev_attr_debug.attr,
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	&dev_attr_write_cmd.attr,
	&dev_attr_read_cmd.attr,
#endif
	&dev_attr_get_tc_fw_ver_bin.attr,
	&dev_attr_get_tc_fw_ver_ic.attr,
	&dev_attr_get_tc_crc.attr,
	&dev_attr_get_mcu_fw_ver.attr,
	NULL,
};

static struct attribute_group key_attr_group = {
	.attrs = key_attributes,
};

static int stm32_dev_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct stm32_dev *device_data;
	int ret = 0;
	u16 boot_addr = 0x51;

	input_info(true, &client->dev, "%s++\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev,
				"i2c_check_functionality fail\n");
		return -EIO;
	}

	device_data = kzalloc(sizeof(struct stm32_dev), GFP_KERNEL);
	if (!device_data) {
		input_err(true, &client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	device_data->client_boot = i2c_new_dummy(client->adapter, boot_addr);
	if (!device_data->client_boot) {
		input_err(true, &client->dev,
				"Failed to register sub client[0x%x]\n", boot_addr);
		kfree(device_data);
		return -ENOMEM;
	}

	mutex_init(&device_data->dev_lock);
	mutex_init(&device_data->irq_lock);
	mutex_init(&device_data->i2c_lock);
	mutex_init(&device_data->conn_lock);
	wake_lock_init(&device_data->stm_wake_lock, WAKE_LOCK_SUSPEND, "stm_key wake lock");
#ifdef CONFIG_PM_SLEEP
	init_completion(&device_data->resume_done);
	complete_all(&device_data->resume_done);
#endif
	init_completion(&device_data->i2c_done);
	complete_all(&device_data->i2c_done);
	device_data->client = client;

	if (client->dev.of_node) {
		device_data->dtdata = devm_kzalloc(&client->dev,
				sizeof(struct stm32_devicetree_data), GFP_KERNEL);
		if (!device_data->dtdata) {
			input_err(true, &client->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_config;
		}
		ret = stm32_parse_dt(&client->dev, device_data);
		if (ret) {
			input_err(true, &client->dev, "Failed to use device tree\n");
			ret = -EIO;
			goto err_config;
		}

	} else {
		input_err(true, &client->dev, "No use device tree\n");
		device_data->dtdata = client->dev.platform_data;
		if (!device_data->dtdata) {
			input_err(true, &client->dev, "Failed to get platform data\n");
			ret = -ENOENT;
			goto err_config;
		}
	}

	/* Get pinctrl if target uses pinctrl */
	device_data->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(device_data->pinctrl)) {
		if (PTR_ERR(device_data->pinctrl) == -EPROBE_DEFER) {
			ret = PTR_ERR(device_data->pinctrl);
			goto err_config;
		}

		input_err(true, &client->dev, "%s: Target does not use pinctrl\n", __func__);
		device_data->pinctrl = NULL;
	}

	device_data->fw_header = kzalloc(sizeof(struct stm32_fw_header), GFP_KERNEL);
	if (!device_data->fw_header) {
		input_err(true, &client->dev, "fw header kzalloc memory error\n");
		ret = -ENOMEM;
		goto err_alloc_fw_header;
	}

	device_data->fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);
	if (!device_data->fw) {
		ret = -ENOMEM;
		goto err_alloc_fw;
	}

	device_data->fw->data = kzalloc(STM32_FW_SIZE, GFP_KERNEL);
	if (!device_data->fw->data) {
		ret = -ENOMEM;
		goto err_alloc_fw_data;
	}

	device_data->tc_fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);
	if (!device_data->tc_fw) {
		ret = -ENOMEM;
		goto err_alloc_tc_fw;
	}

	device_data->tc_fw->data = kzalloc(STM32_FW_SIZE, GFP_KERNEL);
	if (!device_data->tc_fw->data) {
		ret = -ENOMEM;
		goto err_alloc_tc_fw_data;
	}

	i2c_set_clientdata(client, device_data);
	i2c_set_clientdata(device_data->client_boot, device_data);
	g_tc_resolution.x = 0;
	g_tc_resolution.y = 0;
	device_data->check_ic_flag = false;

	device_data->dev_irq = gpio_to_irq(device_data->dtdata->gpio_int);
	input_info(true, &client->dev,
			"%s INT mode (%d)\n", __func__, device_data->dev_irq);

	device_data->conn_irq = gpio_to_irq(device_data->dtdata->gpio_conn);
	input_info(true, &client->dev,
			"%s CONN INT PIN (%d)\n", __func__, device_data->conn_irq);

	ret = stm32_dev_firmware_update_menu(device_data, 0);
	if (ret < 0)
		input_err(true, &client->dev, "Failed to update stm32_mcu_dev firmware %d\n", ret);

	ret = request_threaded_irq(device_data->dev_irq, NULL, stm32_dev_isr,
					device_data->dtdata->irq_type,
					STM32_DRV_NAME, device_data);
	if (ret < 0) {
		input_err(true, &client->dev, "stm32_dev_irq request error %d\n", ret);
		goto err_firmware_update;
	}

	ret = request_threaded_irq(device_data->conn_irq, NULL, stm32_conn_isr,
					device_data->dtdata->irq_conn_type,
					STM32_DRV_CONN_NAME, device_data);
	if (ret < 0) {
		input_err(true, &client->dev, "stm32_dev_conn_isr request error %d\n", ret);
		goto interrupt_err;
	}

	enable_irq_wake(device_data->conn_irq);
	stm32_enable_irq(device_data, INT_DISABLE_NOSYNC);

	device_data->sec_pogo = sec_device_create(device_data, "sec_keypad");

	if (IS_ERR(device_data->sec_pogo)) {
		input_err(true, &client->dev, "Failed to create sec_keypad device\n");
		ret = PTR_ERR(device_data->sec_pogo);
		goto err_create_device;
	}

	ret = sysfs_create_group(&device_data->sec_pogo->kobj, &key_attr_group);
	if (ret) {
		input_err(true, &client->dev, "Failed to create sysfs: %d\n", ret);
		goto err_create_group;
	}

	device_init_wakeup(&client->dev, 1);

	BLOCKING_INIT_NOTIFIER_HEAD(&pogo_notifier.pogo_notifier_call_chain);

	INIT_DELAYED_WORK(&device_data->check_ic_work, stm32_check_ic_work);
	INIT_DELAYED_WORK(&device_data->print_info_work, stm32_print_info_work);
	INIT_DELAYED_WORK(&device_data->check_conn_work, stm32_check_conn_work);
	INIT_DELAYED_WORK(&device_data->check_init_work, stm32_check_init_work);

	device_data->connect_state = gpio_get_value(device_data->dtdata->gpio_conn);
	if (device_data->connect_state)
		device_data->check_ic_flag = true;
	else
		device_data->check_ic_flag = false;
	stm32_keyboard_connect(device_data);

	input_info(true, &client->dev, "%s done\n", __func__);

	return ret;

	//sysfs_remove_group(&device_data->sec_pogo->kobj, &key_attr_group);
err_create_group:
	sec_device_destroy(15);
err_create_device:
interrupt_err:
err_firmware_update:
	i2c_unregister_device(client);
	i2c_unregister_device(device_data->client_boot);
	kfree(device_data->tc_fw->data);
err_alloc_tc_fw_data:
	kfree(device_data->tc_fw);
err_alloc_tc_fw:
	kfree(device_data->fw->data);
err_alloc_fw_data:
	kfree(device_data->fw);
err_alloc_fw:
	kfree(device_data->fw_header);
err_alloc_fw_header:
err_config:
	mutex_destroy(&device_data->dev_lock);
	mutex_destroy(&device_data->irq_lock);
	mutex_destroy(&device_data->i2c_lock);
	mutex_destroy(&device_data->conn_lock);

	wake_lock_destroy(&device_data->stm_wake_lock);

	kfree(device_data);

	input_err(true, &client->dev, "Error at %s\n", __func__);

	return ret;
}

static int stm32_dev_remove(struct i2c_client *client)
{
	struct stm32_dev *device_data = i2c_get_clientdata(client);

	input_info(true, &client->dev, "%s\n", __func__);

	cancel_delayed_work_sync(&device_data->print_info_work);
	cancel_delayed_work_sync(&device_data->check_ic_work);
	cancel_delayed_work_sync(&device_data->check_conn_work);
	cancel_delayed_work_sync(&device_data->check_init_work);
	device_init_wakeup(&client->dev, 0);
	wake_lock_destroy(&device_data->stm_wake_lock);

	stm32_dev_regulator(device_data, 0);

	i2c_unregister_device(device_data->client_boot);

	free_irq(device_data->dev_irq, device_data);
	free_irq(device_data->conn_irq, device_data);
	sec_device_destroy(14);

	kfree(device_data->tc_fw->data);
	kfree(device_data->tc_fw);
	kfree(device_data->fw->data);
	kfree(device_data->fw);
	kfree(device_data->fw_header);
	kfree(device_data);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int stm32_dev_suspend(struct device *dev)
{
	struct stm32_dev *device_data = dev_get_drvdata(dev);
	int ret = 0;

	input_dbg(false, &device_data->client->dev, "%s\n", __func__);

	ret = wait_for_completion_interruptible_timeout(&device_data->i2c_done,
				msecs_to_jiffies(3 * MSEC_PER_SEC));
	if (ret <= 0) {
		input_err(true, &device_data->client->dev,
				"%s: i2c is not handled, error %d\n",
				__func__, ret);
	}

	cancel_delayed_work(&device_data->print_info_work);

	reinit_completion(&device_data->resume_done);

	if (device_data->connect_state && device_may_wakeup(dev)) {
		enable_irq_wake(device_data->dev_irq);
		device_data->irq_wake = true;
		input_info(false, &device_data->client->dev,
				"%s enable irq wake\n", __func__);
	}

	return 0;
}

static int stm32_dev_resume(struct device *dev)
{
	struct stm32_dev *device_data = dev_get_drvdata(dev);

	input_dbg(false, &device_data->client->dev, "%s\n", __func__);

	complete_all(&device_data->resume_done);

	if (device_data->irq_wake && device_may_wakeup(dev)) {
		disable_irq_wake(device_data->dev_irq);
		device_data->irq_wake = false;
		input_info(false, &device_data->client->dev,
				"%s disable irq wake\n", __func__);
	}
	if (device_data->connect_state)
		schedule_delayed_work(&device_data->print_info_work, STM32_PRINT_INFO_DELAY);

	return 0;
}

static SIMPLE_DEV_PM_OPS(stm32_dev_pm_ops,
		stm32_dev_suspend, stm32_dev_resume);
#endif

static const struct i2c_device_id stm32_dev_id[] = {
	{ STM32_DRV_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id stm32_match_table[] = {
	{ .compatible = "stm,stm32_pogo",},
	{ },
};
#else
#define stm32_match_table NULL
#endif

static struct i2c_driver stm32_dev_driver = {
	.driver = {
		.name = STM32_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = stm32_match_table,
#ifdef CONFIG_PM_SLEEP
		.pm = &stm32_dev_pm_ops,
#endif
	},
	.probe = stm32_dev_probe,
	.remove = stm32_dev_remove,
	.id_table = stm32_dev_id,
};

static int __init stm32_dev_init(void)
{
	pr_err("%s++\n", __func__);

	return i2c_add_driver(&stm32_dev_driver);
}
static void __exit stm32_dev_exit(void)
{
	i2c_del_driver(&stm32_dev_driver);
}
module_init(stm32_dev_init);
//late_initcall(stm32_dev_init);
module_exit(stm32_dev_exit);

MODULE_DEVICE_TABLE(i2c, stm32_dev_id);

MODULE_DESCRIPTION("STM32 POGO I2C Driver");
MODULE_AUTHOR("Samsung");
MODULE_LICENSE("GPL");
