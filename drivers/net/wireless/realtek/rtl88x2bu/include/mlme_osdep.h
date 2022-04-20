/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
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
#ifndef	__MLME_OSDEP_H_
#define __MLME_OSDEP_H_

extern void rtw_os_indicate_disconnectbu(_adapter *adapter, u16 reason, u8 locally_generated);
extern void rtw_os_indicate_connectbu(_adapter *adapter);
void rtw_os_indicate_scan_donebu(_adapter *padapter, bool aborted);
extern void rtw_report_sec_iebu(_adapter *adapter, u8 authmode, u8 *sec_ie);

void rtw_reset_securityprivbu(_adapter *adapter);

#endif /* _MLME_OSDEP_H_ */
