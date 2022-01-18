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
#include "ss_dsi_panel_S6E3FC3_AMS646YD01.h"
#include "ss_dsi_mdnie_S6E3FC3_AMS646YD01.h"

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
	/* Module info */
	if (!vdd->module_info_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_module_info_read))
			LCD_ERR("no samsung_module_info_read function\n");
		else
			vdd->module_info_loaded_dsi = vdd->panel_func.samsung_module_info_read(vdd);
	}

	/* Manufacture date */
	if (!vdd->manufacture_date_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_manufacture_date_read))
			LCD_ERR("no samsung_manufacture_date_read function\n");
		else
			vdd->manufacture_date_loaded_dsi = vdd->panel_func.samsung_manufacture_date_read(vdd);
	}

	/* DDI ID */
	if (!vdd->ddi_id_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_ddi_id_read))
			LCD_ERR("no samsung_ddi_id_read function\n");
		else
			vdd->ddi_id_loaded_dsi = vdd->panel_func.samsung_ddi_id_read(vdd);
	}

	/* MDNIE X,Y (1.Manufacture Date -> 2.MDNIE X,Y -> 3.Cell ID -> 4.OCTA ID) */
	if (!vdd->mdnie_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_mdnie_read))
			LCD_ERR("no samsung_mdnie_read function\n");
		else
			vdd->mdnie_loaded_dsi = vdd->panel_func.samsung_mdnie_read(vdd);
	}

	/* Panel Unique Cell ID (1.Manufacture Date -> 2.MDNIE X,Y -> 3.Cell ID -> 4.OCTA ID) */
	if (!vdd->cell_id_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_cell_id_read))
			LCD_ERR("no samsung_cell_id_read function\n");
		else
			vdd->cell_id_loaded_dsi = vdd->panel_func.samsung_cell_id_read(vdd);
	}

	/* Panel Unique OCTA ID (1.Manufacture Date -> 2.MDNIE X,Y -> 3.Cell ID -> 4.OCTA ID) */
	if (!vdd->octa_id_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_octa_id_read))
			LCD_ERR("no samsung_octa_id_read function\n");
		else
			vdd->octa_id_loaded_dsi = vdd->panel_func.samsung_octa_id_read(vdd);
	}

	/* self mask cmd send again under splash mode(cause,sleep out cmd) */
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
		vdd->panel_revision = 'A';
		break;
	case 0x01:
		vdd->panel_revision = 'B';
		break;
	case 0x02 ... 0x05:
		vdd->panel_revision = 'C';
		break;
	case 0x06 ... 0x09:
		vdd->panel_revision = 'G';
		break;
	case 0x0A:
		vdd->panel_revision = 'I';
		break;
	default:
		vdd->panel_revision = 'I';
		LCD_ERR("Invalid panel_rev(default rev : %c)\n",
				vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';

	LCD_INFO_ONCE("panel_revision = %c %d \n",
					vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision + 'A');
}

