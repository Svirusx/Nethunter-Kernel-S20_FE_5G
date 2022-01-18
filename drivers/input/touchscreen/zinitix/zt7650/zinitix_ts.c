/*
 *
 * Zinitix zt touchscreen driver
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 */


#undef TSP_VERBOSE_DEBUG

#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>
#include <linux/input/mt.h>
#include <linux/input/sec_cmd.h>
#include <linux/input/sec_tclm_v2.h>
#include <linux/of_gpio.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif
#ifdef CONFIG_VBUS_NOTIFIER
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/vbus_notifier.h>
#include <linux/power_supply.h>
#endif

#include "zinitix_ts.h"

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
#include <linux/completion.h>
#include <linux/atomic.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#endif

#define CONFIG_INPUT_ENABLED

#define MAX_FW_PATH 255

#define SPU_FW_SIGNED
#ifdef SPU_FW_SIGNED
#include <linux/spu-verify.h>
#endif

#define TSP_PATH_EXTERNAL_FW		"/sdcard/Firmware/TSP/tsp.bin"
#define TSP_PATH_EXTERNAL_FW_SIGNED	"/sdcard/Firmware/TSP/tsp_signed.bin"
#define TSP_PATH_SPU_FW_SIGNED		"/spu/TSP/ffu_tsp.bin"

#define TSP_TYPE_BUILTIN_FW			0
#define TSP_TYPE_EXTERNAL_FW			1
#define TSP_TYPE_EXTERNAL_FW_SIGNED		2
#define TSP_TYPE_SPU_FW_SIGNED			3

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
#include <linux/t-base-tui.h>
#endif

#ifdef CONFIG_SAMSUNG_TUI
#include "stui_inf.h"
#endif

#define ZINITIX_DEBUG					0
#define PDIFF_DEBUG					1
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
#define USE_MISC_DEVICE
#endif

/* added header file */

#define TOUCH_POINT_MODE			0
#define MAX_SUPPORTED_FINGER_NUM	10 /* max 10 */

/* if you use isp mode, you must add i2c device :
   name = "zinitix_isp" , addr 0x50*/

/* resolution offset */
#define ABS_PT_OFFSET			(-1)

#define USE_CHECKSUM			1
#define CHECK_HWID			0

#define CHIP_OFF_DELAY			300 /*ms*/
#define CHIP_ON_DELAY			100 /*ms*/
#define FIRMWARE_ON_DELAY		150 /*ms*/

#define DELAY_FOR_SIGNAL_DELAY		30 /*us*/
#define DELAY_FOR_TRANSCATION		50
#define DELAY_FOR_POST_TRANSCATION	10

#define CMD_RESULT_WORD_LEN	10

enum power_control {
	POWER_OFF,
	POWER_ON,
	POWER_ON_SEQUENCE,
};

/* ESD Protection */
/*second : if 0, no use. if you have to use, 3 is recommended*/
#define ESD_TIMER_INTERVAL			2
#define SCAN_RATE_HZ				1000
#define CHECK_ESD_TIMER				5

/*Test Mode (Monitoring Raw Data) */
#define TSP_INIT_TEST_RATIO  100

#define	SEC_MUTUAL_AMP_V_SEL	0x0232

#define	SEC_DND_N_COUNT			11
#define	SEC_DND_U_COUNT			16
#define	SEC_DND_FREQUENCY		139

#define	SEC_HFDND_N_COUNT		11
#define	SEC_HFDND_U_COUNT		16
#define	SEC_HFDND_FREQUENCY		104

#define	SEC_SX_AMP_V_SEL		0x0434
#define	SEC_SX_SUB_V_SEL		0x0055
#define	SEC_SY_AMP_V_SEL		0x0232
#define	SEC_SY_SUB_V_SEL		0x0022
#define	SEC_SHORT_N_COUNT		2
#define	SEC_SHORT_U_COUNT		1

#define SEC_SY_SAT_FREQUENCY	200
#define SEC_SY_SAT_N_COUNT		9
#define SEC_SY_SAT_U_COUNT		9
#define SEC_SY_SAT_RS0_TIME		0x00FF
#define SEC_SY_SAT_RBG_SEL		0x0404
#define SEC_SY_SAT_AMP_V_SEL	0x0434
#define SEC_SY_SAT_SUB_V_SEL	0x0044

#define SEC_SY_SAT2_FREQUENCY	200
#define SEC_SY_SAT2_N_COUNT		9
#define SEC_SY_SAT2_U_COUNT		3
#define SEC_SY_SAT2_RS0_TIME	0x00FF
#define SEC_SY_SAT2_RBG_SEL		0x0404
#define SEC_SY_SAT2_AMP_V_SEL	0x0434
#define SEC_SY_SAT2_SUB_V_SEL	0x0011

#define MAX_RAW_DATA_SZ				792 /* 36x22 */  /* need to read from ic */
#define MAX_TRAW_DATA_SZ	\
	(MAX_RAW_DATA_SZ + 4 * MAX_SUPPORTED_FINGER_NUM + 2)

#define RAWDATA_DELAY_FOR_HOST		20000

struct raw_ioctl {
	u32 sz;
	u32 buf;
};

struct reg_ioctl {
	u32 addr;
	u32 val;
};

#define TOUCH_SEC_MODE				48
#define TOUCH_REF_MODE				10
#define TOUCH_NORMAL_MODE			5
#define TOUCH_DELTA_MODE			3
#define TOUCH_RAW_MODE				7
#define TOUCH_REFERENCE_MODE			8
#define TOUCH_DND_MODE				11
#define TOUCH_HFDND_MODE			12
#define TOUCH_TXSHORT_MODE			13
#define TOUCH_RXSHORT_MODE			14
#define TOUCH_CHANNEL_TEST_MODE			14
#define TOUCH_JITTER_MODE			15
#define TOUCH_SELF_DND_MODE			17
#define TOUCH_SENTIVITY_MEASUREMENT_MODE	21
#define TOUCH_CHARGE_PUMP_MODE			25
#define TOUCH_REF_ABNORMAL_TEST_MODE		33
#define DEF_RAW_SELF_SSR_DATA_MODE		39	/* SELF SATURATION RX */
#define TOUCH_AGING_MODE		40
#define TOUCH_AMP_CHECK_MODE	50

#define TOUCH_SENTIVITY_MEASUREMENT_COUNT	9

/*  Other Things */
#define INIT_RETRY_CNT				3
#define I2C_SUCCESS					0
#define I2C_FAIL					1

/*---------------------------------------------------------------------*/

/* chip code */
#define BT43X_CHIP_CODE		0xE200
#define BT53X_CHIP_CODE		0xF400
#define ZT7548_CHIP_CODE	0xE548
#define ZT7538_CHIP_CODE	0xE538
#define ZT7532_CHIP_CODE	0xE532
#define ZT7554_CHIP_CODE	0xE700
#define ZT7650_CHIP_CODE	0xE650

/////////////////////////////////////////////////////
//[Judge download type]
/////////////////////////////////////////////////////
#define NEED_FULL_DL			0
#define NEED_PARTIAL_DL_CUSTOM		1
#define NEED_PARTIAL_DL_REG		2

/////////////////////////////////////////////////////
//[VCMD]
/////////////////////////////////////////////////////

#define VCMD_UPGRADE_PART_ERASE_START			0x01DA

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7650)
#define VCMD_UPGRADE_INIT_FLASH				0x20F0
#define VCMD_UPGRADE_WRITE_FLASH			0x21F0
#define VCMD_UPGRADE_READ_FLASH				0x22F0
#define VCMD_UPGRADE_MODE				0x25F0
#define VCMD_UPGRADE_WRITE_MODE				0x27F0
#define VCMD_UPGRADE_MASS_ERASE				0x28F0
#define VCMD_UPGRADE_START_PAGE				0x29F0
#define VCMD_UPGRADE_BLOCK_ERASE			0x2AF0

#define VCMD_NVM_PROG_START				0x11F0
#define VCMD_NVM_INIT					0x12F0
#define VCMD_NVM_WRITE_ENABLE				0x13F0
#define VCMD_INTN_CLR					0x14F0
#define VCMD_OSC_SEL					0x1AF0

#define VCMD_ID						0x17F0
#define VCMD_ENABLE					0x10F0
#define VCMD_NVM_ERASE					0x2AF0
#define VCMD_NVM_WRITE					0x34F0
#define VCMD_REG_READ					0x18F0
#define VCMD_REG_WRITE					0x19F0
#else
#define VCMD_UPGRADE_INIT_FLASH				0x01D0
#define VCMD_UPGRADE_WRITE_FLASH			0x01D1
#define VCMD_UPGRADE_READ_FLASH				0x01D2
#define VCMD_UPGRADE_MODE				0x01D5
#define VCMD_UPGRADE_WRITE_MODE				0x01DE
#define VCMD_UPGRADE_MASS_ERASE				0x01DF

#define VCMD_NVM_PROG_START				0xC001
#define VCMD_NVM_INIT					0xC002
#define VCMD_NVM_WRITE_ENABLE				0xC003
#define VCMD_INTN_CLR					0xC004
#define VCMD_OSC_SEL					0xC201

#define VCMD_ID						0xCC00
#define VCMD_ENABLE					0xC000
#define VCMD_NVM_ERASE					0xC10D
#define VCMD_NVM_WRITE					0xC10B

#define VCMD_REG_READ					0xCC01
#define VCMD_REG_WRITE					0xCC02
#endif

/* Register Map*/
#define ZT_SWRESET_CMD					0x0000
#define ZT_WAKEUP_CMD					0x0001
#define ZT_IDLE_CMD					0x0004
#define ZT_SLEEP_CMD					0x0005
#define ZT_CLEAR_INT_STATUS_CMD				0x0003
#define ZT_CALIBRATE_CMD				0x0006
#define ZT_SAVE_STATUS_CMD				0x0007
#define ZT_SAVE_CALIBRATION_CMD				0x0008
#define ZT_RECALL_FACTORY_CMD				0x000f
#define ZT_THRESHOLD					0x0020
#define ZT_DEBUG_REG					0x0115
#define ZT_TOUCH_MODE					0x0010
#define ZT_CHIP_REVISION				0x0011
#define ZT_FIRMWARE_VERSION				0x0012
#define ZT_MINOR_FW_VERSION				0x0121
#define ZT_VENDOR_ID					0x001C
#define ZT_HW_ID					0x0014
#define ZT_DATA_VERSION_REG				0x0013
#define ZT_SUPPORTED_FINGER_NUM				0x0015
#define ZT_EEPROM_INFO					0x0018
#define ZT_INITIAL_TOUCH_MODE				0x0019
#define ZT_TOTAL_NUMBER_OF_X				0x0061
#define ZT_TOTAL_NUMBER_OF_Y				0x0060
#define ZT_CONNECTION_CHECK_REG				0x0062
#define ZT_POWER_STATE_FLAG				0x007E
#define ZT_DELAY_RAW_FOR_HOST				0x007f
#define ZT_BUTTON_SUPPORTED_NUM				0x00B0
#define ZT_BUTTON_SENSITIVITY				0x00B2
#define ZT_DUMMY_BUTTON_SENSITIVITY			0X00C8
#define ZT_X_RESOLUTION					0x00C0
#define ZT_Y_RESOLUTION					0x00C1
#define ZT_CALL_AOT_REG				0x00D3
#define ZT_STATUS_REG				0x0080
#define ZT_POINT_STATUS_REG				0x0200
#define ZT_POINT_STATUS_REG1				0x0201
#define ZT_FOD_STATUS_REG				0x020A
#define ZT_VI_STATUS_REG				0x020B

#define ZT_OSC_TIMER_LSB			0x019F
#define ZT_OSC_TIMER_MSB			0x01A0

#define ZT_ICON_STATUS_REG				0x00AA
#define ZT_JITTER_RESULT				0x01A3

#define ZT_SET_AOD_X_REG				0x00AB
#define ZT_SET_AOD_Y_REG				0x00AC
#define ZT_SET_AOD_W_REG				0x00AD
#define ZT_SET_AOD_H_REG				0x00AE
#define ZT_LPM_MODE_REG					0x00AF

#define ZT_GET_AOD_X_REG				0x0191
#define ZT_GET_AOD_Y_REG				0x0192

#define ZT_GET_FOD_WITH_FINGER_PACKET			0x019A
#define ZT_SET_SIP_MODE					0x019D

#define ZT_DND_SHIFT_VALUE				0x012B
#define ZT_AFE_FREQUENCY					0x0100
#define ZT_DND_N_COUNT					0x0122
#define ZT_DND_U_COUNT					0x0135

#define ZT_RAWDATA_REG					0x0200

#define ZT_INT_ENABLE_FLAG				0x00f0
#define ZT_PERIODICAL_INTERRUPT_INTERVAL	0x00f1
#define ZT_BTN_WIDTH						0x0316
#define ZT_REAL_WIDTH					0x03A6

#define ZT_CHECKSUM_RESULT				0x012c


#define ZINITIX_INTERNAL_FLAG_03		0x011f

#define ZT_OPTIONAL_SETTING				0x0116

#define ZT_SET_WIRELESSCHARGER_MODE		0x0199
#define ZT_SET_NOTE_MODE			0x019B
#define ZT_SET_GAME_MODE			0x019C

#define ZT_SET_SCANRATE			0x01A0
#define ZT_SET_SCANRATE_ENABLE			0x01A1
#define ZT_VSYNC_TEST_RESULT 			0x01A2

#define ZT_COVER_CONTROL_REG			0x023E

#define ZT_REJECT_ZONE_AREA			0x01AD

#define	ZT_EDGE_LANDSCAPE_MODE					0x0038
#define	ZT_EDGE_REJECT_PORT_SIDE_UP_DOWN_DIV	0x0039
#define	ZT_EDGE_REJECT_PORT_SIDE_UP_WIDTH		0x003A
#define	ZT_EDGE_REJECT_PORT_SIDE_DOWN_WIDTH		0x003E
#define	ZT_EDGE_REJECT_PORT_EDGE_EXCEPT_SEL		0x003F
#define	ZT_EDGE_REJECT_PORT_EDGE_EXCEPT_START	0x0040
#define	ZT_EDGE_REJECT_PORT_EDGE_EXCEPT_END		0x0041
//#define	ZT_EDGE_GRIP_PORT_TOP_BOT_WIDTH			0x0042
#define	ZT_EDGE_GRIP_PORT_SIDE_WIDTH			0x0045
#define	ZT_EDGE_REJECT_LAND_SIDE_WIDTH			0x0046
#define	ZT_EDGE_REJECT_LAND_TOP_BOT_WIDTH		0x0047
#define	ZT_EDGE_GRIP_LAND_SIDE_WIDTH			0x0048
#define	ZT_EDGE_GRIP_LAND_TOP_BOT_WIDTH			0x0049

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

#define ZT_RESOLUTION_EXPANDER			0x0186
#define ZT_MUTUAL_AMP_V_SEL			0x02F9
#define ZT_SX_AMP_V_SEL				0x02DF
#define ZT_SX_SUB_V_SEL				0x02E0
#define ZT_SY_AMP_V_SEL				0x02EC
#define ZT_SY_SUB_V_SEL				0x02ED
#define ZT_CHECKSUM					0x03DF
#define ZT_JITTER_SAMPLING_CNT			0x001F

#define ZT_SY_SAT_FREQUENCY			0x03E0
#define ZT_SY_SAT_N_COUNT			0x03E1
#define ZT_SY_SAT_U_COUNT			0x03E2
#define ZT_SY_SAT_RS0_TIME			0x03E3
#define ZT_SY_SAT_RBG_SEL			0x03E4
#define ZT_SY_SAT_AMP_V_SEL			0x03E5
#define ZT_SY_SAT_SUB_V_SEL			0x03E6

#define ZT_SY_SAT2_FREQUENCY		0x03E7
#define ZT_SY_SAT2_N_COUNT			0x03E8
#define ZT_SY_SAT2_U_COUNT			0x03E9
#define ZT_SY_SAT2_RS0_TIME			0x03EA
#define ZT_SY_SAT2_RBG_SEL			0x03EB
#define ZT_SY_SAT2_AMP_V_SEL		0x03EC
#define ZT_SY_SAT2_SUB_V_SEL		0x03ED

#define ZT_PROXIMITY_XDATA			0x030E
#define ZT_PROXIMITY_YDATA			0x030F
#define ZT_PROXIMITY_DETECT			0x0024
#define ZT_PROXIMITY_THRESHOLD		0x023F
#define ABS_MT_CUSTOM2		0x3f	/* custom event only for sensor */


#define ZT_POCKET_DETECT			0x0037

#define CORRECT_CHECK_SUM			0x55AA

#define REG_FOD_AREA_STR_X				0x013B
#define REG_FOD_AREA_STR_Y				0x013C
#define REG_FOD_AREA_END_X				0x013E
#define REG_FOD_AREA_END_Y				0x013F

#define REG_FOD_MODE_SET				0x0142

/* Interrupt & status register flag bit */
#define BIT_PT_CNT_CHANGE	0
#define BIT_DOWN		1
#define BIT_MOVE		2
#define BIT_UP			3
#define BIT_PALM		4
#define BIT_PALM_REJECT		5
#define BIT_GESTURE		6
#define RESERVED_1		7
#define BIT_WEIGHT_CHANGE	8
#define BIT_POCKET_MODE		8
#define BIT_PT_NO_CHANGE	9
#define BIT_REJECT		10
#define BIT_PT_EXIST		11
#define BIT_PROXIMITY		12
#define BIT_MUST_ZERO		13
#define BIT_DEBUG		14
#define BIT_ICON_EVENT		15

/* button */
#define BIT_O_ICON0_DOWN	0
#define BIT_O_ICON1_DOWN	1
#define BIT_O_ICON2_DOWN	2
#define BIT_O_ICON3_DOWN	3
#define BIT_O_ICON4_DOWN	4
#define BIT_O_ICON5_DOWN	5
#define BIT_O_ICON6_DOWN	6
#define BIT_O_ICON7_DOWN	7

#define BIT_O_ICON0_UP		8
#define BIT_O_ICON1_UP		9
#define BIT_O_ICON2_UP		10
#define BIT_O_ICON3_UP		11
#define BIT_O_ICON4_UP		12
#define BIT_O_ICON5_UP		13
#define BIT_O_ICON6_UP		14
#define BIT_O_ICON7_UP		15


#define SUB_BIT_EXIST		0
#define SUB_BIT_DOWN		1
#define SUB_BIT_MOVE		2
#define SUB_BIT_UP		3
#define SUB_BIT_UPDATE		4
#define SUB_BIT_WAIT		5

/* zt_DEBUG_REG */
#define DEF_DEVICE_STATUS_NPM			0
#define DEF_DEVICE_STATUS_WALLET_COVER_MODE	1
#define DEF_DEVICE_STATUS_NOISE_MODE		2
#define DEF_DEVICE_STATUS_WATER_MODE		3
#define DEF_DEVICE_STATUS_LPM__MODE		4
#define BIT_GLOVE_TOUCH				5
#define DEF_DEVICE_STATUS_PALM_DETECT		10
#define DEF_DEVICE_STATUS_SVIEW_MODE		11

/* zt_zt_COVER_CONTROL_REG */
#define WALLET_COVER_CLOSE	0x0000
#define VIEW_COVER_CLOSE	0x0100
#define COVER_OPEN			0x0200
#define LED_COVER_CLOSE		0x0700
#define CLEAR_COVER_CLOSE	0x0800
#define CLEAR_SIDE_VIEW_COVER_CLOSE	0x0F00
#define MINI_SVIEW_WALLET_COVER_CLOSE	0x1000

enum zt_cover_id {
	ZT_FLIP_WALLET = 0,
	ZT_VIEW_COVER,
	ZT_COVER_NOTHING1,
	ZT_VIEW_WIRELESS,
	ZT_COVER_NOTHING2,
	ZT_CHARGER_COVER,
	ZT_VIEW_WALLET,
	ZT_LED_COVER,
	ZT_CLEAR_FLIP_COVER,
	ZT_QWERTY_KEYBOARD_EUR,
	ZT_QWERTY_KEYBOARD_KOR,
	ZT_NEON_COVER,
	ZT_CLEAR_SIDE_VIEW_COVER = 15,
	ZT_MINI_SVIEW_WALLET_COVER = 16,
	ZT_MONTBLANC_COVER = 100,
};

#define zinitix_bit_set(val, n)		((val) &= ~(1<<(n)), (val) |= (1<<(n)))
#define zinitix_bit_clr(val, n)		((val) &= ~(1<<(n)))
#define zinitix_bit_test(val, n)	((val) & (1<<(n)))
#define zinitix_swap_v(a, b, t)		((t) = (a), (a) = (b), (b) = (t))
#define zinitix_swap_16(s)			(((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)))

/* REG_USB_STATUS : optional setting from AP */
#define DEF_OPTIONAL_MODE_USB_DETECT_BIT		0
#define	DEF_OPTIONAL_MODE_SVIEW_DETECT_BIT		1
#define DEF_OPTIONAL_MODE_SENSITIVE_BIT		2
#define DEF_OPTIONAL_MODE_EDGE_SELECT			3
#define	DEF_OPTIONAL_MODE_DUO_TOUCH		4
#define DEF_OPTIONAL_MODE_TOUCHABLE_AREA		5
#define DEF_OPTIONAL_MODE_EAR_DETECT		6
#define DEF_OPTIONAL_MODE_EAR_DETECT_MUTUAL		7
#define DEF_OPTIONAL_MODE_POCKET_MODE		8
#define DEF_OPTIONAL_MODE_OTG_MODE		15

/* end header file */

#define DEF_MIS_CAL_SPEC_MIN 40
#define DEF_MIS_CAL_SPEC_MAX 160
#define DEF_MIS_CAL_SPEC_MID 100
#define ZT_MIS_CAL_SET		0x00D0
#define ZT_MIS_CAL_RESULT	0x00D1

#define TOUCH_PRINT_INFO_DWORK_TIME 30000 /* 30 secs */

typedef enum {
	SPONGE_EVENT_TYPE_SPAY			= 0x04,
	SPONGE_EVENT_TYPE_SINGLE_TAP		= 0x08,
	SPONGE_EVENT_TYPE_AOD_PRESS		= 0x09,
	SPONGE_EVENT_TYPE_AOD_LONGPRESS		= 0x0A,
	SPONGE_EVENT_TYPE_AOD_DOUBLETAB		= 0x0B,
	SPONGE_EVENT_TYPE_FOD_PRESS			= 0x0F,
	SPONGE_EVENT_TYPE_FOD_RELEASE		= 0x10,
	SPONGE_EVENT_TYPE_FOD_OUT			= 0x11,
} SPONGE_EVENT_TYPE;

/* Touch Screen */
#define TSP_CMD_STR_LEN			32
#define TSP_CMD_RESULT_STR_LEN		3264	//34*16*6
#define TSP_CMD_PARAM_NUM		8
#define TSP_CMD_X_NUM			34
#define TSP_CMD_Y_NUM			16
#define TSP_CMD_NODE_NUM		(TSP_CMD_Y_NUM * TSP_CMD_X_NUM)
#define tostring(x) #x

struct tsp_raw_data {
	s16 cnd_data[TSP_CMD_NODE_NUM];
	s16 dnd_data[TSP_CMD_NODE_NUM];
	s16 hfdnd_data[TSP_CMD_NODE_NUM];
	s16 delta_data[TSP_CMD_NODE_NUM];
	s16 vgap_data[TSP_CMD_NODE_NUM];
	s16 hgap_data[TSP_CMD_NODE_NUM];
	s16 rxshort_data[TSP_CMD_NODE_NUM];
	s16 txshort_data[TSP_CMD_NODE_NUM];
	s16 selfdnd_data[TSP_CMD_NODE_NUM];
	u16 ssr_data[TSP_CMD_NODE_NUM];
	s16 self_sat_dnd_data[TSP_CMD_NODE_NUM];
	s16 self_vgap_data[TSP_CMD_NODE_NUM];
	s16 self_hgap_data[TSP_CMD_NODE_NUM];
	s16 jitter_data[TSP_CMD_NODE_NUM];
	s16 amp_check_data[TSP_CMD_NODE_NUM];
	s16 reference_data[TSP_CMD_NODE_NUM];
	s16 reference_data_abnormal[TSP_CMD_NODE_NUM];
	s16 charge_pump_data[TSP_CMD_NODE_NUM];
	u16 channel_test_data[5];
};

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
 *		 unsigned char data[1];
 *	 };
 *};
 * ----------------------------------------
 */
struct ts_test_result {
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
#define TEST_OCTA_MODULE	1
#define TEST_OCTA_ASSAY		2

#define TEST_OCTA_NONE		0
#define TEST_OCTA_FAIL		1
#define TEST_OCTA_PASS		2

#define TSP_NORMAL_EVENT_MSG	1
static int m_ts_debug_mode = ZINITIX_DEBUG;
struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *tsp_cb, bool mode);
};

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
#define SECURE_TOUCH_ENABLED	1
#define SECURE_TOUCH_DISABLED	0
static bool old_ta_status;
#endif

static bool g_ta_connected =0;
typedef union {
	u16 optional_mode;
	struct select_mode {
		u16 flag;
	} select_mode;
} zt_setting;

#if ESD_TIMER_INTERVAL
static struct workqueue_struct *esd_tmr_workqueue;
#endif

#define TOUCH_V_FLIP	0x01
#define TOUCH_H_FLIP	0x02
#define TOUCH_XY_SWAP	0x04

#define RETRY_CNT	3
struct capa_info {
	u16 vendor_id;
	u16 ic_revision;
	u16 fw_version;
	u16 fw_minor_version;
	u16 reg_data_version;
	u16 threshold;
	u16 key_threshold;
	u16 dummy_threshold;
	u32 ic_fw_size;
	u32 MaxX;
	u32 MaxY;
	u16 multi_fingers;
	u16 button_num;
	u16 ic_int_mask;
	u16 x_node_num;
	u16 y_node_num;
	u16 total_node_num;
	u16 hw_id;
	u16 afe_frequency;
	u16 shift_value;
	u16 mutual_amp_v_sel;
	u16 n_cnt;
	u16 u_cnt;
	u16 is_zmt200;
	u16 sx_amp_v_sel;
	u16 sx_sub_v_sel;
	u16 sy_amp_v_sel;
	u16 sy_sub_v_sel;
	u16 current_touch_mode;
	u8 gesture_support;
};

enum work_state {
	NOTHING = 0,
	NORMAL,
	ESD_TIMER,
	EALRY_SUSPEND,
	SUSPEND,
	RESUME,
	LATE_RESUME,
	UPGRADE,
	REMOVE,
	SET_MODE,
	HW_CALIBRAION,
	RAW_DATA,
	PROBE,
	SLEEP_MODE_IN,
	SLEEP_MODE_OUT,
};

enum {
	BUILT_IN = 0,
	UMS,
	REQ_FW,
	SPU,
};

struct zt_ts_info {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *input_dev_pad;
	struct input_dev *input_dev_proximity;
	struct zt_ts_platform_data *pdata;
	char phys[32];
	struct capa_info cap_info;
	struct point_info touch_info[MAX_SUPPORTED_FINGER_NUM];
	struct point_info touch_fod_info;
	u8 fod_touch_vi_data[SEC_CMD_STR_LEN];
	struct ts_coordinate cur_coord[MAX_SUPPORTED_FINGER_NUM];
	struct ts_coordinate old_coord[MAX_SUPPORTED_FINGER_NUM];
	unsigned char *fw_data;
	u16 icon_event_reg;
	u16 prev_icon_event;
	int irq;
	u8 work_state;
	u8 finger_cnt1;
	unsigned int move_count[MAX_SUPPORTED_FINGER_NUM];
	struct mutex set_reg_lock;
	struct mutex set_lpmode_lock;
	struct mutex modechange;
	struct mutex work_lock;
	struct mutex raw_data_lock;
	struct mutex i2c_mutex;
	struct mutex sponge_mutex;
	struct mutex power_init;

	void (*register_cb)(void *);
	struct tsp_callbacks callbacks;

#if ESD_TIMER_INTERVAL
	struct work_struct tmr_work;
	struct timer_list esd_timeout_tmr;
	struct timer_list *p_esd_timeout_tmr;
	struct mutex lock;
#endif
	u16 touch_mode;
	s16 cur_data[MAX_TRAW_DATA_SZ];
	s16 sensitivity_data[TOUCH_SENTIVITY_MEASUREMENT_COUNT];
	u8 update;

	struct tsp_raw_data *raw_data;
	struct sec_cmd_data sec;

	struct delayed_work work_read_info;
	bool info_work_done;

	struct delayed_work ghost_check;
	u8 tsp_dump_lock;

	struct completion resume_done;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	atomic_t secure_enabled;
	atomic_t secure_pending_irqs;
	struct completion secure_powerdown;
	struct completion secure_interrupt;
#endif
	struct ts_test_result	test_result;

	s16 Gap_max_x;
	s16 Gap_max_y;
	s16 Gap_max_val;
	s16 Gap_min_x;
	s16 Gap_min_y;
	s16 Gap_min_val;
	s16 Gap_Gap_val;
	s16 Gap_node_num;

	struct pinctrl *pinctrl;
	bool tsp_pwr_enabled;
#ifdef CONFIG_VBUS_NOTIFIER
	struct notifier_block vbus_nb;
#endif
	u8 cover_type;
	bool flip_enable;
	bool spay_enable;
	bool fod_enable;
	bool fod_lp_mode;
	bool singletap_enable;
	bool aod_enable;
	bool aot_enable;
	bool sleep_mode;
	bool glove_touch;
	u8 lpm_mode;
	zt_setting m_optional_mode;

	bool fod_with_finger_packet;
	u8 fod_mode_set;
	u8 fod_info_vi_trx[3];
	u16 fod_info_vi_data_len;
	u16 fod_rect[4];

	u16 aod_rect[4];
	u16 aod_active_area[3];

	unsigned int scrub_id;
	unsigned int scrub_x;
	unsigned int scrub_y;

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

	u8 check_multi;
	unsigned int multi_count;
	unsigned int wet_count;
	u8 touched_finger_num;
	struct delayed_work work_print_info;
	int print_info_cnt_open;
	int print_info_cnt_release;
	unsigned int comm_err_count;
	u16 pressed_x[MAX_SUPPORTED_FINGER_NUM];
	u16 pressed_y[MAX_SUPPORTED_FINGER_NUM];
	long prox_power_off;

	u8 ed_enable;
	int pocket_enable;
	u16 hover_event; /* keystring for protos */
	u16 store_reg_data;

	int noise_flag;
	int flip_cover_flag;

	u8 ito_test[4];
	struct sec_tclm_data *tdata;
};

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
struct zt_ts_info *tui_tsp_info;
extern int tui_force_close(uint32_t arg);
#endif

#ifdef CONFIG_SAMSUNG_TUI
struct zt_ts_info *tui_tsp_info;
#endif

#ifdef CONFIG_DISPLAY_SAMSUNG
extern int get_lcd_attached(char *mode);
#endif
#if defined(CONFIG_EXYNOS_DPU30)
int get_lcd_info(char *arg);
#endif
void zt_print_info(struct zt_ts_info *info);
bool shutdown_is_on_going_tsp;
static int ts_set_touchmode(u16 value);

#if ESD_TIMER_INTERVAL
static void esd_timer_stop(struct zt_ts_info *info);
static void esd_timer_start(u16 sec, struct zt_ts_info *info);
#endif

void zt_delay(int ms)
{
	if (ms > 20)
		msleep(ms);
	else
		usleep_range(ms * 1000, ms * 1000);
}

/* define i2c sub functions*/
static inline s32 read_data(struct i2c_client *client,
		u16 reg, u8 *values, u16 length)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);
	s32 ret;
	int count = 0;

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &client->dev,
				"%s TSP power off\n", __func__);
		return -EIO;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &client->dev,
				"%s TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif

	mutex_lock(&info->i2c_mutex);

retry:
	/* select register*/
	ret = i2c_master_send(client , (u8 *)&reg , 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: send failed %d, retry %d\n", __func__, ret, count);
		zt_delay(1);

		if (++count < RETRY_CNT)
			goto retry;

		info->comm_err_count++;
		mutex_unlock(&info->i2c_mutex);
		goto I2C_ERROR;
	}
	/* for setup tx transaction. */
	usleep_range(DELAY_FOR_TRANSCATION, DELAY_FOR_TRANSCATION);
	ret = i2c_master_recv(client , values , length);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: recv failed %d\n", __func__, ret);
		info->comm_err_count++;
		mutex_unlock(&info->i2c_mutex);
		goto I2C_ERROR;
	}

	usleep_range(DELAY_FOR_POST_TRANSCATION, DELAY_FOR_POST_TRANSCATION);
	mutex_unlock(&info->i2c_mutex);
	return length;

I2C_ERROR:
	if (info->work_state == PROBE) {
		input_err(true, &client->dev,
				"%s work state is PROBE.\n", __func__);
		return ret;
	}
	if (info->work_state == ESD_TIMER) {
		input_err(true, &client->dev,
				"%s reset work queue be working.\n", __func__);
		return -EIO;
	}
	info->work_state = NOTHING;
	esd_timer_stop(info);
	queue_work(esd_tmr_workqueue, &info->tmr_work);
	return ret;
}

static inline s32 write_data(struct i2c_client *client,
		u16 reg, u8 *values, u16 length)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);
	s32 ret;
	int count = 0;
	u8 pkt[66]; /* max packet */

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &client->dev,
				"%s TSP power off\n", __func__);
		return -EIO;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &client->dev,
				"%s TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif

	mutex_lock(&info->i2c_mutex);

	pkt[0] = (reg) & 0xff; /* reg addr */
	pkt[1] = (reg >> 8)&0xff;
	memcpy((u8 *)&pkt[2], values, length);

retry:
	ret = i2c_master_send(client , pkt , length + 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed %d, retry %d\n", __func__, ret, count);
		usleep_range(1 * 1000, 1 * 1000);

		if (++count < RETRY_CNT)
			goto retry;

		info->comm_err_count++;
		mutex_unlock(&info->i2c_mutex);
		goto I2C_ERROR;
	}

	usleep_range(DELAY_FOR_POST_TRANSCATION, DELAY_FOR_POST_TRANSCATION);
	mutex_unlock(&info->i2c_mutex);
	return length;
I2C_ERROR:
	if (info->work_state == PROBE) {
		input_err(true, &client->dev,
				"%s work state is PROBE.\n", __func__);
		return ret;
	}
	if (info->work_state == ESD_TIMER) {
		input_err(true, &client->dev,
				"%s reset work queue be working.\n", __func__);
		return -EIO;
	}
	info->work_state = NOTHING;
	esd_timer_stop(info);
	queue_work(esd_tmr_workqueue, &info->tmr_work);
	return ret;

}

static inline s32 write_reg(struct i2c_client *client, u16 reg, u16 value)
{
	if (write_data(client, reg, (u8 *)&value, 2) < 0)
		return I2C_FAIL;

	return I2C_SUCCESS;
}

static inline s32 write_cmd(struct i2c_client *client, u16 reg)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);
	s32 ret;
	int count = 0;

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &client->dev,
				"%s TSP power off\n", __func__);
		return -EIO;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &client->dev,
				"%s TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif

	mutex_lock(&info->i2c_mutex);

retry:
	ret = i2c_master_send(client , (u8 *)&reg , 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed %d, retry %d\n", __func__, ret, count);
		zt_delay(1);

		if (++count < RETRY_CNT)
			goto retry;

		info->comm_err_count++;
		mutex_unlock(&info->i2c_mutex);
		goto I2C_ERROR;
	}

	usleep_range(DELAY_FOR_POST_TRANSCATION, DELAY_FOR_POST_TRANSCATION);
	mutex_unlock(&info->i2c_mutex);
	return I2C_SUCCESS;

I2C_ERROR:
	if (info->work_state == PROBE) {
		input_err(true, &client->dev,
				"%s work state is PROBE.\n", __func__);
		return ret;
	}
	if (info->work_state == ESD_TIMER) {
		input_err(true, &client->dev,
				"%s reset work queue be working.\n", __func__);
		return -EIO;
	}
	info->work_state = NOTHING;
	esd_timer_stop(info);
	queue_work(esd_tmr_workqueue, &info->tmr_work);
	return ret;

}

static inline s32 read_raw_data(struct i2c_client *client,
		u16 reg, u8 *values, u16 length)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);
	s32 ret;
	int count = 0;

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &client->dev,
				"%s TSP power off\n", __func__);
		return -EIO;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &client->dev,
				"%s TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif

	mutex_lock(&info->i2c_mutex);

retry:
	/* select register */
	ret = i2c_master_send(client , (u8 *)&reg , 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: send failed %d, retry %d\n", __func__, ret, count);
		zt_delay(1);

		if (++count < RETRY_CNT)
			goto retry;

		info->comm_err_count++;
		mutex_unlock(&info->i2c_mutex);
		goto I2C_ERROR;
	}

	/* for setup tx transaction. */
	usleep_range(200, 200);

	ret = i2c_master_recv(client , values , length);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: recv failed %d\n", __func__, ret);
		info->comm_err_count++;
		mutex_unlock(&info->i2c_mutex);
		goto I2C_ERROR;
	}

	usleep_range(DELAY_FOR_POST_TRANSCATION, DELAY_FOR_POST_TRANSCATION);
	mutex_unlock(&info->i2c_mutex);
	return length;

I2C_ERROR:
	if (info->work_state == PROBE) {
		input_err(true, &client->dev,
				"%s work state is PROBE.\n", __func__);
		return ret;
	}
	if (info->work_state == ESD_TIMER) {
		input_err(true, &client->dev,
				"%s reset work be working.\n", __func__);
		return -EIO;
	}
	info->work_state = NOTHING;
	esd_timer_stop(info);
	queue_work(esd_tmr_workqueue, &info->tmr_work);
	return ret;
}

static inline s32 read_firmware_data(struct i2c_client *client,
		u16 addr, u8 *values, u16 length)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);
	s32 ret;

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &client->dev,
				"%s TSP power off\n", __func__);
		return -EIO;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &client->dev,
				"%s TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif

	mutex_lock(&info->i2c_mutex);

	/* select register*/
	ret = i2c_master_send(client , (u8 *)&addr , 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: send failed %d\n", __func__, ret);
		info->comm_err_count++;
		mutex_unlock(&info->i2c_mutex);
		return ret;
	}

	/* for setup tx transaction. */
	zt_delay(1);

	ret = i2c_master_recv(client , values , length);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: recv failed %d\n", __func__, ret);
		info->comm_err_count++;
		mutex_unlock(&info->i2c_mutex);
		return ret;
	}

	usleep_range(DELAY_FOR_POST_TRANSCATION, DELAY_FOR_POST_TRANSCATION);
	mutex_unlock(&info->i2c_mutex);
	return length;
}

static void zt_set_optional_mode(struct zt_ts_info *info, int event, bool enable)
{
	mutex_lock(&info->power_init);
	mutex_lock(&info->set_reg_lock);
	if (enable)
		zinitix_bit_set(info->m_optional_mode.select_mode.flag, event);
	else
		zinitix_bit_clr(info->m_optional_mode.select_mode.flag, event);

	if (write_reg(info->client, ZT_OPTIONAL_SETTING, info->m_optional_mode.optional_mode) != I2C_SUCCESS)
		input_info(true, &info->client->dev, "%s, fail optional mode set\n", __func__);

	mutex_unlock(&info->set_reg_lock);
	mutex_unlock(&info->power_init);
}

static int ts_read_from_sponge(struct zt_ts_info *info, u16 offset, u8* value, int len)
{
	int ret = 0;
	u8 pkt[66];

	pkt[0] = offset & 0xFF;
	pkt[1] = (offset >> 8) & 0xFF;

	pkt[2] = len & 0xFF;
	pkt[3] = (len >> 8) & 0xFF;

	mutex_lock(&info->power_init);
	mutex_lock(&info->sponge_mutex);
	if (write_data(info->client, ZT_SPONGE_READ_REG, (u8 *)&pkt, 4) < 0) {
		input_err(true, &info->client->dev, "%s: fail to write sponge command\n", __func__);
		ret = -EIO;
	}

	if (read_data(info->client, ZT_SPONGE_READ_REG, value, len) < 0) {
		input_err(true, &info->client->dev, "%s: fail to read sponge command\n", __func__);
		ret = -EIO;
	}
	mutex_unlock(&info->sponge_mutex);
	mutex_unlock(&info->power_init);

	return ret;
}

static int ts_write_to_sponge(struct zt_ts_info *info, u16 offset, u8 *value, int len)
{
	int ret = 0;
	u8 pkt[66];

	mutex_lock(&info->power_init);
	mutex_lock(&info->sponge_mutex);

	pkt[0] = offset & 0xFF;
	pkt[1] = (offset >> 8) & 0xFF;

	pkt[2] = len & 0xFF;
	pkt[3] = (len >> 8) & 0xFF;

	memcpy((u8 *)&pkt[4], value, len);

	if (write_data(info->client, ZT_SPONGE_WRITE_REG, (u8 *)&pkt, len + 4) < 0) {
		input_err(true, &info->client->dev, "%s: Failed to write offset\n", __func__);
		ret = -EIO;
	}

	if (write_cmd(info->client, ZT_SPONGE_SYNC_REG) != I2C_SUCCESS) {
		input_err(true, &info->client->dev, "%s: Failed to send notify\n", __func__);
		ret = -EIO;
	}
	mutex_unlock(&info->sponge_mutex);
	mutex_unlock(&info->power_init);

	return ret;
}

static void ts_set_utc_sponge(struct zt_ts_info *info)
{
	struct timeval current_time;
	int ret;
	u8 data[4] = {0, 0};

	do_gettimeofday(&current_time);
	data[0] = (0xFF & (u8)((current_time.tv_sec) >> 0));
	data[1] = (0xFF & (u8)((current_time.tv_sec) >> 8));
	data[2] = (0xFF & (u8)((current_time.tv_sec) >> 16));
	data[3] = (0xFF & (u8)((current_time.tv_sec) >> 24));
	input_info(true, &info->client->dev, "Write UTC to Sponge = %X\n", (int)(current_time.tv_sec));

	ret = ts_write_to_sponge(info, ZT_SPONGE_UTC, (u8*)&data, 4);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: Failed to write sponge\n", __func__);
}

int get_fod_info(struct zt_ts_info *info)
{
	u8 data[6] = {0, };
	int ret, i;

	ret = ts_read_from_sponge(info, ZT_SPONGE_AOD_ACTIVE_INFO, data, 6);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: Failed to read rect\n", __func__);
		return ret;
	}

	for (i = 0; i < 3; i++)
		info->aod_active_area[i] = (data[i * 2 + 1] & 0xFF) << 8 | (data[i * 2] & 0xFF);

	input_info(true, &info->client->dev, "%s: top:%d, edge:%d, bottom:%d\n",
			__func__, info->aod_active_area[0], info->aod_active_area[1], info->aod_active_area[2]);

	return ret;
}

int get_aod_active_area(struct zt_ts_info *info)
{
	int ret;

	/* get fod information */
	ret = ts_read_from_sponge(info, ZT_SPONGE_FOD_INFO, info->fod_info_vi_trx, 3);
	if (ret < 0)
		input_err(true, &info->client->dev,"%s: fail fod channel info.\n", __func__);
	
	info->fod_info_vi_data_len = info->fod_info_vi_trx[2];
	
	input_info(true, &info->client->dev, "%s: fod info %d,%d,%d\n", __func__,
			info->fod_info_vi_trx[0], info->fod_info_vi_trx[1], info->fod_info_vi_data_len);

	return ret;
}

void ts_check_custom_library(struct zt_ts_info *info)
{
	u8 data[10] = { 0 };
	int ret;

	ret = read_data(info->client, ZT_SPONGE_READ_INFO, &data[0], 10);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: fail to read status reg\n", __func__);

	input_info(true, &info->client->dev,
			"%s: (%d) %c%c%c%c, || %02X, %02X, %02X, %02X, || %02X, %02X\n",
			__func__, ret, data[0], data[1], data[2], data[3], data[4],
			data[5], data[6], data[7], data[8], data[9]);

	ret = get_aod_active_area(info);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: fail to get aod area\n", __func__);

	ret = get_fod_info(info);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: fail to read fod info\n", __func__);

	data[0] = 0;
	ret = read_data(info->client, ZT_GET_FOD_WITH_FINGER_PACKET, data, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: fail to read fod_with_finger_packet, set as false\n", __func__);
		info->fod_with_finger_packet = false;
	} else {
		input_info(true, &info->client->dev, "%s: fod_with_finger_packet: %d\n", __func__, data[0]);
		info->fod_with_finger_packet = !!data[0];
	}
}

static void zt_set_lp_mode(struct zt_ts_info *info, int event, bool enable)
{
	int ret;

	mutex_lock(&info->set_lpmode_lock);

	if (enable)
		zinitix_bit_set(info->lpm_mode, event);
	else
		zinitix_bit_clr(info->lpm_mode, event);

	ret = ts_write_to_sponge(info, ZT_SPONGE_LP_FEATURE, &info->lpm_mode, 1);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: fail to write sponge\n", __func__);

	mutex_unlock(&info->set_lpmode_lock);
}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
static irqreturn_t zt_touch_work(int irq, void *data);
static void clear_report_data(struct zt_ts_info *info);
static irqreturn_t secure_filter_interrupt(struct zt_ts_info *info)
{
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLED) {
		if (atomic_cmpxchg(&info->secure_pending_irqs, 0, 1) == 0) {
			sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");
		} else {
			input_info(true, &info->client->dev, "%s: pending irq:%d\n",
					__func__, (int)atomic_read(&info->secure_pending_irqs));
		}

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

/**
 * Sysfs attr group for secure touch & interrupt handler for Secure world.
 * @atomic : syncronization for secure_enabled
 * @pm_runtime : set rpm_resume or rpm_ilde
 */
static ssize_t secure_touch_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct zt_ts_info *info = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d", atomic_read(&info->secure_enabled));
}

static ssize_t secure_touch_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct zt_ts_info *info = dev_get_drvdata(dev);
	int ret;
	unsigned long data;

	if (count > 2) {
		input_err(true, &info->client->dev,
				"%s: cmd length is over (%s,%d)!!\n",
				__func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	ret = kstrtoul(buf, 10, &data);
	if (ret != 0) {
		input_err(true, &info->client->dev, "%s: failed to read:%d\n",
				__func__, ret);
		return -EINVAL;
	}

	if (data == 1) {
		/* Enable Secure World */
		if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLED) {
			input_err(true, &info->client->dev, "%s: already enabled\n", __func__);
			return -EBUSY;
		}

		zt_delay(200);
		/* syncronize_irq -> disable_irq + enable_irq
		 * concern about timing issue.
		 */
		mutex_lock(&info->work_lock);
		disable_irq(info->client->irq);

		/* zinitix timer stop, release all finger */
#if ESD_TIMER_INTERVAL
		write_reg(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL, 0);
		esd_timer_stop(info);
#endif
		clear_report_data(info);

		if (pm_runtime_get_sync(info->client->adapter->dev.parent) < 0) {
			enable_irq(info->client->irq);
#if ESD_TIMER_INTERVAL
			esd_timer_start(CHECK_ESD_TIMER, info);
			write_reg(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL, SCAN_RATE_HZ * ESD_TIMER_INTERVAL);
#endif
			input_err(true, &info->client->dev, "%s: failed to get pm_runtime\n", __func__);
			mutex_unlock(&info->work_lock);
			return -EIO;
		}

		reinit_completion(&info->secure_powerdown);
		reinit_completion(&info->secure_interrupt);

		zt_delay(10);

		atomic_set(&info->secure_enabled, 1);
		atomic_set(&info->secure_pending_irqs, 0);

		enable_irq(info->client->irq);

		input_info(true, &info->client->dev, "%s: secure touch enable\n", __func__);
		mutex_unlock(&info->work_lock);
	} else if (data == 0) {

		/* Disable Secure World */
		if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_DISABLED) {
			input_err(true, &info->client->dev, "%s: already disabled\n", __func__);
			return count;
		}

		zt_delay(200);

		pm_runtime_put_sync(info->client->adapter->dev.parent);
		atomic_set(&info->secure_enabled, 0);
		sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");
		zt_delay(10);

		clear_report_data(info);
		zt_touch_work(info->client->irq, info);

		complete(&info->secure_powerdown);
		complete(&info->secure_interrupt);

		if (old_ta_status != g_ta_connected)
			zt_set_optional_mode(info, DEF_OPTIONAL_MODE_USB_DETECT_BIT, g_ta_connected);

#if ESD_TIMER_INTERVAL
		/* zinitix timer start */
		esd_timer_start(CHECK_ESD_TIMER, info);
		write_reg(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL, SCAN_RATE_HZ * ESD_TIMER_INTERVAL);
#endif
		input_info(true, &info->client->dev, "%s: secure touch disable\n", __func__);

	} else {
		input_err(true, &info->client->dev, "%s: unsupported value, %ld\n", __func__, data);
		return -EINVAL;
	}

	return count;
}

static ssize_t secure_touch_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct zt_ts_info *info = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_DISABLED) {
		input_err(true, &info->client->dev, "%s: disabled\n", __func__);
		return -EBADF;
	}

	if (atomic_cmpxchg(&info->secure_pending_irqs, -1, 0) == -1) {
		input_err(true, &info->client->dev, "%s: pending irq -1\n", __func__);
		return -EINVAL;
	}

	if (atomic_cmpxchg(&info->secure_pending_irqs, 1, 0) == 1)
		val = 1;

	input_err(true, &info->client->dev, "%s: pending irq is %d\n",
			__func__, atomic_read(&info->secure_pending_irqs));

	complete(&info->secure_interrupt);

	return snprintf(buf, PAGE_SIZE, "%u", val);
}

static DEVICE_ATTR(secure_touch_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
		secure_touch_enable_show, secure_touch_enable_store);
static DEVICE_ATTR(secure_touch, S_IRUGO, secure_touch_show, NULL);

static struct attribute *secure_attr[] = {
	&dev_attr_secure_touch_enable.attr,
	&dev_attr_secure_touch.attr,
	NULL,
};

static struct attribute_group secure_attr_group = {
	.attrs = secure_attr,
};

static void secure_touch_init(struct zt_ts_info *info)
{
	input_info(true, &info->client->dev, "%s\n", __func__);

	init_completion(&info->secure_powerdown);
	init_completion(&info->secure_interrupt);
}

static void secure_touch_stop(struct zt_ts_info *info, bool stop)
{
	if (atomic_read(&info->secure_enabled)) {
		atomic_set(&info->secure_pending_irqs, -1);

		sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");

		if (stop)
			wait_for_completion_interruptible(&info->secure_powerdown);

		input_info(true, &info->client->dev, "%s: %d\n", __func__, stop);
	}
}
#endif

#ifdef CONFIG_INPUT_ENABLED
static int  zt_ts_open(struct input_dev *dev);
static void zt_ts_close(struct input_dev *dev);
#endif

static bool zt_power_control(struct zt_ts_info *info, u8 ctl);
static int zt_pinctrl_configure(struct zt_ts_info *info, bool active);

static bool init_touch(struct zt_ts_info *info);
static bool mini_init_touch(struct zt_ts_info *info);
static void clear_report_data(struct zt_ts_info *info);

#if ESD_TIMER_INTERVAL
static void esd_timer_init(struct zt_ts_info *info);
static void esd_timeout_handler(struct timer_list *t);
#endif

static void zt_display_rawdata(struct zt_ts_info *info, struct tsp_raw_data *raw_data, int type, int gap);
void zt_set_grip_type(struct zt_ts_info *info, u8 set_type);

#ifdef TCLM_CONCEPT
int get_zt_tsp_nvm_data(struct zt_ts_info *info, u8 addr, u8 *values, u16 length);
int set_zt_tsp_nvm_data(struct zt_ts_info *info, u8 addr, u8 *values, u16 length);
#endif

void location_detect(struct zt_ts_info *info, char *loc, int x, int y);

#ifdef USE_MISC_DEVICE
static long ts_misc_fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int ts_misc_fops_open(struct inode *inode, struct file *filp);
static int ts_misc_fops_close(struct inode *inode, struct file *filp);

static const struct file_operations ts_misc_fops = {
	.owner = THIS_MODULE,
	.open = ts_misc_fops_open,
	.release = ts_misc_fops_close,
	//.unlocked_ioctl = ts_misc_fops_ioctl,
	.compat_ioctl = ts_misc_fops_ioctl,
};

static struct miscdevice touch_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "zinitix_touch_misc",
	.fops = &ts_misc_fops,
};

#define TOUCH_IOCTL_BASE	0xbc
#define TOUCH_IOCTL_GET_DEBUGMSG_STATE		_IOW(TOUCH_IOCTL_BASE, 0, int)
#define TOUCH_IOCTL_SET_DEBUGMSG_STATE		_IOW(TOUCH_IOCTL_BASE, 1, int)
#define TOUCH_IOCTL_GET_CHIP_REVISION		_IOW(TOUCH_IOCTL_BASE, 2, int)
#define TOUCH_IOCTL_GET_FW_VERSION			_IOW(TOUCH_IOCTL_BASE, 3, int)
#define TOUCH_IOCTL_GET_REG_DATA_VERSION	_IOW(TOUCH_IOCTL_BASE, 4, int)
#define TOUCH_IOCTL_VARIFY_UPGRADE_SIZE		_IOW(TOUCH_IOCTL_BASE, 5, int)
#define TOUCH_IOCTL_VARIFY_UPGRADE_DATA		_IOW(TOUCH_IOCTL_BASE, 6, int)
#define TOUCH_IOCTL_START_UPGRADE			_IOW(TOUCH_IOCTL_BASE, 7, int)
#define TOUCH_IOCTL_GET_X_NODE_NUM			_IOW(TOUCH_IOCTL_BASE, 8, int)
#define TOUCH_IOCTL_GET_Y_NODE_NUM			_IOW(TOUCH_IOCTL_BASE, 9, int)
#define TOUCH_IOCTL_GET_TOTAL_NODE_NUM		_IOW(TOUCH_IOCTL_BASE, 10, int)
#define TOUCH_IOCTL_SET_RAW_DATA_MODE		_IOW(TOUCH_IOCTL_BASE, 11, int)
#define TOUCH_IOCTL_GET_RAW_DATA			_IOW(TOUCH_IOCTL_BASE, 12, int)
#define TOUCH_IOCTL_GET_X_RESOLUTION		_IOW(TOUCH_IOCTL_BASE, 13, int)
#define TOUCH_IOCTL_GET_Y_RESOLUTION		_IOW(TOUCH_IOCTL_BASE, 14, int)
#define TOUCH_IOCTL_HW_CALIBRAION			_IOW(TOUCH_IOCTL_BASE, 15, int)
#define TOUCH_IOCTL_GET_REG					_IOW(TOUCH_IOCTL_BASE, 16, int)
#define TOUCH_IOCTL_SET_REG					_IOW(TOUCH_IOCTL_BASE, 17, int)
#define TOUCH_IOCTL_SEND_SAVE_STATUS		_IOW(TOUCH_IOCTL_BASE, 18, int)
#define TOUCH_IOCTL_DONOT_TOUCH_EVENT		_IOW(TOUCH_IOCTL_BASE, 19, int)
#endif

struct zt_ts_info *misc_info;

static void zt_ts_esd_timer_stop(struct zt_ts_info *info)
{
#if ESD_TIMER_INTERVAL
	esd_timer_stop(info);
	write_reg(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL, 0);
	write_cmd(info->client, ZT_CLEAR_INT_STATUS_CMD);
#endif
}

static void zt_ts_esd_timer_start(struct zt_ts_info *info)
{
#if ESD_TIMER_INTERVAL
	esd_timer_start(CHECK_ESD_TIMER, info);
	write_reg(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL,
			SCAN_RATE_HZ * ESD_TIMER_INTERVAL);
#endif
}
static void set_cover_type(struct zt_ts_info *info, bool enable)
{
	struct i2c_client *client = info->client;

	mutex_lock(&info->power_init);

	if (enable) {
		switch (info->cover_type) {
		case ZT_FLIP_WALLET:
			write_reg(client, ZT_COVER_CONTROL_REG, WALLET_COVER_CLOSE);
			break;
		case ZT_VIEW_COVER:
			write_reg(client, ZT_COVER_CONTROL_REG, VIEW_COVER_CLOSE);
			break;
		case ZT_CLEAR_FLIP_COVER:
			write_reg(client, ZT_COVER_CONTROL_REG, CLEAR_COVER_CLOSE);
			break;
		case ZT_NEON_COVER:
			write_reg(client, ZT_COVER_CONTROL_REG, LED_COVER_CLOSE);
			break;
		case ZT_CLEAR_SIDE_VIEW_COVER:
			write_reg(client, ZT_COVER_CONTROL_REG, CLEAR_SIDE_VIEW_COVER_CLOSE);
			break;
		case ZT_MINI_SVIEW_WALLET_COVER:
			write_reg(client, ZT_COVER_CONTROL_REG, MINI_SVIEW_WALLET_COVER_CLOSE);
			break;
		default:
			input_err(true, &info->client->dev, "%s: touch is not supported for %d cover\n",
					__func__, info->cover_type);
		}
	} else {
		write_reg(client, ZT_COVER_CONTROL_REG, COVER_OPEN);
	}

	mutex_unlock(&info->power_init);

	input_info(true, &info->client->dev, "%s: type %d enable %d\n", __func__, info->cover_type, enable);
}

static bool get_raw_data(struct zt_ts_info *info, u8 *buff, int skip_cnt)
{
	struct i2c_client *client = info->client;
	struct zt_ts_platform_data *pdata = info->pdata;
	u32 total_node = info->cap_info.total_node_num;
	u32 sz;
	int i, j = 0;

	disable_irq(info->irq);

	mutex_lock(&info->work_lock);
	if (info->work_state != NOTHING) {
		input_info(true, &client->dev, "other process occupied.. (%d)\n",
				info->work_state);
		enable_irq(info->irq);
		mutex_unlock(&info->work_lock);
		return false;
	}

	info->work_state = RAW_DATA;

	for (i = 0; i < skip_cnt; i++) {
		while (gpio_get_value(pdata->gpio_int)) {
			zt_delay(1);
			if (++j > 3000) {
				input_err(true, &info->client->dev, "%s: (skip_cnt) wait int timeout\n", __func__);
				break;
			}
		}

		write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
		zt_delay(1);
	}

	input_dbg(true, &info->client->dev, "%s read raw data\r\n", __func__);
	sz = total_node*2;

	j = 0;
	while (gpio_get_value(pdata->gpio_int)) {
		zt_delay(1);
		if (++j > 3000) {
			input_err(true, &info->client->dev, "%s: wait int timeout\n", __func__);
			break;
		}
	}

	if (read_raw_data(client, ZT_RAWDATA_REG, (char *)buff, sz) < 0) {
		input_info(true, &client->dev, "%s: error : read zinitix tc raw data\n", __func__);
		info->work_state = NOTHING;
		clear_report_data(info);
		enable_irq(info->irq);
		mutex_unlock(&info->work_lock);
		return false;
	}

	write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
	clear_report_data(info);
	info->work_state = NOTHING;
	enable_irq(info->irq);
	mutex_unlock(&info->work_lock);

	return true;
}

static bool ts_get_raw_data(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	u32 total_node = info->cap_info.total_node_num;
	u32 sz;

	if (!mutex_trylock(&info->raw_data_lock)) {
		input_err(true, &client->dev, "%s: Failed to occupy mutex\n", __func__);
		return true;
	}

	sz = total_node * 2 + sizeof(struct point_info) * MAX_SUPPORTED_FINGER_NUM;

	if (read_raw_data(info->client, ZT_RAWDATA_REG,
				(char *)info->cur_data, sz) < 0) {
		input_err(true, &client->dev, "%s: Failed to read raw data\n", __func__);
		mutex_unlock(&info->raw_data_lock);
		return false;
	}

	info->update = 1;
	memcpy((u8 *)(&info->touch_info[0]),
			(u8 *)&info->cur_data[total_node],
			sizeof(struct point_info) * MAX_SUPPORTED_FINGER_NUM);
	mutex_unlock(&info->raw_data_lock);

	return true;
}

static void zt_ts_fod_event_report(struct zt_ts_info *info, struct point_info touch_info)
{
	if (!info->fod_enable)
		return;

	if ((touch_info.byte01.value_u8bit == 0)
			 || (touch_info.byte01.value_u8bit == 1)) {
		info->scrub_id = SPONGE_EVENT_TYPE_FOD_PRESS;

		info->scrub_x = ((touch_info.byte02.value_u8bit << 4) & 0xFF0)
			| ((touch_info.byte04.value_u8bit & 0xF0) >> 4);
		info->scrub_y = ((touch_info.byte03.value_u8bit << 4) & 0xFF0)
			| ((touch_info.byte04.value_u8bit & 0x0F));

		input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
		input_sync(info->input_dev);
		input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
		input_sync(info->input_dev);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
		input_info(true, &info->client->dev, "%s: FOD %s PRESS: %d\n", __func__,
				touch_info.byte01.value_u8bit ? "NORMAL" : "LONG", info->scrub_id);
#else
		input_info(true, &info->client->dev, "%s: FOD %s PRESS: %d, %d, %d\n", __func__,
				touch_info.byte01.value_u8bit ? "NORMAL" : "LONG",
				info->scrub_id, info->scrub_x, info->scrub_y);
#endif
	} else if (touch_info.byte01.value_u8bit == 2) {
		info->scrub_id = SPONGE_EVENT_TYPE_FOD_RELEASE;

		info->scrub_x = ((touch_info.byte02.value_u8bit << 4) & 0xFF0)
			| ((touch_info.byte04.value_u8bit & 0xF0) >> 4);
		info->scrub_y = ((touch_info.byte03.value_u8bit << 4) & 0xFF0)
			| ((touch_info.byte04.value_u8bit & 0x0F));

		input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
		input_sync(info->input_dev);
		input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
		input_sync(info->input_dev);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
		input_info(true, &info->client->dev, "%s: FOD RELEASE: %d\n", __func__, info->scrub_id);
#else
		input_info(true, &info->client->dev, "%s: FOD RELEASE: %d, %d, %d\n",
				__func__, info->scrub_id, info->scrub_x, info->scrub_y);
#endif
	} else if (touch_info.byte01.value_u8bit == 3) {
		info->scrub_id = SPONGE_EVENT_TYPE_FOD_OUT;

		info->scrub_x = ((touch_info.byte02.value_u8bit << 4) & 0xFF0)
			| ((touch_info.byte04.value_u8bit & 0xF0) >> 4);
		info->scrub_y = ((touch_info.byte03.value_u8bit << 4) & 0xFF0)
			| ((touch_info.byte04.value_u8bit & 0x0F));

		input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
		input_sync(info->input_dev);
		input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
		input_sync(info->input_dev);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
		input_info(true, &info->client->dev, "%s: FOD OUT: %d\n", __func__, info->scrub_id);
#else
		input_info(true, &info->client->dev, "%s: FOD OUT: %d, %d, %d\n",
				__func__, info->scrub_id, info->scrub_x, info->scrub_y);
#endif
	}
}

static bool ts_read_coord(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	int i;
	u16 status_data;
	u16 pocket_data;

	/* for  Debugging Tool */

	if (info->touch_mode != TOUCH_POINT_MODE) {
		if (ts_get_raw_data(info) == false)
			return false;

		if (info->touch_mode == TOUCH_SENTIVITY_MEASUREMENT_MODE) {
			for (i = 0; i < TOUCH_SENTIVITY_MEASUREMENT_COUNT; i++) {
				info->sensitivity_data[i] = info->cur_data[i];
			}
		}

		goto out;
	}

	if (info->pocket_enable) {
		if (read_data(info->client, ZT_STATUS_REG, (u8 *)&status_data, 2) < 0) {
			input_err(true, &client->dev, "%s: fail to read status reg\n", __func__);
		}

		if (zinitix_bit_test(status_data, BIT_POCKET_MODE)) {
			if (read_data(info->client, ZT_POCKET_DETECT, (u8 *)&pocket_data, 2) < 0) {
				input_err(true, &client->dev, "%s: fail to read pocket reg\n", __func__);
			} else if (info->input_dev_proximity) {
				input_info(true, &client->dev, "Pocket %s \n", pocket_data == 11 ? "IN" : "OUT");
				input_report_abs(info->input_dev_proximity, ABS_MT_CUSTOM, pocket_data);
				input_sync(info->input_dev_proximity);
			} else {
				input_err(true, &client->dev, "%s: no dev for proximity\n", __func__);
			}
		}
	}

	memset(info->touch_info, 0x0, sizeof(struct point_info) * MAX_SUPPORTED_FINGER_NUM);

	if (read_data(info->client, ZT_POINT_STATUS_REG,
				(u8 *)(&info->touch_info[0]), sizeof(struct point_info)) < 0) {
		input_err(true, &client->dev, "Failed to read point info\n");
		return false;
	}

	if (info->fod_enable && info->fod_with_finger_packet) {
		memset(&info->touch_fod_info, 0x0, sizeof(struct point_info));

		if (read_data(info->client, ZT_FOD_STATUS_REG,
			(u8 *)(&info->touch_fod_info), sizeof(struct point_info)) < 0) {
			input_err(true, &client->dev, "Failed to read Touch FOD info\n");
			return false;
		}

		memset(info->fod_touch_vi_data, 0x0, info->fod_info_vi_data_len);

		if (info->fod_info_vi_data_len > 0) {
			if (read_data(info->client, ZT_VI_STATUS_REG,
				info->fod_touch_vi_data, info->fod_info_vi_data_len) < 0) {
				input_err(true, &client->dev, "Failed to read Touch VI Data\n");
				return false;
			}
		}

		if (info->touch_fod_info.byte00.value.eid == GESTURE_EVENT
				&& info->touch_fod_info.byte00.value.tid == FINGERPRINT)
			zt_ts_fod_event_report(info, info->touch_fod_info);
	}

	if (info->touch_info[0].byte00.value.eid == COORDINATE_EVENT) {
		info->touched_finger_num = info->touch_info[0].byte07.value.left_event;
		if (info->touched_finger_num > 0) {
			if (read_data(info->client, ZT_POINT_STATUS_REG1, (u8 *)(&info->touch_info[1]),
						(info->touched_finger_num)*sizeof(struct point_info)) < 0) {
				input_err(true, &client->dev, "Failed to read touched point info\n");
				return false;
			}
		}
	} else if (info->touch_info[0].byte00.value.eid == GESTURE_EVENT) {
		if (info->touch_info[0].byte00.value.tid == SWIPE_UP) {
			input_info(true, &client->dev, "%s: Spay Gesture\n", __func__);

			info->scrub_id = SPONGE_EVENT_TYPE_SPAY;
			info->scrub_x = 0;
			info->scrub_y = 0;

			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
			input_sync(info->input_dev);
			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
			input_sync(info->input_dev);
		} else if (info->touch_info[0].byte00.value.tid == FINGERPRINT) {
				zt_ts_fod_event_report(info, info->touch_info[0]);
		} else if (info->touch_info[0].byte00.value.tid == SINGLE_TAP) {
			if (info->singletap_enable) {
				info->scrub_id = SPONGE_EVENT_TYPE_SINGLE_TAP;

				info->scrub_x = ((info->touch_info[0].byte02.value_u8bit << 4) & 0xFF0)
					| ((info->touch_info[0].byte04.value_u8bit & 0xF0) >> 4);
				info->scrub_y = ((info->touch_info[0].byte03.value_u8bit << 4) & 0xFF0)
					| ((info->touch_info[0].byte04.value_u8bit & 0x0F));

				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
				input_sync(info->input_dev);
				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
				input_sync(info->input_dev);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &client->dev, "%s: SINGLE TAP: %d\n", __func__, info->scrub_id);
#else
				input_info(true, &client->dev, "%s: SINGLE TAP: %d, %d, %d\n",
						__func__, info->scrub_id, info->scrub_x, info->scrub_y);
#endif
			}
		} else if (info->touch_info[0].byte00.value.tid == DOUBLE_TAP) {
			if (info->aot_enable && (info->touch_info[0].byte01.value_u8bit == 1)) {
				input_report_key(info->input_dev, KEY_WAKEUP, 1);
				input_sync(info->input_dev);
				input_report_key(info->input_dev, KEY_WAKEUP, 0);
				input_sync(info->input_dev);

				/* request from sensor team */
				if (info->input_dev_proximity) {
					input_report_abs(info->input_dev_proximity, ABS_MT_CUSTOM2, 1);
					input_sync(info->input_dev_proximity);
					input_report_abs(info->input_dev_proximity, ABS_MT_CUSTOM2, 0);
					input_sync(info->input_dev_proximity);
				}

				input_info(true, &client->dev, "%s: AOT Doubletab\n", __func__);
			} else if (info->aod_enable && (info->touch_info[0].byte01.value_u8bit == 0)) {
				info->scrub_id = SPONGE_EVENT_TYPE_AOD_DOUBLETAB;

				info->scrub_x = ((info->touch_info[0].byte02.value_u8bit << 4) & 0xFF0)
					| ((info->touch_info[0].byte04.value_u8bit & 0xF0) >> 4);
				info->scrub_y = ((info->touch_info[0].byte03.value_u8bit << 4) & 0xFF0)
					| ((info->touch_info[0].byte04.value_u8bit & 0x0F));

				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
				input_sync(info->input_dev);
				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
				input_sync(info->input_dev);

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &client->dev, "%s: AOD Doubletab: %d\n", __func__, info->scrub_id);
#else
				input_info(true, &client->dev, "%s: AOD Doubletab: %d, %d, %d\n",
						__func__, info->scrub_id, info->scrub_x, info->scrub_y);
#endif
			}
		}
	}
out:
	write_cmd(info->client, ZT_CLEAR_INT_STATUS_CMD);
	return true;
}

#if ESD_TIMER_INTERVAL
static void esd_timeout_handler(struct timer_list *t)
{
	struct zt_ts_info *info = from_timer(info, t, esd_timeout_tmr);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	struct i2c_client *client = info->client;
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &client->dev,
				"%s TSP no accessible from Linux, TUI is enabled!\n", __func__);
		esd_timer_stop(info);
		return;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	struct i2c_client *client = info->client;

	if (STUI_MODE_TOUCH_SEC & stui_get_mode()) {
		input_err(true, &client->dev,
				"%s TSP not accessible during TUI\n", __func__);
		return;
	}
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &info->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		esd_timer_stop(info);
		return;
	}
#endif
	info->p_esd_timeout_tmr = NULL;
	queue_work(esd_tmr_workqueue, &info->tmr_work);
}

static void esd_timer_start(u16 sec, struct zt_ts_info *info)
{
	if (info->sleep_mode) {
		input_info(true, &info->client->dev, "%s skip (sleep_mode)!\n", __func__);
		return;
	}

	mutex_lock(&info->lock);
	if (info->p_esd_timeout_tmr != NULL)
#ifdef CONFIG_SMP
		del_singleshot_timer_sync(info->p_esd_timeout_tmr);
#else
		del_timer(info->p_esd_timeout_tmr);
#endif
	info->p_esd_timeout_tmr = NULL;

	timer_setup(&info->esd_timeout_tmr, esd_timeout_handler, 0);
	mod_timer(&info->esd_timeout_tmr, jiffies + (HZ * sec));
	info->p_esd_timeout_tmr = &info->esd_timeout_tmr;
	mutex_unlock(&info->lock);
}

static void esd_timer_stop(struct zt_ts_info *info)
{
	mutex_lock(&info->lock);
	if (info->p_esd_timeout_tmr)
#ifdef CONFIG_SMP
		del_singleshot_timer_sync(info->p_esd_timeout_tmr);
#else
		del_timer(info->p_esd_timeout_tmr);
#endif

	info->p_esd_timeout_tmr = NULL;
	mutex_unlock(&info->lock);
}

static void esd_timer_init(struct zt_ts_info *info)
{
	mutex_lock(&info->lock);
	timer_setup(&info->esd_timeout_tmr, esd_timeout_handler, 0);
	info->p_esd_timeout_tmr = NULL;
	mutex_unlock(&info->lock);
}

static void ts_tmr_work(struct work_struct *work)
{
	struct zt_ts_info *info =
		container_of(work, struct zt_ts_info, tmr_work);
	struct i2c_client *client = info->client;

	input_info(true, &client->dev, "%s++\n", __func__);

	if (info->work_state != NOTHING) {
		input_info(true, &client->dev, "%s: Other process occupied (%d)\n",
				__func__, info->work_state);

		return;
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &client->dev, "%s: ignored, because touch is in secure mode\n", __func__);
		return;
	}
#endif

	info->work_state = ESD_TIMER;

	disable_irq_nosync(info->irq);
	zt_power_control(info, POWER_OFF);
	zt_power_control(info, POWER_ON_SEQUENCE);

	clear_report_data(info);
	if (mini_init_touch(info) == false)
		goto fail_time_out_init;

	info->work_state = NOTHING;
	enable_irq(info->irq);
#if defined(TSP_VERBOSE_DEBUG)
	input_info(true, &client->dev, "%s--\n", __func__);
#endif

	return;
fail_time_out_init:
	input_err(true, &client->dev, "%s: Failed to restart\n", __func__);
	esd_timer_start(CHECK_ESD_TIMER, info);
	info->work_state = NOTHING;
	enable_irq(info->irq);
	return;
}
#endif

static bool zt_power_sequence(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	u16 chip_code;
	u16 checksum;

	if (read_data(client, ZT_CHECKSUM_RESULT, (u8 *)&checksum, 2) < 0) {
		input_err(true, &client->dev, "%s: Failed to read checksum\n", __func__);
		goto fail_power_sequence;
	}

	if (checksum == CORRECT_CHECK_SUM)
		return true;

	if (write_reg(client, VCMD_ENABLE, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: Failed to send power sequence(vendor cmd enable)\n", __func__);
		goto fail_power_sequence;
	}
	usleep_range(10, 10);

	if (read_data(client, VCMD_ID, (u8 *)&chip_code, 2) < 0) {
		input_err(true, &client->dev, "%s: Failed to read chip code\n", __func__);
		goto fail_power_sequence;
	}

	input_info(true, &client->dev, "%s: chip code = 0x%x\n", __func__, chip_code);
	usleep_range(10, 10);

	if (write_cmd(client, VCMD_INTN_CLR) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: Failed to send power sequence(intn clear)\n", __func__);
		goto fail_power_sequence;
	}
	usleep_range(10, 10);

	if (write_reg(client, VCMD_NVM_INIT, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: Failed to send power sequence(nvm init)\n", __func__);
		goto fail_power_sequence;
	}
	zt_delay(2);

	if (write_reg(client, VCMD_NVM_PROG_START, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: Failed to send power sequence(program start)\n", __func__);
		goto fail_power_sequence;
	}

	zt_delay(FIRMWARE_ON_DELAY);	/* wait for checksum cal */

	if (read_data(client, ZT_CHECKSUM_RESULT, (u8 *)&checksum, 2) < 0)
		input_err(true, &client->dev, "%s: Failed to read checksum (retry)\n", __func__);

	if (checksum == CORRECT_CHECK_SUM)
		return true;
	else
		input_err(true, &client->dev, "%s: Failed to read checksum 0x%x\n", __func__, checksum);

fail_power_sequence:
	return false;
}

static bool zt_power_control(struct zt_ts_info *info, u8 ctl)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	input_info(true, &client->dev, "[TSP] %s, %d\n", __func__, ctl);

	mutex_lock(&info->power_init);
	if (ctl == POWER_OFF)
		info->tsp_pwr_enabled = ctl;

	ret = info->pdata->tsp_power(info, ctl);
	if (ret) {
		mutex_unlock(&info->power_init);
		return false;
	}

	zt_pinctrl_configure(info, ctl);

	if (ctl == POWER_ON_SEQUENCE) {
		zt_delay(CHIP_ON_DELAY);
		info->tsp_pwr_enabled = ctl;
		input_info(true, &client->dev, "[TSP] %s, info->tsp_pwr_enabled %d\n", __func__, info->tsp_pwr_enabled);
		ret =  zt_power_sequence(info);
		mutex_unlock(&info->power_init);
		return ret;
	} else if (ctl == POWER_OFF) {
		zt_delay(CHIP_OFF_DELAY);
	} else if (ctl == POWER_ON) {
		zt_delay(CHIP_ON_DELAY);
		info->tsp_pwr_enabled = ctl;
	}

	mutex_unlock(&info->power_init);

	input_info(true, &client->dev, "[TSP] %s, info->tsp_pwr_enabled %d\n", __func__, info->tsp_pwr_enabled);

	return true;
}

#ifdef CONFIG_VBUS_NOTIFIER
int tsp_vbus_notification(struct notifier_block *nb,
		unsigned long cmd, void *data)
{
	struct zt_ts_info *info = container_of(nb, struct zt_ts_info, vbus_nb);
	vbus_status_t vbus_type = *(vbus_status_t *)data;
	struct power_supply *psy_otg;
	union power_supply_propval val;
	int ret = 0;

	input_info(true, &info->client->dev, "%s cmd=%lu, vbus_type=%d\n", __func__, cmd, vbus_type);

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
		input_info(true, &info->client->dev, "%s : attach\n", __func__);
		g_ta_connected = true;
		break;
	case STATUS_VBUS_LOW:
		input_info(true, &info->client->dev, "%s : detach\n", __func__);
		g_ta_connected = false;
		break;
	default:
		break;
	}

	psy_otg = power_supply_get_by_name("otg");
	if (psy_otg) {
		ret = psy_otg->desc->get_property(psy_otg, POWER_SUPPLY_PROP_ONLINE, &val);
		if (ret) {
			input_err(true, &info->client->dev, "%s: fail to set power_suppy ONLINE property(%d)\n",
					__func__, ret);
		} else {
			zt_set_optional_mode(info, DEF_OPTIONAL_MODE_OTG_MODE, val.intval);
			input_info(true, &info->client->dev, "VBUS %s\n", val.intval ? "OTG" : "CHARGER");
			if (val.intval) {
				g_ta_connected = false;
			}
		}
	} else {
		input_err(true, &info->client->dev, "%s: Fail to get psy battery\n", __func__);
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled)) {
		input_info(true, &info->client->dev,
			"%s: ignored, because secure mode, old:%d, TA:%d\n",
			__func__, old_ta_status, g_ta_connected);
		return 0;
	} else {
		old_ta_status = g_ta_connected;
	}
#endif
	zt_set_optional_mode(info, DEF_OPTIONAL_MODE_USB_DETECT_BIT, g_ta_connected);
	return 0;
}
#endif

