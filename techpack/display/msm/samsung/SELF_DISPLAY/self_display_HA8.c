/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * DDI operation : self clock, self mask, self icon.. etc.
 * Author: QC LCD driver <cj1225.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ss_dsi_panel_common.h"
#include "self_display_HA8.h"

/* #define SELF_DISPLAY_TEST */

/*
 * make dsi_panel_cmds using image data
 */
void make_self_dispaly_img_cmds_HA8(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type cmd, u32 op)
{
	struct dsi_cmd_desc *tcmds;
	struct dsi_panel_cmd_set *pcmds;

	u32 data_size = vdd->self_disp.operation[op].img_size;
	char *data = vdd->self_disp.operation[op].img_buf;
	int i, j;
	int data_idx = 0;

	u32 p_size = CMD_ALIGN;
	u32 paylod_size = 0;
	u32 cmd_size = 0;

	if (!data) {
		LCD_ERR("data is null..\n");
		return;
	}

	if (!data_size) {
		LCD_ERR("data size is zero..\n");
		return;
	}

	/* msg.tx_buf size */
	while (p_size < MAX_PAYLOAD_SIZE) {
		 if (data_size % p_size == 0) {
			paylod_size = p_size;
		 }
		 p_size += CMD_ALIGN;
	}
	/* cmd size */
	cmd_size = data_size / paylod_size;

	LCD_INFO("[%d] total data size [%d]\n", cmd, data_size);
	LCD_INFO("cmd size [%d] msg.tx_buf size [%d]\n", cmd_size, paylod_size);

	pcmds = ss_get_cmds(vdd, cmd);
	if (SS_IS_CMDS_NULL(pcmds)) {
		pcmds->cmds = kzalloc(cmd_size * sizeof(struct dsi_cmd_desc), GFP_KERNEL);
		if (SS_IS_CMDS_NULL(pcmds)) {
			LCD_ERR("fail to kzalloc for self_mask cmds \n");
			return;
		}
	}

	pcmds->state = DSI_CMD_SET_STATE_HS;
	pcmds->count = cmd_size;

	tcmds = pcmds->cmds;
	if (tcmds == NULL) {
		LCD_ERR("tcmds is NULL \n");
		return;
	}

	for (i = 0; i < pcmds->count; i++) {
		tcmds[i].msg.type = MIPI_DSI_GENERIC_LONG_WRITE;
		tcmds[i].last_command = 1;

		/* fill image data */
		if (tcmds[i].msg.tx_buf == NULL) {
			/* +1 means HEADER TYPE 0x4C or 0x5C */
			tcmds[i].msg.tx_buf = kzalloc(paylod_size + 1, GFP_KERNEL);
			if (tcmds[i].msg.tx_buf == NULL) {
				LCD_ERR("fail to kzalloc for self_mask cmds msg.tx_buf \n");
				return;
			}
		}

		tcmds[i].msg.tx_buf[0] = (i == 0) ? 0x4C : 0x5C;

		for (j = 1; (j <= paylod_size) && (data_idx < data_size); j++)
			tcmds[i].msg.tx_buf[j] = data[data_idx++];

		tcmds[i].msg.tx_len = j;

		LCD_DEBUG("dlen (%d), data_idx (%d)\n", j, data_idx);
	}

	return;
}

