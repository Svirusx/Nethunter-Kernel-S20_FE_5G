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

#ifndef __SELF_DISPLAY_HAC_H__
#define __SELF_DISPLAY_HAC_H__

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include "self_display.h"

int self_display_init_HAC(struct samsung_display_driver_data *vdd);
void make_self_dispaly_img_cmds_HAC(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type cmd, u32 op);
void make_mass_self_display_img_cmds_HAC(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type cmd, u32 op);
#if 0 /* No need to update this just use that from dtsi */
#define AC_HH_MASK_ST_X 0x0F
#define AC_HH_MASK_ST_Y 0x07
#define AC_HH_MASK_ED_X 0x85
#define AC_HH_MASK_ED_Y 0x1F
#define AC_MM_MASK_ST_X 0x0F
#define AC_MM_MASK_ST_Y 0x07
#define AC_MM_MASK_ED_X 0xA6
#define AC_MM_MASK_ED_Y 0x20
#define AC_SS_MASK_ST_X 0x01
#define AC_SS_MASK_ST_Y 0x00
#define AC_SS_MASK_ED_X 0xB2
#define AC_SS_MASK_ED_Y 0x28

#define AC_HH_MEM_REUSE_X 0
#define AC_HH_MEM_REUSE_Y 0
#define AC_HH_MEM_REUSE_W 640
#define AC_HH_MEM_REUSE_H 640
#endif
#endif /* __SELF_DISPLAY_HAC_H__ */
