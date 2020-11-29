/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 */
#ifndef _CAM_EEPROM_DEV_H_
#define _CAM_EEPROM_DEV_H_

#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <media/v4l2-event.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ioctl.h>
#include <media/cam_sensor.h>
#include <cam_sensor_i2c.h>
#include <cam_sensor_spi.h>
#include <cam_sensor_io.h>
#include <cam_cci_dev.h>
#include <cam_req_mgr_util.h>
#include <cam_req_mgr_interface.h>
#include <cam_mem_mgr.h>
#include <cam_subdev.h>
#include <media/cam_sensor.h>
#include "cam_soc_util.h"
#include "cam_context.h"

#define DEFINE_MSM_MUTEX(mutexname) \
	static struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)

#define OK 1
#define CRASH 0
#define PROPERTY_MAXSIZE 32

#define MSM_EEPROM_MEMORY_MAP_MAX_SIZE          80
#define MSM_EEPROM_MAX_MEM_MAP_CNT              50
#define MSM_EEPROM_MEM_MAP_PROPERTIES_CNT       6

#define SYSFS_FW_VER_SIZE                       40
#define SYSFS_MODULE_INFO_SIZE                  96

#define FROM_MODULE_FW_INFO_SIZE                11
#define FROM_MTF_SIZE                           54
#define FROM_MODULE_ID_SIZE                     10

#define FROM_REAR_AF_CAL_SIZE                   10
#define FROM_SENSOR_ID_SIZE                     16

#define FROM_REAR_DUAL_CAL_SIZE                 2060
#define FROM_FRONT_DUAL_CAL_SIZE                1024

#define PAF_2PD_CAL_INFO_SIZE                   4096
#define PAF_SPARSEPD_CAL_INFO_SIZE              2048
#define PAF_CAL_ERR_CHECK_OFFSET                0x14

#define CAMERA_CAL_CRC_WIDE                     0x1FFF
#define FROM_REAR_HEADER_SIZE                   0x0200

#define HW_INFO_MAX_SIZE                        (6)
#define SW_INFO_MAX_SIZE                        (5)
#define VENDOR_INFO_MAX_SIZE                    (2)
#define PROCESS_INFO_MAX_SIZE                   (2)
#define PROJECT_CAL_TYPE_MAX_SIZE               (20)

#define MAKE_STRINGIZE(arg) #arg

