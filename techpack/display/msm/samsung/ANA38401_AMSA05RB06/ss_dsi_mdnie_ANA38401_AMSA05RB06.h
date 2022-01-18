/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: wu.deng
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
*/
#ifndef _SS_DSI_MDNIE_ANA38401_AMSA05RB06_H_
#define _SS_DSI_MDNIE_ANA38401_AMSA05RB06_H_

#include "ss_dsi_mdnie_lite_common.h"

#define MDNIE_COLOR_BLINDE_CMD_OFFSET 32

#define ADDRESS_SCR_WHITE_RED   0x32
#define ADDRESS_SCR_WHITE_GREEN 0x34
#define ADDRESS_SCR_WHITE_BLUE  0x36

#define MDNIE_RGB_SENSOR_INDEX	1

#define MDNIE_STEP1_INDEX 1
#define MDNIE_STEP2_INDEX 3

static char mdnie_address_1[] = {
	0xB0,
	0xA8,
};

static char mdnie_address_2[] = {
	0xB0,
	0x42,
};

static char adjust_ldu_data_1[] = {
	0xff, 0xff, 0xff,
	0xf6, 0xfa, 0xff,
	0xf4, 0xf8, 0xff,
	0xe9, 0xf2, 0xff,
	0xe2, 0xef, 0xff,
	0xd4, 0xe8, 0xff,
};

static char adjust_ldu_data_2[] = {
	0xff, 0xfa, 0xf1,
	0xff, 0xfd, 0xf8,
	0xff, 0xfd, 0xfa,
	0xfa, 0xfd, 0xff,
	0xf5, 0xfb, 0xff,
	0xe5, 0xf3, 0xff,
};

static char *adjust_ldu_data[MAX_MODE] = {
	adjust_ldu_data_2,
	adjust_ldu_data_2,
	adjust_ldu_data_2,
	adjust_ldu_data_1,
	adjust_ldu_data_1,
};

static char night_mode_data[] = {
	0x00, 0xff, 0xfa, 0x00, 0xf1, 0x00, 0xff, 0x00, 0x00, 0xfa, 0xf1, 0x00, 0xff, 0x00, 0xfa, 0x00, 0x00, 0xf1, 0xff, 0x00, 0xfa, 0x00, 0xf1, 0x00, /* 6500K */
	0x00, 0xff, 0xf8, 0x00, 0xeb, 0x00, 0xff, 0x00, 0x00, 0xf8, 0xeb, 0x00, 0xff, 0x00, 0xf8, 0x00, 0x00, 0xeb, 0xff, 0x00, 0xf8, 0x00, 0xeb, 0x00, /* 6100K */
	0x00, 0xff, 0xf5, 0x00, 0xe2, 0x00, 0xff, 0x00, 0x00, 0xf5, 0xe2, 0x00, 0xff, 0x00, 0xf5, 0x00, 0x00, 0xe2, 0xff, 0x00, 0xf5, 0x00, 0xe2, 0x00, /* 5700K */
	0x00, 0xff, 0xf2, 0x00, 0xda, 0x00, 0xff, 0x00, 0x00, 0xf2, 0xda, 0x00, 0xff, 0x00, 0xf2, 0x00, 0x00, 0xda, 0xff, 0x00, 0xf2, 0x00, 0xda, 0x00, /* 5300K */
	0x00, 0xff, 0xee, 0x00, 0xd0, 0x00, 0xff, 0x00, 0x00, 0xee, 0xd0, 0x00, 0xff, 0x00, 0xee, 0x00, 0x00, 0xd0, 0xff, 0x00, 0xee, 0x00, 0xd0, 0x00, /* 4900K */
	0x00, 0xff, 0xe9, 0x00, 0xc4, 0x00, 0xff, 0x00, 0x00, 0xe9, 0xc4, 0x00, 0xff, 0x00, 0xe9, 0x00, 0x00, 0xc4, 0xff, 0x00, 0xe9, 0x00, 0xc4, 0x00, /* 4500K */
	0x00, 0xff, 0xe3, 0x00, 0xb4, 0x00, 0xff, 0x00, 0x00, 0xe3, 0xb4, 0x00, 0xff, 0x00, 0xe3, 0x00, 0x00, 0xb4, 0xff, 0x00, 0xe3, 0x00, 0xb4, 0x00, /* 4100K */
	0x00, 0xff, 0xdd, 0x00, 0xa4, 0x00, 0xff, 0x00, 0x00, 0xdd, 0xa4, 0x00, 0xff, 0x00, 0xdd, 0x00, 0x00, 0xa4, 0xff, 0x00, 0xdd, 0x00, 0xa4, 0x00, /* 3700K */
	0x00, 0xff, 0xd6, 0x00, 0x91, 0x00, 0xff, 0x00, 0x00, 0xd6, 0x91, 0x00, 0xff, 0x00, 0xd6, 0x00, 0x00, 0x91, 0xff, 0x00, 0xd6, 0x00, 0x91, 0x00, /* 3300K */
	0x00, 0xff, 0xcb, 0x00, 0x7b, 0x00, 0xff, 0x00, 0x00, 0xcb, 0x7b, 0x00, 0xff, 0x00, 0xcb, 0x00, 0x00, 0x7b, 0xff, 0x00, 0xcb, 0x00, 0x7b, 0x00, /* 2900K */
	0x00, 0xff, 0xbe, 0x00, 0x68, 0x00, 0xff, 0x00, 0x00, 0xbe, 0x68, 0x00, 0xff, 0x00, 0xbe, 0x00, 0x00, 0x68, 0xff, 0x00, 0xbe, 0x00, 0x68, 0x00, /* 2500K */
	0x00, 0xff, 0xfa, 0x00, 0xf1, 0x00, 0xff, 0x00, 0x00, 0xfa, 0xf1, 0x00, 0xff, 0x00, 0xfa, 0x00, 0x00, 0xf1, 0xff, 0x00, 0xfa, 0x00, 0xf1, 0x00, /* 6500K */
	0x00, 0xff, 0xf8, 0x00, 0xeb, 0x00, 0xff, 0x00, 0x00, 0xf8, 0xeb, 0x00, 0xff, 0x00, 0xf8, 0x00, 0x00, 0xeb, 0xff, 0x00, 0xf8, 0x00, 0xeb, 0x00, /* 6100K */
	0x00, 0xff, 0xf5, 0x00, 0xe2, 0x00, 0xff, 0x00, 0x00, 0xf5, 0xe2, 0x00, 0xff, 0x00, 0xf5, 0x00, 0x00, 0xe2, 0xff, 0x00, 0xf5, 0x00, 0xe2, 0x00, /* 5700K */
	0x00, 0xff, 0xf2, 0x00, 0xda, 0x00, 0xff, 0x00, 0x00, 0xf2, 0xda, 0x00, 0xff, 0x00, 0xf2, 0x00, 0x00, 0xda, 0xff, 0x00, 0xf2, 0x00, 0xda, 0x00, /* 5300K */
	0x00, 0xff, 0xee, 0x00, 0xd0, 0x00, 0xff, 0x00, 0x00, 0xee, 0xd0, 0x00, 0xff, 0x00, 0xee, 0x00, 0x00, 0xd0, 0xff, 0x00, 0xee, 0x00, 0xd0, 0x00, /* 4900K */
	0x00, 0xff, 0xe9, 0x00, 0xc4, 0x00, 0xff, 0x00, 0x00, 0xe9, 0xc4, 0x00, 0xff, 0x00, 0xe9, 0x00, 0x00, 0xc4, 0xff, 0x00, 0xe9, 0x00, 0xc4, 0x00, /* 4500K */
	0x00, 0xff, 0xe3, 0x00, 0xb4, 0x00, 0xff, 0x00, 0x00, 0xe3, 0xb4, 0x00, 0xff, 0x00, 0xe3, 0x00, 0x00, 0xb4, 0xff, 0x00, 0xe3, 0x00, 0xb4, 0x00, /* 4100K */
	0x00, 0xff, 0xdd, 0x00, 0xa4, 0x00, 0xff, 0x00, 0x00, 0xdd, 0xa4, 0x00, 0xff, 0x00, 0xdd, 0x00, 0x00, 0xa4, 0xff, 0x00, 0xdd, 0x00, 0xa4, 0x00, /* 3700K */
	0x00, 0xff, 0xd6, 0x00, 0x91, 0x00, 0xff, 0x00, 0x00, 0xd6, 0x91, 0x00, 0xff, 0x00, 0xd6, 0x00, 0x00, 0x91, 0xff, 0x00, 0xd6, 0x00, 0x91, 0x00, /* 3300K */
	0x00, 0xff, 0xcb, 0x00, 0x7b, 0x00, 0xff, 0x00, 0x00, 0xcb, 0x7b, 0x00, 0xff, 0x00, 0xcb, 0x00, 0x00, 0x7b, 0xff, 0x00, 0xcb, 0x00, 0x7b, 0x00, /* 2900K */
	0x00, 0xff, 0xbe, 0x00, 0x68, 0x00, 0xff, 0x00, 0x00, 0xbe, 0x68, 0x00, 0xff, 0x00, 0xbe, 0x00, 0x00, 0x68, 0xff, 0x00, 0xbe, 0x00, 0x68, 0x00, /* 2500K */
};

