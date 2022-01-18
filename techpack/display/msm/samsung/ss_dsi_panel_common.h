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
Copyright (C) 2012, Samsung Electronics. All rights reserved.

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

#ifndef SS_DSI_PANEL_COMMON_H
#define SS_DSI_PANEL_COMMON_H

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/err.h>
#include <linux/lcd.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <asm/div64.h>
#include <linux/interrupt.h>
#include <linux/msm-bus.h>
#include <linux/sched.h>
#include <linux/dma-buf.h>
#include <linux/debugfs.h>
#include <linux/miscdevice.h>
#include <linux/reboot.h>
#include <video/mipi_display.h>

#include <linux/dev_ril_bridge.h>

#include <linux/regulator/consumer.h>
#if 0
#if defined(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif
#endif

#include "dsi_display.h"
#include "dsi_panel.h"
#include "dsi_drm.h"
#include "sde_kms.h"
#include "sde_connector.h"
#include "sde_encoder.h"
#include "sde_encoder_phys.h"
#include "sde_trace.h"

#include "ss_ddi_spi_common.h"
#include "ss_dpui_common.h"

#include "ss_dsi_panel_sysfs.h"
#include "ss_dsi_panel_debug.h"

#include "ss_ddi_poc_common.h"
#include "ss_copr_common.h"

#include "./SELF_DISPLAY/self_display.h"
#include "./MAFPC/ss_dsi_mafpc_common.h"

#include "ss_interpolation_common.h"
#include "ss_flash_table_data_common.h"

#include "ss_panel_notify.h"

#if defined(CONFIG_SEC_DEBUG)
#include <linux/sec_debug.h>
#endif

extern bool enable_pr_debug;

#define LOG_KEYWORD "[SDE]"

