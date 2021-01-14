// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <linux/crc32.h>
#include <media/cam_sensor.h>

#include "cam_eeprom_core.h"
#include "cam_eeprom_soc.h"
#include "cam_debug_util.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"
#include <linux/ctype.h>

#define CAM_EEPROM_DBG  1
#define MAX_READ_SIZE  0x7FFFF

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

char rear_cam_cal_check[SYSFS_FW_VER_SIZE] = "NULL";
char front_cam_cal_check[SYSFS_FW_VER_SIZE] = "NULL";

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
char front2_cam_cal_check[SYSFS_FW_VER_SIZE] = "NULL";
#endif

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
char front3_cam_cal_check[SYSFS_FW_VER_SIZE] = "NULL";
#else
char front2_cam_cal_check[SYSFS_FW_VER_SIZE] = "NULL";
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
char rear2_cam_cal_check[SYSFS_FW_VER_SIZE] = "NULL";
#endif

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
char rear3_cam_cal_check[SYSFS_FW_VER_SIZE] = "NULL";
#endif

#if defined(CONFIG_SAMSUNG_REAR_TOF)
char rear_tof_cam_cal_check[SYSFS_FW_VER_SIZE] = "NULL";
#endif

#if defined(CONFIG_SAMSUNG_FRONT_TOF)
char front_tof_cam_cal_check[SYSFS_FW_VER_SIZE] = "NULL";
#endif

#if defined(CONFIG_SAMSUNG_REAR_BOKEH)
char bokeh_module_fw_ver[FROM_MODULE_FW_INFO_SIZE+1];
#endif

#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
char rear3_module_fw_ver[FROM_MODULE_FW_INFO_SIZE+1];
#endif

static unsigned int system_rev __read_mostly;
static int __init sec_hw_rev_setup(char *p)
{
	int ret;

	ret = kstrtouint(p, 0, &system_rev);

	if (unlikely(ret < 0)) {

		pr_warn("androidboot.revision is malformed (%s)\n", p);

		return -EINVAL;
	}

	pr_info("androidboot.revision %x\n", system_rev);

	return 0;
}

early_param("androidboot.revision", sec_hw_rev_setup);

static unsigned int sec_hw_rev(void)
{
	return system_rev;
}

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
extern uint8_t ois_wide_xygg[OIS_XYGG_SIZE];
extern uint8_t ois_wide_cal_mark;
uint8_t ois_wide_xysr[OIS_XYSR_SIZE] = { 0, };
extern int ois_gain_rear_result;
extern int ois_sr_rear_result;
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
extern uint8_t ois_wide_center_shift[OIS_CENTER_SHIFT_SIZE];
extern uint8_t ois_tele_xygg[OIS_XYGG_SIZE];
#if defined(CONFIG_SEC_R8Q_PROJECT)
#define M2_YGG_LMT        0x3F733333        //0.95f    Limit Value
#endif
extern uint8_t ois_tele_center_shift[OIS_CENTER_SHIFT_SIZE];
extern uint8_t ois_tele_cal_mark;
uint8_t ois_tele_xysr[OIS_XYSR_SIZE] = { 0, };
uint8_t ois_tele_cross_talk[OIS_CROSSTALK_SIZE] = { 0, };
extern int ois_tele_cross_talk_result;
extern int ois_gain_rear3_result;
extern int ois_sr_rear3_result;
#endif
#endif

uint32_t CAMERA_NORMAL_CAL_CRC;

ConfigInfo_t ConfigInfo[MAX_CONFIG_INFO_IDX];

char M_HW_INFO[HW_INFO_MAX_SIZE] = "";
char M_SW_INFO[SW_INFO_MAX_SIZE] = "";
char M_VENDOR_INFO[VENDOR_INFO_MAX_SIZE] = "";
char M_PROCESS_INFO[PROCESS_INFO_MAX_SIZE] = "";

char S_HW_INFO[HW_INFO_MAX_SIZE] = "";
char S_SW_INFO[SW_INFO_MAX_SIZE] = "";
char S_VENDOR_INFO[VENDOR_INFO_MAX_SIZE] = "";
char S_PROCESS_INFO[PROCESS_INFO_MAX_SIZE] = "";

uint8_t CriterionRev;
uint8_t ModuleVerOnPVR;
uint8_t ModuleVerOnSRA;
uint8_t minCalMapVer;

#ifdef CAM_EEPROM_DBG_DUMP
static int cam_eeprom_dump(uint32_t subdev_id, uint8_t *mapdata, uint32_t addr, uint32_t size)
{
	int rc = 0;
	int j;

	if (mapdata == NULL) {
		CAM_ERR(CAM_EEPROM, "mapdata is NULL");
		return -1;
	}
	if (size == 0) {
		CAM_ERR(CAM_EEPROM, "size is 0");
		return -1;
	}

	CAM_INFO(CAM_EEPROM, "subdev_id: %d, eeprom dump addr = 0x%04X, total read size = %d", subdev_id, addr, size);
	for (j = 0; j < size; j++)
		CAM_INFO(CAM_EEPROM, "addr = 0x%04X, data = 0x%02X", addr+j, mapdata[addr+j]);

	return rc;
}
#endif

static int isValidIdx(eConfigNameInfoIdx confIdx, uint32_t *ConfAddr)
{
	if(confIdx >= MAX_CONFIG_INFO_IDX)
	{
		CAM_ERR(CAM_EEPROM, "invalid index: %d, max:%d", confIdx, MAX_CONFIG_INFO_IDX);
		return 0;
	}

	if(ConfigInfo[confIdx].isSet == 1)
	{
		*ConfAddr = ConfigInfo[confIdx].value;
		CAM_INFO(CAM_EEPROM, "%s: %d, isSet: %d, addr: 0x%08X",
			ConfigInfoStrs[confIdx], confIdx, ConfigInfo[confIdx].isSet,
			ConfigInfo[confIdx].value);

		return 1;
	}
	else
	{
		*ConfAddr = 0;
		CAM_ERR(CAM_EEPROM, "%s: %d, isSet: %d",
			ConfigInfoStrs[confIdx], confIdx, ConfigInfo[confIdx].isSet);

		return 0;
	}
}

static int cam_eeprom_module_info_set_sensor_id(ModuleInfo_t *mInfo, eConfigNameInfoIdx idx, uint8_t *pMapData)
{
	char 	*sensorId = "";

	if((mInfo == NULL || mInfo->mVer.sensor_id == NULL || mInfo->mVer.sensor2_id == NULL)
		|| (idx != ADDR_M_SENSOR_ID && idx != ADDR_S_SENSOR_ID))
	{
		CAM_ERR(CAM_EEPROM, "sensor_id is NULL");
		return 1;
	}

	if(idx == ADDR_S_SENSOR_ID)
		sensorId = mInfo->mVer.sensor2_id;
	else
		sensorId = mInfo->mVer.sensor_id;

	memcpy(sensorId, pMapData, FROM_SENSOR_ID_SIZE);
	sensorId[FROM_SENSOR_ID_SIZE] = '\0';

	CAM_DBG(CAM_EEPROM,
		"%s sensor_id = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		mInfo->typeStr,
		sensorId[0], sensorId[1], sensorId[2], sensorId[3],
		sensorId[4], sensorId[5], sensorId[6], sensorId[7],
		sensorId[8], sensorId[9], sensorId[10], sensorId[11],
		sensorId[12], sensorId[13], sensorId[14], sensorId[15]);

	return 0;
}

static int cam_eeprom_module_info_set_module_id(ModuleInfo_t *mInfo, uint8_t *pMapData)
{
	char 	*moduleId = "";

	if(mInfo == NULL || mInfo->mVer.module_id == NULL)
	{
		CAM_ERR(CAM_EEPROM, "module_id is NULL");
		return 1;
	}
	moduleId = mInfo->mVer.module_id;

	memcpy(moduleId, pMapData, FROM_MODULE_ID_SIZE);
	moduleId[FROM_MODULE_ID_SIZE] = '\0';

	CAM_DBG(CAM_EEPROM, "%s module_id = %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
		mInfo->typeStr,
		moduleId[0], moduleId[1], moduleId[2], moduleId[3],
		moduleId[4], moduleId[5], moduleId[6], moduleId[7],
		moduleId[8], moduleId[9]);

	return 0;
}

static int cam_eeprom_module_info_set_load_version(int rev, uint32_t hasSubCaldata,
	uint32_t is_supported, uint8_t *pMapData, ModuleInfo_t *mInfo)
{
	int         rc                  = 0;
	int         i                   = 0;

	uint8_t     loadfrom            = 'N';
	uint8_t     sensor_ver[2]       = {0,};
	uint8_t     dll_ver[2]          = {0,};
	uint32_t    normal_is_supported = 0;
	uint8_t     normal_cri_rev      = 0;
	uint8_t     bVerNull            = FALSE;

	uint32_t    ConfIdx             = 0;
	uint32_t    ConfAddr            = 0;

	char        cal_ver[FROM_MODULE_FW_INFO_SIZE+1]  = "";
	char        ideal_ver[FROM_MODULE_FW_INFO_SIZE+1] = "";

	char        *module_fw_ver;
	char        *load_fw_ver;
	char        *phone_fw_ver;

	if(mInfo == NULL)
	{
		CAM_ERR(CAM_EEPROM, "invalid argument");
		rc = 1;
		return rc;
	}

	module_fw_ver = mInfo->mVer.module_fw_ver;
	phone_fw_ver = mInfo->mVer.phone_fw_ver;
	load_fw_ver = mInfo->mVer.load_fw_ver;

	memset(module_fw_ver, 0x00, FROM_MODULE_FW_INFO_SIZE+1);
	memset(phone_fw_ver, 0x00, FROM_MODULE_FW_INFO_SIZE+1);
	memset(load_fw_ver, 0x00, FROM_MODULE_FW_INFO_SIZE+1);

	if(isValidIdx(ADDR_M_CALMAP_VER, &ConfAddr) == 1) {
		ConfAddr += 0x03;
		mInfo->mapVer = pMapData[ConfAddr];
	}

	if(mInfo->mapVer >= 0x80 || !isalnum(mInfo->mapVer)) {
		CAM_INFO(CAM_EEPROM, "subdev_id: %d, map version = 0x%x", mInfo->type, mInfo->mapVer);
		mInfo->mapVer = '0';
	} else {
		CAM_INFO(CAM_EEPROM, "subdev_id: %d, map version = %c [0x%x]", mInfo->type, mInfo->mapVer, mInfo->mapVer);
	}

	if(mInfo->M_or_S == MAIN_MODULE) {
		ConfIdx = ADDR_M_FW_VER;
	} else {
		ConfIdx = ADDR_S_FW_VER;
	}

	if(isValidIdx(ConfIdx, &ConfAddr) == 1)
	{
		memcpy(module_fw_ver, &pMapData[ConfAddr], FROM_MODULE_FW_INFO_SIZE);
		module_fw_ver[FROM_MODULE_FW_INFO_SIZE] = '\0';
		CAM_DBG(CAM_EEPROM,
			"%s manufacturer info = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			mInfo->typeStr,
			module_fw_ver[0], module_fw_ver[1], module_fw_ver[2], module_fw_ver[3], module_fw_ver[4],
			module_fw_ver[5], module_fw_ver[6], module_fw_ver[7], module_fw_ver[8], module_fw_ver[9],
			module_fw_ver[10]);

		/* temp phone version */
		snprintf(phone_fw_ver, FROM_MODULE_FW_INFO_SIZE+1, "%s%s%s%s",
			mInfo->mVer.phone_hw_info, mInfo->mVer.phone_sw_info,
			mInfo->mVer.phone_vendor_info, mInfo->mVer.phone_process_info);
		phone_fw_ver[FROM_MODULE_FW_INFO_SIZE] = '\0';
		CAM_DBG(CAM_EEPROM,
			"%s phone info = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			mInfo->typeStr,
			phone_fw_ver[0], phone_fw_ver[1], phone_fw_ver[2], phone_fw_ver[3], phone_fw_ver[4],
			phone_fw_ver[5], phone_fw_ver[6], phone_fw_ver[7], phone_fw_ver[8], phone_fw_ver[9],
			phone_fw_ver[10]);
	}

#if defined(CONFIG_SAMSUNG_REAR_BOKEH)
        if(mInfo->type == CAM_EEPROM_IDX_BACK) {
            ConfIdx = ADDR_CUSTOM_FW_VER;
            memset(bokeh_module_fw_ver,0x00,sizeof(bokeh_module_fw_ver));
            if(isValidIdx(ConfIdx, &ConfAddr) == 1)
            {
                memcpy(bokeh_module_fw_ver, &pMapData[ConfAddr], FROM_MODULE_FW_INFO_SIZE);
                bokeh_module_fw_ver[FROM_MODULE_FW_INFO_SIZE] = '\0';
                CAM_DBG(CAM_EEPROM,
                        "[BOKEH]%s manufacturer info = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                        mInfo->typeStr,
                        bokeh_module_fw_ver[0], bokeh_module_fw_ver[1], bokeh_module_fw_ver[2], bokeh_module_fw_ver[3], bokeh_module_fw_ver[4],
                        bokeh_module_fw_ver[5], bokeh_module_fw_ver[6], bokeh_module_fw_ver[7], bokeh_module_fw_ver[8], bokeh_module_fw_ver[9],
                        bokeh_module_fw_ver[10]);
            }
            ConfIdx = ADDR_CUSTOM_SENSOR_ID;
            memset(rear3_sensor_id,0x00,sizeof(rear3_sensor_id));
            if(isValidIdx(ConfIdx, &ConfAddr) == 1)
            {
                memcpy(rear3_sensor_id, &pMapData[ConfAddr], FROM_SENSOR_ID_SIZE);
                rear3_sensor_id[FROM_SENSOR_ID_SIZE] = '\0';
				CAM_DBG(CAM_EEPROM,
                        "[BOKEH]%s sensor_id = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                        mInfo->typeStr,
                        rear3_sensor_id[0], rear3_sensor_id[1], rear3_sensor_id[2], rear3_sensor_id[3],
                        rear3_sensor_id[4], rear3_sensor_id[5], rear3_sensor_id[6], rear3_sensor_id[7],
                        rear3_sensor_id[8], rear3_sensor_id[9], rear3_sensor_id[10], rear3_sensor_id[11],
                        rear3_sensor_id[12], rear3_sensor_id[13], rear3_sensor_id[14], rear3_sensor_id[15]);
            }
        }
        //fill rear3 fw info
        sprintf(cam3_fw_ver, "%s %s\n", bokeh_module_fw_ver, bokeh_module_fw_ver);
        sprintf(cam3_fw_full_ver, "%s %s %s\n", bokeh_module_fw_ver, bokeh_module_fw_ver,bokeh_module_fw_ver);
#endif

#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
        if(mInfo->type == CAM_EEPROM_IDX_BACK3) {
            ConfIdx = ADDR_M_FW_VER;
            memset(rear3_module_fw_ver, 0x00, sizeof(rear3_module_fw_ver));
            if (isValidIdx(ConfIdx, &ConfAddr) == 1)
            {
                memcpy(rear3_module_fw_ver, &pMapData[ConfAddr], FROM_MODULE_FW_INFO_SIZE);
                rear3_module_fw_ver[FROM_MODULE_FW_INFO_SIZE] = '\0';
                CAM_DBG(CAM_EEPROM,
                        "[TRIPLE]%s manufacturer info = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                        mInfo->typeStr,
                        rear3_module_fw_ver[0], rear3_module_fw_ver[1], rear3_module_fw_ver[2], rear3_module_fw_ver[3], rear3_module_fw_ver[4],
                        rear3_module_fw_ver[5], rear3_module_fw_ver[6], rear3_module_fw_ver[7], rear3_module_fw_ver[8], rear3_module_fw_ver[9],
                        rear3_module_fw_ver[10]);
            }

            ConfIdx = ADDR_M_SENSOR_ID;
            memset(rear3_sensor_id, 0x00, sizeof(rear3_sensor_id));
            if (isValidIdx(ConfIdx, &ConfAddr) == 1)
            {
                memcpy(rear3_sensor_id, &pMapData[ConfAddr], FROM_SENSOR_ID_SIZE);
                rear3_sensor_id[FROM_SENSOR_ID_SIZE] = '\0';
                CAM_DBG(CAM_EEPROM,
                        "[TRIPLE]%s sensor_id = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                        mInfo->typeStr,
                        rear3_sensor_id[0], rear3_sensor_id[1], rear3_sensor_id[2], rear3_sensor_id[3],
                        rear3_sensor_id[4], rear3_sensor_id[5], rear3_sensor_id[6], rear3_sensor_id[7],
                        rear3_sensor_id[8], rear3_sensor_id[9], rear3_sensor_id[10], rear3_sensor_id[11],
                        rear3_sensor_id[12], rear3_sensor_id[13], rear3_sensor_id[14], rear3_sensor_id[15]);
            }
        }

        //fill rear3 fw info
        sprintf(cam3_fw_ver, "%s %s\n", rear3_module_fw_ver, rear3_module_fw_ver);
        sprintf(cam3_fw_full_ver, "%s %s %s\n", rear3_module_fw_ver, rear3_module_fw_ver,rear3_module_fw_ver);
#endif

	/* temp load version */
	if (mInfo->type == CAM_EEPROM_IDX_BACK && mInfo->M_or_S == MAIN_MODULE &&
		strncmp(phone_fw_ver, module_fw_ver, HW_INFO_MAX_SIZE-1) == 0 &&
		strncmp(&phone_fw_ver[HW_INFO_MAX_SIZE-1], &module_fw_ver[HW_INFO_MAX_SIZE-1], SW_INFO_MAX_SIZE-1) >= 0)
	{
		CAM_INFO(CAM_EEPROM, "Load from phone");
		strcpy(load_fw_ver, phone_fw_ver);
		loadfrom = 'P';
	}
	else
	{
		CAM_INFO(CAM_EEPROM, "Load from EEPROM");
		strcpy(load_fw_ver, module_fw_ver);
		loadfrom = 'E';
	}

	//	basically, cal_ver is the version when the module is calibrated.
	//	It can be different in case that the module_fw_ver is updated by FW on F-ROM for testing.
	//	otherwise, module_fw_ver and cal_ver should be the same.
	if(mInfo->M_or_S == MAIN_MODULE) {
		ConfIdx = ADDR_M_FW_VER;
	} else {
		ConfIdx = ADDR_S_FW_VER;
	}

	if(isValidIdx(ConfIdx, &ConfAddr) == 1)
	{
		bVerNull = FALSE;
		for(i = 0; i < FROM_MODULE_FW_INFO_SIZE; i ++) {
			if(pMapData[ConfAddr + i] >= 0x80 || !isalnum(pMapData[ConfAddr + i])) {
				cal_ver[i] = ' ';
				bVerNull = TRUE;
			} else {
				cal_ver[i] = pMapData[ConfAddr + i];
			}

			if(phone_fw_ver[i] >= 0x80 || !isalnum(phone_fw_ver[i]))
			{
				phone_fw_ver[i] = ' ';
			}
		}
	}

	if(isValidIdx(ADDR_M_MODULE_ID, &ConfAddr) == 1)
	{
		ConfAddr += 0x06;
		cam_eeprom_module_info_set_module_id(mInfo, &pMapData[ConfAddr]);
	}

	sensor_ver[0] = 0;
	sensor_ver[1] = 0;
	dll_ver[0] = 0;
	dll_ver[1] = 0;

	ConfIdx = ADDR_M_SENSOR_ID;
	if(isValidIdx(ConfIdx, &ConfAddr) == 1)
	{
		cam_eeprom_module_info_set_sensor_id(mInfo, ConfIdx, &pMapData[ConfAddr]);
		sensor_ver[0] = mInfo->mVer.sensor_id[8];
	}

	if(isValidIdx(ADDR_M_DLL_VER, &ConfAddr) == 1)
	{
		ConfAddr += 0x03;
		dll_ver[0] = pMapData[ConfAddr] - '0';
	}

	if(hasSubCaldata == 1)
	{
		ConfIdx = ADDR_S_SENSOR_ID;
		if(isValidIdx(ADDR_S_SENSOR_ID, &ConfAddr) == 1)
		{
			cam_eeprom_module_info_set_sensor_id(mInfo, ConfIdx, &pMapData[ConfAddr]);
			sensor_ver[1] = mInfo->mVer.sensor2_id[8];
		}

		if(isValidIdx(ADDR_S_DLL_VER, &ConfAddr) == 1)
		{
			ConfAddr += 0x03;
			dll_ver[1] = pMapData[ConfAddr] - '0';
		}
	}

	normal_is_supported = CAMERA_NORMAL_CAL_CRC;

	if(isValidIdx(DEF_M_CHK_VER, &ConfAddr) == 1)
	{
		normal_cri_rev = CriterionRev;
	}

	strcpy(ideal_ver, phone_fw_ver);
	if(module_fw_ver[9] < 0x80 && isalnum(module_fw_ver[9])) {
		ideal_ver[9] = module_fw_ver[9];
	}
	if(module_fw_ver[10] < 0x80 && isalnum(module_fw_ver[10])) {
		ideal_ver[10] = module_fw_ver[10];
	}

	if(rev < normal_cri_rev && bVerNull == TRUE)
	{
		strcpy(cal_ver, ideal_ver);
		loadfrom = 'P';
		CAM_ERR(CAM_EEPROM, "set tmp ver: %s", cal_ver);
	}

	snprintf(mInfo->mVer.module_info, SYSFS_MODULE_INFO_SIZE, "SSCAL %c%s%04X%04XR%02dM%cD%02XD%02XS%02XS%02X/%s%04X%04XR%02d",
		loadfrom, cal_ver, (is_supported >> 16) & 0xFFFF, is_supported & 0xFFFF,
		rev & 0xFF, mInfo->mapVer, dll_ver[0] & 0xFF, dll_ver[1] & 0xFF, sensor_ver[0] & 0xFF, sensor_ver[1] & 0xFF,
		ideal_ver, (normal_is_supported >> 16) & 0xFFFF, normal_is_supported & 0xFFFF, normal_cri_rev);
#ifdef CAM_EEPROM_DBG
	CAM_DBG(CAM_EEPROM, "%s info = %s", mInfo->typeStr, mInfo->mVer.module_info);
#endif

	/* update EEPROM fw version on sysfs */
	// if(mInfo->type != CAM_EEPROM_IDX_BACK)
	{
		strncpy(load_fw_ver, module_fw_ver, FROM_MODULE_FW_INFO_SIZE);
		load_fw_ver[FROM_MODULE_FW_INFO_SIZE] = '\0';
		sprintf(phone_fw_ver, "N");
	}

	//	tele module
	if(mInfo->type == CAM_EEPROM_IDX_BACK && mInfo->M_or_S != MAIN_MODULE)
	{
		sprintf(phone_fw_ver, "N");
	}

	sprintf(mInfo->mVer.cam_fw_ver, "%s %s\n", module_fw_ver, load_fw_ver);
	sprintf(mInfo->mVer.cam_fw_full_ver, "%s %s %s\n", module_fw_ver, phone_fw_ver, load_fw_ver);

#ifdef CAM_EEPROM_DBG
	CAM_DBG(CAM_EEPROM, "%s manufacturer info = %c %c %c %c %c %c %c %c %c %c %c",
		mInfo->typeStr,
		module_fw_ver[0], module_fw_ver[1], module_fw_ver[2], module_fw_ver[3], module_fw_ver[4],
		module_fw_ver[5], module_fw_ver[6], module_fw_ver[7], module_fw_ver[8], module_fw_ver[9],
		module_fw_ver[10]);

	CAM_DBG(CAM_EEPROM, "%s phone_fw_ver = %c %c %c %c %c %c %c %c %c %c %c",
		mInfo->typeStr,
		phone_fw_ver[0], phone_fw_ver[1], phone_fw_ver[2], phone_fw_ver[3], phone_fw_ver[4],
		phone_fw_ver[5], phone_fw_ver[6], phone_fw_ver[7], phone_fw_ver[8], phone_fw_ver[9],
		phone_fw_ver[10]);

	CAM_DBG(CAM_EEPROM, "%s load_fw_ver = %c %c %c %c %c %c %c %c %c %c %c",
		mInfo->typeStr,
		load_fw_ver[0], load_fw_ver[1], load_fw_ver[2], load_fw_ver[3], load_fw_ver[4],
		load_fw_ver[5], load_fw_ver[6], load_fw_ver[7], load_fw_ver[8], load_fw_ver[9],
		load_fw_ver[10]);
#endif

	return rc;
}

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE) || defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_DUAL) || defined(CONFIG_SAMSUNG_FRONT_TOF)
static int cam_eeprom_module_info_set_dual_tilt(eDualTiltMode tiltMode, uint32_t dual_addr_idx,
	uint32_t dual_size_idx, uint8_t *pMapData, char *log_str,
	ModuleInfo_t *mInfo)
{
	uint32_t offset_dll_ver          = 0;
	uint32_t offset_x                = 0;
	uint32_t offset_y                = 0;
	uint32_t offset_z                = 0;
	uint32_t offset_sx               = 0;
	uint32_t offset_sy               = 0;
	uint32_t offset_range            = 0;
	uint32_t offset_max_err          = 0;
	uint32_t offset_avg_err          = 0;
	uint32_t offset_project_cal_type = 0;

	uint8_t    *dual_cal;
	DualTilt_t *dual_tilt;

	uint32_t addr = 0;
	uint32_t size = 0;
	uint8_t  var  = 0;

	if (mInfo == NULL || mInfo->mVer.dual_cal == NULL || mInfo->mVer.DualTilt == NULL)
	{
		CAM_ERR(CAM_EEPROM, "dual_cal or DualTilt is NULL");
		return 1;
	}

	dual_cal = mInfo->mVer.dual_cal;
	dual_tilt = mInfo->mVer.DualTilt;
	memset(dual_tilt, 0x00, sizeof(DualTilt_t));

	if (isValidIdx(dual_addr_idx, &addr) == 1 && isValidIdx(dual_size_idx, &size) == 1)
	{
		switch (tiltMode)
		{
#if defined(CONFIG_SEC_X1Q_PROJECT) || defined(CONFIG_SEC_Y2Q_PROJECT) || defined(CONFIG_SEC_C1Q_PROJECT) || defined(CONFIG_SEC_F2Q_PROJECT)\
	 || defined(CONFIG_SEC_R8Q_PROJECT) || defined(CONFIG_SEC_VICTORY_PROJECT)
			case DUAL_TILT_REAR_TELE:
				offset_dll_ver          = 0x02F4;
				offset_x                = 0x00B8;
				offset_y                = 0x00BC;
				offset_z                = 0x00C0;
				offset_sx               = 0x00DC;
				offset_sy               = 0x00E0;
				offset_range            = 0x02D2;
				offset_max_err          = 0x02D6;
				offset_avg_err          = 0x02DA;
				offset_project_cal_type = 0x02DF;
				break;
#endif

#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
			case DUAL_TILT_REAR_TELE:
				offset_dll_ver          = 0x02F4;
				offset_x                = 0x00B8;
				offset_y                = 0x00BC;
				offset_z                = 0x00C0;
				offset_sx               = 0x00DC;
				offset_sy               = 0x00E0;
				offset_range            = 0x02D2;
				offset_max_err          = 0x02D6;
				offset_avg_err          = 0x02DA;
				offset_project_cal_type = 0x02DF;
				break;
#endif

			case DUAL_TILT_REAR_WIDE:
			case DUAL_TILT_REAR_UW:
				offset_dll_ver = 0x0000;
				offset_x       = 0x0060;
				offset_y       = 0x0064;
				offset_z       = 0x0068;
				offset_sx      = 0x00C0;
				offset_sy      = 0x00C4;
				offset_range   = 0x07E0;
				offset_max_err = 0x07E4;
				offset_avg_err = 0x07E8;
#if defined(CONFIG_SEC_BLOOMXQ_PROJECT)
				offset_x       = 0x011C;
				offset_y       = 0x0120;
				offset_z       = 0x0124;
				offset_sx      = 0x0128;
				offset_sy      = 0x012C;
				offset_range   = 0x07F0;
				offset_max_err = 0x07F4;
				offset_avg_err = 0x07F8;
				offset_project_cal_type = 0x0108;
#endif
				break;

			case DUAL_TILT_FRONT:
				offset_dll_ver = 0x0000;
				offset_x       = 0x0060;
				offset_y       = 0x0064;
				offset_z       = 0x0068;
				offset_sx      = 0x00C0;
				offset_sy      = 0x00C4;
				offset_range   = 0x03E4;
				offset_max_err = 0x03E8;
				offset_avg_err = 0x03EC;
				break;

			case DUAL_TILT_TOF_REAR:
				offset_dll_ver = 0x0000;
				offset_x       = 0x006C;
				offset_y       = 0x0070;
				offset_z       = 0x0074;
				offset_sx      = 0x03C0;
				offset_sy      = 0x03C4;
				offset_range   = 0x04E0;
				offset_max_err = 0x04E4;
				offset_avg_err = 0x04E8;
				break;

			case DUAL_TILT_TOF_REAR2:
				offset_dll_ver = 0x0000;
				offset_x       = 0x0160;
				offset_y       = 0x0164;
				offset_z       = 0x0168;
				offset_sx      = 0x05C8;
				offset_sy      = 0x05CC;
				offset_range   = 0x06E8;
				offset_max_err = 0x06EC;
				offset_avg_err = 0x06F0;
				break;

			case DUAL_TILT_TOF_FRONT:
				offset_dll_ver = 0x07F4;
				offset_x       = 0x04B8;
				offset_y       = 0x04BC;
				offset_z       = 0x04C0;
				offset_sx      = 0x04DC;
				offset_sy      = 0x04E0;
				offset_range   = 0x07EC;
				offset_max_err = 0x07E8;
				offset_avg_err = 0x07E4;
				break;

			default:
				break;
		}

		memcpy(dual_cal, &pMapData[addr], size);
		dual_cal[size] = '\0';
		CAM_INFO(CAM_EEPROM, "%s dual cal = %s", log_str, dual_cal);

		/* dual tilt */
		memcpy(&dual_tilt->dll_ver, &pMapData[addr + offset_dll_ver], 4);
		memcpy(&dual_tilt->x,       &pMapData[addr + offset_x], 4);
		memcpy(&dual_tilt->y,       &pMapData[addr + offset_y], 4);
		memcpy(&dual_tilt->z,       &pMapData[addr + offset_z], 4);
		memcpy(&dual_tilt->sx,      &pMapData[addr + offset_sx], 4);
		memcpy(&dual_tilt->sy,      &pMapData[addr + offset_sy], 4);
		memcpy(&dual_tilt->range,   &pMapData[addr + offset_range], 4);
		memcpy(&dual_tilt->max_err, &pMapData[addr + offset_max_err], 4);
		memcpy(&dual_tilt->avg_err, &pMapData[addr + offset_avg_err], 4);

		sprintf(dual_tilt->project_cal_type, "NONE");
		if (offset_project_cal_type)
		{
			for (var = 0; var < PROJECT_CAL_TYPE_MAX_SIZE; var++)
			{
				if ((pMapData[addr + offset_project_cal_type] == 0xFF) && (var == 0))
				{
					break;
				}

				if ((pMapData[addr + offset_project_cal_type+var] == 0xFF) || (pMapData[addr + offset_project_cal_type+var] == 0x00))
				{
					dual_tilt->project_cal_type[var] = '\0';
					break;
				}
				memcpy(&dual_tilt->project_cal_type[var], &pMapData[addr + offset_project_cal_type+var], 1);
			}
		}

		CAM_DBG(CAM_EEPROM,
			"%s dual tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d, project_cal_type=%s",
			log_str,
			dual_tilt->x, dual_tilt->y, dual_tilt->z, dual_tilt->sx,
			dual_tilt->sy, dual_tilt->range, dual_tilt->max_err,
			dual_tilt->avg_err, dual_tilt->dll_ver, dual_tilt->project_cal_type);
	}
	else
	{
		CAM_ERR(CAM_EEPROM,
			"isSet addr: %d, size: %d", ConfigInfo[dual_addr_idx].isSet, ConfigInfo[dual_addr_idx].isSet);
	}

	return 0;
}
#endif

