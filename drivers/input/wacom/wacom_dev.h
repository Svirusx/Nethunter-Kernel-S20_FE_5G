#include <linux/wakelock.h>
#include <linux/input/sec_cmd.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/spu-verify.h>
#include "wacom_reg.h"

#undef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/typec/manager/usb_typec_manager_notifier.h>
#endif

#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif

#define RETRY_COUNT			3

#if defined(CONFIG_SEC_FACTORY)
#define WACOM_SEC_FACTORY	1
#else
#define WACOM_SEC_FACTORY	0
#endif

#if defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
#define WACOM_PRODUCT_SHIP	1
#else
#define WACOM_PRODUCT_SHIP	0
#endif

#define WACOM_I2C_RETRY		3
#define WACOM_CMD_RETRY		2
#define WACOM_INVALID_IRQ_COUNT	2

#define CMD_RESULT_WORD_LEN	20

#define WACOM_I2C_MODE_NORMAL		false
#define WACOM_I2C_MODE_BOOT		true

#define WACOM_BLE_HISTORY_SIZE		(22 * 150)
#define WACOM_BLE_HISTORY1_SIZE		(22)

/* wacom features */
#define USE_OPEN_CLOSE
#define WACOM_USE_SURVEY_MODE /* SURVEY MODE is LPM mode : Only detect garage(pdct) & aop */

#ifdef WACOM_USE_SURVEY_MODE
#define EPEN_RESUME_DELAY		0
#else
#define EPEN_RESUME_DELAY		180
#endif
#define EPEN_OFF_TIME_LIMIT		10000 // usec

/* query data */
#define COM_COORD_NUM			16
#define COM_RESERVED_NUM		0
#define COM_QUERY_NUM			15
#define COM_QUERY_POS			(COM_COORD_NUM + COM_RESERVED_NUM)
#define COM_QUERY_BUFFER		(COM_QUERY_POS + COM_QUERY_NUM)

/* packet ID for wacom data */
enum PACKET_ID {
	COORD_PACKET	= 1,
	AOP_PACKET	= 3,
	NOTI_PACKET	= 13,
	REPLY_PACKET,
	SEPC_PACKET,
};

enum NOTI_SUB_ID {
	ERROR_PACKET		= 0,
	TABLE_SWAP_PACKET,
	EM_NOISE_PACKET,
	TSP_STOP_PACKET,
	OOK_PACKET,
	CMD_PACKET,
	GCF_PACKET		= 7,	/* Garage Charging Finished notification data*/
};

enum REPLY_SUB_ID {
	OPEN_CHECK_PACKET	= 1,
	SWAP_PACKET,
	SENSITIVITY_PACKET,
	TSP_STOP_COND_PACKET,
	GARAGE_CHARGE_PACKET	= 6,
	ELEC_TEST_PACKET	= 101,
};

enum SEPC_SUB_ID {
	QUERY_PACKET		= 0,
	CHECKSUM_PACKET,
};

enum TABLE_SWAP {
	TABLE_SWAP_DEX_STATION = 1,
	TABLE_SWAP_KBD_COVER = 2,
};

enum COVER_STATE {
	NOMAL_MODE = 0,
	BOOKCOVER_MODE,
	KBDCOVER_MODE,
};

#define WACOM_PATH_EXTERNAL_FW		"/sdcard/FIRMWARE/WACOM/wacom.bin"
#define WACOM_PATH_EXTERNAL_FW_SIGNED	"/sdcard/FIRMWARE/WACOM/wacom_signed.bin"
#define WACOM_PATH_SPU_FW_SIGNED	"/spu/WACOM/ffu_wacom.bin"

#define FW_UPDATE_RUNNING		1
#define FW_UPDATE_PASS			2
#define FW_UPDATE_FAIL			-1

enum {
	WACOM_BUILT_IN = 0,
	WACOM_UMS,
	WACOM_NONE,
	WACOM_FFU,
	WACOM_TEST,
};

enum {
	FW_NONE = 0,
	FW_BUILT_IN,
	FW_SPU,
	FW_HEADER,
	FW_IN_SDCARD,
	FW_IN_SDCARD_SIGNED,
#if WACOM_SEC_FACTORY
	FW_FACTORY_GARAGE,
	FW_FACTORY_UNIT,
#endif
};

/* header ver 1 */
struct fw_image {
	u8 hdr_ver;
	u8 hdr_len;
	u16 fw_ver1;
	u16 fw_ver2;
	u16 fw_ver3;
	u32 fw_len;
	u8 checksum[5];
	u8 alignment_dummy[3];
	u8 data[0];
} __attribute__ ((packed));


#define PDCT_NOSIGNAL			true
#define PDCT_DETECT_PEN			false

