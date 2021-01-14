/*
 * @file hdm.h
 * @brief Header file for HDM driver
 * Copyright (c) 2019, Samsung Electronics Corporation. All rights reserved.
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

#ifndef __HDM_H__
#define __HDM_H__
#include <linux/types.h>


#ifndef __ASSEMBLY__

#define HDM_CMD_LEN ((size_t)8)

#define HDM_P_BITMASK		0xFFFF
#define HDM_C_BITMASK		0xF0000
#define HDM_KERNEL_PRE		0x10000
#define HDM_KERNEL_POST		0x20000
#define HDM_HYP_CALL		0x40000
#define HDM_HYP_CALLP		0x80000
#define HDM_CMD_MAX			0xFFFFF

enum {
	HDM_ALLOW = 0,
	HDM_PROTECT,
};

enum {
	HDM_DEVICE_CAMERA = 0,
	HDM_DEVICE_MMC,
	HDM_DEVICE_USB,
	HDM_DEVICE_WIFI,
	HDM_DEVICE_BLUETOOTH,
	HDM_DEVICE_MAX,
};

enum {
	APP_HDM_EVENT_INIT = 0,
	APP_HDM_EVENT_BLOCK_ALL,
	APP_HDM_EVENT_CTRL_DEVICE,
	APP_HDM_EVENT_CAMERA,
	APP_HDM_EVENT_MMC,
	APP_HDM_EVENT_USB,
	APP_HDM_EVENT_WIFI,
	APP_HDM_EVENT_BLUETOOTH,

	APP_HDM_EVENT_MAX
};


typedef int (*hdm_device_control_handler_t)(int cmd);

typedef struct {
	hdm_device_control_handler_t fn;
} hdm_device_ctrl_t;

extern struct net_device *hdm_net;


extern int hdm_prot[HDM_DEVICE_MAX];

extern int hdm_command(int cmd_id);

extern const struct file_operations hdm_fops;

#endif //__ASSEMBLY__
#endif //__HDM_H__
