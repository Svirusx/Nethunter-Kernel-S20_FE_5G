/*
 * =================================================================
 *
 *	Description: Header file for samsung panel notify
 *	Company: Samsung Electronics
 *
 * ================================================================
 *
 *
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018, Samsung Electronics. All rights reserved.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __SS_PANEL_NOTIFY_H__
#define __SS_PANEL_NOTIFY_H__

enum panel_notifier_event_t {
	PANEL_EVENT_BL_CHANGED,
	PANEL_EVENT_VRR_CHANGED,
	PANEL_EVENT_STATE_CHANGED,
	PANEL_EVENT_LFD_CHANGED,
	PANEL_EVENT_UB_CON_CHANGED,
};

enum panel_notifier_event_ub_con_state {
	PANEL_EVENT_UB_CON_CONNECTED = 0,
	PANEL_EVENT_UB_CON_DISCONNECTED = 1,
};

struct panel_ub_con_event_data {
	enum panel_notifier_event_ub_con_state state;
	int display_idx;
};

struct panel_bl_event_data {
	int bl_level;
	int aor_data;
	int display_idx;
};

struct panel_dms_data {
	int fps;
	int lfd_min_freq;
	int lfd_max_freq;
	int display_idx;
};

enum panel_state {
	PANEL_OFF,
	PANEL_ON,
	PANEL_LPM,
	MAX_PANEL_STATE,
};

struct panel_state_data {
	enum panel_state state;
};

extern int ss_panel_notifier_register(struct notifier_block *nb);
extern int ss_panel_notifier_unregister(struct notifier_block *nb);
extern int ss_panel_notifier_call_chain(unsigned long val, void *v);

#endif /*__PANEL_NOTIFY_H__*/
