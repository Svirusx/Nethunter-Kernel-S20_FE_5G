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
#include "self_display_HAB.h"

/* #define SELF_DISPLAY_TEST */

/*
 * make dsi_panel_cmds using image data
 */
void make_self_dispaly_img_cmds_HAB(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type cmd, u32 op)
{
	struct dsi_cmd_desc *tcmds;
	struct dsi_panel_cmd_set *pcmds;

	u32 data_size = vdd->self_disp.operation[op].img_size;
	char *data = vdd->self_disp.operation[op].img_buf;
	int i, j;
	int data_idx = 0;
	u32 check_sum_0 = 0;
	u32 check_sum_1 = 0;
	u32 check_sum_2 = 0;
	u32 check_sum_3 = 0;

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
	if (IS_ERR_OR_NULL(pcmds->cmds)) {
		LCD_ERR("pcmds->cmds is null!!\n");
		pcmds->cmds = kzalloc(cmd_size * sizeof(struct dsi_cmd_desc), GFP_KERNEL);
		if (IS_ERR_OR_NULL(pcmds->cmds)) {
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

	/* Image Check Sum Calculation */
	for (i = 0; i < data_size; i=i+4)
		check_sum_0 += data[i];

	for (i = 1; i < data_size; i=i+4)
		check_sum_1 += data[i];

	for (i = 2; i < data_size; i=i+4)
		check_sum_2 += data[i];

	for (i = 3; i < data_size; i=i+4)
		check_sum_3 += data[i];

	LCD_INFO("[CheckSum] cmd=%d, data_size = %d, cs_0 = 0x%X, cs_1 = 0x%X, cs_2 = 0x%X, cs_3 = 0x%X\n", cmd, data_size, check_sum_0, check_sum_1, check_sum_2, check_sum_3);

	vdd->self_disp.operation[op].img_checksum_cal = (check_sum_3 & 0xFF);
	vdd->self_disp.operation[op].img_checksum_cal |= ((check_sum_2 & 0xFF) << 8);
	vdd->self_disp.operation[op].img_checksum_cal |= ((check_sum_1 & 0xFF) << 16);
	vdd->self_disp.operation[op].img_checksum_cal |= ((check_sum_0 & 0xFF) << 24);

	return;
}

void make_mass_self_display_img_cmds_HAB(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type cmd, u32 op)
{
	struct dsi_cmd_desc *tcmds;
	struct dsi_panel_cmd_set *pcmds;

	u32 data_size = vdd->self_disp.operation[op].img_size;
	char *data = vdd->self_disp.operation[op].img_buf;
	u32 data_idx = 0;
	u32 payload_len = 0;
	u32 cmd_cnt = 0;
	u32 p_len = 0;
	u32 c_cnt = 0;
	int i;
	u32 check_sum_0 = 0;
	u32 check_sum_1 = 0;
	u32 check_sum_2 = 0;
	u32 check_sum_3 = 0;

	if (!data) {
		LCD_ERR("data is null..\n");
		return;
	}

	if (!data_size) {
		LCD_ERR("data size is zero..\n");
		return;
	}

	payload_len = data_size + (data_size + MASS_CMD_ALIGN - 1)/MASS_CMD_ALIGN;
	cmd_cnt = (payload_len + MAX_PAYLOAD_SIZE_MASS - 1) / MAX_PAYLOAD_SIZE_MASS;
	LCD_INFO("[%d] total data size [%d], total cmd len[%d], cmd count [%d]\n",
					cmd, data_size, payload_len, cmd_cnt);

	pcmds = ss_get_cmds(vdd, cmd);
	if (IS_ERR_OR_NULL(pcmds->cmds)) {
		LCD_ERR("pcmds->cmds is null!!\n");
		pcmds->cmds = kzalloc(cmd_cnt * sizeof(struct dsi_cmd_desc), GFP_KERNEL);
		if (IS_ERR_OR_NULL(pcmds->cmds)) {
			LCD_ERR("fail to kzalloc for self_mask cmds \n");
			return;
		}
	}

	pcmds->state = DSI_CMD_SET_STATE_HS;
	pcmds->count = cmd_cnt;

	tcmds = pcmds->cmds;
	if (tcmds == NULL) {
		LCD_ERR("tcmds is NULL \n");
		return;
	}
	/* fill image data */

	SDE_ATRACE_BEGIN("mass_cmd_generation");
	for (c_cnt = 0; c_cnt < pcmds->count ; c_cnt++) {
		tcmds[c_cnt].msg.type = MIPI_DSI_GENERIC_LONG_WRITE;
		tcmds[c_cnt].last_command = 1;

		/* Memory Alloc for each cmds */
		if (tcmds[c_cnt].msg.tx_buf == NULL) {
			/* HEADER TYPE 0x4C or 0x5C */
			tcmds[c_cnt].msg.tx_buf = vzalloc(MAX_PAYLOAD_SIZE_MASS);
			if (tcmds[c_cnt].msg.tx_buf == NULL) {
				LCD_ERR("fail to vzalloc for self_mask cmds msg.tx_buf \n");
				return;
			}
		}

		/* Copy from Data Buffer to each cmd Buffer */
		for (p_len = 0; p_len < MAX_PAYLOAD_SIZE_MASS && data_idx < data_size ; p_len++) {
			if (p_len % MASS_CMD_ALIGN)
				tcmds[c_cnt].msg.tx_buf[p_len] = data[data_idx++];
			else
				tcmds[c_cnt].msg.tx_buf[p_len] = (p_len == 0 && c_cnt == 0) ? 0x4C : 0x5C;

		}
		tcmds[c_cnt].msg.tx_len = p_len;
	}

	/* Image Check Sum Calculation */
	for (i = 0; i < data_size; i=i+4)
		check_sum_0 += data[i];

	for (i = 1; i < data_size; i=i+4)
		check_sum_1 += data[i];

	for (i = 2; i < data_size; i=i+4)
		check_sum_2 += data[i];

	for (i = 3; i < data_size; i=i+4)
		check_sum_3 += data[i];

	LCD_INFO("[CheckSum] cmd=%d, data_size = %d, cs_0 = 0x%X, cs_1 = 0x%X, cs_2 = 0x%X, cs_3 = 0x%X\n", cmd, data_size, check_sum_0, check_sum_1, check_sum_2, check_sum_3);

	vdd->self_disp.operation[op].img_checksum_cal = (check_sum_3 & 0xFF);
	vdd->self_disp.operation[op].img_checksum_cal |= ((check_sum_2 & 0xFF) << 8);
	vdd->self_disp.operation[op].img_checksum_cal |= ((check_sum_1 & 0xFF) << 16);
	vdd->self_disp.operation[op].img_checksum_cal |= ((check_sum_0 & 0xFF) << 24);

	SDE_ATRACE_END("mass_cmd_generation");

	LCD_INFO("Total Cmd Count(%d), Last Cmd Payload Len(%d)\n", c_cnt, tcmds[c_cnt-1].msg.tx_len);

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

	cmd_pload[2] = 0x03; /* SC_TIME_EN | SC_T_DISP_ON */

	cmd_pload[3] = vdd->self_disp.st_info.cur_h & 0x1F;
	cmd_pload[4] = vdd->self_disp.st_info.cur_m & 0x3F;
	cmd_pload[5] = vdd->self_disp.st_info.cur_s & 0x3F;

	/* Time Compensation should be enabled for Analog Clock only.
	   But, Do not enable time compensation before analog clock on */
	switch (vdd->self_disp.st_info.interval) {
	case INTERVAL_100:
		cmd_pload[7] = 0x03;
		if (vdd->self_disp.time_set)
			cmd_pload[8] = 0x10;
		else
			cmd_pload[8] = 0x00;
		break;
	case INTERVAL_200:
		cmd_pload[7] = 0x06;
		if (vdd->self_disp.time_set)
			cmd_pload[8] = 0x11;
		else
			cmd_pload[8] = 0x01;
		break;
	case INTERVAL_500:
		cmd_pload[7] = 0x0f;
		if (vdd->self_disp.time_set)
			cmd_pload[8] = 0x12;
		else
			cmd_pload[8] = 0x02;
		break;
	case INTERVAL_1000:
		cmd_pload[7] = 0x1e;
		if (vdd->self_disp.time_set)
			cmd_pload[8] = 0x13;
		else
			cmd_pload[8] = 0x03;
		break;
	case INTERVAL_DEBUG:
		cmd_pload[7] = 0x01;
		cmd_pload[8] = 0x03;
		break;
	default: /* Default Interval is 1000 */
		cmd_pload[7] = 0x1e;
		if (vdd->self_disp.time_set)
			cmd_pload[8] = 0x13;
		else
			cmd_pload[8] = 0x03;
		LCD_ERR("Invalid Time Interval (%d)\n", vdd->self_disp.st_info.interval);
	}

	if (from_self_move) {
		LCD_INFO("Self Move Without Any Clock Enabled\n");
		cmd_pload[1] = cmd_pload[3] = cmd_pload[4] = cmd_pload[5] = 0x00;
		cmd_pload[6] = cmd_pload[8] = cmd_pload[9] = cmd_pload[10] = 0x00;
		cmd_pload[2] = 0x01; /* SC_TIME_EN */
		cmd_pload[7] = 0x03; /* SC_UPDATE_RATE(3) */
	}

	ss_send_cmd(vdd, TX_SELF_TIME_SET);

	LCD_ERR("--\n");

	return 0;
}

static void self_move_set(struct samsung_display_driver_data *vdd, int ctrl)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ Control(%s), Interval(%d)\n", ctrl == SELF_MOVE_ON ? "SELF_MOVE_ON" :
						ctrl == SELF_MOVE_RESET ? "SELF_MOVE_RESET" :
						ctrl == SELF_MOVE_OFF ? "SELF_MOVE_OFF" : "Unknown Self Move"
						, vdd->self_disp.st_info.interval);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	switch (ctrl) {
	case SELF_MOVE_ON:
		switch (vdd->self_disp.st_info.interval) {
		case INTERVAL_100:
		case INTERVAL_200:
		case INTERVAL_500:
		case INTERVAL_1000:
			ss_send_cmd(vdd, TX_SELF_MOVE_ON);
			break;
		case INTERVAL_DEBUG:
			ss_send_cmd(vdd, TX_SELF_MOVE_ON_DEBUG);
			break;
		default:
			ss_send_cmd(vdd, TX_SELF_MOVE_ON);
			LCD_ERR("Invalid Time Interval (%d)\n", vdd->self_disp.st_info.interval);
		}
		break;
	case SELF_MOVE_RESET:
		ss_send_cmd(vdd, TX_SELF_MOVE_RESET);
		break;
	case SELF_MOVE_OFF:
		ss_send_cmd(vdd, TX_SELF_MOVE_OFF);
		break;
	default:
		LCD_ERR("Invalid Self Move Control (%d)\n", ctrl);
	}

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_ERR("-- \n");

	return;
}

