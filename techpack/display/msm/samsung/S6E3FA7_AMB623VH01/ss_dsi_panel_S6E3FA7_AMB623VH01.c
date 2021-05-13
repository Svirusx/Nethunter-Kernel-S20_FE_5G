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
#include "ss_dsi_panel_S6E3FA7_AMB623VH01.h"
#include "ss_dsi_mdnie_S6E3FA7_AMB623VH01.h"

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
#define ALPM_CTRL_REG	0xB9

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
		vdd->panel_revision = 'A';
		break;
	default:
		vdd->panel_revision = 'A';
		LCD_ERR("Invalid panel_rev(default rev : %c)\n",
				vdd->panel_revision);
		break;
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
			LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_NORMAL);

	tset =	vdd->br_info.temperature > 0 ?	vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

	pcmds->cmds[0].msg.tx_buf[1] = tset;

	pcmds->cmds[4].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 3);
	pcmds->cmds[4].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);
	LCD_INFO("cd_idx: %d, cd_level: %d, WRDISBV: %x %x, elvss_dim_offset: %x, tset = 0x%x\n",
			vdd->br_info.common_br.cd_idx,
			vdd->br_info.common_br.cd_level,
			pcmds->cmds[4].msg.tx_buf[1],
			pcmds->cmds[4].msg.tx_buf[2],
			pcmds->cmds[0].msg.tx_buf[3],
			pcmds->cmds[0].msg.tx_buf[1]);
	*level_key = LEVEL1_KEY;

	return pcmds;
}

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_hbm(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;
	int tset;

	if (IS_ERR_OR_NULL(vdd)) {
			LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HBM);

	tset =	vdd->br_info.temperature > 0 ?	vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

	pcmds->cmds[2].msg.tx_buf[1] = tset;

	pcmds->cmds[2].msg.tx_buf[3] = elvss_table_hbm_revA[vdd->br_info.common_br.cd_idx]; 	/* ELVSS Value for HBM brgihtness */

	/* 29 00 00 00 00 00 04 B1 11 70 7B			AOR interpolation */
	/* FA7_AMS623 uses same wrdisbv for HBM & interpolate aor */
	pcmds->cmds[1].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 3) << 4 | 0x01;
	pcmds->cmds[1].msg.tx_buf[3] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);
	LCD_INFO("cd_idx: %d, cd_level: %d, AOR 1st : %x, AOR 3rd : %x elvss_dim_offset: %x, tset = 0x%x\n",
			vdd->br_info.common_br.cd_idx,
			vdd->br_info.common_br.cd_level,
			pcmds->cmds[1].msg.tx_buf[1],
			pcmds->cmds[1].msg.tx_buf[3],
			pcmds->cmds[2].msg.tx_buf[3], 
			pcmds->cmds[2].msg.tx_buf[1]);
	*level_key = LEVEL1_KEY;

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

