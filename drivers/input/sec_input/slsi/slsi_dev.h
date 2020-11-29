/* drivers/input/sec_input/slsi/slsi_dev.h
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
  *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SEC_TS_H__
#define __SEC_TS_H__

#include <asm/unaligned.h>
#include <linux/completion.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input/sec_cmd.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>

#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
#include <linux/input/tui_hal_ts.h>
#endif
#ifdef CONFIG_SAMSUNG_TUI
#include "stui_inf.h"
#endif

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
#include <linux/input/sec_secure_touch.h>
#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <soc/qcom/scm.h>

#define SECURE_TOUCH_ENABLE	1
#define SECURE_TOUCH_DISABLE	0
#endif

#include <linux/input/sec_tclm_v2.h>
#ifdef CONFIG_INPUT_TOUCHSCREEN_TCLMV2
#define TCLM_CONCEPT
#endif

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
#include <linux/input/sec_tsp_dumpkey.h>
extern struct tsp_dump_callbacks dump_callbacks;
static struct delayed_work *p_ghost_check;
#endif

#include "../sec_input.h"

#define SEC_TS_I2C_NAME		"sec_ts"
#define SEC_TS_DEVICE_NAME	"SEC_TS"

#define USE_OPEN_CLOSE
#undef USE_RESET_DURING_POWER_ON
#undef USE_RESET_EXIT_LPM
#define USE_POR_AFTER_I2C_RETRY
#undef USER_OPEN_DWORK
#define MINORITY_REPORT

#if defined(USE_RESET_DURING_POWER_ON) || defined(USE_POR_AFTER_I2C_RETRY) || defined(USE_RESET_EXIT_LPM)
#define USE_POWER_RESET_WORK
#endif

#define TOUCH_PRINT_INFO_DWORK_TIME	30000	/* 30s */
#define TOUCH_RESET_DWORK_TIME		10
#define TOUCH_POWER_ON_DWORK_TIME	100
#define BRUSH_Z_DATA		63	/* for ArtCanvas */

#define TSP_PATH_EXTERNAL_FW		"/sdcard/Firmware/TSP/tsp.bin"
#define TSP_PATH_EXTERNAL_FW_SIGNED	"/sdcard/Firmware/TSP/tsp_signed.bin"
#define TSP_PATH_SPU_FW_SIGNED		"/spu/TSP/ffu_tsp.bin"

#define SEC_TS_MAX_FW_PATH		64
#define SEC_TS_FW_BLK_SIZE_MAX		(512)
#define SEC_TS_FW_BLK_SIZE_DEFAULT	(512)	// y761 & y771 ~
#define SEC_TS_SELFTEST_REPORT_SIZE	80

#define SEC_TS_FW_HEADER_SIGN		0x53494654
#define SEC_TS_FW_CHUNK_SIGN		0x53434654

#define SEC_TS_LOCATION_DETECT_SIZE	6

#define AMBIENT_CAL			0
#define OFFSET_CAL_SDC			1
#define OFFSET_CAL_SEC			2

#define SEC_TS_NVM_OFFSET_FAC_RESULT			0
#define SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT		1

/* TCLM_CONCEPT */
#define SEC_TS_NVM_OFFSET_CAL_COUNT			2
#define SEC_TS_NVM_OFFSET_TUNE_VERSION			3
#define SEC_TS_NVM_OFFSET_TUNE_VERSION_LENGTH		2

#define SEC_TS_NVM_OFFSET_CAL_POSITION			5
#define SEC_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT		6
#define SEC_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP		7
#define SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO		8
#define SEC_TS_NVM_OFFSET_HISTORY_QUEUE_SIZE		20

#define SEC_TS_NVM_OFFSET_CAL_FAIL_FLAG			(SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO + SEC_TS_NVM_OFFSET_HISTORY_QUEUE_SIZE + 1)
#define SEC_TS_NVM_OFFSET_CAL_FAIL_CNT			(SEC_TS_NVM_OFFSET_CAL_FAIL_FLAG + 1)
#define SEC_TS_NVM_OFFSET_LENGTH			(SEC_TS_NVM_OFFSET_CAL_FAIL_CNT + 1)

#define SEC_TS_NVM_LAST_BLOCK_OFFSET			SEC_TS_NVM_OFFSET_LENGTH
#define SEC_TS_NVM_TOTAL_OFFSET_LENGTH		(SEC_TS_NVM_LAST_BLOCK_OFFSET + 1)

