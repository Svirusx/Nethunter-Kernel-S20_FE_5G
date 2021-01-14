 /* =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: jb09.kim@samsung.com
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
#include "ss_flash_table_data_common.h"

static void init_br_info(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	struct flash_raw_table *gamma_tbl = br_tbl->gamma_tbl;
	int gamma_size = vdd->br_info.gamma_size;
	int aor_size = vdd->br_info.aor_size;
	int vint_size = vdd->br_info.vint_size;
	int elvss_size = vdd->br_info.elvss_size;
	int irc_size = vdd->br_info.irc_size;

	int hbm_size = vdd->br_info.hbm_brightness_step * (aor_size + vint_size + elvss_size + irc_size);
	int normal_size = vdd->br_info.normal_brightness_step * (gamma_size + aor_size + vint_size + elvss_size + irc_size);
	int hmd_size = vdd->br_info.hmd_brightness_step * (gamma_size + aor_size);

	gamma_tbl->br_data_size = hbm_size + normal_size + hmd_size;

	LCD_INFO("vrr: %dhz, hbm_size (%d) = hbm_brightness_step(%d) * (aor_size (%d) + vint_size (%d) + elvss_size (%d) + irc_size (%d))\n",
		br_tbl->refresh_rate, hbm_size, vdd->br_info.hbm_brightness_step, aor_size, vint_size, elvss_size, irc_size);
	LCD_INFO("normal_size (%d) = normal_brightness_step(%d) * (gamma_size (%d) + aor_size (%d) + vint_size (%d) + elvss_size (%d) + irc_size (%d))\n",
		normal_size, vdd->br_info.normal_brightness_step, gamma_size, aor_size, vint_size, elvss_size, irc_size);
	LCD_INFO("hmd_size (%d) = hmd_brightness_step(%d) * (gamma_size (%d) + aor_size (%d))\n",
		hmd_size, vdd->br_info.hmd_brightness_step, gamma_size, aor_size);
	LCD_INFO("br_data_size (%d) = hbm_size (%d) + normal_size (%d) + hmd_size (%d)\n",
		gamma_tbl->br_data_size, hbm_size, normal_size, hmd_size);

	/*
		free alloc memory : test purpose for slab memory integrity
	*/
	if (!IS_ERR_OR_NULL(gamma_tbl->br_data_raw)) {
		kfree(gamma_tbl->br_data_raw);
		gamma_tbl->br_data_raw = NULL;
	}

	gamma_tbl->br_data_raw =
		kzalloc(gamma_tbl->br_data_size * sizeof(char), GFP_KERNEL);

	LCD_INFO("hbm step (%d), noarml step (%d), hmd step (%d)\n",
		vdd->br_info.hbm_brightness_step, vdd->br_info.normal_brightness_step, vdd->br_info.hmd_brightness_step);
	LCD_INFO("gamma (%d) vint (%d) elvss (%d) irc (%d) aor (%d)\n", gamma_size, vint_size, elvss_size,
		irc_size, aor_size);
	LCD_INFO("alloc size : %d (%d + %d + %d) \n", gamma_tbl->br_data_size,
		hbm_size, normal_size, hmd_size);
}

