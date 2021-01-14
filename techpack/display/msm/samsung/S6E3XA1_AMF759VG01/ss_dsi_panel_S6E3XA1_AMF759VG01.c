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
#include "ss_dsi_panel_S6E3XA1_AMF759VG01.h"
#include "ss_dsi_mdnie_S6E3XA1_AMF759VG01.h"

/* AOD Mode status on AOD Service */

enum {
	HLPM_CTRL_2NIT,
	HLPM_CTRL_10NIT,
	HLPM_CTRL_30NIT,
	HLPM_CTRL_60NIT,
	MAX_LPM_CTRL,
};


/* Register to control brightness level */
#define ALPM_REG	0x53
/* Register to cnotrol ALPM/HLPM mode */
#define ALPM_CTRL_REG	0xBB

#define IRC_MODERATO_MODE_VAL	0x61
#define IRC_FLAT_GAMMA_MODE_VAL	0x21

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data vdd : 0x%zx", __func__, (size_t)vdd);
		return false;
	}

	LCD_INFO("+: ndx=%d\n", vdd->ndx);
	ss_panel_attach_set(vdd, true);

	return true;
}
extern unsigned int lpcharge;

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	struct vrr_info *vrr = &vdd->vrr;
	int cur_rr = vrr->cur_refresh_rate;

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

	/* mafpc */
	if (vdd->mafpc.is_support) {
		vdd->mafpc.need_to_write = true;
		LCD_INFO("Need to write mafpc image data to DDI\n");
	}
	LCD_INFO("lpcharge = %d vdd->is_recovery_mode %d\n", lpcharge, vdd->is_recovery_mode);
	/* to avoid tearing issue at lpm mode & recovery mode, 
		pull te rising timing to get more margin to mipi image tx */
	if (lpcharge || vdd->is_recovery_mode)
		ss_send_cmd(vdd, TX_ADJUST_TE);

	if (cur_rr == 48 | cur_rr == 96) {
		/* GAMMA MTP is reset to original MTP value by display off -> on.
		 * In 48/96hz mode, apply compensated gamma.
		 */
		vrr->gm2_gamma = VRR_GM2_GAMMA_COMPENSATE;
		LCD_INFO("compensate gamma for 48/96hz mode\n");
	}

	return true;
}

static char ss_panel_revision(struct samsung_display_driver_data *vdd)
{
	char panel_rev, opcode_rev;
	if (vdd->manufacture_id_dsi == PBA_ID)
		ss_panel_attach_set(vdd, false);
	else
		ss_panel_attach_set(vdd, true);

	panel_rev = ss_panel_id0_get(vdd);
	opcode_rev = ss_panel_id2_get(vdd);

	switch (panel_rev) {
	case 0x81:
		vdd->panel_revision = 'A';
		break;
	case 0x91:
		vdd->panel_revision = 'C';
		switch (opcode_rev) {
			case 0x04:
				vdd->panel_revision = 'D';
				break;
			default:
				vdd->panel_revision = 'C';
		}
		break;

	default:
		vdd->panel_revision = 'D';
		LCD_ERR("Invalid panel_rev(default rev : %c)\n",
				vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';

	LCD_INFO("panel_revision = %c %d \n",
					vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision + 'A');
}


#define get_bit(value, shift, width)	((value >> shift) & (GENMASK(width - 1, 0)))
static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_normal(struct samsung_display_driver_data *vdd, int *level_key)
{
	//struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	struct dsi_panel_cmd_set *pcmds;
	int tset;

	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_NORMAL);

	tset =  vdd->br_info.temperature > 0 ?  vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));


	pcmds->cmds[0].msg.tx_buf[1] = vdd->finger_mask_updated? 0x20 : 0x28;  /* Normal Smooth transition : 0x28 */
	pcmds->cmds[1].msg.tx_buf[1] = tset;

	pcmds->cmds[2].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 2);
	pcmds->cmds[2].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

	/* IRC mode: 0xE1: Moderato mode, 0xA1: flat gamma mode */
	if (vdd->br_info.common_br.irc_mode == IRC_MODERATO_MODE)
		pcmds->cmds[4].msg.tx_buf[2] = 0xE1;
	else if (vdd->br_info.common_br.irc_mode == IRC_FLAT_GAMMA_MODE)
		pcmds->cmds[4].msg.tx_buf[2] = 0xA1;
	else
		LCD_ERR("invalid irc mode(%d)\n", vdd->br_info.common_br.irc_mode);

	LCD_INFO("cd_idx: %d, cd_level: %d, WRDISBV: %x %x, tset: %x\n",
			vdd->br_info.common_br.cd_idx,
			vdd->br_info.common_br.cd_level,
			pcmds->cmds[2].msg.tx_buf[1],
			pcmds->cmds[2].msg.tx_buf[2],
			pcmds->cmds[1].msg.tx_buf[1]);
	*level_key = LEVEL1_KEY;

	return pcmds;
}

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_hbm(struct samsung_display_driver_data *vdd, int *level_key)
{
	//struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	struct dsi_panel_cmd_set *pcmds;
	int tset;

	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HBM);

	tset =  vdd->br_info.temperature > 0 ?  vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

	pcmds->cmds[1].msg.tx_buf[1] = tset;

	pcmds->cmds[1].msg.tx_buf[3] = elvss_table_hbm_revA[vdd->br_info.common_br.cd_idx]; 	/* ELVSS Value for HBM brgihtness */

	pcmds->cmds[2].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 2);
	pcmds->cmds[2].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

	/* IRC mode: 0xE1: Moderato mode, 0xA1: flat gamma mode */
	if (vdd->br_info.common_br.irc_mode == IRC_MODERATO_MODE)
		pcmds->cmds[4].msg.tx_buf[2] = 0xE1;
	else if (vdd->br_info.common_br.irc_mode == IRC_FLAT_GAMMA_MODE)
		pcmds->cmds[4].msg.tx_buf[2] = 0xA1;
	else
		LCD_ERR("invalid irc mode(%d)\n", vdd->br_info.common_br.irc_mode);

	LCD_INFO("cd_idx: %d, cd_level: %d, WRDISBV: %x %x, elvss: %x, tset: %x\n",
			vdd->br_info.common_br.cd_idx,
			vdd->br_info.common_br.cd_level,
			pcmds->cmds[2].msg.tx_buf[1],
			pcmds->cmds[2].msg.tx_buf[2],
			pcmds->cmds[1].msg.tx_buf[3],
			pcmds->cmds[1].msg.tx_buf[1]);
	*level_key = LEVEL1_KEY;

	return pcmds;
}

/* flash map
 * F0000 ~ F0053: 2 mode * 3 temperature * 14 vbias cmd = 84 bytes
 * F0054 ~ F0055: checksum = 2 bytes
 * F0056: 1 byte
 * F0057 ~ F0058: NS/HS setting = 2 bytes
 */
static int ss_gm2_ddi_flash_prepare_revA(struct samsung_display_driver_data *vdd)
{
	u8 checksum_buf[2];
	int checksum_tot_flash;
	int checksum_tot_cal = 0;

	struct flash_gm2 *gm2_table = &vdd->br_info.gm2_table;
	u8 *ddi_flash_raw_buf;
	int ddi_flash_raw_len = gm2_table->ddi_flash_raw_len;
	int start_addr = gm2_table->ddi_flash_start_addr;

	u8 *vbias_data; /* 2 modes (NS/HS) * 3 temperature modes * 22 bytes cmd = 132 */

	int mode;
	int i;	
	char read_buf = 0;
	int addr, loop_cnt;

	ddi_flash_raw_buf = gm2_table->ddi_flash_raw_buf;
	vbias_data = gm2_table->vbias_data;
	
	LCD_INFO("ddi_flash_raw_len: %d\n", ddi_flash_raw_len);

	ss_send_cmd(vdd, TX_FLASH_GAMMA_PRE1);
	
	for (loop_cnt = 0, addr = start_addr; addr < start_addr + ddi_flash_raw_len ; addr++, loop_cnt++) {
		read_buf = flash_read_one_byte(vdd, addr);
		ddi_flash_raw_buf[loop_cnt] = read_buf;
	}

	ss_send_cmd(vdd, TX_FLASH_GAMMA_POST);

	for (i = 0; i < ddi_flash_raw_len; i++)
		LCD_INFO("ddi_flash_raw_buf[%d]: %X\n", i, ddi_flash_raw_buf[i]);

	/* save vbias data to vbias & vint buffer according to panel spec. */
	for (mode = VBIAS_MODE_NS_WARM; mode < VBIAS_MODE_MAX; mode++) {
		u8 *vbias_one_mode = ss_get_vbias_data(gm2_table, mode);

		memcpy(vbias_one_mode, ddi_flash_raw_buf + (mode * 14), 14);
		memset(vbias_one_mode + 14, 0, 7);
	}

	/* flash: F0054 ~ F0055: off_gm2_flash_checksum = 0x56 -2 0x54 = 84 */
	memcpy(checksum_buf, ddi_flash_raw_buf + gm2_table->off_gm2_flash_checksum - 2, 2);

	checksum_tot_flash = (checksum_buf[0] << 8) | checksum_buf[1];

	/* 16bit sum chec: F0000 ~ F0053 ff_gm2_flash_checksum = 0x56, -2 54*/
	for (i = 0; i < gm2_table->off_gm2_flash_checksum - 2; i++) {
		checksum_tot_cal += ddi_flash_raw_buf[i];
		LCD_INFO("ddi_flash_raw_buf[%d] = 0x%x\n", i, ddi_flash_raw_buf[i]);
	}

	checksum_tot_cal &= ERASED_MMC_16BIT;

	LCD_INFO("checksum: %08X %08X, one mode comp: %s\n",
			checksum_tot_flash,
			checksum_tot_cal,
			"not SUPPORT");


	gm2_table->is_flash_checksum_ok = 0;
	LCD_ERR("ddi flash test: FAIL..\n");
	if (checksum_tot_flash != checksum_tot_cal)
		LCD_ERR("total flash checksum err: flash: %X, cal: %X\n",
				checksum_tot_flash, checksum_tot_cal);
	else
		LCD_ERR("total flash checksum SUCCESS\n");

	LCD_ERR("one vbias mode err, not SUPPORT (under REVD)\n");

	gm2_table->checksum_tot_flash = checksum_tot_flash;
	gm2_table->checksum_tot_cal = checksum_tot_cal;
	gm2_table->checksum_one_mode_mtp = 0xDE;
	gm2_table->checksum_one_mode_flash = 0xAD;

	gm2_table->is_diff_one_vbias_mode = 0xAD;

	return 0;
}

