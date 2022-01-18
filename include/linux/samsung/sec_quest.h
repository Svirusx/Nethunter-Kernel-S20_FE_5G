/*
 * inluce/linux/sec_quest.h
 *
 * COPYRIGHT(C) 2006-2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SEC_QUEST_H
#define SEC_QUEST_H


#include <linux/kern_levels.h>
#include <linux/types.h>


#define MAX_LEN_STR			1024
#define QUEST_BUFF_SIZE		10
#define QUEST_CMD_LIST		3
#define QUEST_CMD_SIZE		64
#define QUEST_MAIN_CMD_LIST	2
#define STEP_MAIN_HLOS_TIMEOUT 	5400
#define BUFF_SZ 			256
#define MAX_DDR_ERR_ADDR_CNT 64
#define CPR_BPS_SZ_BYTE		256
#define QUEST_CPR_MODE_CNT 10
#define MAIN_QUEST_QUEFI_REPEATS 2


/*
We will determine if we insert ITEM_QUESTHLOS into STEP_SMDDL later.
It is better to keep QUEST DONE popup if we complete to run all items of STEP_SMDDL 
 although we do not run ITEM_QUESTHLOS in STEP_SMDDL.
So, let's call proper shell script as concept of STEP_SDMDL
1) no ITEM_QUESTHLOS in STEP_SMDDL => call quest_dummy.sh which send nad_end after 5 seconds
2) ITEM_QUESTHLOS in STEP_SMDDL => call quest.sh which execute ITEM_QUESTHLOS
3) ITEM_QUESTHLOSNATURESCENE in STEP_SMDDL => call quest_naturescene.sh which execute ITEM_QUESTHLOSNATURESCENE
*/
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
#define QUESTHLOS_HLOS_ITEM_SMD			ITEM_QUESTHLOSDUMMY
#define QUESTHLOS_PROG_SMD				"/system/bin/quest/quest_dummy.sh"
#elif defined(CONFIG_SEC_QUEST_HLOS_NATURESCENE_SMD)
#define QUESTHLOS_HLOS_ITEM_SMD			ITEM_QUESTHLOSNATURESCENE
#define QUESTHLOS_PROG_SMD				"/system/bin/quest/quest_naturescene.sh"
#else
#define QUESTHLOS_HLOS_ITEM_SMD			ITEM_QUESTHLOS
#define QUESTHLOS_PROG_SMD				"/system/bin/quest/quest.sh"
#endif

// quest_dummy for MAIN AND CAL
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_MAIN_CAL)
#define QUESTHLOS_HLOS_ITEM_MAIN_CAL	ITEM_QUESTHLOSDUMMY
#define QUESTHLOS_PROG_MAIN_CAL			"/system/bin/quest/quest_dummy.sh"
#elif defined(CONFIG_SEC_QUEST_USE_SS_HLOS_SH_MAIN_CAL)
#define QUESTHLOS_HLOS_ITEM_MAIN_CAL	ITEM_QUESTHLOS
#define QUESTHLOS_PROG_MAIN_CAL			"/system/bin/quest/quest_ss.sh"
#else
#define QUESTHLOS_HLOS_ITEM_MAIN_CAL	ITEM_QUESTHLOS
#define QUESTHLOS_PROG_MAIN_CAL			"/system/bin/quest/quest.sh"
#endif

#define ERASE_QUEST_PRG			"/system/bin/quest/remove_files.sh"
#define ARRANGE_QUEST_LOGS_PRG	"/system/bin/quest/arrange_quest_logs.sh"
#define MOVE_QUESTRESULT_PRG	"/system/bin/quest/move_questresult.sh"
#define QUEST_DEBUGGING_PRG		"/system/bin/quest/quest_debugging.sh"

#define SMD_QUEST_RESULT		"/data/log/quest/SMD/test_result.csv"
#define CAL_QUEST_RESULT		"/data/log/quest/CAL/test_result.csv"
#define MAIN_QUEST_RESULT		"/data/log/quest/MAIN/test_result.csv"

#define SMD_QUEST_LOGPATH		"/data/log/quest/SMD"
#define CAL_QUEST_LOGPATH		"/data/log/quest/CAL"
#define MAIN_QUEST_LOGPATH		"/data/log/quest/MAIN"