/*--------------------------------------------------
 * event
 * wac_i2c->function_result
 * 7. ~ 4. reserved || 3. Garage | 2. Power Save | 1. AOP | 0. Pen In/Out |
 *
 * 0. Pen In/Out ( pen_insert )
 * ( 0: IN / 1: OUT )
 *--------------------------------------------------
 */
#define EPEN_EVENT_PEN_OUT		(1 << 0)
#define EPEN_EVENT_GARAGE		(1 << 1)
#define EPEN_EVENT_AOP			(1 << 2)
#define EPEN_EVENT_SURVEY		(EPEN_EVENT_GARAGE | EPEN_EVENT_AOP)

#define EPEN_SURVEY_MODE_NONE		0x0
#define EPEN_SURVEY_MODE_GARAGE_ONLY	EPEN_EVENT_GARAGE
#define EPEN_SURVEY_MODE_GARAGE_AOP	EPEN_EVENT_AOP


/*--------------------------------------------------
 * function setting by user or default
 * wac_i2c->function_set
 * 7~4. reserved | 3. AOT | 2. ScreenOffMemo | 1. AOD | 0. Depend on AOD
 *
 * 3. AOT - aot_enable sysfs
 * 2. ScreenOffMemo - screen_off_memo_enable sysfs
 * 1. AOD - aod_enable sysfs
 * 0. Depend on AOD - 0 : lcd off status, 1 : lcd on status
 *--------------------------------------------------
 */
#define EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS	(0x1<<0)
#define EPEN_SETMODE_AOP_OPTION_AOD		(0x1<<1)
#define EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO	(0x1<<2)
#define EPEN_SETMODE_AOP_OPTION_AOT		(0x1<<3)
#define EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON	(EPEN_SETMODE_AOP_OPTION_AOD | EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS)
#define EPEN_SETMODE_AOP_OPTION_AOD_LCD_OFF	EPEN_SETMODE_AOP_OPTION_AOD
#define EPEN_SETMODE_AOP			(EPEN_SETMODE_AOP_OPTION_AOD | EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO | \
						EPEN_SETMODE_AOP_OPTION_AOT)

enum {
	EPEN_POS_ID_SCREEN_OF_MEMO = 1,
};
/*
 * struct epen_pos - for using to send coordinate in survey mode
 * @id: for extension of function
 *      0 -> not use
 *      1 -> for Screen off Memo
 *      2 -> for oter app or function
 * @x: x coordinate
 * @y: y coordinate
 */
struct epen_pos {
	u8 id;
	int x;
	int y;
};

/*--------------------------------------------------
 * Set S-Pen mode for TSP
 * 1 byte input/output parameter
 * byte[0]: S-pen mode
 * - 0: non block tsp scan
 * - 1: block tsp scan
 * - others: Reserved for future use
 *--------------------------------------------------
 */
#define DISABLE_TSP_SCAN_BLCOK		0x00
#define ENABLE_TSP_SCAN_BLCOK		0x01

enum WAKEUP_ID {
	HOVER_WAKEUP		= 2,
	SINGLETAP_WAKEUP,
	DOUBLETAP_WAKEUP,
};

enum SCAN_MODE {
	GLOBAL_Y_MODE		= 1,
	GLOBAL_X_MODE,
	FULL_MODE,
	LOCAL_MODE,
};

#define WACOM_STOP_CMD				0x00
#define WACOM_START_CMD				0x01
#define WACOM_STOP_AND_START_CMD	0x02

#define EPEN_CHARGER_OFF_TIME	90	/* default 60s + buffer time */
enum epen_scan_info{
	EPEN_CHARGE_OFF			= 0,
	EPEN_CHARGE_ON			= 1,
	EPEN_CHARGE_HAPPENED	= 2,
};

typedef enum {
	WACOM_ENABLE = 0,
	WACOM_DISABLE = 1,
} wacom_disable_mode_t;

/* WACOM_DEBUG : Print event contents */
#define WACOM_DEBUG_PRINT_I2C_READ_CMD		0x01
#define WACOM_DEBUG_PRINT_I2C_WRITE_CMD		0x02
#define WACOM_DEBUG_PRINT_COORDEVENT		0x04
#define WACOM_DEBUG_PRINT_ALLEVENT		0x08

/* digitizer & garage open test */
#define EPEN_OPEN_TEST_PASS			0
#define EPEN_OPEN_TEST_NOTSUPPORT	-1
#define EPEN_OPEN_TEST_EIO			-2
#define EPEN_OPEN_TEST_FAIL			-3
#define EPEN_OPEN_TEST_EMODECHG		-4
enum {
	WACOM_GARAGE_TEST = 0,
	WACOM_DIGITIZER_TEST,
};

