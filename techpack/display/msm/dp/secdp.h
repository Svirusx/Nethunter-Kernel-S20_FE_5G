/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
#ifndef __SECDP_H
#define __SECDP_H

#include <linux/usb/usbpd.h>
#include <linux/usb/manager/usb_typec_manager_notifier.h>
#include <linux/ccic/ccic_notifier.h>
#include <linux/secdp_logger.h>
#include <linux/pm_wakeup.h>

#include "dp_power.h"
#include "dp_panel.h"
#include "dp_catalog.h"

extern unsigned int lpcharge;

/*defined at include/linux/ccic/ccic_alternate.h*/
#define SAMSUNG_VENDOR_ID	0x04E8

#define DEXDOCK_PRODUCT_ID	0xA020	/* EE-MG950, DeX station */
#define HG950_PRODUCT_ID	0xA025	/* EE-HG950 */
#define MPA2_PRODUCT_ID		0xA027	/* EE-P5000 */
#define DEXPAD_PRODUCT_ID	0xA029	/* EE-M5100 */
#define DEXCABLE_PRODUCT_ID	0xA048	/* EE-I3100 */
#define MPA3_PRODUCT_ID		0xA056	/* EE-P3200 */

#define SECDP_ENUM_STR(x)	#x
#define dim(x)			((sizeof(x))/(sizeof(x[0])))

/*#define SECDP_HDCP_DISABLE*/
#define SECDP_USB_CONCURRENCY
#define SECDP_USE_WAKELOCK
/*#define NOT_SUPPORT_DEX_RES_CHANGE*/
#define SECDP_MAX_HBR2
/*#define SECDP_AUDIO_CTS*/
/*#define SECDP_SUPPORT_ODYSSEY*/
/*#define SECDP_TEST_HDCP2P2_REAUTH*/

/*#define SECDP_CALIBRATE_VXPX*/	/* for calibration of v_level, p_level*/
#define SECDP_OPTIMAL_LINK_RATE		/* use optimum link_rate, not max link_rate*/

#define SECDP_WIDE_21_9_SUPPORT		/* support ultra-wide 21:9 resolution*/
#define SECDP_WIDE_32_9_SUPPORT		/* support ultra-wide 32:9 resolution*/
#define SECDP_WIDE_32_10_SUPPORT	/* support ultra-wide 32:10 resolution*/
/*#define SECDP_HIGH_REFRESH_SUPPORT*/	/* support high refresh rate (g.t.60hz)*/

#define LEN_BRANCH_REVISION		3
#define		DPCD_BRANCH_HW_REVISION         0x509
#define		DPCD_BRANCH_SW_REVISION_MAJOR   0x50A
#define		DPCD_BRANCH_SW_REVISION_MINOR   0x50B

#define MAX_CNT_LINK_STATUS_UPDATE		4
#define MAX_CNT_HDCP_RETRY			10

/* MST: max resolution, max refresh rate, max pclk */
#define MST_MAX_COLS	3840
#define MST_MAX_ROWS	2160
#define MST_MAX_FPS	30
#define MST_MAX_PCLK	300000

/* displayport self test */
#if defined(CONFIG_SEC_DISPLAYPORT) && defined(CONFIG_SEC_DISPLAYPORT_ENG)
#define SECDP_SELF_TEST
#endif

#ifdef SECDP_SELF_TEST
#define ST_EDID_SIZE	256
#define ST_ARG_CNT	20

enum {
	ST_CLEAR_CMD,
	ST_LANE_CNT,
	ST_LINK_RATE,
	ST_CONNECTION_TEST,
	ST_HDCP_TEST,
	ST_EDID,
	ST_PREEM_TUN,
	ST_VOLTAGE_TUN,
	ST_MAX,
};

struct secdp_sef_test_item {
	char cmd_str[20];
	int arg[ST_ARG_CNT];
	int arg_cnt;
	char arg_str[100];
	bool enabled;
	void (*clear)(void);
};

int  secdp_self_test_status(int cmd);
void secdp_self_test_start_reconnect(void (*func)(void));
void secdp_self_test_start_hdcp_test(void (*func_on)(void),
	void (*func_off)(void));
u8  *secdp_self_test_get_edid(void);
void secdp_self_register_clear_func(int cmd, void (*func)(void));
int *secdp_self_test_get_arg(int cmd);
#endif

/* monitor aspect ratio */
enum mon_aspect_ratio_t {
	MON_RATIO_NA = -1,
	MON_RATIO_16_9,
	MON_RATIO_16_10,
	MON_RATIO_21_9,
	MON_RATIO_32_9,
	MON_RATIO_32_10,
};

/* dex supported resolutions */
enum dex_support_res_t {
	DEX_RES_NOT_SUPPORT = 0,
	DEX_RES_1920X1080, /* FHD */
	DEX_RES_1920X1200, /* WUXGA */
	DEX_RES_2560X1080, /* UW-UXGA */
	DEX_RES_2560X1440, /* QHD */
	DEX_RES_2560X1600, /* WQXGA */
	DEX_RES_3440X1440, /* UW-QHD */
	DEX_RES_3840X2160, /* UHD */
};
#define DEX_RES_DFT	DEX_RES_1920X1080   /* DeX default resolution */
#define DEX_RES_MAX	DEX_RES_3440X1440   /* DeX max resolution */
#define DEX_FPS_MIN	50                  /* DeX min refresh rate */
#define DEX_FPS_MAX	60                  /* DeX max refresh rate */

