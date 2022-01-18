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
#include "ss_dsi_panel_S6E3HA9_AMB611WE01.h"
#include "ss_dsi_mdnie_S6E3HA9_AMB611WE01.h"
#include "ss_dsi_interpolation_S6E3HA9_AMB611WE01.h"

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

#define IRC_MODERATO_MODE_VAL	0xE1
#define IRC_FLAT_GAMMA_MODE_VAL	0xA1

/* This has been applied temporarily for sensors and will be removed later.*/
void sensor_set_brightness_level(int32_t level, int32_t aor);

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
		/* Rev.A*/
		vdd->panel_revision = 'A';
		break;
	case 0x01:
		/* Rev.B*/
		vdd->panel_revision = 'B';
		break;
	case 0x02:
		/* Rev.C*/
		vdd->panel_revision = 'C';
		break;
	default:
		vdd->panel_revision = 'C';
		LCD_ERR("Invalid panel_rev(default rev : %c)\n",
				vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';

	LCD_INFO_ONCE("panel_revision = %c %d \n",
					vdd->panel_revision + 'A', vdd->panel_revision);

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

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(hbm_gamma_cmds)) {
		LCD_ERR("Invalid data  vdd : 0x%zx cmd : 0x%zx", (size_t)vdd, (size_t)hbm_gamma_cmds);
		return NULL;
	}

	*level_key = LEVEL1_KEY;
	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_GAMMA, &hbm_gamma_cmds->cmds->msg.tx_buf[1]);
	return hbm_gamma_cmds;
}

static struct dsi_panel_cmd_set *ss_hbm_etc(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_etc_cmds = ss_get_cmds(vdd, TX_HBM_ETC);

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(hbm_etc_cmds)) {
		LCD_ERR("Invalid data  vdd : 0x%zx cmd : 0x%zx", (size_t)vdd, (size_t)hbm_etc_cmds);
		return NULL;
	}

	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_AOR, &hbm_etc_cmds->cmds[0].msg.tx_buf[1]);

	/* 	0xB5 1th TSET
		BIT(7) is signed bit.
		BIT(6) ~ BIT(0) is real temperature.
	*/
	hbm_etc_cmds->cmds[1].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
			vdd->br_info.temperature : (char)(BIT(7) | (-1*vdd->br_info.temperature));

	/* 412cd ~ HBM is 0xDC */
	hbm_etc_cmds->cmds[1].msg.tx_buf[2] = 0xDC;

	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_ELVSS, &hbm_etc_cmds->cmds[1].msg.tx_buf[3]);

	/* read B5h 58th -> write B5h 57h */
	hbm_etc_cmds->cmds[3].msg.tx_buf[1] = vdd->br_info.common_br.elvss_value[1];

	/* VINT */
	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_VINT, &hbm_etc_cmds->cmds[4].msg.tx_buf[2]);

	/* ACL percentage
	 * 1. 8% : 0x26
	 * 2. 15% : 0x55
	 */
	if (vdd->br_info.gradual_acl_val) {
		hbm_etc_cmds->cmds[5].msg.tx_buf[6] = 0x26;	/* 8% */
		hbm_etc_cmds->cmds[6].msg.tx_buf[1] = 0x2;	/* ACL ON */
		hbm_etc_cmds->cmds[5].msg.tx_buf[2] = 0x55;
	} else {
		hbm_etc_cmds->cmds[5].msg.tx_buf[6] = 0x0;	/* 0% */
		hbm_etc_cmds->cmds[6].msg.tx_buf[1] = 0x0;	/* ACL OFF */
		hbm_etc_cmds->cmds[5].msg.tx_buf[2] = 0x44;
	}

	*level_key = LEVEL1_KEY;

	LCD_INFO("elvss 3th: 0x%x 0x%x, vint 0x%x, acl (%x, per=%x, frame avg=%x)\n",
			hbm_etc_cmds->cmds[1].msg.tx_buf[3],
			vdd->br_info.common_br.elvss_value[1],
			hbm_etc_cmds->cmds[4].msg.tx_buf[2],
			hbm_etc_cmds->cmds[6].msg.tx_buf[1],
			hbm_etc_cmds->cmds[5].msg.tx_buf[6],
			hbm_etc_cmds->cmds[5].msg.tx_buf[2]);

	return hbm_etc_cmds;
}

