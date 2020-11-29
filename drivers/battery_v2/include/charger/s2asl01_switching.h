/*
 * s2asl01_switching.h
 * Samsung S2ASL01 Fuel Gauge Header
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
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

#ifndef __S2ASL01_SWITCHING_H
#define __S2ASL01_SWITCHING_H __FILE__

#include <linux/power_supply.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include "include/sec_charging_common.h"

#define I2C_ADDR_LIMITER_MAIN		(0x70 >> 1)
#define I2C_ADDR_LIMITER_SUB		(0x72 >> 1)

#define S2ASL01_SWITCHING_CORE_INT1		0x00
#define S2ASL01_SWITCHING_CORE_INT2		0x01
#define S2ASL01_SWITCHING_PM_INT		0x02
#define S2ASL01_SWITCHING_CORE_INT1M		0x03
#define S2ASL01_SWITCHING_CORE_INT2M		0x04
#define S2ASL01_SWITCHING_PM_INTM		0x05
#define S2ASL01_SWITCHING_CORE_STATUS1		0x06
#define S2ASL01_SWITCHING_VAL1_VCHG		0x07
#define S2ASL01_SWITCHING_VAL2_VCHG		0x08
#define S2ASL01_SWITCHING_VAL1_VBAT		0x09
#define S2ASL01_SWITCHING_VAL2_VBAT		0x0A
#define S2ASL01_SWITCHING_VAL1_ICHG		0x0B
#define S2ASL01_SWITCHING_VAL2_ICHG		0x0C
#define S2ASL01_SWITCHING_VAL1_IDISCHG		0x0D
#define S2ASL01_SWITCHING_VAL2_IDISCHG		0x0E
#define S2ASL01_SWITCHING_CORE_CTRL1		0x0F
#define S2ASL01_SWITCHING_CORE_CTRL2		0x10
#define S2ASL01_SWITCHING_CORE_CTRL3		0x11
#define S2ASL01_SWITCHING_CORE_CTRL4		0x12
#define S2ASL01_SWITCHING_CORE_CTRL5		0x13
#define S2ASL01_SWITCHING_CORE_CTRL6		0x14
#define S2ASL01_SWITCHING_CORE_CTRL7		0x15
#define S2ASL01_SWITCHING_CORE_CTRL8		0x16
#define S2ASL01_SWITCHING_TOP_RECHG_CTRL1	0x17
#define S2ASL01_SWITCHING_TOP_EOC_CTRL1		0x18
#define S2ASL01_SWITCHING_TOP_EOC_CTRL2		0x19
#define S2ASL01_SWITCHING_COMASK_CALDIS		0x1A // ?
#define S2ASL01_SWITCHING_PM_ENABLE			0x1B
#define S2ASL01_SWITCHING_PM_HYST_LEVEL1	0x1C // ?
#define S2ASL01_SWITCHING_PM_HYST_LEVEL2	0x1D
#define S2ASL01_SWITCHING_PM_V_OPTION		0x1E
#define S2ASL01_SWITCHING_PM_I_OPTION		0x1F
#define S2ASL01_SWITCHING_ID			0x29
#define S2ASL01_SWITCHING_COMMON1		0x40

#define MASK(width, shift)	(((0x1 << (width)) - 1) << shift)

#define S2ASL01_SW_CORE1_INT_MASK	0x0F
#define S2ASL01_SW_CORE2_INT_MASK	0x03
#define S2ASL01_SW_PM_INT_MASK		0xFE

#define S2ASL01_PM_INT_VCHGI_MASK		(0x01 << 7)
#define S2ASL01_PM_INT_BATI_MASK		(0x01 << 6)
#define S2ASL01_PM_INT_ICHGI_MASK		(0x01 << 5)
#define S2ASL01_PM_INT_IDISCHGI_MASK	(0x01 << 4)
#define S2ASL01_PM_INT_PRREQ_DONE_MASK	(0x01 << 3)
#define S2ASL01_PM_INT_CO_REQ_DONE_MASK	(0x01 << 2)
#define S2ASL01_PM_INT_ADCEOC_ERR_MASK	(0x01 << 1)

#define S2ASL01_INT1_FCC_TRICLE_MASK	(0x01 << 3)
#define S2ASL01_INT1_TRICLE_FCC_MASK	(0x01 << 2)
#define S2ASL01_INT1_TRICKLE_PRE_MASK	(0x01 << 1)
#define S2ASL01_INT1_PRE_TRICKLE_MASK	(0x01 << 0)

#define S2ASL01_INT2_RESTART_IM_MASK	(0x01 << 1)
#define S2ASL01_INT2_EOC_IM_MASK		(0x01 << 0)

#define S2ASL01_SUB_PWR_OFF_MODE1		0x01
#define S2ASL01_SUB_PWR_OFF_MODE2		0x00
#define S2ASL01_SUB_PWR_OFF_MODE_MASK	0x01

#define S2ASL01_CTRL1_RESTART_SHIFT	1
#define S2ASL01_CTRL1_RESTART_MASK	BIT(S2ASL01_CTRL1_RESTART_SHIFT)
#define S2ASL01_CTRL1_EOC_SHIFT		0
#define S2ASL01_CTRL1_EOC_MASK		BIT(S2ASL01_CTRL1_EOC_SHIFT)

#define S2ASL01_CTRL3_INOK_SHIFT	7
#define S2ASL01_CTRL3_INOK_MASK		BIT(S2ASL01_CTRL3_INOK_SHIFT)
#define S2ASL01_CTRL3_SUPLLEMENTMODE_SHIFT	6
#define S2ASL01_CTRL3_SUPLLEMENTMODE_MASK	BIT(S2ASL01_CTRL3_SUPLLEMENTMODE_SHIFT)

#define S2ASL01_CTRL3_DIS_MODE_SHIFT	2
#define S2ASL01_CTRL3_DIS_MODE_MASK	0x0C
#define S2ASL01_CTRL3_CHG_MODE_SHIFT	0
#define S2ASL01_CTRL3_CHG_MODE_MASK	0x03

#define FCC_CHG_CURRENTLIMIT_MASK		0x3F
#define TRICKLE_CHG_CURRENT_LIMIT_MASK		0x0F
#define DISCHG_CURRENT_LIMIT_MASK		0x7F

#define S2ASL01_COMMON1_CM_TSD_EN	0x40

enum current_limiter_type{
	LIMITER_MAIN = 0x1,
	LIMITER_SUB = 0x2,
};

char *current_limiter_type_str[] = {
	"NONE",
	"MAIN LIMITER",
	"SUB LIMITER",
};

enum s2asl01_switching_chg_mode{
	CURRENT_REGULATION = 0,
	CURRENT_VOLTAGE_REGULATION,
	CURRENT_SMARTER_VOLTAGE_REGULATION,
	NO_REGULATION_FULLY_ON,
};

//enum power_supply_ext_property {

//};

enum {
	LIMITER_DATA,
};

ssize_t s2asl01_limiter_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t s2asl01_limiter_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define S2ASL01_LIMITER_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0660},	\
	.show = s2asl01_limiter_show_attrs,			\
	.store = s2asl01_limiter_store_attrs,			\
}

struct s2asl01_platform_data {
	char *switching_name;
	int bat_type;
	int bat_int;
	int bat_enb;
	int chg_current_limit;
	int eoc; /* for interrupt setting, not used */
	int float_voltage; /* for interrupt setting, not used */
	int hys_vchg_level;
	int hys_vbat_level;
	int hys_ichg_level;
	int hys_idischg_level;
	bool tsd_en;
};

struct s2asl01_switching_data {
	struct device           *dev;
	struct i2c_client       *client;
	struct mutex            i2c_lock;
	struct s2asl01_platform_data *pdata;
	struct power_supply     *psy_sw;
	struct workqueue_struct *wqueue;	
	struct delayed_work	limiter_isr_work;
	int rev_id;
	int es_num;
	int irq_bat_int;
	bool in_ok;
	bool supllement_mode;
	bool power_meter;
	bool tsd;
	/* debug I2C */
	u8 addr;
	u8 data;
};
#endif /* __S2ASL01_SWITCHING_H */
