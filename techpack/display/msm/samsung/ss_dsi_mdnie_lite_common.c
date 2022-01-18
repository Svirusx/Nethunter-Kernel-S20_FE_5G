/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
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

#include "ss_dsi_mdnie_lite_common.h"

#define MDNIE_LITE_TUN_DEBUG

#ifdef MDNIE_LITE_TUN_DEBUG
#define DPRINT(x...)	printk(KERN_ERR "[SDE_mdnie] " x)
#else
#define DPRINT(x...)
#endif

static struct class *mdnie_class;

char mdnie_app_name[][NAME_STRING_MAX] = {
	"UI_APP",
	"VIDEO_APP",
	"VIDEO_WARM_APP",
	"VIDEO_COLD_APP",
	"CAMERA_APP",
	"NAVI_APP",
	"GALLERY_APP",
	"VT_APP",
	"BROWSER_APP",
	"eBOOK_APP",
	"EMAIL_APP",
	"GAME_LOW_APP",
	"GAME_MID_APP",
	"GAME_HIGH_APP",
	"VIDEO_ENHANCER",
	"VIDEO_ENHANCER_THIRD",
	"TDMB_APP",
};

char mdnie_mode_name[][NAME_STRING_MAX] = {
	"DYNAMIC_MODE",
	"STANDARD_MODE",
#if defined(NATURAL_MODE_ENABLE)
	"NATURAL_MODE",
#endif
	"MOVIE_MODE",
	"AUTO_MODE",
	"READING_MODE",
};

char outdoor_name[][NAME_STRING_MAX] = {
	"OUTDOOR_OFF_MODE",
	"OUTDOOR_ON_MODE",
};

char mdnie_hdr_name[][NAME_STRING_MAX] = {
	"HDR_OFF",
	"HDR_1",
	"HDR_2",
	"HDR_3"
};

char mdnie_light_notification_name[][NAME_STRING_MAX] = {
	"LIGHT_NOTIFICATION_OFF",
	"LIGHT_NOTIFICATION_ON"
};

/* send_dsi_tcon_mdnie_register():
 * tx mdnie packet to panel wich vdd is included.
 */
static void send_dsi_tcon_mdnie_register(struct samsung_display_driver_data *vdd,
		struct dsi_cmd_desc *tune_data_dsi,
		struct mdnie_lite_tun_type *tune)
{
	struct mdnie_lite_tune_data *mdnie_data = vdd->mdnie.mdnie_data;
	struct dsi_panel_cmd_set *pcmds;
	int i;

	if (!vdd->mdnie.support_mdnie)
		return;

	if (!tune_data_dsi || !mdnie_data->dsi_bypass_mdnie_size) {
		DPRINT("Command Tx Fail, tune_data_dsi=%p(%d), vdd=%p, tune=%p\n",
			tune_data_dsi, mdnie_data->dsi_bypass_mdnie_size, vdd, tune);
		return;
	}

	DPRINT("hbm: %d bypass: %d accessibility: %d app: %d mode: %d hdr: %d " \
			"night_mode: %d whiteRGB: (%d %d %d) color_lens: (%d %d %d)\n",
			tune->hbm_enable, tune->mdnie_bypass, tune->mdnie_accessibility,
			tune->mdnie_app, tune->mdnie_mode, tune->hdr, tune->night_mode_enable,
			mdnie_data->dsi_white_balanced_r, mdnie_data->dsi_white_balanced_g,
			mdnie_data->dsi_white_balanced_b,
			tune->color_lens_enable, tune->color_lens_color, tune->color_lens_level);

	pcmds = ss_get_cmds(vdd, TX_MDNIE_TUNE);
	pcmds->cmds = tune_data_dsi;
	pcmds->count = mdnie_data->dsi_bypass_mdnie_size;
	pcmds->state = DSI_CMD_SET_STATE_HS;

	/* temp to avoid tx fail with single TX enabled */
	for (i = 0; i < pcmds->count; i++)
		pcmds->cmds[i].last_command = true;

	ss_send_cmd(vdd, TX_MDNIE_TUNE);
}

/* uupdate_dsi_tcon_mdnie_register():
 * pdate and tx mdnie packet to panel wich vdd is included.
 */
