/* Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bsearch.h>
#include <linux/dev_ril_bridge.h>
#include "cam_sensor_mipi.h"
#include "cam_sensor_adaptive_mipi_imx374.h"
#include "cam_sensor_adaptive_mipi_imx516.h"
#include "cam_sensor_adaptive_mipi_imx555.h"
#include "cam_sensor_adaptive_mipi_imx586.h"
#include "cam_sensor_adaptive_mipi_s5k2la.h"
#include "cam_sensor_adaptive_mipi_s5kgw2.h"
#include "cam_sensor_adaptive_mipi_s5kgh1.h"
#include "cam_sensor_adaptive_mipi_s5k2l3.h"
#include "cam_sensor_adaptive_mipi_s5khm1.h"
#include "cam_sensor_adaptive_mipi_s5k3m5.h"
#include "cam_sensor_dev.h"

static struct cam_cp_noti_info g_cp_noti_info;
static struct mutex g_mipi_mutex;
static bool g_init_notifier;
extern char mipi_string[20];
#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
extern char tof_freq[10];
#endif
char band_info[20] = "\n";

/* CP notity format (HEX raw format)
 * 10 00 AA BB 27 01 03 XX YY YY YY YY ZZ ZZ ZZ ZZ
 *
 * 00 10 (0x0010) - len
 * AA BB - not used
 * 27 - MAIN CMD (SYSTEM CMD : 0x27)
 * 01 - SUB CMD (CP Channel Info : 0x01)
 * 03 - NOTI CMD (0x03)
 * XX - RAT MODE
 * YY YY YY YY - BAND MODE
 * ZZ ZZ ZZ ZZ - FREQ INFO
 */

static int cam_mipi_ril_notifier(struct notifier_block *nb,
				unsigned long size, void *buf)
{
	struct dev_ril_bridge_msg *msg;
	struct cam_cp_noti_info *cp_noti_info;

	if (!g_init_notifier) {
		CAM_ERR(CAM_SENSOR, "[adaptive_mipi] not init ril notifier");
		return NOTIFY_DONE;
	}

	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] ril notification size [%ld]", size);

	msg = (struct dev_ril_bridge_msg *)buf;
	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] dev_id : %d, data_len : %d",
		msg->dev_id, msg->data_len);

	if (msg->dev_id == IPC_SYSTEM_CP_CHANNEL_INFO
				&& msg->data_len == sizeof(struct cam_cp_noti_info)) {
	   cp_noti_info = (struct cam_cp_noti_info *)msg->data;
	   mutex_lock(&g_mipi_mutex);
	   memcpy(&g_cp_noti_info, cp_noti_info, sizeof(struct cam_cp_noti_info));
	   mutex_unlock(&g_mipi_mutex);

	   CAM_INFO(CAM_SENSOR, "[adaptive_mipi] update mipi channel [%d,%d,%d]",
		   g_cp_noti_info.rat, g_cp_noti_info.band, g_cp_noti_info.channel);
	   return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}

static struct notifier_block g_ril_notifier_block = {
	.notifier_call = cam_mipi_ril_notifier,
};

void cam_mipi_register_ril_notifier(void)
{
	if (!g_init_notifier) {
		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] register ril notifier");

		mutex_init(&g_mipi_mutex);
		memset(&g_cp_noti_info, 0, sizeof(struct cam_cp_noti_info));

		register_dev_ril_bridge_event_notifier(&g_ril_notifier_block);
		g_init_notifier = true;
	}
}

static void cam_mipi_get_rf_channel(struct cam_cp_noti_info *ch)
{
	if (!g_init_notifier) {
		CAM_ERR(CAM_SENSOR, "[adaptive_mipi] not init ril notifier");
		memset(ch, 0, sizeof(struct cam_cp_noti_info));
		return;
	}

	mutex_lock(&g_mipi_mutex);
	memcpy(ch, &g_cp_noti_info, sizeof(struct cam_cp_noti_info));
	mutex_unlock(&g_mipi_mutex);
}

static int compare_rf_channel(const void *key, const void *element)
{
	struct cam_mipi_channel *k = ((struct cam_mipi_channel *)key);
	struct cam_mipi_channel *e = ((struct cam_mipi_channel *)element);

	if (k->rat_band < e->rat_band)
		return -1;
	else if (k->rat_band > e->rat_band)
		return 1;

	if (k->channel_max < e->channel_min)
		return -1;
	else if (k->channel_min > e->channel_max)
		return 1;

	return 0;
}

