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
#include "ss_dsi_panel_S6TUUM2_AMSA24VU01.h"
#include "ss_dsi_mdnie_S6TUUM2_AMSA24VU01.h"

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
int prev_bl_s6tuum2; /* To save previous brightness level */

static struct dsi_panel_cmd_set *__ss_vrr(struct samsung_display_driver_data *vdd,
					int *level_key, bool is_hbm, bool is_hmt)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_panel_cmd_set  *vrr_cmds = ss_get_cmds(vdd, DSI_CMD_SET_TIMING_SWITCH);

	struct vrr_info *vrr = &vdd->vrr;
	enum SS_BRR_MODE brr_mode = vrr->brr_mode;

	int cur_rr;
	bool cur_hs;
	//char vbias_cmd[14];
	//char vaint_cmd[14];

	if (SS_IS_CMDS_NULL(vrr_cmds)) {
		LCD_INFO("no vrr cmds\n");
		return NULL;
	}
#if 0
	// TabS7+ only diffs refresh rate now.
	if (vrr->cur_refresh_rate == vdd->vrr.adjusted_refresh_rate) {
		LCD_INFO("same RR (%dhz%s / %dhz%s) just return.\n",
				panel->cur_mode->timing.refresh_rate,
				panel->cur_mode->timing.sot_hs_mode ? "HS" : "NM",
				vdd->vrr.adjusted_refresh_rate,
				vdd->vrr.adjusted_sot_hs_mode ? "HS" : "NM");
		return NULL;
	}
#endif
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

	//vrr_cmds = ss_get_cmds(vdd, DSI_CMD_SET_TIMING_SWITCH);
#if 0
	ss_get_cmd_vbais_from_flash(vdd, vbias_cmd, vaint_cmd, 7);

	memcpy(&vrr_cmds->cmds[VRR_CMDID_VBAIS].msg.tx_buf[1], vbias_cmd, 7);
	memcpy(&vrr_cmds->cmds[VRR_CMDID_VAINT].msg.tx_buf[1], vaint_cmd, 7);
	if (cur_hs == 1) {
		vrr_cmds->cmds[VRR_OSC_SETTING].msg.tx_buf[1] = 0x41;
		vrr_cmds->cmds[VRR_OSC_SETTING].msg.tx_buf[4] = 0x46;
		vrr_cmds->cmds[VRR_FREQ_SETTING].msg.tx_buf[1] = 0x08;
		switch (cur_rr) {
			case 120 :
				vrr_cmds->cmds[VRR_FREQ_SETTING].msg.tx_buf[2]= HS_120HZ;
				break;
			case 60 :
				vrr_cmds->cmds[VRR_FREQ_SETTING].msg.tx_buf[2]= HS_60HZ;
				break;
			case 30 :
				vrr_cmds->cmds[VRR_FREQ_SETTING].msg.tx_buf[2]= HS_30HZ;
				break;
			case 24 :
				vrr_cmds->cmds[VRR_FREQ_SETTING].msg.tx_buf[2]= HS_24HZ;
				break;
			case 10 :
				vrr_cmds->cmds[VRR_FREQ_SETTING].msg.tx_buf[2]= HS_10HZ;
				break;
		}
		vrr_cmds->cmds[VRR_CMDOFFSET_VAINT].msg.tx_buf[4] = 0x47;
	} else {
		vrr_cmds->cmds[VRR_OSC_SETTING].msg.tx_buf[1] = 0x43;
		vrr_cmds->cmds[VRR_OSC_SETTING].msg.tx_buf[4] = 0x7E;
		vrr_cmds->cmds[VRR_FREQ_SETTING].msg.tx_buf[1] = 0x00;
		switch (cur_rr) {
			case 60 :
				vrr_cmds->cmds[VRR_FREQ_SETTING].msg.tx_buf[2]= NS_60HZ;
				break;
			case 30 :
				vrr_cmds->cmds[VRR_FREQ_SETTING].msg.tx_buf[2]= NS_30HZ;
				break;
		}
		vrr_cmds->cmds[VRR_CMDOFFSET_VAINT].msg.tx_buf[4] = 0x39;
	}
