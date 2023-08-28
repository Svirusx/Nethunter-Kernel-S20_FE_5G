/*
 * Copyright (C) 2010 - 2017 Novatek, Inc.
 *
 * $Revision: 22429 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#ifndef _LINUX_NVT_TOUCH_H
#define _LINUX_NVT_TOUCH_H

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/wakelock.h>

#ifdef CONFIG_SAMSUNG_TUI
#include "stui_inf.h"
#endif

#if defined(CONFIG_DISPLAY_SAMSUNG)
#include <linux/notifier.h>
#endif
#if defined(CONFIG_FB)
#include <linux/fb.h>
#endif
#include <linux/mutex.h>
#include <linux/input/sec_cmd.h>
#include <linux/firmware.h>

#include "nt36xxx_mem_map.h"

#define TSP_PATH_EXTERNAL_FW		"/sdcard/firmware/tsp/tsp.bin"
#define TSP_PATH_EXTERNAL_FW_SIGNED		"/sdcard/firmware/tsp/tsp_signed.bin"
#define TSP_PATH_SPU_FW_SIGNED		"/spu/TSP/ffu_tsp.bin"

//---I2C driver info.---
#define NVT_I2C_NAME "nvt-ts"
#define I2C_BLDR_Address 0x01
#define I2C_FW_Address 0x01
#define I2C_HW_Address 0x62

#define TOUCH_MAX_FINGER_NUM	10

#define SIZE_4KB		4096
#define FLASH_SECTOR_SIZE	SIZE_4KB
#define SIZE_64KB		65536
#define BLOCK_64KB_NUM		4


#define DEFAULT_MAX_HEIGHT	2560
#define DEFAULT_MAX_WIDTH	1600

#define INT_ENABLE		1
#define INT_DISABLE		0

#define NVT_TS_DEBUG_PRINT_I2C_READ_CMD  0x04
#define NVT_TS_DEBUG_PRINT_I2C_WRITE_CMD 0x08

/* sponge mode */
#define NVT_SPONGE_MODE_SPAY			(1 << 1)
#define NVT_SPONGE_MODE_AOD			(1 << 2)
#define NVT_SPONGE_MODE_SINGLETAP		(1 << 3)
#define NVT_SPONGE_MODE_PRESS			(1 << 4)
#define NVT_SPONGE_MODE_DOUBLETAP_WAKEUP	(1 << 5)

/* event id */
#define NVT_GESTURE_EVENT			2

/* sponge gesture type */
#define NVT_GESTURE_TYPE_SWIPE			0x00
#define NVT_GESTURE_TYPE_DOUBLE_TAP		0x01
#define NVT_GESTURE_TYPE_PRESS			0x03
#define NVT_GESTURE_TYPE_SINGLE_TAP		0x04

/* sponge gesture id */
#define NVT_GESTURE_ID_AOD			0x00
#define NVT_GESTURE_ID_DOUBLETAP_TO_WAKEUP	0x01

struct sponge_report_format {
	u8 eid:2;
	u8 gesturetype:4;
	u8 sf:2;
	u8 gestureid;
	u8 x_11_4;
	u8 y_11_4;
	u8 y_3_0:4;
	u8 x_3_0:4;
	u8 reserved2;
	u8 reserved3;
	u8 leftevent:6;
	u8 reserved4:2;
};

struct nvt_ts_coord {
	u16 x;
	u16 y;
	u16 p;
	u16 p_x;
	u16 p_y;
	u8 w_major;
	u8 w_minor;
	u8 palm;
	u8 status;
	u8 p_status;
	bool press;
	bool p_press;
	int move_count;
};

struct nvt_ts_platdata {
	int irq_gpio;
	u32 irq_flags;
	u8 x_num;
	u8 y_num;
	u16 abs_x_max;
	u16 abs_y_max;
	u32 area_indicator;
	u32 area_navigation;
	u32 area_edge;
	u8 max_touch_num;
	u32 bringup;
	bool check_fw_project_id;
	const char *firmware_name;
	u32 open_test_spec[2];
	u32 short_test_spec[2];
	int diff_test_frame;
	bool support_dex;
	bool enable_settings_aot;
	bool scanoff_cover_close;
	bool enable_glove_mode;
	bool enable_sysinput_enabled;
};

struct nvt_ts_data {
	struct i2c_client *client;
	struct nvt_ts_platdata *platdata;
	struct input_dev *input_dev;
	struct input_dev *input_dev_pad;
	struct nvt_ts_coord coords[TOUCH_MAX_FINGER_NUM];
	u8 touch_count;
#if defined(CONFIG_DISPLAY_SAMSUNG)
	struct notifier_block drm_notif;
#endif
#if defined(CONFIG_FB)
	struct notifier_block fb_nb;
#endif
	struct completion resume_done;
	struct wake_lock wakelock;

	u8 lowpower_mode;

