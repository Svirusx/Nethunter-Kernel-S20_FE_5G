/*
 * =================================================================
 *
 *       Filename:  ss_dsi_flash_dimming_S6E3HAB_AMB687TZ01.c
 *
 *    Description:  Smart dimming algorithm implementation
 *
 *        Company:  Samsung Electronics
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
#ifndef _SS_DSI_INTERPOLATION_S6E3HAB_AMB687TZ01_H_
#define _SS_DSI_INTERPOLATION_S6E3HAB_AMB687TZ01_H_

#include "ss_dsi_panel_common.h"

#define GAMMA_SIZE 34
#define AOR_SIZE 2
#define VINT_SIZE 1
#define ELVSS_SIZE 3
#define IRC_SIZE 33

#define IRC_START_OFFSET 14

enum IRC_MTP {
	IRC_0_V1,
	IRC_64_V1,
	IRC_128_V1,
	IRC_192_V1,
	IRC_255_V1,
	IRC_V1_MAX,
};

enum {
	V255_BLUE_BIT8,
	V255_BLUE_BIT9,
	V255_GREEN_BIT8,
	V255_GREEN_BIT9,
	V255_RED_BIT8,
	V255_RED_BIT9,
	V255_SIGN_BIT_MAX,
};

int init_interpolation_S6E3HAB_AMB687TZ01(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl, enum INTERPOLATION_MODE mode);
int flash_gamma_support_S6E3HAB_AMB687TZ01(struct samsung_display_driver_data *vdd);

/* Below formula could be changed for each panel */
void gen_hbm_interpolation_gamma_S6E3HAB_AMB687TZ01(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size);
void gen_hbm_interpolation_irc_S6E3HAB_AMB687TZ01(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *hbml_table, int hbm_table_size);
void gen_normal_interpolation_irc_S6E3HAB_AMB687TZ01(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size);
int get_gamma_V_size_S6E3HAB_AMB687TZ01(void);
void convert_GAMMA_to_V(unsigned char* src, unsigned int *dst);
void convert_V_to_GAMMA(unsigned int *src, unsigned char* dst);
int table_gamma_update_hmd_S6E3HAB_AMB687TZ01(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl);

#endif
