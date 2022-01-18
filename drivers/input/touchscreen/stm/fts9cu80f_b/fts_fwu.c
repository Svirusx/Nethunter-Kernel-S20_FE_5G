
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#include "fts_ts.h"
#include <linux/spu-verify.h>

#define FTS_DEFAULT_UMS_FW "/sdcard/Firmware/TSP/stm.fw"
#define FTS_DEFAULT_SPU_FW "/spu/TSP/ffu_tsp.fw"
#define FTSFILE_SIGNATURE 0xAA55AA55

enum {
	BUILT_IN = 0,
	UMS,
	NONE,
	SPU,
};

/**
 * @brief  Type definitions - header structure of firmware file (ftb)
 */

struct fts_header {
	u32	signature;
	u32	ftb_ver;
	u32	target;
	u32	fw_id;
	u32	fw_ver;
	u32	cfg_id;
	u32	cfg_ver;
	u8	fw_area_bs;	// bs : block size
	u8	panel_area_bs;
	u8	cx_area_bs;
	u8	cfg_area_bs;
	u32	reserved1;
	u32	ext_release_ver;
	u8	project_id;
	u8	ic_name;
	u8	module_ver;
	u8	reserved2;
	u32	sec0_size;
	u32	sec1_size;
	u32	sec2_size;
	u32	sec3_size;
	u32	hdr_crc;
} __packed;

#define FW_HEADER_SIZE  64

#define WRITE_CHUNK_SIZE 1024
#define DRAM_SIZE (64 * 1024) // 64kB

#define CODE_ADDR_START 0x00000000
#define CX_ADDR_START 0x00007000
#define CONFIG_ADDR_START 0x00007C00

#define	SIGNEDKEY_SIZE		(256)

int FTS_Check_DMA_StartAndDone(struct fts_ts_info *info)
{
	int timeout = 60;
	u8 regAdd[6] = { 0xFA, 0x20, 0x00, 0x00, 0x71, 0xC0 };
	u8 val[1];

	info->fts_write_reg(info, &regAdd[0], 6);
	fts_delay(10);

	do {
		info->fts_read_reg(info, &regAdd[0], 5, (u8 *)val, 1);

		if ((val[0] & 0x80) != 0x80)
			break;

		fts_delay(50);
		timeout--;
	} while (timeout != 0);

	if (timeout == 0) {
		input_err(true, &info->client->dev, "%s: Time Over\n", __func__);
		return -1;
	}

	return 0;
}

static int FTS_Check_Erase_Done(struct fts_ts_info *info)
{
	int timeout = 60;  // 3 sec timeout
	u8 regAdd[5] = { 0xFA, 0x20, 0x00, 0x00, 0x6A };
	u8 val[1];

	do {
		info->fts_read_reg(info, &regAdd[0], 5, (u8 *)val, 1);

		if ((val[0] & 0x80) != 0x80)
			break;

		fts_delay(50);
		timeout--;
	} while (timeout != 0);

	if (timeout == 0) {
		input_err(true, &info->client->dev, "%s: Time Over\n", __func__);
		return -1;
	}

	return 0;
}

