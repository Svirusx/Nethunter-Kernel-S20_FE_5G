/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: jb09.kim
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2020, Samsung Electronics. All rights reserved.

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

#include "ss_dsi_panel_common.h"
#include <linux/preempt.h>
#include <linux/sec_param.h>
#include <linux/input/sec_cmd.h>

static void ss_panel_recovery(struct samsung_display_driver_data *vdd);
static void ss_event_osc_te_fitting(
		struct samsung_display_driver_data *vdd, int event, void *arg);
static irqreturn_t samsung_te_check_handler(int irq, void *handle);
static void samsung_te_check_done_work(struct work_struct *work);
static void ss_event_esd_recovery_init(
		struct samsung_display_driver_data *vdd, int event, void *arg);
static void samsung_display_delay_disp_on_work(struct work_struct *work);
static void read_panel_data_work_fn(struct work_struct *work);

struct samsung_display_driver_data vdd_data[MAX_DISPLAY_NDX];

/* disable pll ssc */
int vdd_pll_ssc_disabled;

void __iomem *virt_mmss_gp_base;

static DEFINE_MUTEX(dyn_mipi_clock);

LIST_HEAD(vdds_list);

char *brr_mode_name[MAX_ALL_BRR_MODE] = {
	"BRR_48_60",
	"BRR_60_48",
	"BRR_96_120",
	"BRR_120_96",
	"BRR_60HS_120",
	"BRR_120_60HS",
	"BRR_60HS_96",
	"BRR_96_60HS",
	"MAX_BRR_ON_MODE",
	"BRR_STOP_MODE",
	"BRR_OFF_MODE",
};

char ss_panel_id0_get(struct samsung_display_driver_data *vdd)
{
	return (vdd->manufacture_id_dsi & 0xFF0000) >> 16;
}

char ss_panel_id1_get(struct samsung_display_driver_data *vdd)
{
	return (vdd->manufacture_id_dsi & 0xFF00) >> 8;
}

char ss_panel_id2_get(struct samsung_display_driver_data *vdd)
{
	return vdd->manufacture_id_dsi & 0xFF;
}

char ss_panel_rev_get(struct samsung_display_driver_data *vdd)
{
	return vdd->manufacture_id_dsi & 0x0F;
}

int ss_panel_attach_get(struct samsung_display_driver_data *vdd)
{
	return vdd->panel_attach_status;
}

int ss_panel_attach_set(struct samsung_display_driver_data *vdd, bool attach)
{
	/* 0bit->DSI0 1bit->DSI1 */
	/* check the lcd id for DISPLAY_1 and DISPLAY_2 */
	if (likely(ss_panel_attached(vdd->ndx) && attach))
		vdd->panel_attach_status = true;
	else
		vdd->panel_attach_status = false;

	LCD_INFO("panel_attach_status : %d\n", vdd->panel_attach_status);

	return vdd->panel_attach_status;
}

/*
 * Check the lcd id for DISPLAY_1 and DISPLAY_2 using the ndx
 */
int ss_panel_attached(int ndx)
{
	int lcd_id = 0;

	/*
	 * ndx 0 means DISPLAY_1 and ndx 1 means DISPLAY_2
	 */
	if (ndx == PRIMARY_DISPLAY_NDX)
		lcd_id = get_lcd_attached("GET");
	else if (ndx == SECONDARY_DISPLAY_NDX)
		lcd_id = get_lcd_attached_secondary("GET");

	/*
	 * The 0xFFFFFF is the id for PBA booting
	 * if the id is same with 0xFFFFFF, this function
	 * will return 0
	 */
	return !(lcd_id == PBA_ID);
}

static int ss_parse_panel_id(char *panel_id)
{
	char *pt;
	int lcd_id = 0;

	if (!IS_ERR_OR_NULL(panel_id))
		pt = panel_id;
	else
		return lcd_id;

	for (pt = panel_id; *pt != 0; pt++)  {
		lcd_id <<= 4;
		switch (*pt) {
		case '0' ... '9':
			lcd_id += *pt - '0';
			break;
		case 'a' ... 'f':
			lcd_id += 10 + *pt - 'a';
			break;
		case 'A' ... 'F':
			lcd_id += 10 + *pt - 'A';
			break;
		}
	}

	return lcd_id;
}

int get_lcd_attached(char *mode)
{
	static int lcd_id = -EINVAL;

	LCD_DEBUG("%s", mode);

	if (mode == NULL)
		return true;

	if (!strncmp(mode, "GET", 3))
		goto end;
	else
		lcd_id = ss_parse_panel_id(mode);

	LCD_ERR("LCD_ID = 0x%X\n", lcd_id);
end:

	/* case 03830582: Sometimes, bootloader fails to save lcd_id value in cmdline
	 * which is located in TZ.
	 * W/A: if panel name is PBA panel while lcd_id is not PBA ID, force to set lcd_id to PBA ID.
	 */
	if (unlikely(lcd_id == -EINVAL)) {
		char panel_name[MAX_CMDLINE_PARAM_LEN];
		char panel_string[] = "ss_dsi_panel_PBA_BOOTING_FHD";

		ss_get_primary_panel_name_cmdline(panel_name);
		if (!strncmp(panel_string, panel_name, strlen(panel_string)) && lcd_id != PBA_ID) {
			LCD_INFO("pba panel name, force lcd id: 0x%X -> 0xFFFFFF\n", lcd_id);
			lcd_id =  PBA_ID;
		}
	}

	return lcd_id;
}
EXPORT_SYMBOL(get_lcd_attached);
__setup("lcd_id=0x", get_lcd_attached);

int get_lcd_attached_secondary(char *mode)
{
	static int lcd_id = -EINVAL;

	LCD_DEBUG("%s", mode);

	if (mode == NULL)
		return true;

	if (!strncmp(mode, "GET", 3))
		goto end;
	else
		lcd_id = ss_parse_panel_id(mode);

	LCD_ERR("LCD_ID = 0x%X\n", lcd_id);
end:

	/* case 03830582: Sometimes, bootloader fails to save lcd_id value in cmdline
	 * which is located in TZ.
	 * W/A: if panel name is PBA panel while lcd_id is not PBA ID, force to set lcd_id to PBA ID.
	 */
	if (unlikely(lcd_id == -EINVAL)) {
		char panel_name[MAX_CMDLINE_PARAM_LEN];
		char panel_string[] = "ss_dsi_panel_PBA_BOOTING_FHD_DSI1";

		ss_get_secondary_panel_name_cmdline(panel_name);
		if (!strncmp(panel_string, panel_name, strlen(panel_string)) && lcd_id != PBA_ID) {
			LCD_INFO("pba panel name, force lcd id: 0x%X -> 0xFFFFFF\n", lcd_id);
			lcd_id =  PBA_ID;
		}
	}

	return lcd_id;
}
EXPORT_SYMBOL(get_lcd_attached_secondary);
__setup("lcd_id1=0x", get_lcd_attached_secondary);

int get_window_color(char *color)
{
	struct samsung_display_driver_data *vdd;
	int i;

	LCD_ERR("window_color from cmdline : %s\n", color);

	for (i = PRIMARY_DISPLAY_NDX; i < MAX_DISPLAY_NDX; i++) {
		vdd = ss_get_vdd(i);
		memcpy(vdd->window_color, color, sizeof(vdd->window_color));
	}

	LCD_ERR("window_color : %s\n", vdd->window_color);

	return true;
}
EXPORT_SYMBOL(get_window_color);
__setup("window_color=", get_window_color);

static int ss_is_recovery_check(char *str)
{
	struct samsung_display_driver_data *vdd;
	int i;

	for (i = PRIMARY_DISPLAY_NDX; i < MAX_DISPLAY_NDX; i++) {
		vdd = ss_get_vdd(i);
		if (strncmp(str, "1", 1) == 0)
			vdd->is_recovery_mode = 1;
		else
			vdd->is_recovery_mode = 0;
	}
	LCD_INFO("vdd->is_recovery_mode = %d\n", vdd->is_recovery_mode);
	return true;
}
EXPORT_SYMBOL(ss_is_recovery_check);
__setup("androidboot.boot_recovery=", ss_is_recovery_check);
static void ss_event_frame_update_pre(
		struct samsung_display_driver_data *vdd, void *arg)
{
	/* mAFPC */
	if (vdd->mafpc.is_support) {
		if (vdd->mafpc.need_to_write) {
			LCD_INFO("update mafpc\n");

			if (!IS_ERR_OR_NULL(vdd->mafpc.img_write))
				vdd->mafpc.img_write(vdd, false);
			else
				LCD_ERR("mafpc img_write function is NULL\n");

			if (vdd->mafpc.en)
				vdd->mafpc.enable(vdd, true);
			else
				vdd->mafpc.enable(vdd, false);

			vdd->mafpc.need_to_write = false;
		}
	}

	/* Block frame update during exclusive mode on.*/
	if (unlikely(vdd->exclusive_tx.enable)) {
		if (unlikely(!vdd->exclusive_tx.permit_frame_update)) {
			LCD_INFO("exclusive mode on, stop frame update\n");
			wait_event(vdd->exclusive_tx.ex_tx_waitq,
				!vdd->exclusive_tx.enable);
			LCD_INFO("continue frame update\n");
		}
	}
}

static void ss_event_frame_update(
		struct samsung_display_driver_data *vdd, int event, void *arg)
{
	struct panel_func *panel_func = NULL;
	static u8 frame_count = 1;
	s64 cur_time_64;
	int wait_time_32;
	s64 wait_time_64;
	int sleep_out_delay_32;
	s64 sleep_out_delay_64;

	panel_func = &vdd->panel_func;

	sleep_out_delay_32 = vdd->dtsi_data.samsung_wait_after_sleep_out_delay;
	sleep_out_delay_64 = (s64)sleep_out_delay_32;

	cur_time_64 = ktime_to_ms(ktime_get());
	LCD_DEBUG("[DISPLAY_%d] ss_event_frame_update\n", vdd->ndx);

	if (gpio_is_valid(vdd->dtsi_data.samsung_tcon_rdy_gpio))
		wait_time_64 = sleep_out_delay_64 - (cur_time_64 - vdd->reset_time_64);
	else
		wait_time_64 = sleep_out_delay_64 - (cur_time_64 - vdd->sleep_out_time_64);

	/* To protect 64bit overflow & underflow */
	if (wait_time_64 <= 0)
		wait_time_32 = 0;
	else if (wait_time_64 > sleep_out_delay_64)
		wait_time_32 = sleep_out_delay_32;
	else
		wait_time_32 = (s32)wait_time_64;

	panel_func = &vdd->panel_func;

	if (vdd->dtsi_data.samsung_osc_te_fitting &&
			!(vdd->te_fitting_info.status & TE_FITTING_DONE)) {
		if (panel_func->ss_event_osc_te_fitting)
			panel_func->ss_event_osc_te_fitting(vdd, event, arg);
	}

	if (vdd->display_status_dsi.wait_disp_on) {
		/* TODO: Add condition to check the susupend state
		 * insted of !msm_is_suspend_state(GET_DRM_DEV(vdd))
		 */

		/* Skip a number of frames to avoid garbage image output from wakeup */
		if (frame_count <= vdd->dtsi_data.samsung_delayed_display_on) {
			LCD_DEBUG("Skip %d frame\n", frame_count);
			frame_count++;
			goto skip_display_on;
		}
		frame_count = 1;

		if (vdd->dtsi_data.samsung_wait_after_sleep_out_delay) {
			if (wait_time_32 > 0) {
				LCD_ERR("sleep_out_delay:%d sleep_out_t:%llu cur_t:%llu wait_t:%d [ms] start\n", sleep_out_delay_32,
					  vdd->sleep_out_time_64, cur_time_64, wait_time_32);
				usleep_range(wait_time_32*1000, wait_time_32*1000);
				LCD_ERR("wait_t: %d end\n", wait_time_32);
			} else
				LCD_ERR("sleep_out_delay:%d sleep_out_t:%llu cur_t:%llu wait_t:%d [ms] skip\n", sleep_out_delay_32,
					  vdd->sleep_out_time_64, cur_time_64, wait_time_32);
		}

		/* insert black frame using self_grid on cmd in case of LPM mode */
		if (vdd->panel_lpm.need_self_grid && ss_is_panel_lpm(vdd)) {
			ss_send_cmd(vdd, TX_SELF_GRID_ON);
			LCD_ERR("self_grid [ON]\n");
		}

		ss_send_cmd(vdd, TX_DISPLAY_ON);
		/* Write clear status bits if first booting for GTS7+ */
		/* UUM1 ddi restriction for GTS7+ */
		if (vdd->display_status_dsi.first_commit_disp_on) {
			if (!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_FIRST_DISPLAY_ON))) {
				usleep_range(18000,18100);
				ss_send_cmd(vdd, TX_FIRST_DISPLAY_ON);
			}
			vdd->display_status_dsi.first_commit_disp_on = false;
		}

		vdd->display_status_dsi.wait_disp_on = false;

		if (vdd->panel_lpm.need_self_grid && ss_is_panel_lpm(vdd)) {
			/* should distinguish display_on and self_grid_off with vsnyc */
			/* delay 34ms in self_grid_off_revA cmds */
			ss_send_cmd(vdd, TX_SELF_GRID_OFF);
			LCD_ERR("self_grid [OFF]\n");
		}

		vdd->panel_lpm.need_self_grid = false;

		if (vdd->panel_func.samsung_backlight_late_on)
			vdd->panel_func.samsung_backlight_late_on(vdd);

		if (vdd->dtsi_data.hmt_enabled &&
				ss_is_panel_on(vdd)) {
			if (vdd->br_info.hmt_stat.hmt_on) {
				LCD_INFO("hmt reset ..\n");
				#if 0
				vdd->br_info.hmt_stat.hmt_enable(vdd);
				vdd->br_info.hmt_stat.hmt_reverse_update(vdd, 1);
				vdd->br_info.hmt_stat.hmt_bright_update(vdd);
				#endif
			}
		}

		if (ss_is_esd_check_enabled(vdd))
			vdd->esd_recovery.is_enabled_esd_recovery = true;

		/* For read flash gamma data before first brightness set */
		LCD_INFO("flash_gamma_init_done: %d, flash_gamma_support: %d\n", vdd->br_info.flash_gamma_init_done, vdd->br_info.flash_gamma_support);
		if (vdd->br_info.flash_gamma_support && !vdd->br_info.flash_gamma_init_done) {
			if (!work_busy(&vdd->br_info.flash_br_work.work)) {
				queue_delayed_work(vdd->br_info.flash_br_workqueue, &vdd->br_info.flash_br_work, msecs_to_jiffies(0));
			}
		}

		LCD_INFO("[DISPLAY_%d] DISPLAY_ON\n", vdd->ndx);
	}

	/* copr - check actual display on (frame - copr > 0) debug */
	/* work thread will be stopped if copr is over 0 */
	if (vdd->display_status_dsi.wait_actual_disp_on && ss_is_panel_on(vdd)) {
		if (vdd->copr.read_copr_wq && vdd->copr.copr_on)
			queue_work(vdd->copr.read_copr_wq, &vdd->copr.read_copr_work);
	}
skip_display_on:
	return;
}

static void ss_send_esd_recovery_cmd(struct samsung_display_driver_data *vdd)
{
	static bool toggle;

	if (toggle)
		ss_send_cmd(vdd, TX_ESD_RECOVERY_1);
	else
		ss_send_cmd(vdd, TX_ESD_RECOVERY_2);
	toggle = !toggle;
}

static void ss_check_te(struct samsung_display_driver_data *vdd)
{
	unsigned int disp_te_gpio;
	int rc, te_count = 0;
	int te_max = 20000; /*sampling 200ms */

	disp_te_gpio = ss_get_te_gpio(vdd);

	LCD_INFO("============ start waiting for TE CTRL%d ============\n", vdd->ndx);
	if (gpio_is_valid(disp_te_gpio)) {
		for (te_count = 0 ; te_count < te_max ; te_count++) {
			rc = gpio_get_value(disp_te_gpio);
			if (rc == 1) {
				LCD_INFO("LDI generate TE within = %d.%dms\n", te_count/100, te_count%100);
				break;
			}
			/* usleep suspends the calling thread whereas udelay is a
			 * busy wait. Here the value of te_gpio is checked in a loop of
			 * max count = 250. If this loop has to iterate multiple
			 * times before the te_gpio is 1, the calling thread will end
			 * up in suspend/wakeup sequence multiple times if usleep is
			 * used, which is an overhead. So use udelay instead of usleep.
			 */
			udelay(10);
		}
		if (te_count == te_max) {
			LCD_ERR("LDI doesn't generate TE");
			SS_XLOG(0xbad);
			inc_dpui_u32_field(DPUI_KEY_QCT_NO_TE, 1);
		}
	} else
		LCD_ERR("disp_te_gpio is not valid\n");
	LCD_INFO("============ end waiting for TE CTRL%d ============\n", vdd->ndx);
}
/* SAMSUNG_FINGERPRINT */
/* TODO: replace with sde_encoder_wait_for_event(drm_enc, MSM_ENC_VBLANK) */
static void ss_wait_for_te_gpio(struct samsung_display_driver_data *vdd, int num_of_te, int delay_after_te)
{
	unsigned int disp_te_gpio;
	int rc, te_count = 0;
	int te_max = 20000; /*sampling 100ms */
	int iter;
	s64 start_time_1_64, start_time_3_64;

	preempt_disable();
	disp_te_gpio = ss_get_te_gpio(vdd);
	for(iter = 0 ; iter < num_of_te ; iter++) {
		start_time_1_64 = ktime_to_us(ktime_get());
		if (gpio_is_valid(disp_te_gpio)) {
			for (te_count = 0 ; te_count < te_max ; te_count++) {
				rc = gpio_get_value(disp_te_gpio);
				if (rc == 1) {
					start_time_3_64 = ktime_to_us(ktime_get());
					LCD_ERR("ss_wait_for_te_gpio  = %llu\n", start_time_3_64- start_time_1_64);
					break;
				}
				ndelay(5000);
			}
		}
		if (te_count == te_max)
			LCD_ERR("LDI doesn't generate TE");

		if (in_atomic())
			udelay(200);
		else
			usleep_range(200, 220);
	}
	udelay(delay_after_te);
	preempt_enable();
}

/* SAMSUNG_FINGERPRINT */
void ss_send_hbm_fingermask_image_tx(struct samsung_display_driver_data *vdd, bool on)
{
	LCD_INFO("[DISPLAY_%d] ++ %s\n",vdd->ndx, on?"on":"off");
	if (on) {
		ss_brightness_dcs(vdd, 0, BACKLIGHT_FINGERMASK_ON);
	} else {
		ss_brightness_dcs(vdd, 0, BACKLIGHT_FINGERMASK_OFF);
	}
	LCD_INFO("[DISPLAY_%d] --\n", vdd->ndx);
}

static void ss_event_fb_event_callback(struct samsung_display_driver_data *vdd, int event, void *arg)
{
	struct panel_func *panel_func = NULL;

	panel_func = &vdd->panel_func;

	if (IS_ERR_OR_NULL(panel_func)) {
		LCD_ERR("Invalid data panel_func : 0x%zx\n",
				(size_t)panel_func);
		return;
	}

	if (panel_func->ss_event_esd_recovery_init)
		panel_func->ss_event_esd_recovery_init(vdd, event, arg);
}

static int _ss_dsi_panel_event_handler(
		struct samsung_display_driver_data *vdd,
		enum mdss_intf_events event, void *arg)
{
	struct panel_func *panel_func = NULL;

	panel_func = &vdd->panel_func;

	if (IS_ERR_OR_NULL(panel_func)) {
		LCD_ERR("Invalid data panel_func : 0x%zx\n", (size_t)panel_func);
		return -EINVAL;
	}

	switch (event) {
	case SS_EVENT_FRAME_UPDATE_POST:
		if (!IS_ERR_OR_NULL(panel_func->ss_event_frame_update))
			panel_func->ss_event_frame_update(vdd, event, arg);
		break;
	case SS_EVENT_FRAME_UPDATE_PRE:
		ss_event_frame_update_pre(vdd, arg);
		break;
	case SS_EVENT_FB_EVENT_CALLBACK:
		if (!IS_ERR_OR_NULL(panel_func->ss_event_fb_event_callback))
			panel_func->ss_event_fb_event_callback(vdd, event, arg);
		break;
	case SS_EVENT_PANEL_ON:
		if (likely(!vdd->is_factory_mode))
			ss_panel_lpm_ctrl(vdd, false);
		break;
	case SS_EVENT_PANEL_OFF:
		if (likely(!vdd->is_factory_mode))
			ss_panel_lpm_ctrl(vdd, true);
		break;
	case SS_EVENT_PANEL_RECOVERY:
		ss_panel_recovery(vdd);
		break;
	case SS_EVENT_PANEL_ESD_RECOVERY:
		if (vdd->esd_recovery.send_esd_recovery)
			ss_send_esd_recovery_cmd(vdd);
		break;
	case SS_EVENT_CHECK_TE:
		ss_check_te(vdd);
		break;
	case SS_EVENT_SDE_HW_CATALOG_INIT:
		if (unlikely(vdd->panel_func.samsung_pba_config))
			vdd->panel_func.samsung_pba_config(vdd, arg);
		break;
	default:
		LCD_DEBUG("unhandled event=%d\n", event);
		break;
	}

	return 0;
}

int ss_dsi_panel_event_handler(
		int display_ndx, enum mdss_intf_events event, void *arg)
{

	struct samsung_display_driver_data *vdd = NULL;

	if (display_ndx >= MAX_DISPLAY_NDX || display_ndx < 0)
		return -EINVAL;

	vdd = ss_get_vdd(display_ndx);

	if (unlikely(!vdd))
		return -EINVAL;

	return _ss_dsi_panel_event_handler(vdd, event, arg);
}

/* Identify VRR HS by drm_mode's name.
 * drm_mode's name is defined by dsi_mode->timing.sot_hs_mode parsed
 * from samsung,mdss-dsi-sot-hs-mode in panel dtsi file.
 * ex) drm_mode->name is "1080x2316x60x193345cmdHS" for HS mode.
 *     drm_mode->name is "1080x2316x60x193345cmdNS" for NS mode.
 * To use this feature, declare different porch between HS and NS modes,
 * in panel dtsi file.
 */
bool ss_is_sot_hs_from_drm_mode(const struct drm_display_mode *drm_mode)
{
	int len;
	bool sot_hs = false;

	if (!drm_mode) {
		LCD_ERR("invalid drm_mode\n");
		goto end;
	}

	len = strlen(drm_mode->name);
	if (len >= DRM_DISPLAY_MODE_LEN || len <= 0) {
		LCD_ERR("invalid drm_mode name len (%d)\n", len);
		goto end;
	}

	if (!strncmp(drm_mode->name + len - 2, "HS", 2))
		sot_hs = true;
	else
		sot_hs = false;

end:
	return sot_hs;
}

/*
 * Determine LFD min and max freq by client's request, and priority of each clients.
 *
 * FACTORY	: factory test scenario.
 *		  determine min and max freq., priority = critical
 * TSP LPM	: tsp lpm mode scenario.
 * 		  determine min freq., priority = high
 * INPUT	: touch press scenario.
 * 		  determine min freq., priority = high
 * DISPLAY MANAGER : HBM (1hz) and low brighntess (LFD off) scenario.
 * 		     determine min freq., priority = high
 * VIDEO DETECT	: video detect scenario.
 * 		  determine max freq., priority = high
 * AOD		: AOD scenario.
 * 		  determine min freq. in lpm: handle thi in lpm brightenss, not here.
 *
 * ss_get_lfd_div should be called followed by ss_update_base_lfd_val in panel function....
 * or keep it in ss_vrr...
 */
int ss_get_lfd_div(struct samsung_display_driver_data *vdd,
		enum LFD_SCOPE_ID scope, u32 *out_min_div, u32 *out_max_div)
{
	struct vrr_info *vrr = &vdd->vrr;
	struct lfd_mngr *mngr;
	u32 max_div, min_div, max_div_def, min_div_def, min_div_lowest;
	u32 min_div_clear;
	u32 min_div_scal;
	int i;
	u32 max_freq, min_freq;
	u32 base_rr;
	struct lfd_base_str lfd_base;

	/* DEFAULT */
	if (IS_ERR_OR_NULL(vdd->panel_func.samsung_lfd_get_base_val)) {
		LCD_ERR("fail to get lfd bsae val..\n");
		return -ENODEV;
	}

	vdd->panel_func.samsung_lfd_get_base_val(vrr, scope, &lfd_base);

	base_rr = lfd_base.base_rr;
	max_div_def = lfd_base.max_div_def;
	min_div_def = lfd_base.min_div_def;
	min_div_lowest = lfd_base.min_div_lowest;
	min_div_clear = min_div_lowest + 1;

	/* FIX */
	for (i = 0, mngr = &vrr->lfd.lfd_mngr[i]; i < LFD_CLIENT_MAX; i++, mngr++) {
		if (mngr->fix[scope] == LFD_FUNC_FIX_LOW &&
				i == LFD_CLIENT_FAC) {
			max_div = min_div = min_div_def;
			LCD_INFO("FIX: (FAC): fix low\n");
			goto done;
		} else if (mngr->fix[scope] == LFD_FUNC_FIX_HIGH) {
			max_div = min_div = max_div_def;
			LCD_INFO("FIX: LFD off, client: %s\n", lfd_client_name[i]);
			goto done;
		}
	}

	/* MAX and MIN */
	max_freq = 0;
	min_freq = 0;
	for (i = 0, mngr = &vrr->lfd.lfd_mngr[i]; i < LFD_CLIENT_MAX; i++, mngr++) {
		if (mngr->max[scope] != LFD_FUNC_MAX_CLEAR) {
			max_freq = max(max_freq, mngr->max[scope]);
			LCD_INFO("client: %s, max freq=%dhz -> %dhz\n",
				lfd_client_name[i], mngr->max[scope], max_freq);
		}

		if (mngr->min[scope] != LFD_FUNC_MIN_CLEAR) {
			min_freq = max(min_freq, mngr->min[scope]);
			LCD_INFO("client: %s, min freq=%dhz -> %dhz\n",
				lfd_client_name[i], mngr->min[scope], min_freq);
		}
	}
	if (max_freq == 0)
		max_div = max_div_def;
	else
		max_div = base_rr / max_freq; // round down in case of freq -> div

	if (min_freq == 0)
		min_div = min_div_clear; // choose min_div_clear instead of min_div_def to allow min_div_lowest below sequence..
	else
		min_div = base_rr / min_freq; // round down in case of freq -> div

	min_div = max(min_div, max_div);

	/* SCALABILITY: handle min_freq.
	 * This function is depends on panel. So use panel callback function
	 * if below fomula doesn't fit for your panel.
	 */
	for (i = 0, mngr = &vrr->lfd.lfd_mngr[i]; i < LFD_CLIENT_MAX; i++, mngr++) {
		/* TODO: get this panel specific data from lfd->... parsed by panel dtsi */
		if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY1) /* under 40lux, LFD off */
			min_div_scal = 1;
		else if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY2) /* touch press/release, div=2 */
		      min_div_scal = 2;
		else if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY3) /* TBD, div=4 */
		      min_div_scal = 4;
		else if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY4) /* TBD, div=5 */
		      min_div_scal = 5;
		else if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY6) /* over 7400lux (HBM), LFD min 1hz */
			min_div_scal = min_div_lowest;
		else /* clear */
			min_div_scal = min_div_clear;

		if (mngr->scalability[scope] != LFD_FUNC_SCALABILITY0 &&
				mngr->scalability[scope] != LFD_FUNC_SCALABILITY5)
			LCD_INFO("client: %s, scalability: %d\n",
					lfd_client_name[i], mngr->scalability[scope]);

		min_div = min(min_div, min_div_scal);
	}

	if (min_div == min_div_clear)
		min_div = min_div_def;

done:
	/* check limitation
	 * High LFD frequency means stable display quality.
	 * Low LFD frequency means low power consumption.
	 * Those are trade-off, and if there are multiple requests of LFD min/max frequency setting,
	 * choose most highest LFD freqency, means most lowest  for display quality.
	 */
	if (max_div > min_div_def) {
		LCD_INFO("raise too low max freq %dhz (max_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, max_div),
				max_div, min_div_def);
		max_div = min_div_def;
	}

	if (max_div < max_div_def) {
		LCD_INFO("lower too high max freq %dhz (max_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, max_div),
				max_div, max_div_def);
		max_div = max_div_def;
	}

	if (min_div > min_div_def && min_div != min_div_lowest) {
		LCD_INFO("raisetoo low min freq %dhz (min_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, min_div),
				min_div, min_div_def);
		min_div = min_div_def;
	}

	if (min_div < max_div_def) {
		LCD_INFO("lower too high min freq %dhz (min_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, min_div),
				min_div, max_div_def);
		min_div = max_div_def;
	}

	if (min_div < max_div) {
		LCD_ERR("raise max freq(%dhz), lower than min freq(%dhz) (max_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, max_div),
				DIV_ROUND_UP(base_rr, min_div),
				max_div, min_div);
		max_div = min_div;
	}

	*out_min_div = min_div;
	*out_max_div = max_div;

	LCD_INFO("LFD: cur: %dHZ@%s, base_rr: %uhz, def: %uhz(%u)~%uhz(%u), " \
			"lowest: %uhz(%u), result: %uhz(%u)~%uhz(%u)\n",
			vrr->cur_refresh_rate,
			vrr->cur_sot_hs_mode ? "HS" : "NS",
			base_rr,
			DIV_ROUND_UP(base_rr, min_div_def), min_div_def,
			DIV_ROUND_UP(base_rr, max_div_def), max_div_def,
			DIV_ROUND_UP(base_rr, min_div_lowest), min_div_lowest,
			DIV_ROUND_UP(base_rr, min_div), min_div,
			DIV_ROUND_UP(base_rr, max_div), max_div);

	return 0;
}



static void ss_lfd_touch_work(struct work_struct *work)
{
	struct lfd_info *lfd = container_of(work, struct lfd_info, lfd_touch_work);
	struct vrr_info *vrr = container_of(lfd, struct vrr_info, lfd);
	struct samsung_display_driver_data *vdd = container_of(vrr,
				struct samsung_display_driver_data, vrr);

	ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
}

static int ss_lfd_touch_notify_cb(struct notifier_block *nb,
				unsigned long val, void *v)
{
	struct lfd_info *lfd;
	struct samsung_display_driver_data *vdd;
	struct lfd_mngr *mngr;
	sec_input_notify_t event = val;
	struct sec_input_notify_data *tsp_ndx = v;


	if (event != NOTIFIER_LCD_VRR_LFD_LOCK_REQUEST &&
		event != NOTIFIER_LCD_VRR_LFD_LOCK_RELEASE)
		goto done;

	vdd = ss_get_vdd(tsp_ndx->dual_policy);
	if (!vdd->vrr.lfd.support_lfd) {
		LCD_INFO("[DISPLAY_%d] not support lfd\n", vdd->ndx);
		goto done;
	}

	lfd = &vdd->vrr.lfd;
	mngr = &lfd->lfd_mngr[LFD_CLIENT_INPUT];

	if (event == NOTIFIER_LCD_VRR_LFD_LOCK_REQUEST) {
		mngr->scalability[LFD_SCOPE_NORMAL] = LFD_FUNC_SCALABILITY2; /* div=2*/
		LCD_INFO("touch: control LFD\n");
		if (ss_is_panel_on(vdd))
			queue_work(lfd->lfd_touch_wq, &lfd->lfd_touch_work);
		else
			LCD_INFO("panel is not normal on(%d), delay applying\n",
					vdd->panel_state);
	} else if (event == NOTIFIER_LCD_VRR_LFD_LOCK_RELEASE) {
		mngr->scalability[LFD_SCOPE_NORMAL] = LFD_FUNC_SCALABILITY0;
		LCD_INFO("touch LFD release LFD\n");
		if (ss_is_panel_on(vdd))
			queue_work(lfd->lfd_touch_wq, &lfd->lfd_touch_work);
		else
			LCD_INFO("panel is not normal on(%d), delay applying\n",
					vdd->panel_state);
	}
done:
	return NOTIFY_DONE;
}

int ss_find_dyn_mipi_clk_timing_idx(struct samsung_display_driver_data *vdd)
{
	int idx = -EINVAL;
	int loop;
	int rat, band, arfcn;
	struct clk_sel_table sel_table = vdd->dyn_mipi_clk.clk_sel_table;

	if (!sel_table.tab_size) {
		LCD_ERR("Table is NULL");
		return -ENOENT;
	}

	rat = vdd->dyn_mipi_clk.rf_info.rat;
	band = vdd->dyn_mipi_clk.rf_info.band;
	arfcn = vdd->dyn_mipi_clk.rf_info.arfcn;

	for (loop = 0 ; loop < sel_table.tab_size ; loop++) {
		if ((rat == sel_table.rat[loop]) && (band == sel_table.band[loop])) {
			if ((arfcn >= sel_table.from[loop]) && (arfcn <= sel_table.end[loop])) {
				idx = sel_table.target_clk_idx[loop];
				break;
			}
		}
	}

	if (vdd->dyn_mipi_clk.force_idx) {
		idx = vdd->dyn_mipi_clk.force_idx;
		LCD_INFO("use force index = %d\n", idx);
	} else {
		LCD_INFO("RAT(%d), BAND(%d), ARFCN(%d), Clock Index(%d)\n",
				rat, band, arfcn, idx);
	}

	return idx;
}

/* CP notity format (HEX raw format)
 * 10 00 AA BB 27 01 03 XX YY YY YY YY ZZ ZZ ZZ ZZ
 *
 * 00 10 (0x0010) - len
 * AA BB - not used
 * 27 - MAIN CMD (SYSTEM CMD : 0x27)
 * 01 - SUB CMD (CP Channel Info : 0x01)
 * 03 - NOTI CMD (0x03)
 * XX - RAT MODE
 * YY YY YY YY - BAND MODE
 * ZZ ZZ ZZ ZZ - FREQ INFO
 */

int ss_rf_info_notify_callback(struct notifier_block *nb,
				unsigned long size, void *data)
{
	struct dyn_mipi_clk *dyn_mipi_clk = container_of(nb, struct dyn_mipi_clk, notifier);
	struct samsung_display_driver_data *vdd =
		container_of(dyn_mipi_clk, struct samsung_display_driver_data, dyn_mipi_clk);

	struct dev_ril_bridge_msg *msg;
	struct rf_info rf_info_backup;
	int idx;

	msg = (struct dev_ril_bridge_msg *)data;

	LCD_INFO("RIL noti: ndx: %d, size: %lu, dev_id: %d, len: %d\n",
			vdd->ndx, size, msg->dev_id, msg->data_len);

	if (msg->dev_id == IPC_SYSTEM_CP_CHANNEL_INFO // #define IPC_SYSTEM_CP_CHANNEL_INFO	0x01
			&& msg->data_len == sizeof(struct rf_info)) {
		mutex_lock(&dyn_mipi_clk->dyn_mipi_lock);

		/* backup currrent rf_info */
		memcpy(&rf_info_backup, &dyn_mipi_clk->rf_info, sizeof(struct rf_info));
		memcpy(&dyn_mipi_clk->rf_info, msg->data, sizeof(struct rf_info));

		/* check data is valid */
		idx = ss_find_dyn_mipi_clk_timing_idx(vdd);
		if (idx < 0) {
			LCD_ERR("Failed to find MIPI clock timing (%d), restore previous rf_info..\n", idx);
			memcpy(&dyn_mipi_clk->rf_info, &rf_info_backup, sizeof(struct rf_info));
			mutex_unlock(&dyn_mipi_clk->dyn_mipi_lock);
			goto err;
		}

		mutex_unlock(&dyn_mipi_clk->dyn_mipi_lock);

		queue_work(dyn_mipi_clk->change_clk_wq, &dyn_mipi_clk->change_clk_work);

		LCD_INFO("RIL noti: RAT(%d), BAND(%d), ARFCN(%d)\n",
				dyn_mipi_clk->rf_info.rat,
				dyn_mipi_clk->rf_info.band,
				dyn_mipi_clk->rf_info.arfcn);
	}

	return NOTIFY_DONE;
err:
	return NOTIFY_BAD;
}

/* refer to dsi_display_set_mode_sub() */
extern int dsi_display_dynamic_clk_configure_cmd(struct dsi_display *display, int clk_rate);
static int _ss_change_dyn_mipi_clk_timing(struct samsung_display_driver_data *vdd, int clk_rate)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);

	LCD_INFO("[DISPLAY_%d] +++: clk_rate: %d, cached_clk_rate: %d\n",
			vdd->ndx, clk_rate, display->cached_clk_rate);

	mutex_lock(&display->display_lock);
	dsi_display_dynamic_clk_configure_cmd(display, clk_rate);
	mutex_unlock(&display->display_lock);

	LCD_INFO("[DISPLAY_%d] ---: clk_rate: %d\n", vdd->ndx, clk_rate);

	return 0;
}

int ss_change_dyn_mipi_clk_timing(struct samsung_display_driver_data *vdd)
{
	struct clk_timing_table timing_table = vdd->dyn_mipi_clk.clk_timing_table;
	int idx;
	int clk_rate;
	int timeout = 60; // 60 seconds
	int ret = 0;

	if (!vdd->dyn_mipi_clk.is_support) {
		LCD_ERR("Dynamic MIPI Clock does not support\n");
		return -ENODEV;
	}

	if (!timing_table.tab_size) {
		LCD_ERR("Table is NULL");
		return -ENODEV;
	}

	mutex_lock(&dyn_mipi_clock);

	LCD_INFO("[DISPLAY_%d] +++", vdd->ndx);

	/*  wait for dispaly init done */
	while (timeout-- && !vdd->display_status_dsi.disp_on_pre) {
		msleep(1000);
		LCD_INFO("wait time: %ds, disp_on_pre: %d\n",
				timeout, vdd->display_status_dsi.disp_on_pre);

		if (!timeout) {
			LCD_ERR("err: display init timeout...\n");
			goto err;
		}
	}

	mutex_lock(&vdd->dyn_mipi_clk.dyn_mipi_lock);
	idx = ss_find_dyn_mipi_clk_timing_idx(vdd);
	mutex_unlock(&vdd->dyn_mipi_clk.dyn_mipi_lock);
	if (idx < 0) {
		LCD_ERR("Failed to find MIPI clock timing (%d)\n", idx);
		goto err;
	}

	clk_rate = timing_table.clk_rate[idx];
	LCD_INFO("clk idx: %d, clk_rate: %d\n", idx, clk_rate);

	/* update dynamic mipi clk */
	ret = _ss_change_dyn_mipi_clk_timing(vdd, clk_rate);
	if (ret) {
		LCD_ERR("fail to change mipi clk(%d), ret:%d\n", clk_rate, ret);
		goto err;
	}

	/* update mipi global timing: skip this.. not required...  */
err:
	LCD_INFO("[DISPLAY_%d] ---", vdd->ndx);
	mutex_unlock(&dyn_mipi_clock);

	return 0;
}