static void self_icon_img_write(struct samsung_display_driver_data *vdd)
{
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	LCD_ERR("++\n");

	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ICON_SET_PRE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ICON_IMAGE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ICON_SET_POST, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 1);

	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_SELF_ICON_SET_PRE);
	ss_send_cmd(vdd, TX_SELF_ICON_IMAGE);
	ss_send_cmd(vdd, TX_SELF_ICON_SET_POST);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ICON_SET_PRE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ICON_IMAGE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ICON_SET_POST, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 0);

	vdd->exclusive_tx.enable = 0;
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);

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

	LCD_INFO("Self Icon Enable(%d), x(%d), y(%d), w(%d), h(%d), Color(0x%x)\n",
		si_info.en, si_info.pos_x, si_info.pos_y,
		si_info.width, si_info.height, si_info.color);

	pcmds = ss_get_cmds(vdd, TX_SELF_ICON_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_SELF_ICON_ON..\n");
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

		cmd_pload[11] = (si_info.color & 0xFF000000) >> 24; /* A */
		cmd_pload[12] = (si_info.color & 0xFF0000) >> 16; /* R */
		cmd_pload[13] = (si_info.color & 0xFF00) >> 8; /* G */
		cmd_pload[14] = (si_info.color & 0xFF); /* B */
	} else {
		cmd_pload[2] &= ~(BIT(0)); /* SI_ICON_EN */
	}

	ss_send_cmd(vdd, TX_SELF_ICON_ON);

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

	if (enable) {
		ss_send_cmd(vdd, TX_SELF_ACLOCK_ON);
		vdd->self_disp.time_set = true;
	} else {
		ss_send_cmd(vdd, TX_SELF_ACLOCK_OFF);
		vdd->self_disp.time_set = false;
	}

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_ERR("-- \n");

	return;
}

