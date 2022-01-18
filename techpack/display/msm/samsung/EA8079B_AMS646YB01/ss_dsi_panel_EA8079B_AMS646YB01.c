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
#include "ss_dsi_panel_EA8079B_AMS646YB01.h"
#include "ss_dsi_mdnie_EA8079B_AMS646YB01.h"

/* AOD Mode status on AOD Service */

enum {
	HLPM_CTRL_2NIT,
	HLPM_CTRL_10NIT,
	HLPM_CTRL_30NIT,
	HLPM_CTRL_60NIT,
	MAX_LPM_CTRL,
};

#define ALPM_REG	0x53	/* Register to control brightness level */
#define ALPM_CTRL_REG	0xBB	/* Register to cnotrol ALPM/HLPM mode */

#define IRC_MODERATO_MODE_VAL	0x61
#define IRC_FLAT_GAMMA_MODE_VAL	0x21

static int ss_gm2_gamma_comp_init(struct samsung_display_driver_data *vdd)
{
	struct vrr_info *vrr = &vdd->vrr;
	struct dsi_panel_cmd_set *rx_cmds;
	u8 readbuf[GAMMA_SET_SIZE];
	int i_mode = 0;
	int k = 0;
	int m = 0;

	rx_cmds = ss_get_cmds(vdd, RX_SMART_DIM_MTP);
	if (SS_IS_CMDS_NULL(rx_cmds)) {
		LCD_ERR("No cmds for RX_SMART_DIM_MTP..\n");
		return -ENODEV;
	}

	//===========================================================================
	// Read original gamma values for gammam modes (SET_1 ~ SET_6) : 60 HS
	//===========================================================================
	rx_cmds->cmds->msg.tx_buf[0] = 0xB7;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 43;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 129;	// 0x81
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);
	memcpy(&G_offset_60_MTP_READ[GAMMA_SET_1][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB7;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 43;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 172;	// 0xAC
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);
	memcpy(&G_offset_60_MTP_READ[GAMMA_SET_2][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB7;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 28;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 215;	// 0xD7
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB8;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 15;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 0x0;	// 0x00
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[28], LEVEL1_KEY);
	memcpy(&G_offset_60_MTP_READ[GAMMA_SET_3][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB8;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 43;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 15;	// 0x0F
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);
	memcpy(&G_offset_60_MTP_READ[GAMMA_SET_4][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB8;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 43;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 58;	// 0x3A
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);
	memcpy(&G_offset_60_MTP_READ[GAMMA_SET_5][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB8;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 43;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 101;	// 0x65
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);
	memcpy(&G_offset_60_MTP_READ[GAMMA_SET_6][0], readbuf, GAMMA_SET_SIZE);

	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++)
		for (k = 0; k < GAMMA_SET_SIZE; k++)
			LCD_DEBUG("G_offset_60_MTP_READ [SET_%d][%d] = %02x\n", i_mode+1, k, G_offset_60_MTP_READ[i_mode][k]);

	//===========================================================================
	// Read original gamma values for gammam modes (SET_1 ~ SET_6) : 120 HS
	//===========================================================================
	//SET_1:[B8:0xE6(230)] + [B9 : 0 ~ 0x29(41)]			//   0 ~  42
	//SET_2:[B9:0x2A(42) ] ~ [B9 : 0x54(84) ]			//  43 ~  85
	//SET_3:[B9:0x55(85) ] ~ [B9 : 0x7F(127)]			//  86 ~ 128
	//SET_4:[B9:0x80(128)] ~ [B9 : 0xAA(170)]			// 129 ~ 128
	//SET_5:[B9:0xAB(171)] ~ [B9 : 0xD5(213)]			// 172 ~ 214
	//SET_6:[B9:0xD6(214)~0xEC(236)](23)+[BA:0x0~0x13(19)](20)	// 215 ~ 257
	//===========================================================================
	rx_cmds->cmds->msg.tx_buf[0] = 0xB8;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 1;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 230; // 0xE6
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB9;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 42;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 0x0;	// 0x0
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[1], LEVEL1_KEY);
	memcpy(&G_offset_120_MTP_READ[GAMMA_SET_1][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB9;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 43;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 42;	// 0x2A
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);
	memcpy(&G_offset_120_MTP_READ[GAMMA_SET_2][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB9;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 43;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 85;	// 0x55
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);
	memcpy(&G_offset_120_MTP_READ[GAMMA_SET_3][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB9;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 43;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 128;	// 0x80
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);
	memcpy(&G_offset_120_MTP_READ[GAMMA_SET_4][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB9;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 43;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 171; // 0xAB
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);
	memcpy(&G_offset_120_MTP_READ[GAMMA_SET_5][0], readbuf, GAMMA_SET_SIZE);

	rx_cmds->cmds->msg.tx_buf[0] = 0xB9;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 23;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 214;
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[0], LEVEL1_KEY);

	rx_cmds->cmds->msg.tx_buf[0] = 0xBA;
	rx_cmds->cmds->msg.tx_buf[1] = rx_cmds->cmds[0].msg.rx_len = 20;
	rx_cmds->cmds->msg.tx_buf[2] = rx_cmds->read_startoffset = 0x0;
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, &readbuf[23], LEVEL1_KEY);
	memcpy(&G_offset_120_MTP_READ[GAMMA_SET_6][0], readbuf, GAMMA_SET_SIZE);

	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++)
		for (k = 0; k < GAMMA_SET_SIZE; k++)
			LCD_DEBUG("G_offset_120_MTP_READ [SET_%d][%d] = %02x\n", i_mode+1, k, G_offset_120_MTP_READ[i_mode][k]);

	// ===============================================
	// 120HS MTP_READ => MTP_READ_10Bit (MTP_READ_33)
	// ===============================================
	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++) {
		m = 0;
		for (k = 0; k < 40; k++) {
			k = k+3; // 3, 7, 11, ...42
			m = m+3; // 3, 6, 9, ...32
			G_offset_120_MTP_READ_10[i_mode][m]   = (GET_BITS(G_offset_120_MTP_READ[i_mode][k],   0, 5)<<4) | (GET_BITS(G_offset_120_MTP_READ[i_mode][k+1], 4, 7));
			G_offset_120_MTP_READ_10[i_mode][m+1] = (GET_BITS(G_offset_120_MTP_READ[i_mode][k+1], 0, 3)<<6) | (GET_BITS(G_offset_120_MTP_READ[i_mode][k+2], 2, 7));
			G_offset_120_MTP_READ_10[i_mode][m+2] = (GET_BITS(G_offset_120_MTP_READ[i_mode][k+2], 0, 1)<<8) | (GET_BITS(G_offset_120_MTP_READ[i_mode][k+3], 0, 7));
		}
	}

	// ===============================================
	// BRIGHT_STEP1 : Platform Brightness (0 ~ 63)
	// ===============================================
	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++)
		for (k = 0; k < GAMMA_V_COMP_SIZE; k++)
			G_offset_Bright1_10Bit[i_mode][k] = G_offset_120_MTP_READ_10[i_mode][k] + Bright1_Comp[i_mode][k];

	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++) {
		m = 0;
		for (k = 0; k < 40; k++) {
			k = k+3; // 3, 7, 11, ...42
			m = m+3; // 3, 6, 9, ...32
			G_offset_Bright1_8Bit[i_mode][k]   = (GET_BITS(G_offset_Bright1_10Bit[i_mode][m],   4, 9));
			G_offset_Bright1_8Bit[i_mode][k+1] = (GET_BITS(G_offset_Bright1_10Bit[i_mode][m],   0, 3)<<4) | (GET_BITS(G_offset_Bright1_10Bit[i_mode][m+1], 6, 9));
			G_offset_Bright1_8Bit[i_mode][k+2] = (GET_BITS(G_offset_Bright1_10Bit[i_mode][m+1], 0, 5)<<2) | (GET_BITS(G_offset_Bright1_10Bit[i_mode][m+2], 8, 9));
			G_offset_Bright1_8Bit[i_mode][k+3] = (GET_BITS(G_offset_Bright1_10Bit[i_mode][m+2], 0, 7));
		}
	}
	// ===============================================
	// BRIGHT_STEP2 : Platform Brightness (64 ~ 68)
	// ===============================================
	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++)
		for (k = 0; k < GAMMA_V_COMP_SIZE; k++)
			G_offset_Bright2_10Bit[i_mode][k] = G_offset_120_MTP_READ_10[i_mode][k] + Bright2_Comp[i_mode][k];

	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++) {
		m = 0;
		for (k = 0; k < 40; k++) {
			k = k+3; // 3, 7, 11, ...42
			m = m+3; // 3, 6, 9, ...32
			G_offset_Bright2_8Bit[i_mode][k]   = (GET_BITS(G_offset_Bright2_10Bit[i_mode][m],   4, 9));
			G_offset_Bright2_8Bit[i_mode][k+1] = (GET_BITS(G_offset_Bright2_10Bit[i_mode][m],   0, 3)<<4) | (GET_BITS(G_offset_Bright2_10Bit[i_mode][m+1], 6, 9));
			G_offset_Bright2_8Bit[i_mode][k+2] = (GET_BITS(G_offset_Bright2_10Bit[i_mode][m+1], 0, 5)<<2) | (GET_BITS(G_offset_Bright2_10Bit[i_mode][m+2], 8, 9));
			G_offset_Bright2_8Bit[i_mode][k+3] = (GET_BITS(G_offset_Bright2_10Bit[i_mode][m+2], 0, 7));
		}
	}
	// ===============================================
	// BRIGHT_STEP3 : Platform Brightness (69 ~ 184)
	// ===============================================
	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++)
		for (k = 0; k < GAMMA_V_COMP_SIZE; k++)
			G_offset_Bright3_10Bit[i_mode][k] = G_offset_120_MTP_READ_10[i_mode][k] + Bright3_Comp[i_mode][k];

	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++) {
		m = 0;
		for (k = 0; k < 40; k++) {
			k = k+3; // 3, 7, 11, ...42
			m = m+3; // 3, 6, 9, ...32
			G_offset_Bright3_8Bit[i_mode][k]   = (GET_BITS(G_offset_Bright3_10Bit[i_mode][m],   4, 9));
			G_offset_Bright3_8Bit[i_mode][k+1] = (GET_BITS(G_offset_Bright3_10Bit[i_mode][m],   0, 3)<<4) | (GET_BITS(G_offset_Bright3_10Bit[i_mode][m+1], 6, 9));
			G_offset_Bright3_8Bit[i_mode][k+2] = (GET_BITS(G_offset_Bright3_10Bit[i_mode][m+1], 0, 5)<<2) | (GET_BITS(G_offset_Bright3_10Bit[i_mode][m+2], 8, 9));
			G_offset_Bright3_8Bit[i_mode][k+3] = (GET_BITS(G_offset_Bright3_10Bit[i_mode][m+2], 0, 7));
		}
	}
	// ===============================================
	// BRIGHT_STEP4 : Platform Brightness (185 ~ 255)
	// ===============================================
	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++)
		for (k = 0; k < GAMMA_V_COMP_SIZE; k++)
			G_offset_Bright4_10Bit[i_mode][k] = G_offset_120_MTP_READ_10[i_mode][k] + Bright4_Comp[i_mode][k];

	for (i_mode = GAMMA_SET_1; i_mode < GAMMA_SET_MAX; i_mode++) {
		m = 0;
		for (k = 0; k < 40; k++) {
			k = k+3; // 3, 7, 11, ...42
			m = m+3; // 3, 6, 9, ...32
			G_offset_Bright4_8Bit[i_mode][k]   = (GET_BITS(G_offset_Bright4_10Bit[i_mode][m],   4, 9));
			G_offset_Bright4_8Bit[i_mode][k+1] = (GET_BITS(G_offset_Bright4_10Bit[i_mode][m],   0, 3)<<4) | (GET_BITS(G_offset_Bright4_10Bit[i_mode][m+1], 6, 9));
			G_offset_Bright4_8Bit[i_mode][k+2] = (GET_BITS(G_offset_Bright4_10Bit[i_mode][m+1], 0, 5)<<2) | (GET_BITS(G_offset_Bright4_10Bit[i_mode][m+2], 8, 9));
			G_offset_Bright4_8Bit[i_mode][k+3] = (GET_BITS(G_offset_Bright4_10Bit[i_mode][m+2], 0, 7));
		}
	}

	vrr->gm2_gamma = VRR_GM2_GAMMA_COMPENSATE;
	return 0;
}

