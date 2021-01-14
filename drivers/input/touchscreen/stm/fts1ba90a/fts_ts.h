#ifndef _LINUX_FTS_TS_H_
#define _LINUX_FTS_TS_H_

#include <linux/device.h>
#include <linux/input/sec_cmd.h>
#include <linux/wakelock.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/spu-verify.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/serio.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/ctype.h>
#include <linux/i2c-dev.h>
#include <linux/fs.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/input/mt.h>
#ifdef CONFIG_SEC_SYSFS
#include <linux/sec_sysfs.h>
#endif

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
#include <linux/pm_runtime.h>
#include <linux/atomic.h>
#include <linux/input/sec_secure_touch.h>
#endif

#ifdef CONFIG_VBUS_NOTIFIER
#include <linux/vbus_notifier.h>
#endif

#define USE_OPEN_CLOSE

#include <linux/input/sec_tclm_v2.h>
#ifdef CONFIG_INPUT_TOUCHSCREEN_TCLMV2
#define TCLM_CONCEPT
#endif
#include <linux/power_supply.h>

#define USE_POR_AFTER_I2C_RETRY

#define BRUSH_Z_DATA			63 /* for ArtCanvas */

#undef USE_OPEN_DWORK

#ifdef USE_OPEN_DWORK
#define TOUCH_OPEN_DWORK_TIME		10
#endif
#define TOUCH_PRINT_INFO_DWORK_TIME	30000	/* 30s */

#define FTS_TS_LOCATION_DETECT_SIZE	6

#define FIRMWARE_IC			"fts_ic"
#define FTS_MAX_FW_PATH			64
#define FTS_TS_DRV_NAME			"fts_touch"
#define FTS_TS_DRV_VERSION		"0100"

#define FTS_TS_I2C_RETRY_CNT		3

/*
 * fts1b: ID0: 0x39
 * fts9c: ID0: 0x50
 */
#define FTS_ID0				0x39
#define FTS_ID1				0x36

#define FTS_FIFO_MAX			32
#define FTS_EVENT_SIZE			16
#define FTS_VERSION_SIZE		9

#define PRESSURE_MIN			0
#define PRESSURE_MAX			127
#define FINGER_MAX			10
#define AREA_MIN			PRESSURE_MIN
#define AREA_MAX			PRESSURE_MAX

#define INT_ENABLE			1
#define INT_DISABLE			0

#define FTS_CMD_SPONGE_ACCESS				0x0000

/* COMMANDS */
#define FTS_CMD_SENSE_ON				0x10
#define FTS_CMD_SENSE_OFF				0x11
#define FTS_CMD_SW_RESET				0x12
#define FTS_CMD_FORCE_CALIBRATION			0x13
#define FTS_CMD_FACTORY_PANELCALIBRATION		0x14

#define FTS_READ_GPIO_STATUS				0x20
#define FTS_READ_FIRMWARE_INTEGRITY			0x21
#define FTS_READ_DEVICE_ID				0x22
#define FTS_READ_PANEL_INFO				0x23
#define FTS_READ_FW_VERSION				0x24

#define FTS_CMD_SET_GET_TOUCH_FUNCTION			0x30
#define FTS_CMD_SET_GET_OPMODE				0x31
#define FTS_CMD_SET_GET_CHARGER_MODE			0x32
#define FTS_CMD_SET_GET_NOISE_MODE			0x33
#define FTS_CMD_SET_GET_REPORT_RATE			0x34
#define FTS_CMD_SET_GET_TOUCH_MODE_FOR_THRESHOLD	0x35
#define FTS_CMD_SET_GET_TOUCH_THRESHOLD			0x36
#define FTS_CMD_SET_GET_KEY_THRESHOLD			0x37
#define FTS_CMD_SET_GET_COVERTYPE			0x38
#define FTS_CMD_WRITE_WAKEUP_GESTURE			0x39
#define FTS_CMD_WRITE_COORDINATE_FILTER			0x3A
#define FTS_CMD_SET_FOD_FINGER_MERGE			0x3B

