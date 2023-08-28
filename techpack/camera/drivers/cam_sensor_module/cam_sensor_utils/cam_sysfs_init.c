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
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "cam_ois_core.h"
#include "cam_eeprom_dev.h"
#include "cam_actuator_core.h"
#if defined(CONFIG_LEDS_PMIC_QPNP)
#include "cam_flash_core.h"
#endif
#if defined(CONFIG_SAMSUNG_APERTURE)
#include "cam_aperture_core.h"
#endif
#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
#include "cam_sensor_mipi.h"
#endif
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
#include "cam_ois_mcu_stm32g.h"
#endif
#if defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
#include "cam_ois_rumba_s4.h"
#endif

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
#include "cam_sensor_cmn_header.h"
#include "cam_debug_util.h"
#endif

#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
#include "cam_sensor_dev.h"
#endif

#if 0 //EARLY_RETENTION
extern int32_t cam_sensor_early_retention(void);
#endif

#if defined(CONFIG_CAMERA_SSM_I2C_ENV)
extern void cam_sensor_ssm_i2c_read(uint32_t addr, uint32_t *data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type);
extern void cam_sensor_ssm_i2c_write(uint32_t addr, uint32_t data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type);
#endif
#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
extern void cam_sensor_tof_i2c_read(uint32_t addr, uint32_t *data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type);
extern void cam_sensor_tof_i2c_write(uint32_t addr, uint32_t data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type);
extern struct cam_sensor_ctrl_t *g_s_ctrl_tof;
extern int check_pd_ready;
#endif

#if defined(CONFIG_LEDS_PMIC_QPNP)
extern ssize_t flash_power_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size);
#endif

#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
extern char tof_freq[10];
#endif
#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
extern char band_info[20];
#endif

extern struct kset *devices_kset;
struct class *camera_class;

#define SYSFS_FW_VER_SIZE       40
#define SYSFS_MODULE_INFO_SIZE  96
/* #define FORCE_CAL_LOAD */
#define SYSFS_MAX_READ_SIZE     4096

#if defined(CONFIG_CAMERA_SSM_I2C_ENV)
static ssize_t rear_ssm_frame_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t read_data = -1;
	int rc = 0;

#if defined(CONFIG_SEC_BLOOMXQ_PROJECT)
	cam_sensor_ssm_i2c_read(0x1A0C, &read_data, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
#else
	cam_sensor_ssm_i2c_read(0x000A, &read_data, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
#endif

	rc = scnprintf(buf, PAGE_SIZE, "%x\n", read_data);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_ssm_frame_id_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value = -1;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	return size;
}

#if defined(CONFIG_SEC_BLOOMXQ_PROJECT)
static ssize_t rear_ssm_gmc_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t read_data = -1;
	uint32_t modified_data = -1;
	int rc = 0;

	cam_sensor_ssm_i2c_write(0x6000, 0x0005, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	cam_sensor_ssm_i2c_write(0xFCFC, 0x2001, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	cam_sensor_ssm_i2c_read(0x0E34, &read_data, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	cam_sensor_ssm_i2c_write(0xFCFC, 0x4000, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	cam_sensor_ssm_i2c_write(0x6000, 0x0085, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);

	modified_data = read_data >> 8;

	rc = scnprintf(buf, PAGE_SIZE, "%x\n", modified_data);
	if (rc)
		return rc;
	return 0;
}
#else
static ssize_t rear_ssm_gmc_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t read_data = -1;
	int rc = 0;

	cam_sensor_ssm_i2c_read(0x9C6A, &read_data, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	rc = scnprintf(buf, PAGE_SIZE, "%x\n", read_data);
	if (rc)
		return rc;
	return 0;
}
#endif
static ssize_t rear_ssm_gmc_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value = -1;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	return size;
}

static ssize_t rear_ssm_flicker_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t read_data = -1;
	int rc = 0;

	cam_sensor_ssm_i2c_read(0x9C6B, &read_data, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	rc = scnprintf(buf, PAGE_SIZE, "%x\n", read_data);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_ssm_flicker_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value = -1;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	return size;
}

#if defined(CONFIG_SEC_BLOOMXQ_PROJECT)
static ssize_t rear_ssm_precheck_md_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t read_data = -1;
	int rc = 0;

	cam_sensor_ssm_i2c_read(0xDB0E, &read_data, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);

	rc = scnprintf(buf, PAGE_SIZE, "%d\n", read_data);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_ssm_precheck_md_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value = -1;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	return size;
}
#endif
#endif

char cam_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";//multi module
static ssize_t rear_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	pr_info("[FW_DBG] cam_fw_ver : %s\n", cam_fw_ver);

	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_fw_ver, sizeof(cam_fw_ver), "%s", buf);

	return size;
}

static ssize_t rear_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
	char cam_type[] = "LSI_S5KHM1\n";
#elif defined(CONFIG_SEC_GTS7XL_PROJECT) || defined(CONFIG_SEC_GTS7L_PROJECT)
	char cam_type[] = "SLSI_S5K3M5\n";
#elif defined(CONFIG_SEC_BLOOMXQ_PROJECT)
	char cam_type[] = "SLSI_SAK2L4SX\n";
#else
	char cam_type[] = "SONY_IMX555\n";
#endif

	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_type);

	if (rc)
		return rc;
	return 0;
}

static ssize_t front_camera_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
#if defined(CONFIG_SEC_Z3Q_PROJECT)
	char cam_type[] = "LSI_S5KGH1\n";
#elif defined(CONFIG_SEC_GTS7XL_PROJECT) || defined(CONFIG_SEC_GTS7L_PROJECT)
	char cam_type[] = "S5K4HA\n";
#elif defined(CONFIG_SEC_BLOOMXQ_PROJECT)
	char cam_type[] = "SONY_IMX374\n";
#elif defined(CONFIG_SEC_R8Q_PROJECT)
	char cam_type[] = "SONY_IMX616\n";
#else
	char cam_type[] = "SONY_IMX374\n";
#endif
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_type);
	if (rc)
		return rc;
	return 0;
}
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
static ssize_t front2_camera_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	char cam_type[] = "S5K4HA\n";

	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_type);
	if (rc)
		return rc;
	return 0;
}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
static ssize_t front3_camera_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	char cam_type[] = "SONY_IMX374\n";

	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_type);
	if (rc)
		return rc;
	return 0;
}
#else
static ssize_t front2_camera_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	char cam_type[] = "SONY_IMX374\n";

	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_type);
	if (rc)
		return rc;
	return 0;
}
#endif
#endif
char cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "NULL\n";//multi module
static ssize_t rear_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_fw_user_ver : %s\n", cam_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{

	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_fw_user_ver, sizeof(cam_fw_user_ver), "%s", buf);

	return size;
}

char cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NULL\n";//multi module
static ssize_t rear_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear_fw_factory_ver : %s\n", cam_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_fw_factory_ver, sizeof(cam_fw_factory_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
char cam3_fw_user_ver[SYSFS_FW_VER_SIZE] = "NULL\n";//multi module
static ssize_t rear3_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam3_fw_user_ver : %s\n", cam3_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam3_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear3_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{

	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam3_fw_user_ver, sizeof(cam3_fw_user_ver), "%s", buf);

	return size;
}

char cam3_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NULL\n";//multi module
static ssize_t rear3_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear3_fw_factory_ver : %s\n", cam3_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam3_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear3_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam3_fw_factory_ver, sizeof(cam3_fw_factory_ver), "%s", buf);

	return size;
}
#endif

char cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";//multi module
static ssize_t rear_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear_fw_full_ver : %s\n", cam_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_fw_full_ver, sizeof(cam_fw_full_ver), "%s", buf);

	return size;
}

char cam_load_fw[SYSFS_FW_VER_SIZE] = "NULL\n";
static ssize_t rear_firmware_load_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear_load_fw : %s\n", cam_load_fw);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_load_fw);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_firmware_load_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_load_fw, sizeof(cam_load_fw), "%s\n", buf);
	return size;
}

char cal_crc[SYSFS_FW_VER_SIZE] = "NULL NULL\n";
static ssize_t rear_cal_data_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cal_crc : %s\n", cal_crc);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cal_crc);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_cal_data_check_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cal_crc, sizeof(cal_crc), "%s", buf);

	return size;
}

char module_info[SYSFS_MODULE_INFO_SIZE] = "NULL\n";
static ssize_t rear_module_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] module_info : %s\n", module_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", module_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_module_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(module_info, sizeof(module_info), "%s", buf);

	return size;
}

char front_module_info[SYSFS_MODULE_INFO_SIZE] = "NULL\n";
static ssize_t front_module_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_module_info : %s\n", front_module_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_module_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_module_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_module_info, sizeof(front_module_info), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
char front2_module_info[SYSFS_MODULE_INFO_SIZE] = "NULL\n";
static ssize_t front2_module_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front2_module_info : %s\n", front2_module_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_module_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_module_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_module_info, sizeof(front2_module_info), "%s", buf);

	return size;
}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
char front3_module_info[SYSFS_MODULE_INFO_SIZE] = "NULL\n";
static ssize_t front3_module_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front3_module_info : %s\n", front3_module_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front3_module_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front3_module_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front3_module_info, sizeof(front3_module_info), "%s", buf);

	return size;
}
#else
char front2_module_info[SYSFS_MODULE_INFO_SIZE] = "NULL\n";
static ssize_t front2_module_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front2_module_info : %s\n", front2_module_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_module_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_module_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_module_info, sizeof(front2_module_info), "%s", buf);

	return size;
}
#endif
#endif

char isp_core[10];
static ssize_t rear_isp_core_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#if 0// Power binning is used
	char cam_isp_core[] = "0.8V\n";

	return scnprintf(buf, sizeof(cam_isp_core), "%s", cam_isp_core);
#else
	int rc = 0;

	pr_debug("[FW_DBG] isp_core : %s\n", isp_core);
	rc = scnprintf(buf, PAGE_SIZE, "%s\n", isp_core);
	if (rc)
		return rc;
	return 0;
#endif
}

static ssize_t rear_isp_core_check_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(isp_core, sizeof(isp_core), "%s", buf);

	return size;
}

#if 1
char rear_af_cal_str[MAX_AF_CAL_STR_SIZE] = "";
static ssize_t rear_afcal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	pr_debug("rear_af_cal_str : 10 %s\n", rear_af_cal_str);
	rc = scnprintf(buf, PAGE_SIZE, "10 %s", rear_af_cal_str);
	if (rc)
		return rc;
#else
	pr_debug("rear_af_cal_str : 1 %s\n", rear_af_cal_str);
	rc = scnprintf(buf, PAGE_SIZE, "1 %s", rear_af_cal_str);
	if (rc)
		return rc;
#endif

	return 0;
}
#else
#define FROM_REAR_AF_CAL_SIZE	 10

//int rear_af_cal[FROM_REAR_AF_CAL_SIZE + 1] = {0,};
char rear_af_cal_str[MAX_AF_CAL_STR_SIZE] = "";
static ssize_t rear_afcal_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	char tempbuf[10];
	char N[] = "N ";

	strncat(buf, "10 ", strlen("10 "));

#ifdef	FROM_REAR_AF_CAL_D10_ADDR
	sprintf(tempbuf, "%d ", rear_af_cal[1]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR_AF_CAL_D20_ADDR
	sprintf(tempbuf, "%d ", rear_af_cal[2]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR_AF_CAL_D30_ADDR
	sprintf(tempbuf, "%d ", rear_af_cal[3]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR_AF_CAL_D40_ADDR
	sprintf(tempbuf, "%d ", rear_af_cal[4]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR_AF_CAL_D50_ADDR
	sprintf(tempbuf, "%d ", rear_af_cal[5]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR_AF_CAL_D60_ADDR
	sprintf(tempbuf, "%d ", rear_af_cal[6]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR_AF_CAL_D70_ADDR
	sprintf(tempbuf, "%d ", rear_af_cal[7]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR_AF_CAL_D80_ADDR
	sprintf(tempbuf, "%d ", rear_af_cal[8]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR_AF_CAL_PAN_ADDR
	sprintf(tempbuf, "%d ", rear_af_cal[9]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

	return strlen(buf);
#else
	return sprintf(buf, "1 %d %d\n", rear_af_cal[0], rear_af_cal[9]);
#endif
}
#endif

char rear_paf_cal_data_far[PAF_2PD_CAL_INFO_SIZE] = {0,};
char rear_paf_cal_data_mid[PAF_2PD_CAL_INFO_SIZE] = {0,};

static ssize_t rear_paf_offset_mid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("rear_paf_cal_data : %s\n", rear_paf_cal_data_mid);
	rc = scnprintf(buf, PAGE_SIZE, "%s", rear_paf_cal_data_mid);
	if (rc) {
		pr_debug("data size %d\n", rc);
		return rc;
	}
	return 0;
}
static ssize_t rear_paf_offset_far_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("rear_paf_cal_data : %s\n", rear_paf_cal_data_far);
	rc = scnprintf(buf, PAGE_SIZE, "%s", rear_paf_cal_data_far);
	if (rc) {
		pr_debug("data size %d\n", rc);
		return rc;
	}
	return 0;
}

uint32_t paf_err_data_result = 0xFFFFFFFF;
static ssize_t rear_paf_cal_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("paf_cal_check : %u\n", paf_err_data_result);
	rc = scnprintf(buf, PAGE_SIZE, "%08X\n", paf_err_data_result);
	if (rc)
		return rc;
	return 0;
}

char rear_f2_paf_cal_data_far[PAF_2PD_CAL_INFO_SIZE] = {0,};
char rear_f2_paf_cal_data_mid[PAF_2PD_CAL_INFO_SIZE] = {0,};
static ssize_t rear_f2_paf_offset_mid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("rear_f2_paf_cal_data : %s\n", rear_f2_paf_cal_data_mid);
	rc = scnprintf(buf, PAGE_SIZE, "%s", rear_f2_paf_cal_data_mid);
	if (rc) {
		pr_debug("data size %d\n", rc);
		return rc;
	}
	return 0;
}
static ssize_t rear_f2_paf_offset_far_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("rear_f2_paf_cal_data : %s\n", rear_f2_paf_cal_data_far);
	rc = scnprintf(buf, PAGE_SIZE, "%s", rear_f2_paf_cal_data_far);
	if (rc) {
		pr_debug("data size %d\n", rc);
		return rc;
	}
	return 0;
}

uint32_t f2_paf_err_data_result = 0xFFFFFFFF;
static ssize_t rear_f2_paf_cal_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("f2_paf_cal_check : %u\n", f2_paf_err_data_result);
	rc = scnprintf(buf, PAGE_SIZE, "%08X\n", f2_paf_err_data_result);
	if (rc)
		return rc;
	return 0;
}
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
uint32_t rear3_paf_err_data_result = 0xFFFFFFFF;
static ssize_t rear3_paf_cal_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("rear3_paf_err_data_result : %u\n", rear3_paf_err_data_result);
	rc = scnprintf(buf, PAGE_SIZE, "%08X\n", rear3_paf_err_data_result);
	if (rc)
		return rc;
	return 0;
}
#endif
uint32_t front_paf_err_data_result = 0xFFFFFFFF;
static ssize_t front_paf_cal_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("front_paf_err_data_result : %u\n", front_paf_err_data_result);
	rc = scnprintf(buf, PAGE_SIZE, "%08X\n", front_paf_err_data_result);
	if (rc)
		return rc;
	return 0;
}

#if 0 //EARLY_RETENTION
static ssize_t rear_early_retention_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	rc = cam_sensor_early_retention();

	if (rc == 0) {
		rc = scnprintf(buf, PAGE_SIZE, "%s\n", "success");
	} else {
		rc = scnprintf(buf, PAGE_SIZE, "%s\n", "fail");
	}
	pr_info("%s: result : %s\n", __func__, buf);

	if (rc)
		return rc;
	return 0;
}
#endif
#if !(defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM) || defined(CONFIG_SAMSUNG_FRONT_FIXED_FOCUS))
uint32_t front_af_cal_pan;
uint32_t front_af_cal_macro;
static ssize_t front_camera_afcal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_af_cal_pan: %d, front_af_cal_macro: %d\n", front_af_cal_pan, front_af_cal_macro);
	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d\n", front_af_cal_macro, front_af_cal_pan);
	if (rc)
		return rc;
	return 0;
}
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
uint32_t front3_af_cal_pan;
uint32_t front3_af_cal_macro;
static ssize_t front3_camera_afcal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front3_af_cal_pan: %d, front_af_cal_macro: %d\n", front3_af_cal_pan, front3_af_cal_macro);
	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d\n", front3_af_cal_macro, front3_af_cal_pan);
	if (rc)
		return rc;
	return 0;
}
#else
uint32_t front2_af_cal_pan;
uint32_t front2_af_cal_macro;
static ssize_t front2_camera_afcal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front2_af_cal_pan: %d, front2_af_cal_macro: %d\n", front2_af_cal_pan, front2_af_cal_macro);
	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d\n", front2_af_cal_macro, front2_af_cal_pan);
	if (rc)
		return rc;
	return 0;
}
#endif
#endif
#endif

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front_cam_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";
#else
char front_cam_fw_ver[SYSFS_FW_VER_SIZE] = "IMX374 N\n";
#endif
static ssize_t front_camera_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_cam_fw_ver : %s\n", front_cam_fw_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_cam_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_camera_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_cam_fw_ver, sizeof(front_cam_fw_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";
#else
char front_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "IMX374 N N\n";
#endif
static ssize_t front_camera_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_cam_fw_full_ver : %s\n", front_cam_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_cam_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_camera_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_cam_fw_full_ver, sizeof(front_cam_fw_full_ver), "%s", buf);
	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
#else
char front_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "OK\n";
#endif
static ssize_t front_camera_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_fw_ver : %s\n", front_cam_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_cam_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_camera_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_cam_fw_user_ver, sizeof(front_cam_fw_user_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
#else
char front_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "OK\n";
#endif
static ssize_t front_camera_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_fw_ver : %s\n", front_cam_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_cam_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_camera_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_cam_fw_factory_ver, sizeof(front_cam_fw_factory_ver), "%s", buf);

	return size;
}
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front2_cam_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";
#else
char front2_cam_fw_ver[SYSFS_FW_VER_SIZE] = "S5K4HA N\n";
#endif
static ssize_t front2_camera_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front2_cam_fw_ver : %s\n", front2_cam_fw_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_cam_fw_ver, sizeof(front2_cam_fw_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front2_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";
#else
char front2_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "S5K4HA N N\n";
#endif
static ssize_t front2_camera_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_cam_fw_full_ver : %s\n", front2_cam_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_cam_fw_full_ver, sizeof(front2_cam_fw_full_ver), "%s", buf);
	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front2_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
#else
char front2_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "OK\n";
#endif
static ssize_t front2_camera_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_fw_ver : %s\n", front2_cam_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_cam_fw_user_ver, sizeof(front2_cam_fw_user_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front2_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
#else
char front2_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "OK\n";
#endif
static ssize_t front2_camera_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_fw_ver : %s\n", front2_cam_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_cam_fw_factory_ver, sizeof(front2_cam_fw_factory_ver), "%s", buf);

	return size;
}
#endif

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
char front3_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front3_cam_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";
#else
char front3_cam_fw_ver[SYSFS_FW_VER_SIZE] = "IMX374 N\n";
#endif
static ssize_t front3_camera_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front3_cam_fw_ver : %s\n", front3_cam_fw_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front3_cam_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front3_camera_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front3_cam_fw_ver, sizeof(front3_cam_fw_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front3_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";
#else
char front3_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "IMX374 N N\n";
#endif
static ssize_t front3_camera_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front3_cam_fw_full_ver : %s\n", front3_cam_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front3_cam_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front3_camera_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front3_cam_fw_full_ver, sizeof(front3_cam_fw_full_ver), "%s", buf);
	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front3_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
#else
char front3_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "OK\n";
#endif
static ssize_t front3_camera_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_fw_ver : %s\n", front3_cam_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front3_cam_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front3_camera_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front3_cam_fw_user_ver, sizeof(front3_cam_fw_user_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front3_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
#else
char front3_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "OK\n";
#endif
static ssize_t front3_camera_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_fw_ver : %s\n", front3_cam_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front3_cam_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front3_camera_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front3_cam_fw_factory_ver, sizeof(front3_cam_fw_factory_ver), "%s", buf);

	return size;
}
#else
#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front2_cam_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";
#else
char front2_cam_fw_ver[SYSFS_FW_VER_SIZE] = "IMX374 N\n";
#endif
static ssize_t front2_camera_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front2_cam_fw_ver : %s\n", front2_cam_fw_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_cam_fw_ver, sizeof(front2_cam_fw_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front2_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";
#else
char front2_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "IMX374 N N\n";
#endif
static ssize_t front2_camera_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front2_cam_fw_full_ver : %s\n", front2_cam_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_cam_fw_full_ver, sizeof(front2_cam_fw_full_ver), "%s", buf);
	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front2_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
#else
char front2_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "OK\n";
#endif
static ssize_t front2_camera_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_fw_ver : %s\n", front2_cam_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_cam_fw_user_ver, sizeof(front2_cam_fw_user_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_EEPROM)
char front2_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
#else
char front2_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "OK\n";
#endif
static ssize_t front2_camera_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_fw_ver : %s\n", front2_cam_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front2_cam_fw_factory_ver, sizeof(front2_cam_fw_factory_ver), "%s", buf);

	return size;
}
#endif
#endif

#if defined(CONFIG_CAMERA_SYSFS_V2)
char rear_cam_info[150] = "NULL\n";	//camera_info
static ssize_t rear_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_info : %s\n", rear_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", rear_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear_cam_info, sizeof(rear_cam_info), "%s", buf);

	return size;
}

char front_cam_info[150] = "NULL\n";	//camera_info
static ssize_t front_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_info : %s\n", front_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front_cam_info, sizeof(front_cam_info), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
char front2_cam_info[150] = "NULL\n";	//camera_info
static ssize_t front2_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_info : %s\n", front2_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front2_cam_info, sizeof(front2_cam_info), "%s", buf);

	return size;
}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
char front3_cam_info[150] = "NULL\n";	//camera_info
static ssize_t front3_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_info : %s\n", front3_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front3_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front3_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front3_cam_info, sizeof(front3_cam_info), "%s", buf);

	return size;
}
#else
char front2_cam_info[150] = "NULL\n";	//camera_info
static ssize_t front2_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_info : %s\n", front2_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front2_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front2_cam_info, sizeof(front2_cam_info), "%s", buf);

	return size;
}
#endif
#endif
#endif

