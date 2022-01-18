/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 */

#ifndef __LINUX_PINCTRL_MSM_H__
#define __LINUX_PINCTRL_MSM_H__

enum msm_gpio_wake {
	MSM_GPIO_WAKE_NONE = 0,
	MSM_GPIO_WAKE_DISABLED,
	MSM_GPIO_WAKE_ENABLED
};

/* APIS to access qup_i3c registers */
int msm_qup_write(u32 mode, u32 val);
int msm_qup_read(u32 mode);

/* API to write to mpm_wakeup registers */
int msm_gpio_mpm_wake_set(unsigned int gpio, bool enable);
enum msm_gpio_wake msm_gpio_mpm_wake_get(unsigned int gpio);

#endif /* __LINUX_PINCTRL_MSM_H__ */