static int cam_eeprom_module_info_set_paf(uint32_t dual_addr_idx,
	uint32_t st_offset, uint32_t mid_far_size, uint8_t *pMapData, char *log_str,
	char *paf_cal, uint32_t paf_cal_size)
{
	int      i, step;
	uint32_t size;
	uint32_t st_addr     = 0;
	char     tempbuf[10] = "";

	if(mid_far_size == PAF_MID_SIZE)
		step = 8;
	else
		step = 2;

	size = (mid_far_size/step)-1;

	CAM_INFO(CAM_EEPROM, "paf_cal: %p, paf_cal_size: %d", paf_cal, paf_cal_size);
	if(isValidIdx(dual_addr_idx, &st_addr) == 1)
	{
		st_addr += st_offset;

		memset(paf_cal, 0, paf_cal_size);

		for (i = 0; i < size; i++) {
			sprintf(tempbuf, "%d,", *((s16 *)&pMapData[st_addr + step*i]));
			strncat(paf_cal, tempbuf, strlen(tempbuf));
			memset(tempbuf, 0, strlen(tempbuf));
		}

		sprintf(tempbuf, "%d", *((s16 *)&pMapData[st_addr + step*i]));
		strncat(paf_cal, tempbuf, strlen(tempbuf));
		strncat(paf_cal, "\n", strlen("\n"));

		CAM_DBG(CAM_EEPROM, "%s = %s", log_str, paf_cal);
	}
	else
	{
		CAM_ERR(CAM_EEPROM,
			"isSet addr: %d, size: %d", ConfigInfo[dual_addr_idx].isSet, ConfigInfo[dual_addr_idx].isSet);
	}

	return 0;
}

static int cam_eeprom_module_info_set_rear_af(uint32_t st_addr_idx, AfIdx_t *af_idx, uint32_t num_idx,
	uint8_t *pMapData, char *af_cal_str)
{
	int  i;
	char tempbuf[15] = "";
	uint32_t tempval;
	uint32_t st_addr = 0;
	int  len = 0;
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	int  j;
#endif

	CAM_INFO(CAM_EEPROM, "st_addr_idx: 0x%04X, af_cal_str = %s", st_addr_idx, af_cal_str);

	if(isValidIdx(st_addr_idx, &st_addr) == 1)
	{
		af_cal_str[0] = '\0';

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
		for(i = 0, j = 0; ((i < AF_CAL_IDX_MAX) && (j < num_idx)); i ++)
		{
			if(i == 0)	continue;

			if(af_idx[j].idx == AF_CAL_MACRO_IDX)
			{
				j ++;
				continue;
			}

			if(j < num_idx && i == af_idx[j].idx)
			{
				memcpy(&tempval, &pMapData[st_addr + af_idx[j].offset], 4);
				sprintf(tempbuf, "%d ", tempval);
				j ++;
			}
			else
			{
				sprintf(tempbuf, "N ");
			}

			strncat(af_cal_str, tempbuf, strlen(tempbuf));
		}
#else
		for(i = 0; i < num_idx; i ++)
		{
			memcpy(&tempval, &pMapData[st_addr + af_idx[i].offset], 4);
			sprintf(tempbuf, "%d ", tempval);

			strncat(af_cal_str, tempbuf, strlen(tempbuf));
		}
#endif
		len = strlen(af_cal_str);
		if(af_cal_str[len-1] == ' ')
			af_cal_str[len-1] = '\0';

		CAM_INFO(CAM_EEPROM, "af_cal_str = %s", af_cal_str);
	}
	else
	{
		CAM_ERR(CAM_EEPROM,
			"isSet addr: %d", ConfigInfo[st_addr_idx].isSet);
	}

	return 0;
}

#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
static int cam_eeprom_module_info_tof(uint8_t *pMapData, char *log_str,
	uint8_t *tof_cal, uint8_t *tof_extra_cal, int *tof_uid, uint8_t *tof_cal_res,
	int *tof_cal_500, int *tof_cal_300)
{
	uint32_t st_addr        = 0;
	uint32_t uid_addr       = 0;
	uint32_t res_addr       = 0;
	uint32_t cal_size       = 0;
	uint32_t cal_extra_size = 0;
	uint32_t validation_500 = 0;
	uint32_t validation_300 = 0;

	if(isValidIdx(ADDR_TOFCAL_START, &st_addr) == 1 &&
		isValidIdx(ADDR_TOFCAL_SIZE, &cal_size) == 1 &&
		isValidIdx(ADDR_TOFCAL_UID, &uid_addr) == 1 &&
		isValidIdx(ADDR_TOFCAL_RESULT, &res_addr) == 1 &&
		isValidIdx(ADDR_VALIDATION_500, &validation_500) == 1 &&
		isValidIdx(ADDR_VALIDATION_300, &validation_300) == 1)
	{
		cal_extra_size = cal_size - TOFCAL_SIZE;
		CAM_INFO(CAM_EEPROM, "%s tof cal_size: %d, cal_extra_size: %d", log_str, cal_size, cal_extra_size);

		memcpy(tof_uid, &pMapData[uid_addr], 4);
		CAM_DBG(CAM_EEPROM, "%s tof uid = 0x%04X", log_str, *tof_uid);

		memcpy(tof_cal_500, &pMapData[validation_500], 2);
		CAM_DBG(CAM_EEPROM, "%s tof 500mm validation data = 0x%04X", log_str, *tof_cal_500);

		memcpy(tof_cal_300, &pMapData[validation_300], 2);
		CAM_DBG(CAM_EEPROM, "%s tof 300mm validation data = 0x%04X", log_str, *tof_cal_300);

		memcpy(tof_cal, &pMapData[st_addr], TOFCAL_SIZE);
		tof_cal[TOFCAL_SIZE] = '\0';
		CAM_DBG(CAM_EEPROM, "%s tof cal = %s", log_str, tof_cal);

		memcpy(tof_extra_cal, &pMapData[st_addr + TOFCAL_SIZE], cal_extra_size);
		tof_extra_cal[cal_extra_size] = '\0';
		CAM_DBG(CAM_EEPROM, "%s tof ext = %s", log_str, tof_extra_cal);

		CAM_DBG(CAM_EEPROM, "%s tof RESULT_ADDR 0x%x 0x%x 0x%x",
			log_str,
			pMapData[res_addr],
			pMapData[res_addr + 2],
			pMapData[res_addr + 4]);

		if (pMapData[res_addr] == 0x11 &&
			pMapData[res_addr + 2] == 0x11 &&
			pMapData[res_addr + 4] == 0x11)
		{
			*tof_cal_res = 1;
		}
	}
	else
	{
		CAM_ERR(CAM_EEPROM, "start: %d, end: %d, uid: %d, result: %d",
			ConfigInfo[ADDR_TOFCAL_START].isSet,
			ConfigInfo[ADDR_TOFCAL_SIZE].isSet,
			ConfigInfo[ADDR_TOFCAL_UID].isSet,
			ConfigInfo[ADDR_TOFCAL_RESULT].isSet);
	}

	return 0;
}
#endif