static int self_time_set(struct samsung_display_driver_data *vdd, int from_self_move)
{
	u8 *cmd_pload;
	struct self_time_info st_info;
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	LCD_ERR("++\n");

	st_info = vdd->self_disp.st_info;

	LCD_INFO("Self Time Set h(%d):m(%d):s(%d).ms(%d) / 24h(%d) / Interval(%d) / Time Set(%d)\n",
		st_info.cur_h, st_info.cur_m, st_info.cur_s,
		st_info.cur_ms, st_info.disp_24h, st_info.interval, vdd->self_disp.time_set);

	pcmds = ss_get_cmds(vdd, TX_SELF_TIME_SET);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_SELF_TIME_SET..\n");
		return -ENODEV;
	}

	cmd_pload = pcmds->cmds[1].msg.tx_buf;

	cmd_pload[2] |= BIT(2);	/* SC_TIME_EN */

	if (vdd->self_disp.st_info.disp_24h)
		cmd_pload[2] |= BIT(5);
	else
		cmd_pload[2] &= ~(BIT(5));

	/* Analog Clock Should be enabled before Analog Clock Enable IOCTL */
	cmd_pload[2] |= BIT(0); /* SC_A_CLOCK_EN */
	cmd_pload[2] &= ~(BIT(1)); /* SC_DISP_ON */

	if (vdd->self_disp.sd_info.en) {
		LCD_INFO("Digital Clock Enabled\n");
		cmd_pload[2] &= ~(BIT(0)); /* SC_A_CLOCK_EN */
		cmd_pload[2] |= BIT(1);	/* SC_DISP_ON */
		cmd_pload[2] |= BIT(4); /* SC_D_CLOCK_EN */

		/* SC_D_EN_MM & SC_D_EN_HH */
		if (vdd->self_disp.sd_info.en_hh) {
			/* Clear EN_HH First */
			cmd_pload[3] &= ~(BIT(2));
			cmd_pload[3] &= ~(BIT(3));
			cmd_pload[3] |= (vdd->self_disp.sd_info.en_hh & 0x3) << 2;
		} else {
			cmd_pload[3] &= ~(BIT(2));
			cmd_pload[3] &= ~(BIT(3));
		}

		if (vdd->self_disp.sd_info.en_mm) {
			/* Clear EN_MM First */
			cmd_pload[3] &= ~(BIT(0));
			cmd_pload[3] &= ~(BIT(1));
			cmd_pload[3] |= (vdd->self_disp.sd_info.en_mm & 0x3);
		} else {
			cmd_pload[3] &= ~(BIT(0));
			cmd_pload[3] &= ~(BIT(1));
		}
	} else if (vdd->self_disp.sa_info.en) {
		LCD_INFO("Analog Clock Enabled\n");
		cmd_pload[2] |= BIT(1); /* SC_DISP_ON */
	} else if (from_self_move) {
		LCD_INFO("Self Move Without Any Clock Enabled\n");
		cmd_pload[2] &= ~(BIT(0)); /* SC_A_CLOCK_EN */
		cmd_pload[2] &= ~(BIT(4)); /* SC_D_CLOCK_EN */
	}

	cmd_pload[4] = vdd->self_disp.st_info.cur_h & 0x1F;
	cmd_pload[5] = vdd->self_disp.st_info.cur_m & 0x3F;
	cmd_pload[6] = vdd->self_disp.st_info.cur_s & 0x3F;

	switch (vdd->self_disp.st_info.interval) {
	case INTERVAL_100:
		cmd_pload[8] = 0x03;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x10;
		else
			cmd_pload[9] = 0x00;
		break;
	case INTERVAL_200:
		cmd_pload[8] = 0x06;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x11;
		else
			cmd_pload[9] = 0x01;
		break;
	case INTERVAL_500:
		cmd_pload[8] = 0x0f;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x12;
		else
			cmd_pload[9] = 0x02;
		break;
	case INTERVAL_1000:
		cmd_pload[8] = 0x1E;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x13;
		else
			cmd_pload[9] = 0x03;
		break;
	case INTERVAL_DEBUG:
		cmd_pload[8] = 0x01;
		cmd_pload[9] = 0x03;
		break;
	default: /* Default Interval is 1000 */
		cmd_pload[8] = 0x1E;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x13;
		else
			cmd_pload[9] = 0x03;
		LCD_ERR("Invalid Time Interval (%d)\n", vdd->self_disp.st_info.interval);
	}

	ss_send_cmd(vdd, TX_SELF_TIME_SET);

	LCD_ERR("--\n");

	return 0;
}

static void self_move_on(struct samsung_display_driver_data *vdd, int enable)
{
	u8 *cmd_pload;
	struct dsi_panel_cmd_set *pcmds;
	static int reset_count = 1;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ Enable(%d), Interval(%d)\n", enable, vdd->self_disp.st_info.interval);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	if (enable) {
		if (!vdd->self_disp.sa_info.en && !vdd->self_disp.sd_info.en)
			self_time_set(vdd, true);

		switch (vdd->self_disp.st_info.interval) {
		case INTERVAL_100:
			ss_send_cmd(vdd, TX_SELF_MOVE_ON_100);
			break;
		case INTERVAL_200:
			ss_send_cmd(vdd, TX_SELF_MOVE_ON_200);
			break;
		case INTERVAL_500:
			ss_send_cmd(vdd, TX_SELF_MOVE_ON_500);
			break;
		case INTERVAL_1000:
			ss_send_cmd(vdd, TX_SELF_MOVE_ON_1000);
			break;
		case INTERVAL_DEBUG:
			ss_send_cmd(vdd, TX_SELF_MOVE_ON_DEBUG);
			break;
		default: /* Default Interval is 1000 */
			ss_send_cmd(vdd, TX_SELF_MOVE_ON_1000);
			LCD_ERR("Invalid Time Interval (%d)\n", vdd->self_disp.st_info.interval);
		}
	} else {
		pcmds = ss_get_cmds(vdd, TX_SELF_MOVE_RESET);
		if (SS_IS_CMDS_NULL(pcmds)) {
			LCD_ERR("No cmds for TX_SELF_MOVE_RESET..\n");
			goto err;
		}
		cmd_pload = pcmds->cmds[1].msg.tx_buf;
		if (reset_count > 255)
			reset_count = 1;
		else
			reset_count++;
		cmd_pload[4] = reset_count & 0xFF;
		ss_send_cmd(vdd, TX_SELF_MOVE_RESET);
	}
err:
	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_ERR("-- \n");

	return;
}

static void self_icon_img_write(struct samsung_display_driver_data *vdd)
{
	LCD_ERR("++\n");
	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_SELF_ICON_SIDE_MEM_SET);
	ss_send_cmd(vdd, TX_SELF_ICON_IMAGE);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);
	LCD_ERR("--\n");
}