#define FFC_REG	(0xC5)
int ss_dyn_mipi_clk_tx_ffc(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *ffc_set;
	struct dsi_panel_cmd_set *dyn_ffc_set;
	struct dsi_panel_cmd_set *cmd_list[1];
	static int reg_list[1][2] = { {FFC_REG, -EINVAL} };
	int pos_ffc;
	int idx;

	mutex_lock(&vdd->dyn_mipi_clk.dyn_mipi_lock);
	idx = ss_find_dyn_mipi_clk_timing_idx(vdd);
	mutex_unlock(&vdd->dyn_mipi_clk.dyn_mipi_lock);
	if (idx < 0) {
		LCD_ERR("Failed to find MIPI clock timing (%d)\n", idx);
		goto err;
	}

	LCD_INFO("[DISPLAY_%d] +++ clk idx: %d, tx FFC\n", vdd->ndx, idx);

	ffc_set = ss_get_cmds(vdd, TX_FFC);
	dyn_ffc_set = ss_get_cmds(vdd, TX_DYNAMIC_FFC_SET);

	if (SS_IS_CMDS_NULL(ffc_set) || SS_IS_CMDS_NULL(dyn_ffc_set)) {
		LCD_ERR("No cmds for TX_FFC..\n");
		return -EINVAL;
	}

	if (unlikely((reg_list[0][1] == -EINVAL))) {
		cmd_list[0] = ffc_set;
		ss_find_reg_offset(reg_list, cmd_list, ARRAY_SIZE(cmd_list));
	}

	pos_ffc = reg_list[0][1];
	if (pos_ffc == -EINVAL) {
		LCD_ERR("fail to find FFC(C5h) offset in set\n");
		return -EINVAL;
	}

	memcpy(ffc_set->cmds[pos_ffc].msg.tx_buf,
			dyn_ffc_set->cmds[idx].msg.tx_buf,
			ffc_set->cmds[pos_ffc].msg.tx_len);

	ss_send_cmd(vdd, TX_FFC);
err:
	LCD_INFO("[DISPLAY_%d] --- clk idx: %d, tx FFC\n", vdd->ndx, idx);

	return 0;
}

static int ss_parse_dyn_mipi_clk_timing_table(struct device_node *np,
		void *tbl, char *keystring)
{
	struct clk_timing_table *table = (struct clk_timing_table *) tbl;
	const __be32 *data;
	int len = 0, i = 0, data_offset = 0;

	data = of_get_property(np, keystring, &len);

	if (!data) {
		LCD_ERR("%d, Unable to read table %s ", __LINE__, keystring);
		return -EINVAL;
	}

	table->tab_size = len / sizeof(int);
	table->clk_rate = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->clk_rate)
		return -ENOMEM;

	for (i = 0 ; i < table->tab_size; i++)
		table->clk_rate[i] = be32_to_cpup(&data[data_offset++]);

	LCD_INFO("%s tab_size (%d)\n", keystring, table->tab_size);

	return 0;
}

static int ss_parse_dyn_mipi_clk_sel_table(struct device_node *np,
		void *tbl, char *keystring)
{
	struct clk_sel_table *table = (struct clk_sel_table *) tbl;
	const __be32 *data;
	int len = 0, i = 0, data_offset = 0;
	int col_size = 0;

	data = of_get_property(np, keystring, &len);

	if (data)
		LCD_INFO("Success to read table %s\n", keystring);
	else {
		LCD_ERR("%d, Unable to read table %s ", __LINE__, keystring);
		return -EINVAL;
	}

	col_size = 5;

	if ((len % col_size) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s , len : %d",
					__LINE__, keystring, len);
		return -EINVAL;
	}

	table->tab_size = len / (sizeof(int) * col_size);

	table->rat = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->rat)
		return -ENOMEM;
	table->band = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->band)
		goto error;
	table->from = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->from)
		goto error;
	table->end = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->end)
		goto error;
	table->target_clk_idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->target_clk_idx)
		goto error;

	for (i = 0 ; i < table->tab_size; i++) {
		table->rat[i] = be32_to_cpup(&data[data_offset++]);
		table->band[i] = be32_to_cpup(&data[data_offset++]);
		table->from[i] = be32_to_cpup(&data[data_offset++]);
		table->end[i] = be32_to_cpup(&data[data_offset++]);
		table->target_clk_idx[i] = be32_to_cpup(&data[data_offset++]);
		LCD_DEBUG("%dst : %d %d %d %d %d\n",
			i, table->rat[i], table->band[i], table->from[i],
			table->end[i], table->target_clk_idx[i]);
	}

	LCD_INFO("%s tab_size (%d)\n", keystring, table->tab_size);

	return 0;

error:
	LCD_ERR("Allocation Fail\n");
	kfree(table->rat);
	kfree(table->band);
	kfree(table->from);
	kfree(table->end);
	kfree(table->target_clk_idx);

	return -ENOMEM;
}

int ss_parse_candella_mapping_table(struct device_node *np,
		void *tbl, char *keystring)
{
	struct candela_map_table *table = (struct candela_map_table *) tbl;
	const __be32 *data;
	int len = 0, i = 0, data_offset = 0;
	int col_size = 0;
	bool is_gamma_mode2_support = 0;

	is_gamma_mode2_support = of_property_read_bool(np, "samsung,support_gamma_mode2");

	LCD_INFO_ONCE("is_gamma_mode2_support = %d\n", is_gamma_mode2_support);

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_DEBUG("%d, Unable to read table %s ", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_ERR("Success to read table %s\n", keystring);

	if (is_gamma_mode2_support)
		col_size = 5;
	else
		col_size = 4;

	if ((len % col_size) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s , len : %d",
					__LINE__, keystring, len);
		return -EINVAL;
	}

	table->tab_size = len / (sizeof(int) * col_size);

	table->cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->cd)
		goto error;
	table->idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->idx)
		goto error;
	table->from = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->from)
		goto error;
	table->end = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->end)
		goto error;
	if (is_gamma_mode2_support) {
		table->gm2_wrdisbv = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
		if (!table->gm2_wrdisbv)
			goto error;
	}
	for (i = 0 ; i < table->tab_size; i++) {
		table->idx[i] = be32_to_cpup(&data[data_offset++]);		/* field => <idx> */
		table->from[i] = be32_to_cpup(&data[data_offset++]);	/* field => <from> */
		table->end[i] = be32_to_cpup(&data[data_offset++]);		/* field => <end> */
		if (is_gamma_mode2_support)
			table->gm2_wrdisbv[i] = be32_to_cpup(&data[data_offset++]);	/* field => <gm2_wrdisbv (panel_real_cd)> */
		table->cd[i] = be32_to_cpup(&data[data_offset++]);		/* field => <cd> */
	}

	table->min_lv = table->from[0];
	table->max_lv = table->end[table->tab_size-1];

	LCD_INFO("tab_size (%d), min/max lv (%d/%d)\n", table->tab_size, table->min_lv, table->max_lv);

	return 0;

error:
	kfree(table->cd);
	kfree(table->idx);
	kfree(table->from);
	kfree(table->end);
	if (is_gamma_mode2_support)
		kfree(table->gm2_wrdisbv);

	return -ENOMEM;
}

int ss_parse_pac_candella_mapping_table(struct device_node *np,
		void *tbl, char *keystring)
{
	struct candela_map_table *table = (struct candela_map_table *) tbl;
	const __be32 *data;
	int len = 0, i = 0, data_offset = 0;
	int col_size = 0;

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_DEBUG("%d, Unable to read table %s ", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_ERR("Success to read table %s\n", keystring);

	col_size = 6;

	if ((len % col_size) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s , len : %d",
					__LINE__, keystring, len);
		return -EINVAL;
	}

	table->tab_size = len / (sizeof(int) * col_size);

	table->interpolation_cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->interpolation_cd)
		return -ENOMEM;
	table->cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->cd)
		goto error;
	table->scaled_idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->scaled_idx)
		goto error;
	table->idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->idx)
		goto error;
	table->from = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->from)
		goto error;
	table->end = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->end)
		goto error;
/*
	table->auto_level = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->auto_level)
		goto error;
*/
	for (i = 0 ; i < table->tab_size; i++) {
		table->scaled_idx[i] = be32_to_cpup(&data[data_offset++]);	/* field => <scaeled idx> */
		table->idx[i] = be32_to_cpup(&data[data_offset++]);			/* field => <idx> */
		table->from[i] = be32_to_cpup(&data[data_offset++]);		/* field => <from> */
		table->end[i] = be32_to_cpup(&data[data_offset++]);			/* field => <end> */
		table->cd[i] = be32_to_cpup(&data[data_offset++]);			/* field => <cd> */
		table->interpolation_cd[i] = be32_to_cpup(&data[data_offset++]);	/* field => <interpolation cd> */
	}

	table->min_lv = table->from[0];
	table->max_lv = table->end[table->tab_size-1];

	LCD_INFO("tab_size (%d), hbm min/max lv (%d/%d)\n", table->tab_size, table->min_lv, table->max_lv);

	return 0;

error:
	kfree(table->interpolation_cd);
	kfree(table->cd);
	kfree(table->scaled_idx);
	kfree(table->idx);
	kfree(table->from);
	kfree(table->end);

	return -ENOMEM;
}

int ss_parse_multi_to_one_candella_mapping_table(struct device_node *np,
		void *tbl, char *keystring)
{
	struct candela_map_table *table = (struct candela_map_table *) tbl;
	const __be32 *data;
	int len = 0, i = 0, data_offset = 0;
	int col_size = 0;

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_DEBUG("%d, Unable to read table %s ", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_ERR("Success to read table %s\n", keystring);

	col_size = 4;

	if ((len % col_size) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s , len : %d",
					__LINE__, keystring, len);
		return -EINVAL;
	}

	table->tab_size = len / (sizeof(int) * col_size);

	table->cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->cd)
		goto error;
	table->idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->idx)
		goto error;
	table->from = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->from)
		goto error;
	table->end = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->end)
		goto error;

	for (i = 0 ; i < table->tab_size; i++) {
		table->idx[i] = be32_to_cpup(&data[data_offset++]); 	/* field => <idx> */
		table->from[i] = be32_to_cpup(&data[data_offset++]);	/* field => <from> */
		table->end[i] = be32_to_cpup(&data[data_offset++]); 	/* field => <end> */
		table->cd[i] = be32_to_cpup(&data[data_offset++]);		/* field => <cd> */
	}

	table->min_lv = table->from[0];
	table->max_lv = table->end[table->tab_size-1];

	LCD_INFO("tab_size (%d), hbm min/max lv (%d/%d)\n", table->tab_size, table->min_lv, table->max_lv);

	return 0;

error:
	kfree(table->cd);
	kfree(table->idx);
	kfree(table->from);
	kfree(table->end);

	return -ENOMEM;
}


int ss_parse_hbm_candella_mapping_table(struct device_node *np,
		void *tbl, char *keystring)
{
	struct candela_map_table *table = (struct candela_map_table *) tbl;
	const __be32 *data;
	int data_offset = 0, len = 0, i = 0;
	int col_size = 0;
	bool is_gamma_mode2_support = 0;

	is_gamma_mode2_support = of_property_read_bool(np, "samsung,support_gamma_mode2");

	LCD_INFO_ONCE("is_gamma_mode2_support = %d\n", is_gamma_mode2_support);

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_DEBUG("%d, Unable to read table %s ", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_ERR("Success to read table %s\n", keystring);

	if (is_gamma_mode2_support)
		col_size = 6;
	else
		col_size = 5;

	if ((len % col_size) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s",
					__LINE__, keystring);
		return -EINVAL;
	}

	table->tab_size = len / (sizeof(int)*col_size);

	table->interpolation_cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->interpolation_cd)
		return -ENOMEM;
	table->cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->cd)
		goto error;
	table->scaled_idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->scaled_idx)
		goto error;
	table->idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->idx)
		goto error;
	table->from = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->from)
		goto error;
	table->end = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->end)
		goto error;
	table->auto_level = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->auto_level)
		goto error;
	if (is_gamma_mode2_support) {
		table->gm2_wrdisbv = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
		if (!table->gm2_wrdisbv)
			goto error;
	}
	for (i = 0 ; i < table->tab_size; i++) {
		table->idx[i] = be32_to_cpup(&data[data_offset++]);		/* 1st field => <idx> */
		table->from[i] = be32_to_cpup(&data[data_offset++]);		/* 2nd field => <from> */
		table->end[i] = be32_to_cpup(&data[data_offset++]);			/* 3rd field => <end> */
		if (is_gamma_mode2_support)
			table->gm2_wrdisbv[i] = be32_to_cpup(&data[data_offset++]);	/* field => <gm2_wrdisbv (panel_real_cd)> */

		table->cd[i] = be32_to_cpup(&data[data_offset++]);		/* 4th field => <candella> */
		table->auto_level[i] = be32_to_cpup(&data[data_offset++]);  /* 5th field => <auto brightness level> */
	}

	table->min_lv = table->from[0];
	table->max_lv = table->end[table->tab_size-1];

	LCD_INFO("tab_size (%d), hbm min/max lv (%d/%d)\n", table->tab_size, table->min_lv, table->max_lv);

	return 0;
error:
	kfree(table->cd);
	kfree(table->idx);
	kfree(table->from);
	kfree(table->end);
	kfree(table->auto_level);
	if (is_gamma_mode2_support)
		kfree(table->gm2_wrdisbv);

	return -ENOMEM;
}

int ss_parse_panel_table(struct device_node *np,
		void *tbl, char *keystring)
{
	struct cmd_map *table = (struct cmd_map *) tbl;
	const __be32 *data;
	int  data_offset, len = 0, i = 0;

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_DEBUG("%d, Unable to read table %s\n", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_ERR("Success to read table %s\n", keystring);

	if ((len % 2) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s",
					__LINE__, keystring);
		return -EINVAL;
	}

	table->size = len / (sizeof(int)*2);
	table->bl_level = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->bl_level)
		return -ENOMEM;

	table->cmd_idx = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->cmd_idx)
		goto error;

	data_offset = 0;
	for (i = 0 ; i < table->size; i++) {
		table->bl_level[i] = be32_to_cpup(&data[data_offset++]);
		table->cmd_idx[i] = be32_to_cpup(&data[data_offset++]);
	}

	return 0;
error:
	kfree(table->cmd_idx);

	return -ENOMEM;
}

#define WAIT_PM_RESUME_TIMEOUT_MS	(3000)	/* 3 seconds */

static int ss_wait_for_pm_resume(struct samsung_display_driver_data *vdd)
{
	struct drm_device *ddev = GET_DRM_DEV(vdd);
	int timeout = WAIT_PM_RESUME_TIMEOUT_MS;

	while (!pm_runtime_enabled(ddev->dev) && --timeout)
		usleep_range(1000, 1000); /* 1ms */

	if (timeout < WAIT_PM_RESUME_TIMEOUT_MS)
		LCD_INFO("wait for pm resume (timeout: %d)", timeout);

	if (!timeout) {
		LCD_ERR("pm resume timeout \n");

		/* Should not reach here... Add panic to debug further */
		panic("timeout wait for pm resume");
		return -ETIMEDOUT;
	}

	return 0;
}

int ss_send_cmd(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type type)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_display *dsi_display = GET_DSI_DISPLAY(vdd);
	struct dsi_panel_cmd_set *set;
	bool is_vdd_locked = false;
	int rc = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null\n");
		return -ENODEV;
	}

	/* Make not to turn on the panel power when ub_con_det gpio is high (ub is not connected) */
	if (unlikely(vdd->is_factory_mode)) {
		if (gpio_is_valid(vdd->ub_con_det.gpio) && gpio_get_value(vdd->ub_con_det.gpio)) {
			LCD_ERR("ub_con_det.gpio = %d\n", gpio_get_value(vdd->ub_con_det.gpio));
			return -EAGAIN;
		}

		if (gpio_is_valid(vdd->fg_err_gpio) && gpio_get_value(vdd->fg_err_gpio)) {
			LCD_ERR("fg_err_gpio val: %d\n", gpio_get_value(vdd->fg_err_gpio));
			return -EAGAIN;
		}
	}

	if (!ss_panel_attach_get(vdd)) {
		LCD_ERR("ss_panel_attach_get(%d) : %d\n",
				vdd->ndx, ss_panel_attach_get(vdd));
		return -EAGAIN;
	}

	/* Skip to lock vdd_lock for commands that has exclusive_pass token
	 * to prevent deadlock between vdd_lock and ex_tx_waitq.
	 * cmd_lock guarantees exclusive tx cmds without vdd_lock.
	 */
	set = ss_get_cmds(vdd, type);
	/*
	if (SS_IS_CMDS_NULL(set)) {
		LCD_ERR("No cmds for type(%d) \n", type);
		return -EINVAL;
	}
*/
	if (likely(!vdd->exclusive_tx.enable || !set->exclusive_pass)) {
		mutex_lock(&vdd->vdd_lock);
		is_vdd_locked = true;
	}

	if (ss_is_panel_off(vdd) || (vdd->panel_func.samsung_lvds_write_reg)) {
		LCD_ERR("skip to tx cmd (%d), panel off (%d)\n",
				type, ss_is_panel_off(vdd));
		goto error;
	}

	if (vdd->debug_data->print_cmds)
		LCD_INFO("[DISPLAY_%d]Send cmd(%d): %s ++\n", vdd->ndx, type, ss_get_cmd_name(type));
	else
		LCD_DEBUG("[DISPLAY_%d]Send cmd(%d): %s ++\n", vdd->ndx, type, ss_get_cmd_name(type));

	/* In pm suspend status, it fails to control display clock.
	 * In result,
	 * - fails to transmit command to DDI.
	 * - sometimes causes unbalanced reference count of qc display clock.
	 * To prevent above issues, wait for pm resume before control display clock.
	 * You don't have to force to wake up PM system by calling pm_wakeup_ws_event().
	 * If cpu core reach to this code, it means interrupt or the other event is waking up
	 * PM system. So, it is enough just waiting until PM resume.
	 */
	if (ss_wait_for_pm_resume(vdd))
		goto error;

	/* case 03063186:
	 * To prevent deadlock between phandle->phandle_lock and panel->panel_lock,
	 * dsi_display_clk_ctrl() should be called without locking panel_lock.
	 */
	rc = dsi_display_clk_ctrl(dsi_display->dsi_clk_handle,
			DSI_ALL_CLKS, DSI_CLK_ON);
	if (rc) {
		LCD_ERR("[%s] failed to enable DSI core clocks, rc=%d\n",
				dsi_display->name, rc);
		goto error;
	}

	dsi_panel_tx_cmd_set(panel, type);

	rc = dsi_display_clk_ctrl(dsi_display->dsi_clk_handle,
			DSI_ALL_CLKS, DSI_CLK_OFF);
	if (rc) {
		LCD_ERR("[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, rc);
		goto error;
	}

	if (vdd->debug_data->print_cmds)
		LCD_INFO("Send cmd(%d): %s --\n", type, ss_get_cmd_name(type));
	else
		LCD_DEBUG("Send cmd(%d): %s --\n", type, ss_get_cmd_name(type));

error:
	if (likely(is_vdd_locked))
		mutex_unlock(&vdd->vdd_lock);

	return rc;
}

#include <linux/cpufreq.h>
#define CPUFREQ_MAX	100
#define CPUFREQ_ALT_HIGH	70
struct cluster_cpu_info {
	int cpu_man; /* cpu number that manages the policy */
	int max_freq;
};

static void __ss_set_cpufreq_cpu(struct samsung_display_driver_data *vdd,
				int enable, int cpu, int max_percent)
{
	struct cpufreq_policy *policy;
	static unsigned int org_min[NR_CPUS + 1];
	static unsigned int org_max[NR_CPUS + 1];

	policy = cpufreq_cpu_get(cpu);
	if (policy == NULL) {
		LCD_ERR("policy is null..\n");
		return;
	}

	if (enable) {
		org_min[cpu] = policy->min;
		org_max[cpu] = policy->max;

		/* max_freq's unit is kHz, and it should not cause overflow.
		 * After applying user_policy, cpufreq driver will find closest
		 * frequency in freq_table, and set the frequency.
		 */
		policy->user_policy.min = policy->user_policy.max =
			policy->cpuinfo.max_freq * max_percent / 100;
	} else {
		policy->user_policy.min = org_min[cpu];
		policy->user_policy.max = org_max[cpu];
	}

	cpufreq_update_policy(cpu);
	cpufreq_cpu_put(policy);

	LCD_DEBUG("en=%d, cpu=%d, per=%d, min=%d, org=(%d-%d)\n",
			enable, cpu, max_percent, policy->user_policy.min,
			org_min[cpu], org_max[cpu]);
}

void ss_set_max_cpufreq(struct samsung_display_driver_data *vdd,
				int enable, enum mdss_cpufreq_cluster cluster)
{
	struct cpufreq_policy *policy;
	int cpu;
	struct cluster_cpu_info cluster_info[NR_CPUS];
	int cnt_cluster;
//	int bigp_cpu_man;
	int big_cpu_man = 0;
	int little_cpu_man = 0;
	int i = 0;

	//return;

	LCD_DEBUG("en=%d, cluster=%d\n", enable, cluster);
	get_online_cpus();

	/* find clusters */
	cnt_cluster = 0;
	for_each_online_cpu(cpu) {
		policy = cpufreq_cpu_get(cpu);
		/* In big-little octa core system,
		 * cpu0 ~ cpu3 has same policy (policy0) and
		 * policy->cpu would be 0. (cpu0 manages policy0)
		 * cpu4 ~ cpu7 has same policy (policy4) and
		 * policy->cpu would be 4. (cpu4 manages policy0)
		 * In this case, cnt_cluster would be two, and
		 * cluster_info[0].cpu_man = 0, cluster_info[1].cpu_man = 4.
		 */
		if (policy && policy->cpu == cpu) {
			cluster_info[cnt_cluster].cpu_man = cpu;
			cluster_info[cnt_cluster].max_freq =
				policy->cpuinfo.max_freq;
			cnt_cluster++;
			//LCD_INFO("cpu(%d)\n", cpu);
		}
		cpufreq_cpu_put(policy);
	}

	if (!cnt_cluster || cnt_cluster > 3) {
		LCD_ERR("cluster count err (%d)\n", cnt_cluster);
		goto end;
	}

	if (cnt_cluster == 1) {
		/* single policy (none big-little case) */
		LCD_INFO("cluster count is one...\n");

		if (cluster == CPUFREQ_CLUSTER_ALL)
			/* set all cores' freq to max*/
			__ss_set_cpufreq_cpu(vdd, enable,
					cluster_info[0].cpu_man, CPUFREQ_MAX);
		else
			/* set all cores' freq to max * 70 percent */
			__ss_set_cpufreq_cpu(vdd, enable,
				cluster_info[0].cpu_man, CPUFREQ_ALT_HIGH);
		goto end;
	}

#if 0
	/* big-little */
	if (cluster_info[0].max_freq > cluster_info[1].max_freq) {
		if (cluster_info[1].max_freq > cluster_info[2].max_freq) {
			/* 0 > 1 > 2 */
			big_cpu_man = cluster_info[0].cpu_man;
			little_cpu_man = cluster_info[2].cpu_man;
		}
		else if (cluster_info[2].max_freq > cluster_info[0].max_freq) {
			/* 2 > 0 > 1 */
			big_cpu_man = cluster_info[2].cpu_man;
			little_cpu_man = cluster_info[1].cpu_man;
		}
		else {
			/* 0 > 2 > 1 */
			big_cpu_man = cluster_info[0].cpu_man;
			little_cpu_man = cluster_info[1].cpu_man;
		}
	} else {
		if (cluster_info[0].max_freq > cluster_info[2].max_freq) {
			/* 1 > 0 > 2 */
			big_cpu_man = cluster_info[1].cpu_man;
			little_cpu_man = cluster_info[2].cpu_man;
		}
		else if (cluster_info[2].max_freq > cluster_info[1].max_freq) {
			/* 2 > 1 > 0 */
			big_cpu_man = cluster_info[2].cpu_man;
			little_cpu_man = cluster_info[0].cpu_man;
		}
		else {
			/* 1 > 2 > 0 */
			big_cpu_man = cluster_info[1].cpu_man;
			little_cpu_man = cluster_info[0].cpu_man;
		}
	}
#endif

	if (cluster == CPUFREQ_CLUSTER_BIG) {
		__ss_set_cpufreq_cpu(vdd, enable, big_cpu_man,
				CPUFREQ_MAX);
	} else if (cluster == CPUFREQ_CLUSTER_LITTLE) {
		__ss_set_cpufreq_cpu(vdd, enable, little_cpu_man,
				CPUFREQ_MAX);
	} else if  (cluster == CPUFREQ_CLUSTER_ALL) {
		for (i = 0; i < cnt_cluster; i++) {
			__ss_set_cpufreq_cpu(vdd, enable, cluster_info[i].cpu_man,
				CPUFREQ_MAX);
		}
		/*
		__ss_set_cpufreq_cpu(vdd, enable, big_cpu_man,
				CPUFREQ_MAX);
		__ss_set_cpufreq_cpu(vdd, enable, little_cpu_man,
				CPUFREQ_MAX);
		*/
	}

end:
	put_online_cpus();
}

void ss_set_max_mem_bw(struct samsung_display_driver_data *vdd, int enable)
{
	struct dsi_display *display = NULL;
	struct msm_drm_private *priv = NULL;
	static u64 cur_mnoc_ab, cur_mnoc_ib;

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_ERR("no display");
		return;
	}

	priv = display->drm_dev->dev_private;

	if (enable) {
		/* Save Current MNOC AB/IB Value */
		cur_mnoc_ab = priv->phandle.data_bus_handle[SDE_POWER_HANDLE_DBUS_ID_MNOC].in_ab_quota;
		cur_mnoc_ib = priv->phandle.data_bus_handle[SDE_POWER_HANDLE_DBUS_ID_MNOC].in_ib_quota;
		sde_power_data_bus_set_quota(&priv->phandle,
				SDE_POWER_HANDLE_DBUS_ID_MNOC,
				SDE_POWER_HANDLE_CONT_SPLASH_BUS_AB_QUOTA,
				SDE_POWER_HANDLE_CONT_SPLASH_BUS_IB_QUOTA);
	} else {
		/* Restore Last MNOC AB/IB Value */
		sde_power_data_bus_set_quota(&priv->phandle,
			SDE_POWER_HANDLE_DBUS_ID_MNOC,
			cur_mnoc_ab,
			cur_mnoc_ib);
	}
}

/* ss_set_exclusive_packet():
 * This shuold be called in ex_tx_lock lock.
 */
void ss_set_exclusive_tx_packet(
		struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type cmd, int pass)
{
	struct dsi_panel_cmd_set *pcmds = NULL;

	pcmds = ss_get_cmds(vdd, cmd);
	pcmds->exclusive_pass = pass;
}

/*
 * ex_tx_lock should be locked before panel_lock if there is a dsi_panel_tx_cmd_set after panel_lock is locked.
 * because of there is a "ex_tx_lock -> panel lock" routine at the sequence of ss_gct_write.
 * So we need to add ex_tx_lock to protect all "dsi_panel_tx_cmd_set after panel_lock".
 */

void ss_set_exclusive_tx_lock_from_qct(struct samsung_display_driver_data *vdd, bool lock)
{
	if (lock)
		mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	else
		mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);
}

#define _ALIGN_DOWN(addr, size)  ((addr)&(~((size)-1)))
//#define MAX_DSI_FIFO_SIZE_HS_MODE	(480) /* AP limitation */
/* TODO: find MAX_DSI_FIFO_SIZE_HS_MODE	for sdm845... if set to 480, cause fail of tx mipi */
#define MAX_DSI_FIFO_SIZE_HS_MODE	(50) /* AP limitation */
#define MAX_SIZE_IMG_PAYLOAD	_ALIGN_DOWN((MAX_DSI_FIFO_SIZE_HS_MODE - 1), 12)

#define GRAM_WR_MEM_START	0x2c
#define GRAM_WR_MEM_CONT	0x3c
#define SIDERAM_WR_MEM_START	0x4c
#define SIDERAM_WR_MEM_CONT	0x5c

int ss_write_ddi_ram(struct samsung_display_driver_data *vdd,
		int target, u8 *buffer, int len)
{
	u8 hdr_start;
	u8 hdr_continue;
	struct dsi_panel_cmd_set *set;
	struct dsi_cmd_desc *cmds;
	u8 *tx_buf;
	u8 *org_tx_buf;
	int loop;
	int remain;
	int psize;

	LCD_INFO("[DISPLAY_%d] +\n", vdd->ndx);

	set = ss_get_cmds(vdd, TX_DDI_RAM_IMG_DATA);

	if (target == MIPI_TX_TYPE_GRAM) {
		hdr_start = GRAM_WR_MEM_START;
		hdr_continue = GRAM_WR_MEM_CONT;
	} else if (target == MIPI_TX_TYPE_SIDERAM) {
		hdr_start = SIDERAM_WR_MEM_START;
		hdr_continue = SIDERAM_WR_MEM_CONT;
	} else {
		LCD_ERR("invalid target (%d)\n", target);
		return -EINVAL;
	}

	set->state = DSI_CMD_SET_STATE_HS;
	set->count = DIV_ROUND_UP(len, MAX_SIZE_IMG_PAYLOAD);

	cmds = vzalloc(sizeof(struct dsi_cmd_desc) * set->count);
	tx_buf = vzalloc((MAX_SIZE_IMG_PAYLOAD + 1) * set->count);
	org_tx_buf = tx_buf;
	set->cmds = cmds;


	LCD_INFO("len=%d, cmds count=%d\n", len, set->count);

	/* split and fill image data to TX_DDI_RAM_IMG_DATA packet */
	loop = 0;
	remain = len;
	while (remain > 0) {
		cmds[loop].msg.type = MIPI_DSI_GENERIC_LONG_WRITE;
		cmds[loop].last_command = 1;
		cmds[loop].msg.tx_buf = tx_buf;

		if (loop == 0)
			*tx_buf = hdr_start;
		else
			*tx_buf = hdr_continue;
		tx_buf++;

		if (remain > MAX_SIZE_IMG_PAYLOAD)
			psize = MAX_SIZE_IMG_PAYLOAD;
		else
			psize = remain;

		memcpy(tx_buf, buffer, psize);
		cmds[loop].msg.tx_len = psize + 1;

		tx_buf += psize;
		buffer += psize;
		remain -= psize;
		loop++;
	}

	LCD_INFO("start tx gram\n");
	ss_send_cmd(vdd, TX_DDI_RAM_IMG_DATA);
	LCD_INFO("fin tx gram\n");

	vfree(org_tx_buf);
	vfree(cmds);
	LCD_INFO("[DISPLAY_%d] -\n", vdd->ndx);

	return 0;
}

/**
 * controller have 4 registers can hold 16 bytes of rxed data
 * dcs packet: 4 bytes header + payload + 2 bytes crc
 * 1st read: 4 bytes header + 10 bytes payload + 2 crc
 * 2nd read: 14 bytes payload + 2 crc
 * 3rd read: 14 bytes payload + 2 crc
 */
#define MAX_LEN_RX_BUF	500
#define RX_SIZE_LIMIT	10
int ss_panel_data_read_no_gpara(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type type, u8 *buffer, int level_key)
{
	struct dsi_panel_cmd_set *set;
	char show_buffer[MAX_LEN_RX_BUF] = {0,};
	char temp_buffer[MAX_LEN_RX_BUF] = {0,};
	int orig_rx_len = 0;
	int orig_offset = 0;
	int pos = 0;
	int i;

	/* samsung mipi rx cmd feature supports only one command */
	set = ss_get_cmds(vdd, type);
	if (SS_IS_CMDS_NULL(set) || !ss_is_read_cmd(set)) {
		LCD_ERR("invalid set(%d): %s\n", type, ss_get_cmd_name(type));
		return -EINVAL;
	}

	/* enable level key */
	if (level_key & LEVEL0_KEY)
		ss_send_cmd(vdd, TX_LEVEL0_KEY_ENABLE);
	if (level_key & LEVEL1_KEY)
		ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	if (level_key & LEVEL2_KEY)
		ss_send_cmd(vdd, TX_LEVEL2_KEY_ENABLE);
	if (level_key & POC_KEY)
		ss_send_cmd(vdd, TX_POC_KEY_ENABLE);

	orig_rx_len = set->cmds[0].msg.rx_len;
	orig_offset = set->read_startoffset;

	/* reset rx len/offset */
	set->cmds[0].msg.rx_len += orig_offset;
	set->read_startoffset = 0;

	set->cmds[0].msg.rx_buf = temp_buffer;

	LCD_DEBUG("orig_rx_len (%d) , orig_offset (%d) \n", orig_rx_len, orig_offset);
	LCD_DEBUG("new_rx_len (%ld) , new_offset (%d) \n", set->cmds[0].msg.rx_len, set->read_startoffset);

	/* RX */
	ss_send_cmd(vdd, type);

	/* oopy to buffer from original offset */
	if (buffer)
		memcpy(buffer, temp_buffer + orig_offset, orig_rx_len);

	/* show buffer */
	if (buffer)
		for (i = 0; i < orig_rx_len; i++)
			pos += snprintf(show_buffer + pos, sizeof(show_buffer) - pos, "%02x ",
				buffer[i]);

	/* restore rx len/offset */
	set->cmds[0].msg.rx_len = orig_rx_len;
	set->read_startoffset = orig_offset;

	/* disable level key */
	if (level_key & POC_KEY)
		ss_send_cmd(vdd, TX_POC_KEY_DISABLE);
	if (level_key & LEVEL2_KEY)
		ss_send_cmd(vdd, TX_LEVEL2_KEY_DISABLE);
	if (level_key & LEVEL1_KEY)
		ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);
	if (level_key & LEVEL0_KEY)
		ss_send_cmd(vdd, TX_LEVEL0_KEY_DISABLE);

	if (type != RX_FLASH_GAMMA && type != RX_POC_READ && type != RX_FW_UP_STATUS)
		LCD_INFO("[%d]%s, addr: 0x%x, off: %d, len: %d, buf: %s\n",
				type, ss_get_cmd_name(type),
				set->cmds[0].msg.tx_buf[0], orig_offset, orig_rx_len,
				show_buffer);

	return 0;
}

int ss_panel_data_read_gpara(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type type, u8 *buffer, int level_key)
{
	struct dsi_panel_cmd_set *set;
	struct dsi_panel_cmd_set *read_pos_cmd;
	static u8 rx_buffer[MAX_LEN_RX_BUF] = {0,};
	char show_buffer[MAX_LEN_RX_BUF] = {0,};
	char temp_buffer[MAX_LEN_RX_BUF] = {0,};
	char rx_addr = 0;
	int orig_rx_len = 0;
	int new_rx_len = 0;
	int orig_offset = 0;
	int new_offset = 0;
	int loop_limit = 0;
	int copy_pos = 0;
	int pos = 0;
	int i, j;

	/* samsung mipi rx cmd feature supports only one command */
	set = ss_get_cmds(vdd, type);
	if (SS_IS_CMDS_NULL(set) || !ss_is_read_cmd(set)) {
		LCD_ERR("invalid set(%d): %s\n", type, ss_get_cmd_name(type));
		return -EINVAL;
	}

	/* enable level key */
	if (level_key & LEVEL0_KEY)
		ss_send_cmd(vdd, TX_LEVEL0_KEY_ENABLE);
	if (level_key & LEVEL1_KEY)
		ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	if (level_key & LEVEL2_KEY)
		ss_send_cmd(vdd, TX_LEVEL2_KEY_ENABLE);
	if (level_key & POC_KEY)
		ss_send_cmd(vdd, TX_POC_KEY_ENABLE);

	set->cmds[0].msg.rx_buf = rx_buffer;

	read_pos_cmd = ss_get_cmds(vdd, TX_REG_READ_POS);
	if (SS_IS_CMDS_NULL(read_pos_cmd)) {
		LCD_ERR("No cmds for TX_REG_READ_POS.. \n");
		return -EINVAL;
	}

	rx_addr = set->cmds[0].msg.tx_buf[0];
	orig_rx_len = set->cmds[0].msg.rx_len;
	orig_offset = new_offset = set->read_startoffset;

	loop_limit = (orig_rx_len + RX_SIZE_LIMIT - 1) / RX_SIZE_LIMIT;

	/* set pointing gpara */
	if (read_pos_cmd->cmds->msg.tx_len == 3)
		read_pos_cmd->cmds->msg.tx_buf[2] = rx_addr;

	LCD_DEBUG("orig_rx_len (%d) , orig_offset (%d) loop_limit (%d)\n", orig_rx_len, orig_offset, loop_limit);

	for (i = 0; i < loop_limit; i++) {
		/* gPara */
		read_pos_cmd->cmds->msg.tx_buf[1] = new_offset;
		new_rx_len = ((orig_rx_len - new_offset + orig_offset) < RX_SIZE_LIMIT) ?
						(orig_rx_len - new_offset + orig_offset) : RX_SIZE_LIMIT;

		LCD_DEBUG("new_offset (%d) new_rx_len (%d) \n", new_offset, new_rx_len);

		ss_send_cmd(vdd, TX_REG_READ_POS);

		set->cmds[0].msg.rx_len = new_rx_len;

		if (vdd->dtsi_data.samsung_anapass_power_seq) {
			/*
				some panel need to update read size by address.
				It means "address + read_size" is essential.
			*/
			if (set->cmds[0].msg.tx_len == 2) {
				set->cmds[0].msg.tx_buf[1] = new_rx_len;
				LCD_DEBUG("update address + read_size(%d) \n", new_rx_len);
			}
		}

		/* RX */
		ss_send_cmd(vdd, type);

		/* oopy to buffer */
		if (buffer) {
			memcpy(&buffer[copy_pos], rx_buffer, new_rx_len);
			copy_pos += new_rx_len;
		}

		/* snprint */
		memcpy(temp_buffer, set->cmds[0].msg.rx_buf, new_rx_len);
		for (j = 0; j < new_rx_len && pos < MAX_LEN_RX_BUF; j++) {
			pos += snprintf(show_buffer + pos, MAX_LEN_RX_BUF - pos, "%02x ",
				temp_buffer[j]);
		}

		/* increase gPara offset */
		new_offset += new_rx_len;

		if (new_offset - orig_offset >= orig_rx_len)
			break;
	}

	/* restore rx len */
	set->cmds[0].msg.rx_len = orig_rx_len;

	/* disable level key */
	if (level_key & POC_KEY)
		ss_send_cmd(vdd, TX_POC_KEY_DISABLE);
	if (level_key & LEVEL2_KEY)
		ss_send_cmd(vdd, TX_LEVEL2_KEY_DISABLE);
	if (level_key & LEVEL1_KEY)
		ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);
	if (level_key & LEVEL0_KEY)
		ss_send_cmd(vdd, TX_LEVEL0_KEY_DISABLE);

	if (type != RX_FLASH_GAMMA && type != RX_POC_READ)
		LCD_INFO("[%d] %s, addr: 0x%x, off: %d, len: %d, buf: %s\n",
				type, ss_get_cmd_name(type),
				set->cmds[0].msg.tx_buf[0], orig_offset, orig_rx_len,
				show_buffer);

	return 0;
}

