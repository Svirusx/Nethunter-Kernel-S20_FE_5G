/*
 * drivers/debug/sec_quest.c
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

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/sec_class.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/sec_debug.h>
#include <linux/sec_param.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/sec_quest.h>
#include <linux/sec_quest_param.h>
#include <linux/sec_quest_qdaf.h>

// TEMPORARY DEFINITION!!!
// Please disable it later!!!
//#if defined(CONFIG_ARCH_LAHAINA)
//#define CONFIG_SEC_QUEST_ALWAYS_RETURN_PASS_FOR_ACAT
//#endif

// Please disable it later!!!
#if defined(CONFIG_ARCH_LAHAINA)
#define CONFIG_SEC_QUEST_NOT_TRIGGER_SMDDL_QDAF
#define CONFIG_SEC_QUEST_NOT_TRIGGER_MAIN_QDAF
#endif

#ifdef CONFIG_SEC_QUEST_BPS_CLASSIFIER
#include <linux/sec_quest_bps_classifier.h>

extern struct bps_info bps_envs;
#endif

// param data
extern struct param_quest_t param_quest_data;
extern struct param_quest_ddr_result_t param_quest_ddr_result_data;
extern unsigned int param_api_gpio_test;
extern char param_api_gpio_test_result[256];

// sysfs
struct device *sec_nad;
#if defined(CONFIG_SEC_FACTORY)
struct device *sec_nad_balancer;
static struct kobj_uevent_env quest_uevent;
#endif

// etc
extern unsigned int lpcharge;
struct mutex sysfs_common_lock;

#if defined(CONFIG_SEC_FACTORY)
static int erased;
static int step_to_smd_quest_hlos;
#if defined(CONFIG_SEC_QUEST_AUTO_TRIGGER_KWORKER)
struct delayed_work trigger_quest_work;
#define WAIT_TIME_BEFORE_TRIGGER_MSECS 60000
#endif
static int call_main_qdaf_after_finighing_main_quest = 0;	// 0:NOT_RUN 1:SHOULD_RUN
static int boot_count;
static int testmode_enabled = 0;
static int testmode_quefi_enabled = 1;
static int testmode_suefi_enabled = 1;
static int testmode_ddrscan_enabled = 1;
#endif


/* Please sync with enum quest_enum_item in sec_qeust.h */
char *STR_ITEM[ITEM_ITEMSCOUNT] = {
	"",
	"HLOS",
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
	"HLOSDUMMY",
#elif defined(CONFIG_SEC_QUEST_HLOS_NATURESCENE_SMD)
	"HLOSNATURESCENE",
#endif
	"FUSION",
	"QUEFI",
	"SUEFI_LIGHT",
	"SUEFI_HEAVY",
	"DDR_SCAN_LOADER",
	"DDR_SCAN_RAMDUMP_ENCACHE",
	"DDR_SCAN_RAMDUMP_DISCACHE",
	"DDR_SCAN_UEFI",
	"SMDDL_QDAF",
	"DDRTRAINING",
};


/* Please sync with enum quest_enum_smd_subitem in sec_qeust.h */
char *STR_SUBITEM[SUBITEM_ITEMSCOUNT] = {
	"",
	"DDR_SCAN",
#if defined(CONFIG_SEC_QUEST_EDL)
	"SUEFI_GROUP1",
	"SUEFI_GROUP2",
	"SUEFI_GROUP3",
	"SUEFI_GROUP4",
	"SUEFI_GROUP5",
	"SUEFI_GROUP6",
	"SUEFI_GROUP7",
	"SUEFI_GROUP8",
	"SUEFI_GROUP9",
	"QUEFI_GROUP1",
	"QUEFI_GROUP2",
	"QUEFI_GROUP3",
	"QUEFI_GROUP4",
	"QUEFI_GROUP5",
	"QUEFI_GROUP6",
	"QUEFI_GROUP7",
	"QUEFI_GROUP8",
	"QUEFI_GROUP9",		
#else
	"QUEFI",
	"SUEFI_CRYPTO",
	"SUEFI_COMPLEX",
#endif	
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
	"HLOS_DUMMY",
#elif defined(CONFIG_SEC_QUEST_HLOS_NATURESCENE_SMD)
	"HLOS_NATURESCENE",
	"HLOS_AOSSTHERMALDIFF",
#else
	"HLOS_CRYPTO",
	"HLOS_ICACHE",
	"HLOS_CCOHERENCY",
	"HLOS_QMESADDR",
	"HLOS_QMESACACHE",
	"HLOS_SUSPEND",
	"HLOS_VDDMIN",
	"HLOS_THERMAL",
	"HLOS_UFS",
	"FUSION_A75G",
	"FUSION_Q65G",
#endif
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
	"SMDDL_QDAF"
#endif
};


#if defined(CONFIG_SEC_FACTORY)


#if defined(CONFIG_SEC_QUEST_BPS_CLASSIFIER)
static void sec_quest_bps_print_info(struct bps_info *bfo, char *info)
{
	QUEST_PRINT(
		"\n=====================================================\n"
		" BPS info : %s\n"
		"=====================================================\n"
		" magic[0] = 0x%x\n"
		" magic[1] = 0x%x\n"
		"=====================================================\n"
		" sp : %d\n"
		" wp : %d\n"
		" dp : %d\n"
		" kp : %d\n"
		" mp : %d\n"
		" tp : %d\n"
		" cp : %d\n"
		"=====================================================\n"
		" pc_lr_cnt = %d\n"
		" pc_lr_last_idx = %d\n"
		" tzerr_cnt = %d\n"
		" klg_cnt = %d\n"
		" dn_cnt = %d\n"
		" build_id = %s\n"
		"=====================================================\n",
		info, bfo->magic[0], bfo->magic[1], bfo->up_cnt.sp,
		bfo->up_cnt.wp, bfo->up_cnt.dp, bfo->up_cnt.kp,
		bfo->up_cnt.mp, bfo->up_cnt.tp, bfo->up_cnt.cp,
		bfo->pc_lr_cnt, bfo->pc_lr_last_idx,
		bfo->tzerr_cnt, bfo->klg_cnt, bfo->dn_cnt,
		bfo->build_id);
}

static void sec_quest_bps_param_read(void)
{
	/* return if bps data already loaded */
	if (sec_quest_bps_env_initialized == true)
		return;

	quest_load_param_quest_bps_data();

	/* print bps status */
	sec_quest_bps_print_info(&bps_envs, "kernel");

	/* set bps data successfully loaded */
	sec_quest_bps_env_initialized = true;
}
#endif

//////////////////////////////////////////////////
/////// panic notifier functions /////////////////
//////////////////////////////////////////////////
static int quest_debug_panic_handler(struct notifier_block *nb,
		unsigned long l, void *buf)
{
	QUEST_PRINT("%s : print param_quest_data\n", __func__);
	quest_print_param_quest_data();

	return NOTIFY_DONE;
}

static struct notifier_block quest_panic_block = {
	.notifier_call = quest_debug_panic_handler,

};

//////////////////////////////////////////////////


//////////////////////////////////////////////////
/////// helper functions /////////////////////////////
//////////////////////////////////////////////////
static int call_user_prg( char **argv, int wait )
{
	int ret_userapp;
	char *envp[5] = {
		"HOME=/",
		"PATH=/system/bin/quest:/system/bin:/system/xbin",
		"ANDROID_DATA=/data",
		"ANDROID_ROOT=/system",
		NULL };

	ret_userapp = call_usermodehelper(argv[0], argv, envp, wait);
	if (!ret_userapp) {
		QUEST_PRINT("%s is executed. ret_userapp = %d\n", argv[0], ret_userapp);
		return 0;
	} else {
		QUEST_PRINT("%s is NOT executed. ret_userapp = %d\n", argv[0], ret_userapp);
		return ret_userapp;
	}
}

