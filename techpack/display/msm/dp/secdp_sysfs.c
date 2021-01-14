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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <drm/drm_edid.h>
#include <linux/string.h>
#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
#include <linux/displayport_bigdata.h>
#endif

#include "dp_debug.h"
#include "secdp.h"
#include "secdp_sysfs.h"
#include "sde_edid_parser.h"
#include "secdp_unit_test.h"

enum secdp_unit_test_cmd {
	SECDP_UTCMD_EDID_PARSE = 0,
};

struct secdp_sysfs_private {
	struct device *dev;
	struct secdp_sysfs dp_sysfs;
	struct secdp_misc *sec;
	enum secdp_unit_test_cmd test_cmd;
};

struct secdp_sysfs_private *g_secdp_sysfs;

static inline char *secdp_utcmd_to_str(u32 cmd_type)
{
	switch (cmd_type) {
	case SECDP_UTCMD_EDID_PARSE:
		return SECDP_ENUM_STR(SECDP_UTCMD_EDID_PARSE);
	default:
		return "unknown";
	}
}

/** check if buf has range('-') format
 * @buf		buf to be checked
 * @size	buf size
 * @retval	0 if args are ok, -1 if '-' included
 */
static int secdp_check_store_args(const char *buf, size_t size)
{
	int ret;

	if (strnchr(buf, size, '-')) {
		DP_ERR("range is forbidden!\n");
		ret = -1;
		goto exit;
	}

	ret = 0;
exit:
	return ret;
}

#ifdef CONFIG_SEC_FACTORY
static ssize_t dp_sbu_sw_sel_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	int val[10] = {0,};
	int sbu_sw_sel, sbu_sw_oe;

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	get_options(buf, ARRAY_SIZE(val), val);

	sbu_sw_sel = val[1];
	sbu_sw_oe = val[2];
	DP_INFO("sbu_sw_sel(%d), sbu_sw_oe(%d)\n", sbu_sw_sel, sbu_sw_oe);

	if (sbu_sw_oe == 0/*on*/)
		secdp_config_gpios_factory(sbu_sw_sel, true);
	else if (sbu_sw_oe == 1/*off*/)
		secdp_config_gpios_factory(sbu_sw_sel, false);
	else
		DP_ERR("unknown sbu_sw_oe value: %d\n", sbu_sw_oe);

exit:
	return size;
}

static CLASS_ATTR_WO(dp_sbu_sw_sel);
#endif

static ssize_t dex_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	int rc = 0;
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	struct secdp_dex *dex = &sysfs->sec->dex;

	if (!secdp_get_cable_status() || !secdp_get_hpd_status() ||
			secdp_get_poor_connection_status() || !secdp_get_link_train_status()) {
		DP_INFO("cable is out\n");
		dex->prev = dex->curr = dex->status = DEX_DISABLED;
	}

	DP_INFO("prev: %d, curr: %d, status: %d\n",
			dex->prev, dex->curr, dex->status);
	rc = scnprintf(buf, PAGE_SIZE, "%d\n", dex->status);

	if (dex->status == DEX_DURING_MODE_CHANGE)
		dex->status = dex->curr;

	return rc;
}

/*
 * assume that 1 HMD device has name(14),vid(4),pid(4) each, then
 * max 32 HMD devices(name,vid,pid) need 806 bytes including TAG, NUM, comba
 */
#define MAX_DEX_STORE_LEN	1024