static void init_br_info_hbm(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int column;
	int hbm_brightness_step = vdd->br_info.hbm_brightness_step;
	int gamma_size = vdd->br_info.gamma_size;
	int aor_size = vdd->br_info.aor_size;
	int vint_size = vdd->br_info.vint_size;
	int elvss_size = vdd->br_info.elvss_size;
	int irc_size = vdd->br_info.irc_size;

	struct dimming_tbl *hbm_tbl = &br_tbl->hbm_tbl;

	/*
		free alloc memory : test purpose for slab memory integrity
	*/
	if (!IS_ERR_OR_NULL(hbm_tbl->candela_table)) {
		kfree(hbm_tbl->candela_table);
		hbm_tbl->candela_table = NULL;
	}

	if (!IS_ERR_OR_NULL(hbm_tbl->gamma)) {
		for (column = 0; column < hbm_brightness_step; column++)
			kfree(hbm_tbl->gamma[column]);
		kfree(hbm_tbl->gamma);
		hbm_tbl->gamma = NULL;
	}

	if (!IS_ERR_OR_NULL(hbm_tbl->aor)) {
		for (column = 0; column < hbm_brightness_step; column++)
			kfree(hbm_tbl->aor[column]);
		kfree(hbm_tbl->aor);
		hbm_tbl->aor = NULL;
	}

	if (!IS_ERR_OR_NULL(hbm_tbl->vint)) {
		for (column = 0; column < hbm_brightness_step; column++)
			kfree(hbm_tbl->vint[column]);
		kfree(hbm_tbl->vint);
		hbm_tbl->vint = NULL;
	}

	if (!IS_ERR_OR_NULL(hbm_tbl->elvss)) {
		for (column = 0; column < hbm_brightness_step; column++)
			kfree(hbm_tbl->elvss[column]);
		kfree(hbm_tbl->elvss);
		hbm_tbl->elvss = NULL;

	}

	if (!IS_ERR_OR_NULL(hbm_tbl->irc)) {
		for (column = 0; column < hbm_brightness_step; column++)
			kfree(hbm_tbl->irc[column]);
		kfree(hbm_tbl->irc);
		hbm_tbl->irc = NULL;
	}

	/*alloc 2 dimension matrix */
	if (!hbm_tbl->candela_table)
		hbm_tbl->candela_table = kzalloc(hbm_brightness_step * sizeof(int), GFP_KERNEL);

	if (test_bit(BR_HBM_GAMMA, vdd->br_info.panel_br_info.br_info_select)) {
		if (!hbm_tbl->gamma) {
			hbm_tbl->gamma = kzalloc(hbm_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < hbm_brightness_step; column++)
				hbm_tbl->gamma[column] = kzalloc(gamma_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_HBM_AOR, vdd->br_info.panel_br_info.br_info_select)) {
		if (!hbm_tbl->aor) {
			hbm_tbl->aor = kzalloc(hbm_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < hbm_brightness_step; column++)
				hbm_tbl->aor[column] = kzalloc(aor_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_HBM_VINT, vdd->br_info.panel_br_info.br_info_select)) {
		if (!hbm_tbl->vint) {
			hbm_tbl->vint = kzalloc(hbm_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < hbm_brightness_step; column++)
				hbm_tbl->vint[column] = kzalloc(vint_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_HBM_ELVSS, vdd->br_info.panel_br_info.br_info_select)) {
		if (!hbm_tbl->elvss) {
			hbm_tbl->elvss = kzalloc(hbm_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < hbm_brightness_step; column++)
				hbm_tbl->elvss[column] = kzalloc(elvss_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_HBM_IRC, vdd->br_info.panel_br_info.br_info_select)) {
		if (!hbm_tbl->irc) {
			hbm_tbl->irc = kzalloc(hbm_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < hbm_brightness_step; column++)
				hbm_tbl->irc[column] = kzalloc(irc_size, GFP_KERNEL);
		}
	}
}

static void init_br_info_normal(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int column;

	int normal_brightness_step = vdd->br_info.normal_brightness_step;
	int gamma_size = vdd->br_info.gamma_size;
	int aor_size = vdd->br_info.aor_size;
	int dbv_size = vdd->br_info.dbv_size;
	int vint_size = vdd->br_info.vint_size;
	int elvss_size = vdd->br_info.elvss_size;
	int irc_size = vdd->br_info.irc_size;

	struct dimming_tbl *normal_tbl = &br_tbl->normal_tbl;

	/* free alloc memory : test purpose for slab memory integrity */
	if (!IS_ERR_OR_NULL(normal_tbl->candela_table)) {
		kfree(normal_tbl->candela_table);
		normal_tbl->candela_table = NULL;
	}

	if (!IS_ERR_OR_NULL(normal_tbl->gamma)) {
		for (column = 0; column < normal_brightness_step; column++)
			kfree(normal_tbl->gamma[column]);
		kfree(normal_tbl->gamma);
		normal_tbl->gamma = NULL;
	}

	if (!IS_ERR_OR_NULL(normal_tbl->aor)) {
		for (column = 0; column < normal_brightness_step; column++)
			kfree(normal_tbl->aor[column]);
		kfree(normal_tbl->aor);
		normal_tbl->aor = NULL;
	}

	if (!IS_ERR_OR_NULL(normal_tbl->vint)) {
		for (column = 0; column < normal_brightness_step; column++)
			kfree(normal_tbl->vint[column]);
		kfree(normal_tbl->vint);
		normal_tbl->vint = NULL;
	}

	if (!IS_ERR_OR_NULL(normal_tbl->elvss)) {
		for (column = 0; column < normal_brightness_step; column++)
			kfree(normal_tbl->elvss[column]);
		kfree(normal_tbl->elvss);
		normal_tbl->elvss = NULL;
	}

	if (!IS_ERR_OR_NULL(normal_tbl->irc)) {
		for (column = 0; column < normal_brightness_step; column++)
			kfree(normal_tbl->irc[column]);
		kfree(normal_tbl->irc);
		normal_tbl->irc = NULL;
	}

	/* alloc 2 dimension matrix */
	if (!normal_tbl->candela_table)
		normal_tbl->candela_table = kzalloc(normal_brightness_step * sizeof(int), GFP_KERNEL);

	if (test_bit(BR_NORMAL_GAMMA, vdd->br_info.panel_br_info.br_info_select)) {
		if (!normal_tbl->gamma) {
			normal_tbl->gamma = kzalloc(normal_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < normal_brightness_step; column++)
				normal_tbl->gamma[column] = kzalloc(gamma_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_NORMAL_AOR, vdd->br_info.panel_br_info.br_info_select)) {
		if (!normal_tbl->aor) {
			normal_tbl->aor = kzalloc(normal_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < normal_brightness_step; column++)
				normal_tbl->aor[column] = kzalloc(aor_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_NORMAL_DBV, vdd->br_info.panel_br_info.br_info_select)) {
		if (!normal_tbl->dbv) {
			normal_tbl->dbv = kzalloc(normal_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < normal_brightness_step; column++)
				normal_tbl->dbv[column] = kzalloc(dbv_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_NORMAL_VINT, vdd->br_info.panel_br_info.br_info_select)) {
		if (!normal_tbl->vint) {
			normal_tbl->vint = kzalloc(normal_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < normal_brightness_step; column++)
				normal_tbl->vint[column] = kzalloc(vint_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_NORMAL_ELVSS, vdd->br_info.panel_br_info.br_info_select)) {
		if (!normal_tbl->elvss) {
			normal_tbl->elvss = kzalloc(normal_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < normal_brightness_step; column++)
				normal_tbl->elvss[column] = kzalloc(elvss_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_NORMAL_IRC, vdd->br_info.panel_br_info.br_info_select)) {
		if (!normal_tbl->irc) {
			normal_tbl->irc = kzalloc(normal_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < normal_brightness_step; column++)
				normal_tbl->irc[column] = kzalloc(irc_size, GFP_KERNEL);
		}
	}
}

static void init_br_info_hmd(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int column;

	int hmd_brightness_step = vdd->br_info.hmd_brightness_step;
	int gamma_size = vdd->br_info.gamma_size;
	int aor_size = vdd->br_info.aor_size;

	struct dimming_tbl *hmd_tbl = &br_tbl->hmd_tbl;

	/* free alloc memory : test purpose for slab memory integrity */
	if (!IS_ERR_OR_NULL(hmd_tbl->candela_table)) {
		kfree(hmd_tbl->candela_table);
		hmd_tbl->candela_table = NULL;
	}

	if (!IS_ERR_OR_NULL(hmd_tbl->gamma)) {
		for (column = 0; column < hmd_brightness_step; column++)
			kfree(hmd_tbl->gamma[column]);
		kfree(hmd_tbl->gamma);
		hmd_tbl->gamma = NULL;
	}

	if (!IS_ERR_OR_NULL(hmd_tbl->aor)) {
		for (column = 0; column < hmd_brightness_step; column++)
			kfree(hmd_tbl->aor[column]);
		kfree(hmd_tbl->aor);
		hmd_tbl->aor = NULL;
	}

	/*alloc 2 dimension matrix */
	if (!hmd_tbl->candela_table)
		hmd_tbl->candela_table = kzalloc(hmd_brightness_step * sizeof(int), GFP_KERNEL);

	if (test_bit(BR_HMD_GAMMA, vdd->br_info.panel_br_info.br_info_select)) {
		if (!hmd_tbl->gamma) {
			hmd_tbl->gamma = kzalloc(hmd_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < hmd_brightness_step; column++)
				hmd_tbl->gamma[column] = kzalloc(gamma_size, GFP_KERNEL);
		}
	}

	if (test_bit(BR_HMD_AOR, vdd->br_info.panel_br_info.br_info_select)) {
		if (!hmd_tbl->aor) {
			hmd_tbl->aor = kzalloc(hmd_brightness_step * sizeof(void *), GFP_KERNEL);
			for (column = 0; column < hmd_brightness_step; column++)
				hmd_tbl->aor[column] = kzalloc(aor_size, GFP_KERNEL);
		}
	}
}

static void update_br_info_hbm(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int column, data_cnt;

	struct dimming_tbl *hbm_tbl = &br_tbl->hbm_tbl;
	struct flash_raw_table *gamma_tbl = br_tbl->gamma_tbl;
	unsigned char *flash_data = gamma_tbl->br_data_raw;
	unsigned char **gamma = hbm_tbl->gamma;
	unsigned char **aor = hbm_tbl->aor;
	unsigned char **vint = 	hbm_tbl->vint;
	unsigned char **elvss = hbm_tbl->elvss;
	unsigned char **irc = hbm_tbl->irc;

	int gamma_index = gamma_tbl->flash_table_hbm_gamma_offset;
	int aor_index = gamma_tbl->flash_table_hbm_aor_offset;
	int vint_index =  gamma_tbl->flash_table_hbm_vint_offset;
	int elvss_index =  gamma_tbl->flash_table_hbm_elvss_offset;
	int irc_inedx =  gamma_tbl->flash_table_hbm_irc_offset;

	int hbm_brightness_step = vdd->br_info.hbm_brightness_step;
	int gamma_size = vdd->br_info.gamma_size;
	int aor_size = vdd->br_info.aor_size;
	int vint_size = vdd->br_info.vint_size;
	int elvss_size = vdd->br_info.elvss_size;
	int irc_size = vdd->br_info.irc_size;

	int rc;
	struct device_node *np;
	np = ss_get_panel_of(vdd);

	for (column = 0; column < hbm_brightness_step; column++) {
		if (test_bit(BR_HBM_GAMMA, vdd->br_info.panel_br_info.br_info_select)) {
			/* hubble panel has only one 800nit  max HBM gamma in DDI flash memory,
			 * and it will save normal brightness gamma after column=1~.
			 * Even though, hbm gamma interpolation take only max HBM gamma
			 * and do HBM gamma interpolation.
			 * (refer to gen_hbm_interpolation_gamma_S6E3HAB_AMB677TY01().)
			 */
			for (data_cnt = 0; data_cnt < gamma_size; data_cnt++)
				gamma[column][data_cnt] = flash_data[gamma_index++];
		}

		if (test_bit(BR_HBM_AOR, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < aor_size; data_cnt++)
				aor[column][data_cnt] = flash_data[aor_index++];
		}

		if (test_bit(BR_HBM_VINT, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < vint_size; data_cnt++)
				vint[column][data_cnt] = flash_data[vint_index++];
		}

		if (test_bit(BR_HBM_ELVSS, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < elvss_size; data_cnt++)
				elvss[column][data_cnt] = flash_data[elvss_index++];
		}

		if (test_bit(BR_HBM_IRC, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < irc_size; data_cnt++)
				irc[column][data_cnt] = flash_data[irc_inedx++];
		}
	}

	rc = of_property_read_u32_array(np,
				"samsung,hbm_brightness",
				hbm_tbl->candela_table,
				hbm_brightness_step);
	if (rc)
		LCD_INFO("fail to get samsung,hbm_brightness\n");
}

static void update_br_info_normal(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int column, data_cnt;

	struct dimming_tbl *normal_tbl = &br_tbl->normal_tbl;
	struct flash_raw_table *gamma_tbl = br_tbl->gamma_tbl;
	unsigned char *flash_data = gamma_tbl->br_data_raw;
	unsigned char **gamma = normal_tbl->gamma;
	unsigned char **aor = normal_tbl->aor;
	unsigned char **vint = 	normal_tbl->vint;
	unsigned char **elvss = normal_tbl->elvss;
	unsigned char **irc = normal_tbl->irc;

	int gamma_index = gamma_tbl->flash_table_normal_gamma_offset;
	int aor_index = gamma_tbl->flash_table_normal_aor_offset;
	int vint_index = gamma_tbl->flash_table_normal_vint_offset;
	int elvss_index = gamma_tbl->flash_table_normal_elvss_offset;
	int irc_inedx = gamma_tbl->flash_table_normal_irc_offset;

	int normal_brightness_step = vdd->br_info.normal_brightness_step;
	int gamma_size = vdd->br_info.gamma_size;
	int aor_size = vdd->br_info.aor_size;
	int vint_size = vdd->br_info.vint_size;
	int elvss_size = vdd->br_info.elvss_size;
	int irc_size = vdd->br_info.irc_size;

	int rc;
	struct device_node *np;
	np = ss_get_panel_of(vdd);

	for (column = 0; column < normal_brightness_step; column++) {
		if (test_bit(BR_NORMAL_GAMMA, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < gamma_size; data_cnt++)
				gamma[column][data_cnt] = flash_data[gamma_index++];
		}

		if (test_bit(BR_NORMAL_AOR, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < aor_size; data_cnt++)
				aor[column][data_cnt] = flash_data[aor_index++];
		}

		if (test_bit(BR_NORMAL_VINT, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < vint_size; data_cnt++)
				vint[column][data_cnt] = flash_data[vint_index++];
		}

		if (test_bit(BR_NORMAL_ELVSS, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < elvss_size; data_cnt++)
				elvss[column][data_cnt] = flash_data[elvss_index++];
		}

		if (test_bit(BR_NORMAL_IRC, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < irc_size; data_cnt++)
				irc[column][data_cnt] = flash_data[irc_inedx++];
		}
	}

	rc = of_property_read_u32_array(np,
				"samsung,normal_brightness",
				normal_tbl->candela_table,
				normal_brightness_step);
	if (rc)
		LCD_INFO("fail to get samsung,normal_brightness\n");
}

static void update_br_info_hmd(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int column, data_cnt;

	struct dimming_tbl *hmd_tbl = &br_tbl->hmd_tbl;
	struct flash_raw_table *gamma_tbl = br_tbl->gamma_tbl;
	unsigned char *flash_data = gamma_tbl->br_data_raw;
	unsigned char **gamma = hmd_tbl->gamma;
	unsigned char **aor = hmd_tbl->aor;

	int gamma_index = gamma_tbl->flash_table_hmd_gamma_offset;
	int aor_index = gamma_tbl->flash_table_hmd_aor_offset;

	int hmd_brightness_step = vdd->br_info.hmd_brightness_step;
	int gamma_size = vdd->br_info.gamma_size;
	int aor_size = vdd->br_info.aor_size;

	int rc;
	struct device_node *np;
	np = ss_get_panel_of(vdd);

	for (column = 0; column < hmd_brightness_step; column++) {
		if (test_bit(BR_HMD_GAMMA, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < gamma_size; data_cnt++)
				gamma[column][data_cnt] = flash_data[gamma_index++];
		}

		if (test_bit(BR_HMD_AOR, vdd->br_info.panel_br_info.br_info_select)) {
			for (data_cnt = 0; data_cnt < aor_size; data_cnt++)
				aor[column][data_cnt] = flash_data[aor_index++];
		}
	}

	rc = of_property_read_u32_array(np,
				"samsung,hmd_brightness",
				hmd_tbl->candela_table,
				hmd_brightness_step);
	if (rc)
		LCD_INFO("fail to get samsung,hmd_brightness\n");
}

void set_up_br_info(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	init_br_info_hbm(vdd, br_tbl);
	init_br_info_normal(vdd, br_tbl);
	init_br_info_hmd(vdd, br_tbl);

	update_br_info_hbm(vdd, br_tbl);
	update_br_info_normal(vdd, br_tbl);
	update_br_info_hmd(vdd, br_tbl);
}

#define read_buf_size 64
char flash_read_one_byte(struct samsung_display_driver_data *vdd, int addr)
{
	struct dsi_panel_cmd_set *flash_gamma_cmds = ss_get_cmds(vdd, TX_FLASH_GAMMA);
	struct dsi_panel_cmd_set *rx_gamma_cmds = ss_get_cmds(vdd, RX_FLASH_GAMMA);

	if (SS_IS_CMDS_NULL(flash_gamma_cmds) || SS_IS_CMDS_NULL(rx_gamma_cmds)) {
		LCD_ERR("No cmds for TX_FLASH_GAMMA or RX_FLASH_GAMMA..\n");
		return 0;
	}

	/*	TX - flash control / Execute Inst.
		3byte address cover 15Mbyte
	*/
	flash_gamma_cmds->cmds[0].msg.tx_buf[vdd->br_info.br_tbl->gamma_tbl->flash_gamma_data_read_addr[0]]
									= (char)((addr & 0xFF0000) >> 16);
	flash_gamma_cmds->cmds[0].msg.tx_buf[vdd->br_info.br_tbl->gamma_tbl->flash_gamma_data_read_addr[1]]
									= (char)((addr & 0xFF00) >> 8);
	flash_gamma_cmds->cmds[0].msg.tx_buf[vdd->br_info.br_tbl->gamma_tbl->flash_gamma_data_read_addr[2]]
									= (char)(addr & 0xFF);

	ss_send_cmd(vdd, TX_FLASH_GAMMA);

	usleep_range(250, 250);

	/* RX - Flash read
	 * do not send level key.. */
	rx_gamma_cmds->state = DSI_CMD_SET_STATE_HS;

	if (vdd->gpara || (vdd->ddi_spi_cs_high_gpio_for_gpara > 0)) {
		char gamma_buf[read_buf_size];
		rx_gamma_cmds->cmds[0].msg.rx_buf = gamma_buf;

		memset(gamma_buf, 0x00, sizeof(gamma_buf));
		ss_send_cmd(vdd, RX_FLASH_GAMMA);

		return gamma_buf[0];
	} else {
		char read_buf[1];
		ss_panel_data_read(vdd, RX_FLASH_GAMMA, &read_buf[0], LEVEL_KEY_NONE);

		return read_buf[0];
	}
}

void flash_write_check_read(struct samsung_display_driver_data *vdd)
{
	int addr, loop_cnt, start_addr, end_addr;
	char read_buf = 0;

	/* init write_check data */
	vdd->br_info.br_tbl->gamma_tbl->write_check = FLASH_GAMMA_BURN_EMPTY;

	start_addr = vdd->br_info.br_tbl->gamma_tbl->flash_gamma_write_check_address;
	end_addr = vdd->br_info.br_tbl->gamma_tbl->flash_gamma_write_check_address;

	for (loop_cnt = 0, addr = start_addr; addr <= end_addr ; addr++) {
		read_buf = flash_read_one_byte(vdd, addr);
		vdd->br_info.br_tbl->gamma_tbl->write_check =
			vdd->br_info.br_tbl->gamma_tbl->write_check << (loop_cnt++ * 8) | read_buf;
	}
}

static void flash_data_read(struct samsung_display_driver_data *vdd)
{
	int addr, loop_cnt, start_addr, end_addr;
	char read_buf = 0;

	/* init check sum cal data */
	vdd->br_info.br_tbl->gamma_tbl->check_sum_cal_data = MMC_CHECK_SUM_INIT;

	start_addr = vdd->br_info.br_tbl->gamma_tbl->flash_gamma_bank_start;
	end_addr = vdd->br_info.br_tbl->gamma_tbl->flash_gamma_bank_end;

	for (loop_cnt = 0, addr = start_addr; addr <= end_addr ; addr++) {
		read_buf = flash_read_one_byte(vdd, addr);

		vdd->br_info.br_tbl->gamma_tbl->br_data_raw[loop_cnt++] = read_buf;
		vdd->br_info.br_tbl->gamma_tbl->check_sum_cal_data += read_buf;
	}

	/* 16bit sum check */
	vdd->br_info.br_tbl->gamma_tbl->check_sum_cal_data &= ERASED_MMC_16BIT;
}

static int flash_gamma_read_spi(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	int i;
	int loop_cnt, bank_addr, start_addr, end_addr, total_size;
	struct spi_device *spi_dev;
	struct ddi_spi_cmd_set *cmd_set = NULL;
	char *rbuf = NULL;
	int ret = 0;
	struct flash_raw_table *gamma_tbl;

	spi_dev = vdd->spi_dev;

	if (IS_ERR_OR_NULL(spi_dev)) {
		LCD_ERR("no spi_dev\n");
		return -EINVAL;
	}

	cmd_set = ss_get_spi_cmds(vdd, RX_DATA);
	if (cmd_set == NULL) {
		LCD_ERR("cmd_set is null..\n");
		return -EINVAL;
	}

	gamma_tbl = br_tbl->gamma_tbl;

	/* Flash Write Check read*/
	start_addr = gamma_tbl->flash_gamma_write_check_address;
	end_addr = gamma_tbl->flash_gamma_write_check_address;
	total_size = end_addr - start_addr + 1;

	LCD_ERR("[Write Check] size = %d addr = %06x ~ %06x\n", total_size, start_addr, end_addr);

	rbuf = kmalloc(total_size, GFP_KERNEL | GFP_DMA);
	if (!rbuf) {
		LCD_ERR("fail to kmalloc for rbuf..\n");
		goto err;
	}

	cmd_set->rx_size = total_size;
	cmd_set->rx_addr = start_addr;

	ret = ss_spi_sync(spi_dev, rbuf, RX_DATA);

	for (i = 0; i < total_size; i++) {
		gamma_tbl->write_check =
			gamma_tbl->write_check << (i * 8) | rbuf[i];
	}

	/* Checksum read */
	if (rbuf)
		kfree(rbuf);

	/* init check sum data */
	gamma_tbl->check_sum_flash_data = MMC_CHECK_SUM_INIT;

	bank_addr = gamma_tbl->flash_gamma_bank_start;
	start_addr =  bank_addr + gamma_tbl->flash_gamma_check_sum_start_offset;
	end_addr = bank_addr + gamma_tbl->flash_gamma_check_sum_end_offset;
	total_size = end_addr - start_addr + 1;

	LCD_ERR("[Checksum] size = %d addr = %06x ~ %06x\n", total_size, start_addr, end_addr);

	rbuf = kmalloc(total_size, GFP_KERNEL | GFP_DMA);
	if (!rbuf) {
		LCD_ERR("fail to kmalloc for rbuf..\n");

		goto err;
	}

	cmd_set->rx_size = total_size;
	cmd_set->rx_addr = start_addr;

	ret = ss_spi_sync(spi_dev, rbuf, RX_DATA);

	for (i = 0; i < total_size; i++) {
		gamma_tbl->check_sum_flash_data =
			gamma_tbl->check_sum_flash_data << (i * 8) | rbuf[i];
	}

	/* GAMMA read */

	if (rbuf)
		kfree(rbuf);

	/* init check sum cal data */
	gamma_tbl->check_sum_cal_data = MMC_CHECK_SUM_INIT;

	start_addr = gamma_tbl->flash_gamma_bank_start;
	end_addr = gamma_tbl->flash_gamma_bank_end;
	total_size = end_addr - start_addr + 1;

	LCD_ERR("[Gamma] size = %d addr = %06x ~ %06x\n", total_size, start_addr, end_addr);

	rbuf = kmalloc(total_size, GFP_KERNEL | GFP_DMA);
	if (!rbuf) {
		LCD_ERR("fail to kmalloc for rbuf..\n");
		goto err;
	}

	cmd_set->rx_size = total_size;
	cmd_set->rx_addr = start_addr;

	ret = ss_spi_sync(spi_dev, rbuf, RX_DATA);

	memcpy(gamma_tbl->br_data_raw, rbuf, total_size);

	for (loop_cnt = 0; loop_cnt < total_size; loop_cnt++)
		gamma_tbl->check_sum_cal_data += gamma_tbl->br_data_raw[loop_cnt];

	/* 16bit sum check */
	gamma_tbl->check_sum_cal_data &= ERASED_MMC_16BIT;

	/* 0xC8 read */

	if (rbuf)
		kfree(rbuf);

	/* init check sum data */
	gamma_tbl->c8_register.check_sum_mtp_data = MMC_CHECK_SUM_INIT;
	gamma_tbl->c8_register.check_sum_flash_data = MMC_CHECK_SUM_INIT;
	gamma_tbl->c8_register.check_sum_cal_data = MMC_CHECK_SUM_INIT;

	bank_addr = gamma_tbl->flash_gamma_bank_start;
	start_addr = bank_addr + gamma_tbl->flash_gamma_0xc8_check_sum_start_offset;
	end_addr = bank_addr + gamma_tbl->flash_gamma_0xc8_check_sum_end_offset;
	total_size = end_addr - start_addr + 1;

	LCD_ERR("[0xC8 1] size = %d addr = %06x ~ %06x\n", total_size, start_addr, end_addr);

	rbuf = kmalloc(total_size, GFP_KERNEL | GFP_DMA);
	if (!rbuf) {
		LCD_ERR("fail to kmalloc for rbuf..\n");
		goto err;
	}

	cmd_set->rx_size = total_size;
	cmd_set->rx_addr = start_addr;

	ret = ss_spi_sync(spi_dev, rbuf, RX_DATA);

	for (i = 0; i < total_size; i++) {
		gamma_tbl->c8_register.check_sum_flash_data =
			gamma_tbl->c8_register.check_sum_flash_data << (i * 8) | rbuf[i];
	}

	if (rbuf)
		kfree(rbuf);

	start_addr = gamma_tbl->flash_gamma_0xc8_start_offset + bank_addr;
	end_addr = gamma_tbl->flash_gamma_0xc8_end_offset + bank_addr;
	total_size = end_addr - start_addr + 1;

	LCD_ERR("[0xC8 2] size = %d addr = %06x ~ %06x\n", total_size, start_addr, end_addr);

	rbuf = kmalloc(total_size, GFP_KERNEL | GFP_DMA);
	if (!rbuf) {
		LCD_ERR("fail to kmalloc for rbuf..\n");
		goto err;
	}

	cmd_set->rx_size = total_size;
	cmd_set->rx_addr = start_addr;

	ret = ss_spi_sync(spi_dev, rbuf, RX_DATA);

	for (i = 0; i < total_size; i++) {
		gamma_tbl->c8_register.flash_data[i] = rbuf[i];
		gamma_tbl->c8_register.check_sum_cal_data += rbuf[i];
	}

	/* check ddi 0xc8 register value */
	for (i = 0; i <= gamma_tbl->flash_gamma_0xc8_size; i++)
		gamma_tbl->c8_register.check_sum_mtp_data += gamma_tbl->c8_register.mtp_data[i];

	/* 16bit sum check */
	gamma_tbl->c8_register.check_sum_mtp_data &= ERASED_MMC_16BIT;
	gamma_tbl->c8_register.check_sum_cal_data &= ERASED_MMC_16BIT;

	LCD_INFO("read 0xC8 mtp_check_sum : 0x%x flash_check_sum : 0x%x cal_check_sum : 0x%x\n",
			gamma_tbl->c8_register.check_sum_mtp_data,
			gamma_tbl->c8_register.check_sum_flash_data,
			gamma_tbl->c8_register.check_sum_cal_data);
err:
	if (rbuf)
		kfree(rbuf);

	return ret;
}

void flash_checksum_read(struct samsung_display_driver_data *vdd)
{
	int addr, loop_cnt, bank_addr, start_addr, end_addr;
	char read_buf = 0;

	/* init check sum data */
	vdd->br_info.br_tbl->gamma_tbl->check_sum_flash_data = MMC_CHECK_SUM_INIT;

	bank_addr = vdd->br_info.br_tbl->gamma_tbl->flash_gamma_bank_start;
	start_addr =  bank_addr + vdd->br_info.br_tbl->gamma_tbl->flash_gamma_check_sum_start_offset;
	end_addr = bank_addr + vdd->br_info.br_tbl->gamma_tbl->flash_gamma_check_sum_end_offset;

	for (loop_cnt = 0, addr = start_addr; addr <= end_addr ; addr++) {
		read_buf = flash_read_one_byte(vdd, addr);

		vdd->br_info.br_tbl->gamma_tbl->check_sum_flash_data =
			vdd->br_info.br_tbl->gamma_tbl->check_sum_flash_data << (loop_cnt++ * 8) | read_buf;
	}
}

void flash_0xc8_read(struct samsung_display_driver_data *vdd)
{
	int addr, loop_cnt, bank_addr,start_addr, end_addr;
	char read_buf = 0;

	/* init check sum data */
	vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_mtp_data = MMC_CHECK_SUM_INIT;
	vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_flash_data = MMC_CHECK_SUM_INIT;
	vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_cal_data = MMC_CHECK_SUM_INIT;

	/* check sum value check */
	bank_addr = vdd->br_info.br_tbl->gamma_tbl->flash_gamma_bank_start;
	start_addr = bank_addr + vdd->br_info.br_tbl->gamma_tbl->flash_gamma_0xc8_check_sum_start_offset;
	end_addr = bank_addr + vdd->br_info.br_tbl->gamma_tbl->flash_gamma_0xc8_check_sum_end_offset;

	for (loop_cnt = 0, addr = start_addr; addr <= end_addr ; addr++) {
		read_buf = flash_read_one_byte(vdd, addr);

		vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_flash_data =
			vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_flash_data << (loop_cnt++ * 8) | read_buf;

	}

	/* read real flash gamma data */
	start_addr = vdd->br_info.br_tbl->gamma_tbl->flash_gamma_0xc8_start_offset + bank_addr;
	end_addr = vdd->br_info.br_tbl->gamma_tbl->flash_gamma_0xc8_end_offset + bank_addr;
	for (loop_cnt = 0, addr = start_addr; addr <= end_addr ; addr++) {
		read_buf = flash_read_one_byte(vdd, addr);

		vdd->br_info.br_tbl->gamma_tbl->c8_register.flash_data[loop_cnt++] = read_buf;
		vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_cal_data += read_buf;
	}

	/* check ddi 0xc8 register value */
	for (loop_cnt = 0; loop_cnt <= vdd->br_info.br_tbl->gamma_tbl->flash_gamma_0xc8_size; loop_cnt++)
		vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_mtp_data += vdd->br_info.br_tbl->gamma_tbl->c8_register.mtp_data[loop_cnt];

	/* 16bit sum check */
	vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_mtp_data &= ERASED_MMC_16BIT;
	vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_cal_data &= ERASED_MMC_16BIT;


	LCD_INFO("read 0xC8 mtp_check_sum : 0x%x flash_check_sum : 0x%x cal_check_sum : 0x%x\n",
			vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_mtp_data,
			vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_flash_data,
			vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_cal_data);
}

static void flash_gamma_read_mipi(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	if (vdd->poc_driver.check_read_case) {
		if (vdd->poc_driver.read_case == READ_CASE1) {
			ss_send_cmd(vdd, TX_FLASH_GAMMA_PRE1);
			LCD_ERR("READ_CASE1 \n");
		}
		else if (vdd->poc_driver.read_case == READ_CASE2) {
			ss_send_cmd(vdd, TX_FLASH_GAMMA_PRE2);
			vdd->poc_driver.need_sleep_in = true;
			LCD_ERR("READ_CASE2 \n");
		}
	} else {
		ss_send_cmd(vdd, TX_FLASH_GAMMA_PRE1);
		LCD_ERR("No check read_case.. READ_CASE1 \n");
	}

	/* 1st */
	flash_write_check_read(vdd);

	/* 2st */
	flash_checksum_read(vdd);

	/* 3 st */
	//init_br_info(vdd, br_tbl);

	/*4 st */
	flash_data_read(vdd);

	/* 5 st extra 0xC8 bank */
	flash_0xc8_read(vdd);

	/* 6 st flash_data.mcd flash value */
	//flash_flash_data.mcd_read(vdd);

	return;
}

void flash_mcd_read(struct samsung_display_driver_data *vdd)
{
	char read_buf = 0;

	read_buf = flash_read_one_byte(vdd, vdd->br_info.br_tbl->gamma_tbl->flash_MCD1_R_address);
	vdd->br_info.br_tbl->gamma_tbl->mcd.flash_MCD1_R = read_buf;

	read_buf = flash_read_one_byte(vdd, vdd->br_info.br_tbl->gamma_tbl->flash_MCD2_R_address);
	vdd->br_info.br_tbl->gamma_tbl->mcd.flash_MCD2_R = read_buf;

	read_buf = flash_read_one_byte(vdd, vdd->br_info.br_tbl->gamma_tbl->flash_MCD1_L_address);
	vdd->br_info.br_tbl->gamma_tbl->mcd.flash_MCD1_L = read_buf;

	read_buf = flash_read_one_byte(vdd, vdd->br_info.br_tbl->gamma_tbl->flash_MCD2_L_address);
	vdd->br_info.br_tbl->gamma_tbl->mcd.flash_MCD2_L = read_buf;
}

static void br_basic_register_read(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	/* Read mtp for hbm max gamma */
	ss_panel_data_read(vdd, RX_HBM, br_tbl->hbm_max_gamma, LEVEL1_KEY);

	/* overwrite some values from default (60 normal) read values */
	if (br_tbl->refresh_rate == 120 || br_tbl->refresh_rate == 96) { /* 120/96hz */
		/* hbm gamma[0~30]: C9h 75th~105th, hbm gamma[31~33] B3h 37th~39th */
		ss_panel_data_read(vdd, RX_HBM2, br_tbl->hbm_max_gamma, LEVEL1_KEY);

		/* Read mtp for normal max gamma */
		ss_panel_data_read(vdd, RX_CENTER_GAMMA_120HS, br_tbl->normal_max_gamma, LEVEL1_KEY);	/* C9h 38th ~ 70th */
	} else if (br_tbl->refresh_rate == 60 && br_tbl->is_sot_hs_mode) { /* 60hz HS */
		/* hbm gamma[0~30]: C9h 112th~142th, hbm gamma[31~33] B3h 37th~39th */
		ss_panel_data_read(vdd, RX_HBM3, br_tbl->hbm_max_gamma, LEVEL1_KEY);

		/* Read mtp for normal max gamma */
		ss_panel_data_read(vdd, RX_CENTER_GAMMA_60HS, br_tbl->normal_max_gamma, LEVEL1_KEY);	/* C8h 112th ~ 144th */
	}

	/* Read 0xC8 */
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, br_tbl->gamma_tbl->c8_register.mtp_data, LEVEL1_KEY);
}

int flash_gamma_support_check(struct samsung_display_driver_data *vdd)
{
	/* check flash gamma support from panel dtsi property */
	if (!vdd->br_info.flash_gamma_support) {
		LCD_ERR("flash_gamma_support not support1");
		return false;
	}

	/* check force update mode */
	if (vdd->br_info.flash_gamma_force_update == true) {
		vdd->br_info.flash_gamma_force_update = false;
		return true;
	}

	/* check flash gamma support for specific contidion
	 * ex) specific panel rev does not support flash gamma support..
	 */
	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_flash_gamma_support)) {
		if (!vdd->panel_func.samsung_flash_gamma_support(vdd)) {
			LCD_ERR("flash_gamma_support not support2\n");
			return false;
		}
	}

	return true;
}

int flash_checksum_check(struct samsung_display_driver_data *vdd)
{
	if (vdd->br_info.br_tbl->gamma_tbl->write_check != FLASH_GAMMA_BURN_WRITE) {
		LCD_ERR("FLASH_GAMMA_BURN_EMPTY\n");
		return false;
	} else if (vdd->br_info.br_tbl->gamma_tbl->check_sum_cal_data != vdd->br_info.br_tbl->gamma_tbl->check_sum_flash_data) {
		LCD_ERR("CHECK_SUM_FALSH_ERROR\n");
		return false;
	} else if (vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_flash_data != vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_cal_data) {
		LCD_ERR("CHECK_SUM_0xC8_ERROR_1\n");
		return false;
	} else if (vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_flash_data != vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_mtp_data) {
		LCD_ERR("CHECK_SUM_0xC8_ERROR_2\n");
		return false;
	} else if (vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_cal_data != vdd->br_info.br_tbl->gamma_tbl->c8_register.check_sum_mtp_data) {
		LCD_ERR("CHECK_SUM_0xC8_ERROR_3\n");
		return false;
	} else
		return true;
}

static int ss_copy_flash_gamma(struct flash_raw_table *dest, struct flash_raw_table *src)
{
	unsigned char *br_data_raw_org = dest->br_data_raw;
	int tot_size;

	*dest = *src;

	/* br_data_raw will be allocated by init_br_info() funciton. */
	dest->br_data_raw = br_data_raw_org;

	if (!dest->br_data_raw || !src->br_data_raw) {
		LCD_ERR("fail to copy flash gamma: mem is null\n");
		return -ENOMEM;
	}

	tot_size = src->br_data_size * sizeof(char);
	LCD_INFO("copy flash gamma: size: %d\n", tot_size);
	memcpy(dest->br_data_raw, src->br_data_raw, tot_size);

	return 0;
}

void __table_br(struct samsung_display_driver_data *vdd)
{
	int count;

	LCD_INFO("+++\n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		goto end;
	}

	if (!vdd->br_info.br_tbl_count) {
		LCD_ERR("br_tbl_count is 0\n");
		goto end;
	}

	for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
		struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];
		table_br_func(vdd, br_tbl);
	}

	for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
		struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];
		vdd->panel_func.samsung_interpolation_init(vdd, br_tbl, TABLE_INTERPOLATION);
	}

	/*
	 * Update init_done
	 * If checksum value of any mode is not OK, use table interpolation for all modes.
	 */
	vdd->br_info.flash_gamma_init_done = true;
	for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
		struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];

		/* If this work is from sysfs, set init_done as true even if checksum is fail. */
		if (!br_tbl->gamma_tbl->checksum &&
			!vdd->br_info.flash_gamma_sysfs)
			vdd->br_info.flash_gamma_init_done = false;
	}

	/* update brighntess */
	ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
end:

	if (vdd->br_info.flash_gamma_sysfs)
		vdd->br_info.flash_gamma_sysfs = false;

	LCD_INFO("--- init_done %d\n", vdd->br_info.flash_gamma_init_done);
}