static int do_quest()
{
	char *argv[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
	int ret;

	char log_path[50] = { '\0', };

	QUEST_PRINT("%s : curr_step=%d\n", __func__, param_quest_data.curr_step);

	switch (param_quest_data.curr_step) {

		case STEP_SMDDL:
			argv[0] = QUESTHLOS_PROG_SMD;
			snprintf(log_path, 50, "logPath:%s", SMD_QUEST_LOGPATH);
			argv[1] = log_path;
#if defined(CONFIG_SEC_DDR_SKP)
			argv[2] = "Reboot";
#endif
			break;
		case STEP_CAL1:
#if defined(CONFIG_SEC_QUEST_CAL_HLOS_SUPPORT_FUSION)
			argv[0] = QUESTHLOS_PROG_MAIN_CAL;
			snprintf(log_path, 50, "logPath:%s", CAL_QUEST_LOGPATH);
			argv[1] = log_path;
			argv[2] = "Reboot";
			QUEST_PRINT("reboot option enabled \n");
			argv[3] = "hlosTestDisabled:1";
			QUEST_PRINT("hlosTestDisabled option enabled \n");
			argv[4] = "fusionTestEnabled:1";
			QUEST_PRINT("fusionTestEnabled option enabled \n");
			argv[5] = "qdafTestEnabled:1";
			QUEST_PRINT("qdafTestEnabled option enabled \n");
			break;
#endif
		case STEP_CALX:
			argv[0] = QUESTHLOS_PROG_MAIN_CAL;
			snprintf(log_path, 50, "logPath:%s", CAL_QUEST_LOGPATH);
			argv[1] = log_path;
			argv[2] = "Reboot";
			QUEST_PRINT("reboot option enabled \n");
			break;
		case STEP_TESTMODE:
			argv[0] = QUESTHLOS_PROG_MAIN_CAL;
			snprintf(log_path, 50, "logPath:%s", CAL_QUEST_LOGPATH);
			argv[1] = log_path;
			argv[2] = "Reboot";
			QUEST_PRINT("reboot option enabled \n");
			argv[3] = "testmodeTestEnabled:1";
			QUEST_PRINT("testmodeTestEnabled option enabled \n");
			break;
		case STEP_MAIN:
			argv[0] = QUESTHLOS_PROG_MAIN_CAL;
			snprintf(log_path, 50, "logPath:%s", MAIN_QUEST_LOGPATH);
			argv[1] = log_path;
			argv[2] = "Reboot";
			QUEST_PRINT("reboot option enabled \n");
			argv[3] = "1800";
#if defined(CONFIG_SEC_QUEST_MAIN_HLOS_SUPPORT_FUSION)
			argv[4] = "fusionTestEnabled:1";
			QUEST_PRINT("fusionTestEnabled option enabled \n");
#endif
			break;
		default:
			QUEST_PRINT("invalid step\n");
			return -1;
	}

	ret = call_user_prg(argv, UMH_WAIT_EXEC);

	return ret;
}

static void move_questresult_to_sub_dir(int quest_step)
{
	char *argv[4] = { NULL, NULL, NULL, NULL };

	argv[0] = MOVE_QUESTRESULT_PRG;
	switch (quest_step)
	{
		case STEP_SMDDL:
			argv[1] = SMD_QUEST_LOGPATH;
			break;
		case STEP_CAL1:
		case STEP_CALX:
		case STEP_TESTMODE:
			argv[1] = CAL_QUEST_LOGPATH;
			break;
		case STEP_MAIN:
			argv[1] = MAIN_QUEST_LOGPATH;
			break;
	}
	QUEST_PRINT("%s : will move questresult files to %s\n", __func__, argv[1]);

	call_user_prg(argv, UMH_WAIT_PROC);
}

static int call_quest_debugging_sh(char* action, int wait)
{
	char *argv[4] = { NULL, NULL, NULL, NULL };
	char step_str[10];

	QUEST_PRINT("%s : will call %s with action (%s)\n", __func__, QUEST_DEBUGGING_PRG, action);

	argv[0] = QUEST_DEBUGGING_PRG;
	argv[1] = action;
	snprintf(step_str, 10, "step:%d", param_quest_data.curr_step);
	argv[2] = step_str;
	return call_user_prg(argv, wait);
}

static enum quest_enum_item_result check_item_result( uint64_t item_result, uint32_t max_cnt )
{
	enum quest_enum_item_result result;
	int iCnt;

	if( item_result == 0 )
		return ITEM_RESULT_NONE;

	// check fail first
	for( iCnt=1; iCnt<max_cnt; iCnt++ )
	{
		result = QUEST_GET_ITEM_SUBITEM_RESULT(item_result, iCnt);
		if( result == ITEM_RESULT_FAIL )
			return ITEM_RESULT_FAIL;
	}

	// check incompleted
	for( iCnt=1; iCnt<max_cnt; iCnt++ )
	{
		result = QUEST_GET_ITEM_SUBITEM_RESULT(item_result, iCnt);
		if( result == ITEM_RESULT_INCOMPLETED )
			return ITEM_RESULT_INCOMPLETED;
	}

	return ITEM_RESULT_PASS;
}

static int check_if_incompleted_item_result_exist( uint64_t item_result, uint32_t max_cnt )
{
	enum quest_enum_item_result result;
	int iCnt;

	if( item_result == 0 )
		return 0;

	for( iCnt=1; iCnt<max_cnt; iCnt++ )
	{
		result = QUEST_GET_ITEM_SUBITEM_RESULT(item_result, iCnt);
		if( result == ITEM_RESULT_INCOMPLETED )
			return 1;
	}

	return 0;
}


// check smd_subitem_result and return result string
static int get_smd_subitem_result_string(char *buf, int piece)
{
	int iCnt, failed_cnt=0;

	if (piece == SUBITEM_ITEMSCOUNT) {
		for (iCnt = SUBITEM_NONE+1; iCnt < SUBITEM_ITEMSCOUNT; iCnt++) {
			switch (QUEST_GET_ITEM_SUBITEM_RESULT(param_quest_data.smd_subitem_result, iCnt)) {
			case ITEM_RESULT_FAIL:
				strcat(buf, "[F]");
				failed_cnt++;
				break;
			case ITEM_RESULT_PASS:
				strcat(buf, "[P]");
				break;
			default:
				strcat(buf, "[X]");
				break;
			}
		}
	} else {
		switch (QUEST_GET_ITEM_SUBITEM_RESULT(param_quest_data.smd_subitem_result, piece)) {
		case ITEM_RESULT_FAIL:
			strlcpy(buf, "FAIL", sizeof(buf));
			failed_cnt++;
			break;
		case ITEM_RESULT_PASS:
			strlcpy(buf, "PASS", sizeof(buf));
			break;
		default:
			strlcpy(buf, "NA", sizeof(buf));
			break;
		}
	}

	return failed_cnt;
}

#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
static void check_and_update_qdaf_result()
{
	int qdaf_failed_cnt;

	// get result of ITEM_SMDDLQDAF
	qdaf_failed_cnt = get_qdaf_failed_cnt();
	if( qdaf_failed_cnt > 0 ) {
		QUEST_PRINT("%s : ITEM_SMDDLQDAF was failed (failed_cnt=%d)\n", __func__, qdaf_failed_cnt);
		QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, ITEM_SMDDLQDAF, ITEM_RESULT_FAIL);
		QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_subitem_result, SUBITEM_SMDDLQDAF, ITEM_RESULT_FAIL);
	}else {
		QUEST_PRINT("%s : ITEM_SMDDLQDAF was succeeded\n", __func__ );
		QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, ITEM_SMDDLQDAF, ITEM_RESULT_PASS);
		QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_subitem_result, SUBITEM_SMDDLQDAF, ITEM_RESULT_PASS);
	}
	quest_sync_param_quest_data();
}
#endif

static void run_qdaf_in_background(enum quest_qdaf_action_t action)
{
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
	if( param_quest_data.curr_step == STEP_SMDDL &&
		((action == QUEST_QDAF_ACTION_CONTROL_START_WITH_PANIC) ||
		(action == QUEST_QDAF_ACTION_CONTROL_START_WITHOUT_PANIC)) ) {
		QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, ITEM_SMDDLQDAF, ITEM_RESULT_INCOMPLETED);
		QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_subitem_result, SUBITEM_SMDDLQDAF, ITEM_RESULT_INCOMPLETED);
		quest_sync_param_quest_data();
	}
#endif
	call_qdaf_from_quest_driver(action, UMH_WAIT_EXEC);
}

//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// initializer functions //////////////////////////
//////////////////////////////////////////////////
static void make_debugging_files()
{
#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	// refer to qpnp_pon_reason (index=boot_reason-1)
	QUEST_PRINT("%s : boot_reason was %d\n", __func__, boot_reason);
#endif
#endif

	// updatebootcount
	// do not call this with UMH_WAIT_PROC. it can cause race condition with init thread
	call_quest_debugging_sh("action:updatebootcount", UMH_WAIT_EXEC);
	msleep(1000);

	boot_count = call_quest_debugging_sh("action:getbootcount", UMH_WAIT_PROC);
	boot_count = (boot_count>=0)? (boot_count>>8) : 0;
	QUEST_PRINT("%s : boot_count = %d\n", __func__, boot_count);

	// ls
	call_quest_debugging_sh("action:ls", UMH_WAIT_PROC);

	// resethist
	call_quest_debugging_sh("action:resethist", UMH_WAIT_PROC);

	// lastkmsg
	call_quest_debugging_sh("action:lastkmsg", UMH_WAIT_PROC);
}

// TODO
static void check_abnormal_param()
{
	// The abnormal param have been checked by quest_check_abnormal_param_quest_data() at quest_setup.c of XBL
	// If we want to check it at kernel, please use this function
	return;
}




// initialize step for smd, cal and main
// move uefi log to output_log_path
static void setup_scenario()
{
	switch (param_quest_data.curr_step) {
	case STEP_SMDDL: {
		enum quest_enum_item_result qdaf_item_result, hlos_item_result, total_result;
		uint64_t smd_item_result_without_qdaf;
		int exist_incompleted=0;

		// smd scenario
		QUEST_PRINT("%s : (step=%d) smd scenario\n", __func__, STEP_SMDDL);

		// move boot questresult files to SMD directory
		move_questresult_to_sub_dir(STEP_SMDDL);

		// check current result for determining next action
		qdaf_item_result = QUEST_GET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, ITEM_SMDDLQDAF);
		hlos_item_result = QUEST_GET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, QUESTHLOS_HLOS_ITEM_SMD);
		smd_item_result_without_qdaf = param_quest_data.smd_item_result;
		QUEST_SET_ITEM_SUBITEM_RESULT(smd_item_result_without_qdaf, ITEM_SMDDLQDAF, ITEM_RESULT_PASS);
		exist_incompleted = check_if_incompleted_item_result_exist(smd_item_result_without_qdaf, ITEM_ITEMSCOUNT);
		total_result = check_item_result(smd_item_result_without_qdaf, ITEM_ITEMSCOUNT);

		if ( hlos_item_result == ITEM_RESULT_INCOMPLETED ) {
			// first boot -> run boot items -> android boot -> quest_hlos
			//	 -> enter to upload mode (or smpl) without starting ITEM_SMDDLQDAF -> JIG POWER OFF -> boot ?
			// let's  initialize step
			QUEST_PRINT("%s : (step=%d) reboot while running quest_hlos\n", __func__, STEP_SMDDL);
			QUEST_PRINT("%s : Let's check lastkmsg\n", __func__);

			QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_subitem_result);
			quest_initialize_curr_step();
		}else if ( total_result == ITEM_RESULT_FAIL ) {
			QUEST_PRINT("%s : (step=%d) total_result == ITEM_RESULT_FAIL, so initialize step\n", __func__, STEP_SMDDL);
			QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_subitem_result);
			quest_initialize_curr_step();
		}
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
		else if( qdaf_item_result == ITEM_RESULT_INCOMPLETED ) {
			// first boot -> run boot items -> android boot
			//   -> run quest_hlos -> ITEM_SMDDLQDAF -> JIG POWER OFF -> boot ?
			// let's write the result of SMDDL QDAF and initialize step
			QUEST_PRINT("%s : (step=%d) maybe booting after executing ITEM_SMDDLQDAF\n",
				__func__, STEP_SMDDL);

			check_and_update_qdaf_result();
			QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_subitem_result); // due to boot after abnormal reset
			quest_initialize_curr_step();
		}
		else if( exist_incompleted == 1 )
		{
			// first boot -> run boot items -> incompleted -> android boot
			// let's ignore running hlos and just run smddl qdaf
			//  (do not update smd_subitem for QUESTHLOS_HLOS_ITEM_SMD)
			QUEST_PRINT("%s : (step=%d) incompleted at boot items, so ignore running hlos and just run smddl qdaf\n", __func__, STEP_SMDDL);

			QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, QUESTHLOS_HLOS_ITEM_SMD, ITEM_RESULT_INCOMPLETED);
			quest_sync_param_quest_data();

			// trigger SMDDL QDAF in background only if SMDDL line
			if( boot_count == 1 ) {
#if defined(CONFIG_SEC_QUEST_NOT_TRIGGER_SMDDL_QDAF)
				QUEST_PRINT("%s : SMDDL line, but skip to run SMDDL QDAF\n", __func__);
				check_and_update_qdaf_result();
				QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_subitem_result); // due to boot after abnormal reset
				quest_initialize_curr_step();