static char color_lens_data[] = {
	//Blue
	0x00, 0xcc, 0xcc, 0x00, 0xff, 0x33, 0xcc, 0x00, 0x00, 0xcc, 0xff, 0x33, 0xcc, 0x00, 0xcc, 0x00, 0x33, 0xff, 0xcc, 0x00, 0xcc, 0x00, 0xff, 0x33, /* 20% */
	0x00, 0xbf, 0xbf, 0x00, 0xff, 0x40, 0xbf, 0x00, 0x00, 0xbf, 0xff, 0x40, 0xbf, 0x00, 0xbf, 0x00, 0x40, 0xff, 0xbf, 0x00, 0xbf, 0x00, 0xff, 0x40, /* 25% */
	0x00, 0xb2, 0xb2, 0x00, 0xff, 0x4d, 0xb2, 0x00, 0x00, 0xb2, 0xff, 0x4d, 0xb2, 0x00, 0xb2, 0x00, 0x4d, 0xff, 0xb2, 0x00, 0xb2, 0x00, 0xff, 0x4d, /* 30% */
	0x00, 0xa6, 0xa6, 0x00, 0xff, 0x59, 0xa6, 0x00, 0x00, 0xa6, 0xff, 0x59, 0xa6, 0x00, 0xa6, 0x00, 0x59, 0xff, 0xa6, 0x00, 0xa6, 0x00, 0xff, 0x59, /* 35% */
	0x00, 0x99, 0x99, 0x00, 0xff, 0x66, 0x99, 0x00, 0x00, 0x99, 0xff, 0x66, 0x99, 0x00, 0x99, 0x00, 0x66, 0xff, 0x99, 0x00, 0x99, 0x00, 0xff, 0x66, /* 40% */
	0x00, 0x8c, 0x8c, 0x00, 0xff, 0x73, 0x8c, 0x00, 0x00, 0x8c, 0xff, 0x73, 0x8c, 0x00, 0x8c, 0x00, 0x73, 0xff, 0x8c, 0x00, 0x8c, 0x00, 0xff, 0x73, /* 45% */
	0x00, 0x7f, 0x7f, 0x00, 0xff, 0x80, 0x7f, 0x00, 0x00, 0x7f, 0xff, 0x80, 0x7f, 0x00, 0x7f, 0x00, 0x80, 0xff, 0x7f, 0x00, 0x7f, 0x00, 0xff, 0x80, /* 50% */
	0x00, 0x73, 0x73, 0x00, 0xff, 0x8c, 0x73, 0x00, 0x00, 0x73, 0xff, 0x8c, 0x73, 0x00, 0x73, 0x00, 0x8c, 0xff, 0x73, 0x00, 0x73, 0x00, 0xff, 0x8c, /* 55% */
	0x00, 0x66, 0x66, 0x00, 0xff, 0x99, 0x66, 0x00, 0x00, 0x66, 0xff, 0x99, 0x66, 0x00, 0x66, 0x00, 0x99, 0xff, 0x66, 0x00, 0x66, 0x00, 0xff, 0x99, /* 60% */

	//Azure
	0x00, 0xcc, 0xe5, 0x19, 0xff, 0x33, 0xcc, 0x00, 0x19, 0xe5, 0xff, 0x33, 0xcc, 0x00, 0xe5, 0x19, 0x33, 0xff, 0xcc, 0x00, 0xe5, 0x19, 0xff, 0x33, /* 20% */
	0x00, 0xbf, 0xdf, 0x20, 0xff, 0x40, 0xbf, 0x00, 0x20, 0xdf, 0xff, 0x40, 0xbf, 0x00, 0xdf, 0x20, 0x40, 0xff, 0xbf, 0x00, 0xdf, 0x20, 0xff, 0x40, /* 25% */
	0x00, 0xb2, 0xd8, 0x26, 0xff, 0x4d, 0xb2, 0x00, 0x26, 0xd8, 0xff, 0x4d, 0xb2, 0x00, 0xd8, 0x26, 0x4d, 0xff, 0xb2, 0x00, 0xd8, 0x26, 0xff, 0x4d, /* 30% */
	0x00, 0xa6, 0xd2, 0x2c, 0xff, 0x59, 0xa6, 0x00, 0x2c, 0xd2, 0xff, 0x59, 0xa6, 0x00, 0xd2, 0x2c, 0x59, 0xff, 0xa6, 0x00, 0xd2, 0x2c, 0xff, 0x59, /* 35% */
	0x00, 0x99, 0xcc, 0x33, 0xff, 0x66, 0x99, 0x00, 0x33, 0xcc, 0xff, 0x66, 0x99, 0x00, 0xcc, 0x33, 0x66, 0xff, 0x99, 0x00, 0xcc, 0x33, 0xff, 0x66, /* 40% */
	0x00, 0x8c, 0xc5, 0x39, 0xff, 0x73, 0x8c, 0x00, 0x39, 0xc5, 0xff, 0x73, 0x8c, 0x00, 0xc5, 0x39, 0x73, 0xff, 0x8c, 0x00, 0xc5, 0x39, 0xff, 0x73, /* 45% */
	0x00, 0x7f, 0xbf, 0x40, 0xff, 0x80, 0x7f, 0x00, 0x40, 0xbf, 0xff, 0x80, 0x7f, 0x00, 0xbf, 0x40, 0x80, 0xff, 0x7f, 0x00, 0xbf, 0x40, 0xff, 0x80, /* 50% */
	0x00, 0x73, 0xb9, 0x46, 0xff, 0x8c, 0x73, 0x00, 0x46, 0xb9, 0xff, 0x8c, 0x73, 0x00, 0xb9, 0x46, 0x8c, 0xff, 0x73, 0x00, 0xb9, 0x46, 0xff, 0x8c, /* 55% */
	0x00, 0x66, 0xb2, 0x4c, 0xff, 0x99, 0x66, 0x00, 0x4c, 0xb2, 0xff, 0x99, 0x66, 0x00, 0xb2, 0x4c, 0x99, 0xff, 0x66, 0x00, 0xb2, 0x4c, 0xff, 0x99, /* 60% */

	//Cyan
	0x00, 0xcc, 0xff, 0x33, 0xff, 0x33, 0xcc, 0x00, 0x33, 0xff, 0xff, 0x33, 0xcc, 0x00, 0xff, 0x33, 0x33, 0xff, 0xcc, 0x00, 0xff, 0x33, 0xff, 0x33, /* 20% */
	0x00, 0xbf, 0xff, 0x40, 0xff, 0x40, 0xbf, 0x00, 0x40, 0xff, 0xff, 0x40, 0xbf, 0x00, 0xff, 0x40, 0x40, 0xff, 0xbf, 0x00, 0xff, 0x40, 0xff, 0x40, /* 25% */
	0x00, 0xb2, 0xff, 0x4d, 0xff, 0x4d, 0xb2, 0x00, 0x4d, 0xff, 0xff, 0x4d, 0xb2, 0x00, 0xff, 0x4d, 0x4d, 0xff, 0xb2, 0x00, 0xff, 0x4d, 0xff, 0x4d, /* 30% */
	0x00, 0xa6, 0xff, 0x59, 0xff, 0x59, 0xa6, 0x00, 0x59, 0xff, 0xff, 0x59, 0xa6, 0x00, 0xff, 0x59, 0x59, 0xff, 0xa6, 0x00, 0xff, 0x59, 0xff, 0x59, /* 35% */
	0x00, 0x99, 0xff, 0x66, 0xff, 0x66, 0x99, 0x00, 0x66, 0xff, 0xff, 0x66, 0x99, 0x00, 0xff, 0x66, 0x66, 0xff, 0x99, 0x00, 0xff, 0x66, 0xff, 0x66, /* 40% */
	0x00, 0x8c, 0xff, 0x73, 0xff, 0x73, 0x8c, 0x00, 0x73, 0xff, 0xff, 0x73, 0x8c, 0x00, 0xff, 0x73, 0x73, 0xff, 0x8c, 0x00, 0xff, 0x73, 0xff, 0x73, /* 45% */
	0x00, 0x7f, 0xff, 0x80, 0xff, 0x80, 0x7f, 0x00, 0x80, 0xff, 0xff, 0x80, 0x7f, 0x00, 0xff, 0x80, 0x80, 0xff, 0x7f, 0x00, 0xff, 0x80, 0xff, 0x80, /* 50% */
	0x00, 0x73, 0xff, 0x8c, 0xff, 0x8c, 0x73, 0x00, 0x8c, 0xff, 0xff, 0x8c, 0x73, 0x00, 0xff, 0x8c, 0x8c, 0xff, 0x73, 0x00, 0xff, 0x8c, 0xff, 0x8c, /* 55% */
	0x00, 0x66, 0xff, 0x99, 0xff, 0x99, 0x66, 0x00, 0x99, 0xff, 0xff, 0x99, 0x66, 0x00, 0xff, 0x99, 0x99, 0xff, 0x66, 0x00, 0xff, 0x99, 0xff, 0x99, /* 60% */

	//Spring green
	0x00, 0xcc, 0xff, 0x33, 0xe5, 0x19, 0xcc, 0x00, 0x33, 0xff, 0xe5, 0x19, 0xcc, 0x00, 0xff, 0x33, 0x19, 0xe5, 0xcc, 0x00, 0xff, 0x33, 0xe5, 0x19, /* 20% */
	0x00, 0xbf, 0xff, 0x40, 0xdf, 0x20, 0xbf, 0x00, 0x40, 0xff, 0xdf, 0x20, 0xbf, 0x00, 0xff, 0x40, 0x20, 0xdf, 0xbf, 0x00, 0xff, 0x40, 0xdf, 0x20, /* 25% */
	0x00, 0xb2, 0xff, 0x4d, 0xd8, 0x26, 0xb2, 0x00, 0x4d, 0xff, 0xd8, 0x26, 0xb2, 0x00, 0xff, 0x4d, 0x26, 0xd8, 0xb2, 0x00, 0xff, 0x4d, 0xd8, 0x26, /* 30% */
	0x00, 0xa6, 0xff, 0x59, 0xd2, 0x2c, 0xa6, 0x00, 0x59, 0xff, 0xd2, 0x2c, 0xa6, 0x00, 0xff, 0x59, 0x2c, 0xd2, 0xa6, 0x00, 0xff, 0x59, 0xd2, 0x2c, /* 35% */
	0x00, 0x99, 0xff, 0x66, 0xcc, 0x33, 0x99, 0x00, 0x66, 0xff, 0xcc, 0x33, 0x99, 0x00, 0xff, 0x66, 0x33, 0xcc, 0x99, 0x00, 0xff, 0x66, 0xcc, 0x33, /* 40% */
	0x00, 0x8c, 0xff, 0x73, 0xc5, 0x39, 0x8c, 0x00, 0x73, 0xff, 0xc5, 0x39, 0x8c, 0x00, 0xff, 0x73, 0x39, 0xc5, 0x8c, 0x00, 0xff, 0x73, 0xc5, 0x39, /* 45% */
	0x00, 0x7f, 0xff, 0x80, 0xbf, 0x40, 0x7f, 0x00, 0x80, 0xff, 0xbf, 0x40, 0x7f, 0x00, 0xff, 0x80, 0x40, 0xbf, 0x7f, 0x00, 0xff, 0x80, 0xbf, 0x40, /* 50% */
	0x00, 0x73, 0xff, 0x8c, 0xb9, 0x46, 0x73, 0x00, 0x8c, 0xff, 0xb9, 0x46, 0x73, 0x00, 0xff, 0x8c, 0x46, 0xb9, 0x73, 0x00, 0xff, 0x8c, 0xb9, 0x46, /* 55% */
	0x00, 0x66, 0xff, 0x99, 0xb2, 0x4c, 0x66, 0x00, 0x99, 0xff, 0xb2, 0x4c, 0x66, 0x00, 0xff, 0x99, 0x4c, 0xb2, 0x66, 0x00, 0xff, 0x99, 0xb2, 0x4c, /* 60% */

	//Green
	0x00, 0xcc, 0xff, 0x33, 0xcc, 0x00, 0xcc, 0x00, 0x33, 0xff, 0xcc, 0x00, 0xcc, 0x00, 0xff, 0x33, 0x00, 0xcc, 0xcc, 0x00, 0xff, 0x33, 0xcc, 0x00, /* 20% */
	0x00, 0xbf, 0xff, 0x40, 0xbf, 0x00, 0xbf, 0x00, 0x40, 0xff, 0xbf, 0x00, 0xbf, 0x00, 0xff, 0x40, 0x00, 0xbf, 0xbf, 0x00, 0xff, 0x40, 0xbf, 0x00, /* 25% */
	0x00, 0xb2, 0xff, 0x4d, 0xb2, 0x00, 0xb2, 0x00, 0x4d, 0xff, 0xb2, 0x00, 0xb2, 0x00, 0xff, 0x4d, 0x00, 0xb2, 0xb2, 0x00, 0xff, 0x4d, 0xb2, 0x00, /* 30% */
	0x00, 0xa6, 0xff, 0x59, 0xa6, 0x00, 0xa6, 0x00, 0x59, 0xff, 0xa6, 0x00, 0xa6, 0x00, 0xff, 0x59, 0x00, 0xa6, 0xa6, 0x00, 0xff, 0x59, 0xa6, 0x00, /* 35% */
	0x00, 0x99, 0xff, 0x66, 0x99, 0x00, 0x99, 0x00, 0x66, 0xff, 0x99, 0x00, 0x99, 0x00, 0xff, 0x66, 0x00, 0x99, 0x99, 0x00, 0xff, 0x66, 0x99, 0x00, /* 40% */
	0x00, 0x8c, 0xff, 0x73, 0x8c, 0x00, 0x8c, 0x00, 0x73, 0xff, 0x8c, 0x00, 0x8c, 0x00, 0xff, 0x73, 0x00, 0x8c, 0x8c, 0x00, 0xff, 0x73, 0x8c, 0x00, /* 45% */
	0x00, 0x7f, 0xff, 0x80, 0x7f, 0x00, 0x7f, 0x00, 0x80, 0xff, 0x7f, 0x00, 0x7f, 0x00, 0xff, 0x80, 0x00, 0x7f, 0x7f, 0x00, 0xff, 0x80, 0x7f, 0x00, /* 50% */
	0x00, 0x73, 0xff, 0x8c, 0x73, 0x00, 0x73, 0x00, 0x8c, 0xff, 0x73, 0x00, 0x73, 0x00, 0xff, 0x8c, 0x00, 0x73, 0x73, 0x00, 0xff, 0x8c, 0x73, 0x00, /* 55% */
	0x00, 0x66, 0xff, 0x99, 0x66, 0x00, 0x66, 0x00, 0x99, 0xff, 0x66, 0x00, 0x66, 0x00, 0xff, 0x99, 0x00, 0x66, 0x66, 0x00, 0xff, 0x99, 0x66, 0x00, /* 60% */

	//Chartreuse Green
	0x19, 0xe5, 0xff, 0x33, 0xcc, 0x00, 0xe5, 0x19, 0x33, 0xff, 0xcc, 0x00, 0xe5, 0x19, 0xff, 0x33, 0x00, 0xcc, 0xe5, 0x19, 0xff, 0x33, 0xcc, 0x00, /* 20% */
	0x20, 0xdf, 0xff, 0x40, 0xbf, 0x00, 0xdf, 0x20, 0x40, 0xff, 0xbf, 0x00, 0xdf, 0x20, 0xff, 0x40, 0x00, 0xbf, 0xdf, 0x20, 0xff, 0x40, 0xbf, 0x00, /* 25% */
	0x26, 0xd8, 0xff, 0x4d, 0xb2, 0x00, 0xd8, 0x26, 0x4d, 0xff, 0xb2, 0x00, 0xd8, 0x26, 0xff, 0x4d, 0x00, 0xb2, 0xd8, 0x26, 0xff, 0x4d, 0xb2, 0x00, /* 30% */
	0x2c, 0xd2, 0xff, 0x59, 0xa6, 0x00, 0xd2, 0x2c, 0x59, 0xff, 0xa6, 0x00, 0xd2, 0x2c, 0xff, 0x59, 0x00, 0xa6, 0xd2, 0x2c, 0xff, 0x59, 0xa6, 0x00, /* 35% */
	0x33, 0xcc, 0xff, 0x66, 0x99, 0x00, 0xcc, 0x33, 0x66, 0xff, 0x99, 0x00, 0xcc, 0x33, 0xff, 0x66, 0x00, 0x99, 0xcc, 0x33, 0xff, 0x66, 0x99, 0x00, /* 40% */
	0x39, 0xc5, 0xff, 0x73, 0x8c, 0x00, 0xc5, 0x39, 0x73, 0xff, 0x8c, 0x00, 0xc5, 0x39, 0xff, 0x73, 0x00, 0x8c, 0xc5, 0x39, 0xff, 0x73, 0x8c, 0x00, /* 45% */
	0x40, 0xbf, 0xff, 0x80, 0x7f, 0x00, 0xbf, 0x40, 0x80, 0xff, 0x7f, 0x00, 0xbf, 0x40, 0xff, 0x80, 0x00, 0x7f, 0xbf, 0x40, 0xff, 0x80, 0x7f, 0x00, /* 50% */
	0x46, 0xb9, 0xff, 0x8c, 0x73, 0x00, 0xb9, 0x46, 0x8c, 0xff, 0x73, 0x00, 0xb9, 0x46, 0xff, 0x8c, 0x00, 0x73, 0xb9, 0x46, 0xff, 0x8c, 0x73, 0x00, /* 55% */
	0x4c, 0xb2, 0xff, 0x99, 0x66, 0x00, 0xb2, 0x4c, 0x99, 0xff, 0x66, 0x00, 0xb2, 0x4c, 0xff, 0x99, 0x00, 0x66, 0xb2, 0x4c, 0xff, 0x99, 0x66, 0x00, /* 60% */

	//Yellow
	0x33, 0xff, 0xff, 0x33, 0xcc, 0x00, 0xff, 0x33, 0x33, 0xff, 0xcc, 0x00, 0xff, 0x33, 0xff, 0x33, 0x00, 0xcc, 0xff, 0x33, 0xff, 0x33, 0xcc, 0x00, /* 20% */
	0x40, 0xff, 0xff, 0x40, 0xbf, 0x00, 0xff, 0x40, 0x40, 0xff, 0xbf, 0x00, 0xff, 0x40, 0xff, 0x40, 0x00, 0xbf, 0xff, 0x40, 0xff, 0x40, 0xbf, 0x00, /* 25% */
	0x4d, 0xff, 0xff, 0x4d, 0xb2, 0x00, 0xff, 0x4d, 0x4d, 0xff, 0xb2, 0x00, 0xff, 0x4d, 0xff, 0x4d, 0x00, 0xb2, 0xff, 0x4d, 0xff, 0x4d, 0xb2, 0x00, /* 30% */
	0x59, 0xff, 0xff, 0x59, 0xa6, 0x00, 0xff, 0x59, 0x59, 0xff, 0xa6, 0x00, 0xff, 0x59, 0xff, 0x59, 0x00, 0xa6, 0xff, 0x59, 0xff, 0x59, 0xa6, 0x00, /* 35% */
	0x66, 0xff, 0xff, 0x66, 0x99, 0x00, 0xff, 0x66, 0x66, 0xff, 0x99, 0x00, 0xff, 0x66, 0xff, 0x66, 0x00, 0x99, 0xff, 0x66, 0xff, 0x66, 0x99, 0x00, /* 40% */
	0x73, 0xff, 0xff, 0x73, 0x8c, 0x00, 0xff, 0x73, 0x73, 0xff, 0x8c, 0x00, 0xff, 0x73, 0xff, 0x73, 0x00, 0x8c, 0xff, 0x73, 0xff, 0x73, 0x8c, 0x00, /* 45% */
	0x80, 0xff, 0xff, 0x80, 0x7f, 0x00, 0xff, 0x80, 0x80, 0xff, 0x7f, 0x00, 0xff, 0x80, 0xff, 0x80, 0x00, 0x7f, 0xff, 0x80, 0xff, 0x80, 0x7f, 0x00, /* 50% */
	0x8c, 0xff, 0xff, 0x8c, 0x73, 0x00, 0xff, 0x8c, 0x8c, 0xff, 0x73, 0x00, 0xff, 0x8c, 0xff, 0x8c, 0x00, 0x73, 0xff, 0x8c, 0xff, 0x8c, 0x73, 0x00, /* 55% */
	0x99, 0xff, 0xff, 0x99, 0x66, 0x00, 0xff, 0x99, 0x99, 0xff, 0x66, 0x00, 0xff, 0x99, 0xff, 0x99, 0x00, 0x66, 0xff, 0x99, 0xff, 0x99, 0x66, 0x00, /* 60% */

	//Orange
	0x33, 0xff, 0xe5, 0x19, 0xcc, 0x00, 0xff, 0x33, 0x19, 0xe5, 0xcc, 0x00, 0xff, 0x33, 0xe5, 0x19, 0x00, 0xcc, 0xff, 0x33, 0xe5, 0x19, 0xcc, 0x00, /* 20% */
	0x40, 0xff, 0xdf, 0x20, 0xbf, 0x00, 0xff, 0x40, 0x20, 0xdf, 0xbf, 0x00, 0xff, 0x40, 0xdf, 0x20, 0x00, 0xbf, 0xff, 0x40, 0xdf, 0x20, 0xbf, 0x00, /* 25% */
	0x4d, 0xff, 0xd8, 0x26, 0xb2, 0x00, 0xff, 0x4d, 0x26, 0xd8, 0xb2, 0x00, 0xff, 0x4d, 0xd8, 0x26, 0x00, 0xb2, 0xff, 0x4d, 0xd8, 0x26, 0xb2, 0x00, /* 30% */
	0x59, 0xff, 0xd2, 0x2c, 0xa6, 0x00, 0xff, 0x59, 0x2c, 0xd2, 0xa6, 0x00, 0xff, 0x59, 0xd2, 0x2c, 0x00, 0xa6, 0xff, 0x59, 0xd2, 0x2c, 0xa6, 0x00, /* 35% */
	0x66, 0xff, 0xcc, 0x33, 0x99, 0x00, 0xff, 0x66, 0x33, 0xcc, 0x99, 0x00, 0xff, 0x66, 0xcc, 0x33, 0x00, 0x99, 0xff, 0x66, 0xcc, 0x33, 0x99, 0x00, /* 40% */
	0x73, 0xff, 0xc5, 0x39, 0x8c, 0x00, 0xff, 0x73, 0x39, 0xc5, 0x8c, 0x00, 0xff, 0x73, 0xc5, 0x39, 0x00, 0x8c, 0xff, 0x73, 0xc5, 0x39, 0x8c, 0x00, /* 45% */
	0x80, 0xff, 0xbf, 0x40, 0x7f, 0x00, 0xff, 0x80, 0x40, 0xbf, 0x7f, 0x00, 0xff, 0x80, 0xbf, 0x40, 0x00, 0x7f, 0xff, 0x80, 0xbf, 0x40, 0x7f, 0x00, /* 50% */
	0x8c, 0xff, 0xb9, 0x46, 0x73, 0x00, 0xff, 0x8c, 0x46, 0xb9, 0x73, 0x00, 0xff, 0x8c, 0xb9, 0x46, 0x00, 0x73, 0xff, 0x8c, 0xb9, 0x46, 0x73, 0x00, /* 55% */
	0x99, 0xff, 0xb2, 0x4c, 0x66, 0x00, 0xff, 0x99, 0x4c, 0xb2, 0x66, 0x00, 0xff, 0x99, 0xb2, 0x4c, 0x00, 0x66, 0xff, 0x99, 0xb2, 0x4c, 0x66, 0x00, /* 60% */

	//Red
	0x33, 0xff, 0xcc, 0x00, 0xcc, 0x00, 0xff, 0x33, 0x00, 0xcc, 0xcc, 0x00, 0xff, 0x33, 0xcc, 0x00, 0x00, 0xcc, 0xff, 0x33, 0xcc, 0x00, 0xcc, 0x00, /* 20% */
	0x40, 0xff, 0xbf, 0x00, 0xbf, 0x00, 0xff, 0x40, 0x00, 0xbf, 0xbf, 0x00, 0xff, 0x40, 0xbf, 0x00, 0x00, 0xbf, 0xff, 0x40, 0xbf, 0x00, 0xbf, 0x00, /* 25% */
	0x4d, 0xff, 0xb2, 0x00, 0xb2, 0x00, 0xff, 0x4d, 0x00, 0xb2, 0xb2, 0x00, 0xff, 0x4d, 0xb2, 0x00, 0x00, 0xb2, 0xff, 0x4d, 0xb2, 0x00, 0xb2, 0x00, /* 30% */
	0x59, 0xff, 0xa6, 0x00, 0xa6, 0x00, 0xff, 0x59, 0x00, 0xa6, 0xa6, 0x00, 0xff, 0x59, 0xa6, 0x00, 0x00, 0xa6, 0xff, 0x59, 0xa6, 0x00, 0xa6, 0x00, /* 35% */
	0x66, 0xff, 0x99, 0x00, 0x99, 0x00, 0xff, 0x66, 0x00, 0x99, 0x99, 0x00, 0xff, 0x66, 0x99, 0x00, 0x00, 0x99, 0xff, 0x66, 0x99, 0x00, 0x99, 0x00, /* 40% */
	0x73, 0xff, 0x8c, 0x00, 0x8c, 0x00, 0xff, 0x73, 0x00, 0x8c, 0x8c, 0x00, 0xff, 0x73, 0x8c, 0x00, 0x00, 0x8c, 0xff, 0x73, 0x8c, 0x00, 0x8c, 0x00, /* 45% */
	0x80, 0xff, 0x7f, 0x00, 0x7f, 0x00, 0xff, 0x80, 0x00, 0x7f, 0x7f, 0x00, 0xff, 0x80, 0x7f, 0x00, 0x00, 0x7f, 0xff, 0x80, 0x7f, 0x00, 0x7f, 0x00, /* 50% */
	0x8c, 0xff, 0x73, 0x00, 0x73, 0x00, 0xff, 0x8c, 0x00, 0x73, 0x73, 0x00, 0xff, 0x8c, 0x73, 0x00, 0x00, 0x73, 0xff, 0x8c, 0x73, 0x00, 0x73, 0x00, /* 55% */
	0x99, 0xff, 0x66, 0x00, 0x66, 0x00, 0xff, 0x99, 0x00, 0x66, 0x66, 0x00, 0xff, 0x99, 0x66, 0x00, 0x00, 0x66, 0xff, 0x99, 0x66, 0x00, 0x66, 0x00, /* 60% */

	//Rose
	0x33, 0xff, 0xcc, 0x00, 0xe5, 0x19, 0xff, 0x33, 0x00, 0xcc, 0xe5, 0x19, 0xff, 0x33, 0xcc, 0x00, 0x19, 0xe5, 0xff, 0x33, 0xcc, 0x00, 0xe5, 0x19, /* 20% */
	0x40, 0xff, 0xbf, 0x00, 0xdf, 0x20, 0xff, 0x40, 0x00, 0xbf, 0xdf, 0x20, 0xff, 0x40, 0xbf, 0x00, 0x20, 0xdf, 0xff, 0x40, 0xbf, 0x00, 0xdf, 0x20, /* 25% */
	0x4d, 0xff, 0xb2, 0x00, 0xd8, 0x26, 0xff, 0x4d, 0x00, 0xb2, 0xd8, 0x26, 0xff, 0x4d, 0xb2, 0x00, 0x26, 0xd8, 0xff, 0x4d, 0xb2, 0x00, 0xd8, 0x26, /* 30% */
	0x59, 0xff, 0xa6, 0x00, 0xd2, 0x2c, 0xff, 0x59, 0x00, 0xa6, 0xd2, 0x2c, 0xff, 0x59, 0xa6, 0x00, 0x2c, 0xd2, 0xff, 0x59, 0xa6, 0x00, 0xd2, 0x2c, /* 35% */
	0x66, 0xff, 0x99, 0x00, 0xcc, 0x33, 0xff, 0x66, 0x00, 0x99, 0xcc, 0x33, 0xff, 0x66, 0x99, 0x00, 0x33, 0xcc, 0xff, 0x66, 0x99, 0x00, 0xcc, 0x33, /* 40% */
	0x73, 0xff, 0x8c, 0x00, 0xc5, 0x39, 0xff, 0x73, 0x00, 0x8c, 0xc5, 0x39, 0xff, 0x73, 0x8c, 0x00, 0x39, 0xc5, 0xff, 0x73, 0x8c, 0x00, 0xc5, 0x39, /* 45% */
	0x80, 0xff, 0x7f, 0x00, 0xbf, 0x40, 0xff, 0x80, 0x00, 0x7f, 0xbf, 0x40, 0xff, 0x80, 0x7f, 0x00, 0x40, 0xbf, 0xff, 0x80, 0x7f, 0x00, 0xbf, 0x40, /* 50% */
	0x8c, 0xff, 0x73, 0x00, 0xb9, 0x46, 0xff, 0x8c, 0x00, 0x73, 0xb9, 0x46, 0xff, 0x8c, 0x73, 0x00, 0x46, 0xb9, 0xff, 0x8c, 0x73, 0x00, 0xb9, 0x46, /* 55% */
	0x99, 0xff, 0x66, 0x00, 0xb2, 0x4c, 0xff, 0x99, 0x00, 0x66, 0xb2, 0x4c, 0xff, 0x99, 0x66, 0x00, 0x4c, 0xb2, 0xff, 0x99, 0x66, 0x00, 0xb2, 0x4c, /* 60% */

	//Magenta
	0x33, 0xff, 0xcc, 0x00, 0xff, 0x33, 0xff, 0x33, 0x00, 0xcc, 0xff, 0x33, 0xff, 0x33, 0xcc, 0x00, 0x33, 0xff, 0xff, 0x33, 0xcc, 0x00, 0xff, 0x33, /* 20% */
	0x40, 0xff, 0xbf, 0x00, 0xff, 0x40, 0xff, 0x40, 0x00, 0xbf, 0xff, 0x40, 0xff, 0x40, 0xbf, 0x00, 0x40, 0xff, 0xff, 0x40, 0xbf, 0x00, 0xff, 0x40, /* 25% */
	0x4d, 0xff, 0xb2, 0x00, 0xff, 0x4d, 0xff, 0x4d, 0x00, 0xb2, 0xff, 0x4d, 0xff, 0x4d, 0xb2, 0x00, 0x4d, 0xff, 0xff, 0x4d, 0xb2, 0x00, 0xff, 0x4d, /* 30% */
	0x59, 0xff, 0xa6, 0x00, 0xff, 0x59, 0xff, 0x59, 0x00, 0xa6, 0xff, 0x59, 0xff, 0x59, 0xa6, 0x00, 0x59, 0xff, 0xff, 0x59, 0xa6, 0x00, 0xff, 0x59, /* 35% */
	0x66, 0xff, 0x99, 0x00, 0xff, 0x66, 0xff, 0x66, 0x00, 0x99, 0xff, 0x66, 0xff, 0x66, 0x99, 0x00, 0x66, 0xff, 0xff, 0x66, 0x99, 0x00, 0xff, 0x66, /* 40% */
	0x73, 0xff, 0x8c, 0x00, 0xff, 0x73, 0xff, 0x73, 0x00, 0x8c, 0xff, 0x73, 0xff, 0x73, 0x8c, 0x00, 0x73, 0xff, 0xff, 0x73, 0x8c, 0x00, 0xff, 0x73, /* 45% */
	0x80, 0xff, 0x7f, 0x00, 0xff, 0x80, 0xff, 0x80, 0x00, 0x7f, 0xff, 0x80, 0xff, 0x80, 0x7f, 0x00, 0x80, 0xff, 0xff, 0x80, 0x7f, 0x00, 0xff, 0x80, /* 50% */
	0x8c, 0xff, 0x73, 0x00, 0xff, 0x8c, 0xff, 0x8c, 0x00, 0x73, 0xff, 0x8c, 0xff, 0x8c, 0x73, 0x00, 0x8c, 0xff, 0xff, 0x8c, 0x73, 0x00, 0xff, 0x8c, /* 55% */
	0x99, 0xff, 0x66, 0x00, 0xff, 0x99, 0xff, 0x99, 0x00, 0x66, 0xff, 0x99, 0xff, 0x99, 0x66, 0x00, 0x99, 0xff, 0xff, 0x99, 0x66, 0x00, 0xff, 0x99, /* 60% */

	//Violet
	0x19, 0xe5, 0xcc, 0x00, 0xff, 0x33, 0xe5, 0x19, 0x00, 0xcc, 0xff, 0x33, 0xe5, 0x19, 0xcc, 0x00, 0x33, 0xff, 0xe5, 0x19, 0xcc, 0x00, 0xff, 0x33, /* 20% */
	0x20, 0xdf, 0xbf, 0x00, 0xff, 0x40, 0xdf, 0x20, 0x00, 0xbf, 0xff, 0x40, 0xdf, 0x20, 0xbf, 0x00, 0x40, 0xff, 0xdf, 0x20, 0xbf, 0x00, 0xff, 0x40, /* 25% */
	0x26, 0xd8, 0xb2, 0x00, 0xff, 0x4d, 0xd8, 0x26, 0x00, 0xb2, 0xff, 0x4d, 0xd8, 0x26, 0xb2, 0x00, 0x4d, 0xff, 0xd8, 0x26, 0xb2, 0x00, 0xff, 0x4d, /* 30% */
	0x2c, 0xd2, 0xa6, 0x00, 0xff, 0x59, 0xd2, 0x2c, 0x00, 0xa6, 0xff, 0x59, 0xd2, 0x2c, 0xa6, 0x00, 0x59, 0xff, 0xd2, 0x2c, 0xa6, 0x00, 0xff, 0x59, /* 35% */
	0x33, 0xcc, 0x99, 0x00, 0xff, 0x66, 0xcc, 0x33, 0x00, 0x99, 0xff, 0x66, 0xcc, 0x33, 0x99, 0x00, 0x66, 0xff, 0xcc, 0x33, 0x99, 0x00, 0xff, 0x66, /* 40% */
	0x39, 0xc5, 0x8c, 0x00, 0xff, 0x73, 0xc5, 0x39, 0x00, 0x8c, 0xff, 0x73, 0xc5, 0x39, 0x8c, 0x00, 0x73, 0xff, 0xc5, 0x39, 0x8c, 0x00, 0xff, 0x73, /* 45% */
	0x40, 0xbf, 0x7f, 0x00, 0xff, 0x80, 0xbf, 0x40, 0x00, 0x7f, 0xff, 0x80, 0xbf, 0x40, 0x7f, 0x00, 0x80, 0xff, 0xbf, 0x40, 0x7f, 0x00, 0xff, 0x80, /* 50% */
	0x46, 0xb9, 0x73, 0x00, 0xff, 0x8c, 0xb9, 0x46, 0x00, 0x73, 0xff, 0x8c, 0xb9, 0x46, 0x73, 0x00, 0x8c, 0xff, 0xb9, 0x46, 0x73, 0x00, 0xff, 0x8c, /* 55% */
	0x4c, 0xb2, 0x66, 0x00, 0xff, 0x99, 0xb2, 0x4c, 0x00, 0x66, 0xff, 0x99, 0xb2, 0x4c, 0x66, 0x00, 0x99, 0xff, 0xb2, 0x4c, 0x66, 0x00, 0xff, 0x99, /* 60% */
};

