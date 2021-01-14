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
Copyright (C) 2012, Samsung Electronics. All rights reserved.

*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *
*/
#include "ss_dsi_panel_S6E3FA7_AMS559NK01.h"
#include "ss_dsi_mdnie_S6E3FA7_AMS559NK01.h"

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

/* Register to control brightness level */
#define ALPM_REG	0x53
/* Register to cnotrol ALPM/HLPM mode */
#define ALPM_CTRL_REG	0xBB

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("+: ndx=%d\n", vdd->ndx);
	ss_panel_attach_set(vdd, true);
	LCD_INFO("-: ndx=%d \n", vdd->ndx);

	return true;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	if (vdd->self_disp.self_mask_img_write)
		vdd->self_disp.self_mask_img_write(vdd);
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
		/* Pre. / Rev.A / Rev.B */
		vdd->panel_revision = 'B';
		break;
	case 0x01:
		/* Rev.C / Rev.D */
		vdd->panel_revision = 'D';
		break;
	case 0x02:
		/* Rev.E / Rev.F */
		vdd->panel_revision = 'F';
		break;
	case 0x03:
		vdd->panel_revision = 'G';
		break;
	default:
		vdd->panel_revision = 'H';
		LCD_ERR("Invalid panel_rev(default rev : %c)\n",
				vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';

	LCD_INFO_ONCE("panel_revision = %c %d \n",
					vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision + 'A');
}

static int ss_manufacture_date_read(struct samsung_display_driver_data *vdd)
{
	unsigned char date[4];
	int year, month, day;
	int hour, min;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (A1h 5,6 th) for manufacture date */
	if (ss_get_cmds(vdd, RX_MANUFACTURE_DATE)->count) {
		ss_panel_data_read(vdd, RX_MANUFACTURE_DATE, date, LEVEL1_KEY);

		year = date[0] & 0xf0;
		year >>= 4;
		year += 2011; // 0 = 2011 year
		month = date[0] & 0x0f;
		day = date[1] & 0x1f;
		hour = date[2] & 0x0f;
		min = date[3] & 0x1f;

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

static int ss_cell_id_read(struct samsung_display_driver_data *vdd)
{
	char cell_id_buffer[MAX_CELL_ID] = {0,};
	int loop;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read Panel Unique Cell ID (C8h 41~51th) */
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

static int ss_elvss_read(struct samsung_display_driver_data *vdd)
{
	char elvss_b5[2];

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (B5h 23th,24th) for elvss*/
	ss_panel_data_read(vdd, RX_ELVSS, elvss_b5, LEVEL1_KEY);

	vdd->br.elvss_value1 = elvss_b5[0]; /*0xB5 23th OTP value*/
	vdd->br.elvss_value2 = elvss_b5[1]; /*0xB5 24th */

	return true;
}

static struct dsi_panel_cmd_set *ss_vint(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *vint_cmds = ss_get_cmds(vdd, TX_VINT);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vint_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)vint_cmds);
		return NULL;
	}

	/* TODO: implement xtalk_mode
	if (vdd->xtalk_mode)
		cmd_index = 1;	// VGH 6.2 V
	else
		cmd_index = 0;	// VGH 7.0 V
	 */

	*level_key = LEVEL1_KEY;

	return vint_cmds;
}

static int ss_hbm_read(struct samsung_display_driver_data *vdd)
{
	int i, j;
	char hbm_buffer1[33];
	struct dsi_panel_cmd_set *hbm_gamma_cmds = ss_get_cmds(vdd, TX_HBM_GAMMA);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_gamma_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)hbm_gamma_cmds);
		return false;
	}

	/* Read mtp (B3h 3~34th) for hbm gamma */
	ss_panel_data_read(vdd, RX_HBM, hbm_buffer1, LEVEL1_KEY);

	/* V255 RGB */
	hbm_gamma_cmds->cmds[0].msg.tx_buf[1] = (hbm_buffer1[0] & 0x04) >> 2;
	hbm_gamma_cmds->cmds[0].msg.tx_buf[2] = hbm_buffer1[2];
	hbm_gamma_cmds->cmds[0].msg.tx_buf[3] = (hbm_buffer1[0] & 0x02) >> 1;
	hbm_gamma_cmds->cmds[0].msg.tx_buf[4] = hbm_buffer1[3];
	hbm_gamma_cmds->cmds[0].msg.tx_buf[5] = (hbm_buffer1[0] & 0x01) >> 0;
	hbm_gamma_cmds->cmds[0].msg.tx_buf[6] = hbm_buffer1[4];

	/* V203 ~ V1 */
	for (i = 7, j = 5; i <= 33; i++, j++)
		hbm_gamma_cmds->cmds[0].msg.tx_buf[i] = hbm_buffer1[j];

	/* VT RGB */
	hbm_gamma_cmds->cmds[0].msg.tx_buf[34] = hbm_buffer1[0] & 0xF0;
	hbm_gamma_cmds->cmds[0].msg.tx_buf[34] |= (hbm_buffer1[1] & 0xF0) >> 4;
	hbm_gamma_cmds->cmds[0].msg.tx_buf[35] = hbm_buffer1[1] & 0x0F;

	return true;
}