#undef COORDINATE_DATA_SIZE
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
	{0xff, 0x00, 0xf9, 0x00, 0xf4, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfa, 0x00, 0xf7, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfc, 0x00, 0xfa, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfd, 0x00, 0xfa, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfd, 0x00, 0xf5, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfd, 0x00, 0xf7, 0x00}, /* Tune_8 */
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
	//mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE_2 = DSI0_EBOOK_DYNAMIC_MDNIE_2;
	//mdnie_data->DSI_EBOOK_STANDARD_MDNIE_2 = DSI0_EBOOK_STANDARD_MDNIE_2;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE_2 = DSI0_EBOOK_AUTO_MDNIE_2;
	//mdnie_data->DSI_TDMB_DYNAMIC_MDNIE_2 = DSI0_TDMB_DYNAMIC_MDNIE_2;
	//mdnie_data->DSI_TDMB_STANDARD_MDNIE_2 = DSI0_TDMB_STANDARD_MDNIE_2;
	//mdnie_data->DSI_TDMB_AUTO_MDNIE_2 = DSI0_TDMB_AUTO_MDNIE_2;

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
	//mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE = DSI0_EBOOK_DYNAMIC_MDNIE;
	//mdnie_data->DSI_EBOOK_STANDARD_MDNIE = DSI0_EBOOK_STANDARD_MDNIE;
	//mdnie_data->DSI_EBOOK_NATURAL_MDNIE = DSI0_EBOOK_NATURAL_MDNIE;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE = DSI0_EBOOK_AUTO_MDNIE;
	mdnie_data->DSI_EMAIL_AUTO_MDNIE = DSI0_EMAIL_AUTO_MDNIE;
	mdnie_data->DSI_GAME_LOW_MDNIE = DSI0_GAME_LOW_MDNIE;
	mdnie_data->DSI_GAME_MID_MDNIE = DSI0_GAME_MID_MDNIE;
	mdnie_data->DSI_GAME_HIGH_MDNIE = DSI0_GAME_HIGH_MDNIE;
	//mdnie_data->DSI_TDMB_DYNAMIC_MDNIE = DSI0_TDMB_DYNAMIC_MDNIE;
	//mdnie_data->DSI_TDMB_STANDARD_MDNIE = DSI0_TDMB_STANDARD_MDNIE;
	//mdnie_data->DSI_TDMB_NATURAL_MDNIE = DSI0_TDMB_NATURAL_MDNIE;
	//mdnie_data->DSI_TDMB_AUTO_MDNIE = DSI0_TDMB_AUTO_MDNIE;
	mdnie_data->DSI_GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
	mdnie_data->DSI_GRAYSCALE_NEGATIVE_MDNIE = DSI0_GRAYSCALE_NEGATIVE_MDNIE;
	mdnie_data->DSI_CURTAIN = DSI0_SCREEN_CURTAIN_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR = DSI0_NIGHT_MODE_MDNIE_1;
	mdnie_data->DSI_COLOR_LENS_MDNIE = DSI0_COLOR_LENS_MDNIE;
	mdnie_data->DSI_COLOR_LENS_MDNIE_SCR = DSI0_COLOR_LENS_MDNIE_1;
	mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR = DSI0_RGB_SENSOR_MDNIE_1;
	//mdnie_data->DSI_AFC = DSI0_AFC;
	//mdnie_data->DSI_AFC_ON = DSI0_AFC_ON;
	//mdnie_data->DSI_AFC_OFF = DSI0_AFC_OFF;

	mdnie_data->mdnie_tune_value_dsi = mdnie_tune_value_dsi0;
	mdnie_data->hmt_color_temperature_tune_value_dsi = hmt_color_temperature_tune_value_dsi0;

	mdnie_data->hdr_tune_value_dsi = hdr_tune_value_dsi0;

	mdnie_data->light_notification_tune_value_dsi = light_notification_tune_value_dsi0;

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
	mdnie_data->dsi_max_night_mode_index = 102;
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
	make_self_dispaly_img_cmds_FA7(vdd, TX_SELF_MASK_IMAGE, FLAG_SELF_MASK);

	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_buf = self_mask_img_crc_data;
	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_size = ARRAY_SIZE(self_mask_img_crc_data);
	make_self_dispaly_img_cmds_FA7(vdd, TX_SELF_MASK_IMAGE_CRC, FLAG_SELF_MASK_CRC);

	vdd->self_disp.operation[FLAG_SELF_ACLK].img_buf = self_aclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_ACLK].img_size = ARRAY_SIZE(self_aclock_img_data);
	make_self_dispaly_img_cmds_FA7(vdd, TX_SELF_ACLOCK_IMAGE, FLAG_SELF_ACLK);

	vdd->self_disp.operation[FLAG_SELF_DCLK].img_buf = self_dclock_img_data;
	vdd->self_disp.operation[FLAG_SELF_DCLK].img_size = ARRAY_SIZE(self_dclock_img_data);
	make_self_dispaly_img_cmds_FA7(vdd, TX_SELF_DCLOCK_IMAGE, FLAG_SELF_DCLK);

	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_buf = self_video_img_data;
	vdd->self_disp.operation[FLAG_SELF_VIDEO].img_size = ARRAY_SIZE(self_video_img_data);
	make_self_dispaly_img_cmds_FA7(vdd, TX_SELF_VIDEO_IMAGE, FLAG_SELF_VIDEO);

	return 1;
}

