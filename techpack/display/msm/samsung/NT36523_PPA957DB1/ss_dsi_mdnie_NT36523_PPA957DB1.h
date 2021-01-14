/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: wu.deng
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2017, Samsung Electronics. All rights reserved.

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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
*/
#ifndef _SS_DSI_MDNIE_NT36523_PPA957DB1_H_
#define _SS_DSI_MDNIE_NT36523_PPA957DB1_H_

#include "ss_dsi_mdnie_lite_common.h"




/* ----------------- Bypass ----------------- */

static char DSI0_BYPASS_MDNIE_1[] = {
	0xFF,
	0x22, // command 2, page 2
};

static char DSI0_BYPASS_MDNIE_1_2[] = {
	0xFB,
	0x01,
};

static char DSI0_BYPASS_MDNIE_2[] = {
	0x00,
	0x00, // VIVID_C2_01
};

static char DSI0_BYPASS_MDNIE_3[] = {
	0x01,
	0x00, // VIVID_C2_02
};

static char DSI0_BYPASS_MDNIE_4[] = {
	0x02,
	0x00, // VIVID_C2_03
};

static char DSI0_BYPASS_MDNIE_5[] = {
	0x03,
	0x00, // VIVID_C2_04
};

static char DSI0_BYPASS_MDNIE_6[] = {
	0x04,
	0x00, // VIVID_C2_05
};

static char DSI0_BYPASS_MDNIE_7[] = {
	0x05,
	0x00, // VIVID_C2_06
};

static char DSI0_BYPASS_MDNIE_8[] = {
	0x06,
	0x00, // VIVID_C2_07
};

static char DSI0_BYPASS_MDNIE_9[] = {
	0x07,
	0x00, // VIVID_C2_08
};

static char DSI0_BYPASS_MDNIE_10[] = {
	0x08,
	0x00, // VIVID_C2_09
};

static char DSI0_BYPASS_MDNIE_11[] = {
	0x09,
	0x00, // VIVID_C2_10
};

static char DSI0_BYPASS_MDNIE_12[] = {
	0x0A,
	0x00, // VIVID_C2_11
};

static char DSI0_BYPASS_MDNIE_13[] = {
	0x0B,
	0x00, // VIVID_C2_12
};

static char DSI0_BYPASS_MDNIE_14[] = {
	0x0C,
	0x00, // VIVID_C2_13
};

static char DSI0_BYPASS_MDNIE_15[] = {
	0x0D,
	0x00, // VIVID_C2_14
};

static char DSI0_BYPASS_MDNIE_16[] = {
	0x0E,
	0x00, // VIVID_C2_15
};

static char DSI0_BYPASS_MDNIE_17[] = {
	0x0F,
	0x00, // VIVID_C2_16
};

static char DSI0_BYPASS_MDNIE_18[] = {
	0x10,
	0x00, // VIVID_C2_17
};

static char DSI0_BYPASS_MDNIE_19[] = {
	0x11,
	0x00, // VIVID_C2_18
};

static char DSI0_BYPASS_MDNIE_20[] = {
	0x12,
	0x00, // VIVID_C2_19
};

static char DSI0_BYPASS_MDNIE_21[] = {
	0x13,
	0x00, // VIVID_C2_20
};

static char DSI0_BYPASS_MDNIE_22[] = {
	0x1A,
	0x00, // EN_SRE_VIVID EN_VIVID
};

static char DSI0_BYPASS_MDNIE_23[] = {
	0x53,
	0x00, // EN_SRE_SMART EN_SMART
};

static char DSI0_BYPASS_MDNIE_24[] = {
	0x54,
	0x00, // EN_SRE_SKIN_KEEP EN_SKIN_KEEP
};

static char DSI0_BYPASS_MDNIE_25[] = {
	0x55,
	0x00, // EN_SRE_V_constrain EN_V_constrain
};

static char DSI0_BYPASS_MDNIE_26[] = {
	0x56,
	0x00, // EN_SRE_SC EN_SC
};

static char DSI0_BYPASS_MDNIE_27[] = {
	0x5C,
	0x10, // SC_Dark_01
};

static char DSI0_BYPASS_MDNIE_28[] = {
	0x5D,
	0x10, // SC_Light_01
};

static char DSI0_BYPASS_MDNIE_29[] = {
	0x64,
	0x2F, // SC_THD_DN SC_THD_UP
};

static char DSI0_BYPASS_MDNIE_30[] = {
	0x68,
	0x00, // EN_SRE_EDGE EN_EDGE
};

static char DSI0_BYPASS_MDNIE_31[] = {
	0x6A,
	0x00, // IE_DMST_CONTRAST IE_DMST_COLOR
};

static char DSI0_BYPASS_MDNIE_32[] = {
	0x7C,
	0x00, // SRE_SC_BLUR_SEL SC_BLUR_SEL
};

static char DSI0_BYPASS_MDNIE_33[] = {
	0x84,
	0xFF, // SIGN_01~08
};

static char DSI0_BYPASS_MDNIE_34[] = {
	0x85,
	0xFF, // SIGN_09~16
};

static char DSI0_BYPASS_MDNIE_35[] = {
	0x86,
	0x0F, // SIGN_17~20
};

static char DSI0_BYPASS_MDNIE_36[] = {
	0xA2,
	0x00, // EN_Dither
};

static char DSI0_BYPASS_MDNIE_37[] = {
	0xFF,
	0x10, // command 1, page 0
};

