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
#include "ss_dsi_panel_S6E3HAB_AMB623TS01.h"
#include "ss_dsi_mdnie_S6E3HAB_AMB623TS01.h"
#include "ss_dsi_interpolation_S6E3HAB_AMB623TS01.h"

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

/* Register to control brightness level */
#define ALPM_REG	0x53
/* Register to cnotrol ALPM/HLPM mode */
#define ALPM_CTRL_REG	0xBB
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

	if (vdd->self_disp.is_support && vdd->self_disp.self_mask_on)
		vdd->self_disp.self_mask_on(vdd, true);

	/* mafpc */
	if (vdd->mafpc.is_support) {
		if (!vdd->samsung_splash_enabled) {
			vdd->mafpc.need_to_write = true;
			LCD_INFO("Need to write mafpc image data to DDI\n");
		} else {
			LCD_INFO("Samsung splash enabled.. skip mafpc write\n");
		}
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
	case 0x0 ... 0x1:
		/* Rev.A: OSC 96.5Mhz, do not support 60 HS mode, should not send LTPS (CBh)  */
		vdd->panel_revision = 'A';
		break;
	case 0x41:
		/* Rev.B: OSC 86.4Mhz, do not support 60 HS mode, should not send LTPS (CBh) */
		vdd->panel_revision = 'B';
		break;
	case 0x42:
		/* Rev.C: OSC 86.4Mhz, support 60 HS mode, should send LTPS (CBh) for 86.4Mhz */
		vdd->panel_revision = 'C';
		break;
	case 0x03:
		/* Rev.D: OSC 96.5Mhz, support 60 HS mode, should send LTPS (CBh) for 96.5Mhz  */
		vdd->panel_revision = 'D';
		break;
	default:
		vdd->panel_revision = 'D';
		LCD_ERR("Invalid panel_rev(default rev : %c)\n",
				vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';

	LCD_INFO_ONCE("panel_revision = %c %d \n",
			vdd->panel_revision + 'A', vdd->panel_revision);

	/* SW W/A: support panel recovery by FG_ERR interrupt under panel revision D (ID3=0x42).
	 * Remove FG_ERR interrupt gpio if panel revision is over than revision D.
	 * Requested by Dlab.
	 */
	if (vdd->manufacture_id_dsi != -EINVAL &&
			vdd->panel_revision + 'A' >= 'D' &&
			vdd->esd_recovery.num_of_gpio > 1) {
		LCD_INFO("W/A: prevent FG_ERR recovery support over than ID3 0x42, num_of_gpio: %d\n",
				vdd->esd_recovery.num_of_gpio);
		vdd->esd_recovery.num_of_gpio--;
	}

	return (vdd->panel_revision + 'A');
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

static struct dsi_panel_cmd_set *ss_hbm_gamma(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_gamma_cmds = ss_get_cmds(vdd, TX_HBM_GAMMA);
	struct brightness_table *br_tbl = ss_get_cur_br_tbl(vdd);

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(hbm_gamma_cmds)) {
		LCD_ERR("Invalid data  vdd : 0x%zx cmd : 0x%zx", (size_t)vdd, (size_t)hbm_gamma_cmds);
		return NULL;
	}

	if (vdd->vrr.black_frame_mode == BLACK_FRAME_INSERT) {
		/* HS <-> Normal mode: insert one black frame to hide screen noise
		 * ss_gamma() set gamma MTP (C8h) to zero.
		 * None zero gamma (CAh) setting causes greenish screen issue during MTP(C8h) is zero.
		 * So, gamma value should be set to zero while black_frame_mode is BLACK_FRAME_INSERT mode.
		 */

		LCD_INFO("insert black frame gamma (%dhz %s)\n",
				br_tbl->refresh_rate, br_tbl->is_sot_hs_mode ? "HS" : "NM");

		/* send all zero gamma (CAh) */
		memset(&hbm_gamma_cmds->cmds[1].msg.tx_buf[1], 0, GAMMA_SIZE);

		/* send all zero gamma MTP (C8h) */
		memset(&hbm_gamma_cmds->cmds[2].msg.tx_buf[1], 0, GAMMA_SIZE);
		hbm_gamma_cmds->cmds[2].msg.tx_buf[0] = 0xC8;
	} else if (vdd->vrr.black_frame_mode == BLACK_FRAME_REMOVE) {
		vdd->vrr.black_frame_mode = BLACK_FRAME_OFF;

		LCD_INFO("remove black frame gamma (%dhz %s)\n",
				br_tbl->refresh_rate, br_tbl->is_sot_hs_mode ? "HS" : "NM");

		/* send original gamma (CAh) */
		br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_GAMMA, &hbm_gamma_cmds->cmds[1].msg.tx_buf[1]);

		/* send original gamma MTP (C8h) */
		memcpy(&hbm_gamma_cmds->cmds[2].msg.tx_buf[1], br_tbl->gamma_tbl->c8_register.mtp_data, GAMMA_SIZE);
		hbm_gamma_cmds->cmds[2].msg.tx_buf[0] = 0xC8;
	} else {
		/* send original gamma (CAh) */
		br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_GAMMA, &hbm_gamma_cmds->cmds[1].msg.tx_buf[1]);
		/* do not touch C8h gamma MTP, send NULL packet */
		hbm_gamma_cmds->cmds[2].msg.tx_buf[0] = 0x00;
	}

	return hbm_gamma_cmds;
}

static struct dsi_panel_cmd_set *ss_hbm_etc(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_etc_cmds = ss_get_cmds(vdd, TX_HBM_ETC);
	int cur_rr = vdd->vrr.cur_refresh_rate;
	bool cur_hs = vdd->vrr.cur_sot_hs_mode;

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(hbm_etc_cmds)) {
		LCD_ERR("Invalid data  vdd : 0x%zx cmd : 0x%zx", (size_t)vdd, (size_t)hbm_etc_cmds);
		return NULL;
	}

	/* 0. HBM Cycle:  48/60Hz normal: 0x21, 60/96/120Hz HS: 0x11 */
	hbm_etc_cmds->cmds[0].msg.tx_buf[1] = cur_hs ? 0x11 : 0x21;

	/* 2. HBM AOR: B1h 3~4th, 5~6th */
	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_AOR, &hbm_etc_cmds->cmds[2].msg.tx_buf[1]);
	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_AOR, &hbm_etc_cmds->cmds[2].msg.tx_buf[3]);

	/* ELVSS setting is implemented in vrr cmds */

	/* 3. Manual DBV Setting (fixed HBM value) */

	/* 4, 5. VINT */
	/* 48/60NM/60HS: 5th (off=4), 96/120hz: 29th (off=28) */
	hbm_etc_cmds->cmds[4].msg.tx_buf[1] = (cur_rr <= 60) ? 4 : 28;
	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_VINT, &hbm_etc_cmds->cmds[5].msg.tx_buf[1]);

	/* 6. VGH level control for xtalk mode, VGH 7.0V: 0x30 */
	hbm_etc_cmds->cmds[6].msg.tx_buf[1] = 0x30;

	/* 7. ACL control, 8. ACL on/off
	 *  8%: 0x26, 15%: 0x55
	 */
	if (vdd->br_info.gradual_acl_val) {
		hbm_etc_cmds->cmds[7].msg.tx_buf[6] = 0x26;	/* 8% */
		hbm_etc_cmds->cmds[8].msg.tx_buf[1] = 0x2;	/* ACL ON */
		hbm_etc_cmds->cmds[7].msg.tx_buf[2] = 0x55;
	} else {
		hbm_etc_cmds->cmds[7].msg.tx_buf[6] = 0x0;	/* 0% */
		hbm_etc_cmds->cmds[8].msg.tx_buf[1] = 0x0;	/* ACL OFF */
		hbm_etc_cmds->cmds[7].msg.tx_buf[2] = 0x44;
	}

	LCD_INFO("gradual: %d, %dhz %s\n", vdd->br_info.gradual_acl_val, cur_rr, cur_hs ? "HS" : "NM" );

	return hbm_etc_cmds;
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

		LCD_ERR("manufacture_date DSI%d = (%d%04d) - year(%d) month(%d) day(%d) hour(%d) min(%d)\n",
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

static int ss_smart_dimming_init(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	struct smartdim_conf *sconf;

	sconf = vdd->panel_func.samsung_smart_get_conf();
	if (IS_ERR_OR_NULL(sconf)) {
		LCD_ERR("fail to get smartdim_conf (ndx: %d)\n", vdd->ndx);
		return false;
	}
	br_tbl->smart_dimming_dsi = sconf;

	/* Read ELVSS MTP */
	ss_panel_data_read(vdd, RX_ELVSS, vdd->br_info.common_br.elvss_value, LEVEL1_KEY);

	/* Read HBM MTP */
	sconf->hbm_payload = kzalloc(vdd->br_info.gamma_size, GFP_KERNEL);
	if (!sconf->hbm_payload) {
		LCD_ERR("fail to allocate hbm_payload memory\n");
		return -ENOMEM;
	}

	ss_panel_data_read(vdd, RX_CENTER_GAMMA_60HS, sconf->center_gamma_60hs, LEVEL1_KEY);	/* C8h 112th ~ 144th */
	ss_panel_data_read(vdd, RX_CENTER_GAMMA_120HS, sconf->center_gamma_120hs, LEVEL1_KEY); 	/* C9h 38th ~ 70th */

	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, sconf->mtp_buffer, LEVEL1_KEY);	/* 60/120 C8h 1st ~ 34th */
	ss_panel_data_read(vdd, RX_HBM, sconf->hbm_payload, LEVEL1_KEY);	/* 60 NR B3h 6th ~ 39th */

	/* overwrite some values from default (60 normal) read values */
	if (br_tbl->refresh_rate == 120 || br_tbl->refresh_rate == 96) {
		/* hbm gamma[0~30]: C9h 75th~105th, hbm gamma[31~33] B3h 37th~39th */
		ss_panel_data_read(vdd, RX_HBM2, sconf->hbm_payload, LEVEL1_KEY); /* C9h 75th~105th */
	} else if (br_tbl->refresh_rate == 60 && br_tbl->is_sot_hs_mode) {
		/* hbm gamma[0~30]: C9h 112th~142th, hbm gamma[31~33] B3h 37th~39th */
		ss_panel_data_read(vdd, RX_HBM3, sconf->hbm_payload, LEVEL1_KEY); /* C9h 112th~142th */
	}

	/* Initialize smart dimming related things here */
	/* lux_tab setting for 350cd */
	sconf->lux_tab = vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].cd;
	sconf->lux_tabsize = vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].tab_size;
	sconf->man_id = vdd->manufacture_id_dsi;
	sconf->rr = br_tbl->refresh_rate;
	sconf->sot_hs = br_tbl->is_sot_hs_mode;

	if (vdd->panel_func.samsung_panel_revision)
		sconf->panel_revision = vdd->panel_func.samsung_panel_revision(vdd);

	/* Just a safety check to ensure smart dimming data is initialised well */
	sconf->init(sconf);

	vdd->br_info.temperature = 20; // default temperature

	vdd->br_info.smart_dimming_loaded_dsi = true;

	LCD_INFO("DSI%d, FPS: %d, HS: %d --\n", vdd->ndx,
			br_tbl->refresh_rate, br_tbl->is_sot_hs_mode);

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