static void self_aclock_img_write(struct samsung_display_driver_data *vdd)
{
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	LCD_ERR("++\n");

	/* Memory BW Boost Enable to prevent DMA Timeout */
	ss_set_max_mem_bw(vdd, true);

	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ACLOCK_SET_PRE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ACLOCK_IMAGE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ACLOCK_SET_POST, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 1);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_SELF_ACLOCK_SET_PRE);
	ss_send_cmd(vdd, TX_SELF_ACLOCK_IMAGE);

	usleep_range(100, 110); /* needs 100us delay between 0x5C & 0x75 */

	ss_send_cmd(vdd, TX_SELF_ACLOCK_SET_POST);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);
	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ACLOCK_SET_PRE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ACLOCK_IMAGE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_ACLOCK_SET_POST, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 0);

	vdd->exclusive_tx.enable = 0;
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);

	/* Memory BW Boost Disable */
	ss_set_max_mem_bw(vdd, false);

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

	LCD_INFO("Self Analog Clock Enable(%d), x(%d), y(%d), rot(%d), mem_mask_en(%d), mem_reuse_en(%d)\n",
		sa_info.en, sa_info.pos_x, sa_info.pos_y, sa_info.rotate, sa_info.mem_mask_en, sa_info.mem_reuse_en);

	if (!sa_info.en)
		goto skip_update;

	pcmds = ss_get_cmds(vdd, TX_SELF_ACLOCK_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_SELF_ACLOCK_ON..\n");
		return -ENODEV;
	}

	cmd_pload = pcmds->cmds[2].msg.tx_buf;

	/* Self Analog Clock Position Update */
	cmd_pload[3] = (sa_info.pos_x & 0x700) >> 8;
	cmd_pload[4] = sa_info.pos_x & 0xFF;
	cmd_pload[5] = (sa_info.pos_y & 0xF00) >> 8;
	cmd_pload[6] = sa_info.pos_y & 0xFF;

	/* Self Analog Clock Rotation Setting */
	cmd_pload[7] &= ~(BIT(0)); /* BIT_0 Clear */
	cmd_pload[7] &= ~(BIT(1)); /* BIT_1 Clear */

	switch (sa_info.rotate) {
	case ROTATE_0:
		cmd_pload[7] &= ~(BIT(0));
		cmd_pload[7] &= ~(BIT(1));
		break;
	case ROTATE_90:
		cmd_pload[7] |= BIT(0);
		cmd_pload[7] &= ~(BIT(1));
		break;
	case ROTATE_180:
		cmd_pload[7] &= ~(BIT(0));
		cmd_pload[7] |= BIT(1);
		break;
	case ROTATE_270:
		cmd_pload[7] |= BIT(0);
		cmd_pload[7] |= BIT(1);
		break;
	default:
		LCD_ERR("Invalid Rotation Setting, (%d)\n", sa_info.rotate);
	}