#define FTS_READ_ONE_EVENT				0x60
#define FTS_READ_ALL_EVENT				0x61
#define FTS_CMD_CLEAR_ALL_EVENT				0x62

#define FTS_CMD_SENSITIVITY_MODE			0x70
#define FTS_READ_SENSITIVITY_VALUE			0x72
#define FTS_CMD_RUN_SRAM_TEST				0x78

#define FTS_CMD_SET_FUNCTION_ONOFF			0xC1

/* FTS SPONGE COMMAND */
#define FTS_CMD_SPONGE_READ_WRITE_CMD			0xAA
#define FTS_CMD_SPONGE_NOTIFY_CMD			0xC0

#define FTS_CMD_SPONGE_OFFSET_MODE			0x00
#define FTS_CMD_SPONGE_OFFSET_AOD_RECT			0x02
#define FTS_CMD_SPONGE_OFFSET_UTC			0x10
#define FTS_CMD_SPONGE_PRESS_PROPERTY			0x14
#define FTS_CMD_SPONGE_FOD_INFO				0x15
#define FTS_CMD_SPONGE_FOD_POSITION			0x19
#define FTS_CMD_SPONGE_FOD_RECT				0x4B
#define FTS_CMD_SPONGE_LP_DUMP				0xF0

/* First byte of ONE EVENT */
#define FTS_EVENT_PASS_REPORT				0x03
#define FTS_EVENT_STATUS_REPORT				0x43
#define FTS_EVENT_JITTER_RESULT 			0x49
#define FTS_EVENT_ERROR_REPORT				0xF3

/* Test Event */
#define FTS_EVENT_JITTER_MUTUAL_TEST			0x01
#define FTS_EVENT_JITTER_SELF_TEST			0x02

#define FTS_EVENT_JITTER_MUTUAL_MAX			0x01
#define FTS_EVENT_JITTER_MUTUAL_MIN			0x02
#define FTS_EVENT_JITTER_SELF_TX_P2P			0x05
#define FTS_EVENT_JITTER_SELF_RX_P2P			0x06

#define FTS_EVENT_SRAM_TEST_RESULT			0xD0

/* Status Event */
#define FTS_COORDINATE_EVENT			0
#define FTS_STATUS_EVENT			1
#define FTS_GESTURE_EVENT			2
#define FTS_VENDOR_EVENT			3

#define FTS_GESTURE_CODE_SPAY			0x00
#define FTS_GESTURE_CODE_DOUBLE_TAP		0x01

#define FTS_COORDINATE_ACTION_NONE		0
#define FTS_COORDINATE_ACTION_PRESS		1
#define FTS_COORDINATE_ACTION_MOVE		2
#define FTS_COORDINATE_ACTION_RELEASE		3

#define FTS_EVENT_TOUCHTYPE_NORMAL		0
#define FTS_EVENT_TOUCHTYPE_HOVER		1
#define FTS_EVENT_TOUCHTYPE_FLIPCOVER		2
#define FTS_EVENT_TOUCHTYPE_GLOVE		3
#define FTS_EVENT_TOUCHTYPE_STYLUS		4
#define FTS_EVENT_TOUCHTYPE_PALM		5
#define FTS_EVENT_TOUCHTYPE_WET			6
#define FTS_EVENT_TOUCHTYPE_PROXIMITY		7
#define FTS_EVENT_TOUCHTYPE_JIG			8

/* Status - ERROR event */
#define FTS_EVENT_STATUSTYPE_CMDDRIVEN		0
#define FTS_EVENT_STATUSTYPE_ERROR		1
#define FTS_EVENT_STATUSTYPE_INFORMATION	2
#define FTS_EVENT_STATUSTYPE_USERINPUT		3
#define FTS_EVENT_STATUSTYPE_VENDORINFO		7

#define FTS_ERR_EVNET_CORE_ERR			0x00
#define FTS_ERR_EVENT_QUEUE_FULL		0x01
#define FTS_ERR_EVENT_ESD			0x02

/* Status - Information report */
#define FTS_INFO_READY_STATUS			0x00
#define FTS_INFO_WET_MODE			0x01
#define FTS_INFO_NOISE_MODE			0x02