#if 0
#define NUM_IRC_OTP	17
static int ss_irc_read(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *set;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	if (!vdd->display_status_dsi.irc_otp) {
		vdd->display_status_dsi.irc_otp	= kzalloc(NUM_IRC_OTP, GFP_KERNEL);
		if (!vdd->display_status_dsi.irc_otp) {
			LCD_ERR("fail to allocate irc_otp memory\n");
			return false;
		}
	}

	/* Read mtp (B5h 23th,24th) for elvss*/
	ss_panel_data_read(vdd, RX_IRC, vdd->display_status_dsi.irc_otp,
							LEVEL1_KEY);

	/* update irc packet with otp value */
	set = ss_get_cmds(vdd, TX_PAC_IRC_SUBDIVISION);
	memcpy(&set->cmds->msg.tx_buf[2], vdd->display_status_dsi.irc_otp, NUM_IRC_OTP);

	set = ss_get_cmds(vdd, TX_HBM_IRC);
	memcpy(&set->cmds->msg.tx_buf[2], vdd->display_status_dsi.irc_otp, NUM_IRC_OTP);

	return true;
}
#endif

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

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	if (ss_get_cmds(vdd, RX_MODULE_INFO)->count) {
		ss_panel_data_read(vdd, RX_MODULE_INFO, buf, LEVEL1_KEY);

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

	/* read values from DDI */

	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, sconf->mtp_buffer, LEVEL1_KEY);

	/* Read mtp (B5h 23th,24th) for elvss*/
	ss_panel_data_read(vdd, RX_ELVSS, vdd->br_info.common_br.elvss_value, LEVEL1_KEY);

	/* Read mtp (B3h 3~35th) for hbm gamma */
	sconf->hbm_payload = kzalloc(vdd->br_info.gamma_size, GFP_KERNEL);
	if (!sconf->hbm_payload) {
		LCD_ERR("fail to allocate hbm_payload memory\n");
		return -ENOMEM;
 	}
	ss_panel_data_read(vdd, RX_HBM, sconf->hbm_payload, LEVEL1_KEY);

	/* Initialize smart dimming related things here */
	/* lux_tab setting for 350cd */
	sconf->lux_tab = vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].cd;
	sconf->lux_tabsize = vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].tab_size;
	sconf->man_id = vdd->manufacture_id_dsi;
	if (vdd->panel_func.samsung_panel_revision)
		sconf->panel_revision = vdd->panel_func.samsung_panel_revision(vdd);

	/* Just a safety check to ensure smart dimming data is initialised well */
	sconf->init(sconf);

	vdd->br_info.temperature = 20; // default temperature

	vdd->br_info.smart_dimming_loaded_dsi = true;

	LCD_INFO("DSI%d : --\n", vdd->ndx);

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
	int cd_index = 0;
	struct dsi_panel_cmd_set *aid_cmds = ss_get_cmds(vdd, TX_PAC_AID_SUBDIVISION);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(aid_cmds)) {
		LCD_ERR("No aid_tx_cmds\n");
		return NULL;
	}

	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_AOR, &aid_cmds->cmds->msg.tx_buf[1]);

	LCD_DEBUG("[%d] level(%d), aid(%x %x)\n",
			cd_index, vdd->br_info.common_br.bl_level,
			aid_cmds->cmds->msg.tx_buf[1],
			aid_cmds->cmds->msg.tx_buf[2]);

	*level_key = LEVEL1_KEY;

	return aid_cmds;
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

	*level_key = LEVEL1_KEY;

	return ss_get_cmds(vdd, TX_ACL_OFF);
}

static struct dsi_panel_cmd_set *ss_elvss(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *elvss_cmds = ss_get_cmds(vdd, TX_ELVSS);
	u8 elvss_57th_val;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(elvss_cmds)) {
		LCD_ERR("No elvss_tx_cmds\n");
		return NULL;
	}

	/* 	0xB5 1th TSET
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
	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_ELVSS, &elvss_cmds->cmds->msg.tx_buf[3]);

	/* 0xB5 elvss_57th_val elvss_cal_offset */
	elvss_57th_val = vdd->br_info.common_br.elvss_value[0];
	elvss_cmds->cmds[2].msg.tx_buf[1] = elvss_57th_val;

	*level_key = LEVEL1_KEY;

	return elvss_cmds;
}