struct dsi_panel_cmd_set *ss_brightness_gm2_gamma_comp(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds = NULL;
	int k = 0;

	pcmds = ss_get_cmds(vdd, TX_VRR_GM2_GAMMA_COMP);

	if (vdd->vrr.adjusted_refresh_rate == 96) {
		// ===============================================
		// BRIGHT_STEP1 : Platform Brightness (0 ~ 63)
		// ===============================================
		if (vdd->br_info.common_br.cd_idx < 64) {
			for (k = 0; k < GAMMA_SET_SIZE; k++) {
				pcmds->cmds[1].msg.tx_buf[k+1]				= G_offset_Bright1_8Bit[0][k]; // 0x81 ~ 0xAB
				pcmds->cmds[1].msg.tx_buf[k+1+GAMMA_SET_SIZE]		= G_offset_Bright1_8Bit[1][k]; // 0xAC ~ 0xD6
				if (k < 28)
					pcmds->cmds[1].msg.tx_buf[k+1+(GAMMA_SET_SIZE*2)] = G_offset_Bright1_8Bit[2][k]; // 0xD7 ~ 0xF2 : 28 (0~27)
				else
					pcmds->cmds[2].msg.tx_buf[k-27]			= G_offset_Bright1_8Bit[2][k]; // 0x00 ~ 0x0E : 15 (1~15)
				pcmds->cmds[2].msg.tx_buf[k+16]				= G_offset_Bright1_8Bit[3][k]; // 0x0F ~ 0x39
				pcmds->cmds[2].msg.tx_buf[k+16+GAMMA_SET_SIZE]		= G_offset_Bright1_8Bit[4][k]; // 0x3A ~ 0x64
				pcmds->cmds[2].msg.tx_buf[k+16+(GAMMA_SET_SIZE*2)]	= G_offset_Bright1_8Bit[5][k]; // 0x65 ~ 0x8F
			}
		}
		// ===============================================
		// BRIGHT_STEP2 : Platform Brightness (64 ~ 68)
		// ===============================================
		else if (vdd->br_info.common_br.cd_idx < 69) {
			for (k = 0; k < GAMMA_SET_SIZE; k++) {
				pcmds->cmds[1].msg.tx_buf[k+1]				= G_offset_Bright2_8Bit[0][k];
				pcmds->cmds[1].msg.tx_buf[k+1+GAMMA_SET_SIZE]		= G_offset_Bright2_8Bit[1][k];
				if (k < 28)
					pcmds->cmds[1].msg.tx_buf[k+1+(GAMMA_SET_SIZE*2)] = G_offset_Bright2_8Bit[2][k];
				else
					pcmds->cmds[2].msg.tx_buf[k-27]			= G_offset_Bright2_8Bit[2][k];
				pcmds->cmds[2].msg.tx_buf[k+16]				= G_offset_Bright2_8Bit[3][k];
				pcmds->cmds[2].msg.tx_buf[k+16+GAMMA_SET_SIZE]		= G_offset_Bright2_8Bit[4][k];
				pcmds->cmds[2].msg.tx_buf[k+16+(GAMMA_SET_SIZE*2)]	= G_offset_Bright2_8Bit[5][k];
			}
		}
		// ===============================================
		// BRIGHT_STEP3 : Platform Brightness (69 ~ 184)
		// ===============================================
		else if (vdd->br_info.common_br.cd_idx < 185) {
			for (k = 0; k < GAMMA_SET_SIZE; k++) {
				pcmds->cmds[1].msg.tx_buf[k+1]				= G_offset_Bright3_8Bit[0][k];
				pcmds->cmds[1].msg.tx_buf[k+1+GAMMA_SET_SIZE]		= G_offset_Bright3_8Bit[1][k];
				if (k < 28)
					pcmds->cmds[1].msg.tx_buf[k+1+(GAMMA_SET_SIZE*2)] = G_offset_Bright3_8Bit[2][k];
				else
					pcmds->cmds[2].msg.tx_buf[k-27]			= G_offset_Bright3_8Bit[2][k];
				pcmds->cmds[2].msg.tx_buf[k+16]				= G_offset_Bright3_8Bit[3][k];
				pcmds->cmds[2].msg.tx_buf[k+16+GAMMA_SET_SIZE]		= G_offset_Bright3_8Bit[4][k];
				pcmds->cmds[2].msg.tx_buf[k+16+(GAMMA_SET_SIZE*2)]	= G_offset_Bright3_8Bit[5][k];
			}
		}
		// ===============================================
		// BRIGHT_STEP4 : Platform Brightness (185 ~ 255)
		// ===============================================
		else {
			for (k = 0; k < GAMMA_SET_SIZE; k++) {
				pcmds->cmds[1].msg.tx_buf[k+1]				= G_offset_Bright4_8Bit[0][k];
				pcmds->cmds[1].msg.tx_buf[k+1+GAMMA_SET_SIZE]		= G_offset_Bright4_8Bit[1][k];
				if (k < 28)
					pcmds->cmds[1].msg.tx_buf[k+1+(GAMMA_SET_SIZE*2)] = G_offset_Bright4_8Bit[2][k];
				else
					pcmds->cmds[2].msg.tx_buf[k-27]			= G_offset_Bright4_8Bit[2][k];
				pcmds->cmds[2].msg.tx_buf[k+16]				= G_offset_Bright4_8Bit[3][k];
				pcmds->cmds[2].msg.tx_buf[k+16+GAMMA_SET_SIZE]		= G_offset_Bright4_8Bit[4][k];
				pcmds->cmds[2].msg.tx_buf[k+16+(GAMMA_SET_SIZE*2)]	= G_offset_Bright4_8Bit[5][k];
			}
		}
	}
	// ======================
	// 60 & 120 Hz
	// ======================
	else {
		for (k = 0; k < GAMMA_SET_SIZE; k++) {
			pcmds->cmds[1].msg.tx_buf[k+1]				= G_offset_60_MTP_READ[0][k];
			pcmds->cmds[1].msg.tx_buf[k+1+GAMMA_SET_SIZE]		= G_offset_60_MTP_READ[1][k];
			if (k < 28)
				pcmds->cmds[1].msg.tx_buf[k+1+(GAMMA_SET_SIZE*2)] = G_offset_60_MTP_READ[2][k];
			else
				pcmds->cmds[2].msg.tx_buf[k-27]			= G_offset_60_MTP_READ[2][k];
			pcmds->cmds[2].msg.tx_buf[k+16]				= G_offset_60_MTP_READ[3][k];
			pcmds->cmds[2].msg.tx_buf[k+16+GAMMA_SET_SIZE]		= G_offset_60_MTP_READ[4][k];
			pcmds->cmds[2].msg.tx_buf[k+16+(GAMMA_SET_SIZE*2)]	= G_offset_60_MTP_READ[5][k];
		}
	}

	*level_key = LEVEL1_KEY;
	return pcmds;
}

