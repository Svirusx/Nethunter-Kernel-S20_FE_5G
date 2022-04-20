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
#define _RTW_MI_C_

#include <drv_types.h>
#include <hal_data.h>

void rtw_mi_update_union_chan_infbu(_adapter *adapter, u8 ch, u8 offset , u8 bw)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);

	if (!ch) {
		dvobj->union_ch_bak = dvobj->union_ch;
		dvobj->union_bw_bak = dvobj->union_bw;
		dvobj->union_offset_bak = dvobj->union_offset;
	}
	dvobj->union_ch = ch;
	dvobj->union_bw = bw;
	dvobj->union_offset = offset;
}

#ifdef DBG_IFACE_STATUS
#ifdef CONFIG_P2P
static u8 _rtw_mi_p2p_listen_scan_chk(_adapter *adapter)
{
	int i;
	_adapter *iface;
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	u8 p2p_listen_scan_state = _FALSE;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (rtw_p2p_chk_state(&iface->wdinfo, P2P_STATE_LISTEN) ||
			rtw_p2p_chk_state(&iface->wdinfo, P2P_STATE_SCAN)) {
			p2p_listen_scan_state = _TRUE;
			break;
		}
	}
	return p2p_listen_scan_state;
}
#endif
#endif

u8 rtw_mi_stayin_union_ch_chkbu(_adapter *adapter)
{
	u8 rst = _TRUE;
	u8 u_ch, u_bw, u_offset;
	u8 o_ch, o_bw, o_offset;

	u_ch = rtw_mi_get_union_chan(adapter);
	u_bw = rtw_mi_get_union_bw(adapter);
	u_offset = rtw_mi_get_union_offset(adapter);

	o_ch = rtw_get_oper_chbu(adapter);
	o_bw = rtw_get_oper_bwbu(adapter);
	o_offset = rtw_get_oper_chbuoffset(adapter);

	if ((u_ch != o_ch) || (u_bw != o_bw) || (u_offset != o_offset))
		rst = _FALSE;

	#ifdef DBG_IFACE_STATUS
	if (rst == _FALSE) {
		RTW_ERR("%s Not stay in union channel\n", __func__);
		if (GET_HAL_DATA(adapter)->bScanInProcess == _TRUE)
			RTW_ERR("ScanInProcess\n");
		#ifdef CONFIG_P2P
		if (_rtw_mi_p2p_listen_scan_chk(adapter))
			RTW_ERR("P2P in listen or scan state\n");
		#endif
		RTW_ERR("union ch, bw, offset: %u,%u,%u\n", u_ch, u_bw, u_offset);
		RTW_ERR("oper ch, bw, offset: %u,%u,%u\n", o_ch, o_bw, o_offset);
		RTW_ERR("=========================\n");
	}
	#endif
	return rst;
}

u8 rtw_mi_stayin_union_band_chkbu(_adapter *adapter)
{
	u8 rst = _TRUE;
	u8 u_ch, o_ch;
	u8 u_band, o_band;

	u_ch = rtw_mi_get_union_chan(adapter);
	o_ch = rtw_get_oper_chbu(adapter);
	u_band = (u_ch > 14) ? BAND_ON_5G : BAND_ON_2_4G;
	o_band = (o_ch > 14) ? BAND_ON_5G : BAND_ON_2_4G;

	if (u_ch != o_ch)
		if(u_band != o_band)
			rst = _FALSE;

	#ifdef DBG_IFACE_STATUS
	if (rst == _FALSE)
		RTW_ERR("%s Not stay in union band\n", __func__);
	#endif

	return rst;
}

/* Find union about ch, bw, ch_offset of all linked/linking interfaces */
int rtw_mi_get_ch_setting_unionbu_by_ifbmpbu(struct dvobj_priv *dvobj, u8 ifbmp, u8 *ch, u8 *bw, u8 *offset)
{
	_adapter *iface;
	struct mlme_ext_priv *mlmeext;
	int i;
	u8 ch_ret = 0;
	u8 bw_ret = CHANNEL_WIDTH_20;
	u8 offset_ret = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
	int num = 0;

	if (ch)
		*ch = 0;
	if (bw)
		*bw = CHANNEL_WIDTH_20;
	if (offset)
		*offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (!iface || !(ifbmp & BIT(iface->iface_id)))
			continue;

		mlmeext = &iface->mlmeextpriv;

		if (!check_fwstate(&iface->mlmepriv, WIFI_ASOC_STATE | WIFI_UNDER_LINKING))
			continue;

		if (check_fwstate(&iface->mlmepriv, WIFI_OP_CH_SWITCHING))
			continue;

		if (num == 0) {
			ch_ret = mlmeext->cur_channel;
			bw_ret = mlmeext->cur_bwmode;
			offset_ret = mlmeext->cur_ch_offset;
			num++;
			continue;
		}

		if (ch_ret != mlmeext->cur_channel) {
			num = 0;
			break;
		}

		if (bw_ret < mlmeext->cur_bwmode) {
			bw_ret = mlmeext->cur_bwmode;
			offset_ret = mlmeext->cur_ch_offset;
		} else if (bw_ret == mlmeext->cur_bwmode && offset_ret != mlmeext->cur_ch_offset) {
			num = 0;
			break;
		}

		num++;
	}

	if (num) {
		if (ch)
			*ch = ch_ret;
		if (bw)
			*bw = bw_ret;
		if (offset)
			*offset = offset_ret;
	}

	return num;
}

inline int rtw_mi_get_ch_setting_unionbu(_adapter *adapter, u8 *ch, u8 *bw, u8 *offset)
{
	return rtw_mi_get_ch_setting_unionbu_by_ifbmpbu(adapter_to_dvobj(adapter), 0xFF, ch, bw, offset);
}

inline int rtw_mi_get_ch_setting_unionbu_no_self(_adapter *adapter, u8 *ch, u8 *bw, u8 *offset)
{
	return rtw_mi_get_ch_setting_unionbu_by_ifbmpbu(adapter_to_dvobj(adapter), 0xFF & ~BIT(adapter->iface_id), ch, bw, offset);
}

