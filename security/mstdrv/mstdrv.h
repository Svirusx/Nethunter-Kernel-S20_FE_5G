/*
 * @file mstdrv.h
 * @brief Header file for MST driver
 * Copyright (c) 2015, Samsung Electronics Corporation. All rights reserved.
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

#ifndef MST_DRV_H
#define MST_DRV_H

#define MST_DRV_DEV     "mst_drv"

#define MFC_MST_LDO_CONFIG_1				0x7400
#define MFC_MST_LDO_CONFIG_2				0x7409
#define MFC_MST_LDO_CONFIG_3				0x7418
#define MFC_MST_LDO_CONFIG_4				0x3014
#define MFC_MST_LDO_CONFIG_5				0x3405
#define MFC_MST_LDO_CONFIG_6				0x3010
#define MFC_MST_LDO_TURN_ON				0x301c
#define MFC_MST_LDO_CONFIG_8				0x343c
#define MFC_MST_OVER_TEMP_INT				0x0024

#define MFC_CHIP_ID_L_REG				0x00
#define MFC_CHIP_ID_P9320				0x20
#define MFC_CHIP_ID_S2MIW04				0x04

typedef enum {
	MFC_CHIP_ID_IDT = 1,
	MFC_CHIP_ID_LSI,
} mfc_chip_vendor;

#endif /* MST_DRV_H */
