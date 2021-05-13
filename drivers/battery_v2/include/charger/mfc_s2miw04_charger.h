/*
 * MFC_charger.h
 * Samsung MFC Charger Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __WIRELESS_CHARGER_MFC_S2MIW04_H
#define __WIRELESS_CHARGER_MFC_S2MIW04_H __FILE__

#include <linux/mfd/core.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/alarmtimer.h>
#include "../sec_charging_common.h"

#define MTP_MAX_PROGRAM_SIZE 0x4000
#define MTP_VERIFY_ADDR			0x0000
#define MTP_VERIFY_SIZE			0x4680
#define MTP_VERIFY_CHKSUM		0x02D26
#define MFC_FW_VER_BIN_LSI  	0xBFC

#define MFC_FLASH_FW_HEX_PATH		"mfc/mfc_fw_flash.bin"
#define MFC_FW_SDCARD_BIN_PATH		"/sdcard/mfc_fw_flash.bin"

#define MFC_FW_BIN_VERSION			0x1020
#define MFC_FLASH_FW_HEX_LSI_PATH	"mfc/mfc_fw_flash_s2miw04.bin"

/* for SPU FW update */
#define MFC_FW_SPU_BIN_PATH		"/spu/mfc/mfc_fw_flash_s2miw04.bin"

#define MFC_CHIP_ID_S2MIW04		0x04

/* REGISTER MAPS */
#define MFC_CHIP_ID_L_REG					0x00
#define MFC_CHIP_ID_H_REG					0x01
#define MFC_CHIP_REVISION_REG				0x02
#define MFC_CUSTOMER_ID_REG					0x03
#define MFC_FW_MAJOR_REV_L_REG				0x04
#define MFC_FW_MAJOR_REV_H_REG				0x05
#define MFC_FW_MINOR_REV_L_REG				0x06
#define MFC_FW_MINOR_REV_H_REG				0x07
#define MFC_PRMC_ID_L_REG					0x0A
#define MFC_PRMC_ID_H_REG					0x0B
/* RXID BIT[0:47] */
#define MFC_WPC_RXID_0_REG					0x10
#define MFC_WPC_RXID_1_REG					0x11
#define MFC_WPC_RXID_2_REG					0x12
#define MFC_WPC_RXID_3_REGs					0x13
#define MFC_WPC_RXID_4_REG					0x14
#define MFC_WPC_RXID_5_REG					0x15

#define MFC_STATUS_L_REG					0x20
#define MFC_STATUS_H_REG					0x21
#define MFC_INT_A_L_REG						0x22
#define MFC_INT_A_H_REG						0x23
#define MFC_INT_A_ENABLE_L_REG				0x24
#define MFC_INT_A_ENABLE_H_REG				0x25
#define MFC_INT_A_CLEAR_L_REG				0x26
#define MFC_INT_A_CLEAR_H_REG				0x27

#define MFC_CTRL_STS_REG					0x28

#define MFC_SYS_OP_MODE_REG					0x2B
#define MFC_BATTERY_CHG_STATUS_REG			0x3A
#define MFC_EPT_REG							0x3B /* EPT(End of Power Transfer) cases. PMA has only EOC case */
#define MFC_ADC_VOUT_L_REG					0x3C
#define MFC_ADC_VOUT_H_REG					0x3D
/* s2miw04: VOUT_SET_L / VOUT_SET_H */
#define MFC_VOUT_SET_L_REG					0x3E
#define MFC_VOUT_SET_H_REG					0x3F
#define MFC_VRECT_ADJ_REG					0x39
#define MFC_ADC_VRECT_L_REG					0x40
#define MFC_ADC_VRECT_H_REG					0x41
#define MFC_TX_IUNO_LIMIT_L_REG				0x34
#define MFC_TX_IUNO_LIMIT_H_REG				0x35
#define MFC_ADC_IOUT_L_REG					0x44
#define MFC_ADC_IOUT_H_REG					0x45
#define MFC_ADC_DIE_TEMP_L_REG				0x46 /* 8 LSB field is used, Celsius */
#define MFC_ADC_DIE_TEMP_H_REG				0x47 /* only 4 MSB[3:0] field is used, Celsius */
#define MFC_TRX_OP_FREQ_L_REG				0x48 /* kHZ */
#define MFC_TRX_OP_FREQ_H_REG				0x49 /* kHZ */
#define MFC_RX_PING_FREQ_L_REG				0x4A /* kHZ */
#define MFC_RX_PING_FREQ_H_REG				0x4B /* kHZ */
#define MFC_ILIM_SET_REG					0x4C /* ILim =  value * 0.05(A) + 0.05(A) */
#define MFC_ILIM_ADJ_REG					0x4D /* AdjVal = ILIM_ADJ * 50 */

#define MFC_AP2MFC_CMD_L_REG				0x4E
#define MFC_AP2MFC_CMD_H_REG				0x4F

/********************************************************************************/
/* Below register are functionally depends on the operation mode(TX or RX mode) */
/* RX mode */
#define MFC_WPC_PCKT_HEADER_REG				0x50
#define MFC_WPC_RX_DATA_COM_REG				0x51 /* WPC Rx to Tx COMMAND */
#define MFC_WPC_RX_DATA_VALUE0_REG			0x52
#define MFC_WPC_RX_DATA_VALUE1_REG			0x53
#define MFC_WPC_RX_DATA_VALUE2_REG			0x54
#define MFC_WPC_RX_DATA_VALUE3_REG			0x55

/* TX mode */
#define MFC_WPC_TX_DATA_COM_REG				0x50 /* WPC Tx to Rx COMMAND */
#define MFC_WPC_TX_DATA_VALUE0_REG			0x51
#define MFC_WPC_TX_DATA_VALUE1_REG			0x52
#define MFC_WPC_TX_DATA_VALUE2_REG			0x53

/* Common */
#define MFC_WPC_TRX_DATA2_COM_REG			0x58
#define MFC_WPC_TRX_DATA2_VALUE0_REG		0x59
#define MFC_WPC_TRX_DATA2_VALUE1_REG		0x5A
/********************************************************************************/

#define MFC_ADT_TIMEOUT_PKT_REG				0x5C
#define MFC_ADT_TIMEOUT_STR_REG				0x5D

#define MFC_TX_IUNO_HYS_REG					0x36
#define MFC_TX_IUNO_OFFSET_L_REG			0x37
#define MFC_TX_IUNO_OFFSET_H_REG			0x38

#define MFC_TX_OC_FOD1_LIMIT_L_REG			0x94
#define MFC_TX_OC_FOD1_LIMIT_H_REG			0x95
#define MFC_TX_OC_FOD2_LIMIT_L_REG			0x96
#define MFC_TX_OC_FOD2_LIMIT_H_REG			0x97

#define MFC_STARTUP_EPT_COUNTER				0x6D

#define MFC_TX_DUTY_CYCLE					0xE6 /* default 0x80(50%) */

/* TX Max Operating Frequency = 60 MHz/value, default is 148kHz (60MHz/0x195=148KHz) */
#define MFC_TX_MAX_OP_FREQ_L_REG			0x60 /* default 0x95 */
#define MFC_TX_MAX_OP_FREQ_H_REG			0x61 /* default 0x01 */
/* TX Min Operating Frequency = 60 MHz/value, default is 80kHz (60MHz/0x2EE=80KHz) */
#define MFC_TX_MIN_OP_FREQ_L_REG			0x62 /* default 0xEE */
#define MFC_TX_MIN_OP_FREQ_H_REG			0x63 /* default 0x2 */

#define TX_MIN_OP_FREQ_DEFAULT	1130

/* TX Digital Ping Frequency = 60 MHz/value, default is 90kHz (60MHz/0x29B=90KHz) */
#define MFC_TX_PING_FREQ_L_REG				0x64 /* default 0x9B */
#define MFC_TX_PING_FREQ_H_REG				0x65 /* default 0x02 */
/* TX Mode Minimum Duty Setting Register, Min_Duty, default is 50% (0x32=50) */
#define MFC_TX_MIN_DUTY_SETTING_REG			0x66 /* default 0x32 */

#define MFC_INVERTER_CTRL_REG				0x67
#define MFC_CMFET_CTRL_REG					0x68

/* RX Mode Communication Modulation FET Ctrl */
#define MFC_MST_MODE_SEL_REG				0x69
#define MFC_RX_OV_CLAMP_REG					0x6A
//#define MFC_RX_COMM_MOD_AFC_FET_REG		0x37
#define MFC_RX_COMM_MOD_FET_REG				0x6B

#define MFC_RECTMODE_REG					0x6C
#define MFC_START_EPT_COUNTER_REG			0x6D
#define MFC_CTRL_MODE_REG					0x6E
#define MFC_RC_PHM_PING_PERIOD_REG			0x6F

#define MFC_WPC_FOD_0A_REG					0x70
#define MFC_WPC_FOD_0B_REG					0x71
#define MFC_WPC_FOD_1A_REG					0x72
#define MFC_WPC_FOD_1B_REG					0x73
#define MFC_WPC_FOD_2A_REG					0x74
#define MFC_WPC_FOD_2B_REG					0x75
#define MFC_WPC_FOD_3A_REG					0x76
#define MFC_WPC_FOD_3B_REG					0x77
#define MFC_WPC_FOD_4A_REG					0x78
#define MFC_WPC_FOD_4B_REG					0x79
#define MFC_WPC_FOD_5A_REG					0x7A
#define MFC_WPC_FOD_5B_REG					0x7B
#define MFC_WPC_FOD_6A_REG					0x7C
#define MFC_WPC_FOD_6B_REG					0x7D
#define MFC_WPC_FOD_7A_REG					0x7E
#define MFC_WPC_FOD_7B_REG					0x7F
#define MFC_WPC_FOD_8A_REG					0x80
#define MFC_WPC_FOD_8B_REG					0x81
#define MFC_WPC_FOD_9A_REG					0x82
#define MFC_WPC_FOD_9B_REG					0x83

#define MFC_PMA_FOD_0A_REG					0x84
#define MFC_PMA_FOD_0B_REG					0x85
#define MFC_PMA_FOD_1A_REG					0x86
#define MFC_PMA_FOD_1B_REG					0x87
#define MFC_PMA_FOD_2A_REG					0x88
#define MFC_PMA_FOD_2B_REG					0x89
#define MFC_PMA_FOD_3A_REG					0x8A
#define MFC_PMA_FOD_3B_REG					0x8B

#define MFC_ADT_ERROR_CODE_REG				0x8D

#define MFC_TX_FOD_GAIN_REG					0x8F
#define MFC_TX_FOD_OFFSET_L_REG				0x90
#define MFC_TX_FOD_OFFSET_H_REG				0x91
#define MFC_TX_FOD_THRESH1_L_REG			0x92
#define MFC_TX_FOD_THRESH1_H_REG			0x93

#define MFX_TX_ID_VALUE_L_REG				0x9C
#define MFX_TX_ID_VALUE_H_REG				0x9D

#define MFC_DEMOD1_REG						0x9E
#define MFC_DEMOD2_REG						0x9F

#define MFC_TX_CONFLICT_CURRENT_REG			0xA0

#define MFC_FW_DATA_CODE_0					0xB0
#define MFC_FW_DATA_CODE_1					0xB1
#define MFC_FW_DATA_CODE_2					0xB2
#define MFC_FW_DATA_CODE_3					0xB3
#define MFC_FW_DATA_CODE_4					0xB4
#define MFC_FW_DATA_CODE_5					0xB5
#define MFC_FW_DATA_CODE_6					0xB6
#define MFC_FW_DATA_CODE_7					0xB7
#define MFC_FW_DATA_CODE_8					0xB8
#define MFC_FW_DATA_CODE_9					0xB9
#define MFC_FW_DATA_CODE_A					0xBA

#define MFC_RX_PWR_L_REG					0xBC
#define MFC_RX_PWR_H_REG					0xBD
/* Timer code contains ASCII value. (ex. 31 means '1', 3A means ':') */
#define MFC_FW_TIMER_CODE_0					0xC0
#define MFC_FW_TIMER_CODE_1					0xC1
#define MFC_FW_TIMER_CODE_2					0xC2
#define MFC_FW_TIMER_CODE_3					0xC3
#define MFC_FW_TIMER_CODE_4					0xC4
#define MFC_FW_TIMER_CODE_5					0xC5
#define MFC_FW_TIMER_CODE_6					0xC6
#define MFC_FW_TIMER_CODE_7					0xC7

#define MFC_PWR_HOLD_INTERVAL_REG			0xCF

#define MFC_TX_FOD_THRESH2_REG				0xE3
#define MFC_TX_DUTY_CYCLE_REG				0xE6

#define MFC_TX_PWR_L_REG					0xEC
#define MFC_TX_PWR_H_REG					0xED

#define MFC_RPP_SCALE_COEF_REG					0xF0
#define MFC_ACTIVE_LOAD_CONTROL_REG				0xF1
/* Parameter 1: Major and Minor Version */
#define MFC_TX_RXID1_READ_REG					0xF2
/* Parameter 2~3: Manufacturer Code */
#define MFC_TX_RXID2_READ_REG					0xF3
#define MFC_TX_RXID3_READ_REG					0xF4
/* Parameter 4~10: Basic Device Identifier */
#define MFC_TX_RXID4_READ_REG					0xF5
#define MFC_TX_RXID5_READ_REG					0xF6
#define MFC_TX_RXID6_READ_REG					0xF7
#define MFC_TX_RXID7_READ_REG					0xF8
#define MFC_TX_RXID8_READ_REG					0xF9
#define MFC_TX_RXID9_READ_REG					0xFA
#define MFC_TX_RXID10_READ_REG					0xFB