#else
				QUEST_PRINT("%s : SMDDL line, so run SMDDL QDAF\n", __func__);
				run_qdaf_in_background(QUEST_QDAF_ACTION_CONTROL_START_WITHOUT_PANIC);
#endif
			}
			else {
				QUEST_PRINT("%s : ERASE seq, so do not run SMDDL QDAF and finish step\n", __func__);
				check_and_update_qdaf_result();
				QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_subitem_result); // due to boot after abnormal reset
				quest_initialize_curr_step();
			}
		}
#endif
		break;
	}
	case STEP_TESTMODE:
	{
		int idx, ddr_err_total = param_quest_ddr_result_data.ddr_err_addr_total;

		if( 0 != ddr_err_total )
		{
			QUEST_PRINT("%s : ddr_err_addr_total=%d\n", __func__, ddr_err_total);

			if (ddr_err_total > MAX_DDR_ERR_ADDR_CNT)
				ddr_err_total = MAX_DDR_ERR_ADDR_CNT;

			for( idx=0; idx<ddr_err_total; idx++ )
			{
				QUEST_PRINT( "ddr err addr : 0x%llx \n",param_quest_ddr_result_data.ddr_err_addr[idx] );
				param_quest_ddr_result_data.ddr_err_addr[idx] = 0;
			}
			param_quest_ddr_result_data.ddr_err_addr_total = 0;
			quest_sync_param_quest_ddr_result_data();

			// triger panic
			QUEST_PRINT("%s : trigger panic\n", __func__);
			panic("ddrscan failed");
		}
	}
	case STEP_CAL1:
	case STEP_CALX:

		// cal scenario
		QUEST_PRINT("%s : (step=%d) cal scenario\n", __func__, param_quest_data.curr_step);

		// move boot questresult files to CAL directory
		move_questresult_to_sub_dir(param_quest_data.curr_step);

		if( param_quest_data.hlos_remained_count==0 ) {
			QUEST_PRINT("%s : scenario ends\n", __func__);

			// initialize step
			quest_initialize_curr_step();
		}

		break;
	case STEP_MAIN:

		// click MAIN button -> quest_hlos -> ... -> ITEM_QUESTQUEFI * X
		// -> ITEM_QUESTSUEFI -> ITEM_DDRSCANRAMDUMPENCACHE -> boot ?
		QUEST_PRINT("%s : (step=%d) maybe booting completing main scenario\n", __func__, STEP_MAIN);

		// move boot questresult files to MAIN directory
		move_questresult_to_sub_dir(STEP_MAIN);

		// initialize step
		quest_initialize_curr_step();

#if !defined(CONFIG_SEC_QUEST_NOT_TRIGGER_MAIN_QDAF)
		// will run run_qdaf_in_background() later
		call_main_qdaf_after_finighing_main_quest = 1;
#endif

		break;
	default: {
		int qdaf_failed_cnt;

		QUEST_PRINT("%s : (step=%d) default actions \n", __func__, param_quest_data.curr_step);

		// default action #1
		//    : get qdaf result to check if qdaf was executed before boot and it was failed
		qdaf_failed_cnt = get_qdaf_failed_cnt();
		QUEST_PRINT("%s : qdaf failed_cnt = %d\n", __func__, qdaf_failed_cnt);

		// default action #2
		// ...

		}
	}

}


static int initialized = 0;
static void __initialize()
{
	if( likely(initialized) ) return;
	initialized = 1;

	QUEST_PRINT("%s +++\n", __func__);

	// load param data
	quest_load_param_quest_data();
	quest_load_param_quest_ddr_result_data();
	quest_load_param_api_gpio_test();
	quest_load_param_api_gpio_test_result();

	// print param_quest_data
	quest_print_param_quest_data();

#if defined(CONFIG_SEC_QUEST_BPS_CLASSIFIER)
	// update nad bps param here!
	sec_quest_bps_param_read();
#endif

	// make debugging files
	make_debugging_files();

	// check abnormal param
	check_abnormal_param();

	// setup scenario
	setup_scenario();

	QUEST_PRINT("%s ---\n", __func__);
}

static void quest_sysfs_enter(__const char *__function)
{
	mutex_lock(&sysfs_common_lock);
	__initialize();
	QUEST_PRINT("%s +++\n", __function);
}

static void quest_sysfs_exit(__const char *__function)
{
	QUEST_PRINT("%s ---\n", __function);
	mutex_unlock(&sysfs_common_lock);
}

#define QUEST_SYSFS_ENTER()	quest_sysfs_enter(__func__)
#define QUEST_SYSFS_EXIT()	quest_sysfs_exit(__func__)
//////////////////////////////////////////////////




//////////////////////////////////////////////////
/////// NAD_END //////////////////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_end(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	char result[20] = { '\0' };
	int failed = 0;
#if defined(CONFIG_SEC_QUEST_HLOS_NATURESCENE_SMD)
	enum quest_enum_item_result hlos_result = ITEM_RESULT_NONE;
#endif

	QUEST_SYSFS_ENTER();

	// print result
	sscanf(buf, "%s", result);
	failed = strcmp(result, "quest_pass")?1:0;

	// process end
	switch(param_quest_data.curr_step) {
	case STEP_SMDDL:

		// update item result
		if( failed ) {
			QUEST_PRINT("%s : SMD quest_hlos was failed (%s)\n", __func__, result);
			QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, QUESTHLOS_HLOS_ITEM_SMD, ITEM_RESULT_FAIL);
		}else {
			QUEST_PRINT("%s : SMD quest_hlos was succeeded\n", __func__ );
			QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, QUESTHLOS_HLOS_ITEM_SMD, ITEM_RESULT_PASS);
#if defined(CONFIG_SEC_QUEST_HLOS_NATURESCENE_SMD)
			hlos_result = QUEST_GET_ITEM_SUBITEM_RESULT(param_quest_data.smd_subitem_result, SUBITEM_QUESTHLOSNATURESCENE);
			if( hlos_result == ITEM_RESULT_INCOMPLETED ) {
				QUEST_PRINT("%s : SUBITEM_QUESTHLOSNATURESCENE was incompleted, so update SMD quest_hlos as incompleted  \n", __func__ );
				QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, QUESTHLOS_HLOS_ITEM_SMD, ITEM_RESULT_INCOMPLETED);
			}
#endif
		}
		quest_sync_param_quest_data();

#if defined(CONFIG_SEC_DDR_SKP)
		param_quest_data.curr_step = STEP_CALX;
		quest_sync_param_quest_data();
#endif

#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
		// trigger SMDDL QDAF in background only if SMDDL line
		if( boot_count == 1 ) {
#if defined(CONFIG_SEC_QUEST_NOT_TRIGGER_SMDDL_QDAF)
			QUEST_PRINT("%s : SMDDL line, but skip to run SMDDL QDAF\n", __func__);
			check_and_update_qdaf_result();
			QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_subitem_result);
			quest_initialize_curr_step();
#else
			QUEST_PRINT("%s : SMDDL line, so run SMDDL QDAF\n", __func__);
			run_qdaf_in_background(QUEST_QDAF_ACTION_CONTROL_START_WITHOUT_PANIC);
#endif
		}
		else {
			QUEST_PRINT("%s : ERASE seq, so do not run SMDDL QDAF and finish step\n", __func__);
			check_and_update_qdaf_result();
			QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_subitem_result);
			quest_initialize_curr_step();
		}
#else
		QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_subitem_result);
		quest_initialize_curr_step();
#endif

		// send "NAD_TEST=DONE" to factory app
		kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, quest_uevent.envp);
#if defined(CONFIG_SEC_DDR_SKP)
		if( failed ) {
			// triger panic
			QUEST_PRINT("%s : trigger panic\n", __func__);
			panic("%s", result);
		}
#endif
		break;
	case STEP_CAL1:
	case STEP_CALX:
	case STEP_TESTMODE:

		if( failed ) {
			// set result as FAIL
			QUEST_PRINT("%s : CAL quest_hlos was failed (%s)\n", __func__, result);
			QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.cal_item_result, QUESTHLOS_HLOS_ITEM_MAIN_CAL, ITEM_RESULT_FAIL);
			quest_sync_param_quest_data();

			if( param_quest_data.curr_step==STEP_TESTMODE ) {
				QUEST_PRINT("%s : ************* DO NOT INITIALIZE STEP AND COUNT for continuoly repeating *************\n", __func__);

				// triger panic
				QUEST_PRINT("%s : trigger panic\n", __func__);
				panic("%s", result);
			}

			// initialize remained count
			// initialize step
			QUEST_PRINT("%s : initialize param and step\n", __func__ );
			param_quest_data.hlos_remained_count = 0;
			param_quest_data.ddrscan_remained_count = 0;
#if defined(CONFIG_SEC_QUEST_UEFI)
			param_quest_data.quefi_remained_count = 0;
#endif
#if defined(CONFIG_SEC_QUEST_UEFI_ENHANCEMENT)
			param_quest_data.suefi_remained_count = 0;
#endif
			quest_initialize_curr_step();

			// triger panic
			QUEST_PRINT("%s : trigger panic\n", __func__);
			panic("%s", result);

		}else {
			// set result as PASS
			QUEST_PRINT("%s : CAL quest_hlos was succeeded\n", __func__ );
			QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.cal_item_result, QUESTHLOS_HLOS_ITEM_MAIN_CAL, ITEM_RESULT_PASS);
			quest_sync_param_quest_data();

			// send "NAD_TEST=DONE" to factory app
			kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, quest_uevent.envp);
		}

		break;
	case STEP_MAIN:

		if( failed ) {
			// set result as FAIL
			QUEST_PRINT("%s : MAIN quest_hlos was failed (%s)\n", __func__, result);
			QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.main_item_result, QUESTHLOS_HLOS_ITEM_MAIN_CAL, ITEM_RESULT_FAIL);
			quest_sync_param_quest_data();

			// triger panic
			QUEST_PRINT("%s : trigger panic\n", __func__);
			panic("%s", result);

		}else {
			// set result as PASS
			QUEST_PRINT("%s : MAIN quest_hlos was succeeded\n", __func__);
			QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.main_item_result, QUESTHLOS_HLOS_ITEM_MAIN_CAL, ITEM_RESULT_PASS);
			quest_sync_param_quest_data();

			// send "NAD_TEST=DONE" to factory app
			kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, quest_uevent.envp);
		}

		break;
	}

	QUEST_SYSFS_EXIT();
	return count;
}
static DEVICE_ATTR(nad_end, S_IWUSR, NULL, store_quest_end);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_ACAT /////////////////////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_acat(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	enum quest_enum_item_result total_result = ITEM_RESULT_NONE;
	ssize_t count = 0;

	QUEST_SYSFS_ENTER();

	// check smd_item_result
	total_result = check_item_result(param_quest_data.cal_item_result, ITEM_ITEMSCOUNT);
	QUEST_PRINT("%s : cal_item_result(%llu) total_result(%d)\n",
					__func__, param_quest_data.cal_item_result, total_result);

