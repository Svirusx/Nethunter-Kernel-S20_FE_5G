
/*
 * MELFAS MMS400 Touchscreen Driver
 *
 * Copyright (C) 2014 MELFAS Inc.
 *
 */

#ifndef __MELFAS_MMS400_H
#define __MELFAS_MMS400_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/limits.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/kernel.h>
#include <linux/spu-verify.h>

#include "melfas_mss100_reg.h"

#include <linux/sec_class.h>

#ifdef CONFIG_VBUS_NOTIFIER
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/vbus_notifier.h>
#endif

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <soc/qcom/scm.h>

#define SECURE_TOUCH_ENABLE	1
#define SECURE_TOUCH_DISABLE	0
#endif

#ifdef CONFIG_SEC_DEBUG_TSP_LOG
#include <linux/sec_debug.h>
#endif
#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
#include <linux/input/sec_tsp_dumpkey.h>
extern struct tsp_dump_callbacks dump_callbacks;
static struct delayed_work *p_ghost_check;
#endif

#include <linux/input/sec_cmd.h>

#ifdef CONFIG_SAMSUNG_TUI
#include <linux/input/stui_inf.h>
#endif

#define GLOVE_MODE
#define COVER_MODE

#define MMS_DEVICE_NAME	"mss_ts"
#define MMS_CONFIG_DATE		{0x00, 0x00, 0x00, 0x00}
#define CHIP_NAME "MSS100"

//Config driver
#define MMS_USE_INPUT_OPEN_CLOSE	1
#define I2C_RETRY_COUNT			3
#ifdef CONFIG_SEC_FACTORY
#define RESET_ON_EVENT_ERROR		0
#else
#define RESET_ON_EVENT_ERROR		1
#endif
#define ESD_COUNT_FOR_DISABLE		7
#define MMS_USE_TOUCHKEY		0

#define POWER_ON_DELAY        150 /* ms */
#define POWER_ON_DELAY_ISC    20 /* ms */
#define POWER_OFF_DELAY       10 /* ms */
#define USE_STARTUP_WAITING   0 /* 0 (default) or 1 */
#define STARTUP_TIMEOUT       200 /* ms */

#define TO_TOUCH_MODE		0
#define TO_LOWPOWER_MODE		1

#define TOUCH_RESET_DWORK_TIME		10

//Features
#define MMS_USE_NAP_MODE		0

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
#define MMS_USE_TEST_MODE		1
#define MMS_USE_DEV_MODE		1
#else
#define MMS_USE_TEST_MODE		0
#define MMS_USE_DEV_MODE		0
#endif

//Input value
#define MAX_FINGER_NUM			10
#define INPUT_AREA_MIN			0
#define INPUT_AREA_MAX			255
#define INPUT_PRESSURE_MIN		0
#define INPUT_PRESSURE_MAX		255
#define INPUT_TOUCH_MAJOR_MIN		0
#define INPUT_TOUCH_MAJOR_MAX		255
#define INPUT_TOUCH_MINOR_MIN		0
#define INPUT_TOUCH_MINOR_MAX		255
#define INPUT_ANGLE_MIN			0
#define INPUT_ANGLE_MAX			255
#define INPUT_HOVER_MIN			0
#define INPUT_HOVER_MAX			255
#define INPUT_PALM_MIN			0
#define INPUT_PALM_MAX			1

/* Firmware update */
#define TSP_PATH_EXTERNAL_FW        "/sdcard/Firmware/TSP/tsp.bin"
#define TSP_PATH_EXTERNAL_FW_SIGNED        "/sdcard/Firmware/TSP/tsp_signed.bin"
#define TSP_PATH_SPU_FW_SIGNED            "/spu/TSP/ffu_tsp.bin"

#define MMS_USE_AUTO_FW_UPDATE		1
#define MMS_FW_MAX_SECT_NUM		4
#define MMS_FW_UPDATE_DEBUG		0
#define MMS_FW_UPDATE_SECTION		1
#define MMS_EXT_FW_FORCE_UPDATE		1

/* Command mode */
#define CMD_LEN				32
#define CMD_RESULT_LEN			512
#define CMD_RESULT_STR_LEN		(4096 - 1)
#define CMD_RESULT_WORD_LEN			10