int fts_fw_fillFlash(struct fts_ts_info *info, u32 address, u8 *data, int size)
{
	int		remaining, index = 0;
	int		toWrite = 0;
	int		byteBlock = 0;
	int		wheel = 0;
	u32	  addr = 0;
	int		rc;
	int		delta;

	u8		buff[WRITE_CHUNK_SIZE + 5] = {0};
	u8		buff2[12] = {0};

	remaining = size;
	while (remaining > 0) {
		byteBlock = 0;
		addr = 0x00100000;

		while ((byteBlock < DRAM_SIZE) && remaining > 0) {
			if (remaining >= WRITE_CHUNK_SIZE) {
				if ((byteBlock + WRITE_CHUNK_SIZE) <= DRAM_SIZE) {
					toWrite = WRITE_CHUNK_SIZE;
					remaining -= WRITE_CHUNK_SIZE;
					byteBlock += WRITE_CHUNK_SIZE;
				} else {
					delta = DRAM_SIZE - byteBlock;
					toWrite = delta;
					remaining -= delta;
					byteBlock += delta;
				}
			} else {
				if ((byteBlock + remaining) <= DRAM_SIZE) {
					toWrite = remaining;
					byteBlock += remaining;
					remaining = 0;
				} else {
					delta = DRAM_SIZE - byteBlock;
					toWrite = delta;
					remaining -= delta;
					byteBlock += delta;
				}
			}

			index = 0;
			buff[index++] = 0xFA;
			buff[index++] = (u8) ((addr & 0xFF000000) >> 24);
			buff[index++] = (u8) ((addr & 0x00FF0000) >> 16);
			buff[index++] = (u8) ((addr & 0x0000FF00) >> 8);
			buff[index++] = (u8) ((addr & 0x000000FF));

			memcpy(&buff[index], data, toWrite);

			rc = info->fts_write_reg(info, &buff[0], index + toWrite);
			if (rc <= 0) {
				input_err(true, &info->client->dev,
						"%s failed to write i2c register. ret:%d\n",
						__func__, rc);
				return -1;
			}
			fts_delay(5);

			addr += toWrite;
			data += toWrite;
		}

		input_info(true, &info->client->dev, "%s: Write %d Bytes\n", __func__, byteBlock);

		//configuring the DMA
		byteBlock = byteBlock / 4 - 1;

		index = 0;
		buff2[index++] = 0xFA;
		buff2[index++] = 0x20;
		buff2[index++] = 0x00;
		buff2[index++] = 0x00;
		buff2[index++] = 0x72;
		buff2[index++] = 0x00;
		buff2[index++] = 0x00;

		addr = address + ((wheel * DRAM_SIZE) / 4);
		buff2[index++] = (u8) ((addr & 0x000000FF));
		buff2[index++] = (u8) ((addr & 0x0000FF00) >> 8);
		buff2[index++] = (u8) (byteBlock & 0x000000FF);
		buff2[index++] = (u8) ((byteBlock & 0x0000FF00) >> 8);
		buff2[index++] = 0x00;

		rc = info->fts_write_reg(info, &buff2[0], index);
		if (rc <= 0) {
			input_err(true, &info->client->dev,
					"%s failed to write i2c register. ret:%d\n",
					__func__, rc);
			return -1;
		}
		fts_delay(10);

		rc = FTS_Check_DMA_StartAndDone(info);
		if (rc < 0)
			return -1;

		wheel++;
	}

	return 0;
}