static int cam_eeprom_update_module_info(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int             rc = 0;

	uint32_t 		ConfAddr 	  = 0;
	uint32_t        hasSubCaldata = 0;

	ModuleInfo_t 	mInfo;
	ModuleInfo_t 	mInfoSub;

	unsigned int rev = sec_hw_rev();

	CAM_INFO(CAM_EEPROM, "E");

	if (!e_ctrl) {
		CAM_ERR(CAM_EEPROM, "e_ctrl is NULL");
		return -EINVAL;
	}

	if (e_ctrl->soc_info.index >= CAM_EEPROM_IDX_MAX) {
		CAM_ERR(CAM_EEPROM, "subdev_id: %d is not supported", e_ctrl->soc_info.index);
		return 0;
	}

	memset(&mInfo, 0x00, sizeof(ModuleInfo_t));
	memset(&mInfoSub, 0x00, sizeof(ModuleInfo_t));

	switch(e_ctrl->soc_info.index)
	{
		case CAM_EEPROM_IDX_BACK:
			strlcpy(mInfo.typeStr, "Rear", FROM_MODULE_FW_INFO_SIZE);
			mInfo.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

			mInfo.type                         = e_ctrl->soc_info.index;
			mInfo.M_or_S                       = MAIN_MODULE;

			mInfo.mVer.sensor_id               = rear_sensor_id;
			mInfo.mVer.sensor2_id              = rear_sensor_id;
			mInfo.mVer.module_id               = rear_module_id;

			mInfo.mVer.module_info             = module_info;

			mInfo.mVer.cam_cal_ack             = rear_cam_cal_check;
			mInfo.mVer.cam_fw_ver              = cam_fw_ver;
			mInfo.mVer.cam_fw_full_ver         = cam_fw_full_ver;

			mInfo.mVer.fw_user_ver             = cam_fw_user_ver;
			mInfo.mVer.fw_factory_ver          = cam_fw_factory_ver;

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
			// No Control
#else
			mInfo.mVer.sensor2_id              = rear3_sensor_id;

			hasSubCaldata                      = 1;

			strlcpy(mInfoSub.typeStr, "Rear3", FROM_MODULE_FW_INFO_SIZE);
			mInfoSub.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

			mInfoSub.type                      = e_ctrl->soc_info.index;
			mInfoSub.M_or_S                    = SUB_MODULE;

			mInfoSub.mVer.sensor_id            = rear_sensor_id;
			mInfoSub.mVer.sensor2_id           = rear3_sensor_id;
			mInfoSub.mVer.module_id            = rear3_module_id;

			mInfoSub.mVer.module_info          = module3_info;

			mInfoSub.mVer.cam_cal_ack          = rear_cam_cal_check;
			mInfoSub.mVer.cam_fw_ver           = cam3_fw_ver;
			mInfoSub.mVer.cam_fw_full_ver      = cam3_fw_full_ver;
#endif
#endif
			break;

#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
		case CAM_EEPROM_IDX_BACK3:
			strlcpy(mInfo.typeStr, "Rear3", FROM_MODULE_FW_INFO_SIZE);
			mInfo.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

			mInfo.type                         = e_ctrl->soc_info.index;
			mInfo.M_or_S                       = MAIN_MODULE;

			mInfo.mVer.sensor_id               = rear3_sensor_id;
			mInfo.mVer.sensor2_id              = rear3_sensor_id;
			mInfo.mVer.module_id               = rear3_module_id;

			mInfo.mVer.module_info             = module3_info;

			mInfo.mVer.cam_cal_ack             = rear3_cam_cal_check;
			mInfo.mVer.cam_fw_ver              = cam3_fw_ver;
			mInfo.mVer.cam_fw_full_ver         = cam3_fw_full_ver;

			mInfo.mVer.fw_user_ver             = cam3_fw_user_ver;
			mInfo.mVer.fw_factory_ver          = cam3_fw_factory_ver;
			break;
#endif

		case CAM_EEPROM_IDX_FRONT:
			strlcpy(mInfo.typeStr, "Front", FROM_MODULE_FW_INFO_SIZE);
			mInfo.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

			mInfo.type                         = e_ctrl->soc_info.index;
			mInfo.M_or_S                       = MAIN_MODULE;

			mInfo.mVer.sensor_id               = front_sensor_id;
			mInfo.mVer.sensor2_id              = front_sensor_id;
			mInfo.mVer.module_id               = front_module_id;

			mInfo.mVer.module_info             = front_module_info;

			mInfo.mVer.cam_cal_ack             = front_cam_cal_check;
			mInfo.mVer.cam_fw_ver              = front_cam_fw_ver;
			mInfo.mVer.cam_fw_full_ver         = front_cam_fw_full_ver;

			mInfo.mVer.fw_user_ver             = front_cam_fw_user_ver;
			mInfo.mVer.fw_factory_ver          = front_cam_fw_factory_ver;

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
			mInfo.mVer.sensor2_id              = front2_sensor_id;

			hasSubCaldata                      = 1;

			strlcpy(mInfoSub.typeStr, "Front2", FROM_MODULE_FW_INFO_SIZE);
			mInfoSub.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

			mInfoSub.type                      = e_ctrl->soc_info.index;
			mInfoSub.M_or_S                    = SUB_MODULE;

			mInfoSub.mVer.sensor_id            = front_sensor_id;
			mInfoSub.mVer.sensor2_id           = front2_sensor_id;
			mInfoSub.mVer.module_id            = front2_module_id;

			mInfoSub.mVer.module_info          = front2_module_info;

			mInfoSub.mVer.cam_cal_ack          = front_cam_cal_check;
			mInfoSub.mVer.cam_fw_ver           = front2_cam_fw_ver;
			mInfoSub.mVer.cam_fw_full_ver      = front2_cam_fw_full_ver;
#endif
			break;

		case CAM_EEPROM_IDX_BACK2:
			strlcpy(mInfo.typeStr, "Rear2", FROM_MODULE_FW_INFO_SIZE);
			mInfo.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
			mInfo.type                         = e_ctrl->soc_info.index;
			mInfo.M_or_S                       = MAIN_MODULE;

			mInfo.mVer.sensor_id               = rear2_sensor_id;
			mInfo.mVer.sensor2_id              = rear2_sensor_id;
			mInfo.mVer.module_id               = rear2_module_id;

			mInfo.mVer.module_info             = module2_info;

			mInfo.mVer.cam_cal_ack             = rear2_cam_cal_check;
			mInfo.mVer.cam_fw_ver              = cam2_fw_ver;
			mInfo.mVer.cam_fw_full_ver         = cam2_fw_full_ver;

			mInfo.mVer.fw_user_ver             = cam2_fw_user_ver;
			mInfo.mVer.fw_factory_ver          = cam2_fw_factory_ver;
#endif
			break;

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
		case CAM_EEPROM_IDX_FRONT2:
			strlcpy(mInfo.typeStr, "Front3", FROM_MODULE_FW_INFO_SIZE);
			mInfo.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

			mInfo.type                         = e_ctrl->soc_info.index;
			mInfo.M_or_S                       = MAIN_MODULE;

			mInfo.mVer.sensor_id               = front3_sensor_id;
			mInfo.mVer.sensor2_id              = front3_sensor_id;
			mInfo.mVer.module_id               = front3_module_id;

			mInfo.mVer.module_info             = front3_module_info;

			mInfo.mVer.cam_cal_ack             = front_cam_cal_check;
			mInfo.mVer.cam_fw_ver              = front3_cam_fw_ver;
			mInfo.mVer.cam_fw_full_ver         = front3_cam_fw_full_ver;

			mInfo.mVer.fw_user_ver             = front3_cam_fw_user_ver;
			mInfo.mVer.fw_factory_ver          = front3_cam_fw_factory_ver;
			break;
#else
		case CAM_EEPROM_IDX_FRONT2:
			strlcpy(mInfo.typeStr, "Front2", FROM_MODULE_FW_INFO_SIZE);
			mInfo.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

			mInfo.type                         = e_ctrl->soc_info.index;
			mInfo.M_or_S                       = MAIN_MODULE;

			mInfo.mVer.sensor_id               = front2_sensor_id;
			mInfo.mVer.sensor2_id              = front2_sensor_id;
			mInfo.mVer.module_id               = front2_module_id;

			mInfo.mVer.module_info             = front2_module_info;

			mInfo.mVer.cam_cal_ack             = front_cam_cal_check;
			mInfo.mVer.cam_fw_ver              = front2_cam_fw_ver;
			mInfo.mVer.cam_fw_full_ver         = front2_cam_fw_full_ver;

			mInfo.mVer.fw_user_ver             = front2_cam_fw_user_ver;
			mInfo.mVer.fw_factory_ver          = front2_cam_fw_factory_ver;
			break;
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_TOF)
		case CAM_EEPROM_IDX_BACK_TOF:
			strlcpy(mInfo.typeStr, "RearTof", FROM_MODULE_FW_INFO_SIZE);
			mInfo.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

			mInfo.type                         = e_ctrl->soc_info.index;
			mInfo.M_or_S                       = MAIN_MODULE;

			mInfo.mVer.sensor_id               = rear_tof_sensor_id;
			mInfo.mVer.sensor2_id              = rear_tof_sensor_id;
			mInfo.mVer.module_id               = rear_tof_module_id;

			mInfo.mVer.module_info             = rear_tof_module_info;

			mInfo.mVer.cam_cal_ack             = rear_tof_cam_cal_check;
			mInfo.mVer.cam_fw_ver              = cam_tof_fw_ver;
			mInfo.mVer.cam_fw_full_ver         = cam_tof_fw_full_ver;

			mInfo.mVer.fw_user_ver             = cam_tof_fw_user_ver;
			mInfo.mVer.fw_factory_ver          = cam_tof_fw_factory_ver;
			break;
#endif

#if defined(CONFIG_SAMSUNG_FRONT_TOF)
		case CAM_EEPROM_IDX_FRONT_TOF:
			strlcpy(mInfo.typeStr, "FrontTof", FROM_MODULE_FW_INFO_SIZE);
			mInfo.typeStr[FROM_MODULE_FW_INFO_SIZE-1] = '\0';

			mInfo.type                         = e_ctrl->soc_info.index;
			mInfo.M_or_S                       = MAIN_MODULE;

			mInfo.mVer.sensor_id               = front_tof_sensor_id;
			mInfo.mVer.sensor2_id              = front_tof_sensor_id;
			mInfo.mVer.module_id               = front2_module_id;

			mInfo.mVer.module_info             = front_tof_module_info;

			mInfo.mVer.cam_cal_ack             = front_tof_cam_cal_check;
			mInfo.mVer.cam_fw_ver              = front_tof_cam_fw_ver;
			mInfo.mVer.cam_fw_full_ver         = front_tof_cam_fw_full_ver;

			mInfo.mVer.fw_user_ver             = front_tof_cam_fw_user_ver;
			mInfo.mVer.fw_factory_ver          = front_tof_cam_fw_factory_ver;
			break;
#endif

		default:
			break;
	}

	memcpy(mInfo.mVer.phone_hw_info, M_HW_INFO, HW_INFO_MAX_SIZE);
	memcpy(mInfo.mVer.phone_sw_info, M_SW_INFO, SW_INFO_MAX_SIZE);
	memcpy(mInfo.mVer.phone_vendor_info, M_VENDOR_INFO, VENDOR_INFO_MAX_SIZE);
	memcpy(mInfo.mVer.phone_process_info, M_PROCESS_INFO, PROCESS_INFO_MAX_SIZE);

	cam_eeprom_module_info_set_load_version(rev, hasSubCaldata,
		e_ctrl->is_supported, e_ctrl->cal_data.mapdata,	&mInfo);

	if(hasSubCaldata == 1)
	{
		memcpy(mInfoSub.mVer.phone_hw_info, S_HW_INFO, HW_INFO_MAX_SIZE);
		memcpy(mInfoSub.mVer.phone_sw_info, S_SW_INFO, SW_INFO_MAX_SIZE);
		memcpy(mInfoSub.mVer.phone_vendor_info, S_VENDOR_INFO, VENDOR_INFO_MAX_SIZE);
		memcpy(mInfoSub.mVer.phone_process_info, S_PROCESS_INFO, PROCESS_INFO_MAX_SIZE);

		cam_eeprom_module_info_set_load_version(rev, hasSubCaldata,
			e_ctrl->is_supported, e_ctrl->cal_data.mapdata, &mInfoSub);
	}

	if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_FRONT) {
#if !(defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM) || defined(CONFIG_SAMSUNG_FRONT_FIXED_FOCUS))
		/* front af cal*/
		if(isValidIdx(ADDR_M_AF, &ConfAddr) == 1)
		{
			front_af_cal_pan = *((uint32_t *)&e_ctrl->cal_data.mapdata[ConfAddr + AF_CAL_PAN_OFFSET_FROM_AF]);
			front_af_cal_macro = *((uint32_t *)&e_ctrl->cal_data.mapdata[ConfAddr + AF_CAL_MACRO_OFFSET_FROM_AF]);

			CAM_INFO(CAM_EEPROM, "front_af_cal_pan: %d, front_af_cal_macro: %d",
				front_af_cal_pan, front_af_cal_macro);
		}
#endif //!defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM)

		/* front mtf exif */
		if(isValidIdx(ADDR_M0_MTF, &ConfAddr) == 1)
		{
			memcpy(front_mtf_exif, &e_ctrl->cal_data.mapdata[ConfAddr], FROM_MTF_SIZE);
			front_mtf_exif[FROM_MTF_SIZE] = '\0';
			CAM_DBG(CAM_EEPROM, "front mtf exif = %s", front_mtf_exif);
		}

		if(isValidIdx(ADDR_M0_PAF, &ConfAddr) == 1)
		{
			ConfAddr += PAF_CAL_ERR_CHECK_OFFSET;
			memcpy(&front_paf_err_data_result, &e_ctrl->cal_data.mapdata[ConfAddr], 4);
		}

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
		/* front2 dual cal */
		mInfo.mVer.dual_cal = front2_dual_cal;
		mInfo.mVer.DualTilt = &front2_dual;
		cam_eeprom_module_info_set_dual_tilt(DUAL_TILT_FRONT, ADDR_M_DUAL_CAL,
			SIZE_M_DUAL_CAL, e_ctrl->cal_data.mapdata, "front2", &mInfo);
#endif //#if defined(CONFIG_SAMSUNG_FRONT_DUAL)

#if defined(CONFIG_SAMSUNG_FRONT_TOF)
		/* front tof dual cal */
		mInfo.mVer.dual_cal = front_tof_dual_cal;
		mInfo.mVer.DualTilt = &front_tof_dual;
		cam_eeprom_module_info_set_dual_tilt(DUAL_TILT_TOF_FRONT, ADDR_TOFCAL_START,
			ADDR_TOFCAL_SIZE, e_ctrl->cal_data.mapdata, "front_tof", &mInfo);
#endif
	}
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
	else if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_FRONT2)
    {
#if !defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM)
		/* front3 af cal*/
		if(isValidIdx(ADDR_M_AF, &ConfAddr) == 1)
		{
			front3_af_cal_pan = *((uint32_t *)&e_ctrl->cal_data.mapdata[ConfAddr + AF_CAL_PAN_OFFSET_FROM_AF]);
			front3_af_cal_macro = *((uint32_t *)&e_ctrl->cal_data.mapdata[ConfAddr + AF_CAL_MACRO_OFFSET_FROM_AF]);

			CAM_INFO(CAM_EEPROM, "front3_af_cal_pan: %d, front3_af_cal_macro: %d",
				front3_af_cal_pan, front3_af_cal_macro);
		}
#endif //!defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM)
	}
#endif //#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
	else if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_BACK || e_ctrl->soc_info.index == CAM_EEPROM_IDX_BACK3)
#else
	else if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_BACK)
#endif
	{
		/*rear af cal*/
		if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_BACK) {
			AfIdx_t rear_idx_simple[] = {
				{AF_CAL_MACRO_IDX, AF_CAL_MACRO_OFFSET_FROM_AF},
				{AF_CAL_PAN_IDX, AF_CAL_PAN_OFFSET_FROM_AF}
			};

			cam_eeprom_module_info_set_rear_af(ADDR_M_AF, rear_idx_simple, sizeof(rear_idx_simple)/sizeof(rear_idx_simple[0]),
				e_ctrl->cal_data.mapdata, rear_af_cal_str);
		}
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
		if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_BACK3) {
			AfIdx_t rear3_idx_simple[] = {
#if defined(CONFIG_SEC_R8Q_PROJECT)
				{AF_CAL_D40_IDX, AF_CAL_MACRO3_OFFSET_FROM_AF},
#else
				{AF_CAL_MACRO_IDX, AF_CAL_MACRO3_OFFSET_FROM_AF},
#endif
				{AF_CAL_PAN_IDX, AF_CAL_PAN_OFFSET_FROM_AF}
			};

			cam_eeprom_module_info_set_rear_af(ADDR_M_AF, rear3_idx_simple, sizeof(rear3_idx_simple)/sizeof(rear3_idx_simple[0]),
				e_ctrl->cal_data.mapdata, rear3_af_cal_str);
		}
#endif

		/* rear mtf exif */
		if(isValidIdx(ADDR_M0_MTF, &ConfAddr) == 1)
		{
			memcpy(rear_mtf_exif, &e_ctrl->cal_data.mapdata[ConfAddr], FROM_MTF_SIZE);
			rear_mtf_exif[FROM_MTF_SIZE] = '\0';
			CAM_DBG(CAM_EEPROM, "rear mtf exif = %s", rear_mtf_exif);
		}

		/* rear mtf2 exif */
		if(isValidIdx(ADDR_M1_MTF, &ConfAddr) == 1)
		{
			memcpy(rear_mtf2_exif, &e_ctrl->cal_data.mapdata[ConfAddr], FROM_MTF_SIZE);
			rear_mtf2_exif[FROM_MTF_SIZE] = '\0';
			CAM_DBG(CAM_EEPROM, "rear mtf2 exif = %s", rear_mtf2_exif);
		}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
		/* rear3 mtf exif */
		// No Control
#else
		/* rear3 mtf exif */
		if(isValidIdx(ADDR_S0_MTF, &ConfAddr) == 1)
		{
			memcpy(rear3_mtf_exif, &e_ctrl->cal_data.mapdata[ConfAddr], FROM_MTF_SIZE);
			rear3_mtf_exif[FROM_MTF_SIZE] = '\0';
			CAM_DBG(CAM_EEPROM, "rear3 mtf exif = %s", rear3_mtf_exif);
		}
#endif

#if defined(CONFIG_SEC_X1Q_PROJECT) || defined(CONFIG_SEC_Y2Q_PROJECT)|| defined(CONFIG_SEC_C2Q_PROJECT)\
	|| defined(CONFIG_SEC_C1Q_PROJECT)
		if (e_ctrl->cal_data.num_data > 0x5C00)
#elif defined(CONFIG_SEC_F2Q_PROJECT) || defined(CONFIG_SEC_VICTORY_PROJECT)
		if (e_ctrl->cal_data.num_data > 0x5200)
#elif defined(CONFIG_SEC_Z3Q_PROJECT)
		if (e_ctrl->cal_data.num_data > 0x6700)
#elif defined(CONFIG_SEC_R8Q_PROJECT)
		if (e_ctrl->cal_data.num_data > 0x2A00)
#else
		if (e_ctrl->cal_data.num_data > 0xAF00)
#endif
		{
			mInfo.mVer.dual_cal = rear3_dual_cal;
			mInfo.mVer.DualTilt = &rear3_dual;
			cam_eeprom_module_info_set_dual_tilt(DUAL_TILT_REAR_TELE, ADDR_S_DUAL_CAL,
				SIZE_M_DUAL_CAL, e_ctrl->cal_data.mapdata, "rear tele", &mInfo);
		} else {
			CAM_INFO(CAM_EEPROM, "this is old module, dual_tilt tests are not supported, will be filled zero");

			/* rear3 tele tilt */
			memset(&rear3_dual, 0x00, sizeof(rear3_dual));
		}

		CAM_DBG(CAM_EEPROM, "rear_af_cal == TEST ");
		{
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
			AfIdx_t rear_idx[] = {
				{AF_CAL_D10_IDX, AF_CAL_D10_OFFSET_FROM_AF},
				{AF_CAL_PAN_IDX, AF_CAL_PAN_OFFSET_FROM_AF}
			};

			AfIdx_t rear3_idx[] = {
#if defined(CONFIG_SEC_R8Q_PROJECT)
				{AF_CAL_D40_IDX, AF_CAL_D80_OFFSET_FROM_AF},
#else
				{AF_CAL_D80_IDX, AF_CAL_D80_OFFSET_FROM_AF},
#endif
				{AF_CAL_PAN_IDX, AF_CAL_PAN_OFFSET_FROM_AF}
			};

			if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_BACK) {
				cam_eeprom_module_info_set_rear_af(ADDR_M_AF, rear_idx, sizeof(rear_idx)/sizeof(rear_idx[0]),
					e_ctrl->cal_data.mapdata, rear_af_cal_str);
			}
			else if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_BACK3) {
				cam_eeprom_module_info_set_rear_af(ADDR_M_AF, rear3_idx, sizeof(rear3_idx)/sizeof(rear3_idx[0]),
					e_ctrl->cal_data.mapdata, rear3_af_cal_str);
			}
#else
			AfIdx_t rear_idx[] = {
				{AF_CAL_D10_IDX, AF_CAL_D10_OFFSET_FROM_AF},
				{AF_CAL_D50_IDX, AF_CAL_D50_OFFSET_FROM_AF},
				{AF_CAL_PAN_IDX, AF_CAL_PAN_OFFSET_FROM_AF}
			};

			AfIdx_t rear3_idx[] = {
#if !defined(CONFIG_SEC_F2Q_PROJECT) && !defined(CONFIG_SEC_VICTORY_PROJECT)
				{AF_CAL_D30_IDX, AF_CAL_D30_OFFSET_FROM_AF},
#endif
				{AF_CAL_D50_IDX, AF_CAL_D50_OFFSET_FROM_AF},
				{AF_CAL_PAN_IDX, AF_CAL_PAN_OFFSET_FROM_AF}
			};

			cam_eeprom_module_info_set_rear_af(ADDR_M_AF, rear_idx, sizeof(rear_idx)/sizeof(rear_idx[0]),
				e_ctrl->cal_data.mapdata, rear_af_cal_str);

			cam_eeprom_module_info_set_rear_af(ADDR_S0_AF, rear3_idx, sizeof(rear3_idx)/sizeof(rear3_idx[0]),
				e_ctrl->cal_data.mapdata, rear3_af_cal_str);
#endif
		}
#endif

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
		/* rear2 sw dual cal */
		mInfo.mVer.dual_cal = rear2_dual_cal;
		mInfo.mVer.DualTilt = &rear2_dual;
		cam_eeprom_module_info_set_dual_tilt(DUAL_TILT_REAR_UW, ADDR_M_DUAL_CAL,
			SIZE_M_DUAL_CAL, e_ctrl->cal_data.mapdata, "rear2 uw", &mInfo);
#endif

		cam_eeprom_module_info_set_paf(ADDR_M0_PAF,
			PAF_MID_OFFSET, PAF_MID_SIZE, e_ctrl->cal_data.mapdata, "rear_paf_cal_data_mid",
			rear_paf_cal_data_mid, sizeof(rear_paf_cal_data_mid));

		cam_eeprom_module_info_set_paf(ADDR_M0_PAF,
			PAF_FAR_OFFSET, PAF_FAR_SIZE, e_ctrl->cal_data.mapdata, "rear_paf_cal_data_far",
			rear_paf_cal_data_far, sizeof(rear_paf_cal_data_far));

		if(isValidIdx(ADDR_M0_PAF, &ConfAddr) == 1)
		{
			ConfAddr += PAF_CAL_ERR_CHECK_OFFSET;
			memcpy(&paf_err_data_result, &e_ctrl->cal_data.mapdata[ConfAddr], 4);
		}

		cam_eeprom_module_info_set_paf(ADDR_M1_PAF,
			PAF_MID_OFFSET, PAF_MID_SIZE, e_ctrl->cal_data.mapdata, "rear_f2_paf_cal_data_mid",
			rear_f2_paf_cal_data_mid, sizeof(rear_f2_paf_cal_data_mid));

		cam_eeprom_module_info_set_paf(ADDR_M1_PAF,
			PAF_FAR_OFFSET, PAF_FAR_SIZE, e_ctrl->cal_data.mapdata, "rear_f2_paf_cal_data_far",
			rear_f2_paf_cal_data_far, sizeof(rear_f2_paf_cal_data_far));

		if(isValidIdx(ADDR_M1_PAF, &ConfAddr) == 1)
		{
			ConfAddr += PAF_CAL_ERR_CHECK_OFFSET;
			memcpy(&f2_paf_err_data_result, &e_ctrl->cal_data.mapdata[ConfAddr], 4);
		}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
		if(isValidIdx(ADDR_S0_PAF, &ConfAddr) == 1)
		{
			ConfAddr += PAF_CAL_ERR_CHECK_OFFSET;
			memcpy(&rear3_paf_err_data_result, &e_ctrl->cal_data.mapdata[ConfAddr], 4);
		}