static int self_icon_set(struct samsung_display_driver_data *vdd)
{
	u8 *cmd_pload;
	struct self_icon_info si_info;
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	LCD_ERR("++\n");

	si_info = vdd->self_disp.si_info;

	LCD_INFO("Self Icon Enable(%d), x(%d), y(%d), w(%d), h(%d)\n",
		si_info.en, si_info.pos_x, si_info.pos_y,
		si_info.width, si_info.height);

	pcmds = ss_get_cmds(vdd, TX_SELF_ICON_GRID);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_SELF_ICON_GRID..\n");
		return -ENODEV;
	}
	cmd_pload = pcmds->cmds[1].msg.tx_buf;

	if (si_info.en) {
		cmd_pload[2] |= BIT(0); /* SI_ICON_EN */

		cmd_pload[3] = (si_info.pos_x & 0x700) >> 8;
		cmd_pload[4] = si_info.pos_x & 0xFF;

		cmd_pload[5] = (si_info.pos_y & 0xF00) >> 8;
		cmd_pload[6] = si_info.pos_y & 0xFF;

		cmd_pload[7] = (si_info.width & 0x700) >> 8;
		cmd_pload[8] = si_info.width & 0xFF;

		cmd_pload[9] = (si_info.height & 0xF00) >> 8;
		cmd_pload[10] = si_info.height & 0xFF;
	} else {
		cmd_pload[2] &= ~(BIT(0)); /* SI_ICON_EN */
	}

	/* Self Grid setting from last stored information */
	if (vdd->self_disp.sg_info.en) {
		cmd_pload[2] |= BIT(4); /* SG_GRID_EN */

		cmd_pload[3] = (vdd->self_disp.sg_info.s_pos_x & 0x700) >> 8;
		cmd_pload[4] = vdd->self_disp.sg_info.s_pos_x & 0xFF;

		cmd_pload[5] = (vdd->self_disp.sg_info.s_pos_y & 0xF00) >> 8;
		cmd_pload[6] = vdd->self_disp.sg_info.s_pos_y & 0xFF;

		cmd_pload[7] = (vdd->self_disp.sg_info.e_pos_x & 0x700) >> 8;
		cmd_pload[8] = vdd->self_disp.sg_info.e_pos_x & 0xFF;

		cmd_pload[9] = (vdd->self_disp.sg_info.e_pos_y & 0xF00) >> 8;
		cmd_pload[10] = vdd->self_disp.sg_info.e_pos_y & 0xFF;
	} else {
		cmd_pload[2] &= ~(BIT(4));
	}

	ss_send_cmd(vdd, TX_SELF_ICON_GRID);

	LCD_ERR("--\n");

	return 0;
}

static int self_grid_set(struct samsung_display_driver_data *vdd)
{
	u8 *cmd_pload;
	struct self_grid_info sg_info;
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	LCD_ERR("++\n");

	sg_info = vdd->self_disp.sg_info;

	LCD_INFO("Self Grid Enable(%d), s_x(%d), s_y(%d), e_x(%d), e_y(%d)\n",
		sg_info.en, sg_info.s_pos_x, sg_info.s_pos_y,
		sg_info.e_pos_x, sg_info.e_pos_y);

	pcmds = ss_get_cmds(vdd, TX_SELF_ICON_GRID);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_SELF_ICON_GRID..\n");
		return -ENODEV;
	}

	cmd_pload = pcmds->cmds[1].msg.tx_buf;

	if (sg_info.en) {
		cmd_pload[2] |= BIT(4); /* SG_GRID_EN */

		cmd_pload[3] = (sg_info.s_pos_x & 0x700) >> 8;
		cmd_pload[4] = sg_info.s_pos_x & 0xFF;

		cmd_pload[5] = (sg_info.s_pos_y & 0xF00) >> 8;
		cmd_pload[6] = sg_info.s_pos_y & 0xFF;

		cmd_pload[7] = (sg_info.e_pos_x & 0x700) >> 8;
		cmd_pload[8] = sg_info.e_pos_x & 0xFF;

		cmd_pload[9] = (sg_info.e_pos_y & 0xF00) >> 8;
		cmd_pload[10] = sg_info.e_pos_y & 0xFF;
	} else {
		cmd_pload[2] &= ~(BIT(4)); /* SG_GRID_EN */
	}

	if (vdd->self_disp.si_info.en) {
		cmd_pload[2] |= BIT(0); /* SI_ICON_EN */

		cmd_pload[3] = (vdd->self_disp.si_info.pos_x & 0x700) >> 8;
		cmd_pload[4] = vdd->self_disp.si_info.pos_x & 0xFF;

		cmd_pload[5] = (vdd->self_disp.si_info.pos_y & 0xF00) >> 8;
		cmd_pload[6] = vdd->self_disp.si_info.pos_y & 0xFF;

		cmd_pload[7] = (vdd->self_disp.si_info.width & 0x700) >> 8;
		cmd_pload[8] = vdd->self_disp.si_info.width & 0xFF;

		cmd_pload[9] = (vdd->self_disp.si_info.height & 0xF00) >> 8;
		cmd_pload[10] = vdd->self_disp.si_info.height & 0xFF;
	} else {
		cmd_pload[2] &= ~(BIT(0)); /* SI_ICON_EN */
	}

	ss_send_cmd(vdd, TX_SELF_ICON_GRID);

	LCD_ERR("--\n");

	return 0;
}
static void self_aclock_on(struct samsung_display_driver_data *vdd, int enable)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	if (enable)
		ss_send_cmd(vdd, TX_SELF_ACLOCK_ON);
	else
		ss_send_cmd(vdd, TX_SELF_ACLOCK_HIDE);

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_ERR("-- \n");

	return;
}

static void self_aclock_img_write(struct samsung_display_driver_data *vdd)
{
	LCD_ERR("++\n");
	mutex_lock(&vdd->self_disp.vdd_self_display_lock);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_SELF_ACLOCK_SIDE_MEM_SET);
	ss_send_cmd(vdd, TX_SELF_ACLOCK_IMAGE);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);
	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);
	LCD_ERR("--\n");
}

static int self_aclock_set(struct samsung_display_driver_data *vdd)
{
	u8 *cmd_pload;
	struct self_analog_clk_info sa_info;
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	LCD_ERR("++\n");

	sa_info = vdd->self_disp.sa_info;

	LCD_INFO("Self Analog Clock Enable(%d), x(%d), y(%d), rot(%d)\n",
		sa_info.en, sa_info.pos_x, sa_info.pos_y, sa_info.rotate);

	if (!sa_info.en)
		goto skip_update;

	pcmds = ss_get_cmds(vdd, TX_SELF_ACLOCK_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_SELF_ACLOCK_ON..\n");
		return -ENODEV;
	}

	cmd_pload = pcmds->cmds[1].msg.tx_buf;

	/* Time Update from Last Time Info */
	cmd_pload[4] = vdd->self_disp.st_info.cur_h & 0x1F;
	cmd_pload[5] = vdd->self_disp.st_info.cur_m & 0x3F;
	cmd_pload[6] = vdd->self_disp.st_info.cur_s & 0x3F;

	switch (vdd->self_disp.st_info.interval) {
	case INTERVAL_100:
		cmd_pload[8] = 0x03;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x10;
		else
			cmd_pload[9] = 0x00;
		break;
	case INTERVAL_200:
		cmd_pload[8] = 0x06;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x11;
		else
			cmd_pload[9] = 0x01;
		break;
	case INTERVAL_500:
		cmd_pload[8] = 0x0f;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x12;
		else
			cmd_pload[9] = 0x02;
		break;
	case INTERVAL_1000:
		cmd_pload[8] = 0x1E;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x13;
		else
			cmd_pload[9] = 0x03;
		break;
	case INTERVAL_DEBUG:
		cmd_pload[8] = 0x01;
		cmd_pload[9] = 0x03;
		break;
	default: /* Default Interval is 1000 */
		cmd_pload[8] = 0x1E;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x13;
		else
			cmd_pload[9] = 0x03;
		LCD_ERR("Invalid Time Interval (%d)\n", vdd->self_disp.st_info.interval);
	}

	/* Self Analog Clock Position Update */
	cmd_pload[10] = (sa_info.pos_x & 0x700) >> 8;
	cmd_pload[11] = sa_info.pos_x & 0xFF;
	cmd_pload[12] = (sa_info.pos_y & 0xF00) >> 8;
	cmd_pload[13] = sa_info.pos_y & 0xFF;

	/* Self Analog Clock Rotation Setting */
	switch (sa_info.rotate) {
	case ROTATE_0:
		cmd_pload[14] &= ~(BIT(0));
		cmd_pload[14] &= ~(BIT(1));
		break;
	case ROTATE_90:
		cmd_pload[14] |= BIT(0);
		cmd_pload[14] &= ~(BIT(1));
		break;
	case ROTATE_180:
		cmd_pload[14] &= ~(BIT(0));
		cmd_pload[14] |= BIT(1);
		break;
	case ROTATE_270:
		cmd_pload[14] |= BIT(0);
		cmd_pload[14] |= BIT(1);
		break;
	default:
		LCD_ERR("Invalid Rotation Setting, (%d)\n", sa_info.rotate);
	}

	vdd->self_disp.time_set = true;

skip_update:
	self_aclock_on(vdd, sa_info.en);

	LCD_ERR("-- \n");

	return 0;
}

static void self_dclock_on(struct samsung_display_driver_data *vdd, int enable)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	if (enable)
		ss_send_cmd(vdd, TX_SELF_DCLOCK_ON);
	else
		ss_send_cmd(vdd, TX_SELF_DCLOCK_HIDE);

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_ERR("-- \n");

	return;
}

static void self_dclock_img_write(struct samsung_display_driver_data *vdd)
{
	LCD_ERR("++\n");
	mutex_lock(&vdd->self_disp.vdd_self_display_lock);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_SELF_DCLOCK_SIDE_MEM_SET);
	ss_send_cmd(vdd, TX_SELF_DCLOCK_IMAGE);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);
	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);
	LCD_ERR("--\n");
}

static int self_dclock_set(struct samsung_display_driver_data *vdd)
{
	u8 *cmd_pload;
	struct self_digital_clk_info sd_info;
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	LCD_ERR("++\n");

	sd_info = vdd->self_disp.sd_info;

	LCD_INFO("Self Digital Clock Enable(%d), 24H(%d), EN_HH(%d), EN_MM(%d), POS_1(%d,%d), POS_2(%d,%d), POS_3(%d,%d), POS_4(%d,%d), W(%d), H(%d)\n",
				sd_info.en, vdd->self_disp.st_info.disp_24h,
				sd_info.en_hh, sd_info.en_mm,
				sd_info.pos1_x, sd_info.pos1_y,
				sd_info.pos2_x, sd_info.pos2_y,
				sd_info.pos3_x, sd_info.pos3_y,
				sd_info.pos4_x, sd_info.pos4_y,
				sd_info.img_width, sd_info.img_height);

	if (!sd_info.en)
		goto skip_update;

	pcmds = ss_get_cmds(vdd, TX_SELF_DCLOCK_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_SELF_DCLOCK_ON..\n");
		return -ENODEV;
	}
	cmd_pload = pcmds->cmds[1].msg.tx_buf;

	if (vdd->self_disp.st_info.disp_24h)
		cmd_pload[2] |= BIT(5);
	else
		cmd_pload[2] &= ~(BIT(5));

	/* Time Update from Last Time Info */
	cmd_pload[4] = vdd->self_disp.st_info.cur_h & 0x1F;
	cmd_pload[5] = vdd->self_disp.st_info.cur_m & 0x3F;
	cmd_pload[6] = vdd->self_disp.st_info.cur_s & 0x3F;

	switch (vdd->self_disp.st_info.interval) {
	case INTERVAL_100:
		cmd_pload[8] = 0x03;
		cmd_pload[9] = 0x00;
		break;
	case INTERVAL_200:
		cmd_pload[8] = 0x06;
		cmd_pload[9] = 0x01;
		break;
	case INTERVAL_500:
		cmd_pload[8] = 0x0f;
		cmd_pload[9] = 0x02;
		break;
	case INTERVAL_1000:
		cmd_pload[8] = 0x1E;
		cmd_pload[9] = 0x03;
		break;
	case INTERVAL_DEBUG:
		cmd_pload[8] = 0x01;
		cmd_pload[9] = 0x03;
		break;
	default: /* Default Interval is 1000 */
		cmd_pload[8] = 0x1E;
		if (vdd->self_disp.time_set)
			cmd_pload[9] = 0x13;
		else
			cmd_pload[9] = 0x03;
		LCD_ERR("Invalid Time Interval (%d)\n", vdd->self_disp.st_info.interval);
	}

	if (sd_info.en_hh) {
		/* Clear EN_HH First */
		cmd_pload[3] &= ~(BIT(2));
		cmd_pload[3] &= ~(BIT(3));
		cmd_pload[3] |= (sd_info.en_hh & 0x3) << 2;
	} else {
		cmd_pload[3] &= ~(BIT(2));
		cmd_pload[3] &= ~(BIT(3));
	}

	if (sd_info.en_mm) {
		/* Clear EN_MM First */
		cmd_pload[3] &= ~(BIT(0));
		cmd_pload[3] &= ~(BIT(1));
		cmd_pload[3] |= (sd_info.en_mm & 0x3);
	} else {
		cmd_pload[3] &= ~(BIT(0));
		cmd_pload[3] &= ~(BIT(1));
	}

	cmd_pload[22] = (sd_info.pos1_x & 0x700) >> 8;
	cmd_pload[23] = sd_info.pos1_x & 0xFF;

	cmd_pload[24] = (sd_info.pos1_y & 0xF00) >> 8;
	cmd_pload[25] = sd_info.pos1_y & 0xFF;

	cmd_pload[26] = (sd_info.pos2_x & 0x700) >> 8;
	cmd_pload[27] = sd_info.pos2_x & 0xFF;

	cmd_pload[28] = (sd_info.pos2_y & 0xF00) >> 8;
	cmd_pload[29] = sd_info.pos2_y & 0xFF;

	cmd_pload[30] = (sd_info.pos3_x & 0x700) >> 8;
	cmd_pload[31] = sd_info.pos3_x & 0xFF;

	cmd_pload[32] = (sd_info.pos3_y & 0xF00) >> 8;
	cmd_pload[33] = sd_info.pos3_y & 0xFF;

	cmd_pload[34] = (sd_info.pos4_x & 0x700) >> 8;
	cmd_pload[35] = sd_info.pos4_x & 0xFF;

	cmd_pload[36] = (sd_info.pos4_y & 0xF00) >> 8;
	cmd_pload[37] = sd_info.pos4_y & 0xFF;

	cmd_pload[38] = (sd_info.img_width & 0x700) >> 8;
	cmd_pload[39] = sd_info.img_width & 0xFF;

	cmd_pload[40] = (sd_info.img_height & 0xF00) >> 8;
	cmd_pload[41] = sd_info.img_height & 0xFF;

	vdd->self_disp.time_set = true;

skip_update:
	self_dclock_on(vdd, sd_info.en);

	LCD_ERR("-- \n");

	return 0;
}