static void zt_charger_status_cb(struct tsp_callbacks *cb, bool ta_status)
{
	struct zt_ts_info *info =
		container_of(cb, struct zt_ts_info, callbacks);
	if (!ta_status)
		g_ta_connected = false;
	else
		g_ta_connected = true;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&info->secure_enabled)) {
		input_info(true, &info->client->dev,
			"%s: ignored, because secure mode, old:%d, TA:%d\n",
			__func__, old_ta_status, g_ta_connected);
		return;
	} else {
		old_ta_status = g_ta_connected;
	}
#endif

	zt_set_optional_mode(info, DEF_OPTIONAL_MODE_USB_DETECT_BIT, g_ta_connected);
	input_info(true, &info->client->dev, "TA %s\n", ta_status ? "connected" : "disconnected");
}

static bool crc_check(struct zt_ts_info *info)
{
	u16 chip_check_sum = 0;

	if (read_data(info->client, ZT_CHECKSUM_RESULT,
				(u8 *)&chip_check_sum, 2) < 0) {
		input_err(true, &info->client->dev, "%s: read crc fail", __func__);
	}

	input_info(true, &info->client->dev, "%s: 0x%04X\n", __func__, chip_check_sum);

	if (chip_check_sum == CORRECT_CHECK_SUM)
		return true;
	else
		return false;
}

static bool ts_check_need_upgrade(struct zt_ts_info *info,
		u16 cur_version, u16 cur_minor_version, u16 cur_reg_version, u16 cur_hw_id)
{
	u16	new_version;
	u16	new_minor_version;
	u16	new_reg_version;
#if CHECK_HWID
	u16	new_hw_id;
#endif

	new_version = (u16) (info->fw_data[52] | (info->fw_data[53]<<8));
	new_minor_version = (u16) (info->fw_data[56] | (info->fw_data[57]<<8));
	new_reg_version = (u16) (info->fw_data[60] | (info->fw_data[61]<<8));

#if CHECK_HWID
	new_hw_id = (u16) (fw_data[0x7528] | (fw_data[0x7529]<<8));
	input_info(true, &info->client->dev, "cur HW_ID = 0x%x, new HW_ID = 0x%x\n",
			cur_hw_id, new_hw_id);
	if (cur_hw_id != new_hw_id)
		return false;
#endif

	input_info(true, &info->client->dev, "cur version = 0x%x, new version = 0x%x\n",
			cur_version, new_version);
	input_info(true, &info->client->dev, "cur minor version = 0x%x, new minor version = 0x%x\n",
			cur_minor_version, new_minor_version);
	input_info(true, &info->client->dev, "cur reg data version = 0x%x, new reg data version = 0x%x\n",
			cur_reg_version, new_reg_version);

	if (info->pdata->bringup == 3) {
		input_info(true, &info->client->dev, "%s: bringup 3, update when version is different\n", __func__);
		if (cur_version == new_version
				&& cur_minor_version == new_minor_version
				&& cur_reg_version == new_reg_version)
			return false;
		else
			return true;
	} else if (info->pdata->bringup == 2) {
		input_info(true, &info->client->dev, "%s: bringup 2, skip update\n", __func__);
		return false;
	}

	if (cur_version > 0xFF)
		return true;
	if (cur_version < new_version)
		return true;
	else if (cur_version > new_version)
		return false;
	if (cur_minor_version < new_minor_version)
		return true;
	else if (cur_minor_version > new_minor_version)
		return false;
	if (cur_reg_version < new_reg_version)
		return true;

	return false;
}

#define TC_SECTOR_SZ		8
#define TC_NVM_SECTOR_SZ	64
#ifdef TCLM_CONCEPT
#define TC_SECTOR_SZ_WRITE		8
#define TC_SECTOR_SZ_READ		8
#endif

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7650)
#define TSP_PAGE_SIZE	1024
#define FUZING_UDELAY 28000
#endif

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7554)
#define TSP_PAGE_SIZE		128
#define FUZING_UDELAY	8000
#endif

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7548) || defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7538)
#define TSP_PAGE_SIZE		64
#define FUZING_UDELAY	8000
#endif

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7532)
#define TSP_PAGE_SIZE		64
#define FUZING_UDELAY	30000
#endif

#define USB_POR_OFF_DELAY	1500
#define USB_POR_ON_DELAY	1500

#define ZT76XX_UPGRADE_INFO_SIZE 1024

u32 ic_info_size, ic_info_checksum;
u32 ic_core_size, ic_core_checksum;
u32 ic_cust_size, ic_cust_checksum;
u32 ic_regi_size, ic_regi_checksum;
u32 fw_info_size, fw_info_checksum;
u32 fw_core_size, fw_core_checksum;
u32 fw_cust_size, fw_cust_checksum;
u32 fw_regi_size, fw_regi_checksum;
u8 kind_of_download_method;

static bool check_upgrade_method(struct zt_ts_info *info, const u8 *firmware_data, u16 chip_code)
{
	char bindata[0x80];
	u16 buff16[64];
	u16 flash_addr;
	int nsectorsize = 8;
	u32 chk_info_checksum;

	nvm_binary_info fw_info;
	nvm_binary_info ic_info;
	int i;

	struct i2c_client *client = info->client;

	if (write_reg(client, VCMD_ENABLE, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: check upgrade error (vcmd)\n", __func__);
		return false;
	}

	zt_delay(1);

	if (write_cmd(client, VCMD_INTN_CLR) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: check upgrade error (intn clear)\n", __func__);
		return false;
	}

	if (write_reg(client, VCMD_NVM_INIT, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: check upgrade error (nvm init)\n", __func__);
		return false;
	}

	zt_delay(5);
	memset(bindata, 0xff, sizeof(bindata));

	if (write_reg(client, VCMD_UPGRADE_INIT_FLASH, 0x0000) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: check upgrade error (init flash)\n", __func__);
		return false;
	}

	for (flash_addr = 0; flash_addr < 0x80; flash_addr += nsectorsize) {
		if (read_firmware_data(client, VCMD_UPGRADE_READ_FLASH,
					(u8*)&bindata[flash_addr], TC_SECTOR_SZ) < 0) {
			input_err(true, &client->dev, "%s: failed to read firmware\n", __func__);
			return false;
		}
	}

	memcpy(ic_info.buff32, bindata, 0x80);	//copy data
	memcpy(buff16, bindata, 0x80);			//copy data

	//BIG ENDIAN ==> LITTLE ENDIAN
	ic_info_size = (((u32)ic_info.val.info_size[1] << 16 & 0x00FF0000) | ((u32)ic_info.val.info_size[2] << 8 & 0x0000FF00) | ((u32)ic_info.val.info_size[3] & 0x000000FF));
	ic_core_size = (((u32)ic_info.val.core_size[1] << 16 & 0x00FF0000) | ((u32)ic_info.val.core_size[2] << 8 & 0x0000FF00) | ((u32)ic_info.val.core_size[3] & 0x000000FF));
	ic_cust_size = (((u32)ic_info.val.custom_size[1] << 16 & 0x00FF0000) | ((u32)ic_info.val.custom_size[2] << 8 & 0x0000FF00) | ((u32)ic_info.val.custom_size[3] & 0x000000FF));
	ic_regi_size = (((u32)ic_info.val.register_size[1] << 16 & 0x00FF0000) | ((u32)ic_info.val.register_size[2] << 8 & 0x0000FF00) | ((u32)ic_info.val.register_size[3] & 0x000000FF));

	memcpy(fw_info.buff32, firmware_data, 0x80);

	fw_info_size = (((u32)fw_info.val.info_size[1] << 16 & 0x00FF0000) | ((u32)fw_info.val.info_size[2] << 8 & 0x0000FF00) | ((u32)fw_info.val.info_size[3] & 0x000000FF));
	fw_core_size = (((u32)fw_info.val.core_size[1] << 16 & 0x00FF0000) | ((u32)fw_info.val.core_size[2] << 8 & 0x0000FF00) | ((u32)fw_info.val.core_size[3] & 0x000000FF));
	fw_cust_size = (((u32)fw_info.val.custom_size[1] << 16 & 0x00FF0000) | ((u32)fw_info.val.custom_size[2] << 8 & 0x0000FF00) | ((u32)fw_info.val.custom_size[3] & 0x000000FF));
	fw_regi_size = (((u32)fw_info.val.register_size[1] << 16 & 0x00FF0000) | ((u32)fw_info.val.register_size[2] << 8 & 0x0000FF00) | ((u32)fw_info.val.register_size[3] & 0x000000FF));


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//[DESC] Determining the execution conditions of [FULL D/L] or [Partial D/L]
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	kind_of_download_method = NEED_FULL_DL;

	if ((ic_core_size == fw_core_size)
			&& (ic_cust_size == fw_cust_size)
			&& (ic_info.val.core_checksum == fw_info.val.core_checksum)
			&& (ic_info.val.custom_checksum == fw_info.val.custom_checksum)
			&& ((ic_regi_size != fw_regi_size)
				|| (ic_info.val.register_checksum != fw_info.val.register_checksum)))
		kind_of_download_method = NEED_PARTIAL_DL_REG;

	if ((ic_core_size == fw_core_size)
			&& (ic_info.val.core_checksum == fw_info.val.core_checksum)
			&& ((ic_cust_size != fw_cust_size)
				|| (ic_info.val.custom_checksum != fw_info.val.custom_checksum)))
		kind_of_download_method = NEED_PARTIAL_DL_CUSTOM;


	if (ic_info_size == 0 || ic_core_size == 0 ||
		ic_cust_size == 0 || ic_regi_size == 0 ||
		fw_info_size == 0 || fw_core_size == 0 ||
		fw_cust_size == 0 || fw_regi_size == 0 ||
		ic_info_size == 0xFFFFFFFF || ic_core_size == 0xFFFFFFFF ||
		ic_cust_size == 0xFFFFFFFF || ic_regi_size == 0xFFFFFFFF)
		kind_of_download_method = NEED_FULL_DL;

	if (kind_of_download_method != NEED_FULL_DL) {
		chk_info_checksum = 0;

		//info checksum.
		buff16[0x20 / 2] = 0;
		buff16[0x22 / 2] = 0;

		//Info checksum
		for (i = 0; i < 0x80 / 2; i++) {
			chk_info_checksum += buff16[i];
		}

		if (chk_info_checksum != ic_info.val.info_checksum) {
			kind_of_download_method = NEED_FULL_DL;
		}
	}
	//////////////////////////////////////////////////////////////////////
	return true;
}

static bool upgrade_fw_full_download(struct zt_ts_info *info, const u8 *firmware_data, u16 chip_code, u32 total_size)
{
	struct i2c_client *client = info->client;
	u32 flash_addr;
	int i;
	int nmemsz;
	int nrdsectorsize = 8;
#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7650)
	unsigned short int erase_info[2];
#endif
	nmemsz = fw_info_size + fw_core_size + fw_cust_size + fw_regi_size;
	if (nmemsz % nrdsectorsize > 0)
		nmemsz = (nmemsz / nrdsectorsize) * nrdsectorsize + nrdsectorsize;


	//[DEBUG]
	input_info(true, &client->dev, "%s: [UPGRADE] ENTER Firmware Upgrade(FULL)\n", __func__);

	if (write_reg(client, VCMD_NVM_INIT, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: power sequence error (nvm init)\n", __func__);
		return false;
	}

	zt_delay(5);

	input_info(true, &client->dev, "%s: init flash\n", __func__);

	if (write_reg(client, VCMD_NVM_WRITE_ENABLE, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to write nvm vpp on\n", __func__);
		return false;
	}
	zt_delay(10);

	if (write_cmd(client, VCMD_UPGRADE_INIT_FLASH) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to init flash\n", __func__);
		return false;
	}

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7650)
	input_info(true, &client->dev, "%s: [UPGRADE] Erase start\n", __func__);	//[DEBUG]
	// Mass Erase
	//====================================================
	if (write_reg(client, VCMD_UPGRADE_WRITE_MODE , 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to enter write mode\n", __func__);
		return false;
	}
	zt_delay(1);

	for (i = 0; i < 3; i++) {
		erase_info[0] = 0x0001;
		erase_info[1] = i; //Section num.
		write_data(client, VCMD_UPGRADE_BLOCK_ERASE , (u8 *)&erase_info[0] , 4);
		zt_delay(50);
	}

	for (i = 95; i < 126; i++) {
		erase_info[0] = 0x0000;
		erase_info[1] = i; //Page num.
		write_data(client, VCMD_UPGRADE_BLOCK_ERASE , (u8 *)&erase_info[0] , 4);
		zt_delay(50);
	}

	zt_delay(100);
	if (write_cmd(client, VCMD_UPGRADE_INIT_FLASH) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to init flash\n", __func__);
		return false;
	}

	input_info(true, &client->dev, "%s: [UPGRADE] Erase End.\n", __func__);	//[DEBUG]
	// Mass Erase End
	//====================================================
#else
	if (write_reg(client, VCMD_UPGRADE_WRITE_MODE , 0x0000) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to enter write mode\n", __func__);
		return false;
	}
#endif
	input_info(true, &client->dev, "%s: [UPGRADE] UPGRADE START.\n", __func__);	//[DEBUG]
	zt_delay(1);

	for (flash_addr = 0; flash_addr < nmemsz; ) {
		for (i = 0; i < TSP_PAGE_SIZE/TC_SECTOR_SZ; i++) {
			if (write_data(client,
						VCMD_UPGRADE_WRITE_FLASH,
						(u8 *)&firmware_data[flash_addr],TC_SECTOR_SZ) < 0) {
				input_err(true, &client->dev, "%s: error: write zinitix tc firmware\n", __func__);
				return false;
			}
			flash_addr += TC_SECTOR_SZ;
			usleep_range(100, 100);
		}

		usleep_range(FUZING_UDELAY, FUZING_UDELAY); /*for fuzing delay*/
	}

	input_err(true, &client->dev, "%s: [UPGRADE] UPGRADE END. VERIFY START.\n", __func__);	//[DEBUG]

	//VERIFY
	zt_power_control(info, POWER_OFF);

	if (zt_power_control(info, POWER_ON_SEQUENCE) == true) {
		zt_delay(10);
		input_info(true, &client->dev, "%s: upgrade finished\n", __func__);
		return true;
	}

	input_info(true, &client->dev, "%s: [UPGRADE] VERIFY FAIL.\n", __func__);	//[DEBUG]

	return false;

}

static bool upgrade_fw_partial_download(struct zt_ts_info *info, const u8 *firmware_data, u16 chip_code, u32 total_size)
{
	struct i2c_client *client = info->client;

	int i;
	int nrdsectorsize;
	int por_off_delay, por_on_delay;
	int idx, nmemsz;
	u32 erase_start_page_num;
#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7650)
	unsigned short int erase_info[2];
#endif
	int nsectorsize = 8;

	nrdsectorsize = 8;
	por_off_delay = USB_POR_OFF_DELAY;
	por_on_delay = USB_POR_ON_DELAY;

	nmemsz = fw_info_size + fw_core_size + fw_cust_size + fw_regi_size;
	if (nmemsz % nrdsectorsize > 0)
		nmemsz = (nmemsz / nrdsectorsize) * nrdsectorsize + nrdsectorsize;

	erase_start_page_num = (fw_info_size + fw_core_size) / TSP_PAGE_SIZE;

	if (write_reg(client, VCMD_NVM_INIT, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: power sequence error (nvm init)\n", __func__);
		return false;
	}

	zt_delay(5);
	input_err(true, &client->dev, "%s: init flash\n", __func__);

	if (write_reg(client, VCMD_NVM_WRITE_ENABLE, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to write nvm vpp on\n", __func__);
		return false;
	}
	zt_delay(10);

	if (write_cmd(client, VCMD_UPGRADE_INIT_FLASH) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to init flash\n", __func__);
		return false;
	}

	/* Erase Area */
#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7650)
	if (write_reg(client, VCMD_UPGRADE_WRITE_MODE , 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to enter write mode\n", __func__);
		return false;

	}
	zt_delay(1);
	erase_info[0] = 0x0000;
	erase_info[1] = 0x0000;	/* Page num. */
	write_data(client, VCMD_UPGRADE_BLOCK_ERASE , (u8 *)&erase_info[0] , 4);

	for (i = erase_start_page_num; i < nmemsz/TSP_PAGE_SIZE; i++) {
		zt_delay(50);
		erase_info[0] = 0x0000;
		erase_info[1] = i;	/* Page num.*/
		write_data(client, VCMD_UPGRADE_BLOCK_ERASE , (u8 *)&erase_info[0] , 4);
	}
	zt_delay(50);

	if (write_cmd(client, VCMD_UPGRADE_INIT_FLASH) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to init flash\n", __func__);
		return false;
	}
	zt_delay(1);

	if (write_reg(client, VCMD_UPGRADE_START_PAGE , 0x00) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to start addr. (erase end)\n", __func__);
		return false;
	}
	zt_delay(5);
#else
	if (write_reg(client, VCMD_UPGRADE_WRITE_MODE , 0x0000) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to enter write mode\n", __func__);
		return false;

	}
	zt_delay(1);
#endif

	for (i = 0; i < fw_info_size; ) {
		for (idx = 0; idx < TSP_PAGE_SIZE / nsectorsize; idx++) {
			if (write_data(client, VCMD_UPGRADE_WRITE_FLASH, (char *)&firmware_data[i], nsectorsize) != 0) {
				input_err(true, &client->dev, "%s: failed to write flash\n", __func__);
				return false;
			}
			i += nsectorsize;
			usleep_range(100, 100);
		}

		usleep_range(FUZING_UDELAY, FUZING_UDELAY); /*for fuzing delay*/
	}

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7650)
	if (write_reg(client, VCMD_UPGRADE_START_PAGE , erase_start_page_num) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to start addr. (erase end)\n", __func__);
		return false;

	}
	zt_delay(5);
#endif

	for (i = TSP_PAGE_SIZE * erase_start_page_num; i < nmemsz; ) {
		for (idx = 0; idx < TSP_PAGE_SIZE  / nsectorsize; idx++) {	//npagesize = 1024 // nsectorsize : 8
			if (write_data(client, VCMD_UPGRADE_WRITE_FLASH, (char *)&firmware_data[i], nsectorsize) != 0) {
				input_err(true, &client->dev, "%s: failed to write flash\n", __func__);
				return false;
			}
			i += nsectorsize;
			usleep_range(100, 100);
		}

		usleep_range(FUZING_UDELAY, FUZING_UDELAY); /*for fuzing delay*/
	}

	input_info(true, &client->dev, "%s: [UPGRADE] PARTIAL UPGRADE END. VERIFY START.\n", __func__);

	//VERIFY
	zt_power_control(info, POWER_OFF);

	if (zt_power_control(info, POWER_ON_SEQUENCE) == true) {
		zt_delay(10);
		input_info(true, &client->dev, "%s: upgrade finished\n", __func__);
		return true;
	}

	input_info(true, &client->dev, "%s: [UPGRADE] VERIFY FAIL.\n", __func__);

	return false;
}

static u8 ts_upgrade_firmware(struct zt_ts_info *info,
		const u8 *firmware_data)
{
	struct i2c_client *client = info->client;
	u32 flash_addr;
	int retry_cnt = 0;
	u16 chip_code;
	bool ret = false;
	u32 size = 0;

retry_upgrade:
	zt_power_control(info, POWER_OFF);
	zt_power_control(info, POWER_ON);
	zt_delay(10);

	if (write_reg(client, VCMD_ENABLE, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: power sequence error (vendor cmd enable)\n", __func__);
		goto fail_upgrade;
	}
	zt_delay(10);

	if (write_cmd(client, VCMD_UPGRADE_MODE) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: upgrade mode error\n", __func__);
		goto fail_upgrade;
	}
	zt_delay(5);

	if (write_reg(client, VCMD_ENABLE, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: power sequence error (vendor cmd enable)\n", __func__);
		goto fail_upgrade;
	}
	zt_delay(1);

	if (write_cmd(client, VCMD_INTN_CLR) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: vcmd int clr error\n", __func__);
		goto fail_upgrade;
	}

	if (read_data(client, VCMD_ID, (u8 *)&chip_code, 2) < 0) {
		input_err(true, &client->dev, "%s: failed to read chip code\n", __func__);
		goto fail_upgrade;
	}

	zt_delay(5);

	input_info(true, &client->dev, "chip code = 0x%x\n", chip_code);

	flash_addr = ((firmware_data[0x59]<<16) | (firmware_data[0x5A]<<8) | firmware_data[0x5B]); //Info
	flash_addr += ((firmware_data[0x5D]<<16) | (firmware_data[0x5E]<<8) | firmware_data[0x5F]); //Core
	flash_addr += ((firmware_data[0x61]<<16) | (firmware_data[0x62]<<8) | firmware_data[0x63]); //Custom
	flash_addr += ((firmware_data[0x65]<<16) | (firmware_data[0x66]<<8) | firmware_data[0x67]); //Register

	info->cap_info.ic_fw_size = ((firmware_data[0x69]<<16) | (firmware_data[0x6A]<<8) | firmware_data[0x6B]); //total size

	input_info(true, &client->dev, "f/w ic_fw_size = %d\n", info->cap_info.ic_fw_size);

	if (flash_addr != 0)
		size = flash_addr;
	else
		size = info->cap_info.ic_fw_size;

	input_info(true, &client->dev, "f/w size = 0x%x Page_sz = %d\n", size, TSP_PAGE_SIZE);
	usleep_range(10, 10);

	/////////////////////////////////////////////////////////////////////////////////////////
	ret = check_upgrade_method(info, firmware_data, chip_code);
	if (ret == false) {
		input_err(true, &client->dev, "%s: check upgrade method error\n", __func__);
		goto fail_upgrade;
	}
	/////////////////////////////////////////////////////////////////////////////////////////

	if (kind_of_download_method == NEED_FULL_DL)
		ret = upgrade_fw_full_download(info, firmware_data, chip_code, size);
	else
		ret = upgrade_fw_partial_download(info, firmware_data, chip_code, size);

	if (ret == true)
		return true;

fail_upgrade:
	zt_power_control(info, POWER_OFF);

	if (retry_cnt++ < INIT_RETRY_CNT) {
		input_err(true, &client->dev, "upgrade failed: so retry... (%d)\n", retry_cnt);
		goto retry_upgrade;
	}

	input_info(true, &client->dev, "%s: Failed to upgrade\n", __func__);

	return false;
}

static bool ts_hw_calibration(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	u16	chip_eeprom_info;
	int time_out = 0;

	input_info(true, &client->dev, "%s start\n", __func__);

	if (write_reg(client, ZT_TOUCH_MODE, 0x07) != I2C_SUCCESS)
		return false;

	zt_delay(10);
	write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
	zt_delay(10);
	write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
	zt_delay(50);
	write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
	zt_delay(10);

	if (write_cmd(client, ZT_CALIBRATE_CMD) != I2C_SUCCESS)
		return false;

	if (write_cmd(client, ZT_CLEAR_INT_STATUS_CMD) != I2C_SUCCESS)
		return false;

	zt_delay(10);
	write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);

	/* wait for h/w calibration*/
	do {
		zt_delay(200);
		write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);

		if (read_data(client, ZT_EEPROM_INFO, (u8 *)&chip_eeprom_info, 2) < 0)
			return false;

		input_dbg(true, &client->dev, "touch eeprom info = 0x%04X\r\n",
				chip_eeprom_info);

		if (!zinitix_bit_test(chip_eeprom_info, 0))
			break;

		if (time_out == 4) {
			write_cmd(client, ZT_CALIBRATE_CMD);
			zt_delay(10);
			write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
			input_err(true, &client->dev, "%s: h/w calibration retry timeout.\n", __func__);
		}

		if (time_out++ > 10) {
			input_err(true, &client->dev, "%s: h/w calibration timeout.\n", __func__);
			write_reg(client, ZT_TOUCH_MODE, TOUCH_POINT_MODE);
			zt_delay(50);
			write_cmd(client, ZT_SWRESET_CMD);
			return false;
		}
	} while (1);

	write_reg(client, VCMD_NVM_WRITE_ENABLE, 0x0001);
	usleep_range(100, 100);
	if (write_cmd(client, ZT_SAVE_CALIBRATION_CMD) != I2C_SUCCESS)
		return false;

	zt_delay(1100);
	write_reg(client, VCMD_NVM_WRITE_ENABLE, 0x0000);

	if (write_reg(client, ZT_TOUCH_MODE, TOUCH_POINT_MODE) != I2C_SUCCESS)
		return false;

	zt_delay(50);
	if (write_cmd(client, ZT_SWRESET_CMD) != I2C_SUCCESS)
		return false;

	return true;
}

static int ic_version_check(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	struct capa_info *cap = &(info->cap_info);
	int ret;
	u8 data[8] = {0};

	/* get chip information */
	ret = read_data(client, ZT_VENDOR_ID, (u8 *)&cap->vendor_id, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,"%s: fail vendor id\n", __func__);
		goto error;
	}

	ret = read_data(client, ZT_MINOR_FW_VERSION, (u8 *)&cap->fw_minor_version, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,"%s: fail fw_minor_version\n", __func__);
		goto error;
	}

	ret = read_data(client, ZT_CHIP_REVISION, data, 8);
	if (ret < 0) {
		input_err(true, &info->client->dev,"%s: fail chip_revision\n", __func__);
		goto error;
	}

	cap->ic_revision = data[0] | (data[1] << 8);
	cap->fw_version = data[2] | (data[3] << 8);
	cap->reg_data_version = data[4] | (data[5] << 8);
	cap->hw_id = data[6] | (data[7] << 8);

error:
	return ret;
}

static int fw_update_work(struct zt_ts_info *info, bool force_update)
{
	struct zt_ts_platform_data *pdata = info->pdata;
	struct capa_info *cap = &(info->cap_info);
	int ret;
	bool need_update = false;
	const struct firmware *tsp_fw = NULL;
	char fw_path[MAX_FW_PATH];
	u16 chip_eeprom_info;
#ifdef TCLM_CONCEPT
	int restore_cal = 0;
#endif
	if (pdata->bringup == 1) {
		input_info(true, &info->client->dev, "%s: bringup 1 skip update\n", __func__);
		return 0;
	}

	snprintf(fw_path, MAX_FW_PATH, "%s", pdata->firmware_name);
	input_info(true, &info->client->dev,
			"%s: start\n", __func__);

	ret = request_firmware(&tsp_fw, fw_path, &(info->client->dev));
	if (ret < 0) {
		input_info(true, &info->client->dev,
				"%s: Firmware image %s not available\n", __func__, fw_path);
		goto fw_request_fail;
	}
	else
		info->fw_data = (unsigned char *)tsp_fw->data;

	need_update = ts_check_need_upgrade(info, cap->fw_version,
			cap->fw_minor_version, cap->reg_data_version, cap->hw_id);
	if (!need_update) {
		if (!crc_check(info))
			need_update = true;
	}

	if (need_update == true || force_update == true) {
		ret = ts_upgrade_firmware(info, info->fw_data);
		if (!ret)
			input_err(true, &info->client->dev, "%s: failed fw update\n", __func__);

#ifdef TCLM_CONCEPT
		ret = sec_tclm_get_nvm_all(info->tdata);
		if (ret < 0) {
			input_info(true, &info->client->dev, "%s: sec_tclm_get_nvm_all error \n", __func__);
		}
		input_info(true, &info->client->dev, "%s: tune_fix_ver [%04X] afe_base [%04X]\n",
				__func__, info->tdata->nvdata.tune_fix_ver, info->tdata->afe_base);

		if (((info->tdata->nvdata.tune_fix_ver == 0xffff)||(info->tdata->afe_base > info->tdata->nvdata.tune_fix_ver))
				&& (info->tdata->tclm_level > TCLM_LEVEL_CLEAR_NV)) {
			/* tune version up case */
			sec_tclm_root_of_cal(info->tdata, CALPOSITION_TUNEUP);
			restore_cal = 1;
		} else if (info->tdata->tclm_level == TCLM_LEVEL_CLEAR_NV) {
			/* firmup case */
			sec_tclm_root_of_cal(info->tdata, CALPOSITION_FIRMUP);
			restore_cal = 1;
		}

		if (restore_cal == 1) {
			input_err(true, &info->client->dev, "%s: RUN OFFSET CALIBRATION\n", __func__);
			ret = sec_execute_tclm_package(info->tdata, 0);
			if (ret < 0) {
				input_err(true, &info->client->dev, "%s: sec_execute_tclm_package fail\n", __func__);
				goto fw_request_fail;
			}
		}

		sec_tclm_root_of_cal(info->tdata, CALPOSITION_NONE);
#endif

		ret = ic_version_check(info);
		if (ret < 0)
			input_err(true, &info->client->dev, "%s: failed ic version check\n", __func__);
	}

	if (read_data(info->client, ZT_EEPROM_INFO,
				(u8 *)&chip_eeprom_info, 2) < 0) {
		ret = -1;
		goto fw_request_fail;
	}

#ifndef TCLM_CONCEPT
	if (zinitix_bit_test(chip_eeprom_info, 0)) { /* hw calibration bit*/
		if (ts_hw_calibration(info) == false) {
			ret = -1;
			goto fw_request_fail;
		}
	}
#endif

fw_request_fail:
	if (tsp_fw)
		release_firmware(tsp_fw);
	return ret;
}

static bool init_touch(struct zt_ts_info *info)
{
	struct zt_ts_platform_data *pdata = info->pdata;
	u16 reg_val = 0;
	u8 data[6] = {0};

	/* get x,y data */
	read_data(info->client, ZT_TOTAL_NUMBER_OF_Y, data, 4);
	info->cap_info.x_node_num = data[2] | (data[3] << 8);
	info->cap_info.y_node_num = data[0] | (data[1] << 8);

	info->cap_info.MaxX= pdata->x_resolution;
	info->cap_info.MaxY = pdata->y_resolution;

	info->cap_info.total_node_num = info->cap_info.x_node_num * info->cap_info.y_node_num;
	info->cap_info.multi_fingers = MAX_SUPPORTED_FINGER_NUM;

	input_info(true, &info->client->dev, "node x %d, y %d  resolution x %d, y %d\n",
			info->cap_info.x_node_num, info->cap_info.y_node_num, info->cap_info.MaxX, info->cap_info.MaxY	);

#if ESD_TIMER_INTERVAL
	if (write_reg(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL,
				SCAN_RATE_HZ * ESD_TIMER_INTERVAL) != I2C_SUCCESS)
		goto fail_init;

	read_data(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL, (u8 *)&reg_val, 2);
#if defined(TSP_VERBOSE_DEBUG)
	input_info(true, &info->client->dev, "Esd timer register = %d\n", reg_val);
#endif
#endif
	if (!mini_init_touch(info))
		goto fail_init;

	return true;
fail_init:
	return false;
}

static int zt_set_fod_rect(struct zt_ts_info *info)
{
	int i, ret;
	u8 data[8];
	u32 sum = 0;

	for (i = 0; i < 4; i++) {
		data[i * 2] = info->fod_rect[i] & 0xFF;
		data[i * 2 + 1] = (info->fod_rect[i] >> 8) & 0xFF;
		sum += info->fod_rect[i];
	}

	if (!sum) /* no data */
		return 0;

	input_info(true, &info->client->dev, "%s: %u,%u,%u,%u\n",
			__func__, info->fod_rect[0], info->fod_rect[1],
			info->fod_rect[2], info->fod_rect[3]);

	ret = ts_write_to_sponge(info, ZT_SPONGE_FOD_RECT, data, sizeof(data));
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);

	return ret;
}

static int zt_set_aod_rect(struct zt_ts_info *info)
{
	u8 data[8] = {0};
	int i;
	int ret;

	for (i = 0; i < 4; i++) {
		data[i * 2] = info->aod_rect[i] & 0xFF;
		data[i * 2 + 1] = (info->aod_rect[i] >> 8) & 0xFF;
	}

	ret = ts_write_to_sponge(info, ZT_SPONGE_TOUCHBOX_W_OFFSET, data, sizeof(data));
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: fail set custom lib \n", __func__);

	return ret;
}

static bool mini_init_touch(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	int i;

	if (write_cmd(client, ZT_SWRESET_CMD) != I2C_SUCCESS) {
		input_info(true, &client->dev, "%s: Failed to write reset command\n", __func__);
		goto fail_mini_init;
	}

	if (write_reg(client, ZT_TOUCH_MODE, info->touch_mode) != I2C_SUCCESS)
		goto fail_mini_init;

	/* cover_set */
	if (write_reg(client, ZT_COVER_CONTROL_REG, COVER_OPEN) != I2C_SUCCESS)
		goto fail_mini_init;

	if (info->flip_enable)
		set_cover_type(info, info->flip_enable);

	if (write_reg(client, ZT_OPTIONAL_SETTING, info->m_optional_mode.optional_mode) != I2C_SUCCESS)
		goto fail_mini_init;

	/* read garbage data */
	for (i = 0; i < 10; i++) {
		write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
		usleep_range(10, 10);
	}

#if ESD_TIMER_INTERVAL
	if (write_reg(client, ZT_PERIODICAL_INTERRUPT_INTERVAL,
				SCAN_RATE_HZ * ESD_TIMER_INTERVAL) != I2C_SUCCESS)
		goto fail_mini_init;

	esd_timer_start(CHECK_ESD_TIMER, info);
#if defined(TSP_VERBOSE_DEBUG)
	input_info(true, &client->dev, "%s: Started esd timer\n", __func__);
#endif
#endif

	ts_write_to_sponge(info, ZT_SPONGE_LP_FEATURE, &info->lpm_mode, 2);
	zt_set_fod_rect(info);

	if (info->sleep_mode) {
#if ESD_TIMER_INTERVAL
		esd_timer_stop(info);
#endif
		write_cmd(info->client, ZT_SLEEP_CMD);
		input_info(true, &info->client->dev, "%s, sleep mode\n", __func__);

		zt_set_aod_rect(info);
	} else {
		zt_set_grip_type(info, ONLY_EDGE_HANDLER);
	}

	input_info(true, &client->dev, "%s: Successfully mini initialized\r\n", __func__);
	return true;

fail_mini_init:
	input_err(true, &client->dev, "%s: Failed to initialize mini init\n", __func__);
	return false;
}

static void clear_report_data(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	int i;
	u8 reported = 0;
	char location[7] = "";

	if (info->prox_power_off) {
		input_report_key(info->input_dev, KEY_INT_CANCEL, 1);
		input_sync(info->input_dev);
		input_report_key(info->input_dev, KEY_INT_CANCEL, 0);
		input_sync(info->input_dev);
	}

	info->prox_power_off = 0;

	for (i = 0; i < info->cap_info.multi_fingers; i++) {
		if (info->cur_coord[i].touch_status > FINGER_NONE) {
			input_mt_slot(info->input_dev, i);
#ifdef CONFIG_SEC_FACTORY
			input_report_abs(info->input_dev, ABS_MT_PRESSURE, 0);
#endif
			input_report_abs(info->input_dev, ABS_MT_CUSTOM, 0);
			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 0);
			reported = true;
			if (!m_ts_debug_mode && TSP_NORMAL_EVENT_MSG) {
				location_detect(info, location, info->cur_coord[i].x, info->cur_coord[i].y);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				input_info(true, &client->dev, "[RA] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d p:%d\n",
						i, location,
						info->cur_coord[i].x - info->pressed_x[i],
						info->cur_coord[i].y - info->pressed_y[i],
						info->move_count[i], info->finger_cnt1,
						info->cur_coord[i].x,
						info->cur_coord[i].y,
						info->cur_coord[i].palm_count);
#else
				input_info(true, &client->dev, "[RA] tID:%02d loc:%s dd:%d,%d mc:%d tc:%d p:%d\n",
						i, location,
						info->cur_coord[i].x - info->pressed_x[i],
						info->cur_coord[i].y - info->pressed_y[i],
						info->move_count[i], info->finger_cnt1,
						info->cur_coord[i].palm_count);
#endif
			}
		}
		memset(&info->old_coord[i], 0, sizeof(struct ts_coordinate));
		memset(&info->cur_coord[i], 0, sizeof(struct ts_coordinate));
		info->move_count[i] = 0;
	}

	info->glove_touch = 0;

	if (reported)
		input_sync(info->input_dev);

	info->finger_cnt1 = 0;
	info->check_multi = 0;
}

#define	PALM_REPORT_WIDTH	200
#define	PALM_REJECT_WIDTH	255

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
void trustedui_mode_on(void) {
	input_info(true, &tui_tsp_info->client->dev, "%s, release all finger..", __func__);
	clear_report_data(tui_tsp_info);
	input_info(true, &tui_tsp_info->client->dev, "%s : esd timer disable", __func__);
	zt_ts_esd_timer_stop(tui_tsp_info);

}
EXPORT_SYMBOL(trustedui_mode_on);

void trustedui_mode_off(void) {
	input_info(true, &tui_tsp_info->client->dev, "%s : esd timer enable", __func__);
	zt_ts_esd_timer_start(tui_tsp_info);

}
EXPORT_SYMBOL(trustedui_mode_off);
#endif

void location_detect(struct zt_ts_info *info, char *loc, int x, int y)
{
	memset(loc, 0x00, 7);
	strncpy(loc, "xy:", 3);
	if (x < info->pdata->area_edge)
		strncat(loc, "E.", 2);
	else if (x < (info->pdata->x_resolution - info->pdata->area_edge))
		strncat(loc, "C.", 2);
	else
		strncat(loc, "e.", 2);
	if (y < info->pdata->area_indicator)
		strncat(loc, "S", 1);
	else if (y < (info->pdata->y_resolution - info->pdata->area_navigation))
		strncat(loc, "C", 1);
	else
		strncat(loc, "N", 1);
}

static irqreturn_t zt_touch_work(int irq, void *data)
{
	struct zt_ts_info* info = (struct zt_ts_info*)data;
	struct i2c_client *client = info->client;
	int i;
	u8 reported = false;
	u8 tid = 0;
	u8 ttype, tstatus;
	u16 x, y, z, maxX, maxY, sen_max;
	u16 st;
	u16 prox_data = 0;
	u8 info_major_w = 0;
	u8 info_minor_w = 0;
	char location[7] = "";
	int ret;
	char pos[5];
	char cur = 0;
	char old = 0;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (IRQ_HANDLED == secure_filter_interrupt(info)) {
		wait_for_completion_interruptible_timeout(&info->secure_interrupt,
				msecs_to_jiffies(5 * MSEC_PER_SEC));

		input_info(true, &client->dev,
				"%s: secure interrupt handled\n", __func__);

		return IRQ_HANDLED;
	}
#endif

	if (info->sleep_mode) {
		pm_wakeup_event(info->input_dev->dev.parent, 500);

		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(&info->resume_done, msecs_to_jiffies(500));
		if (ret == 0) {
			input_err(true, &info->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			return IRQ_HANDLED;
		} else if (ret < 0) {
			input_err(true, &info->client->dev, "%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			return IRQ_HANDLED;
		}
	}

	if (gpio_get_value(info->pdata->gpio_int)) {
		input_err(true, &client->dev, "%s: Invalid interrupt\n", __func__);

		return IRQ_HANDLED;
	}

	if (!mutex_trylock(&info->work_lock)) {
		input_err(true, &client->dev, "%s: Failed to occupy work lock\n", __func__);
		write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
		clear_report_data(info);
		return IRQ_HANDLED;
	}
#if ESD_TIMER_INTERVAL
	esd_timer_stop(info);
#endif

	if (info->work_state != NOTHING) {
		input_err(true, &client->dev, "%s: Other process occupied\n", __func__);
		usleep_range(DELAY_FOR_SIGNAL_DELAY, DELAY_FOR_SIGNAL_DELAY);

		if (!gpio_get_value(info->pdata->gpio_int)) {
			write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
			clear_report_data(info);
			usleep_range(DELAY_FOR_SIGNAL_DELAY, DELAY_FOR_SIGNAL_DELAY);
		}

		goto out;
	}

	if (ts_read_coord(info) == false) { /* maybe desirable reset */
		input_err(true, &client->dev, "%s: Failed to read info coord\n", __func__);
		goto out;
	}

	info->work_state = NORMAL;
	reported = false;

	if (info->touch_info[0].byte00.value.eid == CUSTOM_EVENT || info->touch_info[0].byte00.value.eid == GESTURE_EVENT)
		goto out;

	for (i = 0; i < info->cap_info.multi_fingers; i++) {
		info->old_coord[i] = info->cur_coord[i];
		memset(&info->cur_coord[i], 0, sizeof(struct ts_coordinate));
	}

	for (i = 0; i < info->cap_info.multi_fingers; i++) {
		ttype = (info->touch_info[i].byte06.value.touch_type23 << 2) | (info->touch_info[i].byte07.value.touch_type01);
		tstatus = info->touch_info[i].byte00.value.touch_status;

		if (tstatus == FINGER_NONE && ttype != TOUCH_PROXIMITY)
			continue;

		tid = info->touch_info[i].byte00.value.tid;

		info->cur_coord[tid].id = tid;
		info->cur_coord[tid].touch_status = tstatus;
		info->cur_coord[tid].x = (info->touch_info[i].byte01.value.x_coord_h << 4) | (info->touch_info[i].byte03.value.x_coord_l);
		info->cur_coord[tid].y = (info->touch_info[i].byte02.value.y_coord_h << 4) | (info->touch_info[i].byte03.value.y_coord_l);
		info->cur_coord[tid].z = info->touch_info[i].byte06.value.z_value;
		info->cur_coord[tid].ttype = ttype;
		info->cur_coord[tid].major = info->touch_info[i].byte04.value_u8bit;
		info->cur_coord[tid].minor = info->touch_info[i].byte05.value_u8bit;
		info->cur_coord[tid].noise = info->touch_info[i].byte08.value_u8bit;
		info->cur_coord[tid].max_sense= info->touch_info[i].byte09.value_u8bit;

		if (!info->cur_coord[tid].palm && (info->cur_coord[tid].ttype == TOUCH_PALM))
			info->cur_coord[tid].palm_count++;
		info->cur_coord[tid].palm = (info->cur_coord[tid].ttype == TOUCH_PALM);

		if (info->cur_coord[tid].z <= 0)
			info->cur_coord[tid].z = 1;
	}

	for (i = 0; i < info->cap_info.multi_fingers; i++) {
		if ((info->cur_coord[i].ttype == TOUCH_PROXIMITY)
				&& (info->pdata->support_ear_detect)) {
			if (read_data(info->client, ZT_PROXIMITY_DETECT, (u8 *)&prox_data, 2) < 0)
				input_err(true, &client->dev, "%s: fail to read proximity detect reg\n", __func__);

			info->hover_event = prox_data;

			input_info(true, &client->dev, "PROXIMITY DETECT. LVL = %d \n", prox_data);
			input_report_abs(info->input_dev_proximity, ABS_MT_CUSTOM, prox_data);
			input_sync(info->input_dev_proximity);
			break;
		}
	}

	info->noise_flag = -1;
	info->flip_cover_flag = 0;

	for (i = 0; i < info->cap_info.multi_fingers; i++) {
		if (info->cur_coord[i].touch_status == FINGER_NONE)
			continue;

		if ((info->noise_flag == -1) && (info->old_coord[i].noise != info->cur_coord[i].noise)) {
			info->noise_flag = info->cur_coord[i].noise;
			input_info(true, &client->dev, "NOISE MODE %s [%d]\n", info->noise_flag > 0 ? "ON":"OFF", info->noise_flag);
		}

		if (info->flip_cover_flag == 0) {
			if (info->old_coord[i].ttype != TOUCH_FLIP_COVER && info->cur_coord[i].ttype == TOUCH_FLIP_COVER) {
				info->flip_cover_flag = 1;
				input_info(true, &client->dev, "%s: FLIP COVER MODE ON\n", __func__);
			}

			if (info->old_coord[i].ttype == TOUCH_FLIP_COVER && info->cur_coord[i].ttype != TOUCH_FLIP_COVER) {
				info->flip_cover_flag = 1;
				input_info(true, &client->dev, "%s: FLIP COVER MODE OFF\n", __func__);
			}
		}

		if (info->old_coord[i].ttype != info->cur_coord[i].ttype) {
			if (info->cur_coord[i].touch_status == FINGER_PRESS)
				snprintf(pos, 5, "P");
			else if (info->cur_coord[i].touch_status == FINGER_MOVE)
				snprintf(pos, 5, "M");
			else
				snprintf(pos, 5, "R");

			if (info->cur_coord[i].ttype == TOUCH_PALM)
				cur = 'P';
			else if (info->cur_coord[i].ttype == TOUCH_GLOVE)
				cur = 'G';
			else
				cur = 'N';

			if (info->old_coord[i].ttype == TOUCH_PALM)
				old = 'P';
			else if (info->old_coord[i].ttype == TOUCH_GLOVE)
				old = 'G';
			else
				old = 'N';

			if (cur != old)
				input_info(true, &client->dev, "tID:%d ttype(%c->%c) : %s\n", i, old, cur, pos);
		}

		if ((info->cur_coord[i].touch_status == FINGER_PRESS || info->cur_coord[i].touch_status == FINGER_MOVE)) {
			x = info->cur_coord[i].x;
			y = info->cur_coord[i].y;
			z = info->cur_coord[i].z;
			info_major_w = info->cur_coord[i].major;
			info_minor_w = info->cur_coord[i].minor;
			sen_max = info->cur_coord[i].max_sense;

			maxX = info->cap_info.MaxX;
			maxY = info->cap_info.MaxY;

			if (x > maxX || y > maxY) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				input_err(true, &client->dev,
						"Invalid coord %d : x=%d, y=%d\n", i, x, y);
#endif
				continue;
			}

			st = sen_max & 0x0F;
			if (st < 1)
				st = 1;

			input_mt_slot(info->input_dev, i);
			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 1);

			input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR, (u32)info_major_w);

#ifdef CONFIG_SEC_FACTORY
			input_report_abs(info->input_dev, ABS_MT_PRESSURE, (u32)st);
#endif
			input_report_abs(info->input_dev, ABS_MT_WIDTH_MAJOR, (u32)info_major_w);
			input_report_abs(info->input_dev, ABS_MT_TOUCH_MINOR, info_minor_w);

			input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(info->input_dev, ABS_MT_CUSTOM, info->cur_coord[i].palm);

			input_report_key(info->input_dev, BTN_TOUCH, 1);

			if ((info->cur_coord[i].touch_status == FINGER_PRESS) && (info->cur_coord[i].touch_status != info->old_coord[i].touch_status)) {
				info->pressed_x[i] = x; /*for getting coordinates of pressed point*/
				info->pressed_y[i] = y;
				info->finger_cnt1++;

				if ((info->finger_cnt1 > 4) && (info->check_multi == 0)) {
					info->check_multi = 1;
					info->multi_count++;
					input_info(true, &client->dev,"data : pn=%d mc=%d \n", info->finger_cnt1, info->multi_count);
				}

				location_detect(info, location, x, y);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				input_info(true, &client->dev, "[P] tID:%d,%d x:%d y:%d z:%d(st:%d) max:%d major:%d minor:%d loc:%s tc:%d touch_type:%x noise:%x\n",
						i, (info->input_dev->mt->trkid - 1) & TRKID_MAX, x, y, z, st, sen_max, info_major_w,
						info_minor_w, location, info->finger_cnt1, info->cur_coord[i].ttype, info->cur_coord[i].noise);
#else
				input_info(true, &client->dev, "[P] tID:%d,%d z:%d(st:%d) max:%d major:%d minor:%d loc:%s tc:%d touch_type:%x noise:%x\n",
						i, (info->input_dev->mt->trkid - 1) & TRKID_MAX, z, st, sen_max, info_major_w,
						info_minor_w, location, info->finger_cnt1, info->cur_coord[i].ttype, info->cur_coord[i].noise);
#endif
			} else if (info->cur_coord[i].touch_status == FINGER_MOVE) {
				info->move_count[i]++;
			}
		} else if (info->cur_coord[i].touch_status == FINGER_RELEASE) {
			input_mt_slot(info->input_dev, i);
			input_report_abs(info->input_dev, ABS_MT_CUSTOM, 0);
			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 0);

			location_detect(info, location, info->cur_coord[i].x, info->cur_coord[i].y);

			if (info->finger_cnt1 > 0)
				info->finger_cnt1--;

			if (info->finger_cnt1 == 0) {
				input_report_key(info->input_dev, BTN_TOUCH, 0);
				info->check_multi = 0;
			}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &client->dev,
					"[R] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d p:%d\n",
					i, location,
					info->cur_coord[i].x - info->pressed_x[i],
					info->cur_coord[i].y - info->pressed_y[i],
					info->move_count[i], info->finger_cnt1,
					info->cur_coord[i].x,
					info->cur_coord[i].y,
					info->cur_coord[i].palm_count);
#else
			input_info(true, &client->dev,
					"[R] tID:%02d loc:%s dd:%d,%d mc:%d tc:%d p:%d\n",
					i, location,
					info->cur_coord[i].x - info->pressed_x[i],
					info->cur_coord[i].y - info->pressed_y[i],
					info->move_count[i], info->finger_cnt1,
					info->cur_coord[i].palm_count);
#endif

			info->move_count[i] = 0;
			memset(&info->cur_coord[i], 0, sizeof(struct ts_coordinate));
		}
	}

	input_sync(info->input_dev);