#if defined(CONFIG_SAMSUNG_FRONT_DUAL) || defined(CONFIG_SAMSUNG_FRONT_TOF)
uint8_t front2_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
static ssize_t front2_camera_moduleid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front2_module_id : %c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  front2_module_id[0], front2_module_id[1], front2_module_id[2], front2_module_id[3], front2_module_id[4],
	  front2_module_id[5], front2_module_id[6], front2_module_id[7], front2_module_id[8], front2_module_id[9]);
	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  front2_module_id[0], front2_module_id[1], front2_module_id[2], front2_module_id[3], front2_module_id[4],
	  front2_module_id[5], front2_module_id[6], front2_module_id[7], front2_module_id[8], front2_module_id[9]);
}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
uint8_t front3_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
static ssize_t front3_camera_moduleid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front3_module_id : %c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  front3_module_id[0], front3_module_id[1], front3_module_id[2], front3_module_id[3], front3_module_id[4],
	  front3_module_id[5], front3_module_id[6], front3_module_id[7], front3_module_id[8], front3_module_id[9]);
	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  front3_module_id[0], front3_module_id[1], front3_module_id[2], front3_module_id[3], front3_module_id[4],
	  front3_module_id[5], front3_module_id[6], front3_module_id[7], front3_module_id[8], front3_module_id[9]);
}
#else
uint8_t front2_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
static ssize_t front2_camera_moduleid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front2_module_id : %c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  front2_module_id[0], front2_module_id[1], front2_module_id[2], front2_module_id[3], front2_module_id[4],
	  front2_module_id[5], front2_module_id[6], front2_module_id[7], front2_module_id[8], front2_module_id[9]);
	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  front2_module_id[0], front2_module_id[1], front2_module_id[2], front2_module_id[3], front2_module_id[4],
	  front2_module_id[5], front2_module_id[6], front2_module_id[7], front2_module_id[8], front2_module_id[9]);
}
#endif
#endif

char supported_camera_ids[128];
static ssize_t supported_camera_ids_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("supported_camera_ids : %s\n", supported_camera_ids);
	rc = scnprintf(buf, PAGE_SIZE, "%s", supported_camera_ids);
	if (rc)
		return rc;
	return 0;
}

static ssize_t supported_camera_ids_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(supported_camera_ids, sizeof(supported_camera_ids), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
char iris_cam_fw_ver[SYSFS_FW_VER_SIZE] = "UNKNOWN N\n";
static ssize_t iris_camera_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] iris_cam_fw_ver : %s\n", iris_cam_fw_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", iris_cam_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t iris_camera_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(iris_cam_fw_ver, sizeof(iris_cam_fw_ver), "%s", buf);
	return size;
}

char iris_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "UNKNOWN N N\n";
static ssize_t iris_camera_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] iris_cam_fw_full_ver : %s\n", iris_cam_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", iris_cam_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t iris_camera_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(iris_cam_fw_full_ver, sizeof(iris_cam_fw_full_ver), "%s", buf);
	return size;
}

char iris_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "NG\n";
static ssize_t iris_camera_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] iris_cam_fw_user_ver : %s\n", iris_cam_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", iris_cam_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t iris_camera_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(iris_cam_fw_user_ver, sizeof(iris_cam_fw_user_ver), "%s", buf);
	return size;
}

char iris_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NG_VER\n";
static ssize_t iris_camera_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] iris_cam_fw_factory_ver : %s\n", iris_cam_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", iris_cam_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t iris_camera_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(iris_cam_fw_factory_ver, sizeof(iris_cam_fw_factory_ver), "%s", buf);
	return size;
}

#if defined(CONFIG_CAMERA_SYSFS_V2)
char iris_cam_info[150] = "NULL\n";	//camera_info
static ssize_t iris_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] iris_cam_info : %s\n", iris_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", iris_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t iris_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front_cam_info, sizeof(front_cam_info), "%s", buf);
	return size;
}
#endif
#endif

#define FROM_SENSOR_ID_SIZE 16
char rear_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
static ssize_t rear_sensorid_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear_sensor_id : %s\n", rear_sensor_id);
	memcpy(buf, rear_sensor_id, FROM_SENSOR_ID_SIZE);
	return FROM_SENSOR_ID_SIZE;
}

static ssize_t rear_sensorid_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear_sensor_id, sizeof(rear_sensor_id), "%s", buf);

	return size;
}

char front_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
static ssize_t front_sensorid_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front_sensor_id : %s\n", front_sensor_id);
	memcpy(buf, front_sensor_id, FROM_SENSOR_ID_SIZE);
	return FROM_SENSOR_ID_SIZE;
}

static ssize_t front_sensorid_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front_sensor_id, sizeof(front_sensor_id), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
char front2_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
static ssize_t front2_sensorid_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front2_sensor_id : %s\n", front2_sensor_id);
	memcpy(buf, front2_sensor_id, FROM_SENSOR_ID_SIZE);
	return FROM_SENSOR_ID_SIZE;
}

static ssize_t front2_sensorid_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front2_sensor_id, sizeof(front2_sensor_id), "%s", buf);

	return size;
}
#endif

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
char front3_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
static ssize_t front3_sensorid_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front3_sensor_id : %s\n", front3_sensor_id);
	memcpy(buf, front3_sensor_id, FROM_SENSOR_ID_SIZE);
	return FROM_SENSOR_ID_SIZE;
}

static ssize_t front3_sensorid_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front3_sensor_id, sizeof(front3_sensor_id), "%s", buf);

	return size;
}
#else
char front2_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
static ssize_t front2_sensorid_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front2_sensor_id : %s\n", front2_sensor_id);
	memcpy(buf, front2_sensor_id, FROM_SENSOR_ID_SIZE);
	return FROM_SENSOR_ID_SIZE;
}

static ssize_t front2_sensorid_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front2_sensor_id, sizeof(front2_sensor_id), "%s", buf);

	return size;
}
#endif
#endif

#define FROM_MTF_SIZE 54
char front_mtf_exif[FROM_MTF_SIZE + 1] = "\0";
static ssize_t front_mtf_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front_mtf_exif : %s\n", front_mtf_exif);
	memcpy(buf, front_mtf_exif, FROM_MTF_SIZE);
	return FROM_MTF_SIZE;
}

static ssize_t front_mtf_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front_mtf_exif, sizeof(front_mtf_exif), "%s", buf);

	return size;
}

char rear_mtf_exif[FROM_MTF_SIZE + 1] = "\0";
static ssize_t rear_mtf_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear_mtf_exif : %s\n", rear_mtf_exif);
	memcpy(buf, rear_mtf_exif, FROM_MTF_SIZE);
	return FROM_MTF_SIZE;
}

static ssize_t rear_mtf_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear_mtf_exif, sizeof(rear_mtf_exif), "%s", buf);

	return size;
}

char rear_mtf2_exif[FROM_MTF_SIZE + 1] = "\0";
static ssize_t rear_mtf2_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear_mtf2_exif : %s\n", rear_mtf2_exif);
	memcpy(buf, rear_mtf2_exif, FROM_MTF_SIZE);
	return FROM_MTF_SIZE;
}

static ssize_t rear_mtf2_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear_mtf2_exif, sizeof(rear_mtf2_exif), "%s", buf);

	return size;
}

uint8_t rear_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
static ssize_t rear_moduleid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear_module_id : %c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  rear_module_id[0], rear_module_id[1], rear_module_id[2], rear_module_id[3], rear_module_id[4],
	  rear_module_id[5], rear_module_id[6], rear_module_id[7], rear_module_id[8], rear_module_id[9]);
	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  rear_module_id[0], rear_module_id[1], rear_module_id[2], rear_module_id[3], rear_module_id[4],
	  rear_module_id[5], rear_module_id[6], rear_module_id[7], rear_module_id[8], rear_module_id[9]);
}

uint8_t front_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
static ssize_t front_camera_moduleid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front_module_id : %c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  front_module_id[0], front_module_id[1], front_module_id[2], front_module_id[3], front_module_id[4],
	  front_module_id[5], front_module_id[6], front_module_id[7], front_module_id[8], front_module_id[9]);
	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  front_module_id[0], front_module_id[1], front_module_id[2], front_module_id[3], front_module_id[4],
	  front_module_id[5], front_module_id[6], front_module_id[7], front_module_id[8], front_module_id[9]);
}

#define SSRM_CAMERA_INFO_SIZE 256
char ssrm_camera_info[SSRM_CAMERA_INFO_SIZE + 1] = "\0";
static ssize_t ssrm_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_info("ssrm_camera_info : %s\n", ssrm_camera_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", ssrm_camera_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t ssrm_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("ssrm_camera_info buf : %s\n", buf);
	scnprintf(ssrm_camera_info, sizeof(ssrm_camera_info), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
char cam3_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";//multi module
static ssize_t rear3_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam3_fw_ver : %s\n", cam3_fw_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam3_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear3_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam3_fw_ver, sizeof(cam3_fw_ver), "%s", buf);

	return size;
}

char cam3_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";//multi module
static ssize_t rear3_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam3_fw_full_ver : %s\n", cam3_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam3_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear3_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam3_fw_full_ver, sizeof(cam3_fw_full_ver), "%s", buf);

	return size;
}

#if 1
char rear3_af_cal_str[MAX_AF_CAL_STR_SIZE] = "";
static ssize_t rear3_afcal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	pr_debug("rear3_af_cal_str : 10 %s\n", rear3_af_cal_str);
	rc = scnprintf(buf, PAGE_SIZE, "10 %s", rear3_af_cal_str);
	if (rc)
		return rc;
#else
	pr_debug("rear3_af_cal_str : 1 %s\n", rear3_af_cal_str);
	rc = scnprintf(buf, PAGE_SIZE, "1 %s", rear3_af_cal_str);
	if (rc)
		return rc;
#endif

	return 0;
}
#else
int rear3_af_cal[FROM_REAR_AF_CAL_SIZE + 1] = {0,};
static ssize_t rear3_afcal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char tempbuf[10];
	char N[] = "N ";

	strncat(buf, "10 ", strlen("10 "));

#ifdef	FROM_REAR3_AF_CAL_D10_ADDR
	sprintf(tempbuf, "%d ", rear3_af_cal[1]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR3_AF_CAL_D20_ADDR
	sprintf(tempbuf, "%d ", rear3_af_cal[2]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR3_AF_CAL_D30_ADDR
	sprintf(tempbuf, "%d ", rear3_af_cal[3]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR3_AF_CAL_D40_ADDR
	sprintf(tempbuf, "%d ", rear3_af_cal[4]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR3_AF_CAL_D50_ADDR
	sprintf(tempbuf, "%d ", rear3_af_cal[5]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR3_AF_CAL_D60_ADDR
	sprintf(tempbuf, "%d ", rear3_af_cal[6]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR3_AF_CAL_D70_ADDR
	sprintf(tempbuf, "%d ", rear3_af_cal[7]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR3_AF_CAL_D80_ADDR
	sprintf(tempbuf, "%d ", rear3_af_cal[8]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef	FROM_REAR3_AF_CAL_PAN_ADDR
	sprintf(tempbuf, "%d ", rear3_af_cal[9]);
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

	return strlen(buf);
}
#endif

char rear3_cam_info[150] = "NULL\n";	//camera_info
static ssize_t rear3_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_info : %s\n", rear3_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", rear3_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear3_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear3_cam_info, sizeof(rear3_cam_info), "%s", buf);

	return size;
}

static ssize_t rear3_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
#if defined(CONFIG_SEC_Z3Q_PROJECT)
	char cam_type[] = "SONY_IMX586\n";
#elif  defined(CONFIG_SEC_C2Q_PROJECT)  || defined(CONFIG_SEC_F2Q_PROJECT) || defined(CONFIG_SEC_VICTORY_PROJECT)
	char cam_type[] = "SLSI_S5K3M5\n";
#elif defined(CONFIG_SEC_R8Q_PROJECT)
	char cam_type[] = "HYNIX_HI847\n";
#else
	char cam_type[] = "SLSI_S5KGW2\n";
#endif

	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_type);
	if (rc)
		return rc;
	return 0;
}

char rear3_mtf_exif[FROM_MTF_SIZE + 1] = "\0";
static ssize_t rear3_mtf_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear3_mtf_exif : %s\n", rear3_mtf_exif);
	memcpy(buf, rear3_mtf_exif, FROM_MTF_SIZE);
	return FROM_MTF_SIZE;
}

static ssize_t rear3_mtf_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear3_mtf_exif, sizeof(rear3_mtf_exif), "%s", buf);

	return size;
}

char rear3_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
static ssize_t rear3_sensorid_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear3_sensor_id : %s\n", rear3_sensor_id);
	memcpy(buf, rear3_sensor_id, FROM_SENSOR_ID_SIZE);
	return FROM_SENSOR_ID_SIZE;
}

static ssize_t rear3_sensorid_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear3_sensor_id, sizeof(rear3_sensor_id), "%s", buf);

	return size;
}

#define FROM_REAR_DUAL_CAL_SIZE 2060
uint8_t rear3_dual_cal[FROM_REAR_DUAL_CAL_SIZE + 1] = "\0";
static ssize_t rear3_dual_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *ret = NULL;
	int copy_size = 0;

	pr_debug("[FW_DBG] rear3_dual_cal : %s\n", rear3_dual_cal);

	if (FROM_REAR_DUAL_CAL_SIZE > SYSFS_MAX_READ_SIZE)
		copy_size = SYSFS_MAX_READ_SIZE;
	else
		copy_size = FROM_REAR_DUAL_CAL_SIZE;

	ret = memcpy(buf, rear3_dual_cal, copy_size);

	if (ret)
		return copy_size;

	return 0;

}


uint32_t rear3_dual_cal_size = FROM_REAR_DUAL_CAL_SIZE;
static ssize_t rear3_dual_cal_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear3_dual_cal_size : %d\n", rear3_dual_cal_size);
	rc = scnprintf(buf, PAGE_SIZE, "%d", rear3_dual_cal_size);
	if (rc)
		return rc;
	return 0;
}

#if 1
DualTilt_t rear3_dual;
static ssize_t rear3_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear3 dual tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d, project_cal_type=%s\n",
		rear3_dual.x, rear3_dual.y, rear3_dual.z, rear3_dual.sx, rear3_dual.sy,
		rear3_dual.range, rear3_dual.max_err, rear3_dual.avg_err, rear3_dual.dll_ver, rear3_dual.project_cal_type);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d %s\n", rear3_dual.x, rear3_dual.y,
			rear3_dual.z, rear3_dual.sx, rear3_dual.sy, rear3_dual.range,
			rear3_dual.max_err, rear3_dual.avg_err, rear3_dual.dll_ver, rear3_dual.project_cal_type);
	if (rc)
		return rc;
	return 0;
}
#else
int rear3_dual_tilt_x;
int rear3_dual_tilt_y;
int rear3_dual_tilt_z;
int rear3_dual_tilt_sx;
int rear3_dual_tilt_sy;
int rear3_dual_tilt_range;
int rear3_dual_tilt_max_err;
int rear3_dual_tilt_avg_err;
int rear3_dual_tilt_dll_ver;
static ssize_t rear3_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear dual tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		rear3_dual_tilt_x, rear3_dual_tilt_y, rear3_dual_tilt_z, rear3_dual_tilt_sx, rear3_dual_tilt_sy,
		rear3_dual_tilt_range, rear3_dual_tilt_max_err, rear3_dual_tilt_avg_err, rear3_dual_tilt_dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", rear3_dual_tilt_x, rear3_dual_tilt_y,
			rear3_dual_tilt_z, rear3_dual_tilt_sx, rear3_dual_tilt_sy, rear3_dual_tilt_range,
			rear3_dual_tilt_max_err, rear3_dual_tilt_avg_err, rear3_dual_tilt_dll_ver);
	if (rc)
		return rc;
	return 0;
}
#endif

uint8_t rear3_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
static ssize_t rear3_moduleid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear3_module_id : %c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  rear3_module_id[0], rear3_module_id[1], rear3_module_id[2], rear3_module_id[3], rear3_module_id[4],
	  rear3_module_id[5], rear3_module_id[6], rear3_module_id[7], rear3_module_id[8], rear3_module_id[9]);
	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  rear3_module_id[0], rear3_module_id[1], rear3_module_id[2], rear3_module_id[3], rear3_module_id[4],
	  rear3_module_id[5], rear3_module_id[6], rear3_module_id[7], rear3_module_id[8], rear3_module_id[9]);
}
#endif

char module3_info[SYSFS_MODULE_INFO_SIZE] = "NULL\n";
static ssize_t rear3_module_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] module3_info : %s\n", module3_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", module3_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear3_module_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(module3_info, sizeof(module3_info), "%s", buf);

	return size;
}
#endif

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
char rear2_cam_info[150] = "NULL\n";	//camera_info
static ssize_t rear2_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_info : %s\n", rear2_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", rear2_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear2_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear2_cam_info, sizeof(rear2_cam_info), "%s", buf);

	return size;
}

char rear2_mtf_exif[FROM_MTF_SIZE + 1] = "\0";
static ssize_t rear2_mtf_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear2_mtf_exif : %s\n", rear2_mtf_exif);
	memcpy(buf, rear2_mtf_exif, FROM_MTF_SIZE);
	return FROM_MTF_SIZE;
}

static ssize_t rear2_mtf_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear2_mtf_exif, sizeof(rear2_mtf_exif), "%s", buf);

	return size;
}

char rear2_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
static ssize_t rear2_sensorid_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear2_sensor_id : %s\n", rear2_sensor_id);
	memcpy(buf, rear2_sensor_id, FROM_SENSOR_ID_SIZE);
	return FROM_SENSOR_ID_SIZE;
}

static ssize_t rear2_sensorid_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear2_sensor_id, sizeof(rear2_sensor_id), "%s", buf);

	return size;
}

char module2_info[SYSFS_MODULE_INFO_SIZE] = "NULL\n";
static ssize_t rear2_module_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] module2_info : %s\n", module2_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", module2_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear2_module_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(module2_info, sizeof(module2_info), "%s", buf);

	return size;
}

uint8_t rear2_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
static ssize_t rear2_moduleid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear2_module_id : %c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  rear2_module_id[0], rear2_module_id[1], rear2_module_id[2], rear2_module_id[3], rear2_module_id[4],
	  rear2_module_id[5], rear2_module_id[6], rear2_module_id[7], rear2_module_id[8], rear2_module_id[9]);
	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  rear2_module_id[0], rear2_module_id[1], rear2_module_id[2], rear2_module_id[3], rear2_module_id[4],
	  rear2_module_id[5], rear2_module_id[6], rear2_module_id[7], rear2_module_id[8], rear2_module_id[9]);
}

char cam2_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";//multi module
static ssize_t rear2_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	pr_debug("[FW_DBG] cam2_fw_ver : %s\n", cam2_fw_ver);

	rc = scnprintf(buf, PAGE_SIZE, "%s", cam2_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear2_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam2_fw_ver, sizeof(cam2_fw_ver), "%s", buf);

	return size;
}


char cam2_fw_user_ver[SYSFS_FW_VER_SIZE] = "NULL\n";//multi module
static ssize_t rear2_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam2_fw_user_ver : %s\n", cam2_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam2_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear2_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{

	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam2_fw_user_ver, sizeof(cam2_fw_user_ver), "%s", buf);

	return size;
}

char cam2_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NULL\n";//multi module
static ssize_t rear2_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam2_fw_factory_ver : %s\n", cam2_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam2_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear2_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam2_fw_factory_ver, sizeof(cam2_fw_factory_ver), "%s", buf);

	return size;
}

char cam2_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";//multi module
static ssize_t rear2_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam2_fw_full_ver : %s\n", cam2_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam2_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear2_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam2_fw_full_ver, sizeof(cam2_fw_full_ver), "%s", buf);

	return size;
}

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
uint8_t rear2_dual_cal[FROM_REAR_DUAL_CAL_SIZE + 1] = "\0";
static ssize_t rear2_dual_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *ret = NULL;
	int copy_size = 0;

	pr_debug("[FW_DBG] rear2_dual_cal : %s\n", rear2_dual_cal);

	if (FROM_REAR_DUAL_CAL_SIZE > SYSFS_MAX_READ_SIZE)
		copy_size = SYSFS_MAX_READ_SIZE;
	else
		copy_size = FROM_REAR_DUAL_CAL_SIZE;

	ret = memcpy(buf, rear2_dual_cal, copy_size);

	if (ret)
		return copy_size;

	return 0;

}

uint32_t rear2_dual_cal_size = FROM_REAR_DUAL_CAL_SIZE;
static ssize_t rear2_dual_cal_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_info("[FW_DBG] rear2_dual_cal_size : %d\n", rear2_dual_cal_size);
	rc = scnprintf(buf, PAGE_SIZE, "%d", rear2_dual_cal_size);
	if (rc)
		return rc;
	return 0;
}

#if 1
DualTilt_t rear2_dual;
static ssize_t rear2_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear2 dual tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d, project_cal_type=%s\n",
		rear2_dual.x, rear2_dual.y, rear2_dual.z, rear2_dual.sx, rear2_dual.sy,
		rear2_dual.range, rear2_dual.max_err, rear2_dual.avg_err, rear2_dual.dll_ver, rear2_dual.project_cal_type);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d %s\n", rear2_dual.x, rear2_dual.y,
			rear2_dual.z, rear2_dual.sx, rear2_dual.sy, rear2_dual.range,
			rear2_dual.max_err, rear2_dual.avg_err, rear2_dual.dll_ver, rear2_dual.project_cal_type);
	if (rc)
		return rc;
	return 0;
}
#else
int rear2_dual_tilt_x;
int rear2_dual_tilt_y;
int rear2_dual_tilt_z;
int rear2_dual_tilt_sx;
int rear2_dual_tilt_sy;
int rear2_dual_tilt_range;
int rear2_dual_tilt_max_err;
int rear2_dual_tilt_avg_err;
int rear2_dual_tilt_dll_ver;
static ssize_t rear2_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear dual tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		rear2_dual_tilt_x, rear2_dual_tilt_y, rear2_dual_tilt_z, rear2_dual_tilt_sx, rear2_dual_tilt_sy,
		rear2_dual_tilt_range, rear2_dual_tilt_max_err, rear2_dual_tilt_avg_err, rear2_dual_tilt_dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", rear2_dual_tilt_x, rear2_dual_tilt_y,
			rear2_dual_tilt_z, rear2_dual_tilt_sx, rear2_dual_tilt_sy, rear2_dual_tilt_range,
			rear2_dual_tilt_max_err, rear2_dual_tilt_avg_err, rear2_dual_tilt_dll_ver);
	if (rc)
		return rc;
	return 0;
}
#endif
#endif