static void self_blinking_on(struct samsung_display_driver_data *vdd, int enable)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	if (enable) {
		ss_send_cmd(vdd, TX_SELF_DCLOCK_BLINKING_ON);
	} else {
		ss_send_cmd(vdd, TX_SELF_DCLOCK_BLINKING_OFF);
	}

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_ERR("-- \n");

	return;
}

static void self_mask_img_write(struct samsung_display_driver_data *vdd)
{
	if (!vdd->self_disp.is_support) {
		LCD_ERR("self display is not supported..(%d) \n",
						vdd->self_disp.is_support);
		return;
	}

	LCD_ERR("++\n");

	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list)) {
		int wait_cnt = 0; /* 1000 * 0.5ms = 500ms */
		if (++wait_cnt > 500)
			break;
		usleep_range(500, 500);
	}
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_SET_PRE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_IMAGE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_SET_POST, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 1);

	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_SELF_MASK_SIDE_MEM_SET);
	ss_send_cmd(vdd, TX_SELF_MASK_IMAGE);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_SET_PRE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_IMAGE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_SET_POST, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 0);
	vdd->exclusive_tx.enable = 0;
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);

	LCD_ERR("--\n");
}

static void self_mask_on(struct samsung_display_driver_data *vdd, int enable)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR("self display is not supported..(%d) \n",
						vdd->self_disp.is_support);
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	if (enable) {
		if (vdd->is_factory_mode && vdd->self_disp.factory_support)
			ss_send_cmd(vdd, TX_SELF_MASK_ON_FACTORY);
		else
			ss_send_cmd(vdd, TX_SELF_MASK_ON);
	} else
		ss_send_cmd(vdd, TX_SELF_MASK_OFF);

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_ERR("-- \n");

	return;
}

static int self_display_debug(struct samsung_display_driver_data *vdd)
{
	char buf[32];

	if (ss_get_cmds(vdd, RX_SELF_DISP_DEBUG)->count) {

		ss_panel_data_read(vdd, RX_SELF_DISP_DEBUG, buf, LEVEL1_KEY);

		vdd->self_disp.debug.SI_X_O = ((buf[14] & 0x07) << 8);
		vdd->self_disp.debug.SI_X_O |= (buf[15] & 0xFF);

		vdd->self_disp.debug.SI_Y_O = ((buf[16] & 0x0F) << 8);
		vdd->self_disp.debug.SI_Y_O |= (buf[17] & 0xFF);

		vdd->self_disp.debug.SM_SUM_O = ((buf[6] & 0xFF) << 24);
		vdd->self_disp.debug.SM_SUM_O |= ((buf[7] & 0xFF) << 16);
		vdd->self_disp.debug.SM_SUM_O |= ((buf[8] & 0xFF) << 8);
		vdd->self_disp.debug.SM_SUM_O |= (buf[9] & 0xFF);

		vdd->self_disp.debug.MEM_SUM_O = ((buf[10] & 0xFF) << 24);
		vdd->self_disp.debug.MEM_SUM_O |= ((buf[11] & 0xFF) << 16);
		vdd->self_disp.debug.MEM_SUM_O |= ((buf[12] & 0xFF) << 8);
		vdd->self_disp.debug.MEM_SUM_O |= (buf[13] & 0xFF);

		LCD_INFO("SI_X_O(%u) SI_Y_O(%u) MEM_SUM_O(%X) SM_SUM_O(%X)\n",
			vdd->self_disp.debug.SI_X_O,
			vdd->self_disp.debug.SI_Y_O,
			vdd->self_disp.debug.MEM_SUM_O,
			vdd->self_disp.debug.SM_SUM_O);

		if (vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum !=
					vdd->self_disp.debug.SM_SUM_O) {
			LCD_ERR("self mask img checksum fail!!\n");
			return -1;
		}
	}

	return 0;
}

