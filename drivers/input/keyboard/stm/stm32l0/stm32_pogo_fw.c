#include "stm32_pogo_i2c.h"


/* Target specific definitions
 *  1. Startup delay
 *     STM32 target needs at least t-ms delay after reset msecs
 *  2. Target I2C slave dddress
 */
static const u32 sysboot_i2c_startup_delay = 50; /* msec */

/* STM32MCU PID */
static const u16 product_id = 0x460;

/* Flash memory characteristics from target datasheet (msec unit) */
static const u32 flash_prog_time = 37; /* per page or sector */
static const u32 flash_full_erase_time = 40 * 32; /* 2K erase time(40ms) * 32 pages */
static const u32 flash_page_erase_time = 36; /* per page or sector */

/* Flash memory page(or sector) structure */
stm32_page_type stm32_memory_pages[] = {
  {2048, 32},
  {   0,  0}
};

stm32_map_type stm32_memory_map =
{
	0x08000000, /* flash memory starting address */
	0x1FFF0000, /* system memory starting address */
	0x1FFF7800, /* option byte starting address */
	(stm32_page_type *)stm32_memory_pages,
};

#define KEYBOARD_PATH_EXTERNAL_FW		"/sdcard/Firmware/KEYBOARD/keyboard.bin"


static u8 stm32_sysboot_checksum(u8 *src, u32 len);
static int stm32_sysboot_conv_memory_map(struct stm32_dev *data, u32 address, size_t len, stm32_erase_param_type *erase);
static int stm32_sysboot_i2c_erase(struct stm32_dev *data, u32 address, size_t len);
static int stm32_sysboot_i2c_read(struct stm32_dev *data, u32 address, u8 *dst, size_t len);
static int stm32_sysboot_i2c_write(struct stm32_dev *data, u32 address, u8 *src, size_t len);
static int stm32_sysboot_i2c_sync(struct stm32_dev *data, uint8_t *cmd);
static int stm32_sysboot_i2c_get_info(struct stm32_dev *data,
	uint8_t *cmd, uint32_t cmd_size, uint32_t data_size);
static int stm32_sysboot_i2c_read_unprotect(struct stm32_dev *data);
static int stm32_sysboot_i2c_info(struct stm32_dev *data);
static int stm32_sysboot_connect(struct stm32_dev *data);
static int stm32_sysboot_wait_for_ack(struct stm32_dev *data, unsigned long timeout);
static int stm32_sysboot_mcu_chip_command(struct stm32_dev *data, int command);
static int stm32_sysboot_mcu_validation(struct stm32_dev *data);
static void stm32_sysboot_disconnect(struct stm32_dev *data);
static int stm32_target_empty_check_status(struct stm32_dev *data);
static int stm32_target_option_update(struct stm32_dev *data);
static int stm32_target_empty_check_clear(struct stm32_dev *data);
static int stm32_dev_calchecksum(unsigned char *data, int size);
static int stm32_dev_firmware_update(struct stm32_dev *data, const u8 *fw_data, size_t size, int retry);
static void stm32_dev_save_version_of_bin(struct stm32_dev *data, const u8 *fw_info);
static int stm32_dev_check_firmware_version(struct stm32_dev *data, const u8 *fw_info);

/**
  * @brief  Erase the device memory
  * @param  destination(address), length
  * @retval 0 is success, others are fail.
  */
/**
  * @brief  Convert the device memory map to erase param. format.
  *         (start page and numbers to be erased)
  * @param  device memory address, length, erase ref.
  * @retval 0 is success, others are fail.
  */

 /**
  * @brief  Calculate 8-bit checksum.
  * @param  source data and length
  * @retval checksum value.
  */
static u8 stm32_sysboot_checksum(u8 *src, u32 len)
{
	u8 csum = *src++;

	if (len) {
		while (--len)
			csum ^= *src++;
	} else {
		csum = 0; /* error (no length param) */
	}

	return csum;
}

static int stm32_sysboot_conv_memory_map(struct stm32_dev *data, u32 address, size_t len, stm32_erase_param_type *erase)
{
	stm32_page_type *map = stm32_memory_map.pages;
	int found = 0;
	int total_bytes = 0, total_pages = 0;
	int ix = 0;
	int unit = 0;
	input_info(true, &data->client->dev, "%s\n", __func__);

	/* find out the matched starting page number and total page count */

	for (ix = 0; map[ix].size != 0; ++ix) {
		for (unit = 0; unit < map[ix].count; ++unit) {
			/* MATCH CASE: Starting address aligned and page number to be erased */
			if (address == stm32_memory_map.flashbase + total_bytes) {
				found++;
				erase->page = total_pages;
			}
			total_bytes += map[ix].size;
			total_pages++;
			/* MATCH CASE: End of page number to be erased */
			if ((found == 1) && (len <= total_bytes)) {
				found++;
				erase->count = total_pages - erase->page;
			}
		}
	}

	if (found < 2)
		/* Not aligned address or too much length inputted */
		return STM32_BOOT_ERR_DEVICE_MEMORY_MAP;

	if ((address == stm32_memory_map.flashbase) && (erase->count == total_pages)) {
		erase->page = 0xFFFF; /* mark the full erase */
		input_info(true, &data->client->dev, "%s super erase page\n", __func__);
	}

	return 0;
}


