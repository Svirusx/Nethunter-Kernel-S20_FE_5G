/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

/*@===========================================================
 * include files
 *============================================================
 */

#include "mp_precomp.h"
#include "phydm_precomp.h"

/*@************************************************************
 * Global var
 * ************************************************************
 */

u32 ofdm_swing_tablebu[OFDM_TABLE_SIZE] = {
	0x7f8001fe, /* 0, +6.0dB */
	0x788001e2, /* 1, +5.5dB */
	0x71c001c7, /* 2, +5.0dB*/
	0x6b8001ae, /* 3, +4.5dB*/
	0x65400195, /* 4, +4.0dB*/
	0x5fc0017f, /* 5, +3.5dB*/
	0x5a400169, /* 6, +3.0dB*/
	0x55400155, /* 7, +2.5dB*/
	0x50800142, /* 8, +2.0dB*/
	0x4c000130, /* 9, +1.5dB*/
	0x47c0011f, /* 10, +1.0dB*/
	0x43c0010f, /* 11, +0.5dB*/
	0x40000100, /* 12, +0dB*/
	0x3c8000f2, /* 13, -0.5dB*/
	0x390000e4, /* 14, -1.0dB*/
	0x35c000d7, /* 15, -1.5dB*/
	0x32c000cb, /* 16, -2.0dB*/
	0x300000c0, /* 17, -2.5dB*/
	0x2d4000b5, /* 18, -3.0dB*/
	0x2ac000ab, /* 19, -3.5dB*/
	0x288000a2, /* 20, -4.0dB*/
	0x26000098, /* 21, -4.5dB*/
	0x24000090, /* 22, -5.0dB*/
	0x22000088, /* 23, -5.5dB*/
	0x20000080, /* 24, -6.0dB*/
	0x1e400079, /* 25, -6.5dB*/
	0x1c800072, /* 26, -7.0dB*/
	0x1b00006c, /* 27. -7.5dB*/
	0x19800066, /* 28, -8.0dB*/
	0x18000060, /* 29, -8.5dB*/
	0x16c0005b, /* 30, -9.0dB*/
	0x15800056, /* 31, -9.5dB*/
	0x14400051, /* 32, -10.0dB*/
	0x1300004c, /* 33, -10.5dB*/
	0x12000048, /* 34, -11.0dB*/
	0x11000044, /* 35, -11.5dB*/
	0x10000040, /* 36, -12.0dB*/
};

u8 cck_swing_table_ch1_ch13bu[CCK_TABLE_SIZE][8] = {
	{0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04}, /* 0, +0dB */
	{0x33, 0x32, 0x2b, 0x23, 0x1a, 0x11, 0x08, 0x04}, /* 1, -0.5dB */
	{0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03}, /* 2, -1.0dB */
	{0x2d, 0x2d, 0x27, 0x1f, 0x18, 0x0f, 0x08, 0x03}, /* 3, -1.5dB */
	{0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03}, /* 4, -2.0dB */
	{0x28, 0x28, 0x22, 0x1c, 0x15, 0x0d, 0x07, 0x03}, /* 5, -2.5dB */
	{0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03}, /* 6, -3.0dB */
	{0x24, 0x23, 0x1f, 0x19, 0x13, 0x0c, 0x06, 0x03}, /* 7, -3.5dB */
	{0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02}, /* 8, -4.0dB */
	{0x20, 0x20, 0x1b, 0x16, 0x11, 0x08, 0x05, 0x02}, /* 9, -4.5dB */
	{0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02}, /* 10, -5.0dB */
	{0x1d, 0x1c, 0x18, 0x14, 0x0f, 0x0a, 0x05, 0x02}, /* 11, -5.5dB */
	{0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02}, /* 12, -6.0 default*/
	{0x1a, 0x19, 0x16, 0x12, 0x0d, 0x09, 0x04, 0x02}, /* 13, -6.5dB */
	{0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02}, /* 14, -7.0dB */
	{0x17, 0x16, 0x13, 0x10, 0x0c, 0x08, 0x04, 0x02}, /* 15, -7.5dB */
	{0x16, 0x15, 0x12, 0x0f, 0x0b, 0x07, 0x04, 0x01}, /* 16, -8.0dB */
	{0x14, 0x14, 0x11, 0x0e, 0x0b, 0x07, 0x03, 0x02}, /* 17, -8.5dB */
	{0x13, 0x13, 0x10, 0x0d, 0x0a, 0x06, 0x03, 0x01}, /* 18, -9.0dB */
	{0x12, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01}, /* 19, -9.5dB */
	{0x11, 0x11, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01}, /* 20, -10.0dB */
	{0x10, 0x10, 0x0e, 0x0b, 0x08, 0x05, 0x03, 0x01}, /* 21, -10.5dB */
	{0x0f, 0x0f, 0x0d, 0x0b, 0x08, 0x05, 0x03, 0x01}, /* 22, -11.0dB */
	{0x0e, 0x0e, 0x0c, 0x0a, 0x08, 0x05, 0x02, 0x01}, /* 23, -11.5dB */
	{0x0d, 0x0d, 0x0c, 0x0a, 0x07, 0x05, 0x02, 0x01}, /* 24, -12.0dB */
	{0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x04, 0x02, 0x01}, /* 25, -12.5dB */
	{0x0c, 0x0c, 0x0a, 0x09, 0x06, 0x04, 0x02, 0x01}, /* 26, -13.0dB */
	{0x0b, 0x0b, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x01}, /* 27, -13.5dB */
	{0x0b, 0x0a, 0x09, 0x08, 0x06, 0x04, 0x02, 0x01}, /* 28, -14.0dB */
	{0x0a, 0x0a, 0x09, 0x07, 0x05, 0x03, 0x02, 0x01}, /* 29, -14.5dB */
	{0x0a, 0x09, 0x08, 0x07, 0x05, 0x03, 0x02, 0x01}, /* 30, -15.0dB */
	{0x09, 0x09, 0x08, 0x06, 0x05, 0x03, 0x01, 0x01}, /* 31, -15.5dB */
	{0x09, 0x08, 0x07, 0x06, 0x04, 0x03, 0x01, 0x01} /* 32, -16.0dB */
};

