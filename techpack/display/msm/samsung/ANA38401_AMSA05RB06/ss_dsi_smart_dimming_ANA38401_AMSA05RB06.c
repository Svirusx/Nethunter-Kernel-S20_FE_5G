/*
 * =================================================================
 *
 *
 *	Description:  samsung display panel file
 *
 *	Author: jb09.kim
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

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
#include "ss_dsi_smart_dimming_ANA38401_AMSA05RB06.h"
/*#define SMART_DIMMING_DEBUG*/

static char max_lux_table[GAMMA_SET_MAX];

static char max_lux_table2[GAMMA_SET_MAX] = {
	0x80, 0x80,	/* V11[7:0],  V23[7:0]	Red */
	0x80, 0x80,	/* V35[7:0],  V51[7:0]	Red */
	0x80, 0x80,	/* V87[7:0],  V151[7:0]	Red */
	0x80, 0x00,	/* V203[7:0], V255[7:0]	Red */
	0x80, 0x80,	/* V3[7:0], V255[8]:V0[6:0]Red */
	0x00,		/* VT[3:0]		Red */
	0x80, 0x80,	/* V11[7:0],  V23[7:0]	Green */
	0x80, 0x80,	/* V35[7:0],  V51[7:0]	Green */
	0x80, 0x80,	/* V87[7:0],  V151[7:0]	Green */
	0x80, 0x00,	/* V203[7:0], V255[7:0]	Green */
	0x80, 0x80,	/* V3[7:0], V255[8]:V0[6:0]Green */
	0x00,		/* VT[3:0]		Green */
	0x80, 0x80,	/* V11[7:0],  V23[7:0]	Blue */
	0x80, 0x80,	/* V35[7:0],  V51[7:0]	Blue */
	0x80, 0x80,	/* V87[7:0],  V151[7:0]	Blue */
	0x80, 0x00,	/* V203[7:0], V255[7:0]	Blue */
	0x80, 0x80,	/* V3[7:0], V255[8]:V0[6:0]Blue */
	0x00		/* VT[3:0]		Blue */
};

/*
*	To support different center cell gamma setting
*/
static char V255_300CD_R_MSB;
static char V255_300CD_R_LSB;

static char V255_300CD_G_MSB;
static char V255_300CD_G_LSB;

static char V255_300CD_B_MSB;
static char V255_300CD_B_LSB;

static char V203_300CD_R;
static char V203_300CD_G;
static char V203_300CD_B;

static char V151_300CD_R;
static char V151_300CD_G;
static char V151_300CD_B;

static char V87_300CD_R;
static char V87_300CD_G;
static char V87_300CD_B;

static char V51_300CD_R;
static char V51_300CD_G;
static char V51_300CD_B;

static char V35_300CD_R;
static char V35_300CD_G;
static char V35_300CD_B;

static char V23_300CD_R;
static char V23_300CD_G;
static char V23_300CD_B;

static char V11_300CD_R;
static char V11_300CD_G;
static char V11_300CD_B;

static char V3_300CD_R;
static char V3_300CD_G;
static char V3_300CD_B;

static char VT_300CD_R;
static char VT_300CD_G;
static char VT_300CD_B;

static int char_to_int(char data1)
{
	int cal_data;

	if (data1 & 0x80) {
		cal_data = data1 & 0x7F;
		cal_data *= -1;
	} else
		cal_data = data1;

	return cal_data;
}

static int char_to_int_v255(char data1, char data2)
{
	int cal_data;

	if (data1)
		cal_data = data2 * -1;
	else
		cal_data = data2;

	return cal_data;
}

static void print_RGB_offset(struct SMART_DIM *pSmart)
{
	LCD_INFO("MTP Offset VT R:%d G:%d B:%d\n",
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_1),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_1),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_1));
	LCD_INFO("MTP Offset V3 R:%d G:%d B:%d\n",
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_3),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_3),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_3));
	LCD_INFO("MTP Offset V11 R:%d G:%d B:%d\n",
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_11),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_11),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_11));
	LCD_INFO("MTP Offset V23 R:%d G:%d B:%d\n",
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_23),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_23),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_23));
	LCD_INFO("MTP Offset V35 R:%d G:%d B:%d\n",
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35));
	LCD_INFO("MTP Offset V51 R:%d G:%d B:%d\n",
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_51),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_51),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_51));
	LCD_INFO("MTP Offset V87 R:%d G:%d B:%d\n",
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87));
	LCD_INFO("MTP Offset V151 R:%d G:%d B:%d\n",
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_151),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_151),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_151));
	LCD_INFO("MTP Offset V203 R:%d G:%d B:%d\n",
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_203),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_203),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_203));
	LCD_INFO("MTP Offset V255 R:%d G:%d B:%d\n",
			char_to_int_v255(pSmart->MTP.R_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.R_OFFSET.OFFSET_255_LSB),
			char_to_int_v255(pSmart->MTP.G_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.G_OFFSET.OFFSET_255_LSB),
			char_to_int_v255(pSmart->MTP.B_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.B_OFFSET.OFFSET_255_LSB));
}

static void print_lux_table(struct SMART_DIM *psmart, char* type)
{
	int lux_loop;
	int cnt;
	char pBuffer[256];
	memset(pBuffer, 0x00, 256);

	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++) {
			if (!strcmp(type,"DEC"))
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %03d",
					psmart->gen_table[lux_loop].gamma_setting[cnt]);
			else
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x",
				psmart->gen_table[lux_loop].gamma_setting[cnt]);
		}
		LCD_INFO("lux : %3d  %s\n", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
}

static void print_hbm_lux_table(struct SMART_DIM *psmart, char* type)
{
	int i,j;
	char pBuffer[256];

	memset(pBuffer, 0x00, 256);
	for (i = 0; i < HBM_INTERPOLATION_STEP; i++) {
		for (j = 0; j < GAMMA_SET_MAX; j++) {
			if (!strcmp(type,"DEC"))
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d",
						psmart->hbm_interpolation_table[i].gamma_setting[j]);
			else
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x",
					psmart->hbm_interpolation_table[i].gamma_setting[j]);
		}
		LCD_INFO("hbm[%3d]  %s\n", hbm_interpolation_candela_table[i], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
}

static void print_aid_log(struct smartdim_conf *conf)
{
	print_RGB_offset(conf->psmart);

	print_lux_table(conf->psmart, "DEC");
	print_lux_table(conf->psmart, "HEX");

	print_hbm_lux_table(conf->psmart, "DEC");
	print_hbm_lux_table(conf->psmart, "HEX");
}

