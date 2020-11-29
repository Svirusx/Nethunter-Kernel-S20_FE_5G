/* drivers/input/sec_input/sec_input.h
 *
 * Core file for Samsung input device driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

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

enum external_noise_mode {
	EXT_NOISE_MODE_NONE		= 0,
	EXT_NOISE_MODE_MONITOR		= 1,	/* for dex mode */
	EXT_NOISE_MODE_MAX,			/* add new mode above this line */
};

enum wireless_charger_param {
	TYPE_WIRELESS_CHARGER_NONE	= 0,
	TYPE_WIRELESS_CHARGER		= 1,
};

/* FACTORY TEST RESULT SAVING FUNCTION
 * bit 3 ~ 0 : OCTA Assy
 * bit 7 ~ 4 : OCTA module
 * param[0] : OCTA modue(1) / OCTA Assy(2)
 * param[1] : TEST NONE(0) / TEST FAIL(1) / TEST PASS(2) : 2 bit
 */
#define TEST_OCTA_MODULE 1
#define TEST_OCTA_ASSAY  2
#define TEST_OCTA_NONE  0
#define TEST_OCTA_FAIL  1
#define TEST_OCTA_PASS  2

/*
 * write 0xE4 [ 11 | 10 | 01 | 00 ]
 * MSB <-------------------> LSB
 * read 0xE4
 * mapping sequnce : LSB -> MSB
 * struct sec_ts_test_result {
 * * assy : front + OCTA assay
 * * module : only OCTA
 *  union {
 *   struct {
 *    u8 assy_count:2;  -> 00
 *    u8 assy_result:2;  -> 01
 *    u8 module_count:2; -> 10
 *    u8 module_result:2; -> 11
 *   } __attribute__ ((packed));
 *   unsigned char data[1];
 *  };
 *};
 */

struct sec_ts_test_result {
 union {
  struct {
   u8 assy_count:2;
   u8 assy_result:2;
   u8 module_count:2;
   u8 module_result:2;
  } __attribute__ ((packed));
  unsigned char data[1];
 };
};

typedef enum {
	SPONGE_EVENT_TYPE_SPAY			= 0x04,
	SPONGE_EVENT_TYPE_SINGLE_TAP		= 0x08,
	SPONGE_EVENT_TYPE_AOD_PRESS		= 0x09,
	SPONGE_EVENT_TYPE_AOD_LONGPRESS		= 0x0A,
	SPONGE_EVENT_TYPE_AOD_DOUBLETAB		= 0x0B,
	SPONGE_EVENT_TYPE_AOD_HOMEKEY_PRESS	= 0x0C,
	SPONGE_EVENT_TYPE_AOD_HOMEKEY_RELEASE	= 0x0D,
	SPONGE_EVENT_TYPE_AOD_HOMEKEY_RELEASE_NO_HAPTIC	= 0x0E
} SPONGE_EVENT_TYPE;

/* SEC_TS_DEBUG : Print event contents */
#define SEC_TS_DEBUG_PRINT_ALLEVENT  0x01
#define SEC_TS_DEBUG_PRINT_ONEEVENT  0x02
#define SEC_TS_DEBUG_PRINT_I2C_READ_CMD  0x04
#define SEC_TS_DEBUG_PRINT_I2C_WRITE_CMD 0x08
#define SEC_TS_DEBUG_SEND_UEVENT  0x80

#define CMD_RESULT_WORD_LEN		10

#define SEC_TS_I2C_RETRY_CNT		3
#define SEC_TS_WAIT_RETRY_CNT		100

#define SEC_TS_MODE_SPONGE_SWIPE		(1 << 1)
#define SEC_TS_MODE_SPONGE_AOD			(1 << 2)
#define SEC_TS_MODE_SPONGE_SINGLE_TAP		(1 << 3)
#define SEC_TS_MODE_SPONGE_PRESS		(1 << 4)
#define SEC_TS_MODE_SPONGE_DOUBLETAP_TO_WAKEUP	(1 << 5)

