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
#include "ss_dsi_panel_common.h"
#include "ss_interpolation_common.h"

#define CONTROL_AUTO_BRIGHTNESS_V3 (256)
#define CONTROL_AUTO_BRIGHTNESS_V4 (512)

char ss_dimming_mode_debug[][DIMMING_MODE_DEBUG_STRING] = {
	"SS_FLASH_DIMMING_MODE",
	"SS_S_DIMMING_AOR_ITP_MODE",
	"SS_S_DIMMING_GAMMA_ITP_MODE",
	"SS_A_DIMMING_MODE",
	"DIMMING_MODE_MAX",
};

static void init_hbm_interpolation(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *hbm_table, int hbm_table_size)
{
	int loop, column;
	int hbm_interpolation_step;
	int gamma_size = vdd->br_info.gamma_size;
	int irc_size = vdd->br_info.irc_size;
	struct ss_hbm_interpolation *hbm_itp;

	/* update hbm interpolation step */
	for (hbm_interpolation_step =0, loop = 0 ; loop < hbm_table_size; loop++)
		hbm_interpolation_step += hbm_table[loop].steps;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		hbm_itp = &br_tbl->flash_itp.hbm;
	else
		hbm_itp = &br_tbl->table_itp.hbm;

	hbm_itp->brightness_step = hbm_interpolation_step;

	/*
		free alloc memory : test purpose for slab memory integrity
	*/
	if (!IS_ERR_OR_NULL(hbm_itp->br_table)) {
		kfree(hbm_itp->br_table);
		hbm_itp->br_table = NULL;
	}

	if (!IS_ERR_OR_NULL(hbm_itp->gamma)) {
		for (column = 0; column < hbm_interpolation_step; column++) {
			kfree(hbm_itp->gamma[column]);
			kfree(hbm_itp->irc[column]);
		}

		kfree(hbm_itp->gamma);
		kfree(hbm_itp->irc);

		hbm_itp->gamma = NULL;
		hbm_itp->irc = NULL;
	}

	/*alloc 2 dimension matrix */
	if (!hbm_itp->br_table) {
		hbm_itp->br_table = kzalloc(hbm_interpolation_step * sizeof(struct ss_hbm_interpolation_br_table), GFP_KERNEL);
	}

	if (!hbm_itp->gamma) {
		hbm_itp->gamma = kzalloc(hbm_interpolation_step * sizeof(void *), GFP_KERNEL);
		hbm_itp->irc = kzalloc(hbm_interpolation_step * sizeof(void *), GFP_KERNEL);

		for (column = 0; column < hbm_interpolation_step; column++) {
			hbm_itp->gamma[column] = kzalloc(gamma_size, GFP_KERNEL);
			hbm_itp->irc[column] = kzalloc(irc_size, GFP_KERNEL);
		}
	}
}

static void gen_hbm_interpolation_platform_level_lux_mode(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *hbml_table, int hbm_table_size)
{
	int loop, step, index;
	struct ss_hbm_interpolation *hbm_itp;
	long long platform_up, platform_down, platform_curr, platform_div;
	long long lux_up, lux_down, lux_add;
	long long result1, result2, result3;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		hbm_itp = &br_tbl->flash_itp.hbm;
	else
		hbm_itp = &br_tbl->table_itp.hbm;

	/* Platform level & lux mode */
	for (index = 0, loop = 0; loop < hbm_table_size; loop++) {
		platform_up = hbml_table[loop].platform;
		lux_up = hbml_table[loop].lux_mode;

		if (loop == 0) {
			platform_down = hbml_table[0].platform;
			lux_add = lux_down = hbml_table[0].lux_mode;

		} else {
			platform_down = hbml_table[loop - 1].platform;
			lux_add = lux_down = hbml_table[loop - 1].lux_mode;
		}

		platform_div = hbml_table[loop].steps;

		platform_up *= MULTIPLY_x10000;
		platform_down *= MULTIPLY_x10000;

		for (step = 0; step < hbml_table[loop].steps; step++) {
			if (step == hbml_table[loop].steps - 1) {
				hbm_itp->br_table[index].platform_level_x10000 =
					hbml_table[loop].platform * MULTIPLY_x10000;

				hbm_itp->br_table[index].interpolation_br_x10000 = hbml_table[loop].lux_mode * MULTIPLY_x10000;
			} else {
				/* Platform level */
				result1 = (((platform_up - platform_down) * (step + 1))/ (platform_div)) + platform_down;

				hbm_itp->br_table[index].platform_level_x10000 =
					(unsigned int)(ROUNDING(result1 / MULTIPLY_x100, MULTIPLY_x100) * MULTIPLY_x100);


				/* lux level */
				platform_curr = hbm_itp->br_table[index].platform_level_x10000;

				result1 = lux_up - lux_down;
				result2 = (platform_curr - platform_down) / ((platform_up - platform_down) / MULTIPLY_x10000);
				result3 = result1 * result2;

				hbm_itp->br_table[index].interpolation_br_x10000 = ROUNDING(result3, MULTIPLY_x100) + (lux_add * MULTIPLY_x10000);
			}

			hbm_itp->br_table[index].lux_mode = hbm_itp->br_table[index].interpolation_br_x10000 / MULTIPLY_x10000;

			LCD_DEBUG("Platform : %3d %7d %7d %4d\n",
					index,
					hbm_itp->br_table[index].platform_level_x10000,
					hbm_itp->br_table[index].interpolation_br_x10000,
					hbm_itp->br_table[index].lux_mode);

			index++;
		}
	}
}

/* -1 means hbm_interpolation_table[0] normal max candela index */
#define BR_TABLE_NORMAL_MAX_SUB (1)
#define AUTO_LEVEL_START (6)
static void update_hbm_candela_map_table(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	struct candela_map_table *table;
	struct ss_hbm_interpolation *hbm_itp;
	int interpolation_step;
	struct ss_hbm_interpolation_br_table *br_table;

	struct ss_normal_interpolation *norma_itp;
	int normal_interpolation_step;

	int column;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION) {
		hbm_itp = &br_tbl->flash_itp.hbm;
		norma_itp = &br_tbl->flash_itp.normal;
	} else {
		hbm_itp = &br_tbl->table_itp.hbm;
		norma_itp = &br_tbl->table_itp.normal;
	}

	normal_interpolation_step = norma_itp->brightness_step;

	interpolation_step = hbm_itp->brightness_step - BR_TABLE_NORMAL_MAX_SUB;
	br_table = hbm_itp->br_table;

	if (vdd->br_info.common_br.pac)
		table = &vdd->br_info.candela_map_table[PAC_HBM][vdd->panel_revision];
	else
		table = &vdd->br_info.candela_map_table[HBM][vdd->panel_revision];

	/* check table size */
	if (table->tab_size != interpolation_step) {
		LCD_INFO("alloc for pac hbm candela_map_table %d <- %d\n", table->tab_size, interpolation_step);

		table->tab_size = interpolation_step;

		/* cd */
		if (!IS_ERR_OR_NULL(table->cd))
			kfree(table->cd);
		table->cd = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);

		/* idx */
		if (!IS_ERR_OR_NULL(table->idx))
			kfree(table->idx);
		table->idx = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);

		/* from */
		if (!IS_ERR_OR_NULL(table->from))
			kfree(table->from);
		table->from = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);

		/* end */
		if (!IS_ERR_OR_NULL(table->end))
			kfree(table->end);
		table->end = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);

		/* auto_level*/
		if (!IS_ERR_OR_NULL(table->auto_level))
			kfree(table->auto_level);
		table->auto_level = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);
	}

	/* update table information */
	for (column = 0; column < table->tab_size; column++) {
		/* cd */
		table->cd[column] = br_table[column + BR_TABLE_NORMAL_MAX_SUB].lux_mode;

		/* idx */
		table->idx[column] = column;

		/* from, end */
		if (normal_interpolation_step < CONTROL_AUTO_BRIGHTNESS_V4) {
			table->from[column] = br_table[column + BR_TABLE_NORMAL_MAX_SUB - 1].platform_level_x10000 / MULTIPLY_x10000 + 1;
			table->end[column] = (br_table[column + BR_TABLE_NORMAL_MAX_SUB].platform_level_x10000 / MULTIPLY_x10000);
		} else {
			table->from[column] = br_table[column + BR_TABLE_NORMAL_MAX_SUB - 1].platform_level_x10000 / MULTIPLY_x100 + 1;
			table->end[column] = (br_table[column + BR_TABLE_NORMAL_MAX_SUB].platform_level_x10000 / MULTIPLY_x100);
		}

		/* auto_level */
		table->auto_level[column] = AUTO_LEVEL_START + column;
	}

	/* hbm min_lv */
	table->min_lv = table->from[0];

	/* hbm max_lv */
	table->max_lv = table->end[table->tab_size-1];
}

