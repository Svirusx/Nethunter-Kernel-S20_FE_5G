/*
 * =================================================================
 *
 *
 *	Description:  samsung display panel file
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
#include "ss_dsi_panel_ANA38401_AMSA05RB06.h"
#include "ss_dsi_mdnie_ANA38401_AMSA05RB06.h"
#include "ss_dsi_interpolation_ANA38401_AMSA05RB06.h"

static char hbm_buffer1[33] = {0,};
char D8h_129th = 0;
char D2h_112th = 0;

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("++\n");
	ss_panel_attach_set(vdd, true);
	LCD_INFO("--\n");

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
		LCD_ERR("Invalid panel_rev(default rev : %c)\n",
				vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';

	LCD_INFO_ONCE("panel_revision = %c %d \n",
		vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision + 'A');
}

static int ss_manufacture_date_read(struct samsung_display_driver_data *vdd)
{
	unsigned char date[8] = {0,};
	int year, month, day;
	int hour, min;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (D8h 134 ~ 140th Para) for manufacture date */
	if (ss_get_cmds(vdd, RX_MANUFACTURE_DATE)->count) {
		ss_panel_data_read(vdd, RX_MANUFACTURE_DATE, date, LEVEL_KEY_NONE);

		year = date[0] & 0xf0;
		year >>= 4;
		year += 2011; // 0 = 2011 year
		month = date[0] & 0x0f;
		day = date[1] & 0x1f;
		hour = date[2]& 0x0f;
		min = date[3] & 0x1f;

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
	char ddi_id[5];
	int loop;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return false;
	}

	/* Read mtp (D6h 1~5th) for ddi id */
	if (ss_get_cmds(vdd, RX_DDI_ID)->count) {
		ss_panel_data_read(vdd, RX_DDI_ID, ddi_id, LEVEL_KEY_NONE);

		for(loop = 0; loop < 5; loop++)
			vdd->ddi_id_dsi[loop] = ddi_id[loop];

		LCD_INFO("DSI%d : %02x %02x %02x %02x %02x\n", vdd->ndx,
			vdd->ddi_id_dsi[0], vdd->ddi_id_dsi[1],
			vdd->ddi_id_dsi[2], vdd->ddi_id_dsi[3],
			vdd->ddi_id_dsi[4]);
	} else {
		LCD_ERR("DSI%d no ddi_id_rx_cmds cmds\n", vdd->ndx);
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

	/* Read Panel Unique Cell ID (0xD8 : 134th para ~ 140th para) */
	if (ss_get_cmds(vdd, RX_CELL_ID)->count) {
		memset(cell_id_buffer, 0x00, MAX_CELL_ID);

		ss_panel_data_read(vdd, RX_CELL_ID, cell_id_buffer, LEVEL_KEY_NONE);

		for(loop = 0; loop < MAX_CELL_ID; loop++)
			vdd->cell_id_dsi[loop] = cell_id_buffer[loop];

		LCD_INFO("DSI%d: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			vdd->ndx, vdd->cell_id_dsi[0],
			vdd->cell_id_dsi[1],	vdd->cell_id_dsi[2],
			vdd->cell_id_dsi[3],	vdd->cell_id_dsi[4],
			vdd->cell_id_dsi[5],	vdd->cell_id_dsi[6],
			vdd->cell_id_dsi[7],	vdd->cell_id_dsi[8],
			vdd->cell_id_dsi[9],	vdd->cell_id_dsi[10]);
	} else {
		LCD_ERR("DSI%d no cell_id_rx_cmds cmd\n", vdd->ndx);
		return false;
	}

	return true;
}

static int ss_hbm_read(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *hbm_rx_cmds = ss_get_cmds(vdd, RX_HBM);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_rx_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)hbm_rx_cmds);
		return false;
	}

	/* Read mtp (D4h 91~123th) for hbm gamma */
	ss_panel_data_read(vdd, RX_HBM, hbm_buffer1, LEVEL_KEY_NONE);

	/* Read mtp (D8h 129th) for HBM OFF -> On elvss*/
	ss_panel_data_read(vdd, RX_HBM2, &D8h_129th, LEVEL_KEY_NONE);

	/* Read mtp (D2h 112th) for HBM ON -> Off elvss*/
	ss_panel_data_read(vdd, RX_ELVSS, &D2h_112th, LEVEL_KEY_NONE);

	return true;
}