static struct dsi_panel_cmd_set *__ss_vrr(struct samsung_display_driver_data *vdd,
					int *level_key, bool is_hbm, bool is_hmt)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_panel_cmd_set  *vrr_cmds = ss_get_cmds(vdd, TX_VRR);

	struct vrr_info *vrr = &vdd->vrr;
	enum SS_BRR_MODE brr_mode = vrr->brr_mode;

	int cur_rr;
	bool cur_hs;

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
			LCD_ERR("VRR: unmatched RR mode (%dHz%s / %dHz%s)\n",
					panel->cur_mode->timing.refresh_rate,
					panel->cur_mode->timing.sot_hs_mode ? "HS" : "NM",
					vdd->vrr.adjusted_refresh_rate,
					vdd->vrr.adjusted_sot_hs_mode ? "HS" : "NM");
	}

	cur_rr = vrr->cur_refresh_rate;
	cur_hs = vrr->cur_sot_hs_mode;

	// Freq Setting
	if (vdd->vrr.adjusted_refresh_rate == 120)
		vrr_cmds->cmds[1].msg.tx_buf[1] = 0x10;
	else
		vrr_cmds->cmds[1].msg.tx_buf[1] = 0x00;

	if (vdd->vrr.adjusted_refresh_rate == 96) {
		vrr_cmds->cmds[3].msg.tx_buf[1] = 0xA2;
		vrr_cmds->cmds[3].msg.tx_buf[2] = 0x5C;

		vrr_cmds->cmds[5].msg.tx_buf[1]  = 0x0B;
		vrr_cmds->cmds[5].msg.tx_buf[2]  = 0xCC;
		vrr_cmds->cmds[5].msg.tx_buf[3]  = 0x09;
		vrr_cmds->cmds[5].msg.tx_buf[4]  = 0x2A;
		vrr_cmds->cmds[5].msg.tx_buf[5]  = 0x09;
		vrr_cmds->cmds[5].msg.tx_buf[6]  = 0x02;
		vrr_cmds->cmds[5].msg.tx_buf[7]  = 0x08;
		vrr_cmds->cmds[5].msg.tx_buf[8]  = 0x8E;
		vrr_cmds->cmds[5].msg.tx_buf[9]  = 0x08;
		vrr_cmds->cmds[5].msg.tx_buf[10] = 0x16;
		vrr_cmds->cmds[5].msg.tx_buf[11] = 0x07;
		vrr_cmds->cmds[5].msg.tx_buf[12] = 0x54;
		vrr_cmds->cmds[5].msg.tx_buf[13] = 0x06;
		vrr_cmds->cmds[5].msg.tx_buf[14] = 0x50;
		vrr_cmds->cmds[5].msg.tx_buf[15] = 0x05;
		vrr_cmds->cmds[5].msg.tx_buf[16] = 0x40;
		vrr_cmds->cmds[5].msg.tx_buf[17] = 0x03;
		vrr_cmds->cmds[5].msg.tx_buf[18] = 0xF2;
		vrr_cmds->cmds[5].msg.tx_buf[19] = 0x02;
		vrr_cmds->cmds[5].msg.tx_buf[29] = 0xA2;
		vrr_cmds->cmds[5].msg.tx_buf[21] = 0x00;
		vrr_cmds->cmds[5].msg.tx_buf[22] = 0xFC;
		vrr_cmds->cmds[5].msg.tx_buf[23] = 0x00;
		vrr_cmds->cmds[5].msg.tx_buf[24] = 0x44;

		vrr_cmds->cmds[6].msg.tx_buf[6] = 0x40;
	}
	else { // 60/120 Hz
		vrr_cmds->cmds[3].msg.tx_buf[1] = 0xA9;
		vrr_cmds->cmds[3].msg.tx_buf[2] = 0x70;

		vrr_cmds->cmds[5].msg.tx_buf[1]  = 0x12;
		vrr_cmds->cmds[5].msg.tx_buf[2]  = 0xE0;
		vrr_cmds->cmds[5].msg.tx_buf[3]  = 0x09;
		vrr_cmds->cmds[5].msg.tx_buf[4]  = 0x2E;
		vrr_cmds->cmds[5].msg.tx_buf[5]  = 0x09;
		vrr_cmds->cmds[5].msg.tx_buf[6]  = 0x03;
		vrr_cmds->cmds[5].msg.tx_buf[7]  = 0x08;
		vrr_cmds->cmds[5].msg.tx_buf[8]  = 0x96;
		vrr_cmds->cmds[5].msg.tx_buf[9]  = 0x08;
		vrr_cmds->cmds[5].msg.tx_buf[10] = 0x2C;
		vrr_cmds->cmds[5].msg.tx_buf[11] = 0x07;
		vrr_cmds->cmds[5].msg.tx_buf[12] = 0x52;
		vrr_cmds->cmds[5].msg.tx_buf[13] = 0x06;
		vrr_cmds->cmds[5].msg.tx_buf[14] = 0x42;
		vrr_cmds->cmds[5].msg.tx_buf[15] = 0x05;
		vrr_cmds->cmds[5].msg.tx_buf[16] = 0x2A;
		vrr_cmds->cmds[5].msg.tx_buf[17] = 0x03;
		vrr_cmds->cmds[5].msg.tx_buf[18] = 0xDA;
		vrr_cmds->cmds[5].msg.tx_buf[19] = 0x02;
		vrr_cmds->cmds[5].msg.tx_buf[29] = 0x82;
		vrr_cmds->cmds[5].msg.tx_buf[21] = 0x01;
		vrr_cmds->cmds[5].msg.tx_buf[22] = 0x2C;
		vrr_cmds->cmds[5].msg.tx_buf[23] = 0x00;
		vrr_cmds->cmds[5].msg.tx_buf[24] = 0x14;

		vrr_cmds->cmds[6].msg.tx_buf[6] = 0x20;
	}

	LCD_INFO("VRR: %s, FPS: %dHz%s (cur: %d%s, target: %d%s)\n",
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

static struct dsi_panel_cmd_set *ss_vrr_hbm(struct samsung_display_driver_data *vdd, int *level_key)
{
	bool is_hbm = true;
	bool is_hmt = false;

	return __ss_vrr(vdd, level_key, is_hbm, is_hmt);
}

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	LCD_INFO("+: ndx=%d\n", vdd->ndx);
	ss_panel_attach_set(vdd, true);

	return true;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
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

static bool finger_exit_cnt;

#define get_bit(value, shift, width)	((value >> shift) & (GENMASK(width - 1, 0)))
static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_normal(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
	        LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_NORMAL);
	LCD_INFO("Normal : cd_idx [%d] \n", vdd->br_info.common_br.cd_idx);

	if (vdd->vrr.adjusted_refresh_rate == 60)
		pcmds->cmds[0].msg.tx_buf[1] = 0x00;
	else
		pcmds->cmds[0].msg.tx_buf[1] = 0x10;

	pcmds->cmds[1].msg.tx_buf[1] = (vdd->finger_mask_updated || finger_exit_cnt) ? 0x20 : 0x28;	/* Normal (0x20) / Smooth (0x28) */
	pcmds->cmds[2].msg.tx_buf[6] = 0x16;
	pcmds->cmds[3].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 2);
	pcmds->cmds[3].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

	/* ELVSS TSET */
	pcmds->cmds[5].msg.tx_buf[1] = vdd->br_info.temperature > 0 ? vdd->br_info.temperature : (char)(BIT(7) | (-1*vdd->br_info.temperature));

	if (vdd->finger_mask_updated) {
		finger_exit_cnt = 1;
	} else {
		finger_exit_cnt = 0;
	}

	*level_key = LEVEL1_KEY;
	return pcmds;
}

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_hbm(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(": Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	pcmds = ss_get_cmds(vdd, TX_GAMMA_MODE2_HBM);
	LCD_INFO("HBM : cd_idx [%d]\n", vdd->br_info.common_br.cd_idx);

	if (vdd->vrr.adjusted_refresh_rate == 60)
		pcmds->cmds[0].msg.tx_buf[1] = 0x00;
	else
		pcmds->cmds[0].msg.tx_buf[1] = 0x10;

	pcmds->cmds[3].msg.tx_buf[6] = elvss_table_hbm[vdd->br_info.common_br.cd_idx];	/* ELVSS Value for HBM brgihtness */
	pcmds->cmds[4].msg.tx_buf[1] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 8, 2);
	pcmds->cmds[4].msg.tx_buf[2] = get_bit(vdd->br_info.common_br.gm2_wrdisbv, 0, 8);

	/* ELVSS TSET */
	pcmds->cmds[6].msg.tx_buf[1] = vdd->br_info.temperature > 0 ? vdd->br_info.temperature : (char)(BIT(7) | (-1*vdd->br_info.temperature));

	*level_key = LEVEL1_KEY;
	return pcmds;
}