static void update_hbm_interpolation(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *hbm_table, int hbm_table_size)
{
	/* 1st */
	gen_hbm_interpolation_platform_level_lux_mode(vdd, br_tbl, hbm_table, hbm_table_size);

	/* 2st */
	if (vdd->panel_func.gen_hbm_interpolation_gamma)
		vdd->panel_func.gen_hbm_interpolation_gamma(vdd, br_tbl, hbm_table, hbm_table_size);
	else
		LCD_ERR("No gen_hbm_interpolation_gamma !!\n");

	/* 3st */
	if (vdd->panel_func.gen_hbm_interpolation_irc)
		vdd->panel_func.gen_hbm_interpolation_irc(vdd, br_tbl, hbm_table, hbm_table_size);
	else
		LCD_ERR("No gen_hbm_interpolation_irc !!\n");

	/* 4st */
	update_hbm_candela_map_table(vdd, br_tbl);
}

static void init_normal_interpolation(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size)
{
	int gamma_size = vdd->br_info.gamma_size;
	int loop, column;
	int normal_interpolation_step;
	int irc_size = vdd->br_info.irc_size;
	struct ss_interpolation *ss_itp;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		ss_itp = &br_tbl->flash_itp;
	else
		ss_itp = &br_tbl->table_itp;

	/* update normal interpolation step */
	for (normal_interpolation_step = 0, loop = 0 ; loop < normal_table_size; loop++)
		normal_interpolation_step += normal_table[loop].steps;

	ss_itp->normal.brightness_step = normal_interpolation_step;

	/*
		free alloc memory : test purpose for slab memory integrity
	*/
	if (!IS_ERR_OR_NULL(ss_itp->normal.br_aor_table)) {
		kfree(ss_itp->normal.br_aor_table);
		ss_itp->normal.br_aor_table = NULL;
	}

	if (!IS_ERR_OR_NULL(ss_itp->normal.irc)) {
		for (column = 0; column < normal_interpolation_step; column++)
			kfree(ss_itp->normal.irc[column]);

		kfree(ss_itp->normal.irc);
		ss_itp->normal.irc = NULL;
	}

	if (!IS_ERR_OR_NULL(ss_itp->normal.gamma)) {
		for (column = 0; column < normal_interpolation_step; column++)
			kfree(ss_itp->normal.gamma[column]);

		kfree(ss_itp->normal.gamma);
		ss_itp->normal.gamma = NULL;
	}

	/* alloc */
	if (!ss_itp->normal.br_aor_table)
		ss_itp->normal.br_aor_table = kzalloc(normal_interpolation_step * sizeof(struct ss_normal_interpolation_br_aor_table), GFP_KERNEL);

	if (!ss_itp->normal.irc) {
		ss_itp->normal.irc = kzalloc(normal_interpolation_step * sizeof(void *), GFP_KERNEL);

		for (column = 0; column < normal_interpolation_step; column++)
			ss_itp->normal.irc[column] = kzalloc(irc_size, GFP_KERNEL);
	}

	if (!ss_itp->normal.gamma) {
		ss_itp->normal.gamma = kzalloc(normal_interpolation_step * sizeof(void *), GFP_KERNEL);

		// TODO: skip alloc if column is over gamma itp threashold candela that can be got from AOR...
		for (column = 0; column < normal_interpolation_step; column++)
			ss_itp->normal.gamma[column] = kzalloc(gamma_size, GFP_KERNEL);
	}

	LCD_DEBUG("%pk %d\n", ss_itp->normal.br_aor_table, normal_interpolation_step);
}

static unsigned int A_DIMMING_AOR_CAL(
	long long aor_dec_up_x10000, long long aor_dec_down_x10000,
	long long platform_up_x10000, long long platform_down_x10000, long long platform_curr_x10000)
{
	long long aor_cal;
	unsigned int aor_curr;

	aor_cal = aor_dec_up_x10000 - aor_dec_down_x10000;
	aor_cal *= ((platform_curr_x10000 - platform_down_x10000)  * MULTIPLY_x10000) / (platform_up_x10000 - platform_down_x10000);
	aor_cal = (ROUNDING_NEGATIVE(aor_cal / MULTIPLY_x100) * MULTIPLY_x100) / MULTIPLY_x10000;

	aor_curr = (unsigned int)(aor_cal + aor_dec_down_x10000);
	aor_curr = (ROUNDING(aor_curr, MULTIPLY_x100) / MULTIPLY_x100) * MULTIPLY_x100;

	return aor_curr;
}

static unsigned int S_DIMMING_AOR_CAL
	(long long aor_dec_up_x10000, long long lux_x10000, long long interpolation_br_x10000)
{
	long long virtual_base_lux;
	long long aor_cal, result1, result2;
	unsigned int aor_curr;

	virtual_base_lux = ((lux_x10000) * ( 100 * MULTIPLY_x10000)) / ((100 * MULTIPLY_x10000) - aor_dec_up_x10000);
	virtual_base_lux = ROUNDING(virtual_base_lux, MULTIPLY_x100);

	result1 = (1 * MULTIPLY_x100 * MULTIPLY_x100000);
	result2 = (interpolation_br_x10000 * MULTIPLY_x100 * MULTIPLY_x100000) / virtual_base_lux;

	aor_cal = ROUNDING((result1 - result2)/MULTIPLY_x10, MULTIPLY_x100);

	aor_curr = (unsigned int)(aor_cal);
	aor_curr = (aor_curr / MULTIPLY_x100) * MULTIPLY_x100;

	return aor_curr;
}

static void convert_dec_to_hex_str(unsigned int aor_hex, unsigned int aor_size, unsigned char *buf)
{
	unsigned char temp_buf[AOR_HEX_STRING_CNT];
	int cnt, index;

	memcpy(temp_buf, &aor_hex, AOR_HEX_STRING_CNT);

	for (index = 0, cnt = aor_size - 1; cnt >= 0; cnt--, index++)
		buf[index] = temp_buf[cnt];
}

static void gen_normal_interpolation_platform_level_lux_mode(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size)
{

	int loop, step, index;
	struct ss_normal_interpolation *normal_itp;
	long long platform_up, platform_down, platform_div;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		normal_itp = &br_tbl->flash_itp.normal;
	else
		normal_itp = &br_tbl->table_itp.normal;

	/* Platform level & lux mode */
	for (index = 0, loop = 0; loop < normal_table_size; loop++) {
		platform_up = normal_table[loop].platform;

		if (loop == 0)
			platform_down = normal_table[0].platform; /* kzalloc used : 0 */
		else
			platform_down = normal_table[loop - 1].platform;

		platform_div = normal_table[loop].steps;

		platform_up *= MULTIPLY_x10000;
		platform_down *= MULTIPLY_x10000;

		for (step = 0; step < normal_table[loop].steps; step++) {
			if (step == normal_table[loop].steps - 1) {
				normal_itp->br_aor_table[index].platform_level_x10000 =
					normal_table[loop].platform * MULTIPLY_x10000;
			} else {
				normal_itp->br_aor_table[index].platform_level_x10000 =
					ROUNDING((((platform_up - platform_down) * (step + 1))/ (platform_div)) + platform_down, MULTIPLY_x100);
			}

			normal_itp->br_aor_table[index].lux_mode = normal_table[loop].lux_mode;

			LCD_DEBUG("Platform : %d %d %d\n", index, normal_itp->br_aor_table[index].platform_level_x10000, normal_itp->br_aor_table[index].lux_mode);

			index++;
		}
	}
}

static void gen_normal_interpolation_br(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size)
{
	int loop, step, index;
	struct ss_normal_interpolation *normal_itp;
	long long platform_up, platform_down, platform_curr;
	long long lux_up, lux_down, lux_add;
	long long result1, result2, result3, result4;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		normal_itp = &br_tbl->flash_itp.normal;
	else
		normal_itp = &br_tbl->table_itp.normal;

	/* Interpolation Br */
	for (index = 0, loop = 0; loop < normal_table_size; loop++) {
		lux_up = normal_table[loop].lux_mode;
		platform_up = normal_table[loop].platform;

		if (loop == 0) {
			platform_down = normal_table[0].platform;
			lux_down = normal_table[0].lux_mode;
			lux_add = normal_table[0].lux_mode;
		} else {
			platform_down = normal_table[loop - 1].platform;
			lux_down = normal_table[loop - 1].lux_mode;
			lux_add = normal_table[loop - 1].lux_mode;
		}

		lux_add *= MULTIPLY_x10000;

		platform_up *= MULTIPLY_x10000;
		platform_down *= MULTIPLY_x10000;

		for (step = 0; step < normal_table[loop].steps; step++) {
			platform_curr = normal_itp->br_aor_table[index].platform_level_x10000;

			result1 = (long long)lux_up - (long long)lux_down;

			result2 = (long long)platform_curr - (long long)platform_down;
			result2 *= MULTIPLY_x10000;

			result3 = (long long)platform_up - (long long)platform_down;

			result4 = (result1 * result2) / result3;

			normal_itp->br_aor_table[index].interpolation_br_x10000 = ROUNDING(result4 +lux_add, MULTIPLY_x100);

			LCD_DEBUG("BR : %d %d\n", index, normal_itp->br_aor_table[index].interpolation_br_x10000);

			index++;
		}
	}
}

