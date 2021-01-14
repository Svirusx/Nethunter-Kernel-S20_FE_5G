/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef SEC_QUEST_BPS_CLASSIFIER_H
#define SEC_QUEST_BPS_CLASSIFIER_H

#define QUEST_BPS_MAX_QUICKBUILD_LEN	10
#define QUEST_BPS_INIT_MAGIC1				0xABCDABCD
#define QUEST_BPS_CLASSIFIER_MAGIC2		0xDCBADCBA

/* a structure for storing upload information for fac binary */
struct upload_info {
	int sp;
	int wp;
	int dp;
	int kp;
	int mp;
	int tp;
	int cp;
};

struct bps_info {
	/* magic code for bps */
	unsigned int magic[2];

	/* factory upload info */
	struct upload_info up_cnt;

	/* BPS filter count */
	int pc_lr_cnt;
	int pc_lr_last_idx;
	int tzerr_cnt;
	int klg_cnt;

	/* binary download info */
	int dn_cnt;
	char build_id[QUEST_BPS_MAX_QUICKBUILD_LEN];
	int rbps_magic;
	int rbpsp;
	int rbpsf;
	int n1bps;
	int n2bps;
	int n3bps;
	int n4bps;
	/* factory info */
	int fac_info;

	/* fac info : smd f/t end ~ rf cal start */
	struct upload_info semi_asm_up_cnt;
	int semi_asm_pc_lr_cnt;
	int semi_asm_pc_lr_last_idx;

	/* fac info : rf cal start ~ */
	struct upload_info asm_up_cnt;
	int asm_pc_lr_cnt;
	int asm_pc_lr_last_idx;
};

static bool sec_quest_bps_env_initialized;
#endif
