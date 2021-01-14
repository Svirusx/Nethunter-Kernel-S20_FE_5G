/*
 * =================================================================
 *
 *
 *	Description:  samsung display panel file
 *
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2020, Samsung Electronics. All rights reserved.

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
#include "ss_dsi_panel_S6E3HAC_AMB687VX01.h"
#include "ss_dsi_mdnie_S6E3HAC_AMB687VX01.h"

/* AOD Mode status on AOD Service */

enum {
	ALPM_CTRL_2NIT,
	ALPM_CTRL_10NIT,
	ALPM_CTRL_30NIT,
	ALPM_CTRL_60NIT,
	HLPM_CTRL_2NIT,
	HLPM_CTRL_10NIT,
	HLPM_CTRL_30NIT,
	HLPM_CTRL_60NIT,
	MAX_LPM_CTRL,
};

enum {
	HIGH_TEMP = 0,
	MID_TEMP,
	LOW_TEMP,
	MAX_TEMP
};

/* Register to cnotrol ALPM/HLPM mode */
#define LUMINANCE_MAX 74

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("+: ndx=%d\n", vdd->ndx);
	ss_panel_attach_set(vdd, true);
	LCD_INFO("-: ndx=%d \n", vdd->ndx);

	return true;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	struct vrr_info *vrr = &vdd->vrr;
	int cur_rr = vrr->cur_refresh_rate;

	/* panel rev.A (ID3 0x0, 0x40): EVT0 DDI version, tx temporal code and wacom setting in TX_TEMP_DSC.
	 * panel rev.B and C (ID3 0x50, 0x51): tx wacom setting in TX_TEMP_DSC.
	 * from panel rev.D (ID3 0x52): do not transmit TX_TEMP_DSC
	 */
	if (vdd->panel_revision < 3) { /* panel rev.A~ rev.C */
		LCD_INFO("tx temproal/wacom setting cmd (ID: 0x%X)\n", vdd->manufacture_id_dsi);
		ss_send_cmd(vdd, TX_TEMP_DSC);
	}

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

	if (cur_rr == 48 | cur_rr == 96) {
		/* GAMMA MTP is reset to original MTP value by display off -> on.
		 * In 48/96hz mode, apply compensated gamma.
		 */
		vrr->gm2_gamma = VRR_GM2_GAMMA_COMPENSATE;
		LCD_INFO("compensate gamma for 48/96hz mode\n");
	}

	if (vdd->is_factory_mode) {
		LCD_INFO("tx fg_err\n");
		ss_send_cmd(vdd, TX_FG_ERR);
	}

	return true;
}

static char ss_panel_revision(struct samsung_display_driver_data *vdd)
{
	char id;

	if (vdd->manufacture_id_dsi == PBA_ID)
		ss_panel_attach_set(vdd, false);
	else
		ss_panel_attach_set(vdd, true);

	id = ss_panel_id2_get(vdd);

	switch (id) {
	case 0x0:
	case 0x40:
		/* Rev.A: EVT0:
		 * - transmit temporal code.
		 * - support only MCD test code.
		 */
		vdd->panel_revision = 'A';
		break;
	case 0x50:
		/* Rev.B: 2nd panel. EVT1:
		 * - do not transmit temporal code.
		 * - support all test code (GCT, grayspot, and etc).
		 */
		vdd->panel_revision = 'B';
		break;
	case 0x51:
		/* Rev.C: 3rd panel */
		vdd->panel_revision = 'C';
		break;
	case 0x52:
		/* Rev.D: 4th panel */
		vdd->panel_revision = 'D';
		break;
	case 0x53:
		/* Rev.E: 5th panel */
		vdd->panel_revision = 'E';
		break;
	case 0x54:
		/* Rev.F: 6th panel (final version) */
		vdd->panel_revision = 'F';
		break;
	default:
		vdd->panel_revision = 'F';
		LCD_ERR("Invalid panel_rev(default rev : %c)\n",
				vdd->panel_revision);
		break;
	}

	/* W/A: Y type 800451 is 4th panel.. requested by dlab.. */
	if (vdd->manufacture_id_dsi == 0x800451) {
		/* Rev.D: 4th panel */
		vdd->panel_revision = 'D';
		LCD_INFO("4th U-type panel\n");
	}

	vdd->panel_revision -= 'A';

	LCD_INFO_ONCE("panel_revision = %c %d \n",
			vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision + 'A');
}

#define get_bit(value, shift, width)	((value >> shift) & (GENMASK(width - 1, 0)))
static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_normal(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;
	int tset;

	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_NORMAL);

	tset =  vdd->br_info.temperature > 0 ?  vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

	pcmds->cmds[1].msg.tx_buf[1] = tset;
	pcmds->cmds[1].msg.tx_buf[3] = 0x16;	/* ELVSS Value is fixed 0x16 for normal brightness */

	pcmds->cmds[2].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 2);
	pcmds->cmds[2].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

	/* IRC mode: 0x65: Moderato mode, 0x25: flat gamma mode */
	if (vdd->br_info.common_br.irc_mode == IRC_MODERATO_MODE)
		pcmds->cmds[4].msg.tx_buf[1] = 0x65;
	else if (vdd->br_info.common_br.irc_mode == IRC_FLAT_GAMMA_MODE)
		pcmds->cmds[4].msg.tx_buf[1] = 0x25;
	else
		LCD_ERR("invalid irc mode(%d)\n", vdd->br_info.common_br.irc_mode);

	LCD_INFO("cd_idx: %d, cd_level: %d, WRDISBV: %x %x, elvss: %x, IRC: %x\n",
			vdd->br_info.common_br.cd_idx,
			vdd->br_info.common_br.cd_level,
			pcmds->cmds[2].msg.tx_buf[1],
			pcmds->cmds[2].msg.tx_buf[2],
			pcmds->cmds[1].msg.tx_buf[3],
			pcmds->cmds[4].msg.tx_buf[1]);

	return pcmds;
}

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_hbm(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;
	int tset;

	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HBM);

	tset =  vdd->br_info.temperature > 0 ?  vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

	pcmds->cmds[1].msg.tx_buf[1] = tset;

	if (vdd->panel_revision >= 3) {	/* Rev.D: 4th panel (81 04 52) */
		pcmds->cmds[1].msg.tx_buf[3] = elvss_table_hbm_revD[vdd->br_info.common_br.cd_idx];		/* ELVSS Value for HBM brgihtness */
	} else {
		pcmds->cmds[1].msg.tx_buf[3] = elvss_table_hbm_revA[vdd->br_info.common_br.cd_idx];		/* ELVSS Value for HBM brgihtness */
	}

	pcmds->cmds[2].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 2);
	pcmds->cmds[2].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

	/* IRC mode: 0x65: Moderato mode, 0x25: flat gamma mode */
	if (vdd->br_info.common_br.irc_mode == IRC_MODERATO_MODE)
		pcmds->cmds[4].msg.tx_buf[1] = 0x65;
	else if (vdd->br_info.common_br.irc_mode == IRC_FLAT_GAMMA_MODE)
		pcmds->cmds[4].msg.tx_buf[1] = 0x25;
	else
		LCD_ERR("invalid irc mode(%d)\n", vdd->br_info.common_br.irc_mode);

	LCD_INFO("cd_idx: %d, cd_level: %d, WRDISBV: %x %x, elvss: %x, IRC: %x\n",
			vdd->br_info.common_br.cd_idx,
			vdd->br_info.common_br.cd_level,
			pcmds->cmds[2].msg.tx_buf[1],
			pcmds->cmds[2].msg.tx_buf[2],
			pcmds->cmds[1].msg.tx_buf[3],
			pcmds->cmds[4].msg.tx_buf[1]);

	return pcmds;
}

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_hmt(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HMT);

	pcmds->cmds[0].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 2);
	pcmds->cmds[0].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

	LCD_INFO("cd_idx: %d, cd_level: %d, WRDISBV: %x %x\n",
			vdd->br_info.common_br.cd_idx,
			vdd->br_info.common_br.cd_level,
			pcmds->cmds[0].msg.tx_buf[1],
			pcmds->cmds[0].msg.tx_buf[2]);

	return pcmds;
}

/* flash map
 * F0000 ~ F0053: 2 mode * 3 temperature * 14 vbias cmd = 84 bytes
 * F0054 ~ F0055: checksum = 2 bytes
 * F0056: 1 byte
 * F0057 ~ F0058: NS/HS setting = 2 bytes
 */