u8 cck_swing_table_ch14bu[CCK_TABLE_SIZE][8] = {
	{0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00}, /* 0, +0dB */
	{0x33, 0x32, 0x2b, 0x19, 0x00, 0x00, 0x00, 0x00}, /* 1, -0.5dB */
	{0x30, 0x2f, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00}, /* 2, -1.0dB */
	{0x2d, 0x2d, 0x17, 0x17, 0x00, 0x00, 0x00, 0x00}, /* 3, -1.5dB */
	{0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00}, /* 4, -2.0dB */
	{0x28, 0x28, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00}, /* 5, -2.5dB */
	{0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00}, /* 6, -3.0dB */
	{0x24, 0x23, 0x1f, 0x12, 0x00, 0x00, 0x00, 0x00}, /* 7, -3.5dB */
	{0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00}, /* 8, -4.0dB */
	{0x20, 0x20, 0x1b, 0x10, 0x00, 0x00, 0x00, 0x00}, /* 9, -4.5dB */
	{0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00}, /* 10, -5.0dB */
	{0x1d, 0x1c, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00}, /* 11, -5.5dB */
	{0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00}, /* 12, -6.0 default*/
	{0x1a, 0x19, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00}, /* 13, -6.5dB */
	{0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00}, /* 14, -7.0dB */
	{0x17, 0x16, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00}, /* 15, -7.5dB */
	{0x16, 0x15, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00}, /* 16, -8.0dB */
	{0x14, 0x14, 0x11, 0x0a, 0x00, 0x00, 0x00, 0x00}, /* 17, -8.5dB */
	{0x13, 0x13, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00}, /* 18, -9.0dB */
	{0x12, 0x12, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00}, /* 19, -9.5dB */
	{0x11, 0x11, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00}, /* 20, -10.0dB */
	{0x10, 0x10, 0x0e, 0x08, 0x00, 0x00, 0x00, 0x00}, /* 21, -10.5dB */
	{0x0f, 0x0f, 0x0d, 0x08, 0x00, 0x00, 0x00, 0x00}, /* 22, -11.0dB */
	{0x0e, 0x0e, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00}, /* 23, -11.5dB */
	{0x0d, 0x0d, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00}, /* 24, -12.0dB */
	{0x0d, 0x0c, 0x0b, 0x06, 0x00, 0x00, 0x00, 0x00}, /* 25, -12.5dB */
	{0x0c, 0x0c, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00}, /* 26, -13.0dB */
	{0x0b, 0x0b, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00}, /* 27, -13.5dB */
	{0x0b, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00}, /* 28, -14.0dB */
	{0x0a, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00}, /* 29, -14.5dB */
	{0x0a, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00}, /* 30, -15.0dB */
	{0x09, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00}, /* 31, -15.5dB */
	{0x09, 0x08, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00} /* 32, -16.0dB */
};

u32 ofdm_swing_tablebu_newbu[OFDM_TABLE_SIZE] = {
	0x0b40002d, /* 0,  -15.0dB */
	0x0c000030, /* 1,  -14.5dB */
	0x0cc00033, /* 2,  -14.0dB */
	0x0d800036, /* 3,  -13.5dB */
	0x0e400039, /* 4,  -13.0dB */
	0x0f00003c, /* 5,  -12.5dB */
	0x10000040, /* 6,  -12.0dB */
	0x11000044, /* 7,  -11.5dB */
	0x12000048, /* 8,  -11.0dB */
	0x1300004c, /* 9,  -10.5dB */
	0x14400051, /* 10, -10.0dB */
	0x15800056, /* 11, -9.5dB */
	0x16c0005b, /* 12, -9.0dB */
	0x18000060, /* 13, -8.5dB */
	0x19800066, /* 14, -8.0dB */
	0x1b00006c, /* 15, -7.5dB */
	0x1c800072, /* 16, -7.0dB */
	0x1e400079, /* 17, -6.5dB */
	0x20000080, /* 18, -6.0dB */
	0x22000088, /* 19, -5.5dB */
	0x24000090, /* 20, -5.0dB */
	0x26000098, /* 21, -4.5dB */
	0x288000a2, /* 22, -4.0dB */
	0x2ac000ab, /* 23, -3.5dB */
	0x2d4000b5, /* 24, -3.0dB */
	0x300000c0, /* 25, -2.5dB */
	0x32c000cb, /* 26, -2.0dB */
	0x35c000d7, /* 27, -1.5dB */
	0x390000e4, /* 28, -1.0dB */
	0x3c8000f2, /* 29, -0.5dB */
	0x40000100, /* 30, +0dB */
	0x43c0010f, /* 31, +0.5dB */
	0x47c0011f, /* 32, +1.0dB */
	0x4c000130, /* 33, +1.5dB */
	0x50800142, /* 34, +2.0dB */
	0x55400155, /* 35, +2.5dB */
	0x5a400169, /* 36, +3.0dB */
	0x5fc0017f, /* 37, +3.5dB */
	0x65400195, /* 38, +4.0dB */
	0x6b8001ae, /* 39, +4.5dB */
	0x71c001c7, /* 40, +5.0dB */
	0x788001e2, /* 41, +5.5dB */
	0x7f8001fe /* 42, +6.0dB */
};