static int ss_gm2_ddi_flash_prepare_revD(struct samsung_display_driver_data *vdd)
{
	u8 checksum_buf[2];
	int checksum_tot_flash;
	int checksum_tot_cal = 0;
	int checksum_one_mode_flash;
	int is_diff_one_vbias_mode;

	struct flash_gm2 *gm2_table = &vdd->br_info.gm2_table;
	u8 *ddi_flash_raw_buf;
	int ddi_flash_raw_len = gm2_table->ddi_flash_raw_len;
	int start_addr = gm2_table->ddi_flash_start_addr;

	u8 *vbias_data; /* 2 modes (NS/HS) * 3 temperature modes * 22 bytes cmd = 132 */

	u8 *mtp_one_vbias_mode; /* 22 bytes, F4h 42nd ~ 55th, and 63rd TBD_TOP */ 
	u8 *flash_one_vbias_mode; /* NS and T > 0 mode, 22 bytes */
	int len_one_vbias_mode = gm2_table->len_one_vbias_mode; // 22 bytes TBD_TOP

	int mode;
	int i;	
	char read_buf = 0;
	int addr, loop_cnt;

	ddi_flash_raw_buf = gm2_table->ddi_flash_raw_buf;
	vbias_data = gm2_table->vbias_data;
	mtp_one_vbias_mode = gm2_table->mtp_one_vbias_mode;
	
	LCD_INFO("ddi_flash_raw_len: %d\n", ddi_flash_raw_len);

	ss_send_cmd(vdd, TX_FLASH_GAMMA_PRE1);
	
	for (loop_cnt = 0, addr = start_addr; addr < start_addr + ddi_flash_raw_len ; addr++, loop_cnt++) {
		read_buf = flash_read_one_byte(vdd, addr);
		ddi_flash_raw_buf[loop_cnt] = read_buf;
	}

	ss_send_cmd(vdd, TX_FLASH_GAMMA_POST);

	for (i = 0; i < ddi_flash_raw_len; i++)
		LCD_INFO("ddi_flash_raw_buf[%d]: %X\n", i, ddi_flash_raw_buf[i]);

	/* save vbias data to vbias & vint buffer according to panel spec. */
	for (mode = VBIAS_MODE_NS_WARM; mode < VBIAS_MODE_MAX; mode++) {
		u8 *vbias_one_mode = ss_get_vbias_data(gm2_table, mode);

		memcpy(vbias_one_mode, ddi_flash_raw_buf + (mode * 14), 14);
		memset(vbias_one_mode + 14, 0, 7);
		if (i <= VBIAS_MODE_NS_COLD) /* NS: flash: F0054 ddi_flash_raw_len = 0x59*/
			vbias_one_mode[21] = ddi_flash_raw_buf[ddi_flash_raw_len - 5];
		else 	/* HS: flash: F0055 */
			vbias_one_mode[21] = ddi_flash_raw_buf[ddi_flash_raw_len - 4];
	}

	/* flash: F0056 ~ F0057: off_gm2_flash_checksum = 0x56 = 86 */
	memcpy(checksum_buf, ddi_flash_raw_buf + gm2_table->off_gm2_flash_checksum, 2);

	checksum_tot_flash = (checksum_buf[0] << 8) | checksum_buf[1];

	/* 16bit sum chec: F0000 ~ F0053 */
	for (i = 0; i < gm2_table->off_gm2_flash_checksum; i++)
		checksum_tot_cal += ddi_flash_raw_buf[i];

	checksum_tot_cal &= ERASED_MMC_16BIT;

	/* checksum of one mode from flash (15bytes, NS and T > 0) */
	flash_one_vbias_mode = ss_get_vbias_data(gm2_table, VBIAS_MODE_HS_WARM);
	checksum_one_mode_flash = 0;
	for (i = 0; i < len_one_vbias_mode; i++) {
		checksum_one_mode_flash += flash_one_vbias_mode[i];
		LCD_INFO("flash_one_vbias_mode[%d] = 0x%x\n", i, flash_one_vbias_mode[i]);
	}
	checksum_one_mode_flash &= ERASED_MMC_16BIT;

	/* configure and calculate checksum of one vbias mode from MTP.
	 * F4h 42nd ~ 55th, and 63rd, and calculate checksum 
	 */
	memset(gm2_table->mtp_one_vbias_mode + 7, 0, 17); /* 48th~54th & 72th~79th */
	gm2_table->checksum_one_mode_mtp = 0;
	for (i = 0; i < gm2_table->len_mtp_one_vbias_mode; i++) {
		gm2_table->checksum_one_mode_mtp += gm2_table->mtp_one_vbias_mode[i];
		LCD_INFO("gm2_table->mtp_one_vbias_mode[%d] = 0x%x\n", i, gm2_table->mtp_one_vbias_mode[i]);
	}
	gm2_table->checksum_one_mode_mtp &= ERASED_MMC_16BIT;

	/* compare one vbias mode data: FLASH Vs. MTP (22 bytes) */
	is_diff_one_vbias_mode = gm2_table->checksum_one_mode_mtp == checksum_one_mode_flash ? 0 : 1;

	LCD_INFO("checksum: %08X %08X %08X %08X, one mode comp: %s\n",
			checksum_tot_flash,
			checksum_tot_cal,
			gm2_table->checksum_one_mode_mtp,
			checksum_one_mode_flash,
			is_diff_one_vbias_mode ? "NG" : "PASS");

	/* save flash test result */
	if (checksum_tot_flash == checksum_tot_cal && !is_diff_one_vbias_mode) {
		gm2_table->is_flash_checksum_ok = 1;
		LCD_INFO("ddi flash test: OK\n");
	} else {
		gm2_table->is_flash_checksum_ok = 0;
		LCD_ERR("ddi flash test: FAIL..\n");
		if (checksum_tot_flash != checksum_tot_cal)
			LCD_ERR("total flash checksum err: flash: %X, cal: %X\n",
					checksum_tot_flash, checksum_tot_cal);

		if (is_diff_one_vbias_mode) {
			LCD_ERR("one vbias mode err..\n");
			for (i = 0; i < len_one_vbias_mode; i++)
				LCD_ERR("  [%d] %X - %X\n", i, mtp_one_vbias_mode[i], flash_one_vbias_mode[i]);
		}
	}

	gm2_table->checksum_tot_flash = checksum_tot_flash;
	gm2_table->checksum_tot_cal = checksum_tot_cal;
	gm2_table->checksum_one_mode_flash = checksum_one_mode_flash;
	gm2_table->is_diff_one_vbias_mode = is_diff_one_vbias_mode;

	return 0;
}
static int ss_gm2_ddi_flash_prepare(struct samsung_display_driver_data *vdd)
{
	if (vdd->panel_revision < 3)
		ss_gm2_ddi_flash_prepare_revA(vdd);
	else /*from 0x91XX04*/
		ss_gm2_ddi_flash_prepare_revD(vdd);

	return 0;
}

/*
 * convert_GAMMA_to_V_XA1 : convert CA gamma reg to V format
 * src : packed gamma value
 * dst : extended V values (V255 ~ VT)
 */
static void convert_GAMMA_to_V_XA1(u8 *src, int *dst)
{
	/* i : V index
	 * j : RGB index
	 * k : extend V index
	 */
	int i, j, k = 0;

	memset(dst, 0, GAMMA_V_SIZE * sizeof(int));

	for (i = V255; i < V_MAX; i++) {
		for (j = R; j < RGB_MAX; j++) {
			if (i == V255) {
				if (j == R)	/* k = 0 */
					dst[k] = (GET_BITS(src[0], 4, 5) << 8) | GET_BITS(src[1], 0, 7);
				else if (j == G)	/* k = 1 */
					dst[k] = (GET_BITS(src[0], 2, 3) << 8) | GET_BITS(src[2], 0, 7);
				else if (j == B)	/* k = 2 */
					dst[k] = (GET_BITS(src[0], 0, 1) << 8) | GET_BITS(src[3], 0, 7);
			} else if (i == V11) { /* V0 */
				if (j == R)  /* k = 30 */
					dst[k] |= GET_BITS(src[31], 4, 7);
				else if (j == G) /* k = 31 */
					dst[k] |= GET_BITS(src[31], 0, 3);
				else if (j == B) /* k = 32 */
					dst[k] |= GET_BITS(src[32], 4, 7);
			} else if (i == V12) { /* VT */
				if (j == R) /* k = 33 */
					dst[k] |= GET_BITS(src[32], 0, 3);
				else if (j == G) /* k = 34 */
					dst[k] |= GET_BITS(src[33], 4, 7);
				else if (j == B) /* k = 35 */
					dst[k] |= GET_BITS(src[33], 0, 3);
			} else if (i == V13) { /* k =36 */
				dst[k] = GET_BITS(src[k - 2], 0, 7);
			} else { /* V2 ~ V10, k = 3 ~ 29 */
				dst[k] = GET_BITS(src[k + 1], 0, 7);
			}

			k++;
		}
	}

	for (i = 0; i < GAMMA_SET_SIZE; i++)
		LCD_INFO("gamma src[%d] %02x", i, src[i]);
#if 0

	for (i = 0; i < GAMMA_V_SIZE; i++)
		LCD_INFO("gammaV dst[%d] %02x", i, dst[i]);
#endif

}

/*
 * convert_V_to_GAMMA_XA1 : convert V format to CAh gamma reg
 * src : extended V values (V255 ~ VT)
 * dst : packed gamma values (CAh)
 */