static struct dsi_panel_cmd_set *ss_vint(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *vint_cmds = ss_get_cmds(vdd, TX_VINT);

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(vint_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)vint_cmds);
		return NULL;
	}

	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_VINT, &vint_cmds->cmds->msg.tx_buf[2]);

	if (vdd->xtalk_mode)
		vint_cmds->cmds->msg.tx_buf[1] = 0x6B;	// VGH 6.2 V
	else
		vint_cmds->cmds->msg.tx_buf[1] = 0xEB;	// VGH 7.0 V

	*level_key = LEVEL1_KEY;

	return vint_cmds;
}

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

	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_IRC, &irc_cmds->cmds->msg.tx_buf[1]);

	/* set irc mode to moderato or flat gamma */
	if (vdd->br_info.common_br.irc_mode == IRC_MODERATO_MODE)
		irc_cmds->cmds->msg.tx_buf[2] = IRC_MODERATO_MODE_VAL;
	else if (vdd->br_info.common_br.irc_mode == IRC_FLAT_GAMMA_MODE)
		irc_cmds->cmds->msg.tx_buf[2] = IRC_FLAT_GAMMA_MODE_VAL;
	else
		LCD_ERR("invalid irc mode(%d)\n", vdd->br_info.common_br.irc_mode);

	*level_key = LEVEL1_KEY;

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

	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_IRC, &hbm_irc_cmds->cmds->msg.tx_buf[1]);

	/* set irc mode to moderato or flat gamma */
	if (vdd->br_info.common_br.irc_mode == IRC_MODERATO_MODE)
		hbm_irc_cmds->cmds->msg.tx_buf[2] = IRC_MODERATO_MODE_VAL;
	else if (vdd->br_info.common_br.irc_mode == IRC_FLAT_GAMMA_MODE)
		hbm_irc_cmds->cmds->msg.tx_buf[2] = IRC_FLAT_GAMMA_MODE_VAL;
	else
		LCD_ERR("invalid irc mode(%d)\n", vdd->br_info.common_br.irc_mode);

	*level_key = LEVEL1_KEY;

	return hbm_irc_cmds;
}