void __flash_br(struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = NULL;
	struct msm_drm_private *priv = NULL;
	u64 cur_mnoc_ab, cur_mnoc_ib;
	int rc = 0;
	int count;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	LCD_INFO("+++\n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		goto end;
	}

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_ERR("no display");
		goto end;
	}

	if (!flash_gamma_support_check(vdd)) {
		LCD_ERR("flash_gamma_support not support2");
		goto end;
	}

	if (vdd->poc_driver.check_read_case)
		vdd->poc_driver.read_case = vdd->poc_driver.check_read_case(vdd);

	for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
		struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];
		table_br_func(vdd, br_tbl);
	}

	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.permit_frame_update = 1;
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	if (test_bit(BOOST_CPU, vdd->br_info.panel_br_info.flash_br_boosting)) {
		/* max cpu freq */
		LCD_INFO("CPU\n");
		ss_set_max_cpufreq(vdd, true, CPUFREQ_CLUSTER_ALL);
	}

	priv = display->drm_dev->dev_private;
	cur_mnoc_ab = priv->phandle.data_bus_handle[SDE_POWER_HANDLE_DBUS_ID_MNOC].in_ab_quota;
	cur_mnoc_ib = priv->phandle.data_bus_handle[SDE_POWER_HANDLE_DBUS_ID_MNOC].in_ib_quota;

	if (test_bit(BOOST_MNOC, vdd->br_info.panel_br_info.flash_br_boosting)) {
		LCD_INFO("MNOC\n");
		/* update only mnoc bw*/
		sde_power_data_bus_set_quota(&priv->phandle,
			SDE_POWER_HANDLE_DBUS_ID_MNOC,
			SDE_POWER_HANDLE_CONT_SPLASH_BUS_AB_QUOTA,
			SDE_POWER_HANDLE_CONT_SPLASH_BUS_IB_QUOTA);
	}

	if (test_bit(BOOST_DSI_CLK, vdd->br_info.panel_br_info.flash_br_boosting)) {
		/* enable dsi clock*/
		LCD_INFO("CLK\n");
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_ALL_CLKS, DSI_CLK_ON);
		if (rc) {
			LCD_ERR("[%s] failed to enable DSI core clocks, rc=%d\n",
					display->name, rc);
			goto error;
		}
	}

	ss_set_exclusive_tx_packet(vdd, TX_FLASH_GAMMA_PRE1, 1);
	ss_set_exclusive_tx_packet(vdd, TX_FLASH_GAMMA_PRE2, 1);
	ss_set_exclusive_tx_packet(vdd, TX_FLASH_GAMMA, 1);
	ss_set_exclusive_tx_packet(vdd, TX_FLASH_GAMMA_POST, 1);
	ss_set_exclusive_tx_packet(vdd, RX_FLASH_GAMMA, 1);
	ss_set_exclusive_tx_packet(vdd, TX_REG_READ_POS, 1);

	if (vdd->ddi_spi_cs_high_gpio_for_gpara > 0) {
		mutex_lock(&vdd->ss_spi_lock);
		gpio_direction_output(vdd->ddi_spi_cs_high_gpio_for_gpara , 1);
		LCD_INFO("gpio%d value : %d\n", vdd->ddi_spi_cs_high_gpio_for_gpara,
			gpio_get_value(vdd->ddi_spi_cs_high_gpio_for_gpara));
	}

	for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
		struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];

		if (br_tbl->parent_idx != -1) {
			LCD_INFO("[%d] copy flash gamma data from parent node [%d]\n",
					count, br_tbl->parent_idx);
			ss_copy_flash_gamma(vdd->br_info.br_tbl[count].gamma_tbl,
					vdd->br_info.br_tbl[br_tbl->parent_idx].gamma_tbl);
			goto skip_read_flash;
		}

		if (!strcmp(vdd->br_info.flash_read_intf, "spi")) {
			flash_gamma_read_spi(vdd, br_tbl);
		} else {
			/* default is 'mipi' */
			flash_gamma_read_mipi(vdd, br_tbl);
		}

		br_tbl->gamma_tbl->checksum = flash_checksum_check(vdd);

