/*
 * max77705.h - Driver for the Maxim 77705
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAX77705_H__
#define __MAX77705_H__
#include <linux/platform_device.h>
#include <linux/regmap.h>

#define MFD_DEV_NAME "max77705"
#define M2SH(m) ((m) & 0x0F ? ((m) & 0x03 ? ((m) & 0x01 ? 0 : 1) : ((m) & 0x04 ? 2 : 3)) : \
		((m) & 0x30 ? ((m) & 0x10 ? 4 : 5) : ((m) & 0x40 ? 6 : 7)))

#if defined(CONFIG_MOTOR_DRV_MAX77705)
struct max77705_haptic_pdata {
	int mode;
	int divisor;
	int irq;
};
#endif

struct max77705_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct max77705_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	bool wakeup;
	bool blocking_waterevent;
	bool extra_fw_enable;
	int wpc_en;
	struct muic_platform_data *muic_pdata;

	int num_regulators;
	struct max77705_regulator_data *regulators;
#if defined(CONFIG_MOTOR_DRV_MAX77705)
	struct max77705_haptic_pdata *haptic_data;
#endif
	struct mfd_cell *sub_devices;
	int num_subdevs;
	bool support_audio;
	char *wireless_charger_name;
};

struct max77705 {
	struct regmap *regmap;
};

#endif /* __MAX77705_H__ */