void rtw_mi_statusbu_by_ifbmpbu(struct dvobj_priv *dvobj, u8 ifbmp, struct mi_state *mstate)
{
	_adapter *iface;
	int i;

	_rtw_memsetbu(mstate, 0, sizeof(struct mi_state));

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (!iface || !(ifbmp & BIT(iface->iface_id)))
			continue;

		if (check_fwstate(&iface->mlmepriv, WIFI_STATION_STATE) == _TRUE) {
			MSTATE_STA_NUM(mstate)++;
			if (check_fwstate(&iface->mlmepriv, WIFI_ASOC_STATE) == _TRUE) {
				MSTATE_STA_LD_NUM(mstate)++;

				#ifdef CONFIG_TDLS
				if (iface->tdlsinfo.link_established == _TRUE)
					MSTATE_TDLS_LD_NUM(mstate)++;
				#endif
				#ifdef CONFIG_P2P
				if (MLME_IS_GC(iface))
					MSTATE_P2P_GC_NUM(mstate)++;
				#endif
			}
			if (check_fwstate(&iface->mlmepriv, WIFI_UNDER_LINKING) == _TRUE)
				MSTATE_STA_LG_NUM(mstate)++;

#ifdef CONFIG_AP_MODE
		} else if (check_fwstate(&iface->mlmepriv, WIFI_AP_STATE) == _TRUE ) {
			if (check_fwstate(&iface->mlmepriv, WIFI_ASOC_STATE) == _TRUE) {
				MSTATE_AP_NUM(mstate)++;
				if (iface->stapriv.asoc_sta_count > 2)
					MSTATE_AP_LD_NUM(mstate)++;
				#ifdef CONFIG_P2P
				if (MLME_IS_GO(iface))
					MSTATE_P2P_GO_NUM(mstate)++;
				#endif
			} else
				MSTATE_AP_STARTING_NUM(mstate)++;
#endif

		} else if (check_fwstate(&iface->mlmepriv, WIFI_ADHOC_STATE | WIFI_ADHOC_MASTER_STATE) == _TRUE
			&& check_fwstate(&iface->mlmepriv, WIFI_ASOC_STATE) == _TRUE
		) {
			MSTATE_ADHOC_NUM(mstate)++;
			if (iface->stapriv.asoc_sta_count > 2)
				MSTATE_ADHOC_LD_NUM(mstate)++;

#ifdef CONFIG_RTW_MESH
		} else if (check_fwstate(&iface->mlmepriv, WIFI_MESH_STATE) == _TRUE
			&& check_fwstate(&iface->mlmepriv, WIFI_ASOC_STATE) == _TRUE
		) {
			MSTATE_MESH_NUM(mstate)++;
			if (iface->stapriv.asoc_sta_count > 2)
				MSTATE_MESH_LD_NUM(mstate)++;
#endif

		}

		if (check_fwstate(&iface->mlmepriv, WIFI_UNDER_WPS) == _TRUE)
			MSTATE_WPS_NUM(mstate)++;

		if (check_fwstate(&iface->mlmepriv, WIFI_UNDER_SURVEY) == _TRUE) {
			MSTATE_SCAN_NUM(mstate)++;

			if (mlmeext_scan_state(&iface->mlmeextpriv) != SCAN_DISABLE
				&& mlmeext_scan_state(&iface->mlmeextpriv) != SCAN_BACK_OP)
				MSTATE_SCAN_ENTER_NUM(mstate)++;
		}

#ifdef CONFIG_IOCTL_CFG80211
		if (rtw_cfg80211_get_is_mgmt_txbu(iface))
			MSTATE_MGMT_TX_NUM(mstate)++;

		if (rtw_cfg80211_get_is_rochbu(iface) == _TRUE)
			MSTATE_ROCH_NUM(mstate)++;

#endif /* CONFIG_IOCTL_CFG80211 */
#ifdef CONFIG_P2P
		if (MLME_IS_PD(iface))
			MSTATE_P2P_DV_NUM(mstate)++;
#endif
	}
}

inline void rtw_mi_statusbu(_adapter *adapter, struct mi_state *mstate)
{
	return rtw_mi_statusbu_by_ifbmpbu(adapter_to_dvobj(adapter), 0xFF, mstate);
}

inline void rtw_mi_statusbu_no_self(_adapter *adapter, struct mi_state *mstate)
{
	return rtw_mi_statusbu_by_ifbmpbu(adapter_to_dvobj(adapter), 0xFF & ~BIT(adapter->iface_id), mstate);
}

inline void rtw_mi_statusbu_no_others(_adapter *adapter, struct mi_state *mstate)
{
	return rtw_mi_statusbu_by_ifbmpbu(adapter_to_dvobj(adapter), BIT(adapter->iface_id), mstate);
}

inline void rtw_mi_statusbu_merge(struct mi_state *d, struct mi_state *a)
{
	d->sta_num += a->sta_num;
	d->ld_sta_num += a->ld_sta_num;
	d->lg_sta_num += a->lg_sta_num;
#ifdef CONFIG_TDLS
	d->ld_tdls_num += a->ld_tdls_num;
#endif
#ifdef CONFIG_AP_MODE
	d->ap_num += a->ap_num;
	d->ld_ap_num += a->ld_ap_num;
#endif
	d->adhoc_num += a->adhoc_num;
	d->ld_adhoc_num += a->ld_adhoc_num;
#ifdef CONFIG_RTW_MESH
	d->mesh_num += a->mesh_num;
	d->ld_mesh_num += a->ld_mesh_num;
#endif
	d->scan_num += a->scan_num;
	d->scan_enter_num += a->scan_enter_num;
	d->uwps_num += a->uwps_num;
#ifdef CONFIG_IOCTL_CFG80211
	#ifdef CONFIG_P2P
	d->roch_num += a->roch_num;
	#endif
	d->mgmt_tx_num += a->mgmt_tx_num;
#endif
}

void dump_mi_statusbu(void *sel, struct dvobj_priv *dvobj)
{
	RTW_PRINT_SEL(sel, "== dvobj-iface_state ==\n");
	RTW_PRINT_SEL(sel, "sta_num:%d\n", DEV_STA_NUM(dvobj));
	RTW_PRINT_SEL(sel, "linking_sta_num:%d\n", DEV_STA_LG_NUM(dvobj));
	RTW_PRINT_SEL(sel, "linked_sta_num:%d\n", DEV_STA_LD_NUM(dvobj));
#ifdef CONFIG_TDLS
	RTW_PRINT_SEL(sel, "linked_tdls_num:%d\n", DEV_TDLS_LD_NUM(dvobj));
#endif
#ifdef CONFIG_AP_MODE
	RTW_PRINT_SEL(sel, "ap_num:%d\n", DEV_AP_NUM(dvobj));
	RTW_PRINT_SEL(sel, "starting_ap_num:%d\n", DEV_AP_STARTING_NUM(dvobj));
	RTW_PRINT_SEL(sel, "linked_ap_num:%d\n", DEV_AP_LD_NUM(dvobj));
#endif
	RTW_PRINT_SEL(sel, "adhoc_num:%d\n", DEV_ADHOC_NUM(dvobj));
	RTW_PRINT_SEL(sel, "linked_adhoc_num:%d\n", DEV_ADHOC_LD_NUM(dvobj));
#ifdef CONFIG_RTW_MESH
	RTW_PRINT_SEL(sel, "mesh_num:%d\n", DEV_MESH_NUM(dvobj));
	RTW_PRINT_SEL(sel, "linked_mesh_num:%d\n", DEV_MESH_LD_NUM(dvobj));
#endif
#ifdef CONFIG_P2P
	RTW_PRINT_SEL(sel, "p2p_device_num:%d\n", DEV_P2P_DV_NUM(dvobj));
	RTW_PRINT_SEL(sel, "p2p_gc_num:%d\n", DEV_P2P_GC_NUM(dvobj));
	RTW_PRINT_SEL(sel, "p2p_go_num:%d\n", DEV_P2P_GO_NUM(dvobj));
#endif
	RTW_PRINT_SEL(sel, "scan_num:%d\n", DEV_SCAN_NUM(dvobj));
	RTW_PRINT_SEL(sel, "under_wps_num:%d\n", DEV_WPS_NUM(dvobj));
#if defined(CONFIG_IOCTL_CFG80211)
	#if defined(CONFIG_P2P)
	RTW_PRINT_SEL(sel, "roch_num:%d\n", DEV_ROCH_NUM(dvobj));
	#endif
	RTW_PRINT_SEL(sel, "mgmt_tx_num:%d\n", DEV_MGMT_TX_NUM(dvobj));
#endif
	RTW_PRINT_SEL(sel, "union_ch:%d\n", DEV_U_CH(dvobj));
	RTW_PRINT_SEL(sel, "union_bw:%d\n", DEV_U_BW(dvobj));
	RTW_PRINT_SEL(sel, "union_offset:%d\n", DEV_U_OFFSET(dvobj));
	RTW_PRINT_SEL(sel, "================\n\n");
}