#if 0 /* No need to update this just use that from dtsi */
	/* Clock Memory Mask for Power Saving */
	if (sa_info.mem_mask_en) {
		cmd_pload[15] = AC_HH_MASK_ST_X & 0xFF;
		cmd_pload[16] = AC_HH_MASK_ST_Y & 0x3F;
		cmd_pload[17] = AC_HH_MASK_ED_X & 0xFF;
		cmd_pload[18] = AC_HH_MASK_ED_Y & 0x3F;
		cmd_pload[19] = AC_MM_MASK_ST_X & 0xFF;
		cmd_pload[20] = AC_MM_MASK_ST_Y & 0x3F;
		cmd_pload[21] = AC_MM_MASK_ED_X & 0xFF;
		cmd_pload[22] = AC_MM_MASK_ED_Y & 0x3F;
		cmd_pload[23] = AC_SS_MASK_ST_X & 0xFF;
		cmd_pload[24] = AC_SS_MASK_ST_Y & 0x3F;
		cmd_pload[25] = AC_SS_MASK_ED_X & 0xFF;
		cmd_pload[26] = AC_SS_MASK_ED_Y & 0x3F;
	} else {
		/* Memory Mask does not have enable register, All the value should be zero incase of disable */
		cmd_pload[15] = cmd_pload[16] = cmd_pload[17] = cmd_pload[18] = cmd_pload[19] = cmd_pload[20] = 0x00;
		cmd_pload[21] = cmd_pload[22] = cmd_pload[23] = cmd_pload[24] = cmd_pload[25] = cmd_pload[26] = 0x00;
	}

	/* Clock Memory Reuse for Power Saving */
	if (sa_info.pos_x >= (AC_HH_MEM_REUSE_W/2))
		mem_reuse_x = sa_info.pos_x - (AC_HH_MEM_REUSE_W/2);
	else
		mem_reuse_x = 0;

	if (sa_info.pos_y  >= (AC_HH_MEM_REUSE_H/2))
		mem_reuse_y = sa_info.pos_y - (AC_HH_MEM_REUSE_H/2);
	else
		mem_reuse_y = 0;

	if (sa_info.mem_reuse_en) {
		cmd_pload[2] |= BIT(4); /* SC_MEM_DISP_ON */
		cmd_pload[27] = (mem_reuse_x & 0x700) >> 8;
		cmd_pload[28] = mem_reuse_x & 0xFF;
		cmd_pload[29] = (mem_reuse_y & 0xF00) >> 8;
		cmd_pload[30] = mem_reuse_y & 0xFF;
		cmd_pload[31] = (AC_HH_MEM_REUSE_W & 0x700) >> 8;
		cmd_pload[32] = AC_HH_MEM_REUSE_W & 0xFF;
		cmd_pload[33] = (AC_HH_MEM_REUSE_H & 0xF00) >> 8;
		cmd_pload[34] = AC_HH_MEM_REUSE_H & 0xFF;
	} else {
		cmd_pload[2] &= ~(BIT(4)); /* SC_MEM_DISP_ON */
	}