static char DSI0_BYPASS_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_BYPASS_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x00, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x00, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_NEGATIVE_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0x00, /* ascr_Wr */
	0xff, /* ascr_Kr */
	0x00, /* ascr_Wg */
	0xff, /* ascr_Kg */
	0x00, /* ascr_Wb */
	0xff, /* ascr_Kb */
	/* end */
};
static char DSI0_NEGATIVE_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_GRAYSCALE_MDNIE_1[] = {
	/* start */
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0xb3, /* ascr_Cr */
	0x4c, /* ascr_Rr */
	0xb3, /* ascr_Cg */
	0x4c, /* ascr_Rg */
	0xb3, /* ascr_Cb */
	0x4c, /* ascr_Rb */
	0x69, /* ascr_Mr */
	0x96, /* ascr_Gr */
	0x69, /* ascr_Mg */
	0x96, /* ascr_Gg */
	0x69, /* ascr_Mb */
	0x96, /* ascr_Gb */
	0xe2, /* ascr_Yr */
	0x1d, /* ascr_Br */
	0xe2, /* ascr_Yg */
	0x1d, /* ascr_Bg */
	0xe2, /* ascr_Yb */
	0x1d, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_GRAYSCALE_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_GRAYSCALE_NEGATIVE_MDNIE_1[] = {
	/* start */
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0xb3, /* ascr_Cr */
	0x4c, /* ascr_Rr */
	0xb3, /* ascr_Cg */
	0x4c, /* ascr_Rg */
	0xb3, /* ascr_Cb */
	0x4c, /* ascr_Rb */
	0x69, /* ascr_Mr */
	0x96, /* ascr_Gr */
	0x69, /* ascr_Mg */
	0x96, /* ascr_Gg */
	0x69, /* ascr_Mb */
	0x96, /* ascr_Gb */
	0xe2, /* ascr_Yr */
	0x1d, /* ascr_Br */
	0xe2, /* ascr_Yg */
	0x1d, /* ascr_Bg */
	0xe2, /* ascr_Yb */
	0x1d, /* ascr_Bb */
	0x00, /* ascr_Wr */
	0xff, /* ascr_Kr */
	0x00, /* ascr_Wg */
	0xff, /* ascr_Kg */
	0x00, /* ascr_Wb */
	0xff, /* ascr_Kb */
	/* end */
};
static char DSI0_GRAYSCALE_NEGATIVE_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_COLOR_BLIND_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_COLOR_BLIND_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_COLOR_LENS_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_COLOR_LENS_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_NIGHT_MODE_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_NIGHT_MODE_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_LIGHT_NOTIFICATION_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x66, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xf9, /* ascr_Cg */
	0x60, /* ascr_Rg */
	0xac, /* ascr_Cb */
	0x13, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x66, /* ascr_Gr */
	0x60, /* ascr_Mg */
	0xf9, /* ascr_Gg */
	0xac, /* ascr_Mb */
	0x13, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x66, /* ascr_Br */
	0xf9, /* ascr_Yg */
	0x60, /* ascr_Bg */
	0x13, /* ascr_Yb */
	0xac, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x66, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x60, /* ascr_Kg */
	0xac, /* ascr_Wb */
	0x13, /* ascr_Kb */
	/* end */
};