/* Do not use for another model in 8250. */
/* Modified only for Bloom5G model using both 2 dimming mode(A,S), just like SM8150 */
int gen_normal_interpolation_aor_gamma_legacy(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size)
{

	int loop, reverse_loop, step, index;
	struct ss_normal_interpolation *normal_itp;
	long long platform_up, platform_down, platform_curr;
	long long aor_hex_up, aor_hex_down, aor_hex_next_up, aor_hex_cnt;
	long long aor_dec_up_x10000, aor_dec_down_x10000, aor_dec_curr_x10000;
	enum ss_dimming_mode dimming_mode_curr = DIMMING_MODE_MAX;
	enum ss_dimming_mode dimming_mode_prev = DIMMING_MODE_MAX;
	enum ss_dimming_mode s_dimming_step = DIMMING_MODE_MAX;

	unsigned int s_dimming_aor_hex = 0;
	unsigned int aor_size = vdd->br_info.aor_size;
	struct dimming_tbl *normal_tbl = &br_tbl->normal_tbl;
	int gamma_V_size;
	int gamma_size;

	/* ddi vertical porches are used for AOR interpolation.
	 * In case of 96/48hz mode, its base AOR came from below base RR mode.
	 * - 96hz: 120hz HS -> AOR_96hz = AOR_120hz * (vtotal_96hz) / (vtotal_120hz)
	 * - 48hz: 60hz normal -> AOR_48hz = AOR_60hz * (vtotal_48hz) / (vtotal_60hz)
	 * If there is no base vertical porches, (ex: ddi_vbp_base) in panel dtsi,
	 * parser function set base value as target value (ex: ddi_vbp_base = ddi_vbp).
	 */
	int ddi_tot_v = br_tbl->ddi_vfp + br_tbl->ddi_vactive + br_tbl->ddi_vbp;
	int ddi_tot_v_base = br_tbl->ddi_vfp_base + br_tbl->ddi_vactive_base + br_tbl->ddi_vbp_base;

	LCD_INFO("RR: %3d %s: ddi_tot_v: %4d, ddi_tot_v_base: %d, aor_size:%d\n",
			br_tbl->refresh_rate,
			br_tbl->is_sot_hs_mode ? "HS" : "NM",
			ddi_tot_v, ddi_tot_v_base, aor_size);

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		normal_itp = &br_tbl->flash_itp.normal;
	else
		normal_itp = &br_tbl->table_itp.normal;

	if (!vdd->panel_func.get_gamma_V_size) {
		LCD_ERR("error: no get_gamma_V_size\n");
		return -ENODEV;
	}
	gamma_V_size = vdd->panel_func.get_gamma_V_size();
	gamma_size = vdd->br_info.gamma_size;

	/* AOR */
	for (index = 0, loop = 0, reverse_loop = normal_table_size - 1;
		loop < normal_table_size; loop++, reverse_loop--) {
		platform_up = normal_table[loop].platform;

		/* use reverse_loop by flash aor data ordering */
		for (aor_hex_up = 0, aor_hex_cnt = 0; aor_hex_cnt < aor_size; aor_hex_cnt++) {
			aor_hex_up <<= (0x08 * aor_hex_cnt);
			aor_hex_up |= normal_tbl->aor[reverse_loop][aor_hex_cnt];
		}

		/* ddi vertical porches are used for AOR interpolation.
		 * In case of 96/48hz mode, its base AOR came from below base RR mode.
		 * - 96hz: 120hz HS -> AOR_96hz = AOR_120hz * (vtotal_96hz) / (vtotal_120hz)
		 * - 48hz: 60hz normal -> AOR_48hz = AOR_60hz * (vtotal_48hz) / (vtotal_60hz)
		 * If there is no base vertical porches, (ex: ddi_vbp_base) in panel dtsi,
		 * parser function set base value as target value (ex: ddi_vbp_base = ddi_vbp).
		 */
		aor_hex_up = (aor_hex_up * ddi_tot_v) / ddi_tot_v_base;

		aor_dec_up_x10000 = AOR_HEX_TO_PERCENT_X10000(ddi_tot_v, aor_hex_up);

		if (loop == 0) {
			platform_down = normal_table[0].platform;
			aor_hex_down = aor_hex_up;
		} else {
			platform_down = normal_table[loop - 1].platform;
			aor_hex_down = normal_itp->br_aor_table[index - 1].aor_hex;
		}
		aor_dec_down_x10000 = AOR_HEX_TO_PERCENT_X10000(ddi_tot_v, aor_hex_down);

		/* use reverse_loop by flash aor data ordering */
		if (reverse_loop < 1)
			aor_hex_next_up = aor_hex_up;
		else {
			for (aor_hex_next_up = 0, aor_hex_cnt = 0; aor_hex_cnt < aor_size; aor_hex_cnt++) {
				aor_hex_next_up <<= (0x08 * aor_hex_cnt);
				aor_hex_next_up |= normal_tbl->aor[reverse_loop - 1][aor_hex_cnt];
			}

			/* ddi vertical porches are used for AOR interpolation.
			 * In case of 96/48hz mode, its base AOR came from below base RR mode.
			 * - 96hz: 120hz HS -> AOR_96hz = AOR_120hz * (vtotal_96hz) / (vtotal_120hz)
			 * - 48hz: 60hz normal -> AOR_48hz = AOR_60hz * (vtotal_48hz) / (vtotal_60hz)
			 * If there is no base vertical porches, (ex: ddi_vbp_base) in panel dtsi,
			 * parser function set base value as target value (ex: ddi_vbp_base = ddi_vbp).
			 */
			aor_hex_next_up = (aor_hex_next_up * ddi_tot_v) / ddi_tot_v_base;
		}

		/* use reverse_loop by flash aor data ordering for gamma */



		platform_up *= MULTIPLY_x10000;
		platform_down *= MULTIPLY_x10000;

		/*
			dimming mdoe check is consist of 2 step.
			dimming mode check 1 is for check sw calculation.
			dimming mode check 2 is for flash nand real data.
		*/

		/* dimming mode check 1 */
		if (aor_hex_up == aor_hex_next_up)	{
			dimming_mode_curr = SS_S_DIMMING_AOR_ITP_MODE;
			s_dimming_step = SS_S_DIMMING_AOR_ITP_MODE;
			s_dimming_aor_hex = aor_hex_up;
		} else if ((s_dimming_step == SS_S_DIMMING_AOR_ITP_MODE) && \
				((aor_hex_up == s_dimming_aor_hex) && (aor_hex_next_up != s_dimming_aor_hex))) {
			dimming_mode_curr = SS_S_DIMMING_AOR_ITP_MODE;
			s_dimming_step = SS_S_DIMMING_EXIT_MODE_1;
		} else if ((s_dimming_step == SS_S_DIMMING_EXIT_MODE_1) && \
				((aor_hex_up != s_dimming_aor_hex) && (aor_hex_next_up != s_dimming_aor_hex))) {
			/* SS_S_DIMMING_EXIT_MODE_2 is real exit for S_DIMMING */
			dimming_mode_curr = SS_S_DIMMING_AOR_ITP_MODE;

			/* reset flags after SS_S_DIMMING_EXIT_MODE_2 */
			s_dimming_aor_hex = 0;
			s_dimming_step = DIMMING_MODE_MAX;
		} else {
			dimming_mode_curr = SS_A_DIMMING_MODE;
		}

		dimming_mode_prev = dimming_mode_curr;

		for (step = 0; step < normal_table[loop].steps; step++) {
			platform_curr = normal_itp->br_aor_table[index].platform_level_x10000;

			/* dimming mode check 2 */
			if (step == normal_table[loop].steps - 1)
				dimming_mode_curr = SS_FLASH_DIMMING_MODE;


			normal_itp->br_aor_table[index].dimming_mode = dimming_mode_curr;

			if (dimming_mode_curr == SS_FLASH_DIMMING_MODE) {
				/* FLASH_DIMMING */
				aor_dec_curr_x10000 = aor_dec_up_x10000;
			} else if (dimming_mode_curr == SS_S_DIMMING_AOR_ITP_MODE) {
				/* S_DIMMING */
				aor_dec_curr_x10000 = S_DIMMING_AOR_CAL (
						aor_dec_up_x10000,
						(long long)(normal_itp->br_aor_table[index].lux_mode * MULTIPLY_x10000),
						(long long)(normal_itp->br_aor_table[index].interpolation_br_x10000));
			} else {
				/* A_DIMMING */
				aor_dec_curr_x10000 = A_DIMMING_AOR_CAL (
						aor_dec_up_x10000, aor_dec_down_x10000,
						platform_up, platform_down, platform_curr);
			}

			normal_itp->br_aor_table[index].aor_percent_x10000 = aor_dec_curr_x10000;
			normal_itp->br_aor_table[index].aor_hex = AOR_PERCENT_X1000_TO_HEX(ddi_tot_v,aor_dec_curr_x10000);//X V

			/* To convert dec to hex string format */
			convert_dec_to_hex_str(
				normal_itp->br_aor_table[index].aor_hex,
				aor_size,
				normal_itp->br_aor_table[index].aor_hex_string);

			LCD_DEBUG("AOR index : %3d  hex: 0x%04x percent_x10000: %6d mode : %s\n", index,
				normal_itp->br_aor_table[index].aor_hex,
				normal_itp->br_aor_table[index].aor_percent_x10000,
				ss_dimming_mode_debug[dimming_mode_curr]);

			index++;
		}

	}

	return 0;
}