static struct dsi_panel_cmd_set *__ss_vrr(struct samsung_display_driver_data *vdd,
					int *level_key, bool is_hbm, bool is_hmt)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_panel_cmd_set  *vrr_cmds = ss_get_cmds(vdd, TX_VRR);
	struct vrr_info *vrr = &vdd->vrr;

	int cur_rr;
	bool cur_hs;

	if (SS_IS_CMDS_NULL(vrr_cmds)) {
		LCD_INFO("no vrr cmds\n");
		return NULL;
	}

	if (panel && panel->cur_mode) {
		LCD_INFO("VRR: cur_mode: %dx%d@%d%s, is_hbm: %d\n",
				panel->cur_mode->timing.h_active,
				panel->cur_mode->timing.v_active,
				panel->cur_mode->timing.refresh_rate,
				panel->cur_mode->timing.sot_hs_mode ? "HS" : "NM",
				is_hbm);

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

	if (cur_rr == 60) {
		vrr_cmds->cmds[0].msg.tx_buf[1] = 0x00; /* 60 HZ */
	} else {
		vrr_cmds->cmds[0].msg.tx_buf[1] = 0x08; /* 120 HZ */
	}

	LCD_INFO("VRR: (cur: %d%s, adj: %d%s)\n",
			cur_rr,
			cur_hs ? "HS" : "NM",
			vrr->adjusted_refresh_rate,
			vrr->adjusted_sot_hs_mode ? "HS" : "NM");

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

#define FRAME_WAIT_60FPS (17)
#define FRAME_WAIT_90FPS (12)
#define FRAME_WAIT_120FPS (9)
#define HBM_NORMAL_DELAY_60FPS (16)
#define HBM_NORMAL_DELAY_90FPS (3)
#define HBM_NORMAL_DELAY_120FPS (9)

static bool last_br_hbm;

#define get_bit(value, shift, width)	((value >> shift) & (GENMASK(width - 1, 0)))
static struct dsi_panel_cmd_set *ss_brightness_gamma_mode2_normal
							(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;
	int cd_index = vdd->br_info.common_br.cd_idx ;
	int finger_mask_update_delay;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(": Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return NULL;
	}

	if (cd_index > ARRAY_SIZE(normal_aor_manual)) {
		LCD_ERR(": Invalid data cd_index : %d array_max_size : %d\n",
			cd_index, ARRAY_SIZE(normal_aor_manual));
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_NORMAL);

	if (ss_panel_revision(vdd) >= 'I') {
		finger_mask_update_delay = vdd->vrr.cur_refresh_rate > 60 ? 9000 : 17000;
		if (pcmds->cmds[6].last_command == 1)
			usleep_range(finger_mask_update_delay, finger_mask_update_delay + 1);

		/*
			This cmd sustain 0x20 value before brightness change.
			This is display lab request point.
		*/
		pcmds->cmds[1].msg.tx_buf[1] = vdd->finger_mask_updated ? 0x20 : 0x60;

		/* Smooth transition : 0x28 */
		pcmds->cmds[4].msg.tx_buf[1] = vdd->finger_mask_updated ? 0x20 : 0x28;

		pcmds->cmds[5].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 8);
		pcmds->cmds[5].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

		pcmds->cmds[6].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
				vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

		if (vdd->finger_mask_updated) {
			/*
				There is panel limitation for HBM & AOR setting.
				TE ->hbm cmd excluded aor cmd -> delay(90 fps 3ms or 60 fps 9ms) ->
				aor cmd -> mask image
			*/
			pcmds->cmds[6].last_command = 1;

			if (vdd->vrr.cur_refresh_rate > 60)
				pcmds->cmds[6].post_wait_ms = HBM_NORMAL_DELAY_120FPS;
			else
				pcmds->cmds[6].post_wait_ms = HBM_NORMAL_DELAY_60FPS;
		} else {
			pcmds->cmds[6].last_command = 0;
			pcmds->cmds[6].post_wait_ms = 0;
		}

		pcmds->cmds[10].msg.tx_buf[1] = normal_aor_manual[cd_index].aor_63h_119;
		pcmds->cmds[10].msg.tx_buf[2] = normal_aor_manual[cd_index].aor_63h_120;
		pcmds->cmds[10].msg.tx_buf[3] = normal_aor_manual[cd_index].aor_63h_121;
	}
	else if (ss_panel_revision(vdd) >= 'G') {
		/* Smooth transition : 0x28 */
		pcmds->cmds[1].msg.tx_buf[1] = vdd->finger_mask_updated ? 0x20 : 0x28;

		pcmds->cmds[2].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 8);
		pcmds->cmds[2].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

		pcmds->cmds[3].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
				vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

		pcmds->cmds[5].msg.tx_buf[1] = normal_aor_manual[cd_index].aor_63h_119;
		pcmds->cmds[5].msg.tx_buf[2] = normal_aor_manual[cd_index].aor_63h_120;
		pcmds->cmds[5].msg.tx_buf[3] = normal_aor_manual[cd_index].aor_63h_121;
	} else {
		finger_mask_update_delay = vdd->vrr.cur_refresh_rate > 60 ? 9000 : 17000;
		if (pcmds->cmds[6].last_command == 1)
			usleep_range(finger_mask_update_delay, finger_mask_update_delay + 1);

		/*
		 * This cmd sustain 0x20 value before brightness change.
		 * This is display lab request point.
		*/
		pcmds->cmds[1].msg.tx_buf[1] = vdd->finger_mask_updated ? 0x20 : 0x60;

		/* Smooth transition : 0x28 */
		pcmds->cmds[4].msg.tx_buf[1] = vdd->finger_mask_updated ? 0x20 : 0x28;

		pcmds->cmds[5].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 8);
		pcmds->cmds[5].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

		pcmds->cmds[6].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
				vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

		if (vdd->finger_mask_updated) {
			/*
			 * There is panel limitation for HBM & AOR setting.
			 * TE ->hbm cmd excluded aor cmd -> delay(90 fps 3ms or 60 fps 9ms) ->
			 * aor cmd -> mask image
			*/
			pcmds->cmds[6].last_command = 1;

			if (vdd->vrr.cur_refresh_rate > 60)
				pcmds->cmds[6].post_wait_ms = HBM_NORMAL_DELAY_120FPS;
			else
				pcmds->cmds[6].post_wait_ms = HBM_NORMAL_DELAY_60FPS;
		} else {
			pcmds->cmds[6].last_command = 0;
			pcmds->cmds[6].post_wait_ms = 0;
		}
	}
	*level_key = LEVEL_KEY_NONE;
	last_br_hbm = false;

	return pcmds;
}