#if defined(CONFIG_SEC_QUEST_ALWAYS_RETURN_PASS_FOR_ACAT)
	QUEST_PRINT("%s : CONFIG_SEC_QUEST_ALWAYS_RETURN_PASS_FOR_ACAT enabled, so return pass\n", __func__);
	total_result = ITEM_RESULT_PASS;
#endif

	if( param_quest_data.curr_step == STEP_TESTMODE && total_result == ITEM_RESULT_FAIL )
	{
		QUEST_PRINT("%s : in the case of STEP_TESTMODEC, force to return ITEM_RESULT_PASS to continue repeating\n", __func__);
		total_result = ITEM_RESULT_PASS;		
	}

	switch (total_result) {
		case ITEM_RESULT_PASS: {
			QUEST_PRINT("ACAT QUEST PASS\n");
			count = snprintf(buf, BUFF_SZ, "OK_ACAT_NONE\n");
		} break;
		case ITEM_RESULT_FAIL: {
			QUEST_PRINT("ACAT QUEST FAIL\n");
			count = snprintf(buf, BUFF_SZ, "NG_ACAT_ASV\n");
		} break;

		case ITEM_RESULT_INCOMPLETED: {
			QUEST_PRINT("ACAT QUEST INCOMPLETED\n");
			count = snprintf(buf, BUFF_SZ, "OK\n");
		} break;

		case ITEM_RESULT_NONE: {
			QUEST_PRINT("ACAT QUEST NOT_TESTED\n");
			count = snprintf(buf, BUFF_SZ, "OK\n");
		} break;
	}

	QUEST_SYSFS_EXIT();
	return count;
}

static ssize_t store_quest_acat(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret = -1;
	int idx = 0;
	int quest_loop_count, dram_loop_count;
	char temp[QUEST_BUFF_SIZE * 3];
	char quest_cmd[QUEST_CMD_LIST][QUEST_BUFF_SIZE];
	char *quest_ptr, *string;

	QUEST_SYSFS_ENTER();

	// check exceptional cases
	if( unlikely( erased || strncmp(buf, "nad_acat", 8) ) ) {
		QUEST_PRINT("%s : exceptional cases (erased=%d, buf=%s\n",
			__func__, erased, buf);
		goto out;
	}

	// parse argument
	strlcpy(temp, buf, QUEST_BUFF_SIZE * 3);
	string = temp;
	while (idx < QUEST_CMD_LIST) {
		quest_ptr = strsep(&string, ",");
		strlcpy(quest_cmd[idx++], quest_ptr, QUEST_BUFF_SIZE);
	}

	// get quest_loop_count and dram_loop_count
	ret = sscanf(quest_cmd[1], "%d\n", &quest_loop_count);
	if (ret != 1) return -EINVAL;
	ret = sscanf(quest_cmd[2], "%d\n", &dram_loop_count);
	if (ret != 1) return -EINVAL;
	QUEST_PRINT("%s : nad_acat%d,%d\n",
		__func__, quest_loop_count, dram_loop_count);

	// nad_acat,0,0
	// trigger SMD quest_hlos or CAL with remained_count
	if( (quest_loop_count==0) && (dram_loop_count==0) ) {

		// S   M   D
		// check if smd quest_hlos should start
		if( unlikely(step_to_smd_quest_hlos && (param_quest_data.curr_step==STEP_SMDDL)) ) {
			QUEST_PRINT("%s : cur_step==STEP_SMDDL and will triger quest_hlos\n", __func__);

			// update item result
			QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, QUESTHLOS_HLOS_ITEM_SMD, ITEM_RESULT_INCOMPLETED);
			quest_sync_param_quest_data();

			// trigger quest_hlos
			do_quest();
			goto out;
		}

		// check exceptional cases
		if( unlikely( (param_quest_data.curr_step!=STEP_TESTMODE &&
						param_quest_data.curr_step!=STEP_CALX && param_quest_data.curr_step!=STEP_CAL1) ||
						(param_quest_data.hlos_remained_count <= 0) ) ) {
			QUEST_PRINT("%s : exceptional cases (step=%d, hlos_cnt=%d)\n",
				__func__, param_quest_data.curr_step, param_quest_data.hlos_remained_count);
			goto out;
		}

		// C   A   L
		// update remained_count and item result
		QUEST_PRINT("%s : will triger quest_hlos\n", __func__);
		param_quest_data.hlos_remained_count--;
		QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.cal_item_result, QUESTHLOS_HLOS_ITEM_MAIN_CAL, ITEM_RESULT_INCOMPLETED);
		quest_sync_param_quest_data();

		// trigger quest_hlos
		do_quest();

	}else {

		QUEST_PRINT("%s : update step, item_result and remained_count\n", __func__);

		if( testmode_enabled ) {
				QUEST_PRINT("%s : testmode_enabled, so set step as STEP_TESTMODE\n", __func__);
				param_quest_data.curr_step = STEP_TESTMODE;
				param_quest_data.cal_item_result = (uint64_t)0;
				param_quest_data.hlos_remained_count = quest_loop_count;
				param_quest_data.quefi_remained_count = 0;
				param_quest_data.suefi_remained_count = 0;
				param_quest_data.ddrscan_remained_count = 0;

				QUEST_PRINT("%s : testmode_enabled, so set count as feature and property value\n", __func__);
#if defined(CONFIG_SEC_QUEST_UEFI)
				if( testmode_quefi_enabled )
					param_quest_data.quefi_remained_count = quest_loop_count;
#endif
#if defined(CONFIG_SEC_QUEST_UEFI_ENHANCEMENT)
				if( testmode_suefi_enabled )
					param_quest_data.suefi_remained_count = quest_loop_count;
#endif
				if( testmode_ddrscan_enabled )
					param_quest_data.ddrscan_remained_count = quest_loop_count;

		}
		else {
			if( (quest_loop_count==1) && (dram_loop_count==0) ) {
				// nad_acat,1,0 (trigger CAL 1time)
				param_quest_data.curr_step = STEP_CAL1;
				param_quest_data.cal_item_result = (uint64_t)0;
				param_quest_data.ddrscan_remained_count = 0;
				param_quest_data.hlos_remained_count = quest_loop_count;
				param_quest_data.quefi_remained_count = 0;
				param_quest_data.suefi_remained_count = 0;
			}else {
				// nad_acat,x,y (trigger CAL Xtime and DDR_SCAN Ytime)
				param_quest_data.curr_step = STEP_CALX;
				param_quest_data.cal_item_result = (uint64_t)0;
				param_quest_data.ddrscan_remained_count = dram_loop_count;
				param_quest_data.hlos_remained_count = quest_loop_count;

				// if quest_loop_count==1, run hlos only
				// if quest_loop_count==0, set quefi,suefi count to 0 also
#if defined(CONFIG_SEC_QUEST_UEFI)
				if( quest_loop_count != 1 )
					param_quest_data.quefi_remained_count = quest_loop_count;
#else
				param_quest_data.quefi_remained_count = 0;
#endif
#if defined(CONFIG_SEC_QUEST_UEFI_ENHANCEMENT)
				if( quest_loop_count != 1 )
					param_quest_data.suefi_remained_count = quest_loop_count;
#else
				param_quest_data.suefi_remained_count = 0;
#endif
			}
		}
		quest_sync_param_quest_data();

		QUEST_PRINT("%s : not trigger quest_hlos and not reboot\n", __func__);
	}

out:
	QUEST_SYSFS_EXIT();
	return count;

}
static DEVICE_ATTR(nad_acat, S_IRUGO | S_IWUSR, show_quest_acat, store_quest_acat);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_STAT /////////////////////////////////
//////////////////////////////////////////////////
char str_bps[CPR_BPS_SZ_BYTE] = { '\0' };

#if defined(CONFIG_SEC_QUEST_BPS_CLASSIFIER)
static void make_bps_stat_string(char *bps_str)
{
	snprintf(bps_str, CPR_BPS_SZ_BYTE, "%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d",
			bps_envs.up_cnt.sp, bps_envs.up_cnt.wp, bps_envs.up_cnt.dp,
			bps_envs.up_cnt.kp, bps_envs.up_cnt.mp, bps_envs.up_cnt.tp,
			bps_envs.up_cnt.cp, bps_envs.pc_lr_cnt, bps_envs.pc_lr_last_idx,
			bps_envs.tzerr_cnt, bps_envs.klg_cnt, bps_envs.dn_cnt);
}
#endif
static void make_additional_stat_string(char *additional_str)
{
#if defined(CONFIG_SEC_QUEST_BPS_CLASSIFIER)
	/* if bps data successfully initialized and if magic needs to be checked */
	if (sec_quest_bps_env_initialized &&
		bps_envs.magic[1] == QUEST_BPS_CLASSIFIER_MAGIC2) {
		make_bps_stat_string(str_bps);
	}else
		snprintf(str_bps, CPR_BPS_SZ_BYTE, "-");
#else
	snprintf(str_bps, CPR_BPS_SZ_BYTE, "-");
#endif

	snprintf(additional_str, MAX_LEN_STR,
		"FQUITH(%d),FQUETH(%d)," \
		"FSUITH(%d),FSUETH(%d)," \
		"FSSIR(%llX)," \
		"FDET(%d)," \
		"FQUET(%d)," \
		"FSUET(%d)," \
		"FQUPT(%d)," \
		"FBR(%.2s)," \
		"FHST(%d),FHET(%d)," \
		"FHITH(%d),FHMTH(%d)," \
		"FNSR(%d)," \
		"FMATD(%d)," \
		"SSIR(%llX)," \
		"DET(%d)," \
		"QUET(%d)," \
		"SUET(%d)," \
		"QUPT(%d)," \
		"SCT(%d)," \
		"SCTH(%d)," \
		"BR(%.2s)," \
		"HST(%d),HET(%d)," \
		"HITH(%d),HMTH(%d)," \
		"NSR(%d)," \
		"MATD(%d)," \
		"BPS(%s)," \
		"CHIP(%llX),"\
		"CPER(%d)",		
		param_quest_data.smd_quefi_init_thermal_first, param_quest_data.smd_quefi_end_thermal_first,
		param_quest_data.smd_suefi_init_thermal_first, param_quest_data.smd_suefi_end_thermal_first,
		param_quest_data.smd_subitem_result_first,
		param_quest_data.smd_ddrscan_elapsed_time_first,
		param_quest_data.smd_quefi_elapsed_time_first,
		param_quest_data.smd_suefi_elapsed_time_first,
		param_quest_data.smd_quefi_total_pause_time_first,
		param_quest_data.smd_boot_reason_first,
		param_quest_data.smd_hlos_start_time_first, param_quest_data.smd_hlos_elapsed_time_first,
		param_quest_data.smd_hlos_init_thermal_first, param_quest_data.smd_hlos_max_thermal_first,
		param_quest_data.smd_ns_repeats_first,
		param_quest_data.smd_max_aoss_thermal_diff_first,
		param_quest_data.smd_subitem_result,
		param_quest_data.smd_ddrscan_elapsed_time,
		param_quest_data.smd_quefi_elapsed_time,
		param_quest_data.smd_suefi_elapsed_time,
		param_quest_data.smd_quefi_total_pause_time,
		param_quest_data.smd_ft_self_cooling_time,
		param_quest_data.smd_ft_thermal_after_self_cooling,
		param_quest_data.smd_boot_reason,
		param_quest_data.smd_hlos_start_time,param_quest_data.smd_hlos_elapsed_time,
		param_quest_data.smd_hlos_init_thermal, param_quest_data.smd_hlos_max_thermal,
		param_quest_data.smd_ns_repeats,
		param_quest_data.smd_max_aoss_thermal_diff,
		str_bps,
		(param_quest_data.ap_serial)? ((param_quest_data.real_smd_register_value)? 
			param_quest_data.ap_serial^(0xfff00000 | param_quest_data.real_smd_register_value):0):0,
		param_quest_data.smd_cper);

	QUEST_PRINT("%s : additional_str : %s\n", __func__, additional_str);
}