#define TOUCH_TX_CHANNEL_NUM			50
#define TOUCH_RX_CHANNEL_NUM			50

#define MAX_SUPPORT_TOUCH_COUNT		10
#define MAX_SUPPORT_HOVER_COUNT		1

enum sec_fw_update_status {
	SEC_NOT_UPDATE = 0,
	SEC_NEED_FW_UPDATE,
	SEC_NEED_CALIBRATION_ONLY,
	SEC_NEED_FW_UPDATE_N_CALIBRATION,
};

/* factory test mode */
struct sec_ts_test_mode {
	u8 type;
	short min;
	short max;
	bool allnode;
	bool frame_channel;
};

struct sec_ts_fw_file {
	u8 *data;
	u32 pos;
	size_t size;
};

/* 48 byte */
struct sec_ts_selftest_fail_hist {
	u32 tsp_signature;
	u32 tsp_fw_version;
	u8 fail_cnt1;
	u8 fail_cnt2;
	u16 selftest_exec_parm;
	u32 test_result;
	u8 fail_data[8];
	u32 fail_type:8;
	u32 reserved:24;
	u32 defective_data[5];
} __attribute__ ((packed));

/* 16 byte */
struct sec_ts_gesture_status {
	u8 eid:2;
	u8 stype:4;
	u8 sf:2;
	u8 gesture_id;
	u8 gesture_data_1;
	u8 gesture_data_2;
	u8 gesture_data_3;
	u8 gesture_data_4;
	u8 reserved_1;
	u8 left_event_5_0:5;
	u8 reserved_2:3;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num:4;
	u8 reserved10:4;
	u8 reserved11;
	u8 reserved12;
	u8 reserved13;
	u8 reserved14;
	u8 reserved15;
} __attribute__ ((packed));

/* 16 byte */
struct sec_ts_event_status {
	u8 eid:2;
	u8 stype:4;
	u8 sf:2;
	u8 status_id;
	u8 status_data_1;
	u8 status_data_2;
	u8 status_data_3;
	u8 status_data_4;
	u8 status_data_5;
	u8 left_event_5_0:5;
	u8 reserved_2:3;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num:4;
	u8 reserved10:4;
	u8 reserved11;
	u8 reserved12;
	u8 reserved13;
	u8 reserved14;
	u8 reserved15;
} __attribute__ ((packed));

/* 16 byte */
struct sec_ts_event_coordinate {
	u8 eid:2;
	u8 tid:4;
	u8 tchsta:2;
	u8 x_11_4;
	u8 y_11_4;
	u8 y_3_0:4;
	u8 x_3_0:4;
	u8 major;
	u8 minor;
	u8 z:6;
	u8 ttype_3_2:2;
//	u8 left_event:6;
	u8 left_event:5;
	u8 max_energy_flag:1;
	u8 ttype_1_0:2;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num:4;
	u8 noise_status:2;
	u8 reserved10:2;
	u8 reserved11;
	u8 reserved12;
	u8 reserved13;
	u8 reserved14;
	u8 reserved15;
} __attribute__ ((packed));

/* not fixed */
struct sec_ts_coordinate {
	u8 id;
	u8 ttype;
	u8 action;
	u16 x;
	u16 y;
	u16 p_x;
	u16 p_y;
	u8 z;
	u8 hover_flag;
	u8 glove_flag;
	u8 touch_height;
	u16 mcount;
	u8 major;
	u8 minor;
	bool palm;
	int palm_count;
	u8 left_event;
	u16 max_energy_x;
	u16 max_energy_y;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num;
	u8 noise_status;
};


struct sec_ts_data {
	u32 crc_addr;
	u32 fw_addr;
	u32 para_addr;
	u32 flash_page_size;
	u8 boot_ver[3];

	struct device *dev;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *input_dev_pad;
	struct input_dev *input_dev_touch;
	struct input_dev *input_dev_proximity;
	struct sec_ts_plat_data *plat_data;
	struct sec_ts_coordinate coord[MAX_SUPPORT_TOUCH_COUNT + MAX_SUPPORT_HOVER_COUNT];


	u8 lowpower_mode;
	u8 brush_mode;
	u8 touchable_area;
	u8 external_noise_mode;
	volatile u8 touch_noise_status;
	volatile u8 touch_pre_noise_status;
	volatile bool input_closed;
	long prox_power_off;