out:
	if (info->work_state == NORMAL) {
#if ESD_TIMER_INTERVAL
		esd_timer_start(CHECK_ESD_TIMER, info);
#endif
		info->work_state = NOTHING;
	}

	mutex_unlock(&info->work_lock);

	return IRQ_HANDLED;
}

#ifdef CONFIG_INPUT_ENABLED
static int  zt_ts_open(struct input_dev *dev)
{
	struct zt_ts_info *info = misc_info;
	int ret = 0;

	if (info == NULL)
		return 0;

	if (!info->info_work_done) {
		input_err(true, &info->client->dev, "%s not finished info work\n", __func__);
		return 0;
	}

	input_info(true, &info->client->dev, "%s, %d \n", __func__, __LINE__);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	zt_delay(100);
	if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
		tui_force_close(1);
		zt_delay(100);
		if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
			trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
			trustedui_set_mode(TRUSTEDUI_MODE_OFF);
		}
	}
#endif // CONFIG_TRUSTONIC_TRUSTED_UI
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	secure_touch_stop(info, 0);
#endif

	if (info->sleep_mode) {
		mutex_lock(&info->work_lock);
		info->work_state = SLEEP_MODE_OUT;
		info->sleep_mode = 0;
		input_info(true, &info->client->dev, "%s, wake up\n", __func__);

		write_cmd(info->client, ZT_WAKEUP_CMD);
		write_reg(info->client, ZT_OPTIONAL_SETTING, info->m_optional_mode.optional_mode);
		info->work_state = NOTHING;
		mutex_unlock(&info->work_lock);

#if ESD_TIMER_INTERVAL
		esd_timer_start(CHECK_ESD_TIMER, info);
#endif
		if (device_may_wakeup(&info->client->dev))
			disable_irq_wake(info->irq);
	} else {
		mutex_lock(&info->work_lock);
		if (info->work_state != RESUME
				&& info->work_state != EALRY_SUSPEND) {
			input_info(true, &info->client->dev, "invalid work proceedure (%d)\r\n",
					info->work_state);
			mutex_unlock(&info->work_lock);
			return 0;
		}

		ret = zt_power_control(info, POWER_ON_SEQUENCE);
		if (ret == false) {
			zt_power_control(info, POWER_OFF);
			zt_power_control(info, POWER_ON_SEQUENCE);
		}

		crc_check(info);

		if (mini_init_touch(info) == false)
			goto fail_late_resume;
		enable_irq(info->irq);
		info->work_state = NOTHING;

		if (g_ta_connected)
			zt_set_optional_mode(info, DEF_OPTIONAL_MODE_USB_DETECT_BIT, g_ta_connected);

		mutex_unlock(&info->work_lock);
		input_dbg(true, &info->client->dev, "%s--\n", __func__);
		return 0;

fail_late_resume:
		input_info(true, &info->client->dev, "%s: failed to late resume\n", __func__);
		enable_irq(info->irq);
		info->work_state = NOTHING;
		mutex_unlock(&info->work_lock);
	}

	cancel_delayed_work(&info->work_print_info);
	info->print_info_cnt_open = 0;
	info->print_info_cnt_release = 0;
	if (!shutdown_is_on_going_tsp)
		schedule_work(&info->work_print_info.work);

	return 0;
}

static void zt_ts_close(struct input_dev *dev)
{
	struct zt_ts_info *info = misc_info;
	int i;
	u8 prev_work_state;

	if (info == NULL)
		return;

	if (!info->info_work_done) {
		input_err(true, &info->client->dev, "%s not finished info work\n", __func__);
		return;
	}

	input_info(true, &info->client->dev,
			"%s, spay:%d aod:%d aot:%d singletap:%d prox:%ld pocket:%d ed:%d\n",
			__func__, info->spay_enable, info->aod_enable,
			info->aot_enable, info->singletap_enable, info->prox_power_off,
			info->pocket_enable, info->ed_enable);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	zt_delay(100);
	if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
		tui_force_close(1);
		zt_delay(100);
		if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
			trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
			trustedui_set_mode(TRUSTEDUI_MODE_OFF);
		}
	}
#endif // CONFIG_TRUSTONIC_TRUSTED_UI
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	secure_touch_stop(info, 1);
#endif

#ifdef TCLM_CONCEPT
	sec_tclm_debug_info(info->tdata);
#endif

#if ESD_TIMER_INTERVAL
	flush_work(&info->tmr_work);
#endif

	if (info->touch_mode == TOUCH_AGING_MODE) {
		ts_set_touchmode(TOUCH_POINT_MODE);
#if ESD_TIMER_INTERVAL
		write_reg(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL,
			SCAN_RATE_HZ * ESD_TIMER_INTERVAL);
#endif
		input_info(true, &info->client->dev, "%s, set touch mode\n", __func__);
	}

	if (((info->spay_enable || info->aod_enable || info->aot_enable || info->singletap_enable))
			|| info->pocket_enable || info->ed_enable || info->fod_enable || info->fod_lp_mode) {
		mutex_lock(&info->work_lock);
		prev_work_state = info->work_state;
		info->work_state = SLEEP_MODE_IN;
		input_info(true, &info->client->dev, "%s, sleep mode\n", __func__);

#if ESD_TIMER_INTERVAL
		esd_timer_stop(info);
#endif
		ts_set_utc_sponge(info);

		if (info->prox_power_off && info->aot_enable)
			zinitix_bit_clr(info->lpm_mode, ZT_SPONGE_MODE_DOUBLETAP_WAKEUP);

		input_info(true, &info->client->dev,
				"%s: write lpm_mode 0x%02x (spay:%d, aod:%d, singletap:%d, aot:%d, fod:%d, fod_lp:%d)\n",
				__func__, info->lpm_mode,
				(info->lpm_mode & (1 << ZT_SPONGE_MODE_SPAY)) ? 1 : 0,
				(info->lpm_mode & (1 << ZT_SPONGE_MODE_AOD)) ? 1 : 0,
				(info->lpm_mode & (1 << ZT_SPONGE_MODE_SINGLETAP)) ? 1 : 0,
				(info->lpm_mode & (1 << ZT_SPONGE_MODE_DOUBLETAP_WAKEUP)) ? 1 : 0,
				info->fod_enable, info->fod_lp_mode);

		ts_write_to_sponge(info, ZT_SPONGE_LP_FEATURE, &info->lpm_mode, 2);
		zt_set_fod_rect(info);

		write_cmd(info->client, ZT_SLEEP_CMD);
		info->sleep_mode = 1;

		if (info->aot_enable)
			zinitix_bit_set(info->lpm_mode, ZT_SPONGE_MODE_DOUBLETAP_WAKEUP);

		/* clear garbage data */
		for (i = 0; i < 2; i++) {
			zt_delay(10);
			write_cmd(info->client, ZT_CLEAR_INT_STATUS_CMD);
		}
		clear_report_data(info);

		info->work_state = prev_work_state;
		if (device_may_wakeup(&info->client->dev))
			enable_irq_wake(info->irq);
	} else {
		disable_irq(info->irq);
		mutex_lock(&info->work_lock);
		if (info->work_state != NOTHING) {
			input_info(true, &info->client->dev, "invalid work proceedure (%d)\r\n",
					info->work_state);
			mutex_unlock(&info->work_lock);
			enable_irq(info->irq);
			return;
		}
		info->work_state = EALRY_SUSPEND;

		clear_report_data(info);

#if ESD_TIMER_INTERVAL
		/*write_reg(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL, 0);*/
		esd_timer_stop(info);
#endif

		zt_power_control(info, POWER_OFF);
	}

	cancel_delayed_work(&info->work_print_info);
	zt_print_info(info);

	input_info(true, &info->client->dev, "%s --\n", __func__);
	mutex_unlock(&info->work_lock);
	return;
}
#endif	/* CONFIG_INPUT_ENABLED */

static int ts_set_touchmode(u16 value)
{
	int i, ret = 1;
	int retry_cnt = 0;
	struct capa_info *cap = &(misc_info->cap_info);

	disable_irq(misc_info->irq);

	mutex_lock(&misc_info->work_lock);
	if (misc_info->work_state != NOTHING) {
		input_info(true, &misc_info->client->dev, "other process occupied.. (%d)\n",
				misc_info->work_state);
		enable_irq(misc_info->irq);
		mutex_unlock(&misc_info->work_lock);
		return -1;
	}

retry_ts_set_touchmode:
	//wakeup cmd
	write_cmd(misc_info->client, 0x0A);
	usleep_range(20 * 1000, 20 * 1000);
	write_cmd(misc_info->client, 0x0A);
	usleep_range(20 * 1000, 20 * 1000);

	misc_info->work_state = SET_MODE;

	if (value == TOUCH_SEC_MODE)
		misc_info->touch_mode = TOUCH_POINT_MODE;
	else
		misc_info->touch_mode = value;

	input_info(true, &misc_info->client->dev, "[zinitix_touch] tsp_set_testmode %d\r\n", misc_info->touch_mode);

	if (!((misc_info->touch_mode == TOUCH_POINT_MODE) ||
				(misc_info->touch_mode == TOUCH_SENTIVITY_MEASUREMENT_MODE))) {
		if (write_reg(misc_info->client, ZT_DELAY_RAW_FOR_HOST,
					RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS)
			input_info(true, &misc_info->client->dev, "%s: Fail to set zt_DELAY_RAW_FOR_HOST.\r\n", __func__);
	}

	if (write_reg(misc_info->client, ZT_TOUCH_MODE,
				misc_info->touch_mode) != I2C_SUCCESS)
		input_info(true, &misc_info->client->dev, "[zinitix_touch] TEST Mode : "
				"Fail to set ZINITX_TOUCH_MODE %d.\r\n", misc_info->touch_mode);

	input_info(true, &misc_info->client->dev, "%s: tsp_set_testmode. write regiter end \n", __func__);

	ret = read_data(misc_info->client, ZT_TOUCH_MODE, (u8 *)&cap->current_touch_mode, 2);
	if (ret < 0) {
		input_err(true, &misc_info->client->dev,"%s: fail touch mode read\n", __func__);
		goto out;
	}

	if (cap->current_touch_mode != misc_info->touch_mode) {
		if (retry_cnt < 1) {
			retry_cnt++;
			goto retry_ts_set_touchmode;
		}
		input_info(true, &misc_info->client->dev, "%s: fail to set touch_mode %d (current_touch_mode %d).\n",
				__func__, misc_info->touch_mode, cap->current_touch_mode);
		ret = -1;
		goto out;
	}

	/* clear garbage data */
	for (i = 0; i < 10; i++) {
		zt_delay(20);
		write_cmd(misc_info->client, ZT_CLEAR_INT_STATUS_CMD);
	}

	clear_report_data(misc_info);

	input_info(true, &misc_info->client->dev, "%s: tsp_set_testmode. garbage data end \n", __func__);

out:
	misc_info->work_state = NOTHING;
	enable_irq(misc_info->irq);
	mutex_unlock(&misc_info->work_lock);
	return ret;
}

static int ts_upgrade_sequence(struct zt_ts_info *info, const u8 *firmware_data, int restore_cal)
{
	int ret = 0;
	disable_irq(info->irq);
	mutex_lock(&info->work_lock);
	info->work_state = UPGRADE;

#if ESD_TIMER_INTERVAL
	esd_timer_stop(info);
#endif
	clear_report_data(info);

	input_info(true, &info->client->dev, "%s: start upgrade firmware\n", __func__);
	if (ts_upgrade_firmware(info, firmware_data) == false) {
		ret = -1;
		goto out;
	}

	if (ic_version_check(info) < 0)
		input_err(true, &info->client->dev, "%s: failed ic version check\n", __func__);

#ifdef TCLM_CONCEPT
	if (restore_cal == 1) {
		input_err(true, &info->client->dev, "%s: RUN OFFSET CALIBRATION\n", __func__);
		ret = sec_execute_tclm_package(info->tdata, 0);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: sec_execute_tclm_package fail\n", __func__);
			ret = -EIO;
			goto out;
		}
	}
#else
	if (ts_hw_calibration(info) == false) {
		ret = -1;
		goto out;
	}
#endif

	if (mini_init_touch(info) == false) {
		ret = -1;
		goto out;

	}

#if ESD_TIMER_INTERVAL
	esd_timer_start(CHECK_ESD_TIMER, info);
#if defined(TSP_VERBOSE_DEBUG)
	input_info(true, &info->client->dev, "%s: Started esd timer\n", __func__);
#endif
#endif
out:
	enable_irq(info->irq);
	misc_info->work_state = NOTHING;
	mutex_unlock(&info->work_lock);
	return ret;
}

static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct zt_ts_platform_data *pdata = info->pdata;
	struct i2c_client *client = info->client;
	const u8 *buff = 0;
	mm_segment_t old_fs = {0};
	struct file *fp = NULL;
	long fsize = 0, nread = 0;
	char fw_path[MAX_FW_PATH+1];
	char result[16] = {0};
	const struct firmware *tsp_fw = NULL;
	unsigned char *fw_data = NULL;
	int restore_cal;
	int ret;
	u8 update_type = 0;

	sec_cmd_set_default_result(sec);

	switch (sec->cmd_param[0]) {
	case BUILT_IN:
		update_type = TSP_TYPE_BUILTIN_FW;
		break;
	case UMS:
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
		update_type = TSP_TYPE_EXTERNAL_FW_SIGNED;
#else
		update_type = TSP_TYPE_EXTERNAL_FW;
#endif
		break;
	case SPU:
		update_type = TSP_TYPE_SPU_FW_SIGNED;
		break;
	default:
		goto fw_update_out;
	}

	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	if (update_type == TSP_TYPE_BUILTIN_FW) {
		/* firmware update builtin binary */
		snprintf(fw_path, MAX_FW_PATH, "%s", pdata->firmware_name);

		ret = request_firmware(&tsp_fw, fw_path, &(client->dev));
		if (ret) {
			input_info(true, &client->dev,
					"%s: Firmware image %s not available\n", __func__,
					fw_path);
			if (tsp_fw)
				release_firmware(tsp_fw);

			goto fw_update_out;
		} else {
			fw_data = (unsigned char *)tsp_fw->data;
		}

#ifdef TCLM_CONCEPT
		sec_tclm_root_of_cal(info->tdata, CALPOSITION_TESTMODE);
		restore_cal = 1;
#endif
		ret = ts_upgrade_sequence(info, (u8*)fw_data, restore_cal);
		release_firmware(tsp_fw);
		if (ret < 0)
			goto fw_update_out;

		sec->cmd_state = SEC_CMD_STATUS_OK;
	} else {
		/* firmware update ums or spu */
		if (update_type == TSP_TYPE_EXTERNAL_FW)
			snprintf(fw_path, MAX_FW_PATH, TSP_PATH_EXTERNAL_FW);
		else if (update_type == TSP_TYPE_EXTERNAL_FW_SIGNED)
			snprintf(fw_path, MAX_FW_PATH, TSP_PATH_EXTERNAL_FW_SIGNED);
		else if (update_type == TSP_TYPE_SPU_FW_SIGNED)
			snprintf(fw_path, MAX_FW_PATH, TSP_PATH_SPU_FW_SIGNED);
		else
			goto fw_update_out;

		old_fs = get_fs();
		set_fs(get_ds());

		fp = filp_open(fw_path, O_RDONLY, 0);
		if (IS_ERR(fp)) {
			input_err(true, &client->dev, "file %s open error\n", fw_path);
			set_fs(old_fs);
			goto fw_update_out;
		}

		fsize = fp->f_path.dentry->d_inode->i_size;

		buff = vzalloc(fsize);
		if (!buff) {
			input_err(true, &client->dev, "%s: failed to alloc buffer for fw\n", __func__);
			filp_close(fp, NULL);
			set_fs(old_fs);
			goto fw_update_out;
		}

		nread = vfs_read(fp, (char __user *)buff, fsize, &fp->f_pos);
		if (nread != fsize) {
			filp_close(fp, NULL);
			set_fs(old_fs);
			goto fw_update_out;
		}

		info->cap_info.ic_fw_size = (buff[0x69] << 16) | (buff[0x6A] << 8) | buff[0x6B]; //total size
		/* signed firmware is not equal with fw.buf_size: add tag and signature */
		if (update_type == TSP_TYPE_EXTERNAL_FW) {
			if (fsize != info->cap_info.ic_fw_size) {
				input_err(true, &client->dev, "%s: invalid fw size!!\n", __func__);
				input_info(true, &client->dev, "f/w size = %ld ic_fw_size = %d \n", fsize, info->cap_info.ic_fw_size);
				filp_close(fp, NULL);
				set_fs(old_fs);
				goto fw_update_out;
			}
		} else {
			input_info(true, &client->dev, "%s: signed firmware\n", __func__);
		}

		filp_close(fp, current->files);
		set_fs(old_fs);
		input_info(true, &client->dev, "%s: ums fw is loaded!!\n", __func__);

#ifdef SPU_FW_SIGNED
		info->fw_data = (unsigned char *)buff;
		if (!(ts_check_need_upgrade(info, info->cap_info.fw_version, info->cap_info.fw_minor_version,
						info->cap_info.reg_data_version, info->cap_info.hw_id)) &&
				(update_type == TSP_TYPE_SPU_FW_SIGNED)) {
			input_info(true, &client->dev, "%s: skip ffu update\n", __func__);
			goto fw_update_out;
		}

		if (update_type == TSP_TYPE_EXTERNAL_FW_SIGNED || update_type == TSP_TYPE_SPU_FW_SIGNED) {
			int ori_size;
			int spu_ret;

			ori_size = fsize - SPU_METADATA_SIZE(TSP);

			spu_ret = spu_firmware_signature_verify("TSP", buff, fsize);
			if (ori_size != spu_ret) {
				input_err(true, &client->dev, "%s: signature verify failed, ori:%d, fsize:%ld\n",
						__func__, ori_size, fsize);

				goto fw_update_out;
			}
		}
#endif

#ifdef TCLM_CONCEPT
		sec_tclm_root_of_cal(info->tdata, CALPOSITION_TESTMODE);
		restore_cal = 1;
#endif
		ret = ts_upgrade_sequence(info, (u8*)buff, restore_cal);
		if (ret < 0)
			goto fw_update_out;

		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

fw_update_out:
#ifdef TCLM_CONCEPT
	sec_tclm_root_of_cal(info->tdata, CALPOSITION_NONE);
#endif
	if (!buff)
		kfree(buff);

	if (sec->cmd_state == SEC_CMD_STATUS_OK)
		snprintf(result, sizeof(result), "OK");
	else
		snprintf(result, sizeof(result), "NG");

	sec_cmd_set_cmd_result(sec, result, strnlen(result, sizeof(result)));
	return;
}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct zt_ts_platform_data *pdata = info->pdata;
	struct i2c_client *client = info->client;
	const struct firmware *tsp_fw = NULL;
	unsigned char *fw_data = NULL;
	char fw_path[MAX_FW_PATH];
	char buff[16] = { 0 };
	u16 fw_version, fw_minor_version, reg_version, hw_id, ic_revision;
	u32 version;
	int ret;

	snprintf(fw_path, MAX_FW_PATH, "%s", pdata->firmware_name);

	ret = request_firmware(&tsp_fw, fw_path, &(client->dev));
	if (ret) {
		input_info(true, &client->dev,
				"%s: Firmware image %s not available\n", __func__,
				fw_path);
		goto fw_request_fail;
	} else {
		fw_data = (unsigned char *)tsp_fw->data;
	}
	sec_cmd_set_default_result(sec);

	/* To Do */
	/* modify m_firmware_data */
	hw_id = (u16)(fw_data[48] | (fw_data[49] << 8));
	fw_version = (u16)(fw_data[52] | (fw_data[53] << 8));
	fw_minor_version = (u16)(fw_data[56] | (fw_data[57] << 8));
	reg_version = (u16)(fw_data[60] | (fw_data[61] << 8));

	ic_revision = (u16)(fw_data[68] | (fw_data[69] << 8));
	version = (u32)((u32)(ic_revision & 0xff) << 24)
		| ((fw_minor_version & 0xff) << 16)
		| ((hw_id & 0xff) << 8) | (reg_version & 0xff);

	snprintf(buff, sizeof(buff), "ZI%08X", version);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_BIN");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

fw_request_fail:
	if (tsp_fw)
		release_firmware(tsp_fw);
	return;
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[16] = { 0 };
	char model[16] = { 0 };
	u16 fw_version, fw_minor_version, reg_version, hw_id, vendor_id, ic_revision;
	u32 version, length;
	int ret;

	sec_cmd_set_default_result(sec);

	zt_ts_esd_timer_stop(info);

	mutex_lock(&info->work_lock);

	ret = ic_version_check(info);
	mutex_unlock(&info->work_lock);
	if (ret < 0) {
		input_info(true, &client->dev, "%s: version check error\n", __func__);
		return;
	}

	zt_ts_esd_timer_start(info);

	fw_version = info->cap_info.fw_version;
	fw_minor_version = info->cap_info.fw_minor_version;
	reg_version = info->cap_info.reg_data_version;
	hw_id = info->cap_info.hw_id;
	ic_revision=  info->cap_info.ic_revision;

	vendor_id = ntohs(info->cap_info.vendor_id);
	version = (u32)((u32)(ic_revision & 0xff) << 24)
		| ((fw_minor_version & 0xff) << 16)
		| ((hw_id & 0xff) << 8) | (reg_version & 0xff);

	length = sizeof(vendor_id);
	snprintf(buff, length + 1, "%s", (u8 *)&vendor_id);
	snprintf(buff + length, sizeof(buff) - length, "%08X", version);
	snprintf(model, length + 1, "%s", (u8 *)&vendor_id);
	snprintf(model + length, sizeof(model) - length, "%04X", version >> 16);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
		sec_cmd_set_cmd_result_all(sec, model, strnlen(model, sizeof(model)), "FW_MODEL");
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

static void get_checksum_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[16] = { 0 };
	u16 checksum;

	sec_cmd_set_default_result(sec);

	read_data(client, ZT_CHECKSUM, (u8 *)&checksum, 2);

	snprintf(buff, sizeof(buff), "0x%X", checksum);
	input_info(true, &client->dev, "%s %d %x\n",__func__,checksum,checksum);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

static void get_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[20] = { 0 };

	sec_cmd_set_default_result(sec);

	read_data(client, ZT_THRESHOLD, (u8 *)&info->cap_info.threshold, 2);

	snprintf(buff, sizeof(buff), "%d", info->cap_info.threshold);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

static void get_module_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[16] = {0};

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff),  "%s", tostring(NA));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}


#define ZT_VENDOR_NAME "ZINITIX"

static void get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s", ZT_VENDOR_NAME);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_VENDOR");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

#define ZT_CHIP_NAME "ZT"

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct zt_ts_platform_data *pdata = info->pdata;
	struct i2c_client *client = info->client;
	const char *name_buff;
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	if (pdata->chip_name)
		name_buff = pdata->chip_name;
	else
		name_buff = ZT_CHIP_NAME;

	snprintf(buff, sizeof(buff), "%s", name_buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_NAME");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

static void get_x_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	read_data(client, ZT_TOTAL_NUMBER_OF_X, (u8 *)&info->cap_info.x_node_num, 2);

	snprintf(buff, sizeof(buff), "%u", info->cap_info.x_node_num);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

static void get_y_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	read_data(client, ZT_TOTAL_NUMBER_OF_Y, (u8 *)&info->cap_info.y_node_num, 2);

	snprintf(buff, sizeof(buff), "%u", info->cap_info.y_node_num);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s", "NA");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;

	sec_cmd_set_cmd_exit(sec);

	input_info(true, &client->dev, "%s: \"%s(%d)\"\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

#define REG_CHANNEL_TEST_RESULT				0x0296
#define TEST_CHANNEL_OPEN				0x0D
#define TEST_PATTERN_OPEN				0x04
#define TEST_SHORT					0x08
#define TEST_PASS					0xFF

static bool get_channel_test_result(struct zt_ts_info *info, int skip_cnt)
{
	struct i2c_client *client = info->client;
	struct zt_ts_platform_data *pdata = info->pdata;
	int i;
	int retry = 150;

	disable_irq(info->irq);

	mutex_lock(&info->work_lock);
	if (info->work_state != NOTHING) {
		input_info(true, &client->dev, "other process occupied.. (%d)\n",
				info->work_state);
		enable_irq(info->irq);
		mutex_unlock(&info->work_lock);
		return false;
	}

	info->work_state = RAW_DATA;

	for (i = 0; i < skip_cnt; i++) {
		while (gpio_get_value(pdata->gpio_int)) {
			zt_delay(7);
			if (--retry < 0)
				break;
		}
		retry = 150;
		write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
		zt_delay(1);
	}

	retry = 100;
	input_info(true, &client->dev, "%s: channel_test_result read\n", __func__);

	while (gpio_get_value(pdata->gpio_int)) {
		zt_delay(30);
		if (--retry < 0)
			break;
		else
			input_info(true, &client->dev, "%s: retry:%d\n", __func__, retry);
	}

	read_data(info->client, REG_CHANNEL_TEST_RESULT, (u8 *)info->raw_data->channel_test_data, 10);

	write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
	clear_report_data(info);
	info->work_state = NOTHING;
	enable_irq(info->irq);
	mutex_unlock(&info->work_lock);

	return true;
}

static void run_test_open_short(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;

	zt_ts_esd_timer_stop(info);
	ts_set_touchmode(TOUCH_CHANNEL_TEST_MODE);
	get_channel_test_result(info, 2);
	ts_set_touchmode(TOUCH_POINT_MODE);

	input_info(true, &client->dev, "channel_test_result : %04X\n", raw_data->channel_test_data[0]);
	input_info(true, &client->dev, "RX Channel : %08X\n",
			raw_data->channel_test_data[1] | ((raw_data->channel_test_data[2] << 16) & 0xffff0000));
	input_info(true, &client->dev, "TX Channel : %08X\n",
			raw_data->channel_test_data[3] | ((raw_data->channel_test_data[4] << 16) & 0xffff0000));

	if (raw_data->channel_test_data[0] == TEST_SHORT) {
		info->ito_test[3] |= 0x0F;
	} else if (raw_data->channel_test_data[0] == TEST_CHANNEL_OPEN || raw_data->channel_test_data[0] == TEST_PATTERN_OPEN) {
		if (raw_data->channel_test_data[3] | ((raw_data->channel_test_data[4] << 16) & 0xffff0000))
			info->ito_test[3] |= 0x10;

		if (raw_data->channel_test_data[1] | ((raw_data->channel_test_data[2] << 16) & 0xffff0000))
			info->ito_test[3] |= 0x20;
	}

	zt_ts_esd_timer_start(info);
}

static void check_trx_channel_test(struct zt_ts_info *info, char *buf)
{
	struct tsp_raw_data *raw_data = info->raw_data;
	u8 temp[10];
	int ii;
	u32 test_result;

	memset(temp, 0x00, sizeof(temp));

	test_result = raw_data->channel_test_data[3] | ((raw_data->channel_test_data[4] << 16) & 0xffff0000);
	for (ii = 0; ii < info->cap_info.x_node_num; ii++) {
		if (test_result & (1 << ii)) {
			memset(temp, 0x00, 10);
			snprintf(temp, sizeof(temp), "T%d, ", ii);
			strlcat(buf, temp, SEC_CMD_STR_LEN);
		}
	}

	test_result = raw_data->channel_test_data[1] | ((raw_data->channel_test_data[2] << 16) & 0xffff0000);
	for (ii = 0; ii < info->cap_info.y_node_num; ii++) {
		if (test_result & (1 << ii)) {
			memset(temp, 0x00, 10);
			snprintf(temp, sizeof(temp), "R%d, ", ii);
			strlcat(buf, temp, SEC_CMD_STR_LEN);
		}
	}
}

static void run_trx_short_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 temp[10];
	char test[32];

	sec_cmd_set_default_result(sec);

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &info->client->dev, "%s: IC is power off\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if (sec->cmd_param[1])
		snprintf(test, sizeof(test), "TEST=%d,%d", sec->cmd_param[0], sec->cmd_param[1]);
	else
		snprintf(test, sizeof(test), "TEST=%d", sec->cmd_param[0]);

	/*
	 * run_test_open_short() need to be fix for separate by test item(open, short, pattern open)
	 */
	if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 1)
		run_test_open_short(info);

	if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 1) {
		/* 1,1 : open  */
		if (raw_data->channel_test_data[0] != TEST_CHANNEL_OPEN)
			goto OK;

		memset(temp, 0x00, sizeof(temp));
		snprintf(temp, sizeof(temp), "OPEN: ");
		strlcat(buff, temp, SEC_CMD_STR_LEN);

		check_trx_channel_test(info, buff);
		input_info(true, &client->dev, "%s\n", buff);
	} else if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 2) {
		/* 1,2 : short  */
		if (raw_data->channel_test_data[0] != TEST_SHORT)
			goto OK;

		memset(temp, 0x00, sizeof(temp));
		snprintf(temp, sizeof(temp), "SHORT: ");
		strlcat(buff, temp, SEC_CMD_STR_LEN);

		check_trx_channel_test(info, buff);
		input_info(true, &client->dev, "%s\n", buff);
	} else if (sec->cmd_param[0] == 2) {
		/* 2 : micro open(pattern open)  */
		if (raw_data->channel_test_data[0] != TEST_PATTERN_OPEN)
			goto OK;

		memset(temp, 0x00, sizeof(temp));
		snprintf(temp, sizeof(temp), "CRACK: ");
		strlcat(buff, temp, SEC_CMD_STR_LEN);

		check_trx_channel_test(info, buff);
		input_info(true, &client->dev, "%s\n", buff);
	} else if (sec->cmd_param[0] == 3) {
		/* 3 : bridge short  */
		snprintf(buff, sizeof(buff), "NA");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;

		sec_cmd_send_event_to_user(sec, test, "RESULT=FAIL");

		input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
				(int)strlen(sec->cmd_result));
		return;
	} else {
		/* 0 or else : old command */
		if (raw_data->channel_test_data[0] == TEST_PASS)
			goto OK;
	}

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	sec_cmd_send_event_to_user(sec, test, "RESULT=FAIL");

	input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));
	return;


OK:
	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_send_event_to_user(sec, test, "RESULT=PASS");

	input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));
	return;

}

static void touch_aging_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec->cmd_param[0] == 1) {
		zt_ts_esd_timer_stop(info);
		ret = ts_set_touchmode(TOUCH_AGING_MODE);
	} else {
		ret = ts_set_touchmode(TOUCH_POINT_MODE);
		zt_ts_esd_timer_start(info);	
	}
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_cnd_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u16 min, max;
	s32 i, j;
	int ret;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ret = ts_set_touchmode(TOUCH_RAW_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)raw_data->cnd_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	input_info(true,&client->dev, "%s: CND start\n", __func__);

	min = 0xFFFF;
	max = 0x0000;

	for (i = 0; i < info->cap_info.y_node_num; i++) {
		for (j = 0; j < info->cap_info.x_node_num; j++) {
			if (raw_data->cnd_data[i * info->cap_info.x_node_num + j] < min &&
					raw_data->cnd_data[i * info->cap_info.x_node_num + j] != 0)
				min = raw_data->cnd_data[i * info->cap_info.x_node_num + j];

			if (raw_data->cnd_data[i * info->cap_info.x_node_num + j] > max)
				max = raw_data->cnd_data[i * info->cap_info.x_node_num + j];
		}
	}
	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CND");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	zt_display_rawdata(info, raw_data, TOUCH_RAW_MODE, 0);
out:
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CND");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}
	input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));

	zt_ts_esd_timer_start(info);
	return;
}

static void run_cnd_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[16] = { 0 };
	char *all_cmdbuff;
	s32 i, j;
	int ret;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ret = ts_set_touchmode(TOUCH_RAW_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)raw_data->cnd_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	all_cmdbuff = kzalloc(info->cap_info.x_node_num * info->cap_info.y_node_num * 6, GFP_KERNEL);
	if (!all_cmdbuff) {
		input_info(true, &info->client->dev, "%s: alloc failed\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < info->cap_info.y_node_num; i++) {
		for (j = 0; j < info->cap_info.x_node_num; j++) {
			sprintf(buff, "%u,", raw_data->cnd_data[i * info->cap_info.x_node_num + j]);
			strcat(all_cmdbuff, buff);
		}
	}

	sec_cmd_set_cmd_result(sec, all_cmdbuff,
			strnlen(all_cmdbuff, sizeof(all_cmdbuff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(all_cmdbuff);

out:
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	zt_ts_esd_timer_start(info);
	return;
}

static void run_dnd_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u16 min, max;
	s32 i, j;
	int ret;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ret = ts_set_touchmode(TOUCH_DND_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)raw_data->dnd_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	min = 0xFFFF;
	max = 0x0000;

	for (i = 0; i < info->cap_info.y_node_num; i++) {
		for (j = 0; j < info->cap_info.x_node_num; j++) {
			if (raw_data->dnd_data[i * info->cap_info.x_node_num + j] < min &&
					raw_data->dnd_data[i * info->cap_info.x_node_num + j] != 0)
				min = raw_data->dnd_data[i * info->cap_info.x_node_num + j];

			if (raw_data->dnd_data[i * info->cap_info.x_node_num + j] > max)
				max = raw_data->dnd_data[i * info->cap_info.x_node_num + j];
		}
	}
	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "DND");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	zt_display_rawdata(info, raw_data, TOUCH_DND_MODE, 0);
out:
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "DND");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}
	input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));

	zt_ts_esd_timer_start(info);
	return;
}

static void run_dnd_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[16] = { 0 };
	char *all_cmdbuff;
	s32 i, j;
	int ret;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ret = ts_set_touchmode(TOUCH_DND_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)raw_data->dnd_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	all_cmdbuff = kzalloc(info->cap_info.x_node_num * info->cap_info.y_node_num * 6, GFP_KERNEL);
	if (!all_cmdbuff) {
		input_info(true, &info->client->dev, "%s: alloc failed\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < info->cap_info.y_node_num; i++) {
		for (j = 0; j < info->cap_info.x_node_num; j++) {
			sprintf(buff, "%u,", raw_data->dnd_data[i * info->cap_info.x_node_num + j]);
			strcat(all_cmdbuff, buff);
		}
	}

	sec_cmd_set_cmd_result(sec, all_cmdbuff,
			strnlen(all_cmdbuff, sizeof(all_cmdbuff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(all_cmdbuff);

out:
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	zt_ts_esd_timer_start(info);
	return;
}

static void run_dnd_v_gap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char buff_onecmd_1[SEC_CMD_STR_LEN] = { 0 };
	int x_num = info->cap_info.x_node_num, y_num = info->cap_info.y_node_num;
	int i, j, offset, val, cur_val, next_val;
	u16 screen_max = 0x0000;

	sec_cmd_set_default_result(sec);

	memset(raw_data->vgap_data, 0x00, TSP_CMD_NODE_NUM);

	input_info(true, &client->dev, "%s: DND V Gap start\n", __func__);

	input_info(true, &client->dev, "%s : ++++++ DND SPEC +++++++++\n",__func__);
	for (i = 0; i < y_num - 1; i++) {
		for (j = 0; j < x_num; j++) {
			offset = (i * x_num) + j;

			cur_val = raw_data->dnd_data[offset];
			next_val = raw_data->dnd_data[offset + x_num];
			if (!next_val) {
				raw_data->vgap_data[offset] = next_val;
				continue;
			}

			if (next_val > cur_val)
				val = 100 - ((cur_val * 100) / next_val);
			else
				val = 100 - ((next_val * 100) / cur_val);

			raw_data->vgap_data[offset] = val;

			if (raw_data->vgap_data[i * x_num + j] > screen_max)
				screen_max = raw_data->vgap_data[i * x_num + j];
		}
	}

	input_info(true, &client->dev, "DND V Gap screen_max %d\n", screen_max);
	snprintf(buff, sizeof(buff), "%d", screen_max);
	snprintf(buff_onecmd_1, sizeof(buff_onecmd_1), "%d,%d", 0, screen_max);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff_onecmd_1, strnlen(buff_onecmd_1, sizeof(buff_onecmd_1)), "DND_V_GAP");

	sec->cmd_state = SEC_CMD_STATUS_OK;

	zt_display_rawdata(info, raw_data, TOUCH_DND_MODE, 1);

	return;
}

static void run_dnd_h_gap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char buff_onecmd_1[SEC_CMD_STR_LEN] = { 0 };
	int x_num = info->cap_info.x_node_num, y_num = info->cap_info.y_node_num;
	int i, j, offset, val, cur_val, next_val;
	u16 screen_max = 0x0000;

	sec_cmd_set_default_result(sec);

	memset(raw_data->hgap_data, 0x00, TSP_CMD_NODE_NUM);

	input_info(true, &client->dev, "%s: DND H Gap start\n", __func__);

	for (i = 0; i < y_num; i++) {
		for (j = 0; j < x_num - 1; j++) {
			offset = (i * x_num) + j;

			cur_val = raw_data->dnd_data[offset];
			if (!cur_val) {
				raw_data->hgap_data[offset] = cur_val;
				continue;
			}

			next_val = raw_data->dnd_data[offset + 1];
			if (!next_val) {
				raw_data->hgap_data[offset] = next_val;
				for (++j; j < x_num - 1; j++) {
					offset = (i * x_num) + j;

					next_val = raw_data->dnd_data[offset];
					if (!next_val) {
						raw_data->hgap_data[offset] = next_val;
						continue;
					}
					break;
				}
			}

			if (next_val > cur_val)
				val = 100 - ((cur_val * 100) / next_val);
			else
				val = 100 - ((next_val * 100) / cur_val);

			raw_data->hgap_data[offset] = val;

			if (raw_data->hgap_data[i * x_num + j] > screen_max)
				screen_max = raw_data->hgap_data[i * x_num + j];
		}
	}

	input_info(true, &client->dev, "DND H Gap screen_max %d\n", screen_max);
	snprintf(buff, sizeof(buff), "%d", screen_max);
	snprintf(buff_onecmd_1, sizeof(buff_onecmd_1), "%d,%d", 0, screen_max);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff_onecmd_1, strnlen(buff_onecmd_1, sizeof(buff_onecmd_1)), "DND_H_GAP");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	zt_display_rawdata(info, raw_data, TOUCH_DND_MODE, 2);

	return;
}