static char DSI0_LIGHT_NOTIFICATION_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_HBM_CE_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x30, /* ascr_skin_Rg */
	0x30, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_HBM_CE_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3f, /* ascr algo lce 10 10 10 */
	0x86, /* lce_on gain 0 00 0000 */
	0x30, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0x90, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0xbf,
	0x00, /* lce_ref_gain 9 */
	0xb0,
	0x77, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x00, /* lce_dark_th 000 */
	0x40, /* lce_min_ref_offset */
	0x07, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x60,
	0x01, /* de_maxplus 11 */
	0x00,
	0x01, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x40,
	0x00, /* curve_1_b */
	0x6b, /* curve_1_a */
	0x03, /* curve_2_b */
	0x48, /* curve_2_a */
	0x08, /* curve_3_b */
	0x32, /* curve_3_a */
	0x08, /* curve_4_b */
	0x32, /* curve_4_a */
	0x08, /* curve_5_b */
	0x32, /* curve_5_a */
	0x08, /* curve_6_b */
	0x32, /* curve_6_a */
	0x08, /* curve_7_b */
	0x32, /* curve_7_a */
	0x10, /* curve_8_b */
	0x28, /* curve_8_a */
	0x10, /* curve_9_b */
	0x28, /* curve_9_a */
	0x10, /* curve10_b */
	0x28, /* curve10_a */
	0x10, /* curve11_b */
	0x28, /* curve11_a */
	0x10, /* curve12_b */
	0x28, /* curve12_a */
	0x19, /* curve13_b */
	0x22, /* curve13_a */
	0x19, /* curve14_b */
	0x22, /* curve14_a */
	0x19, /* curve15_b */
	0x22, /* curve15_a */
	0x19, /* curve16_b */
	0x22, /* curve16_a */
	0x19, /* curve17_b */
	0x22, /* curve17_a */
	0x19, /* curve18_b */
	0x22, /* curve18_a */
	0x23, /* curve19_b */
	0x1e, /* curve19_a */
	0x2e, /* curve20_b */
	0x1b, /* curve20_a */
	0x33, /* curve21_b */
	0x1a, /* curve21_a */
	0x40, /* curve22_b */
	0x18, /* curve22_a */
	0x48, /* curve23_b */
	0x17, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_HBM_CE_TEXT_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x56, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x67, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x17,
	0xd0,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x13,
	0xe2,
	0xff, /* ascr_skin_Rr */
	0xa0, /* ascr_skin_Rg */
	0xa0, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0x90, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_HBM_CE_TEXT_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3f, /* ascr algo lce 10 10 10 */
	0x86, /* lce_on gain 0 00 0000 */
	0x30, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0x90, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0xbf,
	0x00, /* lce_ref_gain 9 */
	0xb0,
	0x77, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x00, /* lce_dark_th 000 */
	0x40, /* lce_min_ref_offset */
	0x06, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x60,
	0x01, /* de_maxplus 11 */
	0x00,
	0x01, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x40,
	0x00, /* curve_1_b */
	0x7b, /* curve_1_a */
	0x03, /* curve_2_b */
	0x48, /* curve_2_a */
	0x08, /* curve_3_b */
	0x32, /* curve_3_a */
	0x08, /* curve_4_b */
	0x32, /* curve_4_a */
	0x08, /* curve_5_b */
	0x32, /* curve_5_a */
	0x08, /* curve_6_b */
	0x32, /* curve_6_a */
	0x08, /* curve_7_b */
	0x32, /* curve_7_a */
	0x10, /* curve_8_b */
	0x28, /* curve_8_a */
	0x10, /* curve_9_b */
	0x28, /* curve_9_a */
	0x10, /* curve10_b */
	0x28, /* curve10_a */
	0x10, /* curve11_b */
	0x28, /* curve11_a */
	0x10, /* curve12_b */
	0x28, /* curve12_a */
	0x19, /* curve13_b */
	0x22, /* curve13_a */
	0x70, /* curve14_b */
	0xf7, /* curve14_a */
	0x70, /* curve15_b */
	0xf7, /* curve15_a */
	0x70, /* curve16_b */
	0xf7, /* curve16_a */
	0x70, /* curve17_b */
	0xf7, /* curve17_a */
	0x66, /* curve18_b */
	0x1a, /* curve18_a */
	0x76, /* curve19_b */
	0x14, /* curve19_a */
	0x82, /* curve20_b */
	0x11, /* curve20_a */
	0x92, /* curve21_b */
	0x0e, /* curve21_a */
	0x98, /* curve22_b */
	0x0d, /* curve22_a */
	0x9f, /* curve23_b */
	0x0c, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_SCREEN_CURTAIN_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0x00, /* ascr_Rr */
	0x00, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0x00, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0x00, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0x00, /* ascr_Gg */
	0x00, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0x00, /* ascr_Yr */
	0x00, /* ascr_Br */
	0x00, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0x00, /* ascr_Bb */
	0x00, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0x00, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0x00, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_SCREEN_CURTAIN_MDNIE_2[] = {
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_HDR_VIDEO_1_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x6a, /* ascr_skin_cb */
	0x9a, /* ascr_skin_cr */
	0x25, /* ascr_dist_up */
	0x1a, /* ascr_dist_down */
	0x16, /* ascr_dist_right */
	0x2a, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x37,
	0x5a,
	0x00, /* ascr_div_down */
	0x4e,
	0xc5,
	0x00, /* ascr_div_right */
	0x5d,
	0x17,
	0x00, /* ascr_div_left */
	0x30,
	0xc3,
	0xff, /* ascr_skin_Rr */
	0x00, /* ascr_skin_Rg */
	0x00, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf0, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xfc, /* ascr_skin_Wg */
	0xf6, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_HDR_VIDEO_1_MDNIE_2[] = {
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x00, /* de_maxplus 11 */
	0x40,
	0x00, /* de_maxminus 11 */
	0x40,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x10,
	0x00, /* fa_pcl_ppi 14 */
	0x00,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0x67, /* fa_skin_cr */
	0xa9, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0xd0,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_HDR_VIDEO_2_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x6a, /* ascr_skin_cb */
	0x9a, /* ascr_skin_cr */
	0x25, /* ascr_dist_up */
	0x1a, /* ascr_dist_down */
	0x16, /* ascr_dist_right */
	0x2a, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x37,
	0x5a,
	0x00, /* ascr_div_down */
	0x4e,
	0xc5,
	0x00, /* ascr_div_right */
	0x5d,
	0x17,
	0x00, /* ascr_div_left */
	0x30,
	0xc3,
	0xff, /* ascr_skin_Rr */
	0x00, /* ascr_skin_Rg */
	0x00, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf0, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xfc, /* ascr_skin_Wg */
	0xf6, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_HDR_VIDEO_2_MDNIE_2[] = {
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x00, /* de_maxplus 11 */
	0x40,
	0x00, /* de_maxminus 11 */
	0x40,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x10,
	0x00, /* fa_pcl_ppi 14 */
	0x00,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0x67, /* fa_skin_cr */
	0xa9, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0xd0,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_HDR_VIDEO_3_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x6a, /* ascr_skin_cb */
	0x9a, /* ascr_skin_cr */
	0x25, /* ascr_dist_up */
	0x1a, /* ascr_dist_down */
	0x16, /* ascr_dist_right */
	0x2a, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x37,
	0x5a,
	0x00, /* ascr_div_down */
	0x4e,
	0xc5,
	0x00, /* ascr_div_right */
	0x5d,
	0x17,
	0x00, /* ascr_div_left */
	0x30,
	0xc3,
	0xff, /* ascr_skin_Rr */
	0x00, /* ascr_skin_Rg */
	0x00, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf0, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_HDR_VIDEO_3_MDNIE_2[] = {
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x00, /* de_maxplus 11 */
	0x40,
	0x00, /* de_maxminus 11 */
	0x40,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x10,
	0x00, /* fa_pcl_ppi 14 */
	0x00,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0x67, /* fa_skin_cr */
	0xa9, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0xd0,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_UI_DYNAMIC_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 0 0000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x37, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x47, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x25,
	0x3d,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x1c,
	0xd8,
	0xff, /* ascr_skin_Rr */
	0x60, /* ascr_skin_Rg */
	0x82, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf4, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x66, /* ascr_Cr */
	0xdf, /* ascr_Rr */
	0xfc, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xf3, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xf2, /* ascr_Mr */
	0x57, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xf1, /* ascr_Gg */
	0xf4, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xf5, /* ascr_Yr */
	0x2f, /* ascr_Br */
	0xf1, /* ascr_Yg */
	0x32, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xea, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_UI_DYNAMIC_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x07, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x02, /* de_maxplus 11 */
	0x00,
	0x02, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_UI_STANDARD_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xee, /* ascr_Rr */
	0xec, /* ascr_Cg */
	0x28, /* ascr_Rg */
	0xe6, /* ascr_Cb */
	0x2e, /* ascr_Rb */
	0xfd, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x4a, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xe9, /* ascr_Mb */
	0x10, /* ascr_Gb */
	0xee, /* ascr_Yr */
	0x2c, /* ascr_Br */
	0xea, /* ascr_Yg */
	0x30, /* ascr_Bg */
	0x44, /* ascr_Yb */
	0xd4, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_UI_STANDARD_MDNIE_2[] = {
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_UI_NATURAL_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x8c, /* ascr_Cr */
	0xc2, /* ascr_Rr */
	0xe8, /* ascr_Cg */
	0x1c, /* ascr_Rg */
	0xe2, /* ascr_Cb */
	0x24, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x88, /* ascr_Gr */
	0x3f, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xda, /* ascr_Mb */
	0x46, /* ascr_Gb */
	0xe4, /* ascr_Yr */
	0x26, /* ascr_Br */
	0xde, /* ascr_Yg */
	0x2e, /* ascr_Bg */
	0x4a, /* ascr_Yb */
	0xc8, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_UI_NATURAL_MDNIE_2[] ={
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_UI_MOVIE_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xe9, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_UI_MOVIE_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x00, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_UI_AUTO_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x5c, /* ascr_skin_Rg */
	0x68, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf8, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xf0, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_UI_AUTO_MDNIE_2[] = {
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x15,
	0x00, /* de_maxplus 11 */
	0xa0,
	0x00, /* de_maxminus 11 */
	0xa0,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x70,
	0x01, /* fa_pcl_ppi 14 */
	0xc0,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0xa9, /* fa_skin_cr */
	0x67, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static unsigned char DSI0_VIDEO_OUTDOOR_MDNIE_1[] = {
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static unsigned char DSI0_VIDEO_OUTDOOR_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VIDEO_DYNAMIC_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 0 0000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x37, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x47, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x25,
	0x3d,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x1c,
	0xd8,
	0xff, /* ascr_skin_Rr */
	0x60, /* ascr_skin_Rg */
	0x82, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf4, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x66, /* ascr_Cr */
	0xdf, /* ascr_Rr */
	0xfc, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xf3, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xf2, /* ascr_Mr */
	0x57, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xf1, /* ascr_Gg */
	0xf4, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xf5, /* ascr_Yr */
	0x2f, /* ascr_Br */
	0xf1, /* ascr_Yg */
	0x32, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xea, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_VIDEO_DYNAMIC_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x07, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x02, /* de_maxplus 11 */
	0x00,
	0x02, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VIDEO_STANDARD_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xee, /* ascr_Rr */
	0xec, /* ascr_Cg */
	0x28, /* ascr_Rg */
	0xe6, /* ascr_Cb */
	0x2e, /* ascr_Rb */
	0xfd, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x4a, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xe9, /* ascr_Mb */
	0x10, /* ascr_Gb */
	0xee, /* ascr_Yr */
	0x2c, /* ascr_Br */
	0xea, /* ascr_Yg */
	0x30, /* ascr_Bg */
	0x44, /* ascr_Yb */
	0xd4, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_VIDEO_STANDARD_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VIDEO_NATURAL_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x8c, /* ascr_Cr */
	0xc2, /* ascr_Rr */
	0xe8, /* ascr_Cg */
	0x1c, /* ascr_Rg */
	0xe2, /* ascr_Cb */
	0x24, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x88, /* ascr_Gr */
	0x3f, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xda, /* ascr_Mb */
	0x46, /* ascr_Gb */
	0xe4, /* ascr_Yr */
	0x26, /* ascr_Br */
	0xde, /* ascr_Yg */
	0x2e, /* ascr_Bg */
	0x4a, /* ascr_Yb */
	0xc8, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_VIDEO_NATURAL_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3C, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VIDEO_MOVIE_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xe9, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_VIDEO_MOVIE_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x00, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VIDEO_AUTO_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x6a, /* ascr_skin_cb */
	0x9a, /* ascr_skin_cr */
	0x25, /* ascr_dist_up */
	0x1a, /* ascr_dist_down */
	0x16, /* ascr_dist_right */
	0x2a, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x37,
	0x5a,
	0x00, /* ascr_div_down */
	0x4e,
	0xc5,
	0x00, /* ascr_div_right */
	0x5d,
	0x17,
	0x00, /* ascr_div_left */
	0x30,
	0xc3,
	0xff, /* ascr_skin_Rr */
	0x34, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf9, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xd8, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xd9, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xe0, /* ascr_Rr */
	0xf0, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x3b, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xd9, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x14, /* ascr_Br */
	0xf9, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_VIDEO_AUTO_MDNIE_2[] ={
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x04, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x00, /* de_maxplus 11 */
	0x40,
	0x00, /* de_maxminus 11 */
	0x40,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x70,
	0x01, /* fa_pcl_ppi 14 */
	0xc0,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0xa9, /* fa_skin_cr */
	0x67, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x20,
	0x00, /* curve_1_b */
	0x14, /* curve_1_a */
	0x00, /* curve_2_b */
	0x14, /* curve_2_a */
	0x00, /* curve_3_b */
	0x14, /* curve_3_a */
	0x00, /* curve_4_b */
	0x14, /* curve_4_a */
	0x03, /* curve_5_b */
	0x9a, /* curve_5_a */
	0x03, /* curve_6_b */
	0x9a, /* curve_6_a */
	0x03, /* curve_7_b */
	0x9a, /* curve_7_a */
	0x03, /* curve_8_b */
	0x9a, /* curve_8_a */
	0x07, /* curve_9_b */
	0x9e, /* curve_9_a */
	0x07, /* curve10_b */
	0x9e, /* curve10_a */
	0x07, /* curve11_b */
	0x9e, /* curve11_a */
	0x07, /* curve12_b */
	0x9e, /* curve12_a */
	0x0a, /* curve13_b */
	0xa0, /* curve13_a */
	0x0a, /* curve14_b */
	0xa0, /* curve14_a */
	0x0a, /* curve15_b */
	0xa0, /* curve15_a */
	0x0a, /* curve16_b */
	0xa0, /* curve16_a */
	0x16, /* curve17_b */
	0xa6, /* curve17_a */
	0x16, /* curve18_b */
	0xa6, /* curve18_a */
	0x16, /* curve19_b */
	0xa6, /* curve19_a */
	0x16, /* curve20_b */
	0xa6, /* curve20_a */
	0x05, /* curve21_b */
	0x21, /* curve21_a */
	0x0b, /* curve22_b */
	0x20, /* curve22_a */
	0x87, /* curve23_b */
	0x0f, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static unsigned char DSI0_CAMERA_OUTDOOR_MDNIE_1[] = {
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static unsigned char DSI0_CAMERA_OUTDOOR_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_CAMERA_DYNAMIC_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 0 0000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x37, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x47, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x25,
	0x3d,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x1c,
	0xd8,
	0xff, /* ascr_skin_Rr */
	0x60, /* ascr_skin_Rg */
	0x82, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf4, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x66, /* ascr_Cr */
	0xdf, /* ascr_Rr */
	0xfc, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xf3, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xf2, /* ascr_Mr */
	0x57, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xf1, /* ascr_Gg */
	0xf4, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xf5, /* ascr_Yr */
	0x2f, /* ascr_Br */
	0xf1, /* ascr_Yg */
	0x32, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xea, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_CAMERA_DYNAMIC_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x07, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x02, /* de_maxplus 11 */
	0x00,
	0x02, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_CAMERA_STANDARD_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xee, /* ascr_Rr */
	0xec, /* ascr_Cg */
	0x28, /* ascr_Rg */
	0xe6, /* ascr_Cb */
	0x2e, /* ascr_Rb */
	0xfd, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x4a, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xe9, /* ascr_Mb */
	0x10, /* ascr_Gb */
	0xee, /* ascr_Yr */
	0x2c, /* ascr_Br */
	0xea, /* ascr_Yg */
	0x30, /* ascr_Bg */
	0x44, /* ascr_Yb */
	0xd4, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_CAMERA_STANDARD_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_CAMERA_NATURAL_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08a8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x8c, /* ascr_Cr */
	0xc2, /* ascr_Rr */
	0xe8, /* ascr_Cg */
	0x1c, /* ascr_Rg */
	0xe2, /* ascr_Cb */
	0x24, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x88, /* ascr_Gr */
	0x3f, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xda, /* ascr_Mb */
	0x46, /* ascr_Gb */
	0xe4, /* ascr_Yr */
	0x26, /* ascr_Br */
	0xde, /* ascr_Yg */
	0x2e, /* ascr_Bg */
	0x4a, /* ascr_Yb */
	0xc8, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_CAMERA_NATURAL_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_CAMERA_MOVIE_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08a8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xe9, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_CAMERA_MOVIE_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x00, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_CAMERA_AUTO_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x6a, /* ascr_skin_cb */
	0x9a, /* ascr_skin_cr */
	0x25, /* ascr_dist_up */
	0x1a, /* ascr_dist_down */
	0x16, /* ascr_dist_right */
	0x2a, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x37,
	0x5a,
	0x00, /* ascr_div_down */
	0x4e,
	0xc5,
	0x00, /* ascr_div_right */
	0x5d,
	0x17,
	0x00, /* ascr_div_left */
	0x30,
	0xc3,
	0xff, /* ascr_skin_Rr */
	0x34, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf9, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xd8, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xd9, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xe0, /* ascr_Rr */
	0xf0, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x3b, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xd9, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x14, /* ascr_Br */
	0xf9, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_CAMERA_AUTO_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x15,
	0x00, /* de_maxplus 11 */
	0xa0,
	0x00, /* de_maxminus 11 */
	0xa0,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x70,
	0x01, /* fa_pcl_ppi 14 */
	0xc0,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0xa9, /* fa_skin_cr */
	0x67, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_GALLERY_DYNAMIC_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x60, /* ascr_skin_on linear_on strength 0 0 0 0000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x37, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x47, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x25,
	0x3d,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x1c,
	0xd8,
	0xff, /* ascr_skin_Rr */
	0x60, /* ascr_skin_Rg */
	0x82, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf4, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x66, /* ascr_Cr */
	0xdf, /* ascr_Rr */
	0xfc, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xf3, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xf2, /* ascr_Mr */
	0x57, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xf1, /* ascr_Gg */
	0xf4, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xf5, /* ascr_Yr */
	0x2f, /* ascr_Br */
	0xf1, /* ascr_Yg */
	0x32, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xea, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_GALLERY_DYNAMIC_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x07, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x02, /* de_maxplus 11 */
	0x00,
	0x02, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_GALLERY_STANDARD_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xee, /* ascr_Rr */
	0xec, /* ascr_Cg */
	0x28, /* ascr_Rg */
	0xe6, /* ascr_Cb */
	0x2e, /* ascr_Rb */
	0xfd, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x4a, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xe9, /* ascr_Mb */
	0x10, /* ascr_Gb */
	0xee, /* ascr_Yr */
	0x2c, /* ascr_Br */
	0xea, /* ascr_Yg */
	0x30, /* ascr_Bg */
	0x44, /* ascr_Yb */
	0xd4, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_GALLERY_STANDARD_MDNIE_2[] = {
	 /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_GALLERY_NATURAL_MDNIE_1[] ={
	//start
	0xB9, /* Start offset 0x08a8, base B1h */
	0x60, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x8c, /* ascr_Cr */
	0xc2, /* ascr_Rr */
	0xe8, /* ascr_Cg */
	0x1c, /* ascr_Rg */
	0xe2, /* ascr_Cb */
	0x24, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x88, /* ascr_Gr */
	0x3f, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xda, /* ascr_Mb */
	0x46, /* ascr_Gb */
	0xe4, /* ascr_Yr */
	0x26, /* ascr_Br */
	0xde, /* ascr_Yg */
	0x2e, /* ascr_Bg */
	0x4a, /* ascr_Yb */
	0xc8, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_GALLERY_NATURAL_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_GALLERY_MOVIE_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08a8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xe9, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_GALLERY_MOVIE_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x00, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_GALLERY_AUTO_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x6a, /* ascr_skin_cb */
	0x9a, /* ascr_skin_cr */
	0x25, /* ascr_dist_up */
	0x1a, /* ascr_dist_down */
	0x16, /* ascr_dist_right */
	0x2a, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x37,
	0x5a,
	0x00, /* ascr_div_down */
	0x4e,
	0xc5,
	0x00, /* ascr_div_right */
	0x5d,
	0x17,
	0x00, /* ascr_div_left */
	0x30,
	0xc3,
	0xff, /* ascr_skin_Rr */
	0x34, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf9, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xd8, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xd9, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xe0, /* ascr_Rr */
	0xf0, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x3b, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xd9, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x14, /* ascr_Br */
	0xf9, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_GALLERY_AUTO_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x50,
	0x00, /* de_maxplus 11 */
	0x50,
	0x00, /* de_maxminus 11 */
	0x50,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x70,
	0x01, /* fa_pcl_ppi 14 */
	0xc0,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0xa9, /* fa_skin_cr */
	0x67, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VT_DYNAMIC_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 0 0000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x37, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x47, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x25,
	0x3d,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x1c,
	0xd8,
	0xff, /* ascr_skin_Rr */
	0x60, /* ascr_skin_Rg */
	0x82, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf4, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x66, /* ascr_Cr */
	0xdf, /* ascr_Rr */
	0xfc, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xf3, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xf2, /* ascr_Mr */
	0x57, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xf1, /* ascr_Gg */
	0xf4, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xf5, /* ascr_Yr */
	0x2f, /* ascr_Br */
	0xf1, /* ascr_Yg */
	0x32, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xea, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_VT_DYNAMIC_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x07, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x02, /* de_maxplus 11 */
	0x00,
	0x02, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VT_STANDARD_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xee, /* ascr_Rr */
	0xec, /* ascr_Cg */
	0x28, /* ascr_Rg */
	0xe6, /* ascr_Cb */
	0x2e, /* ascr_Rb */
	0xfd, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x4a, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xe9, /* ascr_Mb */
	0x10, /* ascr_Gb */
	0xee, /* ascr_Yr */
	0x2c, /* ascr_Br */
	0xea, /* ascr_Yg */
	0x30, /* ascr_Bg */
	0x44, /* ascr_Yb */
	0xd4, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_VT_STANDARD_MDNIE_2[] = {
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VT_NATURAL_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x8c, /* ascr_Cr */
	0xc2, /* ascr_Rr */
	0xe8, /* ascr_Cg */
	0x1c, /* ascr_Rg */
	0xe2, /* ascr_Cb */
	0x24, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x88, /* ascr_Gr */
	0x3f, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xda, /* ascr_Mb */
	0x46, /* ascr_Gb */
	0xe4, /* ascr_Yr */
	0x26, /* ascr_Br */
	0xde, /* ascr_Yg */
	0x2e, /* ascr_Bg */
	0x4a, /* ascr_Yb */
	0xc8, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_VT_NATURAL_MDNIE_2[] = {
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VT_MOVIE_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xe9, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_VT_MOVIE_MDNIE_2[] = {
	 /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x00, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_VT_AUTO_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xf0, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_VT_AUTO_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x15,
	0x00, /* de_maxplus 11 */
	0xa0,
	0x00, /* de_maxminus 11 */
	0xa0,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x70,
	0x01, /* fa_pcl_ppi 14 */
	0xc0,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0xa9, /* fa_skin_cr */
	0x67, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_BROWSER_DYNAMIC_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 0 0000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x37, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x47, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x25,
	0x3d,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x1c,
	0xd8,
	0xff, /* ascr_skin_Rr */
	0x60, /* ascr_skin_Rg */
	0x82, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf4, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x66, /* ascr_Cr */
	0xdf, /* ascr_Rr */
	0xfc, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xf3, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xf2, /* ascr_Mr */
	0x57, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xf1, /* ascr_Gg */
	0xf4, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xf5, /* ascr_Yr */
	0x2f, /* ascr_Br */
	0xf1, /* ascr_Yg */
	0x32, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xea, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_BROWSER_DYNAMIC_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x07, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x02, /* de_maxplus 11 */
	0x00,
	0x02, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_BROWSER_STANDARD_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xee, /* ascr_Rr */
	0xec, /* ascr_Cg */
	0x28, /* ascr_Rg */
	0xe6, /* ascr_Cb */
	0x2e, /* ascr_Rb */
	0xfd, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x4a, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xe9, /* ascr_Mb */
	0x10, /* ascr_Gb */
	0xee, /* ascr_Yr */
	0x2c, /* ascr_Br */
	0xea, /* ascr_Yg */
	0x30, /* ascr_Bg */
	0x44, /* ascr_Yb */
	0xd4, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_BROWSER_STANDARD_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_BROWSER_NATURAL_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x8c, /* ascr_Cr */
	0xc2, /* ascr_Rr */
	0xe8, /* ascr_Cg */
	0x1c, /* ascr_Rg */
	0xe2, /* ascr_Cb */
	0x24, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x88, /* ascr_Gr */
	0x3f, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xda, /* ascr_Mb */
	0x46, /* ascr_Gb */
	0xe4, /* ascr_Yr */
	0x26, /* ascr_Br */
	0xde, /* ascr_Yg */
	0x2e, /* ascr_Bg */
	0x4a, /* ascr_Yb */
	0xc8, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_BROWSER_NATURAL_MDNIE_2[] = {
	 /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_BROWSER_MOVIE_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xe9, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_BROWSER_MOVIE_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x00, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_BROWSER_AUTO_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x6a, /* ascr_skin_cb */
	0x9a, /* ascr_skin_cr */
	0x25, /* ascr_dist_up */
	0x1a, /* ascr_dist_down */
	0x16, /* ascr_dist_right */
	0x2a, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x37,
	0x5a,
	0x00, /* ascr_div_down */
	0x4e,
	0xc5,
	0x00, /* ascr_div_right */
	0x5d,
	0x17,
	0x00, /* ascr_div_left */
	0x30,
	0xc3,
	0xff, /* ascr_skin_Rr */
	0x34, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf9, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xd8, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xd9, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xe0, /* ascr_Rr */
	0xf0, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x3b, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xd9, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x14, /* ascr_Br */
	0xf9, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_BROWSER_AUTO_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x15,
	0x00, /* de_maxplus 11 */
	0xa0,
	0x00, /* de_maxminus 11 */
	0xa0,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x70,
	0x01, /* fa_pcl_ppi 14 */
	0xc0,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0xa9, /* fa_skin_cr */
	0x67, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_EBOOK_DYNAMIC_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 0 0000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x37, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x47, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x25,
	0x3d,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x1c,
	0xd8,
	0xff, /* ascr_skin_Rr */
	0x60, /* ascr_skin_Rg */
	0x82, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf4, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x66, /* ascr_Cr */
	0xdf, /* ascr_Rr */
	0xfc, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xf3, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xf2, /* ascr_Mr */
	0x57, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xf1, /* ascr_Gg */
	0xf4, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xf5, /* ascr_Yr */
	0x2f, /* ascr_Br */
	0xf1, /* ascr_Yg */
	0x32, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xea, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_EBOOK_DYNAMIC_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x07, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x02, /* de_maxplus 11 */
	0x00,
	0x02, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_EBOOK_STANDARD_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xee, /* ascr_Rr */
	0xec, /* ascr_Cg */
	0x28, /* ascr_Rg */
	0xe6, /* ascr_Cb */
	0x2e, /* ascr_Rb */
	0xfd, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x4a, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xe9, /* ascr_Mb */
	0x10, /* ascr_Gb */
	0xee, /* ascr_Yr */
	0x2c, /* ascr_Br */
	0xea, /* ascr_Yg */
	0x30, /* ascr_Bg */
	0x44, /* ascr_Yb */
	0xd4, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_EBOOK_STANDARD_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_EBOOK_NATURAL_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x8c, /* ascr_Cr */
	0xc2, /* ascr_Rr */
	0xe8, /* ascr_Cg */
	0x1c, /* ascr_Rg */
	0xe2, /* ascr_Cb */
	0x24, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x88, /* ascr_Gr */
	0x3f, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xda, /* ascr_Mb */
	0x46, /* ascr_Gb */
	0xe4, /* ascr_Yr */
	0x26, /* ascr_Br */
	0xde, /* ascr_Yg */
	0x2e, /* ascr_Bg */
	0x4a, /* ascr_Yb */
	0xc8, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_EBOOK_NATURAL_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_EBOOK_MOVIE_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xf8, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xe9, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_EBOOK_MOVIE_MDNIE_2[] = {
	 /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_EBOOK_AUTO_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xec, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xf0, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xec, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_EBOOK_AUTO_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x15,
	0x00, /* de_maxplus 11 */
	0xa0,
	0x00, /* de_maxminus 11 */
	0xa0,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x70,
	0x01, /* fa_pcl_ppi 14 */
	0xc0,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0xa9, /* fa_skin_cr */
	0x67, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_EMAIL_AUTO_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x00, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x0c, /* ascr_dist_up */
	0x0c, /* ascr_dist_down */
	0x0c, /* ascr_dist_right */
	0x0c, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0xaa,
	0xab,
	0x00, /* ascr_div_down */
	0xaa,
	0xab,
	0x00, /* ascr_div_right */
	0xaa,
	0xab,
	0x00, /* ascr_div_left */
	0xaa,
	0xab,
	0xd5, /* ascr_skin_Rr */
	0x2c, /* ascr_skin_Rg */
	0x2a, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf5, /* ascr_skin_Yg */
	0x63, /* ascr_skin_Yb */
	0xfe, /* ascr_skin_Mr */
	0x4a, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf9, /* ascr_skin_Wg */
	0xec, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xf0, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf9, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xec, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};
static char DSI0_EMAIL_AUTO_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x30, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static unsigned char DSI0_TDMB_DYNAMIC_MDNIE_1[] = {
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 0 0000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x37, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x47, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x25,
	0x3d,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x1c,
	0xd8,
	0xff, /* ascr_skin_Rr */
	0x60, /* ascr_skin_Rg */
	0x82, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf4, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x66, /* ascr_Cr */
	0xdf, /* ascr_Rr */
	0xfc, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xf3, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xf2, /* ascr_Mr */
	0x57, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xf1, /* ascr_Gg */
	0xf4, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xf5, /* ascr_Yr */
	0x2f, /* ascr_Br */
	0xf1, /* ascr_Yg */
	0x32, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xea, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xfc, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xf6, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_TDMB_DYNAMIC_MDNIE_2[] ={
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x07, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x10,
	0x02, /* de_maxplus 11 */
	0x00,
	0x02, /* de_maxminus 11 */
	0x00,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x00, /* curve_5_b */
	0x20, /* curve_5_a */
	0x00, /* curve_6_b */
	0x20, /* curve_6_a */
	0x00, /* curve_7_b */
	0x20, /* curve_7_a */
	0x00, /* curve_8_b */
	0x20, /* curve_8_a */
	0x00, /* curve_9_b */
	0x20, /* curve_9_a */
	0x00, /* curve10_b */
	0x20, /* curve10_a */
	0x00, /* curve11_b */
	0x20, /* curve11_a */
	0x00, /* curve12_b */
	0x20, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static unsigned char DSI0_TDMB_STANDARD_MDNIE_1[] = {
	0xB9, /* Start offset 0x08A8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xee, /* ascr_Rr */
	0xec, /* ascr_Cg */
	0x28, /* ascr_Rg */
	0xe6, /* ascr_Cb */
	0x2e, /* ascr_Rb */
	0xfd, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x4a, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xe9, /* ascr_Mb */
	0x10, /* ascr_Gb */
	0xee, /* ascr_Yr */
	0x2c, /* ascr_Br */
	0xea, /* ascr_Yg */
	0x30, /* ascr_Bg */
	0x44, /* ascr_Yb */
	0xd4, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static unsigned char DSI0_TDMB_STANDARD_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static unsigned char DSI0_TDMB_NATURAL_MDNIE_1[] = {
	0xB9, /* Start offset 0x08a8, base B1h */
	0x20, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x50, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x8c, /* ascr_Cr */
	0xc2, /* ascr_Rr */
	0xe8, /* ascr_Cg */
	0x1c, /* ascr_Rg */
	0xe2, /* ascr_Cb */
	0x24, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x88, /* ascr_Gr */
	0x3f, /* ascr_Mg */
	0xda, /* ascr_Gg */
	0xda, /* ascr_Mb */
	0x46, /* ascr_Gb */
	0xe4, /* ascr_Yr */
	0x26, /* ascr_Br */
	0xde, /* ascr_Yg */
	0x2e, /* ascr_Bg */
	0x4a, /* ascr_Yb */
	0xc8, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xf7, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xef, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static unsigned char DSI0_TDMB_NATURAL_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x00,
	0x07, /* de_maxplus 11 */
	0xff,
	0x07, /* de_maxminus 11 */
	0xff,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x0a,
	0x00, /* fa_step_n 10 */
	0x32,
	0x01, /* fa_max_de_gain 10 */
	0xf4,
	0x0b, /* fa_pcl_ppi 14 */
	0x8a,
	0x20, /* fa_os_cnt_10_co */
	0x2d, /* fa_os_cnt_20_co */
	0x6e, /* fa_skin_cr */
	0x99, /* fa_skin_cb */
	0x1b, /* fa_dist_left */
	0x17, /* fa_dist_right */
	0x14, /* fa_dist_down */
	0x1e, /* fa_dist_up */
	0x02, /* fa_div_dist_left */
	0x5f,
	0x02, /* fa_div_dist_right */
	0xc8,
	0x03, /* fa_div_dist_down */
	0x33,
	0x02, /* fa_div_dist_up */
	0x22,
	0x10, /* fa_px_min_weight */
	0x10, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};
static unsigned char DSI0_TDMB_AUTO_MDNIE_1[] = {
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x6a, /* ascr_skin_cb */
	0x9a, /* ascr_skin_cr */
	0x25, /* ascr_dist_up */
	0x1a, /* ascr_dist_down */
	0x16, /* ascr_dist_right */
	0x2a, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x37,
	0x5a,
	0x00, /* ascr_div_down */
	0x4e,
	0xc5,
	0x00, /* ascr_div_right */
	0x5d,
	0x17,
	0x00, /* ascr_div_left */
	0x30,
	0xc3,
	0xff, /* ascr_skin_Rr */
	0x34, /* ascr_skin_Rg */
	0x60, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xf9, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xd8, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xd9, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xff, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xe0, /* ascr_Rr */
	0xf0, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xd8, /* ascr_Mr */
	0x3b, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xd9, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x14, /* ascr_Br */
	0xf9, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static unsigned char DSI0_TDMB_AUTO_MDNIE_2[] = {
  /* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x50,
	0x00, /* de_maxplus 11 */
	0x50,
	0x00, /* de_maxminus 11 */
	0x50,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x70,
	0x01, /* fa_pcl_ppi 14 */
	0xc0,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0xa9, /* fa_skin_cr */
	0x67, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static char DSI0_RGB_SENSOR_MDNIE_1[] = {
	//start
	0xB9, /* Start offset 0x08A8, base B1h */
	0x50, /* ascr_skin_on linear_on strength 0 0 00000 */
	0x67, /* ascr_skin_cb */
	0xa9, /* ascr_skin_cr */
	0x17, /* ascr_dist_up */
	0x29, /* ascr_dist_down */
	0x19, /* ascr_dist_right */
	0x27, /* ascr_dist_left */
	0x00, /* ascr_div_up 20 */
	0x59,
	0x0b,
	0x00, /* ascr_div_down */
	0x31,
	0xf4,
	0x00, /* ascr_div_right */
	0x51,
	0xec,
	0x00, /* ascr_div_left */
	0x34,
	0x83,
	0xff, /* ascr_skin_Rr */
	0x58, /* ascr_skin_Rg */
	0x7b, /* ascr_skin_Rb */
	0xff, /* ascr_skin_Yr */
	0xff, /* ascr_skin_Yg */
	0x00, /* ascr_skin_Yb */
	0xff, /* ascr_skin_Mr */
	0x00, /* ascr_skin_Mg */
	0xff, /* ascr_skin_Mb */
	0xff, /* ascr_skin_Wr */
	0xf8, /* ascr_skin_Wg */
	0xff, /* ascr_skin_Wb */
	0x00, /* ascr_Cr */
	0xff, /* ascr_Rr */
	0xff, /* ascr_Cg */
	0x00, /* ascr_Rg */
	0xff, /* ascr_Cb */
	0x00, /* ascr_Rb */
	0xff, /* ascr_Mr */
	0x00, /* ascr_Gr */
	0x00, /* ascr_Mg */
	0xff, /* ascr_Gg */
	0xff, /* ascr_Mb */
	0x00, /* ascr_Gb */
	0xff, /* ascr_Yr */
	0x00, /* ascr_Br */
	0xff, /* ascr_Yg */
	0x00, /* ascr_Bg */
	0x00, /* ascr_Yb */
	0xff, /* ascr_Bb */
	0xff, /* ascr_Wr */
	0x00, /* ascr_Kr */
	0xff, /* ascr_Wg */
	0x00, /* ascr_Kg */
	0xff, /* ascr_Wb */
	0x00, /* ascr_Kb */
	/* end */
};

static char DSI0_RGB_SENSOR_MDNIE_2[] = {
	/* start */
	0xB9, /* Start offset 0x0842, base B1h */
	0x01, /* mdnie_en */
	0x00, /* data_width mask 00 0000 */
	0x3c, /* ascr algo lce 10 10 10 */
	0x98, /* lce_on gain 0 00 0000 */
	0x24, /* lce_color_gain 00 0000 */
	0x00, /* lce_scene_trans 0000 */
	0xb3, /* lce_illum_gain */
	0x01, /* lce_ref_offset 9 */
	0x0e,
	0x01, /* lce_ref_gain 9 */
	0x00,
	0x66, /* lce_block_size h v 000 000 */
	0x17, /* lce_black reduct_slope 0 0000 */
	0x03, /* lce_dark_th 000 */
	0x96, /* lce_min_ref_offset */
	0x00, /* nr fa de cs gamma 0 0000 */
	0xff, /* nr_mask_th */
	0x00, /* de_gain 10 */
	0x15,
	0x00, /* de_maxplus 11 */
	0xa0,
	0x00, /* de_maxminus 11 */
	0xa0,
	0x14, /* fa_edge_th */
	0x00, /* fa_step_p 10 */
	0x01,
	0x00, /* fa_step_n 10 */
	0x01,
	0x00, /* fa_max_de_gain 10 */
	0x70,
	0x01, /* fa_pcl_ppi 14 */
	0xc0,
	0x28, /* fa_os_cnt_10_co */
	0x3c, /* fa_os_cnt_20_co */
	0xa9, /* fa_skin_cr */
	0x67, /* fa_skin_cb */
	0x27, /* fa_dist_left */
	0x19, /* fa_dist_right */
	0x29, /* fa_dist_down */
	0x17, /* fa_dist_up */
	0x01, /* fa_div_dist_left */
	0xa4,
	0x02, /* fa_div_dist_right */
	0x8f,
	0x01, /* fa_div_dist_down */
	0x90,
	0x02, /* fa_div_dist_up */
	0xc8,
	0x20, /* fa_px_min_weight */
	0x20, /* fa_fr_min_weight */
	0x07, /* fa_skin_zone_w */
	0x07, /* fa_skin_zone_h */
	0x01, /* cs_gain 10 */
	0x00,
	0x00, /* curve_1_b */
	0x20, /* curve_1_a */
	0x00, /* curve_2_b */
	0x20, /* curve_2_a */
	0x00, /* curve_3_b */
	0x20, /* curve_3_a */
	0x00, /* curve_4_b */
	0x20, /* curve_4_a */
	0x02, /* curve_5_b */
	0x1b, /* curve_5_a */
	0x02, /* curve_6_b */
	0x1b, /* curve_6_a */
	0x02, /* curve_7_b */
	0x1b, /* curve_7_a */
	0x02, /* curve_8_b */
	0x1b, /* curve_8_a */
	0x09, /* curve_9_b */
	0xa6, /* curve_9_a */
	0x09, /* curve10_b */
	0xa6, /* curve10_a */
	0x09, /* curve11_b */
	0xa6, /* curve11_a */
	0x09, /* curve12_b */
	0xa6, /* curve12_a */
	0x00, /* curve13_b */
	0x20, /* curve13_a */
	0x00, /* curve14_b */
	0x20, /* curve14_a */
	0x00, /* curve15_b */
	0x20, /* curve15_a */
	0x00, /* curve16_b */
	0x20, /* curve16_a */
	0x00, /* curve17_b */
	0x20, /* curve17_a */
	0x00, /* curve18_b */
	0x20, /* curve18_a */
	0x00, /* curve19_b */
	0x20, /* curve19_a */
	0x00, /* curve20_b */
	0x20, /* curve20_a */
	0x00, /* curve21_b */
	0x20, /* curve21_a */
	0x00, /* curve22_b */
	0x20, /* curve22_a */
	0x00, /* curve23_b */
	0x20, /* curve23_a */
	0x00, /* curve24_b */
	0xFF, /* curve24_a */
	/* end */
};

static struct dsi_cmd_desc DSI0_BYPASS_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_1), DSI0_BYPASS_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_2), DSI0_BYPASS_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_NEGATIVE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_NEGATIVE_MDNIE_1), DSI0_NEGATIVE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_NEGATIVE_MDNIE_2), DSI0_NEGATIVE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_GRAYSCALE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GRAYSCALE_MDNIE_1), DSI0_GRAYSCALE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GRAYSCALE_MDNIE_2), DSI0_GRAYSCALE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_GRAYSCALE_NEGATIVE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GRAYSCALE_NEGATIVE_MDNIE_1), DSI0_GRAYSCALE_NEGATIVE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GRAYSCALE_NEGATIVE_MDNIE_2), DSI0_GRAYSCALE_NEGATIVE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_COLOR_BLIND_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_COLOR_BLIND_MDNIE_1), DSI0_COLOR_BLIND_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_COLOR_BLIND_MDNIE_2), DSI0_COLOR_BLIND_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_NIGHT_MODE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_NIGHT_MODE_MDNIE_1), DSI0_NIGHT_MODE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_NIGHT_MODE_MDNIE_2), DSI0_NIGHT_MODE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_COLOR_LENS_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_COLOR_LENS_MDNIE_1), DSI0_COLOR_LENS_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_COLOR_LENS_MDNIE_2), DSI0_COLOR_LENS_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_LIGHT_NOTIFICATION_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_LIGHT_NOTIFICATION_MDNIE_1), DSI0_LIGHT_NOTIFICATION_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_LIGHT_NOTIFICATION_MDNIE_2), DSI0_LIGHT_NOTIFICATION_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_HBM_CE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_1), DSI0_HBM_CE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_2), DSI0_HBM_CE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_HBM_CE_TEXT_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_TEXT_MDNIE_1), DSI0_HBM_CE_TEXT_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_TEXT_MDNIE_2), DSI0_HBM_CE_TEXT_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_RGB_SENSOR_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_RGB_SENSOR_MDNIE_1), DSI0_RGB_SENSOR_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_RGB_SENSOR_MDNIE_2), DSI0_RGB_SENSOR_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_CURTAIN[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_SCREEN_CURTAIN_MDNIE_1), DSI0_SCREEN_CURTAIN_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_SCREEN_CURTAIN_MDNIE_2), DSI0_SCREEN_CURTAIN_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_HDR_VIDEO_1_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HDR_VIDEO_1_MDNIE_1), DSI0_HDR_VIDEO_1_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HDR_VIDEO_1_MDNIE_2), DSI0_HDR_VIDEO_1_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_HDR_VIDEO_2_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HDR_VIDEO_2_MDNIE_1), DSI0_HDR_VIDEO_2_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HDR_VIDEO_2_MDNIE_2), DSI0_HDR_VIDEO_2_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_HDR_VIDEO_3_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HDR_VIDEO_3_MDNIE_1), DSI0_HDR_VIDEO_3_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HDR_VIDEO_3_MDNIE_2), DSI0_HDR_VIDEO_3_MDNIE_2, 0, NULL}, false, 0},
};