static int get_hbm_candela_value(int level)
{
	if (level == 13)
		return 443;
	else if (level == 6)
		return 465;
	else if (level == 7)
		return 488;
	else if (level == 8)
		return 510;
	else if (level == 9)
		return 533;
	else if (level == 10)
		return 555;
	else if (level == 11)
		return 578;
	else if (level == 12)
		return 600;
	else
		return 600;
}

static struct dsi_panel_cmd_set *ss_hbm_gamma(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_gamma_cmds = ss_get_cmds(vdd, TX_HBM_GAMMA);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_gamma_cmds)) {
		LCD_ERR("Invalid data  vdd : 0x%zx cmd : 0x%zx", (size_t)vdd, (size_t)hbm_gamma_cmds);
		return NULL;
	}

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi->generate_hbm_gamma)) {
		LCD_ERR("generate_hbm_gamma is NULL error");
		return NULL;
	} else {
		vdd->smart_dimming_dsi->generate_hbm_gamma(
			vdd->smart_dimming_dsi,
			vdd->br.auto_level,
			&hbm_gamma_cmds->cmds[0].msg.tx_buf[1]);

		*level_key = LEVEL1_KEY;
		return hbm_gamma_cmds;
	}
}

static struct dsi_panel_cmd_set *ss_hbm_etc(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_etc_cmds = ss_get_cmds(vdd, TX_HBM_ETC);
	char hbm_elvss_offset[] = {
		0x0D,	/* 443 */
		0x0B,	/* 465 */
		0x0A,	/* 488 */
		0x09,	/* 510 */
		0x08,	/* 533 */
		0x06,	/* 555 */
		0x05,	/* 578 */
		0x04,	/* 600 */
	};

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_etc_cmds)) {
		LCD_ERR("Invalid data  vdd : 0x%zx cmd : 0x%zx", (size_t)vdd, (size_t)hbm_etc_cmds);
		return NULL;
	}

	/* MPS/ELVSS : 0xB5 1th TSET */
	hbm_etc_cmds->cmds[1].msg.tx_buf[1] =
		vdd->temperature > 0 ? vdd->temperature : 0x80|(-1*vdd->temperature);

	/* MPS/ELVSS : 0xB5 3rd val */
	hbm_etc_cmds->cmds[1].msg.tx_buf[3] = hbm_elvss_offset[vdd->br.auto_level - HBM_MODE];

	if (vdd->acl_status == 1) {  /*acl on : 8%*/
		hbm_etc_cmds->cmds[2].msg.tx_buf[1] = 0x50; /*B4 1st*/
		hbm_etc_cmds->cmds[2].msg.tx_buf[13] = 0x50; /*B4 13th*/
		hbm_etc_cmds->cmds[3].msg.tx_buf[1] = 0x02; /*55 1st*/
	} else {  /*acl off*/
		hbm_etc_cmds->cmds[2].msg.tx_buf[1] = 0x40; /*B4 1st*/
		hbm_etc_cmds->cmds[2].msg.tx_buf[13] = 0x40; /*B4 13th*/
		hbm_etc_cmds->cmds[3].msg.tx_buf[1] = 0x00; /*55 1st*/
	}
	/* Read B5h 24th para -> Write to B5h 23th para */
	hbm_etc_cmds->cmds[1].msg.tx_buf[23] = vdd->br.elvss_value2;

	*level_key = LEVEL1_KEY;

	LCD_INFO("B6_3rd_PARA = 0x%x, TSET = 0x%x, 23th = 0x%x \n",
		hbm_etc_cmds->cmds[1].msg.tx_buf[3], hbm_etc_cmds->cmds[1].msg.tx_buf[1], hbm_etc_cmds->cmds[1].msg.tx_buf[23]);

	return hbm_etc_cmds;
}

#define COORDINATE_DATA_SIZE 6

#define F1(x, y) ((y)-((39*(x))/38)-95)
#define F2(x, y) ((y)-((36*(x))/35)-56)
#define F3(x, y) ((y)+((7*(x))/1)-24728)
#define F4(x, y) ((y)+((25*(x))/7)-14031)