/* event format */
#define EVENT_FORMAT_BASIC 			0
#define EVENT_FORMAT_WITH_RECT 			1
#define EVENT_FORMAT_WITH_PRESSURE		3
#define EVENT_FORMAT_KEY_ONLY 		4
#define EVENT_FORMAT_WITH_PRESSURE_2BYTE	8
#define EVENT_FORMAT_16BYTE			10
/* vector id */
#define VECTOR_ID_SCREEN_RX				1
#define VECTOR_ID_SCREEN_TX				2
#define VECTOR_ID_KEY_RX				3
#define VECTOR_ID_KEY_TX				4
#define VECTOR_ID_PRESSURE				5
#define VECTOR_ID_OPEN_RESULT			7
#define VECTOR_ID_OPEN_RX				8
#define VECTOR_ID_OPEN_TX				9
#define VECTOR_ID_SHORT_RESULT			10
#define VECTOR_ID_SHORT_RX				11
#define VECTOR_ID_SHORT_TX				12

#define OPEN_SHORT_TEST		1
#define CHECK_ONLY_OPEN_TEST	1
#define CHECK_ONLY_SHORT_TEST	2

/**
 * LPM status bitmask
 */
#define MMS_LPM_FLAG_SPAY		(1 << 0)
#define MMS_LPM_FLAG_AOD		(1 << 1)

#define MMS_MODE_SPONGE_SWIPE		(1 << 1)
#define MMS_MODE_SPONGE_AOD			(1 << 2)
#define MMS_MODE_SPONGE_SINGLE_TAP		(1 << 3)
#define MMS_MODE_SPONGE_PRESS		(1 << 4)
#define MMS_MODE_SPONGE_DOUBLETAP_TO_WAKEUP	(1 << 5)

#define MMS_GESTURE_CODE_SWIPE		0x00
#define MMS_GESTURE_CODE_DOUBLE_TAP		0x01
#define MMS_GESTURE_CODE_PRESS		0x03
#define MMS_GESTURE_CODE_SINGLE_TAP		0x04

/**
 * proximity status bitmask
 */
#define MMS_MODE_HOVER_1		(1 << 0)
#define MMS_MODE_HOVER_2		(1 << 1)
#define MMS_MODE_HOVER_3		(1 << 2)
#define MMS_MODE_HOVER_LP		(1 << 3)

/* MMS_GESTURE_ID */
#define MMS_GESTURE_ID_AOD			0x00
#define MMS_GESTURE_ID_DOUBLETAP_TO_WAKEUP	0x01

#define MMS_GESTURE_ID_FOD_LONG			0x00
#define MMS_GESTURE_ID_FOD_NORMAL			0x01
#define MMS_GESTURE_ID_FOD_RELEASE	0x02
#define MMS_GESTURE_ID_FOD_OUT	0x03

typedef enum {
	SPONGE_EVENT_TYPE_SPAY			= 0x04,
	SPONGE_EVENT_TYPE_SINGLE_TAP		= 0x08,
	SPONGE_EVENT_TYPE_AOD_PRESS		= 0x09,
	SPONGE_EVENT_TYPE_AOD_LONGPRESS		= 0x0A,
	SPONGE_EVENT_TYPE_AOD_DOUBLETAB		= 0x0B,
	SPONGE_EVENT_TYPE_FOD		= 0x0F,
	SPONGE_EVENT_TYPE_FOD_RELEASE		= 0x10,
	SPONGE_EVENT_TYPE_FOD_OUT = 0x11
} SPONGE_EVENT_TYPE;

#define SPONGE_AOD_ENABLE_OFFSET 0x00
#define SPONGE_TOUCHBOX_W_OFFSET 0x02
#define SPONGE_TOUCHBOX_H_OFFSET 0x04
#define SPONGE_TOUCHBOX_X_OFFSET 0x06
#define SPONGE_TOUCHBOX_Y_OFFSET 0x08
#define SPONGE_UTC_OFFSET        0x10
#define SPONGE_FOD_PROPERTY      0x14
#define SPONGE_FOD_INFO			0x15
#define SPONGE_FOD_POSITION			0x19
#define SPONGE_FOD_RECT				0x4B
#define SPONGE_LP_DUMP_REG_ADDR            0xF0
#define SPONGE_DUMP_FORMAT_REG_OFFSET    (D_BASE_LP_DUMP_REG_ADDR + 0x00)
#define SPONGE_DUMP_NUM_REG_OFFSET       (D_BASE_LP_DUMP_REG_ADDR + 0x01)
#define SPONGE_DUMP_CUR_INDEX_REG_OFFSET (D_BASE_LP_DUMP_REG_ADDR + 0x02)
#define SPONGE_DUMP_START                (D_BASE_LP_DUMP_REG_ADDR + 0x04)

