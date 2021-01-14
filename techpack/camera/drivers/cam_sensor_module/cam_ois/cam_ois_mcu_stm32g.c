/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/crc32.h>
#include <cam_sensor_cmn_header.h>
#include <cam_sensor_util.h>
#include <cam_sensor_io.h>
#include <cam_req_mgr_util.h>
#include "cam_debug_util.h"
#include <cam_sensor_i2c.h>
#include "cam_ois_mcu_stm32g.h"
#include "cam_ois_thread.h"
#include "cam_ois_core.h"
#include "cam_eeprom_dev.h"
#if defined(CONFIG_SAMSUNG_APERTURE)
#include "cam_aperture_core.h"
#endif
#if defined(CONFIG_SAMSUNG_OIS_TAMODE_CONTROL)
#include <linux/battery/sec_charging_common.h>
#endif

#define OIS_FW_STATUS_OFFSET	(0x00FC)
#define OIS_FW_STATUS_SIZE		(4)
#define OIS_HW_VERSION_SIZE 	(3)
#define OIS_MCU_VERSION_SIZE 	(4)
#define OIS_MCU_VDRINFO_SIZE 	(4)
#define OIS_HW_VERSION_OFFSET	(0xAFF1)
#define OIS_FW_VERSION_OFFSET	(0xAFED)
#define OIS_MCU_VERSION_OFFSET	(0x80F8)
#define OIS_MCU_VDRINFO_OFFSET	(0x807C)
#define OIS_FW_PATH "/vendor/lib64/camera"
#define OIS_MCU_FW_NAME "ois_mcu_stm32g_fw.bin"
#define OIS_USER_DATA_START_ADDR (0xB400)
#define OIS_FW_UPDATE_PACKET_SIZE (256)
#define PROGCODE_SIZE			(1024 * 44)
#define MAX_RETRY_COUNT 		 (3)
#define OIS_GYRO_SCALE_FACTOR_LSM6DSO (114)
#define MAX_EFS_DATA_LENGTH     (30)
#define OIS_GYRO_CAL_VALUE_FROM_EFS "/efs/FactoryApp/camera_ois_gyro_cal"
#if defined(CONFIG_SEC_C2Q_PROJECT)
#define MAX_EFS_DATA_LENGTH_TELE     (10)
#define OIS_TELE_CAMERA_TILT_CALIBRATION_EFS "/efs/FactoryApp/camera_tilt_calibration_info_3_1"
#endif

extern char ois_fw_full[40];
extern char ois_debug[40];

uint8_t ois_wide_xygg[OIS_XYGG_SIZE] = { 0, };
uint8_t ois_wide_cal_mark = 0;
#if defined(CONFIG_SEC_C2Q_PROJECT)
bool efs_cal_data_read_done = FALSE;
#endif
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
uint8_t ois_wide_center_shift[OIS_CENTER_SHIFT_SIZE] = { 0, };
uint8_t ois_tele_xygg[OIS_XYGG_SIZE] = { 0, };
uint8_t ois_tele_center_shift[OIS_CENTER_SHIFT_SIZE] = { 0, };
uint8_t ois_tele_cal_mark = 0;
#endif

#if defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
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
#endif

int total_fw_size;

//for mcu sysboot

/* Target specific definitions
 *  1. Startup delay
 *     STM32 target needs at least t-ms delay after reset msecs
 *  2. Target I2C slave dddress
 */
const uint32_t sysboot_i2c_startup_delay = 50; /* msec */
const uint16_t sysboot_i2c_slave_address = 0x51;

/* STM32MCU PID */
const uint16_t product_id = 0x460;

/* Flash memory characteristics from target datasheet (msec unit) */
const uint32_t flash_prog_time = 37; /* per page or sector */
const uint32_t flash_full_erase_time = 40 * 32; /* 2K erase time(40ms) * 32 pages */
const uint32_t flash_page_erase_time = 36; /* per page or sector */

/* Memory map specific */

typedef struct
{
	uint32_t size;
	uint32_t count;
} sysboot_page_type;

typedef struct
{
	uint32_t flashbase;  /* flash memory starting address */
	uint32_t sysboot;    /* system memory starting address */
	uint32_t optionbyte; /* option byte starting address */
	sysboot_page_type *pages;
} sysboot_map_type;

/* Flash memory page(or sector) structure */
sysboot_page_type memory_pages[] = {
  {2048, 32},
  {   0,  0}
};

sysboot_map_type memory_map =
{
	0x08000000, /* flash memory starting address */
	0x1FFF0000, /* system memory starting address */
	0x1FFF7800, /* option byte starting address */
	(sysboot_page_type *)memory_pages,
};

static int ois_mcu_chip_command(struct cam_ois_ctrl_t *o_ctrl, int command);

/**
  * @brief  Connect to the device and do SYNC transaction.
  *         Reset(NRST) and BOOT0 pin control
  * @param  protocol
  * @retval 0: success, others are fail.
  */
int sysboot_connect(struct cam_ois_ctrl_t *o_ctrl)
{
	int ret = 0;
	CAM_INFO(CAM_OIS, "mcu sysboot_connect");

	/* STEP1. Turn to the MCU system boot mode */
	{
		/* Assert NRST reset */
		gpio_direction_output(o_ctrl->reset_ctrl_gpio, 0);
		/* Change BOOT pins to System Bootloader */
		gpio_direction_output(o_ctrl->boot0_ctrl_gpio, 1);
		/* NRST should hold down (Vnf(NRST) > 300 ns), considering capacitor, give enough time */
		usleep_range(BOOT_NRST_PULSE_INTVL* 1000,
			BOOT_NRST_PULSE_INTVL* 1000 + 1000);
		/* Release NRST reset */
		gpio_direction_output(o_ctrl->reset_ctrl_gpio, 1);
		/* Put little delay for the target prepared */
		msleep(BOOT_I2C_STARTUP_DELAY);
		gpio_direction_output(o_ctrl->boot0_ctrl_gpio, 0);
	}
	/* STEP2. Send SYNC frame then waiting for ACK */
	ret = ois_mcu_chip_command(o_ctrl, BOOT_I2C_CMD_SYNC);

	if (ret >= 0)
	{
		/* STEP3. When I2C mode, Turn to the MCU system boot mode once again for protocol == SYSBOOT_PROTO_I2C */
		{
			/* Assert NRST reset */
			gpio_direction_output(o_ctrl->reset_ctrl_gpio, 0);
			gpio_direction_output(o_ctrl->boot0_ctrl_gpio, 1);
			/* NRST should hold down (Vnf(NRST) > 300 ns), considering capacitor, give enough time */
			usleep_range(BOOT_NRST_PULSE_INTVL* 1000,
				BOOT_NRST_PULSE_INTVL* 1000 + 1000);
			/* Release NRST reset */
			gpio_direction_output(o_ctrl->reset_ctrl_gpio, 1);
			/* Put little delay for the target prepared */
			msleep(BOOT_I2C_STARTUP_DELAY);
			gpio_direction_output(o_ctrl->boot0_ctrl_gpio, 0);
		}
	}

	return ret;
}

/**
  * @brief  Disconnect the device
  *         Reset(NRST) and BOOT0 pin control
  * @param  protocol
  * @retval None
  */
void sysboot_disconnect(struct cam_ois_ctrl_t *o_ctrl)
{
	CAM_INFO(CAM_OIS, "sysboot disconnect");
	/* Change BOOT pins to Main flash */
	gpio_direction_output(o_ctrl->boot0_ctrl_gpio, 0);
	usleep_range(1000, 1100);
	/* Assert NRST reset */
	gpio_direction_output(o_ctrl->reset_ctrl_gpio, 0);
	/* NRST should hold down (Vnf(NRST) > 300 ns), considering capacitor, give enough time */
	usleep_range(BOOT_NRST_PULSE_INTVL* 1000, BOOT_NRST_PULSE_INTVL* 1000 + 1000);
	/* Release NRST reset */
	gpio_direction_output(o_ctrl->reset_ctrl_gpio, 1);
	msleep(150);
}

/**
  * @brief  Convert the device memory map to erase param. format.
  *         (start page and numbers to be erased)
  * @param  device memory address, length, erase ref.
  * @retval 0 is success, others are fail.
  */
int sysboot_conv_memory_map(struct cam_ois_ctrl_t *o_ctrl, uint32_t address, size_t len, sysboot_erase_param_type *erase)
{
	sysboot_page_type *map = memory_map.pages;
	int found = 0;
	int total_bytes = 0, total_pages = 0;
	int ix = 0;
	int unit = 0;
	CAM_INFO(CAM_OIS, "mcu");

	/* find out the matched starting page number and total page count */

	for (ix = 0; map[ix].size != 0; ++ix)
	{
		for (unit = 0; unit < map[ix].count; ++unit)
		{
			/* MATCH CASE: Starting address aligned and page number to be erased */
			if (address == memory_map.flashbase + total_bytes)
			{
				found++;
				erase->page = total_pages;
			}
			total_bytes += map[ix].size;
			total_pages++;
			/* MATCH CASE: End of page number to be erased */
			if ((found == 1) && (len <= total_bytes))
			{
				found++;
				erase->count = total_pages - erase->page;
			}
		}
	}

	if (found < 2)
	{
		/* Not aligned address or too much length inputted */
		return BOOT_ERR_DEVICE_MEMORY_MAP;
	}

	if ((address == memory_map.flashbase) && (erase->count == total_pages))
	{
		erase->page = 0xFFFF; /* mark the full erase */
	}

	return 0;
}

//sysboot.c
/**
  * @brief  Calculate 8-bit checksum.
  * @param  source data and length
  * @retval checksum value.
  */
uint8_t sysboot_checksum(uint8_t *src, uint32_t len)
{
	uint8_t csum = *src++;
	//CAM_ERR(CAM_OIS, "mcu");

	if (len)
	{
		while (--len)
		{
			csum ^= *src++;
		}
	}
	else
	{
		csum = 0; /* error (no length param) */
	}

	return csum;
}

//sysboot_i2c.c
//static uint8_t xmit[BOOT_I2C_ERASE_PARAM_LEN(BOOT_I2C_MAX_PAYLOAD_LEN)] = {0, };

/**
  * @brief  Waiting for an host ACK response
  * @param  timeout (msec)
  * @retval 0 is success, others are fail.
  */
static int sysboot_i2c_wait_ack(struct cam_ois_ctrl_t *o_ctrl, unsigned long timeout)
{
	int ret = 0;
	uint32_t retry = 3;
	unsigned char resp = 0;

	while(retry--)
	{
		ret = i2c_master_recv(o_ctrl->io_master_info.client, &resp, 1);
		if(ret >= 0)
		{
			if(resp == BOOT_I2C_RESP_ACK)
			{
				//CAM_ERR(CAM_OIS, "[mcu] wait ack success 0x%x ",resp);
			}else{
				CAM_ERR(CAM_OIS, "[mcu] wait ack failed 0x%x ", resp);
			}
			//return resp;
			return 0;
		}
		else
		{
			CAM_ERR(CAM_OIS, "[mcu] failed resp is 0x%x ,ret is %d	", resp, ret);
			if (time_after(jiffies, timeout))
			{
				ret = -ETIMEDOUT;
				break;
			}
			usleep_range(BOOT_I2C_INTER_PKT_BACK_INTVL * 1000, BOOT_I2C_INTER_PKT_BACK_INTVL * 1000 + 1000);
		}
	}
	return -1;

}

#if 0
/**
  * @brief  Transmit the raw packet datas.
  * @param  source, length, timeout (msec)
  * @retval 0 is success, others are fail.
  */
static int sysboot_i2c_send(struct cam_ois_ctrl_t *o_ctrl, uint8_t *cmd, uint32_t len, unsigned long timeout)
{
    int ret = 0;
    int retry = 0;
    int i = 0;

    for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
    {
        /* transmit command */
        ret = i2c_master_send(o_ctrl->io_master_info.client, cmd, len);
        if (ret < 0)
        {

            if (time_after(jiffies,timeout))
            {
                ret = -ETIMEDOUT;
                break;
            }
            msleep(BOOT_I2C_SYNC_RETRY_INTVL);
            CAM_ERR(CAM_OIS, "[mcu] send data fail ");
            continue;
        }
    }
    CAM_ERR(CAM_OIS, "client->addr=0x%x success send: %d Byte", o_ctrl->io_master_info.client->addr, ret);
    for(i = 0; i < ret; i++)
    {
        CAM_ERR(CAM_OIS, "[mcu] send data : 0x%x ", cmd[i]);
    }
    return ret;
}

/**
  * @brief  Receive the raw packet datas.
  * @param  destination, length, timeout (msec)
  * @retval 0 is success, others are fail.
  */

static int sysboot_i2c_recv(struct cam_ois_ctrl_t *o_ctrl, uint8_t *recv, uint32_t len, unsigned long timeout)
{
    int ret = 0;
    int retry = 0;
    int i = 0;

    for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
    {
        /* transmit command */
        ret = i2c_master_recv(o_ctrl->io_master_info.client, recv, len);

        if (ret < 0)
        {
            if (time_after(jiffies,timeout))
            {
                ret = -ETIMEDOUT;
                break;
            }
            msleep(BOOT_I2C_SYNC_RETRY_INTVL);
            CAM_ERR(CAM_OIS, "[mcu] recv data fail ");
            continue;

        }
    }
    for(i = 0; i < ret; i++)
    {
        CAM_ERR(CAM_OIS, "[mcu] recv data : 0x%x ", recv[i]);
    }

    return ret;

}
#endif

/**
  * @brief  Get device PID or Get device BL version
  * @param  None
  * @retval 0 is success, others are fail.
  */
static int sysboot_i2c_get_info(struct cam_ois_ctrl_t *o_ctrl,
	uint8_t *cmd, uint32_t cmd_size, uint32_t data_size)
{
	uint8_t recv[BOOT_I2C_RESP_GET_ID_LEN] = {0, };
	int ret = 0;
	int retry = 0;

	CAM_INFO(CAM_OIS, "mcu 0x%x 0x%x", cmd[0], cmd[1]);
	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
	{
		/* transmit command */
		ret = i2c_master_send(o_ctrl->io_master_info.client, cmd, cmd_size);
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "mcu send data fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "mcu wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* receive payload */
		ret = i2c_master_recv(o_ctrl->io_master_info.client, recv, data_size);
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "mcu receive payload fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "mcu wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		if(cmd[0] == BOOT_I2C_CMD_GET_ID){
			memcpy((void *)&(o_ctrl->info.id), &recv[1], recv[0] + 1);
			o_ctrl->info.id = NTOHS(o_ctrl->info.id);
			CAM_INFO(CAM_OIS, "success get info id %d", o_ctrl->info.id);
		}else if(cmd[0] == BOOT_I2C_CMD_GET_VER){
			memcpy((void *)&(o_ctrl->info.ver), recv , 1);
			CAM_INFO(CAM_OIS, "success get info version %d", o_ctrl->info.ver);
		}

		return 0;
	}

	return ret + cmd[0];
}

/**
  * @brief  SYNC transaction
  * @param  None
  * @retval 0 is success, others are fail.
  */
int sysboot_i2c_sync(struct cam_ois_ctrl_t *o_ctrl, uint8_t *cmd)
{
	int ret = 0;

	CAM_INFO(CAM_OIS, "mcu");
	/* set it and wait for it to be so */
	ret = i2c_master_send(o_ctrl->io_master_info.client, cmd, 1);
	CAM_INFO(CAM_OIS,"i2c client addr 0x%x ", o_ctrl->io_master_info.client->addr);
	if(ret >= 0){
		CAM_ERR(CAM_OIS,"success connect to target mcu ");
	}else{
		CAM_ERR(CAM_OIS,"failed connect to target mcu ");
	}
	return ret;
}

/**
  * @brief  Get device info.(PID, BL ver, etc,.)
  * @param  None
  * @retval 0 is success, others are fail.
  */
int sysboot_i2c_info(struct cam_ois_ctrl_t *o_ctrl)
{
	int ret = 0;
	CAM_INFO(CAM_OIS, "mcu");
	memset((void *)&(o_ctrl->info), 0x00, sizeof(o_ctrl->info));
	ois_mcu_chip_command(o_ctrl, BOOT_I2C_CMD_GET_ID);
	ois_mcu_chip_command(o_ctrl, BOOT_I2C_CMD_GET_VER);
	return ret;
}

/**
  * @brief  Read the device memory
  * @param  source(address), destination, length
  * @retval 0 is success, others are fail.
  */