#endif
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
		ss_send_cmd(vdd, TX_SELF_DCLOCK_OFF);

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_ERR("-- \n");

	return;
}

static void self_dclock_img_write(struct samsung_display_driver_data *vdd)
{
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	LCD_ERR("++\n");

	/* Memory BW Boost Enable to prevent DMA Timeout */
	ss_set_max_mem_bw(vdd, true);

	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_DCLOCK_SET_PRE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_DCLOCK_IMAGE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_DCLOCK_SET_POST, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 1);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_SELF_DCLOCK_SET_PRE);
	ss_send_cmd(vdd, TX_SELF_DCLOCK_IMAGE);

	usleep_range(100, 110); /* needs 100us delay between 0x5C & 0x75 */

	ss_send_cmd(vdd, TX_SELF_DCLOCK_SET_POST);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);
	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_DCLOCK_SET_PRE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_DCLOCK_IMAGE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_DCLOCK_SET_POST, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 0);

	vdd->exclusive_tx.enable = 0;
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);

	/* Memory BW Boost Disable */
	ss_set_max_mem_bw(vdd, false);

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

	LCD_INFO("Self Digital Clock Enable(%d), 24H(%d), EN_HH(%d), EN_MM(%d), POS_1(%d,%d), POS_2(%d,%d), POS_3(%d,%d), POS_4(%d,%d), W(%d), H(%d), Color(0x%x), UNI_ATTR(%d), UNI_W(%d)\n",
				sd_info.en, vdd->self_disp.st_info.disp_24h,
				sd_info.en_hh, sd_info.en_mm,
				sd_info.pos1_x, sd_info.pos1_y,
				sd_info.pos2_x, sd_info.pos2_y,
				sd_info.pos3_x, sd_info.pos3_y,
				sd_info.pos4_x, sd_info.pos4_y,
				sd_info.img_width, sd_info.img_height,
				sd_info.color, sd_info.unicode_attr, sd_info.unicode_width);

	if (!sd_info.en)
		goto skip_update;

	pcmds = ss_get_cmds(vdd, TX_SELF_DCLOCK_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_SELF_DCLOCK_ON..\n");
		return -ENODEV;
	}
	cmd_pload = pcmds->cmds[2].msg.tx_buf;

	if (vdd->self_disp.st_info.disp_24h) {
		cmd_pload[4] |= (vdd->self_disp.st_info.disp_24h << 4) & 0x30;
	} else {
		cmd_pload[4] &= ~(BIT(4));
		cmd_pload[4] &= ~(BIT(5));
	}

	if (sd_info.en_hh) {
		/* Clear EN_HH First */
		cmd_pload[4] &= ~(BIT(2));
		cmd_pload[4] &= ~(BIT(3));
		cmd_pload[4] |= (sd_info.en_hh & 0x3) << 2;
	} else {
		cmd_pload[4] &= ~(BIT(2));
		cmd_pload[4] &= ~(BIT(3));
	}

	if (sd_info.en_mm) {
		/* Clear EN_MM First */
		cmd_pload[4] &= ~(BIT(0));
		cmd_pload[4] &= ~(BIT(1));
		cmd_pload[4] |= (sd_info.en_mm & 0x3);
	} else {
		cmd_pload[4] &= ~(BIT(0));
		cmd_pload[4] &= ~(BIT(1));
	}

	cmd_pload[5] = (sd_info.pos1_x & 0x700) >> 8;
	cmd_pload[6] = sd_info.pos1_x & 0xFF;
	cmd_pload[7] = (sd_info.pos1_y & 0xF00) >> 8;
	cmd_pload[8] = sd_info.pos1_y & 0xFF;

	cmd_pload[9] = (sd_info.pos2_x & 0x700) >> 8;
	cmd_pload[10] = sd_info.pos2_x & 0xFF;
	cmd_pload[11] = (sd_info.pos2_y & 0xF00) >> 8;
	cmd_pload[12] = sd_info.pos2_y & 0xFF;

	cmd_pload[13] = (sd_info.pos3_x & 0x700) >> 8;
	cmd_pload[14] = sd_info.pos3_x & 0xFF;
	cmd_pload[15] = (sd_info.pos3_y & 0xF00) >> 8;
	cmd_pload[16] = sd_info.pos3_y & 0xFF;

	cmd_pload[17] = (sd_info.pos4_x & 0x700) >> 8;
	cmd_pload[18] = sd_info.pos4_x & 0xFF;
	cmd_pload[19] = (sd_info.pos4_y & 0xF00) >> 8;
	cmd_pload[20] = sd_info.pos4_y & 0xFF;

	cmd_pload[21] = (sd_info.img_width & 0x700) >> 8;
	cmd_pload[22] = sd_info.img_width & 0xFF;
	cmd_pload[23] = (sd_info.img_height & 0xF00) >> 8;
	cmd_pload[24] = sd_info.img_height & 0xFF;

	/* Color */
	cmd_pload[25] = (sd_info.color & 0xFF000000) >> 24; /* A */
	cmd_pload[26] = (sd_info.color & 0xFF0000) >> 16; /* R */
	cmd_pload[27] = (sd_info.color & 0xFF00) >> 8; /* G */
	cmd_pload[28] = (sd_info.color & 0xFF); /* B */

	/* Unicode */
	cmd_pload[3] = sd_info.unicode_attr & 0xFF;

	switch (sd_info.unicode_attr) {
	case 0x02:
	default:
		cmd_pload[31] = 0;
		cmd_pload[32] = 0;
		cmd_pload[33] = 0;
		cmd_pload[34] = 0;
		cmd_pload[35] = 0;
	}

	cmd_pload[29] = ((sd_info.unicode_width) & 0x700) >> 8;
	cmd_pload[30] = sd_info.unicode_width & 0xFF;

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
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	if (!vdd->self_disp.is_support) {
		LCD_ERR("self display is not supported..(%d) \n",
						vdd->self_disp.is_support);
		return;
	}

	LCD_ERR("++\n");

	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_SET_PRE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_IMAGE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_SET_POST, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 1);

	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_SELF_MASK_SET_PRE);
	ss_send_cmd(vdd, TX_SELF_MASK_IMAGE);

	usleep_range(100, 110); /* needs 100us delay between 0x5C& 0x75 */

	ss_send_cmd(vdd, TX_SELF_MASK_SET_POST);
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