int ss_panel_data_read(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type type, u8 *buffer, int level_key)
{
	int ret;

	if (!ss_panel_attach_get(vdd)) {
		LCD_ERR("ss_panel_attach_get(%d) : %d\n",
				vdd->ndx, ss_panel_attach_get(vdd));
		return -EPERM;
	}

	if (ss_is_panel_off(vdd)) {
		LCD_ERR("skip to rx cmd (%d), panel off (%d)\n",
				type, ss_is_panel_off(vdd));
		return -EPERM;
	}

	/* To block read operation at esd-recovery */
	if (vdd->panel_dead) {
		LCD_ERR("esd recovery, skip %s\n", ss_get_cmd_name(type));
		return -EPERM;
	}

	if (vdd->gpara)
		ret = ss_panel_data_read_gpara(vdd, type, buffer, level_key);
	else
		ret = ss_panel_data_read_no_gpara(vdd, type, buffer, level_key);

	return ret;
}

struct STM_REG_OSSET stm_offset_list[] = {
	{.name = "stm_ctrl_en=", 		.offset = offsetof(struct STM_CMD, STM_CTRL_EN)},
	{.name = "stm_max_opt=", 		.offset = offsetof(struct STM_CMD, STM_MAX_OPT)},
	{.name = "stm_default_opt=", 	.offset = offsetof(struct STM_CMD, STM_DEFAULT_OPT)},
	{.name = "stm_dim_step=", 		.offset = offsetof(struct STM_CMD, STM_DIM_STEP)},
	{.name = "stm_frame_period=",	.offset = offsetof(struct STM_CMD, STM_FRAME_PERIOD)},
	{.name = "stm_min_sect=", 		.offset = offsetof(struct STM_CMD, STM_MIN_SECT)},
	{.name = "stm_pixel_period=",	.offset = offsetof(struct STM_CMD, STM_PIXEL_PERIOD)},
	{.name = "stm_line_period=", 	.offset = offsetof(struct STM_CMD, STM_LINE_PERIOD)},
	{.name = "stm_min_move=", 		.offset = offsetof(struct STM_CMD, STM_MIN_MOVE)},
	{.name = "stm_m_thres=", 		.offset = offsetof(struct STM_CMD, STM_M_THRES)},
	{.name = "stm_v_thres=", 		.offset = offsetof(struct STM_CMD, STM_V_THRES)},
};

int ss_stm_set_cmd_offset(struct STM_CMD *cmd, char* p)
{
	int list_size = ARRAY_SIZE(stm_offset_list);
	const char *name;
	int i, val;
	int *offset;

	for (i = 0; i < list_size; i++) {
		name = stm_offset_list[i].name;
		if (!strncmp(name, p, strlen(name))) {
			sscanf(p + strlen(name), "%d", &val);
			offset = (int *)((void*)cmd + stm_offset_list[i].offset);
			*offset = val;
			return 0;
		}
	}

	return -1;
}

void print_stm_cmd(struct STM_CMD cmd)
{
	LCD_INFO("CTRL_EN(%d) MAX_OPT(%d) DEFAULT_OPT(%d) DIM_STEP(%d)\n",
		cmd.STM_CTRL_EN, cmd.STM_MAX_OPT, cmd.STM_DEFAULT_OPT, cmd.STM_DIM_STEP);
	LCD_INFO("FRAME_PERIOD(%d) MIN_SECT(%d) PIXEL_PERIOD(%d) LINE_PEROID(%d) \n",
		cmd.STM_FRAME_PERIOD, cmd.STM_MIN_SECT, cmd.STM_PIXEL_PERIOD, cmd.STM_LINE_PERIOD);
	LCD_INFO("MIN_MOVE(%d) M_THRES(%d) V_THRES(%d)\n",
		cmd.STM_MIN_MOVE, cmd.STM_M_THRES, cmd.STM_V_THRES);
}

/**
 * ss_get_stm_orig_cmd - get original stm cmd from panel dtsi.
 */
int ss_get_stm_orig_cmd(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *pcmds = NULL;
	struct STM_CMD *cmd = &vdd->stm.orig_cmd;
	u8 *cmd_pload;
	int cmd_len;
	char buf[256];
	int i, len = 0;

	pcmds = ss_get_cmds(vdd, TX_STM_ENABLE);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_STM_ENABLE..\n");
		return -EINVAL;
	}

	cmd_pload = pcmds->cmds[1].msg.tx_buf;
	cmd_len = pcmds->cmds[1].msg.tx_len;

	cmd->STM_CTRL_EN = (cmd_pload[1] & 0x01) ? 1 : 0;
	cmd->STM_MAX_OPT = (cmd_pload[3] & 0xF0) >> 4;

	cmd->STM_DEFAULT_OPT = (cmd_pload[3] & 0x0F);

	cmd->STM_DIM_STEP = (cmd_pload[4] & 0x0F);

	cmd->STM_FRAME_PERIOD = (cmd_pload[5] & 0xF0) >> 4;
	cmd->STM_MIN_SECT = (cmd_pload[5] & 0x0F);

	cmd->STM_PIXEL_PERIOD = (cmd_pload[6] & 0xF0) >> 4;
	cmd->STM_LINE_PERIOD = (cmd_pload[6] & 0x0F);

	cmd->STM_MIN_MOVE = cmd_pload[7];
	cmd->STM_M_THRES = cmd_pload[8];
	cmd->STM_V_THRES = cmd_pload[9];

	/* init current cmd with origianl cmd */
	memcpy(&vdd->stm.cur_cmd, cmd, sizeof(struct STM_CMD));

	print_stm_cmd(vdd->stm.cur_cmd);

	for (i = 0; i < cmd_len; i++)
		len += snprintf(buf + len, sizeof(buf) - len,
						"%02x ", cmd_pload[i]);
	LCD_ERR("cmd[%d] : %s\n", cmd_len, buf);

	return 1;
}

/**
 * ss_stm_set_cmd - set stm mipi cmd using STM_CMD para.
 *
 * stm_cmd : new stm cmd to update.
 */
void ss_stm_set_cmd(struct samsung_display_driver_data *vdd, struct STM_CMD *cmd)
{
	struct dsi_panel_cmd_set *pcmds = NULL;
	int cmd_len;
	u8 *cmd_pload;
	char buf[256];
	int i, len = 0;

	pcmds = ss_get_cmds(vdd, TX_STM_ENABLE);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR("No cmds for TX_STM_ENABLE..\n");
		return;
	}

	cmd_pload = pcmds->cmds[1].msg.tx_buf;
	cmd_len = pcmds->cmds[1].msg.tx_len;

	cmd_pload[1] = cmd->STM_CTRL_EN ? 0x01 : 0x00;

	cmd_pload[3] = ((cmd->STM_MAX_OPT & 0x0F) << 4);
	cmd_pload[3] |= (cmd->STM_DEFAULT_OPT & 0x0F);

	cmd_pload[4] |= (cmd->STM_DIM_STEP & 0x0F);

	cmd_pload[5] = ((cmd->STM_FRAME_PERIOD & 0x0F) << 4);
	cmd_pload[5] |= (cmd->STM_MIN_SECT & 0x0F);

	cmd_pload[6] = ((cmd->STM_PIXEL_PERIOD & 0x0F) << 4);
	cmd_pload[6] |= (cmd->STM_LINE_PERIOD & 0x0F);

	cmd_pload[7] = cmd->STM_MIN_MOVE & 0xFF;
	cmd_pload[8] = cmd->STM_M_THRES & 0xFF;
	cmd_pload[9] = cmd->STM_V_THRES & 0xFF;

	for (i = 0; i < cmd_len; i++)
		len += snprintf(buf + len, sizeof(buf) - len,
						"%02x ", cmd_pload[i]);
	LCD_ERR("cmd[%d] : %s\n", cmd_len, buf);

	/* reset current stm cmds */
	memcpy(&vdd->stm.cur_cmd, cmd, sizeof(struct STM_CMD));

	return;
}

int ss_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	int lcd_id_cmdline;

	ss_set_vrr_switch(vdd, false);

	/* At this time, it already enabled SDE clock/power and  display power.
	 * It is possible to send mipi comamnd to display.
	 * To send mipi command, like mdnie setting or brightness setting,
	 * change panel_state: PANEL_PWR_OFF -> PANEL_PWR_ON_READY, here.
	 */
	ss_set_panel_state(vdd, PANEL_PWR_ON_READY);

	vdd->display_status_dsi.disp_on_pre = 1;

	if (vdd->other_line_panel_work_cnt)
		vdd->other_line_panel_work_cnt = 0; /*stop open otherline dat file*/

	/* Print debug data : fw up fail info */
	if (vdd->fw_up.is_support)
		ss_read_fw_up_debug_partition();

	if (!ss_panel_attach_get(vdd)) {
		ss_read_fw_up_debug_partition();

		LCD_ERR("ss_panel_attach_get NG\n");
		return false;
	}
	if (vdd->mipi_header_modi) {
		LCD_INFO("2808 panel skip on_pre\n");
		return true;
	}

	LCD_INFO("[DISPLAY_%d] +\n", vdd->ndx);

	if (unlikely(!vdd->esd_recovery.esd_recovery_init))
		ss_event_esd_recovery_init(vdd, 0, NULL);

	if (unlikely(vdd->is_factory_mode &&
			vdd->dtsi_data.samsung_support_factory_panel_swap)) {
		/* LCD ID read every wake_up time incase of factory binary */
		vdd->manufacture_id_dsi = PBA_ID;

		/* Factory Panel Swap*/
		vdd->manufacture_date_loaded_dsi = 0;
		vdd->ddi_id_loaded_dsi = 0;
		vdd->module_info_loaded_dsi = 0;
		vdd->cell_id_loaded_dsi = 0;
		vdd->octa_id_loaded_dsi = 0;
		vdd->br_info.hbm_loaded_dsi = 0;
		vdd->mdnie_loaded_dsi = 0;
		vdd->br_info.smart_dimming_loaded_dsi = 0;
		vdd->br_info.smart_dimming_hmt_loaded_dsi = 0;
		vdd->br_info.table_interpolation_loaded = 0;
	}

	if (vdd->manufacture_id_dsi == PBA_ID || vdd->manufacture_id_dsi == -EINVAL) {
		u8 recv_buf[3];

		LCD_INFO("invalid ddi id from cmdline(0x%x), read ddi id\n", vdd->manufacture_id_dsi);

		/*
		*	At this time, panel revision it not selected.
		*	So last index(SUPPORT_PANEL_REVISION-1) used.
		*/
		vdd->panel_revision = SUPPORT_PANEL_REVISION-1;

		/*
		*	Some panel needs to update register at init time to read ID & MTP
		*	Such as, dual-dsi-control or sleep-out so on.
		*/
		if ((ss_get_cmds(vdd, RX_MANUFACTURE_ID)->count)) {
			ss_panel_data_read(vdd, RX_MANUFACTURE_ID,
					recv_buf, LEVEL1_KEY);
		} else {
			LCD_INFO("manufacture_read_pre_tx_cmds\n");
			ss_send_cmd(vdd, TX_MANUFACTURE_ID_READ_PRE);

			ss_panel_data_read(vdd, RX_MANUFACTURE_ID0,
					recv_buf, LEVEL1_KEY);
			ss_panel_data_read(vdd, RX_MANUFACTURE_ID1,
					recv_buf + 1, LEVEL1_KEY);
			ss_panel_data_read(vdd, RX_MANUFACTURE_ID2,
					recv_buf + 2, LEVEL1_KEY);
		}

		vdd->manufacture_id_dsi =
			(recv_buf[0] << 16) | (recv_buf[1] << 8) | recv_buf[2];
		LCD_INFO("manufacture id: 0x%x\n", vdd->manufacture_id_dsi);

		/* compare lcd_id from bootloader and kernel side for debugging */
		if (likely(!(vdd->is_factory_mode &&
				vdd->dtsi_data.samsung_support_factory_panel_swap))) {
			if (vdd->ndx == PRIMARY_DISPLAY_NDX)
				lcd_id_cmdline = get_lcd_attached("GET");
			else
				lcd_id_cmdline = get_lcd_attached_secondary("GET");

			if (vdd->manufacture_id_dsi != lcd_id_cmdline)
				LCD_ERR("LCD ID is changed (0x%X -> 0x%X)\n",
						lcd_id_cmdline, vdd->manufacture_id_dsi);
		}

		/* Panel revision selection */
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_revision))
			LCD_ERR("no panel_revision_selection_error function\n");
		else
			vdd->panel_func.samsung_panel_revision(vdd);

		LCD_INFO("Panel_Revision = %c %d\n", vdd->panel_revision + 'A', vdd->panel_revision);
	}

	/* Read panel status to check panel is ok from bootloader */
	if (!vdd->read_panel_status_from_lk) {
		int rddpm, rddsm, errfg, dsierror, protocol_err;

		rddpm = ss_read_rddpm(vdd);
		rddsm = ss_read_rddsm(vdd);
		errfg = ss_read_errfg(vdd);
		dsierror = ss_read_dsierr(vdd);
		protocol_err = ss_read_mipi_protocol_err(vdd);
		ss_read_pps_data(vdd);

		SS_XLOG(rddpm, rddsm, errfg, dsierror);
		LCD_INFO("panel dbg: %x %x %x %x %x\n", rddpm, rddsm, errfg, dsierror, protocol_err);

		vdd->read_panel_status_from_lk = 1;
	}

	if (vdd->skip_read_on_pre) {
		LCD_INFO("Skip read operation in on_pre\n");
		goto skip_read;
	}

	/* Module info */
	if (!vdd->module_info_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_module_info_read))
			LCD_ERR("no samsung_module_info_read function\n");
		else
			vdd->module_info_loaded_dsi = vdd->panel_func.samsung_module_info_read(vdd);
	}

	/* Manufacture date */
	if (!vdd->manufacture_date_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_manufacture_date_read))
			LCD_ERR("no samsung_manufacture_date_read function\n");
		else
			vdd->manufacture_date_loaded_dsi = vdd->panel_func.samsung_manufacture_date_read(vdd);
	}

	/* DDI ID */
	if (!vdd->ddi_id_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_ddi_id_read))
			LCD_ERR("no samsung_ddi_id_read function\n");
		else
			vdd->ddi_id_loaded_dsi = vdd->panel_func.samsung_ddi_id_read(vdd);
	}

	/* Panel Unique OCTA ID */
	if (!vdd->octa_id_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_octa_id_read))
			LCD_ERR("no samsung_octa_id_read function\n");
		else
			vdd->octa_id_loaded_dsi = vdd->panel_func.samsung_octa_id_read(vdd);
	}

	/* ELVSS read */
	if (!vdd->br_info.elvss_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_elvss_read))
			LCD_ERR("no samsung_elvss_read function\n");
		else
			vdd->br_info.elvss_loaded_dsi = vdd->panel_func.samsung_elvss_read(vdd);
	}

	/* IRC read */
	if (!vdd->br_info.irc_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_irc_read))
			LCD_ERR("no samsung_irc_read function\n");
		else
			vdd->br_info.irc_loaded_dsi = vdd->panel_func.samsung_irc_read(vdd);
	}

	/* HBM */
	if (!vdd->br_info.hbm_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_read))
			LCD_ERR("no samsung_hbm_read function\n");
		else
			vdd->br_info.hbm_loaded_dsi = vdd->panel_func.samsung_hbm_read(vdd);
	}

	/* MDNIE X,Y */
	if (!vdd->mdnie_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_mdnie_read))
			LCD_ERR("no samsung_mdnie_read function\n");
		else
			vdd->mdnie_loaded_dsi = vdd->panel_func.samsung_mdnie_read(vdd);
	}

	/* Panel Unique Cell ID */
	if (!vdd->cell_id_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_cell_id_read))
			LCD_ERR("no samsung_cell_id_read function\n");
		else
			vdd->cell_id_loaded_dsi = vdd->panel_func.samsung_cell_id_read(vdd);
	}

	/* Smart dimming*/
	if (vdd->br_info.flash_gamma_support && !vdd->br_info.smart_dimming_loaded_dsi) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_smart_dimming_init)) {
			LCD_ERR("no samsung_smart_dimming_init function\n");
		} else {
			int count;
			LCD_ERR("br_info.br_tbl_count %d \n", vdd->br_info.br_tbl_count);

			for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
				struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];

				vdd->br_info.smart_dimming_loaded_dsi =
					vdd->panel_func.samsung_smart_dimming_init(vdd, br_tbl);
			}
		}
	}

	/* Smart dimming for hmt */
	if (vdd->br_info.flash_gamma_support && vdd->dtsi_data.hmt_enabled) {
		if (!vdd->br_info.smart_dimming_hmt_loaded_dsi) {
			if (IS_ERR_OR_NULL(vdd->panel_func.samsung_smart_dimming_hmt_init))
				LCD_ERR("no samsung_smart_dimming_hmt_init function\n");
			else
				vdd->br_info.smart_dimming_hmt_loaded_dsi = vdd->panel_func.samsung_smart_dimming_hmt_init(vdd);
		}
	}

	/* copr */
	if (!vdd->copr_load_init_cmd && vdd->copr.copr_on) {
		vdd->copr_load_init_cmd = ss_get_copr_orig_cmd(vdd);
	}

	/* stm */
	if (!vdd->stm_load_init_cmd) {
		vdd->stm_load_init_cmd = ss_get_stm_orig_cmd(vdd);
	}

	/* gamma flash */
	if (vdd->br_info.flash_gamma_support &&
			!vdd->br_info.support_early_gamma_flash &&
			!vdd->br_info.flash_gamma_init_done) {

		if (!strcmp(vdd->br_info.flash_gamma_type, "flash"))
			__flash_br(vdd);
		else if (!strcmp(vdd->br_info.flash_gamma_type, "table"))
			__table_br(vdd);
	}

	/* gamma mode2 and HOP display */
	if (vdd->br_info.common_br.gamma_mode2_support) {
		struct flash_gm2 *gm2_table = &vdd->br_info.gm2_table;

		if (gm2_table->mtp_one_vbias_mode &&
				(vdd->is_factory_mode || !gm2_table->flash_gm2_init_done)) {
			/* Read one vbias mode from MTP.
			 * Factory binary has panel swap test. For that case, read vbias MTP data
			 * in every display on sequence.
			 */
			LCD_INFO("read vbias mtp data\n");
			ss_panel_data_read(vdd, RX_VBIAS_MTP, gm2_table->mtp_one_vbias_mode, LEVEL1_KEY);
		}

		if (vdd->panel_func.samsung_gm2_ddi_flash_prepare &&
				!gm2_table->flash_gm2_init_done) {
			LCD_INFO("init gamma mode2 ddi flash\n");
			vdd->panel_func.samsung_gm2_ddi_flash_prepare(vdd);
			gm2_table->flash_gm2_init_done = true;
		}

		if (vdd->panel_func.samsung_gm2_gamma_comp_init &&
				!vdd->br_info.gm2_mtp.mtp_gm2_init_done) {
			LCD_INFO("init gamma mode2 gamma compensation\n");
			vdd->panel_func.samsung_gm2_gamma_comp_init(vdd);
			vdd->br_info.gm2_mtp.mtp_gm2_init_done = true;
		}

	}

	/* table itp if it failed gamma flash */
	if (vdd->br_info.flash_gamma_support && !vdd->br_info.table_interpolation_loaded) {
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_interpolation_init)
			&& !vdd->br_info.flash_gamma_init_done) {
			int count;

			LCD_INFO("flash interpolation is not done, use table interpolation !\n");
			for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
				struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];

				table_br_func(vdd, br_tbl);
				vdd->br_info.table_interpolation_loaded =
					vdd->panel_func.samsung_interpolation_init(vdd,	br_tbl, TABLE_INTERPOLATION);
			}
			vdd->br_info.flash_gamma_init_done = true;
		}

		/* Even though flash interpolation was done, use table gamma for hmd.
		 * max mtp gamma for hmd is wrong from SDC.
		 */
		if (!IS_ERR_OR_NULL(vdd->panel_func.force_use_table_for_hmd)
			&& !vdd->br_info.force_use_table_for_hmd_done
			&& vdd->br_info.flash_gamma_init_done) {
			int count;

			for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
				struct brightness_table *br_tbl = &vdd->br_info.br_tbl[count];
				vdd->panel_func.force_use_table_for_hmd(vdd, br_tbl);
			}

			vdd->br_info.force_use_table_for_hmd_done = true;
		}
	}

skip_read:
	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_on_pre))
		vdd->panel_func.samsung_panel_on_pre(vdd);

	if (!vdd->samsung_splash_enabled)
		ss_send_cmd(vdd, TX_ON_PRE_SEQ);
	else
		LCD_INFO("splash booting.. do not send ON_PRE_SEQ..\n");

	LCD_INFO("[DISPLAY_%d] -\n", vdd->ndx);

	return true;
}

int ss_panel_on_post(struct samsung_display_driver_data *vdd)
{
	if (!ss_panel_attach_get(vdd)) {
		LCD_ERR("ss_panel_attach_get NG\n");
		return false;
	}

	LCD_INFO("[DISPLAY_%d] +\n", vdd->ndx);

/* TODO: enable debug file and activate below...
	ss_read_self_diag(vdd);
*/

	if (vdd->support_cabc && !vdd->br_info.common_br.auto_level)
		ss_cabc_update(vdd);
	else if (vdd->br_info.ss_panel_tft_outdoormode_update && vdd->br_info.common_br.auto_level)
		vdd->br_info.ss_panel_tft_outdoormode_update(vdd);
	else if (vdd->support_cabc && vdd->br_info.common_br.auto_level)
		ss_tft_autobrightness_cabc_update(vdd);

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_on_post))
		vdd->panel_func.samsung_panel_on_post(vdd);

#if 0	// ccb is set by mdnie cmd.
	if ((vdd->panel_func.color_weakness_ccb_on_off) && vdd->color_weakness_mode)
		vdd->panel_func.color_weakness_ccb_on_off(vdd, vdd->color_weakness_mode);
#endif

	if (vdd->support_hall_ic) {
		/*
		* Brightenss cmds sent by samsung_display_hall_ic_status() at panel switching operation
		*/
		if (ss_is_bl_dcs(vdd) &&
			(vdd->hall_ic_mode_change_trigger == false))
			ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	} else {
		if (ss_is_bl_dcs(vdd)) {
			struct backlight_device *bd = GET_SDE_BACKLIGHT_DEVICE(vdd);

			/* In case of backlight update in panel off,
			 * dsi_display_set_backlight() returns error
			 * without updating vdd->br_info.common_br.bl_level.
			 * Update bl_level from bd->props.brightness.
			 */
			if (bd && vdd->br_info.common_br.bl_level != bd->props.brightness) {
				LCD_INFO("update bl_level: %d -> %d\n",
					vdd->br_info.common_br.bl_level, bd->props.brightness);
				vdd->br_info.common_br.bl_level = bd->props.brightness;
			}

			ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
		}
	}

	if (vdd->mdnie.support_mdnie) {
		vdd->mdnie.lcd_on_notifiy = true;
		update_dsi_tcon_mdnie_register(vdd);
		if (vdd->mdnie.support_trans_dimming)
			vdd->mdnie.disable_trans_dimming = false;
	}

	/*
	 * Update Cover Control Status every Normal sleep & wakeup
	 * Do not update Cover_control at this point in case of AOD.
	 * Because, below update is done before entering AOD.
	 */
	if (vdd->panel_func.samsung_cover_control && vdd->cover_control
		&& !ss_is_panel_lpm(vdd))
		vdd->panel_func.samsung_cover_control(vdd);

	/* Work around: For folder model, the status bar resizes itself and old UI appears in sub-panel
	 * before premium watch or screen lock is on, so it needs to skip old UI.
	 */
	if (!vdd->lcd_flip_not_refresh &&
			vdd->support_hall_ic &&
			vdd->hall_ic_mode_change_trigger &&
			vdd->lcd_flip_delay_ms) {
		schedule_delayed_work(&vdd->delay_disp_on_work, msecs_to_jiffies(vdd->lcd_flip_delay_ms));
	} else
		vdd->display_status_dsi.wait_disp_on = true;

	vdd->display_status_dsi.wait_actual_disp_on = true;
	vdd->panel_lpm.need_self_grid = true;

	if (vdd->dyn_mipi_clk.is_support) {
		LCD_INFO("FFC Setting for Dynamic MIPI Clock\n");
		ss_send_cmd(vdd, TX_FFC);
	}

	if (vdd->copr.copr_on)
		ss_send_cmd(vdd, TX_COPR_ENABLE);

	if (unlikely(vdd->is_factory_mode)) {
		ss_send_cmd(vdd, TX_FD_ON);
		/*
		* 1Frame Delay(33.4ms - 30FPS) Should be added for the project that needs FD cmd
		* Swire toggled at TX_FD_OFF, TX_LPM_ON, TX_FD_ON, TX_LPM_OFF.
		*/
		if(!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_FD_ON)))
			usleep_range(34*1000, 34*1000);
	}

	/*
	 * ESD Enable should be called after all the cmd's tx is done.
	 * Some command may occur unexpected esd detect because of EL_ON signal control
	 */
	if (ss_is_esd_check_enabled(vdd)) {
		if (vdd->esd_recovery.esd_irq_enable)
			vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd);
	}

	ss_set_panel_state(vdd, PANEL_PWR_ON);

	if (vdd->poc_driver.check_read_case)
		vdd->poc_driver.read_case = vdd->poc_driver.check_read_case(vdd);

	LCD_INFO("[DISPLAY_%d] -\n", vdd->ndx);

	return true;
}

int ss_panel_off_pre(struct samsung_display_driver_data *vdd)
{
	int rddpm, rddsm, errfg, dsierror, protocol_err;
	int ret = 0;

	LCD_INFO("[DISPLAY_%d] +\n", vdd->ndx);
	rddpm = ss_read_rddpm(vdd);
	rddsm = ss_read_rddsm(vdd);
	errfg = ss_read_errfg(vdd);
	dsierror = ss_read_dsierr(vdd);
	protocol_err = ss_read_mipi_protocol_err(vdd);
	ss_read_pps_data(vdd);
	SS_XLOG(rddpm, rddsm, errfg, dsierror);
	LCD_INFO("panel dbg: %x %x %x %x %x\n", rddpm, rddsm, errfg, dsierror, protocol_err);

	if (ss_is_esd_check_enabled(vdd)) {
		vdd->esd_recovery.is_wakeup_source = false;
		if (vdd->esd_recovery.esd_irq_enable)
			vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd);
		vdd->esd_recovery.is_enabled_esd_recovery = false;
	}

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_off_pre))
		vdd->panel_func.samsung_panel_off_pre(vdd);

	mutex_lock(&vdd->vdd_lock);
	ss_set_panel_state(vdd, PANEL_PWR_OFF);
	mutex_unlock(&vdd->vdd_lock);

	LCD_INFO("[DISPLAY_%d] -\n", vdd->ndx);

	return ret;
}

/*
 * Do not call ss_send_cmd() or ss_panel_data_read() here.
 * Any MIPI Tx/Rx can not be alowed in here.
 */
int ss_panel_off_post(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	LCD_INFO("[DISPLAY_%d] +\n", vdd->ndx);

	if (vdd->mdnie.support_trans_dimming)
		vdd->mdnie.disable_trans_dimming = true;

	if (vdd->support_hall_ic && vdd->lcd_flip_delay_ms)
		cancel_delayed_work(&vdd->delay_disp_on_work);

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_off_post))
		vdd->panel_func.samsung_panel_off_post(vdd);

	/* gradual acl on/off - not used */
//	vdd->gradual_pre_acl_on = GRADUAL_ACL_UNSTABLE;

	/* Reset Self Display Status */
	if (vdd->self_disp.is_support) {
		vdd->self_disp.sa_info.en = false;
		vdd->self_disp.sd_info.en = false;
		vdd->self_disp.si_info.en = false;
		vdd->self_disp.sg_info.en = false;
		vdd->self_disp.time_set = false;
		vdd->self_disp.on = false;
	}

	if (vdd->finger_mask)
		vdd->finger_mask = false;

	/* To prevent panel off without finger off */
	if (vdd->br_info.common_br.finger_mask_hbm_on)
		vdd->br_info.common_br.finger_mask_hbm_on = false;

	LCD_INFO("[DISPLAY_%d] -\n", vdd->ndx);
	SS_XLOG(vdd->cnt_mdp_clk_underflow);

	return ret;
}

/*************************************************************
*
*		TFT BACKLIGHT GPIO FUNCTION BELOW.
*
**************************************************************/
int ss_backlight_tft_request_gpios(struct samsung_display_driver_data *vdd)
{
	int rc = 0, i;
	/*
	 * gpio_name[] named as gpio_name + num(recomend as 0)
	 * because of the num will increase depend on number of gpio
	 */
	char gpio_name[17] = "disp_bcklt_gpio0";
	static u8 gpio_request_status = -EINVAL;

	if (!gpio_request_status)
		goto end;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data pinfo : 0x%zx\n", (size_t)vdd);
		goto end;
	}

	for (i = 0; i < MAX_BACKLIGHT_TFT_GPIO; i++) {
		if (gpio_is_valid(vdd->dtsi_data.backlight_tft_gpio[i])) {
			rc = gpio_request(vdd->dtsi_data.backlight_tft_gpio[i],
							gpio_name);
			if (rc) {
				LCD_ERR("request %s failed, rc=%d\n", gpio_name, rc);
				goto tft_backlight_gpio_err;
			}
		}
	}

	gpio_request_status = rc;
end:
	return rc;
tft_backlight_gpio_err:
	if (i) {
		do {
			if (gpio_is_valid(vdd->dtsi_data.backlight_tft_gpio[i]))
				gpio_free(vdd->dtsi_data.backlight_tft_gpio[i--]);
			LCD_ERR("i = %d\n", i);
		} while (i > 0);
	}

	return rc;
}

int ss_backlight_tft_gpio_config(struct samsung_display_driver_data *vdd, int enable)
{
	int ret = 0, i = 0, add_value = 1;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data pinfo : 0x%zx\n",  (size_t)vdd);
		goto end;
	}

	LCD_INFO("[DISPLAY_%d] + enable(%d) ndx(%d)\n", vdd->ndx, enable, ss_get_display_ndx(vdd));

	if (ss_backlight_tft_request_gpios(vdd)) {
		LCD_ERR("fail to request tft backlight gpios");
		goto end;
	}

	LCD_DEBUG("%s tft backlight gpios\n", enable ? "enable" : "disable");

	/*
	 * The order of backlight_tft_gpio enable/disable
	 * 1. Enable : backlight_tft_gpio[0], [1], ... [MAX_BACKLIGHT_TFT_GPIO - 1]
	 * 2. Disable : backlight_tft_gpio[MAX_BACKLIGHT_TFT_GPIO - 1], ... [1], [0]
	 */
	if (!enable) {
		add_value = -1;
		i = MAX_BACKLIGHT_TFT_GPIO - 1;
	}

	do {
		if (gpio_is_valid(vdd->dtsi_data.backlight_tft_gpio[i])) {
			gpio_set_value(vdd->dtsi_data.backlight_tft_gpio[i], enable);
			LCD_DEBUG("set backlight tft gpio[%d] to %s\n",
						 vdd->dtsi_data.backlight_tft_gpio[i],
						enable ? "high" : "low");
			usleep_range(500, 500);
		}
	} while (((i += add_value) < MAX_BACKLIGHT_TFT_GPIO) && (i >= 0));

end:
	LCD_INFO("[DISPLAY_%d] -\n", vdd->ndx);
	return ret;
}

/*************************************************************
*
*		ESD RECOVERY RELATED FUNCTION BELOW.
*
**************************************************************/

#if 0
static int ss_esd_check_status(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("lcd esd - check_ststus\n");
	return 0;
}
static int ss_esd_read_status(struct mdss_dsi_ctrl_pdata *ctrl)
{
	LCD_INFO("lcd esd - check_read_status\n");
	// esd status must return 0 to go status_dead(blank->unblnk) in ss_check_dsi_ctrl_status.
	return 0;
}
#endif

/*
 * esd_irq_enable() - Enable or disable esd irq.
 *
 * @enable	: flag for enable or disabled
 * @nosync	: flag for disable irq with nosync
 * @data	: point ot struct ss_panel_info
 */
#define IRQS_PENDING	0x00000200
#define istate core_internal_state__do_not_mess_with_it
static void esd_irq_enable(bool enable, bool nosync, void *data)
{
	/* The irq will enabled when do the request_threaded_irq() */
	static bool is_enabled[MAX_DISPLAY_NDX] = {true, true};
	static bool is_wakeup_source[MAX_DISPLAY_NDX];
	int gpio;
	int irq[MAX_ESD_GPIO] = {0,};
	unsigned long flags;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)data;
	struct irq_desc *desc = NULL;
	u8 i = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return;
	}

	for (i = 0; i < vdd->esd_recovery.num_of_gpio; i++) {
		gpio = vdd->esd_recovery.esd_gpio[i];

		if (!gpio_is_valid(gpio)) {
			LCD_ERR("Invalid ESD_GPIO : %d\n", gpio);
			continue;
		}
		irq[i] = gpio_to_irq(gpio);
	}

	spin_lock_irqsave(&vdd->esd_recovery.irq_lock, flags);

	if (enable == is_enabled[vdd->ndx]) {
		LCD_INFO("ESD irq already %s\n",
				enable ? "enabled" : "disabled");
		goto config_wakeup_source;
	}


	for (i = 0; i < vdd->esd_recovery.num_of_gpio; i++) {
		gpio = vdd->esd_recovery.esd_gpio[i];
		desc = irq_to_desc(irq[i]);
		if (enable) {
			/* clear esd irq triggered while it was disabled. */
			if (desc->istate & IRQS_PENDING) {
				LCD_INFO("clear esd irq pending status\n");
				desc->istate &= ~IRQS_PENDING;
			}
		}

		if (!gpio_is_valid(gpio)) {
			LCD_ERR("Invalid ESD_GPIO : %d\n", gpio);
			continue;
		}

		if (enable) {
			is_enabled[vdd->ndx] = true;
			enable_irq(irq[i]);
		} else {
			if (nosync)
				disable_irq_nosync(irq[i]);
			else
				disable_irq(irq[i]);
			is_enabled[vdd->ndx] = false;
		}
	}

	/* TODO: Disable log if the esd function stable */
	LCD_INFO("ndx=%d, ESD irq %s with %s\n",
				vdd->ndx, enable ? "enabled" : "disabled",
				nosync ? "nosync" : "sync");

config_wakeup_source:
	if (vdd->esd_recovery.is_wakeup_source == is_wakeup_source[vdd->ndx]) {
		LCD_DEBUG("[ESD] IRQs are already irq_wake %s\n",
				is_wakeup_source[vdd->ndx] ? "enabled" : "disabled");
		goto end;
	}

	for (i = 0; i < vdd->esd_recovery.num_of_gpio; i++) {
		gpio = vdd->esd_recovery.esd_gpio[i];

		if (!gpio_is_valid(gpio)) {
			LCD_ERR("Invalid ESD_GPIO : %d\n", gpio);
			continue;
		}

		is_wakeup_source[vdd->ndx] =
			vdd->esd_recovery.is_wakeup_source;

		if (is_wakeup_source[vdd->ndx])
			enable_irq_wake(irq[i]);
		else
			disable_irq_wake(irq[i]);
	}

	LCD_INFO("[ESD] ndx=%d, IRQs are set to irq_wake %s\n",
				vdd->ndx, is_wakeup_source[vdd->ndx] ? "enabled" : "disabled");

end:
	spin_unlock_irqrestore(&vdd->esd_recovery.irq_lock, flags);
}

static irqreturn_t ss_ub_con_det_handler(int irq, void *handle);

static irqreturn_t esd_irq_handler(int irq, void *handle)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *) handle;
	struct sde_connector *conn = GET_SDE_CONNECTOR(vdd);
	struct esd_recovery *esd = NULL;
	int i;

	if (!vdd->esd_recovery.is_enabled_esd_recovery) {
		LCD_ERR("esd recovery is not enabled yet");
		goto end;
	}

	if (unlikely(vdd->is_factory_mode)) {
		if (gpio_is_valid(vdd->ub_con_det.gpio)) {
			LCD_INFO("ub_con_det.gpio = %d\n", gpio_get_value(vdd->ub_con_det.gpio));
		}
	}

	LCD_INFO("[DISPLAY_%d] ++\n", vdd->ndx);

	esd_irq_enable(false, true, (void *)vdd);

	vdd->panel_lpm.esd_recovery = true;

	schedule_work(&conn->status_work.work);

	LCD_INFO("Panel Recovery(ESD, irq%d), Trial Count = %d\n", irq, vdd->panel_recovery_cnt++);
	SS_XLOG(vdd->panel_recovery_cnt);
	inc_dpui_u32_field(DPUI_KEY_QCT_RCV_CNT, 1);

	if (vdd->is_factory_mode) {
		esd = &vdd->esd_recovery;

		for (i = 0; i < esd->num_of_gpio; i++) {
			if ((esd->esd_gpio[i] == vdd->ub_con_det.gpio) &&
				(gpio_to_irq(esd->esd_gpio[i]) == irq)) {
				ss_ub_con_det_handler(irq, handle);
			}
		}
	}

	LCD_INFO("[DISPLAY_%d] --\n", vdd->ndx);