static char DSI0_BYPASS_MDNIE_37_2[] = {
	0xFB,
	0x01,
};

static char DSI0_BYPASS_MDNIE_38[] = {
	0x55,
	0x80,
};

static char DSI0_BYPASS_MDNIE_39[] = {
	0xFF,
	0x10,
};

static char DSI0_BYPASS_MDNIE_39_2[] = {
	0xFB,
	0x01,
};




/* ----------------- Dynamic ----------------- */

static char DSI0_UI_DYNAMIC_MDNIE_1[] = {
	0xFF,
	0x22, // command 2, page 2
};

static char DSI0_UI_DYNAMIC_MDNIE_1_2[] = {
	0xFB,
	0x01,
};

static char DSI0_UI_DYNAMIC_MDNIE_2[] = {
	0x00,
	0x00, // VIVID_C2_01
};

static char DSI0_UI_DYNAMIC_MDNIE_3[] = {
	0x01,
	0x00, // VIVID_C2_02
};

static char DSI0_UI_DYNAMIC_MDNIE_4[] = {
	0x02,
	0x00, // VIVID_C2_03
};

static char DSI0_UI_DYNAMIC_MDNIE_5[] = {
	0x03,
	0x00, // VIVID_C2_04
};

static char DSI0_UI_DYNAMIC_MDNIE_6[] = {
	0x04,
	0x00, // VIVID_C2_05
};

static char DSI0_UI_DYNAMIC_MDNIE_7[] = {
	0x05,
	0x00, // VIVID_C2_06
};

static char DSI0_UI_DYNAMIC_MDNIE_8[] = {
	0x06,
	0x00, // VIVID_C2_07
};

static char DSI0_UI_DYNAMIC_MDNIE_9[] = {
	0x07,
	0x00, // VIVID_C2_08
};

static char DSI0_UI_DYNAMIC_MDNIE_10[] = {
	0x08,
	0x00, // VIVID_C2_09
};

static char DSI0_UI_DYNAMIC_MDNIE_11[] = {
	0x09,
	0x00, // VIVID_C2_10
};

static char DSI0_UI_DYNAMIC_MDNIE_12[] = {
	0x0A,
	0x00, // VIVID_C2_11
};

static char DSI0_UI_DYNAMIC_MDNIE_13[] = {
	0x0B,
	0x00, // VIVID_C2_12
};

static char DSI0_UI_DYNAMIC_MDNIE_14[] = {
	0x0C,
	0x00, // VIVID_C2_13
};

static char DSI0_UI_DYNAMIC_MDNIE_15[] = {
	0x0D,
	0x00, // VIVID_C2_14
};

static char DSI0_UI_DYNAMIC_MDNIE_16[] = {
	0x0E,
	0x00, // VIVID_C2_15
};

static char DSI0_UI_DYNAMIC_MDNIE_17[] = {
	0x0F,
	0x00, // VIVID_C2_16
};

static char DSI0_UI_DYNAMIC_MDNIE_18[] = {
	0x10,
	0x00, // VIVID_C2_17
};

static char DSI0_UI_DYNAMIC_MDNIE_19[] = {
	0x11,
	0x00, // VIVID_C2_18
};

static char DSI0_UI_DYNAMIC_MDNIE_20[] = {
	0x12,
	0x00, // VIVID_C2_19
};

static char DSI0_UI_DYNAMIC_MDNIE_21[] = {
	0x13,
	0x00, // VIVID_C2_20
};

static char DSI0_UI_DYNAMIC_MDNIE_22[] = {
	0x1A,
	0x01, // EN_SRE_VIVID EN_VIVID
};

static char DSI0_UI_DYNAMIC_MDNIE_23[] = {
	0x53,
	0x00, // EN_SRE_SMART EN_SMART
};

static char DSI0_UI_DYNAMIC_MDNIE_24[] = {
	0x54,
	0x00, // EN_SRE_SKIN_KEEP EN_SKIN_KEEP
};

static char DSI0_UI_DYNAMIC_MDNIE_25[] = {
	0x55,
	0x00, // EN_SRE_V_constrain EN_V_constrain
};

static char DSI0_UI_DYNAMIC_MDNIE_26[] = {
	0x56,
	0x00, // EN_SRE_SC EN_SC
};

static char DSI0_UI_DYNAMIC_MDNIE_27[] = {
	0x5C,
	0x10, // SC_Dark_01
};

static char DSI0_UI_DYNAMIC_MDNIE_28[] = {
	0x5D,
	0x10, // SC_Light_01
};

static char DSI0_UI_DYNAMIC_MDNIE_29[] = {
	0x64,
	0x2F, // SC_THD_DN SC_THD_UP
};

static char DSI0_UI_DYNAMIC_MDNIE_30[] = {
	0x68,
	0x00, // EN_SRE_EDGE EN_EDGE
};

static char DSI0_UI_DYNAMIC_MDNIE_31[] = {
	0x6A,
	0x02, // IE_DMST_CONTRAST IE_DMST_COLOR
};

static char DSI0_UI_DYNAMIC_MDNIE_32[] = {
	0x7C,
	0x00, // SRE_SC_BLUR_SEL SC_BLUR_SEL
};

static char DSI0_UI_DYNAMIC_MDNIE_33[] = {
	0x84,
	0xFF, // SIGN_01~08
};

static char DSI0_UI_DYNAMIC_MDNIE_34[] = {
	0x85,
	0xFF, // SIGN_09~16
};

static char DSI0_UI_DYNAMIC_MDNIE_35[] = {
	0x86,
	0x0F, // SIGN_17~20
};