///////////////////////////////////////////////////////////////////////////////////

static struct dsi_cmd_desc DSI0_UI_DYNAMIC_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_1), DSI0_UI_DYNAMIC_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_2), DSI0_UI_DYNAMIC_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_UI_STANDARD_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_STANDARD_MDNIE_1), DSI0_UI_STANDARD_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_STANDARD_MDNIE_2), DSI0_UI_STANDARD_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_UI_NATURAL_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_NATURAL_MDNIE_1), DSI0_UI_NATURAL_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_NATURAL_MDNIE_2), DSI0_UI_NATURAL_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_UI_MOVIE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_MOVIE_MDNIE_1), DSI0_UI_MOVIE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_MOVIE_MDNIE_2), DSI0_UI_MOVIE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_UI_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_1), DSI0_UI_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_2), DSI0_UI_AUTO_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VIDEO_OUTDOOR_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_OUTDOOR_MDNIE_1), DSI0_VIDEO_OUTDOOR_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_OUTDOOR_MDNIE_2), DSI0_VIDEO_OUTDOOR_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VIDEO_DYNAMIC_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_DYNAMIC_MDNIE_1), DSI0_VIDEO_DYNAMIC_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_DYNAMIC_MDNIE_2), DSI0_VIDEO_DYNAMIC_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VIDEO_STANDARD_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_STANDARD_MDNIE_1), DSI0_VIDEO_STANDARD_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_STANDARD_MDNIE_2), DSI0_VIDEO_STANDARD_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VIDEO_NATURAL_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_NATURAL_MDNIE_1), DSI0_VIDEO_NATURAL_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_NATURAL_MDNIE_2), DSI0_VIDEO_NATURAL_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VIDEO_MOVIE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_MOVIE_MDNIE_1), DSI0_VIDEO_MOVIE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_MOVIE_MDNIE_2), DSI0_VIDEO_MOVIE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VIDEO_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_AUTO_MDNIE_1), DSI0_VIDEO_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VIDEO_AUTO_MDNIE_2), DSI0_VIDEO_AUTO_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_CAMERA_OUTDOOR_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_OUTDOOR_MDNIE_1), DSI0_CAMERA_OUTDOOR_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_OUTDOOR_MDNIE_2), DSI0_CAMERA_OUTDOOR_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_CAMERA_DYNAMIC_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_DYNAMIC_MDNIE_1), DSI0_CAMERA_DYNAMIC_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_DYNAMIC_MDNIE_2), DSI0_CAMERA_DYNAMIC_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_CAMERA_STANDARD_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_STANDARD_MDNIE_1), DSI0_CAMERA_STANDARD_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_STANDARD_MDNIE_2), DSI0_CAMERA_STANDARD_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_CAMERA_NATURAL_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_NATURAL_MDNIE_1), DSI0_CAMERA_NATURAL_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_NATURAL_MDNIE_2), DSI0_CAMERA_NATURAL_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_CAMERA_MOVIE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_MOVIE_MDNIE_1), DSI0_CAMERA_MOVIE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_MOVIE_MDNIE_2), DSI0_CAMERA_MOVIE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_CAMERA_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_AUTO_MDNIE_1), DSI0_CAMERA_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_CAMERA_AUTO_MDNIE_2), DSI0_CAMERA_AUTO_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_GALLERY_DYNAMIC_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_DYNAMIC_MDNIE_1), DSI0_GALLERY_DYNAMIC_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_DYNAMIC_MDNIE_2), DSI0_GALLERY_DYNAMIC_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_GALLERY_STANDARD_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_STANDARD_MDNIE_1), DSI0_GALLERY_STANDARD_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_STANDARD_MDNIE_2), DSI0_GALLERY_STANDARD_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_GALLERY_NATURAL_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_NATURAL_MDNIE_1), DSI0_GALLERY_NATURAL_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_NATURAL_MDNIE_2), DSI0_GALLERY_NATURAL_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_GALLERY_MOVIE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_MOVIE_MDNIE_1), DSI0_GALLERY_MOVIE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_MOVIE_MDNIE_2), DSI0_GALLERY_MOVIE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_GALLERY_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_AUTO_MDNIE_1), DSI0_GALLERY_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_GALLERY_AUTO_MDNIE_2), DSI0_GALLERY_AUTO_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VT_DYNAMIC_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_DYNAMIC_MDNIE_1), DSI0_VT_DYNAMIC_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_DYNAMIC_MDNIE_2), DSI0_VT_DYNAMIC_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VT_STANDARD_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_STANDARD_MDNIE_1), DSI0_VT_STANDARD_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_STANDARD_MDNIE_2), DSI0_VT_STANDARD_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VT_NATURAL_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_NATURAL_MDNIE_1), DSI0_VT_NATURAL_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_NATURAL_MDNIE_2), DSI0_VT_NATURAL_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VT_MOVIE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_MOVIE_MDNIE_1), DSI0_VT_MOVIE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_MOVIE_MDNIE_2), DSI0_VT_MOVIE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_VT_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_AUTO_MDNIE_1), DSI0_VT_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_VT_AUTO_MDNIE_2), DSI0_VT_AUTO_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_BROWSER_DYNAMIC_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_DYNAMIC_MDNIE_1), DSI0_BROWSER_DYNAMIC_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_DYNAMIC_MDNIE_2), DSI0_BROWSER_DYNAMIC_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_BROWSER_STANDARD_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_STANDARD_MDNIE_1), DSI0_BROWSER_STANDARD_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_STANDARD_MDNIE_2), DSI0_BROWSER_STANDARD_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_BROWSER_NATURAL_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_NATURAL_MDNIE_1), DSI0_BROWSER_NATURAL_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_NATURAL_MDNIE_2), DSI0_BROWSER_NATURAL_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_BROWSER_MOVIE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_MOVIE_MDNIE_1), DSI0_BROWSER_MOVIE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_MOVIE_MDNIE_2), DSI0_BROWSER_MOVIE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_BROWSER_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_AUTO_MDNIE_1), DSI0_BROWSER_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BROWSER_AUTO_MDNIE_2), DSI0_BROWSER_AUTO_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_EBOOK_DYNAMIC_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_DYNAMIC_MDNIE_1), DSI0_EBOOK_DYNAMIC_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_DYNAMIC_MDNIE_2), DSI0_EBOOK_DYNAMIC_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_EBOOK_STANDARD_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_STANDARD_MDNIE_1), DSI0_EBOOK_STANDARD_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_STANDARD_MDNIE_2), DSI0_EBOOK_STANDARD_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_EBOOK_NATURAL_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_NATURAL_MDNIE_1), DSI0_EBOOK_NATURAL_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_NATURAL_MDNIE_2), DSI0_EBOOK_NATURAL_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_EBOOK_MOVIE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_MOVIE_MDNIE_1), DSI0_EBOOK_MOVIE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_MOVIE_MDNIE_2), DSI0_EBOOK_MOVIE_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_EBOOK_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_AUTO_MDNIE_1), DSI0_EBOOK_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EBOOK_AUTO_MDNIE_2), DSI0_EBOOK_AUTO_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_EMAIL_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EMAIL_AUTO_MDNIE_1), DSI0_EMAIL_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_EMAIL_AUTO_MDNIE_2), DSI0_EMAIL_AUTO_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_TDMB_DYNAMIC_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_TDMB_DYNAMIC_MDNIE_1), DSI0_TDMB_DYNAMIC_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_TDMB_DYNAMIC_MDNIE_2), DSI0_TDMB_DYNAMIC_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_TDMB_STANDARD_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_TDMB_STANDARD_MDNIE_1), DSI0_TDMB_STANDARD_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_TDMB_STANDARD_MDNIE_2), DSI0_TDMB_STANDARD_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_TDMB_NATURAL_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_TDMB_NATURAL_MDNIE_1), DSI0_TDMB_NATURAL_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_TDMB_NATURAL_MDNIE_2), DSI0_TDMB_NATURAL_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc DSI0_TDMB_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_1), mdnie_address_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_TDMB_AUTO_MDNIE_1), DSI0_TDMB_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, 0, sizeof(mdnie_address_2), mdnie_address_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_TDMB_AUTO_MDNIE_2), DSI0_TDMB_AUTO_MDNIE_2, 0, NULL}, false, 0},
};