static int gen_normal_interpolation_aor_gamma(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size)
{
	int loop, reverse_loop, step, index;
	struct ss_normal_interpolation *normal_itp;
	long long platform_up, platform_down, platform_curr;
	long long aor_hex_up, aor_hex_down, aor_hex_next_up, aor_hex_cnt;
	long long aor_dec_up_x10000, aor_dec_down_x10000, aor_dec_curr_x10000;
	enum ss_dimming_mode dimming_mode_curr = DIMMING_MODE_MAX;
	unsigned int aor_size = vdd->br_info.aor_size;
	struct dimming_tbl *normal_tbl = &br_tbl->normal_tbl;

	unsigned char *max_gamma;
	unsigned char *min_gamma;
	int *max_gammaV;
	int *min_gammaV;
	int *itp_gammaV;
	int max_cd, min_cd;
	int gamma_loop;
	int gamma_V_size;
	int gamma_size;


	/* ddi vertical porches are used for AOR interpolation.
	 * In case of 96/48hz mode, its base AOR came from below base RR mode.
	 * - 96hz: 120hz HS -> AOR_96hz = AOR_120hz * (vtotal_96hz) / (vtotal_120hz)
	 * - 48hz: 60hz normal -> AOR_48hz = AOR_60hz * (vtotal_48hz) / (vtotal_60hz)
	 * If there is no base vertical porches, (ex: ddi_vbp_base) in panel dtsi,
	 * parser function set base value as target value (ex: ddi_vbp_base = ddi_vbp).
	 */
	int ddi_tot_v = br_tbl->ddi_vfp + br_tbl->ddi_vactive + br_tbl->ddi_vbp;
	int ddi_tot_v_base = br_tbl->ddi_vfp_base + br_tbl->ddi_vactive_base + br_tbl->ddi_vbp_base;

	int i;
	char pBuffer[256];
	memset(pBuffer, 0x00, 256);

	LCD_INFO("flash %d, R: %3d %s: ddi_tot_v: %4d, ddi_tot_v_base: %d\n",
			vdd->br_info.panel_br_info.itp_mode,
			br_tbl->refresh_rate,
			br_tbl->is_sot_hs_mode ? "HS" : "NM",
			ddi_tot_v, ddi_tot_v_base);

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		normal_itp = &br_tbl->flash_itp.normal;
	else
		normal_itp = &br_tbl->table_itp.normal;

	if (!vdd->panel_func.get_gamma_V_size) {
		LCD_ERR("error: no get_gamma_V_size\n");
		return -ENODEV;
	}
	gamma_V_size = vdd->panel_func.get_gamma_V_size();
	gamma_size = vdd->br_info.gamma_size;

	max_gammaV = kzalloc(gamma_V_size * sizeof(int), GFP_KERNEL);
	min_gammaV = kzalloc(gamma_V_size * sizeof(int), GFP_KERNEL);
	itp_gammaV = kzalloc(gamma_V_size * sizeof(int), GFP_KERNEL);

	if (!max_gammaV || !min_gammaV || !itp_gammaV) {
		LCD_ERR("fail to alloc gammaV memory\n");
		kfree(max_gammaV);
		kfree(min_gammaV);
		kfree(itp_gammaV);
		return -ENOMEM;
	}

	for (index = 0, loop = 0, reverse_loop = normal_table_size - 1;
		loop < normal_table_size; loop++, reverse_loop--) {
		platform_up = normal_table[loop].platform;

		/* use reverse_loop by flash aor data ordering */
		for (aor_hex_up = 0, aor_hex_cnt = 0; aor_hex_cnt < aor_size; aor_hex_cnt++) {
			aor_hex_up <<= (0x08 * aor_hex_cnt);
			aor_hex_up |= normal_tbl->aor[reverse_loop][aor_hex_cnt];
		}

		/* ddi vertical porches are used for AOR interpolation.
		 * In case of 96/48hz mode, its base AOR came from below base RR mode.
		 * - 96hz: 120hz HS -> AOR_96hz = AOR_120hz * (vtotal_96hz) / (vtotal_120hz)
		 * - 48hz: 60hz normal -> AOR_48hz = AOR_60hz * (vtotal_48hz) / (vtotal_60hz)
		 * If there is no base vertical porches, (ex: ddi_vbp_base) in panel dtsi,
		 * parser function set base value as target value (ex: ddi_vbp_base = ddi_vbp).
		 */
		aor_hex_up = (aor_hex_up * ddi_tot_v) / ddi_tot_v_base;

		aor_dec_up_x10000 = AOR_HEX_TO_PERCENT_X10000(ddi_tot_v, aor_hex_up);

		if (loop == 0) {
			platform_down = normal_table[0].platform;
			aor_hex_down = aor_hex_up;
		} else {
			platform_down = normal_table[loop - 1].platform;
			aor_hex_down = normal_itp->br_aor_table[index - 1].aor_hex;
		}
		aor_dec_down_x10000 = AOR_HEX_TO_PERCENT_X10000(ddi_tot_v, aor_hex_down);

		/* use reverse_loop by flash aor data ordering */
		if (reverse_loop < 1)
			aor_hex_next_up = aor_hex_up;
		else {
			for (aor_hex_next_up = 0, aor_hex_cnt = 0; aor_hex_cnt < aor_size; aor_hex_cnt++) {
				aor_hex_next_up <<= (0x08 * aor_hex_cnt);
				aor_hex_next_up |= normal_tbl->aor[reverse_loop - 1][aor_hex_cnt];
			}

			/* ddi vertical porches are used for AOR interpolation.
			 * In case of 96/48hz mode, its base AOR came from below base RR mode.
			 * - 96hz: 120hz HS -> AOR_96hz = AOR_120hz * (vtotal_96hz) / (vtotal_120hz)
			 * - 48hz: 60hz normal -> AOR_48hz = AOR_60hz * (vtotal_48hz) / (vtotal_60hz)
			 * If there is no base vertical porches, (ex: ddi_vbp_base) in panel dtsi,
			 * parser function set base value as target value (ex: ddi_vbp_base = ddi_vbp).
			 */
			aor_hex_next_up = (aor_hex_next_up * ddi_tot_v) / ddi_tot_v_base;
		}

		/* use reverse_loop by flash aor data ordering for gamma */

		platform_up *= MULTIPLY_x10000;
		platform_down *= MULTIPLY_x10000;

		/* gamma interpolation for all steps */
		max_gamma = normal_tbl->gamma[reverse_loop];
		if (reverse_loop == normal_table_size - 1)
			min_gamma = max_gamma;
		else
			min_gamma = normal_tbl->gamma[reverse_loop + 1];

		vdd->panel_func.convert_GAMMA_to_V(max_gamma, max_gammaV);
		vdd->panel_func.convert_GAMMA_to_V(min_gamma, min_gammaV);

		max_cd = normal_tbl->candela_table[reverse_loop];
		if (reverse_loop == normal_table_size - 1)
			min_cd = max_cd;
		else
			min_cd = normal_tbl->candela_table[reverse_loop + 1];

		dimming_mode_curr = DIMMING_MODE_MAX;

		/* AOR/GAMMA interpolation steps
		 * Do interpoation for all aor/gamma of step.
		 */
		for (step = 0; step < normal_table[loop].steps; step++) {
			platform_curr = normal_itp->br_aor_table[index].platform_level_x10000;

			if (step == normal_table[loop].steps - 1)
				dimming_mode_curr = SS_FLASH_DIMMING_MODE;

			normal_itp->br_aor_table[index].dimming_mode = dimming_mode_curr;

			if (dimming_mode_curr == SS_FLASH_DIMMING_MODE) {
				aor_dec_curr_x10000 = aor_dec_up_x10000;
				memcpy(normal_itp->gamma[index], normal_tbl->gamma[reverse_loop], gamma_size);
			} else {
				/* gamma interpolation */
				for (gamma_loop = 0; gamma_loop < gamma_V_size; gamma_loop++) {
					itp_gammaV[gamma_loop] = gamma_interpolation(
									max_gammaV[gamma_loop], min_gammaV[gamma_loop],
									max_cd, min_cd,
									normal_itp->br_aor_table[index].interpolation_br_x10000);
				}

				/* Make GAMMA reg packet format from V format */
				vdd->panel_func.convert_V_to_GAMMA(itp_gammaV, normal_itp->gamma[index]);

				/* A_DIMMING */
				aor_dec_curr_x10000 = A_DIMMING_AOR_CAL (
						aor_dec_up_x10000, aor_dec_down_x10000,
						platform_up, platform_down, platform_curr);
			}

			normal_itp->br_aor_table[index].aor_percent_x10000 = aor_dec_curr_x10000;
			normal_itp->br_aor_table[index].aor_hex = AOR_PERCENT_X1000_TO_HEX(ddi_tot_v, aor_dec_curr_x10000);

			/* To convert dec to hex string format */
			convert_dec_to_hex_str(
				normal_itp->br_aor_table[index].aor_hex,
				aor_size,
				normal_itp->br_aor_table[index].aor_hex_string);

			LCD_DEBUG("AOR index : %3d  hex: 0x%04x percent_x10000: %6d mode : %s\n", index,
				normal_itp->br_aor_table[index].aor_hex,
				normal_itp->br_aor_table[index].aor_percent_x10000,
				ss_dimming_mode_debug[dimming_mode_curr]);

			/* print interpolated gamma */
			for (i = 0; i < gamma_size; i++)
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %2x", normal_itp->gamma[index][i]);
			LCD_DEBUG("[gamma_itp] [%3d]  %s\n", index, pBuffer);
			memset(pBuffer, 0x00, 256);

			index++;
		}
	}

	kfree(max_gammaV);
	kfree(min_gammaV);
	kfree(itp_gammaV);

	return 0;
}