static char DSI0_UI_DYNAMIC_MDNIE_36[] = {
	0xA2,
	0x00, // EN_Dither
};

static char DSI0_UI_DYNAMIC_MDNIE_37[] = {
	0xFF,
	0x10, // command 1, page 0
};

static char DSI0_UI_DYNAMIC_MDNIE_37_2[] = {
	0xFB,
	0x01,
};

static char DSI0_UI_DYNAMIC_MDNIE_38[] = {
	0x55,
	0x80,
};

static char DSI0_UI_DYNAMIC_MDNIE_39[] = {
	0xFF,
	0x10,
};

static char DSI0_UI_DYNAMIC_MDNIE_39_2[] = {
	0xFB,
	0x01,
};



/* ----------------- Auto ----------------- */

static char DSI0_UI_AUTO_MDNIE_1[] = {
	0xFF,
	0x22, // command 2, page 2
};

static char DSI0_UI_AUTO_MDNIE_1_2[] = {
	0xFB,
	0x01,
};

static char DSI0_UI_AUTO_MDNIE_2[] = {
	0x00,
	0x00, // VIVID_C2_01
};

static char DSI0_UI_AUTO_MDNIE_3[] = {
	0x01,
	0x00, // VIVID_C2_02
};

static char DSI0_UI_AUTO_MDNIE_4[] = {
	0x02,
	0x00, // VIVID_C2_03
};

static char DSI0_UI_AUTO_MDNIE_5[] = {
	0x03,
	0x00, // VIVID_C2_04
};

static char DSI0_UI_AUTO_MDNIE_6[] = {
	0x04,
	0x00, // VIVID_C2_05
};

static char DSI0_UI_AUTO_MDNIE_7[] = {
	0x05,
	0x00, // VIVID_C2_06
};

static char DSI0_UI_AUTO_MDNIE_8[] = {
	0x06,
	0x00, // VIVID_C2_07
};

static char DSI0_UI_AUTO_MDNIE_9[] = {
	0x07,
	0x00, // VIVID_C2_08
};

static char DSI0_UI_AUTO_MDNIE_10[] = {
	0x08,
	0x00, // VIVID_C2_09
};

static char DSI0_UI_AUTO_MDNIE_11[] = {
	0x09,
	0x00, // VIVID_C2_10
};

static char DSI0_UI_AUTO_MDNIE_12[] = {
	0x0A,
	0x00, // VIVID_C2_11
};

static char DSI0_UI_AUTO_MDNIE_13[] = {
	0x0B,
	0x0C, // VIVID_C2_12
};

static char DSI0_UI_AUTO_MDNIE_14[] = {
	0x0C,
	0x18, // VIVID_C2_13
};

static char DSI0_UI_AUTO_MDNIE_15[] = {
	0x0D,
	0x24, // VIVID_C2_14
};

static char DSI0_UI_AUTO_MDNIE_16[] = {
	0x0E,
	0x30, // VIVID_C2_15
};

static char DSI0_UI_AUTO_MDNIE_17[] = {
	0x0F,
	0x30, // VIVID_C2_16
};

static char DSI0_UI_AUTO_MDNIE_18[] = {
	0x10,
	0x24, // VIVID_C2_17
};

static char DSI0_UI_AUTO_MDNIE_19[] = {
	0x11,
	0x18, // VIVID_C2_18
};

static char DSI0_UI_AUTO_MDNIE_20[] = {
	0x12,
	0x0C, // VIVID_C2_19
};

static char DSI0_UI_AUTO_MDNIE_21[] = {
	0x13,
	0x00, // VIVID_C2_20
};

static char DSI0_UI_AUTO_MDNIE_22[] = {
	0x1A,
	0x01, // EN_SRE_VIVID EN_VIVID
};

static char DSI0_UI_AUTO_MDNIE_23[] = {
	0x53,
	0x00, // EN_SRE_SMART EN_SMART
};

static char DSI0_UI_AUTO_MDNIE_24[] = {
	0x54,
	0x00, // EN_SRE_SKIN_KEEP EN_SKIN_KEEP
};

static char DSI0_UI_AUTO_MDNIE_25[] = {
	0x55,
	0x00, // EN_SRE_V_constrain EN_V_constrain
};

static char DSI0_UI_AUTO_MDNIE_26[] = {
	0x56,
	0x00, // EN_SRE_SC EN_SC
};

static char DSI0_UI_AUTO_MDNIE_27[] = {
	0x5C,
	0x10, // SC_Dark_01
};

static char DSI0_UI_AUTO_MDNIE_28[] = {
	0x5D,
	0x10, // SC_Light_01
};

static char DSI0_UI_AUTO_MDNIE_29[] = {
	0x64,
	0x2F, // SC_THD_DN SC_THD_UP
};

static char DSI0_UI_AUTO_MDNIE_30[] = {
	0x68,
	0x00, // EN_SRE_EDGE EN_EDGE
};

static char DSI0_UI_AUTO_MDNIE_31[] = {
	0x6A,
	0x02, // IE_DMST_CONTRAST IE_DMST_COLOR
};

static char DSI0_UI_AUTO_MDNIE_32[] = {
	0x7C,
	0x00, // SRE_SC_BLUR_SEL SC_BLUR_SEL
};

static char DSI0_UI_AUTO_MDNIE_33[] = {
	0x84,
	0xFF, // SIGN_01~08
};