#endif

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
		if(isValidIdx(ADDR_M_OIS, &ConfAddr) == 1) {

			ConfAddr += OIS_CAL_MARK_START_OFFSET;
			memcpy(&ois_wide_cal_mark, &e_ctrl->cal_data.mapdata[ConfAddr], 1);
			ConfAddr -= OIS_CAL_MARK_START_OFFSET;
			if (ois_wide_cal_mark == 0xBB) {
				ois_gain_rear_result = 0;
				ois_sr_rear_result = 0;
			} else {
				ois_gain_rear_result = 1;
				ois_sr_rear_result = 1;
			}

			ConfAddr += OIS_XYGG_START_OFFSET;
			memcpy(ois_wide_xygg, &e_ctrl->cal_data.mapdata[ConfAddr], OIS_XYGG_SIZE);
			ConfAddr -= OIS_XYGG_START_OFFSET;

			ConfAddr += OIS_XYSR_START_OFFSET;
			memcpy(ois_wide_xysr, &e_ctrl->cal_data.mapdata[ConfAddr], OIS_XYSR_SIZE);
		}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
		if(isValidIdx(ADDR_S_OIS, &ConfAddr) == 1) {
			int isCal = 0, j = 0;
			ConfAddr += OIS_CAL_MARK_START_OFFSET;
			memcpy(&ois_tele_cal_mark, &e_ctrl->cal_data.mapdata[ConfAddr], 1);
			ConfAddr -= OIS_CAL_MARK_START_OFFSET;
			if (ois_tele_cal_mark == 0xBB) {
				ois_gain_rear3_result = 0;
				ois_sr_rear3_result = 0;
			} else {
				ois_gain_rear3_result = 1;
				ois_sr_rear3_result = 1;
			}

			ConfAddr += OIS_XYGG_START_OFFSET;
			memcpy(ois_tele_xygg, &e_ctrl->cal_data.mapdata[ConfAddr], OIS_XYGG_SIZE);
#if defined(CONFIG_SEC_R8Q_PROJECT)
			{
				uint32_t ygg = 0;
				uint8_t M2_YGG_STD[4] = {0x52,0xB8,0x1E,0x3F};        //0.62f    Standard Value

				memcpy(&ygg, &ois_tele_xygg[4], 4);
				if (ygg > M2_YGG_LMT)
					memcpy(&ois_tele_xygg[4], M2_YGG_STD, 4);
			}
#endif

			ConfAddr -= OIS_XYGG_START_OFFSET;

			ConfAddr += OIS_XYSR_START_OFFSET;
			memcpy(ois_tele_xysr, &e_ctrl->cal_data.mapdata[ConfAddr], OIS_XYSR_SIZE);
			ConfAddr -= OIS_XYSR_START_OFFSET;

			ConfAddr += TELE_OIS_CROSSTALK_START_OFFSET;
			memcpy(ois_tele_cross_talk, &e_ctrl->cal_data.mapdata[ConfAddr], OIS_CROSSTALK_SIZE);
			ConfAddr -= TELE_OIS_CROSSTALK_START_OFFSET;
			ois_tele_cross_talk_result = 0;
			for (j = 0; j < OIS_CROSSTALK_SIZE; j++) {
				if (ois_tele_cross_talk[j] != 0xFF) {
					isCal = 1;
					break;
				}
			}
			ois_tele_cross_talk_result = (isCal == 0) ?  1 : 0;
		}

		if (isValidIdx(ADDR_S_DUAL_CAL, &ConfAddr) == 1) {

			ConfAddr += WIDE_OIS_CENTER_SHIFT_START_OFFSET;
			memcpy(ois_wide_center_shift, &e_ctrl->cal_data.mapdata[ConfAddr], OIS_CENTER_SHIFT_SIZE);
			ConfAddr -= WIDE_OIS_CENTER_SHIFT_START_OFFSET;

			ConfAddr += TELE_OIS_CENTER_SHIFT_START_OFFSET;
			memcpy(ois_tele_center_shift,  &e_ctrl->cal_data.mapdata[ConfAddr], OIS_CENTER_SHIFT_SIZE);
			ConfAddr -= TELE_OIS_CENTER_SHIFT_START_OFFSET;

		}
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_TOF)
		/* rear tof dual cal */
		mInfo.mVer.dual_cal = rear_tof_dual_cal;
		mInfo.mVer.DualTilt = &rear_tof_dual;
		cam_eeprom_module_info_set_dual_tilt(DUAL_TILT_TOF_REAR, ADDR_TOFCAL_START,
			ADDR_TOFCAL_SIZE, e_ctrl->cal_data.mapdata, "rear tof", &mInfo);

		/* rear2 tof tilt */
		//	same dual_cal data between rear_tof and rear2_tof
		mInfo.mVer.dual_cal = rear_tof_dual_cal;
		mInfo.mVer.DualTilt = &rear2_tof_dual;
		cam_eeprom_module_info_set_dual_tilt(DUAL_TILT_TOF_REAR2, ADDR_TOFCAL_START,
			ADDR_TOFCAL_SIZE, e_ctrl->cal_data.mapdata, "rear2 tof", &mInfo);
#endif
	}
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	else if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_BACK2) {
		/* rear2 mtf exif */
		if(isValidIdx(ADDR_M0_MTF, &ConfAddr) == 1)
		{
			memcpy(rear2_mtf_exif, &e_ctrl->cal_data.mapdata[ConfAddr], FROM_MTF_SIZE);
			rear2_mtf_exif[FROM_MTF_SIZE] = '\0';
			CAM_DBG(CAM_EEPROM, "rear2 mtf exif = %s", rear2_mtf_exif);
		}
	}
#endif
#if defined(CONFIG_SAMSUNG_REAR_TOF)
	else if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_BACK_TOF) {
		cam_eeprom_module_info_tof(e_ctrl->cal_data.mapdata, "rear",
			rear_tof_cal, rear_tof_cal_extra, &rear_tof_uid, &rear_tof_cal_result,
			&rear_tof_validation_500, &rear_tof_validation_300);
	}

#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
	else if (e_ctrl->soc_info.index == CAM_EEPROM_IDX_FRONT_TOF) {
		cam_eeprom_module_info_tof(e_ctrl->cal_data.mapdata, "front",
			front_tof_cal, front_tof_cal_extra, &front_tof_uid, &front_tof_cal_result,
			&rear_tof_validation_500, &rear_tof_validation_300);

		if (mInfo.mapVer < '5') {
			CAM_INFO(CAM_EEPROM, "invalid front tof uid (map_ver %c), force chage to 0xCB29", mInfo.mapVer);
			front_tof_uid = 0xCB29;
		}
	}
#endif

	rc = cam_eeprom_check_firmware_cal(e_ctrl->is_supported, &mInfo);
	return rc;
}

void cam_eeprom_update_sysfs_fw_version(
	const char *update_fw_ver, cam_eeprom_fw_version_idx update_fw_index, ModuleInfo_t *mInfo)
{
#if 1
	if (update_fw_index == EEPROM_FW_VER)
		strlcpy(mInfo->mVer.module_fw_ver, update_fw_ver, FROM_MODULE_FW_INFO_SIZE + 1);
	else if (update_fw_index == PHONE_FW_VER)
		strlcpy(mInfo->mVer.phone_fw_ver, update_fw_ver, FROM_MODULE_FW_INFO_SIZE + 1);
	else {
#if defined(CONFIG_SAMSUNG_REAR_TOF)
		if(mInfo->type == CAM_EEPROM_IDX_BACK_TOF)
		{
		    sprintf(mInfo->mVer.load_fw_ver, "N");
			CAM_INFO(CAM_EEPROM, "rear tof not support load_fw_ver");
		}
		else
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
		if(mInfo->type == CAM_EEPROM_IDX_FRONT_TOF)
		{
		    sprintf(mInfo->mVer.load_fw_ver, "N");
			CAM_INFO(CAM_EEPROM, "front tof not support load_fw_ver");
		}
		else
#endif
		strlcpy(mInfo->mVer.load_fw_ver, update_fw_ver, FROM_MODULE_FW_INFO_SIZE + 1);
	}

	/* update EEPROM fw version on sysfs */
	//	all camera module except rear wide module.
	if ((mInfo->type != CAM_EEPROM_IDX_BACK)
	 || (mInfo->type == CAM_EEPROM_IDX_BACK && mInfo->M_or_S != MAIN_MODULE))
	{
		sprintf(mInfo->mVer.phone_fw_ver, "N");
	}

	sprintf(mInfo->mVer.cam_fw_ver, "%s %s\n", mInfo->mVer.module_fw_ver, mInfo->mVer.load_fw_ver);
	sprintf(mInfo->mVer.cam_fw_full_ver, "%s %s %s\n", mInfo->mVer.module_fw_ver, mInfo->mVer.phone_fw_ver, mInfo->mVer.load_fw_ver);

	CAM_ERR(CAM_EEPROM, "camera_idx: %d, cam_fw_full_ver: %s", mInfo->type, mInfo->mVer.cam_fw_full_ver);
#else
	char *pCAM_fw_version, *pCAM_fw_full_version;
	char *pEEPROM_fw_version, *pPHONE_fw_version, *pLOAD_fw_version;

	CAM_INFO(CAM_EEPROM, "camera_idx: %d, update_fw_ver: %s, update_fw_index: %d",
		idx, update_fw_ver, update_fw_index);
	if (idx == CAM_EEPROM_IDX_BACK) {
		pEEPROM_fw_version = rear_fw_ver;
		pPHONE_fw_version = rear_phone_fw_ver;
		pLOAD_fw_version = rear_load_fw_ver;
		pCAM_fw_version = cam_fw_ver;
		pCAM_fw_full_version = cam_fw_full_ver;
	} else if (idx == CAM_EEPROM_IDX_FRONT) {
		pEEPROM_fw_version = front_fw_ver;
		pPHONE_fw_version = "N";
		pLOAD_fw_version = front_load_fw_ver;
		pCAM_fw_version = front_cam_fw_ver;
		pCAM_fw_full_version = front_cam_fw_full_ver;
	}
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	else if (idx == CAM_EEPROM_IDX_BACK2) {
		pEEPROM_fw_version = rear2_fw_ver;
		pPHONE_fw_version = "N";
		pLOAD_fw_version = rear2_load_fw_ver;
		pCAM_fw_version = cam2_fw_ver;
		pCAM_fw_full_version = cam2_fw_full_ver;
	}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
	else if (idx == CAM_EEPROM_IDX_FRONT2) {
		pEEPROM_fw_version = front3_fw_ver;
		pPHONE_fw_version = "N";
		pLOAD_fw_version = front3_load_fw_ver;
		pCAM_fw_version = front3_cam_fw_ver;
		pCAM_fw_full_version = front3_cam_fw_full_ver;
	}
#endif
#if defined(CONFIG_SAMSUNG_REAR_TOF)
	else if (idx == CAM_EEPROM_IDX_BACK_TOF) {
		pEEPROM_fw_version = rear_tof_fw_ver;
		pPHONE_fw_version = "N";
		pLOAD_fw_version = "N";
		pCAM_fw_version = cam_tof_fw_ver;
		pCAM_fw_full_version = cam_tof_fw_full_ver;
	}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
	else if (idx == CAM_EEPROM_IDX_FRONT_TOF) {
		pEEPROM_fw_version = front_tof_fw_ver;
		pPHONE_fw_version = "N";
		pLOAD_fw_version = "N";
		pCAM_fw_version = front_tof_cam_fw_ver;
		pCAM_fw_full_version = front_tof_cam_fw_full_ver;
	}
#endif
	else {
		pEEPROM_fw_version = "N";
		pPHONE_fw_version = "N";
		pLOAD_fw_version = "N";
		pCAM_fw_version = "NULL NULL\n";
		pCAM_fw_full_version = "NULL NULL NULL\n";
	}

	if (update_fw_index == EEPROM_FW_VER)
		strlcpy(pEEPROM_fw_version, update_fw_ver, FROM_MODULE_FW_INFO_SIZE + 1);
	else if (update_fw_index == PHONE_FW_VER)
		strlcpy(pPHONE_fw_version, update_fw_ver, FROM_MODULE_FW_INFO_SIZE + 1);
	else {
#if defined(CONFIG_SAMSUNG_REAR_TOF)
		if(idx == CAM_EEPROM_IDX_BACK_TOF)
			CAM_INFO(CAM_EEPROM,"rear tof not support pLOAD_fw_version");
		else
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
		if(idx == CAM_EEPROM_IDX_FRONT_TOF)
			CAM_INFO(CAM_EEPROM,"front tof not support pLOAD_fw_version");
		else
#endif
		strlcpy(pLOAD_fw_version, update_fw_ver, FROM_MODULE_FW_INFO_SIZE + 1);
	}
	sprintf(pCAM_fw_version, "%s %s\n", pEEPROM_fw_version, pLOAD_fw_version);
	sprintf(pCAM_fw_full_version, "%s %s %s\n", pEEPROM_fw_version, pPHONE_fw_version, pLOAD_fw_version);

	CAM_ERR(CAM_EEPROM, "camera_idx: %d, pCAM_fw_full_version: %s", idx, pCAM_fw_full_version);
#endif
}

int32_t cam_eeprom_check_firmware_cal(uint32_t camera_cal_crc, ModuleInfo_t *mInfo)
{
	int rc = 0;
	char final_cmd_ack[SYSFS_FW_VER_SIZE] = "NG_";
	char cam_cal_ack[SYSFS_FW_VER_SIZE] = "NULL";

	uint8_t isNeedUpdate = TRUE;
	uint8_t version_isp = 0, version_module_maker_ver = 0;
	uint8_t isValid_EEPROM_data = TRUE;
	uint8_t isQCmodule = TRUE;
	uint8_t camera_cal_ack = OK;
	uint8_t camera_fw_crc = OK;
	uint8_t camera_fw_ack = OK;

	version_isp = mInfo->mVer.cam_fw_ver[3];
	version_module_maker_ver = mInfo->mVer.cam_fw_ver[10];

	if (version_isp == 0xff || version_module_maker_ver == 0xff) {
		CAM_ERR(CAM_EEPROM, "invalid eeprom data");
		isValid_EEPROM_data = FALSE;
		cam_eeprom_update_sysfs_fw_version("NULL", EEPROM_FW_VER, mInfo);
	}

	/* 1. check camera firmware and cal data */
	CAM_INFO(CAM_EEPROM, "camera_cal_crc: 0x%x, camera_fw_crc: 0x%x", camera_cal_crc, camera_fw_crc);

	if (mInfo->type == CAM_EEPROM_IDX_BACK)	{
		if (camera_fw_crc == OK) {
			camera_fw_ack = OK;
		} else {
			camera_fw_ack = CRASH;
			strncat(final_cmd_ack, "FW", 2);
		}
	} else {
		camera_fw_ack = OK;
	}

	if (camera_cal_crc == CAMERA_NORMAL_CAL_CRC) {
		camera_cal_ack = OK;
		strncpy(cam_cal_ack, "Normal", SYSFS_FW_VER_SIZE);
	} else {
		camera_cal_ack = CRASH;
		strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);

		if (mInfo->type == CAM_EEPROM_IDX_BACK) {
			camera_cal_ack = CRASH;
			strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
			if ((camera_cal_crc & CAMERA_CAL_CRC_WIDE) != CAMERA_CAL_CRC_WIDE)
				strncat(final_cmd_ack, "CD", 2);
			else
				strncat(final_cmd_ack, "CD4", 3);
#else
			strncat(final_cmd_ack, "CD", 2);
#endif
		} else {
			camera_cal_ack = CRASH;
			strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);
			strncat(final_cmd_ack, "CD3", 3);
		}

		switch(mInfo->type)
		{
			case CAM_EEPROM_IDX_FRONT:
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
			case CAM_EEPROM_IDX_FRONT2:
#endif
#if defined(UNUSE_FRONT_EEPROM)
				strncpy(final_cmd_ack, "NG_", 3);
				strncpy(cam_cal_ack, "NULL", SYSFS_FW_VER_SIZE);
				camera_cal_ack = OK;
#endif
				break;

			default:
				break;
		}
	}

	/* 3-1. all success case: display LOAD FW */
	if (camera_fw_ack && camera_cal_ack)
		isNeedUpdate = FALSE;

	/* 3-2. fail case: update CMD_ACK on sysfs (load fw) */
	// If not QC module, return NG.
	if(version_isp >= 0x80 || !isalnum(version_isp))
		CAM_INFO(CAM_EEPROM, "ISP Ver : 0x%x", version_isp);
	else
		CAM_INFO(CAM_EEPROM, "ISP Ver : %c", version_isp);

	if (version_isp != 'Q' && version_isp != 'U' && version_isp != 'A' && version_isp != 'X') {
		CAM_ERR(CAM_EEPROM, "This is not Qualcomm module!");

		if (mInfo->type == CAM_EEPROM_IDX_BACK) {
			strncpy(final_cmd_ack, "NG_FWCD", SYSFS_FW_VER_SIZE);
			strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);
		} else {
			strncpy(final_cmd_ack, "NG_CD3_L", SYSFS_FW_VER_SIZE);
			strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);
		}

		isNeedUpdate = TRUE;
		isQCmodule = FALSE;
		camera_cal_ack = CRASH;
	}

#if defined(CONFIG_SAMSUNG_REAR_TOF)
		if(mInfo->type == CAM_EEPROM_IDX_BACK_TOF)
		{
		    isNeedUpdate = TRUE;
			CAM_INFO(CAM_EEPROM,"Set true sysfs update for TOF");
		}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
		if(mInfo->type == CAM_EEPROM_IDX_FRONT_TOF)
		{
		    isNeedUpdate = TRUE;
			CAM_INFO(CAM_EEPROM,"Set true sysfs update for TOF");
		}
#endif

	if (isNeedUpdate) {
		CAM_ERR(CAM_EEPROM, "final_cmd_ack : %s", final_cmd_ack);
		cam_eeprom_update_sysfs_fw_version(final_cmd_ack, LOAD_FW_VER, mInfo);
	} else {
		// just display success fw version log
		CAM_INFO(CAM_EEPROM, "final_cmd_ack : %s", final_cmd_ack);
		memset(final_cmd_ack, 0, sizeof(final_cmd_ack));
		strncpy(final_cmd_ack, mInfo->mVer.cam_fw_full_ver, SYSFS_FW_VER_SIZE);
		final_cmd_ack[SYSFS_FW_VER_SIZE-1] = '\0';

		CAM_INFO(CAM_EEPROM, "final_cmd_ack : %s", final_cmd_ack);
	}

	/* 4. update CAL check ack on sysfs rear_calcheck */
	strlcpy(mInfo->mVer.cam_cal_ack, cam_cal_ack, SYSFS_FW_VER_SIZE);
	snprintf(cal_crc, SYSFS_FW_VER_SIZE, "%s %s\n", rear_cam_cal_check, front_cam_cal_check);

	CAM_INFO(CAM_EEPROM,
		"version_module_maker: 0x%x, MODULE_VER_ON_PVR: 0x%x, MODULE_VER_ON_SRA: 0x%x",
		version_module_maker_ver, ModuleVerOnPVR, ModuleVerOnSRA);
	CAM_INFO(CAM_EEPROM,
		"cal_map_version: 0x%x vs FROM_CAL_MAP_VERSION: 0x%x",
		mInfo->mapVer, minCalMapVer);

	if ((isQCmodule == TRUE) && ((isValid_EEPROM_data == FALSE) || (mInfo->mapVer < minCalMapVer)
		|| (version_module_maker_ver < ModuleVerOnPVR))) {
		strncpy(mInfo->mVer.fw_user_ver, "NG", SYSFS_FW_VER_SIZE);
	} else {
		if (camera_cal_ack == CRASH)
			strncpy(mInfo->mVer.fw_user_ver, "NG", SYSFS_FW_VER_SIZE);
		else
			strncpy(mInfo->mVer.fw_user_ver, "OK", SYSFS_FW_VER_SIZE);
	}

	if ((isQCmodule == TRUE) && ((isValid_EEPROM_data == FALSE) || (mInfo->mapVer < minCalMapVer)
		|| (version_module_maker_ver < ModuleVerOnSRA))) {
		strncpy(mInfo->mVer.fw_factory_ver, "NG_VER", SYSFS_FW_VER_SIZE);
	} else {
		if (camera_cal_ack == CRASH) {
			if(mInfo->type == CAM_EEPROM_IDX_BACK) {
				strncpy(mInfo->mVer.fw_factory_ver, "NG_VER", SYSFS_FW_VER_SIZE);
			} else {
				strncpy(mInfo->mVer.fw_factory_ver, "NG_CRC", SYSFS_FW_VER_SIZE);
			}
		}
		else {
			strncpy(mInfo->mVer.fw_factory_ver, "OK", SYSFS_FW_VER_SIZE);
		}
	}

	return rc;
}

/**
 * cam_eeprom_verify_sum - verify crc32 checksum
 * @mem:			data buffer
 * @size:			size of data buffer
 * @sum:			expected checksum
 * @rev_endian:	compare reversed endian (0:little, 1:big)
 *
 * Returns 0 if checksum match, -EINVAL otherwise.
 */
