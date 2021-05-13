/*
 * =================================================================
 *
 *
 *	Description:  samsung display panel file
 *
 *	Author: jb09.kim
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2017, Samsung Electronics. All rights reserved.

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
#include "ss_dsi_panel_S6TUUM3_AMSA24VU01.h"
#include "ss_dsi_mdnie_S6TUUM3_AMSA24VU01.h"

#if 0
/* AOD Mode status on AOD Service */
enum {
	HLPM_CTRL_2NIT,
	HLPM_CTRL_10NIT,
	HLPM_CTRL_30NIT,
	HLPM_CTRL_60NIT,
	MAX_LPM_CTRL,
};
#endif

#define IRC_MODERATO_MODE_VAL	0x6B
#define IRC_FLAT_GAMMA_MODE_VAL	0x2B
#define get_bit(value, shift, width)	((value >> shift) & (GENMASK(width - 1, 0)))
int prev_bl_s6tuum3; /* To save previous brightness level */
static int prev_refresh_rate;
ktime_t time_vrr;
int init_smooth_off;
int poc_done;

static struct dsi_panel_cmd_set *__ss_vrr(struct samsung_display_driver_data *vdd,
					int *level_key, bool is_hbm, bool is_hmt)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_panel_cmd_set  *vrr_cmds;

	struct vrr_info *vrr = &vdd->vrr;
	enum SS_BRR_MODE brr_mode = vrr->brr_mode;

	int cur_rr;
	bool cur_hs;
	s64 vrr_gap;

	vrr_cmds = ss_get_cmds(vdd, TX_VRR);

	if (SS_IS_CMDS_NULL(vrr_cmds)) {
		LCD_INFO("no vrr cmds\n");
		return NULL;
	}

	if (panel && panel->cur_mode) {
		LCD_INFO("VRR: cur_mode: %dx%d@%d%s, %s, running:%d\n",
				panel->cur_mode->timing.h_active,
				panel->cur_mode->timing.v_active,
				panel->cur_mode->timing.refresh_rate,
				panel->cur_mode->timing.sot_hs_mode ? "HS" : "NM",
				ss_get_brr_mode_name(brr_mode), vrr->running_vrr);

		if (panel->cur_mode->timing.refresh_rate != vdd->vrr.adjusted_refresh_rate ||
				panel->cur_mode->timing.sot_hs_mode != vdd->vrr.adjusted_sot_hs_mode)
			LCD_ERR("VRR: unmatched RR mode (%dhz%s / %dhz%s)\n",
					panel->cur_mode->timing.refresh_rate,
					panel->cur_mode->timing.sot_hs_mode ? "HS" : "NM",
					vdd->vrr.adjusted_refresh_rate,
					vdd->vrr.adjusted_sot_hs_mode ? "HS" : "NM");
	}

	cur_rr = vrr->cur_refresh_rate;
	cur_hs = vrr->cur_sot_hs_mode;

	switch (cur_rr) {
		case 120 :
			if (prev_refresh_rate == 96) { /* 96->120 */
				vrr_cmds = NULL;
				vrr_cmds = ss_get_cmds(vdd, DSI_CMD_SET_TIMING_SWITCH);
			} else { /* 60->120hz : 60h 20 */
				vrr_cmds->cmds[0].msg.tx_buf[1]= 0x20;
			}
			break;
		case 60 : /* 60h 20 */
			vrr_cmds->cmds[0].msg.tx_buf[1]= 0x00;

			break;
		case 96 : /* Always 120hz -> 96hz */
			vrr_cmds = ss_get_cmds(vdd, DSI_CMD_SET_TIMING_SWITCH);
			break;
	}

	LCD_INFO("VRR: %s, FPS:%dhz-> %dhz%s (cur: %d%s, target: %d%s)\n",
			ss_get_brr_mode_name(brr_mode),
			prev_refresh_rate, cur_rr,
			cur_hs ? "HS" : "NM",
			vrr->cur_refresh_rate,
			vrr->cur_sot_hs_mode ? "HS" : "NM",
			vrr->adjusted_refresh_rate,
			vrr->adjusted_sot_hs_mode ? "HS" : "NM");

	prev_refresh_rate = vrr->cur_refresh_rate;

	if (time_vrr) { /* if not zero */
		vrr_gap = ktime_ms_delta(ktime_get(), time_vrr);

		if (vrr_gap < 15) { /* Add delay to make 2frame btw freq cmds */
			LCD_INFO("VRR: vrr_gap = %lld ms\n", vrr_gap);
#if 0	/* Hold adding delay */
			usleep_range((15-vrr_gap) * 1000, (15-vrr_gap) * 1000);
			LCD_INFO("VRR: vrr_gap = %lld ms. Waited for %lld ms\n", vrr_gap, (15-vrr_gap));
#endif
		}
	}

	/* save timestamp for freq cmds */
	time_vrr = ktime_get();

	return vrr_cmds;
}


static struct dsi_panel_cmd_set *ss_vrr(struct samsung_display_driver_data *vdd, int *level_key)
{
	bool is_hbm = false;
	bool is_hmt = false;

	return __ss_vrr(vdd, level_key, is_hbm, is_hmt);
}

static struct dsi_panel_cmd_set *ss_vrr_hbm(struct samsung_display_driver_data *vdd, int *level_key)
{
	bool is_hbm = true;
	bool is_hmt = false;

	return __ss_vrr(vdd, level_key, is_hbm, is_hmt);
}

static int ss_fw_id_read(struct samsung_display_driver_data *vdd)
{
	u8 ddi_fw_id[4];
	int fw_id = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}
	/* Read mtp (F7h 1~4th) for DDI FW REV */
	if (ss_get_cmds(vdd, RX_DDI_FW_ID)) {
		ss_panel_data_read(vdd, RX_DDI_FW_ID, ddi_fw_id, LEVEL_KEY_NONE);
		//fw_id = ddi_fw_id[0] << 24 | ddi_fw_id[1] << 16 | ddi_fw_id[2] << 8 | ddi_fw_id[3];
		fw_id = ddi_fw_id[0];
		LCD_INFO("DSI%d fw_id: %02x %02x %02x %02x => %x \n", vdd->ndx,
			ddi_fw_id[0], ddi_fw_id[1], ddi_fw_id[2], ddi_fw_id[3], fw_id);
	} else {
		LCD_ERR("DSI%d no ddi_fw_id_rx_cmds cmds", vdd->ndx);
		return false;
	}
	return fw_id;
}

static int ss_fw_up_send_cmd(struct samsung_display_driver_data *vdd, enum dsi_cmd_set_type type, u32 delay_us)
{
	int ret = 0;
	u8 status_check[2] = {0,};

	ss_send_cmd(vdd, type);
	usleep_range(delay_us, delay_us + 10);
	ss_panel_data_read(vdd, RX_FW_UP_STATUS, status_check, LEVEL_KEY_NONE);
	if (status_check[0] != vdd->fw_up.read_status_value) {
		usleep_range(400000, 400010);
		ss_panel_data_read(vdd, RX_FW_UP_STATUS, status_check, LEVEL_KEY_NONE);
		if (status_check[0] != vdd->fw_up.read_status_value) {
			LCD_ERR("%s send Fail status_check = 0x%x\n", status_check[0]);
			return FW_UP_ERR_UPDATE_FAIL;
		}
	}

	return ret;
}

/* Flash erase related SP before fw_update */
static int ss_flash_erase(struct samsung_display_driver_data *vdd)
{
	int ret;
	struct dsi_panel_cmd_set *pcmds;

	LCD_INFO("SP related FirmWare ERASE Start +++\n");
	usleep_range(20000, 21000);

	/* Replace to Tcon SPI Floating before erase */
	pcmds = ss_get_cmds(vdd, TX_NORMAL_BRIGHTNESS_ETC);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_NORMAL_BRIGHTNESS_ETC..\n");
		return -ENODEV;
	}

	/* SPI Floating */
	/* [0] : B0 00 09 ->	B0 00 5C */
	/* [1] : 5D 00 ->  AD 01 */
	pcmds->cmds[0].msg.tx_buf[2] = 0x5C;
	pcmds->cmds[1].msg.tx_buf[0] = 0xAD;
	pcmds->cmds[1].msg.tx_buf[1] = 0x01;
	ss_send_cmd(vdd, TX_NORMAL_BRIGHTNESS_ETC);
	/* Restore data */
	/* [0] : B0 00 09 */
	/* [1] : 5D 00 */
	pcmds->cmds[0].msg.tx_buf[2] = 0x09;
	pcmds->cmds[1].msg.tx_buf[0] = 0x5D;
	pcmds->cmds[1].msg.tx_buf[1] = 0x00;

	pcmds = ss_get_cmds(vdd, TX_FW_UP_ERASE);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_FW_UP_ERASE..\n");
		return -ENODEV;
	}
	/* Replace data */
	/* flash# : tx_buf[0~1]  C0 00 -> C0 01 */
	/* addr  : tx_buf[3~5]  03 20 00 -> 00 70 00 */
	pcmds->cmds[0].msg.tx_buf[1] = 0x01;
	pcmds->cmds[0].msg.tx_buf[3] = 0x00;
	pcmds->cmds[0].msg.tx_buf[4] = 0x10;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, 500000);
	if (ret)
		LCD_ERR("FirmWare Erase Fail!!\n");

	pcmds->cmds[0].msg.tx_buf[4] = 0x20;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, 500000);
	if (ret)
		LCD_ERR("FirmWare Erase Fail!!\n");
	pcmds->cmds[0].msg.tx_buf[4] = 0x30;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, 500000);
	if (ret)
		LCD_ERR("FirmWare Erase Fail!!\n");
	pcmds->cmds[0].msg.tx_buf[4] = 0x40;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, 500000);
	if (ret)
		LCD_ERR("FirmWare Erase Fail!!\n");
	pcmds->cmds[0].msg.tx_buf[4] = 0x50;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, 500000);
	if (ret)
		LCD_ERR("FirmWare Erase Fail!!\n");
	pcmds->cmds[0].msg.tx_buf[4] = 0x60;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, 500000);
	if (ret)
		LCD_ERR("FirmWare Erase Fail!!\n");
	pcmds->cmds[0].msg.tx_buf[4] = 0x70;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, 500000);
	if (ret)
		LCD_ERR("FirmWare Erase Fail!!\n");

	pcmds->cmds[0].msg.tx_buf[4] = 0x80;
	pcmds->cmds[0].msg.tx_buf[7] = 0x80;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, 1000000);
	if (ret)
		LCD_ERR("FirmWare Erase Fail!!\n");

	/* Restore data */
	/* tx_buf[0~1]  C0 00 */
	/* tx_buf[3~5] 03 20 00 */
	pcmds->cmds[0].msg.tx_buf[1] = 0x00;
	pcmds->cmds[0].msg.tx_buf[3] = 0x03;
	pcmds->cmds[0].msg.tx_buf[4] = 0x20;
	pcmds->cmds[0].msg.tx_buf[7] = 0x10;

	// Test 5 minute 300, -33= 267 sec
#if 0
	wait_cnt = 267;
	while (--wait_cnt)
		msleep(1000);
#endif

	LCD_INFO("SP related flash ERASE END ---\n");

	return ret;
}

