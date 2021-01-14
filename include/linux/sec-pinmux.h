/* Copyright (c) 2010-2011,2013-2014, The Linux Foundation. All rights reserved.
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
#ifndef __LINUX_PINCTRL_SEC_PINMUX_H
#define __LINUX_PINCTRL_SEC_PINMUX_H

#include <linux/bitops.h>
#include <linux/gpio.h>

#define dir_to_inout_val(dir)	(dir << 1)
#define GPIOMUX_DRV_SHFT		6
#define GPIOMUX_DRV_MASK		0x7
#define GPIOMUX_PULL_SHFT		0
#define GPIOMUX_PULL_MASK		0x3
#define GPIOMUX_DIR_SHFT		9
#define GPIOMUX_DIR_MASK		1
#define GPIOMUX_FUNC_SHFT		2
#define GPIOMUX_FUNC_MASK		0xF
#define GPIO_OUT_BIT			1
#define GPIO_IN_BIT			0
#define GPIO_OE_BIT			9

enum msm_gpiomux_setting {
	GPIOMUX_ACTIVE = 0,
	GPIOMUX_SUSPENDED,
	GPIOMUX_NSETTINGS
};

enum gpiomux_drv {
	GPIOMUX_DRV_2MA = 0,
	GPIOMUX_DRV_4MA,
	GPIOMUX_DRV_6MA,
	GPIOMUX_DRV_8MA,
	GPIOMUX_DRV_10MA,
	GPIOMUX_DRV_12MA,
	GPIOMUX_DRV_14MA,
	GPIOMUX_DRV_16MA,
};

enum gpiomux_func {
	GPIOMUX_FUNC_GPIO = 0,
	GPIOMUX_FUNC_1,
	GPIOMUX_FUNC_2,
	GPIOMUX_FUNC_3,
	GPIOMUX_FUNC_4,
	GPIOMUX_FUNC_5,
	GPIOMUX_FUNC_6,
	GPIOMUX_FUNC_7,
	GPIOMUX_FUNC_8,
	GPIOMUX_FUNC_9,
	GPIOMUX_FUNC_A,
	GPIOMUX_FUNC_B,
	GPIOMUX_FUNC_C,
	GPIOMUX_FUNC_D,
	GPIOMUX_FUNC_E,
	GPIOMUX_FUNC_F,
};

enum gpiomux_pull {
	GPIOMUX_PULL_NONE = 0,
	GPIOMUX_PULL_DOWN,
	GPIOMUX_PULL_KEEPER,
	GPIOMUX_PULL_UP,
};

enum gpio_cfg_set {
	GPIO_DVS_CFG_PULL_NONE = 0,
	GPIO_DVS_CFG_PULL_DOWN,
	GPIO_DVS_CFG_PULL_UP,
	GPIO_DVS_CFG_OUTPUT,
	GPIO_DVS_CFG_ERROR,
};

/* Direction settings are only meaningful when GPIOMUX_FUNC_GPIO is selected.
 * This element is ignored for all other FUNC selections, as the output-
 * enable pin is not under software control in those cases.  See the SWI
 * for your target for more details.
 */
enum gpiomux_dir {
	GPIOMUX_IN = 0,
	GPIOMUX_OUT_HIGH,
	GPIOMUX_OUT_LOW,
};

struct gpiomux_setting {
	enum gpiomux_func func;
	enum gpiomux_drv  drv;
	enum gpiomux_pull pull;
	enum gpiomux_dir  dir;
};

int get_msm_gpio_chip_base(void);
void msm_gpio_print_enabled(void);
int msm_set_gpio_status(struct gpio_chip *chip,
			uint pin_no, uint id, bool level);
void msm_gp_get_cfg(struct gpio_chip *chip, uint pin_no,
			struct gpiomux_setting *val);
int msm_gp_get_value(struct gpio_chip *chip, uint pin_no, int in_out_type);
bool msm_gpio_is_valid(int gpionum);

#endif /* __LINUX_PINCTRL_SEC_PINMUX_H */