static int fts_fw_burn(struct fts_ts_info *info, u8 *fw_data)
{
	const struct fts_header *fw_header;
	u8 *pFWData;
	int rc;
	int i;
	u8 regAdd[FTS_EVENT_SIZE] = {0};
	int NumberOfMainBlock = 0;

	fw_header = (struct fts_header *) &fw_data[0];
	
	if ((fw_header->fw_area_bs) > 0)
		NumberOfMainBlock = fw_header->fw_area_bs;
	else
		NumberOfMainBlock = 26; // original value

	// System Reset
	regAdd[0] = 0xFA;	regAdd[1] = 0x20;	regAdd[2] = 0x00;
	regAdd[3] = 0x00;	regAdd[4] = 0x24;	regAdd[5] = 0x81;
	rc = info->fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0)
		return rc;

	fts_delay(50);

	// System Reset and Hold
	regAdd[0] = 0xFA;	regAdd[1] = 0x20;	regAdd[2] = 0x00;
	regAdd[3] = 0x00;	regAdd[4] = 0x24;	regAdd[5] = 0x01;
	rc = info->fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0)
		return rc;
	fts_delay(200);

		// Enable UVLO 
	regAdd[0] = 0xFA;	regAdd[1] = 0x20;	regAdd[2] = 0x00;
	regAdd[3] = 0x00;	regAdd[4] = 0x1B;	regAdd[5] = 0x66;
	rc = info->fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0)
		return rc;
	fts_delay(30);
	
		// Flash Auto Power Down 
	regAdd[0] = 0xFA;	regAdd[1] = 0x20;	regAdd[2] = 0x00;
	regAdd[3] = 0x00;	regAdd[4] = 0x68;	regAdd[5] = 0x13;
	rc = info->fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0)
		return rc;
	fts_delay(30);

	// Change application mode
	regAdd[0] = 0xFA;	regAdd[1] = 0x20;	regAdd[2] = 0x00;
	regAdd[3] = 0x00;	regAdd[4] = 0x25;	regAdd[5] = 0x20;
	rc = info->fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0)
		return rc;
	fts_delay(200);

	// Unlock Flash
	regAdd[0] = 0xFA;	regAdd[1] = 0x20;	regAdd[2] = 0x00;
	regAdd[3] = 0x00;	regAdd[4] = 0xDE;	regAdd[5] = 0x03;
	rc = info->fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0)
		return rc;
	fts_delay(200);

	regAdd[0] = 0xFA; regAdd[1] = 0x20; regAdd[2] = 0x00;
	regAdd[3] = 0x00; regAdd[4] = 0x6B; regAdd[5] = 0x00;
	rc = info->fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0)
		return rc;
	fts_delay(50);
	

	//==================== Erase Partial Flash ====================
	input_info(true, &info->client->dev, "%s: Start Flash(Main Block) Erasing\n", __func__);
	for (i = 0; i < NumberOfMainBlock; i++) {

		regAdd[0] = 0xFA; regAdd[1] = 0x20; regAdd[2] = 0x00;
		regAdd[3] = 0x00; regAdd[4] = 0x6A;
		regAdd[5] = (0x80 + i) & 0xFF;
		rc = info->fts_write_reg(info, &regAdd[0], 6);
		if (rc < 0)
			return rc;
		rc = FTS_Check_Erase_Done(info);
		if (rc < 0)
			return rc;
	}
	
	input_info(true, &info->client->dev, "%s: Start Flash(Config Block) Erasing\n", __func__);
	regAdd[0] = 0xFA; regAdd[1] = 0x20; regAdd[2] = 0x00;
	regAdd[3] = 0x00; regAdd[4] = 0x6B; regAdd[5] = 0x00;
	info->fts_write_reg(info, &regAdd[0], 6);
	fts_delay(50);

	regAdd[0] = 0xFA; regAdd[1] = 0x20; regAdd[2] = 0x00;
	regAdd[3] = 0x00; regAdd[4] = 0x6A;
	regAdd[5] = (0x80 + 31) & 0xFF;
	info->fts_write_reg(info, &regAdd[0], 6);

	rc = FTS_Check_Erase_Done(info);
	if (rc < 0)
		return rc;
	// Code Area
	if (fw_header->sec0_size > 0) {
		pFWData = (u8 *) &fw_data[FW_HEADER_SIZE];

		input_info(true, &info->client->dev, "%s: Start Flashing for Code\n", __func__);
		rc = fts_fw_fillFlash(info, CODE_ADDR_START, &pFWData[0], fw_header->sec0_size);
		if (rc < 0)
			return rc;

		input_info(true, &info->client->dev, "%s: Finished total flashing %ld Bytes for Code\n",
				__func__, (long)fw_header->sec0_size);
	}

	// Config Area
	if (fw_header->sec1_size > 0) {
		input_info(true, &info->client->dev, "%s: Start Flashing for Config\n", __func__);
		pFWData = (u8 *) &fw_data[FW_HEADER_SIZE + fw_header->sec0_size];
		rc = fts_fw_fillFlash(info, CONFIG_ADDR_START, &pFWData[0], fw_header->sec1_size);
		if (rc < 0)
			return rc;
		input_info(true, &info->client->dev, "%s: Finished total flashing %ld Bytes for Config\n",
				__func__, (long)fw_header->sec1_size);
	}

	// CX Area
	if (fw_header->sec2_size > 0) {
		input_info(true, &info->client->dev, "%s: Start Flashing for CX\n", __func__);
		pFWData = (u8 *) &fw_data[FW_HEADER_SIZE + fw_header->sec0_size + fw_header->sec1_size];
		rc = fts_fw_fillFlash(info, CX_ADDR_START, &pFWData[0], fw_header->sec2_size);
		if (rc < 0)
			return rc;
		input_info(true, &info->client->dev, "%s: Finished total flashing %ld Bytes for CX\n",
				__func__, (long)fw_header->sec2_size);
	}

	regAdd[0] = 0xFA;	regAdd[1] = 0x20;	regAdd[2] = 0x00;
	regAdd[3] = 0x00;	regAdd[4] = 0x24;	regAdd[5] = 0x80;
	rc = info->fts_write_reg(info, &regAdd[0], 6);
	if (rc < 0)
		return rc;
	fts_delay(200);

	// System Reset
	info->fts_systemreset(info, 0);

	return 0;
}