// Scan mode for A0 command
#define FTS_SCAN_MODE_SCAN_OFF			0
#define FTS_SCAN_MODE_MS_SS_SCAN		(1 << 0)
#define FTS_SCAN_MODE_KEY_SCAN			(1 << 1)
#define FTS_SCAN_MODE_HOVER_SCAN		(1 << 2)
#define FTS_SCAN_MODE_FORCE_TOUCH_SCAN		(1 << 4)
#define FTS_SCAN_MODE_DEFAULT			FTS_SCAN_MODE_MS_SS_SCAN


/* Control Command */

// For 0x30 command - touch type
#define FTS_TOUCHTYPE_BIT_TOUCH		(1 << 0)
#define FTS_TOUCHTYPE_BIT_HOVER		(1 << 1)
#define FTS_TOUCHTYPE_BIT_COVER		(1 << 2)
#define FTS_TOUCHTYPE_BIT_GLOVE		(1 << 3)
#define FTS_TOUCHTYPE_BIT_STYLUS	(1 << 4)
#define FTS_TOUCHTYPE_BIT_PALM		(1 << 5)
#define FTS_TOUCHTYPE_BIT_WET		(1 << 6)
#define FTS_TOUCHTYPE_BIT_PROXIMITY	(1 << 7)
#define FTS_TOUCHTYPE_DEFAULT_ENABLE	(FTS_TOUCHTYPE_BIT_TOUCH | FTS_TOUCHTYPE_BIT_PALM | FTS_TOUCHTYPE_BIT_WET)

// For 0x31 command - touch operation mode
#define FTS_OPMODE_NORMAL		0
#define FTS_OPMODE_LOWPOWER		1

// For 0x32 command - charger mode
#define FTS_CHARGER_MODE_NORMAL				0
#define FTS_CHARGER_MODE_WIRE_CHARGER			1
#define FTS_CHARGER_MODE_WIRELESS_CHARGER		2
#define FTS_CHARGER_MODE_WIRELESS_BATTERY_PACK		3

// For 0xC1 command - on/off function
#define FTS_FUNCTION_ENABLE_SIP_MODE			0x00
#define FTS_FUNCTION_SET_MONITOR_NOISE_MODE		0x01
#define FTS_FUNCTION_ENABLE_BRUSH_MODE			0x02
#define FTS_FUNCTION_ENABLE_DEAD_ZONE			0x04	/* *#0*# */
#define FTS_FUNCTION_ENABLE_SPONGE_LIB			0x05
#define FTS_FUNCTION_EDGE_AREA				0x07	/* used for grip cmd */
#define FTS_FUNCTION_DEAD_ZONE				0x08	/* used for grip cmd */
#define FTS_FUNCTION_LANDSCAPE_MODE			0x09	/* used for grip cmd */
#define FTS_FUNCTION_LANDSCAPE_TOP_BOTTOM		0x0A	/* used for grip cmd */
#define FTS_FUNCTION_EDGE_HANDLER			0x0C	/* used for grip cmd */
#define FTS_FUNCTION_ENABLE_VSYNC			0x0D
#define FTS_FUNCTION_SET_TOUCHABLE_AREA			0x0F
#define FTS_FUNCTION_SET_NOTE_MODE			0x10
#define FTS_FUNCTION_SET_GAME_MODE			0x11

/* FTS DEBUG FLAG */
#define FTS_DEBUG_PRINT_I2C_READ_CMD			0x04
#define FTS_DEBUG_PRINT_I2C_WRITE_CMD			0x08
#define FTS_DEBUG_SEND_UEVENT				0x80

#define FTS_RETRY_COUNT					10
#define FTS_DELAY_NVWRITE				50

/* gesture SF */
#define FTS_GESTURE_SAMSUNG_FEATURE			1

/* gesture type */
#define FTS_SPONGE_EVENT_SWIPE_UP			0
#define FTS_SPONGE_EVENT_DOUBLETAP			1
#define FTS_SPONGE_EVENT_PRESS				3
#define FTS_SPONGE_EVENT_SINGLETAP			4