/*(Full) 64K FW update ID 17, 18 -> 19 */
static int ss_fw_write(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *erase_tx_cmds = NULL;
	struct dsi_panel_cmd_set *read_tx_cmds = NULL;
	struct dsi_panel_cmd_set *write_tx_cmds = NULL;
	int cur_addr, last_addr, write_size, buf_pos;
	int loop, mem_check_fail;
	int ret = 0;
	int retry_cnt;
	int retry_all_cnt;
	u8 read_data[128] = {0,}; //write_size
	u8 read_check[2] = {0,};
	u32 erase_delay_us, write_delay_us, read_delay_us;
	u32 img_size, start_addr, sector_size;
	char write_buffer[384] = {0,}; //write_size * 3

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	erase_tx_cmds = ss_get_cmds(vdd, TX_FW_UP_ERASE);
	if (SS_IS_CMDS_NULL(erase_tx_cmds)) {
		LCD_ERR("No cmds for TX_FW_ERASE..\n");
		return -ENODEV;
	}
	/* Replace data */
	//[0~1]  C0 00 Firmware //-> C0 01 Flash data
	//erase_tx_cmds->cmds[0].msg.tx_buf[1] = 0x01;
	//[3~5]  03 20 00 -> 00 00 00
	erase_tx_cmds->cmds[0].msg.tx_buf[3] = 0x00;
	erase_tx_cmds->cmds[0].msg.tx_buf[4] = 0x00;
	//[6~8]  00 10 00 4k erase // -> 01 00 00
	//erase_tx_cmds->cmds[0].msg.tx_buf[6] = 0x01;
	//erase_tx_cmds->cmds[0].msg.tx_buf[7] = 0x00;

	write_tx_cmds = ss_get_cmds(vdd, TX_FW_UP_WRITE);
	if (SS_IS_CMDS_NULL(write_tx_cmds)) {
		LCD_ERR("No cmds for TX_FW_UP_WRITE..\n");
		return -ENODEV;
	}

	read_tx_cmds = ss_get_cmds(vdd, TX_FW_UP_READ);
	if (SS_IS_CMDS_NULL(read_tx_cmds)) {
		LCD_ERR("No cmds for TX_FW_UP_READ..\n");
		return -ENODEV;
	}

	read_tx_cmds->cmds[0].msg.rx_buf = read_data;
	read_tx_cmds->state = DSI_CMD_SET_STATE_HS;

	retry_all_cnt = 10;
retry_all:
	start_addr = vdd->fw_up.start_addr;
	img_size = vdd->fw_up.image_size;
	last_addr = vdd->fw_up.start_addr + vdd->fw_up.image_size;
	erase_delay_us = vdd->fw_up.erase_delay_us;
	write_delay_us = vdd->fw_up.write_delay_us;
	read_delay_us = vdd->fw_up.read_delay_us;
	sector_size = vdd->fw_up.sector_size;
	write_size = vdd->fw_up.write_data_size;
	buf_pos = 0;

	/* FW WRITE */
	LCD_INFO("FW Write Start Address(0x%x), Sector Size(0x%x), Write Size(%d), Last Address(0x%x), delay %dus\n",
		start_addr, sector_size, write_size, last_addr, write_delay_us);

	for (cur_addr = start_addr; (cur_addr < last_addr) && (buf_pos < img_size); ) {
		retry_cnt = 10;
retry:
		/* Erase 1 Sector */
		if (sector_size && (cur_addr % sector_size == 0)) {
			erase_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.erase_addr_idx[0]]
								= (cur_addr & 0xFF0000) >> 16;
			erase_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.erase_addr_idx[1]]
								= (cur_addr & 0x00FF00) >> 8;
			erase_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.erase_addr_idx[2]]
								= (cur_addr & 0x0000FF);
			erase_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.erase_size_idx[0]]
								= (sector_size & 0xFF0000) >> 16;
			erase_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.erase_size_idx[1]]
								= (sector_size & 0x00FF00) >> 8;
			erase_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.erase_size_idx[2]]
								= (sector_size & 0x0000FF);

			ss_send_cmd(vdd, TX_FW_UP_ERASE);
			usleep_range(erase_delay_us, erase_delay_us + 5);

			ss_panel_data_read(vdd, RX_FW_UP_STATUS, read_check, LEVEL_KEY_NONE);
			if (read_check[0] != vdd->fw_up.read_status_value) {
				ss_write_fw_up_debug_partition(FW_FAIL_LINE, __LINE__);
				LCD_ERR("erase Fail status_check = 0x%x\n", read_check[0]);
				return FW_UP_ERR_UPDATE_FAIL;
			}

			LCD_INFO("Erase 1 sector, Address [0x%02x%02x%02x], Sector Size[0x%x] ret:%x\n",
				erase_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.erase_addr_idx[0]],
				erase_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.erase_addr_idx[1]],
				erase_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.erase_addr_idx[2]],
				sector_size, read_check[0]);
		}

		/* Write 1 Sector */
		for (; ; ) {
			int pos = 0;

			write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]]
								= (cur_addr & 0xFF0000) >> 16;
			write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]]
								= (cur_addr & 0x00FF00) >> 8;
			write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]]
								= (cur_addr & 0x0000FF);

			write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_size_idx[0]]
								= (write_size & 0xFF0000) >> 16;
			write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_size_idx[1]]
								= (write_size & 0x00FF00) >> 8;
			write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_size_idx[2]]
								= (write_size & 0x0000FF);

			if (cur_addr % sector_size == 0) {
				LCD_INFO("Write Address [0x%02x%02x%02x], Size[0x%02x%02x%02x = %d] \n",
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_size_idx[0]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_size_idx[1]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_size_idx[2]],
					write_size);
			} else {
				LCD_DEBUG("Write Address [0x%02x%02x%02x], Size[0x%02x%02x%02x = %d] \n",
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_size_idx[0]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_size_idx[1]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_size_idx[2]],
					write_size);
			}

			memcpy(&write_tx_cmds->cmds[0].msg.tx_buf[17], &fw_data_64k[buf_pos], write_size);

			for(loop = 0; loop < write_size; loop++)
				pos += snprintf(write_buffer + pos, sizeof(write_buffer) - pos, "%02x ", 
					write_tx_cmds->cmds[0].msg.tx_buf[17 + loop]);
			LCD_INFO("Write Buffer = (%s)", write_buffer);

			/* Write Data */
			ss_send_cmd(vdd, TX_FW_UP_WRITE);
			usleep_range(write_delay_us, write_delay_us + 5);

			ss_panel_data_read(vdd, RX_FW_UP_STATUS, read_check, LEVEL_KEY_NONE);
			if (read_check[0] != vdd->fw_up.read_status_value) {
				/* Wait more time */
				ss_write_fw_up_debug_partition(FW_FAIL_LINE, __LINE__);
				LCD_ERR("TX_FW_UP_WRITE Fail [0x%02x%02x%02x], read_check = 0x%x, Wait %dus more \n",
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]],
					write_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]],
					read_check[0], write_delay_us * 3);
				usleep_range(write_delay_us * 3, write_delay_us * 4);
			}
			/* Clear flash buffer */
			ss_send_cmd(vdd, TX_FLASH_CLEAR_BUFFER);

			cur_addr += write_size;
			buf_pos += write_size;
			if (sector_size && (cur_addr % sector_size == 0))
				break;
		}

		/* Read & Compare 1 Sector */
		cur_addr -= sector_size;
		buf_pos -= sector_size;
		for (; ; ) {
			/* Read Data */
			read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]]
								= (cur_addr & 0xFF0000) >> 16;
			read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]]
								= (cur_addr & 0x00FF00) >> 8;
			read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]]
								= (cur_addr & 0x0000FF);

			/* Tx read size set 0x83 instead of 0x80 to avoid ddi read bug. */
			ss_send_cmd(vdd, TX_FW_UP_READ);
			usleep_range(read_delay_us, read_delay_us + 5);
			/* Read 0x80 bytes in real */
			ss_panel_data_read(vdd, RX_FW_UP_STATUS, read_check, LEVEL_KEY_NONE);
			if (read_check[0] != vdd->fw_up.read_status_value) {
				/* Wait more time */
				ss_write_fw_up_debug_partition(FW_FAIL_LINE, __LINE__);
				LCD_ERR("TX_FW_READ Fail [0x%02x%02x%02x], read_check = 0x%x, Wait %dus more \n",
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]],
					read_check[0], write_delay_us);
				usleep_range(write_delay_us, write_delay_us * 2);
			}

			/* Compare Write Data & Read Data */
			memset(read_data, 0x00, 128);
			ss_panel_data_read(vdd, RX_FW_UP_READ, read_data, LEVEL_KEY_NONE);
			mem_check_fail = false;
			for (loop = 0; loop < 128 ; loop++) {
				if (read_data[loop] != fw_data_64k[buf_pos + loop]) {
					mem_check_fail = true;

					ss_write_fw_up_debug_partition(FW_UP_FAIL, cur_addr);
					break;
				}
			}
			if (mem_check_fail) {
				usleep_range(write_delay_us * 2, write_delay_us * 3);
				ss_write_fw_up_debug_partition(FW_FAIL_LINE, __LINE__);
				LCD_ERR("FW Check Fail!! Address [0x%02x%02x%02x][%d] \n",
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]], loop);
				retry_cnt--;
				LCD_INFO("Retry Cnt = %d, cur_addr = 0x%x, buf_pos = 0x%x\n",
									retry_cnt, cur_addr, buf_pos);
				if (retry_cnt > 0) {
					cur_addr &= ~(sector_size - 1);
					buf_pos &= ~(sector_size - 1);
					goto retry;
				} else
					return mem_check_fail;
			} else {
				LCD_DEBUG("FW Check Pass!! Address [0x%02x%02x%02x] \n",
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]]);
			}
			ret = mem_check_fail;

			cur_addr += write_size;
			buf_pos += write_size;
			if (sector_size && (cur_addr % sector_size == 0))
				break;
		}
	}

	/* read entire fw 64kb */
	if (!mem_check_fail) {
		LCD_INFO("Check Entire Firmware Start ***\n");
		cur_addr = vdd->fw_up.start_addr;
		start_addr = vdd->fw_up.start_addr;
		last_addr = vdd->fw_up.start_addr + vdd->fw_up.image_size;
		buf_pos = 0;

		LCD_INFO("FW Read Start Address(0x%x), Sector Size(0x%x), Read Size(%d), Last Address(0x%x)\n",
			start_addr, sector_size, write_size, last_addr);
		for (cur_addr = start_addr; (cur_addr < last_addr); ) {
			/* Read Data */
			read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]]
								= (cur_addr & 0xFF0000) >> 16;
			read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]]
								= (cur_addr & 0x00FF00) >> 8;
			read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]]
								= (cur_addr & 0x0000FF);

			/* Tx read size set 0x83 instead of 0x80 to avoid ddi read bug. */
			ss_send_cmd(vdd, TX_FW_UP_READ);
			usleep_range(read_delay_us, read_delay_us + 5);
			/* Read 0x80 bytes in real */
			ss_panel_data_read(vdd, RX_FW_UP_STATUS, read_check, LEVEL_KEY_NONE);
			if (read_check[0] != vdd->fw_up.read_status_value) {
				ss_write_fw_up_debug_partition(FW_FAIL_LINE, __LINE__);
				LCD_ERR("TX_FW_READ Fail [0x%02x%02x%02x], read_check = 0x%x, Wait %dus more \n",
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]],
					read_check[0], write_delay_us);
				/* Wait more time */
				usleep_range(write_delay_us, write_delay_us * 2);
			}

			/* Compare Write Data & Read Data */
			memset(read_data, 0x00, 128);
			ss_panel_data_read(vdd, RX_FW_UP_READ, read_data, LEVEL_KEY_NONE);
			mem_check_fail = false;
			for (loop = 0; loop < 128 ; loop++) {
				if (read_data[loop] != fw_data_64k[buf_pos + loop]) {
					mem_check_fail = true;

					ss_write_fw_up_debug_partition(FW_UP_FAIL, cur_addr);
					break;
				}
			}
			if (mem_check_fail) {
				usleep_range(write_delay_us * 2, write_delay_us * 3);
				ss_write_fw_up_debug_partition(FW_FAIL_LINE, __LINE__);
				LCD_ERR("FW Check Fail!! Address [0x%02x%02x%02x][%d] \n",
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]], loop);
				retry_all_cnt--;
				LCD_INFO("Retry_all Cnt = %d, cur_addr = 0x%x, buf_pos = 0x%x\n",
									retry_all_cnt, cur_addr, buf_pos);
				if (retry_all_cnt > 0)
					goto retry_all;
				else
					return mem_check_fail;
			} else {
				LCD_DEBUG("FW Check Pass!! Address [0x%02x%02x%02x] \n",
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[0]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[1]],
					read_tx_cmds->cmds[0].msg.tx_buf[vdd->fw_up.write_addr_idx[2]]);
			}
			ret = mem_check_fail;

			cur_addr += write_size;
			buf_pos += write_size;
		}
		LCD_INFO("Read Entire Firmware End ***\n");
	}

	/* Restore data */
	//[0~1]  C0 00
	//rase_tx_cmds->cmds[0].msg.tx_buf[1] = 0x00;
	//[3~5] 03 20 00
	//erase_tx_cmds->cmds[0].msg.tx_buf[3] = 0x03;
	//erase_tx_cmds->cmds[0].msg.tx_buf[4] = 0x20;
	//[6~8] 00 10 00
	//erase_tx_cmds->cmds[0].msg.tx_buf[6] = 0x00;
	//erase_tx_cmds->cmds[0].msg.tx_buf[7] = 0x10;

	LCD_INFO("FW Write Finish (ret = %d)\n", ret);

	return ret;
}

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *poc_read_tx_cmds = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	LCD_INFO("+: ndx=%d\n", vdd->ndx);
	ss_panel_attach_set(vdd, true);

	prev_refresh_rate = 0;	/* make previous refresh_rate 0 before start */
	time_vrr = 0;	/* make timestamp 0 for freq calculation */
	init_smooth_off = 1;

	if (vdd->manufacture_id_dsi > 0x801413) {/* Support Samsung IP if higher revision*/
		LCD_INFO("Support mdnie, selfmask.\n");
		/* For PV2 : 50ms from dtsi*/
		/* For PV3 : 0ms from dtsi*/
	}

	/* Read flash to check if poc done at first boot */
	if (vdd->display_status_dsi.first_commit_disp_on) {
		poc_read_tx_cmds = ss_get_cmds(vdd, TX_POC_READ);
		if (SS_IS_CMDS_NULL(poc_read_tx_cmds)) {
			LCD_ERR("no cmds for TX_POC_READ..\n");
			return -EINVAL;
		} else {
			u8 rx_buf[128], status_c1;
			int read_addr = vdd->poc_driver.start_addr + 527;

			poc_read_tx_cmds->cmds[0].msg.tx_buf[vdd->poc_driver.read_addr_idx[0]]
										= (read_addr & 0xFF0000) >> 16;
			poc_read_tx_cmds->cmds[0].msg.tx_buf[vdd->poc_driver.read_addr_idx[1]]
										= (read_addr & 0x00FF00) >> 8;
			poc_read_tx_cmds->cmds[0].msg.tx_buf[vdd->poc_driver.read_addr_idx[2]]
										= read_addr & 0x0000FF;

			ss_send_cmd(vdd, TX_POC_READ);
			usleep_range(40000, 40005); //40ms

			ss_panel_data_read(vdd, RX_POC_STATUS, &status_c1, LEVEL_KEY_NONE);

			if (!(status_c1 == 04))
				LCD_ERR("READ status not 04 ret:%d\n", status_c1);
			else {
				LCD_INFO("READ status_c1: %02x\n", status_c1); /* Read status 1 byte*/

				ss_panel_data_read(vdd, RX_POC_READ, rx_buf, LEVEL_KEY_NONE);

				LCD_DEBUG("addr[%x] = [0x%x 0x%x 0x%x]\n",
					read_addr, rx_buf[0], rx_buf[1], rx_buf[2]);

				//Compare if 0xA33Ae7
				if ((rx_buf[0] == 0xA3) && (rx_buf[1] == 0x3A) && (rx_buf[2] == 0xe7))
					poc_done = 1;
			}
		}
	}
	return true;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	if (vdd->manufacture_id_dsi >= 0x801416) {
		/* Read ddi fw id ([0] of 4bytes*/
		if (vdd->display_status_dsi.first_commit_disp_on) {
			vdd->check_fw_id = ss_fw_id_read(vdd);
			if (!vdd->check_fw_id)
				LCD_INFO("check_fw_id : 0x%x\n", vdd->check_fw_id);
		}

		/* Write additioal TSP Vsync 120hz cmds for opcodeG (801416)
		*  which is deleted for opcod H (801417)
		*  write only for 8014167
		*/
		if (vdd->manufacture_id_dsi == 0x801416) {
			struct dsi_panel_cmd_set *pcmds;

			LCD_INFO("Write tsp vsync cmds only for 801416. %dhz\n",
					vdd->vrr.adjusted_refresh_rate);
			pcmds = ss_get_cmds(vdd, TX_HSYNC_ON);
			if (vdd->vrr.adjusted_refresh_rate == 60) /* 60hz : D6 10 */
				pcmds->cmds[5].msg.tx_buf[1] = 0x10;
			else /* 120hz : D6 30 */
				pcmds->cmds[5].msg.tx_buf[1] = 0x30;

			ss_send_cmd(vdd, TX_HSYNC_ON);
		}
	}

	/* MTP ID 17~18 && FW id 16 : SP Off write */
	if ((vdd->manufacture_id_dsi > 0x801416) && (vdd->manufacture_id_dsi < 0x801419)
		&& (vdd->check_fw_id == 0x16))
		ss_send_cmd(vdd, TX_NORMAL_BRIGHTNESS_ETC);

	/* Check if support selfmask */
	if (!vdd->self_disp.is_support)
		return false;

	/*
	 * self mask is enabled from bootloader.
	 * so skip self mask setting during splash booting.
	 */
	if (!vdd->samsung_splash_enabled) {
		if (vdd->self_disp.self_mask_img_write)
			vdd->self_disp.self_mask_img_write(vdd);
	} else {
		LCD_INFO("samsung splash enabled.. skip image write\n");
	}

	if (vdd->self_disp.self_mask_on)
		vdd->self_disp.self_mask_on(vdd, true);

	return true;
}

