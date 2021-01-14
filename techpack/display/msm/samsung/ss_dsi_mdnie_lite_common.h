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

#ifndef _SAMSUNG_DSI_TCON_MDNIE_LIGHT_H_
#define _SAMSUNG_DSI_TCON_MDNIE_LIGHT_H_

#include "ss_dsi_panel_common.h"

#define NATURAL_MODE_ENABLE

#define NAME_STRING_MAX 30
#define MDNIE_COLOR_BLINDE_CMD_SIZE 18
#define MDNIE_COLOR_BLINDE_HBM_CMD_SIZE 24
#define COORDINATE_DATA_SIZE 6
#define MDNIE_SCR_CMD_SIZE 24
#define AFC_ROI_CMD_SIZE 12

extern char mdnie_app_name[][NAME_STRING_MAX];
extern char mdnie_mode_name[][NAME_STRING_MAX];
extern char outdoor_name[][NAME_STRING_MAX];
extern struct mdnie_lite_tune_data mdnie_data;

#define APP_ID_TDMB (20)	/* for fake_id() */

enum BYPASS {
	BYPASS_DISABLE = 0,
	BYPASS_ENABLE,
};

enum APP {
	UI_APP = 0,
	VIDEO_APP,
	VIDEO_WARM_APP,
	VIDEO_COLD_APP,
	CAMERA_APP,
	NAVI_APP,
	GALLERY_APP,
	VT_APP,
	BROWSER_APP,
	eBOOK_APP,
	EMAIL_APP,
	GAME_LOW_APP,
	GAME_MID_APP,
	GAME_HIGH_APP,
	VIDEO_ENHANCER,
	VIDEO_ENHANCER_THIRD,
	TDMB_APP,	/* is linked to APP_ID_TDMB */
	MAX_APP_MODE,
};

enum MODE {
	DYNAMIC_MODE = 0,
	STANDARD_MODE,
#if defined(NATURAL_MODE_ENABLE)
	NATURAL_MODE,
#endif
	MOVIE_MODE,
	AUTO_MODE,
	READING_MODE,
	MAX_MODE,
};

enum OUTDOOR {
	OUTDOOR_OFF_MODE = 0,
	OUTDOOR_ON_MODE,
	MAX_OUTDOOR_MODE,
};

enum ACCESSIBILITY {
	ACCESSIBILITY_OFF = 0,
	NEGATIVE,
	COLOR_BLIND,
	CURTAIN,
	GRAYSCALE,
	GRAYSCALE_NEGATIVE,
	COLOR_BLIND_HBM,
	ACCESSIBILITY_MAX,
};

enum HMT_GRAY {
	HMT_GRAY_OFF = 0,
	HMT_GRAY_1,
	HMT_GRAY_2,
	HMT_GRAY_3,
	HMT_GRAY_4,
	HMT_GRAY_5,
	HMT_GRAY_MAX,
};

enum HMT_COLOR_TEMPERATURE {
	HMT_COLOR_TEMP_OFF = 0,
	HMT_COLOR_TEMP_3000K, /* 3000K */
	HMT_COLOR_TEMP_4000K, /* 4000K */
	HMT_COLOR_TEMP_5000K, /* 5000K */
	HMT_COLOR_TEMP_6500K, /* 6500K */
	HMT_COLOR_TEMP_7500K, /* 7500K */
	HMT_COLOR_TEMP_MAX
};

enum HDR {
	HDR_OFF = 0,
	HDR_1,
	HDR_2,
	HDR_3,
	HDR_4,
	HDR_5,
	HDR_MAX
};

enum LIGHT_NOTIFICATION {
	LIGHT_NOTIFICATION_OFF = 0,
	LIGHT_NOTIFICATION_ON,
	LIGHT_NOTIFICATION_MAX,
};

enum COLOR_LENS {
	COLOR_LENS_OFF = 0,
	COLOR_LENS_ON,
	COLOR_LENS_MAX
};

enum COLOR_LENS_COLOR {
	COLOR_LENS_COLOR_BLUE = 0,
	COLOR_LENS_COLOR_AZURE,
	COLOR_LENS_COLOR_CYAN,
	COLOR_LENS_COLOR_SPRING_GREEN,
	COLOR_LENS_COLOR_GREEN,
	COLOR_LENS_COLOR_CHARTREUSE_GREEN,
	COLOR_LENS_COLOR_YELLOW,
	COLOR_LENS_COLOR_ORANGE,
	COLOR_LENS_COLOR_RED,
	COLOR_LENS_COLOR_ROSE,
	COLOR_LENS_COLOR_MAGENTA,
	COLOR_LENS_COLOR_VIOLET,
	COLOR_LENS_COLOR_MAX
};