static int cam_eeprom_verify_sum(const char *mem, uint32_t size, uint32_t sum, uint32_t rev_endian)
{
	uint32_t crc = ~0;
	uint32_t cmp_crc = 0;

	/* check overflow */
	if (size > crc - sizeof(uint32_t))
		return -EINVAL;

	crc = crc32_le(crc, mem, size);

	crc = ~crc;
	if (rev_endian == 1) {
		cmp_crc = (((crc) & 0xFF) << 24)
				| (((crc) & 0xFF00) << 8)
				| (((crc) >> 8) & 0xFF00)
				| ((crc) >> 24);
	} else {
		cmp_crc = crc;
	}
	CAM_DBG(CAM_EEPROM, "endian %d, expect 0x%x, result 0x%x", rev_endian, sum, cmp_crc);

	if (cmp_crc != sum) {
		CAM_ERR(CAM_EEPROM, "endian %d, expect 0x%x, result 0x%x", rev_endian, sum, cmp_crc);
		return -EINVAL;
	} else {
		CAM_INFO(CAM_EEPROM, "checksum pass 0x%x", sum);
		return 0;
	}
}

/**
 * cam_eeprom_match_crc - verify multiple regions using crc
 * @data:	data block to be verified
 *
 * Iterates through all regions stored in @data.  Regions with odd index
 * are treated as data, and its next region is treated as checksum.  Thus
 * regions of even index must have valid_size of 4 or 0 (skip verification).
 * Returns a bitmask of verified regions, starting from LSB.  1 indicates
 * a checksum match, while 0 indicates checksum mismatch or not verified.
 */
static uint32_t cam_eeprom_match_crc(struct cam_eeprom_memory_block_t *data, uint32_t subdev_id)
{
	int j, rc;
	uint32_t *sum;
	uint32_t ret = 0;
	uint8_t *memptr, *memptr_crc;
	struct cam_eeprom_memory_map_t *map;

	if (!data) {
		CAM_ERR(CAM_EEPROM, "data is NULL");
		return -EINVAL;
	}
	map = data->map;

#if 1
{

	uint8_t map_ver = 0;
	uint32_t ConfAddr = 0;

	if (subdev_id >= CAM_EEPROM_IDX_BACK && subdev_id < CAM_EEPROM_IDX_MAX)
	{
		if(isValidIdx(ADDR_M_CALMAP_VER, &ConfAddr) == 1) {
			ConfAddr += 0x03;
			map_ver = data->mapdata[ConfAddr];
		}
		else
		{
			CAM_INFO(CAM_EEPROM, "ADDR_M_CALMAP_VER is not set: %d", subdev_id);
		}
	}
	else
	{
		CAM_INFO(CAM_EEPROM, "subdev_id: %d is not supported", subdev_id);
		return 0;
	}

	if(map_ver >= 0x80 || !isalnum(map_ver))
		CAM_INFO(CAM_EEPROM, "map subdev_id = %d, version = 0x%x", subdev_id, map_ver);
	else
		CAM_INFO(CAM_EEPROM, "map subdev_id = %d, version = %c [0x%x]", subdev_id, map_ver, map_ver);
}
#endif

	//  idx 0 is the actual reading section (whole data)
	//  from idx 1, start to compare CRC checksum
	//  (1: CRC area for header, 2: CRC value)
	for (j = 1; j + 1 < data->num_map; j += 2) {
		memptr = data->mapdata + map[j].mem.addr;
		memptr_crc = data->mapdata + map[j+1].mem.addr;

		/* empty table or no checksum */
		if (!map[j].mem.valid_size || !map[j+1].mem.valid_size) {
			CAM_ERR(CAM_EEPROM, "continue");
			continue;
		}

		if (map[j+1].mem.valid_size < sizeof(uint32_t)) {
			CAM_ERR(CAM_EEPROM, "[%d : size 0x%X] malformatted data mapping", j+1, map[j+1].mem.valid_size);
			return -EINVAL;
		}
		CAM_DBG(CAM_EEPROM, "[%d] memptr 0x%x, memptr_crc 0x%x", j, map[j].mem.addr, map[j + 1].mem.addr);
		sum = (uint32_t *) (memptr_crc + map[j+1].mem.valid_size - sizeof(uint32_t));
		rc = cam_eeprom_verify_sum(memptr, map[j].mem.valid_size, *sum, 0);

		if (!rc)
			ret |= 1 << (j/2);
	}

#if 0
	//  if PAF cal data has error (even though CRC is correct),
	//  set crc value of PAF cal data to 0.
	if(subdev_id != 1 && data->mapdata[REAR_MODULE_FW_VERSION+10] == 'M')
	{
		uint32_t PAF_err = 0;
		uint32_t PAF_bit = 0;

		PAF_err = *((uint32_t *)(&data->mapdata[FROM_PAF_CAL_DATA_START_ADDR + 0x14]));
		if(PAF_err != 0)
		{
			PAF_bit |= 0x08;	//	refer to the start addr of PAF cal data at the calibration map
			CAM_ERR(CAM_EEPROM, "Wide1 PAF_err = 0x%08X, Wide1 PAF_bit = 0x%08X", PAF_err, PAF_bit);
		}

		PAF_err = *((uint32_t *)(&data->mapdata[FROM_F2_PAF_CAL_DATA_START_ADDR + 0x14]));
		if(PAF_err != 0)
		{
			PAF_bit |= 0x20;
			CAM_ERR(CAM_EEPROM, "Wide2 PAF_err = 0x%08X, Wide2 PAF_bit = 0x%08X", PAF_err, PAF_bit);
		}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
		PAF_err = *((uint32_t *)(&data->mapdata[FROM_REAR3_PAF_CAL_DATA_START_ADDR + 0x14]));
		if(PAF_err != 0)
		{
			PAF_bit |= 0x8000;
			CAM_ERR(CAM_EEPROM, "Tele PAF_err = 0x%08X, Tele PAF_bit = 0x%08X", PAF_err, PAF_bit);
		}
#endif

		ret &= ~PAF_bit;
	}
#endif

	CAM_INFO(CAM_EEPROM, "CRC result = 0x%08X", ret);

	return ret;
}


/**
 * cam_eeprom_read_memory() - read map data into buffer
 * @e_ctrl:     eeprom control struct
 * @block:      block to be read
 *
 * This function iterates through blocks stored in block->map, reads each
 * region and concatenate them into the pre-allocated block->mapdata
 */
static int cam_eeprom_read_memory(struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_eeprom_memory_block_t *block)
{
	int                                rc = 0;
	int                                j;
	struct cam_sensor_i2c_reg_setting  i2c_reg_settings = {0};
	struct cam_sensor_i2c_reg_array    i2c_reg_array = {0};
	struct cam_eeprom_memory_map_t    *emap = block->map;
	struct cam_eeprom_soc_private     *eb_info = NULL;
	uint8_t                           *memptr = block->mapdata;
#if 1
	uint32_t                          addr = 0, size = 0, read_size = 0;
#endif

	if (!e_ctrl) {
		CAM_ERR(CAM_EEPROM, "e_ctrl is NULL");
		return -EINVAL;
	}

	eb_info = (struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;

	for (j = 0; j < block->num_map; j++) {
		CAM_DBG(CAM_EEPROM, "slave-addr = 0x%X", emap[j].saddr);
		if (emap[j].saddr) {
			eb_info->i2c_info.slave_addr = emap[j].saddr;
			rc = cam_eeprom_update_i2c_info(e_ctrl,
				&eb_info->i2c_info);
			if (rc) {
				CAM_ERR(CAM_EEPROM,
					"failed: to update i2c info rc %d",
					rc);
				return rc;
			}
		}

		if (emap[j].page.valid_size) {
			i2c_reg_settings.addr_type = emap[j].page.addr_type;
			i2c_reg_settings.data_type = emap[j].page.data_type;
			i2c_reg_settings.size = 1;
			i2c_reg_array.reg_addr = emap[j].page.addr;
			i2c_reg_array.reg_data = emap[j].page.data;
			i2c_reg_array.delay = emap[j].page.delay;
			i2c_reg_settings.reg_setting = &i2c_reg_array;
			rc = camera_io_dev_write(&e_ctrl->io_master_info,
				&i2c_reg_settings);
			if (rc) {
				CAM_ERR(CAM_EEPROM, "page write failed rc %d",
					rc);
				return rc;
			}
		}

		if (emap[j].pageen.valid_size) {
			i2c_reg_settings.addr_type = emap[j].pageen.addr_type;
			i2c_reg_settings.data_type = emap[j].pageen.data_type;
			i2c_reg_settings.size = 1;
			i2c_reg_array.reg_addr = emap[j].pageen.addr;
			i2c_reg_array.reg_data = emap[j].pageen.data;
			i2c_reg_array.delay = emap[j].pageen.delay;
			i2c_reg_settings.reg_setting = &i2c_reg_array;
			rc = camera_io_dev_write(&e_ctrl->io_master_info,
				&i2c_reg_settings);
			if (rc) {
				CAM_ERR(CAM_EEPROM, "page enable failed rc %d",
					rc);
				return rc;
			}
		}

		if (emap[j].poll.valid_size) {
			rc = camera_io_dev_poll(&e_ctrl->io_master_info,
				emap[j].poll.addr, emap[j].poll.data,
				0, emap[j].poll.addr_type,
				emap[j].poll.data_type,
				emap[j].poll.delay);
			if (rc) {
				CAM_ERR(CAM_EEPROM, "poll failed rc %d",
					rc);
				return rc;
			}
		}

		if (emap[j].mem.valid_size) {
#if 1
			size = emap[j].mem.valid_size;
			addr = emap[j].mem.addr;
			memptr = block->mapdata + addr;

			CAM_DBG(CAM_EEPROM, "[%d / %d] memptr = %pK, addr = 0x%X, size = 0x%X, subdev = %d",
				j, block->num_map, memptr, emap[j].mem.addr, emap[j].mem.valid_size, e_ctrl->soc_info.index);

			CAM_DBG(CAM_EEPROM, "addr_type = %d, data_type = %d, device_type = %d",
				emap[j].mem.addr_type, emap[j].mem.data_type, e_ctrl->eeprom_device_type);
			if ((e_ctrl->eeprom_device_type == MSM_CAMERA_SPI_DEVICE
				|| e_ctrl->eeprom_device_type == MSM_CAMERA_I2C_DEVICE)
				&& emap[j].mem.data_type == 0) {
				CAM_DBG(CAM_EEPROM,
					"skipping read as data_type 0, skipped:%d",
					read_size);
				continue;
			}

			while(size > 0) {
				read_size = size;
				if (size > I2C_REG_DATA_MAX) {
					read_size = I2C_REG_DATA_MAX;
				}
				rc = camera_io_dev_read_seq(&e_ctrl->io_master_info,
					addr, memptr,
					emap[j].mem.addr_type,
					emap[j].mem.data_type,
					read_size);
				if (rc < 0) {
					CAM_ERR(CAM_EEPROM, "read failed rc %d",
						rc);
					return rc;
				}
				size -= read_size;
				addr += read_size;
				memptr += read_size;
			}
#else
			rc = camera_io_dev_read_seq(&e_ctrl->io_master_info,
				emap[j].mem.addr, memptr,
				emap[j].mem.addr_type,
				emap[j].mem.data_type,
				emap[j].mem.valid_size);
			if (rc < 0) {
				CAM_ERR(CAM_EEPROM, "read failed rc %d",
					rc);
				return rc;
			}
			memptr += emap[j].mem.valid_size;
#endif
		}

		if (emap[j].pageen.valid_size) {
			i2c_reg_settings.addr_type = emap[j].pageen.addr_type;
			i2c_reg_settings.data_type = emap[j].pageen.data_type;
			i2c_reg_settings.size = 1;
			i2c_reg_array.reg_addr = emap[j].pageen.addr;
			i2c_reg_array.reg_data = 0;
			i2c_reg_array.delay = emap[j].pageen.delay;
			i2c_reg_settings.reg_setting = &i2c_reg_array;
			rc = camera_io_dev_write(&e_ctrl->io_master_info,
				&i2c_reg_settings);
			if (rc < 0) {
				CAM_ERR(CAM_EEPROM,
					"page disable failed rc %d",
					rc);
				return rc;
			}
		}
	}
	return rc;
}

/**
 * cam_eeprom_power_up - Power up eeprom hardware
 * @e_ctrl:     ctrl structure
 * @power_info: power up/down info for eeprom
 *
 * Returns success or failure
 */
static int cam_eeprom_power_up(struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_sensor_power_ctrl_t *power_info)
{
	int32_t                 rc = 0;
	struct cam_hw_soc_info *soc_info =
		&e_ctrl->soc_info;

	/* Parse and fill vreg params for power up settings */
	rc = msm_camera_fill_vreg_params(
		&e_ctrl->soc_info,
		power_info->power_setting,
		power_info->power_setting_size);
	if (rc) {
		CAM_ERR(CAM_EEPROM,
			"failed to fill power up vreg params rc:%d", rc);
		return rc;
	}

	/* Parse and fill vreg params for power down settings*/
	rc = msm_camera_fill_vreg_params(
		&e_ctrl->soc_info,
		power_info->power_down_setting,
		power_info->power_down_setting_size);
	if (rc) {
		CAM_ERR(CAM_EEPROM,
			"failed to fill power down vreg params  rc:%d", rc);
		return rc;
	}

	power_info->dev = soc_info->dev;

	rc = cam_sensor_core_power_up(power_info, soc_info);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "failed in eeprom power up rc %d", rc);
		return rc;
	}

	if (e_ctrl->io_master_info.master_type == CCI_MASTER) {
		rc = camera_io_init(&(e_ctrl->io_master_info));
		if (rc) {
			CAM_ERR(CAM_EEPROM, "cci_init failed");
			return -EINVAL;
		}
	}
	msleep(20);
	return rc;
}

/**
 * cam_eeprom_power_down - Power down eeprom hardware
 * @e_ctrl:    ctrl structure
 *
 * Returns success or failure
 */
static int cam_eeprom_power_down(struct cam_eeprom_ctrl_t *e_ctrl)
{
	struct cam_sensor_power_ctrl_t *power_info;
	struct cam_hw_soc_info         *soc_info;
	struct cam_eeprom_soc_private  *soc_private;
	int                             rc = 0;

	if (!e_ctrl) {
		CAM_ERR(CAM_EEPROM, "failed: e_ctrl %pK", e_ctrl);
		return -EINVAL;
	}

	soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;
	soc_info = &e_ctrl->soc_info;

	if (!power_info) {
		CAM_ERR(CAM_EEPROM, "failed: power_info %pK", power_info);
		return -EINVAL;
	}
	rc = cam_sensor_util_power_down(power_info, soc_info);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "power down the core is failed:%d", rc);
		return rc;
	}

	if (e_ctrl->io_master_info.master_type == CCI_MASTER)
		camera_io_release(&(e_ctrl->io_master_info));

	return rc;
}

/**
 * cam_eeprom_match_id - match eeprom id
 * @e_ctrl:     ctrl structure
 *
 * Returns success or failure
 */
static int cam_eeprom_match_id(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int                      rc;
	struct camera_io_master *client = &e_ctrl->io_master_info;
	uint8_t                  id[2];

	rc = cam_spi_query_id(client, 0, CAMERA_SENSOR_I2C_TYPE_WORD,
		&id[0], 2);
	if (rc)
		return rc;
	CAM_DBG(CAM_EEPROM, "read 0x%x 0x%x, check 0x%x 0x%x",
		id[0], id[1], client->spi_client->mfr_id0,
		client->spi_client->device_id0);
	if (id[0] != client->spi_client->mfr_id0
		|| id[1] != client->spi_client->device_id0)
		return -ENODEV;
	return 0;
}

/**
 * cam_eeprom_parse_read_memory_map - Parse memory map
 * @of_node:    device node
 * @e_ctrl:     ctrl structure
 *
 * Returns success or failure
 */
int32_t cam_eeprom_parse_read_memory_map(struct device_node *of_node,
	struct cam_eeprom_ctrl_t *e_ctrl)
{
	int32_t                         rc = 0;
	struct cam_eeprom_soc_private  *soc_private;
	struct cam_sensor_power_ctrl_t *power_info;
	int i;
	int normal_crc_value = 0;

	if (!e_ctrl || !of_node) {
		CAM_ERR(CAM_EEPROM, "failed: e_ctrl is NULL");
		return -EINVAL;
	}

	soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;

	rc = cam_eeprom_parse_dt_memory_map(of_node, &e_ctrl->cal_data);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "failed: eeprom dt parse rc %d", rc);
		return rc;
	}
	rc = cam_eeprom_power_up(e_ctrl, power_info);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "failed: eeprom power up rc %d", rc);
		goto data_mem_free;
	}

	e_ctrl->cam_eeprom_state = CAM_EEPROM_CONFIG;
	if (e_ctrl->eeprom_device_type == MSM_CAMERA_SPI_DEVICE) {
		rc = cam_eeprom_match_id(e_ctrl);
		if (rc) {
			CAM_DBG(CAM_EEPROM, "eeprom not matching %d", rc);
			//goto power_down;
			rc = 0;
		}
	}

	normal_crc_value = 0;
	for (i = 0; i < e_ctrl->cal_data.num_map>>1; i++)
		normal_crc_value |= (1 << i);

	CAMERA_NORMAL_CAL_CRC = normal_crc_value;
	CAM_INFO(CAM_EEPROM, "num_map = %d, CAMERA_NORMAL_CAL_CRC = 0x%X",
		e_ctrl->cal_data.num_map, CAMERA_NORMAL_CAL_CRC);

	rc = cam_eeprom_read_memory(e_ctrl, &e_ctrl->cal_data);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "read_eeprom_memory failed");
		goto power_down;
	}
	e_ctrl->is_supported |= cam_eeprom_match_crc(&e_ctrl->cal_data, e_ctrl->soc_info.index);

	if (e_ctrl->is_supported != normal_crc_value)
		CAM_ERR(CAM_EEPROM, "Any CRC values at F-ROM are not matched.");
	else
		CAM_INFO(CAM_EEPROM, "All CRC values are matched.");

	rc = cam_eeprom_update_module_info(e_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "cam_eeprom_update_module_info failed");
		goto power_down;
	}

#ifdef CAM_EEPROM_DBG_DUMP
	if (e_ctrl->soc_info.index == 1) {
		rc = cam_eeprom_dump(e_ctrl->soc_info.index, e_ctrl->cal_data.mapdata, 0x0000, 0x200);
	}
#endif
	rc = cam_eeprom_power_down(e_ctrl);
	if (rc < 0)
		CAM_ERR(CAM_EEPROM, "failed: eeprom power down rc %d", rc);

	e_ctrl->cam_eeprom_state = CAM_EEPROM_ACQUIRE;
	return rc;
power_down:
	cam_eeprom_power_down(e_ctrl);
data_mem_free:
	vfree(e_ctrl->cal_data.mapdata);
	vfree(e_ctrl->cal_data.map);
	e_ctrl->cal_data.num_data = 0;
	e_ctrl->cal_data.num_map = 0;
	e_ctrl->cam_eeprom_state = CAM_EEPROM_ACQUIRE;
	return rc;
}

/**
 * cam_eeprom_get_dev_handle - get device handle
 * @e_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int32_t cam_eeprom_get_dev_handle(struct cam_eeprom_ctrl_t *e_ctrl,
	void *arg)
{
	struct cam_sensor_acquire_dev    eeprom_acq_dev;
	struct cam_create_dev_hdl        bridge_params;
	struct cam_control              *cmd = (struct cam_control *)arg;

	if (e_ctrl->bridge_intf.device_hdl != -1) {
		CAM_ERR(CAM_EEPROM, "Device is already acquired");
		return -EFAULT;
	}
	if (copy_from_user(&eeprom_acq_dev,
		u64_to_user_ptr(cmd->handle),
		sizeof(eeprom_acq_dev))) {
		CAM_ERR(CAM_EEPROM,
			"EEPROM:ACQUIRE_DEV: copy from user failed");
		return -EFAULT;
	}

	bridge_params.session_hdl = eeprom_acq_dev.session_handle;
	bridge_params.ops = &e_ctrl->bridge_intf.ops;
	bridge_params.v4l2_sub_dev_flag = 0;
	bridge_params.media_entity_flag = 0;
	bridge_params.priv = e_ctrl;

	eeprom_acq_dev.device_handle =
		cam_create_device_hdl(&bridge_params);
	e_ctrl->bridge_intf.device_hdl = eeprom_acq_dev.device_handle;
	e_ctrl->bridge_intf.session_hdl = eeprom_acq_dev.session_handle;

	CAM_DBG(CAM_EEPROM, "Device Handle: %d", eeprom_acq_dev.device_handle);
	if (copy_to_user(u64_to_user_ptr(cmd->handle),
		&eeprom_acq_dev, sizeof(struct cam_sensor_acquire_dev))) {
		CAM_ERR(CAM_EEPROM, "EEPROM:ACQUIRE_DEV: copy to user failed");
		return -EFAULT;
	}
	return 0;
}

/**
 * cam_eeprom_update_slaveInfo - Update slave info
 * @e_ctrl:     ctrl structure
 * @cmd_buf:    command buffer
 *
 * Returns success or failure
 */
static int32_t cam_eeprom_update_slaveInfo(struct cam_eeprom_ctrl_t *e_ctrl,
	void *cmd_buf)
{
	int32_t                         rc = 0;
	struct cam_eeprom_soc_private  *soc_private;
	struct cam_cmd_i2c_info        *cmd_i2c_info = NULL;

	soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	cmd_i2c_info = (struct cam_cmd_i2c_info *)cmd_buf;
	soc_private->i2c_info.slave_addr = cmd_i2c_info->slave_addr;
	soc_private->i2c_info.i2c_freq_mode = cmd_i2c_info->i2c_freq_mode;

	rc = cam_eeprom_update_i2c_info(e_ctrl,
		&soc_private->i2c_info);
	CAM_DBG(CAM_EEPROM, "Slave addr: 0x%x Freq Mode: %d",
		soc_private->i2c_info.slave_addr,
		soc_private->i2c_info.i2c_freq_mode);

	return rc;
}