int update_dsi_tcon_mdnie_register(struct samsung_display_driver_data *vdd)
{
	struct mdnie_lite_tun_type *tune = NULL;
	struct dsi_cmd_desc *tune_data_dsi = NULL;
	struct mdnie_lite_tune_data *mdnie_data;
	enum BYPASS temp_bypass = BYPASS_ENABLE;

	if (vdd == NULL || !vdd->mdnie.support_mdnie)
		return 0;

	if (ss_is_seamless_mode(vdd) ||
			ss_is_panel_off(vdd)) {
		LCD_ERR("do not send mdnie data (%d) (%d)\n",
			ss_is_seamless_mode(vdd), ss_is_panel_off(vdd));
		return 0;
	}

	tune = vdd->mdnie.mdnie_tune_state_dsi;
	mdnie_data = vdd->mdnie.mdnie_data;
	/*
	*	Checking HBM mode first.
	*/
	if (vdd->br_info.lux >= vdd->mdnie.enter_hbm_ce_lux)
		tune->hbm_enable = true;
	else
		tune->hbm_enable = false;

	/*
	 * Safe Code for When LCD ON is should be LIGHT_NOTIFICATION_OFF
	 */
	if (vdd->mdnie.lcd_on_notifiy) {
		tune->light_notification = LIGHT_NOTIFICATION_OFF;
		vdd->mdnie.lcd_on_notifiy = false;
	}

	if(tune->mdnie_bypass == BYPASS_DISABLE) {
		if (ss_is_panel_lpm(vdd)) {
			if (tune->mdnie_accessibility == CURTAIN) {
				temp_bypass = BYPASS_DISABLE;
			}
			else if((tune->mdnie_accessibility == NEGATIVE) || (tune->mdnie_accessibility == GRAYSCALE_NEGATIVE) || (tune->color_lens_enable == true)){
				temp_bypass = BYPASS_ENABLE;
			}
			else if ((tune->mdnie_accessibility == COLOR_BLIND || tune->mdnie_accessibility == COLOR_BLIND_HBM) ||
				(tune->mdnie_accessibility == GRAYSCALE) || (tune->night_mode_enable == true) || (tune->ldu_mode_index != 0)) {
				temp_bypass = BYPASS_DISABLE;
			}
			else {
				temp_bypass = BYPASS_ENABLE;
			}
		}
		else {
			temp_bypass = BYPASS_DISABLE;
		}
	}

	/*
	* mDnie priority
	* Accessibility > HBM > Screen Mode
	*/
	if (temp_bypass == BYPASS_ENABLE) {
		tune_data_dsi = mdnie_data->DSI_BYPASS_MDNIE;
	} else if (tune->light_notification) {
		tune_data_dsi = mdnie_data->light_notification_tune_value_dsi[tune->light_notification];
	} else if (tune->mdnie_accessibility == COLOR_BLIND ||
			tune->mdnie_accessibility == COLOR_BLIND_HBM) {
		tune_data_dsi  = mdnie_data->DSI_COLOR_BLIND_MDNIE;
	} else if (tune->mdnie_accessibility == NEGATIVE) {
		tune_data_dsi  = mdnie_data->DSI_NEGATIVE_MDNIE;
	} else if (tune->mdnie_accessibility == CURTAIN) {
		tune_data_dsi  = mdnie_data->DSI_CURTAIN;
	} else if (tune->mdnie_accessibility == GRAYSCALE) {
		tune_data_dsi  = mdnie_data->DSI_GRAYSCALE_MDNIE;
	} else if (tune->mdnie_accessibility == GRAYSCALE_NEGATIVE) {
		tune_data_dsi  = mdnie_data->DSI_GRAYSCALE_NEGATIVE_MDNIE;
	} else if (tune->color_lens_enable == true) {
		tune_data_dsi  = mdnie_data->DSI_COLOR_LENS_MDNIE;
	} else if (tune->hmt_color_temperature) {
		tune_data_dsi =
			mdnie_data->hmt_color_temperature_tune_value_dsi[tune->hmt_color_temperature];
	} else if (tune->night_mode_enable == true) {
		tune_data_dsi  = mdnie_data->DSI_NIGHT_MODE_MDNIE;
	} else if (tune->hbm_enable == true) {
		if(tune->mdnie_mode == AUTO_MODE) {
			if (vdd->dtsi_data.hbm_ce_text_mode_support &&
				((tune->mdnie_app == BROWSER_APP) ||
				 (tune->mdnie_app == eBOOK_APP)))
				tune_data_dsi  = mdnie_data->DSI_HBM_CE_TEXT_MDNIE;
			else
				tune_data_dsi  = mdnie_data->DSI_HBM_CE_MDNIE;
		} else {
			tune_data_dsi  = mdnie_data->DSI_HBM_CE_D65_MDNIE;
		}
	} else if (tune->hdr) {
		tune_data_dsi = mdnie_data->hdr_tune_value_dsi[tune->hdr];
	} else if (tune->mdnie_app == EMAIL_APP) {
		/*
			Some kind of panel doesn't suooprt EMAIL_APP mode, but SSRM module use same control logic.
			It means SSRM doesn't consider panel unique character.
			To support this issue eBOOK_APP used insted of EMAIL_APP under EMAIL_APP doesn't exist status..
		*/
		tune_data_dsi = mdnie_data->mdnie_tune_value_dsi[tune->mdnie_app][tune->mdnie_mode][tune->outdoor];
		if (!tune_data_dsi)
			tune_data_dsi = mdnie_data->mdnie_tune_value_dsi[eBOOK_APP][tune->mdnie_mode][tune->outdoor];
	} else {
		tune_data_dsi = mdnie_data->mdnie_tune_value_dsi[tune->mdnie_app][tune->mdnie_mode][tune->outdoor];
	}

	if (vdd->mdnie.support_trans_dimming && vdd->mdnie.disable_trans_dimming && (tune->hbm_enable == false)) {
		if (tune_data_dsi) {
			memcpy(mdnie_data->DSI_RGB_SENSOR_MDNIE_1,
					tune_data_dsi[mdnie_data->mdnie_step_index[MDNIE_STEP1]].msg.tx_buf,
					mdnie_data->dsi_rgb_sensor_mdnie_1_size);
			memcpy(mdnie_data->DSI_RGB_SENSOR_MDNIE_2,
					tune_data_dsi[mdnie_data->mdnie_step_index[MDNIE_STEP2]].msg.tx_buf,
					mdnie_data->dsi_rgb_sensor_mdnie_2_size);
			memcpy(mdnie_data->DSI_RGB_SENSOR_MDNIE_3,
					tune_data_dsi[mdnie_data->mdnie_step_index[MDNIE_STEP3]].msg.tx_buf,
					mdnie_data->dsi_rgb_sensor_mdnie_3_size);

			mdnie_data->DSI_TRANS_DIMMING_MDNIE[mdnie_data->dsi_trans_dimming_data_index] = 0x0;
			tune_data_dsi = mdnie_data->DSI_RGB_SENSOR_MDNIE;
		}
	}

	if (!tune_data_dsi) {
		DPRINT("%s tune_data is NULL hbm : %d mdnie_bypass : %d mdnie_accessibility : %d  color_lens : %d"
				" mdnie_app: %d mdnie_mode : %d hdr : %d night_mode_enable : %d\n",
			__func__, tune->hbm_enable, tune->mdnie_bypass, tune->mdnie_accessibility, tune->color_lens_enable,
			tune->mdnie_app, tune->mdnie_mode, tune->hdr, tune->night_mode_enable);
		return -EFAULT;
	}

	tune->scr_white_red = tune_data_dsi[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]];
	tune->scr_white_green = tune_data_dsi[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]];
	tune->scr_white_blue = tune_data_dsi[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]];

	send_dsi_tcon_mdnie_register(vdd, tune_data_dsi, tune);

	return 0;
}

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf + buffer_pos, 256, "Current Mode: %s\n",
			mdnie_mode_name[tune->mdnie_mode]);

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int value = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	sscanf(buf, "%d", &value);

	if (value < DYNAMIC_MODE || value >= MAX_MODE) {
		DPRINT("[ERROR] wrong mode value : %d\n",
			value);
		return size;
	}

	if (vdd->dtsi_data.tft_common_support && value >= NATURAL_MODE)
		value++;

	tune->mdnie_mode = value;

	DPRINT("%s mode : %d\n", __func__, tune->mdnie_mode);

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t scenario_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf, 256, "Current APP : ");

	buffer_pos += snprintf(buf + buffer_pos, 256, "Current APP: %s\n",
			mdnie_app_name[tune->mdnie_app]);

	DPRINT("%s \n", buf);

	return buffer_pos;
}