int fts_fw_wait_for_event(struct fts_ts_info *info, u8 *result, u8 result_cnt)
{
	int rc = 0;
	int i;
	bool matched = false;
	u8 regAdd;
	u8 data[FTS_EVENT_SIZE];
	int retry = 0;

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd = FTS_READ_ONE_EVENT;
	rc = -1;
	while (info->fts_read_reg(info, &regAdd, 1, (u8 *)data, FTS_EVENT_SIZE)) {
		for (i = 0; i < result_cnt; i++) {
			if (data[i] != result[i]) {
				matched = false;
				break;
			}
			matched = true;
		}

		if (matched == true) {
			rc = 0;
			break;
		}

		if (data[0] == FTS_EVENT_ERROR_REPORT) {
			input_info(true, &info->client->dev, "%s: Error detected %02X,%02X,%02X,%02X,%02X,%02X\n",
					__func__, data[0], data[1], data[2], data[3], data[4], data[5]);
			break;
		}

		if (retry++ > FTS_RETRY_COUNT * 15) {
			rc = -1;
			input_err(true, &info->client->dev, "%s: Time Over (%02X,%02X,%02X,%02X,%02X,%02X)\n",
					__func__, data[0], data[1], data[2], data[3], data[4], data[5]);
			break;
		}
		fts_delay(20);
	}

	return rc;
}

int fts_fw_wait_for_echo_event(struct fts_ts_info *info, u8 *cmd, u8 cmd_cnt, int delay)
{
	int rc = 0;
	int i;
	bool matched = false;
	u8 regAdd;
	u8 data[FTS_EVENT_SIZE];
	int retry = 0;

	mutex_lock(&info->wait_for);

	rc = info->fts_write_reg(info, cmd, cmd_cnt);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to write command\n", __func__);
		mutex_unlock(&info->wait_for);
		return rc;
	}

	if (delay)
		fts_delay(delay);

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd = FTS_READ_ONE_EVENT;
	rc = -1;
	while (info->fts_read_reg(info, &regAdd, 1, (u8 *)data, FTS_EVENT_SIZE)) {
		if (data[0] != 0x00)
			input_info(true, &info->client->dev,
					"%s: event %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X\n",
					__func__, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
					data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

		if ((data[0] == FTS_EVENT_STATUS_REPORT) && (data[1] == 0x01)) {  // Check command ECHO
			int loop_cnt;

			if (cmd_cnt > 4)
				loop_cnt = 4;
			else
				loop_cnt = cmd_cnt;

			for (i = 0; i < loop_cnt; i++) {
				if (data[i + 2] != cmd[i]) {
					matched = false;
					break;
				}
				matched = true;
			}

			if (matched == true) {
				rc = 0;
				break;
			}
		} else if (data[0] == FTS_EVENT_ERROR_REPORT) {
			input_info(true, &info->client->dev, "%s: Error detected %02X,%02X,%02X,%02X,%02X,%02X\n",
				__func__, data[0], data[1], data[2], data[3], data[4], data[5]);

			if (retry >= FTS_RETRY_COUNT) 
				break;
		}

		if (retry++ > FTS_RETRY_COUNT * 50) {
			rc = -1;
			input_err(true, &info->client->dev, "%s: Time Over (%02X,%02X,%02X,%02X,%02X,%02X)\n",
				__func__, data[0], data[1], data[2], data[3], data[4], data[5]);
			break;
		}
		fts_delay(20);
	}

	mutex_unlock(&info->wait_for);

	return rc;
}

#if 0
/* block on winner model / use other tablet models */
static void fts_set_factory_history_data(struct fts_ts_info *info, u8 level)
{
	int ret;
	u8 regaddr[3] = { 0 };
	u8 wlevel;

	switch (level) {
	case OFFSET_FAC_NOSAVE:
		input_info(true, &info->client->dev, "%s: not save to flash area\n", __func__);
		return;
	case OFFSET_FAC_SUB:
		wlevel = OFFSET_FW_SUB;
		break;
	case OFFSET_FAC_MAIN:
		wlevel = OFFSET_FW_MAIN;
		break;
	default:
		input_info(true, &info->client->dev, "%s: wrong level %d\n", __func__, level);
		return;
	}

	regaddr[0] = 0xC7;
	regaddr[1] = 0x04;
	regaddr[2] = wlevel;

	ret = info->fts_write_reg(info, regaddr, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to write factory level %d\n", __func__, wlevel);
		return;
	}

	regaddr[0] = 0xA4;
	regaddr[1] = 0x05;
	regaddr[2] = 0x04; /* panel configuration area */
	ret = info->fts_write_reg(info, regaddr, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to save flash (level=%d)\n", __func__, wlevel);
		return;
	}

	fts_delay(200);

	ret = fts_fw_wait_for_echo_event(info, regaddr, 3);
	if (ret < 0)
		return;

	input_info(true, &info->client->dev, "%s: save to flash area, level=%d\n", __func__, wlevel);
	return;
}
#endif

