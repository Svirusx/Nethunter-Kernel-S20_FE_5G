/*
 * =================================================================
 *
 *	Description:  samsung display sysfs common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2015, Samsung Electronics. All rights reserved.

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

#ifndef SAMSUNG_DSI_PANEL_SYSFS_H
#define SAMSUNG_DSI_PANEL_SYSFS_H

#include "ss_dsi_panel_common.h"
#include "dsi_phy.h"

struct samsung_display_driver_data;
/* PANEL SYSFS FUNCTION */
int ss_create_sysfs(struct samsung_display_driver_data *vdd);
void ss_cabc_update(struct samsung_display_driver_data *vdd);
#endif
