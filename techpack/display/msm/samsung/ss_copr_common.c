/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * COPR :
 * Author: QC LCD driver <kr0124.cho@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ss_dsi_panel_common.h"
#include "ss_ddi_spi_common.h"
#include "ss_copr_common.h"

struct COPR_REG_OSSET copr_offset_list[] = {
	{.name = "copr_mask=", 		.offset = offsetof(struct COPR_CMD, COPR_MASK)},
	{.name = "copr_cnt_re=", 	.offset = offsetof(struct COPR_CMD, CNT_RE)},
	{.name = "copr_ilc=", 		.offset = offsetof(struct COPR_CMD, COPR_ILC)},
	{.name = "copr_gamma=", 	.offset = offsetof(struct COPR_CMD, COPR_GAMMA)},
	{.name = "copr_en=",		.offset = offsetof(struct COPR_CMD, COPR_EN)},
	{.name = "copr_er=", 		.offset = offsetof(struct COPR_CMD, COPR_ER)},
	{.name = "copr_eg=",		.offset = offsetof(struct COPR_CMD, COPR_EG)},
	{.name = "copr_eb=", 		.offset = offsetof(struct COPR_CMD, COPR_EB)},
	{.name = "copr_erc=", 		.offset = offsetof(struct COPR_CMD, COPR_ERC)},
	{.name = "copr_egc=", 		.offset = offsetof(struct COPR_CMD, COPR_EGC)},
	{.name = "copr_ebc=", 		.offset = offsetof(struct COPR_CMD, COPR_EBC)},
	{.name = "copr_max_cnt=",	.offset = offsetof(struct COPR_CMD, MAX_CNT)},
	{.name = "copr_roi_on=",	.offset = offsetof(struct COPR_CMD, ROI_ON)},

	{.name = "copr_roi_x_s=", 	.offset = offsetof(struct COPR_CMD, roi[0].ROI_X_S)},
	{.name = "copr_roi_y_s=", 	.offset = offsetof(struct COPR_CMD, roi[0].ROI_Y_S)},
	{.name = "copr_roi_x_e=",	.offset = offsetof(struct COPR_CMD, roi[0].ROI_X_E)},
	{.name = "copr_roi_y_e=",	.offset = offsetof(struct COPR_CMD, roi[0].ROI_Y_E)},

	{.name = "copr_roi1_x_s=", .offset = offsetof(struct COPR_CMD, roi[0].ROI_X_S)},
	{.name = "copr_roi1_y_s=", .offset = offsetof(struct COPR_CMD, roi[0].ROI_Y_S)},
	{.name = "copr_roi1_x_e=", .offset = offsetof(struct COPR_CMD, roi[0].ROI_X_E)},
	{.name = "copr_roi1_y_e=", .offset = offsetof(struct COPR_CMD, roi[0].ROI_Y_E)},

	{.name = "copr_roi2_x_s=", .offset = offsetof(struct COPR_CMD, roi[1].ROI_X_S)},
	{.name = "copr_roi2_y_s=", .offset = offsetof(struct COPR_CMD, roi[1].ROI_Y_S)},
	{.name = "copr_roi2_x_e=", .offset = offsetof(struct COPR_CMD, roi[1].ROI_X_E)},
	{.name = "copr_roi2_y_e=", .offset = offsetof(struct COPR_CMD, roi[1].ROI_Y_E)},

	{.name = "copr_roi3_x_s=", .offset = offsetof(struct COPR_CMD, roi[2].ROI_X_S)},
	{.name = "copr_roi3_y_s=", .offset = offsetof(struct COPR_CMD, roi[2].ROI_Y_S)},
	{.name = "copr_roi3_x_e=", .offset = offsetof(struct COPR_CMD, roi[2].ROI_X_E)},
	{.name = "copr_roi3_y_e=", .offset = offsetof(struct COPR_CMD, roi[2].ROI_Y_E)},

	{.name = "copr_roi4_x_s=", .offset = offsetof(struct COPR_CMD, roi[3].ROI_X_S)},
	{.name = "copr_roi4_y_s=", .offset = offsetof(struct COPR_CMD, roi[3].ROI_Y_S)},
	{.name = "copr_roi4_x_e=", .offset = offsetof(struct COPR_CMD, roi[3].ROI_X_E)},
	{.name = "copr_roi4_y_e=", .offset = offsetof(struct COPR_CMD, roi[3].ROI_Y_E)},

	{.name = "copr_roi5_x_s=", .offset = offsetof(struct COPR_CMD, roi[4].ROI_X_S)},
	{.name = "copr_roi5_y_s=", .offset = offsetof(struct COPR_CMD, roi[4].ROI_Y_S)},
	{.name = "copr_roi5_x_e=", .offset = offsetof(struct COPR_CMD, roi[4].ROI_X_E)},
	{.name = "copr_roi5_y_e=", .offset = offsetof(struct COPR_CMD, roi[4].ROI_Y_E)},

	{.name = "copr_roi6_x_s=", .offset = offsetof(struct COPR_CMD, roi[5].ROI_X_S)},
	{.name = "copr_roi6_y_s=", .offset = offsetof(struct COPR_CMD, roi[5].ROI_Y_S)},
	{.name = "copr_roi6_x_e=", .offset = offsetof(struct COPR_CMD, roi[5].ROI_X_E)},
	{.name = "copr_roi6_y_e=", .offset = offsetof(struct COPR_CMD, roi[5].ROI_Y_E)},
};

