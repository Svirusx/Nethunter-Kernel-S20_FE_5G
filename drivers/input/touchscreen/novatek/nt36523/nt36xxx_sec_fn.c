/*
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
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


#include <linux/delay.h>

#include "nt36xxx.h"

#define NORMAL_MODE		0x00
#define TEST_MODE_1		0x21
#define TEST_MODE_2		0x22
#define MP_MODE_CC		0x41
#define FREQ_HOP_DISABLE	0x66
#define FREQ_HOP_ENABLE		0x65
#define HANDSHAKING_HOST_READY	0xBB

#define GLOVE_ENTER		0xB1
#define GLOVE_LEAVE		0xB2
#define EDGE_REJ_VERTICLE_MODE	0xBA
#define EDGE_REJ_LEFT_UP_MODE	0xBB
#define EDGE_REJ_RIGHT_UP_MODE	0xBC
#define BLOCK_AREA_ENTER	0x71
#define BLOCK_AREA_LEAVE	0x72
#define EDGE_AREA_ENTER		0x73 //min:7
#define EDGE_AREA_LEAVE		0x74 //0~6
#define HOLE_AREA_ENTER		0x75 //0~120
#define HOLE_AREA_LEAVE		0x76 //no report
#define SPAY_SWIPE_ENTER	0x77
#define SPAY_SWIPE_LEAVE	0x78
#define DOUBLE_CLICK_ENTER	0x79
#define DOUBLE_CLICK_LEAVE	0x7A
#define SENSITIVITY_TEST_ENTER	0x7B
#define SENSITIVITY_TEST_LEAVE	0x7C
#define EXTENDED_CUSTOMIZED_CMD	0x7F

typedef enum {
	SET_GRIP_EXECPTION_ZONE = 1,
	SET_GRIP_PORTRAIT_MODE = 2,
	SET_GRIP_LANDSCAPE_MODE = 3,
	SET_PALM_MODE = 4,
	SET_TOUCH_DEBOUNCE = 5,
	SET_GAME_MODE = 6,
	SET_HIGH_SENSITIVITY_MODE = 7,
} EXTENDED_CUSTOMIZED_CMD_TYPE;

typedef enum {
	PALM_DISABLE = 0,
	PALM_NORMAL = 1,
	PALM_SUPPORT_STYLUS = 2,	//to prevent stylus not working in NOTE APP
} PALM_MODE;

typedef enum {
	DEBOUNCE_NORMAL = 0,
	DEBOUNCE_LOWER = 1,	//to optimize tapping performance for SIP
} TOUCH_DEBOUNCE;

typedef enum {
	GAME_MODE_DISABLE = 0,
	GAME_MODE_ENABLE = 1,
} GAME_MODE;

typedef enum {
	HIGH_SENSITIVITY_DISABLE = 0,
	HIGH_SENSITIVITY_ENABLE = 1,
} HIGH_SENSITIVITY_MODE;

#define I2C_TANSFER_LENGTH	64

#define XDATA_SECTOR_SIZE	256

#define CMD_RESULT_WORD_LEN	10

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


/* function bit combination code */
#define EDGE_REJ_VERTICLE	1
#define EDGE_REJ_LEFT_UP	2
#define EDGE_REJ_RIGHT_UP	3

#define ORIENTATION_0		0
#define ORIENTATION_90		1
#define ORIENTATION_180		2
#define ORIENTATION_270		3

#define OPEN_SHORT_TEST		1
#define CHECK_ONLY_OPEN_TEST	1
#define CHECK_ONLY_SHORT_TEST	2

typedef enum {
	GLOVE = 1,
	EDGE_REJECT_L = 5,
	EDGE_REJECT_H,
	EDGE_PIXEL,
	HOLE_PIXEL,
	SPAY_SWIPE,
	DOUBLE_CLICK,
	SENSITIVITY_TEST,
	BLOCK_AREA,
	NOISE,
	HIGH_SENSITIVITY,
	FUNCT_MAX,
} FUNCT_BIT;

typedef enum {
	GLOVE_MASK		= 0x0002,
	EDGE_REJECT_MASK 	= 0x0060,
	EDGE_PIXEL_MASK		= 0x0080,
	HOLE_PIXEL_MASK		= 0x0100,
	SPAY_SWIPE_MASK		= 0x0200,
	DOUBLE_CLICK_MASK	= 0x0400,
	SENSITIVITY_TEST_MASK	= 0x0800,
	BLOCK_AREA_MASK		= 0x1000,
	NOISE_MASK		= 0x2000,
	HIGH_SENSITIVITY_MASK		= 0x4000,
	FUNCT_ALL_MASK		= 0x7FE2,
} FUNCT_MASK;

enum {
	BUILT_IN = 0,
	UMS,
	NONE,
	SPU,
};

u16 landscape_deadzone[2] = { 0 };

int nvt_ts_get_fw_info(struct nvt_ts_data *ts);

int nvt_ts_fw_update_from_bin(struct nvt_ts_data *ts);
int nvt_ts_fw_update_from_external(struct nvt_ts_data *ts, const char *file_path);

int nvt_ts_resume_pd(struct nvt_ts_data *ts);
int nvt_ts_get_checksum(struct nvt_ts_data *ts, u8 *csum_result, u8 csum_size);

int32_t nvt_ts_set_page(struct nvt_ts_data *ts, uint16_t i2c_addr, uint32_t addr);

int nvt_ts_nt36523_ics_i2c_read(struct nvt_ts_data *ts, u32 address, u8 *data, u16 len);
int nvt_ts_nt36523_ics_i2c_write(struct nvt_ts_data *ts, u32 address, u8 *data, u16 len);

static void nvt_ts_set_grip_exception_zone(struct nvt_ts_data *ts, int *cmd_param);
static void nvt_ts_set_grip_portrait_mode(struct nvt_ts_data *ts, int *cmd_param);
static void nvt_ts_set_grip_landscape_mode(struct nvt_ts_data *ts, int *cmd_param);

static int nvt_ts_set_touchable_area(struct nvt_ts_data *ts)
{
	u16 smart_view_area[4] = {0, 210, DEFAULT_MAX_WIDTH, 2130};
	char data[10] = { 0 };
	int ret;

	//---set xdata index to EVENT BUF ADDR---
	data[0] = 0xFF;
	data[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	data[2] = (ts->mmap->EVENT_BUF_ADDR>> 8) & 0xFF;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, data, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set xdata index\n",
				__func__);
		return ret;
	}

	//---set area---
	data[0] = EVENT_MAP_BLOCK_AREA;
	data[1] = smart_view_area[0] & 0xFF;
	data[2] = smart_view_area[0] >> 8;
	data[3] = smart_view_area[1] & 0xFF;
	data[4] = smart_view_area[1] >> 8;
	data[5] = smart_view_area[2] & 0xFF;
	data[6] = smart_view_area[2] >> 8;
	data[7] = smart_view_area[3] & 0xFF;
	data[8] = smart_view_area[3] >> 8;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, data, 9);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set area\n",
				__func__);
		return ret;
	}

	return 0;
}

/*
static int nvt_ts_set_untouchable_area(struct nvt_ts_data *ts)
{
	char data[10] = { 0 };
	u16 touchable_area[4] = {0, 0, DEFAULT_MAX_WIDTH, DEFAULT_MAX_HEIGHT};
	int ret;

	if (landscape_deadzone[0])
		touchable_area[1] = landscape_deadzone[0];

	if (landscape_deadzone[1])
		touchable_area[3] -= landscape_deadzone[1];

	//---set xdata index to EVENT BUF ADDR---
	data[0] = 0xFF;
	data[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	data[2] = (ts->mmap->EVENT_BUF_ADDR>> 8) & 0xFF;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, data, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set xdata index\n",
				__func__);
		return ret;
	}

	//---set area---
	data[0] = EVENT_MAP_BLOCK_AREA;
	data[1] = touchable_area[0] & 0xFF;
	data[2] = touchable_area[0] >> 8;
	data[3] = touchable_area[1] & 0xFF;
	data[4] = touchable_area[1] >> 8;
	data[5] = touchable_area[2] & 0xFF;
	data[6] = touchable_area[2] >> 8;
	data[7] = touchable_area[3] & 0xFF;
	data[8] = touchable_area[3] >> 8;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, data, 9);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set area\n",
				__func__);
		return ret;
	}

	return 0;
}
*/

static int nvt_ts_disable_block_mode(struct nvt_ts_data *ts)
{
	char data[10] = { 0 };
	int ret;

	//---set xdata index to EVENT BUF ADDR---
	data[0] = 0xFF;
	data[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	data[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, data, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set xdata index\n",
				__func__);
		return ret;
	}

	//---set dummy value---
	data[0] = EVENT_MAP_BLOCK_AREA;
	data[1] = 1;
	data[2] = 1;
	data[3] = 1;
	data[4] = 1;
	data[5] = 1;
	data[6] = 1;
	data[7] = 1;
	data[8] = 1;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, data, 9);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to set area\n",
				__func__);
		return ret;
	}

	return 0;
}

static int nvt_ts_clear_fw_status(struct nvt_ts_data *ts)
{
	u8 buf[8] = {0};
	int i = 0;
	const int retry = 20;

	for (i = 0; i < retry; i++) {
		//---set xdata index to EVENT BUF ADDR---
		buf[0] = 0xFF;
		buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		//---clear fw status---
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = 0x00;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);

		//---read fw status---
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;

		msleep(10);
	}

	if (i >= retry) {
		input_err(true, &ts->client->dev, "failed, i=%d, buf[1]=0x%02X\n", i, buf[1]);
		return -1;
	} else {
		return 0;
	}
}

static int nvt_ts_check_fw_status(struct nvt_ts_data *ts)
{
	u8 buf[8] = {0};
	int i = 0;
	const int retry = 50;

	for (i = 0; i < retry; i++) {
		//---set xdata index to EVENT BUF ADDR---
		buf[0] = 0xFF;
		buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		//---read fw status---
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);

		if ((buf[1] & 0xF0) == 0xA0)
			break;

		msleep(10);
	}

	if (i >= retry) {
		input_err(true, &ts->client->dev, "failed, i=%d, buf[1]=0x%02X\n", i, buf[1]);
		return -1;
	} else {
		return 0;
	}
}

