// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/component.h>
#include <linux/of_irq.h>
#include <linux/extcon.h>
#ifndef CONFIG_SEC_DISPLAYPORT
#include <linux/soc/qcom/fsa4480-i2c.h>
#endif
#include <linux/usb/usbpd.h>

#include "sde_connector.h"

#include "msm_drv.h"
#include "dp_hpd.h"
#include "dp_parser.h"
#include "dp_power.h"
#include "dp_catalog.h"
#include "dp_aux.h"
#include "dp_link.h"
#include "dp_panel.h"
#include "dp_ctrl.h"
#include "dp_audio.h"
#include "dp_display.h"
#include "sde_hdcp.h"
#include "dp_debug.h"
#include "sde_dbg.h"

#ifdef CONFIG_SEC_DISPLAYPORT
/*#undef CONFIG_SWITCH*/
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif
#include <linux/string.h>
#include <linux/reboot.h>
#include <linux/sec_displayport.h>
#include <linux/sched/clock.h>
#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
#include <linux/displayport_bigdata.h>
#endif
#include "secdp.h"
#include "secdp_sysfs.h"
#define CCIC_DP_NOTI_REG_DELAY		1000

#ifdef SECDP_EVENT_THREAD
struct secdp_event {
	struct dp_display_private *dp;
	u32 id;
};

#define SECDP_EVENT_Q_MAX 4

struct secdp_event_data {
	wait_queue_head_t event_q;
	u32 pndx;
	u32 gndx;
	struct secdp_event event_list[SECDP_EVENT_Q_MAX];
	spinlock_t event_lock;
};
#endif
#endif

#define DP_MST_DEBUG(fmt, ...) DP_DEBUG(fmt, ##__VA_ARGS__)

#define dp_display_state_show(x) { \
	DP_ERR("%s: state (0x%x): %s\n", x, dp->state, \
		dp_display_state_name(dp->state)); \
	SDE_EVT32_EXTERNAL(dp->state); }

#define dp_display_state_log(x) { \
	DP_DEBUG("%s: state (0x%x): %s\n", x, dp->state, \
		dp_display_state_name(dp->state)); \
	SDE_EVT32_EXTERNAL(dp->state); }

#define dp_display_state_is(x) (dp->state & (x))
#define dp_display_state_add(x) { \
	(dp->state |= (x)); \
	dp_display_state_log("add "#x); }
#define dp_display_state_remove(x) { \
	(dp->state &= ~(x)); \
	dp_display_state_log("remove "#x); }

enum dp_display_states {
	DP_STATE_DISCONNECTED           = 0,
	DP_STATE_CONFIGURED             = BIT(0),
	DP_STATE_INITIALIZED            = BIT(1),
	DP_STATE_READY                  = BIT(2),
	DP_STATE_CONNECTED              = BIT(3),
	DP_STATE_CONNECT_NOTIFIED       = BIT(4),
	DP_STATE_DISCONNECT_NOTIFIED    = BIT(5),
	DP_STATE_ENABLED                = BIT(6),
	DP_STATE_SUSPENDED              = BIT(7),
	DP_STATE_ABORTED                = BIT(8),
	DP_STATE_HDCP_ABORTED           = BIT(9),
	DP_STATE_SRC_PWRDN              = BIT(10),
};

static char *dp_display_state_name(enum dp_display_states state)
{
	static char buf[SZ_1K];
	u32 len = 0;

	memset(buf, 0, SZ_1K);

	if (state & DP_STATE_CONFIGURED)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"CONFIGURED");

	if (state & DP_STATE_INITIALIZED)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"INITIALIZED");

	if (state & DP_STATE_READY)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"READY");

	if (state & DP_STATE_CONNECTED)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"CONNECTED");

	if (state & DP_STATE_CONNECT_NOTIFIED)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"CONNECT_NOTIFIED");

	if (state & DP_STATE_DISCONNECT_NOTIFIED)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"DISCONNECT_NOTIFIED");

	if (state & DP_STATE_ENABLED)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"ENABLED");

	if (state & DP_STATE_SUSPENDED)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"SUSPENDED");

	if (state & DP_STATE_ABORTED)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"ABORTED");

	if (state & DP_STATE_HDCP_ABORTED)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"HDCP_ABORTED");

	if (state & DP_STATE_SRC_PWRDN)
		len += scnprintf(buf + len, sizeof(buf) - len, "|%s|",
			"SRC_PWRDN");

	if (!strlen(buf))
		return "DISCONNECTED";

	return buf;
}

static struct dp_display *g_dp_display;
#define HPD_STRING_SIZE 30

#if defined(CONFIG_SEC_DISPLAYPORT) && defined(CONFIG_SWITCH)
static struct switch_dev switch_secdp_msg = {
	.name = "secdp_msg",
};
#endif

struct dp_hdcp_dev {
	void *fd;
	struct sde_hdcp_ops *ops;
	enum sde_hdcp_version ver;
};

struct dp_hdcp {
	void *data;
	struct sde_hdcp_ops *ops;

	u32 source_cap;

	struct dp_hdcp_dev dev[HDCP_VERSION_MAX];
};

struct dp_mst {
	bool mst_active;

	bool drm_registered;
	struct dp_mst_drm_cbs cbs;
};

struct dp_display_private {
	char *name;
	int irq;

	enum drm_connector_status cached_connector_status;
	enum dp_display_states state;

	struct platform_device *pdev;
	struct device_node *aux_switch_node;	/* secdp does not use, dummy */
	struct dentry *root;
	struct completion notification_comp;

	struct dp_hpd     *hpd;
	struct dp_parser  *parser;
	struct dp_power   *power;
	struct dp_catalog *catalog;
	struct dp_aux     *aux;
	struct dp_link    *link;
	struct dp_panel   *panel;
	struct dp_ctrl    *ctrl;
	struct dp_debug   *debug;

	struct dp_panel *active_panels[DP_STREAM_MAX];
	struct dp_hdcp hdcp;

#ifdef CONFIG_SEC_DISPLAYPORT
	struct completion dp_off_comp;
	struct completion dp_disconnect_comp;
	int is_dp_disconnecting;

	struct secdp_sysfs	*sysfs;
	struct secdp_misc	 sec;

	/* for ccic event handler */
#ifdef SECDP_EVENT_THREAD
	struct secdp_event_data dp_event;
	struct task_struct *ev_thread;
	struct workqueue_struct *workq;
	struct list_head attention_head;
#endif
	struct mutex notify_lock;
	struct mutex attention_lock;
	atomic_t notification_status;
	struct notifier_block reboot_nb;
#endif

	struct dp_hpd_cb hpd_cb;
	struct dp_display_mode mode;
	struct dp_display dp_display;
	struct msm_drm_private *priv;

	struct workqueue_struct *wq;
	struct delayed_work hdcp_cb_work;
	struct work_struct connect_work;
	struct work_struct attention_work;
	struct mutex session_lock;
	bool hdcp_delayed_off;

	u32 active_stream_cnt;
	struct dp_mst mst;

	u32 tot_dsc_blks_in_use;

	bool process_hpd_connect;

	struct notifier_block usb_nb;
};

static const struct of_device_id dp_dt_match[] = {
	{.compatible = "qcom,dp-display"},
	{}
};

#ifdef CONFIG_SEC_DISPLAYPORT
struct dp_display_private *g_secdp_priv;

static void secdp_hpd_noti_work(struct work_struct *work);

#ifndef SECDP_USE_WAKELOCK
static void secdp_init_wakelock(struct dp_display_private *dp)
{
	do {} while (0);
}

static void secdp_destroy_wakelock(struct dp_display_private *dp)
{
	do {} while (0);
}

static void secdp_set_wakelock(struct dp_display_private *dp, bool en)
{
	do {} while (0);
}
#else
static void secdp_init_wakelock(struct dp_display_private *dp)
{
	dp->sec.ws = wakeup_source_register(&dp->pdev->dev, "secdp_ws");
}

static void secdp_destroy_wakelock(struct dp_display_private *dp)
{
	wakeup_source_unregister(dp->sec.ws);
}

static void secdp_set_wakelock(struct dp_display_private *dp, bool en)
{
	struct wakeup_source *ws = dp->sec.ws;

	DP_DEBUG("++ en<%d>, active<%d>\n", en, ws->active);

	if (en) {
		__pm_stay_awake(ws);
		DP_DEBUG("set!\n");

	} else {
		__pm_relax(ws);
		DP_DEBUG("clear!\n");
	}

	DP_DEBUG("-- en<%d>, active<%d>\n", en, ws->active);
}
#endif

/** check if it's low power mode or not */
bool secdp_check_if_lpm_mode(void)
{
	bool retval = false;

	if (lpcharge)
		retval = true;

	DP_DEBUG("LPM: %d\n", retval);

	return retval;
}

void secdp_send_poor_connection_event(void)
{
	struct dp_display_private *dp = g_secdp_priv;
#ifndef CONFIG_SWITCH
	struct drm_device *dev = NULL;
	struct drm_connector *connector;
	char *envp[3];
#endif

	DP_INFO("poor connection!\n");

	dp->link->poor_connection = true;
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_secdp_msg, 1);
	switch_set_state(&switch_secdp_msg, 0);
#else
	connector = dp->dp_display.base_connector;
	if (!connector) {
		DP_ERR("connector not set\n");
		return;
	}

	dev = connector->dev;

	envp[0] = "DEVPATH=/devices/virtual/switch/secdp_msg";
	envp[1] = "SWITCH_STATE=1";
	envp[2] = NULL;

	DP_DEBUG("[%s]:[%s]\n", envp[0], envp[1]);

	kobject_uevent_env(&dev->primary->kdev->kobj, KOBJ_CHANGE, envp);
#endif
	dp->sec.dex.prev = dp->sec.dex.curr = DEX_DISABLED;
}

/** check if dp has powered on */
bool secdp_get_power_status(void)
{
	struct dp_display_private *dp;

	if (!g_secdp_priv)
		return false;

	dp = g_secdp_priv;
	return dp_display_state_is(DP_STATE_ENABLED);
}

/** check if dp cable has connected or not */
bool secdp_get_cable_status(void)
{
	struct dp_display_private *dp;

	if (!g_secdp_priv)
		return false;

	dp = g_secdp_priv;
	return dp->sec.cable_connected;
}

/** check if hpd high has come or not */
int secdp_get_hpd_status(void)
{
	struct dp_display_private *dp;

	if (!g_secdp_priv)
		return 0;

	dp = g_secdp_priv;
	return atomic_read(&dp->sec.hpd);
}

bool secdp_get_poor_connection_status(void)
{
	struct dp_display_private *dp;

	if (!g_secdp_priv)
		return false;

	dp = g_secdp_priv;
	return dp->link->poor_connection;
}

bool secdp_get_link_train_status(void)
{
	struct dp_display_private *dp;

	if (!g_secdp_priv)
		return false;

	dp = g_secdp_priv;
	return dp->ctrl->get_link_train_status(dp->ctrl);
}

#define DP_HAL_INIT_TIME	30/*sec*/
/**
 * retval	wait	if booting time has not yet passed over DP_HAL_INIT_TIME
 *			how long to wait [DP_HAL_INIT_TIME - curr_time]
 * retval	0	otherwise
 */
static int secdp_check_boot_time(void)
{
	int wait = 0;
	u64 curr_time;
	unsigned long nsec;

	curr_time = local_clock();
	nsec = do_div(curr_time, 1000000000);

	if ((unsigned long)curr_time < DP_HAL_INIT_TIME)
		wait =  DP_HAL_INIT_TIME - (unsigned long)curr_time;

	DP_INFO("curr_time: %lu[s], wait: %d\n",
		(unsigned long)curr_time, wait);
	return wait;
}

/** check if current connection supports MST or not */
int secdp_is_mst_receiver(void)
{
	struct dp_display_private *dp;

	if (!g_secdp_priv)
		return SECDP_ADT_UNKNOWN;

	dp = g_secdp_priv;

#ifndef CONFIG_SEC_DISPLAYPORT_MST
	return SECDP_ADT_SST;
#else
	return (dp->sec.is_mst_receiver) ? SECDP_ADT_MST : SECDP_ADT_SST;
#endif
}

int secdp_read_branch_revision(struct dp_display_private *dp)
{
	int rlen = 0;
	struct drm_dp_aux *drm_aux;
	char *fw_ver;

	if (!dp || !dp->aux || !dp->aux->drm_aux) {
		DP_ERR("invalid param\n");
		goto end;
	}

	drm_aux = dp->aux->drm_aux;
	fw_ver = dp->sec.dex.fw_ver;

	rlen = drm_dp_dpcd_read(drm_aux, DPCD_BRANCH_HW_REVISION, fw_ver,
			LEN_BRANCH_REVISION);
	if (rlen < 3) {
		DP_ERR("read fail, rlen(%d)\n", rlen);
		goto end;
	}

	DP_INFO("branch revision: HW(0x%X), SW(0x%X, 0x%X)\n",
		fw_ver[0], fw_ver[1], fw_ver[2]);

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	secdp_bigdata_save_item(BD_ADAPTER_HWID, fw_ver[0]);
	secdp_bigdata_save_item(BD_ADAPTER_FWVER, (fw_ver[1] << 8) | fw_ver[2]);
#endif

end:
	return rlen;
}

void secdp_clear_branch_info(struct dp_display_private *dp)
{
	int i;
	char *fw_ver;

	if (!dp)
		goto end;

	fw_ver = dp->sec.dex.fw_ver;
	for (i = 0; i < LEN_BRANCH_REVISION; i++)
		fw_ver[i] = 0;

end:
	return;
}

/**
 * get max dex resolution of current dongle/cable.
 * it's decided by secdp_check_adapter_type() at connection moment.
 */
enum dex_support_res_t secdp_get_dex_res(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	enum dex_support_res_t res = dp->sec.dex.res;

	if (dp->sec.dex.adapter_check_skip)
		res = DEX_RES_MAX;

	return res;
}

/**
 * check if dex is running
 */
bool secdp_check_dex_mode(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	bool mode = false;

	if (secdp_get_dex_res() == DEX_RES_NOT_SUPPORT)
		goto end;

	if (dp->sec.dex.setting_ui == DEX_DISABLED &&
			dp->sec.dex.curr == DEX_DISABLED)
		goto end;

	mode = true;
end:
	return mode;
}

bool secdp_dex_adapter_skip_show(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	bool skip = dp->sec.dex.adapter_check_skip;

	DP_DEBUG("skip: %d\n", skip);

	return skip;
}

void secdp_dex_adapter_skip_store(bool skip)
{
	struct dp_display_private *dp = g_secdp_priv;

	dp->sec.dex.adapter_check_skip = skip;
	DP_INFO("skip: %d\n", dp->sec.dex.adapter_check_skip);
}

/**
 * check connected dongle type with given vid and pid. Based upon this info,
 * we can decide maximum dex resolution for that cable/adapter.
 */
static enum dex_support_res_t secdp_dex_adapter_check(CC_NOTI_TYPEDEF *noti)
{
	struct dp_display_private *dp = g_secdp_priv;
	enum dex_support_res_t type = DEX_RES_DFT; /* default resolution */
	uint ven_id, prod_id;

	ven_id = (uint)(noti->sub2);
	prod_id = (uint)(noti->sub3);
	DP_INFO("ven_id(0x%04x), prod_id(0x%04x)\n", ven_id, prod_id);

#ifdef NOT_SUPPORT_DEX_RES_CHANGE
	return DEX_RES_NOT_SUPPORT;
#endif

	if (dp->parser->dex_dft_res > DEX_RES_NOT_SUPPORT) {
		type = dp->parser->dex_dft_res;
		goto end;
	}

	if (ven_id == SAMSUNG_VENDOR_ID) {
		switch (prod_id) {
		case DEXDOCK_PRODUCT_ID:
		case DEXPAD_PRODUCT_ID:
			type = DEX_RES_MAX;
			break;
		default:
			DP_INFO("it's SS dongle but UNKNOWN\n");
			break;
		}
	} else {
		DP_INFO("it's NOT SS dongle\n");
	}

end:
	DP_INFO("type: %s\n", secdp_dex_res_to_string(type));
	dp->sec.adapter.ven_id = ven_id;
	dp->sec.adapter.prod_id = prod_id;
	return type;
}

enum {
	DEX_HMD_MON = 0,	/* monitor name field */
	DEX_HMD_VID,		/* vid field */
	DEX_HMD_PID,		/* pid field */
	DEX_HMD_FIELD_MAX,
};

/**
 * convert VID/PID string to uint in hexadecimal
 * @tok		[in] 4bytes, char
 * @result	[inout] converted value
 */
static int _secdp_strtoint(char *tok, uint *result)
{
	int ret = 0, len;

	if (!tok || !result) {
		DP_ERR("invalid arg!\n");
		ret = -EINVAL;
		goto end;
	}

	len = strlen(tok);
	if (len == 5 && tok[len - 1] == 0xa/*LF*/) {
		/* continue since it's ended with line feed */
	} else if ((len == 4 && tok[len - 1] == 0xa/*LF*/) || (len != 4)) {
		DP_ERR("wrong! tok:%s, len:%d\n", tok, len);
		ret = -EINVAL;
		goto end;
	}

	ret = kstrtouint(tok, 16, result);
	if (ret) {
		DP_ERR("fail to convert %s! ret:%d\n", tok, ret);
		goto end;
	}
end:
	return ret;
}

#ifdef CONFIG_SEC_DISPLAYPORT_ENG
int secdp_show_hmd_dev(char *buf)
{
	struct dp_display_private *dp = g_secdp_priv;
	struct secdp_sink_dev  *hmd_list;
	int i, rc = 0;

	hmd_list = dp->sec.hmd_list;
	if (!hmd_list) {
		DP_ERR("hmd_list is null!\n");
		rc = -ENOMEM;
		goto end;
	}

	for (i = 0; i < MAX_NUM_HMD; i++) {
		if (strlen(hmd_list[i].monitor_name) > 0) {
			if (buf) {
				rc += scnprintf(buf + rc, PAGE_SIZE - rc,
						"%s,0x%04x,0x%04x\n",
						hmd_list[i].monitor_name,
						hmd_list[i].ven_id,
						hmd_list[i].prod_id);
			}
			/*
			DP_INFO("%s, 0x%04x, 0x%04x\n",
				hmd_list[i].monitor_name,
				hmd_list[i].ven_id,
				hmd_list[i].prod_id);
			*/
		}
	}

end:
	return rc;
}
#endif/*CONFIG_SEC_DISPLAYPORT_ENG*/

int secdp_store_hmd_dev(char *str, size_t len, int num_hmd)
{
	struct dp_display_private *dp = g_secdp_priv;
	struct secdp_sink_dev *hmd_list;
	struct secdp_sink_dev hmd_bak[MAX_NUM_HMD] = {0,};
	bool backup = false;
	char *tok;
	int  i, j, ret = 0, rmdr;
	uint value;

	if (num_hmd <= 0 || num_hmd > MAX_NUM_HMD) {
		DP_ERR("invalid num_hmd! %d\n", num_hmd);
		ret = -EINVAL;
		goto end;
	}

	hmd_list = dp->sec.hmd_list;
	if (!hmd_list) {
		DP_ERR("hmd_list is null!\n");
		ret = -ENOMEM;
		goto end;
	}

	DP_INFO("+++ %s, %lu, %d\n", str, len, num_hmd);

	/* backup and reset */
	for (j = 0; j < MAX_NUM_HMD; j++) {
		memcpy(&hmd_bak[j], &hmd_list[j], sizeof(struct secdp_sink_dev));
		memset(&hmd_list[j], 0, sizeof(struct secdp_sink_dev));
	}
	backup = true;

	tok = strsep(&str, ",");
	i = 0, j = 0;
	while (tok != NULL && *tok != 0xa/*LF*/) {
		if (i > num_hmd * DEX_HMD_FIELD_MAX) {
			DP_ERR("num of tok cannot exceed <%dx%d>!\n",
				num_hmd, DEX_HMD_FIELD_MAX);
			break;
		}
		if (j > MAX_NUM_HMD) {
			DP_ERR("num of HMD cannot exceed %d!\n", MAX_NUM_HMD);
			break;
		}

		rmdr = i % DEX_HMD_FIELD_MAX;

		switch (rmdr) {
		case DEX_HMD_MON:
			strlcpy(hmd_list[j].monitor_name, tok, MON_NAME_LEN);
			break;

		case DEX_HMD_VID:
		case DEX_HMD_PID:
			ret = _secdp_strtoint(tok, &value);
			if (ret)
				goto end;

			if (rmdr == DEX_HMD_VID) {
				hmd_list[j].ven_id  = value;
			} else {
				hmd_list[j].prod_id = value;
				j++;	/* move next */
			}
			break;
		}

		tok = strsep(&str, ",");
		i++;
	}

	for (j = 0; j < MAX_NUM_HMD; j++) {
		if (strlen(hmd_list[j].monitor_name) > 0)
			DP_INFO("%s,0x%04x,0x%04x\n",
				hmd_list[j].monitor_name,
				hmd_list[j].ven_id,
				hmd_list[j].prod_id);
	}

end:
	if (backup && ret) {
		DP_INFO("restore hmd list!\n");
		for (j = 0; j < MAX_NUM_HMD; j++) {
			memcpy(&hmd_list[j], &hmd_bak[j],
				sizeof(struct secdp_sink_dev));
		}
	}
	return ret;
}

/**
 * check if connected sink is HMD device from hmd_list or not
 */
static bool _secdp_check_hmd_dev(struct dp_display_private *dp,
		const struct secdp_sink_dev *hmd)
{
	bool ret = false;

	if (!dp || !hmd) {
		DP_ERR("invalid args!\n");
		goto end;
	}

	if (dp->sec.adapter.ven_id != hmd->ven_id)
		goto end;

	if (dp->sec.adapter.prod_id != hmd->prod_id)
		goto end;

	if (strncmp(dp->panel->monitor_name, hmd->monitor_name,
			strlen(dp->panel->monitor_name)))
		goto end;

	ret = true;
end:
	return ret;
}

/**
 * check if connected sink is predefined HMD(AR/VR) device or not
 * @param	string to search
 *              if NULL, check if one of HMD devices in list are connected
 * @retval	true if found, false otherwise
 */
bool secdp_check_hmd_dev(const char *name_to_search)
{
	struct dp_display_private *dp = g_secdp_priv;
	struct secdp_sink_dev *hmd = dp->sec.hmd_list;
	int  i, list_size;
	bool found = false;

	mutex_lock(&dp->sec.hmd_lock);

	list_size = MAX_NUM_HMD;

	for (i = 0; i < list_size; i++) {
		if (name_to_search != NULL &&
				strncmp(name_to_search, hmd[i].monitor_name,
					strlen(name_to_search)))
			continue;

		found = _secdp_check_hmd_dev(dp, &hmd[i]);
		if (found)
			break;
	}

	if (found)
		DP_INFO("hmd <%s>\n", hmd[i].monitor_name);

	mutex_unlock(&dp->sec.hmd_lock);

	return found;
}
#endif

