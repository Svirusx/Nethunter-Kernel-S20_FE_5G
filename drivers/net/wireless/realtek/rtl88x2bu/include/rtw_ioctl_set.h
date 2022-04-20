/******************************************************************************
 *
 * Copyright(c) 2007 - 2019 Realtek Corporation.
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
#ifndef __RTW_IOCTL_SET_H_
#define __RTW_IOCTL_SET_H_

u8 rtw_set_802_11_authentication_modebu(_adapter *pdapter, NDIS_802_11_AUTHENTICATION_MODE authmode);
u8 rtw_set_802_11_bssidbu(_adapter *padapter, u8 *bssid);
u8 rtw_set_802_11_add_wepbu(_adapter *padapter, NDIS_802_11_WEP *wep);
u8 rtw_set_802_11_disassociatebu(_adapter *padapter);
u8 rtw_set_802_11_bssidbu_list_scan(_adapter *padapter, struct sitesurvey_parm *pparm);
#ifdef CONFIG_RTW_ACS
u8 rtw_set_acs_sitesurvey(_adapter *adapter);
#endif
u8 rtw_set_802_11_infrastructure_modebu(_adapter *padapter, NDIS_802_11_NETWORK_INFRASTRUCTURE networktype, u8 flags);
u8 rtw_set_802_11_ssidbu(_adapter *padapter, NDIS_802_11_SSID *ssid);
u8 rtw_set_802_11_connectbu(_adapter *padapter,
			  u8 *bssid, NDIS_802_11_SSID *ssid, u16 ch);

u8 rtw_validate_bssidbu(u8 *bssid);
u8 rtw_validate_ssidbu(NDIS_802_11_SSID *ssid);

u16 rtw_get_cur_max_ratebu(_adapter *adapter);
int rtw_set_scan_modebu(_adapter *adapter, RT_SCAN_TYPE scan_mode);
int rtw_set_channel_planbu(_adapter *adapter, u8 channel_plan);
int rtw_set_countrybu(_adapter *adapter, const char *country_code);
int rtw_set_bandbu(_adapter *adapter, u8 band);

#endif
