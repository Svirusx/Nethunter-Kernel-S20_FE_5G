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
#ifndef SAMSUNG_DSI_PANEL_NT36523_PPA957DB1_H
#define SAMSUNG_DSI_PANEL_NT36523_PPA957DB1_H

#include <linux/completion.h>
#include "ss_dsi_panel_common.h"

enum {
	MID_TEMP = 0,
	LOW_TEMP,
	HIGH_TEMP,
	MAX_TEMP
};

extern int isl98608_buck_control(struct samsung_display_driver_data *vdd);

#endif