static struct dsi_panel_cmd_set *ss_hbm_gamma(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_gamma_cmds = ss_get_cmds(vdd, TX_HBM_GAMMA);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_gamma_cmds)) {
		LCD_ERR("Invalid data  vdd : 0x%zx cmd : 0x%zx", (size_t)vdd, (size_t)hbm_gamma_cmds);
		return NULL;
	}
	//if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi->generate_hbm_gamma)) {
	//	LCD_ERR("generate_hbm_gamma is NULL error");
	//	return NULL;
	//} else {
	//	vdd->smart_dimming_dsi->generate_hbm_gamma(
	//		vdd->smart_dimming_dsi,
	//		vdd->br.auto_level,
	//		&hbm_gamma_cmds->cmds[0].msg.tx_buf[1]);

	//	*level_key = LEVEL_KEY_NONE;
	//	return hbm_gamma_cmds;
	//}
	*level_key = LEVEL1_KEY;
	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_GAMMA, &hbm_gamma_cmds->cmds->msg.tx_buf[1]);
	return hbm_gamma_cmds;

}

static struct dsi_panel_cmd_set *ss_hbm_etc(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_etc_cmds = ss_get_cmds(vdd, TX_HBM_ETC);
	u8 aor_data[AOR_SIZE];
	int acl_opr;
	int acl_start;
	int acl_percent;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_etc_cmds)) {
		LCD_ERR("Invalid data  vdd : 0x%zx cmd : 0x%zx", (size_t)vdd, (size_t)hbm_etc_cmds);
		return NULL;
	}

	if (!vdd->br_info.gradual_acl_val) {	/* gallery app */
		acl_opr = 0x4;		/* 16 Frame Avg at ACL off */
		acl_start = 0x99;	/* Start setting: 60% start */
		acl_percent = 0x10;	/* ACL off */
	}
	else {				/* not gallery app */
		acl_opr = 0x5;		/* 32 Frame Avg at ACL on */
		acl_start = 0x99;	/* Start setting: 60% start */
		acl_percent = 0x11;	/* ACL 8% on */
	}

	/* aor */
	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_AOR, aor_data);
	hbm_etc_cmds->cmds[0].msg.tx_buf[1] = aor_data[0];
	hbm_etc_cmds->cmds[0].msg.tx_buf[2] = 0x00;
	hbm_etc_cmds->cmds[0].msg.tx_buf[3] = aor_data[1];

	/* elvss setting */
	hbm_etc_cmds->cmds[2].msg.tx_buf[1] = D8h_129th;

	/* elvss_dim offset */
	br_interpolation_generate_event(vdd, GEN_HBM_INTERPOLATION_ELVSS, &hbm_etc_cmds->cmds[4].msg.tx_buf[1]);

	/* opr average calculation */
	hbm_etc_cmds->cmds[6].msg.tx_buf[1] = acl_opr;

	/* Start setting */
	hbm_etc_cmds->cmds[8].msg.tx_buf[1] = acl_start;

	/* acl */
	hbm_etc_cmds->cmds[10].msg.tx_buf[1] = acl_percent;

	LCD_INFO("acl_val:%d elvss_setting : 0x%x elvss_offset:0x%x opr:0x%x start:0x%x acl:0x%x\n",
			vdd->br_info.gradual_acl_val,
			hbm_etc_cmds->cmds[2].msg.tx_buf[1],
			hbm_etc_cmds->cmds[4].msg.tx_buf[1],
			acl_opr, acl_start, acl_percent);

	*level_key = LEVEL_KEY_NONE;

	return hbm_etc_cmds;
}

static struct dsi_panel_cmd_set *ss_hbm_off(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *hbm_off_cmds = ss_get_cmds(vdd, TX_HBM_OFF);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_off_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	hbm_off_cmds->cmds[1].msg.tx_buf[1] = D2h_112th;

	*level_key = LEVEL_KEY_NONE;

	return hbm_off_cmds;
}


#define COORDINATE_DATA_SIZE 6
#define MDNIE_SCR_WR_ADDR 50

#define F1(x,y) ((y)-((39*(x))/38)-95)
#define F2(x,y) ((y)-((36*(x))/35)-56)
#define F3(x,y) ((y)+((7*(x))/1)-24728)
#define F4(x,y) ((y)+((25*(x))/7)-14031)

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
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* dummy */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_9 */
};

