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
#ifndef __RECV_OSDEP_H_
#define __RECV_OSDEP_H_


extern sint _rtw_init_recv_privbu(struct recv_priv *precvpriv, _adapter *padapter);
extern void _rtw_free_recv_privbu(struct recv_priv *precvpriv);


extern s32  rtw_recv_entrybu(union recv_frame *precv_frame);
void rtw_rframe_set_os_pktbu(union recv_frame *rframe);
extern int rtw_recv_indicatepktbu(_adapter *adapter, union recv_frame *precv_frame);
extern void rtw_recv_returnpacket(_nic_hdl cnxt, _pkt *preturnedpkt);

#ifdef CONFIG_WIFI_MONITOR
extern int rtw_recv_monitorbu(_adapter *padapter, union recv_frame *precv_frame);
#endif /* CONFIG_WIFI_MONITOR */

#ifdef CONFIG_HOSTAPD_MLME
extern void rtw_hostapd_mlme_rx(_adapter *padapter, union recv_frame *precv_frame);
#endif

struct sta_info;
extern void rtw_handle_tkip_mic_errbu(_adapter *padapter, struct sta_info *sta, u8 bgroup);


int rtw_os_recv_resource_initbu(struct recv_priv *precvpriv, _adapter *padapter);
int rtw_os_recv_resource_allocbu(_adapter *padapter, union recv_frame *precvframe);
void rtw_os_recv_resource_freebu(struct recv_priv *precvpriv);


int rtw_os_alloc_recvframebu(_adapter *padapter, union recv_frame *precvframe, u8 *pdata, _pkt *pskb);
int rtw_os_recvframe_duplicate_skbbu(_adapter *padapter, union recv_frame *pcloneframe, _pkt *pskb);
void rtw_os_free_recvframebu(union recv_frame *precvframe);


int rtw_os_recvbuf_resource_allocbu(_adapter *padapter, struct recv_buf *precvbuf, u32 size);
int rtw_os_recvbuf_resource_freebu(_adapter *padapter, struct recv_buf *precvbuf);

_pkt *rtw_os_alloc_msdu_pktbu(union recv_frame *prframe, const u8 *da, const u8 *sa
	, u8 *msdu ,u16 msdu_len, enum rtw_rx_llc_hdl llc_hdl);
void rtw_os_recv_indicate_pktbu(_adapter *padapter, _pkt *pkt, union recv_frame *rframe);

void rtw_os_read_portbu(_adapter *padapter, struct recv_buf *precvbuf);

#ifdef PLATFORM_LINUX
#ifdef CONFIG_RTW_NAPI
#include <linux/netdevice.h>	/* struct napi_struct */

int rtw_recv_napi_pollbu(struct napi_struct *, int budget);
#ifdef CONFIG_RTW_NAPI_DYNAMIC
void dynamic_napi_th_chk (_adapter *adapter);
#endif /* CONFIG_RTW_NAPI_DYNAMIC */
#endif /* CONFIG_RTW_NAPI */
#endif /* PLATFORM_LINUX */

#endif /*  */