void ss_copr_enable(struct samsung_display_driver_data *vdd, int enable)
{
	if (vdd->copr.copr_on == enable) {
		LCD_ERR("copr already %d..\n", vdd->copr.copr_on);
		return;
	}

	mutex_lock(&vdd->copr.copr_lock);

	if (enable) {
		/* enable COPR IP */
		ss_send_cmd(vdd, TX_COPR_ENABLE);
	} else {
		/* disable COPR IP */
		ss_send_cmd(vdd, TX_COPR_DISABLE);

		vdd->copr.copr_cd[COPR_CD_INDEX_0].cd_sum = 0;
		vdd->copr.copr_cd[COPR_CD_INDEX_0].total_t = 0;
	}

	vdd->copr.copr_on = enable;

	LCD_INFO("copr %s .. \n", vdd->copr.copr_on?"enabled..":"disabled..");

	mutex_unlock(&vdd->copr.copr_lock);

	return;
}

void print_copr_cmd(struct COPR_CMD cmd)
{
	int i;

	LCD_INFO("MASK(%d) RE(%d) ILC(%d) GAMMA(%d) EN(%d)\n",
		cmd.COPR_MASK, cmd.CNT_RE, cmd.COPR_ILC, cmd.COPR_GAMMA, cmd.COPR_EN);
	LCD_INFO("ER(%d) EG(%d) EB(%d) ERC(%d) EGC(%d) EBC(%d)\n",
		cmd.COPR_ER, cmd.COPR_EG, cmd.COPR_EB,
		cmd.COPR_ERC, cmd.COPR_EGC, cmd.COPR_EBC);
	LCD_INFO("MAX_CNT(%d) ROI_EN(%d) ROI_CTRL(%d)\n",
		cmd.MAX_CNT, cmd.ROI_ON, cmd.COPR_ROI_CTRL);

	for (i = 0; i < MAX_COPR_ROI_CNT; i++)
		LCD_INFO("ROI[%d] X_S(%d) Y_S(%d) X_E(%d) Y_E(%d)\n", i + 1,
			cmd.roi[i].ROI_X_S, cmd.roi[i].ROI_Y_S, cmd.roi[i].ROI_X_E, cmd.roi[i].ROI_Y_E);
}

/**
 * ss_copr_set_cnt_re - set Counter Reset Trigger Signal values to copr cmds
 */
void ss_copr_set_cnt_re(struct COPR_CMD *cmd, int reset)
{
	cmd->CNT_RE = !!reset;
}

/**
 * ss_copr_set_en - set copr power on/off control values to copr cmds
 */
void ss_copr_set_en(struct COPR_CMD *cmd, int enable)
{
	cmd->COPR_EN = !!enable;
}

/**
 * ss_copr_set_ilc - set Illuminance Compensated COPR values to copr cmds
 */
void ss_copr_set_ilc(struct COPR_CMD *cmd, int enable)
{
	cmd->COPR_ILC = !!enable;
}

/**
 * ss_copr_set_gamma - set Gamma Selection values to copr cmds
 */
void ss_copr_set_gamma(struct COPR_CMD *cmd, int val)
{
	cmd->COPR_GAMMA = !!val;
}

/**
 * ss_copr_set_max_cnt - set Max Count values to copr cmds
 */
void ss_copr_set_max_cnt(struct COPR_CMD *cmd, int cnt)
{
	if (cnt > MAX_COPR_CNT)
		cnt = MAX_COPR_CNT;

	cmd->MAX_CNT = cnt;
}

/**
 * ss_copr_set_e - set Efficiency values to copr cmds
 */
void ss_copr_set_e(struct COPR_CMD *cmd, int r, int g, int b)
{
	cmd->COPR_ER = r;
	cmd->COPR_EG = g;
	cmd->COPR_EB = b;
}

/**
 * ss_copr_set_ec - set Efficiency for Illuminance Compensation values to copr cmds
 */
void ss_copr_set_ec(struct COPR_CMD *cmd, int r, int g, int b)
{
	cmd->COPR_ERC = r;
	cmd->COPR_EGC = g;
	cmd->COPR_EBC = b;
}

/**
 * ss_copr_set_roi - set roi values in cmds.
 */
void ss_copr_set_roi(struct samsung_display_driver_data *vdd, struct COPR_CMD *cmd, int idx)
{
	int roi_idx;

	if (vdd->copr.ver < COPR_VER_3P0)
		roi_idx = 0;
	else
		roi_idx = idx;

	if (vdd->copr.afc_roi[idx].ROI_X_S == -1) {
		LCD_ERR("Do not set for idx[%d] - %d %d %d %d\n", idx,
			vdd->copr.afc_roi[idx].ROI_X_S,
			vdd->copr.afc_roi[idx].ROI_Y_S,
			vdd->copr.afc_roi[idx].ROI_X_E,
			vdd->copr.afc_roi[idx].ROI_Y_E);
		return;
	}

	cmd->roi[roi_idx].ROI_X_S = vdd->copr.afc_roi[idx].ROI_X_S;
	cmd->roi[roi_idx].ROI_Y_S = vdd->copr.afc_roi[idx].ROI_Y_S;
	cmd->roi[roi_idx].ROI_X_E = vdd->copr.afc_roi[idx].ROI_X_E;
	cmd->roi[roi_idx].ROI_Y_E = vdd->copr.afc_roi[idx].ROI_Y_E;

	if (vdd->copr.ver < COPR_VER_3P0) {
		if (vdd->copr.afc_roi[idx].ROI_X_S || vdd->copr.afc_roi[idx].ROI_Y_S ||
			vdd->copr.afc_roi[idx].ROI_X_E || vdd->copr.afc_roi[idx].ROI_Y_E)
			cmd->ROI_ON = 1;
		else
			cmd->ROI_ON = 0;
	}
}