skip_read_flash:

		LCD_INFO("read write_check : 0x%x flash_check_sum : 0x%x cal_check_sum : 0x%x\n",
			br_tbl->gamma_tbl->write_check,
			br_tbl->gamma_tbl->check_sum_flash_data,
			br_tbl->gamma_tbl->check_sum_cal_data);

		if (br_tbl->gamma_tbl->checksum) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_interpolation_init)) {
				if (br_tbl->gamma_tbl->force_table_interpolatioin)
					vdd->panel_func.samsung_interpolation_init(vdd, br_tbl, TABLE_INTERPOLATION);
				else
					vdd->panel_func.samsung_interpolation_init(vdd, br_tbl, FLASH_INTERPOLATION);
			}
		} else {
			LCD_ERR("flash checksum fail.. stop reading flash data !\n");
			break;
		}
	}

	if (strcmp(vdd->br_info.flash_read_intf, "spi"))
		ss_send_cmd(vdd, TX_FLASH_GAMMA_POST);

	if (vdd->ddi_spi_cs_high_gpio_for_gpara > 0) {
		gpio_direction_input(vdd->ddi_spi_cs_high_gpio_for_gpara);
		mutex_unlock(&vdd->ss_spi_lock);
		LCD_INFO("gpio%d value : %d\n", vdd->ddi_spi_cs_high_gpio_for_gpara,
			gpio_get_value(vdd->ddi_spi_cs_high_gpio_for_gpara));
	}

	if (test_and_clear_bit(BOOST_DSI_CLK, vdd->br_info.panel_br_info.flash_br_boosting)) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_ALL_CLKS, DSI_CLK_OFF);
		if (rc) {
			LCD_ERR("[%s] failed to disable DSI core clocks, rc=%d\n",
					display->name, rc);
		}
	}

	ss_set_exclusive_tx_packet(vdd, TX_FLASH_GAMMA_PRE1, 0);
	ss_set_exclusive_tx_packet(vdd, TX_FLASH_GAMMA_PRE2, 0);
	ss_set_exclusive_tx_packet(vdd, TX_FLASH_GAMMA, 0);
	ss_set_exclusive_tx_packet(vdd, TX_FLASH_GAMMA_POST, 0);
	ss_set_exclusive_tx_packet(vdd, RX_FLASH_GAMMA, 0);
	ss_set_exclusive_tx_packet(vdd, TX_REG_READ_POS, 0);