#define LCD_DEBUG(X, ...)	\
		do {	\
			if (enable_pr_debug)	\
				pr_info("%s %s : "X, LOG_KEYWORD, __func__, ## __VA_ARGS__);\
			else	\
				pr_debug("%s %s : "X, LOG_KEYWORD, __func__, ## __VA_ARGS__);\
		} while (0)	\

#define LCD_INFO(X, ...) pr_info("%s %s : "X, LOG_KEYWORD, __func__, ## __VA_ARGS__)
#define LCD_INFO_ONCE(X, ...) pr_info_once("%s %s : "X, LOG_KEYWORD, __func__, ## __VA_ARGS__)
#define LCD_ERR(X, ...) pr_err("%s %s : "X, LOG_KEYWORD, __func__, ## __VA_ARGS__)

#define MAX_PANEL_NAME_SIZE 100

#define PARSE_STRING 64
#define MAX_EXTRA_POWER_GPIO 4
#define MAX_BACKLIGHT_TFT_GPIO 4

/* OSC TE FITTING */
#define OSC_TE_FITTING_LUT_MAX 2

/* Register dump info */
#define MAX_INTF_NUM 2

/* Panel Unique Chip ID Byte count */
#define MAX_CHIP_ID 6

/* Panel Unique Cell ID Byte count */
#define MAX_CELL_ID 16

/* Panel Unique OCTA ID Byte count */
#define MAX_OCTA_ID 20

/* PBA booting ID */
#define PBA_ID 0xFFFFFF

/* Default elvss_interpolation_lowest_temperature */
#define ELVSS_INTERPOLATION_TEMPERATURE -20

/* Default lux value for entering mdnie HBM */
#define ENTER_HBM_CE_LUX 40000

/* MAX ESD Recovery gpio */
#define MAX_ESD_GPIO 2

#define BASIC_FB_PANLE_TYPE 0x01
#define NEW_FB_PANLE_TYPE 0x00
#define OTHERLINE_WORKQ_DEALY 900 /*ms*/
#define OTHERLINE_WORKQ_CNT 70

#define MDNIE_TUNE_MAX_SIZE 6
//#define DYNAMIC_DSI_CLK

#define USE_CURRENT_BL_LEVEL 0xFFFFFF

extern int poweroff_charging;

struct vrr_info;
struct lfd_base_str;
enum LFD_SCOPE_ID;

enum PANEL_LEVEL_KEY {
	LEVEL_KEY_NONE = 0,
	LEVEL0_KEY = BIT(0),
	LEVEL1_KEY = BIT(1),
	LEVEL2_KEY = BIT(2),
	POC_KEY = BIT(3),
};

enum backlight_origin {
	BACKLIGHT_NORMAL,
	BACKLIGHT_FINGERMASK_ON,
	BACKLIGHT_FINGERMASK_OFF,
};

enum mipi_samsung_cmd_map_list {
	PANEL_CMD_MAP_NULL,
	PANEL_CMD_MAP_MAX,
};

enum {
	MIPI_RESUME_STATE,
	MIPI_SUSPEND_STATE,
};

enum {
	ACL_OFF,
	ACL_30,
	ACL_15,
	ACL_50
};

enum {
	TE_FITTING_DONE = BIT(0),
	TE_CHECK_ENABLE = BIT(1),
	TE_FITTING_REQUEST_IRQ = BIT(3),
	TE_FITTING_STEP1 = BIT(4),
	TE_FITTING_STEP2 = BIT(5),
};

enum br_mode {
	FLASH_GAMMA = 0, /* read gamma related data from flash memory */
	TABLE_GAMMA,	 /* use gamma related data from fixed values */
	GAMMA_MODE2,	 /* use gamma mode2 */
	MAX_BR_MODE,
};


#if 0
enum ACL_GRADUAL_MODE {
	GRADUAL_ACL_OFF,
	GRADUAL_ACL_ON,
	GRADUAL_ACL_UNSTABLE,
};
#endif

#define SAMSUNG_DISPLAY_PINCTRL0_STATE_DEFAULT "samsung_display_gpio_control0_default"
#define SAMSUNG_DISPLAY_PINCTRL0_STATE_SLEEP  "samsung_display_gpio_control0_sleep"
#define SAMSUNG_DISPLAY_PINCTRL1_STATE_DEFAULT "samsung_display_gpio_control0_default"
#define SAMSUNG_DISPLAY_PINCTRL1_STATE_SLEEP  "samsung_display_gpio_control0_sleep"

enum {
	SAMSUNG_GPIO_CONTROL0,
	SAMSUNG_GPIO_CONTROL1,
};

/* foder open : 0, close : 1 */
enum {
	HALL_IC_OPEN,
	HALL_IC_CLOSE,
	HALL_IC_UNDEFINED,
};

#define LCD_FLIP_NOT_REFRESH	BIT(8)

enum IRC_MODE {
	IRC_MODERATO_MODE = 0, /* fixed value */
	IRC_FLAT_GAMMA_MODE = 1,  /* fixed value */
	IRC_MAX_MODE,
};

enum {
	VDDM_ORG = 0,
	VDDM_LV,
	VDDM_HV,
	MAX_VDDM,
};

enum {
	MIPI_TX_TYPE_GRAM,
	MIPI_TX_TYPE_SIDERAM,
};

struct te_fitting_lut {
	int te_duration;
	int value;
};

struct osc_te_fitting_info {
	unsigned int status;
	long long te_duration;
	long long *te_time;
	int sampling_rate;
	struct completion te_check_comp;
	struct work_struct work;
	struct te_fitting_lut *lut[OSC_TE_FITTING_LUT_MAX];
};

struct lpm_pwr_ctrl {
	bool support_lpm_pwr_ctrl;
	char lpm_pwr_ctrl_supply_name[32];
	int lpm_pwr_ctrl_supply_min_v;
	int lpm_pwr_ctrl_supply_max_v;

	char lpm_pwr_ctrl_elvss_name[32];
	int lpm_pwr_ctrl_elvss_lpm_v;
	int lpm_pwr_ctrl_elvss_normal_v;
};

struct lpm_info {
	u8 origin_mode;
	u8 ver;
	u8 mode;
	u8 hz;
	int lpm_bl_level;
	bool esd_recovery;
	int need_self_grid;

	struct mutex lpm_lock;

	/* To prevent normal brightness patcket after tx LPM on */
	struct mutex lpm_bl_lock;

	struct lpm_pwr_ctrl lpm_pwr;
};

struct clk_timing_table {
	int tab_size;
	int *clk_rate;
};

struct clk_sel_table {
	int tab_size;
	int *rat;
	int *band;
	int *from;
	int *end;
	int *target_clk_idx;
};

struct rf_info {
	u8 rat;
	u32 band;
	u32 arfcn;
} __packed;

struct dyn_mipi_clk {
	struct notifier_block notifier;
	struct mutex dyn_mipi_lock; /* protect rf_info's rat, band, and arfcn */
	struct workqueue_struct *change_clk_wq;
	struct work_struct change_clk_work;
	struct clk_sel_table clk_sel_table;
	struct clk_timing_table clk_timing_table;
	struct rf_info rf_info;
	int is_support;
	int force_idx;  /* force to set clk idx for test purpose */
};

struct cmd_map {
	int *bl_level;
	int *cmd_idx;
	int size;
};

enum CD_MAP_TABLE_LIST {
	NORMAL,
	PAC_NORMAL,
	HBM,
	PAC_HBM,
	AOD,
	HMT,
	MULTI_TO_ONE_NORMAL,
	CD_MAP_TABLE_MAX,
};

struct candela_map_table {
	int tab_size;
	int *scaled_idx;
	int *idx;
	int *from;
	int *end;

	/*
		cd :
		This is cacultated brightness to standardize brightness formula.
	*/
	int *cd;

	/* 	interpolation_cd :
		cd value is only calcuated brightness by formula.
		This is real measured brightness on panel.
	*/
	int *interpolation_cd;
	int *gm2_wrdisbv;	/* Gamma mode2 WRDISBV Write Display Brightness */

	int min_lv;
	int max_lv;

	int *auto_level;
};

struct samsung_display_dtsi_data {
	bool samsung_lp11_init;
	bool samsung_tcon_clk_on_support;
	bool samsung_esc_clk_128M;
	bool samsung_osc_te_fitting;
	bool samsung_support_factory_panel_swap;
	u32  samsung_power_on_reset_delay;
	u32  samsung_dsi_off_reset_delay;
	u32 samsung_lpm_init_delay;	/* 2C/3C memory access -> wait for samsung_lpm_init_delay ms -> HLPM on seq. */
	u8	samsung_delayed_display_on;

	/* Anapass DDI's unique power sequence:
	 * VDD on -> LP11 -> reset -> wait TCON RDY high -> HS clock.
	 * samsung_anapass_power_seq flag allows above unique power seq..
	 */
	bool samsung_anapass_power_seq;
	int samsung_tcon_rdy_gpio;
	int samsung_delay_after_tcon_rdy;

	/* If display->ctrl_count is 2, it broadcasts.
	 * To prevent to send mipi cmd thru mipi dsi1, set samsung_cmds_unicast flag.
	 */
	bool samsung_cmds_unicast;

	/* To reduce DISPLAY ON time */
	u32 samsung_reduce_display_on_time;
	u32 samsung_dsi_force_clock_lane_hs;
	u32 samsung_wait_after_reset_delay;
	u32 samsung_wait_after_sleep_out_delay;
	u32 samsung_finger_print_irq_num;
	u32 samsung_home_key_irq_num;

	/*
	 * index[0] : array index for te fitting command from "ctrl->on_cmd"
	 * index[1] : array index for te fitting command from "osc_te_fitting_tx_cmds"
	 */
	int samsung_osc_te_fitting_cmd_index[2];
	int backlight_tft_gpio[MAX_BACKLIGHT_TFT_GPIO];

	/* PANEL TX / RX CMDS LIST */
	struct dsi_panel_cmd_set cmd_sets[SS_DSI_CMD_SET_MAX];

	bool panel_lpm_enable;
	bool hmt_enabled;

	/* TFT LCD Features*/
	int tft_common_support;
	int backlight_gpio_config;
	int pwm_ap_support;
	const char *tft_module_name;
	const char *panel_vendor;
	const char *disp_model;

	/* MDINE HBM_CE_TEXT_MDNIE mode used */
	int hbm_ce_text_mode_support;

	/* Backlight IC discharge delay */
	int blic_discharging_delay_tft;
	int cabc_delay;

	int ddi_id_length;
};

struct display_status {
	bool wait_disp_on;
	int wait_actual_disp_on;
	int hbm_mode;
	int disp_on_pre; /* set to 1 at first ss_panel_on_pre(). it  means that kernel initialized display */
	bool first_commit_disp_on;
};

struct hmt_status {
	unsigned int hmt_on;

	int hmt_bl_level;
	int candela_level_hmt;
	int cmd_idx_hmt;
};

struct esd_recovery {
	spinlock_t irq_lock;
	bool esd_recovery_init;
	bool is_enabled_esd_recovery;
	bool is_wakeup_source;
	int esd_gpio[MAX_ESD_GPIO];
	u8 num_of_gpio;
	unsigned long irqflags[MAX_ESD_GPIO];
	void (*esd_irq_enable)(bool enable, bool nosync, void *data);

	/* send recovery pck before sending image date (for ESD recovery) */
	int send_esd_recovery;
};

/* Panel LPM(ALPM/HLPM) status flag */
// TODO: how about to use vdd->panel_state instaed of  below..???
enum {
	LPM_VER0 = 0,
	LPM_VER1,
};

enum {
	LPM_MODE_OFF = 0,			/* Panel LPM Mode OFF */
	ALPM_MODE_ON,			/* ALPM Mode On */
	HLPM_MODE_ON,			/* HLPM Mode On */
	MAX_LPM_MODE,			/* Panel LPM Mode MAX */
};

enum {
	ALPM_MODE_ON_2NIT = 1,		/* ALPM Mode On 2nit*/
	HLPM_MODE_ON_2NIT,			/* HLPM Mode On 60nit*/
	ALPM_MODE_ON_60NIT,			/* ALPM Mode On 2nit*/
	HLPM_MODE_ON_60NIT,			/* HLPM Mode On 60nit*/
};

enum {
	LPM_2NIT_IDX = 0,
	LPM_10NIT_IDX,
	LPM_30NIT_IDX,
	LPM_40NIT_IDX,
	LPM_60NIT_IDX,
	LPM_BRIGHTNESS_MAX_IDX,
};

enum {
	LPM_2NIT = 2,
	LPM_10NIT = 10,
	LPM_30NIT = 30,
	LPM_40NIT = 40,
	LPM_60NIT = 60,
	LPM_BRIGHTNESS_MAX,
};

struct samsung_register_info {
	size_t virtual_addr;
};

struct samsung_register_dump_info {
	/* DSI PLL */
	struct samsung_register_info dsi_pll;

	/* DSI CTRL */
	struct samsung_register_info dsi_ctrl;

	/* DSI PHY */
	struct samsung_register_info dsi_phy;
};

struct samsung_display_debug_data {
	struct dentry *root;
	struct dentry *dump;
	struct dentry *hw_info;
	struct dentry *display_status;
	struct dentry *display_ltp;

	bool print_cmds;
	bool *is_factory_mode;
	bool panic_on_pptimeout;

	/* misc */
	struct miscdevice dev;
	bool report_once;
};

struct self_display {
	struct miscdevice dev;

	int is_support;
	int factory_support;
	int on;
	int file_open;
	int time_set;

	struct self_time_info st_info;
	struct self_icon_info si_info;
	struct self_grid_info sg_info;
	struct self_analog_clk_info sa_info;
	struct self_digital_clk_info sd_info;
	struct self_partial_hlpm_scan sphs_info;

	struct mutex vdd_self_display_lock;
	struct mutex vdd_self_display_ioctl_lock;
	struct self_display_op operation[FLAG_SELF_DISP_MAX];

	struct self_display_debug debug;

	u8 *mask_crc_pass_data;	// implemented in dtsi
	u8 *mask_crc_read_data;
	int mask_crc_size;

	/* Self display Function */
	int (*init)(struct samsung_display_driver_data *vdd);
	int (*data_init)(struct samsung_display_driver_data *vdd);
	int (*aod_enter)(struct samsung_display_driver_data *vdd);
	int (*aod_exit)(struct samsung_display_driver_data *vdd);
	void (*self_mask_img_write)(struct samsung_display_driver_data *vdd);
	void (*self_mask_on)(struct samsung_display_driver_data *vdd, int enable);
	int (*self_mask_check)(struct samsung_display_driver_data *vdd);
	void (*self_blinking_on)(struct samsung_display_driver_data *vdd, int enable);
	int (*self_display_debug)(struct samsung_display_driver_data *vdd);
	void (*self_move_set)(struct samsung_display_driver_data *vdd, int ctrl);
	int (*self_icon_set)(struct samsung_display_driver_data *vdd);
	int (*self_grid_set)(struct samsung_display_driver_data *vdd);
	int (*self_aclock_set)(struct samsung_display_driver_data *vdd);
	int (*self_dclock_set)(struct samsung_display_driver_data *vdd);
	int (*self_time_set)(struct samsung_display_driver_data *vdd, int from_self_move);
	int (*self_partial_hlpm_scan_set)(struct samsung_display_driver_data *vdd);
};

enum mdss_cpufreq_cluster {
	CPUFREQ_CLUSTER_BIG,
	CPUFREQ_CLUSTER_LITTLE,
	CPUFREQ_CLUSTER_ALL,
};

enum ss_panel_pwr_state {
	PANEL_PWR_OFF,
	PANEL_PWR_ON_READY,
	PANEL_PWR_ON,
	PANEL_PWR_LPM,
	MAX_PANEL_PWR,
};

/*  COMMON_DISPLAY_NDX
 *   - ss_display ndx for common data such as debugging info
 *    which do not need to be handled seperately by the panels
 *  PRIMARY_DISPLAY_NDX
 *   - ss_display ndx for data only for primary display
 *  SECONDARY_DISPLAY_NDX
 *   - ss_display ndx for data only for secondary display
 */
enum ss_display_ndx {
	COMMON_DISPLAY_NDX = 0,
	PRIMARY_DISPLAY_NDX = 0,
	SECONDARY_DISPLAY_NDX,
	MAX_DISPLAY_NDX,
};

/* POC */

enum {
	POC_OP_NONE = 0,
	POC_OP_ERASE = 1,
	POC_OP_WRITE = 2,
	POC_OP_READ = 3,
	POC_OP_ERASE_WRITE_IMG = 4,
	POC_OP_ERASE_WRITE_TEST = 5,
	POC_OP_BACKUP = 6,
	POC_OP_ERASE_SECTOR = 7,
	POC_OP_CHECKSUM,
	POC_OP_CHECK_FLASH,
	POC_OP_SET_FLASH_WRITE,
	POC_OP_SET_FLASH_EMPTY,
	MAX_POC_OP,
};

enum poc_state {
	POC_STATE_NONE,
	POC_STATE_FLASH_EMPTY,
	POC_STATE_FLASH_FILLED,
	POC_STATE_ER_START,
	POC_STATE_ER_PROGRESS,
	POC_STATE_ER_COMPLETE,
	POC_STATE_ER_FAILED,
	POC_STATE_WR_START,
	POC_STATE_WR_PROGRESS,
	POC_STATE_WR_COMPLETE,
	POC_STATE_WR_FAILED,
	MAX_POC_STATE,
};

#define IOC_GET_POC_STATUS	_IOR('A', 100, __u32)		/* 0:NONE, 1:ERASED, 2:WROTE, 3:READ */
#define IOC_GET_POC_CHKSUM	_IOR('A', 101, __u32)		/* 0:CHKSUM ERROR, 1:CHKSUM SUCCESS */
#define IOC_GET_POC_CSDATA	_IOR('A', 102, __u32)		/* CHKSUM DATA 4 Bytes */
#define IOC_GET_POC_ERASED	_IOR('A', 103, __u32)		/* 0:NONE, 1:NOT ERASED, 2:ERASED */
#define IOC_GET_POC_FLASHED	_IOR('A', 104, __u32)		/* 0:NOT POC FLASHED(0x53), 1:POC FLAHSED(0x52) */

#define IOC_SET_POC_ERASE	_IOR('A', 110, __u32)		/* ERASE POC FLASH */
#define IOC_SET_POC_TEST	_IOR('A', 112, __u32)		/* POC FLASH TEST - ERASE/WRITE/READ/COMPARE */

struct POC {
	bool is_support;
	int poc_operation;

	u32 file_opend;
	struct miscdevice dev;
	bool erased;
	atomic_t cancel;
	struct notifier_block dpui_notif;

	u8 chksum_data[4];
	u8 chksum_res;

	u8 *wbuf;
	u32 wpos;
	u32 wsize;

	u8 *rbuf;
	u32 rpos;
	u32 rsize;

	int start_addr;
	int image_size;

	/* ERASE */
	int er_try_cnt;
	int er_fail_cnt;
	u32 erase_delay_us; /* usleep */
	int erase_sector_addr_idx[3];

	/* WRITE */
	int wr_try_cnt;
	int wr_fail_cnt;
	u32 write_delay_us; /* usleep */
	int write_loop_cnt;
	int write_data_size;
	int write_addr_idx[3];

	/* READ */
	int rd_try_cnt;
	int rd_fail_cnt;
	u32 read_delay_us;	/* usleep */
	int read_addr_idx[3];

	/* MCA (checksum) */
	u8 *mca_data;
	int mca_size;

	/* POC Function */
	int (*poc_write)(struct samsung_display_driver_data *vdd, u8 *data, u32 pos, u32 size);
	int (*poc_read)(struct samsung_display_driver_data *vdd, u8 *buf, u32 pos, u32 size);
	int (*poc_erase)(struct samsung_display_driver_data *vdd, u32 erase_pos, u32 erase_size, u32 target_pos);

	int (*poc_open)(struct samsung_display_driver_data *vdd);
	int (*poc_release)(struct samsung_display_driver_data *vdd);

	void (*poc_comp)(struct samsung_display_driver_data *vdd);
	int (*check_read_case)(struct samsung_display_driver_data *vdd);

	int read_case;

	bool need_sleep_in;
};

/* FirmWare Update */
struct FW_UP {
	bool is_support;

	u32 start_addr;
	u32 image_size;
	u32 sector_size;
	u8 *image_buf;

	/* ERASE */
	int er_try_cnt;
	int er_fail_cnt;
	u32 erase_delay_us; /* usleep */
	u32 erase_data_size;
	int erase_addr_idx[3];
	int erase_size_idx[3];

	/* WRITE */
	int wr_try_cnt;
	int wr_fail_cnt;
	u32 write_delay_us; /* usleep */
	u32 write_data_size;
	int write_addr_idx[3];
	int write_size_idx[3];

	/* READ */
	u32 read_data_size;
	u32 read_delay_us;	/* usleep */
	u32 read_status_value;
	u32 read_done_check;

	bool need_sleep_in;
	bool cmd_done;
};

enum fw_up_state {
	FW_UP_DONE,
	FW_UP_ERR_UPDATE_FAIL,
	FW_UP_ERR_ALREADY_DONE,
	FW_UP_ERR_NOT_SUPPORT,
	MAX_FW_UP_STATE,
};

#define GCT_RES_CHECKSUM_PASS	(1)
#define GCT_RES_CHECKSUM_NG	(0)
#define GCT_RES_CHECKSUM_OFF	(-2)
#define GCT_RES_CHECKSUM_NOT_SUPPORT	(-3)

struct gram_checksum_test {
	bool is_support;
	int on;
	u8 checksum[4];
};

/* ss_exclusive_mipi_tx: block dcs tx and
 * "frame update", in ss_event_frame_update_pre().
 * Allow only dcs packets that has pass token.
 *
 * How to use exclusive_mipi_tx
 * 1) lock ex_tx_lock.
 * 2) set enable to true. This means that exclusvie mode is enabled,
 *    and the only packet that has token (exclusive_pass==true) can be sent.
 *    Other packets, which doesn't have token, should wait
 *    until exclusive mode disabled.
 * 3) calls ss_set_exclusive_tx_packet() to set the packet's
 *    exclusive_pass to true.
 * 4) In the end, calls wake_up_all(&vdd->exclusive_tx.ex_tx_waitq)
 *    to start to tx other mipi cmds that has no token and is waiting
 *    to be sent.
 */
struct ss_exclusive_mipi_tx {
	struct mutex ex_tx_lock;
	int enable; /* This shuold be set in ex_tx_lock lock */
	wait_queue_head_t ex_tx_waitq;

	/*
		To allow frame update under exclusive mode.
		Please be careful & Check exclusive cmds allow 2C&3C or othere value at frame header.
	*/
	int permit_frame_update;
};

struct mdnie_info {
	int support_mdnie;
	int support_trans_dimming;
	int disable_trans_dimming;

	int enter_hbm_ce_lux;	// to apply HBM CE mode in mDNIe

	bool tuning_enable_tft;

	int lcd_on_notifiy;

	int mdnie_x;
	int mdnie_y;

	int mdnie_tune_size[MDNIE_TUNE_MAX_SIZE];

	struct mdnie_lite_tun_type *mdnie_tune_state_dsi;
	struct mdnie_lite_tune_data *mdnie_data;
};

struct brightness_info {
	int pac;

	int elvss_interpolation_temperature;

	int auto_level;		// auto level in HBM step
	int bl_level;		// brightness level via backlight dev

	int cd_level;
	int interpolation_cd;
	int gm2_wrdisbv;	/* Gamma mode2 WRDISBV Write Display Brightness */
	int gamma_mode2_support;
	int multi_to_one_support;

	/* SAMSUNG_FINGERPRINT */
	int finger_mask_bl_level;
	int finger_mask_hbm_on;

	int cd_idx;			// original idx
	int pac_cd_idx;		// scaled idx

	u8 elvss_value[2];	// elvss otp value

	u8 *irc_otp;		// irc otp value

	int aor_data;

	/* To enhance color accuracy, change IRC mode for each screen mode.
	 * - Adaptive display mode: Moderato mode
	 * - Other screen modes: Flat Gamma mode
	 */
	enum IRC_MODE irc_mode;
	int support_irc;

	struct workqueue_struct *br_wq;
	struct work_struct br_work;
};

struct STM_CMD {
	int STM_CTRL_EN;
	int STM_MAX_OPT;
	int	STM_DEFAULT_OPT;
	int STM_DIM_STEP;
	int STM_FRAME_PERIOD;
	int STM_MIN_SECT;
	int STM_PIXEL_PERIOD;
	int STM_LINE_PERIOD;
	int STM_MIN_MOVE;
	int STM_M_THRES;
	int STM_V_THRES;
};

struct STM_REG_OSSET{
	const char *name;
	int offset;
};

struct STM {
	int stm_on;
	struct STM_CMD orig_cmd;
	struct STM_CMD cur_cmd;
};

struct ub_con_detect {
	spinlock_t irq_lock;
	int gpio;
	unsigned long irqflag;
	bool enabled;
	int ub_con_cnt;
	int current_wakeup_context_gpio_status;
};

struct motto_data {
	bool init_backup;
	u32 motto_swing;
	u32 hstx_init;
	u32 motto_emphasis;
	u32 cal_sel_init;
	u32 cmn_ctrl2_init;
	u32 cal_sel_curr; /* current value */
	u32 cmn_ctrl2_curr;
};

enum CHECK_SUPPORT_MODE {
	CHECK_SUPPORT_MCD,
	CHECK_SUPPORT_HMD,
	CHECK_SUPPORT_BRIGHTDOT,
};

struct brightness_table {
	int refresh_rate;
	bool is_sot_hs_mode; /* Scan of Time is HS mode for VRR */

	int idx;

	/* Copy MTP data from parent node, and skip reading MTP data
	 * from DDI flash.
	 * Ex) 48hz and 96hz mode use same MTP data with 60hz and 120hz mode,
	 * respectively. So, those modes copy MTP data from 60hz and 120hz mode.
	 * But, those have different DDI VFP value, so have different AOR
	 * interpolation table.
	 */
	int parent_idx;

	/* ddi vertical porches are used for AOR interpolation.
	 * In case of 96/48hz mode, its base AOR came from below base RR mode.
	 * - 96hz: 120hz HS -> AOR_96hz = AOR_120hz * (vtotal_96hz) / (vtotal_120hz)
	 * - 48hz: 60hz normal -> AOR_48hz = AOR_60hz * (vtotal_48hz) / (vtotal_60hz)
	 * If there is no base vertical porches, (ex: ddi_vbp_base) in panel dtsi,
	 * parser function set base value as target value (ex: ddi_vbp_base = ddi_vbp).
	 */
	int ddi_vfp, ddi_vbp, ddi_vactive;
	int ddi_vfp_base, ddi_vbp_base, ddi_vactive_base;

	/*
	 *  Smart Diming
	 */
	struct smartdim_conf *smart_dimming_dsi;

	/*
	 *  HMT
	 */
	struct smartdim_conf *smart_dimming_dsi_hmt;

	/* flash */
	struct flash_raw_table *gamma_tbl;
	struct ss_interpolation flash_itp;
	struct ss_interpolation table_itp;

	/* Brightness_raw data parsing information */
	struct dimming_tbl hbm_tbl;
	struct dimming_tbl normal_tbl;
	struct dimming_tbl hmd_tbl; /* hmd doesn't use vint, elvss, and irc members */

	unsigned char hbm_max_gamma[SZ_64]; /* hbm max gamma from ddi */
	unsigned char normal_max_gamma[SZ_64]; /* normal max gamma from ddi */

	/* Debugging */
	struct PRINT_TABLE *print_table;
	int print_size;
	struct PRINT_TABLE *print_table_hbm;
	int print_size_hbm;
	struct ss_interpolation_brightness_table *orig_normal_table;
	struct ss_interpolation_brightness_table *orig_hbm_table;
};

struct panel_func {
	/* ON/OFF */
	int (*samsung_panel_on_pre)(struct samsung_display_driver_data *vdd);
	int (*samsung_panel_on_post)(struct samsung_display_driver_data *vdd);
	int (*samsung_panel_off_pre)(struct samsung_display_driver_data *vdd);
	int (*samsung_panel_off_post)(struct samsung_display_driver_data *vdd);
	void (*samsung_backlight_late_on)(struct samsung_display_driver_data *vdd);
	void (*samsung_panel_init)(struct samsung_display_driver_data *vdd);

	/* DDI RX */
	char (*samsung_panel_revision)(struct samsung_display_driver_data *vdd);
	int (*samsung_module_info_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_manufacture_date_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_ddi_id_read)(struct samsung_display_driver_data *vdd);

	int (*samsung_cell_id_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_octa_id_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_hbm_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_elvss_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_irc_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_mdnie_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_smart_dimming_init)(struct samsung_display_driver_data *vdd, struct brightness_table *br_tbl);

	/* samsung_flash_gamma_support : check flash gamma support for specific contidion */
	int (*samsung_flash_gamma_support)(struct samsung_display_driver_data *vdd);
	int (*samsung_interpolation_init)(struct samsung_display_driver_data *vdd, struct brightness_table *br_tbl, enum INTERPOLATION_MODE mode);
	int (*force_use_table_for_hmd)(struct samsung_display_driver_data *vdd, struct brightness_table *br_tbl);

	struct smartdim_conf *(*samsung_smart_get_conf)(void);

	/* Brightness */
	struct dsi_panel_cmd_set * (*samsung_brightness_hbm_off)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_aid)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_acl_on)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_pre_acl_percent)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_acl_percent)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_acl_off)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_pre_elvss)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_elvss)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_pre_caps)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_caps)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_elvss_temperature1)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_elvss_temperature2)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_dbv)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_vint)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_irc)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_gamma)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_gm2_gamma_comp)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_ltps)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_normal_etc)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_vrr)(struct samsung_display_driver_data *vdd, int *level_key);

	/* HBM */
	struct dsi_panel_cmd_set * (*samsung_hbm_gamma)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_hbm_etc)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_hbm_irc)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_vrr_hbm)(struct samsung_display_driver_data *vdd, int *level_key);

	struct dsi_panel_cmd_set * (*samsung_hbm_acl_on)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_hbm_acl_off)(struct samsung_display_driver_data *vdd, int *level_key);
	int (*get_hbm_candela_value)(int level);

	/* Total level key in brightness */
	int (*samsung_brightness_tot_level)(struct samsung_display_driver_data *vdd);

	/* Event */
	void (*ss_event_frame_update)(struct samsung_display_driver_data *vdd, int event, void *arg);
	void (*ss_event_fb_event_callback)(struct samsung_display_driver_data *vdd, int event, void *arg);
	void (*ss_event_osc_te_fitting)(struct samsung_display_driver_data *vdd, int event, void *arg);
	void (*ss_event_esd_recovery_init)(struct samsung_display_driver_data *vdd, int event, void *arg);

	/* OSC Tuning */
	int (*samsung_osc_te_fitting)(struct samsung_display_driver_data *vdd);
	int (*samsung_change_ldi_fps)(struct samsung_display_driver_data *vdd,
			unsigned int input_fps);

	/* HMT */
	struct dsi_panel_cmd_set * (*samsung_brightness_gamma_hmt)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_aid_hmt)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_elvss_hmt)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_vint_hmt)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_hmt)(struct samsung_display_driver_data *vdd, int *level_key);
	struct dsi_panel_cmd_set * (*samsung_brightness_vrr_hmt)(struct samsung_display_driver_data *vdd, int *level_key);

	int (*samsung_smart_dimming_hmt_init)(struct samsung_display_driver_data *vdd);
	struct smartdim_conf *(*samsung_smart_get_conf_hmt)(void);

	/* TFT */
	void (*samsung_tft_blic_init)(struct samsung_display_driver_data *vdd);
	void (*samsung_brightness_tft_pwm)(struct samsung_display_driver_data *vdd, int level);
	struct dsi_panel_cmd_set * (*samsung_brightness_tft_pwm_ldi)(struct samsung_display_driver_data *vdd, int *level_key);

	void (*samsung_bl_ic_pwm_en)(int enable);
	void (*samsung_bl_ic_i2c_ctrl)(int scaled_level);
	void (*samsung_bl_ic_outdoor)(int enable);

	/*LVDS*/
	void (*samsung_ql_lvds_register_set)(struct samsung_display_driver_data *vdd);
	int (*samsung_lvds_write_reg)(u16 addr, u32 data);

	/* Panel LPM(ALPM/HLPM) */
	void (*samsung_update_lpm_ctrl_cmd)(struct samsung_display_driver_data *vdd);
	void (*samsung_set_lpm_brightness)(struct samsung_display_driver_data *vdd);

	/* A3 line panel data parsing fn */
	int (*parsing_otherline_pdata)(struct file *f, struct samsung_display_driver_data *vdd,
		char *src, int len);
	void (*set_panel_fab_type)(int type);
	int (*get_panel_fab_type)(void);

	/* color weakness */
	void (*color_weakness_ccb_on_off)(struct samsung_display_driver_data *vdd, int mode);

	/* DDI H/W Cursor */
	int (*ddi_hw_cursor)(struct samsung_display_driver_data *vdd, int *input);

	/* COVER CONTROL */
	void (*samsung_cover_control)(struct samsung_display_driver_data *vdd);

	/* POC */
	int (*samsung_poc_ctrl)(struct samsung_display_driver_data *vdd, u32 cmd, const char *buf);

	/* FirmWare Update */
	int (*samsung_fw_up)(struct samsung_display_driver_data *vdd, int mode);


	/* Gram Checksum Test */
	int (*samsung_gct_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_gct_write)(struct samsung_display_driver_data *vdd);

	/* GraySpot Test */
	void (*samsung_gray_spot)(struct samsung_display_driver_data *vdd, int enable);

	/* MCD Test */
	void (*samsung_mcd_etc)(struct samsung_display_driver_data *vdd, int enable);

	/* Gamma mode2 DDI flash */
	int (*samsung_gm2_ddi_flash_prepare)(struct samsung_display_driver_data *vdd);

	/* Gamma mode2 gamma compensation (for 48/96hz VRR mode) */
	int (*samsung_gm2_gamma_comp_init)(struct samsung_display_driver_data *vdd);

	/* PBA */
	void (*samsung_pba_config)(struct samsung_display_driver_data *vdd, void *arg);

	/* This has been applied temporarily for sensors and will be removed later.*/
	void (*get_aor_data)(struct samsung_display_driver_data *vdd);

	/* gamma_flash : interpolation function */
	/* Below formula could be changed for each panel */
	void (*gen_hbm_interpolation_gamma)(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size);
	void (*gen_hbm_interpolation_irc)(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *hbml_table, int hbm_table_size);
	void (*gen_normal_interpolation_irc)(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size);
	int (*get_gamma_V_size)(void);
	void (*convert_GAMMA_to_V)(unsigned char* src, unsigned int *dst);
	void (*convert_V_to_GAMMA)(unsigned int * src, unsigned char *dst);

	int (*samsung_vrr_set_te_mod)(struct samsung_display_driver_data *vdd, int cur_rr, int cur_hs);
	int (*samsung_lfd_get_base_val)(struct vrr_info *vrr,
			enum LFD_SCOPE_ID scope, struct lfd_base_str *lfd_base);

	/* check current mode (VRR, DMS, or etc..) to support tests (MCD, GCT, or etc..) */
	bool (*samsung_check_support_mode)(struct samsung_display_driver_data *vdd,
			enum CHECK_SUPPORT_MODE mode);

	/*
		For video panel :
		dfps operation related panel update func.
	*/
	void (*samsung_dfps_panel_update)(struct samsung_display_driver_data *vdd, int fps);

	/* Dynamic MIPI Clock */
	int (*samsung_dyn_mipi_pre)(struct samsung_display_driver_data *vdd);
	int (*samsung_dyn_mipi_post)(struct samsung_display_driver_data *vdd);
};