static char ss_panel_revision(struct samsung_display_driver_data *vdd)
{
	if (vdd->manufacture_id_dsi == PBA_ID)
		ss_panel_attach_set(vdd, false);
	else
		ss_panel_attach_set(vdd, true);

	switch (ss_panel_rev_get(vdd)) {
	case 0x00:
		vdd->panel_revision = 'A';
		break;
	default:
		vdd->panel_revision = 'A';
		LCD_ERR("Invalid panel_rev(default rev : %c)\n", vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';
	LCD_INFO_ONCE("panel_revision = %c %d \n", vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision + 'A');
}

/* green weight control to compensate green tint issue on low brighness level */
struct dsi_panel_cmd_set *ss_brightness_gm2_gamma_comp(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds = NULL;
	char id3 = ss_panel_id2_get(vdd);
	int cur_rr = vdd->vrr.cur_refresh_rate;

	LCD_INFO("+++ ID3: 0x%x, bl_level: %d, VRR: %d Hz poc:%d\n",
			id3, vdd->br_info.common_br.bl_level, cur_rr, poc_done);

	if ((id3 > 0x19 || id3 < 0x16) || /* support ID3 0x16 ~ 0x19*/
			(cur_rr != 120) || /* support for VRR 120HS */
			poc_done) { /* Avoid comp if poc has done */
		LCD_INFO("skip green weight\n");
		return NULL;
	}

	if (vdd->br_info.common_br.bl_level <= 6) {
		/* under 10nit (bl_level 6): 80% */
		pcmds = ss_get_cmds(vdd, TX_GREEN_WEIGHT_80PERCENT);
		LCD_INFO("green weight: 80%\n");
	} else {
		/* normal setting */
		pcmds = ss_get_cmds(vdd, TX_GREEN_WEIGHT_NORMAL);
		LCD_INFO("green weight: normal\n");
	}

	*level_key = LEVEL_KEY_NONE;

	LCD_INFO("--- ID3: 0x%x, bl_level: %d, VRR: %d Hz\n",
			id3, vdd->br_info.common_br.bl_level, cur_rr);

	return pcmds;
}

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_normal(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_NORMAL);

	LCD_INFO("Normal : bl_level:%d, prev_bl:%d, finger:%d Pstate:%d(2:on),%d,%d\n",
		vdd->br_info.common_br.bl_level, prev_bl_s6tuum3, vdd->finger_mask_updated,
		vdd->panel_state, vdd->display_status_dsi.wait_disp_on, init_smooth_off);
	if (vdd->finger_mask_updated)
		pcmds->cmds[2].msg.tx_buf[1] = 0x20;
	else if (vdd->display_status_dsi.wait_disp_on)
		pcmds->cmds[2].msg.tx_buf[1] = 0x20;
	else if (unlikely(vdd->is_factory_mode))
		pcmds->cmds[2].msg.tx_buf[1] = 0x20;
	else if (prev_bl_s6tuum3 > 255) /* smooth off from HBM*/
		pcmds->cmds[2].msg.tx_buf[1] = 0x20;
	else if (vdd->panel_state < PANEL_PWR_ON) /* smooth off before display on*/
		pcmds->cmds[2].msg.tx_buf[1] = 0x20;
	else
		pcmds->cmds[2].msg.tx_buf[1] = 0x28;

	if (init_smooth_off && vdd->br_info.common_br.bl_level) {
		pcmds->cmds[2].msg.tx_buf[1] = 0x20;
		LCD_INFO("Normal : smooth off\n");
		init_smooth_off = 0;
	}

	pcmds->cmds[1].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8); /* DBV [7:0] */
	pcmds->cmds[1].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 3); /* DBV [10:8] */

	*level_key = LEVEL_KEY_NONE;
	prev_bl_s6tuum3 = vdd->br_info.common_br.bl_level;

	return pcmds;
}

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_hbm(struct samsung_display_driver_data *vdd, int *level_key)
{
    struct dsi_panel_cmd_set *pcmds;

    if (IS_ERR_OR_NULL(vdd)) {
        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
        return NULL;
    }

	LCD_INFO("HBM : cd:%d  refresh_rate:%d bl_level:%d prev_bl:%d finger:%d\n",
		vdd->br_info.common_br.cd_level, vdd->vrr.adjusted_refresh_rate, vdd->br_info.common_br.bl_level,
		prev_bl_s6tuum3, vdd->finger_mask_updated);

	if (vdd->vrr.adjusted_refresh_rate == 60) { /* 60hz : from hbm_60hz_gamma_table in ~.h */
		int val3;

		/* opcodeH
 		 B0 00 7B
		 67 00 28 <-1,2
 		 B0 00 82
		 67 A0   <-val3
		 B0 00 02
		 B8 01 52 <= AOR
		 53 E0  */
		pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HBM_60HZ);
		pcmds->cmds[1].msg.tx_buf[1] = get_bit(hbm_60hz_67h_table[vdd->br_info.common_br.bl_level], 8, 3); /* [10:8] */
		pcmds->cmds[1].msg.tx_buf[2] = get_bit(hbm_60hz_67h_table[vdd->br_info.common_br.bl_level], 0, 8); /* [7:0] */

		if (vdd->br_info.common_br.bl_level > 296)
			val3 = 0x17;
		else if (vdd->br_info.common_br.bl_level > 289)
			val3 = 0x27;
		else
			val3 = 0x37;
		pcmds->cmds[3].msg.tx_buf[1] = val3; /* 3rd: 67h 83th write*/

		pcmds->cmds[5].msg.tx_buf[1] = get_bit(hbm_60hz_gamma_table[vdd->br_info.common_br.bl_level], 8, 3); /* [10:8] */
		pcmds->cmds[5].msg.tx_buf[2] = get_bit(hbm_60hz_gamma_table[vdd->br_info.common_br.bl_level], 0, 8); /* [7:0] */
	} else { /* 120hz : from dtsi hbm gamma table*/
		/* opcodeH
		 B0 00 7D
		 C7 00 50
		 B0 00 C7
		 6F 09
		 B0 00 79
		 C7 01 54  <= AOR
		 53 E0 */
		pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HBM);
		pcmds->cmds[5].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 3); /* [10:8] */
		pcmds->cmds[5].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8); /* [7:0] */
	}

	/* Fingerprint->HBM MAX? */
    *level_key = LEVEL_KEY_NONE;
	prev_bl_s6tuum3 = vdd->br_info.common_br.bl_level;

    return pcmds;
}

static int ss_manufacture_date_read(struct samsung_display_driver_data *vdd)
{
	unsigned char date[8];
	int year, month, day;
	int hour, min;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (EAh 8~14th) for manufacture date */
	if (ss_get_cmds(vdd, RX_MANUFACTURE_DATE)->count) {
		ss_panel_data_read(vdd, RX_MANUFACTURE_DATE, date, LEVEL_KEY_NONE);

		year = date[0] & 0xf0;
		year >>= 4;
		year += 2011; // 0 = 2011 year
		month = date[0] & 0x0f;
		day = date[1] & 0x1f;
		hour = date[2] & 0x1f;
		min = date[3] & 0x3f;

		vdd->manufacture_date_dsi = year * 10000 + month * 100 + day;
		vdd->manufacture_time_dsi = hour * 100 + min;

		LCD_ERR("manufacture_date DSI%d = (%d%04d) - year(%d) month(%d) day(%d) hour(%d) min(%d)\n",
			vdd->ndx, vdd->manufacture_date_dsi, vdd->manufacture_time_dsi,
			year, month, day, hour, min);

	} else {
		LCD_ERR("DSI%d no manufacture_date_rx_cmds cmds(%d)", vdd->ndx, vdd->panel_revision);
		return false;
	}

	return true;
}

static int ss_ddi_id_read(struct samsung_display_driver_data *vdd)
{
	char ddi_id[5];
	int loop;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (D6h 1~5th) for CHIP ID */
	if (ss_get_cmds(vdd, RX_DDI_ID)->count) {
		ss_panel_data_read(vdd, RX_DDI_ID, ddi_id, LEVEL_KEY_NONE);

		for (loop = 0; loop < 5; loop++)
			vdd->ddi_id_dsi[loop] = ddi_id[loop];

		LCD_INFO("DSI%d : %02x %02x %02x %02x %02x\n", vdd->ndx,
			vdd->ddi_id_dsi[0], vdd->ddi_id_dsi[1],
			vdd->ddi_id_dsi[2], vdd->ddi_id_dsi[3],
			vdd->ddi_id_dsi[4]);
	} else {
		LCD_ERR("DSI%d no ddi_id_rx_cmds cmds", vdd->ndx);
		return false;
	}

	return true;
}

#undef COORDINATE_DATA_SIZE
#define COORDINATE_DATA_SIZE 6

#define F1(x, y) ((y)-((39*(x))/38)-95)
#define F2(x, y) ((y)-((36*(x))/35)-56)
#define F3(x, y) ((y)+((7*(x))/1)-24728)
#define F4(x, y) ((y)+((25*(x))/7)-14031)

/* Normal Mode */
static char coordinate_data_1[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xfb, 0x00, 0xfb, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfc, 0x00, 0xff, 0x00}, /* Tune_2 */
	{0xfa, 0x00, 0xf9, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfd, 0x00, 0xfb, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xfa, 0x00, 0xfb, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xfd, 0x00, 0xff, 0x00, 0xfa, 0x00}, /* Tune_7 */
	{0xfc, 0x00, 0xff, 0x00, 0xfc, 0x00}, /* Tune_8 */
	{0xfa, 0x00, 0xfd, 0x00, 0xff, 0x00}, /* Tune_9 */
};

/* sRGB/Adobe RGB Mode */
static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* dummy */
	{0xff, 0x00, 0xfa, 0x00, 0xf3, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfa, 0x00, 0xf6, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfb, 0x00, 0xfa, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfc, 0x00, 0xf3, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfe, 0x00, 0xfb, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfd, 0x00, 0xf3, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xff, 0x00, 0xf7, 0x00}, /* Tune_8 */
	{0xfd, 0x00, 0xff, 0x00, 0xf9, 0x00}, /* Tune_9 */
};

static char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE] = {
	coordinate_data_2, /* DYNAMIC - DCI */
	coordinate_data_2, /* STANDARD - sRGB/Adobe RGB */
	coordinate_data_2, /* NATURAL - sRGB/Adobe RGB */
	coordinate_data_1, /* MOVIE - Normal */
	coordinate_data_1, /* AUTO - Normal */
	coordinate_data_1, /* READING - Normal */
};

static int mdnie_coordinate_index(int x, int y)
{
	int tune_number = 0;

	if (F1(x, y) > 0) {
		if (F3(x, y) > 0) {
			tune_number = 3;
		} else {
			if (F4(x, y) < 0)
				tune_number = 1;
			else
				tune_number = 2;
		}
	} else {
		if (F2(x, y) < 0) {
			if (F3(x, y) > 0) {
				tune_number = 9;
			} else {
				if (F4(x, y) < 0)
					tune_number = 7;
				else
					tune_number = 8;
			}
		} else {
			if (F3(x, y) > 0)
				tune_number = 6;
			else {
				if (F4(x, y) < 0)
					tune_number = 4;
				else
					tune_number = 5;
			}
		}
	}

	return tune_number;
}

static int ss_elvss_read(struct samsung_display_driver_data *vdd)
{
	char elvss_b7[2];

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (B7h 9th,10th) for elvss*/
	ss_panel_data_read(vdd, RX_ELVSS, elvss_b7, LEVEL_KEY_NONE);

	vdd->br_info.common_br.elvss_value[0] = elvss_b7[0]; /*0xB7 9th para OTP value*/
	vdd->br_info.common_br.elvss_value[1] = elvss_b7[1]; /*0xB7 10th para OTP value */

	return true;
}