#define v255_coefficient 129
#define v255_denominator 640
static int v255_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	unsigned long long add_mtp;
	int LSB;
	int v255_value;

	v255_value = (V255_300CD_R_MSB << 8) | (V255_300CD_R_LSB);
	LSB = char_to_int_v255(pSmart->MTP.R_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.R_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + v255_value;
	result_1 = result_2 = (v255_coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, v255_denominator);
	result_3 = ((ANA34801_VREG0_REF_L-ANA34801_VREG0_REF_H) * result_2) >> BIT_SHIFT;
	result_4 = ANA34801_VREG0_REF_L - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_0 = ANA34801_VREG0_REF_L;

	v255_value = (V255_300CD_G_MSB << 8) | (V255_300CD_G_LSB);
	LSB = char_to_int_v255(pSmart->MTP.G_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.G_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + v255_value;
	result_1 = result_2 = (v255_coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, v255_denominator);
	result_3 = ((ANA34801_VREG0_REF_L-ANA34801_VREG0_REF_H) * result_2) >> BIT_SHIFT;
	result_4 = ANA34801_VREG0_REF_L - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_0 = ANA34801_VREG0_REF_L;

	v255_value = (V255_300CD_B_MSB << 8) | (V255_300CD_B_LSB);
	LSB = char_to_int_v255(pSmart->MTP.B_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.B_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + v255_value;
	result_1 = result_2 = (v255_coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, v255_denominator);
	result_3 = ((ANA34801_VREG0_REF_L-ANA34801_VREG0_REF_H) * result_2) >> BIT_SHIFT;
	result_4 = ANA34801_VREG0_REF_L - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_0 = ANA34801_VREG0_REF_L;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s MTP Offset VT R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_1),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_1),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_1));
	pr_info("%s MTP Offset V3 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_3),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_3),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_3));
	pr_info("%s MTP Offset V11 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_11),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_11),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_11));
	pr_info("%s MTP Offset V23 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_23),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_23),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_23));
	pr_info("%s MTP Offset V35 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35));
	pr_info("%s MTP Offset V51 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_51),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_51),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_51));
	pr_info("%s MTP Offset V87 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87));
	pr_info("%s MTP Offset V151 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_151),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_151),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_151));
	pr_info("%s MTP Offset V203 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_203),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_203),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_203));
	pr_info("%s MTP Offset V255 R:%d G:%d B:%d\n", __func__,
			char_to_int_v255(pSmart->MTP.R_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.R_OFFSET.OFFSET_255_LSB),
			char_to_int_v255(pSmart->MTP.G_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.G_OFFSET.OFFSET_255_LSB),
			char_to_int_v255(pSmart->MTP.B_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.B_OFFSET.OFFSET_255_LSB));

	pr_info("%s V255 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_255,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_255,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_255);
#endif

	return 0;
}

static void v255_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = ANA34801_VREG0_REF_L -
		(pSmart->GRAY.TABLE[index[V255_INDEX]].R_Gray);
	result_2 = result_1 * v255_denominator;
	do_div(result_2, (ANA34801_VREG0_REF_L-ANA34801_VREG0_REF_H));
	result_3 = result_2  - v255_coefficient;
	str[0] = (result_3 & 0xff00) >> 8;
	str[1] = result_3 & 0xff;

	result_1 = ANA34801_VREG0_REF_L -
		(pSmart->GRAY.TABLE[index[V255_INDEX]].G_Gray);
	result_2 = result_1 * v255_denominator;
	do_div(result_2, (ANA34801_VREG0_REF_L-ANA34801_VREG0_REF_H));
	result_3 = result_2  - v255_coefficient;
	str[2] = (result_3 & 0xff00) >> 8;
	str[3] = result_3 & 0xff;

	result_1 = ANA34801_VREG0_REF_L -
			(pSmart->GRAY.TABLE[index[V255_INDEX]].B_Gray);
	result_2 = result_1 * v255_denominator;
	do_div(result_2, (ANA34801_VREG0_REF_L-ANA34801_VREG0_REF_H));
	result_3 = result_2  - v255_coefficient;
	str[4] = (result_3 & 0xff00) >> 8;
	str[5] = result_3 & 0xff;

}

static int vt_coefficient[] = {
	0, 12, 24, 36, 48,
	60, 72, 84, 96, 108,
	138, 148, 158, 168,
	178, 186,
};
#define vt_denominator 600
static int vt_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	unsigned long long add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_1);
	add_mtp = LSB + VT_300CD_R;
	result_1 = result_2 = vt_coefficient[LSB] << BIT_SHIFT;
	do_div(result_2, vt_denominator);
	result_3 = (ANA34801_VREG0_REF_L * result_2) >> BIT_SHIFT;
	result_4 = ANA34801_VREG0_REF_L - result_3;
	pSmart->GRAY.VT_TABLE.R_Gray = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_1);
	add_mtp = LSB + VT_300CD_G;
	result_1 = result_2 = vt_coefficient[LSB] << BIT_SHIFT;
	do_div(result_2, vt_denominator);
	result_3 = (ANA34801_VREG0_REF_L * result_2) >> BIT_SHIFT;
	result_4 = ANA34801_VREG0_REF_L - result_3;
	pSmart->GRAY.VT_TABLE.G_Gray = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_1);
	add_mtp = LSB + VT_300CD_B;
	result_1 = result_2 = vt_coefficient[LSB] << BIT_SHIFT;
	do_div(result_2, vt_denominator);
	result_3 = (ANA34801_VREG0_REF_L * result_2) >> BIT_SHIFT;
	result_4 = ANA34801_VREG0_REF_L - result_3;
	pSmart->GRAY.VT_TABLE.B_Gray = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s VT RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->GRAY.VT_TABLE.R_Gray,
			pSmart->GRAY.VT_TABLE.G_Gray,
			pSmart->GRAY.VT_TABLE.B_Gray);
#endif

	return 0;

}

static void vt_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	str[30] = VT_300CD_R;
	str[31] = VT_300CD_G;
	str[32] = VT_300CD_B;
}

#define v203_coefficient 64
#define v203_denominator 320
static int v203_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	unsigned long long add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_203);
	add_mtp = LSB + V203_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
				- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_255);
	result_2 = (v203_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v203_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_203 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_203);
	add_mtp = LSB + V203_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
				- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_255);
	result_2 = (v203_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v203_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_203 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_203);
	add_mtp = LSB + V203_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
				- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_255);
	result_2 = (v203_coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, v203_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_203 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V203 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_203,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_203,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_203);
#endif

	return 0;

}

static void v203_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].R_Gray);
	result_2 = result_1 * v203_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[6] = (result_2  - v203_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].G_Gray);
	result_2 = result_1 * v203_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[7] = (result_2  - v203_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].B_Gray);
	result_2 = result_1 * v203_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[8] = (result_2  - v203_coefficient) & 0xff;

}

#define v151_coefficient 64
#define v151_denominator 320
static int v151_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	unsigned long long add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_151);
	add_mtp = LSB + V151_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_203);
	result_2 = (v151_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v151_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_151 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_151);
	add_mtp = LSB + V151_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_203);
	result_2 = (v151_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v151_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_151 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_151);
	add_mtp = LSB + V151_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_203);
	result_2 = (v151_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v151_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_151 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V151 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_151,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_151,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_151);
#endif

	return 0;

}

static void v151_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].R_Gray);
	result_2 = result_1 * v151_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[9] = (result_2  - v151_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].G_Gray);
	result_2 = result_1 * v151_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[10] = (result_2  - v151_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].B_Gray);
	result_2 = result_1 * v151_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[11] = (result_2  - v151_coefficient) & 0xff;
}

#define v87_coefficient 64
#define v87_denominator 320
static int v87_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	unsigned long long add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_151);
	result_2 = (v87_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_87 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_151);
	result_2 = (v87_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_87 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_151);
	result_2 = (v87_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_87 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V87 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_87,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_87,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_87);
#endif

	return 0;
}

static void v87_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].R_Gray);
	result_2 = result_1 * v87_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[12] = (result_2  - v87_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].G_Gray);
	result_2 = result_1 * v87_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[13] = (result_2  - v87_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].B_Gray);
	result_2 = result_1 * v87_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[14] = (result_2  - v87_coefficient) & 0xff;
}

#define v51_coefficient 64
#define v51_denominator 320
static int v51_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	unsigned long long add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_51);
	add_mtp = LSB + V51_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_87);
	result_2 = (v51_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v51_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_51 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_51);
	add_mtp = LSB + V51_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_87);
	result_2 = (v51_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v51_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_51 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_51);
	add_mtp = LSB + V51_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_87);
	result_2 = (v51_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v51_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_51 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V51 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_51,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_51,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_51);
#endif

	return 0;
}

static void v51_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].R_Gray);
	result_2 = result_1 * v51_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[15] = (result_2  - v51_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].G_Gray);
	result_2 = result_1 * v51_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[16] = (result_2  - v51_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].B_Gray);
	result_2 = result_1 * v51_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[17] = (result_2  - v51_coefficient) & 0xff;
}

#define v35_coefficient 64
#define v35_denominator 320
static int v35_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	unsigned long long add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35);
	add_mtp = LSB + V35_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_51);
	result_2 = (v35_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v35_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_35 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35);
	add_mtp = LSB + V35_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_51);
	result_2 = (v35_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v35_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_35 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35);
	add_mtp = LSB + V35_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_51);
	result_2 = (v35_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v35_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_35 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V35 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_35,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_35,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_35);
#endif

	return 0;
}

static void v35_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].R_Gray);
	result_2 = result_1 * v35_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[18] = (result_2  - v35_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].G_Gray);
	result_2 = result_1 * v35_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[19] = (result_2  - v35_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].B_Gray);
	result_2 = result_1 * v35_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[20] = (result_2  - v35_coefficient) & 0xff;

}

#define v23_coefficient 64
#define v23_denominator 320
static int v23_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	unsigned long long add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_23);
	add_mtp = LSB + V23_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_35);
	result_2 = (v23_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v23_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_23 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_23);
	add_mtp = LSB + V23_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_35);
	result_2 = (v23_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v23_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_23 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_23);
	add_mtp = LSB + V23_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_35);
	result_2 = (v23_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v23_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_23 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V23 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_23,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_23,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_23);
#endif

	return 0;
}