/* Target Vrect is ReadOnly register, and updated by every 10ms
	Its default value is 0x1A90(6800mV).
	Target_Vrect (Iout,Vout) = {Vout + 0.05} + { Vrect(Iout,5V)-Vrect(1A,5V) } * 5/9
	*/
#define MFC_TARGET_VRECT_L_REG				0x015B /* default 0x90 */
#define MFC_TARGET_VRECT_H_REG				0x015C /* default 0x1A */

#define MFC_RX_CEP_PACKET_COUNTER0			0x029C
#define MFC_RX_CEP_PACKET_COUNTER1			0x029D
#define MFC_RX_CEP_PACKET_COUNTER2			0x029E
#define MFC_RX_CEP_PACKET_COUNTER3			0x029F
#define MFC_RX_RPP_PACKET_COUNTER0			0x02A0
#define MFC_RX_RPP_PACKET_COUNTER1			0x02A1
#define MFC_RX_RPP_PACKET_COUNTER2			0x02A2
#define MFC_RX_RPP_PACKET_COUNTER3			0x02A3
#define MFC_RX_CSP_PACKET_COUNTER0			0x02A4
#define MFC_RX_CSP_PACKET_COUNTER1			0x02A5
#define MFC_RX_CSP_PACKET_COUNTER2			0x02A6
#define MFC_RX_CSP_PACKET_COUNTER3			0x02A7
#define MFC_RX_PPP_PACKET_COUNTER0			0x02A8
#define MFC_RX_PPP_PACKET_COUNTER1			0x02A9
#define MFC_RX_PPP_PACKET_COUNTER2			0x02AA
#define MFC_RX_PPP_PACKET_COUNTER3			0x02AB

/* ADT Buffer Registers, (0x0800 ~ 0x0FFF) */
#define MFC_ADT_BUFFER_ADT_TYPE_REG				0x0800
#define MFC_ADT_BUFFER_ADT_MSG_SIZE_REG			0x0801
#define MFC_ADT_BUFFER_ADT_PARAM_REG			0x0804
#define MFC_ADT_BUFFER_ADT_PARAM_MAX_REG		0x0FFF

/* System Operating Mode Register, Sys_Op_Mode (0x2B) */
#define PAD_MODE_MISSING			0
#define PAD_MODE_WPC_BASIC			1
#define PAD_MODE_WPC_ADV			2
#define PAD_MODE_PMA_SR1			3
#define PAD_MODE_PMA_SR1E			4
#define PAD_MODE_UNKNOWN			5

/* MFC_RX_DATA_COM_REG (0x51) : RX Data Command VALUE of 0x19 PPP Heaader */
#define	WPC_COM_CLEAR_PACKET_COUNTING		0x01
#define	WPC_COM_START_PACKET_COUNTING		0x02
#define	WPC_COM_DISABLE_PACKET_COUNTING		0x03

/* RX Data Value1 Register (Data Sending), RX_Data_VALUE1_Out (0x51) : Function and Description */
#define	WPC_COM_UNKNOWN					0x00
#define	WPC_COM_TX_ID					0x01
#define	WPC_COM_CHG_STATUS				0x05
#define	WPC_COM_AFC_SET					0x06
#define	WPC_COM_AFC_DEBOUNCE			0x07 /* Data Values [ 0~1000mV : 0x0000~0x03E8 ], 2 bytes*/
#define	WPC_COM_SID_TAG					0x08
#define	WPC_COM_SID_TOKEN				0x09
#define	WPC_COM_TX_STANDBY				0x0A
#define	WPC_COM_LED_CONTROL				0x0B /* Data Value LED Enable(0x00), LED Disable(0xFF) */
#define	WPC_COM_REQ_AFC_TX				0x0C /* Data Value (0x00) */
#define	WPC_COM_COOLING_CTRL			0x0D /* Data Value ON(0x00), OFF(0xFF) */
#define	WPC_COM_RX_ID					0x0E /* Received RX ID */
#define	WPC_COM_CHG_LEVEL				0x0F /* Battery level */
#define	WPC_COM_ENTER_PHM				0x18 /* GEAR entered PHM */
#define	WPC_COM_DISABLE_TX				0x19 /* Turn off UNO of TX, OFF(0xFF) */

/* RX Data Value 2~5 Register (Data Sending), RX_Data_Value2_5_Out : Function and Description */
#define	RX_DATA_VAL2_5V					0x05
#define	RX_DATA_VAL2_10V				0x2C
#define	RX_DATA_VAL2_12V				0x4B
#define	RX_DATA_VAL2_12_5V				0x69
#define	RX_DATA_VAL2_20V				0x9B

/* MFC_TX_DATA_COM_REG (0x58) : TX Command */
#define	WPC_TX_COM_UNKNOWN		0x00
#define	WPC_TX_COM_TX_ID		0x01
#define	WPC_TX_COM_AFC_SET		0x02
#define	WPC_TX_COM_ACK			0x03
#define	WPC_TX_COM_NAK			0x04
#define WPC_TX_COM_CHG_ERR		0x05
#define WPC_TX_COM_WPS		0x07
#define WPC_TX_COM_RX_POWER		0x0A

/* value of WPC_TX_COM_AFC_SET(0x02) */
#define TX_AFC_SET_5V			0x00
#define TX_AFC_SET_10V			0x01
#define TX_AFC_SET_12V			0x02
#define TX_AFC_SET_18V			0x03
#define TX_AFC_SET_19V			0x04
#define TX_AFC_SET_20V			0x05
#define TX_AFC_SET_24V			0x06

/* value of WPC_TX_COM_TX_ID(0x01) */
#define TX_ID_UNKNOWN				0x00
#define TX_ID_SNGL_PORT_START		0x01
#define TX_ID_VEHICLE_PAD			0x11
#define TX_ID_SNGL_PORT_END			0x1F
#define TX_ID_MULTI_PORT_START		0x20
#define TX_ID_MULTI_PORT_END		0x2F
#define TX_ID_STAND_TYPE_START		0x30
#define TX_ID_STAND_TYPE_END		0x3F
#define TX_ID_BATT_PACK_TA			0x41 /* 0x40 ~ 0x41 is N/C*/
#define TX_ID_BATT_PACK				0x42
#define TX_ID_BATT_PACK_END			0x4F /* reserved 0x40 ~ 0x4F for wireless battery pack */
#define TX_ID_DREAM_STAND			0x31
#define TX_ID_DREAM_DOWN			0x14
#define TX_ID_UNO_TX				0x72
#define TX_ID_UNO_TX_B0				0x80
#define TX_ID_UNO_TX_B1				0x81
#define TX_ID_UNO_TX_B2				0x82
#define TX_ID_UNO_TX_MAX			0x9F