static void run_dnd_h_gap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct tsp_raw_data *raw_data = info->raw_data;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	char *buff = NULL;
	int total_node = info->cap_info.x_node_num * info->cap_info.y_node_num;
	int i, j, offset, val, cur_val, next_val;

	sec_cmd_set_default_result(sec);

	buff = kzalloc(total_node * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff) {
		snprintf(temp, SEC_CMD_STR_LEN, "NG");
		sec_cmd_set_cmd_result(sec, temp, SEC_CMD_STR_LEN);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (i = 0; i < info->cap_info.y_node_num; i++) {
		for (j = 0; j < info->cap_info.x_node_num - 1; j++) {
			offset = (i * info->cap_info.x_node_num) + j;

			cur_val = raw_data->dnd_data[offset];
			if (!cur_val) {
				raw_data->hgap_data[offset] = cur_val;
				continue;
			}

			next_val = raw_data->dnd_data[offset + 1];
			if (!next_val) {
				raw_data->hgap_data[offset] = next_val;
				for (++j; j < info->cap_info.x_node_num - 1; j++) {
					offset = (i * info->cap_info.x_node_num) + j;

					next_val = raw_data->dnd_data[offset];
					if (!next_val) {
						raw_data->hgap_data[offset] = next_val;
						continue;
					}
					break;
				}
			}

			if (next_val > cur_val)
				val = 100 - ((cur_val * 100) / next_val);
			else
				val = 100 - ((next_val * 100) / cur_val);

			raw_data->hgap_data[offset] = val;

			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", raw_data->hgap_data[offset]);
			strncat(buff, temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, total_node * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void run_dnd_v_gap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct tsp_raw_data *raw_data = info->raw_data;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	char *buff = NULL;
	int total_node = info->cap_info.x_node_num * info->cap_info.y_node_num;
	int i, j, offset, val, cur_val, next_val;

	sec_cmd_set_default_result(sec);

	buff = kzalloc(total_node * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff) {
		snprintf(temp, SEC_CMD_STR_LEN, "NG");
		sec_cmd_set_cmd_result(sec, temp, SEC_CMD_STR_LEN);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (i = 0; i < info->cap_info.y_node_num - 1; i++) {
		for (j = 0; j < info->cap_info.x_node_num; j++) {
			offset = (i * info->cap_info.x_node_num) + j;

			cur_val = raw_data->dnd_data[offset];
			next_val = raw_data->dnd_data[offset + info->cap_info.x_node_num];
			if (!next_val) {
				raw_data->vgap_data[offset] = next_val;
				continue;
			}

			if (next_val > cur_val)
				val = 100 - ((cur_val * 100) / next_val);
			else
				val = 100 - ((next_val * 100) / cur_val);

			raw_data->vgap_data[offset] = val;

			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", raw_data->vgap_data[offset]);
			strncat(buff, temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, total_node * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void run_selfdnd_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	int total_node = info->cap_info.x_node_num + info->cap_info.y_node_num;
	char tx_buff[SEC_CMD_STR_LEN] = { 0 };
	char rx_buff[SEC_CMD_STR_LEN] = { 0 };
	u16 tx_min, tx_max, rx_min, rx_max;
	s32 j;
	int ret;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ret = ts_set_touchmode(TOUCH_SELF_DND_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)raw_data->selfdnd_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	input_info(true,&client->dev, "%s: SELF DND start\n", __func__);

	tx_min = 0xFFFF;
	tx_max = 0x0000;
	rx_min = 0xFFFF;
	rx_max = 0x0000;

	for (j = 0; j < info->cap_info.x_node_num; j++) {
		if (raw_data->selfdnd_data[j] < rx_min && raw_data->selfdnd_data[j] != 0)
			rx_min = raw_data->selfdnd_data[j];

		if (raw_data->selfdnd_data[j] > rx_max)
			rx_max = raw_data->selfdnd_data[j];
	}

	for (j = info->cap_info.x_node_num; j < total_node; j++) {
		if (raw_data->selfdnd_data[j] < tx_min && raw_data->selfdnd_data[j] != 0)
			tx_min = raw_data->selfdnd_data[j];

		if (raw_data->selfdnd_data[j] > tx_max)
			tx_max = raw_data->selfdnd_data[j];
	}

	input_info(true, &client->dev, "%s: SELF DND Pass\n", __func__);

	snprintf(tx_buff, sizeof(tx_buff), "%d,%d", tx_min, tx_max);
	snprintf(rx_buff, sizeof(rx_buff), "%d,%d", rx_min, rx_max);
	sec_cmd_set_cmd_result(sec, rx_buff, strnlen(rx_buff, sizeof(rx_buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, rx_buff, strnlen(rx_buff, sizeof(rx_buff)), "SELF_DND_RX");
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	if (ret < 0) {
		snprintf(rx_buff, sizeof(rx_buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, rx_buff, strnlen(rx_buff, sizeof(rx_buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
			sec_cmd_set_cmd_result_all(sec, rx_buff, strnlen(rx_buff, sizeof(rx_buff)), "SELF_DND_RX");
		}
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}
	input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));

	zt_ts_esd_timer_start(info);
	return;
}

static void run_charge_pump_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ret = ts_set_touchmode(TOUCH_CHARGE_PUMP_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)raw_data->charge_pump_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	snprintf(buff, sizeof(buff), "%d,%d", raw_data->charge_pump_data[0], raw_data->charge_pump_data[0]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CHARGE_PUMP");
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CHARGE_PUMP");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}
	input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));

	zt_ts_esd_timer_start(info);
	return;
}

static void run_selfdnd_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct tsp_raw_data *raw_data = info->raw_data;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	char *buff = NULL;
	int total_node = info->cap_info.x_node_num + info->cap_info.y_node_num;
	s32 j;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ts_set_touchmode(TOUCH_SELF_DND_MODE);

	get_raw_data(info, (u8 *)raw_data->selfdnd_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	buff = kzalloc(total_node * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto NG;

	for (j = 0; j < total_node; j++) {
		snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", raw_data->selfdnd_data[j]);
		strncat(buff, temp, CMD_RESULT_WORD_LEN);
		memset(temp, 0x00, SEC_CMD_STR_LEN);
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, total_node * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);

NG:
	if (sec->cmd_state != SEC_CMD_STATUS_OK) {
		snprintf(temp, SEC_CMD_STR_LEN, "NG");
		sec_cmd_set_cmd_result(sec, temp, SEC_CMD_STR_LEN);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}
	zt_ts_esd_timer_start(info);
}

static void run_self_saturation_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	int total_node = info->cap_info.x_node_num + info->cap_info.y_node_num;
	char tx_buff[SEC_CMD_STR_LEN] = { 0 };
	char rx_buff[SEC_CMD_STR_LEN] = { 0 };
	u16 tx_min, tx_max, rx_min, rx_max;
	s32 j;
	int ret;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ret = ts_set_touchmode(DEF_RAW_SELF_SSR_DATA_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)raw_data->ssr_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	input_info(true,&client->dev, "%s: SELF SATURATION start\n", __func__);

	tx_min = 0xFFFF;
	tx_max = 0x0000;
	rx_min = 0xFFFF;
	rx_max = 0x0000;

	for (j = 0; j < info->cap_info.x_node_num; j++) {
		if (raw_data->ssr_data[j] < rx_min && raw_data->ssr_data[j] != 0)
			rx_min = raw_data->ssr_data[j];

		if (raw_data->ssr_data[j] > rx_max)
			rx_max = raw_data->ssr_data[j];
	}

	for (j = info->cap_info.x_node_num; j < total_node; j++) {
		if (raw_data->ssr_data[j] < tx_min && raw_data->ssr_data[j] != 0)
			tx_min = raw_data->ssr_data[j];

		if (raw_data->ssr_data[j] > tx_max)
			tx_max = raw_data->ssr_data[j];
	}

	input_info(true, &client->dev, "%s: SELF SATURATION Pass\n", __func__);

	snprintf(tx_buff, sizeof(tx_buff), "%d,%d", tx_min, tx_max);
	snprintf(rx_buff, sizeof(rx_buff), "%d,%d", rx_min, rx_max);
	sec_cmd_set_cmd_result(sec, rx_buff, strnlen(rx_buff, sizeof(rx_buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, tx_buff, strnlen(tx_buff, sizeof(tx_buff)), "SELF_SATURATION_TX");
		sec_cmd_set_cmd_result_all(sec, rx_buff, strnlen(rx_buff, sizeof(rx_buff)), "SELF_SATURATION_RX");
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	if (ret < 0) {
		snprintf(rx_buff, sizeof(rx_buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, rx_buff, strnlen(rx_buff, sizeof(rx_buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
			sec_cmd_set_cmd_result_all(sec, rx_buff, strnlen(rx_buff, sizeof(rx_buff)), "SELF_SATURATION_TX");
			sec_cmd_set_cmd_result_all(sec, rx_buff, strnlen(rx_buff, sizeof(rx_buff)), "SELF_SATURATION_RX");
		}
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}
	input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));

	zt_ts_esd_timer_start(info);
	return;
}

static void run_ssr_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[16] = { 0 };
	int total_node = info->cap_info.x_node_num + info->cap_info.y_node_num;
	char *all_cmdbuff;
	s32 j;
	int ret;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ret = ts_set_touchmode(DEF_RAW_SELF_SSR_DATA_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)raw_data->ssr_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	all_cmdbuff = kzalloc(total_node * 6, GFP_KERNEL);
	if (!all_cmdbuff) {
		input_info(true, &info->client->dev, "%s: alloc failed\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	for (j = 0; j < total_node; j++) {
		sprintf(buff, "%u,", raw_data->ssr_data[j]);
		strcat(all_cmdbuff, buff);
	}

	sec_cmd_set_cmd_result(sec, all_cmdbuff,
			strnlen(all_cmdbuff, sizeof(all_cmdbuff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(all_cmdbuff);

out:
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	zt_ts_esd_timer_start(info);
	return;
}

static void run_selfdnd_h_gap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char buff_onecmd[SEC_CMD_STR_LEN] = { 0 };
	int j, offset, val, cur_val, next_val;
	u16 screen_max = 0x0000;

	sec_cmd_set_default_result(sec);

	memset(raw_data->self_hgap_data, 0x00, TSP_CMD_NODE_NUM);

	input_info(true, &client->dev, "%s: SELFDND H Gap start\n", __func__);

	for (j = 0; j < info->cap_info.x_node_num - 1; j++) {
		offset = j;
		cur_val = raw_data->selfdnd_data[offset];

		if (!cur_val) {
			raw_data->self_hgap_data[offset] = cur_val;
			continue;
		}

		next_val = raw_data->selfdnd_data[offset + 1];

		if (next_val > cur_val)
			val = 100 - ((cur_val * 100) / next_val);
		else
			val = 100 - ((next_val * 100) / cur_val);

		raw_data->self_hgap_data[offset] = val;

		if (raw_data->self_hgap_data[j] > screen_max)
			screen_max = raw_data->self_hgap_data[j];

	}

	input_info(true, &client->dev, "SELFDND H Gap screen_max %d\n", screen_max);
	snprintf(buff, sizeof(buff), "%d", screen_max);
	snprintf(buff_onecmd, sizeof(buff_onecmd), "%d,%d", 0, screen_max);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff_onecmd, strnlen(buff_onecmd, sizeof(buff_onecmd)), "SELF_DND_H_GAP");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return;
}

static void run_selfdnd_h_gap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct tsp_raw_data *raw_data = info->raw_data;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	char *buff = NULL;
	int total_node = info->cap_info.y_node_num;
	int j, offset, val, cur_val, next_val;

	sec_cmd_set_default_result(sec);

	buff = kzalloc(total_node * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff) {
		snprintf(temp, SEC_CMD_STR_LEN, "NG");
		sec_cmd_set_cmd_result(sec, temp, SEC_CMD_STR_LEN);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (j = 0; j < info->cap_info.x_node_num - 1; j++) {
		offset = j;
		cur_val = raw_data->selfdnd_data[offset];

		if (!cur_val) {
			raw_data->self_hgap_data[offset] = cur_val;
			continue;
		}

		next_val = raw_data->selfdnd_data[offset + 1];

		if (next_val > cur_val)
			val = 100 - ((cur_val * 100) / next_val);
		else
			val = 100 - ((next_val * 100) / cur_val);

		raw_data->self_hgap_data[offset] = val;

		snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", raw_data->self_hgap_data[offset]);
		strncat(buff, temp, CMD_RESULT_WORD_LEN);
		memset(temp, 0x00, SEC_CMD_STR_LEN);
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, total_node * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void run_tsp_rawdata_read(void *device_data, u16 rawdata_mode, s16* buff)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	int x_num = info->cap_info.x_node_num, y_num = info->cap_info.y_node_num;
	int i, j, ret;

	zt_ts_esd_timer_stop(info);
	ret = ts_set_touchmode(rawdata_mode);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)buff, 2);
	ts_set_touchmode(TOUCH_POINT_MODE);

	input_info(true, &info->client->dev, "touch rawdata %d start\n", rawdata_mode);

	for (i = 0; i < y_num; i++) {
		pr_info("%s [%5d] :", SECLOG, i);
		for (j = 0; j < x_num; j++) {
			pr_cont("%06d ", buff[(i * x_num) + j]);
		}
		pr_cont("\n");
	}
out:
	zt_ts_esd_timer_start(info);
	return;
}

/*
## Mis Cal result ##
FD : spec out
F3,F4 : i2c faile
F2 : power off state
F1 : not support mis cal concept
F0 : initial value in function
00 : pass
 */

static void run_mis_cal_read(void * device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct zt_ts_platform_data *pdata = info->pdata;
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int x_num = info->cap_info.x_node_num, y_num = info->cap_info.y_node_num;
	int i, j, offset;
	char mis_cal_data = 0xF0;
	int ret = 0;
	s16 raw_data_buff[TSP_CMD_NODE_NUM];
	u16 chip_eeprom_info;
	int min = 0xFFFF, max = -0xFF;

	zt_ts_esd_timer_stop(info);
	disable_irq(info->irq);
	sec_cmd_set_default_result(sec);

	if (pdata->mis_cal_check == 0) {
		input_info(true, &info->client->dev, "%s: [ERROR] not support\n", __func__);
		mis_cal_data = 0xF1;
		goto NG;
	}

	if (info->work_state == SUSPEND) {
		input_info(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",__func__);
		mis_cal_data = 0xF2;
		goto NG;
	}

	if (read_data(info->client, ZT_EEPROM_INFO, (u8 *)&chip_eeprom_info, 2) < 0) {
		input_info(true, &info->client->dev, "%s: read eeprom_info i2c fail!\n", __func__);
		mis_cal_data = 0xF3;
		goto NG;
	}

	if (zinitix_bit_test(chip_eeprom_info, 0)) {
		input_info(true, &info->client->dev, "%s: eeprom cal 0, skip !\n", __func__);
		mis_cal_data = 0xF1;
		goto NG;
	}

	ret = ts_set_touchmode(TOUCH_REF_ABNORMAL_TEST_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		mis_cal_data = 0xF4;
		goto NG;
	}
	ret = get_raw_data(info, (u8 *)raw_data->reference_data_abnormal, 2);
	if (!ret) {
		input_info(true, &info->client->dev, "%s:[ERROR] i2c fail!\n", __func__);
		ts_set_touchmode(TOUCH_POINT_MODE);
		mis_cal_data = 0xF4;
		goto NG;
	}
	ts_set_touchmode(TOUCH_POINT_MODE);

	input_info(true, &info->client->dev, "%s start\n", __func__);

	ret = 1;
	for (i = 0; i < y_num; i++) {
		for (j = 0; j < x_num; j++) {
			offset = (i * x_num) + j;

			if (raw_data->reference_data_abnormal[offset] < min)
				min = raw_data->reference_data_abnormal[offset];

			if (raw_data->reference_data_abnormal[offset] > max)
				max = raw_data->reference_data_abnormal[offset];
		}
	}
	if (!ret)
		goto NG;

	mis_cal_data = 0x00;
	snprintf(buff, sizeof(buff), "%d,%d", min, max);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MIS_CAL");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);

	zt_display_rawdata(info, raw_data, TOUCH_REF_ABNORMAL_TEST_MODE, 0);

	return;
NG:
	snprintf(buff, sizeof(buff), "%s_%d", "NG", mis_cal_data);

	if (mis_cal_data == 0xFD) {
		run_tsp_rawdata_read(device_data, 7, raw_data_buff);
		run_tsp_rawdata_read(device_data, TOUCH_REFERENCE_MODE, raw_data_buff);
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MIS_CAL");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &client->dev, "%s: \"%s\"(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
	return;
}

static void run_mis_cal_read_all(void * device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct tsp_raw_data *raw_data = info->raw_data;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	char *buff = NULL;
	int total_node = info->cap_info.x_node_num * info->cap_info.y_node_num;
	int i, j, offset;

	zt_ts_esd_timer_stop(info);
	disable_irq(info->irq);

	sec_cmd_set_default_result(sec);

	ts_set_touchmode(TOUCH_POINT_MODE);

	buff = kzalloc(total_node * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto NG;

	for (i = 0; i < info->cap_info.y_node_num ; i++) {
		for (j = 0; j < info->cap_info.x_node_num ; j++) {
			offset = (i * info->cap_info.x_node_num) + j;
			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", raw_data->reference_data_abnormal[offset]);
			strncat(buff, temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, total_node * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);

NG:
	if (sec->cmd_state != SEC_CMD_STATUS_OK) {
		snprintf(temp, SEC_CMD_STR_LEN, "NG");
		sec_cmd_set_cmd_result(sec, temp, SEC_CMD_STR_LEN);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
}

static void run_jitter_test(void * device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char tbuff[2] = { 0 };
	short jitter_max;
	int ret = 0;

	zt_ts_esd_timer_stop(info);
	disable_irq(info->irq);
	sec_cmd_set_default_result(sec);

	if (info->work_state == SUSPEND) {
		input_info(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",__func__);
		goto NG;
	}

	if (write_reg(client, ZT_JITTER_SAMPLING_CNT, 0x0064) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed SAMPLING_CNT\n", __func__);
		goto NG;
	}

	ret = ts_set_touchmode(TOUCH_JITTER_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto NG;
	}

	zt_delay(1500);

	ret = read_data(client, ZT_JITTER_RESULT, tbuff, 2);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		input_err(true, &client->dev,"%s: fail fw_minor_version\n", __func__);
		goto NG;
	}

	jitter_max = tbuff[1] << 8 | tbuff[0];

	ts_set_touchmode(TOUCH_POINT_MODE);

	snprintf(buff, sizeof(buff), "%d", jitter_max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &client->dev, "%s: %s\n", __func__, buff);
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);

	return;
NG:
	snprintf(buff, sizeof(buff), "%s", "NG");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &client->dev, "%s: Failed\n", __func__);
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
	return;
}


static void run_factory_miscalibration(void * device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct zt_ts_platform_data *pdata = info->pdata;
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char tbuff[2] = { 0 };
	char mis_cal_data = 0xF0;
	int ret = 0;
	u16 chip_eeprom_info;

	zt_ts_esd_timer_stop(info);
	disable_irq(info->irq);
	sec_cmd_set_default_result(sec);

	if (pdata->mis_cal_check == 0) {
		input_info(true, &info->client->dev, "%s: [ERROR] not support\n", __func__);
		mis_cal_data = 0xF1;
		goto NG;
	}

	if (info->work_state == SUSPEND) {
		input_info(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",__func__);
		mis_cal_data = 0xF2;
		goto NG;
	}

	if (read_data(info->client, ZT_EEPROM_INFO, (u8 *)&chip_eeprom_info, 2) < 0) {
		input_info(true, &info->client->dev, "%s: read eeprom_info i2c fail!\n", __func__);
		mis_cal_data = 0xF3;
		goto NG;
	}

	if (zinitix_bit_test(chip_eeprom_info, 0)) {
		input_info(true, &info->client->dev, "%s: eeprom cal 0, skip !\n", __func__);
		mis_cal_data = 0xF1;
		goto NG;
	}

	ret = ts_set_touchmode(TOUCH_REF_ABNORMAL_TEST_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		mis_cal_data = 0xF4;
		goto NG;
	}

	ret = write_cmd(client, ZT_MIS_CAL_SET);
	if (ret != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s:failed mis_cal_set\n", __func__);
		mis_cal_data = 0xF4;
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto NG;
	}

	zt_delay(600);

	ret = read_data(client, ZT_MIS_CAL_RESULT, tbuff, 2);
	if (ret < 0) {
		input_err(true, &client->dev,"%s: fail fw_minor_version\n", __func__);
		mis_cal_data = 0xF4;
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto NG;
	}

	ts_set_touchmode(TOUCH_POINT_MODE);

	input_info(true, &info->client->dev, "%s start\n", __func__);

	if (tbuff[1] == 0x55) {
		snprintf(buff, sizeof(buff), "OK,0,%d", tbuff[0]);
	} else if (tbuff[1] == 0xAA) {
		snprintf(buff, sizeof(buff), "NG,0,%d", tbuff[0]);
	} else {
		input_err(true, &client->dev,"%s: failed tbuff[1]:0x%x, tbuff[0]:0x%x \n",
				__func__, tbuff[1], tbuff[0]);
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto NG;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &client->dev, "%s: %s\n", __func__, buff);
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);

	return;
NG:
	snprintf(buff, sizeof(buff), "%s_%d", "NG", mis_cal_data);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &client->dev, "%s: %s\n", __func__, buff);
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
	return;
}

#ifdef TCLM_CONCEPT
static void get_pat_information(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[50] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "C%02XT%04X.%4s%s%c%d%c%d%c%d",
			info->tdata->nvdata.cal_count, info->tdata->nvdata.tune_fix_ver, info->tdata->tclm_string[info->tdata->nvdata.cal_position].f_name,
			(info->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L " : " ",
			info->tdata->cal_pos_hist_last3[0], info->tdata->cal_pos_hist_last3[1],
			info->tdata->cal_pos_hist_last3[2], info->tdata->cal_pos_hist_last3[3],
			info->tdata->cal_pos_hist_last3[4], info->tdata->cal_pos_hist_last3[5]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

/* FACTORY TEST RESULT SAVING FUNCTION
 * bit 3 ~ 0 : OCTA Assy
 * bit 7 ~ 4 : OCTA module
 * param[0] : OCTA module(1) / OCTA Assy(2)
 * param[1] : TEST NONE(0) / TEST FAIL(1) / TEST PASS(2) : 2 bit
 */
static void get_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char cbuff[SEC_CMD_STR_LEN] = { 0 };
	u8 buff[2] = {0};

	sec_cmd_set_default_result(sec);

	get_zt_tsp_nvm_data(info, ZT_TS_NVM_OFFSET_FAC_RESULT, (u8 *)buff, 2);
	info->test_result.data[0] = buff[0];

	input_info(true, &info->client->dev, "%s : %X", __func__, info->test_result.data[0]);

	if (info->test_result.data[0] == 0xFF) {
		input_info(true, &info->client->dev, "%s: clear factory_result as zero\n", __func__);
		info->test_result.data[0] = 0;
	}

	snprintf(cbuff, sizeof(cbuff), "M:%s, M:%d, A:%s, A:%d",
			info->test_result.module_result == 0 ? "NONE" :
			info->test_result.module_result == 1 ? "FAIL" : "PASS",
			info->test_result.module_count,
			info->test_result.assy_result == 0 ? "NONE" :
			info->test_result.assy_result == 1 ? "FAIL" : "PASS",
			info->test_result.assy_count);

	sec_cmd_set_cmd_result(sec, cbuff, strnlen(cbuff, sizeof(cbuff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void set_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char cbuff[SEC_CMD_STR_LEN] = { 0 };
	u8 buff[2] = {0};

	sec_cmd_set_default_result(sec);

	get_zt_tsp_nvm_data(info, ZT_TS_NVM_OFFSET_FAC_RESULT, (u8 *)buff, 2);
	info->test_result.data[0] = buff[0];

	input_info(true, &info->client->dev, "%s : %X", __func__, info->test_result.data[0]);

	if (info->test_result.data[0] == 0xFF) {
		input_info(true, &info->client->dev, "%s: clear factory_result as zero\n", __func__);
		info->test_result.data[0] = 0;
	}

	if (sec->cmd_param[0] == TEST_OCTA_ASSAY) {
		info->test_result.assy_result = sec->cmd_param[1];
		if (info->test_result.assy_count < 3)
			info->test_result.assy_count++;

	} else if (sec->cmd_param[0] == TEST_OCTA_MODULE) {
		info->test_result.module_result = sec->cmd_param[1];
		if (info->test_result.module_count < 3)
			info->test_result.module_count++;
	}

	input_info(true, &info->client->dev, "%s: [0x%X] M:%s, M:%d, A:%s, A:%d\n",
			__func__, info->test_result.data[0],
			info->test_result.module_result == 0 ? "NONE" :
			info->test_result.module_result == 1 ? "FAIL" : "PASS",
			info->test_result.module_count,
			info->test_result.assy_result == 0 ? "NONE" :
			info->test_result.assy_result == 1 ? "FAIL" : "PASS",
			info->test_result.assy_count);

	set_zt_tsp_nvm_data(info, ZT_TS_NVM_OFFSET_FAC_RESULT, &info->test_result.data[0], 1);

	snprintf(cbuff, sizeof(cbuff), "OK");
	sec_cmd_set_cmd_result(sec, cbuff, strnlen(cbuff, sizeof(cbuff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void increase_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 count[2] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &info->client->dev, "%s: IC is power off\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	get_zt_tsp_nvm_data(info, ZT_TS_NVM_OFFSET_DISASSEMBLE_COUNT, count, 2);
	input_info(true, &info->client->dev, "%s: current disassemble count: %d\n", __func__, count[0]);

	if (count[0] == 0xFF)
		count[0] = 0;
	if (count[0] < 0xFE)
		count[0]++;

	set_zt_tsp_nvm_data(info, ZT_TS_NVM_OFFSET_DISASSEMBLE_COUNT, count , 2);

	zt_delay(5);

	memset(count, 0x00, 2);
	get_zt_tsp_nvm_data(info, ZT_TS_NVM_OFFSET_DISASSEMBLE_COUNT, count, 2);
	input_info(true, &info->client->dev, "%s: check disassemble count: %d\n", __func__, count[0]);

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

}

static void get_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 count[2] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &info->client->dev, "%s: IC is power off\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	get_zt_tsp_nvm_data(info, ZT_TS_NVM_OFFSET_DISASSEMBLE_COUNT, count, 2);
	if (count[0] == 0xFF) {
		count[0] = 0;
		count[1] = 0;
		set_zt_tsp_nvm_data(info, ZT_TS_NVM_OFFSET_DISASSEMBLE_COUNT, count , 2);
	}

	input_info(true, &info->client->dev, "%s: read disassemble count: %d\n", __func__, count[0]);
	snprintf(buff, sizeof(buff), "%d", count[0]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_ZT7650)
#define DEF_IUM_ADDR_OFFSET		0xB000
#else
#define DEF_IUM_ADDR_OFFSET		0xF0A0
#endif
#define DEF_IUM_LOCK			0xF0F6
#define DEF_IUM_UNLOCK			0xF0FA
#define DEF_IUM_SAVE_CMD		0xF0F8

int get_zt_tsp_nvm_data(struct zt_ts_info *info, u8 addr, u8 *values, u16 length)
{
	struct i2c_client *client = info->client;
	u16 buff_start;

	zt_ts_esd_timer_stop(info);
	disable_irq(info->irq);

	if (write_reg(client, ZT_POWER_STATE_FLAG, 1) != I2C_SUCCESS) {
		input_info(true, &client->dev, "%s: Fail to set ZT_POWER_STATE_FLAG, 1\n", __func__);
	}
	zt_delay(10);

	if (write_cmd(client, DEF_IUM_LOCK) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed ium lock\n", __func__);
		goto fail_ium_random_read;
	}
	zt_delay(40);

	buff_start = addr;	//custom setting address(0~62, 0,2,4,6)
	//length = 2;		// custom setting(max 64)
	if (length > TC_NVM_SECTOR_SZ)
		length = TC_NVM_SECTOR_SZ;
	if (length < 2) {
		length = 2;	//read 2byte
	}

	if (read_raw_data(client, buff_start + DEF_IUM_ADDR_OFFSET,
				values, length) < 0) {
		input_err(true, &client->dev, "Failed to read raw data %d\n", length);
		goto fail_ium_random_read;
	}

	if (write_cmd(client, DEF_IUM_UNLOCK) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed ium unlock\n", __func__);
		goto fail_ium_random_read;
	}

	if (write_reg(client, ZT_POWER_STATE_FLAG, 0) != I2C_SUCCESS) {
		input_info(true, &client->dev, "%s: Fail to set ZT_POWER_STATE_FLAG, 0\n", __func__);
	}
	zt_delay(10);

	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
	return 0;

fail_ium_random_read:

	zt_power_control(info, POWER_OFF);
	zt_power_control(info, POWER_ON_SEQUENCE);

	mini_init_touch(info);

	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
	return -1;
}

int set_zt_tsp_nvm_data(struct zt_ts_info *info, u8 addr, u8 *values, u16 length)
{
	struct i2c_client *client = info->client;
	u8 buff[64];
	u16 buff_start;

	zt_ts_esd_timer_stop(info);
	disable_irq(info->irq);

	if (write_reg(client, ZT_POWER_STATE_FLAG, 1) != I2C_SUCCESS) {
		input_info(true, &client->dev, "%s: Fail to set ZT_POWER_STATE_FLAG, 1\n", __func__);
	}
	zt_delay(10);

	if (write_cmd(client, DEF_IUM_LOCK) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed ium lock\n", __func__);
		goto fail_ium_random_write;
	}

	buff_start = addr;	//custom setting address(0~62, 0,2,4,6)

	memcpy((u8 *)&buff[buff_start], values, length);

	/* data write start */
	if (length > TC_NVM_SECTOR_SZ)
		length = TC_NVM_SECTOR_SZ;
	if (length < 2) {
		length = 2;	//write 2byte
		buff[buff_start+1] = 0;
	}

	if (write_data(client, buff_start + DEF_IUM_ADDR_OFFSET,
				(u8 *)&buff[buff_start], length) < 0) {
		input_err(true, &client->dev, "%s: error : write zinitix tc firmware\n", __func__);
		goto fail_ium_random_write;
	}
	/* data write end */

	/* for save rom start */
	if (write_reg(client, VCMD_NVM_WRITE_ENABLE, 0x0001) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed to write nvm wp disable\n", __func__);
		goto fail_ium_random_write;
	}
	zt_delay(10);

	if (write_cmd(client, DEF_IUM_SAVE_CMD) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed save ium\n", __func__);
		goto fail_ium_random_write;
	}
	zt_delay(30);

	if (write_reg(client, VCMD_NVM_WRITE_ENABLE, 0x0000) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: nvm wp enable\n", __func__);
		goto fail_ium_random_write;
	}
	zt_delay(10);
	/* for save rom end */

	if (write_cmd(client, DEF_IUM_UNLOCK) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: failed ium unlock\n", __func__);
		goto fail_ium_random_write;
	}

	if (write_reg(client, ZT_POWER_STATE_FLAG, 0) != I2C_SUCCESS) {
		input_info(true, &client->dev, "%s: Fail to set ZT_POWER_STATE_FLAG, 0\n", __func__);
	}
	zt_delay(10);

	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
	return 0;

fail_ium_random_write:

	if (write_reg(client, VCMD_NVM_WRITE_ENABLE, 0x0000) != I2C_SUCCESS) {
		input_err(true, &client->dev, "%s: nvm wp enable\n", __func__);
	}
	zt_delay(10);

	zt_power_control(info, POWER_OFF);
	zt_power_control(info, POWER_ON_SEQUENCE);

	mini_init_touch(info);

	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
	return -1;
}

int zt_tclm_data_read(struct i2c_client *client, int address)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);
	int i, ret = 0;
	u8 nbuff[ZT_TS_NVM_OFFSET_LENGTH];

	switch (address) {
	case SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER:
		ret = ic_version_check(info);
		if (ret < 0) {
			input_err(true, &info->client->dev,"%s: fail to version check\n", __func__);
			return ret;
		}

		return ((info->cap_info.hw_id & 0xff) << 8) | (info->cap_info.reg_data_version & 0xff);

	case SEC_TCLM_NVM_ALL_DATA:
		/* Zinitx driver support index read/write so do not need read FAC_RESULT and DISASSEMBLE_COUNT here
		 * length minus the first 4 bytes
		 */
		ret = get_zt_tsp_nvm_data(info, 4, &nbuff[4], ZT_TS_NVM_OFFSET_LENGTH - 4);
		if (ret < 0)
			return ret;

		info->tdata->nvdata.cal_count = nbuff[ZT_TS_NVM_OFFSET_CAL_COUNT];
		info->tdata->nvdata.tune_fix_ver = (nbuff[ZT_TS_NVM_OFFSET_TUNE_VERSION] << 8) | nbuff[ZT_TS_NVM_OFFSET_TUNE_VERSION + 1];
		info->tdata->nvdata.cal_position = nbuff[ZT_TS_NVM_OFFSET_CAL_POSITION];
		info->tdata->nvdata.cal_pos_hist_cnt = nbuff[ZT_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT];
		info->tdata->nvdata.cal_pos_hist_lastp = nbuff[ZT_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP];
		for (i = ZT_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO; i < ZT_TS_NVM_OFFSET_LENGTH; i++)
			info->tdata->nvdata.cal_pos_hist_queue[i - ZT_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO] = nbuff[i];

		input_err(true, &info->client->dev, "%s: %d %X %x %d %d\n", __func__,
				info->tdata->nvdata.cal_count, info->tdata->nvdata.tune_fix_ver, info->tdata->nvdata.cal_position,
				info->tdata->nvdata.cal_pos_hist_cnt, info->tdata->nvdata.cal_pos_hist_lastp);

		return ret;
	default:
		return ret;
	}
}

int zt_tclm_data_write(struct i2c_client *client, int address)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);
	int i, ret = 1;
	u8 nbuff[ZT_TS_NVM_OFFSET_LENGTH];

	memset(&nbuff[4], 0x00, ZT_TS_NVM_OFFSET_LENGTH - 4);

	nbuff[ZT_TS_NVM_OFFSET_CAL_COUNT] = info->tdata->nvdata.cal_count;
	nbuff[ZT_TS_NVM_OFFSET_TUNE_VERSION] = (u8)(info->tdata->nvdata.tune_fix_ver >> 8);
	nbuff[ZT_TS_NVM_OFFSET_TUNE_VERSION + 1] = (u8)(0xff & info->tdata->nvdata.tune_fix_ver);
	nbuff[ZT_TS_NVM_OFFSET_CAL_POSITION] = info->tdata->nvdata.cal_position;
	nbuff[ZT_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT] = info->tdata->nvdata.cal_pos_hist_cnt;
	nbuff[ZT_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP] = info->tdata->nvdata.cal_pos_hist_lastp;
	for (i = ZT_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO; i < ZT_TS_NVM_OFFSET_LENGTH; i++)
		nbuff[i] = info->tdata->nvdata.cal_pos_hist_queue[i - ZT_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO];

	ret = set_zt_tsp_nvm_data(info, 4, &nbuff[4], ZT_TS_NVM_OFFSET_LENGTH - 4);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] set_tsp_nvm_data ret:%d\n", __func__, ret);
	}

	return ret;
}
#endif

/*
 *	flag     1  :  set edge handler
 *		2  :  set (portrait, normal) edge zone data
 *		4  :  set (portrait, normal) dead zone data
 *		8  :  set landscape mode data
 *		16 :  mode clear
 *	data
 *		0xAA, FFF (y start), FFF (y end),  FF(direction)
 *		0xAB, FFFF (edge zone)
 *		0xAC, FF (up x), FF (down x), FFFF (y)
 *		0xAD, FF (mode), FFF (edge), FFF (dead zone x), FF (dead zone top y), FF (dead zone bottom y)
 *	case
 *		edge handler set :  0xAA....
 *		booting time :  0xAA...  + 0xAB...
 *		normal mode : 0xAC...  (+0xAB...)
 *		landscape mode : 0xAD...
 *		landscape -> normal (if same with old data) : 0xAD, 0
 *		landscape -> normal (etc) : 0xAC....  + 0xAD, 0
 */

static void set_grip_data_to_ic(struct zt_ts_info *ts, u8 flag)
{
	struct i2c_client *client = ts->client;

	input_info(true, &ts->client->dev, "%s: flag: %02X (clr,lan,nor,edg,han)\n", __func__, flag);

	mutex_lock(&ts->power_init);

	if (flag & G_SET_EDGE_HANDLER) {
		if (ts->grip_edgehandler_direction == 0) {
			ts->grip_edgehandler_start_y = 0x0;
			ts->grip_edgehandler_end_y = 0x0;
		}

		if (write_reg(client, ZT_EDGE_REJECT_PORT_EDGE_EXCEPT_START,
					ts->grip_edgehandler_start_y) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set except start y error\n", __func__);
		}

		if (write_reg(client, ZT_EDGE_REJECT_PORT_EDGE_EXCEPT_END,
					ts->grip_edgehandler_end_y) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set except end y error\n", __func__);
		}

		if (write_reg(client, ZT_EDGE_REJECT_PORT_EDGE_EXCEPT_SEL,
					(ts->grip_edgehandler_direction) & 0x0003) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set except direct error\n", __func__);
		}

		input_info(true, &ts->client->dev, "%s: 0x%02X %02X, 0x%02X %02X, 0x%02X %02X\n", __func__,
				ZT_EDGE_REJECT_PORT_EDGE_EXCEPT_SEL, ts->grip_edgehandler_direction,
				ZT_EDGE_REJECT_PORT_EDGE_EXCEPT_START, ts->grip_edgehandler_start_y,
				ZT_EDGE_REJECT_PORT_EDGE_EXCEPT_END, ts->grip_edgehandler_end_y);
	}

	if (flag & G_SET_EDGE_ZONE) {
		if (write_reg(client, ZT_EDGE_GRIP_PORT_SIDE_WIDTH, ts->grip_edge_range) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set grip side width error\n", __func__);
		}

		input_info(true, &ts->client->dev, "%s: 0x%02X %02X\n", __func__,
				ZT_EDGE_GRIP_PORT_SIDE_WIDTH, ts->grip_edge_range);
	}

	if (flag & G_SET_NORMAL_MODE) {
		if (write_reg(client, ZT_EDGE_REJECT_PORT_SIDE_UP_WIDTH, ts->grip_deadzone_up_x) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set dead zone up x error\n", __func__);
		}

		if (write_reg(client, ZT_EDGE_REJECT_PORT_SIDE_DOWN_WIDTH, ts->grip_deadzone_dn_x) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set dead zone down x error\n", __func__);
		}

		if (write_reg(client, ZT_EDGE_REJECT_PORT_SIDE_UP_DOWN_DIV, ts->grip_deadzone_y) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set dead zone up/down div location error\n", __func__);
		}

		input_info(true, &ts->client->dev, "%s: 0x%02X %02X, 0x%02X %02X, 0x%02X %02X\n", __func__,
				ZT_EDGE_REJECT_PORT_SIDE_UP_WIDTH, ts->grip_deadzone_up_x,
				ZT_EDGE_REJECT_PORT_SIDE_DOWN_WIDTH, ts->grip_deadzone_dn_x,
				ZT_EDGE_REJECT_PORT_SIDE_UP_DOWN_DIV, ts->grip_deadzone_y);
	}

	if (flag & G_SET_LANDSCAPE_MODE) {
		if (write_reg(client, ZT_EDGE_LANDSCAPE_MODE, ts->grip_landscape_mode & 0x1) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set landscape mode error\n", __func__);
		}

		if (write_reg(client, ZT_EDGE_GRIP_LAND_SIDE_WIDTH, ts->grip_landscape_edge) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set landscape side edge error\n", __func__);
		}

		if (write_reg(client, ZT_EDGE_REJECT_LAND_SIDE_WIDTH, ts->grip_landscape_deadzone) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set landscape side deadzone error\n", __func__);
		}

		if (write_reg(client, ZT_EDGE_REJECT_LAND_TOP_BOT_WIDTH,
					(((ts->grip_landscape_top_deadzone << 8) & 0xFF00) | (ts->grip_landscape_bottom_deadzone & 0x00FF)))
				!= I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set landscape top bot deazone error\n", __func__);
		}

		if (write_reg(client, ZT_EDGE_GRIP_LAND_TOP_BOT_WIDTH,
					(((ts->grip_landscape_top_gripzone << 8) & 0xFF00) | (ts->grip_landscape_bottom_gripzone & 0x00FF)))
				!= I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: set landscape top bot gripzone error\n", __func__);
		}

		input_info(true, &ts->client->dev,
				"%s: 0x%02X %02X, 0x%02X %02X, 0x%02X %02X, 0x%02X %02X, 0x%02X %02X\n", __func__,
				ZT_EDGE_LANDSCAPE_MODE, ts->grip_landscape_mode & 0x1,
				ZT_EDGE_GRIP_LAND_SIDE_WIDTH, ts->grip_landscape_edge,
				ZT_EDGE_REJECT_LAND_SIDE_WIDTH, ts->grip_landscape_deadzone,
				ZT_EDGE_REJECT_LAND_TOP_BOT_WIDTH,
				((ts->grip_landscape_top_deadzone << 8) & 0xFF00) | (ts->grip_landscape_bottom_deadzone & 0x00FF),
				ZT_EDGE_GRIP_LAND_TOP_BOT_WIDTH,
				((ts->grip_landscape_top_gripzone << 8) & 0xFF00) | (ts->grip_landscape_bottom_gripzone & 0x00FF));
	}

	if (flag & G_CLR_LANDSCAPE_MODE) {
		if (write_reg(client, ZT_EDGE_LANDSCAPE_MODE, ts->grip_landscape_mode) != I2C_SUCCESS) {
			input_err(true, &client->dev, "%s: clr landscape mode error\n", __func__);
		}

		input_info(true, &ts->client->dev, "%s: 0x%02X %02X\n", __func__,
				ZT_EDGE_LANDSCAPE_MODE, ts->grip_landscape_mode);
	}
	mutex_unlock(&ts->power_init);
}

/*
 *	index  0 :  set edge handler
 *		1 :  portrait (normal) mode
 *		2 :  landscape mode
 *
 *	data
 *		0, X (direction), X (y start), X (y end)
 *		direction : 0 (off), 1 (left), 2 (right)
 *			ex) echo set_grip_data,0,2,600,900 > cmd
 *
 *		1, X (edge zone), X (dead zone up x), X (dead zone down x), X (dead zone y)
 *			ex) echo set_grip_data,1,200,10,50,1500 > cmd
 *
 *		2, 1 (landscape mode), X (edge zone), X (dead zone x), X (dead zone top y), X (dead zone bottom y), X (edge zone top y), X (edge zone bottom y)
 *			ex) echo set_grip_data,2,1,200,100,120,0 > cmd
 *
 *		2, 0 (portrait mode)
 *			ex) echo set_grip_data,2,0  > cmd
 */

static void set_grip_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *ts = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 mode = G_NONE;

	sec_cmd_set_default_result(sec);

	memset(buff, 0, sizeof(buff));

	if (sec->cmd_param[0] == 0) {	// edge handler
		if (sec->cmd_param[1] == 0) {	// clear
			ts->grip_edgehandler_direction = 0;
		} else if (sec->cmd_param[1] < 3) {
			ts->grip_edgehandler_direction = sec->cmd_param[1];
			ts->grip_edgehandler_start_y = sec->cmd_param[2];
			ts->grip_edgehandler_end_y = sec->cmd_param[3];
		} else {
			input_err(true, &ts->client->dev, "%s: cmd1 is abnormal, %d (%d)\n",
					__func__, sec->cmd_param[1], __LINE__);
			goto err_grip_data;
		}

		mode = mode | G_SET_EDGE_HANDLER;
		set_grip_data_to_ic(ts, mode);
	} else if (sec->cmd_param[0] == 1) {	// normal mode
		if (ts->grip_edge_range != sec->cmd_param[1])
			mode = mode | G_SET_EDGE_ZONE;

		ts->grip_edge_range = sec->cmd_param[1];
		ts->grip_deadzone_up_x = sec->cmd_param[2];
		ts->grip_deadzone_dn_x = sec->cmd_param[3];
		ts->grip_deadzone_y = sec->cmd_param[4];
		mode = mode | G_SET_NORMAL_MODE;

		if (ts->grip_landscape_mode == 1) {
			ts->grip_landscape_mode = 0;
			mode = mode | G_CLR_LANDSCAPE_MODE;
		}
		set_grip_data_to_ic(ts, mode);
	} else if (sec->cmd_param[0] == 2) {	// landscape mode
		if (sec->cmd_param[1] == 0) {	// normal mode
			ts->grip_landscape_mode = 0;
			mode = mode | G_CLR_LANDSCAPE_MODE;
		} else if (sec->cmd_param[1] == 1) {
			ts->grip_landscape_mode = 1;
			ts->grip_landscape_edge = sec->cmd_param[2];
			ts->grip_landscape_deadzone	= sec->cmd_param[3];
			ts->grip_landscape_top_deadzone = sec->cmd_param[4];
			ts->grip_landscape_bottom_deadzone = sec->cmd_param[5];
			ts->grip_landscape_top_gripzone = sec->cmd_param[6];
			ts->grip_landscape_bottom_gripzone = sec->cmd_param[7];
			mode = mode | G_SET_LANDSCAPE_MODE;
		} else {
			input_err(true, &ts->client->dev, "%s: cmd1 is abnormal, %d (%d)\n",
					__func__, sec->cmd_param[1], __LINE__);
			goto err_grip_data;
		}
		set_grip_data_to_ic(ts, mode);
	} else {
		input_err(true, &ts->client->dev, "%s: cmd0 is abnormal, %d", __func__, sec->cmd_param[0]);
		goto err_grip_data;
	}

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

err_grip_data:

	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

void zt_set_grip_type(struct zt_ts_info *ts, u8 set_type)
{
	u8 mode = G_NONE;

	input_info(true, &ts->client->dev, "%s: re-init grip(%d), edh:%d, edg:%d, lan:%d\n", __func__,
			set_type, ts->grip_edgehandler_direction, ts->grip_edge_range, ts->grip_landscape_mode);

	/* edge handler */
	if (ts->grip_edgehandler_direction != 0)
		mode |= G_SET_EDGE_HANDLER;

	if (set_type == GRIP_ALL_DATA) {
		/* edge */
		if (ts->grip_edge_range != 60)
			mode |= G_SET_EDGE_ZONE;

		/* dead zone */
		if (ts->grip_landscape_mode == 1)	/* default 0 mode, 32 */
			mode |= G_SET_LANDSCAPE_MODE;
		else
			mode |= G_SET_NORMAL_MODE;
	}

	if (mode)
		set_grip_data_to_ic(ts, mode);
}

static void set_touchable_area(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	input_info(true, &info->client->dev,
			"%s: set 16:9 mode %s\n", __func__, sec->cmd_param[0] ? "enable" : "disable");

	zt_set_optional_mode(info, DEF_OPTIONAL_MODE_TOUCHABLE_AREA, sec->cmd_param[0]);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void clear_cover_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int arg = sec->cmd_param[0];

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%u", (unsigned int) arg);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0] > 1) {
			info->flip_enable = true;
			info->cover_type = sec->cmd_param[1];
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
			zt_delay(100);
			if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
				tui_force_close(1);
				zt_delay(100);
				if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
					trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
					trustedui_set_mode(TRUSTEDUI_MODE_OFF);
				}
			}
#endif // CONFIG_TRUSTONIC_TRUSTED_UI
#ifdef CONFIG_SAMSUNG_TUI
			stui_cancel_session();
#endif
		} else {
			info->flip_enable = false;
		}

		set_cover_type(info, info->flip_enable);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	return;
}

static void clear_reference_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	zt_ts_esd_timer_stop(info);

	write_reg(client, ZT_EEPROM_INFO, 0xffff);

	write_reg(client, VCMD_NVM_WRITE_ENABLE, 0x0001);
	usleep_range(100, 100);
	if (write_cmd(client, ZT_SAVE_STATUS_CMD) != I2C_SUCCESS)
		return;

	zt_delay(500);
	write_reg(client, VCMD_NVM_WRITE_ENABLE, 0x0000);
	usleep_range(100, 100);

	zt_ts_esd_timer_start(info);
	input_info(true, &client->dev, "%s: TSP clear calibration bit\n", __func__);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__,
			sec->cmd_result, (int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));
	return;
}

int zt_tclm_execute_force_calibration(struct i2c_client *client, int cal_mode)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);

	if (ts_hw_calibration(info) == false)
		return -1;

	return 0;
}

static void ts_enter_strength_mode(struct zt_ts_info *info, int testnum)
{
	struct i2c_client *client = info->client;
	u8 i;

	mutex_lock(&info->work_lock);
	if (info->work_state != NOTHING) {
		input_info(true, &info->client->dev, "other process occupied.. (%d)\n", info->work_state);
		mutex_unlock(&info->work_lock);
		return;
	}

	info->touch_mode = testnum;

	write_cmd(client, 0x0A);
	usleep_range(20 * 1000, 20 * 1000);
	write_cmd(client, 0x0A);
	usleep_range(20 * 1000, 20 * 1000);

	if (testnum == TOUCH_DELTA_MODE) {
		input_info(true, &info->client->dev, "%s: shorten delay raw for host\n", __func__);
		if (write_reg(client, ZT_DELAY_RAW_FOR_HOST, RAWDATA_DELAY_FOR_HOST / 5) != I2C_SUCCESS) {
			input_info(true, &client->dev, "%s: Fail to delay_raw_for_host enter\n", __func__);
			mutex_unlock(&info->work_lock);
			return;
		}
	}

	if (write_reg(client, ZT_POWER_STATE_FLAG, 1) != I2C_SUCCESS) {
		input_info(true, &client->dev, "%s: Fail to set ZT_POWER_STATE_FLAG 1\n", __func__);
		mutex_unlock(&info->work_lock);
		return;
	}
	zt_delay(10);

	if (write_reg(client, ZT_TOUCH_MODE, testnum) != I2C_SUCCESS) {
		input_info(true, &client->dev, "%s: Fail to set ZINITX_TOUCH_MODE %d\n", __func__, testnum);
		mutex_unlock(&info->work_lock);
		return;
	}

	/* clear garbage data */
	for (i = 0; i < 10; i++) {
		usleep_range(20 * 1000, 20 * 1000);
		write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
	}

	clear_report_data(info);

	input_info(true, &client->dev, "%s: Enter_strength_mode\n", __func__);

	mutex_unlock(&info->work_lock);
}

static void ts_exit_strength_mode(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	u8 i;

	mutex_lock(&info->work_lock);
	if (info->work_state != NOTHING) {
		input_info(true, &info->client->dev, "other process occupied.. (%d)\n", info->work_state);
		mutex_unlock(&info->work_lock);
		return;
	}

	if (write_reg(client, ZT_TOUCH_MODE, TOUCH_POINT_MODE) != I2C_SUCCESS) {
		input_info(true, &client->dev, "[zinitix_touch] TEST Mode : "
				"Fail to set ZINITX_TOUCH_MODE %d.\r\n", TOUCH_POINT_MODE);
		mutex_unlock(&info->work_lock);
		return;
	}
	if (write_reg(client, ZT_POWER_STATE_FLAG, 0) != I2C_SUCCESS) {
		input_info(true, &client->dev, "%s: Fail to set ZT_POWER_STATE_FLAG 0\n", __func__);
		mutex_unlock(&info->work_lock);
		return;
	}
	zt_delay(10);

	if (info->touch_mode == TOUCH_DELTA_MODE) {
		input_info(true, &info->client->dev, "%s: restore delay raw for host\n", __func__);
		if (write_reg(client, ZT_DELAY_RAW_FOR_HOST, RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS) {
			input_info(true, &client->dev, "%s: Fail to delay_raw_for_host exit\n", __func__);
			mutex_unlock(&info->work_lock);
			return;
		}
	}

	/* clear garbage data */
	for (i = 0; i < 10; i++) {
		usleep_range(20 * 1000, 20 * 1000);
		write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
	}

	clear_report_data(info);
	input_info(true, &client->dev, "%s\n", __func__);

	info->touch_mode = TOUCH_POINT_MODE;
	mutex_unlock(&info->work_lock);
}

static void ts_get_strength_data(struct zt_ts_info *info)
{
	struct i2c_client *client = info->client;
	int i, j, n;
	u8 ref_max[2] = {0, 0};

	mutex_lock(&info->raw_data_lock);
	read_data(info->client, 0x0308, ref_max, 2);

	input_info(true, &client->dev, "reference max: %X %X\n", ref_max[0], ref_max[1]);

	n = 0;
	for (i = 0 ; i < info->cap_info.y_node_num; i++) {
		pr_info("%s %d |", SECLOG, i);
		for (j = 0 ; j < info->cap_info.x_node_num; j++, n++) {
			pr_cont(" %d", info->cur_data[n]);
		}
		pr_cont("\n");
	}
	mutex_unlock(&info->raw_data_lock);
}

static void run_cs_raw_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int retry = 0;
	char *all_cmdbuff;
	s32 i, j;

	sec_cmd_set_default_result(sec);

	disable_irq(info->irq);

	ts_enter_strength_mode(info, TOUCH_RAW_MODE);

	while (gpio_get_value(info->pdata->gpio_int)) {
		zt_delay(30);

		retry++;

		input_info(true, &client->dev, "%s: retry:%d\n", __func__, retry);

		if (retry > 100) {
			enable_irq(info->irq);
			goto out;
		}
	}
	ts_get_raw_data(info);

	ts_exit_strength_mode(info);

	enable_irq(info->irq);

	ts_get_strength_data(info);

	all_cmdbuff = kzalloc(info->cap_info.x_node_num * info->cap_info.y_node_num * 6, GFP_KERNEL);
	if (!all_cmdbuff) {
		input_info(true, &info->client->dev, "%s: alloc failed\n", __func__);
		goto out;
	}

	for (i = 0; i < info->cap_info.y_node_num; i++) {
		for (j = 0; j < info->cap_info.x_node_num; j++) {
			sprintf(buff, "%d,", info->cur_data[i * info->cap_info.x_node_num + j]);
			strcat(all_cmdbuff, buff);
		}
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, all_cmdbuff,
			strnlen(all_cmdbuff, sizeof(all_cmdbuff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(all_cmdbuff);

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__,
			sec->cmd_result, (int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));
	return;

out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__,
			sec->cmd_result, (int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

}

static void run_cs_delta_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int retry = 0;
	char *all_cmdbuff;
	s32 i, j;

	sec_cmd_set_default_result(sec);

	zt_ts_esd_timer_stop(info);
	disable_irq(info->irq);

	ts_enter_strength_mode(info, TOUCH_DELTA_MODE);

	while (gpio_get_value(info->pdata->gpio_int)) {
		zt_delay(30);

		retry++;

		input_info(true, &client->dev, "%s: retry:%d\n", __func__, retry);

		if (retry > 100) {
			enable_irq(info->irq);
			zt_ts_esd_timer_start(info);
			goto out;
		}
	}
	ts_get_raw_data(info);

	ts_exit_strength_mode(info);

	enable_irq(info->irq);

	zt_ts_esd_timer_start(info);

	ts_get_strength_data(info);

	all_cmdbuff = kzalloc(info->cap_info.x_node_num * info->cap_info.y_node_num * 6, GFP_KERNEL);
	if (!all_cmdbuff) {
		input_info(true, &info->client->dev, "%s: alloc failed\n", __func__);
		goto out;
	}

	for (i = 0; i < info->cap_info.y_node_num; i++) {
		for (j = 0; j < info->cap_info.x_node_num; j++) {
			sprintf(buff, "%d,", info->cur_data[i * info->cap_info.x_node_num + j]);
			strcat(all_cmdbuff, buff);
		}
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, all_cmdbuff,
			strnlen(all_cmdbuff, sizeof(all_cmdbuff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(all_cmdbuff);

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__,
			sec->cmd_result, (int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));
	return;

out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__,
			sec->cmd_result, (int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));
}

static void run_ref_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int i;
#ifdef TCLM_CONCEPT
	int ret;
#endif
	sec_cmd_set_default_result(sec);

	if (info->finger_cnt1 != 0) {
		input_info(true, &client->dev, "%s: return (finger cnt %d)\n", __func__, info->finger_cnt1);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	disable_irq(info->irq);

	zt_ts_esd_timer_stop(info);
	zt_power_control(info, POWER_OFF);
	zt_power_control(info, POWER_ON_SEQUENCE);

	if (ts_hw_calibration(info) == true) {
#ifdef TCLM_CONCEPT
		/* devide tclm case */
		sec_tclm_case(info->tdata, sec->cmd_param[0]);

		input_info(true, &info->client->dev, "%s: param, %d, %c, %d\n", __func__,
				sec->cmd_param[0], sec->cmd_param[0], info->tdata->root_of_calibration);

		ret = sec_execute_tclm_package(info->tdata, 1);
		if (ret < 0) {
			input_err(true, &info->client->dev,
					"%s: sec_execute_tclm_package\n", __func__);
		}
		sec_tclm_root_of_cal(info->tdata, CALPOSITION_NONE);
#endif
		input_info(true, &client->dev, "%s: TSP calibration Pass\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec_cmd_set_cmd_result(sec, buff, (int)strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
	} else {
		input_info(true, &client->dev, "%s: TSP calibration Fail\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, (int)strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	zt_power_control(info, POWER_OFF);
	zt_power_control(info, POWER_ON_SEQUENCE);
	mini_init_touch(info);

	for (i = 0; i < 5; i++) {
		write_cmd(client, ZT_CLEAR_INT_STATUS_CMD);
		usleep_range(10, 10);
	}

	clear_report_data(info);

	zt_ts_esd_timer_start(info);
	enable_irq(info->irq);
	input_info(true, &client->dev, "%s: %s(%d)\n", __func__,
			sec->cmd_result, (int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));
	return;
}

static void run_amp_check_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	struct tsp_raw_data *raw_data = info->raw_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int total_node = info->cap_info.y_node_num;
	u16 min, max;
	s32 i;
	int ret;

	zt_ts_esd_timer_stop(info);
	sec_cmd_set_default_result(sec);

	ret = ts_set_touchmode(TOUCH_AMP_CHECK_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	get_raw_data(info, (u8 *)raw_data->amp_check_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	min = 0xFFFF;
	max = 0x0000;

	for (i = 0; i < total_node; i++) {
		if (raw_data->amp_check_data[i] < min)
			min = raw_data->amp_check_data[i];

		if (raw_data->amp_check_data[i] > max)
			max = raw_data->amp_check_data[i];
	}

	input_info(true, &client->dev, "%s: amp check data min:%d, max:%d\n", __func__, min, max);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "AMP_CHECK");
	}

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strlen(sec->cmd_result));

	zt_ts_esd_timer_start(info);
	return;
}

static void dead_zone_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EDGE_SELECT, !sec->cmd_param[0]);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;
}

static void spay_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->spay_enable = !!sec->cmd_param[0];

	zt_set_lp_mode(info, ZT_SPONGE_MODE_SPAY, info->spay_enable);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;
}

static void fod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	input_info(true, &info->client->dev, "%s: fod_enable:%d, short_mode:%d, strict mode:%d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],	sec->cmd_param[2]);

	info->fod_enable = !!sec->cmd_param[0];
	info->fod_mode_set = (sec->cmd_param[1] & 0x01) | ((sec->cmd_param[2] & 0x01) << 1);

	zt_set_lp_mode(info, ZT_SPONGE_MODE_PRESS, info->fod_enable);
	ret = ts_write_to_sponge(info, ZT_SPONGE_FOD_PROPERTY, (u8 *)&info->fod_mode_set, 2);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: Failed to write sponge\n", __func__);

	input_info(true, &info->client->dev, "%s: %s, fast:%s, strict:%s, %02X\n",
			__func__, sec->cmd_param[0] ? "on" : "off",
			info->fod_mode_set & 1 ? "on" : "off",
			info->fod_mode_set & 2 ? "on" : "off",
			info->lpm_mode);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;
}

static void fod_lp_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int val = sec->cmd_param[0];

	sec_cmd_set_default_result(sec);

	if (val) {
		info->fod_lp_mode = 1;
	} else {
		info->fod_lp_mode = 0;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;
}

static void singletap_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->singletap_enable = !!sec->cmd_param[0];

	zt_set_lp_mode(info, ZT_SPONGE_MODE_SINGLETAP, info->singletap_enable);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;
}

static void aot_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->aot_enable = !!sec->cmd_param[0];

	zt_set_lp_mode(info, ZT_SPONGE_MODE_DOUBLETAP_WAKEUP, info->aot_enable);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;
}

static void aod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->aod_enable = !!sec->cmd_param[0];

	zt_set_lp_mode(info, ZT_SPONGE_MODE_AOD, info->aod_enable);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;
}

static void set_fod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int i;
	int ret;

	sec_cmd_set_default_result(sec);

	input_info(true, &info->client->dev, "%s: start x:%d, start y:%d, end x:%d, end y:%d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2], sec->cmd_param[3]);

	for (i = 0; i < 4; i++)
		info->fod_rect[i] = sec->cmd_param[i] & 0xFFFF;

	ret = zt_set_fod_rect(info);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto error;
	}
	
	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;

error:
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void set_aod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int i;
	int ret;

	sec_cmd_set_default_result(sec);

	input_info(true, &info->client->dev, "%s: w:%d, h:%d, x:%d, y:%d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2], sec->cmd_param[3]);

	for (i = 0; i < 4; i++)
		info->aod_rect[i] = sec->cmd_param[i];

	ret = zt_set_aod_rect(info);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto error;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;

error:
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);		
}

static void get_wet_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u16 temp;

	sec_cmd_set_default_result(sec);

	mutex_lock(&info->work_lock);
	read_data(client, ZT_DEBUG_REG, (u8 *)&temp, 2);
	mutex_unlock(&info->work_lock);

	input_info(true, &client->dev, "%s, %x\n", __func__, temp);

	if (zinitix_bit_test(temp, DEF_DEVICE_STATUS_WATER_MODE))
		temp = true;
	else
		temp = false;

	snprintf(buff, sizeof(buff), "%u", temp);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "WET_MODE");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

static void glove_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		zt_set_optional_mode(info, DEF_OPTIONAL_MODE_SENSITIVE_BIT, sec->cmd_param[0]);
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

static void pocket_mode_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->pocket_enable = sec->cmd_param[0];

		zt_set_optional_mode(info, DEF_OPTIONAL_MODE_POCKET_MODE, info->pocket_enable);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;
}

static void set_sip_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		mutex_lock(&info->power_init);
		write_reg(client, ZT_SET_SIP_MODE, (u8)sec->cmd_param[0]);
		mutex_unlock(&info->power_init);
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &client->dev, "%s: %s(%d)\n", __func__, sec->cmd_result,
			(int)strnlen(sec->cmd_result, sizeof(sec->cmd_result)));

	return;
}

static void get_crc_check(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u16 chip_check_sum = 0;

	sec_cmd_set_default_result(sec);

	if (read_data(info->client, ZT_CHECKSUM_RESULT,
				(u8 *)&chip_check_sum, 2) < 0) {
		input_err(true, &info->client->dev, "%s: read crc fail", __func__);
		goto err_get_crc_check;
	}

	input_info(true, &info->client->dev, "%s: 0x%04X\n", __func__, chip_check_sum);

	if (chip_check_sum != CORRECT_CHECK_SUM)
		goto err_get_crc_check;

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;

err_get_crc_check:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;
}

static void run_test_vsync(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buf[64] = { 0 };
	u16 data = 0;

	sec_cmd_set_default_result(sec);

	if (write_reg(info->client, ZT_POWER_STATE_FLAG, 1) != I2C_SUCCESS) {
		input_err(true, &info->client->dev, "%s: Fail to set ZT_POWER_STATE_FLAG 1\n", __func__);
		snprintf(buf, sizeof(buf), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	zt_delay(100);
	if (read_data(info->client, ZT_VSYNC_TEST_RESULT, (u8 *)&data, 2) < 0) {
		input_err(true, &info->client->dev, "%s: read crc fail", __func__);
		snprintf(buf, sizeof(buf), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	input_info(true, &info->client->dev, "%s: result %d\n", __func__, data);

	snprintf(buf, sizeof(buf), "%d", data);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	if (write_reg(info->client, ZT_POWER_STATE_FLAG, 0) != I2C_SUCCESS)
		input_err(true, &info->client->dev, "%s: Fail to set ZT_POWER_STATE_FLAG 0\n", __func__);

	zt_delay(10);
	
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buf, strnlen(buf, sizeof(buf)), "VSYNC");
}

static void read_osc_value(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u16 data[2] = {0};
	u32 osc_timer_val;

	sec_cmd_set_default_result(sec);

	ret = read_data(info->client, ZT_OSC_TIMER_LSB, (u8 *)&data[0], 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,"%s: fail read proximity threshold\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto ERROR;
	}

	ret = read_data(info->client, ZT_OSC_TIMER_MSB, (u8 *)&data[1], 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,"%s: fail read proximity threshold\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto ERROR;
	}

	osc_timer_val = (data[1] << 16) | data[0];

	input_info(true, &info->client->dev,
					"%s: osc_timer_value %08X\n", __func__, osc_timer_val);

	snprintf(buff, sizeof(buff), "%u,%u", osc_timer_val, osc_timer_val);
	sec->cmd_state = SEC_CMD_STATUS_OK;

ERROR:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "OSC_DATA");
	return;
}

static void factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;

	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &info->client->dev, "%s: IC is power off\n", __func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	get_chip_vendor(sec);
	get_chip_name(sec);
	get_fw_ver_bin(sec);
	get_fw_ver_ic(sec);

	run_dnd_read(sec);
	run_dnd_v_gap_read(sec);
	run_dnd_h_gap_read(sec);
	run_cnd_read(sec);
	run_selfdnd_read(sec);
	run_selfdnd_h_gap_read(sec);
	run_self_saturation_read(sec);
	run_charge_pump_read(sec);
	run_test_vsync(sec);
	read_osc_value(sec);
	run_amp_check_read(sec);

#ifdef TCLM_CONCEPT
	run_mis_cal_read(sec);
#endif

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

out:
	input_info(true, &client->dev, "%s: %d%s\n", __func__, sec->item_count,
			sec->cmd_result_all);
}

static void check_connection(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[3] = { 0 };
	u8 conn_check_val;
	int ret;

	sec_cmd_set_default_result(sec);

	ret = read_data(client, ZT_CONNECTION_CHECK_REG, (u8 *)&conn_check_val, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev,"%s: fail read TSP connection value\n", __func__);
		goto err_conn_check;
	}

	if (conn_check_val != 1)
		goto err_conn_check;

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;

err_conn_check:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;
}

static void run_prox_intensity_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u16 prox_x_data, prox_y_data;
	u16 threshold;
	int ret;

	sec_cmd_set_default_result(sec);

	zt_ts_esd_timer_stop(info);
	disable_irq(info->irq);
	mutex_lock(&info->work_lock);

	ret = read_data(client, ZT_PROXIMITY_XDATA, (u8 *)&prox_x_data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,"%s: fail read proximity x data\n", __func__);
		goto READ_FAIL;
	}

	ret = read_data(client, ZT_PROXIMITY_YDATA, (u8 *)&prox_y_data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,"%s: fail read proximity y data\n", __func__);
		goto READ_FAIL;
	}

	ret = read_data(client, ZT_PROXIMITY_THRESHOLD, (u8 *)&threshold, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,"%s: fail read proximity threshold\n", __func__);
		goto READ_FAIL;
	}

	mutex_unlock(&info->work_lock);
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);

	snprintf(buff, sizeof(buff), "SUM_X:%d SUM_Y:%d THD_X:%d THD_Y:%d", prox_x_data, prox_y_data, threshold, threshold);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;

READ_FAIL:
	mutex_unlock(&info->work_lock);
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);

	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;
}

static void ear_detect_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] != 0 && sec->cmd_param[0] != 1 && sec->cmd_param[0] != 3) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->ed_enable = sec->cmd_param[0];

		if (info->ed_enable == 3) {
			zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT, true);
			zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT_MUTUAL, false);
		} else if (info->ed_enable == 1) {
			zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT, false);
			zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT_MUTUAL, true);
		} else {
			zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT, false);
			zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT_MUTUAL, false);
		}

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &client->dev, "%s, %s\n", __func__, sec->cmd_result);

	return;
}