/* SEC_TS SPONGE OPCODE COMMAND */
#define SEC_TS_CMD_SPONGE_OFFSET_UTC   0x10
#define SEC_TS_CMD_SPONGE_PRESS_PROPERTY  0x14
#define SEC_TS_CMD_SPONGE_FOD_INFO   0x15
#define SEC_TS_CMD_SPONGE_FOD_POSITION   0x19
#define SEC_TS_CMD_SPONGE_GET_INFO   0x90
#define SEC_TS_CMD_SPONGE_WRITE_PARAM   0x91
#define SEC_TS_CMD_SPONGE_READ_PARAM   0x92
#define SEC_TS_CMD_SPONGE_NOTIFY_PACKET   0x93
#define SEC_TS_CMD_SPONGE_LP_DUMP   0xF0

enum sec_ts_cover_id {
	SEC_TS_FLIP_WALLET = 0,
	SEC_TS_VIEW_COVER,
	SEC_TS_COVER_NOTHING1,
	SEC_TS_VIEW_WIRELESS,
	SEC_TS_COVER_NOTHING2,
	SEC_TS_CHARGER_COVER,
	SEC_TS_VIEW_WALLET,
	SEC_TS_LED_COVER,
	SEC_TS_CLEAR_FLIP_COVER,
	SEC_TS_QWERTY_KEYBOARD_KOR,
	SEC_TS_QWERTY_KEYBOARD_US,
	SEC_TS_NEON_COVER,
	SEC_TS_ALCANTARA_COVER,
	SEC_TS_GAMEPACK_COVER,
	SEC_TS_LED_BACK_COVER,
	SEC_TS_CLEAR_SIDE_VIEW_COVER,
	SEC_TS_MONTBLANC_COVER = 100,
};

enum tsp_hw_parameter {
	TSP_ITO_CHECK		= 1,
	TSP_RAW_CHECK		= 2,
	TSP_MULTI_COUNT		= 3,
	TSP_WET_MODE		= 4,
	TSP_COMM_ERR_COUNT	= 5,
	TSP_MODULE_ID		= 6,
};

#define TEST_MODE_MIN_MAX		false
#define TEST_MODE_ALL_NODE		true
#define TEST_MODE_READ_FRAME		false
#define TEST_MODE_READ_CHANNEL		true

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

enum offset_fac_data_type {
	OFFSET_FAC_DATA_NO			= 0,
	OFFSET_FAC_DATA_CM			= 1,
	OFFSET_FAC_DATA_CM2			= 2,
	OFFSET_FAC_DATA_CM3			= 3,
	OFFSET_FAC_DATA_MISCAL			= 5,
	OFFSET_FAC_DATA_SELF_FAIL	= 7,
};

struct sec_ts_plat_data {
	int max_x;
	int max_y;
	int display_x;
	int display_y;
	unsigned irq_gpio;
	int irq_type;
	int i2c_burstmax;
	int bringup;
	u32	area_indicator;
	u32	area_navigation;
	u32	area_edge;

	const char *firmware_name;
	const char *regulator_dvdd;
	const char *regulator_avdd;

	u8 core_version_of_ic[4];
	u8 core_version_of_bin[4];
	u8 config_version_of_ic[4];
	u8 config_version_of_bin[4];
	u8 img_version_of_ic[4];
	u8 img_version_of_bin[4];

	struct pinctrl *pinctrl;

	int (*power)(void *data, bool on);
	int tsp_icid;
	int tsp_id;

	bool regulator_boot_on;
	bool support_mt_pressure;
	bool support_dex;
	bool support_ear_detect;
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	int ss_touch_num;
#endif
	bool support_fod;
	bool enable_settings_aot;
	bool sync_reportrate_120;
	bool support_open_short_test;
	bool support_mis_calibration_test;
};

#ifdef TCLM_CONCEPT
int sec_tclm_data_read(struct i2c_client *client, int address);
int sec_tclm_data_write(struct i2c_client *client, int address);
int sec_tclm_execute_force_calibration(struct i2c_client *client, int cal_mode);
#endif

#if defined(CONFIG_DISPLAY_SAMSUNG)
int get_lcd_attached(char *mode);
#endif

#if defined(CONFIG_EXYNOS_DPU30)
int get_lcd_info(char *arg);
#endif

#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif
