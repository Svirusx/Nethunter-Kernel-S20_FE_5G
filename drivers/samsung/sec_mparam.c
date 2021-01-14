/*
 * drivers/samsung/sec_mparam.c
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>

/* sec_mparam.mparam_ex=n from kernel command line */
int __read_mostly mparam_ex;
module_param(mparam_ex, int, S_IRUGO);

EXPORT_SYMBOL(mparam_ex);

unsigned int __read_mostly lpcharge;
EXPORT_SYMBOL(lpcharge);
module_param(lpcharge, uint, 0444);

int __read_mostly fg_reset;
EXPORT_SYMBOL(fg_reset);
module_param(fg_reset, int, 0444);

int __read_mostly factory_mode;
EXPORT_SYMBOL(factory_mode);
module_param(factory_mode, int, 0444);

int __read_mostly pd_disable;
EXPORT_SYMBOL(pd_disable);
module_param(pd_disable, int, 0444);

int __read_mostly wireless_ic;
EXPORT_SYMBOL(wireless_ic);
module_param(wireless_ic, int, 0444);

int __read_mostly pmic_info;
EXPORT_SYMBOL(pmic_info);
module_param(pmic_info, int, 0444);

int __read_mostly ccic_info;
EXPORT_SYMBOL(ccic_info);
module_param(ccic_info, int, 0444);

int __read_mostly charging_mode;
EXPORT_SYMBOL(charging_mode);
module_param(charging_mode, int, 0444);

static int sec_mparam_init(void)
{
	pr_err("%s done!!\n", __func__);
	pr_info("%s: lpcharge=%d, fg_reset=%d, factory_mode=%d, pd_disable=0x%x, wireless_ic=0x%08x\n",
		__func__, lpcharge, fg_reset, factory_mode, pd_disable, wireless_ic);
	pr_info("%s: pmic_info=%d, ccic_info=%d, charging_mode=%x\n",
		__func__, pmic_info, ccic_info, charging_mode);

	return 0;
}

module_init(sec_mparam_init);
MODULE_LICENSE("GPL");