int ss_copr_set_cmd_offset(struct COPR_CMD *cmd, char* p)
{
	int list_size = ARRAY_SIZE(copr_offset_list);
	const char *name;
	int i, val;
	int *offset;

	for (i = 0; i < list_size; i++) {
		name = copr_offset_list[i].name;
		if (!strncmp(name, p, strlen(name))) {
			sscanf(p + strlen(name), "%d", &val);
			offset = (int *)((void*)cmd + copr_offset_list[i].offset);
			*offset = val;
			return 0;
		}
	}

	return -1;
}

void ss_copr_set_cmd_1P0(struct COPR_CMD *cmd, u8 *cmd_pload)
{
	cmd_pload[1] = cmd->COPR_EN;
	cmd_pload[1] |= (cmd->COPR_GAMMA << 1);
	cmd_pload[1] |= (cmd->CNT_RE << 2);

	cmd_pload[2] = ((cmd->COPR_ER & 0x300) >> 4);
	cmd_pload[2] |= ((cmd->COPR_EG & 0x300) >> 6);
	cmd_pload[2] |= ((cmd->COPR_EB & 0x300) >> 8);

	cmd_pload[3] = (cmd->COPR_ER & 0xFF);
	cmd_pload[4] = (cmd->COPR_EG & 0xFF);
	cmd_pload[5] = (cmd->COPR_EB & 0xFF);

	cmd_pload[6] = ((cmd->MAX_CNT & 0xFF00) >> 8);
	cmd_pload[7] = (cmd->MAX_CNT & 0xFF);

	cmd_pload[8] = (cmd->ROI_ON << 3);

	cmd_pload[8] |= ((cmd->roi[0].ROI_X_S & 0x700) >> 8);
	cmd_pload[9] = (cmd->roi[0].ROI_X_S & 0xFF);

	cmd_pload[10] = ((cmd->roi[0].ROI_Y_S & 0xF00) >> 8);
	cmd_pload[11] = (cmd->roi[0].ROI_Y_S & 0xFF);

	cmd_pload[12] = ((cmd->roi[0].ROI_X_E & 0x700) >> 8);
	cmd_pload[13] = (cmd->roi[0].ROI_X_E & 0xFF);

	cmd_pload[14] = ((cmd->roi[0].ROI_Y_E & 0xF00) >> 8);
	cmd_pload[15] = (cmd->roi[0].ROI_Y_E & 0xFF);
}

void ss_copr_set_cmd_2P0(struct COPR_CMD *cmd, u8 *cmd_pload)
{
	cmd_pload[1] = cmd->COPR_EN;
	cmd_pload[1] |= (cmd->COPR_GAMMA << 1);
	cmd_pload[1] |= (cmd->COPR_ILC << 2);
	cmd_pload[1] |= (cmd->CNT_RE << 3);

	cmd_pload[2] = ((cmd->COPR_ER & 0x300) >> 4);
	cmd_pload[2] |= ((cmd->COPR_EG & 0x300) >> 6);
	cmd_pload[2] |= ((cmd->COPR_EB & 0x300) >> 8);

	cmd_pload[3] = ((cmd->COPR_ERC & 0x300) >> 4);
	cmd_pload[3] |= ((cmd->COPR_EGC & 0x300) >> 6);
	cmd_pload[3] |= ((cmd->COPR_EBC & 0x300) >> 8);

	cmd_pload[4] = (cmd->COPR_ER & 0xFF);
	cmd_pload[5] = (cmd->COPR_EG & 0xFF);
	cmd_pload[6] = (cmd->COPR_EB & 0xFF);

	cmd_pload[7] = (cmd->COPR_ERC & 0xFF);
	cmd_pload[8] = (cmd->COPR_EGC & 0xFF);
	cmd_pload[9] = (cmd->COPR_EBC & 0xFF);

	cmd_pload[10] = ((cmd->MAX_CNT & 0xFF00) >> 8);
	cmd_pload[11] = (cmd->MAX_CNT & 0xFF);

	cmd_pload[12] = (cmd->ROI_ON << 3);

	cmd_pload[12] |= ((cmd->roi[0].ROI_X_S & 0x700) >> 8);
	cmd_pload[13] = (cmd->roi[0].ROI_X_S & 0xFF);

	cmd_pload[14] = ((cmd->roi[0].ROI_Y_S & 0xF00) >> 8);
	cmd_pload[15] = (cmd->roi[0].ROI_Y_S & 0xFF);

	cmd_pload[16] = ((cmd->roi[0].ROI_X_E & 0x700) >> 8);
	cmd_pload[17] = (cmd->roi[0].ROI_X_E & 0xFF);

	cmd_pload[18] = ((cmd->roi[0].ROI_Y_E & 0xF00) >> 8);
	cmd_pload[19] = (cmd->roi[0].ROI_Y_E & 0xFF);

}

