/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Samsung's POC Driver
 * Author: ChangJae Jang <cj1225.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef SAMSUNG_POC_COMMON_H
#define SAMSUNG_POC_COMMON_H

#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/err.h>
#include <linux/mutex.h>

#define POC_IMG_ADDR	(0x000000)
#define POC_ERASE_4KB	(4096)
#define POC_ERASE_32KB	(32768)
#define POC_ERASE_64KB	(65536)

#define DEBUG_POC_CNT 4096

enum FLASH_READ_CASE {
	READ_CASE1,
	READ_CASE2,
	NONE_CASE,
};

int ss_dsi_poc_init(struct samsung_display_driver_data *vdd);
void ss_poc_read_mca(struct samsung_display_driver_data *vdd);
void ss_poc_comp(struct samsung_display_driver_data *vdd);

#endif
