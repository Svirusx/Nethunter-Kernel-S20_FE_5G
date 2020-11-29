/*
 *
 * Zinitix ZT touch driver
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_ZT_TS_H
#define _LINUX_ZT_TS_H

#define TS_DRVIER_VERSION	"1.0.18_1"

#define ZT_TS_DEVICE		"zt_ts_device"

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
#include <linux/input/sec_secure_touch.h>
#endif

#ifdef CONFIG_INPUT_TOUCHSCREEN_TCLMV2
#define TCLM_CONCEPT
#endif

/* TCLM_CONCEPT */
#define ZT_TS_NVM_OFFSET_FAC_RESULT			0
#define ZT_TS_NVM_OFFSET_DISASSEMBLE_COUNT		2

#define ZT_TS_NVM_OFFSET_CAL_COUNT			4
#define ZT_TS_NVM_OFFSET_TUNE_VERSION			5
#define ZT_TS_NVM_OFFSET_CAL_POSITION			7
#define ZT_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT		8
#define ZT_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP		9
#define ZT_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO		10
#define ZT_TS_NVM_OFFSET_HISTORY_QUEUE_SIZE		20

#define ZT_TS_NVM_OFFSET_LENGTH		(ZT_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO + ZT_TS_NVM_OFFSET_HISTORY_QUEUE_SIZE)

#ifdef CONFIG_SEC_DEBUG_TSP_LOG
#include <linux/sec_debug.h>
#endif

/* sponge enable feature */
#define ZT_SPONGE_MODE_SPAY				1
#define ZT_SPONGE_MODE_AOD				2
#define ZT_SPONGE_MODE_SINGLETAP		3
#define ZT_SPONGE_MODE_PRESS			4
#define ZT_SPONGE_MODE_DOUBLETAP_WAKEUP	5

#define ZT_SPONGE_LP_FEATURE		0x0000
#define ZT_SPONGE_TOUCHBOX_W_OFFSET 0x0002
#define ZT_SPONGE_TOUCHBOX_H_OFFSET 0x0004
#define ZT_SPONGE_TOUCHBOX_X_OFFSET 0x0006
#define ZT_SPONGE_TOUCHBOX_Y_OFFSET 0x0008
#define ZT_SPONGE_AOD_ACTIVE_INFO	0x000A

#define ZT_SPONGE_UTC				0x0010
#define ZT_SPONGE_FOD_PROPERTY		0x0014
#define ZT_SPONGE_FOD_INFO			0x0015
#define ZT_SPONGE_FOD_POSITION		0x0019
#define ZT_SPONGE_FOD_RECT			0x004B



#define ZT_SPONGE_DUMP_FORMAT		0x00F0
#define ZT_SPONGE_DUMP_CNT			0x00F1
#define ZT_SPONGE_DUMP_CUR_IDX		0x00F2
#define ZT_SPONGE_DUMP_START		0x00F4

#define ZT_SPONGE_READ_INFO			0xA401
#define ZT_SPONGE_READ_REG			0xA402
#define ZT_SPONGE_WRITE_REG			0xA403
#define ZT_SPONGE_SYNC_REG			0xA404

struct zt_ts_platform_data {
	u32 irq_gpio;
	u32 gpio_int;
	u32 gpio_scl;
	u32 gpio_sda;
	u32 gpio_ldo_en;
	int (*tsp_power)(void *data, bool on);
	u16 x_resolution;
	u16 y_resolution;
	u8 area_indicator;
	u8 area_navigation;
	u8 area_edge;
	u16 page_size;
	u8 orientation;
	bool support_spay;
	bool support_aod;
	bool support_aot;
	bool support_ear_detect;
	int bringup;
	bool mis_cal_check;
	bool support_dex;
	u16 pat_function;
	u16 afe_base;
	void (*register_cb)(void *);

	const char *regulator_dvdd;
	const char *regulator_avdd;
	const char *firmware_name;
	const char *chip_name;
	struct pinctrl *pinctrl;
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	int ss_touch_num;
#endif
};

union coord_byte00_t
{
	u8 value_u8bit;
	struct {
		u8 eid : 2;
		u8 tid : 4;
		u8 touch_status : 2;
	} value;
};

union coord_byte01_t
{
	u8 value_u8bit;
	struct {
		u8 x_coord_h : 8;
	} value;
};