static struct dsi_panel_cmd_set *ss_aid(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *aid_cmds = ss_get_cmds(vdd, TX_PAC_AID_SUBDIVISION);
	int cd_index = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (IS_ERR_OR_NULL(aid_cmds)) {
		LCD_ERR("No aid_tx_cmds\n");
		return NULL;
	}

	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_AOR, &aid_cmds->cmds->msg.tx_buf[1]);

	LCD_INFO("VRR: [%d] level(%d), aid(%x %x), cur: %dhz%s, brr: %s\n",
			cd_index, vdd->br_info.common_br.bl_level,
			aid_cmds->cmds->msg.tx_buf[1],
			aid_cmds->cmds->msg.tx_buf[2],
			vdd->vrr.cur_refresh_rate,
			vdd->vrr.cur_sot_hs_mode ? "HS" : "NM",
			ss_get_brr_mode_name(vdd->vrr.brr_mode));

	return aid_cmds;
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

	LCD_INFO("gradual_acl: %d, acl per: 0x%x",
			vdd->br_info.gradual_acl_val, pcmds->cmds[0].msg.tx_buf[6]);

	return pcmds;
}

static struct dsi_panel_cmd_set *ss_acl_off(struct samsung_display_driver_data *vdd, int *level_key)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	return ss_get_cmds(vdd, TX_ACL_OFF);
}

#if 0
static struct dsi_panel_cmd_set *ss_elvss(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *elvss_cmds = ss_get_cmds(vdd, TX_ELVSS);
	u8 elvss_75th_val;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(elvss_cmds)) {
		LCD_ERR("No elvss_tx_cmds\n");
		return NULL;
	}

	/*	0xB5 1th TSET
		BIT(7) is signed bit.
		BIT(6) ~ BIT(0) is real temperature.
	*/
	elvss_cmds->cmds[0].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
			vdd->br_info.temperature : (char)(BIT(7) | (-1*vdd->br_info.temperature));

	/* 0xB5 2th MSP */
	if (vdd->br_info.common_br.cd_level > 39)
		elvss_cmds->cmds[0].msg.tx_buf[2] = 0xDC;
	else
		elvss_cmds->cmds[0].msg.tx_buf[2] = 0xCC;

	/* ELVSS Compensation for Low Temperature & Low Birghtness*/
	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_ELVSS, &elvss_cmds->cmds[0].msg.tx_buf[3]);

	/* 0xB5 elvss_75th_val elvss_cal_offset */
	elvss_75th_val = vdd->br_info.common_br.elvss_value[0];
	elvss_cmds->cmds[2].msg.tx_buf[1] = elvss_75th_val;

	return elvss_cmds;
}
#endif

static struct dsi_panel_cmd_set *ss_dbv(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *dbv_cmds = ss_get_cmds(vdd, TX_MANUAL_DBV);

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(dbv_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)dbv_cmds);
		return NULL;
	}

	br_interpolation_generate_event(vdd, GEN_NORMAL_DBV, &dbv_cmds->cmds[0].msg.tx_buf[1]);

	LCD_DEBUG("DBV cmd: %X %X\n", dbv_cmds->cmds[0].msg.tx_buf[1], dbv_cmds->cmds[0].msg.tx_buf[2]);

	return dbv_cmds;
}

static struct dsi_panel_cmd_set *ss_vint(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *vint_cmds = ss_get_cmds(vdd, TX_VINT);
	int cur_rr = vdd->vrr.cur_refresh_rate;

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(vint_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)vint_cmds);
		return NULL;
	}

	/* 48/60NM/60HS: 5th (off=4), 96/120hz: 29th (off=28) */
	vint_cmds->cmds[0].msg.tx_buf[1] = (cur_rr <= 60) ? 4 : 28;

	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_VINT, &vint_cmds->cmds[1].msg.tx_buf[1]);

	if (vdd->xtalk_mode)
		vint_cmds->cmds[2].msg.tx_buf[1] = 0x28;	/* VGH 6.4 V */
	else
		vint_cmds->cmds[2].msg.tx_buf[1] = 0x30;	/* VGH 7.0 V */

	LCD_INFO("%dhz %s\n", cur_rr, vdd->vrr.cur_sot_hs_mode ? "HS" : "NM");

	return vint_cmds;
}

static u8 cmd_moderato_mode[4] = {0x65, 0xFF, 0x01, 0x00};
static u8 cmd_flat_mode[4] = {0x25, 0xD1, 0x02, 0x2D};

static struct dsi_panel_cmd_set *ss_irc(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *irc_cmds = ss_get_cmds(vdd, TX_PAC_IRC_SUBDIVISION);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx",	(size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(irc_cmds)) {
		LCD_ERR("No irc_subdivision_tx_cmds\n");
		return NULL;
	}

	if (!vdd->br_info.common_br.support_irc)
		return NULL;

	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_IRC, &irc_cmds->cmds[1].msg.tx_buf[1]);

	/* set irc mode to moderato or flat gamma */
	if (vdd->br_info.common_br.irc_mode == IRC_MODERATO_MODE)
		memcpy(&irc_cmds->cmds[1].msg.tx_buf[2], cmd_moderato_mode, sizeof(cmd_moderato_mode));
	else if (vdd->br_info.common_br.irc_mode == IRC_FLAT_GAMMA_MODE)
		memcpy(&irc_cmds->cmds[1].msg.tx_buf[2], cmd_flat_mode, sizeof(cmd_flat_mode));
	else
		LCD_ERR("invalid irc mode(%d)\n", vdd->br_info.common_br.irc_mode);

	return irc_cmds;
}

static struct dsi_panel_cmd_set *ss_hbm_irc(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_irc_cmds = ss_get_cmds(vdd, TX_HBM_IRC);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(hbm_irc_cmds)) {
		LCD_ERR("No irc_tx_cmds\n");
		return NULL;
	}

	if (!vdd->br_info.common_br.support_irc)
		return NULL;

	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_IRC, &hbm_irc_cmds->cmds[1].msg.tx_buf[1]);

	/* set irc mode to moderato or flat gamma */
	if (vdd->br_info.common_br.irc_mode == IRC_MODERATO_MODE)
		memcpy(&hbm_irc_cmds->cmds[1].msg.tx_buf[2], cmd_moderato_mode, sizeof(cmd_moderato_mode));
	else if (vdd->br_info.common_br.irc_mode == IRC_FLAT_GAMMA_MODE)
		memcpy(&hbm_irc_cmds->cmds[1].msg.tx_buf[2], cmd_flat_mode, sizeof(cmd_flat_mode));
	else
		LCD_ERR("invalid irc mode(%d)\n", vdd->br_info.common_br.irc_mode);

	return hbm_irc_cmds;
}

static struct dsi_panel_cmd_set *ss_gamma(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set  *gamma_cmds = ss_get_cmds(vdd, TX_GAMMA);
	struct brightness_table *br_tbl = ss_get_cur_br_tbl(vdd);

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(gamma_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)gamma_cmds);
		return NULL;
	}

	LCD_DEBUG("bl_level : %d candela : %dCD\n", vdd->br_info.common_br.bl_level, vdd->br_info.common_br.cd_level);

	if (vdd->vrr.black_frame_mode == BLACK_FRAME_INSERT) {
		/* HS <-> Normal mode: insert one black frame to hide screen noise
		 * ss_gamma() set gamma MTP (C8h) to zero.
		 * None zero gamma (CAh) setting causes greenish screen issue during MTP(C8h) is zero.
		 * So, gamma value should be set to zero while black_frame_mode is BLACK_FRAME_INSERT mode.
		 */

		LCD_INFO("VRR: insert black frame gamma (%dhz %s)\n",
				br_tbl->refresh_rate, br_tbl->is_sot_hs_mode ? "HS" : "NM");

		/* send all zero gamma (CAh) */
		memset(&gamma_cmds->cmds[0].msg.tx_buf[1], 0, GAMMA_SIZE);

		/* send all zero gamma MTP (C8h) */
		memset(&gamma_cmds->cmds[1].msg.tx_buf[1], 0, GAMMA_SIZE);
		gamma_cmds->cmds[1].msg.tx_buf[0] = 0xC8;
		gamma_cmds->count = 2;
	} else if (vdd->vrr.black_frame_mode == BLACK_FRAME_REMOVE) {
		vdd->vrr.black_frame_mode = BLACK_FRAME_OFF;

		LCD_INFO("VRR: remove black frame gamma (%dhz %s)\n",
				br_tbl->refresh_rate, br_tbl->is_sot_hs_mode ? "HS" : "NM");

		/* send original gamma (CAh) */
		br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_GAMMA, &gamma_cmds->cmds[0].msg.tx_buf[1]);

		/* send original gamma MTP (C8h) */
		memcpy(&gamma_cmds->cmds[1].msg.tx_buf[1], br_tbl->gamma_tbl->c8_register.mtp_data, GAMMA_SIZE);
		gamma_cmds->cmds[1].msg.tx_buf[0] = 0xC8;
		gamma_cmds->count = 2;
	} else {
		/* send original gamma (CAh) */
		br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_GAMMA, &gamma_cmds->cmds[0].msg.tx_buf[1]);
		/* do not touch C8h gamma MTP */
		gamma_cmds->cmds[1].msg.tx_buf[0] = 0x00;
		gamma_cmds->count = 1;
	}
	LCD_INFO("VRR: cur: %dhz%s, brr: %s, cmd count: %d\n",
			vdd->vrr.cur_refresh_rate,
			vdd->vrr.cur_sot_hs_mode ? "HS" : "NM",
			ss_get_brr_mode_name(vdd->vrr.brr_mode),
			gamma_cmds->count);

	return gamma_cmds;
}