/* Normal Mode */
static char coordinate_data_1[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xfb, 0x00, 0xfa, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfd, 0x00, 0xff, 0x00}, /* Tune_2 */
	{0xfa, 0x00, 0xfa, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfe, 0x00, 0xfb, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xfa, 0x00, 0xfc, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xfd, 0x00, 0xff, 0x00, 0xfa, 0x00}, /* Tune_7 */
	{0xfd, 0x00, 0xff, 0x00, 0xfd, 0x00}, /* Tune_8 */
	{0xfa, 0x00, 0xfe, 0x00, 0xff, 0x00}, /* Tune_9 */
};

/* sRGB/Adobe RGB Mode */
static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xf9, 0x00, 0xf3, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfa, 0x00, 0xf6, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfc, 0x00, 0xfa, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfb, 0x00, 0xf3, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfd, 0x00, 0xfa, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfd, 0x00, 0xf4, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfd, 0x00, 0xf6, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xff, 0x00, 0xfa, 0x00}, /* Tune_9 */
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

static int ss_smart_dimming_init(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	vdd->smart_dimming_dsi = vdd->panel_func.samsung_smart_get_conf();
	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi)) {
		LCD_ERR("DSI%d smart_dimming_dsi is null", vdd->ndx);
		return false;
	}

	ss_panel_data_read(vdd, RX_SMART_DIM_MTP,
			vdd->smart_dimming_dsi->mtp_buffer, LEVEL1_KEY);

	/* Initialize smart dimming related things here */
	/* lux_tab setting for 350cd */
	vdd->smart_dimming_dsi->lux_tab = vdd->dtsi_data.candela_map_table[NORMAL][vdd->panel_revision].cd;
	vdd->smart_dimming_dsi->lux_tabsize = vdd->dtsi_data.candela_map_table[NORMAL][vdd->panel_revision].tab_size;
	vdd->smart_dimming_dsi->man_id = vdd->manufacture_id_dsi;
	if (vdd->panel_func.samsung_panel_revision)
		vdd->smart_dimming_dsi->panel_revision = vdd->panel_func.samsung_panel_revision(vdd);

	/* copy hbm gamma payload for hbm interpolation calc */
	pcmds = ss_get_cmds(vdd, TX_HBM_GAMMA);
	vdd->smart_dimming_dsi->hbm_payload = &pcmds->cmds[0].msg.tx_buf[1];

	/* Just a safety check to ensure smart dimming data is initialised well */
	vdd->smart_dimming_dsi->init(vdd->smart_dimming_dsi);

	vdd->temperature = 20; // default temperature

	vdd->smart_dimming_loaded_dsi = true;

	LCD_INFO("DSI%d : --\n", vdd->ndx);

	return true;
}

static struct dsi_panel_cmd_set aid_cmd;
static struct dsi_panel_cmd_set *ss_aid(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds = ss_get_cmds(vdd, TX_AID_SUBDIVISION);
	int cd_index = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (vdd->br.pac)
		cd_index = vdd->br.pac_cd_idx;
	else
		cd_index = vdd->br.bl_level;

	aid_cmd.count = 1;
	aid_cmd.cmds = &pcmds->cmds[cd_index];
	LCD_DEBUG("[%d] level(%d), aid(%x %x)\n",
			cd_index, vdd->br.bl_level,
			aid_cmd.cmds->msg.tx_buf[1],
			aid_cmd.cmds->msg.tx_buf[2]);

	*level_key = LEVEL1_KEY;

	return &aid_cmd;
}