/* gesture ID */
#define FTS_SPONGE_EVENT_GESTURE_ID_AOD			0
#define FTS_SPONGE_EVENT_GESTURE_ID_DOUBLETAP_TO_WAKEUP	1
#define FTS_SPONGE_EVENT_GESTURE_ID_FOD_LONG		0
#define FTS_SPONGE_EVENT_GESTURE_ID_FOD_NORMAL		1
#define FTS_SPONGE_EVENT_GESTURE_ID_FOD_RELEASE		2
#define FTS_SPONGE_EVENT_GESTURE_ID_FOD_OUT		3

#define FTS_ENABLE					1
#define FTS_DISABLE					0

/* sponge mode */
#define FTS_MODE_SPAY					(1 << 1)
#define FTS_MODE_AOD					(1 << 2)
#define FTS_MODE_SINGLETAP				(1 << 3)
#define FTS_MODE_PRESS					(1 << 4)
#define FTS_MODE_DOUBLETAP_WAKEUP			(1 << 5)

typedef enum {
	SPONGE_EVENT_TYPE_SPAY			= 0x04,
	SPONGE_EVENT_TYPE_SINGLE_TAP		= 0x08,
	SPONGE_EVENT_TYPE_AOD_PRESS		= 0x09,
	SPONGE_EVENT_TYPE_AOD_LONGPRESS		= 0x0A,
	SPONGE_EVENT_TYPE_AOD_DOUBLETAB		= 0x0B,
	SPONGE_EVENT_TYPE_FOD			= 0x0F,
	SPONGE_EVENT_TYPE_FOD_RELEASE		= 0x10,
	SPONGE_EVENT_TYPE_FOD_OUT		= 0x11,
} SPONGE_EVENT_TYPE;

#define FTS_MAX_X_RESOLUTION	1599
#define FTS_MAX_Y_RESOLUTION	2559
#define FTS_MAX_NUM_FORCE		53	/* Number of TX CH */
#define FTS_MAX_NUM_SENSE		33	/* Number of RX CH */

enum external_noise_mode {
	EXT_NOISE_MODE_NONE		= 0,
	EXT_NOISE_MODE_MONITOR		= 1,	/* for dex mode */
	EXT_NOISE_MODE_MAX,			/* add new mode above this line */
};

enum fts_error_return {
	FTS_NOT_ERROR = 0,
	FTS_I2C_ERROR,
	FTS_ERROR_INVALID_CHIP_ID,
	FTS_ERROR_INVALID_CHIP_VERSION_ID,
	FTS_ERROR_INVALID_SW_VERSION,
	FTS_ERROR_EVENT_ID,
	FTS_ERROR_FW_CORRUPTION,
	FTS_ERROR_TIMEOUT,
	FTS_ERROR_TIMEOUT_ZERO,
	FTS_ERROR_FW_UPDATE_FAIL,
	FTS_ERROR_BROKEN_OSC_TRIM,
};

enum fts_fw_update_status {
	FTS_NOT_UPDATE = 10,
	FTS_NEED_FW_UPDATE,
	FTS_NEED_CALIBRATION_ONLY,
	FTS_NEED_FW_UPDATE_N_CALIBRATION,
};

struct fts_sponge_information {
	u8 sponge_use;  // 0 : don't use, 1 : use
	u16 sponge_ver;
	u16 sponge_model_number;
	u16 sponge_model_name_size;
	u8 sponge_model_name[32];
} __packed;

enum grip_write_mode {
	G_NONE				= 0,
	G_SET_EDGE_HANDLER		= 1,
	G_SET_EDGE_ZONE			= 2,
	G_SET_NORMAL_MODE		= 4,
	G_SET_LANDSCAPE_MODE		= 8,
	G_CLR_LANDSCAPE_MODE		= 16,
};
enum grip_set_data {
	ONLY_EDGE_HANDLER		= 0,
	GRIP_ALL_DATA			= 1,
};

/**
 * struct fts_finger - Represents fingers.
 * @ state: finger status (Event ID).
 * @ mcount: moving counter for debug.
 */
struct fts_finger {
	u8 id;
	u8 ttype;
	u8 prev_ttype;
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
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num;
};