/* app_id : App give self_app_id to mdnie driver.
* ret_id : app_id for mdnie data structure.
* example. TDMB app tell mdnie-driver that my app_id is 20. but mdnie driver will change it to TDMB_APP value.
*/
static int fake_id(int app_id)
{
	int ret_id;

	switch (app_id) {
#ifdef CONFIG_TDMB
	case APP_ID_TDMB:
		ret_id = TDMB_APP;
		DPRINT("%s : change app_id(%d) to mdnie_app(%d)\n", __func__, app_id, ret_id);
		break;
#endif
	default:
		ret_id = app_id;
		break;
	}

	return ret_id;
}


static ssize_t scenario_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int value = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	sscanf(buf, "%d", &value);
	value = fake_id(value);

	if (value < UI_APP || value >= MAX_APP_MODE) {
		DPRINT("[ERROR] wrong Scenario mode value : %d\n",
			value);
		return size;
	}

	tune->mdnie_app = value;
	DPRINT("%s APP : %d\n", __func__, tune->mdnie_app);

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t outdoor_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf + buffer_pos, 256, "Current outdoor Mode: %s\n",
			outdoor_name[tune->outdoor]);

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t outdoor_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int value = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	sscanf(buf, "%d", &value);

	if (value < OUTDOOR_OFF_MODE || value >= MAX_OUTDOOR_MODE) {
		DPRINT("[ERROR] : wrong outdoor mode value : %d\n", value);
		return size;
	}

	tune->outdoor = value;
	DPRINT("outdoor value = %d, APP = %d\n", value, tune->mdnie_app);

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t bypass_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf + buffer_pos, 256, "Current MDNIE bypass:  %s\n",
			tune->mdnie_bypass ? "ENABLE" : "DISABLE");
	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t bypass_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tun_type *tune = NULL;
	int value = 0;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	sscanf(buf, "%d", &value);

	if (value)
		tune->mdnie_bypass = BYPASS_ENABLE;
	else
		tune->mdnie_bypass = BYPASS_DISABLE;

	DPRINT("%s bypass : %s value : %d\n", __func__,
			tune->mdnie_bypass ? "ENABLE" : "DISABLE",
			value);

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t accessibility_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf + buffer_pos, 256, "Current accessibility: %s\n",
		tune->mdnie_accessibility ?
		tune->mdnie_accessibility == 1 ? "NEGATIVE" :
		tune->mdnie_accessibility == 2 ? "COLOR_BLIND" :
		tune->mdnie_accessibility == 3 ? "CURTAIN" :
		tune->mdnie_accessibility == 4 ? "GRAYSCALE" :
		tune->mdnie_accessibility == 5 ? "GRAYSCALE_NEGATIVE" :
			"COLOR_BLIND_HBM" : "ACCESSIBILITY_OFF");
	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t accessibility_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tune_data *mdnie_data;
	struct mdnie_lite_tun_type *tune = NULL;
	int cmd_value = 0;
	char buffer[MDNIE_COLOR_BLINDE_HBM_CMD_SIZE] = {0,};
	int buffer2[MDNIE_COLOR_BLINDE_HBM_CMD_SIZE/2] = {0,};
	int loop;
	char temp;

	if (!vdd)
		return size;

	mdnie_data = vdd->mdnie.mdnie_data;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	sscanf(buf, "%d %x %x %x %x %x %x %x %x %x %x %x %x", &cmd_value,
		&buffer2[0], &buffer2[1], &buffer2[2], &buffer2[3], &buffer2[4],
		&buffer2[5], &buffer2[6], &buffer2[7], &buffer2[8], &buffer2[9],
		&buffer2[10], &buffer2[11]);

	for (loop = 0; loop < MDNIE_COLOR_BLINDE_HBM_CMD_SIZE/2; loop++) {
		buffer2[loop] = buffer2[loop] & 0xFFFF;
		buffer[loop * 2] = (buffer2[loop] & 0xFF00) >> 8;
		buffer[loop * 2 + 1] = buffer2[loop] & 0xFF;
	}

	for (loop = 0; loop < MDNIE_COLOR_BLINDE_HBM_CMD_SIZE; loop += 2) {
		temp = buffer[loop];
		buffer[loop] = buffer[loop + 1];
		buffer[loop + 1] = temp;
	}

	/*
	* mDnie priority
	* Accessibility > HBM > Screen Mode
	*/
	if (cmd_value == NEGATIVE) {
		tune->mdnie_accessibility = NEGATIVE;
	} else if (cmd_value == COLOR_BLIND) {
		tune->mdnie_accessibility = COLOR_BLIND;

		if (!IS_ERR_OR_NULL(mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR))
			memcpy(&mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR[mdnie_data->mdnie_color_blinde_cmd_offset],
					buffer, MDNIE_COLOR_BLINDE_CMD_SIZE);
	} else if (cmd_value == COLOR_BLIND_HBM) {
		tune->mdnie_accessibility = COLOR_BLIND_HBM;

		if (!IS_ERR_OR_NULL(mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR))
			memcpy(&mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR[mdnie_data->mdnie_color_blinde_cmd_offset],
					buffer, MDNIE_COLOR_BLINDE_HBM_CMD_SIZE);
	} else if (cmd_value == CURTAIN) {
		tune->mdnie_accessibility = CURTAIN;
	} else if (cmd_value == GRAYSCALE) {
		tune->mdnie_accessibility = GRAYSCALE;
	} else if (cmd_value == GRAYSCALE_NEGATIVE) {
		tune->mdnie_accessibility = GRAYSCALE_NEGATIVE;
	} else if (cmd_value == ACCESSIBILITY_OFF) {
		tune->mdnie_accessibility = ACCESSIBILITY_OFF;
	} else
		DPRINT("%s ACCESSIBILITY_MAX", __func__);

#if defined(CONFIG_64BIT)
	DPRINT("%s cmd_value : %d size : %lu", __func__, cmd_value, size);
#else
	DPRINT("%s cmd_value : %d size : %u", __func__, cmd_value, size);
#endif

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t sensorRGB_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf, 256, "%d %d %d",
			tune->scr_white_red,
			tune->scr_white_green,
			tune->scr_white_blue);

	return buffer_pos;
}