#define GRADUAL_ACL_STEP_JP2 6 //refer config_display.xml
static struct dsi_panel_cmd_set *ss_acl_on(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;
	u8 acl_value[GRADUAL_ACL_STEP_JP2] = {0x00, 0x0F, 0x1F, 0x2F, 0x3F, 0x4F};
	int step;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL1_KEY;

	pcmds = ss_get_cmds(vdd, TX_ACL_ON);

	if (vdd->gradual_acl_val
			&& vdd->gradual_acl_val < GRADUAL_ACL_STEP_JP2)
		step = vdd->gradual_acl_val;
	else
		step = GRADUAL_ACL_STEP_JP2 - 1; /* default value */

	if (vdd->gradual_acl_val)
		pcmds->cmds[0].msg.tx_buf[5] = acl_value[step];

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

static struct dsi_panel_cmd_set elvss_cmd;
static struct dsi_panel_cmd_set *ss_elvss(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *elvss_cmds;
	int cd_index = 0;
	int cmd_idx = 0;
	int candela_value = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (vdd->temperature > 0) // high temp
		elvss_cmds = ss_get_cmds(vdd, TX_ELVSS_HIGH);
	else if (vdd->temperature <= -15) // low temp
		elvss_cmds = ss_get_cmds(vdd, TX_ELVSS_LOW);
	else // mid temp
		elvss_cmds = ss_get_cmds(vdd, TX_ELVSS_MID);

	if (IS_ERR_OR_NULL(elvss_cmds)) {
		LCD_ERR("Invalid data cmds : 0x%zx \n", (size_t)elvss_cmds);
		return NULL;
	}

	cd_index = vdd->br.cd_idx;
	candela_value = vdd->br.cd_level;

	if (!vdd->dtsi_data.smart_acl_elvss_map_table[vdd->panel_revision].size ||
		cd_index > vdd->dtsi_data.smart_acl_elvss_map_table[vdd->panel_revision].size)
		goto end;

	cmd_idx = vdd->dtsi_data.smart_acl_elvss_map_table[vdd->panel_revision].cmd_idx[cd_index];

	LCD_DEBUG("cd_index (%d) cmd_idx(%d) candela_value(%d) B5 1st(%x) 2nd (%x) 3rd (%x)\n",
		cd_index, cmd_idx, candela_value,
		elvss_cmds->cmds[cmd_idx].msg.tx_buf[1],
		elvss_cmds->cmds[cmd_idx].msg.tx_buf[2],
		elvss_cmds->cmds[cmd_idx].msg.tx_buf[3]);

	elvss_cmd.cmds = &(elvss_cmds->cmds[cmd_idx]);
	elvss_cmd.count = 1;

	/* TEST */
	elvss_cmd.cmds->msg.tx_buf[1] = vdd->temperature > 0 ? vdd->temperature : 0x80|(-1*vdd->temperature);

	/* Read B5h 23th para -> Write to B5h 23th para */
	elvss_cmd.cmds->msg.tx_buf[23] = vdd->br.elvss_value1;

	*level_key = LEVEL1_KEY;

	return &elvss_cmd;
end:
	LCD_ERR("error");
	return NULL;
}

static struct dsi_panel_cmd_set *ss_gamma(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set  *gamma_cmds = ss_get_cmds(vdd, TX_GAMMA);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(gamma_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)gamma_cmds);
		return NULL;
	}

	LCD_DEBUG("bl_level : %d candela : %dCD\n", vdd->br.bl_level, vdd->br.cd_level);

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi->generate_gamma)) {
		LCD_ERR("generate_gamma is NULL error");
		return NULL;
	} else {
		vdd->smart_dimming_dsi->generate_gamma(
			vdd->smart_dimming_dsi,
			vdd->br.cd_level,
			&gamma_cmds->cmds[0].msg.tx_buf[1]);

		*level_key = LEVEL1_KEY;

		return gamma_cmds;
	}
}

/* IRC */
#define NUM_IRC_OTP	2
static int ss_irc_read(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	if (!vdd->br.irc_otp) {
		vdd->br.irc_otp	= kzalloc(NUM_IRC_OTP, GFP_KERNEL);
		if (!vdd->br.irc_otp) {
			LCD_ERR("fail to allocate irc_otp memory\n");
			return false;
		}
	}

	/* Read mtp (B5h 23th,24th) for elvss*/
	ss_panel_data_read(vdd, RX_IRC, vdd->br.irc_otp,
							LEVEL1_KEY);


	return true;
}

static struct dsi_panel_cmd_set irc_cmd;
static struct dsi_panel_cmd_set *ss_irc(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *irc_cmds = ss_get_cmds(vdd, TX_IRC_SUBDIVISION);
	int cd_index = 0;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(irc_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx",
				(size_t)vdd, (size_t)irc_cmds);
		return NULL;
	}

	if (IS_ERR_OR_NULL(irc_cmds->cmds)) {
		LCD_ERR("No irc_subdivision_tx_cmds\n");
		return NULL;
	}

	if (!vdd->br.support_irc)
		return NULL;

	/* IRC Subdivision works like as AID Subdivision */
	if (vdd->br.pac)
		cd_index = vdd->br.pac_cd_idx;
	else
		cd_index = vdd->br.bl_level;

	LCD_DEBUG("irc idx (%d)\n", cd_index);

	irc_cmd.cmds = &(irc_cmds->cmds[cd_index]);
	irc_cmd.count = 1;
	*level_key = LEVEL1_KEY;

	/* read B8 1st,2nd from MTP and write to B8 1st,2nd */
	irc_cmd.cmds->msg.tx_buf[1] = vdd->br.irc_otp[0];
	irc_cmd.cmds->msg.tx_buf[2] = vdd->br.irc_otp[1];

	return &irc_cmd;
}