static struct dsi_panel_cmd_set *ss_brightness_gamma_mode2_hbm
							(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;
	struct dsi_panel_cmd_set *pcmds_smooth_off;
	int cd_index = vdd->br_info.common_br.cd_idx ;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(": Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return NULL;
	}

	if (cd_index > ARRAY_SIZE(hbm_aor_manual)) {
		LCD_ERR(": Invalid data cd_index : %d array_max_size : %d\n",
			cd_index, ARRAY_SIZE(hbm_aor_manual));
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HBM);
	pcmds_smooth_off = ss_get_cmds(vdd, TX_SMOOTH_DIMMING_OFF);

	if (ss_panel_revision(vdd) >= 'I') {
		pcmds->cmds[5].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 8);
		pcmds->cmds[5].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

		pcmds->cmds[6].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
				vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

		if (vdd->finger_mask_updated) {
			if (last_br_hbm == false) { /* Normal -> HBM Case Only */
				/* Smooth Dimming Off First */
				if (vdd->vrr.cur_refresh_rate > 60)
					pcmds_smooth_off->cmds[3].post_wait_ms = FRAME_WAIT_120FPS;
				else
					pcmds_smooth_off->cmds[3].post_wait_ms = FRAME_WAIT_60FPS;

				ss_send_cmd(vdd, TX_SMOOTH_DIMMING_OFF);
			}

			/*
			 * There is panel limitation for HBM & AOR setting.
			 * TE ->hbm cmd excluded aor cmd -> delay(90 fps 3ms or 60 fps 9ms) ->
			 * aor cmd -> mask image
			*/
			pcmds->cmds[6].last_command = 1;

			if (vdd->vrr.cur_refresh_rate > 60)
				pcmds->cmds[6].post_wait_ms = HBM_NORMAL_DELAY_120FPS;
			else
				pcmds->cmds[6].post_wait_ms = HBM_NORMAL_DELAY_60FPS;
		} else {
			pcmds->cmds[6].last_command = 0;
			pcmds->cmds[6].post_wait_ms = 0;
		}

		pcmds->cmds[10].msg.tx_buf[1] = hbm_aor_manual[cd_index].aor_63h_119;
		pcmds->cmds[10].msg.tx_buf[2] = hbm_aor_manual[cd_index].aor_63h_120;
		pcmds->cmds[10].msg.tx_buf[3] = hbm_aor_manual[cd_index].aor_63h_121;
	} else if (ss_panel_revision(vdd) >= 'G') {
		pcmds->cmds[2].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 8);
		pcmds->cmds[2].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

		pcmds->cmds[3].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
				vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

		pcmds->cmds[5].msg.tx_buf[1] = hbm_aor_manual[cd_index].aor_63h_119;
		pcmds->cmds[5].msg.tx_buf[2] = hbm_aor_manual[cd_index].aor_63h_120;
		pcmds->cmds[5].msg.tx_buf[3] = hbm_aor_manual[cd_index].aor_63h_121;
	} else {
		pcmds->cmds[5].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 8);
		pcmds->cmds[5].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

		pcmds->cmds[6].msg.tx_buf[1] = vdd->br_info.temperature > 0 ?
				vdd->br_info.temperature : (char)(BIT(7) | (-1 * vdd->br_info.temperature));

		if (vdd->finger_mask_updated) {
			/* Smooth Dimming Off First */
			if (vdd->vrr.cur_refresh_rate > 60)
				pcmds_smooth_off->cmds[3].post_wait_ms = FRAME_WAIT_120FPS;
			else
				pcmds_smooth_off->cmds[3].post_wait_ms = FRAME_WAIT_60FPS;

			ss_send_cmd(vdd, TX_SMOOTH_DIMMING_OFF);

			/*
			 * There is panel limitation for HBM & AOR setting.
			 * TE ->hbm cmd excluded aor cmd -> delay(90 fps 3ms or 60 fps 9ms) ->
			 * aor cmd -> mask image
			*/
			pcmds->cmds[6].last_command = 1;

			if (vdd->vrr.cur_refresh_rate > 60)
				pcmds->cmds[6].post_wait_ms = HBM_NORMAL_DELAY_120FPS;
			else
				pcmds->cmds[6].post_wait_ms = HBM_NORMAL_DELAY_60FPS;
		} else {
			pcmds->cmds[6].last_command = 0;
			pcmds->cmds[6].post_wait_ms = 0;
		}
	}

	*level_key = LEVEL_KEY_NONE;
	last_br_hbm = true;

	return pcmds;
}

#undef COORDINATE_DATA_SIZE
#define COORDINATE_DATA_SIZE 6

#define F1(x, y) ((y)-((39*(x))/38)-95)
#define F2(x, y) ((y)-((36*(x))/35)-56)
#define F3(x, y) ((y)+((7*(x))/1)-24728)
#define F4(x, y) ((y)+((25*(x))/7)-14031)

#if 0
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
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* dummy */
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_9 */
};

static char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE] = {
	coordinate_data_2, /* DYNAMIC - DCI */
	coordinate_data_2, /* STANDARD - sRGB/Adobe RGB */
	coordinate_data_2, /* NATURAL - sRGB/Adobe RGB */
	coordinate_data_1, /* MOVIE - Normal */
	coordinate_data_1, /* AUTO - Normal */
	coordinate_data_1, /* READING - Normal */
};
#endif

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
	return true;
}