#ifdef CONFIG_SEC_DISPLAYPORT_ENG
int secdp_debug_prefer_skip_show(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	bool skip;

	skip = dp->sec.debug.prefer_check_skip;
	DP_DEBUG("skip: %d\n", skip);

	return skip;
}

void secdp_debug_prefer_skip_store(bool skip)
{
	struct dp_display_private *dp = g_secdp_priv;

	dp->sec.debug.prefer_check_skip = skip;
	DP_DEBUG("skip: %d\n", dp->sec.debug.prefer_check_skip);
}

int secdp_debug_prefer_ratio_show(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	enum mon_aspect_ratio_t ratio;

	ratio = dp->sec.prefer_ratio;
	DP_DEBUG("ratio: %d\n", ratio);

	return ratio;
}

void secdp_debug_prefer_ratio_store(int ratio)
{
	struct dp_display_private *dp = g_secdp_priv;

	dp->sec.prefer_ratio = ratio;
	DP_DEBUG("ratio: %d\n", dp->sec.prefer_ratio);
}

int secdp_show_link_param(char *buf)
{
	struct dp_display_private *dp = g_secdp_priv;
	int rc = 0;

	rc += scnprintf(buf + rc, PAGE_SIZE - rc,
			"v_level: %u, p_level: %u\nlane_cnt: %u\nbw_code: 0x%x\n",
			dp->link->phy_params.v_level,
			dp->link->phy_params.p_level,
			dp->link->link_params.lane_count,
			dp->link->link_params.bw_code);

	return rc;
}
#endif/*CONFIG_SEC_DISPLAYPORT_ENG*/

static inline bool dp_display_is_hdcp_enabled(struct dp_display_private *dp)
{
	return dp->link->hdcp_status.hdcp_version && dp->hdcp.ops;
}

static irqreturn_t dp_display_irq(int irq, void *dev_id)
{
	struct dp_display_private *dp = dev_id;

	if (!dp) {
		DP_ERR("invalid data\n");
		return IRQ_NONE;
	}

	/* DP HPD isr */
	if (dp->hpd->type ==  DP_HPD_LPHW)
		dp->hpd->isr(dp->hpd);

	/* DP controller isr */
	dp->ctrl->isr(dp->ctrl);

	/* DP aux isr */
	dp->aux->isr(dp->aux);

	/* HDCP isr */
	if (dp_display_is_hdcp_enabled(dp) && dp->hdcp.ops->isr) {
		if (dp->hdcp.ops->isr(dp->hdcp.data))
			DP_ERR("dp_hdcp_isr failed\n");
	}

	return IRQ_HANDLED;
}
static bool dp_display_is_ds_bridge(struct dp_panel *panel)
{
	return (panel->dpcd[DP_DOWNSTREAMPORT_PRESENT] &
		DP_DWN_STRM_PORT_PRESENT);
}

static bool dp_display_is_sink_count_zero(struct dp_display_private *dp)
{
	return dp_display_is_ds_bridge(dp->panel) &&
		(dp->link->sink_count.count == 0);
}

static bool dp_display_is_ready(struct dp_display_private *dp)
{
	return dp->hpd->hpd_high && dp_display_state_is(DP_STATE_CONNECTED) &&
		!dp_display_is_sink_count_zero(dp) &&
		dp->hpd->alt_mode_cfg_done;
}

static void dp_audio_enable(struct dp_display_private *dp, bool enable)
{
	struct dp_panel *dp_panel;
	int idx;

	DP_DEBUG("+++, %d\n", enable);

	for (idx = DP_STREAM_0; idx < DP_STREAM_MAX; idx++) {
		if (!dp->active_panels[idx])
			continue;
		dp_panel = dp->active_panels[idx];

		if (dp_panel->audio_supported) {
			if (enable) {
				dp_panel->audio->bw_code =
					dp->link->link_params.bw_code;
				dp_panel->audio->lane_count =
					dp->link->link_params.lane_count;
				dp_panel->audio->on(dp_panel->audio);
			} else {
				dp_panel->audio->off(dp_panel->audio);
			}
		}
	}
}

static void dp_display_update_hdcp_status(struct dp_display_private *dp,
					bool reset)
{
	if (reset) {
		dp->link->hdcp_status.hdcp_state = HDCP_STATE_INACTIVE;
		dp->link->hdcp_status.hdcp_version = HDCP_VERSION_NONE;
	}

	memset(dp->debug->hdcp_status, 0, sizeof(dp->debug->hdcp_status));

	snprintf(dp->debug->hdcp_status, sizeof(dp->debug->hdcp_status),
		"%s: %s\ncaps: %d\n",
		sde_hdcp_version(dp->link->hdcp_status.hdcp_version),
		sde_hdcp_state_name(dp->link->hdcp_status.hdcp_state),
		dp->hdcp.source_cap);
}

static void dp_display_update_hdcp_info(struct dp_display_private *dp)
{
	void *fd = NULL;
	struct dp_hdcp_dev *dev = NULL;
	struct sde_hdcp_ops *ops = NULL;
	int i = HDCP_VERSION_2P2;

	dp_display_update_hdcp_status(dp, true);

	dp->hdcp.data = NULL;
	dp->hdcp.ops = NULL;

	if (dp->debug->hdcp_disabled || dp->debug->sim_mode)
		return;

	while (i) {
		dev = &dp->hdcp.dev[i];
		ops = dev->ops;
		fd = dev->fd;

		i >>= 1;

		if (!(dp->hdcp.source_cap & dev->ver))
			continue;

		if (ops->sink_support(fd)) {
			dp->hdcp.data = fd;
			dp->hdcp.ops = ops;
			dp->link->hdcp_status.hdcp_version = dev->ver;
			break;
		}
	}

	DP_DEBUG("HDCP version supported: %s\n",
		sde_hdcp_version(dp->link->hdcp_status.hdcp_version));
}

static void dp_display_check_source_hdcp_caps(struct dp_display_private *dp)
{
	int i;
	struct dp_hdcp_dev *hdcp_dev = dp->hdcp.dev;

	if (dp->debug->hdcp_disabled) {
		DP_DEBUG("hdcp disabled\n");
		return;
	}

	DP_DEBUG("+++\n");

	for (i = 0; i < HDCP_VERSION_MAX; i++) {
		struct dp_hdcp_dev *dev = &hdcp_dev[i];
		struct sde_hdcp_ops *ops = dev->ops;
		void *fd = dev->fd;

		if (!fd || !ops)
			continue;

		if (ops->set_mode && ops->set_mode(fd, dp->mst.mst_active))
			continue;

		if (!(dp->hdcp.source_cap & dev->ver) &&
				ops->feature_supported &&
				ops->feature_supported(fd))
			dp->hdcp.source_cap |= dev->ver;
	}

	DP_DEBUG("hdcp.source_cap: 0x%x\n", dp->hdcp.source_cap);

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	secdp_bigdata_save_item(BD_HDCP_VER,
		(dp->hdcp.source_cap & HDCP_VERSION_2P2) ? "hdcp2" :
		((dp->hdcp.source_cap & HDCP_VERSION_1X) ? "hdcp1" : "X"));
#endif

	dp_display_update_hdcp_status(dp, false);
}

static void dp_display_hdcp_register_streams(struct dp_display_private *dp)
{
	int rc;
	size_t i;
	struct sde_hdcp_ops *ops = dp->hdcp.ops;
	void *data = dp->hdcp.data;

	if (dp_display_is_ready(dp) && dp->mst.mst_active && ops &&
			ops->register_streams){
		struct stream_info streams[DP_STREAM_MAX];
		int index = 0;

		DP_DEBUG("Registering all active panel streams with HDCP\n");
		for (i = DP_STREAM_0; i < DP_STREAM_MAX; i++) {
			if (!dp->active_panels[i])
				continue;
			streams[index].stream_id = i;
			streams[index].virtual_channel =
				dp->active_panels[i]->vcpi;
			index++;
		}

		if (index > 0) {
			rc = ops->register_streams(data, index, streams);
			if (rc)
				DP_ERR("failed to register streams. rc = %d\n",
					rc);
		}
	}
}

static void dp_display_hdcp_deregister_stream(struct dp_display_private *dp,
		enum dp_stream_id stream_id)
{
	if (dp->hdcp.ops->deregister_streams) {
		struct stream_info stream = {stream_id,
				dp->active_panels[stream_id]->vcpi};

		DP_DEBUG("Deregistering stream within HDCP library\n");
		dp->hdcp.ops->deregister_streams(dp->hdcp.data, 1, &stream);
	}
}

static void dp_display_abort_hdcp(struct dp_display_private *dp,
		bool abort)
{
	u32 i = HDCP_VERSION_2P2;
	struct dp_hdcp_dev *dev = NULL;

	while (i) {
		dev = &dp->hdcp.dev[i];
		i >>= 1;
		if (!(dp->hdcp.source_cap & dev->ver))
			continue;

		dev->ops->abort(dev->fd, abort);
	}
}

static void dp_display_hdcp_cb_work(struct work_struct *work)
{
	struct dp_display_private *dp;
	struct delayed_work *dw = to_delayed_work(work);
	struct sde_hdcp_ops *ops;
	struct dp_link_hdcp_status *status;
	void *data;
	int rc = 0;
	u32 hdcp_auth_state;
	u8 sink_status = 0;

	DP_DEBUG("+++\n");

	dp = container_of(dw, struct dp_display_private, hdcp_cb_work);

	if (!dp_display_state_is(DP_STATE_ENABLED | DP_STATE_CONNECTED) ||
	     dp_display_state_is(DP_STATE_ABORTED | DP_STATE_HDCP_ABORTED))
		return;

	if (dp_display_state_is(DP_STATE_SUSPENDED)) {
		DP_DEBUG("System suspending. Delay HDCP operations\n");
		queue_delayed_work(dp->wq, &dp->hdcp_cb_work, HZ);
		return;
	}

	if (dp->hdcp_delayed_off) {
		if (dp->hdcp.ops && dp->hdcp.ops->off)
			dp->hdcp.ops->off(dp->hdcp.data);
		dp_display_update_hdcp_status(dp, true);
		dp->hdcp_delayed_off = false;
	}

	if (dp->debug->hdcp_wait_sink_sync) {
		drm_dp_dpcd_readb(dp->aux->drm_aux, DP_SINK_STATUS,
				&sink_status);
		sink_status &= (DP_RECEIVE_PORT_0_STATUS |
				DP_RECEIVE_PORT_1_STATUS);
#ifndef CONFIG_SEC_DISPLAYPORT
		if (sink_status < 1) {
			DP_DEBUG("Sink not synchronized. Queuing again then exiting\n");
			queue_delayed_work(dp->wq, &dp->hdcp_cb_work, HZ);
			return;
		}
#else
		if (sink_status < 1 && !secdp_get_link_train_status()) {
			DP_INFO("hdcp_retry: %d\n", dp->sec.hdcp_retry);
			if (dp->sec.hdcp_retry >= MAX_CNT_HDCP_RETRY) {
				DP_DEBUG("stop queueing!\n");
				schedule_delayed_work(&dp->sec.poor_discon_work,
					msecs_to_jiffies(10));
				return;
			}
			dp->sec.hdcp_retry++;

			DP_DEBUG("Sink not synchronized. Queuing again then exiting\n");
			queue_delayed_work(dp->wq, &dp->hdcp_cb_work, HZ);
			return;
		}
#endif
	}

	status = &dp->link->hdcp_status;

	if (status->hdcp_state == HDCP_STATE_INACTIVE) {
		dp_display_check_source_hdcp_caps(dp);
		dp_display_update_hdcp_info(dp);

		if (dp_display_is_hdcp_enabled(dp)) {
			if (dp->hdcp.ops && dp->hdcp.ops->on &&
					dp->hdcp.ops->on(dp->hdcp.data)) {
				dp_display_update_hdcp_status(dp, true);
				return;
			}
		} else {
			dp_display_update_hdcp_status(dp, true);
			return;
		}
	}

	rc = dp->catalog->ctrl.read_hdcp_status(&dp->catalog->ctrl);
	if (rc >= 0) {
		hdcp_auth_state = (rc >> 20) & 0x3;
		DP_DEBUG("hdcp auth state %d\n", hdcp_auth_state);
	}

	ops = dp->hdcp.ops;
	data = dp->hdcp.data;

	DP_INFO("%s: %s\n", sde_hdcp_version(status->hdcp_version),
		sde_hdcp_state_name(status->hdcp_state));

	dp_display_update_hdcp_status(dp, false);

#ifdef CONFIG_SEC_DISPLAYPORT
	if (secdp_get_reboot_status()) {
		DP_INFO("shutdown\n");
		return;
	}
#endif

	if (status->hdcp_state != HDCP_STATE_AUTHENTICATED &&
		dp->debug->force_encryption && ops && ops->force_encryption)
		ops->force_encryption(data, dp->debug->force_encryption);

	switch (status->hdcp_state) {
	case HDCP_STATE_INACTIVE:
		DP_INFO("start authenticaton\n");
		dp_display_hdcp_register_streams(dp);

#ifdef CONFIG_SEC_DISPLAYPORT
		if (status->hdcp_version < HDCP_VERSION_2P2)
			secdp_reset_link_status(dp->link);
#endif
		if (dp->hdcp.ops && dp->hdcp.ops->authenticate)
			rc = dp->hdcp.ops->authenticate(data);
		if (!rc)
			status->hdcp_state = HDCP_STATE_AUTHENTICATING;
		break;
	case HDCP_STATE_AUTH_FAIL:
#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
		secdp_bigdata_inc_error_cnt(ERR_HDCP_AUTH);
#endif
		if (dp_display_is_ready(dp) &&
		    dp_display_state_is(DP_STATE_ENABLED)) {
			if (ops && ops->on && ops->on(data)) {
				dp_display_update_hdcp_status(dp, true);
				return;
			}
			dp_display_hdcp_register_streams(dp);
			if (ops && ops->reauthenticate) {
				rc = ops->reauthenticate(data);
				if (rc)
					DP_ERR("failed rc=%d\n", rc);
			}
			status->hdcp_state = HDCP_STATE_AUTHENTICATING;
		} else {
			DP_DEBUG("not reauthenticating, cable disconnected\n");
		}
		break;
	default:
		dp_display_hdcp_register_streams(dp);
		break;
	}
}

static void dp_display_notify_hdcp_status_cb(void *ptr,
		enum sde_hdcp_state state)
{
	struct dp_display_private *dp = ptr;

	if (!dp) {
		DP_ERR("invalid input\n");
		return;
	}

	DP_DEBUG("+++\n");

	dp->link->hdcp_status.hdcp_state = state;

	queue_delayed_work(dp->wq, &dp->hdcp_cb_work, HZ/4);
}

static void dp_display_deinitialize_hdcp(struct dp_display_private *dp)
{
	if (!dp) {
		DP_ERR("invalid input\n");
		return;
	}

	sde_dp_hdcp2p2_deinit(dp->hdcp.data);
}

static int dp_display_initialize_hdcp(struct dp_display_private *dp)
{
	struct sde_hdcp_init_data hdcp_init_data;
	struct dp_parser *parser;
	void *fd;
	int rc = 0;

	if (!dp) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	parser = dp->parser;

	hdcp_init_data.client_id     = HDCP_CLIENT_DP;
	hdcp_init_data.drm_aux       = dp->aux->drm_aux;
	hdcp_init_data.cb_data       = (void *)dp;
	hdcp_init_data.workq         = dp->wq;
	hdcp_init_data.sec_access    = true;
	hdcp_init_data.notify_status = dp_display_notify_hdcp_status_cb;
	hdcp_init_data.dp_ahb        = &parser->get_io(parser, "dp_ahb")->io;
	hdcp_init_data.dp_aux        = &parser->get_io(parser, "dp_aux")->io;
	hdcp_init_data.dp_link       = &parser->get_io(parser, "dp_link")->io;
	hdcp_init_data.dp_p0         = &parser->get_io(parser, "dp_p0")->io;
	hdcp_init_data.hdcp_io       = &parser->get_io(parser,
						"hdcp_physical")->io;
	hdcp_init_data.revision      = &dp->panel->link_info.revision;
	hdcp_init_data.msm_hdcp_dev  = dp->parser->msm_hdcp_dev;

	fd = sde_hdcp_1x_init(&hdcp_init_data);
	if (IS_ERR_OR_NULL(fd)) {
		DP_ERR("Error initializing HDCP 1.x\n");
		rc = -EINVAL;
		goto error;
	}

	dp->hdcp.dev[HDCP_VERSION_1X].fd = fd;
	dp->hdcp.dev[HDCP_VERSION_1X].ops = sde_hdcp_1x_get(fd);
	dp->hdcp.dev[HDCP_VERSION_1X].ver = HDCP_VERSION_1X;
	DP_DEBUG("HDCP 1.3 initialized\n");

	fd = sde_dp_hdcp2p2_init(&hdcp_init_data);
	if (IS_ERR_OR_NULL(fd)) {
		DP_ERR("Error initializing HDCP 2.x\n");
		rc = -EINVAL;
		goto error;
	}

	dp->hdcp.dev[HDCP_VERSION_2P2].fd = fd;
	dp->hdcp.dev[HDCP_VERSION_2P2].ops = sde_dp_hdcp2p2_get(fd);
	dp->hdcp.dev[HDCP_VERSION_2P2].ver = HDCP_VERSION_2P2;
	DP_DEBUG("HDCP 2.2 initialized\n");

	return 0;
error:
	dp_display_deinitialize_hdcp(dp);

	return rc;
}

static int dp_display_bind(struct device *dev, struct device *master,
		void *data)
{
	int rc = 0;
	struct dp_display_private *dp;
	struct drm_device *drm;
	struct platform_device *pdev = to_platform_device(dev);

	if (!dev || !pdev || !master) {
		DP_ERR("invalid param(s), dev %pK, pdev %pK, master %pK\n",
				dev, pdev, master);
		rc = -EINVAL;
		goto end;
	}

	drm = dev_get_drvdata(master);
	dp = platform_get_drvdata(pdev);
	if (!drm || !dp) {
		DP_ERR("invalid param(s), drm %pK, dp %pK\n",
				drm, dp);
		rc = -EINVAL;
		goto end;
	}

	dp->dp_display.drm_dev = drm;
	dp->priv = drm->dev_private;
end:
	return rc;
}

static void dp_display_unbind(struct device *dev, struct device *master,
		void *data)
{
	struct dp_display_private *dp;
	struct platform_device *pdev = to_platform_device(dev);

	DP_DEBUG("+++\n");

	if (!dev || !pdev) {
		DP_ERR("invalid param(s)\n");
		return;
	}

	dp = platform_get_drvdata(pdev);
	if (!dp) {
		DP_ERR("Invalid params\n");
		return;
	}

	if (dp->power)
		(void)dp->power->power_client_deinit(dp->power);
	if (dp->aux)
		(void)dp->aux->drm_aux_deregister(dp->aux);
	dp_display_deinitialize_hdcp(dp);
}

static const struct component_ops dp_display_comp_ops = {
	.bind = dp_display_bind,
	.unbind = dp_display_unbind,
};

static void dp_display_send_hpd_event(struct dp_display_private *dp)
{
	struct drm_device *dev = NULL;
	struct drm_connector *connector;
	char name[HPD_STRING_SIZE], status[HPD_STRING_SIZE],
		bpp[HPD_STRING_SIZE], pattern[HPD_STRING_SIZE];
	char *envp[5];

	if (dp->mst.mst_active) {
		DP_DEBUG("skip notification for mst mode\n");
		dp_display_state_remove(DP_STATE_DISCONNECT_NOTIFIED);
		return;
	}

	connector = dp->dp_display.base_connector;

	if (!connector) {
		DP_ERR("connector not set\n");
		return;
	}

	connector->status = connector->funcs->detect(connector, false);
	if (dp->cached_connector_status == connector->status) {
		DP_DEBUG("connector status (%d) unchanged, skipping uevent\n",
				dp->cached_connector_status);
		return;
	}

#ifdef CONFIG_SEC_DISPLAYPORT
	msleep(100);
	atomic_set(&dp->notification_status, 1);
#endif

	dp->cached_connector_status = connector->status;

	dev = connector->dev;

	snprintf(name, HPD_STRING_SIZE, "name=%s", connector->name);
	snprintf(status, HPD_STRING_SIZE, "status=%s",
		drm_get_connector_status_name(connector->status));
	snprintf(bpp, HPD_STRING_SIZE, "bpp=%d",
		dp_link_bit_depth_to_bpp(
		dp->link->test_video.test_bit_depth));
	snprintf(pattern, HPD_STRING_SIZE, "pattern=%d",
		dp->link->test_video.test_video_pattern);

	DP_DEBUG("[%s]:[%s] [%s] [%s]\n", name, status, bpp, pattern);
	envp[0] = name;
	envp[1] = status;
	envp[2] = bpp;
	envp[3] = pattern;
	envp[4] = NULL;
	kobject_uevent_env(&dev->primary->kdev->kobj, KOBJ_CHANGE,
			envp);

	if (connector->status == connector_status_connected) {
		dp_display_state_add(DP_STATE_CONNECT_NOTIFIED);
		dp_display_state_remove(DP_STATE_DISCONNECT_NOTIFIED);
	} else {
		dp_display_state_add(DP_STATE_DISCONNECT_NOTIFIED);
		dp_display_state_remove(DP_STATE_CONNECT_NOTIFIED);
	}
}

static int dp_display_send_hpd_notification(struct dp_display_private *dp)
{
	int ret = 0;
	bool hpd = !!dp_display_state_is(DP_STATE_CONNECTED);

#ifdef CONFIG_SEC_DISPLAYPORT
	if (hpd && !secdp_get_cable_status()) {
		DP_INFO("cable is out\n");
		return -EIO;
	}
	DP_DEBUG("+++\n");

	mutex_lock(&dp->notify_lock);
#endif

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state, hpd);

	/*
	 * Send the notification only if there is any change. This check is
	 * necessary since it is possible that the connect_work may or may not
	 * skip sending the notification in order to respond to a pending
	 * attention message. Attention work thread will always attempt to
	 * send the notification after successfully handling the attention
	 * message. This check here will avoid any unintended duplicate
	 * notifications.
	 */
	if (dp_display_state_is(DP_STATE_CONNECT_NOTIFIED) && hpd) {
		DP_DEBUG("connection notified already, skip notification\n");
		goto skip_wait;
	} else if (dp_display_state_is(DP_STATE_DISCONNECT_NOTIFIED) && !hpd) {
		DP_DEBUG("disonnect notified already, skip notification\n");
		goto skip_wait;
	}

	dp->aux->state |= DP_STATE_NOTIFICATION_SENT;

	if (!dp->mst.mst_active)
		dp->dp_display.is_sst_connected = hpd;
	else
		dp->dp_display.is_sst_connected = false;

	reinit_completion(&dp->notification_comp);
	dp_display_send_hpd_event(dp);