static void update_candela_map_table(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	struct candela_map_table *table;
	int interpolation_step;
	struct ss_normal_interpolation_br_aor_table *br_aor_table;
	struct ss_normal_interpolation *norma_itp;

	int column;
	int pre_lux = -1, lux_idx = -1;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		norma_itp = &br_tbl->flash_itp.normal;
	else
		norma_itp = &br_tbl->table_itp.normal;

	interpolation_step = norma_itp->brightness_step;
	br_aor_table = norma_itp->br_aor_table;

	if (vdd->br_info.common_br.pac)
		table = &vdd->br_info.candela_map_table[PAC_NORMAL][vdd->panel_revision];
	else
		table = &vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision];

	/* check table size */
	if (table->tab_size != interpolation_step) {
		LCD_INFO("alloc for pac_candela_map_table");

		table->tab_size = interpolation_step;
		/* scaled_idx */
		if (!IS_ERR_OR_NULL(table->scaled_idx))
			kfree(table->scaled_idx);
		table->scaled_idx = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);

		/* idx */
		if (!IS_ERR_OR_NULL(table->idx))
			kfree(table->idx);
		table->idx = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);

		/* from */
		if (!IS_ERR_OR_NULL(table->from))
			kfree(table->from);
		table->from = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);

		/* end */
		if (!IS_ERR_OR_NULL(table->end))
			kfree(table->end);
		table->end = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);

		/* cd */
		if (!IS_ERR_OR_NULL(table->cd))
			kfree(table->cd);
		table->cd = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);

		/* interpolation_cd (upscale not rounding)*/
		if (!IS_ERR_OR_NULL(table->interpolation_cd))
			kfree(table->interpolation_cd);
		table->interpolation_cd = kzalloc(interpolation_step * sizeof(int), GFP_KERNEL);
	}

	/* update table information */
	for (column = 0; column < table->tab_size; column++) {
		/* scaled_idx */
		table->scaled_idx[column] = column;

		/* idx */
		if (pre_lux != br_aor_table[column].lux_mode) {
			pre_lux = br_aor_table[column].lux_mode;
			lux_idx++;
		}
		table->idx[column] = lux_idx;

		/* from */
		if (column == 0)
			table->from[column] = 0;
		else
			table->from[column] = table->end[column - 1] + 1;

		/* end */
		if (interpolation_step < CONTROL_AUTO_BRIGHTNESS_V4)
			table->end[column] = br_aor_table[column].platform_level_x10000 / MULTIPLY_x10000;
		else
			table->end[column] = br_aor_table[column].platform_level_x10000 / MULTIPLY_x100;

		/* cd */
		table->cd[column] = br_aor_table[column].lux_mode;

		/* interpolation_cd (upscale not rounding)*/
		table->interpolation_cd[column] = (br_aor_table[column].interpolation_br_x10000 + (MULTIPLY_x10000 - 1)) / MULTIPLY_x10000;
	}

	/* min_lv */
	table->min_lv = table->from[0];

	/* max_lv */
	table->max_lv = table->end[table->tab_size-1];
}

static void update_normal_interpolation(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size)
{
	/* 1st */
	gen_normal_interpolation_platform_level_lux_mode(vdd, br_tbl, normal_table, normal_table_size);

	/* 2st */
	gen_normal_interpolation_br(vdd, br_tbl, normal_table, normal_table_size);

	/* 3st */
	if (vdd->old_aor_dimming) /* Bloom5G needs old style s-dimming and a-dimming */
		gen_normal_interpolation_aor_gamma_legacy(vdd, br_tbl, normal_table, normal_table_size);
	else
		gen_normal_interpolation_aor_gamma(vdd, br_tbl, normal_table, normal_table_size);

	/* 4st */
	if (vdd->panel_func.gen_normal_interpolation_irc)
		vdd->panel_func.gen_normal_interpolation_irc(vdd, br_tbl, normal_table, normal_table_size);
	else
		LCD_ERR("No gen_normal_interpolation_irc !!\n");

	/* 5st */
	update_candela_map_table(vdd, br_tbl);
}


void set_up_interpolation(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size,
		struct ss_interpolation_brightness_table *hbm_table, int hbm_table_size)
{
	/* init samsung normal interpolation data */
	init_normal_interpolation(vdd, br_tbl, normal_table, normal_table_size);
	update_normal_interpolation(vdd, br_tbl, normal_table, normal_table_size);

	/* init samsung hbm interpolation data */
	init_hbm_interpolation(vdd, br_tbl, hbm_table, hbm_table_size);
	update_hbm_interpolation(vdd, br_tbl, hbm_table, hbm_table_size);
}

static int find_hbm_candela(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int hbm_brightness_step = vdd->br_info.hbm_brightness_step;
	int index = -1;
	int candela = vdd->br_info.common_br.cd_level;
	int loop;
	struct dimming_tbl *hbm_tbl = &br_tbl->hbm_tbl;

	for(loop = 0; loop < hbm_brightness_step; loop++)
		if (candela == hbm_tbl->candela_table[loop]) {
			index = loop;
			break;
		}

	/* find the high bound closed index */
	if (index < 0) {
		for(loop = hbm_brightness_step - 1; loop >= 0; loop--)
			if (hbm_tbl->candela_table[loop] - candela >= 0) {
				LCD_DEBUG("index : %d lux : %d vdd_lux : %d\n", loop, hbm_tbl->candela_table[loop], candela);
				index = loop;
				break;
			}
	}

	if (index < 0) {
		LCD_INFO("fail to find %d candela at hbm.candela_table\n", candela);
	}

	return index;
}

static int find_normal_candela(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int normal_brightness_step = vdd->br_info.normal_brightness_step;
	int index = -1;
	int candela = vdd->br_info.common_br.cd_level;
	int loop;
	struct dimming_tbl *normal_tbl = &br_tbl->normal_tbl;

	for(loop = 0; loop < normal_brightness_step; loop++)
		if (candela == normal_tbl->candela_table[loop]) {
			index = loop;
			break;
		}

	/* find the high bound closed index */
	if (index < 0) {
		for(loop = normal_brightness_step - 1; loop >= 0; loop--)
			if (normal_tbl->candela_table[loop] - candela >= 0) {
				LCD_DEBUG("index : %d lux : %d vdd_lux : %d\n", loop, normal_tbl->candela_table[loop], candela);
				index = loop;
				break;
			}
	}

	if (index < 0) {
		LCD_INFO("fail to find %d candela at normal.candela_table\n", candela);
	}

	return index;
}

static int find_hmd_candela(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int hmd_brightness_step = vdd->br_info.hmd_brightness_step;
	int index = -1;
	int candela = vdd->br_info.common_br.interpolation_cd;
	int loop;
	struct dimming_tbl *hmd_tbl = &br_tbl->hmd_tbl;

	for(loop = 0; loop < hmd_brightness_step; loop++)
		if (candela == hmd_tbl->candela_table[loop]) {
			index = loop;
			break;
		}

	if (index < 0) {
		LCD_INFO("fail to find %d candela at hmd.candela_table\n", candela);
	}

	return index;
}