static ssize_t dex_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t size)
{
	char str[MAX_DEX_STORE_LEN] = {0,}, *p, *tok;
	int len, val[4] = {0,};
	int setting_ui;	/* setting has Dex mode? if yes, 1. otherwise 0 */
	int run;	/* dex is running now?   if yes, 1. otherwise 0 */

	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	struct secdp_misc *sec = sysfs->sec;
	struct secdp_dex *dex = &sec->dex;

	if (secdp_check_if_lpm_mode()) {
		DP_INFO("it's LPM mode. skip\n");
		goto exit;
	}

	if (size >= MAX_DEX_STORE_LEN) {
		DP_ERR("too long args! %d\n", size);
		goto exit;
	}

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	mutex_lock(&sec->hmd_lock);
	memcpy(str, buf, size);
	p   = str;
	tok = strsep(&p, ",");
	len = strlen(tok);
	DP_DEBUG("tok: %s, len: %d\n", tok, len);

	if (!strncmp(DEX_TAG_HMD, tok, len)) {
		/* called by HmtManager to inform list of supported HMD devices
		 *
		 * Format :
		 *   HMD,NUM,NAME01,VID01,PID01,NAME02,VID02,PID02,...
		 *
		 *   HMD  : tag
		 *   NUM  : num of HMD dev ..... max 2 bytes to decimal (max 32)
		 *   NAME : name of HMD ...... max 14 bytes, char string
		 *   VID  : vendor  id ....... 4 bytes to hexadecimal
		 *   PID  : product id ....... 4 bytes to hexadecimal
		 *
		 * ex) HMD,2,PicoVR,2d40,0000,Nreal light,0486,573c
		 *
		 * call hmd store function with tag(HMD),NUM removed
		 */
		int num_hmd = 0, sz = 0, ret;

		tok = strsep(&p, ",");
		sz  = strlen(tok);
		ret = kstrtouint(tok, 10, &num_hmd);
		DP_DEBUG("HMD num: %d, sz:%d, ret:%d\n", num_hmd, sz, ret);
		if (!ret) {
			ret = secdp_store_hmd_dev(str + (len + sz + 2),
					size - (len + sz + 2), num_hmd);
		}
		if (ret)
			size = ret;	/* error! */

		mutex_unlock(&sec->hmd_lock);
		goto exit;
	}
	mutex_unlock(&sec->hmd_lock);

	get_options(buf, ARRAY_SIZE(val), val);
	DP_INFO("%d(0x%02x)\n", val[1], val[1]);
	setting_ui = (val[1] & 0xf0) >> 4;
	run = (val[1] & 0x0f);

	DP_INFO("setting_ui: %d, run: %d, cable: %d\n",
		setting_ui, run, sec->cable_connected);

	dex->setting_ui = setting_ui;
	dex->status = dex->curr = run;

	mutex_lock(&sec->notifier_lock);
	if (!sec->ccic_noti_registered) {
		int rc;

		DP_DEBUG("notifier get registered by dex\n");

		/* cancel immediately */
		rc = cancel_delayed_work(&sec->ccic_noti_reg_work);
		DP_DEBUG("cancel_work, rc(%d)\n", rc);
		destroy_delayed_work_on_stack(&sec->ccic_noti_reg_work);

		/* register */
		rc = secdp_ccic_noti_register_ex(sec, false);
		if (rc)
			DP_ERR("noti register fail, rc(%d)\n", rc);

		mutex_unlock(&sec->notifier_lock);
		goto exit;
	}
	mutex_unlock(&sec->notifier_lock);

	if (!secdp_get_cable_status() || !secdp_get_hpd_status() ||
			secdp_get_poor_connection_status() || !secdp_get_link_train_status()) {
		DP_INFO("cable is out\n");
		dex->prev = dex->curr = dex->status = DEX_DISABLED;
		goto exit;
	}

	if (sec->hpd_noti_deferred) {
		secdp_send_deferred_hpd_noti();
		dex->prev = dex->setting_ui;
		goto exit;
	}

	if (dex->curr == dex->prev) {
		DP_INFO("dex is %s already\n",
			dex->curr ? "enabled" : "disabled");
		goto exit;
	}

	if (dex->curr != dex->setting_ui) {
		DP_INFO("values of cur(%d) and setting_ui(%d) are difference\n",
			dex->curr, dex->setting_ui);
		goto exit;
	}

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	if (run)
		secdp_bigdata_save_item(BD_DP_MODE, "DEX");
	else
		secdp_bigdata_save_item(BD_DP_MODE, "MIRROR");
#endif

	if (sec->dex.res == DEX_RES_NOT_SUPPORT) {
		DP_DEBUG("this dongle does not support dex\n");
		goto exit;
	}

	if (!secdp_check_dex_reconnect()) {
		DP_INFO("not need reconnect\n");
		goto exit;
	}

	secdp_dex_do_reconnecting();

	dex->prev = run;
exit:
	return size;
}

static CLASS_ATTR_RW(dex);