static struct dsi_panel_cmd_set *ss_ltps(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set  *ltps_cmds = ss_get_cmds(vdd, TX_PANEL_LTPS);

	if (SS_IS_CMDS_NULL(ltps_cmds)) {
		LCD_DEBUG("no LTPS cmds for old panel(rev: %c)\n", vdd->panel_revision + 'A');
		return NULL;
	}

	LCD_INFO("TX LTPS cmd: rev: %c, RR: %dHz, HS: %d\n",
			vdd->panel_revision + 'A',
			vdd->vrr.cur_refresh_rate, vdd->vrr.cur_sot_hs_mode);

	return ltps_cmds;
}

enum VRR_CMD_ID {
	VRR_CMDID_AID = 2,
	VRR_CMDID_OSC = 3,
	VRR_CMDID_POWERGEN = 5,
	VRR_CMDID_SRCAMP = 7,
	VRR_CMDID_MPS_ELVSS = 8,
	VRR_CMDID_ELVSS_CAL = 10,
	VRR_CMDID_HBM = 11,
	VRR_CMDID_FQ = 12,
	VRR_CMDID_VFP_GPARA_DFT = 13,
	VRR_CMDID_VFP_GPARA = 15,
	VRR_CMDID_VFP = 16,
};

static struct dsi_panel_cmd_set *__ss_vrr(struct samsung_display_driver_data *vdd,
					int *level_key, bool is_hbm, bool is_hmt)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_panel_cmd_set  *vrr_cmds = ss_get_cmds(vdd, TX_VRR);

	struct vrr_info *vrr = &vdd->vrr;
	enum SS_BRR_MODE brr_mode = vrr->brr_mode;

	struct vrr_bridge_rr_tbl *brr;
	int vfp_target, vbp_target, vactive_target, fps_brr = 0;
	int vfp_base, vbp_base, vactive_base, fps_base;

	int cur_rr;
	bool cur_hs;

	/* skip VRR setting in brightness command (ss_vrr() funciton)
	 * 1) start waiting SSD improve and HS_NM change
	 * 2) triger brightness setting in other thread, and change fps
	 *    before finishing SSD/HS_NM waiting...
	 *    In result, it causes side effect, like screen noise or stuck..
	 * 3) finish waiting SSD improve and HS_NM change...
	 */
	if (vrr->skip_vrr_in_brightness) {
		LCD_INFO("skip to tx vrr cmd\n");
		return NULL;
	}

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

	if (ss_is_brr_on(brr_mode)) {
		/* Bridge RR mode */

		/* In BRR mode fps_brr (current bridge FPS) is used for only VFP calculation.
		 * The other VRR factors refers base refresh mode (60hz or 120hz mode).
		 */
		brr = &vrr->brr_tbl[brr_mode];

		vbp_target = brr->vbp_base;
		vactive_target = brr->vactive_base;
		fps_brr = vrr->cur_refresh_rate;

		vfp_base = brr->vfp_base;
		vbp_base = brr->vbp_base;
		vactive_base = brr->vactive_base;
		fps_base = brr->fps_base;

		vfp_target = ((vbp_base + vactive_base + vfp_base) * fps_base / fps_brr) -
				(vbp_target + vactive_target);

		cur_rr = fps_base;
		cur_hs = brr->sot_hs_base;
	} else {
		cur_rr = vrr->cur_refresh_rate;
		cur_hs = vrr->cur_sot_hs_mode;

		if (cur_rr == 48 || cur_rr == 96)
			vfp_target = 824;
		else if (cur_rr == 60 && cur_hs)	/* 60hz HS */
			vfp_target = 3248;
		else	/* 60hzNM, 120hzHS*/
			vfp_target = 16;
	}

	if (is_hmt && !(cur_rr == 60 && !cur_hs))
		LCD_ERR("error: HMT not in 60hz NM mode, cur: %dhz%s\n",
				cur_rr, cur_hs ? "HS" : "NM");

	/* 2. AID: HS: 0x40 (AOR cycle=2), Normal: 0x80 (AOR cycle=4), HMT: 0x00 (AOR cycle=1) */
	if (is_hmt) {
		vrr_cmds->cmds[VRR_CMDID_AID].msg.tx_buf[1] = 0x00;
	} else {
		vrr_cmds->cmds[VRR_CMDID_AID].msg.tx_buf[1] = cur_hs ? 0x40 : 0x80;

		if ((get_lcd_attached("GET") & 0xF) >= 3) {
			if (fps_brr == 70 || fps_brr == 80 || fps_brr == 100 || fps_brr == 110 ||
					fps_brr == 120) {
				/* In case of 60HS <-> 120HS BRR mode,
				 * all bridge FPS (70, 100, 110, and 120hz) mode
				 * should set AOR cycle 4 (0x80), which is for normal mode,
				 * to avoid brightness flicker. (requested by dlab.)
				 */
				vrr_cmds->cmds[VRR_CMDID_AID].msg.tx_buf[1] = 0x80;
			}
		} else {
			if (fps_brr == 70 || fps_brr == 80) {
				/* In case of 70 and 80hz, AID be set to 0x80 (AOR cycle=4)
				 * even it is HS mode... (requested by dlab)
				 */
				vrr_cmds->cmds[VRR_CMDID_AID].msg.tx_buf[1] = 0x80;
			}
		}
	}

	/* 3. OSC: HS: 0xC0, Normal: 0xC2 */
	vrr_cmds->cmds[VRR_CMDID_OSC].msg.tx_buf[1] = cur_hs ? 0xC0 : 0xC2;

	/* 5. power gen done: 48/96hz: 0xC0, 60/120hz/bridge FPS: 0x00 */
	if (cur_rr == 48 || cur_rr == 96)
		vrr_cmds->cmds[VRR_CMDID_POWERGEN].msg.tx_buf[1] = 0xC0;
	else
		vrr_cmds->cmds[VRR_CMDID_POWERGEN].msg.tx_buf[1] = 0x00;

	/* 7. source AMP: 60/120hz: on(0x01), 48/96hz: off(0x41) */
	if (cur_rr == 60 || cur_rr == 120)
		vrr_cmds->cmds[VRR_CMDID_SRCAMP].msg.tx_buf[1] = 0x1;
	else
		vrr_cmds->cmds[VRR_CMDID_SRCAMP].msg.tx_buf[1] = 0x41;

	/* 8. ELVSS - restriction : TE -> ELVSS -> 53h/F7h -> TE */
	vrr_cmds->cmds[VRR_CMDID_MPS_ELVSS].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
			vdd->br_info.temperature : (char)(BIT(7) | (-1*vdd->br_info.temperature));
	if (is_hbm) {
		vrr_cmds->cmds[VRR_CMDID_MPS_ELVSS].msg.tx_buf[2] = 0xDC; /* 412cd ~ HBM is 0xDC */
		br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_ELVSS, &vrr_cmds->cmds[VRR_CMDID_MPS_ELVSS].msg.tx_buf[3]);
		/* 10. ELVSS: read B5h 76th -> write B5h 75h */
		vrr_cmds->cmds[VRR_CMDID_ELVSS_CAL].msg.tx_buf[1] = vdd->br_info.common_br.elvss_value[1];
	} else {
		if (vdd->br_info.common_br.cd_level > 39)
			vrr_cmds->cmds[VRR_CMDID_MPS_ELVSS].msg.tx_buf[2] = 0xDC;
		else
			vrr_cmds->cmds[VRR_CMDID_MPS_ELVSS].msg.tx_buf[2] = 0xCC;
		br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_ELVSS, &vrr_cmds->cmds[VRR_CMDID_MPS_ELVSS].msg.tx_buf[3]);
		/* 10. ELVSS: read B5h 75th -> write B5h 75h : OTP Value */
		vrr_cmds->cmds[VRR_CMDID_ELVSS_CAL].msg.tx_buf[1] = vdd->br_info.common_br.elvss_value[0];
	}

	/* 11. HBM: on: 0xC0, off: 0 */
	vrr_cmds->cmds[VRR_CMDID_HBM].msg.tx_buf[1] = is_hbm ? 0xC0 : 0x00;

	/* 12. Frequency setting: 48/60hz: 0x00, 96/120hz: 0x20 */
	vrr_cmds->cmds[VRR_CMDID_FQ].msg.tx_buf[1] = (cur_rr <= 60) ? 0x00 : 0x20;

	/* 13. gpara: VFP off: 48/60hz: 0x0B, 96/120hz: 0x06
	 *    This is default setting, so if current brr fps is 60hz mode, set 120hz vfp as 16.
	 */
	vrr_cmds->cmds[VRR_CMDID_VFP_GPARA_DFT].msg.tx_buf[1] = (cur_rr <= 60) ? 0x06 : 0x0B;

	/* 15. gpara: VFP offset: 48/60hz: 0x0B, 96/120hz: 0x06 */
	vrr_cmds->cmds[VRR_CMDID_VFP_GPARA].msg.tx_buf[1] = (cur_rr <= 60) ? 0x0B : 0x06;

	/* 16. VFP: determines FPS */
	vrr_cmds->cmds[VRR_CMDID_VFP].msg.tx_buf[1] = (u8)(vfp_target >> 8);
	vrr_cmds->cmds[VRR_CMDID_VFP].msg.tx_buf[2] = (u8)(vfp_target & 0xFF);

	LCD_INFO("VRR: %s, FPS: %dhz%s (cur: %d%s, target: %d%s), AOR: %x\n",
			ss_get_brr_mode_name(brr_mode),
			fps_brr ? fps_brr : cur_rr,
			cur_hs ? "HS" : "NM",
			vrr->cur_refresh_rate,
			vrr->cur_sot_hs_mode ? "HS" : "NM",
			vrr->adjusted_refresh_rate,
			vrr->adjusted_sot_hs_mode ? "HS" : "NM",
			vrr_cmds->cmds[VRR_CMDID_AID].msg.tx_buf[1]);

	return vrr_cmds;
}