enum SS_VBIAS_MODE {
	VBIAS_MODE_NS_WARM = 0,
	VBIAS_MODE_NS_COOL,
	VBIAS_MODE_NS_COLD,
	VBIAS_MODE_HS_WARM,
	VBIAS_MODE_HS_COOL,
	VBIAS_MODE_HS_COLD,
	VBIAS_MODE_MAX
};

enum PHY_STRENGTH_SETTING {
	SS_PHY_CMN_VREG_CTRL_0 = 0,
	SS_PHY_CMN_CTRL_2,
	SS_PHY_CMN_GLBL_RESCODE_OFFSET_TOP_CTRL,
	SS_PHY_CMN_GLBL_RESCODE_OFFSET_BOT_CTRL,
	SS_PHY_CMN_GLBL_RESCODE_OFFSET_MID_CTRL,
	SS_PHY_CMN_GLBL_STR_SWI_CAL_SEL_CTRL,
	SS_PHY_CMN_MAX,
};

/* DDI flash buffer for HOP display */
struct flash_gm2 {
	bool flash_gm2_init_done;

	u8 *ddi_flash_raw_buf;
	int ddi_flash_raw_len;
	int ddi_flash_start_addr;

	u8 *vbias_data;	/* 2 modes (NS/HS), 3 temperature modes, 22 bytes cmd */