static struct dsi_panel_cmd_set *ss_gamma(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set  *gamma_cmds = ss_get_cmds(vdd, TX_GAMMA);

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(gamma_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)gamma_cmds);
		return NULL;
	}

	LCD_DEBUG("bl_level : %d candela : %dCD\n", vdd->br_info.common_br.bl_level, vdd->br_info.common_br.cd_level);

	*level_key = LEVEL1_KEY;

	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_GAMMA, &gamma_cmds->cmds[0].msg.tx_buf[1]);

	return gamma_cmds;
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

	if (SS_IS_CMDS_NULL(hmt_gamma_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)hmt_gamma_cmds);
		return NULL;
	}

	LCD_DEBUG("hmt_bl_level : %d candela : %dCD\n", vdd->br_info.hmt_stat.hmt_bl_level, vdd->br_info.hmt_stat.candela_level_hmt);

	*level_key = LEVEL1_KEY;
	br_interpolation_generate_event(vdd, GEN_HMD_GAMMA, &hmt_gamma_cmds->cmds[0].msg.tx_buf[1]);
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

	*level_key = LEVEL1_KEY;

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

	*level_key = LEVEL1_KEY;

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
	mdnie_data->dsi_max_night_mode_index = 12;
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
	u8 valid_checksum[4] = {0xEC, 0xEC, 0xEC, 0xEC};
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
	/* vddm set, 0x0: 1.0V, 0x08: 0.9V, 0x01: 1.1V */
	u8 vddm_set[MAX_VDDM] = {0x0, 0x08, 0x01};
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

	if (panel_type == PANEL_TYPE_PHOLE && panel_color != PANEL_COLOR_UTYPE) { /* P-Hole & C-Type */
		LCD_INFO("Self Mask Image for P-Hole C-Type\n");
		vdd->self_disp.operation[FLAG_SELF_MASK].img_buf = self_mask_img_phole_ctype_data;
		vdd->self_disp.operation[FLAG_SELF_MASK].img_size = ARRAY_SIZE(self_mask_img_phole_ctype_data);
		vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum = SELF_MASK_IMG_PHOLE_CTYPE_CHECKSUM;
	} else if (panel_type == PANEL_TYPE_PHOLE && panel_color == PANEL_COLOR_UTYPE) {
		LCD_INFO("Self Mask Image for P-Hole U-Type\n");
		vdd->self_disp.operation[FLAG_SELF_MASK].img_buf = self_mask_img_phole_utype_data;
		vdd->self_disp.operation[FLAG_SELF_MASK].img_size = ARRAY_SIZE(self_mask_img_phole_utype_data);
		vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum = SELF_MASK_IMG_PHOLE_UTYPE_CHECKSUM;
	} else {
		LCD_INFO("Self Mask Image for L-Cut\n");
		vdd->self_disp.operation[FLAG_SELF_MASK].img_buf = self_mask_img_lcut_data;
		vdd->self_disp.operation[FLAG_SELF_MASK].img_size = ARRAY_SIZE(self_mask_img_lcut_data);
		vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum = SELF_MASK_IMG_LCUT_CHECKSUM;
	}
	make_self_dispaly_img_cmds_HA9(vdd, TX_SELF_MASK_IMAGE, FLAG_SELF_MASK);

	vdd->self_disp.operation[FLAG_SELF_ICON].img_buf = self_icon_img_data;
	vdd->self_disp.operation[FLAG_SELF_ICON].img_size = ARRAY_SIZE(self_icon_img_data);
	make_self_dispaly_img_cmds_HA9(vdd, TX_SELF_ICON_IMAGE, FLAG_SELF_ICON);

	vdd->self_disp.operation[FLAG_SELF_ACLK].img_buf = self_aclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_ACLK].img_size = ARRAY_SIZE(self_aclock_img_data);
	make_self_dispaly_img_cmds_HA9(vdd, TX_SELF_ACLOCK_IMAGE, FLAG_SELF_ACLK);

	vdd->self_disp.operation[FLAG_SELF_DCLK].img_buf = self_dclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_DCLK].img_size = ARRAY_SIZE(self_dclock_img_data);
	make_self_dispaly_img_cmds_HA9(vdd, TX_SELF_DCLOCK_IMAGE, FLAG_SELF_DCLK);

	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_buf = self_video_img_data;
	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_size = ARRAY_SIZE(self_video_img_data);
	make_self_dispaly_img_cmds_HA9(vdd, TX_SELF_VIDEO_IMAGE, FLAG_SELF_VIDEO);

	return 1;
}

static void ss_copr_panel_init(struct samsung_display_driver_data *vdd)
{
	vdd->copr.tx_bpw = 8;
	vdd->copr.rx_bpw = 8;
	vdd->copr.tx_size = 1;
	vdd->copr.rx_addr = 0x5A;
	vdd->copr.rx_size = 41;
	vdd->copr.ver = COPR_VER_3P0;
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
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)poc_comp_cmds);
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