static char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE] = {
	coordinate_data_2, /* DYNAMIC - Normal */
	coordinate_data_2, /* STANDARD - sRGB/Adobe RGB */
	coordinate_data_2, /* NATURAL - sRGB/Adobe RGB */
	coordinate_data_1, /* MOVIE - Normal */
	coordinate_data_1, /* AUTO - Normal */
	coordinate_data_1, /* READING - Normal */
};

static int mdnie_coordinate_index(int x, int y)
{
	int tune_number = 0;

	if (F1(x,y) > 0) {
		if (F3(x,y) > 0) {
			tune_number = 3;
		} else {
			if (F4(x,y) < 0)
				tune_number = 1;
			else
				tune_number = 2;
		}
	} else {
		if (F2(x,y) < 0) {
			if (F3(x,y) > 0) {
				tune_number = 9;
			} else {
				if (F4(x,y) < 0)
					tune_number = 7;
				else
					tune_number = 8;
			}
		} else {
			if (F3(x,y) > 0)
				tune_number = 6;
			else {
				if (F4(x,y) < 0)
					tune_number = 4;
				else
					tune_number = 5;
			}
		}
	}

	return tune_number;
}

static int ss_mdnie_read(struct samsung_display_driver_data *vdd)
{
	char x_y_location[4];
	int mdnie_tune_index = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* Read mtp (D8h 123~126th) for ddi id */
	if (ss_get_cmds(vdd, RX_MDNIE)->count) {
		ss_panel_data_read(vdd, RX_MDNIE, x_y_location, LEVEL_KEY_NONE);

		vdd->mdnie.mdnie_x = x_y_location[0] << 8 | x_y_location[1];	/* X */
		vdd->mdnie.mdnie_y = x_y_location[2] << 8 | x_y_location[3];	/* Y */

		mdnie_tune_index = mdnie_coordinate_index(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);

		coordinate_tunning_multi(vdd, coordinate_data_multi, mdnie_tune_index,
				MDNIE_SCR_WR_ADDR, COORDINATE_DATA_SIZE);  // JUN_TEMP : From TAB_S4 : MDNIE_SCR_WR_ADDR

		LCD_INFO("DSI%d : X-%d Y-%d \n", vdd->ndx,
			vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);
	} else {
		LCD_ERR("DSI%d no mdnie_read_rx_cmds cmds", vdd->ndx);
		return false;
	}

	return true;
}

static int ss_smart_dimming_init(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl)
{
	struct dsi_panel_cmd_set *pcmds;
	struct dsi_panel_cmd_set *hbm_gamma_cmds = ss_get_cmds(vdd, TX_HBM_GAMMA);

	struct smartdim_conf *sconf;

	sconf = vdd->panel_func.samsung_smart_get_conf();
	if (IS_ERR_OR_NULL(sconf)) {
		LCD_ERR("fail to get smartdim_conf (ndx: %d)\n", vdd->ndx);
		return false;
	}

	LCD_INFO("++\n");

	br_tbl->smart_dimming_dsi = sconf;
#if 0
	//if (IS_ERR_OR_NULL(vdd->panel_func.samsung_smart_get_conf)) {
	//	LCD_ERR("DSI%d samsung_smart_get_conf is null", vdd->ndx);
	//	return false;
	//}

	vdd->smart_dimming_dsi = vdd->panel_func.samsung_smart_get_conf();
	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi)) {
		LCD_ERR("DSI%d smart_dimming_dsi is null", vdd->ndx);
		return false;
	}