static ssize_t show_quest_stat(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
	enum quest_enum_item_result qdaf_result = ITEM_RESULT_NONE;
	uint64_t smd_item_result_without_qdaf;
	int exist_incompleted=0;

#endif
	enum quest_enum_item_result total_result = ITEM_RESULT_NONE;
	enum quest_enum_item_result hlos_result = ITEM_RESULT_NONE;
	ssize_t count = 0;
	char additional_str[MAX_LEN_STR] = { '\0' };

	QUEST_SYSFS_ENTER();

#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
	qdaf_result = QUEST_GET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, ITEM_SMDDLQDAF);
	if( qdaf_result==ITEM_RESULT_PASS || qdaf_result==ITEM_RESULT_FAIL )
	{
		QUEST_PRINT("%s : ITEM_SMDDLQDAF was completed, so include its result into total_result\n", __func__);
		total_result = check_item_result(param_quest_data.smd_item_result, ITEM_ITEMSCOUNT);
		exist_incompleted = check_if_incompleted_item_result_exist(param_quest_data.smd_item_result, ITEM_ITEMSCOUNT);
	}else
	{
		// check smd_item_result
		// There is a possiblity that this function is called before checking qdaf result
		// So, let's ignore the result of qdaf
		QUEST_PRINT("%s : ITEM_SMDDLQDAF was not completed, so ignore its result from total_result\n", __func__);
		smd_item_result_without_qdaf = param_quest_data.smd_item_result;
		QUEST_SET_ITEM_SUBITEM_RESULT(smd_item_result_without_qdaf, ITEM_SMDDLQDAF, ITEM_RESULT_PASS);
		total_result = check_item_result(smd_item_result_without_qdaf, ITEM_ITEMSCOUNT);
		exist_incompleted = check_if_incompleted_item_result_exist(smd_item_result_without_qdaf, ITEM_ITEMSCOUNT);
	}
	QUEST_PRINT("%s : smd_item_result(%llu) total_result(%d)\n",
				__func__, param_quest_data.smd_item_result, total_result);

	if( (total_result == ITEM_RESULT_FAIL && exist_incompleted) || (param_quest_data.curr_step!=STEP_SMDDL) )
	{
		QUEST_PRINT("%s : in this case, the curr_step is none due to incompleted subitem\n", __func__);
		QUEST_PRINT("%s : let's skip hlos subitem as the policy with incompletion\n", __func__);
	}
	// If the total_result is ITEM_RESULT_INCOMPLETED before running HLOS,
	//   it means the DDRSCAN, QUEFI or SUEFI was not completed.
	//   So we do not have to run HLOS and set the smd result as REWORK.
	// Otherwise, the followings are needed.
	else if( total_result != ITEM_RESULT_INCOMPLETED ) {

		// This function check NOT_TESTED and TESTING using only the result of quest_hlos
		//  regardless of its existence at STEP_SMDDL
		// If it is NON_TESTED, we will trigger quest_hlos or dummy sh later
		hlos_result = QUEST_GET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, QUESTHLOS_HLOS_ITEM_SMD);
		QUEST_PRINT("%s : the result of quest_hlos at smd_item_result (%d)\n",
						__func__, hlos_result);
		if( hlos_result == ITEM_RESULT_NONE ) {
			QUEST_PRINT("%s : set step_to_smd_quest_hlos=1\n", __func__);
			step_to_smd_quest_hlos = 1;
			QUEST_PRINT("SMD QUEST NOT_TESTED\n");
			count = snprintf(buf, MAX_LEN_STR, "NOT_TESTED\n");
			goto out;
		}else if( (hlos_result==ITEM_RESULT_INCOMPLETED) &&
					(param_quest_data.curr_step==STEP_SMDDL) ) {
			QUEST_PRINT("SMD QUEST TESTING\n");
			count = snprintf(buf, MAX_LEN_STR, "TESTING\n");
			goto out;
		}
	}
#else
	if( param_quest_data.curr_step==STEP_SMDDL ) {

		hlos_result = QUEST_GET_ITEM_SUBITEM_RESULT(param_quest_data.smd_item_result, QUESTHLOS_HLOS_ITEM_SMD);
		QUEST_PRINT("%s : hlos_result(%d) total_result(%d)\n",
				__func__, hlos_result, total_result);

		if( hlos_result == ITEM_RESULT_NONE ) {
			QUEST_PRINT("%s : set step_to_smd_quest_hlos=1\n", __func__);
			step_to_smd_quest_hlos = 1;
			QUEST_PRINT("SMD QUEST NOT_TESTED\n");
			count = snprintf(buf, MAX_LEN_STR, "NOT_TESTED\n");
			goto out;
		}else if( hlos_result==ITEM_RESULT_INCOMPLETED ) {
			QUEST_PRINT("SMD QUEST TESTING\n");
			count = snprintf(buf, MAX_LEN_STR, "TESTING\n");
			goto out;
		}
	}

	total_result = check_item_result(param_quest_data.smd_item_result, ITEM_ITEMSCOUNT);
#endif

	make_additional_stat_string(additional_str);

	switch (total_result) {
		case ITEM_RESULT_PASS: {
			QUEST_PRINT("%s : SMD QUEST PASS\n", __func__);
			count = snprintf(buf, MAX_LEN_STR, "OK_3.1,%s\n", additional_str);
		} break;
		case ITEM_RESULT_FAIL: {
			char strResult[BUFF_SZ-14] = { '\0', };
			get_smd_subitem_result_string(strResult, SUBITEM_ITEMSCOUNT);
			QUEST_PRINT("%s : SMD QUEST FAIL\n", __func__);
			count = snprintf(buf, MAX_LEN_STR, "NG_3.1_FAIL_%s,%s\n", strResult, additional_str);
		} break;

		case ITEM_RESULT_INCOMPLETED: {
			QUEST_PRINT("%s : SMD QUEST INCOMPLETED\n", __func__);
			count = snprintf(buf, MAX_LEN_STR, "RE_WORK,%s\n", additional_str);
		} break;

		case ITEM_RESULT_NONE: {
			QUEST_PRINT("%s : SMD QUEST NOT_TESTED\n", __func__);
			count = snprintf(buf, MAX_LEN_STR, "NOT_TESTED\n");
		} break;
	}

out:
	QUEST_SYSFS_EXIT();
	return count;
}

static DEVICE_ATTR(nad_stat, S_IRUGO, show_quest_stat, NULL);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_RESULT ///////////////////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_smd_subitem_result(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int iCnt;

	QUEST_SYSFS_ENTER();

	for (iCnt = SUBITEM_NONE+1; iCnt < SUBITEM_ITEMSCOUNT; iCnt++) {
		char strResult[QUEST_BUFF_SIZE] = { '\0', };
		get_smd_subitem_result_string(strResult, iCnt);
		info_size +=
		    snprintf((char *)(buf + info_size), MAX_LEN_STR - info_size,
			     "\"%s\":\"%s\",", STR_SUBITEM[iCnt], strResult);
	}
	info_size += snprintf((char *)(buf + info_size), MAX_LEN_STR - info_size,"\n");

	QUEST_PRINT("%s : smd_subitem_result(%llu)=%s\n", __func__, param_quest_data.smd_subitem_result, buf);

	QUEST_SYSFS_EXIT();
	return info_size;
}