void dump_dvobj_mi_statusbu(void *sel, const char *fun_name, _adapter *adapter)
{
	RTW_INFO("\n[ %s ] call %s\n", fun_name, __func__);
	dump_mi_statusbu(sel, adapter_to_dvobj(adapter));
}

inline void rtw_mi_update_iface_statusbu(struct mlme_priv *pmlmepriv, sint state)
{
	_adapter *adapter = container_of(pmlmepriv, _adapter, mlmepriv);
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	struct mi_state *iface_state = &dvobj->iface_state;
	struct mi_state tmp_mstate;

	if (state == WIFI_MONITOR_STATE
		|| state == 0xFFFFFFFF
	)
		return;

	if (0)
		RTW_INFO("%s => will change or clean state to 0x%08x\n", __func__, state);

	rtw_mi_statusbu(adapter, &tmp_mstate);
	_rtw_memcpybu(iface_state, &tmp_mstate, sizeof(struct mi_state));

#ifdef DBG_IFACE_STATUS
	DBG_IFACE_STATUS_DUMP(adapter);
#endif
}
u8 rtw_mi_check_statusbu(_adapter *adapter, u8 type)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	struct mi_state *iface_state = &dvobj->iface_state;
	u8 ret = _FALSE;

#ifdef DBG_IFACE_STATUS
	DBG_IFACE_STATUS_DUMP(adapter);
	RTW_INFO("%s-"ADPT_FMT" check type:%d\n", __func__, ADPT_ARG(adapter), type);
#endif

	switch (type) {
	case MI_LINKED:
		if (MSTATE_STA_LD_NUM(iface_state) || MSTATE_AP_NUM(iface_state) || MSTATE_ADHOC_NUM(iface_state) || MSTATE_MESH_NUM(iface_state)) /*check_fwstate(&iface->mlmepriv, WIFI_ASOC_STATE)*/
			ret = _TRUE;
		break;
	case MI_ASSOC:
		if (MSTATE_STA_LD_NUM(iface_state) || MSTATE_AP_LD_NUM(iface_state) || MSTATE_ADHOC_LD_NUM(iface_state) || MSTATE_MESH_LD_NUM(iface_state))
			ret = _TRUE;
		break;
	case MI_UNDER_WPS:
		if (MSTATE_WPS_NUM(iface_state))
			ret = _TRUE;
		break;

	case MI_AP_MODE:
		if (MSTATE_AP_NUM(iface_state))
			ret = _TRUE;
		break;
	case MI_AP_ASSOC:
		if (MSTATE_AP_LD_NUM(iface_state))
			ret = _TRUE;
		break;

	case MI_ADHOC:
		if (MSTATE_ADHOC_NUM(iface_state))
			ret = _TRUE;
		break;
	case MI_ADHOC_ASSOC:
		if (MSTATE_ADHOC_LD_NUM(iface_state))
			ret = _TRUE;
		break;

#ifdef CONFIG_RTW_MESH
	case MI_MESH:
		if (MSTATE_MESH_NUM(iface_state))
			ret = _TRUE;
		break;
	case MI_MESH_ASSOC:
		if (MSTATE_MESH_LD_NUM(iface_state))
			ret = _TRUE;
		break;
#endif

	case MI_STA_NOLINK: /* this is misleading, but not used now */
		if (MSTATE_STA_NUM(iface_state) && (!(MSTATE_STA_LD_NUM(iface_state) || MSTATE_STA_LG_NUM(iface_state))))
			ret = _TRUE;
		break;
	case MI_STA_LINKED:
		if (MSTATE_STA_LD_NUM(iface_state))
			ret = _TRUE;
		break;
	case MI_STA_LINKING:
		if (MSTATE_STA_LG_NUM(iface_state))
			ret = _TRUE;
		break;

	default:
		break;
	}
	return ret;
}

/*
* return value : 0 is failed or have not interface meet condition
* return value : !0 is success or interface numbers which meet condition
* return value of ops_func must be _TRUE or _FALSE
*/
static u8 _rtw_mi_process(_adapter *padapter, bool exclude_self,
		  void *data, u8(*ops_func)(_adapter *padapter, void *data))
{
	int i;
	_adapter *iface;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);

	u8 ret = 0;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if ((iface) && rtw_is_adapter_upbu(iface)) {

			if ((exclude_self) && (iface == padapter))
				continue;

			if (ops_func)
				if (_TRUE == ops_func(iface, data))
					ret++;
		}
	}
	return ret;
}
static u8 _rtw_mi_process_without_schk(_adapter *padapter, bool exclude_self,
		  void *data, u8(*ops_func)(_adapter *padapter, void *data))
{
	int i;
	_adapter *iface;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);

	u8 ret = 0;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (iface) {
			if ((exclude_self) && (iface == padapter))
				continue;

			if (ops_func)
				if (ops_func(iface, data) == _TRUE)
					ret++;
		}
	}
	return ret;
}

static u8 _rtw_mi_netif_caroff_qstopbu(_adapter *padapter, void *data)
{
	struct net_device *pnetdev = padapter->pnetdev;

	rtw_netif_carrier_off(pnetdev);
	rtw_netif_stop_queue(pnetdev);
	return _TRUE;
}
u8 rtw_mi_netif_caroff_qstopbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_netif_caroff_qstopbu);
}
u8 rtw_mi_buddy_netif_caroff_qstopbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_netif_caroff_qstopbu);
}

static u8 _rtw_mi_netif_caron_qstartbu(_adapter *padapter, void *data)
{
	struct net_device *pnetdev = padapter->pnetdev;

	rtw_netif_carrier_on(pnetdev);
	rtw_netif_start_queue(pnetdev);
	return _TRUE;
}
u8 rtw_mi_netif_caron_qstartbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_netif_caron_qstartbu);
}
u8 rtw_mi_buddy_netif_caron_qstartbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_netif_caron_qstartbu);
}

static u8 _rtw_mi_netif_stop_queuebu(_adapter *padapter, void *data)
{
	struct net_device *pnetdev = padapter->pnetdev;

	rtw_netif_stop_queue(pnetdev);
	return _TRUE;
}
u8 rtw_mi_netif_stop_queuebu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_netif_stop_queuebu);
}
u8 rtw_mi_buddy_netif_stop_queuebu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_netif_stop_queuebu);
}