int sysboot_i2c_read(struct cam_ois_ctrl_t *o_ctrl, uint32_t address, uint8_t *dst, size_t len)
{
	uint8_t cmd[BOOT_I2C_REQ_CMD_LEN] = {0, }; //BOOT_I2C_REQ_CMD_LEN = 2
	uint8_t startaddr[BOOT_I2C_REQ_ADDRESS_LEN] = {0, }; //BOOT_I2C_REQ_ADDRESS_LEN = 5
	uint8_t nbytes[BOOT_I2C_READ_PARAM_LEN] = {0, }; //BOOT_I2C_READ_PARAM_LEN = 2
	int ret = 0;
	int retry = 0;

	/* build command */
	cmd[0] = BOOT_I2C_CMD_READ;
	cmd[1] = ~cmd[0];

	/* build address + checksum */
	*(uint32_t *)startaddr = HTONL(address);
	startaddr[BOOT_I2C_ADDRESS_LEN] = sysboot_checksum(startaddr, BOOT_I2C_ADDRESS_LEN);

	/* build number of bytes + checksum */
	nbytes[0] = len - 1;
	nbytes[1] = ~nbytes[0];
	CAM_INFO(CAM_OIS, "read address 0x%x",address);

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
	{
		/* transmit command */
		ret = i2c_master_send(o_ctrl->io_master_info.client, cmd, sizeof(cmd));
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* transmit address */
		ret = i2c_master_send(o_ctrl->io_master_info.client, startaddr, sizeof(startaddr));
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* transmit number of bytes */
		ret = i2c_master_send(o_ctrl->io_master_info.client, nbytes, sizeof(nbytes));
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* receive payload */
		ret = i2c_master_recv(o_ctrl->io_master_info.client, dst, len);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		return 0;
	}

	return ret + BOOT_ERR_API_READ;
}

/**
  * @brief  Write the contents to the device memory
  * @param  destination(address), source, length
  * @retval 0 is success, others are fail.
  */
int sysboot_i2c_write(struct cam_ois_ctrl_t *o_ctrl, uint32_t address, uint8_t *src, size_t len)
{
	uint8_t cmd[BOOT_I2C_REQ_CMD_LEN] = {0, };
	uint8_t startaddr[BOOT_I2C_REQ_ADDRESS_LEN] = {0, };
	int ret = 0;
	int retry = 0;
	char * buf = NULL;
	/* build command */
	cmd[0] = BOOT_I2C_CMD_WRITE;
	cmd[1] = ~cmd[0];

	/* build address + checksum */
	*(uint32_t *)startaddr = HTONL(address);
	startaddr[BOOT_I2C_ADDRESS_LEN] = sysboot_checksum(startaddr, BOOT_I2C_ADDRESS_LEN);

	/* build number of bytes + checksum */
	CAM_INFO(CAM_OIS, "mcu address = 0x%x", address);

	buf = kzalloc(len + 2, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	buf[0] = len -1;
	memcpy(&buf[1], src, len);
	buf[len+1] = sysboot_checksum(buf, len + 1);

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
	{
		/* transmit command */
		ret = i2c_master_send(o_ctrl->io_master_info.client, cmd, 2);
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "[mcu] txdata fail ");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "[mcu]mcu_wait_ack fail after txdata ");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}


		/* transmit address */
		ret = i2c_master_send(o_ctrl->io_master_info.client, startaddr, 5);
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "[mcu] txdata fail ");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "[mcu]mcu_wait_ack fail after txdata ");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* transmit number of bytes + datas */

		ret = i2c_master_send(o_ctrl->io_master_info.client, buf, BOOT_I2C_WRITE_PARAM_LEN(len));
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "[mcu] txdata fail ");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		//msleep(len);
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WRITE_TMOUT);
		if (ret < 0)
		{
			CAM_ERR(CAM_OIS, "[mcu]mcu_wait_ack fail after txdata ");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		kfree(buf);

		return 0;
	}
	msleep(2);
	kfree(buf);

	return ret + BOOT_ERR_API_WRITE;
}

/**
  * @brief  Erase the device memory
  * @param  destination(address), length
  * @retval 0 is success, others are fail.
  */
int sysboot_i2c_erase(struct cam_ois_ctrl_t *o_ctrl, uint32_t address, size_t len)
{
	uint8_t cmd[BOOT_I2C_REQ_CMD_LEN] = {0, };
	sysboot_erase_param_type erase;
	uint8_t xmit_bytes = 0;
	int ret = 0;
	int retry = 0;
	uint8_t *xmit = NULL;

	/* build command */
	cmd[0] = BOOT_I2C_CMD_ERASE;
	cmd[1] = ~cmd[0];

	/* build erase parameter */
	ret = sysboot_conv_memory_map(o_ctrl, address, len, &erase);
	if (ret < 0)
	{
		return ret + BOOT_ERR_API_ERASE;
	}
	CAM_INFO(CAM_OIS, "erase.page 0x%x", erase.page);

	xmit = kmalloc(1024, GFP_KERNEL | GFP_DMA);
	if (xmit == NULL) {
		CAM_ERR(CAM_OIS, "out of memory");
		return ret + BOOT_ERR_API_ERASE;
	}

	memset(xmit, 0, 1024);

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
	{
		/* build full erase command */
		if (erase.page == 0xFFFF)
		{
			*(uint16_t *)xmit = (uint16_t)erase.page;
		}
		/* build page erase command */
		else
		{
			*(uint16_t *)xmit = HTONS((erase.count - 1));
		}
		xmit_bytes = sizeof(uint16_t);
		xmit[xmit_bytes] = sysboot_checksum(xmit, xmit_bytes);
		xmit_bytes++;
		/* transmit command */
		ret = i2c_master_send(o_ctrl->io_master_info.client, cmd, sizeof(cmd));
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* transmit parameter */
		ret = i2c_master_send(o_ctrl->io_master_info.client, xmit, xmit_bytes);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		//msleep(2*32);
		ret = sysboot_i2c_wait_ack(o_ctrl, (erase.page == 0xFFFF) ? BOOT_I2C_FULL_ERASE_TMOUT : BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* case of page erase */
		if (erase.page != 0xFFFF)
		{
			/* build page erase parameter */
			register int ix;
			register uint16_t *pbuf = (uint16_t *)xmit;
			for (ix = 0; ix < erase.count; ++ix)
			{
				pbuf[ix] = HTONS((erase.page + ix));
			}
			CAM_INFO(CAM_OIS, "erase.count %d", erase.count);
			CAM_INFO(CAM_OIS, "&pbuf[ix] %pK,xmit %pK", &pbuf[ix], xmit);
			xmit_bytes = 2 * erase.count;
			*((uint8_t *)&pbuf[ix]) = sysboot_checksum(xmit, xmit_bytes);
			CAM_INFO(CAM_OIS, "xmit_bytes %d", xmit_bytes);
			xmit_bytes++;
			/* transmit parameter */
			ret = i2c_master_send(o_ctrl->io_master_info.client, xmit, xmit_bytes);
			if (ret < 0)
			{
				msleep(BOOT_I2C_SYNC_RETRY_INTVL);
				continue;
			}
			//msleep(2*32);
			/* wait for ACK response */
			ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_PAGE_ERASE_TMOUT(erase.count + 1));
			if (ret < 0)
			{
				msleep(BOOT_I2C_SYNC_RETRY_INTVL);
				continue;
			}
		}
		CAM_INFO(CAM_OIS, "erase finish");
		kfree(xmit);
		return 0;
	}

	if (xmit)
		kfree(xmit);
	return ret + BOOT_ERR_API_ERASE;
}

/**
  * @brief  Go to specific address of the device (for starting application)
  * @param  branch destination(address)
  * @retval 0 is success, others are fail.
  */
int sysboot_i2c_go(struct cam_ois_ctrl_t *o_ctrl, uint32_t address)
{
	uint8_t cmd[BOOT_I2C_REQ_CMD_LEN] = {0, };
	uint8_t startaddr[BOOT_I2C_REQ_ADDRESS_LEN] = {0, };
	int ret = 0;
	int retry = 0;

	/* build command */
	cmd[0] = BOOT_I2C_CMD_GO;
	cmd[1] = ~cmd[0];

	/* build address + checksum */
	*(uint32_t *)startaddr = HTONL(address);
	startaddr[BOOT_I2C_ADDRESS_LEN] = sysboot_checksum(startaddr, BOOT_I2C_ADDRESS_LEN);
	CAM_INFO(CAM_OIS, "mcu");

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
	{
		/* transmit command */
		ret = i2c_master_send(o_ctrl->io_master_info.client, cmd, sizeof(cmd));
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* transmit address */
		ret = i2c_master_send(o_ctrl->io_master_info.client, startaddr, sizeof(startaddr));
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_WAIT_RESP_TMOUT + 200); /* 200??? */
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		return 0;
	}

	return ret + BOOT_ERR_API_GO;
}

/**
  * @brief  Unprotect the write protect
  * @param  None
  * @retval 0 is success, others are fail.
  */
int sysboot_i2c_write_unprotect(struct cam_ois_ctrl_t *o_ctrl)
{
	uint8_t cmd[BOOT_I2C_REQ_CMD_LEN] = {0, };
	int ret = 0;
	int retry = 0;

	/* build command */
	cmd[0] = BOOT_I2C_CMD_WRITE_UNPROTECT;
	cmd[1] = ~cmd[0];
	CAM_INFO(CAM_OIS, "mcu");

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
	{
		/* transmit command */
		ret = i2c_master_send(o_ctrl->io_master_info.client, cmd, sizeof(cmd));
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		return 0;
	}

	return ret + BOOT_ERR_API_WRITE_UNPROTECT;
}

/**
  * @brief  Unprotect the read protect
  * @param  None
  * @retval 0 is success, others are fail.
  */
int sysboot_i2c_read_unprotect(struct cam_ois_ctrl_t *o_ctrl)
{
	uint8_t cmd[BOOT_I2C_REQ_CMD_LEN] = {0, };
	int ret = 0;
	int retry = 0;

	/* build command */
	cmd[0] = BOOT_I2C_CMD_READ_UNPROTECT;
	cmd[1] = ~cmd[0];
	CAM_INFO(CAM_OIS, "mcu");

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry)
	{
		/* transmit command */
		ret = i2c_master_send(o_ctrl->io_master_info.client, cmd, sizeof(cmd));
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = sysboot_i2c_wait_ack(o_ctrl, BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0)
		{
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		return 0;
	}

	return ret + BOOT_ERR_API_READ_UNPROTECT;
}

/* ---------------------------------------------------------------------- */

static int ois_mcu_chip_command(struct cam_ois_ctrl_t *o_ctrl, int command)
{
	/* build command */
	uint8_t cmd[BOOT_I2C_REQ_CMD_LEN] = {0, };
	int ret = 0;
	CAM_INFO(CAM_OIS, "[mcu] start");

	/* execute the command */
	switch(command)
	{
	case BOOT_I2C_CMD_GET:
		cmd[0] = 0x00;
		break;

	case BOOT_I2C_CMD_GET_VER:
		cmd[0] = 0x01;
		cmd[1] = ~cmd[0];
		ret = sysboot_i2c_get_info(o_ctrl, cmd, 2, 1);
		break;

	case BOOT_I2C_CMD_GET_ID:
		cmd[0] = 0x02;
		cmd[1] = ~cmd[0];
		ret = sysboot_i2c_get_info(o_ctrl, cmd, 2, 3);
		break;

	case BOOT_I2C_CMD_READ:
		cmd[0] = 0x11;
		break;

	case BOOT_I2C_CMD_WRITE:
		cmd[0] = 0x31;
		break;

	case BOOT_I2C_CMD_ERASE:
		cmd[0] = 0x44;
		break;

	case BOOT_I2C_CMD_GO:
		cmd[0] = 0x21;
		break;

	case BOOT_I2C_CMD_WRITE_UNPROTECT:
		cmd[0] = 0x73;
		break;

	case BOOT_I2C_CMD_READ_UNPROTECT:
		cmd[0] = 0x92;
		break;

	case BOOT_I2C_CMD_SYNC:
		/* UNKNOWN command */
		cmd[0] = 0xFF;
		sysboot_i2c_sync(o_ctrl, cmd);
		break;

	default:
		break;
		return -EINVAL;
	}

	return ret ;
}


/**
  * @brief  Validation check for TARGET
  * @param  None
  * @retval 0: success, others are fail
  */
int target_validation(struct cam_ois_ctrl_t *o_ctrl)
{
	int ret = 0;
	CAM_DBG(CAM_OIS, "Start target validation");
	/* Connection ------------------------------------------------------------- */
	ret = sysboot_connect(o_ctrl);
	if (ret < 0)
	{
		CAM_INFO(CAM_OIS, "Error: Cannot connect to the target (%d) but skip", ret);
		goto validation_fail;
	}
	CAM_DBG(CAM_OIS, "1. Connection OK");

	ret = sysboot_i2c_info(o_ctrl);
	if (ret < 0)
	{
		CAM_DBG(CAM_OIS, "Error: Failed to collect the target info (%d)", ret);
		goto validation_fail;
	}

	CAM_DBG(CAM_OIS, " 2. Get target info OK Target PID: 0x%X, Bootloader version: 0x%X", o_ctrl->info.id, o_ctrl->info.ver);

	return 0;

validation_fail:
	sysboot_disconnect(o_ctrl);
	CAM_ERR(CAM_OIS, " Failed: target disconnected");

	return -1;
}

/**
  * @brief  Getting STATUS of the TARGET empty check
  * @param  None
  * @retval 0: empty check reset, 1: empty check set, others are fail
  */
int target_empty_check_status(struct cam_ois_ctrl_t *o_ctrl)
{
	uint32_t value = 0;
	int ret = 0;
	CAM_INFO(CAM_OIS, "mcu");

	/* Read first flash memory word ------------------------------------------- */
	ret = sysboot_i2c_read(o_ctrl, memory_map.flashbase, (uint8_t *)&value, sizeof(value));

	if (ret < 0)
	{
		CAM_ERR(CAM_OIS, "[INF] Error: Failed to read word for empty check (%d)", ret);
		goto empty_check_status_fail;
	}

	CAM_DBG(CAM_OIS, "[INF] Flash Word: 0x%08X", value);

	if (value == 0xFFFFFFFF)
	{
		return 1;
	}

	return 0;

empty_check_status_fail:

	return -1;
}