/*	for game mode 
	byte[0]: Setting for the Game Mode with 240Hz scan rate
		- 0: Disable
		- 1: Enable

	byte[1]: Vsycn mode
		- 0: Normal 60
		- 1: HS60
		- 2: HS120
		- 3: VSYNC 48
		- 4: VSYNC 96 
*/
static void set_scan_rate(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char tBuff[2] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1 ||
			sec->cmd_param[1] < 0 || sec->cmd_param[1] > 4) {
		input_err(true, &info->client->dev, "%s: not support param\n", __func__);
		goto NG;
	}

	tBuff[0] = sec->cmd_param[0];
	tBuff[1] = sec->cmd_param[1];

	mutex_lock(&info->power_init);
	if (write_reg(info->client, ZT_SET_SCANRATE_ENABLE, tBuff[0]) != I2C_SUCCESS) {
		input_err(true, &info->client->dev,
				"%s: failed to set scan mode enable\n", __func__);
		mutex_unlock(&info->power_init);
		goto NG;
	}

	if (write_reg(info->client, ZT_SET_SCANRATE, tBuff[1]) != I2C_SUCCESS) {
		input_err(true, &info->client->dev,
				"%s: failed to set scan rate\n", __func__);
		mutex_unlock(&info->power_init);
		goto NG;
	}
	mutex_unlock(&info->power_init);

	input_info(true, &info->client->dev,
					"%s: set scan rate %d %d\n", __func__, tBuff[0], tBuff[1]);

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

NG:
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void set_wirelesscharger_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &info->client->dev, "%s: not support param\n", __func__);
		goto NG;
	}

	mutex_lock(&info->power_init);
	if (write_reg(info->client, ZT_SET_WIRELESSCHARGER_MODE, (u8)sec->cmd_param[0]) != I2C_SUCCESS) {
		input_err(true, &info->client->dev,
				"%s: failed to set scan mode enable\n", __func__);
		mutex_unlock(&info->power_init);
		goto NG;
	}
	mutex_unlock(&info->power_init);

	input_info(true, &info->client->dev,
					"%s: set wireless charger mode %d\n", __func__, sec->cmd_param[0]);

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

NG:
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void set_note_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &info->client->dev, "%s: not support param\n", __func__);
		goto NG;
	}

	mutex_lock(&info->power_init);
	if (write_reg(info->client, ZT_SET_NOTE_MODE, (u8)sec->cmd_param[0]) != I2C_SUCCESS) {
		input_err(true, &info->client->dev,
				"%s: failed to set scan mode enable\n", __func__);
		mutex_unlock(&info->power_init);
		goto NG;
	}
	mutex_unlock(&info->power_init);

	input_info(true, &info->client->dev,
					"%s: set note mode %d\n", __func__, sec->cmd_param[0]);

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

NG:
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void set_game_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char tBuff[2] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &info->client->dev, "%s: not support param\n", __func__);
		goto NG;
	}

	tBuff[0] = sec->cmd_param[0];

	mutex_lock(&info->power_init);
	if (write_reg(info->client, ZT_SET_GAME_MODE, tBuff[0]) != I2C_SUCCESS) {
		input_err(true, &info->client->dev,
				"%s: failed to set scan mode enable\n", __func__);
		mutex_unlock(&info->power_init);
		goto NG;
	}
	mutex_unlock(&info->power_init);

	input_info(true, &info->client->dev,
					"%s: set game mode %d\n", __func__, tBuff[0]);

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

NG:
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static ssize_t scrub_position_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	input_info(true, &client->dev, "%s: scrub_id: %d, X:%d, Y:%d \n", __func__,
			info->scrub_id, info->scrub_x, info->scrub_y);

	snprintf(buff, sizeof(buff), "%d %d %d", info->scrub_id, info->scrub_x, info->scrub_y);

	info->scrub_id = 0;
	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t sensitivity_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	s16 i, value[TOUCH_SENTIVITY_MEASUREMENT_COUNT];
	char buff[SEC_CMD_STR_LEN] = { 0 };

	for (i = 0; i < TOUCH_SENTIVITY_MEASUREMENT_COUNT; i++) {
		value[i] = info->sensitivity_data[i];
	}

	input_info(true, &info->client->dev, "%s: sensitivity mode,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
			value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], value[8]);

	snprintf(buff, sizeof(buff),"%d,%d,%d,%d,%d,%d,%d,%d,%d",
			value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], value[8]);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t sensitivity_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	int ret;
	unsigned long value = 0;

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret != 0)
		return ret;

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &info->client->dev, "%s: power off in IC\n", __func__);
		return 0;
	}

	zt_ts_esd_timer_stop(info);
	input_err(true, &info->client->dev, "%s: enable:%ld\n", __func__, value);

	if (value == 1) {
		ts_set_touchmode(TOUCH_SENTIVITY_MEASUREMENT_MODE);
		input_info(true, &info->client->dev, "%s: enable end\n", __func__);
	} else {
		ts_set_touchmode(TOUCH_POINT_MODE);
		input_info(true, &info->client->dev, "%s: disable end\n", __func__);
	}

	zt_ts_esd_timer_start(info);
	input_info(true, &info->client->dev, "%s: done\n", __func__);
	return count;
}

static ssize_t fod_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	int ret;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	/* get fod information */
	if (!info->fod_info_vi_trx[0] || !info->fod_info_vi_trx[1] || !info->fod_info_vi_data_len) {
		ret = ts_read_from_sponge(info, ZT_SPONGE_FOD_INFO, info->fod_info_vi_trx, 3);
		if (ret < 0) {
			input_err(true, &info->client->dev,"%s: fail fod channel info.\n", __func__);
			return ret;
		}
	}

	info->fod_info_vi_data_len = info->fod_info_vi_trx[2];

	input_info(true, &info->client->dev, "%s: %d,%d,%d\n", __func__,
			info->fod_info_vi_trx[0], info->fod_info_vi_trx[1], info->fod_info_vi_data_len);

	snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d",
			info->fod_info_vi_trx[0], info->fod_info_vi_trx[1], info->fod_info_vi_data_len,
			info->cap_info.x_node_num, info->cap_info.y_node_num);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t fod_pos_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	int i, ret;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	if (!info->fod_info_vi_data_len) {
		input_err(true, &info->client->dev, "%s: not read fod_info yet\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NG");
	}

	if (!info->fod_with_finger_packet) {
		memset(info->fod_touch_vi_data, 0x00, info->fod_info_vi_data_len);
		ret = ts_read_from_sponge(info, ZT_SPONGE_FOD_POSITION,
						info->fod_touch_vi_data, info->fod_info_vi_data_len);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: fail fod data read error.\n", __func__);
			return snprintf(buf, SEC_CMD_BUF_SIZE, "NG");
		}
	}

	for (i = 0; i < info->fod_info_vi_data_len; i++) {
		snprintf(buff, 3, "%02X", info->fod_touch_vi_data[i]);
		strlcat(buf, buff, SEC_CMD_BUF_SIZE);
	}

	return strlen(buf);
}

static ssize_t aod_active_area(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	input_info(true, &info->client->dev, "%s: top:%d, edge:%d, bottom:%d\n",
			__func__, info->aod_active_area[0], info->aod_active_area[1], info->aod_active_area[2]);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d,%d,%d", info->aod_active_area[0], info->aod_active_area[1], info->aod_active_area[2]);
}

static ssize_t read_ito_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %02X%02X%02X%02X\n",
			__func__, info->ito_test[0], info->ito_test[1],
			info->ito_test[2], info->ito_test[3]);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%02X%02X%02X%02X",
			info->ito_test[0], info->ito_test[1],
			info->ito_test[2], info->ito_test[3]);
}

static ssize_t read_wet_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__,info->wet_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->wet_count);
}

static ssize_t clear_wet_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	info->wet_count = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_multi_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__,
			info->multi_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->multi_count);
}

static ssize_t clear_multi_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	info->multi_count = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_comm_err_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__,
			info->comm_err_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->comm_err_count);
}

static ssize_t clear_comm_err_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	info->comm_err_count = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_module_id_show(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	input_info(true, &info->client->dev, "%s\n", __func__);
#ifdef TCLM_CONCEPT
	return snprintf(buf, SEC_CMD_BUF_SIZE, "ZI%02X%02x%c%01X%04X\n",
			info->cap_info.reg_data_version, info->test_result.data[0],
			info->tdata->tclm_string[info->tdata->nvdata.cal_position].s_name,
			info->tdata->nvdata.cal_count & 0xF, info->tdata->nvdata.tune_fix_ver);
#else
	return snprintf(buf, SEC_CMD_BUF_SIZE, "ZI%02X\n",
			info->cap_info.reg_data_version);
#endif
}

static ssize_t prox_power_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %ld\n", __func__,
			info->prox_power_off);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%ld", info->prox_power_off);
}

static ssize_t prox_power_off_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	int ret;
	unsigned long value = 0;

	ret = kstrtoul(buf, 10, &value);
	if (ret != 0)
		return ret;

	input_info(true, &info->client->dev, "%s: enable:%ld\n", __func__, value);
	info->prox_power_off = value;

	return count;
}

/*
 * read_support_feature function
 * returns the bit combination of specific feature that is supported.
 */
static ssize_t read_support_feature(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	struct i2c_client *client = info->client;

	char buff[SEC_CMD_STR_LEN] = { 0 };
	u32 feature = 0;

	if (info->pdata->support_aot)
		feature |= INPUT_FEATURE_ENABLE_SETTINGS_AOT;
	if (info->pdata->support_open_short_test)
		feature |= INPUT_FEATURE_SUPPORT_OPEN_SHORT_TEST;

	snprintf(buff, sizeof(buff), "%d", feature);
	input_info(true, &client->dev, "%s: %s%s%s\n", __func__, buff,
			feature & INPUT_FEATURE_ENABLE_SETTINGS_AOT ? " aot" : "",
			feature & INPUT_FEATURE_SUPPORT_OPEN_SHORT_TEST ? " openshort" : "");

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);
}

/** for protos **/
static ssize_t protos_event_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__,
			info->hover_event);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->hover_event != 3 ? 0 : 3);
}

static ssize_t protos_event_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	u8 data;
	int ret;

	ret = kstrtou8(buf, 10, &data);
	if (ret < 0)
		return ret;

	input_info(true, &info->client->dev, "%s: %d\n", __func__, data);

	if (data != 0 && data != 1 && data != 3) {
		input_err(true, &info->client->dev, "%s: incorrect data\n", __func__);
		return -EINVAL;
	}

	info->ed_enable = data;

	if (info->ed_enable == 3) {
		zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT, true);
		zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT_MUTUAL, false);
	} else if (info->ed_enable == 1) {
		zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT, false);
		zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT_MUTUAL, true);
	} else {
		zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT, false);
		zt_set_optional_mode(info, DEF_OPTIONAL_MODE_EAR_DETECT_MUTUAL, false);
	}

	return count;
}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
static u16 ts_get_touch_reg(u16 addr)
{
	int ret = 1;
	u16 reg_value;

	disable_irq(misc_info->irq);

	mutex_lock(&misc_info->work_lock);
	if (misc_info->work_state != NOTHING) {
		input_info(true, &misc_info->client->dev, "other process occupied.. (%d)\n",
				misc_info->work_state);
		enable_irq(misc_info->irq);
		mutex_unlock(&misc_info->work_lock);
		return -1;
	}
	misc_info->work_state = SET_MODE;

	write_reg(misc_info->client, 0x0A, 0x0A);
	usleep_range(20 * 1000, 20 * 1000);
	write_reg(misc_info->client, 0x0A, 0x0A);

	ret = read_data(misc_info->client, addr, (u8 *)&reg_value, 2);
	if (ret < 0) {
		input_err(true, &misc_info->client->dev,"%s: fail read touch reg\n", __func__);
	}

	misc_info->work_state = NOTHING;
	enable_irq(misc_info->irq);
	mutex_unlock(&misc_info->work_lock);

	return reg_value;
}

static void ts_set_touch_reg(u16 addr, u16 value)
{
	disable_irq(misc_info->irq);

	mutex_lock(&misc_info->work_lock);
	if (misc_info->work_state != NOTHING) {
		input_info(true, &misc_info->client->dev, "other process occupied.. (%d)\n",
				misc_info->work_state);
		enable_irq(misc_info->irq);
		mutex_unlock(&misc_info->work_lock);
		return;
	}
	misc_info->work_state = SET_MODE;

	write_reg(misc_info->client, 0x0A, 0x0A);
	usleep_range(20 * 1000, 20 * 1000);
	write_reg(misc_info->client, 0x0A, 0x0A);

	if (write_reg(misc_info->client, addr, value) != I2C_SUCCESS)
		input_err(true, &misc_info->client->dev,"%s: fail write touch reg\n", __func__);

	misc_info->work_state = NOTHING;
	enable_irq(misc_info->irq);
	mutex_unlock(&misc_info->work_lock);
}

static ssize_t read_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	input_info(true, &info->client->dev, "%s: 0x%x\n", __func__,
			info->store_reg_data);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "0x%x", info->store_reg_data);
}

static ssize_t store_read_reg(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	u32 buff[2] = {0, }; //addr, size
	int ret;

	ret = sscanf(buf, "0x%x,0x%x", &buff[0], &buff[1]);
	if (ret != 2) {
		input_err(true, &info->client->dev,
				"%s: failed read params[0x%x]\n", __func__, ret);
		return -EINVAL;
	}

	if (buff[1] != 1 && buff[1] != 2) {
		input_err(true, &info->client->dev,
				"%s: incorrect byte length [0x%x]\n", __func__, buff[1]);
		return -EINVAL;
	}

	info->store_reg_data = ts_get_touch_reg((u16)buff[0]);

	if (buff[1] == 1)
		info->store_reg_data = info->store_reg_data & 0x00FF;

	input_info(true, &info->client->dev,
			"%s: read touch reg [addr:0x%x][data:0x%x]\n",
			__func__, buff[0], info->store_reg_data);

	return size;
}

static ssize_t store_write_reg(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);
	int buff[3];
	int ret;

	ret = sscanf(buf, "0x%x,0x%x,0x%x", &buff[0], &buff[1], &buff[2]);
	if (ret != 3) {
		input_err(true, &info->client->dev,
				"%s: failed read params[0x%x]\n", __func__, ret);
		return -EINVAL;
	}

	if (buff[1] != 1 && buff[1] != 2) {
		input_err(true, &info->client->dev,
				"%s: incorrect byte length [0x%x]\n", __func__, buff[1]);
		return -EINVAL;
	}

	if (buff[1] == 1)
		buff[2] = buff[2] & 0x00FF;

	ts_set_touch_reg((u16)buff[0], (u16)buff[2]);
	input_info(true, &info->client->dev,
			"%s: write touch reg [addr:0x%x][byte:0x%x][data:0x%x]\n",
			__func__, buff[0], buff[1], buff[2]);

	return size;
}
#endif
static ssize_t get_lp_dump(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct zt_ts_info *info = container_of(sec, struct zt_ts_info, sec);

	u8 sponge_data[10] = {0, };
	u16 current_index;
	u8 dump_format, dump_num;
	u16 dump_start, dump_end;
	int i;

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_err(true, &info->client->dev, "%s: Touch is stopped!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
	}

	if (info->work_state == ESD_TIMER) {
		input_err(true, &info->client->dev, "%s: Reset is ongoing!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "Reset is ongoing");
	}

	disable_irq(info->irq);
	ts_read_from_sponge(info, ZT_SPONGE_DUMP_FORMAT, (u8*)sponge_data, 4);

	dump_format = sponge_data[0];
	dump_num = sponge_data[1];
	dump_start = ZT_SPONGE_DUMP_START;
	dump_end = dump_start + (dump_format * (dump_num - 1));

	current_index = (sponge_data[3] & 0xFF) << 8 | (sponge_data[2] & 0xFF);
	if (current_index > dump_end || current_index < dump_start) {
		input_err(true, &info->client->dev,
				"Failed to Sponge LP log %d\n", current_index);
		snprintf(buf, SEC_CMD_BUF_SIZE,
				"NG, Failed to Sponge LP log, current_index=%d",
				current_index);
		goto out;
	}

	input_info(true, &info->client->dev, "%s: DEBUG format=%d, num=%d, start=%d, end=%d, current_index=%d\n",
				__func__, dump_format, dump_num, dump_start, dump_end, current_index);

	for (i = dump_num - 1 ; i >= 0 ; i--) {
		u16 data0, data1, data2, data3, data4;
		char buff[30] = {0, };
		u16 sponge_addr;

		if (current_index < (dump_format * i))
			sponge_addr = (dump_format * dump_num) + current_index - (dump_format * i);
		else
			sponge_addr = current_index - (dump_format * i);

		if (sponge_addr < dump_start)
			sponge_addr += (dump_format * dump_num);

		ts_read_from_sponge(info, sponge_addr, (u8*)sponge_data, dump_format);

		data0 = (sponge_data[1] & 0xFF) << 8 | (sponge_data[0] & 0xFF);
		data1 = (sponge_data[3] & 0xFF) << 8 | (sponge_data[2] & 0xFF);
		data2 = (sponge_data[5] & 0xFF) << 8 | (sponge_data[4] & 0xFF);
		data3 = (sponge_data[7] & 0xFF) << 8 | (sponge_data[6] & 0xFF);
		data4 = (sponge_data[9] & 0xFF) << 8 | (sponge_data[8] & 0xFF);

		if (data0 || data1 || data2 || data3 || data4) {
			if (dump_format == 10) {
				snprintf(buff, sizeof(buff),
						"%d: %04x%04x%04x%04x%04x\n",
						sponge_addr, data0, data1, data2, data3, data4);
			} else {
				snprintf(buff, sizeof(buff),
						"%d: %04x%04x%04x%04x\n",
						sponge_addr, data0, data1, data2, data3);
			}
			strlcat(buf, buff, PAGE_SIZE);
		}
	}

out:
	enable_irq(info->irq);
	return strlen(buf);
}

static DEVICE_ATTR(scrub_pos, S_IRUGO, scrub_position_show, NULL);
static DEVICE_ATTR(sensitivity_mode, S_IRUGO | S_IWUSR | S_IWGRP, sensitivity_mode_show, sensitivity_mode_store);
static DEVICE_ATTR(ito_check, S_IRUGO | S_IWUSR | S_IWGRP, read_ito_check_show, clear_wet_mode_store);
static DEVICE_ATTR(wet_mode, S_IRUGO | S_IWUSR | S_IWGRP, read_wet_mode_show, clear_wet_mode_store);
static DEVICE_ATTR(comm_err_count, S_IRUGO | S_IWUSR | S_IWGRP, read_comm_err_count_show, clear_comm_err_count_store);
static DEVICE_ATTR(multi_count, S_IRUGO | S_IWUSR | S_IWGRP, read_multi_count_show, clear_multi_count_store);
static DEVICE_ATTR(module_id, S_IRUGO, read_module_id_show, NULL);
static DEVICE_ATTR(prox_power_off, S_IRUGO | S_IWUSR | S_IWGRP, prox_power_off_show, prox_power_off_store);
static DEVICE_ATTR(support_feature, S_IRUGO, read_support_feature, NULL);
static DEVICE_ATTR(virtual_prox, S_IRUGO | S_IWUSR | S_IWGRP, protos_event_show, protos_event_store);
static DEVICE_ATTR(fod_info, S_IRUGO, fod_info_show, NULL);
static DEVICE_ATTR(fod_pos, S_IRUGO, fod_pos_show, NULL);
static DEVICE_ATTR(aod_active_area, 0444, aod_active_area, NULL);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
static DEVICE_ATTR(read_reg_data, S_IRUGO | S_IWUSR | S_IWGRP, read_reg_show, store_read_reg);
static DEVICE_ATTR(write_reg_data, S_IWUSR | S_IWGRP, NULL, store_write_reg);
#endif
static DEVICE_ATTR(get_lp_dump, 0444, get_lp_dump, NULL);

static struct attribute *touchscreen_attributes[] = {
	&dev_attr_scrub_pos.attr,
	&dev_attr_sensitivity_mode.attr,
	&dev_attr_ito_check.attr,
	&dev_attr_wet_mode.attr,
	&dev_attr_multi_count.attr,
	&dev_attr_comm_err_count.attr,
	&dev_attr_module_id.attr,
	&dev_attr_prox_power_off.attr,
	&dev_attr_support_feature.attr,
	&dev_attr_virtual_prox.attr,
	&dev_attr_fod_info.attr,
	&dev_attr_fod_pos.attr,
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	&dev_attr_read_reg_data.attr,
	&dev_attr_write_reg_data.attr,
#endif
	&dev_attr_get_lp_dump.attr,
	&dev_attr_aod_active_area.attr,
	NULL,
};

static struct attribute_group touchscreen_attr_group = {
	.attrs = touchscreen_attributes,
};

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("get_checksum_data", get_checksum_data),},
	{SEC_CMD("get_threshold", get_threshold),},
	{SEC_CMD("get_module_vendor", get_module_vendor),},
	{SEC_CMD("get_chip_vendor", get_chip_vendor),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("get_x_num", get_x_num),},
	{SEC_CMD("get_y_num", get_y_num),},

	/* vendor dependant command */
	{SEC_CMD("run_cnd_read", run_cnd_read),},
	{SEC_CMD("run_cnd_read_all", run_cnd_read_all),},
	{SEC_CMD("run_dnd_read", run_dnd_read),},
	{SEC_CMD("run_dnd_read_all", run_dnd_read_all),},
	{SEC_CMD("run_dnd_v_gap_read", run_dnd_v_gap_read),},
	{SEC_CMD("run_dnd_v_gap_read_all", run_dnd_v_gap_read_all),},
	{SEC_CMD("run_dnd_h_gap_read", run_dnd_h_gap_read),},
	{SEC_CMD("run_dnd_h_gap_read_all", run_dnd_h_gap_read_all),},
	{SEC_CMD("run_selfdnd_read", run_selfdnd_read),},
	{SEC_CMD("run_selfdnd_read_all", run_selfdnd_read_all),},
	{SEC_CMD("run_self_saturation_read", run_self_saturation_read),},	/* self_saturation_rx */
	{SEC_CMD("run_self_saturation_read_all", run_ssr_read_all),},
	{SEC_CMD("run_selfdnd_h_gap_read", run_selfdnd_h_gap_read),},
	{SEC_CMD("run_selfdnd_h_gap_read_all", run_selfdnd_h_gap_read_all),},
	{SEC_CMD("run_trx_short_test", run_trx_short_test),},
	{SEC_CMD("run_jitter_test", run_jitter_test),},
	{SEC_CMD("run_charge_pump_read", run_charge_pump_read),},
	{SEC_CMD("run_mis_cal_read", run_mis_cal_read),},
	{SEC_CMD("run_factory_miscalibration", run_factory_miscalibration),},
	{SEC_CMD("run_mis_cal_read_all", run_mis_cal_read_all),},
	{SEC_CMD_H("touch_aging_mode", touch_aging_mode),},
#ifdef TCLM_CONCEPT
	{SEC_CMD("get_pat_information", get_pat_information),},
	{SEC_CMD("get_tsp_test_result", get_tsp_test_result),},
	{SEC_CMD("set_tsp_test_result", set_tsp_test_result),},
	{SEC_CMD("increase_disassemble_count", increase_disassemble_count),},
	{SEC_CMD("get_disassemble_count", get_disassemble_count),},
#endif
	{SEC_CMD("run_cs_raw_read_all", run_cs_raw_read_all),},
	{SEC_CMD("run_cs_delta_read_all", run_cs_delta_read_all),},
	{SEC_CMD("run_force_calibration", run_ref_calibration),},
	{SEC_CMD("run_amp_check_read", run_amp_check_read),},
	{SEC_CMD("clear_reference_data", clear_reference_data),},
	{SEC_CMD("dead_zone_enable", dead_zone_enable),},
	{SEC_CMD("set_grip_data", set_grip_data),},
	{SEC_CMD_H("set_touchable_area", set_touchable_area),},
	{SEC_CMD_H("clear_cover_mode", clear_cover_mode),},
	{SEC_CMD_H("spay_enable", spay_enable),},
	{SEC_CMD("fod_enable", fod_enable),},
	{SEC_CMD_H("fod_lp_mode", fod_lp_mode),},
	{SEC_CMD("set_fod_rect", set_fod_rect),},
	{SEC_CMD_H("singletap_enable", singletap_enable),},
	{SEC_CMD_H("aot_enable", aot_enable),},
	{SEC_CMD_H("aod_enable", aod_enable),},
	{SEC_CMD("set_aod_rect", set_aod_rect),},
	{SEC_CMD("get_wet_mode", get_wet_mode),},
	{SEC_CMD_H("glove_mode", glove_mode),},
	{SEC_CMD_H("pocket_mode_enable", pocket_mode_enable),},
	{SEC_CMD("get_crc_check", get_crc_check),},
	{SEC_CMD("run_test_vsync", run_test_vsync),},	
	{SEC_CMD("factory_cmd_result_all", factory_cmd_result_all),},
	{SEC_CMD("check_connection", check_connection),},
	{SEC_CMD("run_prox_intensity_read_all", run_prox_intensity_read_all),},
	{SEC_CMD_H("ear_detect_enable", ear_detect_enable),},
	{SEC_CMD_H("set_scan_rate", set_scan_rate),},
	{SEC_CMD_H("set_wirelesscharger_mode", set_wirelesscharger_mode),},
	{SEC_CMD_H("set_note_mode", set_note_mode),},
	{SEC_CMD_H("set_game_mode", set_game_mode),},
	{SEC_CMD("read_osc_value", read_osc_value),},
	{SEC_CMD_H("set_sip_mode", set_sip_mode),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

static int init_sec_factory(struct zt_ts_info *info)
{
	struct tsp_raw_data *raw_data;
	int ret;

	raw_data = kzalloc(sizeof(struct tsp_raw_data), GFP_KERNEL);
	if (unlikely(!raw_data)) {
		input_err(true, &info->client->dev, "%s: Failed to allocate memory\n",
				__func__);
		ret = -ENOMEM;

		goto err_alloc;
	}

	ret = sec_cmd_init(&info->sec, sec_cmds,
			ARRAY_SIZE(sec_cmds), SEC_CLASS_DEVT_TSP);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to sec_cmd_init\n", __func__);
		goto err_init_cmd;
	}

	ret = sysfs_create_group(&info->sec.fac_dev->kobj,
			&touchscreen_attr_group);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: FTS Failed to create sysfs attributes\n", __func__);
		goto err_create_sysfs;
	}

	ret = sysfs_create_link(&info->sec.fac_dev->kobj,
			&info->input_dev->dev.kobj, "input");
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: Failed to create link\n", __func__);
		goto err_create_sysfs;
	}

	info->raw_data = raw_data;

	return ret;

err_create_sysfs:
err_init_cmd:
	kfree(raw_data);
err_alloc:

	return ret;
}

#ifdef USE_MISC_DEVICE
static int ts_misc_fops_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ts_misc_fops_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static long ts_misc_fops_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	struct raw_ioctl raw_ioctl;
	u8 *u8Data;
	int ret = 0;
	size_t sz = 0;
	u16 mode;
	struct reg_ioctl reg_ioctl;
	u16 val;
	int nval = 0;
#ifdef CONFIG_COMPAT
	void __user *argp = compat_ptr(arg);
#else
	void __user *argp = (void __user *)arg;
#endif

	if (misc_info == NULL) {
		pr_err("%s %s misc device NULL?\n", SECLOG, __func__);
		return -1;
	}

	switch (cmd) {
	case TOUCH_IOCTL_GET_DEBUGMSG_STATE:
		ret = m_ts_debug_mode;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_SET_DEBUGMSG_STATE:
		if (copy_from_user(&nval, argp, sizeof(nval))) {
			input_err(true, &misc_info->client->dev, "%s: error : copy_from_user\n", __func__);
			return -1;
		}
		if (nval)
			input_info(true, &misc_info->client->dev, "[zinitix_touch] on debug mode (%d)\n", nval);
		else
			input_info(true, &misc_info->client->dev, "[zinitix_touch] off debug mode (%d)\n", nval);
		m_ts_debug_mode = nval;
		break;

	case TOUCH_IOCTL_GET_CHIP_REVISION:
		ret = misc_info->cap_info.ic_revision;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_FW_VERSION:
		ret = misc_info->cap_info.fw_version;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_REG_DATA_VERSION:
		ret = misc_info->cap_info.reg_data_version;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_VARIFY_UPGRADE_SIZE:
		if (copy_from_user(&sz, argp, sizeof(size_t)))
			return -1;
		if (misc_info->cap_info.ic_fw_size != sz) {
			input_info(true, &misc_info->client->dev, "%s: firmware size error\n", __func__);
			return -1;
		}
		break;

	case TOUCH_IOCTL_GET_X_RESOLUTION:
		ret = misc_info->pdata->x_resolution;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_Y_RESOLUTION:
		ret = misc_info->pdata->y_resolution;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_X_NODE_NUM:
		ret = misc_info->cap_info.x_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_Y_NODE_NUM:
		ret = misc_info->cap_info.y_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_TOTAL_NODE_NUM:
		ret = misc_info->cap_info.total_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_HW_CALIBRAION:
		ret = -1;
		disable_irq(misc_info->irq);
		mutex_lock(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			input_info(true, &misc_info->client->dev, "[zinitix_touch]: other process occupied.. (%d)\r\n",
					misc_info->work_state);
			mutex_unlock(&misc_info->work_lock);
			return -1;
		}
		misc_info->work_state = HW_CALIBRAION;
		zt_delay(100);

		/* h/w calibration */
		if (ts_hw_calibration(misc_info) == true) {
			ret = 0;
#ifdef TCLM_CONCEPT
			sec_tclm_root_of_cal(misc_info->tdata, CALPOSITION_TESTMODE);
			sec_execute_tclm_package(misc_info->tdata, 1);
			sec_tclm_root_of_cal(misc_info->tdata, CALPOSITION_NONE);
#endif
		}

		mode = misc_info->touch_mode;
		if (write_reg(misc_info->client,
					ZT_TOUCH_MODE, mode) != I2C_SUCCESS) {
			input_info(true, &misc_info->client->dev, "[zinitix_touch]: failed to set touch mode %d.\n",
					mode);
			goto fail_hw_cal;
		}

		if (write_cmd(misc_info->client,
					ZT_SWRESET_CMD) != I2C_SUCCESS)
			goto fail_hw_cal;

		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		mutex_unlock(&misc_info->work_lock);
		return ret;
fail_hw_cal:
		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		mutex_unlock(&misc_info->work_lock);
		return -1;

	case TOUCH_IOCTL_SET_RAW_DATA_MODE:
		if (misc_info == NULL) {
			pr_err("%s %s misc device NULL?\n", SECLOG, __func__);
			return -1;
		}
		if (copy_from_user(&nval, argp, sizeof(nval))) {
			input_info(true, &misc_info->client->dev, "%s: error : copy_from_user\n", __func__);
			misc_info->work_state = NOTHING;
			return -1;
		}
		ts_set_touchmode((u16)nval);

		return 0;

	case TOUCH_IOCTL_GET_REG:
		if (misc_info == NULL) {
			pr_err("%s %s misc device NULL?\n", SECLOG, __func__);
			return -1;
		}
		mutex_lock(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			input_info(true, &misc_info->client->dev, "[zinitix_touch]:other process occupied.. (%d)\n",
					misc_info->work_state);
			mutex_unlock(&misc_info->work_lock);
			return -1;
		}

		misc_info->work_state = SET_MODE;

		if (copy_from_user(&reg_ioctl, argp, sizeof(struct reg_ioctl))) {
			misc_info->work_state = NOTHING;
			mutex_unlock(&misc_info->work_lock);
			input_info(true, &misc_info->client->dev, "%s: error : copy_from_user\n", __func__);
			return -1;
		}

		if (read_data(misc_info->client,
					(u16)reg_ioctl.addr, (u8 *)&val, 2) < 0)
			ret = -1;

		nval = (int)val;

#ifdef CONFIG_COMPAT
		if (copy_to_user(compat_ptr(reg_ioctl.val), (u8 *)&nval, 4)) {
#else
		if (copy_to_user((void __user *)(reg_ioctl.val), (u8 *)&nval, 4)) {
#endif
			misc_info->work_state = NOTHING;
			mutex_unlock(&misc_info->work_lock);
			input_info(true, &misc_info->client->dev, "%s: error : copy_to_user\n", __func__);
			return -1;
		}

		input_info(true, &misc_info->client->dev, "%s read : reg addr = 0x%x, val = 0x%x\n", __func__,
				reg_ioctl.addr, nval);

		misc_info->work_state = NOTHING;
		mutex_unlock(&misc_info->work_lock);
		return ret;

		case TOUCH_IOCTL_SET_REG:

		mutex_lock(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			input_info(true, &misc_info->client->dev, "[zinitix_touch]: other process occupied.. (%d)\n",
					misc_info->work_state);
			mutex_unlock(&misc_info->work_lock);
			return -1;
		}

		misc_info->work_state = SET_MODE;
		if (copy_from_user(&reg_ioctl,
					argp, sizeof(struct reg_ioctl))) {
			misc_info->work_state = NOTHING;
			mutex_unlock(&misc_info->work_lock);
			input_info(true, &misc_info->client->dev, "%s: error : copy_from_user(1)\n", __func__);
			return -1;
		}

#ifdef CONFIG_COMPAT
		if (copy_from_user(&val, compat_ptr(reg_ioctl.val), sizeof(val))) {
#else
		if (copy_from_user(&val,(void __user *)(reg_ioctl.val), sizeof(val))) {
#endif
			misc_info->work_state = NOTHING;
			mutex_unlock(&misc_info->work_lock);
			input_info(true, &misc_info->client->dev, "%s: error : copy_from_user(2)\n", __func__);
			return -1;
		}

		if (write_reg(misc_info->client,
					(u16)reg_ioctl.addr, val) != I2C_SUCCESS)
			ret = -1;

		input_info(true, &misc_info->client->dev, "%s write : reg addr = 0x%x, val = 0x%x\r\n", __func__,
				reg_ioctl.addr, val);
		misc_info->work_state = NOTHING;
		mutex_unlock(&misc_info->work_lock);
		return ret;

		case TOUCH_IOCTL_DONOT_TOUCH_EVENT:

		if (misc_info == NULL) {
			pr_err("%s %s misc device NULL?\n", SECLOG, __func__);
			return -1;
		}
		mutex_lock(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			input_info(true, &misc_info->client->dev, "[zinitix_touch]: other process occupied.. (%d)\r\n",
					misc_info->work_state);
			mutex_unlock(&misc_info->work_lock);
			return -1;
		}

		misc_info->work_state = SET_MODE;
		if (write_reg(misc_info->client,
					ZT_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			ret = -1;
		input_info(true, &misc_info->client->dev, "%s write : reg addr = 0x%x, val = 0x0\r\n", __func__,
				ZT_INT_ENABLE_FLAG);

		misc_info->work_state = NOTHING;
		mutex_unlock(&misc_info->work_lock);
		return ret;

		case TOUCH_IOCTL_SEND_SAVE_STATUS:
		if (misc_info == NULL) {
			pr_err("%s %s misc device NULL?\n", SECLOG, __func__);
			return -1;
		}
		mutex_lock(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			input_info(true, &misc_info->client->dev, "[zinitix_touch]: other process occupied.." \
					"(%d)\r\n", misc_info->work_state);
			mutex_unlock(&misc_info->work_lock);
			return -1;
		}
		misc_info->work_state = SET_MODE;
		ret = 0;
		write_reg(misc_info->client, VCMD_NVM_WRITE_ENABLE, 0x0001);
		if (write_cmd(misc_info->client,
					ZT_SAVE_STATUS_CMD) != I2C_SUCCESS)
			ret =  -1;

		zt_delay(1000);	/* for fusing eeprom */
		write_reg(misc_info->client, VCMD_NVM_WRITE_ENABLE, 0x0000);

		misc_info->work_state = NOTHING;
		mutex_unlock(&misc_info->work_lock);
		return ret;

		case TOUCH_IOCTL_GET_RAW_DATA:
		if (misc_info == NULL) {
			pr_err("%s %s misc device NULL?\n", SECLOG, __func__);
			return -1;
		}

		if (misc_info->touch_mode == TOUCH_POINT_MODE)
			return -1;

		mutex_lock(&misc_info->raw_data_lock);
		if (misc_info->update == 0) {
			mutex_unlock(&misc_info->raw_data_lock);
			return -2;
		}

		if (copy_from_user(&raw_ioctl,
					argp, sizeof(struct raw_ioctl))) {
			mutex_unlock(&misc_info->raw_data_lock);
			input_info(true, &misc_info->client->dev, "%s: error : copy_from_user\n", __func__);
			return -1;
		}

		misc_info->update = 0;

		u8Data = (u8 *)&misc_info->cur_data[0];
		if (raw_ioctl.sz > MAX_TRAW_DATA_SZ*2)
			raw_ioctl.sz = MAX_TRAW_DATA_SZ*2;
#ifdef CONFIG_COMPAT
		if (copy_to_user(compat_ptr(raw_ioctl.buf), (u8 *)u8Data, raw_ioctl.sz)) {
#else
		if (copy_to_user((void __user *)(raw_ioctl.buf), (u8 *)u8Data, raw_ioctl.sz)) {
#endif
			mutex_unlock(&misc_info->raw_data_lock);
			return -1;
		}

		mutex_unlock(&misc_info->raw_data_lock);
		return 0;

	default:
		break;
	}
	return 0;
}
#endif

#ifdef CONFIG_OF
static int zt_pinctrl_configure(struct zt_ts_info *info, bool active)
{
	struct device *dev = &info->client->dev;
	struct pinctrl_state *pinctrl_state;
	int retval = 0;

	input_dbg(true, dev, "%s: pinctrl %d\n", __func__, active);

	if (active)
		pinctrl_state = pinctrl_lookup_state(info->pinctrl, "on_state");
	else
		pinctrl_state = pinctrl_lookup_state(info->pinctrl, "off_state");

	if (IS_ERR(pinctrl_state)) {
		input_err(true, dev, "%s: Failed to lookup pinctrl.\n", __func__);
	} else {
		retval = pinctrl_select_state(info->pinctrl, pinctrl_state);
		if (retval)
			input_err(true, dev, "%s: Failed to configure pinctrl.\n", __func__);
	}
	return 0;
}

static int zt_power_ctrl(void *data, bool on)
{
	struct zt_ts_info* info = (struct zt_ts_info*)data;
	struct zt_ts_platform_data *pdata = info->pdata;
	struct device *dev = &info->client->dev;
	struct regulator *regulator_dvdd = NULL;
	struct regulator *regulator_avdd;
	int retval = 0;
	static bool enabled;

	if (enabled == on)
		return retval;

	if (!pdata->gpio_ldo_en) {
		regulator_dvdd = regulator_get(NULL, pdata->regulator_dvdd);
		if (IS_ERR(regulator_dvdd)) {
			input_err(true, dev, "%s: Failed to get %s regulator.\n",
				 __func__, pdata->regulator_dvdd);
			return PTR_ERR(regulator_dvdd);
		}
	}
	regulator_avdd = regulator_get(NULL, pdata->regulator_avdd);
	if (IS_ERR(regulator_avdd)) {
		input_err(true, dev, "%s: Failed to get %s regulator.\n",
			 __func__, pdata->regulator_avdd);
		return PTR_ERR(regulator_avdd);
	}

	input_info(true, dev, "%s: %s\n", __func__, on ? "on" : "off");

	if (on) {
		retval = regulator_enable(regulator_avdd);
		if (retval) {
			input_err(true, dev, "%s: Failed to enable avdd: %d\n", __func__, retval);
			return retval;
		}
		if (!pdata->gpio_ldo_en) {
			retval = regulator_enable(regulator_dvdd);
			if (retval) {
				input_err(true, dev, "%s: Failed to enable vdd: %d\n", __func__, retval);
				return retval;
			}
		}
	} else {
		if (!pdata->gpio_ldo_en) {
			if (regulator_is_enabled(regulator_dvdd))
				regulator_disable(regulator_dvdd);
		}
		if (regulator_is_enabled(regulator_avdd))
			regulator_disable(regulator_avdd);
	}

	enabled = on;
	if (!pdata->gpio_ldo_en)
		regulator_put(regulator_dvdd);
	regulator_put(regulator_avdd);

	return retval;
}


static int zinitix_init_gpio(struct zt_ts_platform_data *pdata)
{
	int ret = 0;

	ret = gpio_request(pdata->gpio_int, "zinitix_tsp_irq");
	if (ret) {
		pr_err("[TSP]%s: unable to request zinitix_tsp_irq [%d]\n",
				__func__, pdata->gpio_int);
		return ret;
	}

	return ret;
}

static int zt_ts_parse_dt(struct device_node *np,
		struct device *dev,
		struct zt_ts_platform_data *pdata)
{
	int ret = 0;
	u32 temp;
	u32 px_zone[3] = { 0 };

	ret = of_property_read_u32(np, "zinitix,x_resolution", &temp);
	if (ret) {
		input_info(true, dev, "%s: Unable to read controller version\n", __func__);
		return ret;
	} else {
		pdata->x_resolution = (u16) temp;
	}

	ret = of_property_read_u32(np, "zinitix,y_resolution", &temp);
	if (ret) {
		input_info(true, dev, "%s: Unable to read controller version\n", __func__);
		return ret;
	} else {
		pdata->y_resolution = (u16) temp;
	}

	if (of_property_read_u32_array(np, "zinitix,area-size", px_zone, 3)) {
		dev_info(dev, "%s: Failed to get zone's size\n", __func__);
		pdata->area_indicator = 48;
		pdata->area_navigation = 96;
		pdata->area_edge = 60;
	} else {
		pdata->area_indicator = (u8) px_zone[0];
		pdata->area_navigation = (u8) px_zone[1];
		pdata->area_edge = (u8) px_zone[2];
	}

	ret = of_property_read_u32(np, "zinitix,page_size", &temp);
	if (ret) {
		input_info(true, dev, "%s: Unable to read controller version\n", __func__);
		return ret;
	} else {
		pdata->page_size = (u16) temp;
	}

	pdata->gpio_int = of_get_named_gpio(np, "zinitix,irq_gpio", 0);
	if (pdata->gpio_int < 0) {
		pr_err("%s: of_get_named_gpio failed: tsp_gpio %d\n", __func__,
				pdata->gpio_int);
		return -EINVAL;
	}

	if (of_get_property(np, "zinitix,gpio_ldo_en", NULL)) {
			pdata->gpio_ldo_en = true;
	} else {
		if (of_property_read_string(np, "zinitix,regulator_dvdd", &pdata->regulator_dvdd)) {
			input_err(true, dev, "Failed to get regulator_dvdd name property\n");
			return -EINVAL;
		}
	}

	if (of_property_read_string(np, "zinitix,regulator_avdd", &pdata->regulator_avdd)) {
		input_err(true, dev, "Failed to get regulator_avdd name property\n");
		return -EINVAL;
	}

	pdata->tsp_power = zt_power_ctrl;

	/* Optional parmeters(those values are not mandatory)
	 * do not return error value even if fail to get the value
	 */
	of_property_read_string(np, "zinitix,firmware_name", &pdata->firmware_name);
	of_property_read_string(np, "zinitix,chip_name", &pdata->chip_name);

	pdata->support_spay = of_property_read_bool(np, "zinitix,spay");
	pdata->support_aod = of_property_read_bool(np, "zinitix,aod");
	pdata->support_aot = of_property_read_bool(np, "zinitix,aot");
	pdata->support_ear_detect = of_property_read_bool(np, "support_ear_detect_mode");
	pdata->mis_cal_check = of_property_read_bool(np, "zinitix,mis_cal_check");
	pdata->support_dex = of_property_read_bool(np, "support_dex_mode");
	pdata->support_open_short_test = of_property_read_bool(np, "support_open_short_test");

	of_property_read_u32(np, "zinitix,bringup", &pdata->bringup);

	input_err(true, dev, "%s: x_r:%d, y_r:%d FW:%s, CN:%s,"
			" Spay:%d, AOD:%d, AOT:%d, ED:%d, Bringup:%d, MISCAL:%d, DEX:%d, OPEN/SHORT:%d"
			" \n",  __func__, pdata->x_resolution, pdata->y_resolution,
			pdata->firmware_name, pdata->chip_name,
			pdata->support_spay, pdata->support_aod, pdata->support_aot,
			pdata->support_ear_detect, pdata->bringup, pdata->mis_cal_check,
			pdata->support_dex, pdata->support_open_short_test);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	of_property_read_u32(np, "zinitix,ss_touch_num", &pdata->ss_touch_num);
	input_info(true, dev, "%s: ss_touch_num:%d\n", __func__, pdata->ss_touch_num);
#endif
	return 0;
}

static void sec_tclm_parse_dt(struct i2c_client *client, struct sec_tclm_data *tdata)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "zinitix,tclm_level", &tdata->tclm_level) < 0) {
		tdata->tclm_level = 0;
		input_err(true, dev, "%s: Failed to get tclm_level property\n", __func__);
	}

	if (of_property_read_u32(np, "zinitix,afe_base", &tdata->afe_base) < 0) {
		tdata->afe_base = 0;
		input_err(true, dev, "%s: Failed to get afe_base property\n", __func__);
	}

	input_err(true, &client->dev, "%s: tclm_level %d, afe_base %04X\n", __func__, tdata->tclm_level, tdata->afe_base);

}
#endif

static void zt_display_rawdata(struct zt_ts_info *info, struct tsp_raw_data *raw_data, int type, int gap)
{
	int x_num = info->cap_info.x_node_num;
	int y_num = info->cap_info.y_node_num;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	int tmp_rawdata;
	int i, j;

	pStr = kzalloc(6 * (x_num + 1), GFP_KERNEL);
	if (pStr == NULL)
		return;

	memset(pStr, 0x0, 6 * (x_num + 1));
	snprintf(pTmp, sizeof(pTmp), "      Rx");
	strlcat(pStr, pTmp, 6 * (x_num + 1));

	for (i = 0; i < x_num; i++) {
		snprintf(pTmp, sizeof(pTmp), " %02d  ", i);
		strlcat(pStr, pTmp, 6 * (x_num + 1));
	}

	input_info(true, &info->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, 6 * (x_num + 1));
	snprintf(pTmp, sizeof(pTmp), " +");
	strlcat(pStr, pTmp, 6 * (x_num + 1));

	for (i = 0; i < x_num; i++) {
		snprintf(pTmp, sizeof(pTmp), "-----");
		strlcat(pStr, pTmp, 6 * (x_num + 1));
	}

	input_info(true, &info->client->dev, "%s\n", pStr);

	for (i = 0; i < y_num; i++) {
		memset(pStr, 0x0, 6 * (x_num + 1));
		snprintf(pTmp, sizeof(pTmp), "Tx%02d | ", i);
		strlcat(pStr, pTmp, 6 * (x_num + 1));

		for (j = 0; j < x_num; j++) {
			switch (type) {
			case TOUCH_REF_ABNORMAL_TEST_MODE:
				/* print mis_cal data (value - DEF_MIS_CAL_SPEC_MID) */
				tmp_rawdata = raw_data->reference_data_abnormal[(i * x_num) + j];
				snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);
				break;
			case TOUCH_DND_MODE:
				if (gap == 1) {
					/* print dnd v gap data */
					tmp_rawdata = raw_data->vgap_data[(i * x_num) + j];
					snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);
				} else if (gap == 2) {
					/* print dnd h gap data */
					tmp_rawdata = raw_data->hgap_data[(i * x_num) + j];
					snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);
				} else {
					/* print dnd data */
					tmp_rawdata = raw_data->dnd_data[(i * x_num) + j];
					snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);
				}
				break;
			case TOUCH_RAW_MODE:
				/* print cnd data */
				tmp_rawdata = raw_data->cnd_data[(i * x_num) + j];
				snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);
				break;
			case TOUCH_JITTER_MODE:
				/* print jitter data */
				tmp_rawdata = raw_data->jitter_data[(i * x_num) + j];
				snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);
				break;
			case TOUCH_REFERENCE_MODE:
				/* print reference data */
				tmp_rawdata = raw_data->reference_data[(i * x_num) + j];
				snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);
				break;
			case TOUCH_DELTA_MODE:
				/* print delta data */
				tmp_rawdata = raw_data->delta_data[(i * x_num) + j];
				snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);
				break;
			}
			strlcat(pStr, pTmp, 6 * (x_num + 1));
		}
		input_info(true, &info->client->dev, "%s\n", pStr);
	}

	kfree(pStr);
}