static u8 _rtw_mi_netif_wake_queuebu(_adapter *padapter, void *data)
{
	struct net_device *pnetdev = padapter->pnetdev;

	if (pnetdev)
		rtw_netif_wake_queue(pnetdev);
	return _TRUE;
}
u8 rtw_mi_netif_wake_queuebu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_netif_wake_queuebu);
}
u8 rtw_mi_buddy_netif_wake_queuebu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_netif_wake_queuebu);
}

static u8 _rtw_mi_netif_carrier_onbu(_adapter *padapter, void *data)
{
	struct net_device *pnetdev = padapter->pnetdev;

	if (pnetdev)
		rtw_netif_carrier_on(pnetdev);
	return _TRUE;
}
u8 rtw_mi_netif_carrier_onbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_netif_carrier_onbu);
}
u8 rtw_mi_buddy_netif_carrier_onbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_netif_carrier_onbu);
}

static u8 _rtw_mi_netif_carrier_offbu(_adapter *padapter, void *data)
{
	struct net_device *pnetdev = padapter->pnetdev;

	if (pnetdev)
		rtw_netif_carrier_off(pnetdev);
	return _TRUE;
}
u8 rtw_mi_netif_carrier_offbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_netif_carrier_offbu);
}
u8 rtw_mi_buddy_netif_carrier_offbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_netif_carrier_offbu);
}

static u8 _rtw_mi_scan_abortbu(_adapter *adapter, void *data)
{
	bool bwait = *(bool *)data;

	if (bwait)
		rtw_scan_abortbu(adapter);
	else
		rtw_scan_abortbu_no_waitbu(adapter);

	return _TRUE;
}
void rtw_mi_scan_abortbu(_adapter *adapter, bool bwait)
{
	bool in_data = bwait;

	_rtw_mi_process(adapter, _FALSE, &in_data, _rtw_mi_scan_abortbu);

}
void rtw_mi_buddy_scan_abortbu(_adapter *adapter, bool bwait)
{
	bool in_data = bwait;

	_rtw_mi_process(adapter, _TRUE, &in_data, _rtw_mi_scan_abortbu);
}

static u32 _rtw_mi_start_drv_threadsbu(_adapter *adapter, bool exclude_self)
{
	int i;
	_adapter *iface = NULL;
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	u32 _status = _SUCCESS;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (iface) {
			if ((exclude_self) && (iface == adapter))
				continue;
			if (rtw_start_drv_threadsbu(iface) == _FAIL) {
				_status = _FAIL;
				break;
			}
		}
	}
	return _status;
}
u32 rtw_mi_start_drv_threadsbu(_adapter *adapter)
{
	return _rtw_mi_start_drv_threadsbu(adapter, _FALSE);
}
u32 rtw_mi_buddy_start_drv_threadsbu(_adapter *adapter)
{
	return _rtw_mi_start_drv_threadsbu(adapter, _TRUE);
}

static void _rtw_mi_stop_drv_threadsbu(_adapter *adapter, bool exclude_self)
{
	int i;
	_adapter *iface = NULL;
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (iface) {
			if ((exclude_self) && (iface == adapter))
				continue;
			rtw_stop_drv_threadsbu(iface);
		}
	}
}
void rtw_mi_stop_drv_threadsbu(_adapter *adapter)
{
	_rtw_mi_stop_drv_threadsbu(adapter, _FALSE);
}
void rtw_mi_buddy_stop_drv_threadsbu(_adapter *adapter)
{
	_rtw_mi_stop_drv_threadsbu(adapter, _TRUE);
}

static u8 _rtw_mi_cancel_all_timerbu(_adapter *adapter, void *data)
{
	rtw_cancel_all_timerbu(adapter);
	return _TRUE;
}
void rtw_mi_cancel_all_timerbu(_adapter *adapter)
{
	_rtw_mi_process(adapter, _FALSE, NULL, _rtw_mi_cancel_all_timerbu);
}
void rtw_mi_buddy_cancel_all_timerbu(_adapter *adapter)
{
	_rtw_mi_process(adapter, _TRUE, NULL, _rtw_mi_cancel_all_timerbu);
}

static u8 _rtw_mi_reset_drv_swbu(_adapter *adapter, void *data)
{
	rtw_reset_drv_swbu(adapter);
	return _TRUE;
}
void rtw_mi_reset_drv_swbu(_adapter *adapter)
{
	_rtw_mi_process_without_schk(adapter, _FALSE, NULL, _rtw_mi_reset_drv_swbu);
}
void rtw_mi_buddy_reset_drv_swbu(_adapter *adapter)
{
	_rtw_mi_process_without_schk(adapter, _TRUE, NULL, _rtw_mi_reset_drv_swbu);
}

static u8 _rtw_mi_intf_startbu(_adapter *adapter, void *data)
{
	rtw_intf_startbu(adapter);
	return _TRUE;
}
void rtw_mi_intf_startbu(_adapter *adapter)
{
	_rtw_mi_process(adapter, _FALSE, NULL, _rtw_mi_intf_startbu);
}
void rtw_mi_buddy_intf_startbu(_adapter *adapter)
{
	_rtw_mi_process(adapter, _TRUE, NULL, _rtw_mi_intf_startbu);
}

static u8 _rtw_mi_intf_stopbu(_adapter *adapter, void *data)
{
	rtw_intf_stopbu(adapter);
	return _TRUE;
}
void rtw_mi_intf_stopbu(_adapter *adapter)
{
	_rtw_mi_process(adapter, _FALSE, NULL, _rtw_mi_intf_stopbu);
}
void rtw_mi_buddy_intf_stopbu(_adapter *adapter)
{
	_rtw_mi_process(adapter, _TRUE, NULL, _rtw_mi_intf_stopbu);
}

#ifdef CONFIG_NEW_NETDEV_HDL
u8 rtw_mi_hal_iface_init(_adapter *padapter)
{
	int i;
	_adapter *iface;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);

	u8 ret = _TRUE;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (iface && iface->netif_up)
			rtw_hal_iface_init(padapter);
	}
	return ret;
}
#endif

static u8 _rtw_mi_suspend_free_assoc_resourcebu(_adapter *padapter, void *data)
{
	return rtw_suspend_free_assoc_resourcebu(padapter);
}
void rtw_mi_suspend_free_assoc_resourcebu(_adapter *adapter)
{
	_rtw_mi_process(adapter, _FALSE, NULL, _rtw_mi_suspend_free_assoc_resourcebu);
}
void rtw_mi_buddy_suspend_free_assoc_resourcebu(_adapter *adapter)
{
	_rtw_mi_process(adapter, _TRUE, NULL, _rtw_mi_suspend_free_assoc_resourcebu);
}

static u8 _rtw_mi_is_scan_denybu(_adapter *adapter, void *data)
{
	return rtw_is_scan_denybu(adapter);
}