void ss_copr_set_cmd_3P0(struct COPR_CMD *cmd, u8 *cmd_pload)
{
	int i, roi_max, roi_start;

	cmd_pload[1] = cmd->COPR_EN;
	cmd_pload[1] |= (cmd->COPR_GAMMA << 1);
	cmd_pload[1] |= (cmd->COPR_ILC << 2);
	cmd_pload[1] |= (cmd->CNT_RE << 3);
	cmd_pload[1] |= (cmd->COPR_MASK << 4);

	cmd_pload[2] = ((cmd->COPR_ER & 0x300) >> 4);
	cmd_pload[2] |= ((cmd->COPR_EG & 0x300) >> 6);
	cmd_pload[2] |= ((cmd->COPR_EB & 0x300) >> 8);

	cmd_pload[3] = ((cmd->COPR_ERC & 0x300) >> 4);
	cmd_pload[3] |= ((cmd->COPR_EGC & 0x300) >> 6);
	cmd_pload[3] |= ((cmd->COPR_EBC & 0x300) >> 8);

	cmd_pload[4] = (cmd->COPR_ER & 0xFF);
	cmd_pload[5] = (cmd->COPR_EG & 0xFF);
	cmd_pload[6] = (cmd->COPR_EB & 0xFF);

	cmd_pload[7] = (cmd->COPR_ERC & 0xFF);
	cmd_pload[8] = (cmd->COPR_EGC & 0xFF);
	cmd_pload[9] = (cmd->COPR_EBC & 0xFF);

	cmd_pload[10] = ((cmd->MAX_CNT & 0xFF00) >> 8);
	cmd_pload[11] = (cmd->MAX_CNT & 0xFF);
	cmd_pload[12] = (cmd->COPR_ROI_CTRL & 0x3F);

	roi_max = 6;
	roi_start = 13;

	for (i = 0; i < roi_max; i++) {
		cmd_pload[roi_start++] = ((cmd->roi[i].ROI_X_S & 0xF00) >> 8);
		cmd_pload[roi_start++] = (cmd->roi[i].ROI_X_S & 0xFF);

		cmd_pload[roi_start++] = ((cmd->roi[i].ROI_Y_S & 0xF00) >> 8);
		cmd_pload[roi_start++] = (cmd->roi[i].ROI_Y_S & 0xFF);

		cmd_pload[roi_start++] = ((cmd->roi[i].ROI_X_E & 0xF00) >> 8);
		cmd_pload[roi_start++] = (cmd->roi[i].ROI_X_E & 0xFF);

		cmd_pload[roi_start++] = ((cmd->roi[i].ROI_Y_E & 0xF00) >> 8);
		cmd_pload[roi_start++] = (cmd->roi[i].ROI_Y_E & 0xFF);
	}
}

/**
 * ss_copr_set_cmd - set copr mipi cmd using COPR_CMD struct.
 *
 * copr_cmd : new copr cmd to update.
 */
void ss_copr_set_cmd(struct samsung_display_driver_data *vdd, struct COPR_CMD *cmd)
{
	struct dsi_panel_cmd_set *pcmds = NULL;
	int cmd_len;
	u8 *cmd_pload;
	char buf[256];
	int i, len = 0;

	pcmds = ss_get_cmds(vdd, TX_COPR_ENABLE);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_COPR_ENABLE..\n");
		return;
	}

	cmd_pload = pcmds->cmds[1].msg.tx_buf;
	cmd_len = pcmds->cmds[1].msg.tx_len;

	if (vdd->copr.ver == COPR_VER_3P0)
		ss_copr_set_cmd_3P0(cmd, cmd_pload);
	else if (vdd->copr.ver == COPR_VER_2P0)
		ss_copr_set_cmd_2P0(cmd, cmd_pload);
	else if (vdd->copr.ver == COPR_VER_1P0)
		ss_copr_set_cmd_1P0(cmd, cmd_pload);
	else {
		LCD_ERR("Do not use copr in display driver.\n");
		return;
	}

	for (i = 0; i < cmd_len; i++)
		len += snprintf(buf + len, sizeof(buf) - len,
						"%02x ", cmd_pload[i]);
	LCD_ERR("cmd[%d] : %s\n", cmd_len, buf);

	/* reset current copr cmds */
	memcpy(&vdd->copr.cur_cmd, cmd, sizeof(struct COPR_CMD));

	return;
}

void ss_get_copr_orig_cmd_1P0(struct COPR_CMD *cmd, u8 *cmd_pload)
{
	cmd->COPR_EN = (cmd_pload[1] & 0x01) ? 1 : 0;
	cmd->COPR_GAMMA = (cmd_pload[1] & 0x02) ? 1 : 0;
	cmd->CNT_RE = (cmd_pload[1] & 0x04) ? 1 : 0;

	cmd->COPR_ER = ((cmd_pload[2] & 0x30) << 4) | (cmd_pload[3] & 0xFF);
	cmd->COPR_EG = ((cmd_pload[2] & 0x0C) << 6) | (cmd_pload[4] & 0xFF);
	cmd->COPR_EB = ((cmd_pload[2] & 0x03) << 8) | (cmd_pload[5] & 0xFF);

	cmd->MAX_CNT = (cmd_pload[6] << 8) | cmd_pload[7];
	cmd->ROI_ON = (cmd_pload[8] & 0x08) ? 1 : 0;

	cmd->roi[0].ROI_X_S = ((cmd_pload[8] & 0x07) << 8);
	cmd->roi[0].ROI_X_S |= (cmd_pload[9] & 0xFF);
	cmd->roi[0].ROI_Y_S = ((cmd_pload[10] & 0x0F) << 8);
	cmd->roi[0].ROI_Y_S |= (cmd_pload[11] & 0xFF);
	cmd->roi[0].ROI_X_E = ((cmd_pload[12] & 0x07) << 8);
	cmd->roi[0].ROI_X_E |= (cmd_pload[13] & 0xFF);
	cmd->roi[0].ROI_Y_E = ((cmd_pload[14] & 0x0F) << 8);
	cmd->roi[0].ROI_Y_E |= (cmd_pload[15] & 0xFF);

}