	int off_ns_temp_warm;	/* NS mode, T > 0 */
	int off_ns_temp_cool;	/* NS mode, -15 < T <= 0 */
	int off_ns_temp_cold;	/* NS mode, T <= -15 */
	int off_hs_temp_warm;	/* HS mode, T > 0 */
	int off_hs_temp_cool;	/* HS mode, -15 < T <= 0 */
	int off_hs_temp_cold;	/* HS mode, T <= -15 */

	int off_gm2_flash_checksum;

	u8 *mtp_one_vbias_mode;

	int len_one_vbias_mode;
	int len_mtp_one_vbias_mode;

	u16 checksum_tot_flash;
	u16 checksum_tot_cal;
	u16 checksum_one_mode_flash;
	u16 checksum_one_mode_mtp;
	int is_diff_one_vbias_mode;
	int is_flash_checksum_ok;
};

/* used for gamma compensation for 48/96hz VRR mode in gamma mode2 */
struct mtp_gm2_info  {
	bool mtp_gm2_init_done;
	u8 *gamma_org; /* original MTP gamma value */
	u8 *gamma_comp; /* compensated gamma value (for 48/96hz mode) */
};

struct ss_brightness_info {
	int temperature;
	int lux;			// current lux via sysfs
	int acl_status;

	/* TODO: rm below flags.. instead, use the other exist values...
	 * after move init seq. (read ddi id, hbm, etc..) to probe timing,
	 * like gamma flash.
	 */
	int hbm_loaded_dsi;
	int elvss_loaded_dsi;
	int irc_loaded_dsi;

	/* graudal acl: config_adaptiveControlValues in config_display.xml
	 * has step values, like 0, 1, 2, 3, 4, and 5.
	 * In case of gallery app, PMS sets <5 ~ 0> via adaptive_control sysfs.
	 * gradual_acl_val has the value, 0 ~ 5, means acl percentage step,
	 * then ss_acl_on() will set appropriate acl cmd value.
	 */
	int gradual_acl_val;

	/* TFT BL DCS Scaled level*/
	int scaled_level;

	/* TFT LCD Features*/
	int (*backlight_tft_config)(struct samsung_display_driver_data *vdd, int enable);
	void (*backlight_tft_pwm_control)(struct samsung_display_driver_data *vdd, int bl_lvl);
	void (*ss_panel_tft_outdoormode_update)(struct samsung_display_driver_data *vdd);

	struct candela_map_table candela_map_table[CD_MAP_TABLE_MAX][SUPPORT_PANEL_REVISION];
	struct cmd_map aid_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map vint_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map acl_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map elvss_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map smart_acl_elvss_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map caps_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map hmt_reverse_aid_map_table[SUPPORT_PANEL_REVISION];

	/*
	 *  AOR & IRC Interpolation feature
	 */
	struct workqueue_struct *flash_br_workqueue;
	struct delayed_work flash_br_work;
	struct brightness_data_info panel_br_info;
	int table_interpolation_loaded;

	int hbm_brightness_step;
	int normal_brightness_step;
	int hmd_brightness_step;

	/*
	 * Common brightness table
	 */
	struct brightness_info common_br;

	/*
	 * Flash gamma feature
	 */
	bool support_early_gamma_flash;	/* prepare gamma flash at display driver probe timing */
	bool flash_gamma_support;
	char flash_gamma_type[10];
	char flash_read_intf[10];

	int flash_gamma_force_update;
	int flash_gamma_init_done; 	/* To check reading operation done */
	bool flash_gamma_sysfs;		/* work from sysfs */

	int gamma_size;
	int aor_size;
	int vint_size;
	int elvss_size;
	int irc_size;
	int dbv_size;

	/* DDI flash buffer for HOP display */
	struct flash_gm2 gm2_table;
	/* used for gamma compensation for 48/96hz VRR mode in gamma mode2 */
	struct mtp_gm2_info gm2_mtp;

	/* In VRR, each refresh rate mode has its own brightness table includes flash gamma (MTP),
	 * respectively.
	 * Support multiple brightness table for VRR.
	 * ex) br_tbl[0]: for refresh rate 60Hz mode.
	 *     br_tbl[1]: for refresh rate 120Hz mode.
	 */
	int br_tbl_count;		/* needed br tble */
	struct brightness_table *br_tbl;
	int br_tbl_flash_cnt;	/* count of real flash tble */

	int force_use_table_for_hmd_done;

	int smart_dimming_loaded_dsi;
	int smart_dimming_hmt_loaded_dsi;
	struct hmt_status hmt_stat;
};

enum SS_BRR_MODE {
	BRR_48_60 = 0,	/* 48 - 52 - 56 - 60 */
	BRR_60_48,
	BRR_96_120,	/* 96 - 104 - 112 - 120 */
	BRR_120_96,
	BRR_60HS_120,	/* 60 - 70 - 80 - 100 - 110 - 120 */
	BRR_120_60HS,
	BRR_60HS_96,	/* 60 - 70 - 80 - 100 - 110 - 120HS - 112 - 104 - 96 */
	BRR_96_60HS,
	MAX_BRR_ON_MODE,
	BRR_STOP_MODE,	/* stop BRR and jump to final target RR mode, in case of brightness change */
	BRR_OFF_MODE,
	MAX_ALL_BRR_MODE,
};

enum HS_NM_SEQ {
	HS_NM_OFF = 0,
	HS_NM_MULTI_RES_FIRST,
	HS_NM_VRR_FIRST,
};

struct vrr_bridge_rr_tbl {
	int tot_steps;
	int *fps;

	int fps_base;
	int fps_start; /* used for gamma/aor interpolation during bridge RR */
	int fps_end; /* used for gamma/aor interpolation during bridge RR */
	int vbp_base;
	int vactive_base;
	int vfp_base;
	bool sot_hs_base;

	/* allowed brightness [candela]
	 * ex) HAB:
	 * 48NM/96NM BRR: 11~420nit
	 * 96HS/120HS BRR: 11~420nit
	 * 60HS/120HS BRR: 98~420nit
	 */
	int min_cd;
	int max_cd;