#define X_ENUMS                \
	X(DEF_M_CORE_VER)          \
	X(DEF_M_VER_HW)            \
	X(DEF_M_VER_SW)            \
	X(DEF_M_VER_ETC)           \
	X(DEF_S_VER_HW)            \
	X(DEF_M_CHK_VER)           \
	X(SIZE_M_PAF_CAL)          \
	X(SIZE_S_PAF_CAL)          \
	X(SIZE_M_DUAL_CAL)         \
	X(SIZE_ONLY_M_CAL_CRC)     \
	X(ADDR_M_HEADER)           \
	X(ADDR_S_FW_VER)           \
	X(ADDR_M_FW_VER)           \
	X(ADDR_M_CALMAP_VER)       \
	X(ADDR_M_DLL_VER)          \
	X(ADDR_S_DLL_VER)          \
	X(ADDR_M_MODULE_ID)        \
	X(ADDR_M_SENSOR_ID)        \
	X(ADDR_M_SENSOR_VER)       \
	X(ADDR_S_SENSOR_ID)        \
	X(ADDR_M0_MTF)             \
	X(ADDR_M1_MTF)             \
	X(ADDR_M2_MTF)             \
	X(ADDR_S0_MTF)             \
	X(ADDR_M0_LSC)             \
	X(ADDR_M1_LSC)             \
	X(ADDR_M2_LSC)             \
	X(ADDR_M0_PAF)             \
	X(ADDR_M0_BP)              \
	X(ADDR_M0_PLC)             \
	X(ADDR_M1_PAF)             \
	X(ADDR_M1_BP)              \
	X(ADDR_M1_PLC)             \
	X(ADDR_M2_PAF)             \
	X(ADDR_M2_BP)              \
	X(ADDR_M2_PLC)             \
	X(ADDR_M_AF)               \
	X(ADDR_M0_MODULE_AWB)      \
	X(ADDR_M1_MODULE_AWB)      \
	X(ADDR_M2_MODULE_AWB)      \
	X(ADDR_M0_AE)              \
	X(ADDR_M1_AE)              \
	X(ADDR_M2_AE)              \
	X(ADDR_M_OIS)              \
	X(ADDR_M_CAL_VER_WHEN_CAL) \
	X(ADDR_M_DUAL_CAL)         \
	X(ADDR_M_ATC_CAL)          \
	X(ADDR_S0_LSC)             \
	X(ADDR_S0_PAF)             \
	X(ADDR_S0_BP)              \
	X(ADDR_S0_PLC)             \
	X(ADDR_S0_AF)              \
	X(ADDR_S0_MODULE_AWB)      \
	X(ADDR_S0_AE)              \
	X(ADDR_S_DUAL_CAL)         \
	X(ADDR_S_OIS)              \
	X(ADDR_4PDC_CAL)           \
	X(ADDR_TCLSC_CAL)          \
	X(ADDR_SPDC_CAL)           \
	X(ADDR_PDXTC_CAL)          \
	X(ADDR_M_XTALK_CAL)        \
	X(ADDR_TOFCAL_START)       \
	X(ADDR_TOFCAL_SIZE)        \
	X(ADDR_TOFCAL_UID)         \
	X(ADDR_TOFCAL_RESULT)      \
	X(ADDR_VALIDATION_500)     \
	X(ADDR_VALIDATION_300)     \
	X(ADDR_CUSTOM_FW_VER)      \
	X(ADDR_CUSTOM_SENSOR_ID)

typedef enum _ConfigNameInfoIdx {
#define X(Enum)       Enum,
    X_ENUMS
#undef X
	MAX_CONFIG_INFO_IDX
} eConfigNameInfoIdx;

static const char* ConfigInfoStrs[] =
{
#define X(String) MAKE_STRINGIZE(String),
    X_ENUMS
#undef X
};

typedef enum _DualTiltMode {
	DUAL_TILT_NONE,
	DUAL_TILT_REAR_WIDE,
	DUAL_TILT_REAR_UW,
	DUAL_TILT_REAR_TELE,
	DUAL_TILT_FRONT,
	DUAL_TILT_TOF_REAR ,
	DUAL_TILT_TOF_REAR2,
	DUAL_TILT_TOF_REAR3,
	DUAL_TILT_TOF_FRONT,
	DUAL_TILT_MAX
} eDualTiltMode;

#define MaximumCustomStringLength		(25)	//	should have the same value in chivendortag.h, camxpropertydefs.h

typedef enum {
	CAM_EEPROM_IDX_BACK,
	CAM_EEPROM_IDX_FRONT,
	CAM_EEPROM_IDX_BACK2,
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
	CAM_EEPROM_IDX_FRONT2,
#endif
#if defined(CONFIG_SAMSUNG_REAR_TOF)
	CAM_EEPROM_IDX_BACK_TOF,
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
	CAM_EEPROM_IDX_FRONT_TOF,
#endif
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
	CAM_EEPROM_IDX_BACK3,
#endif
	CAM_EEPROM_IDX_MAX
} cam_eeprom_idx_type;

typedef struct _cam_eeprom_configInfo_t {
	uint32_t    isSet;
	uint32_t    value;
} ConfigInfo_t;

typedef enum _MainOrSub {
	MAIN_MODULE,
	SUB_MODULE,
} eMainSub;

typedef struct _cam_eeprom_dual_tilt_t {
	int x;
	int y;
	int z;
	int sx;
	int sy;
	int range;
	int max_err;
	int avg_err;
	int dll_ver;
	char project_cal_type[PROJECT_CAL_TYPE_MAX_SIZE];
} DualTilt_t;