#define TX_ID_AUTH_PAD				0xA0
#define TX_ID_DAVINCI_PAD_V			0xA1
#define TX_ID_DAVINCI_PAD_H			0xA2
#define TX_ID_AUTH_PAD_ACLASS_END	0xAF
#define TX_ID_AUTH_PAD_END			0xBF /* reserved 0xA1 ~ 0xBF for auth pad */
#define TX_ID_NON_AUTH_PAD			0xF0
#define TX_ID_NON_AUTH_PAD_END		0xFF

/* value of WPC_TX_COM_CHG_ERR(0x05) */
#define TX_CHG_ERR_OTP			0x12
#define TX_CHG_ERR_OCP			0x13
#define TX_CHG_ERR_DARKZONE		0x14
#define TX_CHG_ERR_FOD			0x20 ... 0x27

/* value of WPC_TX_COM_WPS 0x07) */
#define WPS_AICL_RESET		0x01

/* value of WPC_TX_COM_RX_POWER(0x0A) */
#define TX_RX_POWER_0W			0x0
#define TX_RX_POWER_3W			0x1E
#define TX_RX_POWER_5W			0x32
#define TX_RX_POWER_6_5W		0x41
#define TX_RX_POWER_7_5W		0x4B
#define TX_RX_POWER_10W			0x64
#define TX_RX_POWER_12W			0x78
#define TX_RX_POWER_15W			0x96
#define TX_RX_POWER_17_5W		0xAF
#define TX_RX_POWER_20W			0xC8

#define MFC_NUM_FOD_REG					20

/* BIT DEFINE of Command Register, COM_L(0x4E) */
#define MFC_CMD_TOGGLE_PHM_SHIFT			7
#define MFC_CMD_RESERVED6_SHIFT				6
#define MFC_CMD_CLEAR_INT_SHIFT				5
#define MFC_CMD_SEND_CHG_STS_SHIFT			4
#define MFC_CMD_SEND_EOP_SHIFT				3
#define MFC_CMD_MCU_RESET_SHIFT				2
#define MFC_CMD_TOGGLE_LDO_SHIFT			1
#define MFC_CMD_SEND_TRX_DATA_SHIFT			0
#define MFC_CMD_TOGGLE_PHM_MASK				(1 << MFC_CMD_TOGGLE_PHM_SHIFT)
#define MFC_CMD_RESERVED6_MASK				(1 << MFC_CMD_RESERVED6_SHIFT)
#define MFC_CMD_CLEAR_INT_MASK				(1 << MFC_CMD_CLEAR_INT_SHIFT)
#define MFC_CMD_SEND_CHG_STS_MASK			(1 << MFC_CMD_SEND_CHG_STS_SHIFT) /* MFC MCU sends ChgStatus packet to TX */
#define MFC_CMD_SEND_EOP_MASK				(1 << MFC_CMD_SEND_EOP_SHIFT)
#define MFC_CMD_MCU_RESET_MASK				(1 << MFC_CMD_MCU_RESET_SHIFT)
#define MFC_CMD_TOGGLE_LDO_MASK				(1 << MFC_CMD_TOGGLE_LDO_SHIFT)
#define MFC_CMD_SEND_TRX_DATA_MASK			(1 << MFC_CMD_SEND_TRX_DATA_SHIFT)

/* Command Register, COM_H(0x4F) */
#define MFC_CMD2_SEND_ADT_SHIFT				0
#define MFC_CMD2_SEND_ADT_MASK				(1 << MFC_CMD2_SEND_ADT_SHIFT)
#define MFC_CMD2_WP_ON_SHIFT				0
#define MFC_CMD2_WP_ON_MASK					(1 << MFC_CMD2_WP_ON_SHIFT)

/* Chip Revision and Font Register, Chip_Rev (0x02) */
#define MFC_CHIP_REVISION_MASK				0xf0
#define MFC_CHIP_FONT_MASK					0x0f

#define MFC_CMD2_ADT_SENT_SHIFT				1
#define MFC_CMD2_ADT_SENT_MASK				(1 << MFC_CMD2_WP_ON_SHIFT)

/* BIT DEFINE of Status Registers, Status_L (0x20), Status_H (0x21) */
#define MFC_STAT_L_STAT_VOUT_SHIFT				7
#define MFC_STAT_L_STAT_VRECT_SHIFT				6
#define MFC_STAT_L_OP_MODE_SHIFT				5
#define MFC_STAT_L_OVER_VOL_SHIFT				4
#define MFC_STAT_L_OVER_CURR_SHIFT				3
#define MFC_STAT_L_OVER_TEMP_SHIFT				2
#define MFC_STAT_L_TXCONFLICT_SHIFT				1
#define MFC_STAT_L_ADT_ERROR_SHIFT				0
#define MFC_STAT_L_STAT_VOUT_MASK				(1 << MFC_STAT_L_STAT_VOUT_SHIFT)
#define MFC_STAT_L_STAT_VRECT_MASK				(1 << MFC_STAT_L_STAT_VRECT_SHIFT)
#define MFC_STAT_L_OP_MODE_MASK					(1 << MFC_STAT_L_OP_MODE_SHIFT)
#define MFC_STAT_L_OVER_VOL_MASK				(1 << MFC_STAT_L_OVER_VOL_SHIFT)
#define MFC_STAT_L_OVER_CURR_MASK				(1 << MFC_STAT_L_OVER_CURR_SHIFT)
#define MFC_STAT_L_OVER_TEMP_MASK				(1 << MFC_STAT_L_OVER_TEMP_SHIFT)
#define MFC_STAT_L_TXCONFLICT_MASK				(1 << MFC_STAT_L_TXCONFLICT_SHIFT)
#define MFC_STAT_L_ADT_ERROR_MASK				(1 << MFC_STAT_L_ADT_ERROR_SHIFT)