static char DSI0_UI_AUTO_MDNIE_34[] = {
	0x85,
	0xFF, // SIGN_09~16
};

static char DSI0_UI_AUTO_MDNIE_35[] = {
	0x86,
	0x0F, // SIGN_17~20
};

static char DSI0_UI_AUTO_MDNIE_36[] = {
	0xA2,
	0x00, // EN_Dither
};

static char DSI0_UI_AUTO_MDNIE_37[] = {
	0xFF,
	0x10, // command 1, page 0
};

static char DSI0_UI_AUTO_MDNIE_37_2[] = {
	0xFB,
	0x01,
};

static char DSI0_UI_AUTO_MDNIE_38[] = {
	0x55,
	0x80,
};

static char DSI0_UI_AUTO_MDNIE_39[] = {
	0xFF,
	0x10,
};

static char DSI0_UI_AUTO_MDNIE_39_2[] = {
	0xFB,
	0x01,
};



/* ----------------- HBM_CE ----------------- */

static char DSI0_HBM_CE_MDNIE_1[] = {
	0xFF,
	0x22, // command 2, page 2
};

static char DSI0_HBM_CE_MDNIE_1_2[] = {
	0xFB,
	0x01,
};

static char DSI0_HBM_CE_MDNIE_2[] = {
	0x00,
	0x00, // VIVID_C2_01
};

static char DSI0_HBM_CE_MDNIE_3[] = {
	0x01,
	0x00, // VIVID_C2_02
};

static char DSI0_HBM_CE_MDNIE_4[] = {
	0x02,
	0x00, // VIVID_C2_03
};

static char DSI0_HBM_CE_MDNIE_5[] = {
	0x03,
	0x00, // VIVID_C2_04
};

static char DSI0_HBM_CE_MDNIE_6[] = {
	0x04,
	0x00, // VIVID_C2_05
};

static char DSI0_HBM_CE_MDNIE_7[] = {
	0x05,
	0x00, // VIVID_C2_06
};

static char DSI0_HBM_CE_MDNIE_8[] = {
	0x06,
	0x00, // VIVID_C2_07
};

static char DSI0_HBM_CE_MDNIE_9[] = {
	0x07,
	0x00, // VIVID_C2_08
};

static char DSI0_HBM_CE_MDNIE_10[] = {
	0x08,
	0x00, // VIVID_C2_09
};

static char DSI0_HBM_CE_MDNIE_11[] = {
	0x09,
	0x00, // VIVID_C2_10
};

static char DSI0_HBM_CE_MDNIE_12[] = {
	0x0A,
	0x00, // VIVID_C2_11
};

static char DSI0_HBM_CE_MDNIE_13[] = {
	0x0B,
	0x0C, // VIVID_C2_12
};

static char DSI0_HBM_CE_MDNIE_14[] = {
	0x0C,
	0x18, // VIVID_C2_13
};

static char DSI0_HBM_CE_MDNIE_15[] = {
	0x0D,
	0x24, // VIVID_C2_14
};

static char DSI0_HBM_CE_MDNIE_16[] = {
	0x0E,
	0x30, // VIVID_C2_15
};

static char DSI0_HBM_CE_MDNIE_17[] = {
	0x0F,
	0x30, // VIVID_C2_16
};

static char DSI0_HBM_CE_MDNIE_18[] = {
	0x10,
	0x24, // VIVID_C2_17
};

static char DSI0_HBM_CE_MDNIE_19[] = {
	0x11,
	0x18, // VIVID_C2_18
};

static char DSI0_HBM_CE_MDNIE_20[] = {
	0x12,
	0x0C, // VIVID_C2_19
};

static char DSI0_HBM_CE_MDNIE_21[] = {
	0x13,
	0x00, // VIVID_C2_20
};

static char DSI0_HBM_CE_MDNIE_22[] = {
	0x1A,
	0x01, // EN_SRE_VIVID EN_VIVID
};

static char DSI0_HBM_CE_MDNIE_23[] = {
	0x53,
	0x00, // EN_SRE_SMART EN_SMART
};

static char DSI0_HBM_CE_MDNIE_24[] = {
	0x54,
	0x00, // EN_SRE_SKIN_KEEP EN_SKIN_KEEP
};

static char DSI0_HBM_CE_MDNIE_25[] = {
	0x55,
	0x00, // EN_SRE_V_constrain EN_V_constrain
};

static char DSI0_HBM_CE_MDNIE_26[] = {
	0x56,
	0x00, // EN_SRE_SC EN_SC
};

static char DSI0_HBM_CE_MDNIE_27[] = {
	0x5C,
	0x10, // SC_Dark_01
};

static char DSI0_HBM_CE_MDNIE_28[] = {
	0x5D,
	0x10, // SC_Light_01
};

static char DSI0_HBM_CE_MDNIE_29[] = {
	0x64,
	0x2F, // SC_THD_DN SC_THD_UP
};

static char DSI0_HBM_CE_MDNIE_30[] = {
	0x68,
	0x00, // EN_SRE_EDGE EN_EDGE
};

static char DSI0_HBM_CE_MDNIE_31[] = {
	0x6A,
	0x02, // IE_DMST_CONTRAST IE_DMST_COLOR
};

static char DSI0_HBM_CE_MDNIE_32[] = {
	0x7C,
	0x00, // SRE_SC_BLUR_SEL SC_BLUR_SEL
};

static char DSI0_HBM_CE_MDNIE_33[] = {
	0x84,
	0xFF, // SIGN_01~08
};