static void nvt_ts_read_mdata(struct nvt_ts_data *ts, int *buff, u32 xdata_addr, s32 xdata_btn_addr)
{
	u8 buf[I2C_TANSFER_LENGTH + 1] = { 0 };
	u8 *rawdata_buf;
	u32 head_addr = 0;
	int dummy_len = 0;
	int data_len = 0;
	int residual_len = 0;
	int i, j, k;

	rawdata_buf = kzalloc(ts->platdata->x_num * ts->platdata->y_num * 2, GFP_KERNEL);
	if (!rawdata_buf)
		return;

	//---set xdata sector address & length---
	head_addr = xdata_addr - (xdata_addr % XDATA_SECTOR_SIZE);
	dummy_len = xdata_addr - head_addr;
	data_len = ts->platdata->x_num * ts->platdata->y_num * 2;
	residual_len = (head_addr + dummy_len + data_len) % XDATA_SECTOR_SIZE;

	input_info(true, &ts->client->dev, "head_addr=0x%05X, dummy_len=0x%05X, data_len=0x%05X, residual_len=0x%05X\n", head_addr, dummy_len, data_len, residual_len);

	//read data : step 1
	for (i = 0; i < ((dummy_len + data_len) / XDATA_SECTOR_SIZE); i++) {
		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = ((head_addr + XDATA_SECTOR_SIZE * i) >> 16) & 0xFF;
		buf[2] = ((head_addr + XDATA_SECTOR_SIZE * i) >> 8) & 0xFF;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		//---read xdata by I2C_TANSFER_LENGTH
		for (j = 0; j < (XDATA_SECTOR_SIZE / I2C_TANSFER_LENGTH); j++) {
			//---read xdata---
			buf[0] = I2C_TANSFER_LENGTH * j;
			nvt_ts_i2c_read(ts, I2C_FW_Address, buf, I2C_TANSFER_LENGTH + 1);

			//---copy buf to tmp---
			for (k = 0; k < I2C_TANSFER_LENGTH; k++) {
				rawdata_buf[XDATA_SECTOR_SIZE * i + I2C_TANSFER_LENGTH * j + k] = buf[k + 1];
				//printk("0x%02X, 0x%04X\n", buf[k+1], (XDATA_SECTOR_SIZE*i + I2C_TANSFER_LENGTH*j + k));
			}
		}
		//printk("addr=0x%05X\n", (head_addr+XDATA_SECTOR_SIZE*i));
	}

	//read xdata : step2
	if (residual_len != 0) {
		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = ((xdata_addr + data_len - residual_len) >> 16) & 0xFF;
		buf[2] = ((xdata_addr + data_len - residual_len) >> 8) & 0xFF;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		//---read xdata by I2C_TANSFER_LENGTH
		for (j = 0; j < (residual_len / I2C_TANSFER_LENGTH + 1); j++) {
			//---read xdata---
			buf[0] = I2C_TANSFER_LENGTH * j;
			nvt_ts_i2c_read(ts, I2C_FW_Address, buf, I2C_TANSFER_LENGTH + 1);

			//---copy buf to tmp---
			for (k = 0; k < I2C_TANSFER_LENGTH; k++) {
				rawdata_buf[(dummy_len + data_len - residual_len) + I2C_TANSFER_LENGTH * j + k] = buf[k + 1];
				//printk("0x%02X, 0x%04x\n", buf[k+1], ((dummy_len+data_len-residual_len) + I2C_TANSFER_LENGTH*j + k));
			}
		}
		//printk("addr=0x%05X\n", (xdata_addr+data_len-residual_len));
	}

	//---remove dummy data and 2bytes-to-1data---
	for (i = 0; i < (data_len / 2); i++)
		buff[i] = (s16)(rawdata_buf[dummy_len + i * 2] + 256 * rawdata_buf[dummy_len + i * 2 + 1]);

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

	kfree(rawdata_buf);
}

static void nvt_ts_change_mode(struct nvt_ts_data *ts, u8 mode)
{
	uint8_t buf[8] = {0};

	/* ---set xdata index to EVENT BUF ADDR--- */
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

	/* ---set mode--- */
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = mode;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);

	if (mode == NORMAL_MODE) {
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = HANDSHAKING_HOST_READY;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
		msleep(20);
	}
}

static u8 nvt_ts_get_fw_pipe(struct nvt_ts_data *ts)
{
	u8 buf[8]= {0};

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

	//---read fw status---
	buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
	nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);

	return (buf[1] & 0x01);
}

static int nvt_ts_hand_shake_status(struct nvt_ts_data *ts)
{
	u8 buf[8] = {0};
	const int retry = 50;
	int i;

	for (i = 0; i < retry; i++) {
		//---set xdata index to EVENT BUF ADDR---
		buf[0] = 0xFF;
		buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		//---read fw status---
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);

		if ((buf[1] == 0xA0) || (buf[1] == 0xA1))
			break;

		msleep(10);
	}

	if (i >= retry) {
		input_err(true, &ts->client->dev, "%s: failed to hand shake status, buf[1]=0x%02X\n",
			__func__, buf[1]);

		// Read back 5 bytes from offset EVENT_MAP_HOST_CMD for debug check
		buf[0] = 0xFF;
		buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		buf[0] = EVENT_MAP_HOST_CMD;
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 6);
		input_err(true, &ts->client->dev, "%s: read back 5 bytes from offset EVENT_MAP_HOST_CMD: "
			"0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
			__func__, buf[1], buf[2], buf[3], buf[4], buf[5]);

		return -EPERM;
	}

	return 0;
}

static int nvt_ts_switch_freqhops(struct nvt_ts_data *ts, u8 freqhop)
{
	u8 buf[8] = {0};
	u8 retry = 0;

	for (retry = 0; retry < 20; retry++) {
		//---set xdata index to EVENT BUF ADDR---
		buf[0] = 0xFF;
		buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		//---switch freqhop---
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = freqhop;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);

		msleep(35);

		buf[0] = EVENT_MAP_HOST_CMD;
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;
	}

	if (unlikely(retry == 20)) {
		input_err(true, &ts->client->dev, "%s: failed to switch freq hop 0x%02X, buf[1]=0x%02X\n",
			__func__, freqhop, buf[1]);
		return -EPERM;
	}

	return 0;
}

static void nvt_ts_print_buff(struct nvt_ts_data *ts, int *buff, int *min, int *max)
{
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	int offset;
	int i, j;
	int lsize = 7 * (ts->platdata->x_num + 1);

	pStr = kzalloc(lsize, GFP_KERNEL);
	if (!pStr)
		return;

	snprintf(pTmp, sizeof(pTmp), "  X   ");
	strlcat(pStr, pTmp, lsize);

	for (i = 0; i < ts->platdata->x_num; i++) {
		snprintf(pTmp, sizeof(pTmp), "  %02d  ", i);
		strlcat(pStr, pTmp, lsize);
	}

	input_raw_info(true, &ts->client->dev, "%s\n", pStr);
	memset(pStr, 0x0, lsize);
	snprintf(pTmp, sizeof(pTmp), " +");
	strlcat(pStr, pTmp, lsize);

	for (i = 0; i < ts->platdata->x_num; i++) {
		snprintf(pTmp, sizeof(pTmp), "------");
		strlcat(pStr, pTmp, lsize);
	}

	input_raw_info(true, &ts->client->dev, "%s\n", pStr);

	for (i = 0; i < ts->platdata->y_num; i++) {
		memset(pStr, 0x0, lsize);
		snprintf(pTmp, sizeof(pTmp), "Y%02d |", i);
		strlcat(pStr, pTmp, lsize);

		for (j = 0; j < ts->platdata->x_num; j++) {
			offset = i * ts->platdata->x_num + j;

			*min = MIN(*min, buff[offset]);
			*max = MAX(*max, buff[offset]);

			snprintf(pTmp, sizeof(pTmp), " %5d", buff[offset]);

			strlcat(pStr, pTmp, lsize);
		}

		input_raw_info(true, &ts->client->dev, "%s\n", pStr);
	}

	kfree(pStr);
}

static void nvt_ts_print_gap_buff(struct nvt_ts_data *ts, int *buff, int *min, int *max)
{
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	int offset;
	int i, j;
	int lsize = 7 * (ts->platdata->x_num + 1);

	pStr = kzalloc(lsize, GFP_KERNEL);
	if (!pStr)
		return;

	snprintf(pTmp, sizeof(pTmp), "  X   ");
	strlcat(pStr, pTmp, lsize);

	for (i = 0; i < ts->platdata->x_num - 2; i++) {
		snprintf(pTmp, sizeof(pTmp), "  %02d  ", i);
		strlcat(pStr, pTmp, lsize);
	}

	input_raw_info(true, &ts->client->dev, "%s\n", pStr);
	memset(pStr, 0x0, lsize);
	snprintf(pTmp, sizeof(pTmp), " +");
	strlcat(pStr, pTmp, lsize);

	for (i = 0; i < ts->platdata->x_num - 2; i++) {
		snprintf(pTmp, sizeof(pTmp), "------");
		strlcat(pStr, pTmp, lsize);
	}

	input_raw_info(true, &ts->client->dev, "%s\n", pStr);

	for (i = 0; i < ts->platdata->y_num; i++) {
		memset(pStr, 0x0, lsize);
		snprintf(pTmp, sizeof(pTmp), "Y%02d |", i);
		strlcat(pStr, pTmp, lsize);

		for (j = 1; j < ts->platdata->x_num - 1; j++) {
			offset = i * ts->platdata->x_num + j;

			*min = MIN(*min, buff[offset]);
			*max = MAX(*max, buff[offset]);

			snprintf(pTmp, sizeof(pTmp), " %5d", buff[offset]);

			strlcat(pStr, pTmp, lsize);
		}

		input_raw_info(true, &ts->client->dev, "%s\n", pStr);
	}

	kfree(pStr);
}

static int nvt_ts_noise_read(struct nvt_ts_data *ts, int *min_buff, int *max_buff)
{
	u8 buf[8] = { 0 };
	int frame_num;
	u32 x, y;
	int offset;
	int ret;
	int *rawdata_buf;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	ret = nvt_ts_switch_freqhops(ts, FREQ_HOP_DISABLE);
	if (ret)
		return ret;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	msleep(100);

	//---Enter Test Mode---
	ret = nvt_ts_clear_fw_status(ts);
	if (ret)
		return ret;

	frame_num = ts->platdata->diff_test_frame / 10;
	if (frame_num <= 0)
		frame_num = 1;

	input_raw_info(true, &ts->client->dev, "%s: frame_num %d\n", __func__, frame_num);

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

	//---enable noise collect---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x47;
	buf[2] = 0xAA;
	buf[3] = frame_num;
	buf[4] = 0x00;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 5);

	// need wait PS_Config_Diff_Test_Frame * 8.3ms
	msleep(frame_num * 83);

	ret = nvt_ts_hand_shake_status(ts);
	if (ret)
		return ret;

	rawdata_buf = kzalloc(ts->platdata->x_num * ts->platdata->y_num * 2 * sizeof(int), GFP_KERNEL);
	if (!rawdata_buf)
		return -ENOMEM;

	if (!nvt_ts_get_fw_pipe(ts))
		nvt_ts_read_mdata(ts, rawdata_buf, ts->mmap->DIFF_PIPE0_ADDR, ts->mmap->DIFF_BTN_PIPE0_ADDR);
	else
		nvt_ts_read_mdata(ts, rawdata_buf, ts->mmap->DIFF_PIPE1_ADDR, ts->mmap->DIFF_BTN_PIPE1_ADDR);

	for (y = 0; y < ts->platdata->y_num; y++) {
		for (x = 0; x < ts->platdata->x_num; x++) {
			offset = y * ts->platdata->x_num + x;
			max_buff[offset] = (s8)((rawdata_buf[offset] >> 8) & 0xFF);
			min_buff[offset] = (s8)(rawdata_buf[offset] & 0xFF);
		}
	}

	kfree(rawdata_buf);

	//---Leave Test Mode---
	nvt_ts_change_mode(ts, NORMAL_MODE);

	input_raw_info(true, &ts->client->dev, "%s\n", __func__);

	return 0;
}

static int nvt_ts_ccdata_read(struct nvt_ts_data *ts, int *buff)
{
	u32 x, y;
	int offset;
	int ret;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	ret = nvt_ts_switch_freqhops(ts, FREQ_HOP_DISABLE);
	if (ret)
		return ret;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	msleep(100);

	//---Enter Test Mode---
	ret = nvt_ts_clear_fw_status(ts);
	if (ret)
		return ret;

	nvt_ts_change_mode(ts, MP_MODE_CC);

	ret = nvt_ts_check_fw_status(ts);
	if (ret)
		return ret;

	if (!nvt_ts_get_fw_pipe(ts))
		nvt_ts_read_mdata(ts, buff, ts->mmap->DIFF_PIPE1_ADDR, ts->mmap->DIFF_BTN_PIPE1_ADDR);
	else
		nvt_ts_read_mdata(ts, buff, ts->mmap->DIFF_PIPE0_ADDR, ts->mmap->DIFF_BTN_PIPE0_ADDR);

	for (y = 0; y < ts->platdata->y_num; y++) {
		for (x = 0; x < ts->platdata->x_num; x++) {
			offset = y * ts->platdata->x_num + x;
			buff[offset] = (u16)buff[offset];
		}
	}

	//---Leave Test Mode---
	nvt_ts_change_mode(ts, NORMAL_MODE);

	input_raw_info(true, &ts->client->dev, "%s\n", __func__);

	return 0;
}