static struct dsi_panel_cmd_set * ss_brightness_gamma_mode2_hmt(struct samsung_display_driver_data *vdd, int *level_key)
{
 	struct dsi_panel_cmd_set *pcmds = NULL;

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
	char ddi_id[6];
	int loop;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (D1h 96th~101st) for CHIP ID */
	if (ss_get_cmds(vdd, RX_DDI_ID)->count) {
		ss_panel_data_read(vdd, RX_DDI_ID, ddi_id, LEVEL1_KEY);

		for (loop = 0; loop < MAX_CHIP_ID; loop++)
			vdd->ddi_id_dsi[loop] = ddi_id[loop];

		LCD_INFO("DSI%d : %02x %02x %02x %02x %02x %02x\n", vdd->ndx,
			vdd->ddi_id_dsi[0], vdd->ddi_id_dsi[1],
			vdd->ddi_id_dsi[2], vdd->ddi_id_dsi[3],
			vdd->ddi_id_dsi[4], vdd->ddi_id_dsi[5]);
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

		LCD_INFO("DSI%d: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			vdd->ndx,
			vdd->cell_id_dsi[0],	vdd->cell_id_dsi[1],
			vdd->cell_id_dsi[2],	vdd->cell_id_dsi[3],
			vdd->cell_id_dsi[4],	vdd->cell_id_dsi[5],
			vdd->cell_id_dsi[6],	vdd->cell_id_dsi[7],
			vdd->cell_id_dsi[8],	vdd->cell_id_dsi[9],
			vdd->cell_id_dsi[10],	vdd->cell_id_dsi[11],
			vdd->cell_id_dsi[12],	vdd->cell_id_dsi[13],
			vdd->cell_id_dsi[14],	vdd->cell_id_dsi[15]);
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

	/* Read Panel Unique OCTA ID (EAh 15th ~ 34th) */
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

#undef COORDINATE_DATA_SIZE
#define COORDINATE_DATA_SIZE 6

#define F1(x, y) ((y)-((39*(x))/38)-95)
#define F2(x, y) ((y)-((36*(x))/35)-56)
#define F3(x, y) ((y)+((7*(x))/1)-24728)
#define F4(x, y) ((y)+((25*(x))/7)-14031)

/* Normal Mode */
static char coordinate_data_1[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xfd, 0x00, 0xfc, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfe, 0x00, 0xff, 0x00}, /* Tune_2 */
	{0xfc, 0x00, 0xfc, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfe, 0x00, 0xfd, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xfc, 0x00, 0xfd, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xfe, 0x00, 0xff, 0x00, 0xfc, 0x00}, /* Tune_7 */
	{0xfe, 0x00, 0xff, 0x00, 0xfe, 0x00}, /* Tune_8 */
	{0xfc, 0x00, 0xfe, 0x00, 0xff, 0x00}, /* Tune_9 */
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
	{0xfd, 0x00, 0xfa, 0x00, 0xf0, 0x00}, /* Tune_9 */
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
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (B7h 8th,9th) for elvss*/
	ss_panel_data_read(vdd, RX_ELVSS, vdd->br_info.common_br.elvss_value, LEVEL1_KEY);

	return true;
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

static void ss_gray_spot(struct samsung_display_driver_data *vdd, int enable)
{
	char gray_spot_buf[3];
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return;
	}

	if (enable) {
		pcmds = ss_get_cmds(vdd, TX_GRAY_SPOT_TEST_OFF);

		/* Read mtp (B5h 3rd, 4th) */
		ss_panel_data_read(vdd, RX_GRAYSPOT_RESTORE_VALUE, gray_spot_buf, LEVEL1_KEY);
		pcmds->cmds[7].msg.tx_buf[3] = gray_spot_buf[0];
		pcmds->cmds[7].msg.tx_buf[4] = gray_spot_buf[1];
	} else {
		/* Nothing to do */
	}
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
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE_2 = NULL;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE_2 = NULL;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE_2 = NULL;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE_2 = DSI0_CAMERA_AUTO_MDNIE_2;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE_2 = NULL;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE_2 = NULL;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE_2 = DSI0_GALLERY_AUTO_MDNIE_2;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE_2 = NULL;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE_2 = NULL;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE_2 = NULL;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE_2 = NULL;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE_2 = NULL;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE_2 = DSI0_EBOOK_AUTO_MDNIE_2;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE_2 = NULL;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE_2 = NULL;
	mdnie_data->DSI_TDMB_AUTO_MDNIE_2 = NULL;

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
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE = NULL;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE = NULL;
	mdnie_data->DSI_VIDEO_NATURAL_MDNIE = NULL;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE = NULL;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE = DSI0_CAMERA_AUTO_MDNIE;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE = NULL;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE = NULL;
	mdnie_data->DSI_GALLERY_NATURAL_MDNIE = NULL;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE = DSI0_GALLERY_AUTO_MDNIE;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE = NULL;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE = NULL;
	mdnie_data->DSI_BROWSER_NATURAL_MDNIE = NULL;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE = NULL;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE = NULL;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE = NULL;
	mdnie_data->DSI_EBOOK_NATURAL_MDNIE = NULL;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE = DSI0_EBOOK_AUTO_MDNIE;
	mdnie_data->DSI_EMAIL_AUTO_MDNIE = DSI0_EMAIL_AUTO_MDNIE;
	mdnie_data->DSI_GAME_LOW_MDNIE = NULL;
	mdnie_data->DSI_GAME_MID_MDNIE = NULL;
	mdnie_data->DSI_GAME_HIGH_MDNIE = NULL;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE = NULL;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE = NULL;
	mdnie_data->DSI_TDMB_NATURAL_MDNIE = NULL;
	mdnie_data->DSI_TDMB_AUTO_MDNIE = NULL;
	mdnie_data->DSI_GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
	mdnie_data->DSI_GRAYSCALE_NEGATIVE_MDNIE = DSI0_GRAYSCALE_NEGATIVE_MDNIE;
	mdnie_data->DSI_CURTAIN = DSI0_SCREEN_CURTAIN_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR = DSI0_NIGHT_MODE_MDNIE_5;
	mdnie_data->DSI_COLOR_LENS_MDNIE = DSI0_COLOR_LENS_MDNIE;
	mdnie_data->DSI_COLOR_LENS_MDNIE_SCR = DSI0_COLOR_LENS_MDNIE_5;
	mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_5;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR = DSI0_RGB_SENSOR_MDNIE_5;

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
	mdnie_data->dsi_max_night_mode_index = 102;
	mdnie_data->dsi_color_lens_table = color_lens_data;
	mdnie_data->dsi_white_default_r = 0xff;
	mdnie_data->dsi_white_default_g = 0xff;
	mdnie_data->dsi_white_default_b = 0xff;
	mdnie_data->dsi_white_balanced_r = 0;
	mdnie_data->dsi_white_balanced_g = 0;
	mdnie_data->dsi_white_balanced_b = 0;
	mdnie_data->dsi_scr_step_index = MDNIE_STEP2_INDEX;
	mdnie_data->dsi_afc_size = 45;
	mdnie_data->dsi_afc_index = 33;

	vdd->mdnie.mdnie_data = mdnie_data;

	return 0;
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

	pcmds->cmds[0].msg.tx_buf[1] = 0x01;	/* 55h 0x01 ACL 8% */

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

	*level_key = LEVEL1_KEY;

	pcmds = ss_get_cmds(vdd, TX_ACL_ON);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_ACL_ON..\n");
		return NULL;
	}