end:
	return IRQ_HANDLED;
}

// refer to dsi_display_res_init() and dsi_panel_get(&display->pdev->dev, display->panel_of);
static void ss_panel_parse_dt_esd(struct device_node *np,
		struct samsung_display_driver_data *vdd)
{
	int rc = 0;
	const char *data;
	char esd_irq_gpio[] = "samsung,esd-irq-gpio1";
	char esd_irqflag[] = "qcom,mdss-dsi-panel-status-irq-trigger1";
	struct esd_recovery *esd = NULL;
	struct dsi_panel *panel = NULL;
	struct drm_panel_esd_config *esd_config = NULL;
	u8 i = 0;

	panel = (struct dsi_panel *)vdd->msm_private;
	esd_config = &panel->esd_config;

	esd = &vdd->esd_recovery;
	esd->num_of_gpio = 0;

	esd_config->esd_enabled = of_property_read_bool(np,
		"qcom,esd-check-enabled");

	if (!esd_config->esd_enabled)
		goto end;

	for (i = 0; i < MAX_ESD_GPIO; i++) {
		esd_irq_gpio[strlen(esd_irq_gpio) - 1] = '1' + i;
		esd->esd_gpio[esd->num_of_gpio] = of_get_named_gpio(np,
				esd_irq_gpio, 0);

		if (gpio_is_valid(esd->esd_gpio[esd->num_of_gpio])) {
			LCD_INFO("[ESD] gpio : %d, irq : %d\n",
					esd->esd_gpio[esd->num_of_gpio],
					gpio_to_irq(esd->esd_gpio[esd->num_of_gpio]));
			esd->num_of_gpio++;
		}
	}

	if (vdd->is_factory_mode && gpio_is_valid(vdd->fg_err_gpio)) {
		esd->esd_gpio[esd->num_of_gpio] = vdd->fg_err_gpio;
		esd->num_of_gpio++;
		LCD_INFO("add fg_err esd gpio(%d), num_of_gpio: %d\n",
				vdd->fg_err_gpio, esd->num_of_gpio);
	}

	rc = of_property_read_string(np, "qcom,mdss-dsi-panel-status-check-mode", &data);
	if (!rc) {
		if (!strcmp(data, "irq_check"))
			esd_config->status_mode = ESD_MODE_PANEL_IRQ;
		else
			LCD_ERR("No valid panel-status-check-mode string\n");
	}

	for (i = 0; i < esd->num_of_gpio; i++) {
		/* Without ONESHOT setting, it fails in request_threaded_irq()
		 * with 'handler=NULL' error log
		 */
		esd->irqflags[i] = IRQF_ONESHOT | IRQF_NO_SUSPEND; /* default setting */

		esd_irqflag[strlen(esd_irqflag) - 1] = '1' + i;
		rc = of_property_read_string(np, esd_irqflag, &data);
		if (!rc) {
			if (!strcmp(data, "rising"))
				esd->irqflags[i] |= IRQF_TRIGGER_RISING;
			else if (!strcmp(data, "falling"))
				esd->irqflags[i] |= IRQF_TRIGGER_FALLING;
			else if (!strcmp(data, "high"))
				esd->irqflags[i] |= IRQF_TRIGGER_HIGH;
			else if (!strcmp(data, "low"))
				esd->irqflags[i] |= IRQF_TRIGGER_LOW;
		} else {
			LCD_ERR("[%d] fail to find ESD irq flag (gpio%d). set default as FALLING..\n",
					i, esd->esd_gpio[i]);
			esd->irqflags[i] |= IRQF_TRIGGER_FALLING;
		}
	}

end:
	LCD_INFO("samsung esd %s mode (%d)\n",
		esd_config->esd_enabled ? "enabled" : "disabled",
		esd_config->status_mode);
	return;
}

static void ss_event_esd_recovery_init(
		struct samsung_display_driver_data *vdd, int event, void *arg)
{
	int ret;
	u8 i;
	int gpio, irqflags;
	struct esd_recovery *esd = NULL;
	struct dsi_panel *panel = NULL;
	struct drm_panel_esd_config *esd_config = NULL;

	panel = (struct dsi_panel *)vdd->msm_private;
	esd_config = &panel->esd_config;

	esd = &vdd->esd_recovery;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return;
	}

	LCD_INFO("[DISPLAY_%d] ++\n", vdd->ndx);

	if (unlikely(!esd->esd_recovery_init)) {
		esd->esd_recovery_init = true;
		esd->esd_irq_enable = esd_irq_enable;
		if (esd_config->status_mode  == ESD_MODE_PANEL_IRQ) {
			for (i = 0; i < esd->num_of_gpio; i++) {
				/* Set gpio num and irqflags */
				gpio = esd->esd_gpio[i];
				irqflags = esd->irqflags[i];
				if (!gpio_is_valid(gpio)) {
					LCD_ERR("[ESD] Invalid GPIO : %d\n", gpio);
					continue;
				}

				gpio_request(gpio, "esd_recovery");
				ret = request_threaded_irq(
						gpio_to_irq(gpio),
						NULL,
						esd_irq_handler,
						irqflags,
						"esd_recovery",
						(void *)vdd);
				if (ret)
					LCD_ERR("Failed to request_irq, ret=%d\n",
							ret);
				else
					LCD_INFO("request esd irq !!\n");
			}
			esd_irq_enable(false, true, (void *)vdd);
		}
	}

	LCD_INFO("[DISPLAY_%d] --\n", vdd->ndx);
}

static void ss_panel_recovery(struct samsung_display_driver_data *vdd)
{
	struct sde_connector *conn = GET_SDE_CONNECTOR(vdd);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return;
	}

	if (!vdd->esd_recovery.is_enabled_esd_recovery) {
		LCD_ERR("esd recovery is not enabled yet");
		return;
	}
	LCD_INFO("Panel Recovery, Trial Count = %d\n", vdd->panel_recovery_cnt++);
	SS_XLOG(vdd->panel_recovery_cnt);
	inc_dpui_u32_field(DPUI_KEY_QCT_RCV_CNT, 1);

	esd_irq_enable(false, true, (void *)vdd);
	vdd->panel_lpm.esd_recovery = true;
	schedule_work(&conn->status_work.work);

	LCD_INFO("Panel Recovery --\n");

}

void ss_send_ub_uevent(struct samsung_display_driver_data *vdd)
{
	char *envp[3] = {"CONNECTOR_NAME=UB_CONNECT", "CONNECTOR_TYPE=HIGH_LEVEL", NULL};
	char *envp_sub[3] = {"CONNECTOR_NAME=UB_CONNECT_SUB", "CONNECTOR_TYPE=HIGH_LEVEL", NULL};

	LCD_INFO("[%s] send uvent \n", vdd->ndx == PRIMARY_DISPLAY_NDX ? "UB_CONNECT" : "UB_CONNECT_SUB");

    if (vdd->ndx == PRIMARY_DISPLAY_NDX)
        kobject_uevent_env(&vdd->lcd_dev->dev.kobj, KOBJ_CHANGE, envp);
    else
        kobject_uevent_env(&vdd->lcd_dev->dev.kobj, KOBJ_CHANGE, envp_sub);

	return;
}

int ub_con_det_status(int index)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(index);

	/* high gpio value means panel detached */
	if (vdd && gpio_is_valid(vdd->ub_con_det.gpio)) {
		if (vdd->ub_con_det.current_wakeup_context_gpio_status ||
			gpio_get_value(vdd->ub_con_det.gpio)) {
			return 1; /* detach */
		} else
			return 0; /* attach */
	} else {
		return 0; /* attach */
	}
}

static irqreturn_t ss_ub_con_det_handler(int irq, void *handle)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *) handle;
	struct panel_ub_con_event_data data;
	bool connected;

	connected = !gpio_get_value(vdd->ub_con_det.gpio); /* gpio low: connected */
	data.state = connected ? PANEL_EVENT_UB_CON_CONNECTED :
				PANEL_EVENT_UB_CON_DISCONNECTED;
	data.display_idx = vdd->ndx;

	LCD_INFO("ub_con_det is [%s] [%s]\n",
			vdd->ub_con_det.enabled ? "enabled" : "disabled",
			connected ? "connected" : "disconnected");

	ss_panel_notifier_call_chain(PANEL_EVENT_UB_CON_CHANGED, &data);

	if (connected)
		return IRQ_HANDLED;

	if (vdd->ub_con_det.enabled)
		ss_send_ub_uevent(vdd);

	vdd->ub_con_det.ub_con_cnt++;

	LCD_ERR("-- cnt : %d\n", vdd->ub_con_det.ub_con_cnt);

	return IRQ_HANDLED;
}

static void ss_panel_parse_dt_ub_con(struct device_node *np,
		struct samsung_display_driver_data *vdd)
{
	struct ub_con_detect *ub_con_det;
	struct esd_recovery *esd = NULL;
	int ret, i;

	ub_con_det = &vdd->ub_con_det;

	ub_con_det->gpio = of_get_named_gpio(np,
			"samsung,ub-con-det", 0);

	esd = &vdd->esd_recovery;
	if (vdd->is_factory_mode) {
		for (i = 0; i < esd->num_of_gpio; i++) {
			if (esd->esd_gpio[i] == ub_con_det->gpio) {
				LCD_INFO("[ub_con_det] gpio : %d share esd gpio : %d\n",
					ub_con_det->gpio, esd->esd_gpio[i]);
				return;
			}
		}
	}

	if (gpio_is_valid(ub_con_det->gpio)) {
		gpio_request(ub_con_det->gpio, "UB_CON_DET");
		LCD_INFO("[ub_con_det] gpio : %d, irq : %d\n",
				ub_con_det->gpio, gpio_to_irq(ub_con_det->gpio));
	} else {
		LCD_ERR("fail to gpio_request\n");
		return;
	}

	ret = request_threaded_irq(
				gpio_to_irq(ub_con_det->gpio),
				NULL,
				ss_ub_con_det_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"UB_CON_DET",
				(void *) vdd);
	if (ret)
		LCD_ERR("Failed to request_irq, ret=%d\n", ret);

	return;
}

/*************************************************************
*
*		OSC TE FITTING RELATED FUNCTION BELOW.
*
**************************************************************/
static void ss_event_osc_te_fitting(
		struct samsung_display_driver_data *vdd, int event, void *arg)
{
	struct osc_te_fitting_info *te_info = NULL;
	int ret, i, lut_count;
	unsigned int disp_te_gpio;

	te_info = &vdd->te_fitting_info;

	if (IS_ERR_OR_NULL(te_info)) {
		LCD_ERR("Invalid te data : 0x%zx\n",
				(size_t)te_info);
		return;
	}

	if (ss_is_seamless_mode(vdd)) {
		LCD_ERR("cont splash enabled\n");
		return;
	}

	if (!ss_is_vsync_enabled(vdd)) {
		LCD_DEBUG("vsync handler does not enabled yet\n");
		return;
	}

	te_info->status |= TE_FITTING_DONE;

	LCD_DEBUG("[DISPLAY_%d] ++\n", vdd->ndx);

	disp_te_gpio = ss_get_te_gpio(vdd);

	if (!(te_info->status & TE_FITTING_REQUEST_IRQ)) {
		te_info->status |= TE_FITTING_REQUEST_IRQ;


		ret = request_threaded_irq(
				gpio_to_irq(disp_te_gpio),
				samsung_te_check_handler,
				NULL,
				IRQF_TRIGGER_FALLING,
				"VSYNC_GPIO",
				(void *) vdd);
		if (ret)
			LCD_ERR("Failed to request_irq, ret=%d\n",
					ret);
		else
			disable_irq(gpio_to_irq(disp_te_gpio));
		te_info->te_time =
			kzalloc(sizeof(long long) * te_info->sampling_rate, GFP_KERNEL);
		INIT_WORK(&te_info->work, samsung_te_check_done_work);
	}

	for (lut_count = 0; lut_count < OSC_TE_FITTING_LUT_MAX; lut_count++) {
		init_completion(&te_info->te_check_comp);
		te_info->status |= TE_CHECK_ENABLE;
		te_info->te_duration = 0;

		LCD_DEBUG("osc_te_fitting _irq : %d\n",
				gpio_to_irq(disp_te_gpio));

		enable_irq(gpio_to_irq(disp_te_gpio));
		ret = wait_for_completion_timeout(
				&te_info->te_check_comp, 1000);

		if (ret <= 0)
			LCD_ERR("timeout\n");

		for (i = 0; i < te_info->sampling_rate; i++) {
			te_info->te_duration +=
				(i != 0 ? (te_info->te_time[i] - te_info->te_time[i-1]) : 0);
			LCD_DEBUG("vsync time : %lld, sum : %lld\n",
					te_info->te_time[i], te_info->te_duration);
		}
		do_div(te_info->te_duration, te_info->sampling_rate - 1);
		LCD_INFO("ave vsync time : %lld\n",
				te_info->te_duration);
		te_info->status &= ~TE_CHECK_ENABLE;

		if (vdd->panel_func.samsung_osc_te_fitting)
			ret = vdd->panel_func.samsung_osc_te_fitting(vdd);

		if (!ret)
			ss_send_cmd(vdd, TX_OSC_TE_FITTING);
		else
			break;
	}
	LCD_DEBUG("[DISPLAY_%d] --\n", vdd->ndx);
}

static void samsung_te_check_done_work(struct work_struct *work)
{
	struct osc_te_fitting_info *te_info = NULL;

	te_info = container_of(work, struct osc_te_fitting_info, work);

	if (IS_ERR_OR_NULL(te_info)) {
		LCD_ERR("Invalid TE tuning data\n");
		return;
	}

	complete_all(&te_info->te_check_comp);
}

static irqreturn_t samsung_te_check_handler(int irq, void *handle)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct osc_te_fitting_info *te_info = NULL;
	static bool skip_first_te = true;
	static u8 count;

	if (skip_first_te) {
		skip_first_te = false;
		goto end;
	}

	if (IS_ERR_OR_NULL(handle)) {
		LCD_ERR("handle is null\n");
		goto end;
	}

	te_info = &vdd->te_fitting_info;


	if (!(te_info->status & TE_CHECK_ENABLE))
		goto end;

	if (count < te_info->sampling_rate) {
		te_info->te_time[count++] =
			ktime_to_us(ktime_get());
	} else {
		disable_irq_nosync(gpio_to_irq(ss_get_te_gpio(vdd)));
		schedule_work(&te_info->work);
		skip_first_te = true;
		count = 0;
	}

end:
	return IRQ_HANDLED;
}

/*************************************************************
*
*		LDI FPS RELATED FUNCTION BELOW.
*
**************************************************************/
int ldi_fps(struct samsung_display_driver_data *vdd, unsigned int input_fps)
{
	int rc = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return 0;
	}

	LCD_INFO("input_fps = %d\n", input_fps);

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_change_ldi_fps))
		rc = vdd->panel_func.samsung_change_ldi_fps(vdd, input_fps);
	else {
		LCD_ERR("samsung_change_ldi_fps function is NULL\n");
		return 0;
	}

	if (rc)
		ss_send_cmd(vdd, TX_LDI_FPS_CHANGE);

	return rc;
}
EXPORT_SYMBOL(ldi_fps);

int ss_set_backlight(struct samsung_display_driver_data *vdd, u32 bl_lvl)
{
	struct dsi_panel *panel = NULL;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		ret = -EINVAL;
		goto end;
	}

	panel = GET_DSI_PANEL(vdd);
	if (IS_ERR_OR_NULL(panel)) {
		LCD_ERR("panel is NULL\n");
		ret = -EINVAL;
		goto end;
	}

	ret = dsi_panel_set_backlight(panel, bl_lvl);

end:
	return ret;
}

/*************************************************************
*
*		HMT RELATED FUNCTION BELOW.
*
**************************************************************/
int hmt_bright_update(struct samsung_display_driver_data *vdd)
{
	if (vdd->br_info.hmt_stat.hmt_on) {
		ss_brightness_dcs_hmt(vdd, vdd->br_info.hmt_stat.hmt_bl_level);
	} else {
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
		LCD_INFO("hmt off state!\n");
	}

	return 0;
}

int hmt_enable(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("[HMT] HMT %s\n", vdd->br_info.hmt_stat.hmt_on ? "ON" : "OFF");

	if (vdd->br_info.hmt_stat.hmt_on) {
		ss_send_cmd(vdd, TX_HMT_ENABLE);
	} else {
		ss_send_cmd(vdd, TX_HMT_DISABLE);
	}

	return 0;
}

int hmt_reverse_update(struct samsung_display_driver_data *vdd, int enable)
{
	LCD_INFO("[HMT] HMT %s\n", enable ? "REVERSE" : "FORWARD");

	if (enable)
		ss_send_cmd(vdd, TX_HMT_REVERSE);
	else
		ss_send_cmd(vdd, TX_HMT_FORWARD);

	return 0;
}

static void parse_dt_data(struct device_node *np, void *data, size_t size,
		char *cmd_string, char panel_rev,
		int (*fnc)(struct device_node *, void *, char *))
{
	char string[PARSE_STRING];
	int ret = 0;

	/* Generate string to parsing from DT */
	snprintf(string, PARSE_STRING, "%s%c", cmd_string, 'A' + panel_rev);

	ret = fnc(np, data, string);

	/* If there is no dtsi data for panel rev B ~ T,
	 * use valid previous panel rev dtsi data.
	 * TODO: Instead of copying all data from previous panel rev,
	 * copy only the pointer...
	 */
	if (ret && (panel_rev > 0))
		memcpy(data, (u8 *) data - size, size);
}

static void ss_panel_parse_dt_bright_tables(struct device_node *np,
		struct samsung_display_driver_data *vdd)
{
	struct ss_brightness_info *info = &vdd->br_info;
	int panel_rev;

	for (panel_rev = 0; panel_rev < SUPPORT_PANEL_REVISION; panel_rev++) {
		parse_dt_data(np, &info->vint_map_table[panel_rev],
				sizeof(struct cmd_map),
				"samsung,vint_map_table_rev", panel_rev,
				ss_parse_panel_table); /* VINT TABLE */

		parse_dt_data(np, &info->candela_map_table[NORMAL][panel_rev],
				sizeof(struct candela_map_table),
				"samsung,candela_map_table_rev", panel_rev,
				ss_parse_candella_mapping_table);

#if (defined(CONFIG_MACH_X1Q_JPN_SINGLE) || \
		(defined(CONFIG_MACH_Y2Q_JPN_SINGLE) && !defined(CONFIG_MACH_Y2Q_JPN_DCMOLY)) || \
		defined(CONFIG_MACH_BLOOMXQ_JPN_SINGLE))
		LCD_INFO("parse jpn AOD brightness table\n");
		parse_dt_data(np, &info->candela_map_table[AOD][panel_rev],
				sizeof(struct candela_map_table),
				"samsung,aod_candela_map_table_jpn_rev", panel_rev,
				ss_parse_candella_mapping_table);
#else
		parse_dt_data(np, &info->candela_map_table[AOD][panel_rev],
				sizeof(struct candela_map_table),
				"samsung,aod_candela_map_table_rev", panel_rev,
				ss_parse_candella_mapping_table);
#endif

		parse_dt_data(np, &info->candela_map_table[HBM][panel_rev],
				sizeof(struct candela_map_table),
				"samsung,hbm_candela_map_table_rev", panel_rev,
				ss_parse_hbm_candella_mapping_table);

		parse_dt_data(np, &info->candela_map_table[PAC_NORMAL][panel_rev],
				sizeof(struct candela_map_table),
				"samsung,pac_candela_map_table_rev", panel_rev,
				ss_parse_pac_candella_mapping_table);

		parse_dt_data(np, &info->candela_map_table[PAC_HBM][panel_rev],
				sizeof(struct candela_map_table),
				"samsung,pac_hbm_candela_map_table_rev", panel_rev,
				ss_parse_hbm_candella_mapping_table);

		parse_dt_data(np, &info->candela_map_table[MULTI_TO_ONE_NORMAL][panel_rev],
				sizeof(struct candela_map_table),
				"samsung,multi_to_one_candela_map_table_rev", panel_rev,
				ss_parse_multi_to_one_candella_mapping_table);

		/* ACL */
		parse_dt_data(np, &info->acl_map_table[panel_rev],
				sizeof(struct cmd_map),
				"samsung,acl_map_table_rev", panel_rev,
				ss_parse_panel_table); /* ACL TABLE */

		parse_dt_data(np, &info->elvss_map_table[panel_rev],
				sizeof(struct cmd_map),
				"samsung,elvss_map_table_rev", panel_rev,
				ss_parse_panel_table); /* ELVSS TABLE */

		parse_dt_data(np, &info->aid_map_table[panel_rev],
				sizeof(struct cmd_map),
				"samsung,aid_map_table_rev", panel_rev,
				ss_parse_panel_table); /* AID TABLE */

		parse_dt_data(np, &info->smart_acl_elvss_map_table[panel_rev],
				sizeof(struct cmd_map),
				"samsung,smart_acl_elvss_map_table_rev", panel_rev,
				ss_parse_panel_table); /* TABLE */

		parse_dt_data(np, &info->hmt_reverse_aid_map_table[panel_rev],
				sizeof(struct cmd_map),
				"samsung,hmt_reverse_aid_map_table_rev", panel_rev,
				ss_parse_panel_table); /* TABLE */

		parse_dt_data(np, &info->candela_map_table[HMT][panel_rev],
				sizeof(struct candela_map_table),
				"samsung,hmt_candela_map_table_rev", panel_rev,
				ss_parse_candella_mapping_table);
	}
}

static void ss_panel_pbaboot_config(struct device_node *np,
		struct samsung_display_driver_data *vdd)
{
	// TODO: set appropriate value to drm display members... not pinfo...
#if 0
	struct ss_panel_info *pinfo = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	bool need_to_force_vidoe_mode = false;

	pinfo = &ctrl->panel_data.panel_info;
	vdd = check_valid_ctrl(ctrl);

	if (vdd->support_hall_ic) {
		/* check the lcd id for DISPLAY_1 and DISPLAY_2 */
		if (!ss_panel_attached(DISPLAY_1) && !ss_panel_attached(DISPLAY_2))
			need_to_force_vidoe_mode = true;
	} else {
		/* check the lcd id for DISPLAY_1 */
		if (!ss_panel_attached(DISPLAY_1))
			need_to_force_vidoe_mode = true;
	}

	/* Support PBA boot without lcd */
	if (need_to_force_vidoe_mode &&
			!IS_ERR_OR_NULL(pinfo) &&
			!IS_ERR_OR_NULL(vdd) &&
			(pinfo->mipi.mode == DSI_CMD_MODE)) {
		LCD_ERR("force VIDEO_MODE : %d\n", vdd->ndx);
		pinfo->type = MIPI_VIDEO_PANEL;
		pinfo->mipi.mode = DSI_VIDEO_MODE;
		pinfo->mipi.traffic_mode = DSI_BURST_MODE;
		pinfo->mipi.bllp_power_stop = true;
		pinfo->mipi.te_sel = 0;
		pinfo->mipi.vsync_enable = 0;
		pinfo->mipi.hw_vsync_mode = 0;
		pinfo->mipi.force_clk_lane_hs = true;
		pinfo->mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;

#if 0
		pinfo->cont_splash_enabled = false;
#else
		ss_set_seamless_mode(vdd);
#endif
		pinfo->mipi.lp11_init = false;

		vdd->mdnie.support_mdnie = false;
		vdd->mdnie.lcd_on_notifiy = false;
		vdd->mdnie.support_trans_dimming = false;
		vdd->mdnie.disable_trans_dimming = false;

		if (!IS_ERR_OR_NULL(vdd->panel_func.parsing_otherline_pdata) && ss_panel_attached(DISPLAY_1)) {
			vdd->panel_func.parsing_otherline_pdata = NULL;
			destroy_workqueue(vdd->other_line_panel_support_workq);
		}

		pinfo->esd_check_enabled = false;
		ctrl->on_cmds.link_state = DSI_LP_MODE;
		ctrl->off_cmds.link_state = DSI_LP_MODE;

#if 0
		/* To avoid underrun panic*/
		mdd->logd.xlog_enable = 0;
#else
#endif
		vdd->dtsi_data.samsung_osc_te_fitting = false;
	}
#endif
}


static void ss_dynamic_mipi_clk_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct dyn_mipi_clk *dyn;

	dyn = container_of(work, struct dyn_mipi_clk, change_clk_work);
	vdd = container_of(dyn, struct samsung_display_driver_data, dyn_mipi_clk);

	ss_change_dyn_mipi_clk_timing(vdd);
}