static void convert_V_to_GAMMA_XA1(int *src, u8 *dst)
{
	/* i : gamma index
	 * k : packed gamma index
	 */
	int i,k = 0;
	int dbg_org[GAMMA_V_SIZE];

	for (i = 0; i < GAMMA_V_SIZE; i++)
		dbg_org[i] = src[i];

	memset(dst, 0, GAMMA_SET_SIZE);

	/* prevent underflow */
	for (i = 0; i < GAMMA_V_SIZE; i++)
		src[i] = max(src[i], 0);

	/* prevent overflow */
	for (i = V255; i < V_MAX; i++) {
		int max;

		if (i == V255)
			max = BIT(10) - 1; /* 10 bits: V255 */
		else if (i == V11 || i == V12)
			max = BIT(4) - 1; /* 4 bits: V0, VT */
		else
			max = BIT(8) - 1; /* 8 bits: V2~V10, V13 */

		src[i * RGB_MAX + R] = min(src[i * RGB_MAX + R], max);
		src[i * RGB_MAX + G] = min(src[i * RGB_MAX + G], max);
		src[i * RGB_MAX + B] = min(src[i * RGB_MAX + B], max);
	}

	for (i = 0; i < GAMMA_V_SIZE; i++)
		if (dbg_org[i] != src[i])
			LCD_INFO("fix: src[%d] %02X -> %02X\n", i, dbg_org[i],  src[i]);

	for (i = 0; i < GAMMA_SET_SIZE; i++) {
		if (i == 0) {
			dst[i] = GET_BITS(src[k], 8, 9) << 4;
			dst[i] |= GET_BITS(src[k+1], 8, 9) << 2;
			dst[i] |= GET_BITS(src[k+2], 8, 9);
		} else if (i >= 31 && i <= 33) {
			dst[i] = GET_BITS(src[k++], 0, 4) << 4;
			dst[i] |= GET_BITS(src[k++], 0, 4);
		} else {
			dst[i] = GET_BITS(src[k++], 0, 7);
		}
	}
#if 0
	for (i = 0; i < GAMMA_V_SIZE; i++)
		LCD_INFO("gammaV src[%d] %02x\n", i, src[i]);
#endif
	for (i = 0; i < GAMMA_SET_SIZE; i++)
		LCD_INFO("gamma dst[%d] %02x\n", i, dst[i]);

	return;
}


static u8 *get_gamma(struct mtp_gm2_info *gm2_mtp,
		enum GAMMA_SET_VRR_MODES vrr_mode, enum GAMMA_SET_MODES gamma_mode, bool is_org)
{
	int offset = (vrr_mode * GAMMA_SET_MODE_MAX + gamma_mode) * GAMMA_SET_SIZE;

	if (is_org)
		return (gm2_mtp->gamma_org + offset);
	else
		return (gm2_mtp->gamma_comp + offset);
}

static int get_rgb_offset(enum GAMMA_SET_VRR_MODES vrr_mode, enum GAMMA_SET_MODES gamma_mode, int gamma_id)
{
		return rgb_offset_hs_revA[gamma_mode][gamma_id];
}

static int ss_gm2_gamma_comp_init(struct samsung_display_driver_data *vdd)
{
	struct mtp_gm2_info *gm2_mtp = &vdd->br_info.gm2_mtp;
	u8 *gamma_org;
	u8 *gamma_comp;
	u8 readbuf[GAMMA_SET_SIZE * GAMMA_SET_VRR_MAX * GAMMA_SET_MODE_MAX]; /* 444 */
	u8 *buf;
	int i_mode, k;
	int gammaV[GAMMA_V_SIZE];
	struct dsi_panel_cmd_set *rx_cmds;

	gm2_mtp->gamma_org = kzalloc(GAMMA_SET_SIZE * GAMMA_SET_MODE_MAX * GAMMA_SET_VRR_MAX, GFP_KERNEL);
	if (!gm2_mtp->gamma_org) {
		LCD_ERR("fail to allocate gamma_org\n");
		return -ENOMEM;
	}

	gm2_mtp->gamma_comp = kzalloc(GAMMA_SET_SIZE * GAMMA_SET_MODE_MAX * GAMMA_SET_VRR_MAX, GFP_KERNEL);
	if (!gm2_mtp->gamma_comp) {
		LCD_ERR("fail to allocate gamma_comp\n");
		kfree(gm2_mtp->gamma_org);
		return -ENOMEM;
	}

	memset(readbuf, 0, GAMMA_SET_SIZE * GAMMA_SET_VRR_MAX * GAMMA_SET_MODE_MAX);

	rx_cmds = ss_get_cmds(vdd, RX_SMART_DIM_MTP);
	if (SS_IS_CMDS_NULL(rx_cmds)) {
		LCD_ERR("No cmds for RX_SMART_DIM_MTP.. \n");
		return -ENODEV;
	}

	/* 1. read original gamma values for each VRR modes(HS/NS) and gammam modes(0~6) */
	/* C8h: 37 * 3 = 222 bytes
	 * 0x6F ~0x93: HS, MODE1
	 * 0x94 ~0xB8: HS, MODE2
	 * 0xB9 ~0xDD: HS, MODE3
	 */
	rx_cmds->cmds->msg.tx_buf[0] =  0xC8;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = GAMMA_SET_SIZE * 3; /* len */
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 0x6F; /* pos */
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, readbuf, LEVEL1_KEY);
	buf = readbuf;
	for (i_mode = GAMMA_SET_MODE1; i_mode < GAMMA_SET_MODE4; i_mode++) {
		gamma_org = get_gamma(gm2_mtp, GAMMA_SET_VRR_HS, i_mode, true);
		memcpy(gamma_org, buf, GAMMA_SET_SIZE);
		buf += GAMMA_SET_SIZE;
	}

	/* C7h
	 * 0x13 ~0x37: HS, MODE4
	 * 0x38 ~0x5C: HS, MODE5
	 * 0x5D ~0x81: HS, MODE6
	 * 0x82 ~0xA6: HS, MODE7
	 */
	rx_cmds->cmds->msg.tx_buf[0] =  0xC7;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = GAMMA_SET_SIZE * 4; /* len */
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 0x13; /* pos */
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, readbuf, LEVEL1_KEY);
	buf = readbuf;
	for (i_mode = GAMMA_SET_MODE4; i_mode < GAMMA_SET_MODE_MAX; i_mode++) {
		gamma_org = get_gamma(gm2_mtp, GAMMA_SET_VRR_HS, i_mode, true);
		memcpy(gamma_org, buf, GAMMA_SET_SIZE);
		buf += GAMMA_SET_SIZE;
	}

	/* 2. compensation gamma */
	for (i_mode = GAMMA_SET_MODE1; i_mode < GAMMA_SET_MODE_MAX; i_mode++) {
		LCD_INFO("gamma_org GAMMA_SET_MODE%d\n", i_mode+1);

		gamma_org = get_gamma(gm2_mtp, GAMMA_SET_VRR_HS, i_mode, true);
		convert_GAMMA_to_V_XA1(gamma_org, gammaV);

		for (k = 0; k < GAMMA_V_COMP_SIZE; k++)
			gammaV[k] += get_rgb_offset(GAMMA_SET_VRR_HS, i_mode, k);

		gamma_comp = get_gamma(gm2_mtp, GAMMA_SET_VRR_HS, i_mode, false);
		
		LCD_INFO("gamma_comp GAMMA_SET_MODE%d\n", i_mode+1);
		convert_V_to_GAMMA_XA1(gammaV, gamma_comp);
	}
	return 0;
}

enum GAMMA_COMP_CMD_ID {
	GAMMA_COMP_CMDID_C8 = 1,
	GAMMA_COMP_CMDID_C7 = 3,
};

struct dsi_panel_cmd_set * ss_brightness_gm2_gamma_comp(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds = NULL;
	struct mtp_gm2_info *gm2_mtp = &vdd->br_info.gm2_mtp;
	struct vrr_info *vrr = &vdd->vrr;
	u8 *gamma;
	u8 buf[GAMMA_SET_SIZE * GAMMA_SET_VRR_MAX * GAMMA_SET_MODE_MAX]; /* 444 */
	int pos;
	bool is_org;
	int j_mode;

	if (vrr->gm2_gamma != VRR_GM2_GAMMA_NO_ACTION) {
		LCD_INFO("gm2_gamma compensation(%d)\n", vrr->gm2_gamma);

		pcmds = ss_get_cmds(vdd, TX_VRR_GM2_GAMMA_COMP);

		if (vrr->gm2_gamma == VRR_GM2_GAMMA_COMPENSATE)
			is_org = false;
		else 	/* VRR_GM2_GAMMA_RESTORE_ORG */
			is_org = true;

		/* C8h: 37 * 3 = 111 bytes
		 * 0x6F ~0x93: HS, MODE1
		 * 0x94 ~0xB8: HS, MODE2
		 * 0xB9 ~0xDD: HS, MODE3
		 */
		pos = 0;
		for (j_mode = GAMMA_SET_MODE1; j_mode < GAMMA_SET_MODE4; j_mode++) {
			gamma = get_gamma(gm2_mtp, GAMMA_SET_VRR_HS, j_mode, is_org);
			memcpy(buf + pos, gamma, GAMMA_SET_SIZE);
			pos += GAMMA_SET_SIZE;
		}
		memcpy(&pcmds->cmds[GAMMA_COMP_CMDID_C8].msg.tx_buf[1], buf,
				pcmds->cmds[GAMMA_COMP_CMDID_C8].msg.tx_len - 1);

		/* C7h : 37 * 4 = 148 bytes
		 * 0x13 ~0x37: HS, MODE4
		 * 0x38 ~0x5C: HS, MODE5
		 * 0x5D ~0x81: HS, MODE6
		 * 0x82 ~0xA6: HS, MODE7
		 */
		pos = 0;
		for (j_mode = GAMMA_SET_MODE4; j_mode < GAMMA_SET_MODE_MAX; j_mode++) {
			gamma = get_gamma(gm2_mtp, GAMMA_SET_VRR_HS, j_mode, is_org);
			memcpy(buf + pos, gamma, GAMMA_SET_SIZE);
			pos += GAMMA_SET_SIZE;
		}
		memcpy(&pcmds->cmds[GAMMA_COMP_CMDID_C7].msg.tx_buf[1], buf,
				pcmds->cmds[GAMMA_COMP_CMDID_C7].msg.tx_len - 1);

		vrr->gm2_gamma = VRR_GM2_GAMMA_NO_ACTION;
	}

	return pcmds;
}

enum VRR_CMD_RR {
	/* 1Hz is PSR mode in LPM (AOD) mode, 10Hz is PSR mode in 120HS mode */
	VRR_48HS = 0,
	VRR_60HS,
	VRR_96HS,
	VRR_120HS,
	VRR_MAX
};

enum VRR_CMD_ID {
	VRR_CMDID_LFD_SET = 2,
	VRR_CMDID_OSC_SETTING = 3,
	VRR_CMDID_FREQ_SETTING = 4,
	VRR_CMDOFFSET_VFP_05 = 5,
	VRR_CMDID_VFP_05 = 6,
	VRR_CMDOFFSET_VFP_0C = 7,
	VRR_CMDID_VFP_0C = 8,
	VRR_CMDOFFSET_AOR = 9,
	VRR_CMDID_AOR = 10,
	VRR_CMDOFFSET_VBIAS = 11,
	VRR_CMDID_VBIAS = 12,
	VRR_CMDOFFSET_VAINT = 13,
	VRR_CMDID_VAINT = 14,
	VRR_CMDID_TE_MOD= 15,
};