	int touch_count;
	int tx_count;
	int rx_count;
	int i2c_burstmax;
	int ta_status;
	volatile int power_status;
	int raw_status;
	u16 touch_functions;
	u16 ic_status;
	u8 charger_mode;
	bool force_charger_mode;
	struct sec_ts_event_coordinate touchtype;
	u8 gesture_status[6];
	u8 cal_status;
	struct mutex device_mutex;
	struct mutex i2c_mutex;
	struct mutex eventlock;
	struct mutex modechange;
	struct mutex sponge_mutex;

	int nv;
	int disassemble_count;

	struct delayed_work work_read_info;
	struct delayed_work work_print_info;
	u32	print_info_cnt_open;
	u32	print_info_cnt_release;
	u16	print_info_currnet_mode;
#ifdef USE_POWER_RESET_WORK
	struct delayed_work reset_work;
	volatile bool reset_is_on_going;
#endif
	struct delayed_work work_read_functions;
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	atomic_t secure_enabled;
	atomic_t secure_pending_irqs;
	struct completion secure_powerdown;
	struct completion secure_interrupt;
#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
	struct completion st_irq_received;
#endif
#endif
	struct completion resume_done;
	struct wake_lock wakelock;
	struct sec_cmd_data sec;
	short *pFrame;

	bool probe_done;
	bool info_work_done;
	volatile bool shutdown_is_on_going;
	int cover_type;
	u8 cover_cmd;
	u16 rect_data[4];
	u16 fod_rect_data[4];
	bool fod_set_val;

	int tspid_val;
	int tspicid_val;
	unsigned int scrub_id;
	unsigned int scrub_x;
	unsigned int scrub_y;

	u8 tsp_temp_data;
	bool tsp_temp_data_skip;

	u8 grip_edgehandler_direction;
	int grip_edgehandler_start_y;
	int grip_edgehandler_end_y;
	u16 grip_edge_range;
	u8 grip_deadzone_up_x;
	u8 grip_deadzone_dn_x;
	int grip_deadzone_y;
	u8 grip_landscape_mode;
	int grip_landscape_edge;
	u16 grip_landscape_deadzone;
	u16 grip_landscape_top_deadzone;
	u16 grip_landscape_bottom_deadzone;
	u16 grip_landscape_top_gripzone;
	u16 grip_landscape_bottom_gripzone;

	struct delayed_work ghost_check;
	u8 tsp_dump_lock;

	struct sec_tclm_data *tdata;
	bool is_cal_done;

	volatile int wet_mode;
	bool factory_level;
	u8 factory_position;

	u8 ed_enable;
	u16 proximity_thd;
	bool proximity_jig_mode; 
	u8 sip_mode;

	unsigned char ito_test[4];		/* ito panel tx/rx chanel */
	unsigned char check_multi;
	unsigned int multi_count;		/* multi touch count */
	unsigned int wet_count;			/* wet mode count */
	unsigned int noise_count;		/* noise mode count */
	unsigned int dive_count;		/* dive mode count */
	unsigned int comm_err_count;	/* i2c comm error count */
	unsigned int checksum_result;	/* checksum result */
	unsigned char module_id[4];
	unsigned int all_finger_count;
	unsigned int all_aod_tap_count;
	unsigned int all_spay_count;
	int max_ambient;
	int max_ambient_channel_tx;
	int max_ambient_channel_rx;
	int min_ambient;
	int min_ambient_channel_tx;
	int min_ambient_channel_rx;
	int mode_change_failed_count;
	int ic_reset_count;

	/* average value for each channel */
	short ambient_tx[TOUCH_TX_CHANNEL_NUM];
	short ambient_rx[TOUCH_RX_CHANNEL_NUM];

	/* max - min value for each channel */
	short ambient_tx_delta[TOUCH_TX_CHANNEL_NUM];
	short ambient_rx_delta[TOUCH_RX_CHANNEL_NUM];

	/* for factory - factory_cmd_result_all() */
	short cm_raw_set_avg_min;
	short cm_raw_set_avg_max;
	short cm_raw_set_p2p;
	
	short self_raw_set_avg_tx_min;
	short self_raw_set_avg_tx_max;
	short self_raw_set_avg_rx_min;
	short self_raw_set_avg_rx_max;
	short self_raw_set_p2p_tx_diff;
	short self_raw_set_p2p_rx_diff;

	short cm_raw_key_p2p_min;
	short cm_raw_key_p2p_max;
	short cm_raw_key_p2p_diff;
	short cm_raw_key_p2p_diff_data[2][3];	/* key : max support key is 3 */
	short cm_raw_set_p2p_gap_y;
	short cm_raw_set_p2p_gap_y_result;	/* mis_cal pass/fail */