static void ss_panel_set_gamma_flash_data(struct brightness_table *br_tbl, int idx, struct device_node *gamma_node)
{
	struct brightness_table *c_br_tbl;
	struct flash_raw_table *gamma_tbl;
	u32 tmp[3], data;
	int rc;

	gamma_tbl = kzalloc((sizeof(struct flash_raw_table)), GFP_KERNEL);
	if (!gamma_tbl) {
		LCD_ERR("fail to alloc gamma_tbl\n");
		return;
	}

	/* get current bightness_table */
	c_br_tbl = &br_tbl[idx];

	/* copy gamma table from parent's first */
	if (c_br_tbl->parent_idx != -1)
		memcpy(gamma_tbl, br_tbl[c_br_tbl->parent_idx].gamma_tbl, sizeof(struct flash_raw_table));

	c_br_tbl->gamma_tbl = gamma_tbl;

	rc = of_property_read_u32(gamma_node, "samsung,refresh_rate", &data);
	c_br_tbl->refresh_rate = !rc ? data : 0;

	c_br_tbl->is_sot_hs_mode = of_property_read_bool(gamma_node, "samsung,br_tbl_sot_hs_mode");

	/* ddi vertical porches are used for AOR interpolation.
	 * In case of 96/48hz mode, its base AOR came from below base RR mode.
	 * - 96hz: 120hz HS -> AOR_96hz = AOR_120hz * (vtotal_96hz) / (vtotal_120hz)
	 * - 48hz: 60hz normal -> AOR_48hz = AOR_60hz * (vtotal_48hz) / (vtotal_60hz)
	 * If there is no base vertical porches, (ex: ddi_vbp_base) in panel dtsi,
	 * parser function set base value as target value (ex: ddi_vbp_base = ddi_vbp).
	 */
	rc = of_property_read_u32(gamma_node, "samsung,ddi_vfp", &data);
	if (!rc)
		c_br_tbl->ddi_vfp = data;
	rc = of_property_read_u32(gamma_node, "samsung,ddi_vbp", &data);
	if (!rc)
		c_br_tbl->ddi_vbp = data;
	rc = of_property_read_u32(gamma_node, "samsung,ddi_vactive", &data);
	if (!rc)
		c_br_tbl->ddi_vactive = data;

	rc = of_property_read_u32(gamma_node, "samsung,ddi_vfp_base", &data);
	c_br_tbl->ddi_vfp_base = !rc ? data : c_br_tbl->ddi_vfp;
	rc = of_property_read_u32(gamma_node, "samsung,ddi_vbp_base", &data);
	c_br_tbl->ddi_vbp_base = !rc ? data : c_br_tbl->ddi_vbp;
	rc = of_property_read_u32(gamma_node, "samsung,ddi_vactive_base", &data);
	c_br_tbl->ddi_vactive_base = !rc ? data : c_br_tbl->ddi_vactive;

	/* 3byte address cover 15Mbyte */
	rc = of_property_read_u32_array(gamma_node, "samsung,flash_gamma_data_read_addr",
			tmp, SS_FLASH_GAMMA_READ_ADDR_SIZE);
	if (!rc) {
		gamma_tbl->flash_gamma_data_read_addr[0] = tmp[0];
		gamma_tbl->flash_gamma_data_read_addr[1] = tmp[1];
		gamma_tbl->flash_gamma_data_read_addr[2] = tmp[2];
	}

	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_write_check_addr", &data);
	if (!rc)
		gamma_tbl->flash_gamma_write_check_address = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_start_bank", &data);
	if (!rc)
		gamma_tbl->flash_gamma_bank_start = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_end_bank", &data);
	if (!rc)
		gamma_tbl->flash_gamma_bank_end = data;

	LCD_INFO("write_check_address: 0x%x start_bank: 0x%x, end_bank: 0x%x\n",
		gamma_tbl->flash_gamma_write_check_address,
		gamma_tbl->flash_gamma_bank_start, gamma_tbl->flash_gamma_bank_end);

	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_check_sum_start_offset", &data);
	if (!rc)
		gamma_tbl->flash_gamma_check_sum_start_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_check_sum_end_offset", &data);
	if (!rc)
		gamma_tbl->flash_gamma_check_sum_end_offset = data;

	LCD_INFO("check_sum_start_offset : 0x%x check_sum_end_offset: 0x%x\n",
		gamma_tbl->flash_gamma_check_sum_start_offset,
		gamma_tbl->flash_gamma_check_sum_end_offset);

	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_0xc8_start_offset", &data);
	if (!rc)
		gamma_tbl->flash_gamma_0xc8_start_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_0xc8_end_offset", &data);
	if (!rc)
		gamma_tbl->flash_gamma_0xc8_end_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_0xc8_size", &data);
	if (!rc)
		gamma_tbl->flash_gamma_0xc8_size = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_0xc8_check_sum_start_offset", &data);
	if (!rc)
		gamma_tbl->flash_gamma_0xc8_check_sum_start_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_gamma_0xc8_check_sum_end_offset", &data);
	if (!rc)
		gamma_tbl->flash_gamma_0xc8_check_sum_end_offset = data;

	LCD_INFO("flash_gamma 0xC8 start_addr:0x%x end_addr: 0x%x size: 0x%x check_sum_start : 0x%x check_sum_end: 0x%x\n",
		gamma_tbl->flash_gamma_0xc8_start_offset,
		gamma_tbl->flash_gamma_0xc8_end_offset,
		gamma_tbl->flash_gamma_0xc8_size,
		gamma_tbl->flash_gamma_0xc8_check_sum_start_offset,
		gamma_tbl->flash_gamma_0xc8_check_sum_end_offset);

	rc = of_property_read_u32(gamma_node, "samsung,flash_table_hbm_gamma_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_hbm_gamma_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_table_hbm_aor_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_hbm_aor_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_table_hbm_vint_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_hbm_vint_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_table_hbm_elvss_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_hbm_elvss_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_table_hbm_irc_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_hbm_irc_offset = data;

	LCD_INFO("flash_table_hbm_addr gamma:0x%x aor: 0x%x vint:0x%x elvss 0x%x hbm:0x%x\n",
		gamma_tbl->flash_table_hbm_gamma_offset,
		gamma_tbl->flash_table_hbm_aor_offset,
		gamma_tbl->flash_table_hbm_vint_offset,
		gamma_tbl->flash_table_hbm_elvss_offset,
		gamma_tbl->flash_table_hbm_irc_offset);

	rc = of_property_read_u32(gamma_node, "samsung,flash_table_normal_gamma_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_normal_gamma_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_table_normal_aor_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_normal_aor_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_table_normal_vint_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_normal_vint_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_table_normal_elvss_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_normal_elvss_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_table_normal_irc_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_normal_irc_offset = data;

	LCD_INFO("flash_table__normal_addr gamma:0x%x aor: 0x%x vint:0x%x elvss 0x%x hbm:0x%x\n",
		gamma_tbl->flash_table_normal_gamma_offset,
		gamma_tbl->flash_table_normal_aor_offset,
		gamma_tbl->flash_table_normal_vint_offset,
		gamma_tbl->flash_table_normal_elvss_offset,
		gamma_tbl->flash_table_normal_irc_offset);

	rc = of_property_read_u32(gamma_node, "samsung,flash_table_hmd_gamma_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_hmd_gamma_offset = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_table_hmd_aor_offset", &data);
	if (!rc)
		gamma_tbl->flash_table_hmd_aor_offset = data;

	LCD_INFO("flash_table_hmt_addr gamma:0x%x aor: 0x%x\n",
		gamma_tbl->flash_table_hmd_gamma_offset,
		gamma_tbl->flash_table_hmd_aor_offset);

	rc = of_property_read_u32(gamma_node, "samsung,flash_MCD1_R_addr", &data);
	if (!rc)
		gamma_tbl->flash_MCD1_R_address = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_MCD2_R_addr", &data);
	if (!rc)
		gamma_tbl->flash_MCD2_R_address = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_MCD1_L_addr", &data);
	if (!rc)
		gamma_tbl->flash_MCD1_L_address = data;
	rc = of_property_read_u32(gamma_node, "samsung,flash_MCD2_L_addr", &data);
	if (!rc)
		gamma_tbl->flash_MCD2_L_address = data;

	LCD_INFO("flash_gamma MCD1_R:0x%x MCD2_R:0x%x MCD1_L:0x%x MCD2_L:0x%x\n",
		gamma_tbl->flash_MCD1_R_address,
		gamma_tbl->flash_MCD2_R_address,
		gamma_tbl->flash_MCD1_L_address,
		gamma_tbl->flash_MCD2_L_address);

	LCD_INFO("[%d] refresh_rate: %d, HS mode: %d, " \
		"DDI target res: %d+%d+%d / base res: %d+%d+%d, parent_node: %d\n" \
		, c_br_tbl->idx,
		c_br_tbl->refresh_rate,
		c_br_tbl->is_sot_hs_mode,
		c_br_tbl->ddi_vfp, c_br_tbl->ddi_vbp, c_br_tbl->ddi_vactive,
		c_br_tbl->ddi_vfp_base, c_br_tbl->ddi_vbp_base, c_br_tbl->ddi_vactive_base,
		c_br_tbl->parent_idx);

	return;
}

static int ss_panel_parse_dt_br_table(struct device_node *np,
		struct samsung_display_driver_data *vdd)
{
	struct device_node *gamma_root_node = NULL;
	struct device_node *gamma_node = NULL;
	struct device_node *gamma_sub_node = NULL;
	int count = 0;
	int p_idx;
	struct brightness_table *br_tbl;
	int rc;
	const char *data;

	/* check support gamma flash first */
	rc = of_property_read_string(np, "samsung,flash_gamma_type", &data);
	if (rc) {
		vdd->br_info.flash_gamma_support = false;
		LCD_ERR("error reading samsung,flash_gamma_type rc=%d\n", rc);
	} else {
		snprintf(vdd->br_info.flash_gamma_type,
				ARRAY_SIZE((vdd->br_info.flash_gamma_type)), "%s", data);
		LCD_INFO("Flash gamma type : [%s] \n", vdd->br_info.flash_gamma_type);

		if (!strcmp(vdd->br_info.flash_gamma_type, "flash")) {
			vdd->br_info.flash_gamma_support = true;
			LCD_ERR("flash gamma support\n");

			vdd->br_info.support_early_gamma_flash = of_property_read_bool(np, "samsung,support_early_gamma_flash");
			LCD_INFO("support_early_gamma_flash: %d\n", vdd->br_info.support_early_gamma_flash);

			vdd->br_info.flash_br_workqueue = create_singlethread_workqueue("flash_br_workqueue");
			INIT_DELAYED_WORK(&vdd->br_info.flash_br_work, flash_br_work_func);

			rc = of_property_read_string(np, "samsung,flash_read_intf", &data);
			if (rc) {
				LCD_ERR("error reading samsung,flash_read_intf rc=%d\n", rc);
			} else {
				snprintf(vdd->br_info.flash_read_intf,
						ARRAY_SIZE((vdd->br_info.flash_read_intf)), "%s", data);
				LCD_INFO("Flash read intf : [%s] \n", vdd->br_info.flash_read_intf);
			}
		} else if (!strcmp(vdd->br_info.flash_gamma_type, "table")) {
			vdd->br_info.flash_gamma_support = true;
			LCD_ERR("table gamma support\n");
		}
	}

	gamma_root_node = of_get_child_by_name(np, "samsung,br_tables");
	if (!gamma_root_node) {
		LCD_ERR("fail to find br_tables root node\n");
		return -EINVAL;
	}

	for_each_child_of_node(gamma_root_node, gamma_node) {
		count++;
		for_each_child_of_node(gamma_node, gamma_sub_node)
				count++;
	}

	vdd->br_info.br_tbl_count = count;
	LCD_INFO("total gamma nodes count: %d\n", count);

	br_tbl = kzalloc((sizeof(struct brightness_table) * count), GFP_KERNEL);
	if (!br_tbl)
		return -ENOMEM;

	vdd->br_info.br_tbl = br_tbl;

	count = 0;
	for_each_child_of_node(gamma_root_node, gamma_node) {

		/* set current index and parent index
		 * -1 means this node is parent gamma flash node
		 */
		br_tbl[count].idx = count;
		br_tbl[count].parent_idx = -1;

		ss_panel_set_gamma_flash_data(br_tbl, count, gamma_node);

		/* set parent node index */
		p_idx = count;

		vdd->br_info.br_tbl_flash_cnt++;
		count++;

		/* set sub node of gamma flash table */
		for_each_child_of_node(gamma_node, gamma_sub_node) {

			/* set current index and parent index */
			br_tbl[count].idx = count;
			br_tbl[count].parent_idx = p_idx;

			/* update child node specific data */
			ss_panel_set_gamma_flash_data(br_tbl, count, gamma_sub_node);

			count++;
		}
	}

	return 0;
}

int ss_panel_power_ctrl(struct samsung_display_driver_data *vdd, bool enable)
{
	if (enable) {
		/* off -> normal on */
		ss_panel_regulator_short_detection(vdd, PANEL_OFF);
	}

	return 0;
}

int ss_panel_regulator_short_detection(struct samsung_display_driver_data *vdd, enum panel_state state)
{
	int i, voltage;
	int rc = 0;
	bool enable;

	for (i = 0; i < vdd->panel_regulator.vreg_count; i++) {
		if (vdd->panel_regulator.vregs[i].ssd) {

			/* set voltage */
			voltage = (state == PANEL_LPM) ?
				vdd->panel_regulator.vregs[i].from_lpm_v : vdd->panel_regulator.vregs[i].from_off_v;

			/* If voltage is 0, diable SSD */
			enable = voltage ? true : false;

#ifdef CONFIG_SEC_PM
			rc = regulator_set_short_detection(vdd->panel_regulator.vregs[i].vreg, enable, voltage);
#else
			rc = -1;
			LCD_ERR("fail to set SSDE, CONFG_SEC_PM is not defined\n");
#endif
			LCD_ERR("[%s] state : %d, voltage : %d, ssd %s - %s\n",
				vdd->panel_regulator.vregs[i].name, state, voltage, (enable ? "enable" : "disable"),
				(rc < 0 ? "fail" : "success"));
		}
	}

	return 0;
}

static void ss_panel_parse_regulator(struct samsung_display_driver_data *vdd, struct device_node *panel_node)
{
	struct device_node *regulators_np, *np;
	int rc;
	const char* data;
	const __be32 *data_32;
	int count = 0;

	regulators_np = of_get_child_by_name(panel_node, "regulators");
	if (!regulators_np) {
		LCD_ERR("fail to find regulators node\n");
		return;
	}

	for_each_child_of_node(regulators_np, np)
		count++;

	if (count == 0) {
		LCD_ERR("No panel regulators..\n");
		return;
	} else
		vdd->panel_regulator.vreg_count = count;

	LCD_INFO("panel regulator count is %d\n", count);

	vdd->panel_regulator.vregs = kcalloc(vdd->panel_regulator.vreg_count,
		sizeof(*vdd->panel_regulator.vregs), GFP_KERNEL);
	if (!vdd->panel_regulator.vregs) {
		vdd->panel_regulator.vreg_count = 0;
		LCD_ERR("fail to kcalloc for panel regulators..\n");
		return;
	}

	count = 0;
	for_each_child_of_node(regulators_np, np) {
		rc = of_property_read_string(np, "reg,name", &data);
		if (rc)
			LCD_ERR("error reading regulator name. rc=%d\n", rc);
		else {
			snprintf(vdd->panel_regulator.vregs[count].name,
				ARRAY_SIZE((vdd->panel_regulator.vregs[count].name)), "%s", data);

			vdd->panel_regulator.vregs[count].vreg = regulator_get(NULL, vdd->panel_regulator.vregs[count].name);
			if (!vdd->panel_regulator.vregs[count].vreg)
				LCD_ERR("Regulator[%s] get fail\n", vdd->panel_regulator.vregs[count].name);
		}

		rc = of_property_read_string(np, "reg,type", &data);
		if (rc)
			LCD_ERR("error reading regulator type. rc=%d\n", rc);
		else {
			if (!strcmp(data, "ssd"))
				vdd->panel_regulator.vregs[count].ssd = true;
		}

		data_32 = of_get_property(np, "reg,from-off", NULL);
		if (data_32)
			vdd->panel_regulator.vregs[count].from_off_v = (int)(be32_to_cpup(data_32));
		else
			LCD_ERR("error reading from-off voltage\n");

		data_32 = of_get_property(np, "reg,from-lpm", NULL);
		if (data_32)
			vdd->panel_regulator.vregs[count].from_lpm_v = (int)(be32_to_cpup(data_32));
		else
			LCD_ERR("error reading from-lpm voltage\n");

		LCD_INFO("PANEL REGULATOR[%d] Name [%s], ssd = %d, from_off_v = %d, from_lpm_v = %d\n", count,
			vdd->panel_regulator.vregs[count].name, vdd->panel_regulator.vregs[count].ssd,
			vdd->panel_regulator.vregs[count].from_off_v,
			vdd->panel_regulator.vregs[count].from_lpm_v);

		count++;
	}

	return;
}

static void ss_panel_parse_dt(struct samsung_display_driver_data *vdd)
{
	int rc, i;
	u32 tmp[2];
	int len;
	char backlight_tft_gpio[] = "samsung,panel-backlight-tft-gpio1";
	const char *data;
	const __be32 *data_32;
	struct device_node *np;

	np = ss_get_panel_of(vdd);

	if (!np)
		return;

	/* Set LP11 init flag */
	vdd->dtsi_data.samsung_lp11_init = of_property_read_bool(np, "samsung,dsi-lp11-init");
	LCD_ERR("LP11 init %s\n",
		vdd->dtsi_data.samsung_lp11_init ? "enabled" : "disabled");

	rc = of_property_read_u32(np, "samsung,mdss-power-on-reset-delay-us", tmp);
	vdd->dtsi_data.samsung_power_on_reset_delay = (!rc ? tmp[0] : 0);

	rc = of_property_read_u32(np, "samsung,mdss-dsi-off-reset-delay-us", tmp);
	vdd->dtsi_data.samsung_dsi_off_reset_delay = (!rc ? tmp[0] : 0);

	/* Set esc clk 128M */
	vdd->dtsi_data.samsung_esc_clk_128M = of_property_read_bool(np, "samsung,esc-clk-128M");
	LCD_ERR("ESC CLK 128M %s\n",
		vdd->dtsi_data.samsung_esc_clk_128M ? "enabled" : "disabled");

	vdd->dtsi_data.panel_lpm_enable = of_property_read_bool(np, "samsung,panel-lpm-enable");
	LCD_ERR("alpm enable %s\n", vdd->dtsi_data.panel_lpm_enable ? "enabled" : "disabled");

	vdd->skip_read_on_pre = of_property_read_bool(np, "samsung,skip_read_on_pre");
	LCD_ERR("Skip read on pre %s\n", vdd->skip_read_on_pre ? "enabled" : "disabled");

	/* Set HALL IC */
	vdd->support_hall_ic  = of_property_read_bool(np, "samsung,mdss_dsi_hall_ic_enable");
	LCD_ERR("hall_ic %s\n", vdd->support_hall_ic ? "enabled" : "disabled");

	rc = of_property_read_u32(np, "samsung,mdss_dsi_lcd_flip_delay_ms", tmp);
		vdd->lcd_flip_delay_ms = (!rc ? tmp[0] : 0);
		LCD_ERR("lcd_flip_delay_ms %d\n", vdd->lcd_flip_delay_ms);

	/*Set OSC TE fitting flag */
	vdd->dtsi_data.samsung_osc_te_fitting =
		of_property_read_bool(np, "samsung,osc-te-fitting-enable");

	if (vdd->dtsi_data.samsung_osc_te_fitting) {
		rc = of_property_read_u32_array(np, "samsung,osc-te-fitting-cmd-index", tmp, 2);
		if (!rc) {
			vdd->dtsi_data.samsung_osc_te_fitting_cmd_index[0] =
				tmp[0];
			vdd->dtsi_data.samsung_osc_te_fitting_cmd_index[1] =
				tmp[1];
		}

		rc = of_property_read_u32(np, "samsung,osc-te-fitting-sampling-rate", tmp);

		vdd->te_fitting_info.sampling_rate = !rc ? tmp[0] : 2;

	}

	LCD_INFO("OSC TE fitting %s\n",
		vdd->dtsi_data.samsung_osc_te_fitting ? "enabled" : "disabled");

	/* Set HMT flag */
	vdd->dtsi_data.hmt_enabled = of_property_read_bool(np, "samsung,hmt_enabled");
	if (vdd->dtsi_data.hmt_enabled)
		vdd->dtsi_data.hmt_enabled = true;

	LCD_INFO("hmt %s\n",
		vdd->dtsi_data.hmt_enabled ? "enabled" : "disabled");

	/* TCON Clk On Support */
	vdd->dtsi_data.samsung_tcon_clk_on_support =
		of_property_read_bool(np, "samsung,tcon-clk-on-support");
	LCD_INFO("tcon clk on support: %s\n",
			vdd->dtsi_data.samsung_tcon_clk_on_support ?
			"enabled" : "disabled");

	vdd->dtsi_data.samsung_tcon_rdy_gpio =
		of_get_named_gpio(np, "samsung,tcon-rdy-gpio", 0);

	if (gpio_is_valid(vdd->dtsi_data.samsung_tcon_rdy_gpio))
		gpio_request(vdd->dtsi_data.samsung_tcon_rdy_gpio, "tcon_rdy");
	else
		LCD_ERR("TCON RDY GPIO(%d) is not valid!!\n", vdd->dtsi_data.samsung_tcon_rdy_gpio);

	LCD_INFO("tcon rdy gpio: %d\n", vdd->dtsi_data.samsung_tcon_rdy_gpio);

	data_32 = of_get_property(np, "samsung,delay-after-tcon-ready", NULL);
	vdd->dtsi_data.samsung_delay_after_tcon_rdy = (data_32 ? (int)(be32_to_cpup(data_32)) : 0);

	LCD_INFO("Delay after Tcon rdy : %d\n", vdd->dtsi_data.samsung_delay_after_tcon_rdy);

	/* Set unicast flags for whole mipi cmds for dual DSI panel
	 * If display->ctrl_count is 2, it broadcasts.
	 * To prevent to send mipi cmd thru mipi dsi1, set unicast flag
	 */
	vdd->dtsi_data.samsung_cmds_unicast =
		of_property_read_bool(np, "samsung,cmds-unicast");
	LCD_INFO("mipi cmds unicast flag: %d\n",
			vdd->dtsi_data.samsung_cmds_unicast);

	/* anapass DDI power sequence: VDD on -> LP11 -> reset -> wait TCON RDY high -> HS clock.
	 * To keep above unique sequence, use anapass_power_seq flag..
	 */
	vdd->dtsi_data.samsung_anapass_power_seq =
		of_property_read_bool(np, "samsung,anapass-power-seq");
	LCD_INFO("anapass ddi power seq. flag: %d\n",
			vdd->dtsi_data.samsung_anapass_power_seq );

	/* Set TFT flag */
	vdd->mdnie.tuning_enable_tft = of_property_read_bool(np,
				"samsung,mdnie-tuning-enable-tft");
	vdd->dtsi_data.tft_common_support  = of_property_read_bool(np,
		"samsung,tft-common-support");

	LCD_INFO("tft_common_support %s\n",
	vdd->dtsi_data.tft_common_support ? "enabled" : "disabled");

	vdd->dtsi_data.tft_module_name = of_get_property(np,
		"samsung,tft-module-name", NULL);  /* for tft tablet */

	vdd->dtsi_data.panel_vendor = of_get_property(np,
		"samsung,panel-vendor", NULL);

	vdd->dtsi_data.disp_model = of_get_property(np,
		"samsung,disp-model", NULL);

	vdd->dtsi_data.backlight_gpio_config = of_property_read_bool(np,
		"samsung,backlight-gpio-config");

	LCD_INFO("backlight_gpio_config %s\n",
	vdd->dtsi_data.backlight_gpio_config ? "enabled" : "disabled");

	/* Factory Panel Swap*/
	vdd->dtsi_data.samsung_support_factory_panel_swap = of_property_read_bool(np,
		"samsung,support_factory_panel_swap");

	vdd->support_window_color = of_property_read_bool(np, "samsung,support_window_color");
	LCD_INFO("ndx(%d) support_window_color %s\n", vdd->ndx,
		vdd->support_window_color ? "support" : "not support");

	/* Set tft backlight gpio */
	for (i = 0; i < MAX_BACKLIGHT_TFT_GPIO; i++) {
		backlight_tft_gpio[strlen(backlight_tft_gpio) - 1] = '1' + i;
		vdd->dtsi_data.backlight_tft_gpio[i] =
				 of_get_named_gpio(np,
						backlight_tft_gpio, 0);
		if (!gpio_is_valid(vdd->dtsi_data.backlight_tft_gpio[i]))
			LCD_ERR("%d, backlight_tft_gpio gpio%d not specified\n",
							__LINE__, i+1);
		else
			LCD_ERR("tft gpio num : %d\n", vdd->dtsi_data.backlight_tft_gpio[i]);
	}

	/* Set Mdnie lite HBM_CE_TEXT_MDNIE mode used */
	vdd->dtsi_data.hbm_ce_text_mode_support = of_property_read_bool(np, "samsung,hbm_ce_text_mode_support");

	/* Set Backlight IC discharge time */
	rc = of_property_read_u32(np, "samsung,blic-discharging-delay-us", tmp);
	vdd->dtsi_data.blic_discharging_delay_tft = (!rc ? tmp[0] : 6);

	/* Set cabc delay time */
	rc = of_property_read_u32(np, "samsung,cabc-delay-us", tmp);
	vdd->dtsi_data.cabc_delay = (!rc ? tmp[0] : 6);

	/* IRC */
	vdd->br_info.common_br.support_irc = of_property_read_bool(np, "samsung,support_irc");

	/* Gram Checksum Test */
	vdd->gct.is_support = of_property_read_bool(np, "samsung,support_gct");
	LCD_DEBUG("vdd->gct.is_support = %d\n", vdd->gct.is_support);

	/* POC Driver */
	vdd->poc_driver.is_support = of_property_read_bool(np, "samsung,support_poc_driver");
	LCD_INFO("[POC] is_support = %d\n", vdd->poc_driver.is_support);

	if (vdd->poc_driver.is_support) {
		rc = of_property_read_u32(np, "samsung,poc_image_size", tmp);
		vdd->poc_driver.image_size = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,poc_start_addr", tmp);
		vdd->poc_driver.start_addr = (!rc ? tmp[0] : 0);

		LCD_INFO("[POC] start_addr (%d) image_size (%d)\n",
			vdd->poc_driver.start_addr, vdd->poc_driver.image_size);

		/* ERASE */
		rc = of_property_read_u32(np, "samsung,poc_erase_delay_us", tmp);
		vdd->poc_driver.erase_delay_us = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32_array(np, "samsung,poc_erase_sector_addr_idx",
				vdd->poc_driver.erase_sector_addr_idx, 3);
		if (rc) {
			vdd->poc_driver.erase_sector_addr_idx[0] = -1;
			LCD_INFO("fail to get poc_erase_sector_addr_idx\n");
		}

		LCD_INFO("[POC][ERASE] delay_us(%d) addr idx (%d %d %d)\n",
			vdd->poc_driver.erase_delay_us,
			vdd->poc_driver.erase_sector_addr_idx[0],
			vdd->poc_driver.erase_sector_addr_idx[1],
			vdd->poc_driver.erase_sector_addr_idx[2]);

		/* WRITE */
		rc = of_property_read_u32(np, "samsung,poc_write_delay_us", tmp);
		vdd->poc_driver.write_delay_us = (!rc ? tmp[0] : 0);
		rc = of_property_read_u32(np, "samsung,poc_write_data_size", tmp);
		vdd->poc_driver.write_data_size = (!rc ? tmp[0] : 0);
		rc = of_property_read_u32(np, "samsung,poc_write_loop_cnt", tmp);
		vdd->poc_driver.write_loop_cnt = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32_array(np, "samsung,poc_write_addr_idx",
				vdd->poc_driver.write_addr_idx, 3);
		if (rc) {
			vdd->poc_driver.write_addr_idx[0] = -1;
			LCD_INFO("fail to get poc_write_addr_idx\n");
		}

		LCD_INFO("[POC][WRITE] delay_us(%d) data_size(%d) loo_cnt(%d) addr idx (%d %d %d)\n",
			vdd->poc_driver.write_delay_us,
			vdd->poc_driver.write_data_size,
			vdd->poc_driver.write_loop_cnt,
			vdd->poc_driver.write_addr_idx[0],
			vdd->poc_driver.write_addr_idx[1],
			vdd->poc_driver.write_addr_idx[2]);

		/* READ */
		rc = of_property_read_u32(np, "samsung,poc_read_delay_us", tmp);
		vdd->poc_driver.read_delay_us = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32_array(np, "samsung,poc_read_addr_idx",
				vdd->poc_driver.read_addr_idx, 3);
		if (rc) {
			vdd->poc_driver.read_addr_idx[0] = -1;
			LCD_INFO("fail to get poc_read_addr_idx\n");
		} else
			LCD_INFO("read addr idx (%d %d %d)\n",
				vdd->poc_driver.read_addr_idx[0],
				vdd->poc_driver.read_addr_idx[1],
				vdd->poc_driver.read_addr_idx[2]);

		LCD_INFO("[POC][READ] delay_us(%d) addr idx (%d %d %d)\n",
			vdd->poc_driver.read_delay_us,
			vdd->poc_driver.read_addr_idx[0],
			vdd->poc_driver.read_addr_idx[1],
			vdd->poc_driver.read_addr_idx[2]);

		/* parse spi cmds */
		ss_panel_parse_spi_cmd(np, vdd);
	}

	/* SWIRE Rework, Firmware update */
	vdd->fw_up.is_support = of_property_read_bool(np, "samsung,support_firmware_update");
	LCD_INFO("[FW_UP]is_support = %d\n", vdd->fw_up.is_support);

	if (vdd->fw_up.is_support) {
		rc = of_property_read_u32(np, "samsung,fw_image_size", tmp);
		vdd->fw_up.image_size = (!rc ? tmp[0] : 0);
		rc = of_property_read_u32(np, "samsung,fw_sector_size", tmp);
		vdd->fw_up.sector_size = (!rc ? tmp[0] : 0);
		rc = of_property_read_u32(np, "samsung,fw_start_addr", tmp);
		vdd->fw_up.start_addr = (!rc ? tmp[0] : 0);

		LCD_INFO("[FW_UP]ADDRESS/SIZE (0x%x/%d)\n",
			vdd->fw_up.start_addr, vdd->fw_up.image_size);

		/* ERASE */
		rc = of_property_read_u32(np, "samsung,firmware_update_erase_delay_us", tmp);
		vdd->fw_up.erase_delay_us = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32_array(np, "samsung,fw_erase_addr_idx", vdd->fw_up.erase_addr_idx, 3);
		if (rc) {
			vdd->fw_up.erase_addr_idx[0] = -1;
			LCD_INFO("fail to get fw_erase_addr_idx\n");
		}

		rc = of_property_read_u32_array(np, "samsung,fw_erase_size_idx", vdd->fw_up.erase_size_idx, 3);
		if (rc) {
			vdd->fw_up.erase_size_idx[0] = -1;
			LCD_INFO("fail to get fw_erase_size_idx\n");
		}
		LCD_INFO("[FW_UP][ERASE] delay_us(%d) addr_idx(%d %d %d), size_idx(%d %d %d)\n",
			vdd->fw_up.erase_delay_us,
			vdd->fw_up.erase_addr_idx[0], vdd->fw_up.erase_addr_idx[1], vdd->fw_up.erase_addr_idx[2],
			vdd->fw_up.erase_size_idx[0], vdd->fw_up.erase_size_idx[1], vdd->fw_up.erase_size_idx[2]);


		/* WRITE */
		rc = of_property_read_u32(np, "samsung,firmware_update_write_delay_us", tmp);
		vdd->fw_up.write_delay_us = (!rc ? tmp[0] : 0);
		rc = of_property_read_u32(np, "samsung,fw_write_data_size", tmp);
		vdd->fw_up.write_data_size = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32_array(np, "samsung,fw_write_addr_idx", vdd->fw_up.write_addr_idx, 3);
		if (rc) {
			vdd->fw_up.write_addr_idx[0] = -1;
			LCD_INFO("fail to get fw_write_addr_idx\n");
		}

		rc = of_property_read_u32_array(np, "samsung,fw_write_size_idx", vdd->fw_up.write_size_idx, 3);
		if (rc) {
			vdd->fw_up.write_size_idx[0] = -1;
			LCD_INFO("fail to get fw_write_size_idx\n");
		}
		LCD_INFO("[FW_UP][WRITE] delay_us(%d) data_size(%d) addr_idx(%d %d %d), size_idx(%d %d %d)\n",
			vdd->fw_up.write_delay_us,	vdd->fw_up.write_data_size,
			vdd->fw_up.write_addr_idx[0], vdd->fw_up.write_addr_idx[1], vdd->fw_up.write_addr_idx[2],
			vdd->fw_up.write_size_idx[0], vdd->fw_up.write_size_idx[1], vdd->fw_up.write_size_idx[2]);


		/* READ */
		rc = of_property_read_u32(np, "samsung,firmware_update_read_delay_us", tmp);
		vdd->fw_up.read_delay_us = (!rc ? tmp[0] : 0);
		rc = of_property_read_u32(np, "samsung,firmware_update_status_read_value", tmp);
		vdd->fw_up.read_status_value = (!rc ? tmp[0] : 0);
		rc = of_property_read_u32(np, "samsung,firmware_update_done_check_read_value", tmp);
		vdd->fw_up.read_done_check = (!rc ? tmp[0] : 0);

		LCD_INFO("[FW_UP] E/W/R delay_us(%d/%d/%d) status_read_value (0x%x) rework_done_check_value(0x%x)\n",
			vdd->fw_up.erase_delay_us, vdd->fw_up.write_delay_us, vdd->fw_up.read_delay_us,
			vdd->fw_up.read_status_value, vdd->fw_up.read_done_check);
	}

	/* PAC */
	vdd->br_info.common_br.pac = of_property_read_bool(np, "samsung,support_pac");
	LCD_INFO("vdd->br_info.common_br.pac = %d\n", vdd->br_info.common_br.pac);

	/* Gamma Mode 2 */
	vdd->br_info.common_br.gamma_mode2_support = of_property_read_bool(np, "samsung,support_gamma_mode2");
	LCD_INFO("vdd->br.gamma_mode2_support = %d\n", vdd->br_info.common_br.gamma_mode2_support);

	if (vdd->br_info.common_br.gamma_mode2_support) {
		rc = of_property_read_u32(np, "samsung,flash_vbias_start_addr", tmp);
		vdd->br_info.gm2_table.ddi_flash_start_addr = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_tot_len", tmp);
		vdd->br_info.gm2_table.ddi_flash_raw_len = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_one_mode_len", tmp);
		vdd->br_info.gm2_table.len_one_vbias_mode = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_mtp_one_mode_len", tmp);
		vdd->br_info.gm2_table.len_mtp_one_vbias_mode = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_ns_temp_warm_offset", tmp);
		vdd->br_info.gm2_table.off_ns_temp_warm = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_ns_temp_cool_offset", tmp);
		vdd->br_info.gm2_table.off_ns_temp_cool = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_ns_temp_cold_offset", tmp);
		vdd->br_info.gm2_table.off_ns_temp_cold = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_hs_temp_warm_offset", tmp);
		vdd->br_info.gm2_table.off_hs_temp_warm = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_hs_temp_cool_offset", tmp);
		vdd->br_info.gm2_table.off_hs_temp_cool = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_hs_temp_cold_offset", tmp);
		vdd->br_info.gm2_table.off_hs_temp_cold = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_checksum_offset", tmp);
		vdd->br_info.gm2_table.off_gm2_flash_checksum = (!rc ? tmp[0] : 0);

		LCD_INFO("flash: start: %x, tot len: %d, one mode len: %d, mode off={%x %x %x %x %x %x, checksum: %x}\n",
				vdd->br_info.gm2_table.ddi_flash_start_addr,
				vdd->br_info.gm2_table.ddi_flash_raw_len,
				vdd->br_info.gm2_table.len_one_vbias_mode,
				vdd->br_info.gm2_table.off_ns_temp_warm,
				vdd->br_info.gm2_table.off_ns_temp_cool,
				vdd->br_info.gm2_table.off_ns_temp_cold,
				vdd->br_info.gm2_table.off_hs_temp_warm,
				vdd->br_info.gm2_table.off_hs_temp_cool,
				vdd->br_info.gm2_table.off_hs_temp_cold,
				vdd->br_info.gm2_table.off_gm2_flash_checksum);
	}

	vdd->br_info.common_br.multi_to_one_support = of_property_read_bool(np, "samsung,multi_to_one_support");
	LCD_INFO("vdd->br.multi_to_one_support = %d\n", vdd->br_info.common_br.multi_to_one_support);

	/* Global Para */
	vdd->gpara = of_property_read_bool(np, "samsung,support_gpara");
	LCD_INFO("vdd->support_gpara = %d\n", vdd->gpara);

	/* Self Display */
	vdd->self_disp.is_support = of_property_read_bool(np, "samsung,support_self_display");
	LCD_INFO("vdd->self_disp.is_support = %d\n", vdd->self_disp.is_support);

	if (vdd->self_disp.is_support) {
		/* Self Mask Check CRC data */
		data = of_get_property(np, "samsung,mask_crc_pass_data", &len);
		if (!data)
			LCD_ERR("fail to get samsung,mask_crc_pass_data .. \n");
		else {
			vdd->self_disp.mask_crc_pass_data = kzalloc(len, GFP_KERNEL);
			vdd->self_disp.mask_crc_read_data = kzalloc(len, GFP_KERNEL);
			vdd->self_disp.mask_crc_size = len;
			if (!vdd->self_disp.mask_crc_pass_data || !vdd->self_disp.mask_crc_read_data)
				LCD_ERR("fail to alloc for mask_crc_data \n");
			else {
				for (i = 0; i < len; i++) {
					vdd->self_disp.mask_crc_pass_data[i] = data[i];
					LCD_ERR("self mask crc_data[%d] = %02x\n", i, vdd->self_disp.mask_crc_pass_data[i]);
				}
			}
		}
	}

	/* mafpc */
	vdd->mafpc.is_support = of_property_read_bool(np, "samsung,support_mafpc");
	LCD_INFO("vdd->mafpc.is_support = %d\n", vdd->mafpc.is_support);

	if (vdd->mafpc.is_support) {
		/* mAFPC Check CRC data */
		data = of_get_property(np, "samsung,mafpc_crc_pass_data", &len);
		if (!data)
			LCD_ERR("fail to get samsung,mask_crc_pass_data .. \n");
		else {
			vdd->mafpc.crc_pass_data = kzalloc(len, GFP_KERNEL);
			vdd->mafpc.crc_read_data = kzalloc(len, GFP_KERNEL);
			vdd->mafpc.crc_size = len;
			if (!vdd->mafpc.crc_pass_data || !vdd->mafpc.crc_read_data)
				LCD_ERR("fail to alloc for mafpc crc data \n");
			else {
				for (i = 0; i < len; i++) {
					vdd->mafpc.crc_pass_data[i] = data[i];
					LCD_ERR("mafpc crc_data[%d] = %02x\n", i, vdd->mafpc.crc_pass_data[i]);
				}
			}
		}
	}

	/* DDI SPI */
	vdd->samsung_support_ddi_spi = of_property_read_bool(np, "samsung,support_ddi_spi");
	if (vdd->samsung_support_ddi_spi) {
		rc = of_property_read_u32(np, "samsung,ddi_spi_cs_high_gpio_for_gpara", tmp);
		vdd->ddi_spi_cs_high_gpio_for_gpara = (!rc ? tmp[0] : -1);

		LCD_INFO("vdd->ddi_spi_cs_high_gpio_for_gpara = %d\n", vdd->ddi_spi_cs_high_gpio_for_gpara);
	}

	/* Set elvss_interpolation_temperature */
	data_32 = of_get_property(np, "samsung,elvss_interpolation_temperature", NULL);

	if (data_32)
		vdd->br_info.common_br.elvss_interpolation_temperature = (int)(be32_to_cpup(data_32));
	else
		vdd->br_info.common_br.elvss_interpolation_temperature = ELVSS_INTERPOLATION_TEMPERATURE;

	/* Set lux value for mdnie HBM */
	data_32 = of_get_property(np, "samsung,enter_hbm_ce_lux", NULL);
	if (data_32)
		vdd->mdnie.enter_hbm_ce_lux = (int)(be32_to_cpup(data_32));
	else
		vdd->mdnie.enter_hbm_ce_lux = ENTER_HBM_CE_LUX;

	/* SAMSUNG_FINGERPRINT */
	vdd->support_optical_fingerprint = of_property_read_bool(np, "samsung,support-optical-fingerprint");
	LCD_ERR("support_optical_fingerprint %s\n",
		vdd->support_optical_fingerprint ? "enabled" : "disabled");

	/* Power Control for LPM */
	vdd->panel_lpm.lpm_pwr.support_lpm_pwr_ctrl = of_property_read_bool(np, "samsung,lpm-power-control");
	LCD_INFO("lpm_power_control %s\n", vdd->panel_lpm.lpm_pwr.support_lpm_pwr_ctrl ? "enabled" : "disabled");

	if (vdd->panel_lpm.lpm_pwr.support_lpm_pwr_ctrl) {
		rc = of_property_read_string(np, "samsung,lpm-power-control-supply-name", &data);
		if (rc)
			LCD_ERR("error reading lpm-power name. rc=%d\n", rc);
		else
			snprintf(vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_supply_name,
				ARRAY_SIZE((vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_supply_name)), "%s", data);

		data_32 = of_get_property(np, "samsung,lpm-power-control-supply-min-voltage", NULL);
		if (data_32)
			vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_supply_min_v = (int)(be32_to_cpup(data_32));
		else
			LCD_ERR("error reading lpm-power min_voltage\n");

		data_32 = of_get_property(np, "samsung,lpm-power-control-supply-max-voltage", NULL);
		if (data_32)
			vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_supply_max_v = (int)(be32_to_cpup(data_32));
		else
			LCD_ERR("error reading lpm-power max_voltage\n");

		LCD_INFO("lpm_power_control Supply Name=%s, Min=%d, Max=%d\n",
			vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_supply_name, vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_supply_min_v,
			vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_supply_max_v);

		rc = of_property_read_string(np, "samsung,lpm-power-control-elvss-name", &data);
		if (rc)
			LCD_ERR("error reading lpm-power-elvss name. rc=%d\n", rc);
		else
			snprintf(vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_elvss_name,
				ARRAY_SIZE((vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_elvss_name)), "%s", data);

		data_32 = of_get_property(np, "samsung,lpm-power-control-elvss-lpm-voltage", NULL);
		if (data_32)
			vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_elvss_lpm_v = (int)(be32_to_cpup(data_32));
		else
			LCD_ERR("error reading lpm-power-elvss lpm voltage\n");

		data_32 = of_get_property(np, "samsung,lpm-power-control-elvss-normal-voltage", NULL);
		if (data_32)
			vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_elvss_normal_v = (int)(be32_to_cpup(data_32));
		else
			LCD_ERR("error reading lpm-power-elvss normal voltage\n");

		LCD_INFO("lpm_power_control ELVSS Name=%s, lpm=%d, normal=%d\n",
			vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_elvss_name, vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_elvss_lpm_v,
			vdd->panel_lpm.lpm_pwr.lpm_pwr_ctrl_elvss_normal_v);
	}

	ss_panel_parse_regulator(vdd, np);

	/* To reduce DISPLAY ON time */
	vdd->dtsi_data.samsung_reduce_display_on_time = of_property_read_bool(np,"samsung,reduce_display_on_time");
	vdd->dtsi_data.samsung_dsi_force_clock_lane_hs = of_property_read_bool(np,"samsung,dsi_force_clock_lane_hs");
	rc = of_property_read_u32(np, "samsung,wait_after_reset_delay", tmp);
	vdd->dtsi_data.samsung_wait_after_reset_delay = (!rc ? tmp[0] : 0);
	rc = of_property_read_u32(np, "samsung,wait_after_sleep_out_delay", tmp);
	vdd->dtsi_data.samsung_wait_after_sleep_out_delay = (!rc ? tmp[0] : 0);

	if (vdd->dtsi_data.samsung_reduce_display_on_time) {
		/*
			Please Check interrupt gpio is general purpose gpio
			1. pmic gpio or gpio-expender is not permitted.
			2. gpio should have unique interrupt number.
		*/

		if(gpio_is_valid(of_get_named_gpio(np, "samsung,home_key_irq_gpio", 0))) {
				vdd->dtsi_data.samsung_home_key_irq_num =
					gpio_to_irq(of_get_named_gpio(np, "samsung,home_key_irq_gpio", 0));
				LCD_ERR("%s gpio : %d (irq : %d)\n",
					np->name, of_get_named_gpio(np, "samsung,home_key_irq_gpio", 0),
					vdd->dtsi_data.samsung_home_key_irq_num);
		}

		if(gpio_is_valid(of_get_named_gpio(np, "samsung,fingerprint_irq_gpio", 0))) {
			vdd->dtsi_data.samsung_finger_print_irq_num =
				gpio_to_irq(of_get_named_gpio(np, "samsung,fingerprint_irq_gpio", 0));
			LCD_ERR("%s gpio : %d (irq : %d)\n",
				np->name, of_get_named_gpio(np, "samsung,fingerprint_irq_gpio", 0),
				vdd->dtsi_data.samsung_finger_print_irq_num);
		}
	}

	/* Dynamic MIPI Clock */
	vdd->dyn_mipi_clk.is_support = of_property_read_bool(np, "samsung,support_dynamic_mipi_clk");
	LCD_INFO("vdd->dyn_mipi_clk.is_support = %d\n", vdd->dyn_mipi_clk.is_support);

	if (vdd->dyn_mipi_clk.is_support) {
		ss_parse_dyn_mipi_clk_sel_table(np, &vdd->dyn_mipi_clk.clk_sel_table,
					"samsung,dynamic_mipi_clk_sel_table");
		ss_parse_dyn_mipi_clk_timing_table(np, &vdd->dyn_mipi_clk.clk_timing_table,
					"samsung,dynamic_mipi_clk_timing_table");

		vdd->dyn_mipi_clk.change_clk_wq = create_singlethread_workqueue("dyna_mipi_clk_wq");
		if (!vdd->dyn_mipi_clk.change_clk_wq) {
			LCD_ERR("failed to create read copr workqueue..\n");
			return;
		}
		INIT_WORK(&vdd->dyn_mipi_clk.change_clk_work, ss_dynamic_mipi_clk_work);

		vdd->dyn_mipi_clk.notifier.priority = 0;
		vdd->dyn_mipi_clk.notifier.notifier_call = ss_rf_info_notify_callback;
		register_dev_ril_bridge_event_notifier(&vdd->dyn_mipi_clk.notifier);
	}

	ss_panel_parse_dt_bright_tables(np, vdd);
	ss_dsi_panel_parse_cmd_sets(vdd->dtsi_data.cmd_sets, GET_DSI_PANEL(vdd));

	if (vdd->support_hall_ic) {
		vdd->hall_ic_notifier_display.priority = 0; /* Tsp is 1, Touch key is 2 */
		vdd->hall_ic_notifier_display.notifier_call = samsung_display_hall_ic_status;
#if defined(CONFIG_FOLDER_HALL)
		hall_ic_register_notify(&vdd->hall_ic_notifier_display);
#endif
	}

	/* pll ssc */
	vdd_pll_ssc_disabled = of_property_read_bool(np, "samsung,pll_ssc_disabled");
	LCD_INFO("vdd->pll_ssc_disabled = %d\n", vdd_pll_ssc_disabled);

	// LCD SELECT
	vdd->select_panel_gpio = of_get_named_gpio(np,
			"samsung,mdss_dsi_lcd_sel_gpio", 0);
	if (gpio_is_valid(vdd->select_panel_gpio))
		gpio_request(vdd->select_panel_gpio, "lcd_sel_gpio");

	vdd->select_panel_use_expander_gpio = of_property_read_bool(np, "samsung,mdss_dsi_lcd_sel_use_expander_gpio");

	/* Panel LPM */
	rc = of_property_read_u32(np, "samsung,lpm-init-delay-ms", tmp);
	vdd->dtsi_data.samsung_lpm_init_delay = (!rc ? tmp[0] : 0);

	rc = of_property_read_u32(np, "samsung,delayed-display-on", tmp);
	vdd->dtsi_data.samsung_delayed_display_on = (!rc ? tmp[0] : 0);

	/* FG_ERR */
	if (vdd->is_factory_mode) {
		vdd->fg_err_gpio = of_get_named_gpio(np, "samsung,fg-err_gpio", 0);
		if (gpio_is_valid(vdd->fg_err_gpio))
			LCD_INFO("FG_ERR gpio: %d\n", vdd->fg_err_gpio);
	} else {
		vdd->fg_err_gpio = -1; /* default 0 is valid gpio... set invalid gpio.. */
	}

	vdd->support_lp_rx_err_recovery = of_property_read_bool(np, "samsung,support_lp_rx_err_recovery");
	LCD_INFO("support_lp_rx_err_recovery : %d\n", vdd->support_lp_rx_err_recovery);

	ss_panel_parse_dt_esd(np, vdd);
	ss_panel_parse_dt_ub_con(np, vdd);
	ss_panel_pbaboot_config(np, vdd);

	/* AOR & IRC INTERPOLATION (FLASH or NO FLASH SUPPORT)*/
	data_32 = of_get_property(np, "samsung,hbm_brightness", &len);
			vdd->br_info.hbm_brightness_step = (data_32 ? len/sizeof(len) : 0);
	data_32 = of_get_property(np, "samsung,normal_brightness", &len);
			vdd->br_info.normal_brightness_step = (data_32 ? len/sizeof(len) : 0);
	data_32 = of_get_property(np, "samsung,hmd_brightness", &len);
			vdd->br_info.hmd_brightness_step = (data_32 ? len/sizeof(len) : 0);
	LCD_INFO("gamma_step hbm: %d normal: %d hmt: %d\n",
		vdd->br_info.hbm_brightness_step,
		vdd->br_info.normal_brightness_step,
		vdd->br_info.hmd_brightness_step);

	rc = of_property_read_u32(np, "samsung,gamma_size", tmp);
			vdd->br_info.gamma_size = (!rc ? tmp[0] : 34);
	rc = of_property_read_u32(np, "samsung,aor_size", tmp);
			vdd->br_info.aor_size = (!rc ? tmp[0] : 2);
	rc = of_property_read_u32(np, "samsung,vint_size", tmp);
			vdd->br_info.vint_size = (!rc ? tmp[0] : 1);
	rc = of_property_read_u32(np, "samsung,elvss_size", tmp);
			vdd->br_info.elvss_size = (!rc ? tmp[0] : 3);
	rc = of_property_read_u32(np, "samsung,irc_size", tmp);
			vdd->br_info.irc_size = (!rc ? tmp[0] : 17);
	rc = of_property_read_u32(np, "samsung,dbv_size", tmp);
			vdd->br_info.dbv_size = (!rc ? tmp[0] : 17);

	LCD_INFO("gamma_size gamma:%d aor:%d vint:%d elvss:%d irc:%d dbv:%d\n",
		vdd->br_info.gamma_size,
		vdd->br_info.aor_size,
		vdd->br_info.vint_size,
		vdd->br_info.elvss_size,
		vdd->br_info.irc_size,
		vdd->br_info.dbv_size);

	/* parse br table */
	ss_panel_parse_dt_br_table(np, vdd);

	/* ccd success,fail value */
	rc = of_property_read_u32(np, "samsung,ccd_pass_val", tmp);
	vdd->ccd_pass_val = (!rc ? tmp[0] : 0);
	rc = of_property_read_u32(np, "samsung,ccd_fail_val", tmp);
	vdd->ccd_fail_val = (!rc ? tmp[0] : 0);
	LCD_INFO("CCD fail value [%02x] \n", vdd->ccd_fail_val);

	vdd->rsc_4_frame_idle = of_property_read_bool(np, "samsung,rsc_4_frame_idle");
	LCD_INFO("rsc_4_frame_idle : %d\n", vdd->rsc_4_frame_idle);

	/* VRR: Variable Refresh Rate */
	vdd->vrr.support_vrr_based_bl = of_property_read_bool(np, "samsung,support_vrr_based_bl");

	/* BRR: Bridge Refresh Rate */
	vdd->vrr.is_support_brr = of_property_read_bool(np, "samsung,support_brr");
	vdd->vrr.is_support_black_frame = of_property_read_bool(np, "samsung,support_black_frame");
	vdd->vrr.is_support_delayed_perf = of_property_read_bool(np, "samsung,support_delayed_perf");

	/* Low Frequency Driving mode for HOP display */
	vdd->vrr.lfd.support_lfd = of_property_read_bool(np, "samsung,support_lfd");

	/* VRR: TE modulation  */
	vdd->vrr.support_te_mod = of_property_read_bool(np, "samsung,support_te_mod");

	LCD_INFO("support: vrr: %d, brr: %d, black_frame: %d, delayed_perf: %d, lfd: %d, te_mod: %d\n",
			vdd->vrr.support_vrr_based_bl, vdd->vrr.is_support_brr,
			vdd->vrr.is_support_black_frame,
			vdd->vrr.is_support_delayed_perf,
			vdd->vrr.lfd.support_lfd,
			vdd->vrr.support_te_mod);

	/* SUPPORT_SINGEL_TX */
	vdd->not_support_single_tx = of_property_read_bool(np, "samsung,not-support-single-tx");
	LCD_INFO("support_single_tx : %s\n",
		vdd->not_support_single_tx ? "do not support single tx" : "support single tx");

	/* Need exclusive tx for Brightness */
	vdd->need_brightness_lock= of_property_read_bool(np, "samsung,need_brightness_lock");
	LCD_INFO("need_brightness_lock : %s\n",
		vdd->need_brightness_lock ? "support brightness_lock" : "Not support brightness_lock");
	/* Not to send QCOM made PPS (A data type) */
	vdd->no_qcom_pps= of_property_read_bool(np, "samsung,no_qcom_pps");
	LCD_INFO("no_qcom_pps : %s\n",
		vdd->no_qcom_pps ? "support no_qcom_pps" : "Not support no_qcom_pps");

	/*
		AOT support : tddi video panel
		To keep high status panel power & reset
	*/
	vdd->aot_enable = of_property_read_bool(np, "samsung,aot_enable");
	LCD_INFO("aot_enable : %s\n",
		vdd->aot_enable ? "aot_enable support" : "aot_enable doesn't support");

	/* Read module ID at probe timing */
	vdd->support_early_id_read = of_property_read_bool(np, "samsung,support_early_id_read");
	LCD_INFO("support_early_id_read : %s\n",
		vdd->support_early_id_read ? "support" : "doesn't support");

	/*
		panel dsi/csi mipi strength value
	*/
	for (i = 0; i < SS_PHY_CMN_MAX; i++) {
		clear_bit(i, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[i] = 0;
	}

	if (!of_property_read_u32(np, "samsung,phy_vreg_ctrl_0", tmp)) {
		set_bit(SS_PHY_CMN_VREG_CTRL_0, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_VREG_CTRL_0] = tmp[0];
		LCD_INFO("phy_vreg_ctrl_0 : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_VREG_CTRL_0]);
	}

	if (!of_property_read_u32(np, "samsung,phy_ctrl2", tmp)) {
		set_bit(SS_PHY_CMN_CTRL_2, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_CTRL_2] = tmp[0];
		LCD_INFO("phy_ctrl2 : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_CTRL_2]);
	}

	if (!of_property_read_u32(np, "samsung,phy_offset_top_ctrl", tmp)) {
		set_bit(SS_PHY_CMN_GLBL_RESCODE_OFFSET_TOP_CTRL, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_TOP_CTRL] = tmp[0];
		LCD_INFO("phy_offset_top_ctrl : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_TOP_CTRL]);
	}

	if (!of_property_read_u32(np, "samsung,phy_offset_bot_ctrl", tmp)) {
		set_bit(SS_PHY_CMN_GLBL_RESCODE_OFFSET_BOT_CTRL, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_BOT_CTRL] = tmp[0];
		LCD_INFO("phy_offset_bot_ctrl : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_BOT_CTRL]);
	}

	if (!of_property_read_u32(np, "samsung,phy_offset_mid_ctrl", tmp)) {
		set_bit(SS_PHY_CMN_GLBL_RESCODE_OFFSET_MID_CTRL, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_MID_CTRL] = tmp[0];
		LCD_INFO("phy_offset_mid_ctrl : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_MID_CTRL]);
	}

	if (!of_property_read_u32(np, "samsung,phy_str_swi_cal_sel_ctrl", tmp)) {
		set_bit(SS_PHY_CMN_GLBL_STR_SWI_CAL_SEL_CTRL, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_STR_SWI_CAL_SEL_CTRL] = tmp[0];
		LCD_INFO("phy_str_swi_cal_sel_ctrl : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_STR_SWI_CAL_SEL_CTRL]);
	}

	/* sustain LP11 signal before LP00 on entering sleep status */
	rc = of_property_read_u32(np, "samsung,lp11_sleep_ms_time", tmp);
	vdd->lp11_sleep_ms_time = (!rc ? tmp[0] : 0);
	LCD_INFO("lp11_sleep_ms_time : %d\n",
		vdd->lp11_sleep_ms_time);

	rc = of_property_read_u32(np, "samsung,ddi_id_length", tmp);
	vdd->dtsi_data.ddi_id_length = (!rc ? tmp[0] : 0);
	LCD_INFO("ddi_id_length = [%d]\n", vdd->dtsi_data.ddi_id_length);
}

/***********/
/* A2 line */
/***********/

#define OTHER_PANEL_FILE "/efs/FactoryApp/a2_line.dat"

int read_line(char *src, char *buf, int *pos, int len)
{
	int idx = 0;

	LCD_DEBUG("(%d) ++\n", *pos);

	while (*(src + *pos) != 10 && *(src + *pos) != 13) {
		buf[idx] = *(src + *pos);

		idx++;
		(*pos)++;

		if (idx > MAX_READ_LINE_SIZE) {
			LCD_ERR("overflow!\n");
			return idx;
		}

		if (*pos >= len) {
			LCD_ERR("End of File (%d) / (%d)\n", *pos, len);
			return idx;
		}
	}

	while (*(src + *pos) == 10 || *(src + *pos) == 13)
		(*pos)++;

	LCD_DEBUG("--\n");

	return idx;
}

int ss_read_otherline_panel_data(struct samsung_display_driver_data *vdd)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret = 0;
	mm_segment_t fs;

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(OTHER_PANEL_FILE, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR "%s File open failed\n", __func__);

		if (!IS_ERR_OR_NULL(vdd->panel_func.set_panel_fab_type))
			vdd->panel_func.set_panel_fab_type(BASIC_FB_PANLE_TYPE);/*to work as original line panel*/
		ret = -ENOENT;
		goto err;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	LCD_INFO("Loading File Size : %ld(bytes)", l);

	dp = kmalloc(l + 10, GFP_KERNEL);
	if (dp == NULL) {
		LCD_INFO("Can't not alloc memory for tuning file load\n");
		filp_close(filp, current->files);
		ret = -1;
		goto err;
	}
	pos = 0;
	memset(dp, 0, l);

	LCD_INFO("before vfs_read()\n");
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	LCD_INFO("after vfs_read()\n");

	if (ret != l) {
		LCD_INFO("vfs_read() filed ret : %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		ret = -1;
		goto err;
	}

	if (!IS_ERR_OR_NULL(vdd->panel_func.parsing_otherline_pdata))
		ret = vdd->panel_func.parsing_otherline_pdata(filp, vdd, dp, l);

	filp_close(filp, current->files);

	set_fs(fs);

	kfree(dp);

	return ret;
err:
	set_fs(fs);
	return ret;
}

static void read_panel_data_work_fn(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
		container_of(work, struct samsung_display_driver_data,
			other_line_panel_support_work.work);
	int ret = 1;

	ret = ss_read_otherline_panel_data(vdd);

	if (ret && vdd->other_line_panel_work_cnt) {
		queue_delayed_work(vdd->other_line_panel_support_workq,
				&vdd->other_line_panel_support_work,
				msecs_to_jiffies(OTHERLINE_WORKQ_DEALY));
		vdd->other_line_panel_work_cnt--;
	} else
		destroy_workqueue(vdd->other_line_panel_support_workq);

	if (vdd->other_line_panel_work_cnt == 0)
		LCD_ERR(" cnt (%d)\n", vdd->other_line_panel_work_cnt);
}

/*********************/
/* LPM control       */
/*********************/
void ss_panel_low_power_config(struct samsung_display_driver_data *vdd, int enable)
{
	if (!vdd->dtsi_data.panel_lpm_enable) {
		LCD_INFO("[Panel LPM] LPM(ALPM/HLPM) is not supported\n");
		return;
	}

	ss_panel_lpm_power_ctrl(vdd, enable);

	if (enable) {
		vdd->esd_recovery.is_wakeup_source = true;
	} else {
		vdd->esd_recovery.is_wakeup_source = false;
	}

	ss_panel_lpm_ctrl(vdd, enable);
}

/*
 * ss_find_reg_offset()
 * This function find offset for reg value
 * reg_list[X][0] is reg value
 * reg_list[X][1] is offset for reg value
 * cmd_list is the target cmds for searching reg value
 */
int ss_find_reg_offset(int (*reg_list)[2],
		struct dsi_panel_cmd_set *cmd_list[], int list_size)
{
	struct dsi_panel_cmd_set *target_cmds = NULL;
	int i = 0, j = 0, max_cmd_cnt;

	if (IS_ERR_OR_NULL(reg_list) || IS_ERR_OR_NULL(cmd_list))
		goto end;

	for (i = 0; i < list_size; i++) {
		target_cmds = cmd_list[i];
		max_cmd_cnt = target_cmds->count;

		for (j = 0; j < max_cmd_cnt; j++) {
			if (target_cmds->cmds[j].msg.tx_buf &&
					target_cmds->cmds[j].msg.tx_buf[0] == reg_list[i][0]) {
				reg_list[i][1] = j;
				break;
			}
		}
	}

end:
	for (i = 0; i < list_size; i++)
		LCD_DEBUG("offset[%d] : %d\n", i, reg_list[i][1]);
	return 0;
}

static bool is_new_lpm_version(struct samsung_display_driver_data *vdd)
{
	if (vdd->panel_lpm.ver == LPM_VER1)
		return true;
	else
		return false;
}

static void set_lpm_br_values(struct samsung_display_driver_data *vdd)
{
	int from, end;
	int left, right, p = 0;
	int loop = 0;
	struct candela_map_table *table;
	int bl_level = vdd->br_info.common_br.bl_level;

	table = &vdd->br_info.candela_map_table[AOD][vdd->panel_revision];

	if (IS_ERR_OR_NULL(table->cd)) {
		LCD_ERR("No aod candela_map_table..\n");
		return;
	}

	LCD_DEBUG("table size (%d)\n", table->tab_size);

	if (bl_level > table->max_lv)
		bl_level = table->max_lv;

	left = 0;
	right = table->tab_size - 1;

	while (left <= right) {
		loop++;
		p = (left + right) / 2;
		from = table->from[p];
		end = table->end[p];
		LCD_DEBUG("[%d] from(%d) end(%d) / %d\n", p, from, end, bl_level);

		if (bl_level >= from && bl_level <= end)
			break;
		if (bl_level < from)
			right = p - 1;
		else
			left = p + 1;

		if (loop > table->tab_size) {
			LCD_ERR("can not find (%d) level in table!\n", bl_level);
			p = table->tab_size - 1;
			break;
		}
	};
	vdd->panel_lpm.lpm_bl_level = table->cd[p];

	LCD_DEBUG("%s: (%d)->(%d)\n",
		__func__, vdd->br_info.common_br.bl_level, vdd->panel_lpm.lpm_bl_level);

}

int ss_panel_lpm_power_ctrl(struct samsung_display_driver_data *vdd, int enable)
{
	int i;
	int rc = 0;
	int get_voltage;
	struct dsi_panel *panel = NULL;
	struct dsi_vreg *target_vreg = NULL;
	struct dsi_regulator_info regs;
	struct lpm_pwr_ctrl *lpm_pwr;

	if (!vdd->dtsi_data.panel_lpm_enable) {
		LCD_INFO("[Panel LPM] LPM(ALPM/HLPM) is not supported\n");
		return -ENODEV;
	}

	panel = GET_DSI_PANEL(vdd);
	if (IS_ERR_OR_NULL(panel)) {
		LCD_ERR("No Panel Data\n");
		return -ENODEV;
	}

	LCD_DEBUG("%s ++\n", enable == true ? "Enable" : "Disable");

	regs = panel->power_info;
	lpm_pwr = &vdd->panel_lpm.lpm_pwr;

	if (!lpm_pwr->support_lpm_pwr_ctrl) {
		LCD_ERR("%s: No panel power control for LPM\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&vdd->panel_lpm.lpm_lock);

	/* Find vreg for LPM setting */
	for (i = 0; i < regs.count; i++) {
		target_vreg = &regs.vregs[i];
		if (!strcmp(target_vreg->vreg_name, lpm_pwr->lpm_pwr_ctrl_supply_name)) {
			LCD_INFO("Found Voltage(%d)\n", i);
			break;
		}
	}

	/* To check previous voltage */
	get_voltage = regulator_get_voltage(target_vreg->vreg);

	if (enable) { /* AOD ON(Enter) */
		if (get_voltage != lpm_pwr->lpm_pwr_ctrl_supply_min_v) {
			rc = regulator_set_voltage(
					target_vreg->vreg,
					lpm_pwr->lpm_pwr_ctrl_supply_min_v,
					lpm_pwr->lpm_pwr_ctrl_supply_max_v);
			if (rc < 0) {
				LCD_ERR("Voltage Set Fail enable=%d voltage : %d rc : %d\n",
							enable, lpm_pwr->lpm_pwr_ctrl_supply_min_v, rc);
			} else {
				get_voltage = regulator_get_voltage(target_vreg->vreg);
				LCD_INFO("enable=%d, current get_voltage=%d rc : %d\n", enable, get_voltage, rc);
			}

			/* normal on -> lpm */
			ss_panel_regulator_short_detection(vdd, PANEL_ON);
		} else
			LCD_DEBUG("enable=%d, previous voltage : %d\n", enable, get_voltage);

	} else { /* AOD OFF(Exit) */
		if (get_voltage != target_vreg->min_voltage) {
			rc = regulator_set_voltage(
						target_vreg->vreg,
						target_vreg->min_voltage,
						target_vreg->max_voltage);
			if (rc < 0) {
				LCD_ERR("Voltage Set Fail enable=%d voltage : %d rc : %d\n",
							enable, target_vreg->min_voltage, rc);
				panic("Voltage Set Fail to NORMAL");
			} else {
				get_voltage = regulator_get_voltage(target_vreg->vreg);
				LCD_INFO("enable=%d, current get_voltage=%d\n", enable, get_voltage);

				if (get_voltage != target_vreg->min_voltage)
					panic("Voltage Set Fail to NORMAL");
			}

			/* lpm -> normal on */
			ss_panel_regulator_short_detection(vdd, PANEL_LPM);
		} else
			LCD_DEBUG("enable=%d, previous voltage : %d\n", enable, get_voltage);
	}

	mutex_unlock(&vdd->panel_lpm.lpm_lock);
	LCD_DEBUG("[Panel LPM] --\n");

	return rc;
}

void ss_panel_lpm_ctrl(struct samsung_display_driver_data *vdd, int enable)
{
	static int stored_bl_level; /* Used for factory mode only */
	int current_bl_level = 0;
	s64 lpm_init_delay_us = 0;

	LCD_INFO("[Panel LPM][DIPSLAY_%d] ++\n", vdd->ndx);

	if (!vdd->dtsi_data.panel_lpm_enable) {
		LCD_INFO("[Panel LPM][DIPSLAY_%d] LPM(ALPM/HLPM) is not supported\n", vdd->ndx);
		return;
	}

	if (ss_is_panel_off(vdd)) {
		LCD_INFO("[Panel LPM][DIPSLAY_%d] Do not change mode\n", vdd->ndx);
		goto end;
	}

	lpm_init_delay_us = (s64)vdd->dtsi_data.samsung_lpm_init_delay * 1000;

	mutex_lock(&vdd->panel_lpm.lpm_lock);

	/* To avoid unexpected ESD detction, Disable ESD irq before cmd tx related with lpm */
	if (vdd->esd_recovery.esd_irq_enable)
			vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd);

	if (enable) { /* AOD ON(Enter) */
		if (unlikely(vdd->is_factory_mode) && !ss_is_panel_lpm(vdd)) {
			LCD_INFO("[Panel LPM][DIPSLAY_%d] Set low brightness for factory mode (%d) \n", vdd->ndx, vdd->br_info.common_br.bl_level);
			stored_bl_level = vdd->br_info.common_br.bl_level;
			ss_brightness_dcs(vdd, 0, BACKLIGHT_NORMAL);
			LCD_INFO("[Panel LPM][DIPSLAY_%d] Send DISPLAY_OFF for factory mode\n", vdd->ndx);
			ss_send_cmd(vdd, TX_DISPLAY_OFF);
		}

		if (vdd->panel_func.samsung_update_lpm_ctrl_cmd) {
			vdd->panel_func.samsung_update_lpm_ctrl_cmd(vdd);
			LCD_INFO("[Panel LPM][DIPSLAY_%d] update lpm cmd done(on)\n", vdd->ndx);
		}

		/* lpm init delay */
		if (lpm_init_delay_us) {
			s64 delay_us = lpm_init_delay_us -
						ktime_us_delta(ktime_get(), vdd->tx_set_on_time);

			LCD_INFO("[Panel LPM][DIPSLAY_%d] %lld us delay before turn on lpm mode\n",
					vdd->ndx, (delay_us < 0) ? 0 : delay_us);
			if (delay_us > 0)
				usleep_range(delay_us, delay_us);
		}

		/* Self Display Setting */
		if (vdd->self_disp.aod_enter)
			vdd->self_disp.aod_enter(vdd);

		if (unlikely(vdd->is_factory_mode))
			ss_send_cmd(vdd, TX_FD_OFF);

		if (!ss_is_panel_lpm(vdd)) {
			LCD_INFO("tx lpm_on_pre\n");
			ss_send_cmd(vdd, TX_LPM_ON_PRE);
		}

 		LCD_INFO("[Panel LPM][DIPSLAY_%d] Send panel LPM cmds +++\n", vdd->ndx);
		mutex_lock(&vdd->panel_lpm.lpm_bl_lock);
		ss_send_cmd(vdd, TX_LPM_ON);
 		LCD_INFO("[Panel LPM][DIPSLAY_%d] Send panel LPM cmds ---\n", vdd->ndx);

		if (unlikely(vdd->is_factory_mode)) {
			/* Do not apply self grid in case of factort lpm (conrolled by adb cmd) */
			ss_send_cmd(vdd, TX_DISPLAY_ON);
			/*
			* 1Frame Delay(33.4ms - 30FPS) Should be added for the project that needs FD cmd
			* Swire toggled at TX_FD_OFF, TX_LPM_ON, TX_FD_ON, TX_LPM_OFF.
			* And the on-off sequences in factory binary are like below
			* AOD on : ESD disable -> TX_FD_OFF -> TX_LPM_ON -> need 34ms delay -> ESD enable
			* AOD off : ESD disable -> TX_LPM_OFF -> TX_FD_ON -> need 34ms delay -> ESD enable
			* so we need 34ms delay for TX_LPM_ON & TX_FD_ON respectively.
			* But to reduce flicker issue, Add delay 34ms after 29(display on) cmd.
			*/
			if(!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_FD_OFF)))
				usleep_range(34*1000, 34*1000);
		} else {
			/* The display_on cmd will be sent on next commit */
			vdd->display_status_dsi.wait_disp_on = true;
			vdd->display_status_dsi.wait_actual_disp_on = true;
			LCD_INFO("[Panel LPM][DIPSLAY_%d] Set wait_disp_on to true\n", vdd->ndx);
		}

 		LCD_INFO("[Panel LPM][DIPSLAY_%d] panel_state: %d -> LPM\n",
				vdd->ndx, vdd->panel_state);
		ss_set_panel_state(vdd, PANEL_PWR_LPM);
		mutex_unlock(&vdd->panel_lpm.lpm_bl_lock);

		/*
			Update mdnie to disable mdnie operation by scenario at AOD display status.
		*/
		if (vdd->mdnie.support_mdnie) {
			update_dsi_tcon_mdnie_register(vdd);
		}


	} else { /* AOD OFF(Exit) */
		/* Self Display Setting */
		if (vdd->self_disp.aod_exit)
			vdd->self_disp.aod_exit(vdd);

		if (vdd->panel_func.samsung_update_lpm_ctrl_cmd) {
			vdd->panel_func.samsung_update_lpm_ctrl_cmd(vdd);
			LCD_INFO("[Panel LPM][DIPSLAY_%d] update lpm cmd done(off)\n", vdd->ndx);
		}

		/* Turn Off ALPM Mode */
		ss_send_cmd(vdd, TX_LPM_OFF);

		/* 1Frame Delay(33.4ms - 30FPS) Should be added */
		usleep_range(34*1000, 34*1000);

		LCD_INFO("[Panel LPM][DIPSLAY_%d] Send panel LPM off cmds\n", vdd->ndx);

		if (unlikely(vdd->is_factory_mode)) {
			LCD_INFO("[Panel LPM][DIPSLAY_%d] restore bl_level for factory (%d) \n", vdd->ndx, stored_bl_level);
			current_bl_level = stored_bl_level;
		} else {
			current_bl_level = USE_CURRENT_BL_LEVEL;
		}

		LCD_INFO("[Panel LPM][DIPSLAY_%d] Restore brightness level (%d) \n", vdd->ndx, current_bl_level);

		ss_set_panel_state(vdd, PANEL_PWR_ON);

		if ((vdd->support_optical_fingerprint) && (vdd->br_info.common_br.finger_mask_hbm_on)) {
			/* SAMSUNG_FINGERPRINT */
			ss_brightness_dcs(vdd, 0, BACKLIGHT_FINGERMASK_ON);
		}
		else
			ss_brightness_dcs(vdd, current_bl_level, BACKLIGHT_NORMAL);

		if (vdd->mdnie.support_mdnie) {
			vdd->mdnie.lcd_on_notifiy = true;
			update_dsi_tcon_mdnie_register(vdd);
			if (vdd->mdnie.support_trans_dimming)
				vdd->mdnie.disable_trans_dimming = false;
		}

		if (vdd->panel_func.samsung_cover_control && vdd->cover_control)
			vdd->panel_func.samsung_cover_control(vdd);

		if (unlikely(vdd->is_factory_mode)) {
			ss_send_cmd(vdd, TX_FD_ON);
			ss_send_cmd(vdd, TX_DISPLAY_ON);
			ss_set_panel_state(vdd, PANEL_PWR_ON);
			/*
			* 1Frame Delay(33.4ms - 30FPS) Should be added for the project that needs FD cmd
			* Swire toggled at TX_FD_OFF, TX_LPM_ON, TX_FD_ON, TX_LPM_OFF.
			* And the on-off sequences in factory binary are like below
			* AOD on : ESD disable -> TX_FD_OFF -> TX_LPM_ON -> need 34ms delay -> ESD enable
			* AOD off : ESD disable -> TX_LPM_OFF -> TX_FD_ON -> need 34ms delay -> ESD enable
			* so we need 34ms delay for TX_LPM_ON & TX_FD_ON respectively.
			* But to reduce flicker issue, Add delay 34ms after 29(display on) cmd.
			*/
			if(!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_FD_ON)))
				usleep_range(34*1000, 34*1000);
		} else {
			/* The display_on cmd will be sent on next commit */
			vdd->display_status_dsi.wait_disp_on = true;
			vdd->display_status_dsi.wait_actual_disp_on = true;
			LCD_INFO("[Panel LPM][DIPSLAY_%d] Set wait_disp_on to true\n", vdd->ndx);
		}
	}

	/* To avoid unexpected ESD detction, Enable ESD irq after cmd tx related with lpm */
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd);

	LCD_INFO("[Panel LPM][DIPSLAY_%d] En/Dis : %s, LPM_MODE : %s, Hz : 30Hz, bl_level : %s\n",
				vdd->ndx,
				/* Enable / Disable */
				enable ? "Enable" : "Disable",
				/* Check LPM mode */
				vdd->panel_lpm.mode == ALPM_MODE_ON ? "ALPM" :
				vdd->panel_lpm.mode == HLPM_MODE_ON ? "HLPM" :
				vdd->panel_lpm.mode == LPM_MODE_OFF ? "MODE_OFF" : "UNKNOWN",
				/* Check current brightness level */
				vdd->panel_lpm.lpm_bl_level == LPM_2NIT ? "2NIT" :
				vdd->panel_lpm.lpm_bl_level == LPM_10NIT ? "10NIT" :
				vdd->panel_lpm.lpm_bl_level == LPM_30NIT ? "30NIT" :
				vdd->panel_lpm.lpm_bl_level == LPM_60NIT ? "60NIT" : "UNKNOWN");

	mutex_unlock(&vdd->panel_lpm.lpm_lock);
end:
	LCD_INFO("[Panel LPM][DIPSLAY_%d] --\n", vdd->ndx);
}

/* This is depricated.. Get vdd pointer not from global variable. */
struct samsung_display_driver_data *ss_get_vdd(enum ss_display_ndx ndx)
{
	if (ndx >= MAX_DISPLAY_NDX || ndx < 0) {
		LCD_DEBUG("invalid ndx(%d)\n", ndx);
		return NULL;
	}

	return &vdd_data[ndx];
}

/*
 * @param:
 *	D0: hall_ic (the hall ic status, 0: foder open. 1: folder close)
 *	D8: flip_not_refresh (0: refresh after flipping. 1: don't refresh after flipping)
 */
int samsung_display_hall_ic_status(struct notifier_block *nb,
				unsigned long param, void *data)
{
	struct samsung_display_driver_data *vdd = container_of(nb,
			struct samsung_display_driver_data, hall_ic_notifier_display);
	bool hall_ic = (bool)(param & 0x1);
	//bool flip_not_refresh = (bool)(!!(param & LCD_FLIP_NOT_REFRESH));
	struct sde_connector *conn = GET_SDE_CONNECTOR(vdd);
	struct drm_event event;
	bool panel_dead = false;

	/*
		previous panel off -> current panel on
		foder open : 0, close : 1
	*/

	if (!vdd->support_hall_ic)
		return 0;

	LCD_ERR("mdss hall_ic : %s, start\n", hall_ic ? "CLOSE" : "OPEN");

#if 0
	if (ss_panel_attached(PRIMARY_DISPLAY_NDX) && ss_panel_attached(SECONDARY_DISPLAY_NDX)) {
		/* To check current blank mode */
		if ((ss_is_panel_on(vdd) ||
			ss_is_panel_lpm(vdd)) &&
			vdd->hall_ic_status != hall_ic) {

			/* set flag */
			vdd->hall_ic_mode_change_trigger = true;
			vdd->lcd_flip_not_refresh = flip_not_refresh;

			/* panel off */
			// TODO: off panel..
			// call msm_disable_outputs()..???
			LCD_ERR("should implement panel off...\n");

			/* set status */
			vdd->hall_ic_status = hall_ic;

			/* panel on */
			// TODO: off panel..
			// call complete_commit()..???
			LCD_ERR("should implement panel on...\n");

			/* clear flag */
			vdd->hall_ic_mode_change_trigger = false;
			vdd->lcd_flip_not_refresh = false;

			/* Brightness setting */
			// TODO: check if it controls right display in dual dsi or dual display...
			if (ss_is_bl_dcs(vdd))
				ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

			/* display on */
			if (ss_is_video_mode(vdd))
				ss_send_cmd(vdd, TX_DISPLAY_ON);

			/* refresh a frame to panel */
			if (ss_is_cmd_mode(vdd) && !flip_not_refresh) {
				/* TODO: refresh a frame to panel.. with drm msm code...
				   call complete_commit()..??
				fbi->fbops->fb_pan_display(&fbi->var, fbi);
				*/
				LCD_ERR("need to refresh a frame to panel.. with drm msm code...\n");
			}
		} else {
			vdd->hall_ic_status = hall_ic;
			LCD_ERR("mdss skip display changing\n");
		}
	} else {
#endif
	/* check the lcd id for DISPLAY_1 and DISPLAY_2 */
	if (ss_panel_attached(PRIMARY_DISPLAY_NDX) && ss_panel_attached(SECONDARY_DISPLAY_NDX)) {
		/* set status */
		vdd->hall_ic_status_unhandled = hall_ic;
		//vdd->panel_dead = true; panel switch, should not set panel_dead, else MTP is not read

		panel_dead = true;
		event.type = DRM_EVENT_PANEL_DEAD;
		event.length = sizeof(bool);
		msm_mode_object_event_notify(&conn->base.base,
			conn->base.dev, &event, (u8 *)&panel_dead);
		/* TODO: send flip_not_refresh to HAL, and let HAL to handle it. */
	} else {
		/* check the lcd id for DISPLAY_1 */
		if (ss_panel_attached(PRIMARY_DISPLAY_NDX))
			vdd->hall_ic_status = HALL_IC_OPEN;

		/* check the lcd id for DISPLAY_2 */
		if (ss_panel_attached(SECONDARY_DISPLAY_NDX))
			vdd->hall_ic_status = HALL_IC_CLOSE;
	}

	return 0;
}

static void samsung_display_delay_disp_on_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
		container_of(work, struct samsung_display_driver_data,
				delay_disp_on_work.work);

	LCD_INFO("wait_disp_on is set\n");
	vdd->display_status_dsi.wait_disp_on = true;
	ss_send_cmd(vdd, TX_DISPLAY_ON);
}

int get_hall_ic_status(char *mode)
{
	struct samsung_display_driver_data *vdd;
	int status;

	if (mode == NULL)
		return true;

	if (*mode - '0')
		status = HALL_IC_CLOSE;
	else
		status = HALL_IC_OPEN;


	vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);
	vdd->hall_ic_status = status;

	vdd = ss_get_vdd(SECONDARY_DISPLAY_NDX);
	vdd->hall_ic_status = status;

	LCD_ERR("hall_ic : %s\n", status ? "CLOSE" : "OPEN");

	return true;
}
EXPORT_SYMBOL(get_hall_ic_status);
__setup("hall_ic=0x", get_hall_ic_status);

/***************************************************************************************************
*		BRIGHTNESS RELATED FUNCTION.
****************************************************************************************************/
static void set_normal_br_values(struct samsung_display_driver_data *vdd)
{
	int from, end;
	int left, right, p = 0;
	int loop = 0;
	struct candela_map_table *table;

	if (vdd->br_info.common_br.pac)
		table = &vdd->br_info.candela_map_table[PAC_NORMAL][vdd->panel_revision];
	else {
		if(vdd->br_info.common_br.multi_to_one_support)
			table = &vdd->br_info.candela_map_table[MULTI_TO_ONE_NORMAL][vdd->panel_revision];
		else
			table = &vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision];
	}

	if (IS_ERR_OR_NULL(table->cd)) {
		LCD_ERR("No candela_map_table.. \n");
		return;
	}

	LCD_DEBUG("table size (%d)\n", table->tab_size);

	if (vdd->br_info.common_br.bl_level > table->max_lv)
		vdd->br_info.common_br.bl_level = table->max_lv;

	left = 0;
	right = table->tab_size - 1;

	while (left <= right) {
		loop++;
		p = (left + right) / 2;
		from = table->from[p];
		end = table->end[p];
		LCD_DEBUG("[%d] from(%d) end(%d) / %d\n", p, from, end, vdd->br_info.common_br.bl_level);

		if (vdd->br_info.common_br.bl_level >= from && vdd->br_info.common_br.bl_level <= end)
			break;
		if (vdd->br_info.common_br.bl_level < from)
			right = p - 1;
		else
			left = p + 1;
		LCD_DEBUG("left(%d) right(%d)\n", left, right);

		if (loop > table->tab_size) {
			LCD_ERR("can not find (%d) level in table!\n", vdd->br_info.common_br.bl_level);
			p = table->tab_size - 1;
			break;
		}
	};

	// set values..
	vdd->br_info.common_br.cd_idx = table->idx[p];
	vdd->br_info.common_br.cd_level = table->cd[p];

	if (vdd->br_info.common_br.pac) {
		vdd->br_info.common_br.pac_cd_idx = table->scaled_idx[p];
		vdd->br_info.common_br.interpolation_cd = table->interpolation_cd[p];

		LCD_INFO("[%d] pac_cd_idx (%d) cd_idx (%d) cd (%d) interpolation_cd (%d)\n",
				p, vdd->br_info.common_br.pac_cd_idx,
				vdd->br_info.common_br.cd_idx,
				vdd->br_info.common_br.cd_level,
				vdd->br_info.common_br.interpolation_cd);
	} else {
		if (vdd->br_info.common_br.gamma_mode2_support) {
			vdd->br_info.common_br.gm2_wrdisbv = table->gm2_wrdisbv[p];
			LCD_INFO("[%d] cd_idx (%d) cd (%d) gm2_wrdisbv (%d)\n",
					p, vdd->br_info.common_br.cd_idx,
					vdd->br_info.common_br.cd_level,
					vdd->br_info.common_br.gm2_wrdisbv);
		} else {
			LCD_INFO("[%d] cd_idx (%d) cd (%d) \n",
					p, vdd->br_info.common_br.cd_idx,
					vdd->br_info.common_br.cd_level);
		}
	}
	return;
}

static void set_hbm_br_values(struct samsung_display_driver_data *vdd)
{
	int from, end;
	int left, right, p;
	int loop = 0;
	struct candela_map_table *table;

	if (vdd->br_info.common_br.pac)
		table = &vdd->br_info.candela_map_table[PAC_HBM][vdd->panel_revision];
	else
		table = &vdd->br_info.candela_map_table[HBM][vdd->panel_revision];

	if (IS_ERR_OR_NULL(table->cd)) {
		LCD_ERR("No hbm candela_map_table..\n");
		return;
	}

	if (vdd->br_info.common_br.bl_level > table->max_lv)
		vdd->br_info.common_br.bl_level = table->max_lv;

	left = 0;
	right = table->tab_size - 1;

	while (left <= right) {
		loop++;
		p = (left + right) / 2;
		from = table->from[p];
		end = table->end[p];
		LCD_DEBUG("[%d] from(%d) end(%d) / %d\n", p, from, end, vdd->br_info.common_br.bl_level);

		if (vdd->br_info.common_br.bl_level >= from && vdd->br_info.common_br.bl_level <= end)
			break;
		if (vdd->br_info.common_br.bl_level < from)
			right = p - 1;
		else
			left = p + 1;
		LCD_DEBUG("left(%d) right(%d)\n", left, right);

		if (loop > table->tab_size) {
			LCD_ERR("can not find (%d) level in table!\n", vdd->br_info.common_br.bl_level);
			p = table->tab_size - 1;
			break;
		}
	};

	// set values..
	vdd->br_info.common_br.cd_idx = table->idx[p];
	vdd->br_info.common_br.cd_level = table->cd[p];
	vdd->br_info.common_br.auto_level = table->auto_level[p];

	if (vdd->br_info.common_br.pac) {
		vdd->br_info.common_br.pac_cd_idx = table->scaled_idx[p];
		vdd->br_info.common_br.interpolation_cd = table->cd[p];

		LCD_INFO("[%d] pac_cd_idx (%d) cd_idx (%d) cd (%d) interpolation_cd (%d) auto (%d)\n",
				p, vdd->br_info.common_br.pac_cd_idx,
				vdd->br_info.common_br.cd_idx,
				vdd->br_info.common_br.cd_level,
				vdd->br_info.common_br.interpolation_cd,
				vdd->br_info.common_br.auto_level);
	} else {
		if (vdd->br_info.common_br.gamma_mode2_support) {
			vdd->br_info.common_br.gm2_wrdisbv = table->gm2_wrdisbv[p];
			LCD_INFO("[%d] cd_idx (%d) cd (%d) gm2_wrdisbv (%d)\n",
					p, vdd->br_info.common_br.cd_idx,
					vdd->br_info.common_br.cd_level,
					vdd->br_info.common_br.gm2_wrdisbv);
		} else {
			LCD_INFO("[%d] cd_idx (%d) cd (%d) interpolation_cd (%d) auto (%d) \n",
					p, vdd->br_info.common_br.cd_idx,
					vdd->br_info.common_br.cd_level,
					vdd->br_info.common_br.interpolation_cd,
					vdd->br_info.common_br.auto_level);
		}
	}

	return;
}

static void ss_update_brightness_packet(struct dsi_cmd_desc *packet,
		int *count, struct dsi_panel_cmd_set *tx_cmd)
{
	int loop = 0;

	if (IS_ERR_OR_NULL(packet)) {
		LCD_ERR("%ps no packet\n", __builtin_return_address(0));
		return;
	}

	if (SS_IS_CMDS_NULL(tx_cmd)) {
		LCD_DEBUG("%ps no tx_cmd\n", __builtin_return_address(0));
		return;
	}

	if (*count > (BRIGHTNESS_MAX_PACKET - 1))
		panic("over max brightness_packet size(%d).. !!",
				BRIGHTNESS_MAX_PACKET);

	for (loop = 0; loop < tx_cmd->count; loop++)
		packet[(*count)++] = tx_cmd->cmds[loop];
}

void update_packet_level_key_enable(struct samsung_display_driver_data *vdd,
		struct dsi_cmd_desc *packet, int *cmd_cnt, int level_key)
{
	if (!level_key)
		return;
	else {
		if (level_key & LEVEL0_KEY)
			ss_update_brightness_packet(packet, cmd_cnt, ss_get_cmds(vdd, TX_LEVEL0_KEY_ENABLE));

		if (level_key & LEVEL1_KEY)
			ss_update_brightness_packet(packet, cmd_cnt, ss_get_cmds(vdd, TX_LEVEL1_KEY_ENABLE));

		if (level_key & LEVEL2_KEY)
			ss_update_brightness_packet(packet, cmd_cnt, ss_get_cmds(vdd, TX_LEVEL2_KEY_ENABLE));
	}
}

void update_packet_level_key_disable(struct samsung_display_driver_data *vdd,
		struct dsi_cmd_desc *packet, int *cmd_cnt, int level_key)
{
	if (!level_key)
		return;
	else {
		if (level_key & LEVEL0_KEY)
			ss_update_brightness_packet(packet, cmd_cnt, ss_get_cmds(vdd, TX_LEVEL0_KEY_DISABLE));

		if (level_key & LEVEL1_KEY)
			ss_update_brightness_packet(packet, cmd_cnt, ss_get_cmds(vdd, TX_LEVEL1_KEY_DISABLE));

		if (level_key & LEVEL2_KEY)
			ss_update_brightness_packet(packet, cmd_cnt, ss_get_cmds(vdd, TX_LEVEL2_KEY_DISABLE));
	}
}

static int ss_hbm_brightness_packet_set(
		struct samsung_display_driver_data *vdd)
{
	int cmd_cnt = 0;
	int level_key = 0;
	struct dsi_panel_cmd_set *set;
	struct dsi_cmd_desc *packet = NULL;
	struct dsi_panel_cmd_set *tx_cmd = NULL;

	/* init packet */
	set = ss_get_cmds(vdd, TX_BRIGHT_CTRL);
	if (SS_IS_CMDS_NULL(set)) {
		LCD_ERR("No cmds for TX_BRIGHT_CTRL.. \n");
		return -EINVAL;
	}

	packet = set->cmds;

	set_hbm_br_values(vdd);

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_tot_level)) {
		level_key = vdd->panel_func.samsung_brightness_tot_level(vdd);
		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
	}

	/* acl */
	if (vdd->br_info.acl_status || vdd->siop_status) {
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_acl_on)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_hbm_acl_on(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}
	} else {
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_acl_off)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_hbm_acl_off(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}
	}

	/* IRC */
	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_irc)) {
		level_key = false;
		tx_cmd = vdd->panel_func.samsung_hbm_irc(vdd, &level_key);

		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
		ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	/* Gamma */
	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_gamma)) {
		level_key = false;
		tx_cmd = vdd->panel_func.samsung_hbm_gamma(vdd, &level_key);

		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
		ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	/* gamma compensation for gamma mode2 48/96hz VRR modes */
	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_gm2_gamma_comp)) {
		level_key = false;
		tx_cmd = vdd->panel_func.samsung_brightness_gm2_gamma_comp(vdd, &level_key);

		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
		ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	/* vint */
	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vint)) {
		level_key = false;
		tx_cmd = vdd->panel_func.samsung_brightness_vint(vdd, &level_key);

		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
		ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	/* LTPS: used for normal and HBM brightness */
	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_ltps)) {
		level_key = false;
		tx_cmd = vdd->panel_func.samsung_brightness_ltps(vdd, &level_key);

		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
		ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	/* mAFPC */
	if (vdd->mafpc.is_support && !IS_ERR_OR_NULL(vdd->mafpc.brightness_scale)) {
		level_key = false;
		tx_cmd = vdd->mafpc.brightness_scale(vdd, &level_key);

		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
		ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	/* hbm etc */
	if (ss_get_cmds(vdd, TX_HBM_ETC)->count) {
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_etc)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_hbm_etc(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}
	}

	/* VRR */
	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vrr_hbm)) {
		level_key = false;
		tx_cmd = vdd->panel_func.samsung_brightness_vrr_hbm(vdd, &level_key);

		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
		ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_tot_level)) {
		level_key = vdd->panel_func.samsung_brightness_tot_level(vdd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	return cmd_cnt;
}

static int ss_normal_brightness_packet_set(
		struct samsung_display_driver_data *vdd)
{
	int cmd_cnt = 0;
	int level_key = 0;
	struct dsi_panel_cmd_set *set;
	struct dsi_cmd_desc *packet = NULL;
	struct dsi_panel_cmd_set *tx_cmd = NULL;

	/* init packet */
	set = ss_get_cmds(vdd, TX_BRIGHT_CTRL);
	if (SS_IS_CMDS_NULL(set)) {
		LCD_ERR("No cmds for TX_BRIGHT_CTRL.. \n");
		return -EINVAL;
	}

	packet = set->cmds;

	vdd->br_info.common_br.auto_level = 0;

	set_normal_br_values(vdd);

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_tot_level)) {
		level_key = vdd->panel_func.samsung_brightness_tot_level(vdd);
		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
	}

	if (vdd->br_info.smart_dimming_loaded_dsi) { /* OCTA PANEL */
		/* hbm off */
		if (vdd->display_status_dsi.hbm_mode) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_hbm_off)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_hbm_off(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}
		}

		/* aid/aor */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_aid)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_aid(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* acl */
		if (vdd->br_info.acl_status || vdd->siop_status) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_acl_on)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_acl_on(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}

			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_pre_acl_percent)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_pre_acl_percent(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}

			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_acl_percent)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_acl_percent(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}
		} else {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_acl_off)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_acl_off(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}
		}

		/* elvss */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_elvss)) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_pre_elvss)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_pre_elvss(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}

			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_elvss(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* temperature elvss */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_elvss_temperature1)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_elvss_temperature1(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_elvss_temperature2)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_elvss_temperature2(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* caps*/
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_caps)) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_pre_caps)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_pre_caps(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_caps(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}
		/* Manual DBV (for DIA setting) */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_dbv)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_dbv(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* vint */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vint)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_vint(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* IRC */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_irc)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_irc(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* LTPS: used for normal and HBM brightness */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_ltps)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_ltps(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* PANEL SPECIFIC SETTINGS */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_normal_etc)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_normal_etc(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* mAFPC */
		if (vdd->mafpc.is_support && !IS_ERR_OR_NULL(vdd->mafpc.brightness_scale)) {
			level_key = false;
			tx_cmd = vdd->mafpc.brightness_scale(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* gamma */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_gamma)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_gamma(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* VRR */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vrr)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_vrr(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}
	} else { /* Gamma Mode2 or TFT PANEL */
		if (vdd->br_info.acl_status || vdd->siop_status) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_acl_on)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_acl_on(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}

			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_pre_acl_percent)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_pre_acl_percent(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}

			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_acl_percent)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_acl_percent(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}
		} else {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_acl_off)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_acl_off(vdd, &level_key);

				update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
				ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
			}
		}

		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_tft_pwm_ldi)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_tft_pwm_ldi(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* mAFPC */
		if (vdd->mafpc.is_support && !IS_ERR_OR_NULL(vdd->mafpc.brightness_scale)) {
			level_key = false;
			tx_cmd = vdd->mafpc.brightness_scale(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* gamma */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_gamma)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_gamma(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* gamma compensation for gamma mode2 48/96hz VRR modes */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_gm2_gamma_comp)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_gm2_gamma_comp(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* vint */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vint)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_vint(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* VRR */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vrr)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_vrr(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

	}

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_tot_level)) {
		level_key = vdd->panel_func.samsung_brightness_tot_level(vdd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	return cmd_cnt;
}

int ss_single_transmission_packet(struct dsi_panel_cmd_set *cmds)
{
	int loop;
	struct dsi_cmd_desc *packet = cmds->cmds;
	int packet_cnt = cmds->count;

	for (loop = 0; (loop < packet_cnt) && (loop < BRIGHTNESS_MAX_PACKET); loop++) {
		if (packet[loop].msg.type == MIPI_DSI_DCS_LONG_WRITE ||
			packet[loop].msg.type == MIPI_DSI_GENERIC_LONG_WRITE) {
			packet[loop].last_command = false;
		} else {
			if (loop > 0)
				packet[loop - 1].last_command = true; /*To ensure previous single tx packet */

			packet[loop].last_command = true;
		}

		/* use null packet for last_command in brightness setting */
		if (packet[loop].msg.tx_buf[0] == 0x0)
			packet[loop].last_command = true;
	}

	if (loop == BRIGHTNESS_MAX_PACKET)
		return false;
	else {
		packet[loop - 1].last_command = true; /* To make last packet flag */
		return true;
	}
}

bool is_hbm_level(struct samsung_display_driver_data *vdd)
{

	struct candela_map_table *table;

	if (vdd->br_info.common_br.pac)
		table = &vdd->br_info.candela_map_table[PAC_HBM][vdd->panel_revision];
	else
		table = &vdd->br_info.candela_map_table[HBM][vdd->panel_revision];

	if (vdd->br_info.common_br.bl_level < table->min_lv)
		return false;

	if (!table->max_lv) /*max should not be zero */
		return false;

	if (vdd->br_info.common_br.bl_level > table->max_lv) {
		LCD_ERR("bl_level(%d) is over max_level (%d), force set to max\n", vdd->br_info.common_br.bl_level, table->max_lv);
		vdd->br_info.common_br.bl_level = table->max_lv;
	}
	return true;
}

/* ss_brightness_dcs() is called not in locking status.
 * Instead, calls ss_set_backlight() when you need to controll backlight
 * in locking status.
 */
int ss_brightness_dcs(struct samsung_display_driver_data *vdd, int level, int backlight_origin)
{
	int cmd_cnt = 0;
	int ret = 0;
	int need_lpm_lock = 0;
	struct dsi_panel_cmd_set *brightness_cmds = NULL;
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	static int backup_bl_level, backup_acl;

#if defined(CONFIG_SEC_C1Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
	if (!strcmp(panel->name, "ss_dsi_panel_S6E3HAB_AMB687TZ01_WQHD") &&
			level != USE_CURRENT_BL_LEVEL) {
		LCD_INFO("hubble panel for canvas, change level: %d -> %d\n",
				level, (level * 100));
		level *= 100;
	}
#endif

	/* FC2 change: set panle mode in SurfaceFlinger initialization, instead of kenrel booting... */
	if (!panel->cur_mode) {
		LCD_ERR("err: no panel mode yet...\n");
		return false;
	}

	/*
	 * panel_lpm.lpm_lock should be locked
	 * to avoid brightness mis-handling (LPM brightness <-> normal brightness) from other thread
	 * but running ss_panel_lpm_ctrl thread
	*/
	if (ss_is_panel_lpm(vdd)) need_lpm_lock = 1;
	if (need_lpm_lock) mutex_lock(&vdd->panel_lpm.lpm_lock);

	/* check BRR_STOP_MODE before bl_lock to avoid deadlock and BRR_OFF
	 *  wait-timeout, with ss_panel_vrr_switch() funciton.
	 * TODO: compare candela, instead of bl_level.
	 */
	if (vdd->vrr.is_support_brr &&
			level != USE_CURRENT_BL_LEVEL &&
			vdd->br_info.common_br.bl_level != level) {
		/* check brightness setting, and stop bridge RR, and jump to target RR. */
		mutex_lock(&vdd->vrr.brr_lock);
		if (ss_is_brr_on(vdd->vrr.brr_mode)) {
			int max_cnt = 100; /* 100ms */

			vdd->vrr.brr_mode = BRR_STOP_MODE;
			vdd->vrr.brr_bl_level = level;
			mutex_unlock(&vdd->vrr.brr_lock);

			LCD_INFO("VRR: new brightness(%d->%d), set to BRR_STOP_MODE\n",
					vdd->br_info.common_br.bl_level, level);

			while (vdd->vrr.brr_mode != BRR_OFF_MODE && max_cnt--)
				usleep_range(1000, 1000);

			if (max_cnt < 0)
				LCD_ERR("BRR OFF timeout: (%s)\n", ss_get_brr_mode_name(vdd->vrr.brr_mode));
		} else {
			mutex_unlock(&vdd->vrr.brr_lock);
		}
	}

	mutex_lock(&vdd->panel_lpm.lpm_bl_lock);

	// need bl_lock..
	mutex_lock(&vdd->bl_lock);

	if (vdd->support_optical_fingerprint) {
		/* SAMSUNG_FINGERPRINT */
		/* From nomal at finger mask on state
		* 1. backup acl
		* 2. backup level if it is not the same value as vdd->br_info.common_br.bl_level
		*   note. we don't need to backup this case because vdd->br_info.common_br.bl_level is a finger_mask_bl_level now
		*/

		if ((vdd->br_info.common_br.finger_mask_hbm_on) && (backlight_origin == BACKLIGHT_NORMAL)) {/* finger mask hbm on & bl update from normal */
			backup_acl = vdd->br_info.acl_status;

			if (level != USE_CURRENT_BL_LEVEL) {
				backup_bl_level = level;

				LCD_INFO("[FINGER_MASK]BACKLIGHT_NORMAL save backup_acl = %d, backup_level = %d, vdd->br_info.common_br.bl_level=%d\n",
					backup_acl, backup_bl_level, vdd->br_info.common_br.bl_level);
				goto skip_bl_update;
			}
		}

		/* From BACKLIGHT_FINGERMASK_ON
		*   1. use finger_mask_bl_level(HBM) & acl 0
		*   2. backup previous bl_level & acl value
		*  From BACKLIGHT_FINGERMASK_OFF
		*   1. restore backup bl_level & acl value
		*/

		if (backlight_origin == BACKLIGHT_FINGERMASK_ON) {
			vdd->br_info.common_br.finger_mask_hbm_on = true;
			ss_wait_for_te_gpio(vdd, 1, vdd->panel_hbm_entry_after_te);
			backup_acl = vdd->br_info.acl_status;
			if (vdd->finger_mask_updated) /* do not backup br.bl_level at on to on */
				backup_bl_level = vdd->br_info.common_br.bl_level;
			level = vdd->br_info.common_br.finger_mask_bl_level;
			vdd->br_info.acl_status = 0;

			LCD_INFO("[FINGER_MASK]BACKLIGHT_FINGERMASK_ON turn on finger hbm & back up acl = %d, level = %d\n",
				backup_acl, backup_bl_level);
		}
		else if(backlight_origin == BACKLIGHT_FINGERMASK_OFF) {
			vdd->br_info.common_br.finger_mask_hbm_on = false;
			ss_wait_for_te_gpio(vdd, 1, vdd->panel_hbm_entry_after_te);
			vdd->br_info.acl_status = backup_acl;
			level = backup_bl_level;

			LCD_INFO("[FINGER_MASK]BACKLIGHT_FINGERMASK_OFF turn off finger hbm & restore acl = %d, level = %d\n",
				vdd->br_info.acl_status, level);
		}
	}

	if (level != USE_CURRENT_BL_LEVEL)
		vdd->br_info.common_br.bl_level = level;

	/* check the lcd id for DISPLAY_1 or DISPLAY_2 */
	if (!ss_panel_attached(vdd->ndx))
		goto skip_bl_update;

	if (vdd->grayspot) {
		LCD_ERR("grayspot on.. %d \n", vdd->grayspot);
		goto skip_bl_update;
	}

	if (vdd->br_info.flash_gamma_support &&
		!vdd->br_info.flash_gamma_init_done) {
		LCD_ERR("flash_gamme not ready\n");
		goto skip_bl_update;
	}

	if (vdd->brightdot_state) {
		/* During brightdot test, prevent whole brightntess setting,
		 * which changes brightdot setting.
		 * BIT0: brightdot test, BIT1: brightdot test in LFD 0.5hz
		 * allow brightness update in both brightdot test off case
		 */
		LCD_INFO("brightdot test on(%d), skip bl update\n", vdd->brightdot_state);
		goto skip_bl_update;
	}

	if (vdd->dtsi_data.panel_lpm_enable && is_new_lpm_version(vdd)) {
		set_lpm_br_values(vdd);

		if (ss_is_panel_lpm(vdd)) {
			LCD_ERR("[Panel LPM]: set brightness.(%d)->(%d)\n", vdd->br_info.common_br.bl_level, vdd->panel_lpm.lpm_bl_level);

			if (vdd->panel_func.samsung_set_lpm_brightness)
				vdd->panel_func.samsung_set_lpm_brightness(vdd);
			goto skip_bl_update;
		}
	}

	if (!is_new_lpm_version(vdd) && ss_is_panel_lpm(vdd)) {
		LCD_ERR("error: AOD service didn't set proper LPM mode " \
			"before enter LPM. bl: %d, lpm_bl: %d, lpm_mode: %d, ver: %d\n",
				vdd->br_info.common_br.bl_level,
				vdd->panel_lpm.lpm_bl_level,
				vdd->panel_lpm.mode,
				vdd->panel_lpm.ver);

		LCD_ERR("ver: %d, skip brightness setting to prevent false ESD detection\n", vdd->panel_lpm.ver);
		goto skip_bl_update;
	}

	if (vdd->dtsi_data.hmt_enabled && vdd->br_info.hmt_stat.hmt_on) {
		LCD_ERR("HMT is on. do not set normal brightness..(%d)\n", level);
		goto skip_bl_update;
	}

	if (ss_is_seamless_mode(vdd)) {
		LCD_ERR("splash is not done..\n");
		goto skip_bl_update;
	}

	if (is_hbm_level(vdd) && !vdd->dtsi_data.tft_common_support) {
		cmd_cnt = ss_hbm_brightness_packet_set(vdd);
		cmd_cnt > 0 ? vdd->display_status_dsi.hbm_mode = true : false;
	} else {
		cmd_cnt = ss_normal_brightness_packet_set(vdd);
		cmd_cnt > 0 ? vdd->display_status_dsi.hbm_mode = false : false;
	}

	if (cmd_cnt) {
		/* setting tx cmds cmt */
		brightness_cmds = ss_get_cmds(vdd, TX_BRIGHT_CTRL);
		brightness_cmds->count = cmd_cnt;

		/* generate single tx packet */
		ret = ss_single_transmission_packet(brightness_cmds);

		/* sending tx cmds */
		if (ret) {
			if(vdd->need_brightness_lock) {
				int locked = 0;
				int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

				if(!mutex_trylock(&vdd->exclusive_tx.ex_tx_lock)) {
					locked = 1;
				}
				vdd->exclusive_tx.enable = 1;
				while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
					usleep_range(500, 500);

				vdd->exclusive_tx.permit_frame_update = 0;
				usleep_range(36000, 36000);
				ss_set_exclusive_tx_packet(vdd, TX_BRIGHT_CTRL, 1);

				ss_send_cmd(vdd, TX_BRIGHT_CTRL);

				ss_set_exclusive_tx_packet(vdd, TX_BRIGHT_CTRL, 0);
				vdd->exclusive_tx.enable = 0;
				wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);
				if(!locked)
					mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);
			} else {
				LCD_INFO("tx bl cmd +\n");
				ss_send_cmd(vdd, TX_BRIGHT_CTRL);
				LCD_INFO("tx bl cmd -\n");
			}

			if (!IS_ERR_OR_NULL(ss_get_cmds(vdd, TX_BLIC_DIMMING)->cmds)) {
				if (vdd->br_info.common_br.bl_level == 0)
					ss_get_cmds(vdd, TX_BLIC_DIMMING)->cmds->msg.tx_buf[1] = 0x24;
				else
					ss_get_cmds(vdd, TX_BLIC_DIMMING)->cmds->msg.tx_buf[1] = 0x2C;

				ss_send_cmd(vdd, TX_BLIC_DIMMING);
			}

			// copr sum after changing brightness to calculate brightness avg.
			if (vdd->copr.copr_on) {
				ss_set_copr_sum(vdd, COPR_CD_INDEX_0);
				ss_set_copr_sum(vdd, COPR_CD_INDEX_1);
			}
			/* send poc compensation */
			if (vdd->poc_driver.is_support)
				ss_poc_comp(vdd);

			LCD_INFO("[DISPLAY_%d] level : %d  candela : %dCD hbm : %d (%d) itp : %s\n",
				vdd->ndx,
				vdd->br_info.common_br.bl_level, vdd->br_info.common_br.cd_level, vdd->display_status_dsi.hbm_mode, vdd->br_info.common_br.auto_level,
				vdd->br_info.panel_br_info.itp_mode == 0 ? "TABLE" : "FLASH");

			ss_notify_queue_work(vdd, PANEL_EVENT_BL_CHANGED);
		} else
			LCD_INFO("single_transmission_fail error\n");
	} else
		LCD_INFO("level : %d skip\n", vdd->br_info.common_br.bl_level);