int target_option_update(struct cam_ois_ctrl_t *o_ctrl){
	int ret = 0;
	uint32_t optionbyte = 0;
	int retry = 3;
	CAM_INFO(CAM_OIS, "[mao]read option byte begin ");

	for(retry = 0; retry < 3; retry ++ ){
		ret = sysboot_i2c_read(o_ctrl,memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
		if((ret < 0) || ((optionbyte & 0xff) != 0xaa)){
			ret = sysboot_i2c_read_unprotect(o_ctrl);
			if(ret < 0){
				CAM_ERR(CAM_OIS, "[mao]ois_mcu_read_unprotect failed ");
			}else{
				CAM_INFO(CAM_OIS, "[mao]ois_mcu_read_unprotect ok ");
			}
			msleep(60);
			ret = sysboot_connect(o_ctrl);
			//try connection again
			continue;
		}

		if (optionbyte & (1 << 24)) {
		/* Option byte write ---------------------------------------------------- */
			optionbyte &= ~(1 << 24);
			ret = sysboot_i2c_write(o_ctrl,memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
			if(ret < 0){
				msleep(1);
				continue;
			}
			CAM_INFO(CAM_OIS, "[mao]write option byte ok ");
			//try connection again
		}else{
			CAM_INFO(CAM_OIS, "[mao]option byte is 0, return success ");
			return 0;
		}
	}

	return ret;
}

int target_read_hwver(struct cam_ois_ctrl_t *o_ctrl){
	int ret = 0;
	int i = 0;

	uint32_t addr[4] = {0, };
	uint8_t dst = 0;
	uint32_t address = 0;

	for(i = 0; i<4 ; i++){
		addr[i] = 0x80F8 + i + memory_map.flashbase;
		address = addr[i];
		ret = sysboot_i2c_read(o_ctrl,address, &dst, 1);

		if(ret < 0){
			CAM_ERR(CAM_OIS,"read fwver addr 0x%x fail", address);
		}else{
			CAM_ERR(CAM_OIS,"read fwver addr 0x%x dst 0x%x", address, dst);
		}
	}
	return ret ;
}

int target_read_vdrinfo(struct cam_ois_ctrl_t *o_ctrl){
	int ret = 0;
	int i = 0;
	uint32_t addr[4] = {0, };
	unsigned char dst[5] = "";
	uint32_t address = 0;
	uint8_t *data = NULL ;

	for(i = 0; i<4 ; i++){
		addr[i] = 0x807C+i+memory_map.flashbase;
		address = addr[i];
		ret = sysboot_i2c_read(o_ctrl, address, dst, 4);

		if(ret < 0){
			CAM_ERR(CAM_OIS,"read fwver addr 0x%x fail", address);
		}else{
			CAM_ERR(CAM_OIS,"read fwver addr 0x%x dst [0] 0x%x,[1] 0x%x,[2] 0x%x,[3] 0x%x,",
				address, dst[0], dst[1], dst[2], dst[3]);
		}
	}
	address = memory_map.flashbase + 0x8000;

	data = kmalloc(256, GFP_KERNEL | GFP_DMA);
	if (data != NULL) {
		memset(data, 0, 256);

		ret = sysboot_i2c_read(o_ctrl, address, data, 256);
		//strncpy(dst,data+0x7c,4);
		strncpy(dst,data + 124, 4);
		CAM_INFO(CAM_OIS,"read fwver addr 0x%x dst [0] 0x%x,[1] 0x%x,[2] 0x%x,[3] 0x%x,",
			address + 0x7C, dst[0], dst[1], dst[2], dst[3]);

		if (data)
			kfree(data);
	} else {
		CAM_ERR(CAM_OIS,"out of memory");
	}
	return ret ;
}

int target_empty_check_clear(struct cam_ois_ctrl_t * o_ctrl)
{
	int ret = 0;
	uint32_t optionbyte = 0;

	/* Option Byte read ------------------------------------------------------- */
	ret = sysboot_i2c_read(o_ctrl, memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
	if (ret < 0) {
		CAM_ERR(CAM_OIS,"Option Byte read fail");
		goto empty_check_clear_fail;
	}

	CAM_INFO(CAM_OIS,"Option Byte read 0x%x ", optionbyte);

	/* Option byte write (dummy: readed value) -------------------------------- */
	ret = sysboot_i2c_write(o_ctrl, memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
	if (ret < 0) {
		CAM_ERR(CAM_OIS,"Option Byte write fail");
		goto empty_check_clear_fail;
	}
	CAM_INFO(CAM_OIS,"Option Byte write 0x%x ", optionbyte);

	/* Put little delay for Target program option byte and self-reset */
	msleep(150);
	/* Option byte read for checking protection status ------------------------ */
	/* 1> Re-connect to the target */
	ret = sysboot_connect(o_ctrl);
	if (ret) {
		CAM_ERR(CAM_OIS,"Cannot connect to the target for RDP check (%d)",ret);
		goto empty_check_clear_fail;
	}

	/* 2> Read from target for status checking and recover it if needed */
	ret = sysboot_i2c_read(o_ctrl, memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
	if ((ret < 0) || ((optionbyte & 0x000000FF) != 0xAA)) {
		CAM_ERR(CAM_OIS,"Failed to read option byte from target (%d)",ret);
		/* Tryout the RDP level to 0 */
		ret = sysboot_i2c_read_unprotect(o_ctrl);
		if (ret) {
			CAM_INFO(CAM_OIS,"Readout unprotect Not OK ... Host restart and try again");
		} else {
			CAM_INFO(CAM_OIS,"Readout unprotect OK ... Host restart and try again");
		}
		/* Put little delay for Target erase all of pages */
		msleep(50);
		goto empty_check_clear_fail;
	}

	return 0;
empty_check_clear_fail:
	return -1;
}

#if 0
int target_normal_on(struct cam_ois_ctrl_t * o_ctrl)
{
	int ret = 0;
	/* Release NRST reset */
	gpio_direction_output(o_ctrl->reset_ctrl_gpio, 1);
	/* Put little delay for the target prepared */
	usleep_range(1000, 1100);
	gpio_direction_output(o_ctrl->boot0_ctrl_gpio, 0);
	usleep_range(1000, 1100);
	return ret;
}
#endif

// ois
int cam_ois_i2c_read(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t addr, uint32_t *data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type)
{
	int rc = 0;
	uint32_t temp;

	rc = camera_io_dev_read(&o_ctrl->io_master_info,
		addr, &temp,
		addr_type, data_type);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois i2c byte read failed addr : 0x%x data : 0x%x", addr, *data);
		return rc;
	}
	*data = temp;

	CAM_DBG(CAM_OIS, "addr = 0x%x data: 0x%x", addr, *data);
	return rc;
}

int cam_ois_i2c_write(struct cam_ois_ctrl_t *o_ctrl,
		uint32_t addr, uint32_t data,
		enum camera_sensor_i2c_type addr_type,
		enum camera_sensor_i2c_type data_type)
{
	int rc = 0;
    struct cam_sensor_i2c_reg_setting write_setting;

    write_setting.reg_setting = kmalloc(sizeof(struct cam_sensor_i2c_reg_array), GFP_KERNEL);
	if (!write_setting.reg_setting) {
		return -ENOMEM;
	}
	memset(write_setting.reg_setting, 0, sizeof(struct cam_sensor_i2c_reg_array));

    write_setting.addr_type = addr_type;
    write_setting.data_type = data_type;
    write_setting.delay = 0;

    write_setting.size = 1;
    write_setting.reg_setting[0].reg_addr = addr;
    write_setting.reg_setting[0].reg_data = data;
    write_setting.reg_setting[0].delay = 0;

	rc = camera_io_dev_write(&o_ctrl->io_master_info, &write_setting);

	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois i2c byte write failed addr : 0x%x data : 0x%x", addr, data);
		goto free_reg_setting;
	}

	CAM_DBG(CAM_OIS, "addr = 0x%x data: 0x%x", addr, data);

free_reg_setting:
	if (write_setting.reg_setting)
		kfree(write_setting.reg_setting);
	return rc;
}

int cam_ois_i2c_write_continous(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t addr, uint8_t *data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type, int data_size)
{
	int i = 0, rc = 0;
	struct cam_sensor_i2c_reg_setting write_settings;

	write_settings.reg_setting =
		(struct cam_sensor_i2c_reg_array *)
		kmalloc(sizeof(struct cam_sensor_i2c_reg_array) * data_size,
			GFP_KERNEL);
	if (!write_settings.reg_setting) {
		return -ENOMEM;
	}
	memset(write_settings.reg_setting, 0,
		sizeof(struct cam_sensor_i2c_reg_array) * data_size);

	write_settings.addr_type = addr_type;
	write_settings.data_type = data_type;
	write_settings.size = data_size;
	write_settings.delay = 0;


	for (i = 0; i < data_size; i++)
	{
		write_settings.reg_setting[i].reg_addr = addr;
		write_settings.reg_setting[i].reg_data = data[i];
		write_settings.reg_setting[i].delay = 0;
	}

	rc = camera_io_dev_write_continuous(&o_ctrl->io_master_info,
		&write_settings,	CAM_SENSOR_I2C_WRITE_SEQ);

	if (write_settings.reg_setting)
		kfree(write_settings.reg_setting);

	return rc;
}

int cam_ois_bypass_i2c_read(struct cam_ois_ctrl_t *o_ctrl,
	uint16_t uild, uint16_t uiReg,
	uint8_t ucRegSize, uint8_t* pBuf,
	uint8_t ucSize)
{
	int i = 0;
	uint32_t RcvData = 0;
	int retry = 10;
	int ret = 0;

	// Device ID
	uild = ((uild & 0xFF) << 8) | ((uild >> 8) & 0xFF);
	ret |= cam_ois_i2c_write(o_ctrl, 0x0100, uild,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	// Register Address
	uiReg = ((uiReg & 0xFF) << 8) | ((uiReg >> 8) & 0xFF);
	ret |= cam_ois_i2c_write(o_ctrl, 0x0102, uiReg,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	// Register Address Size
	ret |= cam_ois_i2c_write(o_ctrl, 0x0104, ucRegSize,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	// Data size
	ret |= cam_ois_i2c_write(o_ctrl, 0x0105, ucSize,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	ret |= cam_ois_i2c_write(o_ctrl, 0x0028, 0x2,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	do {
		ret |= cam_ois_i2c_read(o_ctrl, 0x0028, &RcvData,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		usleep_range(1000, 1100);
	} while ((RcvData != 0) && (retry-- > 0));

	// Parsing data into transmit buffer
	for (i = 0; i < ucSize; i++) {
		ret |= cam_ois_i2c_read(o_ctrl, 0x0106 + i, &RcvData,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		CAM_DBG(CAM_OIS, "RcvData[0x%x] %d", 0x0106 + i, RcvData);
		*(pBuf + i) = (RcvData & 0xFF);
	}

	return ret;
}

int cam_ois_bypass_i2c_write(struct cam_ois_ctrl_t *o_ctrl,
	uint16_t uild, uint16_t uiReg,
	uint8_t ucRegSize, uint8_t* pBuf,
	uint8_t ucSize)
{
	uint32_t RcvData = 0;
	int retry = 10;
	int ret = 0;

	// Device ID
	uild = ((uild & 0xFF) << 8) | ((uild >> 8) & 0xFF);
	ret |= cam_ois_i2c_write(o_ctrl, 0x0100, uild,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	// Register Address
	uiReg = ((uiReg & 0xFF) << 8) | ((uiReg >> 8) & 0xFF);
	ret |= cam_ois_i2c_write(o_ctrl, 0x0102, uiReg,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	// Register Address Size
	ret |= cam_ois_i2c_write(o_ctrl, 0x0104, ucRegSize,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	// Data size
	ret |= cam_ois_i2c_write(o_ctrl, 0x0105, ucSize,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	ret |= cam_ois_i2c_write_continous(o_ctrl, 0x0106, pBuf,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, ucSize);

	ret |= cam_ois_i2c_write(o_ctrl, 0x0028, 0x2,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	do {
		ret |= cam_ois_i2c_read(o_ctrl, 0x0028, &RcvData,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		usleep_range(1000, 1100);
	} while ((RcvData != 0) && (retry-- > 0));

	return ret;
}

int cam_ois_check_tele_cross_talk(struct cam_ois_ctrl_t *o_ctrl, uint16_t *result)
{
	uint8_t buf[2];
	uint16_t val;
	int i = 0, ret = 0;

	buf[0] = 0x08;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x0002 , 2, buf, 1);

	buf[0] = 0x01;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x0080 , 2, buf, 1);

	buf[0] = 0x01;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x0000 , 2, buf, 1);


	// X,Y initial position (2 Byte)
	// X axis
	buf[0] = (uint8_t)(800 & 0xFF);
	buf[1] = (uint8_t)((800 >> 8) & 0xFF);
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x0022 , 2, buf, 2);

	// Y axis
	buf[0] = (2048 & 0xFF);
	buf[1] = (2048 >> 8) & 0xFF;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x0024 , 2, buf, 2);

	for (i = 0; i < STEP_COUNT; i++) {
		// Move X axis
		val = (uint16_t)(INIT_X_TARGET + (i * STEP_VALUE));
		buf[0] = (uint8_t)(val & 0xFF);
		buf[1] = (uint8_t)((val >> 8) & 0xFF);
		ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x0022, 2, buf, 2);
		msleep(45);

		// Read Y Hall
		ret |= cam_ois_bypass_i2c_read(o_ctrl, RUMBA_READ_UILD, 0x0090, 2, buf, 2);
		result[i] = (buf[1] << 8)| buf[0];
		CAM_INFO(CAM_OIS, "result[%d] %d", i, result[i]);
	}

	return ret;
}

uint32_t cam_ois_check_ext_clk(struct cam_ois_ctrl_t *o_ctrl)
{
	uint8_t buf[4];
    int ret = 0;
    uint32_t cur_clk = 0;

	ret |= cam_ois_bypass_i2c_read(o_ctrl, RUMBA_READ_UILD, 0x03F0, 2, buf, 4);
	cur_clk = (buf[3] << 24) | (buf[2] << 16) |
					(buf[1] << 8) | buf[0];
	CAM_INFO(CAM_OIS, "cur_clk %u", cur_clk);

	return cur_clk;
}

int32_t cam_ois_set_ext_clk(struct cam_ois_ctrl_t *o_ctrl, uint32_t clk)
{
	uint8_t buf[4];
	uint8_t pll_multi = 0, pll_divide = 0;
	int i = 0, ret = 0;
	uint32_t cur_clk = 0;
	int retry = 100;

	cur_clk = cam_ois_check_ext_clk(o_ctrl);

	if (cur_clk == clk)
		return cur_clk;
	CAM_INFO(CAM_OIS, "cur_clk %u, new_clk %u", cur_clk, clk);

	switch (clk) {
	case CAMERA_OIS_EXT_CLK_12MHZ:
		pll_multi = 0x08;
		pll_divide = 0x03;
		break;
	case CAMERA_OIS_EXT_CLK_17MHZ:
		pll_multi = 0x09;
		pll_divide = 0x05;
		break;
	case CAMERA_OIS_EXT_CLK_19P2MHZ:
		pll_multi = 0x05;
		pll_divide = 0x03;
		break;
	case CAMERA_OIS_EXT_CLK_24MHZ:
		pll_multi = 0x04;
		pll_divide = 0x03;
		break;
	case CAMERA_OIS_EXT_CLK_26MHZ:
		pll_multi = 0x06;
		pll_divide = 0x05;
		break;
	default:
		CAM_INFO(CAM_OIS, "unsupported cur_clk: 0x%08x", clk);
		return -EINVAL;
	}

	// Reg EXTCLK(0x03F0) = 26000000U
	for (i = 0; i < 4; i++)
		buf[i] = (clk >> (i * 8)) & 0xFF;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x03F0 , 2, buf, 4);

	// Reg PLLMULTIPLE(0x03F4)=0x06
	buf[0] = pll_multi;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x03F4 , 2, buf, 1);

	// Reg PLLDIVIDE(0x03F5)=0x05
	buf[0] = pll_divide;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x03F5 , 2, buf, 1);

	// Reg FLSWRTRESULT(0x0027)=0xAA
	buf[0] = 0xAA;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x0027 , 2, buf, 1);

	// Reg OISDATAWRITE(0x0003)=0x01
	buf[0] = 0x01;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x0003 , 2, buf, 1);

	msleep(200);

	// Read Reg FLSWRTRESULT(0x27)
	retry = 100;
	do {
		usleep_range(2000, 2100);
		ret |= cam_ois_bypass_i2c_read(o_ctrl, RUMBA_READ_UILD, 0x0027, 2, buf, 1);
	} while ((buf[0] != 0xAA) && (--retry > 0));
	if ((ret < 0) || (retry <= 0))
		CAM_ERR(CAM_OIS, "Read Reg FLSWRTRESULT fail val %u, retry %d", buf[0], retry);

	// Reg OISDATAWRITE(0x000D)=0x01
	buf[0] = 0x01;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x000D , 2, buf, 1);

	// Read Reg OISSTS
	retry = 100;
	do {
		usleep_range(2000, 2100);
		ret |= cam_ois_bypass_i2c_read(o_ctrl, RUMBA_READ_UILD, 0x0001, 2, buf, 1);
	} while ((buf[0] != 0x09) && (--retry > 0));
	if ((ret < 0) || (retry <= 0))
		CAM_ERR(CAM_OIS, "Read Reg OISSTS fail val %u, retry %d", buf[0], retry);

	// Reg DFLSCMD(0x000E)=0x06
	buf[0] = 0x06;
	ret |= cam_ois_bypass_i2c_write(o_ctrl, RUMBA_WRITE_UILD, 0x000E , 2, buf, 1);
	msleep(50);

	return ret;
}

int cam_ois_wait_idle(struct cam_ois_ctrl_t *o_ctrl, int retries)
{
	uint32_t status = 0;
	int ret = 0;

	/* check ois status if it`s idle or not */
	/* OISSTS register(0x0001) 1Byte read */
	/* 0x01 == IDLE State */
	do {
		ret = cam_ois_i2c_read(o_ctrl, 0x0001, &status,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (status == 0x01)
			break;
		if (--retries < 0) {
			if (ret < 0) {
				CAM_ERR(CAM_OIS, "failed due to i2c fail");
				return -EIO;
			}
			CAM_ERR(CAM_OIS, "ois status is not idle, current status %d", status);
			return -EBUSY;
		}
		usleep_range(10000, 11000);
	} while (status != 0x01);
	return 0;
}

int cam_ois_init(struct cam_ois_ctrl_t *o_ctrl)
{
	uint32_t status = 0;
	uint32_t read_value = 0;
	int rc = 0, retries = 0;
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
	struct cam_hw_param *hw_param = NULL;
	uint32_t *hw_cam_position = NULL;
#endif

	CAM_INFO(CAM_OIS, "E");

#if defined(CONFIG_SEC_C2Q_PROJECT)
	efs_cal_data_read_done	 = FALSE;
#endif

	retries = 20;
	do {
		rc = cam_ois_i2c_read(o_ctrl, 0x0001, &status,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if ((status == 0x01) ||
			(status == 0x13))
			break;
		if (--retries < 0) {
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "failed due to i2c fail %d", rc);
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
				if (rc < 0) {
					msm_is_sec_get_sensor_position(&hw_cam_position);
					if (hw_cam_position != NULL) {
						switch (*hw_cam_position) {
						case CAMERA_0:
							if (!msm_is_sec_get_rear_hw_param(&hw_param)) {
								if (hw_param != NULL) {
									CAM_ERR(CAM_HWB, "[R][OIS] Err\n");
									hw_param->i2c_ois_err_cnt++;
									hw_param->need_update_to_file = true;
								}
							}
							break;

						case CAMERA_1:
						case CAMERA_12:
							if (!msm_is_sec_get_front_hw_param(&hw_param)) {
								if (hw_param != NULL) {
									CAM_ERR(CAM_HWB, "[F][OIS] Err\n");
									hw_param->i2c_ois_err_cnt++;
									hw_param->need_update_to_file = true;
								}
							}
							break;

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
						case CAMERA_11:
							if (!msm_is_sec_get_front3_hw_param(&hw_param)) {
								if (hw_param != NULL) {
									CAM_ERR(CAM_HWB, "[F3][OIS] Err\n");
									hw_param->i2c_ois_err_cnt++;
									hw_param->need_update_to_file = true;
								}
							}
							break;
#else
						case CAMERA_11:
							if (!msm_is_sec_get_front2_hw_param(&hw_param)) {
								if (hw_param != NULL) {
									CAM_ERR(CAM_HWB, "[F2][OIS] Err\n");
									hw_param->i2c_ois_err_cnt++;
									hw_param->need_update_to_file = true;
								}
							}
							break;
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
						case CAMERA_2:
							if (!msm_is_sec_get_rear2_hw_param(&hw_param)) {
								if (hw_param != NULL) {
									CAM_ERR(CAM_HWB, "[R2][OIS] Err\n");
									hw_param->i2c_ois_err_cnt++;
									hw_param->need_update_to_file = true;
								}
							}
							break;
#endif

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
						case CAMERA_3:
							if (!msm_is_sec_get_rear3_hw_param(&hw_param)) {
								if (hw_param != NULL) {
									CAM_ERR(CAM_HWB, "[R3][OIS] Err\n");
									hw_param->i2c_ois_err_cnt++;
									hw_param->need_update_to_file = true;
								}
							}
							break;
#endif

						default:
							CAM_DBG(CAM_HWB, "[NON][OIS][%d] Unsupport\n", *hw_cam_position);
							break;
						}
					}
				}
#endif

				break;
			}
			CAM_ERR(CAM_OIS, "ois status is 0x01 or 0x13, current status %d", status);
			break;
		}
		usleep_range(5000, 5050);
	} while ((status != 0x01) && (status != 0x13));

	rc = cam_ois_mcu_init(o_ctrl);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "OIS MCU init failed %d", rc);

	// OIS Shift Setting
	rc = cam_ois_set_shift(o_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois shift calibration enable failed, i2c fail %d", rc);
		return rc;
	}

	// VDIS Setting
	rc = cam_ois_set_ggfadeup(o_ctrl, 1000);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois set vdis setting ggfadeup failed %d", rc);
		return rc;
	}
	rc = cam_ois_set_ggfadedown(o_ctrl, 1000);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois set vdis setting ggfadedown failed %d", rc);
		return rc;
	}

	// OIS Hall Center Read
	rc = cam_ois_i2c_read(o_ctrl, 0x021A, &read_value,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois read hall X center failed %d", rc);
		return rc;
	}
	o_ctrl->x_center =
		((read_value >> 8) & 0xFF) | ((read_value & 0xFF) << 8);
	CAM_DBG(CAM_OIS, "ois read hall x center %d", o_ctrl->x_center);

	rc = cam_ois_i2c_read(o_ctrl, 0x021C, &read_value,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois read hall Y center failed %d", rc);
		return rc;
	}
	o_ctrl->y_center =
		((read_value >> 8) & 0xFF) | ((read_value & 0xFF) << 8);
	CAM_DBG(CAM_OIS, "ois read hall y center %d", o_ctrl->y_center);

	// Compensation Angle Setting
	rc = cam_ois_set_angle_for_compensation(o_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois set angle for compensation failed %d", rc);
		return rc;
	}

	// Init Setting(Dual OIS Setting)
	mutex_lock(&(o_ctrl->i2c_init_data_mutex));
	rc = cam_ois_apply_settings(o_ctrl, &o_ctrl->i2c_init_data);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "ois set dual ois setting failed %d", rc);

	rc = delete_request(&o_ctrl->i2c_init_data);
	if (rc < 0) {
		CAM_WARN(CAM_OIS,
			"Failed deleting Init data: rc: %d", rc);
		rc = 0;
	}
	mutex_unlock(&(o_ctrl->i2c_init_data_mutex));

	// Read error register
	rc = cam_ois_i2c_read(o_ctrl, 0x0004, &o_ctrl->err_reg,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "get ois error register value failed, i2c fail");
		return rc;
	}