enum VRR_FREQ_SETTING_INDEX {
	HS_120HZ = 0,
	HS_96HZ = 0,		
	HS_60HZ = 1,
	HS_48HZ = 1,
 };

u8 cmd_lfd_set[VRR_MAX][23] = {
	{0x01, 0x00, 0x00, 0x80, 0x00, 0x02, 0x40, 0x00,
		0x01, 0x70, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x14},/*VRR_48HS*/
	{0x01, 0x00, 0x00, 0x80, 0x00, 0x02, 0x40, 0x00,
		0x01, 0x70, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x14},/*VRR_60HS*/
	{0x01, 0x00, 0x00, 0x80, 0x00, 0x01, 0x40, 0x00,
		0x01, 0x70, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x14},/*VRR_96HS*/
	{0x01, 0x00, 0x00, 0x80, 0x00, 0x01, 0x40, 0x00,
		0x01, 0x70, 0x00, 0x00,	0x00, 0x0B, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x14},/*VRR_120HS*/
};

u8 cmd_aor_set[VRR_MAX][24] = {
	{0x08, 0xC4, 0x08, 0xC4, 0x08, 0xC4, 0x08, 0xC4,
		0x08, 0xC4, 0x08, 0xC4, 0x08, 0xC4, 0x08, 0xC4,
		0x08, 0xC4, 0x08, 0xC4, 0x08, 0x97, 0x01, 0x69},/*VRR_48HS*/
	{0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 
		0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00,
		0x07, 0x00, 0x07, 0x00, 0x06, 0xDC, 0x01, 0x20},/*VRR_60HS*/
	{0x08, 0xC4, 0x08, 0xC4, 0x08, 0xC4, 0x08, 0xC4,
		0x08, 0xC4, 0x08, 0xC4, 0x08, 0xC4, 0x08, 0xC4,
		0x08, 0xC4, 0x08, 0xC4, 0x08, 0x97, 0x01, 0x69},/*VRR_96HS*/
	{0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 
		0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00,
		0x07, 0x00, 0x07, 0x00, 0x06, 0xDC, 0x01, 0x20},/*VRR_120HS*/		
};


u8 cmd_vfp_change[VRR_MAX][2] = {
	{0x01, 0xCC},/*VRR_48HS*/
	{0x00, 0x08},/*VRR_60HS*/
	{0x01, 0xCC},/*VRR_96HS*/
	{0x00, 0x08},/*VRR_120HS*/
};

u8 cmd_hw_te_mod[VRR_MAX][1] = {
	{0x11}, // 48HS, org TE is 96hz -> 48hz
	{0x11}, // 60HS, org TE is 120hz -> 60hz
	{0x01}, // 96HS, org TE is 96hz
	{0x01}, // 120HS, org TE is 120hz
};

static enum VRR_CMD_RR ss_get_vrr_id(int cur_rr, int cur_hs)
{
	enum VRR_CMD_RR vrr_id;

	switch (cur_rr) {
	case 48:
		vrr_id = VRR_48HS ;
		break;
	case 60:
		vrr_id = VRR_60HS ;
		break;
	case 96:
		vrr_id = VRR_96HS;
		break;
	case 120:
		vrr_id = VRR_120HS;
		break;
	default:
		LCD_ERR("invalid refresh rate (%d, %d), set default 120HS..\n", cur_rr, cur_hs);
		vrr_id = VRR_120HS;
		break;
	}

	return vrr_id;
}

static int ss_update_base_lfd_val(struct vrr_info *vrr,
			enum LFD_SCOPE_ID scope, struct lfd_base_str *lfd_base)
{
	u32 base_rr, max_div_def, min_div_def, min_div_lowest;
	enum VRR_CMD_RR vrr_id;

	if (scope == LFD_SCOPE_LPM) {
		base_rr = 30;
		max_div_def = 1;
		min_div_def = 30;
		min_div_lowest = 30;
		goto done;
	}

	vrr_id = ss_get_vrr_id(vrr->cur_refresh_rate, vrr->cur_sot_hs_mode);

	switch (vrr_id) {
	case VRR_48HS:
		base_rr = 96;
		max_div_def = 2; // 48hz
		min_div_def = 8; // 12hz
		min_div_lowest = 96; // 1hz
		break;
	case VRR_60HS:
		base_rr = 120;
		max_div_def = 2; // 60hz
		min_div_def = 11; // 10.9hz
		min_div_lowest = 120; // 1hz
		break;
	case VRR_96HS:
		base_rr = 96;
		max_div_def = 1; // 96hz
		min_div_def = 8; // 12hz
		min_div_lowest = 96; // 1hz
		break;
	case VRR_120HS:
		base_rr = 120;
		max_div_def = 1; // 120hz
		min_div_def = 11; // 10.9hz
		min_div_lowest = 120; // 1hz
		break;
	default:
		LCD_ERR("invalid vrr_id\n");
		base_rr = 120;
		max_div_def = 1; // 120hz
		min_div_def = 11; // 10.9hz
		min_div_lowest = 120; // 1hz
		break;
	}

done:
	lfd_base->base_rr = base_rr;
	lfd_base->max_div_def = max_div_def;
	lfd_base->min_div_def = min_div_def;
	lfd_base->min_div_lowest = min_div_lowest;

	vrr->lfd.base_rr = base_rr;

	LCD_DEBUG("LFD(%s): base_rr: %uhz, def: %uhz(%u)~%uhz(%u), lowest: %uhz(%u)\n",
			lfd_scope_name[scope],
			base_rr,
			DIV_ROUND_UP(base_rr, min_div_def), min_div_def,
			DIV_ROUND_UP(base_rr, max_div_def), max_div_def,
			DIV_ROUND_UP(base_rr, min_div_lowest), min_div_lowest);

	return 0;
}