static int ss_gm2_ddi_flash_prepare(struct samsung_display_driver_data *vdd)
{
	struct spi_device *spi_dev;
	struct ddi_spi_cmd_set *cmd_set = NULL;
	u8 checksum_buf[2];
	int checksum_tot_flash;
	int checksum_tot_cal = 0;
	int checksum_one_mode_flash;
	int is_diff_one_vbias_mode;

	struct flash_gm2 *gm2_table = &vdd->br_info.gm2_table;
	u8 *ddi_flash_raw_buf;
	int ddi_flash_raw_len = gm2_table->ddi_flash_raw_len;

	u8 *vbias_data; /* 2 modes (NS/HS) * 3 temperature modes * 22 bytes cmd = 132 */

	u8 *mtp_one_vbias_mode; /* 22 bytes, F4h 42nd ~ 55th, and 63rd */
	u8 *flash_one_vbias_mode; /* NS and T > 0 mode, 22 bytes */
	int len_one_vbias_mode = gm2_table->len_one_vbias_mode; // 22 bytes... for c2..

	int mode;
	int ret;
	int i;

	ddi_flash_raw_buf = gm2_table->ddi_flash_raw_buf;
	vbias_data = gm2_table->vbias_data;
	mtp_one_vbias_mode = gm2_table->mtp_one_vbias_mode;

	/* read ddi flash data via SPI */
	spi_dev = vdd->spi_dev;
	if (IS_ERR_OR_NULL(spi_dev)) {
		LCD_ERR("no spi_dev\n");
		ret = -ENODEV;
		goto err;
	}

	cmd_set = ss_get_spi_cmds(vdd, RX_DATA);
	if (cmd_set == NULL) {
		LCD_ERR("cmd_set is null..\n");
		ret = -EINVAL;
		goto err;
	}

	cmd_set->rx_size = ddi_flash_raw_len;
	cmd_set->rx_addr = gm2_table->ddi_flash_start_addr;
	ret = ss_spi_sync(spi_dev, ddi_flash_raw_buf, RX_DATA);
	if (ret) {
		LCD_ERR("fail to spi read.. ret (%d) \n", ret);
		goto err;
	}

	for (i = 0; i < ddi_flash_raw_len; i++)
		LCD_DEBUG("ddi_flash_raw_buf[%d]: %X\n", i, ddi_flash_raw_buf[i]);

	/* save vbias data to vbias & vint buffer according to panel spec. */
	for (mode = VBIAS_MODE_NS_WARM; mode < VBIAS_MODE_MAX; mode++) {
		u8 *vbias_one_mode = ss_get_vbias_data(gm2_table, mode);

		memcpy(vbias_one_mode, ddi_flash_raw_buf + (mode * 14), 14);
		memset(vbias_one_mode + 14, 0, 7);
		if (mode <= VBIAS_MODE_NS_COLD) /* NS: flash: F0057 */
			vbias_one_mode[21] = ddi_flash_raw_buf[ddi_flash_raw_len - 2];
		else 	/* HS: flash: F0058 */
			vbias_one_mode[21] = ddi_flash_raw_buf[ddi_flash_raw_len - 1];

		/* For 1st and 2nd panel (UB ID3 0x40 and 0x50), set to zero.
		 * Requested by dlab.
		 */
		if (vdd->panel_revision < 2)
			vbias_one_mode[21] = 0;
	}

	/* flash: F0054 ~ F0055: off_gm2_flash_checksum = 0x54 = 84 */
	memcpy(checksum_buf, ddi_flash_raw_buf + gm2_table->off_gm2_flash_checksum, 2);

	checksum_tot_flash = (checksum_buf[0] << 8) | checksum_buf[1];

	/* 16bit sum chec: F0000 ~ F0053 */
	for (i = 0; i < gm2_table->off_gm2_flash_checksum; i++)
		checksum_tot_cal += ddi_flash_raw_buf[i];

	if (vdd->panel_revision >= 3) {
		/* Rev.D: 4th panel: flash: F0057, F0058,
		 * C2 panel's checksum is located on middle of flash address...
		 */
		checksum_tot_cal += (ddi_flash_raw_buf[ddi_flash_raw_len - 2] + ddi_flash_raw_buf[ddi_flash_raw_len - 1]);
	}

	checksum_tot_cal &= ERASED_MMC_16BIT;

	/* checksum of one mode from flash (15bytes, NS and T > 0) */
	flash_one_vbias_mode = ss_get_vbias_data(gm2_table, VBIAS_MODE_NS_WARM);
	checksum_one_mode_flash = 0;
	for (i = 0; i < len_one_vbias_mode; i++)
		checksum_one_mode_flash += flash_one_vbias_mode[i];
	checksum_one_mode_flash &= ERASED_MMC_16BIT;

	/* configure and calculate checksum of one vbias mode from MTP.
	 * F4h 42nd ~ 55th, and 63rd, and calculate checksum
	 */
	memset(gm2_table->mtp_one_vbias_mode + 14, 0, 7);
	gm2_table->checksum_one_mode_mtp = 0;
	for (i = 0; i < gm2_table->len_one_vbias_mode; i++)
		gm2_table->checksum_one_mode_mtp += gm2_table->mtp_one_vbias_mode[i];
	gm2_table->checksum_one_mode_mtp &= ERASED_MMC_16BIT;

	/* compare one vbias mode data: FLASH Vs. MTP (22 bytes) */
	is_diff_one_vbias_mode = memcmp(flash_one_vbias_mode, mtp_one_vbias_mode, len_one_vbias_mode);

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

err:
	LCD_ERR("fail to read flash: ret=%d\n", ret);

	gm2_table->is_flash_checksum_ok = 0;
	gm2_table->checksum_tot_flash = 0;
	gm2_table->checksum_tot_cal = 0;
	gm2_table->checksum_one_mode_flash = 0;
	gm2_table->is_diff_one_vbias_mode = 1;

	return ret;
}

/*
 * convert_GAMMA_to_V_HAC : convert CA gamma reg to V format
 * src : packed gamma value
 * dst : extended V values (V255 ~ VT)
 */
static void convert_GAMMA_to_V_HAC(u8 *src, int *dst)
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
		LCD_DEBUG("gamma src[%d] %02x", i, src[i]);

	for (i = 0; i < GAMMA_V_SIZE; i++)
		LCD_DEBUG("gammaV dst[%d] %02x", i, dst[i]);
}

/*
 * convert_V_to_GAMMA_HAC : convert V format to CAh gamma reg
 * src : extended V values (V255 ~ VT)
 * dst : packed gamma values (CAh)
 */
static void convert_V_to_GAMMA_HAC(int *src, u8 *dst)
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

	for (i = 0; i < GAMMA_V_SIZE; i++)
		LCD_DEBUG("gammaV src[%d] %02x\n", i, src[i]);

	for (i = 0; i < GAMMA_SET_SIZE; i++)
		LCD_DEBUG("gamma dst[%d] %02x\n", i, dst[i]);

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
	if (vrr_mode == GAMMA_SET_VRR_NS)
		return rgb_offset_ns_revA[gamma_mode][gamma_id];
	else
		return rgb_offset_hs_revA[gamma_mode][gamma_id];
}

static int ss_gm2_gamma_comp_init(struct samsung_display_driver_data *vdd)
{
	struct mtp_gm2_info *gm2_mtp = &vdd->br_info.gm2_mtp;
	u8 *gamma_org;
	u8 *gamma_comp;
	u8 readbuf[GAMMA_SET_SIZE * GAMMA_SET_VRR_MAX * GAMMA_SET_MODE_MAX]; /* 444 */
	u8 *buf;
	int i_vrr, j_mode, k;
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
	/* C8h: 37 * 6 = 222 bytes
	 * 0x00 ~0x24: NS, MODE1
	 * 0x25 ~0x49: NS, MODE2
	 * 0x4A ~0x6E: NS, MODE3
	 * 0x6F ~0x93: HS, MODE1
	 * 0x94 ~0xB8: HS, MODE2
	 * 0xB9 ~0xDD: HS, MODE3
	 */
	rx_cmds->cmds->msg.tx_buf[0] =  0xC8;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = GAMMA_SET_SIZE * 6; /* len */
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 0x00; /* pos */
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, readbuf, LEVEL1_KEY);
	buf = readbuf;
	for (i_vrr = 0; i_vrr < GAMMA_SET_VRR_MAX; i_vrr++) {
		for (j_mode = GAMMA_SET_MODE1; j_mode < GAMMA_SET_MODE4; j_mode++) {
			gamma_org = get_gamma(gm2_mtp, i_vrr, j_mode, true);
			memcpy(gamma_org, buf, GAMMA_SET_SIZE);
			buf += GAMMA_SET_SIZE;
		}
	}

	/* C9h
	 * 0x00 ~0x24: NS, MODE4
	 * 0x25 ~0x49: NS, MODE5
	 * 0x4A ~0x6E: NS, MODE6
	 * 0x6F ~0x93: NS, MODE7
	 */
	rx_cmds->cmds->msg.tx_buf[0] =  0xC9;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = GAMMA_SET_SIZE * 4; /* len */
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 0x00; /* pos */
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, readbuf, LEVEL1_KEY);
	buf = readbuf;
	for (j_mode = GAMMA_SET_MODE4; j_mode < GAMMA_SET_MODE_MAX; j_mode++) {
		gamma_org = get_gamma(gm2_mtp, GAMMA_SET_VRR_NS, j_mode, true);
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
	for (j_mode = GAMMA_SET_MODE4; j_mode < GAMMA_SET_MODE_MAX; j_mode++) {
		gamma_org = get_gamma(gm2_mtp, GAMMA_SET_VRR_HS, j_mode, true);
		memcpy(gamma_org, buf, GAMMA_SET_SIZE);
		buf += GAMMA_SET_SIZE;
	}

	/* 2. compensation gamma */
	for (i_vrr = 0; i_vrr < GAMMA_SET_VRR_MAX; i_vrr++) {
		for (j_mode = GAMMA_SET_MODE1; j_mode < GAMMA_SET_MODE_MAX; j_mode++) {
			gamma_org = get_gamma(gm2_mtp, i_vrr, j_mode, true);
			convert_GAMMA_to_V_HAC(gamma_org, gammaV);

			for (k = 0; k < GAMMA_V_COMP_SIZE; k++)
				gammaV[k] += get_rgb_offset(i_vrr, j_mode, k);

			gamma_comp = get_gamma(gm2_mtp, i_vrr, j_mode, false);
			convert_V_to_GAMMA_HAC(gammaV, gamma_comp);
		}
	}

	return 0;
}

enum GAMMA_COMP_CMD_ID {
	GAMMA_COMP_CMDID_C8_1 = 0,
	GAMMA_COMP_CMDID_C8_2 = 2,
	GAMMA_COMP_CMDID_C9 = 3,
	GAMMA_COMP_CMDID_C7 = 5,
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
		 * 0x00 ~0x24: NS, MODE1
		 * 0x25 ~0x49: NS, MODE2
		 * 0x4A ~0x6E: NS, MODE3
		 */
		pos = 0;
		for (j_mode = GAMMA_SET_MODE1; j_mode < GAMMA_SET_MODE4; j_mode++) {
			gamma = get_gamma(gm2_mtp, GAMMA_SET_VRR_NS, j_mode, is_org);
			memcpy(buf + pos, gamma, GAMMA_SET_SIZE);
			pos += GAMMA_SET_SIZE;
		}
		memcpy(&pcmds->cmds[GAMMA_COMP_CMDID_C8_1].msg.tx_buf[1], buf,
				pcmds->cmds[GAMMA_COMP_CMDID_C8_1].msg.tx_len - 1);

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
		memcpy(&pcmds->cmds[GAMMA_COMP_CMDID_C8_2].msg.tx_buf[1], buf,
				pcmds->cmds[GAMMA_COMP_CMDID_C8_2].msg.tx_len - 1);

		/* C9h : 37 * 4 = 148 bytes
		 * 0x00 ~0x24: NS, MODE4
		 * 0x25 ~0x49: NS, MODE5
		 * 0x4A ~0x6E: NS, MODE6
		 * 0x6F ~0x93: NS, MODE7
		 */
		pos = 0;
		for (j_mode = GAMMA_SET_MODE4; j_mode < GAMMA_SET_MODE_MAX; j_mode++) {
			gamma = get_gamma(gm2_mtp, GAMMA_SET_VRR_NS, j_mode, is_org);
			memcpy(buf + pos, gamma, GAMMA_SET_SIZE);
			pos += GAMMA_SET_SIZE;
		}
		memcpy(&pcmds->cmds[GAMMA_COMP_CMDID_C9].msg.tx_buf[1], buf,
				pcmds->cmds[GAMMA_COMP_CMDID_C9].msg.tx_len - 1);

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

enum LFD_FREQ_MODE {
	LFD_FREQ_VRR48HS_48HZ = 0,
	LFD_FREQ_VRR48HS_32HZ,
	LFD_FREQ_VRR48HS_24HZ,
	LFD_FREQ_VRR48HS_12HZ,
	LFD_FREQ_VRR48HS_1HZ,

	LFD_FREQ_VRR48NS,

	LFD_FREQ_VRR60HS_60HZ,
	LFD_FREQ_VRR60HS_40HZ,
	LFD_FREQ_VRR60HS_30HZ,
	LFD_FREQ_VRR60HS_24HZ,
	LFD_FREQ_VRR60HS_11HZ,
	LFD_FREQ_VRR60HS_10HZ,
	LFD_FREQ_VRR60HS_1HZ,

	LFD_FREQ_VRR60NS_60HZ,
	LFD_FREQ_VRR60NS_30HZ,