static int ss_module_info_read(struct samsung_display_driver_data *vdd)
{
	unsigned char buf[11] = {0,};
	int year, month, day;
	int hour, min;
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

		LCD_INFO("manufacture_date DSI%d = (%d%04d) - year(%d) month(%d) day(%d) hour(%d) min(%d)\n",
			vdd->ndx, vdd->manufacture_date_dsi, vdd->manufacture_time_dsi,
			year, month, day, hour, min);

		/* While Coordinates */
		vdd->mdnie.mdnie_x = buf[0] << 8 | buf[1];	/* X */
		vdd->mdnie.mdnie_y = buf[2] << 8 | buf[3];	/* Y */

		mdnie_tune_index = mdnie_coordinate_index(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);
#if 0
		coordinate_tunning_multi(vdd, coordinate_data_multi, mdnie_tune_index,
				ADDRESS_SCR_WHITE_RED, COORDINATE_DATA_SIZE);
#endif

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
	}
	else {
		LCD_ERR("DSI%d no module_info_rx_cmds cmds(%d)", vdd->ndx, vdd->panel_revision);
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
	mdnie_data->DSI_RGB_SENSOR_MDNIE_1 = DSI0_RGB_SENSOR_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_2 = DSI0_RGB_SENSOR_MDNIE_2;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_3 = DSI0_RGB_SENSOR_MDNIE_3;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE_2 = DSI0_UI_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_UI_AUTO_MDNIE_2 = DSI0_UI_AUTO_MDNIE_2;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE_2 = DSI0_GALLERY_AUTO_MDNIE_2;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE_2 = DSI0_EBOOK_AUTO_MDNIE_2;

	mdnie_data->DSI_BYPASS_MDNIE = DSI0_BYPASS_MDNIE;
	mdnie_data->DSI_COLOR_BLIND_MDNIE = DSI0_COLOR_BLIND_MDNIE;
	mdnie_data->DSI_HBM_CE_MDNIE = DSI0_HBM_CE_MDNIE;
	mdnie_data->DSI_HBM_CE_D65_MDNIE = DSI0_HBM_CE_D65_MDNIE;
	mdnie_data->DSI_RGB_SENSOR_MDNIE = DSI0_RGB_SENSOR_MDNIE;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE = DSI0_UI_DYNAMIC_MDNIE;
	mdnie_data->DSI_UI_AUTO_MDNIE = DSI0_UI_AUTO_MDNIE;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE = DSI0_GALLERY_AUTO_MDNIE;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE = DSI0_EBOOK_AUTO_MDNIE;
	mdnie_data->DSI_CURTAIN = DSI0_SCREEN_CURTAIN_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR = DSI0_NIGHT_MODE_MDNIE_2;
	mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_2;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR = DSI0_RGB_SENSOR_MDNIE_2;

	mdnie_data->mdnie_tune_value_dsi = mdnie_tune_value_dsi0;
	mdnie_data->light_notification_tune_value_dsi = light_notification_tune_value_dsi0;
	mdnie_data->hdr_tune_value_dsi = hdr_tune_value_dsi0;
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
	mdnie_data->dsi_rgb_sensor_mdnie_1_size = DSI0_RGB_SENSOR_MDNIE_1_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_2_size = DSI0_RGB_SENSOR_MDNIE_2_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_3_size = DSI0_RGB_SENSOR_MDNIE_3_SIZE;

	mdnie_data->dsi_adjust_ldu_table = adjust_ldu_data;
	mdnie_data->dsi_max_adjust_ldu = 6;
	mdnie_data->dsi_night_mode_table = night_mode_data;
	mdnie_data->dsi_max_night_mode_index = 102;
	mdnie_data->dsi_white_default_r = 0xff;
	mdnie_data->dsi_white_default_g = 0xff;
	mdnie_data->dsi_white_default_b = 0xff;
	mdnie_data->dsi_white_balanced_r = 0;
	mdnie_data->dsi_white_balanced_g = 0;
	mdnie_data->dsi_white_balanced_b = 0;
	mdnie_data->dsi_scr_step_index = MDNIE_STEP2_INDEX;

	vdd->mdnie.mdnie_data = mdnie_data;

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

static int ss_octa_id_read(struct samsung_display_driver_data *vdd)
{
	char cell_id_buffer[MAX_CELL_ID] = {0,};
	int loop;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read Panel Unique OCTA ID (A1 12th ~ 15th(4bytes) + Cell ID(16bytes)) */
	if (ss_get_cmds(vdd, RX_OCTA_ID)->count) {
		memset(vdd->octa_id_dsi, 0x00, MAX_OCTA_ID);

		/* Read A1 12th ~ 15th(4bytes) */
		ss_panel_data_read(vdd, RX_OCTA_ID,
				vdd->octa_id_dsi, LEVEL1_KEY);

		if (ss_get_cmds(vdd, RX_CELL_ID)->count) {
			memset(cell_id_buffer, 0x00, MAX_CELL_ID);
			/* Read 92h 3rd ~ 18th */
			ss_panel_data_read(vdd, RX_CELL_ID, cell_id_buffer, LEVEL1_KEY);
		}

		/* Copy from cell_id(16bytes) */
		for (loop = 0; loop < MAX_CELL_ID; loop++)
			vdd->octa_id_dsi[loop + 4] = cell_id_buffer[loop];
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

	pcmds->cmds[2].msg.tx_buf[1] = 0x01;	/* ACL 8% in HBM */

	LCD_INFO("HBM: gradual_acl: %d, acl per: 0x%x",
			vdd->br_info.gradual_acl_val, pcmds->cmds[2].msg.tx_buf[1]);

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

	pcmds->cmds[2].msg.tx_buf[1] = 0x03;	/* ACL 15% in normal brightness */

	LCD_INFO("gradual_acl: %d, acl per: 0x%x",
			vdd->br_info.gradual_acl_val, pcmds->cmds[2].msg.tx_buf[1]);

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


enum LPMON_CMD_ID {
	LPM_BL_CMDID_CTRL = 1,
	LPM_ON_CMDID_BL = 2,
};

static void ss_set_panel_lpm_brightness(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *set = ss_get_cmds(vdd, TX_LPM_BL_CMD);
	struct dsi_panel_cmd_set *set_lpm_bl;

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

	memcpy(&set->cmds[LPM_BL_CMDID_CTRL].msg.tx_buf[1],
			&set_lpm_bl->cmds->msg.tx_buf[1],
			sizeof(char) * (set->cmds[LPM_BL_CMDID_CTRL].msg.tx_len - 1));

	/* send lpm bl cmd */
	ss_send_cmd(vdd, TX_LPM_BL_CMD);

	LCD_INFO("[Panel LPM] bl_level : %s\n",
			/* Check current brightness level */
			vdd->panel_lpm.lpm_bl_level == LPM_2NIT ? "2NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_10NIT ? "10NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_30NIT ? "30NIT" :
			vdd->panel_lpm.lpm_bl_level == LPM_60NIT ? "60NIT" : "UNKNOWN");
}

static void ss_update_panel_lpm_ctrl_cmd(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *set_lpm_on = ss_get_cmds(vdd, TX_LPM_ON);
	struct dsi_panel_cmd_set *set_lpm_off = ss_get_cmds(vdd, TX_LPM_OFF);
	struct dsi_panel_cmd_set *set_lpm_bl;

	LCD_INFO("%s++\n", __func__);

	if (SS_IS_CMDS_NULL(set_lpm_on) || SS_IS_CMDS_NULL(set_lpm_off)) {
		LCD_ERR("No cmds for TX_LPM_ON/OFF\n");
		goto start_lpm_bl;
	}

start_lpm_bl:
	/* LPM_ON: 3. HLPM brightness */
	/* should restore normal brightness in LPM off sequence to prevent flicker.. */
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

	memcpy(&set_lpm_on->cmds[LPM_ON_CMDID_BL].msg.tx_buf[1],
			&set_lpm_bl->cmds->msg.tx_buf[1],
			sizeof(char) * set_lpm_on->cmds[LPM_ON_CMDID_BL].msg.tx_len - 1);

	LCD_INFO("%s--\n", __func__);
}


static int ss_brightness_tot_level(struct samsung_display_driver_data *vdd)
{
	int tot_level_key = 0;

	tot_level_key = LEVEL1_KEY;

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

#if 1
static int ss_dyn_mipi_pre(struct samsung_display_driver_data *vdd)
{
	int rc = 0;

	ss_send_cmd(vdd, TX_FFC_OFF);
	LCD_INFO("[DISPLAY_%d] tx FFC OFF\n", vdd->ndx);

	return rc;
}

#define FFC_CMD_LEN (4)
#define FFC_START_IDX (2)
static int ss_dyn_mipi_post(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *ffc_set;
	struct dsi_panel_cmd_set *dyn_ffc_set;

	int idx;
	int ffc_idx, dyn_ffc_idx;
	int loop;

	mutex_lock(&vdd->dyn_mipi_clk.dyn_mipi_lock);
	idx = ss_find_dyn_mipi_clk_timing_idx(vdd);
	mutex_unlock(&vdd->dyn_mipi_clk.dyn_mipi_lock);

	if (idx < 0) {
		LCD_ERR("[S6E3FC3_AMS646YD01] Failed to find MIPI clock timing (%d)\n", idx);
		return 0;
	}

	LCD_INFO("[DISPLAY_%d] +++ clk idx: %d, tx FFC\n", vdd->ndx, idx);

	ffc_set = ss_get_cmds(vdd, TX_FFC);
	dyn_ffc_set = ss_get_cmds(vdd, TX_DYNAMIC_FFC_SET);

	if (SS_IS_CMDS_NULL(ffc_set) || SS_IS_CMDS_NULL(dyn_ffc_set)) {
		LCD_ERR("No cmds for TX_FFC..\n");
		return -EINVAL;
	}

	for (loop = 0; loop < FFC_CMD_LEN; loop++) {
		ffc_idx = FFC_START_IDX + loop;
		dyn_ffc_idx = (FFC_CMD_LEN * idx) + loop;

		if (ffc_set->cmds[ffc_idx].msg.tx_len ==
			dyn_ffc_set->cmds[dyn_ffc_idx].msg.tx_len) {
			memcpy(ffc_set->cmds[ffc_idx].msg.tx_buf,
					dyn_ffc_set->cmds[dyn_ffc_idx].msg.tx_buf,
					ffc_set->cmds[ffc_idx].msg.tx_len);
		} else {
			LCD_INFO("[DISPLAY_%d] ffc cmd(%d) doesn't match\n", vdd->ndx, loop);
		}
	}

	ss_send_cmd(vdd, TX_FFC);

	LCD_INFO("[DISPLAY_%d] --- clk idx: %d, tx FFC\n", vdd->ndx, idx);

	return 0;
}
#else
#define FFC_CMD_LEN (4)
#define FFC_START_IDX (2)
static int ss_ffc(struct samsung_display_driver_data *vdd, int idx)
{
	struct dsi_panel_cmd_set *ffc_set;
	struct dsi_panel_cmd_set *dyn_ffc_set;

	int ffc_idx, dyn_ffc_idx;
	int loop;

	LCD_INFO("[DISPLAY_%d] +++ clk idx: %d, tx FFC\n", vdd->ndx, idx);

	ffc_set = ss_get_cmds(vdd, TX_FFC);
	dyn_ffc_set = ss_get_cmds(vdd, TX_DYNAMIC_FFC_SET);

	if (SS_IS_CMDS_NULL(ffc_set) || SS_IS_CMDS_NULL(dyn_ffc_set)) {
		LCD_ERR("No cmds for TX_FFC..\n");
		return -EINVAL;
	}

	for (loop = 0; loop < FFC_CMD_LEN; loop++) {
		ffc_idx = FFC_START_IDX + loop;
		dyn_ffc_idx = (FFC_CMD_LEN * idx) + loop;

		if (ffc_set->cmds[ffc_idx].msg.tx_len ==
			dyn_ffc_set->cmds[dyn_ffc_idx].msg.tx_len) {
			memcpy(ffc_set->cmds[ffc_idx].msg.tx_buf,
					dyn_ffc_set->cmds[dyn_ffc_idx].msg.tx_buf,
					ffc_set->cmds[ffc_idx].msg.tx_len);
		} else {
			LCD_INFO("[DISPLAY_%d] ffc cmd(%d) doesn't match\n", vdd->ndx, loop);
		}
	}

	ss_send_cmd(vdd, TX_FFC);

	LCD_INFO("[DISPLAY_%d] --- clk idx: %d, tx FFC\n", vdd->ndx, idx);

	return 0;
}
#endif

#if 0
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
	make_self_dispaly_img_cmds_FC3(vdd, TX_SELF_MASK_IMAGE, FLAG_SELF_MASK);

	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_buf = self_mask_img_fhd_crc_data;
	vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_size = ARRAY_SIZE(self_mask_img_fhd_crc_data);
	make_self_dispaly_img_cmds_FC3(vdd, TX_SELF_MASK_IMAGE_CRC, FLAG_SELF_MASK_CRC);

	LCD_INFO("--\n");
	return 1;
}
#endif

static int ss_vrr_init(struct vrr_info *vrr)
{
	LCD_INFO("S6E3FC3_AMS646YD01 ++\n");

	mutex_init(&vrr->vrr_lock);
	mutex_init(&vrr->brr_lock);

	/* Bootloader: FHD@120hz HSl mode */
	vrr->cur_refresh_rate = vrr->adjusted_refresh_rate = 120;
	vrr->cur_sot_hs_mode = vrr->adjusted_sot_hs_mode = true;
	vrr->max_h_active_support_120hs = 1080; /* supports 120hz until FHD 1080 */

	vrr->vrr_workqueue = create_singlethread_workqueue("vrr_workqueue");
	INIT_WORK(&vrr->vrr_work, ss_panel_vrr_switch_work);

	LCD_INFO("S6E3FC3_AMS646YD01 --\n");

	return 0;
}

#if 0
static void make_brightness_packet(struct samsung_display_driver_data *vdd,
	struct dsi_cmd_desc *packet, int *cmd_cnt, enum BR_TYPE br_type)
{
	if (br_type == BR_TYPE_NORMAL) {
		if (vdd->br_info.smart_dimming_loaded_dsi) { /* OCTA PANEL */
			/* hbm off */
			if (vdd->br_info.is_hbm)
				ss_add_brightness_packet(vdd, BR_FUNC_HBM_OFF, packet, cmd_cnt);

			/* aid/aor */
			ss_add_brightness_packet(vdd, BR_FUNC_AID, packet, cmd_cnt);

			/* acl */
			if (vdd->br_info.acl_status || vdd->siop_status) {
				ss_add_brightness_packet(vdd, BR_FUNC_ACL_ON, packet, cmd_cnt);
				ss_add_brightness_packet(vdd, BR_FUNC_ACL_PERCENT_PRE, packet, cmd_cnt);
				ss_add_brightness_packet(vdd, BR_FUNC_ACL_PERCENT, packet, cmd_cnt);
			} else {
				ss_add_brightness_packet(vdd, BR_FUNC_ACL_OFF, packet, cmd_cnt);
			}

			/* elvss */
			ss_add_brightness_packet(vdd, BR_FUNC_ELVSS_PRE, packet, cmd_cnt);
			ss_add_brightness_packet(vdd, BR_FUNC_ELVSS, packet, cmd_cnt);

			/* temperature elvss */
			ss_add_brightness_packet(vdd, BR_FUNC_ELVSS_TEMP1, packet, cmd_cnt);
			ss_add_brightness_packet(vdd, BR_FUNC_ELVSS_TEMP2, packet, cmd_cnt);

			/* caps*/
			ss_add_brightness_packet(vdd, BR_FUNC_CAPS_PRE, packet, cmd_cnt);
			ss_add_brightness_packet(vdd, BR_FUNC_CAPS, packet, cmd_cnt);

			/* Manual DBV (for DIA setting) */
			ss_add_brightness_packet(vdd, BR_FUNC_DBV, packet, cmd_cnt);

			/* vint */
			ss_add_brightness_packet(vdd, BR_FUNC_VINT, packet, cmd_cnt);

			/* IRC */
			ss_add_brightness_packet(vdd, BR_FUNC_IRC, packet, cmd_cnt);

			/* LTPS: used for normal and HBM brightness */
			ss_add_brightness_packet(vdd, BR_FUNC_LTPS, packet, cmd_cnt);

			/* PANEL SPECIFIC SETTINGS */
			ss_add_brightness_packet(vdd, BR_FUNC_ETC, packet, cmd_cnt);

			/* mAFPC */
			if (vdd->mafpc.is_support)
				ss_add_brightness_packet(vdd, BR_FUNC_MAFPC_SCALE, packet, cmd_cnt);

			/* gamma */
			ss_add_brightness_packet(vdd, BR_FUNC_GAMMA, packet, cmd_cnt);

			/* VRR */
			ss_add_brightness_packet(vdd, BR_FUNC_VRR, packet, cmd_cnt);
		} else { /* Gamma Mode2 or TFT PANEL */

			ss_add_brightness_packet(vdd, BR_FUNC_1, packet, cmd_cnt);

			/* ACL */
			if (vdd->br_info.acl_status || vdd->siop_status) {
				ss_add_brightness_packet(vdd, BR_FUNC_ACL_ON, packet, cmd_cnt);
				ss_add_brightness_packet(vdd, BR_FUNC_ACL_PERCENT_PRE, packet, cmd_cnt);
				ss_add_brightness_packet(vdd, BR_FUNC_ACL_PERCENT, packet, cmd_cnt);
			} else {
				ss_add_brightness_packet(vdd, BR_FUNC_ACL_OFF, packet, cmd_cnt);
			}

			/* TFM_PWM */
			ss_add_brightness_packet(vdd, BR_FUNC_TFT_PWM, packet, cmd_cnt);

			/* mAFPC */
			if (vdd->mafpc.is_support)
				ss_add_brightness_packet(vdd, BR_FUNC_MAFPC_SCALE, packet, cmd_cnt);

			/* gamma */
			ss_add_brightness_packet(vdd, BR_FUNC_GAMMA, packet, cmd_cnt);

			/* gamma compensation for gamma mode2 VRR modes */
			ss_add_brightness_packet(vdd, BR_FUNC_GAMMA_COMP, packet, cmd_cnt);

			/* vint */
			ss_add_brightness_packet(vdd, BR_FUNC_VINT, packet, cmd_cnt);

			/* VRR */
			ss_add_brightness_packet(vdd, BR_FUNC_VRR, packet, cmd_cnt);

		}
	} else if (br_type == BR_TYPE_HBM) {
		/* acl */
		if (vdd->br_info.acl_status || vdd->siop_status) {
			ss_add_brightness_packet(vdd, BR_FUNC_HBM_ACL_ON, packet, cmd_cnt);
		} else {
			ss_add_brightness_packet(vdd, BR_FUNC_HBM_ACL_OFF, packet, cmd_cnt);
		}

		/* IRC */
		ss_add_brightness_packet(vdd, BR_FUNC_HBM_IRC, packet, cmd_cnt);

		/* Gamma */
		ss_add_brightness_packet(vdd, BR_FUNC_HBM_GAMMA, packet, cmd_cnt);

		/* vint */
		ss_add_brightness_packet(vdd, BR_FUNC_HBM_VINT, packet, cmd_cnt);

		/* LTPS: used for normal and HBM brightness */
		ss_add_brightness_packet(vdd, BR_FUNC_HBM_LTPS, packet, cmd_cnt);

		/* mAFPC */
		if (vdd->mafpc.is_support)
			ss_add_brightness_packet(vdd, BR_FUNC_MAFPC_SCALE, packet, cmd_cnt);

		/* hbm etc */
		ss_add_brightness_packet(vdd, BR_FUNC_HBM_ETC, packet, cmd_cnt);

		/* VRR */
		ss_add_brightness_packet(vdd, BR_FUNC_HBM_VRR, packet, cmd_cnt);
	} else if (br_type == BR_TYPE_HMT) {
		if (vdd->br_info.smart_dimming_hmt_loaded_dsi) {
			/* aid/aor B2 */
			ss_add_brightness_packet(vdd, BR_FUNC_HMT_AID, packet, cmd_cnt);

			/* elvss B5 */
			ss_add_brightness_packet(vdd, BR_FUNC_HMT_ELVSS, packet, cmd_cnt);

			/* vint F4 */
			ss_add_brightness_packet(vdd, BR_FUNC_HMT_VINT, packet, cmd_cnt);

			/* gamma CA */
			ss_add_brightness_packet(vdd, BR_FUNC_HMT_GAMMA, packet, cmd_cnt);

			/* VRR */
			ss_add_brightness_packet(vdd, BR_FUNC_HMT_VRR, packet, cmd_cnt);
		} else {
			ss_add_brightness_packet(vdd, BR_FUNC_HMT_GAMMA, packet, cmd_cnt);
		}
	} else {
		LCD_ERR("undefined br_type (%d) \n", br_type);
	}

	return;
}
#endif

static void samsung_panel_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("S6E3FC3_AMS646YD01 : ++\n");
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
	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;
	vdd->panel_func.samsung_elvss_read = ss_elvss_read;

	/* Make brightness packet */
	//vdd->panel_func.make_brightness_packet = make_brightness_packet;

	/* Brightness */
	vdd->panel_func.samsung_brightness_gamma = ss_brightness_gamma_mode2_normal;
	vdd->panel_func.samsung_brightness_acl_on = ss_acl_on;
	vdd->panel_func.samsung_brightness_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_vrr = ss_vrr;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_brightness_gamma_mode2_hbm;
	vdd->panel_func.samsung_hbm_acl_on = ss_acl_on_hbm;
	vdd->panel_func.samsung_hbm_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_vrr_hbm = ss_vrr_hbm;

	/* VRR */
	ss_vrr_init(&vdd->vrr);

	/* Panel LPM */
	vdd->panel_func.samsung_update_lpm_ctrl_cmd = ss_update_panel_lpm_ctrl_cmd; // JUN_TEMP_R8
	vdd->panel_func.samsung_set_lpm_brightness = ss_set_panel_lpm_brightness;

	/* Total level key in brightness */
	vdd->panel_func.samsung_brightness_tot_level = ss_brightness_tot_level;

	/* default brightness */
	vdd->br_info.common_br.bl_level = 255;

	/* ACL default ON */
	vdd->br_info.acl_status = 1;
	vdd->br_info.temperature = 20; // default temperature

	/* ACL default status in acl on */
	vdd->br_info.gradual_acl_val = 1;

	/* Self display */
	//vdd->self_disp.is_support = true;
	//vdd->self_disp.factory_support = true;
	//vdd->self_disp.init = self_display_init_FC3;
	//vdd->self_disp.data_init = ss_self_display_data_init;

	/* FFC */ // JUN_TEMP_R8
	// vdd->panel_func.set_ffc = ss_ffc;
	/* Dynamic(Adaptive) MIPI Clock */
	vdd->panel_func.samsung_dyn_mipi_pre = ss_dyn_mipi_pre;
	vdd->panel_func.samsung_dyn_mipi_post = ss_dyn_mipi_post;

	/* SAMSUNG_FINGERPRINT */
	vdd->panel_hbm_entry_delay = 0;

	/* te high -> 52us (120fps) -> te low -> tx start */
	vdd->panel_hbm_entry_after_te = 60; //52us is TE high time
	vdd->panel_hbm_exit_delay = 0;

	/* mdnie */
	vdd->mdnie.support_mdnie = true;
	vdd->mdnie.support_trans_dimming = false;
	vdd->mdnie.mdnie_tune_size[0] = sizeof(DSI0_BYPASS_MDNIE_1);
	vdd->mdnie.mdnie_tune_size[1] = sizeof(DSI0_BYPASS_MDNIE_2);
	vdd->mdnie.mdnie_tune_size[2] = sizeof(DSI0_BYPASS_MDNIE_3);
	vdd->mdnie.mdnie_tune_size[3] = sizeof(DSI0_BYPASS_MDNIE_4);

	dsi_update_mdnie_data(vdd);
	LCD_INFO("S6E3FC3_AMS646YD01 : --\n");
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_S6E3FC3_AMS646YD01_FHD";
	char panel_name[MAX_CMDLINE_PARAM_LEN];
	char panel_secondary_name[MAX_CMDLINE_PARAM_LEN];

	LCD_INFO("S6E3FC3_AMS646YD01 : ++\n");

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
		LCD_ERR("can not find panel_name (%s) / (%s)\n", panel_string, panel_name);
		LCD_INFO("S6E3FC3_AMS646YD01 : --\n");
		return 0;
	}

	vdd = ss_get_vdd(ndx);
	vdd->panel_func.samsung_panel_init = samsung_panel_init;

	LCD_INFO("%s done..\n", panel_name);
	LCD_INFO("S6E3FC3_AMS646YD01 : --\n");

	return 0;
}

early_initcall(samsung_panel_initialize);