static struct dsi_panel_cmd_set *ss_elvss(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *elvss_cmds = ss_get_cmds(vdd, TX_ELVSS);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(elvss_cmds)) {
		LCD_ERR("No elvss_tx_cmds\n");
		return NULL;
	}

	LCD_DEBUG("temperature:%d\n",vdd->br_info.temperature);
	/* 	0x68 1th TSET
		BIT(7) is signed bit.
		BIT(6) ~ BIT(0) is real br_info.temperature.
	*/
	elvss_cmds->cmds[2].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
			vdd->br_info.temperature : (char)(BIT(7) | (-1*vdd->br_info.temperature));
	/* B8 00 C3->B3h : Compensation ON*/
#if 0
	/* 0xB5 2th MSP */
	if (vdd->br_info.common_br.cd_level > 39)
		elvss_cmds->cmds[0].msg.tx_buf[2] = 0xDC;
	else
		elvss_cmds->cmds[0].msg.tx_buf[2] = 0xCC;

	/* ELVSS Compensation for Low Temperature & Low Birghtness*/
	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_ELVSS, &elvss_cmds->cmds->msg.tx_buf[3]);

	/* 0xB5 elvss_57th_val elvss_cal_offset */

	/* Read B5h 23th para -> Write to B5h 23th para */
	elvss_cmds->cmds->msg.tx_buf[23] = vdd->br_info.common_br.elvss_value[1];

	*level_key = LEVEL1_KEY;
#endif
	return elvss_cmds;
}

static int ss_mdnie_read(struct samsung_display_driver_data *vdd)
{
	char x_y_location[4];
	int mdnie_tune_index = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (A1h 1~5th) for ddi id */
	if (ss_get_cmds(vdd, RX_MDNIE)->count) {
		ss_panel_data_read(vdd, RX_MDNIE, x_y_location, LEVEL1_KEY);

		vdd->mdnie.mdnie_x = x_y_location[0] << 8 | x_y_location[1];	/* X */
		vdd->mdnie.mdnie_y = x_y_location[2] << 8 | x_y_location[3];	/* Y */

		mdnie_tune_index = mdnie_coordinate_index(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);

		coordinate_tunning_multi(vdd, coordinate_data_multi, mdnie_tune_index,
				ADDRESS_SCR_WHITE_RED, COORDINATE_DATA_SIZE);

		LCD_INFO("DSI%d : X-%d Y-%d \n", vdd->ndx, vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);
	} else {
		LCD_ERR("DSI%d no mdnie_read_rx_cmds cmds", vdd->ndx);
		return false;
	}

	return true;
}

static int dsi_update_mdnie_data(struct samsung_display_driver_data *vdd)
{
	struct mdnie_lite_tune_data *mdnie_data;

	mdnie_data = kzalloc(sizeof(struct mdnie_lite_tune_data), GFP_KERNEL);
	if (!mdnie_data) {
		LCD_ERR("fail to allocate mdnie_data memory\n");
		return -ENOMEM;
	}

	/* Update mdnie command */
	mdnie_data->DSI_COLOR_BLIND_MDNIE_1 = DSI0_COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_1 = DSI0_RGB_SENSOR_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_2 = DSI0_RGB_SENSOR_MDNIE_2;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_3 = DSI0_RGB_SENSOR_MDNIE_3;
	mdnie_data->DSI_TRANS_DIMMING_MDNIE = DSI0_RGB_SENSOR_MDNIE_3;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE_2 = DSI0_UI_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_UI_STANDARD_MDNIE_2 = DSI0_UI_STANDARD_MDNIE_2;
	mdnie_data->DSI_UI_AUTO_MDNIE_2 = DSI0_UI_AUTO_MDNIE_2;
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE_2 = DSI0_VIDEO_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE_2 = DSI0_VIDEO_STANDARD_MDNIE_2;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE_2 = DSI0_VIDEO_AUTO_MDNIE_2;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE_2 = DSI0_CAMERA_AUTO_MDNIE_2;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE_2 = DSI0_GALLERY_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE_2 = DSI0_GALLERY_STANDARD_MDNIE_2;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE_2 = DSI0_GALLERY_AUTO_MDNIE_2;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE_2 = DSI0_BROWSER_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE_2 = DSI0_BROWSER_STANDARD_MDNIE_2;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE_2 = DSI0_BROWSER_AUTO_MDNIE_2;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE_2 = DSI0_EBOOK_AUTO_MDNIE_2;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE_2 = DSI0_EBOOK_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE_2 = DSI0_EBOOK_STANDARD_MDNIE_2;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE_2 = DSI0_EBOOK_AUTO_MDNIE_2;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE_2 = DSI0_TDMB_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE_2 = DSI0_TDMB_STANDARD_MDNIE_2;
	mdnie_data->DSI_TDMB_AUTO_MDNIE_2 = DSI0_TDMB_AUTO_MDNIE_2;

	mdnie_data->DSI_BYPASS_MDNIE = DSI0_BYPASS_MDNIE;
	mdnie_data->DSI_NEGATIVE_MDNIE = DSI0_NEGATIVE_MDNIE;
	mdnie_data->DSI_COLOR_BLIND_MDNIE = DSI0_COLOR_BLIND_MDNIE;
	mdnie_data->DSI_HBM_CE_MDNIE = DSI0_HBM_CE_MDNIE;
	mdnie_data->DSI_HBM_CE_D65_MDNIE = DSI0_HBM_CE_D65_MDNIE;
	mdnie_data->DSI_RGB_SENSOR_MDNIE = DSI0_RGB_SENSOR_MDNIE;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE = DSI0_UI_DYNAMIC_MDNIE;
	mdnie_data->DSI_UI_STANDARD_MDNIE = DSI0_UI_STANDARD_MDNIE;
	mdnie_data->DSI_UI_NATURAL_MDNIE = DSI0_UI_NATURAL_MDNIE;
	mdnie_data->DSI_UI_AUTO_MDNIE = DSI0_UI_AUTO_MDNIE;
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE = DSI0_VIDEO_DYNAMIC_MDNIE;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE = DSI0_VIDEO_STANDARD_MDNIE;
	mdnie_data->DSI_VIDEO_NATURAL_MDNIE = DSI0_VIDEO_NATURAL_MDNIE;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE = DSI0_VIDEO_AUTO_MDNIE;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE = DSI0_CAMERA_AUTO_MDNIE;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE = DSI0_GALLERY_DYNAMIC_MDNIE;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE = DSI0_GALLERY_STANDARD_MDNIE;
	mdnie_data->DSI_GALLERY_NATURAL_MDNIE = DSI0_GALLERY_NATURAL_MDNIE;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE = DSI0_GALLERY_AUTO_MDNIE;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE = DSI0_BROWSER_DYNAMIC_MDNIE;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE = DSI0_BROWSER_STANDARD_MDNIE;
	mdnie_data->DSI_BROWSER_NATURAL_MDNIE = DSI0_BROWSER_NATURAL_MDNIE;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE = DSI0_BROWSER_AUTO_MDNIE;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE = DSI0_EBOOK_DYNAMIC_MDNIE;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE = DSI0_EBOOK_STANDARD_MDNIE;
	mdnie_data->DSI_EBOOK_NATURAL_MDNIE = DSI0_EBOOK_NATURAL_MDNIE;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE = DSI0_EBOOK_AUTO_MDNIE;
	mdnie_data->DSI_EMAIL_AUTO_MDNIE = DSI0_EMAIL_AUTO_MDNIE;
	mdnie_data->DSI_GAME_LOW_MDNIE = DSI0_GAME_LOW_MDNIE;
	mdnie_data->DSI_GAME_MID_MDNIE = DSI0_GAME_MID_MDNIE;
	mdnie_data->DSI_GAME_HIGH_MDNIE = DSI0_GAME_HIGH_MDNIE;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE = DSI0_TDMB_DYNAMIC_MDNIE;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE = DSI0_TDMB_STANDARD_MDNIE;
	mdnie_data->DSI_TDMB_NATURAL_MDNIE = DSI0_TDMB_NATURAL_MDNIE;
	mdnie_data->DSI_TDMB_AUTO_MDNIE = DSI0_TDMB_AUTO_MDNIE;
	mdnie_data->DSI_GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
	mdnie_data->DSI_GRAYSCALE_NEGATIVE_MDNIE = DSI0_GRAYSCALE_NEGATIVE_MDNIE;
	mdnie_data->DSI_CURTAIN = DSI0_SCREEN_CURTAIN_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR = DSI0_NIGHT_MODE_MDNIE_1;
	mdnie_data->DSI_COLOR_LENS_MDNIE = DSI0_COLOR_LENS_MDNIE;
	mdnie_data->DSI_COLOR_LENS_MDNIE_SCR = DSI0_COLOR_LENS_MDNIE_1;
	mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR = DSI0_RGB_SENSOR_MDNIE_1;

	mdnie_data->mdnie_tune_value_dsi = mdnie_tune_value_dsi0;
	mdnie_data->hmt_color_temperature_tune_value_dsi = hmt_color_temperature_tune_value_dsi0;
	mdnie_data->light_notification_tune_value_dsi = light_notification_tune_value_dsi0;
	mdnie_data->hdr_tune_value_dsi = hdr_tune_value_dsi0;

	/* Update MDNIE data related with size, offset or index */
	mdnie_data->dsi_bypass_mdnie_size = ARRAY_SIZE(DSI0_BYPASS_MDNIE);
	mdnie_data->mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
	mdnie_data->mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP3] = MDNIE_STEP3_INDEX;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = ADDRESS_SCR_WHITE_RED;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = ADDRESS_SCR_WHITE_GREEN;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = ADDRESS_SCR_WHITE_BLUE;
	mdnie_data->dsi_rgb_sensor_mdnie_1_size = DSI0_RGB_SENSOR_MDNIE_1_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_2_size = DSI0_RGB_SENSOR_MDNIE_2_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_3_size = DSI0_RGB_SENSOR_MDNIE_3_SIZE;

	mdnie_data->dsi_trans_dimming_data_index = MDNIE_TRANS_DIMMING_DATA_INDEX;

	mdnie_data->dsi_adjust_ldu_table = adjust_ldu_data;
	mdnie_data->dsi_max_adjust_ldu = 6;
	mdnie_data->dsi_night_mode_table = night_mode_data;
	mdnie_data->dsi_max_night_mode_index = 11;
	mdnie_data->dsi_color_lens_table = color_lens_data;
	mdnie_data->dsi_white_default_r = 0xff;
	mdnie_data->dsi_white_default_g = 0xff;
	mdnie_data->dsi_white_default_b = 0xff;
	mdnie_data->dsi_white_balanced_r = 0;
	mdnie_data->dsi_white_balanced_g = 0;
	mdnie_data->dsi_white_balanced_b = 0;
	mdnie_data->dsi_scr_step_index = MDNIE_STEP1_INDEX;
	mdnie_data->dsi_afc_size = 45;
	mdnie_data->dsi_afc_index = 33;

	vdd->mdnie.mdnie_data = mdnie_data;

	return 0;
}

static int ss_cell_id_read(struct samsung_display_driver_data *vdd)
{
	char cell_id_buffer[MAX_CELL_ID] = {0,};
	int loop;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read Panel Unique Cell ID (C9h 19~34th) */
	if (ss_get_cmds(vdd, RX_CELL_ID)->count) {
		memset(cell_id_buffer, 0x00, MAX_CELL_ID);

		ss_panel_data_read(vdd, RX_CELL_ID, cell_id_buffer, LEVEL_KEY_NONE);

		for (loop = 0; loop < MAX_CELL_ID; loop++)
			vdd->cell_id_dsi[loop] = cell_id_buffer[loop];

		LCD_INFO("DSI%d: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			vdd->ndx, vdd->cell_id_dsi[0],
			vdd->cell_id_dsi[1],	vdd->cell_id_dsi[2],
			vdd->cell_id_dsi[3],	vdd->cell_id_dsi[4],
			vdd->cell_id_dsi[5],	vdd->cell_id_dsi[6],
			vdd->cell_id_dsi[7],	vdd->cell_id_dsi[8],
			vdd->cell_id_dsi[9],	vdd->cell_id_dsi[10]);

	} else {
		LCD_ERR("DSI%d no cell_id_rx_cmds cmd\n", vdd->ndx);
		return false;
	}

	return true;
}

static int ss_octa_id_read(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read Panel Unique OCTA ID (C9h 2nd~21th) */
	if (ss_get_cmds(vdd, RX_OCTA_ID)->count) {
		memset(vdd->octa_id_dsi, 0x00, MAX_OCTA_ID);

		ss_panel_data_read(vdd, RX_OCTA_ID,
				vdd->octa_id_dsi, LEVEL_KEY_NONE);

		LCD_INFO("octa id: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			vdd->octa_id_dsi[0], vdd->octa_id_dsi[1],
			vdd->octa_id_dsi[2], vdd->octa_id_dsi[3],
			vdd->octa_id_dsi[4], vdd->octa_id_dsi[5],
			vdd->octa_id_dsi[6], vdd->octa_id_dsi[7],
			vdd->octa_id_dsi[8], vdd->octa_id_dsi[9],
			vdd->octa_id_dsi[10], vdd->octa_id_dsi[11],
			vdd->octa_id_dsi[12], vdd->octa_id_dsi[13],
			vdd->octa_id_dsi[14], vdd->octa_id_dsi[15],
			vdd->octa_id_dsi[16], vdd->octa_id_dsi[17],
			vdd->octa_id_dsi[18], vdd->octa_id_dsi[19]);

	} else {
		LCD_ERR("DSI%d no octa_id_rx_cmds cmd\n", vdd->ndx);
		return false;
	}

	return true;
}

static struct dsi_panel_cmd_set *ss_acl_on_hbm(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL_KEY_NONE;

	pcmds = ss_get_cmds(vdd, TX_ACL_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_ACL_ON..\n");
		return NULL;
	}
	if (vdd->br_info.common_br.bl_level <= MAX_BL_PF_LEVEL) {
		LCD_ERR("Not a hbm level..:&d\n", vdd->br_info.common_br.bl_level);
		return NULL;
 	}

	pcmds->cmds[0].msg.tx_buf[1] = 0x01;	/* 55h 0x01 ACL 8% */
	LCD_INFO("HBM: gradual_acl: %d, acl per: 0x%x",
			vdd->br_info.gradual_acl_val, pcmds->cmds[0].msg.tx_buf[1]);

	return pcmds;
}