int cam_mipi_select_mipi_by_rf_channel(const struct cam_mipi_channel *channel_list, const int size)
{
	struct cam_mipi_channel *result = NULL;
	struct cam_mipi_channel key;
	struct cam_cp_noti_info input_ch;

	cam_mipi_get_rf_channel(&input_ch);

#if 1 //adaptive mipi test <[0]rat, [1~3]band, [4~8]channel>
	if (band_info != NULL) {
		uint32_t value = 0;
		char temp[10] = "\n";
		memcpy(temp, &band_info[0], 1);
		kstrtouint(temp, 10, &value);
		if (value > 0) {
			input_ch.rat = value;

			memset(temp, 0, sizeof(temp));
			memcpy(temp, &band_info[1], 3);
			kstrtouint(temp, 10, &value);
			input_ch.band = value;

			memset(temp, 0, sizeof(temp));
			memcpy(temp, &band_info[4], 5);
			kstrtouint(temp, 10, &value);
			input_ch.channel = value;
		}
	}
#endif

	key.rat_band = CAM_RAT_BAND(input_ch.rat, input_ch.band);
	key.channel_min = input_ch.channel;
	key.channel_max = input_ch.channel;

	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] searching rf channel s [%d,%d,%d]",
		input_ch.rat, input_ch.band, input_ch.channel);

	result = bsearch(&key,
			channel_list,
			size,
			sizeof(struct cam_mipi_channel),
			compare_rf_channel);

	if (result == NULL) {
		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] searching result : not found");
		return -1;
	}

	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] searching result : [0x%x,(%d-%d)]->(%d)",
		result->rat_band, result->channel_min, result->channel_max, result->setting_index);

	return result->setting_index;
}

void cam_mipi_init_setting(struct cam_sensor_ctrl_t *s_ctrl)
{
	const struct cam_mipi_sensor_mode *cur_mipi_sensor_mode;

#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
	extern long rear_frs_test_mode;
	
	if (rear_frs_test_mode == 0) {
#endif
	if (s_ctrl->sensordata->slave_info.sensor_id == FRONT_SENSOR_ID_IMX374) {
		s_ctrl->mipi_info = sensor_imx374_setfile_A_mipi_sensor_mode;
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else if (s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX516
		|| s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX518) {
		s_ctrl->mipi_info = sensor_imx516_rear_setfile_tx_sensor_mode;
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else if (s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_IMX555) {
		s_ctrl->mipi_info = sensor_imx555_setfile_tx_sensor_all_mode;
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else if (s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_IMX586) {
		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] sensor_mode : %d", s_ctrl->sensor_mode);
		if (s_ctrl->sensor_mode == 0) {
			s_ctrl->mipi_info = sensor_imx586_setfile_A_mipi_sensor_mode;
		} else if (s_ctrl->sensor_mode == 1) {
			s_ctrl->mipi_info = sensor_imx586_setfile_B_mipi_sensor_mode;
		} else {
			s_ctrl->mipi_info = sensor_imx586_setfile_A_mipi_sensor_mode;
		}
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else if (s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_S5K2LA) {
		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] sensor_mode : %d", s_ctrl->sensor_mode);
		if (s_ctrl->sensor_mode == 0) {
			s_ctrl->mipi_info = sensor_2la_setfile_tx_sensor_full_mode;
		} else if (s_ctrl->sensor_mode == 1) {
			s_ctrl->mipi_info = sensor_2la_setfile_tx_sensor_binning_mode;
		} else {
			s_ctrl->mipi_info = sensor_2la_setfile_tx_sensor_full_mode;
		}
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else if (s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_S5KGW2) {
		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] sensor_mode : %d", s_ctrl->sensor_mode);
		if (s_ctrl->sensor_mode == 0) {
			s_ctrl->mipi_info = sensor_s5kgw2_setfile_A_mipi_sensor_mode;
		} else if (s_ctrl->sensor_mode == 1) {
			s_ctrl->mipi_info = sensor_s5kgw2_setfile_B_mipi_sensor_mode;
		} else if (s_ctrl->sensor_mode == 2) {
			s_ctrl->mipi_info = sensor_s5kgw2_setfile_C_mipi_sensor_mode;
		} else {
			s_ctrl->mipi_info = sensor_s5kgw2_setfile_A_mipi_sensor_mode;
		}
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else if (s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_S5KGH1) {
		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] sensor_mode : %d", s_ctrl->sensor_mode);
		if (s_ctrl->sensor_mode == 0) {
			s_ctrl->mipi_info = sensor_s5kgh1_setfile_A_mipi_sensor_mode;
		} else if (s_ctrl->sensor_mode == 1) {
			s_ctrl->mipi_info = sensor_s5kgh1_setfile_B_mipi_sensor_mode;
		} else if (s_ctrl->sensor_mode == 2) {
			s_ctrl->mipi_info = sensor_s5kgh1_setfile_C_mipi_sensor_mode;
		} else if (s_ctrl->sensor_mode == 3) {
			s_ctrl->mipi_info = sensor_s5kgh1_setfile_D_mipi_sensor_mode;
		} else {
			s_ctrl->mipi_info = sensor_s5kgh1_setfile_A_mipi_sensor_mode;
		}
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else if (s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_S5KHM1) {
		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] sensor_mode : %d", s_ctrl->sensor_mode);
		if (s_ctrl->sensor_mode == 0) {
			s_ctrl->mipi_info = sensor_s5khm1_setfile_A_mipi_sensor_mode;
		} else if (s_ctrl->sensor_mode == 1) {
			s_ctrl->mipi_info = sensor_s5khm1_setfile_B_mipi_sensor_mode;
		} else {
			s_ctrl->mipi_info = sensor_s5khm1_setfile_A_mipi_sensor_mode;
		}
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else if (s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_S5K2L3) {
		s_ctrl->mipi_info = sensor_s5k2l3_setfile_A_mipi_sensor_mode;
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else if (s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_S5K3M5) {
		s_ctrl->mipi_info = sensor_3m5_setfile_tx_sensor_full_mode;
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
	} else {
		CAM_ERR(CAM_SENSOR, "[adaptive_mipi] Not support adaptive mipi : 0x%x", s_ctrl->sensordata->slave_info.sensor_id);
	}
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
	}
#endif

	s_ctrl->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
	s_ctrl->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;
}

void cam_mipi_update_info(struct cam_sensor_ctrl_t *s_ctrl)
{
	const struct cam_mipi_sensor_mode *cur_mipi_sensor_mode;
	int found = -1;

	cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);

	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] cur rat : %d", cur_mipi_sensor_mode->mipi_channel->rat_band);
	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] cur channel_min : %d", cur_mipi_sensor_mode->mipi_channel->channel_min);
	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] cur channel_max : %d", cur_mipi_sensor_mode->mipi_channel->channel_max);
	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] cur setting_index : %d", cur_mipi_sensor_mode->mipi_channel->setting_index);

	found = cam_mipi_select_mipi_by_rf_channel(cur_mipi_sensor_mode->mipi_channel,
				cur_mipi_sensor_mode->mipi_channel_size);
	if (found != -1) {
		if (found < cur_mipi_sensor_mode->sensor_setting_size) {
			s_ctrl->mipi_clock_index_new = found;

			CAM_INFO(CAM_SENSOR, "[adaptive_mipi] mipi_clock_index_new : %d",
				s_ctrl->mipi_clock_index_new);
		} else {
			CAM_ERR(CAM_SENSOR, "sensor setting size is out of bound");
		}
	}