static int nvt_ts_rawdata_read(struct nvt_ts_data *ts, int *buff)
{
	int offset;
	int x, y;
	int ret;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	ret = nvt_ts_switch_freqhops(ts, FREQ_HOP_DISABLE);
	if (ret)
		return ret;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	msleep(100);

	//---Enter Test Mode---
	ret = nvt_ts_clear_fw_status(ts);
	if (ret)
		return ret;

	nvt_ts_change_mode(ts, MP_MODE_CC);

	ret = nvt_ts_check_fw_status(ts);
	if (ret)
		return ret;

	nvt_ts_read_mdata(ts, buff, ts->mmap->BASELINE_ADDR, ts->mmap->BASELINE_BTN_ADDR);

	for (y = 0; y < ts->platdata->y_num; y++) {
		for (x = 0; x < ts->platdata->x_num; x++) {
			offset = y * ts->platdata->x_num + x;
			buff[offset] = (s16)buff[offset];
		}
	}

	//---Leave Test Mode---
	nvt_ts_change_mode(ts, NORMAL_MODE);

	input_raw_info(true, &ts->client->dev, "%s\n", __func__);

	return 0;
}

static int nvt_ts_short_read(struct nvt_ts_data *ts, int *buff)
{
	u8 buf[128] = { 0 };
	u8 *rawdata_buf;
	u32 raw_pipe_addr;
	u32 x, y;
	int offset;
	int ret;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	ret = nvt_ts_switch_freqhops(ts, FREQ_HOP_DISABLE);
	if (ret)
		return ret;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	msleep(100);

	//---Enter Test Mode---
	ret = nvt_ts_clear_fw_status(ts);
	if (ret)
		return ret;

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

	//---enable short test---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x43;
	buf[2] = 0xAA;
	buf[3] = 0x02;
	buf[4] = 0x00;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 5);

	ret = nvt_ts_hand_shake_status(ts);
	if (ret)
		return ret;

	rawdata_buf = kzalloc(ts->platdata->x_num * ts->platdata->y_num * 2, GFP_KERNEL);
	if (!rawdata_buf)
		return -ENOMEM;

	if (nvt_ts_get_fw_pipe(ts) == 0)
		raw_pipe_addr = ts->mmap->RAW_PIPE0_ADDR;
	else
		raw_pipe_addr = ts->mmap->RAW_PIPE1_ADDR;

	for (y = 0; y < ts->platdata->y_num; y++) {
		offset = y * ts->platdata->x_num * 2;
		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = (u8)(((raw_pipe_addr + offset) >> 16) & 0xFF);
		buf[2] = (u8)(((raw_pipe_addr + offset) >> 8) & 0xFF);
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		buf[0] = (u8)((raw_pipe_addr + offset) & 0xFF);
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, ts->platdata->x_num * 2 + 1);

		memcpy(rawdata_buf + offset, buf + 1, ts->platdata->x_num * 2);
	}

	for (y = 0; y < ts->platdata->y_num; y++) {
		for (x = 0; x < ts->platdata->x_num; x++) {
			offset = y * ts->platdata->x_num + x;
			buff[offset] = (s16)(rawdata_buf[offset * 2]
					+ 256 * rawdata_buf[offset * 2 + 1]);
		}
	}

	kfree(rawdata_buf);

	//---Leave Test Mode---
	nvt_ts_change_mode(ts, NORMAL_MODE);

	input_raw_info(true, &ts->client->dev, "%s: Raw_Data_Short\n", __func__);

	return 0;
}

static int nvt_ts_open_read(struct nvt_ts_data *ts, int *buff)
{
	u8 buf[128] = { 0 };
	u8 *rawdata_buf;
	u32 raw_pipe_addr;
	u32 x, y;
	int offset;
	int ret;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	ret = nvt_ts_switch_freqhops(ts, FREQ_HOP_DISABLE);
	if (ret)
		return ret;

	ret = nvt_ts_check_fw_reset_state(ts, RESET_STATE_NORMAL_RUN);
	if (ret)
		return ret;

	msleep(100);

	//---Enter Test Mode---
	ret = nvt_ts_clear_fw_status(ts);
	if (ret)
		return ret;

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

	//---enable open test---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x45;
	buf[2] = 0xAA;
	buf[3] = 0x02;
	buf[4] = 0x00;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 5);

	ret = nvt_ts_hand_shake_status(ts);
	if (ret)
		return ret;

	rawdata_buf = kzalloc(ts->platdata->x_num * ts->platdata->y_num * 2, GFP_KERNEL);
	if (!rawdata_buf)
		return -ENOMEM;

	if (!nvt_ts_get_fw_pipe(ts))
		raw_pipe_addr = ts->mmap->RAW_PIPE0_ADDR;
	else
		raw_pipe_addr = ts->mmap->RAW_PIPE1_ADDR;

	for (y = 0; y < ts->platdata->y_num; y++) {
		offset = y * ts->platdata->x_num * 2;
		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = (uint8_t)(((raw_pipe_addr + offset) >> 16) & 0xFF);
		buf[2] = (uint8_t)(((raw_pipe_addr + offset) >> 8) & 0xFF);
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

		buf[0] = (uint8_t)((raw_pipe_addr + offset) & 0xFF);
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, ts->platdata->x_num  * 2 + 1);

		memcpy(rawdata_buf + offset, buf + 1, ts->platdata->x_num * 2);
	}

	for (y = 0; y < ts->platdata->y_num; y++) {
		for (x = 0; x < ts->platdata->x_num; x++) {
			offset = y * ts->platdata->x_num + x;
			buff[offset] = (s16)((rawdata_buf[(offset) * 2]
					+ 256 * rawdata_buf[(offset) * 2 + 1]));
		}
	}

	kfree(rawdata_buf);

	//---Leave Test Mode---
	nvt_ts_change_mode(ts, NORMAL_MODE);

	input_raw_info(true, &ts->client->dev, "%s\n", __func__);

	return 0;
}

static int nvt_ts_open_test(struct nvt_ts_data *ts)
{
	int *raw_buff;
	int ret = 0;
	int min, max;

	min = 0x7FFFFFFF;
	max = 0x80000000;

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff)
		return -ENOMEM;

	ret = nvt_ts_open_read(ts, raw_buff);
	if (ret)
		goto out;

	nvt_ts_print_buff(ts, raw_buff, &min, &max);

	if (min < ts->platdata->open_test_spec[0])
		ret = -EPERM;

	if (max > ts->platdata->open_test_spec[1])
		ret = -EPERM;

	input_raw_info(true, &ts->client->dev, "%s: min(%d,%d), max(%d,%d)",
		__func__, min, ts->platdata->open_test_spec[0],
		max, ts->platdata->open_test_spec[1]);
out:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	return ret;
}

static int nvt_ts_short_test(struct nvt_ts_data *ts)
{
	int *raw_buff;
	int ret = 0;
	int min, max;

	min = 0x7FFFFFFF;
	max = 0x80000000;

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff)
		return -ENOMEM;

	ret = nvt_ts_short_read(ts, raw_buff);
	if (ret)
		goto out;

	nvt_ts_print_buff(ts, raw_buff, &min, &max);

	if (min < ts->platdata->short_test_spec[0])
		ret = -EPERM;

	if (max > ts->platdata->short_test_spec[1])
		ret = -EPERM;

	input_raw_info(true, &ts->client->dev, "%s: min(%d,%d), max(%d,%d)",
		__func__, min, ts->platdata->short_test_spec[0],
		max, ts->platdata->short_test_spec[1]);
out:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	return ret;
}

#define NVT_SRAM_TEST_PATTERN_SIZE 4
static int nvt_ts_nt36523_sram_test(struct nvt_ts_data *ts)
{
	u8 test_pattern[NVT_SRAM_TEST_PATTERN_SIZE] = {0xFF, 0x00, 0x55, 0xAA};
	u8 buf[10];
	int ret = 0, result = 0;
	int i;

	//---SRAM TEST (ICM) ---

	// SW Reset & Idle
	nvt_ts_sw_reset_idle(ts);

	//---set xdata index to SRAM Test Registers---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->MBT_MUX_CTL0 >> 16) & 0xFF;
	buf[2] = (ts->mmap->MBT_MUX_CTL0 >> 8) & 0xFF;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);
	if (ret < 0)
		goto out;

	//---set control registers---
	buf[0] = (u8)(ts->mmap->MBT_MUX_CTL0 & 0xFF);
	buf[1] = 0x18;
	buf[2] = 0x5B;
	buf[3] = 0x52;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 4);
	if (ret < 0)
		goto out;

	//---set mode registers---
	buf[0] = (u8)(ts->mmap->MBT_MODE & 0xFF);
	buf[1] = 0x00;	
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
	if (ret < 0)
		goto out;

	for (i = 0; i < NVT_SRAM_TEST_PATTERN_SIZE; i++) {
		//---set test pattern---
		buf[0] = (u8)(ts->mmap->MBT_DB & 0xFF);
		buf[1] = test_pattern[i];
		ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
		if (ret < 0)
			goto out;

		//---set operation as WRITE test---
		buf[0] = (u8)(ts->mmap->MBT_OP & 0xFF);
		buf[1] = 0xE1;
		ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
		if (ret < 0)
			goto out;

		//---enable test---
		buf[0] = (u8)(ts->mmap->MBT_EN & 0xFF);
		buf[1] = 0x01;
		ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
		if (ret < 0)
			goto out;

		msleep(100);

		//---get test result---
		buf[0] = (u8)(ts->mmap->MBT_STATUS & 0xFF);
		buf[1] = 0;
		ret = nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);
		if (ret < 0)
			goto out;

		if (buf[1] != 0x02) {
			input_info(true, &ts->client->dev,
					"%s: ICM WRITE fail at pattern=0x%02X, MBT_STATUS=0x%02X\n",
					__func__, test_pattern[i], buf[1]);
			result = -EIO;
		}

		//---set operation as READ test---
		buf[0] = (u8)(ts->mmap->MBT_OP & 0xFF);
		buf[1] = 0xA1;
		ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
		if (ret < 0)
			goto out;

		//---enable test---
		buf[0] = (u8)(ts->mmap->MBT_EN & 0xFF);
		buf[1] = 0x01;
		ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
		if (ret < 0)
			goto out;

		msleep(100);

		//---get test result---
		buf[0] = (u8)(ts->mmap->MBT_STATUS & 0xFF);
		buf[1] = 0x00;
		ret = nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);
		if (ret < 0)
			goto out;

		if (buf[1] != 0x02) {
			input_info(true, &ts->client->dev,
					"%s: ICM READ fail at pattern=0x%02X, MBT_STATUS=0x%02X\n",
					__func__, test_pattern[i], buf[1]);
			result = -EIO;
		}
	}

	//---SRAM TEST (ICS) ---

	// SW Reset & Idle
	nvt_ts_sw_reset_idle(ts);
	
	//---set ICS DMA RX speed to 1X---
	buf[0] = 0x00;	//dummy
	buf[1] = 0x00;
	ret = nvt_ts_nt36523_ics_i2c_write(ts, ts->mmap->RX_DMA_2X_EN, buf, 2);
	if (ret < 0)
		goto out;

	//---set ICM DMA TX speed to 1X---
	ret = nvt_ts_set_page(ts, I2C_FW_Address, ts->mmap->TX_DMA_2X_EN);
	if (ret < 0)
		goto out;

	buf[0] = (u8)(ts->mmap->TX_DMA_2X_EN & 0xFF);
	buf[1] = 0x00;
	ret = nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);
	if (ret < 0)
		goto out;

	msleep(10);

	//---set control registers---
	buf[0] = 0x00;	//dummy
	buf[1] = 0x18;
	buf[2] = 0x5B;
	buf[3] = 0x52;
	ret = nvt_ts_nt36523_ics_i2c_write(ts, ts->mmap->MBT_MUX_CTL0, buf, 4);
	if (ret < 0)
		goto out;

	//---set mode registers---	
	buf[0] = 0x00;	//dummy
	buf[1] = 0x00;
	ret = nvt_ts_nt36523_ics_i2c_write(ts, ts->mmap->MBT_MODE, buf, 2);
	if (ret < 0)
		goto out;

	for (i = 0; i < NVT_SRAM_TEST_PATTERN_SIZE; i++) {
		//---set test pattern---
		buf[0] = 0x00;	//dummy
		buf[1] = test_pattern[i];
		ret = nvt_ts_nt36523_ics_i2c_write(ts, ts->mmap->MBT_DB, buf, 2);
		if (ret < 0)
			goto out;

		//---set operation as WRITE test---
		buf[0] = 0x00;	//dummy
		buf[1] = 0xE1;
		ret = nvt_ts_nt36523_ics_i2c_write(ts, ts->mmap->MBT_OP, buf, 2);
		if (ret < 0)
			goto out;

		//---enable test---
		buf[0] = 0x00;	//dummy
		buf[1] = 0x01;
		ret = nvt_ts_nt36523_ics_i2c_write(ts, ts->mmap->MBT_EN, buf, 2);
		if (ret < 0)
			goto out;

		msleep(100);

		//---get test result---
		buf[0] = 0x00;	//dummy
		buf[1] = 0x00;
		ret = nvt_ts_nt36523_ics_i2c_read(ts, ts->mmap->MBT_STATUS, buf, 2);
		if (ret < 0)
			goto out;

		if (buf[1] != 0x02) {			
			input_info(true, &ts->client->dev,
					"%s: ICS WRITE fail at pattern=0x%02X, MBT_STATUS=0x%02X\n",
					__func__, test_pattern[i], buf[1]);
			result = -EIO;
		}

		//---set operation as READ test---
		buf[0] = 0x00;	//dummy
		buf[1] = 0xA1;	
		ret = nvt_ts_nt36523_ics_i2c_write(ts, ts->mmap->MBT_OP, buf, 2);
		if (ret < 0)
			goto out;

		//---enable test---
		buf[0] = 0x00;	//dummy
		buf[1] = 0x01;
		ret = nvt_ts_nt36523_ics_i2c_write(ts, ts->mmap->MBT_EN, buf, 2);
		if (ret < 0)
			goto out;

		msleep(100);

		//---get test result---
		buf[0] = 0x00;	//dummy
		buf[1] = 0x00;
		ret = nvt_ts_nt36523_ics_i2c_read(ts, ts->mmap->MBT_STATUS, buf, 2);
		if (ret < 0)
			goto out;

		if (buf[1] != 0x02) {
			input_info(true, &ts->client->dev,
					"%s: ICS READ fail at pattern=0x%02X, MBT_STATUS=0x%02X\n",
					__func__, test_pattern[i], buf[1]);
			result = -EIO;
		}	
	}