#ifdef CONFIG_SEC_DISPLAYPORT
	if (!hpd && !dp_display_state_is(DP_STATE_ENABLED)) {
		DP_INFO("DP is already off, no wait\n");
		goto skip_wait;
	}
#endif

	if (hpd && dp->mst.mst_active)
		goto skip_wait;

	if (!dp->mst.mst_active &&
			(!!dp_display_state_is(DP_STATE_ENABLED) == hpd))
		goto skip_wait;

	if (!wait_for_completion_timeout(&dp->notification_comp,
#ifndef CONFIG_SEC_DISPLAYPORT
						HZ * 5)) {
#else
						HZ * 20)) {
#endif
		DP_WARN("%s timeout\n", hpd ? "connect" : "disconnect");
		ret = -EINVAL;
	}

	DP_DEBUG("---, ret(%d)\n", ret);
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state, hpd, ret);
#ifdef CONFIG_SEC_DISPLAYPORT
	mutex_unlock(&dp->notify_lock);
#endif
	return ret;
skip_wait:
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state, hpd, ret);
#ifdef CONFIG_SEC_DISPLAYPORT
	mutex_unlock(&dp->notify_lock);
#endif
	return 0;
}

static void dp_display_update_mst_state(struct dp_display_private *dp,
					bool state)
{
	dp->mst.mst_active = state;
	dp->panel->mst_state = state;
}

static void dp_display_process_mst_hpd_high(struct dp_display_private *dp,
						bool mst_probe)
{
	bool is_mst_receiver;
	struct dp_mst_hpd_info info;
	const unsigned long clear_mstm_ctrl_timeout_us = 100000;
	u8 old_mstm_ctrl;
	int ret;

	if (!dp->parser->has_mst || !dp->mst.drm_registered) {
		DP_MST_DEBUG("mst not enabled. has_mst:%d, registered:%d\n",
				dp->parser->has_mst, dp->mst.drm_registered);
		return;
	}

	DP_MST_DEBUG("mst_hpd_high work. mst_probe:%d\n", mst_probe);

	if (!dp->mst.mst_active) {
		is_mst_receiver = dp->panel->read_mst_cap(dp->panel);
#ifdef CONFIG_SEC_DISPLAYPORT
		dp->sec.is_mst_receiver = is_mst_receiver;
		DP_INFO("cnt_mst_hpd:%d\n", dp->sec.cnt_mst_hpd);
		if (secdp_is_mst_receiver() == SECDP_ADT_MST && dp->sec.cnt_mst_hpd == 0) {
			DP_INFO("increment!\n");
			dp->sec.cnt_mst_hpd++;
		}
#endif

		if (!is_mst_receiver) {
			DP_MST_DEBUG("sink doesn't support mst\n");
			return;
		}

		/* clear sink mst state */
		drm_dp_dpcd_readb(dp->aux->drm_aux, DP_MSTM_CTRL,
				&old_mstm_ctrl);
		drm_dp_dpcd_writeb(dp->aux->drm_aux, DP_MSTM_CTRL, 0);

		/* add extra delay if MST state is not cleared */
		if (old_mstm_ctrl) {
			DP_MST_DEBUG("MSTM_CTRL is not cleared, wait %luus\n",
					clear_mstm_ctrl_timeout_us);
			usleep_range(clear_mstm_ctrl_timeout_us,
				clear_mstm_ctrl_timeout_us + 1000);
		}

		ret = drm_dp_dpcd_writeb(dp->aux->drm_aux, DP_MSTM_CTRL,
				 DP_MST_EN | DP_UP_REQ_EN | DP_UPSTREAM_IS_SRC);
		if (ret < 0) {
			DP_ERR("sink mst enablement failed\n");
			return;
		}

		dp_display_update_mst_state(dp, true);
	} else if (dp->mst.mst_active && mst_probe) {
		info.mst_protocol = dp->parser->has_mst_sideband;
		info.mst_port_cnt = dp->debug->mst_port_cnt;
		info.edid = dp->debug->get_edid(dp->debug);

		if (dp->mst.cbs.hpd)
			dp->mst.cbs.hpd(&dp->dp_display, true, &info);
	}

	DP_MST_DEBUG("mst_hpd_high. mst_active:%d\n", dp->mst.mst_active);
}

static void dp_display_host_init(struct dp_display_private *dp)
{
	bool flip = false;
	bool reset;

	if (dp_display_state_is(DP_STATE_INITIALIZED)) {
		dp_display_state_log("[already initialized]");
		return;
	}

	DP_DEBUG("+++\n");

	if (dp->hpd->orientation == ORIENTATION_CC2)
		flip = true;

#ifndef CONFIG_SEC_DISPLAYPORT
	reset = dp->debug->sim_mode ? false :
		(!dp->hpd->multi_func || !dp->hpd->peer_usb_comm);
#else
	reset = dp->debug->sim_mode ? false : !dp->hpd->multi_func;
#endif

	dp->power->init(dp->power, flip);
	dp->hpd->host_init(dp->hpd, &dp->catalog->hpd);
	dp->ctrl->init(dp->ctrl, flip, reset);
	enable_irq(dp->irq);
	dp_display_abort_hdcp(dp, false);

	dp_display_state_add(DP_STATE_INITIALIZED);

	/* log this as it results from user action of cable connection */
	DP_INFO("[OK]\n");
}

static void dp_display_host_ready(struct dp_display_private *dp)
{
	if (!dp_display_state_is(DP_STATE_INITIALIZED)) {
		dp_display_state_show("[not initialized]");
		return;
	}

	if (dp_display_state_is(DP_STATE_READY)) {
		dp_display_state_log("[already ready]");
		return;
	}

	DP_DEBUG("+++\n");

	/*
	 * Reset the aborted state for AUX and CTRL modules. This will
	 * allow these modules to execute normally in response to the
	 * cable connection event.
	 *
	 * One corner case still exists. While the execution flow ensures
	 * that cable disconnection flushes all pending work items on the DP
	 * workqueue, and waits for the user module to clean up the DP
	 * connection session, it is possible that the system delays can
	 * lead to timeouts in the connect path. As a result, the actual
	 * connection callback from user modules can come in late and can
	 * race against a subsequent connection event here which would have
	 * reset the aborted flags. There is no clear solution for this since
	 * the connect/disconnect notifications do not currently have any
	 * sessions IDs.
	 */
	dp->aux->abort(dp->aux, false);
	dp->ctrl->abort(dp->ctrl, false);

	dp->aux->init(dp->aux, dp->parser->aux_cfg);
	dp->panel->init(dp->panel);

	dp_display_state_add(DP_STATE_READY);
	/* log this as it results from user action of cable connection */
	DP_INFO("[OK]\n");
}

static void dp_display_host_unready(struct dp_display_private *dp)
{
	if (!dp_display_state_is(DP_STATE_INITIALIZED)) {
		dp_display_state_show("[not initialized]");
		return;
	}

	if (!dp_display_state_is(DP_STATE_READY)) {
		dp_display_state_show("[not ready]");
		return;
	}

	DP_DEBUG("+++\n");

	dp_display_state_remove(DP_STATE_READY);
	dp->aux->deinit(dp->aux);
	/* log this as it results from user action of cable connection */
	DP_INFO("[OK]\n");
}

static void dp_display_host_deinit(struct dp_display_private *dp)
{
	if (dp->active_stream_cnt) {
		SDE_EVT32_EXTERNAL(dp->state, dp->active_stream_cnt);
		DP_DEBUG("active stream present\n");
		return;
	}

	if (!dp_display_state_is(DP_STATE_INITIALIZED)) {
		dp_display_state_show("[not initialized]");
		return;
	}

	DP_DEBUG("+++\n");

	dp_display_abort_hdcp(dp, true);
	dp->ctrl->deinit(dp->ctrl);
	dp->hpd->host_deinit(dp->hpd, &dp->catalog->hpd);
	dp->power->deinit(dp->power);
	disable_irq(dp->irq);
	dp->aux->state = 0;

	dp_display_state_remove(DP_STATE_INITIALIZED);

	/* log this as it results from user action of cable dis-connection */
	DP_INFO("[OK]\n");
}

static int dp_display_process_hpd_high(struct dp_display_private *dp)
{
	int rc = -EINVAL;
#ifdef CONFIG_SEC_DISPLAYPORT
	int  wait;
	bool is_poor_connection = false;
#endif

	DP_DEBUG("+++\n");

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);
	mutex_lock(&dp->session_lock);

	if (dp_display_state_is(DP_STATE_CONNECTED)) {
		DP_DEBUG("dp already connected, skipping hpd high\n");
		mutex_unlock(&dp->session_lock);
		return -EISCONN;
	}

	dp_display_state_add(DP_STATE_CONNECTED);

	dp->dp_display.max_pclk_khz = min(dp->parser->max_pclk_khz,
					dp->debug->max_pclk_khz);

	/*
	 * If dp video session is not restored from a previous session teardown
	 * by userspace, ensure the host_init is executed, in such a scenario,
	 * so that all the required DP resources are enabled.
	 *
	 * Below is one of the sequences of events which describe the above
	 * scenario:
	 *  a. Source initiated power down resulting in host_deinit.
	 *  b. Sink issues hpd low attention without physical cable disconnect.
	 *  c. Source initiated power up sequence returns early because hpd is
	 *     not high.
	 *  d. Sink issues a hpd high attention event.
	 */
	if (dp_display_state_is(DP_STATE_SRC_PWRDN) &&
			dp_display_state_is(DP_STATE_CONFIGURED)) {
		dp_display_host_init(dp);
		dp_display_state_remove(DP_STATE_SRC_PWRDN);
	}

	dp_display_host_ready(dp);

	dp->link->psm_config(dp->link, &dp->panel->link_info, false);
	dp->debug->psm_enabled = false;

	if (!dp->dp_display.base_connector)
		goto end;

	rc = dp->panel->read_sink_caps(dp->panel,
			dp->dp_display.base_connector, dp->hpd->multi_func);
	/*
	 * ETIMEDOUT --> cable may have been removed
	 * ENOTCONN --> no downstream device connected
	 */
#ifndef CONFIG_SEC_DISPLAYPORT
	if (rc == -ETIMEDOUT || rc == -ENOTCONN) {
		dp_display_state_remove(DP_STATE_CONNECTED);
		goto end;
	}
#else
	if (rc) {
		if (!secdp_get_hpd_status() || !secdp_get_cable_status()
			|| rc == -EIO) {
			DP_INFO("hpd_low or cable_lost or AUX failure\n");
			is_poor_connection = true;
			goto off;
		}

		if (rc == -ENOTCONN) {
			DP_DEBUG("no downstream devices connected.\n");
			rc = -EINVAL;
			goto off;
		}

		DP_INFO("fall through failsafe\n");
		is_poor_connection = true;
		goto notify;
	}
	dp->sec.dex.prev = secdp_check_dex_mode();
	DP_INFO("dex.setting_ui: %d, dex.curr: %d\n",
		dp->sec.dex.setting_ui, dp->sec.dex.curr);
	secdp_read_branch_revision(dp);
	dp->sec.hmd_dev = secdp_check_hmd_dev(NULL);
#endif

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	if (dp->sec.dex.prev)
		secdp_bigdata_save_item(BD_DP_MODE, "DEX");
	else
		secdp_bigdata_save_item(BD_DP_MODE, "MIRROR");
#endif

	dp->link->process_request(dp->link);
	dp->panel->handle_sink_request(dp->panel);

	dp_display_process_mst_hpd_high(dp, false);

#ifdef CONFIG_SEC_DISPLAYPORT
notify:
#endif
	rc = dp->ctrl->on(dp->ctrl, dp->mst.mst_active,
			dp->panel->fec_en, dp->panel->dsc_en, false);
	if (rc) {
		dp_display_state_remove(DP_STATE_CONNECTED);
#ifndef CONFIG_SEC_DISPLAYPORT
		goto end;
#else
		is_poor_connection = true;
		goto off;
#endif
	}

	dp->process_hpd_connect = false;

	dp_display_process_mst_hpd_high(dp, true);

#ifdef CONFIG_SEC_DISPLAYPORT
	secdp_set_wakelock(dp, true);

	if (is_poor_connection)
		secdp_send_poor_connection_event();
#endif

end:
	mutex_unlock(&dp->session_lock);

#ifdef CONFIG_SEC_DISPLAYPORT
	wait = secdp_check_boot_time();
	if (!rc && !dp_display_state_is(DP_STATE_ABORTED) && wait) {
		DP_INFO("deferred HPD noti at boot time! wait: %d\n", wait);
		schedule_delayed_work(&dp->sec.hpd_noti_work,
			msecs_to_jiffies(wait * 1000));
		dp->sec.hpd_noti_deferred = true;
		return rc;
	}
#endif

	/*
	 * Delay the HPD connect notification to see if sink generates any
	 * IRQ HPDs immediately after the HPD high.
	 */
	usleep_range(10000, 10100);

	/*
	 * If an IRQ HPD is pending, then do not send a connect notification.
	 * Once this work returns, the IRQ HPD would be processed and any
	 * required actions (such as link maintenance) would be done which
	 * will subsequently send the HPD notification. To keep things simple,
	 * do this only for SST use-cases. MST use cases require additional
	 * care in order to handle the side-band communications as well.
	 *
	 * One of the main motivations for this is DP LL 1.4 CTS use case
	 * where it is possible that we could get a test request right after
	 * a connection, and the strict timing requriements of the test can
	 * only be met if we do not wait for the e2e connection to be set up.
	 */
	if (!dp->mst.mst_active &&
		(work_busy(&dp->attention_work) == WORK_BUSY_PENDING)) {
		SDE_EVT32_EXTERNAL(dp->state, 99);
		DP_DEBUG("Attention pending, skip HPD notification\n");
		goto skip_notify;
	}

	if (!rc && !dp_display_state_is(DP_STATE_ABORTED))
		dp_display_send_hpd_notification(dp);

skip_notify:
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state, rc);
	return rc;

#ifdef CONFIG_SEC_DISPLAYPORT
off:
	dp_display_state_remove(DP_STATE_CONNECTED);

	dp_display_host_unready(dp);
	dp_display_host_deinit(dp);

	mutex_unlock(&dp->session_lock);

	if (is_poor_connection)
		secdp_send_poor_connection_event();

	return rc;
#endif
}

static void dp_display_process_mst_hpd_low(struct dp_display_private *dp)
{
	struct dp_mst_hpd_info info = {0};

	if (dp->mst.mst_active) {
		DP_MST_DEBUG("mst_hpd_low work\n");

		if (dp->mst.cbs.hpd) {
			info.mst_protocol = dp->parser->has_mst_sideband;
			dp->mst.cbs.hpd(&dp->dp_display, false, &info);
		}
		dp_display_update_mst_state(dp, false);
	}

	DP_MST_DEBUG("mst_hpd_low. mst_active:%d\n", dp->mst.mst_active);
}

static int dp_display_process_hpd_low(struct dp_display_private *dp)
{
	int rc = 0;

	DP_DEBUG("+++\n");

	dp_display_state_remove(DP_STATE_CONNECTED);
	dp->process_hpd_connect = false;
#ifdef CONFIG_SEC_DISPLAYPORT
	cancel_delayed_work(&dp->sec.hpd_noti_work);
	cancel_delayed_work_sync(&dp->sec.hdcp_start_work);
	cancel_delayed_work(&dp->sec.link_status_work);
	cancel_delayed_work(&dp->sec.poor_discon_work);
#endif
	dp_audio_enable(dp, false);
	dp_display_process_mst_hpd_low(dp);

	if ((dp_display_state_is(DP_STATE_CONNECT_NOTIFIED) ||
			dp_display_state_is(DP_STATE_ENABLED)) &&
			!dp->mst.mst_active)
		rc = dp_display_send_hpd_notification(dp);

	mutex_lock(&dp->session_lock);
	if (!dp->active_stream_cnt)
		dp->ctrl->off(dp->ctrl);
	mutex_unlock(&dp->session_lock);

	dp->panel->video_test = false;

#ifdef CONFIG_SEC_DISPLAYPORT
	secdp_set_wakelock(dp, false);
#endif
	return rc;
}

#ifdef CONFIG_SEC_DISPLAYPORT
void secdp_dex_do_reconnecting(void)
{
	struct dp_display_private *dp = g_secdp_priv;

	if (dp->link->poor_connection) {
		DP_INFO("poor connection, return!\n");
		return;
	}

	mutex_lock(&dp->attention_lock);
	DP_INFO("dex_reconnect hpd low++\n");
	dp->sec.dex.reconnecting = true;
	dp->sec.dex.status = DEX_DURING_MODE_CHANGE;

	if (dp->sec.dex.curr == DEX_ENABLED)
		dp->sec.dex.curr = DEX_DURING_MODE_CHANGE;

	dp->hpd->hpd_high = false;
	dp_display_host_init(dp);
	dp_display_process_hpd_low(dp);

	DP_INFO("dex_reconnect hpd low--\n");
	mutex_unlock(&dp->attention_lock);

	msleep(400);

	mutex_lock(&dp->attention_lock);
	if (!dp_display_state_is(DP_STATE_ENABLED) &&
		dp->sec.dex.reconnecting && !dp_display_state_is(DP_STATE_CONNECTED)) {
		DP_INFO("dex_reconnect hpd high++\n");
		if (dp_display_state_is(DP_STATE_INITIALIZED)) {
			/* aux timeout happens whenever DeX reconnect scenario,
			 * init aux here
			 */
			dp_display_host_unready(dp);
			dp_display_host_deinit(dp);
			usleep_range(5000, 6000);
		}

		dp->hpd->hpd_high = true;
		dp_display_host_init(dp);
		dp_display_process_hpd_high(dp);

		DP_INFO("dex_reconnect hpd high--\n");
	}
	dp->sec.dex.reconnecting = false;
	mutex_unlock(&dp->attention_lock);
}
#endif

static int dp_display_usbpd_configure_cb(struct device *dev)
{
	int rc = 0;
	struct dp_display_private *dp;

	if (!dev) {
		DP_ERR("invalid dev\n");
		rc = -EINVAL;
		goto end;
	}

	dp = dev_get_drvdata(dev);
	if (!dp) {
		DP_ERR("no driver data found\n");
		rc = -ENODEV;
		goto end;
	}

	DP_DEBUG("+++\n");

	if (!dp->debug->sim_mode && !dp->parser->no_aux_switch
	    && !dp->parser->gpio_aux_switch) {
		rc = dp->aux->aux_switch(dp->aux, true, dp->hpd->orientation);
		if (rc)
			goto end;
	}

	mutex_lock(&dp->session_lock);

	dp_display_state_remove(DP_STATE_ABORTED);
	dp_display_state_add(DP_STATE_CONFIGURED);

	dp_display_host_init(dp);

	/* check for hpd high */
	if (dp->hpd->hpd_high)
		queue_work(dp->wq, &dp->connect_work);
	else
		dp->process_hpd_connect = true;
	mutex_unlock(&dp->session_lock);
end:
	return rc;
}

static int dp_display_stream_pre_disable(struct dp_display_private *dp,
			struct dp_panel *dp_panel)
{
	dp->ctrl->stream_pre_off(dp->ctrl, dp_panel);

	return 0;
}

static void dp_display_stream_disable(struct dp_display_private *dp,
			struct dp_panel *dp_panel)
{
	if (!dp->active_stream_cnt) {
		DP_ERR("invalid active_stream_cnt (%d)\n",
				dp->active_stream_cnt);
		return;
	}

	DP_DEBUG("stream_id=%d, active_stream_cnt=%d\n",
			dp_panel->stream_id, dp->active_stream_cnt);

	dp->ctrl->stream_off(dp->ctrl, dp_panel);
	dp->active_panels[dp_panel->stream_id] = NULL;
	dp->active_stream_cnt--;
}

static void dp_display_clean(struct dp_display_private *dp)
{
	int idx;
	struct dp_panel *dp_panel;
	struct dp_link_hdcp_status *status = &dp->link->hdcp_status;

	DP_DEBUG("+++\n");
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);

#ifdef CONFIG_SEC_DISPLAYPORT
	cancel_delayed_work(&dp->sec.hpd_noti_work);
	cancel_delayed_work_sync(&dp->sec.hdcp_start_work);
	cancel_delayed_work(&dp->sec.link_status_work);
	cancel_delayed_work(&dp->sec.poor_discon_work);
#endif

	if (dp_display_is_hdcp_enabled(dp) &&
			status->hdcp_state != HDCP_STATE_INACTIVE) {
		cancel_delayed_work_sync(&dp->hdcp_cb_work);
		if (dp->hdcp.ops->off)
			dp->hdcp.ops->off(dp->hdcp.data);

		dp_display_update_hdcp_status(dp, true);
	}

	for (idx = DP_STREAM_0; idx < DP_STREAM_MAX; idx++) {
		if (!dp->active_panels[idx])
			continue;

		dp_panel = dp->active_panels[idx];
		if (dp_panel->audio_supported)
			dp_panel->audio->off(dp_panel->audio);

		dp_display_stream_pre_disable(dp, dp_panel);
		dp_display_stream_disable(dp, dp_panel);
		dp_panel->deinit(dp_panel, 0);
	}

	dp_display_state_remove(DP_STATE_ENABLED | DP_STATE_CONNECTED);

	dp->ctrl->off(dp->ctrl);
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state);
}

static int dp_display_handle_disconnect(struct dp_display_private *dp)
{
	int rc;

	DP_DEBUG("+++\n");

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);
	rc = dp_display_process_hpd_low(dp);
	if (rc) {
		/* cancel any pending request */
		dp->ctrl->abort(dp->ctrl, true);
		dp->aux->abort(dp->aux, true);
	}

	mutex_lock(&dp->session_lock);
	if (rc && dp_display_state_is(DP_STATE_ENABLED))
		dp_display_clean(dp);

	dp_display_host_unready(dp);

	mutex_unlock(&dp->session_lock);

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state);
	return rc;
}

static void dp_display_disconnect_sync(struct dp_display_private *dp)
{
	DP_DEBUG("+++\n");

#ifdef CONFIG_SEC_DISPLAYPORT
	if (dp->link->poor_connection) {
		secdp_send_poor_connection_event();
		dp->link->status_update_cnt = 0;
		dp->sec.hdcp_retry = 0;
	}
#endif

	/* cancel any pending request */
	dp_display_state_add(DP_STATE_ABORTED);

	dp->ctrl->abort(dp->ctrl, true);
	dp->aux->abort(dp->aux, true);

	/* wait for idle state */
	cancel_work_sync(&dp->connect_work);
	cancel_work_sync(&dp->attention_work);
	flush_workqueue(dp->wq);

	dp_display_handle_disconnect(dp);
}