static ssize_t dex_ver_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	int rc;
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	struct secdp_misc *sec = sysfs->sec;
	struct secdp_dex *dex = &sec->dex;

	DP_INFO("branch revision: HW(0x%X), SW(0x%X, 0x%X)\n",
		dex->fw_ver[0], dex->fw_ver[1], dex->fw_ver[2]);

	rc = scnprintf(buf, PAGE_SIZE, "%02X%02X\n",
		dex->fw_ver[1], dex->fw_ver[2]);

	return rc;
}

static CLASS_ATTR_RO(dex_ver);

/* note: needs test once wifi is fixed */
static ssize_t monitor_info_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	int rc = 0;
	short prod_id = 0;
	struct dp_panel *info = NULL;
	struct sde_edid_ctrl *edid_ctrl = NULL;
	struct edid *edid = NULL;

	info = secdp_get_panel_info();
	if (!info) {
		DP_ERR("unable to find panel info\n");
		goto exit;
	}

	edid_ctrl = info->edid_ctrl;
	if (!edid_ctrl) {
		DP_ERR("unable to find edid_ctrl\n");
		goto exit;
	}

	edid = edid_ctrl->edid;
	if (!edid) {
		DP_ERR("unable to find edid\n");
		goto exit;
	}

	DP_DEBUG("prod_code[0]: %02x, [1]: %02x\n",
		edid->prod_code[0], edid->prod_code[1]);
	prod_id |= (edid->prod_code[0] << 8) | (edid->prod_code[1]);
	DP_DEBUG("prod_id: %04x\n", prod_id);

	rc = snprintf(buf, SZ_32, "%s,0x%x,0x%x\n",
			edid_ctrl->vendor_id, prod_id, edid->serial); /* byte order? */
exit:
	return rc;
}

static CLASS_ATTR_RO(monitor_info);

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
static ssize_t dp_error_info_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return _secdp_bigdata_show(class, attr, buf);
}

static ssize_t dp_error_info_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	return _secdp_bigdata_store(dev, attr, buf, size);
}

static CLASS_ATTR_RW(dp_error_info);
#endif

#ifdef SECDP_SELF_TEST
struct secdp_sef_test_item g_self_test[] = {
	{DP_ENUM_STR(ST_CLEAR_CMD), .arg_cnt = 0, .arg_str = "clear all configurations"},
	{DP_ENUM_STR(ST_LANE_CNT), .arg_cnt = 1, .arg_str = "lane_count: 1 = 1 lane, 2 = 2 lane, 4 = 4 lane, -1 = disable"},
	{DP_ENUM_STR(ST_LINK_RATE), .arg_cnt = 1, .arg_str = "link_rate: 1 = 1.62G , 2 = 2.7G, 3 = 5.4G, -1 = disable"},
	{DP_ENUM_STR(ST_CONNECTION_TEST), .arg_cnt = 1, .arg_str = "reconnection time(sec) : range = 5 ~ 50, -1 = disable"},
	{DP_ENUM_STR(ST_HDCP_TEST), .arg_cnt = 1, .arg_str = "hdcp on/off time(sec): range = 5 ~ 50, -1 = disable"},
	{DP_ENUM_STR(ST_EDID), .arg_cnt = 0, .arg_str = "need to write edid to \"sys/class/dp_sec/dp_edid\" sysfs node, -1 = disable"},
	{DP_ENUM_STR(ST_PREEM_TUN), .arg_cnt = 16, .arg_str = "pre-emphasis calibration value, -1 = disable"},
	{DP_ENUM_STR(ST_VOLTAGE_TUN), .arg_cnt = 16, .arg_str = "voltage-level calibration value, -1 = disable"},
};

int secdp_self_test_status(int cmd)
{
	if (cmd >= ST_MAX)
		return -EINVAL;

	if (g_self_test[cmd].enabled) {
		DP_INFO("%s : %s\n", g_self_test[cmd].cmd_str,
			g_self_test[cmd].enabled ? "true" : "false");
	}

	return g_self_test[cmd].enabled ? g_self_test[cmd].arg_cnt : -1;
}

int *secdp_self_test_get_arg(int cmd)
{
	return g_self_test[cmd].arg;
}

void secdp_self_register_clear_func(int cmd, void (*func)(void))
{
	if (cmd >= ST_MAX)
		return;

	g_self_test[cmd].clear = func;
	DP_INFO("%s : clear func was registered.\n", g_self_test[cmd].cmd_str);
}

u8 *secdp_self_test_get_edid(void)
{
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;

	return	sysfs->sec->self_test_edid;
}