#define MMS_TS_COORDINATE_ACTION_NONE		0
#define MMS_TS_COORDINATE_ACTION_PRESS		1
#define MMS_TS_COORDINATE_ACTION_MOVE		2
#define MMS_TS_COORDINATE_ACTION_RELEASE	3

#define TWO_LEVEL_GRIP_CONCEPT
#ifdef TWO_LEVEL_GRIP_CONCEPT
#define MMS_CMD_EDGE_HANDLER 	0xAA
#define MMS_CMD_EDGE_AREA		0xAB
#define MMS_CMD_DEAD_ZONE		0xAC
#define MMS_CMD_LANDSCAPE_MODE	0xAD

#define DIFF_SCALER 1000

#define TOUCHTYPE_NORMAL		0
#define TOUCHTYPE_HOVER		1
#define TOUCHTYPE_FLIPCOVER	2
#define TOUCHTYPE_GLOVE		3
#define TOUCHTYPE_STYLUS		4
#define TOUCHTYPE_PALM		5
#define TOUCHTYPE_WET		6
#define TOUCHTYPE_PROXIMITY	7
#define TOUCHTYPE_JIG		8

#define OUT_POCKET 10
#define IN_POCKET 11

enum grip_write_mode {
	G_NONE				= 0,
	G_SET_EDGE_HANDLER		= 1,
	G_SET_EDGE_ZONE			= 2,
	G_SET_NORMAL_MODE		= 4,
	G_SET_LANDSCAPE_MODE	= 8,
	G_CLR_LANDSCAPE_MODE	= 16,
};
enum grip_set_data {
	ONLY_EDGE_HANDLER		= 0,
	GRIP_ALL_DATA			= 1,
};
#endif

enum cover_id {
	SEC_FLIP_COVER = 0,
	SEC_SVIEW_COVER,
	SEC_COVER_NOTHING1,
	SEC_SVIEW_WIRELESS,
	SEC_HEALTH_COVER,
	SEC_CHARGER_COVER,
	SEC_VIEW_WALLET,
	SEC_LED_COVER,
	SEC_CLEAR_COVER,
	SEC_QWERTY_KEYBOARD_KOR,
	SEC_QWERTY_KEYBOARD_US,
	SEC_NEON_COVER,
	SEC_ALCANTARA_COVER,
	SEC_GAMEPACK_COVER,
	SEC_LED_BACK_COVER,
	SEC_CLEAR_SIDE_VIEW_COVER,
	SEC_MINI_SVIEW_WALLET_COVER,
	SEC_MONTBLANC_COVER = 100,
};

struct mms_ts_coordinate {
	u8 id;
	u8 action;
	u16 x;
	u16 y;
	u16 p_x;
	u16 p_y;
	u16 z;
	u8 major;
	u8 minor;
	bool palm;
	u16 mcount;
	int palm_count;
	u8 left_event;
	u8 type;
	u8 pre_type;
};


/**
 * Device info structure
 */
struct mms_ts_info {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *input_dev_proximity;
	char phys[32];
	struct mms_dt_data *dtdata;
	struct pinctrl *pinctrl;
	struct completion resume_done;

	dev_t mms_dev;
	struct class *class;

	struct mutex lock;
	struct mutex lock_test;
	struct mutex lock_cmd;
	struct mutex lock_dev;
	struct mutex sponge_mutex;

	struct sec_cmd_data sec;

	struct mms_ts_coordinate coord[MAX_FINGER_NUM];
	volatile bool reset_is_on_going;
	struct delayed_work reset_work;

	bool probe_done;
	char *fw_name;

	int test_min;
	int test_max;
	int test_diff_max;

	u8 product_name[16];
	int max_x;
	int max_y;
	u8 node_x;
	u8 node_y;
	u8 node_key;
	u16 fw_ver_ic;
	u16 fw_ver_bin[8];
	u16 fw_model_ver_ic;
	u8 event_size;
	u8 event_size_type;
	int event_format;
	u16 fw_year;
	u8 fw_month;
	u8 fw_date;
	u16 pre_chksum;
	u16 rt_chksum;
	unsigned char finger_state[MAX_FINGER_NUM];
	int touch_count;