static ssize_t store_quest_smd_subitem_result(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	enum quest_enum_item_result _result = ITEM_RESULT_NONE;
	enum quest_enum_smd_subitem item = SUBITEM_NONE;
	char test_name[QUEST_BUFF_SIZE * 2] = { '\0', };
	char result_string[QUEST_BUFF_SIZE] = { '\0', };
	char temp[QUEST_BUFF_SIZE * 3] = { '\0', };
	char quest_test[2][QUEST_BUFF_SIZE * 2];	// 2: "test_name", "result"
	char *quest_ptr, *string;

	QUEST_SYSFS_ENTER();

	// parse argument
	strlcpy(temp, buf, QUEST_BUFF_SIZE * 3);
	string = temp;
	quest_ptr = strsep(&string, ",");
	strlcpy(quest_test[0], quest_ptr, QUEST_BUFF_SIZE * 2);
	quest_ptr = strsep(&string, ",");
	strlcpy(quest_test[1], quest_ptr, QUEST_BUFF_SIZE * 2);
	sscanf(quest_test[0], "%s", test_name);
	sscanf(quest_test[1], "%s", result_string);
	QUEST_PRINT("%s : test_name(%s), test result(%s)\n", __func__, test_name, result_string);

	// match enum quest_enum_item_result
	if (TEST_PASS(result_string)) _result = ITEM_RESULT_PASS;
	else if (TEST_FAIL(result_string)) _result = ITEM_RESULT_FAIL;
	else if (TEST_NA(result_string)) _result = ITEM_RESULT_INCOMPLETED;
	else _result = ITEM_RESULT_NONE;

	// match enum quest_enum_smd_subitem
#if defined(CONFIG_SEC_QUEST_HLOS_DUMMY_SMD)
	if (TEST_QDAF(test_name)) item = SUBITEM_SMDDLQDAF;
	else if (TEST_DUMMY(test_name)) item = SUBITEM_QUESTHLOSDUMMY;
#elif defined(CONFIG_SEC_QUEST_HLOS_NATURESCENE_SMD)
	if (TEST_NATURESCENE(test_name)) item = SUBITEM_QUESTHLOSNATURESCENE;
	else if (TEST_AOSSTHERMALDIFF(test_name)) item = SUBITEM_QUESTHLOSAOSSTHERMALDIFF;
#else
	if (TEST_CRYPTO(test_name)) item = SUBITEM_QUESTHLOSCRYPTO;
	else if (TEST_ICACHE(test_name)) item = SUBITEM_QUESTHLOSICACHE;
	else if (TEST_CCOHERENCY(test_name)) item = SUBITEM_QUESTHLOSCCOHERENCY;
	else if (TEST_QMESADDR(test_name)) item = SUBITEM_QUESTHLOSQMESADDR;
	else if (TEST_QMESACACHE(test_name)) item = SUBITEM_QUESTHLOSQMESACACHE;
	else if (TEST_SUSPEND(test_name)) item = SUBITEM_QUESTHLOSSUSPEND;
	else if (TEST_VDDMIN(test_name)) item = SUBITEM_QUESTHLOSVDDMIN;
	else if (TEST_THERMAL(test_name)) item = SUBITEM_QUESTHLOSTHERMAL;
	else if (TEST_UFS(test_name)) item = SUBITEM_QUESTHLOSUFS;
	else if (TEST_A75G(test_name)) item = SUBITEM_QUESTFUSIONA75G;
	else if (TEST_Q65G(test_name)) item = SUBITEM_QUESTFUSIONQ65G;
#endif

	// check exceptional cases
	if( unlikely( (item==SUBITEM_NONE) || (param_quest_data.curr_step!=STEP_SMDDL) ) ) {
		QUEST_PRINT("%s : exceptional cases (item=%d, step=%d)\n",
			__func__, item, param_quest_data.curr_step );
		goto out;
	}

	// update param
	QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.smd_subitem_result, item, _result);
	quest_sync_param_quest_data();

out:
	QUEST_SYSFS_EXIT();
	return count;
}
static DEVICE_ATTR(nad_result, S_IRUGO | S_IWUSR,
	show_quest_smd_subitem_result, store_quest_smd_subitem_result);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_ERASE ////////////////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_erase(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	char *argv[4] = { NULL, NULL, NULL, NULL };
	int init_param_api_gpio_test = 0;
	char init_param_api_gpio_test_result[256] = { 0, };

	QUEST_SYSFS_ENTER();

	// check exceptional cases
	if( unlikely( erased || strncmp(buf, "erase", 5) ) ) {
		QUEST_PRINT("%s : exceptional cases (erased=%d, buf=%s\n",
			__func__, erased, buf);
		goto out;
	}

	// set erased = 1 to prevent do_quest()
	erased = 1;

	// call remove_files.sh
	argv[0] = ERASE_QUEST_PRG;
	argv[1] = "all";
	call_user_prg(argv, UMH_WAIT_EXEC);

	// clearing param_quest_data
	quest_clear_param_quest_data();

	if( strncmp(buf, "eraseall", 8)==0 ) {
		QUEST_PRINT("%s : clear also first_xxx just for debugging purpose\n", __func__);
		param_quest_data.smd_subitem_result_first = 0;
		param_quest_data.smd_quefi_init_thermal_first = 0;
		param_quest_data.smd_quefi_end_thermal_first = 0;
		param_quest_data.smd_suefi_init_thermal_first = 0;
		param_quest_data.smd_suefi_end_thermal_first = 0;
		param_quest_data.smd_ddrscan_elapsed_time_first = 0;
		param_quest_data.smd_quefi_elapsed_time_first = 0;
		param_quest_data.smd_suefi_elapsed_time_first = 0;
		param_quest_data.smd_quefi_total_pause_time_first = 0;
		param_quest_data.smd_boot_reason_first[0] = '\0';
		param_quest_data.smd_hlos_start_time_first = 0;
		param_quest_data.smd_hlos_elapsed_time_first = 0;
		param_quest_data.smd_hlos_init_thermal_first = 0;
		param_quest_data.smd_hlos_max_thermal_first = 0;
		param_quest_data.smd_ns_repeats_first = 0;
		param_quest_data.smd_max_aoss_thermal_diff_first = 0;
		param_quest_data.real_smd_register_value = 0;
		param_quest_data.ap_serial = 0;
		param_quest_data.num_smd_try = 0;
		param_quest_data.smd_cper = 0;
		quest_sync_param_quest_data();
	}

	// clearing API test result
	param_api_gpio_test = init_param_api_gpio_test;
	strlcpy(param_api_gpio_test_result, init_param_api_gpio_test_result, 256);
	quest_sync_param_api_gpio_test();
	quest_sync_param_api_gpio_test_result();

out:
	QUEST_SYSFS_EXIT();
	return count;
}

static ssize_t show_quest_erase(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	if (erased)
		return snprintf(buf, BUFF_SZ, "OK\n");
	else
		return snprintf(buf, BUFF_SZ, "NG\n");
}
static DEVICE_ATTR(nad_erase, S_IRUGO | S_IWUSR, show_quest_erase, store_quest_erase);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// BALANCER /////////////////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_main(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	QUEST_SYSFS_ENTER();

	// check exceptional cases
	if( unlikely( erased || (param_quest_data.curr_step == STEP_MAIN) || strncmp(buf, "start", 5) ) ) {
		QUEST_PRINT("%s : exceptional cases (erased=%d, curr_step=%d, buf=%s\n",
			__func__, erased, param_quest_data.curr_step, buf);
		goto out;
	}

	// update param
	QUEST_PRINT("%s : start STEP_MAIN\n", __func__);
	param_quest_data.curr_step = STEP_MAIN;
	param_quest_data.main_item_result = 0;
	param_quest_data.hlos_remained_count = 1;
#if defined(CONFIG_SEC_QUEST_UEFI)
	param_quest_data.quefi_remained_count = MAIN_QUEST_QUEFI_REPEATS;
#endif
#if defined(CONFIG_SEC_QUEST_UEFI_ENHANCEMENT)
	param_quest_data.suefi_remained_count = 1;
#endif
	param_quest_data.ddrscan_remained_count = 1;

	param_quest_data.hlos_remained_count--;
	QUEST_SET_ITEM_SUBITEM_RESULT(param_quest_data.main_item_result, QUESTHLOS_HLOS_ITEM_MAIN_CAL, ITEM_RESULT_INCOMPLETED);

	quest_sync_param_quest_data();

	// trigger quest_hlos first
	do_quest();

out:
	QUEST_SYSFS_EXIT();
	return count;
}

static ssize_t show_quest_main(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int count;
	enum quest_enum_item_result total_result = ITEM_RESULT_NONE;

	QUEST_SYSFS_ENTER();

	if( call_main_qdaf_after_finighing_main_quest == 1 ) {

		QUEST_PRINT("%s : call MAIN QDAF\n", __func__);
		// trigger MAIN QDAF in background
		run_qdaf_in_background(QUEST_QDAF_ACTION_CONTROL_START_WITH_PANIC);
	}

	total_result = check_item_result(param_quest_data.main_item_result, ITEM_ITEMSCOUNT);
	QUEST_PRINT("%s : main_item_result(%llu) total_result(%d)\n",
					__func__, param_quest_data.main_item_result, total_result);
	switch (total_result) {
		case ITEM_RESULT_PASS: {
			QUEST_PRINT("MAIN QUEST PASS\n");
			count = snprintf(buf, BUFF_SZ, "OK_2.0\n");
		} break;

		case ITEM_RESULT_FAIL: {
			QUEST_PRINT("MAIN QUEST FAIL\n");
			count = snprintf(buf, BUFF_SZ, "NG_2.0_FAIL\n");
		} break;

		case ITEM_RESULT_INCOMPLETED: {
			QUEST_PRINT("MAIN QUEST INCOMPLETED\n");
			count = snprintf(buf, BUFF_SZ, "OK\n");
		} break;

		case ITEM_RESULT_NONE: {
			QUEST_PRINT("MAIN QUEST NOT_TESTED\n");
			count = snprintf(buf, BUFF_SZ, "OK\n");
		} break;
	}

	QUEST_SYSFS_EXIT();
	return count;
}
static DEVICE_ATTR(balancer, S_IRUGO | S_IWUSR, show_quest_main, store_quest_main);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// MAIN_NAD_TIMEOUT /////////////////////////
//////////////////////////////////////////////////
static ssize_t show_main_quest_timeout(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	QUEST_SYSFS_ENTER();
	QUEST_SYSFS_EXIT();
	return snprintf(buf, BUFF_SZ, "%d\n", STEP_MAIN_HLOS_TIMEOUT);
}
static DEVICE_ATTR(timeout, 0444, show_main_quest_timeout, NULL);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// MAIN_NAD_RUN /////////////////////////////
//////////////////////////////////////////////////
static ssize_t store_main_quest_run(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	QUEST_SYSFS_ENTER();
	QUEST_SYSFS_EXIT();
	return count;
}

static ssize_t show_main_quest_run(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	QUEST_SYSFS_ENTER();
	QUEST_SYSFS_EXIT();
	return snprintf(buf, BUFF_SZ, "END\n");
}
static DEVICE_ATTR(run, S_IRUGO | S_IWUSR, show_main_quest_run, store_main_quest_run);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_QMVS_REMAIN_COUNT ///////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_hlos_remain_count(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	QUEST_SYSFS_ENTER();
	count = snprintf(buf, BUFF_SZ, "%d\n", param_quest_data.hlos_remained_count);
	QUEST_SYSFS_EXIT();

	return count;
}
static DEVICE_ATTR(nad_qmvs_remain_count, S_IRUGO, show_quest_hlos_remain_count, NULL);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_DDRTEST_REMAIN_COUNT ////////////////
//////////////////////////////////////////////////
static ssize_t show_ddrtest_remain_count(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	ssize_t count = 0;

	QUEST_SYSFS_ENTER();
	count = snprintf(buf, BUFF_SZ, "%d\n", param_quest_data.ddrscan_remained_count);
	QUEST_SYSFS_EXIT();

	return count;
}
static DEVICE_ATTR(nad_ddrtest_remain_count, S_IRUGO, show_ddrtest_remain_count, NULL);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_DRAM /////////////////////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_dram(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum quest_enum_item_result ddrscan_result = ITEM_RESULT_NONE;

	QUEST_SYSFS_ENTER();

	// The factory app needs only the ddrtest result of ACAT now.
	// If the ddrtest result of SMD and MAIN are also needed,
	// implement an additional sysfs node or a modification of app.
	ddrscan_result = QUEST_GET_ITEM_SUBITEM_RESULT(param_quest_data.cal_item_result, ITEM_DDRSCANRAMDUMPDISCACHE);

