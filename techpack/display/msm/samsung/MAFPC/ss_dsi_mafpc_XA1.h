/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * DDI mAFPC operation
 * Author: Samsung Display Driver Team <cj1225.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SS_DSI_MAFPC_XA1_H__
#define __SS_DSI_MAFPC_XA1_H__

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include "ss_dsi_mafpc_common.h"

int ss_mafpc_init_XA1(struct samsung_display_driver_data *vdd);

#define MAFPC_ENABLE_COMMAND_LEN_XA1 39
#define MAFPC_BRIGHTNESS_SCALE_STEP 75
#define MAFPC_BRIGHTNESS_SCALE_CMD 3

/* 256 (Normal: 0~255) + 1 (HBM) */
#define MAX_MAFPC_BL_SCALE	257

static int brightness_scale_idx[MAX_MAFPC_BL_SCALE] = {
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	10,
	11,
	12,
	13,
	13,
	14,
	14,
	15,
	16,
	17,
	18,
	18,
	19,
	19,
	20,
	21,
	22,
	22,
	22,
	23,
	23,
	24,
	24,
	25,
	25,
	26,
	26,
	27,
	27,
	27,
	28,
	28,
	28,
	29,
	29,
	30,
	30,
	30,
	30,
	31,
	31,
	31,
	32,
	32,
	32,
	32,
	33,
	33,
	33,
	34,
	34,
	34,
	34,
	35,
	35,
	35,
	35,
	36,
	36,
	36,
	36,
	36,
	37,
	37,
	37,
	37,
	38,
	38,
	38,
	38,
	38,
	38,
	39,
	39,
	39,
	39,
	39,
	40,
	40,
	40,
	40,
	40,
	41,
	41,
	41,
	41,
	41,
	41,
	41,
	42,
	42,
	42,
	42,
	42,
	42,
	43,
	43,
	43,
	43,
	43,
	43,
	43,
	44,
	44,
	44,
	44,
	44,
	44,
	44,
	45,
	45,
	45,
	45,
	45,
	45,
	45,
	45,
	46,
	46,
	46,
	46,
	46,
	46,
	47,
	47,
	47,
	47,
	47,
	47,
	47,
	48,
	48,
	48,
	48,
	48,
	48,
	48,
	49,
	49,
	49,
	49,
	49,
	49,
	49,
	49,
	50,
	50,
	50,
	50,
	51,
	51,
	51,
	51,
	52,
	52,
	52,
	52,
	53,
	53,
	53,
	53,
	53,
	54,
	54,
	54,
	54,
	55,
	55,
	55,
	55,
	55,
	56,
	56,
	56,
	56,
	56,
	56,
	56,
	56,
	56,
	56,
	57,
	57,
	57,
	57,
	57,
	57,
	57,
	57,
	58,
	58,
	58,
	58,
	58,
	58,
	58,
	58,
	58,
	58,
	59,
	59,
	59,
	59,
	59,
	59,
	59,
	59,
	60,
	61,
	61,
	62,
	62,
	63,
	63,
	64,
	64,
	65,
	65,
	66,
	66,
	67,
	67,
	68,
	68,
	69,
	69,
	69,
	69,
	69,
	70,
	70,
	70,
	70,
	70,
	71,
	71,
	71,
	71,
	71,
	72,
	72,
	72,
	72,
	72,
	73,
	74,
};

