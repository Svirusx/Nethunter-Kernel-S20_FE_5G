/*
 * Copyrights (C) 2016-2019 Samsung Electronics, Inc.
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
 */

#ifndef __CCIC_NOTIFIER_H__
#define __CCIC_NOTIFIER_H__

/* CCIC notifier call sequence,
 * largest priority number device will be called first.
 * refer Team Docs > ETC > 1. USB Type C >
 * code managing > ccic_notifier, pdic_notifier
 */
#define CCIC_NOTI_DEST_NUM			(12)
#define CCIC_NOTI_ID_NUM			(17)
#define CCIC_NOTI_RID_NUM			(8)
#define CCIC_NOTI_USB_STATUS_NUM	(4)
#define CCIC_NOTI_PIN_STATUS_NUM	(8)

typedef enum {
	CCIC_NOTIFY_DEV_INITIAL		= 0,
	CCIC_NOTIFY_DEV_USB			= 1,
	CCIC_NOTIFY_DEV_BATTERY		= 2,
	CCIC_NOTIFY_DEV_PDIC		= 3,
	CCIC_NOTIFY_DEV_MUIC		= 4,
	CCIC_NOTIFY_DEV_CCIC		= 5,
	CCIC_NOTIFY_DEV_MANAGER		= 6,
	CCIC_NOTIFY_DEV_DP			= 7,
	CCIC_NOTIFY_DEV_USB_DP		= 8,
	CCIC_NOTIFY_DEV_SUB_BATTERY	= 9,
	CCIC_NOTIFY_DEV_SECOND_MUIC = 10,
	CCIC_NOTIFY_DEV_ALL 		= 11,
} ccic_notifier_device_t;

typedef enum {
	CCIC_NOTIFY_ID_INITIAL			= 0,
	CCIC_NOTIFY_ID_ATTACH			= 1,
	CCIC_NOTIFY_ID_RID				= 2,
	CCIC_NOTIFY_ID_USB				= 3,
	CCIC_NOTIFY_ID_POWER_STATUS		= 4,
	CCIC_NOTIFY_ID_WATER			= 5,
	CCIC_NOTIFY_ID_VCONN			= 6,
	CCIC_NOTIFY_ID_OTG				= 7,
	CCIC_NOTIFY_ID_TA				= 8,
	CCIC_NOTIFY_ID_DP_CONNECT		= 9,
	CCIC_NOTIFY_ID_DP_HPD			= 10,
	CCIC_NOTIFY_ID_DP_LINK_CONF		= 11,
	CCIC_NOTIFY_ID_USB_DP			= 12,
	CCIC_NOTIFY_ID_ROLE_SWAP		= 13,
	CCIC_NOTIFY_ID_FAC				= 14,
	CCIC_NOTIFY_ID_CC_PIN_STATUS	= 15,
	CCIC_NOTIFY_ID_WATER_CABLE		= 16,
} ccic_notifier_id_t;

typedef enum {
	RID_UNDEFINED	= 0,
	RID_000K		= 1,
	RID_001K		= 2,
	RID_255K		= 3,
	RID_301K		= 4,
	RID_523K		= 5,
	RID_619K		= 6,
	RID_OPEN		= 7,
} ccic_notifier_rid_t;

typedef enum {
	USB_STATUS_NOTIFY_DETACH		= 0,
	USB_STATUS_NOTIFY_ATTACH_DFP	= 1,
	USB_STATUS_NOTIFY_ATTACH_UFP	= 2,
	USB_STATUS_NOTIFY_ATTACH_DRP	= 3,
} USB_STATUS;

typedef enum {
	CCIC_NOTIFY_PIN_STATUS_NO_DETERMINATION = 0,
	CCIC_NOTIFY_PIN_STATUS_CC1_ACTIVE		= 1,
	CCIC_NOTIFY_PIN_STATUS_CC2_ACTIVE		= 2,
	CCIC_NOTIFY_PIN_STATUS_AUDIO_ACCESSORY	= 3,
	CCIC_NOTIFY_PIN_STATUS_DEBUG_ACCESSORY	= 4,
	CCIC_NOTIFY_PIN_STATUS_CCIC_ERROR		= 5,
	CCIC_NOTIFY_PIN_STATUS_DISABLED			= 6,
	CCIC_NOTIFY_PIN_STATUS_RFU				= 7,
} ccic_notifier_pin_status_t;

typedef enum {
	CCIC_NOTIFY_DP_PIN_UNKNOWN	= 0,
	CCIC_NOTIFY_DP_PIN_A		= 1,
	CCIC_NOTIFY_DP_PIN_B		= 2,
	CCIC_NOTIFY_DP_PIN_C		= 3,
	CCIC_NOTIFY_DP_PIN_D		= 4,
	CCIC_NOTIFY_DP_PIN_E		= 5,
	CCIC_NOTIFY_DP_PIN_F		= 6,
} ccic_notifier_dp_pinconf_t;

typedef enum {
	CCIC_NOTIFY_DETACH = 0,
	CCIC_NOTIFY_ATTACH = 1,
} ccic_notifier_attach_t;

typedef enum {
	CCIC_NOTIFY_DEVICE	= 0,
	CCIC_NOTIFY_HOST	= 1,
} ccic_notifier_attach_rprd_t;

typedef enum {
	CCIC_NOTIFY_LOW		= 0,
	CCIC_NOTIFY_HIGH	= 1,
	CCIC_NOTIFY_IRQ		= 2,
} ccic_notifier_dp_hpd_t;

typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t sub1:16;
	uint64_t sub2:16;
	uint64_t sub3:16;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} CC_NOTI_TYPEDEF;

/* ID = 1 : Attach */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t attach:16;
	uint64_t rprd:16;
	uint64_t cable_type:16;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} CC_NOTI_ATTACH_TYPEDEF;

/* ID = 2 : RID */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t rid:16;
	uint64_t sub2:16;
	uint64_t sub3:16;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} CC_NOTI_RID_TYPEDEF;

/* ID = 3 : USB status */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t attach:16;
	uint64_t drp:16;
	uint64_t sub3:16;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} CC_NOTI_USB_STATUS_TYPEDEF;

typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t is_connect:16;
	uint64_t hs_connect:16;
	uint64_t reserved:16;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} USB_DP_NOTI_TYPEDEF;

struct ccic_notifier_struct {
	CC_NOTI_TYPEDEF ccic_template;
	struct blocking_notifier_head notifier_call_chain;
};

#define CCIC_NOTIFIER_BLOCK(name)	\
	struct notifier_block (name)

extern char CCIC_NOTI_DEST_Print[CCIC_NOTI_DEST_NUM][10];
extern char CCIC_NOTI_ID_Print[CCIC_NOTI_ID_NUM][20];
extern char CCIC_NOTI_RID_Print[CCIC_NOTI_RID_NUM][15];
extern char CCIC_NOTI_USB_STATUS_Print[CCIC_NOTI_USB_STATUS_NUM][20];

extern int ccic_notifier_notify(CC_NOTI_TYPEDEF *noti, void *pd,
		int pdic_attach);
extern int ccic_notifier_register(struct notifier_block *nb,
		notifier_fn_t notifier, ccic_notifier_device_t listener);
extern int ccic_notifier_unregister(struct notifier_block *nb);
extern int ccic_notifier_init(void);
#endif /* __CCIC_NOTIFIER_H__ */