static inline char *secdp_dex_res_to_string(int res)
{
	switch (res) {
	case DEX_RES_NOT_SUPPORT:
		return DP_ENUM_STR(DEX_RES_NOT_SUPPORT);
	case DEX_RES_1920X1080:
		return DP_ENUM_STR(DEX_RES_1920X1080);
	case DEX_RES_1920X1200:
		return DP_ENUM_STR(DEX_RES_1920X1200);
	case DEX_RES_2560X1080:
		return DP_ENUM_STR(DEX_RES_2560X1080);
	case DEX_RES_2560X1440:
		return DP_ENUM_STR(DEX_RES_2560X1440);
	case DEX_RES_2560X1600:
		return DP_ENUM_STR(DEX_RES_2560X1600);
	case DEX_RES_3440X1440:
		return DP_ENUM_STR(DEX_RES_3440X1440);
	case DEX_RES_3840X2160:
		return DP_ENUM_STR(DEX_RES_3840X2160);
	default:
		return "unknown";
	}
}

enum DEX_STATUS {
	DEX_DISABLED,
	DEX_ENABLED,
	DEX_DURING_MODE_CHANGE,
};

struct secdp_adapter {
	uint ven_id;
	uint prod_id;
};

#define MON_NAME_LEN	14	/*monitor name length, max 13 chars + null*/

#define MAX_NUM_HMD	32
#define DEX_TAG_HMD	"HMD"

struct secdp_sink_dev {
	uint ven_id;		/*vendor id from CCIC*/
	uint prod_id;		/*product id from CCIC*/
	char monitor_name[MON_NAME_LEN];	/* from EDID */
};

struct secdp_dex {
	struct class		*sysfs_class;

	int  prev;		/*previously known as "dex_now"*/
	int  curr;		/*previously known as "dex_en"*/
	int  setting_ui;	/*"dex_set", true if setting has Dex mode*/

	enum dex_support_res_t res;	/*dex supported resolution*/
	char fw_ver[10];	/*firmware ver, 0:h/w, 1:s/w major, 2:s/w minor*/
	int  reconnecting;	/*it's 1 during dex reconnecting*/
};

struct secdp_debug {
	bool prefer_check_skip;
};

struct secdp_misc {
	bool			ccic_noti_registered;
	struct delayed_work	ccic_noti_reg_work;
	struct notifier_block	ccic_noti_block;

	bool			hpd_noti_deferred;
	struct delayed_work	hpd_noti_work;
	struct delayed_work	hdcp_start_work;
	struct delayed_work	link_status_work;
	struct delayed_work	poor_discon_work;

	bool is_mst_receiver;	/*true if MST receiver, false otherwise(SST)*/
	int  cnt_mst_hpd;	/*avoid successive hpd low/high/low/.. from MST adapter*/

#ifdef SECDP_SELF_TEST
	struct delayed_work	self_test_reconnect_work;
	struct delayed_work	self_test_hdcp_test_work;

	void (*self_test_reconnect_callback)(void);
	void (*self_test_hdcp_on_callback)(void);
	void (*self_test_hdcp_off_callback)(void);

	u8 self_test_edid[ST_EDID_SIZE];
#endif

	struct secdp_adapter	adapter;
	struct secdp_dex	dex;
	struct secdp_debug	debug;

	int  max_mir_res_idx;	/*Index of max resolution by sink(mirror mode)*/
	int  max_dex_res_idx;	/*Index of max resolution by dex mode*/
	int  prefer_res_idx;	/*Index of preferred resolution*/
	int  ignore_ratio;

	struct secdp_sink_dev  *hmd_list;  /*list of supported HMD device*/
	struct mutex		hmd_lock;
	bool hmd_dev;		/*true if connected sink is known HMD device*/

	bool cable_connected;	/*previously known as "cable_connected_phy"*/
	bool link_conf;		/*previously known as "sec_link_conf"*/
	atomic_t hpd;		/*previously known as "sec_hpd"*/
	bool reboot;		/*true if rebooted or shutdown*/
	int  hdcp_retry;	/*count if dp link is unstable during hdcp*/

	bool has_prefer;	/*true if preferred resolution*/
	int  prefer_hdisp;	/*horizontal pixel of preferred resolution*/
	int  prefer_vdisp;	/*vertical pixel of preferred resolution*/
	int  prefer_refresh;	/*refresh rate of preferred resolution*/
	bool ignore_prefer;	/*true if larger refresh rate exists*/
	enum mon_aspect_ratio_t	prefer_ratio;

	struct mutex		notifier_lock;

#ifdef SECDP_USE_WAKELOCK
	struct wakeup_source	ws;
#endif
};