out:
	if (ret < 0)
		result = -EIO;

	input_info(true, &ts->client->dev,
			"%s: %s, %d\n", __func__, result == 0 ? "pass" : "fail", ret);

	//---Reset IC---
	//nvt_ts_bootloader_reset(ts);

	return result;
}

int nvt_ts_mode_read(struct nvt_ts_data *ts)
{
	u8 buf[3] = {0};
	int mode_masked;

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR>> 8) & 0xFF;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

	//---read cmd status---
	buf[0] = EVENT_MAP_FUNCT_STATE;
	nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 3);

	mode_masked = (buf[2] << 8 | buf[1]) & FUNCT_ALL_MASK;

	input_info(true, &ts->client->dev, "%s: 0x%02X%02X, masked:0x%04X\n",
			__func__, buf[2], buf[1], mode_masked);

	ts->noise_mode = (mode_masked & NOISE_MASK) ? 1 : 0;

	return mode_masked;
}

static int nvt_ts_mode_switch(struct nvt_ts_data *ts, u8 cmd, bool stored)
{
	int i, retry = 5;
	u8 buf[3] = { 0 };

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR>> 8) & 0xFF;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

	for (i = 0; i < retry; i++) {
		//---set cmd---
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = cmd;
		nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 2);

		usleep_range(15000, 16000);

		//---read cmd status---
		buf[0] = EVENT_MAP_HOST_CMD;
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;
	}

	if (unlikely(i == retry)) {
		input_err(true, &ts->client->dev, "failed to switch 0x%02X mode - 0x%02X\n", cmd, buf[1]);
		return -EIO;
	}

	if (stored) {
		msleep(10);

		ts->sec_function = nvt_ts_mode_read(ts);
	}

	input_err(true, &ts->client->dev, "%s: 0x%02X\n", __func__, cmd);
	return 0;
}

static int nvt_ts_mode_switch_extended(struct nvt_ts_data *ts, u8 *cmd, u8 len, bool stored)
{
	int i, retry = 5;
	u8 buf[4] = { 0 };

//	input_info(true, &ts->client->dev, "0x%02X - 0x%02X - 0x%02X, %d\n", cmd[0], cmd[1], cmd[2], len);

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = ((ts->mmap->EVENT_BUF_ADDR | cmd[0]) >> 16) & 0xFF;
	buf[2] = ((ts->mmap->EVENT_BUF_ADDR | cmd[0]) >> 8) & 0xFF;
	nvt_ts_i2c_write(ts, I2C_FW_Address, buf, 3);

	for (i = 0; i < retry; i++) {
		//---set cmd---
		nvt_ts_i2c_write(ts, I2C_FW_Address, cmd, len);

		usleep_range(15000, 16000);

		//---read cmd status---
		buf[0] = cmd[0];
		nvt_ts_i2c_read(ts, I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;
		else
			input_err(true, &ts->client->dev, "%s, retry:%d, buf[1]:0x%x\n", __func__, i, buf[1]);
	}

	if (unlikely(i == retry)) {
		input_err(true, &ts->client->dev, "failed to switch mode - 0x%02X 0x%02X\n", cmd[0], cmd[1]);
		return -EIO;
	}

	if (stored) {
		msleep(20);
		ts->sec_function = nvt_ts_mode_read(ts);
	}

	input_err(true, &ts->client->dev, "%s: 0x%02X 0x%02X\n", __func__, cmd[0], cmd[1]);
	return 0;
}

int nvt_ts_mode_restore(struct nvt_ts_data *ts)
{
	u16 func_need_switch;
	u8 cmd;
	u8 cmd_list[4] = {0};
	int i;
	int ret = 0;

	func_need_switch = ts->sec_function ^ nvt_ts_mode_read(ts);

	if (!func_need_switch)
		goto out;

	for (i = GLOVE; i < FUNCT_MAX; i++) {
		if ((func_need_switch >> i) & 0x01) {
			cmd = 0;
			switch(i) {
			case GLOVE:
				if (ts->sec_function & GLOVE_MASK)
					cmd = GLOVE_ENTER;
				else
					cmd = GLOVE_LEAVE;
				break;
			case HIGH_SENSITIVITY:
				if (ts->sec_function & HIGH_SENSITIVITY_MASK) {
					cmd_list[0] = EVENT_MAP_HOST_CMD;
					cmd_list[1] = EXTENDED_CUSTOMIZED_CMD;
					cmd_list[2] = SET_HIGH_SENSITIVITY_MODE;
					cmd_list[3] = HIGH_SENSITIVITY_ENABLE;
				} else {
					cmd_list[0] = EVENT_MAP_HOST_CMD;
					cmd_list[1] = EXTENDED_CUSTOMIZED_CMD;
					cmd_list[2] = SET_HIGH_SENSITIVITY_MODE;
					cmd_list[3] = HIGH_SENSITIVITY_DISABLE;
				}
				break;
/*
			case EDGE_REJECT_L:
				i++;
			case EDGE_REJECT_H:
				switch((ts->sec_function & EDGE_REJECT_MASK) >> EDGE_REJECT_L) {
					case EDGE_REJ_LEFT_UP:
						cmd = EDGE_REJ_LEFT_UP_MODE;
						break;
					case EDGE_REJ_RIGHT_UP:
						cmd = EDGE_REJ_RIGHT_UP_MODE;
						break;
					default:
						cmd = EDGE_REJ_VERTICLE_MODE;
				}
				break;
			case EDGE_PIXEL:
				if (ts->sec_function & EDGE_PIXEL_MASK)
					cmd = EDGE_AREA_ENTER;
				else
					cmd = EDGE_AREA_LEAVE;
				break;
			case HOLE_PIXEL:
				if (ts->sec_function & HOLE_PIXEL_MASK)
					cmd = HOLE_AREA_ENTER;
				else
					cmd = HOLE_AREA_LEAVE;
				break;
			case SPAY_SWIPE:
				if (ts->sec_function & SPAY_SWIPE_MASK)
					cmd = SPAY_SWIPE_ENTER;
				else
					cmd = SPAY_SWIPE_LEAVE;
				break;
			case DOUBLE_CLICK:
				if (ts->sec_function & DOUBLE_CLICK_MASK)
					cmd = DOUBLE_CLICK_ENTER;
				else
					cmd = DOUBLE_CLICK_LEAVE;
				break;
			case SENSITIVITY_TEST:
				if (ts->sec_function & SENSITIVITY_TEST_MASK)
					cmd = SENSITIVITY_TEST_ENTER;
				else
					cmd = SENSITIVITY_TEST_LEAVE;
				break;
*/
			case BLOCK_AREA:
				if (ts->touchable_area && ts->sec_function & BLOCK_AREA_MASK)
					cmd = BLOCK_AREA_ENTER;
				else
					cmd = BLOCK_AREA_LEAVE;
				break;
			default:
				continue;
			}

			if (ts->touchable_area) {
				ret = nvt_ts_set_touchable_area(ts);
				if (ret < 0)
					cmd = BLOCK_AREA_LEAVE;
			}

			if (cmd) {
				ret = nvt_ts_mode_switch(ts, cmd, false);
				if (ret)
					input_info(true, &ts->client->dev, "%s: failed to restore %X\n", __func__, cmd);
			} else {
				ret = nvt_ts_mode_switch_extended(ts, cmd_list, 4, false);
				if (ret)
					input_info(true, &ts->client->dev, "%s: failed to restore %X %X %X\n", __func__, cmd_list[1], cmd_list[2], cmd_list[3]);
			}
		}
	}
	input_info(true, &ts->client->dev, "%s: 0x%X\n", __func__, func_need_switch);

	if (ts->setgrip_restore_data[0] == 1) {
		input_info(true, &ts->client->dev, "%s restore set grip portrait_mode, %d, %d, %d, %d\n",
				__func__, ts->setgrip_restore_data[1], ts->setgrip_restore_data[2],
				ts->setgrip_restore_data[3], ts->setgrip_restore_data[4]);
		nvt_ts_set_grip_portrait_mode(ts, ts->setgrip_restore_data);
	} else if (ts->setgrip_restore_data[0] == 2) {
		input_info(true, &ts->client->dev, "%s restore set grip landscape_mode, %d, %d, %d, %d\n",
				__func__, ts->setgrip_restore_data[1], ts->setgrip_restore_data[2],
				ts->setgrip_restore_data[3], ts->setgrip_restore_data[4]);
		nvt_ts_set_grip_landscape_mode(ts, ts->setgrip_restore_data);
	}

	if ((ts->grip_edgehandler_restore_data[0] == 0) && (ts->grip_edgehandler_restore_data[1] != 0) && (ts->grip_edgehandler_restore_data[1] < 3)) {
		input_info(true, &ts->client->dev, "%s restore set grip exeiption zone, %d, %d, %d\n",
				__func__, ts->grip_edgehandler_restore_data[1], ts->grip_edgehandler_restore_data[2],
				ts->grip_edgehandler_restore_data[3]);
		nvt_ts_set_grip_exception_zone(ts, ts->grip_edgehandler_restore_data);
	}

out:
	return ret;
}

static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	switch (sec->cmd_param[0]) {
	case BUILT_IN:
		ret = nvt_ts_fw_update_from_bin(ts);
		break;
	case UMS:
#if defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		ret = nvt_ts_fw_update_from_external(ts, TSP_PATH_EXTERNAL_FW_SIGNED);
#else
		ret = nvt_ts_fw_update_from_external(ts, TSP_PATH_EXTERNAL_FW);
#endif
		break;
	case SPU:
		ret = nvt_ts_fw_update_from_external(ts, TSP_PATH_SPU_FW_SIGNED);
		break;
	default:
		input_err(true, &ts->client->dev, "%s: Not support command[%d]\n",
			__func__, sec->cmd_param[0]);
		ret = -EINVAL;
		break;
	}

	if (ret)
		goto out;

	nvt_ts_get_fw_info(ts);

	ts->sec_function = nvt_ts_mode_read(ts);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

out:
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "NO%02X%02X%02X%02X",
		ts->fw_ver_bin[0], ts->fw_ver_bin[1], ts->fw_ver_bin[2], ts->fw_ver_bin[3]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_BIN");

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char data[4] = { 0 };
	char model[16] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out_power_off;
	}

	/* ic_name, project_id, panel, fw info */
	ret = nvt_ts_get_fw_info(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvt_ts_get_fw_info fail!\n", __func__);
		goto out;
	}
	data[0] = ts->fw_ver_ic[0];
	data[1] = ts->fw_ver_ic[1];
	data[2] = ts->fw_ver_ic[2];
	data[3] = ts->fw_ver_ic[3];

	snprintf(buff, sizeof(buff), "NO%02X%02X%02X%02X", data[0], data[1], data[2], data[3]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	snprintf(model, sizeof(model), "NO%02X%02X", data[0], data[1]);

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
		sec_cmd_set_cmd_result_all(sec, model, strnlen(model, sizeof(model)), "FW_MODEL");
	}

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