static int self_mask_check(struct samsung_display_driver_data *vdd)
{
	int i, ret = 1;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return 0;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR("self display is not supported..(%d) \n",
						vdd->self_disp.is_support);
		return 0;
	}

	if (!vdd->self_disp.mask_crc_size) {
		LCD_ERR("mask crc size is zero..\n\n");
		return 0;
	}

	if (!vdd->self_disp.mask_crc_read_data) {
		vdd->self_disp.mask_crc_read_data = kzalloc(vdd->self_disp.mask_crc_size, GFP_KERNEL);
		if (!vdd->self_disp.mask_crc_read_data) {
			LCD_ERR("fail to alloc for mask_crc_read_data \n");
			return 0;
		}
	}

	LCD_ERR("++ \n");

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	ss_send_cmd(vdd, TX_SELF_MASK_CHECK_PRE1);

	/* Do not permit image update (2C, 3C) during sending self mask image (4C, 5C) */
	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_IMAGE_CRC, 1);

	/* self mask data write (4C, 5C) */
	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_SELF_MASK_IMAGE_CRC);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);

	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_SELF_MASK_IMAGE_CRC, 0);
	vdd->exclusive_tx.enable = 0;
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);

	ss_send_cmd(vdd, TX_SELF_MASK_CHECK_PRE2);

	ss_panel_data_read(vdd, RX_SELF_MASK_CHECK, vdd->self_disp.mask_crc_read_data, LEVEL1_KEY | LEVEL2_KEY);

	ss_send_cmd(vdd, TX_SELF_MASK_CHECK_POST);

	for (i = 0; i < vdd->self_disp.mask_crc_size; i++) {
		if (vdd->self_disp.mask_crc_read_data[i] != vdd->self_disp.mask_crc_pass_data[i]) {
			LCD_ERR("self mask check fail !!\n");
			ret = 0;
			break;
		}
	}

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_ERR("-- \n");

	return ret;
}