static ssize_t rear2_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
	char cam_type[] = "SLSI_S5K2L3\n";
#elif defined(CONFIG_SEC_GTS7XL_PROJECT) || defined(CONFIG_SEC_GTS7L_PROJECT)
	char cam_type[] = "SLSI_S5K5E9\n";
#elif defined(CONFIG_SEC_BLOOMXQ_PROJECT) || defined(CONFIG_SEC_F2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT) || defined(CONFIG_SEC_VICTORY_PROJECT)
	char cam_type[] = "SLSI_S5K3L6\n";

#else
	char cam_type[] = "SLSI_S5K2LA\n";
#endif

	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_type);
	if (rc)
		return rc;
	return 0;
}
#endif

#if defined(CONFIG_SAMSUNG_ACTUATOR_PREVENT_SHAKING)
static int actuator_power = 0;
#endif
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
static int ois_power = 0;
extern struct cam_ois_ctrl_t *g_o_ctrl;
extern struct cam_actuator_ctrl_t *g_a_ctrls[2];
#if defined(CONFIG_SAMSUNG_APERTURE)
extern struct cam_aperture_ctrl_t *g_cam_aperture_t;
#endif
uint32_t ois_autotest_threshold = 150;
static ssize_t ois_autotest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool ret = false;
	int result = 0;
	bool x1_result = true, y1_result = true;
	int cnt = 0;
	struct cam_ois_sinewave_t sinewave[1];

	pr_info("%s: E\n", __func__);

	if (g_a_ctrls[0] != NULL)
		cam_actuator_power_up(g_a_ctrls[0]);
	msleep(100);

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
	cam_actuator_move_for_ois_test(g_a_ctrls[0]);
#endif

	ret = cam_ois_sine_wavecheck(g_o_ctrl, ois_autotest_threshold, sinewave, &result, 1);
	if (ret)
		x1_result = y1_result = true;
	else {
		if (result & 0x01) {
			// Module#1 X Axis Fail
			x1_result = false;
		}
		if (result & 0x02) {
			// Module#1 Y Axis Fail
			y1_result = false;
		}
	}

	cnt = scnprintf(buf, PAGE_SIZE, "%s, %d, %s, %d",
		(x1_result ? "pass" : "fail"), (x1_result ? 0 : sinewave[0].sin_x),
		(y1_result ? "pass" : "fail"), (y1_result ? 0 : sinewave[0].sin_y));
	pr_info("%s: result : %s\n", __func__, buf);

	if (g_a_ctrls[0] != NULL)
		cam_actuator_power_down(g_a_ctrls[0]);

	pr_info("%s: X\n", __func__);

	if (cnt)
		return cnt;
	return 0;
}

static ssize_t ois_autotest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	uint32_t value = 0;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;
	ois_autotest_threshold = value;
	return size;
}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static ssize_t ois_autotest_2nd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool ret = false;
	int result = 0;
	bool x1_result = true, y1_result = true, x2_result = true, y2_result = true;
	int cnt = 0, i = 0;
	struct cam_ois_sinewave_t sinewave[2];

	if (ois_power == 0) {
		pr_info("%s: [WARNING] ois is off, skip", __func__);
		return 0;
	}

	pr_info("%s: E\n", __func__);

	for (i = 0; i < 2; i++) {
		if (g_a_ctrls[i] != NULL) {
			cam_actuator_power_up(g_a_ctrls[i]);
			msleep(5);
			cam_actuator_move_for_ois_test(g_a_ctrls[i]);
		}
	}
	msleep(100);

	ret = cam_ois_sine_wavecheck(g_o_ctrl, ois_autotest_threshold, sinewave, &result, 2); //two at once
	if (ret)
		x1_result = y1_result = x2_result = y2_result = true;
	else {
		if (result & 0x01) {
			// Module#1 X Axis Fail
			x1_result = false;
		}
		if (result & 0x02) {
			// Module#1 Y Axis Fail
			y1_result = false;
		}
		if ((result >> 4) & 0x01) {
			// Module#2 X Axis Fail
			x2_result = false;
		}
		if ((result >> 4) & 0x02) {
			// Module#2 Y Axis Fail
			y2_result = false;
		}
	}
	cnt = sprintf(buf, "%s, %d, %s, %d",
		(x1_result ? "pass" : "fail"), (x1_result ? 0 : sinewave[0].sin_x),
		(y1_result ? "pass" : "fail"), (y1_result ? 0 : sinewave[0].sin_y));
	cnt += sprintf(buf + cnt, ", %s, %d, %s, %d",
		(x2_result ? "pass" : "fail"), (x2_result ? 0 : sinewave[1].sin_x),
		(y2_result ? "pass" : "fail"), (y2_result ? 0 : sinewave[1].sin_y));
	pr_info("%s: result : %s\n", __func__, buf);

	for (i = 0; i < 2; i++) {
		if (g_a_ctrls[i] != NULL)
			cam_actuator_power_down(g_a_ctrls[i]);
	}
	pr_info("%s: X\n", __func__);

	return cnt;
}

static ssize_t ois_autotest_2nd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	uint32_t value = 0;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;
	ois_autotest_threshold = value;
	return size;
}
#endif

static ssize_t ois_power_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (g_o_ctrl == NULL ||
		((g_o_ctrl->io_master_info.master_type == I2C_MASTER) &&
		(g_o_ctrl->io_master_info.client == NULL)) ||
		((g_o_ctrl->io_master_info.master_type == CCI_MASTER) &&
		(g_o_ctrl->io_master_info.cci_client == NULL)))
		return size;

	mutex_lock(&(g_o_ctrl->ois_mutex));
	if (g_o_ctrl->cam_ois_state != CAM_OIS_INIT) {
		pr_err("%s: Not in right state to control OIS power %d",
			__func__, g_o_ctrl->cam_ois_state);
		goto error;
	}

	switch (buf[0]) {
	case '0':
		if (ois_power == 0) {
			pr_info("%s: [WARNING] ois is off, skip power down", __func__);
			goto error;
		}
		cam_ois_power_down(g_o_ctrl);
		pr_info("%s: power down", __func__);
		ois_power = 0;
		break;
	case '1':
#if defined(CONFIG_SAMSUNG_ACTUATOR_PREVENT_SHAKING)
		if (actuator_power > 0) {
			pr_info("%s: [WARNING] actuator is used", __func__);
			goto error;
		}
#endif

		ois_power = 1;

		cam_ois_power_up(g_o_ctrl);
		msleep(200);
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
		cam_ois_mcu_init(g_o_ctrl);
#endif
		g_o_ctrl->ois_mode = 0;
		pr_info("%s: power up", __func__);
		break;

	default:
		break;
	}

error:
	mutex_unlock(&(g_o_ctrl->ois_mutex));
	return size;
}

static ssize_t gyro_calibration_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int result = 0;
	long raw_data_x = 0, raw_data_y = 0;

	result = cam_ois_gyro_sensor_calibration(g_o_ctrl, &raw_data_x, &raw_data_y);

	if (raw_data_x < 0 && raw_data_y < 0) {
		return scnprintf(buf, PAGE_SIZE, "%d,-%ld.%03ld,-%ld.%03ld\n", result, abs(raw_data_x / 1000),
			abs(raw_data_x % 1000), abs(raw_data_y / 1000), abs(raw_data_y % 1000));
	} else if (raw_data_x < 0) {
		return scnprintf(buf, PAGE_SIZE, "%d,-%ld.%03ld,%ld.%03ld\n", result, abs(raw_data_x / 1000),
			abs(raw_data_x % 1000), raw_data_y / 1000, raw_data_y % 1000);
	} else if (raw_data_y < 0) {
		return scnprintf(buf, PAGE_SIZE, "%d,%ld.%03ld,-%ld.%03ld\n", result, raw_data_x / 1000,
			raw_data_x % 1000, abs(raw_data_y / 1000), abs(raw_data_y % 1000));
	} else {
		return scnprintf(buf, PAGE_SIZE, "%d,%ld.%03ld,%ld.%03ld\n", result, raw_data_x / 1000,
			raw_data_x % 1000, raw_data_y / 1000, raw_data_y % 1000);
	}
}

long raw_init_x = 0, raw_init_y = 0;

static ssize_t gyro_selftest_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	int result_total = 0, result = 0;
	bool result_offset = 0, result_selftest = 0;
	uint32_t selftest_ret = 0;
	long raw_data_x = 0, raw_data_y = 0;
	int OIS_GYRO_OFFSET_SPEC = 10000;

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
	result = cam_ois_offset_test(g_o_ctrl, &raw_data_x, &raw_data_y, 1);
#else
	cam_ois_offset_test(g_o_ctrl, &raw_data_x, &raw_data_y, 1);
#endif
	msleep(50);
	selftest_ret = cam_ois_self_test(g_o_ctrl);

	if (selftest_ret == 0x0)
		result_selftest = true;
	else
		result_selftest = false;

	if ((result < 0) ||
		abs(raw_init_x - raw_data_x) > OIS_GYRO_OFFSET_SPEC ||
		abs(raw_init_y - raw_data_y) > OIS_GYRO_OFFSET_SPEC)
		result_offset = false;
	else
		result_offset = true;

	if (result_offset && result_selftest)
		result_total = 0;
	else if (!result_offset && !result_selftest)
		result_total = 3;
	else if (!result_offset)
		result_total = 1;
	else if (!result_selftest)
		result_total = 2;

	pr_info("%s: Result : 0 (success), 1 (offset fail), 2 (selftest fail) , 3 (both fail)\n", __func__);
	sprintf(buf, "Result : %d, result x = %ld.%03ld, result y = %ld.%03ld\n",
		result_total, raw_data_x / 1000, (long int)abs(raw_data_x % 1000),
		raw_data_y / 1000, (long int)abs(raw_data_y % 1000));
	pr_info("%s", buf);

	if (raw_data_x < 0 && raw_data_y < 0) {
		rc = scnprintf(buf, PAGE_SIZE, "%d,-%ld.%03ld,-%ld.%03ld\n", result_total,
			(long int)abs(raw_data_x / 1000), (long int)abs(raw_data_x % 1000),
			(long int)abs(raw_data_y / 1000), (long int)abs(raw_data_y % 1000));
	} else if (raw_data_x < 0) {
		rc = scnprintf(buf, PAGE_SIZE, "%d,-%ld.%03ld,%ld.%03ld\n", result_total,
			(long int)abs(raw_data_x / 1000), (long int)abs(raw_data_x % 1000),
			raw_data_y / 1000, raw_data_y % 1000);
	} else if (raw_data_y < 0) {
		rc = scnprintf(buf, PAGE_SIZE, "%d,%ld.%03ld,-%ld.%03ld\n", result_total,
			raw_data_x / 1000, raw_data_x % 1000,
			(long int)abs(raw_data_y / 1000), (long int)abs(raw_data_y % 1000));
	} else {
		rc = scnprintf(buf, PAGE_SIZE, "%d,%ld.%03ld,%ld.%03ld\n",
			result_total, raw_data_x / 1000, raw_data_x % 1000,
			raw_data_y / 1000, raw_data_y % 1000);
	}

	if (rc)
		return rc;
	return 0;
}

static ssize_t gyro_rawdata_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	long raw_data_x = 0, raw_data_y = 0;

	raw_init_x = 0;
	raw_init_y = 0;

	cam_ois_get_offset_data(g_o_ctrl, &raw_data_x, &raw_data_y);

	raw_init_x = raw_data_x;
	raw_init_y = raw_data_y;

	pr_info("%s: raw data x = %ld.%03ld, raw data y = %ld.%03ld\n", __func__,
		raw_data_x / 1000, raw_data_x % 1000,
		raw_data_y / 1000, raw_data_y % 1000);

	if (raw_data_x < 0 && raw_data_y < 0) {
		rc = scnprintf(buf, PAGE_SIZE, "-%ld.%03ld,-%ld.%03ld\n",
			(long int)abs(raw_data_x / 1000), (long int)abs(raw_data_x % 1000),
			(long int)abs(raw_data_y / 1000), (long int)abs(raw_data_y % 1000));
	} else if (raw_data_x < 0) {
		rc = scnprintf(buf, PAGE_SIZE, "-%ld.%03ld,%ld.%03ld\n",
			(long int)abs(raw_data_x / 1000), (long int)abs(raw_data_x % 1000),
			raw_data_y / 1000, raw_data_y % 1000);
	} else if (raw_data_y < 0) {
		rc = scnprintf(buf, PAGE_SIZE, "%ld.%03ld,-%ld.%03ld\n",
			raw_data_x / 1000, raw_data_x % 1000,
			(long int)abs(raw_data_y / 1000), (long int)abs(raw_data_y % 1000));
	} else {
		rc = scnprintf(buf, PAGE_SIZE, "%ld.%03ld,%ld.%03ld\n",
			raw_data_x / 1000, raw_data_x % 1000,
			raw_data_y / 1000, raw_data_y % 1000);
	}

	if (rc)
		return rc;
	return 0;
}

char ois_fw_full[SYSFS_FW_VER_SIZE] = "NULL NULL\n";
static ssize_t ois_fw_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_info("[FW_DBG] OIS_fw_ver : %s\n", ois_fw_full);
	rc = scnprintf(buf, PAGE_SIZE, "%s", ois_fw_full);
	if (rc)
		return rc;
	return 0;
}

static ssize_t ois_fw_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("[FW_DBG] buf : %s\n", buf);
	scnprintf(ois_fw_full, sizeof(ois_fw_full), "%s", buf);

	return size;
}

char ois_debug[40] = "NULL NULL NULL\n";
static ssize_t ois_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_info("[FW_DBG] ois_debug : %s\n", ois_debug);
	rc = scnprintf(buf, PAGE_SIZE, "%s", ois_debug);
	if (rc)
		return rc;
	return 0;
}

static ssize_t ois_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("[FW_DBG] buf: %s\n", buf);
	scnprintf(ois_debug, sizeof(ois_debug), "%s", buf);

	return size;
}

static ssize_t ois_reset_check(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	if (g_o_ctrl == NULL)
		return 0;

	pr_debug("ois reset_check : %d\n", g_o_ctrl->ois_mode);
	rc = scnprintf(buf, PAGE_SIZE, "%d", g_o_ctrl->ois_mode);
	return rc;
}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static ssize_t ois_hall_position_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
    uint32_t targetPosition[4] = { 0, 0, 0, 0};
    uint32_t hallPosition[4] = { 0, 0, 0, 0};

	rc = cam_ois_read_hall_position(g_o_ctrl, targetPosition, hallPosition);

	rc = scnprintf(buf, PAGE_SIZE, "%u,%u,%u,%u,%u,%u,%u,%u",
		targetPosition[0], targetPosition[1],
		targetPosition[2], targetPosition[3],
		hallPosition[0], hallPosition[1],
		hallPosition[2], hallPosition[3]);

	if (rc)
		return rc;
	return 0;
}
#endif

static ssize_t ois_set_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0;
	uint32_t mode = 0;

	if (g_o_ctrl == NULL || g_o_ctrl->io_master_info.client == NULL)
		return size;

	if (buf == NULL || kstrtouint(buf, 10, &mode))
		return -1;

	if (g_o_ctrl->is_power_up == false) {
		pr_err("%s: Fail, power down state",
			__func__);
		return -1;
	}

	mutex_lock(&(g_o_ctrl->ois_mutex));
	if (g_o_ctrl->cam_ois_state != CAM_OIS_INIT) {
		pr_err("%s: Not in right state to control OIS power %d",
			__func__, g_o_ctrl->cam_ois_state);
		goto error;
	}

	rc |= cam_ois_i2c_write(g_o_ctrl, 0x00BE, 0x03,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* select module */
	rc |= cam_ois_set_ois_mode(g_o_ctrl, mode); // Centering mode

error:
	mutex_unlock(&(g_o_ctrl->ois_mutex));
	return size;
}
#endif

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
extern uint8_t ois_wide_xygg[OIS_XYGG_SIZE];
extern uint8_t ois_wide_cal_mark;
int ois_gain_rear_result = 2; //0:normal, 1: No cal, 2: rear cal fail
static ssize_t ois_gain_rear_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	uint32_t xgg = 0, ygg = 0;

	pr_info("[FW_DBG] ois_gain_rear_result : %d\n",
		ois_gain_rear_result);
	if (ois_gain_rear_result == 0) {
		memcpy(&xgg, &ois_wide_xygg[0], 4);
		memcpy(&ygg, &ois_wide_xygg[4], 4);
		rc = scnprintf(buf, PAGE_SIZE, "%d,0x%x,0x%x",
			ois_gain_rear_result, xgg, ygg);
	} else {
		rc = scnprintf(buf, PAGE_SIZE, "%d",
			ois_gain_rear_result);
	}
	if (rc)
		return rc;
	return 0;
}
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
extern uint8_t ois_tele_xygg[OIS_XYGG_SIZE];
extern uint8_t ois_tele_cal_mark;
int ois_gain_rear3_result = 2; //0:normal, 1: No cal, 2: rear cal fail
static ssize_t ois_gain_rear3_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	uint32_t xgg = 0, ygg = 0;

	pr_info("[FW_DBG] ois_gain_rear3_result : %d\n",
		ois_gain_rear3_result);
	if (ois_gain_rear3_result == 0) {
		memcpy(&xgg, &ois_tele_xygg[0], 4);
		memcpy(&ygg, &ois_tele_xygg[4], 4);
		rc = scnprintf(buf, PAGE_SIZE, "%d,0x%x,0x%x",
			ois_gain_rear3_result, xgg, ygg);
	} else {
		rc = scnprintf(buf, PAGE_SIZE, "%d",
			ois_gain_rear3_result);
	}
	if (rc)
		return rc;
	return 0;
}
#endif

extern uint8_t ois_wide_xysr[OIS_XYSR_SIZE];
int ois_sr_rear_result = 2; //0:normal, 1: No cal, 2: rear cal fail
static ssize_t ois_supperssion_ratio_rear_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	uint32_t xsr = 0, ysr = 0;

	pr_info("[FW_DBG] ois_sr_rear_result : %d\n",
		ois_sr_rear_result);
	if (ois_sr_rear_result == 0) {
		memcpy(&xsr, &ois_wide_xysr[0], 2);
		memcpy(&ysr, &ois_wide_xysr[2], 2);
		rc = scnprintf(buf, PAGE_SIZE, "%d,%u.%02u,%u.%02u",
			ois_sr_rear_result, (xsr / 100), (xsr % 100), (ysr / 100), (ysr % 100));
	} else {
		rc = scnprintf(buf, PAGE_SIZE, "%d",
			ois_sr_rear_result);
	}

	if (rc)
		return rc;
	return 0;
}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
extern uint8_t ois_tele_xysr[OIS_XYSR_SIZE];
int ois_sr_rear3_result = 2; //0:normal, 1: No cal, 2: rear cal fail
static ssize_t ois_supperssion_ratio_rear3_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	uint32_t xsr = 0, ysr = 0;

	pr_info("[FW_DBG] ois_sr_rear3_result : %d\n",
		ois_sr_rear3_result);
	if (ois_sr_rear3_result == 0) {
		memcpy(&xsr, &ois_tele_xysr[0], 2);
		memcpy(&ysr, &ois_tele_xysr[2], 2);
		rc = scnprintf(buf, PAGE_SIZE, "%d,%u.%02u,%u.%02u",
			ois_sr_rear3_result, (xsr / 100), (xsr % 100), (ysr / 100), (ysr % 100));
	} else {
		rc = scnprintf(buf, PAGE_SIZE, "%d",
			ois_sr_rear3_result);
	}
	if (rc)
		return rc;
	return 0;
}

extern uint8_t ois_tele_cross_talk[OIS_CROSSTALK_SIZE];
int ois_tele_cross_talk_result = 2; //0:normal, 1: No cal, 2: rear cal fail
static ssize_t ois_rear3_read_cross_talk_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	uint32_t xcrosstalk = 0, ycrosstalk = 0;

	pr_info("[FW_DBG] ois_tele_crosstalk_result : %d\n",
		ois_tele_cross_talk_result);
	memcpy(&xcrosstalk, &ois_tele_cross_talk[0], 2);
	memcpy(&ycrosstalk, &ois_tele_cross_talk[2], 2);
	if (ois_tele_cross_talk_result == 0) { // normal
		rc = scnprintf(buf, PAGE_SIZE, "%u.%02u,%u.%02u",
			(xcrosstalk/ 100), (xcrosstalk % 100),
			(ycrosstalk / 100), (ycrosstalk % 100));
	} else if (ois_tele_cross_talk_result == 1) { // No cal
		rc = scnprintf(buf, PAGE_SIZE, "NONE");
	} else { // read cal fail
		rc = scnprintf(buf, PAGE_SIZE, "NG");
	}

	if (rc)
		return rc;
	return 0;
}
#endif

static ssize_t ois_check_cross_talk_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	uint16_t result[STEP_COUNT] = { 0, };

	rc = cam_ois_check_tele_cross_talk(g_o_ctrl, result);
	if (rc < 0)
		pr_err("ois check tele cross talk fail\n");

	rc = scnprintf(buf, PAGE_SIZE, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
		(rc < 0 ? 0 : 1), result[0], result[1], result[2], result[3], result[4],
		result[5], result[6], result[7], result[8], result[9]);

	if (rc)
		return rc;
	return 0;
}

static ssize_t ois_ext_clk_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	uint32_t clk = 0;

	clk = cam_ois_check_ext_clk(g_o_ctrl);
	if (clk == 0)
		pr_err("ois check ext clk fail\n");

	rc = scnprintf(buf, PAGE_SIZE, "%u", clk);

	if (rc)
		return rc;
	return 0;
}

static ssize_t ois_ext_clk_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0;
	uint32_t clk = 0;

	if (buf == NULL || kstrtouint(buf, 10, &clk))
		return -1;
	pr_info("new ois ext clk %a\n", clk);

	rc = cam_ois_set_ext_clk(g_o_ctrl, clk);
	if (rc < 0) {
		pr_err("ois check ext clk fail\n");
		return -1;
	}

	return size;
}
#endif