static int poc_erase(struct samsung_display_driver_data *vdd, u32 erase_pos, u32 erase_size, u32 target_pos)
{
	struct dsi_display *display = NULL;
	struct dsi_panel_cmd_set *poc_erase_sector_tx_cmds = NULL;
	int delay_us = 0;
	int image_size = 0;
	int type;
	int ret = 0;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

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

	if (erase_size == POC_ERASE_64KB) {
		delay_us = 1000000; /* 1000ms */
		poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[4] = 0xD8;
	} else if (erase_size == POC_ERASE_32KB) {
		delay_us = 800000; /* 800ms */
		poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[4] = 0x52;
	} else {
		delay_us = 400000; /* 400ms */
		poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[4] = 0x20;
	}

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
		usleep_range(500, 500);

	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 1);

	/* POC MODE ENABLE */
	ss_send_cmd(vdd, TX_POC_ENABLE);

	LCD_INFO("WRITE [TX_POC_PRE_ERASE_SECTOR]");
	ss_send_cmd(vdd, TX_POC_PRE_ERASE_SECTOR);

	poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[vdd->poc_driver.erase_sector_addr_idx[0]]
											= (erase_pos & 0xFF0000) >> 16;
	poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[vdd->poc_driver.erase_sector_addr_idx[1]]
											= (erase_pos & 0x00FF00) >> 8;
	poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[vdd->poc_driver.erase_sector_addr_idx[2]]
											= erase_pos & 0x0000FF;

	ss_send_cmd(vdd, TX_POC_ERASE_SECTOR);

	usleep_range(delay_us, delay_us);

	if ((erase_pos + erase_size >= target_pos) || ret == -EIO) {
		LCD_INFO("WRITE [TX_POC_POST_ERASE_SECTOR] - cur_erase_pos(%d) target_pos(%d) ret(%d)\n",
			erase_pos, target_pos, ret);
		ss_send_cmd(vdd, TX_POC_POST_ERASE_SECTOR);
	}

	/* POC MODE DISABLE */
	ss_send_cmd(vdd, TX_POC_DISABLE);

	/* exit exclusive mode*/
	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 0);
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
	struct dsi_display *display = NULL;
	struct dsi_panel_cmd_set *write_cmd = NULL;
	struct dsi_panel_cmd_set *write_data_add = NULL;
	int pos, type, ret = 0;
	int last_pos, delay_us, image_size, loop_cnt, poc_w_size;
	int tx_size, tx_size1, tx_size2;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_ERR("no display");
		return -EINVAL;
	}

	write_cmd = ss_get_cmds(vdd, TX_POC_WRITE_LOOP_256BYTE);
	if (SS_IS_CMDS_NULL(write_cmd)) {
		LCD_ERR("no cmds for TX_POC_WRITE_LOOP_256BYTE..\n");
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
	loop_cnt = vdd->poc_driver.write_loop_cnt;

	LCD_INFO("[WRITE] write_pos : %6d, write_size : %6d, last_pos %6d, poc_w_sise : %d delay %dus\n",
		write_pos, write_size, last_pos, poc_w_size, delay_us);

	/* MAX CPU & Memory BW ON */
	ss_set_max_cpufreq(vdd, true, CPUFREQ_CLUSTER_ALL);
	ss_set_max_mem_bw(vdd, true);
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_ON);

	/* Enter exclusive mode */
	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.permit_frame_update = 1;
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 1);

	/* POC MODE ENABLE */
	ss_send_cmd(vdd, TX_POC_ENABLE);

	if (write_pos == 0) {
		LCD_INFO("WRITE [TX_POC_PRE_WRITE]");
		ss_send_cmd(vdd, TX_POC_PRE_WRITE);
	}

	for (pos = write_pos; pos < last_pos; ) {
		if (!(pos % DEBUG_POC_CNT))
			LCD_INFO("cur_write_pos : %d data : 0x%x\n", pos, data[pos]);

		if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
			LCD_ERR("cancel poc write by user\n");
			ret = -EIO;
			goto cancel_poc;
		}

		if (pos % loop_cnt == 0) {
			if (pos > 0) {
				LCD_DEBUG("WRITE_LOOP_END pos : %d \n", pos);
				ss_send_cmd(vdd, TX_POC_WRITE_LOOP_END);
			}

			LCD_DEBUG("WRITE_LOOP_START pos : %d \n", pos);
			ss_send_cmd(vdd, TX_POC_WRITE_LOOP_START);

			usleep_range(delay_us, delay_us);

			/*	Multi Data Address */
			write_data_add->cmds[0].msg.tx_buf[vdd->poc_driver.write_addr_idx[0]]
											= (pos & 0xFF0000) >> 16;
			write_data_add->cmds[0].msg.tx_buf[vdd->poc_driver.write_addr_idx[1]]
											= (pos & 0x00FF00) >> 8;
			write_data_add->cmds[0].msg.tx_buf[vdd->poc_driver.write_addr_idx[2]]
											= (pos & 0x0000FF);
			ss_send_cmd(vdd, TX_POC_WRITE_LOOP_DATA_ADD);
		}

		/* 256 Byte Write (128 and 128) */
		if (last_pos - pos >= poc_w_size) {
			tx_size = poc_w_size;
			tx_size1 = tx_size / 2;
			tx_size2 = tx_size / 2;
		} else {
			tx_size = last_pos - pos;
			tx_size1 = (tx_size / (poc_w_size / 2)) ? poc_w_size / 2 : tx_size;
			tx_size2 = (tx_size / (poc_w_size / 2)) ? tx_size - tx_size1 : 0;

			/* set as dummpy data */
			memset(&write_cmd->cmds[1].msg.tx_buf[1], 0xFF, poc_w_size / 2);
			memset(&write_cmd->cmds[3].msg.tx_buf[1], 0xFF, poc_w_size / 2);
		}

		/* data copy */
		if (tx_size1)
			memcpy(&write_cmd->cmds[1].msg.tx_buf[1], &data[pos], tx_size1);
		if (tx_size2)
			memcpy(&write_cmd->cmds[3].msg.tx_buf[1], &data[pos + tx_size1], tx_size2);

		ss_send_cmd(vdd, TX_POC_WRITE_LOOP_256BYTE);

		usleep_range(delay_us, delay_us);

		pos += tx_size;
	}