static int self_display_aod_enter(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_DEBUG("self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}

	LCD_INFO("++\n");

	if (!vdd->self_disp.on) {
		if (vdd->self_disp.operation[FLAG_SELF_ICON].select)
			self_icon_img_write(vdd);
		if (vdd->self_disp.operation[FLAG_SELF_ACLK].select)
			self_aclock_img_write(vdd);
		if (vdd->self_disp.operation[FLAG_SELF_DCLK].select)
			self_dclock_img_write(vdd);

		/* Self display on */
		ss_send_cmd(vdd, TX_SELF_DISP_ON);

		self_mask_on(vdd, false);
#ifdef SELF_DISPLAY_TEST
		//self_dclock_img_write(vdd);
		self_aclock_img_write(vdd);
#endif
	}

	vdd->self_disp.on = true;

	LCD_INFO("--\n");

	return ret;
}

static int self_display_aod_exit(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_DEBUG("self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}

	LCD_INFO("++\n");

	/* self display off */
	ss_send_cmd(vdd, TX_SELF_DISP_OFF);

	self_mask_on(vdd, true);

	vdd->self_disp.sa_info.en = false;
	vdd->self_disp.sd_info.en = false;
	vdd->self_disp.si_info.en = false;
	vdd->self_disp.sg_info.en = false;
	vdd->self_disp.time_set = false;

	vdd->self_disp.on = false;
	LCD_INFO("--\n");

	return ret;
}

/*
 * self_display_ioctl() : get ioctl from aod framework.
 *                           set self display related registers.
 */
static long self_display_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;

	void __user *argp = (void __user *)arg;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -ENODEV;
	}

	if (!vdd->self_disp.on) {
		LCD_ERR("self_display was turned off\n");
		return -EPERM;
	}

	if ((_IOC_TYPE(cmd) != SELF_DISPLAY_IOCTL_MAGIC) ||
				(_IOC_NR(cmd) >= IOCTL_SELF_MAX)) {
		LCD_ERR("TYPE(%u) NR(%u) is wrong..\n",
			_IOC_TYPE(cmd), _IOC_NR(cmd));
		return -EINVAL;
	}

	LCD_INFO("cmd = %s\n", cmd == IOCTL_SELF_MOVE_EN ? "IOCTL_SELF_MOVE_EN" :
				cmd == IOCTL_SELF_MOVE_OFF ? "IOCTL_SELF_MOVE_OFF" :
				cmd == IOCTL_SET_ICON ? "IOCTL_SET_ICON" :
				cmd == IOCTL_SET_GRID ? "IOCTL_SET_GRID" :
				cmd == IOCTL_SET_ANALOG_CLK ? "IOCTL_SET_ANALOG_CLK" :
				cmd == IOCTL_SET_DIGITAL_CLK ? "IOCTL_SET_DIGITAL_CLK" :
				cmd == IOCTL_SET_TIME ? "IOCTL_SET_TIME" :
				cmd == IOCTL_SET_PARTIAL_HLPM_SCAN ? "IOCTL_PARTIAL_HLPM_SCAN" : "IOCTL_ERR");

	switch (cmd) {
	case IOCTL_SELF_MOVE_EN:
		self_move_on(vdd, true);
		break;
	case IOCTL_SELF_MOVE_OFF:
		self_move_on(vdd, false);
		break;
	case IOCTL_SET_ICON:
		ret = copy_from_user(&vdd->self_disp.si_info, argp,
					sizeof(vdd->self_disp.si_info));
		if (ret) {
			LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
			goto error;
		}

		self_icon_set(vdd);
		break;
	case IOCTL_SET_GRID:
		ret = copy_from_user(&vdd->self_disp.sg_info, argp,
					sizeof(vdd->self_disp.sg_info));
		if (ret) {
			LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
			goto error;
		}

		ret = self_grid_set(vdd);
		break;
	case IOCTL_SET_ANALOG_CLK:
		ret = copy_from_user(&vdd->self_disp.sa_info, argp,
					sizeof(vdd->self_disp.sa_info));
		if (ret) {
			LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
			goto error;
		}

		ret = self_aclock_set(vdd);
		break;
	case IOCTL_SET_DIGITAL_CLK:
		ret = copy_from_user(&vdd->self_disp.sd_info, argp,
					sizeof(vdd->self_disp.sd_info));
		if (ret) {
			LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
			goto error;
		}

		ret = self_dclock_set(vdd);
		break;
	case IOCTL_SET_TIME:
		ret = copy_from_user(&vdd->self_disp.st_info, argp,
					sizeof(vdd->self_disp.st_info));
		if (ret) {
			LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
			goto error;
		}

		ret = self_time_set(vdd, false);
		break;
	default:
		LCD_ERR("invalid cmd : %u \n", cmd);
		break;
	}
error:

	return ret;
}

/*
 * self_display_write() : get image data from aod framework.
 *                           prepare for dsi_panel_cmds.
 */