static void v23_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].R_Gray);
	result_2 = result_1 * v23_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[21] = (result_2  - v23_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].G_Gray);
	result_2 = result_1 * v23_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[22] = (result_2  - v23_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].B_Gray);
	result_2 = result_1 * v23_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[23] = (result_2  - v23_coefficient) & 0xff;
}

#define v11_coefficient 64
#define v11_denominator 320
static int v11_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	unsigned long long add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_11);
	add_mtp = LSB + V11_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_23);
	result_2 = (v11_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v11_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_11 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_11);
	add_mtp = LSB + V11_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_23);
	result_2 = (v11_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v11_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_11 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_11);
	add_mtp = LSB + V11_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_23);
	result_2 = (v11_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v11_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_11 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V11 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_11,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_11,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_11);
#endif

	return 0;
}

static void v11_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].R_Gray);
	result_2 = result_1 * v11_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[24] = (result_2  - v11_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].G_Gray);
	result_2 = result_1 * v11_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[25] = (result_2  - v11_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].B_Gray);
	result_2 = result_1 * v11_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[26] = (result_2  - v11_coefficient) & 0xff;

}

#define v3_coefficient 64
#define v3_denominator 320
static int v3_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_3);
	add_mtp = LSB + V3_300CD_R;
	result_1 = (ANA34801_VREG0_REF_L)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_11);
	result_2 = (v3_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v3_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (ANA34801_VREG0_REF_L) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_3 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_3);
	add_mtp = LSB + V3_300CD_G;
	result_1 = (ANA34801_VREG0_REF_L)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_11);
	result_2 = (v3_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v3_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (ANA34801_VREG0_REF_L) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_3 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_3);
	add_mtp = LSB + V3_300CD_B;
	result_1 = (ANA34801_VREG0_REF_L)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_11);
	result_2 = (v3_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v3_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (ANA34801_VREG0_REF_L) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_3 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_err("%s V3 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_3,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_3,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_3);
#endif

	return 0;
}

static void v3_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (ANA34801_VREG0_REF_L)
			- (pSmart->GRAY.TABLE[index[V3_INDEX]].R_Gray);
	result_2 = result_1 * v3_denominator;
	result_3 = (ANA34801_VREG0_REF_L)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[27] = (result_2  - v3_coefficient) & 0xff;

	result_1 = (ANA34801_VREG0_REF_L)
			- (pSmart->GRAY.TABLE[index[V3_INDEX]].G_Gray);
	result_2 = result_1 * v3_denominator;
	result_3 = (ANA34801_VREG0_REF_L)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[28] = (result_2  - v3_coefficient) & 0xff;

	result_1 = (ANA34801_VREG0_REF_L)
			- (pSmart->GRAY.TABLE[index[V3_INDEX]].B_Gray);
	result_2 = result_1 * v3_denominator;
	result_3 = (ANA34801_VREG0_REF_L)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[29] = (result_2  - v3_coefficient) & 0xff;
}

/*V0,V1,V3,V11,V23,V35,V51,V87,V151,V203,V255*/
static int INFLECTION_VOLTAGE_ARRAY[ARRAY_MAX] = {0, 1, 3, 11, 23, 35, 51, 87, 151, 203, 255};

#define V0toV3_Coefficient 2
#define V0toV3_Multiple 1
#define V0toV3_denominator 3

#define V3toV11_Coefficient 7
#define V3toV11_Multiple 1
#define V3toV11_denominator 8

#define V11toV23_Coefficient 11
#define V11toV23_Multiple 1
#define V11toV23_denominator 12

#define V23toV35_Coefficient 11
#define V23toV35_Multiple 1
#define V23toV35_denominator 12

#define V35toV51_Coefficient 15
#define V35toV51_Multiple 1
#define V35toV51_denominator 16

#define V51toV87_Coefficient 35
#define V51toV87_Multiple 1
#define V51toV87_denominator 36

#define V87toV151_Coefficient 63
#define V87toV151_Multiple 1
#define V87toV151_denominator 64

#define V151toV203_Coefficient 51
#define V151toV203_Multiple 1
#define V151toV203_denominator 52

#define V203toV255_Coefficient 51
#define V203toV255_Multiple 1
#define V203toV255_denominator 52

static int cal_gray_scale_linear(int up, int low, int coeff,
int mul, int deno, int cnt)
{
	unsigned long long result_1, result_2, result_3, result_4;

	result_1 = up - low;
	result_2 = (result_1 * (coeff - (cnt * mul))) << BIT_SHIFT;
	do_div(result_2, deno);
	result_3 = result_2 >> BIT_SHIFT;
	result_4 = low + result_3;

	return (int)result_4;
}

