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

#ifndef __OSDEP_INTF_H_
#define __OSDEP_INTF_H_


struct intf_priv {

	u8 *intf_dev;
	u32	max_iosz;	/* USB2.0: 128, USB1.1: 64, SDIO:64 */
	u32	max_xmitsz; /* USB2.0: unlimited, SDIO:512 */
	u32	max_recvsz; /* USB2.0: unlimited, SDIO:512 */

	volatile u8 *io_rwmem;
	volatile u8 *allocated_io_rwmem;
	u32	io_wsz; /* unit: 4bytes */
	u32	io_rsz;/* unit: 4bytes */
	u8 intf_status;

	void (*_bus_io)(u8 *priv);

	/*
	Under Sync. IRP (SDIO/USB)
	A protection mechanism is necessary for the io_rwmem(read/write protocol)

	Under Async. IRP (SDIO/USB)
	The protection mechanism is through the pending queue.
	*/

	_mutex ioctl_mutex;


#ifdef PLATFORM_LINUX
#ifdef CONFIG_USB_HCI
	/* when in USB, IO is through interrupt in/out endpoints */
	struct usb_device	*udev;
	PURB	piorw_urb;
	u8 io_irp_cnt;
	u8 bio_irp_pending;
	_sema io_retevt;
	_timer	io_timer;
	u8 bio_irp_timeout;
	u8 bio_timer_cancel;
#endif
#endif

};

struct dvobj_priv *devobj_initx(void);
void devobj_deinitx(struct dvobj_priv *pdvobj);

u8 rtw_init_drv_swx(_adapter *padapter);
u8 rtw_free_drv_swx(_adapter *padapter);
u8 rtw_reset_drv_swx(_adapter *padapter);
void rtw_dev_unloadx(PADAPTER padapter);

u32 rtw_start_drv_threadsx(_adapter *padapter);
void rtw_stop_drv_threadsx(_adapter *padapter);
#if defined(CONFIG_WOWLAN) || defined(CONFIG_AP_WOWLAN)
void rtw_cancel_dynamic_chk_timer(_adapter *padapter);
#endif
void rtw_cancel_all_timerx(_adapter *padapter);

uint loadparamx(_adapter *adapter);

#ifdef PLATFORM_LINUX
int rtw_ioctlx(struct net_device *dev, struct ifreq *rq, int cmd);

int rtw_init_netdevx_name(struct net_device *pnetdev, const char *ifnamex);
struct net_device *rtw_init_netdevx(_adapter *padapter);

void rtw_os_ndev_freex(_adapter *adapter);
int rtw_os_ndev_initx(_adapter *adapter, const char *name);
void rtw_os_ndev_deinitx(_adapter *adapter);
void rtw_os_ndev_unregisterx(_adapter *adapter);
void rtw_os_ndevs_unregisterx(struct dvobj_priv *dvobj);
int rtw_os_ndevs_initx(struct dvobj_priv *dvobj);
void rtw_os_ndevs_deinitx(struct dvobj_priv *dvobj);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
u16 rtw_recv_select_queuex(struct sk_buff *skb);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35) */

int rtw_ndev_notifier_registerx(void);
void rtw_ndev_notifier_unregisterx(void);
void rtw_inetaddr_notifier_registerx(void);
void rtw_inetaddr_notifier_unregisterx(void);

#include "../os_dep/linux/rtw_proc.h"

#ifdef CONFIG_IOCTL_CFG80211
	#include "../os_dep/linux/ioctl_cfg80211.h"
#endif /* CONFIG_IOCTL_CFG80211 */

u8 rtw_rtnl_lock_neededx(struct dvobj_priv *dvobj);
void rtw_set_rtnl_lock_holderx(struct dvobj_priv *dvobj, _thread_hdl_ thd_hdl);

#endif /* PLATFORM_LINUX */


#ifdef PLATFORM_FREEBSD
extern int rtw_ioctlx(struct ifnet *ifp, u_long cmd, caddr_t data);
#endif

void rtw_ips_dev_unloadx(_adapter *padapter);

#ifdef CONFIG_IPS
int rtw_ips_pwr_up(_adapter *padapter);
void rtw_ips_pwr_down(_adapter *padapter);
#endif

#ifdef CONFIG_CONCURRENT_MODE
struct _io_ops;
struct dvobj_priv;
_adapter *rtw_drv_add_vir_if(_adapter *primary_padapter, void (*set_intf_ops)(_adapter *primary_padapter, struct _io_ops *pops));
void rtw_drv_stop_vir_ifaces(struct dvobj_priv *dvobj);
void rtw_drv_free_vir_ifaces(struct dvobj_priv *dvobj);
#endif

void rtw_ndev_destructorx(_nic_hdl ndev);
#ifdef CONFIG_ARP_KEEP_ALIVE
int rtw_gw_addr_query(_adapter *padapter);
#endif

int rtw_suspend_commonx(_adapter *padapter);
int rtw_resume_commonx(_adapter *padapter);

#endif /* _OSDEP_INTF_H_ */