	int fod_lp_mode;
	u8 fod_tx;
	u8 fod_rx;
	u8 fod_vi_size;
	u16 fod_rect_data[4];
	u16 rect_data[4];

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

	bool tkey_enable;

	u8 nap_mode;
	u8 glove_mode;
	u8 charger_mode;
	u8 cover_mode;
	u8 cover_type;

	u8 esd_cnt;
	bool disable_esd;

	unsigned int sram_addr_num;
	u32 sram_addr[8];

	u8 *print_buf;
	int *image_buf;

	bool test_busy;
	bool cmd_busy;
	bool dev_busy;

#if MMS_USE_DEV_MODE
	struct cdev cdev;
	u8 *dev_fs_buf;
#endif

#ifdef CONFIG_VBUS_NOTIFIER
	struct notifier_block vbus_nb;
	bool ta_stsatus;
#endif

#ifdef USE_TSP_TA_CALLBACKS
	void (*register_cb)(struct tsp_callbacks *);
	struct tsp_callbacks callbacks;
#endif
	struct delayed_work ghost_check;
	u8 tsp_dump_lock;

	struct mutex modechange;
	struct delayed_work work_read_info;
	bool info_work_done;

	struct delayed_work work_print_info;
	int noise_mode;
	int wet_mode;
	int print_info_cnt_open;
	int print_info_cnt_release;

	bool lowpower_mode;
	unsigned char lowpower_flag;

	long prox_power_off;
	u8 ed_enable;
	int pocket_enable;

	volatile int power_status;
	unsigned int scrub_id;
	unsigned int scrub_x;
	unsigned int scrub_y;

	u8 check_multi;
	unsigned int multi_count;
	unsigned int comm_err_count;

	int open_short_type;
	int open_short_result;

	u32 defect_probability;
	u8 item_cmdata;
	bool check_version;
	u8 hover_event; /* keystring for protos */
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	atomic_t secure_enabled;
	atomic_t secure_pending_irqs;
	struct completion secure_powerdown;
	struct completion secure_interrupt;
#endif
};

typedef enum {
	STATE_POWER_OFF = 0,
	STATE_LPM,
	STATE_POWER_ON
} TOUCH_POWER_MODE;


enum FW_SIGN {
	NORMAL = 0,
	SIGNING = 1,
};

/**
 * Platform Data
 */
struct mms_dt_data {
	int gpio_intr;
	const char *gpio_vdd_en;
	const char *gpio_io_en;
	int bringup;
	struct regulator *vdd_io;
	const char *fw_name;
	const char *fw_name_old;
	const char *model_name;
	bool support_lpm;
	bool support_ear_detect;
	bool support_fod;
	bool enable_settings_aot;
	bool sync_reportrate_120;
	bool support_open_short_test;
	bool no_vsync;
	bool support_protos;
	bool regulator_boot_on;
	bool support_dual_fw;
	int gpio_en;

	int max_x;
	int max_y;
	int display_x;
	int display_y;
	int node_x;
	int node_y;
	int node_key;
	int event_format;
	int event_size;
	int event_size_type;
	int fod_tx;
	int fod_rx;
	int fod_vi_size;

	u32	area_indicator;
	u32	area_navigation;
	u32	area_edge;
};

/**
 * Firmware binary header info
 */
struct mms_bin_hdr {
	char	tag[8];
	u16	core_version;
	u16	section_num;
	u16	contains_full_binary;
	u16	reserved0;

	u32	binary_offset;
	u32	binary_length;

	u32	extention_offset;
	u32	reserved1;
} __attribute__ ((packed));

/**
 * Firmware image info
 */
struct mms_fw_img {
	u16	type;
	u16	version;

	u16	start_page;
	u16	end_page;

	u32	offset;
	u32	length;
} __attribute__ ((packed));

/*
 * Firmware update error code
 */
enum fw_update_errno {
	FW_ERR_FILE_READ = -4,
	FW_ERR_FILE_OPEN = -3,
	FW_ERR_FILE_TYPE = -2,
	FW_ERR_DOWNLOAD = -1,
	FW_ERR_NONE = 0,
	FW_ERR_UPTODATE = 1,
};

/*
 * Firmware file location
 */
enum fw_bin_source {
	FW_BIN_SOURCE_KERNEL = 1,
	FW_BIN_SOURCE_EXTERNAL = 2,
};

/*
 * Flash failure type
 */