typedef struct _cam_eeprom_module_ver_t {
	char *sensor_id;
	char *sensor2_id;
	char *module_id;

	char phone_hw_info[HW_INFO_MAX_SIZE];
	char phone_sw_info[SW_INFO_MAX_SIZE];
	char phone_vendor_info[VENDOR_INFO_MAX_SIZE];
	char phone_process_info[PROCESS_INFO_MAX_SIZE];

	char module_fw_ver[FROM_MODULE_FW_INFO_SIZE+1];
	char load_fw_ver[FROM_MODULE_FW_INFO_SIZE+1];
	char phone_fw_ver[FROM_MODULE_FW_INFO_SIZE+1];

	char *module_info;
	char *cam_cal_ack;
	char *cam_fw_ver;
	char *cam_fw_full_ver;

	char *fw_factory_ver;
	char *fw_user_ver;

	uint8_t *dual_cal;
	DualTilt_t *DualTilt;
} ModuleVer_t;

typedef struct _cam_eeprom_module_info_t {
	ModuleVer_t         mVer;
	cam_eeprom_idx_type type;
	uint8_t             mapVer;
	eMainSub            M_or_S;
	char                typeStr[FROM_MODULE_FW_INFO_SIZE];
} ModuleInfo_t;

typedef enum _AfOffsetIdx {
	AF_CAL_MACRO_IDX = 0,
	AF_CAL_D10_IDX,
	AF_CAL_D20_IDX,
	AF_CAL_D30_IDX,
	AF_CAL_D40_IDX,
	AF_CAL_D50_IDX,
	AF_CAL_D60_IDX,
	AF_CAL_D70_IDX,
	AF_CAL_D80_IDX,
	AF_CAL_PAN_IDX,
	AF_CAL_IDX_MAX
} eAfOffsetIdx;

typedef struct _cam_eeprom_af_idx_t {
	eAfOffsetIdx idx;
	uint32_t     offset;
} AfIdx_t;

#define AF_CAL_PAN_OFFSET_FROM_AF                   0x0004
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
#define AF_CAL_D70_OFFSET_FROM_AF                   0x0008
#define AF_CAL_D10_OFFSET_FROM_AF                   0x0010
#define AF_CAL_D80_OFFSET_FROM_AF                   0x0010
#define AF_CAL_MACRO_OFFSET_FROM_AF                 (AF_CAL_D10_OFFSET_FROM_AF)
#define AF_CAL_MACRO3_OFFSET_FROM_AF                (AF_CAL_D80_OFFSET_FROM_AF)
#else
#define AF_CAL_D80_OFFSET_FROM_AF                   0x0000
#define AF_CAL_D50_OFFSET_FROM_AF                   0x0008
#define AF_CAL_D30_OFFSET_FROM_AF                   0x0010
#define AF_CAL_D10_OFFSET_FROM_AF                   0x0010
#define AF_CAL_MACRO_OFFSET_FROM_AF                 (AF_CAL_D10_OFFSET_FROM_AF)
#endif
#define AF_CAL_D40_OFFSET_FROM_AF                   0x000C

#define PAF_OFFSET_CAL_ERR_CHECK                    (0x0014)
#define PAF_MID_SIZE                                936
#define PAF_MID_OFFSET                              (0x0730)

#define PAF_FAR_SIZE                                234
#define PAF_FAR_OFFSET                              (0x0CD0)

#define TOFCAL_START_ADDR                           0x0100
#define TOFCAL_END_ADDR                             0x11A3
#define TOFCAL_TOTAL_SIZE	                        (TOFCAL_END_ADDR - TOFCAL_START_ADDR + 1)
#define TOFCAL_SIZE                                 (4096 - 1)
#define TOFCAL_EXTRA_SIZE                           (TOFCAL_TOTAL_SIZE - TOFCAL_SIZE)
#define TOFCAL_UID_ADDR                             0x11A4
#define TOFCAL_UID                                  (TOFCAL_UID_ADDR + 0x0000)
#define TOFCAL_RESULT_ADDR                          0x00CA