skip_bl_update:
	if (vdd->support_optical_fingerprint) {
	/* SAMSUNG_FINGERPRINT */
	/* hbm needs vdd->panel_hbm_entry_delay TE to be updated, where as normal needs no */
		if (backlight_origin == BACKLIGHT_FINGERMASK_ON) {
			ss_wait_for_te_gpio(vdd, vdd->panel_hbm_entry_delay, 200);
		}
		if ((backlight_origin >= BACKLIGHT_FINGERMASK_ON) && (vdd->finger_mask_updated)) {
			SDE_ERROR("finger_mask_updated/ sysfs_notify finger_mask_state = %d\n", vdd->finger_mask);
			sysfs_notify(&vdd->lcd_dev->dev.kobj, NULL, "actual_mask_brightness");
			vdd->finger_mask_updated = 0;
		}
		if (backlight_origin == BACKLIGHT_FINGERMASK_OFF) {
			ss_wait_for_te_gpio(vdd, vdd->panel_hbm_exit_delay, 200);
		}
	}
	mutex_unlock(&vdd->bl_lock);

	mutex_unlock(&vdd->panel_lpm.lpm_bl_lock);

	if (need_lpm_lock) mutex_unlock(&vdd->panel_lpm.lpm_lock);

	return 0;
}

// HMT brightness
static void set_hmt_br_values(struct samsung_display_driver_data *vdd)
{
	int from, end;
	int left, right, p = 0;
	struct candela_map_table *table;

	table = &vdd->br_info.candela_map_table[HMT][vdd->panel_revision];

	if (IS_ERR_OR_NULL(table->cd)) {
		LCD_ERR("No candela_map_table..\n");
		return;
	}

	LCD_DEBUG("table size (%d)\n", table->tab_size);

	if (vdd->br_info.hmt_stat.hmt_bl_level > table->max_lv)
		vdd->br_info.hmt_stat.hmt_bl_level = table->max_lv;

	left = 0;
	right = table->tab_size - 1;

	while (left <= right) {
		p = (left + right) / 2;
		from = table->from[p];
		end = table->end[p];
		LCD_DEBUG("[%d] from(%d) end(%d) / %d\n", p, from, end, vdd->br_info.hmt_stat.hmt_bl_level);

		if (vdd->br_info.hmt_stat.hmt_bl_level >= from && vdd->br_info.hmt_stat.hmt_bl_level <= end)
			break;
		if (vdd->br_info.hmt_stat.hmt_bl_level < from)
			right = p - 1;
		else
			left = p + 1;
	};

	// for elvess, vint etc.. which are using 74 steps.
	vdd->br_info.common_br.interpolation_cd = vdd->br_info.hmt_stat.candela_level_hmt = table->cd[p];
	vdd->br_info.hmt_stat.cmd_idx_hmt = table->idx[p];

	if (vdd->br_info.common_br.gamma_mode2_support) {
		vdd->br_info.common_br.gm2_wrdisbv = table->gm2_wrdisbv[p];
		LCD_INFO("[%d] cd_idx (%d) cd (%d) gm2_wrdisbv (%d)\n",
				p, vdd->br_info.hmt_stat.cmd_idx_hmt,
				vdd->br_info.hmt_stat.candela_level_hmt,
				vdd->br_info.common_br.gm2_wrdisbv);
	} else {
		LCD_INFO("cd_idx (%d) cd_level (%d) \n",
				vdd->br_info.hmt_stat.cmd_idx_hmt,
				vdd->br_info.hmt_stat.candela_level_hmt);
	}

	return;
}