#define MFC_STAT_H_TRX_DATA_RECEIVED_SHIFT		7
#define MFC_STAT_H_TX_OCP_SHIFT					6
#define MFC_STAT_H_TX_MODE_RX_NOT_DET_SHIFT		5
#define MFC_STAT_H_TX_FOD_SHIFT					4
#define MFC_STAT_H_TX_CON_DISCON_SHIFT			3
#define MFC_STAT_H_AC_MISSING_DET_SHIFT			2
#define MFC_STAT_H_ADT_RECEIVED_SHIFT			1
#define MFC_STAT_H_ADT_SENT_SHIFT				0
#define MFC_STAT_H_TRX_DATA_RECEIVED_MASK		(1 << MFC_STAT_H_TRX_DATA_RECEIVED_SHIFT)
#define MFC_STAT_H_TX_OCP_MASK					(1 << MFC_STAT_H_TX_OCP_SHIFT)
#define MFC_STAT_H_TX_MODE_RX_NOT_DET_MASK		(1 << MFC_STAT_H_TX_MODE_RX_NOT_DET_SHIFT)
#define MFC_STAT_H_TX_FOD_MASK					(1 << MFC_STAT_H_TX_FOD_SHIFT)
#define MFC_STAT_H_TX_CON_DISCON_MASK			(1 << MFC_STAT_H_TX_CON_DISCON_SHIFT)
#define MFC_STAT_H_AC_MISSING_DET_MASK			(1 << MFC_STAT_H_AC_MISSING_DET_SHIFT)
#define MFC_STAT_H_ADT_RECEIVED_MASK			(1 << MFC_STAT_H_ADT_RECEIVED_SHIFT)
#define MFC_STAT_H_ADT_SENT_MASK				(1 << MFC_STAT_H_ADT_SENT_SHIFT)

/* BIT DEFINE of Interrupt_A Registers, INT_L (0x22), INT_H (0x23) */
#define MFC_INTA_L_STAT_VOUT_SHIFT				7
#define MFC_INTA_L_STAT_VRECT_SHIFT				6
#define MFC_INTA_L_OP_MODE_SHIFT				5
#define MFC_INTA_L_OVER_VOL_SHIFT				4
#define MFC_INTA_L_OVER_CURR_SHIFT				3
#define MFC_INTA_L_OVER_TEMP_SHIFT				2
#define MFC_INTA_L_TXCONFLICT_SHIFT				1
#define MFC_INTA_L_ADT_ERROR_SHIFT				0
#define MFC_INTA_L_STAT_VOUT_MASK				(1 << MFC_INTA_L_STAT_VOUT_SHIFT)
#define MFC_INTA_L_STAT_VRECT_MASK				(1 << MFC_INTA_L_STAT_VRECT_SHIFT)
#define MFC_INTA_L_OP_MODE_MASK					(1 << MFC_INTA_L_OP_MODE_SHIFT)
#define MFC_INTA_L_OVER_VOL_MASK				(1 << MFC_INTA_L_OVER_VOL_SHIFT)
#define MFC_INTA_L_OVER_CURR_MASK				(1 << MFC_STAT_L_OVER_CURR_SHIFT)
#define MFC_INTA_L_OVER_TEMP_MASK				(1 << MFC_STAT_L_OVER_TEMP_SHIFT)
#define MFC_INTA_L_TXCONFLICT_MASK				(1 << MFC_STAT_L_TXCONFLICT_SHIFT)
#define MFC_INTA_L_ADT_ERROR_MASK				(1 << MFC_INTA_L_ADT_ERROR_SHIFT)

#define MFC_INTA_H_TRX_DATA_RECEIVED_SHIFT		7
#define MFC_INTA_H_TX_OCP_SHIFT					6
#define MFC_INTA_H_TX_MODE_RX_NOT_DET			5
#define MFC_INTA_H_TX_FOD_SHIFT					4
#define MFC_INTA_H_TX_CON_DISCON_SHIFT			3
#define MFC_INTA_H_AC_MISSING_DET_SHIFT			2
#define MFC_INTA_H_ADT_RECEIVED_SHIFT			1
#define MFC_INTA_H_ADT_SENT_SHIFT				0
#define MFC_INTA_H_TRX_DATA_RECEIVED_MASK		(1 << MFC_INTA_H_TRX_DATA_RECEIVED_SHIFT)
#define MFC_INTA_H_TX_OCP_MASK					(1 << MFC_INTA_H_TX_OCP_SHIFT)
#define MFC_INTA_H_TX_MODE_RX_NOT_DET_MASK			(1 << MFC_INTA_H_TX_MODE_RX_NOT_DET)
#define MFC_INTA_H_TX_FOD_MASK					(1 << MFC_INTA_H_TX_FOD_SHIFT)
#define MFC_INTA_H_TX_CON_DISCON_MASK			(1 << MFC_INTA_H_TX_CON_DISCON_SHIFT)
#define MFC_INTA_H_AC_MISSING_DET_MASK			(1 << MFC_INTA_H_AC_MISSING_DET_SHIFT)
#define MFC_INTA_H_ADT_RECEIVED_MASK			(1 << MFC_INTA_H_ADT_RECEIVED_SHIFT)
#define MFC_INTA_H_ADT_SENT_MASK				(1 << MFC_INTA_H_ADT_SENT_SHIFT)

/* System Operating Mode Register, Sys_op_mode(0x2B) */
/* RX MODE[7:5] */
#define MFC_RX_MODE_AC_MISSING					0x0
#define MFC_RX_MODE_WPC_BASIC					0x1
#define MFC_RX_MODE_WPC_ADV						0x2
#define MFC_RX_MODE_PMA_SR1						0x3
#define MFC_RX_MODE_PMA_SR1E					0x4
#define MFC_RX_MODE_RESERVED1					0x5
#define MFC_RX_MODE_RESERVED2					0x6
#define MFC_RX_MODE_UNKNOWN						0x7
#if defined(CONFIG_WIRELESS_CHARGER_MFC_S2MIW04)
/* TX MODE[3:0] */
#define MFC_TX_MODE_RX_MODE				0x0
#define MFC_TX_MODE_MST_MODE1			0x1
#define MFC_TX_MODE_MST_MODE2			0x2
#define MFC_TX_MODE_TX_MODE				0x3
#define MFC_TX_MODE_MST_PCR_MODE1		0x7
#define MFC_TX_MODE_MST_PCR_MODE2		0xF
#endif
/* TX MODE[3:0] */
#define MFC_TX_MODE_BACK_PWR_MISSING			0x0
#define MFC_TX_MODE_MST_ON						0x4
#define MFC_TX_MODE_TX_MODE_ON					0x8
#define MFC_TX_MODE_TX_ERROR					0x9 /* TX FOD, TX conflict */
#define MFC_TX_MODE_TX_PWR_HOLD					0xA

/* End of Power Transfer Register, EPT (0x3B) (RX only) */
#define MFC_WPC_EPT_UNKNOWN						0
#define MFC_WPC_EPT_END_OF_CHG					1
#define MFC_WPC_EPT_INT_FAULT					2
#define MFC_WPC_EPT_OVER_TEMP					3
#define MFC_WPC_EPT_OVER_VOL					4
#define MFC_WPC_EPT_OVER_CURR					5
#define MFC_WPC_EPT_BATT_FAIL					6
#define MFC_WPC_EPT_RECONFIG					7
#define MFC_WPC_EPT_NO_RESPONSE					8