static struct dsi_panel_cmd_set *__ss_vrr(struct samsung_display_driver_data *vdd,
					int *level_key, bool is_hbm, bool is_hmt)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_panel_cmd_set  *vrr_cmds = ss_get_cmds(vdd, TX_VRR);

	struct vrr_info *vrr = &vdd->vrr;
	enum SS_BRR_MODE brr_mode = vrr->brr_mode;

	int cur_rr;
	bool cur_hs;
	enum VRR_CMD_RR vrr_id;
	enum SS_VBIAS_MODE mode;
	u8 *vbias = NULL;
	int vaint_offset = 7;
	int vaint_last_value = 8;
	int i = 0;
	u32 min_div, max_div;
	enum LFD_SCOPE_ID scope;

	if (SS_IS_CMDS_NULL(vrr_cmds)) {
		LCD_INFO("no vrr cmds\n");
		return NULL;
	}

	if (panel && panel->cur_mode) {
		LCD_INFO("VRR: cur_mode: %dx%d@%d%s, is_hbm: %d, is_hmt: %d, %s\n",
				panel->cur_mode->timing.h_active,
				panel->cur_mode->timing.v_active,
				panel->cur_mode->timing.refresh_rate,
				panel->cur_mode->timing.sot_hs_mode ? "HS" : "NM",
				is_hbm, is_hmt, ss_get_brr_mode_name(brr_mode));

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

	if (is_hmt && !(cur_rr == 60 && !cur_hs))
		LCD_ERR("error: HMT not in 60hz NM mode, cur: %dhz%s\n",
				cur_rr, cur_hs ? "HS" : "NM");

	vrr_id = ss_get_vrr_id(cur_rr, cur_hs);

	scope = is_hmt ? LFD_SCOPE_HMD : LFD_SCOPE_NORMAL ;
	if (ss_get_lfd_div(vdd, scope, &min_div, &max_div)) {
		LCD_ERR("fail to get LFD divider.. set default 1..\n");
		max_div = min_div = 1;
	}
	vrr->lfd.min_div = min_div;
	vrr->lfd.max_div = max_div;

	/* 1. LFD setting */
	memcpy(&vrr_cmds->cmds[VRR_CMDID_LFD_SET].msg.tx_buf[1],
			&cmd_lfd_set[vrr_id][0],
			vrr_cmds->cmds[VRR_CMDID_LFD_SET].msg.tx_len - 1);

	vrr_cmds->cmds[VRR_CMDID_LFD_SET].msg.tx_buf[14] = (min_div - 1);

	/* 2. VRR_OSC_SETTING VRR_FREQ_SETTING */
	switch (cur_rr) {
		case 120 :
		case 96 :
			vrr_cmds->cmds[VRR_CMDID_FREQ_SETTING].msg.tx_buf[2]= HS_120HZ;
			break;
		case 60 :
		case 48 :
			vrr_cmds->cmds[VRR_CMDID_FREQ_SETTING].msg.tx_buf[2]= HS_60HZ;
			break;
	}
	vrr_cmds->cmds[VRR_CMDID_FREQ_SETTING].msg.tx_buf[2] = max_div - 1;

	/* 3. VFP setting */
	memcpy(&vrr_cmds->cmds[VRR_CMDID_VFP_05].msg.tx_buf[1],
			&cmd_vfp_change[vrr_id][0],
			vrr_cmds->cmds[VRR_CMDID_VFP_05].msg.tx_len - 1);
	memcpy(&vrr_cmds->cmds[VRR_CMDID_VFP_0C].msg.tx_buf[1],
			&cmd_vfp_change[vrr_id][0],
			vrr_cmds->cmds[VRR_CMDID_VFP_0C].msg.tx_len - 1);

	/* 4. AOR setting */
	memcpy(&vrr_cmds->cmds[VRR_CMDID_AOR].msg.tx_buf[1],
			&cmd_aor_set[vrr_id][0],
			vrr_cmds->cmds[VRR_CMDID_AOR].msg.tx_len - 1);

	/* 5. Vbais & Vaint setting: 14bytes and last 1byte from flash */
	if (vdd->br_info.temperature > 0) {
		mode = (vdd->vrr.cur_sot_hs_mode) ? VBIAS_MODE_HS_WARM : VBIAS_MODE_NS_WARM;
	} else if (vdd->br_info.temperature > -15) {
		mode = (vdd->vrr.cur_sot_hs_mode) ? VBIAS_MODE_HS_COOL : VBIAS_MODE_NS_COOL;
	} else {
		mode = (vdd->vrr.cur_sot_hs_mode) ? VBIAS_MODE_HS_COLD : VBIAS_MODE_NS_COLD;
	}

	vbias = ss_get_vbias_data(&vdd->br_info.gm2_table, mode);

	/* ++ FOR DEBUGGING */
	LCD_DEBUG("vbias value for mode = %d start \n", mode);

	for (i = 0 ; i < 22 ; i++) {
		LCD_DEBUG("0x%x ", vbias[i]);
	}
	LCD_DEBUG("vbias value for mode = %d end \n", mode);
	/* -- FOR DEBUGGING */

	if (cur_hs == 1) {
		vrr_cmds->cmds[VRR_CMDOFFSET_VAINT].msg.tx_buf[1] = 0x47;
	}
	else {
		vrr_cmds->cmds[VRR_CMDOFFSET_VAINT].msg.tx_buf[1] = 0x39;
	}

	if (!vbias) {
		LCD_ERR("fail to get vbias..\n");
	} else {
		memcpy(&vrr_cmds->cmds[VRR_CMDID_VBIAS].msg.tx_buf[1],
				&vbias[0],
				vrr_cmds->cmds[VRR_CMDID_VBIAS].msg.tx_len - 1);
		memcpy(&vrr_cmds->cmds[VRR_CMDID_VAINT].msg.tx_buf[1],
				&vbias[vaint_offset],
				vrr_cmds->cmds[VRR_CMDID_VAINT].msg.tx_len - 1);
		/*from panel rev4 need one more vaint byte to be sent */
		if (vdd->panel_revision >= 3)
			vrr_cmds->cmds[VRR_CMDID_VAINT].msg.tx_buf[vaint_last_value] = vbias[21];
	}

	/* 6. TE modulation (B9h) */
	memcpy(&vrr_cmds->cmds[VRR_CMDID_TE_MOD].msg.tx_buf[1],
			&cmd_hw_te_mod[vrr_id][0],
			vrr_cmds->cmds[VRR_CMDID_TE_MOD].msg.tx_len - 1);

	if (lpcharge || vdd->is_recovery_mode)
		vrr_cmds->cmds[VRR_CMDID_TE_MOD].msg.tx_buf[1] = 0x01;


	LCD_INFO("VRR: (cur: %d%s, adj: %d%s, te_mod: (%d %d), LFD: %uhz(%u)~%uhz(%u)\n",
			cur_rr,
			cur_hs ? "HS" : "NM",
			vrr->adjusted_refresh_rate,
			vrr->adjusted_sot_hs_mode ? "HS" : "NM",
			vrr->te_mod_on, vrr->te_mod_divider,
			DIV_ROUND_UP(vrr->lfd.base_rr, min_div), min_div,
			DIV_ROUND_UP(vrr->lfd.base_rr, max_div), max_div);

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

/* TE moudulation (reports vsync as 60hz even TE is 120hz, in 60HS mode)
 * Some HOP display, like C2 XA1 UB, supports self scan function.
 * In this case, if it sets to 60HS mode, panel fetches pixel data from GRAM in 60hz,
 * but DDI generates TE as 120hz.
 * This architecture is for preventing brightness flicker in VRR change and keep fast touch responsness.
 *
 * In 60HS mode, TE period is 120hz, but display driver reports vsync to graphics HAL as 60hz.
 * So, do TE modulation in 60HS mode, reports vsync as 60hz.
 * In 30NS mode, TE is 60hz, but reports vsync as 30hz.
 */
static int ss_vrr_set_te_mode(struct samsung_display_driver_data *vdd, int cur_rr, int cur_hs)
{
	if (cur_hs) {
		if (cur_rr == 60 || cur_rr == 48) {
			vdd->vrr.te_mod_divider = 2; /* 120/60  or 96/48 */
			vdd->vrr.te_mod_cnt = vdd->vrr.te_mod_divider;
			vdd->vrr.te_mod_on = 1;
			LCD_INFO("%dHS: enable te_mode: div = %d\n", cur_rr, vdd->vrr.te_mod_divider);

			return 0;
		}
	}

	LCD_INFO("disable te_mod\n");
	vdd->vrr.te_mod_divider = 0;
	vdd->vrr.te_mod_cnt = vdd->vrr.te_mod_divider;
	vdd->vrr.te_mod_on = 0;

	return 0;
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
		ss_panel_data_read(vdd, RX_MANUFACTURE_DATE, date, LEVEL1_KEY);

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

	/* Read mtp (D6h 1~5th) for ddi id */
	if (ss_get_cmds(vdd, RX_DDI_ID)->count) {
		ss_panel_data_read(vdd, RX_DDI_ID, ddi_id, LEVEL1_KEY);

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
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_9 */
};

/* sRGB/Adobe RGB Mode */
static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* dummy */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_9 */
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

		LCD_INFO("DSI%d : X-%d Y-%d \n", vdd->ndx,
			vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);
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
	mdnie_data->DSI_COLOR_BLIND_MDNIE_1 = DSI_COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_1 = DSI_RGB_SENSOR_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_2 = DSI_RGB_SENSOR_MDNIE_2;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_3 = DSI_RGB_SENSOR_MDNIE_3;
	mdnie_data->DSI_TRANS_DIMMING_MDNIE = DSI_RGB_SENSOR_MDNIE_3;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE_2 = DSI_UI_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_UI_STANDARD_MDNIE_2 = DSI_UI_STANDARD_MDNIE_2;
	mdnie_data->DSI_UI_AUTO_MDNIE_2 = DSI_UI_AUTO_MDNIE_2;
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE_2 = DSI_VIDEO_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE_2 = DSI_VIDEO_STANDARD_MDNIE_2;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE_2 = DSI_VIDEO_AUTO_MDNIE_2;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE_2 = DSI_CAMERA_AUTO_MDNIE_2;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE_2 = DSI_GALLERY_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE_2 = DSI_GALLERY_STANDARD_MDNIE_2;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE_2 = DSI_GALLERY_AUTO_MDNIE_2;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE_2 = DSI_BROWSER_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE_2 = DSI_BROWSER_STANDARD_MDNIE_2;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE_2 = DSI_BROWSER_AUTO_MDNIE_2;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE_2 = DSI_EBOOK_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE_2 = DSI_EBOOK_STANDARD_MDNIE_2;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE_2 = DSI_EBOOK_AUTO_MDNIE_2;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE_2 = DSI_TDMB_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE_2 = DSI_TDMB_STANDARD_MDNIE_2;
	mdnie_data->DSI_TDMB_AUTO_MDNIE_2 = DSI_TDMB_AUTO_MDNIE_2;

	mdnie_data->DSI_BYPASS_MDNIE = DSI_BYPASS_MDNIE;
	mdnie_data->DSI_NEGATIVE_MDNIE = DSI_NEGATIVE_MDNIE;
	mdnie_data->DSI_COLOR_BLIND_MDNIE = DSI_COLOR_BLIND_MDNIE;
	mdnie_data->DSI_HBM_CE_MDNIE = DSI_HBM_CE_MDNIE;
	mdnie_data->DSI_HBM_CE_D65_MDNIE = DSI_HBM_CE_D65_MDNIE;
	mdnie_data->DSI_RGB_SENSOR_MDNIE = DSI_RGB_SENSOR_MDNIE;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE = DSI_UI_DYNAMIC_MDNIE;
	mdnie_data->DSI_UI_STANDARD_MDNIE = DSI_UI_STANDARD_MDNIE;
	mdnie_data->DSI_UI_NATURAL_MDNIE = DSI_UI_NATURAL_MDNIE;
	mdnie_data->DSI_UI_AUTO_MDNIE = DSI_UI_AUTO_MDNIE;
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE = DSI_VIDEO_DYNAMIC_MDNIE;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE = DSI_VIDEO_STANDARD_MDNIE;
	mdnie_data->DSI_VIDEO_NATURAL_MDNIE = DSI_VIDEO_NATURAL_MDNIE;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE = DSI_VIDEO_AUTO_MDNIE;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE = DSI_CAMERA_AUTO_MDNIE;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE = DSI_GALLERY_DYNAMIC_MDNIE;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE = DSI_GALLERY_STANDARD_MDNIE;
	mdnie_data->DSI_GALLERY_NATURAL_MDNIE = DSI_GALLERY_NATURAL_MDNIE;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE = DSI_GALLERY_AUTO_MDNIE;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE = DSI_BROWSER_DYNAMIC_MDNIE;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE = DSI_BROWSER_STANDARD_MDNIE;
	mdnie_data->DSI_BROWSER_NATURAL_MDNIE = DSI_BROWSER_NATURAL_MDNIE;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE = DSI_BROWSER_AUTO_MDNIE;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE = DSI_EBOOK_DYNAMIC_MDNIE;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE = DSI_EBOOK_STANDARD_MDNIE;
	mdnie_data->DSI_EBOOK_NATURAL_MDNIE = DSI_EBOOK_NATURAL_MDNIE;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE = DSI_EBOOK_AUTO_MDNIE;
	mdnie_data->DSI_EMAIL_AUTO_MDNIE = DSI_EMAIL_AUTO_MDNIE;
	mdnie_data->DSI_GAME_LOW_MDNIE = DSI_GAME_LOW_MDNIE;
	mdnie_data->DSI_GAME_MID_MDNIE = DSI_GAME_MID_MDNIE;
	mdnie_data->DSI_GAME_HIGH_MDNIE = DSI_GAME_HIGH_MDNIE;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE = DSI_TDMB_DYNAMIC_MDNIE;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE = DSI_TDMB_STANDARD_MDNIE;
	mdnie_data->DSI_TDMB_NATURAL_MDNIE = DSI_TDMB_NATURAL_MDNIE;
	mdnie_data->DSI_TDMB_AUTO_MDNIE = DSI_TDMB_AUTO_MDNIE;
	mdnie_data->DSI_GRAYSCALE_MDNIE = DSI_GRAYSCALE_MDNIE;
	mdnie_data->DSI_GRAYSCALE_NEGATIVE_MDNIE = DSI_GRAYSCALE_NEGATIVE_MDNIE;
	mdnie_data->DSI_CURTAIN = DSI_SCREEN_CURTAIN_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE = DSI_NIGHT_MODE_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR = DSI_NIGHT_MODE_MDNIE_1;
	mdnie_data->DSI_COLOR_LENS_MDNIE = DSI_COLOR_LENS_MDNIE;
	mdnie_data->DSI_COLOR_LENS_MDNIE_SCR = DSI_COLOR_LENS_MDNIE_1;
	mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR = DSI_COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR = DSI_RGB_SENSOR_MDNIE_1;
	mdnie_data->DSI_AFC = DSI_AFC;
	mdnie_data->DSI_AFC_ON = DSI_AFC_ON;
	mdnie_data->DSI_AFC_OFF = DSI_AFC_OFF;

	mdnie_data->mdnie_tune_value_dsi = mdnie_tune_value_dsi;
	mdnie_data->hmt_color_temperature_tune_value_dsi = hmt_color_temperature_tune_value_dsi;

	mdnie_data->hdr_tune_value_dsi = hdr_tune_value_dsi;

	mdnie_data->light_notification_tune_value_dsi = light_notification_tune_value_dsi;

	/* Update MDNIE data related with size, offset or index */
	mdnie_data->dsi_bypass_mdnie_size = ARRAY_SIZE(DSI_BYPASS_MDNIE);
	mdnie_data->mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
	mdnie_data->mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP3] = MDNIE_STEP3_INDEX;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = ADDRESS_SCR_WHITE_RED;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = ADDRESS_SCR_WHITE_GREEN;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = ADDRESS_SCR_WHITE_BLUE;
	mdnie_data->dsi_rgb_sensor_mdnie_1_size = DSI_RGB_SENSOR_MDNIE_1_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_2_size = DSI_RGB_SENSOR_MDNIE_2_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_3_size = DSI_RGB_SENSOR_MDNIE_3_SIZE;

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
	mdnie_data->dsi_afc_size = 71;
	mdnie_data->dsi_afc_index = 56;

	vdd->mdnie.mdnie_data = mdnie_data;

	return 0;
}