enum switch_system_mode {
	TO_TOUCH_MODE			= 0,
	TO_LOWPOWER_MODE		= 1,
};

enum tsp_power_mode {
	FTS_POWER_STATE_POWERDOWN = 0,
	FTS_POWER_STATE_LOWPOWER,
	FTS_POWER_STATE_ACTIVE,
};

enum fts_cover_id {
	FTS_FLIP_WALLET = 0,
	FTS_VIEW_COVER,
	FTS_COVER_NOTHING1,
	FTS_VIEW_WIRELESS,
	FTS_COVER_NOTHING2,
	FTS_CHARGER_COVER,
	FTS_VIEW_WALLET,
	FTS_LED_COVER,
	FTS_CLEAR_FLIP_COVER,
	FTS_QWERTY_KEYBOARD_EUR,
	FTS_QWERTY_KEYBOARD_KOR,
	FTS_MONTBLANC_COVER = 100,
};

enum fts_config_value_feature {
	FTS_CFG_NONE = 0,
	FTS_CFG_APWR = 1,
	FTS_CFG_AUTO_TUNE_PROTECTION = 2,
};

enum {
	SPECIAL_EVENT_TYPE_SPAY					= 0x04,
	SPECIAL_EVENT_TYPE_PRESSURE_TOUCHED			= 0x05,
	SPECIAL_EVENT_TYPE_PRESSURE_RELEASED			= 0x06,
	SPECIAL_EVENT_TYPE_AOD					= 0x08,
	SPECIAL_EVENT_TYPE_AOD_PRESS				= 0x09,
	SPECIAL_EVENT_TYPE_AOD_LONGPRESS			= 0x0A,
	SPECIAL_EVENT_TYPE_AOD_DOUBLETAB			= 0x0B,
	SPECIAL_EVENT_TYPE_AOD_HOMEKEY_PRESS			= 0x0C,
	SPECIAL_EVENT_TYPE_AOD_HOMEKEY_RELEASE			= 0x0D,
	SPECIAL_EVENT_TYPE_AOD_HOMEKEY_RELEASE_NO_HAPTIC	= 0x0E
};

enum fts_system_information_address {
	FTS_SI_CONFIG_CHECKSUM = 0x58, /* 4 bytes */
};

enum fts_ito_test_mode {
	OPEN_TEST = 0,
	OPEN_SHORT_CRACK_TEST,
	SAVE_MISCAL_REF_RAW,
};

enum fts_ito_test_result {
	ITO_PASS = 0,
	ITO_FAIL,
	ITO_FAIL_OPEN,
	ITO_FAIL_SHORT,
	ITO_FAIL_MICRO_OPEN,
	ITO_FAIL_MICRO_SHORT,
};

#define UEVENT_OPEN_SHORT_PASS		1
#define UEVENT_OPEN_SHORT_FAIL		2

/* ----------------------------------------
 * write 0xE4 [ 11 | 10 | 01 | 00 ]
 * MSB <-------------------> LSB
 * read 0xE4
 * mapping sequnce : LSB -> MSB
 * struct sec_ts_test_result {
 * * assy : front + OCTA assay
 * * module : only OCTA
 *	 union {
 *		 struct {
 *			 u8 assy_count:2;	-> 00
 *			 u8 assy_result:2;	-> 01
 *			 u8 module_count:2;	-> 10
 *			 u8 module_result:2;	-> 11
 *		 } __attribute__ ((packed));
 *		 u8 data[1];
 *	 };
 *};
 * ----------------------------------------
 */
struct fts_ts_test_result {
	union {
		struct {
			u8 assy_count:2;
			u8 assy_result:2;
			u8 module_count:2;
			u8 module_result:2;
		} __packed;
		u8 data[1];
	};
};

#define TEST_OCTA_MODULE	1
#define TEST_OCTA_ASSAY		2

#define TEST_OCTA_NONE		0
#define TEST_OCTA_FAIL		1
#define TEST_OCTA_PASS		2

#define SEC_OFFSET_SIGNATURE		0x59525446