static int find_hbm_interpolation_candela(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	struct ss_interpolation *ss_itp;
	int hbm_interpolation_brightness_step;
	int index = -1;
	int candela = vdd->br_info.common_br.cd_level;
	int loop;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION) {
		ss_itp = &br_tbl->flash_itp;
	} else {
		ss_itp = &br_tbl->table_itp;
	}

	hbm_interpolation_brightness_step = ss_itp->hbm.brightness_step;

	for (loop = 0; loop < hbm_interpolation_brightness_step; loop++)
		if (candela == ss_itp->hbm.br_table[loop].lux_mode) {
			index = loop;
			break;
		}

	if (index < 0) {
		LCD_INFO("fail to find %d candela at interpolation.hbm.candela_table\n", candela);
	}

	return index;
}

void copy_cmd_debug(char *debug_str, char *cmds, int cmd_size)
{
	int data_cnt;
	char buf[FLASH_GAMMA_DBG_BUF_SIZE];

	memset(buf, '\n', sizeof(buf));

	snprintf(buf, FLASH_GAMMA_DBG_BUF_SIZE, "%s : ", debug_str);

	for (data_cnt = 0; data_cnt < cmd_size; data_cnt++)
		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf),
				"%02x ", cmds[data_cnt]);

	LCD_INFO("%s\n", buf);

}

// TODO: get proper gamma size for each panel..
#define GAMMA_V_SIZE_TMP	36 /* V_MAX * RGB_MAX */
int ss_gamma_itp_based_fps(struct samsung_display_driver_data *vdd,
			int fps_start, int fps_end, int fps_itp, bool sot_hs,
			int pac_cd_idx, char *out_buf)
{
	struct brightness_table *br_tbl_start = ss_get_br_tbl(vdd, fps_start, sot_hs);
	struct brightness_table *br_tbl_end = ss_get_br_tbl(vdd, fps_end, sot_hs);
	struct ss_interpolation *ss_itp_start;
	struct ss_interpolation *ss_itp_end;

	u8 *gamma_start;
	u8 *gamma_end;

	static int *gammaV_start;
	static int *gammaV_end;
	static int *gammaV_itp;

	int gamma_loop;

	int gamma_V_size = vdd->panel_func.get_gamma_V_size();

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION) {
		ss_itp_start = &br_tbl_start->flash_itp;
		ss_itp_end = &br_tbl_end->flash_itp;
	} else {
		ss_itp_start = &br_tbl_start->table_itp;
		ss_itp_end = &br_tbl_end->table_itp;
	}

	gamma_start = ss_itp_start->normal.gamma[pac_cd_idx];
	gamma_end = ss_itp_end->normal.gamma[pac_cd_idx];

	if (unlikely(!gammaV_start))
		gammaV_start = kzalloc(gamma_V_size * sizeof(int), GFP_KERNEL);
	if (unlikely(!gammaV_end))
		gammaV_end = kzalloc(gamma_V_size * sizeof(int), GFP_KERNEL);
	if (unlikely(!gammaV_itp))
		gammaV_itp = kzalloc(gamma_V_size * sizeof(int), GFP_KERNEL);

	if (unlikely(!gammaV_start || !gammaV_end || !gammaV_itp)) {
		LCD_ERR("fail to alloc gammaV memory\n");
		return -ENOMEM;
	}

	vdd->panel_func.convert_GAMMA_to_V(gamma_start, gammaV_start);
	vdd->panel_func.convert_GAMMA_to_V(gamma_end, gammaV_end);

	for (gamma_loop = 0; gamma_loop < GAMMA_V_SIZE_TMP; gamma_loop++)
		gammaV_itp[gamma_loop] =
			ss_common_interpolation(gammaV_end[gamma_loop],
						gammaV_start[gamma_loop],
						fps_end, fps_start,
						fps_itp);

	vdd->panel_func.convert_V_to_GAMMA(gammaV_itp, out_buf);

	return 0;
}

int br_interpolation_generate_event(struct samsung_display_driver_data *vdd,
		enum GEN_INTERPOLATION_EVENT event, char *buf)
{
	int candela_index = -1;
	int gamma_size = vdd->br_info.gamma_size;
	int elvss_size = vdd->br_info.elvss_size / INTERPOLATION_ELVSS_MAX_TEMP;
	int vint_size = vdd->br_info.vint_size;
	int aor_size = vdd->br_info.aor_size;
	int irc_size = vdd->br_info.irc_size;
	int dbv_size = vdd->br_info.dbv_size;
	struct ss_interpolation *ss_itp;

	struct brightness_table *br_tbl;
	struct dimming_tbl *hbm_tbl;
	struct dimming_tbl *normal_tbl;
	struct dimming_tbl *hmd_tbl;

	enum SS_BRR_MODE brr_mode = vdd->vrr.brr_mode;

	int pac_cd_idx = vdd->br_info.common_br.pac_cd_idx;

	/* select brightness table for current refresh rate mode */
	br_tbl = ss_get_cur_br_tbl(vdd);
	if (!br_tbl) {
		LCD_ERR("br tble is null!\n");
		return -ENODEV;
	}