enum COLOR_LENS_LEVEL {
	COLOR_LENS_LEVEL_20P = 0,
	COLOR_LENS_LEVEL_25P,
	COLOR_LENS_LEVEL_30P,
	COLOR_LENS_LEVEL_35P,
	COLOR_LENS_LEVEL_40P,
	COLOR_LENS_LEVEL_45P,
	COLOR_LENS_LEVEL_50P,
	COLOR_LENS_LEVEL_55P,
	COLOR_LENS_LEVEL_60P,
	COLOR_LENS_LEVEL_MAX,
};

struct mdnie_lite_tun_type {
	enum BYPASS mdnie_bypass;
	enum BYPASS cabc_bypass;
	int hbm_enable;

	enum APP mdnie_app;
	enum MODE mdnie_mode;
	enum OUTDOOR outdoor;
	enum ACCESSIBILITY mdnie_accessibility;
	enum HMT_COLOR_TEMPERATURE hmt_color_temperature;
	enum HDR hdr;
	enum LIGHT_NOTIFICATION light_notification;

	char scr_white_red;
	char scr_white_green;
	char scr_white_blue;

	int night_mode_enable;
	int night_mode_index;
	int ldu_mode_index;

	int color_lens_enable;
	int color_lens_color;
	int color_lens_level;

	int afc_enable;
	char afc_roi[AFC_ROI_CMD_SIZE];

	struct samsung_display_driver_data *vdd;
	struct notifier_block dpui_notif;
};

int config_cabc(struct samsung_display_driver_data *vdd, int value);

enum {
	MDNIE_STEP1 = 0,
	MDNIE_STEP2,
	MDNIE_STEP3,
	MDNIE_STEP_MAX,
};

enum {
	ADDRESS_SCR_WHITE_RED_OFFSET = 0,
	ADDRESS_SCR_WHITE_GREEN_OFFSET,
	ADDRESS_SCR_WHITE_BLUE_OFFSET,
	ADDRESS_SCR_WHITE_MAX,
};

/* COMMON DATA THAT POINT TO MDNIE TUNING DATA */
struct mdnie_lite_tune_data {
	char *DSI_COLOR_BLIND_MDNIE_1;
	char *DSI_COLOR_BLIND_MDNIE_2;
	char *DSI_COLOR_BLIND_MDNIE_SCR;
	char *DSI_RGB_SENSOR_MDNIE_1;
	char *DSI_RGB_SENSOR_MDNIE_2;
	char *DSI_RGB_SENSOR_MDNIE_3;
	char *DSI_RGB_SENSOR_MDNIE_SCR;
	char *DSI_TRANS_DIMMING_MDNIE;
	char *DSI_UI_DYNAMIC_MDNIE_2;
	char *DSI_UI_STANDARD_MDNIE_2;
	char *DSI_UI_AUTO_MDNIE_2;
	char *DSI_VIDEO_DYNAMIC_MDNIE_2;
	char *DSI_VIDEO_STANDARD_MDNIE_2;
	char *DSI_VIDEO_AUTO_MDNIE_2;
	char *DSI_CAMERA_MDNIE_2;
	char *DSI_CAMERA_AUTO_MDNIE_2;
	char *DSI_GALLERY_DYNAMIC_MDNIE_2;
	char *DSI_GALLERY_STANDARD_MDNIE_2;
	char *DSI_GALLERY_AUTO_MDNIE_2;
	char *DSI_VT_DYNAMIC_MDNIE_2;
	char *DSI_VT_STANDARD_MDNIE_2;
	char *DSI_VT_AUTO_MDNIE_2;
	char *DSI_BROWSER_DYNAMIC_MDNIE_2;
	char *DSI_BROWSER_STANDARD_MDNIE_2;
	char *DSI_BROWSER_AUTO_MDNIE_2;
	char *DSI_EBOOK_DYNAMIC_MDNIE_2;
	char *DSI_EBOOK_STANDARD_MDNIE_2;
	char *DSI_EBOOK_AUTO_MDNIE_2;
	char *DSI_TDMB_DYNAMIC_MDNIE_2;
	char *DSI_TDMB_STANDARD_MDNIE_2;
	char *DSI_TDMB_AUTO_MDNIE_2;
	char *DSI_NIGHT_MODE_MDNIE_1;
	char *DSI_NIGHT_MODE_MDNIE_2;
	char *DSI_NIGHT_MODE_MDNIE_SCR;
	char *DSI_COLOR_LENS_MDNIE_1;
	char *DSI_COLOR_LENS_MDNIE_2;
	char *DSI_COLOR_LENS_MDNIE_SCR;
	char *DSI_AFC;
	char *DSI_AFC_ON;
	char *DSI_AFC_OFF;