#if 1
#define REAR_TOF_DUAL_CAL_SIZE                      (0x08FC)
#define FRONT_TOF_DUAL_CAL_SIZE                     (0x0800)
#else
#define REAR_TOF_DUAL_CAL_ADDR                      0xB800
#define REAR_TOF_DUAL_CAL_END_ADDR                  0xC0FB
#define REAR_TOF_DUAL_CAL_SIZE                      (REAR_TOF_DUAL_CAL_END_ADDR - REAR_TOF_DUAL_CAL_ADDR + 1)
#define REAR_TOF_DUAL_TILT_DLL_VERSION              (REAR_TOF_DUAL_CAL_ADDR + 0x0000)
#define REAR_TOF_DUAL_TILT_X                        (REAR_TOF_DUAL_CAL_ADDR + 0x006C)
#define REAR_TOF_DUAL_TILT_Y                        (REAR_TOF_DUAL_CAL_ADDR + 0x0070)
#define REAR_TOF_DUAL_TILT_Z                        (REAR_TOF_DUAL_CAL_ADDR + 0x0074)
#define REAR_TOF_DUAL_TILT_SX                       (REAR_TOF_DUAL_CAL_ADDR + 0x03C0)
#define REAR_TOF_DUAL_TILT_SY                       (REAR_TOF_DUAL_CAL_ADDR + 0x03C4)
#define REAR_TOF_DUAL_TILT_RANGE                    (REAR_TOF_DUAL_CAL_ADDR + 0x04E0)
#define REAR_TOF_DUAL_TILT_MAX_ERR                  (REAR_TOF_DUAL_CAL_ADDR + 0x04E4)
#define REAR_TOF_DUAL_TILT_AVG_ERR                  (REAR_TOF_DUAL_CAL_ADDR + 0x04E8)

#define REAR2_TOF_DUAL_CAL_ADDR                     0xB800
#define REAR2_TOF_DUAL_TILT_DLL_VERSION             (REAR2_TOF_DUAL_CAL_ADDR + 0x0000)
#define REAR2_TOF_DUAL_TILT_X                       (REAR2_TOF_DUAL_CAL_ADDR + 0x0160)
#define REAR2_TOF_DUAL_TILT_Y                       (REAR2_TOF_DUAL_CAL_ADDR + 0x0164)
#define REAR2_TOF_DUAL_TILT_Z                       (REAR2_TOF_DUAL_CAL_ADDR + 0x0168)
#define REAR2_TOF_DUAL_TILT_SX                      (REAR2_TOF_DUAL_CAL_ADDR + 0x05C8)
#define REAR2_TOF_DUAL_TILT_SY                      (REAR2_TOF_DUAL_CAL_ADDR + 0x05CC)
#define REAR2_TOF_DUAL_TILT_RANGE                   (REAR2_TOF_DUAL_CAL_ADDR + 0x06E8)
#define REAR2_TOF_DUAL_TILT_MAX_ERR                 (REAR2_TOF_DUAL_CAL_ADDR + 0x06EC)
#define REAR2_TOF_DUAL_TILT_AVG_ERR                 (REAR2_TOF_DUAL_CAL_ADDR + 0x06F0)

#if defined(CONFIG_SAMSUNG_FRONT_TOF)
#define FRONT_TOF_DUAL_CAL_ADDR                     0x2200
#define FRONT_TOF_DUAL_CAL_END_ADDR                 0x29FF
#define FRONT_TOF_DUAL_CAL_SIZE                     (FRONT_TOF_DUAL_CAL_END_ADDR - FRONT_TOF_DUAL_CAL_ADDR + 1)
#define FRONT_TOF_DUAL_TILT_DLL_VERSION             (FRONT_TOF_DUAL_CAL_ADDR + 0x07F4) // 29F4
#define FRONT_TOF_DUAL_TILT_X                       (FRONT_TOF_DUAL_CAL_ADDR + 0x04B8) // 26B8
#define FRONT_TOF_DUAL_TILT_Y                       (FRONT_TOF_DUAL_CAL_ADDR + 0x04BC) // 26BC
#define FRONT_TOF_DUAL_TILT_Z                       (FRONT_TOF_DUAL_CAL_ADDR + 0x04C0) // 26C0
#define FRONT_TOF_DUAL_TILT_SX                      (FRONT_TOF_DUAL_CAL_ADDR + 0x04DC) // 26DC
#define FRONT_TOF_DUAL_TILT_SY                      (FRONT_TOF_DUAL_CAL_ADDR + 0x04E0) // 26E0
#define FRONT_TOF_DUAL_TILT_RANGE                   (FRONT_TOF_DUAL_CAL_ADDR + 0x07EC) // 29EC
#define FRONT_TOF_DUAL_TILT_MAX_ERR                 (FRONT_TOF_DUAL_CAL_ADDR + 0x07E8) // 29E8
#define FRONT_TOF_DUAL_TILT_AVG_ERR                 (FRONT_TOF_DUAL_CAL_ADDR + 0x07E4) // 29E4
#endif
#endif