#if defined(CONFIG_SAMSUNG_ACTUATOR_PREVENT_SHAKING)
extern struct cam_actuator_ctrl_t *g_a_ctrls[2];
#endif

static ssize_t rear_actuator_power_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
#if defined(CONFIG_SAMSUNG_ACTUATOR_PREVENT_SHAKING)
	int i = 0, cnt = 0, rc = 0;
	cnt = (int)(sizeof(g_a_ctrls) / sizeof(g_a_ctrls[0]));
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
	if (g_a_ctrls[0] != NULL) {
		mutex_lock(&(g_a_ctrls[0]->actuator_mutex));
	}
#endif
	switch (buf[0]) {
	case '0':
		if (actuator_power == 0) {
			pr_info("%s: [WARNING] actuator is off, skip power down", __func__);
			goto error;
		}
		for (i = 0; i < cnt; i++) {
			if (g_a_ctrls[i] != NULL) {
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
				if (g_a_ctrls[i]->soc_info.index == 2)
				{
					mutex_lock(&(g_o_ctrl->ois_mutex));
					rc = cam_ois_power_down(g_o_ctrl);
					mutex_unlock(&(g_o_ctrl->ois_mutex));
				}
				else
#endif
					rc |= cam_actuator_force_power_down(g_a_ctrls[i]);
				pr_info("%s: actuator %d power down", __func__, i);
			}
		}
		if (rc < 0) {
			pr_info("%s: actuator power down fail", __func__);
			actuator_power = 0;
			return 0;
		}
		actuator_power = 0;
		break;
	case '1':
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		if (ois_power > 0) {
			pr_info("%s: [WARNING] ois is used", __func__);
			goto error;
		}
#endif
		actuator_power = 1;

		for (i = 0; i < cnt; i++) {
			if (g_a_ctrls[i] != NULL) {
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
				if (g_a_ctrls[i]->soc_info.index == 2)
				{
					mutex_lock(&(g_o_ctrl->ois_mutex));
					cam_ois_power_up(g_o_ctrl);
					msleep(20);
#if defined(CONFIG_SEC_C2Q_PROJECT)
					g_o_ctrl->ois_mode = 0;
					if (cam_ois_wait_idle(g_o_ctrl, 20) < 0)
						CAM_ERR(CAM_OIS, "wait ois idle status failed");
					cam_ois_i2c_write(g_o_ctrl, 0x00BE, 0x02,
						CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); /* select module */
					cam_ois_set_ois_mode(g_o_ctrl, 0x05);
#else
					cam_ois_mcu_init(g_o_ctrl);
#endif
					mutex_unlock(&(g_o_ctrl->ois_mutex));
				}
				else
#endif
				cam_actuator_power_up(g_a_ctrls[i]);
				cam_actuator_default_init_setting(g_a_ctrls[i]);
				pr_info("%s: actuator %d power up", __func__, i);
			}
		}
		break;

	default:
		break;
	}

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	error:
#endif
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
	if (g_a_ctrls[0] != NULL) {
		 mutex_unlock(&(g_a_ctrls[0]->actuator_mutex));
	}
#endif
#endif
	return size;
}

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
char mipi_string[20] = {0, };
static ssize_t front_camera_mipi_clock_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_info("front_camera_mipi_clock_show : %s\n", mipi_string);
	rc = scnprintf(buf, PAGE_SIZE, "%s\n", mipi_string);
	if (rc)
		return rc;
	return 0;
}
#endif

#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
extern long rear_frs_test_mode;

static ssize_t rear_camera_frs_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_info("[FRS_DBG] rear_frs_test_mode : %ld\n", rear_frs_test_mode);
	rc = scnprintf(buf, PAGE_SIZE, "%ld\n", rear_frs_test_mode);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_camera_frs_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	long data = simple_strtol(buf, NULL, 10);

	pr_info("[FRS_DBG] rear_frs_test_mode : %c(%ld)\n", buf[0], data);
	rear_frs_test_mode = data;

	return size;
}
#endif

#if defined(CONFIG_CAMERA_FAC_LN_TEST) // Factory Low Noise Test
extern uint8_t factory_ln_test;
static ssize_t cam_factory_ln_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_info("[LN_TEST] factory_ln_test : %d\n", factory_ln_test);
	rc = scnprintf(buf, PAGE_SIZE, "%d\n", factory_ln_test);
	if (rc)
		return rc;
	return 0;
}
static ssize_t cam_factory_ln_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("[LN_TEST] factory_ln_test : %c\n", buf[0]);
	if (buf[0] == '1')
		factory_ln_test = 1;
	else
		factory_ln_test = 0;

	return size;
}
#endif

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
static int16_t is_hw_param_valid_module_id(char *moduleid)
{
	int i = 0;
	int32_t moduleid_cnt = 0;
	int16_t rc = HW_PARAMS_MI_VALID;

	if (moduleid == NULL) {
		CAM_ERR(CAM_HWB, "MI_INVALID\n");
		return HW_PARAMS_MI_INVALID;
	}

	for (i = 0; i < FROM_MODULE_ID_SIZE; i++) {
		if (moduleid[i] == '\0') {
			moduleid_cnt = moduleid_cnt + 1;
		} else if ((i < 5)
				&& (!((moduleid[i] > 47 && moduleid[i] < 58)	// 0 to 9
						|| (moduleid[i] > 64 && moduleid[i] < 91)))) { // A to Z
			CAM_ERR(CAM_HWB, "MIR_ERR_1\n");
			rc = HW_PARAMS_MIR_ERR_1;
			break;
		}
	}

	if (moduleid_cnt == FROM_MODULE_ID_SIZE) {
		CAM_ERR(CAM_HWB, "MIR_ERR_0\n");
		rc = HW_PARAMS_MIR_ERR_0;
	}

	return rc;
}

static ssize_t rear_camera_hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	int16_t moduelid_chk = 0;
	struct cam_hw_param *ec_param = NULL;

	msm_is_sec_get_rear_hw_param(&ec_param);

	if (ec_param != NULL) {
		moduelid_chk = is_hw_param_valid_module_id(rear_module_id);
		switch (moduelid_chk) {
		case HW_PARAMS_MI_VALID:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIR_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CR_AF\":\"%d\",\"I2CR_COM\":\"%d\",\"I2CR_OIS\":\"%d\","
					"\"I2CR_SEN\":\"%d\",\"MIPIR_COM\":\"%d\",\"MIPIR_SEN\":\"%d\"\n",
					rear_module_id[0], rear_module_id[1], rear_module_id[2], rear_module_id[3],
					rear_module_id[4], rear_module_id[7], rear_module_id[8], rear_module_id[9],
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		case HW_PARAMS_MIR_ERR_1:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIR_ID\":\"MIR_ERR\",\"I2CR_AF\":\"%d\",\"I2CR_COM\":\"%d\",\"I2CR_OIS\":\"%d\","
					"\"I2CR_SEN\":\"%d\",\"MIPIR_COM\":\"%d\",\"MIPIR_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		default:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIR_ID\":\"MI_NO\",\"I2CR_AF\":\"%d\",\"I2CR_COM\":\"%d\",\"I2CR_OIS\":\"%d\","
					"\"I2CR_SEN\":\"%d\",\"MIPIR_COM\":\"%d\",\"MIPIR_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;
		}
	}

	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_camera_hw_param_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_hw_param *ec_param = NULL;

	CAM_DBG(CAM_HWB, "[R] buf : %s\n", buf);

	if (!strncmp(buf, "c", 1)) {
		msm_is_sec_get_rear_hw_param(&ec_param);
		if (ec_param != NULL) {
			msm_is_sec_init_err_cnt_file(ec_param);
		}
	}

	return size;
}
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)

//#define FROM_FRONT2_DUAL_CAL_SIZE 1024

uint8_t front2_dual_cal[FROM_FRONT_DUAL_CAL_SIZE + 1] = "\0";
static ssize_t front2_dual_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *ret = NULL;
	int copy_size = 0;

	pr_debug("[FW_DBG] front2_dual_cal : %s\n", front2_dual_cal);

	if (FROM_FRONT_DUAL_CAL_SIZE > SYSFS_MAX_READ_SIZE)
		copy_size = SYSFS_MAX_READ_SIZE;
	else
		copy_size = FROM_FRONT_DUAL_CAL_SIZE;

	ret = memcpy(buf, front2_dual_cal, copy_size);

	if (ret)
		return copy_size;

	return 0;

}


uint32_t front2_dual_cal_size = FROM_FRONT_DUAL_CAL_SIZE;
static ssize_t front2_dual_cal_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front2_dual_cal_size : %d\n", front2_dual_cal_size);
	rc = scnprintf(buf, PAGE_SIZE, "%d", front2_dual_cal_size);
	if (rc)
		return rc;
	return 0;
}

#if 1
DualTilt_t front2_dual;
static ssize_t front2_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front2 dual tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		front2_dual.x, front2_dual.y, front2_dual.z, front2_dual.sx, front2_dual.sy,
		front2_dual.range, front2_dual.max_err, front2_dual.avg_err, front2_dual.dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", front2_dual.x, front2_dual.y,
			front2_dual.z, front2_dual.sx, front2_dual.sy, front2_dual.range,
			front2_dual.max_err, front2_dual.avg_err, front2_dual.dll_ver);
	if (rc)
		return rc;
	return 0;
}
#else
int front2_dual_tilt_x;
int front2_dual_tilt_y;
int front2_dual_tilt_z;
int front2_dual_tilt_sx;
int front2_dual_tilt_sy;
int front2_dual_tilt_range;
int front2_dual_tilt_max_err;
int front2_dual_tilt_avg_err;
int front2_dual_tilt_dll_ver;
static ssize_t front2_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front dual tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		front2_dual_tilt_x, front2_dual_tilt_y, front2_dual_tilt_z, front2_dual_tilt_sx, front2_dual_tilt_sy,
		front2_dual_tilt_range, front2_dual_tilt_max_err, front2_dual_tilt_avg_err, front2_dual_tilt_dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", front2_dual_tilt_x, front2_dual_tilt_y,
			front2_dual_tilt_z, front2_dual_tilt_sx, front2_dual_tilt_sy, front2_dual_tilt_range,
			front2_dual_tilt_max_err, front2_dual_tilt_avg_err, front2_dual_tilt_dll_ver);
	if (rc)
		return rc;
	return 0;
}
#endif

#endif
static ssize_t front_camera_hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t rc = 0;
	int16_t moduelid_chk = 0;
	struct cam_hw_param *ec_param = NULL;

	msm_is_sec_get_front_hw_param(&ec_param);

	if (ec_param != NULL) {
		moduelid_chk = is_hw_param_valid_module_id(front_module_id);

		switch (moduelid_chk) {
		case HW_PARAMS_MI_VALID:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CF_AF\":\"%d\",\"I2CF_COM\":\"%d\",\"I2CF_OIS\":\"%d\","
					"\"I2CF_SEN\":\"%d\",\"MIPIF_COM\":\"%d\",\"MIPIF_SEN\":\"%d\"\n",
					front_module_id[0], front_module_id[1], front_module_id[2], front_module_id[3],
					front_module_id[4], front_module_id[7], front_module_id[8], front_module_id[9],
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		case HW_PARAMS_MIR_ERR_1:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF_ID\":\"MIR_ERR\",\"I2CF_AF\":\"%d\",\"I2CF_COM\":\"%d\",\"I2CF_OIS\":\"%d\","
					"\"I2CF_SEN\":\"%d\",\"MIPIF_COM\":\"%d\",\"MIPIF_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		default:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF_ID\":\"MI_NO\",\"I2CF_AF\":\"%d\",\"I2CF_COM\":\"%d\",\"I2CF_OIS\":\"%d\","
					"\"I2CF_SEN\":\"%d\",\"MIPIF_COM\":\"%d\",\"MIPIF_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;
		}
	}

	if (rc)
		return rc;
	return 0;
}

static ssize_t front_camera_hw_param_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_hw_param *ec_param = NULL;

	CAM_DBG(CAM_HWB, "[F] buf : %s\n", buf);

	if (!strncmp(buf, "c", 1)) {
		msm_is_sec_get_front_hw_param(&ec_param);
		if (ec_param != NULL) {
			msm_is_sec_init_err_cnt_file(ec_param);
		}
	}

	return size;
}

#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
uint8_t iris_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
static ssize_t iris_camera_hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t rc = 0;
	int16_t moduelid_chk = 0;
	struct cam_hw_param *ec_param = NULL;

	msm_is_sec_get_iris_hw_param(&ec_param);

	if (ec_param != NULL) {
		moduelid_chk = is_hw_param_valid_module_id(iris_module_id);

		switch (moduelid_chk) {
		case HW_PARAMS_MI_VALID:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMII_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CI_AF\":\"%d\",\"I2CI_COM\":\"%d\",\"I2CI_OIS\":\"%d\","
					"\"I2CI_SEN\":\"%d\",\"MIPII_COM\":\"%d\",\"MIPII_SEN\":\"%d\"\n",
					iris_module_id[0], iris_module_id[1], iris_module_id[2], iris_module_id[3],
					iris_module_id[4], iris_module_id[7], iris_module_id[8], iris_module_id[9],
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		case HW_PARAMS_MIR_ERR_1:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMII_ID\":\"MIR_ERR\",\"I2CI_AF\":\"%d\",\"I2CI_COM\":\"%d\",\"I2CI_OIS\":\"%d\","
					"\"I2CI_SEN\":\"%d\",\"MIPII_COM\":\"%d\",\"MIPII_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		default:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMII_ID\":\"MI_NO\",\"I2CI_AF\":\"%d\",\"I2CI_COM\":\"%d\",\"I2CI_OIS\":\"%d\","
					"\"I2CI_SEN\":\"%d\",\"MIPII_COM\":\"%d\",\"MIPII_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;
		}
	}

	if (rc)
		return rc;
	return 0;
}

static ssize_t iris_camera_hw_param_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_hw_param *ec_param = NULL;

	CAM_DBG(CAM_HWB, "[I] buf : %s\n", buf);

	if (!strncmp(buf, "c", 1)) {
		msm_is_sec_get_iris_hw_param(&ec_param);
		if (ec_param != NULL) {
			msm_is_sec_init_err_cnt_file(ec_param);
		}
	}

	return size;
}
#endif

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static ssize_t rear2_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t rc = 0;
	int16_t moduelid_chk = 0;
	struct cam_hw_param *ec_param = NULL;

	msm_is_sec_get_rear2_hw_param(&ec_param);

	if (ec_param != NULL) {
		moduelid_chk = is_hw_param_valid_module_id(rear2_module_id);

		switch (moduelid_chk) {
		case HW_PARAMS_MI_VALID:
			rc = sprintf(buf, "\"CAMIR2_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CR2_AF\":\"%d\",\"I2CR2_COM\":\"%d\",\"I2CR2_OIS\":\"%d\","
					"\"I2CR2_SEN\":\"%d\",\"MIPIR2_COM\":\"%d\",\"MIPIR2_SEN\":\"%d\"\n",
					rear2_module_id[0], rear2_module_id[1], rear2_module_id[2], rear2_module_id[3],
					rear2_module_id[4], rear2_module_id[7], rear2_module_id[8], rear2_module_id[9],
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		case HW_PARAMS_MIR_ERR_1:
			rc = sprintf(buf, "\"CAMIR2_ID\":\"MIR_ERR\",\"I2CR2_AF\":\"%d\",\"I2CR2_COM\":\"%d\",\"I2CR2_OIS\":\"%d\","
					"\"I2CR2_SEN\":\"%d\",\"MIPIR2_COM\":\"%d\",\"MIPIR2_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		default:
			rc = sprintf(buf, "\"CAMIR2_ID\":\"MI_NO\",\"I2CR2_AF\":\"%d\",\"I2CR2_COM\":\"%d\",\"I2CR2_OIS\":\"%d\","
					"\"I2CR2_SEN\":\"%d\",\"MIPIR2_COM\":\"%d\",\"MIPIR2_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;
		}
	}

	return rc;
}

static ssize_t rear2_camera_hw_param_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_hw_param *ec_param = NULL;

	CAM_DBG(CAM_HWB, "[R2] buf : %s\n", buf);

	if (!strncmp(buf, "c", 1)) {
		msm_is_sec_get_rear2_hw_param(&ec_param);
		if (ec_param != NULL) {
			msm_is_sec_init_err_cnt_file(ec_param);
		}
	}

	return size;
}
#endif

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static ssize_t rear3_camera_hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t rc = 0;
	int16_t moduelid_chk = 0;
	struct cam_hw_param *ec_param = NULL;

	msm_is_sec_get_rear3_hw_param(&ec_param);

	if (ec_param != NULL) {
		moduelid_chk = is_hw_param_valid_module_id(rear3_module_id);

		switch (moduelid_chk) {
		case HW_PARAMS_MI_VALID:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIR3_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CR3_AF\":\"%d\",\"I2CR3_COM\":\"%d\",\"I2CR3_OIS\":\"%d\","
					"\"I2CR3_SEN\":\"%d\",\"MIPIR3_COM\":\"%d\",\"MIPIR3_SEN\":\"%d\"\n",
					rear3_module_id[0], rear3_module_id[1], rear3_module_id[2], rear3_module_id[3],
					rear3_module_id[4], rear3_module_id[7], rear3_module_id[8], rear3_module_id[9],
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		case HW_PARAMS_MIR_ERR_1:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIR3_ID\":\"MIR_ERR\",\"I2CR3_AF\":\"%d\",\"I2CR3_COM\":\"%d\",\"I2CR3_OIS\":\"%d\","
					"\"I2CR3_SEN\":\"%d\",\"MIPIR3_COM\":\"%d\",\"MIPIR3_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		default:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIR3_ID\":\"MI_NO\",\"I2CR3_AF\":\"%d\",\"I2CR3_COM\":\"%d\",\"I2CR3_OIS\":\"%d\","
					"\"I2CR3_SEN\":\"%d\",\"MIPIR3_COM\":\"%d\",\"MIPIR3_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;
		}
	}

	if (rc)
		return rc;
	return 0;
}

static ssize_t rear3_camera_hw_param_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_hw_param *ec_param = NULL;

	CAM_DBG(CAM_HWB, "[R3] buf : %s\n", buf);

	if (!strncmp(buf, "c", 1)) {
		msm_is_sec_get_rear3_hw_param(&ec_param);
		if (ec_param != NULL) {
			msm_is_sec_init_err_cnt_file(ec_param);
		}
	}

	return size;
}
#endif

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
static ssize_t front2_camera_hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t rc = 0;
	int16_t moduelid_chk = 0;
	struct cam_hw_param *ec_param = NULL;

	msm_is_sec_get_front2_hw_param(&ec_param);

	if (ec_param != NULL) {
		moduelid_chk = is_hw_param_valid_module_id(front2_module_id);

		switch (moduelid_chk) {
		case HW_PARAMS_MI_VALID:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF2_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CF2_AF\":\"%d\",\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\","
					"\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
					front2_module_id[0], front2_module_id[1], front2_module_id[2], front2_module_id[3],
					front2_module_id[4], front2_module_id[7], front2_module_id[8], front2_module_id[9],
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		case HW_PARAMS_MIR_ERR_1:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF2_ID\":\"MIR_ERR\",\"I2CF2_AF\":\"%d\",\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\","
					"\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		default:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF2_ID\":\"MI_NO\",\"I2CF2_AF\":\"%d\",\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\","
					"\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;
		}
	}

	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_hw_param_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_hw_param *ec_param = NULL;

	CAM_DBG(CAM_HWB, "[F2] buf : %s\n", buf);

	if (!strncmp(buf, "c", 1)) {
		msm_is_sec_get_front2_hw_param(&ec_param);
		if (ec_param != NULL) {
			msm_is_sec_init_err_cnt_file(ec_param);
		}
	}

	return size;
}
#endif

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
static ssize_t front3_camera_hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t rc = 0;
	int16_t moduelid_chk = 0;
	struct cam_hw_param *ec_param = NULL;

	msm_is_sec_get_front3_hw_param(&ec_param);

	if (ec_param != NULL) {
		moduelid_chk = is_hw_param_valid_module_id(front3_module_id);

		switch (moduelid_chk) {
		case HW_PARAMS_MI_VALID:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF2_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CF2_AF\":\"%d\",\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\","
					"\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
					front3_module_id[0], front3_module_id[1], front3_module_id[2], front3_module_id[3],
					front3_module_id[4], front3_module_id[7], front3_module_id[8], front3_module_id[9],
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		case HW_PARAMS_MIR_ERR_1:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF2_ID\":\"MIR_ERR\",\"I2CF2_AF\":\"%d\",\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\","
					"\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		default:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF2_ID\":\"MI_NO\",\"I2CF2_AF\":\"%d\",\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\","
					"\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;
		}
	}

	if (rc)
		return rc;
	return 0;
}

static ssize_t front3_camera_hw_param_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_hw_param *ec_param = NULL;

	CAM_DBG(CAM_HWB, "[F3] buf : %s\n", buf);

	if (!strncmp(buf, "c", 1)) {
		msm_is_sec_get_front3_hw_param(&ec_param);
		if (ec_param != NULL) {
			msm_is_sec_init_err_cnt_file(ec_param);
		}
	}

	return size;
}
#else
static ssize_t front2_camera_hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t rc = 0;
	int16_t moduelid_chk = 0;
	struct cam_hw_param *ec_param = NULL;

	msm_is_sec_get_front2_hw_param(&ec_param);

	if (ec_param != NULL) {
		moduelid_chk = is_hw_param_valid_module_id(front2_module_id);

		switch (moduelid_chk) {
		case HW_PARAMS_MI_VALID:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF2_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CF2_AF\":\"%d\",\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\","
					"\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
					front2_module_id[0], front2_module_id[1], front2_module_id[2], front2_module_id[3],
					front2_module_id[4], front2_module_id[7], front2_module_id[8], front2_module_id[9],
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		case HW_PARAMS_MIR_ERR_1:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF2_ID\":\"MIR_ERR\",\"I2CF2_AF\":\"%d\",\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\","
					"\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;

		default:
			rc = scnprintf(buf, PAGE_SIZE, "\"CAMIF2_ID\":\"MI_NO\",\"I2CF2_AF\":\"%d\",\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\","
					"\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
					ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
					ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
					ec_param->mipi_sensor_err_cnt);
			break;
		}
	}

	if (rc)
		return rc;
	return 0;
}