	if(vdd->br_info.common_br.cd_idx <= MAX_BL_PF_LEVEL)
		pcmds->cmds[0].msg.tx_buf[1] = 0x02;	/* ACL 15% */
	else
		pcmds->cmds[0].msg.tx_buf[1] = 0x01;	/* ACL 8% */

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

	*level_key = LEVEL1_KEY;
	LCD_INFO("off\n");
	return ss_get_cmds(vdd, TX_ACL_OFF);
}

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
		{ALPM_REG, -EINVAL},		// 0x53
		{ALPM_CTRL_REG, -EINVAL}	// 0xBB
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
		{ALPM_REG, -EINVAL},		// 0x53
		{ALPM_CTRL_REG, -EINVAL}	// 0xBB
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

static void ss_copr_panel_init(struct samsung_display_driver_data *vdd)
{
	ss_copr_init(vdd);
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

static int ss_dyn_mipi_pre(struct samsung_display_driver_data *vdd)
{
	int rc = 0;

	ss_send_cmd(vdd, TX_FFC_OFF);
	LCD_INFO("[DISPLAY_%d] tx FFC OFF\n", vdd->ndx);

	return rc;
}

static int ss_dyn_mipi_post(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *ffc_set;
	struct dsi_panel_cmd_set *dyn_ffc_pre_set;
	struct dsi_panel_cmd_set *dyn_ffc_set;
	int idx;
	int rc = 0;

	mutex_lock(&vdd->dyn_mipi_clk.dyn_mipi_lock);
	idx = ss_find_dyn_mipi_clk_timing_idx(vdd);
	mutex_unlock(&vdd->dyn_mipi_clk.dyn_mipi_lock);

	if (idx < 0) {
		LCD_ERR("[EA8079B] Failed to find MIPI clock timing (%d)\n", idx);
		goto err;
	}

	LCD_INFO("[DISPLAY_%d] +++ clk idx: [%d], tx FFC\n", vdd->ndx, idx);
	ffc_set = ss_get_cmds(vdd, TX_FFC);
	dyn_ffc_set = ss_get_cmds(vdd, TX_DYNAMIC_FFC_SET);
	dyn_ffc_pre_set = ss_get_cmds(vdd, TX_DYNAMIC_FFC_PRE_SET);

	if (SS_IS_CMDS_NULL(ffc_set) || SS_IS_CMDS_NULL(dyn_ffc_set) || SS_IS_CMDS_NULL(dyn_ffc_set) ) {
		LCD_ERR("No cmds for TX_FFC..\n");
		return -EINVAL;
	}

	memcpy(ffc_set->cmds[3].msg.tx_buf, dyn_ffc_pre_set->cmds[idx].msg.tx_buf, ffc_set->cmds[3].msg.tx_len);
	memcpy(ffc_set->cmds[4].msg.tx_buf, dyn_ffc_set->cmds[idx].msg.tx_buf, ffc_set->cmds[4].msg.tx_len);

	ss_send_cmd(vdd, TX_FFC);
err:
	LCD_INFO("[DISPLAY_%d] --- clk idx: [%d], tx FFC\n", vdd->ndx, idx);
	return rc;
}

static int ss_vrr_init(struct vrr_info *vrr)
{
	LCD_INFO("EA8079B_AMS646YB01 +++\n");

	mutex_init(&vrr->vrr_lock);
	mutex_init(&vrr->brr_lock);

	vrr->running_vrr_mdp = false;
	vrr->running_vrr = false;

	/* Bootloader: FHD@120hz HS mode */
	vrr->cur_refresh_rate = vrr->adjusted_refresh_rate = 120;
	vrr->cur_sot_hs_mode = vrr->adjusted_sot_hs_mode = true;
	vrr->max_h_active_support_120hs = 1080; /* supports 120hz until FHD 1080 */

	vrr->hs_nm_seq = HS_NM_OFF;
	vrr->delayed_perf_normal = false;
	vrr->skip_vrr_in_brightness = false;

	vrr->brr_mode = BRR_OFF_MODE;
	vrr->brr_rewind_on = false;

	vrr->vrr_workqueue = create_singlethread_workqueue("vrr_workqueue");
	INIT_WORK(&vrr->vrr_work, ss_panel_vrr_switch_work);

	LCD_INFO("EA8079B_AMS646YB01 ---\n");

	return 0;
}

static void samsung_panel_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("EA8079B_AMS646YB01 : +++ \n");
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
	vdd->panel_func.samsung_elvss_read = ss_elvss_read;
	vdd->panel_func.samsung_mdnie_read = ss_mdnie_read;

	/* Brightness */
	vdd->panel_func.samsung_brightness_gamma = ss_brightness_gamma_mode2_normal;
	vdd->panel_func.samsung_brightness_acl_on = ss_acl_on;
	vdd->panel_func.samsung_brightness_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_hbm_off = NULL;
	vdd->panel_func.samsung_brightness_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_elvss = NULL;
	vdd->panel_func.samsung_brightness_vint = NULL;
	vdd->panel_func.samsung_brightness_vrr = ss_vrr;
	vdd->panel_func.samsung_brightness_gm2_gamma_comp = ss_brightness_gm2_gamma_comp;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_brightness_gamma_mode2_hbm;
	vdd->panel_func.samsung_hbm_acl_on = ss_acl_on_hbm;
	vdd->panel_func.samsung_hbm_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_vrr_hbm = ss_vrr_hbm;

	/* Event */
	vdd->panel_func.samsung_change_ldi_fps = NULL;

	/* HMT */
	vdd->panel_func.samsung_brightness_gamma_hmt = NULL;
	vdd->panel_func.samsung_brightness_aid_hmt = NULL;
	vdd->panel_func.samsung_brightness_elvss_hmt = NULL;
	vdd->panel_func.samsung_brightness_vint_hmt = NULL;
	vdd->panel_func.samsung_smart_dimming_hmt_init = NULL;
	vdd->panel_func.samsung_smart_get_conf_hmt = NULL;
	vdd->panel_func.samsung_brightness_hmt = ss_brightness_gamma_mode2_hmt;

	/* Panel LPM */
	vdd->panel_func.samsung_update_lpm_ctrl_cmd = ss_update_panel_lpm_ctrl_cmd;
	vdd->panel_func.samsung_set_lpm_brightness = ss_set_panel_lpm_brightness;

	/* Gray Spot Test */
	vdd->panel_func.samsung_gray_spot = ss_gray_spot;

	/* Dynamic(Adaptive) MIPI Clock */
	vdd->panel_func.samsung_dyn_mipi_pre = ss_dyn_mipi_pre;
	vdd->panel_func.samsung_dyn_mipi_post = ss_dyn_mipi_post;

	/* default brightness */
	vdd->br_info.common_br.bl_level = 25500;

	/* mdnie */
	vdd->mdnie.support_mdnie = true;
	vdd->no_qcom_pps = true;

	vdd->mdnie.support_trans_dimming = false;
	vdd->mdnie.mdnie_tune_size[0] = sizeof(DSI0_BYPASS_MDNIE_1);
	vdd->mdnie.mdnie_tune_size[1] = sizeof(DSI0_BYPASS_MDNIE_2);
	vdd->mdnie.mdnie_tune_size[2] = sizeof(DSI0_BYPASS_MDNIE_3);
	vdd->mdnie.mdnie_tune_size[3] = sizeof(DSI0_BYPASS_MDNIE_4);
	vdd->mdnie.mdnie_tune_size[4] = sizeof(DSI0_BYPASS_MDNIE_5);
	vdd->mdnie.mdnie_tune_size[5] = sizeof(DSI0_BYPASS_MDNIE_6);
	dsi_update_mdnie_data(vdd);

	/* send recovery pck before sending image date (for ESD recovery) */
	vdd->esd_recovery.send_esd_recovery = false;

	/* Enable panic on first pingpong timeout */

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

	/*Default br_info.temperature*/
	vdd->br_info.temperature = 20;

	/* ACL default status in acl on */
	vdd->br_info.gradual_acl_val = 1;

	/* Gram Checksum Test */
	vdd->panel_func.samsung_gct_write = NULL;
	vdd->panel_func.samsung_gct_read = NULL;

	/* SAMSUNG_FINGERPRINT */
	vdd->panel_hbm_entry_delay = 1;
	vdd->panel_hbm_entry_after_te = 0;
	vdd->panel_hbm_exit_delay = 1;

	/* Gamma compensation (Gamma Offset) */
	vdd->panel_func.samsung_gm2_gamma_comp_init = ss_gm2_gamma_comp_init;

	/* VRR */
	ss_vrr_init(&vdd->vrr);

	LCD_INFO("EA8079B_AMS646YB01 : --- \n");
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_EA8079B_AMS646YB01_FHD";
	char panel_name[MAX_CMDLINE_PARAM_LEN];
	char panel_secondary_name[MAX_CMDLINE_PARAM_LEN];

	LCD_INFO("EA8079B_AMS646YB01 : +++ \n");

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
		LCD_INFO("EA8079B_AMS646YB01 : --- \n");
		return 0;
	}

	vdd = ss_get_vdd(ndx);
	vdd->panel_func.samsung_panel_init = samsung_panel_init;

	LCD_INFO("%s done.. \n", panel_name);
	LCD_INFO("EA8079B_AMS646YB01 : --- \n");

	return 0;
}

early_initcall(samsung_panel_initialize);