#if defined(CONFIG_MACH_X1Q_USA_SINGLE)
	else if (s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_IMX555) {
		CAM_INFO(CAM_SENSOR, "Change imx555 default adaptive mipi value for x1 mmw project");
		s_ctrl->mipi_clock_index_new = 1;
	}
#endif
	else {
		CAM_INFO(CAM_SENSOR, "not found rf channel, use default mipi clock");
		s_ctrl->mipi_clock_index_new = 0;
	}

#if defined(CONFIG_MACH_Y2Q_KOR_SINGLE)
	if (s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX516 ||
		s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX518) {
		s_ctrl->mipi_clock_index_new = 2;
	}
#endif

#if defined(CONFIG_SEC_FACTORY)
	s_ctrl->mipi_clock_index_new = 0;//only for factory
#endif
}

void cam_mipi_get_clock_string(struct cam_sensor_ctrl_t *s_ctrl)
{
	const struct cam_mipi_sensor_mode *cur_mipi_sensor_mode;

	cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);

	sprintf(mipi_string, "%s",
		cur_mipi_sensor_mode->mipi_setting[s_ctrl->mipi_clock_index_new].str_mipi_clk);

	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] cam_mipi_get_clock_string : %d", s_ctrl->mipi_clock_index_new);
	CAM_INFO(CAM_SENSOR, "[adaptive_mipi] mipi_string : %s", mipi_string);

#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
	if (s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX516
		|| s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX518) {
		if (strcmp(mipi_string, "96_19 VGA Mhz") == 0) {
			scnprintf(tof_freq, sizeof(tof_freq), "96");
		} else if (strcmp(mipi_string, "101_20 VGA Mhz") == 0) {
			scnprintf(tof_freq, sizeof(tof_freq), "101");
		} else if (strcmp(mipi_string, "100_20 VGA Mhz") == 0) {
			scnprintf(tof_freq, sizeof(tof_freq), "100");
		} else {
			scnprintf(tof_freq, sizeof(tof_freq), "100");
		}
#if defined(CONFIG_MACH_Y2Q_KOR_SINGLE)
		scnprintf(tof_freq, sizeof(tof_freq), "101");
#endif

#if defined(CONFIG_SEC_FACTORY)
		scnprintf(tof_freq, sizeof(tof_freq), "100");//only for factory
#endif
		CAM_INFO(CAM_SENSOR, "[TOF_FREQ_DBG] tof_freq : %s", tof_freq);
	}
#endif
}
#endif