/* Proprietary Packet Header Register, PPP_Header VALUE(0x50) */
#define MFC_HEADER_END_SIG_STRENGTH			0x01 /* Message Size 1 */
#define MFC_HEADER_END_POWER_TRANSFER		0x02 /* Message Size 1 */
#define MFC_HEADER_END_CTR_ERROR			0x03 /* Message Size 1 */
#define MFC_HEADER_END_RECEIVED_POWER		0x04 /* Message Size 1 */
#define MFC_HEADER_END_CHARGE_STATUS		0x05 /* Message Size 1 */
#define MFC_HEADER_POWER_CTR_HOLD_OFF		0x06 /* Message Size 1 */
#define MFC_HEADER_PACKET_COUNTING			0x19 /* Message Size 1 */
#define MFC_HEADER_AFC_CONF					0x28 /* Message Size 2 */
#define MFC_HEADER_CONFIGURATION			0x51 /* Message Size 5 */
#define MFC_HEADER_IDENTIFICATION			0x71 /* Message Size 7 */
#define MFC_HEADER_EXTENDED_IDENT			0x81 /* Message Size 8 */

/* TX Data Command Register, TX Data_COM VALUE(0x50) */
#define MFC_TX_DATA_COM_TX_ID				0x01

/* END POWER TRANSFER CODES IN WPC */
#define MFC_EPT_CODE_UNKOWN				0x00
#define MFC_EPT_CODE_CHARGE_COMPLETE		0x01
#define MFC_EPT_CODE_INTERNAL_FAULT		0x02
#define MFC_EPT_CODE_OVER_TEMPERATURE		0x03
#define MFC_EPT_CODE_OVER_VOLTAGE			0x04
#define MFC_EPT_CODE_OVER_CURRENT			0x05
#define MFC_EPT_CODE_BATTERY_FAILURE		0x06
#define MFC_EPT_CODE_RECONFIGURE			0x07
#define MFC_EPT_CODE_NO_RESPONSE			0x08

#define MFC_POWER_MODE_MASK				(0x1 << 0)
#define MFC_SEND_USER_PKT_DONE_MASK		(0x1 << 7)
#define MFC_SEND_USER_PKT_ERR_MASK		(0x3 << 5)
#define MFC_SEND_ALIGN_MASK				(0x1 << 3)
#define MFC_SEND_EPT_CC_MASK			(0x1 << 0)
#define MFC_SEND_EOC_MASK				(0x1 << 0)

#define MFC_PTK_ERR_NO_ERR				0x00
#define MFC_PTK_ERR_ERR					0x01
#define MFC_PTK_ERR_ILLEGAL_HD			0x02
#define MFC_PTK_ERR_NO_DEF				0x03

#define MFC_FW_RESULT_DOWNLOADING		2
#define MFC_FW_RESULT_PASS				1
#define MFC_FW_RESULT_FAIL				0

#define MFC_FW_MSG		"@MFC_FW "

/* F/W Update & Verification ERROR CODES */
enum {
	MFC_FWUP_ERR_COMMON_FAIL = 0,
	MFC_FWUP_ERR_SUCCEEDED,
	MFC_FWUP_ERR_RUNNING,

	MFC_FWUP_ERR_REQUEST_FW_BIN,

	/* F/W update error */
	MFC_FWUP_ERR_WIRTE_KEY_ERR,
	MFC_FWUP_ERR_CLK_TIMING_ERR1,  /* 5 */
	MFC_FWUP_ERR_CLK_TIMING_ERR2,
	MFC_FWUP_ERR_CLK_TIMING_ERR3,
	MFC_FWUP_ERR_CLK_TIMING_ERR4,
	MFC_FWUP_ERR_INFO_PAGE_EMPTY,
	MFC_FWUP_ERR_HALT_M0_ERR, /* 10 */
	MFC_FWUP_ERR_FAIL,
	MFC_FWUP_ERR_ADDR_READ_FAIL,
	MFC_FWUP_ERR_DATA_NOT_MATCH,
	MFC_FWUP_ERR_OTP_LOADER_IN_RAM_ERR,
	MFC_FWUP_ERR_CLR_MTP_STATUS_BYTE, /* 15 */
	MFC_FWUP_ERR_MAP_RAM_TO_OTP_ERR,
	MFC_FWUP_ERR_WRITING_TO_OTP_BUFFER,
	MFC_FWUP_ERR_OTF_BUFFER_VALIDATION,
	MFC_FWUP_ERR_READING_OTP_BUFFER_STATUS,
	MFC_FWUP_ERR_TIMEOUT_ON_BUFFER_TO_OTP, /* 20 */
	MFC_FWUP_ERR_MTP_WRITE_ERR,
	MFC_FWUP_ERR_PKT_CHECKSUM_ERR,
	MFC_FWUP_ERR_UNKNOWN_ERR,
	MFC_FWUP_ERR_BUFFER_WRITE_IN_SECTOR,
	MFC_FWUP_ERR_WRITING_FW_VERION, /* 25 */

	/* F/W verification error */
	MFC_VERIFY_ERR_WIRTE_KEY_ERR,
	MFC_VERIFY_ERR_HALT_M0_ERR,
	MFC_VERIFY_ERR_KZALLOC_ERR,
	MFC_VERIFY_ERR_FAIL,
	MFC_VERIFY_ERR_ADDR_READ_FAIL, /* 30 */
	MFC_VERIFY_ERR_DATA_NOT_MATCH,
	MFC_VERIFY_ERR_MTP_VERIFIER_IN_RAM_ERR,
	MFC_VERIFY_ERR_CLR_MTP_STATUS_BYTE,
	MFC_VERIFY_ERR_MAP_RAM_TO_OTP_ERR,
	MFC_VERIFY_ERR_UNLOCK_SYS_REG_ERR, /* 35 */
	MFC_VERIFY_ERR_LDO_CLK_2MHZ_ERR,
	MFC_VERIFY_ERR_LDO_OUTPUT_5_5V_ERR,
	MFC_VERIFY_ERR_ENABLE_LDO_ERR,
	MFC_VERIFY_ERR_WRITING_TO_MTP_VERIFY_BUFFER,
	MFC_VERIFY_ERR_START_MTP_VERIFY_ERR, /* 40 */
	MFC_VERIFY_ERR_READING_MTP_VERIFY_STATUS,
	MFC_VERIFY_ERR_CRC_BUSY,
	MFC_VERIFY_ERR_READING_MTP_VERIFY_PASS_FAIL,
	MFC_VERIFY_ERR_CRC_ERROR,
	MFC_VERIFY_ERR_UNKOWN_ERR, /* 45 */
	MFC_VERIFY_ERR_BUFFER_WRITE_IN_SECTOR,