	u32	defect_probability;
#ifdef MINORITY_REPORT
	u8	item_ito;
	u8	item_rawdata;
	u8	item_crc;
	u8	item_i2c_err;
	u8	item_wet;
#endif

	int debug_flag;
	int fix_active_mode;

	u8 lp_sensitivity;

	u8 fod_vi_size;
	u8 press_prop;
/* thermistor */
	union power_supply_propval psy_value;
	struct power_supply *psy;

	int proc_cmoffset_size;
	int proc_cmoffset_all_size;
	char *cmoffset_sdc_proc;
	char *cmoffset_main_proc;
	char *miscal_proc;
	char *cmoffset_all_proc;

	int proc_fail_hist_size;
	int proc_fail_hist_all_size;
	char *fail_hist_sdc_proc;
	char *fail_hist_sub_proc;
	char *fail_hist_main_proc;
	char *fail_hist_all_proc;

	int (*sec_ts_i2c_write)(struct sec_ts_data *ts, u8 reg, u8 *data, int len);
	int (*sec_ts_i2c_read)(struct sec_ts_data *ts, u8 reg, u8 *data, int len);
	int (*sec_ts_i2c_write_burst)(struct sec_ts_data *ts, u8 *data, int len);
	int (*sec_ts_i2c_read_bulk)(struct sec_ts_data *ts, u8 *data, int len);
	int (*sec_ts_read_sponge)(struct sec_ts_data *ts, u8 *data, int len);
	int (*sec_ts_write_sponge)(struct sec_ts_data *ts, u8 *data, int len);
};

typedef struct {
	u32 signature;			/* signature */
	u32 version;			/* version */
	u32 totalsize;			/* total size */
	u32 checksum;			/* checksum */
	u32 img_ver;			/* image file version */
	u32 img_date;			/* image file date */
	u32 img_description;		/* image file description */
	u32 fw_ver;			/* firmware version */
	u32 fw_date;			/* firmware date */
	u32 fw_description;		/* firmware description */
	u32 para_ver;			/* parameter version */
	u32 para_date;			/* parameter date */
	u32 para_description;		/* parameter description */
	u32 num_chunk;			/* number of chunk */
	u32 reserved1;
	u32 reserved2;
} fw_header;

typedef struct {
	u32 signature;
	u32 addr;
	u32 size;
	u32 reserved;
} fw_chunk;

#ifdef CONFIG_SAMSUNG_TUI
struct sec_ts_data *tsp_info;
#endif

static bool shutdown_is_on_going_tsp;

int sec_ts_power(void *data, bool on);
int sec_ts_stop_device(struct sec_ts_data *ts);
int sec_ts_start_device(struct sec_ts_data *ts);
int sec_ts_set_lowpowermode(struct sec_ts_data *ts, u8 mode);
int sec_ts_set_external_noise_mode(struct sec_ts_data *ts, u8 mode);
int sec_ts_firmware_update_on_probe(struct sec_ts_data *ts, bool force_update);
int sec_ts_firmware_update_on_hidden_menu(struct sec_ts_data *ts, int update_type);
int sec_ts_glove_mode_enables(struct sec_ts_data *ts, int mode);
int sec_ts_set_cover_type(struct sec_ts_data *ts, bool enable);
int sec_ts_wait_for_ready(struct sec_ts_data *ts, unsigned int ack);
int sec_ts_fn_init(struct sec_ts_data *ts);
int sec_ts_read_calibration_report(struct sec_ts_data *ts);
int sec_ts_fix_tmode(struct sec_ts_data *ts, u8 mode, u8 state);
int sec_ts_release_tmode(struct sec_ts_data *ts);
int sec_ts_set_custom_library(struct sec_ts_data *ts);
int sec_ts_set_aod_rect(struct sec_ts_data *ts);
int sec_ts_set_fod_rect(struct sec_ts_data *ts);
int sec_ts_set_temp(struct sec_ts_data *ts, bool bforced);

int sec_ts_check_custom_library(struct sec_ts_data *ts);
int sec_ts_set_touch_function(struct sec_ts_data *ts);

void sec_ts_locked_release_all_finger(struct sec_ts_data *ts);
void sec_ts_unlocked_release_all_finger(struct sec_ts_data *ts);

void sec_ts_fn_remove(struct sec_ts_data *ts);
void sec_ts_delay(unsigned int ms);
int sec_ts_read_information(struct sec_ts_data *ts);

void sec_ts_run_rawdata_all(struct sec_ts_data *ts, bool full_read);