#define MOUNTPOINT_LOGFS		"/data/log/quest"
#define UEFI_QUESTRESULT_FILE	"/data/log/quest/questresult.txt"
#define UEFI_ENHANCEMENT_RESULT_FAILE	"/data/log/quest/EnhanceLog.txt"


#define TEST_CRYPTO(x) (!strcmp((x), "CRYPTOTEST"))
#define TEST_ICACHE(x) (!strcmp((x), "ICACHETEST"))
#define TEST_CCOHERENCY(x) (!strcmp((x), "CCOHERENCYTEST"))
#define TEST_SUSPEND(x) (!strcmp((x), "SUSPENDTEST"))
#define TEST_VDDMIN(x) (!strcmp((x), "VDDMINTEST"))
#define TEST_QMESADDR(x) (!strcmp((x), "QMESADDRTEST"))
#define TEST_QMESACACHE(x) (!strcmp((x), "QMESACACHETEST"))
#define TEST_UFS(x) (!strcmp((x), "UFSTEST"))
#define TEST_NATURESCENE(x) (!strcmp((x), "NATURESCENE"))
#define TEST_AOSSTHERMALDIFF(x) (!strcmp((x), "AOSSTHERMALDIFF"))
#define TEST_SENSOR(x) (!strcmp((x), "SENSORTEST"))
#define TEST_SENSORPROBE(x) (!strcmp((x), "SENSORPROBETEST"))
#define TEST_DDR_SCAN(x) (!strcmp((x), "DDRSCANTEST"))
#define TEST_A75G(x) (!strcmp((x), "A75GTEST"))
#define TEST_Q65G(x) (!strcmp((x), "Q65GTEST"))
#define TEST_THERMAL(x) (!strcmp((x), "THERMALTEST"))
#define TEST_QDAF(x) (!strcmp((x), "QDAFTEST"))
#define TEST_DUMMY(x)(!strcmp((x), "DUMMY")) 
#define TEST_PASS(x) (!strcmp((x), "PASS"))
#define TEST_FAIL(x) (!strcmp((x), "FAIL"))
#define TEST_NA(x) (!strcmp((x), "NA"))



#define QUEST_PRINT(format, ...) printk(KERN_ERR "[QUEST] " format, ##__VA_ARGS__)


/* bit operation */
#define QUEST_GET_BIT(data, bit)	(data) & (0x1 << (bit))
#define QUEST_SET_BIT(data, bit)	(data) | (0x1 << (bit))
#define QUEST_GET_ITEM_SUBITEM_RESULT(data, item) \
	(((data) >> ((item) * 2)) & 0x3)
#define QUEST_SET_ITEM_SUBITEM_RESULT(data, item, val) \
	((data) = (((data)&(~((uint64_t)0x3<<((item))*2))) | ((uint64_t)val << ((item) *2))))
#define QUEST_CLEAR_ITEM_SUBITEM_RESULT(data, item) \
	((data) = (((data)&(~((uint64_t)0x3<<((item))*2))) | ((uint64_t)0x0 << ((item) *2))))	
#define QUEST_SET_BIT_WITH_SHIFT(data, layer, code) \
	((data) |= ((data) | ((uint64_t)0x1 << (code)<<((layer)*QUEST_MAX_ERR_CODES_AT_ONE_LAYER))))


/* smddl variable */
#define QUEST_UPDATE_SMDDL_INFO(param_member) \
	if( param_quest_data.num_smd_try == 1 ) \
		param_member##_first = param_member;

#define QUEST_UPDATE_SMDDL_INFO_WITH_VAL(param_first_member, val) \
	do { \
		if( param_quest_data.num_smd_try == 1 ) {\
			param_first_member = val; \
		}\
	} while(0)

#define QUEST_UPDATE_SMDDL_INFO_WITH_VAL_AND_RET(param_first_member, val, ret) \
	do { \
		if( param_quest_data.num_smd_try == 1 ) {\
			param_first_member = val; \
			ret = 1; \
		}else \
			ret = 0; \
	} while(0)

#define QUEST_UPDATE_SMDDL_INFO_WITH_STRING(param_first_member, str, len) \
	do { \
		if( param_quest_data.num_smd_try == 1 ) { \
			strncpy(param_first_member, str, len); \
		} \
	} while(0)
	



/* enum */
enum quest_enum_item {
	ITEM_NONE = 0,
	ITEM_QUESTHLOS,
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)	
	ITEM_QUESTHLOSDUMMY,