	hbm_tbl = &br_tbl->hbm_tbl;
	normal_tbl = &br_tbl->normal_tbl;
	hmd_tbl = &br_tbl->hmd_tbl;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION) {
		ss_itp = &br_tbl->flash_itp;
	} else {
		ss_itp = &br_tbl->table_itp;
	}

	switch (event) {
	case GEN_HBM_GAMMA:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_NORMAL_GAMMA:
		candela_index = find_normal_candela(vdd, br_tbl);
		if (candela_index >= 0)
			memcpy(buf, normal_tbl->gamma[candela_index], gamma_size);  // table need to copy
		break;

	case GEN_NORMAL_INTERPOLATION_GAMMA:
#if defined(CONFIG_PANEL_S6E3HAB_AMB677TY01_WQHD) || defined (CONFIG_PANEL_S6E3HAB_AMB623TS01_WQHD) || defined (CONFIG_PANEL_S6E3HAB_AMB687TZ01_WQHD)
		/* ID3 03 panel:
		 * 48MTPnm : 60MTPnm x 110MTPhs / 120MTPhs
		 * 96MTPhs = if brt >= 98nit : 100MTPhs  else : 110MTPhs (98nit = 10601~10800 platform level)
		 */
		if ((get_lcd_attached("GET") & 0xF) >= 3 &&
				(vdd->vrr.cur_refresh_rate == 48 ||
				 brr_mode == BRR_48_60 || brr_mode == BRR_60_48)) {
			struct ss_interpolation *ss_itp_target;

			u8 gamma_110hs[34];
			u8 gamma_48nm[34];
			u8 *gamma_60nm;
			u8 *gamma_120hs;

			int gammaV_110hs[GAMMA_V_SIZE_TMP];
			int gammaV_48nm[GAMMA_V_SIZE_TMP];
			int gammaV_60nm[GAMMA_V_SIZE_TMP];
			int gammaV_120hs[GAMMA_V_SIZE_TMP];

			struct brightness_table *br_tbl_target;
			int gamma_loop;

			/* get 60nm gamma */
			br_tbl_target = ss_get_br_tbl(vdd, 60, false);
			if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
				ss_itp_target = &br_tbl_target->flash_itp;
			else
				ss_itp_target = &br_tbl_target->table_itp;
			gamma_60nm = ss_itp_target->normal.gamma[pac_cd_idx];

			/* get 60nm gamma */
			br_tbl_target = ss_get_br_tbl(vdd, 120, true);
			if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
				ss_itp_target = &br_tbl_target->flash_itp;
			else
				ss_itp_target = &br_tbl_target->table_itp;
			gamma_120hs = ss_itp_target->normal.gamma[pac_cd_idx];

			/* get 110hs gamma */
			ss_gamma_itp_based_fps(vdd, 60, 120, 110, true, pac_cd_idx, gamma_110hs);

			/* calculate 48nm gamma: 48MTPnm = 60MTPnm x 110MTPhs / 120MTPhs */
			vdd->panel_func.convert_GAMMA_to_V(gamma_110hs, gammaV_110hs);
			vdd->panel_func.convert_GAMMA_to_V(gamma_48nm, gammaV_48nm);
			vdd->panel_func.convert_GAMMA_to_V(gamma_60nm, gammaV_60nm);
			vdd->panel_func.convert_GAMMA_to_V(gamma_120hs, gammaV_120hs);

			for (gamma_loop = 0; gamma_loop < GAMMA_V_SIZE_TMP; gamma_loop++) {
				int dummy_round_off = gammaV_120hs[gamma_loop] / 2;

				/* 48MTPnm : 60MTPnm x 110MTPhs / 120MTPhs */
				gammaV_48nm[gamma_loop] =
					(gammaV_60nm[gamma_loop] * gammaV_110hs[gamma_loop] + dummy_round_off) /
					gammaV_120hs[gamma_loop];
			}

			vdd->panel_func.convert_V_to_GAMMA(gammaV_48nm, gamma_48nm);

			/* save gamma to 48hz or 96hz br_tbl */
			LCD_INFO("VRR: save gamma to 48hz br_tbl\n");
			br_tbl_target = ss_get_br_tbl(vdd, 48, vdd->vrr.cur_sot_hs_mode);
			if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
				ss_itp_target = &br_tbl_target->flash_itp;
			else
				ss_itp_target = &br_tbl_target->table_itp;

			memcpy(ss_itp_target->normal.gamma[pac_cd_idx], gamma_48nm, gamma_size);

		} else if ((get_lcd_attached("GET") & 0xF) >= 3 &&
				(vdd->vrr.cur_refresh_rate == 96 ||
					brr_mode == BRR_96_120 || brr_mode == BRR_120_96 ||
					brr_mode == BRR_60HS_96 || brr_mode == BRR_96_60HS)) {
			struct ss_interpolation *ss_itp_target;

			u8 gamma_target[34];
			int fps_itp;
			int bl = vdd->br_info.common_br.bl_level;
			struct brightness_table *br_tbl_target;

			if (vdd->vrr.cur_refresh_rate == 96 && bl > 10800)
				fps_itp = 100;
			else
				fps_itp = 110;

			/* get 110hz or 100hz gamma */
			ss_gamma_itp_based_fps(vdd, 60, 120, fps_itp, true, pac_cd_idx, gamma_target);

			/* save gamma to 48hz or 96hz br_tbl */

			LCD_INFO("VRR: save %dhz gamma to 96hz br_tbl\n", fps_itp);
			br_tbl_target = ss_get_br_tbl(vdd, 96, vdd->vrr.cur_sot_hs_mode);
			if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
				ss_itp_target = &br_tbl_target->flash_itp;
			else
				ss_itp_target = &br_tbl_target->table_itp;

			memcpy(ss_itp_target->normal.gamma[pac_cd_idx], gamma_target, gamma_size);
		}
#endif

		if (vdd->vrr.is_support_brr && ss_is_brr_on(brr_mode)) {
			/* Bridge RR mode
			 * All BRR mode: aor/gamma interpolation based on VFP duty.
			 * Other values: use 60NM/120HS bl_tbl
			 */
			struct vrr_bridge_rr_tbl *brr = &vdd->vrr.brr_tbl[brr_mode];
			int fps_start;
			int fps_end;
			int fps_itp = vdd->vrr.cur_refresh_rate;

			/* TODO: refactoring BRR for relay mode... */
			if (brr_mode == BRR_60HS_96 || brr_mode == BRR_96_60HS) {
				struct vrr_bridge_rr_tbl *brr_60_120 = &vdd->vrr.brr_tbl[BRR_60HS_120];
				int i;

				fps_start = 96; /* bridge fps: 96hz ~ 120hz */
				for (i = 0; i < brr_60_120->tot_steps; i++) {
					if (fps_itp == brr_60_120->fps[i]) {
						/* bridge fps: 60hz ~ 120hz */
						fps_start = 60;
						break;
					}
				}

				fps_end = 120;
			} else {
				fps_start = brr->fps_start;
				fps_end = brr->fps_end;
			}

			ss_gamma_itp_based_fps(vdd, fps_start, fps_end, fps_itp,
					brr->sot_hs_base, pac_cd_idx, buf);

		} else {
			memcpy(buf, ss_itp->normal.gamma[pac_cd_idx], gamma_size);
		}

		break;

	case GEN_HMD_GAMMA:
		candela_index = find_hmd_candela(vdd, br_tbl);
		if (candela_index >= 0)
			memcpy(buf, hmd_tbl->gamma[candela_index], gamma_size); // table need to copy
		break;
	case GEN_HBM_INTERPOLATION_GAMMA:
		candela_index = find_hbm_interpolation_candela(vdd, br_tbl);
		if (candela_index >= 0)
			memcpy(buf, ss_itp->hbm.gamma[candela_index], gamma_size);
		break;
	case GEN_HBM_AOR:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_NORMAL_AOR:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_HMD_AOR:
		candela_index = find_hmd_candela(vdd, br_tbl);
		if (candela_index >= 0)
			memcpy(buf, hmd_tbl->aor[candela_index], aor_size);
		break;
	case GEN_NORMAL_INTERPOLATION_AOR:
		if (vdd->vrr.is_support_brr && ss_is_brr_on(brr_mode)) {
			/* Bridge RR mode
			 * - all BRR mode: aor interpolation based on VFP duty. other values: use 60hz bl_tbl
			 * - 96/120 HS: gamma interpolation based on FPS duty. other values: use 120hz bl_tbl
			 */
			struct vrr_bridge_rr_tbl *brr = &vdd->vrr.brr_tbl[brr_mode];
			struct brightness_table *br_tbl_base = ss_get_br_tbl(vdd, brr->fps_base, brr->sot_hs_base);
			struct ss_interpolation *ss_itp_base;
			unsigned int aor_base, aor_itp;
			unsigned char aor_hex_string_itp[AOR_HEX_STRING_CNT];
			unsigned int aor_percent_x10000_itp;

			int ddi_tot_v, ddi_tot_v_base;
			int vfp_target, vbp_target, vactive_target, fps_target;
			int vfp_base, vbp_base, vactive_base, fps_base;

			if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
				ss_itp_base = &br_tbl_base->flash_itp;
			else
				ss_itp_base = &br_tbl_base->table_itp;

			aor_base = ss_itp_base->normal.br_aor_table[pac_cd_idx].aor_hex;

			vbp_target = brr->vbp_base;
			vactive_target = brr->vactive_base;
			fps_target = vdd->vrr.cur_refresh_rate;

			vfp_base = brr->vfp_base;
			vbp_base = brr->vbp_base;
			vactive_base = brr->vactive_base;
			fps_base = brr->fps_base;

			vfp_target = ((vbp_base + vactive_base + vfp_base) * fps_base / fps_target) -
					(vbp_target + vactive_target);

			ddi_tot_v_base = vfp_base + vactive_base + vbp_base;
			ddi_tot_v = vfp_target + vactive_target + vbp_target;


			aor_itp = (aor_base * ddi_tot_v) / ddi_tot_v_base;

#if defined(CONFIG_PANEL_S6E3HAB_AMB677TY01_WQHD) || defined (CONFIG_PANEL_S6E3HAB_AMB623TS01_WQHD) || defined (CONFIG_PANEL_S6E3HAB_AMB687TZ01_WQHD)
			if (fps_target == 70) {
				aor_itp += 14;
				LCD_INFO("VRR: AOR offset +14 for 70hz\n");
			} else if (fps_target == 100) {
				aor_itp -= 1;
				LCD_INFO("VRR: AOR offset -1 for 100hz\n");
			}
#endif

			/* To convert dec to hex string format */
			convert_dec_to_hex_str(aor_itp, aor_size, aor_hex_string_itp);
			memcpy(buf, aor_hex_string_itp, aor_size);

			aor_percent_x10000_itp = (ss_itp_base->normal.br_aor_table[pac_cd_idx].aor_percent_x10000 * ddi_tot_v) / ddi_tot_v_base;
			vdd->br_info.common_br.aor_data = aor_percent_x10000_itp / 100;
		} else {
			memcpy(buf, ss_itp->normal.br_aor_table[pac_cd_idx].aor_hex_string, aor_size);
			vdd->br_info.common_br.aor_data = ss_itp->normal.br_aor_table[pac_cd_idx].aor_percent_x10000 / 100;
		}
		break;
	case GEN_HBM_INTERPOLATION_AOR:
		candela_index = find_hbm_candela(vdd, br_tbl);
		if (candela_index >= 0)
			memcpy(buf, hbm_tbl->aor[candela_index], aor_size);
		break;

	case GEN_NORMAL_DBV:
		candela_index = find_normal_candela(vdd, br_tbl);
		if (candela_index >= 0)
			memcpy(buf, normal_tbl->dbv[candela_index], dbv_size);
		break;

	case GEN_HBM_VINT:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_NORMAL_VINT:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_HMD_VINT:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_NORMAL_INTERPOLATION_VINT:
		candela_index = find_normal_candela(vdd, br_tbl);
		if (candela_index >= 0)
			memcpy(buf, normal_tbl->vint[candela_index], vint_size);
		break;
	case GEN_HBM_INTERPOLATION_VINT:
		candela_index = find_hbm_candela(vdd, br_tbl);
		if (candela_index >= 0)
			memcpy(buf, hbm_tbl->vint[candela_index], vint_size);
		break;
	case GEN_HBM_ELVSS:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_NORMAL_ELVSS:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_HMD_ELVSS:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_NORMAL_INTERPOLATION_ELVSS:
		candela_index = find_normal_candela(vdd, br_tbl);
		if (candela_index >= 0) {
			if (vdd->br_info.temperature > 0) {
				memcpy(buf, &normal_tbl->elvss[candela_index][INTERPOLATION_ELVSS_HIGH_TEMP], elvss_size);
			}else if (vdd->br_info.temperature > vdd->br_info.common_br.elvss_interpolation_temperature) {
				memcpy(buf, &normal_tbl->elvss[candela_index][INTERPOLATION_ELVSS_MID_TEMP], elvss_size);
			} else {
				memcpy(buf, &normal_tbl->elvss[candela_index][INTERPOLATION_ELVSS_LOW_TEMP], elvss_size);
			}
		}
		break;
	case GEN_HBM_INTERPOLATION_ELVSS:
		candela_index = find_hbm_candela(vdd, br_tbl);
		if (candela_index >= 0) {
			if (vdd->br_info.temperature > 0) {
				memcpy(buf, &hbm_tbl->elvss[candela_index][INTERPOLATION_ELVSS_HIGH_TEMP], elvss_size);
			}else if (vdd->br_info.temperature > vdd->br_info.common_br.elvss_interpolation_temperature) {
				memcpy(buf, &hbm_tbl->elvss[candela_index][INTERPOLATION_ELVSS_MID_TEMP], elvss_size);
			} else {
				memcpy(buf, &hbm_tbl->elvss[candela_index][INTERPOLATION_ELVSS_LOW_TEMP], elvss_size);
			}
		}
		break;
	case GEN_HBM_IRC:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_NORMAL_IRC:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_HMD_IRC:
		LCD_INFO("not support event=%d\n", event);
		break;
	case GEN_NORMAL_INTERPOLATION_IRC:
		memcpy(buf, ss_itp->normal.irc[pac_cd_idx], irc_size);
		break;
	case GEN_HBM_INTERPOLATION_IRC:
		candela_index = find_hbm_interpolation_candela(vdd, br_tbl);
		if (candela_index >= 0)
			memcpy(buf, ss_itp->hbm.irc[candela_index], irc_size);

		//copy_cmd_debug("GEN_HBM_INTERPOLATION_IRC", buf, irc_size);
		break;
	default:
		LCD_INFO("unhandled event=%d\n", event);
		break;
	}

	LCD_DEBUG("event=%d candela_index:%d\n", event, candela_index);

	return candela_index;
}