/**
 * cam_eeprom_parse_memory_map - Parse memory map info
 * @data:             memory block data
 * @cmd_buf:          command buffer
 * @cmd_length:       command buffer length
 * @num_map:          memory map size
 * @cmd_length_bytes: command length processed in this function
 *
 * Returns success or failure
 */
static int32_t cam_eeprom_parse_memory_map(
	struct cam_eeprom_memory_block_t *data,
	void *cmd_buf, int cmd_length, uint32_t *cmd_length_bytes,
	int *num_map, size_t remain_buf_len)
{
	int32_t                            rc = 0;
	int32_t                            cnt = 0;
	int32_t                            processed_size = 0;
	uint8_t                            generic_op_code;
	struct cam_eeprom_memory_map_t    *map = data->map;
	struct common_header              *cmm_hdr =
		(struct common_header *)cmd_buf;
	uint16_t                           cmd_length_in_bytes = 0;
	struct cam_cmd_i2c_random_wr      *i2c_random_wr = NULL;
	struct cam_cmd_i2c_continuous_rd  *i2c_cont_rd = NULL;
	struct cam_cmd_conditional_wait   *i2c_poll = NULL;
	struct cam_cmd_unconditional_wait *i2c_uncond_wait = NULL;
	size_t                             validate_size = 0;

	generic_op_code = cmm_hdr->fifth_byte;

	if (cmm_hdr->cmd_type == CAMERA_SENSOR_CMD_TYPE_I2C_RNDM_WR)
		validate_size = sizeof(struct cam_cmd_i2c_random_wr);
	else if (cmm_hdr->cmd_type == CAMERA_SENSOR_CMD_TYPE_I2C_CONT_RD)
		validate_size = sizeof(struct cam_cmd_i2c_continuous_rd);
	else if (cmm_hdr->cmd_type == CAMERA_SENSOR_CMD_TYPE_WAIT)
		validate_size = sizeof(struct cam_cmd_unconditional_wait);

	if (remain_buf_len < validate_size ||
	    *num_map >= (MSM_EEPROM_MAX_MEM_MAP_CNT *
		MSM_EEPROM_MEMORY_MAP_MAX_SIZE)) {
		CAM_ERR(CAM_EEPROM, "not enough buffer");
		return -EINVAL;
	}
	switch (cmm_hdr->cmd_type) {
	case CAMERA_SENSOR_CMD_TYPE_I2C_RNDM_WR:
		i2c_random_wr = (struct cam_cmd_i2c_random_wr *)cmd_buf;

		if (i2c_random_wr->header.count == 0 ||
		    i2c_random_wr->header.count >= MSM_EEPROM_MAX_MEM_MAP_CNT ||
		    (size_t)*num_map >= ((MSM_EEPROM_MAX_MEM_MAP_CNT *
				MSM_EEPROM_MEMORY_MAP_MAX_SIZE) -
				i2c_random_wr->header.count)) {
			CAM_ERR(CAM_EEPROM, "OOB Error");
			return -EINVAL;
		}
		cmd_length_in_bytes   = sizeof(struct cam_cmd_i2c_random_wr) +
			((i2c_random_wr->header.count - 1) *
			sizeof(struct i2c_random_wr_payload));

		if (cmd_length_in_bytes > remain_buf_len) {
			CAM_ERR(CAM_EEPROM, "Not enough buffer remaining");
			return -EINVAL;
		}
		for (cnt = 0; cnt < (i2c_random_wr->header.count);
			cnt++) {
			map[*num_map + cnt].page.addr =
				i2c_random_wr->random_wr_payload[cnt].reg_addr;
			map[*num_map + cnt].page.addr_type =
				i2c_random_wr->header.addr_type;
			map[*num_map + cnt].page.data =
				i2c_random_wr->random_wr_payload[cnt].reg_data;
			map[*num_map + cnt].page.data_type =
				i2c_random_wr->header.data_type;
			map[*num_map + cnt].page.valid_size = 1;
		}

		*num_map += (i2c_random_wr->header.count - 1);
		cmd_buf += cmd_length_in_bytes / sizeof(int32_t);
		processed_size +=
			cmd_length_in_bytes;
		break;
	case CAMERA_SENSOR_CMD_TYPE_I2C_CONT_RD:
		i2c_cont_rd = (struct cam_cmd_i2c_continuous_rd *)cmd_buf;
		cmd_length_in_bytes = sizeof(struct cam_cmd_i2c_continuous_rd);

		if (i2c_cont_rd->header.count >= U32_MAX - data->num_data) {
			CAM_ERR(CAM_EEPROM,
				"int overflow on eeprom memory block");
			return -EINVAL;
		}
		map[*num_map].mem.addr = i2c_cont_rd->reg_addr;
		map[*num_map].mem.addr_type = i2c_cont_rd->header.addr_type;
		map[*num_map].mem.data_type = i2c_cont_rd->header.data_type;
		map[*num_map].mem.valid_size =
			i2c_cont_rd->header.count;
		cmd_buf += cmd_length_in_bytes / sizeof(int32_t);
		processed_size +=
			cmd_length_in_bytes;
		data->num_data += map[*num_map].mem.valid_size;
		break;
	case CAMERA_SENSOR_CMD_TYPE_WAIT:
		if (generic_op_code ==
			CAMERA_SENSOR_WAIT_OP_HW_UCND ||
			generic_op_code ==
			CAMERA_SENSOR_WAIT_OP_SW_UCND) {
			i2c_uncond_wait =
				(struct cam_cmd_unconditional_wait *)cmd_buf;
			cmd_length_in_bytes =
				sizeof(struct cam_cmd_unconditional_wait);

			if (*num_map < 1) {
				CAM_ERR(CAM_EEPROM,
					"invalid map number, num_map=%d",
					*num_map);
				return -EINVAL;
			}

			/*
			 * Though delay is added all of them, but delay will
			 * be applicable to only one of them as only one of
			 * them will have valid_size set to >= 1.
			 */
			map[*num_map - 1].mem.delay = i2c_uncond_wait->delay;
			map[*num_map - 1].page.delay = i2c_uncond_wait->delay;
			map[*num_map - 1].pageen.delay = i2c_uncond_wait->delay;
		} else if (generic_op_code ==
			CAMERA_SENSOR_WAIT_OP_COND) {
			i2c_poll = (struct cam_cmd_conditional_wait *)cmd_buf;
			cmd_length_in_bytes =
				sizeof(struct cam_cmd_conditional_wait);

			map[*num_map].poll.addr = i2c_poll->reg_addr;
			map[*num_map].poll.addr_type = i2c_poll->addr_type;
			map[*num_map].poll.data = i2c_poll->reg_data;
			map[*num_map].poll.data_type = i2c_poll->data_type;
			map[*num_map].poll.delay = i2c_poll->timeout;
			map[*num_map].poll.valid_size = 1;
		}
		cmd_buf += cmd_length_in_bytes / sizeof(int32_t);
		processed_size +=
			cmd_length_in_bytes;
		break;
	default:
		break;
	}

	*cmd_length_bytes = processed_size;
	return rc;
}

static struct i2c_settings_list*
	cam_eeprom_get_i2c_ptr(struct i2c_settings_array *i2c_reg_settings,
		uint32_t size)
{
	struct i2c_settings_list *tmp;

	tmp = kzalloc(sizeof(struct i2c_settings_list), GFP_KERNEL);

	if (tmp != NULL)
		list_add_tail(&(tmp->list),
			&(i2c_reg_settings->list_head));
	else
		return NULL;

	tmp->seq_settings.reg_data =
		kcalloc(size, sizeof(uint8_t), GFP_KERNEL);
	if (tmp->seq_settings.reg_data == NULL) {
		list_del(&(tmp->list));
		kfree(tmp);
		tmp = NULL;
		return NULL;
	}
	tmp->seq_settings.size = size;

	return tmp;
}

static int32_t cam_eeprom_handle_continuous_write(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_cmd_i2c_continuous_wr *cam_cmd_i2c_continuous_wr,
	struct i2c_settings_array *i2c_reg_settings,
	uint32_t *cmd_length_in_bytes, int32_t *offset,
	struct list_head **list)
{
	struct i2c_settings_list *i2c_list;
	int32_t rc = 0, cnt = 0;


	CAM_DBG(CAM_EEPROM, "Total Size: %d",
		cam_cmd_i2c_continuous_wr->header.count);

	i2c_list = cam_eeprom_get_i2c_ptr(i2c_reg_settings,
		cam_cmd_i2c_continuous_wr->header.count);
	if (i2c_list == NULL ||
		i2c_list->seq_settings.reg_data == NULL) {
		CAM_ERR(CAM_SENSOR, "Failed in allocating i2c_list");
		return -ENOMEM;
	}

	*cmd_length_in_bytes = (sizeof(struct i2c_rdwr_header) +
		sizeof(cam_cmd_i2c_continuous_wr->reg_addr) +
		sizeof(struct cam_cmd_read) *
		(cam_cmd_i2c_continuous_wr->header.count));
	if (cam_cmd_i2c_continuous_wr->header.op_code ==
		CAMERA_SENSOR_I2C_OP_CONT_WR_BRST)
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
	else if (cam_cmd_i2c_continuous_wr->header.op_code ==
		CAMERA_SENSOR_I2C_OP_CONT_WR_SEQN)
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_SEQ;
	else {
		rc = -EINVAL;
		goto deallocate_i2c_list;
	}

	i2c_list->seq_settings.addr_type =
		cam_cmd_i2c_continuous_wr->header.addr_type;

	CAM_ERR(CAM_EEPROM, "Write Address: 0x%x",
		cam_cmd_i2c_continuous_wr->reg_addr);
	if (i2c_list->op_code == CAM_SENSOR_I2C_WRITE_SEQ) {
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
		e_ctrl->eebin_info.start_address =
			cam_cmd_i2c_continuous_wr->reg_addr;
		e_ctrl->eebin_info.size =
			cam_cmd_i2c_continuous_wr->header.count;
		CAM_DBG(CAM_EEPROM, "Header Count: %d",
			cam_cmd_i2c_continuous_wr->header.count);
		e_ctrl->eebin_info.is_valid = 1;

		i2c_list->seq_settings.reg_addr =
			cam_cmd_i2c_continuous_wr->reg_addr;
	} else
		CAM_ERR(CAM_EEPROM, "Burst Mode Not Supported\n");

	(*offset) += cnt;
	*list = &(i2c_list->list);
	return rc;
deallocate_i2c_list:
	kfree(i2c_list);
	return rc;
}

static int32_t cam_eeprom_handle_delay(
	uint32_t **cmd_buf,
	uint16_t generic_op_code,
	struct i2c_settings_array *i2c_reg_settings,
	uint32_t offset, uint32_t *byte_cnt,
	struct list_head *list_ptr,
	size_t remain_buf_len)
{
	int32_t rc = 0;
	struct i2c_settings_list *i2c_list = NULL;
	struct cam_cmd_unconditional_wait *cmd_uncond_wait =
		(struct cam_cmd_unconditional_wait *) *cmd_buf;

	if (remain_buf_len < (sizeof(struct cam_cmd_unconditional_wait))) {
		CAM_ERR(CAM_EEPROM, "Not Enough buffer");
		return -EINVAL;
	}

	if (list_ptr == NULL) {
		CAM_ERR(CAM_SENSOR, "Invalid list ptr");
		return -EINVAL;
	}

	if (offset > 0) {
		i2c_list =
			list_entry(list_ptr, struct i2c_settings_list, list);
		if (generic_op_code ==
			CAMERA_SENSOR_WAIT_OP_HW_UCND)
			i2c_list->i2c_settings.reg_setting[offset - 1].delay =
				cmd_uncond_wait->delay;
		else
			i2c_list->i2c_settings.delay =
				cmd_uncond_wait->delay;
		(*cmd_buf) +=
			sizeof(
			struct cam_cmd_unconditional_wait) / sizeof(uint32_t);
		(*byte_cnt) +=
			sizeof(
			struct cam_cmd_unconditional_wait);
	} else {
		CAM_ERR(CAM_SENSOR, "Delay Rxed before any buffer: %d", offset);
		return -EINVAL;
	}

	return rc;
}

/**
 * cam_eeprom_parse_write_memory_packet - Write eeprom packet
 * @csl_packet:   csl packet received
 * @e_ctrl:       ctrl structure
 *
 * Returns success or failure
 */
static int32_t cam_eeprom_parse_write_memory_packet(
	struct cam_packet *csl_packet,
	struct cam_eeprom_ctrl_t *e_ctrl)
{
	struct cam_cmd_buf_desc        *cmd_desc = NULL;
	uint32_t                       *offset = NULL;
	int32_t                         i, rc = 0;
	uint32_t                       *cmd_buf = NULL;
	uintptr_t                       generic_pkt_addr;
	size_t                          pkt_len = 0;
	size_t                          remain_len = 0;
	uint32_t                        total_cmd_buf_in_bytes = 0;
	uint32_t                        processed_cmd_buf_in_bytes = 0;
	struct common_header           *cmm_hdr = NULL;
	uint32_t                        cmd_length_in_bytes = 0;
	struct cam_cmd_i2c_info        *i2c_info = NULL;


	offset = (uint32_t *)&csl_packet->payload;
	offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
	cmd_desc = (struct cam_cmd_buf_desc *)(offset);

	CAM_DBG(CAM_EEPROM, "Number of Command Buffers: %d",
		csl_packet->num_cmd_buf);
	for (i = 0; i < csl_packet->num_cmd_buf; i++) {
		struct list_head               *list = NULL;
		uint16_t                       generic_op_code;
		uint32_t                       off = 0;
		int                            master;
		struct cam_sensor_cci_client   *cci;

		total_cmd_buf_in_bytes = cmd_desc[i].length;
		processed_cmd_buf_in_bytes = 0;

		if (!total_cmd_buf_in_bytes)
			continue;

		rc = cam_mem_get_cpu_buf(cmd_desc[i].mem_handle,
			&generic_pkt_addr, &pkt_len);
		if (rc) {
			CAM_ERR(CAM_EEPROM, "Failed to get cpu buf");
			return rc;
		}

		cmd_buf = (uint32_t *)generic_pkt_addr;
		if (!cmd_buf) {
			CAM_ERR(CAM_EEPROM, "invalid cmd buf");
			rc = -EINVAL;
			goto end;
		}

		if ((pkt_len < sizeof(struct common_header) ||
			(cmd_desc[i].offset) > (pkt_len -
			sizeof(struct common_header)))) {
			CAM_ERR(CAM_EEPROM, "Not enough buffer");
			rc = -EINVAL;
			goto end;
		}

		remain_len = pkt_len - cmd_desc[i].offset;
		cmd_buf += cmd_desc[i].offset / sizeof(uint32_t);

		if (total_cmd_buf_in_bytes > remain_len) {
			CAM_ERR(CAM_EEPROM, "Not enough buffer for command");
			rc = -EINVAL;
			goto end;
		}

		master = e_ctrl->io_master_info.master_type;
		cci = e_ctrl->io_master_info.cci_client;
		while (processed_cmd_buf_in_bytes < total_cmd_buf_in_bytes) {
			if ((remain_len - processed_cmd_buf_in_bytes) <
				sizeof(struct common_header)) {
				CAM_ERR(CAM_EEPROM, "Not Enough buffer");
				rc = -EINVAL;
				goto end;
			}
			cmm_hdr = (struct common_header *)cmd_buf;
			generic_op_code = cmm_hdr->fifth_byte;

			switch (cmm_hdr->cmd_type) {
			case CAMERA_SENSOR_CMD_TYPE_I2C_INFO:
				i2c_info = (struct cam_cmd_i2c_info *)cmd_buf;
				if ((remain_len - processed_cmd_buf_in_bytes) <
					sizeof(struct cam_cmd_i2c_info)) {
					CAM_ERR(CAM_EEPROM, "Not enough buf");
					rc = -EINVAL;
					goto end;
				}
				if (master == CCI_MASTER) {
					cci->cci_i2c_master =
						e_ctrl->cci_i2c_master;
					cci->i2c_freq_mode =
						i2c_info->i2c_freq_mode;
					cci->sid =
						i2c_info->slave_addr >> 1;
					CAM_DBG(CAM_EEPROM,
						"Slave addr: 0x%x Freq Mode: %d",
						i2c_info->slave_addr,
						i2c_info->i2c_freq_mode);
				} else if (master == I2C_MASTER) {
					e_ctrl->io_master_info.client->addr =
						i2c_info->slave_addr;
					CAM_DBG(CAM_EEPROM,
						"Slave addr: 0x%x",
						i2c_info->slave_addr);
				} else if (master == SPI_MASTER) {
					CAM_ERR(CAM_EEPROM,
						"No Need of Slave Info");
				} else {
					CAM_ERR(CAM_EEPROM,
						"Invalid Master type: %d",
						master);
					rc = -EINVAL;
					goto end;
				}
				cmd_length_in_bytes =
					sizeof(struct cam_cmd_i2c_info);
				processed_cmd_buf_in_bytes +=
					cmd_length_in_bytes;
				cmd_buf += cmd_length_in_bytes/4;
				break;
			case CAMERA_SENSOR_CMD_TYPE_I2C_CONT_WR: {
				struct cam_cmd_i2c_continuous_wr
					*cam_cmd_i2c_continuous_wr =
					(struct cam_cmd_i2c_continuous_wr *)
					cmd_buf;
				if ((remain_len - processed_cmd_buf_in_bytes) <
				sizeof(struct cam_cmd_i2c_continuous_wr)) {
					CAM_ERR(CAM_EEPROM, "Not enough buf");
					rc = -EINVAL;
					goto end;
				}

				CAM_DBG(CAM_EEPROM,
					"CAMERA_SENSOR_CMD_TYPE_I2C_CONT_WR");
				rc = cam_eeprom_handle_continuous_write(
					e_ctrl,
					cam_cmd_i2c_continuous_wr,
					&(e_ctrl->wr_settings),
					&cmd_length_in_bytes, &off, &list);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR,
					"Failed in continuous write %d", rc);
					goto end;
				}

				processed_cmd_buf_in_bytes +=
					cmd_length_in_bytes;
				cmd_buf += cmd_length_in_bytes /
					sizeof(uint32_t);
				break;
			}
			case CAMERA_SENSOR_CMD_TYPE_WAIT: {
				CAM_DBG(CAM_EEPROM,
					"CAMERA_SENSOR_CMD_TYPE_WAIT");
				if (generic_op_code ==
					CAMERA_SENSOR_WAIT_OP_HW_UCND ||
					generic_op_code ==
						CAMERA_SENSOR_WAIT_OP_SW_UCND) {

					rc = cam_eeprom_handle_delay(
						&cmd_buf, generic_op_code,
						&(e_ctrl->wr_settings), off,
						&cmd_length_in_bytes,
						list, (remain_len -
						processed_cmd_buf_in_bytes));
					if (rc < 0) {
						CAM_ERR(CAM_EEPROM,
							"delay hdl failed: %d",
							rc);
						goto end;
					}
					processed_cmd_buf_in_bytes +=
						cmd_length_in_bytes;
					cmd_buf += cmd_length_in_bytes /
					sizeof(uint32_t);
				} else {
					CAM_ERR(CAM_EEPROM,
						"Wrong Wait Command: %d",
						generic_op_code);
					rc = -EINVAL;
					goto end;
				}
				break;
			}
			default:
				CAM_ERR(CAM_EEPROM,
					"Invalid Cmd_type rxed: %d\n",
					cmm_hdr->cmd_type);
				rc = -EINVAL;
				break;
			}
		}
	}

end:
	return rc;
}

/**
 * cam_eeprom_calc_calmap_size - Calculate cal array size based on the cal map
 * @e_ctrl:       ctrl structure
 *
 * Returns size of cal array
 */
static int32_t cam_eeprom_calc_calmap_size(struct cam_eeprom_ctrl_t *e_ctrl)
{
	struct cam_eeprom_memory_map_t    *map = NULL;
	uint32_t minMap, maxMap, minLocal, maxLocal;
	int32_t i;
	int32_t calmap_size = 0;

	if (e_ctrl == NULL ||
		(e_ctrl->cal_data.num_map == 0) ||
		(e_ctrl->cal_data.map == NULL)) {
		CAM_INFO(CAM_EEPROM, "cal size is wrong");
		return calmap_size;
	}

	map = e_ctrl->cal_data.map;
	minMap = minLocal = 0xFFFFFFFF;
	maxMap = maxLocal = 0x00;

	for (i = 0; i < e_ctrl->cal_data.num_map; i++) {
		minLocal = map[i].mem.addr;
		maxLocal = minLocal + map[i].mem.valid_size;

		if(minMap > minLocal)
		{
			minMap = minLocal;
		}

		if(maxMap < maxLocal)
		{
			maxMap = maxLocal;
		}

		CAM_INFO(CAM_EEPROM, "[%d / %d] minLocal = 0x%X, minMap = 0x%X, maxLocal = 0x%X, maxMap = 0x%X",
			i+1, e_ctrl->cal_data.num_map, minLocal, minMap, maxLocal, maxMap);
	}
	calmap_size = maxMap - minMap;

	CAM_INFO(CAM_EEPROM, "calmap_size = 0x%X, minMap = 0x%X, maxMap = 0x%X",
		calmap_size, minMap, maxMap);

	return calmap_size;
}