#endif
	LCD_INFO("VRR: %s, FPS: %dhz%s (cur: %d%s, target: %d%s)\n",
			ss_get_brr_mode_name(brr_mode),
			cur_rr,
			cur_hs ? "HS" : "NM",
			vrr->cur_refresh_rate,
			vrr->cur_sot_hs_mode ? "HS" : "NM",
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
		fw_id = ddi_fw_id[0] << 24 | ddi_fw_id[1] << 16 | ddi_fw_id[2] << 8 | ddi_fw_id[3];
		LCD_INFO("DSI%d fw_id: %02x %02x %02x %02x => %x (%dbyte)\n", vdd->ndx,
			ddi_fw_id[0], ddi_fw_id[1],	ddi_fw_id[2], ddi_fw_id[3], fw_id, sizeof(fw_id));
	} else {
		LCD_ERR("DSI%d no ddi_fw_id_rx_cmds cmds", vdd->ndx);
		return false;
	}
	return fw_id;
}

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	LCD_INFO("+: ndx=%d\n", vdd->ndx);
	ss_panel_attach_set(vdd, true);

	if (vdd->manufacture_id_dsi == 0x801413) {
		/* compare if 0x01010100 to distinguish if support Samsung IP*/
		if (ss_fw_id_read(vdd) == 0x01010100) {
			LCD_INFO("fw_id matches. Support mdnie, selfmask.\n");
			vdd->check_fw_id = 0x01010100;
		} else {
			vdd->check_fw_id = 0;
			vdd->mdnie.support_mdnie = false;
			vdd->self_disp.is_support = false;
			LCD_INFO("fw_id does not match. NO mdnie, selfmask.\n");
		}

		vdd->dtsi_data.samsung_delay_after_tcon_rdy = 100; /* For PV1, dtsi 50 for PV2*/
	} else if (vdd->manufacture_id_dsi < 0x801413) {
		/* Not support Samsung IP if lower revision*/
		vdd->mdnie.support_mdnie = false;
		vdd->self_disp.is_support = false;

		vdd->dtsi_data.samsung_delay_after_tcon_rdy = 100; /* For PV1, dtsi 50 for PV2*/
	} else if (vdd->manufacture_id_dsi > 0x801413) {/* Support Samsung IP if higher revision*/
		LCD_INFO("Support mdnie, selfmask.\n");
		//vdd->dtsi_data.samsung_delay_after_tcon_rdy = 50; /* For PV2 */
	}

	return true;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	/* TABS7+ exception case : wait until fdone 1. (only for PV1[801413] panel) */
	if (vdd->manufacture_id_dsi == 0x801413) {
		char fdone = 0;
		int ret, timeout = 7;

		LCD_INFO("delay for manufacture_id_dsi:%x\n",vdd->manufacture_id_dsi);
		do { /*  wait for DDI flash done */
			msleep(50);
			ret = ss_panel_data_read(vdd, RX_LDI_DEBUG1, &fdone, LEVEL_KEY_NONE);
			if (ret) {
				LCD_ERR("fail to read fdone(ret=%d)\n", ret);
			}
			LCD_INFO("wait time: %dms, fdone: %d\n", (8-timeout)*50, fdone);

		} while (timeout-- && !(fdone & 0x01));
	}

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

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_normal(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_NORMAL);

	LCD_INFO("Normal : bl_level:%d, prev_bl:%d, finger:%d\n", vdd->br_info.common_br.bl_level,
		prev_bl_s6tuum2, vdd->finger_mask_updated);
	//pcmds->cmds[0].msg.tx_buf[1] = vdd->finger_mask_updated? 0x20 : 0x28;	/* Normal Smooth transition : 0x28 */
	if (vdd->finger_mask_updated)
		pcmds->cmds[0].msg.tx_buf[1] = 0x20;
	else if (vdd->display_status_dsi.wait_disp_on)
		pcmds->cmds[0].msg.tx_buf[1] = 0x20;
	else if (unlikely(vdd->is_factory_mode))
		pcmds->cmds[0].msg.tx_buf[1] = 0x20;
	else if (prev_bl_s6tuum2 > 255) /* smooth off from HBM*/
		pcmds->cmds[0].msg.tx_buf[1] = 0x20;
	else
		pcmds->cmds[0].msg.tx_buf[1] = 0x28;
	
	pcmds->cmds[2].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8); /* DBV [7:0] */
	pcmds->cmds[2].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 3); /* DBV [10:8] */

	*level_key = LEVEL_KEY_NONE;
	prev_bl_s6tuum2 = vdd->br_info.common_br.bl_level;

	return pcmds;
}

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_hbm(struct samsung_display_driver_data *vdd, int *level_key)
{
    struct dsi_panel_cmd_set *pcmds;
	//struct dsi_panel *panel = GET_DSI_PANEL(vdd);

    if (IS_ERR_OR_NULL(vdd)) {
        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
        return NULL;
    }

	LCD_INFO("HBM : cd:%d  refresh_rate:%d bl_level:%d prev_bl:%d finger:%d\n",
		vdd->br_info.common_br.cd_level, vdd->vrr.adjusted_refresh_rate, vdd->br_info.common_br.bl_level,
		prev_bl_s6tuum2, vdd->finger_mask_updated);

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HBM);

	if (vdd->vrr.adjusted_refresh_rate == 60) { /* 60hz : from hbm_60hz_gamma_table in ~.h */
		//pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HBM_60HZ);
		/* 53 E0
		    B0 00 00
 		    B8 80 00
 		    B0 00 02
 		    B8 01 52 */
		pcmds->cmds[1].msg.tx_buf[2] = 0x00; /* gpara 2nd byte*/
		pcmds->cmds[2].msg.tx_buf[0] = 0xB8;
		pcmds->cmds[2].msg.tx_buf[1] = 0x80;
		pcmds->cmds[2].msg.tx_buf[2] = 0x00;

		pcmds->cmds[3].msg.tx_buf[2] = 0x02; /* gpara 2nd byte*/
		pcmds->cmds[4].msg.tx_buf[0] = 0xB8;
		pcmds->cmds[4].msg.tx_buf[1] = get_bit(hbm_60hz_gamma_table[vdd->br_info.common_br.bl_level], 8, 3); /* [10:8] */
		pcmds->cmds[4].msg.tx_buf[2] = get_bit(hbm_60hz_gamma_table[vdd->br_info.common_br.bl_level], 0, 8); /* [7:0] */
	} else { /* 120hz : from dtsi hbm gamma table*/
		/* 02 53 E0
		    B0 00 79
		    C7 01 54
		    B0 00 79
	          C7 01 54 */
		pcmds->cmds[1].msg.tx_buf[2] = 0x79; /* gpara 2nd byte*/
		pcmds->cmds[2].msg.tx_buf[0] = 0xC7;
		pcmds->cmds[2].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 3); /* [10:8] */
		pcmds->cmds[2].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8); /* [7:0] */
		pcmds->cmds[3].msg.tx_buf[2] = 0x79; /* gpara 2nd byte*/
		pcmds->cmds[4].msg.tx_buf[0] = 0xC7;
		pcmds->cmds[4].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 3); /* [10:8] */
		pcmds->cmds[4].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8); /* [7:0] */
	}

	/* Fingerprint->HBM MAX? */

    *level_key = LEVEL_KEY_NONE;
	prev_bl_s6tuum2 = vdd->br_info.common_br.bl_level;

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
 	if(vdd->br_info.common_br.bl_level <= MAX_BL_PF_LEVEL) {
		LCD_ERR("Not a hbm level..:&d\n", vdd->br_info.common_br.bl_level);
		return NULL;
 	}

	pcmds->cmds[0].msg.tx_buf[1] = 0x01;	/* 55h ACL 8% */
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

	if(vdd->br_info.common_br.bl_level <= MAX_BL_PF_LEVEL) {
		pcmds->cmds[0].msg.tx_buf[1] = 0x02;	/* 55h ACL 15% */
	} else
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

