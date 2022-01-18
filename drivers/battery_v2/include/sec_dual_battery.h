/*
 * sec_multi_charger.h
 * Samsung Mobile Charger Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SEC_DUAL_BATTERY_H
#define __SEC_DUAL_BATTERY_H __FILE__

#include "sec_charging_common.h"

enum sec_dual_battery_chg_mode {
	SEC_DUAL_BATTERY_NONE = 0, 
	SEC_DUAL_BATTERY_MAIN_CONDITION_DONE,
	SEC_DUAL_BATTERY_SUB_CONDITION_DONE,
};

struct sec_dual_battery_platform_data {
	char *main_limiter_name;
	char *sub_limiter_name;

	int *main_full_condition_vcell;
	int *sub_full_condition_vcell;

	int main_full_condition_eoc;
	int sub_full_condition_eoc;

	int main_bat_con_det_gpio;
	int sub_bat_con_det_gpio;
};

struct sec_dual_battery_info {
	struct device *dev;
	struct sec_dual_battery_platform_data *pdata;
	struct power_supply		*psy_bat;

	int cable_type;
	int status;

	int multi_mode;

	int main_voltage_now;		/* cell voltage (mV) */
	int sub_voltage_now;		/* cell voltage (mV) */	
	int main_voltage_avg;		/* average voltage (mV) */
	int sub_voltage_avg;		/* average voltage (mV) */	
	int main_current_now;		/* current (mA) */
	int sub_current_now;		/* current (mA) */	
	int main_current_avg;		/* average current (mA) */
	int sub_current_avg;		/* average current (mA) */	
	int main_discurrent_now;		/* current (mA) */
	int sub_discurrent_now;		/* current (mA) */	
	int main_discurrent_avg;		/* average current (mA) */
	int sub_discurrent_avg;		/* average current (mA) */	

	int full_voltage_status;
	int full_current_status;
	int full_total_status;

	int chg_mode;

	unsigned long force_full_time;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	int age_step;
#endif
};

#endif /* __SEC_DUAL_BATTERY_H */