#ifdef TCLM_CONCEPT
int fts_tclm_execute_force_calibration(struct i2c_client *client, int cal_mode)
{
	struct fts_ts_info *info = (struct fts_ts_info *)i2c_get_clientdata(client);

	return fts_execute_autotune(info, true);
}
#endif

int fts_execute_autotune(struct fts_ts_info *info, bool IsSaving)
{
	u8 regAdd[FTS_EVENT_SIZE] = {0,};
	u8 DataType = 0x00;
	int rc;

	input_info(true, &info->client->dev, "%s: start\n", __func__);

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_interrupt_set(info, INT_DISABLE);

	// w A4 00 03
	if (IsSaving == true) {
		// full panel init
		regAdd[0] = 0xA4; regAdd[1] = 0x00; regAdd[2] = 0x03;

		rc = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 500);
#ifdef TCLM_CONCEPT
		if (info->tdata->nvdata.cal_fail_cnt == 0xFF)
			info->tdata->nvdata.cal_fail_cnt = 0;
		if (rc < 0) {
			info->tdata->nvdata.cal_fail_cnt++;
			info->tdata->nvdata.cal_fail_falg = 0;
		} else {
			info->tdata->nvdata.cal_fail_falg = SEC_CAL_PASS;
			info->is_cal_done = true;
		}
		info->tdata->tclm_write(info->tdata->client, SEC_TCLM_NVM_ALL_DATA);
#endif
		if (rc < 0) {
			input_info(true, &info->client->dev, "%s: timeout\n", __func__);
			goto ERROR;
		}
	} else {
		// SS ATune
		//DataType = 0x0C;
		DataType = 0x3F;

		regAdd[0] = 0xA4; regAdd[1] = 0x03; regAdd[2] = (u8)DataType; regAdd[3] = 0x00;

		rc = fts_fw_wait_for_echo_event(info, &regAdd[0], 4, 500);
		if (rc < 0) {
			input_info(true, &info->client->dev, "%s: timeout\n", __func__);
			goto ERROR;
		}
	}

	/* block on winner model / use other tablet models */
//	fts_set_factory_history_data(info, info->factory_position);
	if (IsSaving == true)
		fts_panel_ito_test(info, SAVE_MISCAL_REF_RAW);

ERROR:

	info->factory_position = OFFSET_FAC_NOSAVE;
	fts_interrupt_set(info, INT_ENABLE);

	return rc;
}

static const int fts_fw_updater(struct fts_ts_info *info, u8 *fw_data)
{
	const struct fts_header *header;
	int retval;
	int retry;
	u16 fw_main_version;

	if (!fw_data) {
		input_err(true, &info->client->dev, "%s: Firmware data is NULL\n",
				__func__);
		return -ENODEV;
	}

	header = (struct fts_header *)fw_data;
	fw_main_version = (u16)header->ext_release_ver;

	input_info(true, &info->client->dev,
			"%s: Starting firmware update : 0x%04X\n", __func__,
			fw_main_version);

	retry = 0;

	fts_interrupt_set(info, INT_DISABLE);
	while (1) {
		retval = fts_fw_burn(info, fw_data);
		if (retval >= 0) {
			info->fts_get_version_info(info);

			if (fw_main_version == info->fw_main_version_of_ic) {
				input_info(true, &info->client->dev,
						"%s: Success Firmware update\n",
						__func__);
				info->fw_corruption = false;

				retval = info->fts_systemreset(info, 0);

				if (retval == -FTS_ERROR_BROKEN_OSC_TRIM) {
					retval = fts_osc_trim_recovery(info);
					if (retval < 0)
						input_err(true, &info->client->dev, "%s: Failed to recover osc trim\n", __func__);
					else
						info->fw_corruption = false;
				}

				fts_set_scanmode(info, info->scan_mode);
				retval = 0;
				break;
			}
		}

		if (retry++ > 3) {
			input_err(true, &info->client->dev, "%s: Fail Firmware update\n",
					__func__);
			retval = -1;
			break;
		}
	}
	fts_interrupt_set(info, INT_ENABLE);

	return retval;
}

int fts_fw_update_on_probe(struct fts_ts_info *info)
{
	int retval = 0, ret = 0;
	const struct firmware *fw_entry = NULL;
	u8 *fw_data = NULL;
	char fw_path[FTS_MAX_FW_PATH];
	const struct fts_header *header;
	bool restore_cal = false;

#ifdef TCLM_CONCEPT
	if (info->tdata->support_tclm_test) {
		ret = sec_tclm_test_on_probe(info->tdata);
		if (ret < 0)
			input_info(true, &info->client->dev, "%s: SEC_TCLM_NVM_ALL_DATA i2c read fail", __func__);
	}
#endif

	if (info->board->bringup == 1)
		return 0;

	if (info->board->firmware_name) {
		info->firmware_name = info->board->firmware_name;
	} else {
		input_err(true, &info->client->dev, "%s: firmware name does not declair in dts\n", __func__);
		goto exit_fwload;
	}

	snprintf(fw_path, FTS_MAX_FW_PATH, "%s", info->firmware_name);
	input_info(true, &info->client->dev, "%s: Load firmware : %s, TSP_ID : %d\n", __func__, fw_path, info->board->tsp_id);

	retval = request_firmware(&fw_entry, fw_path, &info->client->dev);
	if (retval) {
		input_err(true, &info->client->dev,
				"%s: Firmware image %s not available\n", __func__,
				fw_path);
		goto done;
	}

	fw_data = (u8 *)fw_entry->data;
	header = (struct fts_header *)fw_data;

	info->fw_version_of_bin = (u16)header->fw_ver;
	info->fw_main_version_of_bin = (u16)header->ext_release_ver;
	info->config_version_of_bin = (u16)header->cfg_ver;
	info->project_id_of_bin = header->project_id;
	info->ic_name_of_bin = header->ic_name;
	info->module_version_of_bin = header->module_ver;

	input_info(true, &info->client->dev,
			"%s: [BIN] Firmware Ver: 0x%04X, Config Ver: 0x%04X, Main Ver: 0x%04X\n", __func__,
			info->fw_version_of_bin,
			info->config_version_of_bin,
			info->fw_main_version_of_bin);
	input_info(true, &info->client->dev,
			"%s: [BIN] Project ID: 0x%02X, IC Name: 0x%02X, Module Ver: 0x%02X\n",
			__func__, info->project_id_of_bin,
			info->ic_name_of_bin, info->module_version_of_bin);

	if (info->board->bringup == 2) {
		input_err(true, &info->client->dev, "%s: skip fw_update for bringup\n", __func__);
		retval = FTS_NOT_ERROR;
		goto done;
	}

	if (info->checksum_result)
		retval = FTS_NEED_FW_UPDATE;
	else if ((info->fw_main_version_of_ic < info->fw_main_version_of_bin)
			|| ((info->config_version_of_ic < info->config_version_of_bin))
			|| ((info->fw_version_of_ic < info->fw_version_of_bin)))
		retval = FTS_NEED_FW_UPDATE;
	else
		retval = FTS_NOT_ERROR;

	/* ic fw ver > bin fw ver && force is false */
	if (retval != FTS_NEED_FW_UPDATE) {
		input_err(true, &info->client->dev, "%s: skip fw update\n", __func__);

		goto done;
	}

	retval = fts_fw_updater(info, fw_data);
	if (retval < 0)
		goto done;

#ifdef TCLM_CONCEPT
	ret = info->tdata->tclm_read(info->tdata->client, SEC_TCLM_NVM_ALL_DATA);
	if (ret < 0) {
		input_info(true, &info->client->dev, "%s: SEC_TCLM_NVM_ALL_DATA i2c read fail", __func__);
		goto done;
	}

	input_info(true, &info->client->dev, "%s: tune_fix_ver [%04X] afe_base [%04X]\n",
		__func__, info->tdata->nvdata.tune_fix_ver, info->tdata->afe_base);

	if ((info->tdata->tclm_level > TCLM_LEVEL_CLEAR_NV) &&
		((info->tdata->nvdata.tune_fix_ver == 0xffff)
		|| (info->tdata->afe_base > info->tdata->nvdata.tune_fix_ver))) {
		/* tune version up case */
		sec_tclm_root_of_cal(info->tdata, CALPOSITION_TUNEUP);
		restore_cal = true;
	} else if (info->tdata->tclm_level == TCLM_LEVEL_CLEAR_NV) {
		/* firmup case */
		sec_tclm_root_of_cal(info->tdata, CALPOSITION_FIRMUP);
		restore_cal = true;
	}

	if (restore_cal) {
		input_info(true, &info->client->dev, "%s: RUN OFFSET CALIBRATION\n", __func__);
		if (sec_execute_tclm_package(info->tdata, 0) < 0)
			input_err(true, &info->client->dev, "%s: sec_execute_tclm_package fail\n", __func__);
	}
#endif

done:
#ifdef TCLM_CONCEPT
	sec_tclm_root_of_cal(info->tdata, CALPOSITION_NONE);
#endif
	if (fw_entry)
		release_firmware(fw_entry);
exit_fwload:
	return retval;
}
EXPORT_SYMBOL(fts_fw_update_on_probe);