void ss_get_copr_orig_cmd_2P0(struct COPR_CMD *cmd, u8 *cmd_pload)
{
	cmd->COPR_EN = (cmd_pload[1] & 0x01) ? 1 : 0;
	cmd->COPR_GAMMA = (cmd_pload[1] & 0x02) ? 1 : 0;
	cmd->COPR_ILC = (cmd_pload[1] & 0x04) ? 1 : 0;
	cmd->CNT_RE = (cmd_pload[1] & 0x08) ? 1 : 0;

	cmd->COPR_ER = ((cmd_pload[2] & 0x30) << 4) | (cmd_pload[4] & 0xFF);
	cmd->COPR_EG = ((cmd_pload[2] & 0x0C) << 6) | (cmd_pload[5] & 0xFF);
	cmd->COPR_EB = ((cmd_pload[2] & 0x03) << 8) | (cmd_pload[6] & 0xFF);
	cmd->COPR_ERC = ((cmd_pload[3] & 0x30) << 4) | (cmd_pload[7] & 0xFF);
	cmd->COPR_EGC = ((cmd_pload[3] & 0x0C) << 6) | (cmd_pload[8] & 0xFF);
	cmd->COPR_EBC = ((cmd_pload[3] & 0x03) << 8) | (cmd_pload[9] & 0xFF);

	cmd->MAX_CNT = (cmd_pload[10] << 8) | cmd_pload[11];
	cmd->ROI_ON = (cmd_pload[12] & 0x08) ? 1 : 0;

	cmd->roi[0].ROI_X_S = ((cmd_pload[12] & 0x07) << 8);
	cmd->roi[0].ROI_X_S |= (cmd_pload[13] & 0xFF);
	cmd->roi[0].ROI_Y_S = ((cmd_pload[14] & 0x0F) << 8);
	cmd->roi[0].ROI_Y_S |= (cmd_pload[15] & 0xFF);
	cmd->roi[0].ROI_X_E = ((cmd_pload[16] & 0x07) << 8);
	cmd->roi[0].ROI_X_E |= (cmd_pload[17] & 0xFF);
	cmd->roi[0].ROI_Y_E = ((cmd_pload[18] & 0x0F) << 8);
	cmd->roi[0].ROI_Y_E |= (cmd_pload[19] & 0xFF);
}

void ss_get_copr_orig_cmd_3P0(struct COPR_CMD *cmd, u8 *cmd_pload)
{
	int i, roi_max, roi_start;

	cmd->COPR_EN = (cmd_pload[1] & 0x01) ? 1 : 0;
	cmd->COPR_GAMMA = (cmd_pload[1] & 0x02) ? 1 : 0;
	cmd->COPR_ILC = (cmd_pload[1] & 0x04) ? 1 : 0;
	cmd->CNT_RE = (cmd_pload[1] & 0x08) ? 1 : 0;
	cmd->COPR_MASK = (cmd_pload[1] & 0x10) ? 1 : 0;

	cmd->COPR_ER = ((cmd_pload[2] & 0x30) << 4) | (cmd_pload[4] & 0xFF);
	cmd->COPR_EG = ((cmd_pload[2] & 0x0C) << 6) | (cmd_pload[5] & 0xFF);
	cmd->COPR_EB = ((cmd_pload[2] & 0x03) << 8) | (cmd_pload[6] & 0xFF);
	cmd->COPR_ERC = ((cmd_pload[3] & 0x30) << 4) | (cmd_pload[7] & 0xFF);
	cmd->COPR_EGC = ((cmd_pload[3] & 0x0C) << 6) | (cmd_pload[8] & 0xFF);
	cmd->COPR_EBC = ((cmd_pload[3] & 0x03) << 8) | (cmd_pload[9] & 0xFF);

	cmd->MAX_CNT = (cmd_pload[10] << 8) | cmd_pload[11];

	cmd->COPR_ROI_CTRL = (cmd_pload[12] & 0x3F);

	roi_max = 6;
	roi_start = 13;

	for (i = 0; i < roi_max; i++) {
		cmd->roi[i].ROI_X_S = ((cmd_pload[roi_start++] & 0x0F) << 8);
		cmd->roi[i].ROI_X_S |= (cmd_pload[roi_start++] & 0xFF);
		cmd->roi[i].ROI_Y_S = ((cmd_pload[roi_start++] & 0x0F) << 8);
		cmd->roi[i].ROI_Y_S |= (cmd_pload[roi_start++] & 0xFF);
		cmd->roi[i].ROI_X_E = ((cmd_pload[roi_start++] & 0x0F) << 8);
		cmd->roi[i].ROI_X_E |= (cmd_pload[roi_start++] & 0xFF);
		cmd->roi[i].ROI_Y_E = ((cmd_pload[roi_start++] & 0x0F) << 8);
		cmd->roi[i].ROI_Y_E |= (cmd_pload[roi_start++] & 0xFF);
	}
}

/**
 * ss_get_copr_orig_cmd - get copr original cmd from panel dtsi (initial copr enable cmd).
 */