out:
	nvt_ts_bootloader_reset(ts);
out_power_off:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "NOVATEK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_VENDOR");

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s", "NT36523");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_NAME");

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_x_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%d", ts->platdata->x_num);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_y_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%d", ts->platdata->y_num);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_checksum_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 csum_result[BLOCK_64KB_NUM * 4 + 1] = { 0 };

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	// get firmware data checksum in flash ic 
	if (nvt_ts_get_checksum(ts, csum_result, sizeof(csum_result))) {
		input_err(true, &ts->client->dev, "%s: nvt_ts_get_checksum fail!\n", __func__);
		goto err;
	}

	nvt_ts_bootloader_reset(ts);

	snprintf(buff, sizeof(buff), "%s", csum_result);
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

err:
	nvt_ts_bootloader_reset(ts);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s", "60");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void check_connection(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	ret = nvt_ts_open_test(ts);

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", ret ? "NG" : "OK");

	sec->cmd_state =  ret ? SEC_CMD_STATUS_FAIL : SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

/*
 *	cmd_param
 *		[0], 0 disable high sensitivity mode
 *		     1 enable high sensitivity mode
 */
static void high_sensitivity_mode(struct sec_cmd_data *sec)
{
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int reti;
	u8 buf[4];

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (sec->cmd_param[0] < HIGH_SENSITIVITY_DISABLE || sec->cmd_param[0] > HIGH_SENSITIVITY_ENABLE) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	}

	input_info(true, &ts->client->dev, "%s: %s touch high sensitivity mode, cmd_param=%d\n",
		__func__, sec->cmd_param[0] ? "enable" : "disable", sec->cmd_param[0]);

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = EXTENDED_CUSTOMIZED_CMD;
	buf[2] = SET_HIGH_SENSITIVITY_MODE;
	buf[3] = (u8)sec->cmd_param[0];
	reti = nvt_ts_mode_switch_extended(ts, buf, 4, false);
	if (reti) {
		input_err(true, &ts->client->dev, "%s failed to switch high sensitivity mode - 0x%02X\n", __func__, buf[3]);
		mutex_unlock(&ts->lock);
		goto out;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

/*
 *	cmd_param
 *		[0], 0 disable glove mode
 *		     1 enable glove mode
 */
static void nvt_ts_glove_mode(struct sec_cmd_data *sec)
{
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;
	u8 mode;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	} else {
		mode = sec->cmd_param[0] ? GLOVE_ENTER : GLOVE_LEAVE;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	ret = nvt_ts_mode_switch(ts, mode, true);
	if (ret) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void glove_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	if (ts->platdata->enable_glove_mode) {
		nvt_ts_glove_mode(sec);
	} else {
		high_sensitivity_mode(sec);
	}
}

static void set_note_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 mode;
	u8 buf[4];
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	}

	if (sec->cmd_param[0] == 1)
		mode = PALM_SUPPORT_STYLUS;
	else
		mode = PALM_NORMAL;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	input_info(true, &ts->client->dev, "%s: change palm mode to %d\n", __func__, mode);

	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = EXTENDED_CUSTOMIZED_CMD;
	buf[2] = SET_PALM_MODE;
	buf[3] = mode;
	ret = nvt_ts_mode_switch_extended(ts, buf, 4, false);
	if (ret < 0) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void set_sip_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 mode;
	u8 buf[4];
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (sec->cmd_param[0] < DEBOUNCE_NORMAL || sec->cmd_param[0] > DEBOUNCE_LOWER) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	}

	mode = sec->cmd_param[0] & 0xFF;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	input_info(true, &ts->client->dev, "%s: use %s touch debounce\n",
			__func__, mode ? "lower" : "normal");

	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = EXTENDED_CUSTOMIZED_CMD;
	buf[2] = SET_TOUCH_DEBOUNCE;
	buf[3] = mode;
	ret = nvt_ts_mode_switch_extended(ts, buf, 4, false);
	if (ret < 0) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

/*	for game mode 
	byte[0]: Setting for the Game Mode
		- 0: Disable
		- 1: Enable
*/
static void set_game_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 mode = 0;
	u8 buf[4] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (sec->cmd_param[0] < GAME_MODE_DISABLE || sec->cmd_param[0] > GAME_MODE_ENABLE) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	}

	mode = sec->cmd_param[0];

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	input_info(true, &ts->client->dev, "%s: %s\n",
			__func__, mode ? "enable" : "disable");

	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = EXTENDED_CUSTOMIZED_CMD;
	buf[2] = SET_GAME_MODE;
	buf[3] = mode;
	ret = nvt_ts_mode_switch_extended(ts, buf, 4, false);
	if (ret < 0) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void dead_zone_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 mode;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	} else {
		mode = sec->cmd_param[0] ? EDGE_AREA_ENTER : EDGE_AREA_LEAVE;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	ret = nvt_ts_mode_switch(ts, mode, true);
	if (ret) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}