/* elec data */
#define COM_ELEC_NUM			38
#define COM_ELEC_DATA_NUM		12
#define COM_ELEC_DATA_POS		14

#define POWER_OFFSET			1000000000000

enum {
	SEC_NORMAL = 0,
	SEC_SHORT,
	SEC_OPEN,
};

enum epen_elec_spec_mode {
	EPEN_ELEC_DATA_MAIN	= 0,
	EPEN_ELEC_DATA_UNIT	= 1,	// pretest
//	EPEN_ELEC_DATA_ASSY	= 2,	// reserved
};

struct wacom_elec_data {
	u32 spec_ver[2];
	u8 max_x_ch;
	u8 max_y_ch;
	u8 shift_value;

	u16 *elec_data;

	u16 *xx;
	u16 *xy;
	u16 *yx;
	u16 *yy;

	long long cal_xx;
	long long cal_xy;
	long long cal_yx;
	long long cal_yy;

	long long *xx_xx;
	long long *xy_xy;
	long long *yx_yx;
	long long *yy_yy;

	long long *rxx;
	long long *rxy;
	long long *ryx;
	long long *ryy;

	long long *drxx;
	long long *drxy;
	long long *dryx;
	long long *dryy;

	long long *xx_ref;
	long long *xy_ref;
	long long *yx_ref;
	long long *yy_ref;

	long long *xx_spec;
	long long *xy_spec;
	long long *yx_spec;
	long long *yy_spec;

	long long *rxx_ref;
	long long *rxy_ref;
	long long *ryx_ref;
	long long *ryy_ref;

	long long *drxx_spec;
	long long *drxy_spec;
	long long *dryx_spec;
	long long *dryy_spec;
};

struct wacom_g5_platform_data {
	struct wacom_elec_data *edata;		/* currnet test spec */
	struct wacom_elec_data *edatas[3];	/* 0:main, 1:sub unit, 2:sub assy(reserved) */

	int irq_gpio;
	int pdct_gpio;
	int fwe_gpio;
	int boot_addr;
	const char *regulator_avdd;

	int x_invert;
	int y_invert;
	int xy_switch;
	bool use_dt_coord;
	u32 origin[2];
	int max_x;
	int max_y;
	int max_pressure;
	int max_x_tilt;
	int max_y_tilt;
	int max_height;
	const char *fw_path;
#if WACOM_SEC_FACTORY
	const char *fw_fac_path;
#endif
	u8 ic_type;
	u8 bl_ver;
	u32 module_ver;
	u32 support_garage_open_test;
	bool use_garage;
	u32 table_swap;
	bool regulator_boot_on;
	u32 bringup;
	bool support_cover_noti;

	u32	area_indicator;
	u32	area_navigation;
	u32	area_edge;
};



struct wacom_i2c {
	struct wacom_g5_platform_data *pdata;
	struct i2c_client *client;
	struct i2c_client *client_boot;
	struct input_dev *input_dev;
	struct sec_cmd_data sec;
	struct mutex i2c_mutex;
	struct mutex lock;
	struct mutex update_lock;
	struct mutex irq_lock;
	struct mutex ble_lock;
	struct mutex ble_charge_mode_lock;
	bool power_enable;
	struct wake_lock wakelock;
	bool pm_suspend;
	volatile bool probe_done;
	bool query_status;
	struct completion resume_done;
	struct completion i2c_done;
	struct regulator *regulator_avdd;
	struct regulator *regulator_dvdd;

	int irq;
	int irq_pdct;
	int pen_pdct;
	bool pdct_lock_fail;
	struct delayed_work pen_insert_dwork;

	/* survey & garage */
	struct mutex mode_lock;
	u8 function_set;
	u8 function_result;
	volatile bool screen_on;
	u8 survey_mode;
	u8 check_survey_mode;
	int samplerate_state;
	volatile u8 report_scan_seq;	/* 1:normal, 2:garage+aod, 3:garage only, 4:sync aop*/

	/* ble(ook) */
	volatile u8 ble_mode;
	long chg_time_stamp;
	volatile bool ble_mode_change;
	volatile bool ble_block_flag;
	bool ble_charging_state;
	bool ble_disable_flag;

	/* for tui or factory test */
	bool epen_blocked;

	/* coord */
	u16 hi_x;
	u16 hi_y;
	u16 p_x;
	u16 p_y;
	u16 x;
	u16 y;
	s8 tilt_x;
	s8 tilt_y;
	u16 pressure;
	u16 height;	
	int tool;
	u32 mcount;
	int pen_prox;
	int pen_pressed;
	int side_pressed;

	struct epen_pos survey_pos;