#if defined(CONFIG_SEC_QUEST_ALWAYS_RETURN_PASS_FOR_ACAT)
	QUEST_PRINT("%s : CONFIG_SEC_QUEST_ALWAYS_RETURN_PASS_FOR_ACAT enabled, so return pass\n", __func__);
	ddrscan_result = ITEM_RESULT_PASS;
#endif

	if (ddrscan_result == ITEM_RESULT_PASS)
		count = snprintf(buf, BUFF_SZ, "OK_DRAM\n");
	else if (ddrscan_result == ITEM_RESULT_FAIL)
		count = snprintf(buf, BUFF_SZ, "NG_DRAM_DATA\n");
	else
		count = snprintf(buf, BUFF_SZ, "NO_DRAMTEST\n");

	QUEST_SYSFS_EXIT();
	return count;
}
static DEVICE_ATTR(nad_dram, S_IRUGO, show_quest_dram, NULL);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_DRAM_ERR_ADDR ///////////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_dram_err_addr(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	int i = 0;

	QUEST_SYSFS_ENTER();

    count = snprintf(buf, BUFF_SZ, "Total : %d\n\n",
				param_quest_ddr_result_data.ddr_err_addr_total);
	for (i = 0; i < param_quest_ddr_result_data.ddr_err_addr_total; i++) {
		count += snprintf(buf + count - 1, BUFF_SZ, "[%d] 0x%llx\n", i,
			    	param_quest_ddr_result_data.ddr_err_addr[i]);
	}

	QUEST_SYSFS_EXIT();
	return count;
}
static DEVICE_ATTR(nad_dram_err_addr, S_IRUGO, show_quest_dram_err_addr, NULL);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_SUPPORT //////////////////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_support(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	// if you do not want to support quest, please set it with disabling CONFIG_SEC_QUEST
	return snprintf(buf, BUFF_SZ, "SUPPORT\n");
}
static DEVICE_ATTR(nad_support, S_IRUGO, show_quest_support, NULL);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_LOGS /////////////////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_logs(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int fd = 0, idx = 0;
	char path[100] = { '\0' };
	char temp[1]={'\0'}, tempbuf[BUFF_SZ]={'\0',};
	mm_segment_t old_fs = get_fs();

	//The program which was triggered with UMH_WAIT_PROC can need this function,
	//and this function does not access param, so skip enter/exit functions
	//QUEST_SYSFS_ENTER();

	QUEST_PRINT("%s : file = %s\n", __func__, buf);

	set_fs(KERNEL_DS);
	sscanf(buf, "%s", path);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
	fd = sys_open(path, O_RDONLY, 0);
#else
	fd = ksys_open(path, O_RDONLY, 0);
#endif
	if (fd >= 0) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
		while (sys_read(fd, temp, 1) == 1) {
#else
		while (ksys_read(fd, temp, 1) == 1) {
#endif
			tempbuf[idx++] = temp[0];
			if( temp[0]=='\n' ) {
				tempbuf[idx] = '\0';
				QUEST_PRINT("%s", tempbuf);
				idx = 0;
			}
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
		sys_close(fd);
#else
		ksys_close(fd);
#endif
	} else {
		QUEST_PRINT("%s : file does not exist\n", __func__);
	}
	set_fs(old_fs);

	//QUEST_SYSFS_EXIT();
	return count;
}
static DEVICE_ATTR(nad_logs, 0200, NULL, store_quest_logs);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_API //////////////////////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_api(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	QUEST_SYSFS_ENTER();

	if (param_api_gpio_test) {
		count = snprintf(buf, BUFF_SZ, "%s", param_api_gpio_test_result);
	} else
		count = snprintf(buf, BUFF_SZ, "NONE\n");

	QUEST_SYSFS_EXIT();
	return count;
}

static DEVICE_ATTR(nad_api, 0444, show_quest_api, NULL);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_INFO //////////////////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_info(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	char info_name[QUEST_BUFF_SIZE * 2] = { '\0', };
	char temp[QUEST_BUFF_SIZE * 3] = { '\0', };
	char quest_test[2][QUEST_BUFF_SIZE * 2];	// 2: "info_name", "result"
	int resultValue;
	char *quest_ptr, *string;

	QUEST_SYSFS_ENTER();

	QUEST_PRINT("buf : %s count : %d\n", buf, (int)count);
	if (QUEST_BUFF_SIZE * 3 < (int)count || (int)count < 4) {
		QUEST_PRINT("result cmd size too long : QUEST_BUFF_SIZE<%d\n",
			  (int)count);
		count = -EINVAL;
		goto out;
	}

	/* Copy buf to quest temp */
	strlcpy(temp, buf, QUEST_BUFF_SIZE * 3);
	string = temp;

	quest_ptr = strsep(&string, ",");
	strlcpy(quest_test[0], quest_ptr, QUEST_BUFF_SIZE * 2);
	quest_ptr = strsep(&string, ",");
	strlcpy(quest_test[1], quest_ptr, QUEST_BUFF_SIZE * 2);

	sscanf(quest_test[0], "%s", info_name);
	sscanf(quest_test[1], "%d", &resultValue);

	if (!strcmp("thermal", info_name))
		param_quest_data.thermal = resultValue;
	else if (!strcmp("clock", info_name))
		param_quest_data.tested_clock = resultValue;

	QUEST_PRINT("info_name : %s, result=%d\n", info_name, resultValue);

	quest_sync_param_quest_data();

out:
	QUEST_SYSFS_EXIT();
	return count;
}

static ssize_t show_quest_info(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	QUEST_SYSFS_ENTER();

	info_size +=
	    snprintf((char *)(buf + info_size), MAX_LEN_STR - info_size,
		     "\"REMAIN_CNT\":\"%d\",",
		     param_quest_data.hlos_remained_count);
	info_size +=
	    snprintf((char *)(buf + info_size), MAX_LEN_STR - info_size,
		     "\"THERMAL\":\"%d\",", param_quest_data.thermal);
	info_size +=
	    snprintf((char *)(buf + info_size), MAX_LEN_STR - info_size,
		     "\"CLOCK\":\"%d\",", param_quest_data.tested_clock);

	QUEST_SYSFS_EXIT();
	return info_size;
}

static DEVICE_ATTR(nad_info, S_IRUGO | S_IWUSR, show_quest_info, store_quest_info);
//////////////////////////////////////////////////




//////////////////////////////////////////////////
/////// NAD_VERSION //////////////////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_version(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	QUEST_SYSFS_ENTER();
	QUEST_SYSFS_EXIT();
#if 0
	//
#else
	//QUEST_1.0.1_SS_10030018_SDM855_naturescene_path
	return snprintf(buf, BUFF_SZ, "SM8150.0103.01.1030RELEASE\n");
#endif
}
static DEVICE_ATTR(nad_version, 0444, show_quest_version, NULL);
//////////////////////////////////////////////////


#if defined(CONFIG_SEC_QUEST_AUTO_TRIGGER_KWORKER) || defined(CONFIG_SEC_QUEST_AUTO_TRIGGER_INIT_WRITE)
//////////////////////////////////////////////////
// if you can auto_trigger, please run the belows first
// adb shell "setprop persist.quest.auto_repeat 1"
// adb shell "setprop debug.quest.auto_repeat.enabled 1"
//////////////////////////////////////////////////
static void quest_auto_trigger(char* test_name)
{
	char *argv[4] = { QUESTHLOS_PROG_MAIN_CAL, "logPath:/data/log/quest", "Reboot", NULL };
	//char *envp[3] = { "HOME=/", "TERM=linux", NULL, };
	char *envp[5] = {
		"HOME=/",
		"PATH=/system/bin/quest:/system/bin:/system/xbin",
		"ANDROID_DATA=/data",
		"ANDROID_ROOT=/system",
		NULL };
	int ret;

	initialized = 1;	// skip initialization routine

	QUEST_PRINT("%s : will trigger quest\n",__func__);
	QUEST_PRINT("%s : test_name (%s)\n", __func__, test_name);

	// set CALX scenario with remained_count
	if( strncmp(test_name, "HLOSUEFI", 8)==0 ||
		strncmp(test_name, "UEFIHLOS", 8)==0 )
	{
		QUEST_PRINT("%s : set CALX\n", __func__);
		param_quest_data.curr_step = STEP_CALX;
		param_quest_data.hlos_remained_count = 1;
		param_quest_data.quefi_remained_count = 1;
		param_quest_data.suefi_remained_count = 1;
		param_quest_data.ddrscan_remained_count = 0;
		quest_sync_param_quest_data();

	}else if( strncmp(test_name, "HLOSONLY", 8)==0 )
	{
		QUEST_PRINT("%s : set CALX\n", __func__);
		param_quest_data.curr_step = STEP_CALX;
		param_quest_data.hlos_remained_count = 1;
		param_quest_data.quefi_remained_count = 0;
		param_quest_data.suefi_remained_count = 0;
		param_quest_data.ddrscan_remained_count = 0;
		quest_sync_param_quest_data();

	}else if( strncmp(test_name, "UEFIONLY", 8)==0 )
	{
		QUEST_PRINT("%s : set CALX\n", __func__);
		param_quest_data.curr_step = STEP_CALX;
		param_quest_data.hlos_remained_count = 0;
		param_quest_data.quefi_remained_count = 1;
		param_quest_data.suefi_remained_count = 1;
		param_quest_data.ddrscan_remained_count = 0;
		quest_sync_param_quest_data();

	}else if( strncmp(test_name, "KILLNOW", 7)==0 )
	{
		QUEST_PRINT("%s : will kill quets.sh now\n", __func__);

		argv[0] = QUEST_DEBUGGING_PRG;
		argv[1] = "action:killnow";
		ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
		QUEST_PRINT("%s : call_usermodehelper(ret=%d)\n", __func__, ret);

	}else if( strncmp(test_name, "SYSREBOOT", 9)==0 )
	{
		QUEST_PRINT("%s : will reboot system now\n", __func__);

		argv[0] = QUEST_DEBUGGING_PRG;
		argv[1] = "action:sysreboot";
		ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
		QUEST_PRINT("%s : call_usermodehelper(ret=%d)\n", __func__, ret);

	}else
	{
		QUEST_PRINT("%s : wrong test_name\n", __func__);
		return;
	}

	// trigger hlos or reboot
	if( param_quest_data.hlos_remained_count > 0 )
	{
		QUEST_PRINT("%s : run hlos\n", __func__);
		param_quest_data.hlos_remained_count--;
		quest_sync_param_quest_data();

		// quest.sh will call reboot
		ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
		QUEST_PRINT("%s : call_usermodehelper(ret=%d)\n", __func__, ret);
	}else
	{
		msleep(3000);
		kernel_restart(NULL);
		QUEST_PRINT("%s : reboot and run uefi\n", __func__);
	}

}
#endif