static struct dsi_panel_cmd_set *ss_acl_on(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL_KEY_NONE;

	pcmds = ss_get_cmds(vdd, TX_ACL_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_ACL_ON..\n");
		return NULL;
	}

	if (vdd->br_info.common_br.bl_level <= MAX_BL_PF_LEVEL)
		pcmds->cmds[0].msg.tx_buf[1] = 0x02;	/* 55h 0x02 ACL 15% */
	else
		LCD_INFO("not normal bl_level for acl :%d\n");

	LCD_INFO("gradual_acl: %d, acl per: 0x%x\n",
			vdd->br_info.gradual_acl_val, pcmds->cmds[0].msg.tx_buf[1]);

	return pcmds;
}

static struct dsi_panel_cmd_set *ss_acl_off(struct samsung_display_driver_data *vdd, int *level_key)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL_KEY_NONE;
	LCD_INFO("off\n");
	return ss_get_cmds(vdd, TX_ACL_OFF);
}

static struct dsi_panel_cmd_set *ss_irc(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *irc_on_cmds = ss_get_cmds(vdd, TX_IRC);
	//struct dsi_panel_cmd_set *irc_off_cmds = ss_get_cmds(vdd, TX_IRC_OFF);
	/* always irc on*/

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx",	(size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(irc_on_cmds)) {
		LCD_ERR("No irc_tx_cmds_revA\n");
		return NULL;
	}

	if (!vdd->br_info.common_br.support_irc)
		return NULL;

	/* set irc mode to moderato or flat gamma */
	if (vdd->br_info.common_br.irc_mode == IRC_MODERATO_MODE)
		irc_on_cmds->cmds->msg.tx_buf[2] = IRC_MODERATO_MODE_VAL;
	else if (vdd->br_info.common_br.irc_mode == IRC_FLAT_GAMMA_MODE)
		irc_on_cmds->cmds->msg.tx_buf[2] = IRC_FLAT_GAMMA_MODE_VAL;
	else
		LCD_ERR("invalid irc mode(%d)\n", vdd->br_info.common_br.irc_mode);
	*level_key = LEVEL_KEY_NONE;

	return irc_on_cmds;
}

#if 0 /* no lpm because of no current saving : HW decided */
static void ss_set_panel_lpm_brightness(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *alpm_brightness[LPM_BRIGHTNESS_MAX_IDX] = {NULL, };
	struct dsi_panel_cmd_set *alpm_ctrl[MAX_LPM_CTRL] = {NULL, };
	struct dsi_panel_cmd_set *cmd_list[2];

	/*default*/
	int mode = ALPM_MODE_ON;
	int ctrl_index = HLPM_CTRL_2NIT;
	int bl_index = LPM_2NIT_IDX;

	/*
	 * Init reg_list and cmd list
	 * reg_list[X][0] is reg value
	 * reg_list[X][1] is offset for reg value
	 * cmd_list is the target cmds for searching reg value
	 */
	static int reg_list[2][2] = {
		{ALPM_REG, -EINVAL},
		{ALPM_CTRL_REG, -EINVAL}
	};

	LCD_INFO("%s++\n", __func__);

	cmd_list[0] = ss_get_cmds(vdd, TX_LPM_BL_CMD);
	cmd_list[1] = ss_get_cmds(vdd, TX_LPM_BL_CMD);
	if (SS_IS_CMDS_NULL(cmd_list[0]) || SS_IS_CMDS_NULL(cmd_list[1])) {
		LCD_ERR("No cmds for TX_LPM_BL_CMD..\n");
		return;
	}

	/* Init alpm_brightness and alpm_ctrl cmds */
	alpm_brightness[LPM_2NIT_IDX]  = ss_get_cmds(vdd, TX_LPM_2NIT_CMD);
	alpm_brightness[LPM_10NIT_IDX] = ss_get_cmds(vdd, TX_LPM_10NIT_CMD);
	alpm_brightness[LPM_30NIT_IDX] = ss_get_cmds(vdd, TX_LPM_30NIT_CMD);
	alpm_brightness[LPM_60NIT_IDX] = ss_get_cmds(vdd, TX_LPM_60NIT_CMD);
	if (SS_IS_CMDS_NULL(alpm_brightness[LPM_2NIT_IDX]) || SS_IS_CMDS_NULL(alpm_brightness[LPM_10NIT_IDX]) ||
		SS_IS_CMDS_NULL(alpm_brightness[LPM_30NIT_IDX]) || SS_IS_CMDS_NULL(alpm_brightness[LPM_60NIT_IDX])) {
		LCD_ERR("No cmds for alpm_brightness..\n");
		return;
	}

	alpm_ctrl[HLPM_CTRL_2NIT]  = ss_get_cmds(vdd, TX_HLPM_2NIT_CMD);
	alpm_ctrl[HLPM_CTRL_10NIT] = ss_get_cmds(vdd, TX_HLPM_10NIT_CMD);
	alpm_ctrl[HLPM_CTRL_30NIT] = ss_get_cmds(vdd, TX_HLPM_30NIT_CMD);
	alpm_ctrl[HLPM_CTRL_60NIT] = ss_get_cmds(vdd, TX_HLPM_60NIT_CMD);
	if (SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_2NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_10NIT]) ||
		SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_30NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_60NIT])) {
		LCD_ERR("No cmds for hlpm_brightness..\n");
		return;
	}

	mode = vdd->panel_lpm.mode;

	switch (vdd->panel_lpm.lpm_bl_level) {
	case LPM_10NIT:
		ctrl_index = HLPM_CTRL_10NIT;
		bl_index = LPM_10NIT_IDX;
		break;
	case LPM_30NIT:
		ctrl_index = HLPM_CTRL_30NIT;
		bl_index = LPM_30NIT_IDX;
		break;
	case LPM_60NIT:
		ctrl_index = HLPM_CTRL_60NIT;
		bl_index = LPM_60NIT_IDX;
		break;
	case LPM_2NIT:
	default:
		ctrl_index = HLPM_CTRL_2NIT;
		bl_index = LPM_2NIT_IDX;
		break;
	}

	LCD_INFO("[Panel LPM] lpm_bl_level = %d bl_index %d, ctrl_index %d, mode %d\n",
			 vdd->panel_lpm.lpm_bl_level, bl_index, ctrl_index, mode);

	/*
	 * Find offset for alpm_reg and alpm_ctrl_reg
	 * alpm_reg  : Control register for ALPM/HLPM on/off
	 * alpm_ctrl_reg : Control register for changing ALPM/HLPM mode
	 */
	if (unlikely((reg_list[0][1] == -EINVAL) || (reg_list[1][1] == -EINVAL)))
		ss_find_reg_offset(reg_list, cmd_list, ARRAY_SIZE(cmd_list));

	if (reg_list[0][1] != -EINVAL) {
		/* Update parameter for ALPM_REG */
		memcpy(cmd_list[0]->cmds[reg_list[0][1]].msg.tx_buf,
				alpm_brightness[bl_index]->cmds[0].msg.tx_buf,
				sizeof(char) * cmd_list[0]->cmds[reg_list[0][1]].msg.tx_len);

		LCD_INFO("[Panel LPM] change brightness cmd : %x, %x\n",
				cmd_list[0]->cmds[reg_list[0][1]].msg.tx_buf[1],
				alpm_brightness[bl_index]->cmds[0].msg.tx_buf[1]);
	}

	if (reg_list[1][1] != -EINVAL) {
		/* Initialize ALPM/HLPM cmds */
		/* Update parameter for ALPM_CTRL_REG */
		memcpy(cmd_list[1]->cmds[reg_list[1][1]].msg.tx_buf,
				alpm_ctrl[ctrl_index]->cmds[0].msg.tx_buf,
				sizeof(char) * cmd_list[1]->cmds[reg_list[1][1]].msg.tx_len);

		LCD_INFO("[Panel LPM] update alpm ctrl reg\n");
	}

	//send lpm bl cmd
	ss_send_cmd(vdd, TX_LPM_BL_CMD);

	LCD_INFO("[Panel LPM] bl_level : %s\n",
			/* Check current brightness level */
			vdd->panel_lpm.lpm_bl_level == LPM_2NIT ? "2NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_10NIT ? "10NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_30NIT ? "30NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_60NIT ? "60NIT" : "UNKNOWN");

	LCD_INFO("%s--\n", __func__);
}

/*
 * This function will update parameters for ALPM_REG/ALPM_CTRL_REG
 * Parameter for ALPM_REG : Control brightness for panel LPM
 * Parameter for ALPM_CTRL_REG : Change panel LPM mode for ALPM/HLPM
 * mode, brightness, hz are updated here.
 */
static void ss_update_panel_lpm_ctrl_cmd(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *alpm_brightness[LPM_BRIGHTNESS_MAX_IDX] = {NULL, };
	struct dsi_panel_cmd_set *alpm_ctrl[MAX_LPM_CTRL] = {NULL, };
	struct dsi_panel_cmd_set *alpm_off_ctrl[MAX_LPM_MODE] = {NULL, };
	struct dsi_panel_cmd_set *cmd_list[2];
	struct dsi_panel_cmd_set *off_cmd_list[1];

	/*default*/
	int mode = ALPM_MODE_ON;
	int ctrl_index = HLPM_CTRL_2NIT;
	int bl_index = LPM_2NIT_IDX;

	/*
	 * Init reg_list and cmd list
	 * reg_list[X][0] is reg value
	 * reg_list[X][1] is offset for reg value
	 * cmd_list is the target cmds for searching reg value
	 */
	static int reg_list[2][2] = {
		{ALPM_REG, -EINVAL},
		{ALPM_CTRL_REG, -EINVAL}
	};

	static int off_reg_list[1][2] = { {ALPM_CTRL_REG, -EINVAL} };

	LCD_INFO("%s++\n", __func__);

	cmd_list[0] = ss_get_cmds(vdd, TX_LPM_ON);
	cmd_list[1] = ss_get_cmds(vdd, TX_LPM_ON);
	if (SS_IS_CMDS_NULL(cmd_list[0]) || SS_IS_CMDS_NULL(cmd_list[1])) {
		LCD_ERR("No cmds for TX_LPM_ON..\n");
		return;
	}

	off_cmd_list[0] = ss_get_cmds(vdd, TX_LPM_OFF);
	if (SS_IS_CMDS_NULL(off_cmd_list[0])) {
		LCD_ERR("No cmds for TX_LPM_OFF..\n");
		return;
	}

	/* Init alpm_brightness and alpm_ctrl cmds */
	alpm_brightness[LPM_2NIT_IDX]  = ss_get_cmds(vdd, TX_LPM_2NIT_CMD);
	alpm_brightness[LPM_10NIT_IDX] = ss_get_cmds(vdd, TX_LPM_10NIT_CMD);
	alpm_brightness[LPM_30NIT_IDX] = ss_get_cmds(vdd, TX_LPM_30NIT_CMD);
	alpm_brightness[LPM_60NIT_IDX] = ss_get_cmds(vdd, TX_LPM_60NIT_CMD);
	if (SS_IS_CMDS_NULL(alpm_brightness[LPM_2NIT_IDX]) || SS_IS_CMDS_NULL(alpm_brightness[LPM_10NIT_IDX]) ||
		SS_IS_CMDS_NULL(alpm_brightness[LPM_30NIT_IDX]) || SS_IS_CMDS_NULL(alpm_brightness[LPM_60NIT_IDX])) {
		LCD_ERR("No cmds for alpm_brightness..\n");
		return;
	}

	alpm_ctrl[HLPM_CTRL_2NIT]  = ss_get_cmds(vdd, TX_HLPM_2NIT_CMD);
	alpm_ctrl[HLPM_CTRL_10NIT] = ss_get_cmds(vdd, TX_HLPM_10NIT_CMD);
	alpm_ctrl[HLPM_CTRL_30NIT] = ss_get_cmds(vdd, TX_HLPM_30NIT_CMD);
	alpm_ctrl[HLPM_CTRL_60NIT] = ss_get_cmds(vdd, TX_HLPM_60NIT_CMD);
	if (SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_2NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_10NIT]) ||
		SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_30NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_60NIT])) {
		LCD_ERR("No cmds for hlpm_brightness..\n");
		return;
	}