#if defined(CONFIG_SAMSUNG_OIS_TAMODE_CONTROL)
	o_ctrl->ois_tamode_onoff = false;
	cam_ois_add_tamode_msg(o_ctrl);
#endif

	o_ctrl->ois_mode = 0;

	CAM_INFO(CAM_OIS, "X");

	return rc;
}

int cam_ois_get_fw_status(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	uint32_t i = 0;
	uint8_t status_arr[OIS_FW_STATUS_SIZE];
	uint32_t status = 0;

	rc = camera_io_dev_read_seq(&o_ctrl->io_master_info,
		OIS_FW_STATUS_OFFSET, status_arr,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE,
		OIS_FW_STATUS_SIZE);
	if (rc < 0){
		CAM_ERR(CAM_OIS, "i2c read fail");
		CAM_ERR(CAM_OIS, "MCU NACK need update FW again");
		return -2;
	}

	for (i = 0; i < OIS_FW_STATUS_SIZE; i++)
		status |= status_arr[i] << (i * 8);

	// In case previous update failed, (like removing the battery during update)
	// Module itself set the 0x00FC ~ 0x00FF register as error status
	// So if previous fw update failed, 0x00FC ~ 0x00FF register value is '4451'
	if (status == 4451) { //previous fw update failed, 0x00FC ~ 0x00FF register value is 4451
		return -1;
	}

	return 0;
}

int32_t cam_ois_read_phone_ver(struct cam_ois_ctrl_t *o_ctrl)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	char	char_ois_ver[OIS_VER_SIZE + 1] = "";
	char	ois_bin_full_path[256] = "";
	int ret = 0, i = 0;

	loff_t pos;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	sprintf(ois_bin_full_path, "%s/%s", OIS_FW_PATH, OIS_MCU_FW_NAME);
	sprintf(o_ctrl->load_fw_name, ois_bin_full_path); // to use in fw_update

	CAM_INFO(CAM_OIS, "OIS FW : %s", ois_bin_full_path);

	filp = filp_open(ois_bin_full_path, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		CAM_ERR(CAM_OIS, "fail to open %s, %ld", ois_bin_full_path, PTR_ERR(filp));
		set_fs(old_fs);
		return -1;
	}

	pos = OIS_MCU_VERSION_OFFSET;
	ret = vfs_read(filp, char_ois_ver, sizeof(char) * OIS_MCU_VERSION_SIZE, &pos);
	if (ret < 0) {
		CAM_ERR(CAM_OIS, "Fail to read OIS FW.");
		ret = -1;
		goto ERROR;
	}

	pos = OIS_MCU_VDRINFO_OFFSET;
	ret = vfs_read(filp, char_ois_ver + OIS_MCU_VDRINFO_SIZE,
		sizeof(char) * (OIS_VER_SIZE - OIS_MCU_VERSION_SIZE), &pos);
	if (ret < 0) {
		CAM_ERR(CAM_OIS, "Fail to read OIS FW.");
		ret = -1;
		goto ERROR;
	}

	o_ctrl->phone_ver[0] = char_ois_ver[3]; // core version
	o_ctrl->phone_ver[1] = char_ois_ver[2];
	o_ctrl->phone_ver[2] = char_ois_ver[1]; // MCU infor
	o_ctrl->phone_ver[3] = char_ois_ver[0]; // Gyro
	o_ctrl->phone_ver[4] = char_ois_ver[4]; // FW release year
	o_ctrl->phone_ver[5] = char_ois_ver[5]; // FW release month
	o_ctrl->phone_ver[6] = char_ois_ver[6]; // FW release count
	o_ctrl->phone_ver[7] = char_ois_ver[7]; // Dev or Rel

	for (i = 0; i < OIS_VER_SIZE; i++) {
		if (!isalnum(o_ctrl->phone_ver[i])) {
			CAM_ERR(CAM_OIS, "version char (%c) is not alnum type.", o_ctrl->phone_ver[i]);
			ret = -1;
			goto ERROR;
		}
	}

	CAM_INFO(CAM_OIS, "%c%c%c%c%c%c%c%c",
		o_ctrl->phone_ver[0], o_ctrl->phone_ver[1],
		o_ctrl->phone_ver[2], o_ctrl->phone_ver[3],
		o_ctrl->phone_ver[4], o_ctrl->phone_ver[5],
		o_ctrl->phone_ver[6], o_ctrl->phone_ver[7]);

ERROR:
	filp_close(filp, NULL);
	filp = NULL;
	set_fs(old_fs);
	return ret;
}

int32_t cam_ois_read_module_ver(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0, i = 0;
	uint8_t data[OIS_VER_SIZE + 1] = "";

	rc = camera_io_dev_read_seq(&o_ctrl->io_master_info,
		0x00F8, data, CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_BYTE, OIS_MCU_VERSION_SIZE);
	if (rc < 0)
		return -2;

	rc = camera_io_dev_read_seq(&o_ctrl->io_master_info,
		0x007C, data + OIS_MCU_VERSION_SIZE,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE,
		OIS_MCU_VDRINFO_SIZE);
	if (rc < 0)
		return -2;

	o_ctrl->module_ver[0] = data[3]; // core version
	o_ctrl->module_ver[1] = data[2];
	o_ctrl->module_ver[2] = data[1]; // MCU infor
	o_ctrl->module_ver[3] = data[0]; // Gyro
	o_ctrl->module_ver[4] = data[4]; // FW release year
	o_ctrl->module_ver[5] = data[5]; // FW release month
	o_ctrl->module_ver[6] = data[6]; // FW release count
	o_ctrl->module_ver[7] = data[7]; // Dev or Rel

	for (i = 0; i < OIS_VER_SIZE; i++) {
		if(!isalnum(o_ctrl->module_ver[i])) {
			CAM_ERR(CAM_OIS, "module_ver[%d] is not alnum type", i);
			return -1;
		}
	}

	CAM_INFO(CAM_OIS, "%c%c%c%c%c%c%c%c",
		o_ctrl->module_ver[0], o_ctrl->module_ver[1],
		o_ctrl->module_ver[2], o_ctrl->module_ver[3],
		o_ctrl->module_ver[4], o_ctrl->module_ver[5],
		o_ctrl->module_ver[6], o_ctrl->module_ver[7]);

	return 0;
}

int32_t cam_ois_read_manual_cal_info(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	uint8_t user_data[OIS_VER_SIZE+1] = {0, };
	uint8_t version_data[20] = { 0x21, 0x43, 0x65, 0x87, 0x23, 0x01, 0xEF, 0xCD, 0x00, 0x74,
							0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
	uint32_t val = 0;

	rc = cam_ois_i2c_write_continous(o_ctrl, 0x0100, version_data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(version_data));
	if (rc < 0)
		CAM_ERR(CAM_OIS, "ois i2c read word failed addr : 0x%x", 0x0100);
	usleep_range(5000, 6000);

	rc |= cam_ois_i2c_read(o_ctrl, 0x0118, &val,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //Core version
	user_data[0] = (uint8_t)(val & 0x00FF);

	rc |= cam_ois_i2c_read(o_ctrl, 0x0119, &val,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //Gyro Sensor
	user_data[1] = (uint8_t)(val & 0x00FF);

	rc |= cam_ois_i2c_read(o_ctrl, 0x011A, &val,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //Driver IC
	user_data[2] = (uint8_t)(val & 0x00FF);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "ois i2c read word failed addr : 0x%x", 0x0100);

	memcpy(o_ctrl->cal_ver, user_data, (OIS_VER_SIZE) * sizeof(uint8_t));
	o_ctrl->cal_ver[OIS_VER_SIZE] = '\0';

	CAM_INFO(CAM_OIS, "Core version = 0x%02x, Gyro sensor = 0x%02x, Driver IC = 0x%02x",
		o_ctrl->cal_ver[0], o_ctrl->cal_ver[1], o_ctrl->cal_ver[2]);

	return 0;
}

int cam_ois_set_shift(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;

	CAM_DBG(CAM_OIS, "Enter");
	CAM_INFO(CAM_OIS, "SET :: SHIFT_CALIBRATION");

	if (cam_ois_wait_idle(o_ctrl, 10) < 0) {
		CAM_ERR(CAM_OIS, "wait ois idle status failed");
		goto ERROR;
	}

	// init af position
	rc = cam_ois_i2c_write(o_ctrl, 0x003A, 0x80,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	rc = cam_ois_i2c_write(o_ctrl, 0x003B, 0x80,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois write init af position , i2c fail");
		goto ERROR;
	}

	//enable shift control
	rc = cam_ois_i2c_write(o_ctrl, 0x0039, 0x01,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);	 // OIS shift calibration enable
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois shift calibration enable failed, i2c fail");
		goto ERROR;
	}

ERROR:
	CAM_DBG(CAM_OIS, "Exit");
	return rc;
}

int cam_ois_set_angle_for_compensation(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	uint8_t data[4] = { 0x06, 0x81, 0x55, 0x3F };

	CAM_INFO(CAM_OIS, "Enter");

	/* angle compensation 1.5->1.25
	   before addr:0x0000, data:0x01
	   write 0x3F558106
	   write 0x3F558106
	*/
	rc = cam_ois_i2c_write_continous(o_ctrl, 0x0348, data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(data));
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "i2c failed");
	}

	rc = cam_ois_i2c_write_continous(o_ctrl, 0x03D8, data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(data));
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "i2c failed");
	}

	return rc;
}
int cam_ois_set_ggfadeup(struct cam_ois_ctrl_t *o_ctrl, uint16_t value)
{
	int rc = 0;
	uint8_t data[2] = { 0, };

	CAM_INFO(CAM_OIS, "Enter %d", value);

	data[0] = value & 0xFF;
	data[1] = (value >> 8) & 0xFF;

	rc = cam_ois_i2c_write_continous(o_ctrl, 0x0238, data,
				CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(data));
	if (rc < 0)
		CAM_ERR(CAM_OIS, "ois set ggfadeup failed, i2c fail");

	CAM_INFO(CAM_OIS, "Exit");
	return rc;
}

int cam_ois_set_ggfadedown(struct cam_ois_ctrl_t *o_ctrl, uint16_t value)
{
	int rc = 0;
	uint8_t data[2] = { 0, };

	CAM_INFO(CAM_OIS, "Enter %d", value);

	data[0] = value & 0xFF;
	data[1] = (value >> 8) & 0xFF;

	rc = cam_ois_i2c_write_continous(o_ctrl, 0x023A, data,
				CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(data));
	if (rc < 0)
		CAM_ERR(CAM_OIS, "ois set ggfadedown failed, i2c fail");

	CAM_INFO(CAM_OIS, "Exit");
	return rc;
}

int cam_ois_create_shift_table(struct cam_ois_ctrl_t *o_ctrl, uint8_t *shift_data)
{
	int i = 0, j = 0, k = 0;
	int16_t dataX[9] = {0, }, dataY[9] = {0, };
	uint16_t tempX = 0, tempY = 0;
	uint32_t addr_en[2] = {0x00, 0x01};
	uint32_t addr_x[2] = {0x10, 0x40};
	uint32_t addr_y[2] = {0x22, 0x52};

	if (!o_ctrl || !shift_data)
		goto ERROR;

	CAM_INFO(CAM_OIS, "Enter");

	for (i = 0; i < 2; i++) {
		if (shift_data[addr_en[i]] != 0x11) {
			o_ctrl->shift_tbl[i].ois_shift_used = false;
			continue;
		}
		o_ctrl->shift_tbl[i].ois_shift_used = true;

		for (j = 0; j < 9; j++) {
			// ACT #1 Shift X : 0x0210 ~ 0x0220 (2byte), ACT #2 Shift X : 0x0240 ~ 0x0250 (2byte)
			tempX = (uint16_t)(shift_data[addr_x[i] + (j * 2)] |
				(shift_data[addr_x[i] + (j * 2) + 1] << 8));
			if (tempX > 32767)
				tempX -= 65536;
			dataX[j] = (int16_t)tempX;

			// ACT #1 Shift Y : 0x0222 ~ 0x0232 (2byte), ACT #2 Shift X : 0x0252 ~ 0x0262 (2byte)
			tempY = (uint16_t)(shift_data[addr_y[i] + (j * 2)] |
				(shift_data[addr_y[i] + (j * 2) + 1] << 8));
			if (tempY > 32767)
				tempY -= 65536;
			dataY[j] = (int16_t)tempY;
		}

		for (j = 0; j < 9; j++)
			CAM_INFO(CAM_OIS, "module%d, dataX[%d] = %5d / dataY[%d] = %5d",
				i + 1, j, dataX[j], j, dataY[j]);

		for (j = 0; j < 8; j++) {
			for (k = 0; k < 64; k++) {
				o_ctrl->shift_tbl[i].ois_shift_x[k + (j << 6)] =
					((((int32_t)dataX[j + 1] - dataX[j])  * k) >> 6) + dataX[j];
				o_ctrl->shift_tbl[i].ois_shift_y[k + (j << 6)] =
					((((int32_t)dataY[j + 1] - dataY[j])  * k) >> 6) + dataY[j];
			}
		}
	}

	CAM_DBG(CAM_OIS, "Exit");
	return 0;

ERROR:
	CAM_ERR(CAM_OIS, "create ois shift table fail");
	return -1;
}

