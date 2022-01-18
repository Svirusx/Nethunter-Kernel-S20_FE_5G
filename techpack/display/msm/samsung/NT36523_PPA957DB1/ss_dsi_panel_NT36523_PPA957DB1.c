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
#include "ss_dsi_panel_NT36523_PPA957DB1.h"

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("++\n");
	ss_panel_attach_set(vdd, true);
	LCD_INFO("--\n");

	return true;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	isl98608_buck_control(vdd);

	ss_send_cmd(vdd, TX_DFPS);

	return true;
}

static char ss_panel_revision(struct samsung_display_driver_data *vdd)
{
	if (vdd->manufacture_id_dsi == PBA_ID)
		ss_panel_attach_set(vdd, true);
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

/* Below value is config_display.xml data */
#define config_screenBrightnessSettingMaximum (255)
#define config_screenBrightnessSettingDefault (125)
#define config_screenBrightnessSettingMinimum (2)
#define config_screenBrightnessSettingzero (0)

/* Below value is pwm value & candela matching data at the 9bit 20khz PWM*/
#define PWM_Outdoor (462) //600CD
#define PWM_Maximum (382) //500CD
#define PWM_Default (133) //190CD
#define PWM_Minimum (3) //4CD
#define PWM_ZERO (0) //0CD

#define BIT_SHIFT 10

struct dsi_panel_cmd_set * tft_pwm_ldi(struct samsung_display_driver_data *vdd, int *level_key)
{
	struct dsi_panel_cmd_set *pwm_cmds = ss_get_cmds(vdd, TX_BRIGHT_CTRL);
	int bl_level = vdd->br_info.common_br.bl_level;
	unsigned long long result;
	unsigned long long multiple;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(pwm_cmds)) {
		LCD_ERR("Invalid data vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	/* 9bit PWM */
	if (bl_level > config_screenBrightnessSettingMaximum) {
		result = PWM_Outdoor;
	} else if ((bl_level > config_screenBrightnessSettingDefault) && (bl_level <= config_screenBrightnessSettingMaximum)) {
		/*
			(((382 - 139 ) / (255 - 125)) * (bl_level - 125)) + 139
		*/
		multiple = (PWM_Maximum - PWM_Default);
		multiple <<= BIT_SHIFT;
		do_div(multiple, config_screenBrightnessSettingMaximum - config_screenBrightnessSettingDefault);

		result = (bl_level - config_screenBrightnessSettingDefault) * multiple;
		result >>= BIT_SHIFT;
		result += PWM_Default;
	} else if ((bl_level > config_screenBrightnessSettingMinimum) && (bl_level <= config_screenBrightnessSettingDefault)) {
		/*
			(139/125) * bl_level
		*/
		result = bl_level * PWM_Default;
		result <<=  BIT_SHIFT;
		do_div(result, config_screenBrightnessSettingDefault);
		result >>= BIT_SHIFT;
	} else if ((bl_level > config_screenBrightnessSettingzero) && (bl_level <= config_screenBrightnessSettingMinimum)){
		result = PWM_Minimum;
	} else
		result = PWM_ZERO;

	pwm_cmds->cmds->msg.tx_buf[1] = (u8)(result >> 8);
	pwm_cmds->cmds->msg.tx_buf[2] = (u8)(result & 0xFF);

	LCD_ERR("bl_level : %d %lld", vdd->br_info.common_br.bl_level, result);

	*level_key = LEVEL_KEY_NONE;

	return pwm_cmds;
}

void dfps_update(struct samsung_display_driver_data *vdd, int fps)
{
	struct dsi_panel_cmd_set *dpfs = ss_get_cmds(vdd, TX_DFPS);
	u8 data = 0;

	if (fps == 120)
		data = 0x0D;
	else if (fps == 96)
		data = 0x0E;
	else if (fps == 60)
		data = 0x0C;
	else
		data = 0x0F;

	dpfs->cmds[2].msg.tx_buf[1] = data;
	ss_send_cmd(vdd, TX_DFPS);

	LCD_INFO("fps : %d data : 0x%x\n", fps, data);
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
	vdd->panel_func.samsung_manufacture_date_read = NULL;

	/* default brightness */
	vdd->br_info.common_br.bl_level = 255;//pac?

	/* mdnie */
	vdd->mdnie.support_mdnie = false;

	/* Brightness */
	vdd->panel_func.samsung_brightness_tft_pwm_ldi = tft_pwm_ldi;

	vdd->panel_func.samsung_dfps_panel_update = dfps_update;

	/* send recovery pck before sending image date (for ESD recovery) */
	vdd->esd_recovery.send_esd_recovery = false;
	vdd->br_info.common_br.auto_level = 12;

	/* Enable panic on first pingpong timeout */
	if (vdd->debug_data)
		vdd->debug_data->panic_on_pptimeout = true;

	/* SAMSUNG_FINGERPRINT */
	//vdd->panel_hbm_entry_delay = 2; //hbm need some TE to be updated.

	LCD_INFO("--\n");
}

static int __init samsung_panel_initialize(void)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx = PRIMARY_DISPLAY_NDX;
	char panel_string[] = "ss_dsi_panel_NT36523_PPA957DB1_WQXGA";
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
