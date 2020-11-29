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

#ifndef __SS_COPR_COMMON_H__
#define __SS_COPR_COMMON_H__

#define MAX_COPR_CNT 36000

enum COPR_VER {
	COPR_VER_1P0 = 0, 	/* BEYOND0 */
	COPR_VER_2P0,	 	/* STAR, CROWN */
	COPR_VER_3P0, 		/* BEYOND1/2, WINNER - COPR_1121 */
	COPR_VER_4P0, 		/* A91, S6E3FA9 */
	COPR_VER_5P0, 		/* Hubble, S6E3HAB */
};

/* MAX COPR ROI is from copr ver. 3.0 */
#define MAX_COPR_ROI_CNT 6

struct COPR_ROI {
	int ROI_X_S;
	int ROI_Y_S;
	int ROI_X_E;
	int ROI_Y_E;
};

struct COPR_REG_ARGS {
	const char *name;
	u32 *store_ptr;
};

struct COPR_CMD {
	int COPR_MASK;
	int CNT_RE;
	int COPR_ILC;
	int COPR_GAMMA;
	int COPR_EN;

	int COPR_ER;
	int COPR_EG;
	int COPR_EB;
	int COPR_ERC;
	int COPR_EGC;
	int COPR_EBC;

	int MAX_CNT;
	int ROI_ON; // only ver 2.0
	int COPR_ROI_CTRL;
	struct COPR_ROI roi[MAX_COPR_ROI_CNT];
};

struct COPR_REG_OSSET{
	const char *name;
	int offset;
};

struct COPR_ROI_OPR {
	int R_OPR;
	int G_OPR;
	int B_OPR;
};

enum COPR_CD_INDEX {
	COPR_CD_INDEX_0,	/* for copr show - SSRM */
	COPR_CD_INDEX_1,	/* for brt_avg show - mDNIe */
	MAX_COPR_CD_INDEX,
};

struct COPR_CD {
	s64 cd_sum;
	int cd_avr;

	ktime_t cur_t;
	ktime_t last_t;
	s64 total_t;
};

struct COPR {
	int ver;
	int copr_on;

	int tx_bpw;
	int rx_bpw;
	int tx_size;
	int rx_size;
	char rx_addr;

	/* read data */
	int copr_ready;
	int current_cnt;
	int current_copr;
	int avg_copr;
	int sliding_current_cnt;
	int sliding_avg_copr;
	int comp_copr;
	struct COPR_ROI_OPR roi_opr[MAX_COPR_ROI_CNT];

	struct mutex copr_lock;
	struct mutex copr_val_lock;
	struct workqueue_struct *read_copr_wq;
	struct work_struct read_copr_work;

	struct COPR_CD copr_cd[MAX_COPR_CD_INDEX];
	struct COPR_CMD cmd;
	struct COPR_CMD orig_cmd;
	struct COPR_CMD cur_cmd;

	/* roi values from mDNIe service for AFC */
	struct COPR_ROI afc_roi[MAX_COPR_ROI_CNT];
	int afc_roi_cnt;

	int display_read; /* Does display driver use copr read operation? ? 1 : 0 */
	void (*panel_init)(struct samsung_display_driver_data *vdd);
};

void print_copr_cmd(struct COPR_CMD cmd);
int ss_copr_set_cmd_offset(struct COPR_CMD *cmd, char* p);
void ss_copr_set_cmd(struct samsung_display_driver_data *vdd, struct COPR_CMD *copr_cmd);
int ss_get_copr_orig_cmd(struct samsung_display_driver_data *vdd);
void ss_copr_enable(struct samsung_display_driver_data *vdd, int enable);
int ss_copr_read(struct samsung_display_driver_data *vdd);
void ss_set_copr_sum(struct samsung_display_driver_data *vdd, enum COPR_CD_INDEX idx);
void ss_copr_reset_cnt(struct samsung_display_driver_data *vdd);
int ss_copr_get_roi_opr(struct samsung_display_driver_data *vdd);

void ss_read_copr_work(struct work_struct *work);
void ss_copr_init(struct samsung_display_driver_data *vdd);

#endif /* __SS_COPR_COMMON_H__ */
