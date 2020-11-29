/*
 * @file hdm_log.h
 * @brief Header file for HDM log
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

#ifndef __HDM_LOG_H__
#define __HDM_LOG_H__

#include <linux/printk.h>

#define TAG				"[sec_hdm]"

void hdm_printk(int level, const char *fmt, ...);

#define DEV_ERR			(1)
#define DEV_WARN		(2)
#define DEV_NOTI		(3)
#define DEV_INFO		(4)
#define DEV_DEBUG		(5)
#define HDM_LOG_LEVEL		DEV_INFO

#define hdm_err(fmt, ...)	hdm_printk(DEV_ERR, fmt, ## __VA_ARGS__);
#define hdm_warn(fmt, ...)	hdm_printk(DEV_WARN, fmt, ## __VA_ARGS__);
#define hdm_noti(fmt, ...)	hdm_printk(DEV_NOTI, fmt, ## __VA_ARGS__);
#define hdm_info(fmt, ...)	hdm_printk(DEV_INFO, fmt, ## __VA_ARGS__);
#define hdm_debug(fmt, ...)	hdm_printk(DEV_DEBUG, fmt, ## __VA_ARGS__);

#endif //__HDM_LOG_H__