void sec_ts_reinit(struct sec_ts_data *ts);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
int sec_ts_raw_device_init(struct sec_ts_data *ts);
#endif

void sec_ts_get_touch_function(struct work_struct *work);
void sec_ts_set_input_prop(struct sec_ts_data *ts, struct input_dev *dev, u8 propbit);
void sec_ts_set_input_prop_proximity(struct sec_ts_data *ts, struct input_dev *dev);
void sec_ts_init_proc(struct sec_ts_data *ts);

void set_grip_data_to_ic(struct sec_ts_data *ts, u8 flag);
void sec_ts_set_grip_type(struct sec_ts_data *ts, u8 set_type);
void sec_ts_ioctl_init(struct sec_ts_data *ts);
void sec_ts_ioctl_remove(struct sec_ts_data *ts);

void set_tsp_nvm_data_clear(struct sec_ts_data *ts, u8 offset);
int get_tsp_nvm_data(struct sec_ts_data *ts, u8 offset);

void rearrange_sft_result(u8 *data, int length);
void location_detect(struct sec_ts_data *ts, char *loc, int x, int y);
void sec_ts_print_info(struct sec_ts_data *ts);

#ifdef MINORITY_REPORT
void minority_report_calculate_rawdata(struct sec_ts_data *ts);
void minority_report_calculate_ito(struct sec_ts_data *ts);
void minority_report_sync_latest_value(struct sec_ts_data *ts);
#endif

void sec_tclm_parse_dt(struct i2c_client *client, struct sec_tclm_data *tdata);
int sec_ts_power(void *data, bool on);
int sec_ts_pinctrl_configure(struct sec_ts_data *ts, bool enable);
int sec_ts_parse_dt(struct i2c_client *client);
int sec_ts_set_press_property(struct sec_ts_data *ts);
int sec_ts_set_fod_rect(struct sec_ts_data *ts);

int sec_ts_i2c_write(struct sec_ts_data *ts, u8 reg, u8 *data, int len);
int sec_ts_i2c_read(struct sec_ts_data *ts, u8 reg, u8 *data, int len);
int sec_ts_i2c_write_burst(struct sec_ts_data *ts, u8 *data, int len);
int sec_ts_i2c_read_bulk(struct sec_ts_data *ts, u8 *data, int len);
int sec_ts_read_from_sponge(struct sec_ts_data *ts, u8 *data, int len);
int sec_ts_write_to_sponge(struct sec_ts_data *ts, u8 *data, int len);
void sec_ts_set_utc_sponge(struct sec_ts_data *ts);

int sec_ts_fix_tmode(struct sec_ts_data *ts, u8 mode, u8 state);
int sec_ts_p2p_tmode(struct sec_ts_data *ts);
int execute_p2ptest(struct sec_ts_data *ts);
int sec_ts_release_tmode(struct sec_ts_data *ts);
int sec_ts_set_press_property(struct sec_ts_data *ts);
int sec_ts_read_information(struct sec_ts_data *ts);

#ifdef USE_POWER_RESET_WORK
void sec_ts_reset_work(struct work_struct *work);
#endif
void sec_ts_read_info_work(struct work_struct *work);
void sec_ts_print_info_work(struct work_struct *work);

#ifdef USE_OPEN_CLOSE
int sec_ts_input_open(struct input_dev *dev);
void sec_ts_input_close(struct input_dev *dev);
#endif

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
void sec_ts_check_rawdata(struct work_struct *work);
void dump_tsp_log(void);
#endif

irqreturn_t sec_ts_irq_thread(int irq, void *ptr);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
irqreturn_t secure_filter_interrupt(struct sec_ts_data *ts);
ssize_t secure_touch_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf);
ssize_t secure_touch_enable_store(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count);
ssize_t secure_touch_show(struct device *dev,
		struct device_attribute *attr, char *buf);
ssize_t secure_ownership_show(struct device *dev,
		struct device_attribute *attr, char *buf);

int secure_touch_init(struct sec_ts_data *ts);
void secure_touch_stop(struct sec_ts_data *ts, bool stop);
#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
int secure_get_irq(struct device *dev);
#endif
#endif

ssize_t get_miscal_dump(struct sec_ts_data *ts, char *buf);
ssize_t get_cmoffset_dump_all(struct sec_ts_data *ts, char *buf, u8 position);
ssize_t get_selftest_fail_hist_dump_all(struct sec_ts_data *ts, char *buf, u8 position);

#endif