	LFD_FREQ_VRR96HS_96HZ,
	LFD_FREQ_VRR96HS_48HZ,
	LFD_FREQ_VRR96HS_32HZ,
	LFD_FREQ_VRR96HS_24HZ,
	LFD_FREQ_VRR96HS_12HZ,
	LFD_FREQ_VRR96HS_1HZ,

	LFD_FREQ_VRR120HS_120HZ,
	LFD_FREQ_VRR120HS_60HZ,
	LFD_FREQ_VRR120HS_40HZ,
	LFD_FREQ_VRR120HS_30HZ,
	LFD_FREQ_VRR120HS_24HZ,
	LFD_FREQ_VRR120HS_11HZ,
	LFD_FREQ_VRR120HS_10HZ,
	LFD_FREQ_VRR120HS_1HZ,

	LFD_FREQ_MAX
};

enum LFD_CMD_MINFREQ_LPM_MODE {
	LFD_MINFREQ_LPM_30HZ = 0,	/* same to LFD off mode */
	LFD_MINFREQ_LPM_1HZ,	/* default LFD mode */
	LFD_MINFREQ_LPM_MAX
};

enum VRR_CMD_RR {
	/* 1Hz is PSR mode in LPM (AOD) mode, 10Hz is PSR mode in 120HS mode */
	VRR_48HS = 0,
	VRR_48NS,
	VRR_60HS,
	VRR_60NS,
	VRR_96HS,
	VRR_120HS,
	VRR_MAX
};

enum VRR_CMD_ID {
	VRR_CMDID_LFD_SET = 1,
	VRR_CMDID_LFD_ON = 4,
	VRR_CMDID_OSC1 = 5,
	VRR_CMDID_OSC2 = 7,
	VRR_CMDID_FREQ = 8,
	VRR_CMDID_VBAIS = 10,

	/* For AOR compensation in 48/96hz mode... */
	VRR_CMDID_VTOT_GPARA = 11,
	VRR_CMDID_VTOT = 12,
	VRR_CMDID_VTOT_VFP_OTP_GPARA = 13,
	VRR_CMDID_VTOT_VFP_OTP = 14,

	VRR_CMDID_TE_MOD= 15,

	VRR_CMDID_AOR_GPARA= 16,
	VRR_CMDID_AOR = 17,
};

static enum VRR_CMD_RR ss_get_vrr_id(int cur_rr, int cur_hs)
{
	enum VRR_CMD_RR vrr_id;