/* 0~73 for Normal, 74 for HBM */
static char brightness_scale_table[MAFPC_BRIGHTNESS_SCALE_STEP][MAFPC_BRIGHTNESS_SCALE_CMD] = {
	{0xFF, 0xFF, 0xFF},	/* 0 */
	{0xFF, 0xFF, 0xFF},	/* 1 */
	{0xFF, 0xFF, 0xFF},	/* 2 */
	{0xFF, 0xFF, 0xFF},	/* 3 */
	{0xFF, 0xFF, 0xFF},	/* 4 */
	{0xFF, 0xFF, 0xFF},	/* 5 */
	{0xFF, 0xFF, 0xFF},	/* 6 */
	{0xFF, 0xFF, 0xFF},	/* 7 */
	{0xFF, 0xFF, 0xFF},	/* 8 */
	{0xFF, 0xFF, 0xFF},	/* 9 */
	{0xFF, 0xFF, 0xFF},	/* 10 */
	{0xFF, 0xFF, 0xFF},	/* 11 */
	{0xFF, 0xFF, 0xFF},	/* 12 */
	{0xFF, 0xFF, 0xFF},	/* 13 */
	{0xFF, 0xFF, 0xFF},	/* 14 */
	{0xFF, 0xFF, 0xFF},	/* 15 */
	{0xFF, 0xFF, 0xFF},	/* 16 */
	{0xFF, 0xFF, 0xFF},	/* 17 */
	{0xFF, 0xFF, 0xFF},	/* 18 */
	{0xFF, 0xFF, 0xFF},	/* 19 */
	{0xFF, 0xFF, 0xFF},	/* 20 */
	{0xFF, 0xFF, 0xFF},	/* 21 */
	{0xFF, 0xFF, 0xFF},	/* 22 */
	{0xFF, 0xFF, 0xFF},	/* 23 */
	{0xFF, 0xFF, 0xFF},	/* 24 */
	{0xFF, 0xFF, 0xFF},	/* 25 */
	{0xFF, 0xFF, 0xFF},	/* 26 */
	{0xFF, 0xFF, 0xFF},	/* 27 */
	{0xFF, 0xFF, 0xFF},	/* 28 */
	{0xFF, 0xFF, 0xFF},	/* 29 */
	{0xFF, 0xFF, 0xFF},	/* 30 */
	{0xFF, 0xFF, 0xFF},	/* 31 */
	{0xFF, 0xFF, 0xFF},	/* 32 */
	{0xFF, 0xFF, 0xFF},	/* 33 */
	{0xFF, 0xFF, 0xFF},	/* 34 */
	{0xFF, 0xFF, 0xFF},	/* 35 */
	{0xFF, 0xFF, 0xFF},	/* 36 */
	{0xFF, 0xFF, 0xFF},	/* 37 */
	{0xFF, 0xFF, 0xFF},	/* 38 */
	{0xFF, 0xFF, 0xFF},	/* 39 */
	{0xFF, 0xFF, 0xFF},	/* 40 */
	{0xFF, 0xFF, 0xFF},	/* 41 */
	{0xFF, 0xFF, 0xFF},	/* 42 */
	{0xFF, 0xFF, 0xFF},	/* 43 */
	{0xFF, 0xFF, 0xFF},	/* 44 */
	{0xFF, 0xFF, 0xFF},	/* 45 */
	{0xFF, 0xFF, 0xFF},	/* 46 */
	{0xFF, 0xFF, 0xFF},	/* 47 */
	{0xFF, 0xFF, 0xFF},	/* 48 */
	{0xFF, 0xFF, 0xFF},	/* 49 */
	{0xFF, 0xFF, 0xFF},	/* 50 */
	{0xFF, 0xFF, 0xFF},	/* 51 */
	{0xFF, 0xFF, 0xFF},	/* 52 */
	{0xFF, 0xFF, 0xFF},	/* 53 */
	{0xFF, 0xFF, 0xFF},	/* 54 */
	{0xFF, 0xFF, 0xFF},	/* 55 */
	{0xFF, 0xFF, 0xFF},	/* 56 */
	{0xFF, 0xFF, 0xFF},	/* 57 */
	{0xFF, 0xFF, 0xFF},	/* 58 */
	{0xFF, 0xFF, 0xFF},	/* 59 */
	{0xFF, 0xFF, 0xFF},	/* 60 */
	{0xFF, 0xFF, 0xFF},	/* 61 */
	{0xFF, 0xFF, 0xFF},	/* 62 */
	{0xFF, 0xFF, 0xFF},	/* 63 */
	{0xFF, 0xFF, 0xFF},	/* 64 */
	{0xFF, 0xFF, 0xFF},	/* 65 */
	{0xFF, 0xFF, 0xFF},	/* 66 */
	{0xFF, 0xFF, 0xFF},	/* 67 */
	{0xFF, 0xFF, 0xFF},	/* 68 */
	{0xFF, 0xFF, 0xFF},	/* 69 */
	{0xFF, 0xFF, 0xFF},	/* 70 */
	{0xFF, 0xFF, 0xFF},	/* 71 */
	{0xFF, 0xFF, 0xFF},	/* 72 */
	{0xFF, 0xFF, 0xFF},	/* 73 */
	{0xFF, 0xFF, 0xFF},	/* 74 for HBM*/
};

#endif /* __SELF_DISPLAY_XA1_H__ */