enum offset_fac_position {
	OFFSET_FAC_NOSAVE		= 0,	// FW index 0
	OFFSET_FAC_SUB			= 1,	// FW Index 2
	OFFSET_FAC_MAIN			= 2,	// FW Index 3
	OFFSET_FAC_SVC			= 3,	// FW Index 4
};

enum offset_fw_position {
	OFFSET_FW_NOSAVE		= 0,
	OFFSET_FW_SDC			= 1,
	OFFSET_FW_SUB			= 2,
	OFFSET_FW_MAIN			= 3,
	OFFSET_FW_SVC			= 4,
};

#define FTS_ITO_RESULT_PRINT_SIZE	1024

struct fts_sec_panel_test_result {
	u8 flag;
	u8 num_of_test;
	u16 max_of_tx_gap;
	u16 max_of_rx_gap;
	u8 tx_of_txmax_gap;
	u8 rx_of_txmax_gap;
	u8 tx_of_rxmax_gap;
	u8 rx_of_rxmax_gap;
} __packed;

struct fts_sdc_panel_test_result {
	u16 max_of_tx_gap;
	u16 max_of_rx_gap;
	u8 tx_of_txmax_gap;
	u8 rx_of_txmax_gap;
	u8 tx_of_rxmax_gap;
	u8 rx_of_rxmax_gap;
} __packed;

/* 16 byte */
struct fts_event_coordinate {
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
	u8 left_event:6;
	u8 ttype_1_0:2;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num:4;
	u8 reserved_10:4;
	u8 reserved_11;
	u8 reserved_12;
	u8 reserved_13;
	u8 reserved_14;
	u8 reserved_15;
} __packed;

/* 16 byte */
struct fts_event_status {
	u8 eid:2;
	u8 stype:4;
	u8 sf:2;
	u8 status_id;
	u8 status_data_1;
	u8 status_data_2;
	u8 status_data_3;
	u8 status_data_4;
	u8 status_data_5;
	u8 left_event_5_0:6;
	u8 reserved:2;
	u8 reserved_8;
	u8 reserved_9;
	u8 reserved_10;
	u8 reserved_11;
	u8 reserved_12;
	u8 reserved_13;
	u8 reserved_14;
	u8 reserved_15;
} __packed;


/* 8 byte */
struct fts_gesture_status {
	u8 eid:2;
	u8 stype:4;
	u8 sf:2;
	u8 gesture_id;
	u8 gesture_data_1;
	u8 gesture_data_2;
	u8 gesture_data_3;
	u8 gesture_data_4;
	u8 reserved_1;
	u8 left_event_5_0:6;
	u8 reserved_2:2;
	u8 reserved_8;
	u8 reserved_9;
	u8 reserved_10;
	u8 reserved_11;
	u8 reserved_12;
	u8 reserved_13;
	u8 reserved_14;
	u8 reserved_15;
} __packed;

struct FTS_SyncFrameHeader {
	u8		header; // 0
	u8		host_data_mem_id; // 1
	u16	  cnt;  // 2~3
	u8		dbg_frm_len;  // 4
	u8		ms_force_len; // 5
	u8		ms_sense_len; // 6
	u8		ss_force_len; // 7
	u8		ss_sense_len; // 8
	u8		key_len;  // 9
	u16	  reserved1;  // 10~11
	u32	  reserved2;  // 12~15
} __packed;

struct fts_i2c_platform_data {
	bool support_mt_pressure;
	bool support_dex;
	bool enable_settings_aot;
	bool sync_reportrate_120;
	bool enable_vbus_noti;
	bool support_fod;
	bool disable_vsync_scan;
	int max_x;
	int max_y;
	int display_x;
	int display_y;
	const char *firmware_name;
	const char *regulator_dvdd;
	const char *regulator_avdd;

	struct pinctrl *pinctrl;
	struct pinctrl_state	*pins_default;
	struct pinctrl_state	*pins_sleep;

	int (*power)(void *data, bool on);
	void (*register_cb)(void *);
	void (*enable_sync)(bool on);
	u8 (*get_ddi_type)(void);	/* to indentify ddi type */

	int tsp_icid;	/* IC Vendor */
	int tsp_id;	/* Panel Vendor */
	int device_id;	/* Device id */

