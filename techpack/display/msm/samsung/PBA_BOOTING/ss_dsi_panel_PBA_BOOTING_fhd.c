/*
 * =================================================================
 *
 *
 *	Description:  samsung display panel file
 *
 *	Author: jb09.kim
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
#include "ss_dsi_panel_PBA_BOOTING_fhd.h"

static void samsung_pba_config(struct samsung_display_driver_data *vdd, void *arg)
{

#if 1
	/* case 03756419: QC recommended not to change has_src_split setting here.
	 * No effect of has_src_split setting in PBA booting mode.
	 * After verifying it on picasso board, remove below depricated code..
	 */
	LCD_INFO("skip to disable source split for pba\n");
#else
	struct sde_mdss_cfg *sde_cfg = (struct sde_mdss_cfg *)arg;

	if (!IS_ERR_OR_NULL(sde_cfg)) {
		LCD_INFO("Disable source split\n");
		sde_cfg->has_src_split = false;
	}
#endif
}

static void samsung_panel_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("%s\n", ss_get_panel_name(vdd));

	vdd->mdnie.support_mdnie = false;
	vdd->dtsi_data.tft_common_support = true;
	vdd->panel_func.samsung_pba_config = samsung_pba_config;
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_string[] = "ss_dsi_panel_PBA_BOOTING_FHD";
	char panel_name[MAX_CMDLINE_PARAM_LEN];
	char panel_secondary_name[MAX_CMDLINE_PARAM_LEN];

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
	else
		return 0;

	vdd = ss_get_vdd(ndx);
	vdd->panel_func.samsung_panel_init = samsung_panel_init;

	return 0;


}
early_initcall(samsung_panel_initialize);