	MFC_REPAIR_ERR_HALT_M0_ERR,
	MFC_REPAIR_ERR_MTP_REPAIR_IN_RAM,
	MFC_REPAIR_ERR_CLR_MTP_STATUS_BYTE,
	MFC_REPAIR_ERR_START_MTP_REPAIR_ERR, /* 50 */
	MFC_REPAIR_ERR_READING_MTP_REPAIR_STATUS,
	MFC_REPAIR_ERR_READING_MTP_REPAIR_PASS_FAIL,
	MFC_REPAIR_ERR_BUFFER_WRITE_IN_SECTOR,
};

enum {
	MFC_VOUT_5V = 0,
	MFC_VOUT_5_5V,	// 1
	MFC_VOUT_6V, // 2
	MFC_VOUT_7V, // 3
	MFC_VOUT_8V, // 4
	MFC_VOUT_9V, // 5
	MFC_VOUT_10V, // 6
	MFC_VOUT_11V, // 7
	MFC_VOUT_12V, // 8
	MFC_VOUT_12_5V, // 9
};

/* PAD Vout */
enum {
	PAD_VOUT_5V = 0,
	PAD_VOUT_9V,
	PAD_VOUT_10V,
	PAD_VOUT_12V,
	PAD_VOUT_18V,
	PAD_VOUT_19V,
	PAD_VOUT_20V,
	PAD_VOUT_24V,
};

enum {
    MFC_ADC_VOUT = 0,
    MFC_ADC_VRECT,
    MFC_ADC_RX_IOUT,
    MFC_ADC_DIE_TEMP,
    MFC_ADC_OP_FRQ,
    MFC_ADC_TX_MAX_OP_FRQ,
    MFC_ADC_TX_MIN_OP_FRQ,
    MFC_ADC_PING_FRQ,
    MFC_ADC_TX_IOUT,
    MFC_ADC_TX_VOUT,
};

enum {
	MFC_END_SIG_STRENGTH = 0,
	MFC_END_POWER_TRANSFER,			/* 1 */
	MFC_END_CTR_ERROR,				/* 2 */
	MFC_END_RECEIVED_POWER,			/* 3 */
	MFC_END_CHARGE_STATUS,			/* 4 */
	MFC_POWER_CTR_HOLD_OFF,			/* 5 */
	MFC_AFC_CONF_5V,				/* 6 */
	MFC_AFC_CONF_10V,				/* 7 */
	MFC_AFC_CONF_5V_TX,				/* 8 */
	MFC_AFC_CONF_10V_TX,			/* 9 */
	MFC_AFC_CONF_12V_TX,			/* 10 */
	MFC_AFC_CONF_12_5V_TX,			/* 11 */
	MFC_AFC_CONF_20V_TX,			/* 12 */
	MFC_CONFIGURATION,				/* 13 */
	MFC_IDENTIFICATION,				/* 14 */
	MFC_EXTENDED_IDENT,				/* 15 */
	MFC_LED_CONTROL_ON,				/* 16 */
	MFC_LED_CONTROL_OFF,			/* 17 */
	MFC_FAN_CONTROL_ON,				/* 18 */
	MFC_FAN_CONTROL_OFF,			/* 19 */
	MFC_REQUEST_AFC_TX,				/* 20 */
	MFC_REQUEST_TX_ID,				/* 21 */
	MFC_DISABLE_TX,					/* 22 */
	MFC_PHM_ON, 				/* 23 */
	MFC_LED_CONTROL_DIMMING,			/* 24 */
};

enum {
	MFC_ADDR = 0,
	MFC_SIZE,
	MFC_DATA,
	MFC_PACKET,	
};

ssize_t mfc_s2miw04_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t mfc_s2miw04_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define MFC_S2MIW04_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0660},	\
	.show = mfc_s2miw04_show_attrs,			\
	.store = mfc_s2miw04_store_attrs,			\
}

enum mfc_irq {
	MFC_IRQ_STAT_VOUT = 0,
	MFC_IRQ_STAT_VRECT,
	MFC_IRQ_MODE_CHANGE,
	MFC_IRQ_TX_DATA_RECEIVED,
	MFC_IRQ_OVER_VOLT,
	MFC_IRQ_OVER_CURR,
	MFC_IRQ_OVER_TEMP,
	MFC_IRQ_TX_OVER_CURR,
	MFC_IRQ_TX_OVER_TEMP,
	MFC_IRQ_TX_FOD,
	MFC_IRQ_TX_CONNECT,
	MFC_IRQ_NR,
};

enum mfc_firmware_mode {
	MFC_RX_FIRMWARE = 0,
	MFC_TX_FIRMWARE,
};

enum mfc_ic_revision {
	MFC_IC_REVISION = 0,
	MFC_IC_FONT,
};

enum mfc_chip_id {
	MFC_CHIP_IDT = 1,
	MFC_CHIP_LSI,
};

enum mfc_headroom {
	MFC_HEADROOM_0 = 0,
	MFC_HEADROOM_1, /* 0.277V */
	MFC_HEADROOM_2, /* 0.497V */
	MFC_HEADROOM_3, /* 0.650V */
	MFC_HEADROOM_4, /* 0.030V */
	MFC_HEADROOM_5, /* 0.082V */
};

#define DEFAULT_PAD_ID		0
enum mfc_fod_state {
	FOD_STATE_CC = 0,
	FOD_STATE_CV,
	FOD_STATE_FULL,

	FOD_STATE_MAX
};

enum mfc_fod_flag {
	FOD_FLAG_NONE = 0,
	FOD_FLAG_ADD,
	FOD_FLAG_USE_CC,
	FOD_FLAG_USE_DEFAULT,
};

typedef struct _mfc_fod_data {
	int pad_id;
	int flag;
	u32* data[FOD_STATE_MAX];
} mfc_fod_data;

#if defined(CONFIG_CHECK_UNAUTH_PAD)
enum mfc_ping_freq {
	FREQ_TXID,
	FREQ_LOW,
	FREQ_HIGH,
	FREQ_MAX,
	FREQ_TXID_MAX = 23,
};