	u8 fw_ver_ic[4];
	u8 fw_ver_ic_bar;
	u8 fw_ver_bin[4];
	u8 fw_ver_bin_bar;
	struct mutex lock;
	struct mutex i2c_mutex;
	struct mutex irq_mutex;
	const struct nvt_ts_mem_map *mmap;
	u8 carrier_system;

	struct delayed_work work_print_info;
	struct delayed_work work_read_info;
	u32 print_info_cnt_open;
	u32 print_info_cnt_release;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	int ss_touch_num;
	atomic_t secure_enabled;
	atomic_t secure_pending_irqs;
	struct completion secure_powerdown;
	struct completion secure_interrupt;
#endif
	struct sec_cmd_data sec;
	const struct firmware *fw_entry;
	volatile int power_status;
	u16 sec_function;
	bool touchable_area;
	bool untouchable_area;

	bool check_multi;
	unsigned int multi_count;	/* multi touch count */
	unsigned int comm_err_count;	/* i2c comm error count */
	unsigned int checksum_result;	/* checksum result */
	unsigned int all_finger_count;

	bool noise_mode;

	int debug_flag;
	bool flip_enable;
	bool display_state_in_progress;

	int grip_edgehandler_restore_data[SEC_CMD_PARAM_NUM];
	int setgrip_restore_data[SEC_CMD_PARAM_NUM];
};

typedef enum {
	RESET_STATE_INIT	= 0xA0,// IC reset
	RESET_STATE_REK,	// ReK baseline
	RESET_STATE_REK_FINISH,	// baseline is ready
	RESET_STATE_NORMAL_RUN,	// normal run
	RESET_STATE_MAX		= 0xAF
} RST_COMPLETE_STATE;

typedef enum {
	EVENT_MAP_HOST_CMD			= 0x50,
	EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE	= 0x51,
	EVENT_MAP_FUNCT_STATE			= 0x5C,
	EVENT_MAP_RESET_COMPLETE		= 0x60,
	EVENT_MAP_BLOCK_AREA			= 0x62,
	EVENT_MAP_FWINFO			= 0x78,
	EVENT_MAP_PANEL				= 0x8F,
	EVENT_MAP_PROJECTID			= 0x9A,
	EVENT_MAP_SENSITIVITY_DIFF		= 0x9D,
} I2C_EVENT_MAP;

#define NVT_CMD_GESTURE_MODE			0x13
#define NVT_CMD_DEEP_SLEEP_MODE			0x11

typedef enum {
	SPONGE_LP_FEATURE = 0x0000,
	SPONGE_UTC = 0x0010,
	SPONGE_DUMP_FORMAT = 0x00F0,
	SPONGE_DUMP_CNT = 0x00F1,
	SPONGE_DUMP_CUR_IDX = 0x00F2,
	SPONGE_DUMP_START = 0x00F4,
} SPONGE_OFFSETS;

enum {
	POWER_OFF_STATUS = 0,
	POWER_LPM_STATUS,
	POWER_ON_STATUS
};

enum display_state {
	DISPLAY_STATE_SERVICE_SHUTDOWN = -1,
	DISPLAY_STATE_NONE = 0,
	DISPLAY_STATE_OFF,
	DISPLAY_STATE_ON,
	DISPLAY_STATE_DOZE,
	DISPLAY_STATE_DOZE_SUSPEND,
	DISPLAY_STATE_LPM_OFF = 20,
	DISPLAY_STATE_FORCE_OFF,
	DISPLAY_STATE_FORCE_ON,
};

enum display_event {
	DISPLAY_EVENT_EARLY = 0,
	DISPLAY_EVENT_LATE,
};

void nvt_ts_run_rawdata_all(struct nvt_ts_data *ts);
void nvt_ts_bootloader_reset(struct nvt_ts_data *ts);

void nvt_interrupt_set(struct nvt_ts_data *ts, int enable);
int nvt_ts_i2c_write(struct nvt_ts_data *ts, u16 address, u8 *buf, u16 len);
int nvt_ts_i2c_read(struct nvt_ts_data *ts, u16 address, u8 *buf, u16 len);
int nvt_ts_read_from_sponge(struct nvt_ts_data *ts, u8 *data, int addr_offset, int len);
int nvt_ts_write_to_sponge(struct nvt_ts_data *ts, u8 *data, int addr_offset, int len);
void nvt_ts_release_all_finger(struct nvt_ts_data *ts);

int nvt_ts_check_fw_reset_state(struct nvt_ts_data *ts, RST_COMPLETE_STATE check_reset_state);
void nvt_ts_sw_reset_idle(struct nvt_ts_data *ts);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
void nvt_ts_flash_proc_init(struct nvt_ts_data *ts);
void nvt_ts_flash_proc_remove(void);
#endif

#if defined(CONFIG_DISPLAY_SAMSUNG)
int msm_drm_register_notifier_client(struct notifier_block *nb);
int msm_drm_unregister_notifier_client(struct notifier_block *nb);
#endif

#if defined(CONFIG_EXYNOS_DPU20)
extern unsigned int lcdtype;
#endif
#endif /* _LINUX_NVT_TOUCH_H */
