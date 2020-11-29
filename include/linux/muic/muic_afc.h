/*
 * Copyright (C) 2010 Samsung Electronics
 * Hyoyoung Kim <hyway.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __MUIC_AFC_H__
#define __MUIC_AFC_H__
#include <../drivers/muic/universal/muic-internal.h>

struct muic_data_t;

/* SM5705 AFC CTRL register */
#define AFCCTRL_ENQC20		 6
#define AFCCTRL_DIS_AFC		5
#define AFCCTRL_VBUS_READ    3
#define AFCCTRL_DM_RESET     2
#define AFCCTRL_DP_RESET     1
#define AFCCTRL_ENAFC        0

enum {
	AFC_5V	= 5,
	AFC_9V	= 9,
};

int muic_check_afc_state(int state);
int muic_torch_prepare(int state);
void muic_init_afc_state(muic_data_t *pmuic);
void muic_afc_delay_check_state(int state);
int sm5705_muic_afc_set_voltage(int voltage);
int muic_restart_afc(int noti);
int muic_dpreset_afc(void);

#endif /* __MUIC_AFC_H__ */