static int dp_display_usbpd_disconnect_cb(struct device *dev)
{
	int rc = 0;
	struct dp_display_private *dp;

	if (!dev) {
		DP_ERR("invalid dev\n");
		rc = -EINVAL;
		goto end;
	}

	dp = dev_get_drvdata(dev);
	if (!dp) {
		DP_ERR("no driver data found\n");
		rc = -ENODEV;
		goto end;
	}

#ifdef CONFIG_SEC_DISPLAYPORT
	DP_DEBUG("+++, psm(%d)\n", dp->debug->psm_enabled);

	if (atomic_read(&dp->notification_status)) {
		reinit_completion(&dp->notification_comp);

		DP_INFO("wait for connection logic++\n");
		if (atomic_read(&dp->notification_status) &&
			!wait_for_completion_timeout(&dp->notification_comp, HZ * 5)) {
			DP_ERR("notification_comp timeout\n");
		}
		DP_INFO("wait for connection logic--\n");
	}
#endif

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state,
			dp->debug->psm_enabled);

	if (dp->debug->psm_enabled && dp_display_state_is(DP_STATE_READY))
		dp->link->psm_config(dp->link, &dp->panel->link_info, true);

	dp_display_disconnect_sync(dp);

	mutex_lock(&dp->session_lock);
	dp_display_host_deinit(dp);
	dp_display_state_remove(DP_STATE_CONFIGURED);
	mutex_unlock(&dp->session_lock);

	if (!dp->debug->sim_mode && !dp->parser->no_aux_switch
	    && !dp->parser->gpio_aux_switch)
		dp->aux->aux_switch(dp->aux, false, ORIENTATION_NONE);

#ifdef CONFIG_SEC_DISPLAYPORT
	/* unset should be here because it's set at above
	 * "dp_display_disconnect_sync()"
	 */
	atomic_set(&dp->notification_status, 0);
	complete(&dp->dp_off_comp);
	DP_DEBUG("---\n");
#endif
end:
	return rc;
}

static int dp_display_stream_enable(struct dp_display_private *dp,
			struct dp_panel *dp_panel)
{
	int rc = 0;

	rc = dp->ctrl->stream_on(dp->ctrl, dp_panel);

	if (dp->debug->tpg_state)
		dp_panel->tpg_config(dp_panel, true);

	if (!rc) {
		dp->active_panels[dp_panel->stream_id] = dp_panel;
		dp->active_stream_cnt++;
	}

	DP_DEBUG("dp active_stream_cnt:%d\n", dp->active_stream_cnt);

	return rc;
}

static void dp_display_mst_attention(struct dp_display_private *dp)
{
	struct dp_mst_hpd_info hpd_irq = {0};

	if (dp->mst.mst_active && dp->mst.cbs.hpd_irq) {
		hpd_irq.mst_hpd_sim = dp->debug->mst_hpd_sim;
		hpd_irq.mst_sim_add_con = dp->debug->mst_sim_add_con;
		hpd_irq.mst_sim_remove_con = dp->debug->mst_sim_remove_con;
		hpd_irq.mst_sim_remove_con_id = dp->debug->mst_sim_remove_con_id;
		hpd_irq.edid = dp->debug->get_edid(dp->debug);
		dp->mst.cbs.hpd_irq(&dp->dp_display, &hpd_irq);
		dp->debug->mst_hpd_sim = false;
		dp->debug->mst_sim_add_con = false;
		dp->debug->mst_sim_remove_con = false;
	}

	DP_MST_DEBUG("mst_attention_work. mst_active:%d\n", dp->mst.mst_active);
}

static void dp_display_attention_work(struct work_struct *work)
{
	struct dp_display_private *dp = container_of(work,
			struct dp_display_private, attention_work);

#ifdef CONFIG_SEC_DISPLAYPORT
	if (!secdp_get_hpd_status() || !secdp_get_cable_status()) {
		DP_INFO("hpd_low or cable_lost\n");
		return;
	}
	DP_DEBUG("+++, sink_request: 0x%08x\n", dp->link->sink_request);
#endif

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);
	mutex_lock(&dp->session_lock);
	SDE_EVT32_EXTERNAL(dp->state);

	if (dp->debug->mst_hpd_sim || !dp_display_state_is(DP_STATE_READY)) {
		mutex_unlock(&dp->session_lock);
		goto mst_attention;
	}

	if (dp->link->process_request(dp->link)) {
		mutex_unlock(&dp->session_lock);
		goto cp_irq;
	}

	mutex_unlock(&dp->session_lock);
	SDE_EVT32_EXTERNAL(dp->state, dp->link->sink_request);

	if (dp->link->sink_request & DS_PORT_STATUS_CHANGED) {
		SDE_EVT32_EXTERNAL(dp->state, DS_PORT_STATUS_CHANGED);
		if (dp_display_is_sink_count_zero(dp)) {
			dp_display_handle_disconnect(dp);
		} else {
			/*
			 * connect work should take care of sending
			 * the HPD notification.
			 */
			if (!dp->mst.mst_active)
				queue_work(dp->wq, &dp->connect_work);
		}
#ifdef CONFIG_SEC_DISPLAYPORT
		/*add some delay to guarantee hpd event handling in framework*/
		msleep(60);
#endif
		goto mst_attention;
	}

	if (dp->link->sink_request & DP_TEST_LINK_VIDEO_PATTERN) {
		SDE_EVT32_EXTERNAL(dp->state, DP_TEST_LINK_VIDEO_PATTERN);
		dp_display_handle_disconnect(dp);

		dp->panel->video_test = true;
		/*
		 * connect work should take care of sending
		 * the HPD notification.
		 */
		queue_work(dp->wq, &dp->connect_work);

		goto mst_attention;
	}

	if (dp->link->sink_request & (DP_TEST_LINK_PHY_TEST_PATTERN |
		DP_TEST_LINK_TRAINING | DP_LINK_STATUS_UPDATED)) {

		mutex_lock(&dp->session_lock);
		dp_audio_enable(dp, false);

		if (dp->link->sink_request & DP_TEST_LINK_PHY_TEST_PATTERN) {
			SDE_EVT32_EXTERNAL(dp->state,
					DP_TEST_LINK_PHY_TEST_PATTERN);
			dp->ctrl->process_phy_test_request(dp->ctrl);
		}

		if (dp->link->sink_request & DP_TEST_LINK_TRAINING) {
			SDE_EVT32_EXTERNAL(dp->state, DP_TEST_LINK_TRAINING);
			dp->link->send_test_response(dp->link);
			dp->ctrl->link_maintenance(dp->ctrl);
		}

		if (dp->link->sink_request & DP_LINK_STATUS_UPDATED) {
			SDE_EVT32_EXTERNAL(dp->state, DP_LINK_STATUS_UPDATED);
			dp->ctrl->link_maintenance(dp->ctrl);
		}

		dp_audio_enable(dp, true);
		mutex_unlock(&dp->session_lock);

		if (dp->link->sink_request & (DP_TEST_LINK_PHY_TEST_PATTERN |
			DP_TEST_LINK_TRAINING))
			goto mst_attention;
	}

cp_irq:
	if (dp_display_is_hdcp_enabled(dp) && dp->hdcp.ops->cp_irq)
		dp->hdcp.ops->cp_irq(dp->hdcp.data);

	if (!dp->mst.mst_active) {
		/*
		 * It is possible that the connect_work skipped sending
		 * the HPD notification if the attention message was
		 * already pending. Send the notification here to
		 * account for that. This is not needed if this
		 * attention work was handling a test request
		 */
		dp_display_send_hpd_notification(dp);
	}

mst_attention:
	dp_display_mst_attention(dp);

#ifdef CONFIG_SEC_DISPLAYPORT
	if (dp->link->status_update_cnt > 9 && !dp->link->poor_connection) {
		dp->link->poor_connection = true;
		dp->sec.dex.status = dp->sec.dex.prev = dp->sec.dex.curr = DEX_DISABLED;
		schedule_delayed_work(&dp->sec.link_status_work,
							msecs_to_jiffies(10));
	}
#endif
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state);
}

static int dp_display_usbpd_attention_cb(struct device *dev)
{
	struct dp_display_private *dp;

	if (!dev) {
		DP_ERR("invalid dev\n");
		return -EINVAL;
	}

	dp = dev_get_drvdata(dev);
	if (!dp) {
		DP_ERR("no driver data found\n");
		return -ENODEV;
	}

	DP_DEBUG("hpd_irq:%d, hpd_high:%d, power_on:%d, is_connected:%d\n",
			dp->hpd->hpd_irq, dp->hpd->hpd_high,
			!!dp_display_state_is(DP_STATE_ENABLED),
			!!dp_display_state_is(DP_STATE_CONNECTED));
	SDE_EVT32_EXTERNAL(dp->state, dp->hpd->hpd_irq, dp->hpd->hpd_high,
			!!dp_display_state_is(DP_STATE_ENABLED),
			!!dp_display_state_is(DP_STATE_CONNECTED));

#ifndef CONFIG_SEC_DISPLAYPORT
	if (!dp->hpd->hpd_high) {
		dp_display_disconnect_sync(dp);
	} else if ((dp->hpd->hpd_irq && dp_display_state_is(DP_STATE_READY)) ||
			dp->debug->mst_hpd_sim) {
		queue_work(dp->wq, &dp->attention_work);
	} else if (dp->process_hpd_connect ||
			 !dp_display_state_is(DP_STATE_CONNECTED)) {
		dp_display_state_remove(DP_STATE_ABORTED);
		queue_work(dp->wq, &dp->connect_work);
	} else {
		DP_DEBUG("ignored\n");
	}
#endif

	return 0;
}

#ifdef CONFIG_SEC_DISPLAYPORT
/** true if it's DP_DISCONNECT event, false otherwise */
static bool secdp_is_disconnect(CC_NOTI_TYPEDEF *noti)
{
	bool ret = false;

	if (noti->id == CCIC_NOTIFY_ID_DP_CONNECT &&
		noti->sub1 == CCIC_NOTIFY_DETACH)
		ret = true;

	return ret;
}

/** true if it's HPD_IRQ event, false otherwise */
static bool secdp_is_hpd_irq(CC_NOTI_TYPEDEF *noti)
{
	bool ret = false;

	if (noti->id == CCIC_NOTIFY_ID_DP_HPD &&
		noti->sub1 == CCIC_NOTIFY_HIGH &&
		noti->sub2 == CCIC_NOTIFY_IRQ)
		ret = true;

	return ret;
}

/** true if it's HPD_LOW event, false otherwise */
static bool secdp_is_hpd_low(CC_NOTI_TYPEDEF *noti)
{
	bool ret = false;

	if (noti->id == CCIC_NOTIFY_ID_DP_HPD &&
		noti->sub1 == CCIC_NOTIFY_LOW)
		ret = true;

	return ret;
}

static void secdp_process_attention(struct dp_display_private *dp,
		CC_NOTI_TYPEDEF *noti)
{
	int rc = 0;

	if (!noti || !dp)
		goto end;

	DP_DEBUG("sub1(%d), sub2(%d), sub3(%d)\n",
		noti->sub1, noti->sub2, noti->sub3);

	dp->sec.dex.reconnecting = false;

	if (secdp_is_disconnect(noti)) {
		dp->hpd->hpd_high = false;
		cancel_delayed_work_sync(&dp->sec.poor_discon_work);
		rc = dp_display_usbpd_disconnect_cb(&dp->pdev->dev);
		goto end;
	}

	if (secdp_is_hpd_irq(noti)) {
		if (secdp_get_reboot_status()) {
			DP_INFO("shutdown\n");
			goto end;
		}
		if (!secdp_get_cable_status()) {
			DP_INFO("cable is out\n");
			goto end;
		}
		if (dp->link->poor_connection) {
			DP_INFO("poor connection\n");
			goto end;
		}

		if (!dp_display_state_is(DP_STATE_ENABLED)) {
			flush_workqueue(dp->wq);

			if (!dp_display_state_is(DP_STATE_ENABLED)) {
				if (secdp_is_mst_receiver() == SECDP_ADT_MST)
					goto attention;

				/* SECDP_ADT_SST */
				if (dp->link->poor_connection) {
					DP_INFO("poor connection\n");
					goto end;
				}

				if (!dp_display_state_is(DP_STATE_CONNECTED)) {
					/* aux timeout happens whenever DeX
					 * reconnect scenario, init aux here
					 */
					dp_display_host_unready(dp);
					dp_display_host_deinit(dp);
					usleep_range(5000, 6000);
				}

				DP_DEBUG("handle it as hpd high\n");
				goto handle_hpd_high;
			}
		}

attention:
		/* irq_hpd: do the same with: dp_display_usbpd_attention_cb */
		queue_work(dp->wq, &dp->attention_work);
		goto end;
	}

	if (secdp_is_hpd_low(noti)) {
		DP_INFO("cnt_mst_hpd:%d\n", dp->sec.cnt_mst_hpd);
		if (secdp_is_mst_receiver() == SECDP_ADT_MST && dp->sec.cnt_mst_hpd > 0) {
			DP_INFO("[MST] Skip 2nd HPD low, Reset cnt_mst_hpd!\n");
			dp->sec.cnt_mst_hpd = -1;
			goto end;
		}

		dp->sec.dex.status = dp->sec.dex.prev = dp->sec.dex.curr = DEX_DISABLED;
		secdp_clear_link_status_update_cnt(dp->link);
		dp_display_disconnect_sync(dp);
		goto end;
	}

handle_hpd_high:
	/* hpd high: do the same with: dp_display_usbpd_attention_cb */
	DP_INFO("connected:%d\n", dp_display_state_is(DP_STATE_CONNECTED));
	if (!dp_display_state_is(DP_STATE_CONNECTED)) {
		secdp_clear_link_status_update_cnt(dp->link);
		dp_display_state_remove(DP_STATE_ABORTED);
		queue_work(dp->wq, &dp->connect_work);
	}
end:
	return;
}

#ifdef SECDP_EVENT_THREAD
static void secdp_handle_attention(struct dp_display_private *dp)
{
	int i = 0;

	while (!list_empty_careful(&dp->attention_head)) {
		struct secdp_attention_node *node;

		DP_DEBUG("+++ processing item %d in the list +++\n", ++i);

		mutex_lock(&dp->attention_lock);
		node = list_first_entry(&dp->attention_head,
				struct secdp_attention_node, list);

		secdp_process_attention(dp, &node->noti);

		/*add some delay to guarantee hpd event handling in framework*/
		msleep(60);

		list_del(&node->list);
		mutex_unlock(&dp->attention_lock);

		kzfree(node);

		DP_DEBUG("--- processing item %d in the list ---\n", i);
	};

}

static int secdp_event_thread(void *data)
{
	unsigned long flag;
	u32 todo = 0;

	struct secdp_event_data *ev_data;
	struct secdp_event *ev;
	struct dp_display_private *dp = NULL;

	if (!data)
		return -EINVAL;

	ev_data = (struct secdp_event_data *)data;
	init_waitqueue_head(&ev_data->event_q);
	spin_lock_init(&ev_data->event_lock);

	while (!kthread_should_stop()) {
		wait_event(ev_data->event_q,
			(ev_data->pndx != ev_data->gndx) ||
			kthread_should_stop());
		spin_lock_irqsave(&ev_data->event_lock, flag);
		ev = &(ev_data->event_list[ev_data->gndx++]);
		todo = ev->id;
		dp = ev->dp;
		ev->id = 0;
		ev_data->gndx %= SECDP_EVENT_Q_MAX;
		spin_unlock_irqrestore(&ev_data->event_lock, flag);

		DP_DEBUG("todo=%s\n", secdp_ev_event_to_string(todo));

		switch (todo) {
		case EV_USBPD_ATTENTION:
			secdp_handle_attention(dp);
			break;
		default:
			DP_ERR("Unknown event:%d\n", todo);
		}
	}

	return 0;
}

static void secdp_send_events(struct dp_display_private *dp, u32 event)
{
	struct secdp_event *ev;
	struct secdp_event_data *ev_data = &dp->dp_event;

	DP_DEBUG("event=%s\n", secdp_ev_event_to_string(event));

	spin_lock(&ev_data->event_lock);
	ev = &ev_data->event_list[ev_data->pndx++];
	ev->id = event;
	ev->dp = dp;
	ev_data->pndx %= SECDP_EVENT_Q_MAX;
	wake_up(&ev_data->event_q);
	spin_unlock(&ev_data->event_lock);
}

static int secdp_event_setup(struct dp_display_private *dp)
{

	dp->ev_thread = kthread_run(secdp_event_thread,
		(void *)&dp->dp_event, "secdp_event");
	if (IS_ERR(dp->ev_thread)) {
		DP_ERR("unable to start event thread\n");
		return PTR_ERR(dp->ev_thread);
	}

	dp->workq = create_workqueue("secdp_hpd");
	if (!dp->workq) {
		DP_ERR("error creating workqueue\n");
		return -EPERM;
	}

	INIT_LIST_HEAD(&dp->attention_head);
	return 0;
}

static void secdp_event_cleanup(struct dp_display_private *dp)
{
	destroy_workqueue(dp->workq);

	if (dp->ev_thread == current)
		return;

	kthread_stop(dp->ev_thread);
}
#endif

#ifdef SECDP_SELF_TEST
static void secdp_hdcp_start_work(struct work_struct *work);

void secdp_self_test_edid_clear(void)
{
	struct dp_display_private *dp = g_secdp_priv;

	dp->panel->set_edid(dp->panel, NULL);
}

void secdp_self_test_hdcp_on(void)
{
	DP_DEBUG("+++\n");

	secdp_hdcp_start_work(NULL);
}

void secdp_self_test_hdcp_off(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	struct dp_link_hdcp_status *status;

	DP_DEBUG("+++\n");

	if (secdp_get_cable_status() && dp_display_state_is(DP_STATE_ENABLED)) {
		status = &dp->link->hdcp_status;

		if (dp_display_is_hdcp_enabled(dp) &&
				status->hdcp_state != HDCP_STATE_INACTIVE) {
			cancel_delayed_work(&dp->hdcp_cb_work);
			usleep_range(3000, 5000);
			if (dp->hdcp.ops->off)
				dp->hdcp.ops->off(dp->hdcp.data);

			dp_display_update_hdcp_status(dp, true);
		}

	}
}
#endif

static void secdp_ccic_connect_init(struct dp_display_private *dp,
		CC_NOTI_TYPEDEF *noti, bool connect)
{
	dp->sec.cable_connected = dp->hpd->alt_mode_cfg_done = connect;
	dp->hpd->orientation    = connect ?
		secdp_get_plug_orientation() : ORIENTATION_NONE;

	dp->hpd->multi_func     = false;
	dp->sec.is_mst_receiver = false;
	dp->sec.link_conf       = false;
	dp->sec.cnt_mst_hpd     = 0;
	dp->sec.hdcp_retry      = 0;

	/* set flags here as soon as disconnected
	 * resource clear will be made later at "secdp_process_attention"
	 */
	dp->sec.dex.res = connect ?
		secdp_dex_adapter_check(noti) : DEX_RES_NOT_SUPPORT;
	dp->sec.dex.prev = dp->sec.dex.curr = dp->sec.dex.status = DEX_DISABLED;
	dp->sec.dex.reconnecting = 0;

	secdp_clear_branch_info(dp);
	secdp_clear_link_status_update_cnt(dp->link);
	secdp_logger_set_max_count(300);

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	if (connect) {
		secdp_bigdata_connection();
		secdp_bigdata_save_item(BD_ORIENTATION,
			(dp->hpd->orientation == ORIENTATION_CC1) ? "CC1" : "CC2");
		secdp_bigdata_save_item(BD_ADT_VID, noti->sub2);
		secdp_bigdata_save_item(BD_ADT_PID, noti->sub3);
	} else {
		secdp_bigdata_disconnection();
	}
#endif
}

static int secdp_ccic_noti_cb(struct notifier_block *nb, unsigned long action,
		void *data)
{
	struct dp_display_private *dp = g_secdp_priv;
	CC_NOTI_TYPEDEF noti = *(CC_NOTI_TYPEDEF *)data;
	int rc = 0;

	if (noti.dest != CCIC_NOTIFY_DEV_DP) {
		/*DP_DEBUG("not DP, skip\n");*/
		goto end;
	}

	switch (noti.id) {
	case CCIC_NOTIFY_ID_ATTACH:
		DP_INFO("CCIC_NOTIFY_ID_ATTACH\n");
		break;

	case CCIC_NOTIFY_ID_DP_CONNECT:
		DP_INFO("CCIC_NOTIFY_ID_DP_CONNECT, <%d>\n", noti.sub1);

		switch (noti.sub1) {
		case CCIC_NOTIFY_ATTACH:
			secdp_ccic_connect_init(dp, &noti, true);

#ifndef SECDP_USB_CONCURRENCY
			/* see dp_display_usbpd_configure_cb() */
			dp_display_host_init(dp);
#endif
#ifdef SECDP_SELF_TEST
			if (secdp_self_test_status(ST_CONNECTION_TEST) >= 0)
				secdp_self_test_start_reconnect(secdp_dex_do_reconnecting);

			secdp_self_register_clear_func(ST_EDID, secdp_self_test_edid_clear);
			if (secdp_self_test_status(ST_EDID) >= 0)
				dp->panel->set_edid(dp->panel, secdp_self_test_get_edid());
#endif
			break;

		case CCIC_NOTIFY_DETACH:
			if (!secdp_get_cable_status()) {
				DP_INFO("already disconnected\n");
				goto end;
			}
			dp->is_dp_disconnecting = 1;
			atomic_set(&dp->sec.hpd, 0);

			secdp_redriver_onoff(false, 0);
			secdp_ccic_connect_init(dp, &noti, false);
			break;

		default:
			break;
		}
		break;

	case CCIC_NOTIFY_ID_DP_LINK_CONF:
		DP_INFO("CCIC_NOTIFY_ID_DP_LINK_CONF, <%c>\n",
			noti.sub1 + 'A' - 1);
		if (!secdp_get_cable_status()) {
			DP_INFO("cable is out\n");
			goto end;
		}

#ifdef SECDP_USB_CONCURRENCY
		if (noti.sub1 == CCIC_NOTIFY_DP_PIN_B ||
			noti.sub1 == CCIC_NOTIFY_DP_PIN_D ||
			noti.sub1 == CCIC_NOTIFY_DP_PIN_F) {
			dp->hpd->multi_func = true;
			secdp_redriver_onoff(true, 2);
		} else {
			dp->hpd->multi_func = false;
			secdp_redriver_onoff(true, 4);
		}

		DP_INFO("multi_func: <%d>\n", dp->hpd->multi_func);
		/* see dp_display_usbpd_configure_cb() */

		/* host_init is commented out to fix phy cts failure. it's
		 * called at dp_display_process_hpd_high function.
		 */
		/*dp_display_host_init(dp);*/
#endif

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
		secdp_bigdata_save_item(BD_LINK_CONFIGURE, noti.sub1 + 'A' - 1);
#endif
		dp->sec.link_conf = true;
		break;

	case CCIC_NOTIFY_ID_DP_HPD:
		DP_INFO("CCIC_NOTIFY_ID_DP_HPD, sub1 <%s>, sub2<%s>\n",
			(noti.sub1 == CCIC_NOTIFY_HIGH) ? "high" :
				((noti.sub1 == CCIC_NOTIFY_LOW) ? "low" : "??"),
			(noti.sub2 == CCIC_NOTIFY_IRQ) ? "irq" : "??");

		if (!secdp_get_cable_status()) {
			DP_INFO("cable is out\n");
			goto end;
		}

		if (noti.sub1 == CCIC_NOTIFY_HIGH) {
			secdp_logger_set_max_count(300);
			atomic_set(&dp->sec.hpd, 1);
			dp->hpd->hpd_high = true;
		} else/* if (noti.sub1 == CCIC_NOTIFY_LOW)*/ {
			atomic_set(&dp->sec.hpd, 0);
			dp->hpd->hpd_high = false;
		}

		break;

	default:
		break;
	}

	DP_DEBUG("sec.link_conf(%d), sec.hpd(%d)\n", dp->sec.link_conf,
		atomic_read(&dp->sec.hpd));
	if ((dp->sec.link_conf && atomic_read(&dp->sec.hpd)) || /*hpd high? or hpd_irq?*/
		secdp_is_hpd_low(&noti) || secdp_is_disconnect(&noti)) {
#ifdef SECDP_EVENT_THREAD
		struct secdp_attention_node *node;

		node = kzalloc(sizeof(*node), GFP_KERNEL);

		node->noti.src  = noti.src;
		node->noti.dest = noti.dest;
		node->noti.id   = noti.id;
		node->noti.sub1 = noti.sub1;
		node->noti.sub2 = noti.sub2;
		node->noti.sub3 = noti.sub3;

		mutex_lock(&dp->attention_lock);
		list_add_tail(&node->list, &dp->attention_head);
		mutex_unlock(&dp->attention_lock);

		secdp_send_events(dp, EV_USBPD_ATTENTION);
#else
		secdp_process_attention(dp, &noti);
#endif

		if (secdp_is_disconnect(&noti)) {
			cancel_work_sync(&dp->connect_work);
			if (dp_display_state_is(DP_STATE_ENABLED) || atomic_read(&dp->notification_status)) {
				int ret;

				DP_DEBUG("wait for detach complete\n");

				init_completion(&dp->dp_off_comp);
				ret = wait_for_completion_timeout(&dp->dp_off_comp,
						msecs_to_jiffies(13500));
				if (ret <= 0) {
					DP_ERR("dp_off_comp timedout\n");
					complete_all(&dp->notification_comp);
					msleep(100);
				} else {
					DP_DEBUG("detach complete!\n");
				}

				atomic_set(&dp->notification_status, 0);
			}
			dp->is_dp_disconnecting = 0;
			complete(&dp->dp_disconnect_comp);
		}
	}

end:
	return rc;
}

int secdp_wait_for_disconnect_complete(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	int ret = 0;

	if (!dp) {
		DP_INFO("dp driver is not initialized completely");
		ret = -1;
		goto end;
	}

	DP_DEBUG("+++\n");

	if (dp->is_dp_disconnecting) {
		init_completion(&dp->dp_disconnect_comp);
		ret = wait_for_completion_timeout(&dp->dp_disconnect_comp,
				msecs_to_jiffies(17000));
		if (ret > 0) {
			DP_DEBUG("dp_disconnect_comp complete!\n");
			ret = 0;
		} else {
			DP_ERR("dp_disconnect_comp timeout\n");
			ret = -1;
		}
	}

	DP_INFO("ret %d\n", ret);
end:
	return ret;
}

int secdp_ccic_noti_register_ex(struct secdp_misc *sec, bool retry)
{
	int rc;

	rc = manager_notifier_register(&sec->ccic_noti_block,
				secdp_ccic_noti_cb, MANAGER_NOTIFY_CCIC_DP);
	if (!rc) {
		sec->ccic_noti_registered = true;
		DP_INFO("success\n");
		goto exit;
	}

	DP_ERR("fail, rc(%d)\n", rc);

	if (!retry) {
		DP_INFO("no more try\n");
		goto exit;
	}

	DP_ERR("manager_dev is not ready, try again in %d[ms]\n",
		CCIC_DP_NOTI_REG_DELAY);
	schedule_delayed_work(&sec->ccic_noti_reg_work,
			msecs_to_jiffies(CCIC_DP_NOTI_REG_DELAY));
exit:
	return rc;
}

static void secdp_ccic_noti_register(struct work_struct *work)
{
	int rc;
	struct dp_display_private *dp = g_secdp_priv;
	struct secdp_misc *sec = &dp->sec;

	mutex_lock(&sec->notifier_lock);

	if (secdp_check_if_lpm_mode()) {
		DP_INFO("it's LPM mode. skip\n");
		goto exit;
	}
	if (sec->ccic_noti_registered) {
		DP_INFO("already registered\n");
		goto exit;
	}

	rc = secdp_ccic_noti_register_ex(sec, true);
	if (rc) {
		DP_ERR("fail, rc(%d)\n", rc);
		goto exit;
	}

	DP_INFO("success\n");
	sec->ccic_noti_registered = true;

	/* cancel immediately */
	rc = cancel_delayed_work(&sec->ccic_noti_reg_work);
	DP_INFO("cancel_work, rc(%d)\n", rc);

	destroy_delayed_work_on_stack(&sec->ccic_noti_reg_work);

exit:
	mutex_unlock(&sec->notifier_lock);
}

int secdp_send_deferred_hpd_noti(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	int rc = 0;

	DP_DEBUG("+++\n");

	cancel_delayed_work_sync(&dp->sec.hpd_noti_work);

	if (dp->sec.hpd_noti_deferred) {
		rc = dp_display_send_hpd_notification(dp);
		dp->sec.hpd_noti_deferred = false;
	}

	return rc;
}

static void secdp_hpd_noti_work(struct work_struct *work)
{
	struct dp_display_private *dp = g_secdp_priv;

	DP_DEBUG("+++\n");

	dp_display_send_hpd_notification(dp);
	dp->sec.hpd_noti_deferred = false;
}

static void secdp_hdcp_start_work(struct work_struct *work)
{
	struct dp_display_private *dp = g_secdp_priv;

	DP_DEBUG("+++\n");

	if (secdp_get_cable_status() && dp_display_state_is(DP_STATE_ENABLED)) {
		cancel_delayed_work_sync(&dp->hdcp_cb_work);
		queue_delayed_work(dp->wq, &dp->hdcp_cb_work, HZ / 4);
	}
}

static void secdp_poor_disconnect_work(struct work_struct *work)
{
	struct dp_display_private *dp = g_secdp_priv;

	DP_DEBUG("+++, poor_connection: %d\n", dp->link->poor_connection);

	if (!dp->link->poor_connection)
		dp->link->poor_connection = true;

	dp_display_disconnect_sync(dp);
}

/**
 * This logic is to check poor DP connection. if link train is failed or
 * HPD_IRQ is coming more than 4th times in 13 sec, regard it as a poor
 * connection and do DP disconnection
 */
static void secdp_link_status_work(struct work_struct *work)
{
	struct dp_display_private *dp = g_secdp_priv;

	DP_INFO("+++ status_update_cnt %d\n", dp->link->status_update_cnt);

	if (dp->link->poor_connection) {
		DP_INFO("poor_connection!\n");
		goto poor_disconnect;
	}

	if (secdp_get_cable_status() && dp_display_state_is(DP_STATE_ENABLED) && dp->sec.dex.curr) {

		if (!secdp_get_link_train_status() ||
				dp->link->status_update_cnt > MAX_CNT_LINK_STATUS_UPDATE) {
			DP_INFO("poor!\n");
			goto poor_disconnect;
		}

		if (!secdp_check_link_stable(dp->link)) {
			DP_INFO("Check poor connection, again\n");
			schedule_delayed_work(&dp->sec.link_status_work,
				msecs_to_jiffies(3000));
		}
	}

	DP_INFO("---\n");
	return;

poor_disconnect:
	schedule_delayed_work(&dp->sec.poor_discon_work, msecs_to_jiffies(10));
}

static int secdp_init(struct dp_display_private *dp)
{
	struct secdp_sink_dev  *hmd_list;
	int rc = -1;

	if (!dp) {
		DP_ERR("error! no dp structure\n");
		goto end;
	}

	if (secdp_check_if_lpm_mode()) {
		DP_INFO("LPM mode! skip!\n");
		rc = 0;
		goto end;
	}

	INIT_DELAYED_WORK(&dp->sec.hpd_noti_work, secdp_hpd_noti_work);
	INIT_DELAYED_WORK(&dp->sec.hdcp_start_work, secdp_hdcp_start_work);
	INIT_DELAYED_WORK(&dp->sec.link_status_work, secdp_link_status_work);
	INIT_DELAYED_WORK(&dp->sec.poor_discon_work, secdp_poor_disconnect_work);

	INIT_DELAYED_WORK(&dp->sec.ccic_noti_reg_work, secdp_ccic_noti_register);
	schedule_delayed_work(&dp->sec.ccic_noti_reg_work,
		msecs_to_jiffies(CCIC_DP_NOTI_REG_DELAY));

	rc = secdp_power_request_gpios(dp->power);
	if (rc)
		DP_ERR("DRM DP gpio request failed\n");

	dp->sysfs = secdp_sysfs_init();
	if (!dp->sysfs)
		DP_ERR("secdp_sysfs_init failed\n");

	mutex_init(&dp->notify_lock);
	mutex_init(&dp->attention_lock);
	mutex_init(&dp->sec.notifier_lock);
	mutex_init(&dp->sec.hmd_lock);

	secdp_init_wakelock(dp);

#ifdef SECDP_EVENT_THREAD
	rc = secdp_event_setup(dp);
	if (rc) {
		DP_ERR("secdp_event_setup failed\n");
		goto end;
	}
#endif

	hmd_list = kzalloc(MAX_NUM_HMD * sizeof(*hmd_list),
				GFP_KERNEL);
	if (ZERO_OR_NULL_PTR(hmd_list)) {
		if (hmd_list)
			kfree(hmd_list);

		dp->sec.hmd_list = NULL;
		rc = -ENOMEM;
		goto end;
	}

	/* add default AR/VR here */
	snprintf(hmd_list[0].monitor_name, MON_NAME_LEN, "PicoVR");
	hmd_list[0].ven_id  = 0x2d40;
	hmd_list[0].prod_id = 0x0000;

	dp->sec.hmd_list = hmd_list;
end:
	DP_INFO("exit, rc(%d)\n", rc);
	return rc;
}

static void secdp_deinit(struct dp_display_private *dp)
{
	if (!dp) {
		DP_ERR("error! no dp structure\n");
		goto end;
	}

	kzfree(dp->sec.hmd_list);
	dp->sec.hmd_list = NULL;

	secdp_destroy_wakelock(dp);

	mutex_destroy(&dp->notify_lock);
	mutex_destroy(&dp->attention_lock);
	mutex_destroy(&dp->sec.notifier_lock);
	mutex_destroy(&dp->sec.hmd_lock);

	secdp_sysfs_deinit(dp->sysfs);
	dp->sysfs = NULL;

#ifdef SECDP_EVENT_THREAD
	secdp_event_cleanup(dp);
#endif

end:
	return;
}

struct dp_panel *secdp_get_panel_info(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	struct dp_panel *panel = NULL;

	if (dp)
		panel = dp->panel;

	return panel;
}

struct drm_connector *secdp_get_connector(void)
{
	struct dp_display *dp_disp = g_dp_display;
	struct drm_connector *connector = NULL;

	if (dp_disp)
		connector = dp_disp->base_connector;

	return connector;
}

static int secdp_reboot_cb(struct notifier_block *nb,
				unsigned long action, void *data)
{
	struct dp_display_private *dp = container_of(nb,
			struct dp_display_private, reboot_nb);

	if (IS_ERR_OR_NULL(dp)) {
		DP_ERR("dp is null!\n");
		goto end;
	}

	if (!secdp_get_cable_status()) {
		DP_DEBUG("cable is out\n");
		goto end;
	}

	DP_DEBUG("+++, reboot:%d\n", dp->sec.reboot);

	dp->sec.reboot = true;

#ifndef SECDP_TEST_HDCP2P2_REAUTH
	msleep(300);
#endif

end:
	return NOTIFY_OK;
}

bool secdp_get_reboot_status(void)
{
	struct dp_display_private *dp;
	bool ret = false;

	if (!g_dp_display) {
		DP_DEBUG("dp display not initialized\n");
		goto end;
	}

	dp = container_of(g_dp_display, struct dp_display_private, dp_display);
	if (IS_ERR_OR_NULL(dp)) {
		DP_ERR("dp is null!\n");
		goto end;
	}

	DP_DEBUG("+++, reboot:%d\n", dp->sec.reboot);

	ret = dp->sec.reboot;

#ifdef SECDP_TEST_HDCP2P2_REAUTH
	ret = false;
	DP_DEBUG("[SECDP_TEST_HDCP2P2_REAUTH]\n");
#endif

end:
	return ret;
}
#endif

static void dp_display_connect_work(struct work_struct *work)
{
	int rc = 0;
	struct dp_display_private *dp = container_of(work,
			struct dp_display_private, connect_work);

	if (dp_display_state_is(DP_STATE_ABORTED)) {
		DP_WARN("HPD off requested\n");
		return;
	}

	if (!dp->hpd->hpd_high) {
		DP_WARN("Sink disconnected\n");
		return;
	}

	DP_DEBUG("+++\n");

#ifdef CONFIG_SEC_DISPLAYPORT
	dp_display_host_init(dp);

	/* fix for PHY CTS v1.2 - 8.1 AUX Manchester - Channel EYE Test failure.
	 * whenever HPD goes high, AUX init makes RC delay and actual AUX
	 * transfer starts even when RC is not yet stabilized. To make RC
	 * waveform to be stable, put some delay here
	 */
	msleep(200);
#endif

	rc = dp_display_process_hpd_high(dp);

	if (!rc && dp->panel->video_test)
		dp->link->send_test_response(dp->link);
}

static int dp_display_usb_notifier(struct notifier_block *nb,
	unsigned long event, void *ptr)
{
	struct extcon_dev *edev = ptr;
	struct dp_display_private *dp = container_of(nb,
			struct dp_display_private, usb_nb);
	if (!edev)
		goto end;

	DP_DEBUG("+++\n");

	if (!event && dp->debug->sim_mode) {
		dp_display_disconnect_sync(dp);
		dp->debug->abort(dp->debug);
	}
end:
	return NOTIFY_DONE;
}

static int dp_display_get_usb_extcon(struct dp_display_private *dp)
{
	struct extcon_dev *edev;
	int rc;

	DP_DEBUG("+++\n");

	edev = extcon_get_edev_by_phandle(&dp->pdev->dev, 0);
	if (IS_ERR(edev))
		return PTR_ERR(edev);

	dp->usb_nb.notifier_call = dp_display_usb_notifier;
	dp->usb_nb.priority = 2;
	rc = extcon_register_notifier(edev, EXTCON_USB, &dp->usb_nb);
	if (rc)
		DP_ERR("failed to register for usb event: %d\n", rc);

	return rc;
}

static void dp_display_deinit_sub_modules(struct dp_display_private *dp)
{
	DP_DEBUG("+++\n");

#ifdef CONFIG_SEC_DISPLAYPORT
	secdp_deinit(dp);
	secdp_sysfs_put(dp->sysfs);
#endif

	dp_audio_put(dp->panel->audio);
	dp_ctrl_put(dp->ctrl);
	dp_link_put(dp->link);
	dp_panel_put(dp->panel);
	dp_aux_put(dp->aux);
	dp_power_put(dp->power);
	dp_catalog_put(dp->catalog);
	dp_parser_put(dp->parser);
	dp_hpd_put(dp->hpd);
	mutex_destroy(&dp->session_lock);
	dp_debug_put(dp->debug);
}

static int dp_init_sub_modules(struct dp_display_private *dp)
{
	int rc = 0;
	bool hdcp_disabled;
	struct device *dev = &dp->pdev->dev;
	struct dp_hpd_cb *cb = &dp->hpd_cb;
	struct dp_ctrl_in ctrl_in = {
		.dev = dev,
	};
	struct dp_panel_in panel_in = {
		.dev = dev,
	};
	struct dp_debug_in debug_in = {
		.dev = dev,
	};

	DP_DEBUG("+++\n");

	mutex_init(&dp->session_lock);

	dp->parser = dp_parser_get(dp->pdev);
	if (IS_ERR(dp->parser)) {
		rc = PTR_ERR(dp->parser);
		DP_ERR("failed to initialize parser, rc = %d\n", rc);
		dp->parser = NULL;
		goto error;
	}

	rc = dp->parser->parse(dp->parser);
	if (rc) {
		DP_ERR("device tree parsing failed\n");
		goto error_catalog;
	}

	g_dp_display->is_mst_supported = dp->parser->has_mst;
	g_dp_display->no_mst_encoder = dp->parser->no_mst_encoder;

	dp->catalog = dp_catalog_get(dev, dp->parser);
	if (IS_ERR(dp->catalog)) {
		rc = PTR_ERR(dp->catalog);
		DP_ERR("failed to initialize catalog, rc = %d\n", rc);
		dp->catalog = NULL;
		goto error_catalog;
	}

	dp->power = dp_power_get(dp->parser);
	if (IS_ERR(dp->power)) {
		rc = PTR_ERR(dp->power);
		DP_ERR("failed to initialize power, rc = %d\n", rc);
		dp->power = NULL;
		goto error_power;
	}

	rc = dp->power->power_client_init(dp->power, &dp->priv->phandle,
		dp->dp_display.drm_dev);
	if (rc) {
		DP_ERR("Power client create failed\n");
		goto error_aux;
	}

	dp->aux = dp_aux_get(dev, &dp->catalog->aux, dp->parser,
			dp->aux_switch_node);
	if (IS_ERR(dp->aux)) {
		rc = PTR_ERR(dp->aux);
		DP_ERR("failed to initialize aux, rc = %d\n", rc);
		dp->aux = NULL;
		goto error_aux;
	}

	rc = dp->aux->drm_aux_register(dp->aux);
	if (rc) {
		DP_ERR("DRM DP AUX register failed\n");
		goto error_link;
	}

	dp->link = dp_link_get(dev, dp->aux);
	if (IS_ERR(dp->link)) {
		rc = PTR_ERR(dp->link);
		DP_ERR("failed to initialize link, rc = %d\n", rc);
		dp->link = NULL;
		goto error_link;
	}

	panel_in.aux = dp->aux;
	panel_in.catalog = &dp->catalog->panel;
	panel_in.link = dp->link;
	panel_in.connector = dp->dp_display.base_connector;
	panel_in.base_panel = NULL;
	panel_in.parser = dp->parser;

	dp->panel = dp_panel_get(&panel_in);
	if (IS_ERR(dp->panel)) {
		rc = PTR_ERR(dp->panel);
		DP_ERR("failed to initialize panel, rc = %d\n", rc);
		dp->panel = NULL;
		goto error_panel;
	}

	ctrl_in.link = dp->link;
	ctrl_in.panel = dp->panel;
	ctrl_in.aux = dp->aux;
	ctrl_in.power = dp->power;
	ctrl_in.catalog = &dp->catalog->ctrl;
	ctrl_in.parser = dp->parser;

	dp->ctrl = dp_ctrl_get(&ctrl_in);
	if (IS_ERR(dp->ctrl)) {
		rc = PTR_ERR(dp->ctrl);
		DP_ERR("failed to initialize ctrl, rc = %d\n", rc);
		dp->ctrl = NULL;
		goto error_ctrl;
	}

	dp->panel->audio = dp_audio_get(dp->pdev, dp->panel,
						&dp->catalog->audio);
	if (IS_ERR(dp->panel->audio)) {
		rc = PTR_ERR(dp->panel->audio);
		DP_ERR("failed to initialize audio, rc = %d\n", rc);
		dp->panel->audio = NULL;
		goto error_audio;
	}

#ifdef CONFIG_SEC_DISPLAYPORT
	dp->sysfs = secdp_sysfs_get(dev, &dp->sec);
	if (IS_ERR(dp->sysfs)) {
		rc = PTR_ERR(dp->sysfs);
		DP_ERR("failed to initialize sysfs, rc = %d\n", rc);
		dp->sysfs = NULL;
		goto error_sysfs;
	}

	rc = secdp_init(dp);
	if (rc)
		DP_ERR("secdp_init failed\n");
#endif

	memset(&dp->mst, 0, sizeof(dp->mst));
	dp->active_stream_cnt = 0;

	cb->configure  = dp_display_usbpd_configure_cb;
	cb->disconnect = dp_display_usbpd_disconnect_cb;
	cb->attention  = dp_display_usbpd_attention_cb;

	dp->hpd = dp_hpd_get(dev, dp->parser, &dp->catalog->hpd, cb);
	if (IS_ERR(dp->hpd)) {
		rc = PTR_ERR(dp->hpd);
		DP_ERR("failed to initialize hpd, rc = %d\n", rc);
		dp->hpd = NULL;
		goto error_hpd;
	}

	hdcp_disabled = !!dp_display_initialize_hdcp(dp);

	debug_in.panel = dp->panel;
	debug_in.hpd = dp->hpd;
	debug_in.link = dp->link;
	debug_in.aux = dp->aux;
	debug_in.connector = &dp->dp_display.base_connector;
	debug_in.catalog = dp->catalog;
	debug_in.parser = dp->parser;
	debug_in.ctrl = dp->ctrl;

	dp->debug = dp_debug_get(&debug_in);
	if (IS_ERR(dp->debug)) {
		rc = PTR_ERR(dp->debug);
		DP_ERR("failed to initialize debug, rc = %d\n", rc);
		dp->debug = NULL;
		goto error_debug;
	}

	dp->cached_connector_status = connector_status_disconnected;
	dp->tot_dsc_blks_in_use = 0;

	dp->debug->hdcp_disabled = hdcp_disabled;
	dp_display_update_hdcp_status(dp, true);

	dp_display_get_usb_extcon(dp);

#ifndef CONFIG_SEC_DISPLAYPORT
	rc = dp->hpd->register_hpd(dp->hpd);
	if (rc) {
		DP_ERR("failed register hpd\n");
		goto error_hpd_reg;
	}
#endif

	return rc;
#ifndef CONFIG_SEC_DISPLAYPORT
error_hpd_reg:
	dp_debug_put(dp->debug);
#endif
error_debug:
	dp_hpd_put(dp->hpd);
error_hpd:
	dp_audio_put(dp->panel->audio);
error_audio:
	dp_ctrl_put(dp->ctrl);
error_ctrl:
	dp_panel_put(dp->panel);
error_panel:
	dp_link_put(dp->link);
#ifdef CONFIG_SEC_DISPLAYPORT
error_sysfs:
	secdp_sysfs_put(dp->sysfs);
#endif
error_link:
	dp_aux_put(dp->aux);
error_aux:
	dp_power_put(dp->power);
error_power:
	dp_catalog_put(dp->catalog);
error_catalog:
	dp_parser_put(dp->parser);
error:
	mutex_destroy(&dp->session_lock);
	return rc;
}

static int dp_display_post_init(struct dp_display *dp_display)
{
	int rc = 0;
	struct dp_display_private *dp;

	if (!dp_display) {
		DP_ERR("invalid input\n");
		rc = -EINVAL;
		goto end;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);
	if (IS_ERR_OR_NULL(dp)) {
		DP_ERR("invalid params\n");
		rc = -EINVAL;
		goto end;
	}

	rc = dp_init_sub_modules(dp);
	if (rc)
		goto end;

	dp_display->post_init = NULL;
end:
	DP_DEBUG("%s\n", rc ? "failed" : "success");
	return rc;
}

static int dp_display_set_mode(struct dp_display *dp_display, void *panel,
		struct dp_display_mode *mode)
{
	const u32 num_components = 3, default_bpp = 24;
	struct dp_display_private *dp;
	struct dp_panel *dp_panel;

	if (!dp_display || !panel) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	dp_panel = panel;
	if (!dp_panel->connector) {
		DP_ERR("invalid connector input\n");
		return -EINVAL;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	mutex_lock(&dp->session_lock);

	if (dp_panel->connector->display_info.max_tmds_clock > 0)
		dp->panel->connector->display_info.max_tmds_clock =
			dp_panel->connector->display_info.max_tmds_clock;

	mode->timing.bpp =
		dp_panel->connector->display_info.bpc * num_components;
	if (!mode->timing.bpp)
		mode->timing.bpp = default_bpp;

	mode->timing.bpp = dp->panel->get_mode_bpp(dp->panel,
			mode->timing.bpp, mode->timing.pixel_clk_khz);

	dp_panel->pinfo = mode->timing;
	mutex_unlock(&dp->session_lock);

	return 0;
}

static int dp_display_prepare(struct dp_display *dp_display, void *panel)
{
	struct dp_display_private *dp;
	struct dp_panel *dp_panel;
	int rc = 0;

	if (!dp_display || !panel) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	dp_panel = panel;
	if (!dp_panel->connector) {
		DP_ERR("invalid connector input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);
	mutex_lock(&dp->session_lock);

	/*
	 * If DP video session is restored by the userspace after display
	 * disconnect notification from dongle i.e. typeC cable connected to
	 * source but disconnected at the display side, the DP controller is
	 * not restored to the desired configured state. So, ensure host_init
	 * is executed in such a scenario so that all the DP controller
	 * resources are enabled for the next connection event.
	 */
	if (dp_display_state_is(DP_STATE_SRC_PWRDN) &&
			dp_display_state_is(DP_STATE_CONFIGURED)) {
		dp_display_host_init(dp);
		dp_display_state_remove(DP_STATE_SRC_PWRDN);
	}

	/*
	 * If the physical connection to the sink is already lost by the time
	 * we try to set up the connection, we can just skip all the steps
	 * here safely.
	 */
	if (dp_display_state_is(DP_STATE_ABORTED)) {
		dp_display_state_log("[aborted]");
		goto end;
	}

	/*
	 * If DP_STATE_ENABLED, there is nothing left to do.
	 * However, this should not happen ideally. So, log this.
	 */
	if (dp_display_state_is(DP_STATE_ENABLED)) {
		dp_display_state_show("[already enabled]");
		goto end;
	}

	if (!dp_display_is_ready(dp)) {
		dp_display_state_show("[not ready]");
		goto end;
	}

	/* For supporting DP_PANEL_SRC_INITIATED_POWER_DOWN case */
	dp_display_host_ready(dp);

	if (dp->debug->psm_enabled) {
		dp->link->psm_config(dp->link, &dp->panel->link_info, false);
		dp->debug->psm_enabled = false;
	}

	/*
	 * Execute the dp controller power on in shallow mode here.
	 * In normal cases, controller should have been powered on
	 * by now. In some cases like suspend/resume or framework
	 * reboot, we end up here without a powered on controller.
	 * Cable may have been removed in suspended state. In that
	 * case, link training is bound to fail on system resume.
	 * So, we execute in shallow mode here to do only minimal
	 * and required things.
	 */
	rc = dp->ctrl->on(dp->ctrl, dp->mst.mst_active, dp_panel->fec_en,
			dp_panel->dsc_en, true);
	if (rc)
		goto end;

end:
	mutex_unlock(&dp->session_lock);

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state);
	return rc;
}

static int dp_display_set_stream_info(struct dp_display *dp_display,
			void *panel, u32 strm_id, u32 start_slot,
			u32 num_slots, u32 pbn, int vcpi)
{
	int rc = 0;
	struct dp_panel *dp_panel;
	struct dp_display_private *dp;
	const int max_slots = 64;

	if (!dp_display) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	if (strm_id >= DP_STREAM_MAX) {
		DP_ERR("invalid stream id:%d\n", strm_id);
		return -EINVAL;
	}

	if (start_slot + num_slots > max_slots) {
		DP_ERR("invalid channel info received. start:%d, slots:%d\n",
				start_slot, num_slots);
		return -EINVAL;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	mutex_lock(&dp->session_lock);

	dp->ctrl->set_mst_channel_info(dp->ctrl, strm_id,
			start_slot, num_slots);

	if (panel) {
		dp_panel = panel;
		dp_panel->set_stream_info(dp_panel, strm_id, start_slot,
				num_slots, pbn, vcpi);
	}

	mutex_unlock(&dp->session_lock);

	return rc;
}

static void dp_display_update_dsc_resources(struct dp_display_private *dp,
		struct dp_panel *panel, bool enable)
{
	u32 dsc_blk_cnt = 0;

	if (panel->pinfo.comp_info.comp_type == MSM_DISPLAY_COMPRESSION_DSC &&
		panel->pinfo.comp_info.comp_ratio) {
		dsc_blk_cnt = panel->pinfo.h_active /
				dp->parser->max_dp_dsc_input_width_pixs;
		if (panel->pinfo.h_active %
				dp->parser->max_dp_dsc_input_width_pixs)
			dsc_blk_cnt++;
	}

	if (enable) {
		dp->tot_dsc_blks_in_use += dsc_blk_cnt;
		panel->tot_dsc_blks_in_use += dsc_blk_cnt;
	} else {
		dp->tot_dsc_blks_in_use -= dsc_blk_cnt;
		panel->tot_dsc_blks_in_use -= dsc_blk_cnt;
	}
}

static int dp_display_enable(struct dp_display *dp_display, void *panel)
{
	int rc = 0;
	struct dp_display_private *dp;

	if (!dp_display || !panel) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);
	mutex_lock(&dp->session_lock);

	/*
	 * If DP_STATE_READY is not set, we should not do any HW
	 * programming.
	 */
	if (!dp_display_state_is(DP_STATE_READY)) {
		dp_display_state_show("[host not ready]");
		goto end;
	}

	/*
	 * It is possible that by the time we get call back to establish
	 * the DP pipeline e2e, the physical DP connection to the sink is
	 * already lost. In such cases, the DP_STATE_ABORTED would be set.
	 * However, it is necessary to NOT abort the display setup here so as
	 * to ensure that the rest of the system is in a stable state prior to
	 * handling the disconnect notification.
	 */
	if (dp_display_state_is(DP_STATE_ABORTED))
		dp_display_state_log("[aborted, but continue on]");

	rc = dp_display_stream_enable(dp, panel);
	if (rc)
		goto end;

	dp_display_update_dsc_resources(dp, panel, true);
	dp_display_state_add(DP_STATE_ENABLED);
end:
	mutex_unlock(&dp->session_lock);
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state);
	return rc;
}

static void dp_display_stream_post_enable(struct dp_display_private *dp,
			struct dp_panel *dp_panel)
{
	dp_panel->spd_config(dp_panel);
	dp_panel->setup_hdr(dp_panel, NULL, false, 0, true);
}

static int dp_display_post_enable(struct dp_display *dp_display, void *panel)
{
	struct dp_display_private *dp;
	struct dp_panel *dp_panel;

	if (!dp_display || !panel) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);
	dp_panel = panel;

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);
	mutex_lock(&dp->session_lock);

	/*
	 * If DP_STATE_READY is not set, we should not do any HW
	 * programming.
	 */
	if (!dp_display_state_is(DP_STATE_ENABLED)) {
		dp_display_state_show("[not enabled]");
		goto end;
	}

#ifndef CONFIG_SEC_DISPLAYPORT
	/*
	 * If the physical connection to the sink is already lost by the time
	 * we try to set up the connection, we can just skip all the steps
	 * here safely.
	 */
	if (dp_display_state_is(DP_STATE_ABORTED)) {
		dp_display_state_log("[aborted]");
		goto end;
	}

	if (!dp_display_is_ready(dp) || !dp_display_state_is(DP_STATE_READY)) {
#else
	if (!dp_display_state_is(DP_STATE_READY)) {
#endif
		dp_display_state_show("[not ready]");
		goto end;
	}

	dp_display_stream_post_enable(dp, dp_panel);

#ifndef CONFIG_SEC_DISPLAYPORT
	cancel_delayed_work_sync(&dp->hdcp_cb_work);
	queue_delayed_work(dp->wq, &dp->hdcp_cb_work, HZ);
#else
#ifdef SECDP_HDCP_DISABLE
	DP_INFO("skip hdcp\n");
#else
	schedule_delayed_work(&dp->sec.hdcp_start_work,
					msecs_to_jiffies(3500));
#endif

#ifdef SECDP_SELF_TEST
	if (secdp_self_test_status(ST_HDCP_TEST) >= 0) {
		cancel_delayed_work_sync(&dp->sec.hdcp_start_work);
		secdp_self_test_start_hdcp_test(secdp_self_test_hdcp_on,
						secdp_self_test_hdcp_off);
	}
#endif

	/* check poor connection only if it's dex mode */
	if (secdp_check_dex_mode())
		schedule_delayed_work(&dp->sec.link_status_work,
						msecs_to_jiffies(13000));
#endif

	if (dp_panel->audio_supported) {
		dp_panel->audio->bw_code = dp->link->link_params.bw_code;
		dp_panel->audio->lane_count = dp->link->link_params.lane_count;
		dp_panel->audio->on(dp_panel->audio);
	}
end:
	dp->aux->state |= DP_STATE_CTRL_POWERED_ON;

#ifdef CONFIG_SEC_DISPLAYPORT
	atomic_set(&dp->notification_status, 0);
#endif
	complete_all(&dp->notification_comp);
	mutex_unlock(&dp->session_lock);
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state);
	return 0;
}

static void dp_display_clear_colorspaces(struct dp_display *dp_display)
{
	struct drm_connector *connector;

	connector = dp_display->base_connector;
	connector->color_enc_fmt = 0;
}

static int dp_display_pre_disable(struct dp_display *dp_display, void *panel)
{
	struct dp_display_private *dp;
	struct dp_panel *dp_panel = panel;
	struct dp_link_hdcp_status *status;
	int rc = 0;
	size_t i;

	if (!dp_display || !panel) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);
	mutex_lock(&dp->session_lock);

	status = &dp->link->hdcp_status;

	if (!dp_display_state_is(DP_STATE_ENABLED)) {
		dp_display_state_show("[not enabled]");
		goto end;
	}

#ifdef CONFIG_SEC_DISPLAYPORT
	if (!dp_display_state_is(DP_STATE_READY)) {
		dp_display_state_show("[not ready]");
		goto end;
	}
#endif

	dp_display_state_add(DP_STATE_HDCP_ABORTED);
	cancel_delayed_work_sync(&dp->hdcp_cb_work);
#ifdef CONFIG_SEC_DISPLAYPORT
	cancel_delayed_work(&dp->sec.hpd_noti_work);
	cancel_delayed_work_sync(&dp->sec.hdcp_start_work);
	cancel_delayed_work(&dp->sec.link_status_work);
	cancel_delayed_work(&dp->sec.poor_discon_work);
#endif

	if (dp_display_is_hdcp_enabled(dp) &&
			status->hdcp_state != HDCP_STATE_INACTIVE) {
		bool off = true;

		if (dp_display_state_is(DP_STATE_SUSPENDED)) {
			DP_DEBUG("Can't perform HDCP cleanup while suspended. Defer\n");
			dp->hdcp_delayed_off = true;
			goto clean;
		}

		flush_delayed_work(&dp->hdcp_cb_work);
		if (dp->mst.mst_active) {
			dp_display_hdcp_deregister_stream(dp,
				dp_panel->stream_id);
			for (i = DP_STREAM_0; i < DP_STREAM_MAX; i++) {
				if (i != dp_panel->stream_id &&
						dp->active_panels[i]) {
					DP_DEBUG("Streams are still active. Skip disabling HDCP\n");
					off = false;
				}
			}
		}

		if (off) {
			if (dp->hdcp.ops->off)
				dp->hdcp.ops->off(dp->hdcp.data);
			dp_display_update_hdcp_status(dp, true);
		}
	}

	dp_display_clear_colorspaces(dp_display);

clean:
	if (dp_panel->audio_supported)
		dp_panel->audio->off(dp_panel->audio);

	rc = dp_display_stream_pre_disable(dp, dp_panel);

end:
	mutex_unlock(&dp->session_lock);
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state);
	return 0;
}

static int dp_display_disable(struct dp_display *dp_display, void *panel)
{
	int i;
	struct dp_display_private *dp = NULL;
	struct dp_panel *dp_panel = NULL;
	struct dp_link_hdcp_status *status;

	if (!dp_display || !panel) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);
	dp_panel = panel;
	status = &dp->link->hdcp_status;

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);
	mutex_lock(&dp->session_lock);

	if (!dp_display_state_is(DP_STATE_ENABLED)) {
		dp_display_state_show("[not enabled]");
		goto end;
	}

	if (!dp_display_state_is(DP_STATE_READY)) {
		dp_display_state_show("[not ready]");
		goto end;
	}

	dp_display_stream_disable(dp, dp_panel);
	dp_display_update_dsc_resources(dp, dp_panel, false);

	dp_display_state_remove(DP_STATE_HDCP_ABORTED);
	for (i = DP_STREAM_0; i < DP_STREAM_MAX; i++) {
		if (dp->active_panels[i]) {
			if (status->hdcp_state != HDCP_STATE_AUTHENTICATED)
				queue_delayed_work(dp->wq, &dp->hdcp_cb_work,
						HZ/4);
			break;
		}
	}
end:
#ifdef CONFIG_SEC_DISPLAYPORT
	atomic_set(&dp->notification_status, 0);
#endif
	mutex_unlock(&dp->session_lock);

	DP_DEBUG("---\n");
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state);
	return 0;
}

static int dp_request_irq(struct dp_display *dp_display)
{
	int rc = 0;
	struct dp_display_private *dp;

	if (!dp_display) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	dp->irq = irq_of_parse_and_map(dp->pdev->dev.of_node, 0);
	if (dp->irq < 0) {
		rc = dp->irq;
		DP_ERR("failed to get irq: %d\n", rc);
		return rc;
	}

	rc = devm_request_irq(&dp->pdev->dev, dp->irq, dp_display_irq,
		IRQF_TRIGGER_HIGH, "dp_display_isr", dp);
	if (rc < 0) {
		DP_ERR("failed to request IRQ%u: %d\n",
				dp->irq, rc);
		return rc;
	}
	disable_irq(dp->irq);

	return 0;
}

static struct dp_debug *dp_get_debug(struct dp_display *dp_display)
{
	struct dp_display_private *dp;

	if (!dp_display) {
		DP_ERR("invalid input\n");
		return ERR_PTR(-EINVAL);
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	return dp->debug;
}

static int dp_display_unprepare(struct dp_display *dp_display, void *panel)
{
	struct dp_display_private *dp;
	struct dp_panel *dp_panel = panel;
	u32 flags = 0;

	if (!dp_display || !panel) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	DP_DEBUG("+++, active_stream_cnt <%d>\n", dp->active_stream_cnt);

	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_ENTRY, dp->state);
	mutex_lock(&dp->session_lock);

	/*
	 * Check if the power off sequence was triggered
	 * by a source initialated action like framework
	 * reboot or suspend-resume but not from normal
	 * hot plug. If connector is in MST mode, skip
	 * powering down host as aux needs to be kept
	 * alive to handle hot-plug sideband message.
	 */
	if (dp_display_is_ready(dp) &&
		(dp_display_state_is(DP_STATE_SUSPENDED) ||
		!dp->mst.mst_active))
		flags |= DP_PANEL_SRC_INITIATED_POWER_DOWN;

	if (dp->active_stream_cnt)
		goto end;

	if (flags & DP_PANEL_SRC_INITIATED_POWER_DOWN) {
		dp->link->psm_config(dp->link, &dp->panel->link_info, true);
		dp->debug->psm_enabled = true;

		dp->ctrl->off(dp->ctrl);
		dp_display_host_unready(dp);
		dp_display_host_deinit(dp);
		dp_display_state_add(DP_STATE_SRC_PWRDN);
	}

	dp_display_state_remove(DP_STATE_ENABLED);
	dp->aux->state = DP_STATE_CTRL_POWERED_OFF;

	complete_all(&dp->notification_comp);

	/* log this as it results from user action of cable dis-connection */
	DP_INFO("[OK]\n");
end:
	dp_panel->deinit(dp_panel, flags);
	mutex_unlock(&dp->session_lock);
	SDE_EVT32_EXTERNAL(SDE_EVTLOG_FUNC_EXIT, dp->state);

	return 0;
}

#ifdef CONFIG_SEC_DISPLAYPORT
void secdp_dex_res_init(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	struct secdp_misc *sec = &dp->sec;

	sec->max_mir_res_idx = -1;
	sec->max_dex_res_idx = -1;
	sec->prefer_res_idx  = -2;

	sec->ignore_ratio    = false;
	sec->prefer_ratio    = MON_RATIO_NA;

	sec->has_prefer      = false;
	sec->ignore_prefer   = false;
	sec->prefer_hdisp    = 0;
	sec->prefer_vdisp    = 0;
	sec->prefer_refresh  = 0;

#ifdef SECDP_IGNORE_PREFER_IF_DEX_RES_EXIST
	sec->dex.res_exist   = false;
#endif
}

static struct secdp_display_timing secdp_supported_resolution[] = {
	{ 0,    640,   480,  60, false, DEX_RES_1920X1080, MON_RATIO_4_3},
	{ 1,    720,   480,  60, false, DEX_RES_1920X1080, MON_RATIO_3_2},
	{ 2,    720,   576,  50, false, DEX_RES_1920X1080, MON_RATIO_5_4},

	{ 3,   1280,   720,  50, false, DEX_RES_1920X1080, MON_RATIO_16_9},
	{ 4,   1280,   720,  60, false, DEX_RES_1920X1080, MON_RATIO_16_9},
	{ 5,   1280,   720, 120, false, DEX_RES_1920X1080, MON_RATIO_16_9},

	{ 6,   1280,   768,  60, false, DEX_RES_1920X1080, MON_RATIO_5_3},	/*CTS 4.4.1.3*/
	{ 7,   1280,   800,  60, false, DEX_RES_1920X1080, MON_RATIO_16_10},	/*CTS 18bpp*/
	{ 8,   1280,  1024,  60, false, DEX_RES_1920X1080, MON_RATIO_5_4},	/*CTS 18bpp*/
	{ 9,   1360,   768,  60, false, DEX_RES_1920X1080, MON_RATIO_16_9},	/*CTS 4.4.1.3*/

	{10,   1366,   768,  60, false, DEX_RES_1920X1080, MON_RATIO_16_9},
	{11,   1600,   900,  60, false, DEX_RES_1920X1080, MON_RATIO_16_9},
	{12,   1600,   900, 120, false, DEX_RES_1920X1080, MON_RATIO_16_9},

	{20,   1920,  1080,  24, false, DEX_RES_1920X1080, MON_RATIO_16_9},
	{21,   1920,  1080,  25, false, DEX_RES_1920X1080, MON_RATIO_16_9},
	{22,   1920,  1080,  30, false, DEX_RES_1920X1080, MON_RATIO_16_9},
	{23,   1920,  1080,  50, false, DEX_RES_1920X1080, MON_RATIO_16_9},
	{24,   1920,  1080,  60, false, DEX_RES_1920X1080, MON_RATIO_16_9},
	{25,   1920,  1080, 120, false, DEX_RES_1920X1080, MON_RATIO_16_9},

	{30,   1920,  1200,  60, false, DEX_RES_1920X1200, MON_RATIO_16_10},
	{31,   1920,  1200, 120, false, DEX_RES_1920X1200, MON_RATIO_16_10},

	{35,   1920,  1440,  60, false, DEX_RES_NOT_SUPPORT},	/*CTS 400.3.3.1*/
	{40,   2048,  1536,  60, false, DEX_RES_NOT_SUPPORT},	/*CTS 18bpp*/
	{45,   2400,  1200,  90, false, DEX_RES_NOT_SUPPORT},	/*relumino*/

#ifdef SECDP_WIDE_21_9_SUPPORT
	{59,   2560,  1080,  60, false, DEX_RES_2560X1080, MON_RATIO_21_9},
	{60,   2560,  1080, 120, false, DEX_RES_2560X1080, MON_RATIO_21_9},
#endif

	{61,   2560,  1440,  60, false, DEX_RES_2560X1440, MON_RATIO_16_9},
	{62,   2560,  1440, 120, false, DEX_RES_2560X1440, MON_RATIO_16_9},
	{65,   2560,  1600,  60, false, DEX_RES_2560X1600, MON_RATIO_16_10},
	{66,   2560,  1600, 120, false, DEX_RES_2560X1600, MON_RATIO_16_10},