	int irq_gpio;	/* Interrupt GPIO */
	unsigned int irq_type;

	int gpio_scl;
	int gpio_sda;

	int bringup;

	bool chip_on_board;
	u32 area_indicator;
	u32 area_navigation;
	u32 area_edge;
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	int ss_touch_num;
#endif
};

struct fts_ts_info {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *input_dev_pad;
	struct input_dev *input_dev_touch;

	int irq;
	int irq_type;
	bool irq_enabled;
	struct fts_i2c_platform_data *board;
#ifdef CONFIG_VBUS_NOTIFIER
	struct notifier_block vbus_nb;
#endif
	struct mutex lock;
	bool probe_done;

	struct sec_cmd_data sec;
	int SenseChannelLength;
	int ForceChannelLength;
	short *pFrame;
	u8 *cx_data;
	u8 *ito_result;
	struct fts_ts_test_result test_result;
	u8 disassemble_count;
	u8 fac_nv;

	struct sec_tclm_data *tdata;
	bool is_cal_done;

	bool fw_corruption;
	bool osc_trim_error;
	bool hover_ready;
	bool hover_enabled;
	bool glove_enabled;
	bool flip_enable;
	unsigned int cover_type;
	u8 cover_cmd;
	u8 external_noise_mode;
	u8 brush_mode;
	u8 touchable_area;

	int ICXResolution;
	int ICYResolution;

	volatile u8 touch_noise_status;
	u8 touch_noise_reason;

	u8 touch_opmode;
	u16 touch_functions;
	u8 charger_mode;

	u8 scan_mode;
	u8 vsync_scan;

	u8 lowpower_flag;
	u8 fod_lp_mode;
	bool wet_mode;
	volatile int fts_power_state;
	int wakeful_edge_side;
	struct completion resume_done;
	struct wake_lock wakelock;

	unsigned int noise_count;		/* noise mode count */

	int touch_count;
	struct fts_finger finger[FINGER_MAX];

	int retry_hover_enable_after_wakeup;

	int fw_version_of_ic;			/* firmware version of IC */
	int fw_version_of_bin;			/* firmware version of binary */
	int config_version_of_ic;		/* Config release data from IC */
	int config_version_of_bin;		/* Config release data from IC */
	u16 fw_main_version_of_ic;	/* firmware main version of IC */
	u16 fw_main_version_of_bin;	/* firmware main version of binary */
	u8 project_id_of_ic;
	u8 project_id_of_bin;
	u8 ic_name_of_ic;
	u8 ic_name_of_bin;
	u8 module_version_of_ic;
	u8 module_version_of_bin;
	int tspid_val;
	int tspid2_val;

#ifdef USE_OPEN_DWORK
	struct delayed_work open_work;
#endif
	struct delayed_work work_read_nv;

	struct delayed_work work_print_info;
	u32 print_info_cnt_open;
	u32 print_info_cnt_release;

	unsigned int delay_time;
	unsigned int debug_string;
	struct delayed_work reset_work;
	struct delayed_work work_read_info;
	struct delayed_work debug_work;
	bool rawdata_read_lock;
	volatile bool reset_is_on_going;
	volatile bool shutdown_is_on_going;

	unsigned int scrub_id;
	unsigned int scrub_x;
	unsigned int scrub_y;
	u8 press_prop;
	int fod_x;
	int fod_y;
	int fod_vi_size;

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	atomic_t st_enabled;
	atomic_t st_pending_irqs;
	struct completion st_powerdown;
	struct completion st_interrupt;
	struct sec_touch_driver *ss_drv;
#endif
	struct mutex i2c_mutex;
	struct mutex irq_mutex;
	struct mutex device_mutex;
	struct mutex eventlock;
	struct mutex sponge_mutex;
	struct mutex wait_for;
	bool reinit_done;
	bool info_work_done;

	u8 ddi_type;

	const char *firmware_name;

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