static struct dsi_panel_cmd_set hbm_irc_cmd;
static struct dsi_panel_cmd_set *ss_hbm_irc(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_irc_cmds = ss_get_cmds(vdd, TX_HBM_IRC);
	int para_idx = 0;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_irc_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)hbm_irc_cmds);
		return NULL;
	}

	if (IS_ERR_OR_NULL(hbm_irc_cmds->cmds)) {
		LCD_ERR("No irc_tx_cmds\n");
		return NULL;
	}

	if (!vdd->br.support_irc)
		return NULL;

	*level_key = LEVEL1_KEY;

	para_idx = MAX_AUTO_LEVEL - vdd->br.auto_level;

	hbm_irc_cmd.cmds = &(hbm_irc_cmds->cmds[para_idx]);
	hbm_irc_cmd.count = 1;

	/* read B8 1st,2nd from MTP and write to B8 1st,2nd */
	hbm_irc_cmd.cmds->msg.tx_buf[1] = vdd->br.irc_otp[0];
	hbm_irc_cmd.cmds->msg.tx_buf[2] = vdd->br.irc_otp[1];

	return &hbm_irc_cmd;
}

// ========================
//			HMT
// ========================
static struct dsi_panel_cmd_set *ss_gamma_hmt(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set  *hmt_gamma_cmds = ss_get_cmds(vdd, TX_HMT_GAMMA);

	if (IS_ERR_OR_NULL(hmt_gamma_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)hmt_gamma_cmds);
		return NULL;
	}

	LCD_DEBUG("hmt_bl_level : %d candela : %dCD\n", vdd->hmt_stat.hmt_bl_level, vdd->hmt_stat.candela_level_hmt);

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi_hmt->generate_gamma)) {
		LCD_ERR("generate_gamma is NULL");
		return NULL;
	} else {
		vdd->smart_dimming_dsi_hmt->generate_gamma(
			vdd->smart_dimming_dsi_hmt,
			vdd->hmt_stat.candela_level_hmt,
			&hmt_gamma_cmds->cmds[0].msg.tx_buf[1]);

		*level_key = LEVEL1_KEY;

		return hmt_gamma_cmds;
	}
}

static struct dsi_panel_cmd_set hmt_aid_cmd;
static struct dsi_panel_cmd_set *ss_aid_hmt(
		struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set  *hmt_aid_cmds = ss_get_cmds(vdd, TX_HMT_AID);
	int cmd_idx = 0;

	if (!vdd->dtsi_data.hmt_reverse_aid_map_table[vdd->panel_revision].size ||
		vdd->hmt_stat.cmd_idx_hmt > vdd->dtsi_data.hmt_reverse_aid_map_table[vdd->panel_revision].size)
		goto end;

	cmd_idx = vdd->dtsi_data.hmt_reverse_aid_map_table[vdd->panel_revision].cmd_idx[vdd->hmt_stat.cmd_idx_hmt];

	hmt_aid_cmd.cmds = &hmt_aid_cmds->cmds[cmd_idx];
	hmt_aid_cmd.count = 1;

	*level_key = LEVEL1_KEY;

	return &hmt_aid_cmd;

end:
	LCD_ERR("error");
	return NULL;
}

static struct dsi_panel_cmd_set *ss_elvss_hmt(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *elvss_cmds = ss_get_cmds(vdd, TX_HMT_ELVSS);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(elvss_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)elvss_cmds);
		return NULL;
	}

	/* 0xB5 1th TSET */
	elvss_cmds->cmds[0].msg.tx_buf[1] =
		vdd->temperature > 0 ? vdd->temperature : 0x80|(-1*vdd->temperature);

	/* ELVSS(MPS_CON) setting condition is equal to normal birghtness */ // B5 2nd para : MPS_CON
	if (vdd->hmt_stat.candela_level_hmt > 40)
		elvss_cmds->cmds[0].msg.tx_buf[2] = 0xDC;
	else
		elvss_cmds->cmds[0].msg.tx_buf[2] = 0xCC;

	*level_key = LEVEL1_KEY;

	return elvss_cmds;
}

static void ss_make_sdimconf_hmt(struct samsung_display_driver_data *vdd)
{
	/* Set the mtp read buffer pointer and read the NVM value*/
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP,
			vdd->smart_dimming_dsi_hmt->mtp_buffer, LEVEL1_KEY);

	/* Initialize smart dimming related things here */
	/* lux_tab setting for 350cd */
	vdd->smart_dimming_dsi_hmt->lux_tab = vdd->dtsi_data.candela_map_table[HMT][vdd->panel_revision].cd;
	vdd->smart_dimming_dsi_hmt->lux_tabsize = vdd->dtsi_data.candela_map_table[HMT][vdd->panel_revision].tab_size;
	vdd->smart_dimming_dsi_hmt->man_id = vdd->manufacture_id_dsi;
	if (vdd->panel_func.samsung_panel_revision)
			vdd->smart_dimming_dsi_hmt->panel_revision = vdd->panel_func.samsung_panel_revision(vdd);

	/* Just a safety check to ensure smart dimming data is initialised well */
	vdd->smart_dimming_dsi_hmt->init(vdd->smart_dimming_dsi_hmt);

	LCD_INFO("[HMT] smart dimming done!\n");
}