int cam_ois_shift_calibration(struct cam_ois_ctrl_t *o_ctrl, uint16_t af_position, uint16_t subdev_id)
{
	//int8_t data[4] = {0, };
	int rc = 0;

	//CAM_DBG(CAM_OIS, "cam_ois_shift_calibration %d, subdev: %d", af_position, subdev_id);

	if (!o_ctrl)
		return -1;

	if (!o_ctrl->is_power_up) {
		CAM_WARN(CAM_OIS, "ois is not power up");
		return 0;
	}
	if (!o_ctrl->is_servo_on) {
		CAM_WARN(CAM_OIS, "ois serve is not on yet");
		return 0;
	}

	if (af_position >= NUM_AF_POSITION) {
		CAM_ERR(CAM_OIS, "af position error %u", af_position);
		return -1;
	}
	CAM_DBG(CAM_OIS, "ois shift af position %X",af_position);

	//ois cal info no shift data, 1byte?
	//send af position both to wide and tele ?
	//assume af position is only 1byte
 	if (subdev_id == 0) {
		CAM_DBG(CAM_OIS, "write for WIDE %d", subdev_id);

		rc = cam_ois_i2c_write(o_ctrl, 0x003A, af_position,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "write module#1 ois shift calibration error");
	}
#if !(defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT))
	else if (subdev_id == 2) {
		CAM_DBG(CAM_OIS, "write for TELE %d", subdev_id);

		rc = cam_ois_i2c_write(o_ctrl, 0x003B, af_position,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "write module#2 ois shift calibration error");
	}
#endif
	return rc;
}