error:
	if (test_and_clear_bit(BOOST_MNOC, vdd->br_info.panel_br_info.flash_br_boosting)) {
		/* restore bw */
		sde_power_data_bus_set_quota(&priv->phandle,
			SDE_POWER_HANDLE_DBUS_ID_MNOC,
			cur_mnoc_ab, cur_mnoc_ib);
	}

	if (test_and_clear_bit(BOOST_CPU, vdd->br_info.panel_br_info.flash_br_boosting)) {
		/* release max cpu freq */
		ss_set_max_cpufreq(vdd, false, CPUFREQ_CLUSTER_ALL);
	}

	vdd->exclusive_tx.permit_frame_update = 0;
	vdd->exclusive_tx.enable = 0;
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);

	/*
	 * Update init_done
	 * If checksum value of any mode is not OK, use table interpolation for all modes.
	 */
	vdd->br_info.flash_gamma_init_done = true;
	for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
		struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];

		/* If this work is from sysfs, set init_done as true even if checksum is fail. */
		if (!br_tbl->gamma_tbl->checksum &&
			!vdd->br_info.flash_gamma_sysfs)
			vdd->br_info.flash_gamma_init_done = false;
	}

	/* update brighntess */
	ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
end:

	if (vdd->br_info.flash_gamma_sysfs)
		vdd->br_info.flash_gamma_sysfs = false;

	LCD_INFO("--- init_done %d\n", vdd->br_info.flash_gamma_init_done);
}