static void secdp_self_test_reconnect_work(struct work_struct *work)
{
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	int delay = g_self_test[ST_CONNECTION_TEST].arg[0];
	static unsigned long test_cnt;

	if (!secdp_get_cable_status() || !secdp_get_hpd_status()) {
		DP_INFO("cable is out\n");
		test_cnt = 0;
		return;
	}

	if (sysfs->sec->self_test_reconnect_callback)
		sysfs->sec->self_test_reconnect_callback();

	test_cnt++;
	DP_INFO("test_cnt :%lu\n", test_cnt);

	schedule_delayed_work(&sysfs->sec->self_test_reconnect_work,
		msecs_to_jiffies(delay * 1000));
}

void secdp_self_test_start_reconnect(void (*func)(void))
{
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	int delay = g_self_test[ST_CONNECTION_TEST].arg[0];

	if (delay > 50 || delay < 5)
		delay = g_self_test[ST_CONNECTION_TEST].arg[0] = 10;

	DP_INFO("start reconnect test : delay %d sec\n", delay);

	sysfs->sec->self_test_reconnect_callback = func;
	schedule_delayed_work(&sysfs->sec->self_test_reconnect_work,
		msecs_to_jiffies(delay * 1000));
}

static void secdp_self_test_hdcp_test_work(struct work_struct *work)
{
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	int delay = g_self_test[ST_HDCP_TEST].arg[0];
	static unsigned long test_cnt;

	if (!secdp_get_cable_status() || !secdp_get_hpd_status()) {
		DP_INFO("cable is out\n");
		test_cnt = 0;
		return;
	}

	if (sysfs->sec->self_test_hdcp_off_callback)
		sysfs->sec->self_test_hdcp_off_callback();

	msleep(3000);

	if (sysfs->sec->self_test_hdcp_on_callback)
		sysfs->sec->self_test_hdcp_on_callback();

	test_cnt++;
	DP_INFO("test_cnt :%lu\n", test_cnt);

	schedule_delayed_work(&sysfs->sec->self_test_hdcp_test_work,
		msecs_to_jiffies(delay * 1000));

}

void secdp_self_test_start_hdcp_test(void (*func_on)(void),
		void (*func_off)(void))
{
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	int delay = g_self_test[ST_HDCP_TEST].arg[0];

	if (delay == 0) {
		DP_INFO("hdcp test is aborted\n");
		return;
	}

	if (delay > 50 || delay < 5)
		delay = g_self_test[ST_HDCP_TEST].arg[0] = 10;

	DP_INFO("start hdcp test : delay %d sec\n", delay);

	sysfs->sec->self_test_hdcp_on_callback = func_on;
	sysfs->sec->self_test_hdcp_off_callback = func_off;

	schedule_delayed_work(&sysfs->sec->self_test_hdcp_test_work,
		msecs_to_jiffies(delay * 1000));
}

static ssize_t dp_self_test_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	int i, j, rc = 0;

	for (i = 0; i < ST_MAX; i++) {
		rc += scnprintf(buf + rc, PAGE_SIZE - rc,
				"%d. %s : %s\n   ==>", i,
				g_self_test[i].cmd_str, g_self_test[i].arg_str);

		if (g_self_test[i].enabled) {
			rc += scnprintf(buf + rc, PAGE_SIZE - rc,
					"current value : enabled - arg :");

			for (j = 0; j < g_self_test[i].arg_cnt; j++) {
				rc += scnprintf(buf + rc, PAGE_SIZE - rc,
						"0x%x ", g_self_test[i].arg[j]);
			}

			rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n\n");
		} else {
			rc += scnprintf(buf + rc, PAGE_SIZE - rc,
				"current value : disabled\n\n");
		}
	}

	return rc;
}

static void dp_self_test_clear_func(int cmd)
{
	int arg_cnt = g_self_test[cmd].arg_cnt < ST_ARG_CNT ? g_self_test[cmd].arg_cnt : ST_ARG_CNT;

	g_self_test[cmd].enabled = false;
	memset(g_self_test[cmd].arg, 0,	sizeof(int) * arg_cnt);

	if (g_self_test[cmd].clear)
		g_self_test[cmd].clear();
}