	{70,   1440,  2560,  60, false, DEX_RES_NOT_SUPPORT},	/* TVR test */
	{71,   1440,  2560,  75, false, DEX_RES_NOT_SUPPORT},	/* TVR test */

#ifdef SECDP_SUPPORT_ODYSSEY
	{73,   2880,  1600,  60, false, DEX_RES_NOT_SUPPORT},
	{74,   2880,  1600,  90, false, DEX_RES_NOT_SUPPORT},
#endif

#ifdef SECDP_WIDE_21_9_SUPPORT
	{80,   3440,  1440,  50, false, DEX_RES_3440X1440, MON_RATIO_21_9},
	{81,   3440,  1440,  60, false, DEX_RES_3440X1440, MON_RATIO_21_9},
	{83,   3440,  1440, 120, false, DEX_RES_NOT_SUPPORT, MON_RATIO_21_9},

#endif

#ifdef SECDP_WIDE_32_9_SUPPORT
	{100,  3840,  1080,  60, false, DEX_RES_NOT_SUPPORT, MON_RATIO_32_9},
	{102,  3840,  1080, 120, false, DEX_RES_NOT_SUPPORT, MON_RATIO_32_9},
#endif

#ifdef SECDP_WIDE_32_10_SUPPORT
	{110,  3840,  1200,  60, false, DEX_RES_NOT_SUPPORT, MON_RATIO_32_10},
	{112,  3840,  1200, 120, false, DEX_RES_NOT_SUPPORT, MON_RATIO_32_10},
#endif