static int self_partial_hlpm_scan_set(struct samsung_display_driver_data *vdd)
{
	u8 *cmd_pload;
	struct self_partial_hlpm_scan sphs_info;
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	LCD_ERR("++\n");

	sphs_info = vdd->self_disp.sphs_info;

	LCD_INFO("Self Partial HLPM/Scan hlpm_En(%d), hlpm_mode_sel(0x%x), hlpm a1(%d)a2(%d)a3(%d)a4(%d) / scan_en(%d), scan_line(%d_%d)\n",
			sphs_info.hlpm_en, sphs_info.hlpm_mode_sel,
			sphs_info.hlpm_area_1, sphs_info.hlpm_area_2,
			sphs_info.hlpm_area_3, sphs_info.hlpm_area_4,
			sphs_info.scan_en, sphs_info.scan_sl, sphs_info.scan_el);

	pcmds = ss_get_cmds(vdd, TX_SELF_PARTIAL_HLPM_SCAN_SET);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_SELF_PARTIAL_HLPM_SCAN_SET..\n");
		return -ENODEV;
	}
	cmd_pload = pcmds->cmds[1].msg.tx_buf;

	/* Partial HLPM */
	if (sphs_info.hlpm_en) {
		cmd_pload[1] |= BIT(0); /* SP_PTL_EN */
		cmd_pload[2] = sphs_info.hlpm_mode_sel & 0x1F; /* SP_PTL_MODE_SEL */

		/* Area_1 */
		cmd_pload[9] = (sphs_info.hlpm_area_1 & 0xF00) >> 8;
		cmd_pload[10] = sphs_info.hlpm_area_1 & 0xFF;
		/* Area_2 */
		cmd_pload[11] = (sphs_info.hlpm_area_2 & 0xF00) >> 8;
		cmd_pload[12] = sphs_info.hlpm_area_2 & 0xFF;
		/* Area_3 */
		cmd_pload[13] = (sphs_info.hlpm_area_3 & 0xF00) >> 8;
		cmd_pload[14] = sphs_info.hlpm_area_3 & 0xFF;
		/* Area_4 */
		cmd_pload[15] = (sphs_info.hlpm_area_4 & 0xF00) >> 8;
		cmd_pload[16] = sphs_info.hlpm_area_4 & 0xFF;
	} else {
		cmd_pload[1] &= ~(BIT(0)); /* SP_PTL_EN */
	}

	/* Partial Scan */
	if (sphs_info.scan_en) {
		cmd_pload[1] |= BIT(4); /* SP_PTLSCAN_EN */

		cmd_pload[5] = (sphs_info.scan_sl & 0xF00) >> 8;
		cmd_pload[6] = sphs_info.scan_sl & 0xFF;
		cmd_pload[7] = (sphs_info.scan_el & 0xF00) >> 8;
		cmd_pload[8] = sphs_info.scan_el & 0xFF;
	} else {
		cmd_pload[1] &= ~(BIT(4)); /* SP_PTLSCAN_EN */
	}

	ss_send_cmd(vdd, TX_SELF_PARTIAL_HLPM_SCAN_SET);

	LCD_ERR("--\n");

	return 0;
}