static char DSI0_HBM_CE_MDNIE_34[] = {
	0x85,
	0xFF, // SIGN_09~16
};

static char DSI0_HBM_CE_MDNIE_35[] = {
	0x86,
	0x0F, // SIGN_17~20
};

static char DSI0_HBM_CE_MDNIE_36[] = {
	0xA2,
	0x00, // EN_Dither
};

static char DSI0_HBM_CE_MDNIE_37[] = {
	0xFF,
	0x10, // command 1, page 0
};

static char DSI0_HBM_CE_MDNIE_37_2[] = {
	0xFB,
	0x01,
};

static char DSI0_HBM_CE_MDNIE_38[] = {
	0x55,
	0x80,
};

static char DSI0_HBM_CE_MDNIE_39[] = {
	0xFF,
	0x10,
};

static char DSI0_HBM_CE_MDNIE_39_2[] = {
	0xFB,
	0x01,
};





/* ----------------- HBM_CE_D65 ----------------- */

static char DSI0_HBM_CE_D65_MDNIE_1[] = {
	0xFF,
	0x22, // command 2, page 2
};

static char DSI0_HBM_CE_D65_MDNIE_1_2[] = {
	0xFB,
	0x01,
};

static char DSI0_HBM_CE_D65_MDNIE_2[] = {
	0x00,
	0x00, // VIVID_C2_01
};

static char DSI0_HBM_CE_D65_MDNIE_3[] = {
	0x01,
	0x00, // VIVID_C2_02
};

static char DSI0_HBM_CE_D65_MDNIE_4[] = {
	0x02,
	0x00, // VIVID_C2_03
};

static char DSI0_HBM_CE_D65_MDNIE_5[] = {
	0x03,
	0x00, // VIVID_C2_04
};

static char DSI0_HBM_CE_D65_MDNIE_6[] = {
	0x04,
	0x00, // VIVID_C2_05
};

static char DSI0_HBM_CE_D65_MDNIE_7[] = {
	0x05,
	0x00, // VIVID_C2_06
};

static char DSI0_HBM_CE_D65_MDNIE_8[] = {
	0x06,
	0x00, // VIVID_C2_07
};

static char DSI0_HBM_CE_D65_MDNIE_9[] = {
	0x07,
	0x00, // VIVID_C2_08
};

static char DSI0_HBM_CE_D65_MDNIE_10[] = {
	0x08,
	0x00, // VIVID_C2_09
};

static char DSI0_HBM_CE_D65_MDNIE_11[] = {
	0x09,
	0x00, // VIVID_C2_10
};

static char DSI0_HBM_CE_D65_MDNIE_12[] = {
	0x0A,
	0x00, // VIVID_C2_11
};

static char DSI0_HBM_CE_D65_MDNIE_13[] = {
	0x0B,
	0x00, // VIVID_C2_12
};

static char DSI0_HBM_CE_D65_MDNIE_14[] = {
	0x0C,
	0x00, // VIVID_C2_13
};

static char DSI0_HBM_CE_D65_MDNIE_15[] = {
	0x0D,
	0x00, // VIVID_C2_14
};

static char DSI0_HBM_CE_D65_MDNIE_16[] = {
	0x0E,
	0x00, // VIVID_C2_15
};

static char DSI0_HBM_CE_D65_MDNIE_17[] = {
	0x0F,
	0x00, // VIVID_C2_16
};

static char DSI0_HBM_CE_D65_MDNIE_18[] = {
	0x10,
	0x00, // VIVID_C2_17
};

static char DSI0_HBM_CE_D65_MDNIE_19[] = {
	0x11,
	0x00, // VIVID_C2_18
};

static char DSI0_HBM_CE_D65_MDNIE_20[] = {
	0x12,
	0x00, // VIVID_C2_19
};

static char DSI0_HBM_CE_D65_MDNIE_21[] = {
	0x13,
	0x00, // VIVID_C2_20
};

static char DSI0_HBM_CE_D65_MDNIE_22[] = {
	0x1A,
	0x01, // EN_SRE_VIVID EN_VIVID
};

static char DSI0_HBM_CE_D65_MDNIE_23[] = {
	0x53,
	0x00, // EN_SRE_SMART EN_SMART
};

static char DSI0_HBM_CE_D65_MDNIE_24[] = {
	0x54,
	0x00, // EN_SRE_SKIN_KEEP EN_SKIN_KEEP
};

static char DSI0_HBM_CE_D65_MDNIE_25[] = {
	0x55,
	0x00, // EN_SRE_V_constrain EN_V_constrain
};

static char DSI0_HBM_CE_D65_MDNIE_26[] = {
	0x56,
	0x00, // EN_SRE_SC EN_SC
};

static char DSI0_HBM_CE_D65_MDNIE_27[] = {
	0x5C,
	0x10, // SC_Dark_01
};

static char DSI0_HBM_CE_D65_MDNIE_28[] = {
	0x5D,
	0x10, // SC_Light_01
};

static char DSI0_HBM_CE_D65_MDNIE_29[] = {
	0x64,
	0x2F, // SC_THD_DN SC_THD_UP
};

static char DSI0_HBM_CE_D65_MDNIE_30[] = {
	0x68,
	0x00, // EN_SRE_EDGE EN_EDGE
};

static char DSI0_HBM_CE_D65_MDNIE_31[] = {
	0x6A,
	0x02, // IE_DMST_CONTRAST IE_DMST_COLOR
};