#elif defined(CONFIG_SEC_QUEST_HLOS_NATURESCENE_SMD)	
	ITEM_QUESTHLOSNATURESCENE,
#endif
	ITEM_QUESTFUSION,
	ITEM_QUESTQUEFI,
	ITEM_QUESTSUEFILIGHT,
	ITEM_QUESTSUEFIHEAVY,	
	ITEM_DDRSCANLOADER,
	ITEM_DDRSCANRAMDUMPENCACHE,
	ITEM_DDRSCANRAMDUMPDISCACHE,
	ITEM_DDRSCANUEFI,
	ITEM_SMDDLQDAF,
	ITEM_DDRTRAINING,
	//
	ITEM_ITEMSCOUNT,
};

enum quest_enum_smd_subitem {
	SUBITEM_NONE = 0,
	SUBITEM_DDRSCANLOADER = 1,
#if defined(CONFIG_SEC_QUEST_EDL)
	SUBITEM_QUESTSUEFI_FIRST = 2,
	SUBITEM_QUESTSUEFI_GROUP1 = SUBITEM_QUESTSUEFI_FIRST,
	SUBITEM_QUESTSUEFI_GROUP2,
	SUBITEM_QUESTSUEFI_GROUP3,
	SUBITEM_QUESTSUEFI_GROUP4,
	SUBITEM_QUESTSUEFI_GROUP5,
	SUBITEM_QUESTSUEFI_GROUP6,
	SUBITEM_QUESTSUEFI_GROUP7,
	SUBITEM_QUESTSUEFI_GROUP8,
	SUBITEM_QUESTSUEFI_GROUP9,
	SUBITEM_QUESTSUEFI_LAST = SUBITEM_QUESTSUEFI_GROUP9,
	SUBITEM_QUESTQUEFI_FIRST = 11,
	SUBITEM_QUESTQUEFI_GROUP1 = SUBITEM_QUESTQUEFI_FIRST,
	SUBITEM_QUESTQUEFI_GROUP2,
	SUBITEM_QUESTQUEFI_GROUP3,
	SUBITEM_QUESTQUEFI_GROUP4,
	SUBITEM_QUESTQUEFI_GROUP5,
	SUBITEM_QUESTQUEFI_GROUP6,
	SUBITEM_QUESTQUEFI_GROUP7,
	SUBITEM_QUESTQUEFI_GROUP8,
	SUBITEM_QUESTQUEFI_GROUP9,
	SUBITEM_QUESTQUEFI_LAST = SUBITEM_QUESTQUEFI_GROUP9,
#else
	SUBITEM_QUESTQUEFI,
	SUBITEM_QUESTSUEFILIGHTCRYPTO,
	SUBITEM_QUESTSUEFILIGHTCOMPLEX,
#endif
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
	SUBITEM_QUESTHLOSDUMMY,
#elif defined(CONFIG_SEC_QUEST_HLOS_NATURESCENE_SMD)
	SUBITEM_QUESTHLOSNATURESCENE,
	SUBITEM_QUESTHLOSAOSSTHERMALDIFF,
#else
	SUBITEM_QUESTHLOSCRYPTO,
	SUBITEM_QUESTHLOSICACHE,
	SUBITEM_QUESTHLOSCCOHERENCY,
	SUBITEM_QUESTHLOSQMESADDR,
	SUBITEM_QUESTHLOSQMESACACHE,
	SUBITEM_QUESTHLOSSUSPEND,
	SUBITEM_QUESTHLOSVDDMIN,
	SUBITEM_QUESTHLOSTHERMAL,
	SUBITEM_QUESTHLOSUFS,	
	SUBITEM_QUESTFUSIONA75G,
	SUBITEM_QUESTFUSIONQ65G,
#endif
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
	SUBITEM_SMDDLQDAF,
#endif	
	//
	SUBITEM_ITEMSCOUNT,
};

enum quest_enum_step {
	STEP_NONE = 0,
	STEP_SMDDL,
	STEP_CAL1,
	STEP_CALX,
	STEP_MAIN,
	STEP_TESTMODE,
//	STEP_SKP,
	//
	STEP_STEPSCOUNT,
};