	struct dsi_cmd_desc *DSI_BYPASS_MDNIE;
	struct dsi_cmd_desc *DSI_NEGATIVE_MDNIE;
	struct dsi_cmd_desc *DSI_COLOR_BLIND_MDNIE;
	struct dsi_cmd_desc *DSI_HBM_CE_MDNIE;
	struct dsi_cmd_desc *DSI_HBM_CE_D65_MDNIE;
	struct dsi_cmd_desc *DSI_HBM_CE_TEXT_MDNIE;
	struct dsi_cmd_desc *DSI_RGB_SENSOR_MDNIE;
	struct dsi_cmd_desc *DSI_CURTAIN;
	struct dsi_cmd_desc *DSI_GRAYSCALE_MDNIE;
	struct dsi_cmd_desc *DSI_GRAYSCALE_NEGATIVE_MDNIE;
	struct dsi_cmd_desc *DSI_UI_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI_UI_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI_UI_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI_UI_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI_UI_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_WARM_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_WARM_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_COLD_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI_VIDEO_COLD_MDNIE;
	struct dsi_cmd_desc *DSI_CAMERA_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI_CAMERA_MDNIE;
	struct dsi_cmd_desc *DSI_CAMERA_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI_GALLERY_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI_GALLERY_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI_GALLERY_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI_GALLERY_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI_GALLERY_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI_VT_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI_VT_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI_VT_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI_VT_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI_VT_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI_BROWSER_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI_BROWSER_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI_BROWSER_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI_BROWSER_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI_BROWSER_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI_EBOOK_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI_EBOOK_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI_EBOOK_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI_EBOOK_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI_EBOOK_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI_EMAIL_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI_GAME_LOW_MDNIE;
	struct dsi_cmd_desc *DSI_GAME_MID_MDNIE;
	struct dsi_cmd_desc *DSI_GAME_HIGH_MDNIE;
	struct dsi_cmd_desc *DSI_TDMB_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI_TDMB_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI_TDMB_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI_TDMB_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI_TDMB_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI_NIGHT_MODE_MDNIE;
	struct dsi_cmd_desc *DSI_COLOR_LENS_MDNIE;

	struct dsi_cmd_desc *(*mdnie_tune_value_dsi)[MAX_MODE][MAX_OUTDOOR_MODE];
	struct dsi_cmd_desc **hmt_color_temperature_tune_value_dsi;
	struct dsi_cmd_desc **hdr_tune_value_dsi;
	struct dsi_cmd_desc **light_notification_tune_value_dsi;

	int dsi_bypass_mdnie_size;
	int mdnie_color_blinde_cmd_offset;
	int mdnie_step_index[MDNIE_STEP_MAX];
	int address_scr_white[ADDRESS_SCR_WHITE_MAX];
	int dsi_rgb_sensor_mdnie_1_size;
	int dsi_rgb_sensor_mdnie_2_size;
	int dsi_rgb_sensor_mdnie_3_size;
	int dsi_trans_dimming_data_index;
	char **dsi_adjust_ldu_table;
	int dsi_max_adjust_ldu;
	char *dsi_night_mode_table;
	int dsi_max_night_mode_index;
	char *dsi_color_lens_table;
	int dsi_scr_step_index;
	char dsi_white_default_r;
	char dsi_white_default_g;
	char dsi_white_default_b;
	char dsi_white_ldu_r;
	char dsi_white_ldu_g;
	char dsi_white_ldu_b;
	int dsi_white_balanced_r;
	int dsi_white_balanced_g;
	int dsi_white_balanced_b;
	int dsi_afc_size;
	int dsi_afc_index;
};

/* COMMON FUNCTION*/
struct mdnie_lite_tun_type *init_dsi_tcon_mdnie_class(struct samsung_display_driver_data *vdd);
int update_dsi_tcon_mdnie_register(struct samsung_display_driver_data *vdd);
void coordinate_tunning(struct samsung_display_driver_data *vdd,
		char *coordinate_data, int scr_wr_addr, int data_size);
void coordinate_tunning_calculate(struct samsung_display_driver_data *vdd,
		int x, int y,
		char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE],
		int *rgb_index, int scr_wr_addr, int data_size);


void coordinate_tunning_multi(struct samsung_display_driver_data *vdd,
    char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE], int mdnie_tune_index, int scr_wr_addr, int data_size);
    
/* COMMON FUNCTION END*/

#endif /*_DSI_TCON_MDNIE_H_*/