static char DSI0_HBM_CE_D65_MDNIE_32[] = {
	0x7C,
	0x00, // SRE_SC_BLUR_SEL SC_BLUR_SEL
};

static char DSI0_HBM_CE_D65_MDNIE_33[] = {
	0x84,
	0xFF, // SIGN_01~08
};

static char DSI0_HBM_CE_D65_MDNIE_34[] = {
	0x85,
	0xFF, // SIGN_09~16
};

static char DSI0_HBM_CE_D65_MDNIE_35[] = {
	0x86,
	0x0F, // SIGN_17~20
};

static char DSI0_HBM_CE_D65_MDNIE_36[] = {
	0xA2,
	0x00, // EN_Dither
};

static char DSI0_HBM_CE_D65_MDNIE_37[] = {
	0xFF,
	0x10, // command 1, page 0
};

static char DSI0_HBM_CE_D65_MDNIE_37_2[] = {
	0xFB,
	0x01,
};

static char DSI0_HBM_CE_D65_MDNIE_38[] = {
	0x55,
	0x80,
};

static char DSI0_HBM_CE_D65_MDNIE_39[] = {
	0xFF,
	0x10,
};

static char DSI0_HBM_CE_D65_MDNIE_39_2[] = {
	0xFB,
	0x01,
};




static struct dsi_cmd_desc DSI0_BYPASS_MDNIE[] = {
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_1), DSI0_BYPASS_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_1), DSI0_BYPASS_MDNIE_1_2, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_22), DSI0_BYPASS_MDNIE_22, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_23), DSI0_BYPASS_MDNIE_23, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_24), DSI0_BYPASS_MDNIE_24, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_25), DSI0_BYPASS_MDNIE_25, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_26), DSI0_BYPASS_MDNIE_26, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_2), DSI0_BYPASS_MDNIE_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_3), DSI0_BYPASS_MDNIE_3, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_4), DSI0_BYPASS_MDNIE_4, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_5), DSI0_BYPASS_MDNIE_5, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_6), DSI0_BYPASS_MDNIE_6, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_7), DSI0_BYPASS_MDNIE_7, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_8), DSI0_BYPASS_MDNIE_8, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_9), DSI0_BYPASS_MDNIE_9, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_10), DSI0_BYPASS_MDNIE_10, 0, NULL}, false, 0},
	
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_11), DSI0_BYPASS_MDNIE_11, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_12), DSI0_BYPASS_MDNIE_12, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_13), DSI0_BYPASS_MDNIE_13, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_14), DSI0_BYPASS_MDNIE_14, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_15), DSI0_BYPASS_MDNIE_15, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_16), DSI0_BYPASS_MDNIE_16, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_17), DSI0_BYPASS_MDNIE_17, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_18), DSI0_BYPASS_MDNIE_18, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_19), DSI0_BYPASS_MDNIE_19, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_20), DSI0_BYPASS_MDNIE_20, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_21), DSI0_BYPASS_MDNIE_21, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_27), DSI0_BYPASS_MDNIE_27, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_28), DSI0_BYPASS_MDNIE_28, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_29), DSI0_BYPASS_MDNIE_29, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_30), DSI0_BYPASS_MDNIE_30, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_31), DSI0_BYPASS_MDNIE_31, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_32), DSI0_BYPASS_MDNIE_32, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_33), DSI0_BYPASS_MDNIE_33, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_34), DSI0_BYPASS_MDNIE_34, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_35), DSI0_BYPASS_MDNIE_35, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_36), DSI0_BYPASS_MDNIE_36, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_37), DSI0_BYPASS_MDNIE_37, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_37), DSI0_BYPASS_MDNIE_37_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_38), DSI0_BYPASS_MDNIE_38, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_39), DSI0_BYPASS_MDNIE_39, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_BYPASS_MDNIE_39), DSI0_BYPASS_MDNIE_39_2, 0, NULL}, false, 0},
};


static struct dsi_cmd_desc DSI0_UI_DYNAMIC_MDNIE[] = {
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_1), DSI0_UI_DYNAMIC_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_1), DSI0_UI_DYNAMIC_MDNIE_1_2, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_22), DSI0_UI_DYNAMIC_MDNIE_22, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_23), DSI0_UI_DYNAMIC_MDNIE_23, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_24), DSI0_UI_DYNAMIC_MDNIE_24, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_25), DSI0_UI_DYNAMIC_MDNIE_25, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_26), DSI0_UI_DYNAMIC_MDNIE_26, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_2), DSI0_UI_DYNAMIC_MDNIE_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_3), DSI0_UI_DYNAMIC_MDNIE_3, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_4), DSI0_UI_DYNAMIC_MDNIE_4, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_5), DSI0_UI_DYNAMIC_MDNIE_5, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_6), DSI0_UI_DYNAMIC_MDNIE_6, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_7), DSI0_UI_DYNAMIC_MDNIE_7, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_8), DSI0_UI_DYNAMIC_MDNIE_8, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_9), DSI0_UI_DYNAMIC_MDNIE_9, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_10), DSI0_UI_DYNAMIC_MDNIE_10, 0, NULL}, false, 0},
	
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_11), DSI0_UI_DYNAMIC_MDNIE_11, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_12), DSI0_UI_DYNAMIC_MDNIE_12, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_13), DSI0_UI_DYNAMIC_MDNIE_13, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_14), DSI0_UI_DYNAMIC_MDNIE_14, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_15), DSI0_UI_DYNAMIC_MDNIE_15, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_16), DSI0_UI_DYNAMIC_MDNIE_16, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_17), DSI0_UI_DYNAMIC_MDNIE_17, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_18), DSI0_UI_DYNAMIC_MDNIE_18, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_19), DSI0_UI_DYNAMIC_MDNIE_19, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_20), DSI0_UI_DYNAMIC_MDNIE_20, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_21), DSI0_UI_DYNAMIC_MDNIE_21, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_27), DSI0_UI_DYNAMIC_MDNIE_27, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_28), DSI0_UI_DYNAMIC_MDNIE_28, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_29), DSI0_UI_DYNAMIC_MDNIE_29, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_30), DSI0_UI_DYNAMIC_MDNIE_30, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_31), DSI0_UI_DYNAMIC_MDNIE_31, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_32), DSI0_UI_DYNAMIC_MDNIE_32, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_33), DSI0_UI_DYNAMIC_MDNIE_33, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_34), DSI0_UI_DYNAMIC_MDNIE_34, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_35), DSI0_UI_DYNAMIC_MDNIE_35, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_36), DSI0_UI_DYNAMIC_MDNIE_36, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_37), DSI0_UI_DYNAMIC_MDNIE_37, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_37), DSI0_UI_DYNAMIC_MDNIE_37_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_38), DSI0_UI_DYNAMIC_MDNIE_38, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_39), DSI0_UI_DYNAMIC_MDNIE_39, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_DYNAMIC_MDNIE_39), DSI0_UI_DYNAMIC_MDNIE_39_2, 0, NULL}, false, 0},
};