int ss_get_copr_orig_cmd(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *pcmds = NULL;
	struct COPR_CMD *cmd = &vdd->copr.orig_cmd;
	u8 *cmd_pload;
	int cmd_len;
	char buf[256];
	int i, len = 0;

	pcmds = ss_get_cmds(vdd, TX_COPR_ENABLE);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_COPR_ENABLE..\n");
		return -EINVAL;
	}

	cmd_pload = pcmds->cmds[1].msg.tx_buf;
	cmd_len = pcmds->cmds[1].msg.tx_len;

	if (vdd->copr.ver == COPR_VER_3P0)
		ss_get_copr_orig_cmd_3P0(cmd, cmd_pload);
	else if (vdd->copr.ver == COPR_VER_2P0)
		ss_get_copr_orig_cmd_2P0(cmd, cmd_pload);
	else if (vdd->copr.ver == COPR_VER_1P0)
		ss_get_copr_orig_cmd_1P0(cmd, cmd_pload);
	else {
		LCD_ERR("Do not use copr in display driver.\n");
		return -EINVAL;
	}

	/* init current cmd with origianl cmd */
	memcpy(&vdd->copr.cur_cmd, cmd, sizeof(struct COPR_CMD));

	print_copr_cmd(vdd->copr.cur_cmd);

	for (i = 0; i < cmd_len; i++)
		len += snprintf(buf + len, sizeof(buf) - len,
						"%02x ", cmd_pload[i]);
	LCD_ERR("cmd[%d] : %s\n", cmd_len, buf);

	return 1;
}

/**
 * ss_copr_get_roi_opr_2P0 - get opr values of all roi's r/g/b for copr ver 2.0.
 */
int ss_copr_get_roi_opr_2P0(struct samsung_display_driver_data *vdd)
{
	int ret = 0;
	int i;
	struct COPR_CMD cmd_backup;
	struct COPR_CMD *cur_cmd;

	LCD_DEBUG("++ (%d)\n", vdd->copr.afc_roi_cnt);

	/* backup copr current cmd */
	memcpy(&cmd_backup, &vdd->copr.cur_cmd, sizeof(cmd_backup));
	cur_cmd = &vdd->copr.cur_cmd;

	ss_copr_set_en(cur_cmd, 1);
	ss_copr_set_cnt_re(cur_cmd, 0);
	ss_copr_set_max_cnt(cur_cmd, 1);
	ss_copr_set_gamma(cur_cmd, 1);

	/**
	 * roi[0] - top
	 * roi[1] - mid
	 * roi[2] - bottom
	 * roi[3] - all
	 */
	for (i = 0; i < vdd->copr.afc_roi_cnt; i++) {

		ss_copr_set_roi(vdd, cur_cmd, i);

		/* R / G --------------------------------------- */
		ss_copr_set_ilc(cur_cmd, 1);
		ss_copr_set_e(cur_cmd, 0x300, 0x0, 0x0);
		ss_copr_set_ec(cur_cmd, 0x0, 0x300, 0x0);

		ss_copr_set_cmd(vdd, cur_cmd);
		ss_send_cmd(vdd, TX_COPR_ENABLE);

		/* sleep 34ms (wait 2 frame : roi cmd write -> vsync -> image write -> opr read) */
		usleep_range(34000, 34000);

		ret = ss_copr_read(vdd);
		if (ret)
			goto err;

		vdd->copr.roi_opr[i].R_OPR = vdd->copr.current_copr;
		vdd->copr.roi_opr[i].G_OPR = vdd->copr.comp_copr;
		/* --------------------------------------------- */

		/* B ------------------------------------------- */
		ss_copr_set_ilc(cur_cmd, 0);
		ss_copr_set_e(cur_cmd, 0x0, 0x0, 0x300);
		ss_copr_set_ec(cur_cmd, 0x0, 0x0, 0x0);

		ss_copr_set_cmd(vdd, cur_cmd);
		ss_send_cmd(vdd, TX_COPR_ENABLE);

		/* sleep 34ms (wait 2 frame : roi cmd write -> vsync -> image write -> opr read) */
		usleep_range(34000, 34000);

		ret = ss_copr_read(vdd);
		if (ret)
			goto err;

		vdd->copr.roi_opr[i].B_OPR = vdd->copr.current_copr;
		/* --------------------------------------------- */

		LCD_INFO("R (%d) G (%d) B (%d)\n",
			vdd->copr.roi_opr[i].R_OPR,
			vdd->copr.roi_opr[i].G_OPR,
			vdd->copr.roi_opr[i].B_OPR);
	}

err:
	/* restore copr cmd */
	ss_copr_set_cmd(vdd, &cmd_backup);

	LCD_DEBUG("--\n");

	return ret;
}

/**
 * ss_copr_get_roi_opr_3P0 - get opr values of all roi's r/g/b for copr ver 3.0.
 */
int ss_copr_get_roi_opr_3P0(struct samsung_display_driver_data *vdd)
{
	int ret = 0;
	int i;
	struct COPR_CMD cmd_backup;
	struct COPR_CMD *cur_cmd;

	LCD_DEBUG("++ (%d)\n", vdd->copr.afc_roi_cnt);

	/* backup copr current cmd */
	memcpy(&cmd_backup, &vdd->copr.cur_cmd, sizeof(cmd_backup));
	cur_cmd = &vdd->copr.cur_cmd;

	/* set roi values as roi values from mDNIe service */
	for (i = 0; i < vdd->copr.afc_roi_cnt; i++)
		ss_copr_set_roi(vdd, cur_cmd, i);

	/* reset copr enable cmds with new roi values */
	ss_copr_set_cmd(vdd, cur_cmd);

	ret = ss_send_cmd(vdd, TX_COPR_ENABLE);
	if (ret)
		LCD_ERR("fail to TX_COPR_ENABLE .. \n");

	ret = ss_copr_read(vdd);
	if (ret)
		LCD_ERR("fail to read copr .. \n");

	/* restore copr cmd */
	ss_copr_set_cmd(vdd, &cmd_backup);

	LCD_DEBUG("--\n");

	return ret;
}