static ssize_t sensorRGB_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tune_data *mdnie_data;
	int white_red = 0, white_green = 0, white_blue = 0;
	struct mdnie_lite_tun_type *tune = NULL;
	struct dsi_cmd_desc *data_dsi = NULL;
	struct dsi_cmd_desc *tune_data_dsi = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;
	mdnie_data = vdd->mdnie.mdnie_data;

	sscanf(buf, "%d %d %d", &white_red, &white_green, &white_blue);

	if (tune->ldu_mode_index == 0) {
		if ((tune->mdnie_accessibility == ACCESSIBILITY_OFF) && (tune->mdnie_mode == AUTO_MODE) &&
			((tune->mdnie_app == BROWSER_APP) || (tune->mdnie_app == eBOOK_APP))) {

			tune->scr_white_red = (char)white_red;
			tune->scr_white_green = (char)white_green;
			tune->scr_white_blue = (char)white_blue;

			DPRINT("%s: white_red = %d, white_green = %d, white_blue = %d, %d %d\n",
				__func__,
				white_red, white_green, white_blue,
				mdnie_data->dsi_rgb_sensor_mdnie_1_size,
				mdnie_data->dsi_rgb_sensor_mdnie_2_size);

			tune_data_dsi = mdnie_data->DSI_RGB_SENSOR_MDNIE;

			data_dsi = mdnie_data->mdnie_tune_value_dsi[tune->mdnie_app][tune->mdnie_mode][tune->outdoor];

			if (data_dsi) {
				memcpy(mdnie_data->DSI_RGB_SENSOR_MDNIE_1, data_dsi[mdnie_data->mdnie_step_index[MDNIE_STEP1]].msg.tx_buf, mdnie_data->dsi_rgb_sensor_mdnie_1_size);
				memcpy(mdnie_data->DSI_RGB_SENSOR_MDNIE_2, data_dsi[mdnie_data->mdnie_step_index[MDNIE_STEP2]].msg.tx_buf, mdnie_data->dsi_rgb_sensor_mdnie_2_size);
				memcpy(mdnie_data->DSI_RGB_SENSOR_MDNIE_3, data_dsi[mdnie_data->mdnie_step_index[MDNIE_STEP3]].msg.tx_buf, mdnie_data->dsi_rgb_sensor_mdnie_3_size);

				mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = white_red;
				mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = white_green;
				mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = white_blue;
			}
		}
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	send_dsi_tcon_mdnie_register(vdd, tune_data_dsi, tune);

	return size;
}

static ssize_t whiteRGB_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tune_data *mdnie_data;
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;
	int r, g, b;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;
	mdnie_data = vdd->mdnie.mdnie_data;

	r = mdnie_data->dsi_white_balanced_r;
	g = mdnie_data->dsi_white_balanced_g;
	b = mdnie_data->dsi_white_balanced_b;

	buffer_pos += snprintf(buf + buffer_pos, 256, "Current whiteRGB SETTING: %d %d %d\n", r, g, b);

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t whiteRGB_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tune_data *mdnie_data;
	int i;
	int white_red = 0, white_green = 0, white_blue = 0;
	struct mdnie_lite_tun_type *tune = NULL;
	struct dsi_cmd_desc *white_tunning_data = NULL;

	if (!vdd || !vdd->mdnie.support_mdnie)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;
	mdnie_data = vdd->mdnie.mdnie_data;

	sscanf(buf, "%d %d %d", &white_red, &white_green, &white_blue);

	DPRINT("%s: white_red = %d, white_green = %d, white_blue = %d\n", __func__, white_red, white_green, white_blue);

	if ((white_red <= 0 && white_red >= -40) && (white_green <= 0 && white_green >= -40) && (white_blue <= 0 && white_blue >= -40)) {
		if (tune->ldu_mode_index == 0) {
			mdnie_data->dsi_white_ldu_r = mdnie_data->dsi_white_default_r;
			mdnie_data->dsi_white_ldu_g = mdnie_data->dsi_white_default_g;
			mdnie_data->dsi_white_ldu_b = mdnie_data->dsi_white_default_b;
		}
		for (i = 0; i < MAX_APP_MODE; i++) {
			if ((mdnie_data->mdnie_tune_value_dsi[i][AUTO_MODE][0] != NULL) && (i != eBOOK_APP)) {
				white_tunning_data = mdnie_data->mdnie_tune_value_dsi[i][AUTO_MODE][0];
				white_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = (char)(mdnie_data->dsi_white_ldu_r + white_red);
				white_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = (char)(mdnie_data->dsi_white_ldu_g + white_green);
				white_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = (char)(mdnie_data->dsi_white_ldu_b + white_blue);
				mdnie_data->dsi_white_balanced_r = white_red;
				mdnie_data->dsi_white_balanced_g = white_green;
				mdnie_data->dsi_white_balanced_b = white_blue;
			}
		}
		white_tunning_data = mdnie_data->DSI_HBM_CE_MDNIE;
		if(white_tunning_data != NULL) {
			white_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = (char)(mdnie_data->dsi_white_ldu_r + white_red);
			white_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = (char)(mdnie_data->dsi_white_ldu_g + white_green);
			white_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = (char)(mdnie_data->dsi_white_ldu_b + white_blue);
		}
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t mdnie_ldu_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf, 256, "%d %d %d",
			tune->scr_white_red,
			tune->scr_white_green,
			tune->scr_white_blue);

	return buffer_pos;
}