	switch (cur_rr) {
	case 48:
		vrr_id = (cur_hs) ? VRR_48HS : VRR_48NS ;
		break;
	case 60:
		vrr_id = (cur_hs) ? VRR_60HS : VRR_60NS ;
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
	case VRR_48NS:
		base_rr = 48;
		max_div_def = min_div_def = min_div_lowest = 1; // 48hz
		break;
	case VRR_60NS:
		base_rr = 60;
		max_div_def = 1; // 60hz
		min_div_def = min_div_lowest = 2; // 30hz
		break;
	case VRR_48HS:
		base_rr = 96;
		max_div_def = 2; // 48hz
		min_div_def = 8; // 12hz
		min_div_lowest = 96; // 1hz
		break;
	case VRR_96HS:
		base_rr = 96;
		max_div_def = 1; // 96hz
		min_div_def = 8; // 12hz
		min_div_lowest = 96; // 1hz
		break;
	case VRR_60HS:
		base_rr = 120;
		max_div_def = 2; // 60hz
		min_div_def = 11; // 10.9hz
		min_div_lowest = 120; // 1hz
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

/* 1. LFD min freq. and frame insertion (BDh 0x06th): BDh 0x6 offset, for LFD control
 * divider: 2nd param: (divder - 1)
 * frame insertion: 5th param: repeat count of current fps
 * 3rd param[7:4] : repeat count of half fps
 * 3rd param[3:0] : repeat count of quad fps
 *
 * ---------------------------------------------------------
 *     LFD_MIN  120HS~60HS  48HS~11HS  1HS       60NS~30NS
 * LFD_MAX
 * ---------------------------------------------------------
 * 120HS~96HS   10 00 01    14 00 01   EF 00 01
 *   60HS~1HS   10 00 01    00 00 02   EF 00 01
 *  60NS~30NS                                    00 00 01
 * ---------------------------------------------------------
 */
int ss_get_frame_insert_cmd(u8 *out_cmd, u32 base_rr,
				u32 min_div, u32 max_div, bool cur_hs)
{
	u32 min_freq = DIV_ROUND_UP(base_rr, min_div);
	u32 max_freq = DIV_ROUND_UP(base_rr, max_div);

	out_cmd[1] = 0;
	if (cur_hs) {
		if (min_freq > 48) {
			/* LFD_MIN=120HS~60HS: 10 00 01 */
			out_cmd[0] = 0x10;
			out_cmd[2] = 0x01;
		} else if (min_freq == 1) {
			/* LFD_MIN=1HS: EF 00 01 */
			out_cmd[0] = 0xEF;
			out_cmd[2] = 0x01;
		} else if (max_freq > 60) {
			/* LFD_MIN=48HS~11HS, LFD_MAX=120HS~96HS: 14 00 01 */
			out_cmd[0] = 0x14;
			out_cmd[2] = 0x01;
		} else {
			/* LFD_MIN=48HS~11HS, LFD_MAX=60HS~1HS: 00 00 02 */
			out_cmd[0] = 0x00;
			out_cmd[2] = 0x02;
		}
	} else {
		/* NS mode: 00 00 01 */
		out_cmd[0] = 0x00;
		out_cmd[2] = 0x01;
	}
	LCD_INFO("frame insert: %02X %02X %02X (LFD %uhz~%uhz, %s)\n",
			out_cmd[0], out_cmd[1], out_cmd[2], min_freq, max_freq,
			cur_hs ? "HS" : "NS");

	return 0;
}

/* 4. LFD (BDh 0x13th) on: 0x41, LFD off: 0xE1,
 * default on, turn off in 48/96hz
 */
u8 cmd_lfd_on[VRR_MAX] = {
	0x41, // 48HS
	0xE1, // 48NS
	0x41, // 60HS
	0x41, // 60NS
	0x41, // 96HS
	0x41, // 120HS
};

/* 5. OSC1 and porch (F2h): panel rev.A~C (UB ID 81 04 40, 50, 51):
 * HS  : C1 B4 04 00 01 1C 00 14 10 00 14 BC 1C 00 14 10 00
 * NS  : C3 B4 04 14 01 1C 00 14 10 00 14 bc 1C 00 34 0F E0
 *
 * 48/96HS: set VFP_HS (F2h 7~8th param (gara off=6, 7)) and VFP_NS (F2h 14~15th param (gpara=13, 14)) to 0x314 = 788
 * 48NS: set VFP_NS (F2h 14~15th param (gpara=13, 14)) to 0x314 = 788
 */
u8 cmd_osc1_revA[VRR_MAX][17] = {
	{0xC1, 0xB4, 0x04, 0x00, 0x01, 0x1C, 0x03, 0x14, 0x10, 0x00, 0x14, 0xBC, 0x1C, 0x03, 0x14, 0x10, 0x00}, // 48HS, based on 96HS porch
	{0xC3, 0xB4, 0x04, 0x14, 0x01, 0x1C, 0x00, 0x14, 0x10, 0x00, 0x14, 0xBC, 0x1C, 0x03, 0x14, 0x0F, 0xE0}, // 48NS
	{0xC1, 0xB4, 0x04, 0x00, 0x01, 0x1C, 0x00, 0x14, 0x10, 0x00, 0x14, 0xBC, 0x1C, 0x00, 0x14, 0x10, 0x00}, // 60HS, VBP=0x1C=28, VFP=0x14=20
	{0xC3, 0xB4, 0x04, 0x14, 0x01, 0x1C, 0x00, 0x14, 0x10, 0x00, 0x14, 0xBC, 0x1C, 0x00, 0x34, 0x0F, 0xE0}, // 60NS, VBP=0x1C=28, VFP=0x34=52
	{0xC1, 0xB4, 0x04, 0x00, 0x01, 0x1C, 0x03, 0x14, 0x10, 0x00, 0x14, 0xBC, 0x1C, 0x03, 0x14, 0x10, 0x00}, // 96HS, VBP=0x1C=28, VFP=0x314=788, it should also update VFP_NS (14,15th param..)
	{0xC1, 0xB4, 0x04, 0x00, 0x01, 0x1C, 0x00, 0x14, 0x10, 0x00, 0x14, 0xBC, 0x1C, 0x00, 0x14, 0x10, 0x00}, // 120HS, VBP=0x1C=28, VFP=0x14=20
};

/* 5. OSC1 and porch (F2h): panel rev.D (UB ID 81 04 52, 4th panel):
 * HS  : C1 B4 04 00 01 20 00 10 10 00 14 BC 20 00 10 10 00
 * NS  : C3 B4 04 14 01 20 00 10 10 00 14 BC 20 00 30 0F E0
 *
 * 48/96HS: set VFP_HS (F2h 7~8th param (gara off=6, 7)) and VFP_NS (F2h 14~15th param (gpara=13, 14)) to 0x320 = 800
 * 48NS: set VFP_NS (F2h 14~15th param (gpara=13, 14)) to 0x320 = 800
 */
u8 cmd_osc1_revD[VRR_MAX][17] = {
	{0xC1, 0xB4, 0x04, 0x00, 0x01, 0x20, 0x03, 0x20, 0x10, 0x00, 0x14, 0xBC, 0x20, 0x03, 0x20, 0x10, 0x00}, // 48HS, VBP=0x20=32, VFP=0x320=800
	{0xC3, 0xB4, 0x04, 0x14, 0x01, 0x20, 0x00, 0x10, 0x10, 0x00, 0x14, 0xBC, 0x20, 0x03, 0x20, 0x0F, 0xE0}, // 48NS, VBP=0x20=32, VFP=0x320=800
	{0xC1, 0xB4, 0x04, 0x00, 0x01, 0x20, 0x00, 0x10, 0x10, 0x00, 0x14, 0xBC, 0x20, 0x00, 0x10, 0x10, 0x00}, // 60HS, VBP=0x20=32, VFP=0x10=16
	{0xC3, 0xB4, 0x04, 0x14, 0x01, 0x20, 0x00, 0x10, 0x10, 0x00, 0x14, 0xBC, 0x20, 0x00, 0x30, 0x0F, 0xE0}, // 60NS, VBP=0x20=32, VFP=0x30=48
	{0xC1, 0xB4, 0x04, 0x00, 0x01, 0x20, 0x03, 0x20, 0x10, 0x00, 0x14, 0xBC, 0x20, 0x03, 0x20, 0x10, 0x00}, // 96HS, VBP=0x20=32, VFP=0x320=800
	{0xC1, 0xB4, 0x04, 0x00, 0x01, 0x20, 0x00, 0x10, 0x10, 0x00, 0x14, 0xBC, 0x20, 0x00, 0x10, 0x10, 0x00}, // 120HS, VBP=0x20=32, VFP=0x10=16
};

/* 7. OSC2 (6Ah): HS: 21 0F, NS: 00 0F */
u8 cmd_osc2[VRR_MAX][2] = {
	{0x21, 0x0F}, // 48HS
	{0x00, 0x0F}, // 48NS
	{0x21, 0x0F}, // 60HS
	{0x00, 0x0F}, // 60NS
	{0x21, 0x0F}, // 96HS
	{0x21, 0x0F}, // 120HS
};

/* 8. Freq. (60h): frequency in image update case (non-LFD mode), HS: 24hz~120hz, NS: 30hz~60hz
 * - 48HS VRR mode:
 *     48hz  : 00 01 : div=2
 *     32hz  : 00 02 : div=3
 *     24hz  : 00 03 : div=4
 *     12hz  : 00 07 : div=8
 *      1hz  : 00 5F : div=96
 *
 * - 48NS VRR mode: turn off LFD
 *
 * - 60HS VRR mode:
 *     60hz  : 00 01 : div=2
 *     40hz  : 00 02 : div=3
 *     30hz  : 00 03 : div=4
 *     24hz  : 00 04 : div=5
 *     10hz  : 00 0B : div=12
 *      1hz  : 00 77 : div=120
 *
 * - 60NS VRR mode:
 *     60hz  : 00 09 : div=1
 *     30hz  : 00 01 : div=2
 *
 * - 96HS VRR mode:
 *     96hz  : 08 00 : div=1
 *     48hz  : 00 01 : div=2
 *     32hz  : 00 02 : div=3
 *     24hz  : 00 03 : div=4
 *     12hz  : 00 07 : div=8
 *      1hz  : 00 5F : div=96
 *
 * - 120HS VRR mode:
 *    120hz  : 08 00 : div=1
 *     60hz  : 00 01 : div=2
 *     40hz  : 00 02 : div=3
 *     30hz  : 00 03 : div=4
 *     24hz  : 00 04 : div=5
 *     11hz  : 00 0A : div=11
 *     10hz  : 00 0B : div=12
 *      1hz  : 00 77 : div=120
 */
u8 cmd_freq_max[VRR_MAX][2] = {
	{0x00, 0x01},	/* 48HS_48HZ, default */
	{0x00, 0x00},	/* 48NS, default  */
	{0x00, 0x01},	/* 60HS_60HZ, default  */
	{0x00, 0x00},	/* 60NS_60HZ, default  */
	{0x08, 0x00},	/* 96HS_96HZ, default  */
	{0x08, 0x00},	/* 120HS_120HZ, default  */
};

/* 11. for 48/96hz, 1 frame (VBP+RES+VFP) offset
 * NS: 0x2B
 * HS: 0x43
 */
u8 cmd_vtot_gpara[VRR_MAX] = {
	0x43, // 48HS
	0x2B, // 48NS
	0x43, // 60HS
	0x2B, // 60NS
	0x43, // 96HS
	0x43, // 120HS
};

/* 12. for 48/96hz, 1 frame (VBP+RES+VFP): panel rev.A~C (UB ID 81 04 40, 50, 51):
 * 60NS MTP  : 0C 30
 * 48NS      : 0F 40
 * 120HS MTP : 0C 30
 * 48/96HS   : 0F 40
 */
u8 cmd_vtot_revA[VRR_MAX][2] = {
	{0x0F, 0x40}, // 48HS based on 120HS
	{0x0F, 0x40}, // 48NS based on 60NS
	{0x0C, 0x30}, // 60HS
	{0x0C, 0x30}, // 60NS
	{0x0F, 0x40}, // 96HS based on 120HS
	{0x0C, 0x30}, // 120HS
};

/* 12. for 48/96hz, 1 frame (VBP+RES+VFP): panel rev.D (UB ID 81 04 52, 4th panel):
 * 60NS MTP  : 0C 60
 * 48NS      : 0F 50
 * 120HS MTP : 0C 40
 * 48/96HS   : 0F 50
 */
u8 cmd_vtot_revD[VRR_MAX][2] = {
	{0x0F, 0x50}, // 48HS based on 120HS
	{0x0F, 0x50}, // 48NS based on 60NS
	{0x0C, 0x40}, // 60HS
	{0x0C, 0x60}, // 60NS
	{0x0F, 0x50}, // 96HS based on 120HS
	{0x0C, 0x40}, // 120HS
};

/* 13. for 48/96hz, 1 frame (VBP+RES+VFP) and VFP OPT #1 offset
 * NS: 0x3D
 * HS: 0x55
 */
u8 cmd_vtot_vfp_otp_gpara[VRR_MAX] = {
	0x55, // 48HS
	0x3D, // 48NS
	0x55, // 60HS
	0x3D, // 60NS
	0x55, // 96HS
	0x55, // 120HS
};

/* 14. for 48/96hz, 1 frame (VBP+RES+VFP) and VFP OPT #1: panel rev.A~C (UB ID 81 04 40, 50, 51):
 * 60NS MTP  : 0C 30 0B F2 / 0C 60 0C 22
 * 48NS      : 0F 40 0E E0 / 0F 40 0E E0
 * 120HS MTP : 0C 30 0B F2 / 0C 40 0C 02
 * 48/96HS   : 0F 40 0E E0 / 0F 40 0E E0
 */
u8 cmd_vtot_vfp_otp_revA[VRR_MAX][4] = {
	{0x0F, 0x40, 0x0E, 0xE0}, // 48HS based on 120HS
	{0x0F, 0x40, 0x0E, 0xE0}, // 48NS based on 60NS
	{0x0C, 0x30, 0x0B, 0xF2}, // 60HS
	{0x0C, 0x30, 0x0B, 0xF2}, // 60NS
	{0x0F, 0x40, 0x0E, 0xE0}, // 96HS based on 120HS
	{0x0C, 0x30, 0x0B, 0xF2}, // 120HS
};

/* 14. for 48/96hz, 1 frame (VBP+RES+VFP) and VFP OPT #1: panel rev.D (UB ID 81 04 52, 4th panel):
 * 60NS MTP  : 0C 60 0C 22
 * 48NS      : 0F 50 0F 04
 * 120HS MTP : 0C 40 0C 02
 * 48/96HS   : 0F 50 0F 04
 */
u8 cmd_vtot_vfp_otp_revD[VRR_MAX][4] = {
	{0x0F, 0x50, 0x0F, 0x04}, // 48HS based on 120HS
	{0x0F, 0x50, 0x0F, 0x04}, // 48NS based on 60NS
	{0x0C, 0x40, 0x0C, 0x02}, // 60HS
	{0x0C, 0x60, 0x0C, 0x22}, // 60NS
	{0x0F, 0x50, 0x0F, 0x04}, // 96HS based on 120HS
	{0x0C, 0x40, 0x0C, 0x02}, // 120HS
};

/* 15. TE modulation (B9h):
 * 08: default,
 * 19: for 60HS mode.
 *     TE operates at every 2 frame
 *     TE_SEL & TE_FRAME_SEL = 1 (BIT0 and BIT4)
 */
/*         1  2  3  4  5  6  7  8  9
 * ref: B9 08 00 14 00 18 00 00 00 00
 * TE_V_ST_1 = 0x14 = 20: TE start from 20th line (count from VBP start point)
 * TE_V_ED_1 = 0x18 = 24 : TE flag is off from 24th line
 */
u8 cmd_hw_te_mod[VRR_MAX][1] = {
	{0x19}, // 48HS, org TE is 96hz -> 48hz
	{0x08}, // 48NS, org TE is 48hz
	{0x19}, // 60HS, org TE is 120hz -> 60hz
	{0x08}, // 60NS, org TE is 60hz
	{0x08}, // 96HS, org TE is 96hz
	{0x08}, // 120HS, org TE is 120hz
};

/* gamma mode2 doesn't have to control AOR setting.
 * But, to compensate brightness/color for 48/96hz VRR modes,
 * calculate proper AOR value by VFP of each VRR modes, and set AOR.
 * AOR shasdow register is updated in next vsync, but the other vrr cmds, like porch setting,
 * will be updated in F7h gamma update cmd.
 * So put aor cmd close to F7h gamma update cmd to prevent flicker.
 */
/* 16. AOR gparam
 * HS: B1h gpara = 0x59;
 * NS: B1h gpara = 0x41;
 */

/* 17. AOR (B1h): panel rev.A~C (UB ID 81 04 40, 50, 51):
 * NS MTP: 01 F0
 * 48NS : 02 70
 * HS MTP: 01 F0
 * 48/96HS : 02 70
 */
u8 cmd_aor_revA[VRR_MAX][2] = {
	{0x02, 0x70}, // 48HS
	{0x02, 0x70}, // 48NS
	{0x01, 0xF0}, // 60HS
	{0x01, 0xF0}, // 60NS
	{0x02, 0x70}, // 96HS
	{0x01, 0xF0}, // 120HS
};

/* 17. AOR (B1h): panel rev.D (UB ID 81 04 52, 4th panel):
 * NS MTP: 02 00
 * 48NS : 02 50
 * HS MTP: 01 F8
 * 48/96HS : 02 50
 */
u8 cmd_aor_revD[VRR_MAX][2] = {
	{0x02, 0x50}, // 48HS
	{0x02, 0x50}, // 48NS
	{0x01, 0xF8}, // 60HS
	{0x02, 0x00}, // 60NS
	{0x02, 0x50}, // 96HS
	{0x01, 0xF8}, // 120HS
};

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
	u32 min_div, max_div;
	enum LFD_SCOPE_ID scope;
	u8 cmd_frame_insert[3];

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

		if (panel->cur_mode->timing.refresh_rate != vrr->adjusted_refresh_rate ||
				panel->cur_mode->timing.sot_hs_mode != vrr->adjusted_sot_hs_mode)
			LCD_DEBUG("VRR: unmatched RR mode (%dhz%s / %dhz%s)\n",
					panel->cur_mode->timing.refresh_rate,
					panel->cur_mode->timing.sot_hs_mode ? "HS" : "NM",
					vrr->adjusted_refresh_rate,
					vrr->adjusted_sot_hs_mode ? "HS" : "NM");
	}

	cur_rr = vrr->cur_refresh_rate;
	cur_hs = vrr->cur_sot_hs_mode;

	if (is_hmt && !(cur_rr == 120 && cur_hs))
		LCD_ERR("error: HMT not in 120HZ@HS mode, cur: %dhz%s\n",
				cur_rr, cur_hs ? "HS" : "NM");

	vrr_id = ss_get_vrr_id(cur_rr, cur_hs);

	scope = is_hmt ? LFD_SCOPE_HMD : LFD_SCOPE_NORMAL ;
	if (ss_get_lfd_div(vdd, scope, &min_div, &max_div)) {
		LCD_ERR("fail to get LFD divider.. set default 1..\n");
		max_div = min_div = 1;
	}
	vrr->lfd.min_div = min_div;
	vrr->lfd.max_div = max_div;