static ssize_t dp_self_test_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	int val[ST_ARG_CNT + 2] = {0, };
	int arg, arg_cnt, cmd, i;

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto end;
	}

	get_options(buf, ARRAY_SIZE(val), val);
	cmd = val[1];
	arg = val[2];

	if (cmd < 0 || cmd >= ST_MAX) {
		DP_INFO("invalid cmd\n");
		goto end;
	}

	if (cmd == ST_CLEAR_CMD) {
		for (i = 1; i < ST_MAX; i++)
			dp_self_test_clear_func(i);

		DP_INFO("cmd : ST_CLEAR_CMD\n");
		goto end;
	}

	g_self_test[cmd].enabled = arg < 0 ? false : true;
	if (g_self_test[cmd].enabled) {
		if ((val[0] - 1) != g_self_test[cmd].arg_cnt) {
			DP_INFO("invalid param.\n");
			goto end;
		}

		arg_cnt = g_self_test[cmd].arg_cnt < ST_ARG_CNT ? g_self_test[cmd].arg_cnt : ST_ARG_CNT;
		memcpy(g_self_test[cmd].arg, val + 2, sizeof(int) * arg_cnt);
	} else {
		dp_self_test_clear_func(cmd);
	}

	DP_INFO("cmd: %d. %s, enabled:%s\n", cmd,
				cmd < ST_MAX ? g_self_test[cmd].cmd_str : "null",
				cmd < ST_MAX ? (g_self_test[cmd].enabled ? "true" : "false") : "null");
end:
	return size;
}

static CLASS_ATTR_RW(dp_self_test);

static ssize_t dp_edid_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	int i, flag = 0;
	int rc = 0;

	rc += scnprintf(buf + rc, PAGE_SIZE - rc,
			"EDID test is %s\n", g_self_test[ST_EDID].enabled ? "enabled" : "disabled");

	for (i = 0; i < ST_EDID_SIZE; i++) {
		rc += scnprintf(buf + rc, PAGE_SIZE - rc,
				"0x%02x ", sysfs->sec->self_test_edid[i]);
		if (!((i+1)%8)) {
			rc += scnprintf(buf + rc, PAGE_SIZE - rc,
					"%s", flag ? "\n" : "  ");
			flag = !flag;
			if (!((i+1) % 128))
				rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n");
		}
	}

	return rc;
}

static ssize_t dp_edid_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	int val[ST_EDID_SIZE + 1] = {0, };
	char *temp;
	size_t i, j = 0;

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto error;
	}

	temp = kzalloc(size, GFP_KERNEL);
	if (!temp) {
		DP_ERR("buffer alloc error\n");
		dp_self_test_clear_func(ST_EDID);
		goto error;
	}

	/* remove space */
	for (i = 0; i < size; i++) {
		if (buf[i] != ' ')
			temp[j++] = buf[i];
	}

	get_options(temp, ARRAY_SIZE(val), val);

	if (val[0] % 128) {
		DP_ERR("invalid EDID(%d)\n", val[0]);
		dp_self_test_clear_func(ST_EDID);
		goto end;
	}

	memset(sysfs->sec->self_test_edid, 0, sizeof(ST_EDID_SIZE));

	for (i = 0; i < val[0]; i++)
		sysfs->sec->self_test_edid[i] = (u8)val[i+1];

	g_self_test[ST_EDID].enabled = true;
end:
	kfree(temp);
error:
	return size;
}

static CLASS_ATTR_RW(dp_edid);
#endif

#ifdef CONFIG_SEC_DISPLAYPORT_ENG
static ssize_t dp_forced_resolution_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	char tmp[SZ_1K] = {0,};
	int  rc = 0, vic = 1;

	if (forced_resolution) {
		rc = scnprintf(buf, PAGE_SIZE,
			"%d: %s", forced_resolution,
			secdp_vic_to_string(forced_resolution));

	} else {
		while (secdp_vic_to_string(vic) != NULL) {
			rc += scnprintf(buf + rc, PAGE_SIZE - rc,
				"%d: %s", vic, secdp_vic_to_string(vic));
			vic++;
		}
	}

	secdp_show_hmd_dev(tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n< HMD >\n%s\n", tmp);

	memset(tmp, 0, ARRAY_SIZE(tmp));

	secdp_show_phy_param(tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n< DP-PHY >\n%s\n", tmp);

	return rc;
}

static ssize_t dp_forced_resolution_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	int val[10] = {0, };

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	get_options(buf, ARRAY_SIZE(val), val);

	if (val[1] <= 0)
		forced_resolution = 0;
	else
		forced_resolution = val[1];

exit:
	return size;
}