static void ss_copr_panel_init(struct samsung_display_driver_data *vdd)
{
	vdd->copr.ver = COPR_VER_5P0;
	vdd->copr.display_read = 0;
	ss_copr_init(vdd);
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

	pcmds->cmds[0].msg.tx_buf[2] = 0x80;	/* start point 60% 80 64 */
	pcmds->cmds[0].msg.tx_buf[3] = 0X64;
	pcmds->cmds[0].msg.tx_buf[5] = 0X29;	/* ACL 8% */

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

	pcmds->cmds[0].msg.tx_buf[2] = 0x40;	/* start point 50% 40 FC */
	pcmds->cmds[0].msg.tx_buf[3] = 0XFC;
	pcmds->cmds[0].msg.tx_buf[5] = 0X4D;	/* ACL 15% */

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

static int ss_octa_id_read(struct samsung_display_driver_data *vdd)
{
	int octa_id_offset = 0;
	int id_cnt[5] = {4,3,2,2,6};
	int id_idx = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read Panel Unique OCTA ID (A1h 5~8/ CBh 30~32/ CBh 42~43/ CBh 84~85/ CBh 88~93/ B5h 9~11) */
	if (ss_get_cmds(vdd, RX_OCTA_ID)->count) {
		memset(vdd->octa_id_dsi, 0x00, MAX_OCTA_ID);

		ss_panel_data_read(vdd, RX_OCTA_ID,
				&vdd->octa_id_dsi[octa_id_offset], LEVEL1_KEY);
		octa_id_offset += id_cnt[id_idx++];
		ss_panel_data_read(vdd, RX_OCTA_ID1,
				&vdd->octa_id_dsi[octa_id_offset], LEVEL1_KEY);
		octa_id_offset += id_cnt[id_idx++];
		ss_panel_data_read(vdd, RX_OCTA_ID2,
				&vdd->octa_id_dsi[octa_id_offset], LEVEL1_KEY);
		octa_id_offset += id_cnt[id_idx++];
		ss_panel_data_read(vdd, RX_OCTA_ID3,
				&vdd->octa_id_dsi[octa_id_offset], LEVEL1_KEY);
		octa_id_offset += id_cnt[id_idx++];
		ss_panel_data_read(vdd, RX_OCTA_ID4,
				&vdd->octa_id_dsi[octa_id_offset], LEVEL1_KEY);
		octa_id_offset += id_cnt[id_idx++];
		ss_panel_data_read(vdd, RX_OCTA_ID5,
				&vdd->octa_id_dsi[octa_id_offset], LEVEL1_KEY);

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

static struct dsi_panel_cmd_set *ss_vint(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *vint_cmds = ss_get_cmds(vdd, TX_VINT);

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(vint_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)vint_cmds);
		return NULL;
	}

	if (vdd->xtalk_mode) {
		vint_cmds->cmds[1].msg.tx_buf[1] = 0x6B;	/* VGH 6.2 V */
		LCD_INFO("xtalk_mode on\n");
	}
	else {
		vint_cmds->cmds[1].msg.tx_buf[1] = 0xCB;	/* VGH return V */
	}

	return vint_cmds;
}

enum LPMBL_CMD_ID {
	LPMBL_CMDID_CTRL = 1,
	LPMBL_CMDID_LPM_ON = 2,
};
static void ss_set_panel_lpm_brightness(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *set_lpm = ss_get_cmds(vdd, TX_LPM_BL_CMD);
	struct dsi_panel_cmd_set *set_lpm_bl;

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

	/* send lpm bl cmd */
	ss_send_cmd(vdd, TX_LPM_BL_CMD);

	LCD_INFO("[Panel LPM] bl_level : %s\n",
				/* Check current brightness level */
				vdd->panel_lpm.lpm_bl_level == LPM_2NIT ? "2NIT" :
				vdd->panel_lpm.lpm_bl_level == LPM_10NIT ? "10NIT" :
				vdd->panel_lpm.lpm_bl_level == LPM_30NIT ? "30NIT" :
				vdd->panel_lpm.lpm_bl_level == LPM_60NIT ? "60NIT" : "UNKNOWN");
}

enum LPMON_CMD_ID {
	LPMON_CMDID_CTRL = 2,
	LPMON_CMDID_LPM_ON = 3,
};
/*
 * This function will update parameters for ALPM_REG/ALPM_CTRL_REG
 * Parameter for ALPM_REG : Control brightness for panel LPM
 * Parameter for ALPM_CTRL_REG : Change panel LPM mode for ALPM/HLPM
 * mode, brightness, hz are updated here.
 */
static void ss_update_panel_lpm_ctrl_cmd(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *set_lpm_on = ss_get_cmds(vdd, TX_LPM_ON);
	struct dsi_panel_cmd_set *set_lpm_bl;

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

static void ss_grayspot_etc(struct samsung_display_driver_data *vdd, int enable)
{
	struct dsi_panel_cmd_set *pcmds;
	static bool is_first = true;
	static char B5_23TH[1];

	if (is_first) {
		ss_panel_data_read(vdd, RX_GRAYSPOT_RESTORE_VALUE, B5_23TH, LEVEL1_KEY);
		is_first = false;
	}

	if(!enable)
	{
		pcmds = ss_get_cmds(vdd, TX_GRAY_SPOT_TEST_OFF);
		pcmds->cmds[7].msg.tx_buf[1] = B5_23TH[0];
	}
	return;
}

static void ss_mcd_etc(struct samsung_display_driver_data *vdd, int enable)
{
	if (!enable)
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	return;
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
		poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[2] = 0xD8;
	} else if (erase_size == POC_ERASE_32KB) {
		delay_us = 800000; /* 800ms */
		poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[2] = 0x52;
	} else {
		delay_us = 400000; /* 400ms */
		poc_erase_sector_tx_cmds->cmds[2].msg.tx_buf[2] = 0x20;
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
	struct dsi_panel_cmd_set *write_cmd = NULL;
	struct dsi_panel_cmd_set *write_data_add = NULL;
	struct dsi_display *display = NULL;
	int pos, type, ret = 0;
	int last_pos, delay_us, image_size, loop_cnt, poc_w_size;
	int tx_size;
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

	write_cmd = ss_get_cmds(vdd, TX_POC_WRITE_LOOP_1BYTE);
	if (SS_IS_CMDS_NULL(write_cmd)) {
		LCD_ERR("no cmds for TX_POC_WRITE_LOOP_1BYTE..\n");
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

		/* 1 Byte Write */
		tx_size = poc_w_size;

		/* data copy */
		write_cmd->cmds[0].msg.tx_buf[1] = data[pos];

		if (ss_send_cmd(vdd, TX_POC_WRITE_LOOP_1BYTE)) {
			ret = -EIO;
			goto cancel_poc;
		}
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

	/* Before read poc data, need to read mca (only for B0) */
	if (read_pos == 0) {
		if (vdd->poc_driver.check_read_case) {
			if (vdd->poc_driver.read_case == READ_CASE1) {
				LCD_INFO("WRITE [TX_POC_PRE_READ]");
				ss_send_cmd(vdd, TX_POC_PRE_READ);
			} else if (vdd->poc_driver.read_case == READ_CASE2) {
				LCD_INFO("WRITE [TX_POC_PRE_READ2]");
				ss_send_cmd(vdd, TX_POC_PRE_READ2);
			}
		} else {
			LCD_INFO("WRITE [TX_POC_PRE_READ]");
			ss_send_cmd(vdd, TX_POC_PRE_READ);
		}
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

/*
 * check 5th 6th value of ECh every sleep out
 * need to save values because these values could be reset after manual read.
 */
static int check_read_case(struct samsung_display_driver_data *vdd)
{
	ss_poc_read_mca(vdd);

	if (vdd->poc_driver.mca_data[4] == 0x00 &&
		vdd->poc_driver.mca_data[5] == 0x00) {
		LCD_INFO("FLASH READ_CASE1\n");
		return READ_CASE1;
	} else {
		LCD_INFO("FLASH READ_CASE2\n");
		return READ_CASE2;
	}
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
	vdd->panel_func.samsung_brightness_vrr = NULL;
	vdd->br_info.smart_dimming_loaded_dsi = false;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_brightness_gamma_mode2_hbm;
	vdd->panel_func.samsung_hbm_acl_on = ss_acl_on_hbm;
	vdd->panel_func.samsung_hbm_acl_off = ss_acl_off;
	vdd->panel_func.samsung_hbm_etc = NULL;
	vdd->panel_func.samsung_hbm_irc = NULL;
	vdd->panel_func.get_hbm_candela_value = NULL;

	/* Total level key in brightness */
	vdd->panel_func.samsung_brightness_tot_level = ss_brightness_tot_level;

	/* Event */
	vdd->panel_func.samsung_change_ldi_fps = NULL;

	/* Panel LPM */
	vdd->panel_func.samsung_update_lpm_ctrl_cmd = ss_update_panel_lpm_ctrl_cmd;
	vdd->panel_func.samsung_set_lpm_brightness = ss_set_panel_lpm_brightness;

	/* Grayspot test */
	vdd->panel_func.samsung_gray_spot = ss_grayspot_etc;

	/* mcd test */
	vdd->panel_func.samsung_mcd_etc = ss_mcd_etc;

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
	vdd->self_disp.init = self_display_init_FA7;
	vdd->self_disp.data_init = ss_self_display_data_init;

	/* POC */
	vdd->poc_driver.poc_erase = poc_erase;
	vdd->poc_driver.poc_write = poc_write;
	vdd->poc_driver.poc_read = poc_read;
	vdd->poc_driver.poc_comp = poc_comp;
	vdd->poc_driver.check_read_case = check_read_case;
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_S6E3FA7_AMB623VH01_HD";
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