void flash_br_work_func(struct work_struct *work)
{
	struct delayed_work * dwork = to_delayed_work(work);
	struct ss_brightness_info *br_info =
		container_of(dwork, struct ss_brightness_info, flash_br_work);
	struct samsung_display_driver_data *vdd =
		container_of(br_info, struct samsung_display_driver_data, br_info);
	struct dsi_display *display = NULL;
	bool skip_host_deinit = true;
	bool skip_restore_panel_state_off = true;
	int rc = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return;
	}

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_ERR("no display");
		return;
	}

	/* display ctrl and mipi host init should be done in display_lock */
	mutex_lock(&display->display_lock);

	LCD_INFO("[%s] init host, support_early_gamma_flash: %d\n",
			display->name, vdd->br_info.support_early_gamma_flash);
	rc = dsi_display_ctrl_init(display);
	if (rc) {
		LCD_INFO("[%s] rc=%d, host was already initialized.. skip deinit in final state\n",
				display->name, rc);
		skip_host_deinit = true;
		mutex_unlock(&display->display_lock);
	} else {
		skip_host_deinit = false;
	}

	/* set panel_state pwr on ready, to allow mipi transmission */
	if (vdd->panel_state == PANEL_PWR_OFF) {
		skip_restore_panel_state_off = false;
		vdd->panel_state = PANEL_PWR_ON_READY;
	}

	__flash_br(vdd);

	/* early gamma flash force to set panel_state to on_ready to allow mipi transmission.
	 * After finshing gamma flash, restore panel_state to poweroff
	 * to prevent other mipi transmission before gfx HAL enable display.
	 *
	 * disp_on_pre == true means panel_state is already overwritten by gfx HAL,
	 * so no more need to restore panel_state
	 */
	if (vdd->display_status_dsi.disp_on_pre)
		skip_restore_panel_state_off = true;
	if (!skip_restore_panel_state_off) {
		LCD_INFO("[%s] restore panel state to off\n", display->name);
		vdd->panel_state = PANEL_PWR_OFF;
	}

	if (!skip_host_deinit) {
		LCD_INFO("[%s] deinit host\n", display->name);
		(void)dsi_display_ctrl_deinit(display);
		mutex_unlock(&display->display_lock);
	}
}