static const u8 Ping_freq[FREQ_TXID_MAX][FREQ_MAX] = {
	/*txid	freq_low	freq_high*/
	{0x0,	0xFF,	0xFF},
	{0x10,	0x8F,	0x95},
	{0x14,	0x8F,	0x95},
	{0x15,	0x8F,	0x95},
	{0x16,	0x8F,	0x95},
	{0x20,	0x8F,	0x95},
	{0x21,	0x8F,	0x95},
	{0x22,	0x8F,	0x95},
	{0x24,	0x8F,	0x95},
	{0x30,	0x8F,	0x95},

	{0x31,	0x8F,	0x95},
	{0x32,	0x8F,	0x95},
	{0x33,	0x8F,	0x95},
	{0x34,	0x7D,	0x83},
	{0x35,	0x7D,	0x83},
	{0x42,	0x8F,	0x95},
	{0x42,	0x7D,	0x83},
	{0xA0,	0x8F,	0x95},
	{0xA1,	0x7D,	0x83},
	{0xA2,	0x7D,	0x83},

	{0xF3,	0x87,	0x8D},
	{0xF3,	0x8F,	0x95},
	{0xFF,	0xFF,	0xFF},
};
#endif

struct mfc_charger_platform_data {
	int pad_mode;
	int wpc_det;
	int irq_wpc_det;
	int wpc_int;
	int mst_pwr_en;
	int wpc_en;
	int wpc_pdrc;
	int irq_wpc_pdrc;
	int ping_nen;
	int irq_wpc_int;
	int cs100_status;
	int vout_status;
	int wireless_cc_cv;
	int siop_level;
	u8 capacity;
	int cable_type;
	bool default_voreg;
	int is_charging;
	u32 *wireless20_vout_list;
	u32 *wireless20_vrect_list;
	u32 *wireless20_max_power_list;
	u8 len_wc20_list;
	bool ic_on_mode;
	int hw_rev_changed; /* this is only for noble/zero2 */
	int otp_firmware_result;
	int tx_firmware_result;
	int wc_ic_grade;
	int wc_ic_rev;
	int otp_firmware_ver;
	int tx_firmware_ver;
	int vout;
	int vrect;
	u8 trx_data_cmd;
	u8 trx_data_val;
	char *wireless_charger_name;
	char *wired_charger_name;
	char *fuelgauge_name;
	int opfq_cnt;
	int mst_switch_delay;
	int wc_cover_rpp;
	int wc_hv_rpp;
	u32 oc_fod1;
	u32 phone_fod_threshold;
	u32 gear_ping_freq;
	u32 gear_min_op_freq;
	u32 gear_min_op_freq_delay;
	bool wpc_vout_ctrl_lcd_on;
	int no_hv;
	bool keep_tx_vout;

	mfc_fod_data* fod_list;
	int fod_data_count;
};

#define mfc_charger_platform_data_t \
	struct mfc_charger_platform_data

#define MST_MODE_0			0
#define MST_MODE_2			1

struct mfc_charger_data {
	struct i2c_client				*client;
	struct device					*dev;
	mfc_charger_platform_data_t 	*pdata;
	struct mutex io_lock;
	struct mutex wpc_en_lock;
	const struct firmware *firm_data_bin;

	int wc_w_state;

	struct power_supply *psy_chg;
	struct wakeup_source *wpc_wake_lock;
	struct wakeup_source *wpc_tx_wake_lock;
	struct wakeup_source *wpc_rx_wake_lock;
	struct wakeup_source *wpc_update_lock;
	struct wakeup_source *wpc_opfq_lock;
	struct wakeup_source *wpc_tx_opfq_lock;
	struct wakeup_source *wpc_tx_duty_min_lock;
	struct wakeup_source *wpc_tx_min_opfq_lock;
	struct wakeup_source *wpc_afc_vout_lock;
	struct wakeup_source *wpc_vout_mode_lock;
	struct wakeup_source *wpc_rx_det_lock;
	struct wakeup_source *wpc_tx_phm_lock;
	struct wakeup_source *wpc_vrect_check_lock;
	struct wakeup_source *wpc_tx_id_lock;
	struct wakeup_source *wpc_cs100_lock;
	struct wakeup_source *wpc_pdrc_lock;
	struct workqueue_struct *wqueue;
	struct work_struct	wcin_work;
	struct delayed_work	wpc_det_work;
	struct delayed_work	wpc_pdrc_work;
	struct delayed_work	wpc_opfq_work;
	struct delayed_work	wpc_isr_work;
	struct delayed_work	wpc_tx_isr_work;
	struct delayed_work	wpc_tx_id_work;
	struct delayed_work mst_off_work;
	struct delayed_work	wpc_int_req_work;
	struct delayed_work	wpc_fw_update_work;
	struct delayed_work	wpc_afc_vout_work;
	struct delayed_work	wpc_fw_booting_work;
	struct delayed_work	wpc_vout_mode_work;
	struct delayed_work wpc_i2c_error_work;
	struct delayed_work	wpc_rx_type_det_work;
	struct delayed_work	wpc_rx_connection_work;
	struct delayed_work wpc_tx_min_op_freq_work;
	struct delayed_work wpc_tx_op_freq_work;
	struct delayed_work wpc_tx_duty_min_work;
	struct delayed_work wpc_tx_phm_work;
	struct delayed_work wpc_vrect_check_work;
	struct delayed_work wpc_rx_power_work;
	struct delayed_work wpc_cs100_work;
#if defined(CONFIG_SEC_FACTORY)
	struct delayed_work evt2_err_detect_work;
#endif

	struct alarm phm_alarm;

	u16 addr;
	int size;
	int is_afc;
	int pad_vout;
	int is_mst_on; /* mst */
	int chip_id;
	u8 chip_id_now;
	int fw_cmd;
	int vout_mode;
	u32 vout_by_txid;
	u32 vrect_by_txid;
	u32 max_power_by_txid;
	int is_full_status;
	int mst_off_lock;
	bool is_otg_on;
	int led_cover;
	bool is_probed;
	bool is_afc_tx;
	bool pad_ctrl_by_lcd;
	bool tx_id_done;
	bool is_suspend;
	int tx_id;
	int tx_id_cnt;
	bool initial_vrect;

	int flicker_delay;
	int flicker_vout_threshold;

	/* wireless tx */
	int tx_status;
	bool initial_wc_check;
	bool wc_tx_enable;
	int wc_rx_type;
	bool wc_rx_connected;
	bool wc_rx_fod;
	bool wc_ldo_status;
	int non_sleep_mode_cnt;
	u8 adt_transfer_status;
	u8 current_rx_power;
	u8 device_event;
	int i2c_error_count;
	unsigned long gear_start_time;
	int input_current;
	int duty_min;
	int wpc_en_flag;

	bool req_tx_id;
	bool is_abnormal_pad;
#if defined(CONFIG_CHECK_UNAUTH_PAD)
	u8 ping_freq;
	bool req_afc_tx;
#endif

	struct mutex fw_lock;
	unsigned long fw_size;
	u8 *fw_img;
};
#endif /* __WIRELESS_CHARGER_MFC_S2MIW04_H */