static CLASS_ATTR_RW(dp_forced_resolution);

static ssize_t dp_unit_test_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	int rc, cmd = sysfs->test_cmd;
	bool res = false;

	DP_INFO("test_cmd: %s\n", secdp_utcmd_to_str(cmd));

	switch (cmd) {
	case SECDP_UTCMD_EDID_PARSE:
		res = secdp_unit_test_edid_parse();
		break;
	default:
		DP_INFO("invalid test_cmd: %d\n", cmd);
		break;
	}

	rc = scnprintf(buf, 3, "%d\n", res ? 1 : 0);
	return rc;
}

static ssize_t dp_unit_test_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs = g_secdp_sysfs;
	int val[10] = {0, };

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	get_options(buf, ARRAY_SIZE(val), val);
	sysfs->test_cmd = val[1];

	DP_INFO("test_cmd: %d...%s\n", sysfs->test_cmd,
		secdp_utcmd_to_str(sysfs->test_cmd));

exit:
	return size;
}

static CLASS_ATTR_RW(dp_unit_test);

static ssize_t dp_aux_cfg_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	secdp_aux_cfg_show(tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n< AUX cfg >\n%s\n", tmp);

	return rc;
}

static ssize_t dp_aux_cfg_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	char tmp[SZ_1K] = {0,};

	memcpy(tmp, buf, size);
	secdp_aux_cfg_store(tmp);

	return size;
}

static CLASS_ATTR_RW(dp_aux_cfg);

static ssize_t dp_preshoot_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	char tmp[SZ_64] = {0,};
	int  rc = 0;

	secdp_catalog_preshoot_show(tmp);
	rc = snprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_preshoot_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	char tmp[SZ_64] = {0,};

	memcpy(tmp, buf, min(ARRAY_SIZE(tmp), size));
	secdp_catalog_preshoot_store(tmp);

	return size;
}

static CLASS_ATTR_RW(dp_preshoot);

static ssize_t dp_vx_lvl_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	secdp_parse_vxpx_show(DP_HW_LEGACY, DP_PARAM_VX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_vx_lvl_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	char tmp[SZ_64] = {0,};

	memcpy(tmp, buf, size);
	secdp_parse_vxpx_store(DP_HW_LEGACY, DP_PARAM_VX, tmp);

	return size;
}

static CLASS_ATTR_RW(dp_vx_lvl);

static ssize_t dp_px_lvl_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	secdp_parse_vxpx_show(DP_HW_LEGACY, DP_PARAM_PX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_px_lvl_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	char tmp[SZ_64] = {0,};

	memcpy(tmp, buf, size);
	secdp_parse_vxpx_store(DP_HW_LEGACY, DP_PARAM_PX, tmp);

	return size;
}

static CLASS_ATTR_RW(dp_px_lvl);

static ssize_t dp_vx_lvl_hbr2_hbr3_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	secdp_parse_vxpx_show(DP_HW_V123_HBR2_HBR3, DP_PARAM_VX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_vx_lvl_hbr2_hbr3_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	char tmp[SZ_64] = {0,};

	memcpy(tmp, buf, size);
	secdp_parse_vxpx_store(DP_HW_V123_HBR2_HBR3, DP_PARAM_VX, tmp);

	return size;

}

static CLASS_ATTR_RW(dp_vx_lvl_hbr2_hbr3);

static ssize_t dp_px_lvl_hbr2_hbr3_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	secdp_parse_vxpx_show(DP_HW_V123_HBR2_HBR3, DP_PARAM_PX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_px_lvl_hbr2_hbr3_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	char tmp[SZ_64] = {0,};

	memcpy(tmp, buf, size);
	secdp_parse_vxpx_store(DP_HW_V123_HBR2_HBR3, DP_PARAM_PX, tmp);

	return size;
}

static CLASS_ATTR_RW(dp_px_lvl_hbr2_hbr3);

