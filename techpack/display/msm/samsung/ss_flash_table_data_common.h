 /* =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author:  jb09.kim@samsung.com
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

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
 */

#ifndef _SS_FLASH_TABLE_DATA_COMMON_H_
#define _SS_FLASH_TABLE_DATA_COMMON_H_

#define ERASED_MMC_8BIT 0xFF
#define ERASED_MMC_16BIT 0xFFFF
#define MMC_CHECK_SUM_INIT 0x00

#define FLASH_GAMMA_DBG_BUF_SIZE 256

enum FLASH_OPERATION_BOOST{
	BOOST_DSI_CLK,
	BOOST_MNOC,
	BOOST_CPU, /* use only at factory checking, kernel booting use performance mode*/
	BOOST_MAX,
};

enum BR_FLASH_TABLE_INFO {
	BR_HBM_GAMMA,
	BR_HBM_AOR,
	BR_HBM_VINT,
	BR_HBM_ELVSS,
	BR_HBM_IRC,

	BR_NORMAL_GAMMA,
	BR_NORMAL_AOR,
	BR_NORMAL_DBV,
	BR_NORMAL_VINT,
	BR_NORMAL_ELVSS,
	BR_NORMAL_IRC,

	BR_HMD_GAMMA,
	BR_HMD_AOR,
	BR_HMD_VINT,
	BR_HMD_ELVSS,
	BR_HMD_IRC,

	BR_INFO_MAX,
};

enum FLASH_GAMMA_BURN {
	FLASH_GAMMA_BURN_EMPTY,
	FLASH_GAMMA_BURN_WRITE,
	FLASH_GAMMA_BURN_ERASED = 0xFFFF,
};

struct flash_0xC8_register {
	unsigned char mtp_data[SZ_64]; /* 0xc8 ddi register read data */
	unsigned char flash_data[SZ_64]; /* flash read data */
	unsigned int check_sum_mtp_data; /* based on mtp_0xc8_data */
	unsigned int check_sum_flash_data; /* flash read data */
	unsigned int check_sum_cal_data; /* based on flash_0xc8_data */
};

struct flash_mcd_value {
	int flash_MCD1_R;
	int flash_MCD2_R;
	int flash_MCD1_L;
	int flash_MCD2_L;
};

struct dimming_tbl {
	int *candela_table; /* dtsi table */

	unsigned char **gamma;
	unsigned char **aor;
	unsigned char **vint;
	unsigned char **elvss;
	unsigned char **irc;
	unsigned char **dbv;
};

struct PRINT_TABLE {
	char name[10];
	char read_addr;
	int read_size;
	int read_pos;
};

#include "ss_interpolation_common.h"

/* Raw brightness data from DDI */
struct brightness_data_info {
	/*
		Currently only HBM(12 step) NORMAL(74 step) are available.
		Consider HBM(more 60 step) NORMAL(512 step) could be supported.
	*/
	enum INTERPOLATION_MODE itp_mode;

	/* dsi, mnoc, cpu boosting */
	DECLARE_BITMAP(br_info_select, BR_INFO_MAX);

	/* dsi, mnoc, cpu boosting */
	DECLARE_BITMAP(flash_br_boosting, BOOST_MAX);
};

#define SS_FLASH_GAMMA_READ_ADDR_SIZE	3

struct flash_raw_table {
	/*
	 * Flash address
	 */
	int flash_table_hbm_gamma_offset;
	int flash_table_hbm_aor_offset;
	int flash_table_hbm_vint_offset;
	int flash_table_hbm_elvss_offset;
	int flash_table_hbm_irc_offset;

	int flash_table_normal_gamma_offset;
	int flash_table_normal_aor_offset;
	int flash_table_normal_vint_offset;
	int flash_table_normal_elvss_offset;
	int flash_table_normal_irc_offset;

	int flash_table_hmd_gamma_offset;
	int flash_table_hmd_aor_offset;

	int flash_gamma_data_read_addr[SS_FLASH_GAMMA_READ_ADDR_SIZE];

	/* Below things are emmc read address */
	int flash_gamma_write_check_address;

	/* Here is bank base address */
	int flash_gamma_bank_start;
	int flash_gamma_bank_end;

	/* Below is offset address of base bank */
	int flash_gamma_check_sum_start_offset;
	int flash_gamma_check_sum_end_offset;

	/* For support 0xC8 register integrity */
	int flash_gamma_0xc8_start_offset;
	int flash_gamma_0xc8_end_offset;
	int flash_gamma_0xc8_size;
	int flash_gamma_0xc8_check_sum_start_offset;
	int flash_gamma_0xc8_check_sum_end_offset;

	/* For MCD flash data */
	int flash_MCD1_R_address;
	int flash_MCD2_R_address;
	int flash_MCD1_L_address;
	int flash_MCD2_L_address;

	/*
	 * Flash raw data
	 */
	int br_data_size;
	unsigned char *br_data_raw;

	enum FLASH_GAMMA_BURN write_check;

	bool checksum;
	unsigned int check_sum_flash_data;
	unsigned int check_sum_cal_data;
	unsigned int force_table_interpolatioin; /* For getting table output - debugging purpose */

	struct flash_0xC8_register c8_register; /* 0xC8 register related flash data */

	struct flash_mcd_value mcd; /*mcd flash data */
};

void __table_br(struct samsung_display_driver_data *vdd);
void __flash_br(struct samsung_display_driver_data *vdd);
void flash_br_work_func(struct work_struct *work); /* For read flash data */
char flash_read_one_byte(struct samsung_display_driver_data *vdd, int addr);

#endif