	/* 1. LFD min freq. and frame insertion: BDh 0x06th
	 * 1st~2nd param: min freq. divider. (divider - 1)
	 * 3rd~5th param: frame insertion
	 *     5th param: repeat count of current fps
	 *     3rd param[7:4] : repeat count of half fps
	 *     3rd param[3:0] : repeat count of quad fps
	 * 6th~8th param: padding to 0x00
	 * 9th param: 0x1 for HS mode, 0x0 for NS mode
	 */
	vrr_cmds->cmds[VRR_CMDID_LFD_SET].msg.tx_buf[2] = (min_div - 1);
	ss_get_frame_insert_cmd(cmd_frame_insert, vrr->lfd.base_rr, min_div, max_div, cur_hs);
	memcpy(&vrr_cmds->cmds[VRR_CMDID_LFD_SET].msg.tx_buf[3], cmd_frame_insert, 3);
	vrr_cmds->cmds[VRR_CMDID_LFD_SET].msg.tx_buf[9] = (cur_hs) ? 0x1 : 0x0;

	/* 4. LFD on/off */
	vrr_cmds->cmds[VRR_CMDID_LFD_ON].msg.tx_buf[1] = cmd_lfd_on[vrr_id];

	/* 5. OSC1,:Porch & HW setting */
	if (vdd->panel_revision < 3)	/* panel rev.A~ rev.C */
		memcpy(&vrr_cmds->cmds[VRR_CMDID_OSC1].msg.tx_buf[1],
				&cmd_osc1_revA[vrr_id][0],
				vrr_cmds->cmds[VRR_CMDID_OSC1].msg.tx_len - 1);
	else	/* panel rev.D: 4th panel (81 04 52) */
		memcpy(&vrr_cmds->cmds[VRR_CMDID_OSC1].msg.tx_buf[1],
				&cmd_osc1_revD[vrr_id][0],
				vrr_cmds->cmds[VRR_CMDID_OSC1].msg.tx_len - 1);

	/* 7. OSC2 **/
	memcpy(&vrr_cmds->cmds[VRR_CMDID_OSC2].msg.tx_buf[1],
			&cmd_osc2[vrr_id][0],
			vrr_cmds->cmds[VRR_CMDID_OSC2].msg.tx_len - 1);

	/* 8. Freq. (60h): frequency in image update case (non-LFD mode) */
	memcpy(&vrr_cmds->cmds[VRR_CMDID_FREQ].msg.tx_buf[1],
			&cmd_freq_max[vrr_id][0],
			vrr_cmds->cmds[VRR_CMDID_FREQ].msg.tx_len - 1);
	vrr_cmds->cmds[VRR_CMDID_FREQ].msg.tx_buf[2] = max_div - 1;
	if (max_div == 1 && cur_hs)
		vrr_cmds->cmds[VRR_CMDID_FREQ].msg.tx_buf[1] = 0x08;
	else
		vrr_cmds->cmds[VRR_CMDID_FREQ].msg.tx_buf[1] = 0x00;


	/* 10. Vbais & Vaint setting: 14bytes and last 1byte from flash */
	if (vdd->br_info.temperature > 0) {
		mode = (vrr->cur_sot_hs_mode) ? VBIAS_MODE_HS_WARM : VBIAS_MODE_NS_WARM;
	} else if (vdd->br_info.temperature > -15) {
		mode = (vrr->cur_sot_hs_mode) ? VBIAS_MODE_HS_COOL : VBIAS_MODE_NS_COOL;
	} else {
		mode = (vrr->cur_sot_hs_mode) ? VBIAS_MODE_HS_COLD : VBIAS_MODE_NS_COLD;
	}

	vbias = ss_get_vbias_data(&vdd->br_info.gm2_table, mode);
	if (!vbias) {
		LCD_ERR("fail to get vbias..\n");
	} else {
		memcpy(&vrr_cmds->cmds[VRR_CMDID_VBAIS].msg.tx_buf[1],
				vbias,
				vrr_cmds->cmds[VRR_CMDID_VBAIS].msg.tx_len - 1);
	}

	/* 11. for 48/96hz, 1 frame (VBP+RES+VFP) offset */
	vrr_cmds->cmds[VRR_CMDID_VTOT_GPARA].msg.tx_buf[1] = cmd_vtot_gpara[vrr_id];

	/* 12. for 48/96hz, 1 frame (VBP+RES+VFP) */
	if (vdd->panel_revision < 3)	/* panel rev.A~ rev.C */
		memcpy(&vrr_cmds->cmds[VRR_CMDID_VTOT].msg.tx_buf[1],
				&cmd_vtot_revA[vrr_id][0],
				vrr_cmds->cmds[VRR_CMDID_VTOT].msg.tx_len - 1);
	else	/* panel rev.D: 4th panel (81 04 52) */
		memcpy(&vrr_cmds->cmds[VRR_CMDID_VTOT].msg.tx_buf[1],
				&cmd_vtot_revD[vrr_id][0],
				vrr_cmds->cmds[VRR_CMDID_VTOT].msg.tx_len - 1);


	/* 13. for 48/96hz, 1 frame (VBP+RES+VFP) and VFP OPT #1 offset */
	vrr_cmds->cmds[VRR_CMDID_VTOT_VFP_OTP_GPARA].msg.tx_buf[1] = cmd_vtot_vfp_otp_gpara[vrr_id];

	/* 14. for 48/96hz, 1 frame (VBP+RES+VFP) and VFP OPT #1 */
	if (vdd->panel_revision < 3)	/* panel rev.A~ rev.C */
		memcpy(&vrr_cmds->cmds[VRR_CMDID_VTOT_VFP_OTP].msg.tx_buf[1],
				&cmd_vtot_vfp_otp_revA[vrr_id][0],
				vrr_cmds->cmds[VRR_CMDID_VTOT_VFP_OTP].msg.tx_len - 1);
	else	/* panel rev.D: 4th panel (81 04 52) */
		memcpy(&vrr_cmds->cmds[VRR_CMDID_VTOT_VFP_OTP].msg.tx_buf[1],
				&cmd_vtot_vfp_otp_revD[vrr_id][0],
				vrr_cmds->cmds[VRR_CMDID_VTOT_VFP_OTP].msg.tx_len - 1);

	/* 15. TE modulation (B9h) */
	memcpy(&vrr_cmds->cmds[VRR_CMDID_TE_MOD].msg.tx_buf[1],
			&cmd_hw_te_mod[vrr_id][0],
			vrr_cmds->cmds[VRR_CMDID_TE_MOD].msg.tx_len - 1);


	/* 16. AOR gparam
	 * HS: B1h gpara = 0x59;
	 * NS: B1h gpara = 0x41;
	 */
	if (cur_hs)
		vrr_cmds->cmds[VRR_CMDID_AOR_GPARA].msg.tx_buf[1] = 0x59;	/* HS: B1h gpara 0x59~0x5A */
	else
		vrr_cmds->cmds[VRR_CMDID_AOR_GPARA].msg.tx_buf[1] = 0x41;	/* NS: B1h gpara 0x41~0x42 */

	/* 17. AOR value */
	if (is_hmt) {
		/* HMT: 0A 64 AOR 85% */
		vrr_cmds->cmds[VRR_CMDID_AOR].msg.tx_buf[1] = 0x0A;
		vrr_cmds->cmds[VRR_CMDID_AOR].msg.tx_buf[2] = 0x64;
	} else {
		if (vdd->panel_revision < 3)	/* panel rev.A~ rev.C */
			memcpy(&vrr_cmds->cmds[VRR_CMDID_AOR].msg.tx_buf[1],
					&cmd_aor_revA[vrr_id][0],
					vrr_cmds->cmds[VRR_CMDID_AOR].msg.tx_len - 1);
		else	/* panel rev.D: 4th panel (81 04 52) */
			memcpy(&vrr_cmds->cmds[VRR_CMDID_AOR].msg.tx_buf[1],
					&cmd_aor_revD[vrr_id][0],
					vrr_cmds->cmds[VRR_CMDID_AOR].msg.tx_len - 1);
	}

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

static struct dsi_panel_cmd_set *ss_vrr_hmt(struct samsung_display_driver_data *vdd, int *level_key)
{
	bool is_hbm = false;
	bool is_hmt = true;

	return __ss_vrr(vdd, level_key, is_hbm, is_hmt);
}


/* TE moudulation (reports vsync as 60hz even TE is 120hz, in 60HS mode)
 * Some HOP display, like C2 HAC UB, supports self scan function.
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

#define COORDINATE_DATA_SIZE 6
#define MDNIE_SCR_WR_ADDR	0x32
#define RGB_INDEX_SIZE 4
#define COEFFICIENT_DATA_SIZE 8

#define F1(x, y) (((y << 10) - (((x << 10) * 56) / 55) - (102 << 10)) >> 10)
#define F2(x, y) (((y << 10) + (((x << 10) * 5) / 1) - (18483 << 10)) >> 10)

static char coordinate_data_1[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xfa, 0x00, 0xf9, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfc, 0x00, 0xff, 0x00}, /* Tune_2 */
	{0xf8, 0x00, 0xf7, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfd, 0x00, 0xf9, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xf8, 0x00, 0xfa, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xfd, 0x00, 0xff, 0x00, 0xf8, 0x00}, /* Tune_7 */
	{0xfb, 0x00, 0xff, 0x00, 0xfc, 0x00}, /* Tune_8 */
	{0xf8, 0x00, 0xfd, 0x00, 0xff, 0x00}, /* Tune_9 */
};

static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xf9, 0x00, 0xf6, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfb, 0x00, 0xfd, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfb, 0x00, 0xf0, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xff, 0x00, 0xfd, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfe, 0x00, 0xf0, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xff, 0x00, 0xf6, 0x00}, /* Tune_8 */
	{0xfc, 0x00, 0xff, 0x00, 0xfa, 0x00}, /* Tune_9 */
};

static char (*coordinate_data[MAX_MODE])[COORDINATE_DATA_SIZE] = {
	coordinate_data_2,
	coordinate_data_2,
	coordinate_data_2,
	coordinate_data_1,
	coordinate_data_1,
	coordinate_data_1
};

static int rgb_index[][RGB_INDEX_SIZE] = {
	{ 5, 5, 5, 5 }, /* dummy */
	{ 5, 2, 6, 3 },
	{ 5, 2, 4, 1 },
	{ 5, 8, 4, 7 },
	{ 5, 8, 6, 9 },
};

static int coefficient[][COEFFICIENT_DATA_SIZE] = {
	{       0,        0,      0,      0,      0,       0,      0,      0 }, /* dummy */
	{  -52615,   -61905,  21249,  15603,  40775,   80902, -19651, -19618 },
	{ -212096,  -186041,  61987,  65143, -75083,  -27237,  16637,  15737 },
	{   69454,    77493, -27852, -19429, -93856, -133061,  37638,  35353 },
	{  192949,   174780, -56853, -60597,  57592,   13018, -11491, -10757 },
};

static int mdnie_coordinate_index(int x, int y)
{
	int tune_number = 0;

	if (F1(x, y) > 0) {
		if (F2(x, y) > 0) {
			tune_number = 1;
		} else {
			tune_number = 2;
		}
	} else {
		if (F2(x, y) > 0) {
			tune_number = 4;
		} else {
			tune_number = 3;
		}
	}

	return tune_number;
}