/*************************************************************************************************/
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
#define OIS_XYGG_SIZE                               8
#define OIS_CENTER_SHIFT_SIZE                       4
#define OIS_XYSR_SIZE                               4
#define OIS_CROSSTALK_SIZE                          4
#define OIS_CAL_MARK_START_OFFSET                   0x30
#define OIS_XYGG_START_OFFSET                       0x10
#define OIS_XYSR_START_OFFSET                       0x38
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
#define WIDE_OIS_CENTER_SHIFT_START_OFFSET          0x2AE
#define TELE_OIS_CENTER_SHIFT_START_OFFSET          0x2AA
#define TELE_OIS_CROSSTALK_START_OFFSET             0x1C
#endif
#endif

#define MAX_AF_CAL_STR_SIZE                         256

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
extern uint8_t rear3_dual_cal[FROM_REAR_DUAL_CAL_SIZE + 1];
//extern int rear3_af_cal[FROM_REAR_AF_CAL_SIZE + 1];
extern char rear3_af_cal_str[MAX_AF_CAL_STR_SIZE];
#endif
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
extern uint8_t rear2_dual_cal[FROM_REAR_DUAL_CAL_SIZE + 1];
#endif
//extern int rear_af_cal[FROM_REAR_AF_CAL_SIZE + 1];
extern char rear_af_cal_str[MAX_AF_CAL_STR_SIZE];
extern char rear_sensor_id[FROM_SENSOR_ID_SIZE + 1];
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
extern char rear3_sensor_id[FROM_SENSOR_ID_SIZE + 1];
extern DualTilt_t rear3_dual;
#endif

extern char rear_paf_cal_data_far[PAF_2PD_CAL_INFO_SIZE];
extern char rear_paf_cal_data_mid[PAF_2PD_CAL_INFO_SIZE];
extern uint32_t paf_err_data_result;
extern char rear_f2_paf_cal_data_far[PAF_2PD_CAL_INFO_SIZE];
extern char rear_f2_paf_cal_data_mid[PAF_2PD_CAL_INFO_SIZE];
extern uint32_t f2_paf_err_data_result;

extern uint8_t rear_module_id[FROM_MODULE_ID_SIZE + 1];
extern char rear_mtf_exif[FROM_MTF_SIZE + 1];
extern char rear_mtf2_exif[FROM_MTF_SIZE + 1];
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE) || defined(CONFIG_SAMSUNG_REAR_BOKEH)
extern char rear3_mtf_exif[FROM_MTF_SIZE + 1];
extern char cam3_fw_ver[SYSFS_FW_VER_SIZE];
extern char cam3_fw_full_ver[SYSFS_FW_VER_SIZE];
extern uint8_t rear3_module_id[FROM_MODULE_ID_SIZE + 1];
extern char module3_info[SYSFS_MODULE_INFO_SIZE];
#endif