static void debug_normal_interpolation(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	char buf[FLASH_GAMMA_DBG_BUF_SIZE];
	struct ss_normal_interpolation *normal_itp;
	int column, brightness_step, data_cnt;

	int gamma_size = vdd->br_info.gamma_size;
	int aor_size = vdd->br_info.aor_size;
	unsigned char **gamma;

	int irc_size = vdd->br_info.irc_size;
	unsigned char **irc;

	struct candela_map_table *table;

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		normal_itp = &br_tbl->flash_itp.normal;
	else
		normal_itp = &br_tbl->table_itp.normal;

	brightness_step = normal_itp->brightness_step;
	gamma = normal_itp->gamma;
	irc = normal_itp->irc;

	for (column = 0; column < brightness_step; column++) {
		LCD_INFO("index: %3d Platform_x1000: %7d lux_mode: %3d BR_x1000: %7d AOR_x10000: %7d 0x%04X dimmng: %s\n",
				column,
				normal_itp->br_aor_table[column].platform_level_x10000,
				normal_itp->br_aor_table[column].lux_mode,
				normal_itp->br_aor_table[column].interpolation_br_x10000,
				normal_itp->br_aor_table[column].aor_percent_x10000,
				normal_itp->br_aor_table[column].aor_hex,
				ss_dimming_mode_debug[normal_itp->br_aor_table[column].dimming_mode]);
	}

	memset(buf, '\n', sizeof(buf));

	LCD_INFO("print interpolation data\n");
	LCD_INFO("GAMMA(%d) AOR(%d) IRC(%d)\n", gamma_size, aor_size, irc_size);

	for (column = brightness_step - 1; column >= 0; column--) {
		snprintf(buf, FLASH_GAMMA_DBG_BUF_SIZE, "NORMAL [%3d][%3d] ", column, normal_itp->br_aor_table[column].lux_mode);

		/* GAMMA */
		if (!IS_ERR_OR_NULL(gamma)) {
			for (data_cnt = 0; data_cnt < gamma_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", gamma[column][data_cnt]);
		}

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* AOR */
		if (!IS_ERR_OR_NULL(normal_itp->br_aor_table)) {
			for (data_cnt = 0; data_cnt < aor_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", normal_itp->br_aor_table[column].aor_hex_string[data_cnt]);
		} else
			LCD_ERR("aor_table is null.. %d", column);

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* IRC */
		if (!IS_ERR_OR_NULL(irc)) {
			for (data_cnt = 0; data_cnt < irc_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02X ", irc[column][data_cnt]);
		}

		LCD_INFO("%s\n", buf);
		memset(buf, '\n', strlen(buf));
	}

	/* candela_map_table debug */
	if (vdd->br_info.common_br.pac)
		table = &vdd->br_info.candela_map_table[PAC_NORMAL][vdd->panel_revision];
	else
		table = &vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision];

	LCD_INFO("/* <scaled idx> <idx>  <from>  <end> <cd> <interpolation cd>*/\n");

	for (column =  0; column < table->tab_size; column++) {
		LCD_INFO("%4d %2d %5d %5d %3d %3d\n",
			table->scaled_idx[column],
			table->idx[column],
			table->from[column],
			table->end[column],
			table->cd[column],
			table->interpolation_cd[column]);
	}
}

static void debug_hbm_interpolation(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	char buf[FLASH_GAMMA_DBG_BUF_SIZE];
	struct ss_hbm_interpolation *hbm_itp;
	int column, data_cnt;

	int brightness_step;

	int gamma_size = vdd->br_info.gamma_size;
	unsigned char **gamma;

	int irc_size = vdd->br_info.irc_size;
	unsigned char **irc;

	struct candela_map_table *table;

	memset(buf, '\n', sizeof(buf));

	if (vdd->br_info.panel_br_info.itp_mode == FLASH_INTERPOLATION)
		hbm_itp = &br_tbl->flash_itp.hbm;
	else
		hbm_itp = &br_tbl->table_itp.hbm;

	brightness_step = hbm_itp->brightness_step;
	gamma = hbm_itp->gamma;
	irc = hbm_itp->irc;

	/* Platform Level, Candela */
	for (column =  0; column < brightness_step; column++) {
		LCD_INFO("index: %3d Platform_x10000: %7d lux_mode_x10000: %3d\n",
			column,
			hbm_itp->br_table[column].platform_level_x10000,
			hbm_itp->br_table[column].interpolation_br_x10000);
	}

	LCD_INFO("print interpolation data\n");
	LCD_INFO("GAMMA(%d) IRC(%d)\n", gamma_size, irc_size);

	for (column = brightness_step - 1; column >= 0; column--) {
		snprintf(buf, FLASH_GAMMA_DBG_BUF_SIZE, "HBM [%3d][%3d] ", column, hbm_itp->br_table[column].lux_mode);

		/* GAMMA */
		if (!IS_ERR_OR_NULL(gamma)) {
			for (data_cnt = 0; data_cnt < gamma_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", gamma[column][data_cnt]);
		}

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* IRC */
		if (!IS_ERR_OR_NULL(irc)) {
			for (data_cnt = 0; data_cnt < irc_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02X ", irc[column][data_cnt]);
		}

		LCD_INFO("%s\n", buf);
		memset(buf, '\n', strlen(buf));
 	}

	/* candela_map_table */
	if (vdd->br_info.common_br.pac)
		table = &vdd->br_info.candela_map_table[PAC_HBM][vdd->panel_revision];
	else
		table = &vdd->br_info.candela_map_table[HBM][vdd->panel_revision];

	LCD_INFO("< idx from end cd auto >\n");

	for (column =  0; column < table->tab_size; column++) {
		LCD_INFO("%2d %d %d %d %2d\n",
			table->idx[column],
			table->from[column],
			table->end[column],
			table->cd[column],
			table->auto_level[column]);
	}
}

void debug_interpolation_log(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	debug_normal_interpolation(vdd, br_tbl);
	debug_hbm_interpolation(vdd, br_tbl);
}

/*
 * gamma_interpolation() - interpolate gamma.
 * Taget Gamma reg = upper Gamma - ((upper cd - Target cd) / (upper cd - lower cd)) * (upper Gamma - lower Gamma)
 * Target_cd has up to two decimal places. (ex. 7238600 (723.86))
 */
uint gamma_interpolation(int upper_g, int lower_g, int upper_cd, int lower_cd, int target_cd)
{
	uint ret = 0;

	ret = (upper_g * MULTIPLY_x10000) -
		((upper_cd * MULTIPLY_x10000 - target_cd) * (upper_g - lower_g)) /
		(upper_cd - lower_cd);
	ret = ROUNDING(ret, MULTIPLY_x10000);
	ret /= MULTIPLY_x10000;

 	return ret;
}

int ss_common_interpolation(s64 y2, s64 y1, s64 x2, s64 x1, s64 target_x)
{
	s64 itp_v;

	itp_v = (y2 * MULTIPLY_x10000) -
		((x2 - target_x) * (y2 - y1) * MULTIPLY_x10000) /
		(x2 - x1);
	itp_v = ROUNDING(itp_v, MULTIPLY_x10000);
	itp_v /= MULTIPLY_x10000;

 	return itp_v;
}