struct secdp_display_timing {
	int  index;		/*resolution priority*/
	int  active_h;
	int  active_v;
	int  refresh_rate;
	bool interlaced;
	enum dex_support_res_t	dex_res;	/*dex supported resolution*/
	enum mon_aspect_ratio_t	mon_ratio;	/*monitor aspect ratio*/
	int  supported;				/*for unit test*/
};

#define EV_USBPD_ATTENTION	BIT(13)

static inline char *secdp_ev_event_to_string(int event)
{
	switch (event) {
	case EV_USBPD_ATTENTION:
		return DP_ENUM_STR(EV_USBPD_ATTENTION);
	default:
		return "unknown";
	}
}

struct secdp_attention_node {
	CC_NOTI_TYPEDEF noti;
	struct list_head list;
};

bool secdp_check_if_lpm_mode(void);
int  secdp_send_deferred_hpd_noti(void);
bool secdp_get_clk_status(enum dp_pm_type type);
void secdp_send_poor_connection_event(void);

int  secdp_ccic_noti_register_ex(struct secdp_misc *sec, bool retry);
bool secdp_get_power_status(void);
bool secdp_get_cable_status(void);
bool secdp_get_hpd_irq_status(void);
int  secdp_get_hpd_status(void);
bool secdp_get_poor_connection_status(void);
bool secdp_get_link_train_status(void);
struct dp_panel *secdp_get_panel_info(void);
struct drm_connector *secdp_get_connector(void);

/** redriver devices */
enum secdp_redrv_dev {
	SECDP_REDRV_NONE = 0,
	SECDP_REDRV_PTN36502,	/* don't need AUX_SEL control */
	SECDP_REDRV_PS5169,	/* need AUX_SEL control */
};

static inline char *secdp_redrv_to_string(int res)
{
	switch (res) {
	case SECDP_REDRV_NONE:
		return DP_ENUM_STR(SECDP_REDRV_NONE);
	case SECDP_REDRV_PTN36502:
		return DP_ENUM_STR(SECDP_REDRV_PTN36502);
	case SECDP_REDRV_PS5169:
		return DP_ENUM_STR(SECDP_REDRV_PS5169);
	default:
		return "unknown";
	}
}

void secdp_redriver_onoff(bool enable, int lane);

/** adapter type : SST or MST */
enum secdp_adapter_t {
	SECDP_ADT_UNKNOWN = -1,
	SECDP_ADT_SST = 10,
	SECDP_ADT_MST = 11,
};
int  secdp_is_mst_receiver(void);

int  secdp_power_request_gpios(struct dp_power *dp_power);
void secdp_config_gpios_factory(int aux_sel, bool out_en);
enum plug_orientation secdp_get_plug_orientation(void);
bool secdp_get_reboot_status(void);

bool secdp_check_hmd_dev(const char *name_to_search);
int  secdp_store_hmd_dev(char* buf, size_t len, int num);
#ifdef CONFIG_SEC_DISPLAYPORT_ENG
int  secdp_show_hmd_dev(char *buf);
#endif

void secdp_dex_res_init(void);
void secdp_dex_do_reconnecting(void);
bool secdp_check_dex_reconnect(void);
bool secdp_check_dex_mode(void);
enum dex_support_res_t secdp_get_dex_res(void);

void secdp_clear_link_status_update_cnt(struct dp_link *dp_link);
void secdp_reset_link_status(struct dp_link *dp_link);
bool secdp_check_link_stable(struct dp_link *dp_link);

bool secdp_panel_hdr_supported(void);
bool secdp_find_supported_resolution(struct dp_panel_info *timing);

#ifdef SECDP_CALIBRATE_VXPX
/* preshoot adjustment */
int  secdp_catalog_prsht0_show(char *val);
int  secdp_catalog_prsht0_store(char val);
int  secdp_catalog_prsht1_show(char *val);
int  secdp_catalog_prsht1_store(char val);

/* legacy */
int  secdp_catalog_vx_show(char *buf, int len);
int  secdp_catalog_vx_store(int *val, int size);
int  secdp_catalog_px_show(char *buf, int len);
int  secdp_catalog_px_store(int *val, int size);

/* HBR2, HBR3 */
int  secdp_catalog_vx_hbr2_hbr3_show(char *buf, int len);
int  secdp_catalog_vx_hbr2_hbr3_store(int *val, int size);
int  secdp_catalog_px_hbr2_hbr3_show(char *buf, int len);
int  secdp_catalog_px_hbr2_hbr3_store(int *val, int size);

/* RBR, HBR */
int  secdp_catalog_vx_hbr_rbr_show(char *buf, int len);
int  secdp_catalog_vx_hbr_rbr_store(int *val, int size);
int  secdp_catalog_px_hbr_rbr_show(char *buf, int len);
int  secdp_catalog_px_hbr_rbr_store(int *val, int size);

int  secdp_debug_prefer_skip_show(void);
void secdp_debug_prefer_skip_store(bool skip);
int  secdp_debug_prefer_ratio_show(void);
void secdp_debug_prefer_ratio_store(int ratio);
#endif

#endif/*__SECDP_H*/