extern char cam_fw_ver[SYSFS_FW_VER_SIZE];
extern char cam_fw_full_ver[SYSFS_FW_VER_SIZE];
extern char cam_fw_factory_ver[SYSFS_FW_VER_SIZE];
extern char cam_fw_user_ver[SYSFS_FW_VER_SIZE];
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT) || defined(CONFIG_SEC_R8Q_PROJECT)
extern char cam3_fw_factory_ver[SYSFS_FW_VER_SIZE];
extern char cam3_fw_user_ver[SYSFS_FW_VER_SIZE];
#endif
extern char cal_crc[SYSFS_FW_VER_SIZE];
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
extern char module2_info[SYSFS_MODULE_INFO_SIZE];
extern char rear2_sensor_id[FROM_SENSOR_ID_SIZE + 1];
extern uint8_t rear2_module_id[FROM_MODULE_ID_SIZE + 1];
extern char rear2_mtf_exif[FROM_MTF_SIZE + 1];
extern char cam2_fw_ver[SYSFS_FW_VER_SIZE];
extern char cam2_fw_full_ver[SYSFS_FW_VER_SIZE];
extern char cam2_fw_factory_ver[SYSFS_FW_VER_SIZE];
extern char cam2_fw_user_ver[SYSFS_FW_VER_SIZE];

extern DualTilt_t rear2_dual;

extern uint32_t rear3_paf_err_data_result;
#endif

extern uint32_t front_paf_err_data_result;

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
extern uint8_t front2_dual_cal[FROM_FRONT_DUAL_CAL_SIZE + 1];
extern DualTilt_t front2_dual;
#endif
extern char module_info[SYSFS_MODULE_INFO_SIZE];

/* phone fw info */
extern uint32_t CAMERA_NORMAL_CAL_CRC;

#if !defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM)
extern uint32_t front_af_cal_pan;
extern uint32_t front_af_cal_macro;
#endif

extern char front_sensor_id[FROM_SENSOR_ID_SIZE + 1];
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
extern char front2_sensor_id[FROM_SENSOR_ID_SIZE + 1];
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
extern char front3_sensor_id[FROM_SENSOR_ID_SIZE + 1];
#else
extern char front2_sensor_id[FROM_SENSOR_ID_SIZE + 1];
#endif
#endif
extern uint8_t front_module_id[FROM_MODULE_ID_SIZE + 1];
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
extern uint8_t front2_module_id[FROM_MODULE_ID_SIZE + 1];
#endif
extern char front_mtf_exif[FROM_MTF_SIZE + 1];
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
extern uint8_t front3_module_id[FROM_MODULE_ID_SIZE + 1];
#else
extern uint8_t front2_module_id[FROM_MODULE_ID_SIZE + 1];
#endif
#endif

extern char front_cam_fw_ver[SYSFS_FW_VER_SIZE];
extern char front_cam_fw_full_ver[SYSFS_FW_VER_SIZE];
extern char front_cam_fw_user_ver[SYSFS_FW_VER_SIZE];
extern char front_cam_fw_factory_ver[SYSFS_FW_VER_SIZE];

extern char front_module_info[SYSFS_MODULE_INFO_SIZE];
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
extern char front2_module_info[SYSFS_MODULE_INFO_SIZE];
extern char front2_cam_fw_ver[SYSFS_FW_VER_SIZE];
extern char front2_cam_fw_full_ver[SYSFS_FW_VER_SIZE];
extern char front2_cam_fw_user_ver[SYSFS_FW_VER_SIZE];
extern char front2_cam_fw_factory_ver[SYSFS_FW_VER_SIZE];
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
extern char front3_module_info[SYSFS_MODULE_INFO_SIZE];
extern char front3_cam_fw_ver[SYSFS_FW_VER_SIZE];
extern char front3_cam_fw_full_ver[SYSFS_FW_VER_SIZE];
extern char front3_cam_fw_user_ver[SYSFS_FW_VER_SIZE];
extern char front3_cam_fw_factory_ver[SYSFS_FW_VER_SIZE];
#else
extern char front2_module_info[SYSFS_MODULE_INFO_SIZE];
extern char front2_cam_fw_ver[SYSFS_FW_VER_SIZE];
extern char front2_cam_fw_full_ver[SYSFS_FW_VER_SIZE];
extern char front2_cam_fw_user_ver[SYSFS_FW_VER_SIZE];
extern char front2_cam_fw_factory_ver[SYSFS_FW_VER_SIZE];
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_TOF)
extern char cam_tof_fw_ver[SYSFS_FW_VER_SIZE];
extern char cam_tof_fw_full_ver[SYSFS_FW_VER_SIZE];
extern char cam_tof_fw_user_ver[SYSFS_FW_VER_SIZE];
extern char cam_tof_fw_factory_ver[SYSFS_FW_VER_SIZE];
extern char rear_tof_module_info[SYSFS_MODULE_INFO_SIZE];
extern char rear_tof_sensor_id[FROM_SENSOR_ID_SIZE + 1];
extern uint8_t rear_tof_module_id[FROM_MODULE_ID_SIZE + 1];