#define DELAY_TIME_SLEEPOUT_VRR	110 /* delay time [ms] between sleep out and VRR setting */
static struct dsi_panel_cmd_set *ss_vrr(struct samsung_display_driver_data *vdd, int *level_key)
{
	bool is_hbm = false;
	bool is_hmt = false;

#if 0
	/* Current HAB display added 150ms delay after sleep out..
	 * So, below delay is not required...
	 */
	s64 wait_ms_ssd;

	/* SSD Improve: it should block FPS change for 110 ms after sleep out command for VRR */
	wait_ms_ssd = DELAY_TIME_SLEEPOUT_VRR - ktime_ms_delta(ktime_get(), vdd->sleep_out_time);
	while (wait_ms_ssd > 0) {
		LCD_INFO("VRR: SSD wait for %lld ms\n", wait_ms_ssd);
		usleep_range(wait_ms_ssd * 1000, wait_ms_ssd * 1000);
		wait_ms_ssd = DELAY_TIME_SLEEPOUT_VRR - ktime_ms_delta(ktime_get(), vdd->sleep_out_time);
	}
#endif

	return __ss_vrr(vdd, level_key, is_hbm, is_hmt);
}

static int ss_brightness_tot_level(struct samsung_display_driver_data *vdd)
{
	int tot_level_key = 0;

	/* HAB brightness setting requires F0h and FCh level key unlocked */
	/* DBV setting require TEST KEY3 (FCh) */
	tot_level_key = LEVEL1_KEY | LEVEL2_KEY;

	return tot_level_key;
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
static struct dsi_panel_cmd_set *ss_gamma_hmt(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set  *hmt_gamma_cmds = ss_get_cmds(vdd, TX_HMT_GAMMA);
	struct brightness_table *br_tbl = ss_get_cur_br_tbl(vdd);

	if (SS_IS_CMDS_NULL(hmt_gamma_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)hmt_gamma_cmds);
		return NULL;
	}

	LCD_DEBUG("hmt_bl_level : %d candela : %dCD\n", vdd->br_info.hmt_stat.hmt_bl_level, vdd->br_info.hmt_stat.candela_level_hmt);

	if (vdd->vrr.black_frame_mode == BLACK_FRAME_INSERT) {
		/* HS <-> Normal mode: insert one black frame to hide screen noise
		 * ss_gamma() set gamma MTP (C8h) to zero.
		 * None zero gamma (CAh) setting causes greenish screen issue during MTP(C8h) is zero.
		 * So, gamma value should be set to zero while black_frame_mode is BLACK_FRAME_INSERT mode.
		 */

		LCD_INFO("insert black frame gamma (%dhz %s)\n",
				br_tbl->refresh_rate, br_tbl->is_sot_hs_mode ? "HS" : "NM");

		/* send all zero gamma (CAh) */
		memset(&hmt_gamma_cmds->cmds[0].msg.tx_buf[1], 0, GAMMA_SIZE);

		/* send all zero gamma MTP (C8h) */
		memset(&hmt_gamma_cmds->cmds[1].msg.tx_buf[1], 0, GAMMA_SIZE);
		hmt_gamma_cmds->cmds[1].msg.tx_buf[0] = 0xC8;
	} else if (vdd->vrr.black_frame_mode == BLACK_FRAME_REMOVE) {
		vdd->vrr.black_frame_mode = BLACK_FRAME_OFF;

		LCD_INFO("remove black frame gamma (%dhz %s)\n",
				br_tbl->refresh_rate, br_tbl->is_sot_hs_mode ? "HS" : "NM");

		/* send original gamma (CAh) */
		br_interpolation_generate_event(vdd, GEN_HMD_GAMMA, &hmt_gamma_cmds->cmds[0].msg.tx_buf[1]);

		/* send original gamma MTP (C8h) */
		memcpy(&hmt_gamma_cmds->cmds[1].msg.tx_buf[1], br_tbl->gamma_tbl->c8_register.mtp_data, GAMMA_SIZE);
		hmt_gamma_cmds->cmds[1].msg.tx_buf[0] = 0xC8;
	} else {
		/* send original gamma (CAh) */
		br_interpolation_generate_event(vdd, GEN_HMD_GAMMA, &hmt_gamma_cmds->cmds[0].msg.tx_buf[1]);

		/* do not touch C8h gamma MTP, send NULL packet */
		hmt_gamma_cmds->cmds[1].msg.tx_buf[0] = 0x00;
	}

	return hmt_gamma_cmds;
}

static struct dsi_panel_cmd_set *ss_aid_hmt(
		struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set  *hmt_aid_cmds = ss_get_cmds(vdd, TX_HMT_AID);

	if (SS_IS_CMDS_NULL(hmt_aid_cmds)) {
		LCD_ERR("No hmt_aid_tx_cmds\n");
		return NULL;
	}

	br_interpolation_generate_event(vdd, GEN_HMD_AOR, &hmt_aid_cmds->cmds->msg.tx_buf[1]);

	vdd->br_info.common_br.aor_data = (hmt_aid_cmds->cmds->msg.tx_buf[1] << 8)
						| hmt_aid_cmds->cmds->msg.tx_buf[2];

	return hmt_aid_cmds;
}

static struct dsi_panel_cmd_set *ss_elvss_hmt(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *elvss_cmds = ss_get_cmds(vdd, TX_HMT_ELVSS);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(elvss_cmds)) {
		LCD_ERR("No hmt_elvss_tx_cmds\n");
		return NULL;
	}

	/* 0xB5 1th TSET */
	elvss_cmds->cmds->msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
			vdd->br_info.temperature : BIT(7) | (-1*vdd->br_info.temperature);

	/* ELVSS(MPS_CON) setting condition is equal to normal birghtness */ // B5 2nd para : MPS_CON
	if (vdd->br_info.hmt_stat.candela_level_hmt > 39)
		elvss_cmds->cmds->msg.tx_buf[2] = 0xDC;
	else
		elvss_cmds->cmds->msg.tx_buf[2] = 0xCC;

	return elvss_cmds;
}

static void ss_make_sdimconf_hmt(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	/* Set the mtp read buffer pointer and read the NVM value*/
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP,
			br_tbl->smart_dimming_dsi_hmt->mtp_buffer, LEVEL1_KEY);

	/* Initialize smart dimming related things here */
	/* lux_tab setting for 350cd */
	br_tbl->smart_dimming_dsi_hmt->lux_tab = vdd->br_info.candela_map_table[HMT][vdd->panel_revision].cd;
	br_tbl->smart_dimming_dsi_hmt->lux_tabsize = vdd->br_info.candela_map_table[HMT][vdd->panel_revision].tab_size;
	br_tbl->smart_dimming_dsi_hmt->man_id = vdd->manufacture_id_dsi;
	if (vdd->panel_func.samsung_panel_revision)
			br_tbl->smart_dimming_dsi_hmt->panel_revision = vdd->panel_func.samsung_panel_revision(vdd);

	/* Just a safety check to ensure smart dimming data is initialised well */
	br_tbl->smart_dimming_dsi_hmt->init(br_tbl->smart_dimming_dsi_hmt);

	LCD_INFO("[HMT] smart dimming done!\n");
}

static int ss_smart_dimming_init_hmt(struct samsung_display_driver_data *vdd)
{
	int count;

	LCD_INFO("DSI%d : ++\n", vdd->ndx);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	vdd->br_info.hmt_stat.hmt_on = 0;
	vdd->br_info.hmt_stat.hmt_bl_level = 0;

	for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
		struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];

		br_tbl->smart_dimming_dsi_hmt = vdd->panel_func.samsung_smart_get_conf_hmt();

		if (IS_ERR_OR_NULL(br_tbl->smart_dimming_dsi_hmt)) {
			LCD_ERR("DSI%d error", vdd->ndx);
			return false;
		}

		ss_make_sdimconf_hmt(vdd, br_tbl);
	}

	vdd->br_info.smart_dimming_hmt_loaded_dsi = true;

	LCD_INFO("DSI%d : --\n", vdd->ndx);

	return true;
}