/*
static void spay_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 mode;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	} else {
		mode = sec->cmd_param[0] ? SPAY_SWIPE_ENTER : SPAY_SWIPE_LEAVE;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	ret = nvt_ts_mode_switch(ts, mode, true);
	if (ret) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}
*/
static void set_touchable_area(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 mode;

	sec_cmd_set_default_result(sec);

	/* temp block until tuning */
	input_err(true, &ts->client->dev, "%s: temp block until tuning\n", __func__);
	goto out;

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	} else {
		mode = sec->cmd_param[0] ? BLOCK_AREA_ENTER: BLOCK_AREA_LEAVE;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	if (mode == BLOCK_AREA_ENTER) {
		ret = nvt_ts_set_touchable_area(ts);
		if (ret < 0)
			goto err;
	} else if (mode == BLOCK_AREA_LEAVE) {
		ret = nvt_ts_disable_block_mode(ts);
		if (ret < 0)
			goto err;
	}

	input_info(true, &ts->client->dev, "%s: %02X(%d)\n", __func__, mode, ts->untouchable_area);

	ret = nvt_ts_mode_switch(ts, mode, true);
	if (ret)
		goto err;

	mutex_unlock(&ts->lock);

	ts->touchable_area = !!sec->cmd_param[0];

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
err:
	mutex_unlock(&ts->lock);
out:
	ts->touchable_area = false;

	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

/*
static void set_untouchable_area_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 mode;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (!sec->cmd_param[0] && !sec->cmd_param[1] && !sec->cmd_param[2] && !sec->cmd_param[3])
		mode = BLOCK_AREA_LEAVE;
	else
		mode = BLOCK_AREA_ENTER;

	ts->untouchable_area = (mode == BLOCK_AREA_ENTER ? true : false);

	if (!sec->cmd_param[1])
		landscape_deadzone[0] = sec->cmd_param[3];

	if (sec->cmd_param[3] == DEFAULT_MAX_HEIGHT)
		landscape_deadzone[1] = sec->cmd_param[3] - sec->cmd_param[1];

	input_info(true, &ts->client->dev, "%s: %d,%d %d,%d %d,%d\n", __func__,
		sec->cmd_param[0], sec->cmd_param[1], sec->cmd_param[2], sec->cmd_param[3],
		landscape_deadzone[0], landscape_deadzone[1]);

	if (ts->touchable_area) {
		input_err(true, &ts->client->dev, "%s: set_touchable_area is enabled\n", __func__);
		goto out_for_touchable;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	if (mode == BLOCK_AREA_ENTER) {
		ret = nvt_ts_set_untouchable_area(ts);
		if (ret < 0)
			goto err;
	}

	ret = nvt_ts_mode_switch(ts, mode, true);
	if (ret)
		goto err;

	mutex_unlock(&ts->lock);

	ts->untouchable_area = (mode == BLOCK_AREA_ENTER ? true : false);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
err:
	mutex_unlock(&ts->lock);
out:
	ts->untouchable_area = false;
out_for_touchable:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}
*/

/*
 *	cmd_param
 *		[1], 0 disable
 *		     1 enable only left side
 *		     2 enable only right side
 *
 *		[2], upper Y coordinate
 *
 *		[3], lower Y coordinate 
 */
static void nvt_ts_set_grip_exception_zone(struct nvt_ts_data *ts, int *cmd_param)
{
	u8 buf[8];

	switch (cmd_param[1]) {
		case 0:	//disable
			input_info(true, &ts->client->dev, "%s: disable\n", __func__);
			buf[0] = EVENT_MAP_HOST_CMD;
			buf[1] = EXTENDED_CUSTOMIZED_CMD;
			buf[2] = SET_GRIP_EXECPTION_ZONE;
			buf[3] = (u8)cmd_param[1];
			buf[4] = 0;
			buf[5] = 0;
			buf[6] = 0;
			buf[7] = 0;
			nvt_ts_mode_switch_extended(ts, buf, 8, false);
			goto out;
		case 1:	//enable left
			input_info(true, &ts->client->dev, "%s: enable left side\n", __func__);
			break;
		case 2: //enable right
			input_info(true, &ts->client->dev, "%s: enable right side\n", __func__);
			break;
		default:
			input_err(true, &ts->client->dev, "%s: not support parameter 0x%02X\n",
					__func__, cmd_param[1]);
			goto err;
	}

	//enable left or right
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = EXTENDED_CUSTOMIZED_CMD;
	buf[2] = SET_GRIP_EXECPTION_ZONE;
	buf[3] = (u8)cmd_param[1];
	buf[4] = (u8)(cmd_param[2] & 0xFF);
	buf[5] = (u8)((cmd_param[2] >> 8) & 0xFF);
	buf[6] = (u8)(cmd_param[3] & 0xFF);
	buf[7] = (u8)((cmd_param[3] >> 8) & 0xFF);
	nvt_ts_mode_switch_extended(ts, buf, 8, false);

out:
	msleep(20);
err:
	return;
}

/*
 *	cmd_param
 *		[1], grip zone (both long side)
 *
 *		[2], upper reject zone (both long side)
 *
 *		[3], lower reject zone (both long side)
 *
 *		[4], reject zone boundary of Y (divide upper and lower)
 */
static void nvt_ts_set_grip_portrait_mode(struct nvt_ts_data *ts, int *cmd_param)
{
	u8 buf[8];

	input_info(true, &ts->client->dev, "%s: set portrait mode parameters\n", __func__);

	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = EXTENDED_CUSTOMIZED_CMD;
	buf[2] = SET_GRIP_PORTRAIT_MODE;
	buf[3] = (u8)cmd_param[1];
	buf[4] = (u8)cmd_param[2];
	buf[5] = (u8)cmd_param[3];
	buf[6] = (u8)(cmd_param[4] & 0xFF);
	buf[7] = (u8)((cmd_param[4] >> 8) & 0xFF);
	nvt_ts_mode_switch_extended(ts, buf, 8, false);

	msleep(20);
}

/*
 *	cmd_param
 *		[1], 0 use previous portrait grip and reject settings
 *		     1 enable landscape mode
 *
 *		[2], reject zone (both long side)
 *
 *		[3], grip zone (both long side)
 *
 *		[4], reject zone (top short side)
 *
 *		[5], reject zone (bottom short side)
 *
 *		[6], grip zone (top short side)
 *
 *		[7], grip zone (bottom short side)
 */
static void nvt_ts_set_grip_landscape_mode(struct nvt_ts_data *ts, int *cmd_param)
{
	u8 buf[12];

	switch (cmd_param[1]) {
		case 0: //use previous portrait setting
			input_info(true, &ts->client->dev, "%s: use previous portrait settings\n", __func__);
			buf[0] = EVENT_MAP_HOST_CMD;
			buf[1] = EXTENDED_CUSTOMIZED_CMD;
			buf[2] = SET_GRIP_LANDSCAPE_MODE;
			buf[3] = (u8)cmd_param[1];
			nvt_ts_mode_switch_extended(ts, buf, 4, false);
			goto out;
		case 1: //enable landscape mode
			input_info(true, &ts->client->dev, "%s: set landscape mode parameters\n", __func__);
			buf[0] = EVENT_MAP_HOST_CMD;
			buf[1] = EXTENDED_CUSTOMIZED_CMD;
			buf[2] = SET_GRIP_LANDSCAPE_MODE;
			buf[3] = (u8)cmd_param[1];
			buf[4] = (u8)cmd_param[2];
			buf[5] = (u8)cmd_param[3];
			buf[6] = (u8)cmd_param[4];
			buf[7] = (u8)cmd_param[5];
			buf[8] = (u8)cmd_param[6];
			buf[9] = (u8)cmd_param[7];
			nvt_ts_mode_switch_extended(ts, buf, 10, false);
			goto out;
		default:
			input_err(true, &ts->client->dev, "%s: not support parameter 0x%02X\n", __func__, cmd_param[1]);
			goto err;
	}

out:
	msleep(20);
err:
	return;
}

/*
 *	cmd_param
 *		[0], 0 edge/grip execption zone
 *		     1 portrait mode
 *		     2 landscape mode
 */
static void set_grip_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] == 0) {
		memcpy(ts->grip_edgehandler_restore_data, sec->cmd_param, sizeof(int) * SEC_CMD_PARAM_NUM);
	} else if ((sec->cmd_param[0] > 0) && (sec->cmd_param[0] < 3)) {
		memcpy(ts->setgrip_restore_data, sec->cmd_param, sizeof(int) * SEC_CMD_PARAM_NUM);
	} else {
		input_err(true, &ts->client->dev, "%s: cmd0 is abnormal, %d\n",
					__func__, sec->cmd_param[0]);
	}

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	// print parameters (debug use)
	input_dbg(true, &ts->client->dev, "%s: 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X\n", 
		__func__, sec->cmd_param[0], sec->cmd_param[1], sec->cmd_param[2], sec->cmd_param[3],
		sec->cmd_param[4], sec->cmd_param[5], sec->cmd_param[6], sec->cmd_param[7]);

	switch (sec->cmd_param[0]) {
		case 0:
			nvt_ts_set_grip_exception_zone(ts, sec->cmd_param);
			break;
		case 1:
			nvt_ts_set_grip_portrait_mode(ts, sec->cmd_param);
			break;
		case 2:
			nvt_ts_set_grip_landscape_mode(ts, sec->cmd_param);
			break;
		default:
			input_err(true, &ts->client->dev, "%s: not support mode 0x%02X\n", __func__, sec->cmd_param[0]);
			goto err;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
err:
	mutex_unlock(&ts->lock);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

/*
static void lcd_orientation(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 mode;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (sec->cmd_param[0] < 0 && sec->cmd_param[0] > 3) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	} else {
		switch (sec->cmd_param[0]) {
		case ORIENTATION_0:
			mode = EDGE_REJ_VERTICLE_MODE;
			input_info(true, &ts->client->dev, "%s: Get 0 degree LCD orientation.\n", __func__);
			break;
		case ORIENTATION_90:
			mode = EDGE_REJ_RIGHT_UP_MODE;
			input_info(true, &ts->client->dev, "%s: Get 90 degree LCD orientation.\n", __func__);
			break;
		case ORIENTATION_180:
			mode = EDGE_REJ_VERTICLE_MODE;
			input_info(true, &ts->client->dev, "%s: Get 180 degree LCD orientation.\n", __func__);
			break;
		case ORIENTATION_270:
			mode = EDGE_REJ_LEFT_UP_MODE;
			input_info(true, &ts->client->dev, "%s: Get 270 degree LCD orientation.\n", __func__);
			break;
		default:
			input_err(true, &ts->client->dev, "%s: LCD orientation value error.\n", __func__);
			goto out;
			break;
		}
	}

	input_info(true, &ts->client->dev, "%s: %d,%02X\n", __func__, sec->cmd_param[0], mode);

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	ret = nvt_ts_mode_switch(ts, mode, true);
	if (ret) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state =  SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}
*/

static void run_self_open_raw_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char tmp[SEC_CMD_STR_LEN] = { 0 };
	char *buff;
	int *raw_buff;
	int x, y;
	int offset;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err;

	if (nvt_ts_open_read(ts, raw_buff)) {
		kfree(buff);
		goto err;
	}

	for (y = 0; y < ts->platdata->y_num; y++) {
		for (x = 0; x < ts->platdata->x_num; x++) {
			offset = y * ts->platdata->x_num + x;
			snprintf(tmp, CMD_RESULT_WORD_LEN, "%d,", raw_buff[offset]);
			strncat(buff, tmp, CMD_RESULT_WORD_LEN);
			memset(tmp, 0x00, CMD_RESULT_WORD_LEN);
		}
	}

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	mutex_unlock(&ts->lock);

	sec_cmd_set_cmd_result(sec, buff,
			strnlen(buff, ts->platdata->x_num * ts->platdata->y_num * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(raw_buff);
	kfree(buff);

	snprintf(tmp, sizeof(tmp), "%s", "OK");
	input_info(true, &ts->client->dev, "%s: %s", __func__, tmp);

	return;

err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	mutex_unlock(&ts->lock);

	kfree(raw_buff);

out:
	snprintf(tmp, sizeof(tmp), "%s", "NG");
	sec_cmd_set_cmd_result(sec, tmp, strnlen(tmp, sizeof(tmp)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &ts->client->dev, "%s: %s", __func__, tmp);
}

static void run_self_open_raw_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int *raw_buff;
	int min, max;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	min = 0x7FFFFFFF;
	max = 0x80000000;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	if (nvt_ts_open_read(ts, raw_buff))
		goto err;

	nvt_ts_print_buff(ts, raw_buff, &min, &max);

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "OPEN_RAW");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);

	return;

err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	mutex_unlock(&ts->lock);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "OPEN_RAW");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);
}

static void run_self_open_uni_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char tmp[SEC_CMD_STR_LEN] = { 0 };
	char *buff;
	int *raw_buff;
	int x, y;
	int offset, data;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err;

	if (nvt_ts_open_read(ts, raw_buff)) {
		kfree(buff);
		goto err;
	}

	for (y = 0; y < ts->platdata->y_num; y++) {
		for (x = 1; x < ts->platdata->x_num / 2; x++) {
			offset = y * ts->platdata->x_num + x;
			data = (int)(raw_buff[offset] - raw_buff[offset - 1]) * 100 / raw_buff[offset];
			snprintf(tmp, CMD_RESULT_WORD_LEN, "%d,", data);
			strncat(buff, tmp, CMD_RESULT_WORD_LEN);
			memset(tmp, 0x00, CMD_RESULT_WORD_LEN);
		}

		for (; x < ts->platdata->x_num - 1; x++) {
			offset = y * ts->platdata->x_num + x;
			data = (int)(raw_buff[offset + 1] - raw_buff[offset]) * 100 / raw_buff[offset];
			snprintf(tmp, CMD_RESULT_WORD_LEN, "%d,", data);
			strncat(buff, tmp, CMD_RESULT_WORD_LEN);
			memset(tmp, 0x00, CMD_RESULT_WORD_LEN);
		}
	}

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	mutex_unlock(&ts->lock);

	sec_cmd_set_cmd_result(sec, buff,
			strnlen(buff, ts->platdata->x_num * ts->platdata->y_num * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(buff);
	kfree(raw_buff);

	snprintf(tmp, sizeof(tmp), "%s", "OK");
	input_info(true, &ts->client->dev, "%s: %s", __func__, tmp);

	return;
err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	mutex_unlock(&ts->lock);

	kfree(raw_buff);
out:
	snprintf(tmp, sizeof(tmp), "%s", "NG");
	sec_cmd_set_cmd_result(sec, tmp, strnlen(tmp, sizeof(tmp)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &ts->client->dev, "%s: %s", __func__, tmp);
}

static void run_self_open_uni_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int *raw_buff, *uni_buff;
	int x, y;
	int min, max;
	int offset, data;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	min = 0x7FFFFFFF;
	max = 0x80000000;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	uni_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!uni_buff) {
		mutex_unlock(&ts->lock);
		kfree(raw_buff);
		goto out;
	}

	if (nvt_ts_open_read(ts, raw_buff))
		goto err;

	for (y = 0; y < ts->platdata->y_num; y++) {
		for (x = 1; x < ts->platdata->x_num / 2; x++) {
			offset = y * ts->platdata->x_num + x;
			data = (int)(raw_buff[offset] - raw_buff[offset - 1]) * 100 / raw_buff[offset];
			uni_buff[offset] = data;
		}

		for (; x < ts->platdata->x_num - 1; x++) {
			offset = y * ts->platdata->x_num + x;
			data = (int)(raw_buff[offset + 1] - raw_buff[offset]) * 100 / raw_buff[offset];
			uni_buff[offset] = data;
		}
	}

	nvt_ts_print_gap_buff(ts, uni_buff, &min, &max);

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(uni_buff);
	kfree(raw_buff);

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "OPEN_UNIFORMITY");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);

	return;
err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(uni_buff);
	kfree(raw_buff);

	mutex_unlock(&ts->lock);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "OPEN_UNIFORMITY");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);
}

static void run_self_short_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int *raw_buff;
	int min, max;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	min = 0x7FFFFFFF;
	max = 0x80000000;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	if (nvt_ts_short_read(ts, raw_buff))
		goto err;

	nvt_ts_print_buff(ts, raw_buff, &min, &max);

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SHORT_TEST");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);

	return;
err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	mutex_unlock(&ts->lock);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SHORT_TEST");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);
}

static void run_self_rawdata_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int *raw_buff;
	int min, max;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	min = 0x7FFFFFFF;
	max = 0x80000000;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	if (nvt_ts_rawdata_read(ts, raw_buff))
		goto err;

	nvt_ts_print_buff(ts, raw_buff, &min, &max);

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_RAW_DATA");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);

	return;

err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	mutex_unlock(&ts->lock);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_RAW_DATA");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);
}

static void run_self_ccdata_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char tmp[SEC_CMD_STR_LEN] = { 0 };
	char *buff;
	int *raw_buff;
	int x, y;
	int offset;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err;

	if (nvt_ts_ccdata_read(ts, raw_buff)) {
		kfree(buff);
		goto err;
	}

	for (y = 0; y < ts->platdata->y_num; y++) {
		for (x = 0; x < ts->platdata->x_num; x++) {
			offset = y * ts->platdata->x_num + x;
			snprintf(tmp, CMD_RESULT_WORD_LEN, "%d,", raw_buff[offset]);
			strncat(buff, tmp, CMD_RESULT_WORD_LEN);
			memset(tmp, 0x00, CMD_RESULT_WORD_LEN);
		}
	}

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	mutex_unlock(&ts->lock);

	sec_cmd_set_cmd_result(sec, buff,
			strnlen(buff, ts->platdata->x_num * ts->platdata->y_num * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(buff);
	kfree(raw_buff);

	snprintf(tmp, sizeof(tmp), "%s", "OK");
	input_info(true, &ts->client->dev, "%s: %s", __func__, tmp);

	return;

err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	mutex_unlock(&ts->lock);

	kfree(raw_buff);
out:
	snprintf(tmp, sizeof(tmp), "%s", "NG");
	sec_cmd_set_cmd_result(sec, tmp, strnlen(tmp, sizeof(tmp)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &ts->client->dev, "%s: %s", __func__, tmp);
}

static void run_self_ccdata_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int *raw_buff;
	int min, max;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	min = 0x7FFFFFFF;
	max = 0x80000000;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	if (nvt_ts_ccdata_read(ts, raw_buff))
		goto err;

	nvt_ts_print_buff(ts, raw_buff, &min, &max);

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_CC");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);

	return;

err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);

	mutex_unlock(&ts->lock);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_CC");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);
}