static int ss_gct_read(struct samsung_display_driver_data *vdd)
{
	u8 valid_checksum[4] = {0xDF, 0x5F, 0xDF, 0x5F};
	int res;

	if (!vdd->gct.is_support)
		return GCT_RES_CHECKSUM_NOT_SUPPORT;

	if (!vdd->gct.on)
		return GCT_RES_CHECKSUM_OFF;


	if (!memcmp(vdd->gct.checksum, valid_checksum, 4))
		res = GCT_RES_CHECKSUM_PASS;
	else
		res = GCT_RES_CHECKSUM_NG;

	return res;
}

static int ss_gct_write(struct samsung_display_driver_data *vdd)
{
	u8 *checksum;
	int i;
	/* vddm set, 0x0: 1.0V, 0x04: 0.9V LV, 0x08: 1.1V HV */
	u8 vddm_set[MAX_VDDM] = {0x0, 0x04, 0x08};
	int ret = 0;
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	LCD_INFO("+\n");

	/* prevent sw reset to trigger esd recovery */
	LCD_INFO("disable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd);

	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	for (i = TX_GCT_ENTER; i <= TX_GCT_EXIT; i++)
		ss_set_exclusive_tx_packet(vdd, i, 1);
	ss_set_exclusive_tx_packet(vdd, RX_GCT_CHECKSUM, 1);
	ss_set_exclusive_tx_packet(vdd, TX_REG_READ_POS, 1);

	usleep_range(10000, 11000);

	checksum = vdd->gct.checksum;
	for (i = VDDM_LV; i < MAX_VDDM; i++) {
		struct dsi_panel_cmd_set *set;

		LCD_INFO("(%d) TX_GCT_ENTER\n", i);
		/* VDDM LV set (0x0: 1.0V, 0x10: 0.9V, 0x30: 1.1V) */
		set = ss_get_cmds(vdd, TX_GCT_ENTER);
		if (SS_IS_CMDS_NULL(set)) {
			LCD_ERR("No cmds for TX_GCT_ENTER..\n");
			break;
		}
		set->cmds[13].msg.tx_buf[1] = vddm_set[i];
		ss_send_cmd(vdd, TX_GCT_ENTER);

		msleep(150);

		ss_panel_data_read(vdd, RX_GCT_CHECKSUM, checksum++,
				LEVEL_KEY_NONE);
		LCD_INFO("(%d) read checksum: %x\n", i, *(checksum - 1));

		LCD_INFO("(%d) TX_GCT_MID\n", i);
		ss_send_cmd(vdd, TX_GCT_MID);

		msleep(150);

		ss_panel_data_read(vdd, RX_GCT_CHECKSUM, checksum++,
				LEVEL_KEY_NONE);

		LCD_INFO("(%d) read checksum: %x\n", i, *(checksum - 1));

		LCD_INFO("(%d) TX_GCT_EXIT\n", i);
		ss_send_cmd(vdd, TX_GCT_EXIT);
	}

	vdd->gct.on = 1;

	LCD_INFO("checksum = {%x %x %x %x}\n",
			vdd->gct.checksum[0], vdd->gct.checksum[1],
			vdd->gct.checksum[2], vdd->gct.checksum[3]);

	/* exit exclusive mode*/
	for (i = TX_GCT_ENTER; i <= TX_GCT_EXIT; i++)
		ss_set_exclusive_tx_packet(vdd, i, 0);
	ss_set_exclusive_tx_packet(vdd, RX_GCT_CHECKSUM, 0);
	ss_set_exclusive_tx_packet(vdd, TX_REG_READ_POS, 0);

	/* Reset panel to enter nornal panel mode.
	 * The on commands also should be sent before update new frame.
	 * Next new frame update is blocked by exclusive_tx.enable
	 * in ss_event_frame_update_pre(), and it will be released
	 * by wake_up exclusive_tx.ex_tx_waitq.
	 * So, on commands should be sent before wake up the waitq
	 * and set exclusive_tx.enable to false.
	 */
	ss_set_exclusive_tx_packet(vdd, DSI_CMD_SET_OFF, 1);
	ss_send_cmd(vdd, DSI_CMD_SET_OFF);

	vdd->panel_state = PANEL_PWR_OFF;
	dsi_panel_power_off(panel);
	dsi_panel_power_on(panel);
	vdd->panel_state = PANEL_PWR_ON_READY;

	ss_set_exclusive_tx_packet(vdd, DSI_CMD_SET_ON, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL0_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, DSI_CMD_SET_PPS, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL0_KEY_DISABLE, 1);

	ss_send_cmd(vdd, DSI_CMD_SET_ON);
	dsi_panel_update_pps(panel);

	ss_set_exclusive_tx_packet(vdd, DSI_CMD_SET_OFF, 0);
	ss_set_exclusive_tx_packet(vdd, DSI_CMD_SET_ON, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL0_KEY_ENABLE, 0);
	ss_set_exclusive_tx_packet(vdd, DSI_CMD_SET_PPS, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL0_KEY_DISABLE, 0);

	vdd->exclusive_tx.enable = 0;
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);

	vdd->mafpc.force_delay = true;
	ss_panel_on_post(vdd);

	/* enable esd interrupt */
	LCD_INFO("enable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd);

	return ret;
}

static int ss_self_display_data_init(struct samsung_display_driver_data *vdd)
{
	u32 panel_type = 0;
	u32 panel_color = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR("Self Display is not supported\n");
		return -EINVAL;
	}

	LCD_INFO("Self Display Panel Data init\n");

	panel_type = (ss_panel_id0_get(vdd) & 0x30) >> 4;
	panel_color = ss_panel_id0_get(vdd) & 0xF;

	LCD_INFO("Panel Type=0x%x, Panel Color=0x%x\n", panel_type, panel_color);

	vdd->self_disp.operation[FLAG_SELF_MASK].img_buf = self_mask_img_data;
	vdd->self_disp.operation[FLAG_SELF_MASK].img_size = ARRAY_SIZE(self_mask_img_data);
	vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum = SELF_MASK_IMG_CHECKSUM;
	make_mass_self_display_img_cmds_XA1(vdd, TX_SELF_MASK_IMAGE, FLAG_SELF_MASK);

	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_buf = self_mask_img_crc_data;
	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_size = ARRAY_SIZE(self_mask_img_crc_data);
	make_mass_self_display_img_cmds_XA1(vdd, TX_SELF_MASK_IMAGE_CRC, FLAG_SELF_MASK_CRC);
	vdd->self_disp.operation[FLAG_SELF_ICON].img_buf = self_icon_img_data;
	vdd->self_disp.operation[FLAG_SELF_ICON].img_size = ARRAY_SIZE(self_icon_img_data);
	make_mass_self_display_img_cmds_XA1(vdd, TX_SELF_ICON_IMAGE, FLAG_SELF_ICON);

	vdd->self_disp.operation[FLAG_SELF_ACLK].img_buf = self_aclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_ACLK].img_size = ARRAY_SIZE(self_aclock_img_data);
	make_mass_self_display_img_cmds_XA1(vdd, TX_SELF_ACLOCK_IMAGE, FLAG_SELF_ACLK);

	vdd->self_disp.operation[FLAG_SELF_DCLK].img_buf = self_dclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_DCLK].img_size = ARRAY_SIZE(self_dclock_img_data);
	make_mass_self_display_img_cmds_XA1(vdd, TX_SELF_DCLOCK_IMAGE, FLAG_SELF_DCLK);

	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_buf = self_video_img_data;
	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_size = ARRAY_SIZE(self_video_img_data);
	make_mass_self_display_img_cmds_XA1(vdd, TX_SELF_VIDEO_IMAGE, FLAG_SELF_VIDEO);
	return 1;
}

static int ss_mafpc_data_init(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->mafpc.is_support) {
		LCD_ERR("mAFPC is not supported\n");
		return -EINVAL;
	}

	LCD_INFO("mAFPC Panel Data init\n");

	vdd->mafpc.img_buf = mafpc_img_data;
	vdd->mafpc.img_size = ARRAY_SIZE(mafpc_img_data);

	if (vdd->mafpc.make_img_mass_cmds)
		vdd->mafpc.make_img_mass_cmds(vdd, vdd->mafpc.img_buf, vdd->mafpc.img_size, TX_MAFPC_IMAGE); /* Image Data */
	else if (vdd->mafpc.make_img_cmds)
		vdd->mafpc.make_img_cmds(vdd, vdd->mafpc.img_buf, vdd->mafpc.img_size, TX_MAFPC_IMAGE); /* Image Data */
	else {
		LCD_ERR("Can not make mafpc image commands\n");
		return -EINVAL;
	}

	/* CRC Check For Factory Mode */
	vdd->mafpc.crc_img_buf = mafpc_img_data_crc_check;
	vdd->mafpc.crc_img_size = ARRAY_SIZE(mafpc_img_data_crc_check);

	if (vdd->mafpc.make_img_mass_cmds)
		vdd->mafpc.make_img_mass_cmds(vdd, vdd->mafpc.crc_img_buf, vdd->mafpc.crc_img_size, TX_MAFPC_CRC_CHECK_IMAGE); /* CRC Check Image Data */
	else if (vdd->mafpc.make_img_cmds)
		vdd->mafpc.make_img_cmds(vdd, vdd->mafpc.crc_img_buf, vdd->mafpc.crc_img_size, TX_MAFPC_CRC_CHECK_IMAGE); /* CRC Check Image Data */
	else {
		LCD_ERR("Can not make mafpc image commands\n");
		return -EINVAL;
	}

	return true;
}