static int mdnie_coordinate_x(int x, int y, int index)
{
	int result = 0;

	result = (coefficient[index][0] * x) + (coefficient[index][1] * y) + (((coefficient[index][2] * x + 512) >> 10) * y) + (coefficient[index][3] * 10000);

	result = (result + 512) >> 10;

	if (result < 0)
		result = 0;
	if (result > 1024)
		result = 1024;

	return result;
}

static int mdnie_coordinate_y(int x, int y, int index)
{
	int result = 0;

	result = (coefficient[index][4] * x) + (coefficient[index][5] * y) + (((coefficient[index][6] * x + 512) >> 10) * y) + (coefficient[index][7] * 10000);

	result = (result + 512) >> 10;

	if (result < 0)
		result = 0;
	if (result > 1024)
		result = 1024;

	return result;
}

static int ss_module_info_read(struct samsung_display_driver_data *vdd)
{
	unsigned char buf[11];
	int year, month, day;
	int hour, min;
	int x, y;
	int mdnie_tune_index = 0;
	int ret;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	if (ss_get_cmds(vdd, RX_MODULE_INFO)->count) {
		ret = ss_panel_data_read(vdd, RX_MODULE_INFO, buf, LEVEL1_KEY);
		if (ret) {
			LCD_ERR("fail to read module ID, ret: %d", ret);
			return false;
		}

		/* Manufacture Date */

		year = buf[4] & 0xf0;
		year >>= 4;
		year += 2011; // 0 = 2011 year
		month = buf[4] & 0x0f;
		day = buf[5] & 0x1f;
		hour = buf[6] & 0x0f;
		min = buf[7] & 0x1f;

		vdd->manufacture_date_dsi = year * 10000 + month * 100 + day;
		vdd->manufacture_time_dsi = hour * 100 + min;

		LCD_INFO("manufacture_date DSI%d = (%d%04d) - year(%d) month(%d) day(%d) hour(%d) min(%d)\n",
			vdd->ndx, vdd->manufacture_date_dsi, vdd->manufacture_time_dsi,
			year, month, day, hour, min);

		/* While Coordinates */

		vdd->mdnie.mdnie_x = buf[0] << 8 | buf[1];	/* X */
		vdd->mdnie.mdnie_y = buf[2] << 8 | buf[3];	/* Y */

		mdnie_tune_index = mdnie_coordinate_index(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);

		if (((vdd->mdnie.mdnie_x - 3050) * (vdd->mdnie.mdnie_x - 3050) + (vdd->mdnie.mdnie_y - 3210) * (vdd->mdnie.mdnie_y - 3210)) <= 225) {
			x = 0;
			y = 0;
		} else {
			x = mdnie_coordinate_x(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y, mdnie_tune_index);
			y = mdnie_coordinate_y(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y, mdnie_tune_index);
		}

		coordinate_tunning_calculate(vdd, x, y, coordinate_data,
				rgb_index[mdnie_tune_index],
				MDNIE_SCR_WR_ADDR, COORDINATE_DATA_SIZE);

		LCD_INFO("DSI%d : X-%d Y-%d \n", vdd->ndx,
			vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);

		/* CELL ID (manufacture date + white coordinates) */
		/* Manufacture Date */
		vdd->cell_id_dsi[0] = buf[4];
		vdd->cell_id_dsi[1] = buf[5];
		vdd->cell_id_dsi[2] = buf[6];
		vdd->cell_id_dsi[3] = buf[7];
		vdd->cell_id_dsi[4] = buf[8];
		vdd->cell_id_dsi[5] = buf[9];
		vdd->cell_id_dsi[6] = buf[10];
		/* White Coordinates */
		vdd->cell_id_dsi[7] = buf[0];
		vdd->cell_id_dsi[8] = buf[1];
		vdd->cell_id_dsi[9] = buf[2];
		vdd->cell_id_dsi[10] = buf[3];

		LCD_INFO("DSI%d CELL ID : %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			vdd->ndx, vdd->cell_id_dsi[0],
			vdd->cell_id_dsi[1],	vdd->cell_id_dsi[2],
			vdd->cell_id_dsi[3],	vdd->cell_id_dsi[4],
			vdd->cell_id_dsi[5],	vdd->cell_id_dsi[6],
			vdd->cell_id_dsi[7],	vdd->cell_id_dsi[8],
			vdd->cell_id_dsi[9],	vdd->cell_id_dsi[10]);
	} else {
		LCD_ERR("DSI%d no module_info_rx_cmds cmds(%d)", vdd->ndx, vdd->panel_revision);
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

	pcmds = ss_get_cmds(vdd, TX_ACL_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_ACL_ON..\n");
		return NULL;
	}

	pcmds->cmds[0].msg.tx_buf[1] = 0x01;	/* ACL 8% in HBM */

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

	pcmds = ss_get_cmds(vdd, TX_ACL_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_ACL_ON..\n");
		return NULL;
	}

	pcmds->cmds[0].msg.tx_buf[1] = 0x02;	/* ACL 15% in normal brightness */

	LCD_INFO("gradual_acl: %d, acl per: 0x%x",
			vdd->br_info.gradual_acl_val, pcmds->cmds[0].msg.tx_buf[1]);

	return pcmds;
}

static struct dsi_panel_cmd_set *ss_acl_off(struct samsung_display_driver_data *vdd, int *level_key)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	LCD_INFO("ACL off\n");
	return ss_get_cmds(vdd, TX_ACL_OFF);
}

static struct dsi_panel_cmd_set *ss_vint(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *vint_cmds = ss_get_cmds(vdd, TX_VINT);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd\n");
		return NULL;
	}

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(vint_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)vint_cmds);
		return NULL;
	}

	/* VGH setting: 0x08: x-talk on, 0x11: x-talk off */
	if (vdd->xtalk_mode)
		vint_cmds->cmds[1].msg.tx_buf[1] = 0x08;
	else
		vint_cmds->cmds[1].msg.tx_buf[1] = 0x11;

	LCD_INFO("xtalk_mode: %d\n", vdd->xtalk_mode);

	return vint_cmds;
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

// ========================
//			HMT
// ========================


enum LPMBL_CMD_ID {
	LPMBL_CMDID_CTRL = 2,
	LPMBL_CMDID_LFD = 4,
};

static void ss_set_panel_lpm_brightness(struct samsung_display_driver_data *vdd)
{
	struct vrr_info *vrr = &vdd->vrr;
	struct dsi_panel_cmd_set *set = ss_get_cmds(vdd, TX_LPM_BL_CMD);
	struct dsi_panel_cmd_set *set_lpm_bl;
	u32 min_div, max_div;

	if (SS_IS_CMDS_NULL(set)) {
		LCD_ERR("No cmds for TX_LPM_BL_CMD\n");
		return;
	}

	/* LPM_ON: 3. HLPM brightness */
	switch (vdd->panel_lpm.lpm_bl_level) {
	case LPM_60NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_LPM_60NIT_CMD);
		break;
	case LPM_30NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_LPM_30NIT_CMD);
		break;
	case LPM_10NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_LPM_10NIT_CMD);
		break;
	case LPM_2NIT:
	default:
		set_lpm_bl = ss_get_cmds(vdd, TX_LPM_2NIT_CMD);
		break;
	}

	if (SS_IS_CMDS_NULL(set_lpm_bl)) {
		LCD_ERR("No cmds for alpm_ctrl..\n");
		return;
	}

	memcpy(&set->cmds[LPMBL_CMDID_CTRL].msg.tx_buf[1],
			&set_lpm_bl->cmds->msg.tx_buf[1],
			sizeof(char) * (set->cmds[LPMBL_CMDID_CTRL].msg.tx_len - 1));

	/* 4. LFD min freq.=1hz and frame insertion=30hz-16frame */
	ss_get_lfd_div(vdd, LFD_SCOPE_LPM, &min_div, &max_div);
	vrr->lfd.min_div = min_div;
	vrr->lfd.max_div = 1;
	if (max_div != 1)
		LCD_ERR("LPM max_div support only 1 (req: %d)\n", max_div);

	set->cmds[LPMBL_CMDID_LFD].msg.tx_buf[2] = min_div - 1;

	/* send lpm bl cmd */
	ss_send_cmd(vdd, TX_LPM_BL_CMD);

	LCD_INFO("[Panel LPM] bl_level : %s, LFD: div cmd: 0x%x, %dhz-%dhz (%d-%d)\n",
			/* Check current brightness level */
			vdd->panel_lpm.lpm_bl_level == LPM_2NIT ? "2NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_10NIT ? "10NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_30NIT ? "30NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_60NIT ? "60NIT" : "UNKNOWN",
			set->cmds[LPMBL_CMDID_LFD].msg.tx_buf[2],
			DIV_ROUND_UP(vrr->lfd.base_rr, min_div),
			DIV_ROUND_UP(vrr->lfd.base_rr, max_div),
			min_div, max_div);
}

enum LPMON_CMD_ID {
	LPMON_CMDID_CTRL = 2,
	LPMON_CMDID_BL = 4,
	LPMON_CMDID_LFD = 6,
};

enum LPMOFF_CMD_ID {
	LPMOFF_CMDID_CTRL = 3,
	LPMOFF_CMDID_NORMAL_BL = 7,
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
	struct dsi_panel_cmd_set *set_lpm_off = ss_get_cmds(vdd, TX_LPM_OFF);
	struct dsi_panel_cmd_set *set_lpm_bl;
	u32 min_div, max_div;
	int gm2_wrdisbv;	/* Gamma mode2 WRDISBV Write Display Brightness */

	LCD_INFO("%s++\n", __func__);

	if (SS_IS_CMDS_NULL(set_lpm_on) || SS_IS_CMDS_NULL(set_lpm_off)) {
		LCD_ERR("No cmds for TX_LPM_ON/OFF\n");
		goto start_lpm_bl;
	}