static ssize_t front2_camera_hw_param_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_hw_param *ec_param = NULL;

	CAM_DBG(CAM_HWB, "[F2] buf : %s\n", buf);

	if (!strncmp(buf, "c", 1)) {
		msm_is_sec_get_front2_hw_param(&ec_param);
		if (ec_param != NULL) {
			msm_is_sec_init_err_cnt_file(ec_param);
		}
	}

	return size;
}
#endif
#endif
#endif

#if defined(CONFIG_SAMSUNG_APERTURE)
#if defined(CONFIG_SAMSUNG_APERTURE_HALLTEST)
extern struct cam_aperture_ctrl_t *g_cam_aperture_t;

int32_t factory_halltest_test;

static ssize_t rear_aperture_halltest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int result = 0;
	int rc     = 0;

	if ((g_cam_aperture_t == NULL) || (g_cam_aperture_t->io_master_info.client == NULL)) {
		CAM_ERR(CAM_HWB, "[HALLTC][R0][ERROR] NULL\n");
		return rc;
	}

	factory_halltest_test = -1;

	CAM_DBG(CAM_HWB, "[HALLTC][R0] %d E\n", factory_halltest_test);
	result = cam_aperture_get_halltest_mode(&(g_cam_aperture_t->io_master_info), &factory_halltest_test);
	if (result == 0) {
		CAM_ERR(CAM_HWB, "[HALLTC][R0][ERROR] %d\n", result);
		rc = scnprintf(buf, PAGE_SIZE, "%d,0x%04x\n", ((result == 0)? 0 : 1), factory_halltest_test);
		if (rc)
			return rc;
	}
	else {
		CAM_DBG(CAM_HWB, "[HALLTC][R0] %d X\n", factory_halltest_test);
		rc = scnprintf(buf, PAGE_SIZE, "%d,0x%04x\n", ((result == 1)? 1 : 0), factory_halltest_test);
		if (rc)
			return rc;
	}

	return 0;
}
static ssize_t rear_aperture_halltest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	CAM_DBG(CAM_HWB, "[HALLTC][R0] buf : %s\n", buf);

	return size;
}
#endif	// CONFIG_SAMSUNG_APERTURE_HALLTEST
#endif	// CONFIG_SAMSUNG_APERTURE

#if defined(CONFIG_SAMSUNG_REAR_TOF)
char rear_tof_cam_info[150] = "NULL\n";	//camera_info
static ssize_t rear_tof_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear_tof_cam_info : %s\n", rear_tof_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", rear_tof_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_tof_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear_tof_cam_info, sizeof(rear_tof_cam_info), "%s", buf);

	return size;
}

char rear_tof_module_info[SYSFS_MODULE_INFO_SIZE] = "NULL\n";
static ssize_t rear_tof_module_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear_tof_module_info : %s\n", rear_tof_module_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", rear_tof_module_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_tof_module_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(rear_tof_module_info, sizeof(rear_tof_module_info), "%s", buf);

	return size;
}

char cam_tof_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";//multi module
static ssize_t rear_tof_camera_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	pr_debug("[FW_DBG] cam_fw_ver : %s\n", cam_tof_fw_ver);

	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_tof_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_tof_camera_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_tof_fw_ver, sizeof(cam_tof_fw_ver), "%s", buf);

	return size;
}

char cam_tof_fw_user_ver[SYSFS_FW_VER_SIZE] = "NULL\n";//multi module
static ssize_t rear_tof_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_tof_fw_user_ver : %s\n", cam_tof_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_tof_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_tof_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{

	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_tof_fw_user_ver, sizeof(cam_tof_fw_user_ver), "%s", buf);

	return size;
}

char cam_tof_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NULL\n";//multi module
static ssize_t rear_tof_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear_tof_fw_factory_ver : %s\n", cam_tof_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_tof_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_tof_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_tof_fw_factory_ver, sizeof(cam_tof_fw_factory_ver), "%s", buf);

	return size;
}

char cam_tof_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";//multi module
static ssize_t rear_tof_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear_tof_fw_full_ver : %s\n", cam_tof_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_tof_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_tof_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_tof_fw_full_ver, sizeof(cam_tof_fw_full_ver), "%s", buf);

	return size;
}

char cam_tof_load_fw[SYSFS_FW_VER_SIZE] = "NULL\n";
static ssize_t rear_tof_firmware_load_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear_tof_load_fw : %s\n", cam_tof_load_fw);
	rc = scnprintf(buf, PAGE_SIZE, "%s", cam_tof_load_fw);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_tof_firmware_load_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(cam_tof_load_fw, sizeof(cam_tof_load_fw), "%s\n", buf);
	return size;
}

uint8_t rear_tof_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
static ssize_t rear_tof_moduleid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear_tof_module_id : %c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  rear_tof_module_id[0], rear_tof_module_id[1], rear_tof_module_id[2], rear_tof_module_id[3], rear_tof_module_id[4],
	  rear_tof_module_id[5], rear_tof_module_id[6], rear_tof_module_id[7], rear_tof_module_id[8], rear_tof_module_id[9]);
	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
	  rear_tof_module_id[0], rear_tof_module_id[1], rear_tof_module_id[2], rear_tof_module_id[3], rear_tof_module_id[4],
	 rear_tof_module_id[5], rear_tof_module_id[6], rear_tof_module_id[7], rear_tof_module_id[8], rear_tof_module_id[9]);
}

char rear_tof_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
static ssize_t rear_tof_sensorid_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] rear_tof_sensor_id : %s\n", rear_tof_sensor_id);
	memcpy(buf, rear_tof_sensor_id, FROM_SENSOR_ID_SIZE);
	return FROM_SENSOR_ID_SIZE;
}

static ssize_t rear_tof_sensorid_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(rear_tof_sensor_id, sizeof(rear_sensor_id), "%s", buf);

	return size;
}

int rear_tof_uid;
static ssize_t rear_tofcal_uid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
    int rc= 0;

	pr_debug("[FW_DBG] rear tof uid = %d\n", rear_tof_uid);

	rc = scnprintf(buf, PAGE_SIZE, "%d\n", rear_tof_uid);
	if (rc)
		return rc;
	return 0;
}

uint8_t rear_tof_cal[TOFCAL_SIZE + 1] = "\0";
static ssize_t rear_tofcal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *ret = NULL;
	int copy_size = 0;

	pr_debug("[FW_DBG] rear_tof_cal : %s\n", rear_tof_cal);

	if (TOFCAL_SIZE > SYSFS_MAX_READ_SIZE)
		copy_size = SYSFS_MAX_READ_SIZE;
	else
		copy_size = TOFCAL_SIZE;

	ret = memcpy(buf, rear_tof_cal, copy_size);

	if (ret)
		return copy_size;

	return 0;
}

uint8_t rear_tof_cal_extra[TOFCAL_EXTRA_SIZE + 1] = "\0";
static ssize_t rear_tofcal_extra_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *ret = NULL;
	int copy_size = 0;

	pr_debug("[FW_DBG] rear_tof_cal_extra : %s\n", rear_tof_cal_extra);

	if (TOFCAL_EXTRA_SIZE > SYSFS_MAX_READ_SIZE)
		copy_size = SYSFS_MAX_READ_SIZE;
	else
		copy_size = TOFCAL_EXTRA_SIZE;

	ret = memcpy(buf, rear_tof_cal_extra, copy_size);

	if (ret)
		return copy_size;

	return 0;
}


uint32_t rear_tof_cal_size = TOFCAL_TOTAL_SIZE;
static ssize_t rear_tofcal_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_info("[FW_DBG] rear_tof_cal_size : %d\n", rear_tof_cal_size);
	rc = scnprintf(buf, PAGE_SIZE, "%d", rear_tof_cal_size);
	if (rc)
		return rc;
	return 0;
}

uint8_t rear_tof_cal_result = 0;
static ssize_t rear_tofcal_result_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	if (rear_tof_cal_result == 1)
		rc = scnprintf(buf, PAGE_SIZE, "%s", "OK\n");
	else
		rc = scnprintf(buf, PAGE_SIZE, "%s", "NG\n");
	if (rc)
		return rc;
	return 0;
}

uint8_t rear_tof_dual_cal[REAR_TOF_DUAL_CAL_SIZE + 1] = "\0";
static ssize_t rear_tof_dual_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *ret = NULL;
	int copy_size = 0;

	pr_debug("[FW_DBG] rear_tof_dual_cal : %s\n", rear_tof_dual_cal);

	if (REAR_TOF_DUAL_CAL_SIZE > SYSFS_MAX_READ_SIZE)
		copy_size = SYSFS_MAX_READ_SIZE;
	else
		copy_size = REAR_TOF_DUAL_CAL_SIZE;

	ret = memcpy(buf, rear_tof_dual_cal, copy_size);

	if (ret)
		return copy_size;

	return 0;
}

#if 1
DualTilt_t rear_tof_dual;
static ssize_t rear_tof_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear tof tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		rear_tof_dual.x, rear_tof_dual.y, rear_tof_dual.z, rear_tof_dual.sx, rear_tof_dual.sy,
		rear_tof_dual.range, rear_tof_dual.max_err, rear_tof_dual.avg_err, rear_tof_dual.dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", rear_tof_dual.x, rear_tof_dual.y,
			rear_tof_dual.z, rear_tof_dual.sx, rear_tof_dual.sy, rear_tof_dual.range,
			rear_tof_dual.max_err, rear_tof_dual.avg_err, rear_tof_dual.dll_ver);
	if (rc)
		return rc;
	return 0;
}
#else
int rear_tof_dual_tilt_x;
int rear_tof_dual_tilt_y;
int rear_tof_dual_tilt_z;
int rear_tof_dual_tilt_sx;
int rear_tof_dual_tilt_sy;
int rear_tof_dual_tilt_range;
int rear_tof_dual_tilt_max_err;
int rear_tof_dual_tilt_avg_err;
int rear_tof_dual_tilt_dll_ver;
static ssize_t rear_tof_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear tof tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		rear_tof_dual_tilt_x, rear_tof_dual_tilt_y, rear_tof_dual_tilt_z, rear_tof_dual_tilt_sx, rear_tof_dual_tilt_sy,
		rear_tof_dual_tilt_range, rear_tof_dual_tilt_max_err, rear_tof_dual_tilt_avg_err, rear_tof_dual_tilt_dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", rear_tof_dual_tilt_x, rear_tof_dual_tilt_y,
			rear_tof_dual_tilt_z, rear_tof_dual_tilt_sx, rear_tof_dual_tilt_sy, rear_tof_dual_tilt_range,
			rear_tof_dual_tilt_max_err, rear_tof_dual_tilt_avg_err, rear_tof_dual_tilt_dll_ver);
	if (rc)
		return rc;
	return 0;
}
#endif

#if 1
DualTilt_t rear2_tof_dual;
static ssize_t rear2_tof_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear tof tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		rear2_tof_dual.x, rear2_tof_dual.y, rear2_tof_dual.z, rear2_tof_dual.sx, rear2_tof_dual.sy,
		rear2_tof_dual.range, rear2_tof_dual.max_err, rear2_tof_dual.avg_err, rear2_tof_dual.dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", rear2_tof_dual.x, rear2_tof_dual.y,
			rear2_tof_dual.z, rear2_tof_dual.sx, rear2_tof_dual.sy, rear2_tof_dual.range,
			rear2_tof_dual.max_err, rear2_tof_dual.avg_err, rear2_tof_dual.dll_ver);
	if (rc)
		return rc;
	return 0;
}
#else
int rear2_tof_dual_tilt_x;
int rear2_tof_dual_tilt_y;
int rear2_tof_dual_tilt_z;
int rear2_tof_dual_tilt_sx;
int rear2_tof_dual_tilt_sy;
int rear2_tof_dual_tilt_range;
int rear2_tof_dual_tilt_max_err;
int rear2_tof_dual_tilt_avg_err;
int rear2_tof_dual_tilt_dll_ver;
static ssize_t rear2_tof_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear tof tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		rear2_tof_dual_tilt_x, rear2_tof_dual_tilt_y, rear2_tof_dual_tilt_z, rear2_tof_dual_tilt_sx, rear2_tof_dual_tilt_sy,
		rear2_tof_dual_tilt_range, rear2_tof_dual_tilt_max_err, rear2_tof_dual_tilt_avg_err, rear2_tof_dual_tilt_dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", rear2_tof_dual_tilt_x, rear2_tof_dual_tilt_y,
			rear2_tof_dual_tilt_z, rear2_tof_dual_tilt_sx, rear2_tof_dual_tilt_sy, rear2_tof_dual_tilt_range,
			rear2_tof_dual_tilt_max_err, rear2_tof_dual_tilt_avg_err, rear2_tof_dual_tilt_dll_ver);
	if (rc)
		return rc;
	return 0;
}
#endif

static ssize_t rear_tof_ld_onoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cam_camera_slave_info *slave_info;
	uint32_t read_data = -1;
	uint32_t read_data0 = -1;
	uint32_t read_data1 = -1;
	int rc = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;

	slave_info = &(g_s_ctrl_tof->sensordata->slave_info);

	if (slave_info->sensor_id == TOF_SENSOR_ID_IMX516 ||
			slave_info->sensor_id == TOF_SENSOR_ID_IMX518) {
		cam_sensor_tof_i2c_read(0x2144, &read_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_read(0x2145, &read_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	} else {
		pr_info("TOF ld ofoff not support\n");
	}


	if(read_data0 && read_data1)
		read_data = 0;
	else read_data = 1;

	rc = scnprintf(buf, PAGE_SIZE, "%d\n", read_data);
	if (rc)
		return rc;

	return 0;
}

static ssize_t rear_tof_ld_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_camera_slave_info *slave_info;
	int value = -1;

	if(g_s_ctrl_tof == NULL)
		return -1;

	slave_info = &(g_s_ctrl_tof->sensordata->slave_info);

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	if (slave_info->sensor_id == TOF_SENSOR_ID_IMX516 ||
			slave_info->sensor_id == TOF_SENSOR_ID_IMX518) {
		if(value == 0) { //ld off
			cam_sensor_tof_i2c_write(0x2144, 0xAA, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2145, 0xAA, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			pr_info("TOF ld off\n");
		} else if(value == 1) { //ld on
			cam_sensor_tof_i2c_write(0x2144, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2145, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			pr_info("TOF ld on\n");
		}
	} else {
		pr_info("TOF ld ofoff not support\n");
	}

	return size;
}

static ssize_t rear_tof_ae_value_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cam_camera_slave_info *slave_info;
	uint32_t ae_value = 0;
	uint32_t reg_data3, reg_data2, reg_data1, reg_data0 = 0;
	int rc = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;

	slave_info = &(g_s_ctrl_tof->sensordata->slave_info);

	if (slave_info->sensor_id == TOF_SENSOR_ID_IMX516 ||
			slave_info->sensor_id == TOF_SENSOR_ID_IMX518) {
		cam_sensor_tof_i2c_read(0x2124, &reg_data3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_read(0x2125, &reg_data2, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_read(0x2126, &reg_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_read(0x2127, &reg_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	} else {
		pr_info("TOF AE support not added\n");
	}

	pr_info("value 0x%x reg_data3: 0x%2x, addr2: 0x%2x, addr1: 0x%2x, addr0: 0x%2x\n", ae_value, reg_data3,reg_data2, reg_data1,reg_data0);

	ae_value = (reg_data3<<24)|(reg_data2<<16)|(reg_data1<<8)|reg_data0;

	pr_info("ae_value: 0x%x\n", ae_value);

	rc = scnprintf(buf, PAGE_SIZE, "%d\n", ae_value/120);

	if (rc)
		return rc;

	return 0;
}

static ssize_t rear_tof_ae_value_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_camera_slave_info *slave_info;
	uint32_t value = 0;
	uint32_t reg_data3, reg_data2, reg_data1, reg_data0 = 0;
	uint32_t hmax = 0x02FE;

	if(g_s_ctrl_tof == NULL)
		return 0;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	pr_info("ae_value: 0x%x\n", value);

	value  = value * 120; // 120Mhz

	if(value >= hmax && value <= 0x1D4C0)
	{
		slave_info = &(g_s_ctrl_tof->sensordata->slave_info);
		reg_data3 = (value & 0xFF000000) >> 24;
		reg_data2 = (value & 0xFF0000) >> 16;
		reg_data1 = (value & 0xFF00) >> 8;
		reg_data0 = value & 0xFF;

		//pr_info("value 0x%x reg_data3: 0x%2x, addr2: 0x%2x, addr1: 0x%2x, addr0: 0x%2x\n", value, reg_data3,reg_data2, reg_data1,reg_data0);
		if(slave_info->sensor_id == TOF_SENSOR_ID_IMX516 ||
				slave_info->sensor_id == TOF_SENSOR_ID_IMX518 ) {
			cam_sensor_tof_i2c_write(0x2124, reg_data3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2125, reg_data2, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2126, reg_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2127, reg_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

			cam_sensor_tof_i2c_write(0x2128, reg_data3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2129, reg_data2, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x212A, reg_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x212B, reg_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		} else {
			pr_info("TOF AE support not added\n");
		}
	}

	return size;
}

static ssize_t rear_tof_check_pd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cam_camera_slave_info *slave_info;
	uint32_t value = 0;
	uint32_t reg_data1, reg_data0 = 0;
	int rc = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;

	if(check_pd_ready == 0) {
		rc = scnprintf(buf, PAGE_SIZE, "NG\n");
	} else {
		slave_info = &(g_s_ctrl_tof->sensordata->slave_info);

		if(slave_info->sensor_id == TOF_SENSOR_ID_IMX516 ||
			slave_info->sensor_id == TOF_SENSOR_ID_IMX518 ) {
			cam_sensor_tof_i2c_read(0x0588, &reg_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_read(0x058F, &reg_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		}

		value = (reg_data1 & 0x30) >> 4;
		value = (value << 8)|reg_data0;

		pr_info("rear pd value: 0x%x\n", value);

		rc = scnprintf(buf, PAGE_SIZE, "%d\n", value);
	}

	if (rc)
		return rc;

	return 0;
}

static ssize_t rear_tof_check_pd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_camera_slave_info *slave_info;
	uint32_t value = 0;

	if (g_s_ctrl_tof == NULL)
		return -1;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	slave_info = &(g_s_ctrl_tof->sensordata->slave_info);

	if (value > 0) {
		if (slave_info->sensor_id == TOF_SENSOR_ID_IMX516 ||
				slave_info->sensor_id == TOF_SENSOR_ID_IMX518)  {
			pr_info("[TOF] rear pd store value: %d, ready: %d\n", value, check_pd_ready);
			if (check_pd_ready == 0)
				check_pd_ready = 1;
			pr_info("[TOF] rear pd store ready: %d\n", check_pd_ready);

			//DIF_ERR_SEL disable
			cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0501, 0x0F, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0502, 0xC4, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //for diff_err_sel
			cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

			// Diff2 Error Mask disable
			cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0501, 0x14, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //DIFF2_TH
			cam_sensor_tof_i2c_write(0x0502, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //external DIFF2_TH
			cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

			// VCSELL current write
			cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0501, 0x08, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0502, value,CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //value of ISW_FIX1&2
			cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

			msleep(70);

			// VCSELL current write
			cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0501, 0x08, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0502, value,CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //value of ISW_FIX1&2
			cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

			//DIF_ERR_SEL enable
			cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0501, 0x0F, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0502, 0x04, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //for diff_err_sel
			cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		}
		msleep(5);
	}

	return size;
}

static ssize_t rear_tof_laser_error_flag_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cam_camera_slave_info *slave_info;
	uint32_t value = 0;
	uint32_t reg_data1, reg_data0 = 0;
	int rc = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;


	slave_info = &(g_s_ctrl_tof->sensordata->slave_info);

	if(slave_info->sensor_id == TOF_SENSOR_ID_IMX516 ||
		slave_info->sensor_id == TOF_SENSOR_ID_IMX518 ) {
		cam_sensor_tof_i2c_read(0x0590, &reg_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_read(0x0591, &reg_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	}

	value = reg_data1 << 6;
	value = value | reg_data0;

	pr_info("laser error flag: 0x%x\n", value);

	rc = scnprintf(buf, PAGE_SIZE, "%x\n", value);


	if (rc)
		return rc;

	return 0;
}


static ssize_t rear_tof_laser_error_flag_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cam_camera_slave_info *slave_info;
	uint32_t value = 0;

	if (g_s_ctrl_tof == NULL)
		return -1;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	slave_info = &(g_s_ctrl_tof->sensordata->slave_info);

	if (value > 0) {
		if (slave_info->sensor_id == TOF_SENSOR_ID_IMX516 ||
				slave_info->sensor_id == TOF_SENSOR_ID_IMX518)  {
			pr_info("[TOF] laser error flag store value: %d\n", value);

			switch (value) {
                case 5:
                    // over current limit : 1.92A set
                    // Reg 0x10 = 0x04
                    cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0501, 0x10, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0502, 0x04, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    // Reg 0x11 = 0x80
                    cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0501, 0x11, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0502, 0x80, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

                    pr_info("[TOF] laser error flag store value: %d Reg 0x10 = 0x04, Reg 0x11 = 0x80\n", value);
                    break;

                case 6:
                    // TEMP_EM : high temperature 71 set
                    // Reg 0x11 = 0x01
                    cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0501, 0x11, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0502, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

                    pr_info("[TOF] laser error flag store value: %d 0x11 = 0x01\n", value);
                    break;
                case 7:
                    // LDVCC_EM 3.0 set
                    // Reg 0x11 = 0x10
                    cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0501, 0x11, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0502, 0x10, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

                    pr_info("[TOF] laser error flag store value: %d Reg 0x11 = 0x10\n", value);
                    break;
                case 12:
                    // SHORT_EM
                    // Reg 0x013 = 0x7F
                    cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0501, 0x13, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0502, 0x7F, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

                    pr_info("[TOF] laser error flag store value: %d Reg 0x013 = 0x7F\n", value);
                    break;
                case 13:
                    // ITO_EM
                    // Reg 0x014 = 0x20
                    cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0501, 0x14, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0502, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

                    pr_info("[TOF] laser error flag store value: %d Reg 0x014 = 0x20\n", value);
                    break;
                case 14:
                    // SHORT_EM
                    // Reg 0x013 = 0x7F
                    cam_sensor_tof_i2c_write(0x0403, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0405, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0407, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0501, 0x13, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0502, 0x40, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0400, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
                    cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

                    pr_info("[TOF] laser error flag store value: %d Reg 0x013 = 0x7F\n", value);
                    break;

                default :
                    pr_info("[TOF] Not support test command value ");
                    break;
			}
		}
	}

	return size;
}

static ssize_t rear_tof_fps_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;

	if (rc)
		return rc;

	return 0;
}

static ssize_t rear_tof_fps_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	uint32_t value = 0;
	struct cam_camera_slave_info *slave_info;

	if(g_s_ctrl_tof == NULL)
		return 0;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	slave_info = &(g_s_ctrl_tof->sensordata->slave_info);

	if(slave_info->sensor_id == TOF_SENSOR_ID_IMX516 ||
			slave_info->sensor_id == TOF_SENSOR_ID_IMX518)
	{
		pr_info("[TOF] rear tof uid : %x\n", rear_tof_uid);

		if(value > 999) //VGA
		{
			value = value / 1000;

			switch (value) {
				case 24:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x13, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x6F, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] VGA fps : %d\n", value);
					break;

				case 20:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x19, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0xF1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] VGA fps : %d\n", value);
					break;

				case 15:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x26, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0xF7, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] VGA fps : %d\n", value);
					break;

				case 10:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x41, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] VGA fps : %d\n", value);
					break;

				case 5:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x8F, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x22, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] VGA fps : %d\n", value);
					break;

				case 2:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x79, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x82, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] VGA fps : %d\n", value);
					break;

				case 1:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x03, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x22, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] VGA fps : %d\n", value);
					break;

				default :
					pr_info("[TOF] invalid fps : %d\n", value);
					break;
			}
		}
		else  // QVGA
		{
			switch (value) {
				case 30:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x13, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x78, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] QVGA fps : %d\n", value);
					break;

				case 24:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x19, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0xFB, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] QVGA fps : %d\n", value);
					break;

				case 20:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x7D, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] QVGA fps : %d\n", value);
					break;

				case 15:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x2D, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x83, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] QVGA fps : %d\n", value);
					break;

				case 10:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x47, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x8E, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] QVGA fps : %d\n", value);
					break;

				case 5:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x95, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0xAE, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] QVGA fps : %d\n", value);
					break;

				case 2:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x80, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0x0E, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] QVGA fps : %d\n", value);
					break;

				case 1:
					cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
					cam_sensor_tof_i2c_write(0x2110, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2111, 0x03, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2112, 0x06, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x2113, 0xAE, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
					cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
					pr_info("[TOF] QVGA fps : %d\n", value);
					break;

				default :
					pr_info("[TOF] Invalid fps : %d\n", value);
					break;
			}
		}
	}
	return size;
}