	/* In every BRR steps, it stays for delay_frame_cnt * TE_period
	 * ex) In HAB DDI,
	 * 48/60, 96/120, 60->120: 4 frames
	 * 120->60: 8 frames
	 */
	int delay_frame_cnt;
};

enum VRR_BLACK_FRAME_MODE {
	BLACK_FRAME_OFF = 0,
	BLACK_FRAME_INSERT,
	BLACK_FRAME_REMOVE,
	BLACK_FRAME_MAX,
};

enum VRR_LFD_FAC_MODE {
	VRR_LFD_FAC_LFDON = 0,	/* dynamic 120hz <-> 10hz in HS mode, or 60hz <-> 30hz in NS mode */
	VRR_LFD_FAC_HIGH = 1,	/* always 120hz in HS mode, or 60hz in NS mode */
	VRR_LFD_FAC_LOW = 2,	/* set LFD min and max freq. to lowest value (HS: 10hz, NS: 30hz).
				 * In image update case, freq. will reach to 120hz, but it will
				 * ignore bridge step to enter LFD mode (ignore frame insertion setting)
				 */
	VRR_LFD_FAC_MAX,
};

enum VRR_LFD_AOD_MODE {
	VRR_LFD_AOD_LFDON = 0,	/* image update case: 30hz, LFD case: 1hz */
	VRR_LFD_AOD_LFDOFF = 1,	/* image update case: 30hz, LFD case: 30hz */
	VRR_LFD_AOD_MAX,
};

enum VRR_LFD_TSPLPM_MODE {
	VRR_LFD_TSPLPM_LFDON = 0,
	VRR_LFD_TSPLPM_LFDOFF = 1,
	VRR_LFD_TSPLPM_MAX,
};

enum VRR_LFD_INPUT_MODE {
	VRR_LFD_INPUT_LFDON = 0,
	VRR_LFD_INPUT_LFDOFF = 1,
	VRR_LFD_INPUT_MAX,
};

enum VRR_LFD_DISP_MODE {
	VRR_LFD_DISP_LFDON = 0,
	VRR_LFD_DISP_LFDOFF = 1, /* low brightness case */
	VRR_LFD_DISP_HBM_1HZ = 2,
	VRR_LFD_DISP_MAX,
};

enum LFD_CLIENT_ID {
	LFD_CLIENT_FAC = 0,
	LFD_CLIENT_DISP,
	LFD_CLIENT_INPUT,
	LFD_CLIENT_AOD,
	LFD_CLIENT_VID,
	LFD_CLIENT_HMD,
	LFD_CLIENT_MAX
};

enum LFD_SCOPE_ID {
	LFD_SCOPE_NORMAL = 0,
	LFD_SCOPE_LPM,
	LFD_SCOPE_HMD,
	LFD_SCOPE_MAX,
};

enum LFD_FUNC_FIX {
	LFD_FUNC_FIX_OFF = 0,
	LFD_FUNC_FIX_HIGH = 1,
	LFD_FUNC_FIX_LOW = 2,
	LFD_FUNC_FIX_MAX
};

enum LFD_FUNC_SCALABILITY {
	LFD_FUNC_SCALABILITY0 = 0,	/* 40lux ~ 7400lux, defealt, div=11 */
	LFD_FUNC_SCALABILITY1 = 1,	/* under 40lux, LFD off, div=1 */
	LFD_FUNC_SCALABILITY2 = 2,	/* touch press/release, div=2 */
	LFD_FUNC_SCALABILITY3 = 3,	/* TBD, div=4 */
	LFD_FUNC_SCALABILITY4 = 4,	/* TBD, div=5 */
	LFD_FUNC_SCALABILITY5 = 5,	/* 40lux ~ 7400lux, same set with LFD_FUNC_SCALABILITY0, div=11 */
	LFD_FUNC_SCALABILITY6 = 6,	/* over 7400lux (HBM), div=120 */
	LFD_FUNC_SCALABILITY_MAX
};

#define LFD_FUNC_MIN_CLEAR	0
#define LFD_FUNC_MAX_CLEAR	0

struct lfd_base_str {
	u32 max_div_def;
	u32 min_div_def;
	u32 min_div_lowest;
	int base_rr;
};

struct lfd_mngr {
	enum LFD_FUNC_FIX fix[LFD_SCOPE_MAX];
	enum LFD_FUNC_SCALABILITY scalability[LFD_SCOPE_MAX];
	u32 min[LFD_SCOPE_MAX];
	u32 max[LFD_SCOPE_MAX];
};

struct lfd_info {
	struct lfd_mngr lfd_mngr[LFD_CLIENT_MAX];

	struct notifier_block nb_lfd_touch;
	struct workqueue_struct *lfd_touch_wq;
	struct work_struct lfd_touch_work;

	bool support_lfd; /* Low Frequency Driving mode for HOP display */

	u32 min_div; /* max_div_def ~ min_div_def, or min_div_lowest */
	u32 max_div; /* max_div_def ~ min_div_def */
	u32 base_rr;
};

enum VRR_GM2_GAMMA {
	VRR_GM2_GAMMA_NO_ACTION = 0,
	VRR_GM2_GAMMA_COMPENSATE,
	VRR_GM2_GAMMA_RESTORE_ORG,
};

struct vrr_info {
	/* LFD information and mangager */
	struct lfd_info lfd;

	/* ss_panel_vrr_switch() is main VRR function.
	 * It can be called in multiple threads, display thread
	 * and kworker thread.
	 * To guarantee exclusive VRR processing, use vrr_lock.
	 */
	struct mutex vrr_lock;
	/* protect brr_mode, lock the code changing brr_mode value */
	/* TODO: brr_lock causes frame drop and screen noise...
	 * fix this issue then comment out brr_lock...
	 */
	struct mutex brr_lock;

	/* - samsung,support_vrr_based_bl : Samsung concept.
	 *   Change VRR mode in brightness update due to brithness compensation for VRR change.
	 * - No samsung,support_vrr_based_bl : Qcomm original concept.
	 *   Change VRR mode in dsi_panel_switch() by sending DSI_CMD_SET_TIMING_SWITCH command.
	 */
	bool support_vrr_based_bl;

	bool is_support_brr;
	bool is_support_black_frame;

	/* MDP set SDE_RSC_CMD_STATE for inter-frame power collapse, which refer to TE.
	 *
	 * In case of SS VRR change (120HS -> 60HS), it changes VRR mode
	 * in other work thread, not it commit thread.
	 * In result, MDP works at 60hz, while panel is working at 120hz yet, for a while.
	 * It causes higher TE period than MDP expected, so it lost wr_ptr irq,
	 * and causes delayed pp_done irq and frame drop.
	 *
	 * To prevent this
	 * During VRR change, set RSC fps to maximum 120hz, to wake up RSC most frequently,
	 * not to lost wr_ptr irq.
	 * After finish VRR change, recover original RSC fps, and rsc timeout value.
	 */
	bool keep_max_rsc_fps;

	bool running_vrr_mdp; /* MDP get VRR request ~ VRR work start */
	bool running_vrr; /* VRR work start ~ end */

	/* QCT use DMS instead of VRR feature for command mdoe panel.
	 * is_vrr_changing flag shows it is VRR chaging mode while DMS flag is set,
	 * and be used for samsung display driver.
	 */
	bool is_vrr_changing;
	bool is_multi_resolution_changing;

	u32 cur_h_active;
	u32 cur_v_active;
	/*
	 * - cur_refresh_rate should be set right before brightness setting (ss_brightness_dcs)
	 * - cur_refresh_rate could not be changed during brightness setting (ss_brightness_dcs)
	 *   Best way is that guarantees to keep cur_refresh_rate during brightness setting,
	 *   But it requires lock (like bl_lock), and it can cause dead lock with brr_lock,
	 *   and frame drop...
	 *   cur_refresh_rate is changed in ss_panel_vrr_switch(), and it sets brightness setting
	 *   with final target refresh rate, so there is no problem.
	 */
	int cur_refresh_rate;
	bool cur_sot_hs_mode; /* Scan of Time HS mode for VRR */

	/* Some displays, like hubble HAB and c2 HAC, cannot support 120hz in WQHD.
	 * In this case, it should guarantee below sequence to prevent WQHD@120hz mode,
	 * in case of WQHD@60hz -> FHD/HD@120hz.
	 * 1) WQHD@60Hz -> FHD/HD@60hz by sending DMS command and  one image frame
	 * 2) FHD/HD@60hz -> FHD/HD@120hz by sendng VRR commands
	 */
	int max_h_active_support_120hs;

	u32 adjusted_h_active;
	u32 adjusted_v_active;
	int adjusted_refresh_rate;
	bool adjusted_sot_hs_mode; /* Scan of Time HS mode for VRR */

	/* bridge refresh rate */
	enum SS_BRR_MODE brr_mode;
	bool brr_rewind_on; /* rewind BRR in case of new VRR set back to BRR start FPS */
	int brr_bl_level;	/* brightness level causing BRR stopa */

	struct workqueue_struct *vrr_workqueue;
	struct work_struct vrr_work;

	struct vrr_bridge_rr_tbl brr_tbl[MAX_BRR_ON_MODE];

	enum HS_NM_SEQ hs_nm_seq; // todo: use enum...

	ktime_t time_vrr_start;

	enum VRR_BLACK_FRAME_MODE black_frame_mode;

	/* Before VRR start, GFX set MDP performance mode to avoid frame drop.
	 * After VRR end, GFX restore MDP mode to normal mode.
	 * delayed_perf_normal delays set back to normal mode until BRR end.
	 */
	bool is_support_delayed_perf;
	bool delayed_perf_normal;

	/* skip VRR setting in brightness command (ss_vrr() funciton)
	 * 1) start waiting SSD improve and HS_NM change
	 * 2) triger brightness setting in other thread, and change fps
	 * 3) finish waiting SSD improve and HS_NM change,
	 *    but fps is already changed, and it causes side effect..
	 */
	bool skip_vrr_in_brightness;
	bool param_update;

	/* TE moudulation (reports vsync as 60hz even TE is 120hz, in 60HS mode)
	 * Some HOP display, like C2 HAC UB, supports self scan function.
	 * In this case, if it sets to 60HS mode, panel fetches pixel data from GRAM in 60hz,
	 * but DDI generates TE as 120hz.
	 * This architecture is for preventing brightness flicker in VRR change and keep fast touch responsness.
	 *
	 * In 60HS mode, TE period is 120hz, but display driver reports vsync to graphics HAL as 60hz.
	 * So, do TE modulation in 60HS mode, reports vsync as 60hz.
	 * In 30NS mode, TE is 60hz, but reports vsync as 30hz.
	 */
	bool support_te_mod;
	int te_mod_on;
	int te_mod_divider;
	int te_mod_cnt;

	enum VRR_GM2_GAMMA gm2_gamma;
};

struct panel_vreg {
	struct regulator *vreg;
	char name[32];
	bool ssd;
	u32 min_voltage;
	u32 max_voltage;
	u32 from_off_v;
	u32 from_lpm_v;
};

struct panel_regulators {
	struct panel_vreg *vregs;
	int vreg_count;
};