static struct dsi_cmd_desc *mdnie_tune_value_dsi0[MAX_APP_MODE][MAX_MODE][MAX_OUTDOOR_MODE] = {
                /*
			DYNAMIC_MODE
			STANDARD_MODE
			NATURAL_MODE
			MOVIE_MODE
			AUTO_MODE
			READING_MODE
		*/
		// UI_APP
		{
			{DSI0_UI_DYNAMIC_MDNIE,	NULL},
			{DSI0_UI_STANDARD_MDNIE,	NULL},
			{DSI0_UI_NATURAL_MDNIE,	NULL},
			{DSI0_UI_MOVIE_MDNIE,	NULL},
			{DSI0_UI_AUTO_MDNIE,	NULL},
			{DSI0_EBOOK_AUTO_MDNIE,	NULL},
		},
		// VIDEO_APP
		{
			{DSI0_VIDEO_DYNAMIC_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_STANDARD_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_NATURAL_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_MOVIE_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_AUTO_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_EBOOK_AUTO_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
		},
		// VIDEO_WARM_APP
		{
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_EBOOK_AUTO_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
		},
		// VIDEO_COLD_APP
		{
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_VIDEO_OUTDOOR_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
			{DSI0_EBOOK_AUTO_MDNIE,	DSI0_VIDEO_OUTDOOR_MDNIE},
		},
		// CAMERA_APP
		{
			{DSI0_CAMERA_DYNAMIC_MDNIE,	DSI0_CAMERA_OUTDOOR_MDNIE},
			{DSI0_CAMERA_STANDARD_MDNIE,	DSI0_CAMERA_OUTDOOR_MDNIE},
			{DSI0_CAMERA_NATURAL_MDNIE,	DSI0_CAMERA_OUTDOOR_MDNIE},
			{DSI0_CAMERA_MOVIE_MDNIE,	DSI0_CAMERA_OUTDOOR_MDNIE},
			{DSI0_CAMERA_AUTO_MDNIE,	DSI0_CAMERA_OUTDOOR_MDNIE},
			{DSI0_EBOOK_AUTO_MDNIE,	DSI0_CAMERA_OUTDOOR_MDNIE},
		},
		// NAVI_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// GALLERY_APP
		{
			{DSI0_GALLERY_DYNAMIC_MDNIE,	NULL},
			{DSI0_GALLERY_STANDARD_MDNIE,	NULL},
			{DSI0_GALLERY_NATURAL_MDNIE,	NULL},
			{DSI0_GALLERY_MOVIE_MDNIE,	NULL},
			{DSI0_GALLERY_AUTO_MDNIE,	NULL},
			{DSI0_EBOOK_AUTO_MDNIE,	NULL},
		},
		// VT_APP
		{
			{DSI0_VT_DYNAMIC_MDNIE,	NULL},
			{DSI0_VT_STANDARD_MDNIE,	NULL},
			{DSI0_VT_NATURAL_MDNIE,	NULL},
			{DSI0_VT_MOVIE_MDNIE,	NULL},
			{DSI0_VT_AUTO_MDNIE,	NULL},
			{DSI0_EBOOK_AUTO_MDNIE,	NULL},
		},
		// BROWSER_APP
		{
			{DSI0_BROWSER_DYNAMIC_MDNIE,	NULL},
			{DSI0_BROWSER_STANDARD_MDNIE,	NULL},
			{DSI0_BROWSER_NATURAL_MDNIE,	NULL},
			{DSI0_BROWSER_MOVIE_MDNIE,	NULL},
			{DSI0_BROWSER_AUTO_MDNIE,	NULL},
			{DSI0_EBOOK_AUTO_MDNIE,	NULL},
		},
		// eBOOK_APP
		{
			{DSI0_EBOOK_DYNAMIC_MDNIE,	NULL},
			{DSI0_EBOOK_STANDARD_MDNIE,NULL},
			{DSI0_EBOOK_NATURAL_MDNIE,	NULL},
			{DSI0_EBOOK_MOVIE_MDNIE,	NULL},
			{DSI0_EBOOK_AUTO_MDNIE,	NULL},
			{DSI0_EBOOK_AUTO_MDNIE,	NULL},
		},
		// EMAIL_APP
		{
			{DSI0_EMAIL_AUTO_MDNIE,	NULL},
			{DSI0_EMAIL_AUTO_MDNIE,	NULL},
			{DSI0_EMAIL_AUTO_MDNIE,	NULL},
			{DSI0_EMAIL_AUTO_MDNIE,	NULL},
			{DSI0_EMAIL_AUTO_MDNIE,	NULL},
			{DSI0_EBOOK_AUTO_MDNIE,	NULL},
		},
		// TDMB_APP
		{
			{DSI0_TDMB_DYNAMIC_MDNIE,       NULL},
			{DSI0_TDMB_STANDARD_MDNIE,      NULL},
			{DSI0_TDMB_NATURAL_MDNIE,       NULL},
			{DSI0_TDMB_NATURAL_MDNIE,         NULL},
			{DSI0_TDMB_AUTO_MDNIE,          NULL},
			{DSI0_EBOOK_AUTO_MDNIE,	NULL},
		},
};

static struct dsi_cmd_desc *light_notification_tune_value_dsi0[LIGHT_NOTIFICATION_MAX] = {
	NULL,
	DSI0_LIGHT_NOTIFICATION_MDNIE,
};

static struct dsi_cmd_desc *hdr_tune_value_dsi0[HDR_MAX] = {
	NULL,
	DSI0_HDR_VIDEO_1_MDNIE,
	DSI0_HDR_VIDEO_2_MDNIE,
	DSI0_HDR_VIDEO_3_MDNIE,
};


#define DSI0_RGB_SENSOR_MDNIE_1_SIZE ARRAY_SIZE(DSI0_RGB_SENSOR_MDNIE_1)
#define DSI0_RGB_SENSOR_MDNIE_2_SIZE ARRAY_SIZE(DSI0_RGB_SENSOR_MDNIE_2)

#endif