static struct dsi_cmd_desc DSI0_UI_AUTO_MDNIE[] = {
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_1), DSI0_UI_AUTO_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_1), DSI0_UI_AUTO_MDNIE_1_2, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_22), DSI0_UI_AUTO_MDNIE_22, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_23), DSI0_UI_AUTO_MDNIE_23, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_24), DSI0_UI_AUTO_MDNIE_24, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_25), DSI0_UI_AUTO_MDNIE_25, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_26), DSI0_UI_AUTO_MDNIE_26, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_2), DSI0_UI_AUTO_MDNIE_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_3), DSI0_UI_AUTO_MDNIE_3, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_4), DSI0_UI_AUTO_MDNIE_4, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_5), DSI0_UI_AUTO_MDNIE_5, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_6), DSI0_UI_AUTO_MDNIE_6, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_7), DSI0_UI_AUTO_MDNIE_7, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_8), DSI0_UI_AUTO_MDNIE_8, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_9), DSI0_UI_AUTO_MDNIE_9, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_10), DSI0_UI_AUTO_MDNIE_10, 0, NULL}, false, 0},
	
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_11), DSI0_UI_AUTO_MDNIE_11, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_12), DSI0_UI_AUTO_MDNIE_12, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_13), DSI0_UI_AUTO_MDNIE_13, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_14), DSI0_UI_AUTO_MDNIE_14, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_15), DSI0_UI_AUTO_MDNIE_15, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_16), DSI0_UI_AUTO_MDNIE_16, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_17), DSI0_UI_AUTO_MDNIE_17, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_18), DSI0_UI_AUTO_MDNIE_18, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_19), DSI0_UI_AUTO_MDNIE_19, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_20), DSI0_UI_AUTO_MDNIE_20, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_21), DSI0_UI_AUTO_MDNIE_21, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_27), DSI0_UI_AUTO_MDNIE_27, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_28), DSI0_UI_AUTO_MDNIE_28, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_29), DSI0_UI_AUTO_MDNIE_29, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_30), DSI0_UI_AUTO_MDNIE_30, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_31), DSI0_UI_AUTO_MDNIE_31, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_32), DSI0_UI_AUTO_MDNIE_32, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_33), DSI0_UI_AUTO_MDNIE_33, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_34), DSI0_UI_AUTO_MDNIE_34, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_35), DSI0_UI_AUTO_MDNIE_35, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_36), DSI0_UI_AUTO_MDNIE_36, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_37), DSI0_UI_AUTO_MDNIE_37, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_37), DSI0_UI_AUTO_MDNIE_37_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_38), DSI0_UI_AUTO_MDNIE_38, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_39), DSI0_UI_AUTO_MDNIE_39, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_UI_AUTO_MDNIE_39), DSI0_UI_AUTO_MDNIE_39_2, 0, NULL}, false, 0},
};