	u16 rect_data[4];
	u16 fod_rect_data[4];
	u8 ito_test[4];
	u8 check_multi;
	unsigned int multi_count;
	unsigned int wet_count;
	unsigned int dive_count;
	unsigned int comm_err_count;
	unsigned int checksum_result;
	unsigned int all_finger_count;
	unsigned int all_aod_tap_count;
	unsigned int all_spay_count;

	u8 factory_position;
	int proc_size;
	char *cmoffset_sdc_proc;
	char *cmoffset_sub_proc;
	char *cmoffset_main_proc;

	int prox_power_off;

	bool fix_active_mode;
	bool touch_aging_mode;

	int rawcap_max;
	int rawcap_max_tx;
	int rawcap_max_rx;
	int rawcap_min;
	int rawcap_min_tx;
	int rawcap_min_rx;

	/* thermistor */
	union power_supply_propval psy_value;
	struct power_supply *psy;
	u8 tsp_temp_data;
	bool tsp_temp_data_skip;

	int (*stop_device)(struct fts_ts_info *info, bool lpmode);
	int (*start_device)(struct fts_ts_info *info);

	int (*fts_write_reg)(struct fts_ts_info *info, u8 *reg, u16 num_com);
	int (*fts_read_reg)(struct fts_ts_info *info, u8 *reg, int cnum, u8 *buf, int num);
	int (*fts_systemreset)(struct fts_ts_info *info, unsigned int msec);
	int (*fts_wait_for_ready)(struct fts_ts_info *info);
	void (*fts_command)(struct fts_ts_info *info, u8 cmd, bool checkEcho);
	int (*fts_get_version_info)(struct fts_ts_info *info);
	int (*fts_get_sysinfo_data)(struct fts_ts_info *info, u8 sysinfo_addr, u8 read_cnt, u8 *data);

	int (*fts_read_from_sponge)(struct fts_ts_info *info, u16 offset, u8 *data, int length);
	int (*fts_write_to_sponge)(struct fts_ts_info *info, u16 offset, u8 *data, int length);
};

int fts_fw_update_on_probe(struct fts_ts_info *info);
int fts_fw_update_on_hidden_menu(struct fts_ts_info *info, int update_type);
void fts_reinit(struct fts_ts_info *info, bool delay);
int fts_execute_autotune(struct fts_ts_info *info, bool IsSaving);
int fts_fw_wait_for_echo_event(struct fts_ts_info *info, u8 *cmd, u8 cmd_cnt, int delay);
int fts_get_tsp_test_result(struct fts_ts_info *info);
void fts_interrupt_set(struct fts_ts_info *info, int enable);
void fts_release_all_finger(struct fts_ts_info *info);
void fts_delay(unsigned int ms);
int fts_set_opmode(struct fts_ts_info *info, u8 mode);
int fts_set_scanmode(struct fts_ts_info *info, u8 scan_mode);
int fts_osc_trim_recovery(struct fts_ts_info *info);
int fts_get_miscal_data(struct fts_ts_info *info, short *min, short *max);
void fts_set_grip_data_to_ic(struct fts_ts_info *info, u8 flag);

#ifdef TCLM_CONCEPT
int fts_tclm_data_read(struct i2c_client *client, int address);
int fts_tclm_data_write(struct i2c_client *client, int address);
int fts_tclm_execute_force_calibration(struct i2c_client *client, int cal_mode);
#endif
int set_nvm_data(struct fts_ts_info *info, u8 type, u8 *buf);
int get_nvm_data(struct fts_ts_info *info, int type, u8 *nvdata);

int fts_panel_ito_test(struct fts_ts_info *info, int testmode);
int fts_set_external_noise_mode(struct fts_ts_info *info, u8 mode);
int fts_fix_active_mode(struct fts_ts_info *info, bool enable);

#ifndef CONFIG_SEC_SYSFS
extern struct class *sec_class;
#endif
#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif
#if defined(CONFIG_DISPLAY_SAMSUNG)
extern int get_lcd_attached(char *mode);
#endif
#if defined(CONFIG_EXYNOS_DECON_FB)
extern int get_lcd_info(char *arg);
#endif

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
extern struct tsp_dump_callbacks dump_callbacks;
#endif

#endif /* _LINUX_FTS_TS_H_ */