/**
 * ss_copr_get_roi_opr - get opr values of all roi's r/g/b for AFC.
 *
 * Get copr snapshot for each roi's r/g/b.
 * This function returns 0 if the average is valid.
 * If not this function returns -ERR.
 */
int ss_copr_get_roi_opr(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	if (vdd->copr.ver < COPR_VER_3P0)
		ret = ss_copr_get_roi_opr_2P0(vdd);
	else if (vdd->copr.ver == COPR_VER_3P0) /* copr ver 3.0 */
		ret = ss_copr_get_roi_opr_3P0(vdd);
	else {
		LCD_ERR("Do not use copr in display driver.\n");
		return -EINVAL;
	}

	return ret;
}

void ss_set_copr_sum(struct samsung_display_driver_data *vdd, enum COPR_CD_INDEX idx)
{
	s64 delta;

	mutex_lock(&vdd->copr.copr_val_lock);
	vdd->copr.copr_cd[idx].last_t = vdd->copr.copr_cd[idx].cur_t;
	vdd->copr.copr_cd[idx].cur_t = ktime_get();
	delta = ktime_ms_delta(vdd->copr.copr_cd[idx].cur_t, vdd->copr.copr_cd[idx].last_t);
	vdd->copr.copr_cd[idx].total_t += delta;
	if (vdd->br_info.common_br.gamma_mode2_support)
		vdd->copr.copr_cd[idx].cd_sum += (vdd->br_info.common_br.cd_level * delta);
	else
		vdd->copr.copr_cd[idx].cd_sum += (vdd->br_info.common_br.interpolation_cd * delta);
	mutex_unlock(&vdd->copr.copr_val_lock);

	LCD_DEBUG("[%d] cd (%d) delta (%lld) cd_sum (%lld) total_t (%lld)\n",
			idx,
			vdd->br_info.common_br.gamma_mode2_support ?
				vdd->br_info.common_br.interpolation_cd :
				vdd->br_info.common_br.cd_level,
			delta,
			vdd->copr.copr_cd[idx].cd_sum,
			vdd->copr.copr_cd[idx].total_t);
}

/**
 * ss_copr_reset_cnt - reset copr frame cnt using CNT_RE reg. only for rev 2.0
 */
void ss_copr_reset_cnt(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *pcmds;
	u8 *cmd_pload;

	pcmds = ss_get_cmds(vdd, TX_COPR_ENABLE);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_COPR_ENABLE..\n");
		return;
	}

	cmd_pload = pcmds->cmds[1].msg.tx_buf;

	/* CNT_RE = 1 */
	cmd_pload[1] = 0x9;
	ss_send_cmd(vdd, TX_COPR_ENABLE);

	/* sleep 20ms (wait te) */
	usleep_range(20000, 20000);

	/* CNT_RE = 0 */
	cmd_pload[1] = 0x3;
	ss_send_cmd(vdd, TX_COPR_ENABLE);

	return;
}

void ss_copr_parse_spi_data(struct samsung_display_driver_data *vdd, u8 *rxbuf)
{
	int i, roi_idx;

	if (vdd->copr.ver == COPR_VER_3P0) {
		/* parse copr data (3.0) */
		vdd->copr.copr_ready = rxbuf[0] & 0x01;
		vdd->copr.current_cnt = (rxbuf[1] << 8) | rxbuf[2];
		vdd->copr.current_copr = ((rxbuf[3] & 0x01) << 8) | rxbuf[4];
		vdd->copr.avg_copr = ((rxbuf[5] & 0x1) << 8) | rxbuf[6];
		vdd->copr.sliding_current_cnt = (rxbuf[7] << 8) | rxbuf[8];
		vdd->copr.sliding_avg_copr = ((rxbuf[9] & 0x1) << 8) | rxbuf[10];

		roi_idx = 11;
		for (i = 0; i < 4; i++) {
			vdd->copr.roi_opr[i].R_OPR = (rxbuf[roi_idx++] & 0x1) << 8;
			vdd->copr.roi_opr[i].R_OPR |= rxbuf[roi_idx++];
			vdd->copr.roi_opr[i].G_OPR = (rxbuf[roi_idx++] & 0x1) << 8;
			vdd->copr.roi_opr[i].G_OPR |= rxbuf[roi_idx++];
			vdd->copr.roi_opr[i].B_OPR = (rxbuf[roi_idx++] & 0x1) << 8;
			vdd->copr.roi_opr[i].B_OPR |= rxbuf[roi_idx++];
		}
	} else {
		/* parse copr data (2.0, 1.0) */
		vdd->copr.copr_ready = (rxbuf[0] & 0x80) >> 7;
		vdd->copr.current_cnt = ((rxbuf[0] & 0x7F) << 9) | (rxbuf[1] << 1) | (rxbuf[2] & 0x80 >> 7) ;
		vdd->copr.current_copr = ((rxbuf[2] & 0x7F) << 2) | ((rxbuf[3] & 0xC0) >> 6);
		vdd->copr.avg_copr = ((rxbuf[3] & 0x3F) << 3) | ((rxbuf[4] & 0xE0) >> 5);
		vdd->copr.sliding_current_cnt = ((rxbuf[4] & 0x1F) << 11) | (rxbuf[5] << 3) | ((rxbuf[6] & 0xE0) >> 5);
		vdd->copr.sliding_avg_copr = ((rxbuf[6] & 0x1F) << 4) | ((rxbuf[7] & 0xF0) >> 4);
		vdd->copr.comp_copr = ((rxbuf[8] & 0xF8) >> 3) | ((rxbuf[7] & 0x0F) << 5);
	}
}