static int fts_load_fw_from_kernel(struct fts_ts_info *info,
		const char *fw_path)
{
	int retval;
	const struct firmware *fw_entry = NULL;
	u8 *fw_data = NULL;

	if (!fw_path) {
		input_err(true, &info->client->dev, "%s: Firmware name is not defined\n",
				__func__);
		return -EINVAL;
	}

	input_info(true, &info->client->dev, "%s: Load firmware : %s\n", __func__,
			fw_path);

	retval = request_firmware(&fw_entry, fw_path, &info->client->dev);

	if (retval) {
		input_err(true, &info->client->dev,
				"%s: Firmware image %s not available\n", __func__,
				fw_path);
		goto done;
	}

	fw_data = (u8 *)fw_entry->data;

	info->fts_systemreset(info, 20);

#ifdef TCLM_CONCEPT
	sec_tclm_root_of_cal(info->tdata, CALPOSITION_TESTMODE);
#endif

	retval = fts_fw_updater(info, fw_data);
	if (retval)
		input_err(true, &info->client->dev, "%s: failed update firmware\n",
				__func__);

#ifdef TCLM_CONCEPT
	input_info(true, &info->client->dev, "%s: RUN OFFSET CALIBRATION\n", __func__);
	if (sec_execute_tclm_package(info->tdata, 0) < 0)
		input_err(true, &info->client->dev, "%s: sec_execute_tclm_package fail\n", __func__);

	sec_tclm_root_of_cal(info->tdata, CALPOSITION_NONE);
#endif

done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}