	/* fw update */
	struct wake_lock fw_wakelock;
	struct fw_image *fw_img;
	const struct firmware *firm_data;

	u16 fw_ver_ic;
	u16 fw_ver_bin;
	u16 fw_ver_spu;
	int update_status;
	u8 *fw_data;
	u8 fw_update_way;

	bool checksum_result;
	char fw_chksum[5];
	bool do_crc_check;

	/* tsp block */
	bool is_tsp_block;
	int tsp_scan_mode;
	int tsp_block_cnt;

	/* support additional mode */
	bool battery_saving_mode;
	int wcharging_mode;

	struct delayed_work nb_reg_work;

	struct notifier_block kbd_nb;
	struct work_struct kbd_work;
	u8 kbd_conn_state;
	u8 kbd_cur_conn_state;

	struct notifier_block typec_nb;
	struct work_struct typec_work;
	u8 dp_connect_state;
	u8 dp_connect_cmd;

	struct notifier_block nb;

	/* open test*/
	volatile bool is_open_test;
	// digitizer
	bool connection_check;
	int  fail_channel;
	int  min_adc_val;
	int  error_cal;
	int  min_cal_val;
	// garage
	bool garage_connection_check;
	int  garage_fail_channel;
	int  garage_min_adc_val;
	int  garage_error_cal;
	int  garage_min_cal_val;

	/* check abnormal case*/
	struct delayed_work work_print_info;
	u32	print_info_cnt_open;
	u32	scan_info_fail_cnt;
	u32 check_elec;
	u32 i2c_fail_count;
	u32 abnormal_reset_count;
	u32 pen_out_count;
	volatile bool reset_flag;
	u8 debug_flag;
#if WACOM_SEC_FACTORY
	volatile bool fac_garage_mode;
	u32 garage_gain0;
	u32 garage_gain1;
	u32 garage_freq0;
	u32 garage_freq1;
#endif
	char *ble_hist;
	char *ble_hist1;
	int ble_hist_index;
	char cover;
};

extern struct wacom_i2c *g_wac_i2c;
#ifdef CONFIG_DISPLAY_SAMSUNG
extern int get_lcd_attached(char *mode);
#endif
#ifdef CONFIG_EXYNOS_DPU30
extern int get_lcd_info(char *arg);
#endif

int wacom_power(struct wacom_i2c *, bool on);
void wacom_reset_hw(struct wacom_i2c *);

void wacom_compulsory_flash_mode(struct wacom_i2c *, bool enable);
int wacom_get_irq_state(struct wacom_i2c *);

void wacom_wakeup_sequence(struct wacom_i2c *);
void wacom_sleep_sequence(struct wacom_i2c *);

int wacom_i2c_load_fw(struct wacom_i2c *wac_i2c, u8 fw_path);
void wacom_i2c_unload_fw(struct wacom_i2c *wac_i2c);
int wacom_fw_update_on_hidden_menu(struct wacom_i2c *, u8 fw_update_way);
int wacom_i2c_flash(struct wacom_i2c *);

void wacom_enable_irq(struct wacom_i2c *, bool enable);

int wacom_i2c_send(struct wacom_i2c *, const char *buf, int count);
int wacom_i2c_send_boot(struct wacom_i2c *, const char *buf, int count);
int wacom_i2c_recv(struct wacom_i2c *, char *buf, int count);
int wacom_i2c_recv_boot(struct wacom_i2c *, char *buf, int count);

int wacom_i2c_query(struct wacom_i2c *);
int wacom_checksum(struct wacom_i2c *);
int wacom_i2c_set_sense_mode(struct wacom_i2c *);

void wacom_forced_release(struct wacom_i2c *);
void forced_release_fullscan(struct wacom_i2c *wac_i2c);

void wacom_select_survey_mode(struct wacom_i2c *, bool enable);
int wacom_i2c_set_survey_mode(struct wacom_i2c *, int mode);
int wacom_start_stop_cmd(struct wacom_i2c *wac_i2c, int mode);

int wacom_open_test(struct wacom_i2c *wac_i2c, int test_mode);

int wacom_sec_init(struct wacom_i2c *);
void wacom_sec_remove(struct wacom_i2c *);

void wacom_print_info(struct wacom_i2c *wac_i2c);
void wacom_i2c_coord_modify(struct wacom_i2c *wac_i2c);
void wacom_disable_mode(struct wacom_i2c *wac_i2c, wacom_disable_mode_t mode);

int wacom_check_ub(struct i2c_client *client);

void wacom_swap_compensation(struct wacom_i2c *wac_i2c, char cmd);
int wacom_ble_charge_mode(struct wacom_i2c *wac_i2c, int mode);