static ssize_t mdnie_ldu_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tune_data *mdnie_data;
	int i, j, idx = 0;
	struct mdnie_lite_tun_type *tune = NULL;
	struct dsi_cmd_desc *ldu_tunning_data = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;
	mdnie_data = vdd->mdnie.mdnie_data;

	sscanf(buf, "%d", &idx);

	DPRINT("%s: idx = %d\n", __func__, idx);

	if ((idx >= 0) && (idx < mdnie_data->dsi_max_adjust_ldu)) {
		tune->ldu_mode_index = idx;
		for (i = 0; i < MAX_APP_MODE; i++) {
			for (j = 0; j < MAX_MODE; j++) {
				if ((mdnie_data->mdnie_tune_value_dsi[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
					ldu_tunning_data = mdnie_data->mdnie_tune_value_dsi[i][j][0];
					if (j == AUTO_MODE) {
						ldu_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] =
							mdnie_data->dsi_adjust_ldu_table[j][idx * 3 + 0] + mdnie_data->dsi_white_balanced_r;
						ldu_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] =
							mdnie_data->dsi_adjust_ldu_table[j][idx * 3 + 1] + mdnie_data->dsi_white_balanced_g;
						ldu_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] =
							mdnie_data->dsi_adjust_ldu_table[j][idx * 3 + 2] + mdnie_data->dsi_white_balanced_b;
							mdnie_data->dsi_white_ldu_r = mdnie_data->dsi_adjust_ldu_table[j][idx * 3 + 0];
							mdnie_data->dsi_white_ldu_g = mdnie_data->dsi_adjust_ldu_table[j][idx * 3 + 1];
							mdnie_data->dsi_white_ldu_b = mdnie_data->dsi_adjust_ldu_table[j][idx * 3 + 2];
					} else {
						ldu_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] =
							mdnie_data->dsi_adjust_ldu_table[j][idx * 3 + 0];
						ldu_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] =
							mdnie_data->dsi_adjust_ldu_table[j][idx * 3 + 1];
						ldu_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] =
							mdnie_data->dsi_adjust_ldu_table[j][idx * 3 + 2];
					}
				}
			}
		}
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t night_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf, 256, "%d %d",
			tune->night_mode_enable,
			tune->night_mode_index);

	return buffer_pos;
}

static ssize_t night_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tune_data *mdnie_data;
	int enable = 0, idx = 0;
	char *buffer;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;
	mdnie_data = vdd->mdnie.mdnie_data;

	sscanf(buf, "%d %d", &enable, &idx);

	tune->night_mode_enable = enable;

	DPRINT("%s: enable = %d, idx = %d\n", __func__, enable, idx);

	if (((idx >= 0) && (idx < mdnie_data->dsi_max_night_mode_index)) && (enable == true)) {
		if (!IS_ERR_OR_NULL(mdnie_data->dsi_night_mode_table)) {
			if(tune->mdnie_mode != AUTO_MODE){
				idx += mdnie_data->dsi_max_night_mode_index;
			}
			buffer = &mdnie_data->dsi_night_mode_table[(MDNIE_SCR_CMD_SIZE * idx)];
			if (!IS_ERR_OR_NULL(mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR)) {
				memcpy(&mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR[mdnie_data->mdnie_color_blinde_cmd_offset],
					buffer, MDNIE_SCR_CMD_SIZE);
				tune->night_mode_index = idx;
			}
		}
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t color_lens_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf, 256, "%d %d %d",
			tune->color_lens_enable,
			tune->color_lens_color,
			tune->color_lens_level);

	return buffer_pos;
}

static ssize_t color_lens_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tune_data *mdnie_data;
	int enable = 0, color = 0, level = 0;
	char *buffer;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;
	mdnie_data = vdd->mdnie.mdnie_data;

	sscanf(buf, "%d %d %d", &enable, &color, &level);

	tune->color_lens_enable = enable;

	DPRINT("%s: enable = %d, color = %d, level = %d\n", __func__, enable, color, level);

	if ((enable == true) && ((color >= 0) && (color < COLOR_LENS_COLOR_MAX)) && ((level >= 0) && (level < COLOR_LENS_LEVEL_MAX))) {
		if (!IS_ERR_OR_NULL(mdnie_data->dsi_color_lens_table)) {
			buffer = &mdnie_data->dsi_color_lens_table[(color * MDNIE_SCR_CMD_SIZE * COLOR_LENS_LEVEL_MAX) + (MDNIE_SCR_CMD_SIZE * level)];
			if (!IS_ERR_OR_NULL(mdnie_data->DSI_COLOR_LENS_MDNIE_SCR)) {
				memcpy(&mdnie_data->DSI_COLOR_LENS_MDNIE_SCR[mdnie_data->mdnie_color_blinde_cmd_offset],
					buffer, MDNIE_SCR_CMD_SIZE);
				tune->color_lens_color = color;
				tune->color_lens_level = level;
			}
		}
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t hdr_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf + buffer_pos, 256, "Current HDR SETTING: %s\n",
			mdnie_hdr_name[tune->hdr]);

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t hdr_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int value = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	sscanf(buf, "%d", &value);

	if (value < HDR_OFF || value >= HDR_MAX) {
		DPRINT("[ERROR] wrong hdr value : %d\n", value);
		return size;
	}

	DPRINT("%s : (%d) -> (%d)\n", __func__, tune->hdr, value);
	tune->hdr = value;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t light_notification_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	pos = snprintf(buf, 256, "Current LIGHT NOTIFICATION SETTING: %s\n",
			mdnie_light_notification_name[tune->light_notification]);

	DPRINT("%s\n", buf);

	return pos;
}

static ssize_t light_notification_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int value = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	sscanf(buf, "%d", &value);

	if (value < LIGHT_NOTIFICATION_OFF || value >= LIGHT_NOTIFICATION_MAX) {
		DPRINT("[ERROR] wrong light notification value : %d\n", value);
		return size;
	}

	DPRINT("%s : (%d) -> (%d)\n", __func__,
			tune->light_notification, value);
	tune->light_notification = value;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t afc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf, 256, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
			tune->afc_enable, tune->afc_roi[0], tune->afc_roi[1], tune->afc_roi[2], tune->afc_roi[3],
			tune->afc_roi[4], tune->afc_roi[5], tune->afc_roi[6], tune->afc_roi[7], tune->afc_roi[8],
			tune->afc_roi[9], tune->afc_roi[10], tune->afc_roi[11]);

	return buffer_pos;
}

