/*
 * sec_quest_bps.h
 *
 *  Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *  jaecheol kim <jc22.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SEC_BPS_H
#define SEC_BPS_H

#include <linux/device.h>
#define QUEST_BPS_INFO_MAGIC				0xABCDABCD

enum {
	SEC_BPS_RBPSP = 0,
	SEC_BPS_RBPSF,
	SEC_BPS_N1BPS,
	SEC_BPS_N2BPS,
	SEC_BPS_N3BPS,
	SEC_BPS_N4BPS,
	SEC_BPS_FACINFO,
	SEC_BPS_MAX_ID,
};

/*
 * 0 : PBA : ~ SMD F/T end
 * 1 : SEMI ASSEMBLED : SMD T/T end ~ RF Cal start (booting)
 * 2 : ASSEMBLED : RF Cal start ~
 */
#define SEC_BPS_INFO_FAC_STEP_PBA				0
#define SEC_BPS_INFO_FAC_STEP_SEMI_ASSEMBLED	1
#define SEC_BPS_INFO_FAC_STEP_ASSEMBLED			2

ssize_t sec_bps_show_attrs(struct device *dev,
							struct device_attribute *attr, char *buf);
ssize_t sec_bps_store_attrs(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count);

#define SEC_BPS_ATTR(_name)							\
{													\
	.attr = { .name = #_name, .mode = 0664 },		\
	.show = sec_bps_show_attrs,						\
	.store = sec_bps_store_attrs,					\
}

struct sec_bps_param {
	struct work_struct sec_bps_work;
	int idx;
	int val;
};
static struct sec_bps_param sec_bps_param_data;
#endif