static void ss_set_panel_lpm_brightness(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *alpm_brightness[LPM_BRIGHTNESS_MAX_IDX] = {NULL, };
	struct dsi_panel_cmd_set *alpm_ctrl[MAX_LPM_CTRL] = {NULL, };
	struct dsi_panel_cmd_set *cmd_list[2];

	/*default*/
	int mode = ALPM_MODE_ON;
	int ctrl_index = ALPM_CTRL_2NIT;
	int bl_index = LPM_2NIT_IDX;

	/*
	 * Init reg_list and cmd list
	 * reg_list[X][0] is reg value
	 * reg_list[X][1] is offset for reg value
	 * cmd_list is the target cmds for searching reg value
	 */
	static int reg_list[2][2] = {
		{ALPM_REG, -EINVAL},
		{ALPM_CTRL_REG, -EINVAL} };

	LCD_DEBUG("%s++\n", __func__);

	cmd_list[0] = ss_get_cmds(vdd, TX_LPM_BL_CMD);
	cmd_list[1] = ss_get_cmds(vdd, TX_LPM_BL_CMD);
	if (SS_IS_CMDS_NULL(cmd_list[0]) || SS_IS_CMDS_NULL(cmd_list[1])) {
		LCD_ERR("No cmds for TX_LPM_BL_CMD..\n");
		return;
	}

	/* Init alpm_brightness and alpm_ctrl cmds */
	alpm_brightness[LPM_2NIT_IDX] = ss_get_cmds(vdd, TX_LPM_2NIT_CMD);
	alpm_brightness[LPM_10NIT_IDX] = ss_get_cmds(vdd, TX_LPM_10NIT_CMD);
	alpm_brightness[LPM_30NIT_IDX] = ss_get_cmds(vdd, TX_LPM_30NIT_CMD);
	alpm_brightness[LPM_60NIT_IDX] = ss_get_cmds(vdd, TX_LPM_60NIT_CMD);
	if (SS_IS_CMDS_NULL(alpm_brightness[LPM_2NIT_IDX]) || SS_IS_CMDS_NULL(alpm_brightness[LPM_10NIT_IDX]) ||
		SS_IS_CMDS_NULL(alpm_brightness[LPM_30NIT_IDX]) || SS_IS_CMDS_NULL(alpm_brightness[LPM_60NIT_IDX])) {
		LCD_ERR("No cmds for alpm_brightness..\n");
		return;
	}

	alpm_ctrl[ALPM_CTRL_2NIT] = ss_get_cmds(vdd, TX_ALPM_2NIT_CMD);
	alpm_ctrl[ALPM_CTRL_10NIT] = ss_get_cmds(vdd, TX_ALPM_10NIT_CMD);
	alpm_ctrl[ALPM_CTRL_30NIT] = ss_get_cmds(vdd, TX_ALPM_30NIT_CMD);
	alpm_ctrl[ALPM_CTRL_60NIT] = ss_get_cmds(vdd, TX_ALPM_60NIT_CMD);
	if (SS_IS_CMDS_NULL(alpm_ctrl[ALPM_CTRL_2NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[ALPM_CTRL_10NIT]) ||
		SS_IS_CMDS_NULL(alpm_ctrl[ALPM_CTRL_30NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[ALPM_CTRL_60NIT])) {
		LCD_ERR("No cmds for alpm_ctrl..\n");
		return;
	}

	alpm_ctrl[HLPM_CTRL_2NIT] = ss_get_cmds(vdd, TX_HLPM_2NIT_CMD);
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
		ctrl_index = (mode == ALPM_MODE_ON) ? ALPM_CTRL_10NIT :
			(mode == HLPM_MODE_ON) ? HLPM_CTRL_10NIT : ALPM_CTRL_10NIT;
		bl_index = LPM_10NIT_IDX;
		break;
	case LPM_30NIT:
		ctrl_index = (mode == ALPM_MODE_ON) ? ALPM_CTRL_30NIT :
			(mode == HLPM_MODE_ON) ? HLPM_CTRL_30NIT : ALPM_CTRL_30NIT;
		bl_index = LPM_30NIT_IDX;
		break;
	case LPM_60NIT:
		ctrl_index = (mode == ALPM_MODE_ON) ? ALPM_CTRL_60NIT :
			(mode == HLPM_MODE_ON) ? HLPM_CTRL_60NIT : ALPM_CTRL_60NIT;
		bl_index = LPM_60NIT_IDX;
		break;
	case LPM_2NIT:
	default:
		ctrl_index = (mode == ALPM_MODE_ON) ? ALPM_CTRL_2NIT :
			(mode == HLPM_MODE_ON) ? HLPM_CTRL_2NIT : ALPM_CTRL_2NIT;
		bl_index = LPM_2NIT_IDX;
		break;
	}

	LCD_DEBUG("[Panel LPM]bl_index %d, ctrl_index %d, mode %d\n",
			 bl_index, ctrl_index, mode);

	/*
	 * Find offset for alpm_reg and alpm_ctrl_reg
	 * alpm_reg  : Control register for ALPM/HLPM on/off
	 * alpm_ctrl_reg : Control register for changing ALPM/HLPM mode
	 */
	if (unlikely((reg_list[0][1] == -EINVAL) ||
				(reg_list[1][1] == -EINVAL)))
		ss_find_reg_offset(reg_list, cmd_list, ARRAY_SIZE(cmd_list));

	if (reg_list[0][1] != -EINVAL) {
		/* Update parameter for ALPM_REG */
		memcpy(cmd_list[0]->cmds[reg_list[0][1]].msg.tx_buf,
				alpm_brightness[bl_index]->cmds[0].msg.tx_buf,
				sizeof(char) * cmd_list[0]->cmds[reg_list[0][1]].msg.tx_len);

		LCD_DEBUG("[Panel LPM] change brightness cmd : %x, %x\n",
				cmd_list[0]->cmds[reg_list[0][1]].msg.tx_buf[1],
				alpm_brightness[bl_index]->cmds[0].msg.tx_buf[1]);
	}

	if (reg_list[1][1] != -EINVAL) {
		/* Initialize ALPM/HLPM cmds */
		/* Update parameter for ALPM_CTRL_REG */
		memcpy(cmd_list[1]->cmds[reg_list[1][1]].msg.tx_buf,
				alpm_ctrl[ctrl_index]->cmds[0].msg.tx_buf,
				sizeof(char) * cmd_list[1]->cmds[reg_list[1][1]].msg.tx_len);

		LCD_DEBUG("[Panel LPM] update alpm ctrl reg\n");
	}

	//send lpm bl cmd
	ss_send_cmd(vdd, TX_LPM_BL_CMD);

	LCD_INFO("[Panel LPM] bl_level : %s\n",
				/* Check current brightness level */
				vdd->panel_lpm.lpm_bl_level == LPM_2NIT ? "2NIT" :
				vdd->panel_lpm.lpm_bl_level == LPM_10NIT ? "10NIT" :
				vdd->panel_lpm.lpm_bl_level == LPM_30NIT ? "30NIT" :
				vdd->panel_lpm.lpm_bl_level == LPM_60NIT ? "60NIT" : "UNKNOWN");

	LCD_DEBUG("%s--\n", __func__);
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
	int ctrl_index = ALPM_CTRL_2NIT;
	int bl_index = LPM_2NIT_IDX;

	/*
	 * Init reg_list and cmd list
	 * reg_list[X][0] is reg value
	 * reg_list[X][1] is offset for reg value
	 * cmd_list is the target cmds for searching reg value
	 */
	static int reg_list[2][2] = {
		{ALPM_REG, -EINVAL},
		{ALPM_CTRL_REG, -EINVAL} };

	static int off_reg_list[1][2] = {
		{ALPM_CTRL_REG, -EINVAL} };

	LCD_ERR("%s++\n", __func__);

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
	alpm_brightness[LPM_2NIT_IDX] = ss_get_cmds(vdd, TX_LPM_2NIT_CMD);
	alpm_brightness[LPM_10NIT_IDX] = ss_get_cmds(vdd, TX_LPM_10NIT_CMD);
	alpm_brightness[LPM_30NIT_IDX] = ss_get_cmds(vdd, TX_LPM_30NIT_CMD);
	alpm_brightness[LPM_60NIT_IDX] = ss_get_cmds(vdd, TX_LPM_60NIT_CMD);
	if (SS_IS_CMDS_NULL(alpm_brightness[LPM_2NIT_IDX]) || SS_IS_CMDS_NULL(alpm_brightness[LPM_10NIT_IDX]) ||
		SS_IS_CMDS_NULL(alpm_brightness[LPM_30NIT_IDX]) || SS_IS_CMDS_NULL(alpm_brightness[LPM_60NIT_IDX])) {
		LCD_ERR("No cmds for alpm_brightness..\n");
		return;
	}

	alpm_ctrl[ALPM_CTRL_2NIT] = ss_get_cmds(vdd, TX_ALPM_2NIT_CMD);
	alpm_ctrl[ALPM_CTRL_10NIT] = ss_get_cmds(vdd, TX_ALPM_10NIT_CMD);
	alpm_ctrl[ALPM_CTRL_30NIT] = ss_get_cmds(vdd, TX_ALPM_30NIT_CMD);
	alpm_ctrl[ALPM_CTRL_60NIT] = ss_get_cmds(vdd, TX_ALPM_60NIT_CMD);
	if (SS_IS_CMDS_NULL(alpm_ctrl[ALPM_CTRL_2NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[ALPM_CTRL_10NIT]) ||
		SS_IS_CMDS_NULL(alpm_ctrl[ALPM_CTRL_30NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[ALPM_CTRL_60NIT])) {
		LCD_ERR("No cmds for alpm_ctrl..\n");
		return;
	}

	alpm_ctrl[HLPM_CTRL_2NIT] = ss_get_cmds(vdd, TX_HLPM_2NIT_CMD);
	alpm_ctrl[HLPM_CTRL_10NIT] = ss_get_cmds(vdd, TX_HLPM_10NIT_CMD);
	alpm_ctrl[HLPM_CTRL_30NIT] = ss_get_cmds(vdd, TX_HLPM_30NIT_CMD);
	alpm_ctrl[HLPM_CTRL_60NIT] = ss_get_cmds(vdd, TX_HLPM_60NIT_CMD);
	if (SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_2NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_10NIT]) ||
		SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_30NIT]) || SS_IS_CMDS_NULL(alpm_ctrl[HLPM_CTRL_60NIT])) {
		LCD_ERR("No cmds for hlpm_brightness..\n");
		return;
	}

	alpm_off_ctrl[ALPM_MODE_ON] = ss_get_cmds(vdd, TX_ALPM_OFF);
	if (SS_IS_CMDS_NULL(alpm_off_ctrl[ALPM_MODE_ON])) {
		LCD_ERR("No cmds for TX_ALPM_OFF..\n");
		return;
	}

	alpm_off_ctrl[HLPM_MODE_ON] = ss_get_cmds(vdd, TX_HLPM_OFF);
	if (SS_IS_CMDS_NULL(alpm_off_ctrl[HLPM_MODE_ON])) {
		LCD_ERR("No cmds for TX_HLPM_OFF..\n");
		return;
	}

	mode = vdd->panel_lpm.mode;

	switch (vdd->panel_lpm.lpm_bl_level) {
	case LPM_10NIT:
		ctrl_index = (mode == ALPM_MODE_ON) ? ALPM_CTRL_10NIT :
			(mode == HLPM_MODE_ON) ? HLPM_CTRL_10NIT : ALPM_CTRL_10NIT;
		bl_index = LPM_10NIT_IDX;
		break;
	case LPM_30NIT:
		ctrl_index = (mode == ALPM_MODE_ON) ? ALPM_CTRL_30NIT :
			(mode == HLPM_MODE_ON) ? HLPM_CTRL_30NIT : ALPM_CTRL_30NIT;
		bl_index = LPM_30NIT_IDX;
		break;
	case LPM_60NIT:
		ctrl_index = (mode == ALPM_MODE_ON) ? ALPM_CTRL_60NIT :
			(mode == HLPM_MODE_ON) ? HLPM_CTRL_60NIT : ALPM_CTRL_60NIT;
		bl_index = LPM_60NIT_IDX;
		break;
	case LPM_2NIT:
	default:
		ctrl_index = (mode == ALPM_MODE_ON) ? ALPM_CTRL_2NIT :
			(mode == HLPM_MODE_ON) ? HLPM_CTRL_2NIT : ALPM_CTRL_2NIT;
		bl_index = LPM_2NIT_IDX;
		break;
	}

	LCD_DEBUG("[Panel LPM] change brightness cmd :%d, %d, %d\n",
			 bl_index, ctrl_index, mode);

	/*
	 * Find offset for alpm_reg and alpm_ctrl_reg
	 * alpm_reg	 : Control register for ALPM/HLPM on/off
	 * alpm_ctrl_reg : Control register for changing ALPM/HLPM mode
	 */
	if (unlikely((reg_list[0][1] == -EINVAL) ||
				(reg_list[1][1] == -EINVAL)))
		ss_find_reg_offset(reg_list, cmd_list, ARRAY_SIZE(cmd_list));

	if (unlikely(off_reg_list[0][1] == -EINVAL))
		ss_find_reg_offset(off_reg_list, off_cmd_list,
						ARRAY_SIZE(off_cmd_list));

	if (reg_list[0][1] != -EINVAL) {
		/* Update parameter for ALPM_REG */
		memcpy(cmd_list[0]->cmds[reg_list[0][1]].msg.tx_buf,
				alpm_brightness[bl_index]->cmds[0].msg.tx_buf,
				sizeof(char) * cmd_list[0]->cmds[reg_list[0][1]].msg.tx_len);

		LCD_DEBUG("[Panel LPM] change brightness cmd : %x, %x\n",
				cmd_list[0]->cmds[reg_list[0][1]].msg.tx_buf[1],
				alpm_brightness[bl_index]->cmds[0].msg.tx_buf[1]);
	}

	if (reg_list[1][1] != -EINVAL) {
		/* Initialize ALPM/HLPM cmds */
		/* Update parameter for ALPM_CTRL_REG */
		memcpy(cmd_list[1]->cmds[reg_list[1][1]].msg.tx_buf,
				alpm_ctrl[ctrl_index]->cmds[0].msg.tx_buf,
				sizeof(char) * cmd_list[1]->cmds[reg_list[1][1]].msg.tx_len);

		LCD_DEBUG("[Panel LPM] update alpm ctrl reg\n");
	}

	if ((off_reg_list[0][1] != -EINVAL) &&\
			(mode != LPM_MODE_OFF)) {
		/* Update parameter for ALPM_CTRL_REG */
		memcpy(off_cmd_list[0]->cmds[off_reg_list[0][1]].msg.tx_buf,
				alpm_off_ctrl[mode]->cmds[0].msg.tx_buf,
				sizeof(char) * off_cmd_list[0]->cmds[off_reg_list[0][1]].msg.tx_len);
	}

	LCD_ERR("%s--\n", __func__);
}