static ssize_t afc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tune_data *mdnie_data;
	struct mdnie_lite_tun_type *tune = NULL;
	int i, enable = 0;
	int roi[12] = {0};

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;
	mdnie_data = vdd->mdnie.mdnie_data;

	if ((mdnie_data->DSI_AFC == NULL) || (mdnie_data->DSI_AFC_ON == NULL) || (mdnie_data->DSI_AFC_OFF == NULL))
		return 0;

	sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d", &enable, &roi[0], &roi[1], &roi[2], &roi[3], &roi[4], &roi[5], &roi[6], &roi[7], &roi[8], &roi[9], &roi[10], &roi[11]);

	if (enable) {
		tune->afc_enable = enable;
		DPRINT("%s: enable, roi = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
			__func__, roi[0], roi[1], roi[2], roi[3], roi[4], roi[5], roi[6], roi[7], roi[8], roi[9], roi[10], roi[11]);

		memcpy(mdnie_data->DSI_AFC, mdnie_data->DSI_AFC_ON, mdnie_data->dsi_afc_size);

		for (i = 0; i < AFC_ROI_CMD_SIZE; i++) {
			if ((roi[i] > 0) && (roi[i] <= 255))
				tune->afc_roi[i] = (char)roi[i];
			else
				tune->afc_roi[i] = 0xff;
			mdnie_data->DSI_AFC[mdnie_data->dsi_afc_index+i] = tune->afc_roi[i];
		}
	} else {
		tune->afc_enable = enable;
		for (i = 0; i < AFC_ROI_CMD_SIZE; i++) {
			tune->afc_roi[i] = 0xff;
		}

		DPRINT("%s: disable\n", __func__);

		memcpy(mdnie_data->DSI_AFC, mdnie_data->DSI_AFC_OFF, mdnie_data->dsi_afc_size);
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t cabc_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	buffer_pos += snprintf(buf + buffer_pos, 256, "Current CABC bypass: %s\n",
			tune->cabc_bypass ? "ENABLE" : "DISABLE");

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t cabc_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tun_type *tune = NULL;
	int value = 0;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	sscanf(buf, "%d", &value);

	if (value)
		tune->cabc_bypass = BYPASS_DISABLE;
	else
		tune->cabc_bypass = BYPASS_ENABLE;

	DPRINT("%s bypass : %s value : %d\n", __func__, tune->cabc_bypass ? "ENABLE" : "DISABLE", value);

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	config_cabc(vdd, value);

	return size;
}

static ssize_t hmt_color_temperature_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return 0;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	DPRINT("Current color temperature : %d\n", tune->hmt_color_temperature);

	return snprintf(buf, 256, "Current color temperature : %d\n", tune->hmt_color_temperature);
}

static ssize_t hmt_color_temperature_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd = dev_get_drvdata(dev);
	int value = 0;
	struct mdnie_lite_tun_type *tune = NULL;

	if (!vdd)
		return size;

	vdd = ss_check_hall_ic_get_vdd(vdd);
	tune = vdd->mdnie.mdnie_tune_state_dsi;

	sscanf(buf, "%d", &value);

	if (value < HMT_COLOR_TEMP_OFF || value >= HMT_COLOR_TEMP_MAX) {
		DPRINT("[ERROR] wrong color temperature value : %d\n", value);
		return size;
	}

	if (tune->mdnie_accessibility == NEGATIVE) {
		DPRINT("already negative mode(%d), do not update color temperature(%d)\n",
			tune->mdnie_accessibility, value);
		return size;
	}

	DPRINT("%s : (%d) -> (%d)\n", __func__, tune->hmt_color_temperature, value);
	tune->hmt_color_temperature = value;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static DEVICE_ATTR(mode, 0664, mode_show, mode_store);
static DEVICE_ATTR(scenario, 0664, scenario_show, scenario_store);
static DEVICE_ATTR(outdoor, 0664, outdoor_show, outdoor_store);
static DEVICE_ATTR(bypass, 0664, bypass_show, bypass_store);
static DEVICE_ATTR(accessibility, 0664, accessibility_show, accessibility_store);
static DEVICE_ATTR(sensorRGB, 0664, sensorRGB_show, sensorRGB_store);
static DEVICE_ATTR(whiteRGB, 0664, whiteRGB_show, whiteRGB_store);
static DEVICE_ATTR(mdnie_ldu, 0664, mdnie_ldu_show, mdnie_ldu_store);
static DEVICE_ATTR(night_mode, 0664, night_mode_show, night_mode_store);
static DEVICE_ATTR(color_lens, 0664, color_lens_show, color_lens_store);
static DEVICE_ATTR(hdr, 0664, hdr_show, hdr_store);
static DEVICE_ATTR(light_notification, 0664, light_notification_show, light_notification_store);
static DEVICE_ATTR(afc, 0664, afc_show, afc_store);
static DEVICE_ATTR(cabc, 0664, cabc_show, cabc_store);
static DEVICE_ATTR(hmt_color_temperature, 0664, hmt_color_temperature_show, hmt_color_temperature_store);

#define MDNIE_WOFS_ORG_PATH ("/efs/FactoryApp/mdnie")
static int mdnie_get_efs(char *filename, int *value)
{
	mm_segment_t old_fs;
	struct file *filp = NULL;
	int fsize = 0, nread, rc, ret = 0;
	u8 buf[128] = { 0, };

	if (!filename || !value) {
		pr_err("%s invalid parameter\n", __func__);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret == -ENOENT)
			pr_err("%s file(%s) not exist\n", __func__, filename);
		else
			pr_info("%s file(%s) open error(ret %d)\n",
					__func__, filename, ret);
		set_fs(old_fs);
		return -ENOENT;
	}

	if (filp->f_path.dentry && filp->f_path.dentry->d_inode)
		fsize = filp->f_path.dentry->d_inode->i_size;

	if (fsize == 0 || fsize >= ARRAY_SIZE(buf)) {
		pr_err("%s invalid file(%s) size %d\n",
				__func__, filename, fsize);
		ret = -EEXIST;
		goto exit;
	}

	memset(buf, 0, sizeof(buf));
	nread = vfs_read(filp, (char __user *)buf, fsize, &filp->f_pos);
	buf[nread] = '\0';

	if (nread != fsize) {
		pr_err("%s failed to read (ret %d)\n", __func__, nread);
		ret = -EIO;
		goto exit;
	}

	rc = sscanf(buf, "%d %d %d", &value[0], &value[1], &value[2]);
	if (rc != 3) {
		pr_err("%s failed to kstrtoint %d\n", __func__, rc);
		ret = -EINVAL;
		goto exit;
	}

	pr_info("%s %s(size %d) : %d %d %d\n",
			__func__, filename, fsize, value[0], value[1], value[2]);