cancel_poc:
	if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
		LCD_ERR("cancel poc write by user\n");
		atomic_set(&vdd->poc_driver.cancel, 0);
		ret = -EIO;
	}

	if (pos == image_size || ret == -EIO) {
		LCD_DEBUG("WRITE_LOOP_END pos : %d \n", pos);
		ss_send_cmd(vdd, TX_POC_WRITE_LOOP_END);

		LCD_INFO("WRITE [TX_POC_POST_WRITE] - image_size(%d) cur_write_pos(%d) ret(%d)\n", image_size, pos, ret);
		ss_send_cmd(vdd, TX_POC_POST_WRITE);
	}

	/* POC MODE DISABLE */
	ss_send_cmd(vdd, TX_POC_DISABLE);

	/* exit exclusive mode*/
	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 0);
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

#define read_buf_size 1
static int poc_read(struct samsung_display_driver_data *vdd, u8 *buf, u32 read_pos, u32 read_size)
{
	struct dsi_display *display = NULL;
	struct dsi_panel_cmd_set *poc_read_tx_cmds = NULL;
	struct dsi_panel_cmd_set *poc_read_rx_cmds = NULL;
	int delay_us;
	int image_size;
	u8 rx_buf[read_buf_size];
	int pos;
	int type;
	int ret = 0;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

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

	LCD_INFO("[READ] read_pos : %6d, read_size : %6d, delay %dus\n", read_pos, read_size, delay_us);

	/* MAX CPU & Memory BW ON */
	ss_set_max_cpufreq(vdd, true, CPUFREQ_CLUSTER_ALL);
	ss_set_max_mem_bw(vdd, true);
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_ON);

	/* Enter exclusive mode */
	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.permit_frame_update = 1;
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 1);
	ss_set_exclusive_tx_packet(vdd, RX_POC_READ, 1);

	/* For sending direct rx cmd  */
	poc_read_rx_cmds->cmds[0].msg.rx_buf = rx_buf;
	poc_read_rx_cmds->state = DSI_CMD_SET_STATE_HS;

	/* POC MODE ENABLE */
	ss_send_cmd(vdd, TX_POC_ENABLE);

	if (read_pos == 0) {
		LCD_INFO("WRITE [TX_POC_PRE_READ]");
		ss_send_cmd(vdd, TX_POC_PRE_READ);
	}

	for (pos = read_pos; pos < (read_pos + read_size); pos++) {
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

		ss_send_cmd(vdd, TX_POC_READ);

		usleep_range(delay_us, delay_us);

		ss_send_cmd(vdd, RX_POC_READ);

		buf[pos] = rx_buf[0];

		if (!(pos % DEBUG_POC_CNT))
			LCD_INFO("buf[%d] = 0x%x\n", pos, buf[pos]);
	}

