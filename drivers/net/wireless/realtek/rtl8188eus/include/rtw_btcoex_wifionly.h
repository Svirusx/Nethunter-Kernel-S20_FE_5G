/******************************************************************************
 *
 * Copyright(c) 2013 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef __RTW_BTCOEX_WIFIONLY_H__
#define __RTW_BTCOEX_WIFIONLY_H__

void rtw_btcoex_wifionly_switchband_notifyx(PADAPTER padapter);
void rtw_btcoex_wifionly_scan_notifyx(PADAPTER padapter);
void rtw_btcoex_wifionly_connect_notifyx(PADAPTER padapter);
void rtw_btcoex_wifionly_hw_configx(PADAPTER padapter);
void rtw_btcoex_wifionly_initializex(PADAPTER padapter);
void rtw_btcoex_wifionly_AntInfoSettingx(PADAPTER padapter);
#endif