#endif
	//ss_panel_data_read(vdd, RX_SMART_DIM_MTP, vdd->smart_dimming_dsi->mtp_buffer, LEVEL_KEY_NONE);
	ss_panel_data_read(vdd, RX_SMART_DIM_MTP, sconf->mtp_buffer, LEVEL_KEY_NONE);

	/* Modifying hbm gamma tx command for Gamma Offset Index 4 */
	memcpy(&hbm_gamma_cmds->cmds[0].msg.tx_buf[1], hbm_buffer1, 33);

	/* Initialize smart dimming related things here */
	/* lux_tab setting for 350cd */
	//vdd->smart_dimming_dsi->lux_tab = vdd->dtsi_data.candela_map_table[NORMAL][vdd->panel_revision].cd;
	sconf->lux_tab = vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].cd;
	sconf->lux_tabsize = vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].tab_size;
	sconf->man_id = vdd->manufacture_id_dsi;
	//vdd->smart_dimming_dsi->lux_tabsize = vdd->dtsi_data.candela_map_table[NORMAL][vdd->panel_revision].tab_size;
	//vdd->smart_dimming_dsi->man_id = vdd->manufacture_id_dsi;

	/* copy hbm gamma payload for hbm interpolation calc */
	pcmds = ss_get_cmds(vdd, TX_HBM_GAMMA);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_HBM_GAMMA.. \n");
		return -EINVAL;
	}
	//vdd->smart_dimming_dsi->hbm_payload = &pcmds->cmds[0].msg.tx_buf[1];
	sconf->hbm_payload = &pcmds->cmds[0].msg.tx_buf[1];

	/* Just a safety check to ensure smart dimming data is initialised well */
	sconf->init(sconf);
	//if (vdd->smart_dimming_dsi->init)
	//	vdd->smart_dimming_dsi->init(vdd->smart_dimming_dsi);
	//else
	//	LCD_ERR("Smart Dimming Init Function is NULL \n");

	vdd->br_info.temperature = 20; // default temperature

	vdd->br_info.smart_dimming_loaded_dsi = true;

	LCD_INFO("DSI%d : --\n", vdd->ndx);

	return true;
}

static struct dsi_panel_cmd_set *ss_aid(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *aid_cmds = ss_get_cmds(vdd, TX_PAC_AID_SUBDIVISION);
	u8 aor_data[AOR_SIZE];

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(aid_cmds)) {
		LCD_ERR("No aid_tx_cmds\n");
		return NULL;
	}

	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_AOR, aor_data);

	aid_cmds->cmds->msg.tx_buf[1] = aor_data[0];
	aid_cmds->cmds->msg.tx_buf[2] = 0x00;
	aid_cmds->cmds->msg.tx_buf[3] = aor_data[1];

	vdd->br_info.common_br.aor_data = (aid_cmds->cmds->msg.tx_buf[1] << 8)
							| aid_cmds->cmds->msg.tx_buf[3];

	LCD_DEBUG("bl_level:%d candela:%dCD aid(0x%x 0x%x 0x%x)\n",
			vdd->br_info.common_br.bl_level,vdd->br_info.common_br.cd_level,
			aid_cmds->cmds->msg.tx_buf[1],
			aid_cmds->cmds->msg.tx_buf[2],
			aid_cmds->cmds->msg.tx_buf[3]);

	*level_key = LEVEL_KEY_NONE;

	return aid_cmds;
}

static struct dsi_panel_cmd_set *ss_acl_on(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL_KEY_NONE;

	pcmds = ss_get_cmds(vdd, TX_ACL_ON);

	if (vdd->br_info.gradual_acl_val)
		pcmds->cmds[5].msg.tx_buf[1] = vdd->br_info.gradual_acl_val; /* ACL ON */
	else
		pcmds->cmds[5].msg.tx_buf[1] = 0x10; /* ACL OFF */

	LCD_INFO("gradual_acl_val : %d\n", vdd->br_info.gradual_acl_val);

	return ss_get_cmds(vdd, TX_ACL_ON);

}

static struct dsi_panel_cmd_set *ss_acl_off(struct samsung_display_driver_data *vdd, int *level_key)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL_KEY_NONE;

	return ss_get_cmds(vdd, TX_ACL_OFF);
}

static struct dsi_panel_cmd_set *ss_elvss(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *elvss_cmds = ss_get_cmds(vdd, TX_ELVSS);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (SS_IS_CMDS_NULL(elvss_cmds)) {
		LCD_ERR("No elvss_tx_cmds\n");
		return NULL;
	}

	/* elvss_dim_offset */
	br_interpolation_generate_event(vdd, GEN_NORMAL_INTERPOLATION_ELVSS, &elvss_cmds->cmds[1].msg.tx_buf[1]);

	*level_key = LEVEL_KEY_NONE;

	LCD_DEBUG("bl_level : %d candela : %dCD elvss_dim_offset : 0x%x\n",
			vdd->br_info.common_br.bl_level, vdd->br_info.common_br.cd_level,
			elvss_cmds->cmds[1].msg.tx_buf[1]);

	return elvss_cmds;
}