static struct dsi_cmd_desc DSI0_HBM_CE_MDNIE[] = {
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_1), DSI0_HBM_CE_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_1), DSI0_HBM_CE_MDNIE_1_2, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_22), DSI0_HBM_CE_MDNIE_22, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_23), DSI0_HBM_CE_MDNIE_23, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_24), DSI0_HBM_CE_MDNIE_24, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_25), DSI0_HBM_CE_MDNIE_25, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_26), DSI0_HBM_CE_MDNIE_26, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_2), DSI0_HBM_CE_MDNIE_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_3), DSI0_HBM_CE_MDNIE_3, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_4), DSI0_HBM_CE_MDNIE_4, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_5), DSI0_HBM_CE_MDNIE_5, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_6), DSI0_HBM_CE_MDNIE_6, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_7), DSI0_HBM_CE_MDNIE_7, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_8), DSI0_HBM_CE_MDNIE_8, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_9), DSI0_HBM_CE_MDNIE_9, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_10), DSI0_HBM_CE_MDNIE_10, 0, NULL}, false, 0},
	
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_11), DSI0_HBM_CE_MDNIE_11, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_12), DSI0_HBM_CE_MDNIE_12, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_13), DSI0_HBM_CE_MDNIE_13, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_14), DSI0_HBM_CE_MDNIE_14, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_15), DSI0_HBM_CE_MDNIE_15, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_16), DSI0_HBM_CE_MDNIE_16, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_17), DSI0_HBM_CE_MDNIE_17, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_18), DSI0_HBM_CE_MDNIE_18, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_19), DSI0_HBM_CE_MDNIE_19, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_20), DSI0_HBM_CE_MDNIE_20, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_21), DSI0_HBM_CE_MDNIE_21, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_27), DSI0_HBM_CE_MDNIE_27, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_28), DSI0_HBM_CE_MDNIE_28, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_29), DSI0_HBM_CE_MDNIE_29, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_30), DSI0_HBM_CE_MDNIE_30, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_31), DSI0_HBM_CE_MDNIE_31, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_32), DSI0_HBM_CE_MDNIE_32, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_33), DSI0_HBM_CE_MDNIE_33, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_34), DSI0_HBM_CE_MDNIE_34, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_35), DSI0_HBM_CE_MDNIE_35, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_36), DSI0_HBM_CE_MDNIE_36, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_37), DSI0_HBM_CE_MDNIE_37, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_37), DSI0_HBM_CE_MDNIE_37_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_38), DSI0_HBM_CE_MDNIE_38, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_39), DSI0_HBM_CE_MDNIE_39, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_39), DSI0_HBM_CE_MDNIE_39_2, 0, NULL}, false, 0},
};


static struct dsi_cmd_desc DSI0_HBM_CE_D65_MDNIE[] = {
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_1), DSI0_HBM_CE_D65_MDNIE_1, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_1), DSI0_HBM_CE_D65_MDNIE_1_2, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_22), DSI0_HBM_CE_D65_MDNIE_22, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_23), DSI0_HBM_CE_D65_MDNIE_23, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_24), DSI0_HBM_CE_D65_MDNIE_24, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_25), DSI0_HBM_CE_D65_MDNIE_25, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_26), DSI0_HBM_CE_D65_MDNIE_26, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_2), DSI0_HBM_CE_D65_MDNIE_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_3), DSI0_HBM_CE_D65_MDNIE_3, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_4), DSI0_HBM_CE_D65_MDNIE_4, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_5), DSI0_HBM_CE_D65_MDNIE_5, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_6), DSI0_HBM_CE_D65_MDNIE_6, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_7), DSI0_HBM_CE_D65_MDNIE_7, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_8), DSI0_HBM_CE_D65_MDNIE_8, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_9), DSI0_HBM_CE_D65_MDNIE_9, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_10), DSI0_HBM_CE_D65_MDNIE_10, 0, NULL}, false, 0},
	
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_11), DSI0_HBM_CE_D65_MDNIE_11, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_12), DSI0_HBM_CE_D65_MDNIE_12, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_13), DSI0_HBM_CE_D65_MDNIE_13, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_14), DSI0_HBM_CE_D65_MDNIE_14, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_15), DSI0_HBM_CE_D65_MDNIE_15, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_16), DSI0_HBM_CE_D65_MDNIE_16, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_17), DSI0_HBM_CE_D65_MDNIE_17, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_18), DSI0_HBM_CE_D65_MDNIE_18, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_19), DSI0_HBM_CE_D65_MDNIE_19, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_20), DSI0_HBM_CE_D65_MDNIE_20, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_21), DSI0_HBM_CE_D65_MDNIE_21, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_27), DSI0_HBM_CE_D65_MDNIE_27, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_28), DSI0_HBM_CE_D65_MDNIE_28, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_29), DSI0_HBM_CE_D65_MDNIE_29, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_30), DSI0_HBM_CE_D65_MDNIE_30, 0, NULL}, false, 0},

	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_31), DSI0_HBM_CE_D65_MDNIE_31, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_32), DSI0_HBM_CE_D65_MDNIE_32, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_33), DSI0_HBM_CE_D65_MDNIE_33, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_34), DSI0_HBM_CE_D65_MDNIE_34, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_35), DSI0_HBM_CE_D65_MDNIE_35, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_36), DSI0_HBM_CE_D65_MDNIE_36, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_37), DSI0_HBM_CE_D65_MDNIE_37, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_37), DSI0_HBM_CE_D65_MDNIE_37_2, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_38), DSI0_HBM_CE_D65_MDNIE_38, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_39), DSI0_HBM_CE_D65_MDNIE_39, 0, NULL}, false, 0},
	{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, 0, sizeof(DSI0_HBM_CE_D65_MDNIE_39), DSI0_HBM_CE_D65_MDNIE_39_2, 0, NULL}, false, 0},
};



static struct dsi_cmd_desc *mdnie_tune_value_dsi0[MAX_APP_MODE][MAX_MODE][MAX_OUTDOOR_MODE] = {
                /*
			DYNAMIC_MODE
			STANDARD_MODE
			NATURAL_MODE
			MOVIE_MODE
			AUTO_MODE
			READING_MODE
		*/
		// UI_APP
		{
			{DSI0_UI_DYNAMIC_MDNIE,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{DSI0_UI_AUTO_MDNIE,	NULL},
			{NULL,	NULL},
		},
		// VIDEO_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// VIDEO_WARM_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// VIDEO_COLD_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// CAMERA_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// NAVI_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// GALLERY_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// VT_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// BROWSER_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// eBOOK_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// EMAIL_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// TDMB_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
};


#endif