/*
 * Manage vdd per dsi_panel, respectivley, like below table.
 * Each dsi_display has one dsi_panel and one vdd, respectively.
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * case		physical conneciton	panel dtsi	dsi_dsiplay	dsi_panel	vdd	dsi_ctrl	lcd sysfs	bl sysfs
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * single dsi	1 panel, 1 dsi port	1		1		1		1	1		1		1
 * split mode	1 panel, 2 dsi port	1		1		1		1	2		1		1
 * dual panel	2 panel, 2 dsi port	2		2		2		2	2		2		1
 * hall ic	2 panel, 1 dsi port	2		2		2		2	1		1		1
 * ------------------------------------------------------------------------------------------------------------------------------------------
 */
struct samsung_display_driver_data {

	void *msm_private;

	/* mipi cmd type */
	int cmd_type;

	bool panel_dead;
	int read_panel_status_from_lk;
	bool is_factory_mode;
	bool panel_attach_status; // 1: attached, 0: detached
	int panel_revision;
	char *panel_vendor;

	/* SAMSUNG_FINGERPRINT */
	bool support_optical_fingerprint;
	bool finger_mask_updated;
	int finger_mask;
	int panel_hbm_entry_delay; //hbm entry delay/ unit = vsync
	int panel_hbm_entry_after_te; /* delay after TE noticed */
	int panel_hbm_exit_delay; /* hbm exit delay frame */
	struct lcd_device *lcd_dev;

	struct display_status display_status_dsi;

	struct mutex vdd_lock;
	struct mutex cmd_lock;
	struct mutex bl_lock;
	struct mutex ss_spi_lock;

	struct samsung_display_debug_data *debug_data;
	struct ss_exclusive_mipi_tx exclusive_tx;
	struct list_head vdd_list;

	int siop_status;

	struct panel_func panel_func;

	// parsed data from dtsi
	struct samsung_display_dtsi_data dtsi_data;

	/*
	 *  Panel read data
	 */
	int manufacture_id_dsi;
	int module_info_loaded_dsi;
	int manufacture_date_loaded_dsi;

	int manufacture_date_dsi;
	int manufacture_time_dsi;

	int ddi_id_loaded_dsi;
	int ddi_id_dsi[MAX_CHIP_ID];

	int cell_id_loaded_dsi;		/* Panel Unique Cell ID */
	int cell_id_dsi[MAX_CELL_ID];	/* white coordinate + manufacture date */

	int octa_id_loaded_dsi;		/* Panel Unique OCTA ID */
	u8 octa_id_dsi[MAX_OCTA_ID];

	/* Cover Control Status */
	int cover_control;

	int select_panel_gpio;
	bool select_panel_use_expander_gpio;

	/* X-Talk */
	int xtalk_mode;

	/* Reset Time check */
	s64 reset_time_64;

	/* Sleep_out cmd time check */
	s64 sleep_out_time_64;
	ktime_t sleep_out_time;
	ktime_t tx_set_on_time;

	/* Some panel read operation should be called after on-command. */
	bool skip_read_on_pre;

	/* Support Global Para */
	int gpara;

	/* Num_of interface get from priv_info->topology.num_intf */
	int num_of_intf;

	/*
	 *  panel pwr state
	 */
	enum ss_panel_pwr_state panel_state;

	/* ndx means display ID
	 * ndx = 0: primary display
	 * ndx = 1: secondary display
	 */
	enum ss_display_ndx ndx;

	/*
	 *  register dump info
	 */
	struct samsung_register_dump_info dump_info[MAX_INTF_NUM];

	/*
	 *  OSC TE fitting info
	 */
	struct osc_te_fitting_info te_fitting_info;

	/*
	 *  FTF
	 */
	/* CABC feature */
	int support_cabc;

	/* LP RX timeout recovery */
	bool support_lp_rx_err_recovery;

	/*
	 *  ESD
	 */
	struct esd_recovery esd_recovery;

	/* FG_ERR */
	int fg_err_gpio;

	/* Panel ESD Recovery Count */
	int panel_recovery_cnt;

	/*
	 *  Image dump
	 */
	struct workqueue_struct *image_dump_workqueue;
	struct work_struct image_dump_work;

	/*
	 *  Other line panel support
	 */
	struct workqueue_struct *other_line_panel_support_workq;
	struct delayed_work other_line_panel_support_work;
	int other_line_panel_work_cnt;

	/*
	 *  LPM
	 */
	struct lpm_info panel_lpm;

	/*
	 *  Big Data
	 */
	struct notifier_block dpui_notif;
	struct notifier_block dpci_notif;
	u64 dsi_errors; /* report dpci bigdata */

	/*
	 *  COPR
	 */
	struct COPR copr;
	int copr_load_init_cmd;

	/*
	 *  SPI
	 */
	bool samsung_support_ddi_spi;
	struct spi_device *spi_dev;
	struct spi_driver spi_driver;
	struct notifier_block spi_notif;
	int ddi_spi_status;

	/* flash spi cmd set */
	struct ddi_spi_cmd_set *spi_cmd_set;

	/*
		unique ddi need to sustain spi-cs(chip select)port high-level for global parameter usecase..

		spi_sync -> spi gpio suspend(ex qupv3_se7_spi_sleep) -> set ddi_spi_cs_high_gpio_for_gpara high ->
		use global paremeter cmds -> set ddi_spi_cs_high_gpio_for_gpara low
	*/
	int ddi_spi_cs_high_gpio_for_gpara;

	/*
	 *  POC
	 */
	struct POC poc_driver;

	/*  FirmWare Update */
	struct FW_UP fw_up;

	/*
	 *  Dynamic MIPI Clock
	 */
	struct dyn_mipi_clk dyn_mipi_clk;

	/*
	 *  GCT
	 */
	struct gram_checksum_test gct;

	/*
	 *  hall ic
	 */
	bool support_hall_ic;
	int hall_ic_status;
	int hall_ic_mode_change_trigger;
	bool hall_ic_status_pending;
	bool hall_ic_status_unhandled;
	struct notifier_block hall_ic_notifier_display;
	bool lcd_flip_not_refresh;
	u32 lcd_flip_delay_ms;
	struct delayed_work delay_disp_on_work;

	/*
	 *  smmu debug(sde & rotator)
	 */
	struct ss_smmu_debug ss_debug_smmu[SMMU_MAX_DEBUG];
	struct kmem_cache *ss_debug_smmu_cache;

	/*
	 *  SELF DISPLAY
	 */
	struct self_display self_disp;

	/* MAFPC */
	struct MAFPC mafpc;

	/*
	 * Samsung brightness information for smart dimming
	 */
	struct ss_brightness_info br_info;

	/*
	 * mDNIe
	 */
	struct mdnie_info mdnie;
	int mdnie_loaded_dsi;

	/*
	 * STN
	 */
	struct STM stm;
	int stm_load_init_cmd;

	/*
	 * grayspot test
	 */
	int grayspot;

	/*
	 * CCD fail value
	 */
	int ccd_pass_val;
	int ccd_fail_val;

	int samsung_splash_enabled;

	/* UB CON DETECT */
	struct ub_con_detect ub_con_det;

	/* VRR (Variable Refresh Rate) */
	struct vrr_info vrr;

	/* Motto phy tune */
	struct device *motto_device;
	struct motto_data motto_info;

	/* panel notifier work */
	struct workqueue_struct *notify_wq;
	struct work_struct bl_event_work;
	struct work_struct vrr_event_work;
	struct work_struct lfd_event_work;
	struct work_struct panel_state_event_work;

	enum panel_notifier_event_t ss_notify_event;
	struct mutex notify_lock;
	bool is_autorefresh_fail;

	/* window color to make different br table */
	char window_color[2];
	bool support_window_color;

	struct panel_regulators panel_regulator;

	/* need additional mipi support */
	bool mipi_header_modi;
	bool need_brightness_lock;
	bool no_qcom_pps;

	/* Single TX  */
	bool not_support_single_tx;

	/*
		AOT support : tddi video panel
		To keep high status panel power & reset
	*/
	bool aot_enable;

	int check_fw_id; 	/* save ddi_fw id (revision)*/
	bool is_recovery_mode;

	/* phy strength setting bit field */
	DECLARE_BITMAP(ss_phy_ctrl_bit, SS_PHY_CMN_MAX);
	uint32_t ss_phy_ctrl_data[SS_PHY_CMN_MAX];

	/* sustain LP11 signal before LP00 on entering sleep status */
	int lp11_sleep_ms_time;

	/* Bloom5G need old style aor dimming : using both A-dimming & S-dimming */
	bool old_aor_dimming;

	/* BIT0: brightdot test, BIT1: brightdot test in LFD 0.5hz */
	u32 brightdot_state;

	/* Some panel has unstable TE period, and causes wr_ptr timeout panic
	 * in inter-frame RSC idle policy.
	 * W/A: select four frame RSC idle policy.
	 */
	bool rsc_4_frame_idle;

	/* flag to support reading module id at probe timing */
	bool support_early_id_read;

	/* mdp clock underflow */
	int cnt_mdp_clk_underflow;
};

extern struct list_head vdds_list;

extern char *lfd_client_name[LFD_CLIENT_MAX];
extern char *lfd_scope_name[LFD_SCOPE_MAX];

/* COMMON FUNCTION */
void ss_panel_init(struct dsi_panel *panel);

/* VRR */
int ss_panel_dms_switch(struct samsung_display_driver_data *vdd);
void ss_panel_vrr_switch_work(struct work_struct *work);
int ss_get_lfd_div(struct samsung_display_driver_data *vdd, enum LFD_SCOPE_ID scope,
			u32 *out_min_div, u32 *out_max_div);

//void ss_dsi_panel_registered(struct dsi_panel *pdata);
void ss_set_max_cpufreq(struct samsung_display_driver_data *vdd,
		int enable, enum mdss_cpufreq_cluster cluster);
void ss_set_max_mem_bw(struct samsung_display_driver_data *vdd, int enable);
void ss_set_exclusive_tx_packet(
		struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type cmd, int pass);
void ss_set_exclusive_tx_lock_from_qct(struct samsung_display_driver_data *vdd, bool lock);
int ss_send_cmd(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type cmd);
int ss_write_ddi_ram(struct samsung_display_driver_data *vdd,
				int target, u8 *buffer, int len);
int ss_panel_on_pre(struct samsung_display_driver_data *vdd);
int ss_panel_on_post(struct samsung_display_driver_data *vdd);
int ss_panel_off_pre(struct samsung_display_driver_data *vdd);
int ss_panel_off_post(struct samsung_display_driver_data *vdd);
//int ss_panel_extra_power(struct dsi_panel *pdata, int enable);
int ss_backlight_tft_gpio_config(struct samsung_display_driver_data *vdd, int enable);
int ss_backlight_tft_request_gpios(struct samsung_display_driver_data *vdd);
int ss_panel_data_read(struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type type, u8 *buffer, int level_key);
void ss_panel_low_power_config(struct samsung_display_driver_data *vdd, int enable);

/*
 * Check lcd attached status for DISPLAY_1 or DISPLAY_2
 * if the lcd was not attached, the function will return 0
 */
int ss_panel_attached(int ndx);
int get_lcd_attached(char *mode);
int get_lcd_attached_secondary(char *mode);
int ss_panel_attached(int ndx);

struct samsung_display_driver_data *check_valid_ctrl(struct dsi_panel *panel);

char ss_panel_id0_get(struct samsung_display_driver_data *vdd);
char ss_panel_id1_get(struct samsung_display_driver_data *vdd);
char ss_panel_id2_get(struct samsung_display_driver_data *vdd);
char ss_panel_rev_get(struct samsung_display_driver_data *vdd);