static struct dsi_panel_cmd_set *ss_gamma(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set  *gamma_cmds = ss_get_cmds(vdd, TX_GAMMA);

	if (IS_ERR_OR_NULL(vdd) || SS_IS_CMDS_NULL(gamma_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx cmds : 0x%zx", (size_t)vdd, (size_t)gamma_cmds);
		return NULL;
	}

	LCD_DEBUG("bl_level : %d candela : %dCD\n", vdd->br_info.common_br.bl_level, vdd->br_info.common_br.cd_level);

	*level_key = LEVEL_KEY_NONE;

	br_interpolation_generate_event(vdd, GEN_NORMAL_GAMMA, &gamma_cmds->cmds[0].msg.tx_buf[1]);

	return gamma_cmds;
}

static int dsi_update_mdnie_data(struct samsung_display_driver_data *vdd)
{
	struct mdnie_lite_tune_data *mdnie_data;

	LCD_INFO("++\n");

	mdnie_data = kzalloc(sizeof(struct mdnie_lite_tune_data), GFP_KERNEL);
	if (!mdnie_data) {
		LCD_ERR("fail to allocate mdnie_data memory\n");
		return -ENOMEM;
	}

	/* Update mdnie command */
	mdnie_data->DSI_COLOR_BLIND_MDNIE_1 = DSI0_COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_COLOR_BLIND_MDNIE_2 = DSI0_COLOR_BLIND_MDNIE_2;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_1 = DSI0_RGB_SENSOR_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_2 = DSI0_RGB_SENSOR_MDNIE_2;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE_2 = DSI0_UI_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_UI_STANDARD_MDNIE_2 = DSI0_UI_STANDARD_MDNIE_2;
	mdnie_data->DSI_UI_AUTO_MDNIE_2 = DSI0_UI_AUTO_MDNIE_2;
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE_2 = DSI0_VIDEO_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE_2 = DSI0_VIDEO_STANDARD_MDNIE_2;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE_2 = DSI0_VIDEO_AUTO_MDNIE_2;
	mdnie_data->DSI_CAMERA_MDNIE_2 = DSI0_CAMERA_AUTO_MDNIE_2;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE_2 = DSI0_CAMERA_AUTO_MDNIE_2;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE_2 = DSI0_GALLERY_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE_2 = DSI0_GALLERY_STANDARD_MDNIE_2;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE_2 = DSI0_GALLERY_AUTO_MDNIE_2;
	mdnie_data->DSI_VT_DYNAMIC_MDNIE_2 = DSI0_VT_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_VT_STANDARD_MDNIE_2 = DSI0_VT_STANDARD_MDNIE_2;
	mdnie_data->DSI_VT_AUTO_MDNIE_2 = DSI0_VT_AUTO_MDNIE_2;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE_2 = DSI0_BROWSER_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE_2 = DSI0_BROWSER_STANDARD_MDNIE_2;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE_2 = DSI0_BROWSER_AUTO_MDNIE_2;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE_2 = DSI0_EBOOK_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE_2 = DSI0_EBOOK_STANDARD_MDNIE_2;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE_2 = DSI0_EBOOK_AUTO_MDNIE_2;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE_2 = DSI0_TDMB_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE_2 = DSI0_TDMB_STANDARD_MDNIE_2;
	mdnie_data->DSI_TDMB_AUTO_MDNIE_2 = DSI0_TDMB_AUTO_MDNIE_2;

	mdnie_data->DSI_BYPASS_MDNIE = DSI0_BYPASS_MDNIE;
	mdnie_data->DSI_NEGATIVE_MDNIE = DSI0_NEGATIVE_MDNIE;
	mdnie_data->DSI_COLOR_BLIND_MDNIE = DSI0_COLOR_BLIND_MDNIE;
	mdnie_data->DSI_HBM_CE_MDNIE = DSI0_HBM_CE_MDNIE;
	mdnie_data->DSI_HBM_CE_TEXT_MDNIE = DSI0_HBM_CE_TEXT_MDNIE;
	mdnie_data->DSI_RGB_SENSOR_MDNIE = DSI0_RGB_SENSOR_MDNIE;
	mdnie_data->DSI_CURTAIN = DSI0_CURTAIN;
	mdnie_data->DSI_GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
	mdnie_data->DSI_GRAYSCALE_NEGATIVE_MDNIE = DSI0_GRAYSCALE_NEGATIVE_MDNIE;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE = DSI0_UI_DYNAMIC_MDNIE;
	mdnie_data->DSI_UI_STANDARD_MDNIE = DSI0_UI_STANDARD_MDNIE;
	mdnie_data->DSI_UI_NATURAL_MDNIE = DSI0_UI_NATURAL_MDNIE;
	mdnie_data->DSI_UI_MOVIE_MDNIE = DSI0_UI_MOVIE_MDNIE;
	mdnie_data->DSI_UI_AUTO_MDNIE = DSI0_UI_AUTO_MDNIE;
	mdnie_data->DSI_VIDEO_OUTDOOR_MDNIE = DSI0_VIDEO_OUTDOOR_MDNIE;
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE = DSI0_VIDEO_DYNAMIC_MDNIE;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE = DSI0_VIDEO_STANDARD_MDNIE;
	mdnie_data->DSI_VIDEO_NATURAL_MDNIE = DSI0_VIDEO_NATURAL_MDNIE;
	mdnie_data->DSI_VIDEO_MOVIE_MDNIE = DSI0_VIDEO_MOVIE_MDNIE;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE = DSI0_VIDEO_AUTO_MDNIE;
	mdnie_data->DSI_VIDEO_WARM_OUTDOOR_MDNIE = DSI0_VIDEO_OUTDOOR_MDNIE;
	mdnie_data->DSI_VIDEO_WARM_MDNIE = DSI0_VIDEO_OUTDOOR_MDNIE;
	mdnie_data->DSI_VIDEO_COLD_OUTDOOR_MDNIE = DSI0_VIDEO_OUTDOOR_MDNIE;
	mdnie_data->DSI_VIDEO_COLD_MDNIE = DSI0_VIDEO_OUTDOOR_MDNIE;
	mdnie_data->DSI_CAMERA_OUTDOOR_MDNIE = DSI0_CAMERA_OUTDOOR_MDNIE;
	mdnie_data->DSI_CAMERA_MDNIE = DSI0_CAMERA_AUTO_MDNIE;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE = DSI0_CAMERA_AUTO_MDNIE;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE = DSI0_GALLERY_DYNAMIC_MDNIE;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE = DSI0_GALLERY_STANDARD_MDNIE;
	mdnie_data->DSI_GALLERY_NATURAL_MDNIE = DSI0_GALLERY_NATURAL_MDNIE;
	mdnie_data->DSI_GALLERY_MOVIE_MDNIE = DSI0_GALLERY_MOVIE_MDNIE;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE = DSI0_GALLERY_AUTO_MDNIE;
	mdnie_data->DSI_VT_DYNAMIC_MDNIE = DSI0_VT_DYNAMIC_MDNIE;
	mdnie_data->DSI_VT_STANDARD_MDNIE = DSI0_VT_STANDARD_MDNIE;
	mdnie_data->DSI_VT_NATURAL_MDNIE = DSI0_VT_NATURAL_MDNIE;
	mdnie_data->DSI_VT_MOVIE_MDNIE = DSI0_VT_MOVIE_MDNIE;
	mdnie_data->DSI_VT_AUTO_MDNIE = DSI0_VT_AUTO_MDNIE;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE = DSI0_BROWSER_DYNAMIC_MDNIE;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE = DSI0_BROWSER_STANDARD_MDNIE;
	mdnie_data->DSI_BROWSER_NATURAL_MDNIE = DSI0_BROWSER_NATURAL_MDNIE;
	mdnie_data->DSI_BROWSER_MOVIE_MDNIE = DSI0_BROWSER_MOVIE_MDNIE;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE = DSI0_BROWSER_AUTO_MDNIE;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE = DSI0_EBOOK_DYNAMIC_MDNIE;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE = DSI0_EBOOK_STANDARD_MDNIE;
	mdnie_data->DSI_EBOOK_NATURAL_MDNIE = DSI0_EBOOK_NATURAL_MDNIE;
	mdnie_data->DSI_EBOOK_MOVIE_MDNIE = DSI0_EBOOK_MOVIE_MDNIE;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE = DSI0_EBOOK_AUTO_MDNIE;
	mdnie_data->DSI_EMAIL_AUTO_MDNIE = DSI0_EMAIL_AUTO_MDNIE;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE = DSI0_TDMB_DYNAMIC_MDNIE;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE = DSI0_TDMB_STANDARD_MDNIE;
	mdnie_data->DSI_TDMB_NATURAL_MDNIE = DSI0_TDMB_NATURAL_MDNIE;
	mdnie_data->DSI_TDMB_MOVIE_MDNIE = DSI0_TDMB_NATURAL_MDNIE;
	mdnie_data->DSI_TDMB_AUTO_MDNIE = DSI0_TDMB_AUTO_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR = DSI0_NIGHT_MODE_MDNIE_1;
	mdnie_data->DSI_COLOR_LENS_MDNIE = DSI0_COLOR_LENS_MDNIE;
	mdnie_data->DSI_COLOR_LENS_MDNIE_SCR = DSI0_COLOR_LENS_MDNIE_1;
	mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR = DSI0_RGB_SENSOR_MDNIE_1;

	mdnie_data->mdnie_tune_value_dsi = mdnie_tune_value_dsi0;
	//mdnie_data.mdnie_tune_value_dsi1 = mdnie_tune_value_dsi0;

	mdnie_data->light_notification_tune_value_dsi = light_notification_tune_value_dsi0;
	//mdnie_data.light_notification_tune_value_dsi1 = light_notification_tune_value_dsi0;

	/* Update MDNIE data related with size, offset or index */
	mdnie_data->dsi_bypass_mdnie_size = ARRAY_SIZE(DSI0_BYPASS_MDNIE);
	mdnie_data->mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
	mdnie_data->mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = ADDRESS_SCR_WHITE_RED;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = ADDRESS_SCR_WHITE_GREEN;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = ADDRESS_SCR_WHITE_BLUE;
	mdnie_data->dsi_rgb_sensor_mdnie_1_size = DSI0_RGB_SENSOR_MDNIE_1_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_2_size = DSI0_RGB_SENSOR_MDNIE_2_SIZE;
	mdnie_data->hdr_tune_value_dsi = hdr_tune_value_dsi0;
	//mdnie_data->hdr_tune_value_dsi1 = hdr_tune_value_dsi0;
	mdnie_data->dsi_adjust_ldu_table = adjust_ldu_data;
	//mdnie_data.dsi1_adjust_ldu_table = adjust_ldu_data;
	mdnie_data->dsi_max_adjust_ldu = 6;
	//mdnie_data.dsi1_max_adjust_ldu = 6;
	mdnie_data->dsi_night_mode_table = night_mode_data;
	//mdnie_data.dsi1_night_mode_table = night_mode_data;
	mdnie_data->dsi_max_night_mode_index = 11;
	//mdnie_data.dsi1_max_night_mode_index = 11;
	mdnie_data->dsi_color_lens_table = color_lens_data;
	//mdnie_data.dsi1_color_lens_table = color_lens_data;
	mdnie_data->dsi_scr_step_index = MDNIE_STEP1_INDEX;
	//mdnie_data.dsi1_scr_step_index = MDNIE_STEP1_INDEX;
	mdnie_data->dsi_white_default_r = 0xff;
	mdnie_data->dsi_white_default_g = 0xff;
	mdnie_data->dsi_white_default_b = 0xff;
	//mdnie_data->dsi1_white_default_r = 0xff;
	//mdnie_data->dsi1_white_default_g = 0xff;
	//mdnie_data->dsi1_white_default_b = 0xff;

	vdd->mdnie.mdnie_data = mdnie_data;

	LCD_INFO("--\n");
	return 0;
}

static void samsung_panel_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("++\n");
	LCD_ERR("%s\n", ss_get_panel_name(vdd));

	/* Default Panel Power Status is OFF */
	vdd->panel_state = PANEL_PWR_OFF;

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = samsung_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = samsung_panel_on_post;

	/* DDI RX */
	vdd->panel_func.samsung_panel_revision = ss_panel_revision;
	vdd->panel_func.samsung_manufacture_date_read = ss_manufacture_date_read;
	vdd->panel_func.samsung_ddi_id_read = ss_ddi_id_read;
	vdd->panel_func.samsung_cell_id_read = ss_cell_id_read;
	vdd->panel_func.samsung_octa_id_read = NULL;
	vdd->panel_func.samsung_elvss_read = NULL;
	vdd->panel_func.samsung_hbm_read = ss_hbm_read;
	vdd->panel_func.samsung_mdnie_read = ss_mdnie_read;

	vdd->panel_func.samsung_smart_dimming_init = ss_smart_dimming_init;

	vdd->panel_func.samsung_smart_get_conf = smart_get_conf_ANA38401_AMSA05RB06;

	/* GAMMA FLASH */
	vdd->panel_func.gen_hbm_interpolation_gamma = NULL;
	vdd->panel_func.gen_hbm_interpolation_irc = NULL;
	vdd->panel_func.gen_normal_interpolation_irc = NULL;
	vdd->panel_func.samsung_flash_gamma_support = NULL;
	vdd->panel_func.samsung_interpolation_init = init_interpolation_ANA38401_AMSA05RB06;
	vdd->panel_func.get_gamma_V_size = NULL;
	vdd->panel_func.convert_GAMMA_to_V = NULL;
	vdd->panel_func.convert_V_to_GAMMA = NULL;

	/* Brightness */
	vdd->panel_func.samsung_brightness_hbm_off = ss_hbm_off;
	vdd->panel_func.samsung_brightness_aid = ss_aid;
	vdd->panel_func.samsung_brightness_acl_on = ss_acl_on;
	vdd->panel_func.samsung_brightness_pre_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_acl_off = ss_acl_off;
	vdd->panel_func.samsung_brightness_pre_elvss = NULL;
	vdd->panel_func.samsung_brightness_elvss = ss_elvss;
	vdd->panel_func.samsung_brightness_elvss_temperature1 = NULL;
	vdd->panel_func.samsung_brightness_elvss_temperature2 = NULL;
	vdd->panel_func.samsung_brightness_vint = NULL;
	vdd->panel_func.samsung_brightness_gamma = ss_gamma;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = ss_hbm_gamma;
	vdd->panel_func.samsung_hbm_etc = ss_hbm_etc;
	vdd->panel_func.samsung_hbm_irc = NULL;
	vdd->panel_func.get_hbm_candela_value = NULL;

	/* Panel LPM */
	//vdd->panel_func.samsung_get_panel_lpm_mode = NULL;

	/* default brightness */
	vdd->br_info.common_br.bl_level = 255;//pac?

	/* mdnie */
	vdd->mdnie.support_mdnie = false; /* TEMP Block*/
	vdd->mdnie.mdnie_tune_size[1]= 2;
	vdd->mdnie.mdnie_tune_size[2]= 56;
	vdd->mdnie.mdnie_tune_size[3]= 2;
	vdd->mdnie.mdnie_tune_size[4]= 103;

	dsi_update_mdnie_data(vdd);

	/* send recovery pck before sending image date (for ESD recovery) */
	vdd->esd_recovery.send_esd_recovery = false;
	vdd->br_info.common_br.auto_level = 12;

	/* Enable panic on first pingpong timeout */
	if (vdd->debug_data)
		vdd->debug_data->panic_on_pptimeout = true;

	/* Set IRC init value */
	vdd->br_info.common_br.irc_mode = IRC_MODERATO_MODE;

	/* COLOR WEAKNESS */
	vdd->panel_func.color_weakness_ccb_on_off = NULL;

	/* Support DDI HW CURSOR */
	vdd->panel_func.ddi_hw_cursor = NULL;

	/* ACL default ON */
	vdd->br_info.acl_status = 1;

	/* SAMSUNG_FINGERPRINT */
	//vdd->panel_hbm_entry_delay = 2; //hbm need some TE to be updated.
	vdd->panel_hbm_entry_after_te = 2000;
	vdd->panel_hbm_exit_delay = 0;

	LCD_INFO("--\n");
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx = PRIMARY_DISPLAY_NDX;
	char panel_string[] = "ss_dsi_panel_ANA38401_AMSA05RB06_WQXGA";
	char panel_name[MAX_CMDLINE_PARAM_LEN];
	char panel_secondary_name[MAX_CMDLINE_PARAM_LEN];

	LCD_INFO("++\n");

	ss_get_primary_panel_name_cmdline(panel_name);
	ss_get_secondary_panel_name_cmdline(panel_secondary_name);

	/* TODO: use component_bind with panel_func
	 * and match by panel_string, instead.
	 */
	if (!strncmp(panel_string, panel_name, strlen(panel_string)))
		ndx = PRIMARY_DISPLAY_NDX;
	else if (!strncmp(panel_string, panel_secondary_name,
				strlen(panel_string)))
		ndx = SECONDARY_DISPLAY_NDX;
	else {
		LCD_ERR("string (%s) can not find given panel_name (%s, %s)\n", panel_string, panel_name, panel_secondary_name);
		return 0;
	}
	vdd = ss_get_vdd(ndx);
	vdd->panel_func.samsung_panel_init = samsung_panel_init;

	if (ndx == PRIMARY_DISPLAY_NDX)
		LCD_INFO("%s done.. \n", panel_name);
	else
		LCD_INFO("%s done.. \n", panel_secondary_name);
	LCD_INFO("--\n");

	return 0;
}

early_initcall(samsung_panel_initialize);