static int fts_load_fw_from_ums(struct fts_ts_info *info, int type)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fw_size, nread;
	int error = 0;
	int spu_ret = 0;
	int ori_size = 0;
	char file_path[100];

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	switch (type) {
	case TSP_TYPE_EXTERNAL_FW:
		snprintf(file_path, sizeof(file_path), TSP_PATH_EXTERNAL_FW);
		break;
	case TSP_TYPE_EXTERNAL_FW_SIGNED:
		snprintf(file_path, sizeof(file_path), TSP_PATH_EXTERNAL_FW_SIGNED);
		break;
	case TSP_TYPE_SPU_FW_SIGNED:
		snprintf(file_path, sizeof(file_path), TSP_PATH_SPU_FW_SIGNED);
		break;
	default:
		return -ENODEV;
	}

	fp = filp_open(file_path, O_RDONLY, 0400);
	if (IS_ERR(fp)) {
		input_err(true, &info->client->dev, "%s: failed to open %s.\n", __func__, file_path);
		error = -ENOENT;
		goto open_err;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (fw_size > 0) {
		u8 *fw_data;
		const struct fts_header *header;

		fw_data = kzalloc(fw_size, GFP_KERNEL);
		if (!fw_data) {
			error = -ENOMEM;
			goto alloc_err;
		}

		nread = vfs_read(fp, (char __user *)fw_data,
				fw_size, &fp->f_pos);

		input_info(true, &info->client->dev,
				"%s: start, file path %s, size %ld Bytes\n",
				__func__, file_path, fw_size);

		if (nread != fw_size) {
			input_err(true, &info->client->dev,
					"%s: failed to read firmware file, nread %ld Bytes\n",
					__func__, nread);
			error = -EIO;
		} else {
			header = (struct fts_header *)fw_data;
			if (header->signature == FTSFILE_SIGNATURE) {
				/* If FFU firmware version is lower than IC's version, do not run update routine */
				if (type == TSP_TYPE_EXTERNAL_FW_SIGNED || type == TSP_TYPE_SPU_FW_SIGNED) {
					/* digest 32, signature 512 TSP 3 */
					ori_size = fw_size - SPU_METADATA_SIZE(TSP);

					spu_ret = spu_firmware_signature_verify("TSP", fw_data, fw_size);
					if (ori_size != spu_ret) {
						error = -1;
						input_err(true, &info->client->dev,
								"%s: signature verify failed: spu_ret:%d, ori_size:%d",
								__func__, spu_ret, ori_size);
						kfree(fw_data);
						goto alloc_err;
					}
				}

				if (type == TSP_TYPE_SPU_FW_SIGNED) {
					if ((info->fw_main_version_of_ic < (u16)header->ext_release_ver)
								|| (info->config_version_of_ic < (u16)header->cfg_ver)
								|| (info->fw_version_of_ic < (u16)header->fw_ver)) {
						input_info(true, &info->client->dev, "%s: run spu update\n", __func__);
					} else {
						error = 0;
						input_info(true, &info->client->dev, "%s: skip spu update\n", __func__);
						kfree(fw_data);
						goto alloc_err;
					}
				} else if (type == TSP_TYPE_EXTERNAL_FW_SIGNED) {
					if ((info->ic_name_of_ic == header->ic_name) &&
							(info->project_id_of_ic == header->project_id)) {
						input_info(true, &info->client->dev, "%s: run external fw update\n", __func__);
					} else {
						error = 0;
						input_info(true, &info->client->dev, "%s: skip external fw update\n", __func__);
						kfree(fw_data);
						goto alloc_err;
					}
				}
				info->fts_systemreset(info, 0);

				input_info(true, &info->client->dev,
						"%s: [UMS] Firmware Ver: 0x%04X, Main Version : 0x%04X\n",
						__func__, (u16)header->fw_ver, (u16)header->ext_release_ver);
#ifdef TCLM_CONCEPT
				sec_tclm_root_of_cal(info->tdata, CALPOSITION_TESTMODE);
#endif

				error = fts_fw_updater(info, fw_data);

#ifdef TCLM_CONCEPT
				input_info(true, &info->client->dev, "%s: RUN OFFSET CALIBRATION\n", __func__);
				if (sec_execute_tclm_package(info->tdata, 0) < 0)
					input_err(true, &info->client->dev, "%s: sec_execute_tclm_package fail\n", __func__);

				sec_tclm_root_of_cal(info->tdata, CALPOSITION_NONE);
#endif
			} else {
				error = -1;
				input_err(true, &info->client->dev,
						"%s: File type is not match with FTS64 file. [%8x]\n",
						__func__, header->signature);
			}
		}

		if (error < 0)
			input_err(true, &info->client->dev, "%s: failed update firmware\n",
					__func__);

		kfree(fw_data);
	}

alloc_err:
	filp_close(fp, NULL);

open_err:
	set_fs(old_fs);
	return error;
}

int fts_fw_update_on_hidden_menu(struct fts_ts_info *info, int update_type)
{
	int retval = 0;

	/* Factory cmd for firmware update
	 * argument represent what is source of firmware like below.
	 *
	 * 0 : [BUILT_IN] Getting firmware which is for user.
	 * 1 : [UMS] Getting firmware from sd card.
	 * 2 : none
	 */
	switch (update_type) {
	case BUILT_IN:
		retval = fts_load_fw_from_kernel(info, info->firmware_name);
		break;
	case UMS:
#if defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		retval = fts_load_fw_from_ums(info, TSP_TYPE_EXTERNAL_FW_SIGNED);
#else
		retval = fts_load_fw_from_ums(info, TSP_TYPE_EXTERNAL_FW);
#endif
		break;
	case SPU:
		retval = fts_load_fw_from_ums(info, TSP_TYPE_SPU_FW_SIGNED);
		break;
	default:
		input_err(true, &info->client->dev, "%s: Not support command[%d]\n",
				__func__, update_type);
		break;
	}

	return retval;
}
EXPORT_SYMBOL(fts_fw_update_on_hidden_menu);
