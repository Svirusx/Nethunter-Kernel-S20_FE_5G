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
 */

#include "ss_dsi_smart_dimming_S6E3HAB_AMB623TS01.h"

//#define SMART_DIMMING_DEBUG

static char max_lux_table[GAMMA_SET_MAX];

/*
 * PRINT LOG
 */
static void print_RGB_offset(struct SMART_DIM *pSmart)
{
	int i;

	for (i = 0; i < V_MAX; i++) {
		LCD_INFO("%s MTP_OFFSET %4s %3d %3d %3d \n", __func__, V_LIST[i],
			pSmart->MTP[i][R],
			pSmart->MTP[i][G],
			pSmart->MTP[i][B]);
	}

	for (i = 0; i < V_MAX; i++) {
		LCD_INFO("%s CENTER_GAMMA %4s %3x %3x %3x \n", __func__, V_LIST[i],
			pSmart->CENTER_GAMMA_V[i][R],
			pSmart->CENTER_GAMMA_V[i][G],
			pSmart->CENTER_GAMMA_V[i][B]);
	}
}

static void print_lux_table(struct SMART_DIM *psmart, char *type)
{
	int lux_loop;
	int cnt;
	char pBuffer[256];
	int gamma;
	int first;
	int temp;
	memset(pBuffer, 0x00, 256);

	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		first = psmart->gen_table[lux_loop].gamma_setting[0];
		for (cnt = 1; cnt < GAMMA_SET_MAX - 3; cnt++) {
			gamma = psmart->gen_table[lux_loop].gamma_setting[cnt];
			if (cnt == 1) {
				gamma += (first & BIT(4) ? BIT(8) : 0);
				gamma += (first & BIT(5) ? BIT(9) : 0);
			} else if (cnt == 2) {
				gamma += (first & BIT(2) ? BIT(8) : 0);
				gamma += (first & BIT(3) ? BIT(9) : 0);
			} else if (cnt == 3) {
				gamma += (first & BIT(0) ? BIT(8) : 0);
				gamma += (first & BIT(1) ? BIT(9) : 0);
			}
			if (!strcmp(type, "DEC"))
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d", gamma);
			else
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x", gamma);
		}
		/* V0, VT */
		for (cnt = GAMMA_SET_MAX - 3; cnt < GAMMA_SET_MAX; cnt++) {
			temp = psmart->gen_table[lux_loop].gamma_setting[cnt];

			gamma = (temp & 0xF0) >> 4;
			if (!strcmp(type, "DEC"))
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d", gamma);
			else
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x", gamma);

			gamma = temp & 0x0F;
			if (!strcmp(type, "DEC"))
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d", gamma);
			else
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x", gamma);
		}
		LCD_INFO("lux[%3d]  %s\n", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
}

static void print_hbm_lux_table(struct SMART_DIM *psmart, char *type)
{
	int i, j;
	char pBuffer[256];
	int *hbm_interpolation_candela_table;
	int hbm_steps;
	int gamma;
	int first;
	int temp;

	hbm_interpolation_candela_table = hbm_interpolation_candela_table_revA;
	hbm_steps = HBM_INTERPOLATION_STEP;

	memset(pBuffer, 0x00, 256);

	for (i = 0; i < hbm_steps; i++) {
		first = psmart->hbm_interpolation_table[i].gamma_setting[0];
		for (j = 1; j < GAMMA_SET_MAX - 3; j++) {
			gamma = psmart->hbm_interpolation_table[i].gamma_setting[j];
			if (j == 1) {
				gamma += (first & BIT(4) ? BIT(8) : 0);
				gamma += (first & BIT(5) ? BIT(9) : 0);
			} else if (j == 2) {
				gamma += (first & BIT(2) ? BIT(8) : 0);
				gamma += (first & BIT(3) ? BIT(9) : 0);
			} else if (j == 3) {
				gamma += (first & BIT(0) ? BIT(8) : 0);
				gamma += (first & BIT(1) ? BIT(9) : 0);
			}
			if (!strcmp(type, "DEC"))
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d", gamma);
			else
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x", gamma);
		}
		/* V0, VT */
		for (j = GAMMA_SET_MAX - 3; j < GAMMA_SET_MAX; j++) {
			temp = psmart->hbm_interpolation_table[i].gamma_setting[j];

			gamma = (temp & 0xF0) >> 4;
			if (!strcmp(type, "DEC"))
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d", gamma);
			else
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x", gamma);

			gamma = temp & 0x0F;
			if (!strcmp(type, "DEC"))
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d", gamma);
			else
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x", gamma);
		}
		LCD_INFO("hbm[%3d]  %s\n", hbm_interpolation_candela_table[i], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
}

static void print_aid_log(struct smartdim_conf *conf)
{
	print_RGB_offset(conf->psmart);
	print_lux_table(conf->psmart, "DEC");
	print_hbm_lux_table(conf->psmart, "DEC");

	print_lux_table(conf->psmart, "HEX");
	print_hbm_lux_table(conf->psmart, "HEX");
}

// 1. VT,V0,V255 : VREG1 - (VREG1 - VREF) * (OutputGamma + numerator_TP)/(denominator_TP)
static unsigned long long v255_TP_gamma_voltage_calc(int VREG1, int VREF, int OutputGamma,
								int fraction[])
{
	unsigned long long val1, val2, val3;

	val1 = (unsigned long long)(fraction[0] + OutputGamma) << BIT_SHIFT;
	do_div(val1, fraction[1]);
	val2 = ((VREG1 - VREF) * val1) >> BIT_SHIFT;

	val3 = VREG1 - val2;

	return val3;
}

// 2. VT1, VT7 : VREG1 - (VREG1 - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP)
// 3. other VT :    VT - (   VT - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP)
static unsigned long long other_TP_gamma_voltage_calc(int VT, int V_nextTP, int OutputGamma,
								int fraction[])
{
	unsigned long long val1, val2, val3, val4;

	val1 = VT - V_nextTP;

	val2 = (unsigned long long)(fraction[0] + OutputGamma) << BIT_SHIFT;
	do_div(val2, fraction[1]);
	val3 = (val1 * val2) >> BIT_SHIFT;

	val4 = VT - val3;

	return val4;
}

/*
 *	each TP's gamma voltage calculation (2.3.3)
 */
static void TP_gamma_voltage_calc(struct SMART_DIM *pSmart)
{
	int i, j;
	int OutputGamma;
	int MTP_OFFSET;
	int idx;

	/* VT */
	for (j = 0; j < RGB_MAX; j++) {
		MTP_OFFSET = pSmart->MTP[VT][j];
		idx = MTP_OFFSET + pSmart->CENTER_GAMMA_V[VT][j];
		fraction[VT][0] = vt_coefficient[idx];
		pSmart->RGB_OUTPUT[VT][j] =
			v255_TP_gamma_voltage_calc(pSmart->vregout_voltage, pSmart->vref,
					0, fraction[VT]);
	}

	/* V0 */
	for (j = 0; j < RGB_MAX; j++) {
		MTP_OFFSET = pSmart->MTP[V0][j];
		idx = MTP_OFFSET + pSmart->CENTER_GAMMA_V[V0][j];
		fraction[V0][0] = v0_coefficient[idx];
		pSmart->RGB_OUTPUT[V0][j] =
			v255_TP_gamma_voltage_calc(pSmart->vregout_voltage, pSmart->vref,
					0, fraction[V0]);
	}

	/* V255 */
	for (j = 0; j < RGB_MAX; j++) {
		MTP_OFFSET = pSmart->MTP[V255][j];
		OutputGamma = MTP_OFFSET + pSmart->CENTER_GAMMA_V[V255][j];
		pSmart->RGB_OUTPUT[V255][j] =
			v255_TP_gamma_voltage_calc(pSmart->vregout_voltage, pSmart->vref,
					OutputGamma, fraction[V255]);
	}

	/* V203 ~ V11 */
	for (i = V203; i >= V11; i--) {
		for (j = 0; j < RGB_MAX; j++) {
			MTP_OFFSET = pSmart->MTP[i][j];
			OutputGamma = MTP_OFFSET + pSmart->CENTER_GAMMA_V[i][j];
			pSmart->RGB_OUTPUT[i][j] =
				other_TP_gamma_voltage_calc(pSmart->RGB_OUTPUT[VT][j],
						pSmart->RGB_OUTPUT[i+1][j],
						OutputGamma, fraction[i]);
		}
	}

	/* V7 ~ V1*/
	for (i = V7; i >= V1; i--) {
		for (j = 0; j < RGB_MAX; j++) {
			MTP_OFFSET = pSmart->MTP[i][j];
			OutputGamma = MTP_OFFSET + pSmart->CENTER_GAMMA_V[i][j];
			pSmart->RGB_OUTPUT[i][j] =
				other_TP_gamma_voltage_calc(pSmart->vregout_voltage,
						pSmart->RGB_OUTPUT[i+1][j],
						OutputGamma, fraction[i]);
		}
	}

#ifdef SMART_DIMMING_DEBUG
	for (i = 0; i < V_MAX; i++) {
		pr_err("%16s %4s %d %d %d\n", __func__, V_LIST[i],
				pSmart->RGB_OUTPUT[i][R],
				pSmart->RGB_OUTPUT[i][G],
				pSmart->RGB_OUTPUT[i][B]);
	}
#endif
}

// 1. V255 : ((VREG1 - V255) * denominator_TP / (VREG - VREF)) - numerator_TP
static unsigned long long v255_TP_gamma_code_calc(int VREG, int VREF, int GRAY, int fraction[])
{
	unsigned long long val1, val2, val3;

	val1 = VREG - GRAY;
	val2 = val1 * fraction[1];
	do_div(val2, (VREG - VREF));
	val3 = val2 - fraction[0];

	return val3;
}

// 2. other : (VT    - V_TP)* denominator_TP /(VT    - V_nextTP) - numerator_TP
// 3. V7,V1 : (VREG1 - V_TP)* denominator_TP /(VREG1 - V_nextTP) - numerator_TP
static unsigned long long other_TP_gamma_code_calc(int VT, int GRAY, int nextGRAY, int fraction[])
{
	unsigned long long val1, val2, val3, val4;
	signed long long s_val1, s_val2, s_val3, s_val4;
	int gray_sign_minus = 1, nextgray_sign_minus = 1;

	if (unlikely((VT - GRAY) < 0))
		gray_sign_minus = -1;

	if (unlikely((VT - nextGRAY) < 0))
		nextgray_sign_minus = -1;

	if (unlikely(gray_sign_minus == -1) ||\
			unlikely(nextgray_sign_minus == -1)) {
		s_val1 = VT - GRAY;
		s_val2 = s_val1 * fraction[1];
		s_val3 = VT - nextGRAY;
		s_val2 *= gray_sign_minus;
		s_val3 *= nextgray_sign_minus;
		do_div(s_val2, s_val3);
		s_val2 *= (gray_sign_minus * nextgray_sign_minus);
		s_val4 = s_val2 - fraction[0];
		val4 = (unsigned long long)s_val4;
	} else {
		val1 = VT - GRAY;
		val2 = val1 * fraction[1];
		val3 = VT - nextGRAY;
		do_div(val2, val3);
		val4 = val2 - fraction[0];
	}

	return val4;
}

/*
 *	each TP's gamma code calculation (3.7.1)
 */
static void TP_gamma_code_calc(struct SMART_DIM *pSmart, int *M_GRAY, int *str)
{
	unsigned long long val;
	int TP, nextTP;
	int i, j;
	u8 v255_10bit[RGB_MAX];
	u8 v255_9bit[RGB_MAX];
	int cnt; // str[0] ~ str[34]

	/* V255 */
	TP = M_GRAY[V255];
	cnt = 1;
	for (j = 0; j < RGB_MAX; j++) {
		val = v255_TP_gamma_code_calc(pSmart->vregout_voltage, pSmart->vref,
				pSmart->GRAY[TP][j], fraction[V255]);

		v255_10bit[j] = !!(val & BIT(9));
		v255_9bit[j] = !!(val & BIT(8));

		str[cnt++] = val & 0xFF;
	}
	str[0] =
		(v255_10bit[R] << 5) | (v255_10bit[G] << 3) | (v255_10bit[B] << 1) |
		(v255_9bit[R] << 4) | (v255_9bit[G] << 2) | (v255_9bit[B] << 0);

	/* V203 ~ V11 */

	for (i = V203; i >= V11; i--) {
		for (j = 0; j < RGB_MAX; j++) {
			TP = M_GRAY[i];
			nextTP = M_GRAY[i+1];
			val = other_TP_gamma_code_calc(pSmart->RGB_OUTPUT[VT][j],
					pSmart->GRAY[TP][j],
					pSmart->GRAY[nextTP][j],
					fraction[i]);
			str[cnt++] = val;
		}
	}

	/* V7 ~ V1 */
	for (i = V7; i >= V1; i--) {
		for (j = 0; j < RGB_MAX; j++) {
			TP = M_GRAY[i];
			nextTP = M_GRAY[i+1];
			val = other_TP_gamma_code_calc(pSmart->vregout_voltage,
					pSmart->GRAY[TP][j],
					pSmart->GRAY[nextTP][j],
					fraction[i]);
			str[cnt++] = val;
		}
	}

	/* V0/VT */
	str[cnt++] = (pSmart->CENTER_GAMMA_V[V0][R] & 0xF) << 4 | (pSmart->CENTER_GAMMA_V[V0][G] & 0xF);
	str[cnt++] = (pSmart->CENTER_GAMMA_V[V0][B] & 0xF) << 4 | (pSmart->CENTER_GAMMA_V[VT][R] & 0xF);
	str[cnt++] = (pSmart->CENTER_GAMMA_V[VT][G] & 0xF) << 4 | (pSmart->CENTER_GAMMA_V[VT][B] & 0xF);
}

/* gray scale = V_down + (V_up - V_down) * num / den */
static int gray_scale_calc(int v_up, int v_down, int num, int den)
{
	unsigned long long val1, val2;

	val1 = v_up - v_down;

	val2 = (unsigned long long)(val1 * num) << BIT_SHIFT;

	do_div(val2, den);

	val2 >>= BIT_SHIFT;

	val2 += v_down;

	return (int)val2;
}

static int generate_gray_scale(struct SMART_DIM *pSmart)
{
	int i, V_idx;
	int cnt = 0, cal_cnt = 0;
	int den = 0;

	/*
		GRAY OUTPUT VOLTAGE of TP's (V0,V1,V7,V11,V23,V35,V51,V87,V151,V203,V255)
		VT is meanless.. use V0...
	*/
	for (i = 0; i < RGB_MAX; i++)
		pSmart->GRAY[0][i] = pSmart->RGB_OUTPUT[V0][i];

	for (cnt = V1; cnt < V_MAX; cnt++) {
		for (i = 0; i < RGB_MAX; i++) {
			pSmart->GRAY[INFLECTION_VOLTAGE_ARRAY[cnt]][i] =
				pSmart->RGB_OUTPUT[cnt][i];
		}
	}

	/*
		ALL GRAY OUTPUT VOLTAGE (0~255)
	*/
	V_idx = 1;
	for (cnt = 0; cnt < GRAY_SCALE_MAX; cnt++) {
		if (cnt == INFLECTION_VOLTAGE_ARRAY[V_idx]) {
			cal_cnt = 1;
			V_idx++;
		} else {
			den = INFLECTION_VOLTAGE_ARRAY[V_idx] - INFLECTION_VOLTAGE_ARRAY[V_idx-1];

			for (i = 0; i < RGB_MAX; i++)
				pSmart->GRAY[cnt][i] = gray_scale_calc(
					pSmart->GRAY[INFLECTION_VOLTAGE_ARRAY[V_idx-1]][i],
					pSmart->GRAY[INFLECTION_VOLTAGE_ARRAY[V_idx]][i],
					den - cal_cnt, den);
			cal_cnt++;
		}
	}

#ifdef SMART_DIMMING_DEBUG
	for (cnt = 0; cnt < GRAY_SCALE_MAX; cnt++) {
		pr_err("%s %8d %8d %8d %d\n", __func__,
			pSmart->GRAY[cnt][R],
			pSmart->GRAY[cnt][G],
			pSmart->GRAY[cnt][B], cnt);
	}
#endif

	return 0;
}

static char offset_cal(int offset,  int value)
{
	if (value - offset < 0)
		return 0;
	else if (value - offset > 255)
		return 0xFF;
	else
		return value - offset;
}

/* subtration MTP_OFFSET value from generated gamma table */
static void mtp_offset_substraction(struct SMART_DIM *pSmart, int *str)
{
	int i, j;
	int idx = 0;
	u8 v255_10bit[RGB_MAX];
	u8 v255_9bit[RGB_MAX];
	int level_V255[RGB_MAX];

	/* V255 : str[0] ~ str[3] */
	v255_10bit[R] = !!(str[0] & BIT(5));
	v255_10bit[G] = !!(str[0] & BIT(3));
	v255_10bit[B] = !!(str[0] & BIT(1));

	v255_9bit[R] = !!(str[0] & BIT(4));
	v255_9bit[G] = !!(str[0] & BIT(2));
	v255_9bit[B] = !!(str[0] & BIT(0));

	level_V255[R] = str[1] | (v255_10bit[R] << 9) | (v255_9bit[R] << 8);
	level_V255[G] = str[2] | (v255_10bit[G] << 9) | (v255_9bit[G] << 8);
	level_V255[B] = str[3] | (v255_10bit[B] << 9) | (v255_9bit[B] << 8);

	level_V255[R] -= pSmart->MTP[V255][R];
	level_V255[G] -= pSmart->MTP[V255][G];
	level_V255[B] -= pSmart->MTP[V255][B];

	v255_10bit[R] = !!(level_V255[R] & BIT(9));
	v255_10bit[G] = !!(level_V255[G] & BIT(9));
	v255_10bit[B] = !!(level_V255[B] & BIT(9));

	v255_9bit[R] = !!(level_V255[R] & BIT(8));
	v255_9bit[G] = !!(level_V255[G] & BIT(8));
	v255_9bit[B] = !!(level_V255[B] & BIT(8));

	str[0] =
		v255_10bit[R] << 5 | v255_10bit[G] << 3 | v255_10bit[B] << 1 |
		v255_9bit[R] << 4 | v255_9bit[G] << 2 | v255_9bit[B] << 0;
	str[1] = level_V255[R] & 0xFF;
	str[2] = level_V255[G] & 0xFF;
	str[3] = level_V255[B] & 0xFF;

	/* V203 ~ V7 : str[6] ~ str[29] */
	idx = 4;
	for (i = V203; i >= V1; i--) {
		for (j = 0; j < RGB_MAX; j++) {
			str[idx] = offset_cal(pSmart->MTP[i][j], str[idx]);
			idx++;
		}
	}
}

/* 3.6 - TP's Voltage Search */
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
			delta_1 = candela - curve_2p15_420[cnt];
			delta_2 = candela - curve_2p15_420[cnt+1];
		} else if (gamma_curve == GAMMA_CURVE_2P2) {
			delta_1 = candela - curve_2p2_420[cnt];
			delta_2 = candela - curve_2p2_420[cnt+1];
		} else if (gamma_curve == GAMMA_CURVE_2P2_400CD) {
			delta_1 = candela - curve_2p2_400[cnt];
			delta_2 = candela - curve_2p2_400[cnt+1];
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

static int get_max_candela(void)
{
	return 420;
}

static int get_vreg_voltage(char panel_revision)
{
	int vreg_voltage = 0;
	switch (panel_revision) {
	case 'A':
		vreg_voltage = VREG1_REF_6P9;
		break;
	default:
		vreg_voltage = VREG1_REF_6P9;
		break;
	}

	LCD_INFO("panel revision %c\n", panel_revision);

	return vreg_voltage;
}

static int get_vref(char panel_revision)
{
	int vreg_voltage = 0;
	switch (panel_revision) {
	case 'A':
		vreg_voltage = VREF_1P0;
		break;
	default:
		vreg_voltage = VREF_1P0;
		break;
	}

	LCD_INFO("panel revision %c\n", panel_revision);

	return vreg_voltage;
}

static int get_base_luminance(struct SMART_DIM *pSmart)
{
	int cnt;
	int base_luminance[LUMINANCE_MAX][2];

	switch (pSmart->panel_revision) {
	case 'A':
		if (pSmart->rr == 60) {
			if (pSmart->sot_hs)
				memcpy(base_luminance, base_luminance_60hs_revA, sizeof(base_luminance_60hs_revA));
			else
				memcpy(base_luminance, base_luminance_60_revA, sizeof(base_luminance_60_revA));
		} else
			memcpy(base_luminance, base_luminance_120_revA, sizeof(base_luminance_120_revA));
		break;
	default:
		if (pSmart->rr == 60) {
			if (pSmart->sot_hs)
				memcpy(base_luminance, base_luminance_60hs_revA, sizeof(base_luminance_60hs_revA));
			else
				memcpy(base_luminance, base_luminance_60_revA, sizeof(base_luminance_60_revA));
		} else
			memcpy(base_luminance, base_luminance_120_revA, sizeof(base_luminance_120_revA));
		break;
	}

	for (cnt = 0; cnt < LUMINANCE_MAX; cnt++)
		if (base_luminance[cnt][0] == pSmart->brightness_level)
			return base_luminance[cnt][1];

	return -EINVAL;
}

static int get_gamma_curve(struct SMART_DIM *pSmart)
{
	int ret;

	if (pSmart->panel_revision < 'G')
		ret = GAMMA_CURVE_2P2;
	else
		ret = GAMMA_CURVE_2P2;

	return ret;
}

static int get_gradation_offset(int table_index, int index, struct SMART_DIM *pSmart)
{
	int ret = 0;

	switch (pSmart->panel_revision) {
	case 'A':
		if (pSmart->rr == 60) {
			if (pSmart->sot_hs)
				ret = gradation_offset_60hs_revA[table_index][index];
			else
				ret = gradation_offset_60_revA[table_index][index];
		} else
			ret = gradation_offset_120_revA[table_index][index];
		break;
	default:
		if (pSmart->rr == 60) {
			if (pSmart->sot_hs)
				ret = gradation_offset_60hs_revA[table_index][index];
			else
				ret = gradation_offset_60_revA[table_index][index];
		} else
			ret = gradation_offset_120_revA[table_index][index];
		break;
	}

	return ret;
}

static int get_rgb_offset(int table_index, int index, struct SMART_DIM *pSmart)
{
	int ret = 0;

	switch (pSmart->panel_revision) {
	case 'A':
		if (pSmart->rr == 60) {
			if (pSmart->sot_hs)
				ret = rgb_offset_60hs_revA[table_index][index];
			else
				ret = rgb_offset_60_revA[table_index][index];
		} else
			ret = rgb_offset_120_revA[table_index][index];
		break;
	default:
		if (pSmart->rr == 60) {
			if (pSmart->sot_hs)
				ret = rgb_offset_60hs_revA[table_index][index];
			else
				ret = rgb_offset_60_revA[table_index][index];
		} else
			ret = rgb_offset_120_revA[table_index][index];
		break;
	}

	return ret;
}

static void TP_L_calc(struct SMART_DIM *pSmart, long long *L, int base_level)
{
	long long temp_cal_data = 0;
	int point_index;
	int cnt;

	if (pSmart->brightness_level < get_max_candela()) {
		for (cnt = 0; cnt < V_MAX; cnt++) {
			point_index = INFLECTION_VOLTAGE_ARRAY[cnt];
			temp_cal_data =
				((long long)(candela_coeff_2p15[point_index])) *
				((long long)(base_level));

			L[cnt] = temp_cal_data;
		}

	} else {
		for (cnt = 0; cnt < V_MAX; cnt++) {
			point_index = INFLECTION_VOLTAGE_ARRAY[cnt];
			temp_cal_data =
				((long long)(candela_coeff_2p2[point_index])) *
				((long long)(base_level));

			L[cnt] = temp_cal_data;
		}
	}
}

static void TP_M_GRAY_calc(struct SMART_DIM *pSmart, long long *L, int *M_GRAY, int table_index)
{
	int i;

	M_GRAY[VT] = 0;
	M_GRAY[V0] = 0;
	M_GRAY[V1] = 1;

	for (i = V7; i < V_MAX; i++) {
		/* 3.6 - TP's Voltage Search */
		if (searching_function(L[i],
			&(M_GRAY[i]), get_gamma_curve(pSmart))) {
			pr_err("%s searching functioin error cnt:%d\n",
			__func__, i);
		}
	}

	/* 2.2.6 - add offset for Candela Gamma Compensation (V255~V7) */
	for (i = V255; i >= V7; i--) {
		if (table_index == -1) {
			table_index = LUMINANCE_MAX-1;
			pr_err("%s : fail candela table_index cnt : %d brightness %d\n",
				__func__, i, pSmart->brightness_level);
		}
		M_GRAY[i] += get_gradation_offset(table_index, V255-i, pSmart);
	}
}

static void Color_shift_compensation(struct SMART_DIM *pSmart, int *gamma_setting, int table_index)
{
	int i;
	u8 v255_10bit[RGB_MAX];
	u8 v255_9bit[RGB_MAX];
	int level_V255[RGB_MAX];

	v255_10bit[R] = !!(gamma_setting[0] & BIT(5));
	v255_10bit[G] = !!(gamma_setting[0] & BIT(3));
	v255_10bit[B] = !!(gamma_setting[0] & BIT(1));

	v255_9bit[R] = !!(gamma_setting[0] & BIT(4));
	v255_9bit[G] = !!(gamma_setting[0] & BIT(2));
	v255_9bit[B] = !!(gamma_setting[0] & BIT(0));

	level_V255[R] = gamma_setting[1] | (v255_10bit[R] << 9) | (v255_9bit[R] << 8);
	level_V255[G] = gamma_setting[2] | (v255_10bit[G] << 9) | (v255_9bit[G] << 8);
	level_V255[B] = gamma_setting[3] | (v255_10bit[B] << 9) | (v255_9bit[B] << 8);

	level_V255[R] += get_rgb_offset(table_index, 0, pSmart);
	level_V255[G] += get_rgb_offset(table_index, 1, pSmart);
	level_V255[B] += get_rgb_offset(table_index, 2, pSmart);

	v255_10bit[R] = !!(level_V255[R] & BIT(9));
	v255_10bit[G] = !!(level_V255[G] & BIT(9));
	v255_10bit[B] = !!(level_V255[B] & BIT(9));

	v255_9bit[R] = !!(level_V255[R] & BIT(8));
	v255_9bit[G] = !!(level_V255[G] & BIT(8));
	v255_9bit[B] = !!(level_V255[B] & BIT(8));

	gamma_setting[0] =
		v255_10bit[R] << 5 | v255_10bit[G] << 3 | v255_10bit[B] << 1 |
		v255_9bit[R] << 4 | v255_9bit[G] << 2 | v255_9bit[B] << 0;
	gamma_setting[1] = level_V255[R] & 0xFF;
	gamma_setting[2] = level_V255[G] & 0xFF;
	gamma_setting[3] = level_V255[B] & 0xFF;

	for (i = 3; i < RGB_COMPENSATION; i++) {
		if (table_index == -1) {
			table_index = LUMINANCE_MAX-1;
			pr_err("%s : fail RGB table_index cnt : %d brightness %d\n",
				__func__, i, pSmart->brightness_level);
		}

		gamma_setting[i+1] += get_rgb_offset(table_index, i, pSmart);
	}
}

static void gamma_init(struct SMART_DIM *pSmart, char *str, int size, int table_index)
{
#ifdef SMART_DIMMING_DEBUG
	int i;
#endif
	long long L[V_MAX] = {0, };
	int M_GRAY[V_MAX] = {0, };
	int gamma_setting[GAMMA_SET_MAX];
	int base_level = 0;
	int cnt;

	pr_debug("%s : start !! table_index(%d)\n", __func__, table_index);

	/* get base luminance */
	base_level = get_base_luminance(pSmart);
	if (base_level < 0)
		pr_err("%s : can not find base luminance!!\n", __func__);

	/* 2.2.5 (F) TP's Luminance  (x4194304) */
	TP_L_calc(pSmart, L, base_level);

	/* 3.5.1 M-Gray */
	TP_M_GRAY_calc(pSmart, L, M_GRAY, table_index);

#ifdef SMART_DIMMING_DEBUG
	pr_err("\n brightness_level (%d) \n %16s %8s\n",
			   pSmart->brightness_level, "L", "M_GRAY");

	for (i = 0; i < V_MAX; i++)
		pr_err("%5s %11llu %8d\n", V_LIST[i], L[i], M_GRAY[i]);
#endif

	/* 3.7.1 - Generate Gamma code */
	TP_gamma_code_calc(pSmart, M_GRAY, gamma_setting);

	/* 3.7.2 - Color Shift (RGB compensation) */
	Color_shift_compensation(pSmart, gamma_setting, table_index);

	/* subtration MTP_OFFSET value from generated gamma table */
	mtp_offset_substraction(pSmart, gamma_setting);

	/* To avoid overflow */
	for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
		str[cnt] = gamma_setting[cnt];
}

static void generate_hbm_gamma(struct SMART_DIM *psmart, char *str, int size)
{
#ifdef SMART_DIMMING_DEBUG
	int i;
	char log_buf[256];
#endif

	struct illuminance_table *ptable = (struct illuminance_table *)
						(&(psmart->hbm_interpolation_table));

	memcpy(str, &(ptable[psmart->hbm_brightness_level].gamma_setting), size);

#ifdef SMART_DIMMING_DEBUG
	memset(log_buf, 0x00, 256);
	for (i = 0; i < GAMMA_SET_MAX; i++)
		snprintf(log_buf + strnlen(log_buf, 256), 256, " %3d", str[i]);

	pr_err("generate_hbm_gamma[%d] : %s\n", psmart->hbm_brightness_level, log_buf);
	memset(log_buf, 0x00, 256);
	pr_err("\n");
#endif
}

static void generate_gamma(struct SMART_DIM *psmart, char *str, int size)
{
	int lux_loop;
	struct illuminance_table *ptable = (struct illuminance_table *)
						(&(psmart->gen_table));

	/* searching already generated gamma from table */
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		if (ptable[lux_loop].lux == psmart->brightness_level) {
			memcpy(str, &(ptable[lux_loop].gamma_setting), size);
			break;
		}
	}

	/* searching fail... Setting 300CD value on gamma table */
	if (lux_loop == psmart->lux_table_max) {
		pr_err("%s searching fail lux : %d\n", __func__,
				psmart->brightness_level);
		memcpy(str, max_lux_table, size);
	}

#ifdef SMART_DIMMING_DEBUG
	if (lux_loop != psmart->lux_table_max)
		pr_err("%s searching ok index : %d lux : %d", __func__,
			lux_loop, ptable[lux_loop].lux);
#endif
}

/*
	set max_lux_table for max lux
*/
static void set_max_lux_table(struct SMART_DIM *pSmart)
{
	int v, cnt;
	u8 v255_10bit[RGB_MAX];
	u8 v255_9bit[RGB_MAX];

	pr_err("%s ldi_revision: 0x%x\n", __func__, pSmart->ldi_revision);

	v255_10bit[R] = (pSmart->CENTER_GAMMA_V[V255][R] & BIT(9)) >> 4;
	v255_10bit[G] = (pSmart->CENTER_GAMMA_V[V255][G] & BIT(9)) >> 6;
	v255_10bit[B] = (pSmart->CENTER_GAMMA_V[V255][B] & BIT(9)) >> 8;

	v255_9bit[R] = (pSmart->CENTER_GAMMA_V[V255][R] & BIT(8)) >> 4;
	v255_9bit[G] = (pSmart->CENTER_GAMMA_V[V255][G] & BIT(8)) >> 6;
	v255_9bit[B] = (pSmart->CENTER_GAMMA_V[V255][B] & BIT(8)) >> 8;

	cnt = 0;
	max_lux_table[cnt++] =
		v255_10bit[R] | v255_10bit[G] | v255_10bit[B] |
		v255_9bit[R] | v255_9bit[G] | v255_9bit[B];

	/* V255 ~ V1 */
	for (v = V255; v >= V1; v--) {
		max_lux_table[cnt++] = pSmart->CENTER_GAMMA_V[v][R] & 0xFF;
		max_lux_table[cnt++] = pSmart->CENTER_GAMMA_V[v][G] & 0xFF;
		max_lux_table[cnt++] = pSmart->CENTER_GAMMA_V[v][B] & 0xFF;
	}

	/* V0, VT */
	max_lux_table[cnt++] =
		(pSmart->CENTER_GAMMA_V[V0][R] & 0xF) << 4 | (pSmart->CENTER_GAMMA_V[V0][G] & 0xF);
	max_lux_table[cnt++] =
		(pSmart->CENTER_GAMMA_V[V0][B] & 0xF) << 4 | (pSmart->CENTER_GAMMA_V[VT][R] & 0xF);
	max_lux_table[cnt++] =
		(pSmart->CENTER_GAMMA_V[VT][G] & 0xF) << 4 | (pSmart->CENTER_GAMMA_V[VT][B] & 0xF);
}

static int char_to_int(char data, bool negative)
{
	int cal_data;

	cal_data = (data & 0x7F);
	if (data & BIT(7)) {
		if (negative)
			cal_data *= -1;
		else
			cal_data += (data & 0x80);
	}

	return cal_data;
}

/*
	copy psmart->MTP_ORIGN to psmart->MTP, convert gamma[] -> V[][]
*/
static void mtp_sorting(unsigned char* src, int (*dst)[RGB_MAX], bool negative)
{
	int i, j;
	u8 v255_10bit[RGB_MAX];
	u8 v255_9bit[RGB_MAX];
	int cnt;

	/*
	 * Before the work, Please check '[Table.1] MTP OFFSET (C8h)' in OP Manual
	 */

	/* V255 */
	cnt = 0;
	v255_10bit[R] = !!(src[cnt] & BIT(5));
	v255_10bit[G] = !!(src[cnt] & BIT(3));
	v255_10bit[B] = !!(src[cnt] & BIT(1));

	v255_9bit[R] = !!(src[cnt] & BIT(4));
	v255_9bit[G] = !!(src[cnt] & BIT(2));
	v255_9bit[B] = !!(src[cnt] & BIT(0));

	cnt++;

	for (i = 0; i < RGB_MAX; i++) {
		dst[V255][i] = src[cnt++];
		if (v255_9bit[i])
			dst[V255][i] += BIT(8); // +256
		if (v255_10bit[i]) {
			if (negative)
				dst[V255][i] *= -1;
			else
				dst[V255][i] += BIT(9); // +512
		}
	}

	/* V203 ~ V1 */
	for (i = V203; i >= V1; i--) {
		for (j = 0; j < RGB_MAX; j++) {
			dst[i][j] = char_to_int(src[cnt++], negative);
		}
	}

	/* V0 */
	dst[V0][R] = (src[31] >> 4) & 0xF;
	dst[V0][G] = src[31] & 0xF;
	dst[V0][B] = (src[32] >> 4) & 0xF;

	/* VT */
	dst[VT][R] = src[32] & 0xF;
	dst[VT][G] = (src[33] >> 4) & 0xF;
	dst[VT][B] = src[33] & 0xF;
}

#define HBM_CANDELA 800
/* packed V255 ~ V1, each Vx has R/G/B, V1 and V0 are zero, and ignore it. */
#define PACKED_GAMM_SET_CNT 30
#define V255_RGB_MSB_CNT 3
static int hbm_interpolation_gamma[HBM_INTERPOLATION_STEP][PACKED_GAMM_SET_CNT];
static void hbm_interpolation_init(struct SMART_DIM *pSmart)
{
	int loop, gamma_index;
	int rate;
	int hbm_gamma[PACKED_GAMM_SET_CNT];
	int max_gamma[PACKED_GAMM_SET_CNT];
	char *hbm_payload;
	int normal_max_candela = pSmart->gen_table[LUMINANCE_MAX-1].lux;
	u8 v255_10bit[RGB_MAX];
	u8 v255_9bit[RGB_MAX];
	int *hbm_interpolation_candela_table;
	int hbm_steps;

	hbm_payload = pSmart->hbm_payload;
	if (!hbm_payload) {
		pr_err("%s : no hbm_payload..\n", __func__);
		return;
	}

	v255_10bit[R] = !!(hbm_payload[0] & BIT(5));
	v255_10bit[G] = !!(hbm_payload[0] & BIT(3));
	v255_10bit[B] = !!(hbm_payload[0] & BIT(1));

	v255_9bit[R] = !!(hbm_payload[0] & BIT(4));
	v255_9bit[G] = !!(hbm_payload[0] & BIT(2));
	v255_9bit[B] = !!(hbm_payload[0] & BIT(0));

	hbm_gamma[R] = (hbm_payload[R + 1] & 0xFF) | (v255_10bit[R] << 9) | (v255_9bit[R] << 8);
	hbm_gamma[G] = (hbm_payload[G + 1] & 0xFF) | (v255_10bit[G] << 9) | (v255_9bit[G] << 8);
	hbm_gamma[B] = (hbm_payload[B + 1] & 0xFF) | (v255_10bit[B] << 9) | (v255_9bit[B] << 8);

	// get values for V255 ~ V1.. VT and V0 are zero..
	for (gamma_index = 3; gamma_index < PACKED_GAMM_SET_CNT; gamma_index++)
		hbm_gamma[gamma_index] = hbm_payload[gamma_index + 1];

	v255_10bit[R] = !!(max_lux_table[0] & BIT(5));
	v255_10bit[G] = !!(max_lux_table[0] & BIT(3));
	v255_10bit[B] = !!(max_lux_table[0] & BIT(1));

	v255_9bit[R] = !!(max_lux_table[0] & BIT(4));
	v255_9bit[G] = !!(max_lux_table[0] & BIT(2));
	v255_9bit[B] = !!(max_lux_table[0] & BIT(0));

	max_gamma[R] = (max_lux_table[R + 1] & 0xFF) | (v255_10bit[R] << 9) | (v255_9bit[R] << 8);
	max_gamma[G] = (max_lux_table[G + 1] & 0xFF) | (v255_10bit[G] << 9) | (v255_9bit[G] << 8);
	max_gamma[B] = (max_lux_table[B + 1] & 0xFF) | (v255_10bit[B] << 9) | (v255_9bit[B] << 8);

	for (gamma_index = 3; gamma_index < PACKED_GAMM_SET_CNT; gamma_index++)
		max_gamma[gamma_index] = max_lux_table[gamma_index + 1];

	/* generate interpolation hbm gamma */
	if (pSmart->panel_revision <= 'A') {
		hbm_interpolation_candela_table = hbm_interpolation_candela_table_revA;
		hbm_steps = HBM_INTERPOLATION_STEP;
	} else {
		hbm_interpolation_candela_table = hbm_interpolation_candela_table_revA;
		hbm_steps = HBM_INTERPOLATION_STEP;
	}

	for (loop = 0 ; loop < hbm_steps; loop++) {
		rate = ((hbm_interpolation_candela_table[loop] - normal_max_candela) * BIT_SHFIT_MUL) /
			(HBM_CANDELA - normal_max_candela);
		for (gamma_index = 0; gamma_index < PACKED_GAMM_SET_CNT; gamma_index++)
			hbm_interpolation_gamma[loop][gamma_index] = max_gamma[gamma_index] +
				((hbm_gamma[gamma_index] - max_gamma[gamma_index]) * rate) / BIT_SHFIT_MUL;
	}

	/* update max hbm gamma with origin hbm gamma */
	for (gamma_index = 0; gamma_index < PACKED_GAMM_SET_CNT; gamma_index++)
		hbm_interpolation_gamma[hbm_steps-1][gamma_index] = hbm_gamma[gamma_index];

	/* translate to HBM gamma packet format */
	for (loop = 0; loop < hbm_steps; loop++) {
		u8 v255_10bit[RGB_MAX];
		u8 v255_9bit[RGB_MAX];

		/* V255 R,G,B */
		v255_10bit[R] = !!(hbm_interpolation_gamma[loop][R] & BIT(9));
		v255_10bit[G] = !!(hbm_interpolation_gamma[loop][G] & BIT(9));
		v255_10bit[B] = !!(hbm_interpolation_gamma[loop][B] & BIT(9));

		v255_9bit[R] = !!(hbm_interpolation_gamma[loop][R] & BIT(8));
		v255_9bit[G] = !!(hbm_interpolation_gamma[loop][G] & BIT(8));
		v255_9bit[B] = !!(hbm_interpolation_gamma[loop][B] & BIT(8));

		pSmart->hbm_interpolation_table[loop].gamma_setting[0] =
			(v255_10bit[R] << 5) | (v255_10bit[G] << 3) | (v255_10bit[B] << 1) |
			(v255_9bit[R] << 4) | (v255_9bit[G] << 2) | (v255_9bit[B] << 0);

		for (gamma_index = 0; gamma_index < PACKED_GAMM_SET_CNT; gamma_index++)
			pSmart->hbm_interpolation_table[loop].gamma_setting[gamma_index + 1] =
				hbm_interpolation_gamma[loop][gamma_index] & 0xFF;
	}

#ifdef SMART_DIMMING_DEBUG
	print_hbm_lux_table(pSmart, "DEC");
	print_hbm_lux_table(pSmart, "HEX");
#endif
	return;
}

static int smart_dimming_init(struct SMART_DIM *psmart)
{
	int lux_loop;

#ifdef SMART_DIMMING_DEBUG
	char pBuffer[256];
	memset(pBuffer, 0x00, 256);
#endif
	id1 = (psmart->ldi_revision & 0x00FF0000) >> 16;
	id2 = (psmart->ldi_revision & 0x0000FF00) >> 8;
	id3 = psmart->ldi_revision & 0xFF;

	LCD_INFO("++ id3(%d), panel_revision(%c) rr(%d) sot(%d)\n",
			id3, psmart->panel_revision, psmart->rr, psmart->sot_hs);

	mtp_sorting(psmart->MTP_ORIGN, psmart->MTP, true);
	set_max_lux_table(psmart);

#ifdef SMART_DIMMING_DEBUG
	print_RGB_offset(psmart);
#endif

	psmart->vregout_voltage = get_vreg_voltage(psmart->panel_revision);
	psmart->vref = get_vref(psmart->panel_revision);

	/********************************************************************************************/
	/* 2.3.3 - each TP's gamma voltage calculation 											 	*/
	/* 1. VT, V0, V255 : VREG1 - (VREG1 - VREF) * (OutputGamma + numerator_TP)/(denominator_TP) 			 	*/
	/* 2. VT1, VT7 : VREG1 - (VREG1 - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP) */
	/* 3. other VT : VT - (VT - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP)		*/
	/********************************************************************************************/
	TP_gamma_voltage_calc(psmart);

	/* 2.3.5 D - Gray Output Voltage */
	if (generate_gray_scale(psmart)) {
		pr_err(KERN_ERR "lcd smart dimming fail generate_gray_scale\n");
		return -EINVAL;
	}

	/* 3.7 - Generating lux_table */
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		/* To set brightness value */
		psmart->brightness_level = psmart->plux_table[lux_loop];
		/* To make lux table index*/
		psmart->gen_table[lux_loop].lux = psmart->plux_table[lux_loop];

		gamma_init(psmart,
			(char *)(&(psmart->gen_table[lux_loop].gamma_setting)),
			GAMMA_SET_MAX, lux_loop);
	}

	/* set 420CD max gamma table */
	memcpy(&(psmart->gen_table[LUMINANCE_MAX-1].gamma_setting),
			max_lux_table, GAMMA_SET_MAX);

	hbm_interpolation_init(psmart);

#ifdef SMART_DIMMING_DEBUG
	print_lux_table(psmart, "DEC");
	print_lux_table(psmart, "HEX");
#endif

	pr_err("%s done\n", __func__);

	return 0;
}

static void wrap_generate_hbm_gamma(struct smartdim_conf *conf, int hbm_brightness_level, char *cmd_str)
{
	struct SMART_DIM *smart = conf->psmart;

	if (!smart) {
		pr_info("%s fail", __func__);
		return ;
	}

	/* TODO: auto_brightness is 13 to use 443cd of hbm step on color weakness mode
	if (hbm_brightness_level == HBM_MODE + 7)
		smart->hbm_brightness_level = -1;
	else
		smart->hbm_brightness_level = hbm_brightness_level - 6;
	 */

	smart->hbm_brightness_level = hbm_brightness_level - HBM_MODE;

	generate_hbm_gamma(conf->psmart, cmd_str, GAMMA_SET_MAX);
}

/* ----------------------------------------------------------------------------
 * Wrapper functions for smart dimming
 * ----------------------------------------------------------------------------
 */
static void wrap_generate_gamma(struct smartdim_conf *conf, int cd, char *cmd_str)
{

	struct SMART_DIM *smart = conf->psmart;

	if (!smart) {
		pr_err("%s fail", __func__);
		return ;
	}

	smart->brightness_level = cd;
	generate_gamma(conf->psmart, cmd_str, GAMMA_SET_MAX);
}

static void wrap_smart_dimming_init(struct smartdim_conf *conf)
{
	struct SMART_DIM *smart = conf->psmart;

	if (!smart) {
		pr_err("%s fail", __func__);
		return ;
	}

	smart->plux_table = conf->lux_tab;
	smart->lux_table_max = conf->lux_tabsize;
	smart->ldi_revision = conf->man_id;
	smart->hbm_payload = conf->hbm_payload;
	smart->panel_revision = conf->panel_revision;
	smart->rr = conf->rr;
	smart->sot_hs = conf->sot_hs;

	/* center gamma of 60NR is fixed value.
	 * center gamma of others is value read from ddi.
	 */
	memcpy(smart->CENTER_GAMMA_V, center_gamma, sizeof(center_gamma));

	/* overwrite some values of center_gamma from read value */
	if (conf->rr == 60 && conf->sot_hs) {
		mtp_sorting(conf->center_gamma_60hs, smart->CENTER_GAMMA_60HS_V, false);
		memcpy(smart->CENTER_GAMMA_V, smart->CENTER_GAMMA_60HS_V, sizeof(smart->CENTER_GAMMA_60HS_V));
	} else if (conf->rr == 120) {
		mtp_sorting(conf->center_gamma_120hs, smart->CENTER_GAMMA_120HS_V, false);
		memcpy(smart->CENTER_GAMMA_V, smart->CENTER_GAMMA_120HS_V, sizeof(smart->CENTER_GAMMA_120HS_V));
	}

	LCD_ERR("rr[%d] sot[%d]\n", smart->rr, smart->sot_hs);

	if (smart->lux_table_max != LUMINANCE_MAX)
		pr_err("%s : [ERROR] LUMINANCE_MAX(%d) lux_table_max(%d) are different!\n",
				__func__, LUMINANCE_MAX, smart->lux_table_max);

	smart_dimming_init(smart);
}

struct smartdim_conf *smart_get_conf_S6E3HAB_AMB623TS01(void)
{

	struct smartdim_conf *smartdim_conf;
	struct SMART_DIM *smart;

	smartdim_conf = kzalloc(sizeof(struct smartdim_conf), GFP_KERNEL);
	if (!smartdim_conf) {
		pr_err("%s allocation fail", __func__);
		goto out2;
	}

	smart = kzalloc(sizeof(struct SMART_DIM), GFP_KERNEL);
	if (!smart) {
		pr_err("%s allocation fail", __func__);
		goto out1;
	}

	smartdim_conf->psmart = smart;
	smartdim_conf->generate_gamma = wrap_generate_gamma;
	smartdim_conf->generate_hbm_gamma = wrap_generate_hbm_gamma;
	smartdim_conf->init = wrap_smart_dimming_init;
	smartdim_conf->get_min_lux_table = NULL;
	smartdim_conf->mtp_buffer = smart->MTP_ORIGN;
	smartdim_conf->center_gamma_60hs = smart->CENTER_GAMMA_60HS;
	smartdim_conf->center_gamma_120hs = smart->CENTER_GAMMA_120HS;
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

/* ============================================================================
 *  HMT
 * ============================================================================
 */

static void print_lux_table_hmt(struct SMART_DIM *psmart)
{
	int lux_loop;
	int cnt;
	char pBuffer[256];
	int gamma;
	int first;
	int temp;
	memset(pBuffer, 0x00, 256);

	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		first = psmart->hmt_gen_table[lux_loop].gamma_setting[0];
		for (cnt = 1; cnt < GAMMA_SET_MAX - 3; cnt++) {
			gamma = psmart->hmt_gen_table[lux_loop].gamma_setting[cnt];
			if (cnt == 1) {
				gamma += (first & BIT(4) ? BIT(8) : 0);
				gamma += (first & BIT(5) ? BIT(9) : 0);
			} else if (cnt == 2) {
				gamma += (first & BIT(2) ? BIT(8) : 0);
				gamma += (first & BIT(3) ? BIT(9) : 0);
			} else if (cnt == 3) {
				gamma += (first & BIT(0) ? BIT(8) : 0);
				gamma += (first & BIT(1) ? BIT(9) : 0);
			}

			snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d", gamma);
		}
		/* V0, VT */
		for (cnt = GAMMA_SET_MAX - 3; cnt < GAMMA_SET_MAX; cnt++) {
			temp = psmart->hmt_gen_table[lux_loop].gamma_setting[cnt];

			gamma = (temp & 0xF0) >> 4;
			snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d", gamma);
			gamma = temp & 0x0F;
			snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %3d", gamma);
		}
		LCD_INFO("lux : %3d  %s\n", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
}


static void print_aid_log_hmt(struct smartdim_conf *conf)
{
	LCD_INFO("== print_aid_log_hmt ==\n");
	print_RGB_offset(conf->psmart);
	print_lux_table_hmt(conf->psmart);
	LCD_INFO("\n");
}

static int get_base_luminance_hmt(int brightness_level, int panel_revision)
{
	int cnt;
	int base_luminance[HMT_LUMINANCE_MAX][2];

	switch (panel_revision) {
	case 'A':
		memcpy(base_luminance, base_luminance_reverse_hmt_single, sizeof(base_luminance_reverse_hmt_single));
		break;
	default:
		memcpy(base_luminance, base_luminance_reverse_hmt_single, sizeof(base_luminance_reverse_hmt_single));
		break;
	}

	for (cnt = 0; cnt < HMT_LUMINANCE_MAX; cnt++)
		if (base_luminance[cnt][0] == brightness_level)
			return base_luminance[cnt][1];

	return -EINVAL;
}

static int get_gradation_offset_hmt(int table_index, int index, int panel_revision)
{
	switch (panel_revision) {
	case 'A':
		return gradation_offset_reverse_hmt_single[table_index][index];
	default:
		return gradation_offset_reverse_hmt_single[table_index][index];
	}
}

static int get_rgb_offset_hmt(int table_index, int index, int panel_revision)
{
	switch (panel_revision) {
	case 'A':
		return rgb_offset_reverse_hmt_single[table_index][index];
	default:
		return rgb_offset_reverse_hmt_single[table_index][index];
	}
}

static void TP_L_calc_hmt(struct SMART_DIM *pSmart, long long *L, int base_level)
{
	long long temp_cal_data = 0;
	int point_index;
	int cnt;

	for (cnt = 0; cnt < V_MAX; cnt++) {
		point_index = INFLECTION_VOLTAGE_ARRAY[cnt];
		temp_cal_data =
			((long long)(candela_coeff_2p15[point_index])) *
			((long long)(base_level));

		L[cnt] = temp_cal_data;
	}
}

static void TP_M_GRAY_calc_hmt(struct SMART_DIM *pSmart, long long *L, int *M_GRAY, int table_index)
{
	int i;

	M_GRAY[VT] = 0;
	M_GRAY[V0] = 0;
	M_GRAY[V1] = 1;

	for (i = V7; i < V_MAX; i++) {
		/* 3.6 - TP's Voltage Search */
		if (searching_function(L[i],
			&(M_GRAY[i]), get_gamma_curve(pSmart))) {
			pr_err("%s searching functioin error cnt:%d\n",
			__func__, i);
		}
	}

	/* 2.2.6 - add offset for Candela Gamma Compensation (V255~V7) */
	for (i = V255; i >= V7; i--) {
		if (table_index == -1) {
			table_index = HMT_LUMINANCE_MAX-1;
			pr_err("%s : fail candela table_index cnt : %d brightness %d\n",
				__func__, i, pSmart->brightness_level);
		}
		M_GRAY[i] += get_gradation_offset_hmt(table_index, V255-i, pSmart->panel_revision);
	}
}

static void Color_shift_compensation_hmt(struct SMART_DIM *pSmart, int *gamma_setting, int table_index)
{
	int i;
	u8 v255_10bit[RGB_MAX];
	u8 v255_9bit[RGB_MAX];
	int level_V255[RGB_MAX];

	v255_10bit[R] = !!(gamma_setting[0] & BIT(5));
	v255_10bit[G] = !!(gamma_setting[0] & BIT(3));
	v255_10bit[B] = !!(gamma_setting[0] & BIT(1));

	v255_9bit[R] = !!(gamma_setting[0] & BIT(4));
	v255_9bit[G] = !!(gamma_setting[0] & BIT(2));
	v255_9bit[B] = !!(gamma_setting[0] & BIT(0));

	level_V255[R] = gamma_setting[1] | (v255_10bit[R] << 9) | (v255_9bit[R] << 8);
	level_V255[G] = gamma_setting[2] | (v255_10bit[G] << 9) | (v255_9bit[G] << 8);
	level_V255[B] = gamma_setting[3] | (v255_10bit[B] << 9) | (v255_9bit[B] << 8);

	level_V255[R] += get_rgb_offset_hmt(table_index, 0, pSmart->panel_revision);
	level_V255[G] += get_rgb_offset_hmt(table_index, 1, pSmart->panel_revision);
	level_V255[B] += get_rgb_offset_hmt(table_index, 2, pSmart->panel_revision);

	v255_10bit[R] = !!(level_V255[R] & BIT(9));
	v255_10bit[G] = !!(level_V255[G] & BIT(9));
	v255_10bit[B] = !!(level_V255[B] & BIT(9));

	v255_9bit[R] = !!(level_V255[R] & BIT(8));
	v255_9bit[G] = !!(level_V255[G] & BIT(8));
	v255_9bit[B] = !!(level_V255[B] & BIT(8));

	gamma_setting[0] =
		v255_10bit[R] << 5 | v255_10bit[G] << 3 | v255_10bit[B] << 1 |
		v255_9bit[R] << 4 | v255_9bit[G] << 2 | v255_9bit[B] << 0;
	gamma_setting[1] = level_V255[R] & 0xFF;
	gamma_setting[2] = level_V255[G] & 0xFF;
	gamma_setting[3] = level_V255[B] & 0xFF;

	for (i = 3; i < RGB_COMPENSATION; i++) {
		if (table_index == -1) {
			table_index = HMT_LUMINANCE_MAX-1;
			pr_err("%s : fail RGB table_index cnt : %d brightness %d\n",
				__func__, i, pSmart->brightness_level);
		}

		gamma_setting[i+1] += get_rgb_offset_hmt(table_index, i, pSmart->panel_revision);
	}
}

#define TABLE_MAX  (V_MAX-1)

static void gamma_init_hmt(struct SMART_DIM *pSmart, char *str, int size, int table_index)
{
#ifdef SMART_DIMMING_DEBUG
	int i;
#endif
	long long L[V_MAX] = {0, };
	int M_GRAY[V_MAX] = {0, };
	int gamma_setting[GAMMA_SET_MAX];
	int base_level = 0;
	int cnt;

	pr_debug("%s : start !! table_index(%d)\n", __func__, table_index);

	/* get base luminance */
	base_level = get_base_luminance_hmt(pSmart->brightness_level, pSmart->panel_revision);
	if (base_level < 0)
		pr_err("%s : can not find base luminance!!\n", __func__);

	/* 2.2.5 (F) TP's Luminance  (x4194304) */
	TP_L_calc_hmt(pSmart, L, base_level);

	/* 3.5.1 M-Gray */
	TP_M_GRAY_calc_hmt(pSmart, L, M_GRAY, table_index);

#ifdef SMART_DIMMING_DEBUG
	pr_err("\n brightness_level (%d) \n %16s %8s\n",
			   pSmart->brightness_level, "L", "M_GRAY");

	for (i = 0; i < V_MAX; i++)
		pr_err("%5s %11llu %8d\n", V_LIST[i], L[i], M_GRAY[i]);
#endif

	/* 3.7.1 - Generate Gamma code */
	TP_gamma_code_calc(pSmart, M_GRAY, gamma_setting);

	/* 3.7.2 - Color Shift (RGB compensation) */
	Color_shift_compensation_hmt(pSmart, gamma_setting, table_index);

	/* subtration MTP_OFFSET value from generated gamma table */
	mtp_offset_substraction(pSmart, gamma_setting);

	/* To avoid overflow */
	for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
		str[cnt] = gamma_setting[cnt];
}

static void generate_gamma_hmt(struct SMART_DIM *psmart, char *str, int size)
{
	int lux_loop;
	struct illuminance_table *ptable = (struct illuminance_table *)
						(&(psmart->hmt_gen_table));

	/* searching already generated gamma table */
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		if (ptable[lux_loop].lux == psmart->brightness_level) {
			memcpy(str, &(ptable[lux_loop].gamma_setting), size);
			break;
		}
	}

	/* searching fail... Setting 150CD value on gamma table */
	if (lux_loop == psmart->lux_table_max) {
		pr_err("%s searching fail lux : %d\n", __func__,
				psmart->brightness_level);
		memcpy(str, max_lux_table, size);
	}

#ifdef SMART_DIMMING_DEBUG
	if (lux_loop != psmart->lux_table_max)
		pr_err("%s searching ok index : %d lux : %d", __func__,
			lux_loop, ptable[lux_loop].lux);
#endif
}

static int smart_dimming_init_hmt(struct SMART_DIM *psmart)
{
	int lux_loop;

#ifdef SMART_DIMMING_DEBUG
	char pBuffer[256];
	memset(pBuffer, 0x00, 256);
#endif
	id1 = (psmart->ldi_revision & 0x00FF0000) >> 16;
	id2 = (psmart->ldi_revision & 0x0000FF00) >> 8;
	id3 = psmart->ldi_revision & 0xFF;

	LCD_INFO("++ id3(%d), panel_revision(%c)\n",
			id3, psmart->panel_revision);

	mtp_sorting(psmart->MTP_ORIGN, psmart->MTP, true);
	set_max_lux_table(psmart);

#ifdef SMART_DIMMING_DEBUG
	print_RGB_offset(psmart);
#endif

	psmart->vregout_voltage = get_vreg_voltage(psmart->panel_revision);
	psmart->vref = get_vref(psmart->panel_revision);

	/********************************************************************************************/
	/* 2.3.3 - each TP's gamma voltage calculation 											 	*/
	/* 1. VT, V0, V255 : VREG1 - (VREG1 - VREF) * (OutputGamma + numerator_TP)/(denominator_TP) 			 	*/
	/* 2. VT1, VT7 : VREG1 - (VREG1 - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP) */
	/* 3. other VT : VT - (VT - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP)		*/
	/********************************************************************************************/
	TP_gamma_voltage_calc(psmart);

	/* 2.3.5 D - Gray Output Voltage */
	if (generate_gray_scale(psmart)) {
		pr_err(KERN_ERR "lcd smart dimming fail generate_gray_scale\n");
		return -EINVAL;
	}

	/* 3.7 - Generating lux_table */
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		/* To set brightness value */
		psmart->brightness_level = psmart->plux_table[lux_loop];
		/* To make lux table index*/
		psmart->hmt_gen_table[lux_loop].lux = psmart->plux_table[lux_loop];

		gamma_init_hmt(psmart,
			(char *)(&(psmart->hmt_gen_table[lux_loop].gamma_setting)),
			GAMMA_SET_MAX, lux_loop);
	}

#ifdef SMART_DIMMING_DEBUG
	print_lux_table_hmt(psmart);
#endif

	pr_err("%s done\n", __func__);

	return 0;
}

static void wrap_generate_gamma_hmt(struct smartdim_conf *conf, int cd, char *cmd_str)
{

	struct SMART_DIM *smart = conf->psmart;

	if (!smart) {
		pr_err("%s fail", __func__);
		return ;
	}

	smart->brightness_level = cd;
	generate_gamma_hmt(conf->psmart, cmd_str, GAMMA_SET_MAX);
}

static void wrap_smart_dimming_init_hmt(struct smartdim_conf *conf)
{

	struct SMART_DIM *smart = conf->psmart;

	if (!smart) {
		pr_err("%s fail", __func__);
		return;
	}

	smart->plux_table = conf->lux_tab;
	smart->lux_table_max = conf->lux_tabsize;
	smart->ldi_revision = conf->man_id;
	smart->panel_revision = conf->panel_revision;

	/* center gamma of 60NR is fixed value.
	 * center gamma of others is value read from ddi.
	 */
	memcpy(smart->CENTER_GAMMA_V, center_gamma_hmt, sizeof(center_gamma_hmt));

	smart_dimming_init_hmt(smart);
}

struct smartdim_conf *smart_get_conf_S6E3HAB_AMB623TS01_hmt(void)
{
	struct smartdim_conf *smartdim_conf_hmt;
	struct SMART_DIM *smart_hmt;

	smartdim_conf_hmt = kzalloc(sizeof(struct smartdim_conf), GFP_KERNEL);
	if (!smartdim_conf_hmt) {
		pr_err("%s allocation fail", __func__);
		goto out2;
	}

	smart_hmt = kzalloc(sizeof(struct SMART_DIM), GFP_KERNEL);
	if (!smart_hmt) {
		pr_err("%s allocation fail", __func__);
		goto out1;
	}

	smartdim_conf_hmt->psmart = smart_hmt;
	smartdim_conf_hmt->generate_gamma = wrap_generate_gamma_hmt;
	smartdim_conf_hmt->init = wrap_smart_dimming_init_hmt;
	smartdim_conf_hmt->get_min_lux_table = NULL;
	smartdim_conf_hmt->mtp_buffer = (char *)(&smart_hmt->MTP_ORIGN);
	smartdim_conf_hmt->print_aid_log = print_aid_log_hmt;

	return smartdim_conf_hmt;

out1:
	kfree(smartdim_conf_hmt);
out2:
	return NULL;
}