static ssize_t dp_vx_lvl_hbr_rbr_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	secdp_parse_vxpx_show(DP_HW_V123_HBR_RBR, DP_PARAM_VX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_vx_lvl_hbr_rbr_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	char tmp[SZ_64] = {0,};

	memcpy(tmp, buf, size);
	secdp_parse_vxpx_store(DP_HW_V123_HBR_RBR, DP_PARAM_VX, tmp);

	return size;
}

static CLASS_ATTR_RW(dp_vx_lvl_hbr_rbr);

static ssize_t dp_px_lvl_hbr_rbr_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	secdp_parse_vxpx_show(DP_HW_V123_HBR_RBR, DP_PARAM_PX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_px_lvl_hbr_rbr_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	char tmp[SZ_64] = {0,};

	memcpy(tmp, buf, size);
	secdp_parse_vxpx_store(DP_HW_V123_HBR_RBR, DP_PARAM_PX, tmp);

	return size;
}

static CLASS_ATTR_RW(dp_px_lvl_hbr_rbr);

static ssize_t dp_pref_skip_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	int skip, rc;

	DP_DEBUG("+++\n");

	skip = secdp_debug_prefer_skip_show();
	rc = snprintf(buf, SZ_8, "%d\n", skip);

	return rc;
}

static ssize_t dp_pref_skip_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	int i, val[30] = {0, };

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	DP_DEBUG("+++, size(%d)\n", (int)size);

	get_options(buf, ARRAY_SIZE(val), val);
	for (i = 0; i < 16; i = i + 4) {
		DP_DEBUG("%02x,%02x,%02x,%02x\n",
			val[i+1], val[i+2], val[i+3], val[i+4]);
	}

	secdp_debug_prefer_skip_store(val[1]);
exit:
	return size;
}

static CLASS_ATTR_RW(dp_pref_skip);

static ssize_t dp_pref_ratio_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	int ratio, rc;

	DP_DEBUG("+++\n");

	ratio = secdp_debug_prefer_ratio_show();
	rc = snprintf(buf, SZ_8, "%d\n", ratio);

	return rc;
}

static ssize_t dp_pref_ratio_store(struct class *dev,
		struct class_attribute *attr, const char *buf, size_t size)
{
	int i, val[30] = {0, };

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	DP_DEBUG("+++, size(%d)\n", (int)size);

	get_options(buf, ARRAY_SIZE(val), val);
	for (i = 0; i < 16; i = i + 4) {
		DP_DEBUG("%02x,%02x,%02x,%02x\n",
			val[i+1], val[i+2], val[i+3], val[i+4]);
	}

	secdp_debug_prefer_ratio_store(val[1]);
exit:
	return size;
}

static CLASS_ATTR_RW(dp_pref_ratio);
#endif

enum {
	DEX = 0,
	DEX_VER,
	MONITOR_INFO,
#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	DP_ERROR_INFO,
#endif
#ifdef CONFIG_SEC_FACTORY
	DP_SBU_SW_SEL,
#endif
#ifdef SECDP_SELF_TEST
	DP_SELF_TEST,
	DP_EDID,
#endif
#ifdef CONFIG_SEC_DISPLAYPORT_ENG
	DP_FORCED_RES,
	DP_UNIT_TEST,
	DP_AUX_CFG,
	DP_PRESHOOT,
	DP_VX_LVL,
	DP_PX_LVL,
	DP_VX_LVL_HBR2_HBR3,
	DP_PX_LVL_HBR2_HBR3,
	DP_VX_LVL_HBR_RBR,
	DP_PX_LVL_HBR_RBR,
	DP_PREF_SKIP,
	DP_PREF_RATIO,
#endif
};