/*
	alpm_off_ctrl[HLPM_MODE_ON] = ss_get_cmds(vdd, TX_HLPM_OFF);
	if (SS_IS_CMDS_NULL(alpm_off_ctrl[HLPM_MODE_ON])) {
		LCD_ERR("No cmds for TX_HLPM_OFF..\n");
		return;
	}
*/
	mode = vdd->panel_lpm.mode;

	switch (vdd->panel_lpm.lpm_bl_level) {
	case LPM_10NIT:
		ctrl_index = HLPM_CTRL_10NIT;
		bl_index = LPM_10NIT_IDX;
		break;
	case LPM_30NIT:
		ctrl_index = HLPM_CTRL_30NIT;
		bl_index = LPM_30NIT_IDX;
		break;
	case LPM_60NIT:
		ctrl_index = HLPM_CTRL_60NIT;
		bl_index = LPM_60NIT_IDX;
		break;
	case LPM_2NIT:
	default:
		ctrl_index = HLPM_CTRL_2NIT;
		bl_index = LPM_2NIT_IDX;
		break;
	}

	LCD_INFO("[Panel LPM] change brightness cmd bl_index :%d, ctrl_index : %d, mode : %d\n",
			 bl_index, ctrl_index, mode);

	/*
	 * Find offset for alpm_reg and alpm_ctrl_reg
	 * alpm_reg  : Control register for ALPM/HLPM on/off
	 * alpm_ctrl_reg : Control register for changing ALPM/HLPM mode
	 */
	if (unlikely((reg_list[0][1] == -EINVAL) || (reg_list[1][1] == -EINVAL)))
		ss_find_reg_offset(reg_list, cmd_list, ARRAY_SIZE(cmd_list));

	if (unlikely(off_reg_list[0][1] == -EINVAL))
		ss_find_reg_offset(off_reg_list, off_cmd_list, ARRAY_SIZE(off_cmd_list));

	if (reg_list[0][1] != -EINVAL) {
		/* Update parameter for ALPM_REG 0x53*/
		memcpy(cmd_list[0]->cmds[reg_list[0][1]].msg.tx_buf,
				alpm_brightness[bl_index]->cmds[0].msg.tx_buf,
				sizeof(char) * cmd_list[0]->cmds[reg_list[0][1]].msg.tx_len);

		LCD_INFO("[Panel LPM] change brightness cmd : %x, %x\n",
				cmd_list[0]->cmds[reg_list[0][1]].msg.tx_buf[1],
				alpm_brightness[bl_index]->cmds[0].msg.tx_buf[1]);
	}

	if (reg_list[1][1] != -EINVAL) {
		/* Initialize ALPM/HLPM cmds */
		/* Update parameter for ALPM_CTRL_REG 0xB9*/
		memcpy(cmd_list[1]->cmds[reg_list[1][1]].msg.tx_buf,
				alpm_ctrl[ctrl_index]->cmds[0].msg.tx_buf,
				sizeof(char) * cmd_list[1]->cmds[reg_list[1][1]].msg.tx_len);

		LCD_INFO("[Panel LPM] update alpm ctrl reg\n");
	}

	if ((off_reg_list[0][1] != -EINVAL) && (mode != LPM_MODE_OFF)) {
		/* Update parameter for ALPM_CTRL_REG */
		memcpy(off_cmd_list[0]->cmds[off_reg_list[0][1]].msg.tx_buf,
				alpm_off_ctrl[mode]->cmds[0].msg.tx_buf,
				sizeof(char) * off_cmd_list[0]->cmds[off_reg_list[0][1]].msg.tx_len);
	}

	LCD_INFO("%s--\n", __func__);
}
#endif

#if 0
static int ss_fw_up_send_cmd(struct samsung_display_driver_data *vdd, enum dsi_cmd_set_type type, u32 delay_us)
{
	int ret = 0;
	u8 status_check[2] = {0,};

	ss_send_cmd(vdd, type);
	usleep_range(delay_us, delay_us+10);
	ss_panel_data_read(vdd, RX_FW_UP_STATUS, status_check, LEVEL_KEY_NONE);
	if (status_check[0] != vdd->fw_up.read_status_value) {
		LCD_ERR("%s send Fail status_check = 0x%x\n", status_check[0]);
		return FW_UP_ERR_UPDATE_FAIL;
	}

	return ret;
}
#endif

static int ss_swire_rework_check(struct samsung_display_driver_data *vdd)
{
	int ret = 0;
	u8 read_data[256] = {0,};
	struct dsi_panel_cmd_set *pcmds = NULL;

	if (ss_panel_id2_get(vdd) < 0x17) {
		LCD_ERR("Does not support swire rework, ID3 = 0x%x\n", ss_panel_id2_get(vdd));
		return FW_UP_ERR_NOT_SUPPORT;
	}

	if (ss_panel_id2_get(vdd) > 0x17) {
		LCD_ERR("Swire rework is already done, ID3 = 0x%x\n", ss_panel_id2_get(vdd));
		return FW_UP_ERR_ALREADY_DONE;
	}

	pcmds = ss_get_cmds(vdd, TX_FW_UP_READ);
	//[3~5] addr : 03 20 00
	pcmds->cmds[0].msg.tx_buf[3] = 0x03;
	pcmds->cmds[0].msg.tx_buf[4] = 0x20;
	pcmds->cmds[0].msg.tx_buf[5] = 0x00;


	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_READ, vdd->fw_up.write_delay_us);
	if (ret) {
		LCD_ERR("FirmWare Write for Read Fail!!\n");
		return ret;
	}

	ss_panel_data_read(vdd, RX_FW_UP_READ, read_data, LEVEL_KEY_NONE);

	if (read_data[39] == vdd->fw_up.read_done_check) {
		LCD_ERR("Swire rework is already done, read_value = 0x%x\n", read_data[39]);
		return FW_UP_ERR_ALREADY_DONE;
	}

	LCD_INFO("Rework Done Check Value = 0x%x\n", read_data[39]);
	return ret;
}

static int ss_swire_rework(struct samsung_display_driver_data *vdd)
{
	int ret = FW_UP_DONE;
	int loop = 0;
	struct dsi_panel_cmd_set *erase_tx_cmds = NULL;
	struct dsi_panel_cmd_set *write_tx_cmds = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return FW_UP_ERR_UPDATE_FAIL;
	}

	write_tx_cmds = ss_get_cmds(vdd, TX_FW_UP_WRITE);
	if (SS_IS_CMDS_NULL(write_tx_cmds)) {
		LCD_ERR("No cmds for TX_FW_WRITE..\n");
		return FW_UP_ERR_UPDATE_FAIL;
	}

	erase_tx_cmds = ss_get_cmds(vdd, TX_FW_UP_ERASE);
	if (SS_IS_CMDS_NULL(erase_tx_cmds)) {
		LCD_ERR("No cmds for erase_tx_cmds..\n");
		return FW_UP_ERR_UPDATE_FAIL;
	}

	LCD_INFO("Swire Rework ++\n");

	/* FW ERASE */
	/* ddr [3~5] : 03 20 00 */
	erase_tx_cmds->cmds[0].msg.tx_buf[3] = 0x03;
	erase_tx_cmds->cmds[0].msg.tx_buf[4] = 0x20;
	erase_tx_cmds->cmds[0].msg.tx_buf[5] = 0x00;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, vdd->fw_up.erase_delay_us);
	if (ret) {
		LCD_ERR("FirmWare Erase Fail!!\n");
		return ret;
	}

	/* FW WRITE */
	for (loop = 0; loop < SWRIE_REWORK_SEQ_MAX; loop++) {
		memcpy(&write_tx_cmds->cmds[0].msg.tx_buf[1], swire_rework_seq[loop], SWRIE_REWORK_SEQ_SIZE);
		ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_WRITE, vdd->fw_up.write_delay_us);
		if (ret) {
			LCD_ERR("FirmWare Write [%d] Fail!!\n", loop);
			return ret;
		}
	}

	LCD_INFO("Swire Rework --\n");
	return ret;
}

static int ss_fwid_back(struct samsung_display_driver_data *vdd)
{
	int ret = FW_UP_DONE;
	int loop = 0;
	uint32_t addr;
	struct dsi_panel_cmd_set *erase_tx_cmds = NULL;
	struct dsi_panel_cmd_set *write_tx_cmds = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return FW_UP_ERR_UPDATE_FAIL;
	}

	write_tx_cmds = ss_get_cmds(vdd, TX_FW_UP_WRITE);
	if (SS_IS_CMDS_NULL(write_tx_cmds)) {
		LCD_ERR("No cmds for TX_FW_WRITE..\n");
		return FW_UP_ERR_UPDATE_FAIL;
	}

	erase_tx_cmds = ss_get_cmds(vdd, TX_FW_UP_ERASE);
	if (SS_IS_CMDS_NULL(erase_tx_cmds)) {
		LCD_ERR("No cmds for erase_tx_cmds..\n");
		return FW_UP_ERR_UPDATE_FAIL;
	}

	LCD_INFO("fwid_back ++\n");

	/* FW ERASE */
	/* ddr [3~5] : 01 00 00 */
	erase_tx_cmds->cmds[0].msg.tx_buf[3] = 0x01;
	erase_tx_cmds->cmds[0].msg.tx_buf[4] = 0x00;
	erase_tx_cmds->cmds[0].msg.tx_buf[5] = 0x00;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, vdd->fw_up.erase_delay_us);
	if (ret) {
		LCD_ERR("FirmWare Erase Fail!!\n");
		ss_write_fw_up_debug_partition(FW_FAIL_LINE, __LINE__);
		return ret;
	}

	/* FW WRITE */
	for (loop = 0; loop < 32; loop++) {  /* 32 to write 1 sectors */
		memcpy(&write_tx_cmds->cmds[0].msg.tx_buf[1], fw_back_id16[loop], 144);/* 128+16=144*/
		ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_WRITE, vdd->fw_up.write_delay_us);
		if (ret) {
			addr = (write_tx_cmds->cmds[0].msg.tx_buf[3] & 0x0000FF) << 16 |
				(write_tx_cmds->cmds[0].msg.tx_buf[4]) << 8 |
				write_tx_cmds->cmds[0].msg.tx_buf[5];
			ss_write_fw_up_debug_partition(FW_UP_FAIL, addr);
			LCD_ERR("FirmWare Write [%d] Fail!!\n", loop);
			return ret;
		}
	}

	/* FW ERASE */
	/* ddr [3~5] : 01 40 00 */
	erase_tx_cmds->cmds[0].msg.tx_buf[3] = 0x01;
	erase_tx_cmds->cmds[0].msg.tx_buf[4] = 0x40;
	erase_tx_cmds->cmds[0].msg.tx_buf[5] = 0x00;
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_ERASE, vdd->fw_up.erase_delay_us);
	if (ret) {
		LCD_ERR("FirmWare Erase Fail!!\n");
		ss_write_fw_up_debug_partition(FW_FAIL_LINE, __LINE__);
		return ret;
	}

	/* FW WRITE */
	for (loop = 32; loop < 64; loop++) {  /* 32*128 = 4096 */
		memcpy(&write_tx_cmds->cmds[0].msg.tx_buf[1], fw_back_id16[loop], 144);
		ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_WRITE, vdd->fw_up.write_delay_us);
		if (ret) {
			LCD_ERR("FirmWare Write [%d] Fail!!\n", loop);
			ss_write_fw_up_debug_partition(FW_UP_FAIL, addr);
			return ret;
		}
	}

	LCD_INFO("fwid_back --\n");
	return ret;
}

static int ss_mtp_id_update(struct samsung_display_driver_data *vdd)
{
	int ret = FW_UP_DONE;
	int loop = 0;
	u8 mtp_id_read[34];
	struct dsi_panel_cmd_set *mtp_id_write_tx_cmds = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return FW_UP_ERR_UPDATE_FAIL;
	}

	mtp_id_write_tx_cmds = ss_get_cmds(vdd, TX_FW_UP_MTP_ID_WRITE);
	if (SS_IS_CMDS_NULL(mtp_id_write_tx_cmds)) {
		LCD_ERR("No cmds for TX_FW_UP_MTP_ID_WRITE..\n");
		return FW_UP_ERR_UPDATE_FAIL;
	}

	LCD_INFO("MTP_ID Update ++\n");

	/* Original MTP_ID Data Read */
	ss_panel_data_read(vdd, RX_FW_UP_MTP_ID_READ, mtp_id_read, LEVEL_KEY_NONE);
	mtp_id_read[2] = 0x18;
	mtp_id_read[14] = 0x11; /* Byte15 = 11h */

	/* Copy and Change MTP_ID Data */
	memcpy(&mtp_id_write_tx_cmds->cmds[0].msg.tx_buf[17], mtp_id_read, SWIRE_REWORK_MTP_ID_LEN);

	/* MTP_ID ERASE */
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_MTP_ID_ERASE, vdd->fw_up.erase_delay_us);
	if (ret) {
		LCD_ERR("FirmWare MTP_ID Erase Fail!!\n");
		return ret;
	}

	/* MTP_ID WRITE */
	ret = ss_fw_up_send_cmd(vdd, TX_FW_UP_MTP_ID_WRITE, vdd->fw_up.write_delay_us);
	if (ret) {
		LCD_ERR("FirmWare Write [%d] Fail!!\n", loop);
		return ret;
	}

	LCD_INFO("MTP_ID Update --\n");
	return ret;
}

/*1: MTP ID 17->18 Swire Ctrl (eeprom)*/
/*2: FW ID 16->17 FW partial update */
static int ss_firmware_update(struct samsung_display_driver_data *vdd, int mode)
{
	int ret = FW_UP_DONE;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */
	struct dsi_display *display = NULL;
	struct msm_drm_private *priv = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_ERR("no display");
		return -EINVAL;
	}

	if (!vdd->fw_up.is_support) {
		LCD_ERR("FirmWare Update is not supported\n");
		return FW_UP_ERR_NOT_SUPPORT;
	}

	LCD_INFO("FirmWare Update Start\n");

	/* MAX CPU/BW ON */
	priv = display->drm_dev->dev_private;
	ss_set_max_cpufreq(vdd, true, CPUFREQ_CLUSTER_ALL);
	ss_set_max_mem_bw(vdd, true);
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_ON);

	/* Enter exclusive mode */
	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.permit_frame_update = false;
	vdd->exclusive_tx.enable = true;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500 * 2);
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_ERASE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_MTP_ID_ERASE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_WRITE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_MTP_ID_WRITE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_READ, 1);
	ss_set_exclusive_tx_packet(vdd, RX_FW_UP_READ, 1);
	ss_set_exclusive_tx_packet(vdd, RX_FW_UP_MTP_ID_READ, 1);
	ss_set_exclusive_tx_packet(vdd, RX_FW_UP_STATUS, 1);
	ss_set_exclusive_tx_packet(vdd, RX_FW_UP_CHECK, 1);
	ss_set_exclusive_tx_packet(vdd, TX_REG_READ_POS, 1);
	ss_set_exclusive_tx_packet(vdd, TX_NORMAL_BRIGHTNESS_ETC, 1);
	ss_set_exclusive_tx_packet(vdd, TX_FLASH_CLEAR_BUFFER, 1);

	if (mode == 1) {
		vdd->exclusive_tx.permit_frame_update = false;

		ret = ss_swire_rework_check(vdd);
		if (ret) {
			LCD_ERR("Skip SWIRE Rework\n");
			vdd->fw_up.cmd_done = true;
		} else {
			ret = ss_swire_rework(vdd);
			if (ret) {
				LCD_ERR("Swire Rework Fail\n");
				goto skip;
			}
			ret = ss_mtp_id_update(vdd);
			if (ret)
				LCD_ERR("MTP_ID Update Fail\n");
			else
				vdd->fw_up.cmd_done = true;
		}
	} else if (mode == 2)  { /* 64K update */
		if (vdd->check_fw_id == 0x16)
		{
			ret = ss_flash_erase(vdd);
			if (ret) {
				LCD_ERR("flash_erase failed, skip write.\n");
				goto skip;
			}

			ss_write_fw_up_debug_partition(FW_UP_TRY, 0);
			ret = ss_fw_write(vdd);
			if (ret) {
				LCD_ERR("write failed\n");
			} else {
				vdd->fw_up.cmd_done = true;
				ss_write_fw_up_debug_partition(FW_UP_PASS, 0);
			}

		} else
			LCD_ERR("Skip fw update..  fw_id:0x%x\n", vdd->check_fw_id);
	} else if (mode == 3) { /* back to fw_id 16 */
		if (vdd->check_fw_id == 0x17) {
			ret = ss_fwid_back(vdd);
			if (ret)
				LCD_ERR("ss_fwid_back failed\n");
		} else
			LCD_INFO("No need fw_back, fw_id:0x%x\n", vdd->check_fw_id);
	} else
		LCD_ERR("Invalid mode %d\n", mode);