u8 rtw_mi_is_scan_denybu(_adapter *adapter)
{
	return _rtw_mi_process(adapter, _FALSE, NULL, _rtw_mi_is_scan_denybu);

}
u8 rtw_mi_buddy_is_scan_denybu(_adapter *adapter)
{
	return _rtw_mi_process(adapter, _TRUE, NULL, _rtw_mi_is_scan_denybu);
}

#ifdef CONFIG_SET_SCAN_DENY_TIMER
static u8 _rtw_mi_set_scan_denybu(_adapter *adapter, void *data)
{
	u32 ms = *(u32 *)data;

	rtw_set_scan_denybu(adapter, ms);
	return _TRUE;
}
void rtw_mi_set_scan_denybu(_adapter *adapter, u32 ms)
{
	u32 in_data = ms;

	_rtw_mi_process(adapter, _FALSE, &in_data, _rtw_mi_set_scan_denybu);
}
void rtw_mi_buddy_set_scan_denybu(_adapter *adapter, u32 ms)
{
	u32 in_data = ms;

	_rtw_mi_process(adapter, _TRUE, &in_data, _rtw_mi_set_scan_denybu);
}
#endif /*CONFIG_SET_SCAN_DENY_TIMER*/

#ifdef CONFIG_AP_MODE
static u8 _rtw_mi_beacon_updatebu(_adapter *padapter, void *data)
{
	if (!MLME_IS_STA(padapter)
	    && check_fwstate(&padapter->mlmepriv, WIFI_ASOC_STATE) == _TRUE) {
		RTW_INFO(ADPT_FMT" - update_beacon\n", ADPT_ARG(padapter));
		update_beacon(padapter, 0xFF, NULL, _TRUE, 0);
	}
	return _TRUE;
}

void rtw_mi_beacon_updatebu(_adapter *padapter)
{
	_rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_beacon_updatebu);
}

void rtw_mi_buddy_beacon_updatebu(_adapter *padapter)
{
	_rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_beacon_updatebu);
}
#endif /* CONFIG_AP_MODE */

#ifndef CONFIG_MI_WITH_MBSSID_CAM
static u8 _rtw_mi_hal_dump_macaddrbu(_adapter *padapter, void *sel)
{
	u8 mac_addr[ETH_ALEN] = {0};

	rtw_hal_get_hwregbu(padapter, HW_VAR_MAC_ADDR, mac_addr);
	RTW_PRINT_SEL(sel, ADPT_FMT"- hw port(%d) mac_addr ="MAC_FMT"\n",
					ADPT_ARG(padapter), padapter->hw_port, MAC_ARG(mac_addr));

	return _TRUE;
}
void rtw_mi_hal_dump_macaddrbu(void *sel, _adapter *padapter)
{
	_rtw_mi_process(padapter, _FALSE, sel, _rtw_mi_hal_dump_macaddrbu);
}
void rtw_mi_buddy_hal_dump_macaddrbu(void *sel, _adapter *padapter)
{
	_rtw_mi_process(padapter, _TRUE, sel, _rtw_mi_hal_dump_macaddrbu);
}
#endif

#ifdef CONFIG_PCI_HCI
static u8 _rtw_mi_xmit_tasklet_schedule(_adapter *padapter, void *data)
{
	if (rtw_txframes_pendingbu(padapter)) {
		/* try to deal with the pending packets */
		tasklet_hi_schedule(&(padapter->xmitpriv.xmit_tasklet));
	}
	return _TRUE;
}
void rtw_mi_xmit_tasklet_schedule(_adapter *padapter)
{
	_rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_xmit_tasklet_schedule);
}
void rtw_mi_buddy_xmit_tasklet_schedule(_adapter *padapter)
{
	_rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_xmit_tasklet_schedule);
}
#endif

u8 _rtw_mi_busy_traffic_checkbubu(_adapter *padapter, void *data)
{
	return padapter->mlmepriv.LinkDetectInfo.bBusyTraffic;
}

u8 rtw_mi_busy_traffic_checkbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_busy_traffic_checkbubu);
}
u8 rtw_mi_buddy_busy_traffic_checkbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_busy_traffic_checkbubu);
}
static u8 _rtw_mi_check_mlmeinfo_statebu(_adapter *padapter, void *data)
{
	u32 state = *(u32 *)data;
	struct mlme_ext_priv *mlmeext = &padapter->mlmeextpriv;

	/*if (mlmeext_msr(mlmeext) == state)*/
	if (check_mlmeinfo_state(mlmeext, state))
		return _TRUE;
	else
		return _FALSE;
}

u8 rtw_mi_check_mlmeinfo_statebu(_adapter *padapter, u32 state)
{
	u32 in_data = state;

	return _rtw_mi_process(padapter, _FALSE, &in_data, _rtw_mi_check_mlmeinfo_statebu);
}

u8 rtw_mi_buddy_check_mlmeinfo_statebu(_adapter *padapter, u32 state)
{
	u32 in_data = state;

	return _rtw_mi_process(padapter, _TRUE, &in_data, _rtw_mi_check_mlmeinfo_statebu);
}

/*#define DBG_DUMP_FW_STATE*/
#ifdef DBG_DUMP_FW_STATE
static void rtw_dbg_dump_fwstate(_adapter *padapter, sint state)
{
	u8 buf[32] = {0};

	if (state & WIFI_FW_NULL_STATE) {
		_rtw_memsetbu(buf, 0, 32);
		sprintf(buf, "WIFI_FW_NULL_STATE");
		RTW_INFO(FUNC_ADPT_FMT"fwstate-%s\n", FUNC_ADPT_ARG(padapter), buf);
	}

	if (state & WIFI_ASOC_STATE) {
		_rtw_memsetbu(buf, 0, 32);
		sprintf(buf, "WIFI_ASOC_STATE");
		RTW_INFO(FUNC_ADPT_FMT"fwstate-%s\n", FUNC_ADPT_ARG(padapter), buf);
	}

	if (state & WIFI_UNDER_LINKING) {
		_rtw_memsetbu(buf, 0, 32);
		sprintf(buf, "WIFI_UNDER_LINKING");
		RTW_INFO(FUNC_ADPT_FMT"fwstate-%s\n", FUNC_ADPT_ARG(padapter), buf);
	}

	if (state & WIFI_UNDER_SURVEY) {
		_rtw_memsetbu(buf, 0, 32);
		sprintf(buf, "WIFI_UNDER_SURVEY");
		RTW_INFO(FUNC_ADPT_FMT"fwstate-%s\n", FUNC_ADPT_ARG(padapter), buf);
	}
}
#endif

static u8 _rtw_mi_check_fwstatebu(_adapter *padapter, void *data)
{
	u8 ret = _FALSE;

	sint state = *(sint *)data;

	if ((state == WIFI_FW_NULL_STATE) &&
	    (padapter->mlmepriv.fw_state == WIFI_FW_NULL_STATE))
		ret = _TRUE;
	else if (_TRUE == check_fwstate(&padapter->mlmepriv, state))
		ret = _TRUE;
#ifdef DBG_DUMP_FW_STATE
	if (ret)
		rtw_dbg_dump_fwstate(padapter, state);
#endif
	return ret;
}
u8 rtw_mi_check_fwstatebu(_adapter *padapter, sint state)
{
	sint in_data = state;

	return _rtw_mi_process(padapter, _FALSE, &in_data, _rtw_mi_check_fwstatebu);
}
u8 rtw_mi_buddy_check_fwstatebu(_adapter *padapter, sint state)
{
	sint in_data = state;

	return _rtw_mi_process(padapter, _TRUE, &in_data, _rtw_mi_check_fwstatebu);
}