	/* LPM_ON: 1. HLPM/ALPM Control: 0x09: HLPM, 0x0B: ALPM */
	/* LPM_OFF: 1. Seamless Control For HLPM: 0x01: HLPM, 0x03: ALPM */
	switch (vdd->panel_lpm.mode) {
	case HLPM_MODE_ON:
		set_lpm_on->cmds[LPMON_CMDID_CTRL].msg.tx_buf[1] = 0x09;
		set_lpm_off->cmds[LPMOFF_CMDID_CTRL].msg.tx_buf[1] = 0x01;
		break;
	case ALPM_MODE_ON:
		set_lpm_on->cmds[LPMON_CMDID_CTRL].msg.tx_buf[1] = 0x0B;
		set_lpm_off->cmds[LPMOFF_CMDID_CTRL].msg.tx_buf[1] = 0x03;
		break;
	default:
		LCD_ERR("invalid lpm mode: %d\n", vdd->panel_lpm.mode);
		break;
	}

start_lpm_bl:
	/* LPM_ON: 3. HLPM brightness */
	/* should restore normal brightness in LPM off sequence to prevent flicker.. */
	switch (vdd->panel_lpm.lpm_bl_level) {
	case LPM_60NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_LPM_60NIT_CMD);
		gm2_wrdisbv = 127;
		break;
	case LPM_30NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_LPM_30NIT_CMD);
		gm2_wrdisbv = 64;
		break;
	case LPM_10NIT:
		set_lpm_bl = ss_get_cmds(vdd, TX_LPM_10NIT_CMD);
		gm2_wrdisbv = 23;
		break;
	case LPM_2NIT:
	default:
		set_lpm_bl = ss_get_cmds(vdd, TX_LPM_2NIT_CMD);
		gm2_wrdisbv = 5;
		break;
	}

	if (vdd->panel_revision >= 3) {	/* Rev.D: 4th panel (81 04 52) */
		set_lpm_off->cmds[LPMOFF_CMDID_NORMAL_BL].msg.tx_buf[1] = get_bit(gm2_wrdisbv, 8, 2);
		set_lpm_off->cmds[LPMOFF_CMDID_NORMAL_BL].msg.tx_buf[2] = get_bit(gm2_wrdisbv, 0, 8);
		LCD_INFO("lpm off: brightness: %02X %02X\n",
			set_lpm_off->cmds[LPMOFF_CMDID_NORMAL_BL].msg.tx_buf[1],
			set_lpm_off->cmds[LPMOFF_CMDID_NORMAL_BL].msg.tx_buf[2]);
	}

	if (SS_IS_CMDS_NULL(set_lpm_bl)) {
		LCD_ERR("No cmds for alpm_ctrl..\n");
		return;
	}

	memcpy(&set_lpm_on->cmds[LPMON_CMDID_BL].msg.tx_buf[1],
			&set_lpm_bl->cmds->msg.tx_buf[1],
			sizeof(char) * set_lpm_on->cmds[LPMON_CMDID_BL].msg.tx_len - 1);


	/* 7. LFD min freq.=1hz and frame insertion=30hz-16frame */
	ss_get_lfd_div(vdd, LFD_SCOPE_LPM, &min_div, &max_div);
	vrr->lfd.min_div = min_div;
	vrr->lfd.max_div = 1;
	if (max_div != 1)
		LCD_ERR("LPM max_div support only 1 (req: %d)\n", max_div);

	set_lpm_on->cmds[LPMON_CMDID_LFD].msg.tx_buf[2] = min_div - 1;

	LCD_INFO("%s--\n", __func__);
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
	u8 valid_checksum[4] = {0x08, 0x48, 0x08, 0x48};
	int res;

	/* For EVT0 DDI version, should prevent GCT test. */
	if (vdd->panel_revision == 0) {
		LCD_ERR("prevent GCT for EVT0 (ID: 0x%X), just return PASS..\n", vdd->manufacture_id_dsi);
		return GCT_RES_CHECKSUM_PASS;
	}

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
	/* vddm set, 0x0: normal, 0x04: LV, 0x08: HV */
	u8 vddm_set[MAX_VDDM] = {0x0, 0x04, 0x08};
	int ret = 0;
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	/* For EVT0 DDI version, should prevent GCT test. */
	if (vdd->panel_revision == 0) {
		LCD_ERR("prevent GCT for EVT0 (ID: 0x%X)\n", vdd->manufacture_id_dsi);
		return 0;
	}

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
		set->cmds[14].msg.tx_buf[1] = vddm_set[i];
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

	LCD_INFO("Self Display Panel Data init\n");

	panel_type = (ss_panel_id0_get(vdd) & 0x30) >> 4;
	panel_color = ss_panel_id0_get(vdd) & 0xF;

	LCD_INFO("Panel Type=0x%x, Panel Color=0x%x\n", panel_type, panel_color);

	vdd->self_disp.operation[FLAG_SELF_MASK].img_buf = self_mask_img_data;
	vdd->self_disp.operation[FLAG_SELF_MASK].img_size = ARRAY_SIZE(self_mask_img_data);
	vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum = SELF_MASK_IMG_CHECKSUM;
	make_self_dispaly_img_cmds_HAC(vdd, TX_SELF_MASK_IMAGE, FLAG_SELF_MASK);

	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_buf = self_mask_img_wqhd_crc_data;
	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_size = ARRAY_SIZE(self_mask_img_wqhd_crc_data);
	make_mass_self_display_img_cmds_HAC(vdd, TX_SELF_MASK_IMAGE_CRC, FLAG_SELF_MASK_CRC);

	vdd->self_disp.operation[FLAG_SELF_ICON].img_buf = self_icon_img_data;
	vdd->self_disp.operation[FLAG_SELF_ICON].img_size = ARRAY_SIZE(self_icon_img_data);
	make_mass_self_display_img_cmds_HAC(vdd, TX_SELF_ICON_IMAGE, FLAG_SELF_ICON);

	vdd->self_disp.operation[FLAG_SELF_ACLK].img_buf = self_aclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_ACLK].img_size = ARRAY_SIZE(self_aclock_img_data);
	make_mass_self_display_img_cmds_HAC(vdd, TX_SELF_ACLOCK_IMAGE, FLAG_SELF_ACLK);

	vdd->self_disp.operation[FLAG_SELF_DCLK].img_buf = self_dclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_DCLK].img_size = ARRAY_SIZE(self_dclock_img_data);
	make_mass_self_display_img_cmds_HAC(vdd, TX_SELF_DCLOCK_IMAGE, FLAG_SELF_DCLK);

	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_buf = self_video_img_data;
	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_size = ARRAY_SIZE(self_video_img_data);
	make_mass_self_display_img_cmds_HAC(vdd, TX_SELF_VIDEO_IMAGE, FLAG_SELF_VIDEO);

	return 1;
}

static int ss_mafpc_data_init(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
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
	vdd->copr.tx_bpw = 8;
	vdd->copr.rx_bpw = 8;
	vdd->copr.tx_size = 1;
	vdd->copr.rx_addr = 0x5A;
	vdd->copr.rx_size = 41;
	vdd->copr.ver = COPR_VER_5P0;
	vdd->copr.display_read = 0;

	ss_copr_init(vdd);
}

static int poc_comp_table[][2] = {
					/*idx  cd */
	{0x53,	0x1A},	/*0 	2 */
	{0x53,	0x1A},	/*1 	3 */
	{0x53,	0x1A},	/*2 	4 */
	{0x53,	0x1A},	/*3 	5 */
	{0x53,	0x1A},	/*4 	6 */
	{0x53,	0x1A},	/*5 	7 */
	{0x53,	0x1A},	/*6 	8 */
	{0x53,	0x1A},	/*7 	9 */
	{0x53,	0x1A},	/*8 	10 */
	{0x53,	0x1A},	/*9 	11 */
	{0x53,	0x1A},	/*10	12 */
	{0x53,	0x1A},	/*11	13 */
	{0x53,	0x1A},	/*12	14 */
	{0x53,	0x1A},	/*13	15 */
	{0x53,	0x1B},	/*14	16 */
	{0x53,	0x1B},	/*15	17 */
	{0x53,	0x1C},	/*16	19 */
	{0x53,	0x1D},	/*17	20 */
	{0x53,	0x1D},	/*18	21 */
	{0x53,	0x1E},	/*19	22 */
	{0x53,	0x1F},	/*20	24 */
	{0x53,	0x1F},	/*21	25 */
	{0x53,	0x20},	/*22	27 */
	{0x53,	0x21},	/*23	29 */
	{0x53,	0x22},	/*24	30 */
	{0x53,	0x23},	/*25	32 */
	{0x53,	0x24},	/*26	34 */
	{0x53,	0x25},	/*27	37 */
	{0x53,	0x26},	/*28	39 */
	{0x53,	0x27},	/*29	41 */
	{0x53,	0x29},	/*30	44 */
	{0x53,	0x2A},	/*31	47 */
	{0x53,	0x2C},	/*32	50 */
	{0x53,	0x2D},	/*33	53 */
	{0x53,	0x2F},	/*34	56 */
	{0x53,	0x31},	/*35	60 */
	{0x53,	0x33},	/*36	64 */
	{0x53,	0x37},	/*37	68 */
	{0x53,	0x3C},	/*38	72 */
	{0x53,	0x42},	/*39	77 */
	{0x53,	0x48},	/*40	82 */
	{0x53,	0x4F},	/*41	87 */
	{0x53,	0x56},	/*42	93 */
	{0x53,	0x5C},	/*43	98 */
	{0x53,	0x61},	/*44	105*/
	{0x53,	0x64},	/*45	111*/
	{0x53,	0x69},	/*46	119*/
	{0x53,	0x6D},	/*47	126*/
	{0x53,	0x71},	/*48	134*/
	{0x53,	0x76},	/*49	143*/
	{0x53,	0x7B},	/*50	152*/
	{0x53,	0x80},	/*51	162*/
	{0x53,	0x85},	/*52	172*/
	{0x53,	0x8B},	/*53	183*/
	{0x53,	0x91},	/*54	195*/
	{0x53,	0x97},	/*55	207*/
	{0x53,	0x9D},	/*56	220*/
	{0x53,	0xA3},	/*57	234*/
	{0x53,	0xAA},	/*58	249*/
	{0x53,	0xB2},	/*59	265*/
	{0x53,	0xBB},	/*60	282*/
	{0x53,	0xC6},	/*61	300*/
	{0x53,	0xCF},	/*62	316*/
	{0x53,	0xD9},	/*63	333*/
	{0x53,	0xE1},	/*64	350*/
	{0x53,	0xE4},	/*65	357*/
	{0x53,	0xE7},	/*66	365*/
	{0x53,	0xEB},	/*67	372*/
	{0x53,	0xEE},	/*68	380*/
	{0x53,	0xF1},	/*69	387*/
	{0x53,	0xF5},	/*70	395*/
	{0x53,	0xF8},	/*71	403*/
	{0x53,	0xFC},	/*72	412*/
	{0x53,	0xFF},	/*73	420*/
	{0x53,	0xFF},	/*74	hbm*/
};

static void poc_comp(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *poc_comp_cmds = ss_get_cmds(vdd, TX_POC_COMP);
	int cd_idx;

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(poc_comp_cmds)) {
		LCD_DEBUG("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)poc_comp_cmds);
		return;
	}

	if (is_hbm_level(vdd))
		cd_idx = ARRAY_SIZE(poc_comp_table) - 1;
	else
		cd_idx = vdd->br_info.common_br.cd_idx;

	LCD_DEBUG("cd_idx (%d) val (%02x %02x)\n", cd_idx, poc_comp_table[cd_idx][0], poc_comp_table[cd_idx][1]);

	poc_comp_cmds->cmds[4].msg.tx_buf[1] = poc_comp_table[cd_idx][0];
	poc_comp_cmds->cmds[4].msg.tx_buf[2] = poc_comp_table[cd_idx][1];

	ss_send_cmd(vdd, TX_POC_COMP);

	return;
}

static int poc_spi_get_status(struct samsung_display_driver_data *vdd)
{
	u8 rxbuf[1];
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	ret = ss_spi_sync(vdd->spi_dev, rxbuf, RX_STATUS_REG1);
	if (ret) {
		LCD_ERR("fail to spi read.. ret (%d) \n", ret);
		return ret;
	}

	LCD_INFO("status : 0x%02x \n", rxbuf[0]);

	return rxbuf[0];
}