static int ss_samart_dimming_init_hmt(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("DSI%d : ++\n", vdd->ndx);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	vdd->smart_dimming_dsi_hmt = vdd->panel_func.samsung_smart_get_conf_hmt();

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi_hmt)) {
		LCD_ERR("DSI%d error", vdd->ndx);
		return false;
	} else {
		vdd->hmt_stat.hmt_on = 0;
		vdd->hmt_stat.hmt_bl_level = 0;
		vdd->hmt_stat.hmt_reverse = 0;
		vdd->hmt_stat.hmt_is_first = 1;

		ss_make_sdimconf_hmt(vdd);

		vdd->smart_dimming_hmt_loaded_dsi = true;
	}

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

	/* Init alpm_brightness and alpm_ctrl cmds */
	alpm_brightness[LPM_2NIT_IDX] = ss_get_cmds(vdd, TX_LPM_2NIT_CMD);
	alpm_brightness[LPM_10NIT_IDX] = ss_get_cmds(vdd, TX_LPM_10NIT_CMD);
	alpm_brightness[LPM_30NIT_IDX] = ss_get_cmds(vdd, TX_LPM_30NIT_CMD);
	alpm_brightness[LPM_60NIT_IDX] = ss_get_cmds(vdd, TX_LPM_60NIT_CMD);

	alpm_ctrl[ALPM_CTRL_2NIT] = ss_get_cmds(vdd, TX_ALPM_2NIT_CMD);
	alpm_ctrl[ALPM_CTRL_10NIT] = ss_get_cmds(vdd, TX_ALPM_10NIT_CMD);
	alpm_ctrl[ALPM_CTRL_30NIT] = ss_get_cmds(vdd, TX_ALPM_30NIT_CMD);
	alpm_ctrl[ALPM_CTRL_60NIT] = ss_get_cmds(vdd, TX_ALPM_60NIT_CMD);
	alpm_ctrl[HLPM_CTRL_2NIT] = ss_get_cmds(vdd, TX_HLPM_2NIT_CMD);
	alpm_ctrl[HLPM_CTRL_10NIT] = ss_get_cmds(vdd, TX_HLPM_10NIT_CMD);
	alpm_ctrl[HLPM_CTRL_30NIT] = ss_get_cmds(vdd, TX_HLPM_30NIT_CMD);
	alpm_ctrl[HLPM_CTRL_60NIT] = ss_get_cmds(vdd, TX_HLPM_60NIT_CMD);

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
	off_cmd_list[0] = ss_get_cmds(vdd, TX_LPM_OFF);

	/* Init alpm_brightness and alpm_ctrl cmds */
	alpm_brightness[LPM_2NIT_IDX] = ss_get_cmds(vdd, TX_LPM_2NIT_CMD);
	alpm_brightness[LPM_10NIT_IDX] = ss_get_cmds(vdd, TX_LPM_10NIT_CMD);
	alpm_brightness[LPM_30NIT_IDX] = ss_get_cmds(vdd, TX_LPM_30NIT_CMD);
	alpm_brightness[LPM_60NIT_IDX] = ss_get_cmds(vdd, TX_LPM_60NIT_CMD);

	alpm_ctrl[ALPM_CTRL_2NIT] = ss_get_cmds(vdd, TX_ALPM_2NIT_CMD);
	alpm_ctrl[ALPM_CTRL_10NIT] = ss_get_cmds(vdd, TX_ALPM_10NIT_CMD);
	alpm_ctrl[ALPM_CTRL_30NIT] = ss_get_cmds(vdd, TX_ALPM_30NIT_CMD);
	alpm_ctrl[ALPM_CTRL_60NIT] = ss_get_cmds(vdd, TX_ALPM_60NIT_CMD);
	alpm_ctrl[HLPM_CTRL_2NIT] = ss_get_cmds(vdd, TX_HLPM_2NIT_CMD);
	alpm_ctrl[HLPM_CTRL_10NIT] = ss_get_cmds(vdd, TX_HLPM_10NIT_CMD);
	alpm_ctrl[HLPM_CTRL_30NIT] = ss_get_cmds(vdd, TX_HLPM_30NIT_CMD);
	alpm_ctrl[HLPM_CTRL_60NIT] = ss_get_cmds(vdd, TX_HLPM_60NIT_CMD);
	alpm_off_ctrl[ALPM_MODE_ON] = ss_get_cmds(vdd, TX_ALPM_OFF);
	alpm_off_ctrl[HLPM_MODE_ON] = ss_get_cmds(vdd, TX_HLPM_OFF);

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

	mdnie_data->DSI_BYPASS_MDNIE = DSI0_BYPASS_MDNIE;
	mdnie_data->DSI_NEGATIVE_MDNIE = DSI0_NEGATIVE_MDNIE;
	mdnie_data->DSI_COLOR_BLIND_MDNIE = DSI0_COLOR_BLIND_MDNIE;
	mdnie_data->DSI_HBM_CE_MDNIE = DSI0_HBM_CE_MDNIE;
	mdnie_data->DSI_RGB_SENSOR_MDNIE = DSI0_RGB_SENSOR_MDNIE;
	mdnie_data->DSI_GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
	mdnie_data->DSI_GRAYSCALE_NEGATIVE_MDNIE = DSI0_GRAYSCALE_NEGATIVE_MDNIE;
	mdnie_data->DSI_CURTAIN = DSI0_SCREEN_CURTAIN_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR = DSI0_NIGHT_MODE_MDNIE_1;
	mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR = DSI0_RGB_SENSOR_MDNIE_1;
	mdnie_data->DSI_COLOR_LENS_MDNIE = DSI0_COLOR_LENS_MDNIE;
	mdnie_data->DSI_COLOR_LENS_MDNIE_SCR = DSI0_COLOR_LENS_MDNIE_1;

	mdnie_data->hdr_tune_value_dsi = hdr_tune_value_dsi0;
	mdnie_data->mdnie_tune_value_dsi = mdnie_tune_value_dsi0;
	mdnie_data->hmt_color_temperature_tune_value_dsi = hmt_color_temperature_tune_value_dsi0;

	/* Update MDNIE data related with size, offset or index */
	mdnie_data->dsi_bypass_mdnie_size = ARRAY_SIZE(DSI0_BYPASS_MDNIE);
	mdnie_data->mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
	mdnie_data->mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP3] = MDNIE_STEP3_INDEX;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = ADDRESS_SCR_WHITE_RED;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = ADDRESS_SCR_WHITE_GREEN;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = ADDRESS_SCR_WHITE_BLUE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
	mdnie_data->dsi_rgb_sensor_mdnie_1_size = DSI0_RGB_SENSOR_MDNIE_1_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_2_size = DSI0_RGB_SENSOR_MDNIE_2_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_3_size = DSI0_RGB_SENSOR_MDNIE_3_SIZE;
	mdnie_data->dsi_trans_dimming_data_index = MDNIE_TRANS_DIMMING_DATA_INDEX;
	mdnie_data->dsi_adjust_ldu_table = adjust_ldu_data;
	mdnie_data->dsi_max_adjust_ldu = 6;
	mdnie_data->dsi_night_mode_table = night_mode_data;
	mdnie_data->dsi_max_night_mode_index = 12;
	mdnie_data->dsi_color_lens_table = color_lens_data;
	mdnie_data->dsi_white_default_r = 0xff;
	mdnie_data->dsi_white_default_g = 0xff;
	mdnie_data->dsi_white_default_b = 0xff;
	mdnie_data->dsi_scr_step_index = MDNIE_STEP1_INDEX;
	mdnie_data->light_notification_tune_value_dsi = light_notification_tune_value_dsi0;

	vdd->mdnie.mdnie_data = mdnie_data;

	return 0;
}