#if 0
static void ss_copr_panel_init(struct samsung_display_driver_data *vdd)
{
	ss_copr_init(vdd);
}
#endif

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

	LCD_INFO("---\n");

	return 0;
}

static void samsung_panel_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("S6TUUM2_AMSA24VU01 : ++ \n");
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

	vdd->br_info.smart_dimming_loaded_dsi = false;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_brightness_gamma_mode2_hbm;
	vdd->panel_func.samsung_hbm_acl_on = ss_acl_on_hbm;
	vdd->panel_func.samsung_hbm_acl_off = ss_acl_off;

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
	vdd->panel_hbm_entry_delay = 0;
	vdd->panel_hbm_exit_delay = 1;
	vdd->panel_hbm_entry_after_te = 1000;
	/* VRR */
	ss_vrr_init(&vdd->vrr);

	vdd->debug_data->print_cmds = false;
	prev_bl_s6tuum2 = 0;

	LCD_INFO("S6TUUM2_AMSA24VU01 : -- \n");
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_S6TUUM2_AMSA24VU01_WQXGA";
	char panel_name[MAX_CMDLINE_PARAM_LEN];
	char panel_secondary_name[MAX_CMDLINE_PARAM_LEN];

	LCD_INFO("S6TUUM2_AMSA24VU01 : ++ \n");

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
	LCD_INFO("S6TUUM2_AMSA24VU01 : -- \n");

	return 0;
}

early_initcall(samsung_panel_initialize);