static int spi_write_enable_wait(struct samsung_display_driver_data *vdd)
{
	struct spi_device *spi_dev;
	int ret = 0;
	int status = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	spi_dev = vdd->spi_dev;

	if (IS_ERR_OR_NULL(spi_dev)) {
		LCD_ERR("no spi_dev\n");
		return -EINVAL;
	}

	/* BUSY check */
	while ((status = poc_spi_get_status(vdd)) & 0x01) {
		usleep_range(20, 30);
		LCD_ERR("status(0x%02x) is busy.. wait!!\n", status);
	}

	/* Write write enable */
	ss_spi_sync(spi_dev, NULL, TX_WRITE_ENABLE);

	/* WEL check */
	while (!((status = poc_spi_get_status(vdd)) & 0x02)) {
		LCD_ERR("Not ready to write (0x%02x).. wait!!\n", status);
		ss_spi_sync(spi_dev, NULL, TX_WRITE_ENABLE);
		usleep_range(20, 30);
	}

	return ret;
}

static int poc_spi_erase(struct samsung_display_driver_data *vdd, u32 erase_pos, u32 erase_size, u32 target_pos)
{
	struct spi_device *spi_dev;
	struct ddi_spi_cmd_set *cmd_set = NULL;
	int image_size = 0;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	spi_dev = vdd->spi_dev;

	if (IS_ERR_OR_NULL(spi_dev)) {
		LCD_ERR("no spi_dev\n");
		return -EINVAL;
	}

	image_size = vdd->poc_driver.image_size;

	cmd_set = ss_get_spi_cmds(vdd, TX_ERASE);
	if (cmd_set == NULL) {
		LCD_ERR("cmd_set is null..\n");
		return -EINVAL;
	}

	/* start */
	spi_write_enable_wait(vdd);
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG1);
	spi_write_enable_wait(vdd);
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG2);

	spi_write_enable_wait(vdd);

	/* spi erase */
	if (erase_size == POC_ERASE_64KB)
		cmd_set->tx_buf[0] = SPI_64K_ERASE_CMD;
	else if (erase_size == POC_ERASE_32KB)
		cmd_set->tx_buf[0] = SPI_32K_ERASE_CMD;
	else
		cmd_set->tx_buf[0] = SPI_SECTOR_ERASE_CMD;

	cmd_set->tx_addr = erase_pos;

	LCD_INFO("[ERASE] (%6d / %6d), erase_size (%d) addr (%06x) %02x\n",
		erase_pos, target_pos, erase_size, cmd_set->tx_addr, cmd_set->tx_buf[0]);

	ss_spi_sync(spi_dev, NULL, TX_ERASE);

	/* end */
	spi_write_enable_wait(vdd);
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG1_END);
	spi_write_enable_wait(vdd);
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG2_END);

	return ret;
}

static int poc_spi_write(struct samsung_display_driver_data *vdd, u8 *data, u32 write_pos, u32 write_size)
{
	struct spi_device *spi_dev;
	struct ddi_spi_cmd_set *cmd_set = NULL;
	int pos, ret = 0;
	int last_pos, image_size, loop_cnt, poc_w_size;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	spi_dev = vdd->spi_dev;

	if (IS_ERR_OR_NULL(spi_dev)) {
		LCD_ERR("no spi_dev\n");
		return -EINVAL;
	}

	image_size = vdd->poc_driver.image_size;
	last_pos = write_pos + write_size;
	poc_w_size = vdd->poc_driver.write_data_size;
	loop_cnt = vdd->poc_driver.write_loop_cnt;

	cmd_set = ss_get_spi_cmds(vdd, TX_WRITE_PAGE_PROGRAM);
	if (cmd_set == NULL) {
		LCD_ERR("cmd_set is null..\n");
		return -EINVAL;
	}

	LCD_INFO("[WRITE] write_pos : %6d, write_size : %6d, last_pos %6d, poc_w_size : %d\n",
		write_pos, write_size, last_pos, poc_w_size);

	/* start */
	spi_write_enable_wait(vdd);
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG1);
	spi_write_enable_wait(vdd);
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG2);

	for (pos = write_pos; pos < last_pos; ) {
		if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
			LCD_ERR("cancel poc write by user\n");
			ret = -EIO;
			goto cancel_poc;
		}

		cmd_set->tx_addr = pos;

		memcpy(&cmd_set->tx_buf[4], &data[pos], cmd_set->tx_size - 3);

		spi_write_enable_wait(vdd);

		/* spi write */
		ss_spi_sync(spi_dev, NULL, TX_WRITE_PAGE_PROGRAM);

		pos += poc_w_size;
	}

cancel_poc:
	if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
		LCD_ERR("cancel poc write by user\n");
		atomic_set(&vdd->poc_driver.cancel, 0);
		ret = -EIO;
	}

	/* end */
	spi_write_enable_wait(vdd);
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG1_END);
	spi_write_enable_wait(vdd);
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG2_END);

	return ret;
}

static int poc_spi_read(struct samsung_display_driver_data *vdd, u8 *buf, u32 read_pos, u32 read_size)
{
	struct spi_device *spi_dev;
	struct ddi_spi_cmd_set *cmd_set = NULL;
	u8 *rbuf;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	spi_dev = vdd->spi_dev;

	if (IS_ERR_OR_NULL(spi_dev)) {
		LCD_ERR("no spi_dev\n");
		return -EINVAL;
	}

	cmd_set = ss_get_spi_cmds(vdd, RX_DATA);
	if (cmd_set == NULL) {
		LCD_ERR("cmd_set is null..\n");
		return -EINVAL;
	}

	LCD_INFO("[READ] read_pos : %6d, read_size : %6d\n", read_pos, read_size);

	rbuf = kmalloc(read_size, GFP_KERNEL | GFP_DMA);
	if (rbuf == NULL) {
		LCD_ERR("fail to alloc rbuf..\n");
		goto cancel_poc;
	}

	cmd_set->rx_size = read_size;
	cmd_set->rx_addr = read_pos;

	if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
		LCD_ERR("cancel poc read by user\n");
		ret = -EIO;
		goto cancel_poc;
	}

	/* spi read */
	ret = ss_spi_sync(spi_dev, rbuf, RX_DATA);

	/* copy to buf */
	memcpy(&buf[read_pos], rbuf, read_size);

	/* rx_buf reset */
	memset(rbuf, 0, read_size);

	if (!(read_pos % DEBUG_POC_CNT))
		LCD_INFO("buf[%d] = 0x%x\n", read_pos, buf[read_pos]);

cancel_poc:
	if (rbuf)
		kfree(rbuf);

	return ret;
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

	/* Bootloader: WQHD@60hz Normal mode */
	vrr->cur_refresh_rate = vrr->adjusted_refresh_rate = 60;
	vrr->cur_sot_hs_mode = vrr->adjusted_sot_hs_mode = false;
	vrr->max_h_active_support_120hs = 1080; /* supports 120hz until FHD 1080 */

	vrr->hs_nm_seq = HS_NM_OFF;
	vrr->delayed_perf_normal = false;
	vrr->skip_vrr_in_brightness = false;

	vrr->vrr_workqueue = create_singlethread_workqueue("vrr_workqueue");
	INIT_WORK(&vrr->vrr_work, ss_panel_vrr_switch_work);

	vrr->brr_mode = BRR_OFF_MODE;

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

static bool ss_check_support_mode(struct samsung_display_driver_data *vdd, enum CHECK_SUPPORT_MODE mode)
{
	bool is_support = true;
	int cur_rr = vdd->vrr.cur_refresh_rate;
	bool cur_hs = vdd->vrr.cur_sot_hs_mode;

	switch (mode) {
	case CHECK_SUPPORT_MCD:
		if (!cur_hs) {
			is_support = false;
			LCD_ERR("MCD fail: supported on HS(cur: %dNS)\n",
					cur_rr);
		}
		break;

	case CHECK_SUPPORT_HMD:
		if (!(cur_rr == 60 && !cur_hs)) {
			is_support = false;
			LCD_ERR("HMD fail: supported on 60NS(cur: %d%s)\n",
					cur_rr, cur_hs ? "HS" : "NS");
		}
		break;

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
	LCD_INFO("%s\n", ss_get_panel_name(vdd));

	/* Default Panel Power Status is OFF */
	vdd->panel_state = PANEL_PWR_OFF;

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = samsung_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = samsung_panel_on_post;
	vdd->panel_func.samsung_panel_off_pre = samsung_panel_off_pre;
	vdd->panel_func.samsung_panel_off_post = samsung_panel_off_post;

	/* DDI RX */
	vdd->panel_func.samsung_panel_revision = ss_panel_revision;
	vdd->panel_func.samsung_module_info_read = ss_module_info_read;
	vdd->panel_func.samsung_ddi_id_read = ss_ddi_id_read;
	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;
	vdd->panel_func.samsung_elvss_read = NULL;

	/* Brightness */
	vdd->panel_func.samsung_brightness_gamma = ss_brightness_gamma_mode2_normal;
	vdd->panel_func.samsung_brightness_acl_on = ss_acl_on;
	vdd->panel_func.samsung_brightness_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_hbm_off = NULL;
	vdd->panel_func.samsung_brightness_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_elvss = NULL;
	vdd->panel_func.samsung_brightness_vint = ss_vint;
	vdd->panel_func.samsung_brightness_vrr = ss_vrr;
	vdd->panel_func.samsung_brightness_gm2_gamma_comp = ss_brightness_gm2_gamma_comp;

	vdd->br_info.smart_dimming_loaded_dsi = false;

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

	/* HMT */
	vdd->panel_func.samsung_brightness_hmt = ss_brightness_gamma_mode2_hmt;
	vdd->panel_func.samsung_brightness_vrr_hmt = ss_vrr_hmt;

	/* Panel LPM */
	vdd->panel_func.samsung_update_lpm_ctrl_cmd = ss_update_panel_lpm_ctrl_cmd;
	vdd->panel_func.samsung_set_lpm_brightness = ss_set_panel_lpm_brightness;

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
//	vdd->debug_data->panic_on_pptimeout = true;

	/* COLOR WEAKNESS */

	/* Support DDI HW CURSOR */

	/* COVER Open/Close */

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
	vdd->self_disp.init = self_display_init_HAC;
	vdd->self_disp.data_init = ss_self_display_data_init;

	/* mAPFC */
	vdd->mafpc.init = ss_mafpc_init_HAC;
	vdd->mafpc.data_init = ss_mafpc_data_init;

	/* stm */
	vdd->stm.stm_on = 1;

	/* POC */
	vdd->poc_driver.poc_erase = poc_spi_erase;
	vdd->poc_driver.poc_write = poc_spi_write;
	vdd->poc_driver.poc_read = poc_spi_read;
	vdd->poc_driver.poc_comp = poc_comp;

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
	char panel_string[] = "ss_dsi_panel_S6E3HAC_AMB687VX01_WQHD";
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