extern int rear_tof_uid;
extern int rear_tof_validation_500;
extern int rear_tof_validation_300;
extern uint8_t rear_tof_cal[TOFCAL_SIZE + 1];
extern uint8_t rear_tof_cal_extra[TOFCAL_EXTRA_SIZE + 1];
extern uint8_t rear_tof_cal_result;

extern uint8_t rear_tof_dual_cal[REAR_TOF_DUAL_CAL_SIZE + 1];
extern DualTilt_t rear_tof_dual;
extern DualTilt_t rear2_tof_dual;
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
extern char front_tof_cam_fw_ver[SYSFS_FW_VER_SIZE];
extern char front_tof_cam_fw_full_ver[SYSFS_FW_VER_SIZE];
extern char front_tof_cam_fw_user_ver[SYSFS_FW_VER_SIZE];
extern char front_tof_cam_fw_factory_ver[SYSFS_FW_VER_SIZE];
extern char front_tof_module_info[SYSFS_MODULE_INFO_SIZE];
extern char front_tof_sensor_id[FROM_SENSOR_ID_SIZE + 1];
extern uint8_t front2_module_id[FROM_MODULE_ID_SIZE + 1];

extern int front_tof_uid;
extern uint8_t front_tof_cal[TOFCAL_SIZE + 1];
extern uint8_t front_tof_cal_extra[TOFCAL_EXTRA_SIZE+1];
extern uint8_t front_tof_cal_result;

extern uint8_t front_tof_dual_cal[FRONT_TOF_DUAL_CAL_SIZE + 1];
extern DualTilt_t front_tof_dual;
#endif

enum cam_eeprom_state {
	CAM_EEPROM_INIT,
	CAM_EEPROM_ACQUIRE,
	CAM_EEPROM_CONFIG,
};

/**
 * struct cam_eeprom_map_t - eeprom map
 * @data_type       :   Data type
 * @addr_type       :   Address type
 * @addr            :   Address
 * @data            :   data
 * @delay           :   Delay
 *
 */
struct cam_eeprom_map_t {
	uint32_t valid_size;
	uint32_t addr;
	uint32_t addr_type;
	uint32_t data;
	uint32_t data_type;
	uint32_t delay;
};

/**
 * struct cam_eeprom_memory_map_t - eeprom memory map types
 * @page            :   page memory
 * @pageen          :   pageen memory
 * @poll            :   poll memory
 * @mem             :   mem
 * @saddr           :   slave addr
 *
 */
struct cam_eeprom_memory_map_t {
	struct cam_eeprom_map_t page;
	struct cam_eeprom_map_t pageen;
	struct cam_eeprom_map_t poll;
	struct cam_eeprom_map_t mem;
	uint32_t saddr;
};

/**
 * struct cam_eeprom_memory_block_t - eeprom mem block info
 * @map             :   eeprom memory map
 * @num_map         :   number of map blocks
 * @mapdata         :   map data
 * @cmd_type        :   size of total mapdata
 *
 */
struct cam_eeprom_memory_block_t {
	struct cam_eeprom_memory_map_t *map;
	uint32_t num_map;
	uint8_t *mapdata;
	uint32_t num_data;
	uint32_t is_supported;
};