exit:
	filp_close(filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int dpui_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct mdnie_lite_tun_type *tune = container_of(self,
			struct mdnie_lite_tun_type, dpui_notif);

	struct samsung_display_driver_data *vdd = tune->vdd;
	struct mdnie_lite_tune_data *mdnie_data = vdd->mdnie.mdnie_data;
	struct dpui_info *dpui = data;
	char tbuf[MAX_DPUI_VAL_LEN];
	int def_wrgb_ofs_org[3] = { 0, };
	int size;

	if (dpui == NULL) {
		DPRINT("err: dpui is null\n");
		return 0;
	}

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
			mdnie_data->dsi_white_balanced_r);
	set_dpui_field(DPUI_KEY_WOFS_R, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
			mdnie_data->dsi_white_balanced_g);
	set_dpui_field(DPUI_KEY_WOFS_G, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
			mdnie_data->dsi_white_balanced_b);
	set_dpui_field(DPUI_KEY_WOFS_B, tbuf, size);

	mdnie_get_efs(MDNIE_WOFS_ORG_PATH, def_wrgb_ofs_org);

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", def_wrgb_ofs_org[0]);
	set_dpui_field(DPUI_KEY_WOFS_R_ORG, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", def_wrgb_ofs_org[1]);
	set_dpui_field(DPUI_KEY_WOFS_G_ORG, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", def_wrgb_ofs_org[2]);
	set_dpui_field(DPUI_KEY_WOFS_B_ORG, tbuf, size);

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", vdd->mdnie.mdnie_x);
	set_dpui_field(DPUI_KEY_WCRD_X, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", vdd->mdnie.mdnie_y);
	set_dpui_field(DPUI_KEY_WCRD_Y, tbuf, size);

	return 0;
}

static int mdnie_register_dpui(struct mdnie_lite_tun_type *tune)
{
	memset(&tune->dpui_notif, 0,
			sizeof(tune->dpui_notif));
	tune->dpui_notif.notifier_call = dpui_notifier_callback;

	return dpui_logging_register(&tune->dpui_notif,
			DPUI_TYPE_PANEL);
}

void create_tcon_mdnie_node(struct samsung_display_driver_data *vdd)

{
	struct device *tune_mdnie_dev;
	char dirname[10];

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		sprintf(dirname, "mdnie");
	else
		sprintf(dirname, "mdnie%d", vdd->ndx);

	tune_mdnie_dev = device_create(mdnie_class, NULL, 0, vdd,  "%s", dirname);

	if (IS_ERR(tune_mdnie_dev))
		DPRINT("Failed to create device(mdnie)!\n");

	/* APP */
	if (device_create_file(tune_mdnie_dev, &dev_attr_scenario) < 0)
		DPRINT("Failed to create device file(%s)!\n", dev_attr_scenario.attr.name);

	/* MODE */
	if (device_create_file(tune_mdnie_dev, &dev_attr_mode) < 0)
		DPRINT("Failed to create device file(%s)!\n", dev_attr_mode.attr.name);

	/* OUTDOOR */
	if (device_create_file(tune_mdnie_dev, &dev_attr_outdoor) < 0)
		DPRINT("Failed to create device file(%s)!\n", dev_attr_outdoor.attr.name);

	/* MDNIE ON/OFF */
	if (device_create_file(tune_mdnie_dev, &dev_attr_bypass) < 0)
		DPRINT("Failed to create device file(%s)!\n", dev_attr_bypass.attr.name);

	/* COLOR BLIND */
	if (device_create_file(tune_mdnie_dev, &dev_attr_accessibility) < 0)
		DPRINT("Failed to create device file(%s)!=n", dev_attr_accessibility.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_sensorRGB) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_sensorRGB.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_whiteRGB) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_whiteRGB.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_mdnie_ldu) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_mdnie_ldu.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_night_mode) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_night_mode.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_color_lens) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_color_lens.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_hdr) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_hdr.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_light_notification) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_light_notification.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_afc) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_afc.attr.name);

	/* hmt_color_temperature */
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_hmt_color_temperature) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_hmt_color_temperature.attr.name);

	/* CABC ON/OFF */
	if (vdd->support_cabc)
		if (device_create_file(tune_mdnie_dev, &dev_attr_cabc) < 0)
			DPRINT("Failed to create device file(%s)!\n", dev_attr_cabc.attr.name);
}

struct mdnie_lite_tun_type *init_dsi_tcon_mdnie_class(struct samsung_display_driver_data *vdd)
{
	struct mdnie_lite_tun_type *tune;

	if (!vdd->mdnie.support_mdnie) {
		DPRINT("not support mdnie!\n");
			return NULL;
	}

	if (!mdnie_class) {
		mdnie_class = class_create(THIS_MODULE, "mdnie");
		if (IS_ERR(mdnie_class)) {
			DPRINT("Failed to create class(mdnie)!\n");
			return NULL;
		}

	}

	create_tcon_mdnie_node(vdd);

	tune =
		kzalloc(sizeof(struct mdnie_lite_tun_type), GFP_KERNEL);

	if (!tune) {
		DPRINT("%s allocation fail", __func__);
		return NULL;
	}

	tune->vdd = vdd;
	vdd->mdnie.mdnie_tune_state_dsi = tune;

	tune->mdnie_bypass = BYPASS_DISABLE;
	if (tune->vdd->support_cabc)
		tune->cabc_bypass = BYPASS_DISABLE;
	tune->hbm_enable = false;

	tune->mdnie_app = UI_APP;
	tune->mdnie_mode = AUTO_MODE;
	tune->outdoor = OUTDOOR_OFF_MODE;
	tune->hdr = HDR_OFF;
	tune->light_notification = LIGHT_NOTIFICATION_OFF;

	tune->mdnie_accessibility = ACCESSIBILITY_OFF;

	tune->scr_white_red = 0xff;
	tune->scr_white_green = 0xff;
	tune->scr_white_blue = 0xff;

	tune->night_mode_enable = false;
	tune->night_mode_index = 0;
	tune->ldu_mode_index = 0;

	tune->color_lens_enable = false;
	tune->color_lens_color = 0;
	tune->color_lens_level = 0;

	tune->afc_enable = 0;
	mdnie_register_dpui(tune);

	/* Set default link_stats as DSI_HS_MODE for mdnie tune data */
//	vdd_data->mdnie_tune_data[index].mdnie_tune_packet_tx_cmds_dsi.link_state = DSI_HS_MODE;

	return tune;
}