static int ss_self_display_data_init(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR("Self Display is not supported\n");
		return -EINVAL;
	}

	LCD_INFO("Self Display Panel Data init\n");

	/* SELF DISPLAY */
	vdd->self_disp.operation[FLAG_SELF_MASK].img_buf = self_mask_img_data;
	vdd->self_disp.operation[FLAG_SELF_MASK].img_size = ARRAY_SIZE(self_mask_img_data);
	make_self_dispaly_img_cmds_FA7(vdd, TX_SELF_MASK_IMAGE, FLAG_SELF_MASK);
	vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum = SELF_MASK_IMG_CHECKSUM;

	vdd->self_disp.operation[FLAG_SELF_ICON].img_buf = self_icon_img_data;
	vdd->self_disp.operation[FLAG_SELF_ICON].img_size = ARRAY_SIZE(self_icon_img_data);
	make_self_dispaly_img_cmds_FA7(vdd, TX_SELF_ICON_IMAGE, FLAG_SELF_ICON);

	vdd->self_disp.operation[FLAG_SELF_ACLK].img_buf = self_aclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_ACLK].img_size = ARRAY_SIZE(self_aclock_img_data);
	make_self_dispaly_img_cmds_FA7(vdd, TX_SELF_ACLOCK_IMAGE, FLAG_SELF_ACLK);

	vdd->self_disp.operation[FLAG_SELF_DCLK].img_buf = self_dclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_DCLK].img_size = ARRAY_SIZE(self_dclock_img_data);
	make_self_dispaly_img_cmds_FA7(vdd, TX_SELF_DCLOCK_IMAGE, FLAG_SELF_DCLK);


	return 1;
}


