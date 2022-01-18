/*
 * Copyright (C) 2016-2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * for ps5169 driver
 */

#ifndef __LINUX_PS5169_H__
#define __LINUX_PS5169_H__

#define Chip_ID1				0xAD
#define Chip_ID2				0xAC
#define Chip_Rev1				0x0AF
#define Chip_Rev2				0x0AE

enum config_type {
	WORK_MODE = 0,
	USB_ONLY_MODE,
	DP_ONLY_MODE,
	DP2_LANE_USB_MODE,
	CLEAR_STATE,
	CHECK_EXIST,
};

struct ps5169_data {
	struct device *dev;
	struct i2c_client *i2c;
	struct mutex i2c_mutex;
	int redriver_en;
	int con_sel;
};

extern int ps5169_config(int config, int is_DFP);
extern int ps5169_i2c_read(u8 command);
#endif