static u8 _rtw_mi_traffic_statisticsbu(_adapter *padapter , void *data)
{
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(padapter);

	/* Tx */
	pdvobjpriv->traffic_stat.tx_bytes += padapter->xmitpriv.tx_bytes;
	pdvobjpriv->traffic_stat.tx_pkts += padapter->xmitpriv.tx_pkts;
	pdvobjpriv->traffic_stat.tx_drop += padapter->xmitpriv.tx_drop;

	/* Rx */
	pdvobjpriv->traffic_stat.rx_bytes += padapter->recvpriv.rx_bytes;
	pdvobjpriv->traffic_stat.rx_pkts += padapter->recvpriv.rx_pkts;
	pdvobjpriv->traffic_stat.rx_drop += padapter->recvpriv.rx_drop;
	return _TRUE;
}
u8 rtw_mi_traffic_statisticsbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_traffic_statisticsbu);
}

static u8 _rtw_mi_check_miracast_enabledbu(_adapter *padapter , void *data)
{
	return is_miracast_enabledbu(padapter);
}
u8 rtw_mi_check_miracast_enabledbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_check_miracast_enabledbu);
}

#ifdef CONFIG_XMIT_THREAD_MODE
static u8 _rtw_mi_check_pending_xmitbuf(_adapter *padapter , void *data)
{
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;

	return check_pending_xmitbuf(pxmitpriv);
}
u8 rtw_mi_check_pending_xmitbuf(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_check_pending_xmitbuf);
}
u8 rtw_mi_buddy_check_pending_xmitbuf(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_check_pending_xmitbuf);
}
#endif

#if defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
static u8 _rtw_mi_dequeue_writeport(_adapter *padapter , bool exclude_self)
{
	int i;
	u8	queue_empty = _TRUE;
	_adapter *iface;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if ((iface) && rtw_is_adapter_upbu(iface)) {

			if ((exclude_self) && (iface == padapter))
				continue;

			queue_empty &= _dequeue_writeport(iface);
		}
	}
	return queue_empty;
}
u8 rtw_mi_dequeue_writeport(_adapter *padapter)
{
	return _rtw_mi_dequeue_writeport(padapter, _FALSE);
}
u8 rtw_mi_buddy_dequeue_writeport(_adapter *padapter)
{
	return _rtw_mi_dequeue_writeport(padapter, _TRUE);
}
#endif
static void _rtw_mi_adapter_resetbu(_adapter *padapter , u8 exclude_self)
{
	int i;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);

	for (i = 0; i < dvobj->iface_nums; i++) {
		if (dvobj->padapters[i]) {
			if ((exclude_self) && (dvobj->padapters[i] == padapter))
				continue;
			dvobj->padapters[i] = NULL;
		}
	}
}

void rtw_mi_adapter_resetbu(_adapter *padapter)
{
	_rtw_mi_adapter_resetbu(padapter, _FALSE);
}

void rtw_mi_buddy_adapter_resetbu(_adapter *padapter)
{
	_rtw_mi_adapter_resetbu(padapter, _TRUE);
}

static u8 _rtw_mi_dynamic_check_timer_handlderbu(_adapter *adapter, void *data)
{
	rtw_iface_dynamic_check_timer_handlderbu(adapter);
	return _TRUE;
}
u8 rtw_mi_dynamic_check_timer_handlderbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_dynamic_check_timer_handlderbu);
}
u8 rtw_mi_buddy_dynamic_check_timer_handlderbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_dynamic_check_timer_handlderbu);
}

static u8 _rtw_mi_dynamic_chk_wk_hdlbu(_adapter *adapter, void *data)
{
	rtw_iface_dynamic_chk_wk_hdlbu(adapter);
	return _TRUE;
}
u8 rtw_mi_dynamic_chk_wk_hdlbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_dynamic_chk_wk_hdlbu);
}
u8 rtw_mi_buddy_dynamic_chk_wk_hdlbu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_dynamic_chk_wk_hdlbu);
}

static u8 _rtw_mi_os_xmit_schedulebu(_adapter *adapter, void *data)
{
	rtw_os_xmit_schedulebu(adapter);
	return _TRUE;
}
u8 rtw_mi_os_xmit_schedulebu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_os_xmit_schedulebu);
}
u8 rtw_mi_buddy_os_xmit_schedulebu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_os_xmit_schedulebu);
}

static u8 _rtw_mi_report_survey_eventbubu(_adapter *adapter, void *data)
{
	union recv_frame *precv_frame = (union recv_frame *)data;

	report_survey_eventbu(adapter, precv_frame);
	return _TRUE;
}
u8 rtw_mi_report_survey_eventbubu(_adapter *padapter, union recv_frame *precv_frame)
{
	return _rtw_mi_process(padapter, _FALSE, precv_frame, _rtw_mi_report_survey_eventbubu);
}
u8 rtw_mi_buddy_report_survey_eventbubu(_adapter *padapter, union recv_frame *precv_frame)
{
	return _rtw_mi_process(padapter, _TRUE, precv_frame, _rtw_mi_report_survey_eventbubu);
}

static u8 _rtw_mi_sreset_adapter_hdlbu(_adapter *adapter, void *data)
{
	u8 bstart = *(u8 *)data;

	if (bstart)
		sreset_start_adapterbu(adapter);
	else
		sreset_stop_adapterbu(adapter);
	return _TRUE;
}
u8 rtw_mi_sreset_adapter_hdlbu(_adapter *padapter, u8 bstart)
{
	u8 in_data = bstart;

	return _rtw_mi_process(padapter, _FALSE, &in_data, _rtw_mi_sreset_adapter_hdlbu);
}

#if defined(CONFIG_AP_MODE) && defined(DBG_CONFIG_ERROR_RESET) && defined(CONFIG_CONCURRENT_MODE)
void rtw_mi_ap_info_restore(_adapter *adapter)
{
	int i;
	_adapter *iface;
	struct mlme_priv *pmlmepriv;
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (iface) {
			pmlmepriv = &iface->mlmepriv;

			if (MLME_IS_AP(iface) || MLME_IS_MESH(iface)) {
				RTW_INFO(FUNC_ADPT_FMT" %s\n", FUNC_ADPT_ARG(iface), MLME_IS_AP(iface) ? "AP" : "MESH");
				rtw_iface_bcmc_sec_cam_map_restore(iface);
			}
		}
	}
}
#endif /*#if defined(DBG_CONFIG_ERROR_RESET) && defined(CONFIG_CONCURRENT_MODE)*/