/* print raw data at booting time */
static void zt_display_rawdata_boot(struct zt_ts_info *info, struct tsp_raw_data *raw_data, int *min, int *max, bool is_mis_cal)
{
	int x_num = info->cap_info.x_node_num;
	int y_num = info->cap_info.y_node_num;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	int tmp_rawdata;
	int i, j;

	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, is_mis_cal ? "mis_cal ": "dnd ");

	pStr = kzalloc(6 * (x_num + 1), GFP_KERNEL);
	if (pStr == NULL)
		return;

	memset(pStr, 0x0, 6 * (x_num + 1));
	snprintf(pTmp, sizeof(pTmp), "      Rx");
	strlcat(pStr, pTmp, 6 * (x_num + 1));

	for (i = 0; i < x_num; i++) {
		snprintf(pTmp, sizeof(pTmp), " %02d  ", i);
		strlcat(pStr, pTmp, 6 * (x_num + 1));
	}

	input_raw_info(true, &info->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, 6 * (x_num + 1));
	snprintf(pTmp, sizeof(pTmp), " +");
	strlcat(pStr, pTmp, 6 * (x_num + 1));

	for (i = 0; i < x_num; i++) {
		snprintf(pTmp, sizeof(pTmp), "-----");
		strlcat(pStr, pTmp, 6 * (x_num + 1));
	}

	input_raw_info(true, &info->client->dev, "%s\n", pStr);

	for (i = 0; i < y_num; i++) {
		memset(pStr, 0x0, 6 * (x_num + 1));
		snprintf(pTmp, sizeof(pTmp), "Tx%02d | ", i);
		strlcat(pStr, pTmp, 6 * (x_num + 1));

		for (j = 0; j < x_num; j++) {

			if (is_mis_cal) {
				/* print mis_cal data (value - DEF_MIS_CAL_SPEC_MID) */
				tmp_rawdata = raw_data->reference_data_abnormal[(i * x_num) + j];
				snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);

				if (tmp_rawdata < *min)
					*min = tmp_rawdata;

				if (tmp_rawdata > *max)
					*max = tmp_rawdata;

			} else {
				/* print dnd data */
				tmp_rawdata = raw_data->dnd_data[(i * x_num) + j];
				snprintf(pTmp, sizeof(pTmp), " %4d", tmp_rawdata);

				if (tmp_rawdata < *min && tmp_rawdata != 0)
					*min = tmp_rawdata;

				if (tmp_rawdata > *max)
					*max = tmp_rawdata;
			}
			strlcat(pStr, pTmp, 6 * (x_num + 1));
		}
		input_raw_info(true, &info->client->dev, "%s\n", pStr);
	}

	input_raw_info(true, &info->client->dev, "Max/Min %d,%d ##\n", *max, *min);

	kfree(pStr);
}

static void zt_run_dnd(struct zt_ts_info *info)
{
	struct tsp_raw_data *raw_data = info->raw_data;
	int min = 0xFFFF, max = -0xFF;
	int ret;

	zt_ts_esd_timer_stop(info);
	ret = ts_set_touchmode(TOUCH_DND_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		goto out;
	}
	get_raw_data(info, (u8 *)raw_data->dnd_data, 1);
	ts_set_touchmode(TOUCH_POINT_MODE);

	zt_display_rawdata_boot(info, raw_data, &min, &max, false);

out:
	zt_ts_esd_timer_start(info);
	return;
}

static void zt_run_mis_cal(struct zt_ts_info *info)
{
	struct zt_ts_platform_data *pdata = info->pdata;
	struct tsp_raw_data *raw_data = info->raw_data;

	char mis_cal_data = 0xF0;
	int ret = 0;
	s16 raw_data_buff[TSP_CMD_NODE_NUM];
	u16 chip_eeprom_info;
	int min = 0xFFFF, max = -0xFF;

	zt_ts_esd_timer_stop(info);
	disable_irq(info->irq);

	if (pdata->mis_cal_check == 0) {
		input_info(true, &info->client->dev, "%s: [ERROR] not support\n", __func__);
		mis_cal_data = 0xF1;
		goto NG;
	}

	if (info->work_state == SUSPEND) {
		input_info(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",__func__);
		mis_cal_data = 0xF2;
		goto NG;
	}

	if (read_data(info->client, ZT_EEPROM_INFO, (u8 *)&chip_eeprom_info, 2) < 0) {
		input_info(true, &info->client->dev, "%s: read eeprom_info i2c fail!\n", __func__);
		mis_cal_data = 0xF3;
		goto NG;
	}

	if (zinitix_bit_test(chip_eeprom_info, 0)) {
		input_info(true, &info->client->dev, "%s: eeprom cal 0, skip !\n", __func__);
		mis_cal_data = 0xF1;
		goto NG;
	}

	ret = ts_set_touchmode(TOUCH_REF_ABNORMAL_TEST_MODE);
	if (ret < 0) {
		ts_set_touchmode(TOUCH_POINT_MODE);
		mis_cal_data = 0xF4;
		goto NG;
	}
	ret = get_raw_data(info, (u8 *)raw_data->reference_data_abnormal, 2);
	if (!ret) {
		input_info(true, &info->client->dev, "%s:[ERROR] i2c fail!\n", __func__);
		ts_set_touchmode(TOUCH_POINT_MODE);
		mis_cal_data = 0xF4;
		goto NG;
	}
	ts_set_touchmode(TOUCH_POINT_MODE);

	zt_display_rawdata_boot(info, raw_data, &min, &max, true);
	if ((min + DEF_MIS_CAL_SPEC_MID) < DEF_MIS_CAL_SPEC_MIN ||
			(max + DEF_MIS_CAL_SPEC_MID) > DEF_MIS_CAL_SPEC_MAX) {
		mis_cal_data = 0xFD;
		goto NG;
	}

	mis_cal_data = 0x00;
	input_info(true, &info->client->dev, "%s : mis_cal_data: %X\n", __func__, mis_cal_data);
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
	return;
NG:
	input_err(true, &info->client->dev, "%s : mis_cal_data: %X\n", __func__, mis_cal_data);
	if (mis_cal_data == 0xFD) {
		run_tsp_rawdata_read(&info->sec, 7, raw_data_buff);
		run_tsp_rawdata_read(&info->sec, TOUCH_REFERENCE_MODE, raw_data_buff);
	}
	enable_irq(info->irq);
	zt_ts_esd_timer_start(info);
	return;
}

static void zt_run_rawdata(struct zt_ts_info *info)
{
	info->tsp_dump_lock = 1;
	input_raw_data_clear();
	input_raw_info(true, &info->client->dev, "%s: start ##\n", __func__);
	zt_run_dnd(info);
	zt_run_mis_cal(info);
	input_raw_info(true, &info->client->dev, "%s: done ##\n", __func__);
	info->tsp_dump_lock = 0;
}

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
#include <linux/input/sec_tsp_dumpkey.h>
extern struct tsp_dump_callbacks dump_callbacks;
static struct delayed_work *p_ghost_check;

static void zt_check_rawdata(struct work_struct *work)
{
	struct zt_ts_info *info = container_of(work, struct zt_ts_info,
			ghost_check.work);

	if (info->tsp_dump_lock == 1) {
		input_info(true, &info->client->dev, "%s: ignored ## already checking..\n", __func__);
		return;
	}

	if (info->tsp_pwr_enabled == POWER_OFF) {
		input_info(true, &info->client->dev, "%s: ignored ## IC is power off\n", __func__);
		return;
	}

	zt_run_rawdata(info);
}

static void dump_tsp_log(void)
{
	pr_info("%s: %s %s: start\n", ZT_TS_DEVICE, SECLOG, __func__);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		pr_err("%s: %s %s: ignored ## lpm charging Mode!!\n", ZT_TS_DEVICE, SECLOG, __func__);
		return;
	}
#endif

	if (p_ghost_check == NULL) {
		pr_err("%s: %s %s: ignored ## tsp probe fail!!\n", ZT_TS_DEVICE, SECLOG, __func__);
		return;
	}
	schedule_delayed_work(p_ghost_check, msecs_to_jiffies(100));
}
#endif

static void zt_read_info_work(struct work_struct *work)
{
	struct zt_ts_info *info = container_of(work, struct zt_ts_info,
			work_read_info.work);
#ifdef TCLM_CONCEPT
	u8 data[2] = {0};
	int ret;
#endif

	mutex_lock(&info->modechange);

#ifdef TCLM_CONCEPT
	get_zt_tsp_nvm_data(info, ZT_TS_NVM_OFFSET_FAC_RESULT, (u8 *)data, 2);
	info->test_result.data[0] = data[0];

	zt_ts_esd_timer_stop(info);

	ret = sec_tclm_check_cal_case(info->tdata);
	input_info(true, &info->client->dev, "%s: sec_tclm_check_cal_case result %d; test result %X\n",
			__func__, ret, info->test_result.data[0]);

	zt_ts_esd_timer_start(info);
#endif
	run_test_open_short(info);

	input_log_fix();
	zt_run_rawdata(info);
	info->info_work_done = true;
	mutex_unlock(&info->modechange);
}

void zt_print_info(struct zt_ts_info *info)
{
	u16 fw_version = 0;

	if (!info)
		return;

	if (!info->client)
		return;

	info->print_info_cnt_open++;

	if (info->print_info_cnt_open > 0xfff0)
		info->print_info_cnt_open = 0;

	if (info->finger_cnt1 == 0)
		info->print_info_cnt_release++;

	fw_version =  ((info->cap_info.hw_id & 0xff) << 8) | (info->cap_info.reg_data_version & 0xff);

	input_info(true, &info->client->dev,
			"tc:%d noise:%s(%d) cover:%d lp:(%x) fod:%d ED:%d // v:%04X C%02XT%04X.%4s%s // #%d %d\n",
			info->finger_cnt1, info->noise_flag > 0 ? "ON":"OFF", info->noise_flag, info->flip_cover_flag,
			info->lpm_mode, info->fod_mode_set, info->ed_enable,
			fw_version,
#ifdef TCLM_CONCEPT
			info->tdata->nvdata.cal_count, info->tdata->nvdata.tune_fix_ver,
			info->tdata->tclm_string[info->tdata->nvdata.cal_position].f_name,
			(info->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ",
#else
			0,0," "," ",
#endif
			info->print_info_cnt_open, info->print_info_cnt_release);
}

static void touch_print_info_work(struct work_struct *work)
{
	struct zt_ts_info *info = container_of(work, struct zt_ts_info,
			work_print_info.work);

	zt_print_info(info);

	if (!shutdown_is_on_going_tsp)
		schedule_delayed_work(&info->work_print_info, msecs_to_jiffies(TOUCH_PRINT_INFO_DWORK_TIME));
}

static void zt_ts_set_input_prop(struct zt_ts_info *info, struct input_dev *dev, u8 propbit)
{
	static char zt_phys[64] = { 0 };

	snprintf(zt_phys, sizeof(zt_phys),
			"%s/input1", dev->name);
	dev->id.bustype = BUS_I2C;
	dev->phys = zt_phys;
	dev->dev.parent = &info->client->dev;

	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_KEY, dev->evbit);
	set_bit(EV_ABS, dev->evbit);
	set_bit(BTN_TOUCH, dev->keybit);
	set_bit(propbit, dev->propbit);
	set_bit(KEY_INT_CANCEL,dev->keybit);
	set_bit(EV_LED, dev->evbit);
	set_bit(LED_MISC, dev->ledbit);

	set_bit(KEY_BLACK_UI_GESTURE,dev->keybit);
	set_bit(KEY_WAKEUP, dev->keybit);


	input_set_abs_params(dev, ABS_MT_POSITION_X,
			0, info->pdata->x_resolution + ABS_PT_OFFSET,	0, 0);
	input_set_abs_params(dev, ABS_MT_POSITION_Y,
			0, info->pdata->y_resolution + ABS_PT_OFFSET,	0, 0);
#ifdef CONFIG_SEC_FACTORY
	input_set_abs_params(dev, ABS_MT_PRESSURE,
			0, 3000, 0, 0);
#endif
	input_set_abs_params(dev, ABS_MT_TOUCH_MAJOR,
			0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_WIDTH_MAJOR,
			0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MINOR,
			0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);

	set_bit(MT_TOOL_FINGER, dev->keybit);
	if (propbit == INPUT_PROP_POINTER)
		input_mt_init_slots(dev, MAX_SUPPORTED_FINGER_NUM, INPUT_MT_POINTER);
	else
		input_mt_init_slots(dev, MAX_SUPPORTED_FINGER_NUM, INPUT_MT_DIRECT);

	input_set_drvdata(dev, info);
}

static void zt_ts_set_input_prop_proximity(struct zt_ts_info *info, struct input_dev *dev)
{
	static char zt_phys[64] = { 0 };

	snprintf(zt_phys, sizeof(zt_phys), "%s/input1", dev->name);
	dev->phys = zt_phys;
	dev->id.bustype = BUS_I2C;
	dev->dev.parent = &info->client->dev;

	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_SW, dev->evbit);

	set_bit(INPUT_PROP_DIRECT, dev->propbit);

	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);
	input_set_abs_params(dev, ABS_MT_CUSTOM2, 0, 0xFFFFFFFF, 0, 0);
	input_set_drvdata(dev, info);
}

static int zt_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *i2c_id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct zt_ts_platform_data *pdata = client->dev.platform_data;
	struct sec_tclm_data *tdata = NULL;
	struct zt_ts_info *info;
	struct device_node *np = client->dev.of_node;
	int ret = 0;
	bool force_update = false;
	int lcdtype = 0;
#if defined(CONFIG_EXYNOS_DPU30)
	int connected;
#endif

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		input_err(true, &client->dev, "%s : Do not load driver due to : lpm %d\n",
				__func__, lpcharge);
		return -ENODEV;
	}
#endif

#ifdef CONFIG_DISPLAY_SAMSUNG
	lcdtype = get_lcd_attached("GET");
	if (lcdtype == 0xFFFFFF || ((lcdtype >> 8) != 0x8000)) {
		input_err(true, &client->dev, "%s: lcd is not attached %X\n", __func__, lcdtype);
		return -ENODEV;
	}
#endif

#if defined(CONFIG_EXYNOS_DPU30)
	connected = get_lcd_info("connected");
	if (connected < 0) {
		input_err(true, &client->dev, "%s: Failed to get lcd info\n", __func__);
		return -EINVAL;
	}

	if (!connected) {
		input_err(true, &client->dev, "%s: lcd is disconnected\n", __func__);
		return -ENODEV;
	}

	input_info(true, &client->dev, "%s: lcd is connected\n", __func__);

	lcdtype = get_lcd_info("id");
	if (lcdtype < 0 || ((lcdtype >> 8) != 0x8000)) {
		input_err(true, &client->dev, "%s: Failed to get lcd info %X\n", __func__, lcdtype);
		return -EINVAL;
	}
#endif
	input_info(true, &client->dev, "%s: lcdtype: %X\n", __func__, lcdtype);

	if (client->dev.of_node) {
		if (!pdata) {
			pdata = devm_kzalloc(&client->dev,
					sizeof(*pdata), GFP_KERNEL);
			if (!pdata)
				return -ENOMEM;
		}
		ret = zt_ts_parse_dt(np, &client->dev, pdata);
		if (ret) {
			input_err(true, &client->dev, "Error parsing dt %d\n", ret);
			goto err_no_platform_data;
		}
		tdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_tclm_data), GFP_KERNEL);
		if (!tdata)
			goto error_allocate_tdata;

		sec_tclm_parse_dt(client, tdata);

		ret = zinitix_init_gpio(pdata);
		if (ret < 0)
			goto err_gpio_request;

	} else if (!pdata) {
		input_err(true, &client->dev, "%s: Not exist platform data\n", __func__);
		return -EINVAL;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev, "%s: Not compatible i2c function\n", __func__);
		return -EIO;
	}

	info = kzalloc(sizeof(struct zt_ts_info), GFP_KERNEL);
	if (!info) {
		input_err(true, &client->dev, "%s: Failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	i2c_set_clientdata(client, info);
	info->client = client;
	info->pdata = pdata;

	info->tdata = tdata;
	if (!info->tdata)
		goto error_null_data;

#ifdef TCLM_CONCEPT
	sec_tclm_initialize(info->tdata);
	info->tdata->client = info->client;
	info->tdata->tclm_read = zt_tclm_data_read;
	info->tdata->tclm_write = zt_tclm_data_write;
	info->tdata->tclm_execute_force_calibration = zt_tclm_execute_force_calibration;
#endif
	INIT_DELAYED_WORK(&info->work_read_info, zt_read_info_work);
	INIT_DELAYED_WORK(&info->work_print_info, touch_print_info_work);

	mutex_init(&info->modechange);
	mutex_init(&info->set_reg_lock);
	mutex_init(&info->set_lpmode_lock);
	mutex_init(&info->work_lock);
	mutex_init(&info->raw_data_lock);
	mutex_init(&info->i2c_mutex);
	mutex_init(&info->sponge_mutex);
	mutex_init(&info->power_init);
#if ESD_TIMER_INTERVAL
	mutex_init(&info->lock);
	INIT_WORK(&info->tmr_work, ts_tmr_work);
#endif

	info->input_dev = input_allocate_device();
	if (!info->input_dev) {
		input_err(true, &client->dev, "%s: Failed to allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_alloc;
	}

	if (pdata->support_dex) {
		info->input_dev_pad = input_allocate_device();
		if (!info->input_dev_pad) {
			input_err(true, &client->dev, "%s: allocate device err!\n", __func__);
			ret = -ENOMEM;
			goto err_allocate_input_dev_pad;
		}
	}

	if (pdata->support_ear_detect) {
		info->input_dev_proximity = input_allocate_device();
		if (!info->input_dev_proximity) {
			input_err(true, &client->dev, "%s: allocate input_dev_proximity err!\n", __func__);
			ret = -ENOMEM;
			goto err_allocate_input_dev_proximity;
		}

		info->input_dev_proximity->name = "sec_touchproximity";
		zt_ts_set_input_prop_proximity(info, info->input_dev_proximity);
	}

	info->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(info->pinctrl)) {
		input_err(true, &client->dev, "%s: Failed to get pinctrl data\n", __func__);
		ret = PTR_ERR(info->pinctrl);
		goto err_get_pinctrl;
	}

	info->work_state = PROBE;

	// power on
	if (zt_power_control(info, POWER_ON_SEQUENCE) == false) {
		input_err(true, &info->client->dev,
				"%s: POWER_ON_SEQUENCE failed", __func__);
		force_update = true;
	}

	/* init touch mode */
	info->touch_mode = TOUCH_POINT_MODE;
	misc_info = info;

#if ESD_TIMER_INTERVAL
	esd_tmr_workqueue =
		create_singlethread_workqueue("esd_tmr_workqueue");

	if (!esd_tmr_workqueue) {
		input_err(true, &client->dev, "%s: Failed to create esd tmr work queue\n", __func__);
		ret = -EPERM;

		goto err_esd_sequence;
	}

	esd_timer_init(info);
#endif

	ret = ic_version_check(info);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: fail version check", __func__);
		force_update = true;
	}

	ret = fw_update_work(info, force_update);
	if (ret < 0) {
		ret = -EPERM;
		input_err(true, &info->client->dev,
				"%s: fail update_work", __func__);
		goto err_fw_update;
	}

	info->input_dev->name = "sec_touchscreen";
	zt_ts_set_input_prop(info, info->input_dev, INPUT_PROP_DIRECT);
	ret = input_register_device(info->input_dev);
	if (ret) {
		input_info(true, &client->dev, "unable to register %s input device\r\n",
				info->input_dev->name);
		goto err_input_register_device;
	}

	if (pdata->support_dex) {
		info->input_dev_pad->name = "sec_touchpad";
		zt_ts_set_input_prop(info, info->input_dev_pad, INPUT_PROP_POINTER);
	}

	if (pdata->support_dex) {
		ret = input_register_device(info->input_dev_pad);
		if (ret) {
			input_err(true, &client->dev, "%s: Unable to register %s input device\n", __func__, info->input_dev_pad->name);
			goto err_input_pad_register_device;
		}
	}

	if (pdata->support_ear_detect) {
		ret = input_register_device(info->input_dev_proximity);
		if (ret) {
			input_err(true, &client->dev, "%s: Unable to register %s input device\n",
					__func__, info->input_dev_proximity->name);
			goto err_input_proximity_register_device;
		}
	}

	if (init_touch(info) == false) {
		ret = -EPERM;
		goto err_init_touch;
	}

	info->work_state = NOTHING;

	init_completion(&info->resume_done);
	complete_all(&info->resume_done);

	/* configure irq */
	info->irq = gpio_to_irq(pdata->gpio_int);
	if (info->irq < 0) {
		input_info(true, &client->dev, "%s: error. gpio_to_irq(..) function is not \
				supported? you should define GPIO_TOUCH_IRQ.\r\n", __func__);
		ret = -EINVAL;
		goto error_gpio_irq;
	}

	/* ret = request_threaded_irq(info->irq, ts_int_handler, zt_touch_work,*/
	ret = request_threaded_irq(info->irq, NULL, zt_touch_work,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT , ZT_TS_DEVICE, info);

	if (ret) {
		input_info(true, &client->dev, "unable to register irq.(%s)\r\n",
				info->input_dev->name);
		goto err_request_irq;
	}
	input_info(true, &client->dev, "%s\n", __func__);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	trustedui_set_tsp_irq(info->irq);
	input_info(true, &client->dev, "%s[%d] called!\n",
			__func__, info->irq);
#endif

#ifdef CONFIG_INPUT_ENABLED
	info->input_dev->open = zt_ts_open;
	info->input_dev->close = zt_ts_close;
#endif

#ifdef USE_MISC_DEVICE
	ret = misc_register(&touch_misc_device);
	if (ret) {
		input_err(true, &client->dev, "%s: Failed to register touch misc device\n", __func__);
		goto err_misc_register;
	}
#endif
	ret = init_sec_factory(info);
	if (ret) {
		input_err(true, &client->dev, "%s: Failed to init sec factory device\n", __func__);

		goto err_kthread_create_failed;
	}

	info->register_cb = info->pdata->register_cb;

	info->callbacks.inform_charger = zt_charger_status_cb;
	if (info->register_cb)
		info->register_cb(&info->callbacks);
#ifdef CONFIG_VBUS_NOTIFIER
	vbus_notifier_register(&info->vbus_nb, tsp_vbus_notification,
			VBUS_NOTIFY_DEV_CHARGER);
#endif
	device_init_wakeup(&client->dev, true);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	tui_tsp_info = info;
#endif
#ifdef CONFIG_SAMSUNG_TUI
	tui_tsp_info = info;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (sysfs_create_group(&info->input_dev->dev.kobj, &secure_attr_group) < 0)
		input_err(true, &info->client->dev, "%s: do not make secure group\n", __func__);
	else
		secure_touch_init(info);
#endif
	ts_check_custom_library(info);

	schedule_delayed_work(&info->work_read_info, msecs_to_jiffies(5000));

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	dump_callbacks.inform_dump = dump_tsp_log;
	INIT_DELAYED_WORK(&info->ghost_check, zt_check_rawdata);
	p_ghost_check = &info->ghost_check;
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (info->pdata->ss_touch_num > 0)
		sec_secure_touch_register(info, info->pdata->ss_touch_num, &info->input_dev->dev.kobj);
#endif
	input_log_fix();
	return 0;

err_kthread_create_failed:
	sec_cmd_exit(&info->sec, SEC_CLASS_DEVT_TSP);
	kfree(info->raw_data);
#ifdef USE_MISC_DEVICE
	misc_deregister(&touch_misc_device);
err_misc_register:
#endif
	free_irq(info->irq, info);
err_request_irq:
error_gpio_irq:
err_init_touch:
	if (pdata->support_ear_detect) {
		input_unregister_device(info->input_dev_proximity);
		info->input_dev_proximity = NULL;
	}
err_input_proximity_register_device:
	if (pdata->support_dex) {
		input_unregister_device(info->input_dev_pad);
		info->input_dev_pad = NULL;
	}
err_input_pad_register_device:
	input_unregister_device(info->input_dev);
	info->input_dev = NULL;
err_input_register_device:
err_fw_update:
#if ESD_TIMER_INTERVAL
	del_timer(&(info->esd_timeout_tmr));
err_esd_sequence:
#endif
	zt_power_control(info, POWER_OFF);
err_get_pinctrl:
	if (pdata->support_ear_detect) {
		if (info->input_dev_proximity)
			input_free_device(info->input_dev_proximity);
	}
err_allocate_input_dev_proximity:
	if (pdata->support_dex) {
		if (info->input_dev_pad)
			input_free_device(info->input_dev_pad);
	}
err_allocate_input_dev_pad:
	if (info->input_dev)
		input_free_device(info->input_dev);
error_null_data:
err_alloc:
	kfree(info);
err_gpio_request:
error_allocate_tdata:
	if (IS_ENABLED(CONFIG_OF))
		devm_kfree(&client->dev, (void *)tdata);
err_no_platform_data:
	if (IS_ENABLED(CONFIG_OF))
		devm_kfree(&client->dev, (void *)pdata);

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	p_ghost_check = NULL;
#endif
	input_info(true, &client->dev, "%s: Failed to probe\n", __func__);
	input_log_fix();
	return ret;
}

static int zt_ts_remove(struct i2c_client *client)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);
	struct zt_ts_platform_data *pdata = info->pdata;

	disable_irq(info->irq);
	mutex_lock(&info->work_lock);

	info->work_state = REMOVE;

	cancel_delayed_work_sync(&info->work_read_info);
	cancel_delayed_work_sync(&info->work_print_info);

	sec_cmd_exit(&info->sec, SEC_CLASS_DEVT_TSP);
	kfree(info->raw_data);

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	p_ghost_check = NULL;
#endif

#if ESD_TIMER_INTERVAL
	flush_work(&info->tmr_work);
	write_reg(info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL, 0);
	esd_timer_stop(info);
#if defined(TSP_VERBOSE_DEBUG)
	input_info(true, &client->dev, "%s: Stopped esd timer\n", __func__);
#endif
	destroy_workqueue(esd_tmr_workqueue);
#endif

	if (info->irq)
		free_irq(info->irq, info);
#ifdef USE_MISC_DEVICE
	misc_deregister(&touch_misc_device);
#endif

	if (gpio_is_valid(pdata->gpio_int) != 0)
		gpio_free(pdata->gpio_int);

	if (pdata->support_dex) {
		input_mt_destroy_slots(info->input_dev_pad);
		input_unregister_device(info->input_dev_pad);
	}

	if (pdata->support_ear_detect) {
		input_mt_destroy_slots(info->input_dev_proximity);
		input_unregister_device(info->input_dev_proximity);
	}

	input_unregister_device(info->input_dev);
	input_free_device(info->input_dev);
	mutex_unlock(&info->work_lock);
	kfree(info);

	return 0;
}

void zt_ts_shutdown(struct i2c_client *client)
{
	struct zt_ts_info *info = i2c_get_clientdata(client);

	input_info(true, &client->dev, "%s++\n",__func__);
	shutdown_is_on_going_tsp = true;
	disable_irq(info->irq);
	mutex_lock(&info->work_lock);
#if ESD_TIMER_INTERVAL
	flush_work(&info->tmr_work);
	esd_timer_stop(info);
#endif
	mutex_unlock(&info->work_lock);
	zt_power_control(info, POWER_OFF);
	input_info(true, &client->dev, "%s--\n",__func__);
}

#ifdef CONFIG_SAMSUNG_TUI
extern int stui_i2c_lock(struct i2c_adapter *adap);
extern int stui_i2c_unlock(struct i2c_adapter *adap);

int stui_tsp_enter(void)
{
	int ret = 0;

	if (!tui_tsp_info)
		return -EINVAL;

#if ESD_TIMER_INTERVAL
	esd_timer_stop(tui_tsp_info);
	write_reg(tui_tsp_info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL, 0);
#endif

	disable_irq(tui_tsp_info->irq);
	clear_report_data(tui_tsp_info);

	ret = stui_i2c_lock(tui_tsp_info->client->adapter);
	if (ret) {
		pr_err("[STUI] stui_i2c_lock failed : %d\n", ret);
		enable_irq(tui_tsp_info->client->irq);
		return -1;
	}

	return 0;
}

int stui_tsp_exit(void)
{
	int ret = 0;

	if (!tui_tsp_info)
		return -EINVAL;

	ret = stui_i2c_unlock(tui_tsp_info->client->adapter);
	if (ret)
		pr_err("[STUI] stui_i2c_unlock failed : %d\n", ret);

	enable_irq(tui_tsp_info->irq);

#if ESD_TIMER_INTERVAL
	write_reg(tui_tsp_info->client, ZT_PERIODICAL_INTERRUPT_INTERVAL,
			SCAN_RATE_HZ * ESD_TIMER_INTERVAL);
	esd_timer_start(CHECK_ESD_TIMER, tui_tsp_info);
#endif

	return ret;
}
#endif

#ifdef CONFIG_PM
static int zt_ts_pm_suspend(struct device *dev)
{
	struct zt_ts_info *info = dev_get_drvdata(dev);

	reinit_completion(&info->resume_done);

	return 0;
}

static int zt_ts_pm_resume(struct device *dev)
{
	struct zt_ts_info *info = dev_get_drvdata(dev);

	complete_all(&info->resume_done);

	return 0;
}

static const struct dev_pm_ops zt_ts_dev_pm_ops = {
	.suspend = zt_ts_pm_suspend,
	.resume = zt_ts_pm_resume,
};
#endif

static struct i2c_device_id zt_idtable[] = {
	{ZT_TS_DEVICE, 0},
	{ }
};

#ifdef CONFIG_OF
static const struct of_device_id zinitix_match_table[] = {
	{ .compatible = "zinitix,zt_ts_device",},
	{},
};
#endif

static struct i2c_driver zt_ts_driver = {
	.probe	= zt_ts_probe,
	.remove	= zt_ts_remove,
	.shutdown = zt_ts_shutdown,
	.id_table	= zt_idtable,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= ZT_TS_DEVICE,
#ifdef CONFIG_OF
		.of_match_table = zinitix_match_table,
#endif
#ifdef CONFIG_PM
		.pm = &zt_ts_dev_pm_ops,
#endif
	},
};

static int __init zt_ts_init(void)
{
	pr_info("[TSP]: %s\n", __func__);
	return i2c_add_driver(&zt_ts_driver);
}

static void __exit zt_ts_exit(void)
{
	i2c_del_driver(&zt_ts_driver);
}

module_init(zt_ts_init);
module_exit(zt_ts_exit);

MODULE_DESCRIPTION("touch-screen device driver using i2c interface");
MODULE_AUTHOR("<mika.kim@samsung.com>");
MODULE_LICENSE("GPL");