union coord_byte02_t
{
	u8 value_u8bit;
	struct {
		u8 y_coord_h : 8;
	} value;
};

union coord_byte03_t
{
	u8 value_u8bit;
	struct {
		u8 y_coord_l : 4;
		u8 x_coord_l : 4;
	} value;
};

union coord_byte04_t
{
	u8 value_u8bit;
	struct {
		u8 major : 8;
	} value;
};

union coord_byte05_t
{
	u8 value_u8bit;
	struct {
		u8 minor : 8;
	} value;
};

union coord_byte06_t
{
	u8 value_u8bit;
	struct {
		u8 z_value : 6;
		u8 touch_type23 : 2;
	} value;
};

union coord_byte07_t
{
	u8 value_u8bit;
	struct {
		u8 left_event : 4;
		u8 max_energy : 2;
		u8 touch_type01 : 2;
	} value;
};

union coord_byte08_t
{
	u8 value_u8bit;
	struct {
		u8 noise_level : 8;
	} value;
};

union coord_byte09_t
{
	u8 value_u8bit;
	struct {
		u8 max_sensitivity : 8;
	} value;
};

union coord_byte10_t
{
	u8 value_u8bit;
	struct {
		u8 hover_id_num  : 4;
		u8 location_area : 4;
	} value;
};

struct point_info {
	union coord_byte00_t byte00;
	union coord_byte01_t byte01;
	union coord_byte02_t byte02;
	union coord_byte03_t byte03;
	union coord_byte04_t byte04;
	union coord_byte05_t byte05;
	union coord_byte06_t byte06;
	union coord_byte07_t byte07;
	union coord_byte08_t byte08;
	union coord_byte09_t byte09;
	union coord_byte10_t byte10;
	u8 byte11;
	u8 byte12;
	u8 byte13;
	u8 byte14;
	u8 byte15;
};

struct ts_coordinate {
	u8 id;
	u8 ttype;
	u8 touch_status;
	u16 x;
	u16 y;
	u8 z;
	u8 hover_flag;
	u8 glove_flag;
	u8 touch_height;
	u16 mcount;
	u8 major;
	u8 minor;
	u8 noise;
	u8 max_sense;
	bool palm;
	int palm_count;
	u8 left_event;
};


typedef enum {
	COORDINATE_EVENT = 0,
	STATUS_EVENT,
	GESTURE_EVENT,
	CUSTOM_EVENT,
} EVENT_ID;

typedef enum {
	FINGER_NONE = 0,
	FINGER_PRESS,
	FINGER_MOVE,
	FINGER_RELEASE,
} TOUCH_STATUS_TYPE;


typedef enum {
	TOUCH_NOTYPE = -1,
	TOUCH_NORMAL,
	TOUCH_HOVER,
	TOUCH_FLIP_COVER,
	TOUCH_GLOVE,
	TOUCH_STYLUS,
	TOUCH_PALM,
	TOUCH_WET,
	TOUCH_PROXIMITY,
	TOUCH_JIG,
} TOUCH_TYPE;

typedef enum {
	SWIPE_UP = 0,
	DOUBLE_TAP,
	PRESSURE,
	FINGERPRINT,
	SINGLE_TAP,
	GESTURE_NONE = 255,
} GESTURE_TYPE;

typedef union _nvm_binary_info
{
	u32 buff32[32]; //128byte
	struct _val
	{
		u32 RESERVED0;
		u32 auto_boot_flag1;
		u32 auto_boot_flag2;
		u32 slave_addr_flag;
		u32 RESERVED1[4];
		u32 info_checksum;
		u32 core_checksum;
		u32 custom_checksum;
		u32 register_checksum;
		u16 hw_id;
		u16 RESERVED2;
		u16 major_ver;
		u16 RESERVED3;
		u16 minor_ver;
		u16 RESERVED4;
		u16 release_ver;
		u16 RESERVED5;
		u16 chip_id;
		u16 chip_id_reverse;
		u16 chip_naming_number;
		u16 RESERVED6[9];
		u8  info_size[4];
		u8  core_size[4];
		u8  custom_size[4];
		u8  register_size[4];
		u8  total_size[4];
		u32 RESERVED7[5];
	}val;
}nvm_binary_info;


extern struct class *sec_class;

void tsp_charger_infom(bool en);

#endif /* LINUX_ZT_TS_H */