static void ss_copr_panel_init(struct samsung_display_driver_data *vdd)
{
	vdd->copr.ver = COPR_VER_5P0;
	vdd->copr.display_read = 0;
	ss_copr_init(vdd);
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

		ss_panel_data_read(vdd, RX_CELL_ID, cell_id_buffer, LEVEL1_KEY);

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
				vdd->octa_id_dsi, LEVEL1_KEY);

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

	*level_key = LEVEL1_KEY;

	pcmds = ss_get_cmds(vdd, TX_ACL_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_ACL_ON..\n");
		return NULL;
	}

	pcmds->cmds[0].msg.tx_buf[3] = 0x80;	/* start point 60% 80 65 */
	pcmds->cmds[0].msg.tx_buf[4] = 0X65;
	pcmds->cmds[0].msg.tx_buf[6] = 0X26;	/* ACL 8% */

	LCD_INFO("HBM: gradual_acl: %d, start point 0x%x0x%x acl per: 0x%x",
			vdd->br_info.gradual_acl_val, pcmds->cmds[0].msg.tx_buf[3], pcmds->cmds[0].msg.tx_buf[4], pcmds->cmds[0].msg.tx_buf[6]);

	return pcmds;
}

static struct dsi_panel_cmd_set *ss_acl_on(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL1_KEY;

	pcmds = ss_get_cmds(vdd, TX_ACL_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_ACL_ON..\n");
		return NULL;
	}

	pcmds->cmds[0].msg.tx_buf[3] = 0x40;	/* start point 50% 40 FF */
	pcmds->cmds[0].msg.tx_buf[4] = 0XFF;
	pcmds->cmds[0].msg.tx_buf[6] = 0X50;	/* ACL 15% */

	LCD_INFO("gradual_acl: %d, start point 0x%x0x%x acl per: 0x%x",
			vdd->br_info.gradual_acl_val, pcmds->cmds[0].msg.tx_buf[3], pcmds->cmds[0].msg.tx_buf[4], pcmds->cmds[0].msg.tx_buf[6]);

	return pcmds;
}

static struct dsi_panel_cmd_set *ss_acl_off(struct samsung_display_driver_data *vdd, int *level_key)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL1_KEY;
	LCD_INFO("off\n");
	return ss_get_cmds(vdd, TX_ACL_OFF);
}

static struct dsi_panel_cmd_set *ss_vint(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *vint_cmds = ss_get_cmds(vdd, TX_VINT);

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(vint_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)vint_cmds);
		return NULL;
	}

	if (vdd->xtalk_mode) {
		vint_cmds->cmds[2].msg.tx_buf[1] = 0x06;	/* VGH 6.2 V */
		LCD_INFO("xtalk_mode on\n");
	}
	else {
		vint_cmds->cmds[2].msg.tx_buf[1] = 0x10;	/* VGH return V */
	}

	return vint_cmds;
}

enum LPMBL_CMD_ID {
	LPMBL_CMDID_CTRL = 1,
	LPMBL_CMDID_LFD = 2,
	LPMBL_CMDID_LPM_ON = 3,
};
static void ss_set_panel_lpm_brightness(struct samsung_display_driver_data *vdd)
{
	struct vrr_info *vrr = &vdd->vrr;
	struct dsi_panel_cmd_set *set_lpm = ss_get_cmds(vdd, TX_LPM_BL_CMD);
	struct dsi_panel_cmd_set *set_lpm_bl;
	u32 min_div, max_div;
	static bool is_first = true;
	static char BB_9TH[1];

	if (is_first || unlikely(vdd->is_factory_mode)) {
		ss_panel_data_read(vdd, RX_ALPM_SET_VALUE, BB_9TH, LEVEL1_KEY);
		LCD_INFO("is_first BB_9TH = 0x%x\n", (int)BB_9TH[0]);

		is_first = false;
	}

	if (SS_IS_CMDS_NULL(set_lpm)) {
		LCD_ERR("No cmds for TX_LPM_BL_CMD\n");
		return;
	}

	/* LPM_ON: 3. HLPM brightness */
	switch (vdd->panel_lpm.lpm_bl_level) {
	case LPM_60NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_HLPM_60NIT_CMD);
		set_lpm->cmds[LPMBL_CMDID_LPM_ON].msg.tx_buf[1]= 0x22;
		break;
	case LPM_30NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_HLPM_30NIT_CMD);
		set_lpm->cmds[LPMBL_CMDID_LPM_ON].msg.tx_buf[1]= 0x22;
		break;
	case LPM_10NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_HLPM_10NIT_CMD);
		set_lpm->cmds[LPMBL_CMDID_LPM_ON].msg.tx_buf[1]= 0x22;
		break;
	case LPM_2NIT:
	default:
		set_lpm_bl = ss_get_cmds(vdd, TX_HLPM_2NIT_CMD);
		set_lpm->cmds[LPMBL_CMDID_LPM_ON].msg.tx_buf[1]= 0x23;
		break;
	}

	if (SS_IS_CMDS_NULL(set_lpm_bl)) {
		LCD_ERR("No cmds for alpm_ctrl..\n");
		return;
	}

	memcpy(&set_lpm->cmds[LPMBL_CMDID_CTRL].msg.tx_buf[1],
			&set_lpm_bl->cmds->msg.tx_buf[1],
			sizeof(char) * (set_lpm->cmds[LPMBL_CMDID_CTRL].msg.tx_len - 1));

	set_lpm->cmds[LPMBL_CMDID_CTRL].msg.tx_buf[9] = 
		(vdd->panel_lpm.lpm_bl_level > LPM_2NIT) ? (((int)BB_9TH[0] & 0x3F) | 0x40) : (((int)BB_9TH[0] & 0x3F) | 0x80);

	/* 2. LFD min freq.=1hz and frame insertion=30hz-16frame */
	ss_get_lfd_div(vdd, LFD_SCOPE_LPM, &min_div, &max_div);
	vrr->lfd.min_div = min_div;
	vrr->lfd.max_div = 1;
	if (max_div != 1)
		LCD_ERR("LPM max_div support only 1 (req: %d)\n", max_div);

	set_lpm->cmds[LPMBL_CMDID_LFD].msg.tx_buf[12] = min_div - 1;

	/* send lpm bl cmd */
	ss_send_cmd(vdd, TX_LPM_BL_CMD);

	LCD_INFO("[Panel LPM] bl_level : %s, LFD: div cmd: 0x%x, %dhz-%dhz (%d-%d)\n",
			/* Check current brightness level */
			vdd->panel_lpm.lpm_bl_level == LPM_2NIT ? "2NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_10NIT ? "10NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_30NIT ? "30NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_60NIT ? "60NIT" : "UNKNOWN",
			set_lpm->cmds[LPMBL_CMDID_LFD].msg.tx_buf[12],
			DIV_ROUND_UP(vrr->lfd.base_rr, min_div),
			DIV_ROUND_UP(vrr->lfd.base_rr, max_div),
			min_div, max_div);
}

enum LPMON_CMD_ID {
	LPMON_CMDID_CTRL = 1,
	LPMON_CMDID_LFD = 2,
	LPMON_CMDID_LPM_ON = 5,
};
/*
 * This function will update parameters for ALPM_REG/ALPM_CTRL_REG
 * Parameter for ALPM_REG : Control brightness for panel LPM
 * Parameter for ALPM_CTRL_REG : Change panel LPM mode for ALPM/HLPM
 * mode, brightness, hz are updated here.
 */
static void ss_update_panel_lpm_ctrl_cmd(struct samsung_display_driver_data *vdd)
{
	struct vrr_info *vrr = &vdd->vrr;
	struct dsi_panel_cmd_set *set_lpm_on = ss_get_cmds(vdd, TX_LPM_ON);
	struct dsi_panel_cmd_set *set_lpm_bl;
	u32 min_div, max_div;
	static bool is_first = true;
	static char BB_9TH[1];

	if (is_first || unlikely(vdd->is_factory_mode)) {
		ss_panel_data_read(vdd, RX_ALPM_SET_VALUE, BB_9TH, LEVEL1_KEY);
		LCD_INFO("is_first BB_9TH = 0x%x\n", (int)BB_9TH[0]);

		is_first = false;
	}

	LCD_INFO("%s++\n", __func__);

	if (SS_IS_CMDS_NULL(set_lpm_on)) {
		LCD_ERR("No cmds for TX_LPM_ON/OFF\n");
		return;
	}

	/* LPM_ON: 3. HLPM brightness */
	switch (vdd->panel_lpm.lpm_bl_level) {
	case LPM_60NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_HLPM_60NIT_CMD);
		set_lpm_on->cmds[LPMON_CMDID_LPM_ON].msg.tx_buf[1]= 0x22;
		break;
	case LPM_30NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_HLPM_30NIT_CMD);
		set_lpm_on->cmds[LPMON_CMDID_LPM_ON].msg.tx_buf[1]= 0x22;
		break;
	case LPM_10NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_HLPM_10NIT_CMD);
		set_lpm_on->cmds[LPMON_CMDID_LPM_ON].msg.tx_buf[1]= 0x22;
		break;
	case LPM_2NIT:
	default:
		set_lpm_bl = ss_get_cmds(vdd, TX_HLPM_2NIT_CMD);
		set_lpm_on->cmds[LPMON_CMDID_LPM_ON].msg.tx_buf[1]= 0x23;
		break;
	}

	if (SS_IS_CMDS_NULL(set_lpm_bl)) {
		LCD_ERR("No cmds for alpm_ctrl..\n");
		return;
	}

	memcpy(&set_lpm_on->cmds[LPMON_CMDID_CTRL].msg.tx_buf[1],
			&set_lpm_bl->cmds->msg.tx_buf[1],
			sizeof(char) * set_lpm_on->cmds[LPMON_CMDID_CTRL].msg.tx_len - 1);

	set_lpm_on->cmds[LPMON_CMDID_CTRL].msg.tx_buf[9] = 
		(vdd->panel_lpm.lpm_bl_level > LPM_2NIT) ? (((int)BB_9TH[0] & 0x3F) | 0x40) : (((int)BB_9TH[0] & 0x3F) | 0x80);

	/* LFD 30fps at image AOD */
	ss_get_lfd_div(vdd, LFD_SCOPE_LPM, &min_div, &max_div);
	vrr->lfd.min_div = min_div;
	vrr->lfd.max_div = 1;
	if (max_div != 1)
		LCD_ERR("LPM max_div support only 1 (req: %d)\n", max_div);

	set_lpm_on->cmds[LPMON_CMDID_LFD].msg.tx_buf[12] = min_div - 1;

	LCD_INFO("%s--\n", __func__);
}

