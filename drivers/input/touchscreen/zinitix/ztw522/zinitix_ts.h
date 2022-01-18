/*
 *
 * Zinitix zt75xx touch driver
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

#ifndef _LINUX_ZT75XX_TS_H
#define _LINUX_ZT75XX_TS_H

#define TS_DRVIER_VERSION	"1.0.18_1"

#define ZT75XX_TS_DEVICE	"zt75xx_ts"

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
#include <linux/input/sec_secure_touch.h>
#endif

#ifdef CONFIG_INPUT_TOUCHSCREEN_TCLMV2
//#define TCLM_CONCEPT
#endif

/* TCLM_CONCEPT */
#define ZT75XX_TS_NVM_OFFSET_FAC_RESULT			0
#define ZT75XX_TS_NVM_OFFSET_DISASSEMBLE_COUNT		2

#define ZT75XX_TS_NVM_OFFSET_CAL_COUNT			4
#define ZT75XX_TS_NVM_OFFSET_TUNE_VERSION		5
#define ZT75XX_TS_NVM_OFFSET_CAL_POSITION		7
#define ZT75XX_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT	8
#define ZT75XX_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP	9
#define ZT75XX_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO		10
#define ZT75XX_TS_NVM_OFFSET_HISTORY_QUEUE_SIZE		20

#define ZT75XX_TS_NVM_OFFSET_LENGTH		(ZT75XX_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO + ZT75XX_TS_NVM_OFFSET_HISTORY_QUEUE_SIZE)

enum switch_system_mode {
	TO_TOUCH_MODE			= 0,
	TO_LOWPOWER_MODE		= 1,
};

struct zt75xx_ts_platform_data {
	u32 irq_gpio;
	u32 gpio_int;
	u32 gpio_scl;
	u32 gpio_sda;
	u32 gpio_ldo_en;
	int gpio_vdd;
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
	bool support_lpm_mode;
	bool bringup;
	bool mis_cal_check;
	bool support_hall_ic;
	bool use_deep_sleep;
	u16 pat_function;
	u16 afe_base;
	const char *project_name;
	void (*register_cb)(void *);

	const char *regulator_dvdd;
	const char *regulator_avdd;
	const char *regulator_tkled;
	const char *firmware_name;
	const char *chip_name;
	struct pinctrl *pinctrl;
	int item_version;
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	int ss_touch_num;
#endif
};

extern struct class *sec_class;

void tsp_charger_infom(bool en);

#endif /* LINUX_ZT75XX_TS_H */