static int ddi_hw_cursor(struct samsung_display_driver_data *vdd, int *input)
{
	struct dsi_panel_cmd_set *pcmds;
	u8 *payload;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return 0;
	}

	if (IS_ERR_OR_NULL(input)) {
		LCD_ERR("input is NULL\n");
		return 0;
	}

	pcmds = ss_get_cmds(vdd, TX_HW_CURSOR);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_HW_CURSOR.. \n");
		return 0;
	}
	payload = pcmds->cmds[0].msg.tx_buf;
	if (IS_ERR_OR_NULL(payload)) {
		LCD_ERR("hw_cursor_tx_cmds is NULL\n");
		return 0;
	}

	LCD_INFO("On/Off:(%d), Por/Land:(%d), On_Select:(%d), X:(%d), Y:(%d), Width:(%d), Length:(%d), Color:(0x%x), Period:(%x), TR_TIME(%x)\n",
		input[0], input[1], input[2], input[3], input[4], input[5],
		input[6], input[7], input[8], input[9]);

	/* Cursor On&Off (0:Off, 1:On) */
	payload[1] = input[0] & 0x1;

	/* 3rd bit : CURSOR_ON_SEL, 2nd bit : Port/Land, 1st bit : BLINK_ON(Default On)*/
	payload[2] = (input[2] & 0x1) << 2 | (input[1] & 0x1) << 1 | 0x1;

	/* Start X address */
	payload[3] = (input[3] & 0x700) >> 8;
	payload[4] = input[3] & 0xFF;

	/* Start Y address */
	payload[5] = (input[4] & 0x700) >> 8;
	payload[6] = input[4] & 0xFF;

	/* Width */
	payload[7] = input[5] & 0xF;

	/* Length */
	payload[8] = (input[6] & 0x100) >> 8;
	payload[9] = input[6] & 0xFF;

	/* Color */
	payload[10] = (input[7] & 0xFF0000) >> 16;
	payload[11] = (input[7] & 0xFF00) >> 8;
	payload[12] = input[7] & 0xFF;

	/* Period */
	payload[13] = input[8] & 0xFF;

	/* TR_TIME */
	payload[14] = input[9] & 0xFF;

	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_HW_CURSOR);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);

	return 1;
}

#if 0
static void ss_send_colorweakness_ccb_cmd(struct samsung_display_driver_data *vdd, int mode)
{
	struct dsi_panel_cmd_set *pcmds;

	LCD_INFO("mode (%d) color_weakness_value (%x) \n", mode, vdd->color_weakness_value);

	if (mode) {
		pcmds = ss_get_cmds(vdd, TX_COLOR_WEAKNESS_ENABLE);
		pcmds->cmds[1].msg.tx_buf[1] = vdd->color_weakness_value;
		ss_send_cmd(vdd, TX_COLOR_WEAKNESS_ENABLE);
	} else {
		ss_send_cmd(vdd, TX_COLOR_WEAKNESS_DISABLE);
	}
}
#endif

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
	u8 valid_checksum[4] = {0xE9, 0x29, 0xE9, 0x29};
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
		set->cmds[10].msg.tx_buf[1] = vddm_set[i];
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
	make_self_dispaly_img_cmds_HAB(vdd, TX_SELF_MASK_IMAGE, FLAG_SELF_MASK);

	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_buf = self_mask_img_wqhd_crc_data;
	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_size = ARRAY_SIZE(self_mask_img_wqhd_crc_data);
	make_mass_self_display_img_cmds_HAB(vdd, TX_SELF_MASK_IMAGE_CRC, FLAG_SELF_MASK_CRC);

	vdd->self_disp.operation[FLAG_SELF_ICON].img_buf = self_icon_img_data;
	vdd->self_disp.operation[FLAG_SELF_ICON].img_size = ARRAY_SIZE(self_icon_img_data);
	make_mass_self_display_img_cmds_HAB(vdd, TX_SELF_ICON_IMAGE, FLAG_SELF_ICON);

	vdd->self_disp.operation[FLAG_SELF_ACLK].img_buf = self_aclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_ACLK].img_size = ARRAY_SIZE(self_aclock_img_data);
	make_mass_self_display_img_cmds_HAB(vdd, TX_SELF_ACLOCK_IMAGE, FLAG_SELF_ACLK);

	vdd->self_disp.operation[FLAG_SELF_DCLK].img_buf = self_dclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_DCLK].img_size = ARRAY_SIZE(self_dclock_img_data);
	make_mass_self_display_img_cmds_HAB(vdd, TX_SELF_DCLOCK_IMAGE, FLAG_SELF_DCLK);

	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_buf = self_video_img_data;
	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_size = ARRAY_SIZE(self_video_img_data);
	make_mass_self_display_img_cmds_HAB(vdd, TX_SELF_VIDEO_IMAGE, FLAG_SELF_VIDEO);

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