static int generate_gray_scale(struct SMART_DIM *pSmart)
{
	int cnt = 0, cal_cnt = 0;
	int array_index = 0;
	struct GRAY_VOLTAGE *ptable = (struct GRAY_VOLTAGE *)
						(&(pSmart->GRAY.TABLE));

	for (cnt = 0; cnt < ARRAY_MAX; cnt++) {
		pSmart->GRAY.TABLE[INFLECTION_VOLTAGE_ARRAY[cnt]].R_Gray =
			((int *)&(pSmart->RGB_OUTPUT.R_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[INFLECTION_VOLTAGE_ARRAY[cnt]].G_Gray =
			((int *)&(pSmart->RGB_OUTPUT.G_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[INFLECTION_VOLTAGE_ARRAY[cnt]].B_Gray =
			((int *)&(pSmart->RGB_OUTPUT.B_VOLTAGE))[cnt];
	}

	/*
		below codes use hard coded value.
		So it is possible to modify on each model.
		V0,V1,V3,V11,V23,V35,V51,V87,V151,V203,V255
	*/
	for (cnt = 0; cnt < GRAY_SCALE_MAX; cnt++) {

		if (cnt == INFLECTION_VOLTAGE_ARRAY[0]) {
			/* 0 */
			array_index = 0;
			cal_cnt = 0;
		} else if ((cnt > INFLECTION_VOLTAGE_ARRAY[0]) &&
			(cnt < INFLECTION_VOLTAGE_ARRAY[2])) {
			/* 1 ~ 2 */
			array_index = 2;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-2]].R_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].R_Gray,
			V0toV3_Coefficient, V0toV3_Multiple,
			V0toV3_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-2]].G_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].G_Gray,
			V0toV3_Coefficient, V0toV3_Multiple,
			V0toV3_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-2]].B_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].B_Gray,
			V0toV3_Coefficient, V0toV3_Multiple,
			V0toV3_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == INFLECTION_VOLTAGE_ARRAY[2]) {
			/* 3 */
			cal_cnt = 0;
		} else if ((cnt > INFLECTION_VOLTAGE_ARRAY[2]) &&
			(cnt < INFLECTION_VOLTAGE_ARRAY[3])) {
			/* 4 ~ 10 */
			array_index = 3;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].R_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].R_Gray,
			V3toV11_Coefficient, V3toV11_Multiple,
			V3toV11_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].G_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].G_Gray,
			V3toV11_Coefficient, V3toV11_Multiple,
			V3toV11_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].B_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].B_Gray,
			V3toV11_Coefficient, V3toV11_Multiple,
			V3toV11_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == INFLECTION_VOLTAGE_ARRAY[3]) {
			/* 11 */
			cal_cnt = 0;
		} else if ((cnt > INFLECTION_VOLTAGE_ARRAY[3]) &&
			(cnt < INFLECTION_VOLTAGE_ARRAY[4])) {
			/* 12 ~ 22 */
			array_index = 4;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].R_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].R_Gray,
			V11toV23_Coefficient, V11toV23_Multiple,
			V11toV23_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].G_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].G_Gray,
			V11toV23_Coefficient, V11toV23_Multiple,
			V11toV23_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].B_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].B_Gray,
			V11toV23_Coefficient, V11toV23_Multiple,
			V11toV23_denominator , cal_cnt);

			cal_cnt++;
		}  else if (cnt == INFLECTION_VOLTAGE_ARRAY[4]) {
			/* 23 */
			cal_cnt = 0;
		} else if ((cnt > INFLECTION_VOLTAGE_ARRAY[4]) &&
			(cnt < INFLECTION_VOLTAGE_ARRAY[5])) {
			/* 24 ~ 34 */
			array_index = 5;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].R_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].R_Gray,
			V23toV35_Coefficient, V23toV35_Multiple,
			V23toV35_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].G_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].G_Gray,
			V23toV35_Coefficient, V23toV35_Multiple,
			V23toV35_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].B_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].B_Gray,
			V23toV35_Coefficient, V23toV35_Multiple,
			V23toV35_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == INFLECTION_VOLTAGE_ARRAY[5]) {
			/* 35 */
			cal_cnt = 0;
		} else if ((cnt > INFLECTION_VOLTAGE_ARRAY[5]) &&
			(cnt < INFLECTION_VOLTAGE_ARRAY[6])) {
			/* 36 ~ 50 */
			array_index = 6;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].R_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].R_Gray,
			V35toV51_Coefficient, V35toV51_Multiple,
			V35toV51_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].G_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].G_Gray,
			V35toV51_Coefficient, V35toV51_Multiple,
			V35toV51_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].B_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].B_Gray,
			V35toV51_Coefficient, V35toV51_Multiple,
			V35toV51_denominator, cal_cnt);
			cal_cnt++;

		} else if (cnt == INFLECTION_VOLTAGE_ARRAY[6]) {
			/* 51 */
			cal_cnt = 0;
		} else if ((cnt > INFLECTION_VOLTAGE_ARRAY[6]) &&
			(cnt < INFLECTION_VOLTAGE_ARRAY[7])) {
			/* 52 ~ 86 */
			array_index = 7;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].R_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].R_Gray,
			V51toV87_Coefficient, V51toV87_Multiple,
			V51toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].G_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].G_Gray,
			V51toV87_Coefficient, V51toV87_Multiple,
			V51toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].B_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].B_Gray,
			V51toV87_Coefficient, V51toV87_Multiple,
			V51toV87_denominator, cal_cnt);
			cal_cnt++;

		} else if (cnt == INFLECTION_VOLTAGE_ARRAY[7]) {
			/* 87 */
			cal_cnt = 0;
		} else if ((cnt > INFLECTION_VOLTAGE_ARRAY[7]) &&
			(cnt < INFLECTION_VOLTAGE_ARRAY[8])) {
			/* 88 ~ 150 */
			array_index = 8;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].R_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].R_Gray,
			V87toV151_Coefficient, V87toV151_Multiple,
			V87toV151_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].G_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].G_Gray,
			V87toV151_Coefficient, V87toV151_Multiple,
			V87toV151_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].B_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].B_Gray,
			V87toV151_Coefficient, V87toV151_Multiple,
			V87toV151_denominator, cal_cnt);

			cal_cnt++;
		} else if (cnt == INFLECTION_VOLTAGE_ARRAY[8]) {
			/* 151 */
			cal_cnt = 0;
		} else if ((cnt > INFLECTION_VOLTAGE_ARRAY[8]) &&
			(cnt < INFLECTION_VOLTAGE_ARRAY[9])) {
			/* 152 ~ 202 */
			array_index = 9;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].R_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].R_Gray,
			V151toV203_Coefficient, V151toV203_Multiple,
			V151toV203_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].G_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].G_Gray,
			V151toV203_Coefficient, V151toV203_Multiple,
			V151toV203_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].B_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].B_Gray,
			V151toV203_Coefficient, V151toV203_Multiple,
			V151toV203_denominator, cal_cnt);

			cal_cnt++;
		} else if (cnt == INFLECTION_VOLTAGE_ARRAY[9]) {
			/* 203 */
			cal_cnt = 0;
		} else if ((cnt > INFLECTION_VOLTAGE_ARRAY[9]) &&
			(cnt < INFLECTION_VOLTAGE_ARRAY[10])) {
			/* 204 ~ 254 */
			array_index = 10;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].R_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].R_Gray,
			V203toV255_Coefficient, V203toV255_Multiple,
			V203toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].G_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].G_Gray,
			V203toV255_Coefficient, V203toV255_Multiple,
			V203toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index-1]].B_Gray,
			ptable[INFLECTION_VOLTAGE_ARRAY[array_index]].B_Gray,
			V203toV255_Coefficient, V203toV255_Multiple,
			V203toV255_denominator, cal_cnt);

			cal_cnt++;
		 } else {
			if (cnt == INFLECTION_VOLTAGE_ARRAY[10]) {
				pr_debug("%s end\n", __func__);
			} else {
				pr_err("%s fail cnt:%d\n", __func__, cnt);
				return -EINVAL;
			}
		}

	}

#ifdef SMART_DIMMING_DEBUG
		for (cnt = 0; cnt < GRAY_SCALE_MAX; cnt++) {
			pr_info("%s %8d %8d %8d %d\n", __func__,
				pSmart->GRAY.TABLE[cnt].R_Gray,
				pSmart->GRAY.TABLE[cnt].G_Gray,
				pSmart->GRAY.TABLE[cnt].B_Gray, cnt);
		}
#endif
	return 0;
}

#if 0
static char offset_cal(int offset,  int value)
{
	if (value - offset < 0)
		return 0;
	else if (value - offset > 255)
		return 0xFF;
	else
		return value - offset;
}

static void mtp_offset_substraction(struct SMART_DIM *pSmart, int *str)
{
	int level_255_temp = 0;
	int level_255_temp_MSB = 0;
	int MTP_V255;

	/*subtration MTP_OFFSET value from generated gamma table*/
	level_255_temp = (str[0] << 8) | str[1] ;
	MTP_V255 = char_to_int_v255(pSmart->MTP.R_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.R_OFFSET.OFFSET_255_LSB);
	level_255_temp -=  MTP_V255;
	level_255_temp_MSB = level_255_temp / 256;
	str[0] = level_255_temp_MSB & 0xff;
	str[1] = level_255_temp & 0xff;

	level_255_temp = (str[2] << 8) | str[3] ;
	MTP_V255 = char_to_int_v255(pSmart->MTP.G_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.G_OFFSET.OFFSET_255_LSB);
	level_255_temp -=  MTP_V255;
	level_255_temp_MSB = level_255_temp / 256;
	str[2] = level_255_temp_MSB & 0xff;
	str[3] = level_255_temp & 0xff;

	level_255_temp = (str[4] << 8) | str[5] ;
	MTP_V255 = char_to_int_v255(pSmart->MTP.B_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.B_OFFSET.OFFSET_255_LSB);
	level_255_temp -=  MTP_V255;
	level_255_temp_MSB = level_255_temp / 256;
	str[4] = level_255_temp_MSB & 0xff;
	str[5] = level_255_temp & 0xff;

	str[6] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_203), str[6]);
	str[7] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_203), str[7]);
	str[8] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_203), str[8]);

	str[9] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_151), str[9]);
	str[10] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_151), str[10]);
	str[11] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_151), str[11]);

	str[12] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87), str[12]);
	str[13] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87), str[13]);
	str[14] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87), str[14]);

	str[15] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_51), str[15]);
	str[16] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_51), str[16]);
	str[17] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_51), str[17]);

	str[18] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35), str[18]);
	str[19] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35), str[19]);
	str[20] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35), str[20]);

	str[21] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_23), str[21]);
	str[22] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_23), str[22]);
	str[23] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_23), str[23]);

	str[24] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_11), str[24]);
	str[25] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_11), str[25]);
	str[26] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_11), str[26]);

	str[27] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_3), str[27]);
	str[28] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_3), str[28]);
	str[29] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_3), str[29]);
}
#endif

static int searching_function(long long candela, int *index, int gamma_curve)
{
	long long delta_1 = 0, delta_2 = 0;
	int cnt;

	/*
	*	This searching_functin should be changed with improved
		searcing algorithm to reduce searching time.
	*/
	*index = -1;

	for (cnt = 0; cnt < (GRAY_SCALE_MAX-1); cnt++) {
		if (gamma_curve == GAMMA_CURVE_1P9) {
			delta_1 = candela - curve_1p9_360[cnt];
			delta_2 = candela - curve_1p9_360[cnt+1];
		} else if (gamma_curve == GAMMA_CURVE_2P15) {
			delta_1 = candela - curve_2p15_360[cnt];
			delta_2 = candela - curve_2p15_360[cnt+1];
		} else if (gamma_curve == GAMMA_CURVE_2P2) {
			delta_1 = candela - curve_2p2_360[cnt];
			delta_2 = candela - curve_2p2_360[cnt+1];
		} else {
			delta_1 = candela - curve_2p2_360[cnt];
			delta_2 = candela - curve_2p2_360[cnt+1];
		}

		if (delta_2 < 0) {
			*index = (delta_1 + delta_2) <= 0 ? cnt : cnt+1;
			break;
		}

		if (delta_1 == 0) {
			*index = cnt;
			break;
		}

		if (delta_2 == 0) {
			*index = cnt+1;
			break;
		}
	}

	if (*index == -1)
		return -EINVAL;
	else
		return 0;
}