/**
  * @brief  Read the device memory
  * @param  source(address), destination, length
  * @retval 0 is success, others are fail.
  */
static int stm32_sysboot_i2c_read(struct stm32_dev *data, u32 address, u8 *dst, size_t len)
{
	u8 cmd[STM32_BOOT_I2C_REQ_CMD_LEN] = {0, }; //BOOT_I2C_REQ_CMD_LEN = 2
	u8 startaddr[STM32_BOOT_I2C_REQ_ADDRESS_LEN] = {0, }; //BOOT_I2C_REQ_ADDRESS_LEN = 5
	u8 nbytes[STM32_BOOT_I2C_READ_PARAM_LEN] = {0, }; //BOOT_I2C_READ_PARAM_LEN = 2
	int ret = 0;
	int retry = 0;

	/* build command */
	cmd[0] = STM32_BOOT_I2C_CMD_READ;
	cmd[1] = ~cmd[0];

	/* build address + checksum */
	*(u32 *)startaddr = HTONL(address);
	startaddr[STM32_BOOT_I2C_ADDRESS_LEN] = stm32_sysboot_checksum(startaddr, STM32_BOOT_I2C_ADDRESS_LEN);

	/* build number of bytes + checksum */
	nbytes[0] = len - 1;
	nbytes[1] = ~nbytes[0];

	for (retry = 0; retry < STM32_BOOT_I2C_SYNC_RETRY_COUNT; ++retry) {
		/* transmit command */
		ret = i2c_master_send(data->client_boot, cmd, sizeof(cmd));
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* transmit address */
		ret = i2c_master_send(data->client_boot, startaddr, sizeof(startaddr));
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* transmit number of bytes */
		ret = i2c_master_send(data->client_boot, nbytes, sizeof(nbytes));
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* receive payload */
		ret = i2c_master_recv(data->client_boot, dst, len);
		if (ret < 0)
		{
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		return 0;
	}

	return ret + STM32_BOOT_ERR_API_READ;
}

/**
  * @brief  Write the contents to the device memory
  * @param  destination(address), source, length
  * @retval 0 is success, others are fail.
  */
static int stm32_sysboot_i2c_write(struct stm32_dev *data, u32 address, u8 *src, size_t len)
{
	u8 cmd[STM32_BOOT_I2C_REQ_CMD_LEN] = {0, };
	u8 startaddr[STM32_BOOT_I2C_REQ_ADDRESS_LEN] = {0, };
	int ret = 0;
	int retry = 0;
	char * buf = NULL;
	/* build command */
	cmd[0] = STM32_BOOT_I2C_CMD_WRITE;
	cmd[1] = ~cmd[0];

	/* build address + checksum */
	*(u32 *)startaddr = HTONL(address);
	startaddr[STM32_BOOT_I2C_ADDRESS_LEN] = stm32_sysboot_checksum(startaddr, STM32_BOOT_I2C_ADDRESS_LEN);

	/* build number of bytes + checksum */

	buf = kzalloc(len + 2, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	buf[0] = len -1;
	memcpy(&buf[1], src, len);
	buf[len+1] = stm32_sysboot_checksum(buf, len + 1);

	for (retry = 0; retry < STM32_BOOT_I2C_SYNC_RETRY_COUNT; ++retry) {
		/* transmit command */
		ret = i2c_master_send(data->client_boot, cmd, 2);
		if (ret < 0) {
			input_info(true, &data->client->dev, "%s Failed txdata\n", __func__);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			input_info(true, &data->client->dev, "%s Failed to wait ack after txdata\n", __func__);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}


		/* transmit address */
		ret = i2c_master_send(data->client_boot, startaddr, 5);
		if (ret < 0) {
			input_info(true, &data->client->dev, "%s Failed txdata\n", __func__);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			input_info(true, &data->client->dev, "%s Failed to wait ack after txdata\n", __func__);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* transmit number of bytes + datas */

		ret = i2c_master_send(data->client_boot, buf, STM32_BOOT_I2C_WRITE_PARAM_LEN(len));
		if (ret < 0) {
			input_info(true, &data->client->dev, "%s Failed txdata\n", __func__);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		//msleep(len);
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_WRITE_TMOUT);
		if (ret < 0) {
			input_info(true, &data->client->dev, "%s Failed to wait ack after txdata\n", __func__);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		kfree(buf);

		return 0;
	}
	msleep(2);
	kfree(buf);

	return ret + STM32_BOOT_ERR_API_WRITE;
}

static int stm32_sysboot_i2c_erase(struct stm32_dev *data, u32 address, size_t len)
{
	u8 cmd[STM32_BOOT_I2C_REQ_CMD_LEN] = {0, };
	stm32_erase_param_type erase;
	u8 xmit_bytes = 0;
	int ret = 0;
	int retry = 0;
	u8 *xmit = NULL;

	/* build command */
	cmd[0] = STM32_BOOT_I2C_CMD_ERASE;
	cmd[1] = ~cmd[0];

	/* build erase parameter */
	ret = stm32_sysboot_conv_memory_map(data, address, len, &erase);
	if (ret < 0)
		return ret + STM32_BOOT_ERR_API_ERASE;

	input_info(true, &data->client->dev, "%s erase.page 0x%x\n", __func__, erase.page);

	xmit = kmalloc(1024, GFP_KERNEL | GFP_DMA);
	if (xmit == NULL) {
		input_info(true, &data->client->dev, "%s out of memory\n", __func__);
		return ret + STM32_BOOT_ERR_API_ERASE;
	}

	memset(xmit, 0, 1024);

	for (retry = 0; retry < STM32_BOOT_I2C_SYNC_RETRY_COUNT; ++retry) {
		/* build full erase command */
		if (erase.page == 0xFFFF)		
			*(u16 *)xmit = (u16)erase.page;
		/* build page erase command */
		else
			*(u16 *)xmit = HTONS((erase.count - 1));

		xmit_bytes = sizeof(u16);
		xmit[xmit_bytes] = stm32_sysboot_checksum(xmit, xmit_bytes);
		xmit_bytes++;
		/* transmit command */
		stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
		ret = i2c_master_send(data->client_boot, cmd, sizeof(cmd));
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		
		stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);

		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* transmit parameter */
		ret = i2c_master_send(data->client_boot, xmit, xmit_bytes);
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		//stm32_delay(2*32);
		ret = stm32_sysboot_wait_for_ack(data, (erase.page == 0xFFFF) ? STM32_BOOT_I2C_FULL_ERASE_TMOUT : STM32_BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* case of page erase */
		if (erase.page != 0xFFFF) {
			/* build page erase parameter */
			register int ix;
			register u16 *pbuf = (u16 *)xmit;
			for (ix = 0; ix < erase.count; ++ix)
				pbuf[ix] = HTONS((erase.page + ix));

			input_info(true, &data->client->dev, "%s erase.count %d\n", __func__, erase.count);
			input_info(true, &data->client->dev, "%s &pbuf[ix] %pK,xmit %pK\n", __func__, &pbuf[ix], xmit);
			xmit_bytes = 2 * erase.count;
			*((u8 *)&pbuf[ix]) = stm32_sysboot_checksum(xmit, xmit_bytes);
			input_info(true, &data->client->dev, "%s xmit_bytes %d\n", __func__, xmit_bytes);
			xmit_bytes++;
			/* transmit parameter */
			ret = i2c_master_send(data->client_boot, xmit, xmit_bytes);
			if (ret < 0) {
				stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
				continue;
			}
			stm32_delay(250);
			/* wait for ACK response */
			ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_PAGE_ERASE_TMOUT(erase.count + 1));
			if (ret < 0) {
				stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
				continue;
			}
		}

		input_info(true, &data->client->dev, "%s finished\n", __func__);
		kfree(xmit);
		return 0;
	}

	kfree(xmit);
	return ret + STM32_BOOT_ERR_API_ERASE;
}


static int stm32_sysboot_connect(struct stm32_dev *data)
{
	struct stm32_devicetree_data *dtdata = data->dtdata;
	int ret = 0;

	input_info(true, &data->client->dev, "%s start\n", __func__);

	/* Assert NRST reset */
	gpio_direction_output(dtdata->mcu_nrst, 0);
	/* Change BOOT pins to System Bootloader */
	gpio_direction_output(dtdata->mcu_swclk, 1);
	/* NRST should hold down (Vnf(NRST) > 300 ns), considering capacitor, give enough time */
	stm32_delay(3);
	/* Release NRST reset */
	gpio_direction_output(dtdata->mcu_nrst, 1);
	/* Put little delay for the target prepared */
	stm32_delay(STM32_BOOT_I2C_STARTUP_DELAY);
	gpio_direction_output(dtdata->mcu_swclk, 0);

	/* STEP2. Send SYNC frame then waiting for ACK */
	ret = stm32_sysboot_mcu_chip_command(data, STM32_BOOT_I2C_CMD_SYNC);

	if (ret >= 0) {
		/* STEP3. When I2C mode, Turn to the MCU system boot mode once again for protocol == SYSBOOT_PROTO_I2C */
		/* Assert NRST reset */
		gpio_direction_output(dtdata->mcu_nrst, 0);
		gpio_direction_output(dtdata->mcu_swclk, 1);
		/* NRST should hold down (Vnf(NRST) > 300 ns), considering capacitor, give enough time */
		stm32_delay(3);
		/* Release NRST reset */
		gpio_direction_output(dtdata->mcu_nrst, 1);
		/* Put little delay for the target prepared */
		stm32_delay(STM32_BOOT_I2C_STARTUP_DELAY);
		gpio_direction_output(dtdata->mcu_swclk, 0);
	}

	return ret;
}

static int stm32_sysboot_wait_for_ack(struct stm32_dev *data, unsigned long timeout)
{
	int ret = 0;
	u32 retry = 3;
	char resp = 0;

	while (retry--) {
		ret = i2c_master_recv(data->client_boot, &resp, 1);
		if(ret >= 0) {
			if(!(resp == STM32_BOOT_I2C_RESP_ACK))
				input_info(true, &data->client->dev, "%s Failed to wait ack 0x%x \n", __func__, resp);
			return 0;
		} else {
			input_info(true, &data->client->dev, "%s Failed resp 0x%x , ret is %d	\n", __func__, resp, ret);
			if (time_after(jiffies, timeout)) {
				ret = -ETIMEDOUT;
				break;
			}
			stm32_delay(STM32_BOOT_I2C_INTER_PKT_BACK_INTVL + 2);
		}
	}
	return -1;

}

/**
  * @brief  Unprotect the read protect
  * @param  None
  * @retval 0 is success, others are fail.
  */
static int stm32_sysboot_i2c_read_unprotect(struct stm32_dev *data)
{
	u8 cmd[STM32_BOOT_I2C_REQ_CMD_LEN] = {0, };
	int ret = 0;
	int retry = 0;

	/* build command */
	cmd[0] = STM32_BOOT_I2C_CMD_READ_UNPROTECT;
	cmd[1] = ~cmd[0];

	input_info(true, &data->client->dev, "%s\n", __func__);

	for (retry = 0; retry < STM32_BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
	{
		/* transmit command */
		ret = i2c_master_send(data->client_boot, cmd, sizeof(cmd));
		if (ret < 0)
		{
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0)
		{
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0)
		{
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		return 0;
	}

	return ret + STM32_BOOT_ERR_API_READ_UNPROTECT;
}


/**
  * @brief  Getting STATUS of the TARGET empty check
  * @param  None
  * @retval 0: empty check reset, 1: empty check set, others are fail
  */
static int stm32_target_empty_check_status(struct stm32_dev *data)
{
	u32 value = 0;
	int ret = 0;
	input_info(true, &data->client->dev, "%s\n",__func__);

	/* Read first flash memory word ------------------------------------------- */
	ret = stm32_sysboot_i2c_read(data, stm32_memory_map.flashbase, (u8 *)&value, sizeof(value));
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s error: (%d)\n", __func__, ret);
		goto empty_check_status_fail;
	}

	input_info(true, &data->client->dev, "%s Flash Word: 0x%08X\n", __func__, value);

	if (value == 0xFFFFFFFF)
		return 1;

	return 0;

empty_check_status_fail:

	return -1;
}


static int stm32_target_option_update(struct stm32_dev *data)
{
	int ret = 0;
	int optionbyte = 0;
	int retry = 3;

	input_info(true, &data->client->dev, "%s\n", __func__);

	for (retry = 0; retry < 3; retry ++ ) {
		ret = stm32_sysboot_i2c_read(data, stm32_memory_map.optionbyte, (u8 *)&optionbyte, sizeof(optionbyte));
		if ((ret < 0) || ((optionbyte & 0xff) != 0xaa)) {
			ret = stm32_sysboot_i2c_read_unprotect(data);
			if (ret < 0)
				input_err(true, &data->client->dev, "%s Failed read_unprotect\n", __func__);
			else
				input_info(true, &data->client->dev, "%s succeed read_unprotect\n", __func__);

			stm32_delay(60);
			ret = stm32_sysboot_connect(data);
			/* try connection again */
			continue;
		}

		if (optionbyte & (1 << 24)) {
		/* Option byte write ---------------------------------------------------- */
			optionbyte &= ~(1 << 24);
			ret = stm32_sysboot_i2c_write(data, stm32_memory_map.optionbyte, (u8 *)&optionbyte, sizeof(optionbyte));
			if (ret < 0) {
				input_err(true, &data->client->dev, "%s Failed to write optionbyte \n", __func__);
				stm32_delay(1);
				continue;
			}
			input_err(true, &data->client->dev, "%s write optionbyte ok\n", __func__);
			//try connection again
		} else {
			input_err(true, &data->client->dev, "%s optionbyte is 0, success\n", __func__);
			return 0;
		}
	}

	return ret;
}

static int stm32_target_empty_check_clear(struct stm32_dev *data)
{
	int ret = 0;
	u32 optionbyte = 0;

	/* Option Byte read ------------------------------------------------------- */
	ret = stm32_sysboot_i2c_read(data, stm32_memory_map.optionbyte, (u8 *)&optionbyte, sizeof(optionbyte));
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s Failed to read Option Byte \n", __func__);
		goto empty_check_clear_fail;
	}

	input_info(true, &data->client->dev, "%s read Option Byte 0x%x\n", __func__, optionbyte);

	/* Option byte write (dummy: readed value) -------------------------------- */
	ret = stm32_sysboot_i2c_write(data, stm32_memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s Failed to write Option Byte\n", __func__);
		goto empty_check_clear_fail;
	}
	input_err(true, &data->client->dev, "%s write Option Byte 0x%x\n", __func__, optionbyte);

	/* Put little delay for Target program option byte and self-reset */
	stm32_delay(150);
	/* Option byte read for checking protection status ------------------------ */
	/* 1> Re-connect to the target */
	ret = stm32_sysboot_connect(data);
	if (ret) {
		input_err(true, &data->client->dev, "%s Cannot connect to the target for RDP check (%d)\n", __func__, ret);
		goto empty_check_clear_fail;
	}

	/* 2> Read from target for status checking and recover it if needed */
	ret = stm32_sysboot_i2c_read(data, stm32_memory_map.optionbyte, (u8 *)&optionbyte, sizeof(optionbyte));
	if ((ret < 0) || ((optionbyte & 0x000000FF) != 0xAA)) {
		input_err(true, &data->client->dev, "%s Failed to read option byte from target (%d)\n", __func__, ret);
		/* Tryout the RDP level to 0 */
		ret = stm32_sysboot_i2c_read_unprotect(data);
		if (ret)
			input_err(true, &data->client->dev, "%s Readout unprotect Not OK ... Host restart and try again\n", __func__);
		else
			input_err(true, &data->client->dev, "%s Readout unprotect OK ... Host restart and try again\n", __func__);
		/* Put little delay for Target erase all of pages */
		stm32_delay(50);
		goto empty_check_clear_fail;
	}

	return 0;
empty_check_clear_fail:
	return -1;
}

static int stm32_sysboot_i2c_sync(struct stm32_dev *data, uint8_t *cmd)
{
	int ret = 0;

	input_info(true, &data->client->dev, "%s \n", __func__);
	/* set it and wait for it to be so */
	ret = stm32_i2c_write_burst(data->client_boot, cmd, 1);
	if (ret >= 0)
		input_info(true, &data->client->dev, "%s succeed to connect to target\n", __func__);
	else
		input_info(true, &data->client->dev, "%s Failed to connect target\n", __func__);
	return ret;
}

static int stm32_sysboot_i2c_get_info(struct stm32_dev *data,
	uint8_t *cmd, uint32_t cmd_size, uint32_t data_size)
{
	uint8_t recv[STM32_BOOT_I2C_RESP_GET_ID_LEN] = {0, };
	int ret = 0;
	int retry = 0;

	input_info(true, &data->client->dev, "%s cmd[0]:0x%x, cmd[1]:0x%x\n", __func__, cmd[0], cmd[1]);

	for (retry = 0; retry < STM32_BOOT_I2C_SYNC_RETRY_COUNT; ++retry) {
		/* transmit command */
		ret = stm32_i2c_write_burst(data->client_boot, cmd, cmd_size);
		if (ret < 0) {
			input_err(true, &data->client->dev, "%s Failed to send data, ret = %d\n", __func__, ret);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			input_err(true, &data->client->dev, "%s Failed to wait ack, ret = %d\n", __func__, ret);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* receive payload */
		ret = stm32_i2c_read_bulk(data->client_boot, recv, data_size);
		if (ret < 0) {
			input_err(true, &data->client->dev, "%s Failed to receive payload, ret = %d\n", __func__, ret);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = stm32_sysboot_wait_for_ack(data, STM32_BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			input_err(true, &data->client->dev, "%s faield to wait ack,ret = %d\n", __func__, ret);
			stm32_delay(STM32_BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		if (cmd[0] == STM32_BOOT_I2C_CMD_GET_ID) {
			memcpy((void *)&(data->icdata.id), &recv[1], recv[0] + 1);
			data->icdata.id = NTOHS(data->icdata.id);
			input_err(true, &data->client->dev, "%s succeed to get info id %d\n", __func__, data->icdata.id);
		} else if(cmd[0] == STM32_BOOT_I2C_CMD_GET_VER) {
			memcpy((void *)&(data->icdata.ver), recv , 1);
			input_err(true, &data->client->dev, "%s succeed to get info version %d\n", __func__, data->icdata.ver);
		}

		return 0;
	}

	return ret + cmd[0];
}

static int stm32_sysboot_mcu_chip_command(struct stm32_dev *data, int command)
{
	/* build command */
	uint8_t cmd[STM32_BOOT_I2C_REQ_CMD_LEN] = {0, };
	int ret = 0;
	input_info(true, &data->client->dev,"%s start\n", __func__);

	/* execute the command */
	switch (command) {
	case STM32_BOOT_I2C_CMD_GET:
		cmd[0] = 0x00;
		break;

	case STM32_BOOT_I2C_CMD_GET_VER:
		cmd[0] = 0x01;
		cmd[1] = ~cmd[0];
		ret = stm32_sysboot_i2c_get_info(data, cmd, 2, 1);
		break;

	case STM32_BOOT_I2C_CMD_GET_ID:
		cmd[0] = 0x02;
		cmd[1] = ~cmd[0];
		ret = stm32_sysboot_i2c_get_info(data, cmd, 2, 3);
		break;

	case STM32_BOOT_I2C_CMD_READ:
		cmd[0] = 0x11;
		break;

	case STM32_BOOT_I2C_CMD_WRITE:
		cmd[0] = 0x31;
		break;

	case STM32_BOOT_I2C_CMD_ERASE:
		cmd[0] = 0x44;
		break;

	case STM32_BOOT_I2C_CMD_GO:
		cmd[0] = 0x21;
		break;

	case STM32_BOOT_I2C_CMD_WRITE_UNPROTECT:
		cmd[0] = 0x73;
		break;

	case STM32_BOOT_I2C_CMD_READ_UNPROTECT:
		cmd[0] = 0x92;
		break;

	case STM32_BOOT_I2C_CMD_SYNC:
		/* UNKNOWN command */
		cmd[0] = 0xFF;
		ret = stm32_sysboot_i2c_sync(data, cmd);
		break;

	default:
		break;
		return -EINVAL;
	}

	return ret ;
}

static int stm32_sysboot_i2c_info(struct stm32_dev *data)
{
	int ret = 0;
	input_info(true, &data->client->dev, "mcu %s \n",__func__);
	memset((void *)&(data->icdata), 0x00, sizeof(data->icdata));
	stm32_sysboot_mcu_chip_command(data, STM32_BOOT_I2C_CMD_GET_ID);
	stm32_sysboot_mcu_chip_command(data, STM32_BOOT_I2C_CMD_GET_VER);
	return ret;
}

static void stm32_sysboot_disconnect(struct stm32_dev *data)
{
	struct stm32_devicetree_data *dtdata = data->dtdata;
	input_info(true, &data->client->dev, "%s start\n", __func__);
	/* Change BOOT pins to Main flash */
	gpio_direction_output(dtdata->mcu_swclk, 0);
	stm32_delay(1);
	/* Assert NRST reset */
	gpio_direction_output(dtdata->mcu_nrst, 0);
	/* NRST should hold down (Vnf(NRST) > 300 ns), considering capacitor, give enough time */
	stm32_delay(2);
	/* Release NRST reset */
	gpio_direction_output(dtdata->mcu_nrst, 1);
	stm32_delay(150);
}


static int stm32_sysboot_mcu_validation(struct stm32_dev *data)
{
	int ret = 0;
	input_info(true, &data->client->dev, "%s start\n", __func__);

	ret = stm32_sysboot_connect(data);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s Cannot connect to the target (%d) but skip\n", __func__, ret);
		goto validation_fail;
	}
	input_info(true, &data->client->dev, "%s Connection OK\n", __func__);

	ret = stm32_sysboot_i2c_info(data);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s Failed to collect the target info (%d)\n", __func__, ret);
		goto validation_fail;
	}

	input_info(true, &data->client->dev, "%s Get target info OK Target PID: 0x%X,"
			"Bootloader version: 0x%X\n", __func__, data->icdata.id,  data->icdata.ver);

	return 0;

validation_fail:
	stm32_sysboot_disconnect(data);
	input_info(true, &data->client->dev, "%s Failed target disconnected\n", __func__);

	return -1;
	
}

static int stm32_dev_calchecksum(unsigned char *data, int size)
{
	int i = 0;
	int result = 0;

	for (i = 0; i < size; i += 2)
		result = result + (0xFFFF & (((*(data + i + 1)) << 8) | (*(data + i))));

	return result;
}

static int stm32_dev_firmware_update(struct stm32_dev *data, const u8 *fw_data, size_t size, int retry)
{
	int ret = 0;
	int checksum = 0;
	int empty_check_en = 0;
	int len = 0;
	int i = 0;
	u32 address = 0;
	u32 wbytes = 0;
	u32 unit = STM32_DEV_FW_UPDATE_PACKET_SIZE;
	u8 senddata[STM32_DEV_FW_UPDATE_PACKET_SIZE] = "";
	u8 *tmp_data = NULL;

	tmp_data = kzalloc(size, GFP_KERNEL);
	if (!tmp_data) {
		input_err(true, &data->client->dev,
				"%s: Failed to kmalloc\n", __func__);
		return -ENOMEM;
	}

	memcpy(tmp_data, fw_data, size);

	/* verify checkSum */
	checksum = stm32_dev_calchecksum(tmp_data, size);
	input_info(true, &data->client->dev, "%s: cal checksum:0x%04X\n", __func__, checksum);

	/* boot loader mode */
	ret = stm32_sysboot_mcu_validation(data);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: Failed mcu validation\n", __func__);
		goto ERROR;
	}

	/* check_option_byte */
	stm32_target_option_update(data);

	/* check empty status */
	empty_check_en = stm32_target_empty_check_status(data);

	/* erase */

	stm32_sysboot_i2c_erase(data, stm32_memory_map.flashbase, 65536 - 2048);

	address = stm32_memory_map.flashbase;
	len = size;

	input_err(true, &data->client->dev, "%s len=%d\n", __func__, len);
		/* Write UserProgram Data */
	while (len > 0) {
		wbytes = (len > unit) ? unit : len;
	  	/* write the unit */
		for (i = 0; i < wbytes; i++ )
			senddata[i] = tmp_data[i];
		ret = stm32_sysboot_i2c_write(data, address, senddata, wbytes);
		if (ret < 0) {
			input_err(true, &data->client->dev, "%s Failed to write stm32_fw_debug i2c byte prog code\n", __func__);
			stm32_sysboot_disconnect(data);
			goto ERROR;
		} 
			address += wbytes;
			tmp_data += wbytes;
			len -= wbytes;
	}
	tmp_data = tmp_data - (address - stm32_memory_map.flashbase);

	if (empty_check_en > 0) {
		if (stm32_target_empty_check_clear(data)<0) {
			ret = -1;
			stm32_sysboot_disconnect(data);
			goto ERROR;
		}
	}

	ret = stm32_sysboot_i2c_read(data, STM32_IC_VERSION_OFFSET, data->mdata.ic_ver, STM32_DEV_FW_VER_SIZE);
	if (ret < 0)
		input_err(true, &data->client->dev, "%s Failed to read firmware ic ver :%d info (%d)\n",
			__func__, data->mdata.ic_ver[3], ret);



	/* sysboot_disconnect */
	stm32_sysboot_disconnect(data);
	
	input_info(true, &data->client->dev, "%s: done\n", __func__);

	ret = 0;
ERROR:
	kfree(tmp_data);
	return ret;
}

static void stm32_dev_save_version_of_bin(struct stm32_dev *data, const u8 *fw_info)
{
	data->mdata.phone_ver[0] = *(fw_info + STM32_DEV_VERSION_OFFSET);
	data->mdata.phone_ver[1] = *(fw_info + STM32_DEV_VERSION_OFFSET + 1);
	data->mdata.phone_ver[2] = *(fw_info + STM32_DEV_VERSION_OFFSET + 2);
	data->mdata.phone_ver[3] = *(fw_info + STM32_DEV_VERSION_OFFSET + 3);

	input_info(true, &data->client->dev, "%s %d%d%d%d\n", __func__,
			data->mdata.phone_ver[0], data->mdata.phone_ver[1],
			data->mdata.phone_ver[2], data->mdata.phone_ver[3]);
}

static int stm32_dev_check_firmware_version(struct stm32_dev *data, const u8 *fw_info)
{
//	u8 buff[1];
//	int i;
//	int ret;
	/*
	 * sec_ts_check_firmware_version
	 * return value = 1 : firmware download needed,
	 * return value = 0 : skip firmware download
	 */

	stm32_dev_save_version_of_bin(data, fw_info);

	input_info(true, &data->client->dev, "%s phone:%02x%02x%02x%02x ic:%02x%02x%02x%02x\n", __func__,
			data->mdata.phone_ver[0], data->mdata.phone_ver[1],
			data->mdata.phone_ver[2], data->mdata.phone_ver[3],
			data->mdata.ic_ver[0], data->mdata.ic_ver[1],
			data->mdata.ic_ver[2], data->mdata.ic_ver[3]);

	if (data->mdata.phone_ver[3] > data->mdata.ic_ver[3])
		return 1;
	else if (data->mdata.phone_ver[3] == 0xff)
		return 1;
	else if ((data->mdata.phone_ver[3] != 0xff) && (data->mdata.ic_ver[3] == 0xff))
		return 1;

	return 0;
	
}

static int stm32_dev_firmware_update_on_probe(struct stm32_dev *data)
{
	const struct firmware *fw_entry;
	char fw_path[STM32_DEV_MAX_FW_PATH];
	int result = -1;
	int ii = 0;
	int ret = 0;

	input_info(true, &data->client->dev, "%s start\n", __func__);

	if (data->dtdata->mcu_fw_name) {
		snprintf(fw_path, STM32_DEV_MAX_FW_PATH, "%s", data->dtdata->mcu_fw_name);
	} else {
		input_err(true, &data->client->dev, "%s: don't exist mcu_fw_name\n", __func__);
		return -1;
	}

	disable_irq(data->client->irq);

	if (request_firmware(&fw_entry, fw_path, &data->client->dev) !=  0) {
		input_err(true, &data->client->dev, "%s: Failed to request firmware\n", __func__);
		goto err_request_fw;
	}

	
	ret = stm32_sysboot_mcu_validation(data);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: Failed mcu validation\n", __func__);
		goto err_mcu_validation;
	}

	ret = stm32_sysboot_i2c_read(data, STM32_IC_VERSION_OFFSET, data->mdata.ic_ver, STM32_DEV_FW_VER_SIZE);
	if (ret < 0) {
		data->mdata.ic_ver[3] = 0;
		input_err(true, &data->client->dev, "%s Failed to read firmware ic ver :%d info (%d)\n",
				__func__, data->mdata.ic_ver[3], ret);
	}

	stm32_sysboot_disconnect(data);

	result = stm32_dev_check_firmware_version(data, fw_entry->data);
	if (result <= 0) {
		input_info(true, &data->client->dev, "%s: skip - fw update\n", __func__);
		goto err_result;
	} else {	/* firmup case */

		for (ii = 0; ii < 3; ii++) {
			ret = stm32_dev_firmware_update(data, fw_entry->data, fw_entry->size, ii);
			if (ret >= 0)
				break;
		}

		if (ret < 0) {
			result = -1;
			goto err_result;
		}
		result = 0;
	}

err_result:
err_mcu_validation:
	release_firmware(fw_entry);
err_request_fw:
	enable_irq(data->client->irq);
	return result;
}

static int stm32_dev_firmware_update_external(struct stm32_dev *data)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fw_size, nread;
	int result = -1;
	int ii = 0;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	input_info(true, &data->client->dev, "%s start\n", __func__);

	disable_irq(data->client->irq);

	fp = filp_open(KEYBOARD_PATH_EXTERNAL_FW, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		input_err(true, &data->client->dev, "%s: failed to open %s.\n", __func__,
				KEYBOARD_PATH_EXTERNAL_FW);
		result = -ENOENT;
		goto err_open;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (fw_size > 0) {
		unsigned char *fw_data;

		fw_data = vzalloc(fw_size);
		if (!fw_data) {
			input_err(true, &data->client->dev, "%s: failed to alloc mem\n", __func__);
			result = -ENOMEM;
			filp_close(fp, NULL);
			goto err_vzalloc;
		}
		nread = vfs_read(fp, (char __user *)fw_data,
				fw_size, &fp->f_pos);

		input_info(true, &data->client->dev,
				"%s: start, file path %s, size %ld Bytes\n",
				__func__, KEYBOARD_PATH_EXTERNAL_FW, fw_size);

		if (nread != fw_size) {
			input_err(true, &data->client->dev,
					"%s: failed to read firmware file, nread %ld Bytes\n",
					__func__, nread);
			result = -EIO;
		} else {		
			ret = stm32_sysboot_mcu_validation(data);
			if (ret < 0) {
				input_err(true, &data->client->dev, "%s: Failed mcu validation\n", __func__);
				vfree(fw_data);
				goto err_mcu_validation;
			}

			ret = stm32_sysboot_i2c_read(data, STM32_IC_VERSION_OFFSET, data->mdata.ic_ver, STM32_DEV_FW_VER_SIZE);
			if (ret < 0) {
				data->mdata.ic_ver[3] = 0;
				input_err(true, &data->client->dev, "%s Failed to read firmware ic ver :%d info (%d)\n",
						__func__, data->mdata.ic_ver[3], ret);
			}

			stm32_sysboot_disconnect(data);
				/* firmup case */

			for (ii = 0; ii < 3; ii++) {
				ret = stm32_dev_firmware_update(data, fw_data, fw_size, ii);
				if (ret >= 0)
					break;
			}

			if (ret < 0) {
				result = -1;
				vfree(fw_data);
				goto err_request_fw;
			}
			result = 0;
		}
		vfree(fw_data);
	}
err_request_fw:
err_mcu_validation:
err_vzalloc:
	filp_close(fp, NULL);
err_open:
	set_fs(old_fs);
	enable_irq(data->client->irq);
	return result;
}


int stm32_dev_firmware_update_menu(struct stm32_dev *data, int update_type)
{
	int ret = 0;

	/* Factory cmd for firmware update
	 * argument represent what is source of firmware like below.
	 *
	 * 0 : [BUILT_IN] Getting firmware which is for user.
	 * 1 : [UMS] Getting firmware from sd card.
	 * 2 : none
	 * 3 : [FFU] Getting firmware from air.
	 */

	switch (update_type) {
	case 0:
		ret = stm32_dev_firmware_update_on_probe(data);
		break;
	case 1:
		ret = stm32_dev_firmware_update_external(data);
		break;
	default:
		input_err(true, &data->client->dev, "%s: Not support command[%d]\n",
				__func__, update_type);
		break;
	}

	return ret;
}

int stm32_dev_get_ic_ver(struct stm32_dev *data)
{
	int ret;

	ret = stm32_sysboot_mcu_validation(data);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: Failed mcu validation\n", __func__);
		return ret;
	}

	ret = stm32_sysboot_i2c_read(data, STM32_IC_VERSION_OFFSET, data->mdata.ic_ver, STM32_DEV_FW_VER_SIZE);
	if (ret < 0) {
		data->mdata.ic_ver[3] = 0;
		input_err(true, &data->client->dev, "%s Failed to read firmware ic ver :%d info (%d)\n",
				__func__, data->mdata.ic_ver[3], ret);
	}

	stm32_sysboot_disconnect(data);
	return ret;
}