u8 poc_spi_read_id(struct samsung_display_driver_data *vdd)
{
	u8 rxbuf[1];
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	/* read manufacture id */
	ret = ss_spi_sync(vdd->spi_dev, rxbuf, RX_FLASH_MANUFACTURE_ID);
	if (ret) {
		LCD_ERR("fail to spi read.. ret (%d) \n", ret);
		return ret;
	}

	LCD_ERR("spi manufacture id = %02x\n", rxbuf[0]);

	return rxbuf[0];
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

	//LCD_ERR("status : 0x%02x \n", rxbuf[0]);

	return rxbuf[0];
}

static int spi_write_enable_wait(struct samsung_display_driver_data *vdd)
{
	struct spi_device *spi_dev;
	int ret = 0;
	int status = 0;
	int try_cnt = 0;
	int total_try = 35000;

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
		try_cnt++;
		LCD_DEBUG("status(0x%02x) is busy.. wait!! cnt %d\n", status, try_cnt);
		usleep_range(20, 30);
		if (try_cnt > total_try) {
			LCD_ERR("BUSY wait error!! (0x%02x)\n", status);
			return -EIO;
		}
	}

	/* Write write enable */
	ss_spi_sync(spi_dev, NULL, TX_WRITE_ENABLE);

	/* WEL check */
	try_cnt = 0;
	while (!((status = poc_spi_get_status(vdd)) & 0x02)) {
		LCD_DEBUG("Not ready to write (0x%02x).. wait!! cnt %d\n", status, try_cnt);
		ss_spi_sync(spi_dev, NULL, TX_WRITE_ENABLE);
		usleep_range(20, 30);
		if (try_cnt++ > total_try) {
			LCD_ERR("WEL wait error!! (0x%02x)\n", status);
			return -EIO;
		}
	}

	return ret;
}

static int spi_wait_status_reg1(struct samsung_display_driver_data *vdd, int wait_val)
{
	struct spi_device *spi_dev;
	int ret = 0;
	int status = 0;
	int try_cnt = 0;
	int total_try = 35000;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	spi_dev = vdd->spi_dev;

	if (IS_ERR_OR_NULL(spi_dev)) {
		LCD_ERR("no spi_dev\n");
		return -EINVAL;
	}

	/* STATUS REG1 check */
	while ((status = poc_spi_get_status(vdd)) != wait_val) {
		LCD_DEBUG("status(0x%02x) .. wait!!\n", status);
		usleep_range(20, 30);
		if (try_cnt++ > total_try) {
			LCD_ERR("STATUS1 wait error!! (0x%02x) (0x%02x)\n", status, wait_val);
			return -EIO;
		}
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
	ret = spi_write_enable_wait(vdd);
	if (ret)
		goto err;
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG1);

	/* 0x00 : check QE is disabled (use single spi mode) */
	ret = spi_wait_status_reg1(vdd, 0x00);
	if (ret)
		goto err;

	ret = spi_write_enable_wait(vdd);
	if (ret)
		goto err;

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
	ret = spi_write_enable_wait(vdd);
	if (ret)
		goto err;
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG1_END);

	/* 0xFC : check QE is enabled (use quad spi mode) and block protection */
	ret = spi_wait_status_reg1(vdd, 0xFC);
	if (ret)
		goto err;
err:
	return ret;
}

static int poc_spi_write(struct samsung_display_driver_data *vdd, u8 *data, u32 write_pos, u32 write_size)
{
	struct spi_device *spi_dev;
	struct ddi_spi_cmd_set *cmd_set = NULL;
	int pos, ret = 0;
	int last_pos, image_size, loop_cnt, poc_w_size, start_addr;

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
	start_addr = vdd->poc_driver.start_addr;

	cmd_set = ss_get_spi_cmds(vdd, TX_WRITE_PAGE_PROGRAM);
	if (cmd_set == NULL) {
		LCD_ERR("cmd_set is null..\n");
		return -EINVAL;
	}

	LCD_INFO("[WRITE] write_pos : %6d, write_size : %6d, last_pos %6d, poc_w_size : %d\n",
		write_pos, write_size, last_pos, poc_w_size);

	/* start */
	ret = spi_write_enable_wait(vdd);
	if (ret)
		goto err;
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG1);

	/* 0x00 : check QE is disabled (use single spi mode) */
	ret = spi_wait_status_reg1(vdd, 0x00);
	if (ret)
		goto err;

	for (pos = write_pos; pos < last_pos; ) {
		if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
			LCD_ERR("cancel poc write by user\n");
			ret = -EIO;
			goto cancel_poc;
		}

		cmd_set->tx_addr = pos;

		memcpy(&cmd_set->tx_buf[4], &data[pos - start_addr], cmd_set->tx_size - 3);

		ret = spi_write_enable_wait(vdd);
		if (ret)
			goto err;

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
	ret = spi_write_enable_wait(vdd);
	if (ret)
		goto err;
	ss_spi_sync(spi_dev, NULL, TX_WRITE_STATUS_REG1_END);

	/* 0xFC : check QE is enabled (use quad spi mode) and block protection */
	ret = spi_wait_status_reg1(vdd, 0xFC);
	if (ret)
		goto err;
err:
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
	memcpy(&buf[read_pos - vdd->poc_driver.start_addr], rbuf, read_size);

	/* rx_buf reset */
	memset(rbuf, 0, read_size);

	if (!(read_pos % DEBUG_POC_CNT))
		LCD_INFO("buf[%d] = 0x%x\n", read_pos, buf[read_pos - vdd->poc_driver.start_addr]);

cancel_poc:
	if (rbuf)
		kfree(rbuf);

	return ret;
}