/* -1 means V1 */
#define TABLE_MAX  (ARRAY_MAX-1)
static void(*Make_hexa[TABLE_MAX])(int*, struct SMART_DIM*, char*) = {
	v255_hexa,
	v203_hexa,
	v151_hexa,
	v87_hexa,
	v51_hexa,
	v35_hexa,
	v23_hexa,
	v11_hexa,
	v3_hexa,
	vt_hexa,
};

static int get_base_luminance(struct SMART_DIM *pSmart)
{
	int cnt;
	int base_luminance[LUMINANCE_MAX][2];

	memcpy(base_luminance, base_luminance_revA, sizeof(base_luminance_revA));

	for (cnt = 0; cnt < LUMINANCE_MAX; cnt++)
		if (base_luminance[cnt][0] == pSmart->brightness_level)
			return base_luminance[cnt][1];

	return -1;
}

static int find_cadela_table(int brightness)
{
	int loop;
	int err = -1;

	for(loop = 0; loop <= CANDELA_MAX_TABLE; loop++)
		if (candela_table[loop][0] == brightness)
			return candela_table[loop][1];

	return err;
}

static void gamma_init_revA(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[TABLE_MAX] = {-1, };
	int bl_index[TABLE_MAX] = {-1, };
	int gamma_setting[GAMMA_SET_MAX];

	long long temp_cal_data = 0;
	int bl_level = 0;

	int level_255_temp_MSB = 0;
	int level_V255 = 0;

	int point_index;
	int cnt;
	int table_index;

	pr_debug("%s : start !!\n",__func__);

	bl_level = get_base_luminance(pSmart);
	if (bl_level < 0)
		pr_err("%s : can not find base luminance!!\n", __func__);

	if (pSmart->brightness_level < 360) {
		for (cnt = 0; cnt < TABLE_MAX; cnt++) {
			point_index = INFLECTION_VOLTAGE_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p15[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
	} else {
		for (cnt = 0; cnt < TABLE_MAX; cnt++) {
			point_index = INFLECTION_VOLTAGE_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p2[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
	}

#ifdef SMART_DIMMING_DEBUG
	pr_info("\n candela_1:%llu  candela_3:%llu  candela_11:%llu \n"
		"candela_23:%llu  candela_35:%llu  candela_51:%llu \n"
		"candela_87:%llu  candela_151:%llu  candela_203:%llu \n"
		"candela_255:%llu brightness_level %d \n",
		candela_level[0], candela_level[1], candela_level[2],
		candela_level[3], candela_level[4], candela_level[5],
		candela_level[6], candela_level[7], candela_level[8],
		candela_level[9], pSmart->brightness_level);
#endif

	for (cnt = 0; cnt < TABLE_MAX; cnt++) {
		if (searching_function(candela_level[cnt],
			&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
			pr_info("%s searching functioin error cnt:%d\n",
			__func__, cnt);
		}
	}

	/*
	*	Candela compensation
	*/
	for (cnt = 1; cnt < TABLE_MAX; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CANDELA_MAX_TABLE;
			pr_info("%s fail candela table_index cnt : %d brightness %d\n",
				__func__, cnt, pSmart->brightness_level);
		}

		bl_index[TABLE_MAX - cnt] +=
			gradation_offset_revA[table_index][cnt - 1];

		/* THERE IS M-GRAY0 target */
		if (bl_index[TABLE_MAX - cnt] == 0)
			bl_index[TABLE_MAX - cnt] = 1;
	}

#ifdef SMART_DIMMING_DEBUG
	pr_info("\n bl_index_1:%d  bl_index_3:%d  bl_index_11:%d \n"
		"bl_index_23:%d bl_index_35:%d  bl_index_51:%d \n"
		"bl_index_87:%d  bl_index_151:%d bl_index_203:%d \n"
		"bl_index_255:%d brightness_level %d \n",
		bl_index[0], bl_index[1], bl_index[2],
		bl_index[3], bl_index[4], bl_index[5],
		bl_index[6], bl_index[7], bl_index[8],
		bl_index[9], pSmart->brightness_level);
#endif
	/*Generate Gamma table*/
	for (cnt = 0; cnt < TABLE_MAX; cnt++)
		(void)Make_hexa[cnt](bl_index , pSmart, str);

	/* To avoid overflow */
	for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
		gamma_setting[cnt] = str[cnt];

	/*
	*	RGB compensation
	*/
	for (cnt = 0; cnt < RGB_COMPENSATION; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CANDELA_MAX_TABLE;
			pr_info("%s fail RGB table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		if (cnt < 3) {
			level_V255 = str[cnt * 2] << 8 | str[(cnt * 2) + 1];
			level_V255 +=
				rgb_offset_revA[table_index][cnt];
			level_255_temp_MSB = level_V255 / 256;

			gamma_setting[cnt * 2] = level_255_temp_MSB & 0xff;
			gamma_setting[(cnt * 2) + 1] = level_V255 & 0xff;
		} else {
			gamma_setting[cnt+3] += rgb_offset_revA[table_index][cnt];
		}
	}

	/*subtration MTP_OFFSET value from generated gamma table*/
	/* mtp_offset_substraction is not need because Change Gamma Offset Index [4] is used. */
	/* mtp_offset_substraction(pSmart, gamma_setting); */

	/* To avoid overflow */
	for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
		str[cnt] = gamma_setting[cnt];
}

static void generate_hbm_gamma(struct SMART_DIM *psmart, char *str, int size)
{
#ifdef SMART_DIMMING_DEBUG
	int cnt;
#endif
	struct illuminance_table *ptable = (struct illuminance_table *)
						(&(psmart->hbm_interpolation_table));
	memcpy(str, &(ptable[psmart->hbm_brightness_level].gamma_setting), size);

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s\n",__func__);
	for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
		pr_info("0x%x ", str[cnt]);
	pr_info("\n");
#endif
}

static void generate_gamma(struct SMART_DIM *psmart, char *str, int size)
{
	int lux_loop;
	struct illuminance_table *ptable = (struct illuminance_table *)
						(&(psmart->gen_table));

	/* searching already generated gamma table */
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		if (ptable[lux_loop].lux == psmart->brightness_level) {
			memcpy(str, &(ptable[lux_loop].gamma_setting), size);
			break;
		}
	}

	/* searching fail... Setting 300CD value on gamma table */
	if (lux_loop == psmart->lux_table_max) {
		pr_info("%s searching fail lux : %d\n", __func__,
				psmart->brightness_level);
		memcpy(str, max_lux_table, size);
	}

#ifdef SMART_DIMMING_DEBUG
	if (lux_loop != psmart->lux_table_max)
		pr_info("%s searching ok index : %d lux : %d\n", __func__,
			lux_loop, ptable[lux_loop].lux);
#endif
}

static void mtp_sorting(struct SMART_DIM *psmart)
{
	int sorting[GAMMA_SET_MAX] = {
		9, 7, 6, 5, 4, 3, 2, 1, 0, 8, 10, /* R*/
		20, 18, 17, 16, 15, 14, 13, 12, 11, 19, 21, /* G */
		31, 29, 28, 27, 26, 25, 24, 23, 22, 30, 32, /* B */
	};
	int loop;
	char *pfrom, *pdest;

	pfrom = (char *)&(psmart->MTP_ORIGN);
	pdest = (char *)&(psmart->MTP);

	for (loop = 0; loop < GAMMA_SET_MAX; loop++)
		pdest[loop] = pfrom[sorting[loop]];

	for (loop = 0; loop < 3 ; loop++) {
		if(pdest[loop*11])
			pdest[loop * 11] *= -1;
	}
}

static void gamma_cell_determine(int ldi_revision)
{
	pr_info("%s ldi_revision: 0x%x", __func__, ldi_revision);

	max_lux_table[0] = V255_300CD_R_MSB = V255_300CD_R_MSB_20;
	max_lux_table[1] = V255_300CD_R_LSB = V255_300CD_R_LSB_20;

	max_lux_table[2] = V255_300CD_G_MSB = V255_300CD_G_MSB_20;
	max_lux_table[3] = V255_300CD_G_LSB = V255_300CD_G_LSB_20;

	max_lux_table[4] = V255_300CD_B_MSB = V255_300CD_B_MSB_20;
	max_lux_table[5] = V255_300CD_B_LSB = V255_300CD_B_LSB_20;

	max_lux_table[6] = V203_300CD_R = V203_300CD_R_20;
	max_lux_table[7] = V203_300CD_G = V203_300CD_G_20;
	max_lux_table[8] = V203_300CD_B = V203_300CD_B_20;

	max_lux_table[9] = V151_300CD_R = V151_300CD_R_20;
	max_lux_table[10] = V151_300CD_G = V151_300CD_G_20;
	max_lux_table[11] = V151_300CD_B = V151_300CD_B_20;

	max_lux_table[12] = V87_300CD_R = V87_300CD_R_20;
	max_lux_table[13] = V87_300CD_G = V87_300CD_G_20;
	max_lux_table[14] = V87_300CD_B = V87_300CD_B_20;

	max_lux_table[15] = V51_300CD_R = V51_300CD_R_20;
	max_lux_table[16] = V51_300CD_G = V51_300CD_G_20;
	max_lux_table[17] = V51_300CD_B = V51_300CD_B_20;

	max_lux_table[18] = V35_300CD_R = V35_300CD_R_20;
	max_lux_table[19] = V35_300CD_G = V35_300CD_G_20;
	max_lux_table[20] = V35_300CD_B = V35_300CD_B_20;

	max_lux_table[21] = V23_300CD_R = V23_300CD_R_20;
	max_lux_table[22] = V23_300CD_G = V23_300CD_G_20;
	max_lux_table[23] = V23_300CD_B = V23_300CD_B_20;

	max_lux_table[24] = V11_300CD_R = V11_300CD_R_20;
	max_lux_table[25] = V11_300CD_G = V11_300CD_G_20;
	max_lux_table[26] = V11_300CD_B = V11_300CD_B_20;

	max_lux_table[27] = V3_300CD_R = V3_300CD_R_20;
	max_lux_table[28] = V3_300CD_G = V3_300CD_G_20;
	max_lux_table[29] = V3_300CD_B = V3_300CD_B_20;

	max_lux_table[30] = VT_300CD_R = VT_300CD_R_20;
	max_lux_table[31] = VT_300CD_G = VT_300CD_G_20;
	max_lux_table[32] = VT_300CD_B = VT_300CD_B_20;
}

static void gamma_cell_determine_max(struct SMART_DIM *pSmart)
{
#ifdef SMART_DIMMING_DEBUG
	int i;
	char log_buf[256];
#endif
	int temp = 0; /* MAX_V255 */

	/* RED */
	max_lux_table2[0]  = 128 + char_to_int(pSmart->MTP.R_OFFSET.OFFSET_11);
	max_lux_table2[1]  = 128 + char_to_int(pSmart->MTP.R_OFFSET.OFFSET_23);
	max_lux_table2[2]  = 128 + char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35);
	max_lux_table2[3]  = 128 + char_to_int(pSmart->MTP.R_OFFSET.OFFSET_51);
	max_lux_table2[4]  = 128 + char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87);
	max_lux_table2[5]  = 128 + char_to_int(pSmart->MTP.R_OFFSET.OFFSET_151);
	max_lux_table2[6]  = 128 + char_to_int(pSmart->MTP.R_OFFSET.OFFSET_203);

	temp = 256 + char_to_int_v255(pSmart->MTP.R_OFFSET.OFFSET_255_MSB, pSmart->MTP.R_OFFSET.OFFSET_255_LSB);
	if (temp >= 256)
		max_lux_table2[7]  = temp - 256;
	else
		max_lux_table2[7]  = temp;
	max_lux_table2[8]  = 128 + char_to_int(pSmart->MTP.R_OFFSET.OFFSET_3);

	if (temp >= 256)
		max_lux_table2[9]  = 0x80; /* V255[8] bit */
	else
		max_lux_table2[9]  = 0;
	max_lux_table2[10] = 0;

	/* GREEN */
	max_lux_table2[11] = 128 + char_to_int(pSmart->MTP.G_OFFSET.OFFSET_11);
	max_lux_table2[12] = 128 + char_to_int(pSmart->MTP.G_OFFSET.OFFSET_23);
	max_lux_table2[13] = 128 + char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35);
	max_lux_table2[14] = 128 + char_to_int(pSmart->MTP.G_OFFSET.OFFSET_51);
	max_lux_table2[15] = 128 + char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87);
	max_lux_table2[16] = 128 + char_to_int(pSmart->MTP.G_OFFSET.OFFSET_151);
	max_lux_table2[17] = 128 + char_to_int(pSmart->MTP.G_OFFSET.OFFSET_203);

	temp = 256 + char_to_int_v255(pSmart->MTP.G_OFFSET.OFFSET_255_MSB, pSmart->MTP.G_OFFSET.OFFSET_255_LSB);
	if (temp >= 256)
		max_lux_table2[18]  = temp - 256;
	else
		max_lux_table2[18]  = temp;
	max_lux_table2[19] = 128 + char_to_int(pSmart->MTP.G_OFFSET.OFFSET_3);

	if (temp >= 256)
		max_lux_table2[20]  = 0x80;
	else
		max_lux_table2[20]  = 0;
	max_lux_table2[21] = 0;

	/* BLUE */
	max_lux_table2[22] = 128 + char_to_int(pSmart->MTP.B_OFFSET.OFFSET_11);
	max_lux_table2[23] = 128 + char_to_int(pSmart->MTP.B_OFFSET.OFFSET_23);
	max_lux_table2[24] = 128 + char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35);
	max_lux_table2[25] = 128 + char_to_int(pSmart->MTP.B_OFFSET.OFFSET_51);
	max_lux_table2[26] = 128 + char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87);
	max_lux_table2[27] = 128 + char_to_int(pSmart->MTP.B_OFFSET.OFFSET_151);
	max_lux_table2[28] = 128 + char_to_int(pSmart->MTP.B_OFFSET.OFFSET_203);

	temp = 256 + char_to_int_v255(pSmart->MTP.B_OFFSET.OFFSET_255_MSB, pSmart->MTP.B_OFFSET.OFFSET_255_LSB);
	if (temp >= 256)
		max_lux_table2[29]  = temp - 256;
	else
		max_lux_table2[29]  = temp;
	max_lux_table2[30] = 128 + char_to_int(pSmart->MTP.B_OFFSET.OFFSET_3);

	if (temp >= 256)
		max_lux_table2[31]  = 0x80;
	else
		max_lux_table2[31]  = 0;
	max_lux_table2[32] = 0;