int32_t cam_ois_read_user_data_section(struct cam_ois_ctrl_t *o_ctrl, uint16_t addr, int size, uint8_t *user_data)
{
	uint8_t read_data[0x02FF] = {0, }, shift_data[0xFF] = {0, };
	int rc = 0, i = 0;
	uint32_t read_status = 0;

	/* OIS Servo Off */
	if (cam_ois_i2c_write(o_ctrl, 0x0000, 0,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE) < 0)
		goto ERROR;

	if (cam_ois_wait_idle(o_ctrl, 10) < 0) {
		CAM_ERR(CAM_OIS, "wait ois idle status failed");
		goto ERROR;
	}
#if 0
	/* User Data Area & Address Setting - 1Page */
	rc = cam_ois_i2c_write(o_ctrl, 0x000F, 0x40,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);	// DLFSSIZE_W Register(0x000F) : Size = 4byte * Value
	memset(&reg_setting, 0, sizeof(struct cam_sensor_i2c_reg_array));
	reg_setting.reg_addr = 0x0010;
	reg_setting.reg_data = 0x0000;
	rc |= cam_ois_i2c_write(o_ctrl, &reg_setting,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	rc |= cam_ois_i2c_write(o_ctrl, 0x000E, 0x04,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); // DFLSCMD Register(0x000E) = READ
	if (rc < 0)
		goto ERROR;

	for (i = MAX_RETRY_COUNT; i > 0; i--) {
		if (cam_ois_i2c_read(o_ctrl, 0x000E, &read_status,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE) < 0)
			goto ERROR;
		if (read_status == 0x14) /* Read Complete? */
			break;
		usleep_range(10000, 11000); // give some delay to wait
	}
	if (i < 0) {
		CAM_ERR(CAM_OIS, "DFLSCMD Read command fail");
		goto ERROR;
	}
#endif
	/* OIS Data Header Read */
	rc = camera_io_dev_read_seq(&o_ctrl->io_master_info,
		0x5F60, read_data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, 0x50);
	if (rc < 0)
		goto ERROR;

	/* copy Cal-Version */
	CAM_INFO(CAM_OIS, "userdata cal ver : %c %c %c %c %c %c %c %c",
			read_data[0], read_data[1], read_data[2], read_data[3],
			read_data[4], read_data[5], read_data[6], read_data[7]);
	memcpy(user_data, read_data, size * sizeof(uint8_t));


	/* User Data Area & Address Setting - 2Page */
	rc = cam_ois_i2c_write(o_ctrl, 0x000F, 0x40,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);	// DLFSSIZE_W Register(0x000F) : Size = 4byte * Value
	rc |= cam_ois_i2c_write(o_ctrl, 0x0010, 0x0001,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD); // Data Write Start Address Offset : 0x0000
	rc |= cam_ois_i2c_write(o_ctrl, 0x000E, 0x04,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); // DFLSCMD Register(0x000E) = READ
	if (rc < 0)
		goto ERROR;

	for (i = MAX_RETRY_COUNT; i >= 0; i--) {
		if (cam_ois_i2c_read(o_ctrl, 0x000E, &read_status,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE) < 0)
			goto ERROR;
		if (read_status == 0x14) /* Read Complete? */
			break;
		usleep_range(10000, 11000); // give some delay to wait
	}
	if (i < 0) {
		CAM_ERR(CAM_OIS, "DFLSCMD Read command fail");
		goto ERROR;
	}

	/* OIS Cal Data Read */
	rc = camera_io_dev_read_seq(&o_ctrl->io_master_info,
		0x0100, read_data + 0x0100,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, 0xFF);
	if (rc < 0)
		goto ERROR;

	/* User Data Area & Address Setting - 3Page */
	rc = cam_ois_i2c_write(o_ctrl, 0x000F, 0x40,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);  // DLFSSIZE_W Register(0x000F) : Size = 4byte * Value
	rc |= cam_ois_i2c_write(o_ctrl, 0x0010, 0x0002,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD); // Data Write Start Address Offset : 0x0000
	rc |= cam_ois_i2c_write(o_ctrl, 0x000E, 0x04,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); // DFLSCMD Register(0x000E) = READ
	if (rc < 0)
		goto ERROR;

	for (i = MAX_RETRY_COUNT; i >= 0; i--) {
		if (cam_ois_i2c_read(o_ctrl, 0x000E, &read_status,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE) < 0)
			goto ERROR;
		if (read_status == 0x14) /* Read Complete? */
			break;
		usleep_range(10000, 11000); // give some delay to wait
	}
	if (i < 0) {
		CAM_ERR(CAM_OIS, "DFLSCMD Read command fail");
		goto ERROR;
	}

	/* OIS Shift Info Read */
	/* OIS Shift Calibration Read */
	rc = camera_io_dev_read_seq(&o_ctrl->io_master_info,
		0x0100, shift_data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, 0xFF);
	if (rc < 0)
		goto ERROR;

	memset(&o_ctrl->shift_tbl, 0, sizeof(o_ctrl->shift_tbl));
	cam_ois_create_shift_table(o_ctrl, shift_data);
ERROR:
	return rc;
}

int32_t cam_ois_read_cal_info(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t *chksum_rumba, uint32_t *chksum_line, uint32_t *is_different_crc)
{
	int rc = 0;
	uint8_t user_data[OIS_VER_SIZE + 1] = {0, };

	rc = cam_ois_i2c_read(o_ctrl, 0x007A, chksum_rumba,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD); // OIS Driver IC cal checksum
	if (rc < 0)
		CAM_ERR(CAM_OIS, "ois i2c read word failed addr : 0x%x", 0x7A);

	rc = cam_ois_i2c_read(o_ctrl, 0x021E, chksum_line,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD); // Line cal checksum
	if (rc < 0)
		CAM_ERR(CAM_OIS, "ois i2c read word failed addr : 0x%x", 0x021E);

	rc = cam_ois_i2c_read(o_ctrl, 0x0004, is_different_crc,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "ois i2c read word failed addr : 0x%x", 0x0004);

	CAM_INFO(CAM_OIS, "cal checksum(rumba : %d, line : %d), compare_crc = %d",
		*chksum_rumba, *chksum_line, *is_different_crc);

	if (cam_ois_read_user_data_section(o_ctrl, OIS_USER_DATA_START_ADDR, OIS_VER_SIZE, user_data) < 0) {
		CAM_ERR(CAM_OIS, " failed to read user data");
		return -1;
	}

	memcpy(o_ctrl->cal_ver, user_data, (OIS_VER_SIZE) * sizeof(uint8_t));
	o_ctrl->cal_ver[OIS_VER_SIZE] = '\0';

	CAM_INFO(CAM_OIS, "cal version = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x(%s)",
		o_ctrl->cal_ver[0], o_ctrl->cal_ver[1],
		o_ctrl->cal_ver[2], o_ctrl->cal_ver[3],
		o_ctrl->cal_ver[4], o_ctrl->cal_ver[5],
		o_ctrl->cal_ver[6], o_ctrl->cal_ver[7],
		o_ctrl->cal_ver);

	return 0;
}

uint16_t cam_ois_calcchecksum(unsigned char *data, int size)
{
	int i = 0;
	uint16_t result = 0;

	for (i = 0; i < size; i += 2)
		result = result + (0xFFFF & (((*(data + i + 1)) << 8) | (*(data + i))));

	return result;
}

int32_t cam_ois_fw_update(struct cam_ois_ctrl_t *o_ctrl,
	bool is_force_update)
{
	int ret = 0;
	uint8_t sendData[OIS_FW_UPDATE_PACKET_SIZE] = "";
	uint16_t checkSum = 0;
	uint32_t val = 0;
	struct file *ois_filp = NULL;
	unsigned char *buffer = NULL;
	char	bin_ver[OIS_VER_SIZE + 1] = "";
	char	mod_ver[OIS_VER_SIZE + 1] = "";
	char	ois_bin_full_path[256] = "";

	int i = 0;
	int empty_check_en = 0;
	int fw_size = 0;
	uint32_t address = 0;
	uint32_t wbytes = 0;
	int len = 0;
	uint32_t unit = OIS_FW_UPDATE_PACKET_SIZE;

	mm_segment_t old_fs;
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	CAM_INFO(CAM_OIS, " ENTER");
	sprintf(ois_bin_full_path, "%s/%s", OIS_FW_PATH, OIS_MCU_FW_NAME);
	sprintf(o_ctrl->load_fw_name, ois_bin_full_path);
	/*file open */
	ois_filp = filp_open(o_ctrl->load_fw_name, O_RDONLY, 0);
	if (IS_ERR(ois_filp)) {
		CAM_ERR(CAM_OIS, "[OIS_FW_DBG] fail to open file %s", o_ctrl->load_fw_name);
		ois_filp = NULL;
		ret = -1;
		goto ERROR;
	}else{
	    CAM_ERR(CAM_OIS, "[OIS_FW_DBG] success to open file %s", o_ctrl->load_fw_name);
	}

	fw_size = ois_filp->f_path.dentry->d_inode->i_size;
	CAM_INFO(CAM_OIS, "fw size %d Bytes", fw_size);
	buffer = vmalloc(fw_size);
	if (!buffer) {
		CAM_ERR(CAM_OIS, "vmalloc failed");
		goto ERROR;
	}
	memset(buffer, 0x00, fw_size);
	ois_filp->f_pos = 0;

	ret = vfs_read(ois_filp, (char __user *)buffer, fw_size, &ois_filp->f_pos);
	if (ret != fw_size) {
		CAM_ERR(CAM_OIS, "[OIS_FW_DBG] failed to read file");
		ret = -1;
		goto ERROR;
	}

	/* update a program code */
	cam_ois_i2c_write(o_ctrl, 0x0C, 0xB5,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	msleep(55);

	/* verify checkSum */
	checkSum = cam_ois_calcchecksum(buffer, fw_size);
	CAM_INFO(CAM_OIS, "[OIS_FW_DBG] ois cal checksum = %u", checkSum);

	//enter system bootloader mode
	CAM_ERR(CAM_OIS,"need update MCU FW, enter system bootloader mode");
	o_ctrl->io_master_info.client->addr = 0x51;

	msleep(50);

	ret = target_validation(o_ctrl);
	if(ret < 0){
	    CAM_ERR(CAM_OIS,"mcu connect failed");
	    goto ERROR;
	}
	//check_option_byte
	target_option_update(o_ctrl);
	//check empty status
	empty_check_en = target_empty_check_status(o_ctrl);
	//erase
	sysboot_i2c_erase(o_ctrl,memory_map.flashbase,65536 - 2048);

	address = memory_map.flashbase;
	len = fw_size;
	/* Write UserProgram Data */
	while (len > 0)
	{
	  wbytes = (len > unit) ? unit : len;
	  /* write the unit */
	  CAM_ERR(CAM_OIS, "[OIS_FW_DBG] write wbytes=%d  left len=%d", wbytes, len);
	  for(i = 0; i<wbytes; i++ ){
	      sendData[i] = buffer[i];
	  }
	  ret = sysboot_i2c_write(o_ctrl, address, sendData, wbytes);
	  if (ret < 0)
	  {
            CAM_ERR(CAM_OIS, "[OIS_FW_DBG] i2c byte prog code write failed");
            break; /* fail to write */
	  }
	  address += wbytes;
	  buffer += wbytes;
	  len -= wbytes;
	}
	buffer = buffer - (address - memory_map.flashbase);
	//target_read_hwver
	target_read_hwver(o_ctrl);
	//target_read_vdrinfo
	target_read_vdrinfo(o_ctrl);
	if(empty_check_en > 0){
		if(target_empty_check_clear(o_ctrl)<0) {
			ret = -1;
			goto ERROR;
		}
	}
	//sysboot_disconnect
	sysboot_disconnect(o_ctrl);

	o_ctrl->io_master_info.client->addr = 0xA2;
	/* write checkSum */
	sendData[0] = (checkSum & 0x00FF);
	sendData[1] = (checkSum & 0xFF00) >> 8;
	sendData[2] = 0;
	sendData[3] = 0x80;
	ret = cam_ois_i2c_write_continous(o_ctrl, 0x0008, sendData,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(sendData));

	msleep(190); // RUMBA Self Reset

	cam_ois_i2c_read(o_ctrl, 0x0006, &val,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD); // Error Status read
	if (val == 0x0000)
		CAM_INFO(CAM_OIS, "progCode update success");
	else
		CAM_ERR(CAM_OIS, "progCode update fail");

	/* s/w reset */
	if (cam_ois_i2c_write(o_ctrl, 0x000D, 0x01,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE) < 0)
		CAM_ERR(CAM_OIS, "[OIS_FW_DBG] s/w reset i2c write error : 0x000D");
	if (cam_ois_i2c_write(o_ctrl, 0x000E, 0x06,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE) < 0)
		CAM_ERR(CAM_OIS, "[OIS_FW_DBG] s/w reset i2c write error : 0x000E");

	msleep(50);

#if 0
	/* Param init - Flash to Rumba */
	if (cam_ois_i2c_write(o_ctrl, 0x0036, 0x03,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE) < 0)
		CAM_ERR(CAM_OIS, "[OIS_FW_DBG] param init i2c write error : 0x0036");
	msleep(200);
#endif
	ret = cam_ois_read_module_ver(o_ctrl);
	if(ret < 0){
	    CAM_ERR(CAM_OIS,"cam_ois_read_module_ver failed after update FW, ret %d",ret);
	}

	ret = cam_ois_read_phone_ver(o_ctrl);
	if(ret < 0){
	    CAM_ERR(CAM_OIS,"cam_ois_read_phone_ver failed after update FW, ret %d",ret);
	}

	memcpy(bin_ver, &o_ctrl->phone_ver, OIS_VER_SIZE * sizeof(char));
	memcpy(mod_ver, &o_ctrl->module_ver, OIS_VER_SIZE * sizeof(char));
	bin_ver[OIS_VER_SIZE] = '\0';
	mod_ver[OIS_VER_SIZE] = '\0';

	CAM_INFO(CAM_OIS, "[OIS_FW_DBG] after update version : phone %s, module %s", bin_ver, mod_ver);
	if (strncmp(bin_ver, mod_ver, OIS_VER_SIZE) != 0) { //after update phone bin ver == module ver
		ret = -1;
		CAM_ERR(CAM_OIS, "[OIS_FW_DBG] module ver is not the same with phone ver , update failed");
		goto ERROR;
	}

	CAM_INFO(CAM_OIS, "[OIS_FW_DBG] ois fw update done");

ERROR:
	if (ois_filp) {
	    filp_close(ois_filp, NULL);
	    ois_filp = NULL;
	}
	if (buffer) {
	    vfree(buffer);
	    buffer = NULL;
	}
	set_fs(old_fs);
	return ret;
}

// check ois version to see if it is available for selftest or not
void cam_ois_version(struct cam_ois_ctrl_t *o_ctrl)
{
	int ret = 0;
	uint32_t val_c = 0, val_d = 0;
	uint32_t version = 0;

	ret = cam_ois_i2c_read(o_ctrl, 0xF8, &val_c,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (ret < 0)
		CAM_ERR(CAM_OIS, "i2c read fail");

	ret = cam_ois_i2c_read(o_ctrl, 0xFA, &val_d,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (ret < 0)
		CAM_ERR(CAM_OIS, "i2c read fail");
	version = (val_d << 8) | val_c;

	CAM_INFO(CAM_OIS, "OIS version = 0x%04x , after 11AE version , fw supoort selftest", version);
	CAM_INFO(CAM_OIS, "End");
}

int cam_ois_gyro_sensor_calibration(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y)
{
	int rc = 0, result = 0;
	uint32_t RcvData = 0;
	uint16_t x = 0, y = 0;
	int XGZERO = 0, YGZERO = 0;
	int retries = 30;
	int scale_factor = OIS_GYRO_SCALE_FACTOR_LSM6DSO;

	CAM_ERR(CAM_OIS, "Enter");
	if (!o_ctrl)
		return 0;

	do
	{
		rc = cam_ois_i2c_read(o_ctrl, 0x0001, &RcvData,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* OISSTS Read */
		if (rc < 0)
			CAM_ERR(CAM_OIS, "i2c read fail %d", rc);
		if (--retries < 0){
		    CAM_ERR(CAM_OIS, "OISSTS Read failed  %d", RcvData);
			rc = -1;
			break;
		}
		usleep_range(20000, 21000);
	} while(RcvData != 1);

	/* Gyro Calibration Start */
	/* GCCTRL GSCEN set */
	cam_ois_i2c_write(o_ctrl, 0x0014, 0x01,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* GCCTRL register(0x0014) 1Byte Send */
	if (rc < 0)
		CAM_ERR(CAM_OIS, "i2c write fail %d", rc);
	/* Check Gyro Calibration Sequence End */
	retries = 40;
	do
	{
		rc = cam_ois_i2c_read(o_ctrl, 0x0014, &RcvData,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* GCCTRL Read */
		if (rc < 0)
			CAM_ERR(CAM_OIS, "i2c read fail %d", rc);
		if(--retries < 0){
			CAM_ERR(CAM_OIS, "GCCTRL Read failed %d", RcvData);
			rc = -1;
			break;
		}
		usleep_range(20000, 21000);
	}while(RcvData != 0);

	/* Result check */
	rc = cam_ois_i2c_read(o_ctrl, 0x0004, &RcvData,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* OISERR Read */
	if((rc >= 0) && ((RcvData & 0x23) == 0x0)) /* OISERR register GXZEROERR & GYZEROERR & GCOMERR Bit = 0(No Error)*/
	{
		CAM_INFO(CAM_OIS, "gyro_sensor_calibration ok %d", RcvData);
		result = 1;
	} else {
		CAM_ERR(CAM_OIS, "gyro_sensor_calibration fail, rc %d, RcvData %d", rc, RcvData);
		result = 0;
	}

	cam_ois_i2c_read(o_ctrl, 0x0248, &RcvData,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	x = RcvData;
	cam_ois_i2c_read(o_ctrl, 0x0249, &RcvData,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	XGZERO = (RcvData << 8) | x;
	if (XGZERO > 0x7FFF)
		XGZERO = -((XGZERO ^ 0xFFFF) + 1);
	CAM_DBG(CAM_OIS, "XGZERO 0x%x", XGZERO);

	cam_ois_i2c_read(o_ctrl, 0x024A, &RcvData,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	y = RcvData;
	cam_ois_i2c_read(o_ctrl, 0x024B, &RcvData,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	YGZERO = (RcvData << 8) | y;
	if (YGZERO > 0x7FFF)
		YGZERO = -((YGZERO ^ 0xFFFF) + 1);
	CAM_DBG(CAM_OIS, "YGZERO 0x%x", YGZERO);

	*raw_data_x = XGZERO * 1000 / scale_factor;
	*raw_data_y = YGZERO * 1000 / scale_factor;
	CAM_INFO(CAM_OIS, "result %d, raw_data_x %ld, raw_data_y %ld", result, *raw_data_x, *raw_data_y);

	CAM_ERR(CAM_OIS, "Exit");

	return result;
}


/* get offset from module for line test */
int cam_ois_offset_test(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y, bool is_need_cal)
{
	int i = 0, rc = 0, result = 0;
	uint32_t val = 0;
	uint16_t x = 0, y = 0;
	int x_sum = 0, y_sum = 0, sum = 0;
	int retries = 0, avg_count = 30;
	int scale_factor = OIS_GYRO_SCALE_FACTOR_LSM6DSO;

	CAM_DBG(CAM_OIS, "cam_ois_offset_test E");
	if (!o_ctrl)
		return -1;

	if (cam_ois_wait_idle(o_ctrl, 5) < 0) {
		CAM_ERR(CAM_OIS, "wait ois idle status failed");
		return -1;
	}

	if (is_need_cal) { // with calibration , offset value will be renewed.
		/* Gyro Calibration Start */
		/* GCCTRL GSCEN set */
		cam_ois_i2c_write(o_ctrl, 0x14, 0x01,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* GCCTRL register(0x0014) 1Byte Send */
		/* Check Gyro Calibration Sequence End */
		do
		{
			cam_ois_i2c_read(o_ctrl, 0x14, &val,
				CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* GCCTRL Read */
			usleep_range(20000, 21000);
		}while(val != 0);
		/* Result check */
		rc = cam_ois_i2c_read(o_ctrl, 0x04, &val,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* OISERR Read */
		if((rc >= 0) && ((val & 0x23) == 0x0)) /* OISERR register GXZEROERR & GYZEROERR & GCOMERR Bit = 0(No Error)*/
		{
			/* Write Gyro Calibration result to OIS DATA SECTION */
			CAM_DBG(CAM_OIS, "cam_ois_offset_test ok %d", val);
			//FlashWriteResultCheck(); /* refer to 4.25 Flash ROM Write Result Check Sample Source */
		} else {
			CAM_DBG(CAM_OIS, "cam_ois_offset_test fail %d", val);
			result = -1;
		}
	}

	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		cam_ois_i2c_read(o_ctrl, 0x0248, &val,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		x = val;
		cam_ois_i2c_read(o_ctrl, 0x0249, &val,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		x_sum = (val << 8) | x;

		if (x_sum > 0x7FFF)
			x_sum = -((x_sum ^ 0xFFFF) + 1);
		sum += x_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_x = sum * 1000 / scale_factor / 10;

	sum = 0;

	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		cam_ois_i2c_read(o_ctrl, 0x024A, &val,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		y = val;
		cam_ois_i2c_read(o_ctrl, 0x024B, &val,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		y_sum = (val << 8) | y;

		if (y_sum > 0x7FFF)
			y_sum = -((y_sum ^ 0xFFFF) + 1);

		sum += y_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_y = sum * 1000 / scale_factor / 10;

	CAM_INFO(CAM_OIS, "end");

	cam_ois_version(o_ctrl);

	return result;
}

static noinline_for_stack long __get_file_size(struct file *file)
{
	struct kstat st;
	u32 request_mask = (STATX_MODE | STATX_SIZE);

	if (vfs_getattr(&file->f_path, &st, request_mask, KSTAT_QUERY_FLAGS))
		return -1;
	if (!S_ISREG(st.mode))
		return -1;
	if (st.size != (long)st.size)
		return -1;

	return st.size;
}

#if defined(CONFIG_SEC_C2Q_PROJECT)
long cam_ois_read_tele_efs(char *efs_path, u8 *buf, int buflen)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	char *filename;
	long ret = 0, fsize = 4, nread = 0;
	loff_t file_offset = 682;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	CAM_INFO(CAM_OIS, "read tele efs enter");

	filename = __getname();
	if (unlikely(!filename)) {
		set_fs(old_fs);
		return 0;
	}

	snprintf(filename, PATH_MAX, "%s", efs_path);

	fp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		__putname(filename);
		set_fs(old_fs);
		return 0;
	}

	nread = kernel_read(fp, buf, fsize, &file_offset);
	if (nread != fsize) {
		CAM_ERR(CAM_OIS, "kernel_read was failed(%ld != %ld)",
			nread, fsize);
		ret = 0;
		goto p_err;
	}

	ret = fsize;

p_err:
	__putname(filename);
	filp_close(fp, current->files);
	set_fs(old_fs);

	return ret;
}
#endif

long cam_ois_read_efs(char *efs_path, u8 *buf, int buflen)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	char *filename;
	long ret = 0, fsize = 0, nread = 0;
	loff_t file_offset = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filename = __getname();
	if (unlikely(!filename)) {
		set_fs(old_fs);
		return 0;
	}

	snprintf(filename, PATH_MAX, "%s", efs_path);

	fp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		__putname(filename);
		set_fs(old_fs);
		return 0;
	}

	fsize = __get_file_size(fp);
	if (fsize <= 0 || fsize > buflen) {
		CAM_ERR(CAM_OIS, " __get_file_size fail(%ld)", fsize);
		ret = 0;
		goto p_err;
	}

	nread = kernel_read(fp, buf, fsize, &file_offset);
	if (nread != fsize) {
		CAM_ERR(CAM_OIS, "kernel_read was failed(%ld != %ld)",
			nread, fsize);
		ret = 0;
		goto p_err;
	}

	ret = fsize;

p_err:
	__putname(filename);
	filp_close(fp, current->files);
	set_fs(old_fs);

	return ret;
}

int cam_ois_get_offset_data(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y)
{
	int ret = 0, i = 0, j = 0, comma_offset = 0;
	bool detect_comma = false;
	char efs_data[MAX_EFS_DATA_LENGTH] = { 0 };
	unsigned char *buffer = NULL;
	long efs_size = 0;

	CAM_DBG(CAM_OIS, "cam_ois_get_offset_data E");
	if (!o_ctrl)
		return 0;

	if (cam_ois_wait_idle(o_ctrl, 5) < 0) {
		CAM_ERR(CAM_OIS, "wait ois idle status failed");
		return -1;
	}

	buffer = vmalloc(MAX_EFS_DATA_LENGTH);
	if (!buffer) {
		CAM_ERR(CAM_OIS, "vmalloc failed");
		return -1;
	}

	efs_size = cam_ois_read_efs(OIS_GYRO_CAL_VALUE_FROM_EFS, buffer, MAX_EFS_DATA_LENGTH);
	if (efs_size == 0) {
			CAM_ERR(CAM_OIS, "efs read failed");
			ret = 0;
			goto ERROR;
	}

	i = 0;
	detect_comma = false;
	for (i = 0; i < efs_size; i++) {
		if (*(buffer + i) == ',') {
			comma_offset = i;
			detect_comma = true;
			break;
		}
	}

	if (detect_comma) {
		memset(efs_data, 0x00, sizeof(efs_data));
		j = 0;
		for (i = 0; i < comma_offset; i++) {
			if (buffer[i] != '.') {
				efs_data[j] = buffer[i];
				j++;
			}
		}
		kstrtol(efs_data, 10, raw_data_x);

		memset(efs_data, 0x00, sizeof(efs_data));
		j = 0;
		for (i = comma_offset + 1; i < efs_size; i++) {
			if (buffer[i] != '.') {
				efs_data[j] = buffer[i];
				j++;
			}
		}
		kstrtol(efs_data, 10, raw_data_y);
	} else {
		CAM_INFO(CAM_OIS, "cannot find delimeter");
		ret = -1;
	}
	CAM_INFO(CAM_OIS, "cam_ois_get_offset_data : X raw_x = %ld, raw_y = %ld",
		*raw_data_x, *raw_data_y);

ERROR:
	if (buffer) {
		vfree(buffer);
		buffer = NULL;
	}

	return ret;
}

/* ois module itselt has selftest function for line test.  */
/* it excutes by setting register and return the result */
uint32_t cam_ois_self_test(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	int retries = 30;
	uint32_t RcvData;
	uint32_t regval = 0, x = 0, y = 0;

	/* OIS Status Check */
	CAM_DBG(CAM_OIS, "GyroSensorSelfTest E");
	if (!o_ctrl)
		return -1;

	if (cam_ois_wait_idle(o_ctrl, 5) < 0) {
		CAM_ERR(CAM_OIS, "wait ois idle status failed");
		return -1;
	}

	/* Gyro Sensor Self Test Start */
	/* GCCTRL GSLFTEST Set */
	rc = cam_ois_i2c_write(o_ctrl, 0x0014, 0x08,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* GCCTRL register(0x0014) 1Byte Send */
	if (rc < 0)
		CAM_ERR(CAM_OIS, "i2c write fail %d", rc);
	/* Check Gyro Sensor Self Test Sequence End */
	do
	{
		rc = cam_ois_i2c_read(o_ctrl, 0x0014, &RcvData,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* GCCTRL Read */
		if (rc < 0)
			CAM_ERR(CAM_OIS, "i2c read fail %d", rc);
		if(--retries < 0){
			CAM_ERR(CAM_OIS, "GCCTRL Read failed , RcvData %X",RcvData);
			break;
		}
		usleep_range(20000, 21000);
	}while(RcvData != 0x00);
	/* Result Check */
	rc = cam_ois_i2c_read(o_ctrl, 0x0004, &RcvData,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* OISERR Read */
	if (rc < 0)
		CAM_ERR(CAM_OIS, "i2c read fail %d", rc);
	if( (RcvData & 0x80) != 0x0) /* OISERR register GSLFERR Bit != 0(Gyro Sensor Self Test Error Found!!) */
	{
		/* Gyro Sensor Self Test Error Process */
		CAM_ERR(CAM_OIS, "GyroSensorSelfTest failed %d \n", RcvData);
		return -1;
	}

	// read x_axis, y_axis
	rc = cam_ois_i2c_read(o_ctrl, 0x00EC, &regval,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	x = regval;

	rc = cam_ois_i2c_read(o_ctrl, 0x00ED, &regval,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	x = (regval << 8)| x;

	rc = cam_ois_i2c_read(o_ctrl, 0x00EE, &regval,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	y = regval;

	rc = cam_ois_i2c_read(o_ctrl, 0x00EF, &regval,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	y = (regval << 8)| y;

	CAM_INFO(CAM_OIS, "Gyro x_axis %u, y_axis %u", x , y);

	CAM_DBG(CAM_OIS, "GyroSensorSelfTest X");
	return RcvData;
}

bool cam_ois_sine_wavecheck(struct cam_ois_ctrl_t *o_ctrl, int threshold,
	struct cam_ois_sinewave_t *sinewave, int *result, int num_of_module)
{
	uint32_t buf = 0, val = 0;
	int i = 0, ret = 0, retries = 10;
	int sinx_count = 0, siny_count = 0;
	uint32_t u16_sinx_count = 0, u16_siny_count = 0;
	uint32_t u16_sinx = 0, u16_siny = 0;
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	int result_addr[2] = {0x00C0, 0x00E4};
#else
	int result_addr[1] = {0x00E4};
#endif
	if (!o_ctrl)
		return false;

	ret = cam_ois_i2c_write(o_ctrl, 0x0052, (uint16_t)threshold,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* error threshold level. */

	if (num_of_module == 1)
		ret |= cam_ois_i2c_write(o_ctrl, 0x00BE, 0x01,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* select module */
	else if (num_of_module == 2) {
		ret |= cam_ois_i2c_write(o_ctrl, 0x00BE, 0x03,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* select module */
		ret |= cam_ois_i2c_write(o_ctrl, 0x005B, (uint16_t)threshold,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* Module#2 error threshold level. */
	}
	ret |= cam_ois_i2c_write(o_ctrl, 0x0053, 0x00,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* count value for error judgement level. */
	ret |= cam_ois_i2c_write(o_ctrl, 0x0054, 0x05,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* frequency level for measurement. */
	if (num_of_module == 1) {
		ret |= cam_ois_i2c_write(o_ctrl, 0x0055, 0x34,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* amplitude level for measurement. */
	} else if (num_of_module == 2) {
		ret |= cam_ois_i2c_write(o_ctrl, 0x0055, 0x2A,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* amplitude level for measurement. */
		ret |= cam_ois_i2c_write(o_ctrl, 0x0056, 0x03,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* dummy pluse setting. */
	}
	ret |= cam_ois_i2c_write(o_ctrl, 0x0057, 0x02,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* vyvle level for measurement. */
	ret |= cam_ois_i2c_write(o_ctrl, 0x0050, 0x01,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* start sine wave check operation */

	if (ret < 0) {
		CAM_ERR(CAM_OIS, "i2c write fail");
		*result = 0xFF;
		return false;
	}

	retries = 30;
	do {
		ret = cam_ois_i2c_read(o_ctrl, 0x0050, &val,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (ret < 0) {
			CAM_ERR(CAM_OIS, "i2c read fail");
			break;
		}

		msleep(100);

		if (--retries < 0) {
			CAM_ERR(CAM_OIS, "sine wave operation fail.");
			*result = 0xFF;
			return false;
		}
	} while (val);

	ret = cam_ois_i2c_read(o_ctrl, 0x0051, &buf,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (ret < 0)
		CAM_ERR(CAM_OIS, "i2c read fail");

	*result = (int)buf;
	CAM_INFO(CAM_OIS, "MCERR(0x51)=%d", buf);

	for (i = 0; i < num_of_module ; i++) {
		ret = cam_ois_i2c_read(o_ctrl, result_addr[i], &u16_sinx_count,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
		sinx_count = ((u16_sinx_count & 0xFF00) >> 8) | ((u16_sinx_count &	0x00FF) << 8);
		if (sinx_count > 0x7FFF)
			sinx_count = -((sinx_count ^ 0xFFFF) + 1);

		ret |= cam_ois_i2c_read(o_ctrl, result_addr[i] + 2, &u16_siny_count,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
		siny_count = ((u16_siny_count & 0xFF00) >> 8) | ((u16_siny_count &	0x00FF) << 8);
		if (siny_count > 0x7FFF)
			siny_count = -((siny_count ^ 0xFFFF) + 1);

		ret |= cam_ois_i2c_read(o_ctrl, result_addr[i] + 4, &u16_sinx,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
		sinewave[i].sin_x = ((u16_sinx & 0xFF00) >> 8) | ((u16_sinx &  0x00FF) << 8);
		if (sinewave[i].sin_x > 0x7FFF)
			sinewave[i].sin_x = -((sinewave[i].sin_x ^ 0xFFFF) + 1);

		ret |= cam_ois_i2c_read(o_ctrl, result_addr[i] + 6, &u16_siny,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
		sinewave[i].sin_y = ((u16_siny & 0xFF00) >> 8) | ((u16_siny &  0x00FF) << 8);
		if (sinewave[i].sin_y > 0x7FFF)
			sinewave[i].sin_y = -((sinewave[i].sin_y ^ 0xFFFF) + 1);

		if (ret < 0)
			CAM_ERR(CAM_OIS, "i2c read fail");

		CAM_INFO(CAM_OIS, "[Module#%d] threshold = %d, sinx = %d, siny = %d, sinx_count = %d, siny_count = %d",
			i + 1, threshold, sinewave[i].sin_x, sinewave[i].sin_y, sinx_count, siny_count);
	}

	if (buf == 0x0)
		return true;
	else
		return false;
}

int cam_ois_check_fw(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0, i = 0;
	uint32_t chksum_rumba = 0xFFFF;
	uint32_t chksum_line = 0xFFFF;
	uint32_t is_different_crc = 0xFFFF;
	bool is_force_update = false;
	bool is_need_retry = false;
	bool is_cal_wrong = false;
	bool is_empty_cal_ver = false;
	bool is_mcu_nack = false;
	bool no_mod_ver = false;
	bool no_fw_at_system = false;
	int update_retries = 3;
	bool is_fw_crack = false;
	char ois_dev_core[] = {'A', 'B', 'E', 'F', 'I', 'J', 'M', 'N'};
	char fw_ver_ng[OIS_VER_SIZE + 1] = "NG_FW2";
	char cal_ver_ng[OIS_VER_SIZE + 1] = "NG_CD2";

	CAM_INFO(CAM_OIS, "E");
FW_UPDATE_RETRY:
	is_mcu_nack = false;
	is_force_update = false;

	rc = cam_ois_power_up(o_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "OIS Power up failed");
		goto end;
	}
	//target_normal_on(o_ctrl);
	//msleep(50);
	msleep(15);

#if defined(CONFIG_SEC_C2Q_PROJECT)
        /*350ms waiting is added for below reason, requested by OIS HW engineer - kioh.jung
         *Clock of Rumba-S4E is going to be set 26Mhz by module maker at shipping.
         *However, QC AP can supply 19.2Mhz external clock for Rumba-S4E and camera sensors.
         *So, OIS MCU will change with clock parameters to fit 19.2Mhz at booting.
         *And AP have to wait while OIS MCU changes the values.
         */
        //TODO:Need to check whether booting time is affected by adding 350ms delay
	rc = cam_ois_wait_idle(o_ctrl, 35);
#else
	rc = cam_ois_wait_idle(o_ctrl, 5);
#endif
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "wait ois idle status failed");
		CAM_ERR(CAM_OIS ,"MCU NACK, may need update FW");
		is_force_update = true;
		is_mcu_nack = true;
	}

	rc = cam_ois_get_fw_status(o_ctrl);
	if (rc) {
		CAM_ERR(CAM_OIS, "Previous update had not been properly, start force update");
		is_force_update = true;
		if(rc == -2){
		    CAM_ERR(CAM_OIS ,"MCU NACK, may need update FW");
		    is_mcu_nack = true;
		}
	} else {
		is_need_retry = false;
	}

	CAM_INFO(CAM_OIS, "ois cal ver : %x %x %x %x %x %x, checksum rumba : 0x%04X, line : 0x%04X, is_different = %d",
		o_ctrl->cal_ver[0], o_ctrl->cal_ver[1],
		o_ctrl->cal_ver[2], o_ctrl->cal_ver[3],
		o_ctrl->cal_ver[4], o_ctrl->cal_ver[5],
		chksum_rumba, chksum_line, is_different_crc);

	//	check cal version, if hw ver of cal version is not string, which means module hw ver is not written
	//	there is no need to compare the version to update FW.
	for (i = 0; i < OIS_VER_SIZE; i++) {
		if (isalnum(o_ctrl->cal_ver[i]) == '\0') {
			is_empty_cal_ver = 0;
			CAM_ERR(CAM_OIS, "Cal Ver is not vaild. will not update firmware");
			break;
		}
	}

	if (!is_need_retry) { // when retry it will skip, not to overwirte the mod ver which might be cracked becase of previous update fail
		rc = cam_ois_read_module_ver(o_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "read module version fail %d. skip fw update", rc);
			no_mod_ver = true;
			if(rc == -2){
				is_mcu_nack = true;
			}else{
			    goto pwr_dwn;
			}
		}
	}

	rc = cam_ois_read_phone_ver(o_ctrl);
	if (rc < 0) {
		no_fw_at_system = true;
		CAM_ERR(CAM_OIS, "No available OIS FW exists in system");
	}

	CAM_INFO(CAM_OIS, "[OIS version] phone : %s, cal %s, module %s",
		o_ctrl->phone_ver, o_ctrl->cal_ver, o_ctrl->module_ver);

	for (i = 0; i < (int)(sizeof(ois_dev_core)/sizeof(char)); i++) {
		if (o_ctrl->module_ver[0] == ois_dev_core[i]) {
			if(is_mcu_nack != true){
			    CAM_ERR(CAM_OIS, "[OIS FW] devleopment module(core version : %c), skip update FW", o_ctrl->module_ver[0]);
			    //goto pwr_dwn;
			}
		}
	}

#if defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_C1Q_PROJECT)
        //Below code checks whether sensor module OIS FW version is from Z3, if so, then load right FW from phone
        if ((strncmp(o_ctrl->module_ver, "IFA1", OIS_MCU_VERSION_SIZE) == '\0') &&  //Z3 OIS FW
	    ((strncmp(o_ctrl->phone_ver, "LFA1", OIS_MCU_VERSION_SIZE) == '\0')||   //Canvas2 OIS FW
             (strncmp(o_ctrl->phone_ver, "KBA1", OIS_MCU_VERSION_SIZE) == '\0'))) { //Canvas1 OIS FW
	   CAM_INFO(CAM_OIS, "[OIS FW] force update(module %s, phone %s)", o_ctrl->module_ver, o_ctrl->phone_ver);
  	   is_mcu_nack = true;
	}
#elif defined(CONFIG_SEC_R8Q_PROJECT)
        //Below code checks whether sensor module OIS FW version is from Hubble, if so, then load right FW from phone
        if ((strncmp(o_ctrl->module_ver, "ML1M", OIS_MCU_VERSION_SIZE) == '\0') &&  //Y2 OIS FW
	    (strncmp(o_ctrl->phone_ver, "NE4A", OIS_MCU_VERSION_SIZE) == '\0')) { //R8 OIS FW
	   CAM_INFO(CAM_OIS, "[OIS FW] force update(module %s, phone %s)", o_ctrl->module_ver, o_ctrl->phone_ver);
  	   is_mcu_nack = true;
	}
#endif

	if(update_retries < 0){
	    is_mcu_nack = false;
	}
 	if (!is_empty_cal_ver || is_mcu_nack == true) {
		if (strncmp(o_ctrl->phone_ver, o_ctrl->module_ver, OIS_MCU_VERSION_SIZE) == '\0' || is_mcu_nack == true) { //check if it is same hw (phone ver = module ver)
			if ((strncmp(o_ctrl->phone_ver, o_ctrl->module_ver, OIS_VER_SIZE) > '\0')
				|| is_force_update || is_mcu_nack == true) {
				CAM_INFO(CAM_OIS, "update OIS FW from phone");
				CAM_INFO(CAM_OIS, "is_force_update %d , is_mcu_nack %d ",is_force_update,is_mcu_nack);
				rc = cam_ois_fw_update(o_ctrl, is_force_update);
				if (rc < 0) {
					is_need_retry = true;
					CAM_ERR(CAM_OIS, "update fw fail, it will retry (%d)", 4 - update_retries);
					if (--update_retries < 0) {
						CAM_ERR(CAM_OIS, "update fw fail, stop retry");
						is_need_retry = false;
					}
				} else{
					CAM_INFO(CAM_OIS, "update succeeded from phone");
					is_need_retry = false;
				}
			}
		}
	}

	if (!is_need_retry) {
		rc = cam_ois_read_module_ver(o_ctrl);
		if (rc < 0) {
			no_mod_ver = true;
			CAM_ERR(CAM_OIS, "read module version fail %d.", rc);
		}
	}

pwr_dwn:
	rc = cam_ois_get_fw_status(o_ctrl);
	if (rc < 0)
		is_fw_crack = true;

	if (!is_need_retry) { //when retry not to change mod ver
		if (is_fw_crack)
			memcpy(o_ctrl->module_ver, fw_ver_ng, (OIS_VER_SIZE) * sizeof(uint8_t));
		else if (is_cal_wrong)
			memcpy(o_ctrl->module_ver, cal_ver_ng, (OIS_VER_SIZE) * sizeof(uint8_t));
	}

	snprintf(ois_fw_full, 40, "%s %s\n", o_ctrl->module_ver,
		((no_fw_at_system == 1 || no_mod_ver == 1)) ? ("NULL") : (o_ctrl->phone_ver));
	CAM_INFO(CAM_OIS, "[init OIS version] module : %s, phone : %s",
		o_ctrl->module_ver, o_ctrl->phone_ver);

#if defined(CONFIG_SAMSUNG_APERTURE)
	// To set aprture open mode while bootup
	cam_aperture_set_mode(&o_ctrl->io_master_info, CAM_APERTURE_1P5);
	msleep(20);
#endif

	cam_ois_power_down(o_ctrl);

	if (is_need_retry)
		goto FW_UPDATE_RETRY;
end:
	CAM_INFO(CAM_OIS, "X");
	return rc;
}

int32_t cam_ois_set_debug_info(struct cam_ois_ctrl_t *o_ctrl, uint16_t mode)
{
	uint32_t status_reg = 0;
	int rc = 0;
	char exif_tag[6] = "ssois"; //defined exif tag for ois

	CAM_DBG(CAM_OIS, "Enter");

	if (cam_ois_i2c_read(o_ctrl, 0x01, &status_reg,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE) < 0) //read Status register
		CAM_ERR(CAM_OIS, "get ois status register value failed, i2c fail");

	snprintf(ois_debug, 40, "%s%s %s %s %x %x %x\n", exif_tag,
		(o_ctrl->module_ver[0] == '\0') ? ("ISNULL") : (o_ctrl->module_ver),
		(o_ctrl->phone_ver[0] == '\0') ? ("ISNULL") : (o_ctrl->phone_ver),
		(o_ctrl->cal_ver[0] == '\0') ? ("ISNULL") : (o_ctrl->cal_ver),
		o_ctrl->err_reg, status_reg, mode);

	CAM_INFO(CAM_OIS, "ois exif debug info %s", ois_debug);
	CAM_DBG(CAM_OIS, "Exit");

	return rc;
}

int cam_ois_get_ois_mode(struct cam_ois_ctrl_t *o_ctrl, uint16_t *mode)
{
	if (!o_ctrl)
		return -1;

	*mode = o_ctrl->ois_mode;
	return 0;
}

/*** Have to lock/unlock ois_mutex, before/after call this function ***/
int cam_ois_set_ois_mode(struct cam_ois_ctrl_t *o_ctrl, uint16_t mode)
{
	int rc = 0;

	if (!o_ctrl)
		return 0;

	if (mode == o_ctrl->ois_mode)
		return 0;

	if (o_ctrl->ois_mode == 0x16) {
		CAM_INFO(CAM_OIS, "SensorHub Reset, Skip mode %u setting", mode);
		return 0;
	}

	rc = cam_ois_i2c_write(o_ctrl, 0x0002, mode,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "i2c write fail");

	rc = cam_ois_i2c_write(o_ctrl, 0x0000, 0x01,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //servo on
	if (rc < 0)
		CAM_ERR(CAM_OIS, "i2c write fail");

	o_ctrl->ois_mode = mode;
	o_ctrl->is_servo_on = true;

	cam_ois_set_debug_info(o_ctrl, o_ctrl->ois_mode);

	CAM_INFO(CAM_OIS, "set ois mode %d", mode);

	return rc;
}

int cam_ois_fixed_aperture(struct cam_ois_ctrl_t *o_ctrl)
{
	uint8_t data[2] = { 0, };
	int rc = 0, val = 0;

	// OIS CMD(Fixed Aperture)
	val = o_ctrl->x_center;
	CAM_DBG(CAM_OIS, "Write X center %d", val);
	data[0] = val & 0xFF;
	data[1] = (val >> 8) & 0xFF;
	rc = cam_ois_i2c_write_continous(o_ctrl, 0x0022, data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(data));
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Failed write X center");

	val = o_ctrl->y_center;
	CAM_DBG(CAM_OIS, "Write Y center %d", val);
	data[0] = val & 0xFF;
	data[1] = (val >> 8) & 0xFF;
	rc = cam_ois_i2c_write_continous(o_ctrl, 0x0024, data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(data));
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Failed write Y center");

	// OIS fixed
	rc = cam_ois_set_ois_mode(o_ctrl, 0x02);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "ois set fixed mode failed %d", rc);
		return rc;
	}
	return rc;
}

int cam_ois_write_xgg_ygg(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	uint32_t i = 0;

	if (!o_ctrl)
		return 0;

	CAM_DBG(CAM_OIS, "E");

	if (ois_wide_cal_mark == 0xBB) {
		//Wide XGG,YGG
		rc = cam_ois_i2c_write_continous(o_ctrl, 0x0254, ois_wide_xygg,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(ois_wide_xygg));
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed write Wide XGG, YGG");
		for (i = 0; i < sizeof(ois_wide_xygg)/sizeof(uint8_t) ; i++)
			CAM_DBG(CAM_OIS, "[0x%x] 0x%x", 0x0254 + i, ois_wide_xygg[i]);
	}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (ois_tele_cal_mark == 0xBB) {
		//Tele XGG,YGG
		rc = cam_ois_i2c_write_continous(o_ctrl, 0x0554, ois_tele_xygg,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(ois_tele_xygg));
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed write Tele XGG, YGG");
		for (i = 0; i < sizeof(ois_tele_xygg)/sizeof(uint8_t) ; i++)
			CAM_DBG(CAM_OIS, "[0x%x] 0x%x", 0x0554 + i, ois_tele_xygg[i]);
	}
#endif

	CAM_DBG(CAM_OIS, "X");

	return rc;
}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
#if defined(CONFIG_SEC_C2Q_PROJECT)
bool cam_ois_write_efs_tele_center_shift(struct cam_ois_ctrl_t *o_ctrl)
{
	char dump[6] = { 0 };
	unsigned char *buffer = NULL;
	long efs_size = 0;
	int i = 0;
	int rc = 0;

	CAM_INFO(CAM_OIS, "Write Tele center shift Enter");

	buffer = vmalloc(MAX_EFS_DATA_LENGTH_TELE);
	if (!buffer) {
		CAM_ERR(CAM_OIS, "vmalloc failed");
		return -1;
	}

	efs_size = cam_ois_read_tele_efs(OIS_TELE_CAMERA_TILT_CALIBRATION_EFS, buffer, MAX_EFS_DATA_LENGTH_TELE);
	if (efs_size == 0) {
		CAM_ERR(CAM_OIS, "efs read failed");
		goto ERROR;
	}

	CAM_DBG(CAM_OIS, "efs size : %ld", efs_size);

	for (i = 0; i < efs_size; i++) {
		dump[i] = buffer[i];
		CAM_DBG(CAM_OIS, "dump : %d : %x", i, dump[i]);
	}

	if (ois_tele_cal_mark == 0xBB) {
		rc = cam_ois_i2c_write_continous(o_ctrl, 0x0446, &buffer[0],
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(buffer[0]));
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed to write Tele center shift 1");

		rc = cam_ois_i2c_write_continous(o_ctrl, 0x0447, &buffer[1],
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(buffer[1]));
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed to write Tele center shift 2");

		rc = cam_ois_i2c_write_continous(o_ctrl, 0x0448, &buffer[2],
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(buffer[2]));
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed to write Tele center shift 3");

		rc = cam_ois_i2c_write_continous(o_ctrl, 0x0449, &buffer[3],
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(buffer[3]));
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed to write Tele center shift 4");

		CAM_DBG(CAM_OIS, "Tele data [0x%x] 0x%x", 0x0446, buffer[0]);
		CAM_DBG(CAM_OIS, "Tele data [0x%x] 0x%x", 0x0447, buffer[1]);
		CAM_DBG(CAM_OIS, "Tele data [0x%x] 0x%x", 0x0448, buffer[2]);
		CAM_DBG(CAM_OIS, "Tele data [0x%x] 0x%x", 0x0449, buffer[3]);
	}

	CAM_INFO(CAM_OIS, "Write Tele center shift success");

	return 1;

ERROR:
	if (buffer) {
		vfree(buffer);
		buffer = NULL;
	}

	return 0;
}
#endif

int cam_ois_write_dual_cal(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	uint32_t i = 0;

	if (!o_ctrl)
		return 0;

	CAM_DBG(CAM_OIS, "E");

	if (ois_wide_cal_mark == 0xBB) {
		rc = cam_ois_i2c_write_continous(o_ctrl, 0x0442, ois_wide_center_shift,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(ois_wide_center_shift));
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed write Wide center shift");
		for (i = 0; i < sizeof(ois_wide_center_shift)/sizeof(uint8_t) ; i++)
			CAM_DBG(CAM_OIS, "[0x%x] 0x%x", 0x0442 + i, ois_wide_center_shift[i]);
	}

#if defined(CONFIG_SEC_C2Q_PROJECT)
	if(!efs_cal_data_read_done){
		efs_cal_data_read_done = TRUE;
		if(cam_ois_write_efs_tele_center_shift(o_ctrl)){
			CAM_ERR(CAM_OIS, "Write Tele center shift from EFS success");
		}else{
			CAM_ERR(CAM_OIS, "Write Tele center shift from EFS failed");
			if (ois_tele_cal_mark == 0xBB) {
				rc = cam_ois_i2c_write_continous(o_ctrl, 0x0446, ois_tele_center_shift,
					CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(ois_tele_center_shift));
			if (rc < 0)
				CAM_ERR(CAM_OIS, "Failed write Tele center shift");
			for (i = 0; i < sizeof(ois_tele_center_shift)/sizeof(uint8_t) ; i++)
				CAM_DBG(CAM_OIS, "Tele data [0x%x] 0x%x", 0x0446 + i, ois_tele_center_shift[i]);
			}
		}
	}
#else
	if (ois_tele_cal_mark == 0xBB) {
		rc = cam_ois_i2c_write_continous(o_ctrl, 0x0446, ois_tele_center_shift,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(ois_tele_center_shift));
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed write Tele center shift");
		for (i = 0; i < sizeof(ois_tele_center_shift)/sizeof(uint8_t) ; i++)
			CAM_DBG(CAM_OIS, "[0x%x] 0x%x", 0x0446 + i, ois_tele_center_shift[i]);
	}
#endif

	rc = cam_ois_i2c_write(o_ctrl, 0x0440, 0x01,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Failed Enable Dual Shift");

	CAM_DBG(CAM_OIS, "X");

	return rc;
}
#endif

int cam_ois_write_gyro_orientation(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	uint8_t SendData1[3] = { 0, };
	uint8_t SendData2[2] = { 0, };
#if defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
	unsigned int rev = sec_hw_rev();
#else
	unsigned int rev = 0;
#endif

#if defined(CONFIG_SEC_X1Q_PROJECT)
	uint8_t new_data1[3] = { 0x01, 0x01, 0x01 };
	uint8_t new_data2[2] = { 0x00, 0x00 };
#elif defined(CONFIG_SEC_Z3Q_PROJECT)
	uint8_t new_data1[3] = { 0x01, 0x00, 0x01 };
	uint8_t new_data2[2] = { 0x00, 0x01 };
#elif defined(CONFIG_SEC_C2Q_PROJECT)
        uint8_t new_data1[3] = {0};
        uint8_t new_data2[2] = {0};

        if(rev <= 2)
        {
           new_data1[0] = 0x00, new_data1[1] = 0x00, new_data1[2] = 0x00;
           new_data2[0] = 0x01, new_data2[1] = 0x01;
        }
        else if(rev == 3 || rev == 4)
        {
           new_data1[0] = 0x01, new_data1[1] = 0x00, new_data1[2] = 0x01;
           new_data2[0] = 0x00, new_data2[1] = 0x01;
        }
        else if(rev == 5)
        {
           new_data1[0] = 0x01, new_data1[1] = 0x01, new_data1[2] = 0x00;
           new_data2[0] = 0x00, new_data2[1] = 0x00;
        }
        else /* above revison 7 onwards*/
        {
           new_data1[0] = 0x00, new_data1[1] = 0x01, new_data1[2] = 0x00;
           new_data2[0] = 0x01, new_data2[1] = 0x00;
        }
#elif defined(CONFIG_SEC_C1Q_PROJECT)
        uint8_t new_data1[3] = { 0x01, 0x00, 0x00 };
        uint8_t new_data2[2] = { 0x00, 0x01 };
#elif defined(CONFIG_SEC_R8Q_PROJECT)
	uint8_t new_data1[3] = { 0x00, 0x00, 0x01 };
	uint8_t new_data2[2] = { 0x01, 0x01 };

	if(rev >= 1)
	{
           new_data1[0] = 0x01, new_data1[1] = 0x01, new_data1[2] = 0x01;
           new_data2[0] = 0x00, new_data2[1] = 0x00;
	}
#if defined(CONFIG_MACH_R8Q_EUR_OPEN) || defined(CONFIG_MACH_R8Q_CHN_HKX) || defined(CONFIG_MACH_R8Q_CHN_OPEN)\
	|| defined(CONFIG_MACH_R8Q_KOR_SINGLE)
	new_data1[0] = 0x00, new_data1[1] = 0x01, new_data1[2] = 0x01;
	new_data2[0] = 0x01, new_data2[1] = 0x00;
#endif
#elif defined(CONFIG_SEC_F2Q_PROJECT) || defined(CONFIG_SEC_VICTORY_PROJECT)
	uint8_t new_data1[3] = { 0x00, 0x00, 0x01 };
	uint8_t new_data2[2] = { 0x01, 0x01 };
#else
	uint8_t new_data1[3] = { 0x01, 0x00, 0x00 };
	uint8_t new_data2[2] = { 0x00, 0x01 };
#endif
	memcpy(SendData1, new_data1, sizeof(new_data1));
	memcpy(SendData2, new_data2, sizeof(new_data2));

	if (!o_ctrl)
		return 0;

	CAM_DBG(CAM_OIS, "E");
	CAM_INFO(CAM_OIS, "HWID 0x%x", rev);

	CAM_DBG(CAM_OIS, "Wx Pole %u, Wy Pole %u, GyroOrientation 0x%x", SendData1[0], SendData1[1], SendData1[2]);
	rc = cam_ois_i2c_write_continous(o_ctrl, 0x0240, SendData1,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(SendData1));
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Failed write Wx Pole, Wy Pole, Gyro Orientation");

	CAM_DBG(CAM_OIS, "Tx Pole %u, Ty Pole %u", SendData2[0], SendData2[1]);
	rc = cam_ois_i2c_write_continous(o_ctrl, 0x0552, SendData2,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, (int)sizeof(SendData2));
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Failed write Tx Pole, Ty Pole");

	CAM_DBG(CAM_OIS, "X");

	return rc;
}

int cam_ois_write_gyro_sensor_calibration(struct cam_ois_ctrl_t *o_ctrl)
{
	int ret = 0;
	uint32_t val = 0;
	int XGZERO = 0, YGZERO = 0;
	int scale_factor = OIS_GYRO_SCALE_FACTOR_LSM6DSO;
	long raw_data_x = 0, raw_data_y = 0;

	ret = cam_ois_get_offset_data(o_ctrl, &raw_data_x, &raw_data_y);
	if (ret < 0) {
		CAM_ERR(CAM_OIS, "Failed to get gyro calibration data");
		return -1;
	}

	CAM_INFO(CAM_OIS, "raw_data_x %ld, raw_data_y %ld", raw_data_x, raw_data_y);

	XGZERO = raw_data_x * scale_factor / 1000;
	if (XGZERO > 0x7FFF)
		XGZERO = -((XGZERO ^ 0xFFFF) + 1);
	CAM_DBG(CAM_OIS, "XGZERO 0x%x", XGZERO);

	val = (XGZERO & 0xFF);
	cam_ois_i2c_write(o_ctrl, 0x0248, val,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	val = ((XGZERO >> 8) & 0xFF);
	cam_ois_i2c_write(o_ctrl, 0x0249, val,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	YGZERO = raw_data_y * scale_factor / 1000;
	if (YGZERO > 0x7FFF)
		YGZERO = -((YGZERO ^ 0xFFFF) + 1);
	CAM_DBG(CAM_OIS, "YGZERO 0x%x", YGZERO);

	val = (YGZERO & 0xFF);
	cam_ois_i2c_write(o_ctrl, 0x024A, val,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	val = ((YGZERO >> 8) & 0xFF);
	cam_ois_i2c_write(o_ctrl, 0x024B, val,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	return ret;
}

int cam_ois_mcu_init(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;

	// Write XGG, YGG to OIS MCU reg
	rc = cam_ois_write_xgg_ygg(o_ctrl);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Write XGG, YGG to OIS MCU reg failed %d", rc);

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	// Write Dual cal value to OIS MCU reg
	rc = cam_ois_write_dual_cal(o_ctrl);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Write Dual cal value to OIS MCU reg failed %d", rc);
#endif

	// Write Gyro orientation to OIS MCU reg
	rc = cam_ois_write_gyro_orientation(o_ctrl);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Write Gyro orientation to OIS MCU reg %d", rc);

	// Write Gyro init offset to OIS MCU reg
	rc = cam_ois_write_gyro_sensor_calibration(o_ctrl);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Write Gyro init to OIS MCU reg %d", rc);

	return rc;
}

void cam_ois_reset(void *ctrl)
{
	struct cam_ois_ctrl_t *o_ctrl = NULL;
	struct cam_ois_thread_msg_t *msg = NULL;
	int rc = 0;

	CAM_INFO(CAM_OIS, "E");

	if (!ctrl)
		return;

	o_ctrl = (struct cam_ois_ctrl_t *)ctrl;

	if (o_ctrl->cam_ois_state >= CAM_OIS_CONFIG) {
		CAM_INFO(CAM_OIS, "camera is running, set mode 0x16");
		msg = kmalloc(sizeof(struct cam_ois_thread_msg_t), GFP_ATOMIC);
		if (msg == NULL) {
			CAM_ERR(CAM_OIS, "Failed alloc memory for msg, Out of memory");
			return;
		}

		memset(msg, 0, sizeof(struct cam_ois_thread_msg_t));
		msg->msg_type = CAM_OIS_THREAD_MSG_RESET;
		rc = cam_ois_thread_add_msg(o_ctrl, msg);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed add msg to OIS thread");
	} else {
		CAM_INFO(CAM_OIS, "camera is not running");
	}

	CAM_INFO(CAM_OIS, "X");
}

int cam_ois_read_hall_position(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t* targetPosition, uint32_t* hallPosition)
{
	int rc = 0, i = 0, j = 0, retries = 5;
    uint32_t val = 0;
	uint32_t targetPositionAddr[4] = { 0x0086, 0x0088, 0x00AC, 0x00AE };
	uint32_t hallPositionAddr[4]   = { 0x008E, 0x0090, 0x00B4, 0x00B6 };

	if (!o_ctrl)
		return 0;

	CAM_INFO(CAM_OIS, "E");
	rc |= cam_ois_i2c_write(o_ctrl, 0x0080, 0x01,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Failed set hall read control bit(FWINFO_CTRL)");

	for (i = 0; i < retries; i++) {
		usleep_range(5000, 5100);
		for (j = 0; j < 4; j++) {
			rc |= cam_ois_i2c_read(o_ctrl, targetPositionAddr[j], &val,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
			targetPosition[j] += ((val >> 8) & 0xFF) | ((val & 0xFF) <<8);

			rc |= cam_ois_i2c_read(o_ctrl, hallPositionAddr[j], &val,
				CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
			hallPosition[j] += ((val >> 8) & 0xFF) | ((val & 0xFF) <<8);

			CAM_DBG(CAM_OIS, "retries %d [%d] target %u, hall %u",
				i, j, targetPosition[j], hallPosition[j]);
		}
	}

    for (j = 0; j < 4; j++) {
        targetPosition[j] /= retries;
        hallPosition[j] /= retries;
    }

	rc |= cam_ois_i2c_write(o_ctrl, 0x0080, 0x0,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "Failed set hall read control bit(FWINFO_CTRL)");

	CAM_INFO(CAM_OIS, "%u,%u,%u,%u,%u,%u,%u,%u",
		targetPosition[0], targetPosition[1],
		targetPosition[2], targetPosition[3],
		hallPosition[0], hallPosition[1],
		hallPosition[2], hallPosition[3]);
	CAM_INFO(CAM_OIS, "X");

	return rc;
}

#if defined(CONFIG_SAMSUNG_OIS_TAMODE_CONTROL)
int ps_notifier_cb(struct notifier_block *nb, unsigned long event, void *data)
{
	struct cam_ois_ctrl_t *o_ctrl =
		container_of(nb, struct cam_ois_ctrl_t, nb);
	struct power_supply *psy = data;
	int rc = 0;

	CAM_DBG(CAM_OIS, "power supply callback");

	if (event != PSY_EVENT_PROP_CHANGED)
		return NOTIFY_OK;

	if (strcmp(psy->desc->name, "battery") == 0) {
		rc = cam_ois_add_tamode_msg(o_ctrl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed add msg to OIS thread");
	}

	return NOTIFY_OK;
}

int cam_ois_add_tamode_msg(struct cam_ois_ctrl_t *o_ctrl) {
	struct cam_ois_thread_msg_t *msg = NULL;
	int rc = 0;

	if (!o_ctrl)
		return rc;

	if (o_ctrl->cam_ois_state >= CAM_OIS_CONFIG)
	{
		msg = kmalloc(sizeof(struct cam_ois_thread_msg_t), GFP_ATOMIC);
		if (msg == NULL) {
			CAM_ERR(CAM_OIS, "Failed alloc memory for msg, Out of memory");
			return -ENOMEM;
		}

		memset(msg, 0, sizeof(struct cam_ois_thread_msg_t));
		msg->msg_type = CAM_OIS_THREAD_MSG_SET_TAMODE;
		rc = cam_ois_thread_add_msg(o_ctrl, msg);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed add msg to OIS thread");
	}
	return rc;
}

int cam_ois_set_ta_mode(struct cam_ois_ctrl_t *o_ctrl) {
	union power_supply_propval status_val, ac_val;
	bool onoff = false;
	int rc = 0;

	CAM_DBG(CAM_OIS, "E");

	status_val.intval = ac_val.intval = 0;
	psy_do_property("battery", get, POWER_SUPPLY_PROP_STATUS, status_val);
	psy_do_property("ac", get, POWER_SUPPLY_PROP_ONLINE, ac_val);
	onoff = (status_val.intval == POWER_SUPPLY_STATUS_FULL && ac_val.intval);

	if (onoff != o_ctrl->ois_tamode_onoff) {
		CAM_INFO(CAM_OIS, "%s: status = %d, ac = %d", __func__, status_val.intval, ac_val.intval);
		CAM_INFO(CAM_OIS, "ois ta mode onoff = %d", onoff);

		rc = cam_ois_i2c_write(o_ctrl, 0x0035, (onoff > 0 ? 0x01 : 0x00),
				CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "set ta mode fail");
			return rc;
		}
		o_ctrl->ois_tamode_onoff = onoff;
	}

	CAM_INFO(CAM_OIS, "X");

	return rc;
}
#endif