u8 cck_swing_table_ch1_ch14_88fbu[CCK_TABLE_SIZE_88F][16] = {
	{0x44, 0x42, 0x3C, 0x33, 0x28, 0x1C, 0x13, 0x0B, 0x05, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-16dB*/
	{0x48, 0x46, 0x3F, 0x36, 0x2A, 0x1E, 0x14, 0x0B, 0x05, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-15.5dB*/
	{0x4D, 0x4A, 0x43, 0x39, 0x2C, 0x20, 0x15, 0x0C, 0x06, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-15dB*/
	{0x51, 0x4F, 0x47, 0x3C, 0x2F, 0x22, 0x16, 0x0D, 0x06, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-14.5dB*/
	{0x56, 0x53, 0x4B, 0x40, 0x32, 0x24, 0x17, 0x0E, 0x06, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-14dB*/
	{0x5B, 0x58, 0x50, 0x43, 0x35, 0x26, 0x19, 0x0E, 0x07, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-13.5dB*/
	{0x60, 0x5D, 0x54, 0x47, 0x38, 0x28, 0x1A, 0x0F, 0x07, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-13dB*/
	{0x66, 0x63, 0x59, 0x4C, 0x3B, 0x2B, 0x1C, 0x10, 0x08, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-12.5dB*/
	{0x6C, 0x69, 0x5F, 0x50, 0x3F, 0x2D, 0x1E, 0x11, 0x08, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-12dB*/
	{0x73, 0x6F, 0x64, 0x55, 0x42, 0x30, 0x1F, 0x12, 0x08, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-11.5dB*/
	{0x79, 0x76, 0x6A, 0x5A, 0x46, 0x33, 0x21, 0x13, 0x09, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-11dB*/
	{0x81, 0x7C, 0x71, 0x5F, 0x4A, 0x36, 0x23, 0x14, 0x0A, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-10.5dB*/
	{0x88, 0x84, 0x77, 0x65, 0x4F, 0x39, 0x25, 0x15, 0x0A, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-10dB*/
	{0x90, 0x8C, 0x7E, 0x6B, 0x54, 0x3C, 0x27, 0x17, 0x0B, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-9.5dB*/
	{0x99, 0x94, 0x86, 0x71, 0x58, 0x40, 0x2A, 0x18, 0x0B, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-9dB*/
	{0xA2, 0x9D, 0x8E, 0x78, 0x5E, 0x43, 0x2C, 0x19, 0x0C, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-8.5dB*/
	{0xAC, 0xA6, 0x96, 0x7F, 0x63, 0x47, 0x2F, 0x1B, 0x0D, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-8dB*/
	{0xB6, 0xB0, 0x9F, 0x87, 0x69, 0x4C, 0x32, 0x1D, 0x0D, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-7.5dB*/
	{0xC1, 0xBA, 0xA8, 0x8F, 0x6F, 0x50, 0x35, 0x1E, 0x0E, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-7dB*/
	{0xCC, 0xC5, 0xB2, 0x97, 0x76, 0x55, 0x38, 0x20, 0x0F, 0x05,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-6.5dB*/
	{0xD8, 0xD1, 0xBD, 0xA0, 0x7D, 0x5A, 0x3B, 0x22, 0x10, 0x05,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00} /*-6dB*/
};

u8 cck_swing_table_ch1_ch13bu_88f[CCK_TABLE_SIZE_88F][16] = {
	{0x44, 0x42, 0x3C, 0x33, 0x28, 0x1C, 0x13, 0x0B, 0x05, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-16dB*/
	{0x48, 0x46, 0x3F, 0x36, 0x2A, 0x1E, 0x14, 0x0B, 0x05, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-15.5dB*/
	{0x4D, 0x4A, 0x43, 0x39, 0x2C, 0x20, 0x15, 0x0C, 0x06, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-15dB*/
	{0x51, 0x4F, 0x47, 0x3C, 0x2F, 0x22, 0x16, 0x0D, 0x06, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-14.5dB*/
	{0x56, 0x53, 0x4B, 0x40, 0x32, 0x24, 0x17, 0x0E, 0x06, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-14dB*/
	{0x5B, 0x58, 0x50, 0x43, 0x35, 0x26, 0x19, 0x0E, 0x07, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-13.5dB*/
	{0x60, 0x5D, 0x54, 0x47, 0x38, 0x28, 0x1A, 0x0F, 0x07, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-13dB*/
	{0x66, 0x63, 0x59, 0x4C, 0x3B, 0x2B, 0x1C, 0x10, 0x08, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-12.5dB*/
	{0x6C, 0x69, 0x5F, 0x50, 0x3F, 0x2D, 0x1E, 0x11, 0x08, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-12dB*/
	{0x73, 0x6F, 0x64, 0x55, 0x42, 0x30, 0x1F, 0x12, 0x08, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-11.5dB*/
	{0x79, 0x76, 0x6A, 0x5A, 0x46, 0x33, 0x21, 0x13, 0x09, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-11dB*/
	{0x81, 0x7C, 0x71, 0x5F, 0x4A, 0x36, 0x23, 0x14, 0x0A, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-10.5dB*/
	{0x88, 0x84, 0x77, 0x65, 0x4F, 0x39, 0x25, 0x15, 0x0A, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-10dB*/
	{0x90, 0x8C, 0x7E, 0x6B, 0x54, 0x3C, 0x27, 0x17, 0x0B, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-9.5dB*/
	{0x99, 0x94, 0x86, 0x71, 0x58, 0x40, 0x2A, 0x18, 0x0B, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-9dB*/
	{0xA2, 0x9D, 0x8E, 0x78, 0x5E, 0x43, 0x2C, 0x19, 0x0C, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-8.5dB*/
	{0xAC, 0xA6, 0x96, 0x7F, 0x63, 0x47, 0x2F, 0x1B, 0x0D, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-8dB*/
	{0xB6, 0xB0, 0x9F, 0x87, 0x69, 0x4C, 0x32, 0x1D, 0x0D, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-7.5dB*/
	{0xC1, 0xBA, 0xA8, 0x8F, 0x6F, 0x50, 0x35, 0x1E, 0x0E, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-7dB*/
	{0xCC, 0xC5, 0xB2, 0x97, 0x76, 0x55, 0x38, 0x20, 0x0F, 0x05,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-6.5dB*/
	{0xD8, 0xD1, 0xBD, 0xA0, 0x7D, 0x5A, 0x3B, 0x22, 0x10, 0x05,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00} /*-6dB*/
};

u8 cck_swing_table_ch14bu_88f[CCK_TABLE_SIZE_88F][16] = {
	{0x44, 0x42, 0x3C, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-16dB*/
	{0x48, 0x46, 0x3F, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-15.5dB*/
	{0x4D, 0x4A, 0x43, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-15dB*/
	{0x51, 0x4F, 0x47, 0x2F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-14.5dB*/
	{0x56, 0x53, 0x4B, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-14dB*/
	{0x5B, 0x58, 0x50, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-13.5dB*/
	{0x60, 0x5D, 0x54, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-13dB*/
	{0x66, 0x63, 0x59, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-12.5dB*/
	{0x6C, 0x69, 0x5F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-12dB*/
	{0x73, 0x6F, 0x64, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-11.5dB*/
	{0x79, 0x76, 0x6A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-11dB*/
	{0x81, 0x7C, 0x71, 0x4A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-10.5dB*/
	{0x88, 0x84, 0x77, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-10dB*/
	{0x90, 0x8C, 0x7E, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-9.5dB*/
	{0x99, 0x94, 0x86, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-9dB*/
	{0xA2, 0x9D, 0x8E, 0x5E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-8.5dB*/
	{0xAC, 0xA6, 0x96, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-8dB*/
	{0xB6, 0xB0, 0x9F, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-7.5dB*/
	{0xC1, 0xBA, 0xA8, 0x6F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-7dB*/
	{0xCC, 0xC5, 0xB2, 0x76, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-6.5dB*/
	{0xD8, 0xD1, 0xBD, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00} /*-6dB*/
};

u8 cck_swing_table_ch1_ch13bu_newbu[CCK_TABLE_SIZE][8] = {
	{0x09, 0x08, 0x07, 0x06, 0x04, 0x03, 0x01, 0x01}, /*   0, -16.0dB*/
	{0x09, 0x09, 0x08, 0x06, 0x05, 0x03, 0x01, 0x01}, /*   1, -15.5dB*/
	{0x0a, 0x09, 0x08, 0x07, 0x05, 0x03, 0x02, 0x01}, /*   2, -15.0dB*/
	{0x0a, 0x0a, 0x09, 0x07, 0x05, 0x03, 0x02, 0x01}, /*   3, -14.5dB*/
	{0x0b, 0x0a, 0x09, 0x08, 0x06, 0x04, 0x02, 0x01}, /*   4, -14.0dB*/
	{0x0b, 0x0b, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x01}, /*   5, -13.5dB*/
	{0x0c, 0x0c, 0x0a, 0x09, 0x06, 0x04, 0x02, 0x01}, /*   6, -13.0dB*/
	{0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x04, 0x02, 0x01}, /*   7, -12.5dB*/
	{0x0d, 0x0d, 0x0c, 0x0a, 0x07, 0x05, 0x02, 0x01}, /*   8, -12.0dB*/
	{0x0e, 0x0e, 0x0c, 0x0a, 0x08, 0x05, 0x02, 0x01}, /*   9, -11.5dB*/
	{0x0f, 0x0f, 0x0d, 0x0b, 0x08, 0x05, 0x03, 0x01}, /*  10, -11.0dB*/
	{0x10, 0x10, 0x0e, 0x0b, 0x08, 0x05, 0x03, 0x01}, /*  11, -10.5dB*/
	{0x11, 0x11, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01}, /*  12, -10.0dB*/
	{0x12, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01}, /*  13, -9.5dB*/
	{0x13, 0x13, 0x10, 0x0d, 0x0a, 0x06, 0x03, 0x01}, /*  14, -9.0dB */
	{0x14, 0x14, 0x11, 0x0e, 0x0b, 0x07, 0x03, 0x02}, /*  15, -8.5dB*/
	{0x16, 0x15, 0x12, 0x0f, 0x0b, 0x07, 0x04, 0x01}, /*  16, -8.0dB */
	{0x17, 0x16, 0x13, 0x10, 0x0c, 0x08, 0x04, 0x02}, /*  17, -7.5dB*/
	{0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02}, /*  18, -7.0dB */
	{0x1a, 0x19, 0x16, 0x12, 0x0d, 0x09, 0x04, 0x02}, /*  19, -6.5dB*/
	{0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02}, /*  20, -6.0dB */
	{0x1d, 0x1c, 0x18, 0x14, 0x0f, 0x0a, 0x05, 0x02}, /*  21, -5.5dB*/
	{0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02}, /*  22, -5.0dB */
	{0x20, 0x20, 0x1b, 0x16, 0x11, 0x08, 0x05, 0x02}, /*  23, -4.5dB*/
	{0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02}, /*  24, -4.0dB */
	{0x24, 0x23, 0x1f, 0x19, 0x13, 0x0c, 0x06, 0x03}, /*  25, -3.5dB*/
	{0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03}, /*  26, -3.0dB*/
	{0x28, 0x28, 0x22, 0x1c, 0x15, 0x0d, 0x07, 0x03}, /*  27, -2.5dB*/
	{0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03}, /*  28, -2.0dB */
	{0x2d, 0x2d, 0x27, 0x1f, 0x18, 0x0f, 0x08, 0x03}, /*  29, -1.5dB*/
	{0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03}, /*  30, -1.0dB*/
	{0x33, 0x32, 0x2b, 0x23, 0x1a, 0x11, 0x08, 0x04}, /*  31, -0.5dB*/
	{0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04} /*   32, +0dB*/
};

u8 cck_swing_table_ch14bu_new[CCK_TABLE_SIZE][8] = {
	{0x09, 0x08, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00}, /*  0, -16.0dB*/
	{0x09, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00}, /* 1, -15.5dB*/
	{0x0a, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00}, /*  2, -15.0dB*/
	{0x0a, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00}, /* 3, -14.5dB*/
	{0x0b, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00}, /*  4, -14.0dB*/
	{0x0b, 0x0b, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00}, /*5, -13.5dB*/
	{0x0c, 0x0c, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00}, /* 6, -13.0dB*/
	{0x0d, 0x0c, 0x0b, 0x06, 0x00, 0x00, 0x00, 0x00}, /*  7, -12.5dB*/
	{0x0d, 0x0d, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00}, /* 8, -12.0dB*/
	{0x0e, 0x0e, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00}, /* 9, -11.5dB*/
	{0x0f, 0x0f, 0x0d, 0x08, 0x00, 0x00, 0x00, 0x00}, /* 10, -11.0dB*/
	{0x10, 0x10, 0x0e, 0x08, 0x00, 0x00, 0x00, 0x00}, /*11, -10.5dB*/
	{0x11, 0x11, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00}, /* 12, -10.0dB*/
	{0x12, 0x12, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00}, /* 13, -9.5dB*/
	{0x13, 0x13, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00}, /*14, -9.0dB */
	{0x14, 0x14, 0x11, 0x0a, 0x00, 0x00, 0x00, 0x00}, /* 15, -8.5dB*/
	{0x16, 0x15, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00}, /* 16, -8.0dB */
	{0x17, 0x16, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00}, /* 17, -7.5dB*/
	{0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00}, /* 18, -7.0dB */
	{0x1a, 0x19, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00}, /* 19, -6.5dB */
	{0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00}, /* 20, -6.0dB */
	{0x1d, 0x1c, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00}, /* 21, -5.5dB*/
	{0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00}, /* 22, -5.0dB */
	{0x20, 0x20, 0x1b, 0x10, 0x00, 0x00, 0x00, 0x00}, /*23, -4.5dB*/
	{0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00}, /* 24, -4.0dB */
	{0x24, 0x23, 0x1f, 0x12, 0x00, 0x00, 0x00, 0x00}, /* 25, -3.5dB */
	{0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00}, /* 26, -3.0dB */
	{0x28, 0x28, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00}, /*27, -2.5dB*/
	{0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00}, /* 28, -2.0dB */
	{0x2d, 0x2d, 0x17, 0x17, 0x00, 0x00, 0x00, 0x00}, /*29, -1.5dB*/
	{0x30, 0x2f, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00}, /* 30, -1.0dB */
	{0x33, 0x32, 0x2b, 0x19, 0x00, 0x00, 0x00, 0x00}, /* 31, -0.5dB */
	{0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00} /* 32, +0dB	*/
};

u32 cck_swing_table_ch1_ch14_8723dbu[CCK_TABLE_SIZE_8723D] = {
	0x0CD, /*0 ,    -20dB*/
	0x0D9,
	0x0E6,
	0x0F3,
	0x102,
	0x111,
	0x121,
	0x132,
	0x144,
	0x158,
	0x16C,
	0x182,
	0x198,
	0x1B1,
	0x1CA,
	0x1E5,
	0x202,
	0x221,
	0x241,
	0x263,
	0x287,
	0x2AE,
	0x2D6,
	0x301,
	0x32F,
	0x35F,
	0x392,
	0x3C9,
	0x402,
	0x43F,
	0x47F,
	0x4C3,
	0x50C,
	0x558,
	0x5A9,
	0x5FF,
	0x65A,
	0x6BA,
	0x720,
	0x78C,
	0x7FF,
};

/*@JJ ADD 20161014 */
u32 cck_swing_table_ch1_ch14_8710bbu[CCK_TABLE_SIZE_8710B] = {
	0x0CD, /*0 ,    -20dB*/
	0x0D9,
	0x0E6,
	0x0F3,
	0x102,
	0x111,
	0x121,
	0x132,
	0x144,
	0x158,
	0x16C,
	0x182,
	0x198,
	0x1B1,
	0x1CA,
	0x1E5,
	0x202,
	0x221,
	0x241,
	0x263,
	0x287,
	0x2AE,
	0x2D6,
	0x301,
	0x32F,
	0x35F,
	0x392,
	0x3C9,
	0x402,
	0x43F,
	0x47F,
	0x4C3,
	0x50C,
	0x558,
	0x5A9,
	0x5FF,
	0x65A,
	0x6BA,
	0x720,
	0x78C,
	0x7FF,
};

/*@Winnita ADD 20171116 PathA 0xAB4[10:0],PathB 0xAB4[21:11]*/
u32 cck_swing_table_ch1_ch14_8192fbu[CCK_TABLE_SIZE_8192F] = {
	0x0CD, /*0 ,    -20dB*/
	0x0D9,
	0x0E6,
	0x0F3,
	0x102,
	0x111,
	0x121,
	0x132,
	0x144,
	0x158,
	0x16C,
	0x182,
	0x198,
	0x1B1,
	0x1CA,
	0x1E5,
	0x202,
	0x221,
	0x241,
	0x263, /*19*/
	0x287, /*20*/
	0x2AE, /*21*/
	0x2D6, /*22*/
	0x301, /*23*/
	0x32F, /*24*/
	0x35F, /*25*/
	0x392, /*26*/
	0x3C9, /*27*/
	0x402, /*28*/
	0x43F, /*29*/
	0x47F, /*30*/
	0x4C3, /*31*/
	0x50C, /*32*/
	0x558, /*33*/
	0x5A9, /*34*/
	0x5FF, /*35*/
	0x65A, /*36*/
	0x6BA,
	0x720,
	0x78C,
	0x7FF,
};

u32 tx_scaling_table_jaguarbu[TXSCALE_TABLE_SIZE] = {
	0x081, /* 0,  -12.0dB*/
	0x088, /* 1,  -11.5dB*/
	0x090, /* 2,  -11.0dB*/
	0x099, /* 3,  -10.5dB*/
	0x0A2, /* 4,  -10.0dB*/
	0x0AC, /* 5,  -9.5dB*/
	0x0B6, /* 6,  -9.0dB*/
	0x0C0, /*7,  -8.5dB*/
	0x0CC, /* 8,  -8.0dB*/
	0x0D8, /* 9,  -7.5dB*/
	0x0E5, /* 10, -7.0dB*/
	0x0F2, /* 11, -6.5dB*/
	0x101, /* 12, -6.0dB*/
	0x110, /* 13, -5.5dB*/
	0x120, /* 14, -5.0dB*/
	0x131, /* 15, -4.5dB*/
	0x143, /* 16, -4.0dB*/
	0x156, /* 17, -3.5dB*/
	0x16A, /* 18, -3.0dB*/
	0x180, /* 19, -2.5dB*/
	0x197, /* 20, -2.0dB*/
	0x1AF, /* 21, -1.5dB*/
	0x1C8, /* 22, -1.0dB*/
	0x1E3, /* 23, -0.5dB*/
	0x200, /* 24, +0  dB*/
	0x21E, /* 25, +0.5dB*/
	0x23E, /* 26, +1.0dB*/
	0x261, /* 27, +1.5dB*/
	0x285, /* 28, +2.0dB*/
	0x2AB, /* 29, +2.5dB*/
	0x2D3, /*30, +3.0dB*/
	0x2FE, /* 31, +3.5dB*/
	0x32B, /* 32, +4.0dB*/
	0x35C, /* 33, +4.5dB*/
	0x38E, /* 34, +5.0dB*/
	0x3C4, /* 35, +5.5dB*/
	0x3FE /* 36, +6.0dB	*/
};

#if (DM_ODM_SUPPORT_TYPE == ODM_CE) && defined(DM_ODM_CE_MAC80211)
#else
u8 delta_swing_table_idx_2ga_p_8188ebu[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3,
					  4, 4, 4, 4, 4, 4, 4, 4, 5, 5,
					  7, 7, 8, 8, 8, 9, 9, 9, 9, 9};
u8 delta_swing_table_idx_2ga_n_8188ebu[] = {0, 0, 0, 2, 2, 3, 3, 4, 4, 4,
					  4, 5, 5, 6, 6, 7, 7, 7, 7, 8,
					  8, 9, 9, 10, 10, 10, 11, 11, 11, 11};
#endif

void odm_txpowertracking_initbu(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_txpowertracking_thermal_meter_initbu(dm);
}

u8 get_swing_indexbu(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
#if ((RTL8812A_SUPPORT == 1) || (RTL8821A_SUPPORT == 1))
	void *adapter = dm->adapter;
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
#endif
	u8 i = 0;
	u32 bb_swing, table_value;

	if (dm->support_ic_type &
		(ODM_RTL8188E | ODM_RTL8723B | ODM_RTL8192E |
		ODM_RTL8188F | ODM_RTL8703B | ODM_RTL8723D |
		ODM_RTL8710B)) {

		bb_swing = odm_get_bb_regbu(dm, R_0xc80, 0xFFC00000);

		for (i = 0; i < OFDM_TABLE_SIZE; i++) {
			table_value = ofdm_swing_tablebu_newbu[i];

			if (table_value >= 0x100000)
				table_value >>= 22;
			if (bb_swing == table_value)
				break;
		}
	} else {
#if ((RTL8812A_SUPPORT == 1) || (RTL8821A_SUPPORT == 1))
		bb_swing =
		phy_get_tx_bb_swing_8812a(adapter,
					  hal_data->current_band_type,
					  RF_PATH_A);
#else
		bb_swing = odm_get_bb_regbu(dm, R_0xc1c, 0xFFE00000);
#endif
		for (i = 0; i < TXSCALE_TABLE_SIZE; i++) {
			table_value = tx_scaling_table_jaguarbu[i];

			if (bb_swing == table_value)
				break;
		}
	}

	return i;
}

u8 get_cck_swing_indexbu(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	u8 i = 0;
	u32 bb_cck_swing;

	if (dm->support_ic_type &
		(ODM_RTL8188E | ODM_RTL8723B | ODM_RTL8192E)) {
		bb_cck_swing = odm_read_1bytebu(dm, 0xa22);

		for (i = 0; i < CCK_TABLE_SIZE; i++) {
			if (bb_cck_swing == cck_swing_table_ch1_ch13bu_newbu[i][0])
				break;
		}
	} else if (dm->support_ic_type & ODM_RTL8703B) {
		bb_cck_swing = odm_read_1bytebu(dm, 0xa22);

		for (i = 0; i < CCK_TABLE_SIZE_88F; i++) {
			if (bb_cck_swing == cck_swing_table_ch1_ch14_88fbu[i][0])
				break;
		}
	}

	return i;
}

s8
get_txagc_default_index(
	void *dm_void
)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	s8 tmp;

	if (dm->support_ic_type == ODM_RTL8814B) {
		tmp = (s8)(odm_get_bb_regbu(dm, R_0x18a0, 0x7f) & 0xff);
		if (tmp & BIT(6))
			tmp = tmp | 0x80;
		return tmp;
	} else
		return 0;
}

void odm_txpowertracking_thermal_meter_initbu(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	u8 swing_idx = get_swing_indexbu(dm);
	u8 cckswing_idx = get_cck_swing_indexbu(dm);
	u8 p = 0;

	cali_info->is_txpowertracking = true;
	cali_info->tx_powercount = 0;
	cali_info->is_txpowertracking_init = false;

	if (!(*dm->mp_mode))
		cali_info->txpowertrack_control = true;
	else
		cali_info->txpowertrack_control = false;

	if (!(*dm->mp_mode))
		cali_info->txpowertrack_control = true;

	RF_DBG(dm, DBG_RF_IQK, "dm txpowertrack_control = %d\n",
	       cali_info->txpowertrack_control);
#if 0
	/* dm->rf_calibrate_info.txpowertrack_control = true; */
#endif
	cali_info->thermal_value = rf->eeprom_thermal;
	cali_info->thermal_value_iqk = rf->eeprom_thermal;
	cali_info->thermal_value_lck = rf->eeprom_thermal;

#if (RTL8822C_SUPPORT == 1 || RTL8814B_SUPPORT == 1 || RTL8723F_SUPPORT == 1)
	if (dm->support_ic_type == ODM_RTL8822C || 
	    dm->support_ic_type == ODM_RTL8723F) {
		cali_info->thermal_value_path[RF_PATH_A] = tssi->thermal[RF_PATH_A];
		cali_info->thermal_value_path[RF_PATH_B] = tssi->thermal[RF_PATH_B];
		cali_info->thermal_value_iqk = tssi->thermal[RF_PATH_A];
		cali_info->thermal_value_lck = tssi->thermal[RF_PATH_A];
	}
	
	if (dm->support_ic_type == ODM_RTL8814B) {
		cali_info->thermal_value_path[RF_PATH_A] = tssi->thermal[RF_PATH_A];
		cali_info->thermal_value_path[RF_PATH_B] = tssi->thermal[RF_PATH_B];
		cali_info->thermal_value_path[RF_PATH_C] = tssi->thermal[RF_PATH_C];
		cali_info->thermal_value_path[RF_PATH_D] = tssi->thermal[RF_PATH_D];
		cali_info->thermal_value_iqk = tssi->thermal[RF_PATH_A];
		cali_info->thermal_value_lck = tssi->thermal[RF_PATH_A];
	}
#endif

	if (!cali_info->default_bb_swing_index_flag) {
		if (dm->support_ic_type &
			(ODM_RTL8188E | ODM_RTL8723B | ODM_RTL8192E |
			ODM_RTL8703B | ODM_RTL8821)) {
			if (swing_idx >= OFDM_TABLE_SIZE)
				cali_info->default_ofdm_index = 30;
			else
				cali_info->default_ofdm_index = swing_idx;

			if (cckswing_idx >= CCK_TABLE_SIZE)
				cali_info->default_cck_index = 20;
			else
				cali_info->default_cck_index = cckswing_idx;
		/*@add by Mingzhi.Guo  2015-03-23*/
		} else if (dm->support_ic_type == ODM_RTL8188F) {
			cali_info->default_ofdm_index = 28; /*OFDM: -1dB*/
			cali_info->default_cck_index = 20; /*CCK:-6dB*/
		/*@add by zhaohe  2015-10-27*/
		} else if (dm->support_ic_type == ODM_RTL8723D) {
			cali_info->default_ofdm_index = 28; /*OFDM: -1dB*/
			cali_info->default_cck_index = 28; /*CCK:   -6dB*/
		/*@JJ ADD 20161014 */
		} else if (dm->support_ic_type == ODM_RTL8710B) {
			cali_info->default_ofdm_index = 28; /*OFDM: -1dB*/
			cali_info->default_cck_index = 28; /*CCK:   -6dB*/
		} else if (dm->support_ic_type == ODM_RTL8192F) {
			cali_info->default_ofdm_index = 30;/*OFDM: 0dB*/
			cali_info->default_cck_index = 28; /*CCK:   -6dB*/
		} else {
			if (swing_idx >= TXSCALE_TABLE_SIZE)
				cali_info->default_ofdm_index = 24;
			else
				cali_info->default_ofdm_index = swing_idx;

			cali_info->default_txagc_index = get_txagc_default_index(dm);

			cali_info->default_cck_index = 24;
		}
		cali_info->default_bb_swing_index_flag = true;
	}

	cali_info->bb_swing_idx_cck_base = cali_info->default_cck_index;
	cali_info->CCK_index = cali_info->default_cck_index;

	for (p = RF_PATH_A; p < MAX_RF_PATH; ++p) {
		cali_info->bb_swing_idx_ofdm_base[p] =
						cali_info->default_ofdm_index;
		cali_info->OFDM_index[p] = cali_info->default_ofdm_index;
		cali_info->delta_power_index[p] = 0;
		cali_info->delta_power_index_last[p] = 0;
		cali_info->power_index_offset[p] = 0;
	}
	cali_info->modify_tx_agc_value_ofdm = 0;
	cali_info->modify_tx_agc_value_cck = 0;
	cali_info->tm_trigger = 0;
}

void odm_txpowertracking_checkbu(void *dm_void)
{
	/*@2011/09/29 MH In HW integration first stage
	 * we provide 4 different handle to operate at the same time.
	 * In the stage2/3, we need to prive universal interface and merge all
	 * HW dynamic mechanism.
	 */
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	switch (dm->support_platform) {
	case ODM_WIN:
		odm_txpowertracking_checkbu_mp(dm);
		break;

	case ODM_CE:
		odm_txpowertracking_checkbu_ce(dm);
		break;

	case ODM_AP:
		odm_txpowertracking_checkbu_ap(dm);
		break;

	default:
		break;
	}
}

void odm_txpowertracking_checkbu_ce(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	if (!(rf->rf_supportability & HAL_RF_TX_PWR_TRACK))
		return;
#if (RTL8723F_SUPPORT == 1)
	if (dm->support_ic_type & ODM_RTL8723F) {
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] Pwrtrack return!\n");*/
		return;
	}
#endif

	if ((rf->power_track_type & 0xf0) >> 4 != 0) {
		if (dm->support_ic_type & ODM_RTL8822C) {
			/*halrf_tssi_cckbu(dm);*/
			/*halrf_thermal_cckbu(dm);*/
			return;
		}
	}

	if (!dm->rf_calibrate_info.tm_trigger) {
		if (dm->support_ic_type &
			(ODM_RTL8188E | ODM_RTL8188F | ODM_RTL8192E |
			ODM_RTL8723B | ODM_RTL8812 | ODM_RTL8821 |
			ODM_RTL8814A | ODM_RTL8703B | ODM_RTL8723D |
			ODM_RTL8822B | ODM_RTL8821C | ODM_RTL8710B |
			ODM_RTL8192F))
			odm_set_rf_regbu(dm, RF_PATH_A, RF_T_METER_NEW,
				       (BIT(17) | BIT(16)), 0x03);
		else if (dm->support_ic_type & ODM_RTL8822C) {
			odm_set_rf_regbu(dm, RF_PATH_A, R_0x42, BIT(19), 0x01);
			odm_set_rf_regbu(dm, RF_PATH_A, R_0x42, BIT(19), 0x00);
			odm_set_rf_regbu(dm, RF_PATH_A, R_0x42, BIT(19), 0x01);
			
			odm_set_rf_regbu(dm, RF_PATH_B, R_0x42, BIT(19), 0x01);
			odm_set_rf_regbu(dm, RF_PATH_B, R_0x42, BIT(19), 0x00);
			odm_set_rf_regbu(dm, RF_PATH_B, R_0x42, BIT(19), 0x01);
		} else if (dm->support_ic_type & ODM_RTL8814B) {
			odm_set_rf_regbu(dm, RF_PATH_A, 0x42, BIT(17), 0x1);
			odm_set_rf_regbu(dm, RF_PATH_B, 0x42, BIT(17), 0x1);
			odm_set_rf_regbu(dm, RF_PATH_C, 0x42, BIT(17), 0x1);
			odm_set_rf_regbu(dm, RF_PATH_D, 0x42, BIT(17), 0x1);
		} else
			odm_set_rf_regbu(dm, RF_PATH_A, RF_T_METER_OLD,
				       RFREGOFFSETMASK, 0x60);

#if (RTL8814B_SUPPORT == 1)
		if (dm->support_ic_type & ODM_RTL8814B) {
			ODM_delay_usbu(300);
			odm_txpowertracking_new_callback_thermal_meter(dm);
			tssi->thermal_trigger = 1;
		}
#endif
		dm->rf_calibrate_info.tm_trigger = 1;
		return;
	}
	
	if (dm->support_ic_type & (ODM_RTL8822C | ODM_RTL8814B)) {
#if (RTL8822C_SUPPORT == 1 || RTL8814B_SUPPORT == 1)
		odm_txpowertracking_new_callback_thermal_meter(dm);
		if (dm->support_ic_type & ODM_RTL8814B)
			tssi->thermal_trigger = 0;
#endif
	} else
		odm_txpowertracking_callback_thermal_meterbu(dm);
	dm->rf_calibrate_info.tm_trigger = 0;
#endif
}

void
odm_txpowertracking_direct_cebu(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;

#if (DM_ODM_SUPPORT_TYPE == ODM_CE)

	if (!(rf->rf_supportability & HAL_RF_TX_PWR_TRACK))
		return;
	if (dm->support_ic_type & ODM_RTL8723F) {
#if (RTL8723F_SUPPORT == 1)
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF]===>%s 8723F TSSI, return!\n", __func__);*/
		return;
#endif
	}

	if (dm->support_ic_type & ODM_RTL8822C) {
		/*halrf_tssi_cckbu(dm);*/
		/*halrf_thermal_cckbu(dm);*/
		return;
	}

	if (dm->support_ic_type &
		(ODM_RTL8188E | ODM_RTL8188F | ODM_RTL8192E |
		ODM_RTL8723B | ODM_RTL8812 | ODM_RTL8821 |
		ODM_RTL8814A | ODM_RTL8703B | ODM_RTL8723D |
		ODM_RTL8822B | ODM_RTL8821C | ODM_RTL8710B |
		ODM_RTL8192F | ODM_RTL8814B))
		odm_set_rf_regbu(dm, RF_PATH_A, RF_T_METER_NEW, (BIT(17) | BIT(16)), 0x03);
	else if (dm->support_ic_type & ODM_RTL8822C) {
		odm_set_rf_regbu(dm, RF_PATH_A, R_0x42, BIT(19), 0x01);
		odm_set_rf_regbu(dm, RF_PATH_A, R_0x42, BIT(19), 0x00);
		odm_set_rf_regbu(dm, RF_PATH_A, R_0x42, BIT(19), 0x01);
			
		odm_set_rf_regbu(dm, RF_PATH_B, R_0x42, BIT(19), 0x01);
		odm_set_rf_regbu(dm, RF_PATH_B, R_0x42, BIT(19), 0x00);
		odm_set_rf_regbu(dm, RF_PATH_B, R_0x42, BIT(19), 0x01);
	} else
		odm_set_rf_regbu(dm, RF_PATH_A, RF_T_METER_OLD, RFREGOFFSETMASK, 0x60);

	if (dm->support_ic_type & (ODM_RTL8822C | ODM_RTL8814B)) {
#if (RTL8822C_SUPPORT == 1 || RTL8814B_SUPPORT == 1)
		odm_txpowertracking_new_callback_thermal_meter(dm);
#endif
	} else
		odm_txpowertracking_callback_thermal_meterbu(dm);
#endif

}


void odm_txpowertracking_checkbu_mp(void *dm_void)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	void *adapter = dm->adapter;

	if (odm_check_power_statusbu(adapter) == false) {
		RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,
			 ("check_pow_status, return false\n"));
		return;
	}

	odm_txpowertracking_thermal_meter_check(adapter);
#endif
}

void odm_txpowertracking_checkbu_ap(void *dm_void)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_AP)
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct rtl8192cd_priv *priv = dm->priv;

	return;

#endif
}