#ifdef SMART_DIMMING_DEBUG
	memset(log_buf, 0x00, 256);
	for (i = 0; i < GAMMA_SET_MAX; i++)
		snprintf(log_buf + strnlen(log_buf, 256), 256, " %03d", max_lux_table2[i]);
	LCD_INFO(" %s\n", log_buf);
#endif
}

static char offeset_add(int offset,  int value)
{
	if (value + offset < 0)
		return 0;
	else if (value + offset > 255)
		return 0x80;
	else
		return value + offset;
}

static void gamma_command_sorting_post(struct SMART_DIM *psmart)
{
	int sorting[GAMMA_SET_MAX] = {
		24, 21, 18, 15, 12, 9,  6, 1, 27, 0, 30, /* R*/
		25, 22, 19, 16, 13, 10, 7, 3, 28, 2, 31, /* G */
		26, 23, 20, 17, 14, 11, 8, 5, 29, 4, 32, /* B */
	};
	int loop, loop2;
	char gamma_temp[GAMMA_SET_MAX];
	char *pfrom;

	pfrom = (char *)&(psmart->MTP_ORIGN);

	for (loop = 0; loop < LUMINANCE_MAX; loop++) {
		/* sorting */
		for (loop2 = 0; loop2 < GAMMA_SET_MAX; loop2++) {
			if(loop == LUMINANCE_MAX-1 ) {
				if(psmart->gen_table[loop].gamma_setting[sorting[loop2]] == 0x01)
					gamma_temp[loop2] =
						offeset_add(psmart->gen_table[loop].gamma_setting[sorting[loop2]], pfrom[loop2] + 0x7F);
				else
					gamma_temp[loop2] =
						offeset_add(psmart->gen_table[loop].gamma_setting[sorting[loop2]], pfrom[loop2]);
			} else {
				gamma_temp[loop2] = psmart->gen_table[loop].gamma_setting[sorting[loop2]];
			}
		}

		for (loop2 = 0; loop2 < 3; loop2++) {
			if(gamma_temp[(loop2*11)+9] == 0x01)
				gamma_temp[(loop2*11)+9] =
						gamma_temp[(loop2*11)+9] << 7;
		}

		/* Copy */
		memcpy(psmart->gen_table[loop].gamma_setting,
					gamma_temp, GAMMA_SET_MAX);
	}
}