void coordinate_tunning_multi(struct samsung_display_driver_data *vdd,
    char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE], int mdnie_tune_index, int scr_wr_addr, int data_size)
{
	int i, j;
	struct dsi_cmd_desc *coordinate_tunning_data = NULL;
	struct mdnie_lite_tune_data *mdnie_data = vdd->mdnie.mdnie_data;


	for (i = 0; i < MAX_APP_MODE; i++) {
		for (j = 0; j < MAX_MODE; j++) {
			if ((mdnie_data->mdnie_tune_value_dsi[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
				coordinate_tunning_data = mdnie_data->mdnie_tune_value_dsi[i][j][0];
				coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][0];
				coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][2];
				coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][4];
			}
			if((i == UI_APP) && (j == AUTO_MODE)) {
				mdnie_data->dsi_white_default_r = coordinate_data_multi[j][mdnie_tune_index][0];
				mdnie_data->dsi_white_default_g = coordinate_data_multi[j][mdnie_tune_index][2];
				mdnie_data->dsi_white_default_b = coordinate_data_multi[j][mdnie_tune_index][4];
#if defined(CONFIG_SEC_FACTORY)
				coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] += mdnie_data->dsi_white_balanced_r;
				coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] += mdnie_data->dsi_white_balanced_g;
				coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] += mdnie_data->dsi_white_balanced_b;
#endif
			}
		}
	}

}


void coordinate_tunning_calculate(struct samsung_display_driver_data *vdd,
		int x, int y,
		char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE],
		int *rgb_index, int scr_wr_addr, int data_size)
{
	struct mdnie_lite_tune_data *mdnie_data = vdd->mdnie.mdnie_data;
	int i, j;
	int r, g, b;
	int r_00, r_01, r_10, r_11;
	int g_00, g_01, g_10, g_11;
	int b_00, b_01, b_10, b_11;
	struct dsi_cmd_desc *coordinate_tunning_data = NULL;
	struct dsi_cmd_desc *coordinate_outdoor_data = NULL;

	DPRINT("coordinate_tunning_calculate index_0 : %d, index_1 : %d, index_2 : %d, index_3 : %d, x : %d, y : %d\n",
			rgb_index[0], rgb_index[1], rgb_index[2], rgb_index[3], x, y);

	for (i = 0; i < MAX_APP_MODE; i++) {
		for (j = 0; j < MAX_MODE; j++) {
			if ((mdnie_data->mdnie_tune_value_dsi[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
				coordinate_tunning_data = mdnie_data->mdnie_tune_value_dsi[i][j][0];

				r_00 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[0]][0];
				r_01 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[1]][0];
				r_10 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[2]][0];
				r_11 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[3]][0];

				g_00 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[0]][2];
				g_01 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[1]][2];
				g_10 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[2]][2];
				g_11 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[3]][2];

				b_00 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[0]][4];
				b_01 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[1]][4];
				b_10 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[2]][4];
				b_11 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[3]][4];

				r = ((r_00 * (1024 - x) + r_10 * x) * (1024 - y) + (r_01 * (1024 - x) + r_11 * x) * y) + 524288;
				r = r >> 20;
				g = ((g_00 * (1024 - x) + g_10 * x) * (1024 - y) + (g_01 * (1024 - x) + g_11 * x) * y) + 524288;
				g = g >> 20;
				b = ((b_00 * (1024 - x) + b_10 * x) * (1024 - y) + (b_01 * (1024 - x) + b_11 * x) * y) + 524288;
				b = b >> 20;

				if (i == 0 && j == 4)
					DPRINT("coordinate_tunning_calculate_Adaptive r : %d, g : %d, b : %d\n", r, g, b);
				if (i == 0 && j == 2)
					DPRINT("coordinate_tunning_calculate_D65 r : %d, g : %d, b : %d\n", r, g, b);

				coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = (char)r;
				coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = (char)g;
				coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = (char)b;

				if ((i == UI_APP) && (j == AUTO_MODE)) {
					mdnie_data->dsi_white_default_r = (char)r;
					mdnie_data->dsi_white_default_g = (char)g;
					mdnie_data->dsi_white_default_b = (char)b;

					coordinate_outdoor_data = mdnie_data->DSI_HBM_CE_MDNIE;
					if(coordinate_outdoor_data != NULL) {
						coordinate_outdoor_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = (char)r;
						coordinate_outdoor_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = (char)g;
						coordinate_outdoor_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = (char)b;
					}
#if defined(CONFIG_SEC_FACTORY)
					coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] += mdnie_data->dsi_white_balanced_r;
					coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] += mdnie_data->dsi_white_balanced_g;
					coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] += mdnie_data->dsi_white_balanced_b;
#endif
				}
				if ((i == UI_APP) && (j == DYNAMIC_MODE)) {
					coordinate_outdoor_data = mdnie_data->DSI_HBM_CE_D65_MDNIE;
					if(coordinate_outdoor_data != NULL) {
						coordinate_outdoor_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = (char)r;
						coordinate_outdoor_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = (char)g;
						coordinate_outdoor_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = (char)b;
					}
				}
			}
		}
	}
}

void coordinate_tunning(struct samsung_display_driver_data *vdd,
		char *coordinate_data, int scr_wr_addr, int data_size)
{
	struct mdnie_lite_tune_data *mdnie_data = vdd->mdnie.mdnie_data;
	int i, j;
	char white_r, white_g, white_b;
	struct dsi_cmd_desc *coordinate_tunning_data = NULL;

	for (i = 0; i < MAX_APP_MODE; i++) {
		for (j = 0; j < MAX_MODE; j++) {
			if (mdnie_data->mdnie_tune_value_dsi[i][j][0] != NULL) {
				coordinate_tunning_data = mdnie_data->mdnie_tune_value_dsi[i][j][0];
				white_r = coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]];
				white_g = coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]];
				white_b = coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]];
				if ((white_r == 0xff) && (white_g == 0xff) && (white_b == 0xff)) {
					coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = coordinate_data[0];
					coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = coordinate_data[2];
					coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = coordinate_data[4];
				}
				if ((i == UI_APP) && (j == AUTO_MODE)) {
					mdnie_data->dsi_white_default_r = coordinate_data[0];
					mdnie_data->dsi_white_default_g = coordinate_data[2];
					mdnie_data->dsi_white_default_b = coordinate_data[4];
#if defined(CONFIG_SEC_FACTORY)
					coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] += mdnie_data->dsi_white_balanced_r;
					coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] += mdnie_data->dsi_white_balanced_g;
					coordinate_tunning_data[mdnie_data->dsi_scr_step_index].msg.tx_buf[mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] += mdnie_data->dsi_white_balanced_b;
#endif
				}
			}
		}
	}
}