void table_br_func(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	/* 1 st */
	init_br_info(vdd, br_tbl);

	/* 2 st: hbm & smart-dimming register read */
	br_basic_register_read(vdd, br_tbl);
}

static void debug_br_info_hbm(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	struct dimming_tbl *hbm_tbl = &br_tbl->hbm_tbl;
	char buf[FLASH_GAMMA_DBG_BUF_SIZE];
	int column, data_cnt;
	unsigned char **gamma, **aor, **vint, **elvss, **irc;
	int brightness_step, gamm_size, aor_size, vint_size, elvss_size, irc_size;

	memset(buf, '\n', sizeof(buf));

	brightness_step = vdd->br_info.hbm_brightness_step;
	gamm_size = vdd->br_info.gamma_size;
	aor_size = vdd->br_info.aor_size;
	vint_size = vdd->br_info.vint_size;
	elvss_size = vdd->br_info.elvss_size;
	irc_size = vdd->br_info.irc_size;

	gamma = hbm_tbl->gamma;
	aor = hbm_tbl->aor;
	vint = hbm_tbl->vint;
	elvss = hbm_tbl->elvss;
	irc = hbm_tbl->irc;

	LCD_INFO("print raw data from flash memory.\n");
	LCD_INFO("AOR(%d) VINT(%d) ELVSS(%d) IRC(%d)\n", aor_size, vint_size, elvss_size, irc_size);

	for (column =  0; column < brightness_step; column++) {
		snprintf(buf, FLASH_GAMMA_DBG_BUF_SIZE, "HBM [%3d] ", hbm_tbl->candela_table[column]);

		/* gamma */
		if (!IS_ERR_OR_NULL(gamma)) {
			for (data_cnt = 0; data_cnt < gamm_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", gamma[column][data_cnt]);
		}

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* AOR */
		if (!IS_ERR_OR_NULL(aor)) {
			for (data_cnt = 0; data_cnt < aor_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", aor[column][data_cnt]);
		}

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* VINT */
		if (!IS_ERR_OR_NULL(vint)) {
			for (data_cnt = 0; data_cnt < vint_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", vint[column][data_cnt]);
		}

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* ELVSS */
		if (!IS_ERR_OR_NULL(elvss)) {
			for (data_cnt = 0; data_cnt < elvss_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", elvss[column][data_cnt]);
		}

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* IRC */
		if (!IS_ERR_OR_NULL(irc)) {
			for (data_cnt = 0; data_cnt < irc_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", irc[column][data_cnt]);
		}

		LCD_INFO("%s\n", buf);
		memset(buf, '\n', strlen(buf));
	}
}

static void debug_br_info_normal(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	struct dimming_tbl *normal_tbl = &br_tbl->normal_tbl;
	char buf[FLASH_GAMMA_DBG_BUF_SIZE];
	int column, data_cnt;
	unsigned char **gamma, **aor, **vint, **elvss, **irc;
	int brightness_step, gamm_size, aor_size, vint_size, elvss_size, irc_size;

	memset(buf, '\n', sizeof(buf));

	brightness_step = vdd->br_info.normal_brightness_step;
	gamm_size = vdd->br_info.gamma_size;
	aor_size = vdd->br_info.aor_size;
	vint_size = vdd->br_info.vint_size;
	elvss_size = vdd->br_info.elvss_size;
	irc_size = vdd->br_info.irc_size;

	gamma = normal_tbl->gamma;
	aor = normal_tbl->aor;
	vint = normal_tbl->vint;
	elvss = normal_tbl->elvss;
	irc = normal_tbl->irc;

	LCD_INFO("print raw data from flash memory.\n");
	LCD_INFO("GAMMA(%d) AOR(%d) VINT(%d) ELVSS(%d) IRC(%d)\n", gamm_size, aor_size, vint_size, elvss_size, irc_size);

	for (column =  0; column < brightness_step; column++) {
		snprintf(buf, FLASH_GAMMA_DBG_BUF_SIZE, "NORMAL [%3d] ", normal_tbl->candela_table[column]);

		/* gamma */
		if (!IS_ERR_OR_NULL(gamma)) {
			for (data_cnt = 0; data_cnt < gamm_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", gamma[column][data_cnt]);
		} else
			LCD_ERR("gamma is null..\n");

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* AOR */
		if (!IS_ERR_OR_NULL(aor)) {
			for (data_cnt = 0; data_cnt < aor_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", aor[column][data_cnt]);
		} else
			LCD_ERR("aor is null..\n");

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* VINT */
		if (!IS_ERR_OR_NULL(vint)) {
			for (data_cnt = 0; data_cnt < vint_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", vint[column][data_cnt]);
		} else
			LCD_ERR("vint is null..\n");

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* ELVSS */
		if (!IS_ERR_OR_NULL(elvss)) {
			for (data_cnt = 0; data_cnt < elvss_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", elvss[column][data_cnt]);
		} else
			LCD_ERR("elvss is null..\n");

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* IRC */
		if (!IS_ERR_OR_NULL(irc)) {
			for (data_cnt = 0; data_cnt < irc_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", irc[column][data_cnt]);
		} else
			LCD_ERR("irc is null..\n");

		LCD_INFO("%s\n", buf);
		memset(buf, '\n', strlen(buf));

	}
}

static void debug_br_info_hmd(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	struct dimming_tbl *hmd_tbl = &br_tbl->hmd_tbl;
	char buf[FLASH_GAMMA_DBG_BUF_SIZE];
	int column, data_cnt;
	unsigned char **gamma, **aor;
	int brightness_step, gamma_size, aor_size;

	memset(buf, '\n', sizeof(buf));

	brightness_step = vdd->br_info.hmd_brightness_step;
	gamma_size = vdd->br_info.gamma_size;
	aor_size = vdd->br_info.aor_size;

	gamma = hmd_tbl->gamma;
	aor = hmd_tbl->aor;

	LCD_INFO("print raw data from flash memory.\n");
	LCD_INFO("GAMMA(%d) AOR(%d) \n", gamma_size, aor_size);

	for (column =  0; column < brightness_step; column++) {
		snprintf(buf, FLASH_GAMMA_DBG_BUF_SIZE, "HMD [%3d] ", hmd_tbl->candela_table[column]);

		/* gamma */
		if (!IS_ERR_OR_NULL(gamma)) {
			for (data_cnt = 0; data_cnt < gamma_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", gamma[column][data_cnt]);
		}

		snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "| ");

		/* AOR */
		if (!IS_ERR_OR_NULL(aor)) {
			for (data_cnt = 0; data_cnt < aor_size; data_cnt++)
				snprintf(buf + strlen(buf), FLASH_GAMMA_DBG_BUF_SIZE - strlen(buf), "%02x ", aor[column][data_cnt]);
		}

		LCD_INFO("%s\n", buf);
		memset(buf, '\n', strlen(buf));
	}
}

void debug_br_info_log(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	LCD_INFO("%s ++ \n", __func__);
	debug_br_info_hbm(vdd, br_tbl);
	debug_br_info_normal(vdd, br_tbl);
	debug_br_info_hmd(vdd, br_tbl);
	LCD_INFO("%s -- \n", __func__);
}