/**
 * ss_copr_read()
 *
 * read copr registers via SPI interface
 * This function returns zero on success, else a negative error code.
 */
int ss_copr_read(struct samsung_display_driver_data *vdd)
{
	u8 *rxbuf;
	u8 rx_addr;
	int tx_bpw, rx_bpw;
	int tx_size, rx_size;
	int ret = 0;
	int i;
	u8 tx_buf[1];

	LCD_DEBUG("%s ++ \n", __func__);

	if (!ss_is_panel_on(vdd)) {
		LCD_ERR("panel stste (%d) \n", vdd->panel_state);
		return -ENODEV;
	}

	if (!vdd->copr.display_read) {
		LCD_ERR("copr read is not from display driver (%d) \n", vdd->copr.display_read);
		return -ENODEV;
	}

	/* Get spi components */
	rx_addr = vdd->copr.rx_addr;
	tx_size = vdd->copr.tx_size;
	rx_size = vdd->copr.rx_size;
	tx_bpw = vdd->copr.tx_bpw;
	rx_bpw = vdd->copr.rx_bpw;

	rxbuf = kzalloc(rx_size, GFP_KERNEL);
	if (rxbuf == NULL) {
		LCD_ERR("fail to kzalloc for rxbuf \n");
		ret = -ENOMEM;
		goto err;
	}

	tx_buf[0] = rx_addr;

	ret = ss_spi_read(vdd->spi_dev, rxbuf, tx_bpw, rx_bpw, tx_buf, tx_size, rx_size);
	if (ret) {
		LCD_ERR("[SDE SPI] %s : spi read fail..(%x)\n", __func__, rx_addr);
		goto err;
	}

	for (i = 0; i < rx_size; i++)
		LCD_DEBUG("[%02d] %02x \n", i, rxbuf[i]);

	ss_copr_parse_spi_data(vdd, rxbuf);

	if (vdd->copr.ver == COPR_VER_3P0)
		LCD_DEBUG("[%d] current_copr (%d), avg_copr (%d) sld_avg (%d) \n",
			vdd->copr.current_cnt, vdd->copr.current_copr, vdd->copr.avg_copr, vdd->copr.sliding_avg_copr);
	else
		LCD_DEBUG("[%d] current_copr (%d), avg_copr (%d) , comp_copr(%d)\n",
			vdd->copr.current_cnt, vdd->copr.current_copr, vdd->copr.avg_copr, vdd->copr.comp_copr);

	/* If current_copr is over 0, copr work thread will be stopped */
	if (vdd->display_status_dsi.wait_actual_disp_on) {
		LCD_INFO("ACTUAL_DISPLAY_ON\n");
		vdd->display_status_dsi.wait_actual_disp_on = false;
	}

	LCD_DEBUG("%s -- data (%d)\n", __func__, vdd->copr.current_copr);

err:
	kfree(rxbuf);
	return ret;
}

/**
 * ss_read_copr_work()
 *
 * COPR 1.0 - Need to run work thread to get copr per frame.
 * COPR 2.0 - Do not need to run work thread to get copr avg.
 *            but for the debugging purpose, this work is used.
 */
void ss_read_copr_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct COPR *copr;

	copr = container_of(work, struct COPR, read_copr_work);
	vdd = container_of(copr, struct samsung_display_driver_data, copr);

	LCD_DEBUG("copr_calc work!!\n");

	mutex_lock(&vdd->copr.copr_lock);

	//ss_set_copr_sum(vdd);
	ss_copr_read(vdd);

	LCD_DEBUG("COPR : %02x (%d) \n", vdd->copr.current_copr, vdd->copr.current_copr);

	mutex_unlock(&vdd->copr.copr_lock);

	return;
}

void ss_copr_init(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	mutex_init(&vdd->copr.copr_val_lock);
	mutex_init(&vdd->copr.copr_lock);

	if (vdd->copr.display_read) {
		/* read_copr_wq is used for checking real frame is entered into ddi with opr value */
		vdd->copr.read_copr_wq = create_singlethread_workqueue("read_copr_wq");
		if (vdd->copr.read_copr_wq == NULL) {
			LCD_ERR("failed to create read copr workqueue..\n");
			return;
		}

		INIT_WORK(&vdd->copr.read_copr_work, (work_func_t)ss_read_copr_work);
	}

	vdd->copr.copr_on = 1;

	LCD_INFO("ver[%d] display_read[%d] tx_bpw(%d) rx_bpw(%d) tx_size(%d) rx_size(%d) rx_addr(%x) on(%d)\n",
		vdd->copr.ver, vdd->copr.display_read,
		vdd->copr.tx_bpw, vdd->copr.rx_bpw, vdd->copr.tx_size, vdd->copr.rx_size,
		vdd->copr.rx_addr, vdd->copr.copr_on);

	LCD_INFO("Success to initialized copr ..\n");

	return;
}