/**
 * cam_eeprom_init_pkt_parser - Parse eeprom packet
 * @e_ctrl:       ctrl structure
 * @csl_packet:	  csl packet received
 *
 * Returns success or failure
 */
static int32_t cam_eeprom_init_pkt_parser(struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_packet *csl_packet)
{
	int32_t                         rc = 0;
	int                             i = 0;
	struct cam_cmd_buf_desc        *cmd_desc = NULL;
	uint32_t                       *offset = NULL;
	uint32_t                       *cmd_buf = NULL;
	uintptr_t                        generic_pkt_addr;
	size_t                          pkt_len = 0;
	size_t                          remain_len = 0;
	uint32_t                        total_cmd_buf_in_bytes = 0;
	uint32_t                        processed_cmd_buf_in_bytes = 0;
	struct common_header           *cmm_hdr = NULL;
	uint32_t                        cmd_length_in_bytes = 0;
	struct cam_cmd_i2c_info        *i2c_info = NULL;
	int                             num_map = -1;
	struct cam_eeprom_memory_map_t *map = NULL;
	struct cam_eeprom_soc_private  *soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t *power_info = &soc_private->power_info;

	e_ctrl->cal_data.map = vzalloc((MSM_EEPROM_MEMORY_MAP_MAX_SIZE *
		MSM_EEPROM_MAX_MEM_MAP_CNT) *
		(sizeof(struct cam_eeprom_memory_map_t)));
	if (!e_ctrl->cal_data.map) {
		rc = -ENOMEM;
		CAM_ERR(CAM_EEPROM, "failed");
		return rc;
	}
	map = e_ctrl->cal_data.map;

	offset = (uint32_t *)&csl_packet->payload;
	offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
	cmd_desc = (struct cam_cmd_buf_desc *)(offset);

	/* Loop through multiple command buffers */
	for (i = 0; i < csl_packet->num_cmd_buf; i++) {
		total_cmd_buf_in_bytes = cmd_desc[i].length;
		processed_cmd_buf_in_bytes = 0;
		if (!total_cmd_buf_in_bytes)
			continue;
		rc = cam_mem_get_cpu_buf(cmd_desc[i].mem_handle,
			&generic_pkt_addr, &pkt_len);
		if (rc) {
			CAM_ERR(CAM_EEPROM, "Failed to get cpu buf");
			return rc;
		}
		cmd_buf = (uint32_t *)generic_pkt_addr;
		if (!cmd_buf) {
			CAM_ERR(CAM_EEPROM, "invalid cmd buf");
			rc = -EINVAL;
			goto end;
		}

		if ((pkt_len < sizeof(struct common_header)) ||
			(cmd_desc[i].offset > (pkt_len -
			sizeof(struct common_header)))) {
			CAM_ERR(CAM_EEPROM, "Not enough buffer");
			rc = -EINVAL;
			goto end;
		}
		remain_len = pkt_len - cmd_desc[i].offset;
		cmd_buf += cmd_desc[i].offset / sizeof(uint32_t);

		if (total_cmd_buf_in_bytes > remain_len) {
			CAM_ERR(CAM_EEPROM, "Not enough buffer for command");
			rc = -EINVAL;
			goto end;
		}
		/* Loop through multiple cmd formats in one cmd buffer */
		while (processed_cmd_buf_in_bytes < total_cmd_buf_in_bytes) {
			if ((remain_len - processed_cmd_buf_in_bytes) <
				sizeof(struct common_header)) {
				CAM_ERR(CAM_EEPROM, "Not enough buf");
				rc = -EINVAL;
				goto end;
			}
			cmm_hdr = (struct common_header *)cmd_buf;
			switch (cmm_hdr->cmd_type) {
			case CAMERA_SENSOR_CMD_TYPE_I2C_INFO:
				i2c_info = (struct cam_cmd_i2c_info *)cmd_buf;
				if ((remain_len - processed_cmd_buf_in_bytes) <
					sizeof(struct cam_cmd_i2c_info)) {
					CAM_ERR(CAM_EEPROM, "Not enough buf");
					rc = -EINVAL;
					goto end;
				}
				/* Configure the following map slave address */
				map[num_map + 1].saddr = i2c_info->slave_addr;
				rc = cam_eeprom_update_slaveInfo(e_ctrl,
					cmd_buf);
				cmd_length_in_bytes =
					sizeof(struct cam_cmd_i2c_info);
				processed_cmd_buf_in_bytes +=
					cmd_length_in_bytes;
				cmd_buf += cmd_length_in_bytes/
					sizeof(uint32_t);
				break;
			case CAMERA_SENSOR_CMD_TYPE_PWR_UP:
			case CAMERA_SENSOR_CMD_TYPE_PWR_DOWN:
				cmd_length_in_bytes = total_cmd_buf_in_bytes;
				rc = cam_sensor_update_power_settings(cmd_buf,
					cmd_length_in_bytes, power_info,
					(remain_len -
					processed_cmd_buf_in_bytes));
				processed_cmd_buf_in_bytes +=
					cmd_length_in_bytes;
				cmd_buf += cmd_length_in_bytes/
					sizeof(uint32_t);
				if (rc) {
					CAM_ERR(CAM_EEPROM, "Failed");
					goto end;
				}
				break;
			case CAMERA_SENSOR_CMD_TYPE_I2C_RNDM_WR:
			case CAMERA_SENSOR_CMD_TYPE_I2C_CONT_RD:
			case CAMERA_SENSOR_CMD_TYPE_WAIT:
				num_map++;
				rc = cam_eeprom_parse_memory_map(
					&e_ctrl->cal_data, cmd_buf,
					total_cmd_buf_in_bytes,
					&cmd_length_in_bytes, &num_map,
					(remain_len -
					processed_cmd_buf_in_bytes));
				processed_cmd_buf_in_bytes +=
					cmd_length_in_bytes;
				cmd_buf += cmd_length_in_bytes/sizeof(uint32_t);
				break;
			default:
				CAM_ERR(CAM_EEPROM, "Invalid cmd_type 0x%x",
					cmm_hdr->cmd_type);
				rc = -EINVAL;
				goto end;
			}
		}
		e_ctrl->cal_data.num_map = num_map + 1;
	}

end:
	return rc;
}

/**
 * cam_eeprom_get_cal_data - parse the userspace IO config and
 *                                        copy read data to share with userspace
 * @e_ctrl:     ctrl structure
 * @csl_packet: csl packet received
 *
 * Returns success or failure
 */
static int32_t cam_eeprom_get_cal_data(struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_packet *csl_packet)
{
	struct cam_buf_io_cfg *io_cfg;
	uint32_t              i = 0;
	int                   rc = 0;
	uintptr_t              buf_addr;
	size_t                buf_size;
	uint8_t               *read_buffer;
	size_t                remain_len = 0;

	io_cfg = (struct cam_buf_io_cfg *) ((uint8_t *)
		&csl_packet->payload +
		csl_packet->io_configs_offset);

	CAM_DBG(CAM_EEPROM, "number of IO configs: %d:",
		csl_packet->num_io_configs);

	for (i = 0; i < csl_packet->num_io_configs; i++) {
		CAM_DBG(CAM_EEPROM, "Direction: %d:", io_cfg->direction);
		if (io_cfg->direction == CAM_BUF_OUTPUT) {
			rc = cam_mem_get_cpu_buf(io_cfg->mem_handle[0],
				&buf_addr, &buf_size);
			if (rc) {
				CAM_ERR(CAM_EEPROM, "Fail in get buffer: %d",
					rc);
				return rc;
			}
			if (buf_size <= io_cfg->offsets[0]) {
				CAM_ERR(CAM_EEPROM, "Not enough buffer");
				rc = -EINVAL;
				return rc;
			}

			remain_len = buf_size - io_cfg->offsets[0];
			CAM_DBG(CAM_EEPROM, "buf_addr : %pK, buf_size : %zu\n",
				(void *)buf_addr, buf_size);

			read_buffer = (uint8_t *)buf_addr;
			if (!read_buffer) {
				CAM_ERR(CAM_EEPROM,
					"invalid buffer to copy data");
				rc = -EINVAL;
				return rc;
			}
			read_buffer += io_cfg->offsets[0];

			if (remain_len < e_ctrl->cal_data.num_data) {
				CAM_ERR(CAM_EEPROM,
					"failed to copy, Invalid size");
				rc = -EINVAL;
				return rc;
			}

			CAM_DBG(CAM_EEPROM, "copy the data, len:%d",
				e_ctrl->cal_data.num_data);
			memcpy(read_buffer, e_ctrl->cal_data.mapdata,
					e_ctrl->cal_data.num_data);
		} else {
			CAM_ERR(CAM_EEPROM, "Invalid direction");
			rc = -EINVAL;
		}
	}

	return rc;
}

static int32_t cam_eeprom_fill_configInfo(char *configString, uint32_t value, ConfigInfo_t *ConfigInfo)
{
	int32_t i, ret = 1;

	for(i = 0; i < MAX_CONFIG_INFO_IDX; i ++)
	{
		if(ConfigInfo[i].isSet == 1)
			continue;

		if(!strcmp(configString, ConfigInfoStrs[i]))
		{
			ConfigInfo[i].isSet = 1;
			ConfigInfo[i].value = value;
			ret = 0;

			switch(i)
			{
				case DEF_M_CORE_VER:
					memset(M_HW_INFO, 0x00, HW_INFO_MAX_SIZE);
					M_HW_INFO[0] = (value) & 0xFF;

					memset(S_HW_INFO, 0x00, HW_INFO_MAX_SIZE);
					S_HW_INFO[0] = (value) & 0xFF;

					if((value>>16) & 0xFF)
					{
						S_HW_INFO[0] = (value>>16) & 0xFF;
						CAM_DBG(CAM_EEPROM, "value: 0x%08X, S_HW_INFO[0]: 0x%02X", value, S_HW_INFO[0]);
					}

					break;

				case DEF_M_VER_HW:
					M_HW_INFO[1] = (value >> 24) & 0xFF;
					M_HW_INFO[2] = (value >> 16) & 0xFF;
					M_HW_INFO[3] = (value >>  8) & 0xFF;
					M_HW_INFO[4] = (value)       & 0xFF;

					CAM_DBG(CAM_EEPROM, "M_HW_INFO: %c %c%c %c %c",
						M_HW_INFO[0], M_HW_INFO[1], M_HW_INFO[2], M_HW_INFO[3], M_HW_INFO[4]);
					break;

				case DEF_M_VER_SW:
					memset(M_SW_INFO, 0x00, SW_INFO_MAX_SIZE);
					M_SW_INFO[0] = (value >> 24) & 0xFF;
					M_SW_INFO[1] = (value >> 16) & 0xFF;
					M_SW_INFO[2] = (value >>  8) & 0xFF;
					M_SW_INFO[3] = (value)       & 0xFF;

					CAM_DBG(CAM_EEPROM, "M_SW_INFO: %c %c %c%c",
						M_SW_INFO[0], M_SW_INFO[1], M_SW_INFO[2], M_SW_INFO[3]);

					memset(S_SW_INFO, 0x00, SW_INFO_MAX_SIZE);
					S_SW_INFO[0] = (value >> 24) & 0xFF;
					S_SW_INFO[1] = (value >> 16) & 0xFF;
					S_SW_INFO[2] = (value >>  8) & 0xFF;
					S_SW_INFO[3] = (value)       & 0xFF;
					break;

				case DEF_M_VER_ETC:
					memset(M_VENDOR_INFO, 0x00, VENDOR_INFO_MAX_SIZE);
					memset(M_PROCESS_INFO, 0x00, PROCESS_INFO_MAX_SIZE);
					M_VENDOR_INFO[0] = (value >> 24) & 0xFF;
					M_PROCESS_INFO[0] = (value >> 16) & 0xFF;

					CAM_DBG(CAM_EEPROM, "M_ETC_VER: %c %c",
						M_VENDOR_INFO[0], M_PROCESS_INFO[0]);

					memset(S_VENDOR_INFO, 0x00, VENDOR_INFO_MAX_SIZE);
					memset(S_PROCESS_INFO, 0x00, PROCESS_INFO_MAX_SIZE);
					S_VENDOR_INFO[0] = (value >> 24) & 0xFF;
					S_PROCESS_INFO[0] = (value >> 16) & 0xFF;
					break;

				case DEF_S_VER_HW:
					S_HW_INFO[1] = (value >> 24) & 0xFF;
					S_HW_INFO[2] = (value >> 16) & 0xFF;
					S_HW_INFO[3] = (value >>  8) & 0xFF;
					S_HW_INFO[4] = (value)       & 0xFF;

					CAM_DBG(CAM_EEPROM, "S_HW_INFO: %c %c%c %c %c",
						S_HW_INFO[0], S_HW_INFO[1], S_HW_INFO[2], S_HW_INFO[3], S_HW_INFO[4]);
					break;

				case DEF_M_CHK_VER:
					CriterionRev    = (value >> 24) & 0xFF;
					ModuleVerOnPVR  = (value >> 16) & 0xFF;
					ModuleVerOnSRA  = (value >>  8) & 0xFF;
					minCalMapVer    = ((value     ) & 0xFF) + '0';

					CAM_DBG(CAM_EEPROM, "value: 0x%08X, CriterionRev: %d, ModuleVerOnPVR: %c, ModuleVerOnSRA: %c, minCalMapVer: %d",
						value, CriterionRev, ModuleVerOnPVR, ModuleVerOnSRA, minCalMapVer);
					break;

				default:
					break;
			}
		}
	}

	return ret;
}


/**
 * cam_eeprom_get_customInfo - parse the userspace IO config and
 *                            read phone version at eebindriver.bin
 * @e_ctrl:     ctrl structure
 * @csl_packet: csl packet received
 *
 * Returns success or failure
 */
static int32_t cam_eeprom_get_customInfo(struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_packet *csl_packet)
{
	struct cam_buf_io_cfg *io_cfg;
	uint32_t              i = 0;
	int                   rc = 0;
	uintptr_t             buf_addr;
	size_t                buf_size = 0;
	uint8_t               *read_buffer;

	uint8_t               *pBuf = NULL;
	uint32_t              nConfig = 0;
	char                  *strConfigName = "CustomInfo";

	char                  configString[MaximumCustomStringLength] = "";
	uint32_t              configValue = 0;

	io_cfg = (struct cam_buf_io_cfg *) ((uint8_t *)
		&csl_packet->payload +
		csl_packet->io_configs_offset);

	CAM_INFO(CAM_EEPROM, "number of IO configs: %d:",
		csl_packet->num_io_configs);

	for (i = 0; i < csl_packet->num_io_configs; i++) {
		CAM_INFO(CAM_EEPROM, "Direction: %d:", io_cfg->direction);
		if (io_cfg->direction == CAM_BUF_OUTPUT) {
			rc = cam_mem_get_cpu_buf(io_cfg->mem_handle[0],
				&buf_addr, &buf_size);
			CAM_INFO(CAM_EEPROM, "buf_addr : %pK, buf_size : %zu",
				(void *)buf_addr, buf_size);

			read_buffer = (uint8_t *)buf_addr;
			if (!read_buffer) {
				CAM_ERR(CAM_EEPROM,
					"invalid buffer to copy data");
				return -EINVAL;
			}
			read_buffer += io_cfg->offsets[0];

			if (buf_size < e_ctrl->cal_data.num_data) {
				CAM_ERR(CAM_EEPROM,
					"failed to copy, Invalid size");
				return -EINVAL;
			}

			CAM_INFO(CAM_EEPROM, "copy the data, len:%d, read_buffer[0] = %d, read_buffer[4] = %d",
				e_ctrl->cal_data.num_data, read_buffer[0], read_buffer[4]);

			memset(&ConfigInfo, 0x00, sizeof(ConfigInfo_t) * MAX_CONFIG_INFO_IDX);

			pBuf = read_buffer;
			if(strcmp(pBuf, strConfigName) == 0) {
				pBuf += strlen(strConfigName)+1+sizeof(uint32_t);

				memcpy(&nConfig, pBuf, sizeof(uint32_t));
				pBuf += sizeof(uint32_t);

				CAM_ERR(CAM_EEPROM, "nConfig: %d", nConfig);
				for(i = 0; i < nConfig; i ++) {
					memcpy(configString, pBuf, MaximumCustomStringLength);
					pBuf += MaximumCustomStringLength;

					memcpy(&configValue, pBuf, sizeof(uint32_t));
					pBuf += sizeof(uint32_t);

#if 0
					CAM_ERR(CAM_EEPROM, "ConfigInfo[%d] = %s     0x%04X", i, configString, configValue);
#endif

					cam_eeprom_fill_configInfo(configString, configValue, ConfigInfo);
				}
			}

#if 0
			for(i = 0; i < MAX_CONFIG_INFO_IDX; i ++)
			{
				if(ConfigInfo[i].isSet == 1)
				{
					CAM_ERR(CAM_EEPROM, "ConfigInfo[%d] (%d) = %s     0x%04X",
						i, ConfigInfo[i].isSet, ConfigInfoStrs[i], ConfigInfo[i].value);
				}
			}
#endif

			memset(read_buffer, 0x00, e_ctrl->cal_data.num_data);
		} else {
			CAM_ERR(CAM_EEPROM, "Invalid direction");
			rc = -EINVAL;
		}
	}

	return rc;
}

/**
 * cam_eeprom_get_phone_ver - parse the userspace IO config and
 *                            read phone version at eebindriver.bin
 * @e_ctrl:     ctrl structure
 * @csl_packet: csl packet received
 *
 * Returns success or failure
 */