#define HBM_CANDELA 500
#define HBM_GAMMA_SET_CNT 33
#define V255_RGB_MSB_CNT 3
int hbm_interpolation_gamma[HBM_INTERPOLATION_STEP][HBM_GAMMA_SET_CNT];

static void hbm_interpolation_init(struct SMART_DIM *pSmart)
{
	int loop, gamma_index, loop2;
	int rate;
	int hbm_gamma[HBM_GAMMA_SET_CNT];
	int max_gamma[HBM_GAMMA_SET_CNT];
	char *hbm_payload;
	int i;
	char log_buf[256];
	char V255_R_MSB;
	char V255_R_LSB;
	int  V255_R;	/* V255_RED [8:0]*/
	char V255_G_MSB;
	char V255_G_LSB;
	int  V255_G;	/* V255_GREEN [8:0]*/
	char V255_B_MSB;
	char V255_B_LSB;
	int  V255_B;	/* V255_BLUE [8:0]*/
	//int hbm_interpolation_gamma[HBM_INTERPOLATION_STEP][HBM_GAMMA_SET_CNT];
	int normal_max_candela = pSmart->gen_table[LUMINANCE_MAX-1].lux;

	hbm_payload = pSmart->hbm_payload;
	if (!hbm_payload) {
		LCD_ERR("no hbm_payload..\n");
		return;
	}

	/* 1. Generate HBM GAMMA */
	/* 1-1. Read From D4 Register */
	for (loop = 0, gamma_index = 0; gamma_index < HBM_GAMMA_SET_CNT;) {
		hbm_gamma[loop++] = hbm_payload[gamma_index];
		gamma_index++;
	}
#ifdef SMART_DIMMING_DEBUG
	memset(log_buf, 0x00, 256);
	for (i = 0; i < GAMMA_SET_MAX; i++)
		snprintf(log_buf + strnlen(log_buf, 256), 256, " %02x", hbm_gamma[i]);
	LCD_INFO("BEFORE Calculate hbm_gamma HEX : %s\n", log_buf);
#endif
	memset(log_buf, 0x00, 256);
	for (i = 0; i < GAMMA_SET_MAX; i++)
		snprintf(log_buf + strnlen(log_buf, 256), 256, " %03d", hbm_gamma[i]);
	LCD_INFO("BEFORE Calculate hbm_gamma DEC: %s\n", log_buf);

	/* 1-2. Calculate : D4 + OFFSET */
	/* RED */
	V255_R_MSB = hbm_gamma[9] >> 7;
	V255_R_LSB = hbm_gamma[7];
	V255_R = char_to_int(V255_R_MSB << 8 | V255_R_LSB)
		+ char_to_int_v255(pSmart->MTP.R_OFFSET.OFFSET_255_MSB, pSmart->MTP.R_OFFSET.OFFSET_255_LSB);
	if (V255_R_MSB >= 1 )
		V255_R = V255_R + 256; 	/* V255_RED [8:0] */

	LCD_INFO("V255_R_MSB=0x(%x) V255_R_LSB=0x(%x) V255_R=0x(%x)(%d)\n", V255_R_MSB, V255_R_LSB, V255_R, V255_R);
	hbm_gamma[0] += char_to_int(pSmart->MTP.R_OFFSET.OFFSET_11);
	hbm_gamma[1] += char_to_int(pSmart->MTP.R_OFFSET.OFFSET_23);
	hbm_gamma[2] += char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35);
	hbm_gamma[3] += char_to_int(pSmart->MTP.R_OFFSET.OFFSET_51);
	hbm_gamma[4] += char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87);
	hbm_gamma[5] += char_to_int(pSmart->MTP.R_OFFSET.OFFSET_151);
	hbm_gamma[6] += char_to_int(pSmart->MTP.R_OFFSET.OFFSET_203);
	hbm_gamma[7] = V255_R;
	hbm_gamma[8] += char_to_int(pSmart->MTP.R_OFFSET.OFFSET_3);

	if ( V255_R_MSB >= 1 )
		hbm_gamma[9] = 0x80;
	else
		hbm_gamma[9] = 0x00;
	hbm_gamma[10] = 0x00;

	/* GREEN */
	V255_G_MSB = hbm_gamma[20] >> 7;
	V255_G_LSB = hbm_gamma[18];
	V255_G = char_to_int(V255_G_MSB << 8 | V255_G_LSB)
		+ char_to_int_v255(pSmart->MTP.G_OFFSET.OFFSET_255_MSB, pSmart->MTP.G_OFFSET.OFFSET_255_LSB);
	if (V255_G_MSB >= 1 )
		V255_G = V255_G + 256; 	/* V255_GREEN [8:0] */

	LCD_INFO("V255_G_MSB=0x(%x) V255_G_LSB=0x(%x) V255_G=0x(%x)(%d)\n", V255_G_MSB, V255_G_LSB, V255_G, V255_G);
	hbm_gamma[11] += char_to_int(pSmart->MTP.G_OFFSET.OFFSET_11);
	hbm_gamma[12] += char_to_int(pSmart->MTP.G_OFFSET.OFFSET_23);
	hbm_gamma[13] += char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35);
	hbm_gamma[14] += char_to_int(pSmart->MTP.G_OFFSET.OFFSET_51);
	hbm_gamma[15] += char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87);
	hbm_gamma[16] += char_to_int(pSmart->MTP.G_OFFSET.OFFSET_151);
	hbm_gamma[17] += char_to_int(pSmart->MTP.G_OFFSET.OFFSET_203);
	hbm_gamma[18] = V255_G;
	hbm_gamma[19] += char_to_int(pSmart->MTP.G_OFFSET.OFFSET_3);

	if ( V255_G_MSB >= 1 )
		hbm_gamma[20] = 0x80;
	else
		hbm_gamma[20] = 0x00;
	hbm_gamma[21] = 0x00;

	/* BLUE */
	V255_B_MSB = hbm_gamma[31] >> 7;
	V255_B_LSB = hbm_gamma[29];
	V255_B = char_to_int(V255_B_MSB << 8 | V255_B_LSB)
		+ char_to_int_v255(pSmart->MTP.B_OFFSET.OFFSET_255_MSB, pSmart->MTP.B_OFFSET.OFFSET_255_LSB);
	if (V255_B_MSB >= 1 )
		V255_B = V255_B + 256; 	/* V255_BLUE [8:0] */

	LCD_INFO("V255_B_MSB=0x(%x) V255_B_LSB=0x(%x) V255_B=0x(%x)(%d)\n", V255_B_MSB, V255_B_LSB, V255_B, V255_B);
	hbm_gamma[22] += char_to_int(pSmart->MTP.B_OFFSET.OFFSET_11);
	hbm_gamma[23] += char_to_int(pSmart->MTP.B_OFFSET.OFFSET_23);
	hbm_gamma[24] += char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35);
	hbm_gamma[25] += char_to_int(pSmart->MTP.B_OFFSET.OFFSET_51);
	hbm_gamma[26] += char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87);
	hbm_gamma[27] += char_to_int(pSmart->MTP.B_OFFSET.OFFSET_151);
	hbm_gamma[28] += char_to_int(pSmart->MTP.B_OFFSET.OFFSET_203);
	hbm_gamma[29] = V255_B;
	hbm_gamma[30] += char_to_int(pSmart->MTP.B_OFFSET.OFFSET_3);

	if ( V255_B_MSB >= 1 )
		hbm_gamma[31] = 0x80;
	else
		hbm_gamma[31] = 0x00;
	hbm_gamma[32] = 0x00;

#ifdef SMART_DIMMING_DEBUG
	memset(log_buf, 0x00, 256);
	for (i = 0; i < GAMMA_SET_MAX; i++)
		snprintf(log_buf + strnlen(log_buf, 256), 256, " %02x", hbm_gamma[i]);
	LCD_INFO("AFTER Calculate hbm_gamma HEX : %s\n", log_buf);