/**
 * struct cam_eeprom_cmm_t - camera multimodule
 * @cmm_support     :   cmm support flag
 * @cmm_compression :   cmm compression flag
 * @cmm_offset      :   cmm data start offset
 * @cmm_size        :   cmm data size
 *
 */
struct cam_eeprom_cmm_t {
	uint32_t cmm_support;
	uint32_t cmm_compression;
	uint32_t cmm_offset;
	uint32_t cmm_size;
};

/**
 * struct cam_eeprom_i2c_info_t - I2C info
 * @slave_addr      :   slave address
 * @i2c_freq_mode   :   i2c frequency mode
 *
 */
struct cam_eeprom_i2c_info_t {
	uint16_t slave_addr;
	uint8_t i2c_freq_mode;
};

/**
 * struct cam_eeprom_soc_private - eeprom soc private data structure
 * @eeprom_name     :   eeprom name
 * @i2c_info        :   i2c info structure
 * @power_info      :   eeprom power info
 * @cmm_data        :   cmm data
 *
 */
struct cam_eeprom_soc_private {
	const char *eeprom_name;
	struct cam_eeprom_i2c_info_t i2c_info;
	struct cam_sensor_power_ctrl_t power_info;
	struct cam_eeprom_cmm_t cmm_data;
};

/**
 * struct cam_eeprom_intf_params - bridge interface params
 * @device_hdl   : Device Handle
 * @session_hdl  : Session Handle
 * @ops          : KMD operations
 * @crm_cb       : Callback API pointers
 */
struct cam_eeprom_intf_params {
	int32_t device_hdl;
	int32_t session_hdl;
	int32_t link_hdl;
	struct cam_req_mgr_kmd_ops ops;
	struct cam_req_mgr_crm_cb *crm_cb;
};

struct eebin_info {
	uint32_t start_address;
	uint32_t size;
	uint32_t is_valid;
};

/**
 * struct cam_eeprom_ctrl_t - EEPROM control structure
 * @device_name         :   Device name
 * @pdev                :   platform device
 * @spi                 :   spi device
 * @eeprom_mutex        :   eeprom mutex
 * @soc_info            :   eeprom soc related info
 * @io_master_info      :   Information about the communication master
 * @gpio_num_info       :   gpio info
 * @cci_i2c_master      :   I2C structure
 * @v4l2_dev_str        :   V4L2 device structure
 * @bridge_intf         :   bridge interface params
 * @cam_eeprom_state    :   eeprom_device_state
 * @userspace_probe     :   flag indicates userspace or kernel probe
 * @cal_data            :   Calibration data
 * @device_name         :   Device name
 * @is_multimodule_mode :   To identify multimodule node
 * @wr_settings         :   I2C write settings
 * @eebin_info          :   EEBIN address, size info
 */
struct cam_eeprom_ctrl_t {
	char device_name[CAM_CTX_DEV_NAME_MAX_LENGTH];
	struct platform_device *pdev;
	struct spi_device *spi;
	struct mutex eeprom_mutex;
	struct cam_hw_soc_info soc_info;
	struct camera_io_master io_master_info;
	struct msm_camera_gpio_num_info *gpio_num_info;
	enum cci_i2c_master_t cci_i2c_master;
	enum cci_device_num cci_num;
	struct cam_subdev v4l2_dev_str;
	struct cam_eeprom_intf_params bridge_intf;
	enum msm_camera_device_type_t eeprom_device_type;
	enum cam_eeprom_state cam_eeprom_state;
	bool userspace_probe;
	struct cam_eeprom_memory_block_t cal_data;
	uint16_t is_multimodule_mode;
	struct i2c_settings_array wr_settings;
	struct eebin_info eebin_info;
	uint32_t is_supported;
};

typedef enum{
	EEPROM_FW_VER = 1,
	PHONE_FW_VER,
	LOAD_FW_VER
} cam_eeprom_fw_version_idx;

int32_t cam_eeprom_update_i2c_info(struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_eeprom_i2c_info_t *i2c_info);

#endif /*_CAM_EEPROM_DEV_H_ */