static void samsung_panel_init(struct samsung_display_driver_data *vdd)
{
	LCD_ERR("%s\n", ss_get_panel_name(vdd));

	/* Default Panel Power Status is OFF */
	vdd->panel_state = PANEL_PWR_OFF;

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = samsung_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = samsung_panel_on_post;

	/* DDI RX */
	vdd->panel_func.samsung_panel_revision = ss_panel_revision;
	vdd->panel_func.samsung_manufacture_date_read = ss_manufacture_date_read;
	vdd->panel_func.samsung_ddi_id_read = ss_ddi_id_read;
	vdd->panel_func.samsung_cell_id_read = ss_cell_id_read;
	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;
	vdd->panel_func.samsung_elvss_read = ss_elvss_read;
	vdd->panel_func.samsung_hbm_read = ss_hbm_read;
	vdd->panel_func.samsung_mdnie_read = ss_mdnie_read;
	vdd->panel_func.samsung_irc_read = ss_irc_read;

	vdd->panel_func.samsung_smart_dimming_init = ss_smart_dimming_init;

	vdd->panel_func.samsung_smart_get_conf = smart_get_conf_S6E3FA7_AMS559NK01;

	/* Brightness */
	vdd->panel_func.samsung_brightness_hbm_off = NULL;
	vdd->panel_func.samsung_brightness_aid = ss_aid;
	vdd->panel_func.samsung_brightness_acl_on = ss_acl_on;
	vdd->panel_func.samsung_brightness_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_elvss = ss_elvss;
	vdd->panel_func.samsung_brightness_elvss_temperature1 = NULL;
	vdd->panel_func.samsung_brightness_elvss_temperature2 = NULL;
	vdd->panel_func.samsung_brightness_vint = ss_vint;
	vdd->panel_func.samsung_brightness_irc = ss_irc;
	vdd->panel_func.samsung_brightness_gamma = ss_gamma;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_hbm_gamma;
	vdd->panel_func.samsung_hbm_etc = ss_hbm_etc;
	vdd->panel_func.samsung_hbm_irc = ss_hbm_irc;
	vdd->panel_func.get_hbm_candela_value = get_hbm_candela_value;

	/* HMT */
	vdd->panel_func.samsung_brightness_gamma_hmt = ss_gamma_hmt;
	vdd->panel_func.samsung_brightness_aid_hmt = ss_aid_hmt;
	vdd->panel_func.samsung_brightness_elvss_hmt = ss_elvss_hmt;
	vdd->panel_func.samsung_brightness_vint_hmt = NULL;
	vdd->panel_func.samsung_smart_dimming_hmt_init = ss_samart_dimming_init_hmt;
	vdd->panel_func.samsung_smart_get_conf_hmt = smart_get_conf_S6E3FA7_AMS559NK01_hmt;

	/* Panel LPM */
	vdd->panel_func.samsung_update_lpm_ctrl_cmd = ss_update_panel_lpm_ctrl_cmd;
	vdd->panel_func.samsung_set_lpm_brightness = ss_set_panel_lpm_brightness;

	/* default brightness */
	vdd->br.bl_level = 255;

	/* mdnie */
	vdd->mdnie.support_mdnie = true;
	vdd->mdnie.support_trans_dimming = true;
	/* for mdnie tuning */
#if 0
	vdd->mdnie_tune_size1 = sizeof(DSI0_BYPASS_MDNIE_1);
	vdd->mdnie_tune_size2 = sizeof(DSI0_BYPASS_MDNIE_2);
	vdd->mdnie_tune_size3 = sizeof(DSI0_BYPASS_MDNIE_3);
#endif

	dsi_update_mdnie_data(vdd);

	/* send recovery pck before sending image date (for ESD recovery) */
	vdd->esd_recovery.send_esd_recovery = false;

	/* Support DDI HW CURSOR */
	vdd->panel_func.ddi_hw_cursor = NULL;

	/* Enable panic on first pingpong timeout */
	if (vdd->debug_data)
		vdd->debug_data->panic_on_pptimeout = true;

	/* COLOR WEAKNESS */
	vdd->panel_func.color_weakness_ccb_on_off = NULL;

	/* Support DDI HW CURSOR */
	vdd->panel_func.ddi_hw_cursor = NULL;

	/* ACL default ON */
	vdd->acl_status = 1;

	/* Self display */
	vdd->self_disp.is_support = true;
	vdd->self_disp.init = self_display_init_FA7;
	vdd->self_disp.data_init = ss_self_display_data_init;
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_S6E3FA7_AMS559NK01_FHD";
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