enum quest_enum_item_result {
	ITEM_RESULT_NONE = 0,		// not started
	ITEM_RESULT_INCOMPLETED,	// set this when starting
	ITEM_RESULT_FAIL,			// completed and failed
	ITEM_RESULT_PASS,			// completed and passed
};

enum quest_enum_err_layer {
	ERR_BOOT = 0,
	ERR_QUEFI,
	ERR_KERNEL,
	ERR_SHELL,
	//
	ERR_LAYERSCOUNT,
};

enum quest_enum_err_boot_code {
	ERR_BOOT_NONE = 0,
	ERR_BOOT_WRONG_REMAINED_COUNT,
	//
	ERR_BOOT_CODESCOUNT,
};

struct param_quest_cpr_t {
	uint32_t Modes;
	uint32_t Floor;
	uint32_t Ceiling;
	uint32_t Current;
};



/* param struct
 *
 *	smd_item_result
 *	cal_item_result
 *	main_item_result
 *		- for complex smd scenario
 *		- for CAL1,CAL20, CALX, MAIN scenario
 *		- refer to the quest_enum_item
 *			bit[1:0] = 0
 *			bit[3:2] = quest_enum_item_result of ITEM_QUESTHLOS
 *			bit[5:4] = quest_enum_item_result of ITEM_QUESTFUSION
 *			...
 *
 *	smd_subitem_result
 *		- for NG_2.0_FAIL_[P][F]... 
 *		- refer to the quest_enum_smd_subitem
 *			bit[1:0] = 0
 *			bit[3:2] = quest_enum_item_result of SUBITEM_DDRSCANLOADER
 *			bit[5:4] = quest_enum_item_result of SUBITEM_SUEFICRYPTO
 *			...
 *
 */
struct param_quest_t {
	
	uint64_t smd_item_result;
	uint64_t smd_subitem_result;
	uint64_t cal_item_result;	
	uint64_t main_item_result;

	uint32_t curr_step; 

	uint32_t hlos_remained_count;
	uint32_t quefi_remained_count;
	uint32_t suefi_remained_count;
	uint32_t ddrscan_remained_count;

	uint32_t thermal;
	uint32_t tested_clock;
	uint64_t err_codes;

	uint32_t main_quefi_init_thermal;
	uint32_t main_quefi_end_thermal;
	uint32_t main_suefi_init_thermal;
	uint32_t main_suefi_end_thermal;
	
	uint32_t smd_ddrscan_elapsed_time;
	uint32_t smd_quefi_elapsed_time;
	uint32_t smd_suefi_elapsed_time;
	
	char     smd_boot_reason[5];
	uint32_t smd_hlos_start_time;
	uint32_t smd_hlos_elapsed_time;
	uint32_t smd_hlos_init_thermal;
	uint32_t smd_hlos_max_thermal;
	uint32_t smd_ns_repeats;

	/* smddl information */
	uint64_t smd_subitem_result_first;

	uint32_t smd_quefi_init_thermal_first;
	uint32_t smd_quefi_end_thermal_first;
	uint32_t smd_suefi_init_thermal_first;
	uint32_t smd_suefi_end_thermal_first;	

	uint32_t smd_ddrscan_elapsed_time_first;
	uint32_t smd_quefi_elapsed_time_first;
	uint32_t smd_suefi_elapsed_time_first;

	char     smd_boot_reason_first[5];
	uint32_t smd_hlos_start_time_first;
	uint32_t smd_hlos_elapsed_time_first;
	uint32_t smd_hlos_init_thermal_first;
	uint32_t smd_hlos_max_thermal_first;
	uint32_t smd_ns_repeats_first;
	uint32_t smd_quefi_rework;
	uint32_t smd_suefi_rework;

	uint32_t smd_quefi_total_pause_time;
	uint32_t smd_quefi_total_pause_time_first;
	/* smddl information */

	uint32_t smd_max_aoss_thermal_diff;
	uint32_t smd_max_aoss_thermal_diff_first;

	uint64_t real_smd_register_value;
	uint64_t ap_serial;

	uint32_t num_smd_try;

	uint32_t smd_ft_self_cooling_time;
	uint32_t smd_ft_thermal_after_self_cooling;

	uint32_t smd_cper;

	uint32_t quest_fv_flashed;	

};

struct param_quest_ddr_result_t {
	uint32_t ddr_err_addr_total;
	uint64_t ddr_err_addr[MAX_DDR_ERR_ADDR_CNT];
};


#endif