u8 rtw_mi_buddy_sreset_adapter_hdlbu(_adapter *padapter, u8 bstart)
{
	u8 in_data = bstart;

	return _rtw_mi_process(padapter, _TRUE, &in_data, _rtw_mi_sreset_adapter_hdlbu);
}

#ifdef CONFIG_AP_MODE
static u8 _rtw_mi_tx_beacon_hdlbubu(_adapter *adapter, void *data)
{
	if ((MLME_IS_AP(adapter) || MLME_IS_MESH(adapter))
		&& check_fwstate(&adapter->mlmepriv, WIFI_ASOC_STATE) == _TRUE
	) {
		adapter->mlmepriv.update_bcn = _TRUE;
#ifndef CONFIG_INTERRUPT_BASED_TXBCN
#if defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI) || defined(CONFIG_PCI_BCN_POLLING)
		tx_beacon_hdlbu(adapter, NULL);
#endif
#endif
	}
	return _TRUE;
}
u8 rtw_mi_tx_beacon_hdlbubu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_tx_beacon_hdlbubu);
}
u8 rtw_mi_buddy_tx_beacon_hdlbubu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_sreset_adapter_hdlbu);
}

static u8 _rtw_mi_set_tx_beacon_cmdbubu(_adapter *adapter, void *data)
{
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;

	if (MLME_IS_AP(adapter) || MLME_IS_MESH(adapter)) {
		if (pmlmepriv->update_bcn == _TRUE)
			set_tx_beacon_cmdbu(adapter, 0);
	}
	return _TRUE;
}
u8 rtw_mi_set_tx_beacon_cmdbubu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_set_tx_beacon_cmdbubu);
}
u8 rtw_mi_buddy_set_tx_beacon_cmdbubu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_set_tx_beacon_cmdbubu);
}
#endif /* CONFIG_AP_MODE */

#ifdef CONFIG_P2P
static u8 _rtw_mi_p2p_chk_statebu(_adapter *adapter, void *data)
{
	struct wifidirect_info *pwdinfo = &(adapter->wdinfo);
	enum P2P_STATE state = *(enum P2P_STATE *)data;

	return rtw_p2p_chk_state(pwdinfo, state);
}
u8 rtw_mi_p2p_chk_statebu(_adapter *padapter, enum P2P_STATE p2p_state)
{
	u8 in_data = p2p_state;

	return _rtw_mi_process(padapter, _FALSE, &in_data, _rtw_mi_p2p_chk_statebu);
}
u8 rtw_mi_buddy_p2p_chk_statebu(_adapter *padapter, enum P2P_STATE p2p_state)
{
	u8 in_data  = p2p_state;

	return _rtw_mi_process(padapter, _TRUE, &in_data, _rtw_mi_p2p_chk_statebu);
}
static u8 _rtw_mi_stay_in_p2p_modebu(_adapter *adapter, void *data)
{
	struct wifidirect_info *pwdinfo = &(adapter->wdinfo);

	if (rtw_p2p_role(pwdinfo) != P2P_ROLE_DISABLE)
		return _TRUE;
	return _FALSE;
}
u8 rtw_mi_stay_in_p2p_modebu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _FALSE, NULL, _rtw_mi_stay_in_p2p_modebu);
}
u8 rtw_mi_buddy_stay_in_p2p_modebu(_adapter *padapter)
{
	return _rtw_mi_process(padapter, _TRUE, NULL, _rtw_mi_stay_in_p2p_modebu);
}
#endif /*CONFIG_P2P*/

_adapter *rtw_get_iface_by_idbu(_adapter *padapter, u8 iface_id)
{
	_adapter *iface = NULL;
	struct dvobj_priv *dvobj;

	if ((padapter == NULL) || (iface_id >= CONFIG_IFACE_NUMBER)) {
		rtw_warn_on(1);
		return iface;
	}

	dvobj = adapter_to_dvobj(padapter);
	return dvobj->padapters[iface_id];
}

_adapter *rtw_get_iface_by_macddrbu(_adapter *padapter, const u8 *mac_addr)
{
	int i;
	_adapter *iface = NULL;
	u8 bmatch = _FALSE;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if ((iface) && (_rtw_memcmpbu(mac_addr, adapter_mac_addr(iface), ETH_ALEN))) {
			bmatch = _TRUE;
			break;
		}
	}
	if (bmatch)
		return iface;
	else
		return NULL;
}

_adapter *rtw_get_iface_by_hwportbu(_adapter *padapter, u8 hw_port)
{
	int i;
	_adapter *iface = NULL;
	u8 bmatch = _FALSE;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if ((iface) && (hw_port == iface->hw_port)) {
			bmatch = _TRUE;
			break;
		}
	}
	if (bmatch)
		return iface;
	else
		return NULL;
}

/*#define CONFIG_SKB_ALLOCATED*/
#define DBG_SKB_PROCESS
#ifdef DBG_SKB_PROCESS
void rtw_dbg_skb_processbu(_adapter *padapter, union recv_frame *precvframe, union recv_frame *pcloneframe)
{
	_pkt *pkt_copy, *pkt_org;

	pkt_org = precvframe->u.hdr.pkt;
	pkt_copy = pcloneframe->u.hdr.pkt;
	/*
		RTW_INFO("%s ===== ORG SKB =====\n", __func__);
		RTW_INFO(" SKB head(%p)\n", pkt_org->head);
		RTW_INFO(" SKB data(%p)\n", pkt_org->data);
		RTW_INFO(" SKB tail(%p)\n", pkt_org->tail);
		RTW_INFO(" SKB end(%p)\n", pkt_org->end);

		RTW_INFO(" recv frame head(%p)\n", precvframe->u.hdr.rx_head);
		RTW_INFO(" recv frame data(%p)\n", precvframe->u.hdr.rx_data);
		RTW_INFO(" recv frame tail(%p)\n", precvframe->u.hdr.rx_tail);
		RTW_INFO(" recv frame end(%p)\n", precvframe->u.hdr.rx_end);

		RTW_INFO("%s ===== COPY SKB =====\n", __func__);
		RTW_INFO(" SKB head(%p)\n", pkt_copy->head);
		RTW_INFO(" SKB data(%p)\n", pkt_copy->data);
		RTW_INFO(" SKB tail(%p)\n", pkt_copy->tail);
		RTW_INFO(" SKB end(%p)\n", pkt_copy->end);

		RTW_INFO(" recv frame head(%p)\n", pcloneframe->u.hdr.rx_head);
		RTW_INFO(" recv frame data(%p)\n", pcloneframe->u.hdr.rx_data);
		RTW_INFO(" recv frame tail(%p)\n", pcloneframe->u.hdr.rx_tail);
		RTW_INFO(" recv frame end(%p)\n", pcloneframe->u.hdr.rx_end);
	*/
	/*
		RTW_INFO("%s => recv_frame adapter(%p,%p)\n", __func__, precvframe->u.hdr.adapter, pcloneframe->u.hdr.adapter);
		RTW_INFO("%s => recv_frame dev(%p,%p)\n", __func__, pkt_org->dev , pkt_copy->dev);
		RTW_INFO("%s => recv_frame len(%d,%d)\n", __func__, precvframe->u.hdr.len, pcloneframe->u.hdr.len);
	*/
	if (precvframe->u.hdr.len != pcloneframe->u.hdr.len)
		RTW_INFO("%s [WARN]  recv_frame length(%d:%d) compare failed\n", __func__, precvframe->u.hdr.len, pcloneframe->u.hdr.len);

	if (_rtw_memcmpbu(&precvframe->u.hdr.attrib, &pcloneframe->u.hdr.attrib, sizeof(struct rx_pkt_attrib)) == _FALSE)
		RTW_INFO("%s [WARN]  recv_frame attrib compare failed\n", __func__);

	if (_rtw_memcmpbu(precvframe->u.hdr.rx_data, pcloneframe->u.hdr.rx_data, precvframe->u.hdr.len) == _FALSE)
		RTW_INFO("%s [WARN]  recv_frame rx_data compare failed\n", __func__);

}
#endif

