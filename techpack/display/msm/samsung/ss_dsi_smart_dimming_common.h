/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
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

#include <linux/kernel.h>

#ifndef _SAMSUNG_DSI_SMART_DIMMING_H_
#define _SAMSUNG_DSI_SMART_DIMMING_H_

struct smartdim_conf {
	void (*generate_gamma)(struct smartdim_conf *conf, int cd, char *str);
	void (*generate_hbm_gamma)(struct smartdim_conf *conf, int cd, char *str);
	void (*init)(struct smartdim_conf *conf);
	void (*print_aid_log)(struct smartdim_conf *conf);
	void (*get_min_lux_table)(char *str, int size);


	struct SMART_DIM *psmart;
	char *mtp_buffer;
	char *center_gamma_60hs;
	char *center_gamma_120hs;
	int *lux_tab;
	int lux_tabsize;
	unsigned int man_id;
	char panel_revision;
	int rr;	/* refresh rate*/
	bool sot_hs;	/* sot normal/hs */

	/* HBM interpolation */
	unsigned char *hbm_payload;
};

/* Define the gamma */
#define GAMMA_INDEX_MAX 256

/*
*		index : 0 ~ 255
*		((index/255)^2.25)*4194304
*/
extern int candela_coeff_2p25[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^2.2)*4194304
*/
extern int candela_coeff_2p2[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^2.15)*4194304
*/
extern int candela_coeff_2p15[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^2.12)*4194304
*/
extern int candela_coeff_2p12[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^2.1)*4194304
*/
extern int candela_coeff_2p1[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^2.05)*4194304
*/
extern int candela_coeff_2p05[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^2.0)*4194304
*/
extern int candela_coeff_2p0[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^1.95)*4194304
*/
extern int candela_coeff_1p95[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^1.9)*4194304
*/
extern int candela_coeff_1p9[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^1.85)*4194304
*/
extern int candela_coeff_1p85[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^1.8)*4194304
*/
extern int candela_coeff_1p8[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^1.75)*4194304
*/
extern int candela_coeff_1p75[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^1.7)*4194304
*/
extern int candela_coeff_1p7[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^1.65)*4194304
*/
extern int candela_coeff_1p65[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		((index/255)^1.6)*4194304
*/
extern int candela_coeff_1p6[GAMMA_INDEX_MAX];





/*
*		index : 0 ~ 255
*		300 is max CANDELA
*		(300*((index/255)^2.2))*4194304
*/
extern int curve_2p2_300[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		300 is max CANDELA
*		(300*((index/255)^1.9))*4194304
*/
extern int curve_1p9_300[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		300 is max CANDELA
*		(300*((index/255)^1.95))*4194304
*/
extern int curve_1p95_300[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		300 is max CANDELA
*		(300*((index/255)^2.0))*4194304
*/
extern int curve_2p0_300[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		300 is max CANDELA
*		(300*((index/255)^2.05))*4194304
*/
extern int curve_2p05_300[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		300 is max CANDELA
*		(300*((index/255)^2.1))*4194304
*/
extern int curve_2p1_300[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		300 is max CANDELA
*		(300*((index/255)^2.12))*4194304
*/
extern int curve_2p12_300[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		350 is max CANDELA
*		(350*((index/255)^1.9))*4194304
*/
extern int curve_1p9_350[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		350 is max CANDELA
*		(350*((index/255)^2.0))*4194304
*/
extern int curve_2p0_350[GAMMA_INDEX_MAX];


/*
*		index : 0 ~ 255
*		350 is max CANDELA
*		(350*((index/255)^2.15))*4194304
*/
extern int curve_2p15_350[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		350 is max CANDELA
*		(350*((index/255)^2.2))*4194304
*/
extern int curve_2p2_350[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		360 is max CANDELA
*		(360*((index/255)^1.9))*4194304
*/
extern int curve_1p9_360[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		360 is max CANDELA
*		(360*((index/255)^2.15))*4194304
*/
extern int curve_2p15_360[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		420 is max CANDELA
*		(420*((index/255)^2.15))*4194304
*/
extern int curve_2p15_420[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		360 is max CANDELA
*		(360*((index/255)^2.2))*4194304
*/
extern int curve_2p2_360[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		420 is max CANDELA
*		(420*((index/255)^2.2))*4194304
*/
extern int curve_2p2_420[GAMMA_INDEX_MAX];

/*
*		index : 0 ~ 255
*		400 is max CANDELA
*		(400*((index/255)^2.2))*4194304
*/
extern int curve_2p2_400[GAMMA_INDEX_MAX];


#endif /* _SAMSUNG_DSI_SMART_DIMMING_H_ */