enum flash_fail_type {
	FLASH_FAIL_NONE = 0,
	FLASH_FAIL_CRITICAL = 1,
	FLASH_FAIL_SECTION = 2,
};

/**
 * Declarations
 */
//main
void mms_power_reboot(struct mms_ts_info *info);
int mms_i2c_read(struct mms_ts_info *info, char *write_buf, unsigned int write_len,
			char *read_buf, unsigned int read_len);
int mms_i2c_read_next(struct mms_ts_info *info, char *read_buf, int start_idx,
			unsigned int read_len);
int mms_i2c_write(struct mms_ts_info *info, char *write_buf, unsigned int write_len);
int mms_enable(struct mms_ts_info *info);
int mms_disable(struct mms_ts_info *info);
int mms_get_ready_status(struct mms_ts_info *info);
int mms_get_fw_version(struct mms_ts_info *info, u8 *ver_buf);
int mms_get_fw_version_u16(struct mms_ts_info *info, u16 *ver_buf_u16);
int mms_disable_esd_alert(struct mms_ts_info *info);
int mms_fw_update_from_kernel(struct mms_ts_info *info, bool force, bool on_probe);
int mms_fw_update_from_storage(struct mms_ts_info *info, bool force, bool signing, const char *file_path);
int mms_fw_update_from_ffu(struct mms_ts_info *info, bool force);

//mod
int mms_power_control(struct mms_ts_info *info, int enable);
void mms_clear_input(struct mms_ts_info *info);
void mms_report_input_event(struct mms_ts_info *info, u8 sz, u8 *buf);
void mms_input_event_handler(struct mms_ts_info *info, u8 sz, u8 *buf);
int mms_custom_event_handler(struct mms_ts_info *info, u8 *rbuf, u8 size);
int sponge_read(struct mms_ts_info *info, u16 addr, u8 *buf, u8 len);
int sponge_write(struct mms_ts_info *info, u16 addr, u8 *buf, u8 len);
int mms_set_custom_library(struct mms_ts_info *info, u16 addr, u8 *buf, u8 len);
int mms_set_fod_rect(struct mms_ts_info *info);

#ifdef CONFIG_VBUS_NOTIFIER
int mms_charger_attached(struct mms_ts_info *info, bool status);
#endif

int mms_parse_dt(struct device *dev, struct mms_ts_info *info);
void mms_config_input(struct mms_ts_info *info);
int mms_lowpower_mode(struct mms_ts_info *info, u8 on);
int mms_set_aod_rect(struct mms_ts_info *info);
//fw_update
int mip4_ts_flash_fw(struct mms_ts_info *info, const u8 *fw_data, size_t fw_size,
			bool force, bool section, bool on_probe);
int mip4_ts_bin_fw_version(struct mms_ts_info *info, const u8 *fw_data, size_t fw_size, u8 *ver_buf);

//test
#if MMS_USE_DEV_MODE
int mms_dev_create(struct mms_ts_info *info);
int mms_get_log(struct mms_ts_info *info);
#endif
int mms_run_test(struct mms_ts_info *info, u8 test_type);
int mms_get_image(struct mms_ts_info *info, u8 image_type);
#if MMS_USE_TEST_MODE
int mms_sysfs_create(struct mms_ts_info *info);
void mms_sysfs_remove(struct mms_ts_info *info);
static const struct attribute_group mms_test_attr_group;
#endif

//cmd
int mms_sysfs_cmd_create(struct mms_ts_info *info);
void mms_sysfs_cmd_remove(struct mms_ts_info *info);

#ifdef USE_TSP_TA_CALLBACKS
void mms_charger_status_cb(struct tsp_callbacks *cb, int status);
void mms_register_callback(struct tsp_callbacks *cb);
#endif

#ifdef CONFIG_VBUS_NOTIFIER
int mms_vbus_notification(struct notifier_block *nb,
		unsigned long cmd, void *data);
#endif

#ifdef COVER_MODE
int set_cover_type(struct mms_ts_info *info);
#endif
void set_grip_data_to_ic(struct mms_ts_info *info, u8 flag);
int mms_reinit(struct mms_ts_info *info);

void minority_report_calculate_cmdata(struct mms_ts_info *info);
void minority_report_sync_latest_value(struct mms_ts_info *info);

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif

//extern unsigned int lcdtype;

#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
extern void mms_trustedui_mode_on(void);
#endif

#endif /* __MELFAS_MMS400_H */