int ss_panel_attach_get(struct samsung_display_driver_data *vdd);
int ss_panel_attach_set(struct samsung_display_driver_data *vdd, bool attach);

int ss_read_loading_detection(void);

/* EXTERN FUNCTION */
#ifdef CONFIG_FOLDER_HALL
extern void hall_ic_register_notify(struct notifier_block *nb);
#endif

/* EXTERN VARIABLE */
extern struct dsi_status_data *pstatus_data;

/* HMT FUNCTION */
int hmt_enable(struct samsung_display_driver_data *vdd);
int hmt_reverse_update(struct samsung_display_driver_data *vdd, int enable);

/* HALL IC FUNCTION */
int samsung_display_hall_ic_status(struct notifier_block *nb,
		unsigned long hall_ic, void *data);

/* CORP CALC */
void ss_copr_calc_work(struct work_struct *work);
void ss_copr_calc_delayed_work(struct delayed_work *work);

/* PANEL LPM FUNCTION */
int ss_find_reg_offset(int (*reg_list)[2],
		struct dsi_panel_cmd_set *cmd_list[], int list_size);
void ss_panel_lpm_ctrl(struct samsung_display_driver_data *vdd, int enable);
int ss_panel_lpm_power_ctrl(struct samsung_display_driver_data *vdd, int enable);

/* Dynamic MIPI Clock FUNCTION */
int ss_find_dyn_mipi_clk_timing_idx(struct samsung_display_driver_data *vdd);
int ss_change_dyn_mipi_clk_timing(struct samsung_display_driver_data *vdd);
int ss_dyn_mipi_clk_tx_ffc(struct samsung_display_driver_data *vdd);

/* read/write mtp */
void ss_read_mtp(struct samsung_display_driver_data *vdd, int addr, int len, int pos, u8 *buf);
void ss_write_mtp(struct samsung_display_driver_data *vdd, int len, u8 *buf);

/* Other line panel support */
#define MAX_READ_LINE_SIZE 256
int read_line(char *src, char *buf, int *pos, int len);

/* STM */
int ss_stm_set_cmd_offset(struct STM_CMD *cmd, char* p);
void print_stm_cmd(struct STM_CMD cmd);
int ss_get_stm_orig_cmd(struct samsung_display_driver_data *vdd);
void ss_stm_set_cmd(struct samsung_display_driver_data *vdd, struct STM_CMD *cmd);

/* uevent for UB con */
void ss_send_ub_uevent(struct samsung_display_driver_data *vdd);

void ss_set_panel_state(struct samsung_display_driver_data *vdd, enum ss_panel_pwr_state panel_state);

int ss_early_display_init(struct samsung_display_driver_data *vdd);

void ss_notify_queue_work(struct samsung_display_driver_data *vdd,
	enum panel_notifier_event_t event);

int ss_panel_power_ctrl(struct samsung_display_driver_data *vdd, bool enable);
int ss_panel_regulator_short_detection(struct samsung_display_driver_data *vdd, enum panel_state state);

/***************************************************************************************************
*		BRIGHTNESS RELATED.
****************************************************************************************************/

#define DEFAULT_BRIGHTNESS 255
#define DEFAULT_BRIGHTNESS_PAC3 25500

#define BRIGHTNESS_MAX_PACKET 100
#define HBM_MODE 6

/* BRIGHTNESS RELATED FUNCTION */
int ss_brightness_dcs(struct samsung_display_driver_data *vdd, int level, int backlight_origin);
void ss_brightness_tft_pwm(struct samsung_display_driver_data *vdd, int level);
void update_packet_level_key_enable(struct samsung_display_driver_data *vdd,
		struct dsi_cmd_desc *packet, int *cmd_cnt, int level_key);
void update_packet_level_key_disable(struct samsung_display_driver_data *vdd,
		struct dsi_cmd_desc *packet, int *cmd_cnt, int level_key);
int ss_single_transmission_packet(struct dsi_panel_cmd_set *cmds);

int ss_set_backlight(struct samsung_display_driver_data *vdd, u32 bl_lvl);
bool is_hbm_level(struct samsung_display_driver_data *vdd);

/* SAMSUNG_FINGERPRINT */
void ss_send_hbm_fingermask_image_tx(struct samsung_display_driver_data *vdd, bool on);

/* HMT BRIGHTNESS */
int ss_brightness_dcs_hmt(struct samsung_display_driver_data *vdd, int level);
int hmt_bright_update(struct samsung_display_driver_data *vdd);

/* TFT BL DCS RELATED FUNCTION */
int get_scaled_level(struct samsung_display_driver_data *vdd, int ndx);
void ss_tft_autobrightness_cabc_update(struct samsung_display_driver_data *vdd);


void set_up_interpolation(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl,
		struct ss_interpolation_brightness_table *normal_table, int normal_table_size,
		struct ss_interpolation_brightness_table *hbm_table, int hbm_table_size);

void debug_interpolation_log(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl);

void table_br_func(struct samsung_display_driver_data *vdd, struct brightness_table *br_tbl); /* For table data */
void debug_br_info_log(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl);

void set_up_br_info(struct samsung_display_driver_data *vdd,
		struct brightness_table *br_tbl);

void ss_set_vrr_switch(struct samsung_display_driver_data *vdd, bool param_update);
bool ss_is_brr_on(enum SS_BRR_MODE brr_mode);
bool ss_is_sot_hs_from_drm_mode(const struct drm_display_mode *drm_mode);
int ub_con_det_status(int index);

/***************************************************************************************************
*		BRIGHTNESS RELATED END.
****************************************************************************************************/


/* SAMSUNG COMMON HEADER*/
#include "ss_dsi_smart_dimming_common.h"
/* MDNIE_LITE_COMMON_HEADER */
#include "ss_dsi_mdnie_lite_common.h"
/* SAMSUNG MODEL HEADER */




/* Interface betwenn samsung and msm display driver
 */

extern struct dsi_display_boot_param boot_displays[MAX_DSI_ACTIVE_DISPLAY];
extern char dsi_display_primary[MAX_CMDLINE_PARAM_LEN];
extern char dsi_display_secondary[MAX_CMDLINE_PARAM_LEN];

static inline void ss_get_primary_panel_name_cmdline(char *panel_name)
{
	char *pos = NULL;
	size_t len;

	pos = strnstr(dsi_display_primary, ":", sizeof(dsi_display_primary));
	if (!pos)
		return;

	len = (size_t) (pos - dsi_display_primary) + 1;

	strlcpy(panel_name, dsi_display_primary, len);
}

static inline void ss_get_secondary_panel_name_cmdline(char *panel_name)
{
	char *pos = NULL;
	size_t len;

	pos = strnstr(dsi_display_secondary, ":", sizeof(dsi_display_secondary));
	if (!pos)
		return;

	len = (size_t) (pos - dsi_display_secondary) + 1;

	strlcpy(panel_name, dsi_display_secondary, len);
}

struct samsung_display_driver_data *ss_get_vdd(enum ss_display_ndx ndx);

#define GET_DSI_PANEL(vdd)	((struct dsi_panel *) (vdd)->msm_private)
static inline struct dsi_display *GET_DSI_DISPLAY(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_display *display = dev_get_drvdata(panel->parent);
	return display;
}

static inline struct drm_device *GET_DRM_DEV(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;

	return ddev;
}

// refer to msm_drm_init()
static inline struct msm_kms *GET_MSM_KMS(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct msm_drm_private *priv = ddev->dev_private;

	return priv->kms;
}
#define GET_SDE_KMS(vdd)	to_sde_kms(GET_MSM_KMS(vdd))

static inline struct drm_crtc *GET_DRM_CRTC(
		struct samsung_display_driver_data *vdd)
{
#if 1
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct msm_drm_private *priv = ddev->dev_private;
	int id;

	// TODO: get correct crtc_id ...
	id = 0; // crtc id: 0 = primary display, 1 = wb...

	return priv->crtcs[id];
#else
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct list_head *crtc_list;
	struct drm_crtc *crtc = NULL, *crtc_iter;
	struct sde_crtc *sde_crtc;

	// refer to _sde_kms_setup_displays()
	crtc_list = &ddev->mode_config.crtc_list;
	list_for_each_entry(crtc_iter, crtc_list, head) {
		sde_crtc = to_sde_crtc(crtc_iter);
		if (sde_crtc->display == display) {
			crtc = crtc_iter;
			break;
		}
	}

	if (!crtc)
		LCD_ERR("failed to find drm_connector\n");

	return crtc;

#endif

}

static inline struct drm_encoder *GET_DRM_ENCODER(
		struct samsung_display_driver_data *vdd)
{
#if 1
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct msm_drm_private *priv = ddev->dev_private;
	int id;

	// TODO: get correct encoder id ...
	id = 0; // crtc id: 0 = primary display, 1 = wb...

	return priv->encoders[id];
#else
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct list_head *connector_list;
	struct drm_connector *conn = NULL, *conn_iter;
	struct sde_connector *c_conn;

	// refer to _sde_kms_setup_displays()
	connector_list = &ddev->mode_config.connector_list;
	list_for_each_entry(conn_iter, connector_list, head) {
		c_conn = to_sde_connector(conn_iter);
		if (c_conn->display == display) {
			conn = conn_iter;
			break;
		}
	}

	if (!conn)
		LCD_ERR("failed to find drm_connector\n");

	return conn;

#endif
}

static inline struct drm_connector *GET_DRM_CONNECTORS(
		struct samsung_display_driver_data *vdd)
{
#if 0
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct msm_drm_private *priv = ddev->dev_private;
	int id;

	// TODO: get connector id ...
	id = 0;

	return priv->connectors[id];
#else
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct list_head *connector_list;
	struct drm_connector *conn = NULL, *conn_iter;
	struct sde_connector *c_conn;

	// refer to _sde_kms_setup_displays()
	connector_list = &ddev->mode_config.connector_list;
	list_for_each_entry(conn_iter, connector_list, head) {
		c_conn = to_sde_connector(conn_iter);
		if (c_conn->display == display) {
			conn = conn_iter;
			break;
		}
	}

	if (!conn)
		LCD_ERR("failed to find drm_connector\n");

	return conn;
#endif
}

#define GET_SDE_CONNECTOR(vdd)	to_sde_connector(GET_DRM_CONNECTORS(vdd))

//#define GET_VDD_FROM_SDE_CONNECTOR(c_conn)

static inline struct backlight_device *GET_SDE_BACKLIGHT_DEVICE(
		struct samsung_display_driver_data *vdd)
{
	struct backlight_device *bd = NULL;
	struct sde_connector *conn = NULL;

	if (IS_ERR_OR_NULL(vdd))
		goto end;

	conn = GET_SDE_CONNECTOR(vdd);
	if (IS_ERR_OR_NULL(conn))
		goto end;

	bd = conn->bl_device;
	if (IS_ERR_OR_NULL(bd))
		goto end;

end:
	return bd;
}

/* In dual panel, it has two panel, and
 * ndx = 0: primary panel, ndx = 1: secondary panel
 * In one panel with dual dsi port, like split mode,
 * it has only one panel, and ndx = 0.
 * In one panel with single dsi port,
 * it has only one panel, and ndx = 0.
 */
static inline enum ss_display_ndx ss_get_display_ndx(
		struct samsung_display_driver_data *vdd)
{
	return vdd->ndx;
}