cancel_poc:
	if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
		LCD_ERR("cancel poc read by user\n");
		atomic_set(&vdd->poc_driver.cancel, 0);
		ret = -EIO;
	}

	if (pos == image_size || ret == -EIO) {
		LCD_INFO("WRITE [TX_POC_POST_READ] - image_size(%d) cur_read_pos(%d) ret(%d)\n", image_size, pos, ret);
		ss_send_cmd(vdd, TX_POC_POST_READ);
	}

	/* POC MODE DISABLE */
	ss_send_cmd(vdd, TX_POC_DISABLE);

	/* MAX CPU & Memory BW OFF */
	dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_ALL_CLKS, DSI_CLK_OFF);
	ss_set_max_mem_bw(vdd, false);
	ss_set_max_cpufreq(vdd, false, CPUFREQ_CLUSTER_ALL);

	/* Exit exclusive mode*/
	for (type = TX_POC_CMD_START; type < TX_POC_CMD_END + 1 ; type++)
		ss_set_exclusive_tx_packet(vdd, type, 0);
	ss_set_exclusive_tx_packet(vdd, RX_POC_READ, 0);
	vdd->exclusive_tx.permit_frame_update = 0;
	vdd->exclusive_tx.enable = 0;
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);

	return ret;
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
	vdd->panel_func.samsung_smart_get_conf = smart_get_conf_S6E3HA9_AMB611WE01;

	/* GAMMA FLASH */
	vdd->panel_func.gen_hbm_interpolation_gamma = gen_hbm_interpolation_gamma_S6E3HA9_AMB611WE01;
	vdd->panel_func.gen_hbm_interpolation_irc = gen_hbm_interpolation_irc_S6E3HA9_AMB611WE01;
	vdd->panel_func.gen_normal_interpolation_irc = gen_normal_interpolation_irc_S6E3HA9_AMB611WE01;
	vdd->panel_func.samsung_flash_gamma_support = flash_gamma_support_S6E3HA9_AMB611WE01;
	vdd->panel_func.samsung_interpolation_init = init_interpolation_S6E3HA9_AMB611WE01;
	vdd->panel_func.get_gamma_V_size = get_gamma_V_size_S6E3HA9_AMB611WE01;
	vdd->panel_func.convert_GAMMA_to_V = convert_GAMMA_to_V_S6E3HA9_AMB611WE01;
	vdd->panel_func.convert_V_to_GAMMA = convert_V_to_GAMMA_S6E3HA9_AMB611WE01;

	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;

	/* Brightness */
	vdd->panel_func.samsung_brightness_aid = ss_aid;
	vdd->panel_func.samsung_brightness_acl_on = ss_acl_on;
	vdd->panel_func.samsung_brightness_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_elvss = ss_elvss;
	vdd->panel_func.samsung_brightness_vint = ss_vint;
	vdd->panel_func.samsung_brightness_irc = ss_irc;
	vdd->panel_func.samsung_brightness_gamma = ss_gamma;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_hbm_gamma;
	vdd->panel_func.samsung_hbm_etc = ss_hbm_etc;
	vdd->panel_func.samsung_hbm_irc = ss_hbm_irc;

	/* Event */

	/* HMT */
	vdd->panel_func.samsung_brightness_gamma_hmt = ss_gamma_hmt;
	vdd->panel_func.samsung_brightness_aid_hmt = ss_aid_hmt;
	vdd->panel_func.samsung_brightness_elvss_hmt = ss_elvss_hmt;
	vdd->panel_func.samsung_smart_dimming_hmt_init = ss_smart_dimming_init_hmt;
	vdd->panel_func.samsung_smart_get_conf_hmt = smart_get_conf_S6E3HA9_AMB611WE01_hmt;

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
	vdd->br_info.common_br.irc_mode = IRC_MODERATO_MODE;

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
	vdd->self_disp.init = self_display_init_HA9;
	vdd->self_disp.data_init = ss_self_display_data_init;

	/* stm */
	vdd->stm.stm_on = 1;

	/* POC */
	vdd->poc_driver.poc_erase = poc_erase;
	vdd->poc_driver.poc_write = poc_write;
	vdd->poc_driver.poc_read = poc_read;
	vdd->poc_driver.poc_comp = poc_comp;
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_S6E3HA9_AMB611WE01_WQHD";
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
