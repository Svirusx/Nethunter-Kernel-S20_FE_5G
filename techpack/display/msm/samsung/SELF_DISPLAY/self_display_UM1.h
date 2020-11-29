/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * DDI operation : self clock, self mask, self icon.. etc.
 * Author: QC LCD driver <cj1225.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SELF_DISPLAY_UM1_H__
#define __SELF_DISPLAY_UM1_H__

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include "self_display.h"

int self_display_init_UM1(struct samsung_display_driver_data *vdd);
void make_self_dispaly_img_cmds_UM1(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type cmd, u32 op);

#endif