static inline void ss_set_display_ndx(struct samsung_display_driver_data *vdd,
		int ndx)
{
	vdd->ndx = ndx;
}

extern struct samsung_display_driver_data vdd_data[MAX_DISPLAY_NDX];

static inline struct samsung_display_driver_data *ss_check_hall_ic_get_vdd(
		struct samsung_display_driver_data *vdd)
{
	enum ss_display_ndx ndx;

	if (!vdd->support_hall_ic)
		return vdd;

/*	if (vdd->hall_ic_status == HALL_IC_OPEN)
		ndx = PRIMARY_DISPLAY_NDX;
	else
		ndx = SECONDARY_DISPLAY_NDX;
	TBD */
	ndx = PRIMARY_DISPLAY_NDX;

	return ss_get_vdd(ndx);
}

static inline const char *ss_get_panel_name(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	return panel->name;
}

/* return panel x resolution */
static inline u32 ss_get_xres(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	/* FC2 change: set panle mode in SurfaceFlinger initialization, instead of kenrel booting... */
	if (!panel->cur_mode) {
		LCD_ERR("err: no panel mode yet...\n");
		return 0;
	}

	return panel->cur_mode->timing.h_active;
}

/* return panel y resolution */
static inline u32 ss_get_yres(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	/* FC2 change: set panle mode in SurfaceFlinger initialization, instead of kenrel booting... */
	if (!panel->cur_mode) {
		LCD_ERR("err: no panel mode yet...\n");
		return 0;
	}
	return panel->cur_mode->timing.v_active;
}

/* check if it is dual dsi port */
static inline bool ss_is_dual_dsi(struct samsung_display_driver_data *vdd)
{
	if (vdd->num_of_intf == 2)
		return true;
	else
		return false;
}

/* check if it is dual dsi port */
static inline bool ss_is_single_dsi(struct samsung_display_driver_data *vdd)
{
	if (vdd->num_of_intf == 1)
		return true;
	else
		return false;
}

/* check if it is command mode panel */
static inline bool ss_is_cmd_mode(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	if (panel->panel_mode == DSI_OP_CMD_MODE)
		return true;
	else
		return false;
}

/* check if it is video mode panel */
static inline bool ss_is_video_mode(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	if (panel->panel_mode == DSI_OP_VIDEO_MODE)
		return true;
	else
		return false;
}
/* check if it controls backlight via MIPI DCS */
static inline bool ss_is_bl_dcs(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	if (panel->bl_config.type == DSI_BACKLIGHT_DCS)
		return true;
	else
		return false;
}

/* check if panel is on */
// blank_state = MDSS_PANEL_BLANK_UNBLANK
static inline bool ss_is_panel_on(struct samsung_display_driver_data *vdd)
{
//	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	// panel_initialized is set to true when panel is on,
	// and is set to fase when panel is off
//	return dsi_panel_initialized(panel);
	return (vdd->panel_state == PANEL_PWR_ON);
}

static inline bool ss_is_panel_on_ready(struct samsung_display_driver_data *vdd)
{
	return (vdd->panel_state == PANEL_PWR_ON_READY);
}

static inline bool ss_is_ready_to_send_cmd(struct samsung_display_driver_data *vdd)
{
	return ((vdd->panel_state == PANEL_PWR_ON) || (vdd->panel_state == PANEL_PWR_LPM));
}

static inline bool ss_is_panel_off(struct samsung_display_driver_data *vdd)
{
//	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	// panel_initialized is set to true when panel is on,
	// and is set to fase when panel is off
//	return dsi_panel_initialized(panel);

	return (vdd->panel_state == PANEL_PWR_OFF);
}

/* check if panel is low power mode */
// blank_state = MDSS_PANEL_BLANK_LOW_POWER
static inline bool ss_is_panel_lpm(
		struct samsung_display_driver_data *vdd)
{

	// TODO: get lpm value from somewhere..
	return (vdd->panel_state == PANEL_PWR_LPM);
}

static inline int ss_is_read_cmd(struct dsi_panel_cmd_set *set)
{
	switch(set->cmds->msg.type) {
		case MIPI_DSI_DCS_READ:
		case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
		case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
		case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
			if (set->count == 1)
				return 1;
			else {
				LCD_ERR("set->count[%d] is not 1.. ", set->count);
				return 0;
			}
		default:
			LCD_DEBUG("type[%02x] is not read type.. ", set->cmds->msg.type);
			return 0;
	}
}

/* check if it is seamless mode (continuous splash mode) */
//  cont_splash_enabled is disabled after kernel booting...
// but seamless flag.. need to check...
//	if (pdata->panel_info.cont_splash_enabled) {
static inline bool ss_is_seamless_mode(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	/* FC2 change: set panle mode in SurfaceFlinger initialization, instead of kenrel booting... */
	if (!panel->cur_mode) {
		LCD_ERR("err: no panel mode yet...\n");
		// setting ture prevents tx dcs cmd...
		return false;
	}

	if (panel->cur_mode->dsi_mode_flags & DSI_MODE_FLAG_SEAMLESS)
		return true;
	else
		return false;
}

// TODO: 1) check if it has no problem to set like below..
//       2) it need to set drm seamless mode also...
//           mode->flags & DRM_MODE_FLAG_SEAMLESS
static inline void ss_set_seamless_mode(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	/* FC2 change: set panle mode in SurfaceFlinger initialization, instead of kenrel booting... */
	if (!panel->cur_mode) {
		LCD_ERR("err: no panel mode yet...\n");
		return;
	}

	panel->cur_mode->dsi_mode_flags |= DSI_MODE_FLAG_SEAMLESS;
}

static inline unsigned int ss_get_te_gpio(struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	return display->disp_te_gpio;
}

static inline bool ss_is_vsync_enabled(struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *dev;
	struct drm_vblank_crtc *vblank;
	struct drm_crtc *crtc = GET_DRM_CRTC(vdd);
	int pipe;

	pipe = drm_crtc_index(crtc);

	dev = display->drm_dev;
	vblank = &dev->vblank[pipe];

	return vblank->enabled;
}

// TOOD: get esd check flag.. not yet implemented in qcomm bsp...
static inline unsigned int ss_is_esd_check_enabled(struct samsung_display_driver_data *vdd)
{
	return true;
}

extern char *cmd_set_prop_map[SS_DSI_CMD_SET_MAX];
static inline char *ss_get_cmd_name(enum dsi_cmd_set_type type)
{
	return cmd_set_prop_map[type];
}


extern char *brr_mode_name[MAX_ALL_BRR_MODE];
static inline char *ss_get_brr_mode_name(enum SS_BRR_MODE mode)
{
	char *max_mode_name = "MAX_ALL_BRR_MODE";

	if (mode == MAX_ALL_BRR_MODE)
		return max_mode_name;

	return brr_mode_name[mode];
}

static inline bool __must_check SS_IS_CMDS_NULL(struct dsi_panel_cmd_set *set)
{
	return unlikely(!set) || unlikely(!set->cmds);
}

static inline struct dsi_panel_cmd_set *ss_get_cmds(
		struct samsung_display_driver_data *vdd,
		enum dsi_cmd_set_type type)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_display_mode_priv_info *priv_info;
	struct dsi_panel_cmd_set *set;
	int rev = vdd->panel_revision;

	/* save the mipi cmd type */
	vdd->cmd_type = type;

	/* QCT cmds */
	if (type < SS_DSI_CMD_SET_START) {
		if (!panel->cur_mode || !panel->cur_mode->priv_info) {
			LCD_ERR("err: (%d) panel has no valid priv\n", type);
			return NULL;
		}

		priv_info = panel->cur_mode->priv_info;
		set = &priv_info->cmd_sets[type];
		return set;
	}

	/* SS cmds */
	if (type == TX_AID_SUBDIVISION && vdd->br_info.common_br.pac)
		type = TX_PAC_AID_SUBDIVISION;

	if (type == TX_IRC_SUBDIVISION && vdd->br_info.common_br.pac)
		type = TX_PAC_IRC_SUBDIVISION;

	if (type > SS_DSI_CMD_SET_FOR_EACH_MODE_START && type < SS_DSI_CMD_SET_FOR_EACH_MODE_END) {
		priv_info = panel->cur_mode->priv_info;
		set = &priv_info->cmd_sets[type];
	} else
		set = &vdd->dtsi_data.cmd_sets[type];

	if (set->cmd_set_rev[rev])
		set = (struct dsi_panel_cmd_set *) set->cmd_set_rev[rev];

	return set;
}

static inline struct device_node *ss_get_panel_of(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	return panel->panel_of_node;
}

static inline struct brightness_table *ss_get_br_tbl(struct samsung_display_driver_data *vdd,
							int target_rr, bool target_sot_hs)
{
	struct brightness_table *br_tbl = NULL;
	int count;

	/* return default br_tbl (first br_tbl, maybe 60NM) in case VRR is not supported */
	if (!vdd->vrr.support_vrr_based_bl) {
		br_tbl = &vdd->br_info.br_tbl[0];
		return br_tbl;
	}

	for (count = 0; count < vdd->br_info.br_tbl_count; count++) {
		if (target_rr == vdd->br_info.br_tbl[count].refresh_rate &&
			target_sot_hs == vdd->br_info.br_tbl[count].is_sot_hs_mode) {
			br_tbl = &vdd->br_info.br_tbl[count];
			break;
		}
	}

	if (!br_tbl) {
		/* return default br_tbl (first br_tbl, maybe 60NM), to prevent NULL return */
		br_tbl = &vdd->br_info.br_tbl[0];

		LCD_ERR("fail to find br_tbl (RR: %d, HS: %d), " \
				"return default br_tbl (RR: %d, HS: %d), called by %pS\n",
				target_rr, target_sot_hs,
				br_tbl->refresh_rate, br_tbl->is_sot_hs_mode,
				__builtin_return_address(0));
	}

	return br_tbl;
}

static inline struct brightness_table *ss_get_cur_br_tbl(struct samsung_display_driver_data *vdd)
{
	int cur_rr = vdd->vrr.cur_refresh_rate;
	bool cur_sot_hs = vdd->vrr.cur_sot_hs_mode;

	if (!vdd->br_info.br_tbl_count) {
		LCD_ERR("br_tbl_count is 0 \n");
		return NULL;
	}

	/* Bridge Refresh Rate mode
	 * 60hs/120: gamam interpolation, 61~120 hz use 120 bl tbl
	 * 48/60 normal: AOR interpolation, 49~60 hz use 60 bl tbl
	 * 96/120 HS: AOR interpolation, 97~120 hz use 120 bl tbl
	 */
	switch (cur_rr) {
	case 49 ... 59:
		cur_rr = 60;
		break;
	case 61 ... 95:
	case 97 ... 119:
		cur_rr = 120;
		break;
	default:
		break;
	}

	return ss_get_br_tbl(vdd, cur_rr, cur_sot_hs);
}

static inline u8 *ss_get_vbias_data(struct flash_gm2 *gm2_table, enum SS_VBIAS_MODE mode)
{
	int offset;
	u8 *vbias;

	if (!gm2_table->vbias_data) {
		LCD_ERR("vbias_data is not allocated yet..\n");
		return NULL;
	}

	offset = gm2_table->len_one_vbias_mode * mode;
	vbias = (u8 *)(gm2_table->vbias_data + offset);

	return vbias;
}

#endif /* SAMSUNG_DSI_PANEL_COMMON_H */