static struct attribute *secdp_class_attrs[] = {
	[DEX]		= &class_attr_dex.attr,
	[DEX_VER]	= &class_attr_dex_ver.attr,
	[MONITOR_INFO]	= &class_attr_monitor_info.attr,
#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	[DP_ERROR_INFO] = &class_attr_dp_error_info.attr,
#endif
#ifdef CONFIG_SEC_FACTORY
	[DP_SBU_SW_SEL]	= &class_attr_dp_sbu_sw_sel.attr,
#endif
#ifdef SECDP_SELF_TEST
	[DP_SELF_TEST]	= &class_attr_dp_self_test.attr,
	[DP_EDID]	= &class_attr_dp_edid.attr,
#endif
#ifdef CONFIG_SEC_DISPLAYPORT_ENG
	[DP_FORCED_RES]	= &class_attr_dp_forced_resolution.attr,
	[DP_UNIT_TEST]	= &class_attr_dp_unit_test.attr,
	[DP_AUX_CFG]	= &class_attr_dp_aux_cfg.attr,
	[DP_PRESHOOT]	= &class_attr_dp_preshoot.attr,
	[DP_VX_LVL]	= &class_attr_dp_vx_lvl.attr,
	[DP_PX_LVL]	= &class_attr_dp_px_lvl.attr,
	[DP_VX_LVL_HBR2_HBR3]	= &class_attr_dp_vx_lvl_hbr2_hbr3.attr,
	[DP_PX_LVL_HBR2_HBR3]	= &class_attr_dp_px_lvl_hbr2_hbr3.attr,
	[DP_VX_LVL_HBR_RBR]	= &class_attr_dp_vx_lvl_hbr_rbr.attr,
	[DP_PX_LVL_HBR_RBR]	= &class_attr_dp_px_lvl_hbr_rbr.attr,
	[DP_PREF_SKIP]	= &class_attr_dp_pref_skip.attr,
	[DP_PREF_RATIO]	= &class_attr_dp_pref_ratio.attr,
#endif
	NULL,
};
ATTRIBUTE_GROUPS(secdp_class);

struct secdp_sysfs *secdp_sysfs_init(void)
{
	struct class *dp_class;
	struct secdp_sysfs *sysfs;
	int rc = -1;

	dp_class = kzalloc(sizeof(*dp_class), GFP_KERNEL);
	if (!dp_class) {
		DP_ERR("fail to alloc sysfs->dp_class\n");
		goto err_exit;
	}

	dp_class->name = "dp_sec";
	dp_class->owner = THIS_MODULE;
	dp_class->class_groups = secdp_class_groups;

	rc = class_register(dp_class);
	if (rc < 0) {
		DP_ERR("couldn't register secdp sysfs class, rc: %d\n", rc);
		goto free_exit;
	}

	sysfs = kzalloc(sizeof(*sysfs), GFP_KERNEL);
	if (!sysfs) {
		DP_ERR("fail to alloc sysfs\n");
		goto free_exit;
	}

	sysfs->dp_class = dp_class;

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	secdp_bigdata_init(dp_class);
#endif

	DP_DEBUG("success, rc:%d\n", rc);
	return sysfs;

free_exit:
	kzfree(dp_class);
err_exit:
	return NULL;
}

void secdp_sysfs_deinit(struct secdp_sysfs *sysfs)
{
	DP_DEBUG("+++\n");

	if (sysfs) {
		if (sysfs->dp_class) {
			class_unregister(sysfs->dp_class);
			kzfree(sysfs->dp_class);
			sysfs->dp_class = NULL;
			DP_DEBUG("freeing dp_class done\n");
		}

		kzfree(sysfs);
	}
}

struct secdp_sysfs *secdp_sysfs_get(struct device *dev, struct secdp_misc *sec)
{
	int rc = 0;
	struct secdp_sysfs_private *sysfs;
	struct secdp_sysfs *dp_sysfs;

	if (!dev || !sec) {
		DP_ERR("invalid input\n");
		rc = -EINVAL;
		goto error;
	}

	sysfs = devm_kzalloc(dev, sizeof(*sysfs), GFP_KERNEL);
	if (!sysfs) {
		rc = -EINVAL;
		goto error;
	}

	sysfs->dev   = dev;
	sysfs->sec   = sec;
	dp_sysfs = &sysfs->dp_sysfs;

	g_secdp_sysfs = sysfs;

#ifdef SECDP_SELF_TEST
	INIT_DELAYED_WORK(&sec->self_test_reconnect_work,
		secdp_self_test_reconnect_work);
	INIT_DELAYED_WORK(&sec->self_test_hdcp_test_work,
		secdp_self_test_hdcp_test_work);
#endif
	return dp_sysfs;

error:
	return ERR_PTR(rc);
}

void secdp_sysfs_put(struct secdp_sysfs *dp_sysfs)
{
	struct secdp_sysfs_private *sysfs;

	if (!dp_sysfs)
		return;

	sysfs = container_of(dp_sysfs, struct secdp_sysfs_private, dp_sysfs);
	devm_kfree(sysfs->dev, sysfs);

	g_secdp_sysfs = NULL;
}