int ss_hmt_brightness_packet_set(
		struct samsung_display_driver_data *vdd)
{
	int cmd_cnt = 0;
	int level_key = 0;
	struct dsi_panel_cmd_set *set;
	struct dsi_cmd_desc *packet = NULL;
	struct dsi_panel_cmd_set *tx_cmd = NULL;

	LCD_DEBUG("[DISPLAY_%d] ++\n", vdd->ndx);

	set_hmt_br_values(vdd);

	/* init packet */
	set = ss_get_cmds(vdd, TX_BRIGHT_CTRL);
	if (SS_IS_CMDS_NULL(set)) {
		LCD_ERR("No cmds for TX_BRIGHT_CTRL.. \n");
		return -EINVAL;
	}
	packet = set->cmds;

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_tot_level)) {
		level_key = vdd->panel_func.samsung_brightness_tot_level(vdd);
		update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
	}

	if (vdd->br_info.smart_dimming_hmt_loaded_dsi) {
		/* aid/aor B2 */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_aid_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_aid_hmt(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* elvss B5 */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_elvss_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_elvss_hmt(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* vint F4 */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vint_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_vint_hmt(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* gamma CA */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_gamma_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_gamma_hmt(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

		/* VRR */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vrr_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_vrr_hmt(vdd, &level_key);

			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}

	} else {
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_hmt(vdd, &level_key);
			update_packet_level_key_enable(vdd, packet, &cmd_cnt, level_key);
			ss_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
		}
	}

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_tot_level)) {
		level_key = vdd->panel_func.samsung_brightness_tot_level(vdd);
		update_packet_level_key_disable(vdd, packet, &cmd_cnt, level_key);
	}

	LCD_DEBUG("[DISPLAY_%d] --\n", vdd->ndx);

	return cmd_cnt;
}

int ss_brightness_dcs_hmt(struct samsung_display_driver_data *vdd,
		int level)
{
	struct dsi_panel_cmd_set *brightness_cmds = NULL;
	int cmd_cnt;
	int ret = 0;

	brightness_cmds = ss_get_cmds(vdd, TX_BRIGHT_CTRL);

	vdd->br_info.hmt_stat.hmt_bl_level = level;
	LCD_INFO("[HMT] hmt_bl_level(%d)\n", vdd->br_info.hmt_stat.hmt_bl_level);

	cmd_cnt = ss_hmt_brightness_packet_set(vdd);

	/* sending tx cmds */
	if (cmd_cnt) {
		/* setting tx cmds cmt */
		brightness_cmds->count = cmd_cnt;

		/* generate single tx packet */
		ret = ss_single_transmission_packet(brightness_cmds);

		if (ret) {
			ss_send_cmd(vdd, TX_BRIGHT_CTRL);

			LCD_INFO("idx(%d), cd_level(%d), hmt_bl_level(%d) itp : %s",
				vdd->br_info.hmt_stat.cmd_idx_hmt, vdd->br_info.hmt_stat.candela_level_hmt, vdd->br_info.hmt_stat.hmt_bl_level,
				vdd->br_info.panel_br_info.itp_mode == 0 ? "TABLE" : "FLASH");
		} else
			LCD_DEBUG("single_transmission_fail error\n");
	} else
		LCD_INFO("level : %d skip\n", vdd->br_info.common_br.bl_level);

	return cmd_cnt;
}

// TFT brightness

void ss_brightness_tft_pwm(struct samsung_display_driver_data *vdd, int level)
{
	if (vdd == NULL) {
		LCD_ERR("no PWM\n");
		return;
	}

	if (ss_is_panel_off(vdd))
		return;

	vdd->br_info.common_br.bl_level = level;

	if (vdd->panel_func.samsung_brightness_tft_pwm)
		vdd->panel_func.samsung_brightness_tft_pwm(vdd, level);
}

void ss_tft_autobrightness_cabc_update(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("\n");

	switch (vdd->br_info.common_br.auto_level) {
	case 0:
		ss_cabc_update(vdd);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		ss_send_cmd(vdd, TX_CABC_ON);
		break;
	case 5:
	case 6:
		ss_send_cmd(vdd, TX_CABC_OFF);
		break;
	}
}

void ss_read_mtp(struct samsung_display_driver_data *vdd, int addr, int len, int pos, u8 *buf)
{
	struct dsi_panel_cmd_set *rx_cmds;
	int wait_cnt = 1000; /* 1000 * 0.5ms = 500ms */

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return;
	}

	if (addr > 0xFF || pos > 0xFF || len > 0xFF) {
		LCD_ERR("unvalind para addr(%x) pos(%d) len(%d)\n", addr, pos, len);
		return;
	}

	rx_cmds = ss_get_cmds(vdd, RX_MTP_READ_SYSFS);
	if (SS_IS_CMDS_NULL(rx_cmds)) {
		LCD_ERR("No cmds for RX_MTP_READ_SYSFS.. \n");
		return;
	}

	rx_cmds->cmds[0].msg.tx_buf[0] =  addr;
	rx_cmds->cmds[0].msg.tx_buf[1] =  len;
	rx_cmds->cmds[0].msg.tx_buf[2] =  pos;

	rx_cmds->cmds[0].msg.rx_len =  len;
	rx_cmds->read_startoffset = pos;

	mutex_lock(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.permit_frame_update = 1;
	vdd->exclusive_tx.enable = 1;
	while (!list_empty(&vdd->cmd_lock.wait_list) && --wait_cnt)
		usleep_range(500, 500);

	ss_set_exclusive_tx_packet(vdd, RX_MTP_READ_SYSFS, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL0_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL0_KEY_DISABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL2_KEY_ENABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL2_KEY_DISABLE, 1);
	ss_set_exclusive_tx_packet(vdd, TX_REG_READ_POS, 1);

	ss_panel_data_read(vdd, RX_MTP_READ_SYSFS, buf, LEVEL0_KEY | LEVEL1_KEY | LEVEL2_KEY);

	ss_set_exclusive_tx_packet(vdd, RX_MTP_READ_SYSFS, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL0_KEY_ENABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL0_KEY_DISABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_ENABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL1_KEY_DISABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL2_KEY_ENABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_LEVEL2_KEY_DISABLE, 0);
	ss_set_exclusive_tx_packet(vdd, TX_REG_READ_POS, 0);

	vdd->exclusive_tx.permit_frame_update = 0;
	vdd->exclusive_tx.enable = 0;
	wake_up_all(&vdd->exclusive_tx.ex_tx_waitq);
	mutex_unlock(&vdd->exclusive_tx.ex_tx_lock);

	return;
}

void ss_write_mtp(struct samsung_display_driver_data *vdd, int len, u8 *buf)
{
	struct dsi_panel_cmd_set *tx_cmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR("Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return;
	}

	if (len > 0xFF) {
		LCD_ERR("unvalind para len(%d)\n", len);
		return;
	}

	tx_cmds = ss_get_cmds(vdd, TX_MTP_WRITE_SYSFS);
	if (SS_IS_CMDS_NULL(tx_cmds)) {
		LCD_ERR("No cmds for TX_MTP_WRITE_SYSFS.. \n");
		return;
	}

	tx_cmds->cmds[0].msg.tx_len = len;
	tx_cmds->cmds[0].msg.tx_buf = buf;

	ss_send_cmd(vdd, TX_LEVEL0_KEY_ENABLE);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_ENABLE);
	ss_send_cmd(vdd, TX_LEVEL2_KEY_ENABLE);

	ss_send_cmd(vdd, TX_MTP_WRITE_SYSFS);

	ss_send_cmd(vdd, TX_LEVEL2_KEY_DISABLE);
	ss_send_cmd(vdd, TX_LEVEL1_KEY_DISABLE);
	ss_send_cmd(vdd, TX_LEVEL0_KEY_DISABLE);

	return;
}

static BLOCKING_NOTIFIER_HEAD(panel_notifier_list);

/**
 *	ss_panel_notifier_register - register a client notifier
 *	@nb: notifier block to callback on events
 */
int ss_panel_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&panel_notifier_list, nb);
}
EXPORT_SYMBOL(ss_panel_notifier_register);

/**
 *	ss_panel_notifier_unregister - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int ss_panel_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&panel_notifier_list, nb);
}
EXPORT_SYMBOL(ss_panel_notifier_unregister);

/**
 * panel_notifier_call_chain - notify clients
 *
 */
int ss_panel_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&panel_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(ss_panel_notifier_call_chain);

static void ss_bl_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct panel_bl_event_data bl_evt_data;

	vdd = container_of(work, struct samsung_display_driver_data, bl_event_work);

	bl_evt_data.bl_level = vdd->br_info.common_br.bl_level;
	bl_evt_data.aor_data = vdd->br_info.common_br.aor_data;
	bl_evt_data.display_idx = vdd->ndx;

	LCD_DEBUG("%d %d ++\n", bl_evt_data.bl_level, bl_evt_data.aor_data);
	ss_panel_notifier_call_chain(PANEL_EVENT_BL_CHANGED, &bl_evt_data);
	LCD_DEBUG("--\n");

	return;
}

static void ss_vrr_event_work(struct work_struct *work)
{
        struct samsung_display_driver_data *vdd =
		container_of(work, struct samsung_display_driver_data, vrr_event_work);
	struct vrr_info *vrr = NULL;
	struct panel_dms_data dms_data;
	char vrr_mode[16];

        if (IS_ERR_OR_NULL(vdd)) {
                LCD_ERR("no vdd");
                return;
        }
	vrr = &vdd->vrr;

	if (vrr->param_update) {
		snprintf(vrr_mode, sizeof(vrr_mode), "%dX%d:%s\n",
			vrr->adjusted_h_active, vrr->adjusted_v_active, vrr->adjusted_sot_hs_mode ? "HS" : "NOR");
		if(!sec_set_param(param_index_VrrStatus, &vrr_mode)) {
			LCD_ERR("Fail to set vrr_mode param\n");
			return;
		}
		LCD_DEBUG(" fps(%d) / vrr_mode = %s\n", vrr->adjusted_refresh_rate, vrr_mode);
	}

	dms_data.fps = vrr->adjusted_refresh_rate;
	dms_data.display_idx = vdd->ndx;

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_lfd_get_base_val)) {
		dms_data.lfd_min_freq = DIV_ROUND_UP(vrr->lfd.base_rr, vrr->lfd.min_div);
		dms_data.lfd_max_freq = DIV_ROUND_UP(vrr->lfd.base_rr, vrr->lfd.max_div);
	} else {
		dms_data.lfd_min_freq = dms_data.fps;
		dms_data.lfd_max_freq = dms_data.fps;
	}

	LCD_INFO("[DISPLAY_%d] fps=%d, lfd_min=%dhz(%d), lfd_max=%dhz(%d), base_rr=%dhz ++\n",
			dms_data.display_idx,
			dms_data.fps,
			dms_data.lfd_min_freq, vrr->lfd.min_div,
			dms_data.lfd_max_freq, vrr->lfd.max_div,
			vrr->lfd.base_rr);

	ss_panel_notifier_call_chain(PANEL_EVENT_VRR_CHANGED, &dms_data);
	LCD_INFO("--\n");

	return;
}

static void ss_lfd_event_work(struct work_struct *work)
{
        struct samsung_display_driver_data *vdd =
		container_of(work, struct samsung_display_driver_data, lfd_event_work);
	struct vrr_info *vrr;
	struct panel_dms_data dms_data;

        if (IS_ERR_OR_NULL(vdd)) {
                LCD_ERR("no vdd");
                return;
        }
	vrr = &vdd->vrr;

	dms_data.fps = vrr->adjusted_refresh_rate;
	dms_data.lfd_min_freq = DIV_ROUND_UP(vrr->lfd.base_rr, vrr->lfd.min_div);
	dms_data.lfd_max_freq = DIV_ROUND_UP(vrr->lfd.base_rr, vrr->lfd.max_div);
	dms_data.display_idx = vdd->ndx;

	LCD_INFO("fps=%d, lfd_min=%d, lfd_max=%d ++\n", dms_data.fps,
			dms_data.lfd_min_freq, dms_data.lfd_max_freq);
	ss_panel_notifier_call_chain(PANEL_EVENT_LFD_CHANGED, &dms_data);
	LCD_INFO("--\n");

	return;
}


static void ss_panel_state_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct panel_state_data state_data;

	vdd = container_of(work, struct samsung_display_driver_data, panel_state_event_work);

	switch(vdd->panel_state) {
		case PANEL_PWR_ON:
			state_data.state = PANEL_ON;
			break;
		case PANEL_PWR_OFF:
			state_data.state = PANEL_OFF;
			break;
		case PANEL_PWR_LPM:
			state_data.state = PANEL_LPM;
			break;
		default:
			state_data.state = MAX_PANEL_STATE;
			break;
	}

	LCD_DEBUG("%d ++\n", state_data.state);
	ss_panel_notifier_call_chain(PANEL_EVENT_STATE_CHANGED, &state_data);
	LCD_DEBUG("--\n");

	return;
}

void ss_notify_queue_work(struct samsung_display_driver_data *vdd,
	enum panel_notifier_event_t event)
{
	mutex_lock(&vdd->notify_lock);

	LCD_DEBUG("[DISPLAY_%d] ++ %d\n", vdd->ndx, event);

	switch (event) {
		case PANEL_EVENT_BL_CHANGED:
			/* notify clients of brightness change */
			queue_work(vdd->notify_wq, &vdd->bl_event_work);
			break;
		case PANEL_EVENT_VRR_CHANGED:
			queue_work(vdd->notify_wq, &vdd->vrr_event_work);
			break;
		case PANEL_EVENT_LFD_CHANGED:
			queue_work(vdd->notify_wq, &vdd->lfd_event_work);
			break;
		case PANEL_EVENT_STATE_CHANGED:
			queue_work(vdd->notify_wq, &vdd->panel_state_event_work);
			break;
		default:
			LCD_ERR("unkown panel notify event %d\n", event);
			break;
	}

	LCD_DEBUG("[DISPLAY_%d] -- %d\n", vdd->ndx, event);

	mutex_unlock(&vdd->notify_lock);

	return;
}

bool ss_is_brr_on(enum SS_BRR_MODE brr_mode)
{
	bool is_brr_on = false;

	if (brr_mode < MAX_BRR_ON_MODE)
		is_brr_on = true;

	return is_brr_on;
}

static enum SS_BRR_MODE ss_get_brr_mode(struct samsung_display_driver_data *vdd)
{
	struct vrr_info *vrr = &vdd->vrr;
	int cur_rr = vrr->cur_refresh_rate;
	bool cur_hs = vrr->cur_sot_hs_mode;
	int new_rr = vrr->adjusted_refresh_rate;
	bool new_hs = vrr->adjusted_sot_hs_mode;
	int candela = vdd->br_info.common_br.cd_level;
	enum SS_BRR_MODE brr_mode = BRR_OFF_MODE;