skip:
	/* exit exclusive mode*/
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_ERASE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_MTP_ID_ERASE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_WRITE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_MTP_ID_WRITE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_FW_UP_READ, 0);
	ss_set_exclusive_tx_packet(vdd, RX_FW_UP_READ, 0);
	ss_set_exclusive_tx_packet(vdd, RX_FW_UP_MTP_ID_READ, 0);
	ss_set_exclusive_tx_packet(vdd, RX_FW_UP_STATUS, 0);
	ss_set_exclusive_tx_packet(vdd, RX_FW_UP_CHECK, 0);
	ss_set_exclusive_tx_packet(vdd, TX_REG_READ_POS, 0);
	ss_set_exclusive_tx_packet(vdd, TX_NORMAL_BRIGHTNESS_ETC, 0);
	ss_set_exclusive_tx_packet(vdd, TX_FLASH_CLEAR_BUFFER, 0);

	vdd->exclusive_tx.enable = false;
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);

	/* MAX CPU OFF */
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_OFF);
	ss_set_max_mem_bw(vdd, false);
	ss_set_max_cpufreq(vdd, false, CPUFREQ_CLUSTER_ALL);

	//vdd->debug_data->print_cmds = false;

	LCD_INFO("FirmWare Update Finish\n");

	return ret;
}


static int ss_self_display_data_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("++\n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR("Self Display is not supported\n");
		return -EINVAL;
	}

	vdd->self_disp.operation[FLAG_SELF_MASK].img_buf = self_mask_img_data;
	vdd->self_disp.operation[FLAG_SELF_MASK].img_size = ARRAY_SIZE(self_mask_img_data);
	vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum = SELF_MASK_IMG_CHECKSUM;
	make_self_dispaly_img_cmds_UM1(vdd, TX_SELF_MASK_IMAGE, FLAG_SELF_MASK);

	//vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_buf = self_mask_img_fhd_crc_data;
	//vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_size = ARRAY_SIZE(self_mask_img_fhd_crc_data);
	//make_self_dispaly_img_cmds_FA9(vdd, TX_SELF_MASK_IMAGE_CRC, FLAG_SELF_MASK_CRC);

	LCD_INFO("--\n");
	return 1;
}

static int poc_erase(struct samsung_display_driver_data *vdd, u32 erase_pos, u32 erase_size, u32 target_pos)
{
	struct dsi_display *display = NULL;
	struct dsi_panel_cmd_set *poc_erase_sector_tx_cmds = NULL;
	int delay_us = 0;
	int image_size = 0;
	int type;
	int ret = 0;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */
	u8 rx_buf;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_ERR("no display");
		return -EINVAL;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -EBUSY;
	}

	if (vdd->poc_driver.erase_sector_addr_idx[0] < 0) {
		LCD_ERR("sector addr index is not implemented.. %d\n",
			vdd->poc_driver.erase_sector_addr_idx[0]);
		return -EINVAL;
	}

	poc_erase_sector_tx_cmds = ss_get_cmds(vdd, TX_POC_ERASE_SECTOR);
	if (SS_IS_CMDS_NULL(poc_erase_sector_tx_cmds)) {
		LCD_ERR("No cmds for TX_POC_ERASE_SECTOR..\n");
		return -ENODEV;
	}

	image_size = vdd->poc_driver.image_size;
	delay_us = vdd->poc_driver.erase_delay_us;

	/* Size is set 4k always 00 10 00*/
#if 0
	if (erase_size == POC_ERASE_64KB) {
		delay_us = 1000000; /* 1000ms */
		poc_erase_sector_tx_cmds->cmds[0].msg.tx_buf[2] = 0xD8;
	} else if (erase_size == POC_ERASE_32KB) {
		delay_us = 800000; /* 800ms */
		poc_erase_sector_tx_cmds->cmds[0].msg.tx_buf[2] = 0x52;
	} else {
		delay_us = 400000; /* 400ms */
		poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[2] = 0x20;
	}
#endif

	LCD_INFO("[ERASE] (%6d / %6d), erase_size (%d), delay %dus\n",
		erase_pos, target_pos, erase_size, delay_us);

	/* MAX CPU & Memory BW ON */
	ss_set_max_cpufreq(vdd, true, CPUFREQ_CLUSTER_ALL);
	ss_set_max_mem_bw(vdd, true);
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_ON);

	/* Enter exclusive mode */
	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.permit_frame_update = 1;
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500 * 2);

	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 1);
	ss_set_exclusive_tx_packet(vdd, RX_POC_STATUS, 1);

	/* POC addr */
	poc_erase_sector_tx_cmds->cmds[0].msg.tx_buf[vdd->poc_driver.erase_sector_addr_idx[0]]
											= (erase_pos & 0xFF0000) >> 16;
	poc_erase_sector_tx_cmds->cmds[0].msg.tx_buf[vdd->poc_driver.erase_sector_addr_idx[1]]
											= (erase_pos & 0x00FF00) >> 8;
	poc_erase_sector_tx_cmds->cmds[0].msg.tx_buf[vdd->poc_driver.erase_sector_addr_idx[2]]
											= erase_pos & 0x0000FF;

	ss_send_cmd(vdd, TX_POC_ERASE_SECTOR);

	usleep_range(delay_us, delay_us * 2);

	if ((erase_pos + erase_size >= target_pos) || ret == -EIO) {
		LCD_INFO("WRITE [TX_POC_POST_ERASE_SECTOR] - cur_erase_pos(%d) target_pos(%d) ret(%d)\n",
			erase_pos, target_pos, ret);
	}

	/* Read status */
	ss_panel_data_read(vdd, RX_POC_STATUS, &rx_buf, LEVEL_KEY_NONE);
	if (!(rx_buf == 04))
		LCD_ERR("ERASE status not 04 ret:%d\n", rx_buf);

	/* exit exclusive mode*/
	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 0);
	ss_set_exclusive_tx_packet(vdd, RX_POC_STATUS, 0);
	vdd->exclusive_tx.permit_frame_update = 0;
	vdd->exclusive_tx.enable = 0;
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);

	/* MAX CPU & Memory BW OFF */
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_OFF);
	ss_set_max_mem_bw(vdd, false);
	ss_set_max_cpufreq(vdd, false, CPUFREQ_CLUSTER_ALL);

	return ret;
}

static int poc_write(struct samsung_display_driver_data *vdd, u8 *data, u32 write_pos, u32 write_size)
{
	struct dsi_panel_cmd_set *write_data_add = NULL;
	struct dsi_display *display = NULL;
	int pos, type, ret = 0;
	int last_pos, delay_us, image_size, poc_w_size;
	int tx_size;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */
	u8 rx_buf;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_ERR("no display");
		return -EINVAL;
	}

	write_data_add = ss_get_cmds(vdd, TX_POC_WRITE_LOOP_DATA_ADD);
	if (SS_IS_CMDS_NULL(write_data_add)) {
		LCD_ERR("no cmds for TX_POC_WRITE_LOOP_DATA_ADD..\n");
		return -EINVAL;
	}

	if (vdd->poc_driver.write_addr_idx[0] < 0) {
		LCD_ERR("write addr index is not implemented.. %d\n",
			vdd->poc_driver.write_addr_idx[0]);
		return -EINVAL;
	}

	delay_us = vdd->poc_driver.write_delay_us; /* Panel dtsi set */
	image_size = vdd->poc_driver.image_size;
	last_pos = write_pos + write_size;
	poc_w_size = vdd->poc_driver.write_data_size;

	LCD_INFO("[WRITE] write_pos : %6d, write_size : %6d, last_pos %6d/0x%x, poc_w_size:%d delay:%dus\n",
		write_pos, write_size, last_pos, last_pos, poc_w_size, delay_us);

	/* MAX CPU & Memory BW ON */
	ss_set_max_cpufreq(vdd, true, CPUFREQ_CLUSTER_ALL);
	ss_set_max_mem_bw(vdd, true);
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_ON);

	/* Enter exclusive mode */
	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.permit_frame_update = 1;
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500 * 2);

	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 1);
	ss_set_exclusive_tx_packet(vdd, RX_POC_STATUS, 1);

	pos = write_pos;

	LCD_INFO("WRITE_LOOP_START addr:0x%x /write_pos:0x%x  pos:%d data[pos]:0x%x\n",
		vdd->poc_driver.start_addr, write_pos, pos-vdd->poc_driver.start_addr, data[pos-vdd->poc_driver.start_addr]);
	for (pos ; pos < last_pos; ) {

		if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
			LCD_ERR("cancel poc write by user\n");
			ret = -EIO;
			goto cancel_poc;
		}

		if (last_pos - pos >= poc_w_size)
			tx_size = poc_w_size;
		else
			tx_size = last_pos - pos;

		memset(&write_data_add->cmds[0].msg.tx_buf[17], 0xFF, poc_w_size);
		memcpy(&write_data_add->cmds[0].msg.tx_buf[17], &data[pos-vdd->poc_driver.start_addr], tx_size);

		/*	Multi Data Address */
		write_data_add->cmds[0].msg.tx_buf[vdd->poc_driver.write_addr_idx[0]]
										= (pos & 0xFF0000) >> 16;
		write_data_add->cmds[0].msg.tx_buf[vdd->poc_driver.write_addr_idx[1]]
										= (pos & 0x00FF00) >> 8;
		write_data_add->cmds[0].msg.tx_buf[vdd->poc_driver.write_addr_idx[2]]
										= (pos & 0x0000FF);
		ss_send_cmd(vdd, TX_POC_WRITE_LOOP_DATA_ADD);
		usleep_range(delay_us, delay_us * 2);

		/* Read status */
		ss_panel_data_read(vdd, RX_POC_STATUS, &rx_buf, LEVEL_KEY_NONE);
		if (!(rx_buf == 04)) {
			LCD_ERR("ERASE status not 04 ret:%d\n", rx_buf);
			usleep_range(delay_us, delay_us * 2);

			/* Read status */
			ss_panel_data_read(vdd, RX_POC_STATUS, &rx_buf, LEVEL_KEY_NONE);
		}

		LCD_INFO("WRITE_LOOP pos:%d/0x%x data[pos]:0x%x tx_size:%d\n",
			pos, pos, data[pos-vdd->poc_driver.start_addr], tx_size);

		usleep_range(10000, 10500);

		pos += tx_size;
	}

cancel_poc:
	if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
		LCD_ERR("cancel poc write by user\n");
		atomic_set(&vdd->poc_driver.cancel, 0);
		ret = -EIO;
	}

	if (pos == last_pos) {
		poc_done = 1;
		LCD_INFO("WRITE_LOOP_END pos:%d poc:%d\n", pos, poc_done);
	}

	/* exit exclusive mode*/
	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 0);
		ss_set_exclusive_tx_packet(vdd, RX_POC_STATUS, 0);
	vdd->exclusive_tx.permit_frame_update = 0;
	vdd->exclusive_tx.enable = 0;
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);

	/* MAX CPU & Memory BW OFF */
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_OFF);
	ss_set_max_mem_bw(vdd, false);
	ss_set_max_cpufreq(vdd, false, CPUFREQ_CLUSTER_ALL);

	return ret;
}