	{150,  3840,  2160,  24, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
	{151,  3840,  2160,  25, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
	{152,  3840,  2160,  30, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
	{153,  3840,  2160,  50, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
	{154,  3840,  2160,  60, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
	{155,  3840,  2160, 120, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},

	{200,  4096,  2160,  24, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
	{201,  4096,  2160,  25, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
	{202,  4096,  2160,  30, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
	{203,  4096,  2160,  50, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
	{204,  4096,  2160,  60, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},

	{210,  5120,  1440,  60, false, DEX_RES_NOT_SUPPORT, MON_RATIO_16_9},
};

bool secdp_check_dex_reconnect(void)
{
	struct dp_display_private *dp = g_secdp_priv;
	struct secdp_misc *sec = &dp->sec;
	int res_idx;
	bool ret = false;

	DP_INFO("%d, %d, %d\n", sec->max_mir_res_idx, sec->max_dex_res_idx,
		sec->prefer_res_idx);

	if (dp->sec.hmd_dev)
		goto end;

	if (sec->has_prefer)
		res_idx = sec->prefer_res_idx;
	else
		res_idx = sec->max_mir_res_idx;

	if (res_idx == sec->max_dex_res_idx)
		goto end;

	ret = true;
end:
	return ret;
}

#ifdef SECDP_IGNORE_PREFER_IF_DEX_RES_EXIST
/* check if given ratio is one of dex ratios (16:9,16:10,21:9) */
static bool secdp_check_dex_ratio(enum mon_aspect_ratio_t ratio)
{
	bool ret = false;

	switch (ratio) {
	case MON_RATIO_16_9:
	case MON_RATIO_16_10:
	case MON_RATIO_21_9:
		ret = true;
		break;
	default:
		break;
	}

	return ret;
}
#endif

static inline char *secdp_aspect_ratio_to_string(enum mon_aspect_ratio_t ratio)
{
	switch (ratio) {
	case MON_RATIO_3_2:     return DP_ENUM_STR(MON_RATIO_3_2);
	case MON_RATIO_4_3:     return DP_ENUM_STR(MON_RATIO_4_3);
	case MON_RATIO_5_3:     return DP_ENUM_STR(MON_RATIO_5_3);
	case MON_RATIO_5_4:     return DP_ENUM_STR(MON_RATIO_5_4);
	case MON_RATIO_16_9:    return DP_ENUM_STR(MON_RATIO_16_9);
	case MON_RATIO_16_10:   return DP_ENUM_STR(MON_RATIO_16_10);
	case MON_RATIO_21_9:    return DP_ENUM_STR(MON_RATIO_21_9);
	case MON_RATIO_32_9:    return DP_ENUM_STR(MON_RATIO_32_9);
	case MON_RATIO_32_10:   return DP_ENUM_STR(MON_RATIO_32_10);
	case MON_RATIO_NA:      return DP_ENUM_STR(MON_RATIO_NA);
	default:                return "unknown";
	}
}

static enum mon_aspect_ratio_t secdp_get_aspect_ratio(struct drm_display_mode *mode)
{
	enum mon_aspect_ratio_t aspect_ratio = MON_RATIO_NA;
	int hdisplay = mode->hdisplay;
	int vdisplay = mode->vdisplay;

	if ((hdisplay == 4096 && vdisplay == 2160) ||
		(hdisplay == 3840 && vdisplay == 2160) ||
		(hdisplay == 2560 && vdisplay == 1440) ||
		(hdisplay == 1920 && vdisplay == 1080) ||
		(hdisplay == 1600 && vdisplay == 900)  ||
		(hdisplay == 1366 && vdisplay == 768)  ||
		(hdisplay == 1280 && vdisplay == 720))
		aspect_ratio = MON_RATIO_16_9;
	else if ((hdisplay == 2560 && vdisplay == 1600) ||
		(hdisplay == 1920 && vdisplay == 1200) ||
		(hdisplay == 1680 && vdisplay == 1050) ||
		(hdisplay == 1440 && vdisplay == 900)  ||
		(hdisplay == 1280 && vdisplay == 800))
		aspect_ratio = MON_RATIO_16_10;
	else if ((hdisplay == 3840 && vdisplay == 1600) ||
		(hdisplay == 3440 && vdisplay == 1440) ||
		(hdisplay == 2560 && vdisplay == 1080))
		aspect_ratio = MON_RATIO_21_9;
	else if ((hdisplay == 5120 && vdisplay == 1440) ||
		(hdisplay == 3840 && vdisplay == 1080))
		aspect_ratio = MON_RATIO_32_9;
	else if (hdisplay == 3840 && vdisplay == 1200)
		aspect_ratio = MON_RATIO_32_10;
	else if ((hdisplay == 1280 && vdisplay == 1024) ||
		(hdisplay == 720 && vdisplay == 576))
		aspect_ratio = MON_RATIO_5_4;
	else if (hdisplay == 1280 && vdisplay == 768)
		aspect_ratio = MON_RATIO_5_3;
	else if (hdisplay == 640 && vdisplay == 480)
		aspect_ratio = MON_RATIO_4_3;
	else if (hdisplay == 720 && vdisplay == 480)
		aspect_ratio = MON_RATIO_3_2;

	return aspect_ratio;
}

static bool secdp_exceed_mst_max_pclk(struct drm_display_mode *mode)
{
	bool ret = false;

	if (secdp_is_mst_receiver() == SECDP_ADT_SST) {
		/* it's SST. No need to check pclk */
		goto end;
	}

	if (mode->clock <= MST_MAX_PCLK) {
		/* it's MST, and current pclk is less than MST's max pclk */
		goto end;
	}

	/* it's MST, and current pclk is bigger than MST's max pclk */
	ret = true;
end:
	return ret;
}

static bool secdp_check_prefer_resolution(struct dp_display_private *dp,
				struct drm_display_mode *mode)
{
	struct secdp_misc *sec;
	bool ret = false;

	if (!dp || !mode)
		goto end;

	sec = &dp->sec;
	if (!sec || sec->debug.prefer_check_skip)
		goto end;

	if (mode->type & DRM_MODE_TYPE_PREFERRED)
		ret = true;

end:
	return ret;
}

static bool secdp_check_supported_resolution(struct dp_display_private *dp,
				struct drm_display_mode *mode, bool supported)
{
	struct secdp_display_timing *secdp_timing = secdp_supported_resolution;
	struct secdp_misc *sec = NULL;
	int i, fps_diff, mode_refresh_rate, res_cnt;
	bool interlaced = !!(mode->flags & DRM_MODE_FLAG_INTERLACE);
	bool prefer_support, mode_is_prefer, ret = false;

	if (!dp) {
		DP_ERR("no dp resources!\n");
		goto end;
	}

	sec = &dp->sec;
	res_cnt = ARRAY_SIZE(secdp_supported_resolution);
	prefer_support = dp->parser->prefer_support;
	mode_is_prefer = secdp_check_prefer_resolution(dp, mode);

	if (mode_is_prefer) {
		sec->prefer_ratio = secdp_get_aspect_ratio(mode);

		DP_INFO("preferred_timing! %dx%d@%dhz, %s\n",
			mode->hdisplay, mode->vdisplay, mode->vrefresh,
			secdp_aspect_ratio_to_string(sec->prefer_ratio));
		DP_INFO("max_mirror: %d, max_dex: %d, prefer: %d\n",
			sec->max_mir_res_idx, sec->max_dex_res_idx,
			sec->prefer_res_idx);

#ifndef SECDP_IGNORE_PREFER_IF_DEX_RES_EXIST
		if (!prefer_support) {
#else
		if (!prefer_support || (secdp_check_dex_mode() &&
				!secdp_check_dex_ratio(sec->prefer_ratio) &&
				sec->dex.res_exist)) {
#endif
			DP_INFO("remove prefer!\n");
			mode->type &= (~DRM_MODE_TYPE_PREFERRED);
		}
	}

	if (sec->prefer_ratio == MON_RATIO_NA) {
		DP_INFO("prefer timing is absent!\n");

		sec->prefer_ratio = secdp_get_aspect_ratio(mode);
		if (sec->prefer_ratio != MON_RATIO_NA) {
			DP_INFO("get prefer ratio from %dx%d@%dhz, %s\n",
				mode->hdisplay, mode->vdisplay, mode->vrefresh,
				secdp_aspect_ratio_to_string(sec->prefer_ratio));
		} else {
			sec->prefer_ratio = MON_RATIO_16_9;
			DP_INFO("set default prefer ratio\n");
		}
	}

	if (!supported)
		goto end;

	mode_refresh_rate = drm_mode_vrefresh(mode);

	for (i = 0; i < res_cnt; i++) {
		fps_diff = secdp_timing[i].refresh_rate - mode_refresh_rate;
		fps_diff = (fps_diff < 0) ? (fps_diff * (-1)) : fps_diff;

		if (fps_diff > 1)
			continue;

		if (secdp_timing[i].interlaced != interlaced)
			continue;

		if (secdp_timing[i].active_h != mode->hdisplay)
			continue;

		if (secdp_timing[i].active_v != mode->vdisplay)
			continue;

		if (secdp_exceed_mst_max_pclk(mode))
			continue;

		/* sink's max mirror resolution */
		if (sec->max_mir_res_idx < secdp_timing[i].index)
			sec->max_mir_res_idx = secdp_timing[i].index;

		/* sink's max dex resolution */
		ret = false;
		if (secdp_timing[i].dex_res != DEX_RES_NOT_SUPPORT &&
				secdp_timing[i].dex_res <= secdp_get_dex_res()) {
			if (sec->ignore_ratio)
				ret = true;
			else if (secdp_timing[i].mon_ratio == sec->prefer_ratio &&
					(mode_refresh_rate >= DEX_FPS_MIN && mode_refresh_rate <= DEX_FPS_MAX))
				ret = true;
		}

		/* fail safe at dex mode */
		if (!ret && (secdp_timing[i].index == 0/*640x480*/ ||
				secdp_timing[i].index == 1/*720x480*/))
			ret = true;

#ifndef SECDP_IGNORE_PREFER_IF_DEX_RES_EXIST
		if (ret) {
#else
		if (ret && !secdp_check_dex_ratio(sec->prefer_ratio)) {
#endif
			if (sec->max_dex_res_idx < secdp_timing[i].index)
				sec->max_dex_res_idx = secdp_timing[i].index;
		}

		if (prefer_support) {
			/* sink's preferred resolution */
			if (mode_is_prefer) {
				if (!sec->ignore_prefer) {
					sec->prefer_hdisp   = mode->hdisplay;
					sec->prefer_vdisp   = mode->vdisplay;
					sec->prefer_refresh = mode_refresh_rate;

					/* update preferred index */
					sec->prefer_res_idx = secdp_timing[i].index;
					sec->has_prefer     = true;
				} else {
					sec->prefer_hdisp   = 0;
					sec->prefer_vdisp   = 0;
					sec->prefer_refresh = 0;

					/* init preferred index */
					sec->prefer_res_idx = -2;
					sec->has_prefer     = false;

					/* clear preferred flag */
					mode->type &= (~DRM_MODE_TYPE_PREFERRED);
				}
			} else if (sec->prefer_refresh > 0) {
				if (mode->hdisplay == sec->prefer_hdisp &&
					mode->vdisplay == sec->prefer_vdisp &&
					mode_refresh_rate > sec->prefer_refresh) {
					/* found same h/v display but higher
					 * refresh than preferred timing
					 */
					sec->ignore_prefer = true;
				}
			}
		}

		if (dp->sec.hmd_dev || !secdp_check_dex_mode())
			ret = true;

		goto end;
	}

	if (prefer_support) {
		if (!secdp_exceed_mst_max_pclk(mode) && mode_is_prefer) {
			/* if hits here, then "prefer_res_idx" will have -2 */
			sec->has_prefer = true;
			if (dp->sec.hmd_dev || !secdp_check_dex_mode())
				ret = true;
		}
	}

end:
#ifdef SECDP_IGNORE_PREFER_IF_DEX_RES_EXIST
	if (sec && secdp_check_dex_mode() &&
			secdp_check_dex_ratio(secdp_get_aspect_ratio(mode)) &&
			!sec->dex.res_exist) {
		DP_INFO("Dex resolution found! %dx%d@%dhz\n",
			mode->hdisplay, mode->vdisplay, mode->vrefresh);
		sec->dex.res_exist = true;
	}
#endif
	return ret;
}
#endif/*CONFIG_SEC_DISPLAYPORT*/

static enum drm_mode_status dp_display_validate_mode(
		struct dp_display *dp_display,
		void *panel, struct drm_display_mode *mode,
		const struct msm_resource_caps_info *avail_res)
{
	struct dp_display_private *dp;
	struct drm_dp_link *link_info;
	u32 mode_rate_khz = 0, supported_rate_khz = 0, mode_bpp = 0;
	struct dp_panel *dp_panel;
	struct dp_debug *debug;
	enum drm_mode_status mode_status = MODE_BAD;
	bool in_list = false;
	struct dp_mst_connector *mst_connector;
	int hdis, vdis, vref, ar, _hdis, _vdis, _vref, _ar, rate;
	struct dp_display_mode dp_mode;
	bool dsc_en;
	u32 num_lm = 0;
	int rc = 0, tmds_max_clock = 0;

	if (!dp_display || !mode || !panel ||
			!avail_res || !avail_res->max_mixer_width) {
		DP_ERR("invalid params\n");
		return mode_status;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	mutex_lock(&dp->session_lock);

	dp_panel = panel;
	if (!dp_panel->connector) {
		DP_ERR("invalid connector\n");
		goto end;
	}

	link_info = &dp->panel->link_info;

	debug = dp->debug;
	if (!debug)
		goto end;

	dp_display->convert_to_dp_mode(dp_display, panel, mode, &dp_mode);

	dsc_en = dp_mode.timing.comp_info.comp_ratio ? true : false;
	mode_bpp = dsc_en ? dp_mode.timing.comp_info.dsc_info.bpp :
			dp_mode.timing.bpp;

	mode_rate_khz = mode->clock * mode_bpp;
	rate = drm_dp_bw_code_to_link_rate(dp->link->link_params.bw_code);
	supported_rate_khz = link_info->num_lanes * rate * 8;
	tmds_max_clock = dp_panel->connector->display_info.max_tmds_clock;

	if (mode_rate_khz > supported_rate_khz) {
		DP_MST_DEBUG("pclk:%d, supported_rate:%d\n",
				mode_rate_khz, supported_rate_khz);
		goto end;
	}

	if (mode->clock > dp_display->max_pclk_khz) {
		DP_MST_DEBUG("clk:%d, max:%d\n", mode->clock,
				dp_display->max_pclk_khz);
		goto end;
	}

#ifdef CONFIG_SEC_DISPLAYPORT
	if (secdp_is_mst_receiver() == SECDP_ADT_MST)
#endif
	if (tmds_max_clock > 0 && mode->clock > tmds_max_clock) {
		DP_MST_DEBUG("clk:%d, max tmds:%d\n", mode->clock,
				tmds_max_clock);
		goto end;
	}

	rc = msm_get_mixer_count(dp->priv, mode, avail_res, &num_lm);
	if (rc) {
		DP_ERR("error getting mixer count. rc:%d\n", rc);
		goto end;
	}

	if (num_lm > avail_res->num_lm ||
			(num_lm == 2 && !avail_res->num_3dmux)) {
		DP_MST_DEBUG("num_lm:%d, req lm:%d 3dmux:%d\n", num_lm,
				avail_res->num_lm, avail_res->num_3dmux);
		goto end;
	}

	/*
	 * If the connector exists in the mst connector list and if debug is
	 * enabled for that connector, use the mst connector settings from the
	 * list for validation. Otherwise, use non-mst default settings.
	 */
	mutex_lock(&debug->dp_mst_connector_list.lock);

	if (list_empty(&debug->dp_mst_connector_list.list)) {
		mutex_unlock(&debug->dp_mst_connector_list.lock);
		goto verify_default;
	}

	list_for_each_entry(mst_connector, &debug->dp_mst_connector_list.list,
			list) {
		if (mst_connector->con_id == dp_panel->connector->base.id) {
			in_list = true;

			if (!mst_connector->debug_en) {
				mode_status = MODE_OK;
				mutex_unlock(
				&debug->dp_mst_connector_list.lock);
				goto end;
			}

			hdis = mst_connector->hdisplay;
			vdis = mst_connector->vdisplay;
			vref = mst_connector->vrefresh;
			ar = mst_connector->aspect_ratio;

			_hdis = mode->hdisplay;
			_vdis = mode->vdisplay;
			_vref = mode->vrefresh;
			_ar = mode->picture_aspect_ratio;

			if (hdis == _hdis && vdis == _vdis && vref == _vref &&
					ar == _ar) {
				mode_status = MODE_OK;
				mutex_unlock(
				&debug->dp_mst_connector_list.lock);
				goto end;
			}

			break;
		}
	}

	mutex_unlock(&debug->dp_mst_connector_list.lock);

	if (in_list)
		goto end;

verify_default:
	if (debug->debug_en && (mode->hdisplay != debug->hdisplay ||
			mode->vdisplay != debug->vdisplay ||
			mode->vrefresh != debug->vrefresh ||
			mode->picture_aspect_ratio != debug->aspect_ratio))
		goto end;

	mode_status = MODE_OK;
end:
#ifdef CONFIG_SEC_DISPLAYPORT
	if (!secdp_check_supported_resolution(dp, mode, mode_status == MODE_OK))
		mode_status = MODE_BAD;

	DP_INFO("%s@%dhz | %s | max:%d | cur:%d | vtest:%d | bpp:%u\n", mode->name,
		drm_mode_vrefresh(mode), mode_status == MODE_BAD ? "NG" : "OK",
		dp_display->max_pclk_khz, mode->clock, dp_panel->video_test, mode_bpp);
#endif
	mutex_unlock(&dp->session_lock);
	return mode_status;
}

static int dp_display_get_modes(struct dp_display *dp, void *panel,
	struct dp_display_mode *dp_mode)
{
	struct dp_display_private *dp_display;
	struct dp_panel *dp_panel;
	int ret = 0;

	if (!dp || !panel) {
		DP_ERR("invalid params\n");
		return 0;
	}

	dp_panel = panel;
	if (!dp_panel->connector) {
		DP_ERR("invalid connector\n");
		return 0;
	}

	dp_display = container_of(dp, struct dp_display_private, dp_display);

	ret = dp_panel->get_modes(dp_panel, dp_panel->connector, dp_mode);
	if (dp_mode->timing.pixel_clk_khz)
		dp->max_pclk_khz = dp_mode->timing.pixel_clk_khz;

#ifdef CONFIG_SEC_DISPLAYPORT
	DP_INFO("get_modes ret:%d\n", ret);
	if (dp_display->sec.prefer_ratio != MON_RATIO_NA) {
		if (dp_display->sec.max_dex_res_idx < 10/*1600x900*/)
			dp_display->sec.ignore_ratio = 1;
	}
	DP_INFO("max mirror:%d, max dex:%d, ignore_ratio:%d\n",
		dp_display->sec.max_mir_res_idx,
		dp_display->sec.max_dex_res_idx,
		dp_display->sec.ignore_ratio);

	if (dp_panel->connector->hdr_supported && !secdp_panel_hdr_supported()) {
		DP_INFO("hdr_supported(%d) but not DP connection. Clear!\n",
			dp_panel->connector->hdr_supported);
		dp_panel->connector->hdr_supported = false;
	}
#endif
	return ret;
}

static void dp_display_convert_to_dp_mode(struct dp_display *dp_display,
		void *panel,
		const struct drm_display_mode *drm_mode,
		struct dp_display_mode *dp_mode)
{
	struct dp_display_private *dp;
	struct dp_panel *dp_panel;
	u32 free_dsc_blks = 0, required_dsc_blks = 0;

	if (!dp_display || !drm_mode || !dp_mode || !panel) {
		DP_ERR("invalid input\n");
		return;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);
	dp_panel = panel;

	memset(dp_mode, 0, sizeof(*dp_mode));

	free_dsc_blks = dp->parser->max_dp_dsc_blks -
				dp->tot_dsc_blks_in_use +
				dp_panel->tot_dsc_blks_in_use;
	required_dsc_blks = drm_mode->hdisplay /
				dp->parser->max_dp_dsc_input_width_pixs;
	if (drm_mode->hdisplay % dp->parser->max_dp_dsc_input_width_pixs)
		required_dsc_blks++;

	if (free_dsc_blks >= required_dsc_blks)
		dp_mode->capabilities |= DP_PANEL_CAPS_DSC;

#ifndef CONFIG_SEC_DISPLAYPORT
	if (dp_mode->capabilities & DP_PANEL_CAPS_DSC)
		DP_DEBUG("in_use:%d, max:%d, free:%d, req:%d, caps:0x%x, width:%d\n",
			dp->tot_dsc_blks_in_use, dp->parser->max_dp_dsc_blks,
			free_dsc_blks, required_dsc_blks, dp_mode->capabilities,
			dp->parser->max_dp_dsc_input_width_pixs);
#endif

	dp_panel->convert_to_dp_mode(dp_panel, drm_mode, dp_mode);
}

static int dp_display_config_hdr(struct dp_display *dp_display, void *panel,
			struct drm_msm_ext_hdr_metadata *hdr, bool dhdr_update)
{
	struct dp_panel *dp_panel;
	struct sde_connector *sde_conn;
	struct dp_display_private *dp;
	u64 core_clk_rate;
	bool flush_hdr;

	if (!dp_display || !panel) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	dp_panel = panel;
	dp = container_of(dp_display, struct dp_display_private, dp_display);
	sde_conn =  to_sde_connector(dp_panel->connector);

	core_clk_rate = dp->power->clk_get_rate(dp->power, "core_clk");
	if (!core_clk_rate) {
		DP_ERR("invalid rate for core_clk\n");
		return -EINVAL;
	}

	if (!dp_display_state_is(DP_STATE_ENABLED)) {
		dp_display_state_show("[not enabled]");
		return 0;
	}

	/*
	 * In rare cases where HDR metadata is updated independently
	 * flush the HDR metadata immediately instead of relying on
	 * the colorspace
	 */
	flush_hdr = !sde_conn->colorspace_updated;

	if (flush_hdr)
		DP_DEBUG("flushing the HDR metadata\n");
	else
		DP_DEBUG("piggy-backing with colorspace\n");

	return dp_panel->setup_hdr(dp_panel, hdr, dhdr_update,
		core_clk_rate, flush_hdr);
}

static int dp_display_setup_colospace(struct dp_display *dp_display,
		void *panel,
		u32 colorspace)
{
	struct dp_panel *dp_panel;
	struct dp_display_private *dp;

	if (!dp_display || !panel) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	if (!dp_display_state_is(DP_STATE_ENABLED)) {
		dp_display_state_show("[not enabled]");
		return 0;
	}

	dp_panel = panel;

	return dp_panel->set_colorspace(dp_panel, colorspace);
}

static int dp_display_create_workqueue(struct dp_display_private *dp)
{
	dp->wq = create_singlethread_workqueue("drm_dp");
	if (IS_ERR_OR_NULL(dp->wq)) {
		DP_ERR("Error creating wq\n");
		return -EPERM;
	}

	INIT_DELAYED_WORK(&dp->hdcp_cb_work, dp_display_hdcp_cb_work);
	INIT_WORK(&dp->connect_work, dp_display_connect_work);
	INIT_WORK(&dp->attention_work, dp_display_attention_work);

	return 0;
}

/* aux switch error is coming repeatedly during boot time when samsung
 * displayport feature is disabled. so, disabled following codes
 */
#if 0/*CONFIG_SEC_DISPLAYPORT*/
static int dp_display_fsa4480_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	return 0;
}

static int dp_display_init_aux_switch(struct dp_display_private *dp)
{
	int rc = 0;
	const char *phandle = "qcom,dp-aux-switch";
	struct notifier_block nb;

	if (!dp->pdev->dev.of_node) {
		DP_ERR("cannot find dev.of_node\n");
		rc = -ENODEV;
		goto end;
	}

	dp->aux_switch_node = of_parse_phandle(dp->pdev->dev.of_node,
			phandle, 0);
	if (!dp->aux_switch_node) {
		DP_WARN("cannot parse %s handle\n", phandle);
		rc = -ENODEV;
		goto end;
	}

	nb.notifier_call = dp_display_fsa4480_callback;
	nb.priority = 0;

	rc = fsa4480_reg_notifier(&nb, dp->aux_switch_node);
	if (rc) {
		DP_ERR("failed to register notifier (%d)\n", rc);
		goto end;
	}

	fsa4480_unreg_notifier(&nb, dp->aux_switch_node);
end:
	return rc;
}
#endif

static int dp_display_mst_install(struct dp_display *dp_display,
			struct dp_mst_drm_install_info *mst_install_info)
{
	struct dp_display_private *dp;

	if (!dp_display || !mst_install_info) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	if (!mst_install_info->cbs->hpd || !mst_install_info->cbs->hpd_irq) {
		DP_ERR("invalid mst cbs\n");
		return -EINVAL;
	}

	dp_display->dp_mst_prv_info = mst_install_info->dp_mst_prv_info;

	if (!dp->parser->has_mst) {
		DP_DEBUG("mst not enabled\n");
		return -EPERM;
	}

	memcpy(&dp->mst.cbs, mst_install_info->cbs, sizeof(dp->mst.cbs));
	dp->mst.drm_registered = true;

	DP_MST_DEBUG("dp mst drm installed\n");

	return 0;
}

static int dp_display_mst_uninstall(struct dp_display *dp_display)
{
	struct dp_display_private *dp;

	if (!dp_display) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	if (!dp->mst.drm_registered) {
		DP_DEBUG("drm mst not registered\n");
		return -EPERM;
	}

	dp = container_of(dp_display, struct dp_display_private,
				dp_display);
	memset(&dp->mst.cbs, 0, sizeof(dp->mst.cbs));
	dp->mst.drm_registered = false;

	DP_MST_DEBUG("dp mst drm uninstalled\n");

	return 0;
}

static int dp_display_mst_connector_install(struct dp_display *dp_display,
		struct drm_connector *connector)
{
	int rc = 0;
	struct dp_panel_in panel_in;
	struct dp_panel *dp_panel;
	struct dp_display_private *dp;
	struct dp_mst_connector *mst_connector;

	if (!dp_display || !connector) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	mutex_lock(&dp->session_lock);

	if (!dp->mst.drm_registered) {
		DP_DEBUG("drm mst not registered\n");
		mutex_unlock(&dp->session_lock);
		return -EPERM;
	}

	panel_in.dev = &dp->pdev->dev;
	panel_in.aux = dp->aux;
	panel_in.catalog = &dp->catalog->panel;
	panel_in.link = dp->link;
	panel_in.connector = connector;
	panel_in.base_panel = dp->panel;
	panel_in.parser = dp->parser;

	dp_panel = dp_panel_get(&panel_in);
	if (IS_ERR(dp_panel)) {
		rc = PTR_ERR(dp_panel);
		DP_ERR("failed to initialize panel, rc = %d\n", rc);
		mutex_unlock(&dp->session_lock);
		return rc;
	}

	dp_panel->audio = dp_audio_get(dp->pdev, dp_panel, &dp->catalog->audio);
	if (IS_ERR(dp_panel->audio)) {
		rc = PTR_ERR(dp_panel->audio);
		DP_ERR("[mst] failed to initialize audio, rc = %d\n", rc);
		dp_panel->audio = NULL;
		mutex_unlock(&dp->session_lock);
		return rc;
	}

	DP_MST_DEBUG("dp mst connector installed. conn:%d\n",
			connector->base.id);

	mutex_lock(&dp->debug->dp_mst_connector_list.lock);

	mst_connector = kmalloc(sizeof(struct dp_mst_connector),
			GFP_KERNEL);
	if (!mst_connector) {
		mutex_unlock(&dp->debug->dp_mst_connector_list.lock);
		mutex_unlock(&dp->session_lock);
		return -ENOMEM;
	}

	mst_connector->debug_en = false;
	mst_connector->conn = connector;
	mst_connector->con_id = connector->base.id;
	mst_connector->state = connector_status_unknown;
	INIT_LIST_HEAD(&mst_connector->list);

	list_add(&mst_connector->list,
			&dp->debug->dp_mst_connector_list.list);

	mutex_unlock(&dp->debug->dp_mst_connector_list.lock);
	mutex_unlock(&dp->session_lock);

	return 0;
}

static int dp_display_mst_connector_uninstall(struct dp_display *dp_display,
			struct drm_connector *connector)
{
	int rc = 0;
	struct sde_connector *sde_conn;
	struct dp_panel *dp_panel;
	struct dp_display_private *dp;
	struct dp_mst_connector *con_to_remove, *temp_con;

	if (!dp_display || !connector) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	mutex_lock(&dp->session_lock);

	if (!dp->mst.drm_registered) {
		DP_DEBUG("drm mst not registered\n");
		mutex_unlock(&dp->session_lock);
		return -EPERM;
	}

	sde_conn = to_sde_connector(connector);
	if (!sde_conn->drv_panel) {
		DP_ERR("invalid panel for connector:%d\n", connector->base.id);
		mutex_unlock(&dp->session_lock);
		return -EINVAL;
	}

	dp_panel = sde_conn->drv_panel;
	dp_audio_put(dp_panel->audio);
	dp_panel_put(dp_panel);

	DP_MST_DEBUG("dp mst connector uninstalled. conn:%d\n",
			connector->base.id);

	mutex_lock(&dp->debug->dp_mst_connector_list.lock);

	list_for_each_entry_safe(con_to_remove, temp_con,
			&dp->debug->dp_mst_connector_list.list, list) {
		if (con_to_remove->conn == connector) {
			list_del(&con_to_remove->list);
			kfree(con_to_remove);
		}
	}

	mutex_unlock(&dp->debug->dp_mst_connector_list.lock);
	mutex_unlock(&dp->session_lock);

	return rc;
}

static int dp_display_mst_get_connector_info(struct dp_display *dp_display,
			struct drm_connector *connector,
			struct dp_mst_connector *mst_conn)
{
	struct dp_display_private *dp;
	struct dp_mst_connector *conn, *temp_conn;

	if (!connector || !mst_conn) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	mutex_lock(&dp->session_lock);
	if (!dp->mst.drm_registered) {
		DP_DEBUG("drm mst not registered\n");
		mutex_unlock(&dp->session_lock);
		return -EPERM;
	}

	mutex_lock(&dp->debug->dp_mst_connector_list.lock);
	list_for_each_entry_safe(conn, temp_conn,
			&dp->debug->dp_mst_connector_list.list, list) {
		if (conn->con_id == connector->base.id)
			memcpy(mst_conn, conn, sizeof(*mst_conn));
	}
	mutex_unlock(&dp->debug->dp_mst_connector_list.lock);
	mutex_unlock(&dp->session_lock);
	return 0;
}

static int dp_display_mst_connector_update_edid(struct dp_display *dp_display,
			struct drm_connector *connector,
			struct edid *edid)
{
	int rc = 0;
	struct sde_connector *sde_conn;
	struct dp_panel *dp_panel;
	struct dp_display_private *dp;

	if (!dp_display || !connector || !edid) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	if (!dp->mst.drm_registered) {
		DP_DEBUG("drm mst not registered\n");
		return -EPERM;
	}

	sde_conn = to_sde_connector(connector);
	if (!sde_conn->drv_panel) {
		DP_ERR("invalid panel for connector:%d\n", connector->base.id);
		return -EINVAL;
	}

	dp_panel = sde_conn->drv_panel;
	rc = dp_panel->update_edid(dp_panel, edid);

	DP_MST_DEBUG("dp mst connector:%d edid updated. mode_cnt:%d\n",
			connector->base.id, rc);

	return rc;
}

static int dp_display_update_pps(struct dp_display *dp_display,
		struct drm_connector *connector, char *pps_cmd)
{
	struct sde_connector *sde_conn;
	struct dp_panel *dp_panel;
	struct dp_display_private *dp;

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	sde_conn = to_sde_connector(connector);
	if (!sde_conn->drv_panel) {
		DP_ERR("invalid panel for connector:%d\n", connector->base.id);
		return -EINVAL;
	}

	if (!dp_display_state_is(DP_STATE_ENABLED)) {
		dp_display_state_show("[not enabled]");
		return 0;
	}

	dp_panel = sde_conn->drv_panel;
	dp_panel->update_pps(dp_panel, pps_cmd);
	return 0;
}

static int dp_display_mst_connector_update_link_info(
			struct dp_display *dp_display,
			struct drm_connector *connector)
{
	int rc = 0;
	struct sde_connector *sde_conn;
	struct dp_panel *dp_panel;
	struct dp_display_private *dp;

	if (!dp_display || !connector) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	if (!dp->mst.drm_registered) {
		DP_DEBUG("drm mst not registered\n");
		return -EPERM;
	}

	sde_conn = to_sde_connector(connector);
	if (!sde_conn->drv_panel) {
		DP_ERR("invalid panel for connector:%d\n", connector->base.id);
		return -EINVAL;
	}

	dp_panel = sde_conn->drv_panel;

	memcpy(dp_panel->dpcd, dp->panel->dpcd,
			DP_RECEIVER_CAP_SIZE + 1);
	memcpy(dp_panel->dsc_dpcd, dp->panel->dsc_dpcd,
			DP_RECEIVER_DSC_CAP_SIZE + 1);
	memcpy(&dp_panel->link_info, &dp->panel->link_info,
			sizeof(dp_panel->link_info));

	DP_MST_DEBUG("dp mst connector:%d link info updated\n",
		connector->base.id);

	return rc;
}

static int dp_display_mst_get_fixed_topology_port(
			struct dp_display *dp_display,
			u32 strm_id, u32 *port_num)
{
	struct dp_display_private *dp;
	u32 port;

	if (!dp_display) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	if (strm_id >= DP_STREAM_MAX) {
		DP_ERR("invalid stream id:%d\n", strm_id);
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	port = dp->parser->mst_fixed_port[strm_id];

	if (!port || port > 255)
		return -ENOENT;

	if (port_num)
		*port_num = port;

	return 0;
}

static int dp_display_get_mst_caps(struct dp_display *dp_display,
			struct dp_mst_caps *mst_caps)
{
	int rc = 0;
	struct dp_display_private *dp;

	if (!dp_display || !mst_caps) {
		DP_ERR("invalid input\n");
		return -EINVAL;
	}

	DP_DEBUG("+++\n");

	dp = container_of(dp_display, struct dp_display_private, dp_display);

	mst_caps->has_mst = dp->parser->has_mst;
	mst_caps->max_streams_supported = (mst_caps->has_mst) ? 2 : 0;
	mst_caps->max_dpcd_transaction_bytes = (mst_caps->has_mst) ? 16 : 0;
	mst_caps->drm_aux = dp->aux->drm_aux;

	return rc;
}

static void dp_display_wakeup_phy_layer(struct dp_display *dp_display,
		bool wakeup)
{
	struct dp_display_private *dp;
	struct dp_hpd *hpd;

	if (!dp_display) {
		DP_ERR("invalid input\n");
		return;
	}

	dp = container_of(dp_display, struct dp_display_private, dp_display);
	if (!dp->mst.drm_registered) {
		DP_DEBUG("drm mst not registered\n");
		return;
	}

	DP_DEBUG("+++\n");

	hpd = dp->hpd;
	if (hpd && hpd->wakeup_phy)
		hpd->wakeup_phy(hpd, wakeup);
}

static int dp_display_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct dp_display_private *dp;

	if (!pdev || !pdev->dev.of_node) {
		DP_ERR("pdev not found\n");
		rc = -ENODEV;
		goto bail;
	}

	dp = devm_kzalloc(&pdev->dev, sizeof(*dp), GFP_KERNEL);
	if (!dp) {
		rc = -ENOMEM;
		goto bail;
	}

	init_completion(&dp->notification_comp);
#ifdef CONFIG_SEC_DISPLAYPORT
	init_completion(&dp->dp_off_comp);
	init_completion(&dp->dp_disconnect_comp);

	dp->reboot_nb.notifier_call = secdp_reboot_cb;
	register_reboot_notifier(&dp->reboot_nb);

#ifdef CONFIG_SWITCH
	rc = switch_dev_register(&switch_secdp_msg);
	if (rc)
		DP_INFO("Failed to register secdp_msg switch(%d)\n", rc);
#endif
#endif

	dp->pdev = pdev;
	dp->name = "drm_dp";

	memset(&dp->mst, 0, sizeof(dp->mst));

/* aux switch error is coming repeatedly during boot time when samsung
 * displayport feature is disabled. so, disabled following codes
 */
#if 0/*CONFIG_SEC_DISPLAYPORT*/
	rc = dp_display_init_aux_switch(dp);
	if (rc) {
		rc = -EPROBE_DEFER;
		goto error;
	}
#endif

	rc = dp_display_create_workqueue(dp);
	if (rc) {
		DP_ERR("Failed to create workqueue\n");
		goto error;
	}

#ifdef CONFIG_SEC_DISPLAYPORT
	g_secdp_priv = dp;
	secdp_logger_init();
	atomic_set(&dp->notification_status, 0);
#endif

	platform_set_drvdata(pdev, dp);

	g_dp_display = &dp->dp_display;

	g_dp_display->enable        = dp_display_enable;
	g_dp_display->post_enable   = dp_display_post_enable;
	g_dp_display->pre_disable   = dp_display_pre_disable;
	g_dp_display->disable       = dp_display_disable;
	g_dp_display->set_mode      = dp_display_set_mode;
	g_dp_display->validate_mode = dp_display_validate_mode;
	g_dp_display->get_modes     = dp_display_get_modes;
	g_dp_display->prepare       = dp_display_prepare;
	g_dp_display->unprepare     = dp_display_unprepare;
	g_dp_display->request_irq   = dp_request_irq;
	g_dp_display->get_debug     = dp_get_debug;
	g_dp_display->post_open     = NULL;
	g_dp_display->post_init     = dp_display_post_init;
	g_dp_display->config_hdr    = dp_display_config_hdr;
	g_dp_display->mst_install   = dp_display_mst_install;
	g_dp_display->mst_uninstall = dp_display_mst_uninstall;
	g_dp_display->mst_connector_install = dp_display_mst_connector_install;
	g_dp_display->mst_connector_uninstall =
					dp_display_mst_connector_uninstall;
	g_dp_display->mst_connector_update_edid =
					dp_display_mst_connector_update_edid;
	g_dp_display->mst_connector_update_link_info =
				dp_display_mst_connector_update_link_info;
	g_dp_display->get_mst_caps = dp_display_get_mst_caps;
	g_dp_display->set_stream_info = dp_display_set_stream_info;
	g_dp_display->update_pps = dp_display_update_pps;
	g_dp_display->convert_to_dp_mode = dp_display_convert_to_dp_mode;
	g_dp_display->mst_get_connector_info =
					dp_display_mst_get_connector_info;
	g_dp_display->mst_get_fixed_topology_port =
					dp_display_mst_get_fixed_topology_port;
	g_dp_display->wakeup_phy_layer =
					dp_display_wakeup_phy_layer;
	g_dp_display->set_colorspace = dp_display_setup_colospace;

	rc = component_add(&pdev->dev, &dp_display_comp_ops);
	if (rc) {
		DP_ERR("component add failed, rc=%d\n", rc);
		goto error;
	}

	DP_INFO("exit, rc(%d)\n", rc);

	return 0;
error:
	devm_kfree(&pdev->dev, dp);
bail:
	return rc;
}

int dp_display_get_displays(void **displays, int count)
{
	if (!displays) {
		DP_ERR("invalid data\n");
		return -EINVAL;
	}

	if (count != 1) {
		DP_ERR("invalid number of displays\n");
		return -EINVAL;
	}

	displays[0] = g_dp_display;
	return count;
}

int dp_display_get_num_of_displays(void)
{
	if (!g_dp_display)
		return 0;

	return 1;
}

int dp_display_get_num_of_streams(void)
{
	if (g_dp_display->no_mst_encoder)
		return 0;

	return DP_STREAM_MAX;
}

static void dp_display_set_mst_state(void *dp_display,
		enum dp_drv_state mst_state)
{
	struct dp_display_private *dp;

	if (!g_dp_display) {
		DP_DEBUG("dp display not initialized\n");
		return;
	}

	dp = container_of(g_dp_display, struct dp_display_private, dp_display);
	if (dp->mst.mst_active && dp->mst.cbs.set_drv_state)
		dp->mst.cbs.set_drv_state(g_dp_display, mst_state);
}

static int dp_display_remove(struct platform_device *pdev)
{
	struct dp_display_private *dp;

	if (!pdev)
		return -EINVAL;

	dp = platform_get_drvdata(pdev);

	dp_display_deinit_sub_modules(dp);

	if (dp->wq)
		destroy_workqueue(dp->wq);

	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, dp);

#if defined(CONFIG_SEC_DISPLAYPORT) && defined(CONFIG_SWITCH)
	switch_dev_unregister(&switch_secdp_msg);
#endif
	return 0;
}

static int dp_pm_prepare(struct device *dev)
{
	struct dp_display_private *dp = container_of(g_dp_display,
			struct dp_display_private, dp_display);

	mutex_lock(&dp->session_lock);
	dp_display_set_mst_state(g_dp_display, PM_SUSPEND);

	/*
	 * There are a few instances where the DP is hotplugged when the device
	 * is in PM suspend state. After hotplug, it is observed the device
	 * enters and exits the PM suspend multiple times while aux transactions
	 * are taking place. This may sometimes cause an unclocked register
	 * access error. So, abort aux transactions when such a situation
	 * arises i.e. when DP is connected but display not enabled yet.
	 */
	if (dp_display_state_is(DP_STATE_CONNECTED) &&
			!dp_display_state_is(DP_STATE_ENABLED)) {
		dp->aux->abort(dp->aux, true);
		dp->ctrl->abort(dp->ctrl, true);
	}

	dp_display_state_add(DP_STATE_SUSPENDED);
	mutex_unlock(&dp->session_lock);

	return 0;
}

static void dp_pm_complete(struct device *dev)
{
	struct dp_display_private *dp = container_of(g_dp_display,
			struct dp_display_private, dp_display);

	mutex_lock(&dp->session_lock);
	dp_display_set_mst_state(g_dp_display, PM_DEFAULT);

	/*
	 * There are multiple PM suspend entry and exits observed before
	 * the connect uevent is issued to userspace. The aux transactions are
	 * aborted during PM suspend entry in dp_pm_prepare to prevent unclocked
	 * register access. On PM suspend exit, there will be no host_init call
	 * to reset the abort flags for ctrl and aux incase DP is connected
	 * but display not enabled. So, resetting abort flags for aux and ctrl.
	 */
	if (dp_display_state_is(DP_STATE_CONNECTED) &&
			!dp_display_state_is(DP_STATE_ENABLED)) {
		dp->aux->abort(dp->aux, false);
		dp->ctrl->abort(dp->ctrl, false);
	}

	dp_display_state_remove(DP_STATE_SUSPENDED);
	mutex_unlock(&dp->session_lock);
}

static const struct dev_pm_ops dp_pm_ops = {
	.prepare = dp_pm_prepare,
	.complete = dp_pm_complete,
};

static struct platform_driver dp_display_driver = {
	.probe  = dp_display_probe,
	.remove = dp_display_remove,
	.driver = {
		.name = "msm-dp-display",
		.of_match_table = dp_dt_match,
		.suppress_bind_attrs = true,
		.pm = &dp_pm_ops,
	},
};

static int __init dp_display_init(void)
{
	int ret;

	ret = platform_driver_register(&dp_display_driver);
	if (ret) {
		DP_ERR("driver register failed\n");
		return ret;
	}

	return ret;
}
late_initcall(dp_display_init);

static void __exit dp_display_cleanup(void)
{
	platform_driver_unregister(&dp_display_driver);
}
module_exit(dp_display_cleanup);