static ssize_t rear_tof_freq_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_err("[SETCAL_DBG] tof_freq : %s\n", tof_freq);
	rc = scnprintf(buf, PAGE_SIZE, "%s", tof_freq);
	if (rc)
		return rc;
	return 0;
}

static ssize_t rear_tof_freq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_err("[SETCAL_DBG] buf : %s\n", buf);
	scnprintf(tof_freq, sizeof(tof_freq), "%s", buf);

	return size;
}

int rear_tof_validation_500;
int rear_tof_validation_300;
static ssize_t rear_tof_get_validation_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
    int rc= 0;

	pr_err("[SET_CAL] rear_tof_validation = %d, %d\n", rear_tof_validation_300, rear_tof_validation_500);

	rc = scnprintf(buf, PAGE_SIZE, "%d,%d\n", rear_tof_validation_300/100, rear_tof_validation_500/100);
	if (rc)
		return rc;
	return 0;
}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
char front_tof_cam_info[150] = "NULL\n";	//camera_info
static ssize_t front_tof_camera_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] cam_info : %s\n", front_tof_cam_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_tof_cam_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_tof_camera_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front_cam_info, sizeof(front_tof_cam_info), "%s", buf);

	return size;
}

char front_tof_module_info[SYSFS_MODULE_INFO_SIZE] = "NULL\n";
static ssize_t front_tof_module_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_tof_module_info : %s\n", front_tof_module_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_tof_module_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_tof_module_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_tof_module_info, sizeof(front_tof_module_info), "%s", buf);

	return size;
}

char front_tof_cam_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";
static ssize_t front_tof_camera_firmware_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_tof_cam_fw_ver : %s\n", front_tof_cam_fw_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_tof_cam_fw_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_tof_camera_firmware_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_tof_cam_fw_ver, sizeof(front_tof_cam_fw_ver), "%s", buf);

	return size;
}

char front_tof_cam_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";
static ssize_t front_tof_camera_firmware_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_tof_cam_fw_full_ver : %s\n", front_tof_cam_fw_full_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_tof_cam_fw_full_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_tof_camera_firmware_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_tof_cam_fw_full_ver, sizeof(front_tof_cam_fw_full_ver), "%s", buf);
	return size;
}

char front_tof_cam_fw_user_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
static ssize_t front_tof_camera_firmware_user_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_tof_cam_fw_ver : %s\n", front_tof_cam_fw_user_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_tof_cam_fw_user_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_tof_camera_firmware_user_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_tof_cam_fw_user_ver, sizeof(front_tof_cam_fw_user_ver), "%s", buf);

	return size;
}

char front_tof_cam_fw_factory_ver[SYSFS_FW_VER_SIZE] = "NULL\n";
static ssize_t front_tof_camera_firmware_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_tof_cam_fw_ver : %s\n", front_tof_cam_fw_factory_ver);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_tof_cam_fw_factory_ver);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_tof_camera_firmware_factory_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_tof_cam_fw_factory_ver, sizeof(front_tof_cam_fw_factory_ver), "%s", buf);

	return size;
}

char front_tof_cam_load_fw[SYSFS_FW_VER_SIZE] = "NULL\n";
static ssize_t front_tof_camera_firmware_load_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] front_tof_cam_load_fw : %s\n", front_tof_cam_load_fw);
	rc = scnprintf(buf, PAGE_SIZE, "%s", front_tof_cam_load_fw);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_tof_camera_firmware_load_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(front_tof_cam_load_fw, sizeof(front_tof_cam_load_fw), "%s\n", buf);
	return size;
}

char front_tof_sensor_id[FROM_SENSOR_ID_SIZE + 1] = "\0";
static ssize_t front_tof_sensorid_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_debug("[FW_DBG] front_tof_sensor_id : %s\n", front_tof_sensor_id);
	memcpy(buf, front_tof_sensor_id, FROM_SENSOR_ID_SIZE);
	return FROM_SENSOR_ID_SIZE;
}

static ssize_t front_tof_sensorid_exif_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
//	scnprintf(front_tof_sensor_id, sizeof(front_tof_sensor_id), "%s", buf);

	return size;
}

int front_tof_uid;
static ssize_t front_tofcal_uid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
    int rc= 0;

	pr_info("[FW_DBG] front tof uid = %d\n", front_tof_uid);

	rc = scnprintf(buf, PAGE_SIZE, "%d\n", front_tof_uid);
	if (rc)
		return rc;
	return 0;
}

uint8_t front_tof_cal[TOFCAL_SIZE + 1] = "\0";
static ssize_t front_tofcal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *ret = NULL;
	int copy_size = 0;

	pr_debug("[FW_DBG] front_tof_cal : %s\n", front_tof_cal);

	if (TOFCAL_SIZE > SYSFS_MAX_READ_SIZE)
		copy_size = SYSFS_MAX_READ_SIZE;
	else
		copy_size = TOFCAL_SIZE;

	ret = memcpy(buf, front_tof_cal, copy_size);

	if (ret)
		return copy_size;

	return 0;
}

uint8_t front_tof_cal_extra[TOFCAL_EXTRA_SIZE + 1] = "\0";
static ssize_t front_tofcal_extra_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *ret = NULL;
	int copy_size = 0;

	pr_debug("[FW_DBG] front_tof_cal_ext : %s\n", front_tof_cal_extra);

	if (TOFCAL_EXTRA_SIZE > SYSFS_MAX_READ_SIZE)
		copy_size = SYSFS_MAX_READ_SIZE;
	else
		copy_size = TOFCAL_EXTRA_SIZE;

	ret = memcpy(buf, front_tof_cal_extra, copy_size);

	if (ret)
		return copy_size;

	return 0;
}


uint32_t front_tof_cal_size = TOFCAL_TOTAL_SIZE;
static ssize_t front_tofcal_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_info("[FW_DBG] front_tof_cal_size : %d\n", front_tof_cal_size);
	rc = scnprintf(buf, PAGE_SIZE, "%d", front_tof_cal_size);
	if (rc)
		return rc;
	return 0;
}

uint8_t front_tof_cal_result = 0;
static ssize_t front_tofcal_result_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	if (front_tof_cal_result == 1)
		rc = scnprintf(buf, PAGE_SIZE, "%s", "OK\n");
	else
		rc = scnprintf(buf, PAGE_SIZE, "%s", "NG\n");
	if (rc)
		return rc;
	return 0;
}

uint8_t front_tof_dual_cal[FRONT_TOF_DUAL_CAL_SIZE + 1] = "\0";
static ssize_t front_tof_dual_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *ret = NULL;
	int copy_size = 0;

	pr_debug("[FW_DBG] front_tof_dual_cal : %s\n", front_tof_dual_cal);

	if (FRONT_TOF_DUAL_CAL_SIZE > SYSFS_MAX_READ_SIZE)
		copy_size = SYSFS_MAX_READ_SIZE;
	else
		copy_size = FRONT_TOF_DUAL_CAL_SIZE;

	ret = memcpy(buf, front_tof_dual_cal, copy_size);

	if (ret)
		return copy_size;

	return 0;
}

#if 1
DualTilt_t front_tof_dual;
static ssize_t front_tof_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear tof tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		front_tof_dual.x, front_tof_dual.y, front_tof_dual.z, front_tof_dual.sx, front_tof_dual.sy,
		front_tof_dual.range, front_tof_dual.max_err, front_tof_dual.avg_err, front_tof_dual.dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", front_tof_dual.x, front_tof_dual.y,
			front_tof_dual.z, front_tof_dual.sx, front_tof_dual.sy, front_tof_dual.range,
			front_tof_dual.max_err, front_tof_dual.avg_err, front_tof_dual.dll_ver);
	if (rc)
		return rc;
	return 0;
}
#else
int front_tof_dual_tilt_x;
int front_tof_dual_tilt_y;
int front_tof_dual_tilt_z;
int front_tof_dual_tilt_sx;
int front_tof_dual_tilt_sy;
int front_tof_dual_tilt_range;
int front_tof_dual_tilt_max_err;
int front_tof_dual_tilt_avg_err;
int front_tof_dual_tilt_dll_ver;
static ssize_t front_tof_tilt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[FW_DBG] rear tof tilt x = %d, y = %d, z = %d, sx = %d, sy = %d, range = %d, max_err = %d, avg_err = %d, dll_ver = %d\n",
		front_tof_dual_tilt_x, front_tof_dual_tilt_y, front_tof_dual_tilt_z, front_tof_dual_tilt_sx, front_tof_dual_tilt_sy,
		front_tof_dual_tilt_range, front_tof_dual_tilt_max_err, front_tof_dual_tilt_avg_err, front_tof_dual_tilt_dll_ver);

	rc = scnprintf(buf, PAGE_SIZE, "1 %d %d %d %d %d %d %d %d %d\n", front_tof_dual_tilt_x, front_tof_dual_tilt_y,
			front_tof_dual_tilt_z, front_tof_dual_tilt_sx, front_tof_dual_tilt_sy, front_tof_dual_tilt_range,
			front_tof_dual_tilt_max_err, front_tof_dual_tilt_avg_err, front_tof_dual_tilt_dll_ver);
	if (rc)
		return rc;
	return 0;
}
#endif