static ssize_t self_display_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;

	char op_buf[IMAGE_HEADER_SIZE];
	u32 op = 0;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	if (unlikely(!buf)) {
		LCD_ERR("invalid read buffer\n");
		return -EINVAL;
	}

	if (count <= IMAGE_HEADER_SIZE) {
		LCD_ERR("Invalid Buffer Size (%d)\n", (int)count);
		return -EINVAL;
	}

	/*
	 * get 2byte flas to distinguish what operation is passing
	 */
	ret = copy_from_user(op_buf, buf, IMAGE_HEADER_SIZE);
	if (unlikely(ret < 0)) {
		LCD_ERR("failed to copy_from_user (header)\n");
		return ret;
	}

	LCD_INFO("Header Buffer = %c%c\n", op_buf[0], op_buf[1]);

	if (op_buf[0] == 'I' && op_buf[1] == 'C')
		op = FLAG_SELF_ICON;
	else if (op_buf[0] == 'A' && op_buf[1] == 'C')
		op = FLAG_SELF_ACLK;
	else if (op_buf[0] == 'D' && op_buf[1] == 'C')
		op = FLAG_SELF_DCLK;
	else {
		LCD_ERR("Invalid Header, (%c%c)\n", op_buf[0], op_buf[1]);
		return -EINVAL;
	}

	LCD_INFO("flag (%d) \n", op);

	if (op >= FLAG_SELF_DISP_MAX) {
		LCD_ERR("invalid data flag : %d \n", op);
		return -EINVAL;
	}

	if (count > vdd->self_disp.operation[op].img_size+IMAGE_HEADER_SIZE) {
		LCD_ERR("Buffer OverFlow Detected!! Buffer_Size(%d) Write_Size(%d)\n",
			vdd->self_disp.operation[op].img_size, (int)count);
		return -EINVAL;
	}

	vdd->self_disp.operation[op].wpos = *ppos;
	vdd->self_disp.operation[op].wsize = count;

	ret = copy_from_user(vdd->self_disp.operation[op].img_buf, buf+IMAGE_HEADER_SIZE, count-IMAGE_HEADER_SIZE);
	if (unlikely(ret < 0)) {
		LCD_ERR("failed to copy_from_user (data)\n");
		return ret;
	}

	switch (op) {
	case FLAG_SELF_MOVE:
		// MOVE has no image data..
		break;
	case FLAG_SELF_MASK:
		make_self_dispaly_img_cmds_HA8(vdd, TX_SELF_MASK_IMAGE, op);
		vdd->self_disp.operation[op].select = true;
		break;
	case FLAG_SELF_ICON:
		make_self_dispaly_img_cmds_HA8(vdd, TX_SELF_ICON_IMAGE, op);
		vdd->self_disp.operation[op].select = true;
		break;
	case FLAG_SELF_GRID:
		// GRID has no image data..
		break;
	case FLAG_SELF_ACLK:
		make_self_dispaly_img_cmds_HA8(vdd, TX_SELF_ACLOCK_IMAGE, op);
		vdd->self_disp.operation[op].select = true;
		vdd->self_disp.operation[FLAG_SELF_DCLK].select = false;
		break;
	case FLAG_SELF_DCLK:
		make_self_dispaly_img_cmds_HA8(vdd, TX_SELF_DCLOCK_IMAGE, op);
		vdd->self_disp.operation[op].select = true;
		vdd->self_disp.operation[FLAG_SELF_ACLK].select = false;
		break;
	default:
		LCD_ERR("invalid data flag %d \n", op);
		break;
	}

	return ret;
}

static int self_display_open(struct inode *inode, struct file *file)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	vdd->self_disp.file_open = 1;

	LCD_DEBUG("[open]\n");

	return 0;
}

static int self_display_release(struct inode *inode, struct file *file)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	vdd->self_disp.file_open = 0;

	LCD_DEBUG("[release]\n");

	return 0;
}

static const struct file_operations self_display_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = self_display_ioctl,
	.open = self_display_open,
	.release = self_display_release,
	.write = self_display_write,
};

#define DEV_NAME_SIZE 24
int self_display_init_HA8(struct samsung_display_driver_data *vdd)
{
	int ret = 0;
	static char devname[DEV_NAME_SIZE] = {'\0', };

	struct dsi_panel *panel = NULL;
	struct mipi_dsi_host *host = NULL;
	struct dsi_display *display = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR("Self Display is not supported\n");
		return -EINVAL;
	}

	panel = (struct dsi_panel *)vdd->msm_private;
	host = panel->mipi_device.host;
	display = container_of(host, struct dsi_display, host);

	mutex_init(&vdd->self_disp.vdd_self_display_lock);

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		snprintf(devname, DEV_NAME_SIZE, "self_display");
	else
		snprintf(devname, DEV_NAME_SIZE, "self_display%d", vdd->ndx);

	vdd->self_disp.dev.minor = MISC_DYNAMIC_MINOR;
	vdd->self_disp.dev.name = devname;
	vdd->self_disp.dev.fops = &self_display_fops;
	vdd->self_disp.dev.parent = &display->pdev->dev;

	vdd->self_disp.aod_enter = self_display_aod_enter;
	vdd->self_disp.aod_exit = self_display_aod_exit;
	vdd->self_disp.self_mask_img_write = self_mask_img_write;
	vdd->self_disp.self_mask_on= self_mask_on;
	vdd->self_disp.self_blinking_on = self_blinking_on;
	vdd->self_disp.self_display_debug = self_display_debug;

	ret = misc_register(&vdd->self_disp.dev);
	if (ret) {
		LCD_ERR("failed to register driver : %d\n", ret);
		vdd->self_disp.is_support = false;
		return -ENODEV;
	}

	LCD_INFO("Success to register self_disp device..(%d)\n", ret);

	return ret;
}

MODULE_DESCRIPTION("Self Display driver");
MODULE_LICENSE("GPL");