#endif
	memset(log_buf, 0x00, 256);
	for (i = 0; i < GAMMA_SET_MAX; i++)
		snprintf(log_buf + strnlen(log_buf, 256), 256, " %03d", hbm_gamma[i]);
	LCD_INFO("AFTER Calculate hbm_gamma DEC : %s\n", log_buf);

	/* 2. Generate MAX GAMMA */
	/* 2-1. Set Max gamma from gamma_setting Table */
	for (loop = 0, gamma_index = 0; gamma_index < HBM_GAMMA_SET_CNT;) {
		max_gamma[loop++] = pSmart->gen_table[LUMINANCE_MAX-1].gamma_setting[gamma_index];
		gamma_index++;
	}

	/* 2-2. MAX GAMMA V255 process : max_gamma = V255[8] | V255[7:0] */
	if ( max_gamma[9] >=  0x80 )
		max_gamma[7] += 256; 	/* V255_RED [8:0] */

	if ( max_gamma[20] >=  0x80 )
		max_gamma[18] += 256; 	/* V255_GREEN [8:0] */

	if ( max_gamma[31] >=  0x80 )
		max_gamma[29] += 256; 	/* V255_BLUE [8:0] */

	/* 3. Generate interpolation hbm gamma */
	/* 3-1. Divide 8 Step */
	for (loop = 0 ; loop < HBM_INTERPOLATION_STEP; loop++) {
		rate = ((hbm_interpolation_candela_table[loop] - normal_max_candela) * BIT_SHFIT_MUL) / (HBM_CANDELA - normal_max_candela);
		for (gamma_index = 0; gamma_index < HBM_GAMMA_SET_CNT; gamma_index++) {
			hbm_interpolation_gamma[loop][gamma_index] = max_gamma[gamma_index] +
				((hbm_gamma[gamma_index] - max_gamma[gamma_index]) * rate) / BIT_SHFIT_MUL;

			/* 3-2.  Addtional process : hbm_interpolation_gamma (After Divide 8 Step) is more than 256 */
			if ( gamma_index == 9 || gamma_index == 20 || gamma_index == 31 ) {
				if (hbm_interpolation_gamma[loop][gamma_index - 2] >= 256 ) /* V255 Check */
					hbm_interpolation_gamma[loop][gamma_index] = 0x80;  /* V255[8] | V0[6:0] */
				else
					hbm_interpolation_gamma[loop][gamma_index] = 0x00;
			}
		}
	}

	/* 4. Generate hbm_interpolation_table */
	for (loop = 0; loop < HBM_INTERPOLATION_STEP; loop++) {
		for (gamma_index = 0; gamma_index < HBM_GAMMA_SET_CNT; gamma_index++) {
			pSmart->hbm_interpolation_table[loop].gamma_setting[gamma_index] =
				hbm_interpolation_gamma[loop][gamma_index];
		}
		for (loop2 = 0; loop2 < 3; loop2++) {
			pSmart->hbm_interpolation_table[loop].gamma_setting[(loop2*11)+10] = 0x00;
		}
	}

#ifdef SMART_DIMMING_DEBUG
	print_hbm_lux_table(pSmart, "DEC");
	print_hbm_lux_table(pSmart, "HEX");
#endif
	return;
}

char pBuffer[256];

static int smart_dimming_init(struct SMART_DIM *psmart)
{
	int lux_loop;
	int id1, id2, id3;
	int cnt;

	id1 = (psmart->ldi_revision & 0x00FF0000) >> 16;
	id2 = (psmart->ldi_revision & 0x0000FF00) >> 8;
	id3 = psmart->ldi_revision & 0xFF;

	LCD_INFO("++\n");

	mtp_sorting(psmart);
	gamma_cell_determine(psmart->ldi_revision);
#ifdef SMART_DIMMING_DEBUG
	print_RGB_offset(psmart);
#endif
	v255_adjustment(psmart);
	vt_adjustment(psmart);
	v203_adjustment(psmart);
	v151_adjustment(psmart);
	v87_adjustment(psmart);
	v51_adjustment(psmart);
	v35_adjustment(psmart);
	v23_adjustment(psmart);
	v11_adjustment(psmart);
	v3_adjustment(psmart);

	if (generate_gray_scale(psmart)) {
		pr_info(KERN_ERR "lcd smart dimming fail generate_gray_scale\n");
		return -EINVAL;
	}

	/*Generating lux_table*/
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		/* To set brightness value */
		psmart->brightness_level = psmart->plux_table[lux_loop];
		/* To make lux table index*/
		psmart->gen_table[lux_loop].lux = psmart->plux_table[lux_loop];

			gamma_init_revA(psmart,
				(char *)(&(psmart->gen_table[lux_loop].gamma_setting)),
				GAMMA_SET_MAX);
		}

	/* set 300CD max gamma table */
	memcpy(&(psmart->gen_table[lux_loop-1].gamma_setting),
			max_lux_table, GAMMA_SET_MAX);

#ifdef SMART_DIMMING_DEBUG
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
			snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d",
				psmart->gen_table[lux_loop].gamma_setting[cnt]);

		pr_info("lux : %3d  %s\n", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
#endif
	gamma_command_sorting_post(psmart);
	gamma_cell_determine_max(psmart);

	/* Set 360CD max gamma table */
	memcpy(&(psmart->gen_table[LUMINANCE_MAX-1].gamma_setting),
			max_lux_table2, GAMMA_SET_MAX);

	memset(pBuffer, 0x00, 256);
	for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
		snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x", max_lux_table2[cnt]);
	LCD_INFO("MAX GAMMA : %s\n", pBuffer);

	hbm_interpolation_init(psmart);

#ifdef SMART_DIMMING_DEBUG
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
			snprintf(pBuffer + strnlen(pBuffer, 256), 256,
				" %02X",
				psmart->gen_table[lux_loop].gamma_setting[cnt]);

		pr_info("lux : %3d  %s\n", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
#endif
	LCD_INFO(" done\n");
	return 0;
}

static void wrap_generate_hbm_gamma(struct smartdim_conf * conf, int hbm_brightness_level, char *cmd_str) {

	struct SMART_DIM *smart = conf->psmart;

	if (!smart) {
		pr_info("%s fail", __func__);
		return ;
	}

	smart->hbm_brightness_level = hbm_brightness_level - 6;
	generate_hbm_gamma(conf->psmart, cmd_str, GAMMA_SET_MAX);
}

/* ----------------------------------------------------------------------------
 * Wrapper functions for smart dimming
 * ----------------------------------------------------------------------------
 */
static void wrap_generate_gamma(struct smartdim_conf * conf, int cd, char *cmd_str) {

	struct SMART_DIM *smart = conf->psmart;

	if (!smart) {
		pr_info("%s fail", __func__);
		return ;
	}

	smart->brightness_level = cd;
	generate_gamma(conf->psmart, cmd_str, GAMMA_SET_MAX);
}

static void wrap_smart_dimming_init(struct smartdim_conf * conf) {

	struct SMART_DIM *smart = conf->psmart;

	if (!smart) {
		pr_info("%s fail", __func__);
		return ;
	}

	smart->plux_table = conf->lux_tab;
	smart->lux_table_max = conf->lux_tabsize;
	smart->ldi_revision = conf->man_id;
	smart->hbm_payload = conf->hbm_payload;

	smart_dimming_init(smart);
}

struct smartdim_conf *smart_get_conf_ANA38401_AMSA05RB06(void) {

	struct smartdim_conf * smartdim_conf;
	struct SMART_DIM *smart;

	smartdim_conf = kzalloc(sizeof(struct smartdim_conf), GFP_KERNEL);
	if (!smartdim_conf) {
		pr_info("%s allocation fail", __func__);
		goto out2;
	}

	smart = kzalloc(sizeof(struct SMART_DIM), GFP_KERNEL);
	if (!smart) {
		pr_info("%s allocation fail", __func__);
		goto out1;
	}

	smartdim_conf->psmart = smart;
	smartdim_conf->generate_gamma = wrap_generate_gamma;
	smartdim_conf->generate_hbm_gamma = wrap_generate_hbm_gamma;
	smartdim_conf->init = wrap_smart_dimming_init;
	smartdim_conf->get_min_lux_table = NULL;
	smartdim_conf->mtp_buffer = (char *)(&smart->MTP_ORIGN);
	smartdim_conf->print_aid_log = print_aid_log;

	return smartdim_conf;

out1:
	kfree(smartdim_conf);
out2:
	return NULL;
}

/* ----------------------------------------------------------------------------
 * END - Wrapper
 * ----------------------------------------------------------------------------
 */