static int self_display_debug(struct samsung_display_driver_data *vdd)
{
	char buf[64];

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

		LCD_INFO("SI_X_O(%u) SI_Y_O(%u) MEM_SUM_O(0x%X) SM_SUM_O(0x%X)\n",
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
		if (vdd->self_disp.operation[FLAG_SELF_ICON].select) {
			LCD_INFO("SELF_ICON CAL CheckSum = 0x%X\n", vdd->self_disp.operation[FLAG_SELF_ICON].img_checksum_cal);
			self_icon_img_write(vdd);
		}

		if (vdd->self_disp.operation[FLAG_SELF_ACLK].select) {
			LCD_INFO("SELF_ACLK CAL CheckSum = 0x%X\n", vdd->self_disp.operation[FLAG_SELF_ACLK].img_checksum_cal);
			self_aclock_img_write(vdd);
		}

		if (vdd->self_disp.operation[FLAG_SELF_DCLK].select) {
			LCD_INFO("SELF_DCLK CAL CheckSum = 0x%X\n", vdd->self_disp.operation[FLAG_SELF_DCLK].img_checksum_cal);
			self_dclock_img_write(vdd);
		}

		/* Self display on */
		ss_send_cmd(vdd, TX_SELF_DISP_ON);

		self_mask_on(vdd, false);
		self_display_debug(vdd);
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

	mutex_lock(&vdd->self_disp.vdd_self_display_ioctl_lock);

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
		self_move_set(vdd, SELF_MOVE_ON);
		break;
	case IOCTL_SELF_MOVE_OFF:
		self_move_set(vdd, SELF_MOVE_OFF);
		break;
	case IOCTL_SELF_MOVE_RESET:
		self_move_set(vdd, SELF_MOVE_RESET);
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
	case IOCTL_SET_PARTIAL_HLPM_SCAN:
		ret = copy_from_user(&vdd->self_disp.sphs_info, argp,
					sizeof(vdd->self_disp.sphs_info));
		if (ret) {
			LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
			goto error;
		}

		ret = self_partial_hlpm_scan_set(vdd);
		break;
	default:
		LCD_ERR("invalid cmd : %u \n", cmd);
		break;
	}
error:
	mutex_unlock(&vdd->self_disp.vdd_self_display_ioctl_lock);
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
		make_mass_self_display_img_cmds_HAB(vdd, TX_SELF_MASK_IMAGE, op);
		vdd->self_disp.operation[op].select = true;
		break;
	case FLAG_SELF_ICON:
		make_mass_self_display_img_cmds_HAB(vdd, TX_SELF_ICON_IMAGE, op);
		vdd->self_disp.operation[op].select = true;
		break;
	case FLAG_SELF_GRID:
		// GRID has no image data..
		break;
	case FLAG_SELF_ACLK:
		make_mass_self_display_img_cmds_HAB(vdd, TX_SELF_ACLOCK_IMAGE, op);
		vdd->self_disp.operation[op].select = true;
		vdd->self_disp.operation[FLAG_SELF_DCLK].select = false;
		break;
	case FLAG_SELF_DCLK:
		make_mass_self_display_img_cmds_HAB(vdd, TX_SELF_DCLOCK_IMAGE, op);
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
int self_display_init_HAB(struct samsung_display_driver_data *vdd)
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
	mutex_init(&vdd->self_disp.vdd_self_display_ioctl_lock);

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
	vdd->self_disp.self_mask_on = self_mask_on;
	vdd->self_disp.self_mask_check = self_mask_check;
	vdd->self_disp.self_move_set = self_move_set;
	vdd->self_disp.self_icon_set = self_icon_set;
	vdd->self_disp.self_aclock_set = self_aclock_set;
	vdd->self_disp.self_dclock_set = self_dclock_set;
	vdd->self_disp.self_time_set = self_time_set;
	vdd->self_disp.self_partial_hlpm_scan_set = self_partial_hlpm_scan_set;
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

