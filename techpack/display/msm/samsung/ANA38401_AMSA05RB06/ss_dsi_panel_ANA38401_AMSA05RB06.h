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
#ifndef SAMSUNG_DSI_PANEL_ANA38401_AMSA05RB06_H
#define SAMSUNG_DSI_PANEL_ANA38401_AMSA05RB06_H

#include <linux/completion.h>
#include "ss_dsi_panel_common.h"

#define HBM_INTERPOLATION_STEP 8
#define SUPPORT_LOWTEMP_ELVSS 15

enum {
	MID_TEMP = 0,
	LOW_TEMP,
	HIGH_TEMP,
	MAX_TEMP
};

struct smartdim_conf *smart_get_conf_ANA38401_AMSA05RB06(void);
struct smartdim_conf *smart_get_conf_ANA38401_AMSA05RB06_hmt(void);
#endif