static ssize_t front_tof_ld_onoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t read_data = -1;
	uint32_t read_data0 = -1;
	uint32_t read_data1 = -1;
	int rc = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;
	if(g_s_ctrl_tof->id == CAMERA_6)
		return 0;

	cam_sensor_tof_i2c_read(0x2134, &read_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	cam_sensor_tof_i2c_read(0x2135, &read_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	if(read_data0 && read_data1)
		read_data = 0;
	else read_data = 1;

	rc = scnprintf(buf, PAGE_SIZE, "%d\n", read_data);
	if (rc)
		return rc;

	return 0;
}

static ssize_t front_tof_ld_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value = -1;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	if(g_s_ctrl_tof == NULL)
		return -1;
	if(g_s_ctrl_tof->id == CAMERA_6)
		return -1;

	if(value == 0) { //ld off
		cam_sensor_tof_i2c_write(0x2134, 0xAA, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x2135, 0xAA, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	}
	else if(value == 1) { //ld on
		cam_sensor_tof_i2c_write(0x2134, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x2135, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	}

	return size;
}

static ssize_t front_tof_ae_value_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t ae_value = 0;
	uint32_t reg_data3, reg_data2, reg_data1, reg_data0 = 0;
	int rc = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;
	if(g_s_ctrl_tof->id == CAMERA_6)
		return 0;

	cam_sensor_tof_i2c_read(0x2110, &reg_data3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	cam_sensor_tof_i2c_read(0x2111, &reg_data2, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	cam_sensor_tof_i2c_read(0x2112, &reg_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	cam_sensor_tof_i2c_read(0x2113, &reg_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	pr_info("value 0x%x reg_data3: 0x%2x, addr2: 0x%2x, addr1: 0x%2x, addr0: 0x%2x\n", ae_value, reg_data3,reg_data2, reg_data1,reg_data0);

	ae_value = (reg_data3<<24)|(reg_data2<<16)|(reg_data1<<8)|reg_data0;

	pr_info("ae_value: 0x%x\n", ae_value);

	rc = scnprintf(buf, PAGE_SIZE, "%d\n", ae_value/120);

	if (rc)
		return rc;

	return 0;
}

static ssize_t front_tof_ae_value_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
 	uint32_t value = 0;
	uint32_t reg_data3, reg_data2, reg_data1, reg_data0 = 0;
	uint32_t hmax = 0x02FE;

        if(g_s_ctrl_tof == NULL)
		return -1;
	if(g_s_ctrl_tof->id == CAMERA_6)
		return -1;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	value  = value * 120; // 120Mhz
	if(value >= hmax && value <= 0x1D4C0)
	{
		reg_data3 = (value & 0xFF000000) >> 24;
		reg_data2 = (value & 0xFF0000) >> 16;
		reg_data1 = (value & 0xFF00) >> 8;
		reg_data0 = value & 0xFF;
		//pr_info("value 0x%x reg_data3: 0x%2x, addr2: 0x%2x, addr1: 0x%2x, addr0: 0x%2x\n", value, reg_data3,reg_data2, reg_data1,reg_data0);

		cam_sensor_tof_i2c_write(0x2110, reg_data3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x2111, reg_data2, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x2112, reg_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x2113, reg_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

		cam_sensor_tof_i2c_write(0x2114, reg_data3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x2115, reg_data2, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x2116, reg_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x2117, reg_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	}

	return size;
}

static ssize_t front_tof_check_pd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t value = 0;
	uint32_t reg_data1, reg_data0 = 0;
	int rc = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;
	if(g_s_ctrl_tof->id == CAMERA_6)
		return 0;

	if(check_pd_ready == 0) {
		rc = scnprintf(buf, PAGE_SIZE, "NG\n");
	}
	else {
		cam_sensor_tof_i2c_write(0x0421, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0436, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0437, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x043A, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0500, 0x11, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0501, 0x90, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_read(0x058B, &reg_data1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

		cam_sensor_tof_i2c_write(0x0423, 0xA0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0436, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0437, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x043A, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0500, 0x07, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0501, 0xA0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		cam_sensor_tof_i2c_read(0x0582, &reg_data0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	        value = (reg_data1 & 0x30) >> 4;
		value = (value << 8)|reg_data0;

		pr_info("front pd value: 0x%x\n", value);

		rc = scnprintf(buf, PAGE_SIZE, "%d\n", value);
	}

	if (rc)
		return rc;

	return 0;
}

static ssize_t front_tof_check_pd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	uint32_t value = 0;

	if(g_s_ctrl_tof == NULL)
		return -1;
	if(g_s_ctrl_tof->id == CAMERA_6)
		return -1;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	if(value > 0) {
		if(check_pd_ready == 0) {
		//read enable sequence
	        cam_sensor_tof_i2c_write(0x0401, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0411, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0421, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0450, 0x43, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0454, 0x03, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0450, 0x43, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0454, 0x03, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0450, 0x47, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0454, 0x03, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0450, 0x47, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0454, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0450, 0x47, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0454, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0450, 0x47, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0454, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0433, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0433, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0435, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0436, 0x08, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0437, 0x08, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0xA3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0xA3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x25, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x12, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x20, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x0C, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x28, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x13, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x09, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x05, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x04, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x0C, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x89, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x03, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x11, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x6B, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x08, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0xB3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x0D, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0xC4, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x05, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x9C, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x07, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x41, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x0E, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0xF1, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x10, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x06, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0xBA, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x0B, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x0F, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x90, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x0A, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x04, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0xA3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0xA3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0508, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0509, 0x23, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x050A, 0x04, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0411, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0413, 0x80, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0413, 0xA0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0415, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0417, 0x18, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0518, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0519, 0x13, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x051A, 0x23, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0411, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0421, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x042A, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0423, 0x80, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0423, 0xA0, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0425, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0426, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0427, 0x1B, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0443, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0441, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0442, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x051B, 0x03, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x051C, 0x94, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x051D, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x051E, 0xA3, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x0421, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x149B, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	        cam_sensor_tof_i2c_write(0x1001, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

		check_pd_ready = 1;
		}
        //APC2 check2
	cam_sensor_tof_i2c_write(0x0436, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
        cam_sensor_tof_i2c_write(0x0437, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
        cam_sensor_tof_i2c_write(0x043A, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
        cam_sensor_tof_i2c_write(0x0500, 0x02, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
        cam_sensor_tof_i2c_write(0x0501, 0x08, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
        cam_sensor_tof_i2c_write(0x0502, value, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
        cam_sensor_tof_i2c_write(0x0431, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
        cam_sensor_tof_i2c_write(0x0430, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
        cam_sensor_tof_i2c_write(0x0431, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	}

	return size;
}

static ssize_t front_tof_fps_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;
	if(g_s_ctrl_tof->id == CAMERA_6)
		return 0;

	if (rc)
		return rc;

	return 0;
}

static ssize_t front_tof_fps_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	uint32_t value = 0;

	if(g_s_ctrl_tof == NULL)
		return 0;
	if(g_s_ctrl_tof->id == CAMERA_6)
		return 0;

	if (buf == NULL || kstrtouint(buf, 10, &value))
		return -1;

	switch (value) {
		case 30:
			cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
			cam_sensor_tof_i2c_write(0x2154, 0x0A, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2155, 0x3E, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
			break;

		case 20:
			cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
			cam_sensor_tof_i2c_write(0x2154, 0x14, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2155, 0x71, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
			break;

		case 15:
			cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
			cam_sensor_tof_i2c_write(0x2154, 0x1E, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2155, 0xA4, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
			break;

		case 10:
			cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
			cam_sensor_tof_i2c_write(0x2154, 0x33, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2155, 0x0A, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
			break;

		case 5:
			cam_sensor_tof_i2c_write(0x0104, 0x01, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold On
			cam_sensor_tof_i2c_write(0x2154, 0x70, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x2155, 0x3C, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			cam_sensor_tof_i2c_write(0x0104, 0x00, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //GroupHold Off
			break;

		default :
			pr_err("[TOF] invalid fps : %d\n", value);
			break;
	}

	return size;
}

static ssize_t front_tof_freq_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_err("[SETCAL_DBG] tof_freq : %s\n", tof_freq);
	rc = scnprintf(buf, PAGE_SIZE, "%s", tof_freq);
	if (rc)
		return rc;
	return 0;
}

static ssize_t front_tof_freq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_err("[SETCAL_DBG] buf : %s\n", buf);
	scnprintf(tof_freq, sizeof(tof_freq), "%s", buf);

	return size;
}

#endif

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
static ssize_t adaptive_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_err("[dynamic_mipi] band_info : %s\n", band_info);
	rc = scnprintf(buf, PAGE_SIZE, "%s", band_info);
	if (rc)
		return rc;
	return 0;
}

static ssize_t adaptive_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_err("[dynamic_mipi] buf : %s\n", buf);
	scnprintf(band_info, sizeof(band_info), "%s", buf);

	return size;
}
#endif

#if defined(CONFIG_CAMERA_SSM_I2C_ENV)
static DEVICE_ATTR(ssm_frame_id, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_ssm_frame_id_show, rear_ssm_frame_id_store);
static DEVICE_ATTR(ssm_gmc_state, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_ssm_gmc_state_show, rear_ssm_gmc_state_store);
static DEVICE_ATTR(ssm_flicker_state, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_ssm_flicker_state_show, rear_ssm_flicker_state_store);
#if defined(CONFIG_SEC_BLOOMXQ_PROJECT)
static DEVICE_ATTR(ssm_precheck_md, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_ssm_precheck_md_show, rear_ssm_precheck_md_store);
#endif
#endif

static DEVICE_ATTR(rear_camtype, S_IRUGO, rear_type_show, NULL);
static DEVICE_ATTR(rear_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_firmware_show, rear_firmware_store);
static DEVICE_ATTR(rear_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_firmware_user_show, rear_firmware_user_store);
static DEVICE_ATTR(rear_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_firmware_factory_show, rear_firmware_factory_store);
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
static DEVICE_ATTR(rear3_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	rear3_firmware_user_show, rear3_firmware_user_store);
static DEVICE_ATTR(rear3_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	rear3_firmware_factory_show, rear3_firmware_factory_store);
#endif
static DEVICE_ATTR(rear_camfw_full, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_firmware_full_show, rear_firmware_full_store);
static DEVICE_ATTR(rear_camfw_load, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_firmware_load_show, rear_firmware_load_store);
static DEVICE_ATTR(rear_calcheck, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_cal_data_check_show, rear_cal_data_check_store);
static DEVICE_ATTR(rear_moduleinfo, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_module_info_show, rear_module_info_store);
static DEVICE_ATTR(isp_core, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_isp_core_check_show, rear_isp_core_check_store);
static DEVICE_ATTR(rear_afcal, S_IRUGO, rear_afcal_show, NULL);
static DEVICE_ATTR(rear_paf_offset_far, S_IRUGO,
	rear_paf_offset_far_show, NULL);
static DEVICE_ATTR(rear_paf_offset_mid, S_IRUGO,
	rear_paf_offset_mid_show, NULL);
static DEVICE_ATTR(rear_paf_cal_check, S_IRUGO,
	rear_paf_cal_check_show, NULL);
static DEVICE_ATTR(rear_f2_paf_offset_far, S_IRUGO,
	rear_f2_paf_offset_far_show, NULL);
static DEVICE_ATTR(rear_f2_paf_offset_mid, S_IRUGO,
	rear_f2_paf_offset_mid_show, NULL);
static DEVICE_ATTR(rear_f2_paf_cal_check, S_IRUGO,
	rear_f2_paf_cal_check_show, NULL);
#if 0 //EARLY_RETENTION
static DEVICE_ATTR(rear_early_retention, S_IRUGO, rear_early_retention_show, NULL);
#endif
#if !(defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM) || defined(CONFIG_SAMSUNG_FRONT_FIXED_FOCUS))
static DEVICE_ATTR(front_afcal, S_IRUGO, front_camera_afcal_show, NULL);
#endif
static DEVICE_ATTR(front_camtype, S_IRUGO, front_camera_type_show, NULL);
static DEVICE_ATTR(front_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	front_camera_firmware_show, front_camera_firmware_store);
static DEVICE_ATTR(front_camfw_full, S_IRUGO | S_IWUSR | S_IWGRP,
	front_camera_firmware_full_show, front_camera_firmware_full_store);
static DEVICE_ATTR(front_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	front_camera_firmware_user_show, front_camera_firmware_user_store);
static DEVICE_ATTR(front_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	front_camera_firmware_factory_show, front_camera_firmware_factory_store);
#if defined (CONFIG_CAMERA_SYSFS_V2)
static DEVICE_ATTR(rear_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_camera_info_show, rear_camera_info_store);
static DEVICE_ATTR(front_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front_camera_info_show, front_camera_info_store);
#endif
static DEVICE_ATTR(front_paf_cal_check, S_IRUGO,
	front_paf_cal_check_show, NULL);
static DEVICE_ATTR(front_moduleinfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front_module_info_show, front_module_info_store);
static DEVICE_ATTR(rear_sensorid_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_sensorid_exif_show, rear_sensorid_exif_store);
static DEVICE_ATTR(front_sensorid_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	front_sensorid_exif_show, front_sensorid_exif_store);
static DEVICE_ATTR(rear_moduleid, S_IRUGO, rear_moduleid_show, NULL);
static DEVICE_ATTR(front_moduleid, S_IRUGO, front_camera_moduleid_show, NULL);
static DEVICE_ATTR(front_mtf_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	front_mtf_exif_show, front_mtf_exif_store);
#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
static DEVICE_ATTR(front_mipi_clock, S_IRUGO, front_camera_mipi_clock_show, NULL);
#endif
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
static DEVICE_ATTR(front2_camtype, S_IRUGO, front2_camera_type_show, NULL);
static DEVICE_ATTR(front2_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_firmware_show, front2_camera_firmware_store);
static DEVICE_ATTR(front2_camfw_full, S_IRUGO | S_IWUSR | S_IWGRP,
	front2_camera_firmware_full_show, front2_camera_firmware_full_store);
static DEVICE_ATTR(front2_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_firmware_user_show, front2_camera_firmware_user_store);
static DEVICE_ATTR(front2_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_firmware_factory_show, front2_camera_firmware_factory_store);
static DEVICE_ATTR(front2_moduleinfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_module_info_show, front2_module_info_store);
#if defined (CONFIG_CAMERA_SYSFS_V2)
static DEVICE_ATTR(front2_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_info_show, front2_camera_info_store);
#endif
static DEVICE_ATTR(front2_sensorid_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_sensorid_exif_show, front2_sensorid_exif_store);
#endif
#if defined(CONFIG_SAMSUNG_FRONT_DUAL) || defined(CONFIG_SAMSUNG_FRONT_TOF)
static DEVICE_ATTR(front2_moduleid, S_IRUGO, front2_camera_moduleid_show, NULL);
#endif

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
static DEVICE_ATTR(front3_camtype, S_IRUGO, front3_camera_type_show, NULL);
static DEVICE_ATTR(front3_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	front3_camera_firmware_show, front3_camera_firmware_store);
static DEVICE_ATTR(front3_camfw_full, S_IRUGO | S_IWUSR | S_IWGRP,
	front3_camera_firmware_full_show, front3_camera_firmware_full_store);
static DEVICE_ATTR(front3_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	front3_camera_firmware_user_show, front3_camera_firmware_user_store);
static DEVICE_ATTR(front3_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	front3_camera_firmware_factory_show, front3_camera_firmware_factory_store);
static DEVICE_ATTR(front3_moduleinfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front3_module_info_show, front3_module_info_store);
#if !defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM)
static DEVICE_ATTR(front3_afcal, S_IRUGO, front3_camera_afcal_show, NULL);
#endif
static DEVICE_ATTR(front3_moduleid, S_IRUGO, front3_camera_moduleid_show, NULL);
static DEVICE_ATTR(SVC_upper_module, S_IRUGO, front3_camera_moduleid_show, NULL);
#if defined (CONFIG_CAMERA_SYSFS_V2)
static DEVICE_ATTR(front3_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front3_camera_info_show, front3_camera_info_store);
#endif
static DEVICE_ATTR(front3_sensorid_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	front3_sensorid_exif_show, front3_sensorid_exif_store);
#else
static DEVICE_ATTR(front2_camtype, S_IRUGO, front2_camera_type_show, NULL);
static DEVICE_ATTR(front2_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_firmware_show, front2_camera_firmware_store);
static DEVICE_ATTR(front2_camfw_full, S_IRUGO | S_IWUSR | S_IWGRP,
	front2_camera_firmware_full_show, front2_camera_firmware_full_store);
static DEVICE_ATTR(front2_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_firmware_user_show, front2_camera_firmware_user_store);
static DEVICE_ATTR(front2_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_firmware_factory_show, front2_camera_firmware_factory_store);
static DEVICE_ATTR(front2_moduleinfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_module_info_show, front2_module_info_store);
#if !defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM)
static DEVICE_ATTR(front2_afcal, S_IRUGO, front2_camera_afcal_show, NULL);
#endif
static DEVICE_ATTR(front2_moduleid, S_IRUGO, front2_camera_moduleid_show, NULL);
static DEVICE_ATTR(SVC_upper_module, S_IRUGO, front2_camera_moduleid_show, NULL);
#if defined (CONFIG_CAMERA_SYSFS_V2)
static DEVICE_ATTR(front2_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_info_show, front2_camera_info_store);
#endif
static DEVICE_ATTR(front2_sensorid_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_sensorid_exif_show, front2_sensorid_exif_store);
#endif
#endif

static DEVICE_ATTR(supported_cameraIds, S_IRUGO|S_IWUSR|S_IWGRP,
	supported_camera_ids_show, supported_camera_ids_store);

static DEVICE_ATTR(rear_mtf_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_mtf_exif_show, rear_mtf_exif_store);
static DEVICE_ATTR(rear_mtf2_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_mtf2_exif_show, rear_mtf2_exif_store);
static DEVICE_ATTR(SVC_rear_module, S_IRUGO, rear_moduleid_show, NULL);
static DEVICE_ATTR(SVC_front_module, S_IRUGO, front_camera_moduleid_show, NULL);
static DEVICE_ATTR(ssrm_camera_info, S_IRUGO|S_IWUSR|S_IWGRP,
	ssrm_camera_info_show, ssrm_camera_info_store);

#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
static DEVICE_ATTR(iris_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	iris_camera_firmware_show, iris_camera_firmware_store);
static DEVICE_ATTR(iris_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	iris_camera_firmware_user_show, iris_camera_firmware_user_store);
static DEVICE_ATTR(iris_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	iris_camera_firmware_factory_show, iris_camera_firmware_factory_store);
static DEVICE_ATTR(iris_camfw_full, S_IRUGO|S_IWUSR|S_IWGRP,
	iris_camera_firmware_full_show, iris_camera_firmware_full_store);
#if defined(CONFIG_CAMERA_SYSFS_V2)
static DEVICE_ATTR(iris_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	iris_camera_info_show, iris_camera_info_store);
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(rear3_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	rear3_firmware_show, rear3_firmware_store);
static DEVICE_ATTR(rear3_camfw_full, S_IRUGO|S_IWUSR|S_IWGRP,
	rear3_firmware_full_show, rear3_firmware_full_store);
static DEVICE_ATTR(rear3_afcal, S_IRUGO, rear3_afcal_show, NULL);
static DEVICE_ATTR(rear3_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	rear3_camera_info_show, rear3_camera_info_store);
static DEVICE_ATTR(rear3_camtype, S_IRUGO, rear3_type_show, NULL);
static DEVICE_ATTR(rear3_moduleinfo, S_IRUGO|S_IWUSR|S_IWGRP,
	rear3_module_info_show, rear3_module_info_store);
static DEVICE_ATTR(rear3_mtf_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	rear3_mtf_exif_show, rear3_mtf_exif_store);
static DEVICE_ATTR(rear3_sensorid_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	rear3_sensorid_exif_show, rear3_sensorid_exif_store);
static DEVICE_ATTR(rear3_dualcal, S_IRUGO, rear3_dual_cal_show, NULL);
static DEVICE_ATTR(rear3_dualcal_size, S_IRUGO, rear3_dual_cal_size_show, NULL);

static DEVICE_ATTR(rear3_tilt, S_IRUGO, rear3_tilt_show, NULL);
static DEVICE_ATTR(rear3_paf_cal_check, S_IRUGO,
	rear3_paf_cal_check_show, NULL);
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
static DEVICE_ATTR(rear3_moduleid, S_IRUGO, rear3_moduleid_show, NULL);
static DEVICE_ATTR(SVC_rear_module3, S_IRUGO, rear3_moduleid_show, NULL);
#endif
#endif
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(rear2_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	rear2_camera_info_show, rear2_camera_info_store);
static DEVICE_ATTR(rear2_moduleinfo, S_IRUGO|S_IWUSR|S_IWGRP,
	rear2_module_info_show, rear2_module_info_store);
static DEVICE_ATTR(rear2_mtf_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	rear2_mtf_exif_show, rear2_mtf_exif_store);
static DEVICE_ATTR(rear2_sensorid_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	rear2_sensorid_exif_show, rear2_sensorid_exif_store);
static DEVICE_ATTR(rear2_moduleid, S_IRUGO,
	rear2_moduleid_show, NULL);
static DEVICE_ATTR(rear2_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	rear2_firmware_show, rear2_firmware_store);
static DEVICE_ATTR(rear2_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	rear2_firmware_user_show, rear2_firmware_user_store);
static DEVICE_ATTR(rear2_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	rear2_firmware_factory_show, rear2_firmware_factory_store);
static DEVICE_ATTR(rear2_camfw_full, S_IRUGO|S_IWUSR|S_IWGRP,
	rear2_firmware_full_show, rear2_firmware_full_store);
static DEVICE_ATTR(SVC_rear_module2, S_IRUGO, rear2_moduleid_show, NULL);
static DEVICE_ATTR(rear2_camtype, S_IRUGO, rear2_type_show, NULL);
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(rear2_dualcal, S_IRUGO, rear2_dual_cal_show, NULL);
static DEVICE_ATTR(rear2_dualcal_size, S_IRUGO, rear2_dual_cal_size_show, NULL);
static DEVICE_ATTR(rear2_tilt, S_IRUGO, rear2_tilt_show, NULL);
#endif
#endif

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
static DEVICE_ATTR(rear_hwparam, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_camera_hw_param_show, rear_camera_hw_param_store);
static DEVICE_ATTR(front_hwparam, S_IRUGO|S_IWUSR|S_IWGRP,
	front_camera_hw_param_show, front_camera_hw_param_store);
#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
static DEVICE_ATTR(iris_hwparam, S_IRUGO|S_IWUSR|S_IWGRP,
	iris_camera_hw_param_show, iris_camera_hw_param_store);
#endif
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(rear2_hwparam, S_IRUGO|S_IWUSR|S_IWGRP,
	rear2_camera_hw_param_show, rear2_camera_hw_param_store);
#endif
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(rear3_hwparam, S_IRUGO|S_IWUSR|S_IWGRP,
	rear3_camera_hw_param_show, rear3_camera_hw_param_store);
#endif
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
static DEVICE_ATTR(front2_hwparam, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_hw_param_show, front2_camera_hw_param_store);

static DEVICE_ATTR(front2_dualcal, S_IRUGO, front2_dual_cal_show, NULL);
static DEVICE_ATTR(front2_dualcal_size, S_IRUGO, front2_dual_cal_size_show, NULL);
static DEVICE_ATTR(front2_tilt, S_IRUGO, front2_tilt_show, NULL);

#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
static DEVICE_ATTR(front3_hwparam, S_IRUGO|S_IWUSR|S_IWGRP,
	front3_camera_hw_param_show, front3_camera_hw_param_store);
#else
static DEVICE_ATTR(front2_hwparam, S_IRUGO|S_IWUSR|S_IWGRP,
	front2_camera_hw_param_show, front2_camera_hw_param_store);
#endif
#endif
#endif

static DEVICE_ATTR(rear_actuator_power, S_IWUSR|S_IWGRP, NULL, rear_actuator_power_store);

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
static DEVICE_ATTR(ois_power, S_IWUSR, NULL, ois_power_store);
static DEVICE_ATTR(autotest, S_IRUGO|S_IWUSR|S_IWGRP, ois_autotest_show, ois_autotest_store);
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(autotest_2nd, S_IRUGO|S_IWUSR|S_IWGRP, ois_autotest_2nd_show, ois_autotest_2nd_store);
#endif
static DEVICE_ATTR(calibrationtest, S_IRUGO, gyro_calibration_show, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, gyro_selftest_show, NULL);
static DEVICE_ATTR(ois_rawdata, S_IRUGO, gyro_rawdata_test_show, NULL);
static DEVICE_ATTR(oisfw, S_IRUGO|S_IWUSR|S_IWGRP, ois_fw_full_show, ois_fw_full_store);
static DEVICE_ATTR(ois_exif, S_IRUGO|S_IWUSR|S_IWGRP, ois_exif_show, ois_exif_store);
static DEVICE_ATTR(reset_check, S_IRUGO, ois_reset_check, NULL);
static DEVICE_ATTR(ois_set_mode, S_IWUSR, NULL, ois_set_mode_store);
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(ois_hall_position, S_IRUGO, ois_hall_position_show, NULL);
#endif
#endif
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
static DEVICE_ATTR(ois_gain_rear, S_IRUGO, ois_gain_rear_show, NULL);
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(ois_gain_rear3, S_IRUGO, ois_gain_rear3_show, NULL);
#endif
static DEVICE_ATTR(ois_supperssion_ratio_rear, S_IRUGO, ois_supperssion_ratio_rear_show, NULL);
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(ois_supperssion_ratio_rear3, S_IRUGO, ois_supperssion_ratio_rear3_show, NULL);
static DEVICE_ATTR(rear3_read_cross_talk, S_IRUGO, ois_rear3_read_cross_talk_show, NULL);
#endif
static DEVICE_ATTR(check_cross_talk, S_IRUGO, ois_check_cross_talk_show, NULL);
static DEVICE_ATTR(ois_ext_clk, S_IRUGO|S_IWUSR|S_IWGRP, ois_ext_clk_show, ois_ext_clk_store);
#endif

#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
static DEVICE_ATTR(rear_frs_test, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_camera_frs_test_show, rear_camera_frs_test_store);
#endif
#if defined(CONFIG_CAMERA_FAC_LN_TEST)
static DEVICE_ATTR(cam_ln_test, S_IRUGO|S_IWUSR|S_IWGRP,
	cam_factory_ln_test_show, cam_factory_ln_test_store);
#endif

#if defined(CONFIG_SAMSUNG_APERTURE)
#if defined(CONFIG_SAMSUNG_APERTURE_HALLTEST)
static DEVICE_ATTR(rear_aperture_halltest, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_aperture_halltest_show, rear_aperture_halltest_store);
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_TOF)
static DEVICE_ATTR(rear_tof_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_camera_info_show, rear_tof_camera_info_store);
static DEVICE_ATTR(rear_tof_moduleinfo, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_module_info_show, rear_tof_module_info_store);
static DEVICE_ATTR(rear_tof_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_camera_firmware_show, rear_tof_camera_firmware_store);
static DEVICE_ATTR(rear_tof_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_firmware_user_show, rear_tof_firmware_user_store);
static DEVICE_ATTR(rear_tof_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_firmware_factory_show, rear_tof_firmware_factory_store);
static DEVICE_ATTR(rear_tof_camfw_full, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_firmware_full_show, rear_tof_firmware_full_store);
static DEVICE_ATTR(rear_tof_camfw_load, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_firmware_load_show, rear_tof_firmware_load_store);
static DEVICE_ATTR(rear_tof_moduleid, S_IRUGO, rear_tof_moduleid_show, NULL);
static DEVICE_ATTR(rear4_moduleid, S_IRUGO, rear_tof_moduleid_show, NULL);
static DEVICE_ATTR(rear_tof_sensorid_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_sensorid_exif_show, rear_tof_sensorid_exif_store);
static DEVICE_ATTR(rear_tofcal_uid, S_IRUGO, rear_tofcal_uid_show, NULL);
static DEVICE_ATTR(rear_tofcal, S_IRUGO, rear_tofcal_show, NULL);
static DEVICE_ATTR(rear_tofcal_extra, S_IRUGO, rear_tofcal_extra_show, NULL);
static DEVICE_ATTR(rear_tofcal_size, S_IRUGO, rear_tofcal_size_show, NULL);
static DEVICE_ATTR(rear_tof_cal_result, S_IRUGO, rear_tofcal_result_show, NULL);
static DEVICE_ATTR(rear_tof_tilt, S_IRUGO, rear_tof_tilt_show, NULL);
static DEVICE_ATTR(rear2_tof_tilt, S_IRUGO, rear2_tof_tilt_show, NULL);
static DEVICE_ATTR(rear_tof_ld_onoff, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_ld_onoff_show, rear_tof_ld_onoff_store);
static DEVICE_ATTR(rear_tof_dual_cal, S_IRUGO, rear_tof_dual_cal_show, NULL);
static DEVICE_ATTR(rear_tof_ae_value, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_ae_value_show, rear_tof_ae_value_store);
static DEVICE_ATTR(SVC_rear_module4, S_IRUGO, rear_tof_moduleid_show, NULL);
static DEVICE_ATTR(rear_tof_check_pd, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_check_pd_show, rear_tof_check_pd_store);
static DEVICE_ATTR(rear_tof_laser_error_flag, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_laser_error_flag_show, rear_tof_laser_error_flag_store);
static DEVICE_ATTR(rear_tof_fps, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_fps_show, rear_tof_fps_store);
static DEVICE_ATTR(rear_tof_freq, S_IRUGO|S_IWUSR|S_IWGRP,
	rear_tof_freq_show, rear_tof_freq_store);
static DEVICE_ATTR(rear_tof_get_validation, S_IRUGO,
	rear_tof_get_validation_show, NULL);
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
static DEVICE_ATTR(front_tof_caminfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_camera_info_show, front_tof_camera_info_store);
static DEVICE_ATTR(front_tof_moduleinfo, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_module_info_show, front_tof_module_info_store);
static DEVICE_ATTR(front_tof_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_camera_firmware_show, front_tof_camera_firmware_store);
static DEVICE_ATTR(front_tof_checkfw_user, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_camera_firmware_user_show, front_tof_camera_firmware_user_store);
static DEVICE_ATTR(front_tof_checkfw_factory, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_camera_firmware_factory_show, front_tof_camera_firmware_factory_store);
static DEVICE_ATTR(front_tof_camfw_full, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_camera_firmware_full_show, front_tof_camera_firmware_full_store);
static DEVICE_ATTR(front_tof_camfw_load, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_camera_firmware_load_show, front_tof_camera_firmware_load_store);
static DEVICE_ATTR(front_tof_sensorid_exif, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_sensorid_exif_show, front_tof_sensorid_exif_store);
static DEVICE_ATTR(front_tofcal_uid, S_IRUGO, front_tofcal_uid_show, NULL);
static DEVICE_ATTR(front_tofcal, S_IRUGO, front_tofcal_show, NULL);
static DEVICE_ATTR(front_tofcal_extra, S_IRUGO, front_tofcal_extra_show, NULL);
static DEVICE_ATTR(front_tofcal_size, S_IRUGO, front_tofcal_size_show, NULL);
static DEVICE_ATTR(front_tof_cal_result, S_IRUGO, front_tofcal_result_show, NULL);
static DEVICE_ATTR(front_tof_tilt, S_IRUGO, front_tof_tilt_show, NULL);
static DEVICE_ATTR(front_tof_ld_onoff, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_ld_onoff_show, front_tof_ld_onoff_store);
static DEVICE_ATTR(front_tof_dual_cal, S_IRUGO, front_tof_dual_cal_show, NULL);
static DEVICE_ATTR(front_tof_ae_value, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_ae_value_show, front_tof_ae_value_store);
static DEVICE_ATTR(SVC_front_module2, S_IRUGO, front2_camera_moduleid_show, NULL);
static DEVICE_ATTR(front_tof_check_pd, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_check_pd_show, front_tof_check_pd_store);
static DEVICE_ATTR(front_tof_fps, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_fps_show, front_tof_fps_store);
static DEVICE_ATTR(front_tof_freq, S_IRUGO|S_IWUSR|S_IWGRP,
	front_tof_freq_show, front_tof_freq_store);
#endif

#if defined(CONFIG_LEDS_PMIC_QPNP)
static DEVICE_ATTR(rear_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH, NULL, flash_power_store);
#endif

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
static DEVICE_ATTR(adaptive_test, S_IRUGO|S_IWUSR|S_IWGRP,
	adaptive_test_show, adaptive_test_store);
#endif

int svc_cheating_prevent_device_file_create(struct kobject **obj)
{
	struct kernfs_node *SVC_sd;
	struct kobject *data;
	struct kobject *Camera;

	/* To find SVC kobject */
	SVC_sd = sysfs_get_dirent(devices_kset->kobj.sd, "svc");
	if (IS_ERR_OR_NULL(SVC_sd)) {
		/* try to create SVC kobject */
		data = kobject_create_and_add("SVC", &devices_kset->kobj);
		if (IS_ERR_OR_NULL(data))
			pr_info("Failed to create sys/devices/svc already exist svc : 0x%pK\n", data);
		else
			pr_info("Success to create sys/devices/svc svc : 0x%pK\n", data);
	} else {
		data = (struct kobject *)SVC_sd->priv;
		pr_info("Success to find SVC_sd : 0x%pK SVC : 0x%pK\n", SVC_sd, data);
	}

	Camera = kobject_create_and_add("Camera", data);
	if (IS_ERR_OR_NULL(Camera))
		pr_info("Failed to create sys/devices/svc/Camera : 0x%pK\n", Camera);
	else
		pr_info("Success to create sys/devices/svc/Camera : 0x%pK\n", Camera);

	*obj = Camera;
	return 0;
}


#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
char af_position_value[128] = "0\n";
static ssize_t af_position_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[syscamera] af_position_show : %s\n", af_position_value);
	rc = snprintf(buf, PAGE_SIZE, "%s", af_position_value);
	if (rc)
		return rc;
	return 0;
}

static ssize_t af_position_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(af_position_value, sizeof(af_position_value), "%s", buf);
	return size;
}
static DEVICE_ATTR(af_position, S_IRUGO|S_IWUSR|S_IWGRP,
	af_position_show, af_position_store);

char dual_fallback_value[SYSFS_FW_VER_SIZE] = "0\n";
static ssize_t dual_fallback_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;

	pr_debug("[syscamera] dual_fallback_show : %s\n", dual_fallback_value);
	rc = scnprintf(buf, PAGE_SIZE, "%s", dual_fallback_value);
	if (rc)
		return rc;
	return 0;
}

static ssize_t dual_fallback_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_debug("[FW_DBG] buf : %s\n", buf);
	scnprintf(dual_fallback_value, sizeof(dual_fallback_value), "%s", buf);
	return size;
}
static DEVICE_ATTR(fallback, S_IRUGO|S_IWUSR|S_IWGRP,
	dual_fallback_show, dual_fallback_store);
#endif

static int __init cam_sysfs_init(void)
{
	struct device		  *cam_dev_rear;
	struct device		  *cam_dev_front;
#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
	struct device		  *cam_dev_iris;
#endif
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	struct device		  *cam_dev_af;
	struct device		  *cam_dev_dual;
#endif
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	struct device		  *cam_dev_ois;
#endif
#if defined(CONFIG_CAMERA_SSM_I2C_ENV)
	struct device		  *cam_dev_ssm;
#endif
#if defined(CONFIG_LEDS_PMIC_QPNP)
	struct device		  *cam_dev_flash;
#endif
#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
	struct device		  *cam_dev_test;
#endif

	struct kobject *SVC = 0;
	int ret = 0;

	svc_cheating_prevent_device_file_create(&SVC);

	if (camera_class == NULL) {
		camera_class = class_create(THIS_MODULE, "camera");
		if (IS_ERR(camera_class))
			pr_err("failed to create device cam_dev_rear!\n");
	}

#if defined(CONFIG_CAMERA_SSM_I2C_ENV)
	cam_dev_ssm = device_create(camera_class, NULL,
		0, NULL, "ssm");

	if (device_create_file(cam_dev_ssm, &dev_attr_ssm_frame_id) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_ssm_frame_id.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_ssm, &dev_attr_ssm_gmc_state) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_ssm_gmc_state.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_ssm, &dev_attr_ssm_flicker_state) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_ssm_flicker_state.attr.name);
		ret = -ENODEV;
	}
#if defined(CONFIG_SEC_BLOOMXQ_PROJECT)
	if (device_create_file(cam_dev_ssm, &dev_attr_ssm_precheck_md) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_ssm_precheck_md.attr.name);
		ret = -ENODEV;
	}
#endif
#endif

	cam_dev_rear = device_create(camera_class, NULL,
		1, NULL, "rear");

	if (device_create_file(cam_dev_rear, &dev_attr_rear_camtype) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_camtype.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_camfw.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_checkfw_user.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_checkfw_factory.attr.name);
		ret = -ENODEV;
	}
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_moduleid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_moduleid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_checkfw_user.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_checkfw_factory.attr.name);
		ret = -ENODEV;
	}
#endif
	if (device_create_file(cam_dev_rear, &dev_attr_rear_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_camfw_full.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_camfw_load) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_camfw_load.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_calcheck) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_calcheck.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_moduleinfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_moduleinfo.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_isp_core) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_isp_core.attr.name);
		ret = -ENODEV;
	}
#if defined(CONFIG_CAMERA_SYSFS_V2)
	if (device_create_file(cam_dev_rear, &dev_attr_rear_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_caminfo.attr.name);
	}
#endif
	if (device_create_file(cam_dev_rear, &dev_attr_rear_afcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_afcal.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_paf_offset_far) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_paf_offset_far.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_paf_offset_mid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_paf_offset_mid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_paf_cal_check) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_paf_cal_check.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_f2_paf_offset_far) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_f2_paf_offset_far.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_f2_paf_offset_mid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_f2_paf_offset_mid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_f2_paf_cal_check) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_f2_paf_cal_check.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_sensorid_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_sensorid_exif.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_moduleid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_moduleid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_mtf_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_mtf_exif.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_mtf2_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_mtf2_exif.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_ssrm_camera_info) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_ssrm_camera_info.attr.name);
		ret = -ENODEV;
	}
	if (sysfs_create_file(SVC, &dev_attr_SVC_rear_module.attr) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_SVC_rear_module.attr.name);
		ret = -ENODEV;
	}