	if (!vdd->vrr.is_support_brr)
		return BRR_OFF_MODE;

	if (cur_hs != new_hs)
		return BRR_OFF_MODE;

	/* set BRR mode */
	if (cur_rr == 48 && new_rr == 60)
		brr_mode = BRR_48_60;
	else if (cur_rr == 60 &&  new_rr == 48)
		brr_mode = BRR_60_48;
	else if (cur_rr == 96 && new_rr == 120)
		brr_mode = BRR_96_120;
	else if (cur_rr == 120 && new_rr == 96)
		brr_mode = BRR_120_96;
	else if (cur_rr == 60 && new_rr == 120)
		brr_mode = BRR_60HS_120;
	else if (cur_rr == 120 && new_rr == 60)
		brr_mode = BRR_120_60HS;
	else if (cur_rr == 60 && new_rr == 96)
		brr_mode = BRR_60HS_96;
	else if (cur_rr == 96 && new_rr == 60)
		brr_mode = BRR_96_60HS;
	else
		brr_mode = BRR_OFF_MODE;

	/* Out of allowed brightness level case, turn off BRR mode */
	if (brr_mode < MAX_BRR_ON_MODE) {
		struct vrr_bridge_rr_tbl *brr = &vrr->brr_tbl[brr_mode];
		if (candela < brr->min_cd  || candela > brr->max_cd) {
			LCD_INFO("out of BRR brightness range(%d~%d): cur: %d\n",
					brr->min_cd, brr->max_cd, candela);
			brr_mode = BRR_OFF_MODE;
		}
	}

	return brr_mode;
}

/* delayed gamma update after sleep out */
/* sleep out -> 20ms -> FCh and etc... -> 130ms -> gamma update (F7h) */
#define DELAY_TIME_SLEEPOUT_GAMMA_UPDATE	150

/* delayed work doesn't have precise timer. trigger work early, and wait 150ms after sleep out */
#define DELAYED_WORK_GAMMA_UPDATE_TIME_MS	100

#define SDE_PERF_MODE_NORMAL	(0)	/* refer to enum sde_perf_mode */

void ss_panel_vrr_switch(struct vrr_info *vrr)
{
	struct samsung_display_driver_data *vdd =
		container_of(vrr, struct samsung_display_driver_data, vrr);
	struct drm_device *ddev = GET_DRM_DEV(vdd);
	struct vrr_bridge_rr_tbl *brr;
	int step;
	int direction;
	int interval_ms;
	s64 wait_ms_wqhd_hs;

	int cur_rr = vrr->cur_refresh_rate;
	bool cur_hs = vrr->cur_sot_hs_mode;
	int adjusted_rr = vrr->adjusted_refresh_rate;
	bool adjusted_hs = vrr->adjusted_sot_hs_mode;

	int fps_min = (cur_rr < adjusted_rr) ? cur_rr : adjusted_rr;
	s64 DELAY_TIME_WQHD_HS_VRR  = (1000 / fps_min) + 2;
	bool is_hs_change = !!(cur_hs != adjusted_hs);
	char trace_name[128];
	int ret;

	mutex_lock(&vrr->vrr_lock);
	vrr->running_vrr = true;
	vrr->running_vrr_mdp = false;

	snprintf(trace_name, 128, "VRR: %d->%d", cur_rr, adjusted_rr);
	SDE_ATRACE_BEGIN(trace_name);
	LCD_INFO("[DISPLAY_%d] +++ VRR: %d%s -> %d%s\n", vdd->ndx,
			cur_rr, cur_hs ? "HS" : "NM",
			adjusted_rr, adjusted_hs ?  "HS" : "NM");

	if (vrr->hs_nm_seq == HS_NM_MULTI_RES_FIRST) {
		// TODO: check: if new dms ioctl change this nm_seq?? add lock?
		// This rarely occurs, so doesn't require lock..
		vrr->hs_nm_seq = HS_NM_OFF;

		wait_ms_wqhd_hs = DELAY_TIME_WQHD_HS_VRR - ktime_ms_delta(ktime_get(), vrr->time_vrr_start);
		LCD_INFO("VRR: wait_ms_wqhd_hs = %lld ms\n", wait_ms_wqhd_hs);
		while (wait_ms_wqhd_hs > 0) {
			LCD_INFO("VRR: WQHD_HS wait for %lld ms\n", wait_ms_wqhd_hs);
			usleep_range(wait_ms_wqhd_hs * 1000, wait_ms_wqhd_hs * 1000);
			wait_ms_wqhd_hs = DELAY_TIME_WQHD_HS_VRR -
						ktime_ms_delta(ktime_get(), vdd->sleep_out_time);
		}

	}

	vrr->skip_vrr_in_brightness = false;

	/* compenstate gamma MTP value for 48/96hz modes in gamma mode2 panel */
	if (vdd->panel_func.samsung_brightness_gm2_gamma_comp) {
		if ((cur_rr != 48 && cur_rr != 96) &&
				(adjusted_rr == 48 || adjusted_rr == 96)) {
			/* 60/120hz mode -> 48/96hz mode: apply compensated gamma */
			vrr->gm2_gamma = VRR_GM2_GAMMA_COMPENSATE;
			LCD_INFO("compensate gamma\n");
		} else if ((cur_rr == 48 || cur_rr == 96) &&
				(adjusted_rr != 48 && adjusted_rr != 96)) {
			/* 48/96hz mode -> 60/120hz mode: restore original gamma MTP */
			vrr->gm2_gamma = VRR_GM2_GAMMA_RESTORE_ORG;
			LCD_INFO("restore org gamma\n");
		}
	}

	/* In OFF/LPM mode, skip VRR change, and just save VRR values.
	 * After normal display on, it will be applied in brightness setting.
	 */
	if (ss_is_panel_off(vdd) || ss_is_panel_lpm(vdd)) {
		LCD_ERR("error: panel_state(%d), skip VRR (save VRR state)\n",
				vdd->panel_state);
		SS_XLOG(0xbad, vdd->panel_state);
		vrr->cur_refresh_rate = adjusted_rr;
		vrr->cur_sot_hs_mode = adjusted_hs;
		goto brr_done;
	}

	mutex_lock(&vrr->brr_lock);
	vrr->brr_mode = ss_get_brr_mode(vdd);
	mutex_unlock(&vrr->brr_lock);

	if (!ss_is_brr_on(vrr->brr_mode)) {
		LCD_INFO("VRR: BRR off (%s)\n", ss_get_brr_mode_name(vrr->brr_mode));

		/* HS <-> Normal mode: insert one black frame to hide screen noise
		 * ss_gamma() set gamma MTP (C8h) to zero.
		 * None zero gamma (CAh) setting causes greenish screen issue during MTP(C8h) is zero.
		 * So, gamma value should be set to zero while black_frame_mode is BLACK_FRAME_INSERT mode.
		 */
		/* TODO:
		 * - pending frame update using exclusive lock..? -> No
		 * - this code is called in display_lock. so frame update and brightness setting
		 *   would be blocked..
		*/
		if (vrr->is_support_black_frame && is_hs_change) {
			LCD_INFO("VRR: set BLACK_FRAME_INSERT\n");
			vrr->black_frame_mode = BLACK_FRAME_INSERT;
		}

		/* In VRR, TE period is configured by cmd included in DSI_CMD_SET_TIMING_SWITCH.
		 * To avoid brightness flicker, TE period and proper brightness setting
		 * should be applied at once.
		 * TE period setting cmd will be applied by gamma update cmd (F7h).
		 * So, update brightness here, to send gamma update (F7h) and apply
		 * new brightness and new TE period which is configured in above DSI_CMD_SET_TIMING_SWITCH.
		 */
		vrr->cur_refresh_rate = adjusted_rr;
		vrr->cur_sot_hs_mode = adjusted_hs;

		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

		/* HS <-> Normal mode: remove one black frame and restore to original brightness */
		if (vrr->black_frame_mode == BLACK_FRAME_INSERT) {
			/* Above brightness setting will be applied to DDI
			 * in next vsync (previous refresh rate).
			 * So, wait for interval based on previous fps.
			 */
			int interval_us = (1000000 / cur_rr) + 1000; /* 1ms dummy */
			usleep_range(interval_us, interval_us);

			/* ss_gamma() will restore gamma (CAh) and MTP (C8h) */
			LCD_INFO("VRR: waited %d us, set BLACK_FRAME_REMOVE\n", interval_us);
			vrr->black_frame_mode = BLACK_FRAME_REMOVE;
			ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
		}

		goto brr_done;
	}

	LCD_INFO("VRR: BRR on (%s)\n", ss_get_brr_mode_name(vrr->brr_mode));

	brr = &vrr->brr_tbl[vrr->brr_mode];

	step = 0;
	direction = 1;
	while (step >= 0 && step < brr->tot_steps) {
		int fps_brr = brr->fps[step];

		LCD_INFO("VRR: [%d] fps: %d, delay %d frames\n", step, fps_brr, brr->delay_frame_cnt);

		/* in ss_gamma, and ss_aor, calcaulate gamma or aor interpolation...
		 * In BRR mode, SOT_HS mode is not changed.
		 */
		vrr->cur_refresh_rate = fps_brr;
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

		if (brr->delay_frame_cnt <= 0) {
			LCD_ERR("VRR: delay_frame_cnt: %d, set default value 4\n", brr->delay_frame_cnt);
			brr->delay_frame_cnt = 4;
		}

		interval_ms = (1000 / fps_brr) * brr->delay_frame_cnt + 1; /* add 1ms dummy delay */
		while (interval_ms--) {
			/* check vrr change, and stop bridge RR, return.
			 * New VRR setting will change RR.
			 * - BRR_STOP_MODE :  case of new brightness level change. Jump to final adjusted RR.
			 *   Here, just stop BRR and update current RR to final adjusted RR.
			 *   New brightness setting will chagne to final adjusted RR.
			 */
			mutex_lock(&vrr->brr_lock);
			if (vrr->brr_mode == BRR_STOP_MODE) {
				vrr->brr_mode = BRR_OFF_MODE; // BRR is done, set to off mode
				mutex_unlock(&vrr->brr_lock);

				vrr->cur_refresh_rate = adjusted_rr;
				vrr->cur_sot_hs_mode = adjusted_hs;

				LCD_INFO("VRR: stop BRR (bl: %d)\n", vrr->brr_bl_level);

				/* update current VRR mode before restore
				 * SDE core clock to prevent screen noise
				 */
				ss_brightness_dcs(vdd, vrr->brr_bl_level, BACKLIGHT_NORMAL);

				goto brr_done;
			}
			mutex_unlock(&vrr->brr_lock);

			usleep_range(1000, 1000);
		}

		/* BRR_REWIND_MODE */
		mutex_lock(&vrr->brr_lock);
		if (vrr->brr_rewind_on) {
			vrr->brr_rewind_on = false;
			direction *= -1;

			LCD_INFO("VRR: rewind BRR, direction: %d, adjusted_rr: %d -> %d\n",
					direction, adjusted_rr, vrr->adjusted_refresh_rate);

			/* In rewind mode, its final adjusted RR is changed by new VRR */
			adjusted_rr = vrr->adjusted_refresh_rate;
		}
		mutex_unlock(&vrr->brr_lock);

		step += direction;
	}

	/* brr_mode is used in ss_gamma or ss_aid. So turn off brr_mode */
	mutex_lock(&vrr->brr_lock);
	vrr->brr_mode = BRR_OFF_MODE; // BRR is done, set to off mode
	mutex_unlock(&vrr->brr_lock);

	/* In VRR, TE period is configured by cmd included in brightness setting */
	vrr->cur_refresh_rate = adjusted_rr;
	vrr->cur_sot_hs_mode = adjusted_hs;

	ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

brr_done:

	/* TE modulation */
	if (vrr->support_te_mod && vdd->panel_func.samsung_vrr_set_te_mod) {
		vdd->panel_func.samsung_vrr_set_te_mod(vdd,
						vrr->cur_refresh_rate,
						vrr->cur_sot_hs_mode);
		SS_XLOG(vrr->te_mod_on, vrr->te_mod_divider, vrr->te_mod_cnt);
	}

	/* Restore to normal sde core clock boosted up to max mdp clock during VRR.
	 * In below case, current VRR mode and adjusting target VRR mode are different,
	 * and should keep maximum SDE core clock to prevent screen noise.
	 *
	 * 1) DISP thread#1: get 60HS -> 120HS VRR change request.
	 *                   Request SDE core clock to 200Mhz.
	 * 2) VRR thread#2:  start 60HS -> 120HS BRR which takes hundreds miliseconds.
	 *                   Set maximum SDE core clock (460Mhz)
	 * 3) DISP thread#1: get 120HS -> 60HS VRR change request.
	 *                   Request SDE core clock to 150Mhz.
	 * 4) VRR thread#2:  finish 60HS -> 120HS BRR.
	 *                   Restore SDE core clock to 150Mhz for 60HS while panel works at 120hz..
	 * 		     It causes screen noise...
	 *                   In this case, current VRR mode and adjusting target VRR mode are different!
	 * 5) VRR thread#2:  start 120HS -> 60HS BRR. Set maximum SDE core clock, and fix screen noise.
	 *                   After hundreds miliseconds, it finishes BRR,
	 *                   and restore SDE core clock to 150Mhz while panel works at 60hz.
	 *                   No more screen noise.
	 */
	if (vrr->cur_refresh_rate == vrr->adjusted_refresh_rate &&
			vrr->cur_sot_hs_mode == vrr->adjusted_sot_hs_mode) {
		/* SDE core clock will be applied to system by calling ss_set_normal_sde_core_clk().
		 * But, panel's new refresh rate will be applied in next TE after sending VRR commands.
		 * So, delay one frame before restore SDE core clock.
		 */
		int min_rr = min(cur_rr, adjusted_rr);
		int interval_us = (1000000 / min_rr) + 1000; /* 1ms dummy */

		LCD_INFO("delay 1frame(min_rr: %d, %dus)\n", min_rr, interval_us);
		usleep_range(interval_us, interval_us);

		if (!vrr->running_vrr_mdp) {
			ret = ss_set_normal_sde_core_clk(ddev);
			if (ret) {
				LCD_ERR("fail to set normal sde core clock..(%d)\n", ret);
				SS_XLOG(ret, 0xbad2);
			}
		} else {
			LCD_INFO("Do not set normal clk, vrr is ongoing!! (%d)\n", vrr->running_vrr_mdp);
		}
	} else {
		LCD_INFO("consecutive VRR req, keep max sde clk (cur: %d%s, adj: %d%s)\n",
				vrr->cur_refresh_rate,
				vrr->cur_sot_hs_mode ? "HS" : "NS",
				vrr->adjusted_refresh_rate,
				vrr->adjusted_sot_hs_mode ? "HS" : "NS");
	}

	/* TODO: need atomic read?? for delayed_perf_normal??? -> No.
	 * There could be race condition with sysfs_sde_core_perf_mode_write(),
	 * But even though, it will set delayed_perf_normal to true,
	 * and reset the perf tune params finally.
	 */
	if (vrr->delayed_perf_normal) {
		struct sde_kms *sde_kms = GET_SDE_KMS(vdd);
		struct sde_core_perf *perf = &sde_kms->perf;;

		vrr->delayed_perf_normal = false;

		if (!perf) {
			LCD_ERR("error: fail to get perf.. skip to normal perf mode..\n");
			goto vrr_end;
		}

		/* reset the perf tune params to 0 */
		perf->perf_tune.min_core_clk = 0;
		perf->perf_tune.min_bus_vote = 0;

		LCD_INFO("VRR: delayed normal performance mode in BRR end (%d -> NORMAL)\n",
				perf->perf_tune.mode);
		perf->perf_tune.mode = SDE_PERF_MODE_NORMAL;
	}

vrr_end:

	vrr->running_vrr = false;

	LCD_INFO("[DISPLAY_%d] --- VRR\n", vdd->ndx);
	SDE_ATRACE_END(trace_name);

	mutex_unlock(&vrr->vrr_lock);

	ss_notify_queue_work(vdd, PANEL_EVENT_LFD_CHANGED);
}

void ss_panel_vrr_switch_work(struct work_struct *work)
{
	struct vrr_info *vrr = container_of(work, struct vrr_info, vrr_work);

	ss_panel_vrr_switch(vrr);
}

int ss_panel_dms_switch(struct samsung_display_driver_data *vdd)
{
	struct vrr_info *vrr = &vdd->vrr;

	int cur_hact = vrr->cur_h_active;
	int cur_vact = vrr->cur_v_active;
	bool cur_hs = vrr->cur_sot_hs_mode;
	int adjusted_hact = vrr->adjusted_h_active;
	int adjusted_vact = vrr->adjusted_v_active;
	bool adjusted_hs = vrr->adjusted_sot_hs_mode;

	bool is_hs_change = !!(cur_hs != adjusted_hs);

	LCD_INFO("[DISPLAY_%d] +++ DMS: %dx%d@%d%s -> %dx%d@%d%s\n", vdd->ndx,
			cur_hact, cur_vact, vrr->cur_refresh_rate,
			cur_hs ? "HS": "NM",
			adjusted_hact, adjusted_vact, vrr->adjusted_refresh_rate,
			adjusted_hs ? "HS": "NM");

	/* Do we have to change param only when the HS<->NORMAL be changed? */
	ss_set_vrr_switch(vdd, true);

	vrr->time_vrr_start = ktime_get();
	if (vrr->max_h_active_support_120hs &&
			vrr->is_multi_resolution_changing &&
			vrr->is_vrr_changing &&
			is_hs_change) {
		/* WQHD@Normal -> FHD/HD@HS: multi resolution -> one image frame -> VRR */
		/* TODO: replace this with check code if it supports HS mode or not.... */
		if ((vrr->cur_h_active > vrr->max_h_active_support_120hs) &&
				(adjusted_hs)) {
			LCD_INFO("WQHD(%d>%d)@NM -> FHD/HD@HS: multi resolution -> one frame -> VRR\n",
					vrr->cur_h_active, vrr->max_h_active_support_120hs);
			vrr->hs_nm_seq = HS_NM_MULTI_RES_FIRST;
		}

		/* FHD/HD@HS -> WQHD@Normal: VRR -> multi resolution */
		if ((vrr->adjusted_h_active > vrr->max_h_active_support_120hs) &&
				(cur_hs)) {
			LCD_INFO("FHD/HD@HS -> WQHD(%d>%d)@NM: VRR -> multi resolution\n",
					vrr->cur_h_active, vrr->max_h_active_support_120hs);
			vrr->hs_nm_seq = HS_NM_VRR_FIRST;
		}
	}

	/* case of HS_NM_VRR_FIRST: change multi resolution after VRR setting */
	if (vrr->is_multi_resolution_changing) {
		if (vrr->hs_nm_seq != HS_NM_VRR_FIRST) {
			LCD_INFO("multi resolution: tx DMS switch cmd\n");
			ss_send_cmd(vdd, DSI_CMD_SET_TIMING_SWITCH);
		}
		vrr->is_multi_resolution_changing = false;
	}

	if (vrr->is_vrr_changing) {
		mutex_lock(&vrr->brr_lock);
		if (ss_is_brr_on(vrr->brr_mode) &&
				vrr->adjusted_refresh_rate == vrr->brr_tbl[vrr->brr_mode].fps_start &&
				vrr->adjusted_sot_hs_mode == vrr->brr_tbl[vrr->brr_mode].sot_hs_base) {
			vrr->brr_rewind_on = true;

			LCD_INFO("VRR: new vrr (%dhz%s), set brr_rewind_on\n",
					vrr->adjusted_refresh_rate,
					vrr->adjusted_sot_hs_mode ? "HS" : "NM" );
		}
		mutex_unlock(&vrr->brr_lock);

		if (vrr->hs_nm_seq == HS_NM_VRR_FIRST) {
			LCD_INFO("VRR: start VRR switch to pend frame update until change resolution\n");
			ss_panel_vrr_switch(vrr);
			LCD_INFO("VRR: multi resolution: tx DMS switch cmd\n");
			ss_send_cmd(vdd, DSI_CMD_SET_TIMING_SWITCH);
			vrr->hs_nm_seq = HS_NM_OFF;
		} else {
			enum SS_BRR_MODE brr_mode = ss_get_brr_mode(vdd);

			/* skip VRR setting in brightness command (ss_vrr() funciton)
			 * 1) start waiting HS_NM change in vrr_work thread
			 * 2) triger brightness setting in other thread, and change fps
			 *    before finishing HS_NM waiting...
			 *    In result, it causes side effect, like screen noise or stuck..
			 * 3) finish waiting HS_NM change...
			 */
			if (vrr->hs_nm_seq == HS_NM_MULTI_RES_FIRST) {
				LCD_INFO("VRR: HS_NM transition, set skip_vrr_in_brightness true\n");
				vrr->skip_vrr_in_brightness = true;
			}

			LCD_INFO("VRR: %s, always enQ VRR work\n", ss_get_brr_mode_name(brr_mode));
			queue_work(vrr->vrr_workqueue, &vrr->vrr_work);
		}
		vrr->is_vrr_changing = false;
	}

	vrr->cur_h_active = vrr->adjusted_h_active;
	vrr->cur_v_active = vrr->adjusted_v_active;

	LCD_INFO("[DISPLAY_%d] --- DMS: HS: %d -> %d, change: %d\n", vdd->ndx, cur_hs, adjusted_hs, is_hs_change);

	return 0;
}

void ss_set_panel_state(struct samsung_display_driver_data *vdd, enum ss_panel_pwr_state panel_state)
{
	vdd->panel_state = panel_state;


	LCD_ERR("set panel state %d\n", vdd->panel_state);

	ss_notify_queue_work(vdd, PANEL_EVENT_STATE_CHANGED);
}

/* bring start time of gamma flash forward: first frame update -> display driver probe.
 * As of now, gamma flash starts when bootanimation kickoff frame update.
 *
 * In that case, it causes below problem.
 * 1) bootanimation turns on main and sub display.
 * 2) bootanimation update one black frame to both displays.
 * 3) bootanimation turns off sub dipslay, in case of folder open.
 *    But, sub display gamma flash takes several seconds (about 3 sec in user binary),
 *    so, it bootanimation waits for it, and both displays shows black screen for a while.
 * 4) After sub display gamma flash is done and finish to turn off sub display,
 *    bootanimation starts to draw animation on main display, and it is too late then expected..
 *
 * To avoid above issue, starts gamma flash earlyer, at display driver probe timing.
 * For this, to do belows at driver probe timing.
 * 1) start gamma flash only in case of splash mode on. Splash on mode means that bootlaoder already turned on mipi phy.
 * 2) initialize mipi host, including enable mipi error interrupts, to avoid blocking mipi transmission.
 * 3) disable autorefresh, which was disabled in first frame update, to prevent dsi fifo underrun error.
 */
int ss_early_display_init(struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display;
	struct drm_encoder *drm_enc;
	int ret;

	if (!vdd) {
		LCD_ERR("error: vdd NULL\n");
		return -ENODEV;
	}

	if (vdd->br_info.support_early_gamma_flash &&
			vdd->br_info.flash_gamma_support &&
			!vdd->br_info.flash_gamma_init_done &&
			!work_busy(&vdd->br_info.flash_br_work.work)) {
		LCD_INFO("early gamma flash\n");
	} else if (vdd->support_early_id_read) {
		LCD_INFO("early module ID read\n");
	} else {
		LCD_DEBUG("no action\n");
		return 0;
	}

	LCD_INFO("ndx=%d: cur panel_state: %d\n", vdd->ndx, vdd->panel_state);

	display = GET_DSI_DISPLAY(vdd);

	if (!display || !display->bridge || !display->bridge->base.encoder) {
		LCD_ERR("fail to get valid drm_enc\n");
		return -ENODEV;
	}

	drm_enc = display->bridge->base.encoder;

	/* disable autorefresh to prevent DSI FIFO underflow */
	sde_encoder_prepare_commit(drm_enc);

	/* set lcd id for getting proper mapping table */
	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		vdd->manufacture_id_dsi = get_lcd_attached("GET");
	else if (vdd->ndx == SECONDARY_DISPLAY_NDX)
		vdd->manufacture_id_dsi = get_lcd_attached_secondary("GET");

	/* Panel revision selection */
	if (IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_revision))
		LCD_ERR("no panel_revision_selection_error function\n");
	else
		vdd->panel_func.samsung_panel_revision(vdd);

	/* in gamma flash work thread, it will initialize mipi host */
	/* In case of autorefresh_fail, do gamma flash job in ss_event_frame_update()
	 * which is called after RESET MDP to recover error status.
	 * TODO: get proper phys_enc and check if phys_enc->enable_state = SDE_ENC_ERR_NEEDS_HW_RESET...
	 */
	if (vdd->is_autorefresh_fail) {
		LCD_ERR("autorefresh failure error\n");
		return -EINVAL;
	}

	/* early gamma flash */
	if (vdd->br_info.support_early_gamma_flash) {
		LCD_INFO("enQ gamma flash work\n");
		queue_delayed_work(vdd->br_info.flash_br_workqueue, &vdd->br_info.flash_br_work, msecs_to_jiffies(0));
	}

	/* early module id read */
	if (vdd->support_early_id_read) {
		bool skip_host_deinit = true;
		bool skip_restore_panel_state_off = true;

		LCD_INFO("read module id\n");

		/* display ctrl and mipi host init should be done in display_lock */
		mutex_lock(&display->display_lock);
		LCD_INFO("[%s] init host, support_early_gamma_flash: %d\n",
				display->name, vdd->br_info.support_early_gamma_flash);
		ret = dsi_display_ctrl_init(display);
		if (ret) {
			LCD_INFO("[%s] ret=%d, host was already initialized.. skip deinit in final state\n",
					display->name, ret);
			skip_host_deinit = true;
			mutex_unlock(&display->display_lock);
		} else {
			skip_host_deinit = false;
		}

		/* set panel_state pwr on ready, to allow mipi transmission */
		if (vdd->panel_state == PANEL_PWR_OFF) {
			skip_restore_panel_state_off = false;
			vdd->panel_state = PANEL_PWR_ON_READY;
		}

		/* Module info */
		if (!vdd->module_info_loaded_dsi) {
			if (IS_ERR_OR_NULL(vdd->panel_func.samsung_module_info_read))
				LCD_ERR("no samsung_module_info_read function\n");
			else
				vdd->module_info_loaded_dsi = vdd->panel_func.samsung_module_info_read(vdd);
		}

		/* MDNIE X,Y */
		if (!vdd->mdnie_loaded_dsi) {
			if (IS_ERR_OR_NULL(vdd->panel_func.samsung_mdnie_read))
				LCD_ERR("no samsung_mdnie_read function\n");
			else
				vdd->mdnie_loaded_dsi = vdd->panel_func.samsung_mdnie_read(vdd);
		}

		/* Panel Unique Cell ID */
		if (!vdd->cell_id_loaded_dsi) {
			if (IS_ERR_OR_NULL(vdd->panel_func.samsung_cell_id_read))
				LCD_ERR("no samsung_cell_id_read function\n");
			else
				vdd->cell_id_loaded_dsi = vdd->panel_func.samsung_cell_id_read(vdd);
		}

		/* restore panel_state to poweroff
		 * to prevent other mipi transmission before gfx HAL enable display.
		 *
		 * disp_on_pre == true means panel_state is already overwritten by gfx HAL,
		 * so no more need to restore panel_state
		 */
		if (vdd->display_status_dsi.disp_on_pre)
			skip_restore_panel_state_off = true;
		if (!skip_restore_panel_state_off) {
			LCD_INFO("[%s] restore panel state to off\n", display->name);
			vdd->panel_state = PANEL_PWR_OFF;
		}

		if (!skip_host_deinit) {
			LCD_INFO("[%s] deinit host\n", display->name);
			(void)dsi_display_ctrl_deinit(display);
			mutex_unlock(&display->display_lock);
		}
	}

	return 0;
}

static int ss_gm2_ddi_flash_init(struct samsung_display_driver_data *vdd)
{
	struct flash_gm2 *gm2_table = &vdd->br_info.gm2_table;
	int ret;

	if (vdd->br_info.gm2_table.ddi_flash_raw_len == 0 ||
			vdd->br_info.gm2_table.len_one_vbias_mode == 0) {
		LCD_DEBUG("do not support gamma mode2 flash (%d, %d)\n",
				vdd->br_info.gm2_table.ddi_flash_raw_len,
				vdd->br_info.gm2_table.len_one_vbias_mode);
		return 0;
	}

	gm2_table->ddi_flash_raw_buf = NULL;
	gm2_table->vbias_data = NULL;
	gm2_table->mtp_one_vbias_mode = NULL;

	/* init gamma mode2 flash data */
	gm2_table->ddi_flash_raw_buf = kzalloc(gm2_table->ddi_flash_raw_len, GFP_KERNEL | GFP_DMA);
	if (!gm2_table->ddi_flash_raw_buf) {
		LCD_ERR("fail to alloc ddi_flash_raw_buf\n");
		return -ENOMEM;
	}

	gm2_table->vbias_data = kzalloc((gm2_table->len_one_vbias_mode * VBIAS_MODE_MAX),
						GFP_KERNEL | GFP_DMA);
	if (!gm2_table->vbias_data) {
		LCD_ERR("fail to alloc vbias_data\n");
		ret = -ENOMEM;
		goto err;
	}
	gm2_table->mtp_one_vbias_mode = kzalloc(gm2_table->len_one_vbias_mode,
							GFP_KERNEL | GFP_DMA);
	if (!gm2_table->mtp_one_vbias_mode) {
		LCD_ERR("fail to alloc mtp_one_vbias_mode\n");
		ret = -ENOMEM;
		goto err;
	}

	LCD_INFO("init flash: OK\n");

	return 0;

err:
	LCD_ERR("fail to init vbias flash.. free memory..(ret: %d)\n", ret);
	kfree(gm2_table->mtp_one_vbias_mode);
	kfree(gm2_table->vbias_data );
	kfree(gm2_table->ddi_flash_raw_buf);
	return ret;
}


void ss_panel_init(struct dsi_panel *panel)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_name[MAX_CMDLINE_PARAM_LEN];
	char panel_secondary_name[MAX_CMDLINE_PARAM_LEN];

	/* compare panel name in command line and dsi_panel.
	 * primary panel: ndx = 0
	 * secondary panel: ndx = 1
	 */

	ss_get_primary_panel_name_cmdline(panel_name);
	ss_get_secondary_panel_name_cmdline(panel_secondary_name);

	if (!strcmp(panel->name, panel_name)) {
		ndx = PRIMARY_DISPLAY_NDX;
	} else if (!strcmp(panel->name, panel_secondary_name)) {
		ndx = SECONDARY_DISPLAY_NDX;
	} else {
		/* If it fails to find panel name, it cannot turn on display,
		 * and this is critical error case...
		 */
		WARN(1, "fail to find panel name, panel=%s, cmdline=%s\n",
				panel->name, panel_name);
		return;
	}

	/* TODO: after using component_bind in samsung_panel_init,
	 * it doesn't have to use vdd_data..
	 * Remove vdd_data, and allocate vdd memory here.
	 * vdds will be managed by vdds_list...
	 */
	vdd = ss_get_vdd(ndx);

	LCD_INFO("[DISPLAY_%d] ++ \n", vdd->ndx);

	ss_set_display_ndx(vdd, ndx);

	panel->panel_private = vdd;
	vdd->msm_private = panel;
	list_add(&vdd->vdd_list, &vdds_list);

	if (ss_panel_debug_init(vdd))
		LCD_ERR("Fail to create debugfs\n");

	if (ss_smmu_debug_init(vdd))
		LCD_ERR("Fail to create smmu debug\n");

	mutex_init(&vdd->vdd_lock);
	mutex_init(&vdd->cmd_lock);
	mutex_init(&vdd->bl_lock);
	mutex_init(&vdd->ss_spi_lock);

	/* To guarantee ALPM ON or OFF mode change operation*/
	mutex_init(&vdd->panel_lpm.lpm_lock);

	/* To prevent normal brightness patcket after tx LPM on */
	mutex_init(&vdd->panel_lpm.lpm_bl_lock);

	/* To guarantee dynamic MIPI clock change*/
	mutex_init(&vdd->dyn_mipi_clk.dyn_mipi_lock);

	if (ss_is_cmd_mode(vdd)) {
		vdd->panel_func.ss_event_osc_te_fitting =
			ss_event_osc_te_fitting;
	}

	vdd->panel_func.ss_event_frame_update =
		ss_event_frame_update;
	vdd->panel_func.ss_event_fb_event_callback =
		ss_event_fb_event_callback;
	vdd->panel_func.ss_event_esd_recovery_init =
		ss_event_esd_recovery_init;

	vdd->panel_dead = false;
	vdd->is_autorefresh_fail = false;

	if (IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_init))
		LCD_ERR("no samsung_panel_init fucn");
	else
		vdd->panel_func.samsung_panel_init(vdd);

	/* Panel revision selection */
	if (IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_revision)) {
		LCD_ERR("no panel_revision_selection_error function\n");
	} else {
		/* set manufacture_id_dsi from cmdline to set proper panel revision */
		if (vdd->ndx == PRIMARY_DISPLAY_NDX)
			vdd->manufacture_id_dsi = get_lcd_attached("GET");
		else if (vdd->ndx == SECONDARY_DISPLAY_NDX)
			vdd->manufacture_id_dsi = get_lcd_attached_secondary("GET");

		vdd->panel_func.samsung_panel_revision(vdd);
	}

	/* set manufacture_id_dsi to PBA_ID to read ID later  */
	vdd->manufacture_id_dsi = PBA_ID;

	if (vdd->mdnie.support_mdnie ||
			vdd->support_cabc ||
			ss_panel_attached(ndx))
		vdd->mdnie.mdnie_tune_state_dsi =
			init_dsi_tcon_mdnie_class(vdd);

	spin_lock_init(&vdd->esd_recovery.irq_lock);

	ss_panel_attach_set(vdd, true);

	INIT_DELAYED_WORK(&vdd->delay_disp_on_work, samsung_display_delay_disp_on_work);

	/* Init Other line panel support */
	if (!IS_ERR_OR_NULL(vdd->panel_func.parsing_otherline_pdata) && ss_panel_attached(vdd->ndx)) {
		if (!IS_ERR_OR_NULL(vdd->panel_func.get_panel_fab_type)) {
			if (vdd->panel_func.get_panel_fab_type() == NEW_FB_PANLE_TYPE) {
				LCD_ERR("parsing_otherline_pdata (%d)\n", vdd->panel_func.get_panel_fab_type());

				INIT_DELAYED_WORK(&vdd->other_line_panel_support_work, read_panel_data_work_fn);
				vdd->other_line_panel_support_workq =
					create_singlethread_workqueue("other_line_panel_support_wq");

				if (vdd->other_line_panel_support_workq) {
					vdd->other_line_panel_work_cnt = OTHERLINE_WORKQ_CNT;
					queue_delayed_work(vdd->other_line_panel_support_workq,
							&vdd->other_line_panel_support_work,
							msecs_to_jiffies(OTHERLINE_WORKQ_DEALY));
				}
			}
		}
	}

	mutex_init(&vdd->exclusive_tx.ex_tx_lock);
	vdd->exclusive_tx.enable = 0;
	init_waitqueue_head(&vdd->exclusive_tx.ex_tx_waitq);

	ss_create_sysfs(vdd);

#if defined(CONFIG_SEC_FACTORY)
	vdd->is_factory_mode = true;
#endif

	/* parse display dtsi node */
	ss_panel_parse_dt(vdd);

	ss_spi_init(vdd);

	ss_dsi_poc_init(vdd);

	/* self display */
	if (vdd->self_disp.is_support) {
		if (vdd->self_disp.init)
			vdd->self_disp.init(vdd);

		if (vdd->self_disp.data_init)
			vdd->self_disp.data_init(vdd);
		else
			LCD_ERR("no self display data init function\n");
	}

	/* mafpc */
	if (vdd->mafpc.is_support) {
		if (vdd->mafpc.init)
			vdd->mafpc.init(vdd);

		if (vdd->mafpc.data_init)
			vdd->mafpc.data_init(vdd);
		else
			LCD_ERR("no mafpc data init function\n");
	}

	if (vdd->copr.panel_init)
		vdd->copr.panel_init(vdd);

	/* register notifier and init workQ.
	 * TODO: move hall_ic_notifier_display and dyn_mipi_clk notifier here.
	 */
	if (vdd->vrr.lfd.support_lfd) {
		vdd->vrr.lfd.nb_lfd_touch.priority = 3;
		vdd->vrr.lfd.nb_lfd_touch.notifier_call = ss_lfd_touch_notify_cb;
		sec_input_register_notify(&vdd->vrr.lfd.nb_lfd_touch, ss_lfd_touch_notify_cb, 3);

		vdd->vrr.lfd.lfd_touch_wq = create_singlethread_workqueue("lfd_touch_wq");
		if (!vdd->vrr.lfd.lfd_touch_wq) {
			LCD_ERR("failed to create touch_lfd workqueue..\n");
			return;
		}
		INIT_WORK(&vdd->vrr.lfd.lfd_touch_work, ss_lfd_touch_work);
	}

	/* work thread for panel notifier */
	vdd->notify_wq = create_singlethread_workqueue("panel_notify_wq");
	if (vdd->notify_wq == NULL)
		LCD_ERR("failed to create read notify workqueue..\n");
	else {
		INIT_WORK(&vdd->bl_event_work, (work_func_t)ss_bl_event_work);
		INIT_WORK(&vdd->vrr_event_work, (work_func_t)ss_vrr_event_work);
		INIT_WORK(&vdd->lfd_event_work, (work_func_t)ss_lfd_event_work);
		INIT_WORK(&vdd->panel_state_event_work, (work_func_t)ss_panel_state_event_work);
	}

	mutex_init(&vdd->notify_lock);

	/* init gamma mode2 flash data */
	vdd->br_info.gm2_table.flash_gm2_init_done = false;
	ss_gm2_ddi_flash_init(vdd);

	LCD_INFO("window_color : [%s]\n", vdd->window_color);

	LCD_INFO("[DISPLAY_%d] -- \n", vdd->ndx);
}

void ss_set_vrr_switch(struct samsung_display_driver_data *vdd, bool enable)
{
	/* notify clients that vrr has changed */
	vdd->vrr.param_update = enable;
	ss_notify_queue_work(vdd, PANEL_EVENT_VRR_CHANGED);
	return;
}