#if defined(CONFIG_SEC_QUEST_AUTO_TRIGGER_INIT_WRITE)
//////////////////////////////////////////////////
/////// NAD_AUTO_TRIGGER ////////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_auto_trigger(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	char test_name[QUEST_BUFF_SIZE * 3] = { '\0', };

	strlcpy(test_name, buf, QUEST_BUFF_SIZE * 3);

	quest_auto_trigger(test_name);
	return count;
}
static DEVICE_ATTR(nad_auto_trigger, S_IWUSR, NULL, store_quest_auto_trigger);
//////////////////////////////////////////////////
#endif


#if defined(CONFIG_SEC_QUEST_AUTO_TRIGGER_KWORKER)
//////////////////////////////////////////////////
/////// NAD_AUTO_TRIGGER /////////////////////////
//////////////////////////////////////////////////
static void delayed_quest_work_func(struct work_struct *work)
{
	quest_auto_trigger("HLOSUEFI");
}
//////////////////////////////////////////////////
#endif


//////////////////////////////////////////////////
/////// NAD_TESTMODE /////////////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_testmode(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int idx = 0, flag = 0;
	char temp[QUEST_BUFF_SIZE * 3];
	char quest_cmd[QUEST_CMD_LIST][QUEST_BUFF_SIZE];
	char *quest_ptr, *string;

	QUEST_SYSFS_ENTER();

	// parse argument
	strlcpy(temp, buf, QUEST_BUFF_SIZE * 3);
	string = temp;
	while (idx < QUEST_CMD_LIST) {
		quest_ptr = strsep(&string, ",");
		strlcpy(quest_cmd[idx++], quest_ptr, QUEST_BUFF_SIZE);
	}

	QUEST_PRINT("%s : %s(%s)\n", __func__, quest_cmd[0], quest_cmd[1]);

	sscanf(quest_cmd[1], "%d", &flag);

	if( strncmp(quest_cmd[0], "testmode", 8)==0 ) testmode_enabled = flag;
	else if( strncmp(quest_cmd[0], "quefi", 5)==0 ) {
		testmode_quefi_enabled = flag;
		// if flag is updated after setting repeating count, we should set count also here
		if( flag )
			param_quest_data.quefi_remained_count = param_quest_data.hlos_remained_count;
		else
			param_quest_data.quefi_remained_count = 0;
	}
	else if( strncmp(quest_cmd[0], "suefi", 5)==0 ) {
		testmode_suefi_enabled = flag;
		// if flag is updated after setting repeating count, we should set count also here
		if( flag )
			param_quest_data.suefi_remained_count = param_quest_data.hlos_remained_count;
		else
			param_quest_data.suefi_remained_count = 0;
	}
	else if( strncmp(quest_cmd[0], "ddrscan", 7)==0 ) {
		testmode_ddrscan_enabled = flag;
		// if flag is updated after setting repeating count, we should set count also here
		if( flag )
			param_quest_data.ddrscan_remained_count = param_quest_data.hlos_remained_count;
		else
			param_quest_data.ddrscan_remained_count = 0;
	}
	quest_sync_param_quest_data();

	QUEST_SYSFS_EXIT();

	return count;
}
static DEVICE_ATTR(nad_testmode, S_IWUSR, NULL, store_quest_testmode);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_INIT_STEP ////////////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_init_step(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	QUEST_SYSFS_ENTER();

	// check exceptional cases
	if( unlikely( strncmp(buf, "init_step", 9) ) ) {
		QUEST_PRINT("%s : exceptional cases (buf=%s)\n", __func__, buf);
		goto out;
	}

	QUEST_PRINT("%s : call quest_initialize_curr_step\n", __func__);
	quest_initialize_curr_step();

out:
	QUEST_SYSFS_EXIT();
	return count;
}
static DEVICE_ATTR(nad_init_step, S_IRUGO | S_IWUSR, NULL, store_quest_init_step);
//////////////////////////////////////////////////




//////////////////////////////////////////////////
/////// NAD_NATURESCENE_INFO /////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_smd_info(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int idx = 0;
	char temp[QUEST_CMD_LIST * QUEST_CMD_SIZE];
	char quest_cmd[QUEST_CMD_LIST][QUEST_CMD_SIZE];
	char *quest_ptr, *string;

	QUEST_SYSFS_ENTER();

	if( param_quest_data.curr_step != STEP_SMDDL ) {
		QUEST_PRINT("%s : The smd info should be updated only at SMDDL\n", __func__);
		goto out;
	}

	// parse argument
	strlcpy(temp, buf, QUEST_CMD_SIZE);
	string = temp;
	while (idx < QUEST_CMD_LIST) {
		quest_ptr = strsep(&string, ",");
		strlcpy(quest_cmd[idx++], quest_ptr, QUEST_CMD_SIZE);
	}

	QUEST_PRINT("%s : %s(%s)\n", __func__, quest_cmd[0], quest_cmd[1]);

	if( strncmp(quest_cmd[0], "smd_boot_reason", 15)==0 ) {
		strncpy(param_quest_data.smd_boot_reason, quest_cmd[1], 2);
		QUEST_UPDATE_SMDDL_INFO_WITH_STRING(param_quest_data.smd_boot_reason_first, quest_cmd[1], 2);
	}

	if( strncmp(quest_cmd[0], "smd_hlos_start_time", 19)==0 ) {
		sscanf(quest_cmd[1], "%d", &param_quest_data.smd_hlos_start_time);
		QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_hlos_start_time);
	}

	if( strncmp(quest_cmd[0], "smd_hlos_elapsed_time", 21)==0 ) {
		sscanf(quest_cmd[1], "%d", &param_quest_data.smd_hlos_elapsed_time);
		QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_hlos_elapsed_time);
	}

	if( strncmp(quest_cmd[0], "smd_hlos_init_thermal", 21)==0 ) {
		sscanf(quest_cmd[1], "%d", &param_quest_data.smd_hlos_init_thermal);
		QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_hlos_init_thermal);
	}

	if( strncmp(quest_cmd[0], "smd_hlos_max_thermal", 20)==0 ) {
		sscanf(quest_cmd[1], "%d", &param_quest_data.smd_hlos_max_thermal);
		QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_hlos_max_thermal);
	}

	if( strncmp(quest_cmd[0], "smd_ns_repeats", 14)==0 ) {
		sscanf(quest_cmd[1], "%d", &param_quest_data.smd_ns_repeats);
		QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_ns_repeats);
	}

	if( strncmp(quest_cmd[0], "smd_max_aoss_thermal_diff", 25)==0 ) {
		sscanf(quest_cmd[1], "%d", &param_quest_data.smd_max_aoss_thermal_diff);
		QUEST_UPDATE_SMDDL_INFO(param_quest_data.smd_max_aoss_thermal_diff);
	}	

	quest_sync_param_quest_data();

out:
	QUEST_SYSFS_EXIT();
	return count;
}

static DEVICE_ATTR(nad_smd_info, S_IWUSR, NULL, store_quest_smd_info);
//////////////////////////////////////////////////


#endif	// #if defined(CONFIG_SEC_FACTORY)


static ssize_t show_quest_fv_flashed(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	//QUEST_SYSFS_ENTER();
	//QUEST_SYSFS_EXIT();
	mutex_lock(&sysfs_common_lock);
	quest_load_param_quest_data();
	mutex_unlock(&sysfs_common_lock);

	return snprintf(buf, BUFF_SZ, "%d\n", param_quest_data.quest_fv_flashed);
}

static DEVICE_ATTR(quest_fv_flashed, 0444, show_quest_fv_flashed, NULL);



static int __init sec_quest_init(void)
{
	int ret = 0;

	QUEST_PRINT("%s\n", __func__);

	/* Skip quest init when device goes to lp charging */
	if (lpcharge)
		return ret;

	// init mutex
	mutex_init(&sysfs_common_lock);

	// sec_nad, sec_nad_balancer device
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
	sec_nad = sec_device_create(0, NULL, "sec_nad");
#else
	sec_nad = sec_device_create(NULL, "sec_nad");
#endif
	if (IS_ERR(sec_nad)) {
		QUEST_PRINT("%s Failed to create device(sec_nad)!\n", __func__);
		return PTR_ERR(sec_nad);
	}

	ret = device_create_file(sec_nad, &dev_attr_quest_fv_flashed);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}	

#if defined(CONFIG_SEC_FACTORY)	

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
	sec_nad_balancer = sec_device_create(0, NULL, "sec_nad_balancer");
#else
	sec_nad_balancer = sec_device_create(NULL, "sec_nad_balancer");
#endif
	if (IS_ERR(sec_nad_balancer)) {
		QUEST_PRINT("%s Failed to create device(sec_nad)!\n", __func__);
		return PTR_ERR(sec_nad_balancer);
	}


	// sysfs nodes at /sys/class/sec/sec_nad
	ret = device_create_file(sec_nad, &dev_attr_nad_stat);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_ddrtest_remain_count);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_qmvs_remain_count);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_erase);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_acat);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_dram);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_support);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_logs);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_end);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_dram_err_addr);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_result);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_api);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_info);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_version);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_testmode);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_smd_info);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_init_step);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

#if defined(CONFIG_SEC_QUEST_AUTO_TRIGGER_INIT_WRITE)
	ret = device_create_file(sec_nad, &dev_attr_nad_auto_trigger);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}
#endif

	// sysfs nodes at /sys/class/sec/sec_nad_balancer
	ret = device_create_file(sec_nad_balancer, &dev_attr_balancer);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad_balancer, &dev_attr_timeout);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad_balancer, &dev_attr_run);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}


	// uevent for factory app
	if (add_uevent_var(&quest_uevent, "NAD_TEST=%s", "DONE")) {
		QUEST_PRINT("%s : uevent NAD_TEST_AND_PASS is failed to add\n",
		       __func__);
		goto err_create_nad_sysfs;
	}

#if defined(CONFIG_SEC_QUEST_AUTO_TRIGGER_KWORKER)
	INIT_DELAYED_WORK(&trigger_quest_work, delayed_quest_work_func);
	schedule_delayed_work(&trigger_quest_work, msecs_to_jiffies(WAIT_TIME_BEFORE_TRIGGER_MSECS));
#endif

	atomic_notifier_chain_register(&panic_notifier_list, &quest_panic_block);
#endif

	return 0;
err_create_nad_sysfs:
	return ret;
}

module_init(sec_quest_init);