static s32 _rtw_mi_buddy_clone_bcmc_packetbu(_adapter *adapter, union recv_frame *precvframe, u8 *pphy_status, union recv_frame *pcloneframe)
{
	s32 ret = _SUCCESS;
#ifdef CONFIG_SKB_ALLOCATED
	u8 *pbuf = precvframe->u.hdr.rx_data;
#endif
	struct rx_pkt_attrib *pattrib = NULL;

	if (pcloneframe) {
		pcloneframe->u.hdr.adapter = adapter;

		_rtw_init_listheadbu(&pcloneframe->u.hdr.list);
		pcloneframe->u.hdr.precvbuf = NULL;	/*can't access the precvbuf for new arch.*/
		pcloneframe->u.hdr.len = 0;

		_rtw_memcpybu(&pcloneframe->u.hdr.attrib, &precvframe->u.hdr.attrib, sizeof(struct rx_pkt_attrib));

		pattrib = &pcloneframe->u.hdr.attrib;
#ifdef CONFIG_SKB_ALLOCATED
		if (rtw_os_alloc_recvframebu(adapter, pcloneframe, pbuf, NULL) == _SUCCESS)
#else
		if (rtw_os_recvframe_duplicate_skbbu(adapter, pcloneframe, precvframe->u.hdr.pkt) == _SUCCESS)
#endif
		{
#ifdef CONFIG_SKB_ALLOCATED
			recvframe_put(pcloneframe, pattrib->pkt_len);
#endif

#ifdef DBG_SKB_PROCESS
			rtw_dbg_skb_processbu(adapter, precvframe, pcloneframe);
#endif

			if (pphy_status)
				rx_query_phy_statusbu(pcloneframe, pphy_status);

			ret = rtw_recv_entrybu(pcloneframe);
		} else {
			ret = -1;
			RTW_INFO("%s()-%d: rtw_os_alloc_recvframebu() failed!\n", __func__, __LINE__);
		}

	}
	return ret;
}

void rtw_mi_buddy_clone_bcmc_packetbu(_adapter *padapter, union recv_frame *precvframe, u8 *pphy_status)
{
	int i;
	s32 ret = _SUCCESS;
	_adapter *iface = NULL;
	union recv_frame *pcloneframe = NULL;
	struct recv_priv *precvpriv = &padapter->recvpriv;/*primary_padapter*/
	_queue *pfree_recv_queue = &precvpriv->free_recv_queue;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	u8 *fhead = get_recvframe_data(precvframe);
	u8 type = GetFrameType(fhead);

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (!iface || iface == padapter)
			continue;
		if (rtw_is_adapter_upbu(iface) == _FALSE || iface->registered == 0)
			continue;
		if (type == WIFI_DATA_TYPE && !adapter_allow_bmc_data_rxbu(iface))
			continue;

		pcloneframe = rtw_alloc_recvframebu(pfree_recv_queue);
		if (pcloneframe) {
			ret = _rtw_mi_buddy_clone_bcmc_packetbu(iface, precvframe, pphy_status, pcloneframe);
			if (_SUCCESS != ret) {
				if (ret == -1)
					rtw_free_recvframebu(pcloneframe, pfree_recv_queue);
				/*RTW_INFO(ADPT_FMT"-clone BC/MC frame failed\n", ADPT_ARG(iface));*/
			}
		}
	}

}

#ifdef CONFIG_PCI_HCI
/*API be created temporary for MI, caller is interrupt-handler, PCIE's interrupt handler cannot apply to multi-AP*/
_adapter *rtw_mi_get_ap_adapter(_adapter *padapter)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	int i;
	_adapter *iface = NULL;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (!iface)
			continue;

		if (check_fwstate(&iface->mlmepriv, WIFI_AP_STATE) == _TRUE
		    && check_fwstate(&iface->mlmepriv, WIFI_ASOC_STATE) == _TRUE)
			break;

	}
	return iface;
}
#endif

u8 rtw_mi_get_ld_sta_ifbmpbu(_adapter *adapter)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	int i;
	_adapter *iface = NULL;
	u8 ifbmp = 0;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (!iface)
			continue;

		if (MLME_IS_STA(iface) && MLME_IS_ASOC(iface))
			ifbmp |= BIT(i);
	}

	return ifbmp;
}

u8 rtw_mi_get_ap_mesh_ifbmpbu(_adapter *adapter)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	int i;
	_adapter *iface = NULL;
	u8 ifbmp = 0;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (!iface)
			continue;

		if (CHK_MLME_STATE(iface, WIFI_AP_STATE | WIFI_MESH_STATE)
			&& MLME_IS_ASOC(iface))
			ifbmp |= BIT(i);
	}

	return ifbmp;
}

void rtw_mi_update_ap_bmc_camidbu(_adapter *padapter, u8 camid_a, u8 camid_b)
{
#ifdef CONFIG_CONCURRENT_MODE
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct macid_ctl_t *macid_ctl = dvobj_to_macidctl(dvobj);

	int i;
	_adapter *iface = NULL;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if (!iface)
			continue;

		if (macid_ctl->iface_bmc[iface->iface_id] != INVALID_SEC_MAC_CAM_ID) {
			if (macid_ctl->iface_bmc[iface->iface_id] == camid_a)
				macid_ctl->iface_bmc[iface->iface_id] = camid_b;
			else if (macid_ctl->iface_bmc[iface->iface_id] == camid_b)
				macid_ctl->iface_bmc[iface->iface_id] = camid_a;
			iface->securitypriv.dot118021x_bmc_cam_id  = macid_ctl->iface_bmc[iface->iface_id];
		}
	}
#endif
}

u8 rtw_mi_get_assoc_if_numbu(_adapter *adapter)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	u8 n_assoc_iface = 0;
#if 1
	u8 i;

	for (i = 0; i < dvobj->iface_nums; i++) {
		if (check_fwstate(&(dvobj->padapters[i]->mlmepriv), WIFI_ASOC_STATE))
			n_assoc_iface++;
	}
#else
	n_assoc_iface = DEV_STA_LD_NUM(dvobj) + DEV_AP_NUM(dvobj) + DEV_ADHOC_NUM(dvobj) + DEV_MESH_NUM(dvobj);
#endif
	return n_assoc_iface;
}
