/*
*
 * Source file for Self Display Driver
 *
 * Copyright (c) 2019 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*
 * SELF DISLAY interface
 *
 * Author: Samsung display driver team
 * Company:  Samsung Electronics
 */

 #ifndef _SELF_DISPLAY_H_
 #define _SELF_DISPLAY_H_

/* Align & Paylod Size for Embedded mode Transfer */
#define CMD_ALIGN 16
#define MAX_PAYLOAD_SIZE 200

/* Align & Paylod Size for Non-Embedded mode(Mass) Command Transfer */
#define MASS_CMD_ALIGN 256
#define MAX_PAYLOAD_SIZE_MASS 0xFFFF0 /* 983,024 */

 /*
  * ioctl
  */
#define SELF_DISPLAY_IOCTL_MAGIC		'S'
#define IOCTL_SELF_MOVE_EN		_IOW(SELF_DISPLAY_IOCTL_MAGIC, 1, int)
#define IOCTL_SELF_MOVE_OFF		_IOW(SELF_DISPLAY_IOCTL_MAGIC, 2, int)
#define IOCTL_SELF_MOVE_RESET		_IOW(SELF_DISPLAY_IOCTL_MAGIC, 3, int)
#define IOCTL_SET_TIME			_IOW(SELF_DISPLAY_IOCTL_MAGIC, 11, struct self_time_info)
#define IOCTL_SET_ICON			_IOW(SELF_DISPLAY_IOCTL_MAGIC, 21, struct self_icon_info)
#define IOCTL_SET_GRID			_IOW(SELF_DISPLAY_IOCTL_MAGIC, 31, struct self_grid_info)
#define IOCTL_SET_ANALOG_CLK		_IOW(SELF_DISPLAY_IOCTL_MAGIC, 41, struct self_analog_clk_info)
#define IOCTL_SET_DIGITAL_CLK		_IOW(SELF_DISPLAY_IOCTL_MAGIC, 51, struct self_digital_clk_info)
#define IOCTL_SET_PARTIAL_HLPM_SCAN	_IOW(SELF_DISPLAY_IOCTL_MAGIC, 61, struct self_partial_hlpm_scan)
#define IOCTL_SELF_MAX			70


#define IMAGE_HEADER_SIZE	2
#define IMAGE_HEADER_SELF_ICON	"IC"
#define IMAGE_HEADER_ANALOG_CLK	"AC"
#define IMAGE_HEADER_DIGITAL_CLK "DC"

#define ROTATE_0	0
#define ROTATE_90	1
#define ROTATE_180	2
#define ROTATE_270	3

#define INTERVAL_100	0
#define INTERVAL_200	1
#define INTERVAL_500	2
#define INTERVAL_1000	3
#define INTERVAL_DEBUG	999

/*
 * data flag
 * This flag is used to distinguish what data will be passed and added at first 2byte of data.
 * ex ) ioctl write data : flag (2byte) + data (bytes)
 */
enum self_display_data_flag {
	FLAG_SELF_MOVE = 1,
	FLAG_SELF_MASK = 2,
	FLAG_SELF_ICON = 3,
	FLAG_SELF_GRID = 4,
	FLAG_SELF_ACLK = 5,
	FLAG_SELF_DCLK = 6,
	FLAG_SELF_VIDEO = 7,
	FLAG_SELF_MAFPC = 8,
	FLAG_SELF_MASK_CRC = 9,
	FLAG_SELF_DISP_MAX,
};

struct self_time_info {
	u32 cur_h;
	u32 cur_m;
	u32 cur_s;
	u32 cur_ms;
	u32 disp_24h;
	u32 interval;
};

struct self_icon_info {
	u32 en;
	u32 pos_x;
	u32 pos_y;
	u32 width;
	u32 height;
	u32 color;
};

struct self_grid_info {
	u32 en;
	u32 s_pos_x;
	u32 s_pos_y;
	u32 e_pos_x;
	u32 e_pos_y;
};

struct self_analog_clk_info {
	u32 en;
	u32 pos_x;
	u32 pos_y;
	u32 rotate;
	u32 mem_mask_en;
	u32 mem_reuse_en;
};

struct self_digital_clk_info {
	u32 en;
	u32 en_hh;
	u32 en_mm;
	u32 pos1_x;
	u32 pos1_y;
	u32 pos2_x;
	u32 pos2_y;
	u32 pos3_x;
	u32 pos3_y;
	u32 pos4_x;
	u32 pos4_y;
	u32 img_width;
	u32 img_height;
	u32 color;
	u32 unicode_attr;
	u32 unicode_width;
};

struct self_partial_hlpm_scan {
	u32 hlpm_en;
	u32 hlpm_mode_sel;
	u32 hlpm_area_1;
	u32 hlpm_area_2;
	u32 hlpm_area_3;
	u32 hlpm_area_4;
	u32 scan_en;
	u32 scan_sl;
	u32 scan_el;
};

enum self_move {
	SELF_MOVE_OFF,
	SELF_MOVE_ON,
	SELF_MOVE_RESET,
	SELF_MOVE_MAX,
};

struct self_display_op {
	bool select;
	bool on;
	u32 wpos;
	u64 wsize;

	char *img_buf;
	u32 img_size;

	int img_checksum;
	u32 img_checksum_cal;
};

struct self_display_debug {
	u32 SI_X_O;
	u32 SI_Y_O;
	u32 MEM_SUM_O;
	u32 SM_SUM_O;
	u32 MEM_WR_DONE_O;
	u32 mem_wr_icon_pk_err_o;
	u32 mem_wr_var_pk_err_o;
	u32 sv_dec_err_fg_o;
	u32 scd_dec_err_fg_o;
	u32 si_dec_err_fg_o;
	u32 CUR_HH_O;
	u32 CUR_MM_O;
	u32 CUR_MSS_O;
	u32 CUR_SS_O;
	u32 SM_WR_DONE_O;
	u32 SM_PK_ERR_O;
	u32 SM_DEC_ERR_O;
};
#endif