static void run_self_noise_max_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int *raw_min, *raw_max;
	int min, max;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	min = 0x7FFFFFFF;
	max = 0x80000000;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_min = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	raw_max = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_min || !raw_max) {
		mutex_unlock(&ts->lock);
		goto err_alloc_mem;
	}

	if (nvt_ts_noise_read(ts, raw_min, raw_max))
		goto err;

	nvt_ts_print_buff(ts, raw_max, &min, &max);

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_min);
	kfree(raw_max);

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "NOISE_MAX");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);

	return;

err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

err_alloc_mem:
	if (raw_min)
		kfree(raw_min);
	if (raw_max)
		kfree(raw_max);

	mutex_unlock(&ts->lock);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "NOISE_MAX");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);
}

static void run_self_noise_min_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int *raw_min, *raw_max;
	int min, max;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	min = 0x7FFFFFFF;
	max = 0x80000000;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	raw_min = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	raw_max = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_min || !raw_max) {
		mutex_unlock(&ts->lock);
		goto err_alloc_mem;
	}

	if (nvt_ts_noise_read(ts, raw_min, raw_max))
		goto err;

	nvt_ts_print_buff(ts, raw_min, &min, &max);

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_min);
	kfree(raw_max);

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "NOISE_MIN");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);

	return;
err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

err_alloc_mem:
	if (raw_min)
		kfree(raw_min);
	if (raw_max)
		kfree(raw_max);

	mutex_unlock(&ts->lock);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "NOISE_MIN");

	input_raw_info(true, &ts->client->dev, "%s: %s", __func__, buff);
}

static void run_sram_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = -EIO;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	/* nvt_ts_nt36523_sram_test supports only NT36523 */
	if (ts->mmap->MBT_EN)
		ret = nvt_ts_nt36523_sram_test(ts);
	else
		input_info(true, &ts->client->dev, "%s: not support SRAM test.\n", __func__);

	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	mutex_unlock(&ts->lock);

out:
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%d", ret);
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SRAM");
	input_info(true, &ts->client->dev, "%s: %s", __func__, buff);
}

static void factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	get_chip_vendor(sec);
	get_chip_name(sec);
	get_fw_ver_bin(sec);
	get_fw_ver_ic(sec);
	run_self_open_raw_read(sec);
	run_self_open_uni_read(sec);
	run_self_rawdata_read(sec);
	run_self_ccdata_read(sec);
	run_self_short_read(sec);
	run_self_noise_max_read(sec);
	run_self_noise_min_read(sec);
	run_sram_test(sec);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

	input_info(true, &ts->client->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static void run_trx_short_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char test[32];
	char result[32];
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[1])
		snprintf(test, sizeof(test), "TEST=%d,%d", sec->cmd_param[0], sec->cmd_param[1]);
	else
		snprintf(test, sizeof(test), "TEST=%d", sec->cmd_param[0]);

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		goto out;
	}

	if (sec->cmd_param[0] != OPEN_SHORT_TEST) {
		input_err(true, &ts->client->dev, "%s: invalid parameter %d\n",
			__func__, sec->cmd_param[0]);
		goto out;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	if (sec->cmd_param[1] == CHECK_ONLY_OPEN_TEST) {
		ret = nvt_ts_open_test(ts);
	} else if (sec->cmd_param[1] == CHECK_ONLY_SHORT_TEST) {
		ret = nvt_ts_short_test(ts);
	} else {
		input_err(true, &ts->client->dev, "%s: invalid parameter option %d\n",
			__func__, sec->cmd_param[1]);
		mutex_unlock(&ts->lock);
		goto out;
	}

	if (ret) {
		mutex_unlock(&ts->lock);
		goto out;
	}

	mutex_unlock(&ts->lock);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	snprintf(result, sizeof(result), "RESULT=PASS");
	sec_cmd_send_event_to_user(&ts->sec, test, result);

	return;
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	snprintf(result, sizeof(result), "RESULT=FAIL");
	sec_cmd_send_event_to_user(&ts->sec, test, result);

	input_info(true, &ts->client->dev, "%s: %s", __func__, buff);
}

static void get_func_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "0x%X", nvt_ts_mode_read(ts));

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void aot_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] > 1 || sec->cmd_param[0] < 0) {
		input_err(true, &ts->client->dev,
				"%s: wrong param %d\n", __func__, sec->cmd_param[0]);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec->cmd_param[0])
		ts->lowpower_mode |= NVT_SPONGE_MODE_DOUBLETAP_WAKEUP;
	else
		ts->lowpower_mode &= ~NVT_SPONGE_MODE_DOUBLETAP_WAKEUP;

	input_info(true, &ts->client->dev, "%s: %d, lp:0x%02X\n",
			__func__, sec->cmd_param[0], ts->lowpower_mode);

	ret = nvt_ts_write_to_sponge(ts, &ts->lowpower_mode, SPONGE_LP_FEATURE, 1);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void clear_cover_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 wbuf[3] = { 0 };
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] > 3 || sec->cmd_param[0] < 0) {
		input_err(true, &ts->client->dev,
				"%s: wrong param %d\n", __func__, sec->cmd_param[0]);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		goto out;
	}

	input_info(true, &ts->client->dev, "%s: %d, %d\n", __func__, sec->cmd_param[0], sec->cmd_param[1]);

	if (sec->cmd_param[0] > 1)
		ts->flip_enable = true;
	else
		ts->flip_enable = false;

	/* disable tsp scan when cover is closed (for Tablet) */
	if (ts->platdata->scanoff_cover_close) {
		if (ts->flip_enable) {
			input_info(true, &ts->client->dev, "%s: keep record for cover on, and enter deep standby mode when screen off\n", __func__);
//			input_info(true, &ts->client->dev, "%s: enter deep standby mode\n", __func__);
//			wbuf[0] = EVENT_MAP_HOST_CMD;
//			wbuf[1] = NVT_CMD_DEEP_SLEEP_MODE;
//			ret = nvt_ts_i2c_write(ts, I2C_FW_Address, wbuf, 2);
//
//			nvt_ts_release_all_finger(ts);
		} else {
			input_info(true, &ts->client->dev, "%s: reset to normal mode\n", __func__);
			nvt_ts_sw_reset_idle(ts);

			//---write command to clear fw state before reset---
			nvt_ts_set_page(ts, I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR);

			wbuf[0] = EVENT_MAP_RESET_COMPLETE;
			wbuf[1] = 0x00;
			ret = nvt_ts_i2c_write(ts, I2C_FW_Address, wbuf, 2);

			nvt_ts_bootloader_reset(ts);

			if (ts->power_status == POWER_LPM_STATUS) {
				nvt_ts_check_fw_reset_state(ts, RESET_STATE_INIT);

				//---write command to enter "wakeup gesture mode"---
				wbuf[0] = EVENT_MAP_HOST_CMD;
				wbuf[1] = NVT_CMD_GESTURE_MODE;
				wbuf[2] = 0x80;
				ret = nvt_ts_i2c_write(ts, I2C_FW_Address, wbuf, 3);

				input_info(true, &ts->client->dev, "%s: enter lp mode, 0x%02X\n",
						__func__, ts->lowpower_mode);
			}
		}
	}

	mutex_unlock(&ts->lock);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void debug(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	ts->debug_flag = sec->cmd_param[0];

	input_info(true, &ts->client->dev, "%s: command is %d\n", __func__, ts->debug_flag);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);
}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s", "NA");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("get_chip_vendor", get_chip_vendor),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("get_checksum_data", get_checksum_data),},
	{SEC_CMD("get_threshold", get_threshold),},
	{SEC_CMD("check_connection", check_connection),},
	{SEC_CMD_H("glove_mode", glove_mode),},
	{SEC_CMD_H("set_note_mode", set_note_mode),},
	{SEC_CMD("set_sip_mode", set_sip_mode),},
	{SEC_CMD_H("set_game_mode", set_game_mode),},
	{SEC_CMD_H("dead_zone_enable", dead_zone_enable),},
	/*{SEC_CMD_H("spay_enable", spay_enable),},*/
	{SEC_CMD("get_x_num", get_x_num),},
	{SEC_CMD("get_y_num", get_y_num),},
	{SEC_CMD_H("set_touchable_area", set_touchable_area),},
	/*{SEC_CMD_H("set_untouchable_area_rect", set_untouchable_area_rect),},*/
	{SEC_CMD("set_grip_data", set_grip_data),},
	/*{SEC_CMD("lcd_orientation", lcd_orientation),},*/
	{SEC_CMD("run_self_open_raw_read", run_self_open_raw_read),},
	{SEC_CMD("run_self_open_raw_read_all", run_self_open_raw_read_all),},
	{SEC_CMD("run_self_open_uni_read", run_self_open_uni_read),},
	{SEC_CMD("run_self_open_uni_read_all", run_self_open_uni_read_all),},
	{SEC_CMD("run_self_short_read", run_self_short_read),},
	{SEC_CMD("run_self_rawdata_read", run_self_rawdata_read),},
	{SEC_CMD("run_self_ccdata_read", run_self_ccdata_read),},
	{SEC_CMD("run_self_ccdata_read_all", run_self_ccdata_read_all),},
	{SEC_CMD("run_self_noise_min_read", run_self_noise_min_read),},
	{SEC_CMD("run_self_noise_max_read", run_self_noise_max_read),},
	{SEC_CMD("factory_cmd_result_all", factory_cmd_result_all),},
	{SEC_CMD("run_trx_short_test", run_trx_short_test),},
	{SEC_CMD("run_sram_test", run_sram_test),},
	{SEC_CMD("get_func_mode", get_func_mode),},
	{SEC_CMD_H("aot_enable", aot_enable),},
	{SEC_CMD_H("clear_cover_mode", clear_cover_mode),},
	{SEC_CMD("debug", debug),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

static ssize_t read_multi_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, ts->multi_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", ts->multi_count);
}

static ssize_t clear_multi_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	ts->multi_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_comm_err_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, ts->comm_err_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", ts->comm_err_count);
}

static ssize_t clear_comm_err_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	ts->comm_err_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_module_id_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[256] = { 0 };

	snprintf(buff, sizeof(buff), "NO%02X%02X%02X%02X",
		ts->fw_ver_bin[0], ts->fw_ver_bin[1], ts->fw_ver_bin[2], ts->fw_ver_bin[3]);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t read_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char *str_orig, *str;
	char buffer[21] = { 0 };
	char *tok;

	if (ts->platdata->firmware_name) {
		str_orig = kstrdup(ts->platdata->firmware_name, GFP_KERNEL);
		if (!str_orig)
			goto err;

		str = str_orig;

		tok = strsep(&str, "/");
		tok = strsep(&str, "_");

		snprintf(buffer, sizeof(buffer), "NVT_%s", tok);

		kfree(str_orig);
	}

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buffer);
err:
	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buffer);
}

static ssize_t clear_checksum_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	ts->checksum_result = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_checksum_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, ts->checksum_result);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", ts->checksum_result);
}

static ssize_t read_all_touch_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: touch:%d\n", __func__, ts->all_finger_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "\"TTCN\":\"%d\"", ts->all_finger_count);
}