#if 0 //EARLY_RETENTION
	if (device_create_file(cam_dev_rear, &dev_attr_rear_early_retention) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_camtype.attr.name);
		ret = -ENODEV;
	}
#endif
	if (device_create_file(cam_dev_rear, &dev_attr_rear_actuator_power) < 0) {
		pr_err("failed to create device file, %s\n",
			dev_attr_rear_actuator_power.attr.name);
		ret = -ENOENT;
	}

	cam_dev_front = device_create(camera_class, NULL,
		2, NULL, "front");

	if (IS_ERR(cam_dev_front)) {
		pr_err("Failed to create cam_dev_front device!");
		ret = -ENODEV;
	}

	if (device_create_file(cam_dev_front, &dev_attr_front_camtype) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_camtype.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_camfw.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_camfw_full.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_checkfw_user.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_checkfw_factory.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_moduleinfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_moduleinfo.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_paf_cal_check) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_paf_cal_check.attr.name);
		ret = -ENODEV;
	}
#if defined(CONFIG_CAMERA_SYSFS_V2)
	if (device_create_file(cam_dev_front, &dev_attr_front_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_caminfo.attr.name);
	}
#endif
#if !(defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM) || defined(CONFIG_SAMSUNG_FRONT_FIXED_FOCUS))
	if (device_create_file(cam_dev_front, &dev_attr_front_afcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_afcal.attr.name);
		ret = -ENODEV;
	}
#endif
	if (device_create_file(cam_dev_front, &dev_attr_front_sensorid_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_sensorid_exif.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_moduleid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_moduleid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_mtf_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_mtf_exif.attr.name);
		ret = -ENODEV;
	}
	if (sysfs_create_file(SVC, &dev_attr_SVC_front_module.attr) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_SVC_front_module.attr.name);
		ret = -ENODEV;
	}

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
	if (device_create_file(cam_dev_front, &dev_attr_front2_camtype) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_camtype.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_camfw.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_camfw_full.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_checkfw_user.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_checkfw_factory.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_moduleinfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_moduleinfo.attr.name);
		ret = -ENODEV;
	}
#if defined(CONFIG_CAMERA_SYSFS_V2)
	if (device_create_file(cam_dev_front, &dev_attr_front2_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_caminfo.attr.name);
	}
#endif
	if (device_create_file(cam_dev_front, &dev_attr_front2_sensorid_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_sensorid_exif.attr.name);
		ret = -ENODEV;
	}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_DUAL) || defined(CONFIG_SAMSUNG_FRONT_TOF)
	if (device_create_file(cam_dev_front, &dev_attr_front2_moduleid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_moduleid.attr.name);
		ret = -ENODEV;
	}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
	if (device_create_file(cam_dev_front, &dev_attr_front3_camtype) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_camtype.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front3_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_camfw.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front3_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_camfw_full.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front3_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_checkfw_user.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front3_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_checkfw_factory.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front3_moduleinfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_moduleinfo.attr.name);
		ret = -ENODEV;
	}
	if (sysfs_create_file(SVC, &dev_attr_SVC_upper_module.attr) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_SVC_upper_module.attr.name);
		ret = -ENODEV;
	}
#if !defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM)
	if (device_create_file(cam_dev_front, &dev_attr_front3_afcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_afcal.attr.name);
		ret = -ENODEV;
	}
#endif
#if defined(CONFIG_CAMERA_SYSFS_V2)
	if (device_create_file(cam_dev_front, &dev_attr_front3_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_caminfo.attr.name);
	}
#endif
	if (device_create_file(cam_dev_front, &dev_attr_front3_moduleid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_moduleid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front3_sensorid_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front3_sensorid_exif.attr.name);
		ret = -ENODEV;
	}
#else
	if (device_create_file(cam_dev_front, &dev_attr_front2_camtype) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_camtype.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_camfw.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_camfw_full.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_checkfw_user.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_checkfw_factory.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_moduleinfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_moduleinfo.attr.name);
		ret = -ENODEV;
	}
	if (sysfs_create_file(SVC, &dev_attr_SVC_upper_module.attr) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_SVC_upper_module.attr.name);
		ret = -ENODEV;
	}
#if !defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM)
	if (device_create_file(cam_dev_front, &dev_attr_front2_afcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_afcal.attr.name);
		ret = -ENODEV;
	}
#endif
#if defined(CONFIG_CAMERA_SYSFS_V2)
	if (device_create_file(cam_dev_front, &dev_attr_front2_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_caminfo.attr.name);
	}
#endif
	if (device_create_file(cam_dev_front, &dev_attr_front2_moduleid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_moduleid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_sensorid_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_sensorid_exif.attr.name);
		ret = -ENODEV;
	}
#endif
#endif

	if (device_create_file(cam_dev_rear, &dev_attr_supported_cameraIds) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_supported_cameraIds.attr.name);
	}

#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
	cam_dev_iris = device_create(camera_class, NULL,
		2, NULL, "secure");

	if (IS_ERR(cam_dev_iris)) {
		pr_err("Failed to create cam_dev_iris device!");
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_iris, &dev_attr_iris_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_iris_camfw.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_iris, &dev_attr_iris_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_iris_camfw_full.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_iris, &dev_attr_iris_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_iris_checkfw_user.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_iris, &dev_attr_iris_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_iris_checkfw_factory.attr.name);
		ret = -ENODEV;
	}
#if defined(CONFIG_CAMERA_SYSFS_V2)
	if (device_create_file(cam_dev_iris, &dev_attr_iris_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_iris_caminfo.attr.name);
	}
#endif
#endif
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_camfw.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_camfw_full.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_afcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_afcal.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_tilt) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_tilt.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_caminfo.attr.name);
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_camtype) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_camtype.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_moduleinfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_moduleinfo.attr.name);
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_mtf_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_mtf_exif.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_sensorid_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_sensorid_exif.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_dualcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_dualcal.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_dualcal_size) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_dualcal_size.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_paf_cal_check) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear3_paf_cal_check.attr.name);
		ret = -ENODEV;
	}
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
	if (sysfs_create_file(SVC, &dev_attr_SVC_rear_module3.attr) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_SVC_rear_module3.attr.name);
		ret = -ENODEV;
	}
#endif
#endif
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_caminfo.attr.name);
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_moduleinfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_moduleinfo.attr.name);
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_sensorid_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_sensorid_exif.attr.name);
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_mtf_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_mtf_exif.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_moduleid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_moduleid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_camfw.attr.name);
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_checkfw_user.attr.name);
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_checkfw_factory.attr.name);
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_camfw_full.attr.name);
	}
	if (sysfs_create_file(SVC, &dev_attr_SVC_rear_module2.attr) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_SVC_rear_module2.attr.name);
		ret = -ENODEV;
	}

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_dualcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_dualcal.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_dualcal_size) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_dualcal_size.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_tilt) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_tilt.attr.name);
		ret = -ENODEV;
	}
#endif
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_camtype) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_camtype.attr.name);
		ret = -ENODEV;
	}
#endif

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
	if (device_create_file(cam_dev_rear, &dev_attr_rear_hwparam) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
				dev_attr_rear_hwparam.attr.name);
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_hwparam) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
				dev_attr_front_hwparam.attr.name);
	}
#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
	if (device_create_file(cam_dev_iris, &dev_attr_iris_hwparam) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
				dev_attr_iris_hwparam.attr.name);
	}
#endif
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_hwparam) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
				dev_attr_rear2_hwparam.attr.name);
	}
#endif
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (device_create_file(cam_dev_rear, &dev_attr_rear3_hwparam) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
				dev_attr_rear3_hwparam.attr.name);
	}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
	if (device_create_file(cam_dev_front, &dev_attr_front2_hwparam) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
				dev_attr_front2_hwparam.attr.name);
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_dualcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_dualcal.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_dualcal_size) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_dualcal_size.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front2_tilt) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front2_tilt.attr.name);
		ret = -ENODEV;
	}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
	if (device_create_file(cam_dev_front, &dev_attr_front3_hwparam) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
				dev_attr_front3_hwparam.attr.name);
	}
#else
    if (device_create_file(cam_dev_front, &dev_attr_front2_hwparam) < 0) {
        pr_err("Failed to create device file!(%s)!\n",
                dev_attr_front2_hwparam.attr.name);
    }
#endif
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	cam_dev_af = device_create(camera_class, NULL,
		1, NULL, "af");

	if (IS_ERR(cam_dev_af)) {
		pr_err("Failed to create cam_dev_af device!\n");
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_af, &dev_attr_af_position) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_af_position.attr.name);
		ret = -ENODEV;
	}

	cam_dev_dual = device_create(camera_class, NULL,
		1, NULL, "dual");
	if (IS_ERR(cam_dev_dual)) {
		pr_err("Failed to create cam_dev_dual device!\n");
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_dual, &dev_attr_fallback) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_fallback.attr.name);
		ret = -ENODEV;
	}
#endif

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	cam_dev_ois = device_create(camera_class, NULL,
		0, NULL, "ois");
	if (IS_ERR(cam_dev_ois)) {
		pr_err("Failed to create cam_dev_ois device!\n");
		ret = -ENOENT;
	}

	if (device_create_file(cam_dev_ois, &dev_attr_ois_power) < 0) {
		pr_err("failed to create device file, %s\n",
			dev_attr_ois_power.attr.name);
		ret = -ENOENT;
	}
	if (device_create_file(cam_dev_ois, &dev_attr_autotest) < 0) {
		pr_err("Failed to create device file, %s\n",
			dev_attr_autotest.attr.name);
		ret = -ENODEV;
	}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (device_create_file(cam_dev_ois, &dev_attr_autotest_2nd) < 0) {
		pr_err("Failed to create device file, %s\n",
			dev_attr_autotest_2nd.attr.name);
		ret = -ENODEV;
	}
#endif

	if (device_create_file(cam_dev_ois, &dev_attr_selftest) < 0) {
		pr_err("failed to create device file, %s\n",
		dev_attr_selftest.attr.name);
		ret = -ENOENT;
	}
	if (device_create_file(cam_dev_ois, &dev_attr_ois_rawdata) < 0) {
		pr_err("failed to create device file, %s\n",
		dev_attr_ois_rawdata.attr.name);
		ret = -ENOENT;
	}
	if (device_create_file(cam_dev_ois, &dev_attr_oisfw) < 0) {
		pr_err("Failed to create device file, %s\n",
		dev_attr_oisfw.attr.name);
		ret = -ENODEV;
	}

	if (device_create_file(cam_dev_ois, &dev_attr_ois_exif) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_ois_exif.attr.name);
		ret = -ENODEV;
	}

	if (device_create_file(cam_dev_ois, &dev_attr_calibrationtest) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_calibrationtest.attr.name);
		ret = -ENODEV;
	}

	if (device_create_file(cam_dev_ois, &dev_attr_reset_check) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_reset_check.attr.name);
		ret = -ENODEV;
	}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (device_create_file(cam_dev_ois, &dev_attr_ois_hall_position) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_ois_hall_position.attr.name);
		ret = -ENODEV;
	}
#endif

	if (device_create_file(cam_dev_ois, &dev_attr_ois_set_mode) < 0) {
	    pr_err("Failed to create device file,%s\n",
	        dev_attr_ois_set_mode.attr.name);
	    ret = -ENODEV;
	}
#endif

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
	if (device_create_file(cam_dev_ois, &dev_attr_ois_gain_rear) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_ois_gain_rear.attr.name);
		ret = -ENODEV;
	}
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (device_create_file(cam_dev_ois, &dev_attr_ois_gain_rear3) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_ois_gain_rear3.attr.name);
		ret = -ENODEV;
	}
#endif
	if (device_create_file(cam_dev_ois, &dev_attr_ois_supperssion_ratio_rear) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_ois_supperssion_ratio_rear.attr.name);
		ret = -ENODEV;
	}
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	if (device_create_file(cam_dev_ois, &dev_attr_ois_supperssion_ratio_rear3) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_ois_supperssion_ratio_rear3.attr.name);
		ret = -ENODEV;
	}

	if (device_create_file(cam_dev_ois, &dev_attr_rear3_read_cross_talk) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_rear3_read_cross_talk.attr.name);
		ret = -ENODEV;
	}
#endif

	if (device_create_file(cam_dev_ois, &dev_attr_check_cross_talk) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_check_cross_talk.attr.name);
		ret = -ENODEV;
	}

	if (device_create_file(cam_dev_ois, &dev_attr_ois_ext_clk) < 0) {
		pr_err("Failed to create device file,%s\n",
			dev_attr_ois_ext_clk.attr.name);
		ret = -ENODEV;
	}
#endif

#if defined(CONFIG_LEDS_PMIC_QPNP)
        cam_dev_flash = device_create(camera_class, NULL,
                0, NULL, "flash");
        if (IS_ERR(cam_dev_flash)) {
                pr_err("Failed to create cam_dev_flash device!\n");
                ret = -ENOENT;
        }

        if (device_create_file(cam_dev_flash, &dev_attr_rear_flash) < 0) {
                pr_err("failed to create device file, %s\n",
                        dev_attr_rear_flash.attr.name);
                ret = -ENOENT;
        }
#endif


#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
	if (device_create_file(cam_dev_front, &dev_attr_front_mipi_clock) < 0) {
		pr_err("failed to create front device file, %s\n",
		dev_attr_front_mipi_clock.attr.name);
	}
	cam_mipi_register_ril_notifier();
#endif
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
	if (device_create_file(cam_dev_rear, &dev_attr_rear_frs_test) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_frs_test.attr.name);
		ret = -ENODEV;
	}
#endif
#if defined(CONFIG_CAMERA_FAC_LN_TEST)
	if (device_create_file(cam_dev_rear, &dev_attr_cam_ln_test) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_cam_ln_test.attr.name);
		ret = -ENODEV;
	}
#endif
#if defined(CONFIG_SAMSUNG_APERTURE)
#if defined(CONFIG_SAMSUNG_APERTURE_HALLTEST)
	if (device_create_file(cam_dev_rear, &dev_attr_rear_aperture_halltest) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_aperture_halltest.attr.name);
		ret = -ENODEV;
	}
#endif
#endif
#if defined(CONFIG_SAMSUNG_REAR_TOF)
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_caminfo.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_moduleinfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_moduleinfo.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_camfw.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_checkfw_user.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_checkfw_factory.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_camfw_full.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_camfw_load) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_camfw_load.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_sensorid_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_sensorid_exif.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_moduleid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_moduleid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear4_moduleid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear4_moduleid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tofcal_uid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tofcal_uid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tofcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tofcal.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tofcal_extra) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tofcal_extra.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tofcal_size) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tofcal_size.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_cal_result) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_cal_result.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_tilt) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_tilt.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear2_tof_tilt) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear2_tof_tilt.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_ld_onoff) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_ld_onoff.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_dual_cal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_dual_cal.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_ae_value) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_ae_value.attr.name);
		ret = -ENODEV;
	}
	if (sysfs_create_file(SVC, &dev_attr_SVC_rear_module4.attr) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_SVC_rear_module4.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_check_pd) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_check_pd.attr.name);
		ret = -ENODEV;
	}
    if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_laser_error_flag) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_laser_error_flag.attr.name);
		ret = -ENODEV;
    }
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_fps) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_fps.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_freq) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_freq.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_rear, &dev_attr_rear_tof_get_validation) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_tof_get_validation.attr.name);
		ret = -ENODEV;
	}
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_caminfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_caminfo.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_moduleinfo) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_moduleinfo.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_camfw.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_checkfw_user) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_checkfw_user.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_checkfw_factory) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_checkfw_factory.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_camfw_full) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_camfw_full.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_camfw_load) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_camfw_load.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_sensorid_exif) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_sensorid_exif.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tofcal_uid) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tofcal_uid.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tofcal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tofcal.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tofcal_extra) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tofcal_extra.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tofcal_size) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tofcal_size.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_cal_result) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_cal_result.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_tilt) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_tilt.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_ld_onoff) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_ld_onoff.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_dual_cal) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_dual_cal.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_ae_value) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_ae_value.attr.name);
		ret = -ENODEV;
	}
	if (sysfs_create_file(SVC, &dev_attr_SVC_front_module2.attr) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_SVC_front_module2.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_check_pd) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_check_pd.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_fps) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_fps.attr.name);
		ret = -ENODEV;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_tof_freq) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_tof_freq.attr.name);
		ret = -ENODEV;
	}
#endif

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
	cam_dev_test = device_create(camera_class, NULL,
			0, NULL, "test");
	if (IS_ERR(cam_dev_test)) {
			pr_err("Failed to create cam_dev_flash device!\n");
			ret = -ENOENT;
	}

	if (device_create_file(cam_dev_test, &dev_attr_adaptive_test) < 0) {
			pr_err("failed to create device file, %s\n",
					dev_attr_adaptive_test.attr.name);
			ret = -ENOENT;
	}
#endif
	return ret;
}

module_init(cam_sysfs_init);
MODULE_DESCRIPTION("cam_sysfs_init");
