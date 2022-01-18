/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SECDP_SYSFS_H
#define __SECDP_SYSFS_H

#include "secdp.h"

struct secdp_sysfs {
	struct class	*dp_class;
};

struct secdp_sysfs *secdp_sysfs_init(void);
void secdp_sysfs_deinit(struct secdp_sysfs *sysfs);

/**
 * secdp_sysfs_get() - get the functionalities of dp test module
 *
 *
 * return: a pointer to dp_link struct
 */
struct secdp_sysfs *secdp_sysfs_get(struct device *dev, struct secdp_misc *sec);

/**
 * secdp_sysfs_put() - releases the dp test module's resources
 *
 * @dp_link: an instance of dp_link module
 *
 */
void secdp_sysfs_put(struct secdp_sysfs *dp_sysfs);

#endif /*__SECDP_SYSFS_H*/