static ssize_t clear_all_touch_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);

	ts->all_finger_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t sensitivity_test_show(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char data[20] = { 0 };
	s16 diff[9] = { 0 };
	int i;
	int ret;

	data[0] = EVENT_MAP_SENSITIVITY_DIFF;
	ret = nvt_ts_i2c_read(ts, I2C_FW_Address, data, 19);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read sensitivity",
			__func__);
	}

	for (i = 0; i < 9; i++)
		diff[i] = (data[2 * i + 2] << 8) + data[2 * i + 1];

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d",
		diff[0], diff[1], diff[2], diff[3], diff[4], diff[5], diff[6], diff[7], diff[8]);
}

static ssize_t sensitivity_test_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	unsigned long val = 0;
	int ret;
	u8 mode;

	if (count > 2) {
		input_err(true, &ts->client->dev, "%s: invalid parameter\n", __func__);
		return count;
	}

	ret = kstrtoul(buf, 10, &val);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: failed to get param\n", __func__);
		return count;
	}

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF!\n", __func__);
		return count;
	}

	input_info(true, &ts->client->dev, "%s: %s\n", __func__,
			val ? "enable" : "disable");

	mode = val ? SENSITIVITY_TEST_ENTER : SENSITIVITY_TEST_LEAVE;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		return count;
	}

	ret = nvt_ts_mode_switch(ts, mode, true);

	mutex_unlock(&ts->lock);

	input_info(true, &ts->client->dev, "%s: %s %s\n", __func__,
			val ? "enable" : "disabled", ret ? "fail" : "done");

	return count;
}

static ssize_t read_support_feature(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	u32 feature = 0;

	if (ts->platdata->enable_settings_aot)
		feature |= INPUT_FEATURE_ENABLE_SETTINGS_AOT;

	input_info(true, &ts->client->dev, "%s: %d%s\n", __func__, feature,
			feature & INPUT_FEATURE_ENABLE_SETTINGS_AOT ? " aot" : "");

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", feature);
}

static ssize_t get_lp_dump(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	u8 sponge_data[10] = {0, };
	u16 current_index;
	u8 dump_format, dump_num;
	u16 dump_start, dump_end;
	int i, ret;

	if (ts->power_status == POWER_OFF_STATUS) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
	}

	nvt_interrupt_set(ts, INT_DISABLE);

	ret = nvt_ts_read_from_sponge(ts, sponge_data, SPONGE_DUMP_FORMAT, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Failed to read dump format\n", __func__);
		snprintf(buf, SEC_CMD_BUF_SIZE, "NG, Failed to read dump format");
		goto out;
	}

	dump_format = sponge_data[0];
	dump_num = sponge_data[1];
	dump_start = SPONGE_DUMP_START;
	dump_end = dump_start + (dump_format * (dump_num - 1));

	current_index = (sponge_data[3] & 0xFF) << 8 | (sponge_data[2] & 0xFF);
	if (current_index > dump_end || current_index < dump_start) {
		input_err(true, &ts->client->dev,
				"Failed to Sponge LP log %d\n", current_index);
		snprintf(buf, SEC_CMD_BUF_SIZE,
				"NG, Failed to Sponge LP log, current_index=%d",
				current_index);
		goto out;
	}

	input_info(true, &ts->client->dev, "%s: DEBUG format=%d, num=%d, start=%d, end=%d, current_index=%d\n",
				__func__, dump_format, dump_num, dump_start, dump_end, current_index);

	if (dump_format > 10) {
		input_err(true, &ts->client->dev, "%s: wrong sponge dump_format size:%d\n", __func__, dump_format);
		snprintf(buf, SEC_CMD_BUF_SIZE, "NG,wrong sponge_dump_format");
		goto out;
	}

	for (i = dump_num - 1 ; i >= 0 ; i--) {
		u16 data0, data1, data2, data3, data4;
		char buff[30] = {0, };
		u16 sponge_addr;

		if (current_index < (dump_format * i))
			sponge_addr = (dump_format * dump_num) + current_index - (dump_format * i);
		else
			sponge_addr = current_index - (dump_format * i);

		if (sponge_addr < dump_start)
			sponge_addr += (dump_format * dump_num);

		ret = nvt_ts_read_from_sponge(ts, sponge_data, sponge_addr, dump_format);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: Failed to read sponge\n", __func__);
			snprintf(buf, SEC_CMD_BUF_SIZE,
					"NG, Failed to read sponge, addr=%d",
					sponge_addr);
			goto out;
		}

		data0 = (sponge_data[1] & 0xFF) << 8 | (sponge_data[0] & 0xFF);
		data1 = (sponge_data[3] & 0xFF) << 8 | (sponge_data[2] & 0xFF);
		data2 = (sponge_data[5] & 0xFF) << 8 | (sponge_data[4] & 0xFF);
		data3 = (sponge_data[7] & 0xFF) << 8 | (sponge_data[6] & 0xFF);
		data4 = (sponge_data[9] & 0xFF) << 8 | (sponge_data[8] & 0xFF);

		if (data0 || data1 || data2 || data3 || data4) {
			if (dump_format == 10) {
				snprintf(buff, sizeof(buff),
						"%d: %04x%04x%04x%04x%04x\n",
						sponge_addr, data0, data1, data2, data3, data4);
			} else {
				snprintf(buff, sizeof(buff),
						"%d: %04x%04x%04x%04x\n",
						sponge_addr, data0, data1, data2, data3);
			}
			strlcat(buf, buff, PAGE_SIZE);
		}
	}

out:
	nvt_interrupt_set(ts, INT_ENABLE);
	return strlen(buf);
}

static DEVICE_ATTR(multi_count, 0664, read_multi_count_show, clear_multi_count_store);
static DEVICE_ATTR(comm_err_count, 0664, read_comm_err_count_show, clear_comm_err_count_store);
static DEVICE_ATTR(module_id, 0444, read_module_id_show, NULL);
static DEVICE_ATTR(vendor, 0444, read_vendor_show, NULL);
static DEVICE_ATTR(checksum, 0664, read_checksum_show, clear_checksum_store);
static DEVICE_ATTR(all_touch_count, 0664, read_all_touch_count_show, clear_all_touch_count_store);
static DEVICE_ATTR(sensitivity_mode, 0664, sensitivity_test_show, sensitivity_test_store);
static DEVICE_ATTR(support_feature, 0444, read_support_feature, NULL);
static DEVICE_ATTR(get_lp_dump, 0444, get_lp_dump, NULL);

static struct attribute *cmd_attributes[] = {
	&dev_attr_multi_count.attr,
	&dev_attr_comm_err_count.attr,
	&dev_attr_module_id.attr,
	&dev_attr_vendor.attr,
	&dev_attr_checksum.attr,
	&dev_attr_all_touch_count.attr,
	&dev_attr_sensitivity_mode.attr,
	&dev_attr_support_feature.attr,
	&dev_attr_get_lp_dump.attr,
	NULL,
};

static struct attribute_group cmd_attr_group = {
	.attrs = cmd_attributes,
};

void nvt_ts_run_rawdata_all(struct nvt_ts_data *ts)
{
	int *raw_buff;
	int min, max;

	if (mutex_lock_interruptible(&ts->lock)) {
		input_err(true, &ts->client->dev, "%s: another task is running\n",
			__func__);
		return;
	}

	raw_buff = kzalloc(ts->platdata->x_num * ts->platdata->y_num * sizeof(int), GFP_KERNEL);
	if (!raw_buff) {
		mutex_unlock(&ts->lock);
		return;
	}

	input_info(true, &ts->client->dev, "%s\n", __func__);

	min = 0x7FFFFFFF;
	max = 0x80000000;
	if (nvt_ts_open_read(ts, raw_buff))
		goto err;

	nvt_ts_print_buff(ts, raw_buff, &min, &max);
	
	min = 0x7FFFFFFF;
	max = 0x80000000;
	if (nvt_ts_rawdata_read(ts, raw_buff))
		goto err;

	nvt_ts_print_buff(ts, raw_buff, &min, &max);
	
	min = 0x7FFFFFFF;
	max = 0x80000000;
	if (nvt_ts_short_read(ts, raw_buff))
		goto err;

	nvt_ts_print_buff(ts, raw_buff, &min, &max);

err:
	//---Reset IC---
	nvt_ts_bootloader_reset(ts);

	kfree(raw_buff);
	mutex_unlock(&ts->lock);
}

static int nvt_mp_parse_dt(struct nvt_ts_data *ts, const char *node_compatible)
{
	struct device_node *np = ts->client->dev.of_node;
	struct device_node *child = NULL;

	/* find each MP sub-nodes */
	for_each_child_of_node(np, child) {
		/* find the specified node */
		if (of_device_is_compatible(child, node_compatible)) {
			np = child;
			break;
		}
	}

	if (!child) {
		input_err(true, &ts->client->dev, "%s: do not find mp criteria for %s\n",
			  __func__, node_compatible);
		return -EINVAL;
	}

	/* MP Criteria */
	if (of_property_read_u32_array(np, "open_test_spec", ts->platdata->open_test_spec, 2))
		return -EINVAL;

	if (of_property_read_u32_array(np, "short_test_spec", ts->platdata->short_test_spec, 2))
		return -EINVAL;

	if (of_property_read_u32(np, "diff_test_frame", &ts->platdata->diff_test_frame))
		return -EINVAL;

	input_info(true, &ts->client->dev, "%s: parse mp criteria for %s\n", __func__, node_compatible);

	return 0;
}

int nvt_ts_sec_fn_init(struct nvt_ts_data *ts)
{
	int ret;

	/* Parsing criteria from dts */
	if(of_property_read_bool(ts->client->dev.of_node, "novatek,mp-support-dt")) {
		u8 mpcriteria[32] = { 0 };
		int pid;

		//---set xdata index to EVENT BUF ADDR---
		mpcriteria[0] = 0xFF;
		mpcriteria[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		mpcriteria[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		nvt_ts_i2c_write(ts, I2C_FW_Address, mpcriteria, 3);

		//---read project id---
		mpcriteria[0] = EVENT_MAP_PROJECTID;
		nvt_ts_i2c_read(ts, I2C_FW_Address, mpcriteria, 3);

		pid = (mpcriteria[2] << 8) + mpcriteria[1];

		/*
		 * Parsing Criteria by Novatek PID
		 * The string rule is "novatek-mp-criteria-<nvt_pid>"
		 * nvt_pid is 2 bytes (show hex).
		 *
		 * Ex. nvt_pid = 500A
		 *     mpcriteria = "novatek-mp-criteria-500A"
		 */
		snprintf(mpcriteria, sizeof(mpcriteria), "novatek-mp-criteria-%04X", pid);

		ret = nvt_mp_parse_dt(ts, mpcriteria);
		if (ret) {
			input_err(true, &ts->client->dev, "%s: failed to parse mp device tree\n",
				__func__);
			//return ret;
		}
	}

	ret = sec_cmd_init(&ts->sec, sec_cmds, ARRAY_SIZE(sec_cmds), SEC_CLASS_DEVT_TSP);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to sec cmd init\n",
			__func__);
		return ret;
	}

	ret = sysfs_create_group(&ts->sec.fac_dev->kobj, &cmd_attr_group);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: failed to create sysfs attributes\n",
			__func__);
			goto out;
	}

	ret = sysfs_create_link(&ts->sec.fac_dev->kobj, &ts->input_dev->dev.kobj, "input");
	if (ret) {
		input_err(true, &ts->client->dev, "%s: failed to creat sysfs link\n",
			__func__);
		sysfs_remove_group(&ts->sec.fac_dev->kobj, &cmd_attr_group);
		goto out;
	}

	input_info(true, &ts->client->dev, "%s\n", __func__);

	return 0;

out:
	sec_cmd_exit(&ts->sec, SEC_CLASS_DEVT_TSP);

	return ret;
}

void nvt_ts_sec_fn_remove(struct nvt_ts_data *ts)
{
	sysfs_delete_link(&ts->sec.fac_dev->kobj, &ts->input_dev->dev.kobj, "input");

	sysfs_remove_group(&ts->sec.fac_dev->kobj, &cmd_attr_group);

	sec_cmd_exit(&ts->sec, SEC_CLASS_DEVT_TSP);
}