static int ss_vrr_init(struct vrr_info *vrr)
{
	struct vrr_bridge_rr_tbl *brr;

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

	vrr->brr_mode = BRR_OFF_MODE;
	vrr->brr_rewind_on = false;

	vrr->vrr_workqueue = create_singlethread_workqueue("vrr_workqueue");
	INIT_WORK(&vrr->vrr_work, ss_panel_vrr_switch_work);

	/* set value for BRR modes respectively */
	/* 48NM -> 60NM */
	brr = &vrr->brr_tbl[BRR_48_60];
	brr->tot_steps = 1;
	brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
	if (!brr->fps)
		return -ENOMEM;
	brr->fps[0] = 52;
	brr->fps_start = 48;
	brr->fps_end = 60;
	brr->fps_base = 60;	/* based on 60 normal*/
	brr->vbp_base = 16;
	brr->vactive_base = 3200;
	brr->vfp_base = 16;
	brr->sot_hs_base = false;
	brr->min_cd = 11;	/* 48NM/96NM BRR: 11nit ~ 420nit */
	brr->max_cd = 420;
	brr->delay_frame_cnt = 4;

	/* 60NM -> 48NM */
	brr = &vrr->brr_tbl[BRR_60_48];
	brr->tot_steps = 1;
	brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
	if (!brr->fps)
		return -ENOMEM;
	brr->fps[0] = 52;
	brr->fps_start = 60;
	brr->fps_end = 48;
	brr->fps_base = 60;	/* based on 60 normal*/
	brr->vbp_base = 16;
	brr->vactive_base = 3200;
	brr->vfp_base = 16;
	brr->sot_hs_base = false;
	brr->min_cd = 11;	/* 48NM/96NM BRR: 11nit ~ 420nit */
	brr->max_cd = 420;
	brr->delay_frame_cnt = 4;

	/* 96HS -> 120HS */
	brr = &vrr->brr_tbl[BRR_96_120];
	brr->tot_steps = 2;
	brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
	if (!brr->fps)
		return -ENOMEM;
	brr->fps[0] = 104;
	brr->fps[1] = 112;
	brr->fps_start = 96;
	brr->fps_end = 120;
	brr->fps_base = 120;	/* based on 120HS */
	brr->vbp_base = 16;
	brr->vactive_base = 3200;
	brr->vfp_base = 16;
	brr->sot_hs_base = true;
	brr->min_cd = 11;	/* 96HS/120HS BRR: 11nit ~ 420nit */
	brr->max_cd = 420;
	brr->delay_frame_cnt = 4;

	/* 120HS -> 96HS */
	brr = &vrr->brr_tbl[BRR_120_96];
	brr->tot_steps = 2;
	brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
	if (!brr->fps)
		return -ENOMEM;
	brr->fps[0] = 112;
	brr->fps[1] = 104;
	brr->fps_start = 120;
	brr->fps_end = 96;
	brr->fps_base = 120;	/* based on 120HS */
	brr->vbp_base = 16;
	brr->vactive_base = 3200;
	brr->vfp_base = 16;
	brr->sot_hs_base = true;
	brr->min_cd = 11;	/* 96HS/120HS BRR: 11nit ~ 420nit */
	brr->max_cd = 420;
	brr->delay_frame_cnt = 4;

	if ((get_lcd_attached("GET") & 0xF) >= 3) {
		/* ID3 03, 04 panel:
		 * - remove 80hz bridge fps
		 * - bridge fps 120hz as 0x80
		 */

		/* 60HS -> 120HS */
		brr = &vrr->brr_tbl[BRR_60HS_120];
		brr->tot_steps = 4;
		brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
		if (!brr->fps)
			return -ENOMEM;
		brr->fps[0] = 70;
		brr->fps[1] = 100;
		brr->fps[2] = 110;
		brr->fps[3] = 120; // 120hz with AOR cycle=4 (0x80)
		brr->fps_start = 60;
		brr->fps_end = 120;
		brr->fps_base = 120;	/* based on 120HS */
		brr->vbp_base = 16;
		brr->vactive_base = 3200;
		brr->vfp_base = 16;
		brr->sot_hs_base = true;
		brr->min_cd = 98;	/* 60HS/120HS BRR: 98~420nit */
		brr->max_cd = 420;
		brr->delay_frame_cnt = 5;

		/* 120HS -> 60HS */
		brr = &vrr->brr_tbl[BRR_120_60HS];
		brr->tot_steps = 4;
		brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
		if (!brr->fps)
			return -ENOMEM;
		brr->fps[0] = 120; // 120hz with AOR cycle=4 (0x80)
		brr->fps[1] = 110;
		brr->fps[2] = 100;
		brr->fps[3] = 70;
		brr->fps_start = 120;
		brr->fps_end = 60;
		brr->fps_base = 120;	/* based on 120HS */
		brr->vbp_base = 16;
		brr->vactive_base = 3200;
		brr->vfp_base = 16;
		brr->sot_hs_base = true;
		brr->min_cd = 98;	/* 60HS/120HS BRR: 98~420nit */
		brr->max_cd = 420;
		brr->delay_frame_cnt = 8;

	} else {
		/* 60HS -> 120HS
		 * - remove 80hz bridge fps
		 */
		brr = &vrr->brr_tbl[BRR_60HS_120];
		brr->tot_steps = 3;
		brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
		if (!brr->fps)
			return -ENOMEM;
		brr->fps[0] = 70;
		brr->fps[1] = 100;
		brr->fps[2] = 110;
		brr->fps_start = 60;
		brr->fps_end = 120;
		brr->fps_base = 120;	/* based on 120HS */
		brr->vbp_base = 16;
		brr->vactive_base = 3200;
		brr->vfp_base = 16;
		brr->sot_hs_base = true;
		brr->min_cd = 98;	/* 60HS/120HS BRR: 98~420nit */
		brr->max_cd = 420;
		brr->delay_frame_cnt = 5;

		/* 120HS -> 60HS */
		brr = &vrr->brr_tbl[BRR_120_60HS];
		brr->tot_steps = 3;
		brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
		if (!brr->fps)
			return -ENOMEM;
		brr->fps[0] = 110;
		brr->fps[1] = 100;
		brr->fps[2] = 70;
		brr->fps_start = 120;
		brr->fps_end = 60;
		brr->fps_base = 120;	/* based on 120HS */
		brr->vbp_base = 16;
		brr->vactive_base = 3200;
		brr->vfp_base = 16;
		brr->sot_hs_base = true;
		brr->min_cd = 98;	/* 60HS/120HS BRR: 98~420nit */
		brr->max_cd = 420;
		brr->delay_frame_cnt = 8;
	}

	/* 60HS -> 120HS -> 96HS */
	brr = &vrr->brr_tbl[BRR_60HS_96];
	brr->tot_steps = 6;
	brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
	if (!brr->fps)
		return -ENOMEM;
	brr->fps[0] = 70;
	brr->fps[1] = 100;
	brr->fps[2] = 110;
	brr->fps[3] = 120;
	brr->fps[4] = 112;
	brr->fps[5] = 104;
	brr->fps_start = 60;
	brr->fps_end = 96;
	brr->fps_base = 120;	/* based on 120HS */
	brr->vbp_base = 16;
	brr->vactive_base = 3200;
	brr->vfp_base = 16;
	brr->sot_hs_base = true;
	brr->min_cd = 98;	/* 96HS/120HS BRR: 11~420nit && 60HS/120HS BRR: 98~420nit */
	brr->max_cd = 420;
	brr->delay_frame_cnt = 4; /* TODO: it should be set: 5 during 60->120, 4 during 120->96 */

	/* 96HS -> 120HS -> 60HS */
	brr = &vrr->brr_tbl[BRR_96_60HS];
	brr->tot_steps = 6;
	brr->fps = kzalloc(sizeof(int) * brr->tot_steps, GFP_KERNEL);
	if (!brr->fps)
		return -ENOMEM;
	brr->fps[0] = 104;
	brr->fps[1] = 112;
	brr->fps[2] = 120;
	brr->fps[3] = 110;
	brr->fps[4] = 100;
	brr->fps[5] = 70;
	brr->fps_start = 96;
	brr->fps_end = 60;
	brr->fps_base = 120;	/* based on 120HS */
	brr->vbp_base = 16;
	brr->vactive_base = 3200;
	brr->vfp_base = 16;
	brr->sot_hs_base = true;
	brr->min_cd = 98;	/* 96HS/120HS BRR: 11~420nit && 60HS/120HS BRR: 98~420nit */
	brr->max_cd = 420;
	brr->delay_frame_cnt = 4; /* TODO: it should be set: 4 during 96->120, 8 during 120->60,  */

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
		if (!(cur_rr == 60 && !cur_hs)) {
			is_support = false;
			LCD_ERR("MCD fail: supported on 60NS(cur: %dNS)\n",
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
	vdd->panel_func.samsung_module_info_read = ss_module_info_read;
	vdd->panel_func.samsung_ddi_id_read = ss_ddi_id_read;

	vdd->panel_func.samsung_smart_dimming_init = ss_smart_dimming_init;
	vdd->panel_func.samsung_smart_get_conf = smart_get_conf_S6E3HAB_AMB623TS01;

	/* GAMMA FLASH */
	vdd->panel_func.gen_hbm_interpolation_gamma = gen_hbm_interpolation_gamma_S6E3HAB_AMB623TS01;
	vdd->panel_func.gen_hbm_interpolation_irc = gen_hbm_interpolation_irc_S6E3HAB_AMB623TS01;
	vdd->panel_func.gen_normal_interpolation_irc = gen_normal_interpolation_irc_S6E3HAB_AMB623TS01;
	vdd->panel_func.samsung_flash_gamma_support = flash_gamma_support_S6E3HAB_AMB623TS01;
	vdd->panel_func.samsung_interpolation_init = init_interpolation_S6E3HAB_AMB623TS01;
	vdd->panel_func.force_use_table_for_hmd = table_gamma_update_hmd_S6E3HAB_AMB623TS01;
	vdd->panel_func.get_gamma_V_size = get_gamma_V_size_S6E3HAB_AMB623TS01;
	vdd->panel_func.convert_GAMMA_to_V = convert_GAMMA_to_V;
	vdd->panel_func.convert_V_to_GAMMA = convert_V_to_GAMMA;

	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;

	/* Brightness */
	vdd->panel_func.samsung_brightness_aid = ss_aid;
	vdd->panel_func.samsung_brightness_acl_on = ss_acl_on;
	vdd->panel_func.samsung_brightness_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_elvss = NULL;
	vdd->panel_func.samsung_brightness_dbv = ss_dbv;
	vdd->panel_func.samsung_brightness_vint = ss_vint;
	vdd->panel_func.samsung_brightness_irc = ss_irc;
	vdd->panel_func.samsung_brightness_gamma = ss_gamma;
	vdd->panel_func.samsung_brightness_ltps = ss_ltps;
	vdd->panel_func.samsung_brightness_vrr = ss_vrr;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_hbm_gamma;
	vdd->panel_func.samsung_hbm_etc = ss_hbm_etc;
	vdd->panel_func.samsung_hbm_irc = ss_hbm_irc;
	vdd->panel_func.samsung_brightness_vrr_hbm = ss_vrr_hbm;

	/* Total level key in brightness */
	vdd->panel_func.samsung_brightness_tot_level = ss_brightness_tot_level;

	/* Event */

	/* HMT */
	vdd->panel_func.samsung_brightness_gamma_hmt = ss_gamma_hmt;
	vdd->panel_func.samsung_brightness_aid_hmt = ss_aid_hmt;
	vdd->panel_func.samsung_brightness_elvss_hmt = ss_elvss_hmt;
	vdd->panel_func.samsung_brightness_vrr_hmt = ss_vrr_hmt;
	vdd->panel_func.samsung_smart_dimming_hmt_init = ss_smart_dimming_init_hmt;
	vdd->panel_func.samsung_smart_get_conf_hmt = smart_get_conf_S6E3HAB_AMB623TS01_hmt;

	/* Panel LPM */
	vdd->panel_func.samsung_update_lpm_ctrl_cmd = ss_update_panel_lpm_ctrl_cmd;
	vdd->panel_func.samsung_set_lpm_brightness = ss_set_panel_lpm_brightness;

	/* default brightness */
	vdd->br_info.common_br.bl_level = 25500;

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

	/* Set IRC init value */
	vdd->br_info.common_br.irc_mode = IRC_FLAT_GAMMA_MODE; /* default value */

	/* COLOR WEAKNESS */

	/* Support DDI HW CURSOR */
	vdd->panel_func.ddi_hw_cursor = ddi_hw_cursor;

	/* COVER Open/Close */

	/* COPR */
	vdd->copr.panel_init = ss_copr_panel_init;

	/* ACL default ON */
	vdd->br_info.acl_status = 1;

	/* ACL default status in acl on */
	vdd->br_info.gradual_acl_val = 1;

	/* Gram Checksum Test */
	vdd->panel_func.samsung_gct_write = ss_gct_write;
	vdd->panel_func.samsung_gct_read = ss_gct_read;

	/* Self display */
	vdd->self_disp.is_support = true;
	vdd->self_disp.factory_support = true;
	vdd->self_disp.init = self_display_init_HAB;
	vdd->self_disp.data_init = ss_self_display_data_init;

	/* mAPFC */
	vdd->mafpc.init = ss_mafpc_init_HAB;
	vdd->mafpc.data_init = ss_mafpc_data_init;

	/* stm */
	vdd->stm.stm_on = 1;

	/* POC */
	vdd->poc_driver.poc_erase = poc_spi_erase;
	vdd->poc_driver.poc_write = poc_spi_write;
	vdd->poc_driver.poc_read = poc_spi_read;
	vdd->poc_driver.poc_comp = poc_comp;

	/* VRR */
	ss_vrr_init(&vdd->vrr);

	vdd->panel_func.samsung_check_support_mode = ss_check_support_mode;
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_S6E3HAB_AMB623TS01_WQHD";
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