#define REAR_MODULE_FW_VERSION (0x50)
static int32_t cam_eeprom_get_phone_ver(struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_packet *csl_packet)
{
	(void) e_ctrl;
	(void) csl_packet;

	return 0;
#if 0
	struct cam_buf_io_cfg *io_cfg;
	uint32_t              i = 0, j = 0;
	int                   rc = 0;
	uintptr_t             buf_addr;
	size_t                buf_size;
	uint8_t               *read_buffer;

	int                   nVer = 0;
	uint8_t               *pBuf = NULL;
	uint8_t	              bVerNormal = TRUE;

	char                  tmp_hw_info[HW_INFO_MAX_SIZE];// = HW_INFO;
	char                  tmp_sw_info[SW_INFO_MAX_SIZE];// = SW_INFO;
	char                  tmp_vendor_info[VENDOR_INFO_MAX_SIZE];// = VENDOR_INFO;
	char                  tmp_process_info[PROCESS_INFO_MAX_SIZE];// = PROCESS_INFO;
	unsigned int          tmp_rev = 0;

	io_cfg = (struct cam_buf_io_cfg *) ((uint8_t *)
		&csl_packet->payload +
		csl_packet->io_configs_offset);

	CAM_INFO(CAM_EEPROM, "number of IO configs: %d:",
		csl_packet->num_io_configs);

	for (i = 0; i < csl_packet->num_io_configs; i++) {
		CAM_INFO(CAM_EEPROM, "Direction: %d:", io_cfg->direction);
		if (io_cfg->direction == CAM_BUF_OUTPUT) {
			rc = cam_mem_get_cpu_buf(io_cfg->mem_handle[0],
				&buf_addr, &buf_size);
			CAM_INFO(CAM_EEPROM, "buf_addr : %pK, buf_size : %zu",
				(void *)buf_addr, buf_size);

			read_buffer = (uint8_t *)buf_addr;
			if (!read_buffer) {
				CAM_ERR(CAM_EEPROM,
					"invalid buffer to copy data");
				return -EINVAL;
			}
			read_buffer += io_cfg->offsets[0];

			if (buf_size < e_ctrl->cal_data.num_data) {
				CAM_ERR(CAM_EEPROM,
					"failed to copy, Invalid size");
				return -EINVAL;
			}

			CAM_INFO(CAM_EEPROM, "copy the data, len:%d, read_buffer[0] = %d, read_buffer[4] = %d",
				e_ctrl->cal_data.num_data, read_buffer[0], read_buffer[4]);

			pBuf = read_buffer;
			memcpy(&nVer, pBuf, sizeof(int));
			pBuf += sizeof(int);

			memcpy(&tmp_rev, pBuf, sizeof(int));
			pBuf += sizeof(int);

			bVerNormal = TRUE;
			for(j = 0; j < FROM_MODULE_FW_INFO_SIZE; j ++) {
				CAM_DBG(CAM_EEPROM, "mapdata[0x%04X] = 0x%02X",
					REAR_MODULE_FW_VERSION + j,
					e_ctrl->cal_data.mapdata[REAR_MODULE_FW_VERSION + j]);

				if(e_ctrl->cal_data.mapdata[REAR_MODULE_FW_VERSION + j] >= 0x80
					|| !isalnum(e_ctrl->cal_data.mapdata[REAR_MODULE_FW_VERSION + j] & 0xFF)) {
					CAM_ERR(CAM_EEPROM, "Invalid Version");
					bVerNormal = FALSE;
					break;
				}
			}

			if(bVerNormal == TRUE) {
				memcpy(hw_phone_info, &e_ctrl->cal_data.mapdata[REAR_MODULE_FW_VERSION],
					HW_INFO_MAX_SIZE);
				hw_phone_info[HW_INFO_MAX_SIZE-1] = '\0';
				CAM_INFO(CAM_EEPROM, "hw_phone_info: %s", hw_phone_info);
			}
#if 0
			else {
				memcpy(hw_phone_info, HW_INFO, HW_INFO_MAX_SIZE);
				memcpy(sw_phone_info, SW_INFO, SW_INFO_MAX_SIZE);
				memcpy(vendor_phone_info, VENDOR_INFO, VENDOR_INFO_MAX_SIZE);
				memcpy(process_phone_info, PROCESS_INFO, PROCESS_INFO_MAX_SIZE);
				CAM_INFO(CAM_EEPROM, "Set Ver : %s %s %s %s",
					hw_phone_info, sw_phone_info,
					vendor_phone_info, process_phone_info);
			}
#endif
			CAM_INFO(CAM_EEPROM, "hw_phone_info: %s", hw_phone_info);

			for (i = 0; i < nVer; i++) {
				memcpy(tmp_hw_info, pBuf, HW_INFO_MAX_SIZE);
				pBuf += HW_INFO_MAX_SIZE;

				memcpy(tmp_sw_info, pBuf, SW_INFO_MAX_SIZE);
				pBuf += SW_INFO_MAX_SIZE;

				memcpy(tmp_vendor_info, pBuf, VENDOR_INFO_MAX_SIZE);
				tmp_vendor_info[VENDOR_INFO_MAX_SIZE-1] = '\0';
				pBuf += VENDOR_INFO_MAX_SIZE-1;

				memcpy(tmp_process_info, pBuf, PROCESS_INFO_MAX_SIZE);
				tmp_process_info[PROCESS_INFO_MAX_SIZE-1] = '\0';
				pBuf += PROCESS_INFO_MAX_SIZE;

				CAM_INFO(CAM_EEPROM, "[temp %d/%d] : %s %s %s %s",
					i, nVer, tmp_hw_info, tmp_sw_info,
					tmp_vendor_info, tmp_process_info);

				if (strcmp(hw_phone_info, tmp_hw_info) == 0) {
					memcpy(sw_phone_info, tmp_sw_info, SW_INFO_MAX_SIZE);
					memcpy(vendor_phone_info, tmp_vendor_info, VENDOR_INFO_MAX_SIZE);
					memcpy(process_phone_info, tmp_process_info, PROCESS_INFO_MAX_SIZE);
					CAM_INFO(CAM_EEPROM, "rear [%d] : %s %s %s %s",
						i, hw_phone_info, sw_phone_info,
						vendor_phone_info, process_phone_info);
				}
#if 0
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
				else if (strcmp(rear2_hw_phone_info, tmp_hw_info) == 0) {
					memcpy(rear2_sw_phone_info, tmp_sw_info, SW_INFO_MAX_SIZE);
					memcpy(rear2_vendor_phone_info, tmp_vendor_info, VENDOR_INFO_MAX_SIZE);
					memcpy(rear2_process_phone_info, tmp_process_info, PROCESS_INFO_MAX_SIZE);
					CAM_INFO(CAM_EEPROM, "rear2 [%d] : %s %s %s %s",
						i, rear2_hw_phone_info, rear2_sw_phone_info,
						rear2_vendor_phone_info, rear2_process_phone_info);
				}
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
				else if (strcmp(rear3_hw_phone_info, tmp_hw_info) == 0) {
					memcpy(rear3_sw_phone_info, tmp_sw_info, SW_INFO_MAX_SIZE);
					memcpy(rear3_vendor_phone_info, tmp_vendor_info, VENDOR_INFO_MAX_SIZE);
					memcpy(rear3_process_phone_info, tmp_process_info, PROCESS_INFO_MAX_SIZE);
					CAM_INFO(CAM_EEPROM, "rear3 [%d] : %s %s %s %s",
						i, rear3_hw_phone_info, rear3_sw_phone_info,
						rear3_vendor_phone_info, rear3_process_phone_info);
				}
#endif
#endif
				else if (strcmp(front_hw_phone_info, tmp_hw_info) == 0) {
					memcpy(front_sw_phone_info, tmp_sw_info, SW_INFO_MAX_SIZE);
					memcpy(front_vendor_phone_info, tmp_vendor_info, VENDOR_INFO_MAX_SIZE);
					memcpy(front_process_phone_info, tmp_process_info, PROCESS_INFO_MAX_SIZE);
					CAM_INFO(CAM_EEPROM, "front [%d] : %s %s %s %s",
						i, front_hw_phone_info, front_sw_phone_info,
						front_vendor_phone_info, front_process_phone_info);
				}
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
				else if (strcmp(front2_hw_phone_info, tmp_hw_info) == 0) {
					memcpy(front2_sw_phone_info, tmp_sw_info, SW_INFO_MAX_SIZE);
					memcpy(front2_vendor_phone_info, tmp_vendor_info, VENDOR_INFO_MAX_SIZE);
					memcpy(front2_process_phone_info, tmp_process_info, PROCESS_INFO_MAX_SIZE);
					CAM_INFO(CAM_EEPROM, "front2 [%d] : %s %s %s %s",
						i, front2_hw_phone_info, front2_sw_phone_info,
						front2_vendor_phone_info, front2_process_phone_info);
				}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
				else if (strcmp(front3_hw_phone_info, tmp_hw_info) == 0) {
					memcpy(front3_sw_phone_info, tmp_sw_info, SW_INFO_MAX_SIZE);
					memcpy(front3_vendor_phone_info, tmp_vendor_info, VENDOR_INFO_MAX_SIZE);
					memcpy(front3_process_phone_info, tmp_process_info, PROCESS_INFO_MAX_SIZE);
					CAM_INFO(CAM_EEPROM, "front3 [%d] : %s %s %s %s",
						i, front3_hw_phone_info, front3_sw_phone_info,
						front3_vendor_phone_info, front3_process_phone_info);
				}
#endif
#endif
				else {
					CAM_INFO(CAM_EEPROM, "invalid hwinfo: %s", tmp_hw_info);
				}
			}
			memset(read_buffer, 0x00, e_ctrl->cal_data.num_data);
		} else {
			CAM_ERR(CAM_EEPROM, "Invalid direction");
			rc = -EINVAL;
		}
	}

	return rc;
#endif
}

static int32_t delete_eeprom_request(struct i2c_settings_array *i2c_array)
{
	struct i2c_settings_list *i2c_list = NULL, *i2c_next = NULL;
	int32_t rc = 0;

	if (i2c_array == NULL) {
		CAM_ERR(CAM_SENSOR, "FATAL:: Invalid argument");
		return -EINVAL;
	}

	list_for_each_entry_safe(i2c_list, i2c_next,
		&(i2c_array->list_head), list) {
		kfree(i2c_list->seq_settings.reg_data);
		list_del(&(i2c_list->list));
		kfree(i2c_list);
	}
	INIT_LIST_HEAD(&(i2c_array->list_head));
	i2c_array->is_settings_valid = 0;

	return rc;
}

/**
 * cam_eeprom_write - Write Packet
 * @e_ctrl:     ctrl structure
 *
 * Returns success or failure
 */
static int32_t cam_eeprom_write(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int32_t rc = 0;
	struct i2c_settings_array *i2c_set = NULL;
	struct i2c_settings_list *i2c_list = NULL;

	i2c_set = &e_ctrl->wr_settings;

	if (i2c_set->is_settings_valid == 1) {
		list_for_each_entry(i2c_list,
			&(i2c_set->list_head), list) {
			rc = camera_io_dev_write_continuous(
				&e_ctrl->io_master_info,
				&i2c_list->i2c_settings, 1);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM,
				"Error in EEPROM write");
			goto del_req;
		}
		}
	}

del_req:
	delete_eeprom_request(i2c_set);
	return rc;
}

/**
 * cam_eeprom_pkt_parse - Parse csl packet
 * @e_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int32_t cam_eeprom_pkt_parse(struct cam_eeprom_ctrl_t *e_ctrl, void *arg)
{
	int32_t                         rc = 0;
	struct cam_control             *ioctl_ctrl = NULL;
	struct cam_config_dev_cmd       dev_config;
	uintptr_t                        generic_pkt_addr;
	size_t                          pkt_len;
	struct cam_packet              *csl_packet = NULL;
	struct cam_eeprom_soc_private  *soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t *power_info = &soc_private->power_info;

	ioctl_ctrl = (struct cam_control *)arg;

	if (copy_from_user(&dev_config,
		u64_to_user_ptr(ioctl_ctrl->handle),
		sizeof(dev_config)))
		return -EFAULT;
	rc = cam_mem_get_cpu_buf(dev_config.packet_handle,
		&generic_pkt_addr, &pkt_len);
	if (rc) {
		CAM_ERR(CAM_EEPROM,
			"error in converting command Handle Error: %d", rc);
		return rc;
	}

	CAM_INFO(CAM_EEPROM,
		"Offset is out of bound: off: %lld, %zu",
		dev_config.offset, pkt_len);

	if (dev_config.offset > pkt_len) {
		CAM_ERR(CAM_EEPROM,
			"Offset is out of bound: off: %lld, %zu",
			dev_config.offset, pkt_len);
		return -EINVAL;
	}

	csl_packet = (struct cam_packet *)
		(generic_pkt_addr + (uint32_t)dev_config.offset);

	switch (csl_packet->header.op_code & 0xFFFFFF) {
	case CAM_EEPROM_PACKET_OPCODE_INIT:
		CAM_INFO(CAM_EEPROM, "e_ctrl->userspace_probe : %d", e_ctrl->userspace_probe);
		if (e_ctrl->userspace_probe == false) {
			CAM_ERR(CAM_EEPROM, "VR:: KERNEL PROBE ");
			rc = cam_eeprom_parse_read_memory_map(
					e_ctrl->soc_info.dev->of_node, e_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_EEPROM, "Failed: rc : %d", rc);
				return rc;
			}
			rc = cam_eeprom_get_cal_data(e_ctrl, csl_packet);
			vfree(e_ctrl->cal_data.mapdata);
			vfree(e_ctrl->cal_data.map);
			e_ctrl->cal_data.num_data = 0;
			e_ctrl->cal_data.num_map = 0;
			CAM_DBG(CAM_EEPROM,
				"Returning the data using kernel probe");
			break;
		}
		rc = cam_eeprom_init_pkt_parser(e_ctrl, csl_packet);
		if (rc) {
			CAM_ERR(CAM_EEPROM,
				"Failed in parsing the pkt");
			return rc;
		}

		if ((e_ctrl->cal_data.num_map == 0) &&
			(e_ctrl->cal_data.map != NULL)) {
			vfree(e_ctrl->cal_data.map);
			CAM_INFO(CAM_EEPROM, "No read settings privided");
			return rc;
		}

		e_ctrl->cal_data.num_data = cam_eeprom_calc_calmap_size(e_ctrl);

		if (e_ctrl->cal_data.num_data == 0) {
			rc = -ENOMEM;
			CAM_ERR(CAM_EEPROM, "failed");
			goto error;
		}

		e_ctrl->cal_data.mapdata =
			vzalloc(e_ctrl->cal_data.num_data);
		if (!e_ctrl->cal_data.mapdata) {
			rc = -ENOMEM;
			CAM_ERR(CAM_EEPROM, "failed");
			goto error;
		}

		if (e_ctrl->eeprom_device_type == MSM_CAMERA_SPI_DEVICE) {
			rc = cam_eeprom_match_id(e_ctrl);
			if (rc) {
				CAM_DBG(CAM_EEPROM,
					"eeprom not matching %d", rc);
				goto memdata_free;
			}
		}

		rc = cam_eeprom_power_up(e_ctrl,
			&soc_private->power_info);
		if (rc) {
			CAM_ERR(CAM_EEPROM, "failed rc %d", rc);
			goto memdata_free;
		}

		e_ctrl->cam_eeprom_state = CAM_EEPROM_CONFIG;
		{
			int i;
			int normal_crc_value = 0;

			if (e_ctrl->eeprom_device_type == MSM_CAMERA_SPI_DEVICE) {
				rc = cam_eeprom_match_id(e_ctrl);
				if (rc) {
					CAM_DBG(CAM_EEPROM, "eeprom not matching %d", rc);
					//goto power_down;
					rc = 0;
				}
			}

			normal_crc_value = 0;
			for (i = 0; i < e_ctrl->cal_data.num_map>>1; i++)
				normal_crc_value |= (1 << i);

			CAMERA_NORMAL_CAL_CRC = normal_crc_value;
			CAM_INFO(CAM_EEPROM, "num_map = %d, CAMERA_NORMAL_CAL_CRC = 0x%X",
				e_ctrl->cal_data.num_map, CAMERA_NORMAL_CAL_CRC);

			rc = cam_eeprom_read_memory(e_ctrl, &e_ctrl->cal_data);
			if (rc < 0) {
				CAM_ERR(CAM_EEPROM,
				 "read_eeprom_memory failed");
				goto power_down;
			}

			if (1 < e_ctrl->cal_data.num_map) {
				rc = cam_eeprom_get_customInfo(e_ctrl, csl_packet);

				e_ctrl->is_supported |= cam_eeprom_match_crc(&e_ctrl->cal_data,
					e_ctrl->soc_info.index);

				if (e_ctrl->is_supported != normal_crc_value)
					CAM_ERR(CAM_EEPROM, "Any CRC values at F-ROM are not matched.");
				else
					CAM_INFO(CAM_EEPROM, "All CRC values are matched.");

				rc = cam_eeprom_update_module_info(e_ctrl);
				if (rc < 0) {
					CAM_ERR(CAM_EEPROM, "cam_eeprom_update_module_info failed");
					goto power_down;
				}

#ifdef CAM_EEPROM_DBG_DUMP
				if (e_ctrl->soc_info.index == 0)
					rc = cam_eeprom_dump(e_ctrl->soc_info.index,
						e_ctrl->cal_data.mapdata, 0x4680, 0x2C);
				else
					rc = cam_eeprom_dump(e_ctrl->soc_info.index,
						e_ctrl->cal_data.mapdata, 0xB0, 0x60);
#endif
			} else if (e_ctrl->cal_data.num_map == 1 &&
				e_ctrl->cal_data.num_data == FROM_REAR_HEADER_SIZE) {
				// run this on eebin check
				rc = cam_eeprom_get_phone_ver(e_ctrl, csl_packet);
			}
		}

		rc = cam_eeprom_get_cal_data(e_ctrl, csl_packet);
		rc = cam_eeprom_power_down(e_ctrl);
		e_ctrl->cam_eeprom_state = CAM_EEPROM_ACQUIRE;
		vfree(e_ctrl->cal_data.mapdata);
		vfree(e_ctrl->cal_data.map);
		kfree(power_info->power_setting);
		kfree(power_info->power_down_setting);
		power_info->power_setting = NULL;
		power_info->power_down_setting = NULL;
		power_info->power_setting_size = 0;
		power_info->power_down_setting_size = 0;
		e_ctrl->cal_data.num_data = 0;
		e_ctrl->cal_data.num_map = 0;
		break;

	case CAM_EEPROM_WRITE: {
		struct i2c_settings_array *i2c_reg_settings =
			&e_ctrl->wr_settings;

		i2c_reg_settings->is_settings_valid = 1;
		rc = cam_eeprom_parse_write_memory_packet(
			csl_packet, e_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM, "Failed: rc : %d", rc);
			return rc;
		}

		rc = cam_eeprom_power_up(e_ctrl,
			&soc_private->power_info);
		if (rc) {
			CAM_ERR(CAM_EEPROM, "failed power up rc %d", rc);
			goto memdata_free;
		}

		usleep_range(10*1000, 11*1000);
		CAM_DBG(CAM_EEPROM,
			"Calling Erase : %d start Address: 0x%x size: %d",
			rc, e_ctrl->eebin_info.start_address,
			e_ctrl->eebin_info.size);

		rc = camera_io_dev_erase(&e_ctrl->io_master_info,
			e_ctrl->eebin_info.start_address,
			e_ctrl->eebin_info.size);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM, "Failed in erase : %d", rc);
			return rc;
		}

		/* Buffer time margin */
		usleep_range(10*1000, 11*1000);

		rc = cam_eeprom_write(e_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM, "Failed: rc : %d", rc);
			return rc;
		}

		rc = cam_eeprom_power_down(e_ctrl);
		if (rc) {
			CAM_ERR(CAM_EEPROM, "failed power down rc %d", rc);
			goto memdata_free;
		}

		break;
	}

	default:
		CAM_ERR(CAM_EEPROM, "Invalid op-code 0x%x",
			csl_packet->header.op_code & 0xFFFFFF);
		rc = -EINVAL;
		break;
	}

	return rc;
power_down:
	cam_eeprom_power_down(e_ctrl);
memdata_free:
	vfree(e_ctrl->cal_data.mapdata);
error:
	kfree(power_info->power_setting);
	kfree(power_info->power_down_setting);
	power_info->power_setting = NULL;
	power_info->power_down_setting = NULL;
	vfree(e_ctrl->cal_data.map);
	e_ctrl->cal_data.num_data = 0;
	e_ctrl->cal_data.num_map = 0;
	e_ctrl->cam_eeprom_state = CAM_EEPROM_ACQUIRE;
	return rc;
}

void cam_eeprom_shutdown(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int rc;
	struct cam_eeprom_soc_private  *soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t *power_info = &soc_private->power_info;

	if (e_ctrl->cam_eeprom_state == CAM_EEPROM_INIT)
		return;

	if (e_ctrl->cam_eeprom_state == CAM_EEPROM_CONFIG) {
		rc = cam_eeprom_power_down(e_ctrl);
		if (rc < 0)
			CAM_ERR(CAM_EEPROM, "EEPROM Power down failed");
		e_ctrl->cam_eeprom_state = CAM_EEPROM_ACQUIRE;
	}

	if (e_ctrl->cam_eeprom_state == CAM_EEPROM_ACQUIRE) {
		rc = cam_destroy_device_hdl(e_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_EEPROM, "destroying the device hdl");

		e_ctrl->bridge_intf.device_hdl = -1;
		e_ctrl->bridge_intf.link_hdl = -1;
		e_ctrl->bridge_intf.session_hdl = -1;

		kfree(power_info->power_setting);
		kfree(power_info->power_down_setting);
		power_info->power_setting = NULL;
		power_info->power_down_setting = NULL;
		power_info->power_setting_size = 0;
		power_info->power_down_setting_size = 0;
	}

	e_ctrl->cam_eeprom_state = CAM_EEPROM_INIT;
}

/**
 * cam_eeprom_driver_cmd - Handle eeprom cmds
 * @e_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
int32_t cam_eeprom_driver_cmd(struct cam_eeprom_ctrl_t *e_ctrl, void *arg)
{
	int                            rc = 0;
	struct cam_eeprom_query_cap_t  eeprom_cap = {0};
	struct cam_control            *cmd = (struct cam_control *)arg;

	if (!e_ctrl || !cmd) {
		CAM_ERR(CAM_EEPROM, "Invalid Arguments");
		return -EINVAL;
	}

	if (cmd->handle_type != CAM_HANDLE_USER_POINTER) {
		CAM_ERR(CAM_EEPROM, "Invalid handle type: %d",
			cmd->handle_type);
		return -EINVAL;
	}

	mutex_lock(&(e_ctrl->eeprom_mutex));
	switch (cmd->op_code) {
	case CAM_QUERY_CAP:
		eeprom_cap.slot_info = e_ctrl->soc_info.index;
		if (e_ctrl->userspace_probe == false)
			eeprom_cap.eeprom_kernel_probe = true;
		else
			eeprom_cap.eeprom_kernel_probe = false;

		eeprom_cap.is_multimodule_mode =
			e_ctrl->is_multimodule_mode;
		CAM_INFO(CAM_EEPROM, "eeprom_cap.is_multimodule_mode: %d, e_ctrl->is_multimodule_mode: %d",
			eeprom_cap.is_multimodule_mode, e_ctrl->is_multimodule_mode);

		if (copy_to_user(u64_to_user_ptr(cmd->handle),
			&eeprom_cap,
			sizeof(struct cam_eeprom_query_cap_t))) {
			CAM_ERR(CAM_EEPROM, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		CAM_DBG(CAM_EEPROM, "eeprom_cap: ID: %d", eeprom_cap.slot_info);
		break;
	case CAM_ACQUIRE_DEV:
		rc = cam_eeprom_get_dev_handle(e_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_EEPROM, "Failed to acquire dev");
			goto release_mutex;
		}
		e_ctrl->cam_eeprom_state = CAM_EEPROM_ACQUIRE;
		break;
	case CAM_RELEASE_DEV:
		if (e_ctrl->cam_eeprom_state != CAM_EEPROM_ACQUIRE) {
			rc = -EINVAL;
			CAM_WARN(CAM_EEPROM,
			"Not in right state to release : %d",
			e_ctrl->cam_eeprom_state);
			goto release_mutex;
		}

		if (e_ctrl->bridge_intf.device_hdl == -1) {
			CAM_ERR(CAM_EEPROM,
				"Invalid Handles: link hdl: %d device hdl: %d",
				e_ctrl->bridge_intf.device_hdl,
				e_ctrl->bridge_intf.link_hdl);
			rc = -EINVAL;
			goto release_mutex;
		}
		rc = cam_destroy_device_hdl(e_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_EEPROM,
				"failed in destroying the device hdl");
		e_ctrl->bridge_intf.device_hdl = -1;
		e_ctrl->bridge_intf.link_hdl = -1;
		e_ctrl->bridge_intf.session_hdl = -1;
		e_ctrl->cam_eeprom_state = CAM_EEPROM_INIT;
		break;
	case CAM_CONFIG_DEV:
		rc = cam_eeprom_pkt_parse(e_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_EEPROM, "Failed in eeprom pkt Parsing");
			goto release_mutex;
		}
		break;
	default:
		CAM_DBG(CAM_EEPROM, "invalid opcode");
		break;
	}

release_mutex:
	mutex_unlock(&(e_ctrl->eeprom_mutex));

	return rc;
}