#define read_buf_size 131
#define read_length 128
static int poc_read(struct samsung_display_driver_data *vdd, u8 *buf, u32 read_pos, u32 read_size)
{
	struct dsi_display *display = NULL;
	struct dsi_panel_cmd_set *poc_read_tx_cmds = NULL;
	struct dsi_panel_cmd_set *poc_read_rx_cmds = NULL;
	int delay_us;
	int last_pos;
	int image_size;
	u8 rx_buf[read_buf_size], status_c1;
	int pos;
	int type;
	int ret = 0;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */
	int read_len = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_ERR("no display");
		return -EINVAL;
	}

	poc_read_tx_cmds = ss_get_cmds(vdd, TX_POC_READ);
	if (SS_IS_CMDS_NULL(poc_read_tx_cmds)) {
		LCD_ERR("no cmds for TX_POC_READ..\n");
		return -EINVAL;
	}

	poc_read_rx_cmds = ss_get_cmds(vdd, RX_POC_READ);
	if (SS_IS_CMDS_NULL(poc_read_rx_cmds)) {
		LCD_ERR("no cmds for RX_POC_READ..\n");
		return -EINVAL;
	}

	if (vdd->poc_driver.read_addr_idx[0] < 0) {
		LCD_ERR("read addr index is not implemented.. %d\n",
			vdd->poc_driver.read_addr_idx[0]);
		return -EINVAL;
	}

	delay_us = vdd->poc_driver.read_delay_us; /* Panel dtsi set */
	image_size = vdd->poc_driver.image_size;

	LCD_INFO("[READ] read_pos : %6d/0x%06x, read_size:%6d, delay:%dus\n",
			read_pos, read_pos, read_size, delay_us);

	/* MAX CPU & Memory BW ON */
	ss_set_max_cpufreq(vdd, true, CPUFREQ_CLUSTER_ALL);
	ss_set_max_mem_bw(vdd, true);
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_ON);

	/* Enter exclusive mode */
	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.permit_frame_update = 1;
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500 * 2);

	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 1);
	ss_set_exclusive_tx_packet(vdd, RX_POC_READ, 1);
	ss_set_exclusive_tx_packet(vdd, RX_POC_STATUS, 1);

	last_pos = read_pos + read_size;
	pos = read_pos;

	for (pos ; pos < (read_pos + read_size); pos += 128) {
		if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
			LCD_ERR("cancel poc read by user\n");
			ret = -EIO;
			goto cancel_poc;
		}

		poc_read_tx_cmds->cmds[0].msg.tx_buf[vdd->poc_driver.read_addr_idx[0]]
									= (pos & 0xFF0000) >> 16;
		poc_read_tx_cmds->cmds[0].msg.tx_buf[vdd->poc_driver.read_addr_idx[1]]
									= (pos & 0x00FF00) >> 8;
		poc_read_tx_cmds->cmds[0].msg.tx_buf[vdd->poc_driver.read_addr_idx[2]]
									= pos & 0x0000FF;
		//LCD_INFO("read addr = 0x%x\n", pos, buf[pos]);

		ss_send_cmd(vdd, TX_POC_READ);

		usleep_range(delay_us, delay_us + 10);

		ss_panel_data_read(vdd, RX_POC_STATUS, &status_c1, LEVEL_KEY_NONE);
			if (!(status_c1 == 04))
				LCD_ERR("READ status not 04 ret:%d\n", status_c1);
		LCD_INFO("READ status_c1: %02x\n", status_c1); /* Read status 1 byte*/

		//ss_send_cmd(vdd, RX_POC_READ); /* Read 128byte */
		ss_panel_data_read(vdd, RX_POC_READ, rx_buf, LEVEL_KEY_NONE);

		read_len = (read_pos + read_size) - pos;	//read length remaining for next
		if (read_len > read_length)
			read_len = read_length;

		memcpy(&buf[pos-vdd->poc_driver.start_addr], rx_buf, read_len);
		LCD_INFO("Copied from buf[%d] = 0x%x, pos:0x%x, read_pos:0x%x ===\n",
			pos-vdd->poc_driver.start_addr, buf[pos-vdd->poc_driver.start_addr], pos, read_pos);

	}

cancel_poc:
	if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
		LCD_ERR("cancel poc read by user\n");
		atomic_set(&vdd->poc_driver.cancel, 0);
		ret = -EIO;
	}

	if (pos == image_size || ret == -EIO)
		LCD_INFO("Read END - image_size(%d) cur_read_pos(%d) ret(%d)\n",
			image_size, pos, ret);

	/* MAX CPU & Memory BW OFF */
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_OFF);
	ss_set_max_mem_bw(vdd, false);
	ss_set_max_cpufreq(vdd, false, CPUFREQ_CLUSTER_ALL);

	/* Exit exclusive mode*/
	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 0);
	ss_set_exclusive_tx_packet(vdd, RX_POC_READ, 0);
	ss_set_exclusive_tx_packet(vdd, RX_POC_STATUS, 0);

	vdd->exclusive_tx.permit_frame_update = 0;
	vdd->exclusive_tx.enable = 0;
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);

	LCD_INFO("Read END! - image_size(%d) cur_read_pos(%d) ret(%d)\n", image_size, pos, ret);

	return ret;
}


static int samsung_panel_off_pre(struct samsung_display_driver_data *vdd)
{
	int rc = 0;
	return rc;
}

static int samsung_panel_off_post(struct samsung_display_driver_data *vdd)
{
	int rc = 0;

	init_smooth_off = 0;
	return rc;
}

static int ss_vrr_init(struct vrr_info *vrr)
{
	struct vrr_bridge_rr_tbl *brr;

	LCD_INFO("+++\n");

	mutex_init(&vrr->vrr_lock);
	mutex_init(&vrr->brr_lock);

	vrr->running_vrr_mdp = false;
	vrr->running_vrr = false;

	/* Bootloader: WQXGA@60hz HS mode? */
	vrr->cur_refresh_rate = vrr->adjusted_refresh_rate = 120;
	vrr->cur_sot_hs_mode = vrr->adjusted_sot_hs_mode = true;

	vrr->hs_nm_seq = HS_NM_OFF;
	vrr->delayed_perf_normal = false;
	vrr->skip_vrr_in_brightness = false;

	vrr->brr_mode = BRR_OFF_MODE;
	vrr->brr_rewind_on = false;

	vrr->vrr_workqueue = create_singlethread_workqueue("vrr_workqueue");
	INIT_WORK(&vrr->vrr_work, ss_panel_vrr_switch_work);

	/* 60HS -> 120HS -> 96HS */
	brr = &vrr->brr_tbl[BRR_60HS_96];
	brr->tot_steps = 1;
	brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
	if (!brr->fps)
		return -ENOMEM;
	brr->fps[0] = 120;
	brr->fps_start = 60;
	brr->fps_end = 96;
	brr->fps_base = 120;	/* Just for interpolation. */
	brr->vbp_base = 16;	/* Just for interpolation. */
	brr->vactive_base = 1752;	/* Just for interpolation. */
	brr->vfp_base = 16;	/* Just for interpolation. */
	brr->sot_hs_base = true;
	brr->min_cd = 2;
	brr->max_cd = 500;
	brr->delay_frame_cnt = 4;

	/* 96HS -> 120HS -> 60HS */
	brr = &vrr->brr_tbl[BRR_96_60HS];
	brr->tot_steps = 1;
	brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
	if (!brr->fps)
		return -ENOMEM;
	brr->fps[0] = 120;
	brr->fps_start = 96;
	brr->fps_end = 60;
	brr->fps_base = 120;	/* Just for interpolation. */
	brr->vbp_base = 16;
	brr->vactive_base = 1752;
	brr->vfp_base = 16;
	brr->sot_hs_base = true;
	brr->min_cd = 2;
	brr->max_cd = 500;
	brr->delay_frame_cnt = 3;

	LCD_INFO("---\n");

	return 0;
}

static void samsung_panel_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("S6TUUM3_AMSA24VU01 : ++ \n");
	LCD_ERR("%s\n", ss_get_panel_name(vdd));

	/* Default Panel Power Status is OFF */
	vdd->panel_state = PANEL_PWR_OFF;

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = samsung_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = samsung_panel_on_post;
	vdd->panel_func.samsung_panel_off_pre = samsung_panel_off_pre;
	vdd->panel_func.samsung_panel_off_post = samsung_panel_off_post;

	/* DDI RX */
	vdd->panel_func.samsung_panel_revision = ss_panel_revision;
	vdd->panel_func.samsung_manufacture_date_read = ss_manufacture_date_read;//
	vdd->panel_func.samsung_ddi_id_read = ss_ddi_id_read;
	vdd->panel_func.samsung_smart_dimming_init = NULL;
	vdd->panel_func.samsung_cell_id_read = ss_cell_id_read;
	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;
	vdd->panel_func.samsung_elvss_read = ss_elvss_read; // no need?
	vdd->panel_func.samsung_elvss_read = NULL;
	vdd->panel_func.samsung_mdnie_read = ss_mdnie_read;

	/* Brightness */
	vdd->panel_func.samsung_brightness_gamma = ss_brightness_gamma_mode2_normal;
	vdd->panel_func.samsung_brightness_acl_on = ss_acl_on;
	vdd->panel_func.samsung_brightness_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_hbm_off = NULL;
	vdd->panel_func.samsung_brightness_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_elvss = ss_elvss;
	vdd->panel_func.samsung_brightness_elvss_temperature1 = NULL;
	vdd->panel_func.samsung_brightness_elvss_temperature2 = NULL;
	vdd->panel_func.samsung_brightness_vint = NULL;
	vdd->panel_func.samsung_brightness_irc = ss_irc;
	vdd->panel_func.samsung_brightness_vrr = ss_vrr;
	vdd->panel_func.samsung_brightness_gm2_gamma_comp = ss_brightness_gm2_gamma_comp;

	vdd->br_info.smart_dimming_loaded_dsi = false;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_brightness_gamma_mode2_hbm;
	vdd->panel_func.samsung_hbm_acl_on = ss_acl_on_hbm;
	vdd->panel_func.samsung_hbm_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_vrr_hbm = ss_vrr_hbm;

	/* Event */
	vdd->panel_func.samsung_change_ldi_fps = NULL;

	/* HMT */
	vdd->panel_func.samsung_brightness_gamma_hmt = NULL;
	vdd->panel_func.samsung_brightness_aid_hmt = NULL;
	vdd->panel_func.samsung_brightness_elvss_hmt = NULL;
	vdd->panel_func.samsung_brightness_vint_hmt = NULL;
	vdd->panel_func.samsung_smart_dimming_hmt_init = NULL;
	vdd->panel_func.samsung_smart_get_conf_hmt = NULL;
	vdd->panel_func.samsung_brightness_hmt = NULL;

	/* Panel LPM */
	vdd->panel_func.samsung_update_lpm_ctrl_cmd = NULL;
	vdd->panel_func.samsung_set_lpm_brightness = NULL;


	/* default brightness */
	vdd->br_info.common_br.bl_level = 255;

	/* mdnie */
	vdd->mdnie.support_mdnie = true;

	vdd->mdnie.support_trans_dimming = true;
	vdd->mdnie.mdnie_tune_size[0] = sizeof(DSI0_BYPASS_MDNIE_1);
	vdd->mdnie.mdnie_tune_size[1] = sizeof(DSI0_BYPASS_MDNIE_2);
	vdd->mdnie.mdnie_tune_size[2] = sizeof(DSI0_BYPASS_MDNIE_3);

	dsi_update_mdnie_data(vdd);

	/* send recovery pck before sending image date (for ESD recovery) */
	vdd->esd_recovery.send_esd_recovery = false;

	/* Enable panic on first pingpong timeout */
	//vdd->debug_data->panic_on_pptimeout = true;

	/* Set IRC init value */
	//vdd->br_info.common_br.irc_mode = IRC_MODERATO_MODE;

	/* COLOR WEAKNESS */
	vdd->panel_func.color_weakness_ccb_on_off =  NULL;

	/* Support DDI HW CURSOR */
	vdd->panel_func.ddi_hw_cursor = NULL;

	/* COVER Open/Close */
	vdd->panel_func.samsung_cover_control = NULL;

	/* COPR */
	//vdd->copr.panel_init = ss_copr_panel_init;

	/* ACL default ON */
	vdd->br_info.acl_status = 1;
	vdd->br_info.temperature = 20; // default temperature

	/*Default Temperature*/
	vdd->br_info.temperature = 20;

	/* ACL default status in acl on */
	//vdd->br_info.gradual_acl_val = 1;

	/* Gram Checksum Test */
	vdd->panel_func.samsung_gct_write = NULL;
	vdd->panel_func.samsung_gct_read = NULL;

	/* Self display */
	vdd->self_disp.is_support = true;
	//vdd->self_disp.factory_support = true;
	vdd->self_disp.init = self_display_init_UM1;
	vdd->self_disp.data_init = ss_self_display_data_init;

	/* SAMSUNG_FINGERPRINT */
	vdd->panel_hbm_entry_delay = 1;
	vdd->panel_hbm_exit_delay = 1;
	vdd->panel_hbm_entry_after_te = 1000;
	/* VRR */
	ss_vrr_init(&vdd->vrr);

	/* POC */
	vdd->poc_driver.poc_erase = poc_erase;
	vdd->poc_driver.poc_write = poc_write;
	vdd->poc_driver.poc_read = poc_read;
	vdd->poc_driver.poc_comp = NULL;

	/* FirmWare Update */
	vdd->panel_func.samsung_fw_up = ss_firmware_update;

	vdd->debug_data->print_cmds = false;
	prev_bl_s6tuum3 = 0;
	prev_refresh_rate = 0;
	poc_done = 0;

	LCD_INFO("S6TUUM3_AMSA24VU01 : -- \n");
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_S6TUUM3_AMSA24VU01_WQXGA";
	char panel_name[MAX_CMDLINE_PARAM_LEN];
	char panel_secondary_name[MAX_CMDLINE_PARAM_LEN];

	LCD_INFO("S6TUUM3_AMSA24VU01 : ++ \n");

	ss_get_primary_panel_name_cmdline(panel_name);
	ss_get_secondary_panel_name_cmdline(panel_secondary_name);

	/* TODO: use component_bind with panel_func
	 * and match by panel_string, instead.
	 */
	if (!strncmp(panel_string, panel_name, strlen(panel_string)))
		ndx = PRIMARY_DISPLAY_NDX;
	else if (!strncmp(panel_string, panel_secondary_name, strlen(panel_string)))
		ndx = SECONDARY_DISPLAY_NDX;
	else {
		LCD_ERR("panel_string %s can not find panel_name (%s, %s)\n", panel_string, panel_name, panel_secondary_name);

		return 0;
	}

	vdd = ss_get_vdd(ndx);
	vdd->panel_func.samsung_panel_init = samsung_panel_init;

	if (ndx == PRIMARY_DISPLAY_NDX)
		LCD_INFO("%s done.. \n", panel_name);
	else
		LCD_INFO("%s done.. \n", panel_secondary_name);
	LCD_INFO("S6TUUM3_AMSA24VU01 : -- \n");

	return 0;
}

early_initcall(samsung_panel_initialize);