static int ss_brightness_tot_level(struct samsung_display_driver_data *vdd)
{
	int tot_level_key = 0;

	/* HAB brightness setting requires F0h and FCh level key unlocked */
	/* DBV setting require TEST KEY3 (FCh) */
	tot_level_key = LEVEL1_KEY | LEVEL2_KEY;

	return tot_level_key;
}

static int samsung_panel_off_pre(struct samsung_display_driver_data *vdd)
{
	int rc = 0;
	return rc;
}

static int samsung_panel_off_post(struct samsung_display_driver_data *vdd)
{
	int rc = 0;
	return rc;
}

static int ss_vrr_init(struct vrr_info *vrr)
{
	struct lfd_mngr *mngr;
	int i, scope;

	LCD_INFO("+++\n");

	mutex_init(&vrr->vrr_lock);
	mutex_init(&vrr->brr_lock);

	vrr->running_vrr_mdp = false;
	vrr->running_vrr = false;

	/* Bootloader: 60HS */
	vrr->cur_refresh_rate = vrr->adjusted_refresh_rate = 120;
	vrr->cur_sot_hs_mode = vrr->adjusted_sot_hs_mode = true;

	vrr->hs_nm_seq = HS_NM_OFF;
	vrr->delayed_perf_normal = false;
	vrr->skip_vrr_in_brightness = false;

	vrr->brr_mode = BRR_OFF_MODE;
	vrr->brr_rewind_on = false;

	vrr->vrr_workqueue = create_singlethread_workqueue("vrr_workqueue");
	INIT_WORK(&vrr->vrr_work, ss_panel_vrr_switch_work);

	/* LFD mode */
	for (i = 0, mngr = &vrr->lfd.lfd_mngr[i]; i < LFD_CLIENT_MAX; i++, mngr++) {
		for (scope = 0; scope < LFD_SCOPE_MAX; scope++) {
			mngr->fix[scope] = LFD_FUNC_FIX_OFF;
			mngr->scalability[scope] = LFD_FUNC_SCALABILITY0;
			mngr->min[scope] = LFD_FUNC_MIN_CLEAR;
			mngr->max[scope] = LFD_FUNC_MAX_CLEAR;
		}
	}

#if defined(CONFIG_SEC_FACTORY)
	mngr = &vrr->lfd.lfd_mngr[LFD_CLIENT_FAC];
	mngr->fix[LFD_SCOPE_NORMAL] = LFD_FUNC_FIX_HIGH;
	mngr->fix[LFD_SCOPE_LPM] = LFD_FUNC_FIX_HIGH;
	mngr->fix[LFD_SCOPE_HMD] = LFD_FUNC_FIX_HIGH;
#endif

	/* TE modulation */
	vrr->te_mod_on = 0;
	vrr->te_mod_divider = 0;
	vrr->te_mod_cnt = 0;

	LCD_INFO("---\n");
	return 0;
}
static void ss_grayspot_etc(struct samsung_display_driver_data *vdd, int enable)
{
	struct dsi_panel_cmd_set *pcmds;
	static bool is_first = true;
	static char B5_75TH[1];

	if (is_first || unlikely(vdd->is_factory_mode)) {
		ss_panel_data_read(vdd, RX_GRAYSPOT_RESTORE_VALUE, B5_75TH, LEVEL1_KEY);
		is_first = false;
	}

	if(!enable)
	{
		pcmds = ss_get_cmds(vdd, TX_GRAY_SPOT_TEST_OFF);
		pcmds->cmds[8].msg.tx_buf[1] = B5_75TH[0];
	}
	return;
}

static bool ss_check_support_mode(struct samsung_display_driver_data *vdd, enum CHECK_SUPPORT_MODE mode)
{
	bool is_support = true;
	int cur_rr = vdd->vrr.cur_refresh_rate;
	bool cur_hs = vdd->vrr.cur_sot_hs_mode;

	switch (mode) {

	case CHECK_SUPPORT_BRIGHTDOT:
		if (!(cur_rr == 120 && cur_hs)) {
			is_support = false;
			LCD_ERR("BRIGHT DOT fail: supported on 120HS(cur: %d%s)\n",
					cur_rr, cur_hs ? "HS" : "NS");
		}
		break;

	default:
		break;
	}

	return is_support;
}

static void samsung_panel_init(struct samsung_display_driver_data *vdd)
{
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
	vdd->panel_func.samsung_manufacture_date_read = ss_manufacture_date_read;
	vdd->panel_func.samsung_ddi_id_read = ss_ddi_id_read;
	vdd->panel_func.samsung_smart_dimming_init = NULL;
	vdd->panel_func.samsung_cell_id_read = ss_cell_id_read;
	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;
	vdd->panel_func.samsung_elvss_read = NULL;
	vdd->panel_func.samsung_mdnie_read = ss_mdnie_read;

	/* Brightness */
	vdd->panel_func.samsung_brightness_gamma = ss_brightness_gamma_mode2_normal;
	vdd->panel_func.samsung_brightness_acl_on = ss_acl_on;
	vdd->panel_func.samsung_brightness_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_hbm_off = NULL;
	vdd->panel_func.samsung_brightness_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_elvss = NULL;
	vdd->panel_func.samsung_brightness_elvss_temperature1 = NULL;
	vdd->panel_func.samsung_brightness_elvss_temperature2 = NULL;
	vdd->panel_func.samsung_brightness_vint = ss_vint;
	vdd->panel_func.samsung_brightness_vrr = ss_vrr;
	vdd->br_info.smart_dimming_loaded_dsi = false;
	vdd->panel_func.samsung_brightness_gm2_gamma_comp = ss_brightness_gm2_gamma_comp;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_brightness_gamma_mode2_hbm;
	vdd->panel_func.samsung_hbm_acl_on = ss_acl_on_hbm;
	vdd->panel_func.samsung_hbm_acl_off = ss_acl_off;
	vdd->panel_func.samsung_hbm_etc = NULL;
	vdd->panel_func.samsung_hbm_irc = NULL;
	vdd->panel_func.get_hbm_candela_value = NULL;
	vdd->panel_func.samsung_brightness_vrr_hbm = ss_vrr_hbm;

	/* Total level key in brightness */
	vdd->panel_func.samsung_brightness_tot_level = ss_brightness_tot_level;

	/* Event */
	vdd->panel_func.samsung_change_ldi_fps = NULL;

	/* Panel LPM */
	vdd->panel_func.samsung_update_lpm_ctrl_cmd = ss_update_panel_lpm_ctrl_cmd;
	vdd->panel_func.samsung_set_lpm_brightness = ss_set_panel_lpm_brightness;

	/* Grayspot test */
	vdd->panel_func.samsung_gray_spot = ss_grayspot_etc;

	/* default brightness */
	vdd->br_info.common_br.bl_level = 255;

	/* mdnie */
	vdd->mdnie.support_mdnie = true;

	vdd->mdnie.support_trans_dimming = true;
	vdd->mdnie.mdnie_tune_size[0] = sizeof(DSI_BYPASS_MDNIE_1);
	vdd->mdnie.mdnie_tune_size[1] = sizeof(DSI_BYPASS_MDNIE_2);
	vdd->mdnie.mdnie_tune_size[2] = sizeof(DSI_BYPASS_MDNIE_3);

	dsi_update_mdnie_data(vdd);

	/* send recovery pck before sending image date (for ESD recovery) */
	vdd->esd_recovery.send_esd_recovery = false;

	/* Enable panic on first pingpong timeout */
	//vdd->debug_data->panic_on_pptimeout = true;
	/* Set IRC init value */
	vdd->br_info.common_br.irc_mode = IRC_MODERATO_MODE;

	/* COLOR WEAKNESS */
	vdd->panel_func.color_weakness_ccb_on_off =  NULL;

	/* Support DDI HW CURSOR */
	vdd->panel_func.ddi_hw_cursor = NULL;

	/* COVER Open/Close */
	vdd->panel_func.samsung_cover_control = NULL;

	/* COPR */
	vdd->copr.panel_init = ss_copr_panel_init;

	/* ACL default ON */
	vdd->br_info.acl_status = 1;
	vdd->br_info.temperature = 20; // default temperature

	/* ACL default status in acl on */
	vdd->br_info.gradual_acl_val = 1;

	/* Gram Checksum Test */
	vdd->panel_func.samsung_gct_write = ss_gct_write;
	vdd->panel_func.samsung_gct_read = ss_gct_read;

	/* Self display */
	vdd->self_disp.is_support = true;
	vdd->self_disp.factory_support = true;
	vdd->self_disp.init = self_display_init_XA1;
	vdd->self_disp.data_init = ss_self_display_data_init;

	/* mAPFC */
	vdd->mafpc.init = ss_mafpc_init_XA1;
	vdd->mafpc.data_init = ss_mafpc_data_init;

	/* DDI flash test */
	vdd->panel_func.samsung_gm2_ddi_flash_prepare = ss_gm2_ddi_flash_prepare;

	/* gamma compensation for 48/96hz VRR mode in gamma mode2 */
	vdd->panel_func.samsung_gm2_gamma_comp_init = ss_gm2_gamma_comp_init;

	/* VRR */
	vdd->panel_func.samsung_vrr_set_te_mod = ss_vrr_set_te_mode;
	vdd->panel_func.samsung_lfd_get_base_val = ss_update_base_lfd_val;
	ss_vrr_init(&vdd->vrr);

	vdd->panel_func.samsung_check_support_mode = ss_check_support_mode;
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_S6E3XA1_AMF759VG01_QXGA";
	char panel_name[MAX_CMDLINE_PARAM_LEN];
	char panel_secondary_name[MAX_CMDLINE_PARAM_LEN];

	ss_get_primary_panel_name_cmdline(panel_name);
	ss_get_secondary_panel_name_cmdline(panel_secondary_name);

	/* TODO: use component_bind with panel_func
	 * and match by panel_string, instead.
	 */
	if (!strncmp(panel_string, panel_name, strlen(panel_string)))
		ndx = PRIMARY_DISPLAY_NDX;
	else if (!strncmp(panel_string, panel_secondary_name,
				strlen(panel_string)))
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

	return 0;
}

early_initcall(samsung_panel_initialize);
